/**
 * @file lv_xml_chart_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_chart_parser.h"
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
static lv_chart_type_t chart_type_to_enum(const char * txt);
static lv_chart_update_mode_t chart_update_mode_to_enum(const char * txt);
static lv_chart_axis_t chart_axis_to_enum(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_chart_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_chart_create", "chart");
}

void lv_xml_chart_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);

    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if(lv_streq("point_count", name)) {
            int32_t cnt = lv_xml_atoi(value);
            if(cnt < 0) {
                LV_LOG_WARN("chart's point count can't be negative");
                cnt = 0;
            }
            //lv_chart_set_point_count(item, cnt);
            lvgen_new_exprinsn(fn, "lv_chart_set_point_count(%s, %d);",
                LV_OBJNAME(item), cnt);
        }
        else if (lv_streq("type", name)) {
            //lv_chart_set_type(item, chart_type_to_enum(value));
            lvgen_new_exprinsn(fn, "lv_chart_set_type(%s, %s);",
                LV_OBJNAME(item), chart_type_to_enum(value));
        }
        else if (lv_streq("update_mode", name)) {
            //lv_chart_set_update_mode(item, chart_update_mode_to_enum(value));
            lvgen_new_exprinsn(fn, "lv_chart_set_update_mode(%s, %s);",
                LV_OBJNAME(item), chart_update_mode_to_enum(value));
        }
        else if(lv_streq("div_line_count", name)) {

            int32_t value1 = lv_xml_atoi_split(&value, ' ');
            int32_t value2 = lv_xml_atoi_split(&value, ' ');
            //lv_chart_set_div_line_count(item, value1, value2);
            lvgen_new_exprinsn(fn, "lv_chart_set_div_line_count(%s, %d, %d);",
                LV_OBJNAME(item), value1, value2);
        }
    }
}

void * lv_xml_chart_series_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    const char * color = lv_xml_get_value_of(attrs, "color");
    const char * axis = lv_xml_get_value_of(attrs, "axis");
    if(color == NULL) color = "0xff0000";
    if(axis == NULL) axis = "primary_y";

    //void * item = lv_chart_add_series(lv_xml_state_get_parent(state), lv_color_hex(lv_xml_strtol(color, NULL, 16)),
    //                                  chart_axis_to_enum(axis));
    //return item;

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    lv_obj_t* parent = lv_xml_state_get_parent(state);
    LV_UNUSED(attrs);

    struct func_callinsn* insn = lvgen_new_callinsn(fn, LV_PTYPE(lv_chart_series_t), 
        "lv_chart_add_series", 
        LV_OBJNAME(parent), 
        color, 
        chart_axis_to_enum(axis),
        NULL);

    lv_obj_t* obj = lvgen_new_lvalue(fn, "chart", insn);
    obj->scope_fn = fn;
    
    return obj;
}

void lv_xml_chart_series_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(state);
    LV_UNUSED(attrs);

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    lv_obj_t * chart = lv_xml_state_get_parent(state);
    lv_chart_series_t * ser = lv_xml_state_get_item(state);

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if(lv_streq("values", name)) {
            while(value[0] != '\0') {
                int32_t v = lv_xml_atoi_split(&value, ' ');
                //lv_chart_set_next_value(chart, ser, v);
                lvgen_new_exprinsn(fn, "lv_chart_set_next_value(%s, %s, %d);",
                    LV_OBJNAME(chart), LV_OBJNAME(ser), v);
            }
        }
    }
}

void * lv_xml_chart_cursor_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    const char * color = lv_xml_get_value_of(attrs, "color");
    const char * dir = lv_xml_get_value_of(attrs, "dir");
    if(color == NULL) color = "0x0000ff";
    if(dir == NULL) dir = "all";

    //void * item = lv_chart_add_cursor(lv_xml_state_get_parent(state), lv_color_hex(lv_xml_strtol(color, NULL, 16)),
    //                                  lv_xml_dir_to_enum(dir));

    //return item;

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    lv_obj_t* parent = lv_xml_state_get_parent(state);
    LV_UNUSED(attrs);

    struct func_callinsn* insn = lvgen_new_callinsn(fn, LV_PTYPE(lv_chart_cursor_t),
        "lv_chart_add_cursor",
        LV_OBJNAME(parent),
        color,
        lv_xml_dir_to_enum(dir),
        NULL);

    lv_obj_t* obj = lvgen_new_lvalue(fn, "chart_cursor", insn);
    obj->scope_fn = fn;

    return obj;
}

void lv_xml_chart_cursor_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(state);
    LV_UNUSED(attrs);

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    lv_obj_t * chart = lv_xml_state_get_parent(state);
    lv_chart_cursor_t * cursor = lv_xml_state_get_item(state);

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if (lv_streq("pos_x", name)) {
            //lv_chart_set_cursor_pos_x(chart, cursor, lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_chart_set_cursor_pos_x(%s, %s, %d);",
                LV_OBJNAME(chart), LV_OBJNAME(cursor), lv_xml_atoi(value));
        }
        if (lv_streq("pos_y", name)) {
            //lv_chart_set_cursor_pos_y(chart, cursor, lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_chart_set_cursor_pos_y(%s, %s, %d);",
                LV_OBJNAME(chart), LV_OBJNAME(cursor), lv_xml_atoi(value));
        }
    }
}

void * lv_xml_chart_axis_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(attrs);

    /*Nothing to create*/
    return lv_xml_state_get_parent(state);
}

void lv_xml_chart_axis_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    LV_UNUSED(state);
    LV_UNUSED(attrs);

    struct func_context* fn = lv_xml_state_get_active_fn(state);
    lv_obj_t * chart = lv_xml_state_get_parent(state);
    lv_chart_axis_t axis = chart_axis_to_enum(lv_xml_get_value_of(attrs, "axis"));

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if(lv_streq("range", name)) {
            int32_t min_val = lv_xml_atoi_split(&value, ' ');
            int32_t max_val = lv_xml_atoi_split(&value, ' ');
            //lv_chart_set_axis_range(chart, axis, min_val, max_val);
            lvgen_new_exprinsn(fn, "lv_chart_set_axis_range(%s, %s, %d, %d);",
                LV_OBJNAME(chart), axis, min_val, max_val);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_chart_type_t chart_type_to_enum(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_chart_type_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for chart's chart_type", txt);
    return "LV_CHART_TYPE_NONE"; /*Return 0 in lack of a better option. */
}

static lv_chart_update_mode_t chart_update_mode_to_enum(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_chart_update_mode_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for chart's chart_update_mode", txt);
    return "LV_CHART_UPDATE_MODE_SHIFT"; /*Return 0 in lack of a better option. */
}

static lv_chart_axis_t chart_axis_to_enum(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_chart_axis_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for chart's chart_axis", txt);
    return "LV_CHART_AXIS_PRIMARY_X"; /*Return 0 in lack of a better option. */
}

#endif /* LV_USE_XML */
