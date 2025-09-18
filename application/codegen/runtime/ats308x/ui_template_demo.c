/*
 * Copyright 2025 wtcat 
 */

#include "ui_template.h"
#include "app_ui_view.h"
#include "find_phone_view.h"

#define PIC_NUMBERS 4
#define STR_NUMBERS 2

typedef struct {
    lv_obj_t* obj;
    lv_obj_t* time;
    lv_obj_t* find_phone_start;
    lv_obj_t* find_phone_dsiconnected;
    lv_obj_t* find_phone_start_icon;

    ui_font_t font;
    ui_font_t font1;
    ui_font_t font34;

    lv_anim_t anim;

    lv_img_dsc_t res_img[PIC_NUMBERS];
    lv_img_dsc_t shared_img;
    ui_string_t  res_txt[STR_NUMBERS];

} demo_private_t;

static void find_phone_event_handler(lv_event_t* e) {
    ui_context_t* ctx = lv_event_get_user_data(e);
    const find_phone_test_view_presenter_t* presenter = ui_context_get_presenter(ctx);
    demo_private_t* data = ui_context_get_user(ctx);

    presenter->find_phone_start();
    lv_anim_start(&data->anim);
}

static void find_phone_connect_status_anim_set_temp(void* user_data, int32_t temp) {
    ui_context_t* ctx = user_data;
    demo_private_t* data = ui_context_get_user(ctx);
    const find_phone_test_view_presenter_t* presenter = ui_context_get_presenter(ctx);

    if (!presenter->get_ble_connected()) {
        lv_obj_clear_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
    }
}

static void find_phone_anim_set_temp(void* user_data, int32_t temp) {
    ui_context_t* ctx = user_data;
    demo_private_t* data = ui_context_get_user(ctx);
    const find_phone_test_view_presenter_t* presenter = ui_context_get_presenter(ctx);
    if (!presenter->get_phone_status()) {
        lv_img_set_src(data->find_phone_start_icon, &data->res_img[2]);
        lv_anim_del(NULL, find_phone_anim_set_temp);
        return;
    }

    if (temp % 2 == 0)
        lv_img_set_src(data->find_phone_start_icon, &data->res_img[1]);
    else
        lv_img_set_src(data->find_phone_start_icon, &data->res_img[3]);
}

