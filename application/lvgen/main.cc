/*
 * Copyright 2025 wtcat
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "lvgen.h"

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    //Parse command line
    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();

        if (cmdline->HasSwitch("help")) {
            printf("lvgen [--indir=input directory] [--outdir=output directory]\n");
            return 0;
        }

        FilePath indir(L"source");
        if (cmdline->HasSwitch("indir"))
            indir = cmdline->GetSwitchValuePath("indir");
 
        if (!file_util::PathExists(indir)) {
            printf("Not found path(%s)\n", indir.AsUTF8Unsafe().c_str());
            return -1;
        }

        FilePath outdir(L"code");
        if (cmdline->HasSwitch("outdir"))
            outdir = cmdline->GetSwitchValuePath("outdir");

        if (!file_util::PathExists(outdir))
            file_util::CreateDirectory(outdir);

        app::LvCodeGenerator *lvgen = app::LvCodeGenerator::GetInstance();
        if (lvgen->LoadAttributes(FilePath(L"lvdb.xml"))) {
            if (lvgen->LoadViews(indir))
                return !lvgen->Generate(outdir);
        }
    }

    return 0;
}
