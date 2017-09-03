/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Maxim Dounin
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

#define NGX_MAX_DYNAMIC_MODULES  128

static ngx_uint_t ngx_module_index(ngx_cycle_t * cycle);
static ngx_uint_t ngx_module_ctx_index(ngx_cycle_t * cycle, ngx_uint_t type, ngx_uint_t index);

ngx_uint_t ngx_max_module;
static ngx_uint_t ngx_modules_n;

ngx_int_t ngx_preinit_modules(void)
{
	ngx_uint_t i;
	for(i = 0; ngx_modules[i]; i++) {
		ngx_modules[i]->index = i;
		ngx_modules[i]->name = ngx_module_names[i];
	}
	ngx_modules_n = i;
	ngx_max_module = ngx_modules_n + NGX_MAX_DYNAMIC_MODULES;
	return NGX_OK;
}

ngx_int_t ngx_cycle_modules(ngx_cycle_t * cycle)
{
	/*
	 * create a list of modules to be used for this cycle,
	 * copy static modules to it
	 */
	cycle->modules = (ngx_module_t **)ngx_pcalloc(cycle->pool, (ngx_max_module + 1) * sizeof(ngx_module_t *));
	if(cycle->modules == NULL) {
		return NGX_ERROR;
	}
	memcpy(cycle->modules, ngx_modules, ngx_modules_n * sizeof(ngx_module_t *));
	cycle->modules_n = ngx_modules_n;
	return NGX_OK;
}

ngx_int_t ngx_init_modules(ngx_cycle_t * cycle)
{
	for(ngx_uint_t i = 0; cycle->modules[i]; i++) {
		if(cycle->modules[i]->init_module) {
			if(cycle->modules[i]->init_module(cycle) != NGX_OK) {
				return NGX_ERROR;
			}
		}
	}
	return NGX_OK;
}

ngx_int_t ngx_count_modules(ngx_cycle_t * cycle, ngx_uint_t type)
{
	ngx_uint_t i;
	ngx_uint_t next = 0;
	ngx_uint_t max = 0;
	// count appropriate modules, set up their indices 
	for(i = 0; cycle->modules[i]; i++) {
		ngx_module_t * module = cycle->modules[i];
		if(module->type == type) {
			if(module->ctx_index != NGX_MODULE_UNSET_INDEX) {
				// if ctx_index was assigned, preserve it 
				SETMAX(max, module->ctx_index);
				if(module->ctx_index == next) {
					next++;
				}
				continue;
			}
			// search for some free index 
			module->ctx_index = ngx_module_ctx_index(cycle, type, next);
			SETMAX(max, module->ctx_index);
			next = module->ctx_index + 1;
		}
	}
	// 
	// make sure the number returned is big enough for previous
	// cycle as well, else there will be problems if the number
	// will be stored in a global variable (as it's used to be)
	// and we'll have to roll back to the previous cycle
	// 
	if(cycle->old_cycle && cycle->old_cycle->modules) {
		for(i = 0; cycle->old_cycle->modules[i]; i++) {
			ngx_module_t * module = cycle->old_cycle->modules[i];
			if(module->type == type) {
				SETMAX(max, module->ctx_index);
			}
		}
	}
	// prevent loading of additional modules 
	cycle->modules_used = 1;
	return (max + 1);
}

