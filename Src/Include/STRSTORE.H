// STRSTORE.H
// Copyright (c) A.Sobolev 1996-1999, 2005, 2007, 2016, 2018, 2020, 2023, 2025
// @codepage UTF-8
//
#ifndef __STRSTORE_H
#define __STRSTORE_H

#include <slib.h>
//
//
//
class StringStore2 {
public:
	StringStore2();
	~StringStore2();
	void   SetActiveLanguage(int slang);
	bool   Load(const char * pFileName, bool selfTest);
	int    GetString(int group, int code, SString & rBuf) const;
	int    GetString(const char * pSignature, SString & rBuf) const;
	//
	// Descr: Создает и заполняет хэш-таблицу символов, содержащихся в группе group.
	// Note: Возвращаемый указатель ссылается на объект, который переходит в собственность
	//   вызывающей функции.
	// Returns:
	//   0 - не удалось создать хэш-таблицу. Вероятные причины:
	//       -- группа group не является хэширующей
	//       -- группа group пуста
	//       -- не достаточно памяти
	//       -- могут быть и иные причины
	//   !0 - указатель на хэш-таблицу, переходящую в собственность вызывающей функции.
	//       после использования таблица должна быть разрушена в целях освобождения памяти.
	//
	SymbHashTable * LoadStringHash(int group) const;
	//
	// Descr: Возвращает собственный экземпляр хэш-таблицы символов, ассоциированный с группой group.
	// Note: В отличии от LoadStringHash эта функция возвращает собственный экземпляр хэш-таблицы.
	//   Если таблица до этого не была загружена, то она загружается.
	// Returns:
	//   0 - не удалось создать хэш-таблицу. См. возможные причины в описании функции LoadStringHash.
	//   !0 - указатель на хэш-таблицу. Этот указатель ссылается на "собственность" this. Таким образом
	//      никакие nonconst действия (в том числе и посредством "трюков") с этим экземпляром
	//      абсолютно не допустимы: в результате могут быть серьезно повреждены внутренности объекта,
	//      совместно используемого многими потоками.
	//
	const  SymbHashTable * FASTCALL GetStringHash(int group); // @nonconst @cs
	int    GetDescription(int group, int code, SString & rBuf); // @nonconst @cs
	int    GetDescription(const char * pSignature, SString & rBuf); // @nonconst @cs
	int    ExpandString(SString & rBuf, int ctransf /*CTRANSF_XXX*/) const;
	int    Error(int errCode, long flags);

	enum {
		cfTranslateOnly = 0x0001
	};

