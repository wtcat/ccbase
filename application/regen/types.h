// Basic value types shared across the converter.
#pragma once

#include <cstdint>
#include <vector>

namespace i8 {

// One pixel, 8 bits per channel.
struct Rgba {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

// Decoded image: tightly packed RGBA8, row-major, no padding.
struct Image {
    int width = 0;
    int height = 0;
    uint8_t *pixels;  // size == width * height * 4

    const Rgba* row(int y) const {
        return reinterpret_cast<const Rgba*>(pixels + static_cast<size_t>(y) * width * 4);
    }
};

}  // namespace i8
