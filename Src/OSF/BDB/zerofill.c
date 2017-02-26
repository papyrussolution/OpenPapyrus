/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
// @v9.5.5 #include "dbinc/db_page.h"
// @v9.5.5 #include "dbinc/lock.h"
// @v9.5.5 #include "dbinc/mp.h"
// @v9.5.5 #include "dbinc/crypto.h"
// @v9.5.5 #include "dbinc/btree.h"
// @v9.5.5 #include "dbinc/hash.h"
#pragma hdrstop
/*
 * __db_zero_fill --
 *	Zero out bytes in the file.
 *
 *	Pages allocated by writing pages past end-of-file are not zeroed,
 *	on some systems.  Recovery could theoretically be fooled by a page
 *	showing up that contained garbage.  In order to avoid this, we
 *	have to write the pages out to disk, and flush them.  The reason
 *	for the flush is because if we don't sync, the allocation of another
 *	page subsequent to this one might reach the disk first, and if we
 *	crashed at the right moment, leave us with this page as the one
 *	allocated by writing a page past it in the file.
 *
 * PUBLIC: int __db_zero_fill(ENV *, DB_FH *);
 */
int __db_zero_fill(ENV * env, DB_FH * fhp)
{
#ifdef HAVE_FILESYSTEM_NOTZERO
	off_t stat_offset;
	size_t blen, nw;
	uint32 bytes, mbytes;
	int group_sync, ret;
	uint8 * bp;
	/* Calculate the byte offset of the next write. */
	off_t write_offset = (off_t)fhp->pgno*fhp->pgsize+fhp->offset;
	/* Stat the file. */
	if((ret = __os_ioinfo(env, NULL, fhp, &mbytes, &bytes, NULL)) != 0)
		return ret;
	stat_offset = (off_t)mbytes*MEGABYTE+bytes;
	/* Check if the file is large enough. */
	if(stat_offset >= write_offset)
		return 0;
	/* Get a large buffer if we're writing lots of data. */
 #undef  ZF_LARGE_WRITE
 #define ZF_LARGE_WRITE  (64*1024)
	if((ret = __os_calloc(env, 1, ZF_LARGE_WRITE, &bp)) != 0)
		return ret;
	blen = ZF_LARGE_WRITE;
	/* Seek to the current end of the file. */
	if((ret = __os_seek(env, fhp, mbytes, MEGABYTE, bytes)) != 0)
		goto err;
	/*
	 * Hash is the only access method that allocates groups of pages.  Hash
	 * uses the existence of the last page in a group to signify the entire
	 * group is OK; so, write all the pages but the last one in the group,
	 * flush them to disk, then write the last one to disk and flush it.
	 */
	for(group_sync = 0; stat_offset < write_offset; group_sync = 1) {
		if(write_offset-stat_offset <= (off_t)blen) {
			blen = (size_t)(write_offset-stat_offset);
			if(group_sync && (ret = __os_fsync(env, fhp)) != 0)
				goto err;
		}
		if((ret = __os_physwrite(env, fhp, bp, blen, &nw)) != 0)
			goto err;
		stat_offset += blen;
	}
	if((ret = __os_fsync(env, fhp)) != 0)
		goto err;
	/* Seek back to where we started. */
	mbytes = (uint32)(write_offset/MEGABYTE);
	bytes = (uint32)(write_offset%MEGABYTE);
	ret = __os_seek(env, fhp, mbytes, MEGABYTE, bytes);
err:
	__os_free(env, bp);
	return ret;
#else
	COMPQUIET(env, NULL);
	COMPQUIET(fhp, NULL);
	return 0;
#endif /* HAVE_FILESYSTEM_NOTZERO */
}
/*
 * __db_zero --
 *	Zero to the end of the file.
 *
 * PUBLIC: int __db_zero_extend __P((ENV *,
 * PUBLIC:     DB_FH *, db_pgno_t, db_pgno_t, uint32));
 */
int __db_zero_extend(ENV * env, DB_FH * fhp, db_pgno_t pgno, db_pgno_t last_pgno, uint32 pgsize)
{
	int ret;
	size_t nwrote;
	uint8 * buf;
	if((ret = __os_calloc(env, 1, pgsize, &buf)) != 0)
		return ret;
	memzero(buf, pgsize);
	for(; pgno <= last_pgno; pgno++)
		if((ret = __os_io(env, DB_IO_WRITE, fhp, pgno, pgsize, 0, pgsize, buf, &nwrote)) != 0) {
			if(ret == 0) {
				ret = EIO;
				goto err;
			}
			goto err;
		}
err:
	__os_free(env, buf);
	return ret;
}
