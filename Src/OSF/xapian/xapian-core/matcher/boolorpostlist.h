/** @file
 * @brief PostList class implementing unweighted Query::OP_OR
 */
// Copyright 2017,2018 Olly Betts
// @license GNU GPL
//
#ifndef XAPIAN_INCLUDED_BOOLORPOSTLIST_H
#define XAPIAN_INCLUDED_BOOLORPOSTLIST_H

#include "backends/postlist.h"

/// PostList class implementing unweighted Query::OP_OR
class BoolOrPostList : public PostList {
	void operator = (const BoolOrPostList&) = delete; /// Don't allow assignment.
	BoolOrPostList(const BoolOrPostList&) = delete; /// Don't allow copying.
	Xapian::docid did; /// The current docid, or zero if we haven't started or are at_end.
	size_t n_kids; /// The number of sub-postlists.

	struct PostListAndDocID {
		PostList * pl;
		Xapian::docid did = 0;
		PostListAndDocID() : pl(nullptr) 
		{
		}
		PostListAndDocID(PostList * pl_) : pl(pl_) 
		{
		}
		bool operator > (const PostListAndDocID& o) const { return did > o.did; }
	};
	PostListAndDocID* plist; /// Array of pointers to sub-postlists.
	Xapian::doccount db_size; /** Total number of documents in the database. */
	/** Helper to apply operation to all postlists matching current docid.
	 *
	 *  This function makes use of the heap structure, descending to any
	 *  children which match the current docid in an effectively recursive way
	 *  which needs O(1) storage, and evaluating func for each of them.
	 *
	 *  There's support for accumulating a value of type Xapian::termcount,
	 *  which is returned (of the three current uses, two want to accumulate a
	 *  value of this type, while the other doesn't need to accumulate a
	 *  value).
	 */
	template <typename F> Xapian::termcount for_all_matches(F func) const
	{
		size_t i = 0;
		Xapian::termcount result = 0;
		AssertEq(plist[0].did, did);
		while(true) {
			result += func(plist[i].pl);
			// Children of i are (2 * i + 1) and (2 * i + 2).
			size_t j = 2 * i + 1;
			if(j < n_kids && plist[j].did == did) {
				// Down left.
				i = j;
				continue;
			}
			if(j + 1 < n_kids && plist[j + 1].did == did) {
				// Down right.
				i = j + 1;
				continue;
			}
try_right:
			if((i & 1) && i + 1 < n_kids && plist[i + 1].did == did) {
				// Right.
				++i;
				continue;
			}
			// Up.
			i = (i - 1) / 2;
			if(i == 0) break;
			goto try_right;
		}
		return result;
	}

public:
	/** Construct from 2 random-access iterators to a container of PostList *,
	 *  a pointer to the matcher, and the document collection size.
	 */
	template <class RandomItor> BoolOrPostList(RandomItor pl_begin, RandomItor pl_end, Xapian::doccount db_size_) : 
		did(0), n_kids(pl_end - pl_begin), plist(NULL), db_size(db_size_)
	{
		plist = new PostListAndDocID[n_kids];
		// This initialises all entries to have did 0, so all entries are
		// equal, which is a valid heap.
		std::copy(pl_begin, pl_end, plist);
	}
	~BoolOrPostList();
	Xapian::doccount get_termfreq_min() const;
	Xapian::doccount get_termfreq_max() const;
	Xapian::doccount get_termfreq_est() const;
	TermFreqs get_termfreq_est_using_stats(const Xapian::Weight::Internal& stats) const;
	Xapian::docid get_docid() const;
	double get_weight(Xapian::termcount doclen, Xapian::termcount unique_terms, Xapian::termcount wdfdocmax) const;
	bool at_end() const;
	double recalc_maxweight();
	PostList * next(double w_min);
	PostList * skip_to(Xapian::docid did, double w_min);
	std::string get_description() const;
	Xapian::termcount get_wdf() const;
	Xapian::termcount count_matching_subqs() const;
	void gather_position_lists(OrPositionList* orposlist);
};

#endif // XAPIAN_INCLUDED_BOOLORPOSTLIST_H
