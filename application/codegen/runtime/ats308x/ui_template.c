/*
 * Copyright (c) 2025 wtcat 
 * 
 * SDK UI message service and resource manager adapt layer
 */

#include <stdarg.h>
#include <assert.h>
#include <app_ui.h>

#include "ui_template.h"
#include "app_ui_view.h"

typedef struct {
    void**    object;
    uint16_t  numbers;
    uint16_t  index;
} sdk_ui_object_t;

typedef struct {
    lvgl_res_scene_t            scene;
    const sdk_resources_t*      resource;
} sdk_resource_node_t;

typedef struct {
    /* Must be place at first */
    ui_context_t                 ui;
    sdk_resource_node_t*         ns_vector[RES_NS_MAX];
    enum resource_namespace      ns;
    uint16_t                     view_id;
    ui_font_t*                   fonts_opened;
    sdk_ui_object_t              picset;
    sdk_ui_object_t              pic;
    sdk_ui_object_t              text;
    const sdk_resource_ns_t     *ns_class;
} sdk_ui_context_t;

#ifndef unlikely
#define unlikely(x) (x)
#endif

#define to_sdkctx(_ctx) (sdk_ui_context_t *)(_ctx)
#define font_opened_foreach(_font, _head) \
    for (ui_font_t **_font = &(_head); *_font != NULL; _font = &(*_font)->next)

/*
 * Memory allocate/free interface 
 */
#define UI_MEM_MALLOC(size) app_mem_malloc(size)
#define UI_MEM_FREE(ptr)    app_mem_free(ptr)

UI_LOCAL_API inline bool
ui_context_need_destroy(view_data_t* view_data) {
    return view_data->user_data && (uintptr_t)view_data->presenter <= 1;
}

UI_LOCAL_API int 
default_on_operation(struct ui_context* ctx) {
    // Empty
    return 0;
}

UI_LOCAL_API int
ui_context_add_object(sdk_ui_object_t* sdk_obj, void* ptr) {
    if (sdk_obj->index >= sdk_obj->numbers)
        return -ENOMEM;

    sdk_obj->object[sdk_obj->index] = ptr;
    sdk_obj->index++;
    return 0;
}

UI_LOCAL_API void
ui_context_free_object(sdk_ui_object_t* sdk_obj, void (*release)(void *ptr, uint32_t)) {
    int count = sdk_obj->index;

    while (--count >= 0) {
        (*release)(sdk_obj->object[count], 1);
        sdk_obj->object[count] = NULL;
    }
}

UI_LOCAL_API void 
ui_view_copy(ui_view_t* dst, const ui_view_t* src) {
    size_t n = offsetof(ui_view_t, user_size) / sizeof(void*);
    void** pdst = (void **)dst;

    *dst = *src;
    for (size_t i = 0; i < n; i++) {
        if (pdst[i] == NULL)
            pdst[i] = (void *)default_on_operation;
    }
}

UI_LOCAL_API void
ui_res_unload_strings(void* txt, uint32_t num) {
    lvgl_res_string_t str;
    str.txt = txt;
    lvgl_res_unload_strings(&str, num);
}

UI_LOCAL_API inline int
ui_context_focus_change(view_data_t* view_data, bool focus) {
    /* Fix message order error */
    if (!view_data->presenter)
        return UI_VIEW_IF(view_data->user_data, on_focus_change, focus);
    return 0;
}

