/*
 * Copyright 2025 wtcat 
 */

#include <vector>

#include "base/file_path.h"
#include "base/file_util.h"
#include "lvgen.h"

namespace app {

bool LvCodeGenerator::LoadAttributes(const FilePath& file) {
    // Make sure the file is valid 
    if (!file_util::PathExists(file)) {
        printf("Not found file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Parse doc
    if (doc_.LoadFile(file.AsUTF8Unsafe().c_str()) != xml::XML_SUCCESS) {
        printf("Failed to parse file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Get root element
    xml::XMLElement* root_node = doc_.RootElement();

    // Parse all types
    std::vector<const char*> type_vector;
    type_vector.reserve(15);

    xml::XMLElement* type_nodes = FindChild(root_node, "types");
    ForeachChild(type_nodes, [&](xml::XMLElement* e) {
        type_vector.push_back(e->Name());
    });

    for (auto iter : type_vector) {
        LvMap* lvmap = new LvMap(iter);
        xml::XMLElement* child = FindChild(type_nodes, iter);
        ForeachChild(child, [&](xml::XMLElement* e) {
            LvAttribute *attr = new LvAttribute(e->Attribute("name"), e->Attribute("value"), "?");
            lvmap->container.insert(std::make_pair(attr->name, attr));
        });

        lv_props_.insert(std::make_pair(lvmap->name, lvmap));
    }

    // Parse styles
    xml::XMLElement* styles_node = FindChild(root_node, "styles");
    LvMap* lvmap = new LvMap("styles");
    ForeachChild(styles_node, [&](xml::XMLElement* e) {
        LvAttribute* attr = new LvAttribute(e->Attribute("name"), e->Attribute("value"), e->Attribute("type"));
        lvmap->container.insert(std::make_pair(attr->name, attr));
        });
    lv_props_.insert(std::make_pair(lvmap->name, lvmap));

    const LvAttribute* p = FindAttribute("lv_border_side_t", "bottom");
    if (p != nullptr) {
        printf("%s\n", p->value.c_str());
    }
    
    return true;
}

const LvCodeGenerator::LvAttribute* LvCodeGenerator::FindAttribute(const std::string& ns,
    const std::string& key) {
    auto ns_iter = lv_props_.find(ns);
    if (ns_iter != lv_props_.end()) {
        LvMap* lvmap = ns_iter->second;
        auto key_iter = lvmap->container.find(key);
        if (key_iter != lvmap->container.end())
            return key_iter->second;
    }
    return nullptr;
}

xml::XMLElement* LvCodeGenerator::FindChild(xml::XMLElement* parent,
    const char* elem) {
    for (xml::XMLElement* node = parent->FirstChildElement();
        node != nullptr;
        node = node->NextSiblingElement()) {
        if (!strcmp(node->Name(), elem))
            return node;
    }
    return nullptr;
}


















} //namespace app

