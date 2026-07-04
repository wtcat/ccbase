#include "cli.h"

#include <cstdlib>
#include <stdexcept>

namespace strres {
namespace {

std::size_t to_size(const std::string& v, const char* opt) {
    try {
        return static_cast<std::size_t>(std::stoul(v));
    } catch (...) {
        throw std::runtime_error(std::string("invalid value for ") + opt + ": " + v);
    }
}

std::string next(int argc, char** argv, int& i, const char* opt) {
    if (++i >= argc) throw std::runtime_error(std::string("missing value for ") + opt);
    return argv[i];
}

}  // namespace

std::string usage(const char* prog) {
    return std::string("Usage: ") + prog +
           " <input.xls|.xlsx> [options]\n"
           "\n"
           "Pack a multi-language string spreadsheet into per-language .bin files.\n"
           "\n"
           "Options:\n"
           "  --out-dir DIR       output directory (default: .)\n"
           "  --sheet N           0-based worksheet index (default: 0)\n"
           "  --header-row N      0-based header row (default: 0)\n"
           "  --id-col N          0-based ResID column (default: 0)\n"
           "  --version V         header version field (default: 1)\n"
           "  --empty MODE        empty cell: skip|emit|error (default: skip)\n"
           "  --name-mode MODE    filename: code|slug (default: code)\n"
           "  --ids-header PATH   also write a C header of ResID -> hash macros\n"
           "  -h, --help          show this help\n";
}

CliOptions parse_cli(int argc, char** argv) {
    CliOptions o;
    bool have_input = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            o.help = true;
            return o;
        } else if (a == "--out-dir") {
            o.out_dir = next(argc, argv, i, "--out-dir");
        } else if (a == "--sheet") {
            o.sheet = to_size(next(argc, argv, i, "--sheet"), "--sheet");
        } else if (a == "--header-row") {
            o.header_row = to_size(next(argc, argv, i, "--header-row"), "--header-row");
        } else if (a == "--id-col") {
            o.id_col = to_size(next(argc, argv, i, "--id-col"), "--id-col");
        } else if (a == "--version") {
            o.version = static_cast<uint32_t>(
                to_size(next(argc, argv, i, "--version"), "--version"));
        } else if (a == "--empty") {
            const std::string m = next(argc, argv, i, "--empty");
            if (m == "skip") o.empty = EmptyPolicy::Skip;
            else if (m == "emit") o.empty = EmptyPolicy::Emit;
            else if (m == "error") o.empty = EmptyPolicy::Error;
            else throw std::runtime_error("invalid --empty: " + m);
        } else if (a == "--name-mode") {
            o.name_mode = next(argc, argv, i, "--name-mode");
            if (o.name_mode != "code" && o.name_mode != "slug")
                throw std::runtime_error("invalid --name-mode: " + o.name_mode);
        } else if (a == "--ids-header") {
            o.ids_header = next(argc, argv, i, "--ids-header");
        } else if (!a.empty() && a[0] == '-') {
            throw std::runtime_error("unknown option: " + a);
        } else if (!have_input) {
            o.input = a;
            have_input = true;
        } else {
            throw std::runtime_error("unexpected argument: " + a);
        }
    }
    if (!o.help && !have_input) throw std::runtime_error("no input file given");
    return o;
}

}  // namespace strres