UI_LOCAL_API int
ui_context_create(view_data_t* view_data, uint16_t view_id) {
    const ui_view_t* view = view_data->user_data;
    const sdk_resource_ns_t* ns_class = _sdk_get_resource_ns_class(view->private);
    const sdk_resources_t* r = ns_class->get_ns_resource[RES_NS_LOCAL_PRIVATE](view_id);
    size_t sdk_size  = UI_ALIGNED_UP(sizeof(sdk_ui_context_t), sizeof(void*));
    size_t user_size = UI_ALIGNED_UP(view->user_size, sizeof(void*));
    size_t picture_size = r->picture_num;
    size_t text_size = r->text_num;
    size_t picset_size = r->pictureset_num;
    size_t ctx_size = sdk_size + user_size;
    sdk_ui_context_t* ctx;
    int err;

    /* Calculate object size that need to be allocate */
    picture_size += SHA_PICTURE_MAX;
    text_size    += SHA_STRING_MAX;

    picture_size *= sizeof(void*);
    text_size    *= sizeof(void*);
    picset_size  *= sizeof(void*);
    ctx_size     += picture_size + text_size + picset_size;

    /* Allocate ui context */
    ctx = UI_MEM_MALLOC(ctx_size);
    if (ctx == NULL) {
        view_data->user_data = NULL;
        return -ENOMEM;
    }

    /* Initialize ui context */
    memset(ctx, 0, ctx_size);
    ui_view_copy(&ctx->ui.view, view);
    ctx->ui.display       = view_data->display;
    ctx->ui.presenter     = view_data->presenter;
    ctx->ui.user          = (void *)((char *)ctx + sdk_size);
    ctx->pic.object       = (void **)((char *)ctx->ui.user + user_size);
    ctx->pic.numbers      = (uint16_t)(picture_size / sizeof(void*));
    ctx->text.object      = (void **)((char*)ctx->pic.object + picture_size);
    ctx->text.numbers     = (uint16_t)(text_size / sizeof(void*));
    ctx->picset.object    = (void**)((char*)ctx->text.object + text_size);
    ctx->picset.numbers   = (uint16_t)(picset_size / sizeof(void*));
    ctx->ns               = RES_NS_LOCAL_PRIVATE;
    ctx->view_id          = view_id;
    ctx->ns_class         = ns_class;

    view_data->user_data  = ctx;
    view_data->presenter  = (void*)1;

    /* Create local private resource node */
    ctx->ns_vector[ctx->ns] = UI_MEM_MALLOC(sizeof(sdk_resource_node_t));
    if (ctx->ns_vector[ctx->ns] == NULL)
        return -ENOMEM;

    /* Load current view scene for resoruce load */
    memset(ctx->ns_vector[ctx->ns], 0, sizeof(sdk_resource_node_t));
    ctx->ns_vector[ctx->ns]->resource = r;
    const sdk_resource_file_t* file = ns_class->files.local_file;
    err = app_ui_lvgl_res_load_scene(r->scene_id, &ctx->ns_vector[ctx->ns]->scene, 
        file->sty_file, file->pic_file, file->txt_file);
    if (err)
        return err;

    /* Execute user view create code */
    err = UI_VIEW_IF(&ctx->ui, on_create);
    if (err == 0) {
        /* 
         * Set focus to current view 
         */
        uint16_t focus = view_manager_get_focused_view();
        view_data->presenter = NULL;
        if (view_id == focus)
            err = ui_context_focus_change(view_data, true);
    }

    return err;
}

UI_LOCAL_API int
ui_context_destroy(view_data_t* view_data) {
    if (ui_context_need_destroy(view_data)) {
        ui_context_t* ctx = view_data->user_data;
        sdk_ui_context_t* sdkctx = to_sdkctx(ctx);

        /* Execute user destroy callback */
        UI_VIEW_IF(ctx, on_destroy);

        /* Release font resource */
        font_opened_foreach(font, sdkctx->fonts_opened)
            _sdk_font_close(&(*font)->lv_font);

        /* Release picture set resource */
        ui_context_free_object(&sdkctx->picset, (void *)lvgl_res_unload_picregion);

        /* Release picture resource */
        ui_context_free_object(&sdkctx->pic, lvgl_res_unload_pictures);

        /* Release text resource */
        ui_context_free_object(&sdkctx->text, ui_res_unload_strings);

        /* Release all resource nodes */
        for (int ns = 0; ns < RES_NS_MAX; ns++) {
            sdk_resource_node_t* pnode = sdkctx->ns_vector[ns];
            if (pnode) {
                sdkctx->ns_vector[ns] = NULL;
                lvgl_res_unload_scene(&pnode->scene);
                lvgl_res_unload_scene_compact(pnode->resource->scene_id);
                UI_MEM_FREE(pnode);
            }
        }

        /* Release ui context */
        UI_MEM_FREE(ctx);

        view_data->user_data = NULL;
    }

    return 0;
}

