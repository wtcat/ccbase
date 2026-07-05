// Copyright 2026 wtcat

#include "embeded/resource/resource_file.h"

#include <errno.h>
#include "regen.h"
#include "reconv.h"
#include "helper/utils.h"
#include "helper/fnmatch.h"

#include <assert.h>
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/command_line.h"

struct kv_format {
#define TERMINAL_ITEM {nullptr, -1}
    const char* fmt;
    int value;
};

static constexpr struct kv_format px_fmttbl[] = {
    {"i8",       PIXEL_FORMAT_INDEXED8},
    {"rgb565",   PIXEL_FORMAT_RGB565},
    {"argb565",  PIXEL_FORMAT_ARGB565},
    {"rgb888",   PIXEL_FORMAT_RGB888},
    {"argb888",  PIXEL_FORMAT_ARGB888},
    TERMINAL_ITEM
};

static constexpr struct kv_format px_comptbl[] = {
    {"none",      REFILE_COMPRESS_NONE},
    {"lz4",       REFILE_COMPRESS_LZ4},
    TERMINAL_ITEM
};

static int get_key_value(const char *fmt, const struct kv_format *table) {
    for (size_t i = 0; table[i].fmt != nullptr; i++) {
        if (!strcmp(fmt, table[i].fmt))
            return table[i].value;
    }
    return -EINVAL;
}

static int get_pixel_format(const std::string& fmt) {
    return get_key_value(fmt.c_str(), px_fmttbl);
}

static int get_pixel_format_cstr(const char *fmt) {
    return get_key_value(fmt, px_fmttbl);
}

static int get_compress_algo(const std::string& comp) {
    return get_key_value(comp.c_str(), px_comptbl);
}

static int get_compress_algo_cstr(const char* comp) {
    return get_key_value(comp, px_comptbl);
}


int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();
        if (cmdline->HasSwitch("help")) {
            printf(
                "regen "
                "[--dir=path]"
                "[--out=file]\n"
                "[--job=number]\n"
                "[--format=value]"
                "[--compress=value]\n"
                "[--groups=dir1,dir2,...]\n"
                "[--filter=filter_expr1,filter_expr2...]\n"
                "[--verbose]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
                "  --job      The number of threads\n"
                "  --out      The output files\n"
                "  --format   The target format(i8, (a)rgb565,(a)rgb888)\n"
                "  --compress Compress algorithm(none, lz4)\n"
                "  --groups   The group of resource file\n"
                "  --filter   The filter expresses(PATH:FMT:COMP)\n"
                "  --verbose  Output log detials\n"
            );

            return 0;
        }

        FilePath dir(L"IMG");
        FilePath out(L"res.bin");
        int format = PIXEL_FORMAT_RGB565;
        int compress = REFILE_COMPRESS_NONE;
        int jobs = 2;

        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");

        if (!file_util::PathExists(dir)) {
            printf("Not found path(%s)\n", dir.AsUTF8Unsafe().c_str());
            return -1;
        }

        if (cmdline->HasSwitch("format")) {
            format = get_pixel_format(cmdline->GetSwitchValueASCII("format"));
            if (format < 0) {
                printf("Invalid pixel format\n");
                return -EINVAL;
            }
        }

        if (cmdline->HasSwitch("out"))
            out = cmdline->GetSwitchValuePath("out");

        if (cmdline->HasSwitch("job")) {
            int nr_jobs = std::stoi(cmdline->GetSwitchValueASCII("job"));
            if (nr_jobs != 0)
                jobs = nr_jobs;
        }

        if (cmdline->HasSwitch("compress")) {
            compress = get_compress_algo(cmdline->GetSwitchValueASCII("compress"));
            if (compress < 0) {
                printf("Invalid compress algorithm\n");
                return -EINVAL;
            }
        }

        bool verbose = cmdline->HasSwitch("verbose");

        RGBConvertor conv("default");
        FileResource fres(&conv, format, compress);

        // Set log switch
        fres.SetVerbose(verbose);

        // Add filter
        if (cmdline->HasSwitch("filter")) {
            std::vector<std::string> filter_group = helper::StringSplit(cmdline->GetSwitchValueASCII("filter"), ',');
            for (auto& iter : filter_group) {
                char* tokens[3] = {};
                int ret = helper::StrSplit(iter.data(), (int)iter.size(), tokens, sizeof(tokens) / sizeof(tokens[0]), ':');
                if (ret == 3) {
                    int format = get_pixel_format_cstr(tokens[1]);
                    if (format < 0)
                        continue;

                    int compress = get_compress_algo_cstr(tokens[2]);
                    if (compress < 0)
                        continue;

                    fres.AddFilter(tokens[0], format, compress);
                    if (verbose) {
                        printf("Add filter item: name(%s) fmt(%s:%d) comp(%s:%d)\n",
                            tokens[0], tokens[1], format, tokens[2], compress);
                    }
                }
            }
        }

        // Create image group
        if (cmdline->HasSwitch("groups")) {
            std::vector<std::string> group_name = helper::StringSplit(cmdline->GetSwitchValueASCII("groups"), ',');
            for (const auto& iter : group_name) {
                if (verbose)
                    printf("Create group: %s\n", iter.c_str());
                fres.CreateImageGroup(dir.AppendASCII(iter));
            }
        }

        // Collect all files
        if (fres.CollectFiles(dir) == 0) {
            printf("Not found any pictures\n");
            return -EINVAL;
        }

        // Convert and generate binnary file
        int err = fres.SubmitWork(jobs).GenerateResFile(out);
        if (!err)
            printf("Generated file(%s)\n", out.AsUTF8Unsafe().c_str());
    }
    return 0;
}
