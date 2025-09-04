/**
 * @file lv_xml_event_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_event_parser.h"
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
static lv_event_code_t trigger_text_to_enum_value(const char * txt);
//static void free_user_data_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_event_call_function_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(attrs);

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    const char * trigger = lv_xml_get_value_of(attrs, "trigger");

    lv_event_code_t code = "LV_EVENT_CLICKED";
    if(trigger) code = trigger_text_to_enum_value(trigger);
    if(code == NULL)  {
        LV_LOG_WARN("Couldn't add call function event because \"%s\" trigger is invalid.", trigger);
        return NULL;
    }

    const char * cb_txt = lv_xml_get_value_of(attrs, "callback");
    if(cb_txt == NULL) {
        LV_LOG_WARN("callback is mandatory for event-call_function");
        return NULL;
    }

    lv_obj_t * obj = lv_xml_state_get_parent(state);
#if 0
    lv_event_cb_t cb = lv_xml_get_event_cb(&state->scope, cb_txt);
    if(cb == NULL) {
        LV_LOG_WARN("Couldn't add call function event because \"%s\" callback is not found.", cb_txt);
        /*Don't return NULL.
         *When the component is isolated e.g. in the editor the callback is not registered */
        return obj;
    }
#endif
    const char * user_data_xml = lv_xml_get_value_of(attrs, "user_data");
    char * user_data = NULL;
    if(user_data_xml) user_data = lv_strdup(user_data_xml);

    lvgen_new_exprinsn(fn, "lv_obj_add_event_cb(%s, %s, %s, %s);",
        LV_OBJNAME(obj), cb_txt, code, user_data);

    struct func_context* newfn = lvgen_new_module_func_named(lvgen_get_module(), cb_txt);
    if (!lvgen_func_initialized(newfn)) {
        lvgen_set_func_rettype(newfn, LV_TYPE(void));
        lvgen_add_func_argument(newfn, LV_PTYPE(lv_event_t), "e");
        lv_strlcpy(newfn->signature, cb_txt, sizeof(newfn->signature));
        lvgen_new_exprinsn(newfn, "lv_obj_t *target = lv_event_get_target(e);");
        lvgen_new_exprinsn(newfn, "lv_event_code_t code = lv_event_get_code(e);");
        if (user_data != NULL)
            lvgen_new_exprinsn(newfn, "void *user = lv_event_get_user_data(e);\n");
    }

    lvgen_new_module_depend(fn->owner, newfn);
    //lv_obj_add_event_cb(obj, cb, code, user_data);
    //if(user_data) lv_obj_add_event_cb(obj, free_user_data_event_cb, LV_EVENT_DELETE, user_data);

    return obj;
}

void lv_xml_event_call_function_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(state);
    LV_UNUSED(attrs);
    /*Nothing to apply*/
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

//static void free_user_data_event_cb(lv_event_t * e)
//{
//    char * user_data = lv_event_get_user_data(e);
//    lv_free(user_data);
//}

static lv_event_code_t trigger_text_to_enum_value(const char * txt)
{
    char* pv = NULL;

    if (lvgen_cc_find_sym("lv_event_code_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for event's trigger", txt);
    return NULL;
}

#endif /* LV_USE_XML */
