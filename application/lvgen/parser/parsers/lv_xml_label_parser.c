/**
 * @file lv_xml_label_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_label_parser.h"

#if LV_USE_XML
#include "parser/lib/lv_types.h"
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
static lv_label_long_mode_t long_mode_text_to_enum_value(const char * txt);
//static void free_fmt_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void * lv_xml_label_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_label_create", "label");
}

void lv_xml_label_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    void * item = lv_xml_state_get_item(state);
    struct func_context* fn = lv_xml_state_get_active_fn(state);

    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];
        struct fn_param* param;

        param = lv_xml_obj_get_parameter(state->parent_scope, fn, name);

        //lv_xml_obj_get_value(param, )
        if (lv_streq("text", name)) {
            lvgen_new_exprinsn(fn, "lv_label_set_text(%s, %s);",
                LV_OBJNAME(item),
                lv_xml_obj_get_value(param, value)
            );
        }
        if (lv_streq("long_mode", name)) {
            lvgen_new_exprinsn(fn, "lv_label_set_long_mode(%s, %s);",
                LV_OBJNAME(item),
                lv_xml_obj_get_value(param, long_mode_text_to_enum_value(value))
            );
        }
#if 0
        if(lv_streq("bind_text", name)) {
            char buf[256];
            lv_strncpy(buf, value, sizeof(buf));
            char * bufp = buf;
            char * subject_name = lv_xml_split_str(&bufp, ' ');
            if(subject_name) {
                lv_subject_t * subject = lv_xml_get_subject(&state->scope, subject_name);
                if(subject) {
                    char * fmt = bufp; /*The second part is the format text*/
                    if(fmt && fmt[0] == '\0') fmt = NULL;
                    if(fmt) {
                        if(fmt[0] == '\'') fmt++;
                        size_t fmt_len = lv_strlen(fmt);
                        if(fmt_len != 0 && fmt[fmt_len - 1] == '\'') fmt[fmt_len - 1] = '\0';
                        fmt = lv_strdup(fmt);
                        lv_obj_add_event_cb(item, free_fmt_event_cb, LV_EVENT_DELETE, fmt);
                    }
                    lv_label_bind_text(item, subject, fmt);
                }
                else {
                    LV_LOG_WARN("Subject \"%s\" doesn't exist in label bind_text", value);
                }
            }
        }
#endif //if 0
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_label_long_mode_t long_mode_text_to_enum_value(const char * txt)
{
    char* pv = (char *)"LV_LABEL_LONG_MODE_WRAP";

    if (!lvgen_cc_find_sym("lv_label_long_mode_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for label's long_mode", txt);

    return pv;
}

//static void free_fmt_event_cb(lv_event_t * e)
//{
//    void * fmt = lv_event_get_user_data(e);
//    lv_free(fmt);
//}

#endif /* LV_USE_XML */
