/*-
 * Copyright (c) 2009-2011 Michihiro NAKAJIMA
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
//#include "archive_read_private.h"

#define UUENCODE_BID_MAX_READ SKILOBYTE(128) /* Maximum lookahead during bid phase (in bytes) */

struct uudecode {
	int64 total;
	uchar * in_buff;
#define IN_BUFF_SIZE    (1024)
	int in_cnt;
	size_t in_allocated;
	uchar * out_buff;
#define OUT_BUFF_SIZE   (64 * 1024)
	int state;
#define ST_FIND_HEAD    0
#define ST_READ_UU      1
#define ST_UUEND        2
#define ST_READ_BASE64  3
#define ST_IGNORE       4
};

static int uudecode_bidder_bid(ArchiveReadFilterBidder *, ArchiveReadFilter * filter);
static int uudecode_bidder_init(ArchiveReadFilter *);
static ssize_t  uudecode_filter_read(ArchiveReadFilter *, const void **);
static int uudecode_filter_close(ArchiveReadFilter *);

#if ARCHIVE_VERSION_NUMBER < 4000000
/* Deprecated; remove in libarchive 4.0 */
int archive_read_support_compression_uu(Archive * a)
{
	return archive_read_support_filter_uu(a);
}

#endif

int archive_read_support_filter_uu(Archive * _a)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	ArchiveReadFilterBidder * bidder;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	bidder->data = NULL;
	bidder->name = "uu";
	bidder->FnBid = uudecode_bidder_bid;
	bidder->FnInit = uudecode_bidder_init;
	bidder->FnOptions = NULL;
	bidder->FnFree = NULL;
	return ARCHIVE_OK;
}

static const uchar ascii[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, '\r', 0, 0, /* 00 - 0F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 - 1F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 20 - 2F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 30 - 3F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 - 4F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 50 - 5F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 - 6F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, /* 70 - 7F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 - 8F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 - 9F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0 - AF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0 - BF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0 - CF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0 - DF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0 - EF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F0 - FF */
};

static const uchar uuchar[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 - 0F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 - 1F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 20 - 2F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 30 - 3F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 - 4F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 50 - 5F */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 60 - 6F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 70 - 7F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 - 8F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 - 9F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0 - AF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0 - BF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0 - CF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0 - DF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0 - EF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F0 - FF */
};

static const uchar base64[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 - 0F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 - 1F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, /* 20 - 2F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, /* 30 - 3F */
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 - 4F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 50 - 5F */
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 - 6F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70 - 7F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 - 8F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 - 9F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0 - AF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0 - BF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0 - CF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0 - DF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0 - EF */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F0 - FF */
};

static const int base64num[128] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  /* 00 - 0F */
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  /* 10 - 1F */
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0, 62,  0,  0,  0, 63,  /* 20 - 2F */
	52, 53, 54, 55, 56, 57, 58, 59,
	60, 61,  0,  0,  0,  0,  0,  0, /* 30 - 3F */
	0,  0,  1,  2,  3,  4,  5,  6,
	7,  8,  9, 10, 11, 12, 13, 14,  /* 40 - 4F */
	15, 16, 17, 18, 19, 20, 21, 22,
	23, 24, 25,  0,  0,  0,  0,  0, /* 50 - 5F */
	0, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, /* 60 - 6F */
	41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51,  0,  0,  0,  0,  0, /* 70 - 7F */
};

static ssize_t get_line(const uchar * b, ssize_t avail, ssize_t * nlsize)
{
	ssize_t len = 0;
	while(len < avail) {
		switch(ascii[*b]) {
			case 0: /* Non-ascii character or control character. */
				ASSIGN_PTR(nlsize, 0);
			    return -1;
			case '\r':
			    if(avail-len > 1 && b[1] == '\n') {
					ASSIGN_PTR(nlsize, 2);
				    return (len+2);
			    }
			// @fallthrough
			case '\n':
				ASSIGN_PTR(nlsize, 1);
			    return (len+1);
			case 1:
			    b++;
			    len++;
			    break;
		}
	}
	ASSIGN_PTR(nlsize, 0);
	return (avail);
}

