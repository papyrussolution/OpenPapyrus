/*-
 * Copyright (c) 2014 Sebastian Freundt
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
/**
 * WARC is standardised by ISO TC46/SC4/WG12 and currently available as
 * ISO 28500:2009.
 * For the purposes of this file we used the final draft from:
 * http://bibnum.bnf.fr/warc/WARC_ISO_28500_version1_latestdraft.pdf
 *
 * Todo:
 * [ ] real-world warcs can contain resources at endpoints ending in /
 *     e.g. http://bibnum.bnf.fr/warc/
 *     if you're lucky their response contains a Content-Location: header
 *     pointing to a unix-compliant filename, in the example above it's
 *     Content-Location: http://bibnum.bnf.fr/warc/index.html
 *     however, that's not mandated and github for example doesn't follow
 *     this convention.
 *     We need a set of archive options to control what to do with
 *     entries like these, at the moment care is taken to skip them.
 *
 **/
//#include "archive_read_private.h"

typedef enum {
	WT_NONE,
	WT_INFO, /* warcinfo */
	WT_META, /* metadata */
	WT_RSRC, /* resource */
	WT_REQ, /* request, unsupported */
	WT_RSP, /* response, unsupported */
	WT_RVIS, /* revisit, unsupported */
	WT_CONV, /* conversion, unsupported */
	WT_CONT, /* continuation, unsupported at the moment */
	LAST_WT /* invalid type */
} warc_type_t;

typedef struct {
	size_t len;
	const char * str;
} warc_string_t;

typedef struct {
	size_t len;
	char * str;
} warc_strbuf_t;

struct warc_s {
	/* content length ahead */
	size_t cntlen;
	/* and how much we've processed so far */
	size_t cntoff;
	/* and how much we need to consume between calls */
	size_t unconsumed;

	/* string pool */
	warc_strbuf_t pool;
	/* previous version */
	uint pver;
	/* stringified format name */
	archive_string sver;
};

static int _warc_bid(ArchiveRead * a, int);
static int _warc_cleanup(ArchiveRead * a);
static int _warc_read(ArchiveRead*, const void**, size_t*, int64*);
static int _warc_skip(ArchiveRead * a);
static int _warc_rdhdr(ArchiveRead * a, ArchiveEntry * e);

/* private routines */
static uint _warc_rdver(const char buf[10], size_t bsz);
static uint _warc_rdtyp(const char * buf, size_t bsz);
static warc_string_t _warc_rduri(const char * buf, size_t bsz);
static ssize_t _warc_rdlen(const char * buf, size_t bsz);
static time_t _warc_rdrtm(const char * buf, size_t bsz);
static time_t _warc_rdmtm(const char * buf, size_t bsz);
static const char * _warc_find_eoh(const char * buf, size_t bsz);
static const char * _warc_find_eol(const char * buf, size_t bsz);

int archive_read_support_format_warc(Archive * _a)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	struct warc_s * w;
	int r;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if((w = static_cast<struct warc_s *>(SAlloc::C(1, sizeof(*w)))) == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate warc data");
		return ARCHIVE_FATAL;
	}
	r = __archive_read_register_format(a, w, "warc", _warc_bid, NULL, _warc_rdhdr, _warc_read, _warc_skip, NULL, _warc_cleanup, NULL, NULL);
	if(r != ARCHIVE_OK) {
		SAlloc::F(w);
		return r;
	}
	return ARCHIVE_OK;
}

static int _warc_cleanup(ArchiveRead * a)
{
	struct warc_s * w = static_cast<struct warc_s *>(a->format->data);
	if(w->pool.len > 0U) {
		SAlloc::F(w->pool.str);
	}
	archive_string_free(&w->sver);
	SAlloc::F(w);
	a->format->data = NULL;
	return ARCHIVE_OK;
}

