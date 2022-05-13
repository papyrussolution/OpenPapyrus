/*-
 * Copyright (c) 2014 Michihiro NAKAJIMA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

int archive_write_set_passphrase(struct archive * _a, const char * p)
{
	struct archive_write * a = (struct archive_write *)_a;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(isempty(p)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Empty passphrase is unacceptable");
		return ARCHIVE_FAILED;
	}
	SAlloc::F(a->passphrase);
	a->passphrase = sstrdup(p);
	if(a->passphrase == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate data for passphrase");
		return ARCHIVE_FATAL;
	}
	return ARCHIVE_OK;
}

int archive_write_set_passphrase_callback(struct archive * _a, void * client_data, archive_passphrase_callback * cb)
{
	struct archive_write * a = (struct archive_write *)_a;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	a->passphrase_callback = cb;
	a->passphrase_client_data = client_data;
	return ARCHIVE_OK;
}

const char * __archive_write_get_passphrase(struct archive_write * a)
{
	if(a->passphrase != NULL)
		return (a->passphrase);
	if(a->passphrase_callback != NULL) {
		const char * p;
		p = a->passphrase_callback(&a->archive, a->passphrase_client_data);
		if(p) {
			a->passphrase = sstrdup(p);
			if(a->passphrase == NULL) {
				archive_set_error(&a->archive, ENOMEM, "Can't allocate data for passphrase");
				return NULL;
			}
			return (a->passphrase);
		}
	}
	return NULL;
}
