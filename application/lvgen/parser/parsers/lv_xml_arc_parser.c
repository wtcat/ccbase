/**
 * @file lv_xml_arc_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_arc_parser.h"
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
static lv_arc_mode_t mode_text_to_enum_value(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_arc_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_arc_create", "arc");
}

void lv_xml_arc_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);
    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];
        //struct fn_param* param;
        //param = lv_xml_obj_get_parameter(state->parent_scope, fn, name);

        if(lv_streq("angles", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //int32_t v2 = lv_xml_atoi(buf_p);
            //lv_arc_set_angles(item, v1, v2);
            lvgen_new_callinsn(fn, LV_PTYPE(void), "lv_arc_set_angles", 
                LV_OBJNAME(item),
                lv_xml_atoi_string(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_atoi_string(buf_p),
                NULL);
        }
        else if(lv_streq("bg_angles", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //int32_t v2 = lv_xml_atoi(buf_p);
            //lv_arc_set_bg_angles(item, v1, v2);
            lvgen_new_callinsn(fn, LV_PTYPE(void), "lv_arc_set_bg_angles",
                LV_OBJNAME(item),
                lv_xml_atoi_string(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_atoi_string(buf_p),
                NULL);
        }
        else if(lv_streq("range", name)) {
            char buf[64];
            lv_strlcpy(buf, value, sizeof(buf));
            char * buf_p = buf;
            //int32_t v1 = lv_xml_atoi(lv_xml_split_str(&buf_p, ' '));
            //int32_t v2 = lv_xml_atoi(buf_p);
            //lv_arc_set_range(item, v1, v2);
            lvgen_new_callinsn(fn, LV_PTYPE(void), "lv_arc_set_range",
                LV_OBJNAME(item),
                lv_xml_atoi_string(lv_xml_split_str(&buf_p, ' ')),
                lv_xml_atoi_string(buf_p),
                NULL);
        }
        else if(lv_streq("value", name)) {
            //lv_arc_set_value(item, lv_xml_atoi(value));
            lvgen_new_callinsn(fn, LV_PTYPE(void), "lv_arc_set_value",
                LV_OBJNAME(item),
                lv_xml_atoi_string(value),
                NULL);
        }
        else if(lv_streq("mode", name)) {
            //lv_arc_set_mode(item, mode_text_to_enum_value(value));
            lvgen_new_callinsn(fn, LV_PTYPE(void), "lv_arc_set_mode",
                LV_OBJNAME(item),
                mode_text_to_enum_value(value),
                NULL);
        }
#if 0
        else if(lv_streq("bind_value", name)) {
            lv_subject_t * subject = lv_xml_get_subject(&state->scope, value);
            if(subject) {
                lv_arc_bind_value(item, subject);
            }
            else {
                LV_LOG_WARN("Subject \"%s\" doesn't exist in arc bind_value", value);
            }
        }
#endif
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static lv_arc_mode_t mode_text_to_enum_value(const char * txt)
{
    char* pv = (char*)"LV_ARC_MODE_NORMAL";

    if (!lvgen_cc_find_sym("lv_arc_mode_t", "txt", &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for bar's mode", txt);

    return pv;
}
#endif /* LV_USE_XML */
