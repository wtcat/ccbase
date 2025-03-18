/*
 * Copyright 2025 wtcat 
 */

#ifndef RESCAN_H_
#define RESCAN_H_

#include <vector>
#include <string>

#include "base/file_path.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"

namespace app {

class ResourceScan : public base::RefCountedThreadSafe<ResourceScan> {
public:
    friend class PictureParser;
    enum {
        LEVEL_RESOURCE = 1,
        LEVEL_VIEW,
        LEVEL_GROUP,
        LEVEL_MAX
    };

    struct Text : public base::RefCountedThreadSafe<Text> {
        Text(const char* s) : text(s), font_height(0) {}
        std::string text;
        int font_height;
    };
    struct Picture : public base::RefCountedThreadSafe<Picture> {
        Picture(const FilePath& fpath) : 
            path(fpath), 
            width(-1), height(-1) {}
        Picture(const std::string& fpath) : 
            path(FilePath::FromUTF8Unsafe(fpath)), 
            width(-1), height(-1) {}
        Picture(const Picture& other) {
            path = other.path;
            width = other.width;
            height = other.height;
        }

        FilePath path;
        int width;
        int height;
    };
    struct PictureGroup: base::RefCountedThreadSafe<PictureGroup> {
        PictureGroup(const std::string& ns) : name(ns) {}
        std::string name;
        std::vector<scoped_refptr<Picture>> pictures;
    };
    struct ViewResource : public base::RefCountedThreadSafe<ViewResource> {
        ViewResource(const std::string &ns): name(ns) {
            pictures.reserve(20);
            strings.reserve(10);
        }
        std::string name;
        std::vector<scoped_refptr<Text>> strings;
        std::vector<scoped_refptr<Picture>> pictures;
        std::vector<scoped_refptr<PictureGroup>> groups;
    };

    ResourceScan(): path_depth_(0) { 
        views_.reserve(10); 
    }
    ~ResourceScan();

    bool ScanResource(const FilePath& dir, bool compatible);
    template<typename Function>
    void ForeachView(base::Callback<Function>& callback) const {
        for (const auto iter : views_)
            callback.Run(iter);
    }
    const FilePath& GetStringFile() const {
        return string_file_;
    }

private:
    struct LineParser {
        LineParser(std::string& text) :
            line_start(nullptr),
            line_end(text.data() - 1),
            end(text.data() + text.size()) {}

        bool ToNextLine() {
            if (line_end >= end)
                return false;

            line_start = ++line_end;
            while (*line_end != '\n' && line_end < end) {
                if (*line_end == ',' || *line_end == ' ' || *line_end == '\t' || *line_end == '\r')
                    *line_end = '\0';
                line_end++;
            }
            return true;
        }

        char* line_start;
        char* line_end;
        char* end;
    };
    bool Scan(const FilePath& dir);
    void ForeachViewPicture(const ViewResource* view,
        base::Callback<void(scoped_refptr<Picture>)>& callback);
    bool CheckPath(const FilePath& dir);
    bool ScanDirectory(const FilePath& dir);
    bool LoadString(const FilePath& file);
    bool IsPicture(const std::string& extname);
    void FixPicture(scoped_refptr<Picture> iter);

private:
    std::vector<scoped_refptr<ViewResource>> views_;
    ViewResource* current_;
    PictureGroup* current_group_;
    FilePath string_file_;
    int path_depth_;
};

} //namespace app

#endif //RESCAN_H_
