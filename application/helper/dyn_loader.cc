/*
 * Dynamic library loader wrapper 
 */

#include "application/helper/dyn_loader.h"

namespace helper {

bool DynLoader::Load(const FilePath& path) {
    if (loaded_)
        return true;

#ifdef _WIN32
    handle_ = LoadLibrary(path.value().c_str());
#else
    handle_ = dlopen(path.AsUTF8Unsafe().c_str(), RTLD_LAZY);
#endif
    if (handle_ == nullptr)
        return false;
    
    path_ = path;
    loaded_ = true;
    return true;
}

void* DynLoader::GetSymbol(const std::string& name) {
    if (loaded_) {
#ifdef _WIN32
        return (void*)GetProcAddress(handle_, name.c_str());
#else
        return dlsym(handle_, name.c_str());
#endif
    }

    return nullptr;
}

void DynLoader::Unload() {
    if (loaded_) {
#ifdef _WIN32
        FreeLibrary(handle_);
#else
        dlclose(handle_);
#endif
        path_.clear();
        handle_ = nullptr;
        loaded_ = false;
    }
}

} //namespace helper