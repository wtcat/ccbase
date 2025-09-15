/*
 * Copyright 2025 wtcat
 */

#define BASE_IMPLEMENTATION
#include "base/base_export.h"
#include "lvgl_res_loader.h"

#include "plugin/plugin.h"
#include "base/linked_list.h"


namespace {
struct SceneResource : public base::LinkNode<SceneResource> {
    enum {
        kImageResource,
        kTextResource
    };
    SceneResource(uint32_t uid) : id(uid), image() {}
    union {
        lv_image_dsc_t image;
        lvgl_res_string_t text;
    };
    uint32_t id;
    int type;
};

struct SceneContext : public base::LinkNode<SceneContext> {
    SceneResource* NewResource(uint32_t id) {
        SceneResource* r = new SceneResource(id);
        list.Append(r);
        return r;
    }
    void DeleteResource(SceneResource* r) {
        r->RemoveFromList();
        delete r;
    }
    SceneResource* GetResource(uint32_t id) {
        for (base::LinkNode<SceneResource>* node = list.head();
            node != list.end(); node = node->next()) {
            if (node->value()->id == id)
                return node->value();
        }
        return nullptr;
    }
    void UnloadAllResources(void) {
        for (base::LinkNode<SceneResource>* node = list.head();
            node != list.end(); node = node->next()) {
            UnloadResource(node->value());
        }
    }
    ~SceneContext() {
        base::LinkNode<SceneResource>* node = list.head();
        base::LinkNode<SceneResource>* next;

        UnloadAllResources();
        while (node != list.end()) {
            next = node->next();
            DeleteResource(node->value());
            node = next;
        }
    }
    void UnloadResource(SceneResource* r) {
        switch (r->type) {
        case SceneResource::kImageResource:
            lvgl_res_unload_pictures(&r->image, 1);
            break;
        case SceneResource::kTextResource:
            lvgl_res_unload_strings(&r->text, 1);
            break;
        default:
            break;
        }
    }

