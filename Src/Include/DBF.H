// DBF.H
// Copyright (c) A.Sobolev 1993, 1997-2001, 2003, 2006, 2008, 2009, 2011, 2018, 2019, 2021, 2022, 2023
// @codepage UTF-8
// Номера записей и полей идут от 1 и далее
//
#ifndef __DBF_H
#define __DBF_H

#include <db.h>
#include <sl-packing-set01.h> // @v11.7.1

#define DEFSWAPBUFSIZE 8

class DbfTable;
//
// Нулевой заголовочный байт
//
struct DbfNilInfoByte {
	unsigned dbt_flag   : 1;  //  Подключение файла DBT
	unsigned sql_flag   : 2;  //  Флаг SQL только для dBASE IV
	unsigned res        : 1;  //  Зарезервировано
	unsigned dbt_dbase4 : 1;  //  Файл DBT в dBASE IV
	unsigned ver        : 3;  //  Номер версии dBASE
};
//
// Заголовок файла dbf
//
struct DBFH { // Size=32
	uint8  Info;         //  Нулевой заголовочный байт
	uint8  Year;         //  Год
	uint8  Month;        //  @#{1..12} месяц
	uint8  Day;          //  @#{1..32} день последней редакции
	uint32 NumRecs;      //  Число записей в файле
	uint16 HeadSize;     //  @#{32..8192} Длина заголовка файла в купе с дескрипторами полей
	uint16 RecSize;      //  @#{0..16384} Длина записи
	uint16 Reserve1;     //  Зарезервировано
	uint8  Transaction;  //  Флаг задержки транзакции только для dBASE IV
	uint8  Reserve2[13]; //  Зарезервировано
	uint8  Mdx;          //  Флаг MDX  только для dBASE IV
	uint8  LdID;         //  DBF Language Driver ID
	uint8  Reserve3[2];  //  Зарезервировано
};
/*
DBF Language Driver IDs

	OEM = 0
	Codepage_437_US_MSDOS = &H1
	Codepage_850_International_MSDOS = &H2
	Codepage_1252_Windows_ANSI = &H3
	ANSI = &H57
	Codepage_737_Greek_MSDOS = &H6A
	Codepage_852_EasernEuropean_MSDOS = &H64
	Codepage_857_Turkish_MSDOS = &H6B
	Codepage_861_Icelandic_MSDOS = &H67
	Codepage_865_Nordic_MSDOS = &H66
	Codepage_866_Russian_MSDOS = &H65
	Codepage_950_Chinese_Windows = &H78
	Codepage_936_Chinese_Windows = &H7A
	Codepage_932_Japanese_Windows = &H7B
	Codepage_1255_Hebrew_Windows = &H7D
	Codepage_1256_Arabic_Windows = &H7E
	Codepage_1250_Eastern_European_Windows = &HC8
	Codepage_1251_Russian_Windows = &HC9
	Codepage_1254_Turkish_Windows = &HCA
	Codepage_1253_Greek_Windows = &HCB
*/
//
// Дескриптор поля //
//
struct DBFF { // Size = 32
	int    GetSType(TYPEID * pTyp, long * pFmt) const;
	char   fname[11];    //  Имя поля //
	uint8  ftype;        //  Тип поля (C,L,N,M,F,D)
	uint16 offset;       //  Смещение поля относительно начала записи
	uint8  resv1[2];     //  Зарезервировано
	uint8  fsize;        //  Длина поля //
	uint8  fprec;        //  Число знаков после точки
	uint8  resv2[13];    //  Зарезервировано
	uint8  mdx_tag;      //  Флаг тега файла MDX только для dBASE IV
};

struct DBFCreateFld {
	DBFCreateFld();
	void   Init(const char * pName, int typ, uint sz, uint prec);
	char   Name[32];
	uint8  Type;
	uint8  Size;
	uint8  Prec;
};

struct DBFDate {
	int16  year;
	int8   month;
	int8   day;
};

