// Scintilla source code edit control
/** @file Selection.cxx
** Classes maintaining the selection.
**/
// Copyright 2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
#include <scintilla-internal.h>

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

SelectionPosition::SelectionPosition(int position_ /*= INVALID_POSITION*/, int virtualSpace_ /*= 0*/) : position(position_), virtualSpace(virtualSpace_)
{
	PLATFORM_ASSERT(virtualSpace < 800000);
	SETMAX(virtualSpace, 0);
}

void SelectionPosition::Reset()
{
	position = 0;
	virtualSpace = 0;
}

void FASTCALL SelectionPosition::SetPosition(int position_)
{
	position = position_;
	virtualSpace = 0;
}

void FASTCALL SelectionPosition::SetVirtualSpace(int virtualSpace_)
{
	PLATFORM_ASSERT(virtualSpace_ < 800000);
	if(virtualSpace_ >= 0)
		virtualSpace = virtualSpace_;
}

void FASTCALL SelectionPosition::Add(int increment)
{
	position = position + increment;
}

void SelectionPosition::MoveForInsertDelete(bool insertion, int startChange, int length)
{
	if(insertion) {
		if(position == startChange) {
			int virtualLengthRemove = smin(length, virtualSpace);
			virtualSpace -= virtualLengthRemove;
			position += virtualLengthRemove;
		}
		else if(position > startChange) {
			position += length;
		}
	}
	else {
		if(position == startChange) {
			virtualSpace = 0;
		}
		if(position > startChange) {
			int endDeletion = startChange + length;
			if(position > endDeletion) {
				position -= length;
			}
			else {
				position = startChange;
				virtualSpace = 0;
			}
		}
	}
}

bool FASTCALL SelectionPosition::operator < (const SelectionPosition &other) const
{
	return (position == other.position) ? (virtualSpace < other.virtualSpace) : (position < other.position);
}

bool FASTCALL SelectionPosition::operator > (const SelectionPosition &other) const
{
	return (position == other.position) ? (virtualSpace > other.virtualSpace) : (position > other.position);
}

bool FASTCALL SelectionPosition::operator <= (const SelectionPosition &other) const
{
	return (position == other.position && virtualSpace == other.virtualSpace) ? true : (other > *this);
}

bool FASTCALL SelectionPosition::operator >= (const SelectionPosition &other) const
{
	return (position == other.position && virtualSpace == other.virtualSpace) ? true : (*this > other);
}
//
//
//
SelectionRange::SelectionRange() : caret(), anchor()
{
}

SelectionRange::SelectionRange(SelectionPosition single) : caret(single), anchor(single)
{
}

SelectionRange::SelectionRange(int single) : caret(single), anchor(single)
{
}

SelectionRange::SelectionRange(SelectionPosition caret_, SelectionPosition anchor_) : caret(caret_), anchor(anchor_)
{
}

SelectionRange::SelectionRange(int caret_, int anchor_) : caret(caret_), anchor(anchor_)
{
}

int SelectionRange::Length() const
{
	return (anchor > caret) ? (anchor.Position() - caret.Position()) : (caret.Position() - anchor.Position());
}

void SelectionRange::Reset()
{
	anchor.Reset();
	caret.Reset();
}

SelectionPosition SelectionRange::Start() const
{
	return (anchor < caret) ? anchor : caret;
}

SelectionPosition SelectionRange::End() const
{
	return (anchor < caret) ? caret : anchor;
}

void SelectionRange::ClearVirtualSpace()
{
	anchor.SetVirtualSpace(0);
	caret.SetVirtualSpace(0);
}

void SelectionRange::MoveForInsertDelete(bool insertion, int startChange, int length)
{
	caret.MoveForInsertDelete(insertion, startChange, length);
	anchor.MoveForInsertDelete(insertion, startChange, length);
}

bool FASTCALL SelectionRange::Contains(int pos) const
{
	return (anchor > caret) ? ((pos >= caret.Position()) && (pos <= anchor.Position())) : ((pos >= anchor.Position()) && (pos <= caret.Position()));
}

bool SelectionRange::Contains(SelectionPosition sp) const
{
	return (anchor > caret) ? ((sp >= caret) && (sp <= anchor)) : ((sp >= anchor) && (sp <= caret));
}

bool FASTCALL SelectionRange::ContainsCharacter(int posChar) const
{
	return (anchor > caret) ? ((posChar >= caret.Position()) && (posChar < anchor.Position())) : ((posChar >= anchor.Position()) && (posChar < caret.Position()));
}

