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
// cccc_mod.cc

// implementation file for CCCC_Module class

#include "cccc.h"

#include "cccc_itm.h"
#include "cccc_mod.h"
#include "cccc_db.h"

CCCC_Module::CCCC_Module()
{
  project=get_active_project();
}


string CCCC_Module::name(int name_level) const
{ 
  string retval;

  switch(name_level)
    {
    case nlMODULE_TYPE: 
      retval=module_type;
      break;

    case nlMODULE_NAME:
      retval=module_name;
      break;

    case nlMODULE_TYPE_AND_NAME: 
      retval=module_type;
      if(retval.size()>0)
	{
	  retval=retval+" ";
	}
      retval=retval+module_name;
      break;

    default: 
      retval=module_name;
    }
  return retval.c_str();
}

int CCCC_Module::get_count(const char* count_tag)
{
  int retval=0;
  if(strcmp(count_tag,"NOM")==0)
    {
      if(is_trivial()==FALSE)
	{
	  retval=1;
	}
    }
  else if(strcmp(count_tag,"CBO")==0)
    {
      retval=client_map.size()+supplier_map.size();
    }
  else if(strcmp(count_tag,"NOC")==0)
    {
      retval=0;

      relationship_map_t::iterator iter;
      iter=client_map.begin();
      while(iter!=client_map.end())
	{
	  if((*iter).second->get_usetype()==utINHERITS)
	    {
	      retval++;
	    }
	  iter++;
	}
    }
  else if(strcmp(count_tag,"DIT")==0)
    {
      retval=0;

      // cyclical inheritance relationships in code would
      // never compile, but this is no excuse for us allowing them
      // to cause us to overflow the stack
      static int recursion_depth=0;
      recursion_depth++;
      if(recursion_depth>100)
	{
	  cerr << "Recursion overflow attempting to calculate DIT for "
	       << key() << endl;
	  retval=1000;
	}
      else
	{
	  relationship_map_t::iterator iter;
	  iter=supplier_map.begin();
	  while(iter!=supplier_map.end())
	    {
	      if((*iter).second->get_usetype()==utINHERITS)
		{
		  int parent_depth=
		    (*iter).second->supplier_module_ptr(project)->get_count("DIT");
		  if(retval<parent_depth+1)
		    {
		      retval=parent_depth+1;
		    }
		}
	      iter++;
	    }
	}
      recursion_depth--;
    }
  else if(strncmp(count_tag,"FI",2)==0)
    {
      relationship_map_t::iterator iter;
      iter=supplier_map.begin();
      while(iter!=supplier_map.end())
	{
	  retval+=(*iter).second->get_count(count_tag);
	  iter++;
	}
    }
  else if(strncmp(count_tag,"FO",2)==0)
    {
      relationship_map_t::iterator iter;
      iter=client_map.begin();
      while(iter!=client_map.end())
	{
	  retval+=(*iter).second->get_count(count_tag);
	  iter++;
	}
    }
  else if(strncmp(count_tag,"IF4",3)==0)
    {
      char if4_suffix=count_tag[3];
      string fi_variant="FI", fo_variant="FO";
      if(if4_suffix!=0)
	{
	  fi_variant+=if4_suffix;
	  fo_variant+=if4_suffix;
	}
      retval=get_count(fi_variant.c_str())*get_count(fo_variant.c_str());
	  retval*=retval;
    }
  else
    {
      CCCC_Extent *extPtr=extent_table.first_item();
      while(extPtr!=NULL)
	{
	  int extent_count=extPtr->get_count(count_tag);
	  retval+=extent_count;
	  extPtr=extent_table.next_item();
	}

      member_map_t::iterator memIter=member_map.begin();
      while(memIter!=member_map.end())
	{
	  int member_count=(*memIter).second->get_count(count_tag);
	  retval+=member_count;
	  memIter++;
	}
    }
  return retval;
}

int CCCC_Module::is_trivial()
{
  int retval=FALSE;

  if(
     (module_type=="builtin") ||
     (module_type=="enum") ||
     (module_type=="struct") ||
     (module_type=="trivial") 
     )
    {
      retval=TRUE;
    }

  return retval;
}

int CCCC_Module::ToFile(ofstream& ofstr)
{
  int retval=FALSE;
  CCCC_Item module_line;
  module_line.Insert(MODULE_PREFIX);
  module_line.Insert(module_name);
  module_line.Insert(module_type);
  module_line.ToFile(ofstr);

  CCCC_Extent *extent_ptr=extent_table.first_item();
  while(extent_ptr!=NULL)
    {
      CCCC_Item extent_line;
      extent_line.Insert(MODEXT_PREFIX);
      extent_line.Insert(module_name);
      extent_line.Insert(module_type);
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

int CCCC_Module::FromFile(ifstream& ifstr)
{
  int retval=RECORD_ERROR;

  CCCC_Item next_line;
  next_line.FromFile(ifstr);
  ifstr_line++;
  
  string line_keyword_dummy;

  CCCC_Module *found_mptr=NULL;

  if(
     next_line.Extract(line_keyword_dummy) &&
     next_line.Extract(this->module_name) &&
     next_line.Extract(this->module_type) 
     ) 
    {
      found_mptr=
	current_loading_project->module_table.find_or_insert(this);
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
      while(PeekAtNextLinePrefix(ifstr,MODEXT_PREFIX))
	{
	  CCCC_Extent *new_extent=new CCCC_Extent;
	  next_line.FromFile(ifstr);
	  ifstr_line++;
	  string module_name_dummy, module_type_dummy;

	  if(
	     next_line.Extract(line_keyword_dummy) &&
	     next_line.Extract(module_name_dummy) &&
	     next_line.Extract(module_type_dummy) &&
	     new_extent->GetFromItem(next_line)
	     )
	    {
	      // We don't ever expect to find duplicated extent records
	      // but just in case... 
	      CCCC_Extent *found_eptr=
		found_mptr->extent_table.find_or_insert(new_extent);
	      if(found_eptr!=new_extent)
       		{
		  cerr << "Failed to add extent for module "
		       << found_mptr->key() << " at line " << ifstr_line 
		       << endl;
		  delete new_extent;
		}
	    }
	}
    }
  else
    {
      // unexpected problem with the input
      retval=RECORD_ERROR;
    }
  
  // If the import was successful, we will also have imported all dependent
  // extent records following the main record.
  // If not, we must skip them.
  while(PeekAtNextLinePrefix(ifstr,MODEXT_PREFIX))
    {
      CCCC_Item next_line;
      next_line.FromFile(ifstr);
      ifstr_line++;
      cerr << "Ignoring member extent on line " << ifstr_line << endl;
    }

  return retval;
}
