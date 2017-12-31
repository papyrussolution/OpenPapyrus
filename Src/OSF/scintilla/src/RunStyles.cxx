/** @file RunStyles.cxx
** Data structure used to store sparse styles.
**/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif
//
//
//
Partitioning::SplitVectorWithRangeAdd::SplitVectorWithRangeAdd(int growSize_)
{
	SetGrowSize(growSize_);
	ReAllocate(growSize_);
}

Partitioning::SplitVectorWithRangeAdd::~SplitVectorWithRangeAdd()
{
}

void Partitioning::SplitVectorWithRangeAdd::RangeAddDelta(int start, int end, int delta)
{
	// end is 1 past end, so end-start is number of elements to change
	int i = 0;
	int rangeLength = end - start;
	int range1Length = rangeLength;
	int part1Left = part1Length - start;
	if(range1Length > part1Left)
		range1Length = part1Left;
	while(i < range1Length) {
		body[start++] += delta;
		i++;
	}
	start += gapLength;
	while(i < rangeLength) {
		body[start++] += delta;
		i++;
	}
}
//
//
//
Partitioning::Partitioning(int growSize)
{
	Allocate(growSize);
}

Partitioning::~Partitioning()
{
	ZDELETE(body);
}

int Partitioning::Partitions() const
{
	return body->Length()-1;
}

void Partitioning::InsertPartition(int partition, int pos)
{
	if(stepPartition < partition)
		ApplyStep(partition);
	body->Insert(partition, pos);
	stepPartition++;
}

void Partitioning::SetPartitionStartPosition(int partition, int pos)
{
	ApplyStep(partition+1);
	if((partition >= 0) && (partition <= body->Length()))
		body->SetValueAt(partition, pos);
}

void Partitioning::InsertText(int partitionInsert, int delta)
{
	// Point all the partitions after the insertion point further along in the buffer
	if(stepLength) {
		if(partitionInsert >= stepPartition) {
			// Fill in up to the new insertion point
			ApplyStep(partitionInsert);
			stepLength += delta;
		}
		else if(partitionInsert >= (stepPartition - body->Length() / 10)) {
			// Close to step but before so move step back
			BackStep(partitionInsert);
			stepLength += delta;
		}
		else {
			ApplyStep(body->Length()-1);
			stepPartition = partitionInsert;
			stepLength = delta;
		}
	}
	else {
		stepPartition = partitionInsert;
		stepLength = delta;
	}
}

void FASTCALL Partitioning::RemovePartition(int partition)
{
	if(partition > stepPartition)
		ApplyStep(partition);
	stepPartition--;
	body->Delete(partition);
}

int FASTCALL Partitioning::PositionFromPartition(int partition) const
{
	PLATFORM_ASSERT(partition >= 0);
	PLATFORM_ASSERT(partition < body->Length());
	int pos = 0;
	if(partition >= 0 && partition < body->Length()) {
		pos = body->ValueAt(partition);
		if(partition > stepPartition)
			pos += stepLength;
	}
	return pos;
}
//
//
//
SparseVectorBase::SparseVectorBase()
{
	starts = new Partitioning(8);
}

SparseVectorBase::~SparseVectorBase()
{
	ZDELETE(starts);
}

int SparseVectorBase::Length() const
{
	return starts->PositionFromPartition(starts->Partitions());
}

int SparseVectorBase::Elements() const
{
	return starts->Partitions();
}

int SparseVectorBase::PositionOfElement(int element) const
{
	return starts->PositionFromPartition(element);
}
//
// Return value in range [0 .. Partitions() - 1] even for arguments outside interval
//
int FASTCALL Partitioning::PartitionFromPosition(int pos) const
{
	if(body->Length() <= 1)
		return 0;
	else if(pos >= (PositionFromPartition(body->Length()-1)))
		return (body->Length() - 1 - 1);
	else {
		int lower = 0;
		int upper = body->Length()-1;
		do {
			const int middle = (upper + lower + 1) / 2;   // Round high
			int posMiddle = body->ValueAt(middle);
			if(middle > stepPartition)
				posMiddle += stepLength;
			if(pos < posMiddle)
				upper = (middle - 1);
			else
				lower = middle;
		} while(lower < upper);
		return lower;
	}
}

