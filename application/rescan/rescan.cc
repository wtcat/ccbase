/*
 * Copyright 2025 wtcat 
 */

#include <filesystem>

#include "base/logging.h"
#include "base/file_util.h"

#include "application/rescan/rescan.h"

#define ERROR_PREFIX "Error: "

namespace app {

namespace fs = std::filesystem;

//Class ResourceScan
inline
bool ResourceScan::IsPicture(const std::string& extname) {
    return extname == ".jpg" || extname == ".png" || extname == ".bmp";
}

bool ResourceScan::Scan(const FilePath& dir) {
    //Check whether the resource path is valid
    if (!CheckPath(dir)) {
        DLOG(ERROR) << "Invalid resource directory!";
        return false;
    }

    //Walk around the directory by recursive
    return ScanDirectory(dir);
}

bool ResourceScan::CheckPath(const FilePath& dir) {
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
                scoped_refptr<PictureGroup> group = new PictureGroup(entry.path().filename().string());
                current_->groups.push_back(group);
                current_group_ = group.get();
            }

            if (!ScanDirectory(FilePath::FromUTF8Unsafe(entry.path().string())))
                return false;
     
        } else if (fs::is_regular_file(entry)) {
            if (path_depth_ == LEVEL_RESOURCE) {
                if (entry.path().filename() == "@view")
                    continue;
                if (entry.path().filename().extension() == ".xls")
                    string_file_ = FilePath::FromUTF8Unsafe(entry.path().string());
            } else if (path_depth_ >= LEVEL_VIEW) {
                DCHECK(current_ != nullptr);
                std::string path_ext = entry.path().filename().extension().string();
                FilePath filepath = FilePath::FromUTF8Unsafe(entry.path().string());

                if (IsPicture(path_ext)) {
                    if (path_depth_ == LEVEL_VIEW)
                        current_->pictures.push_back(filepath);
                    else
                        current_group_->pictures.push_back(filepath);
                } else if (entry.path().filename().string() == "@STR.txt") {
                    //Load string from text file
                    LoadString(filepath);
                }
            } else {
                DCHECK(current_ != nullptr);

            }

            DLOG(ERROR) << "Regular file: " << entry.path() << std::endl;
        }
    }

    //Restore path depth counter
    --path_depth_;

    return true;
}

bool ResourceScan::LoadString(const FilePath& file) {
    std::string text;

    if (!file_util::ReadFileToString(file, &text)) {
        DLOG(ERROR) << "Failed to read string file" << "(" << file.value() << ")\n";
        return false;
    }

    //Parse string ID
    char* buf  = text.data();
    size_t len = text.size();

    //Insert terminal char
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == ',' || buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r')
            buf[i] = '\0';
    }

    for (size_t i = 0; i < len;) {
        if (buf[i]) {
            if (buf[i] == 'S') {
                std::string str(&buf[i]);
                current_->strings.push_back(str);
                i += str.size();
            } else {
                i++;
            }
            continue;
        }
        i++;
    }

    return true;
}

} //namespace app
