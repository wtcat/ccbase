/*
 * Copyright 2026 wtcat
 */
#ifndef HELPER_UTILS_H_
#define HELPER_UTILS_H_

#include <vector>

class FilePath;

namespace helper {

bool FileCollect(const FilePath& dir, const char* regex, std::vector<FilePath>& out);

} //helper

#endif //HELPER_UTILS_H_