static int _warc_bid(ArchiveRead * a, int best_bid)
{
	const char * hdr;
	ssize_t nrd;
	uint ver;
	CXX_UNUSED(best_bid);
	/* check first line of file, it should be a record already */
	if((hdr = static_cast<const char *>(__archive_read_ahead(a, 12U, &nrd))) == NULL) {
		/* no idea what to do */
		return -1;
	}
	else if(nrd < 12) {
		/* nah, not for us, our magic cookie is at least 12 bytes */
		return -1;
	}
	/* otherwise snarf the record's version number */
	ver = _warc_rdver(hdr, nrd);
	if(ver < 1200U || ver > 10000U) {
		/* we only support WARC 0.12 to 1.0 */
		return -1;
	}
	/* otherwise be confident */
	return (64);
}

static int _warc_rdhdr(ArchiveRead * a, ArchiveEntry * entry)
{
#define HDR_PROBE_LEN           (12U)
	struct warc_s * w = static_cast<struct warc_s *>(a->format->data);
	uint ver;
	const char * buf;
	ssize_t nrd;
	const char * eoh;
	/* for the file name, saves some strndup()'ing */
	warc_string_t fnam;
	/* warc record type, not that we really use it a lot */
	warc_type_t ftyp;
	/* content-length+error monad */
	ssize_t cntlen;
	/* record time is the WARC-Date time we reinterpret it as ctime */
	time_t rtime;
	/* mtime is the Last-Modified time which will be the entry's mtime */
	time_t mtime;

start_over:
	/* just use read_ahead() they keep track of unconsumed
	 * bits and bobs for us; no need to put an extra shift in
	 * and reproduce that functionality here */
	buf = static_cast<const char *>(__archive_read_ahead(a, HDR_PROBE_LEN, &nrd));
	if(nrd < 0) {
		/* no good */
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Bad record header");
		return ARCHIVE_FATAL;
	}
	else if(buf == NULL) {
		/* there should be room for at least WARC/bla\r\n
		 * must be EOF therefore */
		return (ARCHIVE_EOF);
	}
	/* looks good so far, try and find the end of the header now */
	eoh = _warc_find_eoh(buf, nrd);
	if(eoh == NULL) {
		/* still no good, the header end might be beyond the
		 * probe we've requested, but then again who'd cram
		 * so much stuff into the header *and* be 28500-compliant */
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Bad record header");
		return ARCHIVE_FATAL;
	}
	ver = _warc_rdver(buf, eoh - buf);
	/* we currently support WARC 0.12 to 1.0 */
	if(ver == 0U) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Invalid record version");
		return ARCHIVE_FATAL;
	}
	else if(ver < 1200U || ver > 10000U) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Unsupported record version: %u.%u", ver / 10000, (ver % 10000) / 100);
		return ARCHIVE_FATAL;
	}
	cntlen = _warc_rdlen(buf, eoh - buf);
	if(cntlen < 0) {
		/* nightmare!  the specs say content-length is mandatory
		* so I don't feel overly bad stopping the reader here */
		archive_set_error(&a->archive, EINVAL, "Bad content length");
		return ARCHIVE_FATAL;
	}
	rtime = _warc_rdrtm(buf, eoh - buf);
	if(rtime == (time_t)-1) {
		/* record time is mandatory as per WARC/1.0,
		 * so just barf here, fast and loud */
		archive_set_error(&a->archive, EINVAL, "Bad record time");
		return ARCHIVE_FATAL;
	}
	/* let the world know we're a WARC archive */
	a->archive.archive_format = ARCHIVE_FORMAT_WARC;
	if(ver != w->pver) {
		/* stringify this entry's version */
		archive_string_sprintf(&w->sver, "WARC/%u.%u", ver / 10000, (ver % 10000) / 100);
		/* remember the version */
		w->pver = ver;
	}
	/* start off with the type */
	ftyp = static_cast<warc_type_t>(_warc_rdtyp(buf, eoh - buf));
	/* and let future calls know about the content */
	w->cntlen = cntlen;
	w->cntoff = 0U;
	mtime = 0; /* Avoid compiling error on some platform. */

	switch(ftyp) {
		case WT_RSRC:
		case WT_RSP:
		    /* only try and read the filename in the cases that are
		     * guaranteed to have one */
		    fnam = _warc_rduri(buf, eoh - buf);
		    /* check the last character in the URI to avoid creating
		     * directory endpoints as files, see Todo above */
		    if(fnam.len == 0 || fnam.str[fnam.len - 1] == '/') {
			    /* break here for now */
			    fnam.len = 0U;
			    fnam.str = NULL;
			    break;
		    }
		    /* bang to our string pool, so we save a
		     * SAlloc::M()+SAlloc::F() roundtrip */
		    if(fnam.len + 1U > w->pool.len) {
			    w->pool.len = ((fnam.len + 64U) / 64U) * 64U;
			    w->pool.str = static_cast<char *>(SAlloc::R(w->pool.str, w->pool.len));
		    }
		    memcpy(w->pool.str, fnam.str, fnam.len);
		    w->pool.str[fnam.len] = '\0';
		    /* let no one else know about the pool, it's a secret, shhh */
		    fnam.str = w->pool.str;

		    /* snarf mtime or deduce from rtime
		     * this is a custom header added by our writer, it's quite
		     * hard to believe anyone else would go through with it
		     * (apart from being part of some http responses of course) */
		    if((mtime = _warc_rdmtm(buf, eoh - buf)) == (time_t)-1) {
			    mtime = rtime;
		    }
		    break;
		case WT_NONE:
		case WT_INFO:
		case WT_META:
		case WT_REQ:
		case WT_RVIS:
		case WT_CONV:
		case WT_CONT:
		case LAST_WT:
		default:
		    fnam.len = 0U;
		    fnam.str = NULL;
		    break;
	}

	/* now eat some of those delicious buffer bits */
	__archive_read_consume(a, eoh - buf);

	switch(ftyp) {
		case WT_RSRC:
		case WT_RSP:
		    if(fnam.len > 0U) {
			    /* populate entry object */
			    archive_entry_set_filetype(entry, AE_IFREG);
			    archive_entry_copy_pathname(entry, fnam.str);
			    archive_entry_set_size(entry, cntlen);
			    archive_entry_set_perm(entry, 0644);
			    /* rtime is the new ctime, mtime stays mtime */
			    archive_entry_set_ctime(entry, rtime, 0L);
			    archive_entry_set_mtime(entry, mtime, 0L);
			    break;
		    }
		// @fallthrough
		case WT_NONE:
		case WT_INFO:
		case WT_META:
		case WT_REQ:
		case WT_RVIS:
		case WT_CONV:
		case WT_CONT:
		case LAST_WT:
		default:
		    /* consume the content and start over */
		    _warc_skip(a);
		    goto start_over;
	}
	return ARCHIVE_OK;
}

