/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

struct write_lrzip {
	struct archive_write_program_data * pdata;
	int compression_level;
	enum { lzma = 0, bzip2, gzip, lzo, none, zpaq } compression;
};

static int archive_write_lrzip_open(struct archive_write_filter *);
static int archive_write_lrzip_options(struct archive_write_filter *, const char *, const char *);
static int archive_write_lrzip_write(struct archive_write_filter *, const void *, size_t);
static int archive_write_lrzip_close(struct archive_write_filter *);
static int archive_write_lrzip_free(struct archive_write_filter *);

int archive_write_add_filter_lrzip(Archive * _a)
{
	struct archive_write_filter * f = __archive_write_allocate_filter(_a);
	struct write_lrzip * data;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	data = (write_lrzip *)SAlloc::C(1, sizeof(*data));
	if(!data) {
		archive_set_error(_a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	data->pdata = __archive_write_program_allocate("lrzip");
	if(data->pdata == NULL) {
		SAlloc::F(data);
		archive_set_error(_a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	f->name = "lrzip";
	f->code = ARCHIVE_FILTER_LRZIP;
	f->data = data;
	f->FnOpen = archive_write_lrzip_open;
	f->FnOptions = archive_write_lrzip_options;
	f->FnWrite = archive_write_lrzip_write;
	f->FnClose = archive_write_lrzip_close;
	f->FnFree = archive_write_lrzip_free;
	// Note: This filter always uses an external program, so we return "warn" to inform of the fact. 
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external lrzip program for lrzip compression");
	return ARCHIVE_WARN;
}

static int archive_write_lrzip_options(struct archive_write_filter * f, const char * key, const char * value)
{
	struct write_lrzip * data = (struct write_lrzip *)f->data;
	if(sstreq(key, "compression")) {
		if(value == NULL)
			return ARCHIVE_WARN;
		else if(sstreq(value, "bzip2"))
			data->compression = write_lrzip::bzip2;
		else if(sstreq(value, "gzip"))
			data->compression = write_lrzip::gzip;
		else if(sstreq(value, "lzo"))
			data->compression = write_lrzip::lzo;
		else if(sstreq(value, "none"))
			data->compression = write_lrzip::none;
		else if(sstreq(value, "zpaq"))
			data->compression = write_lrzip::zpaq;
		else
			return ARCHIVE_WARN;
		return ARCHIVE_OK;
	}
	else if(sstreq(key, "compression-level")) {
		if(value == NULL || !(value[0] >= '1' && value[0] <= '9') || value[1] != '\0')
			return ARCHIVE_WARN;
		data->compression_level = value[0] - '0';
		return ARCHIVE_OK;
	}
	// Note: The "warn" return is just to inform the options supervisor that we didn't handle it.  It will generate a suitable error if no one used this option
	return ARCHIVE_WARN;
}

static int archive_write_lrzip_open(struct archive_write_filter * f)
{
	struct write_lrzip * data = (struct write_lrzip *)f->data;
	archive_string as;
	int r;
	archive_string_init(&as);
	archive_strcpy(&as, "lrzip -q");
	/* Specify compression type. */
	switch(data->compression) {
		case write_lrzip::lzma:/* default compression */
		    break;
		case write_lrzip::bzip2:
		    archive_strcat(&as, " -b");
		    break;
		case write_lrzip::gzip:
		    archive_strcat(&as, " -g");
		    break;
		case write_lrzip::lzo:
		    archive_strcat(&as, " -l");
		    break;
		case write_lrzip::none:
		    archive_strcat(&as, " -n");
		    break;
		case write_lrzip::zpaq:
		    archive_strcat(&as, " -z");
		    break;
	}
	/* Specify compression level. */
	if(data->compression_level > 0) {
		archive_strcat(&as, " -L ");
		archive_strappend_char(&as, '0' + data->compression_level);
	}

	r = __archive_write_program_open(f, data->pdata, as.s);
	archive_string_free(&as);
	return r;
}

static int archive_write_lrzip_write(struct archive_write_filter * f, const void * buff, size_t length)
{
	struct write_lrzip * data = (struct write_lrzip *)f->data;
	return __archive_write_program_write(f, data->pdata, buff, length);
}

static int archive_write_lrzip_close(struct archive_write_filter * f)
{
	struct write_lrzip * data = (struct write_lrzip *)f->data;
	return __archive_write_program_close(f, data->pdata);
}

static int archive_write_lrzip_free(struct archive_write_filter * f)
{
	struct write_lrzip * data = (struct write_lrzip *)f->data;
	__archive_write_program_free(data->pdata);
	SAlloc::F(data);
	return ARCHIVE_OK;
}
