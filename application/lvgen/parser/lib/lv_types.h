/*
 * Copyright 2025 wtcat
 */

#ifndef LIB_LV_TYPES_H_
#define LIB_LV_TYPES_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif
#include "lv_ll.h"

/**********************
 *      MACROS
 **********************/
#define LV_FN_PREFIX  "ui_lvgen__"
#define LV_VFN_PARAM2 "priv"
#define LV_VFN_STYLE_AT(n)  "&" LV_VFN_PARAM2 "->styles[" #n "]"


#define LV_LOG_WARN(fmt, ...) printf("%s:" fmt "\n", __func__, ##__VA_ARGS__)
#define LV_LOG_ERROR LV_LOG_WARN
#define LV_LOG_INFO(...)
#define LV_UNUSED(x) (void)(x)


#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LV_MIN3(a, b, c) (LV_MIN(LV_MIN(a,b), c))
#define LV_MIN4(a, b, c, d) (LV_MIN(LV_MIN(a,b), LV_MIN(c,d)))

#define LV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LV_MAX3(a, b, c) (LV_MAX(LV_MAX(a,b), c))
#define LV_MAX4(a, b, c, d) (LV_MAX(LV_MAX(a,b), LV_MAX(c,d)))

#define LV_CLAMP(min, val, max) (LV_MAX(min, (LV_MIN(val, max))))

#define LV_ABS(x) ((x) > 0 ? (x) : (-(x)))
#define LV_UDIV255(x) (((x) * 0x8081U) >> 0x17)

#define LV_IS_SIGNED(t) (((t)(-1)) < ((t)0))
#define LV_UMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0xFULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define LV_SMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define LV_MAX_OF(t) ((unsigned long)(LV_IS_SIGNED(t) ? LV_SMAX_OF(t) : LV_UMAX_OF(t)))



#define LV_COORD_TYPE_SHIFT    (29U)
#define LV_COORD_TYPE_MASK     (3 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE(x)       ((x) & LV_COORD_TYPE_MASK)  /*Extract type specifiers*/
#define LV_COORD_PLAIN(x)      ((x) & ~LV_COORD_TYPE_MASK) /*Remove type specifiers*/

#define LV_COORD_TYPE_PX       (0 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_SPEC     (1 << LV_COORD_TYPE_SHIFT)
#define LV_COORD_TYPE_PX_NEG   (3 << LV_COORD_TYPE_SHIFT)

#define LV_COORD_IS_PX(x)       (LV_COORD_TYPE(x) == LV_COORD_TYPE_PX || LV_COORD_TYPE(x) == LV_COORD_TYPE_PX_NEG)
#define LV_COORD_IS_SPEC(x)     (LV_COORD_TYPE(x) == LV_COORD_TYPE_SPEC)

#define LV_COORD_SET_SPEC(x)    ((x) | LV_COORD_TYPE_SPEC)

     /** Max coordinate value */
#define LV_COORD_MAX            ((1 << LV_COORD_TYPE_SHIFT) - 1)
#define LV_COORD_MIN            (-LV_COORD_MAX)

/*Special coordinates*/
#define LV_SIZE_CONTENT         LV_COORD_SET_SPEC(LV_COORD_MAX)
#define LV_PCT_STORED_MAX       (LV_COORD_MAX - 1)
#if LV_PCT_STORED_MAX % 2 != 0
#error LV_PCT_STORED_MAX should be an even number
#endif
#define LV_PCT_POS_MAX          (LV_PCT_STORED_MAX / 2)
#define LV_PCT(x)               (LV_COORD_SET_SPEC(((x) < 0 ? (LV_PCT_POS_MAX - LV_MAX((x), -LV_PCT_POS_MAX)) : LV_MIN((x), LV_PCT_POS_MAX))))
#define LV_COORD_IS_PCT(x)      ((LV_COORD_IS_SPEC(x) && LV_COORD_PLAIN(x) <= LV_PCT_STORED_MAX))
#define LV_COORD_GET_PCT(x)     (LV_COORD_PLAIN(x) > LV_PCT_POS_MAX ? LV_PCT_POS_MAX - LV_COORD_PLAIN(x) : LV_COORD_PLAIN(x))


