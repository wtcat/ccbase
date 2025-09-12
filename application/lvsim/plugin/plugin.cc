/*
 * Plugin manager
 */

#include <filesystem>

#include "base/file_util.h"
#include "plugin/plugin.h"

namespace lvsim {
namespace fs = std::filesystem;

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
            if (file_ext == "dll" || file_ext == "so") {
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

const ResourceLoader* ResourcePluginManager::FindLoader(const std::string& name) {
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