/**
 * @file lv_xml_bar_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_bar_parser.h"
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
static lv_bar_orientation_t orentation_text_to_enum_value(const char * txt);
static lv_bar_mode_t mode_text_to_enum_value(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_bar_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_bar_create", "bar");
}

void lv_xml_bar_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);
    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if(lv_streq("value", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            bool v2 = lv_xml_to_bool(buf_p);
            //lv_bar_set_value(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_bar_set_value(%s, %d, %s);",
                LV_OBJNAME(item), v1, v2 ? "true" : "false");
        }
        if(lv_streq("start_value", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            bool v2 = lv_xml_to_bool(buf_p);
            /*lv_bar_set_start_value*/(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_bar_set_start_value(%s, %d, %s);",
                LV_OBJNAME(item), v1, v2 ? "true" : "false");
        }
        if (lv_streq("orientation", name)) {
            //lv_bar_set_orientation(item, orentation_text_to_enum_value(value));
            lvgen_new_exprinsn(fn, "lv_bar_set_orientation(%s, %s);",
                LV_OBJNAME(item), orentation_text_to_enum_value(value));
        }
        if (lv_streq("mode", name)) {
            //lv_bar_set_mode(item, mode_text_to_enum_value(value));
            lvgen_new_exprinsn(fn, "lv_bar_set_mode(%s, %s);",
                LV_OBJNAME(item), mode_text_to_enum_value(value));
        }
        if(lv_streq("range", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            int32_t v2 = lv_xml_atoi(buf_p);
            //lv_bar_set_range(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_bar_set_range(%s, %d, %d);",
                LV_OBJNAME(item), v1, v2);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_bar_orientation_t orentation_text_to_enum_value(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_bar_orientation_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for bar's orientation", txt);
    return "LV_BAR_ORIENTATION_AUTO"; /*Return 0 in lack of a better option. */
}

static lv_bar_mode_t mode_text_to_enum_value(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_bar_mode_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for bar's mode", txt);
    return "LV_BAR_MODE_NORMAL"; /*Return 0 in lack of a better option. */
}

#endif /* LV_USE_XML */
