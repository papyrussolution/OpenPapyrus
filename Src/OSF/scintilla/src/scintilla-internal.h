// scintilla-internal.h 
//
#ifndef __SCINTILLA_INTERNAL_H
#define __SCINTILLA_INTERNAL_H

#if PLAT_WIN
	#define EXT_LEXER_DECL __stdcall
#else
	#define EXT_LEXER_DECL
#endif
//
// FontQuality
//
// These definitions match Scintilla.h
//
//#define SC_EFF_QUALITY_MASK            0xF
//#define SC_EFF_QUALITY_DEFAULT           0
//#define SC_EFF_QUALITY_NON_ANTIALIASED   1
//#define SC_EFF_QUALITY_ANTIALIASED       2
//#define SC_EFF_QUALITY_LCD_OPTIMIZED     3
//
// These definitions must match SC_TECHNOLOGY_* in Scintilla.h
//
#define SCWIN_TECH_GDI         0
#define SCWIN_TECH_DIRECTWRITE 1
//
#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class DocWatcher;
class DocModification;
class Document;
#ifdef SCI_LEXER
	class LexState;
#endif
//
// SplitVector.h
//
template <typename T> class SplitVector {
public:
	/// Construct a split buffer.
	SplitVector()
	{
		Init();
	}
	~SplitVector()
	{
		ZDELETEARRAY(body);
	}
	int GetGrowSize() const { return growSize; }
	void FASTCALL SetGrowSize(int growSize_) { growSize = growSize_; }
	/// Reallocate the storage for the buffer to be newSize and
	/// copy exisiting contents to the new buffer.
	/// Must not be used to decrease the size of the buffer.
	void FASTCALL ReAllocate(int newSize)
	{
		if(newSize < 0)
			throw std::runtime_error("SplitVector::ReAllocate: negative size.");
		if(newSize > size) {
			// Move the gap to the end
			GapTo(lengthBody);
			T * newBody = new T[newSize];
			if(size && body) {
				std::copy(body, body + lengthBody, newBody);
				delete []body;
			}
			body = newBody;
			gapLength += newSize - size;
			size = newSize;
		}
	}
	// 
	// Descr: Retrieve the character at a particular position.
	//   Retrieving positions outside the range of the buffer returns 0.
	//   The assertions here are disabled since calling code can be
	//   simpler if out of range access works and returns 0.
	// 
	T FASTCALL ValueAt(int position) const
	{
		if(position < part1Length) {
			//PLATFORM_ASSERT(position >= 0);
			return (position < 0) ? 0 : body[position];
		}
		else {
			//PLATFORM_ASSERT(position < lengthBody);
			return (position >= lengthBody) ? 0 : body[gapLength + position];
		}
	}
	void SetValueAt(int position, T v)
	{
		if(position < part1Length) {
			PLATFORM_ASSERT(position >= 0);
			if(position < 0) {
				;
			}
			else
				body[position] = v;
		}
		else {
			PLATFORM_ASSERT(position < lengthBody);
			if(position >= lengthBody) {
				;
			}
			else
				body[gapLength + position] = v;
		}
	}
	T & FASTCALL operator[] (int position) const
	{
		PLATFORM_ASSERT(position >= 0 && position < lengthBody);
		return (position < part1Length) ? body[position] : body[gapLength + position];
	}
	//
	// Descr: Retrieve the length of the buffer.
	//
	int Length() const { return lengthBody; }
	// 
	// Descr: Insert a single value into the buffer.
	//   Inserting at positions outside the current range fails.
	// 
	void Insert(int position, T v)
	{
		PLATFORM_ASSERT((position >= 0) && (position <= lengthBody));
		if((position >= 0) && (position <= lengthBody)) {
			RoomFor(1);
			GapTo(position);
			body[part1Length] = v;
			lengthBody++;
			part1Length++;
			gapLength--;
		}
	}
	/// Insert a number of elements into the buffer setting their value.
	/// Inserting at positions outside the current range fails.
	void InsertValue(int position, int insertLength, T v)
	{
		PLATFORM_ASSERT((position >= 0) && (position <= lengthBody));
		if(insertLength > 0) {
			if((position >= 0) && (position <= lengthBody)) {
				RoomFor(insertLength);
				GapTo(position);
				std::fill(&body[part1Length], &body[part1Length + insertLength], v);
				lengthBody += insertLength;
				part1Length += insertLength;
				gapLength -= insertLength;
			}
		}
	}
	/// Ensure at least length elements allocated,
	/// appending zero valued elements if needed.
	void FASTCALL EnsureLength(int wantedLength)
	{
		if(Length() < wantedLength) {
			InsertValue(Length(), wantedLength - Length(), 0);
		}
	}
	/// Insert text into the buffer from an array.
	void InsertFromArray(int positionToInsert, const T s[], int positionFrom, int insertLength)
	{
		PLATFORM_ASSERT((positionToInsert >= 0) && (positionToInsert <= lengthBody));
		if(insertLength > 0) {
			if((positionToInsert >= 0) && (positionToInsert <= lengthBody)) {
				RoomFor(insertLength);
				GapTo(positionToInsert);
				std::copy(s + positionFrom, s + positionFrom + insertLength, body + part1Length);
				lengthBody += insertLength;
				part1Length += insertLength;
				gapLength -= insertLength;
			}
		}
	}
	/// Delete one element from the buffer.
	void FASTCALL Delete(int position)
	{
		PLATFORM_ASSERT((position >= 0) && (position < lengthBody));
		if(position >= 0 && position < lengthBody)
			DeleteRange(position, 1);
	}
	/// Delete a range from the buffer.
	/// Deleting positions outside the current range fails.
	void DeleteRange(int position, int deleteLength)
	{
		PLATFORM_ASSERT((position >= 0) && (position + deleteLength <= lengthBody));
		if((position >= 0) && ((position + deleteLength) <= lengthBody)) {
			if((position == 0) && (deleteLength == lengthBody)) {
				// Full deallocation returns storage and is faster
				delete []body;
				Init();
			}
			else if(deleteLength > 0) {
				GapTo(position);
				lengthBody -= deleteLength;
				gapLength += deleteLength;
			}
		}
	}
	/// Delete all the buffer contents.
	void DeleteAll() { DeleteRange(0, lengthBody); }
	// Retrieve a range of elements into an array
	void GetRange(T * buffer, int position, int retrieveLength) const
	{
		// Split into up to 2 ranges, before and after the split then use memcpy on each.
		int range1Length = 0;
		if(position < part1Length) {
			int part1AfterPosition = part1Length - position;
			range1Length = retrieveLength;
			if(range1Length > part1AfterPosition)
				range1Length = part1AfterPosition;
		}
		std::copy(body + position, body + position + range1Length, buffer);
		buffer += range1Length;
		position = position + range1Length + gapLength;
		int range2Length = retrieveLength - range1Length;
		std::copy(body + position, body + position + range2Length, buffer);
	}
	T * BufferPointer()
	{
		RoomFor(1);
		GapTo(lengthBody);
		body[lengthBody] = 0;
		return body;
	}
	T * RangePointer(int position, int rangeLength)
	{
		if(position < part1Length) {
			if((position + rangeLength) > part1Length) {
				// Range overlaps gap, so move gap to start of range.
				GapTo(position);
				return (body + position + gapLength);
			}
			else
				return (body + position);
		}
		else
			return (body + position + gapLength);
	}
	int GapPosition() const { return part1Length; }
protected:
	// 
	// Descr: Move the gap to a particular position so that insertion and
	//   deletion at that point will not require much copying and hence be fast.
	// 
	void FASTCALL GapTo(int position)
	{
		if(position != part1Length) {
			if(position < part1Length) // Moving the gap towards start so moving elements towards end
				std::copy_backward(body + position, body + part1Length, body + gapLength + part1Length);
			else // Moving the gap towards end so moving elements towards start
				std::copy(body + part1Length + gapLength, body + gapLength + position, body + part1Length);
			part1Length = position;
		}
	}
	// 
	// Descr: Check that there is room in the buffer for an insertion,
	//   reallocating if more space needed.
	// 
	void FASTCALL RoomFor(int insertionLength)
	{
		if(gapLength <= insertionLength) {
			while(growSize < size / 6)
				growSize *= 2;
			ReAllocate(size + insertionLength + growSize);
		}
	}
	void Init()
	{
		body = NULL;
		growSize = 8;
		size = 0;
		lengthBody = 0;
		part1Length = 0;
		gapLength = 0;
	}
	int    lengthBody; // @firstmember
	T    * body;
	int    size;
	int    part1Length;
	int    gapLength;  /// invariant: gapLength == size - lengthBody
	int    growSize;
};
//
// Partitioning.h
//
// Descr: Divide an interval into multiple partitions.
//   Useful for breaking a document down into sections such as lines.
//   A 0 length interval has a single 0 length partition, numbered 0
//   If interval not 0 length then each partition non-zero length
//   When needed, positions after the interval are considered part of the last partition
//   but the end of the last partition can be found with PositionFromPartition(last+1).
//
class Partitioning {
public:
	explicit Partitioning(int growSize);
	~Partitioning();
	int Partitions() const;
	void InsertPartition(int partition, int pos);
	void SetPartitionStartPosition(int partition, int pos);
	void InsertText(int partitionInsert, int delta);
	void FASTCALL RemovePartition(int partition);
	int FASTCALL PositionFromPartition(int partition) const;
	/// Return value in range [0 .. Partitions() - 1] even for arguments outside interval
	int FASTCALL PartitionFromPosition(int pos) const;
	void DeleteAll();
private:
	//
	// Move step forward
	//
	void FASTCALL ApplyStep(int partitionUpTo);
	//
	// Move step backward
	//
	void FASTCALL BackStep(int partitionDownTo);
	void FASTCALL Allocate(int growSize);
	//
	// To avoid calculating all the partition positions whenever any text is inserted
	// there may be a step somewhere in the list.
	//
	int stepPartition;
	int stepLength;
	//
	// A split vector of integers with a method for adding a value to all elements in a range.
	// Used by the Partitioning class.
	//
	class SplitVectorWithRangeAdd : public SplitVector <int> {
	public:
		explicit SplitVectorWithRangeAdd(int growSize_);
		~SplitVectorWithRangeAdd();
		void RangeAddDelta(int start, int end, int delta);
	};
	SplitVectorWithRangeAdd * body;
};
//
// CellBuffer.h
//
// Descr: Interface to per-line data that wants to see each line insertion and deletion
//
class PerLine {
public:
	virtual ~PerLine() {}
	virtual void Init()=0;
	virtual void InsertLine(int line)=0;
	virtual void RemoveLine(int line)=0;
};
//
//
//
class UndoHistory {
	public:
	// 
	// Descr: Actions are used to store all the information required to perform one undo/redo step.
	// 
	class Action {
	public:
		enum actionType { 
			tInsert, 
			tRemove, 
			tStart, 
			tContainer 
		};
		Action();
		~Action();
		void Create(actionType at_, int position_=0, const char *data_=0, int lenData_=0, bool mayCoalesce_=true);
		void Destroy();
		void Grab(Action *source);

		actionType at;
		int    position;
		char * data;
		int    lenData;
		bool   mayCoalesce;
	};
	UndoHistory();
	~UndoHistory();

	const char * AppendAction(Action::actionType at, int position, const char *data, int length, bool &startSequence, bool mayCoalesce=true);

	void BeginUndoAction();
	void EndUndoAction();
	void DropUndoSequence();
	void DeleteUndoHistory();

	/// The save point is a marker in the undo stack where the container has stated that
	/// the buffer was saved. Undo and redo can move over the save point.
	void SetSavePoint();
	bool IsSavePoint() const;

	// Tentative actions are used for input composition so that it can be undone cleanly
	void TentativeStart();
	void TentativeCommit();
	bool TentativeActive() const { return tentativePoint >= 0; }
	int TentativeSteps();

	/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
	/// called that many times. Similarly for redo.
	bool CanUndo() const;
	int StartUndo();
	const Action &GetUndoStep() const;
	void CompletedUndoStep();
	bool CanRedo() const;
	int StartRedo();
	const Action &GetRedoStep() const;
	void CompletedRedoStep();
private:
	void EnsureUndoRoom();
	// Private so UndoHistory objects can not be copied
	UndoHistory(const UndoHistory &);

	Action * actions;
	int lenActions;
	int maxAction;
	int currentAction;
	int undoSequenceDepth;
	int savePoint;
	int tentativePoint;
};
//
// PerLine.h
//
class LineMarkers : public PerLine {
public:
	LineMarkers();
	virtual ~LineMarkers();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);

	int    MarkValue(int line);
	int    MarkerNext(int lineStart, int mask) const;
	int    AddMark(int line, int marker, int lines);
	void   MergeMarkers(int pos);
	bool   DeleteMark(int line, int markerNum, bool all);
	void   DeleteMarkFromHandle(int markerHandle);
	int    FASTCALL LineFromHandle(int markerHandle);
private:
	// 
	// Descr: A marker handle set contains any number of MarkerHandleNumbers.
	//
	class MarkerHandleSet {
	public:
		MarkerHandleSet();
		~MarkerHandleSet();
		int    Length() const;
		int    MarkValue() const;	///< Bit set of marker numbers.
		bool   FASTCALL Contains(int handle) const;
		bool   InsertHandle(int handle, int markerNum);
		void   RemoveHandle(int handle);
		bool   RemoveNumber(int markerNum, bool all);
		void   CombineWith(MarkerHandleSet *other);
	private:
		// 
		// Descr: This holds the marker identifier and the marker type to display.
		// MarkerHandleNumbers are members of lists.
		// 
		struct MarkerHandleNumber {
			int    handle;
			int    number;
			MarkerHandleNumber * next;
		};
		MarkerHandleNumber * root;
	};
	SplitVector <MarkerHandleSet *> markers;
	/// Handles are allocated sequentially and should never have to be reused as 32 bit ints are very big.
	int handleCurrent;
};

class LineLevels : public PerLine {
public:
	virtual ~LineLevels();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);

	void ExpandLevels(int sizeNew=-1);
	void ClearLevels();
	int SetLevel(int line, int level, int lines);
	int GetLevel(int line) const;
private:
	SplitVector <int> levels;
};

class LineState : public PerLine {
public:
	LineState();
	virtual ~LineState();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);
	int    SetLineState(int line, int state);
	int    FASTCALL GetLineState(int line);
	int    GetMaxLineState() const;
private:
	SplitVector <int> lineStates;
};

class LineAnnotation : public PerLine {
public:
	LineAnnotation(); 
	virtual ~LineAnnotation();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);
	bool   FASTCALL MultipleStyles(int line) const;
	int    FASTCALL Style(int line) const;
	const  char  * FASTCALL Text(int line) const;
	const  uchar * FASTCALL Styles(int line) const;
	void SetText(int line, const char *text);
	void ClearAll();
	void SetStyle(int line, int style);
	void SetStyles(int line, const uchar *styles);
	int Length(int line) const;
	int Lines(int line) const;
private:
	SplitVector <char *> annotations;
};

typedef std::vector <int> TabstopList;

class LineTabstops : public PerLine {
public:
	LineTabstops(); 
	virtual ~LineTabstops();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);

	bool ClearTabstops(int line);
	bool AddTabstop(int line, int x);
	int GetNextTabstop(int line, int x) const;
