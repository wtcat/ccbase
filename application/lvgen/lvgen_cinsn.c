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

#define LIST_REMOVE_NODE(_head, _node) \
do { \
    TAILQ_REMOVE((_head), (_node), link); \
    lv_free((_node)); \
} while (0)

#define LIST_CLEAR(_head, _type) \
do { \
    struct _type* __p, *__n; \
    TAILQ_FOREACH_SAFE(__p, (_head), link, __n) { \
        LIST_REMOVE_NODE((_head), __p); \
    } \
} while(0)

#define LIST_NEW_NODE(_head, _type, _ret, _clr) \
do { \
    struct _type *__node = lv_malloc(sizeof(struct _type)); \
    if (__node != NULL) { \
        if (_clr) lv_memset(__node, 0, sizeof(struct _type)); \
        TAILQ_INSERT_TAIL((_head), __node, link); \
    } \
    (_ret) = __node; \
} while (0)

static void lvgen_func_clear(struct _fn_list *list) {
    struct func_context* fn;
    TAILQ_FOREACH(fn, list, link) {
        LIST_CLEAR(&fn->ll_insn, func_callinsn);
        LIST_CLEAR(&fn->ll_objs, _lv_obj);
        LIST_CLEAR(&fn->ll_params, fn_param);
    }
    LIST_CLEAR(list, func_context);
}

static void lvgen_module_clear(struct module_context* mod) {
    lvgen_func_clear(&mod->ll_funs);
    LIST_CLEAR(&mod->ll_fdecls, forward_declare);
    LIST_CLEAR(&mod->ll_deps, module_depend);
}

struct global_context* lvgen_get_context(void) {
    return &lvgen_context;
}

static struct module_context* lvgen_new_module(const char* file, bool is_view) {
    struct _mod_list* ll_modules = &lvgen_get_context()->ll_modules;
    struct module_context* mod;
    char modname[LV_SYMBOL_LEN];

    /* Remove extension name ".xml" */
    lv_strlcpy(modname, lv_basename(file), LV_SYMBOL_LEN);
    modname[lv_strlen(modname) - 4] = '\0';

    mod = lvgen_get_module_by_name(modname);
    if (mod == NULL) {
        LIST_NEW_NODE(ll_modules, module_context, mod, false);
        if (mod != NULL) {
            lv_strlcpy(mod->name, modname, LV_SYMBOL_LEN);
            lv_strlcpy(mod->path, file, sizeof(mod->path));
            TAILQ_INIT(&mod->ll_fdecls);
            TAILQ_INIT(&mod->ll_funs);
            TAILQ_INIT(&mod->ll_deps);
            mod->is_view = is_view;
        }
    }

    lvgen_get_context()->module = mod;
    return mod;
}

struct module_context* lvgen_get_module(void) {
    return lvgen_get_context()->module;
}

struct module_context* lvgen_get_module_by_name(const char *name) {
    struct module_context* mod;

    TAILQ_FOREACH(mod, &lvgen_get_context()->ll_modules, link) {
        if (!lv_strcmp(mod->name, name))
            return mod;
    }
    return NULL;
}

struct module_depend *lvgen_new_module_depend(struct module_context* mod, 
    struct func_context* depfn) {
    struct module_depend* dep;

    if (mod == depfn->owner) {
        depfn->ref_cnt++;
        return NULL;
    }

    TAILQ_FOREACH(dep, &mod->ll_deps, link) {
        if (dep->mod == depfn->owner)
            return dep;
    }

    LIST_NEW_NODE(&mod->ll_deps, module_depend, dep, false);
    if (dep != NULL) {
        dep->mod = depfn->owner;
        depfn->export_cnt++;
        depfn->ref_cnt++;
    }

    return dep;
}

struct func_context* lvgen_new_global_func(void) {
    return lvgen_new_func(&lvgen_get_context()->ll_funs, NULL, NULL);
}

struct func_context* lvgen_new_global_func_named(const char *fn_name) {
    return lvgen_new_func(&lvgen_get_context()->ll_funs, NULL, fn_name);
}

struct func_context* lvgen_new_module_func(struct module_context* mod) {
    return lvgen_new_module_func_named(mod, NULL);
}

bool lvgen_func_initialized(struct func_context* fn) {
    return fn->signature[0] != '\0';
}

struct func_context* lvgen_new_module_func_named(struct module_context* mod,
    const char *fn_name) {
    return lvgen_new_func(&mod->ll_funs, mod, fn_name);
}

