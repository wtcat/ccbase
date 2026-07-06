/*
 * wfc — WatchFace Compiler.
 *
 * Compiles a Tier-1 authoring JSON (see docs/schema/watchface.schema.json)
 * into the compact WFB binary runtime layer (see docs/wfb_format.h).
 *
 * It only PACKAGES layout: resources (images / frame-anim groups) live in an
 * external ResFile and are addressed by re_name_hash(name); fonts are provided
 * by firmware at runtime by the same namekey. Therefore wfc never opens the
 * resource binaries — it just records namekeys, resolves names to indices,
 * topologically orders widgets, and emits header + tables + CRC.
 *
 * Usage: wfc <input.json> -o <output.wfb>
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embeded/wfb_format.h"
#include "cJSON.h"
#include "wfc_config.h"

/* Capacity caps come from the compile-time target (see wfc_config.h). */
#define MAX_WIDGETS WFC_MAX_WIDGETS
#define MAX_IMAGES WFC_MAX_IMAGES
#define MAX_FONTS WFC_MAX_FONTS
#define MAX_ANIMS WFC_MAX_ANIMS
#define MAX_EVENTS WFC_MAX_EVENTS
#define MAX_STYLES WFC_MAX_STYLES
#define STRPOOL_CAP WFC_STRPOOL_CAP

#if !WFC_HAVE_STDIO
#include <setjmp.h>
static jmp_buf wfc_err_jmp;   /* die() longjmps here instead of exit() */
static char wfc_err_msg[128];
const char *wfc_last_error(void) { return wfc_err_msg; }
#endif

typedef struct {
	uint32_t id_hash;	   /* re_name_hash(id): dedup + parent resolution   */
	uint32_t parent_hash;  /* re_name_hash(parent); valid iff parent!=NULL  */
	const char *id;		   /* borrowed from cJSON tree; diagnostics only    */
	const char *parent;	   /* borrowed; NULL = root                         */
	int parent_orig;	   /* resolved original index, -1 root              */
	wf_widget_t w;		   /* parent/event_* filled after sort              */

	/* dynamic imglabel (WF_WF_DYNTEXT); emitted into strings at serialize   */
	uint8_t has_dyntext;
	uint8_t dt_glyph_cnt;
	uint8_t dt_max_len;
	uint32_t dt_provider;  /* provider name hash                            */
	const char *dt_glyphs; /* -> dt_glyphs_buf; dt_glyph_cnt chars          */
	char dt_glyphs_buf[16];/* expanded glyphs (shorthand resolved)          */

	/* flex (WF_WF_FLEX container config; emitted into strings at serialize)  */
	uint8_t has_flex;
	uint8_t flex_flow, flex_main, flex_cross, flex_track;
	uint8_t flex_grow;	   /* this widget's grow AS A CHILD (default 0)      */
} pwidget_t;

#define WFC_IMGTEXT_MAX 16 /* max glyphs and max displayed chars (uint8)    */

typedef struct {
	int owner;
	wf_event_t e;
} pevent_t;

/* Only the re_name_hash of each resource NAME is kept — enough for dedup and
 * for widget->resource resolution. The output tables store namekey(file) for
 * the ResFile/firmware lookup, which is a separate string. */
static uint32_t img_keys[MAX_IMAGES];
static int n_img;
static uint32_t fnt_keys[MAX_FONTS];
static int n_fnt;
static uint32_t anm_keys[MAX_ANIMS];
static int n_anm;

static wf_image_t images[MAX_IMAGES];
static wf_font_t fonts[MAX_FONTS];
static wf_anim_t anims[MAX_ANIMS];

static wf_style_t styles[MAX_STYLES];
static int n_style;

static char strpool[STRPOOL_CAP];
static uint32_t pool_len;

static pwidget_t pw[MAX_WIDGETS];
static int n_pw;

static pevent_t pev[MAX_EVENTS];
static int n_pev;

static int order[MAX_WIDGETS];	   /* order[k] = original index */
static int new_index[MAX_WIDGETS]; /* new_index[orig] = position */

static const char *const ALIGN_NAMES[] = {"center",		  "top_left",	 "top_mid",
										  "top_right",	  "bottom_left", "bottom_mid",
										  "bottom_right", "left_mid",	 "right_mid"};
static const char *const EVENT_NAMES[] = {"clicked", "pressed", "released",
										  "long_pressed", "value_changed"};
