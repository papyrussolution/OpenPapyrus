/*-
 * Copyright (c) 2011 Tim Kientzle
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
#include "archive_options_private.h"

static const char * parse_option(const char ** str,
    const char ** mod, const char ** opt, const char ** val);

int _archive_set_option(struct archive * a, const char * m, const char * o, const char * v,
    int magic, const char * fn, option_handler use_option)
{
	const char * mp, * op, * vp;
	int r;
	archive_check_magic(a, magic, ARCHIVE_STATE_NEW, fn);
	mp = isempty(m) ? 0 : m;
	op = isempty(o) ? 0 : o;
	vp = isempty(v) ? 0 : v;
	if(op == NULL && vp == NULL)
		return ARCHIVE_OK;
	if(op == NULL) {
		archive_set_error(a, ARCHIVE_ERRNO_MISC, "Empty option");
		return ARCHIVE_FAILED;
	}
	r = use_option(a, mp, op, vp);
	if(r == ARCHIVE_WARN - 1) {
		archive_set_error(a, ARCHIVE_ERRNO_MISC, "Unknown module name: `%s'", mp);
		return ARCHIVE_FAILED;
	}
	if(r == ARCHIVE_WARN) {
		archive_set_error(a, ARCHIVE_ERRNO_MISC, "Undefined option: `%s%s%s%s%s%s'", vp ? "" : "!", mp ? mp : "", mp ? ":" : "", op, vp ? "=" : "", vp ? vp : "");
		return ARCHIVE_FAILED;
	}
	return r;
}

int _archive_set_either_option(struct archive * a, const char * m, const char * o, const char * v,
    option_handler use_format_option, option_handler use_filter_option)
{
	int r1, r2;
	if(o == NULL && v == NULL)
		return ARCHIVE_OK;
	if(o == NULL)
		return ARCHIVE_FAILED;
	r1 = use_format_option(a, m, o, v);
	if(r1 == ARCHIVE_FATAL)
		return ARCHIVE_FATAL;
	r2 = use_filter_option(a, m, o, v);
	if(r2 == ARCHIVE_FATAL)
		return ARCHIVE_FATAL;
	if(r2 == ARCHIVE_WARN - 1)
		return r1;
	return r1 > r2 ? r1 : r2;
}

int _archive_set_options(struct archive * a, const char * options, int magic, const char * fn, option_handler use_option)
{
	int allok = 1, anyok = 0, ignore_mod_err = 0, r;
	char * data;
	const char * s, * mod, * opt, * val;
	archive_check_magic(a, magic, ARCHIVE_STATE_NEW, fn);
	if(isempty(options))
		return ARCHIVE_OK;
	if((data = sstrdup(options)) == NULL) {
		archive_set_error(a, ENOMEM, "Out of memory adding file to list");
		return ARCHIVE_FATAL;
	}
	s = (const char *)data;
	do {
		mod = opt = val = NULL;
		parse_option(&s, &mod, &opt, &val);
		if(mod == NULL && opt && sstreq("__ignore_wrong_module_name__", opt)) {
			// Ignore module name error 
			if(val) {
				ignore_mod_err = 1;
				anyok = 1;
			}
			continue;
		}
		r = use_option(a, mod, opt, val);
		if(r == ARCHIVE_FATAL) {
			SAlloc::F(data);
			return ARCHIVE_FATAL;
		}
		if(r == ARCHIVE_FAILED && mod != NULL) {
			SAlloc::F(data);
			return ARCHIVE_FAILED;
		}
		if(r == ARCHIVE_WARN - 1) {
			if(ignore_mod_err)
				continue;
			/* The module name is wrong. */
			archive_set_error(a, ARCHIVE_ERRNO_MISC, "Unknown module name: `%s'", mod);
			SAlloc::F(data);
			return ARCHIVE_FAILED;
		}
		if(r == ARCHIVE_WARN) {
			/* The option name is wrong. No-one used this. */
			archive_set_error(a, ARCHIVE_ERRNO_MISC, "Undefined option: `%s%s%s'", mod ? mod : "", mod ? ":" : "", opt);
			SAlloc::F(data);
			return ARCHIVE_FAILED;
		}
		if(r == ARCHIVE_OK)
			anyok = 1;
		else
			allok = 0;
	} while(s != NULL);
	SAlloc::F(data);
	return allok ? ARCHIVE_OK : anyok ? ARCHIVE_WARN : ARCHIVE_FAILED;
}

static const char * parse_option(const char ** s, const char ** m, const char ** o, const char ** v)
{
	const char * end, * mod, * opt, * val;
	char * p;
	end = NULL;
	mod = NULL;
	opt = *s;
	val = "1";
	p = (char *)(strchr(opt, ',')); // @badcast
	if(p) {
		*p = '\0';
		end = ((const char *)p) + 1;
	}
	if(0 == strlen(opt)) {
		*s = end;
		*m = NULL;
		*o = NULL;
		*v = NULL;
		return end;
	}
	p = (char *)strchr(opt, ':'); // @badcast
	if(p) {
		*p = '\0';
		mod = opt;
		opt = ++p;
	}
	p = (char *)strchr(opt, '='); // @badcast
	if(p) {
		*p = '\0';
		val = ++p;
	}
	else if(opt[0] == '!') {
		++opt;
		val = NULL;
	}

	*s = end;
	*m = mod;
	*o = opt;
	*v = val;

	return end;
}