private:
	SplitVector <TabstopList *> tabstops;
};
//
// CallTip.h
//
class CallTip {
public:
	CallTip();
	~CallTip();
	void PaintCT(SciSurface *surfaceWindow);
	void MouseClick(SciPoint pt);
	/// Setup the calltip and return a rectangle of the area required.
	PRectangle CallTipStart(int pos, SciPoint pt, int textHeight, const char *defn,
		const char *faceName, int size, int codePage_, int characterSet, int technology, SciWindow &wParent);
	void CallTipCancel();
	/// Set a range of characters to be displayed in a highlight style.
	/// Commonly used to highlight the current parameter.
	void SetHighlight(int start, int end);
	/// Set the tab size in pixels for the call tip. 0 or -ve means no tab expand.
	void SetTabSize(int tabSz);
	/// Set calltip position.
	void SetPosition(bool aboveText);
	/// Used to determine which STYLE_xxxx to use for call tip information
	bool UseStyleCallTip() const { return BIN(Flags & fUseStyleCallTip); }
	// Modify foreground and background colours
	void SetForeBack(const ColourDesired &fore, const ColourDesired &back);
	bool IsInCollTipMode() const { return BIN(Flags & fInCallTipMode); }

	SciWindow wCallTip;
	SciWindow wDraw;
	int posStartCallTip;
	ColourDesired colourBG;
	ColourDesired colourUnSel;
	ColourDesired colourSel;
	ColourDesired colourShade;
	ColourDesired colourLight;
	int codePage;
	int clickPlace;
	int insetX; // text inset in x from calltip border
	int widthArrow;
	int borderHeight;
	int verticalOffset; // pixel offset up or down of the calltip with respect to the line
private:
	// Private so CallTip objects can not be copied
	CallTip(const CallTip &);
	CallTip & FASTCALL operator = (const CallTip &);
	void   DrawChunk(SciSurface *surface, int &x, const char *s, int posStart, int posEnd, int ytext, PRectangle rcClient, bool highlight, bool draw);
	int    PaintContents(SciSurface *surfaceWindow, bool draw);
	bool   IsTabCharacter(char c) const;
	int    NextTabPos(int x) const;

	int startHighlight;    // character offset to start and...
	int endHighlight;      // ...end of highlighted text
	//std::string val;
	SString Text;
	SciFont font;
	PRectangle rectUp;      // rectangle of last up angle in the tip
	PRectangle rectDown;    // rectangle of last down arrow in the tip
	int lineHeight;         // vertical line spacing
	int offsetMain;         // The alignment point of the call tip
	int tabSize;            // Tab size in pixels, <=0 no TAB expand
	//bool inCallTipMode;
	//bool useStyleCallTip;   // if true, STYLE_CALLTIP should be used
	//bool above;		// if true, display calltip above text
	enum {
		fInCallTipMode   = 0x0001,
		fUseStyleCallTip = 0x0002, // if true, STYLE_CALLTIP should be used
		fAbove           = 0x0004  // if true, display calltip above text
	};
	long   Flags;
};
//
// CaseConvert.h
//
enum CaseConversion {
	CaseConversionFold,
	CaseConversionUpper,
	CaseConversionLower
};

class ICaseConverter {
public:
	virtual size_t CaseConvertString(char *converted, size_t sizeConverted, const char *mixed, size_t lenMixed) = 0;
};

ICaseConverter * ConverterFor(enum CaseConversion conversion);

// Returns a UTF-8 string. Empty when no conversion
const char *CaseConvert(int character, enum CaseConversion conversion);

// When performing CaseConvertString, the converted value may be up to 3 times longer than the input.
// Ligatures are often decomposed into multiple characters and long cases include:
// ΐ "\xce\x90" folds to ΐ "\xce\xb9\xcc\x88\xcc\x81"
const int maxExpansionCaseConversion=3;

// Converts a mixed case string using a particular conversion.
// Result may be a different length to input and the length is the return value.
// If there is not enough space then 0 is returned.
size_t CaseConvertString(char *converted, size_t sizeConverted, const char *mixed, size_t lenMixed, enum CaseConversion conversion);

// Converts a mixed case string using a particular conversion.
std::string CaseConvertString(const std::string &s, enum CaseConversion conversion);
//
//
//
int FASTCALL UnicodeFromUTF8(const uchar * us);
//
// KeyMap.h
//
#define SCI_NORM 0
#define SCI_SHIFT SCMOD_SHIFT
#define SCI_CTRL SCMOD_CTRL
#define SCI_ALT SCMOD_ALT
#define SCI_META SCMOD_META
#define SCI_SUPER SCMOD_SUPER
#define SCI_CSHIFT (SCI_CTRL | SCI_SHIFT)
#define SCI_ASHIFT (SCI_ALT | SCI_SHIFT)
//
//
//
class KeyMap {
public:
	KeyMap();
	~KeyMap();
	void   Clear();
	void   AssignCmdKey(int key, int modifiers, uint msg);
	uint   Find(int key, int modifiers) const;	// 0 returned on failure
private:
	class KeyModifiers {
	public:
		KeyModifiers(int key_, int modifiers_);
		bool FASTCALL operator < (const KeyModifiers &other) const;
	private:
		int    key;
		int    modifiers;
	};
	class KeyToCommand {
	public:
		/*
		int    key;
		int    modifiers;
		uint   msg;
		*/
		int16  key;
		int8   modifiers;
		uint16 msg;
	};
	std::map <KeyModifiers, uint> kmap;
	static const KeyToCommand MapDefault[];
};
//
// Document.h
//
class RunStyles {
public:
	RunStyles();
	~RunStyles();
	int    Length() const;
	int    FASTCALL ValueAt(int position) const;
	int    FindNextChange(int position, int end) const;
	int    StartRun(int position) const;
	int    EndRun(int position) const;
	// Returns true if some values may have changed
	bool   FillRange(int &position, int value, int &fillLength);
	void   SetValueAt(int position, int value);
	void   InsertSpace(int position, int insertLength);
	void   DeleteAll();
	void   DeleteRange(int position, int deleteLength);
	int    Runs() const;
	bool   AllSame() const;
	bool   AllSameAs(int value) const;
	int    Find(int value, int start) const;
	void   Check() const;
private:
	Partitioning * starts;
	SplitVector <int> *styles;
	int RunFromPosition(int position) const;
	int SplitRun(int position);
	void RemoveRun(int run);
	void RemoveRunIfEmpty(int run);
	void RemoveRunIfSameAsPrevious(int run);
	// Private so RunStyles objects can not be copied
	RunStyles(const RunStyles &);
};
//
// Decoration.h
//
class Decoration {
public:
	explicit Decoration(int indicator_);
	~Decoration();
	bool Empty() const;

	Decoration * next;
	RunStyles rs;
	int indicator;
};

class DecorationList {
public:
	DecorationList();
	~DecorationList();
	void   SetCurrentIndicator(int indicator);
	int    GetCurrentIndicator() const { return currentIndicator; }
	void   SetCurrentValue(int value);
	int    GetCurrentValue() const { return currentValue; }
	// Returns true if some values may have changed
	bool   FillRange(int &position, int value, int &fillLength);
	void   InsertSpace(int position, int insertLength);
	void   DeleteRange(int position, int deleteLength);
	int    AllOnFor(int position) const;
	int    ValueAt(int indicator, int position);
	int    Start(int indicator, int position);
	int    End(int indicator, int position);

	Decoration * root;
	bool   clickNotified;
	uint8  Reserve[3]; // @alignment
private:
	Decoration * DecorationFromIndicator(int indicator);
	Decoration * Create(int indicator, int length);
	void   Delete(int indicator);
	void   DeleteAnyEmpty();

	int    currentIndicator;
	int    currentValue;
	int    lengthDocument;
	Decoration * current;
};
/**
 * A Position is a position within a document between two characters or at the beginning or end.
 * Sometimes used as a character index where it identifies the character after the position.
 */
typedef int Position;
const Position invalidPosition = -1;

enum EncodingFamily { efEightBit, efUnicode, efDBCS };
/**
 * The range class represents a range of text in a document.
 * The two values are not sorted as one end may be more significant than the other
 * as is the case for the selection where the end position is the position of the caret.
 * If either position is invalidPosition then the range is invalid and most operations will fail.
 */
class Range {
public:
	Position start;
	Position end;

	explicit Range(Position pos = 0) : start(pos), end(pos)
	{
	}
	Range(Position start_, Position end_) : start(start_), end(end_)
	{
	}
	bool FASTCALL operator == (const Range &other) const { return (start == other.start) && (end == other.end); }
	bool Valid() const { return (start != invalidPosition) && (end != invalidPosition); }
	Position First() const { return (start <= end) ? start : end; }
	Position Last() const { return (start > end) ? start : end; }
	//
	// Is the position within the range?
	//
	bool FASTCALL Contains(Position pos) const { return (start < end) ? (pos >= start && pos <= end) : (pos <= start && pos >= end); }
	//
	// Is the character after pos within the range?
	//
	bool FASTCALL ContainsCharacter(Position pos) const { return (start < end) ? (pos >= start && pos < end) : (pos < start && pos >= end); }
	bool FASTCALL Contains(Range other) const { return Contains(other.start) && Contains(other.end); }
	bool Overlaps(Range other) const { return Contains(other.start) || Contains(other.end) || other.Contains(start) || other.Contains(end); }
};
/**
 * Interface class for regular expression searching
 */
class RegexSearchBase {
public:
	virtual ~RegexSearchBase()
	{
	}
	virtual long FindText(Document * doc, int minPos, int maxPos, const char * s,
	    bool caseSensitive, bool word, bool wordStart, int flags, int * length) = 0;
	///@return String with the substitutions, must remain valid until the next call or destruction
	virtual const char * SubstituteByPosition(Document * doc, const char * text, int * length) = 0;
};

/// Factory function for RegexSearchBase
extern RegexSearchBase * CreateRegexSearch(CharClassify * charClassTable);

inline int LevelNumber(int level) { return level & SC_FOLDLEVELNUMBERMASK; }

class LexInterface {
public:
	explicit LexInterface(Document * pdoc_);
	virtual ~LexInterface();
	void   Colourise(int start, int end);
	int    LineEndTypesSupported();
	bool   UseContainerLexing() const { return instance == 0; }
protected:
	Document * pdoc;
	ILexer * instance;
	bool   performingStyle;   ///< Prevent reentrance
	uint8  Reserve[3]; // @alignment
};

struct RegexError : public std::runtime_error {
	RegexError() : std::runtime_error("regex failure")
	{
	}
};
//
//
//
class Document : PerLine, public IDocumentWithLineEnd, public ILoader {
public:
	//
	// Descr: Used to pair watcher pointer with user data.
	//
	struct WatcherWithUserData {
		WatcherWithUserData(DocWatcher * watcher_ = 0, void * userData_ = 0);
		bool FASTCALL operator == (const WatcherWithUserData &other) const;

		DocWatcher * watcher;
		void * userData;
	};
	struct StyledText {
		//StyledText(size_t length_, const char * text_, bool multipleStyles_, int style_, const uchar * styles_);
		StyledText(const LineAnnotation & rLA, int line);
		// Return number of bytes from start to before '\n' or end of text.
		// Return 1 when start is outside text
		size_t FASTCALL LineLength(size_t start) const;
		size_t FASTCALL StyleAt(size_t i) const;

		size_t length;
		const  char * text;
		size_t style;
		const  uchar * styles;
		bool   multipleStyles;
		uint8  Reserve[3]; // @alignment
	};
	enum {
		dfInsertionSet       = 0x0001,
		dfMatchesValid       = 0x0002,
		dfUseTabs            = 0x0004,
		dfTabIndents         = 0x0008,
		dfBackspaceUnindents = 0x0010
	};
	bool   IsDocFlag(uint f) const { return LOGIC(DocFlags & f); }
	void   SetDocFlag(uint f, int set) { SETFLAG(DocFlags, f, set); }
private:
	// 
	// Descr: Holder for an expandable array of characters that supports undo and line markers.
	//   Based on article "Data Structures in a Bit-Mapped Text Editor"
	//   by Wilfred J. Hansen, Byte January 1987, page 183.
	// 
	class CellBuffer {
	public:
		CellBuffer();
		~CellBuffer();
		/// Retrieving positions outside the range of the buffer works and returns 0
		char   FASTCALL CharAt(int position) const;
		void   GetCharRange(char *buffer, int position, int lengthRetrieve) const;
		char   FASTCALL StyleAt(int position) const;
		void   GetStyleRange(uchar *buffer, int position, int lengthRetrieve) const;
		const char * BufferPointer();
		const char * RangePointer(int position, int rangeLength);
		int    GapPosition() const;
		int    Length() const;
		void   FASTCALL Allocate(int newSize);
		int    GetLineEndTypes() const { return utf8LineEnds; }
		void   SetLineEndTypes(int utf8LineEnds_);
		bool   ContainsLineEnd(const char *s, int length) const;
		void   SetPerLine(PerLine *pl);
		int    Lines() const;
		int    LineStart(int line) const;
		int    LineFromPosition(int pos) const { return lv.LineFromPosition(pos); }
		void   InsertLine(int line, int position, bool lineStart);
		void   RemoveLine(int line);
		const char * InsertString(int position, const char *s, int insertLength, bool &startSequence);
		/// Setting styles for positions outside the range of the buffer is safe and has no effect.
		/// @return true if the style of a character is changed.
		bool   SetStyleAt(int position, char styleValue);
		bool   SetStyleFor(int position, int length, char styleValue);
		const char *DeleteChars(int position, int deleteLength, bool &startSequence);
		bool   IsReadOnly() const;
		void   SetReadOnly(bool set);
		/// The save point is a marker in the undo stack where the container has stated that
		/// the buffer was saved. Undo and redo can move over the save point.
		void   SetSavePoint();
		bool   IsSavePoint() const;
		void   TentativeStart();
		void   TentativeCommit();
		bool   TentativeActive() const;
		int    TentativeSteps();
		bool   SetUndoCollection(bool collectUndo);
		bool   IsCollectingUndo() const;
		void   BeginUndoAction();
		void   EndUndoAction();
		void   AddUndoAction(int token, bool mayCoalesce);
		void   DeleteUndoHistory();
		/// To perform an undo, StartUndo is called to retrieve the number of steps, then UndoStep is
		/// called that many times. Similarly for redo.
		bool   CanUndo() const;
		int    StartUndo();
		const  UndoHistory::Action &GetUndoStep() const;
		void   PerformUndoStep();
		bool   CanRedo() const;
		int    StartRedo();
		const  UndoHistory::Action & GetRedoStep() const;
		void   PerformRedoStep();
	private:
		// 
		// Descr: The line vector contains information about each of the lines in a cell buffer.
		// 
		class LineVector {
		public:
			LineVector();
			~LineVector();
			void   Init();
			void   FASTCALL SetPerLine(PerLine * pl);
			void   InsertText(int line, int delta);
			void   InsertLine(int line, int position, bool lineStart);
			void   SetLineStart(int line, int position);
			void   FASTCALL RemoveLine(int line);
			int    Lines() const;
			int    FASTCALL LineFromPosition(int pos) const;
			int    FASTCALL LineStart(int line) const;
		private:
			Partitioning starts;
			PerLine * perLine;
		};
		SplitVector <char> substance;
		SplitVector <char> style;
		bool   readOnly;
		bool   collectingUndo;
		uint8  Reserve[2]; // @alignment
		int    utf8LineEnds;
		UndoHistory uh;
		LineVector lv;

