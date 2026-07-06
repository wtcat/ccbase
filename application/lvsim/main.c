/*
 * Copyright 2025 wtcat 
 */

#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "driver/simulator.h"
#include "thirdparty/lvgl/lvgl.h"
#include "thirdparty/lvgl/src/misc/lv_event.h"
#include "thirdparty/lvgl/src/display/lv_display.h"
#include "thirdparty/lz4/lib/lz4.h"
#undef main

#include "embeded/resource/resource_file.h"
#include "embeded/resource/resource_loader.h"
#include "embeded/decoder/lvgl_lazydecoder.h"
#include "embeded/wf_loader.h"


#define SCREEN_W (466)
#define SCREEN_H (466)
#define SCREEN_BUFSIZE (SCREEN_W * SCREEN_H * (LV_COLOR_DEPTH / 8))


static char decoder_buffer[SCREEN_BUFSIZE * 3];
static wf_instance_t* wf_current;
static void* wf_binary;
static re_file_t re_file;

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


static uint8_t get_image_text(uint32_t provider_hash, char* buf, uint8_t cap) {
    if (provider_hash == __RE("get_hour")) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        uint8_t idx = 0;

        if (idx < cap) buf[idx++] = t->tm_hour / 10 + '0';
        if (idx < cap) buf[idx++] = t->tm_hour % 10 + '0';
        return idx;
    } else if (provider_hash == __RE("get_min")) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        uint8_t idx = 0;

        if (idx < cap) buf[idx++] = t->tm_min / 10 + '0';
        if (idx < cap) buf[idx++] = t->tm_min % 10 + '0';
        return idx;
    } else if (provider_hash == __RE("get_week")) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        uint8_t idx = 0;

        if (idx < cap) buf[idx++] = t->tm_wday+ '0';
        return idx;
    } else if (provider_hash == __RE("get_heartrate")) {
        uint8_t idx = 0;
        if (idx < cap) buf[idx++] = 8 + '0';
        if (idx < cap) buf[idx++] = 6 + '0';
        return idx;
    }

    return 0;
}

static void wf_event_start_anim(struct _lv_event_t* e, uint32_t param) {
    lv_obj_t* target = lv_event_get_current_target(e);
    (void)param;
    lv_animimg_start(target);
}

static void view_init(void *user) {
    (void)user;
    lvgl_lazydecoder_init();
    lazy_cache_init(decoder_buffer, sizeof(decoder_buffer), NULL);

    size_t size;
    void* wfb = read_file("IMG/wface.wfb", &size);
    wf_binary = wfb;

    wf_env_t env = {NULL};
    env.get_text = get_image_text;
    wf_register_event("restart_anim", wf_event_start_anim);
    wf_current = wf_load(wfb, (uint32_t)size, &re_file, &env, lv_screen_active());
}

static void on_exit(void) {
    if (wf_current)
        wf_unload(wf_current, false);
    if (wf_binary)
        free(wf_binary);
    re_file_close(&re_file);
}

int main(int argc, char* argv[]) {
    if (re_file_open("IMG/res.bin", RE_F_FILE_CHECK, &re_file))
        return -1;

#if 0 //String test
    re_file_t temp;
    if (!re_file_open("en.bin", RE_F_FILE_CHECK, &temp)) {
        uint32_t array[] = {
            0x768fc4c1u,
            0x768fc4c2u,
            0x768fc4c8u,
            0x6552bbe6u,
            0x6552bbe4u,
            0x6552bbeau,
            0x6652bd57u,
            0x6652bd55u,
            0x6652bd53u,
            0x6652bd51u,
            0x6652bd5fu,
            0x6752bec9u,
            0x6752becau,
            0
        };
        re_desc_t desc;
        char buf[128];
        for (int i = 0; array[i]; i++) {
            re_read_dsc(&temp, array[i], &desc);
            re_read_buf(&desc, buf, desc.size, 0);
            printf(">%s\n", buf);
        }
        re_file_close(&temp);
    }
#endif

    atexit(on_exit);
	lvgl_runloop(466, 466, view_init, NULL, NULL);
	return 0;
}

