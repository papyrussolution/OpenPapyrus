// Scintilla source code edit control
/** @file LexDMAP.cxx
** Lexer for MSC Nastran DMAP.
** Written by Mark Robinson, based on the Fortran lexer by Chuan-jian Shen, Last changed Aug. 2013
**/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
// 
#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool FASTCALL IsAWordChar(int ch) { return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '%'); }
static bool FASTCALL IsAWordStart(const int ch) { return (ch < 0x80) && (isalnum(ch)); }

static void ColouriseDMAPDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList * keywordlists[], Accessor & styler) 
{
	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	Sci_Position posLineStart = 0;
	Sci_Position numNonBlank = 0;
	Sci_Position endPos = startPos + length;
	// backtrack to the nearest keyword
	while((startPos > 1) && (styler.StyleAt(startPos) != SCE_DMAP_WORD)) {
		startPos--;
	}
	startPos = styler.LineStart(styler.GetLine(startPos));
	initStyle = styler.StyleAt(startPos - 1);
	StyleContext sc(startPos, endPos-startPos, initStyle, styler);
	for(; sc.More(); sc.Forward()) {
		// remember the start position of the line
		if(sc.atLineStart) {
			posLineStart = sc.currentPos;
			numNonBlank = 0;
			sc.SetState(SCE_DMAP_DEFAULT);
		}
		if(!IsASpaceOrTab(sc.ch)) numNonBlank++;
		// Handle data appearing after column 72; it is ignored
		Sci_Position toLineStart = sc.currentPos - posLineStart;
		if(toLineStart >= 72 || sc.ch == '$') {
			sc.SetState(SCE_DMAP_COMMENT);
			while(!sc.atLineEnd && sc.More()) sc.Forward();  // Until line end
			continue;
		}
		// Determine if the current state should terminate.
		if(sc.state == SCE_DMAP_OPERATOR) {
			sc.SetState(SCE_DMAP_DEFAULT);
		}
		else if(sc.state == SCE_DMAP_NUMBER) {
			if(!(IsAWordChar(sc.ch) || sc.ch=='\'' || sc.ch=='\"' || sc.ch=='.')) {
				sc.SetState(SCE_DMAP_DEFAULT);
			}
		}
		else if(sc.state == SCE_DMAP_IDENTIFIER) {
			if(!IsAWordChar(sc.ch) || (sc.ch == '%')) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if(keywords.InList(s)) {
					sc.ChangeState(SCE_DMAP_WORD);
				}
				else if(keywords2.InList(s)) {
					sc.ChangeState(SCE_DMAP_WORD2);
				}
				else if(keywords3.InList(s)) {
					sc.ChangeState(SCE_DMAP_WORD3);
				}
				sc.SetState(SCE_DMAP_DEFAULT);
			}
		}
		else if(sc.state == SCE_DMAP_COMMENT) {
			if(sc.ch == '\r' || sc.ch == '\n') {
				sc.SetState(SCE_DMAP_DEFAULT);
			}
		}
		else if(sc.state == SCE_DMAP_STRING1) {
			if(sc.ch == '\'') {
				if(sc.chNext == '\'') {
					sc.Forward();
				}
				else {
					sc.ForwardSetState(SCE_DMAP_DEFAULT);
				}
			}
			else if(sc.atLineEnd) {
				sc.ChangeState(SCE_DMAP_STRINGEOL);
				sc.ForwardSetState(SCE_DMAP_DEFAULT);
			}
		}
		else if(sc.state == SCE_DMAP_STRING2) {
			if(sc.atLineEnd) {
				sc.ChangeState(SCE_DMAP_STRINGEOL);
				sc.ForwardSetState(SCE_DMAP_DEFAULT);
			}
			else if(sc.ch == '\"') {
				if(sc.chNext == '\"') {
					sc.Forward();
				}
				else {
					sc.ForwardSetState(SCE_DMAP_DEFAULT);
				}
			}
		}
		// Determine if a new state should be entered.
		if(sc.state == SCE_DMAP_DEFAULT) {
			if(sc.ch == '$') {
				sc.SetState(SCE_DMAP_COMMENT);
			}
			else if(isdec(sc.ch) || (sc.ch == '.' && isdec(sc.chNext)) || (sc.ch == '-' && isdec(sc.chNext))) {
				sc.SetState(SCE_F_NUMBER);
			}
			else if(IsAWordStart(sc.ch)) {
				sc.SetState(SCE_DMAP_IDENTIFIER);
			}
			else if(sc.ch == '\"') {
				sc.SetState(SCE_DMAP_STRING2);
			}
			else if(sc.ch == '\'') {
				sc.SetState(SCE_DMAP_STRING1);
			}
			else if(isoperator(static_cast<char>(sc.ch))) {
				sc.SetState(SCE_DMAP_OPERATOR);
			}
		}
	}
	sc.Complete();
}
//
// To determine the folding level depending on keywords
//
static int classifyFoldPointDMAP(const char * s, const char * prevWord) 
{
	int lev = 0;
	if((sstreq(prevWord, "else") && sstreq(s, "if")) || sstreq(s, "enddo") || sstreq(s, "endif")) {
		lev = -1;
	}
	else if((sstreq(prevWord, "do") && sstreq(s, "while")) || sstreq(s, "then")) {
		lev = 1;
	}
	return lev;
}

// Folding the code
static void FoldDMAPDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *[], Accessor & styler) 
{
	//
	// bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	// Do not know how to fold the comment at the moment.
	//
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
	int levelCurrent = levelPrev;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	Sci_Position lastStart = 0;
	char prevWord[32] = "";
	for(Sci_PositionU i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		//
		if((stylePrev == SCE_DMAP_DEFAULT || stylePrev == SCE_DMAP_OPERATOR ||
			    stylePrev == SCE_DMAP_COMMENT) && (style == SCE_DMAP_WORD)) {
			// Store last word and label start point.
			lastStart = i;
		}
		if(style == SCE_DMAP_WORD) {
			if(iswordchar(ch) && !iswordchar(chNext)) {
				char s[32];
				Sci_PositionU k;
				for(k = 0; (k<31 ) && (k<i-lastStart+1 ); k++) {
					s[k] = static_cast<char>(tolower(styler[lastStart+k]));
				}
				s[k] = '\0';
				levelCurrent += classifyFoldPointDMAP(s, prevWord);
				strcpy(prevWord, s);
			}
		}
		if(atEOL) {
			int lev = levelPrev;
			if(visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if((levelCurrent > levelPrev) && (visibleChars > 0))
				lev |= SC_FOLDLEVELHEADERFLAG;
			if(lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelPrev = levelCurrent;
			visibleChars = 0;
			strcpy(prevWord, "");
		}
		if(!isspacechar(ch)) 
			visibleChars++;
	}
	// Fill in the real level of the next line, keeping the current flags as they will be filled in later
	int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
	styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const DMAPWordLists[] = {
	"Primary keywords and identifiers",
	"Intrinsic functions",
	"Extended and user defined functions",
	0,
};

LexerModule lmDMAP(SCLEX_DMAP, ColouriseDMAPDoc, "DMAP", FoldDMAPDoc, DMAPWordLists);
