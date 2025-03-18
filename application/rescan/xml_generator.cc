/*
 * Copyright 2025 wtcat 
 */

#include <stdlib.h>
#include <string>
#include <algorithm>

#include "base/logging.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"

#include "application/helper/helper.h"
#include "application/rescan/rescan.h"
#include "application/rescan/xml_generator.h"

#define PICTURE_PERFIX "PIC_"
#define GROUP_PREFIX   "GRP_"
#define SCENE_PREFIX   "SCENE__"

namespace app {
    
static bool filename_number_compare(const FilePath& l,
    const FilePath& r) {
    std::string lstr = l.BaseName().RemoveExtension().AsUTF8Unsafe();
    std::string rstr = r.BaseName().RemoveExtension().AsUTF8Unsafe();
    unsigned long lv = std::strtoul(lstr.c_str(), NULL, 10);
    unsigned long rv = std::strtoul(rstr.c_str(), NULL, 10);
    return lv < rv;
}

UIEditorProject::UIEditorProject(const ResourceScan* rescan, const FilePath& repath) :
    doc_(),
    rescan_ptr_(rescan),
    current_view_(nullptr),
    resource_path_(repath),
    resource_output_path_(L"..\\output"),
    compatible_root_node_(nullptr),
    compatible_lastscene_node_(nullptr),
    compatible_resource_node_(nullptr),
    screen_width_("0x0"),
    screen_height_("0x0"),
    scene_idc_(0x0),
    string_idc_(0x10000),
    compatible_old_(false) {
    FilePath abs_path(repath);

    file_util::AbsolutePath(&abs_path);
    resource_abspath_ = abs_path.AsUTF8Unsafe();
    rel_path_.reserve(1024);
    res_name_.reserve(256);
}

inline const std::string &UIEditorProject::GetResourceName(const char* prefix, 
    const std::string& name) {
    res_name_.clear();
    return res_name_.append(prefix)
                    .append(current_view_->name)
                    .append("_")
                    .append(name);
}

void UIEditorProject::SetResourceOutputPath(const FilePath& opath) {
    resource_output_path_.clear();
    resource_output_path_ = opath;
}

bool UIEditorProject::GenerateXMLDoc(const std::string& filename) {
    if (!compatible()) {
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
        AddResources(root);

        //Add generate options
        AddGenerateOption(root);
    } else {
        //Add new scene
        AddScenes(compatible_root_node_);
        
        //Append resource
        AppendResource(compatible_resource_node_);
    }

    //Save doc
    if (doc_.SaveFile(filename.c_str()) == xml::XML_SUCCESS) {
        DLOG(INFO) << "Generate " << filename << "success\n";
        return true;
    }

    DLOG(ERROR) << "Generate XML failed!\n";
    return false;
}

bool UIEditorProject::SetCompatibleFile(const FilePath& file) {
    if (file_util::PathExists(file)) {
        //Parse UI-Eidtor project file
        if (doc_.LoadFile(file.AsUTF8Unsafe().c_str()) == xml::XML_SUCCESS) {
            compatible_root_node_ = doc_.RootElement();
            if (compatible_root_node_ == nullptr)
                return false;

            //Find resource and config node
            compatible_resource_node_ = FindChild(compatible_root_node_, "resource");
            if (compatible_resource_node_ == nullptr)
                return false;

            //Get last scene node
            compatible_lastscene_node_ = compatible_resource_node_->PreviousSiblingElement("scene");
            if (compatible_lastscene_node_ == nullptr)
                return false;

            compatible_old_ = true;
            return true;
        }
    }
    return false;
}

void UIEditorProject::SetSceenSize(int w, int h) {
    snprintf(screen_width_, sizeof(screen_width_), "0x%x", w);
    snprintf(screen_height_, sizeof(screen_height_), "0x%x", h);
}

void UIEditorProject::AddProperty(xml::XMLElement* parent, const char* name, 
    const char* value) {
    xml::XMLElement* property = doc_.NewElement("property");
    property->SetAttribute("name", name);
    property->SetAttribute("value", value);
    parent->InsertEndChild(property);
}

void UIEditorProject::AddStringItem(xml::XMLElement* parent, const char* value) {
    xml::XMLElement* string = doc_.NewElement("string");
    string->SetAttribute("value", value);
    parent->InsertEndChild(string);
}

void UIEditorProject::AddTextItem(xml::XMLElement* parent, const char* value) {
    xml::XMLElement* text = doc_.NewElement("txt");
    text->SetAttribute("value", value);
    parent->InsertEndChild(text);
}

//Add picture to resource list
void UIEditorProject::AddPictureItem(xml::XMLElement* parent, const char* name, 
    const char* value) {
    xml::XMLElement* picture = doc_.NewElement("picture");
    picture->SetAttribute("value", name);
    if (value != nullptr)
        picture->SetAttribute("LayerID", value);
    if (!compatible())
        parent->InsertEndChild(picture);
    else
        parent->InsertFirstChild(picture);
}

void UIEditorProject::AddProjectProperty(xml::XMLElement* root) {
#define _PATH_PREIFX(x) "..\\output\\" x
    //auto _PATH_PREIFX = [&](const char *path) -> const char* {
    //    return resource_output_path_.Append(FilePath::FromUTF8Unsafe(path))
    //        .AsUTF8Unsafe().c_str();
    //};

    AddProperty(root, "prj_name", "bt_watch");
    AddProperty(root, "prj_language", "0x0");
    AddProperty(root, "prj_screen_width", screen_width_);
    AddProperty(root, "prj_screen_height", screen_height_);
    AddProperty(root, "prj_res_file", _PATH_PREIFX("res\\bt_watch.res"));
    AddProperty(root, "prj_res_str_dir", _PATH_PREIFX("res"));
    AddProperty(root, "prj_res_head_file", _PATH_PREIFX("bt_watch_pic.h"));
    AddProperty(root, "prj_res_str_head_file", _PATH_PREIFX("bt_watch_str.h"));
    AddProperty(root, "prj_sty_head_file", _PATH_PREIFX("bt_watch_sty.h"));
    AddProperty(root, "prj_sty_file", _PATH_PREIFX("res\\bt_watch.sty"));
    AddProperty(root, "prj_build_res_type", "0x1");
    AddProperty(root, "prj_build_res_mode", "0x0");
    
    const char *rel_path = GetRelativePath(FilePath::FromUTF8Unsafe(resource_abspath_));
    AddProperty(root, "prj_case_path", rel_path);
    AddProperty(root, "prj_sdk_path", rel_path);

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
    if (compatible())
        root->InsertAfterChild(compatible_lastscene_node_, scene);
    else
        root->InsertEndChild(scene);

    //Set current view pointer
    current_view_ = view;

    //Add scene header
    char* name = buffer.get();
    strcpy(name, SCENE_PREFIX);
    name += sizeof(SCENE_PREFIX) - 1;
    StringToUpper(view->name.c_str(), name, BUFFER_SIZE);
    name[view->name.size()] = '\0';
    AddSceneHeader(scene, buffer.get());

    //Add resource group
    xml::XMLElement* group = AddSceneResourceGroup(scene);

    //Add picture group
    for (auto grp_iter : view->groups)
        AddPictureRegion(group, grp_iter.get());

    //Add Pictures
    for (auto iter : view->pictures)
        AddScenePicture(group, iter.get());

    //Add strings
    for (const auto &iter : view->strings)
        AddSceneString(group, iter.get());
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
    rescan_ptr_->ForeachView(base::Bind(&UIEditorProject::SceneCallback, this, root));
}

xml::XMLElement* UIEditorProject::AddSceneResourceGroup(xml::XMLElement* parent) {
    xml::XMLElement* rgroup = doc_.NewElement("element");
    rgroup->SetAttribute("class", "resgroup_resource");
    parent->InsertEndChild(rgroup);

    AddProperty(rgroup, "name", "window");
    AddProperty(rgroup, "id", AddCount(scene_idc_, 1));
    AddProperty(rgroup, "x", "0x0000");
    AddProperty(rgroup, "y", "0x0000");
    AddProperty(rgroup, "width", screen_width_);
    AddProperty(rgroup, "height", screen_height_);
    AddProperty(rgroup, "background", "0x00202020");
    AddProperty(rgroup, "opaque", "0x0000");
    AddProperty(rgroup, "transparency", "0x00ff");
    AddProperty(rgroup, "visible", "0x0001");
    AddProperty(rgroup, "editable", "1");

    return rgroup;
}

void UIEditorProject::AddScenePicture(xml::XMLElement* parent, 
    const ResourceScan::Picture* picture) {
    xml::XMLElement* rpic = doc_.NewElement("element");
    rpic->SetAttribute("class", "picture_resource");
    parent->InsertEndChild(rpic);

    //Add picture property
    std::string file_name = picture->path.BaseName().RemoveExtension().AsUTF8Unsafe();
    AddPictureHeader(rpic, GetResourceName(PICTURE_PERFIX, file_name),
        picture->width, picture->height);

    //Add picture path information
    xml::XMLElement* layer = doc_.NewElement("element");
    layer->SetAttribute("class", "layer");
    rpic->InsertEndChild(layer);

    AddProperty(layer, "0", GetRelativePath(picture->path));
}

void UIEditorProject::AddSceneString(xml::XMLElement* parent, const ResourceScan::Text *str) {
    xml::XMLElement* rstr = doc_.NewElement("element");
    rstr->SetAttribute("class", "string_resource");
    parent->InsertEndChild(rstr);

    //Add string property
    AddProperty(rstr, "name", str->text.c_str());
    AddProperty(rstr, "id", AddCount(string_idc_, 1));
    AddProperty(rstr, "x", "0x0000");
    AddProperty(rstr, "y", "0x0000");
    AddProperty(rstr, "width", "0x0020");
    AddProperty(rstr, "height", "0x0020");
    AddProperty(rstr, "foreground", "0x00000000");
    AddProperty(rstr, "background", "0x00ffffff");
    AddProperty(rstr, "visible", "0x0001");
    AddProperty(rstr, "align", "0x000e");
    AddProperty(rstr, "mode", "0x0002");
    AddProperty(rstr, "size", ToHexString((int)str->font_height));
    AddProperty(rstr, "scroll", "0x0");
    AddProperty(rstr, "direction", "-1");
    AddProperty(rstr, "space", "0x0064");
    AddProperty(rstr, "pixel", "0x0001");
    AddProperty(rstr, "strid", str->text.c_str());
}

void UIEditorProject::AddPictureRegion(xml::XMLElement* parent, 
    const ResourceScan::PictureGroup* picgrp) {
    int picture_nums = (int)picgrp->pictures.size();
    if (picture_nums > 0) {
        xml::XMLElement* rgrp = doc_.NewElement("element");
        rgrp->SetAttribute("class", "picregion_resource");
        parent->InsertEndChild(rgrp);

        //Add picture common property
        AddPictureHeader(rgrp, GetResourceName(GROUP_PREFIX, picgrp->name),
            picgrp->pictures[0]->width,
            picgrp->pictures[0]->height);

        //Add layer for picture region
        xml::XMLElement* layer = doc_.NewElement("element");
        layer->SetAttribute("class", "layer");
        rgrp->InsertEndChild(layer);

        //Sort files
        std::vector<FilePath> picture_sorted;
        picture_sorted.reserve(picture_nums);
        for (int i = 0; i < picture_nums; i++)
            picture_sorted.push_back(picgrp->pictures[i]->path);

        std::sort(picture_sorted.begin(),
            picture_sorted.end(),
            filename_number_compare);

        for (int i = 0; i < picture_nums; i++) {
            AddProperty(layer, AddCount(i, 0),
                GetRelativePath(picture_sorted[i]));
        }
    }
}

void UIEditorProject::AddPictureHeader(xml::XMLElement* node, const std::string& name, 
    int width, int height) {
    AddProperty(node, "name", name.c_str());
    AddProperty(node, "id", AddCount(scene_idc_, 1));
    AddProperty(node, "x", "0x0000");
    AddProperty(node, "y", "0x0000");
    AddProperty(node, "width", ToHexString(width));
    AddProperty(node, "height", ToHexString(height));
    AddProperty(node, "visible", "0x0010");
    AddProperty(node, "compress", "0x0000");
    AddProperty(node, "PNG_A8", "0x0000");
    AddProperty(node, "ARGB", "0x0000");
}

void UIEditorProject::AddResources(xml::XMLElement* parent) {
    xml::XMLElement* resource = doc_.NewElement("resource");
    parent->InsertEndChild(resource);

    std::vector<const StringResource *> str_vector;
    str_vector.reserve(50);
    layer_idc_ = 0;

    //Add all picture resource
    rescan_ptr_->ForeachView(
        base::Bind(&UIEditorProject::ResourceCallback, this, resource, &str_vector));

    //Add text file
    AddTextItem(resource, GetRelativePath(rescan_ptr_->GetStringFile()));

    //Add string resource
    for (auto &svec : str_vector) {
        for (auto& iter : *svec)
            AddStringItem(resource, iter->text.c_str());
    }
}

void UIEditorProject::AddGenerateOption(xml::XMLElement* parent) {
    xml::XMLElement* config = doc_.NewElement("config");
    parent->InsertEndChild(config);

    AddProperty(config, "blockHeight", "78");
    AddProperty(config, "blockWidth", "78");
    //AddProperty(config, "cacheID", "yyyyyyyyyyyy");
    AddProperty(config, "compressFormat", "2");
    AddProperty(config, "etc2CompSpeed", "1");
    AddProperty(config, "index_algorithm", "0");
    AddProperty(config, "index_frameDelay", "-1");
    AddProperty(config, "index_isDithering", "0");
    AddProperty(config, "isBlockCompress", "0");
    AddProperty(config, "isCompressRes", "1");
    AddProperty(config, "isDither", "0");
    AddProperty(config, "isPNGARGBToETC2", "1");
    AddProperty(config, "isPNGIndex8Comp", "1");
    AddProperty(config, "isRawJpeg", "1");
    AddProperty(config, "isRebuildETC2Res", "0");
    AddProperty(config, "isSplitPack", "0");
    AddProperty(config, "isSplitRes", "0");
    AddProperty(config, "picResFormat", "0");
    AddProperty(config, "rawJpeg_Quar", "80");
    AddProperty(config, "rawJpeg_blockHeight", "466");
    AddProperty(config, "rawJpeg_blockWidth", "466");
    AddProperty(config, "rawJpeg_minHeight", "160");
    AddProperty(config, "rawJpeg_minWidth", "160");
    AddProperty(config, "rawJpeg_perPixBytes", "2");
    AddProperty(config, "resMode", "0");
    AddProperty(config, "splitPackStart", "0");
    AddProperty(config, "splitResSize", "10");
}

void UIEditorProject::AppendResource(xml::XMLElement* parent) {
    //Prepend picture resource
    //Append string resource
    std::vector<const StringResource*> str_vector;
    str_vector.reserve(50);

    //Calculator first layout ID
    layer_idc_ = 1;
    for (xml::XMLElement* node = parent->FirstChildElement();
        node != nullptr;
        node = node->NextSiblingElement()) {
        if (!strcmp(node->Name(), "txt"))
            break;
        layer_idc_++;
    }

    //Add all picture resource
    rescan_ptr_->ForeachView(
        base::Bind(&UIEditorProject::ResourceCallback, this, parent, &str_vector));

    //Add string resource
    for (auto& svec : str_vector) {
        for (auto& iter : *svec)
            AddStringItem(parent, iter->text.c_str());
    }
}

void UIEditorProject::SceneCallback(xml::XMLElement* parent, 
    const scoped_refptr<ResourceScan::ViewResource> view) {
    AddScene(parent, view.get());
}

void UIEditorProject::ResourceCallback(xml::XMLElement* parent, 
    std::vector<const StringResource*> *svec, 
    const scoped_refptr<ResourceScan::ViewResource> view) {
    ResourceScan::ViewResource* viewp = view.get();

    //Add all pictures to resource list
    for (auto grp_iter : viewp->groups) {
        for (auto iter : grp_iter->pictures) {
            AddPictureItem(parent, GetRelativePath(iter->path), 
                AddCount(layer_idc_, 1));
        }
    }
    for (auto iter : viewp->pictures) {
        AddPictureItem(parent, GetRelativePath(iter->path),
            AddCount(layer_idc_, 1));
    }

    //Collect strings
    svec->push_back(&viewp->strings);
}

const char* UIEditorProject::AddCount(int& left, int right) {
    static char buf[NUM_BUFFER_SIZE];
    left += right;
    return itoa(left, buf, 10);
}

const char* UIEditorProject::ToHexString(int value) {
    static char buf[NUM_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "0x%x", value);
    return buf;
}

const char* UIEditorProject::GetRelativePath(const FilePath& path) {
    rel_path_.clear();
    rel_path_.append(path.AsUTF8Unsafe());

    if (rel_path_.size() == resource_abspath_.size())
        return ".\\";

    char* cpath = rel_path_.data();
    cpath += resource_abspath_.size();
    if (*cpath == '/' || *cpath == '\\')
        *--cpath = '.';
    return cpath;
}

xml::XMLElement* UIEditorProject::FindChild(xml::XMLElement* parent, 
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
