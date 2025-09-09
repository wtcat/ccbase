/**
 * @file lv_xml_obj_parser.h
 *
 */

#ifndef LV_OBJ_XML_PARSER_H
#define LV_OBJ_XML_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "parser/lv_xml.h"
#include "parser/lv_xml_private.h"
#if LV_USE_XML

/**********************
 *      TYPEDEFS
 **********************/
struct func_context;
struct fn_param;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void * lv_xml_obj_create(lv_xml_parser_state_t * state, const char ** attrs);
void lv_xml_obj_apply(lv_xml_parser_state_t * state, const char ** attrs);

struct fn_param* lv_xml_obj_get_parameter(lv_xml_component_scope_t* parent_scope,
    struct func_context* fn, const char* name);
const char* lv_xml_obj_get_value(struct fn_param* param, const char* value);
/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_XML */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_XML_PARSE_H*/
