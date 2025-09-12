/*
 * Copyright 2025 wtcat 
 */
#ifndef RES_CONFIG_H_
#define RES_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>

#include "SDL2/SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Options */
#define CONFIG_SIMULATOR 1
#define CONFIG_RES_MANAGER_SKIP_PRELOAD
#define CONFIG_RES_MANAGER_BLOCK_SIZE 0
#define CONFIG_RES_MANAGER_BLOCK_NUM 0
#define CONFIG_UI_RES_MEM_POOL_SIZE 0

#define MEM_RES 0


/* Mutex */
typedef struct {
	SDL_mutex* mutex;
} os_mutex;

#define os_mutex_init(_l)       (_l)->mutex = SDL_CreateMutex()
#define os_mutex_lock(_l, ...)  SDL_LockMutex((_l)->mutex)
#define os_mutex_unlock(_l)     SDL_UnlockMutex((_l)->mutex)

/* Trace */
#define os_strace_u32x4(...)
#define os_strace_u32(...)
#define os_strace_end_call_u32(...)
#define os_strace_string(...)

/* Log */
#define SYS_LOG_DBG(...)
#define SYS_LOG_INF(...)
#define SYS_LOG_ERR(fmt, ...)                 printf(fmt "\n", ##__VA_ARGS__)

/* Memory */
#define ui_mem_realloc(_t, _p, _s, ...)       realloc(_p, _s)
#define ui_mem_aligned_alloc(_t, _a, _s, ...) ui_mem_alloc(_t, _s)
#define ui_mem_alloc(_t, _s, ...)             malloc(_s)
#define ui_mem_free(_t, _p)                   free(_p)
#define mem_malloc(_s)                        malloc(_s)
#define mem_free(_p)                          free(_p)
#define ui_mem_dump(...)


/* File */
struct fs_file_t {
	FILE* filep;
};

static inline
	int fs_open(struct fs_file_t* zfp, const char* file_name, int flags) {
	zfp->filep = fopen(file_name, "rb");
	if (zfp->filep == NULL) {
		printf("open file %s failed\n", file_name);
		return -1;
	}
	return 0;
}

static inline
	int fs_close(struct fs_file_t* zfp) {
	return fclose(zfp->filep);
}

static inline
	int fs_seek(struct fs_file_t* zfp, uint32_t offset, int whence) {
	return fseek(zfp->filep, offset, whence);
}

static inline
	uint32_t fs_tell(struct fs_file_t* zfp) {
	return ftell(zfp->filep);
}

static inline
	size_t fs_read(struct fs_file_t* zfp, void* ptr, size_t size) {
	return fread(ptr, 1, size, zfp->filep);
}

#ifdef __cplusplus
}
#endif
#endif /* RES_CONFIG_H_ */
