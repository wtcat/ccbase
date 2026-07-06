/*
 * wf_loader — WFB runtime loader implementation.
 */
#include <stdlib.h>
#include <string.h>

#define WF_USE_LAZYDECOMP 1

#include "embeded/wf_loader.h"
#include "embeded/resource/resource_lvgl.h"
#include "embeded/widget/lvgl_imglabel.h"


#define WF_MAX_REG     (16)
#define WF_MAX_IMGTEXT (16) /* max displayed chars per dynamic imglabel */

#ifndef ROUND_UP
#define ROUND_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#endif
#define ROUND_UP_PTR(x) ROUND_UP(x, sizeof(void *))

#if WF_USE_LAZYDECOMP
# define WF_IMAGE_ESIZE (sizeof(lv_image_dsc_t) + sizeof(struct image_decomp))
#else
# define WF_IMAGE_ESIZE (sizeof(lv_image_dsc_t))
#endif

typedef struct {
	wf_event_cb_t cb;
	uint32_t param;
} wf_evbind_t;

typedef struct {
	uint32_t hash;
	wf_event_cb_t cb;
} wf_evnode_t;

/* One per dynamic (WF_WF_DYNTEXT) imglabel; drives wf_refresh. */
typedef struct {
	lv_obj_t *obj;
	uint32_t provider_hash;
	const char *glyphs; /* into the read-only wfb strings section (zero-copy) */
	uint8_t *idxbuf;	/* currently-displayed indices (malloc'd + tracked)   */
	uint8_t glyph_cnt;
	uint8_t max_len;
	uint8_t cur_cnt;	/* number of indices currently displayed (change det) */
} wf_dynbind_t;

struct wf_instance {
	lv_obj_t *screen;

	/* growable list of malloc'd blocks to RE_FREE on unload (image blobs, dsc
	 * arrays, frame pointer arrays) — compact vector, not a linked list. */
	void **allocs;
	int n_alloc, cap_alloc;

	/* both point into the trailing bytes of this same block — never RE_FREE()d
	 * separately (see wf_load's single-block RE_CALLOC) */
	lv_font_t **font_cache; /* [font_count], lazily resolved             */
	wf_evbind_t *binds;		/* [event_count], resolved at load time      */
	void*  image_array;
	
	/* dynamic imglabel bindings — also in the single block (after image_array) */
	wf_dynbind_t *dyn;
	uint8_t dyn_count;
	uint8_t (*get_text)(uint32_t provider_hash, char *buf, uint8_t cap);

	/* read-only views into the wfb buffer */
	const wf_widget_t *widgets;
	const wf_image_t *images;
	const wf_font_t *fonts;
	const wf_anim_t *anims;
	const wf_event_t *events;
	const wf_style_t *styles;
	const char *strings;
	const wf_header_t *hdr;
};


static wf_evnode_t evnode_array[WF_MAX_REG];
static int evnode_count;


static inline void* image_array_at(void *array, size_t idx) {
	return (void *)((char*)array + WF_IMAGE_ESIZE * idx);
}

static uint32_t crc32_calc(const uint8_t *data, uint32_t len) {
	return re_crc32_update(0, data, len);
}

void wf_register_event(const char *name, wf_event_cb_t cb) {
	uint32_t h = re_name_hash((const uint8_t *)name, (uint32_t)strlen(name));
	for (int i = 0; i < evnode_count; i++)
		if (evnode_array[i].hash == h) {
			evnode_array[i].cb = cb;
			return;
		}
	if (evnode_count < WF_MAX_REG) {
		evnode_array[evnode_count].hash = h;
		evnode_array[evnode_count].cb = cb;
		evnode_count++;
	}
}

wf_event_cb_t wf_lookup_event(uint32_t cb_hash) {
	for (int i = 0; i < evnode_count; i++)
		if (evnode_array[i].hash == cb_hash)
			return evnode_array[i].cb;
	return NULL;
}

