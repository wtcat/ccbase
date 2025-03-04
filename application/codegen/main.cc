/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "application/codegen/codegen.h"


int main(int argc, char* argv[]) {
    //CommandLine cmdline(argc, argv);
    base::AtExitManager atexit;

    scoped_refptr<app::ViewCodeFactory> factory(new app::ViewCodeFactory);
    factory->GenerateViewCode(FilePath(L"re_output.json"));

    return 0;
}
