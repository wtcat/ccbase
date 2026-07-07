#ifndef MERGEBIN_LE_HPP
#define MERGEBIN_LE_HPP

#include <cstddef>
#include <cstdint>

/* Little-endian (de)serialization helpers. All uint32 container fields are
 * stored little-endian so that produced blobs are deterministic and
 * host-endianness independent. */

inline void put_u32_le(uint8_t* dst, uint32_t value) {
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

inline uint32_t get_u32_le(const uint8_t* src) {
    return (uint32_t)src[0]
         | ((uint32_t)src[1] << 8)
         | ((uint32_t)src[2] << 16)
         | ((uint32_t)src[3] << 24);
}

#endif /* MERGEBIN_LE_HPP */
