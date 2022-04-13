/** @file
 * @brief Result comparison functions.
 */
/* Copyright (C) 2006,2009,2013,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <xapian-internal.h>
#pragma hdrstop

/* We use templates to generate all the different comparison functions which we
 * need, which avoids having to write them all out by hand.
 */

// Order by docid, inlined.  Used as the last fallback by the others.
template <bool FORWARD_DID> static inline bool msetcmp_by_docid_inline(const Result& a, const Result& b)
{
	return FORWARD_DID ? (a.get_docid() < b.get_docid()) : (a.get_docid() > b.get_docid());
}

// Order by docid, used when relevance is always 0.
template <bool FORWARD_DID> static bool msetcmp_by_docid(const Result& a, const Result& b)
{
	return msetcmp_by_docid_inline<FORWARD_DID>(a, b);
}

// Order by relevance, then docid.
template <bool FORWARD_DID> static bool msetcmp_by_relevance(const Result& a, const Result& b)
{
	if(a.get_weight() > b.get_weight()) 
		return true;
	else if(a.get_weight() < b.get_weight()) 
		return false;
	else
		return msetcmp_by_docid_inline<FORWARD_DID>(a, b);
}

// Order by value, then docid.
template <bool FORWARD_VALUE, bool FORWARD_DID> static bool msetcmp_by_value(const Result& a, const Result& b)
{
	int sort_cmp = a.get_sort_key().compare(b.get_sort_key());
	if(sort_cmp > 0) 
		return FORWARD_VALUE;
	else if(sort_cmp < 0) 
		return !FORWARD_VALUE;
	else
		return msetcmp_by_docid_inline<FORWARD_DID>(a, b);
}

// Order by value, then relevance, then docid.
template <bool FORWARD_VALUE, bool FORWARD_DID> static bool msetcmp_by_value_then_relevance(const Result& a, const Result& b)
{
	int sort_cmp = a.get_sort_key().compare(b.get_sort_key());
	if(sort_cmp > 0) 
		return FORWARD_VALUE;
	else if(sort_cmp < 0) 
		return !FORWARD_VALUE;
	else if(a.get_weight() > b.get_weight()) 
		return true;
	else if(a.get_weight() < b.get_weight()) 
		return false;
	else
		return msetcmp_by_docid_inline<FORWARD_DID>(a, b);
}

// Order by relevance, then value, then docid.
template <bool FORWARD_VALUE, bool FORWARD_DID> static bool msetcmp_by_relevance_then_value(const Result& a, const Result& b)
{
	if(a.get_weight() > b.get_weight()) return true;
	if(a.get_weight() < b.get_weight()) return false;
	int sort_cmp = a.get_sort_key().compare(b.get_sort_key());
	if(sort_cmp > 0) return FORWARD_VALUE;
	if(sort_cmp < 0) return !FORWARD_VALUE;
	return msetcmp_by_docid_inline<FORWARD_DID>(a, b);
}

MSetCmp get_msetcmp_function(Xapian::Enquire::Internal::sort_setting sort_by, bool sort_forward, bool sort_val_reverse)
{
	switch(sort_by) {
		case Xapian::Enquire::Internal::DOCID:
		    return sort_forward ? msetcmp_by_docid<true> : msetcmp_by_docid<false>;
		case Xapian::Enquire::Internal::REL:
		    return sort_forward ? msetcmp_by_relevance<true> : msetcmp_by_relevance<false>;
		case Xapian::Enquire::Internal::VAL:
		    if(sort_forward) {
			    return sort_val_reverse ? msetcmp_by_value<true, true> : msetcmp_by_value<false, true>;
		    }
		    else {
			    return sort_val_reverse ? msetcmp_by_value<true, false> : msetcmp_by_value<false, false>;
		    }
		case Xapian::Enquire::Internal::VAL_REL:
		    if(sort_forward) {
			    return sort_val_reverse ? msetcmp_by_value_then_relevance<true, true> : msetcmp_by_value_then_relevance<false, true>;
		    }
		    else {
			    return sort_val_reverse ? msetcmp_by_value_then_relevance<true, false> : msetcmp_by_value_then_relevance<false, false>;
		    }
		default:
		    // Must be REL_VAL, but handle with "default" to avoid warnings
		    // about falling off the end of the function.
		    AssertEq(sort_by, Xapian::Enquire::Internal::REL_VAL);
		    if(sort_forward) {
			    return sort_val_reverse ? msetcmp_by_relevance_then_value<true, true> : msetcmp_by_relevance_then_value<false, true>;
		    }
		    else {
			    return sort_val_reverse ? msetcmp_by_relevance_then_value<true, false> : msetcmp_by_relevance_then_value<false, false>;
		    }
	}
}
