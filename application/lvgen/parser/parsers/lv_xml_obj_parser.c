/**
 * @file lv_xml_obj_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#
#include "lv_xml_obj_parser.h"
#include "lvgen_cinsn.h"

#if LV_USE_XML


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_obj_flag_t flag_to_enum(const char * txt);
static void apply_styles(lv_xml_parser_state_t * state, lv_obj_t * obj, const char * name, const char * value,
    struct fn_param* param);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/* Expands to
   if(lv_streq(prop_name, "style_height")) lv_obj_set_style_height(obj, value, selector)
 */
//#define SET_STYLE_IF(prop, value) if(lv_streq(prop_name, "style_" #prop)) lv_obj_set_style_##prop(obj, value, selector)

#define SET_STYLE_IF(prop, value) \
    if(lv_streq(prop_name, "style_" #prop)) lvgen_new_callinsn(fn, LV_TYPE(void), "lv_obj_set_style_" #prop, LV_OBJNAME(obj), value, selector, NULL)



/**********************
 *   GLOBAL FUNCTIONS
 **********************/
struct fn_param* lv_xml_obj_get_parameter(lv_xml_component_scope_t * parent_scope,
    struct func_context* fn, const char* name) {
    struct fn_param* param = lvgen_get_fnparam(fn, name);
    //if (param != NULL) {
    //    if (parent_scope != NULL) {
    //        struct fn_param* param_p;
    //        param_p = lvgen_get_fnparam(parent_scope->active_func, param->name + 1);
    //        if (param_p != NULL)
    //            lv_strlcpy(param->pname, param_p->name, LV_SYMBOL_LEN);
    //    }
    //}
    return param;
}

const char* lv_xml_obj_get_value(struct fn_param* param, const char* value) {
    if (param == NULL)
        return value;

    lvgen_fnparam_copy_value(param, value);
    //if (param->pname[0] != '\0')
    //    return param->pname + 1;

    return param->name + 1;
}

void * lv_xml_obj_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_obj_create", "obj");
}

void lv_xml_obj_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    lv_obj_t *item = lv_xml_state_get_item(state);
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    const char* pv;

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];
        size_t name_len = lv_strlen(name);
        struct fn_param* param;

        param = lvgen_get_fnparam(fn, name); // lv_xml_obj_get_parameter(state->parent_scope, fn, name);
#if LV_USE_OBJ_NAME
        if(lv_streq("name", name)) {
            lv_obj_set_name(item, value);
        }
#endif
        
        if (lv_streq("x", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_x(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_to_size(value)));
        }
        else if (lv_streq("y", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_y(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_to_size(value)));
        }
        else if (lv_streq("width", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_width(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_to_size(value)));
        }
        else if (lv_streq("height", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_height(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_to_size(value)));
        }
        else if (lv_streq("align", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_align(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_align_to_enum(value)));
        }
        else if (lv_streq("flex_flow", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_flex_flow(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_flex_flow_to_enum(value)));
        }
        else if (lv_streq("flex_grow", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_flex_grow(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_atoi_string(value)));
        }
        else if (lv_streq("ext_click_area", name)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_ext_click_area(%s, %s);",
                LV_OBJNAME(item), lv_xml_obj_get_value(param, lv_xml_atoi_string(value)));
        }
        else if (lvgen_cc_find_sym("lv_obj_flag_t", name, &pv, NULL)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_flag(%s, %s, %s);",
                LV_OBJNAME(item), pv, lv_xml_obj_get_value(param, lv_xml_to_bool_string(value)));
        }
        else if (lvgen_cc_find_sym("lv_state_t", name, &pv, NULL)) {
            lvgen_new_exprinsn(fn, "lv_obj_set_state(%s, %s, %s);",
                LV_OBJNAME(item), pv, lv_xml_obj_get_value(param, lv_xml_to_bool_string(value)));
        }
        else if (lv_streq("styles", name)) {
            lv_xml_style_add_to_obj(state, item, value);
        }