static const char *const TYPE_NAMES[] = {"screen", "image", "label",	 "frame_anim",
										 "arc",	   "bar",	"container", "imglabel"};
static const char *const FLEX_FLOW_NAMES[] = {"row",		 "column",		 "row_wrap",
											  "column_wrap", "row_reverse",	 "column_reverse"};
static const char *const FLEX_ALIGN_NAMES[] = {"start",		  "end",		 "center",
											   "space_evenly", "space_around", "space_between"};


static void die(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
#if WFC_HAVE_STDIO
	fputs("wfc: error: ", stderr);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
#else
	vsnprintf(wfc_err_msg, sizeof(wfc_err_msg), fmt, ap);
	va_end(ap);
	longjmp(wfc_err_jmp, 1);
#endif
}

/* 1-based line/col of `at` within null-terminated `text` (same buffer). */
static void json_loc(const char *text, const char *at, int *line, int *col) {
	int ln = 1, cl = 1;
	for (const char *p = text; at && p < at && *p; p++) {
		if (*p == '\n') {
			ln++;
			cl = 1;
		} else {
			cl++;
		}
	}
	*line = ln;
	*col = cl;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
		0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};
	crc = ~crc;
	for (size_t i = 0; i < len; i++) {
		uint8_t b = data[i];
		crc = (crc >> 4) ^ table[(crc ^ b) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)b >> 4)) & 0x0f];
	}
	return ~crc;
}

static int find_key(const uint32_t *keys, int n, uint32_t key) {
	for (int i = 0; i < n; i++)
		if (keys[i] == key)
			return i;
	return -1;
}

static uint32_t namekey(const char *s) {
	return wf_name_hash(s, (uint32_t)strlen(s));
}

static uint16_t intern_style(const wf_style_t *s) {
	for (int i = 0; i < n_style; i++)
		if (memcmp(&styles[i], s, sizeof(*s)) == 0)
			return (uint16_t)i;
	if (n_style >= MAX_STYLES)
		die("too many distinct styles (max %d)", MAX_STYLES);
	styles[n_style] = *s;
	return (uint16_t)n_style++;
}

static uint32_t intern_string(const char *s) {
	size_t off = 0;
	while (off < pool_len) {
		if (strcmp(&strpool[off], s) == 0)
			return (uint32_t)off;
		off += strlen(&strpool[off]) + 1;
	}
	size_t need = strlen(s) + 1;
	if (pool_len + need > STRPOOL_CAP)
		die("label string pool overflow");
	uint32_t at = pool_len;
	memcpy(&strpool[pool_len], s, need);
	pool_len += (uint32_t)need;
	return at;
}


/* 
 * JSON field helpers 
 */
static const char *req_str(const cJSON *o, const char *key, const char *ctx) {
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	if (!cJSON_IsString(v) || !v->valuestring)
		die("%s: missing/invalid string field \"%s\"", ctx, key);
	return v->valuestring;
}

static int opt_int(const cJSON *o, const char *key, int dflt) {
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	if (!v)
		return dflt;
	if (!cJSON_IsNumber(v))
		die("field \"%s\" must be a number", key);
	return v->valueint;
}

static int opt_bool(const cJSON *o, const char *key) {
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	return cJSON_IsTrue(v) ? 1 : 0;
}

static uint32_t parse_color(const char *s, const char *ctx) {
	if (s[0] != '#' || strlen(s) != 7)
		die("%s: bad color \"%s\" (want #RRGGBB)", ctx, s);
	char *end = NULL;
	unsigned long v = strtoul(s + 1, &end, 16);
	if (!end || *end)
		die("%s: bad color \"%s\"", ctx, s);
	return (uint32_t)(v & 0x00FFFFFFu);
}

/* string enum lookup; returns index or -1 */
static int enum_index(const char *s, const char *const *names, int n) {
	for (int i = 0; i < n; i++)
		if (strcmp(s, names[i]) == 0)
			return i;
	return -1;
}

/* Expand glyphs shorthand into `out` (cap bytes). "X..Y" (Y>=X) -> inclusive
 * ASCII run X..Y step 1; any other char copied verbatim. Returns length.
 * die() on descending range or overflow. A lone/single '.' stays literal. */
