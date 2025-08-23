/*
 * Copyright 2025 wtcat 
 */
#ifndef LVGEN_LVGEN_CINSN_H_
#define LVGEN_LVGEN_CINSN_H_

#include <stdbool.h>
#include <parser/lib/lv_ll.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LV_MAX_PATH   255
#define LV_SYMBOL_LEN 64

/*
 * LV types list 
 */
#define LV_TYPE(name)  type__##name
#define LV_PTYPE(name) (LV_TYPE(name) | type__lv_pointer)
enum lv_types {
#define type__lv_pointer (0x10000000ul)
#define type__lv_mask    (~type__lv_pointer)
    type__lv_obj_t,
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
    _LV_TYPE_MAP(lv_obj_t),
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


enum var_scope {
    LV_VAR_SCOPE_FUN_LOCAL,
    LV_VAR_SCOPE_FUN_STATIC,
    LV_VAR_SCOPE_MOD_STATIC,
    LV_VAR_SCOPE_MOD_GLOBAL,
};

struct func_callinsn {
#define LV_MAX_ARGS 6
    int   rtype;
    char  rval[LV_SYMBOL_LEN];
    char  signature[LV_SYMBOL_LEN];
    char  args[LV_MAX_ARGS][LV_SYMBOL_LEN];
    int   args_num;
    bool  need_define;
};

struct var_insn {
    int  type;
    char name[LV_SYMBOL_LEN];
};

struct module_const {
    char name[LV_SYMBOL_LEN];
    char value[LV_SYMBOL_LEN];
};

struct func_context {
    int  rtype;
    char id[LV_SYMBOL_LEN];
    char signature[LV_SYMBOL_LEN];
    struct var_insn args[LV_MAX_ARGS];
    int args_num;
    lv_ll_t ll_insn;
};

struct module_context {
    char path[LV_MAX_PATH];
    char name[LV_SYMBOL_LEN];
    lv_ll_t ll_consts;
    lv_ll_t ll_funs;
    bool is_view;
};

struct global_context {
    lv_ll_t ll_funs;
    lv_ll_t ll_modules;
};

/*
 * Public API declare 
 */
struct global_context* lvgen_get_context(void);
struct module_context* lvgen_get_module(void);
struct func_context* lvgen_new_func(lv_ll_t* fn_ll);
int lvgen_parse(const char* file, bool is_view);
bool lvgen_generate(void);
void lvgen_context_destroy(void);

bool lvgen_cc_find_sym(const char* ns, const char* key,
    const char** pv, const char** pt);

#ifdef __cplusplus
}
#endif
#endif /* LVGEN_LVGEN_CINSN_H_ */
