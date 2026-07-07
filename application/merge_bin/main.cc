#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "merger.h"

namespace {

int usage(const char* prog) {
    std::fprintf(stderr,
        "usage:\n"
        "  %s pack -o <out.bin> <file1> <file2> ...   merge files, in order\n"
        "  %s info <container.bin>                      verify and list entries\n",
        prog, prog);
    return 2;
}

int cmd_pack(int argc, char** argv) {
    std::string out;
    std::vector<std::string> inputs;

    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "error: -o requires an argument\n");
                return 2;
            }
            out = argv[++i];
        } else {
            inputs.push_back(arg);
        }
    }

    if (out.empty()) {
        std::fprintf(stderr, "error: output file (-o) is required\n");
        return 2;
    }
    if (inputs.empty()) {
        std::fprintf(stderr, "error: at least one input file is required\n");
        return 2;
    }

    try {
        std::vector<uint8_t> blob = binfile::merge(inputs);
        std::ofstream os(out, std::ios::binary | std::ios::trunc);
        if (!os) {
            std::fprintf(stderr, "error: cannot open output file: %s\n", out.c_str());
            return 1;
        }
        os.write(reinterpret_cast<const char*>(blob.data()),
                 static_cast<std::streamsize>(blob.size()));
        if (!os) {
            std::fprintf(stderr, "error: failed writing output file: %s\n", out.c_str());
            return 1;
        }
        std::printf("packed %zu file(s) into %s (%zu bytes)\n",
                    inputs.size(), out.c_str(), blob.size());
        return 0;
    } catch (const binfile::MergeError& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}

int cmd_info(int argc, char** argv) {
    if (argc != 1) {
        std::fprintf(stderr, "error: info takes exactly one container file\n");
        return 2;
    }
    const std::string path = argv[0];

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        std::fprintf(stderr, "error: cannot open container: %s\n", path.c_str());
        return 1;
    }
    std::streamsize len = in.tellg();
    std::vector<uint8_t> buf(static_cast<size_t>(len < 0 ? 0 : len));
    in.seekg(0);
    if (len > 0) {
        in.read(reinterpret_cast<char*>(buf.data()), len);
    }

    try {
        binfile::ContainerInfo info = binfile::parse(buf);
        std::printf("magic BINS, count %u\n", info.count);
        std::printf("%-5s %-10s %-10s %-10s\n", "idx", "namekey", "offset", "size");
        for (uint32_t i = 0; i < info.count; i++) {
            const auto& e = info.entries[i];
            std::printf("%-5u 0x%08x %-10u %-10u\n", i, e.namekey, e.offset, e.size);
        }
        std::printf("CRC: %s (stored=0x%08x computed=0x%08x)\n",
                    info.crc_ok() ? "OK" : "MISMATCH",
                    info.stored_crc, info.computed_crc);
        return info.crc_ok() ? 0 : 1;
    } catch (const binfile::MergeError& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        return usage(argv[0]);
    }
    std::string cmd = argv[1];
    if (cmd == "pack") {
        return cmd_pack(argc - 2, argv + 2);
    }
    if (cmd == "info") {
        return cmd_info(argc - 2, argv + 2);
    }
    return usage(argv[0]);
}
