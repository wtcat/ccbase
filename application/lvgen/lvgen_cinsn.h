/*
 * Copyright 2025 wtcat 
 */
#ifndef LVGEN_LVGEN_CINSN_H_
#define LVGEN_LVGEN_CINSN_H_

#include <stdbool.h>
#include <parser/lib/lv_types.h>

#include "sys/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LV_MAX_PATH   255
#define LV_SYMBOL_LEN 64

/*
 * LV types list 
 */
#define LV_TYPE(name)  type__##name
#define LV_TYPE_USED(name)  (LV_TYPE(name) | type__lv_used)
#define LV_PTYPE(name) (LV_TYPE(name) | type__lv_pointer)

#define LV_IS_EXPR(_t) ((_t) & type__lv_expr)

    enum lv_types {
#define type__lv_used    (0x00010000ul)
#define type__lv_pointer (0x00020000ul)
#define type__lv_expr    (0x01000000ul)
#define type__lv_mask    (0x0000FFFF)
    type__void,
    type__lv_obj_t,
    type__lv_style_t,
    type__lv_grad_dsc_t,
    type__lv_event_t,
    type__lv_chart_series_t,
    type__lv_chart_cursor_t,
    type__lv_view__private_t,
    type__lv__end,
};

#ifndef _LV_SOURCE_CODE
const char* lv_type_to_name(int type);

#else 
#define _LV_TYPE_MAP(name) [type__##name] = {#name, #name "*"}
static const struct type_name {
    const char* name;
    const char* pointer;
} lv__typename[] = {
    _LV_TYPE_MAP(void),
    _LV_TYPE_MAP(lv_obj_t),
    _LV_TYPE_MAP(lv_style_t),
    _LV_TYPE_MAP(lv_grad_dsc_t),
    _LV_TYPE_MAP(lv_event_t),
    _LV_TYPE_MAP(lv_chart_series_t),
    _LV_TYPE_MAP(lv_chart_cursor_t),
    _LV_TYPE_MAP(lv_view__private_t),
};

const char* lv_type_to_name(int type) {
    int offset = type & type__lv_mask;
    if (offset < type__lv__end) {
        return (type & type__lv_pointer) ?
            lv__typename[offset].pointer :
            lv__typename[offset].name;
    }
    return NULL;
}
#endif /* _LV_SOURCE_CODE */

struct module_context;
struct func_context;

enum var_scope {
    LV_VAR_SCOPE_FUN_LOCAL,
    LV_VAR_SCOPE_FUN_STATIC,
    LV_VAR_SCOPE_MOD_STATIC,
    LV_VAR_SCOPE_MOD_GLOBAL,
};

TAILQ_HEAD(_fn_list, func_context);
TAILQ_HEAD(_mod_list, module_context);

struct func_callinsn {
#define LV_MAX_ARGS 7
    TAILQ_ENTRY(func_callinsn) link;
    int   rtype;
    char* lvalue;
    union {
        struct {
            char  insn[LV_SYMBOL_LEN];
            char  args[LV_MAX_ARGS][LV_SYMBOL_LEN];
            int   args_num;
        };
        char expr[(LV_MAX_ARGS + 1) * LV_SYMBOL_LEN];
    };
};

struct var_insn {
    char type[LV_SYMBOL_LEN];
    char name[LV_SYMBOL_LEN];
};

struct fn_param {
#define FN_COMPOSITE_ARGS 2
    TAILQ_ENTRY(fn_param) link;
    char key[LV_SYMBOL_LEN];
    char name[LV_SYMBOL_LEN];
    char value[LV_SYMBOL_LEN];
    char pname[LV_SYMBOL_LEN];
};

struct module_depend {
    TAILQ_ENTRY(module_depend) link;
    struct module_context* mod;
};

struct forward_declare {
    TAILQ_ENTRY(forward_declare) link;
    struct func_context* fn;
};

struct module_context {
    TAILQ_ENTRY(module_context) link;
    char     path[LV_MAX_PATH];
    char     name[LV_SYMBOL_LEN];
    TAILQ_HEAD(, forward_declare) ll_fdecls;
    TAILQ_HEAD(, module_depend)   ll_deps;
    struct _fn_list    ll_funs;
    bool     is_view;
};

struct func_context {
    TAILQ_ENTRY(func_context) link;
    char            signature[LV_SYMBOL_LEN];
    struct var_insn args[LV_MAX_ARGS];
    const char*     rvar;
    int             rtype;
    int             args_num;
    int             style_num;
    int             image_num;
    int             font_num;
    int             export_cnt;
    int             ref_cnt;
    int             grad_cnt;
    TAILQ_HEAD(, func_callinsn) ll_insn;
    TAILQ_HEAD(, _lv_obj) ll_objs;
    TAILQ_HEAD(, fn_param) ll_params;

    struct module_context* owner;
};

struct global_context {
    struct _fn_list  ll_funs;
    struct _mod_list ll_modules;

    struct module_context* module;
};

/* Just only for C++ declare */
typedef struct func_callinsn  LvFunctionCallInsn;
typedef struct func_context   LvFunctionContext;
typedef struct module_context LvModuleContext;
typedef struct global_context LvGlobalContext;
typedef struct module_depend  LvModuleDepend;

/*
 * Public API declare 
 */
struct global_context* lvgen_get_context(void);
struct module_context* lvgen_get_module(void);
struct module_context* lvgen_get_module_by_name(const char* name);
struct fn_param* lvgen_new_fnparam(struct func_context* fn, const char* key);
struct fn_param* lvgen_get_fnparam(struct func_context* fn, const char* key);
struct func_context* lvgen_new_func(struct _fn_list* fn_ll, struct module_context *mod,
    const char* signature);
struct func_context* lvgen_new_module_func(struct module_context* mod);
struct func_context* lvgen_new_module_func_named(struct module_context* mod,
    const char* fn_name);
struct func_context* lvgen_new_global_func(void);
struct func_context* lvgen_new_global_func_named(const char* fn_name);
struct func_callinsn* lvgen_new_callinsn(struct func_context* fn, int retype, const char* insn, ...);
struct func_callinsn* lvgen_new_exprinsn(struct func_context* fn, const char* insn, ...);
void lvgen_add_func_argument(struct func_context* fn, const char* type, const char* var);
void lvgen_set_func_rettype(struct func_context* fn, int type);
lv_obj_t* lvgen_new_lvalue(struct func_context* fn, const char* name,
    struct func_callinsn* insn);
struct module_depend* lvgen_new_module_depend(struct module_context* mod,
    struct func_context* depfn);
bool lvgen_func_initialized(struct func_context* fn);

void lvgen_context_init(void);
void lvgen_context_destroy(void);
int lvgen_parse(const char* file, bool is_view);
bool lvgen_generate(void);

bool lvgen_cc_find_sym(const char* ns, const char* key,
    const char** pv, const char** pt);


#define FOREACH_FN_PARAM(_param, _next, _fn) \
    TAILQ_FOREACH_SAFE((_param), &(_fn)->ll_params, link, (_next))

#ifdef __cplusplus
}
#endif
#endif /* LVGEN_LVGEN_CINSN_H_ */
