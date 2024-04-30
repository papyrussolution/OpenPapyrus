// archive_write_private.h
// Copyright (c) 2003-2007 Tim Kientzle All rights reserved.
// 
#ifndef ARCHIVE_WRITE_PRIVATE_H_INCLUDED
#define ARCHIVE_WRITE_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#ifndef __LIBARCHIVE_TEST
		#error This header is only to be used internally to libarchive.
	#endif
#endif
#define	ARCHIVE_WRITE_FILTER_STATE_NEW		1U
#define	ARCHIVE_WRITE_FILTER_STATE_OPEN		2U
#define	ARCHIVE_WRITE_FILTER_STATE_CLOSED	4U
#define	ARCHIVE_WRITE_FILTER_STATE_FATAL	0x8000U

struct archive_write;

struct archive_write_filter {
	int64 bytes_written;
	Archive * archive; /* Associated archive. */
	struct archive_write_filter * next_filter; /* Who I write to. */
	int	(* FnOptions)(struct archive_write_filter *, const char *key, const char *value);
	int	(* FnOpen)(struct archive_write_filter *);
	int	(* FnWrite)(struct archive_write_filter *, const void *, size_t);
	int	(* FnClose)(struct archive_write_filter *);
	int	(* FnFree)(struct archive_write_filter *);
	void * data;
	const char *name;
	int	  code;
	int	  bytes_per_block;
	int	  bytes_in_last_block;
	int	  state;
};

#if ARCHIVE_VERSION < 4000000
void __archive_write_filters_free(Archive *);
#endif

struct archive_write_filter *__archive_write_allocate_filter(Archive *);
int __archive_write_output(struct archive_write *, const void *, size_t);
int __archive_write_nulls(struct archive_write *, size_t);
int __archive_write_filter(struct archive_write_filter *, const void *, size_t);

struct archive_write {
	Archive	archive;
	/* Dev/ino of the archive being written. */
	int    skip_file_set;
	int64  skip_file_dev;
	int64  skip_file_ino;
	/* Utility:  Pointer to a block of nulls. */
	const uchar	*nulls;
	size_t null_length;
	/* Callbacks to open/read/write/close archive stream. */
	archive_open_callback	*client_opener;
	archive_write_callback	*client_writer;
	archive_close_callback	*client_closer;
	archive_free_callback	*client_freer;
	void * client_data;
	/*
	 * Blocking information.  Note that bytes_in_last_block is
	 * misleadingly named; I should find a better name.  These
	 * control the final output from all compressors, including
	 * compression_none.
	 */
	int bytes_per_block;
	int bytes_in_last_block;
	/*
	 * First and last write filters in the pipeline.
	 */
	struct archive_write_filter *filter_first;
	struct archive_write_filter *filter_last;
	/*
	 * Pointers to format-specific functions for writing.  They're
	 * initialized by archive_write_set_format_XXX() calls.
	 */
	void	 *format_data;
	const char *format_name;
	int	(*format_init)(struct archive_write *);
	int	(*format_options)(struct archive_write *, const char *key, const char *value);
	int	(*format_finish_entry)(struct archive_write *);
	int (*format_write_header)(struct archive_write *, ArchiveEntry *);
	ssize_t	(*format_write_data)(struct archive_write *, const void *buff, size_t);
	int	(*format_close)(struct archive_write *);
	int	(*format_free)(struct archive_write *);
	/*
	 * Encryption passphrase.
	 */
	char * passphrase;
	archive_passphrase_callback *passphrase_callback;
	void * passphrase_client_data;
};
/*
 * Utility function to format a USTAR header into a buffer.  If
 * "strict" is set, this tries to create the absolutely most portable
 * version of a ustar header.  If "strict" is set to 0, then it will
 * relax certain requirements.
 *
 * Generally, format-specific declarations don't belong in this
 * header; this is a rare example of a function that is shared by
 * two very similar formats (ustar and pax).
 */
int __archive_write_format_header_ustar(struct archive_write *, char buff[512], ArchiveEntry *, int tartype, int strict, archive_string_conv *);
struct archive_write_program_data;
struct archive_write_program_data * __archive_write_program_allocate(const char *program_name);
int	__archive_write_program_free(struct archive_write_program_data *);
int	__archive_write_program_open(struct archive_write_filter *, struct archive_write_program_data *, const char *);
int	__archive_write_program_close(struct archive_write_filter *, struct archive_write_program_data *);
int	__archive_write_program_write(struct archive_write_filter *, struct archive_write_program_data *, const void *, size_t);
/*
 * Get a encryption passphrase.
 */
const char * __archive_write_get_passphrase(struct archive_write *a);
#endif
