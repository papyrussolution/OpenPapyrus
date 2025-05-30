// Document.cxx
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.
// Scintilla source code edit control
// Text document that handles notifications, DBCS, styling, words and end of line.
//
#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

// (see slport.h) #define NOEXCEPT
#ifndef NO_CXX11_REGEX
	#include <regex>
	#if defined(__GLIBCXX__)
		// If using the GNU implementation of <regex> then have 'noexcept' so can use
		// when defining regex iterators to keep Clang analyze happy.
		// (see slport.h) #undef NOEXCEPT
		// (see slport.h) #define NOEXCEPT noexcept
	#endif
#endif
#ifdef SCI_NAMESPACE
	using namespace Scintilla;
#endif

LexInterface::LexInterface(Document * pdoc_) : pdoc(pdoc_), instance(0), performingStyle(false)
{
}

LexInterface::~LexInterface()
{
}

void LexInterface::Colourise(int start, int end)
{
	if(pdoc && instance && !performingStyle) {
		// Protect against reentrance, which may occur, for example, when
		// fold points are discovered while performing styling and the folding
		// code looks for child lines which may trigger styling.
		performingStyle = true;
		int lengthDoc = pdoc->Length();
		if(end == -1)
			end = lengthDoc;
		int len = end - start;
		PLATFORM_ASSERT(len >= 0);
		PLATFORM_ASSERT(start + len <= lengthDoc);
		int styleStart = 0;
		if(start > 0)
			styleStart = pdoc->StyleAt(start - 1);
		if(len > 0) {
			instance->Lex(start, len, styleStart, pdoc);
			instance->Fold(start, len, styleStart, pdoc);
		}
		performingStyle = false;
	}
}

int LexInterface::LineEndTypesSupported()
{
	if(instance) {
		const int interfaceVersion = instance->Version();
		if(interfaceVersion >= lvSubStyles) {
			ILexerWithSubStyles * ssinstance = static_cast<ILexerWithSubStyles *>(instance);
			return ssinstance->LineEndTypesSupported();
		}
	}
	return 0;
}

UndoGroup::UndoGroup(Document * pdoc_, bool groupNeeded_/*= true*/) : pdoc(pdoc_), groupNeeded(groupNeeded_)
{
	if(groupNeeded)
		pdoc->BeginUndoAction();
}

UndoGroup::~UndoGroup()
{
	if(groupNeeded)
		pdoc->EndUndoAction();
}

bool UndoGroup::Needed() const { return groupNeeded; }

DocModification::DocModification(int modificationType_, int position_ /*= 0*/, int length_ /*= 0*/, int linesAdded_ /*= 0*/, const char * text_ /*= 0*/, int line_ /*= 0*/) :
	modificationType(modificationType_), position(position_), length(length_), linesAdded(linesAdded_), text(text_),
	line(line_), foldLevelNow(0), foldLevelPrev(0), annotationLinesAdded(0), token(0)
{
}

DocModification::DocModification(int modificationType_, const UndoHistory::Action & act, int linesAdded_ /*= 0*/) :
	modificationType(modificationType_), position(act.position), length(act.lenData), linesAdded(linesAdded_),
	text(act.data), line(0), foldLevelNow(0), foldLevelPrev(0), annotationLinesAdded(0), token(0)
{
}

Document::Document()
{
	refCount = 0;
	pcf = NULL;
#ifdef _WIN32
	eolMode = SC_EOL_CRLF;
#else
	eolMode = SC_EOL_LF;
#endif
	dbcsCodePage = 0;
	lineEndBitSet = SC_LINE_END_TYPE_DEFAULT;
	endStyled = 0;
	styleClock = 0;
	enteredModification = 0;
	enteredStyling = 0;
	enteredReadOnlyCount = 0;
	//insertionSet = false;
	//useTabs = true;
	//tabIndents = true;
	//backspaceUnindents = false;
	//matchesValid = false;
	DocFlags = (dfUseTabs | dfTabIndents);
	tabInChars = 8;
	indentInChars = 0;
	actualIndentInChars = 8;
	durationStyleOneLine = 0.00001;
	regex = 0;
	UTF8BytesOfLeadInitialise();
	perLineData[ldMarkers] = new LineMarkers();
	perLineData[ldLevels] = new LineLevels();
	perLineData[ldState] = new LineState();
	perLineData[ldMargin] = new LineAnnotation();
	perLineData[ldAnnotation] = new LineAnnotation();
	cb.SetPerLine(this);
	pli = 0;
}

Document::~Document()
{
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it) {
		it->watcher->NotifyDeleted(this, it->userData);
	}
	for(int j = 0; j<ldSize; j++) {
		delete perLineData[j];
		perLineData[j] = 0;
	}
	ZDELETE(regex);
	ZDELETE(pli);
	ZDELETE(pcf);
}

void Document::Init()
{
	for(int j = 0; j<ldSize; j++) {
		if(perLineData[j])
			perLineData[j]->Init();
	}
}

int Document::LineEndTypesSupported() const
{
	return ((dbcsCodePage == SC_CP_UTF8) && pli) ? pli->LineEndTypesSupported() : 0;
}

bool Document::SetDBCSCodePage(int dbcsCodePage_)
{
	if(dbcsCodePage != dbcsCodePage_) {
		dbcsCodePage = dbcsCodePage_;
		SetCaseFolder(NULL);
		cb.SetLineEndTypes(lineEndBitSet & LineEndTypesSupported());
		return true;
	}
	else
		return false;
}

