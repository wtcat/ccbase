/*
 * Copyright 2025 wtcat 
 */

#include "lvgen.h"

bool lvgen_cc_find_sym(const char* ns, const char* key, 
    const char **pv, const char **pt) {
    const app::LvCodeGenerator::LvAttribute* attr;
    attr = app::LvCodeGenerator::GetInstance()->FindAttribute(ns, key);
    if (attr != nullptr) {
        if (pv)
            *pv = attr->value.c_str();
        if (pt)
            *pt = attr->value.c_str();
        return true;
    }
    return false;
}