/*-
 * Copyright (c) 2007 Joerg Sonnenberger
 * Copyright (c) 2012 Michihiro NAKAJIMA
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
#ifdef HAVE_SYS_WAIT_H
	#include <sys/wait.h>
#endif

#if ARCHIVE_VERSION_NUMBER < 4000000
	/* Deprecated; remove in libarchive 4.0 */
	int archive_read_support_compression_program(Archive * a, const char * cmd)
	{
		return archive_read_support_filter_program(a, cmd);
	}

	int archive_read_support_compression_program_signature(Archive * a, const char * cmd, const void * signature, size_t signature_len)
	{
		return archive_read_support_filter_program_signature(a, cmd, signature, signature_len);
	}
#endif

int archive_read_support_filter_program(Archive * a, const char * cmd)
{
	return (archive_read_support_filter_program_signature(a, cmd, NULL, 0));
}

/*
 * The bidder object stores the command and the signature to watch for.
 * The 'inhibit' entry here is used to ensure that unchecked filters never
 * bid twice in the same pipeline.
 */
struct program_bidder {
	char * description;
	char * cmd;
	void * signature;
	size_t signature_len;
	int inhibit;
};

static int program_bidder_bid(ArchiveReadFilterBidder *,
    ArchiveReadFilter * upstream);
static int program_bidder_init(ArchiveReadFilter *);
static int program_bidder_free(ArchiveReadFilterBidder *);

/*
 * The actual filter needs to track input and output data.
 */
struct program_filter {
	archive_string description;
#if defined(_WIN32) && !defined(__CYGWIN__)
	HANDLE child;
#else
	pid_t child;
#endif
	int exit_status;
	int waitpid_return;
	int child_stdin, child_stdout;

	char * out_buf;
	size_t out_buf_len;
};

static ssize_t  program_filter_read(ArchiveReadFilter *,
    const void **);
static int program_filter_close(ArchiveReadFilter *);
static void     free_state(struct program_bidder *);

static int set_bidder_signature(ArchiveReadFilterBidder * bidder, struct program_bidder * state, const void * signature, size_t signature_len)
{
	if(signature && signature_len > 0) {
		state->signature_len = signature_len;
		state->signature = SAlloc::M(signature_len);
		memcpy(state->signature, signature, signature_len);
	}
	/*
	 * Fill in the bidder object.
	 */
	bidder->data = state;
	bidder->FnBid = program_bidder_bid;
	bidder->FnInit = program_bidder_init;
	bidder->FnOptions = NULL;
	bidder->FnFree = program_bidder_free;
	return ARCHIVE_OK;
}

int archive_read_support_filter_program_signature(Archive * _a, const char * cmd, const void * signature, size_t signature_len)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	ArchiveReadFilterBidder * bidder;
	struct program_bidder * state;
	/*
	 * Get a bidder object from the read core.
	 */
	if(__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	/*
	 * Allocate our private state.
	 */
	state = (struct program_bidder *)SAlloc::C(1, sizeof(*state));
	if(state == NULL)
		goto memerr;
	state->cmd = sstrdup(cmd);
	if(state->cmd == NULL)
		goto memerr;
	return set_bidder_signature(bidder, state, signature, signature_len);
memerr:
	free_state(state);
	archive_set_error(_a, ENOMEM, SlTxtOutOfMem);
	return ARCHIVE_FATAL;
}

static int program_bidder_free(ArchiveReadFilterBidder * self)
{
	struct program_bidder * state = (struct program_bidder *)self->data;
	free_state(state);
	return ARCHIVE_OK;
}

static void free_state(struct program_bidder * state)
{
	if(state) {
		SAlloc::F(state->cmd);
		SAlloc::F(state->signature);
		SAlloc::F(state);
	}
}

/*
 * If we do have a signature, bid only if that matches.
 *
 * If there's no signature, we bid INT_MAX the first time
 * we're called, then never bid again.
 */
