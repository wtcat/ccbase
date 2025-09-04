//
// Copyright 2025 wtcat
//
#include <stdio.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/core/lv_obj_private.h"
#include "driver/simulator.h"
#undef main

#include <unordered_map>
#include <filesystem>

#include "base/bind.h"
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/linked_list.h"
#include "base/base_time.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/files/file_path_watcher.h"
#include "base/threading/thread.h"
#include "base/command_line.h"

namespace {
namespace fs = std::filesystem;

void ShowLayout(lv_obj_t* obj, int level) {
    int children = lv_obj_get_child_count(obj);

    for (int i = 0; i < level; i++)
        printf("    ");

    printf("=>obj(%s@%p) flags(0x%x) childcnt(%d) coord{(%d, %d) (%d, %d)} class_p(%p)\n",
        lv_obj_get_name(obj),
        obj,
        obj->flags,
        children,
        obj->coords.x1, obj->coords.y1, obj->coords.x2, obj->coords.y2,
        obj->class_p);

    for (int i = 0; i < children; i++)
        ShowLayout(lv_obj_get_child(obj, i), level + 1);
}

bool KeyProcess(int code, bool pressed) {
    if (pressed) {
        switch (code) {
        case SDLK_ESCAPE:
            printf("Exit code: %d\n", code);
            return false;
        case SDLK_SPACE:
            ShowLayout(lv_screen_active(), 0);
            break;
        default:
            break;
        }
    }
    return true;
}

class FileWatcher {
public:
    struct WatcherNode : public base::RefCounted<WatcherNode> {
        base::files::FilePathWatcher watcher;
    };

    FileWatcher() = default;
    ~FileWatcher() = default;
    bool AddPath(const FilePath& path, const base::files::FilePathWatcher::Callback &callback) {
        scoped_refptr<WatcherNode> ptr = new WatcherNode();
        std::string str(path.AsUTF8Unsafe());
        auto item = watcher_nodes_.find(str);
        if (item != watcher_nodes_.end())
            return true;

        if (ptr->watcher.Watch(path, callback)) {
            watcher_nodes_.insert(std::make_pair(str, ptr));
            return true;
        }

        return false;
    }
    void RemovePath(const FilePath& path) {
        std::string str(path.AsUTF8Unsafe());
        auto item = watcher_nodes_.find(str);
        if (item != watcher_nodes_.end())
            watcher_nodes_.erase(str);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(FileWatcher);
    std::unordered_map<std::string, scoped_refptr<WatcherNode>> watcher_nodes_;
};

class ViewManager : public base::RefCountedThreadSafe<ViewManager> {
public:
    struct ViewFile {
        ViewFile() = default;
        ViewFile(const FilePath& path): file(path) {
            view = path.BaseName().RemoveExtension().AsUTF8Unsafe();
        }
        FilePath    file;
        std::string view;
    };

    struct ViewNode : public base::LinkNode<ViewNode> {
        ViewNode(const FilePath& path) : view(path) {}
        ViewFile view;
    };

    struct ViewInitParam {
        base::Thread* thread;
        ViewManager* view;
    };

