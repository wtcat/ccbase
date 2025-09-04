/**
 * @file lv_xml_checkbox_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_checkbox_parser.h"
#if LV_USE_XML

#include "lvgen_cinsn.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_checkbox_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_checkbox_create", "checkbox");
}

void lv_xml_checkbox_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);
    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if (lv_streq("text", name)) {
            //lv_checkbox_set_text(item, value);
            lvgen_new_exprinsn(fn, "lv_checkbox_set_text(%s, %s);",
                LV_OBJNAME(item), value);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


#endif /* LV_USE_XML */
