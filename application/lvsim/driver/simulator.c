/*
 * Copyright 2025 wtcat 
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "simulator.h"
#include "driver/queue.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/drivers/lv_drivers.h"


typedef struct simulator_message {
	TAILQ_ENTRY(simulator_message) node;
	lvgl_message_t message;
} simulator_message_t;

typedef struct {
	TAILQ_HEAD(, simulator_message) pending;
	TAILQ_HEAD(, simulator_message) free;
	SDL_mutex* mutex;
} simulator_context_t;

#define MAX_MESSAGES 20

static simulator_context_t sim_context;
static simulator_message_t msg_buffer[MAX_MESSAGES];

extern lv_indev_t* lv_sdl_mouse_create(void);
extern void lv_sdl_mouse_handler(SDL_Event* event);

static void message_init(void) {
	sim_context.mutex = SDL_CreateMutex();
	assert(sim_context.mutex != NULL);

	TAILQ_INIT(&sim_context.pending);
	TAILQ_INIT(&sim_context.free);
	for (int i = 0; i < MAX_MESSAGES; i++)
		TAILQ_INSERT_TAIL(&sim_context.free, &msg_buffer[i], node);
}

static void sched_message(void) {
	simulator_context_t* ctx = &sim_context;

	if (!TAILQ_EMPTY(&ctx->pending)) {
		SDL_LockMutex(sim_context.mutex);
		if (!TAILQ_EMPTY(&ctx->pending)) {
			simulator_message_t* p = TAILQ_FIRST(&ctx->pending);

			TAILQ_REMOVE(&ctx->pending, p, node);
			if (p->message.routine)
				p->message.routine(&p->message);

			TAILQ_INSERT_HEAD(&ctx->free, p, node);
		}
		SDL_UnlockMutex(sim_context.mutex);
	}
}

static bool key_defaultproc(int code, bool pressed) {
	(void)code;
	(void)pressed;
	return true;
}

int lvgl_send_message(lvgl_message_cb_t cb, uint16_t id, void* user) {
	simulator_context_t* ctx = &sim_context;
	int err = -ENOMEM;

	if (!TAILQ_EMPTY(&ctx->free)) {
		SDL_LockMutex(sim_context.mutex);
		if (!TAILQ_EMPTY(&ctx->free)) {
			simulator_message_t* p = TAILQ_FIRST(&ctx->free);

			TAILQ_REMOVE(&ctx->free, p, node);
			p->message.routine = cb;
			p->message.id = id;
			p->message.user = user;
			TAILQ_INSERT_TAIL(&ctx->pending, p, node);
			err = 0;
		}
		SDL_UnlockMutex(sim_context.mutex);
	}

	return err;
}

int lvgl_runloop(int hor_res, int ver_res,
	void (*ui_bringup)(void),
	bool (*key_action)(int code, bool pressed)) {

	/* Initialize lvgl graphic stack */
	lv_init();
	atexit(lv_deinit);

	/* Initialize SDL display driver */
	lv_sdl_window_create(hor_res, ver_res);

	/* Initialize message for lvgl */
	message_init();

	/* Initialize user extension */
	if (ui_bringup)
		ui_bringup();

	if (key_action == NULL)
		key_action = key_defaultproc;

	/* Register pointer input device */
	lv_sdl_mouse_create();

	for (; ; ) {
		SDL_Event e;

		/* Get input events */
		if (SDL_PollEvent(&e) != 0) {
			/* Process pointer device input event */
			lv_sdl_mouse_handler(&e);

			/* Quit event */
			if (e.type == SDL_QUIT)
				break;

			/* Keypad input event */
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				if (!key_action(e.key.keysym.sym, e.type == SDL_KEYDOWN))
					break;
			}
		}

		/* Process UI message */
		sched_message();

		/* Refresh UI */
		lv_timer_handler();

		SDL_Delay(16);
	}

	return 0;
}
