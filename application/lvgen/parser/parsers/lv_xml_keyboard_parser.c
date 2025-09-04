/**
 * @file lv_xml_keyboard_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_keyboard_parser.h"
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
static lv_keyboard_mode_t mode_text_to_enum_value(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_keyboard_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_keyboard_create", "keyboard");
}

void lv_xml_keyboard_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);

    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        //        if(lv_streq("textarea", name)) lv_keyboard_set_mode(item, lv_obj_get_child_by_name());
        if (lv_streq("mode", name)) {
            //lv_keyboard_set_mode(item, mode_text_to_enum_value(value));
            lvgen_new_exprinsn(fn, "lv_keyboard_set_mode(%s, %s);",
                LV_OBJNAME(item), mode_text_to_enum_value(value));
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_keyboard_mode_t mode_text_to_enum_value(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_keyboard_mode_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for keyboard's mode", txt);
    return "LV_KEYBOARD_MODE_TEXT_UPPER"; /*Return 0 in lack of a better option. */
}

#endif /* LV_USE_XML */
