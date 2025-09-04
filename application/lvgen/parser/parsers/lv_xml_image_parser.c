/**
 * @file lv_xml_image_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_xml_image_parser.h"
#if LV_USE_XML

#include "lvgen_cinsn.h"
#include "parser/lib/lv_stdio.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef const char* lv_image_align_t;
/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_image_align_t image_align_to_enum(const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void * lv_xml_image_create(lv_xml_parser_state_t * state, const char ** attrs)
{
    return lv_xml_default_widget_create(state, attrs, "lv_image_create", "image");
}

void lv_xml_image_apply(lv_xml_parser_state_t * state, const char ** attrs)
{
    struct func_context* fn = lv_xml_state_get_active_fn(state);
    void * item = lv_xml_state_get_item(state);

    lv_xml_obj_apply(state, attrs); /*Apply the common properties, e.g. width, height, styles flags etc*/

    for(int i = 0; attrs[i]; i += 2) {
        const char * name = attrs[i];
        const char * value = attrs[i + 1];

        if (lv_streq("src", name)) { //lv_image_set_src(item, lv_xml_get_image(&state->scope, value));
            lvgen_new_exprinsn(fn, "lv_image_set_src(%s, %s);", 
                LV_OBJNAME(item), value);
        }
        else if (lv_streq("inner_align", name)) { //lv_image_set_inner_align(item, image_align_to_enum(value));
            lvgen_new_exprinsn(fn, "lv_image_set_inner_align(%s, %s);",
                LV_OBJNAME(item), image_align_to_enum(value));
        }
        else if (lv_streq("rotation", name)) {//lv_image_set_rotation(item, lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_image_set_rotation(%s, %s);",
                LV_OBJNAME(item), lv_xml_atoi_string(value));
        }
        else if (lv_streq("scale_x", name)) { //lv_image_set_scale_x(item, lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_image_set_scale_x(%s, %s);",
                LV_OBJNAME(item), lv_xml_atoi_string(value));
        }
        else if (lv_streq("scale_y", name)) { //lv_image_set_scale_y(item, lv_xml_atoi(value));
            lvgen_new_exprinsn(fn, "lv_image_set_scale_y(%s, %s);",
                LV_OBJNAME(item), lv_xml_atoi_string(value));
        }
        else if (lv_streq("pivot", name)) {
            int32_t x = lv_xml_atoi_split(&value, ' ');
            int32_t y = lv_xml_atoi_split(&value, ' ');

            lvgen_new_exprinsn(fn, "lv_image_set_pivot(%s, %d, %d);",
                LV_OBJNAME(item), x, y);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_image_align_t image_align_to_enum(const char * txt)
{
    char* pv = (char *)"0";

    if (!lvgen_cc_find_sym("lv_image_align_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for image align", txt);

    return pv;
}


#endif /* LV_USE_XML */
