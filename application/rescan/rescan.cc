/*
 * Copyright 2025 wtcat 
 */

#include <filesystem>

#include "base/logging.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/bind.h"
#include "base/threading/simple_thread.h"

#include <opencv2/opencv.hpp>
#include "application/rescan/rescan.h"
#include "application/helper/helper.h"


namespace app {

namespace fs = std::filesystem;

class PictureParser : public base::DelegateSimpleThreadPool::Delegate {
public:
    friend class ResourceScan;
    PictureParser(ResourceScan *cls, ResourceScan::ViewResource* view) :
        view_ptr_(view), cls_(cls) {}

    void Run() override {
        cls_->ForeachViewPicture(view_ptr_, 
            base::Bind(&ResourceScan::FixPicture, cls_));
        delete this;
    }
private:
    ResourceScan::ViewResource* view_ptr_;
    ResourceScan* cls_;
};

//Class ResourceScan
ResourceScan::~ResourceScan() {
    DLOG(INFO) << "Completed\n";
}

inline
bool ResourceScan::IsPicture(const std::string& extname) {
    return extname == ".jpg" || extname == ".png" || extname == ".bmp";
}

bool ResourceScan::ScanResource(const FilePath& dir, const FilePath::StringType &name, bool compatible) {
    FilePath path(dir);

    if (!file_util::AbsolutePath(&path)) {
        printf("Invalid path: %s\n", path.AsUTF8Unsafe().c_str());
        return false;
    }

    //Initialize resource root path(Just only for symbol link)
    resource_rootpath_ = path;

    //Find '@new' directory when in compatible mode 
    if (compatible)
        return Scan(path.Append(name));

    return Scan(path);
}

bool ResourceScan::Scan(const FilePath& path) {
    //Check whether the resource path is valid
    if (!CheckPath(path)) {
        printf("Invalid resource directory(%s)\n", path.AsUTF8Unsafe().c_str());
        return false;
    }

    //Walk around the directory by recursive
    if (!ScanDirectory(path))
        return false;

    //Fix picture parameters
    base::DelegateSimpleThreadPool thread_pool("pictures", 4);
    thread_pool.Start();
    for (auto iter : views_)
        thread_pool.AddWork(new PictureParser(this, iter.get()));
    thread_pool.JoinAll();

    return true;
}

bool ResourceScan::CheckPath(const FilePath& dir) {
    if (!file_util::PathExists(dir)) {
        DLOG(ERROR) << "Path invalid: " << dir.value();
        return false;
    }

    const fs::path root_path(dir.MaybeAsASCII());
    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (fs::is_directory(entry))
            continue;

        if (fs::is_regular_file(entry)) {
            if (entry.path().filename() == "@view")
                return true;
        }
    }

    return false;
}

bool ResourceScan::ScanDirectory(const FilePath& dir) {
    const fs::path root_path(dir.MaybeAsASCII());

    //Limited max path deepth
    if (path_depth_ == LEVEL_MAX)
        return true;

    //Add path depth counter
    ++path_depth_;

    for (const auto& entry : fs::directory_iterator(root_path)) {
        if (fs::is_directory(entry)) {
            if (path_depth_ == LEVEL_RESOURCE) {
                //Create a new view
                scoped_refptr<ViewResource> viewptr = new ViewResource(entry.path().filename().string());
                views_.push_back(viewptr);
                current_ = viewptr.get();
            } else if (path_depth_ == LEVEL_VIEW) {
                //Create a picture group
                scoped_refptr<PictureGroup> group = new PictureGroup(entry.path().filename().string());
                current_->groups.push_back(group);
                current_group_ = group.get();
            }

            //Scan the directory recursive
            if (!ScanDirectory(FilePath::FromUTF8Unsafe(entry.path().string())))
                return false;
     
        } else if (fs::is_regular_file(entry)) {
            fs::path file_realpath;

            // Get real target path if it is a symbol link
            if (IsSymbolFile(entry))
                file_realpath = ReadSymbolPath(entry, resource_rootpath_);
            else
                file_realpath = entry.path();

            if (path_depth_ == LEVEL_RESOURCE) {
                if (file_realpath.filename() == "@view")
                    continue;

                //The string database file 
                if (file_realpath.filename().extension() == ".xls")
                    string_file_ = FilePath::FromUTF8Unsafe(file_realpath.string());

            } else if (path_depth_ >= LEVEL_VIEW) {
                DCHECK(current_ != nullptr);
                std::string path_ext = file_realpath.filename().extension().string();
                FilePath filepath = FilePath::FromUTF8Unsafe(file_realpath.string());

                if (IsPicture(path_ext)) {
                    if (path_depth_ == LEVEL_VIEW)
                        current_->pictures.push_back(new Picture(filepath));
                    else
                        current_group_->pictures.push_back(new Picture(filepath));
                } else if (file_realpath.filename().string() == "@STR.txt") {
                    //Load and parse string from text file
                    ParserString(filepath);
                }
            } else {
                DCHECK(current_ != nullptr);
            }
        }
    }

    //Restore path depth counter
    --path_depth_;

    return true;
}

bool ResourceScan::ParserString(const FilePath& file) {
    enum { BUFFER_SIZE = 1024 };
    TextLineReader text_lines(file);
    scoped_ptr<char> line_buffer(new char[BUFFER_SIZE]);

    for (size_t n = 0; n < text_lines.size(); n++) {
        const char* src = text_lines[n];
        const char *pstr = strstr(src, "STR_");
        bool okay = false;
        if (pstr != nullptr) {
            StringCopy(line_buffer.get(), src, BUFFER_SIZE);
            char* str = line_buffer.get() + (pstr - src);
            char* p = str + 4;
            char* alias = nullptr;

            //Get string name
            while (*p != '\0') {
                if (*p == ',' || isspace(*p)) {
                    *p = '\0';
                    p++;
                    break;
                }
                if (*p == '@') {
                    *p++ = '\0';
                    alias = p;
                    break;
                }
                p++;
            }

            //Get text alias 
            if (alias) {
                char* p_alias = alias;
                while (*p_alias != '\0') {
                    if (*p_alias == ',' || isspace(*p_alias)) {
                        *p_alias++ = '\0';
                        p = p_alias;
                        break;
                    }
                    p_alias++;
                }
            }

            //Get font height
            while (*p != '\0') {
                if (isdigit((int)*p)) {
                    scoped_refptr<Text> text(new Text(str));
                    text.get()->font_height = atoi(p);
                    if (alias != nullptr)
                        text.get()->alias.append(alias);
                    current_->strings.push_back(text);
                    okay = true;
                    break;
                }
                p++;
            }

            if (!okay) {
                printf("Failed to parser file(%s) content(%s)\n", file.AsUTF8Unsafe().c_str(), src);
                return false;
            }
        }
    }

    return true;
}

void ResourceScan::ForeachViewPicture(const ViewResource* view,
    base::Callback<void(scoped_refptr<Picture>)>&& callback) {
    for (auto iter : view->pictures)
        callback.Run(iter);

    for (auto group : view->groups) {
        for (auto iter : group->pictures)
            callback.Run(iter);
    }
}

void ResourceScan::FixPicture(scoped_refptr<Picture> iter) {
    cv::Mat img = cv::imread(iter->path.AsUTF8Unsafe());
    if (!img.empty()) {
        iter->width  = img.cols;
        iter->height = img.rows;
    }
}

} //namespace app