		bool UTF8LineEndOverlaps(int position) const;
		void ResetLineEnds();
		/// Actions without undo
		void BasicInsertString(int position, const char *s, int insertLength);
		void BasicDeleteChars(int position, int deleteLength);
	};
	int    refCount;
	CellBuffer cb;
	CharClassify charClass;
	CaseFolder * pcf;
	int    endStyled;
	int    styleClock;
	int    enteredModification;
	int    enteredStyling;
	int    enteredReadOnlyCount;
	uint   DocFlags;
	/*
	bool   insertionSet;
	bool   matchesValid;
	bool   useTabs;
	bool   tabIndents;
	bool   backspaceUnindents;
	*/
	std::string insertion;
	std::vector <WatcherWithUserData> watchers;

	// ldSize is not real data - it is for dimensions and loops
	enum lineData { 
		ldMarkers, 
		ldLevels, 
		ldState, 
		ldMargin, 
		ldAnnotation, 
		ldSize 
	};
	PerLine * perLineData[ldSize];
	RegexSearchBase * regex;
public:
	struct CharacterExtracted {
		uint character;
		uint widthBytes;
		CharacterExtracted(uint character_, uint widthBytes_) : character(character_), widthBytes(widthBytes_)
		{
		}
		// For DBCS characters turn 2 bytes into an int
		static CharacterExtracted FASTCALL DBCS(uchar lead, uchar trail)
		{
			return CharacterExtracted((lead << 8) | trail, 2);
		}
	};
	class HighlightDelimiter {
	public:
		HighlightDelimiter();
		void   Clear();
		bool   FASTCALL NeedsDrawing(int line) const;
		bool   FASTCALL IsFoldBlockHighlighted(int line) const;
		bool   FASTCALL IsHeadOfFoldBlock(int line) const;
		bool   FASTCALL IsBodyOfFoldBlock(int line) const;
		bool   FASTCALL IsTailOfFoldBlock(int line) const;

		int    beginFoldBlock;     // Begin of current fold block
		int    endFoldBlock;       // End of current fold block
		int    firstChangeableLineBefore;  // First line that triggers repaint before starting line that determined current fold block
		int    firstChangeableLineAfter;   // First line that triggers repaint after starting line that determined current fold block
		bool   isEnabled;
		uint8  Reserve[3]; // @alignment
	};

	LexInterface * pli;
	int    eolMode;
	/// Can also be SC_CP_UTF8 to enable UTF-8 mode
	int    dbcsCodePage;
	int    lineEndBitSet;
	int    tabInChars;
	int    indentInChars;
	int    actualIndentInChars;
	double durationStyleOneLine;
	DecorationList decorations;

	Document();
	virtual ~Document();
	int AddRef();
	int SCI_METHOD Release();
	virtual void Init();
	virtual void InsertLine(int line);
	virtual void RemoveLine(int line);
	int LineEndTypesSupported() const;
	bool SetDBCSCodePage(int dbcsCodePage_);
	int GetLineEndTypesAllowed() const { return cb.GetLineEndTypes(); }
	bool SetLineEndTypesAllowed(int lineEndBitSet_);
	int GetLineEndTypesActive() const { return cb.GetLineEndTypes(); }
	int SCI_METHOD Version() const { return dvLineEnd; }
	void SCI_METHOD SetErrorStatus(int status);
	Sci_Position SCI_METHOD LineFromPosition(Sci_Position pos) const;
	int  FASTCALL ClampPositionIntoDocument(int pos) const;
	bool ContainsLineEnd(const char * s, int length) const { return cb.ContainsLineEnd(s, length); }
	bool FASTCALL IsCrLf(int pos) const;
	int  FASTCALL LenChar(int pos);
	bool InGoodUTF8(int pos, int &start, int &end) const;
	int  MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd = true) const;
	int  NextPosition(int pos, int moveDir) const;
	bool NextCharacter(int &pos, int moveDir) const;        // Returns true if pos changed
	Document::CharacterExtracted FASTCALL CharacterAfter(int position) const;
	Document::CharacterExtracted FASTCALL CharacterBefore(int position) const;
	Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const;
	int  GetRelativePositionUTF16(int positionStart, int characterOffset) const;
	int  SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position * pWidth) const;
	int  SCI_METHOD CodePage() const;
	bool SCI_METHOD IsDBCSLeadByte(char ch) const;
	int  SafeSegment(const char * text, int length, int lengthSegment) const;
	EncodingFamily CodePageFamily() const;

	// Gateways to modifying document
	void FASTCALL ModifiedAt(int pos);
	void CheckReadOnly();
	bool DeleteChars(int pos, int len);
	int  InsertString(int position, const char * s, int insertLength);
	void ChangeInsertion(const char * s, int length);
	int  SCI_METHOD AddData(char * data, Sci_Position length);
	void * SCI_METHOD ConvertToDocument();
	int  Undo();
	int  Redo();
	bool CanUndo() const { return cb.CanUndo(); }
	bool CanRedo() const { return cb.CanRedo(); }
	void DeleteUndoHistory() { cb.DeleteUndoHistory(); }
	bool SetUndoCollection(bool collectUndo) { return cb.SetUndoCollection(collectUndo); }
	bool IsCollectingUndo() const { return cb.IsCollectingUndo(); }
	void BeginUndoAction() { cb.BeginUndoAction(); }
	void EndUndoAction() { cb.EndUndoAction(); }
	void AddUndoAction(int token, bool mayCoalesce) { cb.AddUndoAction(token, mayCoalesce); }
	void SetSavePoint();
	bool IsSavePoint() const { return cb.IsSavePoint(); }
	void TentativeStart() { cb.TentativeStart(); }
	void TentativeCommit() { cb.TentativeCommit(); }
	void TentativeUndo();
	bool TentativeActive() const { return cb.TentativeActive(); }
	const char * SCI_METHOD BufferPointer() { return cb.BufferPointer(); }
	const char * RangePointer(int position, int rangeLength) { return cb.RangePointer(position, rangeLength); }
	int GapPosition() const { return cb.GapPosition(); }
	int SCI_METHOD GetLineIndentation(Sci_Position line);
	int SetLineIndentation(int line, int indent);
	int GetLineIndentPosition(int line) const;
	int FASTCALL GetColumn(int position);
	int CountCharacters(int startPos, int endPos) const;
	int CountUTF16(int startPos, int endPos) const;
	int FindColumn(int line, int column);
	void Indent(bool forwards, int lineBottom, int lineTop);
	static std::string TransformLineEnds(const char * s, size_t len, int eolModeWanted);
	void ConvertLineEnds(int eolModeSet);
	void SetReadOnly(bool set) { cb.SetReadOnly(set); }
	bool IsReadOnly() const { return cb.IsReadOnly(); }
	void DelChar(int pos);
	void DelCharBack(int pos);
	char FASTCALL CharAt(int position) const { return cb.CharAt(position); }
	void SCI_METHOD GetCharRange(char * buffer, Sci_Position position, Sci_Position lengthRetrieve) const
	{
		cb.GetCharRange(buffer, position, lengthRetrieve);
	}
	char SCI_METHOD StyleAt(Sci_Position position) const
	{
		return cb.StyleAt(position);
	}
	int StyleIndexAt(Sci_Position position) const
	{
		return static_cast<uchar>(cb.StyleAt(position));
	}
	void GetStyleRange(uchar * buffer, int position, int lengthRetrieve) const
	{
		cb.GetStyleRange(buffer, position, lengthRetrieve);
	}
	int    GetMark(int line);
	int    MarkerNext(int lineStart, int mask) const;
	int    AddMark(int line, int markerNum);
	void   AddMarkSet(int line, int valueSet);
	void   DeleteMark(int line, int markerNum);
	void   DeleteMarkFromHandle(int markerHandle);
	void   DeleteAllMarks(int markerNum);
	int    LineFromHandle(int markerHandle);
	Sci_Position SCI_METHOD LineStart(Sci_Position line) const;
	bool   IsLineStartPosition(int position) const;
	Sci_Position SCI_METHOD LineEnd(Sci_Position line) const;
	int    LineEndPosition(int position) const;
	bool   FASTCALL IsLineEndPosition(int position) const;
	bool   FASTCALL IsPositionInLineEnd(int position) const;
	int    VCHomePosition(int position) const;
	int    SCI_METHOD SetLevel(Sci_Position line, int level);
	int    SCI_METHOD GetLevel(Sci_Position line) const;
	void   ClearLevels();
	int    GetLastChild(int lineParent, int level = -1, int lastLine = -1);
	int    GetFoldParent(int line) const;
	void   GetHighlightDelimiters(Document::HighlightDelimiter & hDelimiter, int line, int lastLine);
	void   Indent(bool forwards);
	int    ExtendWordSelect(int pos, int delta, bool onlyWordCharacters = false) const;
	int    NextWordStart(int pos, int delta) const;
	int    NextWordEnd(int pos, int delta) const;
	Sci_Position SCI_METHOD Length() const { return cb.Length(); }
	void   FASTCALL Allocate(int newSize) { cb.Allocate(newSize); }
	CharacterExtracted ExtractCharacter(int position) const;
	bool   IsWordStartAt(int pos) const;
	bool   IsWordEndAt(int pos) const;
	bool   IsWordAt(int start, int end) const;
	bool   MatchesWordOptions(bool word, bool wordStart, int pos, int length) const;
	bool   HasCaseFolder() const;
	void   SetCaseFolder(CaseFolder * pcf_);
	long   FindText(int minPos, int maxPos, const char * search, int flags, int * length);
	const  char * SubstituteByPosition(const char * text, int * length);
	int    LinesTotal() const;
	void   SetDefaultCharClasses(bool includeWordClass);
	void   SetCharClasses(const uchar * chars, CharClassify::cc newCharClass);
	int    GetCharsOfClass(CharClassify::cc characterClass, uchar * buffer) const;
	void   SCI_METHOD StartStyling(Sci_Position position, char mask);
	bool   SCI_METHOD SetStyleFor(Sci_Position length, char style);
	bool   SCI_METHOD SetStyles(Sci_Position length, const char * styles);
	int    GetEndStyled() const { return endStyled; }
	void   EnsureStyledTo(int pos);
	void   StyleToAdjustingLineDuration(int pos);
	void   LexerChanged();
	int    GetStyleClock() const { return styleClock; }
	void   IncrementStyleClock();
	void   SCI_METHOD DecorationSetCurrentIndicator(int indicator) { decorations.SetCurrentIndicator(indicator); }
	void   SCI_METHOD DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength);
	int    SCI_METHOD SetLineState(Sci_Position line, int state);
	int    SCI_METHOD GetLineState(Sci_Position line) const;
	int    GetMaxLineState();
	void   SCI_METHOD ChangeLexerState(Sci_Position start, Sci_Position end);
	StyledText MarginStyledText(int line) const;
	void   MarginSetStyle(int line, int style);
	void   MarginSetStyles(int line, const uchar * styles);
	void   MarginSetText(int line, const char * text);
	void   MarginClearAll();
	StyledText AnnotationStyledText(int line) const;
	void   AnnotationSetText(int line, const char * text);
	void   AnnotationSetStyle(int line, int style);
	void   AnnotationSetStyles(int line, const uchar * styles);
	int    FASTCALL AnnotationLines(int line) const;
	void   AnnotationClearAll();
	bool   AddWatcher(DocWatcher * watcher, void * userData);
	bool   RemoveWatcher(DocWatcher * watcher, void * userData);
	bool   FASTCALL IsASCIIWordByte(uchar ch) const;
	CharClassify::cc WordCharacterClass(uint ch) const;
	bool   IsWordPartSeparator(uint ch) const;
	int    WordPartLeft(int pos) const;
	int    WordPartRight(int pos) const;
	int    ExtendStyleRange(int pos, int delta, bool singleLine = false);
	bool   FASTCALL IsWhiteLine(int line) const;
	int    FASTCALL ParaUp(int pos) const;
	int    FASTCALL ParaDown(int pos) const;
	int    IndentSize() const { return actualIndentInChars; }
	int    BraceMatch(int position, int maxReStyle);
private:
	void   NotifyModifyAttempt();
	void   NotifySavePoint(bool atSavePoint);
	void   FASTCALL NotifyModified(const DocModification & mh);
};

class UndoGroup {
public:
	UndoGroup(Document * pdoc_, bool groupNeeded_ = true);
	~UndoGroup();
	bool Needed() const;
private:
	Document * pdoc;
	bool   groupNeeded;
	uint8  Reserve[3]; // @alignment
};
// 
// To optimise processing of document modifications by DocWatchers, a hint is passed indicating the
// scope of the change.
// If the DocWatcher is a document view then this can be used to optimise screen updating.
// 
class DocModification {
public:
	DocModification(int modificationType_, int position_ = 0, int length_ = 0, int linesAdded_ = 0, const char * text_ = 0, int line_ = 0);
	DocModification(int modificationType_, const UndoHistory::Action & act, int linesAdded_ = 0);

	int    modificationType;
	int    position;
	int    length;
	int    linesAdded; /**< Negative if lines deleted. */
	const  char * text; /**< Only valid for changes to text, not for changes to style. */
	int    line;
	int    foldLevelNow;
	int    foldLevelPrev;
	int    annotationLinesAdded;
	int    token;
};
// 
// A class that wants to receive notifications from a Document must be derived from DocWatcher
// and implement the notification methods. It can then be added to the watcher list with AddWatcher.
// 
class DocWatcher {
public:
	virtual ~DocWatcher()
	{
	}
	virtual void NotifyModifyAttempt(Document * doc, void * userData) = 0;
	virtual void NotifySavePoint(Document * doc, void * userData, bool atSavePoint) = 0;
	virtual void NotifyModified(Document * doc, const DocModification & rMh, void * userData) = 0;
	virtual void NotifyDeleted(Document * doc, void * userData) = 0;
	virtual void NotifyStyleNeeded(Document * doc, void * userData, int endPos) = 0;
	virtual void NotifyLexerChanged(Document * doc, void * userData) = 0;
	virtual void NotifyErrorOccurred(Document * doc, void * userData, int status) = 0;
};
//
//
//
static inline bool IsEOLChar(char ch)
{
	return oneof2(ch, '\r', '\n');
}
// 
// Descr: A point in document space.
//   Uses double for sufficient resolution in large (>20,000,000 line) documents.
// 
class PointDocument {
public:
	explicit PointDocument(double x_ = 0, double y_ = 0) : x(x_), y(y_)
	{
	}
	// Conversion from SciPoint.
	explicit PointDocument(SciPoint pt) : x(pt.x), y(pt.y)
	{
	}
	double x;
	double y;
};
//
// There are two points for some positions and this enumeration
// can choose between the end of the first line or subline and the start of the next line or subline.
//
enum PointEnd {
	peDefault = 0x0,
	peLineEnd = 0x1,
	peSubLineEnd = 0x2
};
//
//
//
class LineLayout {
public:
	enum validLevel { 
		llInvalid, 
		llCheckTextAndStyle, 
		llPositions, 
		llLines 
	};
	explicit LineLayout(int maxLineLength_);
	virtual ~LineLayout();
	void   FASTCALL Resize(int maxLineLength_);
	void   Free();
	void   FASTCALL Invalidate(validLevel validity_);
	int    FASTCALL LineStart(int line) const;
	int    FASTCALL LineLastVisible(int line) const;
	Range  FASTCALL SubLineRange(int line) const;
	bool   InLine(int offset, int line) const;
	void   SetLineStart(int line, int start);
	void   SetBracesHighlight(Range rangeLine, const Position braces[], char bracesMatchStyle, int xHighlight, bool ignoreStyle);
	void   RestoreBracesHighlight(Range rangeLine, const Position braces[], bool ignoreStyle);
	int    FindBefore(XYPOSITION x, int lower, int upper) const;
	int    FindPositionFromX(XYPOSITION x, Range range, bool charPosition) const;
	SciPoint  PointFromPosition(int posInLine, int lineHeight, PointEnd pe) const;
	int    EndLineStyle() const;

