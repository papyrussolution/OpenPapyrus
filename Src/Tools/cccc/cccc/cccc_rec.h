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
 * cccc_rec.h
 * 
 * defines the database used by CCCC to generate a report
 */
#ifndef CCCC_REC_H
#define CCCC_REC_H

#include "cccc_tbl.h"
#include "cccc_ext.h"
#include "cccc_utl.h"

// The entities held within the database need to be able to return a variety 
// of kinds of name including a simple name (typically one word), a fully 
// qualified local name (i.e as used within a class), and a fully 
// qualified global name.
// Subclasses may also have particular other names, which should be defined
// using negative indexes.
enum NameLevel { nlRANK, nlSEARCH, nlSIMPLE, nlLOCAL, nlGLOBAL };
class CCCC_Record 
{
  friend class CCCC_Html_Stream;
  friend class CCCC_Xml_Stream;
  static CCCC_Project *active_project;
 protected:
  typedef CCCC_Table<CCCC_Extent> Extent_Table;
  Extent_Table extent_table;
  string flags;
  virtual void merge_flags(string& new_flags);
 public:
  virtual ~CCCC_Record() {}
  virtual string name(int level) const;
  virtual string key() const;
  AugmentedBool get_flag(PSFlag psf) { return (AugmentedBool) flags[psf]; }

  virtual void add_extent(CCCC_Item&);
  virtual void sort() { extent_table.sort(); }
  virtual int get_count(const char *count_tag)=0;
  friend int rank_by_string(const void *p1, const void *p2);
  static CCCC_Project* get_active_project();
  static void set_active_project(CCCC_Project* prj);
};



#endif // CCCC_REC_H






