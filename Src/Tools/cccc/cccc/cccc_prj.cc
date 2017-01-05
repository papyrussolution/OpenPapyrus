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
// cccc_prj.cc

// We have some debugging messages specifically for looking at how
// use relationships are being handled.
#define DEBUG_USEREL 0

// implementation file for class CCCC_Project

#include "cccc.h"

#include "cccc_itm.h"
#include "cccc_prj.h"
#include "cccc_db.h"

CCCC_Project::CCCC_Project(const string& name)
{
  // we prime the database with knowledge of the builtin base types
  // we also add a record for the anonymous class which we will treat
  // as the parent of all non-member functions
  char *builtin_type_info[]=
  {
    "void@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    "int@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    "char@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    "long@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    "float@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    "double@builtin@<nofile>@0@builtin definition@d?????@@0@d@",
    NULL
  };
  for(char **ptr=builtin_type_info; *ptr!=NULL; ptr++)
    {
      CCCC_Item type_info(*ptr);
      add_module(type_info);
    }
}


void CCCC_Project::add_module(CCCC_Item& module_line) {
  char linebuf[1024];

  CCCC_Module *module_ptr=new CCCC_Module;
  CCCC_Extent *extent_ptr=new CCCC_Extent;

  if(
     module_line.Extract(module_ptr->module_name) &&
     module_line.Extract(module_ptr->module_type) &&
     extent_ptr->GetFromItem(module_line)
     )
    {
      CCCC_Module *lookup_module_ptr=module_table.find_or_insert(module_ptr);
      if(lookup_module_ptr != NULL) 
	{
	  lookup_module_ptr->extent_table.find_or_insert(extent_ptr);
	
	  if(lookup_module_ptr!=module_ptr) 
	    {
	      // do some work to transfer knowledge from the new module object
	      // then delete it
	      Resolve_Fields(lookup_module_ptr->module_type,module_ptr->module_type);
	      delete module_ptr;
	    }
	}
    }
  else
    {
      cerr << "CCCC_Project::add_module_extent: extraction failed" << endl;
    }
}

void CCCC_Project::add_member(CCCC_Item& member_data_line) 
{
  CCCC_Module *new_module_ptr=new CCCC_Module;
  CCCC_Member *new_member_ptr=new CCCC_Member;
  if(
     member_data_line.Extract(new_module_ptr->module_name) &&  
     member_data_line.Extract(new_member_ptr->member_name) &&
     member_data_line.Extract(new_member_ptr->member_type) &&
     member_data_line.Extract(new_member_ptr->param_list)
     )
    {
      CCCC_Module *found_module_ptr=module_table.find_or_insert(new_module_ptr);
      if(found_module_ptr==new_module_ptr)
	{
	  // protect the new module from deletion at the end of this function
	  new_module_ptr=NULL;
	}

      new_member_ptr->parent=found_module_ptr;
      CCCC_Member *found_member_ptr=member_table.find_or_insert(new_member_ptr);
      if(found_member_ptr==new_member_ptr)
	{
	  new_member_ptr=NULL;
	}
      found_member_ptr->add_extent(member_data_line);
    }
  else
    {
      cerr << "CCCC_Project::add_module extraction failed" << endl;
    }

  // clean up newly allocated records if they have not been accepted
  // into the database
  delete new_module_ptr;
  delete new_member_ptr;
}    

void CCCC_Project::add_userel(CCCC_Item& userel_data_line) {
  CCCC_UseRelationship *new_userel_ptr =
    new CCCC_UseRelationship(userel_data_line);
  CCCC_UseRelationship *lookup_userel_ptr = 
    userel_table.find_or_insert(new_userel_ptr);  
  
  if(lookup_userel_ptr != NULL)
    {
      if(new_userel_ptr != lookup_userel_ptr)
	{
	  delete new_userel_ptr;
	}
      lookup_userel_ptr->add_extent(userel_data_line);
    }
#if DEBUG_USEREL
  cerr << "Adding " << lookup_userel_ptr->client << " uses " 
       << lookup_userel_ptr->supplier << endl;
#endif
}

