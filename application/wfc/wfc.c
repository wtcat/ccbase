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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embeded/wfb_format.h"
#include "cJSON.h"

#define MAX_WIDGETS 512
#define MAX_IMAGES 1024
#define MAX_FONTS 64
#define MAX_ANIMS 64
#define MAX_EVENTS 512
#define MAX_STYLES 128
#define MAX_NAME 64
#define STRPOOL_CAP 8192

struct resource_struct {
	void (*release)(void *ptr);
	void *ptr;
};

#define MAX_RESOURCES 5
static struct resource_struct resource_array[MAX_RESOURCES];
static size_t resource_count;


typedef struct {
	char id[MAX_NAME];
	char parent[MAX_NAME]; /* "" = root */
	int parent_orig;	   /* resolved original index, -1 root */
	wf_widget_t w;		   /* parent/event_* filled after sort */
} pwidget_t;

typedef struct {
	int owner;
	wf_event_t e;
} pevent_t;

static char img_names[MAX_IMAGES][MAX_NAME];
static int n_img;
static char fnt_names[MAX_FONTS][MAX_NAME];
static int n_fnt;
static char anm_names[MAX_ANIMS][MAX_NAME];
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


static void die(const char *fmt, ...) {
	va_list ap;
	fputs("wfc: error: ", stderr);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
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

static int find_name(char table[][MAX_NAME], int n, const char *s) {
	for (int i = 0; i < n; i++)
		if (strcmp(table[i], s) == 0)
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

static void copy_name(char *dst, const char *src, const char *ctx) {
	if (strlen(src) >= MAX_NAME)
		die("%s: name \"%s\" too long (max %d)", ctx, src, MAX_NAME - 1);
	strcpy(dst, src);
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
		req_str(it, "file", "image"); /* validated, but unused by wfc */
		if (find_name(img_names, n_img, nm) >= 0)
			die("duplicate image name \"%s\"", nm);
		copy_name(img_names[n_img], nm, "image");
		images[n_img].namekey = namekey(nm);
		n_img++;
	}

	arr = cJSON_GetObjectItemCaseSensitive(res, "fonts");
	cJSON_ArrayForEach(it, arr) {
		if (n_fnt >= MAX_FONTS)
			die("too many fonts (max %d)", MAX_FONTS);
		const char *nm = req_str(it, "name", "font");
		req_str(it, "file", "font");
		if (find_name(fnt_names, n_fnt, nm) >= 0)
			die("duplicate font name \"%s\"", nm);
		copy_name(fnt_names[n_fnt], nm, "font");
		fonts[n_fnt].namekey = namekey(nm);
		n_fnt++;
	}

	arr = cJSON_GetObjectItemCaseSensitive(res, "anims");
	cJSON_ArrayForEach(it, arr) {
		if (n_anm >= MAX_ANIMS)
			die("too many anims (max %d)", MAX_ANIMS);
		const char *nm = req_str(it, "name", "anim");
		int dur = opt_int(it, "duration", -1);
		if (dur < 1 || dur > 65535)
			die("anim \"%s\": duration out of range (1..65535)", nm);
		int rep = opt_int(it, "repeat", 0);
		if (rep < 0 || rep > 255)
			die("anim \"%s\": repeat out of range (0..255)", nm);
		if (find_name(anm_names, n_anm, nm) >= 0)
			die("duplicate anim name \"%s\"", nm);
		copy_name(anm_names[n_anm], nm, "anim");
		anims[n_anm].namekey = namekey(nm);
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

	out->style_ref = intern_style(&s);
}

static void parse_widget(const cJSON *w) {
	if (n_pw >= MAX_WIDGETS)
		die("too many widgets (max %d)", MAX_WIDGETS);
	pwidget_t *p = &pw[n_pw];
	memset(p, 0, sizeof(*p));
	wf_widget_t *o = &p->w;

	const char *id = req_str(w, "id", "widget");
	copy_name(p->id, id, "widget id");
	for (int i = 0; i < n_pw; i++)
		if (strcmp(pw[i].id, id) == 0)
			die("duplicate widget id \"%s\"", id);

	const char *ts = req_str(w, "type", "widget");
	int type =
		enum_index(ts, TYPE_NAMES, (int)(sizeof(TYPE_NAMES) / sizeof(*TYPE_NAMES)));
	if (type < 0)
		die("widget \"%s\": unknown type \"%s\"", id, ts);
	o->type = (uint8_t)type;

	const cJSON *pj = cJSON_GetObjectItemCaseSensitive(w, "parent");
	if (cJSON_IsString(pj) && pj->valuestring)
		copy_name(p->parent, pj->valuestring, "parent");
	else
		p->parent[0] = '\0';

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
		int idx = find_name(img_names, n_img, nm);
		if (idx < 0)
			die("widget \"%s\": unknown image \"%s\"", id, nm);
		o->res_ref = (uint16_t)idx;
		break;
	}
	case WF_W_LABEL: {
		const char *fn = req_str(w, "font", "label widget");
		int idx = find_name(fnt_names, n_fnt, fn);
		if (idx < 0)
			die("widget \"%s\": unknown font \"%s\"", id, fn);
		o->res_ref = (uint16_t)idx;
		o->extra = intern_string(req_str(w, "text", "label widget"));
		break;
	}
	case WF_W_FRAME_ANIM: {
		const char *nm = req_str(w, "anim", "frame_anim widget");
		int idx = find_name(anm_names, n_anm, nm);
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
		if (pw[i].parent[0] == '\0') {
			pw[i].parent_orig = -1;
			continue;
		}
		int pj = -1;
		for (int k = 0; k < n_pw; k++)
			if (strcmp(pw[k].id, pw[i].parent) == 0) {
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


/* Serialize */
static void write_out(const char *path, uint16_t sw, uint16_t sh) {
	/* final widget array in topo order */
	static wf_widget_t out_w[MAX_WIDGETS];
	static wf_event_t out_e[MAX_EVENTS];
	int n_e = 0;

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
	uint32_t pool_pad = (4 - (pool_len & 3)) & 3;
	uint32_t layout_size = off_strings + pool_len + pool_pad;

	uint8_t *layout = calloc(1, layout_size);
	if (!layout)
		die("out of memory");
	memcpy(layout + off_widgets, out_w, (size_t)n_pw * sizeof(wf_widget_t));
	memcpy(layout + off_images, images, (size_t)n_img * sizeof(wf_image_t));
	memcpy(layout + off_fonts, fonts, (size_t)n_fnt * sizeof(wf_font_t));
	memcpy(layout + off_anims, anims, (size_t)n_anm * sizeof(wf_anim_t));
	memcpy(layout + off_events, out_e, (size_t)n_e * sizeof(wf_event_t));
	memcpy(layout + off_styles, styles, (size_t)n_style * sizeof(wf_style_t));
	memcpy(layout + off_strings, strpool, pool_len);

	wf_header_t h;
	memset(&h, 0, sizeof(h));
	h.magic[0] = WFB_MAGIC0;
	h.magic[1] = WFB_MAGIC1;
	h.magic[2] = WFB_MAGIC2;
	h.magic[3] = WFB_MAGIC3;
	h.version = WFB_VERSION;
	h.flags = 0;
	h.screen_w = sw;
	h.screen_h = sh;
	h.widget_count = (uint16_t)n_pw;
	h.image_count = (uint16_t)n_img;
	h.font_count = (uint16_t)n_fnt;
	h.anim_count = (uint16_t)n_anm;
	h.event_count = (uint16_t)n_e;
	h.style_count = (uint16_t)n_style;
	h.off_widgets = off_widgets;
	h.off_images = off_images;
	h.off_fonts = off_fonts;
	h.off_anims = off_anims;
	h.off_events = off_events;
	h.off_styles = off_styles;
	h.off_strings = off_strings;
	h.layout_size = layout_size;
	h.crc32 = crc32_update(0, layout, layout_size);

	FILE *f = fopen(path, "wb");
	if (!f)
		die("cannot open output \"%s\"", path);
	if (fwrite(&h, sizeof(h), 1, f) != 1 ||
		fwrite(layout, 1, layout_size, f) != layout_size)
		die("write failed");
	fclose(f);
	free(layout);

	fprintf(stderr,
			"wfc: ok -> %s (%u widgets, %u img, %u font, %u anim, %u ev, %u style; "
			"header %zu + layout %u bytes)\n",
			path, (unsigned)n_pw, (unsigned)n_img, (unsigned)n_fnt, (unsigned)n_anm,
			(unsigned)n_e, (unsigned)n_style, sizeof(h), layout_size);
}

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

#define RESOURCE_ADD(fn, p) resource_add((void (*)(void*))fn, p)
static void resource_add(void (*release)(void *), void *ptr) {
	assert(resource_count < MAX_RESOURCES);
	resource_array[resource_count].release = release;
	resource_array[resource_count].ptr = ptr;
	resource_count++;
}

static void on_exit(void) {
	printf("Release all memory resources\n");
	for (size_t i = 0; i < resource_count; i++) {
		struct resource_struct *p = &resource_array[i];
		if (p->release && p->ptr)
			p->release(p->ptr);
	}
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

	atexit(on_exit);
	char *text = read_file(in);
	cJSON *root = cJSON_Parse(text);
	RESOURCE_ADD(free, text);
	RESOURCE_ADD(cJSON_Delete, root);
	if (!root) {
		const char *e = cJSON_GetErrorPtr();
		die("JSON parse error near: %.40s", e ? e : "(unknown)");
	}

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
	write_out(out, (uint16_t)sw, (uint16_t)sh);

	// cJSON_Delete(root);
	// free(text);
	return 0;
}