class DbfRecord {
public:
	DbfRecord(const DbfTable *);
	~DbfRecord();
	SCodepage getCodePage() const;
	int    copy(DbfRecord * toRec);
	DbfRecord & Z();
	//
	// Для параметра fldN во всех следующих функциях диапазон [1..]
	//
	int    put(int fldN, TYPEID, const void *);
	int    put(const SdRecord & rRec);
	int    put(int fldN, const char *);
	int    put(int fldN, double);
	int    put(int fldN, float);
	int    put(int fldN, long);
	int    put(int fldN, int);
	int    put(int fldN, const DBFDate *);
	int    put(int fldN, LDATE);
	int    get(int fldN, TYPEID typ, void * pBuf) const;
	int    get(SdRecord & rRec) const;
	int    get(int fldN, char * pBuf, size_t bufSize, int skipEmpty = 0) const;
	int    get(int fldN, SString &, int skipEmpty = 0) const;
	int    get(int fldN, double &) const;
	int    get(int fldN, float &) const;
	int    get(int fldN, long &) const;
	int    get(int fldN, int &) const;
	int    get(int fldN, int64 & data) const;
	int    get(int fldN, DBFDate *) const;
	int    get(int fldN, LDATE &) const;
	int    get(int fldN, LTIME &) const;
	int    getFieldName(uint fldN /* 1.. */, char * pFldName, size_t bufLen) const;
	int    FASTCALL getFieldNumber(const char * pFldName, int * pFldN /* 1.. */) const;
	int    FASTCALL getFieldType(uint fldN /* 1.. */, int * pType);
protected:
	char * STDCALL getFldBuf(int, char * pBuf, size_t bufSize, DBFF * pF) const;
	friend class DbfTable;
	char * P_Buffer;
	size_t BufSize;
	const  DbfTable * P_Tbl;
};

class DbfTable {
public:
	DbfTable(const char * pTblName);
	~DbfTable();
	int    isOpened() const { return Opened; }
	const char * getName() const { return P_Name; }
	SCodepage getCodePage() const;
	int    getFieldName(uint fldN /* 1.. */, char * pFldName, size_t bufLen) const;
	int    FASTCALL getFieldNumber(const char * pFldName, int * pFldN /* 1.. */) const;
	bool   FASTCALL getField(uint fldN /* 1.. */, DBFF *) const;
	ulong  getNumRecs() const;
	size_t getRecSize() const;
	uint   getNumFields() const;
	int    getPosition(ulong * pPos) const;
	int    open();
	int    close();
	int    create(int aNumFlds, const DBFCreateFld * pFldsDescr, SCodepage = cpOEM, int infoByte = -1);
	int    copy(DbfTable * toTbl);
	int    rename(char * newName);
	int    empty();
	int    del();
	int    add(DbfTable * toTbl);
	int    FASTCALL goToRec(ulong);
	int    top();
	int    bottom();
	int    next();
	int    prev();
	int    deleteRec();
	DbfRecord * makeRec() const;
	int    FASTCALL getRec(DbfRecord *);
	int    FASTCALL insertRec(DbfRecord *);
	int    FASTCALL appendRec(const DbfRecord *);
	int    FASTCALL updateRec(const DbfRecord *);
	int    flush();
	int    initBuffer();
	int    releaseBuffer();
	int    isDeletedRec();
protected:
	ulong  recPerBuffer() const;
	int    flushBuffer();
	int    loadBuffer(long beg, long end);
	int    loadBuffer(ulong recNo);
	int    getHeader();
	int    getFields();
	int    putHeader();
	int    putFields();
private:
	DBFH   Head;
	DBFF * P_Flds;
	FILE * Stream;
	uint   NumFlds;
	char * P_Name;
	int    Opened;
	ulong  Current;
	ulong  BFirst;
	ulong  BLast;
	uint   BufSize; // @v12.5.0 size_t-->uint
	char * P_Buffer;
	int    Mod; // Признак модификации таблицы
};

#include <sl-packing-reset.h> // @v11.7.1

#endif //  __DBF_H
