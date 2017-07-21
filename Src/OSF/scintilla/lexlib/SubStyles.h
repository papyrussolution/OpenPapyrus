// Scintilla source code edit control
/** @file SubStyles.h
** Manage substyles for a lexer.
**/
// Copyright 2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SUBSTYLES_H
#define SUBSTYLES_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class WordClassifier {
private:
	int baseStyle;
	int firstStyle;
	int lenStyles;
	std::map <std::string, int> wordToStyle;
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
};

class SubStyles {
private:
	int classifications;
	const char * baseStyles;
	int styleFirst;
	int stylesAvailable;
	int secondaryDistance;
	int allocated;
	std::vector <WordClassifier> classifiers;

	int BlockFromBaseStyle(int baseStyle) const;
	int BlockFromStyle(int style) const;
public:
	SubStyles(const char * baseStyles_, int styleFirst_, int stylesAvailable_, int secondaryDistance_);
	int Allocate(int styleBase, int numberStyles);
	int Start(int styleBase);
	int Length(int styleBase);
	int BaseStyle(int subStyle) const;
	int DistanceToSecondaryStyles() const;
	void SetIdentifiers(int style, const char * identifiers);
	void Free();
	const WordClassifier &Classifier(int baseStyle) const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