static int track(wf_instance_t *in, void *p) {
	if (!p)
		return -1;
	if (in->n_alloc == in->cap_alloc) {
		int nc = in->cap_alloc ? in->cap_alloc * 2 : 16;
		void **na = RE_REALLOC(in->allocs, (size_t)nc * sizeof(void *));
		if (!na)
			return -1;
		in->allocs = na;
		in->cap_alloc = nc;
	}
	in->allocs[in->n_alloc++] = p;
	return 0;
}

static lv_align_t map_align(uint8_t a) {
	static const lv_align_t m[] = {
		LV_ALIGN_CENTER,
        LV_ALIGN_TOP_LEFT,
        LV_ALIGN_TOP_MID,
		LV_ALIGN_TOP_RIGHT,
        LV_ALIGN_BOTTOM_LEFT,
        LV_ALIGN_BOTTOM_MID,
		LV_ALIGN_BOTTOM_RIGHT,
        LV_ALIGN_LEFT_MID,
        LV_ALIGN_RIGHT_MID,
	};
	return (a < sizeof(m) / sizeof(m[0])) ? m[a] : LV_ALIGN_TOP_LEFT;
}

static lv_event_code_t map_event(uint8_t c) {
	static const lv_event_code_t m[] = {
		LV_EVENT_CLICKED,
        LV_EVENT_PRESSED,
        LV_EVENT_RELEASED,
		LV_EVENT_LONG_PRESSED,
        LV_EVENT_VALUE_CHANGED,
	};
	return (c < sizeof(m) / sizeof(m[0])) ? m[c] : LV_EVENT_CLICKED;
}

static lv_flex_flow_t map_flow(uint8_t f) {
	static const lv_flex_flow_t m[] = {
		LV_FLEX_FLOW_ROW,		 LV_FLEX_FLOW_COLUMN,	   LV_FLEX_FLOW_ROW_WRAP,
		LV_FLEX_FLOW_COLUMN_WRAP, LV_FLEX_FLOW_ROW_REVERSE, LV_FLEX_FLOW_COLUMN_REVERSE,
	};
	return (f < sizeof(m) / sizeof(m[0])) ? m[f] : LV_FLEX_FLOW_ROW;
}

static lv_flex_align_t map_falign(uint8_t a) {
	static const lv_flex_align_t m[] = {
		LV_FLEX_ALIGN_START,		LV_FLEX_ALIGN_END,			LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_SPACE_BETWEEN,
	};
	return (a < sizeof(m) / sizeof(m[0])) ? m[a] : LV_FLEX_ALIGN_START;
}

#if WF_USE_LAZYDECOMP == 0
static void fill_image_dsc(lv_image_dsc_t *dsc, const struct refile_data *d) {
	memset(dsc, 0, sizeof(*dsc));
	dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
	dsc->header.cf = (uint8_t)re_map_colorfmt(d->format);
	dsc->header.w = (uint16_t)d->width;
	dsc->header.h = (uint16_t)d->height;
	dsc->data_size = d->size;
	dsc->data = (const uint8_t *)d->data;
	/* NOTE: LZ4-compressed entries (d->compress) are not decoded here yet. */
}

static int load_image_dsc(wf_instance_t *in, const re_file_t *res,
						uint32_t namekey, lv_image_dsc_t* odsc) {
	struct refile_data *blob = NULL;
	if (re_load_image(res, namekey, &blob) != 0 || !blob)
		return -ENODATA;
	if (track(in, blob) != 0) {
		RE_FREE(blob);
		return -ENOMEM;
	}

	fill_image_dsc(odsc, blob);
	return 0;
}
#endif /* WF_USE_LAZYDECOMP == 1 */

static void apply_style(lv_obj_t *obj, const wf_style_t *s) {
	lv_obj_set_style_text_color(obj, lv_color_hex(s->text_color), 0);
	if (s->bg_opa > 0) {
		lv_obj_set_style_bg_color(obj, lv_color_hex(s->bg_color), 0);
		lv_obj_set_style_bg_opa(obj, (lv_opa_t)s->bg_opa, 0);
	}
	if (s->arc_width > 0) {
		lv_obj_set_style_arc_width(obj, s->arc_width, 0);
		lv_obj_set_style_arc_color(obj, lv_color_hex(s->arc_color), 0);
	}
	if (s->image_recolor_opa > 0) {
		lv_obj_set_style_image_recolor(obj, lv_color_hex(s->image_recolor), 0);
		lv_obj_set_style_image_recolor_opa(obj, (lv_opa_t)s->image_recolor_opa, 0);
	}
}

