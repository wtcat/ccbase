/*
 * Copyright 2025 wtcat 
 */

#include <stdlib.h>
#include <string>


#include "base/logging.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"

#include "application/helper/helper.h"
#include "application/rescan/rescan.h"
#include "application/rescan/xml_generator.h"

namespace app {

static void SceneCallback(const scoped_refptr<ResourceScan::ViewResource> view, 
    xml::XMLElement* parent) {

}

//Class UIEditorProject
bool UIEditorProject::GenerateXMLDoc(const std::string& filename) {
    //Create XML declare
    xml::XMLDeclaration* declare = doc_.NewDeclaration(
        "xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"");
    doc_.InsertFirstChild(declare);

    //Create root node
    xml::XMLElement* root = doc_.NewElement("ui-rad");
    doc_.InsertEndChild(root);

    //Add global property 
    AddProjectProperty(root);

    //Add scenes
    AddScenes(root);

    //Add resource list

    //Add generate parameters

    //Save doc
    if (doc_.SaveFile(filename.c_str()) == xml::XML_SUCCESS) {
        DLOG(INFO) << "Generate " << filename << "success\n";
        return true;
    }

    DLOG(ERROR) << "Generate XML failed!\n";
    return false;
}

void UIEditorProject::AddProperty(xml::XMLElement* parent, const char* name, 
    const char* value) {
    xml::XMLElement* property = doc_.NewElement("property");
    property->SetAttribute("name", name);
    property->SetAttribute("value", value);
    parent->InsertEndChild(property);
}

void UIEditorProject::AddProjectProperty(xml::XMLElement* root) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);

    AddProperty(root, "prj_name", "bt_watch");
    AddProperty(root, "prj_language", "0x0");

    snprintf(buffer.get(), BUFFER_SIZE,
        "0x%x", screen_width_);
    AddProperty(root, "prj_screen_width", buffer.get());

    snprintf(buffer.get(), BUFFER_SIZE,
        "0x%x", screen_height_);
    AddProperty(root, "prj_screen_height", buffer.get());

    AddProperty(root, "prj_res_file", ".\\res\\bt_watch.res");
    AddProperty(root, "prj_res_str_dir", ".\\res");
    AddProperty(root, "prj_res_head_file", ".\\bt_watch_pic.h");
    AddProperty(root, "prj_res_str_head_file", ".\\bt_watch_str.h");
    AddProperty(root, "prj_sty_head_file", ".\\bt_watch_sty.h");
    AddProperty(root, "prj_sty_file", ".\\res\\bt_watch.sty");
    AddProperty(root, "prj_build_res_type", "0x1");
    AddProperty(root, "prj_build_res_mode", "0x0");
    
    std::string rpath(resource_path_.AsUTF8Unsafe());
    AddProperty(root, "prj_case_path", rpath.c_str());
    AddProperty(root, "prj_sdk_path", rpath.c_str());

    AddProperty(root, "desktop_file_create", "0x0");
    AddProperty(root, "desktop_file_path", "");
    AddProperty(root, "desktop_file_comment", "");
    AddProperty(root, "desktop_file_exec_dir", "/mnt/diska/apps/");
    AddProperty(root, "desktop_file_type", "0x0");
    AddProperty(root, "desktop_file_child", "");
    AddProperty(root, "desktop_file_exec", "Application.app");
    AddProperty(root, "desktop_file_icon_id", "0x0");
    AddProperty(root, "desktop_file_icon", "0x0");
    AddProperty(root, "desktop_file_name_id", "0x0");
    AddProperty(root, "desktop_file_name", "Application");
    AddProperty(root, "desktop_file_name_str_id", "");
}

void UIEditorProject::AddConfig(xml::XMLElement* root) {

}

void UIEditorProject::AddResource(xml::XMLElement* root) {

}

void UIEditorProject::AddScene(xml::XMLElement* root, const ResourceScan::ViewResource* view) {
    scoped_ptr<char> buffer(new char[BUFFER_SIZE]);
    //Create scene node
    xml::XMLElement* scene = doc_.NewElement("scene");
    doc_.InsertEndChild(root);

    //Add scene header
    StringToUpper(view->name.c_str(), buffer.get(), BUFFER_SIZE);
    AddSceneHeader(scene, buffer.get());


}

void UIEditorProject::AddSceneHeader(xml::XMLElement* parent, const char* scene_name) {
    AddProperty(parent, "name", scene_name);
    AddProperty(parent, "id", "0");
    AddProperty(parent, "direction", "0");
    for (int i = 1; i <= 16; i++) {
        char keybuf[8], valbuf[8];

        snprintf(keybuf, sizeof(keybuf), "key%d", i);
        AddProperty(parent, keybuf, itoa(i, valbuf, 10));
    }
}

void UIEditorProject::AddScenes(xml::XMLElement* root) {
    rescan_ptr_->ForeachView(base::Bind(&SceneCallback), root);
}
} //namespace app
