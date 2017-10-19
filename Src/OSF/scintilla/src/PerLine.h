// Scintilla source code edit control
/** @file PerLine.h
 ** Manages data associated with each line of the document
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PERLINE_H
#define PERLINE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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
	SplitVector<int> levels;
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

	bool MultipleStyles(int line) const;
	int Style(int line) const;
	const char *Text(int line) const;
	const uchar *Styles(int line) const;
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

#ifdef SCI_NAMESPACE
}
#endif

#endif
