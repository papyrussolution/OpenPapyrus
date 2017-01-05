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
 * cccc_tok.C
 * implementation of a token class for the cccc project
 *
 */

#include "cccc.h"
#include "cccc_tok.h"

/* static variables */
int ANTLRToken::RunningNesting=0;
int ANTLRToken::bCodeLine=0;
int ANTLRToken::numAllocated=0;
int toks_alloc1=0, toks_alloc2=0, toks_alloc3=0, toks_freed=0;

ANTLRToken currentLexerToken;

/* 
** Token objects are used to count the occurences of states which 
** our analyser is interested in within the text.  Any metric which
** can be reduced to lexical counting on the text can be recorded
** this way.
**
** This implementation counts the following features:
**   tokens
**   comment lines
**   lines containing at least one token of code
**  
** It also makes a lexical count for the following tokens, each of which
** is expected to increase McCabe's cyclomatic complexity (Vg) for the 
** section of code by one unit:
**  IF FOR WHILE SWITCH BREAK RETURN ? && ||
**
** Note that && and || create additional paths through the code due to C/C++ 
** short circuit evaluation of logical expressions.
**
** Also note the way SWITCH constructs are counted: the desired increment
** in Vg is equal to the number of cases provided for, including the 
** default case, whether or not an action is defined for it.  This is acheived
** by counting the SWITCH at the head of the construct as a surrogate for 
** the default case, and counting BREAKs as surrogates for the individual
** cases.  This approach yields the correct results provided that the
** coding style in use ensures the use of BREAK after all non-default
** cases, and forbids 'drop through' from one case to another other than
** in the case where two or more values of the switch variable require
** identical actions, and no executable code is defined between the 
** case gates (as in the switch statement in ANTLRToken::CountToken() below).
*/

/* default constructor */
ANTLRToken::ANTLRToken() : ANTLRCommonToken() { 
  toks_alloc1++;
  CurrentNesting=-99;
}

/* 
** constructor used by makeToken below 
*/
ANTLRToken::ANTLRToken(ANTLRTokenType t, ANTLRChar *s) : 
  ANTLRCommonToken(t,s) {
  setType(t);
  setText(s);
  CountToken();

  toks_alloc2++;
}

/* copy constructor */
ANTLRToken::ANTLRToken(ANTLRToken& copyTok) {
  setType(copyTok.getType());
  setText(copyTok.getText());
  setLine(copyTok.getLine());
  CurrentNesting=copyTok.CurrentNesting;
  toks_alloc3++;
}

/* 
** the virtual pseudo-constructor 
** This is required because the PCCTS support code does not know the
** exact nature of the token which will be created by the user's code, 
** and indeed does not forbid the user creating more than one kind of
** token, so long as ANTLRToken is defined and all token classes are
** subclassed from ANTLRAbstractToken
*/
ANTLRAbstractToken *ANTLRToken::makeToken(
					  ANTLRTokenType tt, ANTLRChar *txt, int line
					  ) {
  
  ANTLRToken *new_t = new ANTLRToken(tt,txt);
  if(new_t==0) { 
    cerr << "Memory overflow in "
      "ANTLRToken::makeToken(" << static_cast<int>(tt) << "," 
	 << txt << "," << line << ")" << endl;
    exit(2);
  }
  new_t->setLine(line);

  DbgMsg(
	 LEXER,cerr, 
	 "makeToken(tt=>" << static_cast<int>(tt) << 
	 ", txt=>" << txt << 
	 ",line=>" << line << 
	 ")" << endl
	 );

  return new_t;
}

/* the destructor */
ANTLRToken::~ANTLRToken() {
  toks_freed++;
  DbgMsg(MEMORY,cerr,"freeing token " << getText()
	 << " on line " << getLine()
	 << " c1:" << toks_alloc1 << " c2:" << toks_alloc2 
	 << " c3:" << toks_alloc3 << " freed:" << toks_freed << endl);
}

/* the assignment operator */
ANTLRToken& ANTLRToken::operator=(ANTLRToken& copyTok) {
  setType(copyTok.getType());
  setText(copyTok.getText());
  setLine(copyTok.getLine());
  CurrentNesting=copyTok.CurrentNesting;
  return *this;
}

/*
** ANTLRToken::CountToken performs counting of features which are traced
** back to individual tokens created up by the lexer, i.e. the token count 
** and McCabes VG.  Code lines and comment lines are both identified during
** the processing of text which the lexer will (usually) skip, so the code
** to increment these counts is in the relevant lexer rules in the file 
** cccc.g
*/
void ANTLRToken::CountToken()
{
  // we have seen a non-skippable pattern => this line counts toward LOC  
  bCodeLine=1;
  CurrentNesting=RunningNesting;
  DbgMsg(COUNTER,cerr,*this);
}

char *ANTLRToken::getTokenTypeName() { return ""; }

/*
** structured output method for token objects
*/
ostream& operator << (ostream& out, ANTLRToken& t) {
  int i;

  out << "TOK: " << t.getTokenTypeName() 
      << " " << t.getText() 
      << " " << t.getLine()
      << " " << t.getNestingLevel();

  out << endl;
  return out;
}







