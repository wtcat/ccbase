/*
 * Copyright 2025 wtcat 
 */

#ifndef LVGEN_H_
#define LVGEN_H_

#include <string>
#include <vector>
#include <unordered_map>
#include "base/memory/ref_counted.h"
#include "thirdparty/tinyxml2/tinyxml2.h"


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

    LvCodeGenerator() = default;
    ~LvCodeGenerator() {
        for (auto iter : lv_props_)
            delete iter.second;
    };

    bool LoadAttributes(const FilePath& file);
    const LvAttribute* FindAttribute(const std::string& ns, const std::string& key);

private:
    xml::XMLElement* FindChild(xml::XMLElement* parent, const char* elem);
    template<typename Func>
    void ForeachChild(xml::XMLElement* parent, Func&& fn) {
        for (xml::XMLElement* node = parent->FirstChildElement();
            node != nullptr;
            node = node->NextSiblingElement()) {
            fn(node);
        }
    }

    // Private data
private:
    xml::XMLDocument doc_;
    std::unordered_map<std::string, LvMap*> lv_props_;
    AttributeMap *sub_props_;
    int sub_prop_count_;
};


} // namespace app

#endif //LVGEN_H_
