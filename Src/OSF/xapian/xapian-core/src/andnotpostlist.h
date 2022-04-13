/** @file
 * @brief PostList class implementing Query::OP_AND_NOT
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_ANDNOTPOSTLIST_H
#define XAPIAN_INCLUDED_ANDNOTPOSTLIST_H

class PostListTree;

/// PostList class implementing Query::OP_AND_NOT
class AndNotPostList : public WrapperPostList {
	PostList * r; /// Right-hand side of OP_NOT.
	Xapian::docid r_did = 0; /// Current docid from r (or 0).
	Xapian::doccount db_size; /// Total number of documents in the database.
public:
	AndNotPostList(PostList * left, PostList * right, PostListTree* /*pltree*/, Xapian::doccount db_size_) : 
		WrapperPostList(left), r(right), db_size(db_size_)
	{
	}
	~AndNotPostList() 
	{
		delete r;
	}
	Xapian::doccount get_termfreq_min() const;
	Xapian::doccount get_termfreq_max() const;
	Xapian::doccount get_termfreq_est() const;
	TermFreqs get_termfreq_est_using_stats(const Xapian::Weight::Internal & stats) const;
	PostList * next(double w_min);
	PostList * skip_to(Xapian::docid did, double w_min);
	PostList * check(Xapian::docid did, double w_min, bool& valid);
	std::string get_description() const;
};

#endif // XAPIAN_INCLUDED_ANDNOTPOSTLIST_H