SelectionSegment SelectionRange::Intersect(SelectionSegment check) const
{
	SelectionSegment inOrder(caret, anchor);
	if((inOrder.start <= check.end) || (inOrder.end >= check.start)) {
		SelectionSegment portion = check;
		SETMAX(portion.start, inOrder.start);
		SETMIN(portion.end, inOrder.end);
		return (portion.start > portion.end) ? SelectionSegment() : portion;
	}
	else
		return SelectionSegment();
}

void SelectionRange::Swap()
{
	std::swap(caret, anchor);
}

bool SelectionRange::Trim(SelectionRange range)
{
	SelectionPosition startRange = range.Start();
	SelectionPosition endRange = range.End();
	SelectionPosition start = Start();
	SelectionPosition end = End();
	PLATFORM_ASSERT(start <= end);
	PLATFORM_ASSERT(startRange <= endRange);
	if((startRange <= end) && (endRange >= start)) {
		if((start > startRange) && (end < endRange)) {
			// Completely covered by range -> empty at start
			end = start;
		}
		else if((start < startRange) && (end > endRange)) {
			// Completely covers range -> empty at start
			end = start;
		}
		else if(start <= startRange) {
			// Trim end
			end = startRange;
		}
		else {   //
			PLATFORM_ASSERT(end >= endRange);
			// Trim start
			start = endRange;
		}
		if(anchor > caret) {
			caret = start;
			anchor = end;
		}
		else {
			anchor = start;
			caret = end;
		}
		return Empty();
	}
	else {
		return false;
	}
}

// If range is all virtual collapse to start of virtual space
void SelectionRange::MinimizeVirtualSpace()
{
	if(caret.Position() == anchor.Position()) {
		int virtualSpace = caret.VirtualSpace();
		SETMIN(virtualSpace, anchor.VirtualSpace());
		caret.SetVirtualSpace(virtualSpace);
		anchor.SetVirtualSpace(virtualSpace);
	}
}

Selection::Selection() : mainRange(0), moveExtends(false), tentativeMain(false), selType(selStream)
{
	AddSelection(SelectionRange(SelectionPosition(0)));
}

Selection::~Selection()
{
}

bool Selection::IsRectangular() const
{
	return (selType == selRectangle) || (selType == selThin);
}

int Selection::MainCaret() const
{
	return Ranges[mainRange].caret.Position();
}

int Selection::MainAnchor() const
{
	return Ranges[mainRange].anchor.Position();
}

SelectionRange &Selection::Rectangular()
{
	return rangeRectangular;
}

SelectionSegment Selection::Limits() const
{
	if(Ranges.empty()) {
		return SelectionSegment();
	}
	else {
		SelectionSegment sr(Ranges[0].anchor, Ranges[0].caret);
		for(size_t i = 1; i < Ranges.size(); i++) {
			sr.Extend(Ranges[i].anchor);
			sr.Extend(Ranges[i].caret);
		}
		return sr;
	}
}

SelectionSegment Selection::LimitsForRectangularElseMain() const
{
	return IsRectangular() ? Limits() : SelectionSegment(Ranges[mainRange].caret, Ranges[mainRange].anchor);
}

size_t Selection::Count() const
{
	return Ranges.size();
}

size_t Selection::Main() const
{
	return mainRange;
}

void Selection::SetMain(size_t r)
{
	PLATFORM_ASSERT(r < Ranges.size());
	mainRange = r;
}

SelectionRange & Selection::Range(size_t r)
{
	return Ranges[r];
}

const SelectionRange & Selection::Range(size_t r) const
{
	return Ranges[r];
}

SelectionRange & Selection::RangeMain()
{
	return Ranges[mainRange];
}

const SelectionRange & Selection::RangeMain() const
{
	return Ranges[mainRange];
}

SelectionPosition Selection::Start() const
{
	return IsRectangular() ? rangeRectangular.Start() : Ranges[mainRange].Start();
}

bool Selection::MoveExtends() const
{
	return moveExtends;
}

void Selection::SetMoveExtends(bool moveExtends_)
{
	moveExtends = moveExtends_;
}

bool Selection::Empty() const
{
	for(size_t i = 0; i < Ranges.size(); i++) {
		if(!Ranges[i].Empty())
			return false;
	}
	return true;
}

SelectionPosition Selection::Last() const
{
	SelectionPosition lastPosition;
	for(size_t i = 0; i < Ranges.size(); i++) {
		SETMAX(lastPosition, Ranges[i].caret);
		SETMAX(lastPosition, Ranges[i].anchor);
	}
	return lastPosition;
}

