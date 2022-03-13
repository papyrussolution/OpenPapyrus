// Scintilla source code edit control
/** @file Accessor.cxx
** Interfaces between Scintilla and lexers.
**/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif
//
// LexAccessor
//
void FASTCALL LexAccessor::Fill(Sci_Position position)
{
	startPos = position - slopSize;
	if((startPos + bufferSize) > lenDoc)
		startPos = lenDoc - bufferSize;
	SETMAX(startPos, 0);
	endPos = startPos + bufferSize;
	SETMIN(endPos, lenDoc);
	pAccess->GetCharRange(buf, startPos, endPos-startPos);
	buf[endPos-startPos] = '\0';
}

LexAccessor::LexAccessor(IDocument * pAccess_) : pAccess(pAccess_), startPos(extremePosition), endPos(0),
	codePage(pAccess->CodePage()), encodingType(enc8bit), lenDoc(pAccess->Length()),
	validLen(0), startSeg(0), startPosStyling(0), documentVersion(pAccess->Version())
{
	// Prevent warnings by static analyzers about uninitialized buf and styleBuf.
	buf[0] = 0;
	styleBuf[0] = 0;
	switch(codePage) {
		case 65001:
			encodingType = encUnicode;
			break;
		case 932:
		case 936:
		case 949:
		case 950:
		case 1361:
			encodingType = encDBCS;
	}
}

char FASTCALL LexAccessor::operator[] (Sci_Position position)
{
	if(position < startPos || position >= endPos)
		Fill(position);
	return buf[position - startPos];
}

IDocumentWithLineEnd * LexAccessor::MultiByteAccess() const
{
	return (documentVersion >= dvLineEnd) ? static_cast<IDocumentWithLineEnd *>(pAccess) : 0;
}
//
// Safe version of operator[], returning a defined value for invalid position
//
char FASTCALL LexAccessor::SafeGetCharAt(Sci_Position position, char chDefault)
{
	if(position < startPos || position >= endPos) {
		Fill(position);
		if(position < startPos || position >= endPos)
			return chDefault; // Position is outside range of document
	}
	return buf[position - startPos];
}

char FASTCALL LexAccessor::SafeGetCharAt(Sci_Position position)
{
	if(position < startPos || position >= endPos) {
		Fill(position);
		if(position < startPos || position >= endPos)
			return ' '; // Position is outside range of document
	}
	return buf[position - startPos];
}

bool LexAccessor::Match(Sci_Position pos, const char * s)
{
	for(int i = 0; *s; i++) {
		if(*s != SafeGetCharAt(pos+i))
			return false;
		s++;
	}
	return true;
}

bool FASTCALL LexAccessor::IsLeadByte(char ch) const { return pAccess->IsDBCSLeadByte(ch); }
EncodingType LexAccessor::Encoding() const { return encodingType; }
char FASTCALL LexAccessor::StyleAt(Sci_Position position) const { return static_cast<char>(pAccess->StyleAt(position)); }
Sci_Position FASTCALL LexAccessor::GetLine(Sci_Position position) const { return pAccess->LineFromPosition(position); }
Sci_Position FASTCALL LexAccessor::LineStart(Sci_Position line) const { return pAccess->LineStart(line); }
int FASTCALL LexAccessor::LevelAt(Sci_Position line) const { return pAccess->GetLevel(line); }
Sci_Position LexAccessor::Length() const { return lenDoc; }
int FASTCALL LexAccessor::GetLineState(Sci_Position line) const { return pAccess->GetLineState(line); }
int LexAccessor::SetLineState(Sci_Position line, int state) { return pAccess->SetLineState(line, state); }
Sci_PositionU LexAccessor::GetStartSegment() const { return startSeg; }
void FASTCALL LexAccessor::StartSegment(Sci_PositionU pos) { startSeg = pos; }

