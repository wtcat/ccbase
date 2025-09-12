/*
 * Copyright 2025 wtcat
 */

#define BASE_IMPLEMENTATION
#include "base/base_export.h"
#include "lvgl_res_loader.h"

#include "plugin/plugin.h"
#include "base/linked_list.h"


class SceneLoader : public lvsim::ResourceLoader {
public:
    struct SceneContext : base::LinkNode<SceneContext> {
        lvgl_res_scene_t scene;
        uint32_t scene_id;
    };

    SceneLoader(const std::string& name) : ResourceLoader(name) {}
    virtual ~SceneLoader() { Clear(); }

    ReHandle Load(const FilePath& dir, void *ext) override {
        uint32_t scene_id = Hash((const uint8_t *)ext, strlen((char *)ext));
        SceneContext* ctx = SceneFind(scene_id);
        if (ctx == nullptr) {
            FilePath sty = dir.Append(FilePath(L"bt_watch.sty"));
            FilePath res = dir.Append(FilePath(L"bt_watch.res"));
            FilePath str = dir.Append(FilePath(L"bt_watch.str"));
            SceneContext* ctx = SceneAllocate();
            int err = lvgl_res_load_scene(scene_id, &ctx->scene,
                sty.AsUTF8Unsafe().c_str(),
                res.AsUTF8Unsafe().c_str(),
                str.AsUTF8Unsafe().c_str()
            );
            if (err != 0) {
                SceneFree(ctx);
                return nullptr;
            }
        }
        ctx->scene_id = scene_id;
        return ctx;
    }
    bool Get(ReHandle h, const std::string& name, void* data) override {
        SceneContext* ctx = (SceneContext*)h;

        if (IsSceneActived(ctx)) {
            uint32_t id = Hash((const uint8_t*)name.c_str(), (uint32_t)name.size());
            return lvgl_res_load_pictures_from_scene(&ctx->scene, &id, 
                (lv_img_dsc_t*)data, nullptr, 1) == 0;
        }
        return false;
    }
    void Put(ReHandle h, void* p) override {
        (void)h;
        (void)p;
    }
    void Unload(ReHandle h) override {
        SceneContext* ctx = (SceneContext*)h;

        if (IsSceneActived(ctx)) {
            lvgl_res_unload_scene_compact(ctx->scene_id);
            lvgl_res_unload_scene(&ctx->scene);
            SceneFree(ctx);
        }
    }
    void Clear() override {
        for (base::LinkNode<SceneContext>* node = scene_list_.head();
            node != scene_list_.end(); node = node->next()) {
            Unload(node->value());
        }
    }

private:
    SceneContext *SceneFind(uint32_t id) {
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
};

BASE_EXPORT lvsim::ResourceLoader* LoaderCreate(void) {
    return new SceneLoader("scene");
}
