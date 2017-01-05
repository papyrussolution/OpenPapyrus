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
 * cccc_use.h
 */
#ifndef CCCC_USE_H
#define CCCC_USE_H

#include "cccc_rec.h"

class CCCC_Module;

static const string USEREL_PREFIX="CCCC_UseRel";
static const string USEEXT_PREFIX="CCCC_UseExt";

enum UserelNameLevel { nlSUPPLIER=-1, nlCLIENT=-2, nlMEMBER=-3 };

class CCCC_UseRelationship : public CCCC_Record 
{
  friend class CCCC_Project;
  string supplier, client, member;
  UseType ut;
  AugmentedBool visible, concrete;
  CCCC_UseRelationship() { ut=utDONTKNOW; }

 public:
  string name( int index ) const;
  CCCC_UseRelationship(CCCC_Item& is);
  int FromFile(ifstream& infile);
  int ToFile(ofstream& outfile);
  void add_extent(CCCC_Item&);
  int get_count(const char *count_tag);
  UseType get_usetype() const { return ut; }
  AugmentedBool is_visible () const { return visible; }
  AugmentedBool is_concrete () const { return concrete; }
  void generate_report(ostream& os);
  CCCC_Module* supplier_module_ptr(CCCC_Project *prj);
  CCCC_Module* client_module_ptr(CCCC_Project *prj);
};


#endif // CCCC_USE_H






