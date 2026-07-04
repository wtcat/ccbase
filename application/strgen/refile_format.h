// refile_format.h
//
// GIVEN binary-format definitions and CRC32 helper. These come from the
// existing reader and MUST NOT be modified. Only additive helpers
// (constants / static_asserts) live at the bottom of this file.
#pragma once

#include "embeded/resource/resource_file.h"
#include <cstddef>
#include <cstdint>

#if 0
struct refile_index {
    uint32_t namekey; /* String ID */
    uint32_t offset;
    uint32_t size;
};

struct refile_header {
#define REFILE_FILE_MAGIC "ResFile"
    uint8_t   magic[8];
    uint32_t  version;
    uint32_t  count;
    uint32_t  chksum; /* crc32 */
    struct refile_index indexs[];
};
#endif

static uint32_t re_crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    /* crc table generated from polynomial 0xedb88320 */
    static const uint32_t table[16] = {
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

/* GIVEN name-hash helper (FNV-style, seed 0). Used verbatim so packed
 * refile_index::namekey values match the reader's lookup keys byte-for-byte.
 * `static` mirrors re_crc32_update above so this header can be included by
 * multiple translation units without a multiple-definition link error. */
static uint32_t re_name_hash(const uint8_t* key, uint32_t len) {
    uint32_t hash = 0;
    for (const uint8_t* end = key + len; key < end; key++) {
        hash *= 16777619;
        hash ^= (uint32_t)(*key);
    }
    return hash;
}

// ---------------------------------------------------------------------------
// Additive helpers (safe to touch; do not alter the definitions above).
// ---------------------------------------------------------------------------

// Byte offsets / sizes of the on-disk header, used by the explicit
// little-endian serializer in bin_writer. The header is laid out as:
//   magic[8] | version(u32) | count(u32) | chksum(u32) | indexs[]
namespace refile {

constexpr std::size_t kHeaderSize = 20;          // bytes before indexs[]
constexpr std::size_t kIndexSize = 12;           // sizeof one refile_index
constexpr std::size_t kMagicSize = 8;            // magic[8]
constexpr std::size_t kChksumOffset = 16;        // byte offset of chksum field
constexpr std::size_t kCrcStart = kHeaderSize;   // crc covers [20 .. EOF)
constexpr uint32_t kDefaultVersion = 1;

static_assert(sizeof(refile_index) == kIndexSize, "refile_index must be 12 bytes");
static_assert(sizeof(refile_header) == kHeaderSize, "refile_header (no flex data) must be 20 bytes");
static_assert(offsetof(refile_header, chksum) == kChksumOffset, "chksum must be at offset 16");

}  // namespace refile
