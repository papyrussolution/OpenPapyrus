/** @file
 * @brief PostList which adds on a term-independent weight contribution
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_EXTRAWEIGHTPOSTLIST_H
#define XAPIAN_INCLUDED_EXTRAWEIGHTPOSTLIST_H

namespace Xapian {
	class Weight;
}

class PostListTree;

/// PostList which adds on a term-independent weight contribution
class ExtraWeightPostList : public WrapperPostList {
	void operator = (const ExtraWeightPostList&) = delete; /// Don't allow assignment.
	ExtraWeightPostList(const ExtraWeightPostList&) = delete; /// Don't allow copying.
	Xapian::Weight* weight;
	PostListTree* pltree;
	double max_extra;
public:
	ExtraWeightPostList(PostList * pl_, Xapian::Weight* weight_, PostListTree* pltree_) : WrapperPostList(pl_),
		weight(weight_), pltree(pltree_), max_extra(weight->get_maxextra()) 
	{
	}
	~ExtraWeightPostList() 
	{
		delete weight;
	}
	double get_weight(Xapian::termcount doclen, Xapian::termcount unique_terms, Xapian::termcount wdfdocmax) const;
	double recalc_maxweight();
	PostList * next(double w_min);
	PostList * skip_to(Xapian::docid, double w_min);
	std::string get_description() const;
};

#endif // XAPIAN_INCLUDED_EXTRAWEIGHTPOSTLIST_H
