/**
 * @file lv_xml.h
 *
 */

#ifndef LV_XML_H
#define LV_XML_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>

#include "parser/lib/lv_types.h"
#include "parser/lib/lv_ll.h"
#include "parser/lib/lv_mem.h"
#include "parser/lib/lv_string.h"

#include "lv_xml_private.h"

#if LV_USE_XML

/*********************
 *      DEFINES
 *********************/
//#define LV_LOG_WARN(fmt, ...) printf("%s:" fmt "\n", __func__, ##__VA_ARGS__)
//#define LV_LOG_ERROR LV_LOG_WARN
//#define LV_LOG_INFO(...)
//#define LV_UNUSED(x) (void)(x)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#if 1
void* lv_xml_default_widget_create(lv_xml_parser_state_t* state, const char** attrs,
    const char* fnname, const char* varname);

void lv_xml_init(void);

void * lv_xml_create(lv_obj_t * parent, const char * name, const char ** attrs);

void * lv_xml_create_in_scope(lv_obj_t * parent, lv_xml_component_scope_t * parent_ctx,
                              lv_xml_component_scope_t * scope,
                              const char ** attrs);

lv_result_t lv_xml_register_font(lv_xml_component_scope_t * scope, const char * name, const lv_font_t * font);

const lv_font_t * lv_xml_get_font(lv_xml_component_scope_t * scope, const char * name);

lv_result_t lv_xml_register_image(lv_xml_component_scope_t * scope, const char * name, const void * src);

const void * lv_xml_get_image(lv_xml_component_scope_t * scope, const char * name);

/**
 * Map globally available subject name to an actual subject variable
 * @param name      name of the subject
 * @param subject   pointer to a subject
 * @return          `LV_RESULT_OK`: success
 */
lv_result_t lv_xml_register_subject(lv_xml_component_scope_t * scope, const char * name, lv_subject_t * subject);

/**
 * Get a subject by name.
 * @param scope     If specified start searching in that component's subject list,
 *                  and if not found search in the global space.
 *                  If `NULL` search in global space immediately.
 * @param name      Name of the subject to find.
 * @return          Pointer to the subject or NULL if not found.
 */
lv_subject_t * lv_xml_get_subject(lv_xml_component_scope_t * scope, const char * name);

int lv_xml_register_const(lv_xml_component_scope_t * scope, const char * name, const char * value);

const char * lv_xml_get_const(lv_xml_component_scope_t * scope, const char * name);

//lv_result_t lv_xml_register_event_cb(lv_xml_component_scope_t * scope, const char * name, lv_event_cb_t cb);
//
//lv_event_cb_t lv_xml_get_event_cb(lv_xml_component_scope_t * scope, const char * name);
#endif //IF 0
/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_XML */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_XML_H*/