static uint8_t expand_glyphs(const char *src, char *out, uint8_t cap,
							 const char *id) {
	uint8_t n = 0;
	for (const char *p = src; *p;) {
		if (p[1] == '.' && p[2] == '.' && p[3]) { /* X..Y */
			unsigned char a = (unsigned char)p[0], b = (unsigned char)p[3];
			if (b < a)
				die("widget \"%s\": glyphs range \"%c..%c\" is descending", id, a, b);
			for (unsigned c = a; c <= b; c++) {
				if (n >= cap)
					die("widget \"%s\": glyphs expands beyond %d chars", id, cap);
				out[n++] = (char)c;
			}
			p += 4;
		} else {
			if (n >= cap)
				die("widget \"%s\": glyphs expands beyond %d chars", id, cap);
			out[n++] = *p++;
		}
	}
	return n;
}

/* 
 * Resource parsing 
 */
static void parse_resources(const cJSON *res) {
	const cJSON *arr, *it;

	arr = cJSON_GetObjectItemCaseSensitive(res, "images");
	cJSON_ArrayForEach(it, arr) {
		if (n_img >= MAX_IMAGES)
			die("too many images (max %d)", MAX_IMAGES);
		const char *nm = req_str(it, "name", "image");
		const char *file = req_str(it, "file", "image");
		uint32_t nk = namekey(nm);
		if (find_key(img_keys, n_img, nk) >= 0)
			die("duplicate image name \"%s\"", nm);
		img_keys[n_img] = nk;
		images[n_img].namekey = namekey(file);
		n_img++;
	}

	arr = cJSON_GetObjectItemCaseSensitive(res, "fonts");
	cJSON_ArrayForEach(it, arr) {
		if (n_fnt >= MAX_FONTS)
			die("too many fonts (max %d)", MAX_FONTS);
		const char *nm = req_str(it, "name", "font");
		const char *file = req_str(it, "file", "font");
		uint32_t nk = namekey(nm);
		if (find_key(fnt_keys, n_fnt, nk) >= 0)
			die("duplicate font name \"%s\"", nm);
		fnt_keys[n_fnt] = nk;
		fonts[n_fnt].namekey = namekey(file);
		n_fnt++;
	}

	arr = cJSON_GetObjectItemCaseSensitive(res, "anims");
	cJSON_ArrayForEach(it, arr) {
		if (n_anm >= MAX_ANIMS)
			die("too many anims (max %d)", MAX_ANIMS);
		const char *nm = req_str(it, "name", "anim");
		const char *file = req_str(it, "file", "anim");
		int dur = opt_int(it, "duration", -1);
		if (dur < 1 || dur > 65535)
			die("anim \"%s\": duration out of range (1..65535)", nm);
		int rep = opt_int(it, "repeat", 0);
		if (rep < 0 || rep > 255)
			die("anim \"%s\": repeat out of range (0..255)", nm);
		uint32_t nk = namekey(nm);
		if (find_key(anm_keys, n_anm, nk) >= 0)
			die("duplicate anim name \"%s\"", nm);
		anm_keys[n_anm] = nk;
		anims[n_anm].namekey = namekey(file);
		anims[n_anm].duration_ms = (uint16_t)dur;
		anims[n_anm].repeat = (uint8_t)rep;
		anims[n_anm].flags = 0;
		n_anm++;
	}
}


/* 
 * Widget parsing 
 */
static int16_t size_val(const cJSON *o, const char *key) {
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	if (!v)
		return WF_SIZE_RES; /* omitted -> native/auto */
	if (!cJSON_IsNumber(v))
		die("field \"%s\" must be a number", key);
	if (v->valueint < -2 || v->valueint > 32767)
		die("field \"%s\" out of range", key);
	return (int16_t)v->valueint;
}

static void parse_style(const cJSON *w, wf_widget_t *out) {
	const cJSON *st = cJSON_GetObjectItemCaseSensitive(w, "style");
	if (!st) {
		out->style_ref = WF_REF_NONE;
		return;
	}
	if (!cJSON_IsObject(st))
		die("widget style must be an object");

	wf_style_t s;
	memset(&s, 0, sizeof(s));
	const cJSON *v;
	if ((v = cJSON_GetObjectItemCaseSensitive(st, "text_color")))
		s.text_color = parse_color(v->valuestring, "style.text_color");
	if ((v = cJSON_GetObjectItemCaseSensitive(st, "bg_color")))
		s.bg_color = parse_color(v->valuestring, "style.bg_color");
	s.bg_opa = (uint16_t)opt_int(st, "bg_opa", 0);
	s.arc_width = (uint16_t)opt_int(st, "arc_width", 0);
	if ((v = cJSON_GetObjectItemCaseSensitive(st, "arc_color")))
		s.arc_color = parse_color(v->valuestring, "style.arc_color");
	int has_ir = 0;
	if ((v = cJSON_GetObjectItemCaseSensitive(st, "image_recolor"))) {
		s.image_recolor = parse_color(v->valuestring, "style.image_recolor");
		has_ir = 1;
	}
	/* recolor is a no-op without opa; default to full when a color was given */
	s.image_recolor_opa = (uint16_t)opt_int(st, "image_recolor_opa", has_ir ? 255 : 0);

	out->style_ref = intern_style(&s);
}

