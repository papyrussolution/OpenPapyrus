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
// cccc_utl.cc

// implementation of enumerations and utility classes for CCCC
// includes the Parse_Utility class which is a helper to centralise
// error recovery and recording facilities across the three parsers
#include "cccc.h"

#include "cccc_itm.h"
#include "cccc_utl.h"
#include "cccc_db.h"
#include "cccc_tok.h"
#include "AParser.h"
#include "ATokPtr.h"

#define DEBUG_EXTENT_STREAMS 1

#include <cassert>
#include <iomanip>
using std::ios;
//using std::trunc;
using std::ends;
using std::setw;
using std::setiosflags;
using std::resetiosflags;


#define FS "@"
#define RS "\n"

string ParseUtility::stack_rules[MAX_STACK_DEPTH];
int         ParseUtility::stack_tokenline[MAX_STACK_DEPTH];
string ParseUtility::stack_tokentext[MAX_STACK_DEPTH];
int         ParseUtility::stack_depth;

ParseUtility* ParseUtility::theCurrentInstance=NULL;
ParseStore* ParseStore::theCurrentInstance=NULL;

// insertion and extraction functions intended to support enumerations
void insert_enum(ostream& os, int e) 
{ 
  os << (char) e;
}

void extract_enum(istream& is, int& e) 
{
  e=0;
  is >> (char&) e;
}


ostream& operator<<(ostream& os, AugmentedBool ab) {
  insert_enum(os,ab);
  return os;
}

istream& operator>>(istream& is, AugmentedBool& ab) {
  extract_enum(is,(int&)ab);
  return is;
}

ostream& operator<<(ostream& os, Visibility v) {
  insert_enum(os,v);
  return os;
}

istream& operator>>(istream& is, Visibility& v) {
  extract_enum(is,(int&)v);
  return is;
}

ostream& operator<<(ostream& os, UseType ut) {
  insert_enum(os,ut);
  return os;
}

istream& operator>>(istream& is, UseType& ut) {
  extract_enum(is,(int&)ut);
  return is;
}

string ParseUtility::lookahead_text(int n)
{
  static string retval;
  retval="";
  int i;
  for(i=1; i<=n; i++)
    {
      if(parser->LT(i) != NULL)
	{
	  retval=retval+parser->LT(i)->getText();
	  retval=retval+" ";
	}
    }
  return retval;
}

void ParseUtility::resynchronize(int initial_nesting, 
				 SetWordType *resync_token_class, 
				 ANTLRTokenPtr& resync_token)
{
  // the interface for resynchronisation is as follows:
  // the caller supplies a nesting level at which the resynchronisation must 
  // occur, and a token class containing all of the tokens which can 
  // be accepted to delimit the resynchronisation
  // this function will scan until it finds that it is at the correct level and
  // the next token of lookahead is in the resynchronisation set
  // it will then accept as many tokens from the resynchronisation set as
  // are available, consolidating the text of the tokens accepted
  // as the text associated with the last token
  string resync_text="...";

  string string1=parser->LT(1)->getText();
  int line1=parser->LT(1)->getLine();
  string string2;
  int line2=0;

  int resynchronising=1;
  while(resynchronising<1024)
    {
      parser->consumeUntil(resync_token_class);
      if( 
	 (MY_TOK(parser->LT(1))->getNestingLevel() > initial_nesting) &&
	 (parser->LT(2) != NULL)
	 )
	{
	  parser->consume();
	}
      else
	{
	  // we are ready to resynchronise
	  resynchronising=1024;
	  string2=parser->LT(1)->getText();
	  line2=parser->LT(1)->getLine();
	}
	resynchronising ++;
    }
  // we now consume a succession of tokens from the resynchronisation token
  // class until we come across a token which is not in the set, or the
  // nesting level changes
  resync_token=parser->LT(1);
  while(
	parser->set_el(parser->LT(1)->getType(),resync_token_class) &&
	( MY_TOK(parser->LT(1))->getNestingLevel() == initial_nesting) 
	)
    {
      string2=parser->LT(1)->getText();
      line2=parser->LT(1)->getLine();
 
      resync_text+=parser->LT(1)->getText();
      resync_text+=" ";
      resync_token=parser->LT(1);
      resync_token->setText(resync_text.c_str());
      parser->consume();
    }
	
  cerr << "Unrecognized section from " 
       << string1.c_str() << " on line " << line1 << " to " 
       << string2.c_str() << " on line " << line2 << endl
       << "=====ignored section begins=====" << endl
       << resync_text.c_str() << endl
       << "===== ignored section ends =====" << endl;
}


