// Scintilla source code edit control
/** @file CellBuffer.cxx
** Manages a buffer of cells.
**/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

Document::CellBuffer::LineVector::LineVector() : starts(256), perLine(0)
{
	Init();
}

Document::CellBuffer::LineVector::~LineVector()
{
	starts.DeleteAll();
}

void Document::CellBuffer::LineVector::Init()
{
	starts.DeleteAll();
	CALLPTRMEMB(perLine, Init());
}

void Document::CellBuffer::LineVector::InsertLine(int line, int position, bool lineStart)
{
	starts.InsertPartition(line, position);
	if(perLine) {
		if((line > 0) && lineStart)
			line--;
		perLine->InsertLine(line);
	}
}

void FASTCALL Document::CellBuffer::LineVector::RemoveLine(int line)
{
	starts.RemovePartition(line);
	CALLPTRMEMB(perLine, RemoveLine(line));
}

void FASTCALL Document::CellBuffer::LineVector::SetPerLine(PerLine * pl) { perLine = pl; }
void Document::CellBuffer::LineVector::InsertText(int line, int delta) { starts.InsertText(line, delta); }
void Document::CellBuffer::LineVector::SetLineStart(int line, int position) { starts.SetPartitionStartPosition(line, position); }
int Document::CellBuffer::LineVector::Lines() const { return starts.Partitions(); }
int FASTCALL Document::CellBuffer::LineVector::LineFromPosition(int pos) const { return starts.PartitionFromPosition(pos); }
int FASTCALL Document::CellBuffer::LineVector::LineStart(int line) const { return starts.PositionFromPartition(line); }

UndoHistory::Action::Action() : at(tStart), position(0), data(0), lenData(0), mayCoalesce(false)
{
}

UndoHistory::Action::~Action()
{
	Destroy();
}

void UndoHistory::Action::Create(actionType at_, int position_, const char * data_, int lenData_, bool mayCoalesce_)
{
	delete [] data;
	data = NULL;
	position = position_;
	at = at_;
	if(lenData_) {
		data = new char[lenData_];
		memcpy(data, data_, lenData_);
	}
	lenData = lenData_;
	mayCoalesce = mayCoalesce_;
}

void UndoHistory::Action::Destroy()
{
	ZDELETEARRAY(data);
}

void UndoHistory::Action::Grab(Action * source)
{
	delete [] data;
	position = source->position;
	at = source->at;
	data = source->data;
	lenData = source->lenData;
	mayCoalesce = source->mayCoalesce;
	// Ownership of source data transferred to this
	source->position = 0;
	source->at = tStart;
	source->data = 0;
	source->lenData = 0;
	source->mayCoalesce = true;
}

// The undo history stores a sequence of user operations that represent the user's view of the
// commands executed on the text.
// Each user operation contains a sequence of text insertion and text deletion actions.
// All the user operations are stored in a list of individual actions with 'start' actions used
// as delimiters between user operations.
// Initially there is one start action in the history.
// As each action is performed, it is recorded in the history. The action may either become
// part of the current user operation or may start a new user operation. If it is to be part of the
// current operation, then it overwrites the current last action. If it is to be part of a new
// operation, it is appended after the current last action.
// After writing the new action, a new start action is appended at the end of the history.
// The decision of whether to start a new user operation is based upon two factors. If a
// compound operation has been explicitly started by calling BeginUndoAction and no matching
// EndUndoAction (these calls nest) has been called, then the action is coalesced into the current
// operation. If there is no outstanding BeginUndoAction call then a new operation is started
// unless it looks as if the new action is caused by the user typing or deleting a stream of text.
// Sequences that look like typing or deletion are coalesced into a single user operation.

UndoHistory::UndoHistory() : lenActions(100), maxAction(0), currentAction(0), undoSequenceDepth(0), savePoint(0), tentativePoint(-1)
{
	actions = new Action[lenActions];
	actions[currentAction].Create(Action::tStart);
}

UndoHistory::~UndoHistory()
{
	delete []actions;
	actions = 0;
}

void UndoHistory::EnsureUndoRoom()
{
	// Have to test that there is room for 2 more actions in the array
	// as two actions may be created by the calling function
	if(currentAction >= (lenActions - 2)) {
		// Run out of undo nodes so extend the array
		int lenActionsNew = lenActions * 2;
		Action * actionsNew = new Action[lenActionsNew];
		for(int act = 0; act <= currentAction; act++)
			actionsNew[act].Grab(&actions[act]);
		delete []actions;
		lenActions = lenActionsNew;
		actions = actionsNew;
	}
}

