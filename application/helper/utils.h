/*
 * Copyright 2026 wtcat
 */
#ifndef HELPER_UTILS_H_
#define HELPER_UTILS_H_

#include <string.h>
#include <vector>
#include <regex>

#include "base/file_util.h"

class FilePath;

namespace helper {

uint32_t crc32_ieee_update(uint32_t crc, const uint8_t* data, size_t len);
std::vector<std::string> StringSplit(const std::string& s, char delim);
int StrSplit(char* string, int stringlen, char** tokens, int maxtokens,
    char delim);

template<typename Function>
bool FileCollect(const FilePath& dir, bool recursive, const char* regex, Function&& fn) {
    if (!file_util::PathExists(dir)) {
        printf("Invalid path: %s\n", dir.AsUTF8Unsafe().c_str());
        return false;
    }

    file_util::FileEnumerator iterator(dir, recursive, file_util::FileEnumerator::FILES);
    if (regex == nullptr) {
        for (FilePath path = iterator.Next(); path.value().size() > 0;
            path = iterator.Next())
            fn(path);
    } else {
        std::regex pattern(regex);
        for (FilePath path = iterator.Next(); path.value().size() > 0;
            path = iterator.Next()) {
            if (std::regex_match(path.AsUTF8Unsafe(), pattern))
                fn(path);
        }
    }
    return true;
}

}

#endif //HELPER_UTILS_H_
