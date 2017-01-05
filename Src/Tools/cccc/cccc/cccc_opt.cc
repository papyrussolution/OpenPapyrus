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
// cccc_opt.cc
#include "cccc.h"
#include <fstream>

#include "cccc_opt.h"
#include "cccc_utl.h"
#include "cccc_met.h"

#include <map>

typedef std::map<string,string> file_extension_language_map_t;
typedef std::map<string,Metric_Treatment*> metric_treatment_map_t;
typedef std::pair<string,string> dialect_keyword_t;
typedef std::map<dialect_keyword_t,string> dialect_keyword_map_t;

static file_extension_language_map_t extension_map;
static metric_treatment_map_t treatment_map;
static dialect_keyword_map_t dialect_keyword_map;

// these are declared extern so that it can be defined later in the file
extern char *default_fileext_options[];
extern char *default_treatment_options[];
extern char *default_dialect_options[];

static void add_file_extension(CCCC_Item& fileext_line)
{
	string ext, lang;
	if(
		fileext_line.Extract(ext) &&
		fileext_line.Extract(lang)
		)
    {
		file_extension_language_map_t::value_type extension_pair(ext,lang);
		extension_map.insert(extension_pair);
    }
}

void add_treatment(CCCC_Item& treatment_line)
{
	Metric_Treatment *new_treatment=new Metric_Treatment(treatment_line);
	metric_treatment_map_t::iterator iter=
		treatment_map.find(new_treatment->code);
	
	if(iter!=treatment_map.end())
    {
		delete (*iter).second;
		(*iter).second=new_treatment;
    }
	else
    {
		metric_treatment_map_t::value_type 
			treatment_pair(new_treatment->code,new_treatment);
		treatment_map.insert(treatment_pair);
    }
}

static void add_dialect_keyword(CCCC_Item& dialect_keyword_line)
{
	string dialect, keyword, policy;
	if(
		dialect_keyword_line.Extract(dialect) &&
		dialect_keyword_line.Extract(keyword) &&
		dialect_keyword_line.Extract(policy) 
		)
    {
		dialect_keyword_map_t::key_type kt(dialect,keyword);
		dialect_keyword_map_t::value_type vt(kt,policy);
		dialect_keyword_map.insert(vt);
    }
}

void CCCC_Options::Add_Option(CCCC_Item& option)
{
	string first_token;
	bool retval=option.Extract(first_token);
	
	if(retval==false)
    {
		// do nothing
    }
	else if(first_token=="CCCC_FileExt")
    {
		add_file_extension(option);
    }
	else if(first_token=="CCCC_MetTmnt")
    {
		add_treatment(option);
    }
	else if(first_token=="CCCC_Dialect")
    {
		add_dialect_keyword(option);
    }
	else
    {
		retval=false;
    }
}


void CCCC_Options::Save_Options(const string& filename)
{
	ofstream optstr(filename.c_str());
	file_extension_language_map_t::iterator felIter;
	for(felIter=extension_map.begin();
	felIter!=extension_map.end();
	++felIter)
    {
		CCCC_Item extLine;
		extLine.Insert("CCCC_FileExt");
		extLine.Insert((*felIter).first.c_str());
		extLine.Insert((*felIter).second.c_str());
		extLine.ToFile(optstr);
    }
	
	metric_treatment_map_t::iterator tIter;
	for(tIter=treatment_map.begin();
	tIter!=treatment_map.end();
	++tIter)
    {
		CCCC_Item tmtLine;
		tmtLine.Insert("CCCC_MetTmnt");
		tmtLine.Insert((*tIter).second->code); 
		tmtLine.Insert((*tIter).second->lower_threshold);
		tmtLine.Insert((*tIter).second->upper_threshold);
		tmtLine.Insert((*tIter).second->numerator_threshold);
		tmtLine.Insert((*tIter).second->width);     
		tmtLine.Insert((*tIter).second->precision);
		tmtLine.Insert((*tIter).second->name);
		tmtLine.ToFile(optstr);
    }
	
	dialect_keyword_map_t::iterator dkIter;
	for(dkIter=dialect_keyword_map.begin();
	dkIter!=dialect_keyword_map.end();
	++dkIter)
    {
		CCCC_Item dkLine;
		dkLine.Insert("CCCC_Dialect");
		dkLine.Insert((*dkIter).first.first);
		dkLine.Insert((*dkIter).first.second);
		dkLine.Insert((*dkIter).second);
		dkLine.ToFile(optstr);
    }
}

void CCCC_Options::Load_Options(const string& filename)
{
	ifstream optstr(filename.c_str());
	
	while(optstr.good())
    {
		CCCC_Item option_line;
		if(optstr.good() &&
			option_line.FromFile(optstr)
			)
		{
			Add_Option(option_line);
		}
    }
	
}

