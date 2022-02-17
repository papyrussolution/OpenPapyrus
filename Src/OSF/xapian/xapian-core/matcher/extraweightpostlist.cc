/** @file
 * @brief PostList which adds on a term-independent weight contribution
 */
// Copyright 2017 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "extraweightpostlist.h"
#include "postlisttree.h"
#include "str.h"

using namespace std;

double ExtraWeightPostList::get_weight(Xapian::termcount doclen, Xapian::termcount unique_terms, Xapian ::termcount wdfdocmax) const
{
	/* Weight::get_sumextra() takes three parameters (document length, number of
	 * unique terms and the max wdf in the document) but currently only doclen
	 * is actually used - unique_terms and wdfdocmax were added because they are
	 * likely to be wanted at some point, and so that we got all the
	 * incompatible changes needed to add support for the unique terms and max
	 * wdf over with in one go.
	 */
	double sum_extra = weight->get_sumextra(doclen, unique_terms, wdfdocmax);
	AssertRel(sum_extra, <=, max_extra);
	return pl->get_weight(doclen, unique_terms, wdfdocmax) + sum_extra;
}

double ExtraWeightPostList::recalc_maxweight()
{
	return pl->recalc_maxweight() + max_extra;
}

PostList* ExtraWeightPostList::next(double w_min)
{
	PostList* res = pl->next(w_min - max_extra);
	if(res) {
		delete pl;
		pl = res;
		pltree->force_recalc();
	}
	return NULL;
}

PostList* ExtraWeightPostList::skip_to(Xapian::docid, double)
{
	// ExtraWeightPostList's parent will be PostListTree which doesn't
	// call skip_to() or check().
	Assert(false);
	return NULL;
}

string ExtraWeightPostList::get_description() const
{
	string desc = "ExtraWeightPostList(";
	desc += pl->get_description();
	desc += ", max_extra=";
	desc += str(max_extra);
	desc += ')';
	return desc;
}
