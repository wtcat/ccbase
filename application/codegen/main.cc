/*
 * Copyright 2025 wtcat 
 */

#include "base/at_exit.h"
#include "base/callback.h"
#include "base/bind.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/worker_pool.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"

#include "application/codegen/codegen.h"


int main(int argc, char* argv[]) {
    //CommandLine cmdline(argc, argv);
    base::AtExitManager atexit;

    scoped_refptr<app::CodeBuilder> builder = new app::ResourceCodeBuilder();
    builder->GenerateCode(FilePath(L"re_output.json"), FilePath(L"ui_resource.c"));

    return 0;
}
