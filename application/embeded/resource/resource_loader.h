/*
 * Copyright 2026 wtcat
 */
#ifndef RESOURCE_LOADER_H_
#define RESOURCE_LOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File and allocator operations
 */
#if defined(CONFIG_RESOURCE_LOADER_HFILE)
#include CONFIG_RESOURCE_LOADER_HFILE

#elif defined(_WIN32) || defined(__linux__)
#include <stdio.h>
#include <stdlib.h>

#define RE_FILE_FD(_fd) FILE *_fd;
#define RE_FILE_OPEN(_fd, _name, _err) \
do { \
    _fd = fopen(_name, "rb"); \
    (_err) = ((_fd) == NULL)? -ENODATA: 0; \
} while(0)

#define RE_FILE_SIZE(_fd, _fsize) \
do { \
    fseek(_fd, 0, SEEK_END); \
    (_fsize) = ftell(_fd); \
    rewind(_fd); \
} while (0)
    
#define RE_FILE_READ_OFFSET(_fd, _buffer, _size, _offset, _err) \
do { \
    fseek(_fd, (long)_offset, SEEK_SET); \
    fread(_buffer, 1, _size, _fd); \
    (_err) = 0; \
} while (0)

#define RE_FILE_CLOSE(_fd) fclose(_fd)
#define RE_MALLOC(_size)   malloc((_size))
#define RE_FREE(_ptr)      free((_ptr))

#else
#error "Unsupported operation system"
#endif /* CONFIG_RESOURCE_LOADER_HFILE */

/*
 * Resource loader data structure 
 */
#define RE_MAX_FILENAME 48

struct refile_data;
typedef struct {
    RE_FILE_FD(fd)
    void* p;
    char name[RE_MAX_FILENAME];
} re_file_t;

typedef struct {
    const re_file_t* refile;
    void* p;
} re_group_t;
    
typedef struct {
    const re_file_t* refile;
    uint32_t offset;
    uint32_t size;
} re_desc_t;

/* File open flags */
#define RE_F_FILE_CHECK (0x0001)

/*
 * Public API
 */
#define __RE(x) re_name_hash((x), sizeof((x))-1)
static inline
#if defined(__GNUC__) || defined(__clang__)
__attribute__((const))
#endif
uint32_t re_name_hash(const uint8_t* key, uint32_t len) {
    uint32_t hash = 0;
    for (const uint8_t* end = key + len;
        key < end; key++) {
        hash *= 16777619;
        hash ^= (uint32_t)(*key);
    }
    return hash;
}


int re_file_open(const char* file, unsigned int flags, re_file_t* refile);
int re_file_close(re_file_t* refile);

int re_read_desc(const re_desc_t* desc, void* buf, size_t bsize);

int re_read_image_dsc(const re_file_t* refile, uint32_t id, re_desc_t* desc);
int re_load_image(const re_file_t* refile, uint32_t id, struct refile_data** img);
int re_unload_image(struct refile_data* img);

int re_load_group(const re_file_t* refile, uint32_t gid, re_group_t* regroup);
int re_unload_group(re_group_t* regroup);
size_t re_group_size(const re_group_t* regroup);
int re_read_group_image_dsc(const re_group_t* regroup, uint32_t idx, re_desc_t* desc);
int re_load_group_image(const re_group_t* regroup, uint32_t idx, struct refile_data** img);

#ifdef __cplusplus
}
#endif
#endif /* RESOURCE_LOADER_H_ */