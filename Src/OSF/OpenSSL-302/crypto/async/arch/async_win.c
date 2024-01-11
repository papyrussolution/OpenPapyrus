/*
 * Copyright 2015-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include <async_local.h> // This must be the first #include file
#ifdef ASYNC_WIN

int ASYNC_is_capable()
{
	return 1;
}

void async_local_cleanup()
{
	async_ctx * ctx = async_get_ctx();
	if(ctx) {
		async_fibre * fibre = &ctx->dispatcher;
		if(fibre && fibre->fibre && fibre->converted) {
			ConvertFiberToThread();
			fibre->fibre = NULL;
		}
	}
}

int async_fibre_init_dispatcher(async_fibre * fibre)
{
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x600
	fibre->fibre = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
#else
	fibre->fibre = ConvertThreadToFiber(NULL);
#endif
	if(fibre->fibre == NULL) {
		fibre->converted = 0;
		fibre->fibre = GetCurrentFiber();
		if(fibre->fibre == NULL)
			return 0;
	}
	else {
		fibre->converted = 1;
	}

	return 1;
}

VOID CALLBACK async_start_func_win(PVOID unused)
{
	async_start_func();
}

#endif
