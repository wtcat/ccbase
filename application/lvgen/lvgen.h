/*
 * Copyright 2025 wtcat 
 */

#ifndef LVGEN_H_
#define LVGEN_H_

#include "lvgen_cinsn.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "base/file_path.h"
#include "thirdparty/tinyxml2/tinyxml2.h"

template<typename Type>
struct DefaultSingletonTraits;

namespace app {
namespace xml = tinyxml2;

class LvCodeGenerator {
public:
    struct LvAttribute {
        LvAttribute(const char* n, const char* v, const char *t) :
            name(n), value(v), type(t) {
        }
        std::string name;
        std::string value;
        std::string type;
    };

    using AttributeMap = std::unordered_map<std::string, LvAttribute*>;

    struct LvMap {
        LvMap(const char* n) : name(n) {}
        ~LvMap() {
            for (auto iter : container)
                delete iter.second;
        }
        std::string name;
        AttributeMap container;
    };

    ~LvCodeGenerator();


    static LvCodeGenerator* GetInstance();
    bool LoadAttributes(const FilePath& file);
    const LvAttribute* FindAttribute(const std::string& ns, const std::string& key);
    bool LoadViews(const FilePath& dir);
    bool Generate();

private:
    LvCodeGenerator() = default;

    bool ScanDirectory(const FilePath& dir, int level);
    bool ParseView(const std::string& file, bool is_view);

    xml::XMLElement* FindChild(xml::XMLElement* parent, const char* elem);
    template<typename Func>
    void ForeachChild(xml::XMLElement* parent, Func&& fn) {
        for (xml::XMLElement* node = parent->FirstChildElement();
            node != nullptr;
            node = node->NextSiblingElement()) {
            fn(node);
        }
    }
    friend struct DefaultSingletonTraits<LvCodeGenerator>;

    // Private data
private:
    std::unordered_map<std::string, LvMap*> lv_props_;
};


} // namespace app

#endif //LVGEN_H_
