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

void hb_ot_face_t::init0(hb_face_t * face)
{
	this->face = face;
#define HB_OT_TABLE(Namespace, Type) Type.init0();
#include "hb-ot-face-table-list.hh"
#undef HB_OT_TABLE
}

void hb_ot_face_t::fini()
{
#define HB_OT_TABLE(Namespace, Type) Type.fini();
#include "hb-ot-face-table-list.hh"
#undef HB_OT_TABLE
}
