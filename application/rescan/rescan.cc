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

bool ResourceScan::ScanResource(const FilePath& dir, bool compatible) {
    FilePath path(dir);

    if (!file_util::AbsolutePath(&path)) {
        DLOG(ERROR) << "Invalid path: " << path.value();
        return false;
    }

    //Find '@new' directory when in compatible mode 
    if (compatible)
        return Scan(path.Append(FilePath::StringType(L"@new")));

    return Scan(path);
}

bool ResourceScan::Scan(const FilePath& path) {
    //Check whether the resource path is valid
    if (!CheckPath(path)) {
        DLOG(ERROR) << "Invalid resource directory!";
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
            if (path_depth_ == LEVEL_RESOURCE) {

                if (entry.path().filename() == "@view")
                    continue;

                //The string database file 
                if (entry.path().filename().extension() == ".xls")
                    string_file_ = FilePath::FromUTF8Unsafe(entry.path().string());

            } else if (path_depth_ >= LEVEL_VIEW) {
                DCHECK(current_ != nullptr);
                std::string path_ext = entry.path().filename().extension().string();
                FilePath filepath = FilePath::FromUTF8Unsafe(entry.path().string());

                if (IsPicture(path_ext)) {
                    if (path_depth_ == LEVEL_VIEW)
                        current_->pictures.push_back(new Picture(filepath));
                    else
                        current_group_->pictures.push_back(new Picture(filepath));
                } else if (entry.path().filename().string() == "@STR.txt") {
                    //Load and parse string from text file
                    LoadString(filepath);
                }
            } else {
                DCHECK(current_ != nullptr);
            }

            DLOG(INFO) << "Regular file: " << entry.path() << std::endl;
        }
    }

    //Restore path depth counter
    --path_depth_;

    return true;
}

bool ResourceScan::LoadString(const FilePath& file) {
    std::string text;

    text.reserve(2048);
    if (!file_util::ReadFileToString(file, &text)) {
        DLOG(ERROR) << "Failed to read string file" << "(" << file.value() << ")\n";
        return false;
    }

    LineParser parser(text);
    Text* ptext = nullptr;
    int line = 0;
    while (parser.ToNextLine()) {
        for (const char* p = parser.line_start; p < parser.line_end;) {
            if (!ptext && *p == 'S') {
                scoped_refptr<Text> text(new Text(p));
                current_->strings.push_back(text);
                ptext = text.get();
                p += ptext->text.size();
            } else if (ptext && isdigit((int)*p)) {
                ptext->font_height = atoi(p);
                ptext = nullptr;
                break;
            } else {
                p++;
            }
        }
        if (ptext && ptext->font_height == 0) {
            DLOG(ERROR) << "Failed to parse line: " << line;
            ptext = nullptr;
        }
        line++;
    }

    return true;
}

void ResourceScan::ForeachViewPicture(const ViewResource* view,
    base::Callback<void(scoped_refptr<Picture>)>& callback) {
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
