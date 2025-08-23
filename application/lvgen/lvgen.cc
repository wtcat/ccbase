/*
 * Copyright 2025 wtcat 
 */

#include "lvgen_cinsn.h"

#include <vector>
#include <filesystem>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/memory/singleton.h"
#include "lvgen.h"

namespace app {
namespace fs = std::filesystem;

LvCodeGenerator::~LvCodeGenerator() {
    for (auto iter : lv_props_)
        delete iter.second;
    lvgen_context_destroy();
}

LvCodeGenerator* LvCodeGenerator::GetInstance() {
    return Singleton<LvCodeGenerator>::get();
}

bool LvCodeGenerator::LoadViews(const FilePath& dir) {
    if (!file_util::PathExists(dir)) {
        printf("Invalid path(%s)\n", dir.AsUTF8Unsafe().c_str());
        return false;
    }
    return ScanDirectory(dir, 0);
}

bool LvCodeGenerator::Generate() {
    return lvgen_generate();
}

bool LvCodeGenerator::LoadAttributes(const FilePath& file) {
    // Make sure the file is valid 
    if (!file_util::PathExists(file)) {
        printf("Not found file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Parse doc
    xml::XMLDocument doc;
    if (doc.LoadFile(file.AsUTF8Unsafe().c_str()) != xml::XML_SUCCESS) {
        printf("Failed to parse file(%s)\n", file.AsUTF8Unsafe().c_str());
        return false;
    }

    // Get root element
    xml::XMLElement* root_node = doc.RootElement();

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

    //TODO: remove 
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

bool LvCodeGenerator::ScanDirectory(const FilePath& dir, int level) {
    const fs::path root_path(dir.MaybeAsASCII());
    bool okay = false;
    bool view;

    // Max path recursive deepth must be less than 2
    if (level > 2)
        return true;

    view = (dir.BaseName() != FilePath(L"components"))? true: false;
    level++;

    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (fs::is_directory(entry)) {
            okay = ScanDirectory(FilePath::FromUTF8Unsafe(entry.path().string()), level);
            if (!okay)
                break;
        }

        if (fs::is_regular_file(entry)) {
            if (entry.path().filename().extension() == ".xml") {
                okay = ParseView(entry.path().string(), view);
                if (!okay) {
                    printf("Failed to parse file(%s)\n", entry.path().string().c_str());
                    break;
                }
            }
        }
    }

    level--;
    return okay;
}

bool LvCodeGenerator::ParseView(const std::string& file, bool is_view) {
    return lvgen_parse(file.c_str(), is_view) == 0;
}



} //namespace app

