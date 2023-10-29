/*-
 * Copyright (c) 2003-2010 Tim Kientzle All rights reserved.
 */
#ifndef ARCHIVE_ACL_PRIVATE_H_INCLUDED
#define ARCHIVE_ACL_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif
//#include "archive_string.h"

struct archive_acl_entry {
	archive_acl_entry *next;
	int    type;    // E.g., access or default
	int    tag;     // E.g., user/group/other/mask 
	int    permset; // r/w/x bits
	int    id;      // uid/gid for user/group
	struct archive_mstring name; // uname/gname 
};

struct archive_acl {
	mode_t mode;
	archive_acl_entry * acl_head;
	archive_acl_entry * acl_p;
	int    acl_state; // See acl_next for details
	wchar_t * acl_text_w;
	char * acl_text;
	int    acl_types;
};

void archive_acl_clear(archive_acl *);
void archive_acl_copy(archive_acl *, archive_acl *);
int archive_acl_count(archive_acl *, int);
int FASTCALL archive_acl_types(const archive_acl *);
int archive_acl_reset(archive_acl *, int);
int archive_acl_next(Archive *, archive_acl *, int, int *, int *, int *, int *, const char **);
int archive_acl_add_entry(archive_acl *, int, int, int, int, const char *);
int archive_acl_add_entry_w_len(archive_acl *, int, int, int, int, const wchar_t *, size_t);
int archive_acl_add_entry_len(archive_acl *, int, int, int, int, const char *, size_t);
wchar_t *archive_acl_to_text_w(archive_acl *, ssize_t *, int, Archive *);
char *archive_acl_to_text_l(archive_acl *, ssize_t *, int, archive_string_conv *);
/*
 * ACL text parser.
 */
int archive_acl_from_text_w(archive_acl *, const wchar_t * /* wtext */, int /* type */);
int archive_acl_from_text_l(archive_acl *, const char * /* text */, int /* type */, archive_string_conv *);

#endif /* ARCHIVE_ENTRY_PRIVATE_H_INCLUDED */
