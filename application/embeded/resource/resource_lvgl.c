/*
 * Copyright 2026 wtcat 
 */

#if 0
#include <lvgl/lvgl.h>

#include "resource_file.h"
#include "resource_loader.h"


int re_make_lvimg(const re_file_t* refile, uint32_t id, size_t size, lv_image_dsc_t *dsc) {
    re_desc_t desc;
    int err;

    err = re_read_image_dsc(refile, id, &desc);
    if (err)
        return err;

    re_read_desc(&desc, pimg, sizeof(image_buffer_2[0]));

    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = LV_COLOR_FORMAT_I8;
    dsc->header.w = pimg->width;
    dsc->header.h = pimg->height;
    dsc->data_size = pimg->size;
    dsc->data = (uint8_t*)pimg->data;
}
#endif