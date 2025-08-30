//
// Copyright 2025 wtcat
//
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "driver/simulator.h"
#undef main

#include "base/bind.h"
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/files/file_path_watcher.h"
#include "base/threading/thread.h"

extern "C" int lvgl_runloop(int hor_res, int ver_res,
    void (*ui_bringup)(void),
    bool (*key_action)(int code, bool pressed));

namespace {
class FileWatcher: public base::RefCounted<FileWatcher> {
public:
    void OnFileChanged(const FilePath& path, bool error) {
        printf("%s updated\n", path.AsUTF8Unsafe().c_str());
        //TODO: notify ui service
    }
    bool AddPath(const FilePath& path) {
        return watcher_.Watch(path, base::Bind(&FileWatcher::OnFileChanged, this));
    }
    static FileWatcher* GetInstance() {
        return Singleton<FileWatcher>::get();
    }

private:
    friend struct DefaultSingletonTraits<FileWatcher>;
    DISALLOW_COPY_AND_ASSIGN(FileWatcher);
    FileWatcher() : watcher_() {}

    base::files::FilePathWatcher watcher_;
};

void OnAddFileWatchPath(void) {
    FilePath current;
    if (file_util::GetCurrentDirectory(&current)) {
        FileWatcher::GetInstance()->AddPath(current.Append(L"source\\view.xml"));
    }
}


} //namespace

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    base::Thread watcher_thread("file_watcher");

    watcher_thread.Start();
    watcher_thread.message_loop()->PostTask(FROM_HERE, base::Bind(OnAddFileWatchPath));
    lvgl_runloop(400, 400, nullptr, nullptr);

    //base::RunLoop loop;
    //loop.Run();

    return 0;
}