// Scintilla source code edit control
/** @file CharClassify.h
 ** Character classifications used by Document and RESearch.
 **/
// Copyright 2006-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHARCLASSIFY_H
#define CHARCLASSIFY_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class CharClassify {
public:
	CharClassify();

	enum cc { 
		ccSpace, 
		ccNewLine, 
		ccWord, 
		ccPunctuation 
	};
	void SetDefaultCharClasses(bool includeWordClass);
	void SetCharClasses(const uchar *chars, cc newCharClass);
	int GetCharsOfClass(cc charClass, uchar *buffer) const;
	cc GetClass(uchar ch) const { return static_cast<cc>(charClass[ch]);}
	bool IsWord(uchar ch) const { return static_cast<cc>(charClass[ch]) == ccWord;}
private:
	enum { maxChar=256 };
	uchar charClass[maxChar];    // not type cc to save space
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
