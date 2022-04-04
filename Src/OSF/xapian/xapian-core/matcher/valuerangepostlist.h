/** @file
 * @brief Return document ids matching a range test on a specified doc value.
 */
/* Copyright 2007,2008,2009,2011 Olly Betts
 * Copyright 2009 Lemur Consulting Ltd
 * Copyright 2010 Richard Boulton
 */
#ifndef XAPIAN_INCLUDED_VALUERANGEPOSTLIST_H
#define XAPIAN_INCLUDED_VALUERANGEPOSTLIST_H

class ValueRangePostList : public PostList {
protected:
	const Xapian::Database::Internal * db;
	Xapian::valueno slot;
	const std::string begin, end;
	Xapian::doccount db_size;
	ValueList * valuelist;
	/// Disallow copying.
	ValueRangePostList(const ValueRangePostList &);
	/// Disallow assignment.
	void operator = (const ValueRangePostList &);
public:
	ValueRangePostList(const Xapian::Database::Internal * db_, Xapian::valueno slot_, const std::string &begin_, const std::string &end_) : 
		db(db_), slot(slot_), begin(begin_), end(end_), db_size(db->get_doccount()), valuelist(0) 
	{
	}
	~ValueRangePostList();
	Xapian::doccount get_termfreq_min() const;
	Xapian::doccount get_termfreq_est() const;
	Xapian::doccount get_termfreq_max() const;
	TermFreqs get_termfreq_est_using_stats(const Xapian::Weight::Internal & stats) const;
	Xapian::docid get_docid() const;
	double get_weight(Xapian::termcount doclen, Xapian::termcount unique_terms, Xapian::termcount wdfdocmax) const;
	Xapian::termcount get_wdfdocmax() const;
	double recalc_maxweight();
	PositionList * read_position_list();
	PostList * next(double w_min);
	PostList * skip_to(Xapian::docid, double w_min);
	PostList * check(Xapian::docid did, double w_min, bool &valid);
	bool at_end() const;
	Xapian::termcount count_matching_subqs() const;
	std::string get_description() const;
};

#endif /* XAPIAN_INCLUDED_VALUERANGEPOSTLIST_H */