Sci_Position FASTCALL LexAccessor::LineEnd(Sci_Position line)
{
	if(documentVersion >= dvLineEnd) {
		return (static_cast<IDocumentWithLineEnd *>(pAccess))->LineEnd(line);
	}
	else {
		// Old interface means only '\r', '\n' and '\r\n' line ends.
		Sci_Position startNext = pAccess->LineStart(line+1);
		char chLineEnd = SafeGetCharAt(startNext-1);
		return (chLineEnd == '\n' && (SafeGetCharAt(startNext-2)  == '\r')) ? (startNext - 2) : (startNext - 1);
	}
}

void LexAccessor::Flush()
{
	if(validLen > 0) {
		pAccess->SetStyles(validLen, styleBuf);
		startPosStyling += validLen;
		validLen = 0;
	}
}

// Style setting
void FASTCALL LexAccessor::StartAt(Sci_PositionU start)
{
	pAccess->StartStyling(start, '\377');
	startPosStyling = start;
}

void LexAccessor::ColourTo(Sci_PositionU pos, int chAttr)
{
	// Only perform styling if non empty range
	if(pos != startSeg - 1) {
		assert(pos >= startSeg);
		if(pos < startSeg) {
			return;
		}
		if(validLen + (pos - startSeg + 1) >= bufferSize)
			Flush();
		if(validLen + (pos - startSeg + 1) >= bufferSize) {
			// Too big for buffer so send directly
			pAccess->SetStyleFor(pos - startSeg + 1, static_cast<char>(chAttr));
		}
		else {
			for(Sci_PositionU i = startSeg; i <= pos; i++) {
				assert((startPosStyling + validLen) < Length());
				styleBuf[validLen++] = static_cast<char>(chAttr);
			}
		}
	}
	startSeg = pos+1;
}

void LexAccessor::SetLevel(Sci_Position line, int level)
{
	pAccess->SetLevel(line, level);
}

void LexAccessor::IndicatorFill(Sci_Position start, Sci_Position end, int indicator, int value)
{
	pAccess->DecorationSetCurrentIndicator(indicator);
	pAccess->DecorationFillRange(start, value, end - start);
}

void LexAccessor::ChangeLexerState(Sci_Position start, Sci_Position end)
{
	pAccess->ChangeLexerState(start, end);
}
//
//
//
Accessor::Accessor(IDocument * pAccess_, PropSetSimple * pprops_) : LexAccessor(pAccess_), pprops(pprops_)
{
}

int Accessor::GetPropertyInt(const char * key, int defaultValue) const
{
	return pprops->GetInt(key, defaultValue);
}

int Accessor::IndentAmount(Sci_Position line, int * flags, PFNIsCommentLeader pfnIsCommentLeader)
{
	Sci_Position end = Length();
	int spaceFlags = 0;
	// Determines the indentation level of the current line and also checks for consistent
	// indentation compared to the previous line.
	// Indentation is judged consistent when the indentation whitespace of each line lines
	// the same or the indentation of one line is a prefix of the other.
	Sci_Position pos = LineStart(line);
	char ch = (*this)[pos];
	int indent = 0;
	bool inPrevPrefix = line > 0;
	Sci_Position posPrev = inPrevPrefix ? LineStart(line-1) : 0;
	while(oneof2(ch, ' ', '\t') && (pos < end)) {
		if(inPrevPrefix) {
			char chPrev = (*this)[posPrev++];
			if(oneof2(chPrev, ' ', '\t')) {
				if(chPrev != ch)
					spaceFlags |= wsInconsistent;
			}
			else {
				inPrevPrefix = false;
			}
		}
		if(ch == ' ') {
			spaceFlags |= wsSpace;
			indent++;
		}
		else {          // Tab
			spaceFlags |= wsTab;
			if(spaceFlags & wsSpace)
				spaceFlags |= wsSpaceTab;
			indent = (indent / 8 + 1) * 8;
		}
		ch = (*this)[++pos];
	}
	*flags = spaceFlags;
	indent += SC_FOLDLEVELBASE;
	// if completely empty line or the start of a comment...
	if(LineStart(line) == Length() || oneof4(ch, ' ', '\t', '\n', '\r') || (pfnIsCommentLeader && (*pfnIsCommentLeader)(*this, pos, end-pos)))
		return indent | SC_FOLDLEVELWHITEFLAG;
	else
		return indent;
}

