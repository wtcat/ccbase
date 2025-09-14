/*
 * Resource Load plugin 
 */
#ifndef LVSIM_PUGIN_PLUGIN_H_
#define LVSIM_PUGIN_PLUGIN_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "base/memory/singleton.h"
#include "application/helper/dyn_loader.h"

namespace lvsim {

class ResourceLoader {
public:
    typedef void* ReHandle;

    class Attribute {
    public:
        Attribute(const char** attr);
        virtual ~Attribute() {
            attrs_ = nullptr;
            scope_ = nullptr;
        }

        virtual const char* GetValue(const char* name) const;
        virtual bool RegisterImage(const char *name, void *data);
        virtual bool RegisterFont(const char* name, void* data);
        virtual bool RegisterText(const char* name, void* data);

    private:
        const char** attrs_;
        void* scope_;
    };

    ResourceLoader(const std::string& name) : name_(name) {}
    virtual ~ResourceLoader() {}
    virtual ReHandle Load(const Attribute &attr) = 0;
    virtual bool Get(ReHandle h, const std::string& name, Attribute& attr) = 0;
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
    typedef bool (*LoaderConstructFn)(std::vector<ResourceLoader*> &loaders);

    ResourcePluginManager();
    ~ResourcePluginManager();

    static ResourcePluginManager* GetInstance();
    bool Load(const FilePath& dir);
    bool Initialize();
    void Unload();
    void Reset();
    ResourceLoader* FindLoader(const std::string& name);
    
private:
    bool RegisterLoader(const std::vector<ResourceLoader*> &loaders);
private:
    DISALLOW_COPY_AND_ASSIGN(ResourcePluginManager);
    friend struct DefaultSingletonTraits<ResourcePluginManager>;

    std::unordered_map<std::string, ResourceLoader*> loaders_;
    std::vector<scoped_refptr<helper::DynLoader>> plugins_;
};

} //namespace lvsim
 

#endif /* LVSIM_PUGIN_PLUGIN_H_ */
