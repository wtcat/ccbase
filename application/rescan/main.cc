/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "application/rescan/rescan.h"
#include "application/rescan/doc_generator.h"


int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    //Parse command line
    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();
        if (cmdline->HasSwitch("help")) {
            printf(
                "rescan "
                "[--input_dir=path] "
                "[--new_entry=name] "
                "[--compatible_file=project file] "
                "[--output_dir=output file] "
                "[--sceen_width=width] "
                "[--sceen_height=height]\n"
                "Options:\n"
                "\t--input_dir         Resource directory path\n"
                "\t--new_entry         New resource directory name\n"
                "\t--compatible_file   Old project file\n"
                "\t--output_dir        Output directory path\n"
                "\t--sceen_width       Screen width\n"
                "\t--sceen_height      Screen Height\n"
            );
            return 0;
        }

        FilePath input_dir(L"TemplateNew");
        FilePath::StringType entry(L"@new");
        bool compatible = cmdline->HasSwitch("compatible_file");
        int  width  = 466;
        int  height = 466;
        bool okay;

        if (cmdline->HasSwitch("input_dir")) {
            input_dir.clear();
            input_dir = cmdline->GetSwitchValuePath("input_dir");
        }
        if (cmdline->HasSwitch("sceen_width")) {
            std::string str = cmdline->GetSwitchValueASCII("sceen_width");
            width = std::strtol(str.c_str(), NULL, 10);
        }
        if (cmdline->HasSwitch("sceen_height")) {
            std::string str = cmdline->GetSwitchValueASCII("sceen_height");
            height = std::strtol(str.c_str(), NULL, 10);
        }
        if (cmdline->HasSwitch("new_entry")) {
            entry.clear();
            entry = cmdline->GetSwitchValueNative("new_entry");
        }

        scoped_refptr<app::ResourceScan> scanner = new app::ResourceScan();
        if (scanner->ScanResource(input_dir, entry, compatible)) {
            scoped_refptr<app::UIEditorProject> ui = new app::UIEditorProject(scanner.get(), input_dir);
            FilePath output_file(L"bt_watch.ui");
            if (compatible) {
                output_file.clear();
                output_file = cmdline->GetSwitchValuePath("compatible_file");
                if (!ui->SetCompatibleFile(output_file)) {
                    printf("Invalid compatible file(%s)\n", output_file.AsUTF8Unsafe().c_str());
                    return -1;
                }
                ui->SetResourceOutputPath(output_file.DirName());
            } else {
                ui->SetSceenSize(width, height);
                if (cmdline->HasSwitch("output_dir")) {
                    FilePath dir = cmdline->GetSwitchValuePath("output_dir");
                    if (!file_util::PathExists(dir)) {
                        if (!file_util::CreateDirectory(dir)) {
                            printf("Failed to create directory: %s\n", dir.AsUTF8Unsafe().c_str());
                            return false;
                        }
                    }
                    ui->SetResourceOutputPath(dir);
                } else {
                    ui->SetResourceOutputPath(FilePath(L"."));
                }
            }

            FilePath json_path(ui->output_path().Append(L"re_doc.json"));
            if (ui->GenerateJsonDoc(*scanner.get(), json_path)) {
                printf("Generated json-ui document(%s)\n", json_path.AsUTF8Unsafe().c_str());
                okay = ui->GenerateXMLDoc(
                    input_dir.Append(output_file.BaseName().InsertBeforeExtension(L"_autogen")).AsUTF8Unsafe().c_str());
                return okay ? 0 : -1;
            }
            printf("Failed to generate json-ui document\n");
        }

        return -1;
    }

    return 0;
}
