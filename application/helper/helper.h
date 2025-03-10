/*
 * Copyright 2025 wtcat 
 */
#ifndef HELPER_H_
#define HELPER_H_

#include <stddef.h>

namespace app {

size_t StringToLower(const char* instr, char* outstr, size_t maxsize);
size_t StringToUpper(const char* instr, char* outstr, size_t maxsize);

} //namespace app

#endif /* HELPER_H_ */