struct _lv_xml_parser_state_t;
typedef struct _lv_xml_parser_state_t lv_xml_parser_state_t;

struct _lv_xml_component_scope_t;
typedef struct _lv_xml_component_scope_t lv_xml_component_scope_t;

#define LV_RESULT_INVALID (-1)
#define LV_RESULT_OK (0)
typedef int lv_result_t;

typedef void * lv_event_cb_t; //TODO
typedef const char* lv_color_t; //TODO
typedef uint8_t lv_opa_t; //TODO

typedef const char* lv_style_prop_t;
typedef const char* lv_state_t;
typedef const char* lv_align_t;
typedef const char* lv_dir_t;
typedef const char* lv_border_side_t;
typedef const char* lv_base_dir_t;
typedef const char* lv_grad_dir_t;
typedef const char* lv_text_align_t;
typedef const char* lv_text_decor_t;
typedef const char* lv_flex_flow_t;
typedef const char* lv_flex_align_t;
typedef const char* lv_grid_align_t;
typedef const char* lv_layout_t;
typedef const char* lv_blend_mode_t;
typedef const char* lv_part_t;
typedef const char* lv_style_selector_t;
typedef const char* lv_obj_flag_t;
typedef const char* lv_label_long_mode_t;
typedef char lv_font_t;

typedef char* lv_style_t;

#define LV_MAX_VNAME 64
typedef struct {
	char name[LV_MAX_VNAME];
} lv_base_t;

typedef struct {
#define LV_OBJNAME(_obj) ((lv_obj_t *)(_obj))->base.name
	lv_base_t base;
    void* scope_fn;
} lv_obj_t;

/*
 * OPA 
 */
enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_0 = 0,
    LV_OPA_10 = 25,
    LV_OPA_20 = 51,
    LV_OPA_30 = 76,
    LV_OPA_40 = 102,
    LV_OPA_50 = 127,
    LV_OPA_60 = 153,
    LV_OPA_70 = 178,
    LV_OPA_80 = 204,
    LV_OPA_90 = 229,
    LV_OPA_100 = 255,
    LV_OPA_COVER = 255,
};

/**
 * Represents a point on the screen.
 */
typedef struct {
    int32_t x;
    int32_t y;
} lv_point_t;


/**
 * Values for lv_subject_t's `type` field
 */
typedef enum {
    LV_SUBJECT_TYPE_INVALID = 0,   /**< indicates Subject not initialized yet */
    LV_SUBJECT_TYPE_NONE = 1,   /**< a null value like None or NILt */
    LV_SUBJECT_TYPE_INT = 2,   /**< an int32_t */
    LV_SUBJECT_TYPE_POINTER = 3,   /**< a void pointer */
    LV_SUBJECT_TYPE_COLOR = 4,   /**< an lv_color_t */
    LV_SUBJECT_TYPE_GROUP = 5,   /**< an array of Subjects */
    LV_SUBJECT_TYPE_STRING = 6,   /**< a char pointer */
} lv_subject_type_t;

/**
 * A common type to handle all the various observable types in the same way
 */
typedef union {
    int32_t num;           /**< Integer number (opacity, enums, booleans or "normal" numbers) */
    const void* pointer;  /**< Constant pointer  (string buffer, format string, font, cone text, etc.) */
    lv_color_t color;      /**< Color */
} lv_subject_value_t;

/**
 * The Subject (an observable value)
 */
typedef struct {
    lv_ll_t subs_ll;                     /**< Subscribers */
    lv_subject_value_t value;            /**< Current value */
    lv_subject_value_t prev_value;       /**< Previous value */
    void* user_data;                    /**< Additional parameter, can be used freely by user */
    uint32_t type : 4;  /**< One of the LV_SUBJECT_TYPE_... values */
    uint32_t size : 24;  /**< String buffer size or group length */
    uint32_t notify_restart_query : 1;  /**< If an Observer was deleted during notifcation,
                                          * start notifying from the beginning. */
} lv_subject_t;

