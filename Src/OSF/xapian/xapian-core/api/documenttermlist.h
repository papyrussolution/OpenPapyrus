/** @file
 * @brief Iteration over terms in a document
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_DOCUMENTTERMLIST_H
#define XAPIAN_INCLUDED_DOCUMENTTERMLIST_H

/// Iteration over terms in a document.
class DocumentTermList final : public TermList {
	void operator = (const DocumentTermList&) = delete; /// Don't allow assignment.
	DocumentTermList(const DocumentTermList&) = delete; /// Don't allow copying.
	/// Document internals we're iterating over.
	Xapian::Internal::intrusive_ptr<const Xapian::Document::Internal> doc;
	/** Iterator over the map inside @a doc.
	 *
	 *  If we haven't started yet, this will be set to: doc->terms.end()
	 */
	std::map<std::string, TermInfo>::const_iterator it;
public:
	explicit DocumentTermList(const Xapian::Document::Internal* doc_)
		: doc(doc_), it(doc->terms->end()) {
	}
	Xapian::termcount get_approx_size() const;
	std::string get_termname() const;
	Xapian::termcount get_wdf() const;
	Xapian::doccount get_termfreq() const;
	const Xapian::VecCOW<Xapian::termpos> * get_vec_termpos() const;
	PositionList* positionlist_begin() const;
	Xapian::termcount positionlist_count() const;
	TermList * next();
	TermList * skip_to(const std::string & term);
	bool at_end() const;
};

#endif // XAPIAN_INCLUDED_DOCUMENTTERMLIST_H