void CCCC_Project::add_rejected_extent(CCCC_Item& rejected_data_line)
{
  CCCC_Extent *new_extent=new CCCC_Extent(rejected_data_line);
  rejected_extent_table.find_or_insert(new_extent);
}

void CCCC_Project::reindex()
{
  CCCC_Member *member_ptr=member_table.first_item();
  while(member_ptr!=NULL)
    {
      if(member_ptr->parent!=NULL)
	{
	  CCCC_Module::member_map_t::value_type 
	    new_pair(member_ptr->key(),member_ptr);
	  member_ptr->parent->member_map.insert(new_pair);
	}
      else
	{
	  cerr << "Member " << member_ptr->key() << " has no parent" 
	       << endl;
	}

      CCCC_Extent *extent_ptr=member_ptr->extent_table.first_item();
      while(extent_ptr!=NULL)
	{
	  Visibility extent_visibility=extent_ptr->get_visibility();
	  Visibility member_visibility=member_ptr->get_visibility();
	 
	  if(member_ptr->visibility==vDONTKNOW)
	    {
	      member_ptr->visibility=extent_visibility;
	    }
	  else if(
		  (extent_visibility!=vDONTKNOW) &&
		  (member_visibility!=extent_visibility)
		  )
	    {
	      member_ptr->visibility=vINVALID;
	    }

	  extent_ptr=member_ptr->extent_table.next_item();
	}

      member_ptr=member_table.next_item();
    }

  CCCC_UseRelationship *userel_ptr=userel_table.first_item();
  while(userel_ptr!=NULL)
    {
      CCCC_Module *supplier_ptr=new CCCC_Module;
      supplier_ptr->module_name=userel_ptr->supplier;
      CCCC_Module *found_supplier_ptr=
	module_table.find_or_insert(supplier_ptr);
      if(found_supplier_ptr!=supplier_ptr)
	{
	  delete supplier_ptr;
	  supplier_ptr=found_supplier_ptr;
	}

      CCCC_Module *client_ptr=new CCCC_Module;
      client_ptr->module_name=userel_ptr->client;
      CCCC_Module *found_client_ptr=module_table.find_or_insert(client_ptr);
      if(found_client_ptr!=client_ptr)
	{
	  delete client_ptr;
	  client_ptr=found_client_ptr;
	}

      if(
	 (userel_ptr->supplier==userel_ptr->client) ||
	 userel_ptr->supplier=="" ||
	 userel_ptr->client=="" ||
	 supplier_ptr->is_trivial() ||
	 client_ptr->is_trivial() 
	 )
	{
#if DEBUG_USEREL
	  cerr << "Removing relationship between "
	       << userel_ptr->supplier.c_str() 
	       << " and "
	       << userel_ptr->client.c_str()
	       << endl;
#endif
	  userel_table.remove(userel_ptr);
	  delete userel_ptr;
	}
      else
	{
	  // create links from the client and supplier modules to the
	  // relationship object
#if DEBUG_USEREL
	  std::cerr << "Creating links for "
		    << client_ptr->key()
		    << " (" << client_ptr << ") uses " 
		    << supplier_ptr->key()
		    << " (" << supplier_ptr << ")" << std::endl;
#endif

	  CCCC_Module::relationship_map_t::value_type 
	    new_supplier_pair(supplier_ptr->key(), userel_ptr),
	    new_client_pair(client_ptr->key(), userel_ptr);
	  client_ptr->supplier_map.insert(new_supplier_pair);
	  supplier_ptr->client_map.insert(new_client_pair);

	  // calculate the visibility and concreteness of the
	  // relationship
	  AugmentedBool visible=abDONTKNOW;
	  AugmentedBool concrete=abDONTKNOW;

	  CCCC_Extent *extent_ptr=userel_ptr->extent_table.first_item();
	  while(extent_ptr!=NULL)
	    {
	      switch(extent_ptr->get_visibility())
		{
		case vPRIVATE:
		case vIMPLEMENTATION:
		  if(visible!=abTRUE)
		    {
		      visible=abFALSE;
		    }
		  break;
		case vPROTECTED:
		case vPUBLIC:
		  visible=abTRUE;
		  break;
		default:
		  // nothing to do
		  ;
		}

	      switch(extent_ptr->get_usetype())
		{
		case utPARBYREF:
		case utHASBYREF:
		  if(concrete!=abTRUE)
		    {
		      concrete=abFALSE;
		    }
		  break;

		case utINHERITS:
		case utPARBYVAL:
		case utHASBYVAL:
		  concrete=abTRUE;
		  break;

		default:
		  // nothing to do
		  ;
		}
	      
	      extent_ptr=userel_ptr->extent_table.next_item();
	    }
	  userel_ptr->visible=visible;
	  userel_ptr->concrete=concrete;
	}

      userel_ptr=userel_table.next_item();
    }
}


