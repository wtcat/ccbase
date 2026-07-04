#include "excel_reader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "reader_internal.h"

namespace strres {
namespace {

std::string lower_ext(const std::string& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

// Convert a ragged Grid into a Table: row `header_row` defines the columns;
// subsequent rows become data, padded/truncated to the header width.
Table to_table(const Grid& grid, std::size_t header_row) {
    if (header_row >= grid.size())
        throw std::runtime_error("header row " + std::to_string(header_row) +
                                 " out of range (sheet has " +
                                 std::to_string(grid.size()) + " rows)");

    Table t;
    t.headers = grid[header_row];
    // Trim trailing empty header columns (spreadsheets often over-report width).
    while (!t.headers.empty() && t.headers.back().empty()) t.headers.pop_back();
    if (t.headers.empty()) throw std::runtime_error("header row is empty");

    const std::size_t ncol = t.headers.size();
    t.cells.reserve(grid.size() - header_row - 1);
    for (std::size_t r = header_row + 1; r < grid.size(); ++r) {
        std::vector<std::string> row = grid[r];
        row.resize(ncol);  // pad short rows, drop overflow columns
        t.cells.push_back(std::move(row));
    }
    return t;
}

}  // namespace

Table read_excel(const std::string& path, const ReadOptions& opt) {
    const std::string ext = lower_ext(path);
    Grid grid;
    if (ext == "xls") {
        grid = read_xls_grid(path, opt.sheet);
    } else if (ext == "xlsx") {
        grid = read_xlsx_grid(path, opt.sheet);
    } else {
        throw std::runtime_error("unsupported input extension: ." + ext +
                                 " (expected .xls or .xlsx)");
    }
    return to_table(grid, opt.header_row);
}

}  // namespace strres