void Partitioning::DeleteAll()
{
	int growSize = body->GetGrowSize();
	delete body;
	Allocate(growSize);
}
//
// Move step forward
//
void FASTCALL Partitioning::ApplyStep(int partitionUpTo)
{
	if(stepLength)
		body->RangeAddDelta(stepPartition+1, partitionUpTo + 1, stepLength);
	stepPartition = partitionUpTo;
	if(stepPartition >= body->Length()-1) {
		stepPartition = body->Length()-1;
		stepLength = 0;
	}
}
//
// Move step backward
//
void FASTCALL Partitioning::BackStep(int partitionDownTo)
{
	if(stepLength)
		body->RangeAddDelta(partitionDownTo+1, stepPartition+1, -stepLength);
	stepPartition = partitionDownTo;
}

void FASTCALL Partitioning::Allocate(int growSize)
{
	body = new SplitVectorWithRangeAdd(growSize);
	stepPartition = 0;
	stepLength = 0;
	body->Insert(0, 0);     // This value stays 0 for ever
	body->Insert(1, 0);     // This is the end of the first partition and will be the start of the second
}
//
//
// Find the first run at a position
int RunStyles::RunFromPosition(int position) const
{
	int run = starts->PartitionFromPosition(position);
	// Go to first element with this position
	while((run > 0) && (position == starts->PositionFromPartition(run-1))) {
		run--;
	}
	return run;
}

// If there is no run boundary at position, insert one continuing style.
int RunStyles::SplitRun(int position)
{
	int run = RunFromPosition(position);
	int posRun = starts->PositionFromPartition(run);
	if(posRun < position) {
		int runStyle = ValueAt(position);
		run++;
		starts->InsertPartition(run, position);
		styles->InsertValue(run, 1, runStyle);
	}
	return run;
}

void RunStyles::RemoveRun(int run)
{
	starts->RemovePartition(run);
	styles->DeleteRange(run, 1);
}

void RunStyles::RemoveRunIfEmpty(int run)
{
	if((run < starts->Partitions()) && (starts->Partitions() > 1)) {
		if(starts->PositionFromPartition(run) == starts->PositionFromPartition(run+1)) {
			RemoveRun(run);
		}
	}
}

void RunStyles::RemoveRunIfSameAsPrevious(int run)
{
	if((run > 0) && (run < starts->Partitions())) {
		if(styles->ValueAt(run-1) == styles->ValueAt(run)) {
			RemoveRun(run);
		}
	}
}

RunStyles::RunStyles()
{
	starts = new Partitioning(8);
	styles = new SplitVector<int>();
	styles->InsertValue(0, 2, 0);
}

RunStyles::~RunStyles()
{
	ZDELETE(starts);
	ZDELETE(styles);
}

int RunStyles::Length() const
{
	return starts->PositionFromPartition(starts->Partitions());
}

int FASTCALL RunStyles::ValueAt(int position) const
{
	return styles->ValueAt(starts->PartitionFromPosition(position));
}

int RunStyles::FindNextChange(int position, int end) const
{
	int run = starts->PartitionFromPosition(position);
	if(run < starts->Partitions()) {
		const int runChange = starts->PositionFromPartition(run);
		if(runChange > position)
			return runChange;
		else {
			int nextChange = starts->PositionFromPartition(run + 1);
			if(nextChange > position)
				return nextChange;
			else if(position < end)
				return end;
			else
				return (end + 1);
		}
	}
	else {
		return end + 1;
	}
}

int RunStyles::StartRun(int position) const
{
	return starts->PositionFromPartition(starts->PartitionFromPosition(position));
}

int RunStyles::EndRun(int position) const
{
	return starts->PositionFromPartition(starts->PartitionFromPosition(position) + 1);
}

bool RunStyles::FillRange(int &position, int value, int &fillLength)
{
	if(fillLength <= 0) {
		return false;
	}
	int end = position + fillLength;
	if(end > Length()) {
		return false;
	}
	int runEnd = RunFromPosition(end);
	if(styles->ValueAt(runEnd) == value) {
		// End already has value so trim range.
		end = starts->PositionFromPartition(runEnd);
		if(position >= end) {
			// Whole range is already same as value so no action
			return false;
		}
		fillLength = end - position;
	}
	else {
		runEnd = SplitRun(end);
	}
	int runStart = RunFromPosition(position);
	if(styles->ValueAt(runStart) == value) {
		// Start is in expected value so trim range.
		runStart++;
		position = starts->PositionFromPartition(runStart);
		fillLength = end - position;
	}
	else {
		if(starts->PositionFromPartition(runStart) < position) {
			runStart = SplitRun(position);
			runEnd++;
		}
	}
	if(runStart < runEnd) {
		styles->SetValueAt(runStart, value);
		// Remove each old run over the range
		for(int run = runStart+1; run<runEnd; run++) {
			RemoveRun(runStart+1);
		}
		runEnd = RunFromPosition(end);
		RemoveRunIfSameAsPrevious(runEnd);
		RemoveRunIfSameAsPrevious(runStart);
		runEnd = RunFromPosition(end);
		RemoveRunIfEmpty(runEnd);
		return true;
	}
	else {
		return false;
	}
}