static int program_bidder_bid(ArchiveReadFilterBidder * self, ArchiveReadFilter * upstream)
{
	struct program_bidder * state = static_cast<struct program_bidder *>(self->data);
	const char * p;
	/* If we have a signature, use that to match. */
	if(state->signature_len > 0) {
		p = static_cast<const char *>(__archive_read_filter_ahead(upstream, state->signature_len, NULL));
		if(!p)
			return 0;
		/* No match, so don't bid. */
		if(memcmp(p, state->signature, state->signature_len) != 0)
			return 0;
		return ((int)state->signature_len * 8);
	}

	/* Otherwise, bid once and then never bid again. */
	if(state->inhibit)
		return 0;
	state->inhibit = 1;
	return (INT_MAX);
}

/*
 * Shut down the child, return ARCHIVE_OK if it exited normally.
 *
 * Note that the return value is sticky; if we're called again,
 * we won't reap the child again, but we will return the same status
 * (including error message if the child came to a bad end).
 */
static int child_stop(ArchiveReadFilter * self, struct program_filter * state)
{
	/* Close our side of the I/O with the child. */
	if(state->child_stdin != -1) {
		close(state->child_stdin);
		state->child_stdin = -1;
	}
	if(state->child_stdout != -1) {
		close(state->child_stdout);
		state->child_stdout = -1;
	}
	if(state->child != 0) {
		/* Reap the child. */
		do {
			state->waitpid_return = waitpid(state->child, &state->exit_status, 0);
		} while(state->waitpid_return == -1 && errno == EINTR);
#if defined(_WIN32) && !defined(__CYGWIN__)
		CloseHandle(state->child);
#endif
		state->child = 0;
	}
	if(state->waitpid_return < 0) {
		/* waitpid() failed?  This is ugly. */
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Child process exited badly");
		return ARCHIVE_WARN;
	}
#if !defined(_WIN32) || defined(__CYGWIN__)
	if(WIFSIGNALED(state->exit_status)) {
#ifdef SIGPIPE
		/* If the child died because we stopped reading before
		* it was done, that's okay.  Some archive formats
		* have padding at the end that we routinely ignore. */
		/* The alternative to this would be to add a step
		 * before close(child_stdout) above to read from the
		 * child until the child has no more to write. */
		if(WTERMSIG(state->exit_status) == SIGPIPE)
			return ARCHIVE_OK;
#endif
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Child process exited with signal %d", WTERMSIG(state->exit_status));
		return ARCHIVE_WARN;
	}
#endif /* !_WIN32 || __CYGWIN__ */
	if(WIFEXITED(state->exit_status)) {
		if(WEXITSTATUS(state->exit_status) == 0)
			return ARCHIVE_OK;
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Child process exited with status %d", WEXITSTATUS(state->exit_status));
		return ARCHIVE_WARN;
	}
	return ARCHIVE_WARN;
}
/*
 * Use select() to decide whether the child is ready for read or write.
 */
static ssize_t child_read(ArchiveReadFilter * self, char * buf, size_t buf_len)
{
	struct program_filter * state = static_cast<struct program_filter *>(self->data);
	ssize_t ret, requested, avail;
	const char * p;
#if defined(_WIN32) && !defined(__CYGWIN__)
	HANDLE handle = (HANDLE)_get_osfhandle(state->child_stdout);
#endif
	requested = buf_len > SSIZE_MAX ? SSIZE_MAX : buf_len;
	for(;;) {
		do {
#if defined(_WIN32) && !defined(__CYGWIN__)
			/* Avoid infinity wait.
			 * Note: If there is no data in the pipe, ReadFile()
			 * called in read() never returns and so we won't
			 * write remaining encoded data to the pipe.
			 * Note: This way may cause performance problem.
			 * we are looking forward to great code to resolve
			 * this.  */
			DWORD pipe_avail = -1;
			int cnt = 2;
			while(PeekNamedPipe(handle, NULL, 0, NULL, &pipe_avail, NULL) != 0 && pipe_avail == 0 && cnt--)
				Sleep(5);
			if(pipe_avail == 0) {
				ret = -1;
				errno = EAGAIN;
				break;
			}
#endif
			ret = read(state->child_stdout, buf, requested);
		} while(ret == -1 && errno == EINTR);
		if(ret > 0)
			return ret;
		if(ret == 0 || (ret == -1 && errno == EPIPE))
			return (child_stop(self, state)); // Child has closed its output; reap the child and return the status.
		if(ret == -1 && errno != EAGAIN)
			return -1;
		if(state->child_stdin == -1) {
			// Block until child has some I/O ready.
			__archive_check_child(state->child_stdin, state->child_stdout);
			continue;
		}
		/* Get some more data from upstream. */
		p = static_cast<const char *>(__archive_read_filter_ahead(self->upstream, 1, &avail));
		if(!p) {
			close(state->child_stdin);
			state->child_stdin = -1;
			fcntl(state->child_stdout, F_SETFL, 0);
			if(avail < 0)
				return (avail);
			continue;
		}
		do {
			ret = write(state->child_stdin, p, avail);
		} while(ret == -1 && errno == EINTR);
		if(ret > 0) {
			__archive_read_filter_consume(self->upstream, ret); // Consume whatever we managed to write
		}
		else if(ret == -1 && errno == EAGAIN) {
			// Block until child has some I/O ready.
			__archive_check_child(state->child_stdin, state->child_stdout);
		}
		else {
			/* Write failed. */
			close(state->child_stdin);
			state->child_stdin = -1;
			fcntl(state->child_stdout, F_SETFL, 0);
			// If it was a bad error, we're done; otherwise
			// it was EPIPE or EOF, and we can still read from the child. 
			if(ret == -1 && errno != EPIPE)
				return -1;
		}
	}
}