/* enum field with a default; die() on a bad string. */
static uint8_t flex_enum(const cJSON *o, const char *key, const char *const *names,
						 int n, uint8_t dflt, const char *id) {
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	if (!v)
		return dflt;
	if (!cJSON_IsString(v))
		die("widget \"%s\": flex.%s must be a string", id, key);
	int i = enum_index(v->valuestring, names, n);
	if (i < 0)
		die("widget \"%s\": bad flex.%s \"%s\"", id, key, v->valuestring);
	return (uint8_t)i;
}

static void parse_flex(const cJSON *w, pwidget_t *p, int type, const char *id) {
	const cJSON *fx = cJSON_GetObjectItemCaseSensitive(w, "flex");
	if (!fx)
		return;
	if (!cJSON_IsObject(fx))
		die("widget \"%s\": flex must be an object", id);
	if (type != WF_W_SCREEN && type != WF_W_CONTAINER)
		die("widget \"%s\": flex only on screen/container", id);
	p->has_flex = 1;
	p->flex_flow = flex_enum(fx, "flow", FLEX_FLOW_NAMES, 6, WF_FLEX_ROW, id);
	p->flex_main = flex_enum(fx, "main", FLEX_ALIGN_NAMES, 6, WF_FLEX_START, id);
	p->flex_cross = flex_enum(fx, "cross", FLEX_ALIGN_NAMES, 6, WF_FLEX_START, id);
	p->flex_track = flex_enum(fx, "track", FLEX_ALIGN_NAMES, 6, WF_FLEX_START, id);
	p->w.flags |= WF_WF_FLEX;
}