static int _warc_read(ArchiveRead * a, const void ** buf, size_t * bsz, int64 * off)
{
	struct warc_s * w = static_cast<struct warc_s *>(a->format->data);
	const char * rab;
	ssize_t nrd;

	if(w->cntoff >= w->cntlen) {
eof:
		/* it's our lucky day, no work, we can leave early */
		*buf = NULL;
		*bsz = 0U;
		*off = w->cntoff + 4U /*for \r\n\r\n separator*/;
		w->unconsumed = 0U;
		return (ARCHIVE_EOF);
	}

	if(w->unconsumed) {
		__archive_read_consume(a, w->unconsumed);
		w->unconsumed = 0U;
	}
	rab = static_cast<const char *>(__archive_read_ahead(a, 1U, &nrd));
	if(nrd < 0) {
		*bsz = 0U;
		/* big catastrophe */
		return (int)nrd;
	}
	else if(nrd == 0) {
		goto eof;
	}
	else if((size_t)nrd > w->cntlen - w->cntoff) {
		/* clamp to content-length */
		nrd = w->cntlen - w->cntoff;
	}
	*off = w->cntoff;
	*bsz = nrd;
	*buf = rab;

	w->cntoff += nrd;
	w->unconsumed = (size_t)nrd;
	return ARCHIVE_OK;
}

static int _warc_skip(ArchiveRead * a)
{
	struct warc_s * w = static_cast<struct warc_s *>(a->format->data);
	__archive_read_consume(a, w->cntlen + 4U /*\r\n\r\n separator*/);
	w->cntlen = 0U;
	w->cntoff = 0U;
	return ARCHIVE_OK;
}

