/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

struct write_grzip {
	struct archive_write_program_data * pdata;
};

static int archive_write_grzip_open(struct archive_write_filter *);
static int archive_write_grzip_options(struct archive_write_filter *, const char *, const char *);
static int archive_write_grzip_write(struct archive_write_filter *, const void *, size_t);
static int archive_write_grzip_close(struct archive_write_filter *);
static int archive_write_grzip_free(struct archive_write_filter *);

int archive_write_add_filter_grzip(Archive * _a)
{
	struct archive_write_filter * f = __archive_write_allocate_filter(_a);
	struct write_grzip * data;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	data = (write_grzip *)SAlloc::C(1, sizeof(*data));
	if(!data) {
		archive_set_error(_a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	data->pdata = __archive_write_program_allocate("grzip");
	if(data->pdata == NULL) {
		SAlloc::F(data);
		archive_set_error(_a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	f->name = "grzip";
	f->code = ARCHIVE_FILTER_GRZIP;
	f->data = data;
	f->FnOpen = archive_write_grzip_open;
	f->FnOptions = archive_write_grzip_options;
	f->FnWrite = archive_write_grzip_write;
	f->FnClose = archive_write_grzip_close;
	f->FnFree = archive_write_grzip_free;
	/* Note: This filter always uses an external program, so we
	 * return "warn" to inform of the fact. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external grzip program for grzip compression");
	return ARCHIVE_WARN;
}

static int archive_write_grzip_options(struct archive_write_filter * f, const char * key, const char * value)
{
	(void)f; /* UNUSED */
	CXX_UNUSED(key);
	(void)value; /* UNUSED */
	// Note: The "warn" return is just to inform the options supervisor that we didn't handle it.  It will generate a suitable error if no one used this option
	return ARCHIVE_WARN;
}

static int archive_write_grzip_open(struct archive_write_filter * f)
{
	struct write_grzip * data = (struct write_grzip *)f->data;
	return __archive_write_program_open(f, data->pdata, "grzip");
}

static int archive_write_grzip_write(struct archive_write_filter * f, const void * buff, size_t length)
{
	struct write_grzip * data = (struct write_grzip *)f->data;
	return __archive_write_program_write(f, data->pdata, buff, length);
}

static int archive_write_grzip_close(struct archive_write_filter * f)
{
	struct write_grzip * data = (struct write_grzip *)f->data;
	return __archive_write_program_close(f, data->pdata);
}

static int archive_write_grzip_free(struct archive_write_filter * f)
{
	struct write_grzip * data = (struct write_grzip *)f->data;
	__archive_write_program_free(data->pdata);
	SAlloc::F(data);
	return ARCHIVE_OK;
}
