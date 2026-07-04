// bin_writer.h
//
// Pure serializer: turns a set of (id, UTF-8 string) entries into the on-disk
// ResFile byte layout (header + index[] + string blob). No Excel dependency.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "refile_format.h"

namespace strres {

struct Entry {
    uint32_t id;        // namekey
    std::string text;   // UTF-8, WITHOUT trailing '\0' (added on write)
};

struct BinResult {
    std::vector<uint8_t> bytes;  // complete file image
    uint32_t crc = 0;            // crc32 over bytes [20 .. EOF)
    uint32_t count = 0;          // number of index entries
};

// Serialize entries. Input need not be sorted; entries are sorted ascending by
// id so the on-disk index satisfies the reader's binary-search invariant.
// Throws std::length_error if the file would exceed 4 GiB (u32 offset overflow).
BinResult build_bin(std::vector<Entry> entries,
                    uint32_t version = refile::kDefaultVersion);

// Write a byte image to disk. Throws std::runtime_error on IO failure.
void write_file(const std::string& path, const std::vector<uint8_t>& bytes);

}  // namespace strres
