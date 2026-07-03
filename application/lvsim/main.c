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

static void view_init(void) {
    size_t size;
    void* wfb = read_file("IMG/wface.wfb", &size);

    wf_env_t env = {NULL};
    wf_binary = wfb;
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

    atexit(on_exit);
	lvgl_runloop(466, 466, view_init, NULL, NULL);
	return 0;
}

