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
 * cccc_ext.cc
 */

#include "cccc_itm.h"
#include "cccc_ext.h"
#include "cccc_db.h"
#include "cccc_utl.h"

unsigned int CCCC_Extent::nextkey=0;

CCCC_Extent::CCCC_Extent()
{
  v=vINVALID;
  ut=utINVALID;
  extkey=++nextkey;
}

CCCC_Extent::CCCC_Extent(CCCC_Item& is) 
{
  char v_as_char='!', ut_as_char='!';
 
  if(
     is.Extract(filename) &&
     is.Extract(linenumber) &&
     is.Extract(description) &&
     is.Extract(flags) &&
     is.Extract(count_buffer) &&
     is.Extract(v_as_char) &&
     is.Extract(ut_as_char)
     ) 
    {
      v=(Visibility) v_as_char;
      ut=(UseType) ut_as_char;
    }
  else
    {
      // we can trust the string constructor to give us empty strings,
      // but we need to initialise these
      v=vDONTKNOW;
      ut=utDONTKNOW;
    }
  extkey=++nextkey;
}

int CCCC_Extent::AddToItem(CCCC_Item& item)
{
  int retval=FALSE;
  
  if(
     item.Insert(filename) &&
     item.Insert(linenumber) &&
     item.Insert(description) &&
     item.Insert(flags) &&
     item.Insert(count_buffer) &&
     item.Insert((char) v) &&
     item.Insert((char) ut)
     )
    {
      retval=TRUE;
    }

  return retval;
}

int CCCC_Extent::GetFromItem(CCCC_Item &item)
{
  int retval=FALSE;
  char v_as_char, ut_as_char;
  if(
     item.Extract(filename) &&
     item.Extract(linenumber) &&
     item.Extract(description) &&
     item.Extract(flags) &&
     item.Extract(count_buffer) &&
     item.Extract(v_as_char) &&
     item.Extract(ut_as_char)
     )
    {
      v = (Visibility) v_as_char;
      ut = (UseType) ut_as_char;
      retval=TRUE;
    }

  return retval;
}


string CCCC_Extent::name(int level) const
{
  string rtnbuf;

  rtnbuf="";

  switch(level)
    {
    case nlFILENAME:
      rtnbuf=filename;
      break;
    case nlLINENUMBER:
      rtnbuf=linenumber;
      break;
    case nlDESCRIPTION:
      rtnbuf=description;
      break;
    case nlSEARCH:
    case nlRANK:
      // Extents have no meaningful internal primary key.
      // We never want two extents to have the same
      // key, so we use the running number extkey
      // which is initialized in both constructors.
      // This should cause extents to sort in order of
      // their creation, which is fine.
      char buf[16];
      sprintf(buf,"%015d",extkey);
      rtnbuf=buf;
      break;

    default:
      rtnbuf+=filename;
      rtnbuf+=":";
      rtnbuf+=linenumber;
    }
  return rtnbuf.c_str();
}

string CCCC_Extent::key() const { return name(nlRANK); }

int CCCC_Extent::get_count(const char* count_tag) {
  int retval=0;
  char local_count_buffer[100], *count_tag_ptr, *count_value_ptr;
  strcpy(local_count_buffer,count_buffer.c_str());
  count_tag_ptr=strtok(local_count_buffer,":");
  while(count_tag_ptr!=NULL)
    {
      count_value_ptr=strtok(NULL," ");
      if(strcmp(count_tag_ptr, count_tag) ==0)
	{
	  retval+=atoi(count_value_ptr);
	}
      count_tag_ptr=strtok(NULL,":");
    }
  return retval;
}












