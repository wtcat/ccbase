#ifndef MERGEBIN_MERGER_HPP
#define MERGEBIN_MERGER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace binfile {

/* Thrown for any format / IO error surfaced by the library. */
struct MergeError : std::runtime_error {
    explicit MergeError(const std::string& msg) : std::runtime_error(msg) {}
};

/* One index entry as read back from a container. */
struct Entry {
    uint32_t namekey;
    uint32_t offset;
    uint32_t size;
};

/* Result of parsing a container blob. */
struct ContainerInfo {
    uint32_t           count;
    uint32_t           stored_crc;
    uint32_t           computed_crc;
    std::vector<Entry> entries;

    bool crc_ok() const { return stored_crc == computed_crc; }
};

/* Merge the given files, in order, into a single container blob.
 * `namekey` is name_hash(basename(path)); duplicate namekeys throw MergeError. */
std::vector<uint8_t> merge(const std::vector<std::string>& paths);

/* Parse and validate a container blob (magic, sizes) and recompute its CRC. */
ContainerInfo parse(const std::vector<uint8_t>& buf);

}  // namespace binfile

#endif /* MERGEBIN_MERGER_HPP */
