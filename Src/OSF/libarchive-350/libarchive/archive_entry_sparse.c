/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * Copyright (c) 2010-2011 Michihiro NAKAJIMA
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
#include "archive_entry_private.h"
/*
 * sparse handling
 */
void archive_entry_sparse_clear(ArchiveEntry * entry)
{
	while(entry->sparse_head) {
		struct ae_sparse * sp = entry->sparse_head->next;
		SAlloc::F(entry->sparse_head);
		entry->sparse_head = sp;
	}
	entry->sparse_tail = NULL;
}

void archive_entry_sparse_add_entry(ArchiveEntry * entry, la_int64_t offset, la_int64_t length)
{
	struct ae_sparse * sp;
	if(offset < 0 || length < 0)
		return; // Invalid value
	if(offset > INT64_MAX - length || offset + length > archive_entry_size(entry))
		return; /* A value of "length" parameter is too large. */
	if((sp = entry->sparse_tail) != NULL) {
		if(sp->offset + sp->length > offset)
			return; // Invalid value
		if(sp->offset + sp->length == offset) {
			if(sp->offset + sp->length + length < 0)
				return; /* A value of "length" parameter is too large. */
			/* Expand existing sparse block size. */
			sp->length += length;
			return;
		}
	}
	if((sp = (struct ae_sparse *)SAlloc::M(sizeof(*sp))) == NULL)
		return; /* XXX Error XXX */
	sp->offset = offset;
	sp->length = length;
	sp->next = NULL;
	if(entry->sparse_head == NULL)
		entry->sparse_head = entry->sparse_tail = sp;
	else {
		// Add a new sparse block to the tail of list
		if(entry->sparse_tail)
			entry->sparse_tail->next = sp;
		entry->sparse_tail = sp;
	}
}
/*
 * returns number of the sparse entries
 */
int archive_entry_sparse_count(ArchiveEntry * entry)
{
	struct ae_sparse * sp;
	int count = 0;
	for(sp = entry->sparse_head; sp != NULL; sp = sp->next)
		count++;
	/*
	 * Sanity check if this entry is exactly sparse.
	 * If amount of sparse blocks is just one and it indicates the whole
	 * file data, we should remove it and return zero.
	 */
	if(count == 1) {
		sp = entry->sparse_head;
		if(sp->offset == 0 && sp->length >= archive_entry_size(entry)) {
			count = 0;
			archive_entry_sparse_clear(entry);
		}
	}
	return (count);
}

int archive_entry_sparse_reset(ArchiveEntry * entry)
{
	entry->sparse_p = entry->sparse_head;
	return archive_entry_sparse_count(entry);
}

int archive_entry_sparse_next(ArchiveEntry * entry, la_int64_t * offset, la_int64_t * length)
{
	if(entry->sparse_p) {
		*offset = entry->sparse_p->offset;
		*length = entry->sparse_p->length;
		entry->sparse_p = entry->sparse_p->next;
		return ARCHIVE_OK;
	}
	else {
		*offset = 0;
		*length = 0;
		return ARCHIVE_WARN;
	}
}
