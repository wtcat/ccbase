// excel_reader.h
//
// Reads a worksheet from .xls or .xlsx into a Table (UTF-8). Format is chosen
// by file extension.
#pragma once

#include <cstddef>
#include <string>

#include "table.h"

namespace strres {

struct ReadOptions {
    std::size_t sheet = 0;       // 0-based worksheet index
    std::size_t header_row = 0;  // 0-based row used as column headers
};

// Throws std::runtime_error on unsupported extension, parse failure, or if the
// sheet has no header row. Data rows are padded to the header column count.
Table read_excel(const std::string& path, const ReadOptions& opt = {});

}  // namespace strres
