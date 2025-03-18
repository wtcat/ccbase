/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "application/codegen/codegen.h"



int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    //Parse command line
    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();

        if (cmdline->HasSwitch("help")) {
            printf(
                "codegen "
                "[--view_base=value] "
                "[--resource_fnname = name] "
                "[--input_file = filename] "
                "[--output_dir = output path] "
                "[--defaut_fontfile = fontfile] "
                "[--overwrite]\n"
                "Options:\n"
                "  --view_base          The base ID for views. (default: 0)\n"
                "  --resource_fnname    The function name that get resource by view ID. (default: _sdk_view_get_resource)\n"
                "  --input_file         The resource information file. (default: re_output.json)\n"
                "  --output_dir         The output directory\n"
                "  --defaut_fontfile    The default font file\n"
                "  --overwrite          Overwrite files that has exists"
            );
            return 0;
        }

        scoped_refptr<app::ViewCodeFactory> factory(new app::ViewCodeFactory);
        FilePath file(L"re_output.json");
        FilePath outpath(L".");
        std::string rfnname("_sdk_view_get_resource");
        std::string default_font("font");
        int view_base = 0;
        bool overwrite;

        if (cmdline->HasSwitch("view_base"))
            view_base = std::stoi(cmdline->GetSwitchValueASCII("view_base"));

        if (cmdline->HasSwitch("input_file")) {
            file.clear();
            file = cmdline->GetSwitchValuePath("input_file");
        }

        if (cmdline->HasSwitch("resource_fnname")) {
            rfnname.clear();
            rfnname = cmdline->GetSwitchValueASCII("resource_fnname");
        }

        if (cmdline->HasSwitch("output_dir")) {
            outpath.clear();
            outpath = cmdline->GetSwitchValuePath("output_dir");
        }

        if (cmdline->HasSwitch("defaut_fontfile")) {
            default_font.clear();
            default_font = cmdline->GetSwitchValueASCII("defaut_fontfile");
        }

        overwrite = cmdline->HasSwitch("overwrite");

        if (!file_util::PathExists(outpath)) {
            if (!file_util::CreateDirectory(outpath)) {
                DLOG(ERROR) << "Failed to create directory: " << outpath.value();
                return false;
            }
        }
        
        //Inject generate options
        factory->SetOptions(view_base, rfnname, outpath, default_font);

        //Generate template code
        if (!factory->GenerateViewCode(file, overwrite)) {
            DLOG(ERROR) << "Generate error";
            return -1;
        }
    }

    return 0;
}