ngx_int_t ngx_add_module(ngx_conf_t * cf, ngx_str_t * file, ngx_module_t * pModule, char ** ppOrder)
{
	void * rv;
	ngx_uint_t i, m, before;
	if(cf->cycle->modules_n >= ngx_max_module) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "too many modules loaded");
		return NGX_ERROR;
	}
	if(pModule->version != nginx_version) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "module \"%V\" version %ui instead of %ui", file, pModule->version, (ngx_uint_t)nginx_version);
		return NGX_ERROR;
	}
	if(ngx_strcmp(pModule->signature, NGX_MODULE_SIGNATURE) != 0) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "module \"%V\" is not binary compatible", file);
		return NGX_ERROR;
	}
	for(m = 0; cf->cycle->modules[m]; m++) {
		if(ngx_strcmp(cf->cycle->modules[m]->name, pModule->name) == 0) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "module \"%s\" is already loaded", pModule->name);
			return NGX_ERROR;
		}
	}
	/*
	 * if the module wasn't previously loaded, assign an index
	 */
	if(pModule->index == NGX_MODULE_UNSET_INDEX) {
		pModule->index = ngx_module_index(cf->cycle);
		if(pModule->index >= ngx_max_module) {
			ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "too many modules loaded");
			return NGX_ERROR;
		}
	}
	/*
	 * put the module into the cycle->modules array
	 */
	before = cf->cycle->modules_n;
	if(ppOrder) {
		for(i = 0; ppOrder[i]; i++) {
			if(ngx_strcmp(ppOrder[i], pModule->name) == 0) {
				i++;
				break;
			}
		}
		for(/* void */; ppOrder[i]; i++) {
#if 0
			ngx_log_debug2(NGX_LOG_DEBUG_CORE, cf->log, 0, "module: %s before %s", module->name, order[i]);
#endif
			for(m = 0; m < before; m++) {
				if(ngx_strcmp(cf->cycle->modules[m]->name, ppOrder[i]) == 0) {
					ngx_log_debug3(NGX_LOG_DEBUG_CORE, cf->log, 0, "module: %s before %s:%i", pModule->name, ppOrder[i], m);
					before = m;
					break;
				}
			}
		}
	}
	/* put the module before modules[before] */
	if(before != cf->cycle->modules_n) {
		memmove(&cf->cycle->modules[before + 1], &cf->cycle->modules[before], (cf->cycle->modules_n - before) * sizeof(ngx_module_t *));
	}
	cf->cycle->modules[before] = pModule;
	cf->cycle->modules_n++;
	if(pModule->type == NGX_CORE_MODULE) {
		/*
		 * we are smart enough to initialize core modules;
		 * other modules are expected to be loaded before
		 * initialization - e.g., http modules must be loaded
		 * before http{} block
		 */
		ngx_core_module_t * p_core_module = (ngx_core_module_t *)pModule->ctx;
		if(p_core_module->create_conf) {
			rv = p_core_module->create_conf(cf->cycle);
			if(rv == NULL) {
				return NGX_ERROR;
			}
			cf->cycle->conf_ctx[pModule->index] = (void ***)rv;
		}
	}
	return NGX_OK;
}

static ngx_uint_t ngx_module_index(ngx_cycle_t * cycle)
{
	ngx_uint_t i;
	ngx_uint_t index = 0;
again:
	// find an unused index 
	for(i = 0; cycle->modules[i]; i++) {
		ngx_module_t * module = cycle->modules[i];
		if(module->index == index) {
			index++;
			goto again;
		}
	}
	// check previous cycle 
	if(cycle->old_cycle && cycle->old_cycle->modules) {
		for(i = 0; cycle->old_cycle->modules[i]; i++) {
			ngx_module_t * module = cycle->old_cycle->modules[i];
			if(module->index == index) {
				index++;
				goto again;
			}
		}
	}
	return index;
}

static ngx_uint_t ngx_module_ctx_index(ngx_cycle_t * cycle, ngx_uint_t type, ngx_uint_t index)
{
	ngx_uint_t i;
again:
	// find an unused ctx_index 
	for(i = 0; cycle->modules[i]; i++) {
		ngx_module_t * module = cycle->modules[i];
		if(module->type == type) {
			if(module->ctx_index == index) {
				index++;
				goto again;
			}
		}
	}
	// check previous cycle 
	if(cycle->old_cycle && cycle->old_cycle->modules) {
		for(i = 0; cycle->old_cycle->modules[i]; i++) {
			ngx_module_t * module = cycle->old_cycle->modules[i];
			if(module->type == type) {
				if(module->ctx_index == index) {
					index++;
					goto again;
				}
			}
		}
	}
	return index;
}