bool Document::SetLineEndTypesAllowed(int lineEndBitSet_)
{
	if(lineEndBitSet != lineEndBitSet_) {
		lineEndBitSet = lineEndBitSet_;
		int lineEndBitSetActive = lineEndBitSet & LineEndTypesSupported();
		if(lineEndBitSetActive != cb.GetLineEndTypes()) {
			ModifiedAt(0);
			cb.SetLineEndTypes(lineEndBitSetActive);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

void Document::InsertLine(int line)
{
	for(int j = 0; j < ldSize; j++) {
		CALLPTRMEMB(perLineData[j], InsertLine(line));
	}
}

void Document::RemoveLine(int line)
{
	for(int j = 0; j < ldSize; j++) {
		CALLPTRMEMB(perLineData[j], RemoveLine(line));
	}
}

// Increase reference count and return its previous value.
int Document::AddRef() { return refCount++; }

// Decrease reference count and return its previous value.
// Delete the document if reference count reaches zero.
int SCI_METHOD Document::Release()
{
	int curRefCount = --refCount;
	if(curRefCount == 0)
		delete this;
	return curRefCount;
}

void Document::SetSavePoint()
{
	cb.SetSavePoint();
	NotifySavePoint(true);
}

void Document::TentativeUndo()
{
	if(TentativeActive()) {
		CheckReadOnly();
		if(enteredModification == 0) {
			enteredModification++;
			if(!cb.IsReadOnly()) {
				bool startSavePoint = cb.IsSavePoint();
				bool multiLine = false;
				int steps = cb.TentativeSteps();
				//Platform::DebugPrintf("Steps=%d\n", steps);
				for(int step = 0; step < steps; step++) {
					const int prevLinesTotal = LinesTotal();
					const UndoHistory::Action & action = cb.GetUndoStep();
					if(action.at == UndoHistory::Action::tRemove) {
						DocModification dm(SC_MOD_BEFOREINSERT|SC_PERFORMED_UNDO, action);
						NotifyModified(dm);
					}
					else if(action.at == UndoHistory::Action::tContainer) {
						DocModification dm(SC_MOD_CONTAINER|SC_PERFORMED_UNDO);
						dm.token = action.position;
						NotifyModified(dm);
					}
					else {
						DocModification dm(SC_MOD_BEFOREDELETE|SC_PERFORMED_UNDO, action);
						NotifyModified(dm);
					}
					cb.PerformUndoStep();
					if(action.at != UndoHistory::Action::tContainer) {
						ModifiedAt(action.position);
					}
					int modFlags = SC_PERFORMED_UNDO;
					// With undo, an insertion action becomes a deletion notification
					if(action.at == UndoHistory::Action::tRemove)
						modFlags |= SC_MOD_INSERTTEXT;
					else if(action.at == UndoHistory::Action::tInsert)
						modFlags |= SC_MOD_DELETETEXT;
					if(steps > 1)
						modFlags |= SC_MULTISTEPUNDOREDO;
					const int linesAdded = LinesTotal() - prevLinesTotal;
					if(linesAdded != 0)
						multiLine = true;
					if(step == steps - 1) {
						modFlags |= SC_LASTSTEPINUNDOREDO;
						if(multiLine)
							modFlags |= SC_MULTILINEUNDOREDO;
					}
					{
						DocModification dm(modFlags, action.position, action.lenData, linesAdded, action.data);
						NotifyModified(dm);
					}
				}
				bool endSavePoint = cb.IsSavePoint();
				if(startSavePoint != endSavePoint)
					NotifySavePoint(endSavePoint);
				cb.TentativeCommit();
			}
			enteredModification--;
		}
	}
}

int Document::GetMark(int line)
{
	return static_cast<LineMarkers *>(perLineData[ldMarkers])->MarkValue(line);
}

int Document::MarkerNext(int lineStart, int mask) const
{
	return static_cast<LineMarkers *>(perLineData[ldMarkers])->MarkerNext(lineStart, mask);
}

int Document::AddMark(int line, int markerNum)
{
	if(line >= 0 && line <= LinesTotal()) {
		int prev = static_cast<LineMarkers *>(perLineData[ldMarkers])->AddMark(line, markerNum, LinesTotal());
		DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
		NotifyModified(mh);
		return prev;
	}
	else
		return 0;
}

void Document::AddMarkSet(int line, int valueSet)
{
	if(line >= 0 && line <= LinesTotal()) {
		uint m = valueSet;
		for(int i = 0; m; i++, m >>= 1)
			if(m & 1)
				static_cast<LineMarkers *>(perLineData[ldMarkers])->AddMark(line, i, LinesTotal());
		DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
		NotifyModified(mh);
	}
}

void Document::DeleteMark(int line, int markerNum)
{
	static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMark(line, markerNum, false);
	DocModification mh(SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::DeleteMarkFromHandle(int markerHandle)
{
	static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMarkFromHandle(markerHandle);
	DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
	mh.line = -1;
	NotifyModified(mh);
}

void Document::DeleteAllMarks(int markerNum)
{
	bool someChanges = false;
	for(int line = 0; line < LinesTotal(); line++) {
		if(static_cast<LineMarkers *>(perLineData[ldMarkers])->DeleteMark(line, markerNum, true))
			someChanges = true;
	}
	if(someChanges) {
		DocModification mh(SC_MOD_CHANGEMARKER, 0, 0, 0, 0);
		mh.line = -1;
		NotifyModified(mh);
	}
}

int  Document::LineFromHandle(int markerHandle) { return static_cast<LineMarkers *>(perLineData[ldMarkers])->LineFromHandle(markerHandle); }
Sci_Position SCI_METHOD Document::LineStart(Sci_Position line) const { return cb.LineStart(line); }
bool Document::IsLineStartPosition(int position) const { return LineStart(LineFromPosition(position)) == position; }

Sci_Position SCI_METHOD Document::LineEnd(Sci_Position line) const
{
	if(line >= LinesTotal() - 1) {
		return LineStart(line + 1);
	}
	else {
		int position = LineStart(line + 1);
		if(SC_CP_UTF8 == dbcsCodePage) {
			uchar bytes[] = {
				static_cast<uchar>(cb.CharAt(position-3)),
				static_cast<uchar>(cb.CharAt(position-2)),
				static_cast<uchar>(cb.CharAt(position-1)),
			};
			if(UTF8IsSeparator(bytes)) {
				return position - UTF8SeparatorLength;
			}
			if(UTF8IsNEL(bytes+1)) {
				return position - UTF8NELLength;
			}
		}
		position--; // Back over CR or LF
		// When line terminator is CR+LF, may need to go back one more
		if((position > LineStart(line)) && (cb.CharAt(position - 1) == '\r')) {
			position--;
		}
		return position;
	}
}

void SCI_METHOD Document::SetErrorStatus(int status)
{
	// Tell the watchers an error has occurred.
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it) {
		it->watcher->NotifyErrorOccurred(this, it->userData, status);
	}
}

Sci_Position SCI_METHOD Document::LineFromPosition(Sci_Position pos) const { return cb.LineFromPosition(pos); }
int  Document::LineEndPosition(int position) const { return LineEnd(LineFromPosition(position)); }
bool FASTCALL Document::IsLineEndPosition(int position) const { return LineEnd(LineFromPosition(position)) == position; }
bool FASTCALL Document::IsPositionInLineEnd(int position) const { return (position >= LineEnd(LineFromPosition(position))); }

int Document::VCHomePosition(int position) const
{
	int line = LineFromPosition(position);
	int startPosition = LineStart(line);
	int endLine = LineEnd(line);
	int startText = startPosition;
	while(startText < endLine && IsASpaceOrTab(cb.CharAt(startText)))
		startText++;
	return (position == startText) ? startPosition : startText;
}

int SCI_METHOD Document::SetLevel(Sci_Position line, int level)
{
	int prev = static_cast<LineLevels *>(perLineData[ldLevels])->SetLevel(line, level, LinesTotal());
	if(prev != level) {
		DocModification mh(SC_MOD_CHANGEFOLD | SC_MOD_CHANGEMARKER, LineStart(line), 0, 0, 0, line);
		mh.foldLevelNow = level;
		mh.foldLevelPrev = prev;
		NotifyModified(mh);
	}
	return prev;
}

int  SCI_METHOD Document::GetLevel(Sci_Position line) const { return static_cast<LineLevels *>(perLineData[ldLevels])->GetLevel(line); }
void Document::ClearLevels() { static_cast<LineLevels *>(perLineData[ldLevels])->ClearLevels(); }
static bool FASTCALL IsSubordinate(int levelStart, int levelTry) { return (levelTry & SC_FOLDLEVELWHITEFLAG) ? true : (LevelNumber(levelStart) < LevelNumber(levelTry)); }

int Document::GetLastChild(int lineParent, int level, int lastLine)
{
	if(level == -1)
		level = LevelNumber(GetLevel(lineParent));
	int maxLine = LinesTotal();
	int lookLastLine = (lastLine != -1) ? smin(LinesTotal() - 1, lastLine) : -1;
	int lineMaxSubord = lineParent;
	while(lineMaxSubord < maxLine - 1) {
		EnsureStyledTo(LineStart(lineMaxSubord + 2));
		if(!IsSubordinate(level, GetLevel(lineMaxSubord + 1)))
			break;
		if((lookLastLine != -1) && (lineMaxSubord >= lookLastLine) && !(GetLevel(lineMaxSubord) & SC_FOLDLEVELWHITEFLAG))
			break;
		lineMaxSubord++;
	}
	if(lineMaxSubord > lineParent) {
		if(level > LevelNumber(GetLevel(lineMaxSubord + 1))) {
			// Have chewed up some whitespace that belongs to a parent so seek back
			if(GetLevel(lineMaxSubord) & SC_FOLDLEVELWHITEFLAG) {
				lineMaxSubord--;
			}
		}
	}
	return lineMaxSubord;
}

int Document::GetFoldParent(int line) const
{
	int level = LevelNumber(GetLevel(line));
	int lineLook = line - 1;
	while((lineLook > 0) && ((!(GetLevel(lineLook) & SC_FOLDLEVELHEADERFLAG)) || (LevelNumber(GetLevel(lineLook)) >= level))) {
		lineLook--;
	}
	return ((GetLevel(lineLook) & SC_FOLDLEVELHEADERFLAG) && (LevelNumber(GetLevel(lineLook)) < level)) ? lineLook : -1;
}

void Document::GetHighlightDelimiters(Document::HighlightDelimiter & highlightDelimiter, int line, int lastLine)
{
	int level = GetLevel(line);
	int lookLastLine = smax(line, lastLine) + 1;
	int lookLine = line;
	int lookLineLevel = level;
	int lookLineLevelNum = LevelNumber(lookLineLevel);
	while(lookLine > 0 && ((lookLineLevel & SC_FOLDLEVELWHITEFLAG) || ((lookLineLevel & SC_FOLDLEVELHEADERFLAG) && (lookLineLevelNum >= LevelNumber(GetLevel(lookLine + 1)))))) {
		lookLineLevel = GetLevel(--lookLine);
		lookLineLevelNum = LevelNumber(lookLineLevel);
	}
	int beginFoldBlock = (lookLineLevel & SC_FOLDLEVELHEADERFLAG) ? lookLine : GetFoldParent(lookLine);
	if(beginFoldBlock == -1) {
		highlightDelimiter.Clear();
		return;
	}
	int endFoldBlock = GetLastChild(beginFoldBlock, -1, lookLastLine);
	int firstChangeableLineBefore = -1;
	if(endFoldBlock < line) {
		lookLine = beginFoldBlock - 1;
		lookLineLevel = GetLevel(lookLine);
		lookLineLevelNum = LevelNumber(lookLineLevel);
		while((lookLine >= 0) && (lookLineLevelNum >= SC_FOLDLEVELBASE)) {
			if(lookLineLevel & SC_FOLDLEVELHEADERFLAG) {
				if(GetLastChild(lookLine, -1, lookLastLine) == line) {
					beginFoldBlock = lookLine;
					endFoldBlock = line;
					firstChangeableLineBefore = line - 1;
				}
			}
			if((lookLine > 0) && (lookLineLevelNum == SC_FOLDLEVELBASE) && (LevelNumber(GetLevel(lookLine - 1)) > lookLineLevelNum))
				break;
			lookLineLevel = GetLevel(--lookLine);
			lookLineLevelNum = LevelNumber(lookLineLevel);
		}
	}
	if(firstChangeableLineBefore == -1) {
		for(lookLine = line - 1, lookLineLevel = GetLevel(lookLine), lookLineLevelNum = LevelNumber(lookLineLevel);
		    lookLine >= beginFoldBlock;
		    lookLineLevel = GetLevel(--lookLine), lookLineLevelNum = LevelNumber(lookLineLevel)) {
			if((lookLineLevel & SC_FOLDLEVELWHITEFLAG) || (lookLineLevelNum > LevelNumber(level))) {
				firstChangeableLineBefore = lookLine;
				break;
			}
		}
	}
	if(firstChangeableLineBefore == -1)
		firstChangeableLineBefore = beginFoldBlock - 1;
	int firstChangeableLineAfter = -1;
	for(lookLine = line + 1, lookLineLevel = GetLevel(lookLine), lookLineLevelNum = LevelNumber(lookLineLevel);
	    lookLine <= endFoldBlock;
	    lookLineLevel = GetLevel(++lookLine), lookLineLevelNum = LevelNumber(lookLineLevel)) {
		if((lookLineLevel & SC_FOLDLEVELHEADERFLAG) && (lookLineLevelNum < LevelNumber(GetLevel(lookLine + 1)))) {
			firstChangeableLineAfter = lookLine;
			break;
		}
	}
	if(firstChangeableLineAfter == -1)
		firstChangeableLineAfter = endFoldBlock + 1;
	highlightDelimiter.beginFoldBlock = beginFoldBlock;
	highlightDelimiter.endFoldBlock = endFoldBlock;
	highlightDelimiter.firstChangeableLineBefore = firstChangeableLineBefore;
	highlightDelimiter.firstChangeableLineAfter = firstChangeableLineAfter;
}

int FASTCALL Document::ClampPositionIntoDocument(int pos) const
{
	return sclamp(pos, 0, Length());
}

bool FASTCALL Document::IsCrLf(int pos) const
{
	if(pos < 0)
		return false;
	else if(pos >= (Length() - 1))
		return false;
	else
		return (cb.CharAt(pos) == '\r') && (cb.CharAt(pos + 1) == '\n');
}

int FASTCALL Document::LenChar(int pos)
{
	if(pos < 0)
		return 1;
	else if(IsCrLf(pos))
		return 2;
	else if(SC_CP_UTF8 == dbcsCodePage) {
		const uchar leadByte = static_cast<uchar>(cb.CharAt(pos));
		const int widthCharBytes = UTF8BytesOfLead[leadByte];
		int lengthDoc = Length();
		if((pos + widthCharBytes) > lengthDoc)
			return lengthDoc - pos;
		else
			return widthCharBytes;
	}
	else if(dbcsCodePage)
		return IsDBCSLeadByte(cb.CharAt(pos)) ? 2 : 1;
	else
		return 1;
}

bool Document::InGoodUTF8(int pos, int &start, int &end) const
{
	int trail = pos;
	while((trail>0) && (pos-trail < UTF8MaxBytes) && UTF8IsTrailByte(static_cast<uchar>(cb.CharAt(trail-1))))
		trail--;
	start = (trail > 0) ? trail-1 : trail;
	const uchar leadByte = static_cast<uchar>(cb.CharAt(start));
	const int widthCharBytes = UTF8BytesOfLead[leadByte];
	if(widthCharBytes == 1) {
		return false;
	}
	else {
		int trailBytes = widthCharBytes - 1;
		int len = pos - start;
		if(len > trailBytes)
			return false; // pos too far from lead
		char charBytes[UTF8MaxBytes] = {static_cast<char>(leadByte), 0, 0, 0};
		for(int b = 1; b<widthCharBytes && ((start+b) < Length()); b++)
			charBytes[b] = cb.CharAt(static_cast<int>(start+b));
		int utf8status = UTF8Classify(reinterpret_cast<const uchar *>(charBytes), widthCharBytes);
		if(utf8status & UTF8MaskInvalid)
			return false;
		end = start + widthCharBytes;
		return true;
	}
}

// Normalise a position so that it is not halfway through a two byte character.
// This can occur in two situations -
// When lines are terminated with \r\n pairs which should be treated as one character.
// When displaying DBCS text such as Japanese.
// If moving, move the position in the indicated direction.
int Document::MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd) const
{
	//Platform::DebugPrintf("NoCRLF %d %d\n", pos, moveDir);
	// If out of range, just return minimum/maximum value.
	if(pos <= 0)
		return 0;
	if(pos >= Length())
		return Length();
	// PLATFORM_ASSERT(pos > 0 && pos < Length());
	if(checkLineEnd && IsCrLf(pos - 1)) {
		if(moveDir > 0)
			return pos + 1;
		else
			return pos - 1;
	}
	if(dbcsCodePage) {
		if(SC_CP_UTF8 == dbcsCodePage) {
			uchar ch = static_cast<uchar>(cb.CharAt(pos));
			// If ch is not a trail byte then pos is valid intercharacter position
			if(UTF8IsTrailByte(ch)) {
				int startUTF = pos;
				int endUTF = pos;
				if(InGoodUTF8(pos, startUTF, endUTF)) {
					// ch is a trail byte within a UTF-8 character
					if(moveDir > 0)
						pos = endUTF;
					else
						pos = startUTF;
				}
				// Else invalid UTF-8 so return position of isolated trail byte
			}
		}
		else {
			// Anchor DBCS calculations at start of line because start of line can
			// not be a DBCS trail byte.
			int posStartLine = LineStart(LineFromPosition(pos));
			if(pos == posStartLine)
				return pos;
			// Step back until a non-lead-byte is found.
			int posCheck = pos;
			while((posCheck > posStartLine) && IsDBCSLeadByte(cb.CharAt(posCheck-1)))
				posCheck--;
			// Check from known start of character.
			while(posCheck < pos) {
				const int mbsize = IsDBCSLeadByte(cb.CharAt(posCheck)) ? 2 : 1;
				if(posCheck + mbsize == pos)
					return pos;
				else if(posCheck + mbsize > pos) {
					if(moveDir > 0)
						return (posCheck + mbsize);
					else
						return posCheck;
				}
				posCheck += mbsize;
			}
		}
	}
	return pos;
}

// NextPosition moves between valid positions - it can not handle a position in the middle of a
// multi-byte character. It is used to iterate through text more efficiently than MovePositionOutsideChar.
// A \r\n pair is treated as two characters.
int Document::NextPosition(int pos, int moveDir) const
{
	// If out of range, just return minimum/maximum value.
	int increment = (moveDir > 0) ? 1 : -1;
	if(pos + increment <= 0)
		return 0;
	if(pos + increment >= Length())
		return Length();
	if(dbcsCodePage) {
		if(SC_CP_UTF8 == dbcsCodePage) {
			if(increment == 1) {
				// Simple forward movement case so can avoid some checks
				const uchar leadByte = static_cast<uchar>(cb.CharAt(pos));
				if(UTF8IsAscii(leadByte)) {
					pos++; // Single byte character or invalid
				}
				else {
					const int widthCharBytes = UTF8BytesOfLead[leadByte];
					char charBytes[UTF8MaxBytes] = {static_cast<char>(leadByte), 0, 0, 0};
					for(int b = 1; b<widthCharBytes; b++)
						charBytes[b] = cb.CharAt(static_cast<int>(pos+b));
					int utf8status = UTF8Classify(reinterpret_cast<const uchar *>(charBytes), widthCharBytes);
					if(utf8status & UTF8MaskInvalid)
						pos++;
					else
						pos += utf8status & UTF8MaskWidth;
				}
			}
			else {
				// Examine byte before position
				pos--;
				uchar ch = static_cast<uchar>(cb.CharAt(pos));
				// If ch is not a trail byte then pos is valid intercharacter position
				if(UTF8IsTrailByte(ch)) {
					// If ch is a trail byte in a valid UTF-8 character then return start of character
					int startUTF = pos;
					int endUTF = pos;
					if(InGoodUTF8(pos, startUTF, endUTF)) {
						pos = startUTF;
					}
					// Else invalid UTF-8 so return position of isolated trail byte
				}
			}
		}
		else {
			if(moveDir > 0) {
				int mbsize = IsDBCSLeadByte(cb.CharAt(pos)) ? 2 : 1;
				pos += mbsize;
				if(pos > Length())
					pos = Length();
			}
			else {
				// Anchor DBCS calculations at start of line because start of line can
				// not be a DBCS trail byte.
				int posStartLine = LineStart(LineFromPosition(pos));
				// See http://msdn.microsoft.com/en-us/library/cc194792%28v=MSDN.10%29.aspx
				// http://msdn.microsoft.com/en-us/library/cc194790.aspx
				if((pos - 1) <= posStartLine) {
					return pos - 1;
				}
				else if(IsDBCSLeadByte(cb.CharAt(pos - 1))) {
					// Must actually be trail byte
					return pos - 2;
				}
				else {
					// Otherwise, step back until a non-lead-byte is found.
					int posTemp = pos - 1;
					while(posStartLine <= --posTemp && IsDBCSLeadByte(cb.CharAt(posTemp)))
						;
					// Now posTemp+1 must point to the beginning of a character,
					// so figure out whether we went back an even or an odd
					// number of bytes and go back 1 or 2 bytes, respectively.
					return (pos - 1 - ((pos - posTemp) & 1));
				}
			}
		}
	}
	else {
		pos += increment;
	}
	return pos;
}

bool Document::NextCharacter(int &pos, int moveDir) const
{
	// Returns true if pos changed
	int posNext = NextPosition(pos, moveDir);
	if(posNext == pos) {
		return false;
	}
	else {
		pos = posNext;
		return true;
	}
}

Document::CharacterExtracted FASTCALL Document::CharacterAfter(int position) const
{
	if(position >= Length())
		return CharacterExtracted(unicodeReplacementChar, 0);
	else {
		const uchar leadByte = static_cast<uchar>(cb.CharAt(position));
		if(!dbcsCodePage || UTF8IsAscii(leadByte)) // Common case: ASCII character
			return CharacterExtracted(leadByte, 1);
		else if(SC_CP_UTF8 == dbcsCodePage) {
			const int widthCharBytes = UTF8BytesOfLead[leadByte];
			uchar charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
			for(int b = 1; b<widthCharBytes; b++)
				charBytes[b] = static_cast<uchar>(cb.CharAt(position + b));
			int utf8status = UTF8Classify(charBytes, widthCharBytes);
			if(utf8status & UTF8MaskInvalid) // Treat as invalid and use up just one byte
				return CharacterExtracted(unicodeReplacementChar, 1);
			else
				return CharacterExtracted(UnicodeFromUTF8(charBytes), utf8status & UTF8MaskWidth);
		}
		else if(IsDBCSLeadByte(leadByte) && ((position + 1) < Length()))
			return CharacterExtracted::DBCS(leadByte, static_cast<uchar>(cb.CharAt(position + 1)));
		else
			return CharacterExtracted(leadByte, 1);
	}
}

Document::CharacterExtracted FASTCALL Document::CharacterBefore(int position) const
{
	if(position <= 0)
		return CharacterExtracted(unicodeReplacementChar, 0);
	else {
		const uchar previousByte = static_cast<uchar>(cb.CharAt(position - 1));
		if(0 == dbcsCodePage)
			return CharacterExtracted(previousByte, 1);
		else if(SC_CP_UTF8 == dbcsCodePage) {
			if(UTF8IsAscii(previousByte))
				return CharacterExtracted(previousByte, 1);
			else {
				position--;
				// If previousByte is not a trail byte then its invalid
				if(UTF8IsTrailByte(previousByte)) {
					// If previousByte is a trail byte in a valid UTF-8 character then find start of character
					int startUTF = position;
					int endUTF = position;
					if(InGoodUTF8(position, startUTF, endUTF)) {
						const int widthCharBytes = endUTF - startUTF;
						uchar charBytes[UTF8MaxBytes] = { 0, 0, 0, 0 };
						for(int b = 0; b<widthCharBytes; b++)
							charBytes[b] = static_cast<uchar>(cb.CharAt(startUTF + b));
						int utf8status = UTF8Classify(charBytes, widthCharBytes);
						if(utf8status & UTF8MaskInvalid) // Treat as invalid and use up just one byte
							return CharacterExtracted(unicodeReplacementChar, 1);
						else
							return CharacterExtracted(UnicodeFromUTF8(charBytes), utf8status & UTF8MaskWidth);
					}
					// Else invalid UTF-8 so return position of isolated trail byte
				}
				return CharacterExtracted(unicodeReplacementChar, 1);
			}
		}
		else { // Moving backwards in DBCS is complex so use NextPosition
			const int posStartCharacter = NextPosition(position, -1);
			return CharacterAfter(posStartCharacter);
		}
	}
}

// Return -1  on out-of-bounds
Sci_Position SCI_METHOD Document::GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const
{
	int pos = positionStart;
	if(dbcsCodePage) {
		const int increment = (characterOffset > 0) ? 1 : -1;
		while(characterOffset != 0) {
			const int posNext = NextPosition(pos, increment);
			if(posNext == pos)
				return INVALID_POSITION;
			pos = posNext;
			characterOffset -= increment;
		}
	}
	else {
		pos = positionStart + characterOffset;
		if((pos < 0) || (pos > Length()))
			return INVALID_POSITION;
	}
	return pos;
}

int Document::GetRelativePositionUTF16(int positionStart, int characterOffset) const
{
	int pos = positionStart;
	if(dbcsCodePage) {
		const int increment = (characterOffset > 0) ? 1 : -1;
		while(characterOffset != 0) {
			const int posNext = NextPosition(pos, increment);
			if(posNext == pos)
				return INVALID_POSITION;
			if(abs(pos-posNext) > 3)        // 4 byte character = 2*UTF16.
				characterOffset -= increment;
			pos = posNext;
			characterOffset -= increment;
		}
	}
	else {
		pos = positionStart + characterOffset;
		if((pos < 0) || (pos > Length()))
			return INVALID_POSITION;
	}
	return pos;
}

int SCI_METHOD Document::GetCharacterAndWidth(Sci_Position position, Sci_Position * pWidth) const
{
	int character;
	int bytesInCharacter = 1;
	if(dbcsCodePage) {
		const uchar leadByte = static_cast<uchar>(cb.CharAt(position));
		if(SC_CP_UTF8 == dbcsCodePage) {
			if(UTF8IsAscii(leadByte)) {
				character =  leadByte; // Single byte character or invalid
			}
			else {
				const int widthCharBytes = UTF8BytesOfLead[leadByte];
				uchar charBytes[UTF8MaxBytes] = {leadByte, 0, 0, 0};
				for(int b = 1; b<widthCharBytes; b++)
					charBytes[b] = static_cast<uchar>(cb.CharAt(position+b));
				int utf8status = UTF8Classify(charBytes, widthCharBytes);
				if(utf8status & UTF8MaskInvalid) {
					character =  0xDC80 + leadByte; // Report as singleton surrogate values which are invalid Unicode
				}
				else {
					bytesInCharacter = utf8status & UTF8MaskWidth;
					character = UnicodeFromUTF8(charBytes);
				}
			}
		}
		else {
			if(IsDBCSLeadByte(leadByte)) {
				bytesInCharacter = 2;
				character = (leadByte << 8) | static_cast<uchar>(cb.CharAt(position+1));
			}
			else {
				character = leadByte;
			}
		}
	}
	else {
		character = cb.CharAt(position);
	}
	ASSIGN_PTR(pWidth, bytesInCharacter);
	return character;
}

int SCI_METHOD Document::CodePage() const
{
	return dbcsCodePage;
}

bool SCI_METHOD Document::IsDBCSLeadByte(char ch) const
{
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	uchar uch = static_cast<uchar>(ch);
	switch(dbcsCodePage) {
		case 932: return ((uch >= 0x81) && (uch <= 0x9F)) || ((uch >= 0xE0) && (uch <= 0xFC)); // Shift_jis
		// Lead bytes F0 to FC may be a Microsoft addition.
		case 936: return (uch >= 0x81) && (uch <= 0xFE); // GBK
		case 949: return (uch >= 0x81) && (uch <= 0xFE); // Korean Wansung KS C-5601-1987
		case 950: return (uch >= 0x81) && (uch <= 0xFE); // Big5
		case 1361: return ((uch >= 0x84) && (uch <= 0xD3)) || ((uch >= 0xD8) && (uch <= 0xDE)) || ((uch >= 0xE0) && (uch <= 0xF9));  // Korean Johab KS C-5601-1992
	}
	return false;
}

//static bool FASTCALL IsSpaceOrTab_Removed(int ch) { return oneof2(ch, ' ', '\t'); }

// Need to break text into segments near lengthSegment but taking into
// account the encoding to not break inside a UTF-8 or DBCS character
// and also trying to avoid breaking inside a pair of combining characters.
// The segment length must always be long enough (more than 4 bytes)
// so that there will be at least one whole character to make a segment.
// For UTF-8, text must consist only of valid whole characters.
// In preference order from best to worst:
//   1) Break after space
//   2) Break before punctuation
//   3) Break after whole character

int Document::SafeSegment(const char * text, int length, int lengthSegment) const
{
	if(length <= lengthSegment)
		return length;
	int lastSpaceBreak = -1;
	int lastPunctuationBreak = -1;
	int lastEncodingAllowedBreak = 0;
	for(int j = 0; j < lengthSegment;) {
		uchar ch = static_cast<uchar>(text[j]);
		if(j > 0) {
			if(IsASpaceOrTab(text[j - 1]) && !IsASpaceOrTab(text[j])) {
				lastSpaceBreak = j;
			}
			if(ch < 'A') {
				lastPunctuationBreak = j;
			}
		}
		lastEncodingAllowedBreak = j;
		if(dbcsCodePage == SC_CP_UTF8) {
			j += UTF8BytesOfLead[ch];
		}
		else if(dbcsCodePage) {
			j += IsDBCSLeadByte(ch) ? 2 : 1;
		}
		else {
			j++;
		}
	}
	if(lastSpaceBreak >= 0)
		return lastSpaceBreak;
	else if(lastPunctuationBreak >= 0)
		return lastPunctuationBreak;
	else
		return lastEncodingAllowedBreak;
}

EncodingFamily Document::CodePageFamily() const
{
	if(dbcsCodePage == SC_CP_UTF8)
		return efUnicode;
	else if(dbcsCodePage)
		return efDBCS;
	else
		return efEightBit;
}

void FASTCALL Document::ModifiedAt(int pos)
{
	SETMIN(endStyled, pos);
}

void Document::CheckReadOnly()
{
	if(cb.IsReadOnly() && enteredReadOnlyCount == 0) {
		enteredReadOnlyCount++;
		NotifyModifyAttempt();
		enteredReadOnlyCount--;
	}
}

// Document only modified by gateways DeleteChars, InsertString, Undo, Redo, and SetStyleAt.
// SetStyleAt does not change the persistent state of a document

bool Document::DeleteChars(int pos, int len)
{
	if(pos < 0)
		return false;
	if(len <= 0)
		return false;
	if((pos + len) > Length())
		return false;
	CheckReadOnly();
	if(enteredModification != 0) {
		return false;
	}
	else {
		enteredModification++;
		if(!cb.IsReadOnly()) {
			{
				DocModification dm(SC_MOD_BEFOREDELETE|SC_PERFORMED_USER, pos, len, 0, 0);
				NotifyModified(dm);
			}
			int prevLinesTotal = LinesTotal();
			bool startSavePoint = cb.IsSavePoint();
			bool startSequence = false;
			const char * text = cb.DeleteChars(pos, len, startSequence);
			if(startSavePoint && cb.IsCollectingUndo())
				NotifySavePoint(!startSavePoint);
			if((pos < Length()) || (pos == 0))
				ModifiedAt(pos);
			else
				ModifiedAt(pos-1);
			{
				DocModification dm(SC_MOD_DELETETEXT|SC_PERFORMED_USER|(startSequence ? SC_STARTACTION : 0), pos, len, LinesTotal() - prevLinesTotal, text);
				NotifyModified(dm);
			}
		}
		enteredModification--;
	}
	return !cb.IsReadOnly();
}
/**
 * Insert a string with a length.
 */
int Document::InsertString(int position, const char * s, int insertLength)
{
	if(insertLength <= 0) {
		return 0;
	}
	CheckReadOnly();        // Application may change read only state here
	if(cb.IsReadOnly()) {
		return 0;
	}
	if(enteredModification != 0) {
		return 0;
	}
	enteredModification++;
	//insertionSet = false;
	DocFlags &= ~dfInsertionSet;
	insertion.clear();
	{
		DocModification dm(SC_MOD_INSERTCHECK, position, insertLength, 0, s);
		NotifyModified(dm);
	}
	if(DocFlags & dfInsertionSet) {
		s = insertion.c_str();
		insertLength = static_cast<int>(insertion.length());
	}
	{
		DocModification dm(SC_MOD_BEFOREINSERT | SC_PERFORMED_USER, position, insertLength, 0, s);
		NotifyModified(dm);
	}
	int prevLinesTotal = LinesTotal();
	bool startSavePoint = cb.IsSavePoint();
	bool startSequence = false;
	const char * text = cb.InsertString(position, s, insertLength, startSequence);
	if(startSavePoint && cb.IsCollectingUndo())
		NotifySavePoint(!startSavePoint);
	ModifiedAt(position);
	{
		DocModification dm(SC_MOD_INSERTTEXT|SC_PERFORMED_USER | (startSequence ? SC_STARTACTION : 0), position, insertLength, LinesTotal() - prevLinesTotal, text);
		NotifyModified(dm);
	}
	if(DocFlags & dfInsertionSet) { // Free memory as could be large
		std::string().swap(insertion);
	}
	enteredModification--;
	return insertLength;
}

void Document::ChangeInsertion(const char * s, int length)
{
	DocFlags |= dfInsertionSet;
	insertion.assign(s, length);
}

int SCI_METHOD Document::AddData(char * data, Sci_Position length)
{
	try {
		int position = Length();
		InsertString(position, data, length);
	} catch(std::bad_alloc &) {
		return SC_STATUS_BADALLOC;
	} catch(...) {
		return SC_STATUS_FAILURE;
	}
	return 0;
}

void * SCI_METHOD Document::ConvertToDocument()
{
	return this;
}

int Document::Undo()
{
	int newPos = -1;
	CheckReadOnly();
	if((enteredModification == 0) && (cb.IsCollectingUndo())) {
		enteredModification++;
		if(!cb.IsReadOnly()) {
			bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			int steps = cb.StartUndo();
			//Platform::DebugPrintf("Steps=%d\n", steps);
			int coalescedRemovePos = -1;
			int coalescedRemoveLen = 0;
			int prevRemoveActionPos = -1;
			int prevRemoveActionLen = 0;
			for(int step = 0; step < steps; step++) {
				const int prevLinesTotal = LinesTotal();
				const UndoHistory::Action & action = cb.GetUndoStep();
				if(action.at == UndoHistory::Action::tRemove) {
					DocModification dm(SC_MOD_BEFOREINSERT|SC_PERFORMED_UNDO, action);
					NotifyModified(dm);
				}
				else if(action.at == UndoHistory::Action::tContainer) {
					DocModification dm(SC_MOD_CONTAINER|SC_PERFORMED_UNDO);
					dm.token = action.position;
					NotifyModified(dm);
					if(!action.mayCoalesce) {
						coalescedRemovePos = -1;
						coalescedRemoveLen = 0;
						prevRemoveActionPos = -1;
						prevRemoveActionLen = 0;
					}
				}
				else {
					DocModification dm(SC_MOD_BEFOREDELETE|SC_PERFORMED_UNDO, action);
					NotifyModified(dm);
				}
				cb.PerformUndoStep();
				if(action.at != UndoHistory::Action::tContainer) {
					ModifiedAt(action.position);
					newPos = action.position;
				}
				int modFlags = SC_PERFORMED_UNDO;
				// With undo, an insertion action becomes a deletion notification
				if(action.at == UndoHistory::Action::tRemove) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
					if((coalescedRemoveLen > 0) && (action.position == prevRemoveActionPos || action.position == (prevRemoveActionPos + prevRemoveActionLen))) {
						coalescedRemoveLen += action.lenData;
						newPos = coalescedRemovePos + coalescedRemoveLen;
					}
					else {
						coalescedRemovePos = action.position;
						coalescedRemoveLen = action.lenData;
					}
					prevRemoveActionPos = action.position;
					prevRemoveActionLen = action.lenData;
				}
				else if(action.at == UndoHistory::Action::tInsert) {
					modFlags |= SC_MOD_DELETETEXT;
					coalescedRemovePos = -1;
					coalescedRemoveLen = 0;
					prevRemoveActionPos = -1;
					prevRemoveActionLen = 0;
				}
				if(steps > 1)
					modFlags |= SC_MULTISTEPUNDOREDO;
				const int linesAdded = LinesTotal() - prevLinesTotal;
				if(linesAdded != 0)
					multiLine = true;
				if(step == steps - 1) {
					modFlags |= SC_LASTSTEPINUNDOREDO;
					if(multiLine)
						modFlags |= SC_MULTILINEUNDOREDO;
				}
				{
					DocModification dm(modFlags, action.position, action.lenData, linesAdded, action.data);
					NotifyModified(dm);
				}
			}
			bool endSavePoint = cb.IsSavePoint();
			if(startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

int Document::Redo()
{
	int newPos = -1;
	CheckReadOnly();
	if((enteredModification == 0) && (cb.IsCollectingUndo())) {
		enteredModification++;
		if(!cb.IsReadOnly()) {
			bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			int steps = cb.StartRedo();
			for(int step = 0; step < steps; step++) {
				const int prevLinesTotal = LinesTotal();
				const UndoHistory::Action & action = cb.GetRedoStep();
				if(action.at == UndoHistory::Action::tInsert) {
					DocModification dm(SC_MOD_BEFOREINSERT | SC_PERFORMED_REDO, action);
					NotifyModified(dm);
				}
				else if(action.at == UndoHistory::Action::tContainer) {
					DocModification dm(SC_MOD_CONTAINER | SC_PERFORMED_REDO);
					dm.token = action.position;
					NotifyModified(dm);
				}
				else {
					DocModification dm(SC_MOD_BEFOREDELETE | SC_PERFORMED_REDO, action);
					NotifyModified(dm);
				}
				cb.PerformRedoStep();
				if(action.at != UndoHistory::Action::tContainer) {
					ModifiedAt(action.position);
					newPos = action.position;
				}
				int modFlags = SC_PERFORMED_REDO;
				if(action.at == UndoHistory::Action::tInsert) {
					newPos += action.lenData;
					modFlags |= SC_MOD_INSERTTEXT;
				}
				else if(action.at == UndoHistory::Action::tRemove) {
					modFlags |= SC_MOD_DELETETEXT;
				}
				if(steps > 1)
					modFlags |= SC_MULTISTEPUNDOREDO;
				const int linesAdded = LinesTotal() - prevLinesTotal;
				if(linesAdded != 0)
					multiLine = true;
				if(step == steps - 1) {
					modFlags |= SC_LASTSTEPINUNDOREDO;
					if(multiLine)
						modFlags |= SC_MULTILINEUNDOREDO;
				}
				{
					DocModification dm(modFlags, action.position, action.lenData, linesAdded, action.data);
					NotifyModified(dm);
				}
			}
			bool endSavePoint = cb.IsSavePoint();
			if(startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

void Document::DelChar(int pos)
{
	DeleteChars(pos, LenChar(pos));
}

void Document::DelCharBack(int pos)
{
	if(pos <= 0) {
		return;
	}
	else if(IsCrLf(pos - 2)) {
		DeleteChars(pos - 2, 2);
	}
	else if(dbcsCodePage) {
		int startChar = NextPosition(pos, -1);
		DeleteChars(startChar, pos - startChar);
	}
	else {
		DeleteChars(pos - 1, 1);
	}
}

static int FASTCALL NextTab(int pos, int tabSize)
{
	return ((pos / tabSize) + 1) * tabSize;
}

static std::string CreateIndentation(int indent, int tabSize, bool insertSpaces)
{
	std::string indentation;
	if(!insertSpaces) {
		while(indent >= tabSize) {
			indentation += '\t';
			indent -= tabSize;
		}
	}
	while(indent > 0) {
		indentation += ' ';
		indent--;
	}
	return indentation;
}

int SCI_METHOD Document::GetLineIndentation(Sci_Position line)
{
	int indent = 0;
	if((line >= 0) && (line < LinesTotal())) {
		int lineStart = LineStart(line);
		int length = Length();
		for(int i = lineStart; i < length; i++) {
			char ch = cb.CharAt(i);
			if(ch == ' ')
				indent++;
			else if(ch == '\t')
				indent = NextTab(indent, tabInChars);
			else
				return indent;
		}
	}
	return indent;
}

int Document::SetLineIndentation(int line, int indent)
{
	int indentOfLine = GetLineIndentation(line);
	SETMAX(indent, 0);
	if(indent != indentOfLine) {
		std::string linebuf = CreateIndentation(indent, tabInChars, !(DocFlags & dfUseTabs));
		int thisLineStart = LineStart(line);
		int indentPos = GetLineIndentPosition(line);
		UndoGroup ug(this);
		DeleteChars(thisLineStart, indentPos - thisLineStart);
		return thisLineStart + InsertString(thisLineStart, linebuf.c_str(), static_cast<int>(linebuf.length()));
	}
	else {
		return GetLineIndentPosition(line);
	}
}

int Document::GetLineIndentPosition(int line) const
{
	if(line < 0)
		return 0;
	int pos = LineStart(line);
	int length = Length();
	while((pos < length) && IsASpaceOrTab(cb.CharAt(pos))) {
		pos++;
	}
	return pos;
}

int FASTCALL Document::GetColumn(int pos)
{
	int column = 0;
	int line = LineFromPosition(pos);
	if((line >= 0) && (line < LinesTotal())) {
		for(int i = LineStart(line); i < pos;) {
			char ch = cb.CharAt(i);
			if(ch == '\t') {
				column = NextTab(column, tabInChars);
				i++;
			}
			else if(ch == '\r')
				return column;
			else if(ch == '\n')
				return column;
			else if(i >= Length())
				return column;
			else {
				column++;
				i = NextPosition(i, 1);
			}
		}
	}
	return column;
}

int Document::CountCharacters(int startPos, int endPos) const
{
	startPos = MovePositionOutsideChar(startPos, 1, false);
	endPos = MovePositionOutsideChar(endPos, -1, false);
	int count = 0;
	for(int i = startPos; i < endPos; i = NextPosition(i, 1))
		count++;
	return count;
}

int Document::CountUTF16(int startPos, int endPos) const
{
	startPos = MovePositionOutsideChar(startPos, 1, false);
	endPos = MovePositionOutsideChar(endPos, -1, false);
	int count = 0;
	for(int i = startPos; i < endPos;) {
		count++;
		const int next = NextPosition(i, 1);
		if((next - i) > 3)
			count++;
		i = next;
	}
	return count;
}

int Document::FindColumn(int line, int column)
{
	int position = LineStart(line);
	if((line >= 0) && (line < LinesTotal())) {
		int columnCurrent = 0;
		while((columnCurrent < column) && (position < Length())) {
			char ch = cb.CharAt(position);
			if(ch == '\t') {
				columnCurrent = NextTab(columnCurrent, tabInChars);
				if(columnCurrent > column)
					return position;
				position++;
			}
			else if(ch == '\r') {
				return position;
			}
			else if(ch == '\n') {
				return position;
			}
			else {
				columnCurrent++;
				position = NextPosition(position, 1);
			}
		}
	}
	return position;
}

void Document::Indent(bool forwards, int lineBottom, int lineTop)
{
	// Dedent - suck white space off the front of the line to dedent by equivalent of a tab
	for(int line = lineBottom; line >= lineTop; line--) {
		const int indentOfLine = GetLineIndentation(line);
		if(forwards) {
			if(LineStart(line) < LineEnd(line))
				SetLineIndentation(line, indentOfLine + IndentSize());
		}
		else
			SetLineIndentation(line, indentOfLine - IndentSize());
	}
}

// Convert line endings for a piece of text to a particular mode.
// Stop at len or when a NUL is found.
std::string Document::TransformLineEnds(const char * s, size_t len, int eolModeWanted)
{
	std::string dest;
	for(size_t i = 0; (i < len) && (s[i]); i++) {
		if(s[i] == '\n' || s[i] == '\r') {
			if(eolModeWanted == SC_EOL_CR) {
				dest.push_back('\r');
			}
			else if(eolModeWanted == SC_EOL_LF) {
				dest.push_back('\n');
			}
			else {   // eolModeWanted == SC_EOL_CRLF
				dest.push_back('\r');
				dest.push_back('\n');
			}
			if((s[i] == '\r') && (i+1 < len) && (s[i+1] == '\n')) {
				i++;
			}
		}
		else {
			dest.push_back(s[i]);
		}
	}
	return dest;
}

void Document::ConvertLineEnds(int eolModeSet)
{
	UndoGroup ug(this);
	for(int pos = 0; pos < Length(); pos++) {
		if(cb.CharAt(pos) == '\r') {
			if(cb.CharAt(pos + 1) == '\n') {
				// CRLF
				if(eolModeSet == SC_EOL_CR) {
					DeleteChars(pos + 1, 1); // Delete the LF
				}
				else if(eolModeSet == SC_EOL_LF) {
					DeleteChars(pos, 1); // Delete the CR
				}
				else {
					pos++;
				}
			}
			else {
				// CR
				if(eolModeSet == SC_EOL_CRLF) {
					pos += InsertString(pos + 1, "\n", 1); // Insert LF
				}
				else if(eolModeSet == SC_EOL_LF) {
					pos += InsertString(pos, "\n", 1); // Insert LF
					DeleteChars(pos, 1); // Delete CR
					pos--;
				}
			}
		}
		else if(cb.CharAt(pos) == '\n') {
			// LF
			if(eolModeSet == SC_EOL_CRLF) {
				pos += InsertString(pos, "\r", 1); // Insert CR
			}
			else if(eolModeSet == SC_EOL_CR) {
				pos += InsertString(pos, "\r", 1); // Insert CR
				DeleteChars(pos, 1); // Delete LF
				pos--;
			}
		}
	}
}

bool FASTCALL Document::IsWhiteLine(int line) const
{
	int currentChar = LineStart(line);
	const int endLine = LineEnd(line);
	while(currentChar < endLine) {
		if(!IsASpaceOrTab(cb.CharAt(currentChar))) {
			return false;
		}
		++currentChar;
	}
	return true;
}

int FASTCALL Document::ParaUp(int pos) const
{
	int line = LineFromPosition(pos);
	line--;
	while(line >= 0 && IsWhiteLine(line)) {  // skip empty lines
		line--;
	}
	while(line >= 0 && !IsWhiteLine(line)) {  // skip non-empty lines
		line--;
	}
	line++;
	return LineStart(line);
}

int FASTCALL Document::ParaDown(int pos) const
{
	int line = LineFromPosition(pos);
	while(line < LinesTotal() && !IsWhiteLine(line)) {  // skip non-empty lines
		line++;
	}
	while(line < LinesTotal() && IsWhiteLine(line)) {  // skip empty lines
		line++;
	}
	if(line < LinesTotal())
		return LineStart(line);
	else // end of a document
		return LineEnd(line-1);
}

bool FASTCALL Document::IsASCIIWordByte(uchar ch) const
{
	return IsASCII(ch) ? (charClass.GetClass(ch) == CharClassify::ccWord) : false;
}

CharClassify::cc Document::WordCharacterClass(uint ch) const
{
	if(dbcsCodePage && (!UTF8IsAscii(ch))) {
		if(SC_CP_UTF8 == dbcsCodePage) {
			// Use hard coded Unicode class
			const CharacterCategory cc = CategoriseCharacter(ch);
			switch(cc) {
				// Separator, Line/Paragraph
				case ccZl:
				case ccZp: return CharClassify::ccNewLine;
				// Separator, Space
				case ccZs:
				// Other
				case ccCc:
				case ccCf:
				case ccCs:
				case ccCo:
				case ccCn: return CharClassify::ccSpace;
				// Letter
				case ccLu:
				case ccLl:
				case ccLt:
				case ccLm:
				case ccLo:
				// Number
				case ccNd:
				case ccNl:
				case ccNo:
				// Mark - includes combining diacritics
				case ccMn:
				case ccMc:
				case ccMe: return CharClassify::ccWord;
				// Punctuation
				case ccPc:
				case ccPd:
				case ccPs:
				case ccPe:
				case ccPi:
				case ccPf:
				case ccPo:
				// Symbol
				case ccSm:
				case ccSc:
				case ccSk:
				case ccSo: return CharClassify::ccPunctuation;
			}
		}
		else
			return CharClassify::ccWord; // Asian DBCS
	}
	return charClass.GetClass(static_cast<uchar>(ch));
}
/**
 * Used by commmands that want to select whole words.
 * Finds the start of word at pos when delta < 0 or the end of the word when delta >= 0.
 */
int Document::ExtendWordSelect(int pos, int delta, bool onlyWordCharacters) const
{
	CharClassify::cc ccStart = CharClassify::ccWord;
	if(delta < 0) {
		if(!onlyWordCharacters) {
			const CharacterExtracted ce = CharacterBefore(pos);
			ccStart = WordCharacterClass(ce.character);
		}
		while(pos > 0) {
			const CharacterExtracted ce = CharacterBefore(pos);
			if(WordCharacterClass(ce.character) != ccStart)
				break;
			pos -= ce.widthBytes;
		}
	}
	else {
		if(!onlyWordCharacters && pos < Length()) {
			const CharacterExtracted ce = CharacterAfter(pos);
			ccStart = WordCharacterClass(ce.character);
		}
		while(pos < Length()) {
			const CharacterExtracted ce = CharacterAfter(pos);
			if(WordCharacterClass(ce.character) != ccStart)
				break;
			pos += ce.widthBytes;
		}
	}
	return MovePositionOutsideChar(pos, delta, true);
}
/**
 * Find the start of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
int Document::NextWordStart(int pos, int delta) const
{
	if(delta < 0) {
		while(pos > 0) {
			const CharacterExtracted ce = CharacterBefore(pos);
			if(WordCharacterClass(ce.character) != CharClassify::ccSpace)
				break;
			pos -= ce.widthBytes;
		}
		if(pos > 0) {
			CharacterExtracted ce = CharacterBefore(pos);
			const CharClassify::cc ccStart = WordCharacterClass(ce.character);
			while(pos > 0) {
				ce = CharacterBefore(pos);
				if(WordCharacterClass(ce.character) != ccStart)
					break;
				pos -= ce.widthBytes;
			}
		}
	}
	else {
		CharacterExtracted ce = CharacterAfter(pos);
		const CharClassify::cc ccStart = WordCharacterClass(ce.character);
		while(pos < Length()) {
			ce = CharacterAfter(pos);
			if(WordCharacterClass(ce.character) != ccStart)
				break;
			pos += ce.widthBytes;
		}
		while(pos < Length()) {
			ce = CharacterAfter(pos);
			if(WordCharacterClass(ce.character) != CharClassify::ccSpace)
				break;
			pos += ce.widthBytes;
		}
	}
	return pos;
}

/**
 * Find the end of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
int Document::NextWordEnd(int pos, int delta) const
{
	if(delta < 0) {
		if(pos > 0) {
			CharacterExtracted ce = CharacterBefore(pos);
			CharClassify::cc ccStart = WordCharacterClass(ce.character);
			if(ccStart != CharClassify::ccSpace) {
				while(pos > 0) {
					ce = CharacterBefore(pos);
					if(WordCharacterClass(ce.character) != ccStart)
						break;
					pos -= ce.widthBytes;
				}
			}
			while(pos > 0) {
				ce = CharacterBefore(pos);
				if(WordCharacterClass(ce.character) != CharClassify::ccSpace)
					break;
				pos -= ce.widthBytes;
			}
		}
	}
	else {
		while(pos < Length()) {
			CharacterExtracted ce = CharacterAfter(pos);
			if(WordCharacterClass(ce.character) != CharClassify::ccSpace)
				break;
			pos += ce.widthBytes;
		}
		if(pos < Length()) {
			CharacterExtracted ce = CharacterAfter(pos);
			CharClassify::cc ccStart = WordCharacterClass(ce.character);
			while(pos < Length()) {
				ce = CharacterAfter(pos);
				if(WordCharacterClass(ce.character) != ccStart)
					break;
				pos += ce.widthBytes;
			}
		}
	}
	return pos;
}

/**
 * Check that the character at the given position is a word or punctuation character and that
 * the previous character is of a different character class.
 */
bool Document::IsWordStartAt(int pos) const
{
	if(pos >= Length())
		return false;
	else if(pos > 0) {
		const CharacterExtracted cePos = CharacterAfter(pos);
		const CharClassify::cc ccPos = WordCharacterClass(cePos.character);
		const CharacterExtracted cePrev = CharacterBefore(pos);
		const CharClassify::cc ccPrev = WordCharacterClass(cePrev.character);
		return oneof2(ccPos, CharClassify::ccWord, CharClassify::ccPunctuation) && (ccPos != ccPrev);
	}
	else
		return true;
}

/**
 * Check that the character at the given position is a word or punctuation character and that
 * the next character is of a different character class.
 */
bool Document::IsWordEndAt(int pos) const
{
	if(pos <= 0)
		return false;
	else if(pos < Length()) {
		const CharacterExtracted cePos = CharacterAfter(pos);
		const CharClassify::cc ccPos = WordCharacterClass(cePos.character);
		const CharacterExtracted cePrev = CharacterBefore(pos);
		const CharClassify::cc ccPrev = WordCharacterClass(cePrev.character);
		return oneof2(ccPrev, CharClassify::ccWord, CharClassify::ccPunctuation) && (ccPrev != ccPos);
	}
	else
		return true;
}

/**
 * Check that the given range is has transitions between character classes at both
 * ends and where the characters on the inside are word or punctuation characters.
 */
bool Document::IsWordAt(int start, int end) const { return (start < end) && IsWordStartAt(start) && IsWordEndAt(end); }
bool Document::MatchesWordOptions(bool word, bool wordStart, int pos, int length) const { return (!word && !wordStart) || (word && IsWordAt(pos, pos + length)) || (wordStart && IsWordStartAt(pos)); }
bool Document::HasCaseFolder() const { return pcf != 0; }

void Document::SetCaseFolder(CaseFolder * pcf_)
{
	delete pcf;
	pcf = pcf_;
}

Document::CharacterExtracted Document::ExtractCharacter(int position) const
{
	const uchar leadByte = static_cast<uchar>(cb.CharAt(position));
	if(UTF8IsAscii(leadByte)) {
		// Common case: ASCII character
		return CharacterExtracted(leadByte, 1);
	}
	const int widthCharBytes = UTF8BytesOfLead[leadByte];
	uchar charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
	for(int b = 1; b<widthCharBytes; b++)
		charBytes[b] = static_cast<uchar>(cb.CharAt(position + b));
	int utf8status = UTF8Classify(charBytes, widthCharBytes);
	if(utf8status & UTF8MaskInvalid) {
		// Treat as invalid and use up just one byte
		return CharacterExtracted(unicodeReplacementChar, 1);
	}
	else {
		return CharacterExtracted(UnicodeFromUTF8(charBytes), utf8status & UTF8MaskWidth);
	}
}

/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 * Has not been tested with backwards DBCS searches yet.
 */
long Document::FindText(int minPos, int maxPos, const char * search, int flags, int * length)
{
	if(*length <= 0)
		return minPos;
	const bool caseSensitive = (flags & SCFIND_MATCHCASE) != 0;
	const bool word = (flags & SCFIND_WHOLEWORD) != 0;
	const bool wordStart = (flags & SCFIND_WORDSTART) != 0;
	const bool regExp = (flags & SCFIND_REGEXP) != 0;
	if(regExp) {
		if(!regex)
			regex = CreateRegexSearch(&charClass);
		return regex->FindText(this, minPos, maxPos, search, caseSensitive, word, wordStart, flags, length);
	}
	else {
		const bool forward = minPos <= maxPos;
		const int increment = forward ? 1 : -1;

		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		const int startPos = MovePositionOutsideChar(minPos, increment, false);
		const int endPos = MovePositionOutsideChar(maxPos, increment, false);

		// Compute actual search ranges needed
		const int lengthFind = *length;

		//Platform::DebugPrintf("Find %d %d %s %d\n", startPos, endPos, ft->lpstrText, lengthFind);
		const int limitPos = smax(startPos, endPos);
		int pos = startPos;
		if(!forward) {
			// Back all of a character
			pos = NextPosition(pos, increment);
		}
		if(caseSensitive) {
			const int endSearch = (startPos <= endPos) ? endPos - lengthFind + 1 : endPos;
			const char charStartSearch =  search[0];
			while(forward ? (pos < endSearch) : (pos >= endSearch)) {
				if(CharAt(pos) == charStartSearch) {
					bool found = (pos + lengthFind) <= limitPos;
					for(int indexSearch = 1; (indexSearch < lengthFind) && found; indexSearch++) {
						found = CharAt(pos + indexSearch) == search[indexSearch];
					}
					if(found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
						return pos;
					}
				}
				if(!NextCharacter(pos, increment))
					break;
			}
		}
		else if(SC_CP_UTF8 == dbcsCodePage) {
			const size_t maxFoldingExpansion = 4;
			std::vector <char> searchThing(lengthFind * UTF8MaxBytes * maxFoldingExpansion + 1);
			const int lenSearch = static_cast<int>(
			    pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind));
			char bytes[UTF8MaxBytes + 1];
			char folded[UTF8MaxBytes * maxFoldingExpansion + 1];
			while(forward ? (pos < endPos) : (pos >= endPos)) {
				int widthFirstCharacter = 0;
				int posIndexDocument = pos;
				int indexSearch = 0;
				bool characterMatches = true;
				for(;;) {
					const uchar leadByte = static_cast<uchar>(cb.CharAt(posIndexDocument));
					bytes[0] = leadByte;
					int widthChar = 1;
					if(!UTF8IsAscii(leadByte)) {
						const int widthCharBytes = UTF8BytesOfLead[leadByte];
						for(int b = 1; b<widthCharBytes; b++) {
							bytes[b] = cb.CharAt(posIndexDocument+b);
						}
						widthChar =
						    UTF8Classify(reinterpret_cast<const uchar *>(bytes), widthCharBytes) & UTF8MaskWidth;
					}
					if(!widthFirstCharacter)
						widthFirstCharacter = widthChar;
					if((posIndexDocument + widthChar) > limitPos)
						break;
					const int lenFlat = static_cast<int>(pcf->Fold(folded, sizeof(folded), bytes, widthChar));
					folded[lenFlat] = 0;
					// Does folded match the buffer
					characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					if(!characterMatches)
						break;
					posIndexDocument += widthChar;
					indexSearch += lenFlat;
					if(indexSearch >= lenSearch)
						break;
				}
				if(characterMatches && (indexSearch == static_cast<int>(lenSearch))) {
					if(MatchesWordOptions(word, wordStart, pos, posIndexDocument - pos)) {
						*length = posIndexDocument - pos;
						return pos;
					}
				}
				if(forward) {
					pos += widthFirstCharacter;
				}
				else {
					if(!NextCharacter(pos, increment))
						break;
				}
			}
		}
		else if(dbcsCodePage) {
			const size_t maxBytesCharacter = 2;
			const size_t maxFoldingExpansion = 4;
			std::vector <char> searchThing(lengthFind * maxBytesCharacter * maxFoldingExpansion + 1);
			const int lenSearch = static_cast<int>(
			    pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind));
			while(forward ? (pos < endPos) : (pos >= endPos)) {
				int indexDocument = 0;
				int indexSearch = 0;
				bool characterMatches = true;
				while(characterMatches && ((pos + indexDocument) < limitPos) && (indexSearch < lenSearch)) {
					char bytes[maxBytesCharacter + 1];
					bytes[0] = cb.CharAt(pos + indexDocument);
					const int widthChar = IsDBCSLeadByte(bytes[0]) ? 2 : 1;
					if(widthChar == 2)
						bytes[1] = cb.CharAt(pos + indexDocument + 1);
					if((pos + indexDocument + widthChar) > limitPos)
						break;
					char folded[maxBytesCharacter * maxFoldingExpansion + 1];
					const int lenFlat = static_cast<int>(pcf->Fold(folded, sizeof(folded), bytes, widthChar));
					folded[lenFlat] = 0;
					// Does folded match the buffer
					characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					indexDocument += widthChar;
					indexSearch += lenFlat;
				}
				if(characterMatches && (indexSearch == static_cast<int>(lenSearch))) {
					if(MatchesWordOptions(word, wordStart, pos, indexDocument)) {
						*length = indexDocument;
						return pos;
					}
				}
				if(!NextCharacter(pos, increment))
					break;
			}
		}
		else {
			const int endSearch = (startPos <= endPos) ? endPos - lengthFind + 1 : endPos;
			std::vector <char> searchThing(lengthFind + 1);
			pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while(forward ? (pos < endSearch) : (pos >= endSearch)) {
				bool found = (pos + lengthFind) <= limitPos;
				for(int indexSearch = 0; (indexSearch < lengthFind) && found; indexSearch++) {
					char ch = CharAt(pos + indexSearch);
					char folded[2];
					pcf->Fold(folded, sizeof(folded), &ch, 1);
					found = folded[0] == searchThing[indexSearch];
				}
				if(found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
					return pos;
				}
				if(!NextCharacter(pos, increment))
					break;
			}
		}
	}
	//Platform::DebugPrintf("Not found\n");
	return -1;
}

const char * Document::SubstituteByPosition(const char * text, int * length)
{
	return regex ? regex->SubstituteByPosition(this, text, length) : 0;
}

int Document::LinesTotal() const
{
	return cb.Lines();
}

void Document::SetDefaultCharClasses(bool includeWordClass)
{
	charClass.SetDefaultCharClasses(includeWordClass);
}

void Document::SetCharClasses(const uchar * chars, CharClassify::cc newCharClass)
{
	charClass.SetCharClasses(chars, newCharClass);
}

int Document::GetCharsOfClass(CharClassify::cc characterClass, uchar * buffer) const
{
	return charClass.GetCharsOfClass(characterClass, buffer);
}

void SCI_METHOD Document::StartStyling(Sci_Position position, char)
{
	endStyled = position;
}

bool SCI_METHOD Document::SetStyleFor(Sci_Position length, char style)
{
	if(enteredStyling != 0) {
		return false;
	}
	else {
		enteredStyling++;
		int prevEndStyled = endStyled;
		if(cb.SetStyleFor(endStyled, length, style)) {
			DocModification mh(SC_MOD_CHANGESTYLE | SC_PERFORMED_USER, prevEndStyled, length);
			NotifyModified(mh);
		}
		endStyled += length;
		enteredStyling--;
		return true;
	}
}

bool SCI_METHOD Document::SetStyles(Sci_Position length, const char * styles)
{
	if(enteredStyling != 0) {
		return false;
	}
	else {
		enteredStyling++;
		bool didChange = false;
		int startMod = 0;
		int endMod = 0;
		for(int iPos = 0; iPos < length; iPos++, endStyled++) {
			PLATFORM_ASSERT(endStyled < Length());
			if(cb.SetStyleAt(endStyled, styles[iPos])) {
				if(!didChange) {
					startMod = endStyled;
				}
				didChange = true;
				endMod = endStyled;
			}
		}
		if(didChange) {
			DocModification mh(SC_MOD_CHANGESTYLE | SC_PERFORMED_USER, startMod, endMod - startMod + 1);
			NotifyModified(mh);
		}
		enteredStyling--;
		return true;
	}
}

void Document::EnsureStyledTo(int pos)
{
	if((enteredStyling == 0) && (pos > GetEndStyled())) {
		IncrementStyleClock();
		if(pli && !pli->UseContainerLexing()) {
			int lineEndStyled = LineFromPosition(GetEndStyled());
			int endStyledTo = LineStart(lineEndStyled);
			pli->Colourise(endStyledTo, pos);
		}
		else {
			// Ask the watchers to style, and stop as soon as one responds.
			for(std::vector <WatcherWithUserData>::iterator it = watchers.begin();
			    (pos > GetEndStyled()) && (it != watchers.end()); ++it) {
				it->watcher->NotifyStyleNeeded(this, it->userData, pos);
			}
		}
	}
}

void Document::StyleToAdjustingLineDuration(int pos)
{
	// Place bounds on the duration used to avoid glitches spiking it
	// and so causing slow styling or non-responsive scrolling
	const double minDurationOneLine = 0.000001;
	const double maxDurationOneLine = 0.0001;

	// Alpha value for exponential smoothing.
	// Most recent value contributes 25% to smoothed value.
	const double alpha = 0.25;

	const Sci_Position lineFirst = LineFromPosition(GetEndStyled());
	ElapsedTime etStyling;
	EnsureStyledTo(pos);
	const double durationStyling = etStyling.Duration();
	const Sci_Position lineLast = LineFromPosition(GetEndStyled());
	if(lineLast >= lineFirst + 8) {
		// Only adjust for styling multiple lines to avoid instability
		const double durationOneLine = durationStyling / (lineLast - lineFirst);
		durationStyleOneLine = alpha * durationOneLine + (1.0 - alpha) * durationStyleOneLine;
		if(durationStyleOneLine < minDurationOneLine) {
			durationStyleOneLine = minDurationOneLine;
		}
		else if(durationStyleOneLine > maxDurationOneLine) {
			durationStyleOneLine = maxDurationOneLine;
		}
	}
}

void Document::LexerChanged()
{
	// Tell the watchers the lexer has changed.
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it) {
		it->watcher->NotifyLexerChanged(this, it->userData);
	}
}

int SCI_METHOD Document::SetLineState(Sci_Position line, int state)
{
	int statePrevious = static_cast<LineState *>(perLineData[ldState])->SetLineState(line, state);
	if(state != statePrevious) {
		DocModification mh(SC_MOD_CHANGELINESTATE, LineStart(line), 0, 0, 0, line);
		NotifyModified(mh);
	}
	return statePrevious;
}

int SCI_METHOD Document::GetLineState(Sci_Position line) const
{
	return static_cast<LineState *>(perLineData[ldState])->GetLineState(line);
}

int Document::GetMaxLineState()
{
	return static_cast<LineState *>(perLineData[ldState])->GetMaxLineState();
}

void SCI_METHOD Document::ChangeLexerState(Sci_Position start, Sci_Position end)
{
	DocModification mh(SC_MOD_LEXERSTATE, start, end-start, 0, 0, 0);
	NotifyModified(mh);
}

Document::WatcherWithUserData::WatcherWithUserData(DocWatcher * watcher_, void * userData_) : watcher(watcher_), userData(userData_)
{
}

bool FASTCALL Document::WatcherWithUserData::operator == (const WatcherWithUserData &other) const
{
	return (watcher == other.watcher) && (userData == other.userData);
}

/*Document::StyledText::StyledText(size_t length_, const char * text_, bool multipleStyles_, int style_, const uchar * styles_) :
	length(length_), text(text_), multipleStyles(multipleStyles_), style(style_), styles(styles_)
{
}*/

Document::StyledText::StyledText(const LineAnnotation & rLA, int line) :
	length(rLA.Length(line)), text(rLA.Text(line)), multipleStyles(rLA.MultipleStyles(line)), style(rLA.Style(line)), styles(rLA.Styles(line))
{
}

size_t FASTCALL Document::StyledText::LineLength(size_t start) const
{
	size_t cur = start;
	while((cur < length) && (text[cur] != '\n'))
		cur++;
	return (cur-start);
}

size_t FASTCALL Document::StyledText::StyleAt(size_t i) const
{
	return multipleStyles ? styles[i] : style;
}

Document::StyledText Document::MarginStyledText(int line) const
{
	LineAnnotation * pla = static_cast<LineAnnotation *>(perLineData[ldMargin]);
	//return StyledText(pla->Length(line), pla->Text(line), pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
	return StyledText(*pla, line);
}

void Document::MarginSetText(int line, const char * text)
{
	static_cast <LineAnnotation *>(perLineData[ldMargin])->SetText(line, text);
	DocModification mh(SC_MOD_CHANGEMARGIN, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::MarginSetStyle(int line, int style)
{
	static_cast <LineAnnotation *>(perLineData[ldMargin])->SetStyle(line, style);
	DocModification dm(SC_MOD_CHANGEMARGIN, LineStart(line), 0, 0, 0, line);
	NotifyModified(dm);
}

void Document::MarginSetStyles(int line, const uchar * styles)
{
	static_cast <LineAnnotation *>(perLineData[ldMargin])->SetStyles(line, styles);
	DocModification dm(SC_MOD_CHANGEMARGIN, LineStart(line), 0, 0, 0, line);
	NotifyModified(dm);
}

void Document::MarginClearAll()
{
	int maxEditorLine = LinesTotal();
	for(int l = 0; l<maxEditorLine; l++)
		MarginSetText(l, 0);
	// Free remaining data
	static_cast<LineAnnotation *>(perLineData[ldMargin])->ClearAll();
}

Document::StyledText Document::AnnotationStyledText(int line) const
{
	LineAnnotation * pla = static_cast<LineAnnotation *>(perLineData[ldAnnotation]);
	//return StyledText(pla->Length(line), pla->Text(line), pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
	return StyledText(*pla, line);
}

void Document::AnnotationSetText(int line, const char * text)
{
	if(line >= 0 && line < LinesTotal()) {
		const int linesBefore = AnnotationLines(line);
		static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetText(line, text);
		const int linesAfter = AnnotationLines(line);
		DocModification mh(SC_MOD_CHANGEANNOTATION, LineStart(line), 0, 0, 0, line);
		mh.annotationLinesAdded = linesAfter - linesBefore;
		NotifyModified(mh);
	}
}

void Document::AnnotationSetStyle(int line, int style)
{
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetStyle(line, style);
	DocModification mh(SC_MOD_CHANGEANNOTATION, LineStart(line), 0, 0, 0, line);
	NotifyModified(mh);
}

void Document::AnnotationSetStyles(int line, const uchar * styles)
{
	if(line >= 0 && line < LinesTotal()) {
		static_cast<LineAnnotation *>(perLineData[ldAnnotation])->SetStyles(line, styles);
	}
}

int FASTCALL Document::AnnotationLines(int line) const
{
	return static_cast<LineAnnotation *>(perLineData[ldAnnotation])->Lines(line);
}

void Document::AnnotationClearAll()
{
	int maxEditorLine = LinesTotal();
	for(int l = 0; l<maxEditorLine; l++)
		AnnotationSetText(l, 0);
	// Free remaining data
	static_cast<LineAnnotation *>(perLineData[ldAnnotation])->ClearAll();
}

void Document::IncrementStyleClock()
{
	styleClock = (styleClock + 1) % 0x100000;
}

void SCI_METHOD Document::DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength)
{
	if(decorations.FillRange(position, value, fillLength)) {
		DocModification mh(SC_MOD_CHANGEINDICATOR | SC_PERFORMED_USER, position, fillLength);
		NotifyModified(mh);
	}
}

bool Document::AddWatcher(DocWatcher * watcher, void * userData)
{
	WatcherWithUserData wwud(watcher, userData);
	std::vector <WatcherWithUserData>::iterator it = std::find(watchers.begin(), watchers.end(), wwud);
	if(it != watchers.end())
		return false;
	else {
		watchers.push_back(wwud);
		return true;
	}
}

bool Document::RemoveWatcher(DocWatcher * watcher, void * userData)
{
	std::vector <WatcherWithUserData>::iterator it = std::find(watchers.begin(), watchers.end(), WatcherWithUserData(watcher, userData));
	if(it != watchers.end()) {
		watchers.erase(it);
		return true;
	}
	else
		return false;
}

void Document::NotifyModifyAttempt()
{
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it) {
		it->watcher->NotifyModifyAttempt(this, it->userData);
	}
}

void Document::NotifySavePoint(bool atSavePoint)
{
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it) {
		it->watcher->NotifySavePoint(this, it->userData, atSavePoint);
	}
}

void FASTCALL Document::NotifyModified(const DocModification & mh)
{
	if(mh.modificationType & SC_MOD_INSERTTEXT)
		decorations.InsertSpace(mh.position, mh.length);
	else if(mh.modificationType & SC_MOD_DELETETEXT)
		decorations.DeleteRange(mh.position, mh.length);
	for(std::vector <WatcherWithUserData>::iterator it = watchers.begin(); it != watchers.end(); ++it)
		it->watcher->NotifyModified(this, mh, it->userData);
}

// Used for word part navigation.
static bool FASTCALL IsASCIIPunctuationCharacter(uint ch)
{
	switch(ch) {
		case '!': case '"': case '#': case '$': case '%': case '&': case '\'': case '(':
		case ')': case '*': case '+': case ',': case '-': case '.': case '/': case ':':
		case ';': case '<': case '=': case '>': case '?': case '@': case '[': case '\\':
		case ']': case '^': case '_': case '`': case '{': case '|': case '}': case '~':
		    return true;
		default:
		    return false;
	}
}

bool Document::IsWordPartSeparator(uint ch) const
{
	return (WordCharacterClass(ch) == CharClassify::ccWord) && IsASCIIPunctuationCharacter(ch);
}

int Document::WordPartLeft(int pos) const
{
	if(pos > 0) {
		pos -= CharacterBefore(pos).widthBytes;
		CharacterExtracted ceStart = CharacterAfter(pos);
		if(IsWordPartSeparator(ceStart.character)) {
			while(pos > 0 && IsWordPartSeparator(CharacterAfter(pos).character)) {
				pos -= CharacterBefore(pos).widthBytes;
			}
		}
		if(pos > 0) {
			ceStart = CharacterAfter(pos);
			pos -= CharacterBefore(pos).widthBytes;
			if(isasciilwr(ceStart.character)) {
				while(pos > 0 && isasciilwr(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(!isasciiupr(CharacterAfter(pos).character) && !isasciilwr(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else if(isasciiupr(ceStart.character)) {
				while(pos > 0 && isasciiupr(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(!isasciiupr(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else if(isdec(ceStart.character)) {
				while(pos > 0 && isdec(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(!isdec(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else if(IsASCIIPunctuationCharacter(ceStart.character)) {
				while(pos > 0 && IsASCIIPunctuationCharacter(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(!IsASCIIPunctuationCharacter(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else if(isspacechar(ceStart.character)) {
				while(pos > 0 && isspacechar(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(!isspacechar(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else if(!IsASCII(ceStart.character)) {
				while(pos > 0 && !IsASCII(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if(IsASCII(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			}
			else {
				pos += CharacterAfter(pos).widthBytes;
			}
		}
	}
	return pos;
}

int Document::WordPartRight(int pos) const
{
	CharacterExtracted ceStart = CharacterAfter(pos);
	const int length = Length();
	if(IsWordPartSeparator(ceStart.character)) {
		while(pos < length && IsWordPartSeparator(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
		ceStart = CharacterAfter(pos);
	}
	if(!IsASCII(ceStart.character)) {
		while(pos < length && !IsASCII(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	}
	else if(isasciilwr(ceStart.character)) {
		while(pos < length && isasciilwr(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	}
	else if(isasciiupr(ceStart.character)) {
		if(isasciilwr(CharacterAfter(pos + ceStart.widthBytes).character)) {
			pos += CharacterAfter(pos).widthBytes;
			while(pos < length && isasciilwr(CharacterAfter(pos).character))
				pos += CharacterAfter(pos).widthBytes;
		}
		else {
			while(pos < length && isasciiupr(CharacterAfter(pos).character))
				pos += CharacterAfter(pos).widthBytes;
		}
		if(isasciilwr(CharacterAfter(pos).character) && isasciiupr(CharacterBefore(pos).character))
			pos -= CharacterBefore(pos).widthBytes;
	}
	else if(isdec(ceStart.character)) {
		while(pos < length && isdec(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	}
	else if(IsASCIIPunctuationCharacter(ceStart.character)) {
		while(pos < length && IsASCIIPunctuationCharacter(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	}
	else if(isspacechar(ceStart.character)) {
		while(pos < length && isspacechar(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	}
	else {
		pos += CharacterAfter(pos).widthBytes;
	}
	return pos;
}

static bool FASTCALL IsLineEndChar(char c)
{
	return (c == '\n' || c == '\r');
}

int Document::ExtendStyleRange(int pos, int delta, bool singleLine)
{
	int sStart = cb.StyleAt(pos);
	if(delta < 0) {
		while(pos > 0 && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))))
			pos--;
		pos++;
	}
	else {
		while(pos < (Length()) && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsLineEndChar(cb.CharAt(pos))))
			pos++;
	}
	return pos;
}

static char FASTCALL BraceOpposite(char ch)
{
	switch(ch) {
		case '(': return ')';
		case ')': return '(';
		case '[': return ']';
		case ']': return '[';
		case '{': return '}';
		case '}': return '{';
		case '<': return '>';
		case '>': return '<';
		default: return '\0';
	}
}

// @todo should be able to extend styled region to find matching brace
int Document::BraceMatch(int position, int /*maxReStyle*/)
{
	char chBrace = CharAt(position);
	char chSeek = BraceOpposite(chBrace);
	if(chSeek == '\0')
		return -1;
	const int styBrace = StyleIndexAt(position);
	int direction = -1;
	if(chBrace == '(' || chBrace == '[' || chBrace == '{' || chBrace == '<')
		direction = 1;
	int depth = 1;
	position = NextPosition(position, direction);
	while((position >= 0) && (position < Length())) {
		char chAtPos = CharAt(position);
		const int styAtPos = StyleIndexAt(position);
		if((position > GetEndStyled()) || (styAtPos == styBrace)) {
			if(chAtPos == chBrace)
				depth++;
			if(chAtPos == chSeek)
				depth--;
			if(depth == 0)
				return position;
		}
		int positionBeforeMove = position;
		position = NextPosition(position, direction);
		if(position == positionBeforeMove)
			break;
	}
	return -1;
}
/**
 * Implementation of RegexSearchBase for the default built-in regular expression engine
 */
class BuiltinRegex : public RegexSearchBase {
public:
	explicit BuiltinRegex(CharClassify * charClassTable) : search(charClassTable)
	{
	}
	virtual ~BuiltinRegex()
	{
	}
	virtual long FindText(Document * doc, int minPos, int maxPos, const char * s, bool caseSensitive, bool word, bool wordStart, int flags, int * length);
	virtual const char * SubstituteByPosition(Document * doc, const char * text, int * length);
private:
	RESearch search;
	std::string substituted;
};

namespace {
/**
 * RESearchRange keeps track of search range.
 */
class RESearchRange {
public:
	const Document * doc;
	int increment;
	int startPos;
	int endPos;
	int lineRangeStart;
	int lineRangeEnd;
	int lineRangeBreak;
	RESearchRange(const Document * doc_, int minPos, int maxPos) : doc(doc_)
	{
		increment = (minPos <= maxPos) ? 1 : -1;
		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		startPos = doc->MovePositionOutsideChar(minPos, 1, false);
		endPos = doc->MovePositionOutsideChar(maxPos, 1, false);
		lineRangeStart = doc->LineFromPosition(startPos);
		lineRangeEnd = doc->LineFromPosition(endPos);
		if((increment == 1) && (startPos >= doc->LineEnd(lineRangeStart)) && (lineRangeStart < lineRangeEnd)) {
			// the start position is at end of line or between line end characters.
			lineRangeStart++;
			startPos = doc->LineStart(lineRangeStart);
		}
		else if((increment == -1) && (startPos <= doc->LineStart(lineRangeStart)) && (lineRangeStart > lineRangeEnd)) {
			// the start position is at beginning of line.
			lineRangeStart--;
			startPos = doc->LineEnd(lineRangeStart);
		}
		lineRangeBreak = lineRangeEnd + increment;
	}
	Range LineRange(int line) const
	{
		Range range(doc->LineStart(line), doc->LineEnd(line));
		if(increment == 1) {
			if(line == lineRangeStart)
				range.start = startPos;
			if(line == lineRangeEnd)
				range.end = endPos;
		}
		else {
			if(line == lineRangeEnd)
				range.start = endPos;
			if(line == lineRangeStart)
				range.end = startPos;
		}
		return range;
	}
};

// Define a way for the Regular Expression code to access the document
class DocumentIndexer : public CharacterIndexer {
	Document * pdoc;
	int end;
public:
	DocumentIndexer(Document * pdoc_, int end_) : pdoc(pdoc_), end(end_)
	{
	}
	virtual ~DocumentIndexer()
	{
	}
	virtual char CharAt(int index)
	{
		return (index < 0 || index >= end) ? 0 : pdoc->CharAt(index);
	}
};

#ifndef NO_CXX11_REGEX

class ByteIterator : public std::iterator<std::bidirectional_iterator_tag, char> {
public:
	const Document * doc;
	Position position;
	ByteIterator(const Document * doc_ = 0, Position position_ = 0) : doc(doc_), position(position_)
	{
	}
	ByteIterator(const ByteIterator &other) NOEXCEPT
	{
		doc = other.doc;
		position = other.position;
	}
	ByteIterator & operator = (const ByteIterator &other)
	{
		if(this != &other) {
			doc = other.doc;
			position = other.position;
		}
		return *this;
	}
	char operator*() const
	{
		return doc->CharAt(position);
	}
	ByteIterator & operator++()
	{
		position++;
		return *this;
	}
	ByteIterator operator++(int)
	{
		ByteIterator retVal(*this);
		position++;
		return retVal;
	}
	ByteIterator & operator--()
	{
		position--;
		return *this;
	}
	bool operator == (const ByteIterator &other) const
	{
		return doc == other.doc && position == other.position;
	}
	bool operator != (const ByteIterator &other) const
	{
		return doc != other.doc || position != other.position;
	}
	int Pos() const
	{
		return position;
	}
	int PosRoundUp() const
	{
		return position;
	}
};

// On Windows, wchar_t is 16 bits wide and on Unix it is 32 bits wide.
// Would be better to use sizeof(wchar_t) or similar to differentiate
// but easier for now to hard-code platforms.
// C++11 has char16_t and char32_t but neither Clang nor Visual C++
// appear to allow specializing basic_regex over these.

#ifdef _WIN32
	#define WCHAR_T_IS_16 1
#else
	#define WCHAR_T_IS_16 0
#endif

#if WCHAR_T_IS_16

// On Windows, report non-BMP characters as 2 separate surrogates as that
// matches wregex since it is based on wchar_t.
class UTF8Iterator : public std::iterator<std::bidirectional_iterator_tag, wchar_t> {
	// These 3 fields determine the iterator position and are used for comparisons
	const Document * doc;
	Position position;
	size_t characterIndex;
	// Remaining fields are derived from the determining fields so are excluded in comparisons
	uint lenBytes;
	size_t lenCharacters;
	wchar_t buffered[2];
public:
	UTF8Iterator(const Document * doc_ = 0, Position position_ = 0) :
		doc(doc_), position(position_), characterIndex(0), lenBytes(0), lenCharacters(0)
	{
		buffered[0] = 0;
		buffered[1] = 0;
		if(doc) {
			ReadCharacter();
		}
	}
	UTF8Iterator(const UTF8Iterator &other)
	{
		doc = other.doc;
		position = other.position;
		characterIndex = other.characterIndex;
		lenBytes = other.lenBytes;
		lenCharacters = other.lenCharacters;
		buffered[0] = other.buffered[0];
		buffered[1] = other.buffered[1];
	}
	UTF8Iterator & operator = (const UTF8Iterator &other)
	{
		if(this != &other) {
			doc = other.doc;
			position = other.position;
			characterIndex = other.characterIndex;
			lenBytes = other.lenBytes;
			lenCharacters = other.lenCharacters;
			buffered[0] = other.buffered[0];
			buffered[1] = other.buffered[1];
		}
		return *this;
	}
	wchar_t operator * () const
	{
		assert(lenCharacters != 0);
		return buffered[characterIndex];
	}
	UTF8Iterator & operator++()
	{
		if((characterIndex + 1) < (lenCharacters)) {
			characterIndex++;
		}
		else {
			position += lenBytes;
			ReadCharacter();
			characterIndex = 0;
		}
		return *this;
	}
	UTF8Iterator operator++(int)
	{
		UTF8Iterator retVal(*this);
		if((characterIndex + 1) < (lenCharacters)) {
			characterIndex++;
		}
		else {
			position += lenBytes;
			ReadCharacter();
			characterIndex = 0;
		}
		return retVal;
	}
	UTF8Iterator & operator--()
	{
		if(characterIndex) {
			characterIndex--;
		}
		else {
			position = doc->NextPosition(position, -1);
			ReadCharacter();
			characterIndex = lenCharacters - 1;
		}
		return *this;
	}
	bool operator == (const UTF8Iterator &other) const
	{
		// Only test the determining fields, not the character widths and values derived from this
		return doc == other.doc && position == other.position && characterIndex == other.characterIndex;
	}
	bool operator != (const UTF8Iterator &other) const
	{
		// Only test the determining fields, not the character widths and values derived from this
		return doc != other.doc || position != other.position || characterIndex != other.characterIndex;
	}
	int Pos() const
	{
		return position;
	}
	int PosRoundUp() const
	{
		return characterIndex ? (position + lenBytes) /* Force to end of character */ : position;
	}
private:
	void ReadCharacter()
	{
		Document::CharacterExtracted charExtracted = doc->ExtractCharacter(position);
		lenBytes = charExtracted.widthBytes;
		if(charExtracted.character == unicodeReplacementChar) {
			lenCharacters = 1;
			buffered[0] = static_cast<wchar_t>(charExtracted.character);
		}
		else {
			lenCharacters = UTF16FromUTF32Character(charExtracted.character, buffered);
		}
	}
};

#else

// On Unix, report non-BMP characters as single characters

class UTF8Iterator : public std::iterator<std::bidirectional_iterator_tag, wchar_t> {
	const Document * doc;
	Position position;
public:
	UTF8Iterator(const Document * doc_ = 0, Position position_ = 0) : doc(doc_), position(position_)
	{
	}
	UTF8Iterator(const UTF8Iterator &other) NOEXCEPT
	{
		doc = other.doc;
		position = other.position;
	}
	UTF8Iterator & operator = (const UTF8Iterator &other)
	{
		if(this != &other) {
			doc = other.doc;
			position = other.position;
		}
		return *this;
	}
	wchar_t operator*() const
	{
		Document::CharacterExtracted charExtracted = doc->ExtractCharacter(position);
		return charExtracted.character;
	}
	UTF8Iterator & operator++()
	{
		position = doc->NextPosition(position, 1);
		return *this;
	}
	UTF8Iterator operator++(int)
	{
		UTF8Iterator retVal(*this);
		position = doc->NextPosition(position, 1);
		return retVal;
	}
	UTF8Iterator & operator--()
	{
		position = doc->NextPosition(position, -1);
		return *this;
	}
	bool operator == (const UTF8Iterator &other) const
	{
		return doc == other.doc && position == other.position;
	}
	bool operator != (const UTF8Iterator &other) const
	{
		return doc != other.doc || position != other.position;
	}
	int Pos() const
	{
		return position;
	}
	int PosRoundUp() const
	{
		return position;
	}
};

#endif

std::regex_constants::match_flag_type MatchFlags(const Document * doc, int startPos, int endPos)
{
	std::regex_constants::match_flag_type flagsMatch = std::regex_constants::match_default;
	if(!doc->IsLineStartPosition(startPos))
		flagsMatch |= std::regex_constants::match_not_bol;
	if(!doc->IsLineEndPosition(endPos))
		flagsMatch |= std::regex_constants::match_not_eol;
	return flagsMatch;
}

template <typename Iterator, typename Regex>
bool MatchOnLines(const Document * doc, const Regex &regexp, const RESearchRange &resr, RESearch &search)
{
	bool matched = false;
	std::match_results<Iterator> match;

	// MSVC and libc++ have problems with ^ and $ matching line ends inside a range
	// If they didn't then the line by line iteration could be removed for the forwards
	// case and replaced with the following 4 lines:
	//	Iterator uiStart(doc, startPos);
	//	Iterator uiEnd(doc, endPos);
	//	flagsMatch = MatchFlags(doc, startPos, endPos);
	//	matched = std::regex_search(uiStart, uiEnd, match, regexp, flagsMatch);

	// Line by line.
	for(int line = resr.lineRangeStart; line != resr.lineRangeBreak; line += resr.increment) {
		const Range lineRange = resr.LineRange(line);
		Iterator itStart(doc, lineRange.start);
		Iterator itEnd(doc, lineRange.end);
		std::regex_constants::match_flag_type flagsMatch = MatchFlags(doc, lineRange.start, lineRange.end);
		matched = std::regex_search(itStart, itEnd, match, regexp, flagsMatch);
		// Check for the last match on this line.
		if(matched) {
			if(resr.increment == -1) {
				while(matched) {
					Iterator itNext(doc, match[0].second.PosRoundUp());
					flagsMatch = MatchFlags(doc, itNext.Pos(), lineRange.end);
					std::match_results<Iterator> matchNext;
					matched = std::regex_search(itNext, itEnd, matchNext, regexp, flagsMatch);
					if(matched) {
						if(match[0].first == match[0].second) {
							// Empty match means failure so exit
							return false;
						}
						match = matchNext;
					}
				}
				matched = true;
			}
			break;
		}
	}
	if(matched) {
		for(size_t co = 0; co < match.size(); co++) {
			search.bopat[co] = match[co].first.Pos();
			search.eopat[co] = match[co].second.PosRoundUp();
			Sci::Position lenMatch = search.eopat[co] - search.bopat[co];
			search.pat[co].resize(lenMatch);
			for(Sci::Position iPos = 0; iPos < lenMatch; iPos++) {
				search.pat[co][iPos] = doc->CharAt(iPos + search.bopat[co]);
			}
		}
	}
	return matched;
}

long Cxx11RegexFindText(Document * doc, int minPos, int maxPos, const char * s, bool caseSensitive, int * length, RESearch &search)
{
	const RESearchRange resr(doc, minPos, maxPos);
	try {
		//ElapsedTime et;
		std::regex::flag_type flagsRe = std::regex::ECMAScript;
		// Flags that apper to have no effect:
		// | std::regex::collate | std::regex::extended;
		if(!caseSensitive)
			flagsRe = flagsRe | std::regex::icase;

		// Clear the RESearch so can fill in matches
		search.Clear();

		bool matched = false;
		if(SC_CP_UTF8 == doc->dbcsCodePage) {
			uint lenS = static_cast<uint>(sstrlen(s));
			std::vector <wchar_t> ws(lenS + 1);
#if WCHAR_T_IS_16
			size_t outLen = UTF16FromUTF8(s, lenS, &ws[0], lenS);
#else
			size_t outLen = UTF32FromUTF8(s, lenS, reinterpret_cast<uint *>(&ws[0]), lenS);
#endif
			ws[outLen] = 0;
			std::wregex regexp;
#if defined(__APPLE__)
			// Using a UTF-8 locale doesn't change to Unicode over a byte buffer so '.'
			// is one byte not one character.
			// However, on OS X this makes wregex act as Unicode
			std::locale localeU("en_US.UTF-8");
			regexp.imbue(localeU);
#endif
			regexp.assign(&ws[0], flagsRe);
			matched = MatchOnLines<UTF8Iterator>(doc, regexp, resr, search);
		}
		else {
			std::regex regexp;
			regexp.assign(s, flagsRe);
			matched = MatchOnLines<ByteIterator>(doc, regexp, resr, search);
		}

		int posMatch = -1;
		if(matched) {
			posMatch = search.bopat[0];
			*length = search.eopat[0] - search.bopat[0];
		}
		// Example - search in doc/ScintillaHistory.html for
		// [[:upper:]]eta[[:space:]]
		// On MacBook, normally around 1 second but with locale imbued -> 14 seconds.
		//double durSearch = et.Duration(true);
		//Platform::DebugPrintf("Search:%9.6g \n", durSearch);
		return posMatch;
	} catch(std::regex_error &) {
		// Failed to create regular expression
		throw RegexError();
	} catch(...) {
		// Failed in some other way
		return -1;
	}
}

#endif
}

long BuiltinRegex::FindText(Document * doc, int minPos, int maxPos, const char * s, bool caseSensitive, bool, bool, int flags, int * length)
{
#ifndef NO_CXX11_REGEX
	if(flags & SCFIND_CXX11REGEX) {
		return Cxx11RegexFindText(doc, minPos, maxPos, s, caseSensitive, length, search);
	}
#endif
	const RESearchRange resr(doc, minPos, maxPos);
	const bool posix = (flags & SCFIND_POSIX) != 0;
	const char * errmsg = search.Compile(s, *length, caseSensitive, posix);
	if(errmsg) {
		return -1;
	}
	// Find a variable in a property file: \$(\([A-Za-z0-9_.]+\))
	// Replace first '.' with '-' in each property file variable reference:
	//     Search: \$(\([A-Za-z0-9_-]+\)\.\([A-Za-z0-9_.]+\))
	//     Replace: $(\1-\2)
	int pos = -1;
	int lenRet = 0;
	const char searchEnd = s[*length - 1];
	const char searchEndPrev = (*length > 1) ? s[*length - 2] : '\0';
	for(int line = resr.lineRangeStart; line != resr.lineRangeBreak; line += resr.increment) {
		int startOfLine = doc->LineStart(line);
		int endOfLine = doc->LineEnd(line);
		if(resr.increment == 1) {
			if(line == resr.lineRangeStart) {
				if((resr.startPos != startOfLine) && (s[0] == '^'))
					continue;       // Can't match start of line if start position after start of line
				startOfLine = resr.startPos;
			}
			if(line == resr.lineRangeEnd) {
				if((resr.endPos != endOfLine) && (searchEnd == '$') && (searchEndPrev != '\\'))
					continue;       // Can't match end of line if end position before end of line
				endOfLine = resr.endPos;
			}
		}
		else {
			if(line == resr.lineRangeEnd) {
				if((resr.endPos != startOfLine) && (s[0] == '^'))
					continue;       // Can't match start of line if end position after start of line
				startOfLine = resr.endPos;
			}
			if(line == resr.lineRangeStart) {
				if((resr.startPos != endOfLine) && (searchEnd == '$') && (searchEndPrev != '\\'))
					continue;       // Can't match end of line if start position before end of line
				endOfLine = resr.startPos;
			}
		}

		DocumentIndexer di(doc, endOfLine);
		int success = search.Execute(di, startOfLine, endOfLine);
		if(success) {
			pos = search.bopat[0];
			// Ensure only whole characters selected
			search.eopat[0] = doc->MovePositionOutsideChar(search.eopat[0], 1, false);
			lenRet = search.eopat[0] - search.bopat[0];
			// There can be only one start of a line, so no need to look for last match in line
			if((resr.increment == -1) && (s[0] != '^')) {
				// Check for the last match on this line.
				int repetitions = 1000; // Break out of infinite loop
				while(success && (search.eopat[0] <= endOfLine) && (repetitions--)) {
					success = search.Execute(di, pos+1, endOfLine);
					if(success) {
						if(search.eopat[0] <= minPos) {
							pos = search.bopat[0];
							lenRet = search.eopat[0] - search.bopat[0];
						}
						else {
							success = 0;
						}
					}
				}
			}
			break;
		}
	}
	*length = lenRet;
	return pos;
}

const char * BuiltinRegex::SubstituteByPosition(Document * doc, const char * text, int * length)
{
	substituted.clear();
	DocumentIndexer di(doc, doc->Length());
	search.GrabMatches(di);
	for(int j = 0; j < *length; j++) {
		if(text[j] == '\\') {
			if(text[j + 1] >= '0' && text[j + 1] <= '9') {
				uint patNum = text[j + 1] - '0';
				uint len = search.eopat[patNum] - search.bopat[patNum];
				if(!search.pat[patNum].empty())         // Will be null if try for a match that did not occur
					substituted.append(search.pat[patNum].c_str(), len);
				j++;
			}
			else {
				j++;
				switch(text[j]) {
					case 'a': substituted.push_back('\a'); break;
					case 'b': substituted.push_back('\b'); break;
					case 'f': substituted.push_back('\f'); break;
					case 'n': substituted.push_back('\n'); break;
					case 'r': substituted.push_back('\r'); break;
					case 't': substituted.push_back('\t'); break;
					case 'v': substituted.push_back('\v'); break;
					case '\\': substituted.push_back('\\'); break;
					default: substituted.push_back('\\'); j--;
				}
			}
		}
		else {
			substituted.push_back(text[j]);
		}
	}
	*length = static_cast<int>(substituted.length());
	return substituted.c_str();
}

#ifndef SCI_OWNREGEX
	#ifdef SCI_NAMESPACE
		RegexSearchBase * Scintilla::CreateRegexSearch(CharClassify * charClassTable)
		{
			return new BuiltinRegex(charClassTable);
		}
	#else
		RegexSearchBase * CreateRegexSearch(CharClassify * charClassTable)
		{
			return new BuiltinRegex(charClassTable);
		}
	#endif
#endif
