// Scintilla source code edit control
/** @file LexerModule.cxx
** Colourise for particular languages.
**/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

LexerModule::LexerModule(int language_, LexerFunction fnLexer_, const char * languageName_, LexerFunction fnFolder_, const char * const wordListDescriptions_[]) :
	language(language_), fnLexer(fnLexer_), fnFolder(fnFolder_), fnFactory(0), wordListDescriptions(wordListDescriptions_), languageName(languageName_) 
{
}

LexerModule::LexerModule(int language_, LexerFactoryFunction fnFactory_, const char * languageName_, const char * const wordListDescriptions_[]) :
	language(language_), fnLexer(0), fnFolder(0), fnFactory(fnFactory_), wordListDescriptions(wordListDescriptions_), languageName(languageName_) 
{
}

LexerModule::~LexerModule() 
{
}

int LexerModule::GetNumWordLists() const 
{
	int    num_word_lists = -1;
	if(wordListDescriptions) {
		for(num_word_lists = 0; wordListDescriptions[num_word_lists];)
			++num_word_lists;
	}
	return num_word_lists;
}

const char * LexerModule::GetWordListDescription(int index) const 
{
	assert(index < GetNumWordLists());
	return (!wordListDescriptions || (index >= GetNumWordLists())) ? "" : wordListDescriptions[index];
}

ILexer * LexerModule::Create() const 
{
	return fnFactory ? fnFactory() : new LexerSimple(this);
}

void LexerModule::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, WordList * keywordlists[], Accessor &styler) const 
{
	if(fnLexer)
		fnLexer(startPos, lengthDoc, initStyle, keywordlists, styler);
}

void LexerModule::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, WordList * keywordlists[], Accessor &styler) const 
{
	if(fnFolder) {
		Sci_Position lineCurrent = styler.GetLine(startPos);
		// Move back one line in case deletion wrecked current line fold state
		if(lineCurrent > 0) {
			lineCurrent--;
			Sci_Position newStartPos = styler.LineStart(lineCurrent);
			lengthDoc += startPos - newStartPos;
			startPos = newStartPos;
			initStyle = 0;
			if(startPos > 0) {
				initStyle = styler.StyleAt(startPos - 1);
			}
		}
		fnFolder(startPos, lengthDoc, initStyle, keywordlists, styler);
	}
}
//
//
//
LexerBase::LexerBase() 
{
	for(int wl = 0; wl < numWordLists; wl++)
		keyWordLists[wl] = new WordList;
	keyWordLists[numWordLists] = 0;
}

LexerBase::~LexerBase() 
{
	for(int wl = 0; wl < numWordLists; wl++) {
		delete keyWordLists[wl];
		keyWordLists[wl] = 0;
	}
	keyWordLists[numWordLists] = 0;
}

void   SCI_METHOD LexerBase::Release() { delete this; }
int    SCI_METHOD LexerBase::Version() const { return lvOriginal; }
const  char * SCI_METHOD LexerBase::PropertyNames() { return ""; }
int    SCI_METHOD LexerBase::PropertyType(const char *) { return SC_TYPE_BOOLEAN; }
const  char * SCI_METHOD LexerBase::DescribeProperty(const char *) { return ""; }
const  char * SCI_METHOD LexerBase::DescribeWordListSets() { return ""; }
void * SCI_METHOD LexerBase::PrivateCall(int, void *) { return 0; }

Sci_Position SCI_METHOD LexerBase::PropertySet(const char * key, const char * val) 
{
	const char * valOld = props.Get(key);
	if(strcmp(val, valOld) != 0) {
		props.Set(key, val);
		return 0;
	}
	else
		return -1;
}

Sci_Position SCI_METHOD LexerBase::WordListSet(int n, const char * wl) 
{
	if(n < numWordLists) {
		WordList wlNew;
		wlNew.Set(wl);
		if(*keyWordLists[n] != wlNew) {
			keyWordLists[n]->Set(wl);
			return 0;
		}
	}
	return -1;
}
//
//
//
Sci_Position SCI_METHOD LexerNoExceptions::PropertySet(const char * key, const char * val)
{
	try {
		return LexerBase::PropertySet(key, val);
	} catch(...) {
		// Should not throw into caller as may be compiled with different compiler or options
	}
	return -1;
}

Sci_Position SCI_METHOD LexerNoExceptions::WordListSet(int n, const char * wl)
{
	try {
		return LexerBase::WordListSet(n, wl);
	} catch(...) {
		// Should not throw into caller as may be compiled with different compiler or options
	}
	return -1;
}

void SCI_METHOD LexerNoExceptions::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument * pAccess)
{
	try {
		Accessor astyler(pAccess, &props);
		Lexer(startPos, length, initStyle, pAccess, astyler);
		astyler.Flush();
	} catch(...) {
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

void SCI_METHOD LexerNoExceptions::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument * pAccess)
{
	try {
		Accessor astyler(pAccess, &props);
		Folder(startPos, length, initStyle, pAccess, astyler);
		astyler.Flush();
	} catch(...) {
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}
//
//
//
LexerSimple::LexerSimple(const LexerModule * module_) : module(module_)
{
	for(int wl = 0; wl < module->GetNumWordLists(); wl++) {
		/*
		if(!wordLists.empty())
			wordLists += "\n";
		wordLists += module->GetWordListDescription(wl);
		*/
		if(WordList.NotEmpty())
			WordList.CR();
		WordList.Cat(module->GetWordListDescription(wl));
	}
}

const char * SCI_METHOD LexerSimple::DescribeWordListSets() 
{
	//return wordLists.c_str();
	return WordList.cptr();
}

void SCI_METHOD LexerSimple::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument * pAccess) 
{
	Accessor astyler(pAccess, &props);
	module->Lex(startPos, lengthDoc, initStyle, keyWordLists, astyler);
	astyler.Flush();
}

void SCI_METHOD LexerSimple::Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument * pAccess) 
{
	if(props.GetInt("fold")) {
		Accessor astyler(pAccess, &props);
		module->Fold(startPos, lengthDoc, initStyle, keyWordLists, astyler);
		astyler.Flush();
	}
}