/* Fetch a dynamic imglabel's current value and push it as glyph indices. */
static void refresh_one(wf_instance_t *in, wf_dynbind_t *b) {
	if (!in->get_text)
		return;

	char tmp[WF_MAX_IMGTEXT];
	uint8_t cap = b->max_len < WF_MAX_IMGTEXT ? b->max_len : WF_MAX_IMGTEXT;
	uint8_t n = in->get_text(b->provider_hash, tmp, cap);
	if (n > cap)
		n = cap;

	uint8_t nb[WF_MAX_IMGTEXT]; /* new indices, staged before comparing */
	uint8_t m = 0;
	for (uint8_t i = 0; i < n; i++) {
		for (uint8_t g = 0; g < b->glyph_cnt; g++) {
			if (b->glyphs[g] == tmp[i]) {
				nb[m++] = g;
				break;
			}
		}
	}

	if (m == b->cur_cnt && memcmp(nb, b->idxbuf, m) == 0)
		return;

	memcpy(b->idxbuf, nb, m); /* m <= cap <= max_len, fits idxbuf */
	b->cur_cnt = m;
	lvgl_imglabel_set_text(b->obj, b->idxbuf, m);
}

static lv_obj_t *create_widget(wf_instance_t *in, const re_file_t *res,
							   const wf_env_t *env, const wf_widget_t *w,
							   lv_obj_t *parent) {
	lv_obj_t *obj = NULL;

	switch (w->type) {
	case WF_W_IMAGE: {
		obj = lv_image_create(parent);
		if (w->res_ref != WF_REF_NONE) {
			lv_image_dsc_t* dsc = image_array_at(in->image_array, w->res_ref);
#if WF_USE_LAZYDECOMP
			struct image_decomp* de = (struct image_decomp*)(dsc + 1);
			if (!re_image_prefetch(res, in->images[w->res_ref].namekey, dsc, de, 
				(re_read_t)re_read_dsc))
				lv_image_set_src(obj, dsc);
#else
			if (!load_image_dsc(in, res, in->images[w->res_ref].namekey, dsc))
				lv_image_set_src(obj, dsc);
#endif
		}
		break;
	}

	case WF_W_LABEL: {
		obj = lv_label_create(parent);
		lv_label_set_text_static(obj, in->strings + w->extra);
		if (w->flags & WF_WF_RECOLOR)
			lv_label_set_recolor(obj, true);
		if (w->res_ref != WF_REF_NONE) {
			lv_font_t *f = in->font_cache[w->res_ref];
			if (!f && env && env->get_font)
				f = in->font_cache[w->res_ref] =
					env->get_font(in->fonts[w->res_ref].namekey);
			if (f)
				lv_obj_set_style_text_font(obj, f, 0);
		}
		break;
	}

	case WF_W_FRAME_ANIM: {
		obj = lv_animimg_create(parent);
		if (w->res_ref != WF_REF_NONE) {
			const wf_anim_t *a = &in->anims[w->res_ref];
			re_group_t grp;
			if (re_load_group(res, a->namekey, &grp) == 0) {
				size_t n = re_group_size(&grp);
				/* One block holds both the pointer array lv_animimg_set_src
				 * needs and the contiguous dsc storage it points into — a
				 * single tracked alloc instead of one RE_MALLOC per frame. */
				void *block = n? RE_MALLOC(n * (sizeof(void *) + WF_IMAGE_ESIZE)) : NULL;
				if (block && track(in, block) == 0) {
					const void **srcs = (const void **)block;
					lv_image_dsc_t *dscs = (void *)(srcs + n);
					size_t got = 0;
					for (size_t i = 0; i < n; i++) {
#if WF_USE_LAZYDECOMP
						lv_image_dsc_t* imgdsc = image_array_at(dscs, i);
						if (re_image_prefetch(&grp, (uint32_t)i, imgdsc, (void*)(imgdsc + 1),
							(re_read_t)re_read_group_dsc))
							break;
						srcs[got] = imgdsc;
#else
						struct refile_data* blob = NULL;
						if (re_load_group_image(&grp, (uint32_t)i, &blob) != 0 || !blob)
							break;

						if (track(in, blob) != 0) {
							RE_FREE(blob);
							break;
						}
						fill_image_dsc(&dscs[got], blob);
						srcs[got] = &dscs[got];
#endif
						got++;
					}

					if (got) {
						lv_animimg_set_src(obj, srcs, got);
						lv_animimg_set_duration(obj, a->duration_ms);
						lv_animimg_set_repeat_count(obj, a->repeat? a->repeat: LV_ANIM_REPEAT_INFINITE);
						lv_animimg_start(obj);
					}
				} else {
					RE_FREE(block);
				}
				re_unload_group(&grp);
			}
		}
		break;
	}

	case WF_W_IMGLABEL: {
		obj = lvgl_imglabel_create(parent);
		if (w->res_ref != WF_REF_NONE) {
			/* the referenced (virtual) image is a group: one member per glyph.
			 * Build one contiguous lv_image_dsc_t array for lvgl_imglabel_set_src. */
			re_group_t grp;
			if (re_load_group(res, in->images[w->res_ref].namekey, &grp) == 0) {
				size_t n = re_group_size(&grp);
				lv_image_dsc_t* chars = n ? RE_MALLOC(n * WF_IMAGE_ESIZE) : NULL;
				if (chars && track(in, chars) == 0) {
#if WF_USE_LAZYDECOMP
					struct image_decomp* rds = (struct image_decomp*)(chars + n);
#endif
					size_t got = 0;
					for (size_t i = 0; i < n; i++) {
#if WF_USE_LAZYDECOMP
						if (re_image_prefetch(&grp, (uint32_t)i, &chars[i], &rds[i],
							(re_read_t)re_read_group_dsc))
							break;
#else
						struct refile_data* blob = NULL;
						if (re_load_group_image(&grp, (uint32_t)i, &blob) != 0 || !blob)
							break;
						if (track(in, blob) != 0) {
							RE_FREE(blob);
							break;
						}
						fill_image_dsc(&chars[got], blob);
#endif
						got++;
					}

					if (got)
						lvgl_imglabel_set_src(obj, chars, (uint8_t)got);
				}
				else {
					RE_FREE(chars);
				}
				re_unload_group(&grp);

				/* dynamic value: bind provider + glyph map, render initial value */
				if ((w->flags & WF_WF_DYNTEXT) && in->dyn) {
					const wf_imgtext_t* d = (const wf_imgtext_t*)(in->strings + w->extra);
					uint8_t* idxbuf = RE_MALLOC(d->max_len ? d->max_len : 1);
					if (idxbuf && track(in, idxbuf) == 0) {
						wf_dynbind_t* b = &in->dyn[in->dyn_count++];
						b->obj = obj;
						b->provider_hash = d->provider_hash;
						b->glyphs = (const char*)(d + 1);
						b->idxbuf = idxbuf;
						b->glyph_cnt = d->glyph_cnt;
						b->max_len = d->max_len;
						refresh_one(in, b);
					} else {
						RE_FREE(idxbuf);
					}
				}
			}
		}
		break;
	}

	case WF_W_ARC:
		obj = lv_arc_create(parent);
		break;
	case WF_W_BAR:
		obj = lv_bar_create(parent);
		break;
	case WF_W_SCREEN:
	case WF_W_CONTAINER:
	default:
		obj = lv_obj_create(parent);
		break;
	}

	if (!obj)
		return NULL;

	/* geometry */
	if (w->w != WF_SIZE_RES || w->h != WF_SIZE_RES) {
		int32_t cw = (w->w == WF_SIZE_CONTENT) ? LV_SIZE_CONTENT
					 : (w->w == WF_SIZE_RES)   ? lv_obj_get_width(obj)
											   : w->w;
		int32_t ch = (w->h == WF_SIZE_CONTENT) ? LV_SIZE_CONTENT
					 : (w->h == WF_SIZE_RES)   ? lv_obj_get_height(obj)
											   : w->h;
		lv_obj_set_size(obj, cw, ch);
	}
	lv_obj_align(obj, map_align(w->align), w->x, w->y);

	if (w->style_ref != WF_REF_NONE)
		apply_style(obj, &in->styles[w->style_ref]);
	if (w->flags & WF_WF_CLICKABLE)
		lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	if (w->flags & WF_WF_HIDDEN)
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

	/* flex container: set flow + align on itself */
	if (w->flags & WF_WF_FLEX) {
		const wf_flex_t *f = (const wf_flex_t *)(in->strings + w->extra);
		lv_obj_set_flex_flow(obj, map_flow(f->flow));
		lv_obj_set_flex_align(obj, map_falign(f->main_align), map_falign(f->cross_align),
							  map_falign(f->track_align));
	}
	/* flex child: pull this child's grow from the parent's blob by child order.
	 * The child was just appended to parent, so its index == child_count - 1. */
	if (w->parent != WF_PARENT_ROOT) {
		const wf_widget_t *p = &in->widgets[w->parent];
		if (p->flags & WF_WF_FLEX) {
			const wf_flex_t *pf = (const wf_flex_t *)(in->strings + p->extra);
			uint32_t ord = lv_obj_get_child_count(parent);
			if (ord)
				ord--;
			if (ord < pf->item_cnt) {
				uint8_t g = ((const uint8_t *)(pf + 1))[ord];
				if (g)
					lv_obj_set_flex_grow(obj, g);
			}
		}
	}

	return obj;
}