// initialise using hard-coded defaults
void CCCC_Options::Load_Options()
{  
	int i=0;
	
	char **option_ptr;
	
	option_ptr=default_fileext_options;
	while( (*option_ptr)!=NULL)
    {
		string option_string="CCCC_FileExt@";
		option_string+=(*option_ptr);
		CCCC_Item option_line(option_string);
		Add_Option(option_line);
		option_ptr++;
    }
	
	option_ptr=default_treatment_options;
	while( (*option_ptr)!=NULL)
    {
		string option_string="CCCC_MetTmnt@";
		option_string+=(*option_ptr);
		CCCC_Item option_line(option_string);
		Add_Option(option_line);
		option_ptr++;
    }
	
	option_ptr=default_dialect_options;
	while( (*option_ptr)!=NULL)
    {
		string option_string="CCCC_Dialect@";
		option_string+=(*option_ptr);
		CCCC_Item option_line(option_string);
		Add_Option(option_line);
		option_ptr++;
    }
}

// map a filename to a language
string CCCC_Options::getFileLanguage(const string& filename)
{
	string retval;
	string extension;
	file_extension_language_map_t::iterator iter;
	
	unsigned int extpos=filename.rfind(".");
	if(extpos!=string::npos)
    {
		extension=filename.substr(extpos);
		iter=extension_map.find(extension);
		if(iter!=extension_map.end())
		{
			retval=(*iter).second;
		}
    }
	if(retval.size()==0)
    {
		iter=extension_map.find("");
		if(iter!=extension_map.end())
		{
			retval=(*iter).second;
		}
		else
		{
			// could not find language for extension
			cerr << "No language found for extension " << extension.c_str() << endl;
		}
    }
	return retval;
}

// map a metric name to a Metric_Treatment object
Metric_Treatment *CCCC_Options::getMetricTreatment(const string& metric_tag)
{
	Metric_Treatment *retval=NULL;
	metric_treatment_map_t::iterator iter=treatment_map.find(metric_tag);
	if(iter!=treatment_map.end())
    {
		retval=(*iter).second;
    }
	return retval;
}

string CCCC_Options::dialectKeywordPolicy(const string& lang, const string& kw)
{
	string retval;
	dialect_keyword_map_t::key_type kt(lang,kw);
	dialect_keyword_map_t::const_iterator iter=dialect_keyword_map.find(kt);
	if(iter!=dialect_keyword_map.end())
    {
		retval=(*iter).second;
    }
	return retval;
}


char *default_fileext_options[]=
{
	// file extensions
	".c@c.ansi@",
		
		".h@c++.ansi@",
		".cc@c++.ansi@",
		".cpp@c++.ansi@",
		".cxx@c++.ansi@",
		".c++@c++.ansi@",
		".C@c++.ansi@",
		".CC@c++.ansi@",
		".CPP@c++.ansi@",
		".CXX@c++.ansi@",
		".hh@c++.ansi@",
		".hpp@c++.ansi@",
		".hxx@c++.ansi@",
		".h++@c++.ansi@",
		".H@c++.ansi@",
		".HH@c++.ansi@",
		".HPP@c++.ansi@",
		".HXX@c++.ansi@",
		".H++@c++.ansi@",
		
		".j@java@",
		".jav@java@",
		".java@java@",
		".J@java@",
		".JAV@java@",
		".JAVA@java@",
		
		".ada@ada.95@",
		".ads@ada.95@",
		".adb@ada.95@",
		
		".ADA@ada.95@",
		".ADS@ada.95@",
		".ADB@ada.95@",
		
		// The language associated with the empty file extension would be used as a default
		// if defined.
		// This is presently disabled so that we don't process files in 
		// MSVC projects like .rc, .odl which are not in C++.
		// "@c++.ansi@",
		
		NULL
};

