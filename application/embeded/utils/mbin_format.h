/*
 * Copyright 2026 wtcat
 */

#ifndef MBIN_FORMAT_H
#define MBIN_FORMAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bin_index {
	uint32_t namekey; /* hash for file name*/
	uint32_t offset;  /* file offset */
	uint32_t size; /* file size */
};

struct mbin_header {
#define MERGE_BIN_MAGIC "BINS"
	uint8_t   magic[4];
	uint32_t  count; /* binary file count */
	uint32_t  crc;
	struct bin_index indexs[];
};

static inline
#if defined(__GNUC__) || defined(__clang__)
__attribute__((const))
#endif
uint32_t mbin_name_hash(const uint8_t* key, uint32_t len) {
    uint32_t hash = 0;
    for (const uint8_t* end = key + len;
        key < end; key++) {
        hash *= 16777619;
        hash ^= (uint32_t)(*key);
    }
    return hash;
}

static inline
uint32_t mbin_crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    /* crc table generated from polynomial 0xedb88320 */
    const uint32_t table[16] = {
        0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
        0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
        0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
    };

    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
        crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
    }
    return ~crc;
}

#ifdef __cplusplus
}
#endif
#endif /* MBIN_FORMAT_H */
