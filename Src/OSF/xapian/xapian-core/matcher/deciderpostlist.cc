/** @file
 * @brief PostList which applies a MatchDecider
 */
// Copyright 2017 Olly Betts
// @licence GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "deciderpostlist.h"
#include <xapian/matchdecider.h>

using namespace std;

bool DeciderPostList::test_doc()
{
	// We know that doc holds a ValueStreamDocument.
	Xapian::Document::Internal* doc_int = doc.internal.get();
	ValueStreamDocument* vsdoc = static_cast<ValueStreamDocument*>(doc_int);
	vsdoc->set_shard_document(pl->get_docid());
	bool decision = (*decider)(doc);
	if(decision) {
		++decider->docs_allowed_;
	}
	else {
		++decider->docs_denied_;
	}
	return decision;
}

string DeciderPostList::get_description() const
{
	string desc = "DeciderPostList(";
	desc += pl->get_description();
	desc += ')';
	return desc;
}