void RunStyles::SetValueAt(int position, int value)
{
	int len = 1;
	FillRange(position, value, len);
}

void RunStyles::InsertSpace(int position, int insertLength)
{
	int runStart = RunFromPosition(position);
	if(starts->PositionFromPartition(runStart) == position) {
		int runStyle = ValueAt(position);
		// Inserting at start of run so make previous longer
		if(runStart == 0) {
			// Inserting at start of document so ensure 0
			if(runStyle) {
				styles->SetValueAt(0, 0);
				starts->InsertPartition(1, 0);
				styles->InsertValue(1, 1, runStyle);
				starts->InsertText(0, insertLength);
			}
			else {
				starts->InsertText(runStart, insertLength);
			}
		}
		else {
			if(runStyle) {
				starts->InsertText(runStart-1, insertLength);
			}
			else {
				// Insert at end of run so do not extend style
				starts->InsertText(runStart, insertLength);
			}
		}
	}
	else {
		starts->InsertText(runStart, insertLength);
	}
}

void RunStyles::DeleteAll()
{
	ZDELETE(starts);
	ZDELETE(styles);
	starts = new Partitioning(8);
	styles = new SplitVector<int>();
	styles->InsertValue(0, 2, 0);
}

void RunStyles::DeleteRange(int position, int deleteLength)
{
	int end = position + deleteLength;
	int runStart = RunFromPosition(position);
	int runEnd = RunFromPosition(end);
	if(runStart == runEnd) {
		// Deleting from inside one run
		starts->InsertText(runStart, -deleteLength);
		RemoveRunIfEmpty(runStart);
	}
	else {
		runStart = SplitRun(position);
		runEnd = SplitRun(end);
		starts->InsertText(runStart, -deleteLength);
		// Remove each old run over the range
		for(int run = runStart; run<runEnd; run++) {
			RemoveRun(runStart);
		}
		RemoveRunIfEmpty(runStart);
		RemoveRunIfSameAsPrevious(runStart);
	}
}

int RunStyles::Runs() const
{
	return starts->Partitions();
}

bool RunStyles::AllSame() const
{
	for(int run = 1; run < starts->Partitions(); run++) {
		if(styles->ValueAt(run) != styles->ValueAt(run - 1))
			return false;
	}
	return true;
}

bool RunStyles::AllSameAs(int value) const
{
	return AllSame() && (styles->ValueAt(0) == value);
}

int RunStyles::Find(int value, int start) const
{
	if(start < Length()) {
		int run = start ? RunFromPosition(start) : 0;
		if(styles->ValueAt(run) == value)
			return start;
		run++;
		while(run < starts->Partitions()) {
			if(styles->ValueAt(run) == value)
				return starts->PositionFromPartition(run);
			run++;
		}
	}
	return -1;
}

void RunStyles::Check() const
{
	if(Length() < 0) {
		throw std::runtime_error("RunStyles: Length can not be negative.");
	}
	if(starts->Partitions() < 1) {
		throw std::runtime_error("RunStyles: Must always have 1 or more partitions.");
	}
	if(starts->Partitions() != styles->Length()-1) {
		throw std::runtime_error("RunStyles: Partitions and styles different lengths.");
	}
	int start = 0;
	while(start < Length()) {
		int end = EndRun(start);
		if(start >= end) {
			throw std::runtime_error("RunStyles: Partition is 0 length.");
		}
		start = end;
	}
	if(styles->ValueAt(styles->Length()-1) != 0) {
		throw std::runtime_error("RunStyles: Unused style at end changed.");
	}
	for(int j = 1; j<styles->Length()-1; j++) {
		if(styles->ValueAt(j) == styles->ValueAt(j-1)) {
			throw std::runtime_error("RunStyles: Style of a partition same as previous.");
		}
	}
}
