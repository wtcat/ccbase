/*
 * Copyright 2025 wtcat 
 */

#ifndef RESCAN_H_
#define RESCAN_H_

#include <vector>
#include <string>

#include "base/file_path.h"
#include "base/memory/ref_counted.h"

namespace app {

class ResourceScan {
public:
    enum {
        LEVEL_RESOURCE = 1,
        LEVEL_VIEW,
        LEVEL_GROUP,
        LEVEL_MAX
    };
    struct PictureGroup: base::RefCounted<PictureGroup> {
        PictureGroup(const std::string& ns) : name(ns) {}
        std::string name;
        std::vector<FilePath> pictures;
    };
    struct ViewResource : public base::RefCounted<ViewResource> {
        ViewResource(const std::string &ns): name(ns) {
            pictures.reserve(20);
            strings.reserve(10);
        }
        std::string name;
        std::vector<FilePath> pictures;
        std::vector<std::string> strings;
        std::vector<scoped_refptr<PictureGroup>> groups;
    };

    ResourceScan(): path_depth_(0) { views_.reserve(10); }
    ~ResourceScan() = default;
    bool Scan(const FilePath& dir);

private:
    bool CheckPath(const FilePath& dir);
    bool ScanDirectory(const FilePath& dir);
    bool LoadString(const FilePath& file);
    bool IsPicture(const std::string& extname);


private:
    std::vector<scoped_refptr<ViewResource>> views_;
    ViewResource* current_;
    PictureGroup* current_group_;
    FilePath string_file_;
    int path_depth_;
};

} //namespace app

#endif //RESCAN_H_
