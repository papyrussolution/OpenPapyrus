// STRSTORE.H
// Copyright (c) A.Sobolev 1996-1999, 2005, 2007, 2016
// @codepage windows-1251
//
#ifndef __STRSTORE_H
#define __STRSTORE_H

#include <stdio.h>
#include <slib.h>

#if 0 // @v9.0.2 {

class StringStore : public SArray {
public:
	SLAPI  StringStore(const char * fname, int for_writing = 0);
	SLAPI ~StringStore();
	int    SLAPI open(const char *, int = 0);
	int    SLAPI close();
	int    SLAPI addString(int group, int code, Pchar string);
	int    SLAPI getString(int group, int code, SString & s);
	int    SLAPI getString(int group, int code, Pchar buf, size_t bufLen);
		// @>>StringStore::getString(int, int, SString &)
	int    error;
protected:
	struct IdxItem {
		int16  group;
		int16  code;
		uint16 offs;
	};
	struct Header {
		int32  sign;
		uint16 offs;
		uint16 count;
		char   pad[8]; // Поле, выравнивающее структуру по границе параграфа
	};
	FILE * file;
	int    writing;
	uint   last;
};

#endif // } 0 @v9.0.2
//
//
//
// @v9.0.2 struct StringStoreItem;

class StringStore2 {
public:
	SLAPI  StringStore2();
	SLAPI ~StringStore2();
	int    SLAPI SetActiveLanguage(int slang);
	int    SLAPI Load(const char * pFileName, int selfTest);
	int    SLAPI GetString(int group, int code, SString & rBuf) const;
	int    SLAPI GetString(const char * pSignature, SString & rBuf) const;
	int    SLAPI GetDescription(int group, int code, SString & rBuf); // @nonconst @cs
	int    SLAPI GetDescription(const char * pSignature, SString & rBuf); // @nonconst @cs
	int    SLAPI ExpandString(SString & rBuf, int ctransf /*CTRANSF_XXX*/) const;
	int    SLAPI Error(int errCode, long flags);

	enum {
		cfTranslateOnly = 0x0001
	};

	int    SLAPI Compile(const char * pFileName, long flags, const char * pTranslateToLang);
private:
	struct ParseItem {
		SLAPI  ParseItem();
		void   SLAPI Clear();
		enum {
			kGroup = 1, // Определитель группы строк [999]
			kString,    // Строка
			kGroupLang, // Определитель языка для всего последующего текста [/ge/] (до конца файла или до следующего
				// определителя языка.
			kLang,      // Определитель языка для следующей строки /en/
			kScopeStart,
			kScopeEnd,
			kDescription
		};
		enum {
			scopeNoTranslate = 1,
			scopeDescr
		};
		enum {
			fNextLine    = 0x0001, // Признак того, что для предыдущей строки был задан перевод на новую строку (\)
			fNoTranslate = 0x0002  // Строку не следует переводить на другие языки
		};
		long   Kind;
		long   Flags;
		long   Group;
		long   Code;
		int    SLang;  // @v8.9.10 Идентификатор языка slangXXX
		int    Scope;
		SString GroupSymb;
		SString CodeSymb;
		SString Signature;
		SString Str;
	};

	class Hash : public HashTableBase {
	public:
		SLAPI  Hash(size_t sz);
        int    SLAPI Add(long id, long pos);
        int    SLAPI Get(long id, long * pPos) const;
	private:
		size_t FASTCALL GetHash(long id) const;
	};

	struct LangStrCollItem {
		LangStrCollItem();
		~LangStrCollItem();
		int    Get(long ident, SString & rBuf) const;

		int    SLang;
		SString FileName;         // Имя бинарного файла, из которого загружаются данные
		StrAssocArray List;       // Собственно строки, сопоставленные с идентификаторами
		StrAssocArray DescrList;  // Список описаний к строкам (сопоставленны с идентификаторами строк)
		LAssocArray DescrPosList; // Список сопоставлений идентификаторов строк с позицией в файле, по которой хранится подробное описание
		StringStore2::Hash * P_PositionHash;
	};

	int    SLAPI InitParsing();
	int    SLAPI ParseFile(const char * pFileName);
	int    SLAPI GenerateHeaderFile(const char * pFileName, const char * pSrcFileName);
	int    SLAPI GenerateRcFile(const char * pFileName, const char * pSrcFileName);
	int    SLAPI ParseString(const SString & rStr, SStrScan & rScan, ParseItem & rItem);
	int    SLAPI AddGroup(ParseItem & rItem);
	int    SLAPI AddString(long group, StringStore2::ParseItem & rItem, long * pCodeStr);
	LangStrCollItem * FASTCALL GetOrConstructList(int slang);
	const LangStrCollItem * FASTCALL GetList(int slang) const;

	int    SLAPI CreateHash(int doTest);
	int    SLAPI Helper_GetString(long ident, SString & rBuf) const;
	int    SLAPI Helper_GetDescription(long ident, SString & rBuf); // @noncost
	int    SLAPI TranslationWanted(const uchar * pOrgText) const;

	int    SLAPI StoreState(const char * pFileName);
	int    SLAPI RestoreState(const char * pFileName);

#ifndef _WIN32_WCE // {
	int    SLAPI Write(SFile & rF, LangStrCollItem * pData);
	int    SLAPI Read(SFile & rF);
#endif // } _WIN32_WCE
	enum {
		fReadOnly         = 0x0001,
		fError            = 0x0002,
		fNoTranslateScope = 0x0004,  // @v9.0.2
		fDescrScope       = 0x0008   // @v9.0.8 Процесс разбора находится в области текстового определения последней строки
	};
	long   Flags;
	int    ErrCode;
	int    ActiveLanguage;
	SString LoadFileName; // Имя файла, из которого был успешно загружен ресурс функцией ::Load()

	TSCollection <LangStrCollItem> StrList;
	StrAssocArray SignatureList; // @persistent
	SymbHashTable * P_SignatureHash;
	SString ErrAddedMsg;
	SString TempBuf;

	struct StrgHeader {        // @persistent @size=64
		char   Signature[4];   // Сигнатура файла "SC2B"
		uint32 Crc32;          // CRC всего файла
		uint32 ListOffs;       // Смещение от начала файла до List (StrAssocArray)
		uint32 SListOffs;      // Смещение от начала файла до SignatureList (StrAssocArray)
		int32  SLang;          // @v8.9.10 Идентификатор языка
		uint32 Ver;            // @v9.0.2 Версия файла
		uint32 DescrListOffs;  // @v9.0.8 Смещение от начала файла до DescrPosList (LAssocArray)
		uint8  Reserve[36];    // @reserve // @v8.9.10 [48]-->[44] // @v9.0.2 [44]-->[40] // @v9.0.8 [40]-->[36]
	};

	struct ParseBlock {
		SString InFileName;            // Имя разбираемого файла
		uint   Line;                   // Номер разбираемой строки
		long   LastAutoGrpCode;
		uint32 LastSignatureCode;
		StrAssocArray GrpSymbList;
		StrAssocArray StrSymbList;
		LAssocArray LastAutoCodeList;
		//
		// Следующие элементы содержат сохраненное при предыдущей компиляции состояние
		// ради поддержания константными значений генерируемых символов
		//
		StrAssocArray Saved_SignatureList;
		StrAssocArray Saved_GrpSymbList;
		StrAssocArray Saved_StrSymbList;
	};
	ParseBlock * P_Pb;
};

#endif /* __STRSTORE_H */

