/** @file
 * @brief A document read from a GlassDatabase.
 */
/* Copyright (C) 2008,2009,2010,2011 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_GLASS_DOCUMENT_H
#define XAPIAN_INCLUDED_GLASS_DOCUMENT_H

/// A document read from a GlassDatabase.
class GlassDocument : public Xapian::Document::Internal {
	void operator =(const GlassDocument &); /// Don't allow assignment.
	GlassDocument(const GlassDocument &); /// Don't allow copying.
	const GlassValueManager * value_manager; /// Used for lazy access to document values.
	const GlassDocDataTable * docdata_table; /// Used for lazy access to document data.
	friend class GlassDatabase; /// GlassDatabase::open_document() needs to call our private constructor.
	/// Private constructor - only called by GlassDatabase::open_document().
	GlassDocument(Xapian::Internal::intrusive_ptr<const Xapian::Database::Internal> db, Xapian::docid did_,
	    const GlassValueManager * value_manager_, const GlassDocDataTable * docdata_table_) : 
		Xapian::Document::Internal(db, did_), value_manager(value_manager_), docdata_table(docdata_table_) 
	{
	}
protected:
	/** Implementation of virtual methods @{ */
	string fetch_value(Xapian::valueno slot) const;
	void fetch_all_values(std::map<Xapian::valueno, std::string>& values_) const;
	string fetch_data() const;
	/** @} */
};

#endif // XAPIAN_INCLUDED_GLASS_DOCUMENT_H