char *default_treatment_options[] = 
{
	// metric treatments
	// all metric values are displayed using the class CCCC_Metric, which may be
	// viewed as ratio of two integers associated with a character string tag
	// the denominator of the ratio defaults to 1, allowing simple counts to
	// be handled by the same code as is used for ratios
	//
	// the tag associated with a metric is used as a key to lookup a record
	// describing a policy for its display (class Metric_Treatment)
	//
	// the fields of each treatment record are as follows:
	// TAG     the short string of characters used as the lookup key. 
	// T1, T2  two numeric thresholds which are the lower bounds for the ratio of
	// the metric's numerator and denominator beyond which the 
	//  value is treated as high or extreme by the analyser
	//  these will be displayed in emphasized fonts, and if the browser
	//  supports the BGCOLOR attribute, extreme values will have a red
	//  background, while high values will have a yellow background.
	//  The intent is that high values should be treated as suspicious but
	//  tolerable in moderation, whereas extreme values should almost 
	//  always be regarded as defects (not necessarily that you will fix 
	//  them).
	// NT  a third threshold which supresses calculation of ratios where
	//     the numerator is lower than NT.
	//     The principal reason for doing this is to prevent ratios like L_C
	//  being shown as *** (infinity) and displayed as extreme when the 
	//  denominator is 0, providing the numerator is sufficiently low.
	//  Suitable values are probably similar to those for T1.
	// W       the width of the metric (total number of digits).
	// P       the precision of the metric (digits after the decimal point).
	// Comment a free form field extending to the end of the line.
	
	// TAG      T1      T2   NT  W  P Comment
	"LOCf@      30@    100@   0@ 6@ 0@Lines of code/function@",
	"LOCm@     500@   2000@   0@ 6@ 0@Lines of code/single module@", 
	"LOCper@   500@   2000@   0@ 6@ 3@Lines of code/average module@", 
	"LOCp@  999999@ 999999@   0@ 6@ 0@Lines of code/project@", 
	"MVGf@      10@     30@   0@ 6@ 0@Cyclomatic complexity/function@",
	"MVGm@     200@   1000@   0@ 6@ 0@Cyclomatic complexity/single module@",
	"MVGper@   200@   1000@   0@ 6@ 3@Cyclomatic complexity/average module@",
	"MVGp@  999999@ 999999@   0@ 6@ 0@Cyclomatic complexity/project@",
	"COM@   999999@ 999999@   0@ 6@ 0@Comment lines@",
	"COMper@999999@ 999999@   0@ 6@ 3@Comment lines (averaged)@",
	"M_C@        5@     10@   5@ 6@ 3@MVG/COM McCabe/comment line@",
	"L_C@        7@     30@  20@ 6@ 3@LOC/COM Lines of code/comment line@",
	"FI@        12@     20@   0@ 6@ 0@Fan in (overall)@",
	"FIv@        6@     12@   0@ 6@ 0@Fan in (visible uses only)@",
	"FIc@        6@     12@   0@ 6@ 0@Fan in (concrete uses only)@",
	"FO@        12@     20@   0@ 6@ 0@Fan out (overall)@",
	"FOv@	       6@     12@   0@ 6@ 0@Fan out (visible uses only)@",
	"FOc@        6@     12@   0@ 6@ 0@Fan out (concrete uses only)@",
	"IF4@      100@   1000@   0@ 6@ 0@Henry-Kafura/Shepperd measure (overall)@",
	"IF4v@      30@    100@   0@ 6@ 0@Henry-Kafura/Shepperd measure (visible)@",
	"IF4c@      30@    100@   0@ 6@ 0@Henry-Kafura/Shepperd measure (concrete)@",
	 // WMC stands for weighted methods per class, 
	 // the suffix distinguishes the weighting function
	 "WMC1@      30@    100@   0@ 6@ 0@Weighting function=1 unit per method@",
	 "WMCv@      10@     30@   0@ 6@ 0@Weighting function=1 unit per visible method@",
	 "DIT@	       3@      6@   0@ 6@ 0@Depth of Inheritance Tree@",
	 "NOC@	       4@     15@   0@ 6@ 0@Number of children@",
	 "CBO@	      12@     30@   0@ 6@ 0@Coupling between objects@",
	 "8.3@    999999@ 999999@   0@ 8@ 3@General format for fixed precision 3 d.p.@",
	 NULL
};

char *default_dialect_options[] = 
{
	// This configuration item allows the description of 
	// dialects in which C/C++ identifiers get treated
	// as supplementary keyword.  The rules specified
	// here (or in the runtime option file, if specified)
	// allow the parser to make the call 
	// CCCC_Options::dialect_keyword_policy(lang,kw) which
	// returns a string.  If nothing is known about the 
	// pair <lang,kw>, an empty string is returned.  If
	// an entry is specified here the associated string is 
	// returned, which is called the policy, which the
	// parser uses to guide its actions.  The most common
	// policy is to ignore, but any string can be 
	// specified, providing the person implementing
	// the parser can think of an intelligent way to 
	// proceed.
	"c++.mfc@BEGIN_MESSAGE_MAP@start_skipping@",
		"c++.mfc@END_MESSAGE_MAP@stop_skipping@",
		"c++.stl@__STL_BEGIN_NAMESPACE@ignore@",
		"c++.stl@__STL_END_NAMESPACE@ignore@",
		NULL
};