	enum { 
		wrapWidthInfinite = 0x7ffffff 
	};
	validLevel validity;
	int maxLineLength;
	int numCharsInLine;
	int numCharsBeforeEOL;
	int xHighlightGuide;
	int edgeColumn;
	char * chars;
	uchar * styles;
	XYPOSITION * positions;
	bool highlightColumn;
	bool containsCaret;
	char bracePreviousStyles[2];
	Range hotspot; // Hotspot support
	// Wrapped line support
	int widthLine;
	int lines;
	XYPOSITION wrapIndent; // In pixels
private:
	friend class LineLayoutCache;
	int * lineStarts;
	int lenLineStarts;
	/// Drawing is only performed for @a maxLineLength characters on each line.
	int lineNumber;
	bool inCache;
};
//
// ViewStyle.h
//
class FontRealised : public FontMeasurements {
	// Private so FontRealised objects can not be copied
	FontRealised(const FontRealised &);
	FontRealised & operator = (const FontRealised &);
public:
	SciFont font;
	FontRealised();
	virtual ~FontRealised();
	void Realise(SciSurface &surface, int zoomLevel, int technology, const FontSpecification &fs);
};

enum IndentView {
	ivNone, 
	ivReal, 
	ivLookForward, 
	ivLookBoth
};

enum WhiteSpaceVisibility {
	wsInvisible=0, 
	wsVisibleAlways=1, 
	wsVisibleAfterIndent=2, 
	wsVisibleOnlyInIndent=3
};

enum TabDrawMode {
	tdLongArrow=0, 
	tdStrikeOut=1
};

typedef std::map <FontSpecification, FontRealised *> FontMap;

enum WrapMode { 
	eWrapNone, 
	eWrapWord, 
	eWrapChar, 
	eWrapWhitespace 
};

class ColourOptional : public ColourDesired {
public:
	ColourOptional(ColourDesired colour_=ColourDesired(0,0,0), bool isSet_=false) : ColourDesired(colour_), isSet(isSet_) 
	{
	}
	ColourOptional(uptr_t wParam, sptr_t lParam) : ColourDesired(static_cast<long>(lParam)), isSet(wParam != 0) 
	{
	}
	bool  isSet;
	uint8 CoReserve[3]; // @alignment
};
//
//
//
class ViewStyle {
public:
	ViewStyle();
	ViewStyle(const ViewStyle &source);
	~ViewStyle();
	void   CalculateMarginWidthAndMask();
	void   Init(size_t stylesSize_=256);
	void   Refresh(SciSurface &surface, int tabInChars);
	void   ReleaseAllExtendedStyles();
	int    FASTCALL AllocateExtendedStyles(int numberStyles);
	void   EnsureStyle(size_t index);
	void   ResetDefaultStyle();
	void   ClearStyles();
	void   SetStyleFontName(int styleIndex, const char *name);
	bool   ProtectionActive() const;
	int    ExternalMarginWidth() const;
	int    MarginFromLocation(SciPoint pt) const;
	bool   ValidStyle(size_t styleIndex) const;
	void   CalcLargestMarkerHeight();
	ColourOptional Background(int marksOfLine, bool caretActive, bool lineContainsCaret) const;
	bool   SelectionBackgroundDrawn() const;
	bool   WhitespaceBackgroundDrawn() const;
	ColourDesired WrapColour() const;

	bool   SetWrapState(int wrapState_);
	bool   SetWrapVisualFlags(int wrapVisualFlags_);
	bool   SetWrapVisualFlagsLocation(int wrapVisualFlagsLocation_);
	bool   SetWrapVisualStartIndent(int wrapVisualStartIndent_);
	bool   SetWrapIndentMode(int wrapIndentMode_);
	bool   WhiteSpaceVisible(bool inIndent) const;

	struct EdgeProperties {
		EdgeProperties(int column_ = 0, ColourDesired colour_ = ColourDesired(0));
		EdgeProperties(uptr_t wParam, sptr_t lParam);

		int    column;
		ColourDesired colour;
	};
	class MarginStyle {
	public:
		MarginStyle();

		int    style;
		ColourDesired back;
		int    width;
		int    mask;
		int    cursor;
		bool   sensitive;
		uint8  Reserve[3]; // @alignment
	};
	struct ForeBackColours {
		ColourOptional fore;
		ColourOptional back;
	};
	size_t nextExtendedStyle;
	LineMarker markers[MARKER_MAX + 1];
	int    largestMarkerHeight;
	Indicator indicators[INDIC_MAX + 1];
	uint   indicatorsDynamic;
	uint   indicatorsSetFore;
	int    technology;
	int    lineHeight;
	int    lineOverlap;
	uint   maxAscent;
	uint   maxDescent;
	XYPOSITION aveCharWidth;
	XYPOSITION spaceWidth;
	XYPOSITION tabWidth;
	ForeBackColours selColours;
	ColourDesired selAdditionalForeground;
	ColourDesired selAdditionalBackground;
	ColourDesired selBackground2;
	int    selAlpha;
	int    selAdditionalAlpha;
	ForeBackColours whitespaceColours;
	int    controlCharSymbol;
	XYPOSITION controlCharWidth;
	ColourDesired selbar;
	ColourDesired selbarlight;
	ColourOptional foldmarginColour;
	ColourOptional foldmarginHighlightColour;
	ForeBackColours hotspotColours;
	/// Margins are ordered: Line Numbers, Selection Margin, Spacing Margin
	int    leftMarginWidth;	///< Spacing margin on left of text
	int    rightMarginWidth;	///< Spacing margin on right of text
	int    maskInLine;	///< Mask for markers to be put into text because there is nowhere for them to go in margin
	int    maskDrawInText;	///< Mask for markers that always draw in text
	int    fixedColumnWidth;	///< Total width of margins
	int    textStart;	///< Starting x position of text within the view
	int    zoomLevel;
	WhiteSpaceVisibility viewWhitespace;
	TabDrawMode tabDrawMode;
	int    whitespaceSize;
	IndentView viewIndentationGuides;
	ColourDesired caretcolour;
	ColourDesired additionalCaretColour;
	ColourDesired caretLineBackground;
	int    caretLineAlpha;
	int    caretStyle;
	int    caretWidth;
	int    extraFontFlag;
	int    extraAscent;
	int    extraDescent;
	int    marginStyleOffset;
	int    annotationVisible;
	int    annotationStyleOffset;
	int    braceHighlightIndicator;
	int    braceBadLightIndicator;
	int    edgeState;
	EdgeProperties theEdge;
	int    marginNumberPadding; // the right-side padding of the number margin
	int    ctrlCharPadding; // the padding around control character text blobs
	int    lastSegItalicsOffset; // the offset so as not to clip italic characters at EOLs

	// Wrapping support
	WrapMode wrapState;
	int    wrapVisualFlags;
	int    wrapVisualFlagsLocation;
	int    wrapVisualStartIndent;
	int    wrapIndentMode; // SC_WRAPINDENT_FIXED, _SAME, _INDENT

	bool   selEOLFilled;
	bool   hotspotUnderline;
	bool   hotspotSingleLine;
	bool   marginInside;	///< true: margin included in text view, false: separate views
	bool   viewEOL;
	bool   showCaretLineBackground;
	bool   alwaysShowCaretLineBackground;
	bool   someStylesProtected;
	bool   someStylesForceCase;
	bool   braceHighlightIndicatorSet;
	bool   braceBadLightIndicatorSet;
	uint8  Reserve; // @alignment

	std::vector <SciStyle> styles;
	std::vector <MarginStyle> ms;
	std::vector <EdgeProperties> theMultiEdge;
private:
	void   AllocStyles(size_t sizeNew);
	void   CreateAndAddFont(const FontSpecification &fs);
	FontRealised *Find(const FontSpecification &fs);
	void   FindMaxAscentDescent();
	// Private so can only be copied through copy constructor which ensures font names initialised correctly
	ViewStyle & operator = (const ViewStyle &);

	class FontNames {
	public:
		FontNames();
		~FontNames();
		void Clear();
		const char * Save(const char *name);
	private:
		std::vector <char *> names;
		// Private so FontNames objects can not be copied
		FontNames(const FontNames &);
	};

	FontNames fontNames;
	FontMap fonts;
};
//
// Selection.h
//
class SelectionPosition { // @flat
public:
	explicit SelectionPosition(int position_ = INVALID_POSITION, int virtualSpace_ = 0);
	void Reset();
	void MoveForInsertDelete(bool insertion, int startChange, int length);
	bool FASTCALL operator == (const SelectionPosition &other) const;
	bool FASTCALL operator != (const SelectionPosition &other) const;
	bool FASTCALL operator < (const SelectionPosition &other) const;
	bool FASTCALL operator > (const SelectionPosition &other) const;
	bool FASTCALL operator <= (const SelectionPosition &other) const;
	bool FASTCALL operator >= (const SelectionPosition &other) const;
	int Position() const { return position; }
	int VirtualSpace() const { return virtualSpace; }
	bool IsValid() const { return (position >= 0); }
	void FASTCALL SetPosition(int position_);
	void FASTCALL SetVirtualSpace(int virtualSpace_);
	void FASTCALL Add(int increment);
private:
	int    position;
	int    virtualSpace;
};
//
// Ordered range to make drawing simpler
//
struct SelectionSegment {
	SelectionSegment();
	SelectionSegment(const SelectionPosition & rA, const SelectionPosition & rB);
	bool Empty() const;
	void FASTCALL Extend(const SelectionPosition & rP);

	SelectionPosition start;
	SelectionPosition end;
};

struct SelectionRange {
	SelectionRange();
	explicit SelectionRange(SelectionPosition single);
	explicit SelectionRange(int single);
	SelectionRange(SelectionPosition caret_, SelectionPosition anchor_);
	SelectionRange(int caret_, int anchor_);
	bool Empty() const { return (anchor == caret); }
	int Length() const;
	// int Width() const;	// Like Length but takes virtual space into account
	bool FASTCALL operator == (const SelectionRange &other) const { return (caret == other.caret && anchor == other.anchor); }
	bool FASTCALL operator < (const SelectionRange &other) const { return (caret < other.caret || ((caret == other.caret) && (anchor < other.anchor))); }
	void Reset();
	void ClearVirtualSpace();
	void MoveForInsertDelete(bool insertion, int startChange, int length);
	bool FASTCALL Contains(int pos) const;
	bool Contains(SelectionPosition sp) const;
	bool FASTCALL ContainsCharacter(int posCharacter) const;
	SelectionSegment Intersect(SelectionSegment check) const;
	SelectionPosition Start() const;
	SelectionPosition End() const;
	void Swap();
	bool Trim(SelectionRange range);
	// If range is all virtual collapse to start of virtual space
	void MinimizeVirtualSpace();

	SelectionPosition caret;
	SelectionPosition anchor;
};

class Selection {
public:
	Selection();
	~Selection();
	bool IsRectangular() const;
	int MainCaret() const;
	int MainAnchor() const;
	SelectionRange & Rectangular();
	SelectionSegment Limits() const;
	// This is for when you want to move the caret in response to a
	// user direction command - for rectangular selections, use the range
	// that covers all selected text otherwise return the main selection.
	SelectionSegment LimitsForRectangularElseMain() const;
	size_t Count() const;
	size_t Main() const;
	void SetMain(size_t r);
	SelectionRange & Range(size_t r);
	const SelectionRange & Range(size_t r) const;
	SelectionRange & RangeMain();
	const SelectionRange & RangeMain() const;
	SelectionPosition Start() const;
	bool MoveExtends() const;
	void SetMoveExtends(bool moveExtends_);
	bool Empty() const;
	SelectionPosition Last() const;
	int Length() const;
	void MovePositions(bool insertion, int startChange, int length);
	void TrimSelection(SelectionRange range);
	void TrimOtherSelections(size_t r, SelectionRange range);
	void SetSelection(SelectionRange range);
	void AddSelection(SelectionRange range);
	void AddSelectionWithoutTrim(SelectionRange range);
	void DropSelection(size_t r);
	void DropAdditionalRanges();
	void TentativeSelection(SelectionRange range);
	void CommitTentative();
	int CharacterInSelection(int posCharacter) const;
	int InSelectionForEOL(int pos) const;
	int VirtualSpaceFor(int pos) const;
	void Clear();
	void RemoveDuplicates();
	void RotateMain();
	bool Tentative() const
	{
		return tentativeMain;
	}
	std::vector <SelectionRange> RangesCopy() const
	{
		return Ranges;
	}
	enum selTypes { 
		noSel, 
		selStream, 
		selRectangle, 
		selLines, 
		selThin 
	};
	selTypes selType;
private:
	std::vector <SelectionRange> Ranges;
	std::vector <SelectionRange> RangesSaved;
	SelectionRange rangeRectangular;
	size_t mainRange;
	bool   moveExtends;
	bool   tentativeMain;
};
//
// PositionCache.h
//
class LineLayoutCache {
public:
	LineLayoutCache();
	virtual ~LineLayoutCache();
	void   Deallocate();
	enum {
		llcNone = SC_CACHE_NONE,
		llcCaret = SC_CACHE_CARET,
		llcPage = SC_CACHE_PAGE,
		llcDocument = SC_CACHE_DOCUMENT
	};
	void   Invalidate(LineLayout::validLevel validity_);
	void   SetLevel(int level_);
	int    GetLevel() const
	{
		return level;
	}
	LineLayout * Retrieve(int lineNumber, int lineCaret, int maxChars, int styleClock_, int linesOnScreen, int linesInDoc);
	void   FASTCALL Dispose(LineLayout * ll);
private:
	void   FASTCALL Allocate(size_t length_);
	void   AllocateForLevel(int linesOnScreen, int linesInDoc);

	int    level;
	std::vector <LineLayout *> cache;
	int    styleClock;
	int    useCount;
	bool   allInvalidated;
	uint8  Reserve[3]; // @alignment
};

class PositionCacheEntry {
public:
	static uint Hash(uint styleNumber_, const char * s, uint len);