static ssize_t bid_get_line(ArchiveReadFilter * filter, const uchar ** b, ssize_t * avail, ssize_t * ravail, ssize_t * nl, size_t* nbytes_read)
{
	ssize_t len;
	int quit = 0;
	if(*avail == 0) {
		*nl = 0;
		len = 0;
	}
	else
		len = get_line(*b, *avail, nl);
	/*
	 * Read bytes more while it does not reach the end of line.
	 */
	while(*nl == 0 && len == *avail && !quit && *nbytes_read < UUENCODE_BID_MAX_READ) {
		ssize_t diff = *ravail - *avail;
		size_t nbytes_req = (*ravail+1023) & ~1023U;
		ssize_t tested;
		/* Increase reading bytes if it is not enough to at least
		 * new two lines. */
		if(nbytes_req < (size_t)*ravail + 160)
			nbytes_req <<= 1;
		*b = static_cast<const uchar *>(__archive_read_filter_ahead(filter, nbytes_req, avail));
		if(*b == NULL) {
			if(*ravail >= *avail)
				return 0;
			/* Reading bytes reaches the end of a stream. */
			*b = static_cast<const uchar *>(__archive_read_filter_ahead(filter, *avail, avail));
			quit = 1;
		}
		*nbytes_read = *avail;
		*ravail = *avail;
		*b += diff;
		*avail -= diff;
		tested = len; /* Skip some bytes we already determinated. */
		len = get_line(*b + tested, *avail - tested, nl);
		if(len >= 0)
			len += tested;
	}
	return (len);
}

#define UUDECODE(c) (((c) - 0x20) & 0x3f)

static int uudecode_bidder_bid(ArchiveReadFilterBidder * self, ArchiveReadFilter * filter)
{
	const uchar * b;
	ssize_t avail, ravail;
	ssize_t len, nl;
	int l;
	int firstline;
	size_t nbytes_read;
	CXX_UNUSED(self);
	b = static_cast<const uchar *>(__archive_read_filter_ahead(filter, 1, &avail));
	if(!b)
		return 0;

	firstline = 20;
	ravail = avail;
	nbytes_read = avail;
	for(;;) {
		len = bid_get_line(filter, &b, &avail, &ravail, &nl, &nbytes_read);
		if(len < 0 || nl == 0)
			return 0; /* No match found. */
		if(len - nl >= 11 && memcmp(b, "begin ", 6) == 0)
			l = 6;
		else if(len -nl >= 18 && memcmp(b, "begin-base64 ", 13) == 0)
			l = 13;
		else
			l = 0;

		if(l > 0 && (b[l] < '0' || b[l] > '7' ||
		    b[l+1] < '0' || b[l+1] > '7' ||
		    b[l+2] < '0' || b[l+2] > '7' || b[l+3] != ' '))
			l = 0;

		b += len;
		avail -= len;
		if(l)
			break;
		firstline = 0;

		/* Do not read more than UUENCODE_BID_MAX_READ bytes */
		if(nbytes_read >= UUENCODE_BID_MAX_READ)
			return 0;
	}
	if(!avail)
		return 0;
	len = bid_get_line(filter, &b, &avail, &ravail, &nl, &nbytes_read);
	if(len < 0 || nl == 0)
		return 0; /* There are non-ascii characters. */
	avail -= len;

	if(l == 6) {
		/* "begin " */
		if(!uuchar[*b])
			return 0;
		/* Get a length of decoded bytes. */
		l = UUDECODE(*b++); len--;
		if(l > 45)
			/* Normally, maximum length is 45(character 'M'). */
			return 0;
		if(l > len - nl)
			return 0; /* Line too short. */
		while(l) {
			if(!uuchar[*b++])
				return 0;
			--len;
			--l;
		}
		if(len-nl == 1 && (uuchar[*b] || /* Check sum. */ (*b >= 'a' && *b <= 'z'))) { /* Padding data(MINIX). */
			++b;
			--len;
		}
		b += nl;
		if(avail && uuchar[*b])
			return (firstline+30);
	}
	else if(l == 13) {
		/* "begin-base64 " */
		while(len-nl > 0) {
			if(!base64[*b++])
				return 0;
			--len;
		}
		b += nl;

		if(avail >= 5 && memcmp(b, "====\n", 5) == 0)
			return (firstline+40);
		if(avail >= 6 && memcmp(b, "====\r\n", 6) == 0)
			return (firstline+40);
		if(avail > 0 && base64[*b])
			return (firstline+30);
	}

	return 0;
}

