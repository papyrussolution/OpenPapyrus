/** @file
 * @brief Wrapper postlist providing positions for an OR
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <xapian-internal.h>
#pragma hdrstop
#include "orpospostlist.h"

using namespace std;

PositionList * OrPosPostList::read_position_list()
{
	return position_list.gather(pl);
}

string OrPosPostList::get_description() const
{
	string desc = "OrPosPostList(";
	desc += pl->get_description();
	desc += ')';
	return desc;
}