	PositionCacheEntry();
	~PositionCacheEntry();
	void Set(uint styleNumber_, const char * s_, uint len_, const XYPOSITION * positions_, uint clock_);
	void Clear();
	bool Retrieve(uint styleNumber_, const char * s_, uint len_, XYPOSITION * positions_) const;
	bool NewerThan(const PositionCacheEntry &other) const;
	void ResetClock();
private:
	uint styleNumber : 8;
	uint len : 8;
	uint clock : 16;
	XYPOSITION * positions;
};

class SpecialRepresentations {
public:
	class Representation {
	public:
		explicit Representation(const char * value = "") : stringRep(value)
		{
		}
		std::string stringRep;
	};

	SpecialRepresentations();
	void SetRepresentation(const char * charBytes, const char * value);
	void ClearRepresentation(const char * charBytes);
	const Representation * RepresentationFromCharacter(const char * charBytes, size_t len) const;
	bool Contains(const char * charBytes, size_t len) const;
	void Clear();
private:
	typedef std::map <int, Representation> MapRepresentation;
	MapRepresentation mapReprs;
	short startByteHasReprs[0x100];
};
//
// Class to break a line of text into shorter runs at sensible places.
//
class BreakFinder {
public:
	struct TextSegment {
		TextSegment(int start_ = 0, int length_ = 0, const SpecialRepresentations::Representation * representation_ = 0);
		int    end() const;

		int    start;
		int    length;
		const  SpecialRepresentations::Representation * representation;
	};
	// If a whole run is longer than lengthStartSubdivision then subdivide
	// into smaller runs at spaces or punctuation.
	enum { 
		lengthStartSubdivision = 300 
	};
	// Try to make each subdivided run lengthEachSubdivision or shorter.
	enum { 
		lengthEachSubdivision = 100 
	};

	BreakFinder(const LineLayout * ll_, const Selection * psel, Range rangeLine_, int posLineStart_,
	    int xStart, bool breakForSelection, const Document * pdoc_, const SpecialRepresentations * preprs_, const ViewStyle * pvsDraw);
	~BreakFinder();
	TextSegment Next();
	bool More() const;
private:
	void Insert(int val);
	// Private so BreakFinder objects can not be copied
	BreakFinder(const BreakFinder &);

	const LineLayout * ll;
	Range lineRange;
	int posLineStart;
	int nextBreak;
	std::vector<int> selAndEdge;
	uint saeCurrentPos;
	int saeNext;
	int subBreak;
	const Document * pdoc;
	EncodingFamily encodingFamily;
	const SpecialRepresentations * preprs;
};

class PositionCache {
public:
	PositionCache();
	~PositionCache();
	void Clear();
	void SetSize(size_t size_);
	size_t GetSize() const
	{
		return pces.size();
	}
	void MeasureWidths(SciSurface * surface, const ViewStyle &vstyle, uint styleNumber, const char * s, uint len, XYPOSITION * positions, Document * pdoc);
private:
	// Private so PositionCache objects can not be copied
	PositionCache(const PositionCache &);

	std::vector <PositionCacheEntry> pces;
	uint clock;
	bool allClear;
	uint8 Reserve[3]; // @alignment
};

// inline bool IsSpaceOrTab_Removed(int ch) { return oneof2(ch, ' ', '\t'); }

//
// ContractionState.h
//
template <class T> class SparseVector;
//
//
//
class ContractionState {
public:
	ContractionState();
	virtual ~ContractionState();
	void Clear();
	int LinesInDoc() const;
	int LinesDisplayed() const;
	int DisplayFromDoc(int lineDoc) const;
	int DisplayLastFromDoc(int lineDoc) const;
	int DocFromDisplay(int lineDisplay) const;
	void InsertLine(int lineDoc);
	void InsertLines(int lineDoc, int lineCount);
	void DeleteLine(int lineDoc);
	void DeleteLines(int lineDoc, int lineCount);
	bool FASTCALL GetVisible(int lineDoc) const;
	bool SetVisible(int lineDocStart, int lineDocEnd, bool isVisible);
	bool HiddenLines() const;
	const char *GetFoldDisplayText(int lineDoc) const;
	bool SetFoldDisplayText(int lineDoc, const char *text);
	bool FASTCALL GetExpanded(int lineDoc) const;
	bool SetExpanded(int lineDoc, bool isExpanded);
	bool GetFoldDisplayTextShown(int lineDoc) const;
	int ContractedNext(int lineDocStart) const;
	int GetHeight(int lineDoc) const;
	bool SetHeight(int lineDoc, int height);
	void ShowAll();
	void Check() const;
private:
	void EnsureData();
	bool OneToOne() const 
	{
		// True when each document line is exactly one display line so need for
		// complex data structures.
		return visible == 0;
	}
	// These contain 1 element for every document line.
	RunStyles * visible;
	RunStyles * expanded;
	RunStyles * heights;
	SparseVector <const char *> *foldDisplayTexts;
	Partitioning *displayLines;
	int linesInDocument;
};
//
// XPM.H
// 
// Hold a pixmap in XPM format.
// 
class XPM {
public:
	explicit XPM(const char *textForm);
	explicit XPM(const char *const *linesForm);
	~XPM();
	void Init(const char *textForm);
	void Init(const char *const *linesForm);
	/// Decompose image into runs and use FillRectangle for each run
	void Draw(SciSurface *surface, const PRectangle &rc);
	int GetHeight() const { return height; }
	int GetWidth() const { return width; }
	void PixelAt(int x, int y, ColourDesired &colour, bool &transparent) const;
private:
	ColourDesired ColourFromCode(int ch) const;
	void FillRun(SciSurface *surface, int code, int startX, int y, int x) const;
	static std::vector<const char *>LinesFormFromTextForm(const char *textForm);

	std::vector <uchar> Pixels; // @firstmember
	int    height;
	int    width;
	int    nColours;
	ColourDesired colourCodeTable[256];
	char   codeTransparent;
	uint8  Reserve[3]; // @alignment
};
// 
// A translucent image stored as a sequence of RGBA bytes.
// 
class RGBAImage {
public:
	RGBAImage(int width_, int height_, float scale_, const uchar *pixels_);
	explicit RGBAImage(const XPM &xpm);
	virtual ~RGBAImage();
	int GetHeight() const { return height; }
	int GetWidth() const { return width; }
	float GetScale() const { return scale; }
	float GetScaledHeight() const { return height / scale; }
	float GetScaledWidth() const { return width / scale; }
	int CountBytes() const;
	const uchar *Pixels() const;
	void SetPixel(int x, int y, ColourDesired colour, int alpha=0xff);
private:
	// Private so RGBAImage objects can not be copied
	RGBAImage(const RGBAImage &);
	RGBAImage & operator= (const RGBAImage &);

	int    height;
	int    width;
	float  scale;
	std::vector <uchar> pixelBytes;
};
// 
// A collection of RGBAImage pixmaps indexed by integer id.
// 
class RGBAImageSet {
public:
	RGBAImageSet();
	~RGBAImageSet();
	/// Remove all images.
	void Clear();
	/// Add an image.
	void Add(int ident, RGBAImage *image);
	/// Get image by id.
	RGBAImage *Get(int ident);
	/// Give the largest height of the set.
	int GetHeight() const;
	/// Give the largest width of the set.
	int GetWidth() const;
private:
	typedef std::map <int, RGBAImage*> ImageMap;
	ImageMap images;
	mutable int height;	///< Memorize largest height of the set.
	mutable int width;	///< Memorize largest width of the set.
};
//
//
//
class EditModel {
public:
	EditModel();
	virtual ~EditModel();
	virtual int TopLineOfMain() const = 0;
	virtual SciPoint GetVisibleOriginInMain() const = 0;
	virtual int LinesOnScreen() const = 0;
	virtual Range GetHotSpotRange() const = 0;
	ColourDesired SelectionBackground(const ViewStyle & vsDraw, bool main) const;

	class Caret {
	public:
		Caret();

		//bool   active;
		//bool   on;
		enum {
			fActive = 0x0001,
			fOn     = 0x0002
		};
		int    Flags;
		int    period;
	};
	uint   EditModelFlags; // @firstmember
	int    xOffset; ///< Horizontal scrolled amount in pixels
	SpecialRepresentations reprs;
	Caret  caret;
	SelectionPosition posDrag;
	Position braces[2];
	int    bracesMatchStyle;
	int    highlightGuideColumn;
	Selection Sel; // @sobolev sel-->Sel

	enum IMEInteraction { 
		imeWindowed, 
		imeInline 
	} imeInteraction;

	int    foldFlags;
	int    foldDisplayTextStyle;
	ContractionState cs;
	Range  hotspot; // Hotspot support
	int    hoverIndicatorPos;
	int    wrapWidth; // Wrapping support
	Document * pdoc;
	//bool   inOverstrike;
	//bool   trackLineWidth;
	//bool   primarySelection;
	enum {
		fInOverstrike     = 0x0001,
		fTrackLineWidth   = 0x0002,
		fPrimarySelection = 0x0004
	};
private:
	// Private so EditModel objects can not be copied
	explicit EditModel(const EditModel &);
	EditModel & FASTCALL operator = (const EditModel &);
};
//
//
//
void DrawWrapMarker(SciSurface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);

typedef void (*DrawWrapMarkerFn)(SciSurface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);
/**
* The view may be drawn in separate phases.
*/
enum DrawPhase {
	drawBack = 0x1,
	drawIndicatorsBack = 0x2,
	drawText = 0x4,
	drawIndentationGuides = 0x8,
	drawIndicatorsFore = 0x10,
	drawSelectionTranslucent = 0x20,
	drawLineTranslucent = 0x40,
	drawFoldLines = 0x80,
	drawCarets = 0x100,
	drawAll = 0x1FF
};

bool   ValidStyledText(const ViewStyle &vs, size_t styleOffset, const Document::StyledText &st);
int    WidestLineWidth(SciSurface *surface, const ViewStyle &vs, int styleOffset, const Document::StyledText &st);
void   DrawTextNoClipPhase(SciSurface *surface, PRectangle rc, const SciStyle & style, XYPOSITION ybase, const char *s, int len, DrawPhase phase);
void   DrawStyledText(SciSurface *surface, const ViewStyle &vs, int styleOffset, PRectangle rcText, const Document::StyledText &st, size_t start, size_t length, DrawPhase phase);

typedef void (*DrawTabArrowFn)(SciSurface *surface, PRectangle rcTab, int ymid);
// 
// Descr: EditView draws the main text area.
// 
class EditView {
public:
	struct PrintParameters {
		PrintParameters();
		int    magnification;
		int    colourMode;
		WrapMode wrapState;
	};
	PrintParameters printParameters;
	PerLine * ldTabstops;
	int tabWidthMinimumPixels;
	// 
	// Descr: In phasesTwo mode, drawing is performed in two phases, first the background
	//   and then the foreground. This avoids chopping off characters that overlap the next run.
	//   In multiPhaseDraw mode, drawing is performed in multiple phases with each phase drawing
	//   one feature over the whole drawing area, instead of within one line. This allows text to
	//   overlap from one line to the next. 
	// 
	enum PhasesDraw { 
		phasesOne, 
		phasesTwo, 
		phasesMultiple 
	};
	PhasesDraw phasesDraw;
	int    lineWidthMaxSeen;
	/*
	bool hideSelection;
	bool drawOverstrikeCaret;
	//
	// In bufferedDraw mode, graphics operations are drawn to a pixmap and then copied to
	// the screen. This avoids flashing but is about 30% slower. 
	//
	bool bufferedDraw;
	bool additionalCaretsBlink;
	bool additionalCaretsVisible;
	bool imeCaretBlockOverride;
	*/
	enum {
		fHideSelection           = 0x0001,
		fDrawOverstrikeCaret     = 0x0002,
		//
		// In bufferedDraw mode, graphics operations are drawn to a pixmap and then copied to
		// the screen. This avoids flashing but is about 30% slower. 
		//
		fBufferedDraw            = 0x0004,
		fAdditionalCaretsBlink   = 0x0008,
		fAdditionalCaretsVisible = 0x0010,
		fImeCaretBlockOverride   = 0x0020,
	};
	uint   EditViewFlags;
	SciSurface * pixmapLine;
	SciSurface * pixmapIndentGuide;
	SciSurface * pixmapIndentGuideHighlight;
	LineLayoutCache llc;
	PositionCache posCache;
	int tabArrowHeight; // draw arrow heads this many pixels above/below line midpoint
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * DrawTabArrow function for drawing tab characters. Allow those platforms to
	 * override it instead of creating a new method in the SciSurface class that
	 * existing platforms must implement as empty. */
	DrawTabArrowFn customDrawTabArrow;
	DrawWrapMarkerFn customDrawWrapMarker;