const char * UndoHistory::AppendAction(Action::actionType at, int position, const char * data, int lengthData,
    bool &startSequence, bool mayCoalesce)
{
	EnsureUndoRoom();
	//Platform::DebugPrintf("%% %d action %d %d %d\n", at, position, lengthData, currentAction);
	//Platform::DebugPrintf("^ %d action %d %d\n", actions[currentAction - 1].at,
	//	actions[currentAction - 1].position, actions[currentAction - 1].lenData);
	if(currentAction < savePoint) {
		savePoint = -1;
	}
	int oldCurrentAction = currentAction;
	if(currentAction >= 1) {
		if(0 == undoSequenceDepth) {
			// Top level actions may not always be coalesced
			int targetAct = -1;
			const Action * actPrevious = &(actions[currentAction + targetAct]);
			// Container actions may forward the coalesce state of Scintilla Actions.
			while((actPrevious->at == Action::tContainer) && actPrevious->mayCoalesce) {
				targetAct--;
				actPrevious = &(actions[currentAction + targetAct]);
			}
			// See if current action can be coalesced into previous action
			// Will work if both are inserts or deletes and position is same
#if defined(_MSC_VER) && defined(_PREFAST_)
			// Visual Studio 2013 Code Analysis wrongly believes actions can be NULL at its next reference
			__analysis_assume(actions);
#endif
			if(oneof2(currentAction, savePoint, tentativePoint)) {
				currentAction++;
			}
			else if(!actions[currentAction].mayCoalesce) {
				// Not allowed to coalesce if this set
				currentAction++;
			}
			else if(!mayCoalesce || !actPrevious->mayCoalesce) {
				currentAction++;
			}
			else if(at == Action::tContainer || actions[currentAction].at == Action::tContainer) {
				;       // A coalescible containerAction
			}
			else if((at != actPrevious->at) && (actPrevious->at != Action::tStart)) {
				currentAction++;
			}
			else if((at == Action::tInsert) && (position != (actPrevious->position + actPrevious->lenData))) {
				// Insertions must be immediately after to coalesce
				currentAction++;
			}
			else if(at == Action::tRemove) {
				if((lengthData == 1) || (lengthData == 2)) {
					if((position + lengthData) == actPrevious->position) {
						; // Backspace -> OK
					}
					else if(position == actPrevious->position) {
						; // Delete -> OK
					}
					else { // Removals must be at same position to coalesce
						currentAction++;
					}
				}
				else { // Removals must be of one character to coalesce
					currentAction++;
				}
			}
			else {
				// Action coalesced.
			}
		}
		else { // Actions not at top level are always coalesced unless this is after return to top level
			if(!actions[currentAction].mayCoalesce)
				currentAction++;
		}
	}
	else {
		currentAction++;
	}
	startSequence = oldCurrentAction != currentAction;
	int actionWithData = currentAction;
	actions[currentAction].Create(at, position, data, lengthData, mayCoalesce);
	currentAction++;
	actions[currentAction].Create(Action::tStart);
	maxAction = currentAction;
	return actions[actionWithData].data;
}

void UndoHistory::BeginUndoAction()
{
	EnsureUndoRoom();
	if(undoSequenceDepth == 0) {
		if(actions[currentAction].at != Action::tStart) {
			currentAction++;
			actions[currentAction].Create(Action::tStart);
			maxAction = currentAction;
		}
		actions[currentAction].mayCoalesce = false;
	}
	undoSequenceDepth++;
}

void UndoHistory::EndUndoAction()
{
	PLATFORM_ASSERT(undoSequenceDepth > 0);
	EnsureUndoRoom();
	undoSequenceDepth--;
	if(0 == undoSequenceDepth) {
		if(actions[currentAction].at != Action::tStart) {
			currentAction++;
			actions[currentAction].Create(Action::tStart);
			maxAction = currentAction;
		}
		actions[currentAction].mayCoalesce = false;
	}
}

void UndoHistory::DropUndoSequence()
{
	undoSequenceDepth = 0;
}

void UndoHistory::DeleteUndoHistory()
{
	for(int i = 1; i < maxAction; i++)
		actions[i].Destroy();
	maxAction = 0;
	currentAction = 0;
	actions[currentAction].Create(Action::tStart);
	savePoint = 0;
	tentativePoint = -1;
}