ParseUtility::ParseUtility(ANTLRParser *parser)
{
  // This is designed as a serial-singleton class (e.g. many 
  // instances may exist over time but no more than one at a
  // time).
  // For the lifetime of an instance, the static member theCurrentInstance
  // points to it. When no instance exists, this pointer is null.
  assert(theCurrentInstance==NULL);
  theCurrentInstance=this;

  trace_depth=0;
  stack_depth=0;
  this->parser=(ANTLR_Assisted_Parser*)parser;

}

ParseUtility::~ParseUtility()
{
  theCurrentInstance=NULL;
}

// This utility function is used to create
// a composite scope name from a qualifier scope
// and a relative name.
string ParseUtility::scopeCombine(const string& baseScope, const string& name)
{
	// I am presently (as at 3.pre44) experimenting with
	// how I handle scopes.  The present code has a policy
	// of discarding scope information altogether and defining
	// modules based solely on the final component of the 
	// fully qualified name.
	// This variable may become a parameter to control policy in this
	// area.
	bool bIgnoreScope=true;
	string retval;
	if(bIgnoreScope)
	{
		retval=name;
	}
	else if(baseScope.size()>0 && name.size()>0)
	{
		retval=baseScope+"::"+name;
	}
	else
	{
		retval=baseScope+name;
	}
	
  return retval;
}

ParseStore::ParseStore(const string& filename)
: theFilename(filename)
, pendingLexicalCounts(static_cast<int>(tcLAST),0)
, flag(static_cast<int>(psfLAST)+1,'?')
{
  // This is designed as a serial-singleton class (e.g. many 
  // instances may exist over time but no more than one at a
  // time).
  // For the lifetime of an instance, the static member theCurrentInstance
  // points to it. When no instance exists, this pointer is null.
  assert(theCurrentInstance==NULL);
  theCurrentInstance=this;
  flag[psfLAST]='\0';
}

ParseStore::~ParseStore()
{
  // If the current object came from the default constructor
  // it is the primary singleton instance and we wish to 
  // set the static pointer to itself back to null.  Otherwise,
  // it was a cached copy, and we don't really care.
  if(theCurrentInstance==this)
    {
      theCurrentInstance=NULL;
    }
}

int ParseStore::get_flag(PSFlag psf) const {
  return int(flag[psf]); 
}

void ParseStore::set_flag(PSFlag psf, int value) { 
  flag[psf]=value;
}

void ParseStore::set_flag(Visibility value) { 
  MAKE_STRSTREAM(ofstr);
  ofstr << value;
  flag[psfVISIBILITY]=(ofstr.str())[0];
  RELEASE_STRSTREAM(ofstr);
}

Visibility ParseStore::get_visibility()
{
  return static_cast<Visibility>(flag[psfVISIBILITY]);
}
  
string ParseStore::filename() 
{ 
  return theFilename; 
}

