// table.h
//
// In-memory representation of a parsed worksheet, independent of the source
// format (.xls / .xlsx). All strings are UTF-8; an empty string denotes a
// missing / blank cell.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace strres {

struct Table {
    // Column headers from the header row (UTF-8). Size defines the column count.
    std::vector<std::string> headers;
    // Data rows in sheet order; each row is padded to headers.size() columns.
    // cells[row][col] == "" means the cell was blank.
    std::vector<std::vector<std::string>> cells;

    std::size_t columns() const { return headers.size(); }
    std::size_t rows() const { return cells.size(); }
};

}  // namespace strres
