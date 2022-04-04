/** @file
 * @brief Merge two PositionList objects using an OR operation.
 */
/* Copyright (C) 2007,2010,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_ORPOSITIONLIST_H
#define XAPIAN_INCLUDED_ORPOSITIONLIST_H

class OrPositionList : public PositionList {
	std::vector <PositionList*> pls; /// The PositionList sub-objects.
	/** Current positions of the subobjects.
	 *
	 *  This will be empty when this position list hasn't yet started.
	 */
	std::vector <Xapian::termpos> current;
	Xapian::termpos current_pos; /// Current position of this object.
public:
	OrPositionList() 
	{
	}
	PositionList* gather(PostList * pl) 
	{
		pls.clear();
		current.clear();
		pl->gather_position_lists(this);
		if(pls.size() == 1)
			return pls[0];
		return this;
	}
	void add_poslist(PositionList* poslist) 
	{
		pls.push_back(poslist);
	}
	Xapian::termcount get_approx_size() const;
	Xapian::termpos back() const;
	Xapian::termpos get_position() const;
	bool next();
	bool skip_to(Xapian::termpos termpos);
};

#endif // XAPIAN_INCLUDED_ORPOSITIONLIST_H
