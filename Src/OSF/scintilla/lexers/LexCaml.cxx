// Scintilla source code edit control
/** @file LexCaml.cxx
** Lexer for Objective Caml.
**/
// Copyright 2005-2009 by Robert Roessler <robertr@rftp.com>
// The License.txt file describes the conditions under which this software may be distributed.
/*	Release History
        20050204 Initial release.
        20050205 Quick compiler standards/"cleanliness" adjustment.
        20050206 Added cast for IsLeadByte().
        20050209 Changes to "external" build support.
        20050306 Fix for 1st-char-in-doc "corner" case.
        20050502 Fix for [harmless] one-past-the-end coloring.
        20050515 Refined numeric token recognition logic.
        20051125 Added 2nd "optional" keywords class.
        20051129 Support "magic" (read-only) comments for RCaml.
        20051204 Swtich to using StyleContext infrastructure.
        20090629 Add full Standard ML '97 support.
 */
#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

//	Since the Microsoft __iscsym[f] funcs are not ANSI...
/*inline*/ int FASTCALL iscaml(int c) 
{
	return isalnum(c) || c == '_';
}

/*inline*/ int FASTCALL iscamlf(int c) 
{
	return isalpha(c) || c == '_';
}

static const int baseT[24] = {
	0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* A - L */
	0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 16      /* M - X */
};

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

#ifdef BUILD_AS_EXTERNAL_LEXER
/*
        (actually seems to work!)
 */
#include "WindowAccessor.h"

#undef EXT_LEXER_DECL
#define EXT_LEXER_DECL __declspec(dllexport) __stdcall

static void ColouriseCamlDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor & styler);
static void FoldCamlDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor & styler);
static void InternalLexOrFold(int lexOrFold, Sci_PositionU startPos, Sci_Position length, int initStyle, char * words[], WindowID window, char * props);

static const char * LexerName = "caml";

#ifdef TRACE
void Platform::DebugPrintf(const char * format, ...) 
{
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}

#else
void Platform::DebugPrintf(const char *, ...) {
}

#endif

bool Platform::IsDBCSLeadByte(int codePage, char ch) 
{
	return ::IsDBCSLeadByteEx(codePage, ch) != 0;
}

