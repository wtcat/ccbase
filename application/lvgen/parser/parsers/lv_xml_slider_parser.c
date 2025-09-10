/**
 * @file lv_xml_slider_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_slider_parser.h"
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
static lv_slider_orientation_t orentation_text_to_enum_value(const char * txt);
static lv_slider_mode_t mode_text_to_enum_value(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_slider_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_slider_create", "slider");
}

void lv_xml_slider_apply(lv_xml_parser_state_t * state, const char ** attrs)
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
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //bool v2 = lv_xml_to_bool(buf_p);
            //lv_bar_set_value(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_bar_set_value(%s, %d, %s);",
                LV_OBJNAME(item),
                lv_xml_atoi(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_to_bool_string(buf_p)
            );
        }
#if 0
        if(lv_streq("bind_value", name)) {
            lv_subject_t * subject = lv_xml_get_subject(&state->scope, value);
            if(subject) {
                lv_slider_bind_value(item, subject);
            }
            else {
                LV_LOG_WARN("Subject \"%s\" doesn't exist in slider bind_value", value);
            }
        }
#endif /* if 0 */

        if(lv_streq("start_value", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //bool v2 = lv_xml_to_bool(buf_p);
            //lv_bar_set_start_value(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_bar_set_start_value(%s, %d, %s);",
                LV_OBJNAME(item),
                lv_xml_atoi(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_to_bool_string(buf_p)
            );
        }
        else if (lv_streq("orientation", name)) {
            //lv_slider_set_orientation(item, orentation_text_to_enum_value(value));
            lvgen_new_exprinsn(fn, "lv_slider_set_orientation(%s, %s);",
                LV_OBJNAME(item),
                orentation_text_to_enum_value(value)
            );
        }
        else if (lv_streq("mode", name)) {
            //lv_slider_set_mode(item, mode_text_to_enum_value(value));
            lvgen_new_exprinsn(fn, "lv_slider_set_mode(%s, %s);",
                LV_OBJNAME(item),
                mode_text_to_enum_value(value)
            );
        }
        else if (lv_streq("range_min", name)) {
            //lv_slider_set_range(item, lv_xml_atoi(value), lv_slider_get_max_value(item));
            lvgen_new_exprinsn(fn, "lv_slider_set_range(%s, %d, lv_slider_get_max_value(%s));",
                LV_OBJNAME(item),
                lv_xml_atoi(value),
                LV_OBJNAME(item)
            );
        }
        else if (lv_streq("range_max", name)) {
            //lv_slider_set_range(item, lv_slider_get_min_value(item), lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_slider_set_range(%s, lv_slider_get_min_value(%s), %d);",
                LV_OBJNAME(item),
                LV_OBJNAME(item),
                lv_xml_atoi(value)
            );
        }
        else if (lv_streq("range", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //int32_t v2 = lv_xml_atoi(buf_p);
            //lv_slider_set_range(item, v1, v2);
            lvgen_new_exprinsn(fn, "lv_slider_set_range(%s, %d, %d);",
                LV_OBJNAME(item),
                lv_xml_atoi(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_atoi(buf_p)
            );
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_slider_orientation_t orentation_text_to_enum_value(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_slider_orientation_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for slider's orientation", txt);
    return "LV_SLIDER_ORIENTATION_AUTO"; /*Return 0 in lack of a better option. */
}

static lv_slider_mode_t mode_text_to_enum_value(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_slider_mode_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for slider's mode", txt);
    return "LV_SLIDER_MODE_NORMAL"; /*Return 0 in lack of a better option. */
}

#endif /* LV_USE_XML */
