
/*
 * Copyright 2026 wtcat (Assisted by AI) 
 */
#include <algorithm>

#include "quantizer.h"
#include "thirdparty/libimagequant/libimagequant.h"

namespace i8 {

QuantizeResult Quantize(const Image& img, const QuantizeOptions& opts) {
    QuantizeResult result;
    result.indices.assign(static_cast<size_t>(img.width) * img.height, 0);

    // libimagequant references the bitmap without copying; `img` outlives every
    // call below, so the pointer stays valid through write_remapped.
    liq_attr* attr = liq_attr_create();
    if (!attr) return result;
    liq_set_max_colors(attr, std::clamp(opts.max_colors, 1, 256));
    liq_set_speed(attr, std::clamp(opts.speed, 1, 10));

    liq_image* image = liq_image_create_rgba(attr, img.pixels, img.width,
                                             img.height, /*gamma=*/0.0);
    if (!image) {
        liq_attr_destroy(attr);
        return result;
    }

    liq_result* res = nullptr;
    const liq_error err = liq_image_quantize(image, attr, &res);
    if (err != LIQ_OK || !res) {
        liq_image_destroy(image);
        liq_attr_destroy(attr);
        return result;  // palette_size == 0 signals failure to the caller
    }

    liq_set_dithering_level(res, opts.dither);
    liq_write_remapped_image(res, image, result.indices.data(), result.indices.size());

    const liq_palette* pal = liq_get_palette(res);
    const unsigned count = std::min<unsigned>(pal->count, 256);
    result.palette_size = static_cast<int>(count);
    for (unsigned i = 0; i < count; ++i) {
        const liq_color& c = pal->entries[i];
        result.palette[i] = Rgba{c.r, c.g, c.b, c.a};
    }

    liq_result_destroy(res);
    liq_image_destroy(image);
    liq_attr_destroy(attr);
    return result;
}

}  // namespace i8
