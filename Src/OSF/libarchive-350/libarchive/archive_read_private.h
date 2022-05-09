/*-
 * Copyright (c) 2003-2007 Tim Kientzle
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
 * $FreeBSD: head/lib/libarchive/archive_read_private.h 201088 2009-12-28 02:18:55Z kientzle $
 */

#ifndef ARCHIVE_READ_PRIVATE_H_INCLUDED
#define ARCHIVE_READ_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
#ifndef __LIBARCHIVE_TEST
#error This header is only to be used internally to libarchive.
#endif
#endif

#include "archive.h"
#include "archive_string.h"
#include "archive_private.h"

struct archive_read;
struct archive_read_filter_bidder;
struct archive_read_filter;

/*
 * How bidding works for filters:
 *   * The bid manager initializes the client-provided reader as the
 *     first filter.
 *   * It invokes the bidder for each registered filter with the
 *     current head filter.
 *   * The bidders can use archive_read_filter_ahead() to peek ahead
 *     at the incoming data to compose their bids.
 *   * The bid manager creates a new filter structure for the winning
 *     bidder and gives the winning bidder a chance to initialize it.
 *   * The new filter becomes the new top filter and we repeat the
 *     process.
 * This ends only when no bidder provides a non-zero bid.  Then
 * we perform a similar dance with the registered format handlers.
 */
struct archive_read_filter_bidder {
	/* Configuration data for the bidder. */
	void *data;
	/* Name of the filter */
	const char *name;
	/* Taste the upstream filter to see if we handle this. */
	int (*bid)(struct archive_read_filter_bidder *,
	    struct archive_read_filter *);
	/* Initialize a newly-created filter. */
	int (*init)(struct archive_read_filter *);
	/* Set an option for the filter bidder. */
	int (*options)(struct archive_read_filter_bidder *,
	    const char *key, const char *value);
	/* Release the bidder's configuration data. */
	int (*free)(struct archive_read_filter_bidder *);
};

/*
 * This structure is allocated within the archive_read core
 * and initialized by archive_read and the init() method of the
 * corresponding bidder above.
 */
struct archive_read_filter {
	int64 position;
	// Essentially all filters will need these values, so just declare them here.
	struct archive_read_filter_bidder * bidder; /* My bidder. */
	struct archive_read_filter * upstream; /* Who I read from. */
	struct archive_read * archive; /* Associated archive. */
	int    (* FnOpen)(struct archive_read_filter *self); /* Open a block for reading */
	ssize_t (* FnRead)(struct archive_read_filter *, const void **); /* Return next block. */
	int64  (* skip)(struct archive_read_filter *self, int64 request); /* Skip forward this many bytes. */
	int64  (* seek)(struct archive_read_filter *self, int64 offset, int whence); /* Seek to an absolute location. */
	int    (* FnClose)(struct archive_read_filter *self); /* Close (just this filter) and SAlloc::F(self). */
	int    (* sswitch)(struct archive_read_filter *self, uint iindex); /* Function that handles switching from reading one block to the next/prev */
	int    (* read_header)(struct archive_read_filter *self, struct archive_entry *entry); /* Read any header metadata if available. */
	void * data; /* My private data. */
	const char * name;
	int    code;
	// Used by reblocking logic
	char * buffer;
	size_t buffer_size;
	char * next;  // Current read location.
	size_t avail; // Bytes in my buffer.
	const void * client_buff; // Client buffer information.
	size_t client_total;
	const char * client_next;
	size_t client_avail;
	char   end_of_file;
	char   closed;
	char   fatal;
};

/*
 * The client looks a lot like a filter, so we just wrap it here.
 *
 * TODO: Make archive_read_filter and archive_read_client identical so
 * that users of the library can easily register their own
 * transformation filters.  This will probably break the API/ABI and
 * so should be deferred at least until libarchive 3.0.
 */
struct archive_read_data_node {
	int64 begin_position;
	int64 total_size;
	void *data;
};
struct archive_read_client {
	archive_open_callback	*opener;
	archive_read_callback	*reader;
	archive_skip_callback	*skipper;
	archive_seek_callback	*seeker;
	archive_close_callback	*closer;
	archive_switch_callback *switcher;
	uint nodes;
	uint cursor;
	int64 position;
	struct archive_read_data_node *dataset;
};
struct archive_read_passphrase {
	char	*passphrase;
	struct archive_read_passphrase *next;
};

