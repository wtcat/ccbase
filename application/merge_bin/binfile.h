#ifndef MERGEBIN_BINFILE_HPP
#define MERGEBIN_BINFILE_HPP

#include <cstdint>

/* On-disk container format:
 *
 *   <binfile_header> + <binfile_index...> + <data>
 *
 * All uint32 fields are stored little-endian. The CRC covers everything from
 * the first index entry (offset 12) to the end of the data section; the magic,
 * count and crc fields themselves are NOT covered.
 *
 *   struct binfile_index { uint32_t namekey; uint32_t offset; uint32_t size; };
 *   struct binfile_header { uint8_t magic[4]; uint32_t count; uint32_t crc;
 *                           struct binfile_index indexs[]; };
 */

namespace binfile {

constexpr char        MAGIC[4]     = {'B', 'I', 'N', 'S'};
constexpr uint32_t    HEADER_SIZE  = 12;  /* magic[4] + count(4) + crc(4) */
constexpr uint32_t    INDEX_SIZE   = 12;  /* namekey(4) + offset(4) + size(4) */
constexpr uint32_t    CRC_START    = 12;  /* byte offset where the CRC region begins */

constexpr uint32_t header_size(uint32_t count) {
    return HEADER_SIZE + count * INDEX_SIZE;
}

}  // namespace binfile

#endif /* MERGEBIN_BINFILE_HPP */
