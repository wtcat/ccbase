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
            printf(
                "lvgen "
                "[--dir=path]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
            );
            return 0;
        }

        FilePath dir(L"source");
        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");
 
        app::LvCodeGenerator *lvgen = app::LvCodeGenerator::GetInstance();
        if (lvgen->LoadAttributes(FilePath(L"lvdb.xml"))) {
            if (lvgen->LoadViews(dir))
                return !lvgen->Generate();
        }
    }

    return 0;
}
