/*
 * Copyright 2025 wtcat 
 */

#include "lv_string.h"

const char* lv_basename(const char* path)
{
    size_t len = lv_strlen(path);
    if (len == 0) return path;

    len--; /*Go before the trailing '\0'*/

    /*Ignore trailing '/' or '\'*/
    while (path[len] == '/' || path[len] == '\\') {
        if (len > 0)
            len--;
        else
            return path;
    }

    size_t i;
    for (i = len; i > 0; i--) {
        if (path[i] == '/' || path[i] == '\\') break;
    }

    /*No '/' or '\' in the path so return with path itself*/
    if (i == 0) return path;

    return &path[i + 1];
}
