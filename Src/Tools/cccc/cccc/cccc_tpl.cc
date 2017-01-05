/*
    CCCC - C and C++ Code Counter
    Copyright (C) 1994-2005 Tim Littlefair (tim_littlefair@hotmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/
// cccc_tpl.cc

// all explicit template instantiations for the project are collected here

// the assumption is that this file will be stable, so the expensive
// recompilation of the templates will be infrequent

#include "cccc_itm.h"
#include "cccc_tbl.h"
#include "cccc_db.h"
#include "cccc_htm.h"
#include "cccc_met.h"

#include "cccc_tbl.cc"

template class std::map<string,Source_Anchor>;
template class CCCC_Table<CCCC_Extent>;
template class CCCC_Table<CCCC_Module>;
template class CCCC_Table<CCCC_UseRelationship>;
template class CCCC_Table<CCCC_Member>;