typedef struct {
    char reserved;
} lv_observer_t;

/**
 * The direction of the gradient.
 */
//typedef enum {
//    LV_GRAD_DIR_NONE,       /**< No gradient (the `grad_color` property is ignored)*/
//    LV_GRAD_DIR_VER,        /**< Simple vertical (top to bottom) gradient*/
//    LV_GRAD_DIR_HOR,        /**< Simple horizontal (left to right) gradient*/
//    LV_GRAD_DIR_LINEAR,     /**< Linear gradient defined by start and end points. Can be at any angle.*/
//    LV_GRAD_DIR_RADIAL,     /**< Radial gradient defined by start and end circles*/
//    LV_GRAD_DIR_CONICAL,    /**< Conical gradient defined by center point, start and end angles*/
//} lv_grad_dir_t;

/**
 * Gradient behavior outside the defined range.
 */
typedef enum {
    LV_GRAD_EXTEND_PAD,     /**< Repeat the same color*/
    LV_GRAD_EXTEND_REPEAT,  /**< Repeat the pattern*/
    LV_GRAD_EXTEND_REFLECT, /**< Repeat the pattern mirrored*/
} lv_grad_extend_t;

/** A gradient stop definition.
 *  This matches a color and a position in a virtual 0-255 scale.
 */
typedef struct {
    lv_color_t color;   /**< The stop color */
    lv_opa_t   opa;     /**< The opacity of the color*/
    uint8_t    frac;    /**< The stop position in 1/255 unit */
} lv_grad_stop_t;

/** A descriptor of a gradient. */
#ifndef LV_GRADIENT_MAX_STOPS
#define LV_GRADIENT_MAX_STOPS   2
#endif
typedef struct {
    lv_grad_stop_t   stops[LV_GRADIENT_MAX_STOPS];  /**< A gradient stop array */
    uint8_t          stops_count;                   /**< The number of used stops in the array */
    lv_grad_dir_t    dir;                       /**< The gradient direction.
                                                         * Any of LV_GRAD_DIR_NONE, LV_GRAD_DIR_VER, LV_GRAD_DIR_HOR,
                                                         * LV_GRAD_TYPE_LINEAR, LV_GRAD_TYPE_RADIAL, LV_GRAD_TYPE_CONICAL */
    lv_grad_extend_t     extend : 3;                    /**< Behaviour outside the defined range.
                                                         * LV_GRAD_EXTEND_NONE, LV_GRAD_EXTEND_PAD, LV_GRAD_EXTEND_REPEAT, LV_GRAD_EXTEND_REFLECT */
    union {
        /*Linear gradient parameters*/
        struct {
            lv_point_t  start;                          /**< Linear gradient vector start point */
            lv_point_t  end;                            /**< Linear gradient vector end point */
        } linear;
        /*Radial gradient parameters*/
        struct {
            lv_point_t  focal;                          /**< Center of the focal (starting) circle in local coordinates */
            /* (can be the same as the ending circle to create concentric circles) */
            lv_point_t  focal_extent;                   /**< Point on the circle (can be the same as the center) */
            lv_point_t  end;                            /**< Center of the ending circle in local coordinates */
            lv_point_t  end_extent;                     /**< Point on the circle determining the radius of the gradient */
        } radial;
        /*Conical gradient parameters*/
        struct {
            lv_point_t  center;                         /**< Conical gradient center point */
            int16_t     start_angle;                    /**< Start angle 0..3600 */
            int16_t     end_angle;                      /**< End angle 0..3600 */
        } conical;
    } params;
    void* state;
} lv_grad_dsc_t;

#ifdef __cplusplus
}
#endif
#endif /* LIB_LV_TYPES_H_ */
