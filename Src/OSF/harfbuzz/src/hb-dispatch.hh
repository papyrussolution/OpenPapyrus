/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2012,2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */
#ifndef HB_DISPATCH_HH
#define HB_DISPATCH_HH

#include "hb.hh"
/*
 * Dispatch
 */
template <typename Context, typename Return = hb_empty_t, uint MaxDebugDepth = 0>
    struct hb_dispatch_context_t {
	hb_dispatch_context_t() : debug_depth(0) {
	}
private:
	/* https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern */
	const Context* thiz() const {
		return static_cast<const Context *> (this);
	}
	Context* thiz() {
		return static_cast<      Context *> (this);
	}
public:
	const char * get_name() { return "UNKNOWN"; }
	static constexpr unsigned max_debug_depth = MaxDebugDepth;
	typedef Return return_t;
	template <typename T, typename F>
	bool may_dispatch(const T * obj CXX_UNUSED_PARAM, const F * format CXX_UNUSED_PARAM) {
		return true;
	}
	template <typename T, typename ... Ts> return_t dispatch(const T &obj, Ts&&... ds)
	{
		return obj.dispatch(thiz(), hb_forward<Ts> (ds) ...);
	}
	static return_t no_dispatch_return_value() {
		return Context::default_return_value();
	}
	static bool stop_sublookup_iteration(const return_t r CXX_UNUSED_PARAM) {
		return false;
	}
	unsigned debug_depth;
};

#endif /* HB_DISPATCH_HH */