int CCCC_Project::get_count(const char* count_tag) 
{
  int retval=0;
  retval+=module_table.get_count(count_tag);
  retval+=rejected_extent_table.get_count(count_tag);
  return retval;
}


int CCCC_Project::ToFile(ofstream& ofstr)
{
  // this function could be rewritten much more elegantly using
  // STL output iterators, and one day will be ...

  int retval=FALSE;
  CCCC_Module *module_ptr=module_table.first_item();
  while(module_ptr!=NULL)
    {
      module_ptr->ToFile(ofstr);
      module_ptr=module_table.next_item();
    }
   
  CCCC_Member *member_ptr=member_table.first_item();
  while(member_ptr!=NULL)
    {
      member_ptr->ToFile(ofstr);
      member_ptr=member_table.next_item();
    }
  
  CCCC_UseRelationship *userel_ptr=userel_table.first_item();
  while(userel_ptr!=NULL)
    {
      userel_ptr->ToFile(ofstr);
      userel_ptr=userel_table.next_item();
    }

  CCCC_Extent *rejext_ptr=rejected_extent_table.first_item();
  while(rejext_ptr!=NULL)
    {
      CCCC_Item extent_line;
      extent_line.Insert(REJEXT_PREFIX);
      rejext_ptr->AddToItem(extent_line);
      extent_line.ToFile(ofstr);

      rejext_ptr=rejected_extent_table.next_item();
    }

  if(ofstr.good())
    {
      retval=TRUE;
    } 

  return retval;
}


int CCCC_Project::FromFile(ifstream& ifstr)
{
  int retval=FALSE;

  set_active_project(this);
  
  while(PeekAtNextLinePrefix(ifstr,MODULE_PREFIX))
    {
      CCCC_Module *new_module=new CCCC_Module;
      int fromfile_status=new_module->FromFile(ifstr);
      DisposeOfImportRecord(new_module,fromfile_status);
    }

  while(PeekAtNextLinePrefix(ifstr,MEMBER_PREFIX))
    {
      CCCC_Member *new_member=new CCCC_Member;
      int fromfile_status=new_member->FromFile(ifstr);
      DisposeOfImportRecord(new_member,fromfile_status);
    }

  while(PeekAtNextLinePrefix(ifstr,USEREL_PREFIX))
    {
      CCCC_UseRelationship *new_userel=new CCCC_UseRelationship;
      int fromfile_status=new_userel->FromFile(ifstr);
      DisposeOfImportRecord(new_userel,fromfile_status);
    }

  while(PeekAtNextLinePrefix(ifstr,REJEXT_PREFIX))
    {
      CCCC_Extent *new_rejext=new CCCC_Extent;
      CCCC_Item next_line;
      next_line.FromFile(ifstr);
      int fromfile_status=RECORD_ERROR;
      if(
	 new_rejext->GetFromItem(next_line) &&
	 new_rejext==rejected_extent_table.find_or_insert(new_rejext)
	 )
	{
	  fromfile_status=RECORD_ADDED;
	}
      DisposeOfImportRecord(new_rejext,fromfile_status);
    }

  set_active_project(NULL);

  return retval;
}

string CCCC_Project::name(int level) const
{
  return "";
}







