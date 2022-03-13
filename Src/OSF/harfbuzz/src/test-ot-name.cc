/*
 * Copyright © 2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Google Author(s): Behdad Esfahbod
 */
#include "harfbuzz-internal.h"
#pragma hdrstop

#ifdef HB_NO_OPEN
	#define hb_blob_create_from_file(x)  hb_blob_get_empty()
#endif

int main(int argc, char ** argv)
{
	if(argc != 2) {
		slfprintf_stderr("usage: %s font-file\n", argv[0]);
		exit(1);
	}
	hb_blob_t * blob = hb_blob_create_from_file(argv[1]);
	hb_face_t * face = hb_face_create(blob, 0 /* first face */);
	hb_blob_destroy(blob);
	blob = nullptr;
	uint count = 0;
#ifndef HB_NO_NAME
	const hb_ot_name_entry_t * entries = hb_ot_name_list_names(face, &count);
	for(uint i = 0; i < count; i++) {
		printf("%u	%s	", entries[i].name_id, hb_language_to_string(entries[i].language));
		char buf[64];
		uint buf_size = sizeof(buf);
		hb_ot_name_get_utf8(face, entries[i].name_id, entries[i].language, &buf_size, buf);
		printf("%s\n", buf);
	}
#endif
	hb_face_destroy(face);
	return count ? 0 : 1;
}
