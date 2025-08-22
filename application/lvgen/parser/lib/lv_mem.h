/**
 * @file lv_mem.h
 *
 */

#ifndef LV_MEM_H
#define LV_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_string.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize to use malloc/free/realloc etc
 */
void lv_mem_init(void);

/**
 * Allocate memory dynamically
 * @param size requested size in bytes
 * @return pointer to allocated uninitialized memory, or NULL on failure
 */
void * lv_malloc(size_t size);

/**
 * Allocate a block of zeroed memory dynamically
 * @param num requested number of element to be allocated.
 * @param size requested size of each element in bytes.
 * @return pointer to allocated zeroed memory, or NULL on failure
 */
void * lv_calloc(size_t num, size_t size);

/**
 * Allocate zeroed memory dynamically
 * @param size requested size in bytes
 * @return pointer to allocated zeroed memory, or NULL on failure
 */
void * lv_zalloc(size_t size);

/**
 * Allocate zeroed memory dynamically
 * @param size requested size in bytes
 * @return pointer to allocated zeroed memory, or NULL on failure
 */
void * lv_malloc_zeroed(size_t size);

/**
 * Free an allocated data
 * @param data pointer to an allocated memory
 */
void lv_free(void * data);

/**
 * Reallocate a memory with a new size. The old content will be kept.
 * @param data_p pointer to an allocated memory.
 *               Its content will be copied to the new memory block and freed
 * @param new_size the desired new size in byte
 * @return pointer to the new memory, NULL on failure
 */
void * lv_realloc(void * data_p, size_t new_size);

/**
 * Reallocate a memory with a new size. The old content will be kept.
 * In case of failure, the old pointer is free'd.
 * @param data_p pointer to an allocated memory.
 *               Its content will be copied to the new memory block and freed
 * @param new_size the desired new size in byte
 * @return pointer to the new memory, NULL on failure
 */
void * lv_reallocf(void * data_p, size_t new_size);

/**
 * Used internally to execute a plain `malloc` operation
 * @param size      size in bytes to `malloc`
 */
void * lv_malloc_core(size_t size);

/**
 * Used internally to execute a plain `free` operation
 * @param p      memory address to free
 */
void lv_free_core(void * p);

/**
 * Used internally to execute a plain realloc operation
 * @param p         memory address to realloc
 * @param new_size  size in bytes to realloc
 */
void * lv_realloc_core(void * p, size_t new_size);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_MEM_H*/
