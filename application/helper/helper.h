/*
 * Copyright 2025 wtcat 
 */
#ifndef HELPER_H_
#define HELPER_H_

#include <stddef.h>
#include <filesystem>

class FilePath;

namespace app {

size_t StringToLower(const char* instr, char* outstr, size_t maxsize);
size_t StringToUpper(const char* instr, char* outstr, size_t maxsize);
size_t StringCopy(char* dst, const char* src, size_t dsize);


bool IsSymbolFile(const std::filesystem::path& path);
const std::filesystem::path ReadSymbolPath(const std::filesystem::path& path, const FilePath &base);

} //namespace app

#endif /* HELPER_H_ */
