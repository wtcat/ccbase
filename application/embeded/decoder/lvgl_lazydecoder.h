/*
 * Copyright 2024 wtcat
 *
 * LVGL 9 image decoder for LZ4-compressed resources.
 *
 * The decoder does not decompress or cache by itself: it forwards work to
 * lazy_cache_decomp() (see lvgl_lazydecomp.h), which owns the LRU cache and
 * calls back the per-asset decompress routine. This header wires an LZ4
 * routine into that mechanism.
 *
 * Note: Not thread-safe (shares the single static cache_controller).
 */
#ifndef BASEWORK_UI_LVGL_LAZYDECODER_H_
#define BASEWORK_UI_LVGL_LAZYDECODER_H_

#include <lvgl.h>

#include "embeded/decoder/lvgl_lazydecomp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Resource layout at (base + lazy_decomp.offset):
 *
 *     +----------------+----------------------------+
 *     | u32 csize (LE) |  LZ4 block, csize bytes     |
 *     +----------------+----------------------------+
 *
 * The block decompresses to lv_image_dsc_t.data_size bytes of raw pixels in
 * the color format declared by the image header.
 */

/*
 * lz4_lazy_decompress - struct lazy_decomp::decompress implementation.
 *
 * Reads the size-prefixed LZ4 block at (base + ld->offset) and inflates it
 * into dst. Returns 0 on success, -1 otherwise (matches module convention).
 */
int lz4_lazy_decompress(const struct lazy_decomp *ld, void *dst, size_t dstsize);

/*
 * lz4_lazydecoder_set_base - Set the base address of the LZ4 resource region.
 *
 * @base pointer that lazy_decomp::offset is relative to (e.g. mapped flash).
 */
void lz4_lazydecoder_set_base(const void *base);

/*
 * lz4_lazydecoder_init - Create and register the LVGL 9 image decoder.
 *
 * Call once after lv_init(). Requires lazy_cache_init() and
 * lz4_lazydecoder_set_base() to have been called before decoding.
 * return 0 on success, -1 if the decoder could not be created.
 */
int lz4_lazydecoder_init(void);


#ifdef __cplusplus
}
#endif
#endif /* BASEWORK_UI_LVGL_LAZYDECODER_H_ */