static int uudecode_bidder_init(ArchiveReadFilter * self)
{
	struct uudecode   * uudecode;
	void * out_buff;
	void * in_buff;
	self->code = ARCHIVE_FILTER_UU;
	self->name = "uu";
	self->FnRead = uudecode_filter_read;
	self->skip = NULL; /* not supported */
	self->FnClose = uudecode_filter_close;
	uudecode = (struct uudecode *)SAlloc::C(sizeof(*uudecode), 1);
	out_buff = SAlloc::M(OUT_BUFF_SIZE);
	in_buff = SAlloc::M(IN_BUFF_SIZE);
	if(uudecode == NULL || out_buff == NULL || in_buff == NULL) {
		archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for uudecode");
		SAlloc::F(uudecode);
		SAlloc::F(out_buff);
		SAlloc::F(in_buff);
		return ARCHIVE_FATAL;
	}
	self->data = uudecode;
	uudecode->in_buff = static_cast<uchar *>(in_buff);
	uudecode->in_cnt = 0;
	uudecode->in_allocated = IN_BUFF_SIZE;
	uudecode->out_buff = static_cast<uchar *>(out_buff);
	uudecode->state = ST_FIND_HEAD;
	return ARCHIVE_OK;
}

static int ensure_in_buff_size(ArchiveReadFilter * self, struct uudecode * uudecode, size_t size)
{
	if(size > uudecode->in_allocated) {
		uchar * ptr;
		/*
		 * Calculate a new buffer size for in_buff.
		 * Increase its value until it has enough size we need.
		 */
		size_t newsize = uudecode->in_allocated;
		do {
			if(newsize < IN_BUFF_SIZE*32)
				newsize <<= 1;
			else
				newsize += IN_BUFF_SIZE;
		} while(size > newsize);
		/* Allocate the new buffer. */
		ptr = static_cast<uchar *>(SAlloc::M(newsize));
		if(ptr == NULL) {
			SAlloc::F(ptr);
			archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for uudecode");
			return ARCHIVE_FATAL;
		}
		/* Move the remaining data in in_buff into the new buffer. */
		if(uudecode->in_cnt)
			memmove(ptr, uudecode->in_buff, uudecode->in_cnt);
		/* Replace in_buff with the new buffer. */
		SAlloc::F(uudecode->in_buff);
		uudecode->in_buff = ptr;
		uudecode->in_allocated = newsize;
	}
	return ARCHIVE_OK;
}