void 
ParseStore::
insert_extent(CCCC_Item& os, int startLine, int endLine,
	      const string& description, const string& flags,
	      UseType ut, bool allocate_lexcounts) 
{
  os.Insert(theFilename);
  os.Insert(startLine);
  os.Insert(description);
  os.Insert(flags);
  int i;
  int lexical_counts_for_this_extent[tcLAST];
  for(i=0; i<tcLAST; i++)
    {
      lexical_counts_for_this_extent[i]=0;
    }

  if(allocate_lexcounts==true)
    {
      LineLexicalCountMatrix::iterator extentStartIter =
	lineLexicalCounts.lower_bound(startLine); 
      LineLexicalCountMatrix::iterator extentEndIter =
	lineLexicalCounts.upper_bound(endLine-1); 
      LineLexicalCountMatrix::iterator llcmIter;
      for(llcmIter=extentStartIter;
	  llcmIter!=extentEndIter; 
	  ++llcmIter)
	{
	  // This record relates to a line within the current
	  // extent.
	  for(i=0; i<tcLAST; i++)
	    {	
	      lexical_counts_for_this_extent[i]+=(*llcmIter).second[i];
	    }
	}
      // The lexical occurrences mentioned in the records processed 
      // above are now been accounted for in the database, so we
      // purge these records.  This has the effect of allowing
      // accurate accounting on nested extents (i.e. the outer
      // extent will only be reported as containing lines which 
      // are not already listed in the inner extent).
      lineLexicalCounts.erase(extentStartIter,extentEndIter);

      ostringstream lexcount_str;

      lexcount_str << "LOC:" << lexical_counts_for_this_extent[tcCODELINES]
		   << " COM:" << lexical_counts_for_this_extent[tcCOMLINES]
		   << " MVG:" << lexical_counts_for_this_extent[tcMCCABES_VG]
		   << ends;

      os.Insert(lexcount_str.str().c_str());

    }
  else
    {
      os.Insert("*");
    }
  os.Insert((char)flag[psfVISIBILITY]);
  os.Insert((char)ut);
}


  
void ParseStore::record_module_extent(int startLine, int endLine, 
				      const string& moduleName, 
				      const string& moduleType,
				      const string& description,
				      UseType ut)
{
  // See the lengthy comment in record_userel_extent about
  // why we are filtering for empty module names.
  if(moduleName.size()>0)
  {
    CCCC_Item module_line;
    module_line.Insert(moduleName);
    module_line.Insert(moduleType);
    insert_extent(module_line,startLine,endLine,
	 description,flags(),ut,true);
    prj->add_module(module_line);
  }
}

void ParseStore::record_function_extent(int startLine, int endLine, 
					const string& returnType, 
					const string& moduleName,
					const string& memberName, 
					const string& paramList,
					const string& description,
					Visibility visibility,
					UseType ut)
{
  // We require every call to this function to specify a member
  // function name and a parameter list.
  if(memberName.size()>0)
  {
    // If the moduleName is an empty string, we remap this to the
	// string "anonymous".  This implies that we treat all
	// C-style functions as belonging to a single module.
	string mappedModuleName = moduleName;
    if(mappedModuleName.size()==0)
	{
	   mappedModuleName = "anonymous";
	}

    CCCC_Item function_line;
    function_line.Insert(mappedModuleName);
    function_line.Insert(memberName);
    function_line.Insert(returnType);
    function_line.Insert(paramList);

    string baseFlags=flags();
    baseFlags[psfVISIBILITY]=visibility;

    insert_extent(function_line,startLine,endLine,
     description,baseFlags,ut,true);
    prj->add_member(function_line);
  }
}

void ParseStore::record_userel_extent(int startLine, int endLine,
				      const string& clientName, 
				      const string& memberName,
				      const string& serverName,
				      const string& description,
				      Visibility visibility,
				      UseType ut)
{
  CCCC_Item userel_line;
  
  // This function should not be invoked unless the clientName
  // and serverName are non-empty strings, however it appears
  // that in test case prn16.java the parser does execute the 
  // actions of the 'implementsClause' rule, even though there
  // is no 'implements' keyword outside comments in the program
  // text.
  // I don't understand this, but as a workaround, I filter at 
  // this point and ensure that if either clientName or serverName
  // is empty, no action is taken.
  if(clientName.size()>0 && serverName.size()>0)
  {
	  userel_line.Insert(clientName);
	  userel_line.Insert(memberName);
	  userel_line.Insert(serverName);

	  // for data member definitions, we record lexical data for the
	  // extent,
	  // for inheritance and parameter relationships we do not
	  bool record_lexcounts=false;
	  switch(ut)
		{
		case utHASBYVAL:
		case utHASBYREF:
		  record_lexcounts=true;
		  break;
		default:
		  record_lexcounts=false;
		}

	  string baseFlags=flags();
	  baseFlags[psfVISIBILITY]=visibility;
	  insert_extent(userel_line,startLine,endLine,
			description,baseFlags,ut,record_lexcounts);
	  prj->add_userel(userel_line);
   }
}

void ParseStore::record_other_extent(int startLine, int endLine, 
					  const string& description)
{
  CCCC_Item rejext_line;
  insert_extent(rejext_line,startLine,endLine,description,flags(),utREJECTED,true);
  prj->add_rejected_extent(rejext_line);
}

