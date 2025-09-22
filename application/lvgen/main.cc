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

#include "lvgen.h"

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    bool okay = false;

    //Parse command line
    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();

        if (cmdline->HasSwitch("help")) {
            printf("lvgen [--indir=input directory] [--outdir=output directory] [--outdb]\n");
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

        leveldb::DB* db = nullptr;
        //if (cmdline->HasSwitch("outdb")) 
        {
            FilePath db_path = outdir.Append(L"DB");
            leveldb::Options options;

            options.create_if_missing = true;
            if (file_util::PathExists(db_path))
                leveldb::DestroyDB(db_path.AsUTF8Unsafe(), options);
            leveldb::DB::Open(options, db_path.AsUTF8Unsafe(), &db);
        }

        scoped_ptr<leveldb::DB> ptr(db);
        app::LvCodeGenerator *lvgen = app::LvCodeGenerator::GetInstance();
        if (lvgen->LoadAttributes(FilePath(L"lvdb.xml"))) {
            if (lvgen->LoadViews(indir))
                okay = lvgen->Generate(outdir, db);
        }
    }

    return okay? 0: -1;
}
