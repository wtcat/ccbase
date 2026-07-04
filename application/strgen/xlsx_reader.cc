#include <cctype>
#include <cstddef>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "reader_internal.h"

#include <miniz.h>  // umbrella header: pulls miniz_common + miniz_zip in order
#include <pugixml.hpp>

namespace strres {
namespace {

struct Zip {
    mz_zip_archive z{};
    explicit Zip(const std::string& path) {
        if (!mz_zip_reader_init_file(&z, path.c_str(), 0))
            throw std::runtime_error("cannot open .xlsx (not a valid zip): " + path);
    }
    ~Zip() { mz_zip_reader_end(&z); }

    std::optional<std::string> read(const std::string& name) {
        std::size_t size = 0;
        void* p = mz_zip_reader_extract_file_to_heap(&z, name.c_str(), &size, 0);
        if (!p) return std::nullopt;
        std::string out(static_cast<const char*>(p), size);
        mz_free(p);
        return out;
    }
};

// 0-based column index from an A1-style cell ref ("B12" -> 1).
std::size_t col_from_ref(const char* ref) {
    std::size_t col = 0;
    for (; *ref && std::isalpha(static_cast<unsigned char>(*ref)); ++ref)
        col = col * 26 + (std::toupper(static_cast<unsigned char>(*ref)) - 'A' + 1);
    return col ? col - 1 : 0;
}

// Concatenate every <t> under a shared-string <si> (handles rich-text runs).
std::string si_text(const pugi::xml_node& si) {
    std::string out;
    if (auto t = si.child("t")) out += t.text().get();
    for (auto r : si.children("r"))
        if (auto t = r.child("t")) out += t.text().get();
    return out;
}

std::vector<std::string> load_shared_strings(Zip& zip) {
    std::vector<std::string> shared;
    auto xml = zip.read("xl/sharedStrings.xml");
    if (!xml) return shared;  // workbook may use only inline strings
    pugi::xml_document doc;
    if (!doc.load_buffer(xml->data(), xml->size()))
        throw std::runtime_error("malformed sharedStrings.xml");
    auto sst = doc.child("sst");
    shared.reserve(sst.attribute("uniqueCount").as_uint());
    for (auto si : sst.children("si")) shared.push_back(si_text(si));
    return shared;
}

// Resolve the worksheet part path for the Nth sheet via workbook.xml + rels.
// Falls back to xl/worksheets/sheet{N+1}.xml when rels are unavailable.
std::string sheet_part(Zip& zip, std::size_t sheet) {
    const std::string fallback =
        "xl/worksheets/sheet" + std::to_string(sheet + 1) + ".xml";

    auto wbxml = zip.read("xl/workbook.xml");
    auto relxml = zip.read("xl/_rels/workbook.xml.rels");
    if (!wbxml || !relxml) return fallback;

    pugi::xml_document wb, rel;
    if (!wb.load_buffer(wbxml->data(), wbxml->size()) ||
        !rel.load_buffer(relxml->data(), relxml->size()))
        return fallback;

    auto sheets = wb.child("workbook").child("sheets");
    std::size_t i = 0;
    std::string rid;
    for (auto s : sheets.children("sheet")) {
        if (i++ == sheet) {
            rid = s.attribute("r:id").as_string();
            break;
        }
    }
    if (rid.empty()) return fallback;

    for (auto r : rel.child("Relationships").children("Relationship")) {
        if (rid == r.attribute("Id").as_string()) {
            std::string target = r.attribute("Target").as_string();
            if (target.rfind("/", 0) == 0) return target.substr(1);        // absolute
            if (target.rfind("xl/", 0) == 0) return target;                 // already rooted
            return "xl/" + target;                                          // relative to xl/
        }
    }
    return fallback;
}

}  // namespace

Grid read_xlsx_grid(const std::string& path, std::size_t sheet) {
    Zip zip(path);
    const std::vector<std::string> shared = load_shared_strings(zip);
    const std::string part = sheet_part(zip, sheet);

    auto sheetxml = zip.read(part);
    if (!sheetxml)
        throw std::runtime_error("worksheet part not found: " + part);

    pugi::xml_document doc;
    if (!doc.load_buffer(sheetxml->data(), sheetxml->size()))
        throw std::runtime_error("malformed worksheet xml: " + part);

    Grid grid;
    auto data = doc.child("worksheet").child("sheetData");
    for (auto row : data.children("row")) {
        std::vector<std::string> cells;
        for (auto c : row.children("c")) {
            const std::size_t col = col_from_ref(c.attribute("r").as_string());
            if (col >= cells.size()) cells.resize(col + 1);

            const std::string t = c.attribute("t").as_string();
            std::string value;
            if (t == "s") {  // shared string index
                const unsigned idx = c.child("v").text().as_uint();
                if (idx < shared.size()) value = shared[idx];
            } else if (t == "inlineStr") {
                value = si_text(c.child("is"));
            } else {  // "str" (formula), "b", numeric, date -> raw <v>
                value = c.child("v").text().get();
            }
            cells[col] = std::move(value);
        }
        grid.push_back(std::move(cells));
    }
    return grid;
}

}  // namespace strres
