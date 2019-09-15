/*-
 * Copyright (c) 2011 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

static const char * parse_option(char ** str, const char ** mod, const char ** opt, const char ** val);

int _archive_set_option(struct archive * a, const char * m, const char * o, const char * v, int magic, const char * fn, option_handler use_option)
{
	const char * mp, * op, * vp;
	int r;
	archive_check_magic(a, magic, ARCHIVE_STATE_NEW, fn);
	mp = (m != NULL && m[0] != '\0') ? m : NULL;
	op = (o != NULL && o[0] != '\0') ? o : NULL;
	vp = (v != NULL && v[0] != '\0') ? v : NULL;
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
	archive_check_magic(a, magic, ARCHIVE_STATE_NEW, fn);
	if(isempty(options))
		return ARCHIVE_OK;
	else {
		char * data = sstrdup(options);
		if(!data) {
			archive_set_error(a, ENOMEM, "Out of memory adding file to list");
			return ARCHIVE_FATAL;
		}
		else {
			char * s = data;
			do {
				const char * mod = 0;
				const char * opt = 0;
				const char * val = 0;
				parse_option(&s, &mod, &opt, &val);
				if(mod == NULL && opt != NULL && strcmp("__ignore_wrong_module_name__", opt) == 0) {
					/* Ignore module name error */
					if(val != NULL) {
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
	}
}

static const char * parse_option(char ** s, const char ** m, const char ** o, const char ** v)
{
	char * end = NULL;
	const char * mod = NULL;
	const char * val = "1";
	char * opt = *s;
	char * p = sstrchr(opt, ',');
	if(p != NULL) {
		*p = '\0';
		end = (p + 1);
	}
	if(strlen(opt) == 0) {
		*s = end;
		*m = NULL;
		*o = NULL;
		*v = NULL;
		return end;
	}
	p = sstrchr(opt, ':');
	if(p != NULL) {
		*p = '\0';
		mod = opt;
		opt = ++p;
	}
	p = sstrchr(opt, '=');
	if(p != NULL) {
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