    base::LinkedList<SceneResource> list;
    lvgl_res_scene_t scene;
    uint32_t scene_id;
};

class SharedLoader {
public:
    SharedLoader() {
        if (!intialized_) {
            intialized_ = true;
            lvgl_res_loader_init(480, 480);
        }
    }
    virtual ~SharedLoader() {
        ClearScene();
        if (intialized_) {
            intialized_ = false;
            lvgl_res_loader_deinit();
        }
    }
    SceneContext* LoadScene(const char* id, const char* path, const char *strfile) {
        uint32_t scene_id = Hash((const uint8_t*)id, strlen(id));
        SceneContext* ctx = SceneFind(scene_id);
        if (ctx == nullptr) {
            FilePath dir = FilePath::FromUTF8Unsafe(path);
            FilePath sty = dir.Append(FilePath(L"bt_watch.sty"));
            FilePath res = dir.Append(FilePath(L"bt_watch.res"));
            FilePath str;

            if (strfile == nullptr)
                str = dir.Append(FilePath(L"bt_watch.Eng"));
            else
                str = dir.Append(FilePath::FromUTF8Unsafe(strfile));

            ctx = SceneAllocate();
            int err = lvgl_res_load_scene(scene_id, &ctx->scene,
                sty.AsUTF8Unsafe().c_str(),
                res.AsUTF8Unsafe().c_str(),
                str.AsUTF8Unsafe().c_str()
            );
            if (err != 0) {
                SceneFree(ctx);
                return nullptr;
            }
            ctx->scene_id = scene_id;
        }

        return ctx;
    }
    SceneResource* GetResourceItem(SceneContext* ctx, const std::string& name, int* found) {
        if (IsSceneActived(ctx)) {
            uint32_t id = Hash((const uint8_t*)name.c_str(), (uint32_t)name.size());
            SceneResource* r = ctx->GetResource(id);
            if (r != nullptr) {
                *found = 1;
                return r;
            }

            r = ctx->NewResource(id);
            if (!GetSceneResource(ctx, r, id)) {
                ctx->DeleteResource(r);
                return nullptr;
            }
            *found = 0;
            return r;
        }

        return nullptr;
    }
    void UnloadScene(SceneContext* ctx) {
        if (IsSceneActived(ctx)) {
            ctx->UnloadAllResources();
            lvgl_res_unload_scene_compact(ctx->scene_id);
            lvgl_res_unload_scene(&ctx->scene);
            SceneFree(ctx);
        }
    }
    void ClearScene() {
        base::LinkNode<SceneContext>* node = scene_list_.head();
        base::LinkNode<SceneContext>* next;
        while (node != scene_list_.end()) {
            next = node->next();
            UnloadScene(node->value());
            node = next;
        }
    }

private:
    virtual bool GetSceneResource(SceneContext* ctx, SceneResource* r, uint32_t id) = 0;
    SceneContext* SceneFind(uint32_t id) {
        for (base::LinkNode<SceneContext>* node = scene_list_.head();
            node != scene_list_.end(); node = node->next()) {
            if (node->value()->scene_id == id)
                return node->value();
        }
        return nullptr;
    }
    bool IsSceneActived(SceneContext* ctx) {
        if (ctx == nullptr)
            return false;

        for (base::LinkNode<SceneContext>* node = scene_list_.head();
            node != scene_list_.end(); node = node->next()) {
            if (node->value() == ctx)
                return true;
        }
        return false;
    }
    SceneContext* SceneAllocate() {
        SceneContext* scene = new SceneContext;
        scene_list_.Append(scene);
        return scene;
    }
    void SceneFree(SceneContext* scene) {
        scene->RemoveFromList();
        delete scene;
    }
    uint32_t Hash(const uint8_t* key, uint32_t len) {
        uint32_t hash = 0;
        for (const uint8_t* end = key + len; key < end; key++) {
            hash *= 16777619;
            hash ^= (uint32_t)(*key);
        }
        return hash;
    }
private:
    base::LinkedList<SceneContext> scene_list_;
    static bool intialized_;
};

bool SharedLoader::intialized_;

// Image loader
class ImageLoader : public lvsim::ResourceLoader, public SharedLoader {
public:
    ImageLoader(const std::string& name) : ResourceLoader(name), SharedLoader() {}
    virtual ~ImageLoader() {
        Clear();
    }
    ReHandle Load(const Attribute& attr) override {
        const char* path = attr.GetValue("src_path");
        if (path == nullptr)
            return nullptr;

        const char* id = attr.GetValue("id");
        if (id == nullptr)
            return nullptr;

        return LoadScene(id, path, nullptr);
    }
    bool Get(ReHandle h, const std::string& name, Attribute& attr) override {
        SceneContext* ctx = (SceneContext*)h;
        int found;

        SceneResource* r = GetResourceItem(ctx, name, &found);
        if (r != nullptr && !found)
            return attr.RegisterImage(name.c_str(), &r->image);

        return r != nullptr;
    }
    void Unload(ReHandle h) override {
        SceneContext* ctx = (SceneContext*)h;
        UnloadScene(ctx);
    }
    void Clear() override {
        ClearScene();
    }

private:
    bool GetSceneResource(SceneContext* ctx, SceneResource* r, uint32_t id) override {
        if (!lvgl_res_load_pictures_from_scene(&ctx->scene, &id, &r->image, nullptr, 1)) {
            r->type = SceneResource::kImageResource;
            return true;
        }
        return false;
    }
};

// Text loader
class TextLoader : public lvsim::ResourceLoader, public SharedLoader {
public:
    TextLoader(const std::string& name) : ResourceLoader(name), SharedLoader() {}
    virtual ~TextLoader() {
        Clear();
    }
    ReHandle Load(const Attribute& attr) override {
        const char* path = attr.GetValue("src_path");
        if (path == nullptr)
            return nullptr;

        const char* id = attr.GetValue("id");
        if (id == nullptr)
            return nullptr;

        const char* lang = attr.GetValue("lang");
        if (lang != nullptr) {
            if (strncmp(lang, "bt_watch.", 9)) {
                printf("Invalid value for \"lang\" (eg: lang=\"bt_watch.Eng\")");
                lang = nullptr;
            }
        }

        return LoadScene(id, path, lang);
    }
    bool Get(ReHandle h, const std::string& name, Attribute& attr) override {
        SceneContext* ctx = (SceneContext*)h;
        int found;

        SceneResource* r = GetResourceItem(ctx, name, &found);
        if (r != nullptr && !found)
            return attr.RegisterText(name.c_str(), r->text.txt);

        return r != nullptr;
    }
    void Unload(ReHandle h) override {
        SceneContext* ctx = (SceneContext*)h;
        UnloadScene(ctx);
    }
    void Clear() override {
        ClearScene();
    }

private:
    bool GetSceneResource(SceneContext* ctx, SceneResource* r, uint32_t id) override {
        if (!lvgl_res_load_strings_from_scene(&ctx->scene, &id, &r->text, 1)) {
            r->type = SceneResource::kTextResource;
            return true;
        }
        return false;
    }
};

} // namespace


extern "C"
BASE_EXPORT bool LoaderCreate(std::vector<lvsim::ResourceLoader*> &loaders) {
    loaders.push_back(new ImageLoader("scene"));
    loaders.push_back(new TextLoader("string"));
    return true;
}
