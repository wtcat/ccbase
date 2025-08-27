/*
 * Copyright 2025 wtcat 
 */

#ifndef LV_STDIO_H_
#define LV_STDIO_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int lv_snprintf(char* buffer, size_t count, const char* format, ...);
int lv_vsnprintf(char* buffer, size_t count, const char* format, va_list va);

#ifdef __cplusplus
}
#endif
#endif /* LV_STDIO_H_ */