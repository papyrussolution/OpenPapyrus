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
// cccc_utl.h

#ifndef __CCCC_UTL_H
#define __CCCC_UTL_H

#include "cccc.h"
#include <map>
#include <vector>
#include "cccc_tok.h"
#include "AParser.h"

class ANTLRAbstractToken;
class ANTLRTokenPtr;
class CCCC_Item;

// this file declares all enumeration datatypes used in the project, and
// also the parse state class, which is used to capture information in the
// parse and transfer it to the code database for later report generation

// for each enumeration, a single character code is defined for each member
// these codes are shown in the inline comments

// the enumerations are designed to support resolution of incomplete 
// knowledge about several sections of code which relate to the same 
// object to give the most complete picture available

class AST;

// the languages which can be parsed
// only C and C++ are implemented as yet
enum Language { lAUTO, lCPLUSPLUS, lANSIC, lJAVA, lADA };
extern Language global_language, file_language;

enum Visibility { 
  vPUBLIC='0',vPROTECTED='1',vPRIVATE='2',vIMPLEMENTATION='3', 
  vDONTKNOW='?',vDONTCARE='X',vINVALID='*'
};
ostream& operator << (ostream&, Visibility);
istream& operator >> (istream&, Visibility&);

enum AugmentedBool { 
  abFALSE='F', abTRUE='T', abDONTKNOW='?', abDONTCARE='X', abINVALID='*'
};
ostream& operator << (ostream& os, AugmentedBool ab);
istream& operator >> (istream& is, AugmentedBool& ab);

enum UseType { 
  utDECLARATION='D', utDEFINITION='d',  // of methods and classes
  utINHERITS='I',                       // inheritance, including Java 
  // extends and implements relations
  utHASBYVAL='H', utHASBYREF='h',       // class data member
  utPARBYVAL='P', utPARBYREF='p',       // method parameter or return value
  utVARBYVAL='V', utVARBYREF='v',       // local variable within a method
  utTEMPLATE_NAME='T',                  // typedef alias for a template
  utTEMPLATE_TYPE='t',                  // type over which a template is 
  // instantiated
  utINVOKES='i',                        // C function invocation 
  utREJECTED='r',                       // for extents rejected by the parser
  utWITH='w',                           // Ada 'with' keyword context
  utDONTKNOW='?', utDONTCARE='X', utINVALID='*'
};

// the parse state object consists of a number of strings representing 
// knowledge about the identification of the source code object currently 
// being processed, a number of flags of type AugmentedBool, and 
// items representing knowledge about the 
// concerning the object's nature, and also its visibility

enum PSString { 
  pssFILE, pssRULE, pssFLAGS, // the context of the parse
  pssMODTYPE, pssMODULE,      // the syntactic class and name of the module 
  pssUTYPE,                    // unqualified type of the current member
  pssINDIR,                   // indirection associated with the type above
  pssITYPE,                   // type qualified with indirection
  pssMEMBER, pssPARAMS,       // name, parameter list of a member
  pssDESCRIPTION,             // textual description of the relationship type
  pssLAST                     // used to dimension the array
};

enum PSFlag { 
  psfCONST, psfSTATIC, psfEXTERN, psfVIRTUAL, // AugmentedBool
  psfVISIBILITY,                              // Visibility
  psfLAST                                     // used to dimension the array
};
enum PSVerbosity { psvSILENT, psvQUIET, psvLOUD };

#define MAX_STACK_DEPTH 1000

// I have moved some actions originally embedded within the C++ grammar
// out of the grammar into the class ParseUtility defined below, so that
// other grammars can use them as well for consistency and efficiency.
// The ParseUtility::resynchronize() method provides a standardised way
// of 1) resynchronising the parser, and 2) reporting the parse error 
// which caused the problem.  Unfortunately, to do the resynchronisation
// it requires access to protected functions of ANTLRParser.
// The class ANTLR_Assisted_Parser below is a hack to enable ParseUtility
// to violate the protection of the functions required: ParseUtility is 
// passed a pointer to a real parser which is of a subclass of ANTLRParser, 
// and casts it to this artificial subclass, so as to give ParseUtility 
// friend rights and to access the protected functions.
// This hack is necessary because the class definition we need to affect
// is generated by PCCTS: I am not proud of it and if anyone can suggest
// a way of doing without modifying PCCTS or its support code, I will be
// very happy to hear about it.
class ANTLR_Assisted_Parser : public ANTLRParser
{
  ANTLR_Assisted_Parser(ANTLRParser& parser) : ANTLRParser(parser) {}
  friend class ParseUtility;
};

// The parse utility class is intended to assist the parser in a number
// of ways.  In earlier versions, this class had at least two distinct
// roles:
// 1) as a place for common functions which each parser might call
//    for diagnostics, resynchronisation etc; and
// 2) as a general storage area for state which needs to be remembered
//    for any length of time during the parsing process.
// The class ParseStore has been added to support the second role,
// and it is hoped that the amount of stored state can be reduced
// in the near future.
class ParseUtility {

