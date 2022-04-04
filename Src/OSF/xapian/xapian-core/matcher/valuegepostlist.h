/** @file
 * @brief Return document ids matching a >= test on a specified doc value.
 */
/* Copyright 2007,2011 Olly Betts
 * Copyright 2008 Lemur Consulting Ltd
 */
#ifndef XAPIAN_INCLUDED_VALUEGEPOSTLIST_H
#define XAPIAN_INCLUDED_VALUEGEPOSTLIST_H

class ValueGePostList : public ValueRangePostList {
	ValueGePostList(const ValueGePostList &); /// Disallow copying.
	void operator = (const ValueGePostList &); /// Disallow assignment.
public:
	ValueGePostList(const Xapian::Database::Internal * db_, Xapian::valueno slot_, const std::string &begin_) : ValueRangePostList(db_, slot_, begin_, std::string()) 
	{
	}
	PostList * next(double w_min);
	PostList * skip_to(Xapian::docid, double w_min);
	PostList * check(Xapian::docid did, double w_min, bool &valid);
	std::string get_description() const;
};

#endif /* XAPIAN_INCLUDED_VALUEGEPOSTLIST_H */
