/*
 * Resource Load plugin 
 */
#ifndef LVSIM_PUGIN_PLUGIN_H_
#define LVSIM_PUGIN_PLUGIN_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "application/helper/dyn_loader.h"

namespace lvsim {

class ResourceLoader {
public:
    typedef void* ReHandle;
    ResourceLoader(const std::string& name) : name_(name) {}
    virtual ~ResourceLoader() {}
    virtual ReHandle Load(const FilePath& file, void *ext) = 0;
    virtual bool Get(ReHandle h, const std::string& name, void *data) = 0;
    virtual void Put(ReHandle h, void* p) = 0;
    virtual void Unload(ReHandle h) = 0;
    virtual void Clear() = 0;

    const std::string& name() const {
        return name_;
    }

private:
    std::string name_;
};

class ResourcePluginManager {
public:
    typedef ResourceLoader* (*LoaderConstructFn)();

    ResourcePluginManager() = default;
    ~ResourcePluginManager() {
        Unload();
    }
    bool Load(const FilePath& dir);
    bool Initialize();
    void Unload();
    const ResourceLoader* FindLoader(const std::string& name);

private:
    bool RegisterLoader(ResourceLoader* loader);
private:
    DISALLOW_COPY_AND_ASSIGN(ResourcePluginManager);
    std::unordered_map<std::string, ResourceLoader*> loaders_;
    std::vector<scoped_refptr<helper::DynLoader>> plugins_;
};

} //namespace lvsim
 

#endif /* LVSIM_PUGIN_PLUGIN_H_ */