 public:
  ParseUtility(ANTLRParser *parser);
  ~ParseUtility();

  // the following methods are used to service the standard tracein/traceout
  // and syntax error reporting calls generated by PCCTS
  void tracein(const char *rulename, int guessing, ANTLRAbstractToken *tok);
  void traceout(const char *rulename, int guessing, ANTLRAbstractToken *tok);
  void syn(_ANTLRTokenPtr tok, ANTLRChar *egroup, SetWordType *eset,
	   ANTLRTokenType etok, int k);

  // this method consolidates the text of the next n tokens of lookahead
  string lookahead_text(int n);

  // this method searches for a string of tokens at the specified nesting
  // depth from the specified token class, and uses them as a marker to 
  // resynchronise the parser
  void resynchronize(
		     int initial_nesting, SetWordType *resync_token_class, 
		     ANTLRTokenPtr& resync_token);

  // This utility function is used to create
  // a composite scope name from a qualifier scope
  // and a relative name.
  string scopeCombine(const string& baseScope, const string& name);

  // Only one instance of this class should exist at any time.
  // This method allows the parsers and lexers to access the instance.
  static ParseUtility *currentInstance() { return theCurrentInstance; }

 private:
  static ParseUtility *theCurrentInstance;

  ANTLR_Assisted_Parser *parser;
  int trace_depth;
  static int stack_depth;
  static string   stack_tokentext[MAX_STACK_DEPTH];
  static int           stack_tokenline[MAX_STACK_DEPTH];  
  static string   stack_rules[MAX_STACK_DEPTH];

  // copy constructor and assignment operator are private to
  // prevent unexpected copying
  ParseUtility(const ParseUtility&);
  const ParseUtility& operator=(const ParseUtility&);
};

  // LOC, COM and MVG are all counted by the lexical analyzer,
  // but the counts must be apportioned after the parser has
  // identified the extents of the various declarations and definitions
  // they belong to.
  // This is achieved by the lexer maintaining counts of each
  // which are reported to the ParseUtility class on a line by line
  // basis.  ParseUtility uses this data to create a store which is
  // used to apportion counts as the parser reports extents.
  enum LexicalCount { tcCOMLINES, tcCODELINES, tcMCCABES_VG, tcLAST };


// The ParseStore class encapsulates all information storage 
// requirements related to the parser, and also manages
// the process of feeding that information to the database
// when it is complete.
// In particular, the class is responsible for receiving and
// retaining counts of the lexical metrics (LOC, COM,
// MVG) on a line-by-line basis.  These are counted in the 
// lexical analyzer, and the line-by-line counts must be 
// integrated to allocate the counts to the extents identified
// by the parser as belonging to significant declarations and
// definitions.
class ParseStore
{
 public:
  ParseStore(const string& filename);
  ~ParseStore();

  void IncrementCount(LexicalCount lc) { pendingLexicalCounts[lc]++; }
  void endOfLine(int line);


  // each of the functions below writes one or more records into 
  // the database of code
  void record_module_extent(int startLine, int endLine, 
			    const string& moduleName, 
			    const string& moduleType,
			    const string& description,
			    UseType ut);
  void record_function_extent(int startLine, int endLine, 
			      const string& returnType, 
			      const string& moduleName,
			      const string& memberName, 
			      const string& paramList,
			      const string& description,
			      Visibility visibility, 
			      UseType ut);
  void record_userel_extent(int startLine, int endLine,
			    const string& clientName, 
			    const string& memberName,
			    const string& serverName,
			    const string& description,
			    Visibility visibility,
			    UseType ut);
  void record_other_extent(int startLine, int endLine, 
			      const string& description);
  void record_file_balance_extent(string);

  // Each of the record_XXX methods above uses this function to 
  // add an extent record.
  void insert_extent(CCCC_Item&, int, int, 
		     const string&, const string&, 
		     UseType, bool allocate_lexcounts);

  // the class maintains a number of strings and flags which reflect 
  // the most recently recognized module, member, type (with and without 
  // indirection) etc, and the visibility of items occuring at the current
  // context
  int get_flag(PSFlag) const;
  void set_flag(PSFlag,int);
  void set_flag(Visibility);
  Visibility get_visibility();
  string filename();

  char *flags() { return &(*flag.begin()); }

  // We also need the automatically generated copy constructor
  // and assignment operator to allow us to save state in the 
  // parser.

  // Only one instance of this class should exist at any time.
  // This method allows the parsers and lexers to access the instance.
  static ParseStore *currentInstance() { return theCurrentInstance; }
 private:
  static ParseStore *theCurrentInstance;

  string theFilename;

  typedef std::vector<int> LexicalCountArray;
  LexicalCountArray pendingLexicalCounts;
 
  typedef std::map<int,LexicalCountArray> LineLexicalCountMatrix;
  LineLexicalCountMatrix lineLexicalCounts;

  typedef std::vector<char> CharArray;
  CharArray flag;

  // copy constructor and assignment operator are private to
  // prevent unexpected copying
  ParseStore(const ParseStore&);
  const ParseStore& operator=(const ParseStore&);
};

#endif