struct archive_read_extract {
	struct archive *ad; /* archive_write_disk object */

	/* Progress function invoked during extract. */
	void			(*extract_progress)(void *);
	void			 *extract_progress_user_data;
};

struct archive_read {
	struct archive	archive;
	struct archive_entry	*entry;
	/* Dev/ino of the archive being read/written. */
	int		  skip_file_set;
	int64		  skip_file_dev;
	int64		  skip_file_ino;
	/* Callbacks to open/read/write/close client archive streams. */
	struct archive_read_client client;
	/* Registered filter bidders. */
	struct archive_read_filter_bidder bidders[16];
	/* Last filter in chain */
	struct archive_read_filter *filter;
	/* Whether to bypass filter bidding process */
	int bypass_filter_bidding;

	/* File offset of beginning of most recently-read header. */
	int64		  header_position;

	/* Nodes and offsets of compressed data block */
	uint data_start_node;
	uint data_end_node;

	/*
	 * Format detection is mostly the same as compression
	 * detection, with one significant difference: The bidders
	 * use the read_ahead calls above to examine the stream rather
	 * than having the supervisor hand them a block of data to
	 * examine.
	 */

	struct archive_format_descriptor {
		void	 *data;
		const char *name;
		int	(*bid)(struct archive_read *, int best_bid);
		int	(*options)(struct archive_read *, const char *key,
		    const char *value);
		int	(*read_header)(struct archive_read *, struct archive_entry *);
		int	(*read_data)(struct archive_read *, const void **, size_t *, int64 *);
		int	(*read_data_skip)(struct archive_read *);
		int64	(*seek_data)(struct archive_read *, int64, int);
		int	(*cleanup)(struct archive_read *);
		int	(*format_capabilties)(struct archive_read *);
		int	(*has_encrypted_entries)(struct archive_read *);
	}	formats[16];
	struct archive_format_descriptor * format; /* Active format. */
	/*
	 * Various information needed by archive_extract.
	 */
	struct archive_read_extract		*extract;
	int (*cleanup_archive_extract)(struct archive_read *);
	/*
	 * Decryption passphrase.
	 */
	struct {
		struct archive_read_passphrase *first;
		struct archive_read_passphrase **last;
		int candidate;
		archive_passphrase_callback *callback;
		void *client_data;
	}		passphrases;
};

int	__archive_read_register_format(struct archive_read *a,
		void *format_data,
		const char *name,
		int (*bid)(struct archive_read *, int),
		int (*options)(struct archive_read *, const char *, const char *),
		int (*read_header)(struct archive_read *, struct archive_entry *),
		int (*read_data)(struct archive_read *, const void **, size_t *, int64 *),
		int (*read_data_skip)(struct archive_read *),
		int64 (*seek_data)(struct archive_read *, int64, int),
		int (*cleanup)(struct archive_read *),
		int (*format_capabilities)(struct archive_read *),
		int (*has_encrypted_entries)(struct archive_read *));

int __archive_read_get_bidder(struct archive_read *a, struct archive_read_filter_bidder **bidder);
const void *__archive_read_ahead(struct archive_read *, size_t, ssize_t *);
const void *__archive_read_filter_ahead(struct archive_read_filter *, size_t, ssize_t *);
int64	__archive_read_seek(struct archive_read*, int64, int);
int64	__archive_read_filter_seek(struct archive_read_filter *, int64, int);
int64	FASTCALL __archive_read_consume(struct archive_read *, int64);
int64	__archive_read_filter_consume(struct archive_read_filter *, int64);
int __archive_read_header(struct archive_read *, struct archive_entry *);
int __archive_read_program(struct archive_read_filter *, const char *);
void __archive_read_free_filters(struct archive_read *);
struct archive_read_extract *__archive_read_get_extract(struct archive_read *);


/*
 * Get a decryption passphrase.
 */
void __archive_read_reset_passphrase(struct archive_read *a);
const char * __archive_read_next_passphrase(struct archive_read *a);
#endif
