#include "merger.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <unordered_map>

#include "binfile.h"
#include "le.h"


namespace binfile {

namespace {

uint32_t name_hash(const uint8_t* key, uint32_t len) {
    uint32_t hash = 0;
    for (const uint8_t* end = key + len;
        key < end; key++) {
        hash *= 16777619;
        hash ^= (uint32_t)(*key);
    }
    return hash;
}

uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
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


/* Basename: everything after the last '/' (POSIX-style). */
std::string basename_of(const std::string& path) {
    auto pos = path.find_last_of('/');
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw MergeError("cannot open input file: " + path);
    }
    std::streamsize len = in.tellg();
    if (len < 0) {
        throw MergeError("cannot determine size of: " + path);
    }
    std::vector<uint8_t> data(static_cast<size_t>(len));
    in.seekg(0);
    if (len > 0 && !in.read(reinterpret_cast<char*>(data.data()), len)) {
        throw MergeError("cannot read input file: " + path);
    }
    return data;
}

uint32_t hash_name(const std::string& name) {
    return name_hash(reinterpret_cast<const uint8_t*>(name.data()),
                     static_cast<uint32_t>(name.size()));
}

}  // namespace

std::vector<uint8_t> merge(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        throw MergeError("no input files given");
    }

    const uint32_t count = static_cast<uint32_t>(paths.size());
    const uint32_t hdr_size = header_size(count);

    std::vector<std::vector<uint8_t>> blobs;
    std::vector<Entry> entries;
    blobs.reserve(count);
    entries.reserve(count);

    std::unordered_map<uint32_t, std::string> seen_keys;  /* namekey -> first path */
    uint32_t offset = hdr_size;

    for (const auto& path : paths) {
        std::vector<uint8_t> data = read_file(path);
        const std::string name = basename_of(path);
        const uint32_t key = hash_name(name);

        auto ins = seen_keys.emplace(key, path);
        if (!ins.second) {
            char hex[11];
            std::snprintf(hex, sizeof(hex), "0x%08x", key);
            throw MergeError("filename hash collision: '" + ins.first->second +
                             "' and '" + path + "' both map to namekey " + hex);
        }

        entries.push_back({key, offset, static_cast<uint32_t>(data.size())});
        offset += static_cast<uint32_t>(data.size());
        blobs.push_back(std::move(data));
    }

    /* Assemble the buffer: header + index table + data. */
    std::vector<uint8_t> buf(offset, 0);
    std::memcpy(buf.data(), MAGIC, 4);
    put_u32_le(buf.data() + 4, count);
    /* crc field at offset 8 stays 0 for now. */

    uint8_t* idx = buf.data() + HEADER_SIZE;
    for (const auto& e : entries) {
        put_u32_le(idx + 0, e.namekey);
        put_u32_le(idx + 4, e.offset);
        put_u32_le(idx + 8, e.size);
        idx += INDEX_SIZE;
    }

    for (size_t i = 0; i < blobs.size(); i++) {
        if (!blobs[i].empty()) {
            std::memcpy(buf.data() + entries[i].offset, blobs[i].data(),
                        blobs[i].size());
        }
    }

    const uint32_t crc =
        crc32_update(0, buf.data() + CRC_START, buf.size() - CRC_START);
    put_u32_le(buf.data() + 8, crc);

    return buf;
}

ContainerInfo parse(const std::vector<uint8_t>& buf) {
    if (buf.size() < HEADER_SIZE) {
        throw MergeError("truncated container: smaller than header");
    }
    if (std::memcmp(buf.data(), MAGIC, 4) != 0) {
        throw MergeError("bad magic: not a BINS container");
    }

    ContainerInfo info;
    info.count = get_u32_le(buf.data() + 4);
    info.stored_crc = get_u32_le(buf.data() + 8);

    const uint64_t hdr_size =
        static_cast<uint64_t>(HEADER_SIZE) + static_cast<uint64_t>(info.count) * INDEX_SIZE;
    if (buf.size() < hdr_size) {
        throw MergeError("truncated container: index table exceeds file size");
    }

    info.entries.reserve(info.count);
    const uint8_t* idx = buf.data() + HEADER_SIZE;
    for (uint32_t i = 0; i < info.count; i++) {
        Entry e;
        e.namekey = get_u32_le(idx + 0);
        e.offset = get_u32_le(idx + 4);
        e.size = get_u32_le(idx + 8);
        idx += INDEX_SIZE;

        const uint64_t end = static_cast<uint64_t>(e.offset) + e.size;
        if (e.offset < hdr_size || end > buf.size()) {
            throw MergeError("index entry points outside the data section");
        }
        info.entries.push_back(e);
    }

    info.computed_crc =
        crc32_update(0, buf.data() + CRC_START, buf.size() - CRC_START);

    return info;
}

}  // namespace binfile
