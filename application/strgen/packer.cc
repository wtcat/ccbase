#include "packer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace strres {

namespace {

std::string slugify(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            out += static_cast<char>(std::tolower(c));
        } else if (!out.empty() && out.back() != '_') {
            out += '_';
        }
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out;
}

}  // namespace

std::string filename_for(const std::string& header, const std::string& name_mode) {
    std::string base;
    if (name_mode == "code") {
        auto pos = header.find_last_of('/');
        base = (pos == std::string::npos) ? header : header.substr(pos + 1);
        base = slugify(base);
    }
    if (base.empty()) base = slugify(header);  // slug mode or empty code
    if (base.empty()) base = "lang";
    return base + ".bin";
}

std::vector<LanguagePack> pack_all(const Table& table, const PackOptions& opts) {
    if (opts.id_col >= table.columns())
        throw std::runtime_error("id column index out of range");

    // One pack per non-ID column.
    std::vector<LanguagePack> packs;
    for (std::size_t c = 0; c < table.columns(); ++c) {
        if (c == opts.id_col) continue;
        packs.push_back({c, table.headers[c],
                         filename_for(table.headers[c], opts.name_mode), {}});
    }

    // Resolve filename collisions (e.g. Simplified/Traditional both "zh") by
    // falling back to the full-header slug for every colliding column.
    std::unordered_map<std::string, int> counts;
    for (const auto& p : packs) counts[p.filename]++;
    for (auto& p : packs) {
        if (counts[p.filename] > 1) p.filename = slugify(p.header) + ".bin";
    }

    std::unordered_set<uint32_t> seen_ids;
    seen_ids.reserve(table.rows() * 2);

    for (std::size_t r = 0; r < table.rows(); ++r) {
        const auto& row = table.cells[r];
        const std::string& id_cell = row[opts.id_col];

        // A blank ID cell marks a non-data row (secondary sub-header, spacer, or
        // trailing filler) -> skip. Otherwise the namekey is the hash of the raw
        // cell text, matching the reader's lookup key.
        if (id_cell.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        const uint32_t id = re_name_hash(
            reinterpret_cast<const uint8_t*>(id_cell.data()),
            static_cast<uint32_t>(id_cell.size()));
        if (!seen_ids.insert(id).second) {
            char hex[11];
            std::snprintf(hex, sizeof(hex), "0x%08x", id);
            throw std::runtime_error("duplicate namekey " + std::string(hex) +
                                     " for ResID \"" + id_cell + "\" at data row " +
                                     std::to_string(r));
        }

        for (auto& p : packs) {
            const std::string& text = row[p.column];
            if (text.empty()) {
                switch (opts.empty) {
                    case EmptyPolicy::Skip: continue;
                    case EmptyPolicy::Emit: p.entries.push_back({id, ""}); continue;
                    case EmptyPolicy::Error:
                        throw std::runtime_error("empty cell for ID " +
                                                 std::to_string(id) + " in column \"" +
                                                 p.header + "\"");
                }
            }
            p.entries.push_back({id, text});
        }
    }
    return packs;
}

std::vector<StringId> collect_string_ids(const Table& table, std::size_t id_col) {
    if (id_col >= table.columns())
        throw std::runtime_error("id column index out of range");

    std::vector<StringId> ids;
    for (std::size_t r = 0; r < table.rows(); ++r) {
        const std::string& id_cell = table.cells[r][id_col];
        if (id_cell.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        const uint32_t hash = re_name_hash(
            reinterpret_cast<const uint8_t*>(id_cell.data()),
            static_cast<uint32_t>(id_cell.size()));
        ids.push_back({id_cell, hash});
    }
    return ids;
}

}  // namespace strres
