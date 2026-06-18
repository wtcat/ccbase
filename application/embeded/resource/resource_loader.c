/*
 * Copyright 2026 wtcat 
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "resource_file.h"
#include "resource_loader.h"

//#define RE_LOADER_DISABLE_CHECKER 

static uint32_t re_crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    /* crc table generated from polynomial 0xedb88320 */
    static const uint32_t table[16] = {
        0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
        0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
        0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
    };

    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
        crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
    }
    return ~crc;
}

static int key_compare(const void* a, const void* b) {
    const struct refile_index* pb = (const struct refile_index*)b;
    uint32_t ua = *(uint32_t*)a;
    uint32_t ub = pb->namekey;
    return (ua > ub) - (ua < ub);
}

int re_file_open(const char* file, re_file_t* refile) {
    if (file == NULL || refile == NULL)
        return -EINVAL;

    struct refile_header* re;
    int err;

    // Open resource file
    RE_FILE_OPEN(refile->fd, file, err);
    if (err)
        return err;

    // Get file size
    size_t fsize = 0;
    RE_FILE_SIZE(refile->fd, fsize);
    if (fsize == 0) {
        err = -EINVAL;
        goto _close;
    }

    // Allocate memory for resource file
    re = RE_MALLOC(fsize);
    if (re == NULL) {
        err = -ENOMEM;
        goto _close;
    }

    // Read file content to buffer
    RE_FILE_READ_OFFSET(refile->fd, re, fsize, 0, err);
    if (err)
        goto _free;

    // Check file magic number
    if (memcmp(re->magic, REFILE_FILE_MAGIC, sizeof(REFILE_FILE_MAGIC)) != 0) {
        err = -ENODATA;
        goto _free;
    }

    // Check file CRC
    if (re->chksum != re_crc32_update(0, (uint8_t*)(re + 1), fsize - sizeof(*re))) {
        err = -EINVAL;
        goto _free;
    }

    refile->p = re;
    return 0;

_free:
    RE_FREE(re);
_close:
    RE_FILE_CLOSE(refile->fd);
    return err; 
}

int re_file_close(re_file_t* refile) {
    if (refile == NULL)
        return -EINVAL;

    if (refile->p) {
        RE_FREE(refile->p);
        refile->p = NULL;
    }
    RE_FILE_CLOSE(refile->fd);
    return 0;
}

int re_read_image_dsc(const re_file_t* refile, uint32_t id, re_desc_t* desc) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (refile == NULL || refile->p == NULL)
        return -EINVAL;

    if (desc == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    struct refile_header* re = refile->p;
    struct refile_index* ind;

    ind = bsearch(&id, re->indexs, re->count, sizeof(struct refile_index), key_compare);
    if (ind != NULL) {
        desc->refile = refile;
        desc->offset = ind->offset;
        desc->size = ind->size & REFILE_INDEX_SIZE_MASK;
        return 0;
    }
    return -ENODATA;
}

int re_load_image(const re_file_t* refile, uint32_t id, struct refile_data **img) {
    re_desc_t desc;

    int err = re_read_image_dsc(refile, id, &desc);
    if (err == 0) {
        struct refile_data* d = RE_MALLOC(desc.size);
        if (d == NULL)
            return -ENOMEM;

        RE_FILE_READ_OFFSET(refile->fd, d, desc.size, desc.offset, err);
        if (err) {
            RE_FREE(d);
            return err;
        }
        *img = d;
        return 0;
    }
    return err;
}

int re_unload_image(struct refile_data* img) {
    if (img == NULL)
        return -EINVAL;

    RE_FREE(img);
    return 0;
}

int re_load_group(const re_file_t* refile, uint32_t gid, re_group_t* regroup) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (refile == NULL || refile->p == NULL)
        return -EINVAL;

    if (regroup == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    struct refile_header* re = refile->p;
    struct refile_index* ind;

    ind = bsearch(&gid, re->indexs, re->count, sizeof(struct refile_index), key_compare);
    if (ind != NULL) {
        if (!(ind->size & REFILE_INDEX_SIZE_GROUP_F))
            return -EINVAL;

        size_t size = ind->size & REFILE_INDEX_SIZE_MASK;
        struct refile_group* g = RE_MALLOC(size);
        if (g == NULL)
            return -ENOMEM;

        int err;
        RE_FILE_READ_OFFSET(refile->fd, g, size, ind->offset, err);
        if (err)
            return err;

        if (g->magic != REFILE_GROUP_MAGIC) {
            RE_FREE(g);
            return -EINVAL;
        }

        regroup->refile = refile;
        regroup->p = g;
        return 0;
    }
    return -ENODATA;
}

int re_unload_group(re_group_t* regroup) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (regroup == NULL || regroup->p == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    RE_FREE(regroup->p);
    regroup->p = NULL;
    regroup->refile = NULL;
    return 0;
}

size_t re_group_size(const re_group_t* regroup) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (regroup == NULL || regroup->p == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    struct refile_group* g = regroup->p;
    return g->count;
}

int re_read_group_image_dsc(const re_group_t* regroup, uint32_t idx, re_desc_t *desc) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (regroup == NULL || regroup->p == NULL)
        return -EINVAL;

    if (desc == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    struct refile_group* g = regroup->p;
    if (idx < g->count) {
        struct refile_bindex* gind = g->indexs + idx;
        desc->refile = regroup->refile;
        desc->offset = gind->offset + g->offset;
        desc->size = gind->size;
        return 0;
    }
    return -EINVAL;
}

int re_load_group_image(const re_group_t* regroup, uint32_t idx, struct refile_data** img) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (regroup == NULL || regroup->p == NULL)
        return -EINVAL;

    if (img == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */

    struct refile_group* g = regroup->p;
    if (idx < g->count) {
        struct refile_bindex* gind = g->indexs + idx;
        struct refile_data* d = RE_MALLOC(gind->size);
        if (d == NULL)
            return -ENOMEM;

        const re_file_t* refile = regroup->refile;
        uint32_t offset = g->offset + gind->offset;
        int err;
        RE_FILE_READ_OFFSET(refile->fd, d, gind->size, offset, err);
        if (err) {
            RE_FREE(d);
            return err;
        }
        *img = d;
        return 0;
    }
    return -EINVAL;
}

int re_read_desc(const re_desc_t* desc, void* buf, size_t bsize) {
#ifndef RE_LOADER_DISABLE_CHECKER
    if (desc == NULL || desc->refile == NULL)
        return -EINVAL;

    if (buf == NULL)
        return -EINVAL;
#endif /* RE_LOADER_DISABLE_CHECKER */
    if (desc->size > bsize)
        return -EINVAL;

    int err;
    RE_FILE_READ_OFFSET(desc->refile->fd, buf, desc->size, desc->offset, err);
    return err;
}