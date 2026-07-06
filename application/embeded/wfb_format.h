/*
 * Copyright 2026 wtcat. WatchFace Binary (WFB) format, normative definitions.
 */
#ifndef WFB_FORMAT_H
#define WFB_FORMAT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WFB_MAGIC0 'W'
#define WFB_MAGIC1 'F'
#define WFB_MAGIC2 'B'
#define WFB_MAGIC3 '1'
#define WFB_VERSION 1u

/* Header flags */
#define WFB_FLAG_HAS_DBG_IDS (1u << 0) /* optional debug widget-id names present */

#define WF_REF_NONE 0xFFFFu	   /* "no reference" for uint16 index fields */
#define WF_PARENT_ROOT 0xFFFFu /* widget parent = screen root            */

#define WF_SIZE_RES ((int16_t)-1)	  /* use the referenced resource's native size */
#define WF_SIZE_CONTENT ((int16_t)-2) /* LV_SIZE_CONTENT                            */

enum wf_widget_type {
	WF_W_SCREEN = 0,	 /* root / background container (lv_obj)          */
	WF_W_IMAGE = 1,		 /* lv_image;      res_ref = image index          */
	WF_W_LABEL = 2,		 /* lv_label;      res_ref = font index, extra=str*/
	WF_W_FRAME_ANIM = 3, /* lv_animimg;    res_ref = anim index           */
	WF_W_ARC = 4,		 /* lv_arc                                        */
	WF_W_BAR = 5,		 /* lv_bar                                        */
	WF_W_CONTAINER = 6,	 /* plain lv_obj                                  */
	WF_W_IMGLABEL = 7,	 /* image label; res_ref = image index (the       */
						 /* referenced virtual image is a ResFile group,  */
						 /* one member per glyph — e.g. digit tiles).     */
						 /* If WF_WF_DYNTEXT is set, extra -> wf_imgtext_t */
						 /* (a runtime-driven value); else the whole group*/
						 /* is shown statically.                          */
};

/* Widget flags (wf_widget_t.flags) */
#define WF_WF_HIDDEN (1u << 0)
#define WF_WF_CLICKABLE (1u << 1)
#define WF_WF_DYNTEXT (1u << 2) /* IMGLABEL: extra -> wf_imgtext_t in strings */
#define WF_WF_RECOLOR (1u << 3) /* LABEL: enable #RRGGBB# inline recolor markup */
#define WF_WF_FLEX (1u << 4)	/* SCREEN/CONTAINER: extra -> wf_flex_t in strings */

/* Alignment (wf_widget_t.align) — the Loader maps these to lv_align_t.     */
/* Order MUST match the "align" enum in watchface.schema.json.             */
enum wf_align {
	WF_ALIGN_CENTER = 0,
	WF_ALIGN_TOP_LEFT,
	WF_ALIGN_TOP_MID,
	WF_ALIGN_TOP_RIGHT,
	WF_ALIGN_BOTTOM_LEFT,
	WF_ALIGN_BOTTOM_MID,
	WF_ALIGN_BOTTOM_RIGHT,
	WF_ALIGN_LEFT_MID,
	WF_ALIGN_RIGHT_MID,
};

/* Flex layout (wf_flex_t) — the Loader maps these to LVGL's lv_flex_* enums.  */
/* Order MUST match the "flexFlow"/"flexAlign" enums in watchface.schema.json. */
enum wf_flex_flow {
	WF_FLEX_ROW = 0,
	WF_FLEX_COLUMN,
	WF_FLEX_ROW_WRAP,
	WF_FLEX_COLUMN_WRAP,
	WF_FLEX_ROW_REVERSE,
	WF_FLEX_COLUMN_REVERSE,
};

enum wf_flex_align {
	WF_FLEX_START = 0,
	WF_FLEX_END,
	WF_FLEX_CENTER,
	WF_FLEX_SPACE_EVENLY,
	WF_FLEX_SPACE_AROUND,
	WF_FLEX_SPACE_BETWEEN,
};

/* Event code (wf_event_t.code) — the Loader maps these to lv_event_code_t.  */
/* Order MUST match the "eventCode" enum in watchface.schema.json.          */
enum wf_event_code {
	WF_EV_CLICKED = 0,
	WF_EV_PRESSED,
	WF_EV_RELEASED,
	WF_EV_LONG_PRESSED,
	WF_EV_VALUE_CHANGED,
};

/* Image flags (wf_image_t.flags) — reserved; the ResFile entry itself carries
 * pixel format / compression / group-ness, so no per-image flag is needed today. */

typedef struct {
	char magic[4];	  /* 'W','F','B','1' */
	uint16_t version; /* WFB_VERSION */
	uint16_t flags;	  /* WFB_FLAG_*                             */

	uint16_t screen_w;
    uint16_t screen_h;

	uint16_t widget_count;
	uint16_t image_count;
	uint16_t font_count;
	uint16_t anim_count;
	uint16_t event_count;
	uint16_t style_count;
	uint16_t reserved0;
	uint16_t reserved1;

	/* All offsets are relative to the START OF THE LAYOUT SECTION,      */
	/* which itself begins immediately after this header. The whole file */
	/* is header + layout section only; resources live externally in a   */
	/* ResFile (images/anims) or are firmware-provided (fonts).          */
	uint32_t off_widgets;
	uint32_t off_images;
	uint32_t off_fonts;
	uint32_t off_anims;
	uint32_t off_events;
	uint32_t off_styles;
	uint32_t off_strings; /* label text pool (always present if labels exist) */

	uint32_t layout_size; /* size of the layout section (for crc)    */
	uint32_t crc32;		  /* CRC-32 of the layout section            */
} wf_header_t;


