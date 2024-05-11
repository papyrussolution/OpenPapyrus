/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_UPGRADE_H_
#define	_DB_UPGRADE_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file defines the metadata pages from the previous release.
 * These structures are only used to upgrade old versions of databases.
 */

/* Structures from the 3.1 release */
typedef struct _dbmeta31 {
	DB_LSN	  lsn;		/* 00-07: LSN. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	uint32 magic;	/* 12-15: Magic number. */
	uint32 version;	/* 16-19: Version. */
	uint32 pagesize;	/* 20-23: Pagesize. */
	uint8  unused1[1];	/* 24: Unused. */
	uint8  type;		/* 25: Page type. */
	uint8  unused2[2];	/* 26-27: Unused. */
	uint32 free;		/* 28-31: Free list page number. */
	DB_LSN    unused3;	/* 36-39: Unused. */
	uint32 key_count;	/* 40-43: Cached key count. */
	uint32 record_count;	/* 44-47: Cached record count. */
	uint32 flags;	/* 48-51: Flags: unique to each AM. */
				/* 52-71: Unique file ID. */
	uint8  uid[DB_FILE_ID_LEN];
} DBMETA31;

typedef struct _btmeta31 {
	DBMETA31  dbmeta;		/* 00-71: Generic meta-data header. */

	uint32 maxkey;	/* 72-75: Btree: Maxkey. */
	uint32 minkey;	/* 76-79: Btree: Minkey. */
	uint32 re_len;	/* 80-83: Recno: fixed-length record length. */
	uint32 re_pad;	/* 84-87: Recno: fixed-length record pad. */
	uint32 root;		/* 88-92: Root page. */

	/*
	 * Minimum page size is 128.
	 */
} BTMETA31;

// 
// HASH METADATA PAGE LAYOUT
// 
typedef struct _hashmeta31 {
	DBMETA31 dbmeta;	/* 00-71: Generic meta-data page header. */

	uint32 max_bucket;	/* 72-75: ID of Maximum bucket in use */
	uint32 high_mask;	/* 76-79: Modulo mask into table */
	uint32 low_mask;	/* 80-83: Modulo mask into table lower half */
	uint32 ffactor;	/* 84-87: Fill factor */
	uint32 nelem;	/* 88-91: Number of keys in hash table */
	uint32 h_charkey;	/* 92-95: Value of hash(CHARKEY) */
#define	NCACHED	32		/* number of spare points */
				/* 96-223: Spare pages for overflow */
	uint32 spares[NCACHED];

	/*
	 * Minimum page size is 256.
	 */
} HMETA31;

/*
 * QAM Meta data page structure
 *
 */
typedef struct _qmeta31 {
	DBMETA31 dbmeta;	/* 00-71: Generic meta-data header. */

	uint32 start;	/* 72-75: Start offset. */
	uint32 first_recno;	/* 76-79: First not deleted record. */
	uint32 cur_recno;	/* 80-83: Last recno allocated. */
	uint32 re_len;	/* 84-87: Fixed-length record length. */
	uint32 re_pad;	/* 88-91: Fixed-length record pad. */
	uint32 rec_page;	/* 92-95: Records Per Page. */

	/*
	 * Minimum page size is 128.
	 */
} QMETA31;
/* Structures from the 3.2 release */
typedef struct _qmeta32 {
	DBMETA31 dbmeta;	/* 00-71: Generic meta-data header. */

	uint32 first_recno;	/* 72-75: First not deleted record. */
	uint32 cur_recno;	/* 76-79: Last recno allocated. */
	uint32 re_len;	/* 80-83: Fixed-length record length. */
	uint32 re_pad;	/* 84-87: Fixed-length record pad. */
	uint32 rec_page;	/* 88-91: Records Per Page. */
	uint32 page_ext;	/* 92-95: Pages per extent */

	/*
	 * Minimum page size is 128.
	 */
} QMETA32;

/* Structures from the 3.0 release */

