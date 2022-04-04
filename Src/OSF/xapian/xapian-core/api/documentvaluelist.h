/** @file
 * @brief Iteration over values in a document.
 */
/* Copyright (C) 2007,2008,2009,2011,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_DOCUMENTVALUELIST_H
#define XAPIAN_INCLUDED_DOCUMENTVALUELIST_H

//#include "backends/valuelist.h"
//#include "backends/documentinternal.h"
//#include <map>

/// Iteration over values in a document.
class DocumentValueList final : public ValueList {
	void operator = (const DocumentValueList &) = delete; /// Don't allow assignment.
	DocumentValueList(const DocumentValueList &) = delete; /// Don't allow copying.
	/// Document internals we're iterating over.
	Xapian::Internal::intrusive_ptr<const Xapian::Document::Internal> doc;
	/** Iterator over the map inside @a doc.
	 *
	 *  If we haven't started yet, this will be set to: doc->values.end()
	 */
	std::map<Xapian::valueno, std::string>::const_iterator it;
public:
	explicit DocumentValueList(const Xapian::Document::Internal* doc_) : doc(doc_), it(doc->values->end()) 
	{
	}
	Xapian::docid get_docid() const;
	std::string get_value() const;
	Xapian::valueno get_valueno() const;
	bool at_end() const;
	void next();
	/// The parameter is actually a Xapian::valueno for this subclass.
	void skip_to(Xapian::docid slot);
	std::string get_description() const;
};

#endif // XAPIAN_INCLUDED_DOCUMENTVALUELIST_H
