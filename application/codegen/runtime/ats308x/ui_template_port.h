/*
 * Copyright 2025 wtcat
 */
#ifndef UI_TEMPLATE_PORT_H_
#define UI_TEMPLATE_PORT_H_

#include <app_ui.h>
#include <view_manager.h>

/* Just only for debug */
#define UI_SHARED_VIEW 0xffff 

#ifndef SHA_PICTURE_MAX
#define SHA_PICTURE_MAX 4
#endif
#ifndef SHA_STRING_MAX
#define SHA_STRING_MAX  4
#endif

#define SDK_BITMAP_FONT_FORMATTER(_name) \
    DEF_FONT_DISK #_name "%d.font"

typedef struct {
#define SDK_RESOURCE_DEFINE(_name, ...) \
    static const sdk_resource_map_t _name[] = { __VA_ARGS__}
#define _RESOURCE_ITEM(_id) {#_id, _id}

    const char* name;
    uint32_t    resource;
} sdk_resource_map_t;

typedef struct {
    uint32_t scene_id;
    uint16_t view_id;
    uint16_t picture_num;
    uint16_t pictureset_num;
    uint16_t text_num;
} sdk_resources_t;

typedef lvgl_res_picregion_t ui_picture_set_t;
typedef const sdk_resources_t* (*sdk_resource_get_fn_t)(uint16_t);

typedef struct {
    const char* sty_file;
    const char* pic_file;
    const char* txt_file;
} sdk_resource_file_t;

typedef struct {
    const sdk_resources_t* (*get_ns_resource[RES_NS_MAX])(uint16_t view);
    union {
        struct {
            uint16_t local_shared_view;
            uint16_t remote_shared_view;
        } views;
        uint16_t     view_array[2];
    };
    union {
        struct {
            const sdk_resource_file_t *local_file;
            const sdk_resource_file_t *remote_file;
        } files;
        const sdk_resource_file_t *file_array[2];
    };
} sdk_resource_ns_t;

UI_PUBLIC_API int 
_sdk_view_handler(uint16_t view_id, uint8_t msg_id, void* msg_data);

UI_PUBLIC_API const
sdk_resource_ns_t* _sdk_get_resource_ns_class(const char* name);

UI_PUBLIC_API int
_sdk_font_open(lv_font_t* font, const char* font_path, uint32_t id);

UI_PUBLIC_API int
_sdk_font_close(lv_font_t* font);

#ifndef CONFIG_SIMULATOR
#define LVGL_VIEW_DEFINE(_id, _view) \
    VIEW_DEFINE_EXT(_name, _sdk_view_handler, NULL, NULL, _id, \
    NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT, (void*)_view)

#else /* CONFIG_SIMULATOR */

#define LVGL_VIEW_DEFINE(...)
#endif /* !CONFIG_SIMULATOR */

static inline unsigned long 
#if defined(__GNUC__) || defined(__clang__)
__attribute__((const))
#endif
__re_hash(const unsigned char* key, unsigned int len) {
    unsigned long hash = 0;
    for (const unsigned char* end = key + len; 
        key < end; key++) {
        hash *= 16777619;
        hash ^= (unsigned long)(*key);
    }
    return hash;
}

#define __RE(x) __re_hash((x), sizeof((x))-1)

#define VIEW_PRIV(_name) .private = _name

#endif /* UI_TEMPLATE_PORT_H_ */
