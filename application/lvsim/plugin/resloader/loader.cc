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
    struct SceneImage : public base::LinkNode<SceneImage> {
        SceneImage(uint32_t uid) : image(), id(uid) {}
        lv_image_dsc_t image;
        uint32_t id;
    };

    struct SceneContext : public base::LinkNode<SceneContext> {
        SceneImage* NewImage(uint32_t id) {
            SceneImage* img = new SceneImage(id);
            list.Append(img);
            return img;
        }
        void DeleteImage(SceneImage* img) {
            img->RemoveFromList();
            delete img;
        }
        SceneImage* GetImage(uint32_t id) {
            for (base::LinkNode<SceneImage>* node = list.head();
                node != list.end(); node = node->next()) {
                if (node->value()->id == id)
                    return node->value();
            }
            return nullptr;
        }
        void UnloadImages(void) {
            for (base::LinkNode<SceneImage>* node = list.head();
                node != list.end(); node = node->next()) {
                lvgl_res_unload_pictures(&node->value()->image, 1);
            }
        }

        ~SceneContext() {
            base::LinkNode<SceneImage>* node = list.head();
            base::LinkNode<SceneImage>* next;
            
            UnloadImages();
            while (node != list.end()) {
                next = node->next();
                DeleteImage(node->value());
                node = next;
            }
        }

        base::LinkedList<SceneImage> list;
        lvgl_res_scene_t scene;
        uint32_t scene_id;
    };

    SceneLoader(const std::string& name) : ResourceLoader(name) {
        lvgl_res_loader_init(480, 480);
    }
    virtual ~SceneLoader() { 
        Clear(); 
        lvgl_res_loader_deinit();
    }

    ReHandle Load(const FilePath& dir, void *ext) override {
        uint32_t scene_id = Hash((const uint8_t *)ext, strlen((char *)ext));
        SceneContext* ctx = SceneFind(scene_id);
        if (ctx == nullptr) {
            FilePath sty = dir.Append(FilePath(L"bt_watch.sty"));
            FilePath res = dir.Append(FilePath(L"bt_watch.res"));
            FilePath str = dir.Append(FilePath(L"bt_watch.str"));

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
    bool Get(ReHandle h, const std::string& name, void** data) override {
        SceneContext* ctx = (SceneContext*)h;

        if (IsSceneActived(ctx)) {
            uint32_t id = Hash((const uint8_t*)name.c_str(), (uint32_t)name.size());
            SceneImage *img = ctx->GetImage(id);
            if (img != nullptr) {
                *data = &img->image;
                return true;
            }

            img = ctx->NewImage(id);
            int err = lvgl_res_load_pictures_from_scene(&ctx->scene, &id, &img->image, nullptr, 1);
            if (err != 0) {
                ctx->DeleteImage(img);
                return false;
            }

            *data = &img->image;
            return true;
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
            ctx->UnloadImages();
            lvgl_res_unload_scene_compact(ctx->scene_id);
            lvgl_res_unload_scene(&ctx->scene);
            SceneFree(ctx);
        }
    }
    void Clear() override {
        base::LinkNode<SceneContext>* node = scene_list_.head();
        base::LinkNode<SceneContext>* next;
        while (node != scene_list_.end()) {
            next = node->next();
            Unload(node->value());
            node = next;
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

extern "C"
BASE_EXPORT lvsim::ResourceLoader* LoaderCreate(void) {
    return new SceneLoader("scene");
}
