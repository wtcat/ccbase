// reader_internal.h
//
// Internal seam between the format-specific parsers and the Table assembler.
// A Grid is a ragged 2-D array of UTF-8 cell strings ("" = blank).
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace strres {

using Grid = std::vector<std::vector<std::string>>;

// Parse the given 0-based worksheet into a ragged Grid. Both throw
// std::runtime_error on failure.
Grid read_xls_grid(const std::string& path, std::size_t sheet);
Grid read_xlsx_grid(const std::string& path, std::size_t sheet);

}  // namespace strres