#if 0
        else if(lv_streq("bind_checked", name)) {
            lv_subject_t * subject = lv_xml_get_subject(&state->scope, value);
            if(subject) {
                lv_obj_bind_checked(item, subject);
            }
            else {
                LV_LOG_WARN("Subject \"%s\" doesn't exist in lv_obj bind_checked", value);
            }
        }

        else if(name_len >= 15 && lv_memcmp("bind_flag_if_", name, 13) == 0) {
            lv_observer_t * (*cb)(lv_obj_t * obj, lv_subject_t * subject, lv_obj_flag_t flag, int32_t ref_value) = NULL;
            if(name[13] == 'e' && name[14] == 'q') cb = lv_obj_bind_flag_if_eq;
            else if(name[13] == 'n' && name[14] == 'o') cb = lv_obj_bind_flag_if_not_eq;
            else if(name[13] == 'g' && name[14] == 't') cb = lv_obj_bind_flag_if_gt;
            else if(name[13] == 'g' && name[14] == 'e') cb = lv_obj_bind_flag_if_ge;
            else if(name[13] == 'l' && name[14] == 't') cb = lv_obj_bind_flag_if_lt;
            else if(name[13] == 'l' && name[14] == 'e') cb = lv_obj_bind_flag_if_le;

            if(cb) {
                char buf[128];
                lv_strlcpy(buf, value, sizeof(buf));
                char * bufp = buf;
                const char * subject_str =  lv_xml_split_str(&bufp, ' ');
                const char * flag_str =  lv_xml_split_str(&bufp, ' ');
                const char * ref_value_str =  lv_xml_split_str(&bufp, ' ');

                if(subject_str == NULL) {
                    LV_LOG_WARN("Subject is missing in lv_obj bind_flag");
                }
                else if(flag_str == NULL) {
                    LV_LOG_WARN("Flag is missing in lv_obj bind_flag");
                }
                else if(ref_value_str == NULL) {
                    LV_LOG_WARN("Reference value is missing in lv_obj bind_flag");
                }
                else {
                    lv_subject_t * subject = lv_xml_get_subject(&state->scope, subject_str);
                    if(subject == NULL) {
                        LV_LOG_WARN("Subject \"%s\" doesn't exist in lv_obj bind_flag", value);
                    }
                    else {
                        lv_obj_flag_t flag = flag_to_enum(flag_str);
                        int32_t ref_value = lv_xml_atoi(ref_value_str);
                        cb(item, subject, flag, ref_value);
                    }
                }
            }
        }
        else if(name_len >= 16 && lv_memcmp("bind_state_if_", name, 14) == 0) {
            lv_observer_t * (*cb)(lv_obj_t * obj, lv_subject_t * subject, lv_state_t flag, int32_t ref_value) = NULL;
            if(name[14] == 'e' && name[15] == 'q') cb = lv_obj_bind_state_if_eq;
            else if(name[14] == 'n' && name[15] == 'o') cb = lv_obj_bind_state_if_not_eq;
            else if(name[14] == 'g' && name[15] == 't') cb = lv_obj_bind_state_if_gt;
            else if(name[14] == 'g' && name[15] == 'e') cb = lv_obj_bind_state_if_ge;
            else if(name[14] == 'l' && name[15] == 't') cb = lv_obj_bind_state_if_lt;
            else if(name[14] == 'l' && name[15] == 'e') cb = lv_obj_bind_state_if_le;

            if(cb) {
                char buf[128];
                lv_strlcpy(buf, value, sizeof(buf));
                char * bufp = buf;
                const char * subject_str =  lv_xml_split_str(&bufp, ' ');
                const char * state_str =  lv_xml_split_str(&bufp, ' ');
                const char * ref_value_str =  lv_xml_split_str(&bufp, ' ');

                if(subject_str == NULL) {
                    LV_LOG_WARN("Subject is missing in lv_obj bind_state");
                }
                else if(state_str == NULL) {
                    LV_LOG_WARN("State is missing in lv_obj bind_state");
                }
                else if(ref_value_str == NULL) {
                    LV_LOG_WARN("Reference value is missing in lv_obj bind_state");
                }
                else {
                    lv_subject_t * subject = lv_xml_get_subject(&state->scope, subject_str);
                    if(subject == NULL) {
                        LV_LOG_WARN("Subject \"%s\" doesn't exist in lv_obj bind_state", value);
                    }
                    else {
                        lv_state_t obj_state = lv_xml_state_to_enum(state_str);
                        int32_t ref_value = lv_xml_atoi(ref_value_str);
                        cb(item, subject, obj_state, ref_value);
                    }
                }
            }
        }
