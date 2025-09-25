/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "thirdparty/leveldb/include/leveldb/db.h"

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
                "[--database_dir = database path] "
                "[--defaut_fontfile = fontfile] "
                "[--resource_namespace = namespace] "
                "[--resource_ids_cfile = filename] "
                "[--resource_list_cfile = filename] "
                "[--overwrite]\n"
                "Options:\n"
                "  --view_base               The base ID for views. (default: 0)\n"
                "  --resource_fnname         The function name that get resource by view ID. (default: _sdk_view_get_resource)\n"
                "  --input_file              The resource information file. (default: re_output.json)\n"
                "  --output_dir              The output directory\n"
                "  --database_dir            The input directory of lvgl database\n"
                "  --defaut_fontfile         The default font file\n"
                "  --resource_namespace      Current resource namespace\n"
                "  --resource_ids_cfile      Resource IDs c header file name\n"
                "  --resource_list_cfile     Resource list c source file name\n"
                "  --overwrite               Overwrite files that has exists\n"
            );
            return 0;
        }

        scoped_refptr<app::ViewCodeFactory> factory(new app::ViewCodeFactory);
        scoped_refptr<app::ResourceParser::ResourceOptions> option(new app::ResourceParser::ResourceOptions);
        FilePath file(L"re_output.json");
        bool overwrite;

        if (cmdline->HasSwitch("input_file")) {
            file.clear();
            file = cmdline->GetSwitchValuePath("input_file");
        }

        if (cmdline->HasSwitch("view_base"))
            option->id_base = std::stoi(cmdline->GetSwitchValueASCII("view_base"));

        if (cmdline->HasSwitch("resource_fnname"))
            option->resource_fnname = cmdline->GetSwitchValueASCII("resource_fnname");
        else
            option->resource_fnname = "_sdk_view_get_resource";

        if (cmdline->HasSwitch("resource_namespace"))
            option->resource_namespace = cmdline->GetSwitchValueASCII("resource_namespace");
        else
            option->resource_namespace = "general";
        
        if (cmdline->HasSwitch("output_dir"))
            option->outpath = cmdline->GetSwitchValuePath("output_dir");
        else
            option->outpath = FilePath(L".");

        if (cmdline->HasSwitch("defaut_fontfile"))
            option->default_font = cmdline->GetSwitchValueASCII("defaut_fontfile");
        else
            option->default_font = "font";

        if (cmdline->HasSwitch("resource_ids_cfile"))
            option->ui_ids_filename = cmdline->GetSwitchValueASCII("resource_ids_cfile");
        else
            option->ui_ids_filename = "ui_template_ids.h";

        if (cmdline->HasSwitch("resource_list_cfile"))
            option->ui_res_filename = cmdline->GetSwitchValueASCII("resource_list_cfile");
        else
            option->ui_res_filename = "ui_template_resource.c";

        overwrite = cmdline->HasSwitch("overwrite");

        if (cmdline->HasSwitch("database_dir")) {
            FilePath tdir = cmdline->GetSwitchValuePath("database_dir");

            FilePath db_path = tdir.Append(L"DB");
            leveldb::Options options;
            if (file_util::PathExists(db_path))
                leveldb::DB::Open(options, db_path.AsUTF8Unsafe(), &option->db);
        }

        scoped_ptr<leveldb::DB> ptr(option->db);
        if (!file_util::PathExists(option->outpath)) {
            if (!file_util::CreateDirectory(option->outpath)) {
                DLOG(ERROR) << "Failed to create directory: " << option->outpath.value();
                return false;
            }
        }
        
        //Inject generate options
        factory->SetOptions(option);

        //Generate template code
        if (!factory->GenerateViewCode(file, overwrite)) {
            DLOG(ERROR) << "Generate error";
            return -1;
        }
    }

    return 0;
}