UI_LOCAL_API int
ui_context_key_process(ui_key_msg_data_t* key) {
    view_data_t* view_data = key->view_data;
    if (view_data->presenter)
        return 0;

    ui_context_t* ctx = view_data->user_data;
    int id, event;

    /* Get keypad ID */
    switch (key->event & 0xFF) {
    case KEY_POWER: 
        id = UIVIEW_KEY_HOME;
        break;
    case KEY_NUM0:
        id = UIVIEW_KEY_1;
        break;
    case KEY_NUM1:
        id = UIVIEW_KEY_2;
        break;
    default:
        id = -1;
        break;
    }

    /* Get keypad event */
    switch (key->event & KEY_TYPE_ALL) {
    case KEY_TYPE_SHORT_DOWN:
        event = UIVIEW_KEY_PRESSED;
        break;
    case KEY_TYPE_SHORT_UP:
        event = UIVIEW_KEY_RELEASE;
        break;
    case KEY_TYPE_LONG_DOWN:
        event = UIVIEW_KEY_LONGPRESSED;
        break;
    default:
        event = -1;
        break;
    }

    /* Execute user callback */
    key->done = true;
    return UI_VIEW_IF(ctx, on_key, id, event, &key->done);
}

UI_LOCAL_API int
ui_context_preload(view_data_t* view_data, uint32_t view_id) {
    const ui_view_t* view = view_data->user_data;
    const sdk_resource_ns_t* ns_class = _sdk_get_resource_ns_class(view->private);
    const sdk_resources_t* r = ns_class->get_ns_resource[RES_NS_LOCAL_PRIVATE](view_id);

    if (!lvgl_res_scene_is_loaded(r->scene_id)) {
        const sdk_resource_file_t* file = ns_class->files.local_file;
        lvgl_res_preload_scene_compact_default_init(r->scene_id, NULL, 0, NULL,
            file->sty_file, file->pic_file, file->txt_file);
    }

    return lvgl_res_preload_scene_compact_default(r->scene_id,
        view_id, 0, 0);
}

UI_PUBLIC_API int
_sdk_view_handler(uint16_t view_id, uint8_t msg_id, void* msg_data) {

    VIEW_HANDLER_LOG
    switch (msg_id) {
    case MSG_VIEW_PRELOAD:
        return ui_context_preload(msg_data, view_id);

    case MSG_VIEW_LAYOUT:
        return ui_context_create(msg_data, view_id);

    case MSG_VIEW_FOCUS:
        return ui_context_focus_change(msg_data, true);

    case MSG_VIEW_DEFOCUS:
        return ui_context_focus_change(msg_data, false);

    case MSG_VIEW_DELETE:
        return ui_context_destroy(msg_data);
        
    case MSG_VIEW_KEY:
        return ui_context_key_process(msg_data);

    default:
        return 0;
    }
}

UI_PUBLIC_API int
ui_context_set_resource_namespace(ui_context_t* ctx, enum resource_namespace ns) {
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);

    if (sdkctx->ns_vector[ns] == NULL) {
        assert(ns > 0);
        uint16_t shared_view = sdkctx->ns_class->view_array[ns - 1];
        const sdk_resources_t* r = sdkctx->ns_class->get_ns_resource[ns](shared_view);
        if (r == NULL)
            return -ENODATA;

        sdk_resource_node_t* pnode = UI_MEM_MALLOC(sizeof(sdk_resource_node_t));
        if (pnode == NULL)
            return -ENOMEM;

        sdkctx->ns_vector[ns] = pnode;

        const sdk_resource_file_t* file = sdkctx->ns_class->file_array[ns - 1];
        int err = app_ui_lvgl_res_load_scene(r->scene_id, &pnode->scene,
            file->sty_file, file->pic_file, file->txt_file);
        if (err)
            return err;

        pnode->resource = r;
    }

    sdkctx->ns = ns;
    return 0;
}