    ViewManager() = default;
    ~ViewManager() {
        ClearComponents();
    }
    template<typename Fn>
    void WalkAroundViews(base::LinkedList<ViewNode>& list, Fn&& fn) {
        for (base::LinkNode<ViewNode>* node = list.head(), *next;
            node != list.end();
            node = next) {
            next = node->next();
            fn(node->value());
        }
    }
    void SetActiveFile(const FilePath& path) {
        updated_file = path;
    }
    bool ReloadView(const FilePath& view_file) {
        // Destroy current view
        lv_obj_clean(lv_screen_active());
        if (!active_view_.file.empty())
            lv_xml_component_unregister(active_view_.view.c_str());

        if (file_util::PathExists(view_file)) {
            active_view_.file = view_file;
            active_view_.view = view_file.BaseName().RemoveExtension().AsUTF8Unsafe();

            if (RegisterView(view_file))
                return lv_xml_create(lv_screen_active(), active_view_.view.c_str(), nullptr) != nullptr;
        }

        return false;
    }
    void AddComponents(const FilePath& dir) {
        // Add view components
        FilePath component_dir = dir.Append(FilePath(L"components"));
        if (file_util::PathExists(component_dir)) {
            const fs::path root_path(component_dir.MaybeAsASCII());
            for (const auto& entry : fs::directory_iterator(root_path)) {
                if (fs::is_regular_file(entry)) {
                    if (entry.path().filename().extension() == ".xml") {
                        FilePath view_path(FilePath::FromUTF8Unsafe(entry.path().string()));
                        if (RegisterView(view_path))
                            components_.Append(new ViewNode(view_path));
                    }
                }
            }
        }

        // Add views
        const fs::path views_path(dir.MaybeAsASCII());
        for (const auto& entry : fs::directory_iterator(views_path)) {
            if (fs::is_regular_file(entry)) {
                if (entry.path().filename().extension() == ".xml") {
                    FilePath view_path(FilePath::FromUTF8Unsafe(entry.path().string()));
                    file_watcher_.AddPath(view_path, base::Bind(&ViewManager::OnFileChanged, this));
                }
            }
        }
    }
    void OnFileChanged(const FilePath& path, bool error) {
        if (!error) {
            printf("%s updated\n", path.AsUTF8Unsafe().c_str());
            SetActiveFile(path);
            lvgl_send_message(ViewManager::ViewMessageCallback, 0, this);
        }
    }
    void OnAddFileWatchPath(void) {
        if (!file_util::AbsolutePath(&root_dir_)) {
            FilePath current;
            if (file_util::GetCurrentDirectory(&current))
                AddComponents(current.Append(root_dir_));
        } else
            AddComponents(root_dir_);
    }
    void Run(const FilePath& dir, int hres, int vres) {
        base::Thread watcher_thread("file_watcher");
        ViewInitParam param;
 
        param.thread = &watcher_thread;
        param.view = this;
        root_dir_ = dir;
        lvgl_runloop(hres, vres, &ViewManager::Init, &param, KeyProcess);
    }
    static void ViewMessageCallback(struct lvgl_message* msg) {
        ViewManager* vm = (ViewManager*)msg->user;
        vm->ReloadView(vm->updated_file);
    }
    static void Init(void* ptr) {
        ViewInitParam* p = (ViewInitParam*)ptr;

        p->thread->Start();
        p->thread->message_loop()->PostTask(FROM_HERE,
            base::Bind(&ViewManager::OnAddFileWatchPath, p->view)
        );

        lv_obj_set_name_static(lv_screen_active(), "ActiveScreen");
        lv_obj_set_name_static(lv_layer_top(), "TopLayerScreen");
        lv_obj_set_name_static(lv_layer_sys(), "SysLayerScreen");
        lv_obj_set_name_static(lv_layer_bottom(), "BottomLayerScreen");
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ViewManager);
    bool RegisterView(const FilePath& file) {
        std::string path("A:");
        path.append(file.AsUTF8Unsafe());
        return lv_xml_component_register_from_file(path.c_str()) == LV_RESULT_OK;
    }
    void ClearComponents(void) {
        WalkAroundViews(components_, [](ViewNode *node) {
            lv_xml_component_unregister(node->view.view.c_str());
            node->RemoveFromList();
            delete node;
            });
        
        if (active_view_.view.size() > 0)
            lv_xml_component_unregister(active_view_.view.c_str());
    }

private:
    base::LinkedList<ViewNode> components_;
    base::LinkedList<ViewNode> views_;
    ViewFile active_view_;
    FilePath root_dir_;
    FilePath updated_file;
    FileWatcher file_watcher_;
};
} //namespace

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();
        if (cmdline->HasSwitch("help")) {
            printf(
                "lvsim "
                "[--dir=path]"
                "[--hres=value]"
                "[--vres=value]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
                "  --hres     The horizontal resolution of window\n"
                "  --vres     The vertical resolution of window\n"
            );

            return 0;
        }

        FilePath dir(L"source");
        int hres = 480;
        int vres = 480;

        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");

        if (!file_util::PathExists(dir)) {
            printf("Not found path(%s)\n", dir.AsUTF8Unsafe().c_str());
            return -1;
        }

        if (cmdline->HasSwitch("hres"))
            hres = std::stoi(cmdline->GetSwitchValueASCII("hres"));

        if (cmdline->HasSwitch("vres"))
            vres = std::stoi(cmdline->GetSwitchValueASCII("vres"));

        scoped_refptr<ViewManager> viewptr(new ViewManager);
        viewptr->Run(dir, hres, vres);
    }

    return 0;
}