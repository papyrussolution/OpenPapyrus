// Scintilla source code edit control
/** @file StyleContext.h
** Lexer infrastructure.
**/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#ifndef STYLECONTEXT_H
#define STYLECONTEXT_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif
//
// All languages handled so far can treat all characters >= 0x80 as one class
// which just continues the current token or starts an identifier if in default.
// DBCS treated specially as the second character can be < 0x80 and hence
// syntactically significant. UTF-8 avoids this as all trail bytes are >= 0x80
//
class StyleContext {
private:
	LexAccessor & styler;
	IDocumentWithLineEnd * multiByteAccess;
	Sci_PositionU endPos;
	Sci_PositionU lengthDocument;

	// Used for optimizing GetRelativeCharacter
	Sci_PositionU posRelative;
	Sci_PositionU currentPosLastRelative;
	Sci_Position offsetRelative;

	StyleContext & operator = (const StyleContext &);

	void   GetNextChar();
public:
	Sci_PositionU currentPos;
	Sci_Position currentLine;
	Sci_Position lineDocEnd;
	Sci_Position lineStartNext;
	bool atLineStart;
	bool atLineEnd;
	int state;
	int chPrev;
	int ch;
	Sci_Position width;
	int chNext;
	Sci_Position widthNext;

	StyleContext(Sci_PositionU startPos, Sci_PositionU length, int initStyle, LexAccessor &styler_, char chMask = '\377');
	void   Complete();
	bool   More() const;
	void   Forward();
	void   FASTCALL Forward(Sci_Position nb);
	void   FASTCALL ForwardBytes(Sci_Position nb);
	void   FASTCALL ChangeState(int state_);
	void   FASTCALL SetState(int state_);
	void   FASTCALL ForwardSetState(int state_);
	Sci_Position LengthCurrent() const;
	int    FASTCALL GetRelative(Sci_Position n);
	int    FASTCALL GetRelativeCharacter(Sci_Position n);
	bool   FASTCALL Match(char ch0) const;
	bool   FASTCALL Match(char ch0, char ch1) const;
	bool   FASTCALL Match(const char * s);
	// Non-inline
	bool   FASTCALL MatchIgnoreCase(const char * s);
	void   GetCurrent(char * s, Sci_PositionU len);
	void   GetCurrentLowered(char * s, Sci_PositionU len);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