static void parse_widget(const cJSON *w) {
	if (n_pw >= MAX_WIDGETS)
		die("too many widgets (max %d)", MAX_WIDGETS);
	pwidget_t *p = &pw[n_pw];
	memset(p, 0, sizeof(*p));
	wf_widget_t *o = &p->w;

	const char *id = req_str(w, "id", "widget");
	p->id = id;
	p->id_hash = namekey(id);
	for (int i = 0; i < n_pw; i++)
		if (pw[i].id_hash == p->id_hash)
			die("duplicate widget id \"%s\"", id);

	const char *ts = req_str(w, "type", "widget");
	int type =
		enum_index(ts, TYPE_NAMES, (int)(sizeof(TYPE_NAMES) / sizeof(*TYPE_NAMES)));
	if (type < 0)
		die("widget \"%s\": unknown type \"%s\"", id, ts);
	o->type = (uint8_t)type;

	const cJSON *pj = cJSON_GetObjectItemCaseSensitive(w, "parent");
	if (cJSON_IsString(pj) && pj->valuestring) {
		p->parent = pj->valuestring;
		p->parent_hash = namekey(pj->valuestring);
	} else {
		p->parent = NULL;
	}

	const cJSON *aj = cJSON_GetObjectItemCaseSensitive(w, "align");
	if (cJSON_IsString(aj)) {
		int a = enum_index(aj->valuestring, ALIGN_NAMES, 9);
		if (a < 0)
			die("widget \"%s\": bad align \"%s\"", id, aj->valuestring);
		o->align = (uint8_t)a;
	} else {
		o->align = WF_ALIGN_TOP_LEFT; /* LVGL default */
	}

	int x = opt_int(w, "x", 0), y = opt_int(w, "y", 0);
	if (x < -32768 || x > 32767 || y < -32768 || y > 32767)
		die("widget \"%s\": x/y out of range", id);
	o->x = (int16_t)x;
	o->y = (int16_t)y;
	o->w = size_val(w, "w");
	o->h = size_val(w, "h");

	o->res_ref = WF_REF_NONE;
	o->extra = 0;
	switch (type) {
	case WF_W_IMAGE:
	case WF_W_IMGLABEL: {
		/* both resolve a name from the images table; imglabel's is a group */
		const char *nm = req_str(
			w, "image", type == WF_W_IMAGE ? "image widget" : "imglabel widget");
		int idx = find_key(img_keys, n_img, namekey(nm));
		if (idx < 0)
			die("widget \"%s\": unknown image \"%s\"", id, nm);
		o->res_ref = (uint16_t)idx;
		if (type == WF_W_IMGLABEL) {
			const cJSON *pv = cJSON_GetObjectItemCaseSensitive(w, "provider");
			if (cJSON_IsString(pv) && pv->valuestring) {
				const char *glyphs = req_str(w, "glyphs", "imglabel provider");
				uint8_t glen = expand_glyphs(glyphs, p->dt_glyphs_buf, WFC_IMGTEXT_MAX, id);
				if (glen < 1)
					die("widget \"%s\": glyphs must be non-empty", id);
				int ml = opt_int(w, "max_len", (int)glen);
				if (ml < 1 || ml > WFC_IMGTEXT_MAX)
					die("widget \"%s\": max_len must be 1..%d", id, WFC_IMGTEXT_MAX);
				p->has_dyntext = 1;
				p->dt_provider = namekey(pv->valuestring);
				p->dt_glyphs = p->dt_glyphs_buf;
				p->dt_glyph_cnt = glen;
				p->dt_max_len = (uint8_t)ml;
			}
		}
		break;
	}
	case WF_W_LABEL: {
		const char *fn = req_str(w, "font", "label widget");
		int idx = find_key(fnt_keys, n_fnt, namekey(fn));
		if (idx < 0)
			die("widget \"%s\": unknown font \"%s\"", id, fn);
		o->res_ref = (uint16_t)idx;
		o->extra = intern_string(req_str(w, "text", "label widget"));
		break;
	}
	case WF_W_FRAME_ANIM: {
		const char *nm = req_str(w, "anim", "frame_anim widget");
		int idx = find_key(anm_keys, n_anm, namekey(nm));
		if (idx < 0)
			die("widget \"%s\": unknown anim \"%s\"", id, nm);
		o->res_ref = (uint16_t)idx;
		break;
	}
	default:
		break;
	}

	parse_style(w, o);

	o->flags = 0;
	if (opt_bool(w, "hidden"))
		o->flags |= WF_WF_HIDDEN;
	if (opt_bool(w, "clickable"))
		o->flags |= WF_WF_CLICKABLE;
	if (p->has_dyntext)
		o->flags |= WF_WF_DYNTEXT;
	if (opt_bool(w, "recolor")) {
		if (type != WF_W_LABEL)
			die("widget \"%s\": recolor only on label", id);
		o->flags |= WF_WF_RECOLOR;
	}
	parse_flex(w, p, type, id);
	int fg = opt_int(w, "flex_grow", 0);
	if (fg < 0 || fg > 255)
		die("widget \"%s\": flex_grow must be 0..255", id);
	p->flex_grow = (uint8_t)fg;

	/* events */
	o->event_start = 0;
	o->event_count = 0;
	const cJSON *evs = cJSON_GetObjectItemCaseSensitive(w, "events"), *e;
	cJSON_ArrayForEach(e, evs) {
		if (n_pev >= MAX_EVENTS)
			die("too many events (max %d)", MAX_EVENTS);
		const char *on = req_str(e, "on", "event");
		int code = enum_index(on, EVENT_NAMES, 5);
		if (code < 0)
			die("widget \"%s\": bad event \"%s\"", id, on);
		const char *call = req_str(e, "call", "event");
		pev[n_pev].owner = n_pw;
		pev[n_pev].e.code = (uint8_t)code;
		pev[n_pev].e.reserved = 0;
		pev[n_pev].e.widget = 0; /* set after topo */
		pev[n_pev].e.cb_hash = namekey(call);
		pev[n_pev].e.param = (uint32_t)opt_int(e, "param", 0);
		n_pev++;
		if (!(o->flags & WF_WF_CLICKABLE))
			o->flags |= WF_WF_CLICKABLE;
	}

	n_pw++;
}

