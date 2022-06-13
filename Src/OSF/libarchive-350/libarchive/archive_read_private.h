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

struct ArchiveRead;
struct ArchiveReadFilterBidder;
struct ArchiveReadFilter;
/*
 * How bidding works for filters:
 *   * The bid manager initializes the client-provided reader as the first filter.
 *   * It invokes the bidder for each registered filter with the current head filter.
 *   * The bidders can use archive_read_filter_ahead() to peek ahead at the incoming data to compose their bids.
 *   * The bid manager creates a new filter structure for the winning bidder and gives the winning bidder a chance to initialize it.
 *   * The new filter becomes the new top filter and we repeat the process.
 * This ends only when no bidder provides a non-zero bid.  Then
 * we perform a similar dance with the registered format handlers.
 */
struct ArchiveReadFilterBidder {
	void * data; // Configuration data for the bidder
	const char * name; // Name of the filter
	int (* FnBid)(ArchiveReadFilterBidder *, ArchiveReadFilter *); // Taste the upstream filter to see if we handle this
	int (* FnInit)(ArchiveReadFilter *); // Initialize a newly-created filter
	int (* FnOptions)(ArchiveReadFilterBidder *, const char *key, const char *value); // Set an option for the filter bidder
	int (* FnFree)(ArchiveReadFilterBidder *); // Release the bidder's configuration data
};
/*
 * This structure is allocated within the ArchiveRead core
 * and initialized by ArchiveRead and the init() method of the
 * corresponding bidder above.
 */
struct ArchiveReadFilter {
	int64 position;
	// Essentially all filters will need these values, so just declare them here.
	ArchiveReadFilterBidder * bidder; /* My bidder. */
	ArchiveReadFilter * upstream; /* Who I read from. */
	ArchiveRead * archive; /* Associated archive. */
	int    (* FnOpen)(ArchiveReadFilter *self); /* Open a block for reading */
	ssize_t (* FnRead)(ArchiveReadFilter *, const void **); /* Return next block. */
	int64  (* skip)(ArchiveReadFilter *self, int64 request); /* Skip forward this many bytes. */
	int64  (* seek)(ArchiveReadFilter *self, int64 offset, int whence); /* Seek to an absolute location. */
	int    (* FnClose)(ArchiveReadFilter *self); /* Close (just this filter) and SAlloc::F(self). */
	int    (* sswitch)(ArchiveReadFilter *self, uint iindex); /* Function that handles switching from reading one block to the next/prev */
	int    (* read_header)(ArchiveReadFilter *self, ArchiveEntry *entry); /* Read any header metadata if available. */
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
	uint8  Reserve; // @alignment
};
/*
 * The client looks a lot like a filter, so we just wrap it here.
 *
 * TODO: Make ArchiveReadFilter and archive_read_client identical so
 * that users of the library can easily register their own
 * transformation filters.  This will probably break the API/ABI and
 * so should be deferred at least until libarchive 3.0.
 */
struct archive_read_data_node {
	int64 begin_position;
	int64 total_size;
	void * data;
};

struct archive_read_client {
	archive_open_callback	*opener;
	archive_read_callback	*reader;
	archive_skip_callback	*skipper;
	archive_seek_callback	*seeker;
	archive_close_callback	*closer;
	archive_switch_callback *switcher;
	uint   nodes;
	uint   cursor;
	int64  position;
	struct archive_read_data_node *dataset;
};

struct archive_read_passphrase {
	char * passphrase;
	archive_read_passphrase * next;
};

struct archive_read_extract {
	Archive *ad; /* archive_write_disk object */
	/* Progress function invoked during extract. */
	void (*extract_progress)(void *);
	void * extract_progress_user_data;
};

struct ArchiveRead {
	Archive	archive;
	ArchiveEntry * entry;
	/* Dev/ino of the archive being read/written. */
	int    skip_file_set;
	int64  skip_file_dev;
	int64  skip_file_ino;
	/* Callbacks to open/read/write/close client archive streams. */
	struct archive_read_client client;
	/* Registered filter bidders. */
	ArchiveReadFilterBidder bidders[16];
	ArchiveReadFilter * filter; /* Last filter in chain */
	int    bypass_filter_bidding; /* Whether to bypass filter bidding process */
	int64  header_position; /* File offset of beginning of most recently-read header. */
	// Nodes and offsets of compressed data block 
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
		int	(*bid)(ArchiveRead *, int best_bid);
		int	(*options)(ArchiveRead *, const char *key, const char *value);
		int	(*read_header)(ArchiveRead *, ArchiveEntry *);
		int	(*read_data)(ArchiveRead *, const void **, size_t *, int64 *);
		int	(*read_data_skip)(ArchiveRead *);
		int64	(*seek_data)(ArchiveRead *, int64, int);
		int	(*cleanup)(ArchiveRead *);
		int	(*format_capabilties)(ArchiveRead *);
		int	(*has_encrypted_entries)(ArchiveRead *);
	} formats[16];
	struct archive_format_descriptor * format; /* Active format. */
	/*
	 * Various information needed by archive_extract.
	 */
	struct archive_read_extract		*extract;
	int (*cleanup_archive_extract)(ArchiveRead *);
	/*
	 * Decryption passphrase.
	 */
	struct {
		struct archive_read_passphrase *first;
		struct archive_read_passphrase **last;
		int candidate;
		archive_passphrase_callback *callback;
		void *client_data;
	} passphrases;
};

int	__archive_read_register_format(ArchiveRead *a, void *format_data, const char *name,
		int (*bid)(ArchiveRead *, int), int (*options)(ArchiveRead *, const char *, const char *),
		int (*read_header)(ArchiveRead *, ArchiveEntry *), int (*read_data)(ArchiveRead *, const void **, size_t *, int64 *),
		int (*read_data_skip)(ArchiveRead *), int64 (*seek_data)(ArchiveRead *, int64, int),
		int (*cleanup)(ArchiveRead *), int (*format_capabilities)(ArchiveRead *), int (*has_encrypted_entries)(ArchiveRead *));
int __archive_read_get_bidder(ArchiveRead *a, ArchiveReadFilterBidder **bidder);
const void *__archive_read_ahead(ArchiveRead *, size_t, ssize_t *);
const void *__archive_read_filter_ahead(ArchiveReadFilter *, size_t, ssize_t *);
int64	__archive_read_seek(ArchiveRead*, int64, int);
int64	__archive_read_filter_seek(ArchiveReadFilter *, int64, int);
int64	FASTCALL __archive_read_consume(ArchiveRead *, int64);
int64	FASTCALL __archive_read_filter_consume(ArchiveReadFilter *, int64);
int __archive_read_header(ArchiveRead *, ArchiveEntry *);
int __archive_read_program(ArchiveReadFilter *, const char *);
void __archive_read_free_filters(ArchiveRead *);
struct archive_read_extract *__archive_read_get_extract(ArchiveRead *);


/*
 * Get a decryption passphrase.
 */
void __archive_read_reset_passphrase(ArchiveRead *a);
const char * __archive_read_next_passphrase(ArchiveRead *a);
#endif