UI_PUBLIC_API int
ui_context_get_picture(ui_context_t* ctx, lv_img_dsc_t* src, size_t n, ...) {
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);
    sdk_resource_node_t* pnode = sdkctx->ns_vector[sdkctx->ns];
    lv_point_t unused;
    va_list ap;
    int err = -EINVAL;

    if (unlikely(pnode == NULL))
        return err;

    va_start(ap, n);
    for (size_t i = 0; i < n; i++) {
        unsigned long key = va_arg(ap, unsigned long);
        err = lvgl_res_load_pictures_from_scene(&pnode->scene,
            &key, src, &unused, 1);
        if (err)
            goto _end;

        err = ui_context_add_object(&sdkctx->pic, src);
        if (err)
            goto _end;

        src++;
    }

_end:
    va_end(ap);
    return err;
}

UI_PUBLIC_API int
ui_conext_get_text(ui_context_t* ctx, ui_string_t* str, size_t n, ...) {
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);
    sdk_resource_node_t* pnode = sdkctx->ns_vector[sdkctx->ns];
    lvgl_res_string_t text;
    va_list ap;
    int err = -EINVAL;

    if (unlikely(pnode == NULL))
        return err;

    va_start(ap, n);
    for (size_t i = 0; i < n; i++) {
        unsigned long key = va_arg(ap, unsigned long);
        err = lvgl_res_load_strings_from_scene(&pnode->scene,
            &key, &text, 1);
        if (err)
            goto _end;

        err = ui_context_add_object(&sdkctx->text, text.txt);
        if (err)
            goto _end;

        str->lv_text = text.txt;
        str++;
    }

_end:
    va_end(ap);
    return err;
}

UI_PUBLIC_API int
ui_context_get_font(ui_context_t* ctx, const char* name, const int id, ui_font_t* font) {
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);
    int err;

    /* Walk around opened list and find request font object */
    font_opened_foreach(f, sdkctx->fonts_opened) {
        if (*f == font) {
            /* Remove it from opened list */
            *f = font->next;
            font->next = NULL;

            /* Release font resource */
            _sdk_font_close(&font->lv_font);
        }
    }

    err = _sdk_font_open(&font->lv_font, name, id);
    if (err == 0) {
        font->next = sdkctx->fonts_opened;
        sdkctx->fonts_opened = font;
    }

    return err;
}

UI_PUBLIC_API int
ui_context_get_picture_set(ui_context_t* ctx, ui_picture_set_t* set, size_t n, ...) {
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);
    sdk_resource_node_t* pnode = sdkctx->ns_vector[sdkctx->ns];
    va_list ap;
    int err;

    va_start(ap, n);
    for (size_t i = 0; i < n; i++) {
        err = lvgl_res_load_picregion_from_scene(&pnode->scene, 
            va_arg(ap, unsigned long), set);
        if (err)
            return err;

        err = ui_context_add_object(&sdkctx->picset, set);
        if (err)
            return err;

        set++;
    }

    va_end(ap);
    return 0;
}

UI_PUBLIC_API void
ui_context_set_picset(ui_context_t* ctx, lv_obj_t* obj, ui_picture_set_t* set) {
extern void anim_img_set_src_picregion(lv_obj_t * obj, uint32_t scene_id, 
    lvgl_res_picregion_t * picregion);
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);
    sdk_resource_node_t* pnode = sdkctx->ns_vector[sdkctx->ns];
    anim_img_set_src_picregion(obj, pnode->resource->scene_id, set);
}

UI_PUBLIC_API int
ui_context_set_refresh_period(ui_context_t* ctx, unsigned int ms) {
extern int ui_focused_notify(const uint16_t * view, int period);
extern int ui_defocused_notify(const uint16_t * view);
    sdk_ui_context_t* sdkctx = to_sdkctx(ctx);

    if (ms > 0)
        ui_focused_notify(&sdkctx->view_id, ms);
    else
        ui_defocused_notify(&sdkctx->view_id);
    return 0;
}
