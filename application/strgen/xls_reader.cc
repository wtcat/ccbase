#include <cmath>
#include <stdexcept>
#include <string>

#include "reader_internal.h"

#include <xls.h>  // libxls: declares symbols inside `namespace xls { extern "C" }`

using namespace xls;

namespace strres {
namespace {

// Extract a cell's UTF-8 text. For numeric records prefer the exact double so an
// integral ResID like 42 never picks up number-format artifacts ("42.00",
// thousands separators) that libxls may bake into cell->str.
std::string cell_text(xlsCell* c) {
    if (!c) return {};
    switch (c->id) {
        case XLS_RECORD_NUMBER:
        case XLS_RECORD_RK: {
            const double d = c->d;
            if (std::isfinite(d) && d == std::floor(d) && std::fabs(d) < 9.0e18)
                return std::to_string(static_cast<long long>(d));
            return c->str ? std::string(c->str) : std::string();
        }
        case XLS_RECORD_BLANK:
            return {};
        default:
            return c->str ? std::string(reinterpret_cast<const char*>(c->str))
                          : std::string();
    }
}

}  // namespace

Grid read_xls_grid(const std::string& path, std::size_t sheet) {
    xls_error_t err = LIBXLS_OK;
    xlsWorkBook* wb = xls_open_file(path.c_str(), "UTF-8", &err);
    if (!wb)
        throw std::runtime_error(std::string("cannot open .xls: ") + xls_getError(err));

    if (sheet >= wb->sheets.count) {
        const DWORD n = wb->sheets.count;
        xls_close_WB(wb);
        throw std::runtime_error("sheet index " + std::to_string(sheet) +
                                 " out of range (" + std::to_string(n) + " sheets)");
    }

    xlsWorkSheet* ws = xls_getWorkSheet(wb, static_cast<int>(sheet));
    if (!ws || xls_parseWorkSheet(ws) != LIBXLS_OK) {
        if (ws) xls_close_WS(ws);
        xls_close_WB(wb);
        throw std::runtime_error("failed to parse worksheet " + std::to_string(sheet));
    }

    Grid grid;
    const WORD lastrow = ws->rows.lastrow;
    const WORD lastcol = ws->rows.lastcol;
    grid.reserve(static_cast<std::size_t>(lastrow) + 1);
    for (WORD r = 0; r <= lastrow; ++r) {
        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(lastcol) + 1);
        for (WORD col = 0; col <= lastcol; ++col)
            row.push_back(cell_text(xls_cell(ws, r, col)));
        grid.push_back(std::move(row));
    }

    xls_close_WS(ws);
    xls_close_WB(wb);
    return grid;
}

}  // namespace strres
