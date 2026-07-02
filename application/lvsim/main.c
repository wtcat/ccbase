/*
 * Copyright 2025 wtcat 
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver/simulator.h"
#include "thirdparty/lvgl/lvgl.h"
#include "thirdparty/lvgl/src/misc/lv_event.h"
#include "thirdparty/lvgl/src/display/lv_display.h"
#include "thirdparty/lvgl/src/core/lv_obj_private.h"
#undef main

#include "embeded/resource/resource_file.h"
#include "embeded/resource/resource_loader.h"
#include "embeded/wf_loader.h"

static uint8_t static_buffer[10 * 1024 * 1024];
static struct refile_header* res_area = (void *)static_buffer;
static re_file_t re_file;
static re_group_t re_group;

static uint8_t image_buffer_2[11][400 * 1024];
static lv_image_dsc_t image_dsc[11];
static lv_obj_t* img_object;

static void anim_size_cb(void* var, int32_t v) {
    static int index = 0;
    while (index < 11) {
        struct refile_data* const pimg = (struct refile_data*)image_buffer_2[index];
        re_desc_t desc;

        
        //re_load_group_image(&re_group, index, &pimg);
        re_read_group_image_dsc(&re_group, index, &desc);
        re_read_desc(&desc, pimg, sizeof(image_buffer_2[0]));

        image_dsc[index].header.magic = LV_IMAGE_HEADER_MAGIC;
        image_dsc[index].header.cf = LV_COLOR_FORMAT_I8;
        image_dsc[index].header.w = pimg->width;
        image_dsc[index].header.h = pimg->height;
        image_dsc[index].data_size = pimg->size;
        image_dsc[index].data = (uint8_t*)pimg->data;
        index++;
    }

    if (img_object == NULL)
        img_object = lv_image_create(var);

    lv_obj_set_pos(img_object, 0, 210);
    lv_image_set_src(img_object, &image_dsc[v]);
    lv_display_refr_timer(NULL);
}

static uint8_t image_buffer_1[400 * 1024];
static lv_obj_t* ui_lvgen__view_create(lv_obj_t* parent) {
    re_desc_t desc;
    static struct refile_data* pimg = (struct refile_data*)image_buffer_1;
    //re_load_image(&re_file, 0xe6fce000, &pimg);
    re_read_group_image_dsc(&re_group, 0, &desc);
    re_read_desc(&desc, pimg, sizeof(image_buffer_1));
    //re_load_group_image(&re_group, 0, &pimg);

    static lv_image_dsc_t dsc;
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_I8;
    dsc.header.w = pimg->width;
    dsc.header.h = pimg->height;
    dsc.data_size = pimg->size;
    dsc.data = (uint8_t *)(pimg + 1);

    lv_obj_t* img = lv_image_create(parent);
    lv_obj_set_pos(img, 0, 0);
    lv_image_set_src(img, &dsc);

    
    /* Shared animation template */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, parent);
    lv_anim_set_duration(&a, 1000);                    /* forward duration (ms) */
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    /* Animation 1: size 30 -> 70 px, then play back to 30 */
    lv_anim_set_exec_cb(&a, anim_size_cb);
    lv_anim_set_values(&a, 0, 10);
    lv_anim_set_playback_duration(&a, 1000);           /* reverse duration (ms) */
    lv_anim_start(&a);


    return parent;
}

static void* read_file(const char* name, size_t *size) {
    FILE* fp = fopen(name, "rb");
    if (fp == NULL)
        return NULL;

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    void* blob = malloc(fsize);
    if (blob == NULL) {
        fclose(fp);
        return NULL;
    }

    rewind(fp);
    fread(blob, 1, fsize, fp);
    fclose(fp);
    if (size)
        *size = fsize;
    return blob;
}

static void view_init(void) {
    size_t size;
    void* wfb = read_file("IMG/wface.wfb", &size);

    wf_env_t env = {NULL};
    wf_load(wfb, (uint32_t)size, &re_file, &env, lv_screen_active());
}

int main(int argc, char* argv[]) {
    if (re_file_open("IMG/res.bin", RE_F_FILE_CHECK, &re_file))
        return -1;

    re_load_group(&re_file, 0x4a43fec8, &re_group);

	lvgl_runloop(400, 400, view_init, NULL, NULL);
	return 0;
}

