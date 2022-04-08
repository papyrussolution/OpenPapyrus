/* Copyright (c) 2010 Lars Nordin <Lars.Nordin@SDlabs.se>
 * Copyright (C) 2010 Simon Josefsson <simon@josefsson.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the
 * following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names
 * of any other contributors may be used to endorse or
 * promote products derived from this software without
 * specific prior written permission.
 */
#include "libssh2_priv.h"
#pragma hdrstop

static int _libssh2_initialized = 0;
static int _libssh2_init_flags = 0;

LIBSSH2_API int libssh2_init(int flags)
{
	if(_libssh2_initialized == 0 && !(flags & LIBSSH2_INIT_NO_CRYPTO)) {
		libssh2_crypto_init();
		_libssh2_init_aes_ctr();
	}
	_libssh2_initialized++;
	_libssh2_init_flags |= flags;
	return 0;
}

LIBSSH2_API void libssh2_exit(void)
{
	if(_libssh2_initialized == 0)
		return;

	_libssh2_initialized--;

	if(!(_libssh2_init_flags & LIBSSH2_INIT_NO_CRYPTO)) {
		libssh2_crypto_exit();
	}

	return;
}

void _libssh2_init_if_needed(void)
{
	if(_libssh2_initialized == 0)
		(void)libssh2_init(0);
}

