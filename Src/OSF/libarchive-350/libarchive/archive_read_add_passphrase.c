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

static void add_passphrase_to_tail(ArchiveRead * a, struct archive_read_passphrase * p)
{
	*a->passphrases.last = p;
	a->passphrases.last = &p->next;
	p->next = NULL;
}

static struct archive_read_passphrase * remove_passphrases_from_head(ArchiveRead * a) 
{
	struct archive_read_passphrase * p;
	p = a->passphrases.first;
	if(p)
		a->passphrases.first = p->next;
	return (p);
}

static void insert_passphrase_to_head(ArchiveRead * a, struct archive_read_passphrase * p)
{
	p->next = a->passphrases.first;
	a->passphrases.first = p;
	if(&a->passphrases.first == a->passphrases.last) {
		a->passphrases.last = &p->next;
		p->next = NULL;
	}
}

static struct archive_read_passphrase * new_read_passphrase(ArchiveRead * a, const char * passphrase) 
{
	struct archive_read_passphrase * p = static_cast<struct archive_read_passphrase *>(SAlloc::M(sizeof(*p)));
	if(!p) {
		archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
		return NULL;
	}
	p->passphrase = sstrdup(passphrase);
	if(p->passphrase == NULL) {
		SAlloc::F(p);
		archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
		return NULL;
	}
	return (p);
}

int archive_read_add_passphrase(Archive * _a, const char * passphrase)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	struct archive_read_passphrase * p;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(isempty(passphrase)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Empty passphrase is unacceptable");
		return ARCHIVE_FAILED;
	}
	p = new_read_passphrase(a, passphrase);
	if(!p)
		return ARCHIVE_FATAL;
	add_passphrase_to_tail(a, p);
	return ARCHIVE_OK;
}

int archive_read_set_passphrase_callback(Archive * _a, void * client_data, archive_passphrase_callback * cb)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	a->passphrases.callback = cb;
	a->passphrases.client_data = client_data;
	return ARCHIVE_OK;
}
/*
 * Call this in advance when you start to get a passphrase for decryption
 * for a entry.
 */
void __archive_read_reset_passphrase(ArchiveRead * a)
{
	a->passphrases.candidate = -1;
}

/*
 * Get a passphrase for decryption.
 */
const char * __archive_read_next_passphrase(ArchiveRead * a)
{
	struct archive_read_passphrase * p;
	const char * passphrase;
	if(a->passphrases.candidate < 0) {
		/* Count out how many passphrases we have. */
		int cnt = 0;
		for(p = a->passphrases.first; p; p = p->next)
			cnt++;
		a->passphrases.candidate = cnt;
		p = a->passphrases.first;
	}
	else if(a->passphrases.candidate > 1) {
		/* Rotate a passphrase list. */
		a->passphrases.candidate--;
		p = remove_passphrases_from_head(a);
		add_passphrase_to_tail(a, p);
		/* Pick a new passphrase candidate up. */
		p = a->passphrases.first;
	}
	else if(a->passphrases.candidate == 1) {
		/* This case is that all candidates failed to decrypt. */
		a->passphrases.candidate = 0;
		if(a->passphrases.first->next) {
			/* Rotate a passphrase list. */
			p = remove_passphrases_from_head(a);
			add_passphrase_to_tail(a, p);
		}
		p = NULL;
	}
	else    /* There is no passphrase candidate. */
		p = NULL;

	if(p)
		passphrase = p->passphrase;
	else if(a->passphrases.callback) {
		/* Get a passphrase through a call-back function
		 * since we tried all passphrases out or we don't
		 * have it. */
		passphrase = a->passphrases.callback(&a->archive, a->passphrases.client_data);
		if(passphrase) {
			p = new_read_passphrase(a, passphrase);
			if(!p)
				return NULL;
			insert_passphrase_to_head(a, p);
			a->passphrases.candidate = 1;
		}
	}
	else
		passphrase = NULL;

	return (passphrase);
}