int __archive_read_program(ArchiveReadFilter * self, const char * cmd)
{
	static const size_t out_buf_len = 65536;
	const char * prefix = "Program: ";
	int ret;
	size_t l = strlen(prefix) + strlen(cmd) + 1;
	struct program_filter * state = (struct program_filter *)SAlloc::C(1, sizeof(*state));
	char * out_buf = (char *)SAlloc::M(out_buf_len);
	if(state == NULL || out_buf == NULL || archive_string_ensure(&state->description, l) == NULL) {
		archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate input data");
		if(state) {
			archive_string_free(&state->description);
			SAlloc::F(state);
		}
		SAlloc::F(out_buf);
		return ARCHIVE_FATAL;
	}
	archive_strcpy(&state->description, prefix);
	archive_strcat(&state->description, cmd);
	self->code = ARCHIVE_FILTER_PROGRAM;
	self->name = state->description.s;
	state->out_buf = out_buf;
	state->out_buf_len = out_buf_len;
	ret = __archive_create_child(cmd, &state->child_stdin, &state->child_stdout, &state->child);
	if(ret != ARCHIVE_OK) {
		SAlloc::F(state->out_buf);
		archive_string_free(&state->description);
		SAlloc::F(state);
		archive_set_error(&self->archive->archive, EINVAL, "Can't initialize filter; unable to run program \"%s\"", cmd);
		return ARCHIVE_FATAL;
	}
	self->data = state;
	self->FnRead = program_filter_read;
	self->skip = NULL;
	self->FnClose = program_filter_close;
	/* XXX Check that we can read at least one byte? */
	return ARCHIVE_OK;
}

static int program_bidder_init(ArchiveReadFilter * self)
{
	struct program_bidder * bidder_state = (struct program_bidder *)self->bidder->data;
	return (__archive_read_program(self, bidder_state->cmd));
}

static ssize_t program_filter_read(ArchiveReadFilter * self, const void ** buff)
{
	ssize_t bytes;
	struct program_filter * state = (struct program_filter *)self->data;
	size_t total = 0;
	char * p = state->out_buf;
	while(state->child_stdout != -1 && total < state->out_buf_len) {
		bytes = child_read(self, p, state->out_buf_len - total);
		if(bytes < 0)
			return ARCHIVE_FATAL; // No recovery is possible if we can no longer read from the child.
		if(bytes == 0)
			break; // We got EOF from the child
		total += bytes;
		p += bytes;
	}
	*buff = state->out_buf;
	return (total);
}

static int program_filter_close(ArchiveReadFilter * self)
{
	struct program_filter * state = (struct program_filter *)self->data;
	int e = child_stop(self, state);
	// Release our private data
	SAlloc::F(state->out_buf);
	archive_string_free(&state->description);
	SAlloc::F(state);
	return (e);
}
