// Copyright 2026 wtcat

#include "embeded/resource/resource_file.h"

#include "regen.h"
#include "reconv.h"
#include "application/helper/utils.h"

#include <assert.h>
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/command_line.h"


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
                "[--verbose]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
                "  --job      The number of threads\n"
                "  --out      The output files\n"
                "  --format   The target format((a)rgb565,(a)rgb888)\n"
                "  --compress Compress algorithm(none, lz4)\n"
                "  --groups   The group of resource file\n"
                "  --verbose  Output log detials\n"
            );

            return 0;
        }

        FilePath dir(L"IMG");
        FilePath out(L"res.bin");
        int format = PIXEL_FORMAT_INDEXED8;
        int compress = REFILE_COMPRESS_NONE;
        int jobs = 2;

        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");

        if (!file_util::PathExists(dir)) {
            printf("Not found path(%s)\n", dir.AsUTF8Unsafe().c_str());
            return -1;
        }

        if (cmdline->HasSwitch("out"))
            out = cmdline->GetSwitchValuePath("out");

        if (cmdline->HasSwitch("job")) {
            int nr_jobs = std::stoi(cmdline->GetSwitchValueASCII("job"));
            if (nr_jobs != 0)
                jobs = nr_jobs;
        }

        if (cmdline->HasSwitch("format"))
            format = std::stoi(cmdline->GetSwitchValueASCII("format"));

        if (cmdline->HasSwitch("compress"))
            compress = std::stoi(cmdline->GetSwitchValueASCII("compress"));


        RGBConvertor conv("default");
        FileResource fres(&conv, format, compress);

        // Create image group
        if (cmdline->HasSwitch("groups")) {
            std::vector<std::string> group_name = helper::StringSplit(cmdline->GetSwitchValueASCII("groups"), ',');
            for (const auto& iter : group_name)
                fres.CreateImageGroup(dir.AppendASCII(iter));
        }

        fres.SetVerbose(cmdline->HasSwitch("verbose"));
        if (fres.CollectFiles(dir) == 0) {
            printf("Not found any pictures\n");
            return -EINVAL;
        }
        return fres.SubmitWork(jobs).GenerateResFile(out);
    }

    return 0;
}
