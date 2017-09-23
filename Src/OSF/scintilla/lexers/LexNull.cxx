// Scintilla source code edit control
/** @file LexNull.cxx
** Lexer for no language. Used for plain text and unrecognized files.
**/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static void ColouriseNullDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler)
{
	// Null language means all style bytes are 0 so just mark the end - no need to fill in.
	if(length > 0) {
		styler.StartAt(startPos + length - 1);
		styler.StartSegment(startPos + length - 1);
		styler.ColourTo(startPos + length - 1, 0);
	}
}

LexerModule lmNull(SCLEX_NULL, ColouriseNullDoc, "null");