void UndoHistory::SetSavePoint()
{
	savePoint = currentAction;
}

bool UndoHistory::IsSavePoint() const
{
	return savePoint == currentAction;
}

void UndoHistory::TentativeStart()
{
	tentativePoint = currentAction;
}

void UndoHistory::TentativeCommit()
{
	tentativePoint = -1;
	// Truncate undo history
	maxAction = currentAction;
}

int UndoHistory::TentativeSteps()
{
	// Drop any trailing startAction
	if(actions[currentAction].at == Action::tStart && currentAction > 0)
		currentAction--;
	return (tentativePoint >= 0) ? (currentAction - tentativePoint) : -1;
}

bool UndoHistory::CanUndo() const
{
	return (currentAction > 0) && (maxAction > 0);
}

int UndoHistory::StartUndo()
{
	// Drop any trailing startAction
	if(actions[currentAction].at == Action::tStart && currentAction > 0)
		currentAction--;
	// Count the steps in this action
	int act = currentAction;
	while(actions[act].at != Action::tStart && act > 0) {
		act--;
	}
	return (currentAction - act);
}

const UndoHistory::Action & UndoHistory::GetUndoStep() const
{
	return actions[currentAction];
}

void UndoHistory::CompletedUndoStep()
{
	currentAction--;
}

bool UndoHistory::CanRedo() const
{
	return maxAction > currentAction;
}

int UndoHistory::StartRedo()
{
	// Drop any leading startAction
	if(actions[currentAction].at == Action::tStart && currentAction < maxAction)
		currentAction++;
	// Count the steps in this action
	int act = currentAction;
	while(actions[act].at != Action::tStart && act < maxAction) {
		act++;
	}
	return (act - currentAction);
}

const UndoHistory::Action & UndoHistory::GetRedoStep() const
{
	return actions[currentAction];
}

void UndoHistory::CompletedRedoStep()
{
	currentAction++;
}

Document::CellBuffer::CellBuffer()
{
	readOnly = false;
	utf8LineEnds = 0;
	collectingUndo = true;
}

Document::CellBuffer::~CellBuffer()
{
}

char FASTCALL Document::CellBuffer::CharAt(int position) const
{
	return substance.ValueAt(position);
}

void Document::CellBuffer::GetCharRange(char * buffer, int position, int lengthRetrieve) const
{
	if(lengthRetrieve > 0 && position >= 0) {
		if((position + lengthRetrieve) > substance.Length())
			Platform::DebugPrintf("Bad GetCharRange %d for %d of %d\n", position, lengthRetrieve, substance.Length());
		else
			substance.GetRange(buffer, position, lengthRetrieve);
	}
}

char FASTCALL Document::CellBuffer::StyleAt(int position) const
{
	return style.ValueAt(position);
}

void Document::CellBuffer::GetStyleRange(uchar * buffer, int position, int lengthRetrieve) const
{
	if(lengthRetrieve >= 0 && position >= 0) {
		if((position + lengthRetrieve) > style.Length())
			Platform::DebugPrintf("Bad GetStyleRange %d for %d of %d\n", position, lengthRetrieve, style.Length());
		else
			style.GetRange(reinterpret_cast<char *>(buffer), position, lengthRetrieve);
	}
}

const char * Document::CellBuffer::BufferPointer()
{
	return substance.BufferPointer();
}

const char * Document::CellBuffer::RangePointer(int position, int rangeLength)
{
	return substance.RangePointer(position, rangeLength);
}

int Document::CellBuffer::GapPosition() const
{
	return substance.GapPosition();
}

// The char* returned is to an allocation owned by the undo history
const char * Document::CellBuffer::InsertString(int position, const char * s, int insertLength, bool &startSequence)
{
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	const char * data = s;
	if(!readOnly) {
		if(collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			// This takes up about half load time
			data = uh.AppendAction(UndoHistory::Action::tInsert, position, s, insertLength, startSequence);
		}
		BasicInsertString(position, s, insertLength);
	}
	return data;
}

bool Document::CellBuffer::SetStyleAt(int position, char styleValue)
{
	char curVal = style.ValueAt(position);
	if(curVal != styleValue) {
		style.SetValueAt(position, styleValue);
		return true;
	}
	else {
		return false;
	}
}

