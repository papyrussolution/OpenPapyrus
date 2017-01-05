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
// cccc_mem.cc

// implementation file for class CCCC_Member

#include "cccc.h"

#include "cccc_itm.h"
#include "cccc_mem.h"
#include "cccc_db.h"

CCCC_Member::CCCC_Member()
  : parent(NULL)
{
  visibility=vDONTKNOW;
}

int CCCC_Member::get_count(const char* count_tag) {
  int retval=0;
  string count_tag_str=count_tag;

  if(count_tag_str=="WMC1")
    {
      retval=1;
    }
  else if(count_tag_str=="WMCv")
    {
      switch(get_visibility())
	{
	case vPUBLIC:
	case vPROTECTED:
	  retval=1;
	  break;
	default:
	  NULL;
	}
    }
  else
    {
      retval=extent_table.get_count(count_tag);
    }

  return retval;
}


int CCCC_Member::ToFile(ofstream& ofstr)
{
  int retval=FALSE;

  CCCC_Item member_line;
  member_line.Insert(MEMBER_PREFIX);
  member_line.Insert(parent->key());
  member_line.Insert(member_name);
  member_line.Insert(member_type);
  member_line.Insert(param_list);
  member_line.ToFile(ofstr);

  CCCC_Extent *extent_ptr=extent_table.first_item();
  while(extent_ptr!=NULL)
    {
      CCCC_Item extent_line;
      extent_line.Insert(MEMEXT_PREFIX);
      extent_line.Insert(parent->key());
      extent_line.Insert(member_name);
      extent_line.Insert(member_type);
      extent_line.Insert(param_list);
      extent_ptr->AddToItem(extent_line);
      extent_line.ToFile(ofstr);
     
      extent_ptr=extent_table.next_item();
    }
  
  if(ofstr.good())
    {
      retval=TRUE;
    } 

  return retval;
}

string CCCC_Member::name(int name_level) const
{ 
  string namestr;

  switch(name_level)
    {
    case nlRANK:
    case nlSEARCH:
      // there is no scoping for C-style functions ...
      if(parent==NULL)
	{
          namestr.append("<NULL>::");
        }
      else if( 
	      (parent->name(nlMODULE_NAME)!="") && 
	      (parent->name(nlMODULE_TYPE)!="file") 
	      )
	{
	  namestr.append(parent->name(nlMODULE_NAME));
          namestr.append("::");
	}
      namestr.append(member_name);
      namestr.append(param_list);
      break;

    case nlMEMBER_NAME:
    case nlSIMPLE:
      namestr=member_name;
      break;
    
    case nlMEMBER_TYPE:
      namestr=member_type; 
      break;

    case nlMEMBER_PARAMS:
      namestr=param_list;
      break;
    case nlLOCAL:
      namestr.append(member_name);
      namestr.append(param_list);
      break;
	
    default:
      cerr << "unexpected name level" << endl;
    }

  return namestr.c_str();
}	

int CCCC_Member::FromFile(ifstream& ifstr)
{
  int retval=RECORD_ERROR;
  enum MemberFromFileStatuses { MEMBER_RECORD_NO_PARENT_FOUND=3 };

  CCCC_Item next_line;
  next_line.FromFile(ifstr);
  ifstr_line++;
  
  string line_keyword_dummy;
  string parent_name;

  CCCC_Member *found_mptr=NULL;

  if(
     next_line.Extract(line_keyword_dummy) &&
     next_line.Extract(parent_name) &&
     next_line.Extract(this->member_name) &&
     next_line.Extract(this->member_type) &&
     next_line.Extract(this->param_list) 
     ) 
    {
      parent=current_loading_project->module_table.find(parent_name);
      if(parent!=NULL)
	{
	  found_mptr=
	    current_loading_project->member_table.find_or_insert(this);
	  if(found_mptr==this)
	    {
	      // the newly created instance of the module is the first
	      // and has taken its place in the database, so we protect
	      // it from deletion
	      retval=RECORD_ADDED;
	    }
	  else
	    {
	      retval=RECORD_TRANSCRIBED;
	    }
 
	  // process extent records
	  while(PeekAtNextLinePrefix(ifstr,MEMEXT_PREFIX))
	    {
	      CCCC_Extent *new_extent=new CCCC_Extent;
	      next_line.FromFile(ifstr);
	      ifstr_line++;
	      string parent_key_dummy, member_name_dummy, 
		member_type_dummy, param_list_dummy;

	      if(
		 next_line.Extract(line_keyword_dummy) &&
		 next_line.Extract(parent_key_dummy) &&
		 next_line.Extract(member_name_dummy) &&
		 next_line.Extract(member_type_dummy) &&
		 next_line.Extract(param_list_dummy) &&
		 new_extent->GetFromItem(next_line)
		 )
		{
		  // We don't ever expect to find duplicated extent records
		  // but just in case... 
		  CCCC_Extent *found_eptr=
		    found_mptr->extent_table.find_or_insert(new_extent);
		  if(found_eptr!=new_extent)
		    {
		      cerr << "Failed to add extent for member "
			   << found_mptr->key() << " at line " << ifstr_line 
			   << endl;
		      delete new_extent;
		    }
		}
	    }

	}
      else // parent record not found
	{
	  retval=MEMBER_RECORD_NO_PARENT_FOUND;
	}
    } 
  else // extraction of module intial line failed
    {
      // unexpected problem with the input
      retval=RECORD_ERROR;
    }

  // If the import was successful, we will also have imported all dependent
  // extent records following the main record.
  // If not, we must skip them.
  while(PeekAtNextLinePrefix(ifstr,MEMEXT_PREFIX))
    {
      CCCC_Item next_line;
      next_line.FromFile(ifstr);
      ifstr_line++;
      cerr << "Ignoring member extent on line " << ifstr_line << endl;
    }
 
  return retval;
}

Visibility CCCC_Member::get_visibility()
{
  return visibility;
}



