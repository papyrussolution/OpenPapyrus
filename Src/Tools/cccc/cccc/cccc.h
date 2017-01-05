/*
 * cccc.h 
 * diagnostic and portability facilities for the cccc project
 */

#ifndef _CCCC_H__
#define _CCCC_H__

#ifdef _WIN32
#pragma warning (disable:4786 4503)
#endif

// I am trying to standardise on using the ANSI C++ names
// for the ANSI C header files, and bringing all of 
// the includes of these libraries into this file.
// I have not yet attempted to purge includes for these
// files from the other source files.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

#include <string>
using std::string;
#include <iostream>
#include <sstream>
#include <fstream>
using std::ostream;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::endl;
using std::cout;
using std::cerr;

// debugging facilities
extern int DebugMask;
enum DebugFlags { 
  LEXER=0x01, 
  PARSER=0x02, 
  COUNTER=0x04, 
  MEMORY=0x08,
  EXTENT=0x10,
  DATABASE=0x20
};
#define DbgMsg(DF,OS,X) if(DebugMask&DF) { OS << X ; }

// the global database to which stuff is added...
class CCCC_Project;
extern CCCC_Project *prj;

// a nasty global array of identifiers we want the lexer to ignore
#define SKIP_IDENTIFIERS_ARRAY_SIZE 256
extern char *skip_identifiers[SKIP_IDENTIFIERS_ARRAY_SIZE];
#if 0
#include "DLGLexer.h"
#endif

// These macros were used to cover differences between the way the
// old strstream classes were used in Win32 and GNU builds.
// The differences are no longer necessary.
#define MAKE_STRSTREAM(X)     stringstream X;
#define CONVERT_STRSTREAM(X)  (X)
#define RELEASE_STRSTREAM(X)  


// The -gd option generates uncompilable code with a missing 
// variable called zzTracePrevRuleName if the generated 
// files cccc.cpp, java.cpp, ada.cpp don't include a version
// of AParser.h seen with zzTRACE_RULES defined.
// I'm not sure how this is supposed to work, but for the moment
// I am including it here which should make all three files OK.
// Note that this could break again if the header files shift around
// and AParser.h gets read before zzTRACE_RULES is defined.
// Another option is turning -gd off, but its the way we do the
// cccc -dp debug output which is very useful.
#include "cccc_tok.h"
#define zzTRACE_RULES
#include "AParser.h"

#endif