bool Document::CellBuffer::SetStyleFor(int position, int lengthStyle, char styleValue)
{
	bool changed = false;
	PLATFORM_ASSERT(lengthStyle == 0 || (lengthStyle > 0 && lengthStyle + position <= style.Length()));
	while(lengthStyle--) {
		char curVal = style.ValueAt(position);
		if(curVal != styleValue) {
			style.SetValueAt(position, styleValue);
			changed = true;
		}
		position++;
	}
	return changed;
}

// The char* returned is to an allocation owned by the undo history
const char * Document::CellBuffer::DeleteChars(int position, int deleteLength, bool &startSequence)
{
	// InsertString and DeleteChars are the bottleneck though which all changes occur
	PLATFORM_ASSERT(deleteLength > 0);
	const char * data = 0;
	if(!readOnly) {
		if(collectingUndo) {
			// Save into the undo/redo stack, but only the characters - not the formatting
			// The gap would be moved to position anyway for the deletion so this doesn't cost extra
			data = substance.RangePointer(position, deleteLength);
			data = uh.AppendAction(UndoHistory::Action::tRemove, position, data, deleteLength, startSequence);
		}
		BasicDeleteChars(position, deleteLength);
	}
	return data;
}

void FASTCALL Document::CellBuffer::Allocate(int newSize)
{
	substance.ReAllocate(newSize);
	style.ReAllocate(newSize);
}

void Document::CellBuffer::SetLineEndTypes(int utf8LineEnds_)
{
	if(utf8LineEnds != utf8LineEnds_) {
		utf8LineEnds = utf8LineEnds_;
		ResetLineEnds();
	}
}

bool Document::CellBuffer::ContainsLineEnd(const char * s, int length) const
{
	uchar chBeforePrev = 0;
	uchar chPrev = 0;
	for(int i = 0; i < length; i++) {
		const uchar ch = s[i];
		if((ch == '\r') || (ch == '\n')) {
			return true;
		}
		else if(utf8LineEnds) {
			uchar back3[3] = { chBeforePrev, chPrev, ch };
			if(UTF8IsSeparator(back3) || UTF8IsNEL(back3 + 1)) {
				return true;
			}
		}
		chBeforePrev = chPrev;
		chPrev = ch;
	}
	return false;
}

int Document::CellBuffer::LineStart(int line) const
{
	if(line < 0)
		return 0;
	else if(line >= Lines())
		return Length();
	else
		return lv.LineStart(line);
}

int    Document::CellBuffer::Length() const { return substance.Length(); }
void   Document::CellBuffer::SetPerLine(PerLine * pl) { lv.SetPerLine(pl); }
int    Document::CellBuffer::Lines() const { return lv.Lines(); }
bool   Document::CellBuffer::IsReadOnly() const { return readOnly; }
void   Document::CellBuffer::SetReadOnly(bool set) { readOnly = set; }
void   Document::CellBuffer::SetSavePoint() { uh.SetSavePoint(); }
bool   Document::CellBuffer::IsSavePoint() const { return uh.IsSavePoint(); }
void   Document::CellBuffer::TentativeStart() { uh.TentativeStart(); }
void   Document::CellBuffer::TentativeCommit() { uh.TentativeCommit(); }
int    Document::CellBuffer::TentativeSteps() { return uh.TentativeSteps(); }
bool   Document::CellBuffer::TentativeActive() const { return uh.TentativeActive(); }

// Without undo

void Document::CellBuffer::InsertLine(int line, int position, bool lineStart)
{
	lv.InsertLine(line, position, lineStart);
}

void Document::CellBuffer::RemoveLine(int line)
{
	lv.RemoveLine(line);
}

bool Document::CellBuffer::UTF8LineEndOverlaps(int position) const
{
	uchar bytes[] = {
		static_cast<uchar>(substance.ValueAt(position-2)),
		static_cast<uchar>(substance.ValueAt(position-1)),
		static_cast<uchar>(substance.ValueAt(position)),
		static_cast<uchar>(substance.ValueAt(position+1)),
	};
	return UTF8IsSeparator(bytes) || UTF8IsSeparator(bytes+1) || UTF8IsNEL(bytes+1);
}

