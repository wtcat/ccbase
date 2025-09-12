// Simple dynamic library loader wrapper
#ifndef HELPER_DYN_LOADER_H_
#define HELPER_DYN_LOADER_H_

#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "base/memory/ref_counted.h"
#include "base/file_path.h"

namespace helper {

class DynLoader: public base::RefCounted<DynLoader> {
public:
#ifdef _WIN32
    typedef HINSTANCE LoaderHandle;
#else
    typedef void* LoaderHandle;
#endif

    DynLoader() : loaded_(false) {};
    DynLoader(const FilePath& path) : loaded_(false) {
        Load(path);
    }
    ~DynLoader() {
        Unload();
    }

    bool Load(const FilePath& path);
    void Unload();
    void* GetSymbol(const std::string& name);
    const FilePath& name() const {
        return path_;
    }

private:
    LoaderHandle handle_;
    FilePath path_;
    bool loaded_;
};


} //namespace helper


#endif //HELPER_DYN_LOADER_H_
