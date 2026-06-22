// Copyright 2026 wtcat

#include "embeded/resource/resource_file.h"
#include "quantizer.h"

#include "reconv.h"
#include "thirdparty/lz4/lib/lz4.h"

#define ALIGN_UP(x, a) (((x) + (a) - 1) / (a) * (a))

size_t RGBConvertor::Convert(const uint8_t* src, int w, int h, int channels, 
    int format, uint8_t* outpx) {
    size_t dst_size;

    switch (format) {
    case PIXEL_FORMAT_RGB888:
        for (int i = 0; i < w * h; i++) {
            outpx[i * 3 + 0] = src[i * channels + 0]; // R
            outpx[i * 3 + 1] = src[i * channels + 1]; // G
            outpx[i * 3 + 2] = src[i * channels + 2]; // B
        }
        dst_size = (size_t)w * h * 3;
        break;

    case PIXEL_FORMAT_ARGB888:
        for (int i = 0; i < w * h; i++) {
            outpx[i * 4 + 0] = src[i * channels + 0]; // R
            outpx[i * 4 + 1] = src[i * channels + 1]; // G
            outpx[i * 4 + 2] = src[i * channels + 2]; // B
            outpx[i * 4 + 3] = src[i * channels + 3]; // A
        }
        dst_size = (size_t)w * h * 4;
        break;

    case PIXEL_FORMAT_RGB565:
        for (int i = 0; i < w * h; i++) {
            uint8_t r = src[i * channels + 0];
            uint8_t g = src[i * channels + 1];
            uint8_t b = src[i * channels + 2];
            uint16_t rgb565 = ToRGB565(r, g, b);

            outpx[i * 2 + 0] = rgb565 & 0xFF;
            outpx[i * 2 + 1] = rgb565 >> 8;
        }
        dst_size = (size_t)w * h * 2;
        break;

    case PIXEL_FORMAT_ARGB565:
        for (int i = 0; i < w * h; i++) {
            uint8_t r = src[i * channels + 0];
            uint8_t g = src[i * channels + 1];
            uint8_t b = src[i * channels + 2];
            uint16_t rgb565 = ToRGB565(r, g, b);

            outpx[i * 3 + 0] = rgb565 & 0xFF;
            outpx[i * 3 + 1] = rgb565 >> 8;
            outpx[i * 3 + 2] = src[i * channels + 3];
        }
        dst_size = (size_t)w * h * 3;
        break;

    case PIXEL_FORMAT_INDEXED8: {
        i8::QuantizeOptions qopts;
        i8::Image image;
        image.width = w;
        image.height = h;
        image.pixels = (uint8_t*)src;
        int stride = ALIGN_UP(w, 1); // 1 bytes align
        const i8::QuantizeResult q = i8::Quantize(image, qopts);

        // palette: 256 entries, byte order B, G, R, A
        for (int i = 0; i < i8::kPaletteEntries; ++i) {
            const i8::Rgba c = (i < q.palette_size) ? q.palette[i] : i8::Rgba{};
            outpx[i * 4 + 0] = c.b;
            outpx[i * 4 + 1] = c.g;
            outpx[i * 4 + 2] = c.r;
            outpx[i * 4 + 3] = c.a;
        }

        // index data: one byte per pixel, rows padded to `stride`
        uint8_t* opx = outpx + i8::kPaletteEntries * 4;
        for (int y = 0; y < h; ++y) {
            const uint8_t* row = q.indices.data() + static_cast<size_t>(y) * w;
            memcpy(opx, row, w);
            opx += w;

            for (int pad = w; pad < stride; ++pad)
                *opx++ = 0;
        }

        dst_size = i8::kPaletteEntries * 4 + (size_t)stride * h * 1;
    }
         break;

    default:
        dst_size = 0;
        break;
    }
    return dst_size;
}
bool RGBConvertor::Compress(const uint8_t* src, size_t size, uint8_t* dst, size_t* dst_size, int comp) {
    if (comp == REFILE_COMPRESS_LZ4) {
        int max_size = LZ4_compressBound((int)size);
        if (*dst_size < max_size)
            return false;

        int ret = LZ4_compress_default((char*)src, (char*)dst, (int)size, (int)*dst_size);
        if (ret <= 0)
            return false;

        *dst_size = ret;
        return true;
    }
    return false;
}

size_t RGBConvertor::GetFormatSize(int* format, int channel) {
    bool alpha = channel == 4;
_repeat:
    switch (*format) {
    case PIXEL_FORMAT_RGB888:
        if (alpha) {
            *format = PIXEL_FORMAT_ARGB888;
            goto _repeat;
        }
        return 3;

    case PIXEL_FORMAT_ARGB888:
        if (!alpha) {
            *format = PIXEL_FORMAT_RGB888;
            goto _repeat;
        }
        return 4;

    case PIXEL_FORMAT_RGB565:
        if (alpha) {
            *format = PIXEL_FORMAT_ARGB565;
            goto _repeat;
        }
        return 2;

    case PIXEL_FORMAT_ARGB565:
        if (!alpha) {
            *format = PIXEL_FORMAT_RGB565;
            goto _repeat;
        }
        return 3;

    case PIXEL_FORMAT_INDEXED8:
        return 4;

    default:
        return 0;
    }
}