	EditView();
	virtual ~EditView();
	bool SetTwoPhaseDraw(bool twoPhaseDraw);
	bool SetPhasesDraw(int phases);
	bool LinesOverlap() const;
	void ClearAllTabstops();
	XYPOSITION NextTabstopPos(int line, XYPOSITION x, XYPOSITION tabWidth) const;
	bool ClearTabstops(int line);
	bool AddTabstop(int line, int x);
	int GetNextTabstop(int line, int x) const;
	void LinesAddedOrRemoved(int lineOfPos, int linesAdded);
	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(SciSurface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);
	LineLayout *RetrieveLineLayout(int lineNumber, const EditModel &model);
	void LayoutLine(const EditModel &model, int line, SciSurface *surface, const ViewStyle &vstyle,
		LineLayout *ll, int width = LineLayout::wrapWidthInfinite);
	SciPoint LocationFromPosition(SciSurface *surface, const EditModel &model, SelectionPosition pos, int topLine, const ViewStyle &vs, PointEnd pe);
	Range RangeDisplayLine(SciSurface *surface, const EditModel &model, int lineVisible, const ViewStyle &vs);
	SelectionPosition SPositionFromLocation(SciSurface *surface, const EditModel &model, const PointDocument & rPt, bool canReturnInvalid,
		bool charPosition, bool virtualSpace, const ViewStyle &vs);
	SelectionPosition SPositionFromLineX(SciSurface *surface, const EditModel &model, int lineDoc, int x, const ViewStyle &vs);
	int DisplayFromPosition(SciSurface *surface, const EditModel &model, int pos, const ViewStyle &vs);
	int StartEndDisplayLine(SciSurface *surface, const EditModel &model, int pos, bool start, const ViewStyle &vs);
	void DrawIndentGuide(SciSurface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight);
	void DrawEOL(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		int line, int lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart, ColourOptional background);
	void DrawFoldDisplayText(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int xStart, PRectangle rcLine, int subLine, XYACCUMULATOR subLineStart, DrawPhase phase);
	void DrawAnnotation(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void DrawCarets(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int xStart, PRectangle rcLine, int subLine) const;
	void DrawBackground(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		Range lineRange, int posLineStart, int xStart, int subLine, ColourOptional background) const;
	void DrawForeground(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int lineVisible,
		PRectangle rcLine, Range lineRange, int posLineStart, int xStart, int subLine, ColourOptional background);
	void DrawIndentGuidesOverEmpty(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int lineVisible, PRectangle rcLine, int xStart, int subLine);
	void DrawLine(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void PaintText(SciSurface *surfaceWindow, const EditModel &model, PRectangle rcArea, PRectangle rcClient, const ViewStyle &vsDraw);
	void FillLineRemainder(SciSurface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line, PRectangle rcArea, int subLine) const;
	long FormatRange(bool draw, Sci_RangeToFormat *pfr, SciSurface *surface, SciSurface *surfaceMeasure, const EditModel &model, const ViewStyle &vs);
};
// 
// Descr: Convenience class to ensure LineLayout objects are always disposed.
// 
class AutoLineLayout {
public:
	AutoLineLayout(LineLayoutCache &llc_, LineLayout *ll_);
	~AutoLineLayout();
	LineLayout * operator->() const 
	{
		return ll;
	}
	operator LineLayout *() const 
	{
		return ll;
	}
	void Set(LineLayout * ll_);
private:
	AutoLineLayout & operator = (const AutoLineLayout &);

	LineLayoutCache & llc;
	LineLayout * ll;
};
//
//
//
class AutoComplete {
public:
	enum {
		fActive           = 0x0001,
		fCancelAtStartPos = 0x0002, /// Should autocompletion be canceled if editor's currentPos <= startPos?
		fAutoHide         = 0x0004,
		fDropRestOfWord   = 0x0008,
		fIgnoreCase       = 0x0010,
		fChooseSingle     = 0x0020
	};
	ListBox * lb;
	int    posStart;
	int    startLen;
	uint   ignoreCaseBehaviour;
	int    widthLBDefault;
	int    heightLBDefault;
	/** SC_ORDER_PRESORTED:   Assume the list is presorted; selection will fail if it is not alphabetical<br />
	 *  SC_ORDER_PERFORMSORT: Sort the list alphabetically; start up performance cost for sorting<br />
	 *  SC_ORDER_CUSTOM:      Handle non-alphabetical entries; start up performance cost for generating a sorted lookup table
	 */
	int    autoSort;

	AutoComplete();
	~AutoComplete();

	/// Is the auto completion list displayed?
	bool   Active() const;
	long   GetFlags() const { return Flags; }
	void   SetFlag(long f, int doSet);
	/// Display the auto completion list positioned to be near a character position
	void Start(SciWindow &parent, int ctrlID, int position, SciPoint location, int startLen_, int lineHeight, bool unicodeMode, int technology);
	/// The stop chars are characters which, when typed, cause the auto completion list to disappear
	void SetStopChars(const char *stopChars_);
	bool IsStopChar(char ch);
	/// The fillup chars are characters which, when typed, fill up the selected word
	void SetFillUpChars(const char *fillUpChars_);
	bool IsFillUpChar(char ch);
	/// The separator character is used when interpreting the list in SetList
	void SetSeparator(char separator_);
	char GetSeparator() const;
	/// The typesep character is used for separating the word from the type
	void SetTypesep(char separator_);
	char GetTypesep() const;
	/// The list string contains a sequence of words separated by the separator character
	void SetList(const char *list);
	/// Return the position of the currently selected list item
	int GetSelection() const;
	/// Return the value of an item in the list
	std::string GetValue(int item) const;
	void Show(bool show);
	void Cancel();
	/// Move the current list element by delta, scrolling appropriately
	void Move(int delta);
	/// Select a list element that starts with word as the current element
	void Select(const char *word);
private:
	//bool active;
	//bool cancelAtStartPos; /// Should autocompletion be canceled if editor's currentPos <= startPos?
	//bool autoHide;
	//bool dropRestOfWord;
	//bool ignoreCase;
	//bool chooseSingle;
	long   Flags;
	char   separator;
	char   typesep; // Type seperator
	char   reserve[2]; // @alignment
	std::string stopChars;
	std::string fillUpChars;
	enum { 
		maxItemLen = 1000 
	};
	std::vector <int> sortMatrix;
};
//
//
//
class Editor : public EditModel, public DocWatcher {
public:
	friend class AutoSurface;
	friend class SelectionLineIterator;

	// Public so scintilla_send_message can use it.
	virtual sptr_t WndProc(uint iMessage, uptr_t wParam, sptr_t lParam);
	// Public so the COM thunks can access it.
	bool   IsUnicodeMode() const;

	int    ctrlID;      // Public so scintilla_set_id can use it.
	int    errorStatus; // Public so COM methods for drag and drop can set it.
protected: // ScintillaBase subclass needs access to much of Editor
	// 
	// Descr: Hold a piece of text selected for copying or dragging, along with 
	//   encoding and selection format information.
	// 
	class SelectionText {
	public:
		SelectionText();
		~SelectionText();
		void Clear();
		void Copy(const std::string &s_, int codePage_, int characterSet_, bool rectangular_, bool lineCopy_);
		void FASTCALL Copy(const SelectionText &other);
		const char * Data() const;
		size_t Length() const;
		size_t LengthWithTerminator() const;
		bool Empty() const;
		bool IsRectangular() const { return rectangular; }
		bool IsLineCopy() const { return lineCopy; }
		int  GetCp() const { return codePage; }
		int  GetCharSet() const { return characterSet; }
	private:
		void   FixSelectionForClipboard();

		bool   rectangular;
		bool   lineCopy;
		uint8  reserve[2]; // @alignment
		int    codePage;
		int    characterSet;
		std::string s;
	};

	struct WrapPending {
		WrapPending();
		void   Reset();
		void   FASTCALL Wrapped(int line);
		bool   NeedsWrap() const;
		bool   AddRange(int lineStart, int lineEnd);

		// The range of lines that need to be wrapped
		enum { 
			lineLarge = 0x7ffffff 
		};
		int    start;      // When there are wraps pending, will be in document range
		int    end;        // May be lineLarge to indicate all of document after start
	};
	// 
	// Descr: When platform has a way to generate an event before painting,
	//   accumulate needed styling range and other work items in
	//   WorkNeeded to avoid unnecessary work inside paint handler
	// 
	class WorkNeeded {
	public:
		enum workItems {
			workNone = 0,
			workStyle = 1,
			workUpdateUI = 2
		};
		enum workItems items;
		Position upTo;

		WorkNeeded();
		void Reset();
		void Need(workItems items_, Position pos);
	};
	//
	//
	//
	class Timer {
	public:
		Timer();

		TickerID tickerID;
		int    ticksToWait;
		enum {
			tickSize = 100
		};
		bool   ticking;
	};
	//
	//
	//
	class Idler {
	public:
		Idler();
		IdlerID idlerID;
		bool   state;
	};
	//
	// Descr: MarginView draws the margins.
	//
	class MarginView {
	public:
		MarginView();
		void DropGraphics(bool freeObjects);
		void AllocateGraphics(const ViewStyle &vsDraw);
		void RefreshPixMaps(SciSurface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);
		void PaintMargin(SciSurface *surface, int topLine, PRectangle rc, PRectangle rcMargin, const EditModel &model, const ViewStyle &vs);

		SciSurface * pixmapSelMargin;
		SciSurface * pixmapSelPattern;
		SciSurface * pixmapSelPatternOffset1;
		Document::HighlightDelimiter highlightDelimiter; // Highlight current folding block
		int    wrapMarkerPaddingRight; // right-most pixel padding of wrap markers
		// 
		// Descr: Some platforms, notably PLAT_CURSES, do not support Scintilla's native
		//   DrawWrapMarker function for drawing wrap markers. Allow those platforms to
		//   override it instead of creating a new method in the SciSurface class that
		//   existing platforms must implement as empty. 
		// 
		DrawWrapMarkerFn customDrawWrapMarker;
	};
	// On GTK+, Scintilla is a container widget holding two scroll bars
	// whereas on Windows there is just one window with both scroll bars turned on.
	SciWindow wMain;   ///< The Scintilla parent window
	SciWindow wMargin; ///< May be separate when using a scroll view for wMain
	ViewStyle vs;
	int    technology;
	SciPoint  sizeRGBAImage;
	float  scaleRGBAImage;
	MarginView marginView;
	EditView view;
	int    cursorMode;
	int    xCaretMargin;       ///< Ensure this many pixels visible on both sides of caret
	int    scrollWidth;
	int    caretSticky;
	int    marginOptions;
	int    multiPasteMode;
	int    virtualSpaceOptions;
	KeyMap kmap;
	Timer  timer;
	Timer  autoScrollTimer;
	enum {
		autoScrollDelay = 200
	};

	Idler  idler;
	SciPoint  lastClick;
	uint   lastClickTime;
	SciPoint  doubleClickCloseThreshold;
	int    dwellDelay;
	int    ticksToDwell;
	enum {
		selChar,
		selWord,
		selSubLine,
		selWholeLine
	} selectionType;

	SciPoint ptMouseLast;
	enum {
		ddNone,
		ddInitial,
		ddDragging
	} inDragDrop;

	SelectionPosition posDrop;
	int    hotSpotClickPos;
	int    lastXChosen;
	int    lineAnchorPos;
	int    originalAnchorPos;
	int    wordSelectAnchorStartPos;
	int    wordSelectAnchorEndPos;
	int    wordSelectInitialCaretPos;
	int    targetStart;
	int    targetEnd;
	int    searchFlags;
	int    topLine;
	int    posTopLine;
	int    lengthForEncode;
	int    needUpdateUI;
	enum {
		notPainting,
		painting,
		paintAbandoned
	} paintState;

	PRectangle rcPaint;
	WorkNeeded workNeeded;
	int    idleStyling;
	int    modEventMask;
	SelectionText drag;
	int    caretXPolicy;
	int    caretXSlop; ///< Ensure this many pixels visible on both sides of caret
	int    caretYPolicy;
	int    caretYSlop; ///< Ensure this many lines visible on both sides of caret
	int    visiblePolicy;
	int    visibleSlop;
	int    searchAnchor;
	int    foldAutomatic;
	WrapPending wrapPending; // Wrapping support
	//bool   stylesValid;
	//bool   hasFocus;
	//bool   mouseDownCaptures;
	//bool   mouseWheelCaptures;
	//bool   horizontalScrollBarVisible;
	//bool   verticalScrollBarVisible;
	//bool   endAtLastLine;
	//bool   mouseSelectionRectangularSwitch;
	//bool   multipleSelection;
	//bool   additionalSelectionTyping;
	//bool   dwelling;
	//bool   dropWentOutside;
	//bool   paintAbandonedByStyling;
	//bool   paintingAllText;
	//bool   willRedrawAll;
	//bool   needIdleStyling;
	//bool   recordingMacro;
	//bool   convertPastes;

	enum {
		fHasFocus                        = 0x00000001,
		fMouseDownCaptures               = 0x00000002,
		fMouseWheelCaptures              = 0x00000004,
		fHorizontalScrollBarVisible      = 0x00000008,
		fVerticalScrollBarVisible        = 0x00000010,
		fEndAtLastLine                   = 0x00000020,
		fMouseSelectionRectangularSwitch = 0x00000040,
		fMultipleSelection               = 0x00000080,
		fAdditionalSelectionTyping       = 0x00000100,
		fDwelling                        = 0x00000200,
		fDropWentOutside                 = 0x00000400,
		fPaintAbandonedByStyling         = 0x00000800,
		fPaintingAllText                 = 0x00001000,
		fWillRedrawAll                   = 0x00002000,
		fNeedIdleStyling                 = 0x00004000,
		fRecordingMacro                  = 0x00008000,
		fConvertPastes                   = 0x00010000,
		fStylesValid                     = 0x00020000 // @internal Style resources may be expensive to allocate 
			// so are cached between uses. When a style attribute is changed, this cache is flushed. 
	};
	uint32 Flags;
	enum TickReason { 
		tickCaret, 
		tickScroll, 
		tickWiden, 
		tickDwell, 
		tickPlatform 
	};

	static int     ModifierFlags(bool shift, bool ctrl, bool alt, bool meta = false, bool super = false);
	static const   char * FASTCALL StringFromEOLMode(int eolMode);
	static sptr_t  StringResult(sptr_t lParam, const char * val);
	static sptr_t  BytesResult(sptr_t lParam, const uchar * val, size_t len);

	Editor();
	virtual ~Editor();
	virtual void Initialise() = 0;
	virtual void Finalise();
	//
	// The top left visible point in main window coordinates. Will be 0,0 except for
	// scroll views where it will be equivalent to the current scroll position.
	//
	virtual SciPoint GetVisibleOriginInMain() const;
	virtual PRectangle GetClientRectangle() const;
	virtual PRectangle GetClientDrawingRectangle();
	virtual int LinesOnScreen() const;
	virtual bool AbandonPaint();
	virtual void RedrawRect(PRectangle rc);
	virtual void DiscardOverdraw();
	virtual void Redraw();
	virtual void ScrollText(int linesToMove);
	virtual void NotifyCaretMove();
	virtual void UpdateSystemCaret();
	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(int nMax, int nPage) = 0;
	virtual void ReconfigureScrollBars();
	virtual void AddCharUTF(const char * s, uint len, bool treatAsDBCS = false);
	virtual void Copy() = 0;
	virtual void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	virtual void ClaimSelection() = 0;
	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID() { return ctrlID; }
	virtual void NotifyParent(SCNotification & rScn) = 0; // @sobolev SCNotification-->SCNotification &
	virtual void NotifyStyleToNeeded(int endStyleNeeded);
	virtual void NotifyDoubleClick(SciPoint pt, int modifiers);
	virtual void NotifyDoubleClick(SciPoint pt, bool shift, bool ctrl, bool alt);
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	virtual void CancelModes();
	virtual int KeyCommand(uint iMessage);
	virtual int KeyDefault(int /* key */, int /*modifiers*/);
	virtual CaseFolder * CaseFolderForEncoding();
	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	virtual void DisplayCursor(SciWindow::Cursor c);
	virtual bool DragThreshold(SciPoint ptStart, SciPoint ptNow);
	virtual void StartDrag();
	virtual void ButtonDownWithModifiers(SciPoint pt, uint curTime, int modifiers);
	virtual void RightButtonDownWithModifiers(SciPoint pt, uint curTime, int modifiers);
	virtual void ButtonDown(SciPoint pt, uint curTime, bool shift, bool ctrl, bool alt);
	virtual void SetTicking(bool on);
	virtual void TickFor(TickReason reason);
	virtual bool FineTickerAvailable();
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool)
	{
		return false;
	}
	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	virtual void IdleWork();
	virtual void QueueIdleWork(WorkNeeded::workItems items, int upTo = 0);
	virtual bool PaintContains(PRectangle rc);
	virtual void SetDocPointer(Document * document);
	virtual bool ValidCodePage(int /* codePage */) const
	{
		return true;
	}
	virtual sptr_t DefWndProc(uint iMessage, uptr_t wParam, sptr_t lParam) = 0;

	void   InvalidateStyleData();
	void   InvalidateStyleRedraw();
	void   RefreshStyleData();
	void   SetRepresentations();
	void   DropGraphics(bool freeObjects);
	void   AllocateGraphics();
	PointDocument FASTCALL DocumentPointFromView(const SciPoint & rPtView) const;  // Convert a point from view space to document
	int    TopLineOfMain() const;   // Return the line at Main's y coordinate 0
	PRectangle GetTextRectangle() const;
	int    LinesToScroll() const;
	int    MaxScrollPos() const;
	SelectionPosition ClampPositionIntoDocument(SelectionPosition sp) const;
	SciPoint  LocationFromPosition(SelectionPosition pos, PointEnd pe = peDefault);
	SciPoint  LocationFromPosition(int pos, PointEnd pe = peDefault);
	int    XFromPosition(int pos);
	int    XFromPosition(SelectionPosition sp);
	SelectionPosition SPositionFromLocation(const SciPoint & rPt, bool canReturnInvalid = false, bool charPosition = false, bool virtualSpace = true);
	int    PositionFromLocation(const SciPoint & rPt, bool canReturnInvalid = false, bool charPosition = false);
	SelectionPosition SPositionFromLineX(int lineDoc, int x);
	int    PositionFromLineX(int line, int x);
	int    LineFromLocation(SciPoint pt) const;
	void   SetTopLine(int topLineNew);
	void   RedrawSelMargin(int line = -1, bool allAfter = false);
	PRectangle RectangleFromRange(Range r, int overlap);
	void   InvalidateRange(int start, int end);
	bool   UserVirtualSpace() const
	{
		return ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0);
	}
	int    CurrentPosition() const;
	bool   SelectionEmpty() const;
	SelectionPosition SelectionStart();
	SelectionPosition SelectionEnd();
	void   SetRectangularRange();
	void   ThinRectangularRange();
	void   InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection = false);
	void   InvalidateWholeSelection();
	void   SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_);
	void   SetSelection(int currentPos_, int anchor_);
	void   SetSelection(SelectionPosition currentPos_);
	void   SetSelection(int currentPos_);
	void   SetEmptySelection(SelectionPosition currentPos_);
	void   SetEmptySelection(int currentPos_);
	
	enum AddNumber { 
		addOne, 
		addEach 
	};

	void   MultipleSelectAdd(AddNumber addNumber);
	bool RangeContainsProtected(int start, int end) const;
	bool SelectionContainsProtected();
	int MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd = true) const;
	SelectionPosition MovePositionOutsideChar(SelectionPosition pos, int moveDir, bool checkLineEnd = true) const;
	void   MovedCaret(SelectionPosition newPos, SelectionPosition previousPos, bool ensureVisible);
	void   MovePositionTo(SelectionPosition newPos, Selection::selTypes selt = Selection::noSel, bool ensureVisible = true);
	void   MovePositionTo(int newPos, Selection::selTypes selt = Selection::noSel, bool ensureVisible = true);
	SelectionPosition MovePositionSoVisible(SelectionPosition pos, int moveDir);
	SelectionPosition MovePositionSoVisible(int pos, int moveDir);
	SciPoint PointMainCaret();
	void   SetLastXChosen();
	void   ScrollTo(int line, bool moveThumb = true);
	void   HorizontalScrollTo(int xPos);
	void   VerticalCentreCaret();
	void   MoveSelectedLines(int lineDelta);
	void   MoveSelectedLinesUp();
	void   MoveSelectedLinesDown();
	void   MoveCaretInsideView(bool ensureVisible = true);
	int    DisplayFromPosition(int pos);

	struct XYScrollPosition {
		XYScrollPosition(int xOffset_, int topLine_);
		bool   FASTCALL operator == (const XYScrollPosition & other) const;

		int    xOffset;
		int    topLine;
	};

	enum XYScrollOptions {
		xysUseMargin = 0x1,
		xysVertical = 0x2,
		xysHorizontal = 0x4,
		xysDefault = xysUseMargin|xysVertical|xysHorizontal
	};

	XYScrollPosition XYScrollToMakeVisible(const SelectionRange &range, const XYScrollOptions options);
	void   SetXYScroll(XYScrollPosition newXY);
	void   EnsureCaretVisible(bool useMargin = true, bool vert = true, bool horiz = true);
	void   ScrollRange(SelectionRange range);
	void   ShowCaretAtCurrentPosition();
	void   DropCaret();
	void   CaretSetPeriod(int period);
	void   InvalidateCaret();
	bool   Wrapping() const;
	void   NeedWrapping(int docLineStart = 0, int docLineEnd = WrapPending::lineLarge);
	bool   WrapOneLine(SciSurface * surface, int lineToWrap);
	enum wrapScope {
		wsAll, 
		wsVisible, 
		wsIdle
	};
	bool   WrapLines(enum wrapScope ws);
	void   LinesJoin();
	void   LinesSplit(int pixelWidth);
	void   PaintSelMargin(SciSurface * surface, const PRectangle & rc);
	void   RefreshPixMaps(SciSurface * surfaceWindow);
	void   Paint(SciSurface * surfaceWindow, PRectangle rcArea);
	long   FormatRange(bool draw, Sci_RangeToFormat * pfr);
	int    TextWidth(int style, const char * text);
	void   SetScrollBars();
	void   ChangeSize();
	void   FilterSelections();
	int    RealizeVirtualSpace(int position, uint virtualSpace);
	SelectionPosition RealizeVirtualSpace(const SelectionPosition &position);
	void   FASTCALL AddChar(char ch);
	void   ClearBeforeTentativeStart();
	void   InsertPaste(const char * text, int len);
	
	enum PasteShape { 
		pasteStream = 0, 
		pasteRectangular = 1, 
		pasteLine = 2 
	};

	void   InsertPasteShape(const char * text, int len, PasteShape shape);
	void   FASTCALL ClearSelection(bool retainMultipleSelections = false);
	void   ClearAll();
	void   ClearDocumentStyle();
	void   Cut();
	void   PasteRectangular(SelectionPosition pos, const char * ptr, int len);
	void   Clear();
	void   SelectAll();
	void   Undo();
	void   Redo();
	void   DelCharBack(bool allowLineStartDeletion);
	void   NotifyChar(int ch);
	void   NotifySavePoint(bool isSavePoint);
	void   NotifyModifyAttempt();
	void   NotifyHotSpotClicked(int position, int modifiers);
	void   NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt);
	void   NotifyHotSpotDoubleClicked(int position, int modifiers);
	void   NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt);
	void   NotifyHotSpotReleaseClick(int position, int modifiers);
	void   NotifyHotSpotReleaseClick(int position, bool shift, bool ctrl, bool alt);
	bool   NotifyUpdateUI();
	void   NotifyPainted();
	void   NotifyIndicatorClick(bool click, int position, int modifiers);
	void   NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt);
	bool   NotifyMarginClick(SciPoint pt, int modifiers);
	bool   NotifyMarginClick(SciPoint pt, bool shift, bool ctrl, bool alt);
	bool   NotifyMarginRightClick(SciPoint pt, int modifiers);
	void   NotifyNeedShown(int pos, int len);
	void   NotifyDwelling(SciPoint pt, bool state);
	void   NotifyZoom();
	void   NotifyModifyAttempt(Document * document, void * userData);
	void   NotifySavePoint(Document * document, void * userData, bool atSavePoint);
	void   FASTCALL CheckModificationForWrap(const DocModification & rMh);
	void   NotifyModified(Document * document, const DocModification & rMh, void * userData);
	void   NotifyDeleted(Document * document, void * userData);
	void   NotifyStyleNeeded(Document * doc, void * userData, int endPos);
	void   NotifyLexerChanged(Document * doc, void * userData);
	void   NotifyErrorOccurred(Document * doc, void * userData, int status);
	void   NotifyMacroRecord(uint iMessage, uptr_t wParam, sptr_t lParam);
	void   FASTCALL ContainerNeedsUpdate(int flags);
	void   PageMove(int direction, Selection::selTypes selt = Selection::noSel, bool stuttered = false);
	enum { 
		cmSame, 
		cmUpper, 
		cmLower 
	};
	void   ChangeCaseOfSelection(int caseMapping);
	void   LineTranspose();
	void   Duplicate(bool forLine);
	void   NewLine();
	SelectionPosition PositionUpOrDown(SelectionPosition spStart, int direction, int lastX);
	void   CursorUpOrDown(int direction, Selection::selTypes selt);
	void   ParaUpOrDown(int direction, Selection::selTypes selt);
	Range  RangeDisplayLine(int lineVisible);
	int    StartEndDisplayLine(int pos, bool start);
	int    VCHomeDisplayPosition(int position);
	int    VCHomeWrapPosition(int position);
	int    LineEndWrapPosition(int position);
	int    HorizontalMove(uint iMessage);
	int    DelWordOrLine(uint iMessage);
	int    KeyDownWithModifiers(int key, int modifiers, bool * consumed);
	int    KeyDown(int key, bool shift, bool ctrl, bool alt, bool * consumed = 0);
	void   Indent(bool forwards);
	long   FindText(uptr_t wParam, sptr_t lParam);
	void   SearchAnchor();
	long   SearchText(uint iMessage, uptr_t wParam, sptr_t lParam);
	long   SearchInTarget(const char * text, int length);
	void   GoToLine(int lineNo);
	std::string RangeText(int start, int end) const;
	void   CopySelectionRange(SelectionText * ss, bool allowLineCopy = false);
	void   CopyRangeToClipboard(int start, int end);
	void   CopyText(int length, const char * text);
	void   SetDragPosition(SelectionPosition newPos);
	void   DropAt(SelectionPosition position, const char * value, size_t lengthValue, bool moving, bool rectangular);
	void   DropAt(SelectionPosition position, const char * value, bool moving, bool rectangular);
	/** PositionInSelection returns true if position in selection. */
	bool   PositionInSelection(int pos);
	bool   PointInSelection(SciPoint pt);
	bool   PointInSelMargin(SciPoint pt) const;
	SciWindow::Cursor GetMarginCursor(SciPoint pt) const;
	void   TrimAndSetSelection(int currentPos_, int anchor_);
	void   LineSelection(int lineCurrentPos_, int lineAnchorPos_, bool wholeLine);
	void   WordSelection(int pos);
	void   DwellEnd(bool mouseMoved);
	void   MouseLeave();
	void   ButtonMoveWithModifiers(SciPoint pt, int modifiers);
	void   ButtonMove(SciPoint pt);
	void   ButtonUp(SciPoint pt, uint curTime, bool ctrl);
	void   Tick();
	bool   Idle();
	void   SetFocusState(bool focusState);
	int    PositionAfterArea(PRectangle rcArea) const;
	void   StyleToPositionInView(Position pos);
	int    PositionAfterMaxStyling(int posMax, bool scrolling) const;
	void   StartIdleStyling(bool truncatedLastStyling);
	void   StyleAreaBounded(PRectangle rcArea, bool scrolling);
	void   IdleStyling();
	bool   PaintContainsMargin();
	void   CheckForChangeOutsidePaint(Range r);
	void   SetBraceHighlight(Position pos0, Position pos1, int matchStyle);
	void   SetAnnotationHeights(int start, int end);
	void   SetAnnotationVisible(int visible);
	int    ExpandLine(int line);
	void   SetFoldExpanded(int lineDoc, bool expanded);
	void   FoldLine(int line, int action);
	void   FoldExpand(int line, int action, int level);
	int    ContractedFoldNext(int lineStart) const;
	void   EnsureLineVisible(int lineDoc, bool enforcePolicy);
	void   FoldChanged(int line, int levelNow, int levelPrev);
	void   NeedShown(int pos, int len);
	void   FoldAll(int action);
	int    GetTag(char * tagValue, int tagNumber);
	int    ReplaceTarget(bool replacePatterns, const char * text, int length = -1);
	bool   PositionIsHotspot(int position) const;
	bool   PointIsHotspot(SciPoint pt);
	void   SetHotSpotRange(const SciPoint * pt);
	Range  GetHotSpotRange() const;
	void   SetHoverIndicatorPosition(int position);
	void   SetHoverIndicatorPoint(SciPoint pt);
	int    CodePage() const;
	int    WrapCount(int line);
	void   AddStyledText(const char * buffer, int appendLength);
	bool   ValidMargin(uptr_t wParam) const;
	void   StyleSetMessage(uint iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t StyleGetMessage(uint iMessage, uptr_t wParam, sptr_t lParam);
	void   SetSelectionNMessage(uint iMessage, uptr_t wParam, sptr_t lParam);
private:
	// Private so Editor objects can not be copied
	explicit Editor(const Editor &);
	Editor & operator = (const Editor &);
};
/**
 * A smart pointer class to ensure Surfaces are set up and deleted correctly.
 */