/* Topological sort (parents before children) */
static void topo_sort(void) {
	/* resolve parent names -> original indices */
	for (int i = 0; i < n_pw; i++) {
		if (pw[i].parent == NULL) {
			pw[i].parent_orig = -1;
			continue;
		}
		int pj = -1;
		for (int k = 0; k < n_pw; k++)
			if (pw[k].id_hash == pw[i].parent_hash) {
				pj = k;
				break;
			}
		if (pj < 0)
			die("widget \"%s\": unknown parent \"%s\"", pw[i].id, pw[i].parent);
		if (pj == i)
			die("widget \"%s\": parent references itself", pw[i].id);
		pw[i].parent_orig = pj;
	}

	char placed[MAX_WIDGETS];
	memset(placed, 0, sizeof(placed));
	int placed_n = 0;
	while (placed_n < n_pw) {
		int progress = 0;
		for (int i = 0; i < n_pw; i++) {
			if (placed[i])
				continue;
			int par = pw[i].parent_orig;
			if (par < 0 || placed[par]) {
				new_index[i] = placed_n;
				order[placed_n++] = i;
				placed[i] = 1;
				progress = 1;
			}
		}
		if (!progress)
			die("widget parent graph has a cycle");
	}
}


/* Reset all accumulator state so the compiler can run more than once
 * (a single main() run on PC, but repeated wfc_compile() calls on MCU). */
static void reset_state(void) {
	n_img = n_fnt = n_anm = 0;
	n_style = 0;
	n_pw = 0;
	n_pev = 0;
	pool_len = 0;
}

/*
 * Serialize the current state into a freshly malloc'd file image
 * (wf_header_t followed by the layout section). Caller frees the buffer.
 * *file_size gets the total byte count. die() on OOM.
 */
#define WFC_RU4(x) (((uint32_t)(x) + 3u) & ~3u)