static void wf_trampoline(lv_event_t *e) {
	wf_evbind_t *b = (wf_evbind_t *)lv_event_get_user_data(e);
	if (b && b->cb)
		b->cb(e, b->param);
}

static int validate(const wf_header_t *h, uint32_t wfb_size) {
	if (wfb_size < sizeof(*h))
		return -1;

	if (h->magic[0] != WFB_MAGIC0 || h->magic[1] != WFB_MAGIC1 ||
		h->magic[2] != WFB_MAGIC2 || h->magic[3] != WFB_MAGIC3)
		return -1;

	if (h->version != WFB_VERSION)
		return -1;

	if ((uint64_t)sizeof(*h) + h->layout_size > wfb_size)
		return -1;

	const uint8_t *layout = (const uint8_t *)h + sizeof(*h);
	if (crc32_calc(layout, h->layout_size) != h->crc32)
		return -1;

	/* every table must lie within the layout section */
	struct {
		uint32_t off, count, sz;
	} t[] = {
		{h->off_widgets, h->widget_count, sizeof(wf_widget_t)},
		{h->off_images, h->image_count, sizeof(wf_image_t)},
		{h->off_fonts, h->font_count, sizeof(wf_font_t)},
		{h->off_anims, h->anim_count, sizeof(wf_anim_t)},
		{h->off_events, h->event_count, sizeof(wf_event_t)},
		{h->off_styles, h->style_count, sizeof(wf_style_t)},
	};
	for (unsigned i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
		uint64_t end = (uint64_t)t[i].off + (uint64_t)t[i].count * t[i].sz;
		if (end > h->layout_size)
			return -1;
	}
	if (h->off_strings > h->layout_size)
		return -1;
	return 0;
}