static int demo_on_create(ui_context_t* ctx) {
    demo_private_t* priv = ui_context_get_user(ctx);
    int err;

    err = ui_context_set_resource_namespace(ctx, RES_NS_LOCAL_SHARED);
    if (err)
        return err;

    err = ui_context_get_picture(ctx, &priv->shared_img, 1, __RE("PIC_RECENT_64"));
    if (err)
        return err;

    err = ui_context_set_resource_namespace(ctx, RES_NS_LOCAL_PRIVATE);
    if (err)
        return err;
    /*
     * Get all local resources  
     */
    err = ui_context_get_picture(ctx, priv->res_img, PIC_NUMBERS, 
        __RE("PIC_FIND_PHONE"),     // 0
        __RE("PIC_FIND_PHONE_ING"), // 1
        __RE("PIC_PHONE_NEW0"),     // 2
        __RE("PIC_PHONE_NEW1"));
    if (err)
        return err;

    err = ui_conext_get_text(ctx, priv->res_txt, STR_NUMBERS,
        __RE("STR_TITLE"),
        __RE("STR_DISCONNECTED"));
    if (err)
        return err;

    err = ui_context_get_font(ctx, DEF_VFONT_FILE, DEF_FONT32_FILE, &priv->font);
    if (err)
        return err;

    err = ui_context_get_font(ctx, DEF_VFONT_FILE, DEF_FONT24_FILE, &priv->font1);
    if (err)
        return err;

    err = ui_context_get_font(ctx, DEF_VFONT_FILE, DEF_FONT36_FILE, &priv->font34);
    if (err)
        return err;

    /*
     * Create lvgl widgets 
     */
    lv_obj_t* scr = lv_disp_get_scr_act(ui_context_get_display(ctx));
    demo_private_t* data = ui_context_get_user(ctx);
    const find_phone_test_view_presenter_t* presenter = ui_context_get_presenter(ctx);
    if (data) {
        data->obj = lv_obj_create(scr);
        lv_obj_set_pos(data->obj, 0, 0);
        lv_obj_set_size(data->obj, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

        lv_obj_t* find_phone_title = app_ui_label_create(data->obj, 0, 0, 236, 0, 
            WHITE_COLOR, data->res_txt[0].lv_text, &data->font34.lv_font);
        lv_obj_align(find_phone_title, LV_ALIGN_TOP_MID, 0, 33);
        lv_obj_set_style_text_align(find_phone_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        data->find_phone_start = lv_obj_create(data->obj);
        lv_obj_set_pos(data->find_phone_start, 0, 97);
        lv_obj_set_size(data->find_phone_start, DEF_UI_VIEW_WIDTH, 466 - 97);

        data->find_phone_start_icon = lv_img_create(data->find_phone_start);
        lv_obj_set_pos(data->find_phone_start_icon, 153, 56);
        lv_obj_set_size(data->find_phone_start_icon, data->res_img[2].header.w, data->res_img[2].header.h);
        lv_img_set_src(data->find_phone_start_icon, &data->res_img[2]);
        lv_obj_add_flag(data->find_phone_start_icon, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(data->find_phone_start_icon, find_phone_event_handler, LV_EVENT_SHORT_CLICKED, ctx);

        data->find_phone_dsiconnected = lv_obj_create(data->obj);
        lv_obj_set_pos(data->find_phone_dsiconnected, 0, 97);
        lv_obj_set_size(data->find_phone_dsiconnected, DEF_UI_VIEW_WIDTH, 466 - 97);

        app_ui_label_paint(data->find_phone_dsiconnected, 65, 116, 336, lv_font_get_line_height(&data->font.lv_font), WHITE_COLOR, 
            data->res_txt[1].lv_text, &data->font.lv_font, LV_TEXT_ALIGN_CENTER);

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_exec_cb(&anim, find_phone_connect_status_anim_set_temp);
        lv_anim_set_time(&anim, 4000);
        lv_anim_set_var(&anim, ctx);
        lv_anim_set_values(&anim, 3, -1);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&anim);

        lv_anim_init(&data->anim);
        lv_anim_set_exec_cb(&data->anim, find_phone_anim_set_temp);
        lv_anim_set_time(&data->anim, 5000);
        lv_anim_set_var(&data->anim, ctx);
        lv_anim_set_values(&data->anim, 0, 10);
        lv_anim_set_repeat_count(&data->anim, LV_ANIM_REPEAT_INFINITE);

        if (!presenter->get_ble_connected()) {
            lv_obj_clear_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
            presenter->find_phone_start();
            lv_anim_start(&data->anim);
        }

#ifdef CONFIG_SIMULATOR
        if (presenter->get_view_status() == 1) {
            lv_obj_clear_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
        }
        if (presenter->get_view_status() == 2) {
            lv_img_set_src(data->find_phone_start_icon, &data->res_img[1]);
            lv_obj_add_flag(data->find_phone_dsiconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(data->find_phone_start, LV_OBJ_FLAG_HIDDEN);
        }
#endif
    }

    return 0;
}

static int demo_on_destroy(ui_context_t* ctx) {
    demo_private_t* data = ui_context_get_user(ctx);

    lv_anim_del(NULL, find_phone_connect_status_anim_set_temp);
    lv_anim_del(NULL, find_phone_anim_set_temp);

    return 0;
}

static int demo_on_key(ui_context_t* ctx, int keyid, int keyevt, bool* done) {
    const find_phone_test_view_presenter_t* presenter = ui_context_get_presenter(ctx);

    if (keyevt == UIVIEW_KEY_RELEASE && keyid == UIVIEW_KEY_1)
        presenter->go_back_view();

    if (keyevt == UIVIEW_KEY_RELEASE && keyid == UIVIEW_KEY_HOME) {
        extern void first_scree_ui_enter();
        first_scree_ui_enter(); /* go back to the CLOCK_VIEW */
    }

    return 0;
}


UI_VIEW_DEFINE(demo_view) = {
    .on_create        = demo_on_create,
    .on_focus_change  = NULL,
    .on_paint         = NULL,
    .on_destroy       = demo_on_destroy,
    .on_key           = demo_on_key,
    .user_size        = sizeof(demo_private_t),
    VIEW_PRIV("general")
};

LVGL_VIEW_DEFINE(10, &demo_view);
