// DataType.java
// Copyright (c) A.Sobolev 2021
//
package ru.petroglif.styloq;
//
// БОльшая часть определений следующего класса заимствована из C++ библиотеки SLIB
//
public class DataType {
	//
	// Base types. Базовые типы, используемые для приведения и преобразования разных типов.
	//
	public final static int BTS_VOID           =  0;
	public final static int BTS_STRING         =  1;    // ASCIIZ
	public final static int BTS_INT            =  2;    // 4 byte
	public final static int BTS_REAL           =  3;    // 8 byte IEEE float
	public final static int BTS_DATE           =  4;    // 4 byte LDATE
	public final static int BTS_TIME           =  5;    // 4 byte LTIME
	public final static int BTS_DATETIME       =  6;    // 8 byte LDATETIME
	public final static int BTS_POINT2         =  7;    // 16 byte SPoint2R
	public final static int BTS_BOOL           =  8;    // 4 byte Специализированный вариант. Фактически не применяется как базовый тип, но используется в локальных ситуациях.
	public final static int BTS_PTR            =  9;    // Указатель. Важно, что размер его зависим от архитектуры (4 или 8 байт)
	public final static int BTS_INT64_         = 10;    // 8 byte integer. Не является базовым типом. Определен для использования в DBConst
	//
	// Data types
	//
	public final static int S_VOID             =  0;
	public final static int S_CHAR             =  1;
	public final static int S_INT              =  2;
	public final static int S_FLOAT            =  3;
	public final static int S_DATE             =  4;
	public final static int S_TIME             =  5;
	public final static int S_DEC              =  6;
	public final static int S_MONEY            =  7;
	public final static int S_LOGICAL          =  8;
	public final static int S_BOOL             =  S_LOGICAL;
	public final static int S_NUMERIC          =  9;
	public final static int S_BFLOAT           = 10;
	public final static int S_LSTRING          = 11;
	public final static int S_ZSTRING          = 12;
	public final static int S_NOTE             = 13;     // Для BTRIEVE - символьное поле переменной длины (последнее в таблице), для SQL - VARCHAR. Максимальная бинарная (с завершающим нулем) длина специфицируется //
	public final static int S_LVAR             = 14;
	public final static int S_UBINARY          = 15;
	public final static int S_UINT             = S_UBINARY;
	public final static int S_AUTOINC          = 16;
	public final static int S_BIT              = 17;
	public final static int S_STS              = 18;

	public final static int S_INTRANGE         = 19;		// int2 range only
	public final static int S_REALRANGE        = 20;		// double range only
	public final static int S_DATERANGE        = 21;      //
	public final static int S_DATETIME	       = 22;      //
	public final static int S_ARRAY		       = 23;		// not real STYPE, only ID. Массив, состоящий из данных одного типа
	public final static int S_STRUCT	       = 24;		//--//--
	public final static int S_VARIANT          = 25;      //
	public final static int S_WCHAR            = 26;      // wide char-строка (оканчивается 0). Размер данных указывается в байтах.
	public final static int S_BLOB             = 27;      // BLOB (для SQL-таблиц)
	public final static int S_CLOB             = 28;      // CLOB (для SQL-таблиц)
	public final static int S_RAW              = 29;      // RAW-data бинарное поле с произвольным содержимым
	public final static int S_ROWID            = 30;      // ROWID (для SQL-таблиц)
	public final static int S_IPOINT2          = 31;      // Целочисленная двумерная точка (x, y)
	public final static int S_FPOINT2          = 32;      // Вещественная двумерная точка (x, y)
	public final static int S_WZSTRING         = 33;      // Unicode-zstring
	public final static int S_UUID_            = 34;      // (Суффиксное подчеркивание необходимо для отличия от class S_GUID)
	public final static int S_INT64            = 35;      // @v10.6.3
	public final static int S_UINT64           = 36;      // @v10.6.3
	public final static int S_COLOR_RGBA       = 37;      // @v10.9.10 4-компонентное представление цвета ARGB class SColorBase

	public static int MKSTYPE(int typ, int siz) { return ((siz)<<16)|(typ); }
	public static int MKSTYPED(int typ, int siz, int prc) { return (((prc)<<24)|(((siz))<<16)|(typ)); }

	public static int T_CHAR()       { return MKSTYPE(S_CHAR, 1); }
	public static int T_INT()        { return MKSTYPE(S_INT,2); }
	public static int T_UINT()       { return MKSTYPE(S_UBINARY,2); }
	public static int T_UINT16()     { return MKSTYPE(S_UBINARY,2); }
	public static int T_UINT32()     { return MKSTYPE(S_UBINARY,4); }
	public static int T_UINT64()     { return MKSTYPE(S_UBINARY,8); }
	public static int T_INT16()      { return MKSTYPE(S_INT,2); }
	public static int T_INT32()      { return MKSTYPE(S_INT,4); }
	public static int T_INT64()      { return MKSTYPE(S_INT64 ,8); }
	public static int T_LONG()       { return MKSTYPE(S_INT,4); }
	public static int T_ULONG()      { return MKSTYPE(S_UBINARY,4); }
	public static int T_FLOAT()      { return MKSTYPE(S_FLOAT,4); }
	public static int T_DOUBLE()     { return MKSTYPE(S_FLOAT,8); }
	public static int T_MONEY()      { return MKSTYPED(S_DEC,8,2); }
	public static int T_DATE()       { return MKSTYPE(S_DATE,4); }
	public static int T_TIME()       { return MKSTYPE(S_TIME,4); }
	public static int T_DATETIME()   { return MKSTYPE(S_DATETIME,8); }
	public static int T_ARRAY()      { return MKSTYPE(S_ARRAY,0); }
	public static int T_STRUCT()     { return MKSTYPE(S_STRUCT,0); }
	//public static int T_VARIANT()    { return MKSTYPE(S_VARIANT, sizeof(VARIANT)); }
	public static int T_ROWID()      { return MKSTYPE(S_ROWID, 8); }
	public static int T_IPOINT2()    { return MKSTYPE(S_IPOINT2, 4); }
	public static int T_FPOINT2()    { return MKSTYPE(S_FPOINT2, 8); }
	public static int T_BOOL()       { return MKSTYPE(S_BOOL, 4); }
	public static int T_GUID()       { return MKSTYPE(S_UUID_, 16); }
	public static int T_COLOR_RGBA() { return MKSTYPE(S_COLOR_RGBA, 4); }
	
	//public final static int FldTypInt    = 1;
	//public final static int FldTypLong   = 2;
	//public final static int FldTypString = 3;
	//public final static int FldTypDouble = 4;
	//public final static int FldTypDate   = 5; // Используется в броузерах, на самом деле в базе хранится как int
	//public final static int FldTypImg    = 6; // Используется в броузерах
	//public final static int FldTypes[] = {FldTypInt, FldTypLong, FldTypString, FldTypDouble, FldTypDate, FldTypImg};
}
