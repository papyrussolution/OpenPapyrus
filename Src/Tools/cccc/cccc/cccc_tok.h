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
 * cccc_tok.h
 * definition of the token class interface for the cccc project
 *
 */

#ifndef __CCCC_TOK_H
#define __CCCC_TOK_H

#include "cccc.h"

// before we go into the token header file, the compiler must have seen 
// a definition for enum ANTLRTokenType
// there are three conflicting 'real' definitions, one in use by each parser
// if we have seen one of these, we do not need to worry, otherwise we
// must create a dummy one
// the three definitions are in the files Ctokens.h Jtokens.h and Atokens.h
#if !defined(Ctokens_h) && !defined(Jtokens_h) && !defined(Atokens_h)
enum ANTLRTokenType { DUMMY,DEFINITION };
#endif

#include "AToken.h"
#include "cccc.h"


/*
** the class definition for ANTLRToken
** Note that the name ANTLRToken is required to be either a class or a typedef
** by the PCCTS support code
*/
class ANTLRToken : public ANTLRCommonToken {

  // Lexical counting is done by attaching running counts of each of the
  // interesting features to every token produced by the lexer
  // the parser calculates the counts for a particular region by taking
  // taking the differences of the counts for the first and last tokens
  // in the region's extent.

  // nesting levels are used to control resynchronisation
  static int RunningNesting;

  static int numAllocated;
  int CurrentNesting;
  friend ostream& operator << (ostream&,ANTLRToken&);
  friend class DLGLexer;
 public:
  static int bCodeLine;

  ANTLRToken(ANTLRTokenType t, ANTLRChar *s);
  ANTLRToken(ANTLRToken& copyTok);
  ANTLRToken();
  ANTLRToken& operator=(ANTLRToken& copyTok);
  
  virtual ~ANTLRToken();

  virtual ANTLRAbstractToken *makeToken(ANTLRTokenType tt, 
					ANTLRChar *txt, 
					int line);

  static void IncrementNesting() { RunningNesting++; }
  static void DecrementNesting() { RunningNesting--; }

  int getNestingLevel() { return CurrentNesting; }
  void CountToken();
  char *getTokenTypeName();
};

#define MY_TOK(t) ((ANTLRToken*)(t))
ostream& operator << (ostream&, ANTLRToken&);

extern ANTLRToken currentLexerToken;


#endif

 