int Selection::Length() const
{
	int len = 0;
	for(size_t i = 0; i < Ranges.size(); i++) {
		len += Ranges[i].Length();
	}
	return len;
}

void Selection::MovePositions(bool insertion, int startChange, int length)
{
	for(size_t i = 0; i < Ranges.size(); i++) {
		Ranges[i].MoveForInsertDelete(insertion, startChange, length);
	}
	if(selType == selRectangle) {
		rangeRectangular.MoveForInsertDelete(insertion, startChange, length);
	}
}

void Selection::TrimSelection(SelectionRange range)
{
	for(size_t i = 0; i < Ranges.size(); ) {
		if((i != mainRange) && Ranges[i].Trim(range)) {
			// Trimmed to empty so remove
			for(size_t j = i; j < Ranges.size()-1; j++) {
				Ranges[j] = Ranges[j+1];
				if(j == mainRange-1)
					mainRange--;
			}
			Ranges.pop_back();
		}
		else
			i++;
	}
}

void Selection::TrimOtherSelections(size_t r, SelectionRange range)
{
	for(size_t i = 0; i < Ranges.size(); ++i) {
		if(i != r)
			Ranges[i].Trim(range);
	}
}

void Selection::SetSelection(SelectionRange range)
{
	Ranges.clear();
	Ranges.push_back(range);
	mainRange = Ranges.size() - 1;
}

void Selection::AddSelection(SelectionRange range)
{
	TrimSelection(range);
	Ranges.push_back(range);
	mainRange = Ranges.size() - 1;
}

void Selection::AddSelectionWithoutTrim(SelectionRange range)
{
	Ranges.push_back(range);
	mainRange = Ranges.size() - 1;
}

void Selection::DropSelection(size_t r)
{
	if(Ranges.size() > 1 && r < Ranges.size()) {
		size_t mainNew = mainRange;
		if(mainNew >= r) {
			if(mainNew == 0) {
				mainNew = Ranges.size() - 2;
			}
			else {
				mainNew--;
			}
		}
		Ranges.erase(Ranges.begin() + r);
		mainRange = mainNew;
	}
}

void Selection::DropAdditionalRanges()
{
	SetSelection(RangeMain());
}

void Selection::TentativeSelection(SelectionRange range)
{
	if(!tentativeMain) {
		RangesSaved = Ranges;
	}
	Ranges = RangesSaved;
	AddSelection(range);
	TrimSelection(Ranges[mainRange]);
	tentativeMain = true;
}

void Selection::CommitTentative()
{
	RangesSaved.clear();
	tentativeMain = false;
}

int Selection::CharacterInSelection(int posCharacter) const
{
	for(size_t i = 0; i < Ranges.size(); i++) {
		if(Ranges[i].ContainsCharacter(posCharacter))
			return i == mainRange ? 1 : 2;
	}
	return 0;
}

int Selection::InSelectionForEOL(int pos) const
{
	for(size_t i = 0; i < Ranges.size(); i++) {
		if(!Ranges[i].Empty() && (pos > Ranges[i].Start().Position()) && (pos <= Ranges[i].End().Position()))
			return i == mainRange ? 1 : 2;
	}
	return 0;
}

int Selection::VirtualSpaceFor(int pos) const
{
	int virtualSpace = 0;
	for(size_t i = 0; i < Ranges.size(); i++) {
		if(Ranges[i].caret.Position() == pos && virtualSpace < Ranges[i].caret.VirtualSpace())
			virtualSpace = Ranges[i].caret.VirtualSpace();
		if(Ranges[i].anchor.Position() == pos && virtualSpace < Ranges[i].anchor.VirtualSpace())
			virtualSpace = Ranges[i].anchor.VirtualSpace();
	}
	return virtualSpace;
}

void Selection::Clear()
{
	Ranges.clear();
	Ranges.push_back(SelectionRange());
	mainRange = Ranges.size() - 1;
	selType = selStream;
	moveExtends = false;
	Ranges[mainRange].Reset();
	rangeRectangular.Reset();
}

void Selection::RemoveDuplicates()
{
	for(size_t i = 0; i < Ranges.size()-1; i++) {
		if(Ranges[i].Empty()) {
			size_t j = i+1;
			while(j < Ranges.size()) {
				if(Ranges[i] == Ranges[j]) {
					Ranges.erase(Ranges.begin() + j);
					if(mainRange >= j)
						mainRange--;
				}
				else
					j++;
			}
		}
	}
}

void Selection::RotateMain()
{
	mainRange = (mainRange + 1) % Ranges.size();
}

