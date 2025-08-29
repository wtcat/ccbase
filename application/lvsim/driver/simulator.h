/*
 * Copyright 2025 wtcat 
 */

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lvgl_message;
typedef void (*lvgl_message_cb_t)(struct lvgl_message*);

typedef struct lvgl_message {
	uint16_t id;
	uint8_t  reserved[2];
	union {
		void* user;
		uintptr_t value;
	};
	lvgl_message_cb_t routine;
} lvgl_message_t;

int lvgl_runloop(
	int hor_res, 
	int ver_res,
	void (*ui_bringup)(void),
	bool (*key_action)(int code, bool pressed)
);

int lvgl_send_message(
	lvgl_message_cb_t cb, 
	uint16_t id, 
	void* user
);


/*
 * Configure options  
 */
#define USE_FILE_WATCHER 1

#ifdef __cplusplus
}
#endif
#endif /* SIMULATOR_H_ */
