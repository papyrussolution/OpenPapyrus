/** @file
 * @brief Wrapper postlist providing positions for an OR
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_ORPOSPOSTLIST_H
#define XAPIAN_INCLUDED_ORPOSPOSTLIST_H

/** Wrapper postlist providing positions for an OR. */
class OrPosPostList : public WrapperPostList {
	OrPositionList position_list;
public:
	OrPosPostList(PostList * pl_) : WrapperPostList(pl_) 
	{
	}
	PositionList * read_position_list();
	std::string get_description() const;
};

#endif // XAPIAN_INCLUDED_ORPOSPOSTLIST_H
