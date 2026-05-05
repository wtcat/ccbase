/*
 * Copyright 2026 wtcat 
 */


#include <regex>

#include "base/file_path.h"
#include "base/file_util.h"

#include "application/helper/utils.h"


namespace helper {

bool FileCollect(const FilePath& dir, const char* regex, std::vector<FilePath>& out) {
    if (!file_util::PathExists(dir)) {
        printf("Invalid path: %s\n", dir.AsUTF8Unsafe().c_str());
        return false;
    }

    file_util::FileEnumerator iterator(dir, false, file_util::FileEnumerator::FILES);
    if (regex == nullptr) {
        for (FilePath path = iterator.Next(); path.value().size() > 0;
            path = iterator.Next())
            out.push_back(path);
    }
    else {
        std::regex pattern(regex);
        for (FilePath path = iterator.Next(); path.value().size() > 0;
            path = iterator.Next()) {
            if (std::regex_match(path.AsUTF8Unsafe(), pattern))
                out.push_back(path);
        }
    }
    return true;
}

} //helper