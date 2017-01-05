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
#include "cccc.h"
#include <fstream>

#include "cccc_itm.h"
#include "cccc_db.h"

CCCC_Project *current_loading_project=NULL;
CCCC_Module  *current_loading_module=NULL;
CCCC_Member  *current_loading_extent=NULL;
CCCC_UseRelationship *current_loading_userel=NULL;

#define LINE_BUFFER_SIZE  1000

int ifstr_line;

extern CCCC_Project *prj;

// the file scope variable last_supplier is used to supress repeated
// output of the supplier name in the use relationship section where
// the current record has the same supplier as the previous one
// the indentation makes this reasonably clear
static string last_supplier="";

// persistence facilities

#define SEP '@'

// This function provides the ability for the persistence functions
// defined below to do a quick peek at the first token on the stream
// leaving the get pointer at the start of that token. 
// This should be static, but on MSVC++ this gives me an unresolved 
// symbol at link.
bool PeekAtNextLinePrefix(ifstream& ifstr, string pfx)
{
  bool retval=false;
  char prefix_buffer[1024];
  size_t initial_stream_pos=ifstr.tellg();
  ifstr.getline(prefix_buffer,1023,SEP);
  if(pfx==prefix_buffer)
    {
      retval=true;
    }
  ifstr.seekg(initial_stream_pos);
  return retval;
}

// this is a sort of abstract junkyard function (cf Abstract Factory)
template <class T> void DisposeOfImportRecord(T *record_ptr, int fromfile_status)
{
  switch(fromfile_status)
    {
    case RECORD_ADDED:
      // the newly created object has been added to the
      // database
      // we must not delete it
      break;

    case RECORD_TRANSCRIBED:
      // the database already had an object for this item
      // the content of the new object was merged in, but the object
      // itself is no longer required
      delete record_ptr;
      break;

    default:
      // something went wrong, so we mention it
      cerr << "Import error " << fromfile_status 
	   << " at line " << ifstr_line 
	   << " for " << record_ptr->key()
	   << endl;
      delete record_ptr;
    }
}

// when we add a record to, for example, the extent table for a member of
// a module, we need to merge the information in the new extent
// with what is already known
// there are two kinds of merge:
// 1. ordinary fields like module_type should either be consistent or 
// blank for all extents relating to the same module, so where the old 
// field is blank, we overwrite with the new field
// 2. the flags field in CCCC_Member contains a variety of single character
// flags giving the visibility, constness, etc. of the member, with '?' being
// used to reflect a state of lack of knowledge: in these cases, any other
// value can overwrite '?', all other values do not change. 

void Resolve_Fields(string& field1, string& field2)
{
  if(field1.size()==0)
    {
      field1=field2;
    }
}

template 
void 
DisposeOfImportRecord(CCCC_Module *record_ptr, int fromfile_status); 

template 
void 
DisposeOfImportRecord(CCCC_Member *record_ptr, int fromfile_status);

template 
void 
DisposeOfImportRecord(CCCC_UseRelationship *record_ptr, int fromfile_status);

template 
void 
DisposeOfImportRecord(CCCC_Extent *record_ptr, int fromfile_status);


