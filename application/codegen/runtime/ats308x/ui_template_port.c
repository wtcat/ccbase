/*
 * Copyright 2025 wtcat 
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "ui_template.h"

#if defined(__GNUC__) || defined(__clang__)
#define WEAK_FUNC __attribute__((weak))
#else
#define WEAK_FUNC
#endif

typedef struct {
    const char* name;
    sdk_resource_ns_t ns;
} sdk_resource_ns_method_t;


#ifndef CONFIG_SIMULATOR
WEAK_FUNC
const sdk_resources_t* _sdk_view_get_resource(uint16_t view_id) {
    return NULL;
}

WEAK_FUNC
const sdk_resources_t* _sdk_view_get_extern_resource(uint16_t view_id) {
    (void)view_id;
    return NULL;
}

#else /* CONFIG_SIMULATOR */
extern const sdk_resources_t* _sdk_view_get_resource(uint16_t view_id);

#ifdef CONFIG_UI_EXTERN_RESOURCE
extern const sdk_resources_t* _sdk_view_get_extern_resource(uint16_t view_id);
#else /* !CONFIG_UI_EXTERN_RESOURCE */
static const sdk_resources_t* _sdk_view_get_extern_resource(uint16_t view_id) {
    return NULL;
}
#endif /* CONFIG_UI_EXTERN_RESOURCE */
#endif /* !CONFIG_SIMULATOR */


static const sdk_resource_file_t const resoruce_file_default = {
    .sty_file = DEF_STY_FILE,
    .pic_file = DEF_RES_FILE,
    .txt_file = DEF_STR_FILE
};

static const sdk_resource_ns_method_t ns_vector[] = {
    /*
     * The shared resource just only at main resource file
     */
    {
        .name = "general",
        .ns = {
            .get_ns_resource = {
                [RES_NS_LOCAL_PRIVATE] = _sdk_view_get_resource,
                [RES_NS_LOCAL_SHARED]  = _sdk_view_get_resource,
                [RES_NS_EXTERN_SHARED] = _sdk_view_get_extern_resource
            },
            .views = {
                .local_shared_view  = UI_SHARED_VIEW, //TODO:
                .remote_shared_view = 0
            },
            .files = {
                .local_file  = &resoruce_file_default,
                .remote_file = &resoruce_file_default
            }
        },
    }
};

UI_PUBLIC_API const
sdk_resource_ns_t* _sdk_get_resource_ns_class(const char* name) {
    assert(name != NULL);
    for (size_t i = 0; i < UI_ARRAY_SIZE(ns_vector); i++) {
        if (!strcmp(name, ns_vector[i].name))
            return &ns_vector[i].ns;
    }
    assert(0);
    return NULL;
}

UI_PUBLIC_API int
_sdk_font_open(lv_font_t* font, const char* font_path, uint32_t id) {
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
    return lvgl_freetype_font_open(font, font_path, id);

#else /* !CONFIG_LVGL_USE_FREETYPE_FONT */
    char filename[128];

    snprintf(filename, sizeof(filename), font_path, id);
    return lvgl_bitmap_font_open(font, filename);
#endif /* CONFIG_LVGL_USE_FREETYPE_FONT */
}

UI_PUBLIC_API int
_sdk_font_close(lv_font_t* font) {
#ifdef CONFIG_LVGL_USE_FREETYPE_FONT
    lvgl_freetype_font_close(font);
#else
    lvgl_bitmap_font_close(font);
#endif /* CONFIG_LVGL_USE_FREETYPE_FONT */
    return 0;
}