/* private routines */
static void * deconst(const void * c)
{
	return (char *)0x1 + (((const char *)c) - (const char *)0x1);
}

static char* xmemmem(const char * hay, const size_t haysize,
    const char * needle, const size_t needlesize)
{
	const char * const eoh = hay + haysize;
	const char * const eon = needle + needlesize;
	const char * hp;
	const char * np;
	const char * cand;
	uint hsum;
	uint nsum;
	uint eqp;

	/* trivial checks first
	 * a 0-sized needle is defined to be found anywhere in haystack
	 * then run strchr() to find a candidate in HAYSTACK (i.e. a portion
	 * that happens to begin with *NEEDLE) */
	if(needlesize == 0UL) {
		return static_cast<char *>(deconst(hay));
	}
	else if((hay = static_cast<const char *>(smemchr(hay, *needle, haysize))) == NULL) {
		/* trivial */
		return NULL;
	}
	/* First characters of haystack and needle are the same now. Both are
	 * guaranteed to be at least one character long.  Now computes the sum
	 * of characters values of needle together with the sum of the first
	 * needle_len characters of haystack. */
	for(hp = hay + 1U, np = needle + 1U, hsum = *hay, nsum = *hay, eqp = 1U; hp < eoh && np < eon; hsum ^= *hp, nsum ^= *np, eqp &= *hp == *np, hp++, np++);

	/* HP now references the (NEEDLESIZE + 1)-th character. */
	if(np < eon) {
		/* haystack is smaller than needle, :O */
		return NULL;
	}
	else if(eqp) {
		/* found a match */
		return static_cast<char *>(deconst(hay));
	}
	/* now loop through the rest of haystack,
	 * updating the sum iteratively */
	for(cand = hay; hp < eoh; hp++) {
		hsum ^= *cand++;
		hsum ^= *hp;
		/* Since the sum of the characters is already known to be
		 * equal at that point, it is enough to check just NEEDLESIZE - 1
		 * characters for equality,
		 * also CAND is by design < HP, so no need for range checks */
		if(hsum == nsum && memcmp(cand, needle, needlesize - 1U) == 0) {
			return static_cast<char *>(deconst(cand));
		}
	}
	return NULL;
}

static int strtoi_lim(const char * str, const char ** ep, int llim, int ulim)
{
	int res = 0;
	const char * sp;
	/* we keep track of the number of digits via rulim */
	int rulim;
	for(sp = str, rulim = ulim > 10 ? ulim : 10; res * 10 <= ulim && rulim && isdec(*sp); sp++, rulim /= 10) {
		res *= 10;
		res += *sp - '0';
	}
	if(sp == str) {
		res = -1;
	}
	else if(res < llim || res > ulim) {
		res = -2;
	}
	*ep = (const char *)sp;
	return res;
}

static time_t time_from_tm(struct tm * t)
{
#if HAVE_TIMEGM
	/* Use platform timegm() if available. */
	return (timegm(t));
#elif HAVE__MKGMTIME64
	return (_mkgmtime64(t));
#else
	/* Else use direct calculation using POSIX assumptions. */
	/* First, fix up tm_yday based on the year/month/day. */
	if(mktime(t) == (time_t)-1)
		return ((time_t)-1);
	/* Then we can compute timegm() from first principles. */
	return (t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_yday * 86400 + (t->tm_year - 70) * 31536000 + 
		((t->tm_year - 69) / 4) * 86400 - ((t->tm_year - 1) / 100) * 86400 + ((t->tm_year + 299) / 400) * 86400);
#endif
}

