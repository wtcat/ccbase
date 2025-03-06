/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
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
                "codegen [--view_base=value] [--resource_fnname=name] [--input_file=filename] [--output_dir]\n"
                "Options:\n"
                "  --view_base          The base ID for views. (default: 0)\n"
                "  --resource_fnname    The function name that get resource by view ID. (default: _sdk_view_get_resource)\n"
                "  --input_file         The resource information file. (default: re_output.json)\n"
                "  --output_dir         The output directory\n"
            );
            return 0;
        }

        scoped_refptr<app::ViewCodeFactory> factory(new app::ViewCodeFactory);
        FilePath file(L"re_output.json");
        FilePath outpath(L".");
        std::string rfnname("_sdk_view_get_resource");
        int view_base = 0;

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

        //Inject generate options
        factory->SetOptions(view_base, rfnname, outpath);

        //Generate template code
        factory->GenerateViewCode(file);
    }

    return 0;
}