typedef struct _dbmeta30 {
	DB_LSN	  lsn;		/* 00-07: LSN. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	uint32 magic;	/* 12-15: Magic number. */
	uint32 version;	/* 16-19: Version. */
	uint32 pagesize;	/* 20-23: Pagesize. */
	uint8  unused1[1];	/* 24: Unused. */
	uint8  type;		/* 25: Page type. */
	uint8  unused2[2];	/* 26-27: Unused. */
	uint32 free;		/* 28-31: Free list page number. */
	uint32 flags;	/* 32-35: Flags: unique to each AM. */
				/* 36-55: Unique file ID. */
	uint8  uid[DB_FILE_ID_LEN];
} DBMETA30;
// 
// BTREE METADATA PAGE LAYOUT
// 
typedef struct _btmeta30 {
	DBMETA30	dbmeta;	/* 00-55: Generic meta-data header. */

	uint32 maxkey;	/* 56-59: Btree: Maxkey. */
	uint32 minkey;	/* 60-63: Btree: Minkey. */
	uint32 re_len;	/* 64-67: Recno: fixed-length record length. */
	uint32 re_pad;	/* 68-71: Recno: fixed-length record pad. */
	uint32 root;		/* 72-75: Root page. */

	/*
	 * Minimum page size is 128.
	 */
} BTMETA30;
// 
// HASH METADATA PAGE LAYOUT
// 
typedef struct _hashmeta30 {
	DBMETA30 dbmeta;	/* 00-55: Generic meta-data page header. */

	uint32 max_bucket;	/* 56-59: ID of Maximum bucket in use */
	uint32 high_mask;	/* 60-63: Modulo mask into table */
	uint32 low_mask;	/* 64-67: Modulo mask into table lower half */
	uint32 ffactor;	/* 68-71: Fill factor */
	uint32 nelem;	/* 72-75: Number of keys in hash table */
	uint32 h_charkey;	/* 76-79: Value of hash(CHARKEY) */
#define	NCACHED30	32		/* number of spare points */
				/* 80-207: Spare pages for overflow */
	uint32 spares[NCACHED30];

	/*
	 * Minimum page size is 256.
	 */
} HMETA30;
// 
// QUEUE METADATA PAGE LAYOUT
// 
/*
 * QAM Meta data page structure
 *
 */
typedef struct _qmeta30 {
	DBMETA30    dbmeta;	/* 00-55: Generic meta-data header. */

	uint32 start;	/* 56-59: Start offset. */
	uint32 first_recno;	/* 60-63: First not deleted record. */
	uint32 cur_recno;	/* 64-67: Last recno allocated. */
	uint32 re_len;	/* 68-71: Fixed-length record length. */
	uint32 re_pad;	/* 72-75: Fixed-length record pad. */
	uint32 rec_page;	/* 76-79: Records Per Page. */

	/*
	 * Minimum page size is 128.
	 */
} QMETA30;
/* Structures from Release 2.x */
// 
// BTREE METADATA PAGE LAYOUT
// 
/*
 * Btree metadata page layout:
 */
typedef struct _btmeta2X {
	DB_LSN	  lsn;		/* 00-07: LSN. */
	db_pgno_t pgno;		/* 08-11: Current page number. */
	uint32 magic;	/* 12-15: Magic number. */
	uint32 version;	/* 16-19: Version. */
	uint32 pagesize;	/* 20-23: Pagesize. */
	uint32 maxkey;	/* 24-27: Btree: Maxkey. */
	uint32 minkey;	/* 28-31: Btree: Minkey. */
	uint32 free;		/* 32-35: Free list page number. */
	uint32 flags;	/* 36-39: Flags. */
	uint32 re_len;	/* 40-43: Recno: fixed-length record length. */
	uint32 re_pad;	/* 44-47: Recno: fixed-length record pad. */
				/* 48-67: Unique file ID. */
	uint8  uid[DB_FILE_ID_LEN];
} BTMETA2X;
// 
// HASH METADATA PAGE LAYOUT
// 
/*
 * Hash metadata page layout:
 */
/* Hash Table Information */
typedef struct hashhdr {	/* Disk resident portion */
	DB_LSN	lsn;		/* 00-07: LSN of the header page */
	db_pgno_t pgno;		/* 08-11: Page number (btree compatibility). */
	uint32 magic;	/* 12-15: Magic NO for hash tables */
	uint32 version;	/* 16-19: Version ID */
	uint32 pagesize;	/* 20-23: Bucket/Page Size */
	uint32 ovfl_point;	/* 24-27: Overflow page allocation location */
	uint32 last_freed;	/* 28-31: Last freed overflow page pgno */
	uint32 max_bucket;	/* 32-35: ID of Maximum bucket in use */
	uint32 high_mask;	/* 36-39: Modulo mask into table */
	uint32 low_mask;	/* 40-43: Modulo mask into table lower half */
	uint32 ffactor;	/* 44-47: Fill factor */
	uint32 nelem;	/* 48-51: Number of keys in hash table */
	uint32 h_charkey;	/* 52-55: Value of hash(CHARKEY) */
	uint32 flags;	/* 56-59: Allow duplicates. */
#define	NCACHED2X	32		/* number of spare points */
				/* 60-187: Spare pages for overflow */
	uint32 spares[NCACHED2X];
				/* 188-207: Unique file ID. */
	uint8  uid[DB_FILE_ID_LEN];

	/*
	 * Minimum page size is 256.
	 */
} HASHHDR;

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_UPGRADE_H_ */