static time_t xstrpisotime(const char * s, char ** endptr)
{
/** like strptime() but strictly for ISO 8601 Zulu strings */
	struct tm tm;
	time_t res = (time_t)-1;
	/* make sure tm is clean */
	memzero(&tm, sizeof(tm));
	/* as a courtesy to our callers, and since this is a non-standard
	 * routine, we skip leading whitespace */
	while(*s == ' ' || *s == '\t')
		++s;
	/* read year */
	if((tm.tm_year = strtoi_lim(s, &s, 1583, 4095)) < 0 || *s++ != '-') {
		goto out;
	}
	/* read month */
	if((tm.tm_mon = strtoi_lim(s, &s, 1, 12)) < 0 || *s++ != '-') {
		goto out;
	}
	/* read day-of-month */
	if((tm.tm_mday = strtoi_lim(s, &s, 1, 31)) < 0 || *s++ != 'T') {
		goto out;
	}
	/* read hour */
	if((tm.tm_hour = strtoi_lim(s, &s, 0, 23)) < 0 || *s++ != ':') {
		goto out;
	}
	/* read minute */
	if((tm.tm_min = strtoi_lim(s, &s, 0, 59)) < 0 || *s++ != ':') {
		goto out;
	}
	/* read second */
	if((tm.tm_sec = strtoi_lim(s, &s, 0, 60)) < 0 || *s++ != 'Z') {
		goto out;
	}

	/* massage TM to fulfill some of POSIX' constraints */
	tm.tm_year -= 1900;
	tm.tm_mon--;

	/* now convert our custom tm struct to a unix stamp using UTC */
	res = time_from_tm(&tm);

out:
	if(endptr != NULL) {
		*endptr = static_cast<char *>(deconst(s));
	}
	return res;
}

static uint _warc_rdver(const char * buf, size_t bsz)
{
	static const char magic[] = "WARC/";
	const char * c;
	uint ver = 0U;
	uint end = 0U;
	if(bsz < 12 || memcmp(buf, magic, sizeof(magic) - 1U) != 0) {
		/* buffer too small or invalid magic */
		return ver;
	}
	/* looks good so far, read the version number for a laugh */
	buf += sizeof(magic) - 1U;
	if(isdec((uchar)buf[0U]) && (buf[1U] == '.') && isdec((uchar)buf[2U])) {
		/* we support a maximum of 2 digits in the minor version */
		if(isdec((uchar)buf[3U]))
			end = 1U;
		/* set up major version */
		ver = (buf[0U] - '0') * 10000U;
		/* set up minor version */
		if(end == 1U) {
			ver += (buf[2U] - '0') * 1000U;
			ver += (buf[3U] - '0') * 100U;
		}
		else
			ver += (buf[2U] - '0') * 100U;
		/*
		 * WARC below version 0.12 has a space-separated header
		 * WARC 0.12 and above terminates the version with a CRLF
		 */
		c = buf + 3U + end;
		if(ver >= 1200U) {
			if(memcmp(c, "\r\n", 2U) != 0)
				ver = 0U;
		}
		else {
			/* ver < 1200U */
			if(*c != ' ' && *c != '\t')
				ver = 0U;
		}
	}
	return ver;
}

static uint _warc_rdtyp(const char * buf, size_t bsz)
{
	static const char _key[] = "\r\nWARC-Type:";
	const char * val, * eol;

	if((val = xmemmem(buf, bsz, _key, sizeof(_key) - 1U)) == NULL) {
		/* no bother */
		return WT_NONE;
	}
	val += sizeof(_key) - 1U;
	if((eol = _warc_find_eol(val, buf + bsz - val)) == NULL) {
		/* no end of line */
		return WT_NONE;
	}

	/* overread whitespace */
	while(val < eol && (*val == ' ' || *val == '\t'))
		++val;

	if(val + 8U == eol) {
		if(memcmp(val, "resource", 8U) == 0)
			return WT_RSRC;
		else if(memcmp(val, "response", 8U) == 0)
			return WT_RSP;
	}
	return WT_NONE;
}