#endif //if 0
        else if(name_len > 6 && lv_memcmp("style_", name, 6) == 0) {
            apply_styles(state, item, name, value, param);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_obj_flag_t flag_to_enum(const char * txt)
{
    char* pv;

    if (lvgen_cc_find_sym("lv_obj_flag_t", txt, &pv, NULL))
        return pv;

    LV_LOG_WARN("%s is an unknown value for flag", txt);
    return "0";
}


static void apply_styles(lv_xml_parser_state_t * state, lv_obj_t * obj, const char * name, 
    const char * value, struct fn_param* param)
{
    char name_local[512];
    lv_strlcpy(name_local, name, sizeof(name_local));

    lv_style_selector_t selector;
    const char * prop_name = lv_xml_style_string_process(name_local, &selector);
    struct func_context* fn = lv_xml_state_get_active_fn(state);

    const char* prop_value = NULL;
    char *pt;

    if (lvgen_cc_find_sym("styles", prop_name + sizeof("style_") - 1, NULL, &pt)) {
        if (!lv_strcmp(pt, "size")) {
            prop_value = !param? lv_xml_to_size(value): lv_xml_obj_get_value(param, lv_xml_to_size(value));
        }
        else if (!lv_strcmp(pt, "int")) {
            prop_value = !param ? lv_xml_atoi_string(value) : lv_xml_obj_get_value(param, lv_xml_atoi_string(value));
        }
        else if (!lv_strcmp(pt, "lv_base_dir_t")) {
            prop_value = !param ? lv_xml_base_dir_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_base_dir_to_enum(value));
        }
        else if (!lv_strcmp(pt, "opa")) {
            prop_value = !param ? lv_xml_to_opa_string(value) : lv_xml_obj_get_value(param, lv_xml_to_opa_string(value));
        }
        else if (!lv_strcmp(pt, "color")) {
            prop_value = !param ? lv_xml_to_color(value) : lv_xml_obj_get_value(param, lv_xml_to_color(value));
        }
        else if (!lv_strcmp(pt, "lv_grad_dir_t")) {
            prop_value = !param ? lv_xml_grad_dir_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_grad_dir_to_enum(value));
        }
        else if (!lv_strcmp(pt, "image")) {
            //prop_value = !param ? lv_xml_grad_dir_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_grad_dir_to_enum(value));
        }
        else if (!lv_strcmp(pt, "bool")) {
            prop_value = !param ? lv_xml_to_bool_string(value) : lv_xml_obj_get_value(param, lv_xml_to_bool_string(value));
        }
        else if (!lv_strcmp(pt, "side")) {
            prop_value = !param ? lv_xml_border_side_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_border_side_to_enum(value));
        }
        else if (!lv_strcmp(pt, "font")) {
            //TODO: fix
            prop_value = !param ? lv_xml_get_font(&state->scope, value) : lv_xml_obj_get_value(param, lv_xml_get_font(&state->scope, value));
        }
        else if (!lv_strcmp(pt, "lv_text_align_t")) {
            prop_value = !param ? lv_xml_text_align_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_text_align_to_enum(value));
        }
        else if (!lv_strcmp(pt, "lv_text_decor_t")) {
            prop_value = !param ? lv_xml_text_decor_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_text_decor_to_enum(value));
        }
        else if (!lv_strcmp(pt, "lv_blend_mode_t")) {
            prop_value = !param ? lv_xml_blend_mode_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_blend_mode_to_enum(value));
        }
        else if (!lv_strcmp(pt, "lv_layout_t")) {
            prop_value = !param ? lv_xml_layout_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_layout_to_enum(value));
        }
        else if (!lv_strcmp(pt, "lv_flex_flow_t")) {
            prop_value = !param ? lv_xml_flex_flow_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_flex_flow_to_enum(value));
        }
        else if (!lv_strcmp(pt, "enum_flex_align")) {
            prop_value = !param ? lv_xml_flex_align_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_flex_align_to_enum(value));
        }
        else if (!lv_strcmp(pt, "lv_grid_align_t")) {
            prop_value = !param ? lv_xml_grid_align_to_enum(value) : lv_xml_obj_get_value(param, lv_xml_grid_align_to_enum(value));
        }
        else {
            //TODO:
            printf("Invalid style type: name(%s) type(%s)\n", prop_name, pt);
        }

        lvgen_new_exprinsn(fn, "lv_obj_set_%s(%s, %s, %s);", prop_name, LV_OBJNAME(obj), prop_value, selector);
    }
    
