// Reduce an RGBA8 image to a <=256 color palette and per-pixel indices,
// using libimagequant (pngquant's perceptual quantizer) for best quality.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "types.h"

namespace i8 {

constexpr int kPaletteEntries = 256;      // INDEX8 always carries a full palette
constexpr int kPaletteBytes = kPaletteEntries * 4;

struct QuantizeOptions {
    int max_colors = 256;  // hard cap for INDEX8
    float dither = 1.0f;   // libimagequant dithering level, 0 = none, 1 = full
    int speed = 4;         // libimagequant speed 1 (best quality) .. 10 (fastest)
};

struct QuantizeResult {
    std::array<Rgba, 256> palette{};  // entries [0, palette_size) are valid; rest zeroed
    int palette_size = 0;
    std::vector<uint8_t> indices;     // size == width * height, row-major
};

// Quantizes `img`. Returns a result whose indices reference `palette`.
// On quantization failure returns a result with palette_size == 0.
QuantizeResult Quantize(const Image& img, const QuantizeOptions& opts);

}  // namespace i8
