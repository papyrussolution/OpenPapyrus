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
// cccc_use.cc

// implementation of CCCC_UseRelationship class

#include "cccc.h"

#include "cccc_itm.h"
#include "cccc_use.h"
#include "cccc_db.h"


CCCC_UseRelationship::CCCC_UseRelationship(CCCC_Item& is) 
{
  is.Extract(client);
  is.Extract(member);
  is.Extract(supplier);
  visible=abDONTKNOW;
  concrete=abDONTKNOW;
  ut=utDONTKNOW;
}

string CCCC_UseRelationship::name(int name_level) const
{ 
  string namestr;

  switch(name_level)
    {
    case nlRANK:
    case nlSIMPLE:
      namestr.append(client);
      namestr.append(" uses ");
      namestr.append(supplier);
      break;
      
    case nlSUPPLIER:
      namestr=supplier;
      break;

    case nlCLIENT:
      namestr=client;
      break;

    default:
      cerr << "unexpected name level" << endl;
    }

  return namestr.c_str();
}	

void CCCC_UseRelationship::add_extent(CCCC_Item& is)
{
  // processing is similar to the CCCC_Record method, except that we update
  // the visibility and concreteness data members
  // but do not do merge_flags
  CCCC_Extent *new_extent=new CCCC_Extent(is);
  CCCC_Extent *inserted_extent=extent_table.find_or_insert(new_extent);

  switch(new_extent->get_visibility())
    {
    case vPUBLIC:
    case vPROTECTED:
      visible=abTRUE;
      break;
    case vPRIVATE:
    case vIMPLEMENTATION:
      visible=abFALSE;
      break;

    default:
      // nothing required
      ;;
    }

  // a single relationship record represents all connections between two
  // modules, hence it may have multiple extents which are of different use
  // types
  // the use type attached to the relationship record is used only to identify
  // inheritance relationships
  UseType new_ut=new_extent->get_usetype();
  if(new_ut==utINHERITS)
    {
      ut=utINHERITS;
    }

  switch(new_ut)
    {
    case utINHERITS:
    case utHASBYVAL:
    case utPARBYVAL:
    case utVARBYVAL:
      concrete=abTRUE;
      break;
    default:
      // no change required
      ;;
    }

  if(new_extent != inserted_extent)
    {
      delete new_extent;
    }
}

int CCCC_UseRelationship::get_count(const char* count_tag) 
{
  int retval=0;

  if( (strncmp(count_tag,"FI",2)==0) || (strncmp(count_tag,"FO",2)==0) )
    {
      char suffix=count_tag[2];
      switch(suffix)
	{
	case 0:
	  retval=1;
	  break;

	case 'v':
	  if(visible!=abFALSE)
	    {
	      retval=1;
	    }
	  break;

	case 'c':
	  if(concrete!=abFALSE)
	    { 
	      retval=1;
	    }
	  break;

	default:
	  cerr << "Unexpected count tag suffix" << count_tag << endl;
	}
    }
  else
    {
      cerr << "Unexpected count tag " << count_tag << endl;
    }

	    
  return retval;
}


  
CCCC_Module* CCCC_UseRelationship::supplier_module_ptr(CCCC_Project *prj)
{
  return prj->module_table.find(supplier.c_str());
}

CCCC_Module* CCCC_UseRelationship::client_module_ptr(CCCC_Project *prj)
{
  return prj->module_table.find(client.c_str());
}


int CCCC_UseRelationship::ToFile(ofstream& ofstr)
{
  int retval=FALSE;

  CCCC_Item line;
  line.Insert(USEREL_PREFIX);
  line.Insert(supplier);
  line.Insert(client);
  line.ToFile(ofstr);

  CCCC_Extent *extent_ptr=extent_table.first_item();
  while(extent_ptr!=NULL)
    {
      CCCC_Item extent_line;
      extent_line.Insert(USEEXT_PREFIX);
      extent_line.Insert(supplier);
      extent_line.Insert(client);
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

int CCCC_UseRelationship::FromFile(ifstream& ifstr)
{
  int retval;
  CCCC_Item next_line;
  next_line.FromFile(ifstr);
  ifstr_line++;
  
  string line_keyword_dummy;

  CCCC_UseRelationship *found_uptr=NULL;

  if(
     next_line.Extract(line_keyword_dummy) &&
     next_line.Extract(this->supplier) &&
     next_line.Extract(this->client) 
     ) 
    {
      found_uptr=
	current_loading_project->userel_table.find_or_insert(this);
      if(found_uptr==this)
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
      while(PeekAtNextLinePrefix(ifstr,USEEXT_PREFIX))
	{
	  CCCC_Extent *new_extent=new CCCC_Extent;
	  next_line.FromFile(ifstr);
	  ifstr_line++;
	  string supplier_dummy, client_dummy;

	  if(
	     next_line.Extract(line_keyword_dummy) &&
	     next_line.Extract(supplier_dummy) &&
	     next_line.Extract(client_dummy) &&
	     new_extent->GetFromItem(next_line)
	     )
	    {
	      // We don't ever expect to find duplicated extent records
	      // but just in case... 
	      CCCC_Extent *found_eptr=
		found_uptr->extent_table.find_or_insert(new_extent);
	      if(found_eptr!=new_extent)
		{
		  cerr << "Failed to add extent for relationship "
		       << found_uptr->key() << " at line " << ifstr_line 
		       << endl;
		  delete new_extent;
		}
	    }
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
  while(PeekAtNextLinePrefix(ifstr,USEEXT_PREFIX))
    {
      CCCC_Item next_line;
      next_line.FromFile(ifstr);
      ifstr_line++;
      cerr << "Ignoring userel extent on line " << ifstr_line << endl;
    }
 
  return retval;
}




