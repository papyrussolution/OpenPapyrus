// Scintilla source code edit control
// @file LexAU3.cxx
// Lexer for AutoIt3  http://www.hiddensoft.com/autoit3
// by Jos van der Zande, jvdzande@yahoo.com
//
// Changes:
// March 28, 2004 - Added the standard Folding code
// April 21, 2004 - Added Preprosessor Table + Syntax Highlighting
//                  Fixed Number highlighting
//                  Changed default isoperator to IsAOperator to have a better match to AutoIt3
//                  Fixed "#comments_start" -> "#comments-start"
//                  Fixed "#comments_end" -> "#comments-end"
//                  Fixed Sendkeys in Strings when not terminated with }
//                  Added support for Sendkey strings that have second parameter e.g. {UP 5} or {a down}
// April 26, 2004 - Fixed # pre-processor statement inside of comment block would invalidly change the color.
//                  Added logic for #include <xyz.au3> to treat the <> as string
//                  Added underscore to IsAOperator.
// May 17, 2004   - Changed the folding logic from indent to keyword folding.
//                  Added Folding logic for blocks of single-commentlines or commentblock.
//                        triggered by: fold.comment=1
//                  Added Folding logic for preprocessor blocks triggered by fold.preprocessor=1
//                  Added Special for #region - #endregion syntax highlight and folding.
// May 30, 2004   - Fixed issue with continuation lines on If statements.
// June 5, 2004   - Added comma to Operators for better readability.
//                  Added fold.compact support set with fold.compact=1
//                  Changed folding inside of #cs-#ce. Default is no keyword folding inside comment blocks when fold.comment=1
//                        it will now only happen when fold.comment=2.
// Sep 5, 2004    - Added logic to handle colourizing words on the last line.
//                        Typed Characters now show as "default" till they match any table.
// Oct 10, 2004   - Added logic to show Comments in "Special" directives.
// Nov  1, 2004   - Added better testing for Numbers supporting x and e notation.
// Nov 28, 2004   - Added logic to handle continuation lines for syntax highlighting.
// Jan 10, 2005   - Added Abbreviations Keyword used for expansion
// Mar 24, 2005   - Updated Abbreviations Keywords to fix when followed by Operator.
// Apr 18, 2005   - Updated #CE/#Comment-End logic to take a linecomment ";" into account
//                - Added folding support for With...EndWith
//                - Added support for a DOT in variable names
//                - Fixed Underscore in CommentBlock
// May 23, 2005   - Fixed the SentKey lexing in case of a missing }
// Aug 11, 2005   - Fixed possible bug with s_save length > 100.
// Aug 23, 2005   - Added Switch/endswitch support to the folding logic.
// Sep 27, 2005   - Fixed the SentKey lexing logic in case of multiple sentkeys.
// Mar 12, 2006   - Fixed issue with <> coloring as String in stead of Operator in rare occasions.
// Apr  8, 2006   - Added support for AutoIt3 Standard UDF library (SCE_AU3_UDF)
// Mar  9, 2007   - Fixed bug with + following a String getting the wrong Color.
// Jun 20, 2007   - Fixed Commentblock issue when LF's are used as EOL.
// Jul 26, 2007   - Fixed #endregion undetected bug.
//
// Copyright for Scintilla: 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
// Scintilla source code edit control

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool FASTCALL IsTypeCharacter(const int ch)
{
	return ch == '$';
}

static bool FASTCALL IsAWordChar(const int ch)
{
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static bool FASTCALL IsAWordStart(const int ch)
{
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '@' || ch == '#' || ch == '$' || ch == '.');
}

