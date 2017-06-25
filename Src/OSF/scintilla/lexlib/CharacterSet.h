// Scintilla source code edit control
/** @file CharacterSet.h
** Encapsulates a set of characters. Used to test if a character is within a set.
**/
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARACTERSET_H
#define CHARACTERSET_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class CharacterSet {
private:
	int    size;
	bool   valueAfter;
	bool * bset;
public:
	enum setBase {
		setNone = 0,
		setLower = 1,
		setUpper = 2,
		setDigits = 4,
		setAlpha = setLower|setUpper,
		setAlphaNum = setAlpha|setDigits
	};
	CharacterSet(setBase base = setNone, const char * initialSet = "", int size_ = 0x80, bool valueAfter_ = false);
	CharacterSet(const CharacterSet &other);
	~CharacterSet();
	CharacterSet & FASTCALL operator = (const CharacterSet &other);
	void FASTCALL Add(int val);
	void FASTCALL AddString(const char * setToAdd);
	bool FASTCALL Contains(int val) const;
};

// Functions for classifying characters

inline bool IsASpace(int ch)
{
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

inline bool IsASpaceOrTab(int ch)
{
	return (ch == ' ') || (ch == '\t');
}

inline bool IsADigit(int ch)
{
	return (ch >= '0') && (ch <= '9');
}

inline bool IsADigit(int ch, int base)
{
	if(base <= 10) {
		return (ch >= '0') && (ch < '0' + base);
	}
	else {
		return ((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch < 'A' + base - 10)) || ((ch >= 'a') && (ch < 'a' + base - 10));
	}
}

inline bool IsASCII(int ch)
{
	return (ch >= 0) && (ch < 0x80);
}

inline bool IsLowerCase(int ch)
{
	return (ch >= 'a') && (ch <= 'z');
}

inline bool IsUpperCase(int ch)
{
	return (ch >= 'A') && (ch <= 'Z');
}

inline bool IsAlphaNumeric(int ch)
{
	return ((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'));
}

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
inline bool isspacechar(int ch)
{
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

inline bool iswordchar(int ch)
{
	return IsAlphaNumeric(ch) || ch == '.' || ch == '_';
}

inline bool iswordstart(int ch)
{
	return IsAlphaNumeric(ch) || ch == '_';
}

inline bool isoperator(int ch)
{
	if(IsAlphaNumeric(ch))
		return false;
	else if(oneof8(ch, '%', '^', '&', '*',  '(', ')', '-', '+') || oneof8(ch, '=', '|', '{', '}', '[', ']', ':', ';') || oneof8(ch, '<', '>', ',', '/', '?', '!', '.', '~'))
		return true;
	else
		return false;
}

// Simple case functions for ASCII.

inline int MakeUpperCase(int ch)
{
	return (ch < 'a' || ch > 'z') ? ch : static_cast<char>(ch - 'a' + 'A');
}

inline int MakeLowerCase(int ch)
{
	return (ch < 'A' || ch > 'Z') ? ch : (ch - 'A' + 'a');
}

int CompareCaseInsensitive(const char * a, const char * b);
int CompareNCaseInsensitive(const char * a, const char * b, size_t len);

#ifdef SCI_NAMESPACE
}
#endif

#endif
