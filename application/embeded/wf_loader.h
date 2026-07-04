/*
 * Copyright wtcat 2026. WatchFace Binary (WFB) runtime loader for LVGL 9.
 */
#ifndef WF_LOADER_H
#define WF_LOADER_H

#include <stdint.h>
#include <stdbool.h>

#include "embeded/wfb_format.h"
#include "embeded/resource/resource_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int tm_sec;   /* seconds after the minute - [0, 60] including leap second */
    int tm_min;   /* minutes after the hour - [0, 59] */
    int tm_hour;  /* hours since midnight - [0, 23] */
    int tm_mday;  /* day of the month - [1, 31] */
    int tm_mon;   /* months since January - [0, 11] */
    int tm_year;  /* years since 1900 */
    int tm_wday;  /* days since Sunday - [0, 6] */
    int tm_yday;  /* days since January 1 - [0, 365] */
    int tm_isdst; /* daylight savings time flag */
} wf_time_t;

/*
 * Firmware-provided environment.
 *   get_font  : return a persistent lv_font_t* for a font namekey (required).
 */
typedef struct {
	void *(*get_font)(uint32_t namekey);
    int  (*get_time)(wf_time_t* tm);
} wf_env_t;

typedef struct wf_instance wf_instance_t;

/*
 * Build the watchface under `screen`. `wfb` must remain valid (read-only) for
 * the lifetime of the instance. `img_res` is an already-opened image ResFile.
 * Returns NULL on any validation/allocation failure.
 */
wf_instance_t *wf_load(const void *wfb, uint32_t wfb_size, const re_file_t *img_res,
					   const wf_env_t *env, void *screen);
void wf_unload(wf_instance_t* inst, bool del_screen);

/* Built-in event registry (used when wf_env_t.get_event is NULL). */
void wf_register_event(const char *name, wf_event_cb_t cb);
wf_event_cb_t wf_lookup_event(uint32_t cb_hash);

#ifdef __cplusplus
}
#endif

#endif /* WF_LOADER_H */
