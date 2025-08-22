/**
 * @file lv_mem.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "lv_string.h"
#include "lv_mem.h"


/*********************
 *      DEFINES
 *********************/
/*memset the allocated memories to 0xaa and freed memories to 0xbb (just for testing purposes)*/
#ifndef LV_MEM_ADD_JUNK
    #define LV_MEM_ADD_JUNK  0
#endif

static unsigned int zero_mem;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
void * lv_malloc_core(size_t size);
void * lv_realloc_core(void * p, size_t new_size);
void lv_free_core(void * p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/
#if LV_USE_LOG && LV_LOG_TRACE_MEM
    #define LV_TRACE_MEM(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define LV_TRACE_MEM(...)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_malloc(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }

    void * alloc = lv_malloc_core(size);

    if(alloc == NULL) {
        printf("couldn't allocate memory (%lu bytes)\n", (unsigned long)size);
        return NULL;
    }

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void * lv_malloc_zeroed(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }

    void * alloc = lv_malloc_core(size);
    if(alloc == NULL) {
        printf("couldn't allocate memory (%lu bytes)", (unsigned long)size);
        return NULL;
    }

    lv_memzero(alloc, size);

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void * lv_calloc(size_t num, size_t size)
{
    LV_TRACE_MEM("allocating number of %zu each %zu bytes", num, size);
    return lv_malloc_zeroed(num * size);
}

void * lv_zalloc(size_t size)
{
    return lv_malloc_zeroed(size);
}

void lv_free(void * data)
{
    LV_TRACE_MEM("freeing %p", data);
    if(data == &zero_mem) return;
    if(data == NULL) return;

    lv_free_core(data);
}

void * lv_reallocf(void * data_p, size_t new_size)
{
    void * new = lv_realloc(data_p, new_size);
    if(!new) {
        lv_free(data_p);
    }
    return new;
}

void * lv_realloc(void * data_p, size_t new_size)
{
    LV_TRACE_MEM("reallocating %p with %lu size", data_p, (unsigned long)new_size);
    if(new_size == 0) {
        LV_TRACE_MEM("using zero_mem");
        lv_free(data_p);
        return &zero_mem;
    }

    if(data_p == &zero_mem) return lv_malloc(new_size);

    void * new_p = lv_realloc_core(data_p, new_size);

    if(new_p == NULL) {
        printf("couldn't reallocate memory\n");
        return NULL;
    }

    LV_TRACE_MEM("reallocated at %p", new_p);
    return new_p;
}
