/*
 * Copyright 2025 wtcat 
 */

#include "driver/config.h"

#if USE_FILE_WATCHER > 0
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <stdatomic.h>
#include <time.h>
#include <threads.h>

#include <windows.h>

#include "driver/queue.h"
#include "driver/file_watcher.h"


enum watcher_state {
    WATCHER_IDLE,
    WATCHER_ACTIVED,
    WATCHER_REQSTOP,
};

struct watcher_node {
#define MAX_FILEPATH 255
    TAILQ_ENTRY(watcher_node) node;
    char            filepath[MAX_FILEPATH + 1];
    uint64_t        timestamp;
    file_watcher_cb callback;
};

static TAILQ_HEAD(, watcher_node) watcher_list;
static unsigned int watcher_period;
static volatile int watcher_state;
static mtx_t watcher_lock;
static thrd_t watcher_tid;

static bool file_watcher_get_timestamp(const char* file, uint64_t *pts) {
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExA(file, GetFileExInfoStandard, &attr))
        return false;

    uint64_t ts = attr.ftLastWriteTime.dwHighDateTime;
    *pts = (ts << 32) | attr.ftLastWriteTime.dwLowDateTime;
    return true;
}

static void sleep_millisec(unsigned int ms) {
#define NS_PER_SEC (1000000000ul)
    struct timespec ts = {0};
    long sec = ms / 1000;

    ts.tv_sec += sec;
    ts.tv_nsec += (ms - sec * 1000) * 1000000ul;
    if (ts.tv_nsec > NS_PER_SEC) {
        sec = ts.tv_nsec / NS_PER_SEC;
        ts.tv_sec  += sec;
        ts.tv_nsec -= sec * NS_PER_SEC;
    }
    thrd_sleep(&ts, NULL);
}

static int file_watcher_thread(void* arg) {
    struct watcher_node* wnd, * wnd_next;

    while (true) {
        if (watcher_state == WATCHER_REQSTOP)
            break;

        mtx_lock(&watcher_lock);
        TAILQ_FOREACH_SAFE(wnd, &watcher_list, node, wnd_next) {
            uint64_t ts;
            if (file_watcher_get_timestamp(wnd->filepath, &ts)) {
                if (wnd->timestamp != ts) {
                    wnd->timestamp = ts;
                    wnd->callback(wnd->filepath);
                }
            }
        }
        mtx_unlock(&watcher_lock);

        sleep_millisec(watcher_period);
    }

    /*
     * Clean all watcher node  
     */
    mtx_lock(&watcher_lock);
    TAILQ_FOREACH_SAFE(wnd, &watcher_list, node, wnd_next) {
        TAILQ_REMOVE(&watcher_list, wnd, node);
        free(wnd);
    }
    TAILQ_INIT(&watcher_list);
    mtx_unlock(&watcher_lock);

    watcher_state = WATCHER_IDLE;
    thrd_exit(0);

    return 0;
}

static void file_watcher_exit(void) {
    (void) file_watcher_deinit();
}

int file_watcher_add(const char* filepath, file_watcher_cb cb) {
    struct watcher_node* wnd;
    uint64_t ts = 0;

    if (filepath == NULL || cb == NULL)
        return -EINVAL;

    if (watcher_state != WATCHER_ACTIVED)
        return -EBUSY;

    if (!file_watcher_get_timestamp(filepath, &ts)) {
        printf("Invalid(%ld) file path(%s)\n", GetLastError(), filepath);
        return -ESRCH;
    }

    wnd = calloc(1, sizeof(struct watcher_node));
    if (wnd == NULL)
        return -ENOMEM;

    strncpy(wnd->filepath, filepath, MAX_FILEPATH);
    wnd->callback = cb;
    wnd->timestamp = ts;

    mtx_lock(&watcher_lock);
    TAILQ_INSERT_TAIL(&watcher_list, wnd, node);
    mtx_unlock(&watcher_lock);

    return 0;
}

int file_watcher_remove(const char* filepath) {
    struct watcher_node* wnd;

    if (filepath == NULL)
        return -EINVAL;

    if (watcher_state != WATCHER_ACTIVED)
        return -EBUSY;

    mtx_lock(&watcher_lock);
    TAILQ_FOREACH(wnd, &watcher_list, node) {
        if (!strcmp(wnd->filepath, filepath)) {
            TAILQ_REMOVE(&watcher_list, wnd, node);
            free(wnd);
            break;
        }
    }
    mtx_unlock(&watcher_lock);

    return 0;
}

int file_watcher_remove_all(void) {
    struct watcher_node* wnd, *wnd_next;

    if (watcher_state != WATCHER_ACTIVED)
        return -EBUSY;

    mtx_lock(&watcher_lock);
    TAILQ_FOREACH_SAFE(wnd, &watcher_list, node, wnd_next) {
        TAILQ_REMOVE(&watcher_list, wnd, node);
        free(wnd);
    }
    mtx_unlock(&watcher_lock);

    return 0;
}

int file_watcher_init(unsigned int monitor_period) {
    if (watcher_state != WATCHER_IDLE)
        return -EBUSY;

    TAILQ_INIT(&watcher_list);
    mtx_init(&watcher_lock, 0);
    watcher_state = WATCHER_ACTIVED;
    watcher_period = monitor_period;

    int err = thrd_create(&watcher_tid, file_watcher_thread, NULL);
    if (err != thrd_success) {
        printf("Failed(%d) to create file watcher thread\n", err);
        watcher_state = WATCHER_IDLE;
        return err;
    }

    thrd_detach(watcher_tid);
    atexit(file_watcher_exit);
    return 0;
}

int file_watcher_deinit(void) {
    if (watcher_state != WATCHER_ACTIVED)
        return -EINVAL;

    /* Waiting for watcher thread stop */
    watcher_state = WATCHER_REQSTOP;
    while (watcher_state != WATCHER_IDLE) {
        sleep_millisec(100);
    }

    mtx_destroy(&watcher_lock);
    return 0;
}
#endif /* USE_FILE_WATCHER > 0 */
