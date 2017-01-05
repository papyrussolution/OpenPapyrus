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
 * cccc_mod.h
 */
#ifndef CCCC_MOD_H
#define CCCC_MOD_H

#include "cccc.h"

#include "cccc_rec.h"

class CCCC_Item;
class CCCC_Project;
class CCCC_UseRelationship;
class CCCC_Member;


static const string MODULE_PREFIX="CCCC_Module";
static const string MODEXT_PREFIX="CCCC_ModExt";


enum ModuleNameLevel 
{ nlMODULE_TYPE=-1, nlMODULE_NAME=-2, nlMODULE_TYPE_AND_NAME=-3 };

class CCCC_Module : public CCCC_Record 
{
  friend class CCCC_Project;
  friend class CCCC_Html_Stream;
  friend class CCCC_Xml_Stream;
  CCCC_Project *project;
  string module_name, module_type;

  typedef std::map<string,CCCC_Member*> member_map_t;
  member_map_t member_map;

  typedef std::map<string,CCCC_UseRelationship*> relationship_map_t;
  relationship_map_t client_map;
  relationship_map_t supplier_map;

  CCCC_Module();
  
public:

  string name(int name_level) const; 
    
  int FromFile(ifstream& infile);
  int ToFile(ofstream& outfile);
  
  virtual int get_count(const char *count_tag);
  int is_trivial();
};

#endif // CCCC_MOD_H






