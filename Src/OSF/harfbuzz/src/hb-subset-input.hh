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
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */
#ifndef HB_SUBSET_INPUT_HH
#define HB_SUBSET_INPUT_HH

#include "hb.hh"
#include "hb-subset.h"
#include "hb-font.hh"

struct hb_subset_input_t {
	hb_object_header_t header;

	hb_set_t * unicodes;
	hb_set_t * glyphs;
	hb_set_t * name_ids;
	hb_set_t * name_languages;
	hb_set_t * drop_tables;

	bool drop_hints;
	bool desubroutinize;
	bool retain_gids;
	bool name_legacy;
	/* TODO
	 *
	 * features
	 * lookups
	 * name_ids
	 * ...
	 */
};

#endif /* HB_SUBSET_INPUT_HH */