long Platform::SendScintilla(WindowID w, uint msg, ulong wParam, long lParam) 
{
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(WindowID w, uint msg, ulong wParam, void * lParam) 
{
	return ::SendMessage(reinterpret_cast<HWND>(w), msg, wParam, reinterpret_cast<LPARAM>(lParam));
}

void EXT_LEXER_DECL Fold(uint lexer, Sci_PositionU startPos, Sci_Position length, int initStyle, char * words[], WindowID window, char * props)
{
	// below useless evaluation(s) to supress "not used" warnings
	lexer;
	// build expected data structures and do the Fold
	InternalLexOrFold(1, startPos, length, initStyle, words, window, props);
}

int EXT_LEXER_DECL GetLexerCount()
{
	return 1;       // just us [Objective] Caml lexers here!
}

void EXT_LEXER_DECL GetLexerName(uint Index, char * name, int buflength)
{
	// below useless evaluation(s) to supress "not used" warnings
	Index;
	// return as much of our lexer name as will fit (what's up with Index?)
	if(buflength > 0) {
		buflength--;
		int n = sstrlen(LexerName);
		if(n > buflength)
			n = buflength;
		memcpy(name, LexerName, n), name[n] = '\0';
	}
}

void EXT_LEXER_DECL Lex(uint lexer, Sci_PositionU startPos, Sci_Position length,
    int initStyle, char * words[], WindowID window, char * props)
{
	// below useless evaluation(s) to supress "not used" warnings
	lexer;
	// build expected data structures and do the Lex
	InternalLexOrFold(0, startPos, length, initStyle, words, window, props);
}

static void InternalLexOrFold(int foldOrLex, Sci_PositionU startPos, Sci_Position length,
    int initStyle, char * words[], WindowID window, char * props)
{
	// create and initialize a WindowAccessor (including contained PropSet)
	PropSetSimple ps;
	ps.SetMultiple(props);
	WindowAccessor wa(window, ps);
	// create and initialize WordList(s)
	int nWL = 0;
	for(; words[nWL]; nWL++) ;      // count # of WordList PTRs needed
	WordList** wl = new WordList* [nWL + 1]; // alloc WordList PTRs
	int i = 0;
	for(; i < nWL; i++) {
		wl[i] = new WordList(); // (works or THROWS bad_alloc EXCEPTION)
		wl[i]->Set(words[i]);
	}
	wl[i] = 0;
	// call our "internal" folder/lexer (... then do Flush!)
	if(foldOrLex)
		FoldCamlDoc(startPos, length, initStyle, wl, wa);
	else
		ColouriseCamlDoc(startPos, length, initStyle, wl, wa);
	wa.Flush();
	// clean up before leaving
	for(i = nWL - 1; i >= 0; i--)
		delete wl[i];
	delete [] wl;
}

static
#endif  /* BUILD_AS_EXTERNAL_LEXER */

void ColouriseCamlDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList * keywordlists[], Accessor & styler)
{
	// initialize styler
	StyleContext sc(startPos, length, initStyle, styler);
	Sci_PositionU chToken = 0;
	int chBase = 0;
	int chLit = 0;
	WordList & keywords  = *keywordlists[0];
	WordList & keywords2 = *keywordlists[1];
	WordList & keywords3 = *keywordlists[2];
	const bool isSML = keywords.InList("andalso");
	const int useMagic = styler.GetPropertyInt("lexer.caml.magic", 0);
	// set up [initial] state info (terminating states that shouldn't "bleed")
	const int state_ = sc.state & 0x0f;
	if(state_ <= SCE_CAML_CHAR || (isSML && state_ == SCE_CAML_STRING))
		sc.state = SCE_CAML_DEFAULT;
	int nesting = (state_ >= SCE_CAML_COMMENT) ? (state_ - SCE_CAML_COMMENT) : 0;
	// foreach char in range...
	while(sc.More()) {
		// set up [per-char] state info
		int state2 = -1;                                // (ASSUME no state change)
		Sci_Position chColor = sc.currentPos - 1; // (ASSUME standard coloring range)
		bool advance = true;                    // (ASSUME scanner "eats" 1 char)
		// step state machine
		switch(sc.state & 0x0f) {
			case SCE_CAML_DEFAULT:
			    chToken = sc.currentPos;    // save [possible] token start (JIC)
			    // it's wide open; what do we have?
			    if(iscamlf(sc.ch))
				    state2 = SCE_CAML_IDENTIFIER;
			    else if(!isSML && sc.Match('`') && iscamlf(sc.chNext))
				    state2 = SCE_CAML_TAGNAME;
			    else if(!isSML && sc.Match('#') && isdec(sc.chNext))
				    state2 = SCE_CAML_LINENUM;
			    else if(isdec(sc.ch)) {
				    // it's a number, assume base 10
				    state2 = SCE_CAML_NUMBER, chBase = 10;
				    if(sc.Match('0')) {
					    // there MAY be a base specified...
					    const char * baseC = "bBoOxX";
					    if(isSML) {
						    if(sc.chNext == 'w')
							    sc.Forward();  // (consume SML "word" indicator)
						    baseC = "x";
					    }
					    // ... change to specified base AS REQUIRED
					    if(sstrchr(baseC, sc.chNext))
						    chBase = baseT[tolower(sc.chNext) - 'a'], sc.Forward();
				    }
			    }
			    else if(!isSML && sc.Match('\''))   // (Caml char literal?)
				    state2 = SCE_CAML_CHAR, chLit = 0;
			    else if(isSML && sc.Match('#', '"')) // (SML char literal?)
				    state2 = SCE_CAML_CHAR, sc.Forward();
			    else if(sc.Match('"'))
				    state2 = SCE_CAML_STRING;
			    else if(sc.Match('(', '*'))
				    state2 = SCE_CAML_COMMENT, sc.Forward(), sc.ch = ' ';  // (*)...
			    else if(sstrchr("!?~"                /* Caml "prefix-symbol" */
				    "=<>@^|&+-*/$%"                     /* Caml "infix-symbol" */
				    "()[]{};,:.#", sc.ch)       // Caml "bracket" or ;,:.#
			    // SML "extra" ident chars
			   || (isSML && (sc.Match('\\') || sc.Match('`'))))
				    state2 = SCE_CAML_OPERATOR;
			    break;

			case SCE_CAML_IDENTIFIER:
			    // [try to] interpret as [additional] identifier char
			    if(!(iscaml(sc.ch) || sc.Match('\''))) {
				    const Sci_Position n = sc.currentPos - chToken;
				    if(n < 24) {
					    // length is believable as keyword, [re-]construct token
					    char t[24];
					    for(Sci_Position i = -n; i < 0; i++)
						    t[n + i] = static_cast<char>(sc.GetRelative(i));
					    t[n] = '\0';
					    // special-case "_" token as KEYWORD
					    if((n == 1 && sc.chPrev == '_') || keywords.InList(t))
						    sc.ChangeState(SCE_CAML_KEYWORD);
					    else if(keywords2.InList(t))
						    sc.ChangeState(SCE_CAML_KEYWORD2);
					    else if(keywords3.InList(t))
						    sc.ChangeState(SCE_CAML_KEYWORD3);
				    }
				    state2 = SCE_CAML_DEFAULT, advance = false;
			    }
			    break;

			case SCE_CAML_TAGNAME:
			    // [try to] interpret as [additional] tagname char
			    if(!(iscaml(sc.ch) || sc.Match('\'')))
				    state2 = SCE_CAML_DEFAULT, advance = false;
			    break;

			/*case SCE_CAML_KEYWORD:
			   case SCE_CAML_KEYWORD2:
			   case SCE_CAML_KEYWORD3:
			        // [try to] interpret as [additional] keyword char
			        if(!iscaml(ch))
			                state2 = SCE_CAML_DEFAULT, advance = false;
			        break;*/

			case SCE_CAML_LINENUM:
			    // [try to] interpret as [additional] linenum directive char
			    if(!isdec(sc.ch))
				    state2 = SCE_CAML_DEFAULT, advance = false;
			    break;

			case SCE_CAML_OPERATOR: {
			    // [try to] interpret as [additional] operator char
			    const char * o = 0;
			    if(iscaml(sc.ch) || isspace(sc.ch)                  // ident or whitespace
				   || (o = sstrchr(")]};,\'\"#", sc.ch), o) // "termination" chars
				   || (!isSML && sc.Match('`'))                // Caml extra term char
				   || (!sstrchr("!$%&*+-./:<=>?@^|~", sc.ch) // "operator" chars
			                    // SML extra ident chars
					 && !(isSML && (sc.Match('\\') || sc.Match('`'))))) {
				    // check for INCLUSIVE termination
				    if(o && sstrchr(")]};,", sc.ch)) {
					    if((sc.Match(')') && sc.chPrev == '(')
						   || (sc.Match(']') && sc.chPrev == '['))
						    // special-case "()" and "[]" tokens as KEYWORDS
						    sc.ChangeState(SCE_CAML_KEYWORD);
					    chColor++;
				    }
				    else
					    advance = false;
				    state2 = SCE_CAML_DEFAULT;
			    }
			    break;
		    }
			case SCE_CAML_NUMBER:
			    // [try to] interpret as [additional] numeric literal char
			    if((!isSML && sc.Match('_')) || IsADigit(sc.ch, chBase))
				    break;
			    // how about an integer suffix?
			    if(!isSML && (sc.Match('l') || sc.Match('L') || sc.Match('n')) && (sc.chPrev == '_' || IsADigit(sc.chPrev, chBase)))
				    break;
			    // or a floating-point literal?
			    if(chBase == 10) {
				    // with a decimal point?
				    if(sc.Match('.') && ((!isSML && sc.chPrev == '_') || IsADigit(sc.chPrev, chBase)))
					    break;
				    // with an exponent? (I)
				    if((sc.Match('e') || sc.Match('E')) && ((!isSML && (sc.chPrev == '.' || sc.chPrev == '_')) || IsADigit(sc.chPrev, chBase)))
					    break;
				    // with an exponent? (II)
				    if(((!isSML && (sc.Match('+') || sc.Match('-'))) || (isSML && sc.Match('~'))) && (sc.chPrev == 'e' || sc.chPrev == 'E'))
					    break;
			    }
			    // it looks like we have run out of number
			    state2 = SCE_CAML_DEFAULT, advance = false;
			    break;

			case SCE_CAML_CHAR:
			    if(!isSML) {
				    // [try to] interpret as [additional] char literal char
				    if(sc.Match('\\')) {
					    chLit = 1;  // (definitely IS a char literal)
					    if(sc.chPrev == '\\')
						    sc.ch = ' ';  // (...\\')
					    // should we be terminating - one way or another?
				    }
				    else if((sc.Match('\'') && sc.chPrev != '\\')
				   || sc.atLineEnd) {
					    state2 = SCE_CAML_DEFAULT;
					    if(sc.Match('\''))
						    chColor++;
					    else
						    sc.ChangeState(SCE_CAML_IDENTIFIER);
					    // ... maybe a char literal, maybe not
				    }
				    else if(chLit < 1 && sc.currentPos - chToken >= 2)
					    sc.ChangeState(SCE_CAML_IDENTIFIER), advance = false;
				    break;
			    } /* else
			             // @fallthrough for SML char literal (handle like string) */

			case SCE_CAML_STRING:
			    // [try to] interpret as [additional] [SML char/] string literal char
			    if(isSML && sc.Match('\\') && sc.chPrev != '\\' && isspace(sc.chNext))
				    state2 = SCE_CAML_WHITE;
			    else if(sc.Match('\\') && sc.chPrev == '\\')
				    sc.ch = ' ';  // (...\\")
			    // should we be terminating - one way or another?
			    else if((sc.Match('"') && sc.chPrev != '\\')
			   || (isSML && sc.atLineEnd)) {
				    state2 = SCE_CAML_DEFAULT;
				    if(sc.Match('"'))
					    chColor++;
			    }
			    break;

			case SCE_CAML_WHITE:
			    // [try to] interpret as [additional] SML embedded whitespace char
			    if(sc.Match('\\')) {
				    // style this puppy NOW...
				    state2 = SCE_CAML_STRING, sc.ch = ' ' /* (...\") */, chColor++,
				    styler.ColourTo(chColor, SCE_CAML_WHITE), styler.Flush();
				    // ... then backtrack to determine original SML literal type
				    Sci_Position p = chColor - 2;
				    for(; p >= 0 && styler.StyleAt(p) == SCE_CAML_WHITE; p--) ;
				    if(p >= 0)
					    state2 = static_cast<int>(styler.StyleAt(p));
				    // take care of state change NOW
				    sc.ChangeState(state2), state2 = -1;
			    }
			    break;

			case SCE_CAML_COMMENT:
			case SCE_CAML_COMMENT1:
			case SCE_CAML_COMMENT2:
			case SCE_CAML_COMMENT3:
			    // we're IN a comment - does this start a NESTED comment?
			    if(sc.Match('(', '*'))
				    state2 = sc.state + 1, chToken = sc.currentPos,
				    sc.Forward(), sc.ch = ' ' /* (*)... */, nesting++;
			    // [try to] interpret as [additional] comment char
			    else if(sc.Match(')') && sc.chPrev == '*') {
				    if(nesting)
					    state2 = (sc.state & 0x0f) - 1, chToken = 0, nesting--;
				    else
					    state2 = SCE_CAML_DEFAULT;
				    chColor++;
				    // enable "magic" (read-only) comment AS REQUIRED
			    }
			    else if(useMagic && sc.currentPos - chToken == 4
			 && sc.Match('c') && sc.chPrev == 'r' && sc.GetRelative(-2) == '@')
				    sc.state |= 0x10;   // (switch to read-only comment style)
			    break;
		}

		// handle state change and char coloring AS REQUIRED
		if(state2 >= 0)
			styler.ColourTo(chColor, sc.state), sc.ChangeState(state2);
		// move to next char UNLESS re-scanning current char
		if(advance)
			sc.Forward();
	}

	// do any required terminal char coloring (JIC)
	sc.Complete();
}

#ifdef BUILD_AS_EXTERNAL_LEXER
static
#endif  /* BUILD_AS_EXTERNAL_LEXER */
void FoldCamlDoc(Sci_PositionU, Sci_Position,
    int,
    WordList *[],
    Accessor &)
{
}

static const char * const camlWordListDesc[] = {
	"Keywords",             // primary Objective Caml keywords
	"Keywords2",    // "optional" keywords (typically from Pervasives)
	"Keywords3",    // "optional" keywords (typically typenames)
	0
};

#ifndef BUILD_AS_EXTERNAL_LEXER
LexerModule lmCaml(SCLEX_CAML, ColouriseCamlDoc, "caml", FoldCamlDoc, camlWordListDesc);
#endif  /* BUILD_AS_EXTERNAL_LEXER */
