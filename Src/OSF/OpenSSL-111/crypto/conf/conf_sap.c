/*
 * Copyright 2002-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
#include "internal/conf.h"
#ifdef _WIN32
	#define strdup _strdup
#endif
/*
 * This is the automatic configuration loader: it is called automatically by
 * OpenSSL when any of a number of standard initialisation functions are
 * called, unless this is overridden by calling OPENSSL_no_config()
 */

static int openssl_configured = 0;

#if OPENSSL_API_COMPAT < 0x10100000L
void OPENSSL_config(const char * appname)
{
	OPENSSL_INIT_SETTINGS settings;
	memzero(&settings, sizeof(settings));
	if(appname != NULL)
		settings.appname = sstrdup(appname);
	settings.flags = DEFAULT_CONF_MFLAGS;
	OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, &settings);
}

#endif

int openssl_config_int(const OPENSSL_INIT_SETTINGS * settings)
{
	int ret = 0;
	const char * filename;
	const char * appname;
	ulong flags;
	if(openssl_configured)
		return 1;
	filename = settings ? settings->filename : NULL;
	appname = settings ? settings->appname : NULL;
	flags = settings ? settings->flags : DEFAULT_CONF_MFLAGS;
#ifdef OPENSSL_INIT_DEBUG
	slfprintf_stderr("OPENSSL_INIT: openssl_config_int(%s, %s, %lu)\n",
	    filename, appname, flags);
#endif
	OPENSSL_load_builtin_modules();
#ifndef OPENSSL_NO_ENGINE
	/* Need to load ENGINEs */
	ENGINE_load_builtin_engines();
#endif
	ERR_clear_error();
#ifndef OPENSSL_SYS_UEFI
	ret = CONF_modules_load_file(filename, appname, flags);
#endif
	openssl_configured = 1;
	return ret;
}

void openssl_no_config_int(void)
{
	openssl_configured = 1;
}
