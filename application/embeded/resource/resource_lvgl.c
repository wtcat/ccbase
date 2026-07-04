/*
 * Copyright 2026 wtcat
 */

#include <errno.h>

#include "embeded/resource/resource_file.h"
#include "embeded/resource/resource_lvgl.h"


extern int LZ4_decompress_safe(const char* src, char* dst, int compressedSize, int dstCapacity);

static int image_decompress(const struct lazy_decomp* ld, void* dst, size_t dstsize) {
	struct image_decomp* rd = (struct image_decomp*)ld;
	int ret;

	if (rd->cflag != REFILE_COMPRESS_NONE) {
		size_t cdata_size = rd->csize - sizeof(struct refile_data);
		void* debuf = RE_MALLOC(cdata_size);
		if (debuf == NULL)
			return -ENOMEM;

		ret = re_read_buf(&rd->re, debuf, cdata_size, sizeof(struct refile_data));
		if (ret)
			goto _free;

		switch (rd->cflag) {
		case REFILE_COMPRESS_LZ4:
			ret = LZ4_decompress_safe(debuf, dst, (int)cdata_size, (int)dstsize);
			break;
		default:
			ret = -ENOTSUP;
			break;
		}

	_free:
		RE_FREE(debuf);
		return ret < 0 ? ret : 0;
	}

	return re_read_buf(&rd->re, dst, rd->csize - sizeof(struct refile_data),
		sizeof(struct refile_data));
}

int re_image_prefetch(const void* handle, uint32_t name,
	lv_image_dsc_t* dsc, struct image_decomp* rd, re_read_t read) {
	struct refile_data data;
	int err;

	/* Get image descriptor */
	err = read(handle, name, &rd->re);
	if (err)
		return err;

	/* Get image information */
	err = re_read_buf(&rd->re, &data, sizeof(data), 0);
	if (err)
		return err;

	rd->base.decompress = image_decompress;
	rd->base.offset = rd->re.offset;
	rd->cflag = data.compress;
	rd->csize = rd->re.size;
	rd->dsc = dsc;

	/* Fill LVGL image descriptor */
	dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
	dsc->header.cf = (uint8_t)re_map_colorfmt(data.format);
	dsc->header.w = data.width;
	dsc->header.h = data.height;
	dsc->header.flags = 0;
	dsc->header.stride = 0;
	dsc->header.reserved_2 = LV_IMG_LAZYDECOMP_MARKER;
	dsc->data_size = data.size;
	dsc->data = (void*)rd;
	dsc->reserved = NULL;
	dsc->reserved_2 = NULL;
	return 0;
}

lv_color_format_t re_map_colorfmt(uint32_t fmt) {
	switch (fmt) {
	case PIXEL_FORMAT_INDEXED8:
		return LV_COLOR_FORMAT_I8;
	case PIXEL_FORMAT_RGB565:
		return LV_COLOR_FORMAT_RGB565;
	case PIXEL_FORMAT_ARGB565:
		return LV_COLOR_FORMAT_RGB565A8; /* TODO: approximate */
	case PIXEL_FORMAT_RGB888:
		return LV_COLOR_FORMAT_RGB888;
	case PIXEL_FORMAT_ARGB888:
		return LV_COLOR_FORMAT_ARGB8888;
	default:
		return LV_COLOR_FORMAT_UNKNOWN;
	}
}
