/**
 * @file lv_xml_style.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#if LV_USE_XML
#include "lvgen_cinsn.h"

#include "lv_xml_base_types.h"
#include "lv_xml_parser.h"
#include "lv_xml_style.h"
#include "lv_xml_utils.h"
#include "lv_xml_component_private.h"
#include "parser/lib/lv_string.h"
#include "parser/lib/lv_mem.h"
#include "parser/lib/lv_stdio.h"

#include <string.h>

/*********************
 *      DEFINES
 *********************/
#ifdef _MSC_VER
    #define strtok_r strtok_s  // Use strtok_s as an equivalent to strtok_r in Visual Studio
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_style_prop_t style_prop_text_to_enum(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/*Expands to e.g.
  if(lv_streq(name, "height")) lv_style_set_height(style, lv_xml_to_size(value));
 */
//#define SET_STYLE_IF(prop, value) if(lv_streq(name, #prop)) lv_style_set_##prop(style, value)

#define SET_STYLE_IF(prop, value) \
    if(lv_streq(name, #prop)) lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_set_" #prop, "style", value, NULL)

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_state_t lv_xml_style_state_to_enum(const char * txt)
{
    char* pv = NULL;

    if (lvgen_cc_find_sym("lv_state_t", "txt", &pv, NULL))
        return pv;

    return NULL;
}

lv_part_t lv_xml_style_part_to_enum(const char * txt)
{
    char* pv = NULL;

    if (lvgen_cc_find_sym("lv_part_t", "txt", &pv, NULL))
        return pv;

    return NULL;
}

lv_result_t lv_xml_style_register(lv_xml_component_scope_t * scope, const char ** attrs)
{
    const char * style_name =  lv_xml_get_value_of(attrs, "name");
    struct func_context* fn;
    bool global = false;

    if(style_name == NULL) {
        LV_LOG_WARN("'name' is missing from a style");
        return LV_RESULT_INVALID;
    }
    if (scope == NULL) {
        scope = lv_xml_component_get_scope("globals");
        if (scope != NULL) global = true;
    } 
    if(scope == NULL) return LV_RESULT_INVALID;

    lv_xml_style_t * xml_style;
    /*If a style with the same name is already created, use it */
    bool found = false;
    LV_LL_READ(&scope->style_ll, xml_style) {
        if(lv_streq(xml_style->name, style_name)) {
            found = true;
            fn = xml_style->link_fn;
            LV_LOG_INFO("Style %s is already registered. Extending it with new properties.", style_name);
            break;
        }
    }

    if(!found) {
        if (global)
            fn = lvgen_new_global_func();
        else
            fn = lvgen_new_module_func(lvgen_get_module());
        
        xml_style = lv_ll_ins_tail(&scope->style_ll);
        xml_style->name = lv_strdup(style_name);
        //lv_style_init(&xml_style->style);
        size_t long_name_len = lv_strlen(scope->name) + 1 + lv_strlen(style_name) + 1;
        xml_style->long_name = lv_malloc(long_name_len);
        lv_snprintf((char *)xml_style->long_name, long_name_len, "%s.%s", scope->name, style_name); /*E.g. my_button.style1*/

        lv_snprintf(fn->signature, sizeof(fn->signature), "%s_%s_style_init", scope->name, style_name);
        lvgen_add_func_argument(fn, LV_PTYPE(lv_style_t), "style");
        lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_init", "style", NULL);
        xml_style->link_fn = fn;
    }

    lv_style_t * style = &xml_style->style;

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];
        if(lv_streq(name, "name")) continue;
        if(lv_streq(name, "help")) continue;
        if(lv_streq(name, "figma_node_id")) continue;

        if(value[0] == '#') {
            const char * value_clean = &value[1];
            lv_xml_const_t * c;
            LV_LL_READ(&scope->const_ll, c) {
                if(lv_streq(c->name, value_clean)) {
                    value = c->value;
                    break;
                }
            }
        }

        if(lv_streq(value, "remove")) {
            lv_style_prop_t prop = style_prop_text_to_enum(name);
            if (lv_strcmp(prop, "LV_STYLE_PROP_INV")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", prop, NULL);
            }
            else if(lv_streq(name, "pad_all")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_TOP", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_BOTTOM", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_LEFT", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_RIGHT", NULL);
            }
            else if(lv_streq(name, "pad_hor")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_LEFT", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_RIGHT", NULL);
            }
            else if(lv_streq(name, "pad_ver")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_TOP", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_BOTTOM", NULL);
            }
            else if(lv_streq(name, "pad_gap")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_COLUMN", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_PAD_ROW", NULL);
            }
            else if(lv_streq(name, "margin_all")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_TOP", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_BOTTOM", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_LEFT", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_RIGHT", NULL);
            }
            else if(lv_streq(name, "margin_hor")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_LEFT", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_RIGHT", NULL);
            }
            else if(lv_streq(name, "margin_ver")) {
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_TOP", NULL);
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_style_remove_prop", "style", "LV_STYLE_MARGIN_BOTTOM", NULL);
            }
        }
        else SET_STYLE_IF(width, lv_xml_to_size(value));
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
        else SET_STYLE_IF(clip_corner, lv_xml_to_bool_string(value));

        else SET_STYLE_IF(bg_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(bg_color, lv_xml_to_color(value));
        else SET_STYLE_IF(bg_grad_dir, lv_xml_grad_dir_to_enum(value));
        else SET_STYLE_IF(bg_grad_color, lv_xml_to_color(value));
        else SET_STYLE_IF(bg_main_stop, lv_xml_atoi_string(value));
        else SET_STYLE_IF(bg_grad_stop, lv_xml_atoi_string(value));
        else SET_STYLE_IF(bg_grad, lv_xml_component_get_grad(scope, value));

        else SET_STYLE_IF(bg_image_src, lv_xml_get_image(scope, value));
        else SET_STYLE_IF(bg_image_tiled, lv_xml_to_bool_string(value));
        else SET_STYLE_IF(bg_image_recolor, lv_xml_to_color(value));
        else SET_STYLE_IF(bg_image_recolor_opa, lv_xml_to_opa_string(value));

        else SET_STYLE_IF(border_color, lv_xml_to_color(value));
        else SET_STYLE_IF(border_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(border_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(border_side, lv_xml_border_side_to_enum(value));
        else SET_STYLE_IF(border_post, lv_xml_to_bool_string(value));

        else SET_STYLE_IF(outline_color, lv_xml_to_color(value));
        else SET_STYLE_IF(outline_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(outline_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(outline_pad, lv_xml_atoi_string(value));

        else SET_STYLE_IF(shadow_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(shadow_color, lv_xml_to_color(value));
        else SET_STYLE_IF(shadow_offset_x, lv_xml_atoi_string(value));
        else SET_STYLE_IF(shadow_offset_y, lv_xml_atoi_string(value));
        else SET_STYLE_IF(shadow_spread, lv_xml_atoi_string(value));
        else SET_STYLE_IF(shadow_opa, lv_xml_to_opa_string(value));

        else SET_STYLE_IF(text_color, lv_xml_to_color(value));
        else SET_STYLE_IF(text_font, lv_xml_get_font(scope, value));
        else SET_STYLE_IF(text_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(text_align, lv_xml_text_align_to_enum(value));
        else SET_STYLE_IF(text_letter_space, lv_xml_atoi_string(value));
        else SET_STYLE_IF(text_line_space, lv_xml_atoi_string(value));
        else SET_STYLE_IF(text_decor, lv_xml_text_decor_to_enum(value));

        else SET_STYLE_IF(image_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(image_recolor, lv_xml_to_color(value));
        else SET_STYLE_IF(image_recolor_opa, lv_xml_to_opa_string(value));

        else SET_STYLE_IF(line_color, lv_xml_to_color(value));
        else SET_STYLE_IF(line_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(line_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(line_dash_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(line_dash_gap, lv_xml_atoi_string(value));
        else SET_STYLE_IF(line_rounded, lv_xml_to_bool_string(value));

        else SET_STYLE_IF(arc_color, lv_xml_to_color(value));
        else SET_STYLE_IF(arc_opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(arc_width, lv_xml_atoi_string(value));
        else SET_STYLE_IF(arc_rounded, lv_xml_to_bool_string(value));
        else SET_STYLE_IF(arc_image_src, lv_xml_get_image(scope, value));

        else SET_STYLE_IF(opa, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(opa_layered, lv_xml_to_opa_string(value));
        else SET_STYLE_IF(color_filter_opa, lv_xml_to_opa_string(value));
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
        else SET_STYLE_IF(bitmap_mask_src, lv_xml_get_image(scope, value));
        else SET_STYLE_IF(rotary_sensitivity, lv_xml_atoi_string(value));
        else SET_STYLE_IF(recolor, lv_xml_to_color(value));
        else SET_STYLE_IF(recolor_opa, lv_xml_to_opa_string(value));

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


        else {
            LV_LOG_WARN("%s style property is not supported", name);
        }
    }

    return LV_RESULT_OK;
}

const char * lv_xml_style_string_process(char * txt, lv_style_selector_t * selector)
{
    static char sty_buf[256];
    char* p = sty_buf;
    int remain = sizeof(sty_buf);
    int len = 0;

    char * style_name = lv_xml_split_str(&txt, ':');
    char * selector_str = lv_xml_split_str(&txt, ':');
    while(selector_str != NULL && remain > 0) {
        /* Handle different states and parts */
        len += (int)lv_strlcpy(p + len, lv_xml_style_state_to_enum(selector_str), remain);
        remain -= len;
        if (remain <= 0)
            break;

        p[len++] = '|';
        remain--;

        len += (int)lv_strlcpy(p + len, lv_xml_style_part_to_enum(selector_str), remain);
        remain -= len;
   
        /* Move to the next token */
        selector_str = lv_xml_split_str(&txt, ':');
    }

    if (remain < sizeof(sty_buf))
        *selector = sty_buf;
    else
        *selector = "LV_PART_MAIN";

    return style_name;
}

void lv_xml_style_add_to_obj(lv_xml_parser_state_t * state, lv_obj_t * obj, const char * text)
{
    char * str = lv_strdup(text);
    char * str_ori = str;

    /* Split the string based on space and colons */
    char * onestyle_str = lv_xml_split_str(&str, ' ');
    while(onestyle_str != NULL) {
        /* Parse the parts and states after the space */
        lv_style_selector_t selector = 0;
        const char * style_name = lv_xml_style_string_process(onestyle_str, &selector);
        if(style_name != NULL) {
            lv_xml_style_t * xml_style = NULL;
            /*Resolve parameters or just find the style*/
            if(style_name[0] == '$') {
                /*E.g. `$pr_style` style name means use the value
                 *coming from the parent's `pr_style` named attribute*/
                const char * name_clean = &style_name[1];
                const char * parent_style_name = lv_xml_get_value_of(state->parent_attrs, name_clean);
                if(parent_style_name) {
                    xml_style = lv_xml_get_style_by_name(state->parent_scope, parent_style_name);
                }
            }
            else {
                xml_style = lv_xml_get_style_by_name(&state->scope, style_name);
            }
            if(xml_style) {
                /* Apply with the selector */
                //lv_obj_add_style(obj, &xml_style->style, selector);
                struct func_context* fn = xml_style->link_fn;
                lvgen_new_callinsn(fn, LV_TYPE(void), "lv_obj_add_style", LV_OBJNAME(obj), "style", selector, NULL);
            }
        }
        onestyle_str = lv_xml_split_str(&str, ' ');
    }

    lv_free(str_ori);
}

lv_xml_style_t * lv_xml_get_style_by_name(lv_xml_component_scope_t * scope, const char * style_name_raw)
{
    const char * style_name = strrchr(style_name_raw, '.');

    if(style_name) {
        char component_name[256];
        size_t len = (size_t)(style_name - style_name_raw);
        lv_memcpy(component_name, style_name_raw, len);
        component_name[len] = '\0';
        scope = lv_xml_component_get_scope(component_name);
        if(scope == NULL) {
            LV_LOG_WARN("'%s' component or widget is not found", component_name);
        }
        style_name++; /*Skip the dot*/
    }
    else {
        style_name = style_name_raw;
    }

    /*Use the global scope is not specified*/
    if(scope == NULL) scope = lv_xml_component_get_scope("globals");
    if(scope == NULL) return NULL;

    lv_xml_style_t * xml_style;
    LV_LL_READ(&scope->style_ll, xml_style) {
        if(lv_streq(xml_style->name, style_name)) return xml_style;
    }

    /*If not found in the component check the global space*/
    if(!lv_streq(scope->name, "globals")) {
        scope = lv_xml_component_get_scope("globals");
        if(scope) {
            LV_LL_READ(&scope->style_ll, xml_style) {
                if(lv_streq(xml_style->name, style_name)) return xml_style;
            }
        }
    }

    LV_LOG_WARN("No style found with %s name", style_name_raw);

    return NULL;
}

lv_grad_dsc_t * lv_xml_component_get_grad(lv_xml_component_scope_t * scope, const char * name)
{
    lv_xml_grad_t * d;
    LV_LL_READ(&scope->gradient_ll, d) {
        if (lv_streq(d->name, name)) return (lv_grad_dsc_t*)"none"; // return &d->grad_dsc;
    }

    return NULL;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_style_prop_t style_prop_text_to_enum(const char * txt)
{
    char* pv = NULL;

    if (lvgen_cc_find_sym("styles", "txt", &pv, NULL))
        return pv;

    return NULL;
}

#endif /* LV_USE_XML */
