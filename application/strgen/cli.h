// cli.h -- command-line parsing for the strpack tool.
#pragma once

#include <cstdint>
#include <string>

#include "packer.h"
#include "refile_format.h"

namespace strres {

struct CliOptions {
    std::string input;              // positional input .xls/.xlsx
    std::string out_dir = ".";
    std::size_t sheet = 0;
    std::size_t header_row = 0;
    std::size_t id_col = 0;
    uint32_t version = refile::kDefaultVersion;
    EmptyPolicy empty = EmptyPolicy::Skip;
    std::string name_mode = "code";  // "code" | "slug"
    std::string ids_header;          // path for string_ids.h; empty = don't emit
    bool help = false;
};

std::string usage(const char* prog);

// Parse argv. Throws std::runtime_error on malformed arguments.
CliOptions parse_cli(int argc, char** argv);

}  // namespace strres
