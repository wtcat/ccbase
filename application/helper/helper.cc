/*
 * Copyright 2025 wtcat 
 */

#include <ctype.h>

#include "application/helper/helper.h"

namespace app {

size_t StringToLower(const char* instr, char* outstr, size_t maxsize) {
    if (instr == nullptr || outstr == nullptr)
        return 0;

    const char* s = instr;
    while (*s != '\0' && maxsize > 1) {
        *outstr++ = tolower(*s);
        s++;
        maxsize--;
    }

    return (size_t)(s - instr);
}

size_t StringToUpper(const char* instr, char* outstr, size_t maxsize) {
    if (instr == nullptr || outstr == nullptr)
        return 0;

    const char* s = instr;
    while (*s != '\0' && maxsize > 1) {
        *outstr++ = toupper(*s);
        s++;
        maxsize--;
    }

    return (size_t)(s - instr);
}

} //namespace app
