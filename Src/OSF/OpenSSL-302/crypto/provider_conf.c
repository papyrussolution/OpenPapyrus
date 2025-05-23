/*
 * Copyright 2019-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include "internal/provider.h"
#include "provider_local.h"

DEFINE_STACK_OF(OSSL_PROVIDER)

/* PROVIDER config module */

typedef struct {
	CRYPTO_RWLOCK * lock;
	STACK_OF(OSSL_PROVIDER) *activated_providers;
} PROVIDER_CONF_GLOBAL;

static void * prov_conf_ossl_ctx_new(OSSL_LIB_CTX * libctx)
{
	PROVIDER_CONF_GLOBAL * pcgbl = (PROVIDER_CONF_GLOBAL*)OPENSSL_zalloc(sizeof(*pcgbl));
	if(pcgbl == NULL)
		return NULL;
	pcgbl->lock = CRYPTO_THREAD_lock_new();
	if(pcgbl->lock == NULL) {
		OPENSSL_free(pcgbl);
		return NULL;
	}
	return pcgbl;
}

static void prov_conf_ossl_ctx_free(void * vpcgbl)
{
	PROVIDER_CONF_GLOBAL * pcgbl = (PROVIDER_CONF_GLOBAL *)vpcgbl;
	sk_OSSL_PROVIDER_pop_free(pcgbl->activated_providers, ossl_provider_free);
	OSSL_TRACE(CONF, "Cleaned up providers\n");
	CRYPTO_THREAD_lock_free(pcgbl->lock);
	OPENSSL_free(pcgbl);
}

static const OSSL_LIB_CTX_METHOD provider_conf_ossl_ctx_method = {
	/* Must be freed before the provider store is freed */
	OSSL_LIB_CTX_METHOD_PRIORITY_2,
	prov_conf_ossl_ctx_new,
	prov_conf_ossl_ctx_free,
};

static const char * skip_dot(const char * name)
{
	const char * p = sstrchr(name, '.');
	if(p)
		return p + 1;
	return name;
}

static int provider_conf_params(OSSL_PROVIDER * prov, OSSL_PROVIDER_INFO * provinfo, const char * name, const char * value, const CONF * cnf)
{
	int ok = 1;
	STACK_OF(CONF_VALUE) * sect = NCONF_get_section(cnf, value);
	if(sect != NULL) {
		int i;
		char buffer[512];
		size_t buffer_len = 0;
		OSSL_TRACE1(CONF, "Provider params: start section %s\n", value);
		if(name != NULL) {
			OPENSSL_strlcpy(buffer, name, sizeof(buffer));
			OPENSSL_strlcat(buffer, ".", sizeof(buffer));
			buffer_len = strlen(buffer);
		}
		for(i = 0; i < sk_CONF_VALUE_num(sect); i++) {
			CONF_VALUE * sectconf = sk_CONF_VALUE_value(sect, i);
			if(buffer_len + strlen(sectconf->name) >= sizeof(buffer))
				return 0;
			buffer[buffer_len] = '\0';
			OPENSSL_strlcat(buffer, sectconf->name, sizeof(buffer));
			if(!provider_conf_params(prov, provinfo, buffer, sectconf->value, cnf))
				return 0;
		}
		OSSL_TRACE1(CONF, "Provider params: finish section %s\n", value);
	}
	else {
		OSSL_TRACE2(CONF, "Provider params: %s = %s\n", name, value);
		if(prov)
			ok = ossl_provider_add_parameter(prov, name, value);
		else
			ok = ossl_provider_info_add_parameter(provinfo, name, value);
	}
	return ok;
}

static int prov_already_activated(const char * name, STACK_OF(OSSL_PROVIDER) * activated)
{
	int i, max;
	if(activated == NULL)
		return 0;
	max = sk_OSSL_PROVIDER_num(activated);
	for(i = 0; i < max; i++) {
		OSSL_PROVIDER * tstprov = sk_OSSL_PROVIDER_value(activated, i);
		if(strcmp(OSSL_PROVIDER_get0_name(tstprov), name) == 0) {
			return 1;
		}
	}
	return 0;
}

