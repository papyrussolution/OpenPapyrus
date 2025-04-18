// STRSTORE.H
// Copyright (c) A.Sobolev 1996-1999, 2005, 2007, 2016, 2018, 2020, 2023
// @codepage windows-1251
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
	int    Load(const char * pFileName, int selfTest);
	int    GetString(int group, int code, SString & rBuf) const;
	int    GetString(const char * pSignature, SString & rBuf) const;
	//
	// Descr: ������� � ��������� ���-������� ��������, ������������ � ������ group.
	// Note: ������������ ��������� ��������� �� ������, ������� ��������� � �������������
	//   ���������� �������.
	// Returns:
	//   0 - �� ������� ������� ���-�������. ��������� �������:
	//       -- ������ group �� �������� ����������
	//       -- ������ group �����
	//       -- �� ���������� ������
	//       -- ����� ���� � ���� �������
	//   !0 - ��������� �� ���-�������, ����������� � ������������� ���������� �������.
	//       ����� ������������� ������� ������ ���� ��������� � ����� ������������ ������.
	//
	SymbHashTable * LoadStringHash(int group) const;
	//
	// Descr: ���������� ����������� ��������� ���-������� ��������, ��������������� � ������� group.
	// Note: � ������� �� LoadStringHash ��� ������� ���������� ����������� ��������� ���-�������.
	//   ���� ������� �� ����� �� ���� ���������, �� ��� �����������.
	// Returns:
	//   0 - �� ������� ������� ���-�������. ��. ��������� ������� � �������� ������� LoadStringHash.
	//   !0 - ��������� �� ���-�������. ���� ��������� ��������� �� "�������������" this. ����� �������
	//      ������� nonconst �������� (� ��� ����� � ����������� "������") � ���� �����������
	//      ��������� �� ���������: � ���������� ����� ���� �������� ���������� ������������ �������,
	//      ��������� ������������� ������� ��������.
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
			kGroup = 1, // ������������ ������ ����� [999]
			kString,    // ������
			kGroupLang, // ������������ ����� ��� ����� ������������ ������ [/ge/] (�� ����� ����� ��� �� ����������
				// ������������ �����.
			kLang,      // ������������ ����� ��� ��������� ������ /en/
			kScopeStart,
			kScopeEnd,
			kDescription
		};
		enum {
			scopeNoTranslate = 1,
			scopeDescr
		};
		enum {
			fNextLine    = 0x0001, // ������� ����, ��� ��� ���������� ������ ��� ����� ������� �� ����� ������ (\)
			fNoTranslate = 0x0002, // ������ �� ������� ���������� �� ������ �����
			fHashGroup   = 0x0004  // ���� Kind == kGroup � ���� ���� ����������, �� ������ ���������� ���������� ������ [abc:hash]
		};
		long   Kind;
		long   Flags;
		long   Group;
		long   Code;
		int    SLang;  // ������������� ����� slangXXX
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
		SString FileName;         // ��� ��������� �����, �� �������� ����������� ������
		StrAssocArray List;       // ���������� ������, �������������� � ����������������
		StrAssocArray DescrList;  // ������ �������� � ������� (������������� � ���������������� �����)
		LAssocArray DescrPosList; // ������ ������������� ��������������� ����� � �������� � �����, �� ������� �������� ��������� ��������
		LAssocArray HashAssocList; // @v9.6.4 ������ ���������� ���������� ����� � ���������������� �����, � ��� ��������.
			// ���������� ������ ������������ ������� ����� ����� ������, �������� � ���, �� ���������� � ������� ���� ���� ������.
			// ������� ���������� ����� ���������� ��� ������� �������� ������� � ������.
		StringStore2::Hash * P_PositionHash;
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
	int    StoreState(const char * pFileName);
	int    RestoreState(const char * pFileName);
#ifndef _WIN32_WCE // {
	int    Write(SFile & rF, LangStrCollItem * pData);
	int    Read(SFile & rF);
#endif // } _WIN32_WCE
	enum {
		fReadOnly         = 0x0001,
		fError            = 0x0002,
		fNoTranslateScope = 0x0004,  // 
		fDescrScope       = 0x0008   // ������� ������� ��������� � ������� ���������� ����������� ��������� ������
	};
	long   Flags;
	int    ErrCode;
	int    ActiveLanguage;
	SString LoadFileName; // ��� �����, �� �������� ��� ������� �������� ������ �������� ::Load()

	TSCollection <LangStrCollItem> StrList;
	StrAssocArray SignatureList; // @persistent
	SymbHashTable * P_SignatureHash;
	SString ErrAddedMsg;
	SString TempBuf;

	struct StrgHeader {        // @persistent @size=64
		char   Signature[4];   // ��������� ����� "SC2B"
		uint32 Crc32;          // CRC ����� �����
		uint32 ListOffs;       // �������� �� ������ ����� �� List (StrAssocArray)
		uint32 SListOffs;      // �������� �� ������ ����� �� SignatureList (StrAssocArray)
		int32  SLang;          // ������������� �����
		uint32 Ver;            // ������ �����
		uint32 DescrListOffs;  // �������� �� ������ ����� �� DescrPosList (LAssocArray)
		uint8  Reserve[36];    // @reserve // @v8.9.10 [48]-->[44] // @v9.0.2 [44]-->[40] // @v9.0.8 [40]-->[36]
	};

	StrAssocArray GrpSymbList; // 
	LongArray HashGroupList;   // ������ ��������������� �����, ������� ���������� ��� ����������
	//
	// Descr: �������������� ���������� ���-������� ��������
	//
	struct PreloadedHashGroupEntry {
		PreloadedHashGroupEntry() : Group(0), P_Tab(0)
		{
		}
		~PreloadedHashGroupEntry()
		{
			ZDELETE(P_Tab);
		}
		int    Group;
		SymbHashTable * P_Tab;
	};
	TSCollection <PreloadedHashGroupEntry> PhgL;
	//
	struct ParseBlock {
		SString InFileName;            // ��� ������������ �����
		uint   Line;                   // ����� ����������� ������
		long   LastAutoGrpCode;
		uint32 LastSignatureCode;
		StrAssocArray StrSymbList;
		LAssocArray LastAutoCodeList;
		//
		// ��������� �������� �������� ����������� ��� ���������� ���������� ���������
		// ���� ����������� ������������ �������� ������������ ��������
		//
		StrAssocArray Saved_SignatureList;
		StrAssocArray Saved_GrpSymbList;
		StrAssocArray Saved_StrSymbList;
	};
	ParseBlock * P_Pb;
};

#endif /* __STRSTORE_H */
