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
/*
 * cccc_mem.h
 */
#ifndef CCCC_MEM_H
#define CCCC_MEM_H

#include "cccc_rec.h"

static const string MEMBER_PREFIX="CCCC_Member";
static const string MEMEXT_PREFIX="CCCC_MemExt";

enum MemberNameLevel { nlMEMBER_NAME=-1, nlMEMBER_TYPE=-2, nlMEMBER_PARAMS=-3 };

class CCCC_Module;

class CCCC_Member : public CCCC_Record 
{
  friend class CCCC_Project;
  friend class CCCC_Module;
  string member_type, member_name, param_list;
  Visibility visibility;
  CCCC_Module *parent;
  CCCC_Member(); 
 public:
  string name( int index ) const;
  CCCC_Member(CCCC_Item& member_data_line, CCCC_Module* parent_ptr=NULL);
  int FromFile(ifstream& infile);
  int ToFile(ofstream& outfile);
  void generate_report(ostream&); 
  int get_count(const char *count_tag);
  Visibility get_visibility();
};

#endif // CCCC_MEM_H






