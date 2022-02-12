/** @file
 * @brief Base class for classes which filter another PostList
 */
// Copyright 2017 Olly Betts
// @licence GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "selectpostlist.h"
#include "postlisttree.h"

bool SelectPostList::vet(double w_min)
{
	if(pl->at_end()) {
		ZDELETE(pl);
		return true;
	}
	// We assume that test_doc() is expensive compared to calculating the
	// weight.
	if(w_min <= 0.0) {
		cached_weight = -HUGE_VAL;
	}
	else {
		Xapian::termcount doclen = 0;
		Xapian::termcount unique_terms = 0;
		Xapian::termcount wdfdocmax = 0;
		pltree->get_doc_stats(pl->get_docid(), doclen, unique_terms, wdfdocmax);
		cached_weight = pl->get_weight(doclen, unique_terms, wdfdocmax);
		if(cached_weight < w_min)
			return false;
	}
	return test_doc();
}

double SelectPostList::get_weight(Xapian::termcount doclen, Xapian::termcount unique_terms, Xapian::termcount wdfdocmax) const
{
	return (cached_weight >= 0.0) ? cached_weight : pl->get_weight(doclen, unique_terms, wdfdocmax);
}

bool SelectPostList::at_end() const
{
	return pl == NULL;
}

PostList* SelectPostList::next(double w_min)
{
	do {
		PostList* result = pl->next(w_min);
		if(result) {
			delete pl;
			pl = result;
		}
	} while(!vet(w_min));
	return NULL;
}

PostList* SelectPostList::skip_to(Xapian::docid did, double w_min)
{
	if(did > pl->get_docid()) {
		PostList* result = pl->skip_to(did, w_min);
		if(result) {
			delete pl;
			pl = result;
		}
		if(!vet(w_min)) {
			// Advance to the next match.
			return SelectPostList::next(w_min);
		}
	}
	return NULL;
}

PostList* SelectPostList::check(Xapian::docid did, double w_min, bool& valid)
{
	PostList* result = pl->check(did, w_min, valid);
	if(result) {
		delete pl;
		pl = result;
	}
	if(valid) {
		// For check() we can simply indicate !valid if the vetting fails.
		valid = vet(w_min);
	}
	return NULL;
}

Xapian::doccount SelectPostList::get_termfreq_min() const
{
	// In general, it's possible no documents get selected.  Subclasses where
	// that's known not to be the case should provide their own implementation.
	return 0;
}
