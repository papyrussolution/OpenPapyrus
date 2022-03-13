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
#ifndef HB_SUBSET_HH
#define HB_SUBSET_HH

#include "hb.hh"
#include "hb-subset.h"
#include "hb-machinery.hh"
#include "hb-subset-input.hh"
#include "hb-subset-plan.hh"

struct hb_subset_context_t : hb_dispatch_context_t<hb_subset_context_t, bool, HB_DEBUG_SUBSET> {
	const char * get_name() {
		return "SUBSET";
	}

	static return_t default_return_value() {
		return true;
	}

private:
	template <typename T, typename ... Ts> auto _dispatch(const T &obj, hb_priority<1>, Ts&&... ds) HB_AUTO_RETURN
		(obj.subset(this, hb_forward<Ts> (ds) ...) )
	template <typename T, typename ... Ts> auto _dispatch(const T &obj, hb_priority<0>, Ts&&... ds) HB_AUTO_RETURN
		(obj.dispatch(this, hb_forward<Ts> (ds) ...) )
public:
	template <typename T, typename ... Ts> auto dispatch(const T &obj, Ts&&... ds) HB_AUTO_RETURN
		(_dispatch(obj, hb_prioritize, hb_forward<Ts> (ds) ...) )

	hb_blob_t *source_blob;
	hb_subset_plan_t * plan;
	hb_serialize_context_t * serializer;
	hb_tag_t table_tag;

	hb_subset_context_t(hb_blob_t *source_blob_,
	    hb_subset_plan_t *plan_,
	    hb_serialize_context_t *serializer_,
	    hb_tag_t table_tag_) :
		source_blob(source_blob_),
		plan(plan_),
		serializer(serializer_),
		table_tag(table_tag_) {
	}
};

#endif /* HB_SUBSET_HH */
