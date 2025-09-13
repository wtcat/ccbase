/*
 * Plugin manager
 */

#include "lvgl/src/others/xml/lv_xml_component_private.h"
#include "lvgl/src/others/xml/lv_xml_private.h"
#include "lvgl/src/font/lv_binfont_loader.h"
#include "lvgl/xml_parser_notify.h"

#include <filesystem>

#include "base/file_util.h"
#include "plugin/plugin.h"


namespace lvsim {
namespace fs = std::filesystem;

static void xml_image_parser(lv_xml_parser_state_t* state, const char* type,
    const char** attrs) {
    const char* name = lv_xml_get_value_of(attrs, "name");
    if (name == NULL) {
        LV_LOG_WARN("'name' is missing from a font");
        return;
    }

    const char* src_path = lv_xml_get_value_of(attrs, "src_path");
    if (src_path == NULL) {
        LV_LOG_WARN("'src_path' is missing from a `%s` font", name);
        return;
    }

    /* E.g. <file name="avatar" src_path="avatar1.png">*/
    if (lv_streq(type, "file")) {
        lv_xml_register_image(&state->scope, name, src_path);

    } else if (lv_streq(type, "scene")) {
        const char* scene_id = lv_xml_get_value_of(attrs, "id");
        if (src_path == NULL) {
            LV_LOG_WARN("'scene_id' is missing from a `%s` font", name);
            return;
        }

        lvsim::ResourcePluginManager* rpm = lvsim::ResourcePluginManager::GetInstance();
        ResourceLoader *loader = rpm->FindLoader("scene");
        if (loader != nullptr) {
            ResourceLoader::ReHandle hre = loader->Load(FilePath::FromUTF8Unsafe(src_path), (void *)scene_id);
            if (hre != nullptr) {
                void* image_src = nullptr;
                if (loader->Get(hre, name, &image_src)) {
                    lv_xml_component_scope_t *scope = lv_xml_component_get_scope("globals");
                    lv_xml_register_image(scope, name, image_src);
                }
            }
        }
    }
    else {
        LV_LOG_INFO("Ignore non-file image `%s`", name);
    }
}

static void xml_font_parser(lv_xml_parser_state_t* state, const char* type,
    const char** attrs) {
    const char* name = lv_xml_get_value_of(attrs, "name");
    if (name == NULL) {
        LV_LOG_WARN("'name' is missing from a font");
        return;
    }

    const char* src_path = lv_xml_get_value_of(attrs, "src_path");
    if (src_path == NULL) {
        LV_LOG_WARN("'src_path' is missing from a `%s` font", name);
        return;
    }

    const char* as_file = lv_xml_get_value_of(attrs, "as_file");
    if (as_file == NULL || lv_streq(as_file, "false")) {
        LV_LOG_INFO("Ignore non-file based font `%s`", name);
        return;
    }

    void* ll_ptr;
    LV_LL_READ(&state->scope.font_ll, ll_ptr) {
        lv_xml_font_t* f = (lv_xml_font_t*)ll_ptr;
        if (lv_streq(f->name, name)) {
            LV_LOG_INFO("Font %s is already registered. Don't register it again.", name);
            return;
        }
    }

    /*E.g. <tiny_ttf name="inter_xl" src_path="fonts/Inter-SemiBold.ttf" size="22"/> */
    if (lv_streq(type, "tiny_ttf")) {
        const char* size = lv_xml_get_value_of(attrs, "size");
        if (size == NULL) {
            LV_LOG_WARN("'size' is missing from a `%s` tiny_ttf font", name);
            return;
        }
#if LV_TINY_TTF_FILE_SUPPORT
        lv_font_t* font = lv_tiny_ttf_create_file(src_path, lv_xml_atoi(size));
        if (font == NULL) {
            LV_LOG_WARN("Couldn't load  `%s` tiny_ttf font", name);
            return;
        }
        lv_result_t res = lv_xml_register_font(&state->scope, name, font);
        if (res == LV_RESULT_INVALID) {
            LV_LOG_WARN("Failed to register `%s` tiny_ttf font", name);
            lv_tiny_ttf_destroy(font);
            return;
        }

        /*Get the font which was just created and add a destroy_cb*/
        lv_xml_font_t* new_font;
        LV_LL_READ(&state->scope.font_ll, new_font) {
            if (lv_streq(new_font->name, name)) {
                new_font->font_destroy_cb = lv_tiny_ttf_destroy;
                break;
            }
        }

#else
        LV_LOG_WARN("LV_TINY_TTF_FILE_SUPPORT is not enabled for `%s` font", name);

#endif
    }
    else if (lv_streq(type, "bin")) {
        lv_font_t* font = lv_binfont_create(src_path);
        if (font == NULL) {
            LV_LOG_WARN("Couldn't load `%s` bin font", name);
            return;
        }

        lv_result_t res = lv_xml_register_font(&state->scope, name, font);
        if (res == LV_RESULT_INVALID) {
            LV_LOG_WARN("Failed to register `%s` bin font", name);
            lv_binfont_destroy(font);
            return;
        }

        LV_LL_READ(&state->scope.font_ll, ll_ptr) {
            lv_xml_font_t* new_font = (lv_xml_font_t*)ll_ptr;
            if (lv_streq(new_font->name, name)) {
                new_font->font_destroy_cb = lv_binfont_destroy;
                break;
            }
        }
    }
    else {
        LV_LOG_WARN("`%s` is a not supported font type", type);
    }
}


ResourcePluginManager::ResourcePluginManager() {
    lv_xml_register_image_parser_cb(xml_image_parser);
    lv_xml_register_font_parser_cb(xml_font_parser);
}

ResourcePluginManager::~ResourcePluginManager() {
    lv_xml_register_image_parser_cb(nullptr);
    lv_xml_register_font_parser_cb(nullptr);
    Unload();
}

ResourcePluginManager* ResourcePluginManager::GetInstance() {
    return Singleton<ResourcePluginManager>::get();
}

bool ResourcePluginManager::Load(const FilePath& dir) {
    if (!file_util::PathExists(dir)) {
        printf("Invalid path: %s\n", dir.AsUTF8Unsafe().c_str());
        return false;
    }
 
    const fs::path root_path(dir.MaybeAsASCII());
    for (const auto& entry : fs::directory_iterator(root_path)) {
        /* Don't recursive search */
        if (fs::is_directory(entry))
            continue;

        if (fs::is_regular_file(entry)) {
            auto file_ext = entry.path().filename().extension();
            if (file_ext == ".dll" || file_ext == ".so") {
                scoped_refptr<helper::DynLoader> dyn(new helper::DynLoader);
                if (dyn->Load(FilePath::FromUTF8Unsafe(entry.path().string())))
                    plugins_.push_back(dyn);
            }
        }
    }

    return true;
}

void ResourcePluginManager::Unload() {
    if (loaders_.size() > 0) {
        for (auto iter : loaders_)
            delete iter.second;
        loaders_.clear();
    }
}

void ResourcePluginManager::Reset() {
    for (auto iter : loaders_) {
        auto loader = iter.second;
        loader->Clear();
    }
}

bool ResourcePluginManager::Initialize() {
    for (auto iter : plugins_) {
        LoaderConstructFn fn = (LoaderConstructFn)iter->GetSymbol("LoaderCreate");
        if (fn != nullptr) {
            if (!RegisterLoader(fn())) {
                printf("Invalid plugin: %s\n", iter->name().AsUTF8Unsafe().c_str());
                Unload();
                return false;
            }
        }
    }
    return loaders_.size() > 0;
}

ResourceLoader* ResourcePluginManager::FindLoader(const std::string& name) {
    auto iter = loaders_.find(name);
    if (iter != loaders_.end())
        return iter->second;
    return nullptr;
}

bool ResourcePluginManager::RegisterLoader(ResourceLoader* loader) {
    if (loader == nullptr)
        return false;

    loaders_.insert(std::make_pair(loader->name(), loader));
    return true;
}


} //namespace lvsim