struct fn_param* lvgen_get_fnparam(struct func_context* fn, const char* key) {
    struct fn_param* param;

    TAILQ_FOREACH(param, &fn->ll_params, link) {
        if (!lv_strcmp(key, param->key))
            return param;
    }

    return NULL;
}

struct fn_param* lvgen_new_fnparam(struct func_context* fn, const char *key) {
    struct fn_param* param;

    if (key != NULL) {
        param = lvgen_get_fnparam(fn, key);
        if (param)
            return param;
    }

    LIST_NEW_NODE(&fn->ll_params, fn_param, param, true);
    if (param != NULL)
        lv_strlcpy(param->key, key, LV_SYMBOL_LEN);

    return param;
}

struct func_context* lvgen_new_func(struct _fn_list *fn_ll, struct module_context *mod,
    const char *signature) {
    struct func_context* fn;

    if (signature != NULL) {
        TAILQ_FOREACH(fn, fn_ll, link) {
            if (!lv_strcmp(signature, fn->signature))
                return fn;
        }
    }

    LIST_NEW_NODE(fn_ll, func_context, fn, true);
    if (fn != NULL) {
        TAILQ_INIT(&fn->ll_insn);
        TAILQ_INIT(&fn->ll_objs);
        TAILQ_INIT(&fn->ll_params);
        fn->owner = mod;
        if (signature != NULL)
            lv_strlcpy(fn->signature, signature, sizeof(fn->signature));
    }

    return fn;
}

lv_obj_t* lvgen_new_lvalue(struct func_context* fn, const char *name, 
    struct func_callinsn *insn) {
    lv_obj_t* obj;
    int no = 0;

    TAILQ_FOREACH(obj, &fn->ll_objs, link) {
        if (!lv_memcmp(obj->base.name, name, lv_strlen(name)))
            no++;
    }

    LIST_NEW_NODE(&fn->ll_objs, _lv_obj, obj, false);
    if (obj != NULL) {
        lv_snprintf(obj->base.name, sizeof(obj->base.name), "%s_%d", name, no);

        /* Attach left-value to instruction */
        insn->lvalue = obj->base.name;
    }
  
    return obj;
}

void lvgen_add_func_argument(struct func_context* fn, const char *type, const char *var) {
    int n = fn->args_num;

    assert(n < LV_MAX_ARGS);
    lv_strlcpy(fn->args[n].name, var, LV_SYMBOL_LEN);
    lv_strlcpy(fn->args[n].type, type, LV_SYMBOL_LEN);
    fn->args_num = n + 1;
}

void lvgen_set_func_rettype(struct func_context* fn, int type) {
    fn->rtype = type;
}

struct func_callinsn* lvgen_new_callinsn(struct func_context* fn,
    int retype, const char *insn, ...) {
    struct func_callinsn* pins;

    LIST_NEW_NODE(&fn->ll_insn, func_callinsn, pins, true);
    if (pins != NULL) {
        va_list ap;

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

struct func_callinsn* lvgen_new_exprinsn(struct func_context* fn, 
    const char* insn, ...) {
    struct func_callinsn* pins;

    LIST_NEW_NODE(&fn->ll_insn, func_callinsn, pins, false);
    if (pins != NULL) {
        va_list ap;

        pins->rtype = type__lv_expr;
        va_start(ap, insn);
        lv_vsnprintf(pins->expr, sizeof(pins->expr), insn, ap);
        va_end(ap);
    }
    return pins;
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
        LIST_REMOVE_NODE(&lvgen_get_context()->ll_modules, mod);
    }

    return ret;
}

bool lvgen_generate(void) {
    struct global_context* ctx = lvgen_get_context();
    struct module_context* mod;

    TAILQ_FOREACH(mod, &ctx->ll_modules, link) {
        if (mod->is_view && !lv_xml_create(NULL, mod->name, NULL)) {
            printf("Failed to generate moudle(%s@ %s)\n", mod->name, mod->path);
        }
    }
    return true;
}

void lvgen_context_init(void) {
    TAILQ_INIT(&lvgen_context.ll_funs);
    TAILQ_INIT(&lvgen_context.ll_modules);
    lv_xml_init();
}

void lvgen_context_destroy(void) {
    struct global_context* ctx = &lvgen_context;
    struct module_context* mod;

    lvgen_func_clear(&ctx->ll_funs);
    TAILQ_FOREACH(mod, &ctx->ll_modules, link) {
        lv_xml_component_unregister(mod->name);
        lvgen_module_clear(mod);
    }
    LIST_CLEAR(&ctx->ll_modules, module_context);
}
