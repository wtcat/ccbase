/*
 * Copyright 2025 wtcat 
 */
#ifndef FILE_WATCHER_H_
#define FILE_WATCHER_H_

#include "driver/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*file_watcher_cb)(const char* file);

#if USE_FILE_WATCHER > 0
int file_watcher_add(const char* filepath, file_watcher_cb cb);
int file_watcher_remove(const char* filepath);
int file_watcher_remove_all(void);
int file_watcher_init(unsigned int monitor_period);
int file_watcher_deinit(void);
#else
# define file_watcher_add(...) 
# define file_watcher_remove(...) 
# define file_watcher_remove_all(...)
# define file_watcher_init(...) 
# define file_watcher_deinit(...) 
#endif /* USE_FILE_WATCHER > 0 */

#ifdef __cplusplus
}
#endif
#endif /* FILE_WATCHER_H_ */
