/** @file
 * @brief Xapian::Query API class
 */
/* Copyright (C) 2011,2012,2013,2015,2016,2017,2018 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
	// Extra () are needed to resolve ambiguity with method declaration.
	const Query Query::MatchAll((string()));
	const Query Query::MatchNothing;
	Query::Query(const string & term, Xapian::termcount wqf, Xapian::termpos pos) : internal(new Xapian::Internal::QueryTerm(term, wqf, pos))
	{
		LOGCALL_CTOR(API, "Query", term | wqf | pos);
	}
	Query::Query(Xapian::PostingSource * source) : internal(new Xapian::Internal::QueryPostingSource(source))
	{
		LOGCALL_CTOR(API, "Query", source);
	}
	Query::Query(double factor, const Xapian::Query & subquery)
	{
		LOGCALL_CTOR(API, "Query", factor | subquery);
		if(!subquery.empty())
			internal = new Xapian::Internal::QueryScaleWeight(factor, subquery);
	}
	Query::Query(op op_, const Xapian::Query & subquery, double factor)
	{
		LOGCALL_CTOR(API, "Query", op_ | subquery | factor);
		if(UNLIKELY(op_ != OP_SCALE_WEIGHT))
			throw Xapian::InvalidArgumentError("op must be OP_SCALE_WEIGHT");
		// If the subquery is MatchNothing then generate Query() which matches
		// nothing.
		if(!subquery.internal.get()) return;
		switch(subquery.internal->get_type()) {
			case OP_VALUE_RANGE:
			case OP_VALUE_GE:
			case OP_VALUE_LE:
				// These operators always return weight 0, so OP_SCALE_WEIGHT has
				// no effect on them.
				internal = subquery.internal;
				return;
			default:
				break;
		}
		internal = new Xapian::Internal::QueryScaleWeight(factor, subquery);
	}

	Query::Query(op op_, Xapian::valueno slot, const std::string & limit)
	{
		LOGCALL_CTOR(API, "Query", op_ | slot | limit);
		if(op_ == OP_VALUE_GE) {
			if(limit.empty())
				internal = new Xapian::Internal::QueryTerm();
			else
				internal = new Xapian::Internal::QueryValueGE(slot, limit);
		}
		else if(LIKELY(op_ == OP_VALUE_LE)) {
			internal = new Xapian::Internal::QueryValueLE(slot, limit);
		}
		else {
			throw Xapian::InvalidArgumentError("op must be OP_VALUE_LE or OP_VALUE_GE");
		}
	}

	Query::Query(op op_, Xapian::valueno slot, const std::string & begin, const std::string & end)
	{
		LOGCALL_CTOR(API, "Query", op_ | slot | begin | end);
		if(UNLIKELY(op_ != OP_VALUE_RANGE))
			throw Xapian::InvalidArgumentError("op must be OP_VALUE_RANGE");
		// If begin > end then generate Query() which matches nothing.
		if(begin.empty()) {
			internal = new Xapian::Internal::QueryValueLE(slot, end);
		}
		else if(LIKELY(begin <= end)) {
			internal = new Xapian::Internal::QueryValueRange(slot, begin, end);
		}
	}

	Query::Query(op op_, const std::string & pattern, Xapian::termcount max_expansion, int flags, op combiner)
	{
		LOGCALL_CTOR(API, "Query", op_ | pattern | max_expansion | flags | combiner);
		if(UNLIKELY(combiner != OP_SYNONYM && combiner != OP_MAX && combiner != OP_OR))
			throw Xapian::InvalidArgumentError("combiner must be OP_SYNONYM or OP_MAX or OP_OR");
		if(op_ == OP_EDIT_DISTANCE) {
			internal = new Xapian::Internal::QueryEditDistance(pattern, max_expansion, flags, combiner);
			return;
		}
		if(UNLIKELY(op_ != OP_WILDCARD))
			throw Xapian::InvalidArgumentError("op must be OP_EDIT_DISTANCE or OP_WILDCARD");
		auto just_flags = flags & ~Query::WILDCARD_LIMIT_MASK_;
		if(pattern.empty()) {
			if(just_flags == 0) {
				// Empty pattern with implicit trailing '*' -> MatchAll.
				internal = new Xapian::Internal::QueryTerm();
			}
			else {
				// Empty pattern with extended wildcards -> MatchNothing.
			}
			return;
		}
		// Check if pattern consists of one or more '*' and at most one '?' (in any
		// order) - if so treat it as just MatchAll.
		bool match_all = false;
		bool question_marks = false;
		for(auto && ch : pattern) {
			if(ch == '*' && (flags & Query::WILDCARD_PATTERN_MULTI)) {
				match_all = true;
			}
			else if(ch == '?' && !question_marks &&
				(flags & Query::WILDCARD_PATTERN_SINGLE)) {
				question_marks = true;
			}
			else {
				match_all = false;
				break;
			}
		}
		if(match_all) {
			internal = new Xapian::Internal::QueryTerm();
			return;
		}
		internal = new Xapian::Internal::QueryWildcard(pattern, max_expansion, flags, combiner);
	}

	Query::Query(op op_, const std::string & pattern, Xapian::termcount max_expansion, int flags, op combiner, unsigned edit_distance, size_t min_prefix_len)
	{
		LOGCALL_CTOR(API, "Query", op_ | pattern | max_expansion | flags | combiner | edit_distance | min_prefix_len);
		if(UNLIKELY(combiner != OP_SYNONYM && combiner != OP_MAX && combiner != OP_OR))
			throw Xapian::InvalidArgumentError("combiner must be OP_SYNONYM or OP_MAX or OP_OR");
		if(UNLIKELY(op_ != OP_EDIT_DISTANCE))
			throw Xapian::InvalidArgumentError("op must be OP_EDIT_DISTANCE");
		internal = new Xapian::Internal::QueryEditDistance(pattern, max_expansion, flags, combiner, edit_distance, min_prefix_len);
	}

	const TermIterator Query::get_terms_begin() const
	{
		if(!internal.get())
			return TermIterator();
		vector <pair<Xapian::termpos, string> > terms;
		internal->gather_terms(static_cast<void*>(&terms));
		sort(terms.begin(), terms.end());
		vector <string> v;
		const string * old_term = NULL;
		Xapian::termpos old_pos = 0;
		for(auto && i : terms) {
			// Remove duplicates (same term at the same position).
			if(old_term && old_pos == i.first && *old_term == i.second)
				continue;

			v.push_back(i.second);
			old_pos = i.first;
			old_term = &(i.second);
		}
		return TermIterator(new VectorTermList(v.begin(), v.end()));
	}

	const TermIterator Query::get_unique_terms_begin() const
	{
		if(!internal.get())
			return TermIterator();
		vector <pair<Xapian::termpos, string> > terms;
		internal->gather_terms(static_cast<void*>(&terms));
		sort(terms.begin(), terms.end(), [](const pair<Xapian::termpos, string>& a, const pair<Xapian::termpos, string>& b) 
		{
			return a.second < b.second;
		});
		vector <string> v;
		const string * old_term = NULL;
		for(auto && i : terms) {
			// Remove duplicate term names.
			if(old_term && *old_term == i.second)
				continue;
			v.push_back(i.second);
			old_term = &(i.second);
		}
		return TermIterator(new VectorTermList(v.begin(), v.end()));
	}

	Xapian::termcount Query::get_length() const noexcept
	{
		return (internal.get() ? internal->get_length() : 0);
	}

	string Query::serialise() const
	{
		string result;
		if(internal.get())
			internal->serialise(result);
		return result;
	}

	const Query Query::unserialise(const string & s, const Registry & reg)
	{
		const char * p = s.data();
		const char * end = p + s.size();
		Query::Internal * q = Query::Internal::unserialise(&p, end, reg);
		AssertEq(p, end);
		return Query(q);
	}

	Xapian::Query::op Query::get_type() const noexcept
	{
		return (!internal.get()) ? Xapian::Query::LEAF_MATCH_NOTHING : internal->get_type();
	}

	size_t Query::get_num_subqueries() const noexcept { return internal.get() ? internal->get_num_subqueries() : 0; }
	const Query Query::get_subquery(size_t n) const { return internal->get_subquery(n); }
	Xapian::termcount Query::get_leaf_wqf() const { return internal->get_wqf(); }
	Xapian::termpos Query::get_leaf_pos() const { return internal->get_pos(); }

	string Query::get_description() const
	{
		string desc = "Query(";
		if(internal.get())
			desc += internal->get_description();
		desc += ")";
		return desc;
	}

	void Query::init(op op_, size_t n_subqueries, Xapian::termcount parameter)
	{
		if(parameter > 0 && op_ != OP_NEAR && op_ != OP_PHRASE && op_ != OP_ELITE_SET)
			throw InvalidArgumentError("parameter only valid with OP_NEAR, OP_PHRASE or OP_ELITE_SET");
		switch(op_) {
			case OP_AND: internal = new Xapian::Internal::QueryAnd(n_subqueries); break;
			case OP_OR: internal = new Xapian::Internal::QueryOr(n_subqueries); break;
			case OP_AND_NOT: internal = new Xapian::Internal::QueryAndNot(n_subqueries); break;
			case OP_XOR: internal = new Xapian::Internal::QueryXor(n_subqueries); break;
			case OP_AND_MAYBE: internal = new Xapian::Internal::QueryAndMaybe(n_subqueries); break;
			case OP_FILTER: internal = new Xapian::Internal::QueryFilter(n_subqueries); break;
			case OP_NEAR: internal = new Xapian::Internal::QueryNear(n_subqueries, parameter); break;
			case OP_PHRASE: internal = new Xapian::Internal::QueryPhrase(n_subqueries, parameter); break;
			case OP_ELITE_SET: internal = new Xapian::Internal::QueryEliteSet(n_subqueries, parameter); break;
			case OP_SYNONYM: internal = new Xapian::Internal::QuerySynonym(n_subqueries); break;
			case OP_MAX: internal = new Xapian::Internal::QueryMax(n_subqueries); break;
			default:
				if(op_ == OP_INVALID && n_subqueries == 0) {
					internal = new Xapian::Internal::QueryInvalid();
					break;
				}
				throw InvalidArgumentError("op not valid with a list of subqueries");
		}
	}

	void Query::add_subquery(bool positional, const Xapian::Query & subquery)
	{
		// We could handle this in a type-safe way, but we'd need to at least
		// declare Xapian::Internal::QueryBranch in the API header, which seems
		// less desirable than a static_cast<> here.
		Xapian::Internal::QueryBranch * branch_query = static_cast<Xapian::Internal::QueryBranch*>(internal.get());
		Assert(branch_query);
		if(positional) {
			switch(subquery.get_type()) {
				case LEAF_TERM:
					break;
				case LEAF_POSTING_SOURCE:
				case LEAF_MATCH_ALL:
				case LEAF_MATCH_NOTHING:
					// None of these have positions, so positional operators won't
					// match.  Add MatchNothing as that is has special handling in
					// AND-like queries to reduce the parent query to MatchNothing,
					// which is appropriate in this case.
					branch_query->add_subquery(MatchNothing);
					return;
				case OP_OR:
					// OP_OR is now handled below OP_NEAR and OP_PHRASE.
					break;
				default:
					throw Xapian::UnimplementedError("OP_NEAR and OP_PHRASE only currently support leaf subqueries");
			}
		}
		branch_query->add_subquery(subquery);
	}

	void Query::done()
	{
		Xapian::Internal::QueryBranch * branch_query = static_cast<Xapian::Internal::QueryBranch*>(internal.get());
		if(branch_query)
			internal = branch_query->done();
	}
}
