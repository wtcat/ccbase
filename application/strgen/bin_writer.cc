#include "bin_writer.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace strres {
namespace {

// Explicit little-endian store — independent of host byte order.
inline void put_u32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}

}  // namespace

BinResult build_bin(std::vector<Entry> entries, uint32_t version) {
    // Ascending by id; stable so equal ids keep input order (packer forbids dups).
    std::stable_sort(entries.begin(), entries.end(),
                     [](const Entry& a, const Entry& b) { return a.id < b.id; });

    const uint64_t count = entries.size();
    const uint64_t index_bytes = count * refile::kIndexSize;
    const uint64_t strings_start = refile::kHeaderSize + index_bytes;

    uint64_t blob_bytes = 0;
    for (const Entry& e : entries) blob_bytes += e.text.size() + 1;  // +1 for '\0'

    const uint64_t total = strings_start + blob_bytes;
    if (total > std::numeric_limits<uint32_t>::max()) {
        throw std::length_error("bin file exceeds 4 GiB (uint32 offset overflow)");
    }

    BinResult out;
    out.count = static_cast<uint32_t>(count);
    out.bytes.resize(static_cast<size_t>(total));
    uint8_t* buf = out.bytes.data();

    // Header
    std::memcpy(buf, REFILE_FILE_MAGIC, refile::kMagicSize);  // "ResFile\0" (8 bytes)
    put_u32(buf + 8, version);
    put_u32(buf + 12, out.count);
    // chksum (buf + 16) filled after crc is known.

    // Index array + string blob, written in one ordered pass.
    uint8_t* idx = buf + refile::kHeaderSize;
    uint64_t str_off = strings_start;
    for (const Entry& e : entries) {
        const uint32_t size = static_cast<uint32_t>(e.text.size() + 1);
        put_u32(idx + 0, e.id);
        put_u32(idx + 4, static_cast<uint32_t>(str_off));
        put_u32(idx + 8, size);
        idx += refile::kIndexSize;

        std::memcpy(buf + str_off, e.text.data(), e.text.size());
        buf[str_off + e.text.size()] = '\0';
        str_off += size;
    }

    // crc32 over everything after the chksum field: [kCrcStart .. EOF)
    out.crc = re_crc32_update(0, buf + refile::kCrcStart,
                              static_cast<size_t>(total - refile::kCrcStart));
    put_u32(buf + refile::kChksumOffset, out.crc);

    return out;
}

void write_file(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream os(path, std::ios::binary | std::ios::trunc);
    if (!os) throw std::runtime_error("cannot open output file: " + path);
    os.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
    if (!os) throw std::runtime_error("write failed: " + path);
}

}  // namespace strres