void Document::CellBuffer::ResetLineEnds()
{
	// Reinitialize line data -- too much work to preserve
	lv.Init();
	const int position = 0;
	const int length = Length();
	int lineInsert = 1;
	bool atLineStart = true;
	lv.InsertText(lineInsert-1, length);
	uchar chBeforePrev = 0;
	uchar chPrev = 0;
	for(int i = 0; i < length; i++) {
		uchar ch = substance.ValueAt(position + i);
		if(ch == '\r') {
			InsertLine(lineInsert, (position + i) + 1, atLineStart);
			lineInsert++;
		}
		else if(ch == '\n') {
			if(chPrev == '\r') {
				// Patch up what was end of line
				lv.SetLineStart(lineInsert - 1, (position + i) + 1);
			}
			else {
				InsertLine(lineInsert, (position + i) + 1, atLineStart);
				lineInsert++;
			}
		}
		else if(utf8LineEnds) {
			uchar back3[3] = {chBeforePrev, chPrev, ch};
			if(UTF8IsSeparator(back3) || UTF8IsNEL(back3+1)) {
				InsertLine(lineInsert, (position + i) + 1, atLineStart);
				lineInsert++;
			}
		}
		chBeforePrev = chPrev;
		chPrev = ch;
	}
}

void Document::CellBuffer::BasicInsertString(int position, const char * s, int insertLength)
{
	if(insertLength) {
		PLATFORM_ASSERT(insertLength > 0);
		uchar chAfter = substance.ValueAt(position);
		bool breakingUTF8LineEnd = false;
		if(utf8LineEnds && UTF8IsTrailByte(chAfter)) {
			breakingUTF8LineEnd = UTF8LineEndOverlaps(position);
		}
		substance.InsertFromArray(position, s, 0, insertLength);
		style.InsertValue(position, insertLength, 0);
		int lineInsert = lv.LineFromPosition(position) + 1;
		bool atLineStart = lv.LineStart(lineInsert-1) == position;
		// SciPoint all the lines after the insertion point further along in the buffer
		lv.InsertText(lineInsert-1, insertLength);
		uchar chBeforePrev = substance.ValueAt(position - 2);
		uchar chPrev = substance.ValueAt(position - 1);
		if(chPrev == '\r' && chAfter == '\n') {
			// Splitting up a crlf pair at position
			InsertLine(lineInsert, position, false);
			lineInsert++;
		}
		if(breakingUTF8LineEnd) {
			RemoveLine(lineInsert);
		}
		uchar ch = ' ';
		for(int i = 0; i < insertLength; i++) {
			ch = s[i];
			if(ch == '\r') {
				InsertLine(lineInsert, (position + i) + 1, atLineStart);
				lineInsert++;
			}
			else if(ch == '\n') {
				if(chPrev == '\r') {
					lv.SetLineStart(lineInsert - 1, (position + i) + 1); // Patch up what was end of line
				}
				else {
					InsertLine(lineInsert, (position + i) + 1, atLineStart);
					lineInsert++;
				}
			}
			else if(utf8LineEnds) {
				uchar back3[3] = {chBeforePrev, chPrev, ch};
				if(UTF8IsSeparator(back3) || UTF8IsNEL(back3+1)) {
					InsertLine(lineInsert, (position + i) + 1, atLineStart);
					lineInsert++;
				}
			}
			chBeforePrev = chPrev;
			chPrev = ch;
		}
		// Joining two lines where last insertion is cr and following substance starts with lf
		if(chAfter == '\n') {
			if(ch == '\r') {
				RemoveLine(lineInsert - 1); // End of line already in buffer so drop the newly created one
			}
		}
		else if(utf8LineEnds && !UTF8IsAscii(chAfter)) {
			// May have end of UTF-8 line end in buffer and start in insertion
			for(int j = 0; j < UTF8SeparatorLength-1; j++) {
				uchar chAt = substance.ValueAt(position + insertLength + j);
				uchar back3[3] = {chBeforePrev, chPrev, chAt};
				if(UTF8IsSeparator(back3)) {
					InsertLine(lineInsert, (position + insertLength + j) + 1, atLineStart);
					lineInsert++;
				}
				if((j == 0) && UTF8IsNEL(back3+1)) {
					InsertLine(lineInsert, (position + insertLength + j) + 1, atLineStart);
					lineInsert++;
				}
				chBeforePrev = chPrev;
				chPrev = chAt;
			}
		}
	}
}

