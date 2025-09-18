/*
 * Copyright 2025 wtcat 
 */
#ifndef UI_TEMPLATE_H_
#define UI_TEMPLATE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_TEMPLATE_VERSION "2.0.0"

#include <lvgl/lvgl.h>

#define UI_LOCAL_API        static
#define UI_PUBLIC_API
#define UI_ALIGNED_UP(v, a) (((v) + ((a) - 1)) & ~((a) - 1))
#define UI_ARRAY_SIZE(a)    (sizeof(a) / sizeof(a[0]))

#ifndef CONFIG_SIMULATOR
#define UI_VIEW_DEFINE(_name) static const ui_view_t _name
#else
#define UI_VIEW_DEFINE(_name) const ui_view_t _name
#endif /* !CONFIG_SIMULATOR */
    
struct ui_context;

/*
 * Keypad ID list
 */
#define UIVIEW_KEY_HOME      0
#define UIVIEW_KEY_1         1
#define UIVIEW_KEY_2         2

/*
 * Key event list
 */
#define UIVIEW_KEY_PRESSED      0
#define UIVIEW_KEY_RELEASE      1
#define UIVIEW_KEY_LONGPRESSED  2

enum resource_namespace {
    RES_NS_LOCAL_PRIVATE,  /* The local resource that just only used by current view */
    RES_NS_LOCAL_SHARED,   /* The local resource that used by many views */
    RES_NS_EXTERN_SHARED,  /* The extern resource that used by many views */
    RES_NS_MAX
};

typedef struct ui_view {
    int (*on_create)(struct ui_context* ctx);
    int (*on_focus_change)(struct ui_context* ctx, bool focuse);
    int (*on_paint)(struct ui_context* ctx);
    int (*on_destroy)(struct ui_context* ctx);
    int (*on_key)(struct ui_context* ctx, int keyid, int keyevt, bool* done);

    size_t   user_size; /* The size of user private data */
    void     *private;

} ui_view_t;

typedef struct ui_context {
#define UI_VIEW_IF(_ctx, _fn, ...) \
    ((struct ui_context *)(_ctx))->view._fn(_ctx, ##__VA_ARGS__)
    /* In order to performance and not use pointer */
    ui_view_t view;

    const void* presenter;
    void* display; /* LVGL display object */
    void* user;    /* User private data */
} ui_context_t;

typedef struct ui_string_ {
    void*  lv_text;
    //size_t len;
} ui_string_t;

typedef struct ui_font_ {
    lv_font_t        lv_font;

    /* Private member */
    struct ui_font_* next;
} ui_font_t;

UI_PUBLIC_API int
ui_context_set_resource_namespace(ui_context_t* ctx, enum resource_namespace ns);

UI_PUBLIC_API int
ui_context_set_refresh_period(ui_context_t* ctx, unsigned int ms);

UI_PUBLIC_API int
ui_context_get_font(ui_context_t* ctx, const char *name, const int id, ui_font_t* font);

UI_PUBLIC_API int
ui_context_get_picture(ui_context_t* ctx, lv_img_dsc_t* src, size_t n, ...);

UI_PUBLIC_API int
ui_conext_get_text(ui_context_t* ctx, ui_string_t* str, size_t n, ...);


static inline const void*
ui_context_get_presenter(struct ui_context* ctx) {
    return ctx->presenter;
}

static inline void*
ui_context_get_display(struct ui_context* ctx) {
    return ctx->display;
}

static inline void*
ui_context_get_user(struct ui_context* ctx) {
    return ctx->user;
}


#include "ui_template_port.h"

UI_PUBLIC_API int
ui_context_get_picture_set(ui_context_t* ctx, ui_picture_set_t* set, size_t n, ...);

UI_PUBLIC_API void
ui_context_set_picset(ui_context_t* ctx, lv_obj_t* obj, ui_picture_set_t* set);


#ifndef __RE
#define __RE(x) (x)
#endif

#ifdef __cplusplus
}
#endif
#endif /* UI_TEMPLATE_H_ */
