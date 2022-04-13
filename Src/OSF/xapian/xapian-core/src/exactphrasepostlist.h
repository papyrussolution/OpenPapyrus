/** @file
 * @brief Return docs containing terms forming a particular exact phrase.
 *
 * Copyright (C) 2006 Olly Betts
 * Copyright (C) 2009 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_EXACTPHRASEPOSTLIST_H
#define XAPIAN_INCLUDED_EXACTPHRASEPOSTLIST_H

class PostListTree;

/** Postlist which matches an exact phrase using positional information.
 *
 *  ExactPhrasePostList only returns a posting for documents contains
 *  all the terms (this part is implemented using an AndPostList) and
 *  additionally the terms occur somewhere in the document in the order given
 *  and at adjacent term positions.
 *
 *  The weight of a posting is the sum of the weights of the
 *  sub-postings (just like an AndPostList).
 */
class ExactPhrasePostList : public SelectPostList {
	std::vector <PostList *> terms;
	PositionList ** poslists;
	uint * order;
	void start_position_list(uint i); /// Start reading from the i-th position list.
	bool test_doc(); /// Test if the current document contains the terms as an exact phrase.
public:
	ExactPhrasePostList(PostList * source_, const std::vector <PostList *>::const_iterator &terms_begin, 
		const std::vector <PostList *>::const_iterator &terms_end, PostListTree* pltree_);
	~ExactPhrasePostList();
	Xapian::termcount get_wdf() const;
	Xapian::doccount get_termfreq_est() const;
	TermFreqs get_termfreq_est_using_stats(const Xapian::Weight::Internal & stats) const;
	std::string get_description() const;
};

#endif
