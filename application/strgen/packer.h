// packer.h
//
// Turns a parsed Table into per-language entry lists ready for bin_writer.
// Owns the domain rules: which column is the ID, how the ID is parsed, how
// blank cells are handled, and duplicate-ID detection.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bin_writer.h"
#include "table.h"

namespace strres {

enum class EmptyPolicy { Skip, Emit, Error };

struct PackOptions {
    std::size_t id_col = 0;               // column holding the ResID
    EmptyPolicy empty = EmptyPolicy::Skip;
    std::string name_mode = "code";       // "code" -> after '/', "slug" -> full header
};

struct LanguagePack {
    std::size_t column;      // source column index
    std::string header;      // raw header text
    std::string filename;    // e.g. "en.bin"
    std::vector<Entry> entries;
};

// One ResID and its namekey, for emitting the string_ids.h header.
struct StringId {
    std::string name;   // raw ResID cell text
    uint32_t hash;      // re_name_hash(name)
};

// Derive a .bin filename from a language header per name_mode. "code" uses the
// text after the last '/', lowercased and slugified; "slug" uses the whole
// header slugified. Disambiguation of colliding names is handled by pack_all.
std::string filename_for(const std::string& header, const std::string& name_mode);

// Build one LanguagePack per non-ID column. The namekey is re_name_hash() of the
// raw ResID cell text; rows with a blank ID cell are skipped. Throws
// std::runtime_error on a duplicate namekey within a language.
std::vector<LanguagePack> pack_all(const Table& table, const PackOptions& opts);

// Scan the ID column in row order, skipping blank/whitespace-only cells, and
// hash the raw cell text (same rule as pack_all). Returns ids in source order.
// Duplicate detection is pack_all's responsibility (main runs it first), so
// this does not re-check.
std::vector<StringId> collect_string_ids(const Table& table, std::size_t id_col);

}  // namespace strres
