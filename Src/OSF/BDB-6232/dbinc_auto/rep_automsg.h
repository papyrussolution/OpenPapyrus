/* Do not edit: automatically built by gen_msg.awk. */

#ifndef	__rep_AUTOMSG_H
#define	__rep_AUTOMSG_H

/*
 * Message sizes are simply the sum of field sizes (not
 * counting variable size parts, when DBTs are present),
 * and may be different from struct sizes due to padding.
 */
#define	__REP_BULK_SIZE	16
typedef struct ___rep_bulk_args {
	uint32	len;
	DB_LSN		lsn;
	DBT		bulkdata;
} __rep_bulk_args;

#define	__REP_CONTROL_SIZE	36
typedef struct ___rep_control_args {
	uint32	rep_version;
	uint32	log_version;
	DB_LSN		lsn;
	uint32	rectype;
	uint32	gen;
	uint32	msg_sec;
	uint32	msg_nsec;
	uint32	flags;
} __rep_control_args;

#define	__REP_EGEN_SIZE	4
typedef struct ___rep_egen_args {
	uint32	egen;
} __rep_egen_args;

#define	__REP_FILEINFO_SIZE	48
typedef struct ___rep_fileinfo_args {
	uint32	pgsize;
	db_pgno_t	pgno;
	db_pgno_t	max_pgno;
	uint32	filenum;
	uint32	finfo_flags;
	uint32	type;
	uint32	db_flags;
	DBT		uid;
	DBT		info;
	DBT		dir;
	uint32	blob_fid_lo;
	uint32	blob_fid_hi;
} __rep_fileinfo_args;

#define	__REP_FILEINFO_V7_SIZE	40
typedef struct ___rep_fileinfo_v7_args {
	uint32	pgsize;
	db_pgno_t	pgno;
	db_pgno_t	max_pgno;
	uint32	filenum;
	uint32	finfo_flags;
	uint32	type;
	uint32	db_flags;
	DBT		uid;
	DBT		info;
	DBT		dir;
} __rep_fileinfo_v7_args;

#define	__REP_FILEINFO_V6_SIZE	36
typedef struct ___rep_fileinfo_v6_args {
	uint32	pgsize;
	db_pgno_t	pgno;
	db_pgno_t	max_pgno;
	uint32	filenum;
	uint32	finfo_flags;
	uint32	type;
	uint32	db_flags;
	DBT		uid;
	DBT		info;
} __rep_fileinfo_v6_args;

#define	__REP_GRANT_INFO_SIZE	8
typedef struct ___rep_grant_info_args {
	uint32	msg_sec;
	uint32	msg_nsec;
} __rep_grant_info_args;

#define	__REP_LOGREQ_SIZE	8
typedef struct ___rep_logreq_args {
	DB_LSN		endlsn;
} __rep_logreq_args;

#define	__REP_NEWFILE_SIZE	4
typedef struct ___rep_newfile_args {
	uint32	version;
} __rep_newfile_args;

#define	__REP_UPDATE_SIZE	16
typedef struct ___rep_update_args {
	DB_LSN		first_lsn;
	uint32	first_vers;
	uint32	num_files;
} __rep_update_args;

#define	__REP_VOTE_INFO_SIZE	28
typedef struct ___rep_vote_info_args {
	uint32	egen;
	uint32	nsites;
	uint32	nvotes;
	uint32	priority;
	uint32	spare_pri;
	uint32	tiebreaker;
	uint32	data_gen;
} __rep_vote_info_args;

#define	__REP_VOTE_INFO_V5_SIZE	20
typedef struct ___rep_vote_info_v5_args {
	uint32	egen;
	uint32	nsites;
	uint32	nvotes;
	uint32	priority;
	uint32	tiebreaker;
} __rep_vote_info_v5_args;

#define	__REP_LSN_HIST_KEY_SIZE	8
typedef struct ___rep_lsn_hist_key_args {
	uint32	version;
	uint32	gen;
} __rep_lsn_hist_key_args;

#define	__REP_LSN_HIST_DATA_SIZE	20
typedef struct ___rep_lsn_hist_data_args {
	uint32	envid;
	DB_LSN		lsn;
	uint32	hist_sec;
	uint32	hist_nsec;
} __rep_lsn_hist_data_args;

#define	__REP_BLOB_UPDATE_REQ_SIZE	36
typedef struct ___rep_blob_update_req_args {
	uint64	blob_fid;
	uint64	blob_sid;
	uint64	blob_id;
	uint64	highest_id;
	uint32	flags;
} __rep_blob_update_req_args;

#define	__REP_BLOB_UPDATE_REQ_V8_SIZE	32
typedef struct ___rep_blob_update_req_v8_args {
	uint64	blob_fid;
	uint64	blob_sid;
	uint64	blob_id;
	uint64	highest_id;
} __rep_blob_update_req_v8_args;

#define	__REP_BLOB_UPDATE_SIZE	24
typedef struct ___rep_blob_update_args {
	uint64	blob_fid;
	uint64	highest_id;
	uint32	flags;
	uint32	num_blobs;
} __rep_blob_update_args;

#define	__REP_BLOB_FILE_SIZE	24
typedef struct ___rep_blob_file_args {
	uint64	blob_sid;
	uint64	blob_id;
	uint64	blob_size;
} __rep_blob_file_args;

#define	__REP_BLOB_CHUNK_SIZE	40
typedef struct ___rep_blob_chunk_args {
	uint32	flags;
	uint64	blob_fid;
	uint64	blob_sid;
	uint64	blob_id;
	uint64	offset;
	DBT		data;
} __rep_blob_chunk_args;

#define	__REP_BLOB_CHUNK_REQ_SIZE	32
typedef struct ___rep_blob_chunk_req_args {
	uint64	blob_fid;
	uint64	blob_sid;
	uint64	blob_id;
	uint64	offset;
} __rep_blob_chunk_req_args;

#define	__REP_MAXMSG_SIZE	48
#endif
