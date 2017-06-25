// Scintilla source code edit control
/** @file StyleContext.cxx
** Lexer infrastructure.
**/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
//#include "ILexer.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"

#ifdef SCI_NAMESPACE
	using namespace Scintilla;
#endif

bool StyleContext::MatchIgnoreCase(const char * s)
{
	if(MakeLowerCase(ch) != static_cast<uchar>(*s))
		return false;
	s++;
	if(MakeLowerCase(chNext) != static_cast<uchar>(*s))
		return false;
	s++;
	for(int n = 2; *s; n++) {
		if(static_cast<uchar>(*s) !=
		    MakeLowerCase(static_cast<uchar>(styler.SafeGetCharAt(currentPos + n, 0))))
			return false;
		s++;
	}
	return true;
}

static void getRange(Sci_PositionU start, Sci_PositionU end, LexAccessor &styler, char * s, Sci_PositionU len)
{
	Sci_PositionU i = 0;
	while((i < end - start + 1) && (i < len-1)) {
		s[i] = styler[start + i];
		i++;
	}
	s[i] = '\0';
}

void StyleContext::GetCurrent(char * s, Sci_PositionU len)
{
	getRange(styler.GetStartSegment(), currentPos - 1, styler, s, len);
}

static void getRangeLowered(Sci_PositionU start, Sci_PositionU end, LexAccessor &styler, char * s, Sci_PositionU len)
{
	Sci_PositionU i = 0;
	while((i < end - start + 1) && (i < len-1)) {
		s[i] = static_cast<char>(tolower(styler[start + i]));
		i++;
	}
	s[i] = '\0';
}

void StyleContext::GetCurrentLowered(char * s, Sci_PositionU len)
{
	getRangeLowered(styler.GetStartSegment(), currentPos - 1, styler, s, len);
}
//
//
//
StyleContext::StyleContext(Sci_PositionU startPos, Sci_PositionU length, int initStyle, LexAccessor &styler_, char chMask /*= '\377'*/) :
	styler(styler_), multiByteAccess(0), endPos(startPos + length), posRelative(0),
	currentPosLastRelative(0x7FFFFFFF), offsetRelative(0), currentPos(startPos), currentLine(-1), lineStartNext(-1),
	atLineEnd(false), state(initStyle & chMask), // Mask off all bits which aren't in the chMask.
	chPrev(0), ch(0), width(0), chNext(0), widthNext(1)
{
	if(styler.Encoding() != enc8bit) {
		multiByteAccess = styler.MultiByteAccess();
	}
	styler.StartAt(startPos /*, chMask*/);
	styler.StartSegment(startPos);
	currentLine = styler.GetLine(startPos);
	lineStartNext = styler.LineStart(currentLine+1);
	lengthDocument = static_cast<Sci_PositionU>(styler.Length());
	if(endPos == lengthDocument)
		endPos++;
	lineDocEnd = styler.GetLine(lengthDocument);
	atLineStart = static_cast<Sci_PositionU>(styler.LineStart(currentLine)) == startPos;

	// Variable width is now 0 so GetNextChar gets the char at currentPos into chNext/widthNext
	width = 0;
	GetNextChar();
	ch = chNext;
	width = widthNext;

	GetNextChar();
}

void StyleContext::Complete()
{
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	styler.Flush();
}

bool StyleContext::More() const
{
	return currentPos < endPos;
}

void StyleContext::Forward()
{
	if(currentPos < endPos) {
		atLineStart = atLineEnd;
		if(atLineStart) {
			currentLine++;
			lineStartNext = styler.LineStart(currentLine+1);
		}
		chPrev = ch;
		currentPos += width;
		ch = chNext;
		width = widthNext;
		GetNextChar();
	}
	else {
		atLineStart = false;
		chPrev = ' ';
		ch = ' ';
		chNext = ' ';
		atLineEnd = true;
	}
}

void FASTCALL StyleContext::Forward(Sci_Position nb)
{
	for(Sci_Position i = 0; i < nb; i++) {
		Forward();
	}
}

void FASTCALL StyleContext::ForwardBytes(Sci_Position nb)
{
	Sci_PositionU forwardPos = currentPos + nb;
	while(forwardPos > currentPos) {
		Forward();
	}
}

void FASTCALL StyleContext::ChangeState(int state_)
{
	state = state_;
}

void FASTCALL StyleContext::SetState(int state_)
{
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	state = state_;
}

void FASTCALL StyleContext::ForwardSetState(int state_)
{
	Forward();
	styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
	state = state_;
}

Sci_Position StyleContext::LengthCurrent() const
{
	return currentPos - styler.GetStartSegment();
}

int FASTCALL StyleContext::GetRelative(Sci_Position n)
{
	return static_cast<uchar>(styler.SafeGetCharAt(currentPos+n, 0));
}

int FASTCALL StyleContext::GetRelativeCharacter(Sci_Position n)
{
	if(n == 0)
		return ch;
	if(multiByteAccess) {
		if((currentPosLastRelative != currentPos) ||
			((n > 0) && ((offsetRelative < 0) || (n < offsetRelative))) ||
			((n < 0) && ((offsetRelative > 0) || (n > offsetRelative)))) {
			posRelative = currentPos;
			offsetRelative = 0;
		}
		Sci_Position diffRelative = n - offsetRelative;
		Sci_Position posNew = multiByteAccess->GetRelativePosition(posRelative, diffRelative);
		int chReturn = multiByteAccess->GetCharacterAndWidth(posNew, 0);
		posRelative = posNew;
		currentPosLastRelative = currentPos;
		offsetRelative = n;
		return chReturn;
	}
	else {
		// fast version for single byte encodings
		return static_cast<uchar>(styler.SafeGetCharAt(currentPos + n, 0));
	}
}

bool FASTCALL StyleContext::Match(char ch0) const
{
	return ch == static_cast<uchar>(ch0);
}

bool FASTCALL StyleContext::Match(char ch0, char ch1) const
{
	return (ch == static_cast<uchar>(ch0)) && (chNext == static_cast<uchar>(ch1));
}

bool FASTCALL StyleContext::Match(const char * s)
{
	if(ch != static_cast<uchar>(*s))
		return false;
	s++;
	if(!*s)
		return true;
	if(chNext != static_cast<uchar>(*s))
		return false;
	s++;
	for(int n = 2; *s; n++) {
		if(*s != styler.SafeGetCharAt(currentPos+n, 0))
			return false;
		s++;
	}
	return true;
}

void StyleContext::GetNextChar()
{
	if(multiByteAccess) {
		chNext = multiByteAccess->GetCharacterAndWidth(currentPos+width, &widthNext);
	}
	else {
		chNext = static_cast<uchar>(styler.SafeGetCharAt(currentPos+width, 0));
		widthNext = 1;
	}
	// End of line determined from line end position, allowing CR, LF,
	// CRLF and Unicode line ends as set by document.
	if(currentLine < lineDocEnd)
		atLineEnd = static_cast<Sci_Position>(currentPos) >= (lineStartNext-1);
	else // Last line
		atLineEnd = static_cast<Sci_Position>(currentPos) >= lineStartNext;
}