static uint8_t *serialize(uint16_t sw, uint16_t sh, uint32_t *file_size) {
	/* final widget array in topo order */
	static wf_widget_t out_w[MAX_WIDGETS];
	static wf_event_t out_e[MAX_EVENTS];
	int n_e = 0;

	/* wf_imgtext_t blobs live in the strings section after the label pool,
	 * each 4-byte aligned; extra (offset within the strings section) is
	 * assigned here since it depends on the final pool length. */
	uint32_t blob_cursor = WFC_RU4(pool_len);

	for (int k = 0; k < n_pw; k++) {
		int orig = order[k];
		wf_widget_t wd = pw[orig].w;
		int par = pw[orig].parent_orig;
		wd.parent = (par < 0) ? WF_PARENT_ROOT : (uint16_t)new_index[par];

		/* gather this widget's events contiguously */
		wd.event_start = (uint16_t)n_e;
		int cnt = 0;
		for (int j = 0; j < n_pev; j++) {
			if (pev[j].owner != orig)
				continue;
			out_e[n_e] = pev[j].e;
			out_e[n_e].widget = (uint16_t)k;
			n_e++;
			cnt++;
		}
		wd.event_count = (uint8_t)cnt;

		if (wd.flags & WF_WF_DYNTEXT) {
			wd.extra = blob_cursor;
			blob_cursor =
				WFC_RU4(blob_cursor + sizeof(wf_imgtext_t) + pw[orig].dt_glyph_cnt);
		}
		if (wd.flags & WF_WF_FLEX) { /* mutually exclusive with DYNTEXT */
			int child_cnt = 0;
			for (int j = 0; j < n_pw; j++)
				if (pw[j].parent_orig == orig)
					child_cnt++;
			wd.extra = blob_cursor;
			blob_cursor = WFC_RU4(blob_cursor + sizeof(wf_flex_t) + child_cnt);
		}
		out_w[k] = wd;
	}

	/* section offsets (relative to layout start); all struct sizes % 4 == 0 */
	uint32_t off_widgets = 0;
	uint32_t off_images = off_widgets + (uint32_t)n_pw * sizeof(wf_widget_t);
	uint32_t off_fonts = off_images + (uint32_t)n_img * sizeof(wf_image_t);
	uint32_t off_anims = off_fonts + (uint32_t)n_fnt * sizeof(wf_font_t);
	uint32_t off_events = off_anims + (uint32_t)n_anm * sizeof(wf_anim_t);
	uint32_t off_styles = off_events + (uint32_t)n_e * sizeof(wf_event_t);
	uint32_t off_strings = off_styles + (uint32_t)n_style * sizeof(wf_style_t);
	/* blob_cursor is the 4-aligned end of the strings section (label pool +
	 * imgtext blobs); equals WFC_RU4(pool_len) when there are no dyntext widgets */
	uint32_t layout_size = off_strings + blob_cursor;

	/* One allocation for the whole file: header + layout, contiguous. */
	uint32_t total = (uint32_t)sizeof(wf_header_t) + layout_size;
	uint8_t *buf = calloc(1, total);
	if (!buf)
		die("out of memory");
	uint8_t *layout = buf + sizeof(wf_header_t);
	memcpy(layout + off_widgets, out_w, (size_t)n_pw * sizeof(wf_widget_t));
	memcpy(layout + off_images, images, (size_t)n_img * sizeof(wf_image_t));
	memcpy(layout + off_fonts, fonts, (size_t)n_fnt * sizeof(wf_font_t));
	memcpy(layout + off_anims, anims, (size_t)n_anm * sizeof(wf_anim_t));
	memcpy(layout + off_events, out_e, (size_t)n_e * sizeof(wf_event_t));
	memcpy(layout + off_styles, styles, (size_t)n_style * sizeof(wf_style_t));
	memcpy(layout + off_strings, strpool, pool_len);

	/* emit wf_imgtext_t blobs (padding bytes already zeroed by calloc) */
	for (int k = 0; k < n_pw; k++) {
		if (!(out_w[k].flags & WF_WF_DYNTEXT))
			continue;
		int orig = order[k];
		uint8_t *bp = layout + off_strings + out_w[k].extra;
		wf_imgtext_t d;
		memset(&d, 0, sizeof(d));
		d.provider_hash = pw[orig].dt_provider;
		d.glyph_cnt = pw[orig].dt_glyph_cnt;
		d.max_len = pw[orig].dt_max_len;
		memcpy(bp, &d, sizeof(d));
		memcpy(bp + sizeof(d), pw[orig].dt_glyphs, pw[orig].dt_glyph_cnt);
	}

	/* emit wf_flex_t blobs; grow[] gathered in topo order == child creation order */
	for (int k = 0; k < n_pw; k++) {
		if (!(out_w[k].flags & WF_WF_FLEX))
			continue;
		int corig = order[k];
		uint8_t *bp = layout + off_strings + out_w[k].extra;
		wf_flex_t f;
		memset(&f, 0, sizeof(f));
		f.flow = pw[corig].flex_flow;
		f.main_align = pw[corig].flex_main;
		f.cross_align = pw[corig].flex_cross;
		f.track_align = pw[corig].flex_track;
		uint8_t *gp = bp + sizeof(f);
		uint8_t ic = 0;
		for (int k2 = 0; k2 < n_pw; k2++) {
			int o2 = order[k2];
			if (pw[o2].parent_orig == corig)
				gp[ic++] = pw[o2].flex_grow;
		}
		f.item_cnt = ic;
		memcpy(bp, &f, sizeof(f));
	}

	wf_header_t *h = (wf_header_t *)buf; /* buf is malloc-aligned */
	h->magic[0] = WFB_MAGIC0;
	h->magic[1] = WFB_MAGIC1;
	h->magic[2] = WFB_MAGIC2;
	h->magic[3] = WFB_MAGIC3;
	h->version = WFB_VERSION;
	h->flags = 0;
	h->screen_w = sw;
	h->screen_h = sh;
	h->widget_count = (uint16_t)n_pw;
	h->image_count = (uint16_t)n_img;
	h->font_count = (uint16_t)n_fnt;
	h->anim_count = (uint16_t)n_anm;
	h->event_count = (uint16_t)n_e;
	h->style_count = (uint16_t)n_style;
	h->off_widgets = off_widgets;
	h->off_images = off_images;
	h->off_fonts = off_fonts;
	h->off_anims = off_anims;
	h->off_events = off_events;
	h->off_styles = off_styles;
	h->off_strings = off_strings;
	h->layout_size = layout_size;
	h->crc32 = crc32_update(0, layout, layout_size);

	*file_size = total;
	return buf;
}

/*
 * Parse a validated document (version/screen/resources/widgets) into the
 * accumulator state and topologically order the widgets. die() on any error.
 */