void Document::CellBuffer::BasicDeleteChars(int position, int deleteLength)
{
	if(deleteLength) {
		if((position == 0) && (deleteLength == substance.Length())) {
			// If whole buffer is being deleted, faster to reinitialise lines data
			// than to delete each line.
			lv.Init();
		}
		else {
			// Have to fix up line positions before doing deletion as looking at text in buffer
			// to work out which lines have been removed
			int lineRemove = lv.LineFromPosition(position) + 1;
			lv.InsertText(lineRemove-1, -(deleteLength));
			uchar chPrev = substance.ValueAt(position - 1);
			uchar chBefore = chPrev;
			uchar chNext = substance.ValueAt(position);
			bool ignoreNL = false;
			if(chPrev == '\r' && chNext == '\n') {
				// Move back one
				lv.SetLineStart(lineRemove, position);
				lineRemove++;
				ignoreNL = true;        // First \n is not real deletion
			}
			if(utf8LineEnds && UTF8IsTrailByte(chNext)) {
				if(UTF8LineEndOverlaps(position)) {
					RemoveLine(lineRemove);
				}
			}
			uchar ch = chNext;
			for(int i = 0; i < deleteLength; i++) {
				chNext = substance.ValueAt(position + i + 1);
				if(ch == '\r') {
					if(chNext != '\n') {
						RemoveLine(lineRemove);
					}
				}
				else if(ch == '\n') {
					if(ignoreNL) {
						ignoreNL = false;       // Further \n are real deletions
					}
					else {
						RemoveLine(lineRemove);
					}
				}
				else if(utf8LineEnds) {
					if(!UTF8IsAscii(ch)) {
						uchar next3[3] = { ch, chNext, static_cast<uchar>(substance.ValueAt(position + i + 2)) };
						if(UTF8IsSeparator(next3) || UTF8IsNEL(next3)) {
							RemoveLine(lineRemove);
						}
					}
				}
				ch = chNext;
			}
			// May have to fix up end if last deletion causes cr to be next to lf
			// or removes one of a crlf pair
			char chAfter = substance.ValueAt(position + deleteLength);
			if(chBefore == '\r' && chAfter == '\n') {
				// Using lineRemove-1 as cr ended line before start of deletion
				RemoveLine(lineRemove - 1);
				lv.SetLineStart(lineRemove - 1, position + 1);
			}
		}
		substance.DeleteRange(position, deleteLength);
		style.DeleteRange(position, deleteLength);
	}
}

bool Document::CellBuffer::SetUndoCollection(bool collectUndo)
{
	collectingUndo = collectUndo;
	uh.DropUndoSequence();
	return collectingUndo;
}

bool Document::CellBuffer::IsCollectingUndo() const { return collectingUndo; }
void Document::CellBuffer::BeginUndoAction() { uh.BeginUndoAction(); }
void Document::CellBuffer::EndUndoAction() { uh.EndUndoAction(); }

void Document::CellBuffer::AddUndoAction(int token, bool mayCoalesce)
{
	bool startSequence;
	uh.AppendAction(UndoHistory::Action::tContainer, token, 0, 0, startSequence, mayCoalesce);
}

void Document::CellBuffer::DeleteUndoHistory() { uh.DeleteUndoHistory(); }
bool Document::CellBuffer::CanUndo() const { return uh.CanUndo(); }
int Document::CellBuffer::StartUndo() { return uh.StartUndo(); }
const UndoHistory::Action & Document::CellBuffer::GetUndoStep() const { return uh.GetUndoStep(); }

void Document::CellBuffer::PerformUndoStep()
{
	const UndoHistory::Action & actionStep = uh.GetUndoStep();
	if(actionStep.at == UndoHistory::Action::tInsert) {
		if(substance.Length() < actionStep.lenData) {
			throw std::runtime_error("Document::CellBuffer::PerformUndoStep: deletion must be less than document length.");
		}
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	}
	else if(actionStep.at == UndoHistory::Action::tRemove) {
		BasicInsertString(actionStep.position, actionStep.data, actionStep.lenData);
	}
	uh.CompletedUndoStep();
}

bool Document::CellBuffer::CanRedo() const { return uh.CanRedo(); }
int Document::CellBuffer::StartRedo() { return uh.StartRedo(); }
const UndoHistory::Action & Document::CellBuffer::GetRedoStep() const { return uh.GetRedoStep(); }

void Document::CellBuffer::PerformRedoStep()
{
	const UndoHistory::Action & actionStep = uh.GetRedoStep();
	if(actionStep.at == UndoHistory::Action::tInsert) {
		BasicInsertString(actionStep.position, actionStep.data, actionStep.lenData);
	}
	else if(actionStep.at == UndoHistory::Action::tRemove) {
		BasicDeleteChars(actionStep.position, actionStep.lenData);
	}
	uh.CompletedRedoStep();
}