wf_instance_t *wf_load(const void *wfb, uint32_t wfb_size, const re_file_t *img_res,
					   const wf_env_t *env, void *screen) {
	if (!wfb || !screen)
		return NULL;

	const wf_header_t *h = (const wf_header_t *)wfb;
	if (validate(h, wfb_size) != 0)
		return NULL;

	const uint8_t *L = (const uint8_t *)wfb + sizeof(*h);

	/* Single instance block: header + font cache + event binds in one calloc.
	 * Both trailing arrays are pointer-aligned, so appending them after the
	 * (pointer-aligned) struct keeps natural alignment. calloc zeroes the tail,
	 * which the font cache relies on for lazy resolution. The objs[] scratch map
	 * stays a separate alloc — it is freed before wf_load returns. */
	const wf_widget_t *W = (const wf_widget_t *)(L + h->off_widgets);
	size_t n_dyn = 0;
	for (uint16_t i = 0; i < h->widget_count; i++)
		if (W[i].type == WF_W_IMGLABEL && (W[i].flags & WF_WF_DYNTEXT))
			n_dyn++;

	size_t fc_bytes = (size_t)h->font_count * ROUND_UP_PTR(sizeof(lv_font_t *));
	size_t bind_bytes = (size_t)h->event_count * ROUND_UP_PTR(sizeof(wf_evbind_t));
	size_t img_bytes = (size_t)h->image_count * ROUND_UP_PTR(WF_IMAGE_ESIZE);
	size_t dyn_bytes = n_dyn * ROUND_UP_PTR(sizeof(wf_dynbind_t));
	wf_instance_t *in = RE_CALLOC(1, sizeof(*in) + fc_bytes + bind_bytes + img_bytes + dyn_bytes);
	if (!in)
		return NULL;

	uint8_t *tail = (uint8_t *)in + sizeof(*in);
	in->font_cache = h->font_count ? (lv_font_t **)tail : NULL;
	in->binds = h->event_count ? (wf_evbind_t *)(tail + fc_bytes) : NULL;
	in->image_array = h->image_count ? (void *)(tail + fc_bytes + bind_bytes) : NULL;
	in->dyn = n_dyn ? (wf_dynbind_t *)(tail + fc_bytes + bind_bytes + img_bytes) : NULL;
	in->get_text = env ? env->get_text : NULL;

	in->screen = screen;
	in->hdr = h;
	in->widgets = (const wf_widget_t *)(L + h->off_widgets);
	in->images = (const wf_image_t *)(L + h->off_images);
	in->fonts = (const wf_font_t *)(L + h->off_fonts);
	in->anims = (const wf_anim_t *)(L + h->off_anims);
	in->events = (const wf_event_t *)(L + h->off_events);
	in->styles = (const wf_style_t *)(L + h->off_styles);
	in->strings = (const char *)(L + h->off_strings);

	/* scratch index->object map, freed before returning (steady-state RAM save) */
	lv_obj_t **objs = RE_CALLOC(h->widget_count ? h->widget_count : 1, sizeof(lv_obj_t *));
	if (!objs) {
		wf_unload(in, false);
		return NULL;
	}

	/* single linear pass — parents precede children (topological order) */
	for (uint16_t i = 0; i < h->widget_count; i++) {
		const wf_widget_t *w = &in->widgets[i];
		lv_obj_t *parent = (w->parent == WF_PARENT_ROOT)? screen:
							(w->parent < i ? objs[w->parent] : screen);
		lv_obj_t *obj = create_widget(in, img_res, env, w, parent);
		if (!obj) {
			RE_FREE(objs);
			wf_unload(in, false);
			return NULL;
		}

		objs[i] = obj;
		for (uint8_t e = 0; e < w->event_count; e++) {
			uint16_t ei = w->event_start + e;
			const wf_event_t *ev = &in->events[ei];
			wf_event_cb_t cb = wf_lookup_event(ev->cb_hash);
			if (!cb)
				continue;
			in->binds[ei].cb = cb;
			in->binds[ei].param = ev->param;
			lv_obj_add_event_cb(obj, wf_trampoline, map_event(ev->code), &in->binds[ei]);
		}
	}

	RE_FREE(objs);
	return in;
}

void wf_unload(wf_instance_t *in, bool del_screen) {
	if (!in)
		return;
	if (in->screen && del_screen)
		lv_obj_clean(in->screen); /* delete created children */
	for (int i = 0; i < in->n_alloc; i++)
		RE_FREE(in->allocs[i]);
	RE_FREE(in->allocs);
	RE_FREE(in);
}

void wf_refresh(wf_instance_t *in) {
	if (!in)
		return;
	for (uint8_t i = 0; i < in->dyn_count; i++)
		refresh_one(in, &in->dyn[i]);
}
