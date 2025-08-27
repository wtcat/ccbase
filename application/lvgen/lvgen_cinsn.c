/*
 * Copyright 2025 wtcat 
 */

#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#define _LV_SOURCE_CODE
#include "lvgen_cinsn.h"

#include "parser/lib/lv_path.h"
#include "parser/lib/lv_mem.h"
#include "parser/lib/lv_string.h"
#include "parser/lv_xml_component.h"
#include "parser/lv_xml.h"

#include "parser/lib/lv_stdio.h"

static struct global_context lvgen_context;

static void lvgen_func_clear(lv_ll_t* fn_ll) {
    struct func_context* fn;

    LV_LL_READ(fn_ll, fn) {
        lv_ll_clear(&fn->ll_insn);
        lv_ll_clear(&fn->ll_objs);
    }
    lv_ll_clear(fn_ll);
}

static void lvgen_module_clear(struct module_context* mod) {
    lv_ll_clear(&mod->ll_consts);
    lvgen_func_clear(&mod->ll_funs);
}

struct global_context* lvgen_get_context(void) {
    return &lvgen_context;
}

static struct module_context* lvgen_new_module(const char* file, bool is_view) {
    struct module_context* mod = lv_ll_ins_tail(&lvgen_get_context()->ll_modules);
    if (mod != NULL) {
        /* Remove extension name ".xml" */
        lv_strlcpy(mod->name, lv_basename(file), LV_SYMBOL_LEN);
        mod->name[lv_strlen(mod->name) - 4] = '\0';

        lv_strlcpy(mod->path, file, sizeof(mod->path));
        lv_ll_init(&mod->ll_consts, sizeof(struct module_const));
        lv_ll_init(&mod->ll_funs, sizeof(struct func_context));
        mod->is_view = is_view;
    }
    return mod;
}

struct module_context* lvgen_get_module(void) {
    return lv_ll_get_tail(&lvgen_get_context()->ll_modules);
}

struct func_context* lvgen_new_global_func(void) {
    return lvgen_new_func(&lvgen_get_context()->ll_funs);
}

struct func_context* lvgen_new_module_func(struct module_context* mod) {
    return lvgen_new_func(&mod->ll_funs);
}

struct func_context* lvgen_new_func(lv_ll_t *fn_ll) {
    struct func_context* fn = lv_ll_ins_tail(fn_ll);
    if (fn != NULL) {
        lv_memset(fn, 0, sizeof(*fn));
        lv_ll_init(&fn->ll_insn, sizeof(struct func_callinsn));
        lv_ll_init(&fn->ll_objs, sizeof(lv_obj_t));
    }
    return fn;
}

lv_obj_t* lvgen_new_lvalue(struct func_context* fn, const char *name, 
    struct func_callinsn *insn) {
    lv_obj_t* obj;
    int no = 0;

    LV_LL_READ(&fn->ll_objs, obj) {
        if (!lv_memcmp(obj->base.name, name, lv_strlen(name)))
            no++;
    }

    obj = lv_ll_ins_tail(&fn->ll_objs);
    if (obj != NULL) {
        lv_snprintf(obj->base.name, sizeof(obj->base.name), "%s_%d", name, no);

        /* Attach left-value to instruction */
        insn->lvalue = obj->base.name;
    }
  
    return obj;
}

void lvgen_add_func_argument(struct func_context* fn, int type, const char *var) {
    int n = fn->args_num;

    assert(n < LV_MAX_ARGS);
    lv_strlcpy(fn->args[n].name, var, sizeof(fn->args[0].name));
    fn->args[n].type = type;
    fn->args_num = n + 1;
}

struct func_callinsn* lvgen_new_callinsn(struct func_context* fn,
    int retype, const char *insn, ...) {
    struct func_callinsn* pins = lv_ll_ins_tail(&fn->ll_insn);
    if (pins != NULL) {
        va_list ap;

        lv_memset(pins, 0, sizeof(*pins));
        va_start(ap, insn);
        for (int i = 0; i < LV_MAX_ARGS; i++) {
            const char* parg = va_arg(ap, const char*);
            if (parg == NULL) {
                lv_strlcpy(pins->insn, insn, sizeof(pins->insn));
                pins->rtype = retype;
                break;
            }

            lv_strlcpy(pins->args[i], parg, sizeof(pins->args[0]));
            pins->args_num++;
        }
        va_end(ap);
        return pins;
    }

    return NULL;
}

int lvgen_parse(const char* file, bool is_view) {
    struct module_context* mod;
    int ret;

    mod = lvgen_new_module(file, is_view);
    if (mod == NULL)
        return -ENOMEM;
    
    ret = lv_xml_component_register_from_file(file);
    if (ret < 0 && mod) {
        lvgen_module_clear(mod);
        lv_ll_remove(&lvgen_get_context()->ll_modules, mod);
        lv_free(mod);
    }

    return ret;
}

bool lvgen_generate(void) {
    struct global_context* ctx = lvgen_get_context();
    struct module_context* mod;

    LV_LL_READ(&ctx->ll_modules, mod) {
        if (mod->is_view && !lv_xml_create(NULL, mod->name, NULL)) {
            printf("Failed to generate moudle(%s@ %s)\n", mod->name, mod->path);
        }
    }
    return true;
}

void lvgen_context_init(void) {
    lv_ll_init(&lvgen_context.ll_funs, sizeof(struct func_context));
    lv_ll_init(&lvgen_context.ll_modules, sizeof(struct module_context));
    lv_xml_init();
}

void lvgen_context_destroy(void) {
    struct global_context* ctx = &lvgen_context;
    struct module_context* mod;

    if (ctx->ll_funs.n_size > 0) {
        lvgen_func_clear(&ctx->ll_funs);
        LV_LL_READ(&ctx->ll_modules, mod) {
            lv_xml_component_unregister(mod->name);
            lvgen_module_clear(mod);
        }
        lv_ll_clear(&ctx->ll_modules);
    }
}