static warc_string_t _warc_rduri(const char * buf, size_t bsz)
{
	static const char _key[] = "\r\nWARC-Target-URI:";
	const char * val, * uri, * eol, * p;
	warc_string_t res = {0U, NULL};

	if((val = xmemmem(buf, bsz, _key, sizeof(_key) - 1U)) == NULL) {
		/* no bother */
		return res;
	}
	/* overread whitespace */
	val += sizeof(_key) - 1U;
	if((eol = _warc_find_eol(val, buf + bsz - val)) == NULL) {
		/* no end of line */
		return res;
	}

	while(val < eol && (*val == ' ' || *val == '\t'))
		++val;

	/* overread URL designators */
	if((uri = xmemmem(val, eol - val, "://", 3U)) == NULL) {
		/* not touching that! */
		return res;
	}

	/* spaces inside uri are not allowed, CRLF should follow */
	for(p = val; p < eol; p++) {
		if(isspace((uchar)*p))
			return res;
	}

	/* there must be at least space for ftp */
	if(uri < (val + 3U))
		return res;

	/* move uri to point to after :// */
	uri += 3U;

	/* now then, inspect the URI */
	if(memcmp(val, "file", 4U) == 0) {
		/* perfect, nothing left to do here */
	}
	else if(memcmp(val, "http", 4U) == 0 ||
	    memcmp(val, "ftp", 3U) == 0) {
		/* overread domain, and the first / */
		while(uri < eol && *uri++ != '/');
	}
	else {
		/* not sure what to do? best to bugger off */
		return res;
	}
	res.str = uri;
	res.len = eol - uri;
	return res;
}

static ssize_t _warc_rdlen(const char * buf, size_t bsz)
{
	static const char _key[] = "\r\nContent-Length:";
	const char * val, * eol;
	char * on = NULL;
	long int len;

	if((val = xmemmem(buf, bsz, _key, sizeof(_key) - 1U)) == NULL) {
		/* no bother */
		return -1;
	}
	val += sizeof(_key) - 1U;
	if((eol = _warc_find_eol(val, buf + bsz - val)) == NULL) {
		/* no end of line */
		return -1;
	}

	/* skip leading whitespace */
	while(val < eol && (*val == ' ' || *val == '\t'))
		val++;
	/* there must be at least one digit */
	if(!isdec((uchar)*val))
		return -1;
	errno = 0;
	len = strtol(val, &on, 10);
	if(errno != 0 || on != eol) {
		/* line must end here */
		return -1;
	}

	return (size_t)len;
}

static time_t _warc_rdrtm(const char * buf, size_t bsz)
{
	static const char _key[] = "\r\nWARC-Date:";
	const char * val, * eol;
	char * on = NULL;
	time_t res;

	if((val = xmemmem(buf, bsz, _key, sizeof(_key) - 1U)) == NULL) {
		/* no bother */
		return (time_t)-1;
	}
	val += sizeof(_key) - 1U;
	if((eol = _warc_find_eol(val, buf + bsz - val)) == NULL) {
		/* no end of line */
		return -1;
	}

	/* xstrpisotime() kindly overreads whitespace for us, so use that */
	res = xstrpisotime(val, &on);
	if(on != eol) {
		/* line must end here */
		return -1;
	}
	return res;
}

static time_t _warc_rdmtm(const char * buf, size_t bsz)
{
	static const char _key[] = "\r\nLast-Modified:";
	const char * val, * eol;
	char * on = NULL;
	time_t res;

	if((val = xmemmem(buf, bsz, _key, sizeof(_key) - 1U)) == NULL) {
		/* no bother */
		return (time_t)-1;
	}
	val += sizeof(_key) - 1U;
	if((eol = _warc_find_eol(val, buf + bsz - val)) == NULL) {
		/* no end of line */
		return -1;
	}

	/* xstrpisotime() kindly overreads whitespace for us, so use that */
	res = xstrpisotime(val, &on);
	if(on != eol) {
		/* line must end here */
		return -1;
	}
	return res;
}

static const char* _warc_find_eoh(const char * buf, size_t bsz)
{
	static const char _marker[] = "\r\n\r\n";
	const char * hit = xmemmem(buf, bsz, _marker, sizeof(_marker) - 1U);

	if(hit != NULL) {
		hit += sizeof(_marker) - 1U;
	}
	return hit;
}

static const char* _warc_find_eol(const char * buf, size_t bsz)
{
	static const char _marker[] = "\r\n";
	const char * hit = xmemmem(buf, bsz, _marker, sizeof(_marker) - 1U);

	return hit;
}

/* archive_read_support_format_warc.c ends here */
