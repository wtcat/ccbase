/*
 * Copyright 2026 wtcat
 */
#ifndef RESOURCE_FILE_H_
#define RESOURCE_FILE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum refile_pixelformat {
	PIXEL_FORMAT_INVALID,
	PIXEL_FORMAT_INDEXED8, /* indexed8 */
	PIXEL_FORMAT_RGB565,   /* rgb565  */
	PIXEL_FORMAT_ARGB565,  /* argb565 */
	PIXEL_FORMAT_RGB888,   /* rgb888  */
	PIXEL_FORMAT_ARGB888,  /* argb888 */
};

enum refile_comp {
	REFILE_COMPRESS_NONE,
	REFILE_COMPRESS_LZ4,
};

#pragma pack(push)
#pragma pack(1)

#define REFILE_INDEX_SIZE_GROUP_F (0x80000000)
#define REFILE_INDEX_SIZE_MASK    (0x7FFFFFFF)
#define REFILE_INDEX_BASE \
	uint32_t offset; \
	uint32_t size;

struct refile_index {
	uint32_t namekey;
	REFILE_INDEX_BASE
};

struct refile_header {
#define REFILE_FILE_MAGIC "ResFile"
	uint8_t   magic[8];
	uint32_t  version;
	uint32_t  count;
	uint32_t  chksum;
	struct refile_index indexs[];
};

struct refile_data {
	uint32_t width;
	uint32_t height;
	uint32_t size; /* origin size */

#ifndef _CPU_BIG_ENDIAN
	uint32_t format : 6;
	uint32_t compress : 2;
	uint32_t count : 24;
#else
	uint32_t count : 24;
	uint32_t compress : 2;
	uint32_t format : 6;
#endif
	char     data[];
};

struct refile_bindex {
	REFILE_INDEX_BASE
};

struct refile_group {
#define REFILE_GROUP_MAGIC 0xFCFCFCFC
	uint32_t magic;
	uint32_t count;
	uint32_t offset;
	struct refile_bindex indexs[];
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif /* */