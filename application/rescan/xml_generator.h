/*
 * Copyright 2025 wtcat 
 */
#ifndef XML_GENERATOR_H_
#define XML_GENERATOR_H_

#include <string>

#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "thirdparty/tinyxml2/tinyxml2.h"

namespace app {

namespace xml = tinyxml2;

//Class UIEditorProject
class UIEditorProject : public base::RefCounted<UIEditorProject> {
public:
    enum { 
        BUFFER_SIZE = 256,
        NUM_BUFFER_SIZE = 32
    };
    using StringResource = std::vector<std::string>;
    UIEditorProject(const ResourceScan* rescan, const FilePath& repath);
    ~UIEditorProject() = default;

    void SetSceenSize(int w, int h);
    bool GenerateXMLDoc(const std::string& filename);

private:
    void AddProperty(xml::XMLElement* parent, const char* name, const char* value);
    void AddPictureItem(xml::XMLElement* parent, const char* name, const char* value);
    void AddStringItem(xml::XMLElement* parent, const char* value);
    void AddTextItem(xml::XMLElement* parent, const char* value);
    void AddProjectProperty(xml::XMLElement* root);
    void AddScenes(xml::XMLElement* root);
    void AddScene(xml::XMLElement* root, const ResourceScan::ViewResource* view);
    void AddResource(xml::XMLElement* root);
    void AddConfig(xml::XMLElement* root);
    void AddSceneHeader(xml::XMLElement* parent, const char* scene_name);
    xml::XMLElement* AddSceneResourceGroup(xml::XMLElement* parent);
    void AddScenePicture(xml::XMLElement* parent, const ResourceScan::Picture* picture);
    void AddSceneString(xml::XMLElement* parent, const std::string& str);
    void AddPictureRegion(xml::XMLElement* parent, const ResourceScan::PictureGroup* picgrp);
    void AddPictureHeader(xml::XMLElement* parent, const std::string& name, int width, int height);
    void AddResources(xml::XMLElement* parent);
    void AddGenerateOption(xml::XMLElement* parent);

    void SceneCallback(xml::XMLElement* parent, const scoped_refptr<ResourceScan::ViewResource> view);
    void ResourceCallback(xml::XMLElement* parent, std::vector<const StringResource*> *svec, 
        const scoped_refptr<ResourceScan::ViewResource> view);

    const char* GetRelativePath(const FilePath& path);
    const char* AddCount(int &left, int right);
    const char* ToHexString(int value);

private:
    xml::XMLDocument doc_;
    const ResourceScan* rescan_ptr_;
    FilePath resource_path_;
    std::string resource_abspath_;
    std::string res_file_;
    std::string res_path_;
    std::string rel_path_;
    char screen_width_[8];
    char screen_height_[8];
    int scene_idc_;
    int string_idc_;
    int layer_idc_;

};


} //namespace app

#endif /* XML_GENERATOR_H_ */