	int    Compile(const char * pFileName, long flags, const char * pTranslateToLang);
private:
	struct ParseItem {
		ParseItem();
		ParseItem & Z();
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
			fNoTranslate = 0x0002, // Строку не следует переводить на другие языки
			fHashGroup   = 0x0004  // Если Kind == kGroup и этот флаг установлен, то строка определяет хэширующую группу [abc:hash]
		};
		long   Kind;
		long   Flags;
		long   Group;
		long   Code;
		int    SLang;  // Идентификатор языка slangXXX
		int    Scope;
		SString GroupSymb;
		SString CodeSymb;
		SString Signature;
		SString Str;
	};

	class Hash : public HashTableBase {
	public:
		Hash(size_t sz);
		int    Add(long id, long pos);
		int    Get(long id, long * pPos) const;
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
		LAssocArray HashAssocList; // Список ассоциаций хэширующих групп с идентификаторами строк, в них входящих.
			// Хэширующая группа обеспечивает быстрый поиск любой строки, входящей в нее, по содержанию и возврат кода этой строки.
			// Техника хэширующих групп необходима для разбора входящий потоков и команд.
		StringStore2::Hash * P_PositionHash;
	};
	//
	// Descr: Структура, определяющая исходный и результирующий файл, обработанные при предыдущем сеансе компиляции.
	//   С файлами ассоциированы хэш-коды, что позволяет пропустить компиляцию в случае, если ничего не изменилось ни в 
	//   исходном, ни в результирующем файлах.
	//
	struct HashEntry { // @v12.3.4
		HashEntry();
		~HashEntry();
		HashEntry & Z();
		
		int   LangId;
		SString SrcFileName; // Имя исходного файла без пути (name.ext)
		SString BinFileName; // Имя скомпилированного файла без пути (name.ext)
		SBinaryChunk SrcFileHash; // Хэш исходного файла 
		SBinaryChunk BinFileHash; // Хэш скомпилированного файла
	};

	class HashBlock : public TSCollection <HashEntry> {
	public:
		HashBlock();
		HashBlock & Z();
		int    CreateEntry(int langId, const char * pSrcFilePath, const char * pBinFilePath);
		bool   MakeFileHash(const char * pFilePath, SString * pResultFileName, SBinaryChunk & rHash) const;
		bool   MakeCHdrHash(const char * pFilePath);
		bool   MakeJavaHash(const char * pFilePath);
		void   CopyCHdrHash(const HashBlock & rS);
		void   CopyJavaHash(const HashBlock & rS);
		bool   CopyEntry(int langId, const HashBlock & rS);
		HashEntry * SearchLangEntry(int langId, uint * pPos) const;

		int    HashFunc;
		SString CHdrFileName; // Имя сгенерированного заголовочного (.h) файла 
		SString JavaFileName; // Имя сгенерированного java файла 
		SBinaryChunk CHdrFileHash; // Хэш сгенерированного заголовочного (.h) файла 
		SBinaryChunk JavaFileHash; // Хэш сгенерированного java файла 
	};
	//
	// Descr: Вспомогательный блок, управляющий набором данных, источником которых является базовый исходный файл, переданный в командной строке
	//
	class OrgSrcFileBlock {
	public:
		OrgSrcFileBlock(const char * pFileName, const char * pTranslateToLang);
		bool   operator !() const { return (State & stError); }
		void   MakeIdentFilePath(SString & rFilePath) const; // *.symb
		void   MakeLangSrcFileSearchPattern(SString & rPattern) const;
		void   MakeAutotranslateFilePath(SString & rFilePath) const;
		SString & MakeSrcFilePath(int langId, SString & rFilePath, SString * pFileName) const;
		SString & MakeBinFilePath(int langId, SString & rFilePath, SString * pFileName) const;
		SString & MakeCHdrFilePath(SString & rFilePath, SString * pFileName) const;
		SString & MakeJavaFilePath(SString & rFilePath, SString * pFileName) const;
		const  SString & GetOrgName() const { return Ps.Nam; }
		const  SString & GetOrgExt() const { return Ps.Ext; }

		enum {
			stError = 0x0001
		};
		uint   State;
		int    PrivateErrCode;
		int    AutoTranslLang;
		const  SFsPath Ps; // Хранит структуру пути исходного файла
		SString IdentFileName; // Имя файла, хранящего соответствия символов и идентификаторов
		SString AutoTranslFile;
		StrAssocArray FileLangList;
	private:
		SString & MakeFilePathByChangingExt(const char * pExt, SString & rFilePath, SString * pFileName) const;
	};

	int    InitParsing();
	int    ParseFile(const char * pFileName);
	int    GenerateHeaderFile(const char * pFileName, const char * pSrcFileName);
	int    GenerateJavaFile(const char * pFileName, const char * pSrcFileName);
	int    GenerateRcFile(const char * pFileName, const char * pSrcFileName);
	int    ParseString(const SString & rStr, SStrScan & rScan, ParseItem & rItem);
	int    AddGroup(ParseItem & rItem);
	int    AddString(long group, StringStore2::ParseItem & rItem, long * pCodeStr);
	LangStrCollItem * FASTCALL GetOrConstructList(int slang);
	const LangStrCollItem * FASTCALL GetList(int slang) const;
	int    CreateHash(int doTest);
	int    Helper_GetString(long ident, SString & rBuf) const;
	int    Helper_GetDescription(long ident, SString & rBuf); // @noncost
	int    TranslationWanted(const uchar * pOrgText) const;
	int    StoreState(const char * pFileName, const HashBlock & rHashBlk);
	int    RestoreState(const char * pFileName, const StrAssocArray & rFileLangList);
	bool   VerifyHashEntry(int langId, const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk);
	bool   VerifyHashEntry_CHdr(const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk);
	bool   VerifyHashEntry_Java(const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk);
	int    Write(SFile & rF, LangStrCollItem * pData);
	int    Read(SFile & rF);

	struct StrgHeader {        // @persistent @size=64
		char   Signature[4];   // Сигнатура файла "SC2B"
		uint32 Crc32;          // CRC всего файла
		uint32 ListOffs;       // Смещение от начала файла до List (StrAssocArray)
		uint32 SListOffs;      // Смещение от начала файла до SignatureList (StrAssocArray)
		int32  SLang;          // Идентификатор языка
		uint32 Ver;            // Версия файла
		uint32 DescrListOffs;  // Смещение от начала файла до DescrPosList (LAssocArray)
		uint8  Reserve[36];    // @reserve // @v8.9.10 [48]-->[44] // @v9.0.2 [44]-->[40] // @v9.0.8 [40]-->[36]
	};
	//
	// Descr: Предварительно загруженая хэш-таблица символов
	//
	struct PreloadedHashGroupEntry {
		PreloadedHashGroupEntry();
		~PreloadedHashGroupEntry();
		
		int    Group;
		SymbHashTable * P_Tab;
	};

	struct ParseBlock {
		ParseBlock();
		~ParseBlock();
		
		SString InFileName;            // Имя разбираемого файла
		uint   Line;                   // Номер разбираемой строки
		long   LastAutoGrpCode;
		uint32 LastSignatureCode;
		StrAssocArray StrSymbList;
		LAssocArray LastAutoCodeList;
		//
		// Следующие элементы содержат сохраненное при предыдущей компиляции состояние
		// ради поддержания константными значений генерируемых символов
		//
		StrAssocArray Saved_SignatureList;
		StrAssocArray Saved_GrpSymbList;
		StrAssocArray Saved_StrSymbList;
		HashBlock Saved_HashBlk; // @v12.3.4
	};

	enum {
		fReadOnly         = 0x0001,
		fError            = 0x0002,
		fNoTranslateScope = 0x0004,  // 
		fDescrScope       = 0x0008   // Процесс разбора находится в области текстового определения последней строки
	};
	long   Flags;
	int    ErrCode;
	int    ActiveLanguage;
	SString LoadFileName; // Имя файла, из которого был успешно загружен ресурс функцией ::Load()
	TSCollection <LangStrCollItem> StrList;
	StrAssocArray SignatureList; // @persistent
	SymbHashTable * P_SignatureHash;
	SString ErrAddedMsg;
	StrAssocArray GrpSymbList; // 
	LongArray HashGroupList;   // Список идентификаторов групп, которые определены как хэширующие	
	TSCollection <PreloadedHashGroupEntry> PhgL;
	ParseBlock * P_Pb;
};

#endif /* __STRSTORE_H */
