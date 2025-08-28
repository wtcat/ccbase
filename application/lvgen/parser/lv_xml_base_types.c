/**
 * @file lv_xml_base_parser.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#if LV_USE_XML

#include "lvgen_cinsn.h"

#include "lv_xml_base_types.h"
#include "lv_xml_private.h"
#include "lv_xml_parser.h"
#include "lv_xml_style.h"
#include "lv_xml_component_private.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_state_t lv_xml_state_to_enum(const char * txt)
{
    char* pv = (char *)"LV_STATE_DEFAULT";

    if (!lvgen_cc_find_sym("lv_state_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for state", txt);

    return pv;
}

int lv_xml_to_size_int(const char * txt)
{
    if(lv_streq(txt, "content")) 
        return LV_SIZE_CONTENT;
    
    int32_t v = lv_xml_atoi(txt);
    if (txt[lv_strlen(txt) - 1] == '%')
        return LV_PCT(v);

    return v;
}

const char* lv_xml_to_size(const char* txt)
{
    static char buf[48];

    if (lv_streq(txt, "content"))
        return "LV_SIZE_CONTENT";

    int32_t v = lv_xml_atoi(txt);
    if (txt[lv_strlen(txt) - 1] == '%')
        snprintf(buf, sizeof(buf), "lv_pct(%d)", v);
    else
        snprintf(buf, sizeof(buf), "%d", v);
    return buf;
}

lv_align_t lv_xml_align_to_enum(const char * txt)
{
    char* pv = (char*)"LV_ALIGN_TOP_LEFT";

    if (!lvgen_cc_find_sym("lv_align_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for align", txt);

    return pv;
}

lv_dir_t lv_xml_dir_to_enum(const char * txt)
{
    char* pv = (char*)"LV_DIR_NONE";

    if (!lvgen_cc_find_sym("lv_dir_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for dir", txt);

    return pv;
}

lv_border_side_t lv_xml_border_side_to_enum(const char * txt)
{
    char* pv = (char*)"LV_BORDER_SIDE_NONE";

    if (!lvgen_cc_find_sym("lv_border_side_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for border_side", txt);

    return pv;
}

lv_grad_dir_t lv_xml_grad_dir_to_enum(const char * txt)
{
    char* pv = (char*)"LV_GRAD_DIR_NONE";

    if (!lvgen_cc_find_sym("lv_grad_dir_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for grad_dir", txt);

    return pv;
}

lv_base_dir_t lv_xml_base_dir_to_enum(const char * txt)
{
    char* pv = (char*)"LV_BASE_DIR_AUTO";

    if (!lvgen_cc_find_sym("lv_base_dir_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for base_dir", txt);

    return pv;
}

lv_text_align_t lv_xml_text_align_to_enum(const char * txt)
{
    char* pv = (char*)"LV_TEXT_ALIGN_AUTO";

    if (!lvgen_cc_find_sym("lv_text_align_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for text_align", txt);

    return pv;
}

lv_text_decor_t lv_xml_text_decor_to_enum(const char * txt)
{
    char* pv = (char*)"0";

    if (!lvgen_cc_find_sym("lv_text_decor_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for text_decor", txt);

    return pv;
}

lv_flex_flow_t lv_xml_flex_flow_to_enum(const char * txt)
{
    char* pv = (char *)"LV_TEXT_DECOR_NONE";

    if (!lvgen_cc_find_sym("lv_flex_flow_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for flex_flow", txt);

    return pv;
}

lv_flex_align_t lv_xml_flex_align_to_enum(const char * txt)
{
    char* pv = (char *)"LV_FLEX_ALIGN_CENTER";

    if (!lvgen_cc_find_sym("lv_flex_align_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for flex_align", txt);

    return pv;
}

lv_grid_align_t lv_xml_grid_align_to_enum(const char * txt)
{
    char* pv = (char*)"LV_GRID_ALIGN_CENTER";

    if (!lvgen_cc_find_sym("lv_grid_align_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for grid_align", txt);

    return pv;
}


lv_layout_t lv_xml_layout_to_enum(const char * txt)
{
    char* pv = (char*)"LV_LAYOUT_NONE";

    if (!lvgen_cc_find_sym("lv_layout_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for layout", txt);

    return pv;
}

lv_blend_mode_t lv_xml_blend_mode_to_enum(const char * txt)
{
    char* pv = (char*)"LV_BLEND_MODE_NORMAL";

    if (!lvgen_cc_find_sym("lv_blend_mode_t", txt, &pv, NULL))
        LV_LOG_WARN("%s is an unknown value for blend_mode", txt);

    return pv;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* LV_USE_XML */
