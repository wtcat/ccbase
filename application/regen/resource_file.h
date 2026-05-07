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
	PIXEL_FORMAT_RGB888,  /* rgb888  */
	PIXEL_FORMAT_ARGB888, /* Argb888 */
	PIXEL_FORMAT_RGB565,  /* rgb565  */
	PIXEL_FORMAT_ARGB565, /* argb565 */
};

enum refile_comp {
	REFILE_COMPRESS_NONE,
	REFILE_COMPRESS_LZ4,
	REFILE_COMPRESS_USER1,
	REFILE_COMPRESS_USER2,
};

#pragma pack(push)
#pragma pack(1)
struct refile_index {
	uint32_t namekey;
	uint32_t offset;
	uint32_t size;
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
	uint32_t size;   /* Compressed size */
	uint16_t format;
	uint16_t compress;
	char     data[];
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif /* */