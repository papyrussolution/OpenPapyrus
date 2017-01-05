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
// cccc_tbl.cc
#ifndef _CCCC_TBL_BODY
#define _CCCC_TBL_BODY

#include "cccc_itm.h"
#include "cccc_tbl.h"
#include <cassert>

#define LINE_BUFFER_SIZE 1000


template <class T> CCCC_Table<T>::CCCC_Table() 
: sorted(true)
{
  iter_ = map_t::end(); 
}

template <class T> CCCC_Table<T>::~CCCC_Table() 
{
  // the container should manage the destruction of its own
  // nodes correctly, we just need to get rid of the 
  // objects to which we hold pointers.
  // NB Although CCCC_Table holds pointers, it owns the 
  // objects they point to and is responsible for their disposal.
  T* itemptr=first_item();
  while(itemptr!=NULL)
    {
      delete itemptr;
      itemptr=next_item();
    }
}

template<class T> 
int CCCC_Table<T>::get_count(const char* count_tag) 
{
  int retval=0;
  T* itemptr=first_item();
  while(itemptr!=NULL)
    {
      retval+=itemptr->get_count(count_tag);
      itemptr=next_item();
    }
  
  return retval;
}

template<class T> 
T* CCCC_Table<T>::find(string name)
{
  T *retval=NULL;
  typename map_t::iterator value_iterator=map_t::find(name);
  if(value_iterator!=map_t::end())
    {
      retval=(*value_iterator).second;
    }
  return retval;
}

template<class T> 
T* CCCC_Table<T>::find_or_insert(T* new_item_ptr)
{
  string new_key=new_item_ptr->key();
  T *retval=find(new_key);
  if(retval==NULL)
    {
      typename map_t::value_type new_pair(new_key,new_item_ptr);
      map_t::insert(new_pair);
      sorted=false;
      retval=new_item_ptr;
    }
  return retval;
}

template<class T>
bool CCCC_Table<T>::remove(T* old_item_ptr)
{
  bool retval=false; 
  typename map_t::iterator value_iterator=map_t::find(old_item_ptr->key());
  if(value_iterator!=map_t::end())
    {
      erase(value_iterator);
      retval=true;
    }
  return retval;
}
   
template <class T> void CCCC_Table<T>::sort() 
{
  if(sorted==false)
    {
      sorted=true;
    }
}

template <class T> void CCCC_Table<T>::reset_iterator()
{
  iter_=map_t::begin();
}

template <class T> T* CCCC_Table<T>::first_item()
{
  reset_iterator();
  return next_item();
}

template <class T> T* CCCC_Table<T>::next_item()
{
  T* retval=NULL;
  if(iter_!=map_t::end())
    {
      retval=(*iter_).second;
      iter_++;
    }
  return retval;
}

template <class T> int CCCC_Table<T>::records()
{ 
  return map_t::size(); 
}

#endif // _CCCC_TBL_BODY











