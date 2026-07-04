/*
 * Copyright 2024 wtcat
 *
 * LVGL 9 image decoder for LZ4-compressed resources.
 * Note: Not thread-safe (shares the single static cache_controller).
 */

#ifdef CONFIG_HEADER_FILE
#include CONFIG_HEADER_FILE
#endif

#include <lvgl.h>
#include <src/misc/lv_color.h>
#include <src/draw/lv_image_decoder_private.h>

#include "embeded/decoder/lvgl_lazydecomp.h"
#include "embeded/decoder/lvgl_lazydecoder.h"


static lv_result_t lvgl_lazy_info_cb(lv_image_decoder_t *decoder,
                                    lv_image_decoder_dsc_t *dsc,
                                    lv_image_header_t *header) {
    const lv_image_dsc_t *img = dsc->src;

    LV_UNUSED(decoder);

    if (dsc->src_type != LV_IMAGE_SRC_VARIABLE)
        return LV_RESULT_INVALID;

    /* Not one of ours: let another decoder handle it. */
    if (img->header.reserved_2 != LV_IMG_LAZYDECOMP_MARKER)
        return LV_RESULT_INVALID;

    *header = img->header;
    return LV_RESULT_OK;
}

static lv_result_t lvgl_lazy_open_cb(lv_image_decoder_t *decoder,
                                    lv_image_decoder_dsc_t *dsc) {
    const lv_image_dsc_t *src = dsc->src;
    lv_image_dsc_t decoded;
    lv_draw_buf_t *buf;
    uint32_t stride;

    LV_UNUSED(decoder);

    if (dsc->src_type != LV_IMAGE_SRC_VARIABLE ||
        src->header.reserved_2 != LV_IMG_LAZYDECOMP_MARKER)
        return LV_RESULT_INVALID;

    /* Decompress (and cache) via the lazy cache controller. */
    if (lazy_cache_decomp(src, &decoded, NULL, 0) != 0)
        return LV_RESULT_INVALID;

    stride = src->header.stride? src->header.stride
             : lv_draw_buf_width_to_stride(src->header.w, src->header.cf);

    /*
     * Wrap the (volatile) cache buffer as a draw buffer. Only the wrapper is
     * allocated here; the pixel data is owned by the lazy cache pool.
     */
    buf = lv_malloc(sizeof(*buf));
    if (buf == NULL)
        return LV_RESULT_INVALID;

    lv_color_format_t cf = src->header.cf;
    if (LV_COLOR_FORMAT_IS_INDEXED(cf)) {
        dsc->palette = (lv_color32_t*)decoded.data;
        dsc->palette_size = LV_COLOR_INDEXED_PALETTE_SIZE(cf);
        cf = LV_COLOR_FORMAT_ARGB8888;
    }

    lv_draw_buf_init(buf, src->header.w, src->header.h, cf,
                     stride, (void *)decoded.data, src->data_size);

    dsc->decoded = buf;

    /*
     * The cache buffer may be evicted/overwritten on the next lazy decode, so
     * it must not enter LVGL's own image cache.
     */
    dsc->args.no_cache = true;

    return LV_RESULT_OK;
}

static void lvgl_lazy_close_cb(lv_image_decoder_t *decoder,
                              lv_image_decoder_dsc_t *dsc) {
    LV_UNUSED(decoder);

    /* Free only the wrapper; decoded->data belongs to the lazy cache pool. */
    if (dsc->decoded != NULL) {
        lv_free((void *)dsc->decoded);
        dsc->decoded = NULL;
    }
}

int lvgl_lazydecoder_init(void) {
    lv_image_decoder_t *dec = lv_image_decoder_create();
    if (dec == NULL)
        return -1;

    lv_image_decoder_set_info_cb(dec, lvgl_lazy_info_cb);
    lv_image_decoder_set_open_cb(dec, lvgl_lazy_open_cb);
    lv_image_decoder_set_close_cb(dec, lvgl_lazy_close_cb);

    return 0;
}
