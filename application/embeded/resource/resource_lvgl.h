/*
 * Copyright 2026 wtcat 
 */
#ifndef RESOURCE_LVGL_H_
#define RESOURCE_LVGL_H_

#include <lvgl.h>

#include "embeded/resource/resource_file.h"
#include "embeded/resource/resource_loader.h"
#include "embeded/decoder/lvgl_lazydecomp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*re_read_t)(const void*, uint32_t, re_desc_t*);

struct image_decomp {
	struct lazy_decomp base;
	const lv_image_dsc_t* dsc;
	uint32_t cflag : 2;
	uint32_t csize : 30;
	re_desc_t re;
};

lv_color_format_t re_map_colorfmt(uint32_t fmt);
int re_image_prefetch(const void* handle, uint32_t name,
	lv_image_dsc_t* dsc, struct image_decomp* rd, re_read_t read);

#ifdef __cplusplus
}
#endif
#endif /* RESOURCE_LVGL_H_ */
