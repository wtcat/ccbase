// Copyright 2026 wtcat

#pragma once

#include <string>

class RGBConvertor {
public:
    RGBConvertor(const std::string& s) : name_(s) {}
    const std::string& name() const {
        return name_;
    }
    size_t Convert(const uint8_t* src, int w, int h, int channels, int format, uint8_t* outpx);
    bool Compress(const uint8_t* src, size_t size, uint8_t* dst, size_t* dst_size, int comp);
    size_t GetFormatSize(int* format, int channel);
 
protected:
    inline uint16_t ToRGB565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

private:
    std::string name_;
};