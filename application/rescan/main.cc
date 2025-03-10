/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "application/rescan/rescan.h"
#include "application/rescan/xml_generator.h"


int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    //Parse command line
    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();

        if (cmdline->HasSwitch("help")) {
            printf(
                "rescan \n"
                "Options:\n"
            );
            return 0;
        }


        scoped_refptr<app::ResourceScan> scanner = new app::ResourceScan();
        FilePath path(L"RES");

        if (scanner->Scan(path)) {
            app::UIEditorProject ui(scanner.get(), path);
            ui.SetSceenSize(454, 454);
            ui.GenerateXMLDoc("bt_watch.ui");
        }
    }

    return 0;
}