/* 
 * Resource tables 
 */
typedef struct {
	uint32_t namekey; /* re_name_hash of image name (ResFile lookup) */
	uint8_t flags;	  /* reserved (0)                                */
	uint8_t reserved[3];
} wf_image_t;

typedef struct {
	uint32_t namekey; /* re_name_hash of font name (firmware lookup) */
	uint32_t reserved;
} wf_font_t;

typedef struct {
	uint8_t type;		  /* enum wf_widget_type                         */
	uint8_t align;		  /* lv_align_t                                  */
	uint16_t parent;	  /* widget index; WF_PARENT_ROOT for screen     */
	int16_t x, y;		  /* offset within alignment                     */
	int16_t w, h;		  /* size; WF_SIZE_RES / WF_SIZE_CONTENT special */
	uint16_t res_ref;	  /* image/font/anim index by type; WF_REF_NONE  */
	uint16_t style_ref;	  /* style index; WF_REF_NONE                    */
	uint16_t event_start; /* first event index in event table            */
	uint8_t event_count;
	uint8_t flags;	/* WF_WF_*                                     */
	uint32_t extra; /* type-specific: LABEL -> string table offset */
} wf_widget_t;

typedef struct {
	uint32_t namekey;	  /* re_name_hash of the group (virtual image)   */
	uint16_t duration_ms; /* one full cycle (lv_animimg_set_duration)    */
	uint8_t repeat;		  /* 0 = infinite, else count                    */
	uint8_t flags;		  /* reserved                                    */
} wf_anim_t;

/*
 * Dynamic imglabel binding. Emitted 4-byte-aligned INTO THE STRINGS POOL;
 * wf_widget_t.extra is its offset (relative to off_strings) when the widget's
 * WF_WF_DYNTEXT flag is set. Immediately followed by glyph_cnt char bytes: the
 * character each atlas glyph displays (glyphs[i] == char shown by atlas glyph i).
 * The Loader fetches the current value string via wf_env_t.get_text(provider_hash),
 * maps each char through glyphs -> atlas index, and calls lvgl_imglabel_set_text.
 */
typedef struct {
	uint32_t provider_hash; /* re_name_hash of the firmware provider name  */
	uint8_t glyph_cnt;		/* number of glyph chars that follow (== atlas)*/
	uint8_t max_len;		/* max displayed chars (index-buffer bound)    */
	uint8_t reserved[2];
	/* char glyphs[glyph_cnt] follows */
} wf_imgtext_t;

/*
 * Flex layout for a SCREEN/CONTAINER widget. Emitted 4-byte-aligned INTO THE
 * STRINGS POOL; wf_widget_t.extra is its offset (relative to off_strings) when
 * the widget's WF_WF_FLEX flag is set. Followed by item_cnt grow bytes — one per
 * child, in child creation order (== widget-table order among siblings): grow[i]
 * is lv_obj_set_flex_grow for the i-th child (0 = no grow). The Loader sets
 * lv_obj_set_flex_flow/align on the container and applies grow[] to each child.
 */
typedef struct {
	uint8_t flow;		 /* enum wf_flex_flow  */
	uint8_t main_align;	 /* enum wf_flex_align */
	uint8_t cross_align; /* enum wf_flex_align */
	uint8_t track_align; /* enum wf_flex_align */
	uint8_t item_cnt;	 /* == container child count; grow[] length */
	uint8_t reserved[3];
	/* uint8_t grow[item_cnt] follows */
} wf_flex_t;


/* Event binding */
typedef struct {
	uint8_t code; /* lv_event_code_t subset (see spec)           */
	uint8_t reserved;
	uint16_t widget;  /* owning widget index                         */
	uint32_t cb_hash; /* re_name_hash of firmware callback name       */
	uint32_t param;	  /* opaque integer passed to callback           */
} wf_event_t;


/* Style (shared, compact) */
typedef struct {
	uint32_t text_color; /* 0x00RRGGBB */
	uint32_t bg_color;	 /* 0x00RRGGBB */
	uint16_t bg_opa;	 /* 0..255 */
	uint16_t arc_width;
	uint32_t arc_color;			/* 0x00RRGGBB */
	uint32_t image_recolor;		/* 0x00RRGGBB (image tint)  */
	uint16_t image_recolor_opa; /* 0..255; 0 = off          */
	uint16_t reserved;
} wf_style_t;

struct _lv_event_t;
typedef void (*wf_event_cb_t)(struct _lv_event_t *e, uint32_t param);

static inline 
#if defined(__GNUC__) || defined(__clang__)
__attribute__((const))
#endif
uint32_t wf_name_hash(const char *s, uint32_t len) {
	uint32_t h = 0;
	for (uint32_t i = 0; i < len; i++) {
		h *= 16777619u;
		h ^= (uint8_t)s[i];
	}
	return h;
}

#ifdef __cplusplus
}
#endif

#endif /* WFB_FORMAT_H */