static void toktrace(ANTLRAbstractToken *tok)
{
  // at the LHS we put out information about the current token
  if(tok != NULL)
    {
      DbgMsg(PARSER,cerr,
	     std::setw(6) << tok->getLine() 
	     << std::setw(4) << (int)tok->getType()
	     << std::setiosflags(ios::left) 
	     << std::resetiosflags(ios::right) 
	     << std::setw(20) << tok->getText()
	     );
    }
  else
    {
      DbgMsg(PARSER,cerr,std::setw(30)<<"");
    }
}

enum InOrOut { IO_IN, IO_OUT };

static void rectrace(const char *rulename, 
		     const char *dir_indic, 
		     int guessing, 
		     ANTLRAbstractToken *tok)
{
  static int trace_depth=0;
  if(guessing)
    {
      DbgMsg(PARSER,cerr,
	     setw(trace_depth*4+1) << "" << dir_indic 
	     << "?" << rulename << endl);
    }
  else
    {
      trace_depth=((ANTLRToken*) tok)->getNestingLevel();
      DbgMsg(PARSER,cerr,
	     setw(trace_depth*4)<< "" << dir_indic << rulename << endl);
    }
}

void ParseUtility::tracein(
			   const char *rulename, int guessing, 
			   ANTLRAbstractToken *tok)
{
  if(guessing == 0)
    {
      stack_tokentext[stack_depth]=tok->getText();
      stack_tokenline[stack_depth]=tok->getLine();
      stack_rules[stack_depth]=rulename;
      stack_depth++;
    }

  // first put out the token details
  toktrace(tok);
  
  // then the indented recognition trace
  rectrace(rulename,"-> ",guessing,tok);
}

void ParseUtility::traceout(const char *rulename, 
			    int guessing, 
			    ANTLRAbstractToken *tok)
{
  if(guessing == 0)
    {
      stack_depth--;
      // some error checking...
      if(stack_depth<0)
	{
	  cerr << "ParseUtility::traceout negative stack depth - "
	       << "exiting from rule " << rulename 
	       << " at " << tok->getText() << " on line " << tok->getLine() 
	       << endl;
	}
      else if(rulename!=stack_rules[stack_depth])
	{
	  cerr << "ParseStore::traceout rule name mismatch - "
	       << rulename << "!=" << stack_rules[stack_depth] << endl;
	}
      stack_tokentext[stack_depth]="";
      stack_tokenline[stack_depth]=0;
      stack_rules[stack_depth]="";
    }
  // first put out the token details
  toktrace(tok);
  rectrace(rulename,"<- ",guessing,tok);
}
  
void ParseUtility::syn(
  _ANTLRTokenPtr tok, ANTLRChar *egroup, SetWordType *eset,
  ANTLRTokenType etok, int k) 
{
	if(ParseStore::currentInstance()) {
		string filename=ParseStore::currentInstance()->filename();
		if(tok != NULL) {
			cerr << filename << '(' << tok->getLine() << "):" 
				<< " syntax error at token " << tok->getText() << endl;
		}
		else {
			cerr << filename << "(0): syntax error at null token" << endl;
		}

#if 1
	  // The logic in the other half of this #if section
	  // generated too much noise for some people's taste.
	  // It's only really useful to myself (TJL) or anyone
	  // else with a taste for debugging cccc.g/java.g etc.
		int i=stack_depth-1;
		cerr << filename << '(' << stack_tokenline[i] 
			<< "): trying to match " << stack_rules[i]
			<< " at '" << stack_tokentext[i] << "'"
			<< endl;
#else
		cerr << "Parser context:" << endl;
		for(int i=stack_depth-1; i>=0; i--) {
			cerr << filename << '(' << stack_tokenline[i] 
				<< "): trying to match " << stack_rules[i]
				<< " at '" << stack_tokentext[i] << "'"
				<< endl;
		}	
#endif
	}
}	

void ParseStore::endOfLine(int line)
{
  // We only do the processing below if the line which has just
  // ended contained at least one non-skippable token
  // The flag which tells us whether this is true is set in the 
  // token constructor
  if(ANTLRToken::bCodeLine)
  {
	pendingLexicalCounts[tcCODELINES]++;
    LineLexicalCountMatrix::value_type 
      vt(line,LexicalCountArray(static_cast<int>(tcLAST),0));

    for(int i=0; i<tcLAST; i++)
      {
	vt.second[i]=pendingLexicalCounts[i];
	pendingLexicalCounts[i]=0;
      }
    lineLexicalCounts.insert(vt);

	// reset the flat for next time
	ANTLRToken::bCodeLine=false;
  }
}








