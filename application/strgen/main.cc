#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <vector>

#include "bin_writer.h"
#include "cli.h"
#include "excel_reader.h"
#include "ids_header.h"
#include "packer.h"

namespace fs = std::filesystem;
using namespace strres;

int main(int argc, char** argv) {
    CliOptions o;
    try {
        o = parse_cli(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n" << usage(argv[0]);
        return 2;
    }
    if (o.help) {
        std::cout << usage(argv[0]);
        return 0;
    }

    try {
        Table table = read_excel(o.input, {o.sheet, o.header_row});
        std::vector<LanguagePack> packs =
            pack_all(table, {o.id_col, o.empty, o.name_mode});

        fs::create_directories(o.out_dir);

        // Build all bins in parallel (CPU-bound, independent), then write.
        std::vector<std::future<BinResult>> jobs;
        jobs.reserve(packs.size());
        for (auto& p : packs)
            jobs.push_back(std::async(std::launch::async, [&p, &o] {
                return build_bin(std::move(p.entries), o.version);
            }));

        std::cout << "packed " << packs.size() << " language(s) from " << o.input
                  << " (" << table.rows() << " data rows)\n";
        for (std::size_t i = 0; i < packs.size(); ++i) {
            BinResult r = jobs[i].get();
            const fs::path out = fs::path(o.out_dir) / packs[i].filename;
            write_file(out.string(), r.bytes);
            std::printf("  %-28s  %6u entries  %9zu bytes  crc=%08x\n",
                        packs[i].filename.c_str(), r.count, r.bytes.size(), r.crc);
        }

        // Optional C header mapping each ResID to its precomputed namekey.
        if (!o.ids_header.empty()) {
            std::vector<StringId> ids = collect_string_ids(table, o.id_col);
            const std::string text = render_ids_header(ids);
            const fs::path hdr = o.ids_header;
            if (hdr.has_parent_path()) fs::create_directories(hdr.parent_path());
            std::ofstream out(hdr, std::ios::binary);
            if (!out) throw std::runtime_error("cannot write " + hdr.string());
            out.write(text.data(), static_cast<std::streamsize>(text.size()));
            std::printf("  wrote %-22s  %6zu ids\n", hdr.string().c_str(), ids.size());
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
