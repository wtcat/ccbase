/*
 * Copyright 2025 wtcat 
 */
#ifndef XML_GENERATOR_H_
#define XML_GENERATOR_H_

#include <string>

#include "base/file_path.h"
#include "thirdparty/tinyxml2/tinyxml2.h"

namespace app {

namespace xml = tinyxml2;

//Class UIEditorProject
class UIEditorProject {
public:
    enum { BUFFER_SIZE = 256 };
    UIEditorProject(const ResourceScan* rescan, const FilePath& repath) :
        doc_(),
        rescan_ptr_(rescan),
        resource_path_(repath),
        screen_width_(0),
        screen_height_(0) {}
    ~UIEditorProject() = default;

    void SetSceenSize(int w, int h) {
        screen_width_ = w;
        screen_height_ = h;
    }
    bool GenerateXMLDoc(const std::string& filename);

private:
    void AddProperty(xml::XMLElement* parent, const char* name, const char* value);
    void AddProjectProperty(xml::XMLElement* root);
    void AddScenes(xml::XMLElement* root);
    void AddScene(xml::XMLElement* root, const ResourceScan::ViewResource* view);
    void AddResource(xml::XMLElement* root);
    void AddConfig(xml::XMLElement* root);
    void AddSceneHeader(xml::XMLElement* parent, const char* scene_name);


private:
    xml::XMLDocument doc_;
    const ResourceScan* rescan_ptr_;
    FilePath resource_path_;
    std::string res_file_;
    std::string res_path_;
    int screen_width_;
    int screen_height_;
};


} //namespace app

#endif /* XML_GENERATOR_H_ */
