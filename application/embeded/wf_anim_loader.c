/*
 * Copyright 2023 wtcat
 */
#include <lvgl.h>

#include "embeded/resource/resource_file.h"
#include "embeded/resource/resource_loader.h"
#include "embeded/widget/lvgl_animation.h"
#include "embeded/widget/lvgl_img_loader.h"


#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, mem) (type *)((char *)ptr - (char *)offsetof(type, mem))
#endif

static int anim_image_load(struct lvgl_img_loader *loader, uint16_t index, lv_img_dsc_t *dsc) {
	re_group_t *group = loader->user_data;
	re_desc_t rdsc;
	int err;

	err = re_read_group_image_dsc(group, index, &rdsc);
	if (!err) {
		struct refile_data d;
		err = re_read_buf(&rdsc, &d, sizeof(d), 0);
		if (!err) {
			dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
			dsc->header.cf = (uint8_t)wf_map_colorfmt(d.format);
			dsc->header.w = (uint16_t)d.width;
			dsc->header.h = (uint16_t)d.height;
			dsc->data_size = d.size;
			//TODO:
			dsc->data = NULL;
		}
	}

	return err;
}

static void anim_image_unload(struct lvgl_img_loader *loader, uint16_t index,
						   lv_img_dsc_t *dsc) {
	(void) loader;
	(void) index;
	(void) dsc;
}

static void anim_image_preload(struct lvgl_img_loader *loader, uint16_t index) {
	(void)loader;
	(void)index;
}

static uint16_t anim_image_get_count(struct lvgl_img_loader *loader) {
	re_group_t* group = loader->user_data;
	return (uint16_t)re_group_size(group);
}

static void anim_image_destroy(struct lvgl_img_loader * loader) {
	(void)loader;
}

const lvgl_img_loader_ops_t _wf_anim_loader = {
	.load = anim_image_load,
	.unload = anim_image_unload,
	.preload = anim_image_preload,
	.get_count = anim_image_get_count,
	.destroy = anim_image_destroy
};