static bool FASTCALL IsAOperator(char ch)
{
	if(IsASCII(ch) && isalnum(ch))
		return false;
	if(ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
	    ch == '&' || ch == '^' || ch == '=' || ch == '<' || ch == '>' ||
	    ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == ',')
		return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// GetSendKey() filters the portion before and after a/multiple space(s)
// and return the first portion to be looked-up in the table
// also check if the second portion is valid... (up,down.on.off,toggle or a number)
///////////////////////////////////////////////////////////////////////////////

static int GetSendKey(const char * szLine, char * szKey)
{
	int nFlag   = 0;
	int nStartFound     = 0;
	int nKeyPos = 0;
	int nSpecPos = 0;
	int nSpecNum = 1;
	int nPos    = 0;
	char cTemp;
	char szSpecial[100];

	// split the portion of the sendkey in the part before and after the spaces
	while(((cTemp = szLine[nPos]) != '\0')) {
		// skip leading Ctrl/Shift/Alt state
		if(cTemp == '{') {
			nStartFound = 1;
		}
		//
		if(nStartFound == 1) {
			if((cTemp == ' ') && (nFlag == 0) ) { // get the stuff till first space
				nFlag = 1;
				// Add } to the end of the first bit for table lookup later.
				szKey[nKeyPos++] = '}';
			}
			else if(cTemp == ' ') {
				// skip other spaces
			}
			else if(nFlag == 0) {
				// save first portion into var till space or } is hit
				szKey[nKeyPos++] = cTemp;
			}
			else if((nFlag == 1) && (cTemp != '}')) {
				// Save second portion into var...
				szSpecial[nSpecPos++] = cTemp;
				// check if Second portion is all numbers for repeat fuction
				if(isdec(cTemp) == false) {
					nSpecNum = 0;
				}
			}
		}
		nPos++;                                                                 // skip to next char
	} // End While

	// Check if the second portion is either a number or one of these keywords
	szKey[nKeyPos] = '\0';
	szSpecial[nSpecPos] = '\0';
	if(sstreq(szSpecial, "down") || sstreq(szSpecial, "up") || sstreq(szSpecial, "on") || sstreq(szSpecial, "off") || sstreq(szSpecial, "toggle") || nSpecNum == 1) {
		nFlag = 0;
	}
	else {
		nFlag = 1;
	}
	return nFlag;  // 1 is bad, 0 is good
} // GetSendKey()

//
// Routine to check the last "none comment" character on a line to see if its a continuation
//
static bool IsContinuationLine(Sci_PositionU szLine, Accessor & styler)
{
	Sci_Position nsPos = styler.LineStart(szLine);
	Sci_Position nePos = styler.LineStart(szLine+1) - 2;
	//int stylech = styler.StyleAt(nsPos);
	while(nsPos < nePos) {
		//stylech = styler.StyleAt(nePos);
		int stylech = styler.StyleAt(nsPos);
		if(!(stylech == SCE_AU3_COMMENT)) {
			char ch = styler.SafeGetCharAt(nePos);
			if(!isspacechar(ch)) {
				if(ch == '_')
					return true;
				else
					return false;
			}
		}
		nePos--; // skip to next char
	} // End While
	return false;
} // IsContinuationLine()

//
// syntax highlighting logic
static void ColouriseAU3Doc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList * keywordlists[], Accessor & styler)
{
	WordList &keywords = *keywordlists[0];
	WordList &keywords2 = *keywordlists[1];
	WordList &keywords3 = *keywordlists[2];
	WordList &keywords4 = *keywordlists[3];
	WordList &keywords5 = *keywordlists[4];
	WordList &keywords6 = *keywordlists[5];
	WordList &keywords7 = *keywordlists[6];
	WordList &keywords8 = *keywordlists[7];
	// find the first previous line without continuation character at the end
	Sci_Position lineCurrent = styler.GetLine(startPos);
	Sci_Position s_startPos = startPos;
	// When not inside a Block comment: find First line without _
	if(!(initStyle==SCE_AU3_COMMENTBLOCK)) {
		while((lineCurrent > 0 && IsContinuationLine(lineCurrent, styler)) || (lineCurrent > 1 && IsContinuationLine(lineCurrent-1, styler))) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent); // get start position
			initStyle =  0;                           // reset the start style to 0
		}
	}
	// Set the new length to include it from the start and set the start position
	length = length + s_startPos - startPos;      // correct the total length to process
	styler.StartAt(startPos);

	StyleContext sc(startPos, length, initStyle, styler);
	char s_save[100] = "";
	char si = 0; // string indicator "=1 '=2
	char ni = 0; // Numeric indicator error=9 normal=0 normal+dec=1 hex=2 Enot=3
	char ci = 0; // comment indicator 0=not linecomment(;)
	//$$$
	for(; sc.More(); sc.Forward()) {
		char s[100];
		sc.GetCurrentLowered(s, sizeof(s));
		// **********************************************
		// save the total current word for eof processing
		if(IsAWordChar(sc.ch) || sc.ch == '}') {
			strcpy(s_save, s);
			int tp = static_cast<int>(sstrlen(s_save));
			if(tp < 99) {
				s_save[tp] = static_cast<char>(tolower(sc.ch));
				s_save[tp+1] = '\0';
			}
		}
		// **********************************************
		//
		switch(sc.state) {
			case SCE_AU3_COMMENTBLOCK:
		    {
			    //Reset at line end
			    if(sc.atLineEnd) {
				    ci = 0;
				    if(sstreq(s, "#ce") || sstreq(s, "#comments-end")) {
					    if(sc.atLineEnd)
						    sc.SetState(SCE_AU3_DEFAULT);
					    else
						    sc.SetState(SCE_AU3_COMMENTBLOCK);
				    }
				    break;
			    }
			    //skip rest of line when a ; is encountered
			    if(sc.chPrev == ';') {
				    ci = 2;
				    sc.SetState(SCE_AU3_COMMENTBLOCK);
			    }
			    // skip rest of the line
			    if(ci==2)
				    break;
			    // check when first character is detected on the line
			    if(ci==0) {
				    if(IsAWordStart(static_cast<char>(sc.ch)) || IsAOperator(static_cast<char>(sc.ch))) {
					    ci = 1;
					    sc.SetState(SCE_AU3_COMMENTBLOCK);
				    }
				    break;
			    }
			    if(!(IsAWordChar(sc.ch) || (sc.ch == '-' && sstreq(s, "#comments")))) {
				    if(sstreq(s, "#ce") || sstreq(s, "#comments-end"))
					    sc.SetState(SCE_AU3_COMMENT);              // set to comment line for the rest of the line
				    else
					    ci = 2;    // line doesn't begin with #CE so skip the rest of the line
			    }
			    break;
		    }
			case SCE_AU3_COMMENT:
			    if(sc.atLineEnd) {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_OPERATOR:
			    // check if its a COMobject
			    if(sc.chPrev == '.' && IsAWordChar(sc.ch)) {
				    sc.SetState(SCE_AU3_COMOBJ);
			    }
			    else {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_SPECIAL:
			    if(sc.ch == ';') {
				    sc.SetState(SCE_AU3_COMMENT);
			    }
			    if(sc.atLineEnd) {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_KEYWORD:
			    if(!(IsAWordChar(sc.ch) || (sc.ch == '-' && (sstreq(s, "#comments") || sstreq(s, "#include"))))) {
				    if(!IsTypeCharacter(sc.ch)) {
					    if(sstreq(s, "#cs") || sstreq(s, "#comments-start")) {
						    sc.ChangeState(SCE_AU3_COMMENTBLOCK);
						    sc.SetState(SCE_AU3_COMMENTBLOCK);
						    break;
					    }
					    else if(keywords.InList(s)) {
						    sc.ChangeState(SCE_AU3_KEYWORD);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(keywords2.InList(s)) {
						    sc.ChangeState(SCE_AU3_FUNCTION);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(keywords3.InList(s)) {
						    sc.ChangeState(SCE_AU3_MACRO);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(keywords5.InList(s)) {
						    sc.ChangeState(SCE_AU3_PREPROCESSOR);
						    sc.SetState(SCE_AU3_DEFAULT);
						    if(sstreq(s, "#include")) {
							    si = 3;       // use to determine string start for #inlude <>
						    }
					    }
					    else if(keywords6.InList(s)) {
						    sc.ChangeState(SCE_AU3_SPECIAL);
						    sc.SetState(SCE_AU3_SPECIAL);
					    }
					    else if((keywords7.InList(s)) && (!IsAOperator(static_cast<char>(sc.ch)))) {
						    sc.ChangeState(SCE_AU3_EXPAND);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(keywords8.InList(s)) {
						    sc.ChangeState(SCE_AU3_UDF);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(sstreq(s, "_")) {
						    sc.ChangeState(SCE_AU3_OPERATOR);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
					    else if(!IsAWordChar(sc.ch)) {
						    sc.ChangeState(SCE_AU3_DEFAULT);
						    sc.SetState(SCE_AU3_DEFAULT);
					    }
				    }
			    }
			    if(sc.atLineEnd) {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_NUMBER:
		    {
			    // Numeric indicator error=9 normal=0 normal+dec=1 hex=2 E-not=3
			    //
			    // test for Hex notation
			    if(sstreq(s, "0") && (sc.ch == 'x' || sc.ch == 'X') && ni == 0) {
				    ni = 2;
				    break;
			    }
			    // test for E notation
			    if(IsADigit(sc.chPrev) && (sc.ch == 'e' || sc.ch == 'E') && ni <= 1) {
				    ni = 3;
				    break;
			    }
			    //  Allow Hex characters inside hex numeric strings
			    if((ni == 2) &&
				    (sc.ch == 'a' || sc.ch == 'b' || sc.ch == 'c' || sc.ch == 'd' || sc.ch == 'e' || sc.ch == 'f' ||
					    sc.ch == 'A' || sc.ch == 'B' || sc.ch == 'C' || sc.ch == 'D' || sc.ch == 'E' || sc.ch ==
					    'F' )) {
				    break;
			    }
			    // test for 1 dec point only
			    if(sc.ch == '.') {
				    if(ni==0) {
					    ni = 1;
				    }
				    else {
					    ni = 9;
				    }
				    break;
			    }
			    // end of numeric string ?
			    if(!(IsADigit(sc.ch))) {
				    if(ni==9) {
					    sc.ChangeState(SCE_AU3_DEFAULT);
				    }
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
		    }
			case SCE_AU3_VARIABLE:
			    // Check if its a COMObject
			    if(sc.ch == '.' && !IsADigit(sc.chNext)) {
				    sc.SetState(SCE_AU3_OPERATOR);
			    }
			    else if(!IsAWordChar(sc.ch)) {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_COMOBJ:
			    if(!(IsAWordChar(sc.ch))) {
				    sc.SetState(SCE_AU3_DEFAULT);
			    }
			    break;
			case SCE_AU3_STRING:
			    // check for " to end a double qouted string or
			    // check for ' to end a single qouted string
			    if((si == 1 && sc.ch == '\"') || (si == 2 && sc.ch == '\'') || (si == 3 && sc.ch == '>')) {
				    sc.ForwardSetState(SCE_AU3_DEFAULT);
				    si = 0;
				    break;
			    }
			    if(sc.atLineEnd) {
				    si = 0;
				    // at line end and not found a continuation char then reset to default
				    Sci_Position lineCurrent = styler.GetLine(sc.currentPos);
				    if(!IsContinuationLine(lineCurrent, styler)) {
					    sc.SetState(SCE_AU3_DEFAULT);
					    break;
				    }
			    }
			    // find Sendkeys in a STRING
			    if(sc.ch == '{' || sc.ch == '+' || sc.ch == '!' || sc.ch == '^' || sc.ch == '#') {
				    sc.SetState(SCE_AU3_SENT);
			    }
			    break;
			case SCE_AU3_SENT:
		    {
			    // Send key string ended
			    if(sc.chPrev == '}' && sc.ch != '}') {
				    // set color to SENDKEY when valid sendkey .. else set back to regular string
				    char sk[100];
				    // split {111 222} and return {111} and check if 222 is valid.
				    // if return code = 1 then invalid 222 so must be string
				    if(GetSendKey(s, sk)) {
					    sc.ChangeState(SCE_AU3_STRING);
				    }
				    // if single char between {?} then its ok as sendkey for a single character
				    else if(sstrlen(sk) == 3) {
					    sc.ChangeState(SCE_AU3_SENT);
				    }
				    // if sendkey {111} is in table then ok as sendkey
				    else if(keywords4.InList(sk)) {
					    sc.ChangeState(SCE_AU3_SENT);
				    }
				    else {
					    sc.ChangeState(SCE_AU3_STRING);
				    }
				    sc.SetState(SCE_AU3_STRING);
			    }
			    else {
				    // check if the start is a valid SendKey start
				    Sci_Position nPos    = 0;
				    int nState  = 1;
				    char cTemp;
				    while(!(nState == 2) && ((cTemp = s[nPos]) != '\0')) {
					    if(cTemp == '{' && nState == 1) {
						    nState = 2;
					    }
					    if(nState == 1 && !(cTemp == '+' || cTemp == '!' || cTemp == '^' || cTemp == '#' )) {
						    nState = 0;
					    }
					    nPos++;
				    }
				    //Verify characters infront of { ... if not assume  regular string
				    if(nState == 1 && (!(sc.ch == '{' || sc.ch == '+' || sc.ch == '!' || sc.ch == '^' || sc.ch == '#' ))) {
					    sc.ChangeState(SCE_AU3_STRING);
					    sc.SetState(SCE_AU3_STRING);
				    }
				    // If invalid character found then assume its a regular string
				    if(nState == 0) {
					    sc.ChangeState(SCE_AU3_STRING);
					    sc.SetState(SCE_AU3_STRING);
				    }
			    }
			    // check if next portion is again a sendkey
			    if(sc.atLineEnd) {
				    sc.ChangeState(SCE_AU3_STRING);
				    sc.SetState(SCE_AU3_DEFAULT);
				    si = 0;      // reset string indicator
			    }
			    //* check in next characters following a sentkey are again a sent key
			    // Need this test incase of 2 sentkeys like {F1}{ENTER} but not detect {{}
			    if(sc.state == SCE_AU3_STRING &&
				    (sc.ch == '{' || sc.ch == '+' || sc.ch == '!' || sc.ch == '^' || sc.ch == '#' )) {
				    sc.SetState(SCE_AU3_SENT);
			    }
			    // check to see if the string ended...
			    // Sendkey string isn't complete but the string ended....
			    if((si == 1 && sc.ch == '\"') || (si == 2 && sc.ch == '\'')) {
				    sc.ChangeState(SCE_AU3_STRING);
				    sc.ForwardSetState(SCE_AU3_DEFAULT);
			    }
			    break;
		    }
		} //switch (sc.state)

		// Determine if a new state should be entered:

		if(sc.state == SCE_AU3_DEFAULT) {
			if(sc.ch == ';')
				sc.SetState(SCE_AU3_COMMENT);
			else if(sc.ch == '#')
				sc.SetState(SCE_AU3_KEYWORD);
			else if(sc.ch == '$')
				sc.SetState(SCE_AU3_VARIABLE);
			else if(sc.ch == '.' && !IsADigit(sc.chNext))
				sc.SetState(SCE_AU3_OPERATOR);
			else if(sc.ch == '@')
				sc.SetState(SCE_AU3_KEYWORD);
			//else if (sc.ch == '_') {sc.SetState(SCE_AU3_KEYWORD);}
			else if(sc.ch == '<' && si==3)
				sc.SetState(SCE_AU3_STRING); // string after #include
			else if(sc.ch == '\"') {
				sc.SetState(SCE_AU3_STRING);
				si = 1;
			}
			else if(sc.ch == '\'') {
				sc.SetState(SCE_AU3_STRING);
				si = 2;
			}
			else if(IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				sc.SetState(SCE_AU3_NUMBER);
				ni = 0;
			}
			else if(IsAWordStart(sc.ch))
				sc.SetState(SCE_AU3_KEYWORD);
			else if(IsAOperator(static_cast<char>(sc.ch)))
				sc.SetState(SCE_AU3_OPERATOR);
			else if(sc.atLineEnd)
				sc.SetState(SCE_AU3_DEFAULT);
		}
	}  //for (; sc.More(); sc.Forward())

	//*************************************
	// Colourize the last word correctly
	//*************************************
	if(sc.state == SCE_AU3_KEYWORD) {
		if(sstreq(s_save, "#cs") || sstreq(s_save, "#comments-start")) {
			sc.ChangeState(SCE_AU3_COMMENTBLOCK);
			sc.SetState(SCE_AU3_COMMENTBLOCK);
		}
		else if(keywords.InList(s_save)) {
			sc.ChangeState(SCE_AU3_KEYWORD);
			sc.SetState(SCE_AU3_KEYWORD);
		}
		else if(keywords2.InList(s_save)) {
			sc.ChangeState(SCE_AU3_FUNCTION);
			sc.SetState(SCE_AU3_FUNCTION);
		}
		else if(keywords3.InList(s_save)) {
			sc.ChangeState(SCE_AU3_MACRO);
			sc.SetState(SCE_AU3_MACRO);
		}
		else if(keywords5.InList(s_save)) {
			sc.ChangeState(SCE_AU3_PREPROCESSOR);
			sc.SetState(SCE_AU3_PREPROCESSOR);
		}
		else if(keywords6.InList(s_save)) {
			sc.ChangeState(SCE_AU3_SPECIAL);
			sc.SetState(SCE_AU3_SPECIAL);
		}
		else if(keywords7.InList(s_save) && sc.atLineEnd) {
			sc.ChangeState(SCE_AU3_EXPAND);
			sc.SetState(SCE_AU3_EXPAND);
		}
		else if(keywords8.InList(s_save)) {
			sc.ChangeState(SCE_AU3_UDF);
			sc.SetState(SCE_AU3_UDF);
		}
		else {
			sc.ChangeState(SCE_AU3_DEFAULT);
			sc.SetState(SCE_AU3_DEFAULT);
		}
	}
	if(sc.state == SCE_AU3_SENT) {
		// Send key string ended
		if(sc.chPrev == '}' && sc.ch != '}') {
			// set color to SENDKEY when valid sendkey .. else set back to regular string
			char sk[100];
			// split {111 222} and return {111} and check if 222 is valid.
			// if return code = 1 then invalid 222 so must be string
			if(GetSendKey(s_save, sk))
				sc.ChangeState(SCE_AU3_STRING);
			// if single char between {?} then its ok as sendkey for a single character
			else if(sstrlen(sk) == 3)
				sc.ChangeState(SCE_AU3_SENT);
			// if sendkey {111} is in table then ok as sendkey
			else if(keywords4.InList(sk))
				sc.ChangeState(SCE_AU3_SENT);
			else
				sc.ChangeState(SCE_AU3_STRING);
			sc.SetState(SCE_AU3_STRING);
		}
		// check if next portion is again a sendkey
		if(sc.atLineEnd) {
			sc.ChangeState(SCE_AU3_STRING);
			sc.SetState(SCE_AU3_DEFAULT);
		}
	}
	//*************************************
	sc.Complete();
}

//
static bool FASTCALL IsStreamCommentStyle(int style)
{
	return oneof2(style, SCE_AU3_COMMENT, SCE_AU3_COMMENTBLOCK);
}

//
// Routine to find first none space on the current line and return its Style
// needed for comment lines not starting on pos 1
static int GetStyleFirstWord(Sci_PositionU szLine, Accessor & styler)
{
	Sci_Position nsPos = styler.LineStart(szLine);
	Sci_Position nePos = styler.LineStart(szLine+1) - 1;
	while(isspacechar(styler.SafeGetCharAt(nsPos)) && nsPos < nePos) {
		nsPos++; // skip to next char
	} // End While
	return styler.StyleAt(nsPos);
} // GetStyleFirstWord()

//
static void FoldAU3Doc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor & styler)
{
	Sci_Position endPos = startPos + length;
	// get settings from the config files for folding comments and preprocessor lines
	bool foldComment = styler.GetPropertyInt("fold.comment") != 0;
	bool foldInComment = styler.GetPropertyInt("fold.comment") == 2;
	bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldpreprocessor = styler.GetPropertyInt("fold.preprocessor") != 0;
	// Backtrack to previous line in case need to fix its fold status
	Sci_Position lineCurrent = styler.GetLine(startPos);
	if(startPos > 0) {
		if(lineCurrent > 0) {
			lineCurrent--;
			startPos = styler.LineStart(lineCurrent);
		}
	}
	// vars for style of previous/current/next lines
	int style = GetStyleFirstWord(lineCurrent, styler);
	int stylePrev = 0;
	// find the first previous line without continuation character at the end
	while((lineCurrent > 0 && IsContinuationLine(lineCurrent, styler)) ||
	    (lineCurrent > 1 && IsContinuationLine(lineCurrent-1, styler))) {
		lineCurrent--;
		startPos = styler.LineStart(lineCurrent);
	}
	if(lineCurrent > 0) {
		stylePrev = GetStyleFirstWord(lineCurrent-1, styler);
	}
	// vars for getting first word to check for keywords
	bool FirstWordStart = false;
	bool FirstWordEnd = false;
	char szKeyword[11] = "";
	int szKeywordlen = 0;
	char szThen[5] = "";
	int szThenlen = 0;
	bool ThenFoundLast = false;
	// var for indentlevel
	int levelCurrent = SC_FOLDLEVELBASE;
	if(lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelNext = levelCurrent;
	//
	int visibleChars = 0;
	char chNext = styler.SafeGetCharAt(startPos);
	char chPrev = ' ';
	//
	for(Sci_Position i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		if(IsAWordChar(ch)) {
			visibleChars++;
		}
		// get the syle for the current character neede to check in comment
		int stylech = styler.StyleAt(i);
		// get first word for the line for indent check max 9 characters
		if(FirstWordStart && (!(FirstWordEnd))) {
			if(!IsAWordChar(ch)) {
				FirstWordEnd = true;
				szKeyword[szKeywordlen] = '\0';
			}
			else {
				if(szKeywordlen < 10) {
					szKeyword[szKeywordlen++] = static_cast<char>(tolower(ch));
				}
			}
		}
		// start the capture of the first word
		if(!(FirstWordStart)) {
			if(IsAWordChar(ch) || IsAWordStart(ch) || ch == ';') {
				FirstWordStart = true;
				szKeyword[szKeywordlen++] = static_cast<char>(tolower(ch));
			}
		}
		// only process this logic when not in comment section
		if(!(stylech == SCE_AU3_COMMENT)) {
			if(ThenFoundLast) {
				if(IsAWordChar(ch)) {
					ThenFoundLast = false;
				}
			}
			// find out if the word "then" is the last on a "if" line
			if(FirstWordEnd && sstreq(szKeyword, "if")) {
				if(szThenlen == 4) {
					szThen[0] = szThen[1];
					szThen[1] = szThen[2];
					szThen[2] = szThen[3];
					szThen[3] = static_cast<char>(tolower(ch));
					if(sstreq(szThen, "then")) {
						ThenFoundLast = true;
					}
				}
				else {
					szThen[szThenlen++] = static_cast<char>(tolower(ch));
					if(szThenlen == 5) {
						szThen[4] = '\0';
					}
				}
			}
		}
		// End of Line found so process the information
		if((ch == '\r' && chNext != '\n') || (ch == '\n') || (i == endPos)) {
			// **************************
			// Folding logic for Keywords
			// **************************
			// if a keyword is found on the current line and the line doesn't end with _ (continuation)
			//    and we are not inside a commentblock.
			if(szKeywordlen > 0 && (!(chPrev == '_')) && ((!(IsStreamCommentStyle(style)) || foldInComment)) ) {
				szKeyword[szKeywordlen] = '\0';
				// only fold "if" last keyword is "then"  (else its a one line if)
				if(sstreq(szKeyword, "if") && ThenFoundLast) {
					levelNext++;
				}
				// create new fold for these words
				if(sstreq(szKeyword, "do") || sstreq(szKeyword, "for") || sstreq(szKeyword, "func") || sstreq(szKeyword, "while") || sstreq(szKeyword, "with") || sstreq(szKeyword, "#region")) {
					levelNext++;
				}
				// create double Fold for select&switch because Case will subtract one of the current level
				if(sstreq(szKeyword, "select") || sstreq(szKeyword, "switch")) {
					levelNext++;
					levelNext++;
				}
				// end the fold for these words before the current line
				if(sstreq(szKeyword, "endfunc") || sstreq(szKeyword, "endif") || sstreq(szKeyword, "next") || sstreq(szKeyword, "until") || sstreq(szKeyword, "endwith") || sstreq(szKeyword, "wend")) {
					levelNext--;
					levelCurrent--;
				}
				// end the fold for these words before the current line and Start new fold
				if(sstreq(szKeyword, "case") || sstreq(szKeyword, "else") || sstreq(szKeyword, "elseif")) {
					levelCurrent--;
				}
				// end the double fold for this word before the current line
				if(sstreq(szKeyword, "endselect") || sstreq(szKeyword, "endswitch")) {
					levelNext--;
					levelNext--;
					levelCurrent--;
					levelCurrent--;
				}
				// end the fold for these words on the current line
				if(sstreq(szKeyword, "#endregion")) {
					levelNext--;
				}
			}
			// Preprocessor and Comment folding
			int styleNext = GetStyleFirstWord(lineCurrent + 1, styler);
			// *************************************
			// Folding logic for preprocessor blocks
			// *************************************
			// process preprosessor line
			if(foldpreprocessor && style == SCE_AU3_PREPROCESSOR) {
				if(!(stylePrev == SCE_AU3_PREPROCESSOR) && (styleNext == SCE_AU3_PREPROCESSOR)) {
					levelNext++;
				}
				// fold till the last line for normal comment lines
				else if(stylePrev == SCE_AU3_PREPROCESSOR && !(styleNext == SCE_AU3_PREPROCESSOR)) {
					levelNext--;
				}
			}
			// *********************************
			// Folding logic for Comment blocks
			// *********************************
			if(foldComment && IsStreamCommentStyle(style)) {
				// Start of a comment block
				if(!(stylePrev==style) && IsStreamCommentStyle(styleNext) && styleNext==style) {
					levelNext++;
				}
				// fold till the last line for normal comment lines
				else if(IsStreamCommentStyle(stylePrev) && !(styleNext == SCE_AU3_COMMENT) && stylePrev == SCE_AU3_COMMENT && style == SCE_AU3_COMMENT) {
					levelNext--;
				}
				// fold till the one but last line for Blockcomment lines
				else if(IsStreamCommentStyle(stylePrev) && !(styleNext == SCE_AU3_COMMENTBLOCK) && style == SCE_AU3_COMMENTBLOCK) {
					levelNext--;
					levelCurrent--;
				}
			}
			int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if(visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if(levelUse < levelNext) {
				lev |= SC_FOLDLEVELHEADERFLAG;
			}
			if(lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			// reset values for the next line
			lineCurrent++;
			stylePrev = style;
			style = styleNext;
			levelCurrent = levelNext;
			visibleChars = 0;
			// if the last character is an Underscore then don't reset since the line continues on the next line.
			if(!(chPrev == '_')) {
				szKeywordlen = 0;
				szThenlen = 0;
				FirstWordStart = false;
				FirstWordEnd = false;
				ThenFoundLast = false;
			}
		}
		// save the last processed character
		if(!isspacechar(ch)) {
			chPrev = ch;
			visibleChars++;
		}
	}
}

//

static const char * const AU3WordLists[] = {
	"#autoit keywords",
	"#autoit functions",
	"#autoit macros",
	"#autoit Sent keys",
	"#autoit Pre-processors",
	"#autoit Special",
	"#autoit Expand",
	"#autoit UDF",
	0
};
LexerModule lmAU3(SCLEX_AU3, ColouriseAU3Doc, "au3", FoldAU3Doc, AU3WordLists);