class AutoSurface {
public:
	AutoSurface(Editor * ed, int technology = -1);
	AutoSurface(SurfaceID sid, Editor * ed, int technology = -1);
	~AutoSurface();
	SciSurface * operator->() const { return surf; }
	operator SciSurface *() const { return surf; }
private:
	SciSurface * surf;
};
//
//
//
class ScintillaBase : public Editor {
public:
	// Public so scintilla_send_message can use it
	virtual sptr_t WndProc(uint iMessage, uptr_t wParam, sptr_t lParam);
protected:
	static void AutoCompleteDoubleClick(void *p);
#ifdef SCI_LEXER
	LexState * DocumentLexState();
	void   SetLexer(uptr_t wParam);
	void   SetLexerLanguage(const char *languageName);
	void   Colourise(int start, int end);
#endif
	ScintillaBase();
	virtual ~ScintillaBase();
	virtual void Initialise() = 0;
	virtual void Finalise();
	virtual void AddCharUTF(const char *s, uint len, bool treatAsDBCS = false);
	virtual void CancelModes();
	virtual int  KeyCommand(uint iMessage);
	virtual void CreateCallTipWindow(PRectangle rc) = 0;
	virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) = 0;
	virtual void ButtonDownWithModifiers(SciPoint pt, uint curTime, int modifiers);
	virtual void ButtonDown(SciPoint pt, uint curTime, bool shift, bool ctrl, bool alt);
	virtual void RightButtonDownWithModifiers(SciPoint pt, uint curTime, int modifiers);

	void   FASTCALL Command(int cmdId);
	void   AutoCompleteInsert(Position startPos, int removeLen, const char *text, int textLen);
	void   AutoCompleteStart(int lenEntered, const char *list);
	void   AutoCompleteCancel();
	void   AutoCompleteMove(int delta);
	int    AutoCompleteGetCurrent() const;
	int    AutoCompleteGetCurrentText(char *buffer) const;
	void   AutoCompleteCharacterAdded(char ch);
	void   AutoCompleteCharacterDeleted();
	void   AutoCompleteCompleted(char ch, uint completionMethod);
	void   AutoCompleteMoveToCurrentWord();
	void   CallTipClick();
	void   CallTipShow(SciPoint pt, const char *defn);
	bool   ShouldDisplayPopup(SciPoint ptInWindowCoordinates) const;
	void   ContextMenu(SciPoint pt);
	void   NotifyStyleToNeeded(int endStyleNeeded);
	void   NotifyLexerChanged(Document *doc, void *userData);
	//
	// Descr: Enumeration of commands and child windows. 
	//
	enum {
		idCallTip=1,
		idAutoComplete=2,

		idcmdUndo=10,
		idcmdRedo=11,
		idcmdCut=12,
		idcmdCopy=13,
		idcmdPaste=14,
		idcmdDelete=15,
		idcmdSelectAll=16
	};
	enum { 
		maxLenInputIME = 200 
	};

	SciMenu popup;
	AutoComplete ac;
	CallTip ct;
	int displayPopupMenu;
	int listType;			///< 0 is an autocomplete list
	int maxListWidth;		/// Maximum width of list, in average character widths
	int multiAutoCMode; /// Mode for autocompleting when multiple selections are present
