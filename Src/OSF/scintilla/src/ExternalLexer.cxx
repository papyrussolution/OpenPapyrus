// Scintilla source code edit control
/** @file ExternalLexer.cxx
** Support external lexers in DLLs.
**/
// Copyright 2001 Simon Steele <ss@pnotepad.org>, portions copyright Neil Hodgson.
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

LexerManager * LexerManager::theInstance = NULL;
//
// ExternalLexerModule
//
ExternalLexerModule::ExternalLexerModule(int language_, LexerFunction fnLexer_, const char *languageName_, LexerFunction fnFolder_) :
	LexerModule(language_, fnLexer_, 0, fnFolder_), fneFactory(0), name(languageName_)
{
	languageName = name.c_str();
}

void ExternalLexerModule::SetExternal(GetLexerFactoryFunction fFactory, int index)
{
	fneFactory = fFactory;
	fnFactory = fFactory(index);
}
//
// LexerLibrary
//
LexerLibrary::LexerLibrary(const char * ModuleName) : first(0), last(0), lib(DynamicLibrary::Load(ModuleName))
{
	if(lib && lib->IsValid()) {
		m_sModuleName = ModuleName;
		//Cannot use reinterpret_cast because: ANSI C++ forbids casting between pointers to functions and objects
		GetLexerCountFn GetLexerCount = (GetLexerCountFn)(sptr_t)lib->FindFunction("GetLexerCount");
		if(GetLexerCount) {
			ExternalLexerModule * lex;
			LexerMinder * lm;
			// Find functions in the DLL
			GetLexerNameFn GetLexerName = (GetLexerNameFn)(sptr_t)lib->FindFunction("GetLexerName");
			GetLexerFactoryFunction fnFactory = (GetLexerFactoryFunction)(sptr_t)lib->FindFunction("GetLexerFactory");
			int nl = GetLexerCount();
			for(int i = 0; i < nl; i++) {
				// Assign a buffer for the lexer name.
				char lexname[100] = "";
				GetLexerName(i, lexname, sizeof(lexname));
				lex = new ExternalLexerModule(SCLEX_AUTOMATIC, NULL, lexname, 0);
				Catalogue::AddLexerModule(lex);
				// Create a LexerMinder so we don't leak the ExternalLexerModule...
				lm = new LexerMinder;
				lm->self = lex;
				lm->next = NULL;
				if(first) {
					last->next = lm;
					last = lm;
				}
				else {
					first = lm;
					last = lm;
				}
				// The external lexer needs to know how to call into its DLL to
				// do its lexing and folding, we tell it here.
				lex->SetExternal(fnFactory, i);
			}
		}
	}
	next = NULL;
}

LexerLibrary::~LexerLibrary()
{
	Release();
	delete lib;
}

void LexerLibrary::Release()
{
	LexerMinder * lmNext;
	for(LexerMinder * lm = first; lm; lm = lmNext) {
		lmNext = lm->next;
		delete lm->self;
		delete lm;
	}
	first = NULL;
	last = NULL;
}
//
// LexerManager
//
/// Return the single LexerManager instance...
LexerManager * LexerManager::GetInstance()
{
	SETIFZ(theInstance, new LexerManager);
	return theInstance;
}

/// Delete any LexerManager instance...
void LexerManager::DeleteInstance()
{
	ZDELETE(theInstance);
}

/// protected constructor - this is a singleton...
LexerManager::LexerManager() : first(0), last(0)
{
}

LexerManager::~LexerManager()
{
	Clear();
}

void LexerManager::Load(const char * path)
{
	LoadLexerLibrary(path);
}

void LexerManager::LoadLexerLibrary(const char * module)
{
	for(LexerLibrary * ll = first; ll; ll = ll->next) {
		if(strcmp(ll->m_sModuleName.c_str(), module) == 0)
			return;
	}
	LexerLibrary * lib = new LexerLibrary(module);
	if(first) {
		last->next = lib;
		last = lib;
	}
	else {
		first = lib;
		last = lib;
	}
}

void LexerManager::Clear()
{
	if(first) {
		LexerLibrary * next;
		for(LexerLibrary * cur = first; cur; cur = next) {
			next = cur->next;
			delete cur;
		}
		first = NULL;
		last = NULL;
	}
}
//
// LexerManager
//
LMMinder::~LMMinder()
{
	LexerManager::DeleteInstance();
}

LMMinder minder;