static int provider_conf_load(OSSL_LIB_CTX * libctx, const char * name, const char * value, const CONF * cnf)
{
	int i;
	STACK_OF(CONF_VALUE) *ecmds;
	int soft = 0;
	OSSL_PROVIDER * prov = NULL, * actual = NULL;
	const char * path = NULL;
	long activate = 0;
	int ok = 0;
	name = skip_dot(name);
	OSSL_TRACE1(CONF, "Configuring provider %s\n", name);
	/* Value is a section containing PROVIDER commands */
	ecmds = NCONF_get_section(cnf, value);
	if(!ecmds) {
		ERR_raise_data(ERR_LIB_CRYPTO, CRYPTO_R_PROVIDER_SECTION_ERROR, "section=%s not found", value);
		return 0;
	}
	/* Find the needed data first */
	for(i = 0; i < sk_CONF_VALUE_num(ecmds); i++) {
		CONF_VALUE * ecmd = sk_CONF_VALUE_value(ecmds, i);
		const char * confname = skip_dot(ecmd->name);
		const char * confvalue = ecmd->value;

		OSSL_TRACE2(CONF, "Provider command: %s = %s\n",
		    confname, confvalue);

		/* First handle some special pseudo confs */

		/* Override provider name to use */
		if(strcmp(confname, "identity") == 0)
			name = confvalue;
		else if(strcmp(confname, "soft_load") == 0)
			soft = 1;
		/* Load a dynamic PROVIDER */
		else if(strcmp(confname, "module") == 0)
			path = confvalue;
		else if(strcmp(confname, "activate") == 0)
			activate = 1;
	}
	if(activate) {
		PROVIDER_CONF_GLOBAL * pcgbl = (PROVIDER_CONF_GLOBAL *)ossl_lib_ctx_get_data(libctx, OSSL_LIB_CTX_PROVIDER_CONF_INDEX, &provider_conf_ossl_ctx_method);
		if(pcgbl == NULL || !CRYPTO_THREAD_write_lock(pcgbl->lock)) {
			ERR_raise(ERR_LIB_CRYPTO, ERR_R_INTERNAL_ERROR);
			return 0;
		}
		if(!prov_already_activated(name, pcgbl->activated_providers)) {
			/*
			 * There is an attempt to activate a provider, so we should disable
			 * loading of fallbacks. Otherwise a misconfiguration could mean the
			 * intended provider does not get loaded. Subsequent fetches could
			 * then fallback to the default provider - which may be the wrong
			 * thing.
			 */
			if(!ossl_provider_disable_fallback_loading(libctx)) {
				CRYPTO_THREAD_unlock(pcgbl->lock);
				ERR_raise(ERR_LIB_CRYPTO, ERR_R_INTERNAL_ERROR);
				return 0;
			}
			prov = ossl_provider_find(libctx, name, 1);
			if(prov == NULL)
				prov = ossl_provider_new(libctx, name, NULL, 1);
			if(prov == NULL) {
				CRYPTO_THREAD_unlock(pcgbl->lock);
				if(soft)
					ERR_clear_error();
				return 0;
			}

			if(path != NULL)
				ossl_provider_set_module_path(prov, path);

			ok = provider_conf_params(prov, NULL, NULL, value, cnf);

			if(ok) {
				if(!ossl_provider_activate(prov, 1, 0)) {
					ok = 0;
				}
				else if(!ossl_provider_add_to_store(prov, &actual, 0)) {
					ossl_provider_deactivate(prov, 1);
					ok = 0;
				}
				else if(actual != prov
				    && !ossl_provider_activate(actual, 1, 0)) {
					ossl_provider_free(actual);
					ok = 0;
				}
				else {
					if(pcgbl->activated_providers == NULL)
						pcgbl->activated_providers = sk_OSSL_PROVIDER_new_null();
					if(pcgbl->activated_providers == NULL
					    || !sk_OSSL_PROVIDER_push(pcgbl->activated_providers,
					    actual)) {
						ossl_provider_deactivate(actual, 1);
						ossl_provider_free(actual);
						ok = 0;
					}
					else {
						ok = 1;
					}
				}
			}
			if(!ok)
				ossl_provider_free(prov);
		}
		CRYPTO_THREAD_unlock(pcgbl->lock);
	}
	else {
		OSSL_PROVIDER_INFO entry;
		memzero(&entry, sizeof(entry));
		ok = 1;
		if(name != NULL) {
			entry.name = OPENSSL_strdup(name);
			if(entry.name == NULL) {
				ERR_raise(ERR_LIB_CRYPTO, ERR_R_MALLOC_FAILURE);
				ok = 0;
			}
		}
		if(ok && path != NULL) {
			entry.path = OPENSSL_strdup(path);
			if(entry.path == NULL) {
				ERR_raise(ERR_LIB_CRYPTO, ERR_R_MALLOC_FAILURE);
				ok = 0;
			}
		}
		if(ok)
			ok = provider_conf_params(NULL, &entry, NULL, value, cnf);
		if(ok && (entry.path != NULL || entry.parameters != NULL))
			ok = ossl_provider_info_add_to_store(libctx, &entry);
		if(!ok || (entry.path == NULL && entry.parameters == NULL)) {
			ossl_provider_info_clear(&entry);
		}
	}

	/*
	 * Even if ok is 0, we still return success. Failure to load a provider is
	 * not fatal. We want to continue to load the rest of the config file.
	 */
	return 1;
}

static int provider_conf_init(CONF_IMODULE * md, const CONF * cnf)
{
	STACK_OF(CONF_VALUE) *elist;
	CONF_VALUE * cval;
	int i;

	OSSL_TRACE1(CONF, "Loading providers module: section %s\n",
	    CONF_imodule_get_value(md));

	/* Value is a section containing PROVIDERs to configure */
	elist = NCONF_get_section(cnf, CONF_imodule_get_value(md));

	if(!elist) {
		ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_PROVIDER_SECTION_ERROR);
		return 0;
	}

	for(i = 0; i < sk_CONF_VALUE_num(elist); i++) {
		cval = sk_CONF_VALUE_value(elist, i);
		if(!provider_conf_load(NCONF_get0_libctx((CONF*)cnf),
		    cval->name, cval->value, cnf))
			return 0;
	}

	return 1;
}

void ossl_provider_add_conf_module(void)
{
	OSSL_TRACE(CONF, "Adding config module 'providers'\n");
	CONF_module_add("providers", provider_conf_init, NULL);
}