static void parse_document(const cJSON *root, uint16_t *sw_out, uint16_t *sh_out) {
	const cJSON *ver = cJSON_GetObjectItemCaseSensitive(root, "version");
	if (!cJSON_IsNumber(ver) || ver->valueint != 1)
		die("version must be 1");

	const cJSON *scr = cJSON_GetObjectItemCaseSensitive(root, "screen");
	if (!cJSON_IsObject(scr))
		die("missing \"screen\" object");
	int sw = opt_int(scr, "w", 0), sh = opt_int(scr, "h", 0);
	if (sw < 1 || sw > 65535 || sh < 1 || sh > 65535)
		die("screen w/h out of range");

	const cJSON *res = cJSON_GetObjectItemCaseSensitive(root, "resources");
	if (res && !cJSON_IsObject(res))
		die("\"resources\" must be an object");
	if (res)
		parse_resources(res);

	const cJSON *ws = cJSON_GetObjectItemCaseSensitive(root, "widgets"), *w;
	if (!cJSON_IsArray(ws) || cJSON_GetArraySize(ws) < 1)
		die("\"widgets\" must be a non-empty array");
	cJSON_ArrayForEach(w, ws) parse_widget(w);

	topo_sort();
	*sw_out = (uint16_t)sw;
	*sh_out = (uint16_t)sh;
}

#if WFC_HAVE_STDIO
/* ------------------------------ PC entry ------------------------------- */

static char *read_file(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f)
		die("cannot open input \"%s\"", path);
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	rewind(f);
	if (n < 0)
		die("cannot size input");
	char *buf = malloc((size_t)n + 1);
	if (!buf)
		die("out of memory");
	if (fread(buf, 1, (size_t)n, f) != (size_t)n)
		die("read failed");
	buf[n] = '\0';
	fclose(f);
	return buf;
}

int main(int argc, char **argv) {
	const char *in = NULL, *out = NULL;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
			out = argv[++i];
		else if (argv[i][0] != '-')
			in = argv[i];
		else
			die("unknown option \"%s\"", argv[i]);
	}
	if (!in || !out) {
		fprintf(stderr, "usage: wfc <input.json> -o <output.wfb>\n");
		return 2;
	}

	reset_state();
	char *text = read_file(in);
	cJSON *root = cJSON_Parse(text);
	if (!root) {
		const char *e = cJSON_GetErrorPtr();
		int line, col;
		json_loc(text, e, &line, &col);
		die("%s:%d:%d: JSON parse error near: %.40s", in, line, col,
			e ? e : "(unknown)");
	}

	uint16_t sw, sh;
	parse_document(root, &sw, &sh);

	uint32_t size;
	uint8_t *img = serialize(sw, sh, &size);
	FILE *f = fopen(out, "wb");
	if (!f)
		die("cannot open output \"%s\"", out);
	if (fwrite(img, 1, size, f) != size)
		die("write failed");
	fclose(f);

	uint32_t layout_size = size - (uint32_t)sizeof(wf_header_t);
	fprintf(stderr,
			"wfc: ok -> %s (%u widgets, %u img, %u font, %u anim, %u ev, %u style; "
			"header %zu + layout %u bytes)\n",
			out, (unsigned)n_pw, (unsigned)n_img, (unsigned)n_fnt, (unsigned)n_anm,
			(unsigned)n_pev, (unsigned)n_style, sizeof(wf_header_t), layout_size);

	free(img);
	cJSON_Delete(root);
	free(text);
	return 0;
}

#else
/* ------------------------------ MCU entry ------------------------------ */

/*
 * Compile a JSON document held in memory into a WFB image written to the
 * caller-provided output buffer. Returns the number of bytes written, or -1
 * on error (message available via wfc_last_error()).
 *
 * Firmware may route cJSON's allocations to a pool by calling cJSON_InitHooks()
 * before this — wfc adds no allocator of its own beyond the single output buffer.
 */
int wfc_compile(const void *json, size_t json_len, void *out, size_t out_cap) {
	cJSON *volatile root = NULL; /* volatile: read after longjmp */
	if (setjmp(wfc_err_jmp)) {
		if (root)
			cJSON_Delete(root);
		return -1;
	}

	reset_state();
	root = cJSON_ParseWithLength((const char *)json, json_len);
	if (!root) {
		const char *e = cJSON_GetErrorPtr();
		int line, col;
		json_loc((const char *)json, e, &line, &col);
		die("JSON parse error at line %d col %d", line, col);
	}

	uint16_t sw, sh;
	parse_document(root, &sw, &sh);

	uint32_t size;
	uint8_t *img = serialize(sw, sh, &size);
	int rc;
	if (size > out_cap) {
		free(img);
		die("output buffer too small (need %u, have %u)", (unsigned)size,
			(unsigned)out_cap);
	}
	memcpy(out, img, size);
	rc = (int)size;

	free(img);
	cJSON_Delete(root);
	return rc;
}

#endif