private:
	// Private so ScintillaBase objects can not be copied
	explicit ScintillaBase(const ScintillaBase &);
	ScintillaBase & operator = (const ScintillaBase &);
};
//
// Descr: Base class for templated SparseVector
//
class SparseVectorBase {
protected:
	SparseVectorBase();
	~SparseVectorBase();
	int Length() const;
	int Elements() const;
	int PositionOfElement(int element) const;
protected:
	Partitioning * starts;
};
//
// SparseVector is similar to RunStyles but is more efficient for cases where values occur
// for one position instead of over a range of positions.
//
template <typename T> class SparseVector : private SparseVectorBase {
public:
	SparseVector() : SparseVectorBase()
	{
		//starts = new Partitioning(8);
		values = new SplitVector<T>();
		values->InsertValue(0, 2, T());
	}
	~SparseVector()
	{
		//ZDELETE(starts);
		// starts dead here but not used by ClearValue.
		for(int part = 0; part < values->Length(); part++) {
			ClearValue(part);
		}
		ZDELETE(values);
	}
	/*
	int Length() const
	{
		return starts->PositionFromPartition(starts->Partitions());
	}
	int Elements() const
	{
		return starts->Partitions();
	}
	int PositionOfElement(int element) const
	{
		return starts->PositionFromPartition(element);
	}
	*/
	T ValueAt(int position) const
	{
		assert(position < Length());
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		return (startPartition == position) ? values->ValueAt(partition) : T();
	}
	void SetValueAt(int position, T value)
	{
		CommonSetValueAt(position, value);
	}
	void InsertSpace(int position, int insertLength)
	{
		assert(position <= Length());   // Only operation that works at end.
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(startPartition == position) {
			T valueCurrent = values->ValueAt(partition);
			// Inserting at start of run so make previous longer
			if(partition == 0) {
				// Inserting at start of document so ensure 0
				if(valueCurrent != T()) {
					ClearValue(0);
					starts->InsertPartition(1, 0);
					values->InsertValue(1, 1, valueCurrent);
					starts->InsertText(0, insertLength);
				}
				else
					starts->InsertText(partition, insertLength);
			}
			else {
				if(valueCurrent != T())
					starts->InsertText(partition - 1, insertLength);
				else // Insert at end of run so do not extend style
					starts->InsertText(partition, insertLength);
			}
		}
		else
			starts->InsertText(partition, insertLength);
	}
	void DeletePosition(int position)
	{
		assert(position < Length());
		int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(startPartition == position) {
			if(partition == 0) {
				ClearValue(0);
			}
			else if(partition == starts->Partitions()) {
				// This should not be possible
				ClearValue(partition);
				throw std::runtime_error("SparseVector: deleting end partition.");
			}
			else {
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
				// Its the previous partition now that gets smaller
				partition--;
			}
		}
		starts->InsertText(partition, -1);
	}
	void Check() const
	{
		if(Length() < 0) {
			throw std::runtime_error("SparseVector: Length can not be negative.");
		}
		if(starts->Partitions() < 1) {
			throw std::runtime_error("SparseVector: Must always have 1 or more partitions.");
		}
		if(starts->Partitions() != values->Length() - 1) {
			throw std::runtime_error("SparseVector: Partitions and values different lengths.");
		}
		// The final element can not be set
		if(values->ValueAt(values->Length() - 1) != T()) {
			throw std::runtime_error("SparseVector: Unused style at end changed.");
		}
	}
private:
	// Private so SparseVector objects can not be copied
	SparseVector(const SparseVector &);
	void ClearValue(int partition)
	{
		values->SetValueAt(partition, T());
	}
	void CommonSetValueAt(int position, T value)
	{
		// Do the work of setting the value to allow for specialization of SetValueAt.
		assert(position < Length());
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(value == T()) {
			// Setting the empty value is equivalent to deleting the position
			if(position == 0) {
				ClearValue(partition);
			}
			else if(position == startPartition) {
				// Currently an element at this position, so remove
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
			}
			// Else element remains empty
		}
		else {
			if(position == startPartition) { // Already a value at this position, so replace
				ClearValue(partition);
				values->SetValueAt(partition, value);
			}
			else { // Insert a new element
				starts->InsertPartition(partition + 1, position);
				values->InsertValue(partition + 1, 1, value);
			}
		}
	}
	//Partitioning * starts;
	SplitVector<T> * values;
};

// The specialization for const char * makes copies and deletes them as needed.

template <> inline void SparseVector<const char *>::ClearValue(int partition)
{
	const char * value = values->ValueAt(partition);
	delete []value;
	values->SetValueAt(partition, 0);
}

template <> inline void SparseVector<const char *>::SetValueAt(int position, const char * value)
{
	// Make a copy of the string
	if(value) {
		const size_t len = sstrlen(value);
		char * valueCopy = new char[len + 1]();
		std::copy(value, value + len, valueCopy);
		CommonSetValueAt(position, valueCopy);
	}
	else {
		CommonSetValueAt(position, 0);
	}
}
// 
// The following defines are not meant to be changeable. They are for readability only.
// 
#define MAXCHR	256
#define CHRBIT	8
#define BITBLK	MAXCHR/CHRBIT

class CharacterIndexer {
public:
	virtual char CharAt(int index) = 0;
	virtual ~CharacterIndexer() 
	{
	}
};

class RESearch {
public:
	explicit RESearch(CharClassify *charClassTable);
	~RESearch();
	void Clear();
	void GrabMatches(CharacterIndexer &ci);
	const char *Compile(const char *pattern, int length, bool caseSensitive, bool posix);
	int Execute(CharacterIndexer &ci, int lp, int endp);

	enum { MAXTAG=10 };
	enum { MAXNFA=4096 };
	enum { NOTFOUND=-1 };

	int bopat[MAXTAG];
	int eopat[MAXTAG];
	std::string pat[MAXTAG];
private:
	void FASTCALL ChSet(uchar c);
	void ChSetWithCase(uchar c, bool caseSensitive);
	int GetBackslashExpression(const char *pattern, int &incr);
	int PMatch(CharacterIndexer &ci, int lp, int endp, char *ap);
	bool iswordc(uchar x) const { return charClass->IsWord(x); }

	int bol;
	int tagstk[MAXTAG]; /* subpat tag stack */
	char nfa[MAXNFA]; /* automaton */
	int sta;
	uchar bittab[BITBLK]; /* bit table for CCL pre-set bits */
	int failure;
	CharClassify *charClass;
};
//
//
//
typedef void *(EXT_LEXER_DECL *GetLexerFunction)(uint Index);
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(uint Index, char *name, int buflength);
typedef LexerFactoryFunction(EXT_LEXER_DECL *GetLexerFactoryFunction)(uint Index);

/// Sub-class of LexerModule to use an external lexer.
class ExternalLexerModule : public LexerModule {
public:
	ExternalLexerModule(int language_, LexerFunction fnLexer_, const char *languageName_=0, LexerFunction fnFolder_=0);
	virtual void SetExternal(GetLexerFactoryFunction fFactory, int index);
protected:
	GetLexerFactoryFunction fneFactory;
	std::string name;
};

/// LexerMinder points to an ExternalLexerModule - so we don't leak them.
class LexerMinder {
public:
	ExternalLexerModule * self;
	LexerMinder * next;
};

/// LexerLibrary exists for every External Lexer DLL, contains LexerMinders.
class LexerLibrary {
public:
	explicit LexerLibrary(const char *ModuleName);
	~LexerLibrary();
	void Release();

	LexerLibrary * next;
	std::string m_sModuleName;
private:
	DynamicLibrary	*lib;
	LexerMinder		*first;
	LexerMinder		*last;
};

/// LexerManager manages external lexers, contains LexerLibrarys.
class LexerManager {
public:
	~LexerManager();
	static LexerManager *GetInstance();
	static void DeleteInstance();
	void Load(const char *path);
	void Clear();
private:
	LexerManager();
	static LexerManager *theInstance;
	void LoadLexerLibrary(const char *module);
	LexerLibrary *first;
	LexerLibrary *last;
};

class LMMinder {
public:
	~LMMinder();
};
//
//#include "OptionSet.h"
//
class _OptionSetBase { // @sobolev
public:
	const char * PropertyNames() const;
	const char * DescribeWordListSets() const;
	void DefineWordListSets(const char * const wordListDescriptions[]);
protected:
	void AppendName(const char * name);
	std::string names;
	std::string wordLists;
};

template <typename T> class OptionSet : public _OptionSetBase { // @sobolev(public _OptionSetBase)
	typedef T Target;
	typedef bool T::* plcob;
	typedef int T::* plcoi;
	typedef std::string T::* plcos;
	struct Option {
		int opType;
		union {
			plcob pb;
			plcoi pi;
			plcos ps;
		};

		std::string description;
		Option() : opType(SC_TYPE_BOOLEAN), pb(0), description("")
		{
		}
		Option(plcob pb_, std::string description_ = "") : opType(SC_TYPE_BOOLEAN), pb(pb_), description(description_)
		{
		}
		Option(plcoi pi_, std::string description_) : opType(SC_TYPE_INTEGER), pi(pi_), description(description_)
		{
		}
		Option(plcos ps_, std::string description_) : opType(SC_TYPE_STRING), ps(ps_), description(description_)
		{
		}
		bool Set(T * base, const char * val) const
		{
			switch(opType) {
				case SC_TYPE_BOOLEAN: {
				    bool option = atoi(val) != 0;
				    if((*base).*pb != option) {
					    (*base).*pb = option;
					    return true;
				    }
				    break;
			    }
				case SC_TYPE_INTEGER: {
				    int option = atoi(val);
				    if((*base).*pi != option) {
					    (*base).*pi = option;
					    return true;
				    }
				    break;
			    }
				case SC_TYPE_STRING: {
				    if((*base).*ps != val) {
					    (*base).*ps = val;
					    return true;
				    }
				    break;
			    }
			}
			return false;
		}
	};

	typedef std::map<std::string, Option> OptionMap;
	OptionMap nameToDef;
public:
	virtual ~OptionSet()
	{
	}
	void DefineProperty(const char * name, plcob pb, std::string description = "")
	{
		nameToDef[name] = Option(pb, description);
		AppendName(name);
	}
	void DefineProperty(const char * name, plcoi pi, std::string description = "")
	{
		nameToDef[name] = Option(pi, description);
		AppendName(name);
	}
	void DefineProperty(const char * name, plcos ps, std::string description = "")
	{
		nameToDef[name] = Option(ps, description);
		AppendName(name);
	}
	int PropertyType(const char * name)
	{
		typename OptionMap::iterator it = nameToDef.find(name);
		return (it != nameToDef.end()) ? it->second.opType : SC_TYPE_BOOLEAN;
	}
	const char * DescribeProperty(const char * name)
	{
		typename OptionMap::iterator it = nameToDef.find(name);
		return (it != nameToDef.end()) ? it->second.description.c_str() : "";
	}
	bool PropertySet(T * base, const char * name, const char * val)
	{
		typename OptionMap::iterator it = nameToDef.find(name);
		return (it != nameToDef.end()) ? it->second.Set(base, val) : false;
	}
};
//
//#include "SparseState.h"
//
template <typename T> class SparseState {
	struct State {
		State(int position_, T value_) : position(position_), value(value_) 
		{
		}
		inline bool operator<(const State &other) const 
		{
			return position < other.position;
		}
		inline bool operator == (const State &other) const 
		{
			return (position == other.position) && (value == other.value);
		}
		int    position;
		T      value;
	};
	int positionFirst;
	typedef std::vector<State> stateVector;
	stateVector states;

	typename stateVector::iterator Find(int position)
	{
		State searchValue(position, T());
		return std::lower_bound(states.begin(), states.end(), searchValue);
	}
public:
	explicit SparseState(int positionFirst_ = -1)
	{
		positionFirst = positionFirst_;
	}
	void Set(int position, T value)
	{
		Delete(position);
		if(states.empty() || (value != states[states.size()-1].value)) {
			states.push_back(State(position, value));
		}
	}
	T ValueAt(int position)
	{
		if(states.empty())
			return T();
		if(position < states[0].position)
			return T();
		typename stateVector::iterator low = Find(position);
		if(low == states.end()) {
			return states[states.size()-1].value;
		}
		else {
			if(low->position > position) {
				--low;
			}
			return low->value;
		}
	}
	bool Delete(int position) 
	{
		typename stateVector::iterator low = Find(position);
		if(low != states.end()) {
			states.erase(low, states.end());
			return true;
		}
		else
			return false;
	}
	size_t size() const 
	{
		return states.size();
	}
	// Returns true if Merge caused a significant change
	bool Merge(const SparseState<T> &other, int ignoreAfter) 
	{
		// Changes caused beyond ignoreAfter are not significant
		Delete(ignoreAfter+1);
		bool different = true;
		bool changed = false;
		typename stateVector::iterator low = Find(other.positionFirst);
		if(static_cast<size_t>(states.end() - low) == other.states.size()) {
			// Same number in other as after positionFirst in this
			different = !std::equal(low, states.end(), other.states.begin());
		}
		if(different) {
			if(low != states.end()) {
				states.erase(low, states.end());
				changed = true;
			}
			typename stateVector::const_iterator startOther = other.states.begin();
			if(!states.empty() && !other.states.empty() && states.back().value == startOther->value)
				++startOther;
			if(startOther != other.states.end()) {
				states.insert(states.end(), startOther, other.states.end());
				changed = true;
			}
		}
		return changed;
	}
};
//
//#include "SubStyles.h"
//
class WordClassifier {
public:
	explicit WordClassifier(int baseStyle_);
	void Allocate(int firstStyle_, int lenStyles_);
	int Base() const;
	int Start() const;
	int Length() const;
	void Clear();
	int ValueFor(const std::string &s) const;
	bool IncludesStyle(int style) const;
	void SetIdentifiers(int style, const char * identifiers);
private:
	int    baseStyle;
	int    firstStyle;
	int    lenStyles;
	std::map <std::string, int> wordToStyle;
};

class SubStyles {
public:
	SubStyles(const char * baseStyles_, int styleFirst_, int stylesAvailable_, int secondaryDistance_);
	int    Allocate(int styleBase, int numberStyles);
	int    Start(int styleBase);
	int    Length(int styleBase);
	int    BaseStyle(int subStyle) const;
	int    DistanceToSecondaryStyles() const;
	void   SetIdentifiers(int style, const char * identifiers);
	void   Free();
	const  WordClassifier &Classifier(int baseStyle) const;
private:
	int    BlockFromBaseStyle(int baseStyle) const;
	int    BlockFromStyle(int style) const;

	int    classifications;
	const  char * baseStyles;
	int    styleFirst;
	int    stylesAvailable;
	int    secondaryDistance;
	int    allocated;
	std::vector <WordClassifier> classifiers;
};
//
// HanjaDic.h
//
namespace HanjaDict {
	int GetHangulOfHanja(wchar_t *inout);
}

#ifdef SCI_NAMESPACE
}
#endif
//
#endif // __SCINTILLA_INTERNAL_H