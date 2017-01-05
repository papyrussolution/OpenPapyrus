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
// cccc_opt.h
#ifndef _CCCC_OPT_H
#define _CCCC_OPT_H

// This file defines the object which holds the major configuration
// options for the CCCC program including:
//  - the default language associated with each file name extension; 
//  - the treatment of specific values of each metric; and
//  - the application of dialect specific parsing rule (e.g. rule to 
//    ignore MSVC++-specific pseudo-keywords when parsing the 
//    MS C++ dialect.

// This is a natural singleton class, hence all member functions are static
// and all data will be declared with static file scope in the implementation
// file.

#include "cccc.h"
#include "cccc_itm.h"

class Metric_Treatment;

class CCCC_Options
{
 public:
  // initialise using a file
  static void Load_Options(const string& filename);

  // initialise using hard-coded defaults
  static void Load_Options();

  // save the current set of options to a file
  static void Save_Options(const string& filename);

  // add a new option into the current option set
  static void Add_Option(CCCC_Item& option_line);

  // map a filename to a language
  static string getFileLanguage(const string& filename);
  
  // map a metric name to a Metric_Treatment object
  static Metric_Treatment *getMetricTreatment(const string& metric_tag);

  // the following function allows the parser to use special 
  // handling rules for identifiers in particular situations
  // (especially pseudo-keywords like BEGIN_MESSAGE_MAP)
  static string dialectKeywordPolicy(const string& lang, const string& kw);
};

#endif

  