static ssize_t uudecode_filter_read(ArchiveReadFilter * self, const void ** buff)
{
	struct uudecode * uudecode;
	const uchar * b, * d;
	uchar * out;
	ssize_t avail_in, ravail;
	ssize_t used;
	ssize_t total;
	ssize_t len, llen, nl;
	uudecode = (struct uudecode *)self->data;
read_more:
	d = static_cast<const uchar *>(__archive_read_filter_ahead(self->upstream, 1, &avail_in));
	if(d == NULL && avail_in < 0)
		return ARCHIVE_FATAL;
	/* Quiet a code analyzer; make sure avail_in must be zero
	 * when d is NULL. */
	if(d == NULL)
		avail_in = 0;
	used = 0;
	total = 0;
	out = uudecode->out_buff;
	ravail = avail_in;
	if(uudecode->state == ST_IGNORE) {
		used = avail_in;
		goto finish;
	}
	if(uudecode->in_cnt) {
		/*
		 * If there is remaining data which is saved by
		 * previous calling, use it first.
		 */
		if(ensure_in_buff_size(self, uudecode,
		    avail_in + uudecode->in_cnt) != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		memcpy(uudecode->in_buff + uudecode->in_cnt,
		    d, avail_in);
		d = uudecode->in_buff;
		avail_in += uudecode->in_cnt;
		uudecode->in_cnt = 0;
	}
	for(; used < avail_in; d += llen, used += llen) {
		int64 l, body;

		b = d;
		len = get_line(b, avail_in - used, &nl);
		if(len < 0) {
			/* Non-ascii character is found. */
			if(uudecode->state == ST_FIND_HEAD && (uudecode->total > 0 || total > 0)) {
				uudecode->state = ST_IGNORE;
				used = avail_in;
				goto finish;
			}
			archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
			return ARCHIVE_FATAL;
		}
		llen = len;
		if((nl == 0) && (uudecode->state != ST_UUEND)) {
			if(total == 0 && ravail <= 0) {
				/* There is nothing more to read, fail */
				archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Missing format data");
				return ARCHIVE_FATAL;
			}
			/*
			 * Save remaining data which does not contain
			 * NL('\n','\r').
			 */
			if(ensure_in_buff_size(self, uudecode, len)
			    != ARCHIVE_OK)
				return ARCHIVE_FATAL;
			if(uudecode->in_buff != b)
				memmove(uudecode->in_buff, b, len);
			uudecode->in_cnt = (int)len;
			if(total == 0) {
				/* Do not return 0; it means end-of-file.
				 * We should try to read bytes more. */
				__archive_read_filter_consume(self->upstream, ravail);
				goto read_more;
			}
			used += len;
			break;
		}
		switch(uudecode->state) {
			default:
			case ST_FIND_HEAD:
			    /* Do not read more than UUENCODE_BID_MAX_READ bytes */
			    if(total + len >= UUENCODE_BID_MAX_READ) {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Invalid format data");
				    return ARCHIVE_FATAL;
			    }
			    if(len - nl >= 11 && memcmp(b, "begin ", 6) == 0)
				    l = 6;
			    else if(len - nl >= 18 && memcmp(b, "begin-base64 ", 13) == 0)
				    l = 13;
			    else
				    l = 0;
			    if(l != 0 && b[l] >= '0' && b[l] <= '7' && b[l+1] >= '0' && b[l+1] <= '7' && b[l+2] >= '0' && b[l+2] <= '7' && b[l+3] == ' ') {
				    if(l == 6)
					    uudecode->state = ST_READ_UU;
				    else
					    uudecode->state = ST_READ_BASE64;
			    }
			    break;
			case ST_READ_UU:
			    if(total + len * 2 > OUT_BUFF_SIZE)
				    goto finish;
			    body = len - nl;
			    if(!uuchar[*b] || body <= 0) {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
				    return ARCHIVE_FATAL;
			    }
			    /* Get length of undecoded bytes of current line. */
			    l = UUDECODE(*b++);
			    body--;
			    if(l > body) {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
				    return ARCHIVE_FATAL;
			    }
			    if(l == 0) {
				    uudecode->state = ST_UUEND;
				    break;
			    }
			    while(l > 0) {
				    int n = 0;
				    if(!uuchar[b[0]] || !uuchar[b[1]])
					    break;
				    n = UUDECODE(*b++) << 18;
				    n |= UUDECODE(*b++) << 12;
				    *out++ = n >> 16; total++;
				    --l;

				    if(l > 0) {
					    if(!uuchar[b[0]])
						    break;
					    n |= UUDECODE(*b++) << 6;
					    *out++ = (n >> 8) & 0xFF; total++;
					    --l;
				    }
				    if(l > 0) {
					    if(!uuchar[b[0]])
						    break;
					    n |= UUDECODE(*b++);
					    *out++ = n & 0xFF; total++;
					    --l;
				    }
			    }
			    if(l) {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
				    return ARCHIVE_FATAL;
			    }
			    break;
			case ST_UUEND:
			    if(len - nl == 3 && memcmp(b, "end ", 3) == 0)
				    uudecode->state = ST_FIND_HEAD;
			    else {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
				    return ARCHIVE_FATAL;
			    }
			    break;
			case ST_READ_BASE64:
			    if(total + len * 2 > OUT_BUFF_SIZE)
				    goto finish;
			    l = len - nl;
			    if(l >= 3 && b[0] == '=' && b[1] == '=' &&
				b[2] == '=') {
				    uudecode->state = ST_FIND_HEAD;
				    break;
			    }
			    while(l > 0) {
				    int n = 0;
				    if(!base64[b[0]] || !base64[b[1]])
					    break;
				    n = base64num[*b++] << 18;
				    n |= base64num[*b++] << 12;
				    *out++ = n >> 16; total++;
				    l -= 2;

				    if(l > 0) {
					    if(*b == '=')
						    break;
					    if(!base64[*b])
						    break;
					    n |= base64num[*b++] << 6;
					    *out++ = (n >> 8) & 0xFF; total++;
					    --l;
				    }
				    if(l > 0) {
					    if(*b == '=')
						    break;
					    if(!base64[*b])
						    break;
					    n |= base64num[*b++];
					    *out++ = n & 0xFF; total++;
					    --l;
				    }
			    }
			    if(l && *b != '=') {
				    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Insufficient compressed data");
				    return ARCHIVE_FATAL;
			    }
			    break;
		}
	}
finish:
	if(ravail < avail_in)
		used -= avail_in - ravail;
	__archive_read_filter_consume(self->upstream, used);
	*buff = uudecode->out_buff;
	uudecode->total += total;
	return (total);
}

static int uudecode_filter_close(ArchiveReadFilter * self)
{
	struct uudecode * uudecode = (struct uudecode *)self->data;
	SAlloc::F(uudecode->in_buff);
	SAlloc::F(uudecode->out_buff);
	SAlloc::F(uudecode);
	return ARCHIVE_OK;
}
