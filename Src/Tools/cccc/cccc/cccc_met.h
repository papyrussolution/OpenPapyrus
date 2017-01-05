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
#ifndef __CCCC_MET_H
#define __CCCC_MET_H


#include <string>
#include "cccc_db.h"
#include "cccc_itm.h"

enum EmphasisLevel { elLOW=0, elMEDIUM=1, elHIGH=2 };

class CCCC_Html_Stream;
class CCCC_Metric;

// the single class CCCC_Metric which will be defined later in this file 
// will be used for all metrics
// differences in output formats will be handled by giving each object
// of type CCCC_Metric a pointer to a an object of type Metric_Treatment
// which will be held in a global array called Metric_Treatment_Table
class Metric_Treatment
{
  friend class CCCC_Metric;
  friend void add_treatment(CCCC_Item&);
  friend CCCC_Html_Stream& operator <<(CCCC_Html_Stream&,const CCCC_Metric&);

  // a short code string is used to search for the metric treatment, and
  // it has a full name
  string code, name;
  
  // lower_threshold and upper_threshold are the levels at which the metric
  // is interpreted as moving between low, medium and high emphasis levels
  float lower_threshold, upper_threshold;
  
  // for ratio type metrics, we provide the facility for screening out of 
  // items for which the numerator lies below a given value
  // e.g. we may impose a standard of 1 line of comment per 3 of code, but
  // say that we do not require this standard to apply to routines shorter
  // than 5 lines
  int numerator_threshold;
  
  // preferred display width and number of decimal places
  int width, precision;

 public:  
  Metric_Treatment(CCCC_Item& treatment_line);

  friend class CCCC_Options;
};

// the main metric class
class CCCC_Metric {
  Metric_Treatment* treatment;
  float numerator, denominator;
  friend CCCC_Metric& operator+(const CCCC_Metric&, const CCCC_Metric&);
 public:
  CCCC_Metric();
  CCCC_Metric(int n, const char* treatment_tag="");
  CCCC_Metric(int n, int d, const char* treatment_tag="");
  void set_treatment(const char* code);
  void set_ratio(float _num, float _denom=1.0);
  EmphasisLevel emphasis_level() const;
  string code() const;
  string name() const;
  string value_string() const;
};
    

    
#endif /* __CCCC_MET_H */