#if 0  
    SET_STYLE_IF(width, lv_xml_to_size(value));
    else SET_STYLE_IF(min_width, lv_xml_to_size(value));
    else SET_STYLE_IF(max_width, lv_xml_to_size(value));
    else SET_STYLE_IF(height, lv_xml_to_size(value));
    else SET_STYLE_IF(min_height, lv_xml_to_size(value));
    else SET_STYLE_IF(max_height, lv_xml_to_size(value));
    else SET_STYLE_IF(length, lv_xml_to_size(value));
    else SET_STYLE_IF(radius, lv_xml_to_size(value));

    else SET_STYLE_IF(pad_left, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_right, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_top, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_bottom, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_hor, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_ver, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_all, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_row, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_column, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_gap, lv_xml_atoi_string(value));
    else SET_STYLE_IF(pad_radial, lv_xml_atoi_string(value));

    else SET_STYLE_IF(margin_left, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_right, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_top, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_bottom, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_hor, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_ver, lv_xml_atoi_string(value));
    else SET_STYLE_IF(margin_all, lv_xml_atoi_string(value));

    else SET_STYLE_IF(base_dir, lv_xml_base_dir_to_enum(value));
    else SET_STYLE_IF(clip_corner, lv_xml_to_bool(value));

    else SET_STYLE_IF(bg_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(bg_color, lv_xml_to_color(value));
    else SET_STYLE_IF(bg_grad_dir, lv_xml_grad_dir_to_enum(value));
    else SET_STYLE_IF(bg_grad_color, lv_xml_to_color(value));
    else SET_STYLE_IF(bg_main_stop, lv_xml_atoi_string(value));
    else SET_STYLE_IF(bg_grad_stop, lv_xml_atoi_string(value));
    //else SET_STYLE_IF(bg_grad, lv_xml_component_get_grad(&state->scope, value));

    //else SET_STYLE_IF(bg_image_src, lv_xml_get_image(&state->scope, value));
    else SET_STYLE_IF(bg_image_tiled, lv_xml_to_bool(value));
    else SET_STYLE_IF(bg_image_recolor, lv_xml_to_color(value));
    else SET_STYLE_IF(bg_image_recolor_opa, lv_xml_to_opa(value));

    else SET_STYLE_IF(border_color, lv_xml_to_color(value));
    else SET_STYLE_IF(border_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(border_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(border_side, lv_xml_border_side_to_enum(value));
    else SET_STYLE_IF(border_post, lv_xml_to_bool(value));

    else SET_STYLE_IF(outline_color, lv_xml_to_color(value));
    else SET_STYLE_IF(outline_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(outline_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(outline_pad, lv_xml_atoi_string(value));

    else SET_STYLE_IF(shadow_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(shadow_color, lv_xml_to_color(value));
    else SET_STYLE_IF(shadow_offset_x, lv_xml_atoi_string(value));
    else SET_STYLE_IF(shadow_offset_y, lv_xml_atoi_string(value));
    else SET_STYLE_IF(shadow_spread, lv_xml_atoi_string(value));
    else SET_STYLE_IF(shadow_opa, lv_xml_to_opa(value));

    else SET_STYLE_IF(text_color, lv_xml_to_color(value));
    else SET_STYLE_IF(text_font, lv_xml_get_font(&state->scope, value));
    else SET_STYLE_IF(text_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(text_align, lv_xml_text_align_to_enum(value));
    else SET_STYLE_IF(text_letter_space, lv_xml_atoi_string(value));
    else SET_STYLE_IF(text_line_space, lv_xml_atoi_string(value));
    else SET_STYLE_IF(text_decor, lv_xml_text_decor_to_enum(value));

    else SET_STYLE_IF(image_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(image_recolor, lv_xml_to_color(value));
    else SET_STYLE_IF(image_recolor_opa, lv_xml_to_opa(value));

    else SET_STYLE_IF(line_color, lv_xml_to_color(value));
    else SET_STYLE_IF(line_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(line_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(line_dash_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(line_dash_gap, lv_xml_atoi_string(value));
    else SET_STYLE_IF(line_rounded, lv_xml_to_bool(value));

    else SET_STYLE_IF(arc_color, lv_xml_to_color(value));
    else SET_STYLE_IF(arc_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(arc_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(arc_rounded, lv_xml_to_bool(value));
    else SET_STYLE_IF(arc_image_src, lv_xml_get_image(&state->scope, value));

    else SET_STYLE_IF(opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(opa_layered, lv_xml_to_opa(value));
    else SET_STYLE_IF(color_filter_opa, lv_xml_to_opa(value));
    else SET_STYLE_IF(anim_duration, lv_xml_atoi_string(value));
    else SET_STYLE_IF(blend_mode, lv_xml_blend_mode_to_enum(value));
    else SET_STYLE_IF(transform_width, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_height, lv_xml_atoi_string(value));
    else SET_STYLE_IF(translate_x, lv_xml_atoi_string(value));
    else SET_STYLE_IF(translate_y, lv_xml_atoi_string(value));
    else SET_STYLE_IF(translate_radial, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_scale_x, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_scale_y, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_rotation, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_pivot_x, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_pivot_y, lv_xml_atoi_string(value));
    else SET_STYLE_IF(transform_skew_x, lv_xml_atoi_string(value));
    else SET_STYLE_IF(bitmap_mask_src, lv_xml_get_image(&state->scope, value));
    else SET_STYLE_IF(rotary_sensitivity, lv_xml_atoi_string(value));
    else SET_STYLE_IF(recolor, lv_xml_to_color(value));
    else SET_STYLE_IF(recolor_opa, lv_xml_to_opa(value));

    else SET_STYLE_IF(layout, lv_xml_layout_to_enum(value));

    else SET_STYLE_IF(flex_flow, lv_xml_flex_flow_to_enum(value));
    else SET_STYLE_IF(flex_grow, lv_xml_atoi_string(value));
    else SET_STYLE_IF(flex_main_place, lv_xml_flex_align_to_enum(value));
    else SET_STYLE_IF(flex_cross_place, lv_xml_flex_align_to_enum(value));
    else SET_STYLE_IF(flex_track_place, lv_xml_flex_align_to_enum(value));

    else SET_STYLE_IF(grid_column_align, lv_xml_grid_align_to_enum(value));
    else SET_STYLE_IF(grid_row_align, lv_xml_grid_align_to_enum(value));
    else SET_STYLE_IF(grid_cell_column_pos, lv_xml_atoi_string(value));
    else SET_STYLE_IF(grid_cell_column_span, lv_xml_atoi_string(value));
    else SET_STYLE_IF(grid_cell_x_align, lv_xml_grid_align_to_enum(value));
    else SET_STYLE_IF(grid_cell_row_pos, lv_xml_atoi_string(value));
    else SET_STYLE_IF(grid_cell_row_span, lv_xml_atoi_string(value));
    else SET_STYLE_IF(grid_cell_y_align, lv_xml_grid_align_to_enum(value));
#endif //if 0
}

#endif /* LV_USE_XML */
