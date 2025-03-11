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
                "rescan [--input_dir=path] [--output_dir=output file] [--sceen_width=width] [--sceen_height=height]\n"
                "Options:\n"
                "\t--input_dir         Resource directory path\n"
                "\t--output_dir        Output directory path\n"
                "\t--sceen_width       Screen width\n"
                "\t--sceen_height      Screen Height\n"
            );
            return 0;
        }

        FilePath input_dir(L"RES");
        std::string output_dir("bt_watch.ui");
        int width = 466;
        int height = 466;

        if (cmdline->HasSwitch("input_dir")) {
            input_dir.clear();
            input_dir = cmdline->GetSwitchValuePath("input_dir");
        }
        if (cmdline->HasSwitch("output_dir")) {
            output_dir.clear();
            output_dir = cmdline->GetSwitchValueASCII("output_dir");
            output_dir.append("\\").append("bt_watch.ui");
        }
        if (cmdline->HasSwitch("sceen_width")) {
            std::string str = cmdline->GetSwitchValueASCII("sceen_width");
            width = std::strtol(str.c_str(), NULL, 10);
        }
        if (cmdline->HasSwitch("sceen_height")) {
            std::string str = cmdline->GetSwitchValueASCII("sceen_height");
            height = std::strtol(str.c_str(), NULL, 10);
        }

        scoped_refptr<app::ResourceScan> scanner = new app::ResourceScan();
        if (scanner->Scan(input_dir)) {
            scoped_refptr<app::UIEditorProject> ui = new app::UIEditorProject(scanner.get(), input_dir);
            ui->SetSceenSize(width, height);
            ui->GenerateXMLDoc(output_dir);
        }
    }

    return 0;
}
