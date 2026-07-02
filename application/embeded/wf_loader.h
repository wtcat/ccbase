/*
 * Copyright wtcat 2026. WatchFace Binary (WFB) runtime loader for LVGL 9.
 */
#ifndef WF_LOADER_H
#define WF_LOADER_H

#include <stdint.h>
#include <lvgl.h>

#include "embeded/wfb_format.h"
#include "embeded/resource/resource_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Firmware-provided environment.
 *   get_font  : return a persistent lv_font_t* for a font namekey (required).
 *   get_event : resolve a callback namekey to a function pointer. May be NULL,
 *               in which case the built-in registry (wf_register_event) is used.
 */
typedef struct {
	lv_font_t *(*get_font)(uint32_t namekey);
	wf_event_cb_t (*get_event)(uint32_t cb_hash);
} wf_env_t;

typedef struct wf_instance wf_instance_t;

/*
 * Build the watchface under `screen`. `wfb` must remain valid (read-only) for
 * the lifetime of the instance. `img_res` is an already-opened image ResFile.
 * Returns NULL on any validation/allocation failure.
 */
wf_instance_t *wf_load(const void *wfb, uint32_t wfb_size, const re_file_t *img_res,
					   const wf_env_t *env, lv_obj_t *screen);

/* Delete the created objects and free all resources held by the instance. */
void wf_unload(wf_instance_t *inst);

/* Built-in event registry (used when wf_env_t.get_event is NULL). */
void wf_register_event(const char *name, wf_event_cb_t cb);
wf_event_cb_t wf_lookup_event(uint32_t cb_hash);

lv_color_format_t wf_map_colorfmt(uint32_t fmt);

#ifdef __cplusplus
}
#endif

#endif /* WF_LOADER_H */
