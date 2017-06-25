// SLIB.H
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#ifndef __SLIB_H
#define __SLIB_H
#define SLIBINCLUDED
#ifndef __SLCONFIG_H
	#include <slconfig.h>
#endif
#ifndef RC_INVOKED
	#ifndef __STDIO_H
		#include <stdio.h>
	#endif
	#if !defined(__palmos__) && !defined(_WIN32_WCE)
		#ifndef __IO_H
			#include <io.h>
		#endif
	#endif
	#include <assert.h>
	#include <limits.h>
	#include <stddef.h>
	#include <math.h>
	#include <float.h>
	#include <time.h>
#endif
#include <port.h>
//
// Types & definitions
//
#ifndef UNDEF
	#define UNDEF         0xffff
#endif
#ifndef TRUE
	#define TRUE               1
#endif
#ifndef FALSE
	#define FALSE              0
#endif
#define _END_                 0
#define _AND_                 1
#define _OR_                  2
#define _NOT_                 3
#define _ALL_                 0
#define _EQ_                  1
#define _OUTER_EQ_          101
#define _GT_                  2
#define _LT_                  3
#define _NE_                  4
#define _GE_                  5
#define _LE_                  6
#define _IN_                  7	// SQL
#define _NIN_                 8	// SQL
#define _BETWEEN_             9	// SQL
#define _NBETWEEN_           10	// SQL
#define _BEGWITH_            11	// SQL
#define _CONTAINS_           12	// SQL
#define _NCONTAINS_          13	// SQL
#define _LIKE_               14	// SQL
#define _NLIKE_              15	// SQL
#define _EXISTS_             16	// SQL
#define _NEXISTS_            17	// SQL
//
// Стороны света
//
#define SOW_UNKN              0
#define SOW_NORD              1
#define SOW_NORDEAST          2
#define SOW_EAST              3
#define SOW_SOUTHEAST         4
#define SOW_SOUTH             5
#define SOW_SOUTHWEST         6
#define SOW_WEST              7
#define SOW_NORDWEST          8
//
// Стороны прямоугольной области
//
#define SIDE_LEFT             SOW_WEST
#define SIDE_TOP              SOW_NORD
#define SIDE_RIGHT            SOW_EAST
#define SIDE_BOTTOM           SOW_SOUTH
#define SIDE_CENTER           100
//
// Направления //
//
#define DIREC_UNKN            SOW_UNKN
#define DIREC_HORZ            SOW_EAST
#define DIREC_VERT            SOW_NORD
//
// Методы комбинации объектов
//
#define SCOMBINE_NONE         0
#define SCOMBINE_AND          1
#define SCOMBINE_OR           2
#define SCOMBINE_XOR          3
#define SCOMBINE_COPY         4
#define SCOMBINE_DIFF         5
//
// Descr: Направления выравнивания строк
//
#define ADJ_LEFT              0 // По левому краю
#define ADJ_RIGHT             1 // По правому краю
#define ADJ_CENTER            2 // По центру
//
// Descr: Сторона "набивки" строк
//
#define PAD_LEFT              0 // Слева
#define PAD_RIGHT             1 // Справа
//
// Descr: Дескрипторы регистра символов в строке
//
#define CCAS_UNDEF            0 // Как есть
#define CCAS_LOWER            1 // Все символы строчные
#define CCAS_UPPER            2 // Все символы прописные
#define CCAS_CAPITAL          3 // Первая буква прописная, остальные строчные
//
// Единицы измерения //
//
#define UNIT_METER            1
#define UNIT_INCH             2
#define UNIT_KILOGRAM         3
#define UNIT_GRAM             4
#define UNIT_LITER            5
#define UNIT_FLOZ             6 // fluid ounce 0.029573531 liters
#define UNIT_WATT             7
#define UNIT_VOLT             8
#define UNIT_DAY              9 // Сутки
#define UNIT_HOUR            10 // Час
#define UNIT_SECOND          11 // Секунда
#define UNIT_ITEM            12 // Абстрактная штука
#define UNIT_OZ              13 // ounce = 28.3495231 gram
#define UNIT_COLOR           14 // Специфицеская единица - количество цветов в наборе
#define UNIT_CMETER          15 // Сантиметр
#define UNIT_MMETER          16 // Миллиметр
#define UNIT_PERCENT         17 // Процент
#define UNIT_GR_PIXEL        18 // Графическая единица: Точка устройства вывода
#define UNIT_GR_PT           19 // Графическая единица: 1/72 дюйма
#define UNIT_GR_PC           20 // Графическая единица: 1/6 дюйма
#define UNIT_GR_EM           21 // Графическая единица: em
#define UNIT_GR_EX           22 // Графическая единица: ex
#define UNIT_ONE             23 // Единица. Используется как база для частичных величин: percent например.
#define UNIT_RADIAN          24
#define UNIT_DEGREE          25 // Угловой градус
#define UNIT_GR_DIALOG       26 // Графическая единица: одна диалоговая единица (неизоморфна, контекстно-зависима)
#define UNIT_MINUTE          27 // Минута
//
//
//
#define WIN32_LEAN_AND_MEAN
//#define _WIN32_WINNT 0x0501
//#define _WIN32_IE 0x0501 // @v9.0.8
#include <windows.h>
#include <tchar.h>
//
//
//
#if !defined(FORCEINLINE)
	#if defined(_MSC_VER)
		#define FORCEINLINE __forceinline
	#else
		#define FORCEINLINE
	#endif
#endif

#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
	#define SFAR
	#define SLAPI    //__stdcall Тип вызова __stdcall позволяет уменьшить размер бинарного кода, но
		// снижает скорость работы member-функция из-за того, что this передается на стеке, а не в регистре
	#define SLAPIV   __cdecl
	// @v9.6.3 #define SLCLASS
	#define FASTCALL __fastcall // Следует использовать для часто вызываемых функций с количеством параметров
		// от 1 до 3. Не совсем понятны побочные эффекты от использования регистров под параметры с точки
		// зрения оптимизации распределения регистров в вызывающих функциях.
#else
	#ifndef PASCAL
		#define PASCAL           pascal
	#endif
	#define SFAR                 far
	#define SLAPI                pascal SFAR
	#define SLAPIV               cdecl SFAR
	// @v9.6.3 #define SLCLASS
	#define FASTCALL
#endif
#ifdef _MSC_VER
	#define THREADLOCAL __declspec(thread)
#else
	#define THREADLOCAL
#endif
/* @v9.1.12
#ifndef _SIZE_T
#define _SIZE_T
	typedef unsigned size_t;
#endif
*/
#ifndef MAXPATH
	#if defined(__WIN32__) || defined(_WIN32)
		#define MAXPATH 260
	#else
		#define MAXPATH  80
	#endif
#endif

// @v9.0.1 #define SPTRDEF(typ)        typedef typ SFAR * P ## typ
// @v9.0.1 #define SPTR2DEF(typ)       typedef typ SFAR * SFAR * PP ## typ
// @v9.0.1 #define SPTRCDEF(typ)       typedef const typ SFAR * PC ## typ
// @v9.0.1 #define SPTR(type)          P ## typ
// @v9.0.1 #define SPTRC(typ)          PC ## typ

#define CALLPTRMEMB(ptr, memb)          if(ptr) ptr->memb;
#define CALLTYPEPTRMEMB(typ, ptr, memb) if(ptr) ((typ *)ptr)->memb;

// @v8.7.2 #ifdef __cplusplus
	// @v8.7.2 #define SREFDEF(typ)     typedef typ SFAR & R ## typ
	// @v8.7.2 #define SREF(typ)        R ## typ
	// @v8.7.2 #define SCLASSDEF(typ)   class SLCLASS typ;  SPTRDEF(typ); SREFDEF(typ);
	// @v8.7.2 #define SSTRUCTDEF(typ)  struct SLCLASS typ; SPTRDEF(typ); SREFDEF(typ);
// @v8.7.2 #endif
#ifdef DECL_INTX
	typedef signed char    int8;
	typedef signed short   int16;
	typedef signed long    int32;
	typedef unsigned char  uint8;
	typedef unsigned short uint16;
	typedef unsigned long  uint32;
	typedef __int64        int64;
	typedef unsigned __int64 uint64;
#endif
typedef long double         LDBL;
typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ulonglong;
typedef char                BCD4[4];
typedef char                BCD8[8];
typedef char                BCD10[10];
typedef char              * BCD_T;
typedef char              * Pchar;
typedef uint16              RECORDSIZE;
typedef unsigned long       RECORDNUMBER;

// @v9.1.12 enum Boolean { False, True };

// @v8.7.2 #ifdef __cplusplus // {

class  SBuffer;
class  SString;
class  TPoint;
class  TRect;
class  RPoint;
class  LMatrix;
#ifndef _WIN32_WCE // {
class  SFile;
#endif // } _WIN32_WCE
class  StringSet;
class  SIniFileBuffer;
struct SStrScan;
struct SInvariantParam;
class  SRandGenerator;
class  SSerializeContext;
class  SArray;
class  LongArray;
class  RAssocArray;
class  UintHashTable;
class  SPaintToolBox;
class  ObjStore;
struct SMathResult;
class  SUsbDevice;
class  SDrawImage;
class  SPaintToolBox;
class  TCanvas2;
class  TVRez;
class  WinServiceMngr;
class  StatBase;
class  SoapPacket;
class  SoapPacketStruc;
class  SoapPacketArray;
//
// Прототип функций сравнения и макросы для использования этих функций
//
typedef int (FASTCALL *CompFunc)(const void *, const void *, void * pExtraData);

#define DECL_CMPFUNC(typ)        int FASTCALL Cmp_##typ (const void *, const void *, void * pExtraData)
#define IMPL_CMPFUNC(typ,p1,p2)  int FASTCALL Cmp_##typ (const void * p1, const void * p2, void * pExtraData)
#define PTR_CMPFUNC(typ)         Cmp_##typ
#define CMPFUNC(typ,p1,p2)       Cmp_##typ(p1,p2,0)
#define DECL_CMPCFUNC(typ)       int FASTCALL CmpC_##typ (const void *, const void *, void * pExtraData)
#define IMPL_CMPCFUNC(typ,p1,p2) int FASTCALL CmpC_##typ (const void * p1, const void * p2, void * pExtraData)
#define PTR_CMPCFUNC(typ)        CmpC_##typ
#define CMPCFUNC(typ,p1,p2)      CmpC_##typ(p1,p2,0)

#define CMPF_LONG                Cmp_long
#define CMPF_INT64               Cmp_int64

#define CMPSIGN(p1, p2) ((p1)>(p2))?+1:(((p1)<(p2))?-1:0)
#define CMPCASCADE2(si, s1, s2, m1, m2)      \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1))) si=CMPSIGN((s1)->m2,(s2)->m2);
#define CMPCASCADE3(si, s1, s2, m1, m2, m3)     \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1)) && !(si=CMPSIGN((s1)->m2,(s2)->m2))) si=CMPSIGN((s1)->m3,(s2)->m3);
#define CMPCASCADE4(si, s1, s2, m1, m2, m3, m4)     \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1)) && !(si=CMPSIGN((s1)->m2,(s2)->m2)) && \
		!(si=CMPSIGN((s1)->m3,(s2)->m3))) si=CMPSIGN((s1)->m4,(s2)->m4);
#define CMPCASCADE5(si, s1, s2, m1, m2, m3, m4, m5)  \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1)) && !(si=CMPSIGN((s1)->m2,(s2)->m2)) && \
		!(si=CMPSIGN((s1)->m3,(s2)->m3)) && !(si=CMPSIGN((s1)->m4,(s2)->m4))) si=CMPSIGN((s1)->m5,(s2)->m5);
#define CMPCASCADE6(si, s1, s2, m1, m2, m3, m4, m5, m6)  \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1)) && !(si=CMPSIGN((s1)->m2,(s2)->m2)) && !(si=CMPSIGN((s1)->m3,(s2)->m3)) && \
		!(si=CMPSIGN((s1)->m4,(s2)->m4)) && !(si=CMPSIGN((s1)->m5,(s2)->m5))) si=CMPSIGN((s1)->m6,(s2)->m6);
#define CMPCASCADE7(si, s1, s2, m1, m2, m3, m4, m5, m6, m7)  \
	if(!(si=CMPSIGN((s1)->m1,(s2)->m1)) && !(si=CMPSIGN((s1)->m2,(s2)->m2)) && !(si=CMPSIGN((s1)->m3,(s2)->m3)) && \
		!(si=CMPSIGN((s1)->m4,(s2)->m4)) && !(si=CMPSIGN((s1)->m5,(s2)->m5)) && !(si=CMPSIGN((s1)->m6,(s2)->m6))) si=CMPSIGN((s1)->m7,(s2)->m7);
//
// Прототип функций вывода значения в строковый буфер
//
typedef int (FASTCALL * OutpFunc)(const void * pData, long fmt, SString & rBuf);

#define DECL_OUTPFUNC(typ) int FASTCALL Outp_##typ (const void *, long, SString &)
#define IMPL_OUTPFUNC(typ) int FASTCALL Outp_##typ (const void * pData, long fmt, SString & rBuf)
#define PTR_OUTPFUNC(typ)  Outp_##typ
#define OUTPFUNC(typ,dat,fmt,buf) Outp_##typ(data,fmt,buf)
//
// Макросы для использования инвариантов
//
#define DECL_INVARIANT_C()    int SLAPI InvariantC(SInvariantParam * pInvP) const
#define IMPL_INVARIANT_C(cls) int SLAPI cls::InvariantC(SInvariantParam * pInvP) const
//
#define SIZEOFARRAY(array) (sizeof(array)/sizeof((array)[0]))
#define IS_ODD(n)          ((n) & 1)
#define IS_EVEN(n)         (!(IS_ODD(n)))
//
//
//
#include <asmlib.h>
#if USE_ASMLIB
	#define memcpy   A_memcpy
	#define memmove  A_memmove
	#define memset   A_memset
	#define memcmp   A_memcmp // @v8.7.7
	//#define strcat   A_strcat
	//#define strlen   A_strlen
	//#define strcmp   A_strcmp
	//#define stricmp  A_stricmp
	//#define strstr   A_strstr
#endif
#if SALLOCATOR == SALLOC_NEDMALLOC

	#include <nedmalloc.h>

	inline void * SAlloc::M(size_t sz)
	{
		return nedmalloc(sz);
	}
	inline void * SAlloc::C(size_t n, size_t sz)
	{
		return nedcalloc(n, sz);
	}
	inline void * SAlloc::R(void * ptr, size_t sz)
	{
		return nedrealloc(ptr, sz);
	}
	inline void SAlloc::F(void * ptr)
	{
		return nedfree(ptr);
	}
	inline void * operator new(size_t sz)
	{
		return nedmalloc(sz);
	}
	inline void operator delete(void * ptr)
	{
		nedfree(ptr);
	}
#endif

class SAlloc {
public:
	static void * FASTCALL M(size_t sz);
	static void * FASTCALL C(size_t n, size_t sz);
	static void * FASTCALL R(void * ptr, size_t sz);
	static void   FASTCALL F(void * ptr);
};
//
// Экземпляр SCriticalSection::Data создается с признаком
// DontDestroyOnDestruction который припятствует "преждевременному"
// разрушению внутреннего объекта.
// Это поможет избежать проблемы, когда какой-либо объект, разрушаемый при
// завершении программы, обращается к критической секции, которая была
// разрушена раньше (программа в этом случае выдает исключение, что раздражает).
//
#define ENTER_CRITICAL_SECTION { static SCriticalSection::Data __csd(1); SCriticalSection __cs(__csd);
#define LEAVE_CRITICAL_SECTION }
#define DO_CRITICAL(op) { static SCriticalSection::Data __csd(1); SCriticalSection __cs(__csd); op; }
//
// Descr: Перечисление, обеспечивающее типизированное применение "лекого" варианта конструктора
//   объекта. Применяется в специальных случаях, когда необходимо максимально быстро создать
//   упрощенный 'кземпляр "тяжелого" объекта.
//
enum SCtrLite {
	SConstructorDef = 0,
	SConstructorLite = 1
};
//
// Классификатор систем программной инфраструктуры
// Это - не операционные системы, а именно инфраструктурные понятия, в рамках которых
// могут быть определены те или иные стандарты и классификаторы (например, кодовые страницы)
//
enum {
	ssisUnkn    = 0,
	ssisWindows = 1,
	ssisMAC     = 2,
	ssisIBM     = 3,
	ssisAIX     = 4,
	ssisJava    = 5,
	ssisSolaris = 6,
	ssisHPUX    = 7,
	ssisGLibC   = 8
};
//
// Descr: Возвращает идентификатор системы программной инфраструктуры ssisXXX
//   по символу pSymb.
//
int    FASTCALL RecognizeSisSymb(const char * pSymb);
//
// Descr: Возвращает в буфере rCode канонический символ системы программной инфраструктуты ident[ssisXXX]
//
int    FASTCALL GetSisCode(int ident, SString & rCode);
//
// Идентификаторы языков
//
enum { // @persistent
	slangMeta = 1,
	slangLA, // LATIN
	slangEN, // ENGLISH
	slangRU, // RUSSIAN
	slangDE, // GERMAN
	slangAA, // AFAR
	slangAB, // ABKHAZIAN
	slangACE, // ACHINESE
	slangACH, // ACOLI
	slangADA, // ADANGME
	slangADY, // ADYGHE
	slangAE, // AVESTAN
	slangAF, // AFRIKAANS
	slangAFA, // AFRO-ASIATIC LANGUAGE
	slangAFH, // AFRIHILI
	slangAGQ, // AGHEM
	slangAIN, // AINU
	slangAK, // AKAN
	slangAKK, // AKKADIAN
	slangALE, // ALEUT
	slangALG, // ALGONQUIAN LANGUAGE
	slangALT, // SOUTHERN ALTAI
	slangAM, // AMHARIC
	slangAN, // ARAGONESE
	slangANG, // OLD ENGLISH
	slangANP, // ANGIKA
	slangAPA, // APACHE LANGUAGE
	slangAR, // ARABIC
	slangARC, // ARAMAIC
	slangARN, // ARAUCANIAN
	slangARP, // ARAPAHO
	slangART, // ARTIFICIAL LANGUAGE
	slangARW, // ARAWAK
	slangAS, // ASSAMESE
	slangASA, // ASU
	slangAST, // ASTURIAN
	slangATH, // ATHAPASCAN LANGUAGE
	slangAUS, // AUSTRALIAN LANGUAGE
	slangAV, // AVARIC
	slangAWA, // AWADHI
	slangAY, // AYMARA
	slangAZ, // AZERBAIJANI
	slangBA, // BASHKIR
	slangBAD, // BANDA
	slangBAI, // BAMILEKE LANGUAGE
	slangBAL, // BALUCHI
	slangBAN, // BALINESE
	slangBAS, // BASAA
	slangBAT, // BALTIC LANGUAGE
	slangBE, // BELARUSIAN
	slangBEJ, // BEJA
	slangBEM, // BEMBA
	slangBER, // BERBER
	slangBEZ, // BENA
	slangBG, // BULGARIAN
	slangBH, // BIHARI
	slangBHO, // BHOJPURI
	slangBI, // BISLAMA
	slangBIK, // BIKOL
	slangBIN, // BINI
	slangBLA, // SIKSIKA
	slangBM, // BAMBARA
	slangBN, // BENGALI
	slangBNT, // BANTU
	slangBO, // TIBETAN
	slangBR, // BRETON
	slangBRA, // BRAJ
	slangBRX, // BODO
	slangBS, // BOSNIAN
	slangBTK, // BATAK
	slangBUA, // BURIAT
	slangBUG, // BUGINESE
	slangBYN, // BLIN
	slangCA, // CATALAN
	slangCAD, // CADDO
	slangCAI, // CENTRAL AMERICAN INDIAN LANGUAGE
	slangCAR, // CARIB
	slangCAU, // CAUCASIAN LANGUAGE
	slangCAY, // CAYUGA
	slangCCH, // ATSAM
	slangCE, // CHECHEN
	slangCEB, // CEBUANO
	slangCEL, // CELTIC LANGUAGE
	slangCGG, // CHIGA
	slangCH, // CHAMORRO
	slangCHB, // CHIBCHA
	slangCHG, // CHAGATAI
	slangCHK, // CHUUKESE
	slangCHM, // MARI
	slangCHN, // CHINOOK JARGON
	slangCHO, // CHOCTAW
	slangCHP, // CHIPEWYAN
	slangCHR, // CHEROKEE
	slangCHY, // CHEYENNE
	slangCMC, // CHAMIC LANGUAGE
	slangCO, // CORSICAN
	slangCOP, // COPTIC
	slangCPE, // ENGLISH-BASED CREOLE OR PIDGIN
	slangCPF, // FRENCH-BASED CREOLE OR PIDGIN
	slangCPP, // PORTUGUESE-BASED CREOLE OR PIDGIN
	slangCR, // CREE
	slangCRH, // CRIMEAN TURKISH
	slangCRP, // CREOLE OR PIDGIN
	slangCS, // CZECH
	slangCSB, // KASHUBIAN
	slangCU, // CHURCH SLAVIC
	slangCUS, // CUSHITIC LANGUAGE
	slangCV, // CHUVASH
	slangCY, // WELSH
	slangDA, // DANISH
	slangDAK, // DAKOTA
	slangDAR, // DARGWA
	slangDAV, // TAITA
	slangDAY, // DAYAK
	slangDE_AT, // AUSTRIAN GERMAN
	slangDE_CH, // SWISS HIGH GERMAN
	slangDEL, // DELAWARE
	slangDEN, // SLAVE
	slangDGR, // DOGRIB
	slangDIN, // DINKA
	slangDJE, // ZARMA
	slangDOI, // DOGRI
	slangDRA, // DRAVIDIAN LANGUAGE
	slangDSB, // LOWER SORBIAN
	slangDUA, // DUALA
	slangDUM, // MIDDLE DUTCH
	slangDV, // DIVEHI
	slangDYO, // JOLA-FONYI
	slangDYU, // DYULA
	slangDZ, // DZONGKHA
	slangEBU, // EMBU
	slangEE, // EWE
	slangEFI, // EFIK
	slangEGY, // ANCIENT EGYPTIAN
	slangEKA, // EKAJUK
	slangEL, // GREEK
	slangELX, // ELAMITE
	slangEN_AU, // AUSTRALIAN ENGLISH
	slangEN_CA, // CANADIAN ENGLISH
	slangEN_GB, // BRITISH ENGLISH
	slangEN_US, // U.S. ENGLISH
	slangENM, // MIDDLE ENGLISH
	slangEO, // ESPERANTO
	slangES, // SPANISH
	slangES_419, // LATIN AMERICAN SPANISH
	slangES_ES, // IBERIAN SPANISH
	slangET, // ESTONIAN
	slangEU, // BASQUE
	slangEWO, // EWONDO
	slangFA, // PERSIAN
	slangFAN, // FANG
	slangFAT, // FANTI
	slangFF, // FULAH
	slangFI, // FINNISH
	slangFIL, // FILIPINO
	slangFIU, // FINNO-UGRIAN LANGUAGE
	slangFJ, // FIJIAN
	slangFO, // FAROESE
	slangFON, // FON
	slangFR, // FRENCH
	slangFR_CA, // CANADIAN FRENCH
	slangFR_CH, // SWISS FRENCH
	slangFRM, // MIDDLE FRENCH
	slangFRO, // OLD FRENCH
	slangFRR, // NORTHERN FRISIAN
	slangFRS, // EASTERN FRISIAN
	slangFUR, // FRIULIAN
	slangFY, // WESTERN FRISIAN
	slangGA, // IRISH
	slangGAA, // GA
	slangGAY, // GAYO
	slangGBA, // GBAYA
	slangGD, // SCOTTISH GAELIC
	slangGEM, // GERMANIC LANGUAGE
	slangGEZ, // GEEZ
	slangGIL, // GILBERTESE
	slangGL, // GALICIAN
	slangGMH, // MIDDLE HIGH GERMAN
	slangGN, // GUARANI
	slangGOH, // OLD HIGH GERMAN
	slangGON, // GONDI
	slangGOR, // GORONTALO
	slangGOT, // GOTHIC
	slangGRB, // GREBO
	slangGRC, // ANCIENT GREEK
	slangGSW, // SWISS GERMAN
	slangGU, // GUJARATI
	slangGUZ, // GUSII
	slangGV, // MANX
	slangGWI, // GWICH?IN
	slangHA, // HAUSA
	slangHAI, // HAIDA
	slangHAW, // HAWAIIAN
	slangHE, // HEBREW
	slangHI, // HINDI
	slangHIL, // HILIGAYNON
	slangHIM, // HIMACHALI
	slangHIT, // HITTITE
	slangHMN, // HMONG
	slangHO, // HIRI MOTU
	slangHR, // CROATIAN
	slangHSB, // UPPER SORBIAN
	slangHT, // HAITIAN
	slangHU, // HUNGARIAN
	slangHUP, // HUPA
	slangHY, // ARMENIAN
	slangHZ, // HERERO
	slangIA, // INTERLINGUA
	slangIBA, // IBAN
	slangID, // INDONESIAN
	slangIE, // INTERLINGUE
	slangIG, // IGBO
	slangII, // SICHUAN YI
	slangIJO, // IJO
	slangIK, // INUPIAQ
	slangILO, // ILOKO
	slangINC, // INDIC LANGUAGE
	slangINE, // INDO-EUROPEAN LANGUAGE
	slangINH, // INGUSH
	slangIO, // IDO
	slangIRA, // IRANIAN LANGUAGE
	slangIRO, // IROQUOIAN LANGUAGE
	slangIS, // ICELANDIC
	slangIT, // ITALIAN
	slangIU, // INUKTITUT
	slangJA, // JAPANESE
	slangJBO, // LOJBAN
	slangJMC, // MACHAME
	slangJPR, // JUDEO-PERSIAN
	slangJRB, // JUDEO-ARABIC
	slangJV, // JAVANESE
	slangKA, // GEORGIAN
	slangKAA, // KARA-KALPAK
	slangKAB, // KABYLE
	slangKAC, // KACHIN
	slangKAJ, // JJU
	slangKAM, // KAMBA
	slangKAR, // KAREN
	slangKAW, // KAWI
	slangKBD, // KABARDIAN
	slangKCG, // TYAP
	slangKDE, // MAKONDE
	slangKEA, // KABUVERDIANU
	slangKFO, // KORO
	slangKG, // KONGO
	slangKHA, // KHASI
	slangKHI, // KHOISAN LANGUAGE
	slangKHO, // KHOTANESE
	slangKHQ, // KOYRA CHIINI
	slangKI, // KIKUYU
	slangKJ, // KUANYAMA
	slangKK, // KAZAKH
	slangKL, // KALAALLISUT
	slangKLN, // KALENJIN
	slangKM, // KHMER
	slangKMB, // KIMBUNDU
	slangKN, // KANNADA
	slangKO, // KOREAN
	slangKOK, // KONKANI
	slangKOS, // KOSRAEAN
	slangKPE, // KPELLE
	slangKR, // KANURI
	slangKRC, // KARACHAY-BALKAR
	slangKRL, // KARELIAN
	slangKRO, // KRU
	slangKRU, // KURUKH
	slangKS, // KASHMIRI
	slangKSB, // SHAMBALA
	slangKSF, // BAFIA
	slangKSH, // COLOGNIAN
	slangKU, // KURDISH
	slangKUM, // KUMYK
	slangKUT, // KUTENAI
	slangKV, // KOMI
	slangKW, // CORNISH
	slangKY, // KIRGHIZ
	slangLAD, // LADINO
	slangLAG, // LANGI
	slangLAH, // LAHNDA
	slangLAM, // LAMBA
	slangLB, // LUXEMBOURGISH
	slangLEZ, // LEZGHIAN
	slangLG, // GANDA
	slangLI, // LIMBURGISH
	slangLN, // LINGALA
	slangLO, // LAO
	slangLOL, // MONGO
	slangLOZ, // LOZI
	slangLT, // LITHUANIAN
	slangLU, // LUBA-KATANGA
	slangLUA, // LUBA-LULUA
	slangLUI, // LUISENO
	slangLUN, // LUNDA
	slangLUO, // LUO
	slangLUS, // LUSHAI
	slangLUY, // LUYIA
	slangLV, // LATVIAN
	slangMAD, // MADURESE
	slangMAG, // MAGAHI
	slangMAI, // MAITHILI
	slangMAK, // MAKASAR
	slangMAN, // MANDINGO
	slangMAP, // AUSTRONESIAN LANGUAGE
	slangMAS, // MASAI
	slangMDF, // MOKSHA
	slangMDR, // MANDAR
	slangMEN, // MENDE
	slangMER, // MERU
	slangMFE, // MORISYEN
	slangMG, // MALAGASY
	slangMGA, // MIDDLE IRISH
	slangMGH, // MAKHUWA-MEETTO
	slangMH, // MARSHALLESE
	slangMI, // MAORI
	slangMIC, // MICMAC
	slangMIN, // MINANGKABAU
	slangMIS, // MISCELLANEOUS LANGUAGE
	slangMK, // MACEDONIAN
	slangMKH, // MON-KHMER LANGUAGE
	slangML, // MALAYALAM
	slangMN, // MONGOLIAN
	slangMNC, // MANCHU
	slangMNI, // MANIPURI
	slangMNO, // MANOBO LANGUAGE
	slangMO, // MOLDAVIAN
	slangMOH, // MOHAWK
	slangMOS, // MOSSI
	slangMR, // MARATHI
	slangMS, // MALAY
	slangMT, // MALTESE
	slangMUA, // MUNDANG
	slangMUL, // MULTIPLE LANGUAGES
	slangMUN, // MUNDA LANGUAGE
	slangMUS, // CREEK
	slangMWL, // MIRANDESE
	slangMWR, // MARWARI
	slangMY, // BURMESE
	slangMYN, // MAYAN LANGUAGE
	slangMYV, // ERZYA
	slangNA, // NAURU
	slangNAH, // NAHUATL
	slangNAI, // NORTH AMERICAN INDIAN LANGUAGE
	slangNAP, // NEAPOLITAN
	slangNAQ, // NAMA
	slangNB, // NORWEGIAN BOKMAL
	slangND, // NORTH NDEBELE
	slangNDS, // LOW GERMAN
	slangNE, // NEPALI
	slangNEW, // NEWARI
	slangNG, // NDONGA
	slangNIA, // NIAS
	slangNIC, // NIGER-KORDOFANIAN LANGUAGE
	slangNIU, // NIUEAN
	slangNL, // DUTCH
	slangNL_BE, // FLEMISH
	slangNMG, // KWASIO
	slangNN, // NORWEGIAN NYNORSK
	slangNO, // NORWEGIAN
	slangNOG, // NOGAI
	slangNON, // OLD NORSE
	slangNQO, // N’KO
	slangNR, // SOUTH NDEBELE
	slangNSO, // NORTHERN SOTHO
	slangNUB, // NUBIAN LANGUAGE
	slangNUS, // NUER
	slangNV, // NAVAJO
	slangNWC, // CLASSICAL NEWARI
	slangNY, // NYANJA
	slangNYM, // NYAMWEZI
	slangNYN, // NYANKOLE
	slangNYO, // NYORO
	slangNZI, // NZIMA
	slangOC, // OCCITAN
	slangOJ, // OJIBWA
	slangOM, // OROMO
	slangOR, // ORIYA
	slangOS, // OSSETIC
	slangOSA, // OSAGE
	slangOTA, // OTTOMAN TURKISH
	slangOTO, // OTOMIAN LANGUAGE
	slangPA, // PUNJABI
	slangPAA, // PAPUAN LANGUAGE
	slangPAG, // PANGASINAN
	slangPAL, // PAHLAVI
	slangPAM, // PAMPANGA
	slangPAP, // PAPIAMENTO
	slangPAU, // PALAUAN
	slangPEO, // OLD PERSIAN
	slangPHI, // PHILIPPINE LANGUAGE
	slangPHN, // PHOENICIAN
	slangPI, // PALI
	slangPL, // POLISH
	slangPON, // POHNPEIAN
	slangPRA, // PRAKRIT LANGUAGE
	slangPRO, // OLD PROVENCAL
	slangPS, // PASHTO
	slangPT, // PORTUGUESE
	slangPT_BR, // BRAZILIAN PORTUGUESE
	slangPT_PT, // IBERIAN PORTUGUESE
	slangQU, // QUECHUA
	slangRAJ, // RAJASTHANI
	slangRAP, // RAPANUI
	slangRAR, // RAROTONGAN
	slangRM, // ROMANSH
	slangRN, // RUNDI
	slangRO, // ROMANIAN
	slangROA, // ROMANCE LANGUAGE
	slangROF, // ROMBO
	slangROM, // ROMANY
	slangROOT, // ROOT
	slangRUP, // AROMANIAN
	slangRW, // KINYARWANDA
	slangRWK, // RWA
	slangSA, // SANSKRIT
	slangSAD, // SANDAWE
	slangSAH, // SAKHA
	slangSAI, // SOUTH AMERICAN INDIAN LANGUAGE
	slangSAL, // SALISHAN LANGUAGE
	slangSAM, // SAMARITAN ARAMAIC
	slangSAQ, // SAMBURU
	slangSAS, // SASAK
	slangSAT, // SANTALI
	slangSBP, // SANGU
	slangSC, // SARDINIAN
	slangSCN, // SICILIAN
	slangSCO, // SCOTS
	slangSD, // SINDHI
	slangSE, // NORTHERN SAMI
	slangSEE, // SENECA
	slangSEH, // SENA
	slangSEL, // SELKUP
	slangSEM, // SEMITIC LANGUAGE
	slangSES, // KOYRABORO SENNI
	slangSG, // SANGO
	slangSGA, // OLD IRISH
	slangSGN, // SIGN LANGUAGE
	slangSH, // SERBO-CROATIAN
	slangSHI, // TACHELHIT
	slangSHN, // SHAN
	slangSI, // SINHALA
	slangSID, // SIDAMO
	slangSIO, // SIOUAN LANGUAGE
	slangSIT, // SINO-TIBETAN LANGUAGE
	slangSK, // SLOVAK
	slangSL, // SLOVENIAN
	slangSLA, // SLAVIC LANGUAGE
	slangSM, // SAMOAN
	slangSMA, // SOUTHERN SAMI
	slangSMI, // SAMI LANGUAGE
	slangSMJ, // LULE SAMI
	slangSMN, // INARI SAMI
	slangSMS, // SKOLT SAMI
	slangSN, // SHONA
	slangSNK, // SONINKE
	slangSO, // SOMALI
	slangSOG, // SOGDIEN
	slangSON, // SONGHAI
	slangSQ, // ALBANIAN
	slangSR, // SERBIAN
	slangSRN, // SRANAN TONGO
	slangSRR, // SERER
	slangSS, // SWATI
	slangSSA, // NILO-SAHARAN LANGUAGE
	slangSSY, // SAHO
	slangST, // SOUTHERN SOTHO
	slangSU, // SUNDANESE
	slangSUK, // SUKUMA
	slangSUS, // SUSU
	slangSUX, // SUMERIAN
	slangSV, // SWEDISH
	slangSW, // SWAHILI
	slangSWB, // COMORIAN
	slangSWC, // CONGO SWAHILI
	slangSYC, // CLASSICAL SYRIAC
	slangSYR, // SYRIAC
	slangTA, // TAMIL
	slangTAI, // TAI LANGUAGE
	slangTE, // TELUGU
	slangTEM, // TIMNE
	slangTEO, // TESO
	slangTER, // TERENO
	slangTET, // TETUM
	slangTG, // TAJIK
	slangTH, // THAI
	slangTI, // TIGRINYA
	slangTIG, // TIGRE
	slangTIV, // TIV
	slangTK, // TURKMEN
	slangTKL, // TOKELAU
	slangTL, // TAGALOG
	slangTLH, // KLINGON
	slangTLI, // TLINGIT
	slangTMH, // TAMASHEK
	slangTN, // TSWANA
	slangTO, // TONGAN
	slangTOG, // NYASA TONGA
	slangTPI, // TOK PISIN
	slangTR, // TURKISH
	slangTRV, // TAROKO
	slangTS, // TSONGA
	slangTSI, // TSIMSHIAN
	slangTT, // TATAR
	slangTUM, // TUMBUKA
	slangTUP, // TUPI LANGUAGE
	slangTUT, // ALTAIC LANGUAGE
	slangTVL, // TUVALU
	slangTW, // TWI
	slangTWQ, // TASAWAQ
	slangTY, // TAHITIAN
	slangTYV, // TUVINIAN
	slangTZM, // CENTRAL MOROCCO TAMAZIGHT
	slangUDM, // UDMURT
	slangUG, // UIGHUR
	slangUGA, // UGARITIC
	slangUK, // UKRAINIAN
	slangUMB, // UMBUNDU
	slangUND, // UNKNOWN LANGUAGE
	slangUR, // URDU
	slangUZ, // UZBEK
	slangVAI, // VAI
	slangVE, // VENDA
	slangVI, // VIETNAMESE
	slangVO, // VOLAPUK
	slangVOT, // VOTIC
	slangVUN, // VUNJO
	slangWA, // WALLOON
	slangWAE, // WALSER
	slangWAK, // WAKASHAN LANGUAGE
	slangWAL, // WALAMO
	slangWAR, // WARAY
	slangWAS, // WASHO
	slangWEN, // SORBIAN LANGUAGE
	slangWO, // WOLOF
	slangXAL, // KALMYK
	slangXH, // XHOSA
	slangXOG, // SOGA
	slangYAO, // YAO
	slangYAP, // YAPESE
	slangYAV, // YANGBEN
	slangYI, // YIDDISH
	slangYO, // YORUBA
	slangYPK, // YUPIK LANGUAGE
	slangYUE, // CANTONESE
	slangZA, // ZHUANG
	slangZAP, // ZAPOTEC
	slangZBL, // BLISSYMBOLS
	slangZEN, // ZENAGA
	slangZH, // CHINESE
	slangZH_HANS, // SIMPLIFIED CHINESE
	slangZH_HANT, // TRADITIONAL CHINESE
	slangZND, // ZANDE
	slangZU, // ZULU
	slangZUN, // ZUNI
	slangZXX, // NO LINGUISTIC CONTENT
	slangZZA // ZAZA
};
//
// Descr: Идентификаторы натуральных скриптов
//
enum {
	snscriptUnkn = 0,
	snscriptMeta = 1,
	snscriptLatin,
	snscriptGreek,
	snscriptCyrillic,
	snscriptArmenian,
	snscriptHebrew,
	snscriptArabic,
	snscriptSyriac,
	snscriptThaana,
	snscriptNko,
	snscriptSamaritan,
	snscriptMandaic,
	snscriptDevanagari,
	snscriptBengali,
	snscriptGurmukhi,
	snscriptGujarati,
	snscriptOriya,
	snscriptTamil,
	snscriptTelugu,
	snscriptKannada,
	snscriptMalayalam,
	snscriptSinhala,
	snscriptThai,
	snscriptLao,
	snscriptTibetan,
	snscriptMyanmar,
	snscriptGeorgian,
    snscriptHangul,   // Хангыль Корейский алфавит
    snscriptEthiopic,
    snscriptCherokee,
    snscriptMongolian,
    snscriptKhmer,
    snscriptLimbu,
    snscriptBuginese,
    snscriptBalinese,
    snscriptSundanese,
    snscriptBatak,
    snscriptLepcha,
    snscriptHiragana,
    snscriptKatakana,
    snscriptBopomofo,
    snscriptKharoshthi,
    snscriptManichaean,
    snscriptAvestan,
    snscriptBrahmi,
    snscriptKaithi,
    snscriptChakma,
    snscriptMahajani,
    snscriptSharada,
    snscriptKhojki,
    snscriptGlagolitic,
    snscriptCanadianSyllabics,
    snscriptTaiLe,
    snscriptNewTaiLue,
    snscriptTaiTham,
    snscriptTaiViet,
    snscriptGothic,
    snscriptUgaritic,
    snscriptMathematical,
    snscriptArabicMathematical,
    snscriptYi,
    snscriptCJK,
    snscriptSquaredCJK,
    snscriptBoxDrawings
};
//
// Descr: Возвращает идентификатор языка по международному коду pSymb.
//
int    FASTCALL RecognizeLinguaSymb(const char * pSymb, int word);
//
// Descr: Возвращает в буфере rCode международный код языка по идентификатору ident.
//
int    FASTCALL GetLinguaCode(int ident, SString & rCode);
//
// Descr: Возвращает Windows код языка по идентификатору ident.
//
uint32 FASTCALL GetLinguaWinIdent(int ident);
int    FASTCALL GetLinguaList(LongArray & rList);
//
// Descr: Возвращает идентификатор натурального скрипта по английскому наименованию pSymb.
//   Внутренняя таблица символов скриптов "заточена" на соответствие со стандартной таблицей unicode-символов.
//
int    FASTCALL RecognizeSNScriptSymb(const char * pSymb, size_t * pLength);
//
// Descr: Возвращает символ текстовое наименование натурального скрипта в буфере rCode по значению
//   идентификатора ident (snscriptXXX).
//
int    FASTCALL GetSNScriptCode(int ident, SString & rCode);

enum SCodepage {
	cpUndef    =  -1,
	cpANSI    =    0,  // == CP_ACP
	cpOEM     =    1,  // == CP_OEMCP

	cp437     =  437,  // 437 US MSDOS
	cp737     =  737,  // 737 Greek MSDOS
	cp850     =  850,  // 850 International MSDOS
	cp852     =  852,  // 852 EasernEuropean MSDOS
	cp857     =  857,  // 857 Turkish MSDOS
	cp861     =  861,  // 861 Icelandic MSDOS
	cp865     =  865,  // 865 Nordic MSDOS
	cp866     =  866,  // 866 Russian MSDOS
	cp932     =  932,  // 932 Japanese Windows
	cp936     =  936,  // 936 Chinese Windows
	cp950     =  950,  // 950 Chinese Windows
	cp1250    = 1250,  // 1250 Eastern European Windows
	cp1251    = 1251,  // 1251 Russian Windows
	cp1252    = 1252,  // 1252 Windows ANSI
	cp1253    = 1253,  // 1253 Greek Windows
	cp1254    = 1254,  // 1254 Turkish Windows
	cp1255    = 1255,  // 1255 Hebrew Windows
	cp1256    = 1256,  // 1256 Arabic Windows

	cpUTF7    = 65000, // == CP_UTF7
	cpUTF8    = 65001, // == CP_UTF8
	cpUTF16   =  1200, // utf-16      Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications
	cpUTF16BE =  1201, // unicodeFFFE Unicode UTF-16, big endian byte order; available only to managed applications
	cpUTF32   = 12000, // utf-32      Unicode UTF-32, little endian byte order; available only to managed applications
	cpUTF32BE = 12001, // utf-32BE    Unicode UTF-32, big endian byte order; available only to managed applications

	// Identifier	.NET Name	Additional information
	cp037   =   37, // IBM037 IBM EBCDIC US-Canada
	cp500   =  500, // IBM500 IBM EBCDIC International
	cp708   =  708, // ASMO-708 Arabic (ASMO 708)
	cp709   =  709, // Arabic (ASMO-449+, BCON V4)
	cp710   =  710, // Arabic - Transparent Arabic
	cp720   =  720, // DOS-720 Arabic (Transparent ASMO); Arabic (DOS)
	//cp737   =  737, // ibm737 OEM Greek (formerly 437G); Greek (DOS)
	cp775   =  775, // ibm775	OEM Baltic; Baltic (DOS)
	//cp850   =  850, // ibm850	OEM Multilingual Latin 1; Western European (DOS)
	//cp852   =  852, // ibm852	OEM Latin 2; Central European (DOS)
	cp855   =  855, // IBM855	OEM Cyrillic (primarily Russian)
	//cp857   =  857, // ibm857	OEM Turkish; Turkish (DOS)
	cp858   =  858, // IBM00858	OEM Multilingual Latin 1 + Euro symbol
	cp860   =  860, // IBM860	OEM Portuguese; Portuguese (DOS)
	//cp861   =  861, // ibm861	OEM Icelandic; Icelandic (DOS)
	cp862   =  862, // DOS-862	OEM Hebrew; Hebrew (DOS)
	cp863   =  863, // IBM863	OEM French Canadian; French Canadian (DOS)
	cp864   =  864, // IBM864	OEM Arabic; Arabic (864)
	//cp865   =  865, // IBM865	OEM Nordic; Nordic (DOS)
	//cp866   =  866, // cp866   = 866, // OEM Russian; Cyrillic (DOS)
	cp869   =  869, // ibm869	OEM Modern Greek; Greek, Modern (DOS)
	cp870   =  870, // IBM870	IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2
	cp874   =  874, // windows-874	ANSI/OEM Thai (ISO 8859-11); Thai (Windows)
	cp875   =  875, // cp875   = 875, // IBM EBCDIC Greek Modern
	//cp932   =  932, // shift_jis	ANSI/OEM Japanese; Japanese (Shift-JIS)
	//cp936   =  936, // gb2312	ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
	cp949   =  949, // ks_c_5601-1987	ANSI/OEM Korean (Unified Hangul Code)
	//cp950   =  950, // big5	ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
	cp1026  = 1026, // IBM1026	IBM EBCDIC Turkish (Latin 5)
	cp1047  = 1047, // IBM01047	IBM EBCDIC Latin 1/Open System
	cp1140  = 1140, // IBM01140	IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)
	cp1141  = 1141, // IBM01141	IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)
	cp1142  = 1142, // IBM01142	IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)
	cp1143  = 1143, // IBM01143	IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)
	cp1144  = 1144, // IBM01144	IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)
	cp1145  = 1145, // IBM01145	IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)
	cp1146  = 1146, // IBM01146	IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)
	cp1147  = 1147, // IBM01147	IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)
	cp1148  = 1148, // IBM01148	IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)
	cp1149  = 1149, // IBM01149	IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)
	//cp1250  = 1250, // windows-1250	ANSI Central European; Central European (Windows)
	//cp1251  = 1251, // windows-1251	ANSI Cyrillic; Cyrillic (Windows)
	//cp1252  = 1252, // windows-1252	ANSI Latin 1; Western European (Windows)
	//cp1253  = 1253, // windows-1253	ANSI Greek; Greek (Windows)
	//cp1254  = 1254, // windows-1254	ANSI Turkish; Turkish (Windows)
	//cp1255  = 1255, // windows-1255	ANSI Hebrew; Hebrew (Windows)
	//cp1256  = 1256, // windows-1256	ANSI Arabic; Arabic (Windows)
	cp1257  =  1257, // windows-1257	ANSI Baltic; Baltic (Windows)
	cp1258  =  1258, // windows-1258	ANSI/OEM Vietnamese; Vietnamese (Windows)
	cp1361  =  1361, // Johab	Korean (Johab)
	cp10000 = 10000, // macintosh	MAC Roman; Western European (Mac)
	cp10001 = 10001, // x-mac-japanese	Japanese (Mac)
	cp10002 = 10002, // x-mac-chinesetrad	MAC Traditional Chinese (Big5); Chinese Traditional (Mac)
	cp10003 = 10003, // x-mac-korean	Korean (Mac)
	cp10004 = 10004, // x-mac-arabic	Arabic (Mac)
	cp10005 = 10005, // x-mac-hebrew	Hebrew (Mac)
	cp10006 = 10006, // x-mac-greek	Greek (Mac)
	cp10007 = 10007, // x-mac-cyrillic	Cyrillic (Mac)
	cp10008 = 10008, // x-mac-chinesesimp	MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)
	cp10010 = 10010, // x-mac-romanian	Romanian (Mac)
	cp10017 = 10017, // x-mac-ukrainian	Ukrainian (Mac)
	cp10021 = 10021, // x-mac-thai	Thai (Mac)
	cp10029 = 10029, // x-mac-ce	MAC Latin 2; Central European (Mac)
	cp10079 = 10079, // x-mac-icelandic	Icelandic (Mac)
	cp10081 = 10081, // x-mac-turkish	Turkish (Mac)
	cp10082 = 10082, // x-mac-croatian	Croatian (Mac)
	cp20000 = 20000, // x-Chinese_CNS	CNS Taiwan; Chinese Traditional (CNS)
	cp20001 = 20001, // x-cp20001	TCA Taiwan
	cp20002 = 20002, // x_Chinese-Eten	Eten Taiwan; Chinese Traditional (Eten)
	cp20003 = 20003, // x-cp20003	IBM5550 Taiwan
	cp20004 = 20004, // x-cp20004	TeleText Taiwan
	cp20005 = 20005, // x-cp20005	Wang Taiwan
	cp20105 = 20105, // x-IA5	IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)
	cp20106 = 20106, // x-IA5-German	IA5 German (7-bit)
	cp20107 = 20107, // x-IA5-Swedish	IA5 Swedish (7-bit)
	cp20108 = 20108, // x-IA5-Norwegian	IA5 Norwegian (7-bit)
	cp20127 = 20127, // us-ascii	US-ASCII (7-bit)
	cp20261 = 20261, // x-cp20261	T.61
	cp20269 = 20269, // x-cp20269	ISO 6937 Non-Spacing Accent
	cp20273 = 20273, // IBM273	IBM EBCDIC Germany
	cp20277 = 20277, // IBM277	IBM EBCDIC Denmark-Norway
	cp20278 = 20278, // IBM278	IBM EBCDIC Finland-Sweden
	cp20280 = 20280, // IBM280	IBM EBCDIC Italy
	cp20284 = 20284, // IBM284	IBM EBCDIC Latin America-Spain
	cp20285 = 20285, // IBM285	IBM EBCDIC United Kingdom
	cp20290 = 20290, // IBM290	IBM EBCDIC Japanese Katakana Extended
	cp20297 = 20297, // IBM297	IBM EBCDIC France
	cp20420 = 20420, // IBM420	IBM EBCDIC Arabic
	cp20423 = 20423, // IBM423	IBM EBCDIC Greek
	cp20424 = 20424, // IBM424	IBM EBCDIC Hebrew
	cp20833 = 20833, // x-EBCDIC-KoreanExtended	IBM EBCDIC Korean Extended
	cp20838 = 20838, // IBM-Thai	IBM EBCDIC Thai
	cp20866 = 20866, // koi8-r	Russian (KOI8-R); Cyrillic (KOI8-R)
	cp20871 = 20871, // IBM871	IBM EBCDIC Icelandic
	cp20880 = 20880, // IBM880	IBM EBCDIC Cyrillic Russian
	cp20905 = 20905, // IBM905	IBM EBCDIC Turkish
	cp20924 = 20924, // IBM00924	IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)
	cp20932 = 20932, // EUC-JP	Japanese (JIS 0208-1990 and 0212-1990)
	cp20936 = 20936, // x-cp20936	Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)
	cp20949 = 20949, // x-cp20949	Korean Wansung
	cp21025 = 21025, // cp1025	IBM EBCDIC Cyrillic Serbian-Bulgarian
	cp21027 = 21027, // 	(deprecated)
	cp21866 = 21866, // koi8-u	Ukrainian (KOI8-U); Cyrillic (KOI8-U)
	cp28591 = 28591, // iso-8859-1	ISO 8859-1 Latin 1; Western European (ISO)
	cp28592 = 28592, // iso-8859-2	ISO 8859-2 Central European; Central European (ISO)
	cp28593 = 28593, // iso-8859-3	ISO 8859-3 Latin 3
	cp28594 = 28594, // iso-8859-4	ISO 8859-4 Baltic
	cp28595 = 28595, // iso-8859-5	ISO 8859-5 Cyrillic
	cp28596 = 28596, // iso-8859-6	ISO 8859-6 Arabic
	cp28597 = 28597, // iso-8859-7	ISO 8859-7 Greek
	cp28598 = 28598, // iso-8859-8	ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
	cp28599 = 28599, // iso-8859-9	ISO 8859-9 Turkish
	cp28603 = 28603, // iso-8859-13	ISO 8859-13 Estonian
	cp28605 = 28605, // iso-8859-15	ISO 8859-15 Latin 9
	cp29001 = 29001, // x-Europa	Europa 3
	cp38598 = 38598, // iso-8859-8-i	ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
	cp50220 = 50220, // iso-2022-jp	ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)
	cp50221 = 50221, // csISO2022JP	ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)
	cp50222 = 50222, // iso-2022-jp	ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)
	cp50225 = 50225, // iso-2022-kr	ISO 2022 Korean
	cp50227 = 50227, // x-cp50227	ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
	cp50229 = 50229, // 	ISO 2022 Traditional Chinese
	cp50930 = 50930, // 	EBCDIC Japanese (Katakana) Extended
	cp50931 = 50931, // 	EBCDIC US-Canada and Japanese
	cp50933 = 50933, // 	EBCDIC Korean Extended and Korean
	cp50935 = 50935, // 	EBCDIC Simplified Chinese Extended and Simplified Chinese
	cp50936 = 50936, // 	EBCDIC Simplified Chinese
	cp50937 = 50937, // 	EBCDIC US-Canada and Traditional Chinese
	cp50939 = 50939, // 	EBCDIC Japanese (Latin) Extended and Japanese
	cp51932 = 51932, // euc-jp	EUC Japanese
	cp51936 = 51936, // EUC-CN	EUC Simplified Chinese; Chinese Simplified (EUC)
	cp51949 = 51949, // euc-kr	EUC Korean
	cp51950 = 51950, // 	EUC Traditional Chinese
	cp52936 = 52936, // hz-gb-2312	HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
	cp54936 = 54936, // GB18030	Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
	cp57002 = 57002, // x-iscii-de	ISCII Devanagari
	cp57003 = 57003, // x-iscii-be	ISCII Bangla
	cp57004 = 57004, // x-iscii-ta	ISCII Tamil
	cp57005 = 57005, // x-iscii-te	ISCII Telugu
	cp57006 = 57006, // x-iscii-as	ISCII Assamese
	cp57007 = 57007, // x-iscii-or	ISCII Odia
	cp57008 = 57008, // x-iscii-ka	ISCII Kannada
	cp57009 = 57009, // x-iscii-ma	ISCII Malayalam
	cp57010 = 57010, // x-iscii-gu	ISCII Gujarati
	cp57011 = 57011  // x-iscii-pa	ISCII Punjabi
};

struct SCodepageIdent {

	static uint GetRegisteredCodepageCount();
	static int  GetRegisteredCodepage(uint idx, SCodepage & rCp, SString & rName);

	enum {
		fmtDefault = 0,
		fmtCLibLocale,
		fmtXML
	};
	SCodepageIdent();
	SCodepageIdent(int cp);
	operator int() const;
	operator SCodepage() const;
	SCodepageIdent & FASTCALL operator = (SCodepage cp);
	int    FASTCALL operator == (SCodepage cp) const;
	int    FASTCALL operator != (SCodepage cp) const;
	int    FASTCALL FromStr(const char * pStr);
	int    ToStr(int format, SString & rBuf) const;

	int32  Cp;
};
//
// Descr: Типы кодировок unicode-символов
//
enum SUnicodeMode {
	suni8Bit       = 0,
	suniUTF8       = 1,
	suni16BE       = 2,
	suni16LE       = 3,
	suniCookie     = 4,
	suni7Bit       = 5,
	suni16BE_NoBOM = 6,
	suni16LE_NoBOM = 7,
	suniEnd
};

class SUtfConst {
public:
	static const char   TrailingBytesForUTF8[256];
	static const uint32 OffsetsFromUTF8[6];
	static const uint8  FirstByteMark[7];

	static const uint32 HalfBase;
	static const uint32 HalfMask;
	static const int    HalfShift; // used for shifting by 10 bits
};
//
// Макро-константы конвертации UTF-символов
//
#define UNI_SUR_HIGH_START    (uint32)0xD800     // SURROGATE LEAD FIRST
#define UNI_SUR_HIGH_END      (uint32)0xDBFF     // SURROGATE LEAD LAST
#define UNI_SUR_LOW_START     (uint32)0xDC00     // SURROGATE TRAIL FIRST
#define UNI_SUR_LOW_END       (uint32)0xDFFF     // SURROGATE TRAIL LAST
#define UNI_REPLACEMENT_CHAR  (uint32)0x0000FFFD
#define UNI_MAX_BMP           (uint32)0x0000FFFF
#define UNI_MAX_UTF16         (uint32)0x0010FFFF
#define UNI_MAX_UTF32         (uint32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32   (uint32)0x0010FFFF
#define UNI_SUPPL_PLANE_START (uint32)0x10000    // SUPPLEMENTAL PLANE FIRST

class SUnicode {
public:
	static int   FASTCALL IsLegalUtf8(const uint8 * pSource, size_t length);
	//
	// Descr: Транслирует unicode32 в один или 2 широких символа wchar_t.
	// Returns:
	//   Количество широких символов, необходимое для того, чтобы вместить u32
	//
	static uint  FASTCALL Utf32ToUtf16(uint32 u32, wchar_t * pU16Buf);
};

enum SEOLFormat {
	eolUndef = 0,
	eolWindows,   // xDxA
	eolUnix,      // xA
	eolMac        // xD
};
//
//
//
class SHandle {
public:
	int operator !() const
	{
		return (P == 0);
	}
	SHandle & operator = (void * ptr)
	{
		P = ptr;
		return *this;
	}
	SHandle & operator = (uint32 i)
	{
		P = 0;
		I = i;
		return *this;
	}
	operator void * () const
	{
		return P;
	}
	operator uint32 () const
	{
		return I;
	}
private:
	void * P;
	uint32 I;
};
//
//
//
struct S_GUID {    // @persistent @size=16
	S_GUID & FASTCALL Init(REFIID);
	operator GUID & ();
	enum {
		fmtIDL = 0,
		fmtC = 1,
		fmtPlain     // "плоский" формат. то есть, без ограничителей и разделителей в 16-ричном формате
	};
	int    FASTCALL operator == (const S_GUID &) const;
	int    FASTCALL operator != (const S_GUID &) const;
	int    IsZero() const;
	void   SetZero();
	SString & ToStr(long fmt, SString &) const;
	int    FASTCALL FromStr(const char *);
	int    Generate();

	uint32 Data[4];
};
//
//
//
//
// Descr: Дата/время в формате Microsoft OLE (DATE)
//   The DATE type is implemented using an 8-byte floating-point number.
//   Days are represented by whole number increments starting with 30 December 1899, midnight as time zero.
//   Hour values are expressed as the absolute value of the fractional part of the number.
//   The following table illustrates this.
//
struct OleDate {
	// @noconstructor
	operator double() const { return v; }
	double v;
};
//
// Descr: Основное представление даты.
//   Два старших байта - год, средний байт - месяц, самый младший - день.
//   |yyyy|mm|dd|
//
struct LDATE {
	// @noconstructor
	enum {
		cZero = 0,         // Дата имеет нулевое значение
		cNormal,           // Дата имеет правильное актуальное значение
		cSpecial,          // Дата представлена в виде, трактуемом относительно заданной даты (see LDATE::getactual())
		cInvalid           // Дата имеет недопустимое значение
	};
	operator unsigned long() const { return v; }
	operator OleDate() const;
	LDATE  operator = (OleDate);
	LDATE  operator = (double); // OleDate
	int    day()   const { return (int)(v & 0x00ff); }
	int    month() const { return (int)((v & 0xff00) >> 8); }
	int    year()  const { return (int)(v >> 16); }
	int    weekno() const;
	int    dayspermonth() const;
	//
	// Descr: Возвращает класс, к которому относится дата.
	// Returns:
	//   Одно из значение LDATE::cXXX
	//
	int    getclass() const;
	int    FASTCALL setday(uint);
	int    FASTCALL setmonth(uint);
	int    FASTCALL setyear(uint);
	//
	// Descr: Сворачивает день, месяц и год в представление LDATE.
	//   Параметры d, m, y могут содержать относительные значения.
	//
	int    encode(int d, int m, int y);
	int    decode(int * pD, int * pM, int * pY) const;
	//
	// Descr: Возвращает эффективное значение даты. Если дата представлена
	//   относительным значение, то разворачивает его относительно даты rel.
	//   Если rel == ZERODATE, то разворачивает относительно getcurdate_().
	//   Если дата не содержит относительных факторов, то возвращает ее как есть.
	//
	LDATE  FASTCALL getactual(LDATE rel) const;
	LDATE  SLAPI getactualcmp(LDATE rel, LDATE cmp) const;
	//
	// Descr: Возвращает !0 если день, месяц или год содержат шаблонизированный ANY-признак.
	//
	int    SLAPI hasanycomponent() const;

	ulong  v;
private:
	LDATE  SLAPI Helper_GetActual(LDATE rel, LDATE cmp) const;
};
//
// Descr: Календарный день. Применяется для хранения регулярных дат.
//   Например, дни рождения, дни недели и т.д.
//
struct CALDATE : public LDATE {
	DECL_INVARIANT_C();

	enum {
		kDate      = 1, // обычная дата LDATE (либо v == 0 либо месяц и год не нулевые)
		kCalDate   = 2, // календарный день года (this->year() == 0)
		kDayOfWeek = 3  // день недели (this->day() >= 1 && this->day() <= 7 && this->month() == 0 && this->year() == 0)
	};
	CALDATE operator = (long);
	int    GetKind() const;
	int    IsDate(LDATE) const;
	SString & Format(int options, SString & rBuf) const;
	int    SetDate(LDATE);
	int    SetCalDate(int day, int mon);
	int    SetDayOfWeek(int dayOfWeek);
};
//
// Descr: Основное представление времени.
//   |h|m|s|.| (. - сотые доли секунды).
//   Таким образом, формат способен хранить время с точностью до 0.01 секунды.
//   Как следствие, для точного хрометража (например времени исполнения кода) этот
//   формат не годится, но для большинства остальных приложений очень хорош.
//
struct LTIME {
	DECL_INVARIANT_C();
	SLAPI  operator unsigned long() const { return v; }
	operator OleDate() const;
	LTIME  operator = (OleDate);
	LTIME  operator = (double); // OleDate
	LTIME  SLAPI encode(int h, int m, int s, int ms);
	int    SLAPI hour()  const { return (int)(((uint8 *)&v)[3]); }
	int    SLAPI minut() const { return (int)(((uint8 *)&v)[2]); }
	int    SLAPI sec()   const { return (int)(((uint8 *)&v)[1]); }
	int    SLAPI hs()    const { return (int)(((uint8 *)&v)[0]); }
	long   SLAPI totalsec() const { return (sec() + minut() * 60 + hour() * 3600); }
	//
	// Descr: Устанавливает время соответствии с количеством секунд, заданных параметром s.
	// Returns:
	//   Количество целых суток, содержащихся в s
	//
	long   FASTCALL settotalsec(long s);
	//
	// Descr: добавляет к значению времени n сотых долей секунды.
	//
	LTIME & FASTCALL addhs(long n);
	ulong  v;
};
//
// Descr: Представление рабочего дня //
//   Календарные даты представлены индексами количества календарных дней после WorkDate::InitDate.
//   Дни недели представлены индексами меньшими нуля и большими -100 [-1..-7].       {-день недели}
//   Даты, не зависящие от года представлены индексами дня меньшими или равными -100 {-(день/месяц/1996)-100}
//
//   Специализированное представление той же сущности, для которой применяется CALDATE.
//   Нюанс использования состоит в очень сжатой форме хранения данного класса (2 байта).
//
class WorkDate { // @size=2
public:
	// @noconstructor

	static int    FASTCALL ShrinkDate(LDATE dt);
	static LDATE  FASTCALL ExpandDate(int16 sdt);

	int    FASTCALL SetDate(LDATE dt);
	int    FASTCALL SetDayOfWeek(int dayOfWeek);
	int    SetDayOfYear(int day, int mon);
	LDATE  IsDate() const;
	int    IsDayOfWeek() const;
	LDATE  IsDayOfYear() const;
	int    FASTCALL IsEqual(LDATE) const;
	int    FASTCALL IsEqual(WorkDate) const;
	int    GetVal() const;
private:
	static LDATE InitDate; // @global
	int16  V;
};
//
// Descr: Пара дата-время. Аналог TIESTAMP в СУБД.
//   Специальное значение FAR означает бесконечно далекий момент времени
//   (применяется в диапазонах).
//
struct LDATETIME {
	SYSTEMTIME & FASTCALL Get(SYSTEMTIME &) const;
	LDATETIME FASTCALL operator = (const SYSTEMTIME &);
	LDATETIME FASTCALL operator = (const FILETIME &);
	//
	// Descr: Возвращает !0 если d == ZERODATE && t == ZEROTIME
	//
	int    operator !() const;
	int    FASTCALL operator == (const LDATETIME & s) const;
	int    FASTCALL operator != (const LDATETIME & s) const;
	operator LDATE () const { return d; }
	operator OleDate() const;
	LDATETIME FASTCALL operator = (OleDate);
	LDATETIME & SLAPI Set(LDATE _d, LTIME _t);
	LDATETIME & SLAPI SetZero();
	LDATETIME & SLAPI SetFar();
	LDATETIME & FASTCALL SetTimeT(time_t t);
	int    SLAPI IsFar() const;
	//
	// Decr: Устанавливает дату-время, разбирая строку pText в соответствии с форматами datf и timf.
	//
	int    SLAPI Set(const char * pText, long datf, long timef);
	//
	// Descr: Устанавливает время в соответсвии с количеством секунд, заданных
	//   параметром s и увеличивает дату на  (s / (3600 * 60 * 60)) дней.
	// Returns:
	//   Количество целых суток, содержащихся в s
	//
	long   FASTCALL settotalsec(long s);
	//
	// Descr: Прибавляет к времени n сотых долей секунды с переносом.
	//
	LDATETIME & FASTCALL addhs(long n);
	LDATETIME & FASTCALL addsec(long nsec);

	LDATE  d;
	LTIME  t;
};

int FASTCALL cmp(const LDATETIME & t1, const LDATETIME & t2);
int FASTCALL cmp(const LDATETIME & t1, LDATE dt, LTIME tm);
//
// Globals
//
extern const LDATE ZERODATE;
extern const LDATE MAXDATE;
extern const LDATE MAXDATEVALID; // 1/1/3000 Используется для хранения в базах данных, поскольку MAXDATE не проходит проверку на правильную дату.
extern const LTIME ZEROTIME;
extern const LTIME MAXTIME;
extern const LTIME MAXDAYTIME;
extern const LDATETIME ZERODATETIME;
extern const LDATETIME MAXDATETIME;
//
// Descr: Отрезок времени. Используется для планирования, расчетов занятости и т.д.
//   Обеспечивает базовые примитивы для работы с временными отрезками.
//   -- Нулевое время верхней границы эквивалентно концу суток.
//   -- Верхняя граница Finish.IsFar() эквивалентна бесконечности
//   @attention Не должен включать виртуальных фукнций.
//
struct STimeChunk {
	SLAPI  STimeChunk();
	SLAPI  STimeChunk(const LDATETIME & rStart, const LDATETIME & rFinish);
	int    SLAPI Init(const LDATETIME & rStart, const LDATETIME & rFinish);
	int    SLAPI Init(const LDATETIME &, long cont);
	//
	// Descr: Определяет, совпадают ли точки начала и окончания //
	//   отрезков *this и rTest.
	// Returns:
	//   !0 - точки начала и окончания this и rTest совпадают
	//   0  - точки начала и окончания this и rTest не совпадают
	//
	int    FASTCALL operator == (const STimeChunk & rTest) const;
	int    FASTCALL operator != (const STimeChunk & rTest) const;
	//
	// Descr: Сравнивает *this с элементом rTest.
	//   Сравнение осуществляется каскадно: сначала Start, затем Finish.
	// Returns:
	//   >0  - *this > rTest
	//   <0  - *this < rTest
	//   ==0 - *this == rTest
	//
	int    FASTCALL cmp(const STimeChunk & rTest) const;
	int    FASTCALL Has(const LDATETIME & rTm) const;
	//
	// Descr: Находит пересечение отрезков *this и rTest.
	// ARG(rTest    IN): Отрезок, на пересечение с которым проверяется *this.
	// ARG(pResult OUT): @#{vptr0} Если указатель не нулевой, то по нему присваивается //
	//   результат пересечения *this и rTest.
	// Returns:
	//   !0 - отрезки *this и rTest пересекаются //
	//   0  - отрезки *this и rTest не пересекаются //
	//
	int    FASTCALL Intersect(const STimeChunk & rTest, STimeChunk * pResult) const;
	//
	// Descr: Определяет, содержится ли период this в периоде rDur.
	//   Если this полностью содержится в rDur, то функция возвращает !0.
	//
	int    FASTCALL ContainsIn(const STimeChunk & rDur) const;
	//
	// Descr: Возвращает продолжительность элемента в секундах.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	long   SLAPI GetDuration() const;
	//
	// Descr: Возвращает продолжительность элемента в сутках.
	//   При вычислении в расчет берутся только даты Start.d и Finish.d.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	long   SLAPI GetDurationDays() const;
	//
	// Descr: Возвращает продолжительность элемента в миллисекундах.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	int64  SLAPI GetDurationMs() const;
	enum {
		fmtOmitSec  = 0x0001  // Не выводить секунды
	};
	SString & SLAPI ToStr(SString & rBuf, long fmt = 0) const;

	LDATETIME Start;
	LDATETIME Finish;
};
//
// Descr: Класс, определяющий календарные периодичности.
//
class DateRepeating { // @persistent @size=8
public:
	//
	// @noconstructor @novtbl
	//
	static int FASTCALL IsValidPrd(int prd);

	int    FASTCALL operator == (const DateRepeating & rS) const;
	int    FASTCALL operator != (const DateRepeating & rS) const;
	int    SLAPI Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    SLAPI Init(int prd, int kind = 1, LDATE = ZERODATE);
	int    SLAPI Next_(LDATE startDate, LDATE * pNextDate) const; // @obsolete use DateRepIterator
	SString & SLAPI Format(int fmt, SString & rBuf) const;
	long   SLAPI DtlToLong();
	int    SLAPI LongToDtl(long);
	int    SLAPI GetMonthlyPeriod(int * pMonthCount, int * pMonthNo) const;
	int    SLAPI SetMonthly(int monthCount, int monthNo, int dayOfMonth);
	int    SLAPI SetMonthly(int monthCount, int monthNo, int weekNo, int weekDay);
	int    SLAPI DayOfWeekNo(LDATE dt, int weekNo, int dayOfWeek, LDATE * pResult) const;

	struct RepeatDay {       // @size=4
		int16  NumPrd;
		uint16 QuantSec;     // @#[0..43200] Каждые QuantSec секунд. Если 0, то - раз в день
	};
	struct RepeatWeek {      // @size=4
		int16  NumPrd;
		uint8  Weekdays;     // @# bitstring[0..6]
		uint8  Zero;
	};
	struct RepeatMonthDay {  // @size=4
		int16  NumPrd;
		uint8  DayOfWeek;    // @#[1..7] День недели
		uint8  WeekNo;       // @#[1..5] Номер недели от начала месяца
	};
	struct RepeatMonthDate { // @size=4
		int16  NumPrd;
		uint8  DayOfMonth;   // @#[1..31]
		uint8  Zero;
	};
	struct RepeatAnnDay {    // @size=4
		int16  Month;        // @#[1..12]
		uint8  DayOfWeek;    // @#[1..7] День недели
		uint8  WeekNo;       // @#[1..5] Номер недели от начала месяца
	};
	struct RepeatAnnDate {   // @size=4
		int16  Month;        // @#[1..12]
		uint8  DayOfMonth;   // @#[1..31]
		uint8  Zero;         // @reserve
	};
	struct RepeatAfterPrd {
		int16  NumPrd;       // Кол-во периодов
		int16  AfterStart;   // 0 - Дату выполнения считать от предыдущей даты окончания, 1 - от предыдущей даты начала
	};

	int16  Prd;              // @anchor Периодичность 0|PRD_DAY|PRD_WEEK|PRD_MONTH|PRD_ANNUAL|PRD_REPEATAFTERPRD
	int16  RepeatKind;       // Вид повторения //
		// PRD_DAY:    0
		// PRD_WEEK:   0
		// PRD_MONTH:  1 - n-е число месяца, 2 - каждый n-й день недели wd
		// PRD_ANNUAL: 1 - n-е число месяца, 2 - каждый n-й день недели wd
		// PRD_REPEATAFTERPRD: PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_ANNUAL
	union RepeatDetail {     // @size=4 Детализация повторения //
		RepeatDay  D;
		RepeatWeek W;
		RepeatMonthDay  MY;
		RepeatMonthDate ME;
		RepeatAnnDay    AY;
		RepeatAnnDate   AE;
		RepeatAfterPrd  RA;
	} Dtl;
};
//
// Descr: Наследуя класс DateRepeating, этот класс вводит дополнительный параметр -
//   время начала.
//
class DateTimeRepeating : public DateRepeating { // @persistent @size=12
public:
	int    SLAPI Init(int prd, int kind = 1, LDATE dt = ZERODATE, LTIME tm = ZEROTIME);
	int    SLAPI Next_(LDATETIME startDtm, LDATETIME * pNextDtm) const; // @obsolete use DateTimeRepIterator
	SString & SLAPI Format(int fmt, SString & rBuf) const;
	LTIME  Time;
};
//
// Descr: Итератор для перечисления календарных дат, наступающих согласно
//   регулярному расписанию DateRepeating.
//
class DateRepIterator {
public:
	DateRepIterator(const DateRepeating & rDr, LDATE startDate, LDATE endDate = ZERODATE, uint maxCount = 0);
	LDATE  Next();
private:
	DateRepeating Dr;
	LDATE  Start;
	LDATE  End;
	uint   MaxCount;
	uint   Count;
	LDATE  Cur;
};
//
// Descr: Итератор для перечисления моментов времени, наступающих согласно
//   регулярному расписанию DateRepeating.
//
class DateTimeRepIterator {
public:
	DateTimeRepIterator(const DateTimeRepeating & rDr, LDATETIME startDtm, LDATE endDate = ZERODATE, uint maxCount = 0);
	LDATETIME Next();
private:
	DateRepIterator DtIter;
	DateTimeRepeating Dr;
	uint   MaxCount;
	uint   Count;
	LDATETIME Start;
	LDATETIME Cur;
};
//
// Descr: Класс реализует циклический таймер, позволяющий проверять
//   истечение фиксированного промежутка времени в цикле.
// Note: Класс предназначен для работы в циклах с достаточно большим временем
//   выполнения итерации. В противном случае, функция проверки таймера может
//   оказаться критичным фактором производительности.
// Sample:
//   SCycleTimer timer(1000); // Таймер включен на задержку в 1 секунду
//   while(SomeIter.Next()) {
//       LDATETIME prev;
//       if(timer.Check(&prev)) {
//           DoSomething(prev);
//       }
//   }
//
class SCycleTimer {
public:
	SCycleTimer(uint32 msDelay);
	int    FASTCALL Restart(uint32 msDelay);
	int    FASTCALL Check(LDATETIME * pLast);
private:
	uint32 Delay;
	LDATETIME  Last;
};

// @v8.7.2 #endif // } __cplusplus

#define ASSIGN_PTR(ptr,val)   if(ptr) *(ptr) = (val);
#define RVALUEPTR(val,ptr)    ((ptr)?((val = *(ptr)), 1) : 0)
#define PTR8(p)               ((uint8*)(p))
#define PTR8C(p)              ((const uint8*)(p))
#define PTR16(p)              ((uint16*)(p))
#define PTR32(p)              ((uint32*)(p))
#define PTR64(p)              ((uint64*)(p))
#define PTR2U(p)              ((uint32)(p))
#define U2PTR(typ,u)          ((typ*)(u))
#define U2PTRV(u)             ((void*)(u))
#define ALIGNSIZE(s,bits)     ((((s) + ((1 << (bits))-1)) >> (bits)) << (bits))
#ifndef MIN
	#define MIN(a,b)          ((a)<(b)?(a):(b))
#endif
#ifndef MAX
	#define MAX(a,b)          ((a)>(b)?(a):(b))
#endif
#define LoByte(w)             ((uint8)(w))
#define HiByte(w)             ((uint8)((uint16)(w) >> 8))
#define LoWord(l)             ((uint16)(l))
#define HiWord(l)             ((uint16)((uint32)(l) >> 16))
#define MakeLong(low,high)    ((long)(((uint16)(low)) | (((ulong)((uint16)(high))) << 16)))

#define oneof2(f,c1,c2)                   ((f)==(c1)||(f)==(c2))
#define oneof3(f,c1,c2,c3)                ((f)==(c1)||(f)==(c2)||(f)==(c3))
#define oneof4(f,c1,c2,c3,c4)             ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4))
#define oneof5(f,c1,c2,c3,c4,c5)          ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5))
#define oneof6(f,c1,c2,c3,c4,c5,c6)       ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6))
#define oneof7(f,c1,c2,c3,c4,c5,c6,c7)    ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7))
#define oneof8(f,c1,c2,c3,c4,c5,c6,c7,c8) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8))
#define oneof9(f,c1,c2,c3,c4,c5,c6,c7,c8,c9) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9))
#define oneof10(f,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9)||(f)==(c10))
#define oneof11(f,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9)||(f)==(c10)||(f)==(c11))
#define NZOR(v,a)                   ((v)?(v):(a))
//#define NZOR_F(v,a)                 ((!!(v))?(v):(a))
// @v8.6.8 #define SETIFZ(v,a)                 if(!v) {(v)=(a);}
#define SETIFZ(v,a)                 ((v) || !!((v)=(a))) // @v8.6.8
#define SETMAX(v,b)                 {if((v)<(b)) (v)=(b);}
#define SETMIN(v,b)                 {if((v)>(b)) (v)=(b);}
#define SETMINMAX(v,i,x)            {if((v)>(x)) (v)=(x); else if((v)<(i)) (v)=(i);}
#define MINMAX(v,i,x)               (((v)<(i)) ? (i) : (((v)>(x)) ? (x) : (v)))
#define ICEIL(v,quant)              (((v)+((quant)-1))/(quant)*(quant))
#define IFLOOR(v,quant)             ((v)/(quant)*(quant))
#define SETFLAG(v,f,b)              {if(b) {(v)|=(f);} else {(v)&=~(f);}}
#define SETFLAGBYSAMPLE(v,f,b)      {if((b)&(f)) {(v)|=(f);} else {(v)&=~(f);}}
#define TESTFLAG(v1,v2,f)           (((v1)&(f))==((v2)&(f)))
#define TESTMULTFLAG(v,f)           (((v)&(f))==(f))
#define INVERSEFLAG(v,f)            {if((v)&(f)) {(v)&=~(f);} else {(v)|=(f);}}
#define CHKXORFLAGS(v,f1,f2)        ((((v)&(f1)) ^ ((v)&(f2))) ? (((v)&(f1))?(f1):(f2)) : 0)
#define BIN(x)                      ((x)?1:0)
#define BOOLXOR(p1,p2)              (((p1)&&(p2))||(!(p1)&&!(p2)))
#define DELETEANDASSIGN(p, n)       ((delete (p)), (p) = (n))
#define ZDELETE(p)                  DELETEANDASSIGN(p, 0)
#define ZDELETEFAST(p)              {if(p) ((delete (p)), (p) = 0);}
#define FREEANDASSIGN(p, n)         (SAlloc::F(p), (p) = (n))
#define ZFREE(p)                    FREEANDASSIGN(p, 0)
//
// Exception handling
//
#define EXCEPTVAR(intvar)      int*__sexcept=&(intvar)
#define EXCEPT                 *__sexcept
#define CALLEXCEPT()           goto __scatch;
#define CALLEXCEPT_S(val)      {SLS.SetError(val);goto __scatch;}
#define CALLEXCEPT_S_S(val,s)  {SLS.SetError(val, s);goto __scatch;}
#define CALLEXCEPTV(val)       {*__sexcept=val;goto __scatch;}
#define THROW(expr)            {if(!(expr)){goto __scatch;}}
#define THROW_V(expr,val)      {if(!(expr)){*__sexcept=val;goto __scatch;}}
#define THROW_S(expr,val)      {if(!(expr)){SLS.SetError(val);goto __scatch;}}
#define THROW_S_S(expr,val,s)  {if(!(expr)){SLS.SetError(val, s);goto __scatch;}}
#define THROW_P()              {if(!_AX){goto __scatch;}}
#define THROW_VP(val)          {if(!_AX){*__sexcept=val;goto __scatch;}}
#define THROW_L()              {if(!_AX&&!_DX){goto __scatch;}}
#define THROW_VL(val)          {if(!_AX&&!_DX){*__sexcept=val;goto __scatch;}}
#define CATCH                  goto __sendcatch;__scatch:
#define ENDCATCH               __sendcatch:;
#define CATCHZOK               \
	goto __sendcatch;__scatch:\
	ok = 0;\
	__sendcatch:;
//
// SLIB Error Codes
//
#define SLERR_SUCCESS                    0
#define SLERR_NOMEM                      1
#define SLERR_INVRANGE                   2
#define SLERR_BOUNDS                     3
#define SLERR_BUFTOOSMALL                4
#define SLERR_NOFOUND                    5
#define SLERR_INVSYMBOL                  6
#define SLERR_INVDATA                    7
#define SLERR_EOLEXPECTED                8
#define SLERR_DATAEXPECTED               9
#define SLERR_TERMEXPECTED              10
#define SLERR_INVDAY                    11
#define SLERR_INVMONTH                  12
#define SLERR_INVYEAR                   13
#define SLERR_ARYDELTA                  15
#define SLERR_ARYZERODELTA              16
#define SLERR_ARYNOTORD                 17
#define SLERR_ARYITEMNFOUND             SLERR_NOFOUND
#define SLERR_ARYDUPITEM                18
#define SLERR_ARYZEROCOUNT              19
#define SLERR_FILENOTFOUND              20
#define SLERR_READFAULT                 21
#define SLERR_WRITEFAULT                22
#define SLERR_INVFORMAT                 23
//
// Printer errors
//
#define SLERR_PRTNOTREADY               24
#define SLERR_PRTBUSY                   25
#define SLERR_PRTOUTOFPAPER             26
#define SLERR_REZNFOUND                 27
#define SLERR_PGLENTOOSMALL             28 // Page lenght too small for report
#define SLERR_NODEFPRINTER              29 // Не удалось идентифицировать принтер по умолчанию

#define SLERR_OPENFAULT                 30 // Error opening file
#define SLERR_MKDIRFAULT                31 // Error creating directory
#define SLERR_INVALIDCRC                32 // Error recovering file (invalid CRC)
#define SLERR_CANTCOMPRESS              33 // Ошибка сжатия //
#define SLERR_CANTDECOMPRESS            34 // Ошибка распаковки
#define SLERR_DISKFULL                  35 // Диск переполнен
#define SLERR_WINDOWS                   36 // Ошибка Win32
#define SLERR_INVGUIDSTR                37 // Недопустимая строка GUID (%s)
#define SLERR_INVSERIALIZEVER           38 // Считан недопустимый номер версии сериализации (%s)
#define SLERR_DLLLOADFAULT              39 // Ошибка загрузки DLL %s
//
// Communication errors
//
#define SLERR_COMMINIT                  40 // Error initializing comm port %s
#define SLERR_COMMSEND                  41 // Error sending data to comm port
#define SLERR_COMMRCV                   42 // Error receiving data from comm port
#define SLERR_HTTPCODE                  43 // HTTP Error code=%s
#define	SLERR_USB						44 // Ошибка usb: %s
//
// Socket errors
//
#define SLERR_SOCK_UNABLEOPEN           50 // Ошибка открытия сокета
#define SLERR_SOCK_CONNECT              51 // Ошибка установки соединения с хостом '%s'
#define SLERR_SOCK_HOSTRESLVFAULT       52 // Ошибка разрешения имени хоста
#define SLERR_SOCK_TIMEOUT              53 // Превышен таймаут ожидания сокета
#define SLERR_SOCK_OPTERR               54 // Ошибка состояния сокета
#define SLERR_SOCK_NONBLOCKINGCONN      55 // Ошибка неблокового соединени с хостом
#define SLERR_SOCK_NONBLOCKINGWR        56 // Ошибка неблоковой записи в сокет
#define SLERR_SOCK_NONBLOCKINGRD        57 // Ошибка неблокового чтения из сокета
#define SLERR_SOCK_SEND                 58 // Ошибка записи в сокет
#define SLERR_SOCK_RECV                 59 // Ошибка чтения из сокета
#define SLERR_SOCK_CONNCLOSEDBYHOST     60 // Соединение закрыто хостом
#define SLERR_SOCK_LINETOOLONG          61 // Считанная строка слишком велика
#define SLERR_SOCK_FILETRANSMFAULT      62 // Ошибка передачи файла
#define SLERR_INVPATH				    63 // Неправильный формат пути
#define SLERR_SOCK_ACCEPT			    64 // Ошибка при создании соединения //
#define SLERR_SOCK_LISTEN			    65 // Ошибка при создании соединения //
#define SLERR_SOCK_WINSOCK              66 // Ошибка WinSock (call WSAGetLastError)
//
// Matrix error
//
#define SLERR_MTX_INCOMPATDIM_VADD      70 // Incompatible dim in vect+vect op
#define SLERR_MTX_INCOMPATDIM_VIMX      71 // Incompatible dim in vect to matrix op
#define SLERR_MTX_INCOMPATDIM_MVMUL     72 // Incompatible dim in matrix*vector op
#define SLERR_MTX_INCOMPATDIM_MMMUL     73 // Incompatible dim in matrix*matrix op
#define SLERR_MTX_INCOMPATDIM_MMADD     74 // Incompatible dim in matrix+matrix op
#define SLERR_MTX_INVERSE               75 // Ошибка обращения матрицы.

#define SLERR_FILESHARINGVIOLATION      97 // Ошибка совместного доступа к файлу
#define SLERR_FLOCKFAULT                98 // Ошибка блокировки файла //
#define SLERR_USERBREAK                 99 // Операция прервана пользователем
#define SLERR_FILENOTOPENED            100 // Файл не открыт

#define SLERR_TXTDB_ZEROLENFIXEDFLD    101 // Для текстового файла с фиксированными полями должны быть определены длины всех полей
#define SLERR_TXTDB_EMPTYFLDDIV        102 // Для текстового файла не определен разделитель полей
#define SLERR_TXTDB_EMPTYVERTTERM      103 // Для текстового файла с вертикальной раскладкой не определ терминатор записей
#define SLERR_TXTDB_MISSPLHEADER       104 // Повторная попытка добавить заголовок к файлу, либо уже добавлены записи (%s)
#define SLERR_XMLDB_INVRECORROOTTAG    110 // Для xml файла не определен корневой тег или тег записи
#define SLERR_XMLDB_ROOTTAGEMPTY       111 //
#define SLERR_XMLDB_RECTAGEMPTY        112 //
#define SLERR_XMLDB_ROOTTAGINVCHR      113 //
#define SLERR_XMLDB_RECTAGINVCHR       114 //

#define SLERR_SDREC_DUPFLDID           121 // Поле с заданным идентификатором в записи уже присутствует ('%s')
#define SLERR_SDREC_DUPFLDNAME         122 // Поле с заданным именем '%s' в записи уже присутствует
#define SLERR_SDREC_SYNTAX             123 // Ошибка синтаксиса при разборе определения типа поля (%s) //
#define SLERR_SDREC_FLDIDNFOUND        124 // Поле с ид=%s в записи отсутствует
#define SLERR_DUPIDXSEG                125 // Дублирование поля в индексе
#define SLERR_DBF_INVHEADER            126 // Нарушена целостность заголовка DBF-файла '%s'
#define SLERR_DBF_NOTOPENED            127 // Таблица DBF '%s' не открыта
#define SLERR_DBF_INVRECNO             128 // Недопустимый номер записи при доступе к DBF-таблице '%s'
#define SLERR_SPII_CMDHDRREADFAULT     129 // Ошибка чтения заголовка команды StyloPalm: %s
#define SLERR_SPII_CMDREADFAULT        130 // Ошибка чтения тела команды StyloPalm: %s
//
// CRegExp error
//
#define SLERR_RE_UNINIT                150 // Регулярное выражение не инициализировано
#define SLERR_RE_NOEXPR                151 // CoolRegexp::compile(): No expression supplied.\n");
#define SLERR_RE_EXPRTOOBIG            152 // CoolRegexp::compile(): Expression too big.\n");
#define SLERR_RE_NOMEM                 153 // CoolRegexp::compile(): Out of memory.\n");
#define SLERR_RE_INVBRANGE             154 // CoolRegexp::compile(): Invalid range in [].\n");
#define SLERR_RE_UNMATCHEDBR           155 // CoolRegexp::compile(): Unmatched [].\n");
#define SLERR_RE_INTERNAL              156 // CoolRegexp::compile(): Internal error.\n"); /* Never here */
#define SLERR_RE_WCNOTHFOLLOWS         157 // CoolRegexp::compile(): ?+* follows nothing.\n");
#define SLERR_RE_TRAILINGBSL           158 // CoolRegexp::compile(): Trailing backslash.\n");
#define SLERR_RE_WCCOULDBEEMPT         159 // CoolRegexp::compile() : *+ operand could be empty.\n");
#define SLERR_RE_NESTEDWC              160 // CoolRegexp::compile(): Nested *?+.\n");
#define SLERR_RE_TOOMANYPAR            161 // CoolRegexp::compile(): Too many parentheses.\n");
#define SLERR_RE_UNMATCHPAR            162 // CoolRegexp::compile(): Unmatched parentheses.\n");
#define SLERR_RE_BUFCORRUPT            163 // CoolRegexp::find(): Compiled regular expression corrupted.\n");
#define SLERR_RE_MEMCORRUPT            164 // CoolRegexp::find(): Internal error -- memory corrupted.\n");
#define SLERR_RE_PTRCORRUPT            165 // CoolRegexp::find(): Internal error -- corrupted pointers.\n");

#define SLERR_HT_NOASSOC               180 // Экземпляр хэш-таблицы не поддерживает ассоциации
#define SLERR_FILE_DELETE              181 // Ошибка удаления файла '%s'
#define SLERR_FILE_RENAME              182 // Ошибка переименования файла '%s'
#define SLERR_INIOPENFAULT             183 // Ошибка открытия INI-файла '%s'
#define SLERR_PAINTTOOLIDBUSY          184 // Идентификатор PaintToolBox занят
#define SLERR_INVHOUR                  185 // Недопустимый час во времени
#define SLERR_INVMIN                   186 // Недопустимые минуты во времени
#define SLERR_INVSEC                   187 // Недопустимые секунды во времени
#define SLERR_INVTSEC                  188 // Недопустимые сотые секунд во времени
#define SLERR_UNMATCHSTREAMARRAY       189 // Несоответствующий размер записи считываемого массива данных (%s)
#define SLERR_INVGEOLATITUDE           190 // Недопустимое значение географической широты
#define SLERR_INVGEOLONGITUDE          191 // Недопустимое значение географической долготы
#define SLERR_SRLZ_UNDEFSUPPDATE       192 // Для восстановления даты из потока нет необходимой опорной даты
#define SLERR_SRLZ_INVDATAIND          193 // Недопустимое значение индикатора данных при восстановлении из потока
#define SLERR_SRLZ_UNEQFLDLIST         194 // Ассоциированная с данными структура не равна заданной в контексте
#define SLERR_SRLZ_COMMRDFAULT         195 // Общий сбой при чтении данных (данные не соответствуют ожидаемым значениям)
#define SLERR_SBUFRDSIZE               196 // Ошибка чтения из буфера: размер считанных данных не равен запрошенному (%s)

#define SLERR_WINSVC_COMMON            200 // Ошибка Windows Service
#define SLERR_WINSVC_SVCNEXISTS        201 // Windows Service '%s' не существует
#define SLERR_WINSEC_ACQCREDHDL        202 // "AcquireCredentialsHandle failed with %s"
#define SLERR_WINSEC_INITSECCTX        203 // "InitializeSecurityContext failed with %s"
#define SLERR_WINSEC_COMPLAUTHTOK      204 // "CompleteAuthToken failed with %s"
#define SLERR_WINSEC_COMPLAUTHTOKNSUPP 205 // "CompleteAuthToken not supported"
#define SLERR_WINSEC_ACCPTSECCTX       206 // "AcceptSecurityContext failed with %s"

#define SLERR_WMI_CREATELOCATOR        210 // Ошибка создания WMI-локатора
#define SLERR_WMI_CONNECTSRV           211 // Ошибка соединения с WMI-сервером '%s'
#define SLERR_WMI_SETPROXIBLANKET      212 // Ошибка вызова метода CoSetProxyBlanket
#define SLERR_WMI_GETOBJECT            213 // Ошибка вызова метода WMI GetObject (%s)
#define SLERR_WMI_GETMETHOD            214 // Ошибка вызова метода WMI GetMethod (%s)
#define SLERR_WMI_SPAWNINSTANCE        215 // Ошибка вызова метода WMI SpawnInstance
#define SLERR_WMI_EXECMETHOD           216 // Ошибка вызова метода WMI ExecMethod '%s'
#define SLERR_WMI_EXECMETHODRETVAL     217 // Код возврата созданного процесса '%s' не равен 0
#define SLERR_GLOBOBJIDXNFOUNT         218 // Не найден глобальный объект SLib по индексу %s
//
//
//
#define SLERR_MATH_DOMAIN              300 // Ошибка области определения функции (sqrt(-1))
#define SLERR_MATH_ROUND               301 // Ошибка округления //
#define SLERR_MATH_OVERFLOW            302 // Ошибка переполнения ieee-числа
#define SLERR_MATH_UNDERFLOW           303 // Ошибка переполнения нижней границы точности ieee-числа
#define SLERR_MATH_MAXITER             304 // Ошибка превышения максимального числа итераций
//
//
//
#define SLERR_INVDBSYMB                400 // Неверный символ базы данных '%s'
#define SLERR_UTFCVT_SRCEXHAUSTED      401 // Не полный входной буфер UTF8
#define SLERR_UTFCVT_ILLUTF8           402 // Не допустимый входной символ UTF8
#define SLERR_UTFCVT_ILLUTF16          403 // Не допустимый входной символ UTF16
#define SLERR_TAB_NFOUND               404 // Таблица STab '%s' не найдена
#define SLERR_INVIMAGEPIXFORMAT        405 // Недопустимый формат пикселя.
#define SLERR_IMAGEFILENOTJPEG         406 // Файл '%s' не является JPEG-файлом.
#define SLERR_JPEGLOADFAULT            407 // Ошибка загрузки JPEG-Файла '%s'
#define SLERR_IMAGEFILENOTPNG          408 // Файл '%s' не является PNG-файлом.
#define SLERR_PNGLOADFAULT             409 // Ошибка загрузки PNG-изображения: '%s'
#define SLERR_UNSUPPIMGFILEFORMAT      410 // Не поддерживаемый или не допустимый формат файла изображения: '%s'
#define SLERR_SFILRDSIZE               411 // Ошибка чтения из файла: размер считанных данных не равен запрошенному (%s)
#define SLERR_IMAGEFILENOTICO          412 // Файл '%s' не является ICO-файлом.
#define SLERR_PNGSTOREFAULT            413 // Ошибка сохранения PNG-изображения: '%s'
#define SLERR_INVSDRAWFIGKIND          414 // Internal: Недопустимый вид графической фигуры (%s)
#define SLERR_DUPDRAWGROUPSYMB         415 // Дублирование символа '%s' элемента, добавляемого в SDrawGroup
#define SLERR_WTMTA_UNDEFFIG           416 // Для элемента TWhatmanToolArray '%s' не заданы ни фигура, ни иконка.
#define SLERR_WTMTA_BADCRC             417 // Нарушена целостность WTA-файла '%s'
#define SLERR_IMAGEFILENOTBMP          418 // Файл '%s' не является BMP-файлом.
#define SLERR_INVBMPHEADER             419 // Недопустимый BMP-заголок (%s)
#define SLERR_BMPCOMPRNSUPPORTED       420 // Сжатые BMP-изображения не поддерживаются.
#define SLERR_SOAPR_UNDEFREF           421 // Неопределенная ссылка '%s' в SOAP-результате
#define SLERR_SOAPR_UNDEFTYPE          422 // Неопределенный тип данных '%s' в SOAP-результате
#define SLERR_SOAPR_ITEMREFNFOUND      423 // Не найден элемент SOAP-результата по ссылке '%s'
#define SLERR_SOAPR_ITEMREFTYPECONFL   424 // Конфликт между атрибутами ref и type (%s) в элементе SOAP-ответа
#define SLERR_SOPAR_UNRESITEMHASNTREF  425 // Неразрешенный элемент структуры (%s) SAOP-результата не имеет ссылки
#define SLERR_SOAPR_UNRESOLVEDITEM     426 // Не удалось резрешить элемент структуры (%s) SOAP-результата
#define SLERR_SOAPR_ITEMNAMENFOUND     427 // Элемент SOAP-пакета '%s' не найден
#define SLERR_SOAPR_INVITEMPOS         428 // Недопустимый индекс (%s) элемента SOAP-пакета
#define SLERR_WTMTA_INVSIGNATURE       429 // Неверная сигнатура WT-файла '%s'
#define SLERR_USB_HIDUSBCLASSFAILED	   430 // Устройство не относится к классу USB или HID
#define SLERR_SFILRDNULLOUTP           431 // Попытка чтения из NullOutput-файла
#define SLERR_MAIL_NOTCONNECTED        432 // Внутренняя ошибка - не установлено соединение почтового клиента
#define SLERR_MAIL_INVPROTOCOL         433 // Внутренняя ошибка - не допустимый протокол '%s' почтового клиента
#define SLERR_MAIL_SMTP_NOREPLY        434 // Почтовый сервер SMTP не ответил на запрос.
#define SLERR_MAIL_SMTP_REPLYERR       435 // Почтовый сервер SMTP вернул ошибку %s.
#define SLERR_MAIL_POP3_NOREPLY        436 // Почтовый сервер POP3 не ответил на запрос.
#define SLERR_MAIL_POP3_REPLYERR       437 // Почтовый сервер POP3 вернул ошибку '%s'
#define SLERR_MAIL_POP3_UNDEFREPLY     438 // Почтовый сервер POP3 вернул не известный ответ '%s'
#define SLERR_FTP_NOTCONNECTED         451 // FTP-соединение не установлено

#define SLERR_PUNYCODE_BADINPUT        452 // PUNYCODE Input is invalid
#define SLERR_PUNYCODE_BIGOUTPUT       453 // PUNYCODE Output would exceed the space provided
#define SLERR_PUNYCODE_OVERFLOW        454 // PUNYCODE Wider integers needed to process input
#define SLERR_CURL                     455 // Ошибка библиотеки libcurl
#define SLERR_LIBXML                   456 // Ошибка библиотеки libxml
#define SLERR_DUPSYMBWITHUNEQID        457 // Дублирование символа '%s' элемента с отличным идентификатором

#define SLERR_FANN_CANT_OPEN_CONFIG_R          500 // Unable to open configuration file for reading
#define SLERR_FANN_CANT_OPEN_CONFIG_W          501 // Unable to open configuration file for writing
#define SLERR_FANN_WRONG_CONFIG_VERSION        502 // Wrong version of configuration file
#define SLERR_FANN_CANT_READ_CONFIG            503 // Error reading info from configuration file
#define SLERR_FANN_CANT_READ_NEURON            504 // Error reading neuron info from configuration file
#define SLERR_FANN_CANT_READ_CONNECTIONS       505 // Error reading connections from configuration file
#define SLERR_FANN_WRONG_NUM_CONNECTIONS       506 // Number of connections not equal to the number expected
#define SLERR_FANN_CANT_OPEN_TD_W              507 // Unable to open train data file for writing
#define SLERR_FANN_CANT_OPEN_TD_R              508 // Unable to open train data file for reading
#define SLERR_FANN_CANT_READ_TD                509 // Error reading training data from file
#define SLERR_FANN_CANT_ALLOCATE_MEM           510 // Unable to allocate memory
#define SLERR_FANN_CANT_TRAIN_ACTIVATION       511 // Unable to train with the selected activation function
#define SLERR_FANN_CANT_USE_ACTIVATION         512 // Unable to use the selected activation function
#define SLERR_FANN_TRAIN_DATA_MISMATCH         513 // Irreconcilable differences between two <Fann::TrainData> structures
#define SLERR_FANN_CANT_USE_TRAIN_ALG          514 // Unable to use the selected training algorithm
#define SLERR_FANN_TRAIN_DATA_SUBSET           515 // Trying to take subset which is not within the training set
#define SLERR_FANN_INDEX_OUT_OF_BOUND          516 // Index is out of bound
#define SLERR_FANN_SCALE_NOT_PRESENT           517 // Scaling parameters not present
#define SLERR_FANN_INPUT_NO_MATCH              518 // The number of input neurons in the ann and data don't match
#define SLERR_FANN_OUTPUT_NO_MATCH             519 // The number of output neurons in the ann and data don't match
#define SLERR_FANN_WRONG_PARAMETERS_FOR_CREATE 520 // The parameters for create_standard are wrong, either too few parameters provided or a negative/very high value provided
#define SLERR_FANN_INVLAYERCOUNT               521 // Не допустимое количество слоев нейронной сети
#define SLERR_FANN_INVLAYERSIZE                522 // Не допустимый размер слоя нейронной сети
#define SLERR_FANN_INVTRAINALG                 523 // Не допустимый алгоритм обучения нейронной сети

// LastError
//
// @ModuleDecl(SType) {
// Система динамических типов. Дескриптор типа является 4-байтовым
// значение, хранящим идентификатор типа и размер.
// Таким образом, максимальный размер одного поля данных составляет 0xffff байт.
// Для очень больших данных может быть использован S_BLOB или S_CLOB (управление этими типами
// достаточно сложное).
//
#define STYPE_SIZE_4 // @v8.9.12 Удалены все ссылки на этот макрос. Теперь безусловно sizeof(TYPEID)==4
#define INVALID_DATA_SIZE 0
typedef int32 TYPEID;
//
// Base types. Базовые типы, используемые для приведения и преобразования разных типов.
//
#define BTS_VOID         0
#define BTS_STRING       1    // ASCIIZ
#define BTS_INT          2    // 4 byte
#define BTS_REAL         3    // 8 byte IEEE float
#define BTS_DATE         4    // 4 byte LDATE
#define BTS_TIME         5    // 4 byte LTIME
#define BTS_DATETIME     6    // 8 byte LDATETIME
#define BTS_POINT2       7    // 16 byte RPoint
#define BTS_BOOL         8    // 4 byte Специализированный вариант. Фактически не применяется как базовый тип,
	// но испольузется в локальных ситуациях.
#define BTS_PTR          9    // Указатель. Важно, что размер его зависим от архитектуры (4 или 8 байт)
//
// Data types
//
#define S_VOID           0
#define S_CHAR           1
#define S_INT            2
#define S_FLOAT          3
#define S_DATE           4
#define S_TIME           5
#define S_DEC            6
#define S_MONEY          7
#define S_LOGICAL        8
#define S_BOOL           S_LOGICAL
#define S_NUMERIC        9
#define S_BFLOAT        10
#define S_LSTRING       11
#define S_ZSTRING       12
#define S_NOTE          13      // Для BTRIEVE - символьное поле переменной длины (последнее в таблице),
	// для SQL - VARCHAR. Максимальная бинарная (с завершающим нулем) длина специфицируется //
#define S_LVAR          14
#define S_UBINARY       15
#define S_UINT          S_UBINARY
#define S_AUTOINC       16
#define S_BIT           17
#define S_STS           18

#define S_INTRANGE      19		// int2 range only
#define S_REALRANGE     20		// double range only
#define S_DATERANGE     21      //
#define S_DATETIME	    22      //
#define S_ARRAY		    23		// not real STYPE, only ID
#define S_STRUCT	    24		//--//--
#define S_VARIANT       25      //
#define S_WCHAR         26      // wide char-строка (оканчивается 0). Размер данных указывается в байтах.
#define S_BLOB          27      // BLOB (для SQL-таблиц)
#define S_CLOB          28      // CLOB (для SQL-таблиц)
#define S_RAW           29      // RAW-data бинарное поле с произвольным содержимым
#define S_ROWID         30      // ROWID (для SQL-таблиц)
#define S_IPOINT2       31      // Целочисленная двумерная точка (x, y)
#define S_FPOINT2       32      // Вещественная двумерная точка (x, y)
#define S_WZSTRING      33      // Unicode-zstring
#define S_UUID_         34      // @v8.0.9 (Суффиксное подчеркивание необходимо для отличия от class S_GUID)
//
#define S_FIRSTUSERTYPE 50
//
// Максимальное количество типов в таблице
//
#define TYPE_TAB_SIZ 64

#define S_ARRAY_INT  0x82
#define S_ARRAY_DBL  0x83

#define MKSTYPE(typ,siz)      ((((uint)(siz))<<16)|(typ))
#define MKSTYPED(typ,siz,prc) (((prc)<<24)|(((uint)(siz))<<16)|(typ))
#define GETSTYPE(typ)         ( (typ)&0x0000ffff)
#define GETSSIZE(typ)         (((typ)&0xffff0000)>>16)
#define GETSSIZED(typ)        (((typ)&0x00ff0000)>>16)
#define GETSPRECD(typ)        (((typ)&0xff000000)>>24)

#ifdef COMPLEX_TYPES
	#define T_CHAR             MKSTYPE(S_CHAR,1)
	#define T_INT              MKSTYPE(S_INT,2)
	#define T_UINT             MKSTYPE(S_UBINARY,2)
	#define T_UINT16           MKSTYPE(S_UBINARY,2)
	#define T_UINT32           MKSTYPE(S_UBINARY,4
	#define T_INT32            MKSTYPE(S_INT,4)
	#define T_LONG             MKSTYPE(S_INT,4)
	#define T_ULONG            MKSTYPE(S_UBINARY,4)
	#define T_FLOAT            MKSTYPE(S_FLOAT,4)
	#define T_DOUBLE           MKSTYPE(S_FLOAT,8)
	#define T_MONEY            MKSTYPED(S_DEC,8,2)
	#define T_DATE             MKSTYPE(S_DATE,4)
	#define T_TIME             MKSTYPE(S_TIME,4)
	#define T_DATETIME         MKSTYPE(S_DATETIME,8)
	#define T_ARRAY            MKSTYPE(S_ARRAY,0)
	#define T_STRUCT           MKSTYPE(S_STRUCT,0)
	#define T_VARIANT          MKSTYPE(S_VARIANT, sizeof(VARIANT))
	#define T_ROWID            MKSTYPE(S_ROWID, sizeof(DBRowId))
	#define T_IPOINT2          MKSTYPE(S_IPOINT2, 4)
	#define T_FPOINT2          MKSTYPE(S_FPOINT2, 8)
	#define T_BOOL             MKSTYPE(S_BOOL, 4)                  // @v8.1.2
	#define T_GUID             MKSTYPE(S_UUID_, 16)                // @v8.6.2
#endif

#if /* @v8.7.2 defined(__cplusplus) &&*/ !defined(RC_INVOKED) /* { */

struct STypEx { // @persistent @size=STYPE_SIZE_4 ? 16 : 12
	DECL_INVARIANT_C();
	void   Init();
	int    IsPure() const;
	int    FASTCALL IsEqual(const STypEx &) const;
	int    FASTCALL IsZStr(size_t * pLen) const;
	int    FASTCALL IsWZStr(size_t * pLen) const;
	int    FASTCALL IsRaw(size_t * pLen) const;
	size_t GetBinSize() const;
	TYPEID GetDbFieldType() const;
	enum {
		fFormula = 0x01, // Поле использует формулу
		fZeroID  = 0x02, // Поле должно иметь нулевой идентификатор в записи
		fStruct  = 0x04, // Если установлен, то тип является сложным. Структура типа в этом случае
			//	определяется полем Link, смысл которого задается приложением.
		fOf      = 0x08, // Если установлен, то тип является модификатором типа, на который
			// ссылается Link. Смысл поля Link определяется приложением.
		fStatic  = 0x10, // Статический член структуры (используется в языковых подсистемах)
		fTypedef = 0x20  // typedef-определение.
	};
	enum {
		modNone = 0,
		modArray,
		modPtr,
		modRef,
		modLink
	};
	uint8  Flags;
	uint8  Mod;     // modXXX Модификатор типа
	uint16 Reserve; // @alignment
	TYPEID Typ;
	uint32 Dim;     // Размерность массива
	uint32 Link;    // Ссылка на связанный тип (oneof4(Mod, modPtr, modRef, modArray, modLink))
};
//
// Descr: Базовый класс, потомки которого реализуют функционал различных типов данных.
//
class DataType {
public:
	SLAPI  DataType() {}
	virtual size_t SLAPI size() const;
	virtual int    SLAPI comp(const void * i1, const void * i2) const;
	virtual char * SLAPI tostr(const void *, long, char * b) const;
	//
	// @v9.1.11
	// Descr: Функция должна преобразовать бинарное представление данные pData
	//   в строку rBuf в соответствии с форматом преобразования format.
	//   Функция ToStr обязана предварительно очистить буфер rBuf вызовом rBuf = 0
	// Note: Эта функция замещает устаревшую tostr.
	//   На начальном этапе default-реализация функции вызывает tostr()
	//
	virtual SString & SLAPI ToStr(const void * pData, long format, SString & rBuf) const;
	virtual int    SLAPI fromstr(void *, long, const char *) const;
	virtual int    SLAPI base() const;
	virtual int    SLAPI tobase(const void * data, void * baseData) const;
	virtual int    SLAPI baseto(void * data, const void * baseData) const;
	virtual void   FASTCALL destroy(void *);
	// DataType::minval and DataType::maxval sets to parameter
	// min and max values correspondly.
	// Returns !0 if function is supported, otherwise 0.
	virtual void   SLAPI minval(void *) const;
	virtual void   SLAPI maxval(void *) const;
	virtual int    SLAPI Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
	int    SLAPI isnumber() const { return (base() == BTS_INT || base() == BTS_REAL); }

	size_t s; // Size or other specific data
};

// @v8.7.2 SPTRDEF(DataType);
// @v8.7.2 SREFDEF(DataType);

inline TYPEID SLAPI GetSTypeID(char)   { return MKSTYPE(S_INT, 1); }
inline TYPEID SLAPI GetSTypeID(int16)  { return MKSTYPE(S_INT, 2); }
inline TYPEID SLAPI GetSTypeID(int)    { return MKSTYPE(S_INT, 4); }
inline TYPEID SLAPI GetSTypeID(long)   { return MKSTYPE(S_INT, 4); }
inline TYPEID SLAPI GetSTypeID(float)  { return MKSTYPE(S_FLOAT, 4); }
inline TYPEID SLAPI GetSTypeID(double) { return MKSTYPE(S_FLOAT, 8); }
inline TYPEID SLAPI GetSTypeID(LDBL)   { return MKSTYPE(S_FLOAT, 10); }
inline TYPEID SLAPI GetSTypeID(char *,  int s) { return MKSTYPE(S_ZSTRING, s); }
inline TYPEID SLAPI GetSTypeID(LDATE)  { return MKSTYPE(S_DATE, 4); }
inline TYPEID SLAPI GetSTypeID(LTIME)  { return MKSTYPE(S_TIME, 4); }
inline TYPEID SLAPI GetSTypeID(LDATETIME)  { return MKSTYPE(S_DATETIME, 8); }

DataType & FASTCALL stype(TYPEID t, DataType *);
TYPEID     FASTCALL bt2st(int baseType);

inline int SLAPI btnumber(int bt)
	{ return oneof2(bt, BTS_INT, BTS_REAL); }
size_t FASTCALL stsize(TYPEID t);
int    SLAPI stcomp(TYPEID t, const void * i1, const void * i2);
char * SLAPI sttostr(TYPEID t, const void * d, long f, char * b);
int    SLAPI stfromstr(TYPEID t, void * d, long f, const char * b);
int    FASTCALL stbase(TYPEID t);
int    SLAPI sttobase(TYPEID t, const void * data, void * baseData);
int    SLAPI stbaseto(TYPEID t, void * data, const void * baseData);
void   FASTCALL stdestroy(TYPEID t, void * d);
int    FASTCALL stisnumber(TYPEID t);
void   FASTCALL stminval(TYPEID t, void * d);
void   FASTCALL stmaxval(TYPEID t, void * d);
int    SLAPI stcast(TYPEID st, TYPEID dt, const void * sd, void * dd, long f);
//
// Descr: Register all BUILT-INS SLIB TYPES (BIST)
//
void   SLAPI RegisterBIST();
//
// Descr: Заносит в буфер rBuf c++ тип, соответствующий типу typ.
//   Если pFldName != 0, то строка будет иметь следующий вид "type  pFldName[count]"
//   Минимальная позиция pFldName от начала строки определяется параметром indent.
//   Если indent <= strlent(type), то indent = strlen(type) + 1.
//   Если pFldName != 0 и тип предполагает указание количество элементов массива, то
//   строка будет иметь вид "type pFldName[count]", если pFldName == 0, то количество
//   элементов не приводится: "type"
// Returns:
//   rBuf
//
SString & GetBinaryTypeString(TYPEID typ, int style, SString & rBuf, const char * pFldName = 0, size_t indent = 0);
//
// Флаги представления строки базового типа
//
#define BTSF_PROGRAMMING 0x0001 // Английское наименование типа
#define BTSF_NATIVE      0x0002 // Наименование на национальном языке (понятное нормальным людям)
#define BTSF_OEM         0x0004 // В OEM-кодировке
//
// Descr: Заносит в буфер rBuf понятную нормальному человеку (не программисту) строку, обозначающую
//   тип данных, соответствующий параметру baseTyp.
//   Если функция не смогла идентифицировать тип baseTyp, то возвращается строка "Не определенный".
// ARG(baseType   IN): базовый тип, для которого необходимо сформировать наименование
// ARG(flags      IN): опции формирования строки (BTSF_XXX)
// ARG(rBuf      OUT): буфер, в который записывается строка наименования типа
// Returns:
//   rBuf
//
SString & GetBaseTypeString(int baseTyp, long flags, SString & rBuf);
TYPEID IsSTypeName(SStrScan & rScan);
//
// User-defined types support
//
void FASTCALL RegisterSType(TYPEID, DataType *);

#endif /* } !RC_INVOKED */
//
// } @ModuleDecl(SType)
//
//
// Common format flags
//
#define ALIGN_RIGHT    0x1000  // Выравнивать строку вправо
#define ALIGN_LEFT     0x2000  // Выравнивать строку влево
#define ALIGN_CENTER   0x3000  // Центрировать строку
#define COMF_FILLOVF   0x4000  // Если результирующая строка не помещается в указанную длину, то заполнять символами '#'
#define COMF_SQL       0x8000  // Форматировать значения в пригодном для SQL виде
	// Этот флаг передается в функции форматирования различных типов для того, чтобы они смогли
	// правильно сформировать строковое представление данных.
//
// Number format flags
//
#define NMBF_NONEG     0x0001  // Отрицательное значение заменяется пустой строкой
#define NMBF_NEGPAR    0x0002  // Отрицательное значение обрамляется круглыми скобками
#define NMBF_FORCEPOS  0x0004  // Перед положительным значением указывать знак '+'
#define NMBF_NOZERO    0x0008  // Нулевое значение заменяется пустой строкой
#define NMBF_TRICOMMA  0x0010  // Разделять тройки цифр запятой
#define NMBF_TRIAPOSTR 0x0020  // Разделять тройки цифр апострофом '
#define NMBF_TRISPACE  0x0040  // Разделять тройки цифр пробелом
#define NMBF_DELCOMMA  NMBF_TRICOMMA   // @obsolete (still used in resource)
#define NMBF_DELAPOSTR NMBF_TRIAPOSTR  // @obsolete (still used in resource)
#define NMBF_DELSPACE  NMBF_TRISPACE   // @obsolete (still used in resource)
#define NMBF_NOTRAILZ  0x0080  // Обрезать незначащие нули после десятичной точки
#define NMBF_DECCOMMA  0x0100  // @v9.4.3 Десятичный разделитель - запатая ',', иначе - точка '.'
//
// Text number format flags
//
#define NTTF_FEMIN     0x0001  // В женском роде
#define NTTF_CURRENCY  0x0002  // Добавить полное наименование валюты
#define NTTF_ABBRCURR  0x0004  // Добавить краткое наименование валюты
#define NTTF_DECCURR   0x0008  // Дробная денежная единица (коп.)
#define NTTF_FIRSTCAP  0x0010  // Первая буква заглавная //
#define NTTF_NOZERO    0x0020  // Не выводить нулевое значение
//
// Date format flags
//
#define DATF_AMERICAN       1  // mm/dd/yy
#define DATF_ANSI           2  // yy.mm.dd
#define DATF_BRITISH        3  // dd/mm/yy
#define DATF_FRENCH         4  // dd/mm/yy
#define DATF_GERMAN         5  // dd.mm.yy
#define DATF_ITALIAN        6  // dd-mm-yy
#define DATF_JAPAN          7  // yy/mm/dd
#define DATF_USA            8  // mm-dd-yy
#define DATF_MDY            9  // mm/dd/yy
#define DATF_DMY           10  // dd/mm/yy
#define DATF_YMD           11  // yy/mm/dd
// @v8.6.12 #define DATF_NUMBER        11  // Number of formats
#define DATF_SQL           12  // DATE 'YYYY-MM-DD'
#define DATF_INTERNET      13  // Wed, 27 Feb 2008
#define DATF_ISO8601       14  // yyyy-mm-dd
#define DATF_CENTURY   0x0010  // Century flag
#define DATF_NOZERO    0x0020  // Не отображать нулевое значение
#define DATF_NODIV     0x0040  // день, месяц и год следуют друг за другом без разделителей.
	// Порядок следования определяется младшими битами (DATF_AMERICAN..DATF_YMD). Если установлен
	// флаг DATF_CENTURY, то год должен быть полным (2011), в противном случае - только последние две
	// цифры года (11).
//
// Time format flags
//
#define TIMF_HMS            1  // 23:20:59
#define TIMF_HM             2  // 23:20
#define TIMF_MS             3  // 20:59
#define TIMF_S              4  // 59
#define TIMF_SQL            5  // TIMESTAMP 'YYYY-MM-DD HH:MM:SS.HS'
#define TIMF_MSEC      0x0008  // 23:20:59.019
#define TIMF_BLANK     0x0010  //
#define TIMF_NOZERO    TIMF_BLANK // Не отображать нулевое значение
#define TIMF_TIMEZONE  0x0020  // +0300
#define TIMF_NODIV     0x0040  // @v9.4.3
//
// String format flags
//
#define STRF_UPPER     0x0001  // Все буквы переводить в прописные
#define STRF_LOWER     0x0002  // Все буквы переводить в строчные
#define STRF_PASSWORD  0x0004  // Все символы заменять символом '*'
#define STRF_OEM       0x0008  // AnsiToOem (Windows only)
#define STRF_ANSI      0x0010  // OemToAnsi (Windows only)
//
// Structure of format field:
//   Precision           4 bit
//   Text output length 12 bit
//   Flags              16 bit
//
#define SFFLAGMASK  0x0000ffffL
#define SFLENMASK   0x0fff0000L
#define SFPRCMASK   0xf0000000L
#define SFALIGNMASK  ((unsigned long)(ALIGN_LEFT | ALIGN_RIGHT | ALIGN_CENTER))
#define SFCOMMONMASK ((unsigned long)(SFALIGNMASK | COMF_FILLOVF))
#define MKSFMT(l,f)           ((((long)(l))<<16)|(long)(f))
#define MKSFMTD(l,p,f)        ((((long)(p))<<28)|(((long)(l))<<16)|(long)(f))
#define SFMTFLAG(fmt)         (unsigned)((fmt)&SFFLAGMASK)
#define SFMTLEN(fmt)          ((uint16)(((fmt)&SFLENMASK)>>16))
#define SFMTPRC(fmt)          (((fmt)&SFPRCMASK)>>28)
#define SFMTALIGN(fmt)        ((fmt)&SFALIGNMASK)
#define SETSFMTLEN(fmt,l)     (fmt)=((((long)(l))<<16)|(long)((fmt)&~SFLENMASK))
#define SETSFMTPRC(fmt,p)     (fmt)=((((long)(p))<<28)|(long)((fmt)&~SFPRCMASK))
#define SETSFMTFLAG(fmt,f)    (fmt)=(((long)(f))|(long)((fmt)&~SFFLAGMASK))

#define SFMT_MONEY            MKSFMTD(0,2,0)
#define SFMT_QTTY             MKSFMTD(0,3,0)

#ifndef RC_INVOKED
//
// Descr: Блок форматов вывода различных типов данных в строку.
//   Используется при форматировании бинарных данных.
//
struct SFormatParam {
	SLAPI  SFormatParam();
	enum {
		fFloatSize = 0x0001, // Выводимая строка может иметь произвольную длину (длина поля,
			// содержащаяся в форматах игнорируется).
		fQuotText  = 0x0002  // Текстовые поля должны обрамляться двойными кавычками.
	};
	long   Flags;
	long   FDate;
	long   FTime;
	long   FStr;
	long   FReal;
};
//
// Descr: Представление целочисленного диапазона (low..upp).
//
struct IntRange { // @persistent @size=8
	//
	// Descr: Возвращает !0 если (this->low == 0 && this->upp == 0)
	//
	int    SLAPI IsZero() const;
	//
	// Descr: Проверяет значение test на принадлежность закрытому диапазону this
	//   (то есть, равенство граничному значению означает принадлежность всему диапазону).
	//   Если low == 0, то нижняя граница не тестируется.
	//   Если upp == 0, то верхняя граница не тестируется.
	// Returns:
	//   !0 - значение test принадлежит закрытому диапазону low..upp
	//   0  - значение test не принадлежит диапазону
	//
	int    FASTCALL CheckVal(int test) const;
	int    FASTCALL operator == (const IntRange & rS) const
	{
		return (low == rS.low && upp == rS.upp);
	}
	int    FASTCALL operator != (const IntRange & rS) const
	{
		return (low != rS.low || upp != rS.upp);
	}
	//
	// Descr: Присваивает экземпляру this значение объекта rS.
	// Returns:
	//   *this
	//
	IntRange & FASTCALL operator = (const IntRange & rS);
	//
	// Descr: Присваивает экземпляру this диапазон val..val
	// Returns:
	//   *this
	//
	IntRange & FASTCALL operator = (int val);
	//
	// Descr: Присваивает экземпляру this диапазон aLow..aUpp
	// Returns:
	//   *this
	//
	IntRange & FASTCALL Set(int aLow, int aUpp);
	//
	// Descr: Присваивает экземпляру this диапазон val..val
	// Returns:
	//   *this
	//
	IntRange & FASTCALL Set(int val);
	//
	// Descr: Вычисляет пересечение дипазонов *this и rD.
	// ARG(rD IN): Диапазон, с которым сравнивается *this
	// ARG(pResult OUT): @#{vptr0} Указатель на пересечение диапазонов.
	//   Если диапазоны не пересекаются, то pResult->low == pResult->upp == 0.
	// Returns:
	//   0 - диапазоны не пересекаются.
	//   1 - диапазоны пересекаются.
	//   2 - диапазоны пересекаются в одной точке.
	//  -1 - один или оба диапазона имеют перевернутые границы (low > upp).
	//
	int    SLAPI Intersect(const IntRange & rD, IntRange * pResult) const;
	SString & SLAPI Format(long fmt, SString &) const;

	int32  low, upp;
};
//
//
//
struct SSzChunk {
	// @noconstructor
	// @novtbl
	SSzChunk & Clear();
	SSzChunk & Set(int begin, int len);
	SSzChunk & SetRange(int begin, int end);
	int    operator !() const;
	int    operator == (const SSzChunk & rS) const;
	int    GetEnd() const;
	int    IsEmpty() const;

	int    Begin;
	int    Len;
};
//
//
//
struct RealRange {         // @persistent @size=16
	int    SLAPI IsZero() const;
	int    FASTCALL IsEqual(const RealRange & rS) const;
	double SLAPI GetDistance() const;
	double SLAPI GetActualLow() const
	{
		return MIN(low, upp);
	}
	double SLAPI GetActualUpp() const
	{
		return MAX(low, upp);
	}
	RealRange & Clear();
	RealRange & FASTCALL operator = (const IntRange & rS);
	int    FASTCALL operator != (const RealRange & rS) const;
	//
	// Descr: Проверяет значение v на принадлежность закрытому диапазону [low..upp]
	//
	int    SLAPI Check(double v) const;
	//
	// Descr: Проверяет значение v на принадлежность закрытому диапазону [low..upp]
	//   если low <= upp, в противном случае - на принадлежность диапазону [upp..low].
	//
	int    SLAPI CheckX(double v) const;
	//
	// Descr: Проверяет значение v на принадлежность закрытому диапазону [low..upp]
	//   с условием, что, если граница (low или upp) имеет значение 0.0, то она
	//   не проверяется.
	//   Например,
	//     -1 удовлетворяет диапазону [0..5.2]
	//     10 удовлетворяет диапазону [-3.1..0]
	//
	int    SLAPI CheckVal(double) const;
	double SLAPI Clip(double v) const;
	RealRange & SLAPI SetVal(double);
	RealRange & SLAPI Set(double, double);
	RealRange & SLAPI SetupMinMax(double v);
	//
	// Descr: Устанавливает диапазон [lo..lo+delta]
	//
	RealRange & SLAPI SetDelta(double lo, double delta);
	RealRange & SLAPI Scale(double sc);
	//
	// Descr: вызывает round(low, prec) и round(upp, prec)
	//
	RealRange & FASTCALL Round(int prec);
	//
	// Descr: Устанавливает диапазон [middle-delta..middle+delta]
	//
	RealRange & SLAPI SetMiddle(double middle, double delta);

	double low, upp;
};

struct DateRange { // @persistent @size=8
	DECL_INVARIANT_C();
	int    FASTCALL operator == (const DateRange & rS) const
	{
		return IsEqual(rS);
	}
	int    FASTCALL operator != (const DateRange & rS) const
	{
		return !IsEqual(rS);
	}
	int    FASTCALL IsEqual(const DateRange & rS) const;
	int    FASTCALL Cmp(const DateRange & rS) const;
	//
	// Descr: Возвращает длину интервала в днях.
	//   Если интервал бесконечный с обеих сторон, то возвращает (MAXLONG-1),
	//   Если интервал открыт слева, но закрыт справа, то возвращает количество дней от начала эры до
	//   правой границы.
	//   Если интервал открыт справа, но закрыт слева, то возвращает (MAXLONG-2)
	//   Для интевалов, нижняя граница которых превышает верхнюю возвращает отрицательное значение.
	//
	long   SLAPI GetLength() const;
	int    SLAPI IsZero() const;
	int    FASTCALL CheckDate(LDATE) const;
	int    FASTCALL Intersect(const DateRange *);
	int    SLAPI IsIntersect(DateRange r) const;
	DateRange & SLAPI Set(LDATE, LDATE);
	//
	// Descr: Если pSrc != 0, то присваивает экземпляру *this значение *pSrc,
	//   в противном случае вызывает SetZero()
	//
	DateRange & FASTCALL Set(const DateRange * pSrc);
	DateRange & SLAPI SetZero();
	void   FASTCALL SetDate(LDATE);
	int    SetPeriod(LDATE date, int prd);
	void   SLAPI CheckAndSwap();
	//
	// Descr: Корректирует нижнюю и верхнюю границы диапазона в согласии с датой dt.
	//   Если нижняя граница ниже dt, то она становится равной dt.
	//   Если верхняя граница больше dt, то она становится равна dt.
	//   Если dt.getclass() != LDATE::cNormal, то функция ничего не делает.
	// Returns:
	//   >0 - функция откорректировала значение периода
	//   -1 - дата находится в пределах диапазона this
	//   -2 - дата не относится к классу LDATE::cNormal
	//    0 - дата относится к классу LDATE::cInvalid
	//
	int    FASTCALL AdjustToDate(LDATE dt);
	//
	// Descr: Заменяет относительные даты в периоде (если такие есть) абсолютными
	//   относительно даты rel. Если rel == ZERODATE, то rel считается равной getcurdate_()
	//
	DateRange & FASTCALL Actualize(LDATE rel);
	DateRange & SLAPI ActualizeCmp(LDATE rel, LDATE cmp);
	LDATE  low, upp;
};

inline int FASTCALL operator > (const DateRange & r1, const DateRange & r2)
	{ return BIN(r1.Cmp(r2) > 0); }
inline int FASTCALL operator < (const DateRange & r1, const DateRange & r2)
	{ return BIN(r1.Cmp(r2) < 0); }

struct TimeRange {
	void   SLAPI Set(LTIME lowTm, LTIME uppTm);
	void   SLAPI SetZero();
	//
	// Descr: Проверяет принадлежность момента tm диапазону low..upp.
	//   -- Если low == upp && low == ZEROTIME, то не зависимо от значения tm возвращается !0.
	//   -- Если low <= upp, то возвращается !0 если (tm >= low && tm <= upp) и 0 в противном случае
	//   -- Если low > upp, то !0 возвращается в случае, если tm принадлежит одному из двух периодов: [upp..24:00:00[ или [00:00:00..low].
	//
	int    FASTCALL Check(LTIME tm) const;

	LTIME  low;
	LTIME  upp;
};
//
// Range functions
//
//
// Descr: Проверяет число nmb на принадлежность диапазону [low, upp].
//   Принадлежность включительная (т.е. если число равно одной из границ, то условие выполняется).
//
int    FASTCALL checkirange(long nmb, long low, long upp);
//
// Descr: Проверяет беззнаковое целое число на принадлежность диапазону [0..upper[.
//   Принадлежность НЕ включительная (nmb == upper - возвращает 0).
//
int    FASTCALL checkupper(uint nmb, uint upper);
int    FASTCALL checkfrange(double nmb, double low, double upp);
int    FASTCALL checkdrange(LDATE dt, LDATE low, LDATE upp);
//
// Descr: Величина, ассоциированная с единицей измерения.
//
struct USize {
	// @noconstructor @novtbl
	USize & Set(double s, int unit = 0, int dir = DIREC_UNKN);
	void   SetInvalid();
	int    IsValid() const;
	int    operator !() const
	{
		return !IsValid();
	}
	operator double() const
	{
		return S;
	}
	enum {
		fmtDefault = 0,
		fmtSVG
	};
	//
	// Descr: Извлекает величину из строки.
	//
	int    FromStr(const char * pStr, int fmt = fmtDefault);
	SString & ToStr(SString & rBuf, int fmt); // @notimplemented

	double S;
	int16  Unit;   // UNIT_XXX
	int16  Dir;    // DIREC_XXX, -1 означает инвалидную величину
};
//
//
//
class SUnit { // @persistent
public:
	//
	// Размерности
	//
	enum {
		clsLength = 1,        // Длина
		clsMass,              // Масса
		clsTime,              // Время
		clsAngle,             // Угол
		clsVolume,            // Объем
		clsPart,              // Часть (1/4, percent, etc)
		clsItem,              // Штука

		clsArea,              // Площадь
		clsCurrent,           // Сила тока
		clsAmount,            // Количество вещества (mole)
		clsSolidAngle,        // Сплошной угол
		clsMoney,             // Денежная единица
		clsForce,             // Сила
		clsPressure,          // Давление
		clsEnergy,            // Энергия (работа)
		clsTemperature,       // Температура
		clsCharge,            // Электрический заряд
		clsCapacitance,       // Электрическая емкость
		clsResistance,        // Электрическое сопротивление
		clsConductance,       // Электрическая проводимость
		clsInductance,        // Электрическая индуктивность
		clsFrequence,         // Частота
		clsVelocity,          // Скорость
		clsAcceleration,      // Ускорение
		clsDensity,           // Плотность
		clsLinearDensity,     // Линейная плотность
		clsViscosity,         // Вязкость
		clsKinematicViscosity // Кинематическая вязкость
	};
	class Context {
	public:
		Context();
		virtual ~Context();
		virtual int Describe(int unitId, int dir, int * pCls, double * pToBase, SString * pName) const;
	};

	static USize & Convert(const USize & rUszFrom, USize & rUszTo, const SUnit::Context * pCtx = 0);

	SUnit();
	SUnit(int id);
	SUnit(const char * pName, const SUnit::Context * pCtx);
	operator int () const;
	int    GetCls() const;
	int    GetName(long flags, SString &);
private:
	friend class SUnit::Context;

	static const void * FASTCALL SearchEntry(int unitId);
	static const void * FASTCALL SearchClsEntry(int cls);
	int    Id; // UNIT_XXX
};
//
//
//
class FPoint {
public:
	//
	// Descr: Конструктор по умолчанию. Ради скорости ничего не инициализирует.
	//
	SLAPI  FPoint();
	//
	// Descr: Конструктор. Инициализирует X и Y в xy.
	//
	SLAPI  FPoint(float xy);
	SLAPI  FPoint(float x, float y);
	FPoint & operator = (const TPoint & p);
	FPoint & operator = (float f);
	int    SLAPI IsZero() const;
	int    SLAPI IsPositive() const;
	FPoint SLAPI Set(float xy);
	FPoint SLAPI Set(float x, float y);
	FPoint SLAPI SetZero();
	FPoint SLAPI Neg() const;
	FPoint SLAPI Scale(float);
	float  SLAPI Ratio() const;
	float  SLAPI Add() const;
	FPoint SLAPI AddX(float x) const;
	FPoint SLAPI AddY(float y) const;
	float  SLAPI Sub() const;
	float  SLAPI Sq() const;
	//
	// Descr: Возвращает гипотенузу прямого угла с катетами X и Y.
	// Note: Ошибки нет - функция возвращает double.
	//
	double SLAPI Hypot() const;
	float  SLAPI Hypotf() const;
	//
	// Descr: (a.X*X + a.Y*Y, b.X*X + b.Y*Y)
	//
	FPoint SLAPI Combine(FPoint a, FPoint b) const;
	FPoint SLAPI Swap() const;
	int    SLAPI Write(SBuffer &) const;
	int    SLAPI Read(SBuffer &);

	float  X;
	float  Y;
};

extern const FPoint ZEROFPOINT; // @global

int    SLAPI operator == (FPoint p1, FPoint p2);
int    SLAPI operator <  (FPoint p1, FPoint p2);
int    SLAPI operator >  (FPoint p1, FPoint p2);
FPoint SLAPI operator +  (FPoint p, float);
FPoint SLAPI operator +  (FPoint p1, FPoint p2);
FPoint SLAPI operator -  (FPoint p1, FPoint p2);
FPoint SLAPI operator *  (FPoint p, float);
FPoint SLAPI operator *  (FPoint p, double);
FPoint SLAPI operator *  (FPoint p1, FPoint p2);
FPoint SLAPI operator /  (FPoint p, float);
FPoint SLAPI operator /  (FPoint p1, FPoint p2);
FPoint SLAPI fmin(FPoint p1, FPoint p2);
FPoint SLAPI fmax(FPoint p1, FPoint p2);
//
// Descr: Возвращает atan2f(p1.Y-p2.Y, p1.X-p2.X)
//
float  SLAPI atan2(FPoint p1, FPoint p2);
//
// Descr: X = p.X + radius.X * sinf(angle), Y = p.Y - radius.Y * cosf(angle)
//
FPoint SLAPI trans01(FPoint p, FPoint radius, float angle);
//
// Complex compare struct and parse functions
//
struct CompareTerm {
	int    type;
	int    cmp;
	char   data[10];
	int    link;
};

typedef int (SLAPI * is_data_func)(char *);
typedef int (SLAPI * get_data_func)(char * buf, int * idx, char * data);
//
// Compare sign sets
//
#define CMPSS_FORTRAN  0
#define CMPSS_SQL      1
#define CMPSS_C        2
//
// Boolean link sign sets
//
#define BOOLSS_SQL     0
#define BOOLSS_C       1
//
// Function prototypes
//
int SLAPI  is_number(char *);
int SLAPI  is_date(char *);
int SLAPI  get_number(char *, int *, char *);
int SLAPI  get_date(char *, int *, char *);

int SLAPI  set_data_func(is_data_func, get_data_func);
int SLAPI  check_cmp_expression(char * pStr, IntRange * pErrLoc);
int SLAPI  parse_cmp_expression(char *, int *, CompareTerm *);
int SLAPI  get_cmp_parse_error();

const char * SLAPI getcmpsign(int cmp, int set);
const char * SLAPI getboolsign(int link, int set);
//
// String functions
//

//
// Descr: Заменяет специальные символы в строке rBuf xml-сущностями.
// ARG(rBuf      INOUT): Строка, в которой должны быть заменены символы
// ARG(pProcessSymb IN): @vptr0 Либо 0, либо строка, содержащая полный список символов,
//   которые следует заменить. Если первым символом pProcessSymb является '-', то
//   последующие символы воспринимаются как те, которые заменять не следует.
//   Символ '&' в списке исключений игнорируется (он присутствует в представлении сущностей, потому не может не замещаться).
//
void FASTCALL XMLReplaceSpecSymb(SString & rBuf, const char * pProcessSymb);
//int  FASTCALL XMLReplaceSpecSymb(char * pBuf, int onlyStandart = 0);
//int  FASTCALL XMLReplaceSpecSymb(SString & rBuf, int onlyStandart = 0);
int  SLAPI XMLWriteSpecSymbEntities(FILE * pStream);
int  SLAPI XMLWriteSpecSymbEntities(void * pWriter);

#ifndef _WIN32_WCE // {
//
// Descr: Класс, реализующий препроцессинг образца и поиск образца в строке
//
class SSrchPattern {
public:
	enum {
		algDefault = 0, // Алгоритм, используемый по умолчанию
		algBmBadChr,    // Алгоритм, использующий только правило плохого символа
		algBmGoodSfx,   // Алгоритм, использующий правило плохого символа и расширенное правило хорошего суффикса
	};
	enum {
		fNoCase = 0x0001, // Поиск без учета регистра символов
		fDebug  = 0x8000  // Класс собирает отладочную статистику и проверяет корректность алгоритмов
	};
	SSrchPattern(const char * pPattern, long flags = 0, int algorithm = algBmGoodSfx);
	~SSrchPattern();
	int    Init(const char * pPattern, long flags = 0, int algorithm = algBmGoodSfx);
	size_t GetLen() const;
	int    Search(const char * pText, size_t start, size_t end, size_t * pPos) const;
private:
	int    AllocPreprocBuf();
	int    Calc_Z(uint16 * pZ, int test);
	int    Calc_N(uint16 * pN, int test);
	int    Calc_L(const uint16 * pN, uint16 * pL, uint16 * pl);
	int    Preprocess();
	int    Search_GS(const char * pText, size_t start, size_t end, size_t * pPos) const;
	int    Search_BC(const char * pText, size_t start, size_t end, size_t * pPos) const;
	//
	// Debug methods
	//
	int    Cmp_Vect(const uint16 *, const uint16 *) const;
	SString & OutputPattern(SString & rBuf) const;
	SString & OutputVect(const uint16 * pVect, SString & rBuf) const;

	uint8  * P_Pat;        // Буфер, содержащий образец для поиска. P_Pat >= P_PatAlloc && P_Pat <= P_PatAlloc+3
	uint16 * P_PreprocBuf; // Буфер, содержащий данные препроцессинга. Длина и содержимое зависят от алгоритма.
	size_t Len;            // Длина образца для поиска
	int    Alg;            // Алгоритм, используемый для препроцессинга и поиска (algXXX)
	long   Flags;
	void * P_PatAlloc;     // Указатель на распределенный для образца буфер
	size_t PatSize;        // Длина буфера, выделенного под P_PatAlloc
	size_t PreprocSize;    // Длина буфера, выделенного под препроцессинг
	size_t HashSize;       // Количество элементов хэш-таблицы для правила плохого двойного слова
};

#endif // } _WIN32_WCE
//
// Descr: Класс, реализующий механизм работы со строками.
//
#define STRINGIZING(var) #var // заключает выражение в кавычки
#define CAT_FLD(fld, str)     str.Cat(#fld).CatChar('=').Cat(fld)
#define CAT_FLD_HEX(fld, str) str.Cat(#fld).CatChar('=').CatHex(fld)

#define _CTRANSF_INNER         0x01
#define _CTRANSF_OUTER         0x02
#define _CTRANSF_UTF8          0x03

#define CTRANSF_INNER_TO_OUTER 0x0102
#define CTRANSF_OUTER_TO_INNER 0x0201
#define CTRANSF_INNER_TO_UTF8  0x0103
#define CTRANSF_OUTER_TO_UTF8  0x0203
#define CTRANSF_UTF8_TO_INNER  0x0301
#define CTRANSF_UTF8_TO_OUTER  0x0302

class SString {
public:
	DECL_INVARIANT_C();
	SLAPI  SString();
	SLAPI  SString(size_t initSize);
	SLAPI  SString(const char *);
	SLAPI  SString(const SString &);
	SLAPI ~SString();
	void   SLAPI Destroy();
	SString & FASTCALL operator = (const SString & rS);
	SString & FASTCALL operator = (const char * pS);
	//
	// Descr: Фактически, оператор присваивания. Из-за того, что очень часто
	//   используется конструкция string = 0, которая компилятором интерпретируется как operator = (const char *)
	//   перегрузить operator = с аргументом (const uchar *) не удается (string = 0) не может однозначно трактоваться.
	//
	SString & FASTCALL Set(const uchar * pS);
	//
	// Descr: Возвращает SString::Cmp(pS, 0) == 0
	//   То есть, если строка this равна строке pS с учетом регистра символов.
	//
	int    FASTCALL operator == (const char * pS) const;
	int    FASTCALL operator != (const char * pS) const;
	int    FASTCALL operator == (const SString & rS) const;
	int    FASTCALL operator != (const SString & rS) const;
	//
	// Descr: Возвращает символ, находящийся в позиции n (n = 0..)
	//   Если n больше длины строки, то возвращает 0.
	//
	int    FASTCALL C(size_t n) const;
	int    SLAPI Empty() const { return (Len() == 0) ? 1 : 0; }
	//int    SLAPI operator !() const { return Empty(); }
	int    SLAPI NotEmpty() const { return !Empty(); }
	//
	// Descr: Возвращает !0 если после применения метода Strip строка становится пустой.
	//
	int    SLAPI NotEmptyS();
	int    FASTCALL IsEqual(const SString & rS) const;
	int    FASTCALL IsEqual(const char * pS) const;
	operator const char * () const { return P_Buf; }
	//
	// Descr: Специализированное преобразование, сделанное главным образом ради (xmlChar *).
	// Note:
	//   Здесь нелья создавать дополнительный перегруженный оператор operator const uchar * ()
	//   из-за того, что существующие использования класса столкнутся с неоднозначностью
	//   преобразования.
	//
	const uchar * ucptr() const { return (uchar *)P_Buf; }
	//
	// Descr: Сокращенная версия operator const char * ()
	//
	const char  * cptr() const { return (char *)P_Buf; }
	//
	// Descr: Сравнивает строку this со строкой pS.
	// ARG(pS         IN): Строка, с которой сравнивается this
	// ARG(ignoreCase IN): Сравнение реализуется без учета регистра символов
	// Returns:
	//  <0 - this < pS
	//  >0 - this > pS
	//  0  - this == pS
	//
	int    FASTCALL Cmp(const char * pS, int ignoreCase) const;
	int    FASTCALL CmpNC(const char * pS) const;
	//
	// Descr: Сравнивает префикс строки this со строкой pS.
	// Returns:
	//   0  - префикс строки this совпадает со строкой pS
	//   !0 - строка pS не совпадает с префиксом this
	//   Если pS == 0 или this->Len() < strlen(pS), то функция возвращает -1
	//
	int    FASTCALL CmpPrefix(const char * pS, int ignoreCase) const;
	//
	// Descr: Сравнивает строку this c префиксом строки pS длиной this->Len().
	//   Если this->Len() == 0, то результат в любом случае -1.
	//
	int    FASTCALL CmpL(const char * pS, int ignoreCase) const;
	int    FASTCALL CmpSuffix(const char * pS, int ignoreCase) const;
	//
	// Descr: Ищет совпадения между строкой this и одной из подстрок
	//   строки pPattern. Подстроки разделяются символом div.
	// Returns:
	//   0 - строка this не совпадает ни с одной из подстрок pPattern
	//  !0 - @#[1..] номер подстроки из pPattern с которой совпала this.
	// Sample: OneOf(';', "yes;true;1", 1)
	//
	uint   SLAPI OneOf(int div, const char * pPattern, int ignoreCase) const;
	int    SLAPI Last() const;
	char * FASTCALL CopyTo(char *, size_t) const;
	BSTR   FASTCALL CopyToOleStr(BSTR * pBuf) const;
	SString & FASTCALL CopyFromOleStr(const BSTR s);

	size_t SLAPI Len() const { return L ? (L-1) : 0; }
	size_t SLAPI BufSize() const;
	void   SLAPI Obfuscate();
	const  char * SLAPI StrChr(int c, size_t * pPos = 0) const;
	//
	// Descr: Определяет факт наличия в строке this символа c.
	//
	int    FASTCALL HasChr(int c) const;
	//
	// Descr: проверяет на принадлежность символов строки диапазону [0-9]||[a-z]||[A..Z]
	// Returns:
	//   0 - не все символы из строки принадлежат указанному диапазону
	//   1 - все символы из строки принадлежат указанному диапазону
	//
	int    SLAPI IsLatin() const; // @v5.3.6 AHTOXA
	//
	// Descr: Сканирует слово, начиная с позиции *pPos. Если pPos == 0,
	//   то начинает сканирование с позиции 0. Разделителями слов являются все символы,
	//   которые не являются буквами и цифрами, а также символами '_'.
	// Returns:
	//   >0 - удалось извлечь слово, длина которого равна возвращаемому значению.
	//   0  - не удалось сосканировать ни одного символа.
	//
	int    SLAPI GetWord(size_t * pPos, SString & rBuf) const;
	int    SLAPI Tokenize(const char * pDelimChrSet, StringSet & rResult) const;
	//
	// Descr: Ищет в строке this подстроку, установленную в качестве образца в блок pBlk.
	// ARG(pBlk     IN): блок, в котором реализован препроцессинг образца для поиска.
	// ARG(startPos IN): позиция в строке this, с которой следует начинать поиск.
	// ARG(pPos    OUT): указатель, по которому в случае успешного поиска, присваивается позиция,
	//   с которой начинается искомый образец в строке this. Эта позиция отсчитывается //
	//   от начала строки this (не от позиции startPos).
	// Returns:
	//   0 - искомый образец в строке this, начиная с позиции startPos, не найден
	//  !0 - искомый образец найден в строке this и находится по смещению *pPos, начиная с первого
	//       символа строки this.
	//
#ifndef _WIN32_WCE
	int    SLAPI Search(const SSrchPattern * pBlk, size_t startPos, size_t * pPos) const;
#endif
		// @>>SSrchPattern::Search
	int    SLAPI Search(const char * pPattern, size_t startPos, int ignoreCase, size_t * pPos) const;
	//
	// Descr: Делит строку на две части, используя в качестве разделителя символ divChr.
	//   Левая часть копируется в буфер rLeft, а правая - в буфер rRight.
	//   Если в строке не обнаружен символ-разделитель, то rLeft = *this, а rRight = 0.
	// Returns:
	//   >0 - в строке имелся символ разделителя и строка успешно разбита на две части.
	//   <0 - в строке не было символа разделителя и вся строка скопирована в буфер rLeft.
	//     Буфер rRight стал пустым.
	//   0  - ошибка. Буферы rLeft и rRight стали пустыми.
	//
	int    SLAPI Divide(int divChr, SString & rLeft, SString & rRight) const;
	int    SLAPI Wrap(uint maxLen, SString & rHead, SString & rTail) const;
	int    SLAPI IsDigit() const;
	long   SLAPI ToLong() const;
	ulong  SLAPI ToULong() const;
	int64  SLAPI ToInt64() const;
	double SLAPI ToReal() const;
	//
	// Descr: Флаги функции ToIntRange
	//
	enum {
		torfDoubleDot   = 0x0001, // Допускается разделитель ..
		torfDoubleComma = 0x0002, // Допускается разделитель ,,
		torfColon       = 0x0004, // Допускается разделитель :
		torfHyphen      = 0x0008, // Допускается разделитель -
		torfAny         = (torfDoubleDot|torfDoubleComma|torfColon|torfHyphen)
	};
	//
	// Descr: Преобразует строку this к целочисленному диапазону.
	//   Диапазон может быть представлен в следующих видах:
	//   Просто число: 19
	//   Два числа, разделенные двумя последовательными точками: 19..37
	//   Два числа, разделенные знаком тире '-': 19-37 (обратите внимание, что -19--12 будет воспринято как [-19:-12],
	//      а -19-12 как [-19:12].
	//   Два числа, разделенные знаком двоеточия ':': 19:37
	//   Два числа, разделенные двумя последовательными запятыми: 19,,37
	//   Пробелы и табуляции перед диапазоном, до разделителя и после разделителя игнорируются.
	//     например: " -19  ..  38" будет воспринято как валидный диапазон.
	// Note: Функция не обращает внимание на упорядоченность нижней и верхней границ. То есть,
	//   строка "125..98" вернет диапазон [125:98] не меняя границы местами.
	//   Для гарантированной упорядоченности границ можно использовать вызов ExchangeToOrder(&range.low, &range.upp).
	//   Кроме того, функция не проверяет значения чисел на переполнение.
	// ARG(rRange OUT): Целочисленный диапазон, полученный в результате разбора строки
	// ARG(flags  IN): Флаги, регулирующие возможные форматы. See SString::torfXXX
	// Returns:
	//   0  - не удалось извлечь диапазон из строки. В этом случае значение rRange не изменяется.
	//   >0 - смещение от начала строки, на котором закончился успешный разбор диапазона.
	//
	int    SLAPI ToIntRange(IntRange & rRange, long flags) const;
	SString & SLAPI Sub(size_t startPos, size_t len, SString & rBuf) const;

	SString & SLAPI TrimRight();
	//
	// Descr: Обрезает самый правый символ строки, если он равен c.
	//
	SString & FASTCALL TrimRightChr(int c);
	//
	// Descr: Обрезает строку справа до позиции n. В итоге длина строки становится не более n.
	//   Если n == 0, то строка становится пустой.
	//   Если n >= Len(), то ничего не происходит.
	// @#out{Len() <= n}
	//
	SString & FASTCALL Trim(size_t n);
	//
	// Descr: Обрезает строку справа, оставляя в строке не более чем n символов.
	//   Если в подстроке [0..n-1] встречается один из символов, перечисленных в
	//   параметре pDivList, то строка обрезается по самому крайне правому в подстроке [0..n-1]
	//   из этих символов (не включая сам символ).
	//
	SString & SLAPI TrimToDiv(size_t n, const char * pDivList);
	//
	// Descr: Вырезает из середины строки, начиная с позиции start, отрезок размером len.
	//
	SString & SLAPI Excise(size_t start, size_t size);
	//
	// Descr: сдвигает строку на n позиций влево. n левых символов теряются.
	//   Если длина строки меньше n, то длина строки становится нулевой.
	//
	SString & FASTCALL ShiftLeft(size_t n = 1);
	//
	// Descr: сдвигает строку на 1 позицию влево если первый символ в строке равен chr.
	//
	SString & FASTCALL ShiftLeftChr(int chr);
	SString & SLAPI Reverse();
	SString & SLAPI PadLeft(size_t n, int pad);
	SString & SLAPI Align(size_t width, int adj);
	SString & SLAPI Chomp();
	SString & SLAPI Quot(int leftQuotChar, int rightQuotChar);
	SString & SLAPI ReplaceChar(int patternChr, int replaceChr);
	//
	// Descr: В строке this замещает подстроки pPattern подстроками pReplacer.
	//   Если once != 0, то производит только одну замену с начала строки.
	// Returns:
	//   Количество произведенных замен.
	//
	int    SLAPI ReplaceStrR(const char * pPattern, const char * pReplacer, int once);
	//
	// Descr: В строке this замещает подстроки pPattern подстроками pReplacer.
	//   Если once != 0, то производит толко одну замену с начала строки.
	// Returns:
	//   *this.
	//
	SString & SLAPI ReplaceStr(const char * pPattern, const char * pReplacer, int once);
	SString & FASTCALL ReplaceSpecSymb(int fileFormat);
	SString & FASTCALL RevertSpecSymb(int fileFormat);
	SString & SLAPI ReplaceCR();
	SString & SLAPI Escape();
	SString & SLAPI Unescape();
	SString & SLAPI ToUrl();
	SString & SLAPI FromUrl();
	SString & SLAPI ToOem();
	SString & SLAPI ToChar();
	SString & SLAPI ToUtf8();
	SString & SLAPI Utf8ToChar();
	SString & SLAPI Utf8ToOem();
	SString & SLAPI Utf8ToCp(SCodepageIdent cp);
	SString & SLAPI Utf8ToLower();
	SString & SLAPI Utf8ToUpper();

	SString & SLAPI ToUpper();
	SString & SLAPI ToLower();
	SString & SLAPI ToUpper1251();
	SString & SLAPI ToLower1251();
	SString & FASTCALL SetCase(int ccas);
	//
	SString & FASTCALL Transf(int ctransf /*CTRANSF_XXX*/);
	//
	// Descr:
	//
	SString & NumberToLat(uint value);
	//
	// ARG(dir IN): 1 - обрезать справа, 2 - обрезать слева, 0 - обрезать с обоих концов
	//
	SString & SLAPI Strip(int dir);
	SString & SLAPI Strip();
	SString & SLAPI StripQuotes();
	SString & FASTCALL CopyFrom(const SString &);
	SString & FASTCALL CopyFrom(const char *);
	SString & FASTCALL SetIfEmpty(const char *);
	int    SLAPI IsLegalUtf8() const;
	int    SLAPI CopyUtf8FromUnicode(const wchar_t * pSrc, const size_t len, int strictConversion);
	//
	// Descr: Копирует в нашу строку не более maxLen символов из строки pSrc.
	//   Если завершающий ноль в строке pStr встретится раньше, чем будут скопированы
	//   maxLen символов, то копирование прекращается.
	//
	SString & FASTCALL CopyFromN(const char * pSrc, size_t maxLen);
	//
	// Descr: Извлекает из коллекции строк, разделенных символом div строку с номером idx
	//   и копирует ее в this.
	// ARG(pStr IN): Указатель на коллекцию строк, разделенных символом-разделителем.
	// ARG(div  IN): Символ-разделитель.
	// ARG(idx  IN): @[0..] Индекс строки, которую необходимо извлечь
	// Returns:
	//   >0 - строка с индексом idx найдена и скопирована в this.
	//   0  - в коллекции строк меньше, чем индекс idx. Строка this становится пустой.
	//
	int       SLAPI GetSubFrom(const char * pStr, int div, int idx);
	int       SLAPI GetIdxBySub(const char * pSubStr, int div);
	SString & FASTCALL Cat(const SString &);
	SString & FASTCALL Cat(const char *);
	SString & FASTCALL CatN(const char * pS, size_t maxLen);
	//
	// Descr: Дополняет текущую строку еще не считанными символами из
	//   буфера rS.
	//
	SString & FASTCALL Cat(SBuffer & rS); // @>>SString::CatN
	//
	// Descr: Дополняет текущую строку строкой, обрамленной двойными кавычками.
	//
	SString & FASTCALL CatQStr(const char * pStr);
	//
	// Descr: Дополняет текущую строку строкой, обрамленной круглыми скобками "()".
	//
	SString & FASTCALL CatParStr(const char * pStr);
	SString & FASTCALL CatParStr(long val);
	SString & FASTCALL CatBrackStr(const char * pStr);
	SString & FASTCALL CatChar(int chr);
	SString & SLAPI    Space();   // @>>CatChar(' ')
	SString & FASTCALL Tab(uint c); // @>>CatChar('\t')
	SString & SLAPI    Tab();       // @>>CatChar('\t')
	SString & SLAPI    Dot();     // @>>CatChar('.')
	SString & SLAPI    Comma();   // @>>CatChar(',')
	SString & SLAPI    Semicol(); // @>>CatChar(';')
	SString & SLAPI    Eq();      // @>>CatChar('=')
	SString & SLAPI    CR();      // @>>CatChar('\n')
	SString & SLAPI    CRB();     // "\xD\xA"
	SString & FASTCALL CatCharN(int chr, size_t n);
	SString & FASTCALL CatDiv(int c, int addSpaces/* = 0*//*, int ifNotEmpty = 0*/);
	SString & FASTCALL CatDivIfNotEmpty(int c, int addSpaces/* = 0*/);
	SString & FASTCALL Cat(long);
	SString & FASTCALL Cat(ulong);
#ifndef _WIN32_WCE // {
	SString & SLAPI    Cat(int64);
	SString & SLAPI    Cat(uint64);
#endif // } _WIN32_WCE
	SString & FASTCALL Cat(int);
	SString & FASTCALL Cat(uint);
	//
	// Descr: форматирует длинное целое число val в строку
	//   Если numDigits > 0 && numDigits <= 512, то форматирутся с количеством символов равным numDigits
	//   с набивкой слева нулями (например: CatLongZ(5, 4) --> buf="0005")
	//   Если (numDigits == 0 || numDigits > 512), то просто переводит val в строку форматом %ld.
	// Returns:
	//   *this
	//
	SString & FASTCALL CatLongZ(long val, uint numDigits);
	SString & FASTCALL CatLongZ(int val, uint numDigits)
	{
		return CatLongZ((long)val, numDigits);
	}
	SString & FASTCALL CatLongZ(uint val, uint numDigits)
	{
		return CatLongZ((long)val, numDigits);
	}
	SString & FASTCALL CatLongZ(uint32 val, uint numDigits)
	{
		return CatLongZ((long)val, numDigits);
	}
	SString & FASTCALL CatLongZ(int64 val, uint numDigits);
	SString & FASTCALL CatHex(long val);
	SString & FASTCALL CatHex(ulong val);
	SString & FASTCALL CatHex(int64 val);
	SString & FASTCALL CatHex(uint8 val);
	//
	// Descr: Конкатенация конструкции {"%d.%d.%d", ver, mjr, mnr}
	//
	SString & SLAPI CatDotTriplet(int ver, int mjr, int mnr);
	//
	// Descr: Конкатенация конструкции {pMsg ' ' ((100 * p) / t) '%'}
	//   Если pMsg == 0, то конкатенируется только процент.
	//
	SString & SLAPI CatPercentMsg(long p, long t, const char * pMsg);
	SString & SLAPI Cat(double v, long fmt);
	SString & SLAPI Cat(double v); // MKSFMTD(0, 6, NMBF_NOTRAILZ)
	SString & SLAPI CatReal(double v);
	SString & FASTCALL Cat(LDATE, long fmt); // @v9.6.4 defargremoved
	SString & FASTCALL Cat(LTIME, long fmt); // @v9.6.4 defargremoved
	//
	// Descr: == Cat(LDATE, DATF_DMY)
	//
	SString & FASTCALL Cat(LDATE); // @v9.6.4
	//
	// Descr: == Cat(LTIME, DATF_HMS)
	//
	SString & FASTCALL Cat(LTIME); // @v9.6.4
	SString & FASTCALL Cat(const LDATETIME &, long datFmt = DATF_DMY, long timFmt = TIMF_HMS);
	SString & FASTCALL Cat(const DateRange &, int ext = 0);
	SString & FASTCALL Cat(const S_GUID &, int fmt = S_GUID::fmtIDL);
	//
	// Descr: Конкатенация конструкции <text> с вариантами
	// ARG(kind IN):
	//   0 - <pTag>
	//   1 - </pTag>
	//   2 - <pTag/>
	//
	SString & SLAPI CatTagBrace(const char * pTag, int kind);
	SString & SLAPI CatTag(const char * pTag, const char * pData);
	SString & SLAPI CatXmlElem(const char * pName, int typ, StringSet * pList);
	SString & SLAPI CatEq(const char * pKey, const char * pVal);
	SString & SLAPI CatEqQ(const char * pKey, const char * pVal);
	SString & SLAPI CatEq(const char * pKey, uint16 val);
	SString & SLAPI CatEq(const char * pKey, uint val);
	SString & SLAPI CatEq(const char * pKey, long val);
	SString & SLAPI CatEq(const char * pKey, ulong val);
	SString & SLAPI CatEq(const char * pKey, int64 val);
	SString & SLAPI CatEq(const char * pKey, uint64 val);
	SString & SLAPI CatEq(const char * pKey, double val, long fmt = SFMT_QTTY);
	SString & SLAPI CatEq(const char * pKey, LDATE val, long fmt = DATF_DMY|DATF_CENTURY);
	SString & SLAPI CatEq(const char * pKey, LTIME val, long fmt = TIMF_HMS);
	//
	// Descr: Вставляет в позицию pos строки this другую строку pStr.
	//   Если pos > this->Len(), то ничего не делает,
	//   если pos == this->Len(), то this->Insert(this->Len(), pStr) == this->Cat(pStr)
	//
	SString & SLAPI Insert(size_t pos, const char * pStr);
	SString & __cdecl Printf(const char * pFormat, ...);
	SString & SLAPI SetLastSlash();
	SString & SLAPI RmvLastSlash();
	//
	// Descr: читает из потока fStream блок размером size. Считает, что
	//   size - это длина строки без завершающего нуля. Считывание начинается с
	//   текущей позиции потока.
	// ARG(fStream IN): поток как минимум открытый для чтения //
	// ARG(size    IN): размер блока, который следует считать из потока. Предполагается,
	//     что этот размер не включает в себя завершающего нуля.
	// Returns:
	//   >0 - строка считана успешно
	//   <0 - либо fStream, либо size равны нулю. Буфер обнуляется //
	//   0  - ошибка (SLibError)
	//
	int    SLAPI Read(FILE * fStream, size_t size);
	//
	// Descr: Кодирует (декодирует в зависимости от параметра decode) строку с помощью параметра pEncodeStr
	//   строка pEncodeStr содержит пары в виде: %01,254000;%02,255000,
	//   где 254000, 255000 и т.д. - кусок строки который нужно закодировать символами - %01, %02 и т.д
	//
	SString & EncodeString(const char * pSrc, const char * pEncodeStr, int decode);
	SString & SLAPI EncodeMime64(const void * pBuf, size_t bufLen);
	int    SLAPI DecodeMime64(void * pBuf, size_t bufLen, size_t * pRealLen) const;
	int    SLAPI DecodeHex(int swapb, void * pBuf, size_t bufLen, size_t * pRealLen) const;
	//
	// Descr: Утилитная функция, формирующая унифицированную строку сообщения при разборе файла.
	//   Необходимость функции обусловлена способностью многих IDE-систем разбирать сообщения (в частности, об ошибках),
	//   формируемые сторонними программами, в формате "filename(lineno): message"
	//
	SString & FormatFileParsingMessage(const char * pFileName, int lineNo, const char * pAddedText);
private:
	int    FASTCALL Alloc(size_t);
	SString & SLAPI ShiftRight(size_t n, int padChar);
	SString & SLAPI Helper_MbToMb(uint srcCodepage, uint destCodepage);
	// @v9.0.4 L поставлен перед Size ради ускорения
	size_t L;     // Длина строки вместе с завершающим нулем
	size_t Size;  // Размер распределенного буфера
	char * P_Buf; // Буфер, в котором хранится строка. @# if(P_Buf) then P_Buf[Len] == 0
};
//
//
//
class SStringU {
public:
	SLAPI  SStringU();
	SLAPI  SStringU(const SStringU &);
	SLAPI  SStringU(const wchar_t *);
	SLAPI ~SStringU();
	size_t SLAPI Len() const { return L ? (L-1) : 0; }
	operator const wchar_t * () const { return P_Buf; }
	SStringU & FASTCALL operator = (const SStringU &);
	SStringU & FASTCALL operator = (const wchar_t *);
	int    FASTCALL IsEqual(const SStringU & rS) const;
	int    FASTCALL IsEqual(const wchar_t * pS) const;
	int    FASTCALL Cmp(const SStringU & rS) const;
	//
	// Descr: Возвращает символ, находящийся в позиции n (n = 0..)
	//   Если n больше длины строки, то возвращает 0.
	//
	wchar_t    FASTCALL C(size_t n) const;
	SStringU & FASTCALL CatChar(wchar_t);
	SStringU & FASTCALL CatN(const wchar_t * pS, size_t maxLen);
	SStringU & FASTCALL CopyFrom(const SStringU & rS);
	SStringU & FASTCALL CopyFrom(const wchar_t * pS);
	SStringU & FASTCALL CopyFromN(const wchar_t * pS, size_t maxLen);
	SStringU & FASTCALL CopyFromMb(int cp, const char * pS, size_t srcLen);
	//
	// Descr: Копирует мультибайт-строку pS в this предполагая, OUTER-кодировку pS.
	//
	SStringU & FASTCALL CopyFromMb_OUTER(const char * pS, size_t srcLen);
	//
	// Descr: Копирует мультибайт-строку pS в this предполагая, INNER-кодировку pS.
	//
	SStringU & FASTCALL CopyFromMb_INNER(const char * pS, size_t srcLen);
	wchar_t *  FASTCALL CopyTo(wchar_t * pS, size_t maxLen);
	SStringU & SLAPI Sub(size_t startPos, size_t len, SStringU & rBuf) const;
	int    FASTCALL CopyFromUtf8(const char * pSrc, size_t srcSize);
	int    FASTCALL CopyFromUtf8R(const char * pSrc, size_t srcSize, size_t * pActualSrcSize);
	int    FASTCALL CopyFromUtf8(const SString & rS);
	int    FASTCALL CopyFromUtf8R(const SString & rS, size_t * pActualSrcSize);
	int    FASTCALL CopyFromUtf8Strict(const char * pSrc, size_t srcSize);
	int    FASTCALL CopyToUtf8(SString & rBuf, int strictConversion) const;
	int    SLAPI CopyToMb(int cp, SString & rBuf) const;
	SStringU & FASTCALL Trim(size_t n);
	SStringU & SLAPI ToUpper();
	SStringU & SLAPI ToLower();

	enum {
		tcEmpty = 0,
		tcMix,
		tcLow,
		tcUpp,
		tcCapital
	};

	int    SLAPI AnalyzeCase() const;

	long   SLAPI ToLong() const;
	int64  SLAPI ToInt64() const;
private:
	int    FASTCALL Alloc(size_t sz);
	int    FASTCALL Helper_CopyFromUtf8(const char * pSrc, size_t srcSize, int strictConversion, size_t * pActualSrcSize);

	size_t Size;
	size_t L;
	wchar_t * P_Buf;
};
//
// Функции преобразования регистра unicode символа
//
int     FASTCALL UGetCharCase(wchar_t code);
wchar_t FASTCALL UToUpperCase(wchar_t code);
wchar_t FASTCALL UToLowerCase(wchar_t code);
//
// StringSet
//
class StringSet {
public:
	SLAPI  StringSet(const char * pDelim = 0);
	SLAPI  StringSet(char delim, const char * pBuf);
	SLAPI  StringSet(const StringSet &);
	SLAPI ~StringSet();
	int    FASTCALL copy(const StringSet & rS);
	int    SLAPI setBuf(const void * b, size_t len);
	int    FASTCALL setBuf(const SString & rBuf);
	//
	// Descr: устанавливает разделитель строк.
	//   Пользоваться этой функцией следует только если в наборе
	//   нет ни одной строки. Иначе начнутся сбои.
	//
	void   FASTCALL setDelim(const char * pDelim);
	uint   SLAPI getDelimLen() const;
	uint   SLAPI getCount() const;
	int    FASTCALL add(const char *, uint * pPos);
	//
	// Descr: Функция добавляет в конец набора новую строку pStr.
	//   Главное отличие от add(const char *, uint *) только в отсутствии последнего параметра.
	//   Так как вызов add(str, 0) крайне популярен, то мы расчитываем получить некоторую
	//   экономию в размере бинарного кода.
	//
	int    FASTCALL add(const char * pStr); // @>>StringSet::add(const char *, uint * pPos)
	//
	// Descr: возвращает длину подстроки, начинающейся в позиции pos.
	//
	size_t FASTCALL getLen(uint pos) const;
	int    FASTCALL get(uint * pPos, char * pStr, size_t maxlen) const;
	int    FASTCALL get(uint * pPos, SString &) const;
	int    SLAPI get(uint pos, SString & rBuf) const;
	//
	// Descr: Специализированный вариант функции get(uint, SString &). Если pos == 0,
	//   то присваивает буферу rBuf нулевую строку, в остальных случаях работает
	//   как get(uint, SString &).
	//
	int    SLAPI getnz(uint pos, SString & rBuf) const;
	int    SLAPI search(const char * pPattern, uint * pPos, int ignoreCase) const;
	int    SLAPI search(const char * pPattern, CompFunc fcmp, uint * pPos, uint * pNextPos) const;
	void   FASTCALL clear(int dontFreeBuf = 0);
	//
	// Descr: Сортирует строки набора в лексикографическом порядке
	//
	int    SLAPI sort();
	//
	// Descr: Сортирует строки набора в лексикографическом порядке и удаляет из него дубликаты
	//
	int    SLAPI sortAndUndup();
	//
	// Descr: Переставляет элементы набора в обратном порядке: первый становится последним, последний - первым.
	//
	int    SLAPI reverse();
	StringSet & FASTCALL operator =  (const StringSet &);
	StringSet & FASTCALL operator +  (const char * s);
	StringSet & FASTCALL operator += (const char * s);
	char * SLAPI getBuf() const;
	//
	// Descr: Возвращает размер данных в буфере.
	//
	size_t SLAPI getDataLen() const;
	size_t SLAPI getSize() const;
	//
	// Descr: Записывает в буфер rBuf состояние экземпляра.
	//   Запись идет в бинарном формате по следующему порядку:
	//   Delim (32 bytes), DataLen (4 bytes), P_Buf (DataLen bytes)
	//
	int    FASTCALL Write(SBuffer & rBuf) const;
	//
	// Descr: Читает из буфера rBuf состояние экземпляра объекта.
	//   Предыдущее состояние разрушается. Чтение идет в том же формате и порядке,
	//   что и запись (см. StringSet::Write).
	//
	int    FASTCALL Read(SBuffer & rBuf);
	int    SLAPI Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
#ifndef _WIN32_WCE // {
	int    SLAPI Write(SFile & rFile, long) const;
	int    SLAPI Read(SFile & rFile, long);
#endif // } _WIN32_WCE
protected:
	int    SLAPI Init(const char * pDelim, size_t prealloc);
	int    FASTCALL Alloc(size_t sz);

	size_t DataLen; // @#{DataLen<=BufLen} Length of data with delimiters and trailings
	size_t Size; //BufLen;  // @#{BufLen>=DataLen} Length of buffer
	char * P_Buf;
	char   Delim[32];
};
//
//
//
struct SSearchReplaceParam {
	SSearchReplaceParam();
	SSearchReplaceParam & Reset();

	enum {
		fNoCase     = 0x0001, // Поиск без учета регистра символов
		fWholeWords = 0x0002, //
		fReverse    = 0x0004,
		fReplace    = 0x0008, //
		fDebug      = 0x8000  // Класс собирает отладочную статистику и проверяет корректность алгоритмов
	};
	long   Flags;
	SString Pattern;
	SString Replacer;
};
//
//
//
struct SPathStruc {
	//
	// Descr: Находит расширение имени файла rPath и возвращает его по указателю pExt (если тот не нулевой).
	// Returns:
	//   0 - путь rPath не содержит расширения
	//  !0 - позиция следующего за точкой первого символа расширения в rPath
	//
	static uint FASTCALL GetExt(const SString & rPath, SString * pExt);
	static int ReplaceExt(SString & rPath, const char * pExt, int force);
	static int ReplacePath(SString & rPath, const char * pNewPath, int force);
	static int GetRelativePath(const char * pFrom, uint dwAttrFrom, const char * pTo, uint dwAttrTo, SString & rPath);
	//
	// Descr: Флаги функции SPathStruc::NormalizePath
	//
	enum {
		npfUpper = 0x0001, // Все символы приводятся к верхнему регистру (иначе - к нижнему)
		npfSlash = 0x0002, // Все обратные слэши заменяются на прямые (иначе - все обратные)
		npfOEM   = 0x0004  // Входящая строка pPath - в OEM-кодировке
	};
	//
	// Descr: Унифицирует текстовое представление пути pPath. Результат переносится в rNormalizedPath
	//
	static SString & NormalizePath(const char * pPath, long flags, SString & rNormalizedPath);

	int    Invariant(SInvariantParam *);
	SPathStruc();
	//
	// Descr: Копирует в this компоненты пути pS заданные
	//   флагами flags.
	//   Если pS == 0, то соответствующие флагам компоненты
	//   обнуляются.
	//
	SPathStruc & Copy(const SPathStruc * pS, long flags);
	SPathStruc & Clear();
	void   FASTCALL Split(const char * pPath);
	int    Merge(SString & rBuf) const;
	int    Merge(const SPathStruc * pPattern, long patternFlags, SString & rBuf);

	enum {
		fDrv = 0x0001, // Путь содержит имя устройства
		fDir = 0x0002, // Путь содержит имя каталога
		fNam = 0x0004, // Путь содержит имя файла
		fExt = 0x0008, // Путь содержит расширение файла
		fUNC = 0x0010  // Путь представлен в UNC-формате
	};
	SString Drv, Dir, Nam, Ext;
	long   Flags;
};
//
//
//
class SStringTag : public SString {
public:
	SStringTag();
	long   Id;
};
//
// Descr: Номер версии программной системы.
//   Note: Гарантируется сохранение количества, размера и порядка следования переменных-членов этой структуры.
//
struct SVerT { // @persistent Size=4
	SLAPI  SVerT(int = 0, int = 0, int = 0);
	int    SLAPI Get(int *, int *, int *) const;
	int    SLAPI Set(int, int, int);
	void   SLAPI Set(uint32);
	int    SLAPI IsLt(int, int, int) const;
	int    SLAPI IsGt(int, int, int) const;
	int    SLAPI IsEq(int, int, int) const;
	int    SLAPI Cmp(const SVerT *) const;
	SString FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL FromStr(const char * pStr);
	operator uint32() const;
	int    SLAPI Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	uint16 V;
	uint16 R;
};
//
// Параметр функции searchstr
//
#define SSPF_WORDS  0x0001
#define SSPF_NOCASE 0x0002

typedef struct tagSSrchParam {
	const  char * P_Pattern;
	const  char * P_WordChars; // Reserved. Must be NULL.
	int    Flags;
} SSrchParam;

#define STRNSCPY(dest,src) strnzcpy(dest,src,sizeof(dest))
//
// If maxlen == 0, then call strcpy(dest, src)
// If (src == 0 && dest != 0) then dest[0] = 0;
//
char * FASTCALL strnzcpy(char * pDest, const char * pSrc, size_t maxlen);
char * FASTCALL strnzcpy(char * pDest, const uchar * pSrc, size_t maxlen);
char * FASTCALL strnzcpy(char * pDest, const SString & rSrc, size_t maxlen);
wchar_t * FASTCALL strnzcpy(wchar_t * pDest, const wchar_t * pSrc, size_t maxlen);
char * SLAPI    wstrcpy(char * pDest, const char * pSrc, size_t maxlen);
char    * FASTCALL sstrcpy(char * pDest, const char * pSrc);
uchar   * FASTCALL sstrcpy(uchar * pDest, const uchar * pSrc);
wchar_t * FASTCALL sstrcpy(wchar_t * pDest, const wchar_t * pSrc);
char * FASTCALL trimleft(char * pStr);
char * FASTCALL trimright(char * pStr);
int    FASTCALL isempty(const char * pStr); // BIN(pStr || pStr[0])
int    FASTCALL isempty(const uchar * pStr); // BIN(pStr || pStr[0])
int    FASTCALL isempty(const wchar_t * pStr); // BIN(pStr || pStr[0])
size_t FASTCALL sstrlen(const char * pStr);
size_t FASTCALL sstrlen(const uchar * pStr);
size_t FASTCALL sstrlen(const wchar_t * pStr);
char * FASTCALL sstrdup(const char * pStr);
uchar * FASTCALL sstrdup(const uchar * pStr);
//
// Descr: определяет эквивалентность строк pS1 и pS2.
//   Если pS1 == pS2, то результат !0
// Returns:
//   !0 - строки эквивалентны
//   0  - строки не эквивалентны
//
int    FASTCALL sstreq(const char * pS1, const char * pS2);
int    FASTCALL sstreq(const uchar * pS1, const uchar * pS2);
//
// Descr: определяет эквивалентность строк pS1 и pS2
//   без учета регистра, но только для ascii-символов.
// Returns:
//   !0 - строки эквивалентны
//   0  - строки не эквивалентны
//
int    FASTCALL sstreqi_ascii(const char * pS1, const char * pS2);
//
// Descr: Определяет все ли len символов строки pS находятся в диапазоне [0..0x7f].
// Note: Функция может быть использована для быстрой идентификации необходимости
//   трансляции строки в UNICODE или для иных специализированных задач.
// Returns:
//   !0  - все len символов в pS находятся в диапазоне [0..0x7f]
//    0  - как минимум один символ в pS - не ascii (вне диапазона [0..0x7f])
//
size_t FASTCALL sisascii(const char * pS, size_t len);
char * FASTCALL strip(char * pStr);
char * SLAPI chomp(char * pStr);
const  char * SLAPI skipws(const char * pStr, size_t * pPos);
//
// Descr: Функция заменяет символ 'c' на строку из одиночного того же символа "c".
// Note: Функция рудиментарная сделана еще во времена DOS с целью снизить размер кода.
//   Не использовать.
//
// @v9.6.5 const  char * FASTCALL onecstr(char c);
char * SLAPI padleft(char  * pStr, char pad, size_t n);
char * SLAPI padright(char * pStr, char pad, size_t n);
char * SLAPI alignstr(char * pStr, size_t width, int adj);
// @v9.5.1 (obsolete) int    SLAPI getTextHight(char * pStr, int width);
int    SLAPI searchstr(const char * pStr, const SSrchParam & rParam, size_t * pBeg, size_t * pLen);
int    SLAPI replacestr(char * pStr, const char * pReplacer, size_t * pPos, size_t * pLen, uint maxlen);
// @v9.4.10 (unused) char * SLAPI quotstr(char * pStr, int leftQuotChr, int rightQuotChr);
// AHTOXA convert hex or octal representation of symbol to symbol in string
int    SLAPI hostrtocstr(const char * pInBuf, char * pOutBuf, size_t outBufSize);
// @v9.5.1 (moved as static to sbrodef.cpp) uint   GetTextHeight(char * pBuf, size_t strLen);
// @v9.5.1 (moved as static to sbrodef.cpp) void   FASTCALL RemoveCtrlNFromStr(char * pBuf, size_t bufLen);
int    SplitBuf(HDC hdc, SString & aBuf, size_t maxStrSize, size_t maxStrsCount);
//
// Descr: Добавляет по указателю pStr разделительный символ div, возможно (addSpaces != 0), обрамленный
//   пробелами и возвращает указатель на следующий за разделителем байт.
//   Например (точка показывает байт, на который ссылается указатель):
//   buf                       // -> .abc
//   ptr                       // -> abc.
//   ptr = catdiv(ptr, '-', 1) // -> abc - .
//
// @v9.4.10 (unused) char * SLAPI catdiv(char * pStr, int div, int addSpaces);

//get string - used in network @vantonov
int QuotedStringToStr(char **pStr, char * pBuf, int maxBytes, char dStr = '\"');
//
// Функции для классификации и конвертации символов
//
// @v8.6.4 #define IS_DIGIT(c) ((c)>='0'&&(c)<='9')
//
// Descr: Возвращает !0 если символ c является шестнацдатиричным знаком [0-9A-Fa-f]
//
int    FASTCALL ishex(char c);
//
// Descr: Возвращает !0 если символ c является десятичным знаком [0-9]
//
int    FASTCALL isdec(char c);
//
// Descr: Возвращает значение шестнадцатиричного знака c. Если c не является шестнацдатиричным знком,
//   то возвращает 0 (для того, чтобы избежать путаницы с hex('0') следует проверять
//   символ функцией ishex(char)).
//
uint   FASTCALL hex(char c);
//
// Descr: Преобразует два шестнадцатиричных символа pBuf[0] и pBuf[1] в
//    бинарный байт.
// Note: Если pBuf[0] или pBuf[1] не являются шестнадцатиричными символами,
//    то возвращает 0. Дабы избежать путаницы с "00" необходимо предварительно
//    проверить эти символы функцией ishex(char).
//
uint8  FASTCALL hextobyte(const char * pBuf);
int    FASTCALL IsLetterASCII(int ch);
int    FASTCALL IsLetter1251(int ch);
int    FASTCALL ToLower1251(int ch);
int    FASTCALL ToUpper1251(int ch);
char * FASTCALL strlwr1251(char *);
char * FASTCALL strupr1251(char *);
int    FASTCALL IsLetter866(int ch);
int    FASTCALL ToLower866(int ch);
int    FASTCALL ToUpper866(int ch);
int    FASTCALL stricmp866(const char *, const char *);
int    FASTCALL strnicmp866(const char *, const char *, size_t);
char * FASTCALL stristr866(const char *, const char *);
char * FASTCALL strlwr866(char *);
char * FASTCALL strupr866(char *);
// @v9.5.1 int    SLAPI rus_tbl_cvt(int ch, int srctbl, int desttbl);
// @v9.5.1 uchar  SLAPI __866_to_1251();
// @v9.5.1 uchar  SLAPI __1251_to_866();
// @v9.5.1 char * FASTCALL _s_866_to_1251(char *);
// @v9.5.1 char * FASTCALL _s_1251_to_866(char *);
int    FASTCALL __866_to_koi7(int);
char * FASTCALL _s_866_to_koi7(char *);
int    FASTCALL _koi8_to_866(int);
// @v9.5.1 (moved as static to tvrez.cpp) char   FASTCALL wchar_to_1251(int16);
#if 0 // @v9.5.1 {
	#if !defined(__WIN32__) || defined(_WIN32_WCE)
		int OemToChar(const char * in, char * out);
		int CharToOem(const char * in, char * out);
	#endif
#endif // } 0 @v9.5.1
char * FASTCALL SOemToChar(char * pStr);
char * FASTCALL SCharToOem(char * pStr);
//
// Formats for date binary representation
//
#define DF_DOS              1
#define DF_FAT              2
#define DF_XBASE            3
#define DF_PARADOX          4
#define DF_CLARION          5
#define DF_BTRIEVE          6
//
// Week days
//
#define WD_SUN              0
#define WD_MON              1
#define WD_TUE              2
#define WD_WED              3
#define WD_THU              4
#define WD_FRI              5
#define WD_SAT              6
//
// Periods
//
#define PRD_UNDEF          -1
#define PRD_BEGIN          -3
#define PRD_END            -2
#define PRD_REPEATAFTERPRD -4
#define PRD_DAY             1
#define PRD_WEEK            7
#define PRD_MONTH          30
#define PRD_QUART          90
#define PRD_SEMIAN        180
#define PRD_ANNUAL        360

#define PRD_PRECDAYSMASK  0x7000
#define NUM_MONTHES        12

extern const char   daysPerMonth[NUM_MONTHES];
extern const char * monthNames[NUM_MONTHES];
//
// Functions for date formats convertion
//
void   SLAPI _encodedate(int d, int m, int y, void *, int);
void   SLAPI _decodedate(int * pD, int * pM, int * pY, const void *, int);
int    SLAPI _checkdate(int d, int m, int y);
char * SLAPI _datefmt(int d, int m, int y, int format, char *);
void   SLAPI encodedate(int, int, int, void *);
void   SLAPI decodedate(int *, int *, int *, const void *);
void   SLAPI decode_fat_datetime(uint16 fd, uint16 ft, LDATETIME *);
void   SLAPI encode_fat_datetime(uint16 * fd, uint16 * ft, LDATETIME *);
int    FASTCALL checkdate(const void * pBinDate);
int    FASTCALL checkdate(LDATE dt, int zeroIsOk);
int    FASTCALL checktime(LTIME tm);
//
//
//
#define MONF_SHORT     0x0001 // Сокращенная форма
#define MONF_CASENOM   0x0002 // Полная форма (именительный падеж)
#define MONF_CASEGEN   0x0004 // Полная форма (родительный падеж)
#define MONF_OEM       0x0080 // OEM-coding

// @v9.4.8 char * SLAPI getMonthText(int mon, long fmt, char * pBuf);
SString & SLAPI SGetMonthText(int mon, long fmt, SString & rBuf); // @v9.4.8
//
// Descr: Флаги функции strtoperiod
//
enum {
	strtoprdfEnableAnySign = 0x0001 // Допускается использование символа '?'
};
//
// Descr: Преобразует строку pStr в период *pPeriod.
//   Период в строке может быть определен со специальными шаблонами.
// Returns:
//   >0 - строка преобразована успешно
//   0  - ошибка
//
int   SLAPI strtoperiod(const char * pStr, DateRange * pPeriod, long flags);
//
// Current date getting
//
int   FASTCALL getcurdate(LDATE *);
int   FASTCALL getcurtime(LTIME *);
//
// Descr: Возвращает текущую системную дату
//
LDATE SLAPI    getcurdate_();
//
// Descr: Возвращает текущее системное время //
//
LTIME SLAPI    getcurtime_();
int   FASTCALL getcurdatetime(LDATETIME *);
int   FASTCALL getcurdatetime(LDATE * pDt, LTIME * pTm);
LDATETIME FASTCALL getcurdatetime_();
int   SLAPI setcurdatetime(LDATETIME dtm);
int   gettimezone();
//
// Date arithmetic
//
long  SLAPI _diffdate(const void * pDest, const void * pSrc, int fmt, int _360); /* dest - src */
void  SLAPI _plusdate(void * pDest, int numdays, int format, int _360);
//
// Descr: К дате pDest прибавляет numperiods периодов вида period. Результат заносится //
//   по указателю pDest.
// ARG(pDest  IN/OUT): Указатель на дату, к которой необходимо прибавить требуемое количество периодов
// ARG(period     IN): Вид периода для сложения. Используется одна из констант PRD_XXX.
// ARG(numperiods IN): Количество периодов для сложения //
// ARG(format     IN): Бинарный формат представления даты. Используестя одна из констант DF_XXX
// ARG(_360       IN): Если этот параметр !0, то применяется банковская календарная арифметика,
//   при этом длительность года полагается равной 360 дням.
//
void  SLAPI _plusperiod(void * pDest, int period, int numperiods, int format, int _360);
int   FASTCALL _dayofweek(const void * pDate, int format); // 0..6 (0=Sunday)
long  SLAPI diffdate(const void *, const void *, int _360);
void  SLAPI plusdate(void *, int, int _360);
//
// Descr: К дате pDest прибавляет numperiods периодов вида period. Результат заносится //
//   по указателю pDest. Функция считает, что бинарный формат представления даты pDest
//   определен глобалной переменной BinDateFmt.
// ARG(pDest  IN/OUT): Указатель на дату, к которой необходимо прибавить требуемое количество периодов
// ARG(period     IN): Вид периода для сложения. Используется одна из констант PRD_XXX.
// ARG(numperiods IN): Количество периодов для сложения //
// ARG(_360       IN): Если этот параметр !0, то применяется банковская календарная арифметика,
//   при этом длительность года полагается равной 360 дням.
//
void  SLAPI plusperiod(void * pDest, int period, int numperiods, int _360);
//
// Descr: возвращает день недели, на который приходится дата pDate.
// Parameters:
//   pDate - указатель на дату в бинарном формате BinDateFmt
//   sundayIsSeventh - если !0, то воскресенье является 7-м днем недели,
//     иначе, воскресение имеет значение 0.
// Returns:
//   Номер дня недели. См комментарии к параметру sundayIsSeventh
//
int   FASTCALL dayofweek(const void * pDate, int sundayIsSeventh = 0); // 0..6 (0=Sunday) // @>>_dayofweek
//
// Descr: возвращает количество дней в месяце month для года year.
// ARG(month IN): номер месяца @# [1..12]
// ARG(year  IN): год
// Returns:
//   номер последнего дня в месяце month года year
//   Если month < 1 || month > 12, то возвращает 0
//
int   FASTCALL dayspermonth(int month, int year);
//
// Descr: Возвращает текст, соответствующий дню недели dayOfWeek.
// ARG(options    IN): формат представления текста (dowtXXX see below).
// ARG(dayOfWeek  IN): @#[1..7] номер дня недели: 1 - monday...7 - sunday.
// ARG(rBuf      OUT): буфер, в который будет занесен текст.
// Returns:
//   !0 - функция успешно отработала - в буфер занесена требуемая строка
//   0  - ошибка - rBuf пустой.
//
enum {
	dowtEnFull = 1,
	dowtEnShrt,
	dowtRuFull,
	dowtRuShrt
};
int   SLAPI GetDayOfWeekText(int options, int dayOfWeek, SString & rBuf);
int   SLAPI GetDayOfWeekByText(const char * pText);
//
// Time format convertion
//
int   SLAPI _encodetime(int h, int m, int s, int ts, void *, int);
int   SLAPI _decodetime(int * h, int * m, int * s, int * ts, void *, int);
int   SLAPI encodetime(int h, int m, int s, int ts, void *);
void  SLAPI decodetime(int * h, int * m, int * s, int * ts, void *);
LTIME SLAPI encodetime(int h, int m, int s, int ts);
/*
	Result of difftime:
		(dim == 1) : hours
		(dim == 2) : minuts
		(dim == 3) : seconds
		(dim == 4) : thousandths of second
*/
long  SLAPI DiffTime(LTIME t1, LTIME t2, int dim);
long  SLAPI diffdatetime(LDATE d1, LTIME t1, LDATE d2, LTIME t2, int dim, long * pDiffDays);
long  SLAPI diffdatetime(const LDATETIME &, const LDATETIME &, int dim, long * pDiffDays);
long  SLAPI diffdatetimesec(LDATE d1, LTIME t1, LDATE d2, LTIME t2);
long  FASTCALL diffdatetimesec(const LDATETIME &, const LDATETIME &);
//
LDATETIME FASTCALL plusdatetime(const LDATETIME &, long plus, int dim);
LDATE FASTCALL encodedate(int day, int month, int year);
LDATE FASTCALL plusdate(LDATE d, long a);
long  FASTCALL diffdate(LDATE d, LDATE s);

char * FASTCALL _commfmt(long fmt, char * pBuf);
SString & FASTCALL _commfmt(long fmt, SString & rBuf);
char * SLAPI realfmt(double val, long fmt, char * pBuf);
char * SLAPI intfmt(long val, long fmt, char * pBuf);
char * SLAPI int64fmt(int64 val, long fmt, char * pBuf);
//
// Descr: форматирует длинное целое число val в строку pBuf
//   Если numdigits > 0, то форматирутся с количеством символов равным numDigits
//   с набивкой слева нулями (например: longfmtz(5, 4, buf, sizeof(buf)) --> buf="0005")
//   Если numDigits <= 0, то просто переводит val в строку форматом %ld.
// Returns:
//   pBuf
//
char * SLAPI longfmtz(long val, int numDigits, char * pBuf, size_t bufLen);
char * SLAPI uintfmt(ulong val, long fmt, char * pBuf);
char * SLAPI uint64fmt(uint64 val, long fmt, char * pBuf);
char * SLAPI decfmt(BCD_T val, int len, int prec, long fmt, char * pBuf);
char * SLAPI datefmt(const void *, long fmt, char *);
char * SLAPI timefmt(LTIME, long fmt, char *);
char * SLAPI datetimefmt(LDATETIME dtm, long dtfmt, long tmfmt, char * pBuf, size_t bufLen);
char * SLAPI periodfmt(const DateRange *, char *);
int    SLAPI periodfmtex(const DateRange *, char *, size_t bufLen);

char * SLAPI strfmt(const char * str, long fmt, char * buf);
//char * SLAPI numbertotext(double nmb, long fmt, char * pBuf);
// @v9.6.0 int   SLAPI strtoirng(const char * pBuf, long * pLow, long * pUpp);
int   SLAPI strtorrng(const char * pBuf, double * pLow, double * pUpp);
int   FASTCALL strtorrng(const char * pBuf, RealRange & rRng);
int   SLAPI strtodoub(const char * pBuf, double *);
int   SLAPI strtolong(const char * pBuf, long *);
int   SLAPI strtoulong(const char * pBuf, ulong *);
//
enum {
	strtodatefZero     = 0x0001, // Пустая дата
	strtodatefDefMon   = 0x0002, // Месяц установлен по умолчанию
	strtodatefDefYear  = 0x0004, // Год установлен по умолчанию
	strtodatefRel      = 0x0008, // Дата задана относительно текущего дня +/- количество дней
	strtodatefRelDay   = 0x0010, // День задан в относительной форме
	strtodatefRelMon   = 0x0020, // Месяц задан в относительной форме
	strtodatefRelYear  = 0x0040, // Год задан в относительной форме
	strtodatefThrsMDay = 0x0080, // Дата задана как начало месяца с порогом в виде дня. Если текущий день менее порога, то - начало предыдущего месяца, иначе - текущего.
	strtodatefAnyDay   = 0x0100, // @v9.5.9
	strtodatefAnyMon   = 0x0200, // @v9.5.9
	strtodatefAnyYear  = 0x0400, // @v9.5.9
	strtodatefRelAny   = (strtodatefRel|strtodatefRelDay|strtodatefRelMon|strtodatefRelYear|strtodatefThrsMDay) // Маска
};

#define REL_DATE_MASK      0x80000000
#define THRSMDAY_DATE_MASK 0x20000000
#define ANY_DATE_VALUE     0x7d0a7d7d
#define ANY_DAYITEM_VALUE  0x7d
#define ANY_MONITEM_VALUE  0x7d
#define ANY_YEARITEM_VALUE 0x7d0a
//
// Descr: конвертирует строку в дату. Из строки извлекается //
//   дата в формате, определенном вторым параметром. В качестве разделителя //
//   числа, месяца и года можно использовать точку, тире или дробь. Параметром
//   определяется только порядок следования этих чисел. Сканирование даты
//   продолжается до тех пор, пока не встретится какой-либо недопустимый
//   символ. Если год, месяц или число отсутствуют в строке, то принимаются //
//   соответсвенно DefaultYear, DefaultMonth и 1. Особый случай:
//   недопустимым символом считаются две следующие одна за другой точки.
// Returns: смещение от начала строки до первого после принятой даты символа.
//
int   SLAPI _strtodate(const char * pBuf, int format, int * pDay, int * pMonth, int * pYear, long * pRetFlags = 0);
int   SLAPI strtodate(const char * pBuf, long fmt, void *);
LDATE FASTCALL strtodate_(const char * pBuf, long fmt);
//
// Descr: Преобразует строку pBuf в представление времени в формате LTIME
//   Строка может иметь вид "hh:mm:ss:hs", где
//     hh - часы; mm - минуты; ss - секунды; hs - сотые доли секунды
//   Количество цифр - не фиксированно (2:00 также допустимо как и 02:00)
//   Минуты, секунды, сотые доли секунд могут быть пропущены
//   В качестве разделителя вместо ':' можно использовать пробел или ';'
// ARG(pBuf   IN) - строка содержащая представление времени
// ARG(fmt    IN) - параметры форматирования (игнорируется)
// ARG(pTime OUT) - указатель на структуру LTIME, в которую будет записано принятое значение времени
// Returns:
//   1
//
int   SLAPI strtotime(const char * pBuf, long fmt, void * pTime); // @>>::strtok
int   SLAPI strtodatetime(const char * pBuf, LDATETIME * pDtm, long datFmt, long timFmt);
//
// Descr: Обобщанная структура поля данных. Используется в основном для работы
//   с таблицами баз данных.
// Note: Перенесено из db.h для реализации универсального сериализатора данных SSerializeContext.
//
struct BNField {
	size_t SLAPI size() const
	{
		return stsize(T);
	}
	int    SLAPI IsEqual(const BNField & rS, int nameAndTypeOnly = 0) const;
	int    FASTCALL getValue(const void * pDataBuf, void * pValBuf, size_t * pSize) const;
	int    SLAPI setValue(void * pDataBuf, const void * pValBuf) const;
	int    SLAPI putValueToString(const void * pDataBuf, char * pBuf) const;

	int32  Id;   // If object attached to dictionary then
	//              id = X$FIELD.XeId, else id = 0,1,..
	char   Name[20];
	uint32 Offs;
	TYPEID T;
};
//
// Descr: Обобщенное представление записи таблицы данных. Используется в основном для работы
//   с таблицами баз данных.
// Note: Перенесено из db.h для реализации универсального сериализатора данных SSerializeContext.
//
class BNFieldList {
public:
	SLAPI  BNFieldList();
	SLAPI  BNFieldList(const BNFieldList & rS);
	SLAPI ~BNFieldList();
	BNFieldList & FASTCALL operator = (const BNFieldList &);
	int    FASTCALL copy(const BNFieldList *);
	int    SLAPI IsEqual(const BNFieldList & rS, int nameAndTypeOnly = 0) const;
	void   SLAPI reset();
	int    SLAPI addField(const char * pName, TYPEID type, int id = UNDEF);
	int    FASTCALL addField(const BNField &);
	int    SLAPI getFieldPosition(int fldId, uint * pPos) const;
	const  BNField & SLAPI getField(uint, int byPosition = 1) const;
	const  BNField & SLAPI getField(const char * pName, uint * pPos = 0) const;
	const  BNField & FASTCALL operator[](uint i) const { return getField(i); }
	uint   SLAPI getCount() const;
	RECORDSIZE SLAPI getRecSize() const;
	int    SLAPI setFieldId(uint fldNo /* 0..(count-1) */, int fldId);
	int    SLAPI IsEqualRecords(const void * pRec1, const void * pRec2, LongArray * pNeqFldList = 0) const;
private:
	uint   count;
	BNField * fields;
};
//
// Descr: Универсальный итератор
//
class SEnumImp {
public:
	virtual ~SEnumImp();
	virtual int Next(void * pData) = 0;
};

class SEnum {
public:
	SEnum(SEnumImp * pE = 0);
	SEnum & FASTCALL operator = (SEnumImp * pE);
	~SEnum();
	int    operator !() const;
	int    FASTCALL Next(void * pData);
private:
	SEnumImp * P_E;
};
//
// File utilities
//
class SFileUtil {
public:
	struct Stat {
		LDATETIME CrtTime;
		LDATETIME AccsTime;
		LDATETIME ModTime;
		int64  Size;
	};
	static int GetStat(const char * pFileName, Stat * pStat);
	static int GetDiskSpace(const char * pPath, int64 * pTotal, int64 * pAvail);

	enum {
		sdSystem = 1,
		sdProgramFiles,
		sdProgramFilesCommon,
		sdWindows,
		sdAppData,
		sdAppDataLocal,
		sdCommonDocuments
	};
	static int GetSysDir(int pathId, SString & rPath);
};

int	   SLAPI pathToUNC(const char * pPath, SString & rUncPath);
char * SLAPI replacePath(char * fileName, const char * newPath, int force);
char * SLAPI replaceExt(char * fileName, const char * newExt, int force);
char * SLAPI setLastSlash(char *);
char * SLAPI rmvLastSlash(char *);
//
// Descr: Returns execution file name in buffer pBuf.
// Returns:
//   pBuf
//
SString & SLAPI getExecPath(SString & rBuf);

int    FASTCALL fileExists(const char * pFileName);
int    SLAPI driveValid(const char * path);
int    SLAPI isDir(const char * pStr);
int    SLAPI pathValid(const char * pPath, int existOnly);
int    SLAPI validFileName(const char * pFileName);
//
// Descr: формирует полное имя файла, состоящее из пути
//   до исполняемого файла и имени pName с расширением pExt.
//   Результат помещается в буфер rPath.
// Returns:
//   ссылка на rPath
// Requares:
//   @# &rPath != 0
//
SString & SLAPI makeExecPathFileName(const char * pName, const char * pExt, SString & rPath);
int    SLAPI isWild(const char * pFileName);
int    SLAPI createDir(const char * pPath);
int    SLAPI copyFileByName(const char * pSrcFileName, const char * pDestFileName);
//
// Descr: Создает пустой файл с именем pFileName.
// Returns:
//   1 - удалось создать файл с заданным именем
//   0 - pFileName == 0 || не удалось создать файл с заданным именем.
//
int    SLAPI createEmptyFile(const char * pFileName);
//
// Descr: создает уникальное имя для временного файла
//   по шаблону {pDir [+ \\] + pPrefix + counter [+ .] + pExt}.
//   counter - число от 1 до 99999. Если префикс имеет длину более 5 символов,
//   то он обрезается до 5 символов.
//   Расширение (pExt) обрезается до трех символов.
//   Каталог может быть задан с или без завершающего обратного слэша.
//   Расширение (pExt) может быть задано с лидирующей точкой или без нее.
//
/*
char * SLAPI MakeTempFileName(const char * pDir, const char * pPrefix,
	const char * pExt, long * pStart, char * pFileNameBuf, size_t fileNameBufLen); // @obsolete
*/
SString & SLAPI MakeTempFileName(const char * pDir, const char * pPrefix, const char * pExt, long * pStart, SString & rBuf);
//
// Descr: Searches a binary file for a string.
//   @>>SearchStrInFile(Filename, startoffset, string, ignoreCase);
// ARG(pFile       IN): Pointer to the (open binary mode) binary file
// ARG(startOffset IN): Startoffset for search
// ARG(pStr        IN): String to search for
// ARG(ignoreCase  IN): 1 - Ignore case (stricmp866), 0 - Don't ignore case (strcmp)
// Returns:
//   -1:   String not found
//   -2:   Not enough memory to perform search
//   else: Offset for string in file
// Module: fsif.c
//
long   SLAPI SearchStrInFile(FILE *, long startOffset, const char * pStr, int ignoreCase);

#define SPRGRS_CONTINUE  0 /* for WIN32 eq PROGRESS_CONTINUE */
#define SPRGRS_CANCEL    1 /* for WIN32 eq PROGRESS_CANCEL   */
#define SPRGRS_STOP      2 /* for WIN32 eq PROGRESS_STOP     */
#define SPRGRS_QUITE     3 /* for WIN32 eq PROGRESS_QUITE    */

struct SCopyFileData {
	const  char * SrcFileName;
	const  char * DestFileName;
	int64  TotalFileSize;
	int64  TransferredBytes;
	void * ExtraParam;
};

typedef int (*SCopyFileProgressProc)(const SCopyFileData *);
typedef int (*PercentFunc)(long, long, const char *, int);

int SLAPI SCopyFile(const char * pSrcFileName, const char * pDestFileName, SCopyFileProgressProc, long shareMode, void * pExtra);
int compress(char *, char *, int, PercentFunc, ulong*);
// @v9.4.8 int SLAPI PKUnzip(const char * pSrc, const char * pDest, const char * pZipFile);
int SLAPI PKZip(const char * pSrcPath, const char * pDestPath, const char * pZipDir);
int DoCompress(const char * pSrc, const char * pDest, int64 * pFileSize, int compress, PercentFunc pf);
//
//
//
struct ExecVDosParam {
	ExecVDosParam()
	{
		Flags = 0;
	}
	enum {
		fExitAfter = 0x0001,
		fWait      = 0x0002
	};
	long   Flags;
	SString ExePath;
	SString StartUpPath;
	StringSet Batch;
};

int SLAPI ExecVDos(ExecVDosParam & rParam);
//
//
//
struct _finddatai64_t;

struct SDirEntry {
	SDirEntry & FASTCALL operator = (const _finddatai64_t & rS);
	SDirEntry & FASTCALL operator = (const WIN32_FIND_DATA & rS);
	//
	// Descr: Возвращает !0 если является каталогом
	//
	int    SLAPI IsFolder() const;
	//
	// Descr: Возвращает !0 если является ссылкой на каталог верхнего уровня ("..")
	//
	int    SLAPI IsUpFolder() const;
	//
	// Descr: Возвращает !0 если является ссылкой на текущий каталог (".")
	//
	int    SLAPI IsSelf() const;
	//
	// Descr: Возвращает !0 если является файлом (== (!IsFolder() && !IsUpFolder() && !IsSelf()))
	//
	int    SLAPI IsFile() const;

	uint   Attr;
	LDATETIME CreateTime;
	LDATETIME AccessTime;
	LDATETIME WriteTime;
	int64  Size;
	char   FileName[1024];
};

class SDirec {
public:
	SLAPI  SDirec();
	SLAPI  SDirec(const char * pathname, int enumDirs = 0);
	SLAPI ~SDirec();
	int    SLAPI Init(const char * pathname, int enumDirs = 0);
	int    FASTCALL Next(SDirEntry *);
private:
	int    SLAPI FindFirst();
	int    SLAPI FindNext();
	int    SLAPI FindClose();

	char   InitPath[MAXPATH];
	int    InitFlag;
	int    EnumDir;
	long   lPosData;
	void * vPosData;
	size_t PosDataSize;
	SDirEntry CurEntry;
};

int    GetFileStat(const char * pFileName, SDirEntry * pEntry);
int    RemoveDir(const char * pDir);
//
// Descr: Класс, реализующий функцию поиска файла (каталога) по шаблону.
//   Осуществляет рекурсивный поиск файла по шаблону P_FileName, начиная с
//   каталога P_Path.
//   Для определения действий над найденными файлами следует реализовать
//   виртуальную функцию CallbackProc.
//
class SFindFile {
public:
	SLAPI  SFindFile(const char * pPath = 0, const char * pFileName = 0);
	//
	// Descr: Запускает механизм поиска. Предварительно должны быть инициализированы
	//   необходимые поля.
	//
	int    SLAPI Run();
	//
	// Descr: вызывается на каждый найденный по заданным условиям файл, а также
	//   для всех каталогов, вложенных в стартовый каталог P_Path.
	//   Для каталогов поле pEntry->FileName[0] == 0.
	// Returns:
	//   !0 - метод Run должен продолжать работу
	//   0 - метод Run следует остановить. В этом случае Run() вернет 0.
	//
	virtual int SLAPI CallbackProc(const char * pPath, SDirEntry * pEntry);

	const  char * P_Path;     // @#{vptr}
	const  char * P_FileName; // @#{vptr0}
	const  char * P_SubStr;   // reserved
	long   Flags;
	long   DirCount;          // Увеличивается на единицу для каждого найденного подкаталога
	long   FileCount;         // Увеличивается на единицу для каждого найденного файла,
		// соответствующего шаблону PFileName
};
//
// Memory manipulation functions
//
//
// Descr: Оптимизированная функция копирования памяти. По спецификации эквивалентна memmove,
//   но быстрее на 20..30%.
//   Единственное существенное отличие от спецификации memmove - memmovo не возвращает ничего.
//   Это сделано ради оптимизации.
//
void  FASTCALL memmovo(void * pDest, const void * pSrc, size_t size);
void * SLAPI catmem(void * pDest, size_t destSz, const void * pSrc, size_t srcSz);
//
// Descr: Быстро меняет содержимое по указателям p1 и p2 размером size.
//
void   FASTCALL memswap(void * p1, void * p2, size_t size);
//
// Descr: Ищет в блоке памяти по указателю p размером size двойное слово, равное k.
//   Если поиск завершился удачно и искомый ключ найден, то присваивает по указателю
//   pOffs смещение от p до найденного значения.
// Returns:
//   !0 - искомый ключ найден
//   0  - искомый ключ не найден
//
int    SLAPI memdword(void * p, size_t size, uint32 k, size_t * pOffs);
//
// Descr: Вызов memzero эквивалентен memset(p, 0, s). Так как такие
//   вызовы весьма часты, то использование memzero даст выигрыш
//   в размере программного кода.
//   Кроме того, memzero проверяет указатель p на ноль, что увеличивает
//   безопасность кода.
//
void * FASTCALL memzero(void * p, size_t s);
int    FASTCALL ismemzero(const void * p, size_t s);

#define MEMSZERO(p)  memzero(&(p), sizeof(p))
#define THISZERO()   memzero(this, sizeof(*this))
//
// Descr: Меняет порядок байтов двухбайтового слова
//
uint16 FASTCALL swapw(uint16);
//
// Descr: Меняет местами слова в двойном слове dw
//
uint32 FASTCALL swapdw(uint32 dw);
char * FASTCALL newStr(const char *);
//
// Descr: Меняет значения по указателям pA и pB один с другим
//
void   FASTCALL Exchange(int * pA, int * pB);
void   FASTCALL Exchange(uint * pA, uint * pB);
void   FASTCALL ExchangeToOrder(int * pA, int * pB);
void   FASTCALL Exchange(long * pA, long * pB);
//
// Descr: Если *pA > *pB, то меняет значения по этим указателям местами,
//   в противном случае ничего не делает
//
void   FASTCALL ExchangeToOrder(long * pA, long * pB);
void   FASTCALL Exchange(ulong * pA, ulong * pB);
void   FASTCALL Exchange(int16 * pA, int16 * pB);
void   FASTCALL ExchangeToOrder(int16 * pA, int16 * pB);
void   FASTCALL Exchange(float * pA, float * pB);
void   FASTCALL Exchange(double * pA, double * pB);
//
// Descr: Если *pA > *pB, то меняет значения по этим указателям местами,
//   в противном случае ничего не делает
//
void   FASTCALL ExchangeToOrder(double * pA, double * pB);
int    FASTCALL cmp_long(long a, long b);
int    FASTCALL cmp_ulong(ulong a, ulong b);
int    FASTCALL cmp_int64(int64 a, int64 b);
int    FASTCALL cmp_double(double a, double b);
//
// Decimal format (COBOL) functions
//
double SLAPI dectobin(const char * dec, int16 len, int16 prec);
char * SLAPI dectostr(const char * dec, int16 len, int16 prec, char * buf);
void   SLAPI dectodec(double v, char * dec, int16 len, int16 prec);
void   SLAPI dectobcd(char * dec, char * bcd, int16 len);
int    SLAPI deccmp(const char * dc1, const char * dc2, int16 len);
//
// Descr: Просто удобная и часто используемая структура.
//   Так как не содержит конструктора и деструктора, может
//   быть использована в union'ах.
//
struct SBaseBuffer {
	// no constructor (used in unions)
	// no destructor  (used in unions)
	void   SLAPI Init();
	void   SLAPI Destroy();
	int    FASTCALL IsEqual(const SBaseBuffer & rS) const;
	//
	// Descr: Копирует содержимое буфера rS в буфер this.
	// Note: И rS и this должны быть "правильными" объектами. То есть, для них
	//   когда-то до этого должны были быть вызваны методы Init().
	//   Если указатели P_Buf или размеры Size этих экземпляров инвалидны, то
	//   последствия не предсказуемы.
	//
	int    FASTCALL Copy(const SBaseBuffer & rS);
	void   SLAPI Zero();
	void   SLAPI Set(void * pBuf, size_t size);
	int    FASTCALL Alloc(size_t sz);
	//
	int    SLAPI Put(size_t offs, const void * pSrc, size_t size);
    int    SLAPI Put(size_t offs, uint8 v);
    int    SLAPI Put(size_t offs, uint16 v);
    int    SLAPI Put(size_t offs, uint32 v);
    int    SLAPI Put(size_t offs, uint64 v);
    int    SLAPI Put(size_t offs, int8 v);
    int    SLAPI Put(size_t offs, int16 v);
    int    SLAPI Put(size_t offs, int32 v);
    int    SLAPI Put(size_t offs, int64 v);

    int    SLAPI Get(size_t offs, void * pDest, size_t size) const;
    int    SLAPI Get(size_t offs, uint8 & rV) const;
    int    SLAPI Get(size_t offs, uint16 & rV) const;
    int    SLAPI Get(size_t offs, uint32 & rV) const;
    int    SLAPI Get(size_t offs, uint64 & rV) const;
    int    SLAPI Get(size_t offs, int8 & rV) const;
    int    SLAPI Get(size_t offs, int16 & rV) const;
    int    SLAPI Get(size_t offs, int32 & rV) const;
    int    SLAPI Get(size_t offs, int64 & rV) const;
	//
	size_t Size;
	char * P_Buf;
};
//
// Descr: Простой буфер с минимальной защитой. Все, что он умеет - распределить
//   память и разрушить распределенную память в деструкторе.
//   Обращение к внутреннему указателю никак не защищено.
//
class STempBuffer : private SBaseBuffer {
public:
	SLAPI  STempBuffer(size_t sz);
	SLAPI ~STempBuffer();
	STempBuffer & FASTCALL operator = (const STempBuffer & rS);
	int    SLAPI IsValid() const;
	size_t SLAPI GetSize() const;
	const  uchar * SLAPI ucptr() const;
	SLAPI  operator char * ();
	SLAPI  operator const char * () const;
	int    FASTCALL Alloc(size_t sz);
};
//
// Descr: Класс, управляющий буфером записи-чтения.
//
class SBuffer {
public:
	enum {
		mRead = 0,
		mWrite
	};
	enum {
		fFixedSize = 0x0001, // Буфер имеет фиксированный размер (не может увеличиваться сверх него)
			// Этот размер должен быть передан буферу в конструкторе
		fMovable   = 0x0002, // При записи в буфер можно считать, что все содержимое до RdOffs
			// не нужно и его можно перезаписать
		fError     = 0x8000  // Ошибка. Внутренний флаг. При попытке передать этот флаг в
			// конструктор, он сбрасывается.
	};
	//
	// Descr: Опции сохранения/восстановления объектов
	//
	enum {
		ffAryCount32    = 0x0001, // Количество элементов в массиве сохранять как 4-x байтовое значение (по умолчанию - 2 байта)
		ffAryForceEmpty = 0x0002  // Сохранить массив как имеющий 0 элементов, даже если он не пустой.
	};
	int    SLAPI InvariantC(SInvariantParam *) const;

	SLAPI  SBuffer(size_t initSize = 0, long flags = fMovable);
	SLAPI  SBuffer(const SBuffer &);
	SLAPI ~SBuffer();
	void   Destroy();
	int    FASTCALL Copy(const SBuffer &);
	SBuffer & FASTCALL operator = (const SBuffer & s);
	//
	// Desct: Возвращает 0 если в объекте произошла ошибка.
	//
	int    SLAPI IsValid();
	//
	// Descr: Сравнивает буфер this c буфером rS.
	//   Сравнение осуществляется бинарным сопоставлением между участками памяти this->P_Buf[0..this->WrOffs-1]
	//   и rS.P_Buf[0..rS.WrOffs-1].
	//   Пустые буферы (WrOffs == 0) считаются равными.
	//
	int    FASTCALL IsEqual(const SBuffer & rS) const;
	SLAPI  operator const void * () const;
	const  void * FASTCALL GetBuf(size_t offs = 0) const;
	size_t SLAPI GetSize() const;
	size_t SLAPI GetRdOffs() const;
	size_t SLAPI GetWrOffs() const;
	void   FASTCALL SetRdOffs(size_t offs);
	void   FASTCALL SetWrOffs(size_t offs);
	//
	// Descr: Сбрасывает в ноль указатели WrOffs и RdOffs.
	//   Таким образом, после выполнения этой операции буфер сохраняет
	//   свой изначальный распределенный размер, но пуст.
	// Returns:
	//   *this
	//
	SBuffer & SLAPI Clear();
	int    FASTCALL Write(const void *, size_t);
	//
	// Descr: Возвращает количество доступных для считывания байт буфера.
	//
	size_t SLAPI GetAvailableSize() const;
	//
	// Descr: Считывает в буфер pBuf, начиная со смещения RdOffs, доступное количество
	//   байт данных, но не более чем bufLen.
	//   Увеличивае указатель RdOffs на количество байт, скопированных в буфер pBuf.
	// Returns:
	//   Количество считанных байт.
	//
	size_t FASTCALL Read(void * pBuf, size_t bufLen);
	//
	// Descr: То же, что и SBuffer::Read(void *, size_t), но с проверкой
	//   равенства считанного размера запрошенному. Если это равенство не
	//   выполняется, то возвращает 0 и устанавливает код ошибки SLERR_SBUFRDSIZE.
	//
	int    FASTCALL ReadV(void * pBuf, size_t bufLen);
	//
	// Descr: Считывает в буфер pBuf, начиная со смещения RdOffs, доступное количество
	//   байт данных, но не более чем bufLen.
	//   НЕ изменяет значение указателя RdOffs
	// Returns:
	//   Количество считанных байт.
	//
	size_t FASTCALL ReadStatic(void * pBuf, size_t bufLen) const;
	int    FASTCALL Unread(size_t offs);
	//
	// Descr: Считывает в буфер pBuf, начиная со смещения RdOffs данные, заканчивающиеся //
	//   терминальной строкой pTerm. В буфер pBuf копируется не более bufLen байт.
	//   Если среди доступных для чтения данных терминал не обнаружен, то данные не
	//   копируются.
	//   Если терминал обнаружен, но bufLen меньше чем необходимо, то копируется не более
	//   bufLen байт.
	//   Данные в буфер копируются вместе с терминалом.
	// Returns:
	//   Количество считанных байт.
	//
	size_t SLAPI ReadTerm(const char * pTerm, void * pBuf, size_t bufLen);
	size_t SLAPI ReadTermStr(const char * pTerm, SString & rBuf);
	int    FASTCALL WriteByte(char);
	int    FASTCALL Read(char &);
	int    FASTCALL Write(const int8 & v);
	int    FASTCALL Read(int8 & v);
	int    FASTCALL Write(const uint8 & v);
	int    FASTCALL Read(uint8 & v);
	int    FASTCALL Write(const SString &);
	int    FASTCALL Read(SString &);
	int    FASTCALL Write(const long & v);
	int    FASTCALL Read(long & v);
	int    FASTCALL Write(const ulong & v);
	int    FASTCALL Read(ulong & v);
	int    FASTCALL Write(const uint16 & v);
	int    FASTCALL Read(uint16 & v);
	int    FASTCALL Write(const int64 & v);
	int    FASTCALL Read(int64 & v);
	int    FASTCALL Write(const uint64 & v);
	int    FASTCALL Read(uint64 & v);
	int    FASTCALL Write(const int16 & v);
	int    FASTCALL Read(int16 & v);
	int    FASTCALL Write(const double & v);
	int    FASTCALL Read(double & v);
	int    FASTCALL WriteFloat(float v);
	int    FASTCALL Read(float & v);
	int    FASTCALL Write(const LDATE & v);
	int    FASTCALL Read(LDATE & v);
	int    FASTCALL Write(const LTIME & v);
	int    FASTCALL Read(LTIME & v);
	int    FASTCALL Write(const LDATETIME & v);
	int    FASTCALL Read(LDATETIME & v);
	int    FASTCALL Write(const SArray * pAry, long options /*= 0*/);
	int    FASTCALL Read(SArray * pAry, long options /* = 0*/);
	int    FASTCALL Write(const SBuffer & v);
	int    FASTCALL Read(SBuffer & v);
	//
	// Descr: Записывает доступные для чтения данные из буфера в
	//   файл, открытый по указателю f.
	// ARG(f     IN): Указатель на открытый файл. Файл должен быть открыт для записи
	//   в бинарном режиме.
	// ARG(sign  IN): Сигнатура буфера. Если это значение больше нуля и меньше 0xffff, то
	//   сначала в файл записывается два байта с этой сигнатурой для проверки корректности
	//   при чтении.
	// ARG(pActualBytes OUT): Полное количество байт записанное в файл.
	// Returns:
	//   !0 - операция успешно завершена.
	//   0  - ошибка.
	//
	int    SLAPI WriteToFile(FILE * f, uint sign, uint32 * pActualBytes);
	int    SLAPI ReadFromFile(FILE * f, uint sign);
protected:
	void * FASTCALL Ptr(size_t) const;
private:
	SBuffer & FASTCALL Reset(int freeBuf);
	int    FASTCALL Alloc(size_t);
	int    SLAPI Search(const char * pStr, size_t * pPos) const;

	size_t Size;   // Размер буфера
	size_t WrOffs; // Смещение до которого буфер заполнен
	size_t RdOffs; // Смещение до которого осуществлено считывание содержимого буфера.
		// Теоретически, часть буфера [0..RdOffs] не нужна и это содержимое буфера может быть
		// удалено. Так происходит если Flags & fMovable.
	long   Flags;
	void * P_Buf;
};
//
// CRC32 functions
//
class CRC32 {
public:
	SLAPI  CRC32();
	SLAPI ~CRC32();
	ulong  SLAPI Calc(ulong, const uint8 *, size_t);
private:
	int    SLAPI MakeTab();
	ulong * P_Tab;
};
//
// Descr: Расчитывает хеш блока pData размером len по алгоритму Bob Jenkins
//
uint32 FASTCALL BobJencHash(const void * pData, size_t len);
//
// Descr: Алгоритмы расчета контрольной цифры
//
#define SCHKDIGALG_BARCODE 1 // Розничные штрихкоды (EAN, UPC)
#define SCHKDIGALG_LUHN    2 // Алгоритм Луна
#define SCHKDIGALG_RUINN   3 // Контрольная цифра ИНН (Россия)
#define SCHKDIGALG_RUOKATO 4 // Контрольная цифра ОКАТО (Россия)
#define SCHKDIGALG_RUSNILS 5 // Контрольная цифра СНИЛС (Россия)

#define SCHKDIGALG_TEST    0x80000000 // Флаг, предписывающий функции SCalcCheckDigit проверить последовательность на
	// предмет соответствия контрольной цифре, содержащейся в ней.
//
// Descr: Функция расчета контрольной цифры для цифровой последовательности pInput.
// Если alg & SCHKDIGALG_TEST, то последовательность должна содержать контрольную цифру, в противном случае - нет.
// Returns:
//   0 - ошибка расчета или проверки контрольной цифры.
//   1 - если alg & SCHKDIGALG_TEST, то последовательность содержит корректную контрольную цифру
//   '0'..'9' - если !(alg & SCHKDIGALG_TEST), то рассчитанная контрольная цифра
//   -1 - выполнение функции возможно только в режиме проверки
// Note: некоторые алгоритмы предполагают наличие более одной контрольной цифры. Для таких
// алгоритмов возможно использование функции только в режиме проверки (alg & SCHKDIGALG_TEST)
//
int SCalcCheckDigit(int alg, const char * pInput, size_t inputLen);

#endif // } _WIN32_WCE
//
// IDEA
//
// Определен в md5.h
//class MD5 {
//public:
//	SLAPI  MD5() { init(); }
//	void   SLAPI init();
//	void   SLAPI update(uchar * bug, uint len);
//	void   SLAPI final(uchar digest[16]);
//private:
//	uint32 buf[4]; // scratch buffer
//	uint32 i[2];   // number of _bits_ handled mod 2^64
//	uchar  in[64]; // input buffer
//};


//#ifndef MD5_H_INCLUDED
//#define MD5_H_INCLUDED

//#include "base_types.h"

class S_MD5 {
public:
	S_MD5();
	~S_MD5();
	void Init();
	void Update(const void *data, uint32 len);
	void Final(uint8 digest[16]);
	/* internal function */
	static void Transform(uint32 buf[4], uint32 in_data[16]);
	static void Calc(const void *data, uint32 length, uint8 digest[16]);
private:
	uint32 m_buf[4];
	uint32 m_bits[2];
	uint8  m_in[64];
	bool   m_need_byteswap;
	bool   m_big_endian;

	void reverse_u32(uint8 *buf, int n_u32);
};

//#endif /* MD5_H_INCLUDED */

void SLAPI Transform(uint32 *buf, uint32 *in);

#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8

// hashpass - Hash pass phrase down to 128 bits (16 bytes).
void SLAPI hashpass(char *keystring, int keylen, uint8 *hash);

#define IDEA_ROUNDS	8            // Don't change this value, should be 8
#define IDEA_KEYLEN	(6*IDEA_ROUNDS+4) // length of key schedule */

typedef uint16 IDEAkey[IDEA_KEYLEN];

class IDEACFB {
public:
	SLAPI  IDEACFB();
	SLAPI  IDEACFB(uint16 iv0[4], uint8 key[16], int decryp);
	SLAPI ~IDEACFB();
	void   SLAPI init(uint16 iv0[4], uint8 key[16], int decryp);
	void   SLAPI close();
	void   SLAPI run(uint8 * buf, int count);
private:
	// Key schedules for IDEA encryption and decryption
	IDEAkey  Z;
	uint16 * iv_idea;     // pointer to IV for CFB or CBC
	int      cfb_dc_idea; // TRUE iff CFB decrypting
};

class IDEARAND {
public:
	SLAPI  IDEARAND();
	SLAPI  IDEARAND(uint8 key[16], uint8 seed[8], uint32 tstamp);
	SLAPI ~IDEARAND();
	void   SLAPI init(uint8 key[16], uint8 seed[8], uint32 tstamp);
	uint8  SLAPI rand();
	void   SLAPI close();
private:
	uint16 dtbuf[4];        // buffer for enciphered timestamp
	uint16 randseed[4];     // seed for IDEA random # generator
	uint16 randbuf[4];      // buffer for IDEA random # generator
	uint8  randbuf_counter; // # of random bytes left in randbuf
	IDEAkey randkey;         // Expanded key for IDEA random # generator
};

void SLAPI IdeaEncrypt(const char * password, void * buf, size_t len);
void SLAPI IdeaDecrypt(const char * password, void * buf, size_t len);
void SLAPI IdeaRandMem(void * buf, size_t len);
//
// Math
//

//
// Descr: Округляет n с точностью prec знаков после точки.
//
double FASTCALL round(double n, int prec);
//
// Descr: Округляет n до величины, кратной prec с учетом
//   направления округления, определяемого параметром dir.
// ARG(n    IN): Округляемое значение.
// ARG(prec IN): Кратность округления. Если prec == 0.0
//   то округляет с кратностью 0.01
// ARG(dir  IN): Направление округления:
//   <0 до меньшей величины
//   >0 до большей величины
//   0  до ближайшей величины по финансовому правилу (0.5 -> 1.0).
//
double SLAPI    round(double n, double prec, int dir);
double FASTCALL R0(double);
//
// Descr: Округляет входящий параметр до ближайшего целого числа
//
long   FASTCALL R0i(double);
double FASTCALL R2(double);
double FASTCALL R3(double);
double FASTCALL R4(double);
//
// Descr: Округляет входящий параметр до 5 знака после точки
//
double FASTCALL R5(double);
//
// Descr: Округляет входящий параметр до 6 знака после точки
//
double FASTCALL R6(double);
//
// Округляет n с точностью prec по правилу "до ближайшего четного"
//
double SLAPI roundnev(double n, int prec);
double SLAPI trunc(double n, int prec);
int    GetRandom(int min, int max);
//
// Int Money
// Операции с представлением числа с фиксированной точкой.
//
double FASTCALL intmnytodbl(long m);
long   FASTCALL dbltointmny(double r);
//
// Bit string's operations
//
void   FASTCALL resetbitstring(void * pBuf, size_t len);
void   FASTCALL setbit32(void * pBuf, size_t len, size_t pos);
void   FASTCALL resetbit32(void * pBuf, size_t len, size_t pos);
int    FASTCALL getbit8(const void * pBuf, size_t len, size_t pos);
int    FASTCALL getbit32(const void * pBuf, size_t len, size_t pos);
//
// Descr: Подсчитывает число установленных бит в двойном слове v
// See: http://graphics.stanford.edu/~seander/bithacks.html
//
uint32 FASTCALL popcount32(uint32 v);
//
// Descr: Рассчитывает побитовую четность двойного слова.
// See: http://graphics.stanford.edu/~seander/bithacks.html
//
uint32 FASTCALL parity32(uint32 v);
//
// Descr: Возвращает 4-байтовое число, составленное из count бит, начиная с
//   позиции pos в битовой строке. (32-count) старших битов результата нулевые.
//   count > 32 эквивалентно count == 32 (в отладочной версии вызывается assert(count <= 32)).
//
uint32 getbits(const void * pBuf, size_t len, size_t pos, size_t count);
int    FASTCALL findbit(const void * pBuf, size_t count, int val, size_t * pPos);
void   SLAPI insbit(void * pBuf, size_t len, size_t);
void   SLAPI delbit(void * pBuf, size_t len, size_t);
uint8  bitscanforward(uint32 * pIdx, uint32 mask);
uint8  bitscanreverse(uint32 * pIdx, uint32 mask);
//
// Table search functions
//
// @v9.4.3 (moved to sformat.cpp as static) int    SLAPI i_tab(const void * tbl, int req, int def);
// @v9.4.3 (moved to sformat.cpp as static) int    SLAPI ai_tab(const void * tbl, int req, int def);
//
// Encrypt / Decrypt functions
//
void * SLAPI encrypt(void *, size_t);
void * SLAPI decrypt(void *, size_t);
ulong  SLAPI _checksum__(const char *, size_t);
//
// MIME64 encoder/decoder
//
int    SLAPI encode64(const char * pIn, size_t inLen, char * pOut, size_t outMax, size_t * pOutLen);
int    SLAPI decode64(const char * pIn, size_t inLen, char * pOut, size_t * pOutLen);

class MIME64 {
public:
	MIME64();
	int    Encode(const void * pIn, size_t inLen, char * pOut, size_t outBufLen, size_t * pOutDataLen) const;
	int    Decode(const char * pIn, size_t inLen, char * pOut, size_t * pOutDataLen) const;
private:
	char   Basis64[256];
};
//
// Punycode
//
int SPunycodeEncode(const uint * input, size_t input_length, SString & rOut, const uchar * pCaseFlags);
int SPunycodeDecode(const char * input, size_t input_length, size_t * output_length, uint * output, uchar * pCaseFlags);
//
//
// @construction
int    EquipInputDecode(const void * pInput, size_t inputSize, int binaryFormat, int outpFormat, SString & rBuf);
//
// User abort functions
//
int SLAPI checkabort();
int SLAPI enableabort(int); // 0 - disable, 1 - enable, (-1) - check status
/*
	Streaming functions
	{ (uint16)strlen(pStr), pStr[] }
	if(pStr == 0) then { (uint16)0 }
*/
int SLAPI WritePStrToFile(const char * pStr, FILE * pStream);
int SLAPI ReadPStrFromFile(char ** ppStr, FILE * pStream);
int SLAPI ReadPStrFromFile(SString & rStr, FILE * pStream);
//
//
//
enum {
	comdvcsCom = 1,
	comdvcsLpt,
	comdvcsPrn,
	comdvcsCon
};

int    SLAPI IsComDvcSymb(const char * pSymb, int * pCount);
//
// Descr: Формирует строку вида COM1, LPT2, CON и т.д. в зависимости от параметров
//   comdvcs, count. Если (option & 0x0001), тогда впереди строки добавляются символы \\.\
//   необходимые для открытия порта в Win32.
//
SString & SLAPI GetComDvcSymb(int comdvcs, int count, int option, SString & rBuf);
int SLAPI GetPort(const char * pPortName, int * pPortNo);
//
// RS232 Communication
//
enum ComBaudRate {
	cbrUndefined = 0,
	cbr110,
	cbr300,
	cbr600,
	cbr1200,
	cbr2400,
	cbr4800,
	cbr9600,
	cbr14400,
	cbr19200,
	cbr38400,
	cbr56000,
	cbr57600,
	cbr115200,
	cbr128000,
	cbr256000
};

struct CommPortParams {    // @persistent size=6
	int16  Cbr;            // ComBaudRate
	int8   ByteSize;       // [4..8]
	int8   Parity;         // [0..4] (no, odd, even, mark, space)
	int8   StopBits;       // 0, 1, 2 (1, 1.5, 2)
	int8   Reserve;        // @reserve
};

struct CommPortTimeouts {
	uint16 Get_NumTries; /* (0..32000)      */
	uint16 Get_Delay;    /* (0..1000)       */
	uint16 Put_NumTries; /* (0..32000)      */
	uint16 Put_Delay;    /* (0..1000)       */

	uint16 Reserve1;
	uint16 W_Get_Delay;  /* (0..1000) Win32 */
	uint16 Reserve2[2];
};

class SCommPort {
public:
	SLAPI  SCommPort();
	SLAPI ~SCommPort();
	int    SLAPI GetParams(CommPortParams *) const;
	int    SLAPI GetTimeouts(CommPortTimeouts *) const;
	int    SLAPI SetParams(const CommPortParams *);
	int    SLAPI SetTimeouts(const CommPortTimeouts *);
	int    SLAPI InitPort(int portNo /* 0..3 */);
	int    FASTCALL PutChr(int c);
	int    SLAPI GetChr();
	int    FASTCALL GetChr(int * pChr);
	int    SLAPI SetReadCyclingParams(int cycleCount, int cycleDelay);
	int    SLAPI GetPortNo() const {return PortNo;}
protected:
	CommPortParams   CPP;
	CommPortTimeouts CPT;
	int    PortNo;
#ifdef __WIN32__
	HANDLE H_Port;
#endif
private:
	int    ReadCycleCount;
	int    ReadCycleDelay;
	int    NumGetColl;      // Количество коллизий при чтении
	int    MaxGetCollIters; // Максимальное число итераций чтения из-за коллизий
};

//
// Dynamic Array structures
//
#define DEFCOLLECTDELTA  32 // @v8.5.12 8-->32

#define UNDEFPOS 0xffffffffU

#define O_COLLECTION     (aryDataOwner | aryPtrContainer | aryEachItem)
#define O_ARRAY          (aryDataOwner)
#define O_N_O_ARRAY      (0)
//
// Search options
//
#define srchFirst        1
#define srchNext         2
#define srchLast         3
#define srchPrev         4
//
// Search flags
//
#define srchBinary  0x0100
#define srchNoCase  0x0200
#define srchFlags   0xff00 // Mask
//
// Array's flags
//
enum {
	arySorted       = 0x01,
	aryUnique       = 0x02,
	//
	// Флаг aryDataOwner предотвращает разрушение данных деструктором класса SArray.
	//
	aryDataOwner    = 0x04,
	//
	// Флаг aryPtrContainer предназначен для того, чтобы верно интерпретировать передаваемые и
	// возвращаемые параметры функциями-членами класса SArray и его потомками. Если этот флаг
	// установлен, то передаваемые функциям вставки (insert, atInsert, ordInsert) указатели
	// интерпретируются как указатели на объекты, переходящие в собственность массива, т.е. они
	// храняться в виде указателей и разрушаются автоматически при вызове деструктора массива или
	// функций atFree и freeAll.
	//
	aryPtrContainer = 0x08,
	//
	// Флаг aryEachItem предписывает классу SArray для разрушения и чтения/записи элементов
	// массива использовать виртуальные функции freeItem, readItem, writeItem соответственно
	// Если этот флаг не установлен и массив не имеет атрибута aryPtrContainer, то разрушение,
	// чтение и запись данных производится одной операцией над областью памяти, что значительно быстрее
	//
	aryEachItem     = 0x10,
	//
	// Флаг aryRing означает кольцевую структуру.
	// На текущий момент (29/11/2008) реализован только кольцевой стек.
	// Кольцевой стек увеличивается до размера delta единственный раз, а
	// затем при достижении лимита, запись осуществляется в начало массива.
	//
	aryRing         = 0x20,
	aryMorePerform  = 0x40  // @v9.1.1 @construction Поведенческий флаг: функции массива принимают меры для увеличения
		// производительности за счет большего объема используемой памяти
};
//
// SArray
//
// @attention Функции вставки и удаления элементов массива могут изменить местоположение данных в памяти.
// Если вы используете переменную, дублирующую адрес SArray::data, то ее следует актуализировать вызовом
// функции SArray::dataPtr()
//
#if 0 // @construction {

class SVector {
public:
	SLAPI  SVector(uint itemSize, int o = O_ARRAY);
	SLAPI  SVector(void * pData, uint itemSize, uint aCount, int o = O_N_O_ARRAY);
	SLAPI  SVector(const SVector & rS);
	SLAPI ~SVector();
	int    FASTCALL insert(const void * pItem);
	SVector & FASTCALL operator = (const SVector &);
	int    FASTCALL operator ! () const { return (count == 0); }
	int    FASTCALL copy(const SVector & src);
	//
	// Descr: Побайтно сравнивает массив this с массивом src.
	//   Если this->getCount() != src.getCount || this->getItemSize() != src.getItemSize(),
	//   то считает, что объекты не равны. Если эта проверка успешна, то сравнивает
	//   побайтно все элементы массивов.
	//   Для массивов с признаками aryPtrContainer и aryEachItem возвращает 0 (считает, что
	//   невозможно корректно побайтно сравнить такие объекты).
	//
	int    FASTCALL IsEqual(const SVector & src) const;
	int    SLAPI atPut(uint, const void *);
	int    FASTCALL atInsert(uint, const void *);
	int    SLAPI ordInsert(const void *, uint *, CompFunc, void * pExtraData = 0);
	//
	// Descr: меняет местами элементы с индексами p1 и p2.
	// Returns:
	//   1 - обмен прошел успешно
	//   0 - один из индексов выходит за пределы размеров массива
	//
	int    SLAPI swap(uint p1, uint p2);
	//
	// Descr: меняет порядок следования элементов массива [pos..pos+numItems-1].
	//   То есть:
	//      первый элемент становится последним, последний - первым
	//      второй элемент становится предпоследним, предпоследний - вторым и т.д.
	//
	int    SLAPI reverse(uint pos, uint numItems);
	//
	// Descr: Перемешивает элементы массива случайным образом
	//
	int    SLAPI shuffle();
	//
	// Descr: перемещает элемент в позиции pos на одну позицию вниз или вверх
	//   в зависимости от параметра dir.
	// ARG(pos      IN): позиция, в которой находится элемент, который надо переместить
	// ARG(dir      IN): направление перемещения: 0 - вниз, !0 - вверх
	// ARG(pNewPos OUT): по этому указателю присваивается новая позиция перемещенного элемента
	// Returns:
	//   !0 - перемещение осуществлено успешно. По указателю pNewPos присвоено значение новой
	//        позиции элемента, которых до этого находился в позиции pos.
	//   0  - ошибка позиционирования: либо pos >= getCount(), либо (dir == 0 && pos == getCount()-1),
	//        либо (dir != 0 && pos == 0).
	//
	int    SLAPI moveItem(uint pos, int dir, uint * pNewPos); // @>>SVector::swap
	int    SLAPI bsearch(const void * key, uint * pIdx, CompFunc, uint ofs, void * pExtraData) const;
	int    SLAPI bsearch(const void * key, uint * pIdx, CompFunc fcmp) const;
	int    SLAPI lsearch(const void * key, uint * pIdx, CompFunc, uint ofs = 0, void * pExtraData = 0) const;
	int    SLAPI sort(CompFunc fcmp, void * pExtraData = 0);
	int    SLAPI sort2(CompFunc fcmp, void * pExtraData = 0);
	//
	// Descr: Удаляет элемент массива по индексу pos. При этом, все последующие элементы
	//   сдвигаются на одну позицию вниз.
	// Returns:
	//   !0 - элемент успешно удален
	//   0  - ошибка. Единственная возможная причина ошибки: pos >= getCount().
	//      В этом случае код ошибки SLibError = SLERR_BOUNDS.
	//
	int    FASTCALL atFree(uint pos);
	//
	// Descr: Удаляет последний элемент массива.
	//
	int    SLAPI freeLast();
	//
	// Descr: удаляет все элементы массива и освобождает занятую под них память.
	//
	void   SLAPI freeAll();
	//
	// Descr: удаляет все элементы массива не освобождая при этом память, выделенную под data
	//   ("глубокие" данные, разрушение которых определено функцией freeItem, освобождаются).
	//
	void   SLAPI clear();
	void * SLAPI dataPtr() const;
	void * FASTCALL at(uint) const;
	int    FASTCALL enumItems(uint * pIdx, void ** ppItem) const;
	//
	// Descr: Возвращает указатель на элемент по индексу getPointer() если getPointer() < getCount().
	//   В противном случае возвращает 0.
	//   При каждом успешном завершении увеличивает значение внутреннего указателя (incPointer()).
	//   В отличии от enumItems работает быстрее и не требует дополнительной переменной итератора.
	//
	// Snippet:
	//
	// ArrayItem * p_item;
	// SVector array(sizeof(ArrayItem));
	// ...
	// for(array.setPointer(0); (p_item = (ArrayItem *)array.next()) != 0;) {
	//    ...
	// }
	//
	void * SLAPI next();
	uint   SLAPI getCount() const { return count; }
	int    SLAPI getEnd() const { return (int)(count-1); }
	uint   SLAPI getItemSize() const { return isize; }
	uint   SLAPI getFlag() const;
	void   SLAPI setFlag(uint val, int set);
	void   SLAPI setLimit(uint);
	void   SLAPI setDelta(uint);
	//
	// Descr: Возвращает значение внутреннего указателя позиции
	//
	uint   SLAPI getPointer() const;
	//
	// Descr: Устанавливает значение внутреннего указателя позиции
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   FASTCALL setPointer(uint p);
	//
	// Descr: Увеличивает на единицу значение внутреннего указателя позиции
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   SLAPI incPointer();
	//
	// Descr: осуществляет бинарный поиск по всем записям массива.
	//   Каждая запись просматривается со смещения ofs.
	//
	int    SLAPI imp_bsearch(const void *, uint *, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: осуществляет последовательный поиск по
	//   всем записям массива, начиная с записи *pos. Каждая запись
	//   просматривается со смещения ofs.
	//
	int    SLAPI imp_lsearch(const void *, uint * pos, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: Оптимизированная функция сложения real-поля по всем элементам массива
	// @construction
	//
	double SLAPI calcSumDouble(size_t offs) const;
	//
	// Descr: Функция массовой вставки элементов в массив. Работает быстрее, чем
	//   вставка по-одному, но требует большей тщательности в использовании.
	//   Наибольшую сложность представляет вставка указателей на элементы (aryPtrContainer).
	//   В этом случае указатель pItems должен быть массивом указателей, которые должны
	//   быть вставлены. Например:
	//
	//   SomeStruct * p_ptr_list[32];
	//   SCollection some_struct_list;
	//   // Заполняем список p_ptr_list
	//   for(uint i = 0; i < sizeof(p_ptr_list) / sizeof(p_ptr_list[0]); i++)
	//      p_ptr_list[i] = new SomeStruct;
	//   some_struct_list.insertChunk(32, p_ptr_list);
	//
	int    SLAPI insertChunk(uint numItemsToInsert, const void * pItems);
	//
	// Descr: Удаляет из массива диапазон элементов, начинающийся с позиции lowPos и до uppPos включетельно.
	//   Если lowPos > uppPos или lowPos >= getCount(), то не делает ничего
	//   Если lowPos == uppPod, то вызов эквивалентен freeItem(lowPos)
	//   Если lowPos < getCount(), то удаляет все элементы от позиции lowPos до MIN(uppPos, getCount()-1)
	// Note: Главное назначение функции - ускорить массовое удаление за счет сокращения издержек
	//   на перемещение участков памяти при серии единичных удалений.
	//
	int    SLAPI freeChunk(uint lowPos, uint uppPos);
	//
	// Descr: Функция записывает массив в бинарный файл rFile.
	//   Формат записи следующий:
	//   uint32 isize
	//   uint32 count
	//   uint8  bytes[count * isize]
	// Note:
	//   Этой функцией не следует подменять унаследованныю функцию WriteSArrayToFile(const SArray *, FILE *)
	//   поскольку формат записи этих функций различается.
	//
	int    SLAPI Write(SFile & rFile, long) const;
	//
	// Descr: Функция считывает массив из бинарного файла rFile. Формат считывания //
	//   соответствует тому, что определен для функции SArray::Write(SFile &, long).
	// Note: Если размер записи в файле не соответсвует размеру записи this->isize,
	//   то функция возвращает ошибку.
	//   Перед считыванием массив this очищается.
	//   Этой функцией не следует подменять унаследованныю функцию ReadSArrayFromFile(SArray *, FILE *)
	//   поскольку формат чтения этих функций различается.
	//
	int    SLAPI Read(SFile & rFile, long);
protected:
	//
	// Descr: Аналог expand(numNewRecs, hsize). Из-за того, что в 99% (или большем) случаев
	//   заголовок в массиве не используется или не меняется его размер, мы с'кономим на
	//   передаче и проверке лишнего параметра. Учитывая, что функция вызывается очень часто
	//   производительность операций вставки в массив улучшится.
	//
	int    FASTCALL expand(uint numNewRecs);
	int    SLAPI shrink();
	int    SLAPI Helper_Cmp(const void * i1, const void * i2, CompFunc fcmp, void * pExtraData) const;
	int    SLAPI isItemEq(const void * pKey, uint pos, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const;

	uint   Count;   // Количество элементов, находящихся в массиве
	uint   Pointer; // Указатель на текущий элемент массива (используется в стеках и очередях)
	uint   Limit;   // Максимальное количество элементов, которое может вместить буфер
	uint16 ISize;   // Размер одного элемента (bytes)
	uint16 BFlags;
	void * P_Data;
};

#endif // } @construction

class SArray {
public:
	SLAPI  SArray(uint itemSize, uint aDelta, int o /*= O_ARRAY*/);
	//
	// Descr: Создает простой массив (O_ARRAY) со значением delta по умолчанию (8)
	//   и размером элемента - itemSize.
	//
	SLAPI  SArray(uint itemSize);
	SLAPI  SArray(void * pData, uint itemSize, uint aCount, int o = O_N_O_ARRAY);
	SLAPI  SArray(const SArray & rS);
	virtual SLAPI ~SArray();
	/* @v9.0.5 virtual */ int FASTCALL insert(const void * pItem);
	SArray & FASTCALL operator = (const SArray &);
	int    FASTCALL operator ! () const { return (count == 0); } // @v7.0.6
	int    FASTCALL copy(const SArray & src);
	//
	// Descr: Побайтно сравнивает массив this с массивом src.
	//   Если this->getCount() != src.getCount || this->getItemSize() != src.getItemSize(),
	//   то считает, что объекты не равны. Если эта проверка успешна, то сравнивает
	//   побайтно все элементы массивов.
	//   Для массивов с признаками aryPtrContainer и aryEachItem возвращает 0 (считает, что
	//   невозможно корректно побайтно сравнить такие объекты).
	//
	int    FASTCALL IsEqual(const SArray & src) const;
	int    SLAPI atPut(uint, const void *);
	int    FASTCALL atInsert(uint, const void *);
	int    SLAPI ordInsert(const void *, uint *, CompFunc, void * pExtraData = 0);
	//
	// Descr: меняет местами элементы с индексами p1 и p2.
	// Returns:
	//   1 - обмен прошел успешно
	//   0 - один из индексов выходит за пределы размеров массива
	//
	int    SLAPI swap(uint p1, uint p2);
	//
	// Descr: меняет порядок следования элементов массива [pos..pos+numItems-1].
	//   То есть:
	//      первый элемент становится последним, последний - первым
	//      второй элемент становится предпоследним, предпоследний - вторым и т.д.
	//
	int    SLAPI reverse(uint pos, uint numItems);
	//
	// Descr: Перемешивает элементы массива случайным образом
	//
	int    SLAPI shuffle();
	//
	// Descr: перемещает элемент в позиции pos на одну позицию вниз или вверх
	//   в зависимости от параметра dir.
	// ARG(pos      IN): позиция, в которой находится элемент, который надо переместить
	// ARG(dir      IN): направление перемещения: 0 - вниз, !0 - вверх
	// ARG(pNewPos OUT): по этому указателю присваивается новая позиция перемещенного элемента
	// Returns:
	//   !0 - перемещение осуществлено успешно. По указателю pNewPos присвоено значение новой
	//        позиции элемента, которых до этого находился в позиции pos.
	//   0  - ошибка позиционирования: либо pos >= getCount(), либо (dir == 0 && pos == getCount()-1),
	//        либо (dir != 0 && pos == 0).
	//
	int    SLAPI moveItem(uint pos, int dir, uint * pNewPos); // @>>SArray::swap
	int    SLAPI bsearch(const void * key, uint * pIdx, CompFunc, size_t ofs, void * pExtraData) const;
	int    SLAPI bsearch(const void * key, uint * pIdx, CompFunc fcmp) const;
	int    SLAPI lsearch(const void * key, uint * pIdx, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	void   SLAPI sort(CompFunc fcmp, void * pExtraData = 0);
	void   SLAPI sort2(CompFunc fcmp, void * pExtraData = 0);
	//
	// Descr: Удаляет элемент массива по индексу pos. При этом, все последующие элементы
	//   сдвигаются на одну позицию вниз.
	// Returns:
	//   !0 - элемент успешно удален
	//   0  - ошибка. Единственная возможная причина ошибки: pos >= getCount().
	//      В этом случае код ошибки SLibError = SLERR_BOUNDS.
	//
	int    FASTCALL atFree(uint pos);
	//
	// Descr: Удаляет последний элемент массива.
	//
	int    SLAPI freeLast();
	//
	// Descr: удаляет все элементы массива и освобождает занятую под них память.
	//
	void   SLAPI freeAll();
	//
	// Descr: удаляет все элементы массива не освобождая при этом память, выделенную под data
	//   ("глубокие" данные, разрушение которых определено функцией freeItem, освобождаются).
	//
	void   SLAPI clear();
	void * SLAPI dataPtr() const;
	void * FASTCALL at(uint) const;
	int    FASTCALL enumItems(uint * pIdx, void ** ppItem) const;
	//
	// Descr: Возвращает указатель на элемент по индексу getPointer() если getPointer() < getCount().
	//   В противном случае возвращает 0.
	//   При каждом успешном завершении увеличивает значение внутреннего указателя (incPointer()).
	//   В отличии от enumItems работает быстрее и не требует дополнительной переменной итератора.
	//
	// Snippet:
	//
	// ArrayItem * p_item;
	// SArray array(sizeof(ArrayItem));
	// ...
	// for(array.setPointer(0); (p_item = (ArrayItem *)array.next()) != 0;) {
	//    ...
	// }
	//
	void * SLAPI next();
	uint   SLAPI getCount() const { return count; }
	int    SLAPI getEnd() const { return (int)(count-1); }
	uint   SLAPI getItemSize() const { return isize; }
	uint   SLAPI getFlag() const;
	void   SLAPI setFlag(uint val, int set);
	void   SLAPI setLimit(uint);
	void   SLAPI setDelta(uint);
	//
	// Descr: Возвращает значение внутреннего указателя позиции
	//
	uint   SLAPI getPointer() const;
	//
	// Descr: Устанавливает значение внутреннего указателя позиции
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   FASTCALL setPointer(uint p);
	//
	// Descr: Увеличивает на единицу значение внутреннего указателя позиции
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   SLAPI incPointer();
	//
	// Descr: Изменяет значение внутреннего указателя позиции на величину incr.
	//   При этом не выходя за границы диапазона [0..getCount()-1].
	//   Если incr < 0, то указатель уменьшается, в противном случае - увеличивается.
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   FASTCALL incPointerSafe(int incr);
	//
	// Descr: Функция должна разрушить объект, по указателю p.
	//   Эта функция вызывается при разрушении массива или удалении его элемента в случае,
	//   если (flags & aryEachItem)
	//
	virtual	void FASTCALL freeItem(void * p);
	//
	// Descr: осуществляет бинарный поиск по всем записям массива.
	//   Каждая запись просматривается со смещения ofs.
	//
	int    SLAPI imp_bsearch(const void *, uint *, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: осуществляет последовательный поиск по
	//   всем записям массива, начиная с записи *pos. Каждая запись
	//   просматривается со смещения ofs.
	//
	int    SLAPI imp_lsearch(const void *, uint * pos, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: Оптимизированная функция сложения real-поля по всем элементам массива
	// @construction
	//
	double SLAPI calcSumDouble(size_t offs) const;
	//
	// Descr: Функция массовой вставки элементов в массив.
	// ARG(numItemsToInsert IN): количество новых элементов, добавляемых в массив.
	// ARG(pItems IN): указатель на блок памяти, содержащий значения новых элементов.
	//   Если pItems == 0, то все вновь распределенное пространство обнуляется.
	// Note: Работает быстрее, чем вставка по-одному, но требует большей тщательности в использовании.
	//   Наибольшую сложность представляет вставка указательей на элементы (aryPtrContainer).
	//   В этом случае указатель pItems должен быть массивом указателей, которые должны
	//   быть вставлены. Например:
	//
	//   SomeStruct * p_ptr_list[32];
	//   SCollection some_struct_list;
	//   // Заполняем список p_ptr_list
	//   for(uint i = 0; i < sizeof(p_ptr_list) / sizeof(p_ptr_list[0]); i++)
	//      p_ptr_list[i] = new SomeStruct;
	//   some_struct_list.insertChunk(32, p_ptr_list);
	//
	int    SLAPI insertChunk(uint numItemsToInsert, const void * pItems);
	//
	// Descr: Удаляет из массива диапазон элементов, начинающийся с позиции lowPos и до uppPos включетельно.
	//   Если lowPos > uppPos или lowPos >= getCount(), то не делает ничего
	//   Если lowPos == uppPod, то вызов эквивалентен freeItem(lowPos)
	//   Если lowPos < getCount(), то удаляет все элементы от позиции lowPos до MIN(uppPos, getCount()-1)
	// Note: Главное назначение функции - ускорить массовое удаление за счет сокращения издержек
	//   на перемещение участков памяти при серии единичных удалений.
	//
	int    SLAPI freeChunk(uint lowPos, uint uppPos);
	//
	// Descr: Функция записывает массив в бинарный файл rFile.
	//   Формат записи следующий:
	//   uint32 isize
	//   uint32 count
	//   uint8  bytes[count * isize]
	// Note:
	//   Этой функцией не следует подменять унаследованныю функцию WriteSArrayToFile(const SArray *, FILE *)
	//   поскольку формат записи этих функций различается.
	//
#ifndef _WIN32_WCE // {
	int    SLAPI Write(SFile & rFile, long) const;
	//
	// Descr: Функция считывает массив из бинарного файла rFile. Формат считывания //
	//   соответствует тому, что определен для функции SArray::Write(SFile &, long).
	// Note: Если размер записи в файле не соответсвует размеру записи this->isize,
	//   то функция возвращает ошибку.
	//   Перед считыванием массив this очищается.
	//   Этой функцией не следует подменять унаследованныю функцию ReadSArrayFromFile(SArray *, FILE *)
	//   поскольку формат чтения этих функций различается.
	//
	int    SLAPI Read(SFile & rFile, long);
#endif // } !_WIN32_WCE
protected:
	//
	// Descr: Аналог expand(numNewRecs, hsize). Из-за того, что в 99% (или большем) случаев
	//   заголовок в массиве не используется или не меняется его размер, мы с'кономим на
	//   передаче и проверке лишнего параметра. Учитывая, что функция вызывается очень часто
	//   производительность операций вставки в массив улучшится.
	//
	int    FASTCALL expand(uint numNewRecs);
	int    SLAPI shrink();
	int    SLAPI Helper_Cmp(const void * i1, const void * i2, CompFunc fcmp, void * pExtraData) const;
	int    SLAPI isItemEq(const void * pKey, uint pos, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const;

	uint   count;   // Количество элементов, находящихся в массиве
	uint   pointer; // Указатель на текущий элемент массива (используется в стеках и очередях)
	uint   isize;   // Размер одного элемента (bytes)
	uint   Limit;   // Максимальное количество элементов, которое может вместить буфер
	uint16 flags;
	uint16 delta;   //
	uint8 * data;
};
//
// Streaming functions
// { (uint16)count; data[count*isize] }
//
int    SLAPI WriteSArrayToFile(const SArray * pAry, FILE * pStream);
int    SLAPI ReadSArrayFromFile(SArray * pAry, FILE * pStream);
//
// SCollection
//
class SCollection : public SArray {
public:
	SLAPI  SCollection(uint aDelta = DEFCOLLECTDELTA, int o = O_COLLECTION);
	SLAPI  SCollection(const SCollection & src) : SArray(src) {}
};
//
// SStack
//
class SStack : public SArray {
public:
	SLAPI  SStack(uint itemSize, uint aDelta = 8, int o = O_ARRAY); // @v8.9.12 aDelta 1-->8
	int    FASTCALL push(const void *);
	int    FASTCALL pop(void *);
	void * SLAPI pop();
	int    SLAPI undo(uint savedPos);
	void * SLAPI peek() const;
};

class SQueue : public SArray {
public:
	SLAPI  SQueue(uint itemSize, uint queueSize, int o = O_ARRAY);
	SLAPI  SQueue(const SQueue &);
	void   SLAPI clear();
	int    FASTCALL copy(const SQueue & src);
	int    FASTCALL push(const void *);
	void * SLAPI pop();
	void * SLAPI peek() const;
	uint   SLAPI size() const { return QueueSize; }
	uint   SLAPI getNumItems() const;
	void * SLAPI get(uint) const;
private:
	uint   QueueSize;
	uint   Front;
	uint   End;
};

class STypArray : public SArray {
public:
	SLAPI  STypArray(TYPEID, uint aDelta = 8, int o = O_ARRAY);
	// @v9.0.5 virtual int FASTCALL insert(const void *);
	virtual	void FASTCALL freeItem(void *);
	TYPEID SLAPI getType() const { return type; }
	int    SLAPI search(const void *, uint *) const;
	void   SLAPI sort();
	SString & SLAPI itemToString(uint idx, long fmt, SString &) const;
protected:
	TYPEID type;
};

template <class T> class TSArray : public SArray {
public:
	TSArray(int o = O_ARRAY) : SArray(sizeof(T), 8, o) {}
	TSArray(const TSArray & src) : SArray(src) {}
	T &    FASTCALL at(uint pos) const { return *(T *)SArray::at(pos); }
};

template <class T> class TSCollection : public SCollection {
public:
	TSCollection() : SCollection(DEFCOLLECTDELTA, O_COLLECTION)
	{
	}
	~TSCollection()
	{
		freeAll();
	}
	T *    FASTCALL at(uint pos) const
	{
		return (T *)SCollection::at(pos);
	}
	virtual void FASTCALL freeItem(void * pItem)
	{
		delete (T *)pItem;
	}
	//
	// Descr: Распределяет память для нового элемента T и вставляет его
	//   в коллекцию.
	// ARG(pPosition OUT): @{vptr0} указатель, по которому функция присваивает позицию в коллекции
	//   нового элемента. В случае ошибки значение по указателю не меняется.
	// Returns:
	//   !0 - Указатель на вновь созданных элемент
	//   0  - Ошибка
	//
	T * FASTCALL CreateNewItem(uint * pPosition)
	{
		T * p_new_item = new T;
		if(!p_new_item)
			SLS.SetError(SLERR_NOMEM);
		else if(insert(p_new_item)) {
			ASSIGN_PTR(pPosition, count-1);
		}
		else
			ZDELETE(p_new_item);
		return p_new_item;
	}
	T * FASTCALL CreateNewItem()
	{
		T * p_new_item = new T;
		if(!p_new_item)
			SLS.SetError(SLERR_NOMEM);
		else if(!insert(p_new_item))
			ZDELETE(p_new_item);
		return p_new_item;
	}
};
//
// Descr: Функция копирования коллекций TSCollection.
// Note: Функция не является членом TSCollection по той причине, что в общем случае не
//   известен способ создания и копирования элементов коллекции. Данная функция предполагает,
//   что новые элементы создаются методом new а копируются штатным operator = ()
//
template <class T> int TSCollection_Copy(TSCollection <T> & rDest, const TSCollection <T> & rSrc)
{
	rDest.freeAll();
	for(uint i = 0; i < rSrc.getCount(); i++) {
		const T * p_src_item = rSrc.at(i);
		if(p_src_item) {
			T * p_new_item = rDest.CreateNewItem();
			if(p_new_item)
				*p_new_item = *p_src_item;
			else
				return 0;
		}
	}
	return 1;
}

template <class T> int TSDupPtr(T ** ppDest, T * pSrc)
{
	int    ok = 1;
	assert(ppDest);
	ZDELETE(*ppDest);
	if(pSrc) {
		*ppDest = new T;
		if(*ppDest)
			**ppDest = *pSrc;
		else
			ok = SLS.SetError(SLERR_NOMEM);
	}
	return ok;
}

template <class T> class TSStack : public SStack {
public:
	TSStack(uint aDelta = 8) : SStack(sizeof(T), aDelta, O_ARRAY) {}
	void   FASTCALL push(const T & t) { SStack::push(&t); }
	int    FASTCALL pop(T & t) { return SStack::pop(&t); }
	T &    FASTCALL peek() const { return *(T *)SStack::peek(); }
};

template <class T> class TSRingStack : public SStack {
public:
	TSRingStack(uint aDelta) : SStack(sizeof(T), aDelta, O_ARRAY|aryRing) {}
	void   FASTCALL push(const T & t) { SStack::push(&t); }
	int    FASTCALL pop(T & t)  { return SStack::pop(&t); }
	T &    FASTCALL peek() const { return *(T *)SStack::peek(); }
};
//
// Descr: Шаблонная функция, позволяющая реализовать глобальный циклический
//   стек для обеспечения примитивов, оперирующих простыми объектами, размеры
//   которых превышают возможности системного стека.
//   Например, такой стек позволяет реализовать быстрые операции над
//   точками координат (@see: RPoint & FASTCALL operator + (const RPoint & one, const RPoint & two)).
// @attention
//
//
template <class T, uint S> T & FASTCALL PushRecycledObject(T & rObj)
{
	static TSRingStack <T> * P_Stack; // @global @threadsafe
	T * p_ret = 0;
	ENTER_CRITICAL_SECTION
		if(P_Stack == 0)
			P_Stack = new TSRingStack <T> (S);
		if(P_Stack) {
			P_Stack->push(rObj);
			p_ret = &P_Stack->peek();
		}
	LEAVE_CRITICAL_SECTION
	return *p_ret;
}
//
//
//
template <class T> class TSQueue : public SQueue {
public:
	TSQueue(uint queueSize) : SQueue(sizeof(T), queueSize) {}
	TSQueue(const TSQueue & src) : SQueue(src) {}
	int    FASTCALL push(const T & t) { return SQueue::push(&t); }
	T &    FASTCALL pop() { return *(T *)SQueue::pop(); }
	T &    FASTCALL peek() const { return *(T *)SQueue::peek(); }
	T &    FASTCALL get(uint p) const { return *(T *)SQueue::get(p); }
};

DECL_CMPFUNC(PcharNoCase);
DECL_CMPFUNC(_PcharNoCase);
DECL_CMPFUNC(Pchar);
DECL_CMPFUNC(int);
DECL_CMPFUNC(int16);
DECL_CMPFUNC(long);
DECL_CMPFUNC(int64);
DECL_CMPFUNC(uint);
DECL_CMPFUNC(double);
DECL_CMPFUNC(_2long);
DECL_CMPFUNC(_2int64);
DECL_CMPFUNC(LDATE);
DECL_CMPFUNC(LDATETIME);
DECL_CMPFUNC(SDirEntry_Time);
DECL_CMPFUNC(S_GUID);
//
// Descr: Реализует сравнение двух элементов STimeChunk.
//   Если extraParam != 0, то осуществляется обратное сравнение (item2 vs item1).
//
DECL_CMPFUNC(STimeChunk);

typedef TSCollection <char> SStrCollection;

class RealArray : public TSArray <double> {
public:
	SLAPI  RealArray();
	SLAPI  RealArray(const RealArray & s);
	RealArray & FASTCALL operator = (const RealArray & s);
	double & FASTCALL operator [] (size_t i) const;
	//
	// Descr: Распределяет память ровно на numItems элементов.
	//   Все элементы обнуляются.
	//
	int    FASTCALL dim(uint numItems);
	double SLAPI Sum() const;
	void   SLAPI Sort();
};
//
//
//
class FloatArray : public TSArray <float> {
public:
	SLAPI  FloatArray();
	int    FASTCALL operator == (const FloatArray & rS) const;
	int    FASTCALL operator != (const FloatArray & rS) const;
	int    FASTCALL IsEqual(const FloatArray *) const;
	int    FASTCALL add(float);
	int    FASTCALL add(FPoint);
	float  FASTCALL get(uint) const;
	//
	// Descr: Распределяет память ровно на numItems элементов.
	//   Все элементы обнуляются.
	//
	int    FASTCALL dim(uint numItems);
	//
	// Descr: Специализированная функция, позволяющая извлечь 2-мерную точку из массива,
	//   начиная с позиции i.
	//
	const  FPoint & FASTCALL getPoint(uint i) const;
	float & FASTCALL operator [] (size_t i) const;
};
//
//
//
class LongArray : public TSArray <long> {
public:
	SLAPI  LongArray();
	SLAPI  LongArray(const LongArray & s);
	LongArray & FASTCALL operator = (const LongArray & s);
	int    FASTCALL operator == (const LongArray & rS) const;
	int    FASTCALL operator != (const LongArray & rS) const;
	int    SLAPI ordInsert(long, uint * pPos);
	int    FASTCALL bsearch(long, uint * pPos) const;
	int    FASTCALL lsearch(long, uint * pPos) const;
	int    FASTCALL bsearch(long) const;
	int    FASTCALL lsearch(long) const;
	int    SLAPI isList() const;
	int    FASTCALL IsEqual(const LongArray *) const;
	long   FASTCALL get(uint) const;
	long   SLAPI getSingle() const;
	long   SLAPI getLast() const;
	int    FASTCALL setSingleNZ(long id);
	int    SLAPI freeByKey(long, int binarySearch); //
	int    SLAPI removeByID(long);                  // @warning dup function (see above)
	void   SLAPI sort();
	void   SLAPI sortAndUndup();
	int    SLAPI isSorted() const;
	//
	// Descr: оставляет в массиве только те элементы, которые одновременно содержатся в
	//   массиве pList. Если pList == 0, то массив this очищается (freeAll).
	// ARG(pList  IN): Массив, пересечение с которым необходимо получить
	// ARG(binary IN): Если этот параметр !0, то присутствие элемента массива this
	//   в массиве pList выясняется применением функции бинарного поиска, что
	//   значительно ускоряет выполнение, но предполагает, что pList отсортирован
	//   (@see LongArray::sort()).
	//
	void   SLAPI intersect(const LongArray * pList, int binary = 0);
#ifndef _WIN32_WCE // {
	void   SLAPI intersect(const UintHashTable * pList);
#endif // } _WIN32_WCE
	int    FASTCALL add(long);
	//
	// Descr: Вставляет элемент value в конец списка только если value != 0
	//
	int    FASTCALL addnz(long value);
	int    FASTCALL addUnique(long);
	int    SLAPIV addzlist(long first, ...);
	int    FASTCALL add(const LongArray *);
	int    FASTCALL addUnique(const LongArray *);
	//
	// Descr: Добавляет в массив this все элементы из pS, которых нет в this, и которые
	//   отсутствуют в pExcl. Если pExcl == 0, то работает как addUnique.
	//   Элементы, присутствующие в pExcl из существующих до вызова функции в this
	//   элементов не удаляются.
	//
	int    SLAPI addUniqueExclusive(const LongArray * pS, const LongArray * pExcl);
	//
	// Descr: То же, что и addUniqueExclusive(), но не проверяет добавляемые элементы на уникальность.
	//   За счет этого функция работает на много быстрее, чем addUniqueExclusive().
	//
	int    SLAPI addExclusive(const LongArray * pS, const LongArray * pExcl);
};

// @v8.6.0 typedef TSArray      <int>  IntArray;
typedef LongArray IntArray;
//
//
//
class Int64Array : public TSArray <int64> {
public:
	SLAPI  Int64Array();
	SLAPI  Int64Array(const Int64Array & s);
	Int64Array & FASTCALL operator = (const Int64Array & s);
	int    FASTCALL operator == (const Int64Array & rS) const;
	int    FASTCALL operator != (const Int64Array & rS) const;
	int    FASTCALL IsEqual(const Int64Array * pS) const;
	int64  FASTCALL get(uint pos) const;
	int    lsearch(int64 key, uint * pPos) const;
	int    FASTCALL lsearch(int64 key) const;
	int    FASTCALL add(int64);
	int    FASTCALL add(int32);
	int    FASTCALL addUnique(int64);
};
//
//
//
typedef TSArray <S_GUID> UuidArray;
//
//
//
class FloatStack : public TSStack <float> {
public:
	SLAPI  FloatStack();
	int    SLAPI pushPoint(FPoint p);
	int    SLAPI popPoint(FPoint & rP);
};
//
//
//
struct LAssoc {
	int    FASTCALL operator == (const LAssoc &) const;
	int32  Key;
	int32  Val;
};

class LAssocArray : public TSArray <LAssoc> {
public:
	SLAPI  LAssocArray();
	SLAPI  LAssocArray(const LAssocArray &);
	LAssocArray & FASTCALL operator = (const LAssocArray &);
	int    FASTCALL operator == (const LAssocArray &) const;
	//
	// Returns:
	//   0  - в массиве уже есть элемент с идентификатором id
	//   !0 - в массиве элемент с таким идентификатором отсутствует.
	//
	int    SLAPI CheckUnique(long key, int binary = 0) const;
	//
	// Descr: Считает количество элементов, соответствующих ключу key
	//
	long   FASTCALL Count(long key) const;
	int    SLAPI GetListByKey(long key, LongArray & rList) const;
	int    SLAPI GetListByVal(long val, LongArray & rList) const;
	int    SLAPI Add(long key, long val, uint * pPos, int inOrder = 0);
	int    SLAPI AddUnique(long key, long val, uint * pPos, int inOrder = 0);
	int    SLAPI Update(long key, long val, int binary = 0);
	int    SLAPI Remove(long key, int binary = 0);
	//
	// Descr:
	//   Находит в массиве элемент, соответствующий ключу key. Если такой элемент найден,
	//   то по указателю pVal возвращается значение, соответсвующее ключу, а по указателю
	//   pPos - позиция найденного элемента в массиве.
	//   Если ключ не найден, то значения по этим указателям не меняются.
	//   Если в массиве содержится более одного элемента со значением key, то при линейном
	//   поиске (binary == 0) найден будет только самый первый, при бинарном поиске (binary == 1)
	//   выбор среди этих элементов непредсказуем.
	// ARG(key    IN): искомое значение ключа в массиве
	// ARG(pVal  OUT): указатель, по которому присваивается значение ключа, если указанный ключ найден.
	//   если ключ не найден, то значение по этому указателю не меняется.
	// ARG(pPos  OUT): указатель, по которому присваивается позиция (0..) найденного элемента в массиве.
	//   если ключ не найден, то значение по этому указателю не меняется.
	// ARG(binary IN): если !0, то используется бинарный поиск (при этом вы должны быть уверены, что
	//   массив отсортирован.
	// Returns:
	//   >0 - ключ key найден в массиве
	//   0  - ключ не найден
	//
	int    SLAPI Search(long key, long * pVal, uint * pPos/*, int binary = 0*/) const;
	//
	// Descr: Тоже что и Search, но поиск бинарный (предполагается, что список отсортирован по ключу в возрастающем порядке).
	//
	int    SLAPI BSearch(long key, long * pVal, uint * pPos) const;
	int    SLAPI SearchByVal(long val, long * pKey, uint * pPos) const;
	int    SLAPI SearchPair(long key, long val, uint * pPos) const;
	int    SLAPI CheckFlag(long key, long mask, int binary = 0) const;
	void   SLAPI Sort();
};
//
//
//
struct LLAssoc {
	int    FASTCALL operator == (const LLAssoc &) const;
	int64  Key;
	int64  Val;
};

class LLAssocArray : public TSArray <LLAssoc> {
public:
	SLAPI  LLAssocArray();
	SLAPI  LLAssocArray(const LLAssocArray &);
	LLAssocArray & FASTCALL operator = (const LLAssocArray &);
	int    FASTCALL operator == (const LLAssocArray &) const;
	//
	// Returns:
	//   0  - в массиве уже есть элемент с идентификатором id
	//   !0 - в массиве элемент с таким идентификатором отсутствует.
	//
	int    SLAPI CheckUnique(int64 key, int binary = 0) const;
	//
	// Descr: Считает количество элементов, соответствующих ключу key
	//
	long   FASTCALL Count(int64 key) const;
	int    SLAPI GetListByKey(int64 key, Int64Array & rList) const;
	int    SLAPI GetListByVal(int64 val, Int64Array & rList) const;
	int    SLAPI Add(int64 key, int64 val, uint * pPos, int inOrder = 0);
	int    SLAPI AddUnique(int64 key, int64 val, uint * pPos, int inOrder = 0);
	int    SLAPI Update(int64 key, int64 val, int binary = 0);
	int    SLAPI Remove(int64 key, int binary = 0);
	//
	// Descr:
	//   Находит в массиве элемент, соответствующий ключу key. Если такой элемент найден,
	//   то по указателю pVal возвращается значение, соответсвующее ключу, а по указателю
	//   pPos - позиция найденного элемента в массиве.
	//   Если ключ не найден, то значения по этим указателям не меняются.
	//   Если в массиве содержится более одного элемента со значением key, то при линейном
	//   поиске (binary == 0) найден будет только самый первый, при бинарном поиске (binary == 1)
	//   выбор среди этих элементов непредсказуем.
	// ARG(key    IN): искомое значение ключа в массиве
	// ARG(pVal  OUT): указатель, по которому присваивается значение ключа, если указанный ключ найден.
	//   если ключ не найден, то значение по этому указателю не меняется.
	// ARG(pPos  OUT): указатель, по которому присваивается позиция (0..) найденного элемента в массиве.
	//   если ключ не найден, то значение по этому указателю не меняется.
	// ARG(binary IN): если !0, то используется бинарный поиск (при этом вы должны быть уверены, что
	//   массив отсортирован.
	// Returns:
	//   >0 - ключ key найден в массиве
	//   0  - ключ не найден
	//
	int    SLAPI Search(int64 key, int64 * pVal, uint * pPos/*, int binary = 0*/) const;
	//
	// Descr: Тоже что и Search, но поиск бинарный (предполагается, что список отсортирован по ключу в возрастающем порядке).
	//
	int    SLAPI BSearch(int64 key, int64 * pVal, uint * pPos) const;
	int    SLAPI SearchByVal(int64 val, int64 * pKey, uint * pPos) const;
	int    SLAPI SearchPair(int64 key, int64 val, uint * pPos) const;
	void   SLAPI Sort();
};
//
//
//
struct RAssoc {
	long   Key;
	double Val;
};

class RAssocArray : public TSArray <RAssoc> {
public:
	SLAPI  RAssocArray();
	SLAPI  RAssocArray(const RAssocArray &);
	RAssocArray & FASTCALL operator = (const RAssocArray &);
	//
	// Descr: Ищет в массиве элемент со значением ключа key.
	//   Если в массиве присутствуют более одного элемента с ключем key,
	//   то при линейном поиске (binary == 0) возвращается значение и позиция //
	//   первого от начала массива, при бинарном поиске не специфицируется какой именно
	//   из множества элементов будет найден.
	// ARG(key    IN): Значение ключа, соответствующий которому элемент должен быть найден
	// ARG(pVal  OUT): @#{vptr0} Указатель, по которому будет записано значение найденного элемента
	//   {if Returns 0} - значение не изменяется //
	// ARG(pPos  OUT): @#{vptr0} Указатель, по которому возвращается позиция в массиве найденного элемента
	//   {if Returns 0} - значение не изменяется //
	// ARG(binary IN): Если !0, то используется бинарный поиск (предполагается, что массив отсортирован
	//   по значению ключа в возрастающем порядке, в противном случае результат поиска непредсказуем).
	// Returns:
	//   !0 - элемент со значением ключа key найден
	//   0  - в массиве нет элемента со значением ключа key
	//
	int    SLAPI Search(long key, double * pVal, uint * pPos, int binary = 0) const;
	int    FASTCALL Has(long key) const;
	double SLAPI Get(long key, int binary = 0) const; // @>>RAssocArray::Search
	int    SLAPI GetList(LongArray & rList, int unique) const;
	int    SLAPI Scale(double rate);

	enum {
		dfRound = 0x0001, // Округлять каждое распределенное значение до точность prec
		dfReset = 0x0002  // Сбросить остаток на последний элемент
	};
	int    SLAPI Distribute(double amount, long flags, int prec, RAssocArray & rResult) const;
	double SLAPI GetTotal() const;
	int    SLAPI Add(long key, double val, int additive = 1, int binary = 0);
	int    SLAPI Add(const RAssocArray & rList, int additive = 1, int binary = 0);
	int    SLAPI Remove(long key, int binary = 0);
	void   SLAPI SortByKey();
	//
	// Descr: Сортирует массив в порядке увеличения значения Val
	//
	void   SLAPI SortByVal();
	//
	// Descr: Сортирует массив в порядке уменьшения значения Val
	//
	void   SLAPI SortByValRev();
};
//
//
//
struct RPair {
	double X;
	double Y;
};

class RPairArray : public TSArray <RPair> {
public:
	SLAPI  RPairArray();
	SLAPI  RPairArray(const RPairArray &);
	RPairArray & FASTCALL operator = (const RPairArray &);
	RPair & FASTCALL operator [](size_t i) const;
	void   SLAPI SortByX();
	void   SLAPI SortByY();
};

#ifndef _WIN32_WCE // {
//
//
//
struct UUIDAssoc {
	long   Key;
	S_GUID Val;
};

class UUIDAssocArray : public TSArray <UUIDAssoc> {
public:
	SLAPI  UUIDAssocArray();
	int    SLAPI Add(long key, const S_GUID & rVal, uint * pPos);
	int    SLAPI Search(long key, S_GUID * pVal, uint * pPos) const;
	int    SLAPI SearchVal(const S_GUID &, long * pKey, uint * pPos) const;
};
#endif // } _WIN32_WCE
//
//
//
struct TaggedString {
	static TYPEID SLAPI BufType() { return (TYPEID)MKSTYPE(S_ZSTRING, TaggedString::BufSize()); }
	static size_t SLAPI BufSize() { return (sizeof(TaggedString)-sizeof(long)); }
	long   Id;
	char   Txt[64];
};

class TaggedStringArray : public TSArray <TaggedString> {
public:
	SLAPI  TaggedStringArray();
	TaggedStringArray & FASTCALL operator = (const TaggedStringArray &);
	int    SLAPI Search(long id, uint * pPos, int binary = 0) const;
	//
	// Descr: Ищет в массиве элемент, текст которого совпадает с параметром pTxt.
	//   поиск не чувствителен к регистру.
	//   Если в массиве содержится более одного элемента, соответствующего
	//   критерию то найден будет только самый первый.
	// Returns:
	//   !0 - элемент найден
	//   0  - элемент не найден
	//
	int    SLAPI SearchByText(const char * pTxt, uint * pPos) const;
	int    SLAPI Get(long id, char * pBuf, size_t bufLen, int binary = 0) const;
	int    SLAPI Get(long id, SString & rBuf, int binary = 0) const;
	int    SLAPI Add(long id, const char * pStr);
	void   SLAPI SortByID();
	void   SLAPI SortByText();
};
//
// Descr: Вспомогательный класс, управляющий набором строк, каждая из которых идентифицируется своей позицией
//   в пуле StringSet.
//   Прямое использование этого класса едва-ли уместно - следует применять как базовый класс для объектов,
//   которым необходимо компактное хранение строк, ассоциированых с элементами структур.
//
//   Так как этот класс будет использоваться в качестве базового для уже существующий классов, основные методы
//   имеют суффикс S для того, чтобы избежать путаницы с методами порожденных классов.
//
class SStrGroup {
public:
	SLAPI  SStrGroup();
	SStrGroup & FASTCALL operator = (const SStrGroup & rS);
	size_t SLAPI GetPoolDataLen() const;
	size_t SLAPI GetPoolSize() const;
	//
	// Descr: Очищает содержимое пула строк не освобождая память.
	//   Полезно применять при ожидаемом повторном использовании объекта.
	//
	void   SLAPI ClearS();
	//
	// Descr: То же, что и ClearS, но с освобождением памяти.
	//
	void   SLAPI DestroyS();
	SStrGroup & FASTCALL CopyS(const SStrGroup & rS);
	int    SLAPI AddS(const char * pStr, uint * pPos);
	int    SLAPI GetS(uint pos, SString & rStr) const;
	int    SLAPI SerializeS(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL WriteS(SBuffer & rBuf) const;
	int    FASTCALL ReadS(SBuffer & rBuf);
protected:
	//
	// Три метода для реализации упаковки пула строк (штатный метод Add не предполагает утилизацию
	// пространства, которое уже не используется).
	//
	// Пример использования:
	// {
	//	void * p_pack_handle = Pack_Start(); // Создаем дескриптор упаковки
	//	if(p_pack_handle) {
	//		for(uint i = 0; i < __count; i++) {
	//			p_item = __foo_get_item(i); // Получаем некоторую структуру, содержащую ссылку FooStrPos на строку в SStrGroup
	//			Pack_Replace(p_pack_handle, p_item->FooStrPos); // Замещаем ссылку FooStrPos на новое значение
	//			__foo_update_item(i, p_item); // Каким-то образом фиксируем факт изменения ссылки
	//		}
	//	}
	//	Pack_Finish(p_pack_handle); // Завершаем упаковку и разрушаем дескриптор. Крайне важный вызов - без него нарушится целостность ссылок.
	// }
	//
	void * SLAPI Pack_Start() const;
	int    SLAPI Pack_Finish(void * pHandle);
	int    SLAPI Pack_Replace(void * pHandle, uint & rPos) const;

	StringSet Pool;
};
//
// Descr: Класс, управляющий хранением ассоциаций {id, string}.
//   Все идентификаторы в массиве уникальны. Допускается хранение пустых строк.
//   Так как строки хранятся в общем пуле, размер каждой строки не ограничен,
//   а объем занимаемой памяти определяется только реальной длиной хранимых строк.
//   В этом основное отличие данного класса от массивов вроде TaggesStringArray,
//   где под каждую пару выделено фиксированное пространство.
//
class StrAssocArray : public SStrGroup {
public:
	friend int SLAPI Helper_CmpFunc_StrAssocByText(const LAssoc *, const LAssoc *, StrAssocArray *, LAssocArray *);
	friend int SLAPI Helper_CmpFunc_StrAssocByID(long, long, StrAssocArray *, LAssocArray *);

	struct Item {
		long   Id;
		long   ParentId;
		const  char * Txt;
	};
	SLAPI  StrAssocArray();
	SLAPI  StrAssocArray(const StrAssocArray & rS);
	StrAssocArray & FASTCALL operator = (const StrAssocArray & rS);
	int    FASTCALL Copy(const StrAssocArray & rS);
	void   FASTCALL SetTextCmpProc(CompFunc fcmp);
	//
	// Descr: Сравнивает экземпляр this с экземпляром src.
	//   Сравнение осуществляется по-элементно. Если this->getCount() != src.getCount(), то
	//   экземпляры не равны, в противном случае, каждый элемент this сравнивается с соотвествующим
	//   (согласно номеру позиции) элементом src. Если Id, ParentId и (побайтно) Txt равны соотвествующим
	//   полям Item экземпляра src, то элементы равны.
	// Returns:
	//   1 - массивы равны
	//   0 - массивы не равны
	//
	int    FASTCALL IsEqual(const StrAssocArray & src) const;
	uint   SLAPI getCount() const;
	uint   SLAPI getPointer() const { return Assoc.getPointer(); }
	uint   SLAPI setPointer(uint p) { return Assoc.setPointer(p); }
	uint   SLAPI incPointer() { return Assoc.incPointer(); }
	Item   FASTCALL at(uint pos) const;
	//
	// Descr: То же, что и StrAssocArray::at, только не пытается инициализировать
	//   родительский идентификатор. Благодаря этому функция выполняется значительно
	//   быстрее, чем предыдущая.
	//
	Item   FASTCALL at_WithoutParent(uint pos) const;

	int    SLAPI atFree(uint pos);
	int    SLAPI Search(long id, uint * pPos) const;
	//
	// Descr: Ищет в массиве элемент, текст которого совпадает с параметром pTxt.
	//   поиск не чувствителен к регистру.
	//   Если в массиве содержится более одного элемента, соответствующего
	//   критерию то найден будет только самый первый.
	// Returns:
	//   !0 - элемент найден
	//   0  - элемент не найден
	//
	int    SLAPI SearchByText(const char * pTxt, int ignoreCase, uint * pPos) const;
	int    SLAPI SearchByText(const char * pTxt, CompFunc fcmp, uint * pPos) const;
	int    SLAPI Get(long id, SString & rBuf) const;
	//
	// Descr: Находит максимальное значение идентификатора элемента в массиве.
	// Returns:
	//   >0 - в массиве есть как минимум один элемент. По указателю pId присвоено
	//        максимальное значение идентификатора.
	//   <0 - в массиве нет ни одного элемента. По указателю pId ничего не присваивается.
	//
	int    SLAPI GetMaxID(long * pId) const;
	int    SLAPI GetMaxLength(uint * pMaxLen) const;
	//
	// Descr: Добавляет в массив ассоциацию {id, pStr}. Если в массиве уже содержится элемент
	//   с идентификатором id, то, в зависимости от параметра replaceDup функция либо
	//   изменяет значение строки, ассоциированной с id, либо ничего не делает.
	// ARG(id         IN): Идентификатор элемента
	// ARG(pStr       IN): Строка, соответствующая идентификатору id
	// ARG(replaceDup IN): Определяет поведение функции, если в массиве найден элемент с идентификаторм id
	//   >0 - функция заменяет строку, соответствующую id на pStr.
	//    0 - функция ничего не делает
	//   <0 - функция добавляет новый элемент, заданный параметрами и дублируемым идентификатором.
	//        Использование этой опции сильно усложняет использование массива, поскольку исчезает
	//        однозначность соответствия {id, string}.
	// -100 - Специальная опция, аналогичная (replaceDup < 0) но при этом наличие в списке записи с
	//        идентификатором id просто не проверяется. Значительно ускоряет внесение новых элементов
	//        в большие списки.
	// Returns:
	//   1 - функция успешно добавила новый элемент в массив
	//   2 - функция изменила строку, соответствующую найденному элементу id (replaceDup != 0)
	//   3 - функция добавила новый элемент в условиях, когда идентификатор id уже содержится в списке
	//       (replaceDup < 0 && replaceDup != -100)
	//  -1 - элемент id уже содержится в массиве и функция ничего не сделала (replaceDup == 0)
	//   0 - ошибка (SLibError)
	//
	int    SLAPI Add(long id, const char * pStr, int replaceDup = 1);
	int    SLAPI Add(long id, long parentId, const char * pStr, int replaceDup = 1);
	int    SLAPI AddFast(long id, const char * pStr);
	int    SLAPI AddFast(long id, long parentId, const char * pStr);
	//
	// Descr: Изменяет значение идентификатора элемента в позиции pos на newId.
	// Attention: Функция опасная. Не следует использовать ее в тех случаях, когда
	//   массив содержит иерархические данные, а также не следует рассчитывать на то, что
	//   эта функция обеспечит уникальность идентификаторов.
	//
	int    SLAPI UpdateByPos(uint pos, long newId);
	int    SLAPI GetListByParent(long parentId, int recursive, LongArray &) const;
	int    FASTCALL HasChild(long id) const;
	int    SLAPI GetParent(long id, long * pParentId) const;
	//
	// Descr: Удаляет элемент с идентификатором id.
	//   Так как строки массива плотно упакованы в пул, при удалении элемента
	//   физически строка остается в пуле. Для освобождения памяти, занятой
	//   висячими строками используйте функцию StrAssocArray::Pack.
	// Returns:
	//   >0 - элемент успешно удален
	//   <0 - элемент и идентификатором id не найден
	//   0  - ошибка
	//
	int    SLAPI Remove(long id);
	int    SLAPI Swap(uint p1, uint p2);
	StrAssocArray & SLAPI Clear();
	//
	// Descr: Обнуляет все родительские ссылки элементов.
	//
	int    SLAPI ClearParents();
	int    SLAPI Pack();
	void   SLAPI SortByID();
	void   SLAPI SortByText();
	void   SLAPI SortByLength(int descend = 0);
	int    SLAPI Reverse();
	int    SLAPI RemoveRecursion(LongArray * pBadList);
	//
	// Descr: Узкоспециализированная функция, извлекающая позицию текстовой строки
	//   по индексу записи.
	// Note: В общем случае следует избегать использования этой функции. В частности потому,
	//   что позиции текста могут изменяться.
	//
	uint   FASTCALL GetTextPos(uint idx) const;
	int    SLAPI GetTextByPos(uint textPos, SString & rBuf) const;

	int    FASTCALL Write(SBuffer & rBuf, long o) const;
	int    FASTCALL Read(SBuffer & rBuf, long o);
#ifndef _WIN32_WCE // {
	int    SLAPI Write(SFile & rFile, long) const;
	int    SLAPI Read(SFile & rFile, long);
#endif // } _WIN32_WCE

	int    SLAPI Test_Cmp(const TaggedStringArray &, int * pNEqPos) const;
	static int SLAPI Helper_CmpFunc_StrAssocByText(const LAssoc * p1, const LAssoc * p2, StrAssocArray * pArray, LAssocArray * pRecurTracer); // @reallyprivate
protected:
	CompFunc TextCmpProc; // @v9.5.9
	LAssocArray Assoc;
	LAssocArray ParentList;
private:
	int    SLAPI Helper_RemoveRecursion(long parentId, LongArray & rRecurList, LongArray & rPassedList, LongArray * pBadList);
};
//
//
//
class StrStrAssocArray : public SStrGroup {
public:
	struct Item {
		const  char * Key;
		const  char * Val;
	};
	SLAPI  StrStrAssocArray();
	StrStrAssocArray & FASTCALL operator = (const StrStrAssocArray &);
	int    FASTCALL Copy(const StrStrAssocArray &);
	int    FASTCALL IsEqual(const StrStrAssocArray & src) const;
	uint   SLAPI getCount() const;
	Item   FASTCALL at(uint pos) const;
	int    SLAPI Search(const char * pKey, SString * pVal, uint * pPos) const;
	int    SLAPI Add(const char * pKey, const char * pVal, int replaceDup = 1);
	int    SLAPI Remove(const char * pKey);
	StrStrAssocArray & SLAPI Clear();
protected:
	LAssocArray L;
};
//
//
//
class SFileEntryPool : public SStrGroup {
public:
	friend DECL_CMPCFUNC(SFileEntryPool_Entry);

	struct Entry {
		uint   Attr;
		LDATETIME CreateTime;
		LDATETIME AccessTime;
		LDATETIME WriteTime;
		int64  Size;
		SString Name;
		SString Path;
	};
	SLAPI  SFileEntryPool();
	int    SLAPI Add(const char * pPath, SDirEntry & rDe);
    uint   SLAPI GetCount() const;
    int    SLAPI Get(uint pos, Entry & rEntry) const;

    enum {
    	scByName = 1,
    	scByExt,
    	scByFullName,
    	scByCrTime,
    	scByAccsTime,
    	scByWrTime,
    	scBySize,

    	scDesc = 0x8000 // Флаг обратного порядка сортировки
    };
    void   SLAPI Sort(int criteria);
private:
	struct InnerEntry {
		uint   Attr;
		LDATETIME CreateTime;
		LDATETIME AccessTime;
		LDATETIME WriteTime;
		int64  Size;
		uint   NameP;
		uint   PathP;
	};

	void   SLAPI ConvertEntry(const InnerEntry & rIEntry, Entry & rEntry) const;

    TSArray <InnerEntry> L;
    long   State;
};
//
// Descr: Список временных отрезков.
// Remark: Класс спроектирован с учетом того, что порожденные классы могут использовать
//   элементы списка, отличные от STimeChunk, но при этом, такие элементы должны в начале
//   обязательно содержать структуру STimeChunk.
//
class STimeChunkArray : public SArray {
public:
	SLAPI  STimeChunkArray();
	//
	// Descr: Добавляет в массив новый элемент. Если checkForFree != 0, то проверяет
	//   чтобы pItem не пересекался ни с одним из существующих элементов массива.
	// ARG(pItem        IN): @#{vptr} Ненулевой указатель на новый элемент. Для порожденных
	//   классов этот указатель в заголовке должен иметь структуру STimeChunk.
	//   Если pItem == 0, то возвращает 0 (@>>assert(pItem))
	// ARG(checkForFree IN): Если !0, то функция проверяет, чтобы новый элемент не пересекался //
	//   с другими элементами массива (@>>IsFreeEntry(pItem, 0))
	// Returns:
	//   >0 - новый элемент успешно добавлен
	//   <0 - новый элемент не добавлен поскольку пересекается с одним из существующих элементов
	//   0  - ошибка
	//
	int    SLAPI Add(const STimeChunk * pItem, int checkForFree);
	//
	// Descr: Вырезает из массива отрезок времени, соответствующий pItem.
	//   Суть вырезания заключается в том, что те элементы, которые пересекаются с pItem либо
	//   обрезаются, либо разбиваются на два, непересекающихся с pItem, отрезка.
	// ARG(pItem IN): @{vptr0} Указатель на отрезок, который следует вырезать. Если указатель нулевойЮ
	//   то функция ничего не делает.
	// Returns:
	//   >0 - функция произвела изменения в массиве
	//   <0 - функция ничего не изменила
	//
	int    SLAPI Excise(const STimeChunk * pItem);
	int    SLAPI Intersect(const STimeChunkArray * pList, STimeChunkArray * pResult) const;
	int    SLAPI Intersect(const STimeChunk & rChunk, STimeChunkArray * pResult) const;
	//
	// Descr: Возвращает порядок пересечения отрезков в массиве.
	//   Порядок пересечения представляет количество "этажей" в которое можно выстроить
	//   отрезки так, что бы в каждом "этаже" не было пересечения //
	//
	uint   SLAPI GetIntersectionOrder(LAssocArray * pOrderList, long tolerance = 0) const;
	//
	// Descr: Сортирует список
	//   Если desc == 0, то порядок сортировки - по возрастанию моменту начала элементов (PrcBusy::Start)
	//   в противном случае - по убыванию момента начала элементов
	//   Функция может быть использована в порожденных класса, элементы которых унаследованы от STimeChunk
	//
	void   SLAPI Sort(int desc = 0);
	//
	// Descr: Ограничивает все элементы массива периодом pLimit.
	//   Те элементы, которые выходят за пределы pLimit удаляются из массива,
	//   те, которые пересекаются с pLimit - ограничиваются пересечением.
	//   Функция может быть использована в порожденных класса, элементы которых унаследованы от STimeChunk
	// Returns:
	//   >0 - по крайней мере один из элементов массива был либо удален, либо изменен.
	//   <0 - все элементы массива удовлетворяют ограничению. Ничего не изменено.
	//
	int    SLAPI Limit(const STimeChunk * pLimit);
	//
	// Descr: Проверяет элемент rEntry на непересечение с элементами массива *this.
	//   Если rEntry пересекается с каким-либо элементов, то функция возвращает 0
	//   и по указателю pPos (если не нулевой) присваивает индекс этого элемента.
	//   Функция может быть использована в порожденных класса, элементы которых унаследованы от STimeChunk
	// Returns:
	//   !0 - отрезок rEntry не пересекается ни с одним элементом массива
	//   0  - отрезок rEntry пересекается по крайней мере с одним элементом массива.
	//        Индекс этого элемента присваивается по указателю pPos.
	//
	int    SLAPI IsFreeEntry(const STimeChunk & rEntry, uint * pPos) const;
	int    SLAPI IsFreeEntry(const LDATETIME &, long cont, uint * pPos) const;
		// @>>STimeChunkArray::IsFreeEntry(const STimeChunk &, uint *);
	//
	// Descr: Получает список доступных периодов.
	//   До вызова этой функции массив this должен быть отсортирован функцией STimeChunkArray::Sort.
	//
	int    SLAPI GetFreeList(STimeChunkArray * pList) const;
	//
	// Descr: Находит нижнюю и верхнюю границы списка. Если параметр update != 0, то
	//   устанавилвает нижнюю границу не меньше, чем pBounds->Start,
	//   а верхнюю не больше чем pBounds->Finish.
	//
	int    SLAPI GetBounds(STimeChunk * pBounds, int update) const;
protected:
	SLAPI  STimeChunkArray(size_t itemSize);
private:
	int    SLAPI Helper_Intersect(const STimeChunk & rChunk, STimeChunkArray & rResult) const;
};
//
// Descr: Элемент массива STimeChunkAssocArray
//   Представляет отрезок времени, ассоциированный с идентификатором и статусом.
//   Значение статуса STCHUNK_STATUS_HOLIDAY (-1000) имеет особый смысл. В приложении к персоналу или машинам
//   это значение соответсвует нерабочим дням (часам).
//   При использовании во временной диаграмме такие отрезки свободно пересекаются со всеми остальными
//   и выделяются цветом.
//
#define STCHUNK_STATUS_HOLIDAY -1000

struct STimeChunkAssoc {
	STimeChunk Chunk;
	long   Id;
	long   Status;
};
//
// Descr: Массив временных отрезков, ассоциированых с идентификатором и статусом.
//   Каждый такой массив сам по себе ассоциирован с идентификатором Id.
//
class STimeChunkAssocArray : public STimeChunkArray {
public:
	STimeChunkAssocArray(long id);
	int    Add(long chunkId, const STimeChunk * pChunk, int checkForFree);
	int    Add(long chunkId, long status, const STimeChunk * pChunk, int checkForFree);
	int    Remove(long chunkId);
	int    Get(long chunkId, uint * pPos, STimeChunkAssoc *) const;
	int    GetStatus(long chunkId, long * pStatus) const;

	long   Id;
};
//
//
//
enum {
	stcgnDataDestroyed = 1,
	stcgnDataModified = 2,
	stcgnTest = 1000
};

typedef int (* STimeChunkGridNotifyHandler)(int event);
//
// Descr: Структура данных для отображения временных диаграмм (STimeChunkBrowser)
//
class STimeChunkGrid : public TSCollection <STimeChunkAssocArray> {
public:
	STimeChunkGrid();
	~STimeChunkGrid();
	STimeChunkGrid & operator = (const STimeChunkGrid &);
	STimeChunkAssocArray * Get(long id, uint * pPos) const;
	int    GetChunk(long chunkId, long * pRowId, STimeChunkAssoc *) const;
	//
	// Descr: Устанавливает массив pList как строку решетки. Вместе со строкой может быть
	//   передан текст описания pText, который будет отображаться в левой части временной диаграммы.
	//   Массив pList передается в собственность объекта STimeChunkGrid. По этому, во-первых,
	//   массив должен быть создан динамически (new STimeChunkArray), а во-вторых, вызывающая //
	//   функция не должна его разрушать после передаче этой функции.
	//
	int    SetRow(STimeChunkAssocArray * pList, const char * pText);
	int    SetRowText(long rowID, const char * pText, int replace);
	//
	// Descr: Вставляет временной отрезок в структуру. Отрезок вставляется в первый
	//   элемент с идентификатором rowID. Если такой элемент не найден, то он создается.
	// Returns:
	//   1 - элемент успешно добавлен
	//   2 - элемент успешно добавлен (для этого пришлось создать новую строку)
	//   0 - ошибка
	//
	int    SetChunk(long rowID, long chunkID, long status, const STimeChunk * pChunk);
	int    RemoveRow(long rowID);
	int    RemoveChunk(long rowID, long chunkID);
	int    SetHolidayList(long rowID, const STimeChunkArray * pList);
	const  STimeChunkArray * FASTCALL GetHolidayList(long rowID) const;
	int    SetCollapseList(const STimeChunkArray * pList);
	const  STimeChunkArray * GetCollapseList() const;
	int    FASTCALL GetBounds(STimeChunk &) const;
	enum {
		iTitle = 0,  // Заголовок таблицы
		iRow,        // Строка
		iChunk,      // Отрезок
		iChunkBallon // Подсказка на MouseHover над отрезком
	};
	//
	// Descr: Функция должна возвратить текст, объекта типа item с идентификатором id.
	//   Для выяснения необходимости выделения зоны заголовков строк объект вызывает функцию
	//   с параметрами GetText(STimeChunkGrid::iRow, -1, temp_buf). Если функция вернула значение
	//   больше нуля, то зона выделяется.
	// Returns:
	//   -1 - функция в таком контексте не поддерживается //
	//   >0 - текст успешно возвращен
	//    0 - ошибка
	//
	virtual int GetText(int item, long id, SString & rBuf);
	//
	// Descr: Функция должна создать или изменить объект типа item с идентификатором *pID.
	//   Если item == iChunk || item == iRow и *pID == 0, то функция должна создать, соответственно,
	//   новую строку или новый отрезок. При этом, если item == iChunk то rowID является идентификатором
	//   текущей (выбранной) строки, а rTm - выбранным временем.
	// Returns:
	//   <0 - функция не поддерживается либо пользователь отказался от редактирования //
	//   >0 - пользователь изменил или создал объект.
	//    0 - ошибка
	//
	virtual int Edit(int item, long rowID, const LDATETIME & rTm, long * pID);
	//
	enum {
		mmCanMove = 0,
		mmCanResizeLeft  = 1,
		mmCanResizeRight = 2,
		mmCommit  = 100,
	};
	//
	// Descr: Функция должна отреагировать на перемещение (изменение размера) отрезка с
	//   идентификатором id.
	//   Для выяснения общей возможности перемещать отрезок объект таблицы вызывает
	//   эту функцию с идентификатором отрезка и его первоначальным значение. Если
	//   функция вернула значение <0, то объект даже не будет предлагать пользователю
	//   сервис перемещения.
	// Returns:
	//   <0 - функция не поддерживается либо таблице отказано в перемещении отрезка
	//   >0 - подтверждение перемещения отрезка - таблица может принять изменение.
	//    0 - ошибка
	//
	virtual int MoveChunk(int mode, long id, long rowId, const STimeChunk & rNewChunk);
	struct Color {
		long   Status;
		COLORREF C;
	};
	//
	// Descr: Функция должна вернуть цвет отрезка с идентификатором id.
	//   Цвет возвращается в виде структуры STimeChunkGrid::Color.
	//   Если цвет ассоциируется со статусом отрезка, то функция должна вместе
	//   с цветом вернуть значение статуса. Тогда в дальнейшем STimeChunkBrowser
	//   не будет обращаться к этой функции для отрезков, имеющих такой статус.
	//   Если цвет со статусом не сопоставлен, то в поле pClr->Status необходимо
	//   установить -1.
	// Returns:
	//   <0 - функция не поддерживается - следует использовать цвет по умолчанию.
	//   >0 - функция успешно выполнена и вернула актуальные данные по указателю pClr
	//    0 - ошибка - следует использовать цвет по умолчанию.
	//
	virtual int GetColor(long id, Color * pClr);
	int    SetNotifyHandler();
private:
	class HolidayArray : public STimeChunkArray {
	public:
		HolidayArray(long id)
		{
			Id = id;
		}
		long   Id;
	};
	TSCollection <HolidayArray> HL;
	STimeChunkArray * P_CollapseList;
	StrAssocArray NameList;
};
//
//
//
class HashTableBase {
public:
	struct Iter {
		uint   P;
		uint   E;
	};
	//
	// Descr: Структура статистики распределения элементов по таблице.
	//   Вычисляется функцией HashTableBase::CalcStat()
	//
	struct Stat {
		uint   NumEntries;
		uint   CountEmpty;
		uint   CountSingle;
		uint   CountMult;
		uint   CountItems;
		uint   Min;
		uint   Max;
		double Average;
		double StdDev;
	};
	//
	// Descr: Рассчитывает статистику распределения элементом по таблице.
	//   На основании такой статистики можно оценить эффективность используемой hash-функции.
	//
	int    CalcStat(Stat & rResult) const;
protected:
	struct Entry {
		int    FASTCALL Copy(const Entry & rSrc);
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
		int    SetVal(uint key, uint val);
		int    FASTCALL Remove(uint pos);
		LAssoc Val;
		uint16 Count; // Количество элементов в точке кэша (включая Val)
			// То есть, если Count == 0, то точка не содержит элементов.
		LAssoc * P_Ext;
	};

	HashTableBase(size_t sz);
	~HashTableBase();
	void   Clear();
	int    FASTCALL Copy(const HashTableBase & rSrc);
	int    InitTab();
	void   DestroyTabItems();
	int    FASTCALL InitIteration(Iter * pI) const;

	uint32 Size;
	Entry * P_Tab;
	//
	// Следующие компоненты являются общими для большинства порожденных
	// классов. Данным классом они почти не управляются - только в части инициализации.
	//
	enum {
		fUseAssoc = 0x0001
	};
	int32  Flags;
	int32  AddCount;  // Количество вставок
	int32  CollCount; // Количество коллизий при вставке
	int32  MaxTail;   // Максимальный хвост
	LAssocArray Assoc;
};
//
// Descr: строковая хэш-таблица. Реализует хранение ассоциаций строковых символов
//   с 4-байтовыми значениями.
//
class SymbHashTable : public HashTableBase { // @persistent
public:
	SymbHashTable(size_t sz, int useAssoc = 0);
	SymbHashTable & FASTCALL operator = (const SymbHashTable &);
	//
	// Descr: Копирует экземпляр rSrc в this.
	//
	int    FASTCALL Copy(const SymbHashTable & rSrc);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Descr: Удаляет все строки из хэш-таблицы. Размер таблицы не меняется.
	//
	void   Clear();
	//
	// Descr: Добавляет новый символ pSymb с идентификатором val в таблицу.
	//
	int    Add(const char * pSymb, uint val, uint * pPos);
	int    Add(const char * pSymb, uint val);
	int    Del(const char * pSymb, uint * pVal);
	int    Get(uint pos, SString & rBuf) const;
	int    GetByAssoc(uint val, SString & rBuf) const;
	//
	// Descr: Ищет символ pSymb в таблице.
	// Returns:
	//   >0 - символ найден (по указателю pVal присвоено значение ассоциированного значения).
	//   0  - символ не найден (значение по указатель pVal не меняется)
	//
	int    Search(const char * pSymb, uint * pVal, uint * pPos) const;
	int    FASTCALL InitIteration(Iter * pI) const;
	int    NextIteration(Iter *, uint * pVal, uint * pPos, SString * pStr) const;
	//
	// Descr: Возвращает максимальное значение идентификатора Val из всех,
	//   хранящихся в таблице.
	//
	uint   GetMaxVal() const;
	//
	// Descr: Удаляет ассоцииации идентификаторов символов с позициями.
	//   После вызова функции хэш теряет статус useAssoc.
	//
	int    ResetAssoc();
	//
	// Descr: Перестраивает ассоцииации идентификаторов символов с позициями.
	//   После вызова функции хэш получает статус useAssoc.
	// Note: Функция может быть полезна в случае, если необходимо создать большой
	//   хэш без задержек на установку ассоциаций. При необходимости же использовать
	//   в дальнейшем ассоциации можно вызвать эту функцию.
	//
	int    BuildAssoc();

	int    Test_Cmp(const SymbHashTable & rPat) const;
private:
	size_t FASTCALL Hash(const char * pSymb) const;

	StringSet NamePool;
};
//
//
//
class GuidHashTable : public HashTableBase  { // @persistent
public:
	GuidHashTable(size_t sz, int useAssoc = 0);
	GuidHashTable & FASTCALL operator = (const GuidHashTable &);
	//
	// Descr: Копирует экземпляр rSrc в this.
	//
	int    FASTCALL Copy(const GuidHashTable & rSrc);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Descr: Удаляет все элементы из хэш-таблицы. Размер таблицы не меняется.
	//
	void   Clear();
	//
	// Descr: Добавляет новый GUID rUuid с идентификатором val в таблицу.
	//
	int    Add(const S_GUID & rUuid, uint val, uint * pPos);
	int    Del(const S_GUID & rUuid, uint * pVal);
	int    Get(uint pos, S_GUID & rUuid) const;
	int    GetByAssoc(uint val, S_GUID & rBuf) const;
	//
	// Descr: Ищет символ pSymb в таблице.
	// Returns:
	//   >0 - символ найден (по указателю pVal присвоено значение ассоциированного значения).
	//   0  - символ не найден (значение по указатель pVal не меняется)
	//
	int    Search(const S_GUID & rUuid, uint * pVal, uint * pPos) const;
	int    FASTCALL InitIteration(Iter * pI) const;
	int    NextIteration(Iter *, uint * pVal, S_GUID & rUuid) const;
	//
	// Descr: Возвращает максимальное значение идентификатора Val из всех,
	//   хранящихся в таблице.
	//
	uint   GetMaxVal() const;
	int    Test_Cmp(const GuidHashTable & rPat) const;
private:
	size_t FASTCALL Hash(const S_GUID & rUuid) const;
	UuidArray Pool;
};
//
//
//
class PtrHashTable : public HashTableBase  {
public:
	PtrHashTable(size_t sz, int useAssoc = 0);
	PtrHashTable & FASTCALL operator = (const PtrHashTable &);
	//
	// Descr: Копирует экземпляр rSrc в this.
	//
	int    FASTCALL Copy(const PtrHashTable & rSrc);
	//
	// Descr: Удаляет все элементы из хэш-таблицы. Размер таблицы не меняется.
	//
	void   Clear();
	//
	// Descr: Добавляет новый указатель с идентификатором val в таблицу.
	//
	int    Add(void * ptr, uint val, uint * pPos);
	int    Del(void * ptr, uint * pVal);
	void * FASTCALL Get(uint pos) const;
	void * FASTCALL GetByAssoc(uint val) const;
	//
	// Descr: Ищет символ pSymb в таблице.
	// Returns:
	//   >0 - символ найден (по указателю pVal присвоено значение ассоциированного значения).
	//   0  - символ не найден (значение по указатель pVal не меняется)
	//
	int    Search(const void * ptr, uint * pVal, uint * pPos) const;
	int    FASTCALL InitIteration(Iter * pI) const;
	int    NextIteration(Iter *, uint * pVal, void ** ppPtr) const;
	//
	// Descr: Возвращает максимальное значение идентификатора Val из всех, хранящихся в таблице.
	//
	uint   GetMaxVal() const;
private:
	size_t FASTCALL Hash(const void * ptr) const;

	TSArray <void *> Pool;
};
//
// Descr: хэш 32-битных значений. Все пространство значений разбито на блоки по 32 единице.
//   Каждый блок представлен первым номером (0, 32, 64...) и 32-битным битовым массивом,
//   каждый бит которого обозначает присутствие в хэше искомой величины.
//   За счет такой организации становится возможным хранить слабо разреженный список очень большого
//   количества величин с небольшим расходом памяти.
//
class UintHashTable {
public:
	UintHashTable();
	UintHashTable & FASTCALL operator = (const UintHashTable &);
	int    FASTCALL Copy(const UintHashTable *);
	int    FASTCALL Add(ulong);
	int    FASTCALL Add(const UintHashTable & rS);
	int    FASTCALL Intersect(const UintHashTable & rS);
	int    FASTCALL Remove(ulong);
	int    FASTCALL Has(ulong) const;
	//
	// Descr: Подсчитывает количество элементов в хэше.
	// Perf: Так как количество именно "подсчитывается", то есть пробегаются все элементы
	//   хэша, производительность функции очень низкая.
	//   Не следует использовать в циклах.
	//
	uint   GetCount() const;
	int    FASTCALL Enum(ulong * pID) const;
	void   Clear();
	void   Destroy();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
private:
	void * GetBlock(ulong val, int cr);
	SArray List;
};
//
// BitArray
//
class BitArray : private SBaseBuffer {
public:
	SLAPI  BitArray();
	SLAPI  BitArray(const BitArray &);
	SLAPI ~BitArray();
	BitArray & FASTCALL operator = (const BitArray &);
	int    FASTCALL Copy(const BitArray &);
	int    SLAPI Init(const void *, size_t count);
	//
	// Descr: Обнуляет внутренний буфер и сбрасывает счетчик битов в 0
	//
	void   SLAPI Clear();
	size_t SLAPI getCount() const;
	int    FASTCALL IsEqual(const BitArray &) const;
	//
	// Descr: Возвращает количество бит, соответствующих величине val.
	//   Т.е. если val == 0, то возвращает количество нулевых бит,
	//   в противном случае - количество установленных.
	//
	size_t FASTCALL getCountVal(int val) const;
	int    FASTCALL atInsert(size_t pos, int val);
	int    FASTCALL insert(int val);
	int    FASTCALL insertN(int val, size_t N);
	int    FASTCALL atFree(size_t pos);
	size_t FASTCALL findFirst(int val, size_t start) const;
	int    FASTCALL get(size_t pos) const;
	uint32 FASTCALL getN(size_t pos, uint count) const;
	int    FASTCALL set(size_t pos, int val);
	int    FASTCALL operator [](size_t) const;
	//
	// Descr: возвращает размер буфера в байтах
	//
	size_t SLAPI getBufSize() const;
	int    SLAPI getBuf(void *, size_t maxLen) const;
private:
	size_t Count;
};
//
//
//
class STree : private SArray {
public:
	class Iter {
	public:
		friend class STree;

		Iter();
		Iter(uint32 parentP);
		void * GetData() const;
		uint32 GetCurrentPos() const;
		uint32 GetParentPos() const;
	private:
		enum {
			fFirstIter  = 0x0001,
			fOnlyParent = 0x0002,
			fDepth      = 0x0004  // Перечисление элементов "В глубину". Иначе - "В ширину"
		};
		uint32 ParentP;
		uint32 CurrP;
		uint32 CurrDataP;
		uint32 NextP;
		TSStack <uint32> Stack;

		long   Flags;
		void * P_Data;
	};
	STree(uint itemSize, int o = O_ARRAY);
	~STree();
	STree & Clear();
	int    Insert(const void * pData, uint32 parentP, uint32 * pSvcPos);
	int    InsertBefore(const void * pData, uint32 prevP, uint32 * pSvcPos);
	int    Delete(uint32 svcPos);
	int    Sort(CompFunc fcmp, long extraData = 0);
	uint32 FASTCALL GetFirstChildP(uint32 itemP) const;
	int    HasChildren(uint32 itemP) const;
	int    Search(const void * pKey, uint32 * pIdx, CompFunc fcmp, size_t ofs = 0, void * pExtraData = 0) const;
	int    Search(const void * pKey, Iter & rIter, CompFunc fcmp, size_t ofs = 0, void * pExtraData = 0) const;
	int    FASTCALL Enum(Iter & rIter) const;
	//
	// Descr: Возвращает указатель на блок данных, соответствующих позиции узла pos.
	//
	void * FASTCALL GetData(uint32 pos) const;
private:
	struct Item_ {
		uint32 DataP;       // Этот компонент находится в начале структуры для быстрого поиска по значению
		uint32 ParentP;
		uint32 NextP;
		uint32 PrevP;
		uint32 FirstChildP;
	};
	Item_ * FASTCALL GetNext_(const Item_ * pItem) const;
	int    CreateNode(uint32 parentP, uint32 prevP, uint32 dataP, uint32 * pP);
	int    CreateDataEntry(const void * pData, uint32 * pPos);
	int    FASTCALL SetupIter(Iter & rIter, uint32 currP) const;
	int    SearchDataP(uint32 dataP, uint32 * pPos) const;

	UintHashTable SvcFreeList;
	UintHashTable DatFreeList;
	TSArray <Item_> TreeList;
};

template <class T> class TSTree : public STree {
public:
	TSTree() : STree(sizeof(T))
	{
	}
	int    Insert(const T & rS, uint32 parentPos, uint32 * pSvcPos)
	{
		return STree::Insert(&rS, parentPos, pSvcPos);
	}
	int    InsertBefore(const T & rS, uint32 prevP, uint32 * pSvcPos)
	{
		return InsertBefore(&rS, prevP, pSvcPos);
	}
	T * FASTCALL Get(uint pos) const
	{
		return (T *)STree::GetData(pos);
	}
};
//
// Descr: Простой вектор, содержащий элементы uint32.
//   В отличии от SArray крайне примитивен и занимает очень мало памяти.
//   Так как не содержит конструктора и деструктора, может быть использован в union'ах.
//
class SV_Uint32 {
public:
	// @noconstructor
	// @nodestructor
	int    Init();
	void   Destroy();
	int    FASTCALL Copy(const SV_Uint32 &);
	int    FASTCALL Add(uint32);
	int    Add(uint count, const uint32 * pSrc);
	int    FASTCALL IsEqual(const SV_Uint32 & rPat) const;
	int    Search(uint val, uint * pPos) const;
	uint   GetCount() const;
	uint32 FASTCALL Get(uint idx) const;
	uint32 operator [] (size_t idx) const { return Get((uint)idx); }
	int    FASTCALL Write(SBuffer & rBuf) const;
	int    FASTCALL Read(SBuffer & rBuf);

	uint32 * P_Data;
};
//
// Descr: Класс для разбора сложных текстов.
//   Для простых случаев следует использовать SString::Tokenize и SStrScan
//   Принцип использования:
//   {
//      STokenizer::Param tp;
//      // ... инициалазация параметров токенайзера
//      STokenizer tknz;
//      tknz.Write(0, text, text_len); // Внесение данных в токенайзер для процессинга
//      tknz.Run();
//      STokenizer::Item ti;
//      for(uint i = 0; i < tknz.GetCount(); i++) {
//          tknz.Get(i, ti);
//          // ... обработка токена ti
//      }
//   }
//
class STokenizer {
public:
	//
	// Descr: Контекст разбора строки. Определяет специализированные правила разбиения строк и
	//   идентификации токенов.
	//
	class Context {
	public:
		Context()
		{
		}
		virtual ~Context()
		{
		}
	};
	enum {
		tokNull = 0,
		tokDelim,          // Если соответствующий текст пустой, то разделитель неявный (например цифра-буква)
		tokWord,

		tokFirstDyn = 1000 // Первый токен, определяемый вызывающим модулем.
	};
	//
	// Descr: Флаги параметров токенайзера.
	//
	enum {
		fRetDelim    = 0x0001, // Возвращать разделители при вызовах Next()
		fEachDelim   = 0x0002, // Каждый отдельный разделитель возвращать отдельным токеном
		fDivAlNum    = 0x0008  // Если после цифры идет нецифровой (и не разделительный) символ или
			// после нецифрового (не разделительного) символа идет цифра, то считать этот факт неявным разделителем.
	};
	enum {
		coClearSymbTab = 0x0001 //
	};
	struct Param {
		Param()
		{
			Flags = 0;
			Cp = 0;
		}
		long   Flags;
		int    Cp;
		SString Delim; // Список символов-разделителей. Если строка пустая, то используется " \t\n\r"
	};
	struct Item {
		int    Token;
		uint   TextId;    // Фактически, позиция текста во внутреннем буфере this. Может быть использовано как идентификатор текста в течении жизни this->T
		int64  OrgOffs;
		SString Resource;
		SString Text;
	};
	SLAPI  STokenizer(const Param * pParam);
	SLAPI ~STokenizer();
	void   SLAPI GetParam(Param * pParam) const;
	int    SLAPI SetParam(const Param * pParam);
	STokenizer & Reset(long options);
	//
	// Descr: Очищает внутренний буфер ввода.
	//
	void   SLAPI ClearInput();
	//
	// Descr: Вносит новую порцию данных для обработки.
	// ARG(pResource IN): Текстовый идентификатор источника данных. Вызывающий модуль самостоятельно определяет
	//   целесообразность использования и формат идентификатора. Сам же следит за уникальностью.
	// ARG(orgOffs   IN): Смещение до передаваемой порции данных в оригинальном источнике данных.
	// ARG(pS        IN): Собственно данные для обработки
	// ARG(sz        IN): Размер передаваемых данных в байтах
	//
	int    SLAPI Write(const char * pResource, int64 orgOffs, const void * pS, size_t sz);
	//
	// Descr: Запускает разбор входного потока, начиная с очередного символа входного буфера.
	// ARG(pIdxFirst OUT): Индекс первого элемента результата разбора. Получить результат можно функцией Get(uint, Item &)
	// ARG(pIdxCount OUT): Количество элементов результатов разбора доступного в буфере.
	//
	int    SLAPI Run(uint * pIdxFirst, uint * pIdxCount);
	//
	// Descr: Высокоуровневая функция, реализующая пару вызовов:
	//   Write(const char * pResource, int64 orgOffs, rS, rS.Len()+1);
	//   Run(pIdxFirst, pIdxCount);
	//
	int    SLAPI RunSString(const char * pResource, int64 orgOffs, const SString & rS, uint * pIdxFirst, uint * pIdxCount);
	uint   SLAPI GetCount() const;
	int    SLAPI Get(uint idx, Item & rItem) const;
	int    SLAPI Get_WithoutText(uint idx, STokenizer::Item & rItem) const;
	uint   SLAPI GetCommCount() const;
	int    SLAPI GetComm(uint idx, Item & rItem) const;
	int    SLAPI GetTextById(uint txtId, SString & rBuf) const;
	int    SLAPI GetSymbHashStat(SymbHashTable::Stat & rStat) const;
	//
	// Descr: Флаги функции STokenizer::Search
	//
	enum {
		sfFirstInTextOnly  = 0x0001, // Принимать во внимание только те ресурсы, для которых есть вхождение одного из искомых токенов первым.
			// "abc 100 xyz" <-- "100 abc" yes
			// "abc 100 xyz" <-- "xyz 100" no
		sfAllInPatternOnly = 0x0002  // Принимать во внимание только те ресурсы, которые содержат все токены из паттерна
			// "abc 100 xyz" <-- "xyz 100" yes
			// "abc 100 xyz" <-- "100 abc gogo" no
	};
	struct ResourceToken {
		uint32 RP;
		uint32 PosInRText;
	};
	struct SearchBlockEntry {
		int    T; // Идент токена
		uint   P; // Позиция токена в исходном тексте
		TSArray <STokenizer::ResourceToken> RL; // Список идентификаторов ресурсов, в которых встречается токен T
		//LongArray ResourceIdList;
	};
	int    SLAPI Search(long flags, TSCollection <STokenizer::SearchBlockEntry> & rResult);
	int    SLAPI IndexResources(int force);
private:
	int16  SLAPI NextChr();
	int    FASTCALL IsDelim(int16 chr) const;
	int    FASTCALL AddToken(TSArray <int16> & rBuf, int tok);
	int    SLAPI ProcessSearchToken(TSArray <int16> & rBuf, int tokType, TSCollection <STokenizer::SearchBlockEntry> & rResult);

	struct Token {
		int    T;  //
		uint   PP; // Позиция токена в STokenizer::T
		uint   RP; // Позиция идентификатора ресурса в STokenizer::T
		int64  SP; // Позиция токена в исходном буфере
	};
	//
	//
	//
	struct CToken {
		int    T;
		uint   PP;
	};
	//
	// Descr: Элемент индекса, сопоставляющего позицию токена списку ресурсов, содержащих токен
	//
	struct ResourceIndexItem {
		int    PP;
		TSArray <STokenizer::ResourceToken> RL;
	};

	Param  P;
	uint   Tc;  // Счетчик для формирования уникального значения, ассоциированного с токенами, добавляемыми в T
	uint   RP;  // Позиция текущего идентификатора ресурса в T
	int64  SO;  // Смещение содержимого S в источнике данных: необходимо для сопоставления токена с источником данных
	SBuffer S;  // Буфер входящего потока
	SymbHashTable T;
	TSArray <Token> L;
	TSArray <CToken> CL;
	TSCollection <ResourceIndexItem> * P_ResourceIndex;
	//
	SString AddTokenBuf;      // Для ускорения AddToken() (не придется при каждом вызове распределять память)
	TSArray <int16> TokenBuf; // Для ускорения Run() (чтобы избежать лишних распределений памяти)
};
//
#ifndef _WIN32_WCE // {
//
//
//
class SSystem {
public:
	enum CpuVendor {
		cpuvUnkn = 0,
		cpuvIntel,
		cpuvAMD,
		cpuvVIA,
		cpuvCyrix,
		cpuvNexGen
	};
	enum CpuCmdSet {
		cpucsUnkn      =  0, // 80386 instruction set only
		cpucsMMX       =  1, // or above = MMX instructions supported
		cpucsFCOMI     =  2, // or above = conditional move and FCOMI supported
		cpucsXMM       =  3, // or above = SSE (XMM) supported by processor and operating system
		cpucsSSE2      =  4, // or above = SSE2 supported
		cpucsSSE3      =  5, // or above = SSE3 supported
		cpucsSupplSSE3 =  6, // or above = Supplementary SSE3
		cpucsSSE41     =  8, // or above = SSE4.1 supported
		cpucsPOPCNT    =  9, // or above = POPCNT supported
		cpucsSSE42     = 10, // or above = SSE4.2 supported
		cpucsAVX       = 11, // or above = AVX supported by processor and operating system
		cpucsPCLMUL    = 12, // or above = PCLMUL and AES supported
		cpucsAVX2      = 13  // or above = AVX2 supported
	};
	enum {
		fBigEndian = 0x0001 // Процессор имеет архитектуру BIG ENDIAN (на текущий момент флаг всегда сброшен)
	};

	static int BigEndian();

	SSystem(int imm);
	int    FASTCALL GetCpuInfo();
	int    CpuId(int feature, uint32 * pA, uint32 * pB, uint32 * pC, uint32 * pD) const;

	CpuVendor CpuV;
	CpuCmdSet CpuCs;
	size_t CpuCacheSizeL0;
	size_t CpuCacheSizeL1;
	size_t CpuCacheSizeL2;
	uint32 CpuA;
	uint32 CpuB;
	uint32 CpuC;
	uint32 CpuD;
	uint32 Flags;
};
//
// Descr: Класс реализующий абстрактные методы для доступа к средствам ввода/вывода операционной системы
//
class SIo {
public:
	enum {
		tDefault = 0,
		tFile,
		tPipe,
		tSerial
	};

	enum {
		crmCrAllways      = (1 << 6),
		crmCrNew          = (2 << 6),
		crmOpenAllways    = (3 << 6),
		crmOpenExist      = (4 << 6),
		crmTrancateExist  = (5 << 6)
	};

	enum {
		mAccsRead         = 0x00000001,
		mAccsWrite        = 0x00000002,
		mAccsExec         = 0x00000004,
		mShrRead          = 0x00000008,
		mShrWrite         = 0x00000010,
		mShrDelete        = 0x00000020,

		mCreationMask     = 0x00000040|0x00000080|0x00000100,

		mAttrArch         = 0x00000200,
		mAttrEncrypt      = 0x00000400,
		mAttrHidden       = 0x00000800,
		mAttrNormal       = 0x00001000,
		mAttrNoIndex      = 0x00002000,
		mAttrOffline      = 0x00004000,
		mAttrRO           = 0x00008000,
		mAttrSys          = 0x00010000,
		mAttrTemp         = 0x00020000,

		mFBackupSemantics = 0x00040000,
		mFPosixSemantics  = 0x00080000,
		mFDelOnClose      = 0x00100000,
		mFNoBuff          = 0x00200000,
		mFOpenNoRecall    = 0x00400000,
		mFOpenReparsePt   = 0x00800000,
		mFOverlapped      = 0x01000000,
		mFRandomAccs      = 0x02000000,
		mFSeqScan         = 0x04000000,
		mFWriteThrough    = 0x08000000
	};
	enum {
		mSecAnonym        = 0x00000001,
		mSecCtxTracking   = 0x00000002,
		mSecDelegation    = 0x00000004,
		mSecEffOnly       = 0x00000008,
		mSecIdent         = 0x00000010,
		mSecImpersonation = 0x00000020
	};

	static void * SCreateFile(int ioType, const char * pName, uint32 mode, uint32 secMode = 0);
};
//
//
//
struct SFileSortProgressData {
	SFileSortProgressData()
	{
		THISZERO();
	}
	const  char * P_SrcFileName;
	int64  TotalFileSize;
	int    Phase; // 1 - splitting, 2 - merging
	int64  SplitBytesRead; // Количество байт считанных на фазе сплиттинга
	int    SplitThreadCount; // Количество потоков, осуществляющих сплиттинг
	uint   MergeChunkCount; // Начальное количество блоков для слияния
	uint   MergeChunkLeft;  // Оставшее число не слитых блоков
	const void * ExtraPtr;
};

typedef int (*SFileSortProgressCbProc)(const SFileSortProgressData *);

class SFile {
public:
	DECL_INVARIANT_C();

	static int SLAPI Remove(const char * pFileName);
	static int SLAPI Rename(const char * pFileName, const char * pNewFileName);
	static int SLAPI Compare(const char * pFileName1, const char * pFileName2, long flags);
	//
	// Descr: Структура дополнительных параметров сортировки файла
	//
	struct SortParam {
		SortParam()
		{
			MaxChunkSize =(8*1024*1024);
			MaxChunkCount = 8;
			MaxThread = 0;
			ProgressCbProc = 0;
			ProgressCbExtraPtr = 0;
		}
        size_t MaxChunkSize;
        uint   MaxChunkCount;
		uint   MaxThread;
        SFileSortProgressCbProc ProgressCbProc;
		const void * ProgressCbExtraPtr;
	};

	static int SLAPI Sort(const char * pSrcFileName, const char * pOutFileName, CompFunc fcmp, SortParam * pExtraParam);
	//
	// Descr: возвращает меточные даты файла с именем pFileName
	// ARG(pFileName    IN): полное имя файла
	// ARG(pCreation   OUT): дата и время создания файла
	// ARG(pLastAccess OUT): дата и время последнего обращения к файлу
	// ARG(pLastModif  OUT): дата и время последней модификации файла
	// Returns:
	//   1 - функция успешно выполнена
	//   0 - ошибка
	//
	static int SLAPI GetTime(const char * pFileName, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif);
	//
	// Descr: возвращает меточные даты файла, открытого с дескриптором fd
	// ARG(fh           IN): дескриптор файла. В WIN32 этот дескриптор должен быть получен вызовом
	//   функции CreateFile. В DOS - как fileno(FILE *).
	// ARG(pCreation   OUT): дата и время создания файла
	// ARG(pLastAccess OUT): дата и время последнего обращения к файлу
	// ARG(pLastModif  OUT): дата и время последней модификации файла
	// Returns:
	//   1 - функция успешно выполнена
	//   0 - ошибка
	//
	static int SLAPI GetTime(int fh, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif);
	static int SLAPI SetTime(int fh, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif);
	static int SLAPI IdentifyFormat(const char * pFileName);
	static int SLAPI IsOpenedForWrite(const char * pFileName);
	//
	// Descr: Ожидает момента, когда файл не открыт для записи.
	// Returns:
	//   <0 - файл не открыт для записи
	//   >0 - время в миллисекундах, в течении которого пришлось ждать освобождения файла
	//    0 - истек предельный timeout ожидания //
	//
	static int SLAPI WaitForWriteSharingRelease(const char * pFileName, long timeout);
	//
	// Descr: Утилитная функция, вызывающая fclose(*ppF) если *ppF != 0 и обнуляющая после этого *ppF.
	//
	static void FASTCALL ZClose(FILE ** ppF);

	enum {
		mRead = 1,       // "r"
		mWrite,          // "w"
		mAppend,         // "a"
		mReadWrite,      // "r+"
		mReadWriteTrunc, // "w+"
		mAppendRead,     // "a+"

		mBinary    = 0x0100, // "..b"
		mDenyRead  = 0x0200,
		mDenyWrite = 0x0400,

		mNoStd     = 0x0800, // Файл открывается в низкоуровневом (небуферизованном) режиме (open).
			// Если размер файла может превышать 2G, то обязательно использовать этот флаг.
		mNullWrite = 0x1000, // @v8.2.5 Файл открывается в режиме нулевого вывода - то есть запись в него ничего не делает,
			// а чтение возвращает ошибку
		mBuffRd    = 0x2000  // @v9.5.9 При установленном mNoStd чтение из файла буферизуется.
	};
	SLAPI  SFile();
	SLAPI  SFile(const char * pName, long mode);
	SLAPI  SFile(SBuffer & rBuf, long mode);
	SLAPI ~SFile();
	int    SLAPI IsValid() const;
	SLAPI  operator FILE * ();
	SLAPI  operator SBuffer * ();
	//
	// Descr: Если экземпляр класса работает как SBuffer (T == tSBuffer), то данное преобразование
	//   присвоит буферу rBuf указатель на область памяти буфера и доступный размер (SBuffer::GetAvailableSize()).
	//   В противном случае rBuf.P_Buf = 0 и rBuf.Size = 0.
	// Note:
	//   Пользоваться функцией следует с осторожностью так как память по указателю rBuf.P_Buf
	//   принадлежит и данному экземпляру класса.
	//   Ни в коем случае нельзя вызывать rBuf.Destroy() для переданной ссылке после вызова данной функции.
	// Returns:
	//   1 - экземпляр использует SBuffer и переданный объект rBuf успешно инициализирован.
	//   0 - экземпляр либо не инициализирован, либо связан с файлом.
	//
	int    SLAPI GetBuffer(SBaseBuffer & rBuf) const;
	int    SLAPI FileNo() const;
	const  SString & SLAPI GetName() const;
	long   SLAPI GetMode() const;
	int    SLAPI Open(const char * pName, long mode);
	int    SLAPI Open(SBuffer & rBuf, long mode);
	int    SLAPI OpenNullOutput();
	int    SLAPI Close();
	//
	// Descr: Устанавливает 32-битное значение текущей позиции в файле.
	// Note: При буферизованном чтении вызов функции сбрасывает буфер.
	//
	int    SLAPI Seek(long offs, int origin = SEEK_SET);
	//
	// Descr: Устанавливает 64-битное значение текущей позиции в файле.
	// Attention: Если файл открыт в стандартном режиме (без флага fNoStd) то
	//   фактически применяется 32-битное значение позиции (младшие четыре байта offs).
	// Note: При буферизованном чтении вызов функции сбрасывает буфер.
	//
	int    SLAPI Seek64(int64 offs, int origin = SEEK_SET);
	//
	// Descr: Возвращает значение текущей позиции в файле.
	//
	long   SLAPI Tell();
	//
	// Descr: Возвращает 64-битное значение текущей позиции в файле.
	// Attention: Если файл открыт в стандартном режиме (без флага fNoStd) то
	//   фактически возвращается 32-битное значение позиции.
	//
	int64  SLAPI Tell64();
	int    SLAPI Flush();
	int    SLAPI Write(const void * pBuf, size_t size);
	//
	// Descr: Пытается считать size байт в буфер pBuf.
	//   Действительное количество считанных байт присваивается по адресу pActualSize
	//   (если pActualSize != 0).
	//
	int    SLAPI Read(void * pBuf, size_t size, size_t * pActualSize = 0);
	//
	// Descr: То же, что и SFile::Read() но с проверкой равенства запрошенного
	//   размера считанному. Если считанное количество байт не равно запрошенному,
	//   то возвращает 0 и устанавливает код ошибки SLERR_SFILRDSIZE.
	//
	int    SLAPI ReadV(void * pBuf, size_t size);
	int    SLAPI WriteLine(const char *);
	int    SLAPI ReadLine(SString &);
	//
	// Descr: Высокоуровневая функция записи буфера rBuf в поток.
	//   Сначала записывает размер буфера, затем - данные.
	// Returns:
	//   >0 - буфер записан успешно
	//   0  - ошибка
	//
	int    SLAPI Write(const SBuffer & rBuf);
	//
	// Descr: Высокоуровневая функция чтения буфера rBuf из потока.
	//   Сначала считывает размер буфера, распределяет необходимый размер в буфере и считывает данные.
	// Returns:
	//   >0 - буфер считан успешно
	//   0  - ошибка
	//
	int    SLAPI Read(SBuffer & rBuf);
	//
	// Descr: Устанавливает либо снимает блокировку участка файла размером size со смещением offs.
	//   При успешном выполнении функция возвращает дескриптор блокировки (>0) который
	//   может быть использован при снятии блокировки функцией Unlock().
	//   При разрушении экземпляра класса все блокировки автоматически снимаются.
	//
	//   При снятии блокировки функция проверяет, чтобы во внутренней таблице блокировок присутствовал заданный
	//   участок и, если это так, то разблокирует участок и возвращает его дескриптор (уже недействительный) вызывающей
	//   функции. В противном случае завершается с ошибкой (return 0).
	//
	// ARG(offs IN): @#{>0} смещение в файле, по которому должна быть установлена блокировка
	// ARG(size IN): @#{>0} размер блокируемого участка в файле
	// ARG(mode IN): если mode != 0, то функция устанавливает блокировку, в противном случае - снимает.
	// Returns:
	//   0 - ошибка
	//   >0 - при mode != 0 дескриптор блокировки. При mode == 0 просто означает успешное завершение операции.
	//
	int    SLAPI _Lock(int64 offs, int32 size, int mode);
	//
	// Descr: Блокирует участок файла размером size со смещением offs.
	//   При успешном выполнении функция возвращает дескриптор блокировки (>0) который
	//   должен быть использован при снятии блокировки функцией Unlock().
	//   При разрушении экземпляра класса все блокировки автоматически снимаются.
	// ARG(offs IN): @#{>0} смещение в файле, по которому должна быть установлена блокировка
	// ARG(size IN): @#{>0} размер блокируемого участка в файле
	// Returns:
	//   0  - ошибка
	//   >0 - значение дескриптора блокировки
	//
	int    SLAPI Lock(int64 offs, int32 size);
	//
	// Descr: Снимает блокировку, установленную перед 'тим функцией Lock(int64, int)
	//
	int    SLAPI Unlock(int lckHandle);
	//
	// Descr: Рассчитывает размер файла. Размер определяется перемещением текущей позиции в конец файла
	//   и определением после этого текущей позиции, которая и является размером.
	//   После подсчета восстанавливает исходную текущую позицию файла.
	// Returns:
	//   !0 - размер файла успешно вычислен
	//   0  - ошибка
	//
	int    SLAPI CalcSize(int64 * pSize);
	int    SLAPI GetDateTime(LDATETIME * pCreate, LDATETIME * pLastAccess, LDATETIME * pModif);
	//
	// Descr: Рассчитывает CRC32 файла, начиная со смещения offs.
	//   После подсчета восстанавливает исходную текущую позицию файла.
	// Returns:
	//   >0 - CRC32 успешно рассчитан
	//   <0 - размер файла меньше offs
	//   0  - ошибка
	//
	int    SLAPI CalcCRC(long offs, uint32 * pCrc);
private:
	struct LckChunk {
		int32  Size;
		int64  Offs;
	};
	int    AcquireLckDescriptor(int64 offs, int32 size);
	const  SFile::LckChunk * FASTCALL GetLckDescriptor(int h) const;
	int    FASTCALL ReleaseLckDescriptor(int h);

	enum {
		tNone = 0,
		tStdFile,
		tFile,
		tSBuffer,
		tNullOutput
	};
	void   SLAPI Init();

	int    T;
	long   Mode;
	union {
		FILE * F;
		SBuffer * P_Sb;
	};
	int    IH;
	SString Name;
	SBuffer BufR; // Буфер чтения ((Mode&mNoStd)&&(Mode&mBuffRd))
	//
	// Таблица дескрипторов блокировок. Таблица в течении жизни объекта никогда не уменьшается в размере.
	// Дескриптором блокировки является индекс позиции блока LckChunk в массиве, увеличенный на 1.
	// При снятии блокировки обнуляется значение Size что свидетельствует о том, что элемент свободен
	// для использования другой блокировкой.
	//
	TSArray <SFile::LckChunk> LckList;
};
//
//
//
class SArchive {
public:
	enum {
		tUnkn = 0,
		tZip  = 1
	};
	SLAPI  SArchive();
	SLAPI ~SArchive();
	int    SLAPI Open(int type, const char * pName, int mode /*SFile::mXXX*/);
	int    SLAPI Close();
	int    SLAPI IsValid() const;
	//
	// Descr: Возвращает количество файлов в архиве
	//
	int64  SLAPI GetEntriesCount() const;
	//
	// Descr: Возвращает имя файла в архиве с индексом idx
	//
    int    FASTCALL GetEntryName(int64 idx, SString & rName);
    //
    // Descr: Извлекает из архива файл с индексом idx и копирует его в pDestName
    //
    int    SLAPI ExtractEntry(int64 idx, const char * pDestName);

    enum {
    	aefRecursive = 0x0001, // Для AddEntries: рекурсивно добавлять файлы из подкаталогов
    	aefDirectory = 0x0002  // Для AddEntry: добавляемый элемент является каталогом
    };
	//
	// Descr: Добавляет в архив новый элемент, соответствующий файлу pSrcFileName с именем pName.
	//   Если pName == 0 || pName[0] == 0, то имя нового элемента соответствует имени файла без пути
	//   в строчном регистре.
	//
    int    SLAPI AddEntry(const char * pSrcFileName, const char * pName, int flags);
    int    SLAPI AddEntries(const char * pMask, int flags);
private:
	int    SLAPI Helper_AddEntries(const SString & rRoot, const SString & rSub, const SString & rMask, int flags);

	int    Type;
	void * H;
};
//
// Descr: Тип функции, используемый для регистрации форматов файлов.
//
typedef int (*FileFormatSignatureFunc)(FILE * pF, long extraParam);
//
// Descr: Определители форматов файлов.
//
class SFileFormat {
public:
	enum {
		Unkn = 0,
		Txt,
		Jpeg,
		Png,
		Tiff,
		Gif,
		Bmp,
		Ico,
		Cur,
		Svg,
		Html, // @v8.1.0
		Xml,  // @v8.1.0
		Ini,  // @v8.1.0
		TxtBomUTF8,        // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF16BE,     // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF16LE,     // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF32BE,     // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF32LE,     // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF7,        // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF1,        // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF_EBCDIC,  // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomSCSU,        // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomBOCU1,       // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomGB18030,     // @v8.6.7 Текстовый файл с сигнатурой типа кодировка (BOM)
		Latex,             // @v8.8.3 LATEX
		Pdf,
		Rtf,
		Mdb,
		AccDb, // Access
		WbXml, // Binary XML
		Wmf,
		Eps,
		Hlp,
		Ppd,    // PostScript
		PList,  // Property List
		Mat,    // Matlab
		Pdb,
		WcbffOld, // Windows Compound Binary File Format

		Zip,               // @v9.0.0 Archive
		Rar,               // @v9.0.0 Archive
		Gz,                // @v9.0.0 Archive
		Tar,               // @v9.0.0 Archive
		Bz2,               // @v9.0.0 Archive
		SevenZ,            // @v9.0.0 Archive
		Xz,                // @v9.0.0 Archive
		Z,                 // @v9.0.0 Archive
		Cab,               // @v9.0.0 Archive
		Arj,               // @v9.0.0 Archive
		Lzh,               // @v9.0.0 Archive
		Xar,               // @v9.0.0 Archive
		Pmd,               // @v9.0.0 Archive
		Deb,               // @v9.0.0 Archive
		Rpm,               // @v9.0.0 Archive
		Chm,               // @v9.0.0 Archive
		Iso,               // @v9.0.0 Archive
		Vhd,               // @v9.0.0 Archive
		Wim,               // @v9.0.0 Archive
		Mdf,               // @v9.0.0 Archive
		Nri,               // @v9.0.0 Archive
		Swf,               // @v9.0.0 Archive
		Mar,               // @v9.0.0 Archive

		Mkv,               // @v9.0.9 Video
		Avi,               // @v9.0.9 Video
		Mp4,               // @v9.0.9 Video
		Wmv,               // @v9.0.9 Video
		Mpg,               // @v9.0.9 Video
		Flv,               // @v9.0.9 Video
		Mov,               // @v9.0.9 Video
		F4f,               // @v9.0.9 Video

		Class,             // @v9.0.9 binary:class 0:CAFEBABE
		Exe,               // @v9.0.9 binary:exe   0:4D5A
		Dll,               // @v9.0.9 binary:dll   0:4D5A
		Pcap,              // @v9.0.9 binary:pcap  0:D4C3B2A1
		Pyo,               // @v9.0.9 binary:pyo   0:03F30D0A
		So,                // @v9.0.9 binary:so    0:7F454C46
		Mo,                // @v9.0.9 binary:mo    0:DE120495
		Mui,               // @v9.0.9 binary:mui   0:50413330
		Cat,               // @v9.0.9 binary:cat   0:30 6:2A864886
		Xsb,               // @v9.0.9 binary:xsb   0:DA7ABABE
		Key,               // @v9.0.9 binary:key   0:4B4C737727
		Sq3,               // @v9.0.9 binary:sq3   0:53514C697465
		Qst,               // @v9.0.9 binary:qst   0:0401C4030000 binary:qst   0:040180040000
		Crx,               // @v9.0.9 binary:crx   0:43723234
		Utx,               // @v9.0.9 binary:utx   0:4C0069006E006500610067006500
		Rx3,               // @v9.0.9 binary:rx3   0:52583362
		Kdc,               // @v9.0.9 binary:kdc   0:44494646
		Xnb,               // @v9.0.9 binary:xnb   0:584E42
		Blp,               // @v9.0.9 binary:blp   0:424C5031 binary:blp   0:424C5032
		Big,               // @v9.0.9 binary:big   0:42494746
		Mdl,               // @v9.0.9 binary:mdl   0:49445354
		Spr,               // @v9.0.9 binary:spr   0:CDCC8C3F
		Sfo,               // @v9.0.9 binary:sfo   0:00505346
		Mpq,               // @v9.0.9 binary:mpq   0:4D50511A
		Nes,               // @v9.0.9 binary:nes   0:4E45531A
		Dmp,               // @v9.0.9 binary:dmp   0:4D444D5093A7
		Dex,               // @v9.0.9 binary:dex   0:6465780a30333500 binary:dex   0:6465780a30333600
		Gim,               // @v9.0.9 binary:gim   0:4D49472E30302E31505350
		Amxx,              // @v9.0.9 binary:amxx  0:58584D41

		Sln,               // Visual Studio Solution
		VCProj,            // Visual Studio Project
		Asm,               // Assempbler source file
		C,                 // C source file
		CPP,               // CPP source file
		H,                 // C header file
		Perl,              // perl source code
		Php,               // php source code
		Java,              // java source code
		Py,                // Python source code
		UnixShell,         // Unix Shell script
		Msi,               // Microsoft Installer package
		TxtUtf8,           // @v9.3.6 Текст в формате utf8
		TxtAscii,          // @v9.3.6 Текст в котором только ASCII-символы
		Log,               // @v9.7.1 Файл журнала (by ext only)
		Properties,        // @v9.7.1 Файл properties. Обычно текстовый файл аналогичный ini, но без зональности ([]).
		Css,               // @v9.7.1 CSS
		JavaScript,        // @v9.7.1 JS

		FirstUser = 10000
	};
	static int Register(int id, const char * pMime, const char * pExt, const char * pSign);
	static int Register();
	static int GetMime(int id, SString & rMime);
	static int GetExt(int id, SString & rExt);

	SFileFormat();
	SFileFormat(int f);
	operator int () const;
	int    operator !() const;
	//
	// Descr: Идентифицирует формат файла pFileName по расширению и сигнатуре.
	// ARG(pFileName IN): @#vptr  Имя файла, формат которого необходимо иденитифицировать
	// ARG(pExt     OUT): @#vptr0 Расширение файла (справочный параметр, позволяющий избежать дополнительных
	//   действий в вызывающем модуле).
	// Returns:
	//   0 - ошибка (isempty(pFileName) || не инициализированна внутренняя таблица сигнатур)
	//   1 - формат идентифицирован по расширению
	//   2 - формат идентифицирован по сигнатуре
	//   3 - формат идентифицирован одновременно по расширению и сигнатуре
	//   4 - формат не удалось идентифицировать по расширению или сигнатуре, однако
	//     по начальному блоку данных он похож на результирующий формат.
	//     На текущий момент такой вариант возможен для форматов: SFileFormat::TxtAscii, SFileFormat::TxtUtf8, SFileFormat::Txt
	//  -1 - не удалось идентифицировать формат
	//
	int    Identify(const char * pFileName, SString * pExt = 0);
	int    IdentifyMime(const char * pMime);
private:
	static uint GloBaseIdx;
	int    Id;
};
//
// Descr: Определители типов кодировок
//
class SEncodingFormat {
public:
	enum {
		Unkn = 0,
		Base64
	};
	SEncodingFormat(int f);
	operator int () const;
private:
	int    Id;
};

SUnicodeMode FASTCALL SDetermineUtfEncoding(const void * pBuf, size_t bufLen);
SEOLFormat FASTCALL SDetermineEOLFormat(const void * pBuf, size_t bufLen);

class STextEncodingStat {
public:
	STextEncodingStat();
	STextEncodingStat & Init();
	int    Add(const void * pData, size_t size);
	int    Finish();

	enum {
		fEmpty         = 0x0001,
		fAsciiOnly     = 0x0002,
		fLegalUtf8Only = 0x0004,
		fMiscEolf      = 0x0008
	};
	long   Flags;
	SCodepageIdent Cp;
	SEOLFormat Eolf;
	uint64 ChrFreq[256];
private:
	uint8 Utf8Prefix[8];
	size_t Utf8PrefixLen;
};

//
// Descr: Low level data alphabet analyzer
//
class SLldAlphabetAnalyzer {
public:
	struct Entry {
		SLAPI  Entry();
		void   SLAPI Clear();

		uint64 C;
		uint64 LastP; // Последняя позиция потока, в которой встречался символ
		double P1; // Сумма периодов появления
		double P2; // Сумма квадратов периодов появления
	};
	SLAPI  SLldAlphabetAnalyzer();
	SLAPI ~SLldAlphabetAnalyzer();
	void   SLAPI Clear();
	int    SLAPI CollectFileData(const char * pFileName);
    int    FASTCALL AddSymb(uint8 s);

    uint64 SLAPI GetCount() const;
	double FASTCALL GetFreq(uint8 s) const;
	double FASTCALL GetPeriodExp(uint8 s) const;
	double FASTCALL GetPeriodStdDev(uint8 s) const;
	//
	// Descr: Возвращает массив символов, ассоциированных с частотой появления,
	//   отсортированный в порядке уменьшения частоты.
	//
	int    FASTCALL GetFreqListOrdered(RAssocArray & rList) const;
private:
	TSArray <Entry> Alphabet;
	uint64 Count;
	long   Status;
};

class SDataChunkStat {
public:
	enum {
		sfText    = 0x0001, // Текстовые данные (иначе - бинарные)
		sfRecords = 0x0002
	};
	struct Result {
		uint64 Size;
		uint32 BlockSize;
		SEOLFormat Eol;
		uint   AvgLineLen;
        uint8  FldDivider[32]; // Если идентифицировано наличие отдельных полей в текстовом файле, то - разделитель полей
        uint   FldCount; // Если идентифицировано наличие отдельных полей в текстовом файле, то - количество полей в записи
		long   Flags;
		SString Ident; // Текстовый идентификатор данных (например, имя файла)
	};
	SDataChunkStat();
};

#endif // } _WIN32_WCE
//
// CRegExp
//
//         ^    Match at beginning of line
//         $    Match at end of line
//         .    Match any single character
//         [ ]  Match any one character inside the brackets
//         [^ ] Match any character NOT inside the brackets
//         -    Match any character in range on either side of dash
//         *    Match preceding pattern zero or more times
//         +    Match preceding pattern one or more times
//         ?    Match preceding pattern zero or once only
//         ()   Save a matched expression and use it in a further match.
//
//   *  The regular expression  "^hello" matches  a "hello"  only at  the
//      beginning of a  line.  It would match "hello  there" but not "hi,
//      hello there".
//
//   *  The regular expression "long$" matches a  "long"  only at the end
//      of a line. It would match "so long\0", but not "long ago".
//
//   *  The regular expression "t..t..g"  will match anything that  has a
//      "t" then any two characters, another "t", any  two characters and
//      then a "g".   It will match  "testing", or "test again" but would
//      not match "toasting"
//
//   *  The regular  expression "[1-9ab]" matches any  number one through
//      nine, and the characters  "a" and  "b".  It would match "hello 1"
//      or "begin", but would not match "no-match".
//
//   *  The  regular expression "[^1-9ab]"  matches any character that is
//      not a number one  through nine, or  an "a" or "b".   It would NOT
//      match "hello 1" or "begin", but would match "no-match".
//
//   *  The regular expression "br* " matches  something that begins with
//      a "b", is followed by zero or more "r"s, and ends in a space.  It
//      would match "brrrrr ", and "b ", but would not match "brrh ".
//
//   *  The regular expression "br+ " matches something  that begins with
//      a "b", is followed by one or more "r"s, and ends in  a space.  It
//      would match "brrrrr ",  and  "br ", but would not  match "b  " or
//      "brrh ".
//
//   *  The regular expression "br? " matches  something that begins with
//      a "b", is followed by zero or one "r"s, and ends in  a space.  It
//      would  match  "br ", and "b  ", but would not match  "brrrr "  or
//      "brrh ".
//
//   *  The regular expression "(..p)b" matches  something ending with pb
//      and beginning with whatever the two characters before the first p
//      encounterd in the line were.  It would find  "repb" in "rep drepa
//      qrepb".  The regular expression "(..p)a"  would find "repa qrepb"
//      in "rep drepa qrepb"
//
//   *  The regular expression "d(..p)" matches something ending  with p,
//      beginning with d, and having  two characters  in between that are
//      the same as the two characters before  the first p  encounterd in
//      the line.  It would match "drepa qrepb" in "rep drepa qrepb".
//
#define NSUBEXP 10

class CRegExp {
public:
	CRegExp();
	CRegExp(const char * s);
	CRegExp(const CRegExp &);
	~CRegExp();
	int    Compile(const char * pPattern);      // Compiles char* --> regexp
	int    Find(const char * pText);            // TRUE if regexp in char* arg
	int    Find(SStrScan *);
	int    GetLastErr() const;
	int    IsValid() const;
	//
	// Descr: Returns index into the searched string that corresponds to the  beginning pointer
	//
	long   start() const { return (long)(startp[0] - P_Text); }
	//
	// Descr: Returns index into the searched string that corresponds to the  end pointer
	//
	long   end() const { return (long)(endp[0] - P_Text); }
	int    FASTCALL operator == (const CRegExp & r) const;
	int    FASTCALL operator != (const CRegExp & r) const { return(!(*this == r)); }
	int    FASTCALL deep_equal(const CRegExp & r) const; // Same regexp and state?
	void   set_invalid();
private:
	char * FASTCALL regatom(int * pFlag);   // @<<regpiece
	char * FASTCALL regpiece(int * pFlag);  // @<<regbranch
	char * FASTCALL regbranch(int * pFlag); // @<<reg
	char * reg(int paren, int * pFlag); // @<<compile,regatom
	char * FASTCALL regnode(char op);
	void   FASTCALL regc(char b);
	void   reginsert(char op, char * opnd);

	const  char * FASTCALL regnext(const char * p) const;
	char * FASTCALL regnext(char * p) const;
	void   regtail(char * p, const char * val);
	void   regoptail(char * p, const char * val);
	int    FASTCALL regrepeat(const char * p);
	int    regtry(const char * string, const char ** start, const char ** end, const char * prog);
	int    FASTCALL regmatch(const char * prog);

	const  char * startp[NSUBEXP];
	const  char * endp[NSUBEXP];
	int    ErrCode;                  // Last error code (SLERR_XXX)
	//
	char   RegStart;                 // Internal use only
	char   RegAnch;                  // Internal use only
	char   RegDummy;                 //
	char   Reserve;                  // @alignment
	//
	const  char * P_RegMust;         // @*compile Internal use only
	const  char * P_RegParse;        // @*compile Input-scan pointer.
	char * P_RegCode;                // @*compile Code-emit  * pointer;  * &RegDummy = don't.
	int    RegNPar;                  // () count.
	size_t RegMLen;                  // Internal use only
	size_t RegSize;                  // Code size.
	char * P_Program;
	size_t ProgSize;                 // Размер распределенного пространства под буфер P_Program
	const char * P_Text;
	//
	// Searching members
	//
	const char * P_RegBol;     // @*find() Beginning of input, for ^ check.
	const char * P_RegInput;   // String-input pointer.
	const char ** PP_RegStart; // @*regtry Pointer to startp array.
	const char ** PP_RegEnd;   // Ditto for endp.
};
//
//
//
class SRegExpSet {
public:
	SRegExpSet();
	~SRegExpSet();
	int    RegisterRe(const char * pRe, long * pHandler);
protected:
	int    InitReNumber();
	int    InitReHex();
	int    InitReEmail();

	CRegExp * P_ReQuotedStr;
	CRegExp * P_ReNumber;
	CRegExp * P_ReIdent;
	CRegExp * P_ReHex;
	CRegExp * P_ReDigits;
	CRegExp * P_ReEMail;
	CRegExp * P_ReDate;
	TSCollection <CRegExp> ReList; // Список регулярных выражений, регистрируемых вызывающим модулем.
};
//
// Descr: Структура, управляющая сканированным участком строки
//
struct SStrScan : public SRegExpSet {
	enum {
		wsSpace   = 0x0001,
		wsTab     = 0x0002,
		wsNewLine = 0x0004,
		wsComma   = 0x0008,
		wsSemicol = 0x0010
	};

	SStrScan(const char * pBuf = 0, size_t offs = 0);
	~SStrScan();
	int    FASTCALL Set(const char * pBuf, size_t offs);
	operator const char * () const { return P_Buf ? (P_Buf+Offs) : 0; }

	int    FASTCALL Push(uint * pPrevPos = 0);
	int    FASTCALL Pop();
	int    FASTCALL Pop(uint prevPos);

	int    IsNumber();
	int    IsHex();
	//
	// Descr: сопоставляет текущий и, возможно, следующий символ с символом новой строки.
	//   Параметр eolf задает формат новой строки.
	// Returns:
	//   0  - текущий символ не является символом новой строки
	//   !0 - текущий символ и, возможно, последующий идентифицируют новую строку. Возвращаемое
	//     значение определяет количество символов, используемых для 'того (1 или 2).
	//
	uint   FASTCALL IsEol(SEOLFormat eolf) const;

	SString & FASTCALL Get(SString & rBuf) const;
	int    FASTCALL GetQuotedString(SString & rBuf);
	int    FASTCALL GetIdent(SString & rBuf);
	int    FASTCALL GetNumber(SString & rBuf);
	//
	// Descr: То же, что и GetNumber, но допускает отсутствие '0' перед '.'
	//   Например: "-.51" будет распознано как число -0.51 в то время как
	//   GetNumber вернет 0.
	//
	int    FASTCALL GetDotPrefixedNumber(SString & rBuf);
	int    FASTCALL GetHex(SString & rBuf);
	int    FASTCALL GetDigits(SString & rBuf);
	int    FASTCALL GetEMail(SString & rBuf);
	int    GetDate(long datefmt, LDATE & rDate);
	//
	// Descr: Если следующая последовательность символов равна pPattern (без учета регистра),
	//   то копирует ее в rBuf и перемещает текущий указатель в позицию, следующую за паттерном.
	// Returns:
	//   >0 - сработало обнаружение паттерна
	//   0  - паттерн не обнаружен в текущей позиции (ничего не изменилось)
	//
	int    SLAPI Get(const char * pPattern, SString & rBuf);
	//
	// Descr: Сканирует все символы до тех пор пока не встретится один из разделителей,
	//   перечисленных в pDiv или конец строки.
	// Returns:
	//   >0 - если был сканирован по крайней мере один символ
	//   0  - очередной символ является разделителем или мы уперлись в конец строки.
	//
	int    SLAPI GetWord(const char * pDiv, SString & rBuf);
	//
	// Descr: Если следующая последовательность равна: key = "value"
	//   то копирует key в rKey, value в rVal и перемещает указатель в позицию, следующую за "value"
	// Returns:
	//   >0 - сработало обнаружение конструкции
	//   0  - конструкция не обнаружена
	//
	int    SLAPI GetEqQ(SString & rKey, SString & rVal);
	//
	// Descr: Если следующая последовательность равна: key = number
	//   то копирует key в rKey, number в rVal и перемещает указатель в позицию, следующую за "value"
	// Returns:
	//   >0 - сработало обнаружение конструкции
	//   0  - конструкция не обнаружена
	//
	int    SLAPI GetEqN(SString & rKey, double & rVal);
	//
	// Descr: Если текущий символ вместе, возможно, с последующими, является допустимым UTF8 символом, то
	//   копирует все необходимые символы в буфер rBuf и возвращает количество скопированных символов.
	//   В противном случае возвращает 0.
	//   Текущая позиция перемещается вперед на возвращенное количество символов.
	// Note: Буфер rBuf предварительно очищается функцией.
	//
	int    FASTCALL GetUtf8(SString & rBuf);
	//
	// Descr: Увеличивает смещение Offs на incr байт.
	// Returns:
	//   Значение Offs, которое было до вызова функции
	//
	size_t FASTCALL Incr(size_t incr = 1);
	//
	// Descr: Увеличивает смещение Offs на величину Len+addedIncr
	// Returns:
	//   Значение Offs, которое было до вызова функции
	//
	size_t FASTCALL IncrLen(size_t addedIncr = 0);
	//
	// Descr: Если очередной символ равен chr, то сдвигает текущую позицию
	//   на единицу и возвращает 1, в противном случае возвращает 0 и не
	//   меняет текущую позицию.
	//
	int    FASTCALL IncrChr(int chr);
	SStrScan & FASTCALL Skip(int ws = wsSpace | wsTab);
	//
	// Descr: Пропускает один опциональный разделитель div.
	//   Если skipWs != 0, то вызывает Skip до разделителя и,
	//   если разделить встретился, то и после него.
	//
	SStrScan & SkipOptionalDiv(int div, int skipWs = wsSpace | wsTab);
	int    FASTCALL SearchChar(int c);
	//
	// Descr: Ищет строку pPattern начиная с адреса P_Buf+Offs.
	//   Если строка найдена, то возращает значение >0 и устанавливает
	//   член Len равным смещению между началом встретившейся подстроки
	//   и Offs.
	// Returns:
	//   0 - образец pPattern не найден: Len не изменяется.
	//   1 - образец pPattern найден
	//
	int    FASTCALL Search(const char * pPattern);
	//
	// Descr: Проверяет текущий символ на значение '<'.
	// Returns:
	//   !0 - текущий символ является началом ML-тега '<'
	//   0  - текущий символ не равен '<'
	//
	int    FASTCALL IsTagBrace() const;
	//
	// Descr: Извлекает из потока обрамление ML-тега <xxx>.
	//   Если это обрамление присутствует в потоке, то в буфер rText заносит текст
	//   внутри скобок. По указателю pKind присваивает вид тега:
	//    0 - <xxx>
	//    1 - </xxx>
	//    2 - <xxx/>
	//   -1 - ошибка, либо в текущей позиции потока нет ML-тега
	//   Если обрамление тега удачно извлечено, то член Len принимает значение общей длины просканированных
	//   данных: функцией IncrLen можно переместить указатель на следующий за тегом символ.
	// Returns:
	//   !0 - обрамление ML-тега успешно считано
	//   0  - по текущей позиции нет тега, либо тег ошибочный
	//
	int    FASTCALL GetTagBrace(SString & rText, int * pKind);
	//
	//
	//
	int    FASTCALL IsRe(long reHandler);
	int    SLAPI GetRe(long reHandler, SString & rBuf);
	int    FASTCALL Is(const char * pS) const;
	//
	// Descr: Проверяет, является ли следующим символом c.
	//
	int    FASTCALL Is(char c) const;
	int    SLAPI IsDigits();
	int    SLAPI IsDotPrefixedNumber();
	//
	// Descr: Проверяет, является ли текущий символ (P_Buf+Offs) вместе, возможно, со следующими
	//   за ним правильной комбинацией, представляющей UTF8 символ.
	//
	int    IsLegalUtf8() const;

	const  char * P_Buf;
	size_t Offs;
	size_t Len;
private:
	TSStack <size_t> Stk;
};
//
//
//
struct ApproxStrSrchParam {
	double umin;     // Мин количество очков, при котором поиск считается успешным
	double weight;   // Вес каждого последующего символа при сравнении
	int    method;   // 0 - default, 1 - Papyrus
	int    no_case;  //
	double maxscore; // Очки, набранные при сравнении
	size_t maxpos;   // Позиция наиболее подходящей подстроки буфера поиска
};

struct dist_weights;
struct dist_components;

class ApproxStrComparator {
public:
	ApproxStrComparator(const char * pPattern, const ApproxStrSrchParam * pParam);
	double FASTCALL Next(const char * b2);
private:
	double Distance();
	double Score();
	double FASTCALL Distance(const dist_weights & w);
	double FASTCALL Score(const dist_weights & w);
	void   FASTCALL Distance(dist_components & c);

	int    MaxSize;
	int    Del1;
	int    Del2;
	int    Swaps;
	int    Subs;
	SString Pattern;
	SString Temp;
	BitArray Track1;
	BitArray Track2;
	ApproxStrSrchParam P;
};

int SLAPI ApproxStrSrch(const char * pPattern, const char * pBuffer, ApproxStrSrchParam *);
int SLAPI ApproxStrCmp(const char * pStr1, const char * pStr2, int noCase, double * pScore);
int FASTCALL ExtStrSrch(const char * pBuffer, const char * pPattern);
//
// Tag parser
//
class XTagParser {
public:
	SLAPI  XTagParser();
	SLAPI ~XTagParser();

	int    SLAPI Run(const char * pFileName);
protected:
	enum {
		tokEOF = -1,
		tokErr = 0,
		tokChar,
		tokTag,
		tokEndTag
	};
	virtual int SLAPI ProcessTag(const char * pTag, long) = 0;
	//
	// Returns tokXXX
	//
	int    SLAPI GetToken(const char * pCurTag, char * pTagBuf, size_t bufLen);
private:
	char   FileName[MAXPATH];
	FILE * P_Stream;
};
//
// Mathematics {
//
class SMathConst {
public:
	static const double Pi;
	static const double Pi2;        // pi*2
	static const float  Pi_f;
	static const double PiDiv180;   // pi/180 (DEG to RAD)
	static const float  PiDiv180_f; // pi/180 (DEG to RAD)
	static const double PiDiv4;     // pi/4
	static const float  PiDiv4_f;   // pi/4
	static const double PiDiv2;     // pi/2
	static const double E;
	static const double LnPi;    // 1.14472988584940017414342735135      ln(pi)
	static const double Ln2;     // 0.69314718055994530941723212146      ln(2)
	static const double Epsilon; // 2.2204460492503131e-16  DBL_EPSILON (машинный ноль)
	static const double Root4Epsilon; // 1.2207031250000000e-04
	static const double Root5Epsilon; // 7.4009597974140505e-04
	static const double SqrtMin; // 1.4916681462400413e-154
	static const double SqrtMax; // 1.3407807929942596e+154
	static const double LogMin;  // -7.0839641853226408e+02
	static const double LogMax;  // 7.0978271289338397e+02
	static const double Min;     // 2.2250738585072014e-308
	static const double Max;     // 1.7976931348623158e+308          max value
	static const double Euler;   // 0.57721566490153286060651209008  Euler constant
	static const double Sqrt2;   // 1.41421356237309504880168872421      sqrt(2)
	static const double SqrtPi;  // 1.77245385090551602729816748334      sqrt(pi)
	static const double Sqrt1_2; // 0.707106781186547524400844362104849039 sqrt(0.5)
};

struct SMathResult {
	operator double() const
	{
		return V;
	}
	void   SetErr(double e, double adjMult);
	void   AdjustErr(double mult);
	void   SetZero();
	int    SetDomainViolation();
	int    SetOverflow();
	int    SetUnderflow();
	double V; // Value (result)
	double E; // Error
};

typedef long LMIDX;
typedef TSArray <unsigned long> UlongArray;

int    SLAPI IsValidIEEE(double);
int    fisnan(double);
int    fisinf(double);
double fgetnan();    // NAN
double fgetposinf(); // +INF
double fgetneginf(); // -INF
//
// Descr: Раскладывает число val на простые множители.
//   Список простых множителей заносится в массив pList.
//   Сейчас используется самый наивный алгоритм перебора.
//
int    SLAPI Factorize(ulong val, UlongArray * pList);
int    SLAPI MutualReducePrimeMultiplicators(UlongArray & rA1, UlongArray & rA2, UlongArray * pReduceList);
long   GetTranspositionNumber(long list[], long count);
int    GetNthTransposition(long list[], long count, long n);

#ifndef _WIN32_WCE // {
//
// Descr: Гистограмма.
//   Каждая корзинка гистограммы ограничивает свой диапазон снизу закрытым условием (>=)
//   Идентификатор -MAXLONG служит для обозначения агрегации величин, меньших, чем минимальная граница
//
class SHistogram {
public:
	struct Val {
		long   Id;
		long   Count;
		double Sum;
	};
	struct Result {
		long   Id;
		double Low;
		double Upp;
		long   Count;
		double Sum;
	};
	enum {
		kStatic = 0, // Статическая гистограмма (корзины заданы до передачи значений)
		kDynamic,    // Динамическая гистограмма (корзины формируются оп мере поступления значений,
			// исходя из заданной нижней величины (LeftEdge) и шага (Step)).
		kDeviation   // Гистограмма, сортирующая значения по отклонению от среднего. Требуется два
			// прохода: на первом собирается статистика, на втором значения разносятся по
			// корзинам. Такая гистограмма может быть либо четной, либо нечетной.
			// Четная - слева и справа от среднего значения располагаются две корзины,
			// Нечетная - среднее значение делит центральную корзину на две части.
	};
	SLAPI  SHistogram();
	SLAPI ~SHistogram();
	int    SLAPI Setup();
	int    SLAPI SetupDynamic(double leftEdge, double step);
	int    SLAPI SetupDev(int even, double widthSigm, uint binCount = 0);
	int    SLAPI AddBin(long binId, double lowBound);
	int    SLAPI GetBin(long binId, Val *) const;
	int    SLAPI GetTotal(Val *) const;
	int    SLAPI GetBinByVal(double val, long * pBinId) const;
	//
	// Descr: Реализует первый проход добавления элемента в гистограмму с признаком kDeviation.
	//
	int    SLAPI PreparePut(double val);
	long   SLAPI Put(double val);
	const  StatBase * SLAPI GetDeviationStat() const;
	int    SLAPI GetDeviationParams(double * pMean, double * pWidth) const;

	uint   SLAPI GetResultCount() const;
	int    SLAPI GetResult(uint pos, Result * pResult) const;
private:
	enum {
		fDynBins   = 0x0001,
		fDeviation = 0x0002,
		fEven      = 0x0004  // Четная гистограмма отклонений о среднего.
	};
	long   Flags;
	RAssocArray BinList;
	TSArray <SHistogram::Val> ValList;
	double LeftEdge;
	double Step;
	double DevWidthSigm;
	uint   DevBinCount;
	double DevMean;
	double DevWidth;
	StatBase * P_Stat;
};
#endif // } _WIN32_WCE
//
//
//
class LVect {
public:
	enum {
		fNotOwner = 0x0001
	};
	SLAPI  LVect();
	SLAPI ~LVect();
	int    SLAPI init(LMIDX dim, const double * pVals = 0);
	int    SLAPI copy(const LVect &);
	const  char * SLAPI getname() const { return P_Name; }
	int    SLAPI setname(const char * pName);
	LMIDX  SLAPI size() const { return Dim; }
	double SLAPI get(LMIDX) const;
	int    SLAPI set(LMIDX, double);
	int    SLAPI zero(LMIDX = -1);

	void   SLAPI mult(double val);           // this *= val
	void   SLAPI div(double val);            // this /= val
	int    SLAPI add(const LVect & s);       // this += s
	double SLAPI dot(const LVect & s) const; // return this * s (scalar)
	void   SLAPI saxpy(double a, const LVect & y); // this = this * a + y

	friend class     LMatrix;
	friend LVect   * SLAPI operator * (const LMatrix &, const LVect &);
	friend LMatrix * SLAPI operator * (const LMatrix &, const LMatrix &);
	friend LVect   * SLAPI operator * (const LVect &, const LMatrix &);
	friend LMatrix * SLAPI operator * (const LVect &, const LVect &);
private:
	char * P_Name;
	LMIDX  Dim;
	double * P_Vals;
};

class LMatrix {
public:
	SLAPI  LMatrix();
	SLAPI ~LMatrix();
	int    SLAPI init(LMIDX numRows, LMIDX numCols);
	int    SLAPI copy(const LMatrix &);
	const  char * SLAPI getname() const { return P_Name; }
	int    SLAPI setname(const char * pName);

	int    SLAPI checktarget(LMIDX row, LMIDX col) const;
	LMIDX  SLAPI rows() const { return NumRows; }
	LMIDX  SLAPI cols() const { return NumCols; }

	double SLAPI get(LMIDX row, LMIDX col) const;
	LVect * SLAPI getrow(LMIDX row) const;
	LVect * SLAPI getcol(LMIDX col) const;

	int    SLAPI set(LMIDX row, LMIDX col, double);
	int    SLAPI setrow(LMIDX row, const LVect &);
	int    SLAPI setcol(LMIDX col, const LVect &);
	void   SLAPI zero(LMIDX row, LMIDX col);

	void   SLAPI swaprows(LMIDX r1, LMIDX r2);

	int    SLAPI transpose(const LMatrix &);
	double SLAPI Jacobian() const;
	int    FASTCALL operator += (const LMatrix &);
	int    FASTCALL operator -= (const LMatrix &);

	friend LVect   * SLAPI operator * (const LMatrix &, const LVect &);
	friend LMatrix * SLAPI operator * (const LMatrix &, const LMatrix &);
	friend LVect   * SLAPI operator * (const LVect &, const LMatrix &);
	friend LMatrix * SLAPI operator * (const LVect &, const LVect &);
private:
	double * SLAPI sget(LMIDX, LMIDX) const;
	int    SLAPI add(const LMatrix &, int minus);
	size_t SLAPI dim() const { return (size_t)(NumRows * NumCols); }

	char * P_Name;
	LMIDX  NumRows;
	LMIDX  NumCols;
	uint   Flags;
	double * P_Vals; // Column(0), Column(1), .., Column(NumCols-1)
};

void   lubksb(LMatrix & a, LMIDX indx[/*b.size()*/], LVect & b);
int    ludcmp(LMatrix & a, LMIDX indx[/*a.rows()*/], int & d);
void   SLAPI print(const LVect &, FILE *, long fmt);
void   SLAPI print(const LMatrix &, FILE *, long fmt);
int    SLAPI read(LVect *, FILE *);
int    SLAPI read(LMatrix *, FILE *);

LVect   * SLAPI operator * (const LMatrix &, const LVect &);
LMatrix * SLAPI operator * (const LMatrix &, const LMatrix &);
//
// Descr: Product of row-vect[m] and matrix[m, n] : row-vect[n]
//
LVect   * SLAPI operator * (const LVect &, const LMatrix &);
//
// Descr: Product of column-vect[n] and row-vect[m] : matrix[n, m]
//
LMatrix * SLAPI operator * (const LVect &, const LVect &);
//
//
//
LVect * SLAPI gaxpy(const LMatrix & a, const LVect & x, const LVect & y);
//
// Descr: Матрица двухмерного аффинного преобразования.
// Attention: Бинарный формат не менять. Он идентичен cairo_matrix_t.
//
class LMatrix2D { // @size=48
public:
	// @novtbl
	LMatrix2D();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	LMatrix2D & InitUnit(double v = 1.0);
	LMatrix2D & InitScale(double x, double y);
	LMatrix2D & InitTranslate(double dx, double dy);
	LMatrix2D & InitTranslate(FPoint p);
	LMatrix2D & InitRotate(double teta);
	LMatrix2D & InitRotateDeg(double tetaDeg);
	LMatrix2D & InitSkew(double angleX, double angleY);
	LMatrix2D & InitSkewDeg(double angleXDeg, double angleYDeg);
	LMatrix2D & operator = (double s);
	LMatrix2D & FASTCALL operator = (const LMatrix2D & rS);
	LMatrix2D & Mult(const LMatrix2D & rM1, const LMatrix2D & rM2);
	LMatrix2D & Mult(double m);
	//
	// Descr: Обращает матрицу this.
	//   Если матрица является вырожденной, то возвращает 0.
	//
	int    Invert();

	int    IsIdentical() const;
	double GetDeterminant() const;
	RPoint & FASTCALL TransformDistance(RPoint & rP) const;
	RPoint & FASTCALL Transform(RPoint & rP) const;

	enum {
		fmtDefault = 0,
		fmtSVG
	};
	int    FromStr(const char * pStr, int fmt);
private:
	LMatrix2D & ComputeAdjoint();

	double xx;
	double yx;
	double xy;
	double yy;
	double x0;
	double y0;
};

const LMatrix2D & FASTCALL operator * (const LMatrix2D & rLeft, const LMatrix2D & rRight);

class LMatrix3D {
public:
	SLAPI  LMatrix3D();
	LMatrix3D & operator = (double s);
	LMatrix3D & operator = (const LMatrix3D & rS);
	LMatrix3D & SLAPI InitUnit(double v);
	LMatrix3D & SLAPI InitScale(double x, double y, double z);
	LMatrix3D & SLAPI InitRotateX(double teta);
	LMatrix3D & SLAPI InitRotateY(double teta);
	LMatrix3D & SLAPI InitRotateZ(double teta);
	LMatrix3D & SLAPI Mult(const LMatrix3D & rM1, const LMatrix3D & rM2);
public:
	double M[4][4];
};

class StatBase {
public:
	enum {
		fExt          = 0x0001, // Рассчитывать дополнительные статистики
		fGammaTest    = 0x0002, // Выполнить тест на гамма-распределение
		fGaussianTest = 0x0004, // Выполнить тест на нормальное распределение
		fStoreVals    = 0x0008  // Сохранять значения в Series
	};
	SLAPI  StatBase(long flags = 0);
	virtual SLAPI ~StatBase();

	virtual int SLAPI Init(long flags = 0);
	virtual int SLAPI Finish();
	int    SLAPI Step(double);

	long   SLAPI GetCount() const;
	double SLAPI GetSum() const;
	double SLAPI GetMin() const;
	double SLAPI GetMax() const;
	double SLAPI GetExp() const;
	double SLAPI GetVar() const;
	double SLAPI GetVariance() const;
	double SLAPI GetStdDev() const;
	double SLAPI GetTestGamma() const;
	double SLAPI GetTestGaussian() const;
	int    SLAPI GetValue(long idx, double * pVal) const;
	int    SLAPI GetGammaParams(double * pAlpha, double * pBeta) const;
protected:
	long   Flags;
	long   IterCount;
	long   Count;
	double Min;
	double Max;
	double Sum[4]; // Sum[0] - линейная сумма, Sum[1] - сумма квадратов, Sum[2] - сумма кубов,
		// Sum[3] - сумма четвертых степеней
	double Exp;    // Математическое ожидание
	double Var;    // Дисперсия //
	double Kurtosis; // if (Flags & fExt) ((Sum4 / Count) / pow(SumQ / Count, 2)) - 3.0;
	double Skew;     // if (Flags & fExt) (Sum3 / Count) / pow(SumQ / Count, 1.5)
	double Test_Z;
	double Test_ChSq;
	RealArray Series; // Если (Flags & (fGammaTest|fGaussianTest|fStoreVals)), то все поступающие
		// значения сохраняются в этом массиве
};

struct TimSerEntry {
	long   M;
	double V;
};

typedef TSQueue <double> DblQueue;
typedef TSQueue <TimSerEntry> TimSerQueue;
//
// Descr: Класс, обрабатывающий непрерывные временные ряды
//   Элементом ряда является значение типа double.
//
class TimSerStat : public StatBase {
public:
	SLAPI  TimSerStat();
	virtual SLAPI ~TimSerStat();
	virtual int SLAPI Init();
	virtual int SLAPI Finish();
	int    SLAPI SetNumLags(long);
	int    SLAPI Step(double, int whiteSpace = 0);
	uint   SLAPI GetNumLags() const;
	double SLAPI GetAutocorrel(uint lag) const;
protected:
	long   NumLags;
	DblQueue * P_Queue;
	double * P_AC_Add;  // [NumLags] Sum(Y[i]+Y[i+l])
	double * P_AC_Mul;  // [NumLags] Sum(Y[i]*Y[i+l])
};
//
//
//
class TimSerSpikes {
public:
	SLAPI  TimSerSpikes();
	SLAPI ~TimSerSpikes();

	int    SLAPI Init(int sign /* > 0 - plus only, < 0 - minus only, 0 - both */);
	int    SLAPI Step(long, double);
	long   SLAPI GetNumSpikes() const;
	double SLAPI GetSpike(long, long * pN) const;
	long   SLAPI GetMostCommonDistance();
private:
	struct Spike {
		long   N;
		double V;
	};
	int    Sign;
	long   Count;
	Spike  Prev[2];
	SArray * P_Spikes;
};

class ARMA {
public:
	SLAPI  ARMA();
	SLAPI ~ARMA();

	int    SLAPI Init(int p, int q, const double * pInitTSeries /* p items */);
	int    SLAPI Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix *);
	int    SLAPI Step(double);
	double SLAPI Predict() const;
	int    SLAPI GetModel(LVect *) const;
	int    SLAPI GetPhi(LVect *) const;
	int    SLAPI GetR(LMatrix *) const;
	void   SLAPI GetPQ(int *, int *) const;
private:
	double SLAPI ComputePredictError(double) const;
	int    SLAPI ComputeGainFactor(LVect *) const;

	int    P;
	int    Q;
	long   IterCount;
	LVect  Phi;        // [P+Q+1] vect
	LMatrix R;         // [P+Q+1] x [P+Q+1] matrix
	LVect  Model;      // [P+Q+1] vect
};
//
// Arima
//
class RDI {
public:
	SLAPI  RDI();
	SLAPI ~RDI();
	int    SLAPI Init(const DblQueue * pInitQueue);
	int    SLAPI Init(const double * pInitQueue, uint interval);
	double SLAPI StepDiff(double) const;
	double SLAPI StepIntg(double) const;
	int    SLAPI Step(double newVal);
	int    SLAPI GetQueue(DblQueue * pQueue) const;
	int    SLAPI GetSeason() const { return P_Queue ? P_Queue->size() : 0; }
private:
	DblQueue * P_Queue;
};

class ARIMA : public ARMA {
public:
	SLAPI  ARIMA();
	int    SLAPI Init(int p, int q, const double * pInitSeries, /* p items */ const DblQueue * pRDIInitQueue);
	int    SLAPI Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix *, const DblQueue * pRDIInitQueue);
	int    SLAPI Step(double);
	double SLAPI Predict() const;
private:
	RDI    Rdi;
};
//
// Approximation using Least Square Solution method
//
struct LssLin {
	SLAPI  LssLin();
	int    SLAPI Solve(const LVect & x, const LVect & y);
	double SLAPI Estimation(double x, double * pYErr) const;

	// y = A + x*B
	double A;
	double B;
	double Cov00;
	double Cov01;
	double Cov11;
	double SumSq;
};
//
// Random Number Generator
//
class SRng {
public:
	enum Algorithm {
		algBSD = 1,  // The original BSD version, e.g. on SunOS 4.1 and FreeBSD.
			// Level = {8, 32, 64, 128, 256}
		algLibC5,    // The Linux libc5 version, which is differs from the BSD version in
			// its seeding procedure, possibly due to the introduction of a typo in the multiplier.
			// Level = {8, 32, 64, 128, 256}
		algGLibC2,   // The GNU glibc2 version, which has a new (and better) seeding procedure.
			// Level = {8, 32, 64, 128, 256}
		algRANLUX_S, // This is an implementation of M. Luescher's second generation version of the
			// RANLUX generator. Thanks to Martin Luescher for providing information on this generator.
			// Level = {0, 1, 2)
		algRANLUX_D, // This is an implementation of Martin Luescher's second generation
			// double-precision (48-bit) version of the RANLUX generator. Thanks to Martin
			// Luescher for providing information on this generator.
			// Level = {1, 2)
		algMT        // The MT19937 generator of Makoto Matsumoto and Takuji Nishimura is a variant of
			// the twisted generalized feedback shift-register algorithm, and is known as the "Mersenne Twister"
			// generator. It has a Mersenne prime period of 2^19937 - 1 (about 10^6000) and is equi-distributed
			// in 623 dimensions. It has passed the DIEHARD statistical tests. It uses 624 words of state
			// per generator and is comparable in speed to the other generators. The original generator used
			// a default seed of 4357 and choosing seed equal to zero in Set reproduces this.
			// Level = {0, 1999, 1998}.
	};

	static SRng * CreateInstance(Algorithm alg, uint level);

	virtual void   Set(ulong seed) = 0;
	virtual ulong  Get() = 0;
	virtual double GetReal() = 0;
	//
	// Descr: Возвращает не отрицательное целое число МЕНЬШЕ ЧЕМ n.
	//
	ulong  FASTCALL GetUniformInt(ulong n);
	double GetUniformPos();
	ulong  GetMin() const;
	ulong  GetMax() const;

	double GetGaussian(double sigma);
	double GetGaussianZiggurat(double sigma);
	double GetGaussianPdf(double x, double sigma);
	ulong  GetBinomial(double p, ulong n);
	ulong  GetPoisson(double mu);
	double GetPoissonPdf(ulong k);
	double GetGamma(double a, double b);
	double GetGammaInt(uint a);
	double GetGammaPdf(double x, double a, double b);
protected:
	SRng(int alg, uint level);

	int    Alg;
	uint   Level;
	ulong  RandMin;
	ulong  RandMax;
private:
	double GetGammaKnuth(double a, double b);
	double GetGammaFrac(double a);
};
//
// Descr: Wrapper для SRng (мне надоело каждый раз использовать new/delete и
//   выбирать один и тот же алгоритм).
//
class SRandGenerator {
public:
	SRandGenerator(SRng::Algorithm alg = SRng::algMT, uint level = 0);
	~SRandGenerator();
	void   Set(ulong seed);
	ulong  Get();
	double GetReal();
	//
	// Descr: Возвращает случайное не отрицательное целое число СТРОГО МЕНЬШЕ чем n.
	//
	ulong  FASTCALL GetUniformInt(ulong n);
	double GetUniformPos();
	ulong  GetMin() const;
	ulong  GetMax() const;
	double GetGaussian(double sigma);
	double GetGaussianZiggurat(double sigma);
	double GetGaussianPdf(double x, double sigma);
	ulong  GetBinomial(double p, ulong n);
	ulong  GetPoisson(double mu);
	double GetPoissonPdf(ulong k);
	double GetGamma(double a, double b);
	double GetGammaInt(uint a);
	double GetGammaPdf(double x, double a, double b);

	int    ObfuscateBuffer(void * pBuffer, size_t bufferSize) const;
private:
	SRng * P_Inner;
};
//
// Descr: Вычисляет наибольший общий делитель (GCD) a и b
//
ulong  FASTCALL Gcd(ulong a, ulong b);
//
// Descr: Вычисляет наименьшее общее кратное (LCM) a и b
//
ulong  FASTCALL Lcm(ulong a, ulong b);
//
// Descr: Если val - простое число, то возвращает !0, в противном случае - 0.
//
int    FASTCALL IsPrime(ulong val);
//
// Descr: Специальная функция, сжимающая 64-битное целое число до минимально возможного
//   размера (кратного байту), убирая ведущие нули.
// Note: Функция используется для компактного хранения целого числа если одновременно
//   со значением подсистема хранения держит размер.
// ARG(value  IN): число, подлежащее сжатию
// ARG(pBuf  OUT): буфер, в который заносится результат. Размер, выделенный под
//   этот указатель должен быть не менее 8 байт.
// Returns:
//   Размер результата в байтах
//
uint   FASTCALL sshrinkuint64(uint64 value, void * pBuf);
//
// Descr: Специальная функция, разворачивающая сжатое целое число до нормального 64-битного
//   варианта. Функция обратна по действию функции sshrinkuint64().
// ARG(pBuf IN): Указатель на буфер, в котором хранится сжатое представление числа.
// ARG(size IN): Размер сжатого представления числа.
// Returns:
//   Результат распаковки.
//
uint64 FASTCALL sexpanduint64(const void * pBuf, uint size);
//
//
//
int    FASTCALL smax(int a, int b);
int    FASTCALL smin(int a, int b);
double SLAPI smax(double a, double b);
double SLAPI smin(double a, double b);
float  SLAPI smax(float a, float b);
float  SLAPI smin(float a, float b);
double FASTCALL fdiv100r(double);
double FASTCALL fdiv100i(long);
double FASTCALL fdiv1000i(long);
//
// Descr: Проверяет равенство величин v1 и v2 с точностью до epsilon.
// Returns:
//   1 - fabs(v1-v2) < epsilon
//   0 - fabs(v1-v2) >= epsilon
//
int SLAPI feqeps(double v1, double v2, double epsilon);
//
// Descr: ret = (dr != 0) ? (dd / dr) : 0
//
double FASTCALL fdivnz(double dd, double dr);
//
// Descr: Возвращает целую часть аргумента v
//
double FASTCALL fint(double v); // @>>modf
//
// Descr: Возвращает дробную часть аргумента v
//
double FASTCALL ffrac(double v); // @>>modf
//
// Descr: Если sign < 0, возвращает (val-addendum)
//   Если sign > 0, возвращает (val+addendum)
//   Если sign == 0, возвращает val.
//
double faddwsign(double val, double addendum, int sign);
//
// Descr: Если sign < 0, возвращает -val
//   Если sign > 0, возвращает +val
//   Если sign == 0, возвращает 0.
//
double fgetwsign(double val, int sign);
//double fhepote

#define SQ(x) ((x)*(x))

double FASTCALL fpowi(double x, int n);
double FASTCALL fpow10i(int n);
inline double fpow2(double x) { return x * x; }
inline double fpow3(double x) { return x * x * x; }
//
// Descr: Возводит число x в степень n
//
uint64 SLAPI ui64pow(uint64 x, uint n);

#define FACT_TAB_SIZE 171

double FASTCALL ffactr(uint i);
ulong  FASTCALL ffacti(uint i);

double FASTCALL degtorad(double);
float  FASTCALL degtorad(float);
double FASTCALL degtorad(int);
double FASTCALL radtodeg(double rad);
//
// Descr: Масштабирует значение v относительно диапазона [low..upp]
// Returns:
//   (v - low) / (upp - low)
//
double fscale(double v, double low, double upp);
//
// Descr: Возвращает значение логистической сигмоидальной функции
//   y = 1 / (1 + exp(-a * x))
//
double SLAPI sigmoid(double a, double x);
//
//
//
// Descr: data for a Chebyshev series over a given interval
//
struct ChebSeries {
	double * c;      // coefficients
	int    order;    // order of expansion
	double a;        // lower interval point
	double b;        // upper interval point
	int    order_sp; // effective single precision order */
};
//
// Descr: compute the polygamma function \psi^{(m)}(x) for m >= 0, x > 0.
//
int    fpsi_n(int m, double x, SMathResult * pResult);
//
// Descr: compute the Trigamma function \psi'(n) for positive integer n.
//
int    fpsi_1(double x, SMathResult * pResult);
//
// Descr: compute the digamma function \psi(x) for general x, x \ne 0.
//
int    fpsi(double x, SMathResult * pResult);
//
// Descr: compute the logarithm of the Gamma function, \log(\Gamma(x)), subject to x not a being negative integer.
//   For x<0 the real part of \log(\Gamma(x)) is returned, which is equivalent to \log(|\Gamma(x)|).
//   The function is computed using the real Lanczos method.
//
int    flngamma(double x, SMathResult * pResult);
//
// Descr: These routines compute the logarithm of the factorial of n, \log(n!).
//   The algorithm is faster than computing \ln(\Gamma(n+1)) via gsl_sf_lngamma for n < 170,
//   but defers for larger n.
//
double flnfact(uint n);
int    flnfact(uint n, SMathResult * pResult);
double SLAPI Gamma(double x);
double SLAPI GammaIncompleteP(double a, double x);
double SLAPI GammaIncompleteQ(double a, double x);
inline int fsign(double x)  { return (x >= 0.0) ? +1 : -1; }
inline int fsignz(double x) { return (x > 0) ? +1 : ((x < 0) ? -1 : 0); }

// } Mathematics
//
// Descr: Рассчитывает пересечение прямоугольников rSrc1 и rSrc2. Результат заносит в rDst.
// Note: Функционально абсолютно аналогична Windows-функции IntersectRect. Реализована
//   только для того, чтобы немного выиграть в производительности за счет FASTCALL и прямого вызова (без DLL).
// Returns:
//   1 - пересечение rSrc1 и rSrc2 не пустое
//   0 - пересечение rSrc1 и rSrc2 пустое (в rDst все координаты обнуляются).
//
int FASTCALL SIntersectRect(RECT & rDst, const RECT & rSrc1, const RECT & rSrc2);
//
// Descr: Идентифицирует факт пересечения прямоугольников rSrc1 и rSrc2.
// Returns:
//   1 - пересечение rSrc1 и rSrc2 не пустое
//   0 - пересечение rSrc1 и rSrc2 пустое
//
int FASTCALL SIntersectRect(const RECT & rSrc1, const RECT & rSrc2);
void FASTCALL SInflateRect(RECT & rRect, int cx, int cy);
//
//
//
#define SHAPE_UNDEF        0
#define SHAPE_LINE         1
#define SHAPE_RECT         2
#define SHAPE_TRIANGLE     3
#define SHAPE_TRAPEZ       4
#define SHAPE_CIRCLE       5
#define SHAPE_ELLIPSE      6
#define SHAPE_CIRCLEARC    7
#define SHAPE_ELLIPSEARC   8
#define SHAPE_POLYGON      9
#define SHAPE_POLYLINE    10
#define SHAPE_ROUNDEDRECT 11

struct FRect {
	FRect();
	FRect(float width, float height);
	FRect(const TRect & r);
	FRect & FASTCALL operator = (FPoint p);
	FRect & Around(FPoint p, FPoint size);
	FRect & Grow(float aDX, float aDY);
	FRect & FASTCALL MoveCenterTo(FPoint center);
	int    Contains(FPoint p) const;
	float  Width() const;
	float  Height() const;
	FPoint GetSize() const;
	FPoint GetCenter() const;
	//
	// Descr: Возвращает отношение высоты (Height) к ширине (Width) прямоугольника.
	//   Если ширина равна нулю, то возвращает SMathConst::Max.
	//
	double Ratio() const;

	FPoint a;
	FPoint b;
};

class FShape { // @persistent @store(SSerializeContext)
public:

	typedef FRect Rect;

	struct Line {
		FPoint A;
		FPoint B;
	};
	struct Circle {
		Circle();
		FPoint C; // Center
		float  R; // Radius
	};
	struct Ellipse {
		FPoint C; // Center
		FPoint R; // Радиус по осям X и Y
	};
	struct EllipseArc : public Ellipse {
		EllipseArc();
		float  Start; // Начальный угол (RAD)
		float  End;   // Конечный угол (RAD)
	};
	struct CircleArc : public Circle {
		CircleArc();
		float  Start; // Начальный угол (RAD)
		float  End;   // Конечный угол (RAD)
	};
	struct Triangle {
		FPoint A;
		FPoint B;
		FPoint C;
	};
	struct RoundedRect : public FRect {
		FPoint R;     // Эллиптический (по осям X и Y) радиус скругления углов.
	};
	struct Polygon : public FloatArray {
		uint   GetVertexCount() const;
	};
	struct Polyline : public Polygon {
	};

	FShape(int kind = SHAPE_UNDEF);
	FShape(const FShape & rS);
	~FShape();
	void  destroy();
	int   FASTCALL Copy(const FShape & rS);
	FShape & FASTCALL operator = (const FShape & rS);
	FShape & FASTCALL operator = (const Rect &);
	FShape & FASTCALL operator = (const RoundedRect &);
	FShape & FASTCALL operator = (const Line &);
	FShape & FASTCALL operator = (const Triangle &);
	FShape & FASTCALL operator = (const Circle &);
	FShape & FASTCALL operator = (const Ellipse &);
	FShape & FASTCALL operator = (const CircleArc &);
	FShape & FASTCALL operator = (const EllipseArc &);
	FShape & FASTCALL operator = (const Polygon &);
	FShape & FASTCALL operator = (const Polyline &);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    GetKind() const;
	//
	// Descr: Возвращает количество элементов массива, необходимое
	//   для определения фигуры.
	//
	int    GetCount() const;

	int    FASTCALL Get(Rect &) const;
	int    FASTCALL Get(RoundedRect &) const;
	int    FASTCALL Get(Line &) const;
	int    FASTCALL Get(Triangle &) const;
	int    FASTCALL Get(Circle &) const;
	int    FASTCALL Get(Ellipse &) const;
	int    FASTCALL Get(CircleArc &) const;
	int    FASTCALL Get(EllipseArc &) const;
	int    FASTCALL Get(Polygon &) const;
	int    FASTCALL Get(Polyline &) const;
private:
	int32  Kind; // SHAPE_XXX
	int32  Flags;
	float  P[12];
	FloatArray * P_List;
};
//
//
//
class TPoint { // @persistent @store(SSerializeContext) @size=4
public:
	//
	// @noconstructor @nodestructor @novtbl
	//
	operator FPoint () const;
	uint32 towparam() const;
	TPoint FASTCALL operator = (TPoint p);
	TPoint FASTCALL operator = (int v);
	TPoint FASTCALL operator = (POINT p);
	operator POINT() const;
	int    IsZero() const;
	TPoint Set(int _x, int _y);
	TPoint FASTCALL setwparam(uint32 wp);
	TPoint Add(int add_x, int add_y);
	TPoint operator += (TPoint adder);
	TPoint operator -= (TPoint subber);

	friend TPoint operator - (TPoint one, TPoint two);
	friend TPoint operator + (TPoint one, TPoint two);
	friend TPoint operator + (TPoint pnt, int _x);
	friend int operator == (TPoint one, TPoint two);
	friend int operator != (TPoint one, TPoint two);

	int16  x, y;
};

class TRect {
public:
	TRect();
	TRect(int ax, int ay, int bx, int by);
	TRect(TPoint p1, TPoint p2);
	TRect(TPoint sz);
	TRect(const RECT &);
	TRect & FASTCALL operator = (const RECT &);
	TRect & FASTCALL operator = (TPoint p);
	int    IsEmpty() const;
	//
	// Descr: Если прямоугольник вырожден до прямой линии, то возвращает !0.
	// Returns:
	//   0 - прямоугольник не вырожден
	//   SIDE_TOP    - прямоугольник вырожден до горизонтальной линии
	//   SIDE_RIGHT  - прямоугольник вырожден до вертикальной линии
	//   SIDE_CENTER - прямоугольник вырожден до точки
	//
	int    IsDegenerated() const;
	//
	// Descr: Если ширина или высота прямоугольника отрицательны, то меняет
	//   соответствующие координаты так, что бы привесит ширину и высоту к
	//   неотрицательным значениям.
	// Returns:
	//   *this
	//
	TRect & Normalize();
	operator RECT() const;
	float  CenterX() const;
	float  CenterY() const;
	TRect & move(int aDX, int aDY);
	TRect & FASTCALL move(TPoint delta);
	TRect & FASTCALL movecenterto(TPoint center);
	TRect & grow(int aDX, int aDY);
	//
	// Descr: Находит пересечение прямоугольников *this и rD.
	// Returns:
	//   0 - прямоугольники не пересекаются.
	//  -1 - у одного из прямоугольников перепутаны координаты.
	//   1 - прямоугольники имеют полное пересечение.
	//   Если возвращаемое значение имеет установленный бит 0x02,
	//   то прямоугольники пересекаются по координате X по касательной.
	//   Если возвращаемое значение имеет установленный бит 0x04,
	//   то прямоугольники пересекаются по координате Y по касательной.
	//
	int    Intersect(const TRect & rD, TRect * pResult) const;
	TRect & FASTCALL Union(const TRect & rR);
	TRect & FASTCALL setmarginx(const TRect & rR);
	TRect & FASTCALL setmarginy(const TRect & rR);
	TRect & FASTCALL setmarginx(int v);
	TRect & FASTCALL setmarginy(int v);
	int    width() const;
	int    height() const;
	TRect & set(int x1, int y1, int x2, int y2);
	TRect & set(const FRect & rR);
	TRect & FASTCALL setwidth(const TRect & rR);
	TRect & FASTCALL setheight(const TRect & rR);
	TRect & setwidthrel(int p, int w);
	TRect & setheightrel(int p, int h);
	//
	// Descr: Если прямоугольник не содержит точку p, то функция возвращает 0.
	//   Если точка содержится на границе с прямоугольником, то возвращается сторона
	//   света (SOW_XXX), на границе которой лежит точка.
	//   Если точка содержится в прямоугольнике, но не на границе, то функция возвращает 1000.
	//
	int    FASTCALL contains(TPoint p) const;
	int    operator == (const TRect& r) const;
	int    operator != (const TRect& r) const;

	TPoint a, b;
};
//
// Descr: Геометрический регион. Представляет собой комбинацию геометрических
//   форм, объединенных по определенным правилам.
// Note: Текущая реализация класса использует механизм регионов Windows GDI.
//
class SRegion {
public:
	SRegion();
	SRegion(const TRect & rRc);
	~SRegion();
	void   Destroy();
	int operator !() const;
	SRegion & operator = (const SRegion & rS);
	int    operator == (const SRegion & rS) const;
	int    Add(const TRect & rR, int combine);
	int    Add(const SRegion & rS, int combine);
	//
	// Descr: Комбинирует текущий регион с рамкой, определяемой прямоугольником
	//   rR и толщиной 2*halfThick вдоль каждой стороны прямоугольника.
	//   Используется для такой перерисовки рамок, которая не задевает внутреннюю
	//   область этих рамок и гарантированно перерисует рамку в ходе перемещения.
	//
	int    AddFrame(const TRect & rR, uint halfThick, int combine);
	TRect  GetBounds() const;
private:
	SHandle H;
};
//
// Descr: Специализированная структура для представления 2D-точки
//   в значениях с двойной точностью.
//
class RPoint {
public:
	RPoint & Set(double _xy);
	RPoint & Set(double _x, double _y);
	//
	// Descr: Устанавливает полярные координаты точки.
	//   x = v * cos(rad);
	//   y = v * sin(rad);
	//
	RPoint & SetPolar(double v, double rad);
	void   GetInt(int * pX, int * pY) const;
	void   GetUInt(uint * pX, uint * pY) const;
	int    FASTCALL FromStr(const char * pStr);

	friend RPoint & FASTCALL operator - (const RPoint & one, const RPoint & two);
	friend RPoint & FASTCALL operator + (const RPoint & one, const RPoint & two);
	friend RPoint & FASTCALL operator + (const RPoint & p, double a);
	friend RPoint & FASTCALL operator * (const RPoint & p, double m);
	friend RPoint & FASTCALL operator / (const RPoint & p, double);

	double x;
	double y;
};

class RPoint3 {
public:
	RPoint3 & Set(double _x, double _y, double _z);

	friend RPoint3 & FASTCALL operator - (const RPoint3 & one, const RPoint3 & two);
	friend RPoint3 & FASTCALL operator + (const RPoint3 & one, const RPoint3 & two);
	friend RPoint3 & FASTCALL operator * (const RPoint3 & p, double m);
	friend RPoint3 & FASTCALL operator / (const RPoint3 & p, double);

	double x;
	double y;
	double z;
};
//
//
//
struct UiCoord {
	enum {
		dfAbs     = 0x0001, // Координата задана в абсолютных единицах
		dfRel     = 0x0002, // Координата задана в долях от общего размера контейнера.
			// Значение кодируется как число с фиксированной точкой (4 знака)
		dfGravity = 0x0004, // Координата притягивается к соответствующей границе контейнера.
		dfOpp     = 0x0008, // Координата отсчитывается от противоположной стороны контейнера.
		dfSize    = 0x0010  // Структура представляет линейный размер (не координату)
	};
	int    Set(int v, int f);
	void   Reset();
	int    IsEmpty() const;

	int16  Val;
	int16  Flags; // UiCoord::dfXXX
};

struct UiRelPoint {
	UiRelPoint & FASTCALL Set(const TPoint & rP);

	UiCoord X;
	UiCoord Y;
};

struct UiRelRect {
	void   Reset();
	int    IsEmpty() const;
	UiRelRect & FASTCALL Set(const TRect & rR);

	UiRelPoint L;
	UiRelPoint R;
};
//
//
//
enum SColourCollection { // 0x00rrggbb
	SClrAliceblue      = 0x00f0f8ff,
	SClrAntiquewhite   = 0x00faebd7,
	SClrAqua           = 0x0000ffff,
	SClrAquamarine     = 0x007fffd4,
	SClrAzure          = 0x00f0ffff,
	SClrBeige          = 0x00f5f5dc,
	SClrBisque         = 0x00ffe4c4,
	SClrBlack          = 0x00000000,
	SClrBlanchedalmond = 0x00ffebcd,
	SClrBlue           = 0x000000ff,
	SClrBlueviolet     = 0x008a2be2,
	SClrBrown          = 0x00a52a2a,
	SClrBurlywood      = 0x00deb887,
	SClrCadetblue      = 0x005f9ea0,
	SClrChartreuse     = 0x007fff00,
	SClrChocolate      = 0x00d2691e,
	SClrCoral          = 0x00ff7f50,
	SClrCornflowerblue = 0x006495ed,
	SClrCornsilk       = 0x00fff8dc,
	SClrCrimson        = 0x00dc143c,
	SClrCyan           = 0x0000ffff,
	SClrDarkblue       = 0x0000008b,
	SClrDarkcyan       = 0x00008b8b,
	SClrDarkgoldenrod  = 0x00b8860b,
	SClrDarkgrey       = 0x00555555,
	SClrDarkgreen      = 0x00006400,
	SClrDarkkhaki      = 0x00bdb76b,
	SClrDarkmagenta    = 0x008b008b,
	SClrDarkolivegreen = 0x00556b2f,
	SClrDarkorange     = 0x00ff8c00,
	SClrDarkorchid     = 0x009932cc,
	SClrDarkred        = 0x008b0000,
	SClrDarksalmon     = 0x00e9967a,
	SClrDarkseagreen   = 0x008fbc8f,
	SClrDarkslateblue  = 0x00483d8b,
	SClrDarkslategrey  = 0x002f4f4f,
	SClrDarkturquoise  = 0x0000ced1,
	SClrDarkviolet     = 0x009400d3,
	SClrDeeppink       = 0x00ff1493,
	SClrDeepskyblue    = 0x0000bfff,
	SClrDimgrey        = 0x00696969,
	SClrDodgerblue     = 0x001e90ff,
	SClrFirebrick      = 0x00b22222,
	SClrFloralwhite    = 0x00fffaf0,
	SClrForestgreen    = 0x00228b22,
	SClrFuchsia        = 0x00ff00ff,
	SClrGainsboro      = 0x00dcdcdc,
	SClrGold           = 0x00ffd700,
	SClrGoldenrod      = 0x00daa520,
	SClrGrey           = 0x00808080,
	SClrGreen          = 0x00008000,
	SClrGreenyellow    = 0x00adff2f,
	SClrHoneydew       = 0x00f0fff0,
	SClrHotpink        = 0x00ff69b4,
	SClrIndianred      = 0x00cd5c5c,
	SClrIndigo         = 0x004b0082,
	SClrIvory          = 0x00fffff0,
	SClrKhaki          = 0x00f0e68c,
	SClrLavender       = 0x00e6e6fa,
	SClrLavenderblush  = 0x00fff0f5,
	SClrLemonchiffon   = 0x00fffacd,
	SClrLightblue      = 0x00add8e6,
	SClrLightcoral     = 0x00f08080,
	SClrLightcyan      = 0x00e0ffff,
	SClrLightgoldenrodyellow = 0x00fafad2,
	SClrLightgreen     = 0x0090ee90,
	SClrLightgrey      = 0x00d3d3d3,
	SClrLightpink      = 0x00ffb6c1,
	SClrLightsalmon    = 0x00ffa07a,
	SClrLightseagreen  = 0x0020b2aa,
	SClrLightskyblue   = 0x0087cefa,
	SClrLightslategrey = 0x00778899,
	SClrLightsteelblue = 0x00b0c4de,
	SClrLightyellow    = 0x00ffffe0,
	SClrLime           = 0x0000ff00,
	SClrLimegreen      = 0x0032cd32,
	SClrLinen          = 0x00faf0e6,
	SClrMagenta        = 0x00ff00ff,
	SClrMaroon         = 0x00800000,
	SClrMediumaquamarine = 0x0066cdaa,
	SClrMediumblue     = 0x000000cd,
	SClrMediumorchid   = 0x00ba55d3,
	SClrMediumpurple   = 0x009370db,
	SClrMediumseagreen = 0x003cb371,
	SClrMediumslateblue = 0x007b68ee,
	SClrMediumspringgreen = 0x0000fa9a,
	SClrMediumturquoise = 0x0048d1cc,
	SClrMediumvioletred = 0x00c71585,
	SClrMidnightblue  = 0x00191970,
	SClrMintcream     = 0x00f5fffa, // Очень светло-голубой //
	SClrMistyrose     = 0x00ffe4e1, // Нежно-розовый
	SClrNavajowhite   = 0x00ffdead, // Телесный
	SClrNavy          = 0x00000080, // Темно-синий
	SClrOldlace       = 0x00fdf5e6,
	SClrOlive         = 0x00808000, // ok Оливковый
	SClrOlivedrab     = 0x006b8e23,
	SClrOrange        = 0x00ffa500,
	SClrOrangered     = 0x00ff4500,
	SClrOrchid        = 0x00da70d6,
	SClrPalegoldenrod = 0x00eee8aa,
	SClrPalegreen     = 0x0098fb98,
	SClrPaleturquoise = 0x00afeeee,
	SClrPalevioletred = 0x00db7093,
	SClrPapayawhip    = 0x00ffefd5,
	SClrPeachpuff     = 0x00ffdab9,
	SClrPeru          = 0x00cd853f,
	SClrPink          = 0x00ffc0cb,
	SClrPlum          = 0x00dda0dd,
	SClrPowderblue    = 0x00b0e0e6,
	SClrPurple        = 0x00800080,
	SClrRed           = 0x00ff0000,
	SClrRosybrown     = 0x00bc8f8f,
	SClrRoyalblue     = 0x004169e1,
	SClrSaddlebrown   = 0x008b4513,
	SClrSalmon        = 0x00fa8072,
	SClrSandybrown    = 0x00f4a460,
	SClrSeagreen      = 0x002e8b57,
	SClrSeashell      = 0x00fff5ee,
	SClrSienna        = 0x00a0522d,
	SClrSilver        = 0x00c0c0c0,
	SClrSkyblue       = 0x0087ceeb,
	SClrSlateblue     = 0x006a5acd,
	SClrSlategrey     = 0x00708090,
	SClrSnow          = 0x00fffafa,
	SClrSpringgreen   = 0x0000ff7f,
	SClrSteelblue     = 0x004682b4,
	SClrSteelblue4    = 0x00306080,
	SClrTan           = 0x00d2b48c,
	SClrTeal          = 0x00008080,
	SClrThistle       = 0x00d8bfd8,
	SClrTomato        = 0x00ff6347,
	SClrTurquoise     = 0x0040e0d0,
	SClrViolet        = 0x00ee82ee,
	SClrWheat         = 0x00f5deb3,
	SClrWhite         = 0x00ffffff,
	SClrWhitesmoke    = 0x00f5f5f5,
	SClrYellow        = 0x00ffff00,
	SClrYellowgreen   = 0x009acd32
};
//
// Descr: Структура для унификации сторонних библиотек
//
struct SColorRGB {
	int    IsZero() const
	{
		return (!R && !G && !B);
	}
	SColorRGB Set(uint r, uint g, uint b)
	{
		R = r;
		G = g;
		B = b;
		return *this;
	}
	SColorRGB FASTCALL Set(uint v)
	{
		R = v;
		G = v;
		B = v;
		return *this;
	}
	uint8  R;
	uint8  G;
	uint8  B;
};
//
// Descr: Представление цвета.
//   Эта структура не должна иметь конструкторов и деструктора из-за того,
//   что используется в объединениях.
//
//   Бинарное представление соответствует типу ARGB32.
//
//   В большинстве случаев следует использовать SColor, порожденный от SColorBase.
//
struct SColorBase { // @persistent
	float RedF() const;
	float GreenF() const;
	float BlueF() const;
	float AlphaF() const;
	float OpacityF() const;

	operator COLORREF() const;
	operator RGBQUAD() const;
	int    IsEmpty() const
	{
		return (!B && !G && !R && !Alpha);
	}
	SColorBase Set(uint r, uint g, uint b);
	SColorBase FASTCALL Set(uint v);
	SColorBase PremultiplyAlpha();
	enum {
		fmtHEX                 = 1, // #rrggbb
		fmtRGB                 = 2, // rgb(rr, gg, bb)
		fmtRgbHexWithoutPrefix = 3, // rrggbb
		fmtName                = 0x1000
	};
	int    FASTCALL FromStr(const char *);
	SString & ToStr(SString & rBuf, int format = fmtHEX) const;
	//
	// Порядок полей существенен. Эквивалентен RGBQUAD (+alpha).
	//
	uint8  B;
	uint8  G;
	uint8  R;
	uint8  Alpha;
};

struct SColor : public SColorBase {
	//
	// Descr: @ctr Создает экземпляр черного непрозрачного цвета (Alpha = 0xff, RGB = 0x000000)
	// Note: Специальная глобальная константа ZEROCOLOR содержит неопределенное значение цвета (Alpha = 0x00, RGB = 0x000000)
	//
	SColor();
	SColor(const SColorBase & rS);
	//
	// Descr: Серый цвет с долей белого whitePart.
	//
	SColor(float whitePart);
	SColor(uint r, uint g, uint b);
	SColor(uint r, uint g, uint b, uint alpha);
	SColor(SColourCollection c);
	SColor(COLORREF c);
	SColor & FASTCALL operator = (const SColorBase & rS);

	SColor FASTCALL Lighten(float factor) const;
	SColor FASTCALL Darken(float factor) const;
};

extern const SColor ZEROCOLOR;

COLORREF FASTCALL GetColorRef(SColourCollection);
COLORREF GetGrayColorRef(float whitePart);
COLORREF LightenColor(COLORREF col, double factor);
COLORREF DarkenColor(COLORREF col, double factor);

#define MULTALPHA(a,c) (uint8)(((((a)*(c))+0x80)+((((a)*(c))+0x80)>>8))>>8)
#define PREMULTIPLY_ALPHA_ARGB32(c) { uint32 a = (((c)&0xff000000)>>24); \
	(c) = (a==0xff) ? (c) : (a ? (((a<<24)|(MULTALPHA(a, (((c)&0x00ff0000)>>16))<<16)|(MULTALPHA(a, (((c)&0x0000ff00)>>8))<<8)|(MULTALPHA(a, (((c)&0x000000ff)))))) : 0); }

//
// Descr: Реализует механизм сериализации данных.
//   Использует виртуальный метод DataType::Serialize()
//
class SSerializeContext {
public:
	enum {
		//
		// Если установлен, то описание структур не записываются в поток
		// вместе с данными. В этом случае клиент класса должен самостоятельно
		// реализовать запись и восстановление описаний структур.
		//
		// Это флаг следует применять в том случае, если считываться данные
		// из потока будут в произвольном порядке, а не в той последовательности,
		// в которой записывались.
		//
		fSeparateDataStruct = 0x0001
	};
	SLAPI  SSerializeContext();
	SLAPI ~SSerializeContext();
	int    SLAPI Init(long flags, LDATE suppDate);
	//
	// Descr: Записывает (считывает) состояние объекта в (из) буфер rBuf.
	//   Если Flags & fSeparateDataStruct, то вместе с состоянием сериализуется и
	//   список структур данных. Если этот флаг не установлен, то считается что
	//   структуры сохраняются вместе с первым экземпляром данных, соответствующим
	//   этой структуре.
	// Note: При считывании состояния (dir < 0) если fSeparateDataStruct установлен,
	//   предварительно до чтения разрушается внутренний кэш символов и список
	//   структур.
	//
	int    SLAPI SerializeState(int dir, SBuffer & rBuf);

	int    SLAPI Serialize(const char * pDbtName, BNFieldList * pFldList, const void * pData, SBuffer & rBuf);
	//
	// Descr: Считывает из буфера rBuf запись таблицы БД в соответствии с внутренним описанием (в потоке)
	//   структуры данных. Данные считываются в буфер, на который указывает pData.
	// Note: Если pFldList == 0 тогда просто указатель считывания перемещается в конец
	//   записи, но по указателю pData ничего не записывается.
	//
	int    SLAPI Unserialize(const BNFieldList * pFldList, void * pData, SBuffer & rBuf);

	int    SLAPI Serialize(int dir, TYPEID typ, void * pData, uint8 * pInd, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, SString & rStr, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, SStringU & rStr, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, int64 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, int32 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, int16 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, int8 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, int & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, uint & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, uint64 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, uint32 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, uint16 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, uint8 & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, LDATE & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, LTIME & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, LDATETIME & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, DateRange & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, float & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, double & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, S_GUID & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, char * pV, size_t valBufLen, SBuffer & rBuf);
	int    SLAPI SerializeFieldList(int dir, BNFieldList * pFldList, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, TPoint & rV, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, FPoint & rV, SBuffer & rBuf);

	int    SLAPI Serialize(int dir, SArray * pArray, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, SStrCollection * pColl, SBuffer & rBuf);
	int    SLAPI Serialize(int dir, StrAssocArray & rArray, SBuffer & rBuf);
	int    SLAPI SerializeBlock(int dir, uint32 size, void * pData, SBuffer & rBuf, int skipMissizedBlock);
	//
	// Descr: Возвращает опорную дату.
	//   Опорная дата используется для сжатого хранения типа LDATE.
	//   В поток заностися разница в днях между сохраняемой датой и опорной датой.
	//
	LDATE  SLAPI GetSupportingDate() const;
private:
	int    SLAPI AddDbtDescr(const char * pName, const BNFieldList * pList, uint32 * pID);
	int    SLAPI GetDbtDescr(uint id, BNFieldList * pList) const;

	long   State;
	long   Flags;
	uint32 LastSymbId;
	LDATE  SuppDate;   // Опорная дата.
	SymbHashTable SymbTbl;
	SCollection * P_DbtDescrList;
	SString TempBuf;
	STempBuffer TempDataBuf;
};

class SSerializer {
public:
	SLAPI  SSerializer(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    SLAPI Serialize(TYPEID typ, void * pData, uint8 * pInd);
	int    FASTCALL Serialize(SString & rStr);
	int    FASTCALL Serialize(SStringU & rStr);
	int    FASTCALL Serialize(int64 & rV);
	int    FASTCALL Serialize(int32 & rV);
	int    FASTCALL Serialize(int16 & rV);
	int    FASTCALL Serialize(int8 & rV);
	int    FASTCALL Serialize(int & rV);
	int    FASTCALL Serialize(uint64 & rV);
	int    FASTCALL Serialize(uint32 & rV);
	int    FASTCALL Serialize(uint16 & rV);
	int    FASTCALL Serialize(uint8 & rV);
	int    FASTCALL Serialize(LDATE & rV);
	int    FASTCALL Serialize(LTIME & rV);
	int    FASTCALL Serialize(LDATETIME & rV);
	int    FASTCALL Serialize(float & rV);
	int    FASTCALL Serialize(double & rV);
	int    FASTCALL Serialize(S_GUID & rV);
	int    SLAPI Serialize(char * pV, size_t valBufLen);
	int    SLAPI SerializeFieldList(BNFieldList * pFldList);
	int    FASTCALL Serialize(TPoint & rV);
	int    FASTCALL Serialize(FPoint & rV);
	int    FASTCALL Serialize(SArray * pArray);
	int    FASTCALL Serialize(SStrCollection * pColl);
	int    FASTCALL Serialize(StrAssocArray & rArray);
	int    SLAPI SerializeBlock(uint32 size, void * pData, int skipMissizedBlock);
private:
	int    Dir;
	SBuffer & R_Buf;
	SSerializeContext * P_SCtx;
};
//
// Descr: Специализированная функция для сериализации шаблонизщированных массивов, элементы которых
//   поддерживают невиртуальный метод Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
//
template <class T> int TSArray_Serialize(TSArray <T> & rList, int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 c = 0;
	if(dir > 0) {
		c = rList.getCount();
		THROW(pSCtx->Serialize(dir, c, rBuf));
		for(uint i = 0; i < c; i++) {
			T & r_item = rList.at(i);
			THROW(r_item.Serialize(dir, rBuf, pSCtx));
		}
	}
	else if(dir < 0) {
		rList.clear();
		THROW(pSCtx->Serialize(dir, c, rBuf));
		for(uint i = 0; i < c; i++) {
			T  item;
			THROW(item.Serialize(dir, rBuf, pSCtx));
			THROW(rList.insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

template <class T> int TSCollection_Serialize(TSCollection <T> & rC, int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		uint32 c = rC.getCount();
		THROW(rBuf.Write(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
			T * p_item = rC.at(i);
			THROW(p_item->Serialize(dir, rBuf, pSCtx))
		}
	}
	else if(dir < 0) {
		rC.freeAll();
		uint32 c = 0;
		THROW(rBuf.ReadV(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
            T * p_new_item = rC.CreateNewItem();
			THROW(p_new_item);
			THROW(p_new_item->Serialize(dir, rBuf, pSCtx));
		}
	}
	CATCHZOK
	return ok;
}
//
// Descr: Управление Ini-файлами
//
#ifndef _WIN32_WCE // {

class SIniFile {
public:
	SLAPI  SIniFile(const char * pFileName, int fcreate = 0, int winCoding = 0, int useIniBuf = 0);
	SLAPI ~SIniFile();
	int    SLAPI IsValid() const; // Вызывать после конструктора
	long   SLAPI GetFlags() const;
	long   SLAPI SetFlag(long f, int set);
	const  SString & SLAPI GetFileName() const;
	int    SLAPI GetSections(StringSet * pSects);
	int    SLAPI IsSectExists(const char * pSect);
	int    SLAPI GetEntries(const char * pSect, StringSet * pEntries, int storeAllString = 0);
	// @v9.2.10 int    SLAPI GetParam(const char * pSect, const char * pParam, char *, size_t);
	int    SLAPI GetParam(const char * pSect, const char * pParam, SString & rBuf);
	int    SLAPI GetIntParam(const char * pSect, const char * param, int *);

	int    SLAPI AppendParam(const char * pSect, const char * pParam, const char * pVal, int overwrite);
	int    SLAPI AppendIntParam(const char * pSect, const char * pParam, int val, int overwrite = 1);
	int    SLAPI RemoveParam(const char * pSect, const char * param);
	int	   SLAPI RemoveSection(const char * pSect);
	int    SLAPI ClearSection(const char * pSect);
	int    SLAPI FlashIniBuf(); // Saves buf to file
	//
	// Descr: Определяет изменилась ли дата модификации файла с момента
	//   открытия его экземпляром этого класса.
	// Returns:
	//   1 - файл был модифицирован
	//   2 - файл был удален (не удалось получить по нему статистику)
	//   0 - время модификации файла меньше или равно времени полседнего открытия экземпляром класса //
	//
	int    SLAPI WasModified() const;

	enum {
		fWinCoding    = 0x0001,
		fIniBufInited = 0x0002
	};
protected:
	int    SLAPI Init(const char * pFileName, int fcreate, int winCoding, int useIniBuf);

	long   Flags;
	SString TempBuf;
private:
	int    SLAPI Open(const char * pFileName);
	int    SLAPI Close();
	int    SLAPI Create(const char * pFileName);
	int    SLAPI SearchParam(const char * pSect, const char * pParam, SString & rVal);
	int    SLAPI InitIniBuf();
	int    SLAPI IsSection(const SString & rLineBuf, const char * pPattern, SString * pRet);
	//
	// Descr: intermediate-функция, реализующая вставку, удаление параметра, а также удаление целой секции.
	// ARG(pSect     IN):
	// ARG(pParam    IN):
	// ARG(pVal      IN):
	// ARG(overwrite IN):
	// Returns:
	//
	int    SLAPI SetParam(const char * pSect, const char * pParam, const char * pVal, int overwrite);

	SFile  File;
	SString FileName;
	SStrScan Scan;
	SIniFileBuffer * P_IniBuf;
	LDATETIME LoadingTime;
};
//
// Tab File
//
class STab {
public:
	class Row {
	public:
		friend class STab;

		SLAPI  Row();
		SLAPI ~Row();
		//
		// Descr: Возвращает !0 если экземпляр является валидным. То есть,
		//   это - действительно STab::Row и он не был разрушен деструктором.
		//   Валидность проверяется по сигнатуре Sign.
		//
		int    SLAPI IsConsistent() const;
		int    SLAPI Clear();
		int    FASTCALL Add(const char * pStr);
		int    FASTCALL Add(long intVal);
		int    SLAPI Add(double number);
		uint   SLAPI GetCount() const;
		int    SLAPI Get(uint pos, SString & rStr) const;
		int    SLAPI Get(uint pos, double & rNumber) const;
	private:
		int    FASTCALL ToStr(SString & rBuf) const;
		int    FASTCALL FromStr(const char *);

		uint32 Sign;
		LongArray PosList;
		StringSet Set;
	};

	SLAPI  STab();
	STab & SLAPI Clear();
	uint   SLAPI GetCount() const;
	int    SLAPI Find(uint columnPos, const char * pKey, uint * pRowPos) const;
	int    SLAPI Find(uint columnPos, double key, uint * pRowPos) const;
	int    SLAPI Find(uint columnPos, long key, uint * pRowPos) const;
	int    FASTCALL AddRow(const Row &);
	int    SLAPI GetRow(uint pos, Row &) const;
private:
	StrAssocArray Data;
	long   LastRecId;
};
/*
 Структура файла, содержащего таблицы, следующая:

	BEGIN table_name_1
		xxxxx xxxxxx xxxxxxx xxxxxxxx xxxxxx
		xxxxx xxxxxx xxxxxxx xxxxxxxx xxxxxx
		xxxxx xxxxxx xxxxxxx xxxxxxxx xxxxxx
		xxxxx xxxxxx xxxxxxx xxxxxxxx xxxxxx
		xxxxx xxxxxx xxxxxxx xxxxxxxx xxxxxx
	END

	BEGIN table_name_2
		//
		// Comment string
		//
		yyyyy yyyyyy yyyyyyy yyyyyyyy yyyyyy
		yyyyy yyyyyy yyyyyyy yyyyyyyy yyyyyy
		yyyyy yyyyyy yyyyyyy yyyyyyyy yyyyyy
		yyyyy yyyyyy yyyyyyy yyyyyyyy yyyyyy
		yyyyy yyyyyy yyyyyyy yyyyyyyy yyyyyy
	END

	То есть, собственно данные обрамлены конструкцией BEGIN END, определяющей
	начало и конец именованной таблцы.
	Данные должны разделяться пробелами или точками с запятой (;).
	Строки могут быть обрамлены кавычками (не обязательно, однако полезно
	с целью избежать путаницы между разделителями полей и содержимым строки).

	В файле допускаются комментарии в стиле c++.
*/
class STabFile {
public:
	SLAPI  STabFile();
	SLAPI  STabFile(const char * pFileName, int updateMode = 0);
	int    SLAPI IsValid() const;
	int    SLAPI Open(const char * pFileName, int updateMode);
	int    SLAPI Close();
	int    SLAPI GetTabList(StringSet * pResult);
	int    SLAPI LoadTab(const char * pTabName, STab & rTab);
	//
	// Descr: Записывает таблицу pTag с именем pTagName в файл.
	//   Если pTag == 0 и таблица с именем pTabName в файле
	//   существует, то она удаляется.
	//
	int    SLAPI WriteTab(const char * pTabName, STab * pTab);
private:
	int    SLAPI Helper_WriteTab(const char * pTabName, STab * pTab, SFile & rFile);

	enum {
		fUpdateMode = 0x0001
	};
	long   Flags;
	SFile  F;
};

#ifndef _WIN32_WCE // {
//
// Descr: Вспомогательный класс для генерации C++ кода
//
class Generator_CPP : public SFile {
public:
	enum {
		clsClass = 0,
		clsStruct,
		clsUnion,
		clsEnum,     // Только для Wr_StartClassDecl()
		clsInterface // Только для IDL
	};
	enum {
		acsPublic = 0,
		acsProtected,
		acsPrivate
	};
	enum {
		fkOrdinary = 0,
		fkConstr,
		fkDestr
	};
	enum {
		fmVirtual = 0x0001,
		fmStatic  = 0x0002
	};
	enum {
		fcmDefault = 0,
		fcmCDecl,
		fcmStdCall,
		fcmFastCall
	};
	Generator_CPP(const char * pFileName);
	int    Open(const char * pFileName);
	int    Wr_Include(const char * pFileName, int quot = 0);
	int    Wr_Define(const char * pMacro, const char * pVal);
	int    Wr_IfDef(const char * pSymb, int _ifndef = 0);
	int    Wr_EndIf(const char * pSymb);
	int    Wr_ClassPrototype(int cls, const char * pName);
	int    Wr_StartClassDecl(int cls, const char * pName, const char * pBase, int acs = acsPublic, uint declAlignment = 0);
	int    Wr_StartIdlInterfaceDecl(const char * pName, int dispIface = 0);
	int    Wr_StartIdlCoClassDecl(const char * pName);
	int    Wr_ClassAcsZone(int acs);
	int    Wr_OpenBrace();
	int    Wr_CloseBrace(int addSemicolon, const char * pInstanceSymb = 0);
	int    Wr_Return(const char * pVal);
	int    Wr_StartDeclFunc(int funcKind, int funcMod, const char * pRetType, const char * pFuncName, int funcCallMod = 0);
	int    Wr_EndDeclFunc(int semicol, int newLine);
	int    Wr_VarDecl(const char * pType, const char * pName, const char * pDef = 0, int term = 0);
	int    Wr_Comment(const char * pBuf);
	int    Wr_Indent();
	void   IndentInc();
	void   IndentDec();
	SString & CatIndent(SString & rBuf);
	SString & MakeClsfName(const char * pClsName, const char * pMembName, SString & rBuf) const;
private:
	SString & CatCls(int cls, SString & rBuf);
	SString & CatAcs(int acs, SString & rBuf);
	uint   Indent;
	SString TempBuf;
};

#endif // } !_WIN32_WCE
//
//
//
class SPrinting {
public:
	struct PrnInfo {
		enum {
			fDefault = 0x0001,
			fLocal   = 0x0002,
			fNetwork = 0x0004
		};
		long   Flags;
		char   ServerName[64];
		char   PrinterName[64];
		char   ShareName[128];
	};
	static int SLAPI GetListOfPrinters(TSArray <PrnInfo> * pInfoList);
	SPrinting(HWND);
	~SPrinting();
	int    Init(const char * pPort);
	int    PrintImage(const char * pImgPath);
private:
	int    StartDocument();

	int    Top;
	int    DocStarted;
	int    PageStarted;
	COLORREF OldColor;
	HFONT  OldFont;
	HDC    PrinterDc;
	int    ScreenDpiX;
	int    ScreenDpiY;
};

#ifndef _WIN32_WCE // {

int    SLAPI SCheckSystemCredentials(const char * pDomain, const char * pUserName, const char * pPw);
//
// Descr: Возвращает ЛОКАЛЬНОЕ время с сервера pServerName.
// Returns:
//   >0 - время на сервере pServerName успешно определено и присвоено по указателю pDtm
//   <0 - указатель pServerName нулевой либо пустой (pServerName[0] == 0)
//   0  - ошибка. Если ошибка системная, то SLibError содержит ссылку на системную ошибку,
//        а SLS.GetTLA().LastOsErr содержит код этой ошибки.
//
int    SLAPI SGetTimeFromRemoteServer(const char * pServerName, LDATETIME * pDtm);
int    SLAPI SGetComputerName(SString & rName);
//
// Descr: Класс-обертка для работы с динамическими библиотеками (DLL).
// Example:
//
//	{
//		SDynLibrary lib("somelib.dll");
//		if(lib.IsValid()) {
//			SOME_FUNC_PTR p_func = (SOME_FUNC_PTR)lib.GetProcAddr("SomeFunc");
//			if(p_func) {
//				p_func();
//			}
//		}
//	}
//
class SDynLibrary {
public:
	SLAPI  SDynLibrary(const char * pFileName = 0);
	SLAPI ~SDynLibrary();
	int    SLAPI IsValid() const;
	int    FASTCALL Load(const char * pFileName);
	FARPROC SLAPI GetProcAddr(const char * pProcName, int unicodeSuffix = 0);
private:
	HMODULE H;
};
//
//
//
class WinRegValue {
public:
	friend class WinRegKey;

	SLAPI  WinRegValue(size_t bufSize = 0);
	SLAPI ~WinRegValue();
	int    SLAPI  Alloc(size_t);
	uint   SLAPI  GetType() const { return Type; }
	uint32 SLAPI  GetDWord() const;
	const  void * SLAPI GetBinary(size_t * pDataLen) const;
	const  char * SLAPI GetString() const;
	int    SLAPI  PutDWord(uint32);
	int    SLAPI  PutBinary(const void * pBuf, size_t dataSize);
	int    SLAPI  PutString(const char * pStr);
private:
	uint   Type;
	void * P_Buf;
	size_t BufSize;
	size_t DataSize;
};

class WinRegKey {
public:
	SLAPI  WinRegKey();
	SLAPI  WinRegKey(HKEY, const char * pSubKey, int readOnly);
	SLAPI ~WinRegKey();
	//
	// Descr: Удаляет ветку реестра key\\pSubKey.
	//   Ветку открывать перед удалением не следует.
	//
	int    SLAPI Delete(HKEY key, const char * pSubKey); // @>>SHLWAPI.SHDeleteKey
	//
	// Descr: Удаляет параметр pValue из реестра key\\pSubKey\\
	//   Ветку открывать перед удалением не следует.
	//
	int    SLAPI DeleteValue(HKEY key, const char * pSubKey, const char * pValue);
	int    SLAPI Open(HKEY, const char * pSubKey, int readOnly, int onlyOpen = 0);
	int    SLAPI Close();
	int    SLAPI GetDWord(const char * pParam, uint32 * pVal);
	int    SLAPI GetString(const char * pParam, char * pBuf, size_t bufLen);
	int    SLAPI GetString(const char * pParam, SString & rBuf);
	int    SLAPI GetBinary(const char * pParam, void * pBuf, size_t bufLen);
	int    SLAPI GetRecSize(const char * pParam, size_t * pRecSize);
	int    SLAPI EnumValues(uint * pIdx, SString * pParam, WinRegValue * pVal);
	int    SLAPI EnumKeys(uint * pIdx, SString & rKey);
	int    SLAPI PutDWord(const char * pParam, uint32 val);
	int    SLAPI PutString(const char * pParam, const char *);
	int    SLAPI PutBinary(const char * pParam, const void * pBuf, size_t bufLen);
	int    SLAPI PutValue(const char * pParam, const WinRegValue *);
private:
	HKEY   Key;
};
//
// Descr: Реализует управление сервисами Windows
//
class WinService {
public:
	static int SLAPI Install(const char * pServiceName, const char * pDisplayName, const char * pModuleName,
		const char * pLoginName, const char * pPassword);
	static int SLAPI Uninstall(const char * pServiceName);
	static int SLAPI Start(const char * pServiceName, int stop = 0);

	SLAPI  WinService(const WinServiceMngr &, const char * pServiceName, long desiredAccess = SERVICE_ALL_ACCESS);
	SLAPI ~WinService();
	SLAPI  operator SC_HANDLE () const { return H; }
	int    SLAPI IsValid() const;
	const SString & SLAPI GetName() const;
	int    SLAPI Create(const char * pDisplayName, const char * pModuleName, const char * pLogin, const char * pPw);
	//
	// Descr: Удаляет сервис из базы данных сервисов.
	//   Для успешного выполнения этой функции необходимо создать экземпляр класса
	//   с параметром desiredAccess = DELETE.
	//   Если функция завершилась успешно, то хандлер H закрывается и никакие другие
	//   операции с сервисом недоступны.
	// Returns:
	//   !0 - сервис успешно удален
	//   0  - ошибка
	//
	int    SLAPI Delete();
	//
	// Descr:
	//   desiredAccess = SERVICE_START
	//
	int    SLAPI Start();
	//
	// Descr:
	//   desiredAccess = SERVICE_STOP
	//
	int    SLAPI Stop();
private:
	const  WinServiceMngr * P_ScMngr;
	SC_HANDLE H;
	SString Name;
};
//
// Descr: Интерфейс с подсистемой Windows Management Instrumentation (WMI)
//
interface IWbemLocator;
interface IWbemServices;
interface IWbemClassObject;

class SWmi {
public:
	SLAPI  SWmi();
	SLAPI ~SWmi();
	int    SLAPI Connect(const char * pServer = 0, const char * pUserName = 0, const char * pPassword = 0);
	int    SLAPI Method_CreateProcess(const char * pCmdLine);
	int    SLAPI GetMethodList(IWbemClassObject * pCls, StrAssocArray * pList);
private:
	void   SLAPI Release();
	int    SLAPI GetSvcError(SString & rBuf);
	enum {
		sConnected	= 0x0001
	};
	long   State;
	IWbemLocator  * P_Loc;
	IWbemServices * P_Svc;
};
//
// Descr: Атомарный счетчик
//
class ACount { // @#size=4
public:
	SLAPI  ACount()	{C = 0;}
	//
	// Descr: Этот конструктор используется тогда, когда
	//   не следует явно инициализировать переменную в ноль.
	//
	SLAPI  ACount(int) {}
	SLAPI  operator long() const { return C; }
	long   SLAPI Add(long add) { ::InterlockedExchangeAdd(&C, add); return C; }
	long   SLAPI Assign(long val) { InterlockedExchange(&C, val); return C; }
	long   SLAPI Incr() { return ::InterlockedIncrement(&C); }
	long   SLAPI Decr() { return ::InterlockedDecrement(&C); }
private:
	long   C;
};

/*
class ACount64 { // @#size=4
public:
	SLAPI  ACount64()	{C = 0;}
	//
	// Descr: Этот конструктор используется тогда, когда
	//   не следует явно инициализировать переменную в ноль.
	//
	SLAPI  ACount64(int64) {}
	SLAPI  operator int64() const { return C; }
	int64  SLAPI Incr() { return ::InterlockedIncrement64(&C); }
	int64  SLAPI Decr() { return ::InterlockedDecrement64(&C); }
private:
	int64  C;
};
*/
//
// Descr: Критическая секция //
//   Этот класс следует использовать для критических секций, охватывающих одну непрерывную
//   область кода. Пример использования:
//   int foo()
//   {
//       ENTER_CRITICAL_SECTION
//       ... // some code
//       LEAVE_CRITICAL_SECTION
//   }
//
//    Ддя критических секций, охватывающих разделенные участки кода следует использовать
//    класс SCriticalSection::Data
//
class SCriticalSection {
public:
	class Data {
	public:
		SLAPI  Data(int dontDestroy = 0);
		SLAPI ~Data();
		void   SLAPI Enter();
		int    SLAPI TryEnter();
		void   SLAPI Leave();
	private:
		CRITICAL_SECTION C;
		int    DontDestroyOnDestruction;
	};
	SLAPI  SCriticalSection(Data & rCsd) : R_Cs(rCsd)
	{
		R_Cs.Enter();
	}
	SLAPI ~SCriticalSection()
	{
		R_Cs.Leave();
	}
private:
	Data & R_Cs;
};

#ifdef __WIN32__
	typedef HANDLE      MutexHandle;
	typedef MutexHandle MutexHandleRef;
#else
	typedef pthread_mutex_t	MutexHandle;
	typedef MutexHandle * MutexHandleRef;
#endif
//
// Descr: Задерживает исполнение потока на msec миллисекунд
//
void   FASTCALL SDelay(uint msec);
//
//
//
class SWaitableObject {
public:
	SLAPI  SWaitableObject();
	virtual SLAPI ~SWaitableObject();
	int    SLAPI IsValid() const;
	SLAPI  operator HANDLE() const { return H; }
	int    FASTCALL operator == (const SWaitableObject &) const;
	//
	// Descr: If the object is in a signaled state, cosume it without waiting.
	//   If the event is in a reset state, wait timeout millisec (-1 - infinitely).
	// Returns:
	//   >0 - объект был освобожден
	//   <0 - ожидание прекратилось по таймауту
	//   0  - ошибка
	//
	int    FASTCALL Wait(long timeout = -1);
protected:
	HANDLE H;
};
//
//
//
class DirChangeNotification : public SWaitableObject {
public:
	SLAPI  DirChangeNotification(const char * pName, int watchSubtree, long filtFlags);
	virtual SLAPI ~DirChangeNotification();
	int    SLAPI Next();
};
//
//
//
class SMutex : public SWaitableObject {
public:
	SLAPI  SMutex(int initialValue, const char * pName = 0);
	int    SLAPI Release();
};
//
//
//
class STimer : public SWaitableObject {
public:
	SLAPI  STimer(const char * pName = 0);
	int    SLAPI Set(const LDATETIME & rDtm, long periodTime);
	int    SLAPI Cancel();
};
//
//
//
class Evnt : public SWaitableObject {
public:
	enum {
		modeCreate = 1,
		modeCreateAutoReset, // Создает событие с автосбросом. То есть такое, что после успешного
			// завершения ожидания этого события, оно автоматически переводится в занятое состояние.
		modeOpen
	};
	//
	// Descr: Создает или открывает именованный объект события. Событие создается в занятом состоянии.
	// ARG(pName     IN): @#{vptr} Создает именованное событие
	// ARG(mode      IN): modeXXX
	//
	SLAPI  Evnt(const char * pName, int mode = modeCreate);
	SLAPI  Evnt(int mode = modeCreate);
	//
	// Descr: Переводит событие в занятое состояние
	//
	int    SLAPI Signal();
	//
	// Descr: Переводит событие в свободное состояние
	//
	int    SLAPI Reset();
};
//
//
//
class Sem : public SWaitableObject {
public:
	enum {
		modeCreate = 1,
		modeOpen   = 3
	};
	SLAPI  Sem(const char * pName, int mode = modeCreate, int initVal = 0);
	SLAPI  Sem(int initVal = 0);
	int    SLAPI Release(int count = 1);
};
//
// Incrementing is non-blocking, decrementing is blocking.
// Blockig condition: Count == 0.
//
class BlockingCounter {
public:
	//
	// Descr: A BlockingCounter instance may be initialized only with a non-negative value
	//
	SLAPI  BlockingCounter();
	int    SLAPI Value() const;
	int    SLAPI IsClear() const;
	//
	// Descr: Blocks until the counter is clear
	//
	int    SLAPI WaitUntilClear();
	//
	// Descr: Blocks until the counter is dirty
	//
	int    SLAPI WaitUntilDirty();
	//
	// Descr: Non-blocking increment
	//
	int    SLAPI Increment();
	//
	// Descr: Blocking decrement
	//
	int    SLAPI BlockingDecrement();
	BlockingCounter & SLAPI operator++ ();
	BlockingCounter & SLAPI operator-- ();
protected:
	int    Count;
	SMutex ExclusiveAccess;
	Evnt ClearEvent;
	Evnt DirtyEvent;
};
//
// Class msdk::SemiMutex
//
class SemiMutex {
public:
	//
	// Descr: Share execution with other "readers".
	//
	int    SLAPI ReadLock();
	//
	// Descr: Terminate a shared execution.
	//
	int    SLAPI ReadUnlock();
	//
	// Descr: Exclusive lock
	//
	int    SLAPI Lock();
	//
	// Descr: Release an exclusive lock
	//
	int    SLAPI Unlock();
protected:
	SMutex ExclusiveAccess;
	BlockingCounter ReadAccess;
};
//
// Descr: Простая блокировка, построенная на критической секции
//
class SMtLock {
public:
	SMtLock() : Cs(0)
	{
	}
	void   SLAPI Lock() { Cs.Enter(); }
	void   SLAPI Unlock() { Cs.Leave(); }
private:
	SCriticalSection::Data Cs;
};
//
//
//
class ReadWriteLock {
public:
	SLAPI  ReadWriteLock();
	SLAPI ~ReadWriteLock();
	//
	// Descr: Блокирует объект на чтение с ожиданием в течении timeout миллисекунт.
	// Returns:
	//   >0 - блокировка успешно установлена
	//   <0 - блокировка не установлена из-за превышения ожидания timeout ms
	//   0  - ошибка
	//
	int    FASTCALL ReadLockT(long timeout);
	int    SLAPI ReadLock();
	int    SLAPI WriteLock();
	int    SLAPI Unlock();
private:
	int    FASTCALL Helper_ReadLock(long timeout);

	SCriticalSection::Data Cs;
	int    ActiveCount; // Текущее число потоков, работающих с ресурсом
		// 0 - нет, >0 - читатели, <0 - писатели
	int    Dr; // Number of delayed readers
	int    Dw; // Number of delayed writers
	Sem    Sr; // Семафор для приостановки читателей
	Sem    Sw; // Семафор для приостановки писателей
};
//
// Descr: Пул строк. Главное назначение класса - предоставлять клиентам экземпляры SString
//   с уже распределенным пространством под буферы. Это значительно ускоряет работу для //
//   модулей, которые постоянно используют короткоживущие строки (экономия достигается за
//   счет того, что однажды распределеннаый буфер строки SString не сокращается, следовательно
//   не расходуется время на динамическое выделение памяти).
//
class SStringPool : public TSCollection <SString> {
public:
	SStringPool();
	~SStringPool();
	SString * FASTCALL Alloc(uint * pPos);
	int    FASTCALL Free(uint pos);
	int    FASTCALL Free(const SString *);
	int    FASTCALL Free(const BitArray &);
	const  BitArray & GetMap() const;
private:
	BitArray BusyList;
};
//
//
//
class SProfile {
public:
	SLAPI  SProfile(int singleThreaded = 0);
	SLAPI ~SProfile();
	uint64 SLAPI GetAbsTimeMicroseconds(); // @cs
protected:
	uint64 SLAPI Helper_GetAbsTimeMicroseconds();

	const  int SingleThreaded;
	int64  StartClock; // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	int64  EndClock;   // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	uint64 ClockFrequency; // result of QueryPerformanceFrequency()
	//
	// Descr: Специализированный блок переменных, используемых только функцией Helper_GetAbsTimeMicroseconds
	//
	struct GetTimeBlock {
		uint64 StartHrc;       // start of QueryPerformanceCounter()
		uint64 PrevHrc;        //
		uint32 StartTick;      // start of GetTickCount()
	};

	GetTimeBlock Gtb;
	SMtLock Lck;
};
//
// Descr: Вспомогательные классы, реализующие автоматическое
//   создание и разрушение объектов, принадлежащих иным классам.
//
class SClassWrapper {
public:
	virtual void * Create() = 0;
	virtual void Destroy(void *) = 0;
};

template <class T> class TSClassWrapper : public SClassWrapper {
	virtual void * Create() { return new T; }
	virtual void Destroy(void * ptr) { delete (T *)ptr; }
};
//
//
//
typedef HANDLE ThreadHandle;
typedef DWORD  ThreadID;
#ifdef _MT
	typedef uint  ThreadProcReturn;
#else
	typedef DWORD ThreadProcReturn;
#endif
#define THREADPROCCALL __stdcall
#define SLTHREAD_DEFAULT_STOP_TIMEOUT 30000

class SlThread {
public:
	//
	// Descr: Initialize the data members without creating an execution thread
	//
	SLAPI  SlThread(void * pInitData = 0, long stopTimeout = SLTHREAD_DEFAULT_STOP_TIMEOUT);
	virtual SLAPI ~SlThread();
	int    SLAPI IsConsistent() const;
	operator HANDLE() const
	{
		return Handle;
	}
	//
	// Descr: Создает поток и запускает его на исполнение.
	//   Если waitOnStartup != 0, то ожидает сигнала от события P_StarupSignal.
	//   Либо Startup(), либо Run() должна сигнализировать, что
	//   родительский процесс может продолжать работу.
	//   Объект P_StartupSignal, если необходимо, должен быть создан
	//   в конструкторе класса.
	//   P_StartupSignal после завершения ожидания разрушается.
	//
	int    FASTCALL Start(int waitOnStartup = 0);
	//
	// Descr: Пытается аккуратно остановить поток в течении StopTimeout миллисекунд.
	//   Если это не удалось, то грубо обрывает поток вызовом Terminate().
	//   Функция устанавливает флаг статуса stStop по которому функции потока
	//   могут определить, что выставлено требование на остановку.
	//
	int    SLAPI Stop();
	//
	// Descr: Terminate the execution thread brutally
	//
	int    SLAPI Terminate();
	int    SLAPI WaitUntilFinished();
	//
	// Descr: This implementation is guaranteed to be cancelable.
	//
	void   FASTCALL Sleep(uint milliseconds);
	void   SLAPI SetStopState();
	ThreadID SLAPI GetThreadID() const;
	int    SLAPI IsRunning() const;
	int    SLAPI IsIdle() const;
	//
	// Descr: Если поток находится в состоянии остановки, то возвращает !0,
	//   в противном случае - 0.
	//
	int    SLAPI IsStopping() const;
protected:
	//
	// Descr: This method is invoked on behalf of the new thread before Run()
	//
	virtual void SLAPI Startup();
	//
	// Descr: This method is invoked on behalf of the dying thread after Run()
	//
	virtual void SLAPI Shutdown();
	//
	// Descr: This represents the main thread proc of the thread. It is invoked
	//   after Startup() and before Shutdown(). The default implementation is void.
	//
	virtual void SLAPI Run();
	void   SLAPI Reset(void * pInitData = 0, int withForce = 0, long stopTimeout = SLTHREAD_DEFAULT_STOP_TIMEOUT);
	int    SLAPI InitStartupSignal();
	int    SLAPI SignalStartup();

	volatile void * P_InitData;
	Evnt   EvLocalStop; // Событие активируется при вызове SlThread::SetStopState()
		// Этот сигнал означает, что поток должен остановиться.
private:
	//
	// Descr: This is each thread's thread proc
	//
	static ThreadProcReturn THREADPROCCALL _Exec(void * pThis);

	uint32 Sign; // Подпись экземпляра класса. Используется для идентификации
		// инвалидных экземпляров.
	enum {
		stRunning   = 0x0001, // Поток запущен
		stIdle      = 0x0002, // Поток находится в состоянии простоя //
		stLocalStop = 0x0004  // Поток получил установку на завершение. Функции, выполняющиеся долгое
			// время могут проверять этот флаг с помощью SlThread::IsStopping()
	};
	volatile long State;
	volatile long StopTimeout; // Таймаут на остановку потока (ms)
	volatile ThreadHandle Handle;
	volatile ThreadID ID;
	Evnt * P_Creation;
	Evnt * P_StartupSignal;
};

#endif // } _WIN32_WCE
//
//
//
#ifndef _WIN32_WCE

struct SGlobalSecureConfig {
	SGlobalSecureConfig()
	{
		Flags = 0;
	}
	int    IsEmpty() const
	{
		return BIN(Flags == 0 && CaFile.Empty() && CaPath.Empty());
	}
	long   Flags;
	SString CaFile;
	SString CaPath;
};

void * FASTCALL SGetTls(const long idx);
//
// Descr: Тип функции, загружающей константные строки из внешнего хранилища.
// ARG(pSignature IN): текстовая сигнатура строки.
// ARG(rBuf      OUT): буфер, в который записывается загруженная строка. Если функция не смогла
//   идентифицировать сигнатуру либо не смогла загрузить строку по другим причинам, она должна
//   обнулить этот буфер присваиванием {rBuf = 0}
// Returns:
//   >0 - функция успешно загрузила строку с сигнатурой pSignature
//   <0 - функция не смогла распознать сигнатуру, или такой сигнатуре не сопоставлено никакой строки
//   0  - ошибка загрузки строки по идентифицированной сигнатуре из внешнего хранилища
//
typedef int (*LoadStringFunc)(const char * pSignature, SString & rBuf);
typedef int (*ExpandStringFunc)(SString & rBuf, int ctransf);
typedef int (*CallHelpFunc)(uint32 hWnd, uint cmd, uint ctx);
typedef int (*CallCalculatorFunc)(uint32 hParentWnd, const char * pInitValue);
typedef int (*CallCalendarFunc)(uint32 hParentWnd, LDATE * pDate);
typedef int (*GetGlobalSecureConfigFunc)(SGlobalSecureConfig * pCfg);
typedef int (*GetDefaultEncrKeyFunc)(SString & rBuf);
//
// Descr: Класс, содержащий функции, определенные вне модуля SLIB
//
class SlExtraProcBlock {
public:
	SlExtraProcBlock();
	void   Reset();
	void   Set(const SlExtraProcBlock * pS);

	LoadStringFunc F_LoadString;
	ExpandStringFunc F_ExpandString;
	CallHelpFunc F_CallHelp;
	CallCalculatorFunc F_CallCalc;
	CallCalendarFunc F_CallCalendar;
	GetGlobalSecureConfigFunc F_GetGlobalSecureConfig;
	GetDefaultEncrKeyFunc F_GetDefaultEncrKey;
};
//
// Descr: Флаги пользовательского интерфейса (SlThreadLocalArea::UiFlags)
//
enum {
	sluifUseLargeDialogs = 0x0001 // Применять диалоги большого размера (ситуативное использование)
};

class SlThreadLocalArea {
public:
	SLAPI  SlThreadLocalArea();
	SLAPI ~SlThreadLocalArea();
	void   SLAPI Destroy();
	int    SLAPI RegisterTempFileName(const char *);
	int    SLAPI RemoveTempFiles();
	//
	// Descr: Возвращает контекст вывода, используемый
	//   для работы с Windows-функциями штрифтов.
	//
	HDC    SLAPI GetFontDC();
	TVRez * SLAPI GetRez();
	//
	// Descr: Устанавливает позицию левого верхнего угла диалогового окна, которое
	//   потенциально может быть открыто. Если left < 0 || top < 0, то при создании
	//   нового окна диалога программа будет считать, что такая позиция не определена.
	//
	void   SLAPI SetNextDialogLuPos(int left, int top);
	//
	// Descr: Возвращает рекомендуемую позицию левого верхнего угла вновь открытого
	//   диалогового окна.
	// Note: Эта функция сразу же сбрасывает значение этого угла в неопределенное.
	//
	TPoint SLAPI GetNextDialogLuPos();

	long   Id;
	int    LastErr;
	int    LastOsErr;       // Код ошибка операционной системы (сохраняет значение в момент
		// инициирования LastErr = SLERR_WINDOWS)
	int    LastSockErr;     // Код последней ошибки WinSock
	int    LastCurlErr;     // @v8.8.1 Код последней ошибки библиотеки LibCURL
	int    BinDateFmt_;
	long   TxtDateFmt_;     // @v8.6.7 int-->long
	int    DefaultYear_;
	int    DefaultMonth_;
	int    UiLanguageId;
	long   UiFlags;            // @v9.1.1 sluifXXX
	SCodepageIdent CurrentCp;  // @v7.8.5
	// @v9.6.5 char   OneCStrBuf[4];      // used in onecstr()
	CompFunc SAry_OrgFCMP;
	int    SAry_PtrContainer;
	void * SAry_SortExtraData;
	SString AddedMsgString; // @anchor
	SString LogPath;        // @v8.0.2
	StringSet TempFileList;
	SRandGenerator Rg;      // Генератор случайных чисел общего пользования. Инициализируется по моменту создания потока.
	SGlobalSecureConfig Gsc; // Конфигурация SSL, шифрования и др. для секьюритизации глобального обмена данными
private:
	TVRez * P_Rez;
	HDC    FontDc;          // Искусственный глобальный контекст вывода для манипулирования шрифтами.
		// (Window-функции, оперирующие со шрифтами требуют HDC с установленным в нем текущим шрифтом).
	TPoint NextDialogLuPos;  // @v8.5.3 Позиция, в которую необходимо установить верхний левый угол диалога.
		// Функция вызова диалога сразу после получения позиции сбрасывает ее в {-1, -1}
};

class SlSession {
public:
	SLAPI  SlSession();
	SLAPI ~SlSession();
	int    SLAPI Construct();
	//
	int    SLAPI Init(const char * pAppName, HINSTANCE hInst = NULL);
	//
	// Descr: Устанавливает наименование приложения, ассоциированное с данным сеансом.
	// Note: В общем случае правильно указывать наименование приложения аргументом функции SlSession::Init,
	//   однако, если это не возможно по каким либо причинам, то это следует сделать вызовом данной функции.
	//
	void   SLAPI SetAppName(const char * pAppName);
	int    SLAPI InitWSA(); // @>>::WSAStartup
	int    SLAPI InitSSL();
	int    SLAPI InitThread();
	void   SLAPI ReleaseThread();
	SlThreadLocalArea & SLAPI GetTLA();
	const  SlThreadLocalArea & SLAPI GetConstTLA() const;
	const SString & SLAPI GetExePath() const;
	const SString & SLAPI GetAppName() const;
	int    SLAPI SetUiLanguageId(int languageId, int currentThreadOnly);
	int    SLAPI GetUiLanguageId() const;
	SString & GetStopEventName(SString & rBuf) const;
	//
	// Descr: Устанавливает флаг StopFlag и сигнализирует
	//   событием GetStopEventName() для того, чтобы все потоки
	//   завершили свою работу.
	// Returns:
	//   >0 - переключение в режим останова успешно реализовано
	//   <0 - класс уже находился в режиме останова
	//
	int    SLAPI Stop();
	//
    HINSTANCE SLAPI GetHInst() const { return H_Inst; }
	//
	// Descr: Отменяет режим останова для процесса
	//
	int    SLAPI ResetStopState();
	//
	// Descr: Проверяет, находится ли класс в режиме останова процесса
	//
	int    SLAPI CheckStopFlag() const;
	void   FASTCALL SetAddedMsgString(const char *);
	//
	// Returns: 0
	//
	int    FASTCALL SetError(int errCode, const char * pAddedMsg);
	//
	// Returns: 0
	//
	int    FASTCALL SetError(int errCode);
	//
	// Returns: 0
	//
	int    FASTCALL SetOsError(const char * pAddedMsg = 0);
	int    SLAPI GetOsError() const;
	const  SString & GetAddedMsgString() const;
	int    SLAPI LogMessage(const char * pFileName, const char * pStr, ulong maxFileSize = 0);
	int    SLAPI InitGdiplus();
	int    SLAPI ShutdownGdiplus();
	int    SLAPI LoadString(const char * pSignature, SString & rBuf) const;
	int    SLAPI ExpandString(SString & rBuf, int ctransf) const;
	int    SLAPI SubstString(const char * pSrcStr, int ansiCoding, SString & rBuf); // @>>this->LoadString
	int    FASTCALL SetCodepage(SCodepage cp);
	SCodepage SLAPI GetCodepage() const;
	int    SLAPI CallHelp(uint32 wnd, uint cmd, uint ctx);
	int    SLAPI RegisterTempFileName(const char *);
	int    SLAPI RemoveTempFiles();
	//
	uint   SLAPI CreateGlobalObject(SClassWrapper & rCls);
	int    SLAPI DestroyGlobalObject(uint idx);
	void * FASTCALL GetGlobalObject(uint idx);
	//
	// Descr: Находит или, если нет, то создает ассоциацию глобального символа pSymb с целочисленным
	//   идентификатором ident.
	//   Если символ pSymb не найден, а ident == 0, то ассоциирует pSymb со значением,
	//   полученным увеличением внутреннего счетчика.
	//   Если символ pSymb не найден, а ident > 0, то ассоциирует pSymb с заданным значением ident.
	//   Если символ pSymb не найден, а ident < 0, то возвращает (<0).
	// Returns:
	//   >0 - идентификатор, ассоциированный с pSymb
	//   <0 - если pSymb не найден, а ident < 0
	//   0  - возможен один из следующих случаев:
	//        -- если pSymb найден, ident > 0, но найденный идентификатор не равен ident
	//        -- если pSymb не найден и не удалось создать новую ассоциацию {pSymb, ident}
	//
	long   SLAPI GetGlobalSymbolId(const char * pSymb, long ident); // @cs
	const  S_GUID & GetSessUuid() const
	{
		return SessUuid;
	}
	//
	// Descr: Возвращает инкрементируемое целочисленное значение, уникальное в пределах данного процесса.
	//
	int64  GetSequenceValue();
	const  SSystem & GetSSys() const
	{
		return SSys;
	}
	//
	// Descr: Устанавливает или снимает (в зависимости от параметра set) флаг пользовательского
	//   интерфейса для текущего потока.
	// Returns:
	//   Предыдущее значение поля флагов пользовательского интерфейса текущего потока.
	//
	long    SLAPI SetUiFlag(long f, int set);
	//
	// Descr: Проверят установлен ли заданный флаг пользовательского интерфейса текущего потока.
	//
	int     FASTCALL CheckUiFlag(long f) const;
	//
	//
	//
	enum {
		ddotNone = 0,
		ddotGlobalFile,
		ddotLocalWhatmanToolArrayItem
	};
	int    SLAPI SetupDragndropObj(int ddoType, void * pObj);
	int    SLAPI IsThereDragndropObj(void ** ppObj);
	//
	const  SGlobalSecureConfig & SLAPI GetGlobalSecureConfig();

	int    SLAPI SetLogPath(const char *);
	SString & SLAPI GetLogPath(SString & rPath) const;
	//
	//
	//
	void   SLAPI GetExtraProcBlock(SlExtraProcBlock * pBlk) const;
	void   SLAPI SetExtraProcBlock(const SlExtraProcBlock * pBlk);
private:
	long   Id;              // @anchor
	long   TlsIdx;          // @anchor
	ACount SeqValue;        // @anchor Последнее значение, возвращенное функций GetSequenceValue().
		// Положение переменной должно быть выровнено по 64-битной границе.
	ACount LastThread;      //
	int    WsaInitCounter;
	ACount SslInitCounter;  // @v8.3.5
	int    StopFlag;
	uint   DragndropObjIdx; // Индекс объекта для перетаскивания в таблице глобальных объектов GlobalObjList
	uint32 HelpCookie;
	Evnt * P_StopEvnt;
	S_GUID SessUuid;        // @v8.0.2 Уникальный идентификатор сессии
	int    UiLanguageId;
	const SSystem SSys;

	SlExtraProcBlock ExtraProcBlk; // @v9.1.2

	SString ExePath;
	SString AppName;
	HINSTANCE H_Inst;
	ULONG_PTR GdiplusToken;

	class GlobalObjectArray : public SArray {
	public:
		GlobalObjectArray();
		~GlobalObjectArray();
		uint CreateObject(SClassWrapper & rCls);
		int  DestroyObject(uint idx);
		void * FASTCALL GetObject(uint idx);
	private:
		SCriticalSection::Data Cs;
	};

	GlobalObjectArray GlobObjList;
	SymbHashTable GlobSymbList;
	ACount LastGlobSymbId;
#ifndef _MT
	SlThreadLocalArea Tla;
#endif
};

extern SlSession SLS;

#define SLibError    (SLS.GetTLA().LastErr)
#define BinDateFmt   (SLS.GetTLA().BinDateFmt_)
#define TxtDateFmt   (SLS.GetTLA().TxtDateFmt_)
#define DefaultYear  (SLS.GetTLA().DefaultYear_)
#define DefaultMonth (SLS.GetTLA().DefaultMonth_)
// @v8.1.11 #define P_SlRez      (SLS.GetTLA().P_Rez)
#define P_SlRez      (SLS.GetTLA().GetRez()) // @v8.1.11
//
// Descr: Интерфейс к некоторым методам библиотеки GDI+ (Microsoft)
//
class SImage {
public:
	SImage();
	~SImage();
	int    Init();
	int    IsValid() const;
	int    LoadImage(const char * pPicPath);
	int    LoadThumbnailImage(const char * pPicPath, int width, int height);
	int    Draw(HWND hWnd, const char * pPicPath, RECT *, int clear = 1, int use2Koeff = 0);
	int    Draw(HWND hWnd, RECT * pRect, int clear = 1, int use2Koeff = 0);
	int    Draw(HDC hdc,   RECT * pRect, int clear = 1, int use2Koeff = 0);
	int    DrawPart(HDC hdc, const RECT * pCliRect, const RECT * pDestRect, const RECT * pImgPart);
	int    DrawPartUnchanged(HDC hdc, int offsX, int offsY, const RECT * pImgPart);
	double GetWidth();
	double GetHeight();
	double GetHRes();
	double GetVRes();
	SString & GetFileName(SString & rBuf);
	void   SetClearColor(COLORREF);
	int    InsertBitmap(HWND hwnd, const char * pPath, COLORREF bkgnd);
private:
	COLORREF ClearColor;
	SString FileName;
	void * P_Image;
};

int    CopyPaste(HWND hWnd, int copy, const char * pPath);
//
//
//
struct _cairo;
struct _cairo_surface;
struct _cairo_pattern;
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;
typedef struct _cairo_font_face cairo_font_face_t;
typedef struct _cairo_scaled_font cairo_scaled_font_t;
//
// Утилитные функции
//
void FASTCALL ZDeleteWinGdiObject(void * pHandle);
//
// Descr: Перечисление, идентифицирующее тип подсистемы графического вывода.
//   Используется классами TCanvas2, SDraw и их окружением.
//
enum SDrawSystem {
	dsysNone = 0,
	dsysWinGdi = 1,
	dsysWinGdiPlus,
	dsysCairo
};

class SDrawContext {
public:
	//
	// Descr: Переводит размер шрифта, заданный в графических поинтах (pt)
	//   в пиксели согласно текущему разрешению экрана.
	//
	static uint CalcScreenFontSizePt(uint pt);

	class UC : public SUnit::Context {
	public:
		UC();
		virtual int Describe(int unitId, int dir, int * pCls, double * pToBase, SString * pName) const;
		FPoint Dpi;       // Разрешение экрана в точках на дюйм
		float  FontSize;
	};

	SDrawContext(cairo_t * pCr);
	SDrawContext(HDC);
	operator SDrawSystem () const
	{
		return S;
	}
	operator cairo_t * () const;
	SDrawContext::UC * GetUnitContext() const;
private:
	SDrawSystem S;
	void * P;
};
//
// Descr: Представление графического порта вывода
//
struct SViewPort : public FRect {
	/*
		Описание вариантов PreservedAspectRation для ViewBox
    	-- none - Do not force uniform scaling. Scale the graphic content of the given
		element non-uniformly if necessary such that the element's bounding box exactly
		matches the viewport rectangle.
      	(Note: if <align> is none, then the optional <meetOrSlice> value is ignored.)
    	-- xMinYMin - Force uniform scaling.
      	Align the <min-x> of the element's viewBox with the smallest X value of the viewport.
      	Align the <min-y> of the element's viewBox with the smallest Y value of the viewport.
    	-- xMidYMin - Force uniform scaling.
      	Align the midpoint X value of the element's viewBox with the midpoint X value of the viewport.
      	Align the <min-y> of the element's viewBox with the smallest Y value of the viewport.
    	-- xMaxYMin - Force uniform scaling.
      	Align the <min-x>+<width> of the element's viewBox with the maximum X value of the viewport.
      	Align the <min-y> of the element's viewBox with the smallest Y value of the viewport.
    	-- xMinYMid - Force uniform scaling.
      	Align the <min-x> of the element's viewBox with the smallest X value of the viewport.
      	Align the midpoint Y value of the element's viewBox with the midpoint Y value of the viewport.
    	-- xMidYMid (the default) - Force uniform scaling.
      	Align the midpoint X value of the element's viewBox with the midpoint X value of the viewport.
      	Align the midpoint Y value of the element's viewBox with the midpoint Y value of the viewport.
    	-- xMaxYMid - Force uniform scaling.
      	Align the <min-x>+<width> of the element's viewBox with the maximum X value of the viewport.
      	Align the midpoint Y value of the element's viewBox with the midpoint Y value of the viewport.
    	-- xMinYMax - Force uniform scaling.
      	Align the <min-x> of the element's viewBox with the smallest X value of the viewport.
      	Align the <min-y>+<height> of the element's viewBox with the maximum Y value of the viewport.
    	-- xMidYMax - Force uniform scaling.
      	Align the midpoint X value of the element's viewBox with the midpoint X value of the viewport.
      	Align the <min-y>+<height> of the element's viewBox with the maximum Y value of the viewport.
    	-- xMaxYMax - Force uniform scaling.
      	Align the <min-x>+<width> of the element's viewBox with the maximum X value of the viewport.
      	Align the <min-y>+<height> of the element's viewBox with the maximum Y value of the viewport.
	*/
	enum {
		parNone = 0,
		parMin,
		parMid,
		parMax
	};
	enum {
		fEmpty         = 0x0001, // ViewPort не определен
		fSlice         = 0x0002, // else Meet
		fDontScale     = 0x0004, // Не масштабировать объект
		fDontEnlarge   = 0x0008, // Не увеличивать объект (можно только уменьшать)
	};

	SViewPort(uint flags = 0);
	enum {
		fmtDefault = 0,
		fmtSVG
	};
	int    FromStr(const char * pStr, int fmt);
	LMatrix2D & GetMatrix(const FRect & rBounds, LMatrix2D & rMtx) const;

	int8   ParX;   // Preserved Aspect Ratio by X-axis
	int8   ParY;   // Preserved Aspect Ratio by Y-axis
	uint16 Flags;  // SDrawFigure::vpfXXX
};
//
// Descr: Реализация универсального буфера изображения.
//
class SImageBuffer : public SBaseBuffer { // @persistent @store(SSerializeContext)
public:
	struct Palette {
		Palette();
		~Palette();
		int    Alloc(uint count);
		uint   GetCount() const;
		size_t GetSize() const;
		void   SetAlpha(uint8 alpha);
		void   SetRGB(size_t idx, uint8 r, uint8 g, uint8 b);
		uint32 FASTCALL GetColor(uint idx) const;
		//
		void * GetBuffer(); // really private
		const  uint32 * GetBufferC() const; // really private
	private:
		uint   Count;
		uint32 * P_Buf;
	};
	struct PixF {
		enum {
			s32ARGB = 0,  // (uniform) Specifies that the format is 32 bits per pixel; 8 bits each are used for the alpha, red, green, and blue components.
			s16ARGB1555,  // 16 bits per pixel. The color information specifies 32,768 shades of color, of which 5 bits are red, 5 bits are green, 5 bits are blue, and 1 bit is alpha.
			s16GrayScale, // 16 bits per pixel. The color information specifies 65536 shades of gray.
			s16RGB555,    // 16 bits per pixel; 5 bits each are used for the red, green, and blue components. The remaining bit is not used.
			s16RGB565,    // 16 bits per pixel; 5 bits are used for the red component, 6 bits are used for the green component, and 5 bits are used for the blue component.
			s24RGB,       // 24 bits per pixel; 8 bits each are used for the red, green, and blue components.
			s32PARGB,     // 32 bits per pixel; 8 bits each are used for the alpha, red, green, and blue components. The red, green, and blue components are premultiplied according to the alpha component.
			s32RGB,       // 32 bits per pixel; 8 bits each are used for the red, green, and blue components. The remaining 8 bits are not used.
			s48RGB,       // 48 bits per pixel; 16 bits each are used for the red, green, and blue components.
			s64ARGB,      // 64 bits per pixel; 16 bits each are used for the alpha, red, green, and blue components.
			s64PARGB,     // 64 bits per pixel; 16 bits each are used for the alpha, red, green, and blue components. The red, green, and blue components are premultiplied according to the alpha component.
			s1A,          // 1 bit alpha component
			s8A,          // 8 bits alpha component
			s8GrayScale,  // 8 bits gray
			s1Idx,        // 1 bit per pixel and that it uses indexed color. The color table therefore has two colors in it.
			s2Idx,        // 2 bit per pixel and that it uses indexed color. The color table therefore has 4 colors in it.
			s4Idx,        // 4 bits per pixel, indexed.
			s8Idx,        // 8 bits per pixel, indexed. The color table therefore has 256 colors in it.
			s16Idx,       // 16 bits per pixel, indexed. The color table therefore has 256 colors in it.
			sCount
		};

		static uint32 FASTCALL UniformToGrayscale(uint32 u);

		PixF(int s = 0);
		int    IsValid() const;
		uint   GetBpp() const;
		uint   FASTCALL GetStride(uint width) const;
		int    GetUniform(const void * pSrc, void * pUniformBuf, uint width, const Palette * pPalette) const;
		int    SetUniform(const void * pUniformBuf, void * pDest, uint width, Palette * pPalette) const;

		int32  S;
	};
	struct StoreParam {
		StoreParam(int fmt);

		enum {
			fInterlaced = 0x0001
		};
		int   Fmt;
		long  Flags;
	};

	static int IsSupportedFormat(int fm);

	SImageBuffer();
	SImageBuffer(const SImageBuffer & rS);
	SImageBuffer(uint w, uint h, PixF f = PixF::s32ARGB);
	~SImageBuffer();
	SImageBuffer & FASTCALL operator = (const SImageBuffer & rS);
	void   Destroy();
	int    Init(uint w, uint h, PixF f = PixF::s32ARGB);
	int    FASTCALL IsEqual(const SImageBuffer & rS) const;
	int    FASTCALL Copy(const SImageBuffer & rS);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    AddLines(const void * pSrc, PixF s, uint count, const Palette * pPalette);
	PixF   GetFormat() const;
	uint   GetWidth() const;
	uint   GetHeight() const;
	TPoint GetDim() const;
	FPoint GetDimF() const;
	const  uint8 * GetData() const;
	int    GetSubImage(SImageBuffer & rDest, TPoint start, TPoint size) const;
	//
	// Descr: Создает контекст вывода для графической подсистемы sys.
	// Note: Пока поддерживает только dsysCairo.
	//
	void * CreateSurface(SDrawSystem sys) const;

	int    Load(const char * pFileName);
	int    Load(int fm, SBuffer & rInBuf);
	//
	// Descr: Специализированная функция, извлекающая изображение из
	//   дескриптора битовой карты WinGDI.
	//
	int    LoadBmp(HDC hDc, HBITMAP hBmp, uint subImgSqIdx = 0, uint subImgSqSide = 0);
	int    LoadMime_Base64(const char * pFormatStr, const SString & rS);
	int    Store(const StoreParam &, SFile & rF);
	int    TransformToBounds(TPoint size, const SViewPort * pVp);
	int    TransformToGrayscale();
private:
	int    LoadJpeg(SFile & rF, int fileFmt);
	int    LoadPng(SFile & rF);
	int    LoadIco(SFile & rF, uint pageIdx);
	int    LoadBmp(SFile & rF);
	int    LoadGif(SFile & rF);
	int    Helper_Load(SFile & rF, SFileFormat ff);
	int    Helper_LoadBmp(SBuffer & rBuf, const char * pAddedErrorInfo);
	int    StorePng(const StoreParam & rP, SFile & rF);
	int    Flip();
	int    PremultiplyAlpha();
	uint8 * FASTCALL GetScanline(uint lineNo) const;

	PixF   F; // Формат пикселей
	TPoint S; // Размер образа
};
//
// Descr: Базовый класс для представления фигур рисования.
//
class SDrawFigure { // @persistent @store(SSerializeContext)
public:
	friend class SDrawGroup;

	static int CheckKind(int kind);
	//
	// Descr: Создает экземпляр фигуры из файла pFileName.
	//   Допускаются следующие форматы файлов: svg, jpeg, bmp, png, ico, cur, gif.
	// ARG(pFileName IN): Путь к файлу-источнику
	// ARG(pSid IN): Опциональный символ загружаемой фигуры.
	// Returns:
	//   0 - error
	//   !0 - указатель на созданный экземпляр фигуры. После использования экземпляр
	//     должен быть разрушен оператором delete.
	//
	static SDrawFigure * CreateFromFile(const char * pFileName, const char * pSid);
	//
	// Descr: Создает экземпляр объекта, возможно, порожденого от SDrawFigure
	//   из буфера сериализации rBuf.
	//
	static SDrawFigure * Unserialize(SBuffer & rBuf, SSerializeContext * pCtx);

	enum { // @persistent
		kShape = 1, // Геометрическая фигура
		kPath,      // Путь
		kImage,     // Растровое изображение
		kGroup,     // Контейнер содержащие произвольные фигуры (в т.ч. и группы)
		kExtern,    // Объект, испольузющий внешнее описание SDrawFigure
		kText       // Текст
	};

	virtual ~SDrawFigure();
	virtual SDrawFigure * Dup() const = 0;
	virtual void SetTransform(LMatrix2D * pMtx);
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    GetKind() const;
	const  SString & GetSid() const;
	void   SetSid(const char * pSid);
	void   SetStyle(int identPen, int identBrush);
	int    GetPen() const;
	int    GetBrush() const;
	void   SetViewPort(SViewPort * pVp);
	void   SetSize(FPoint sz);
	//
	// Descr: Возвращает ссылку на внутреннюю структуру SViewPort (порт вывода).
	//
	const  SViewPort & GetViewPort() const;
	//
	// Descr: Присваивает по адресу pVp структуру эффективного порта вывода
	//   (с учетом контекста).
	//
	int    GetViewPort(SViewPort * pVp) const;
	const  FPoint   & GetSize() const;
	const  LMatrix2D & GetTransform() const;
	SPaintToolBox * GetToolBox() const;
	int    TransformToImage(const SViewPort * pVp, SImageBuffer & rImg);
	SDrawImage * DupToImage(TPoint size, const SViewPort * pVp, const char * pSid);
protected:
	SDrawFigure(int kind, const char * pSid);
	int    FASTCALL Copy(const SDrawFigure &);

	enum {
		fOuterToolbox = 0x0001, // Объект использует внешний SPaintToolBox
			// Используется порожденными классами.
		fDraw         = 0x0002, // Устанавливается классом SDraw для того,
			// чтобы члены контейнера могли распознать его как родителя,
			// имеющего специфические атрибуты (SPaintToolBox, например).
		fDefinedSize  = 0x0004  // @!{SDrawFigure::SetSize} Размер элемента определен из-вне
			// (то есть, он не должен рассчитываться).
	};
	int32  Kind;
	int32  Flags;
	int32  IdPen;   // Если IdPen < 0, то это означает, что при отрисовке необходимо
		// внешнее определение карандаша (либо некоторое default-значение)
	int32  IdBrush;
	FPoint Size;
	SViewPort Vp;
	LMatrix2D Tf;
	SDrawFigure * P_Parent; // Родительский контейнер
	SString Sid;
};

class SDrawShape : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	SDrawShape(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawShape & rS);

	FShape S;
};

class SDrawText : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	SDrawText(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawText & rS);

	int32  IdFont;
	FPoint Begin;  // Левый верхний угол текста. Размер прямоугольника определяется полем SDrawFigure::Size
	SString Text;  // UTF-8
};

class SDrawPath : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	DECL_INVARIANT_C();

	enum {
		opNop = 1,  // arg_count=0
		opMove,     // arg_count=2
		opLine,     // arg_count=2
		opCurve,    // arg_count=6
		opQuad,     // arg_count=4
		opArcSvg,   // arg_count=7
		opClose     // arg_count=0
	};
	struct Item {
		const  FPoint & Pnt(uint p)
		{
			return *(const FPoint *)(P_ArgList+p);
		}
		int    Op;
		uint   ArgCount;
		const  float * P_ArgList;
	};
	SDrawPath(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	int    FASTCALL Copy(const SDrawPath & rS);
	int    IsEmpty() const;
	int    HasCurrent() const;
	const  FPoint & GetCurrent();
	void   Clear();
	//
	uint   GetCount() const;
	const Item * FASTCALL Get(uint i, Item &) const;
	//
	int    Nop();
	int    FASTCALL Move(const FPoint &);
	int    FASTCALL Line(const FPoint &);
	int    Curve(const FPoint & rMiddle1, const FPoint & rMiddle2, const FPoint & rEnd);
	int    Quad(const FPoint & rMiddle, const FPoint & rEnd);
	int    ArcSvg(const FPoint & rCenter, float xAxRotation, int largeFlag, int sweepFlag, const FPoint & rEnd);
	int    Close();

	enum {
		fmtDefaut,
		fmtSVG
	};
	int    FromStr(const char * pStr, int fmt);
private:
	void   Helper_Init();
	int    FASTCALL CheckOp(int op) const;
	uint   FASTCALL GetOpArgCount(int op) const;
	enum {
		fHasCur      = 0x00010000,
		fHasLastMove = 0x00020000
	};
	FPoint Cur;      // Текущая точка
	FPoint LastMove; // Начало последнего подпути
	LAssocArray OpList; // Key - op, Val - index of arg_list in ArgList;
	FloatArray ArgList;
};

class SDrawImage : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	SDrawImage(const char * pSid = 0);
	SDrawImage(SImageBuffer & rBuf, const char * pSid = 0);
	~SDrawImage();
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawImage & rS);
	int    SetBuffer(const SImageBuffer * pBuf);
	const  SImageBuffer & GetBuffer() const;
	int    LoadFile(const char * pFileName);
	int    LoadBuf(int fm, SBuffer & rInBuf);
	int    LoadMime_Base64(const char * pFormatStr, const SString & rS);
	int    Store(SImageBuffer::StoreParam & rP, SFile & rF);
	int    TransformToBounds(TPoint size, const SViewPort * pVp);
	int    TransformToGrayscale();
private:
	SImageBuffer Buf;
};

class SDrawGroup : public SDrawFigure, private TSCollection <SDrawFigure> { // @persistent @store(SSerializeContext)
public:
	SDrawGroup(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawGroup & rS);

	int    Add(SDrawFigure * pItem);
	//
	// Descr: Удаляет элемент группы, имеющий символ pSid. Если параметр recur != 0,
	//   то элемент с символом pSid ищется и внутри вложенных в this групп.
	// Returns:
	//   >0 - элемент найден и успешно удален
	//   <0 - элемент по символу pSid не найден
	//   0  - ошибка
	//
	int    Remove(const char * pSid, int recur);
	void   Clear();
	uint   GetCount() const;
	const  SDrawFigure * Get(uint i) const;
	const  SDrawFigure * Find(const char * pSid, int recur) const;
};

class SDraw : public SDrawGroup {
public:
	friend class SDrawFigure;

	SDraw();
	SDraw(const char * pSid);
	SDraw(const char * pSid, SPaintToolBox * pOuterTb);
	~SDraw();
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDraw & rS);
	//
	// Descr: Устанавливает контекст пересчета единиц измерения в
	//   соответствии с экземпляром класса SDrawContext.
	//
	int    SetupUnitContext(SDrawContext & rDCtx);
	int    ParseSvgFile(const char * pFileName);
	int    ConvertCoord(const USize & rUsz, double * pR) const;
private:
	SPaintToolBox * P_Tb;
	SDrawContext::UC UCtx;
};
//
// SylkWriter
//
enum SylkFontStyle {
	slkfsBold      = 0x0001,
	slkfsItalic    = 0x0002,
	slkfsUnderline = 0x0004
};

class SylkWriter {
public:
	SLAPI  SylkWriter(const char * pFileName = 0);
	SLAPI ~SylkWriter();
	int    SLAPI Open(const char * pFileName);
	int    SLAPI Close();
	int    SLAPI PutVal(const char *, int cvtOemToChr);
	int    SLAPI PutVal(double);
	int    SLAPI PutColumnWidth(int start, int end, int width);
	int    SLAPI PutRec(const char * pTypeStr, const char *);
	int    SLAPI PutRec(int typeChr, const char *);
	int    SLAPI PutLine(const char *);
	int    SLAPI PutFont(int symb, const char * pFontName, int size, uint fontStyle);
	int    SLAPI PutFormat(const char * pBuf, int fontId, int col, int row);
	int    SLAPI GetBuf(SString * pBuf) const;
private:
	SString Buf;
	FILE * Stream;
};

class VCard {
public:
	enum Property {
		propBegin = 0,
		propEnd,
		propVersion,
		propName,
		propOrg,
		propBirthDay,
		propAddr,
		propPhone,
		propFax,
		propEmail
	};
	enum PropAttribute {
		paNone = 0,
		paWork,
		paHome,
		paMobile,
		paInternet,
		paVoice,
		paFax
	};
	struct Rec {
		Rec();
		void Init();

		SString Name;
		SString Org;
		SString WorkAddr;
		SString HomeAddr;
		LDATE   BirthDay;
		SString WorkPhone;
		SString HomePhone;
		SString MobilePhone;
		SString WorkFax;
		SString HomeFax;
		SString Email1;
		SString Email2;
	};
	SLAPI  VCard(const char * pFileName = 0, int forExport = 1);
	SLAPI ~VCard();
	int    SLAPI Open(const char * pFileName, int forExport);
	int    SLAPI Close();
	int    SLAPI Put(const Rec *);
	int    SLAPI Get(Rec *);
private:
	int    SLAPI PutProp(Property prop, const void * pData, PropAttribute attrib = paNone);
	int    SLAPI GetName(SString &);
	int    SLAPI GetBirthDate(LDATE *);
	int    SLAPI GetPhones(SString &);
	int    SLAPI GetEmails(SString &);
	int    SLAPI GetAddrs(SString &);

	int    Export;
	SString Properties;
	SString PropAttrib;
	SFile * P_Stream;
};

class VCalendar {
public:
	enum TodoProperty {
		prpBeginCal = 0,
		prpBeginTodo,
		prpEndCal,
		prpEndTodo,
		prpCreatedDtm,
		prpCompletedDtm,
		prpStartDtm,
		prpEndDtm,
		prpDueDtm,
		prpSequence,
		prpStatus,
		prpCategory,
		prpClassification,
		prpPriority,
		prpOwner,
		prpSummary,
		prpLocation,
		prpDescr,
		prpContact,
		prpAttendee
	};
	enum TodoStatus {
		stNeedsAction = 0,
		stAccepted,
		stSent,
		stTentative,
		stConfirmed,
		stDeclined,
		stCompleted,
		stDelegated
	};
	enum TodoClass {
		clPublic = 0,
		clPrivate,
		clConfidential
	};
	enum TodoPropAttrib {
		prpatrRoleOwner = 0,
		prpatrEncMime64,
		prpatrShortEncMime64
	};
	struct Event {
		Event()
		{
			Init();
		}
		void Init()
		{
		}
	};
	struct Todo {
		Todo();
		void   Init();

		LDATETIME CreatedDtm;
		LDATETIME CompletedDtm;
		LDATETIME StartDtm;
		LDATETIME EndDtm;
		LDATETIME DueDtm;
		int16  Sequence;
		int16  Priority;
		TodoStatus Status;
		TodoClass  Classification;
		SString Category;
		SString Owner;
		SString Contact;  // @v9.4.3
		SString Attendee; // @v9.4.3
		SString Summary;
		SString Location;
		SString Descr;
	};
	SLAPI  VCalendar(const char * pFileName = 0, int forExport = 1);
	SLAPI ~VCalendar();
	int    SLAPI Open(const char * pFileName, int forExport);
	int    SLAPI Close();
	int    SLAPI PutEvent(VCalendar::Event *);
	int    SLAPI PutTodo(const VCalendar::Todo *);
	int    SLAPI GetTodo(VCalendar::Todo *);
private:
	int    SLAPI PutTodoProperty(TodoProperty prop, const void * pVal, long addedParam);
	int    SLAPI GetDtm(SString & rBuf, LDATETIME * pDtm);
	int    SLAPI ReadProp(TodoProperty * pProp, SString & rVal, SString & rAttrib);

	int    Export;
	SString PrevTempBuf;
	SString Properties;
	SString Status;
	SString Classification;
	SString PropAttrib;
	SFile * P_Stream;
};

#else // }{

extern int SLibError;
extern int BinDateFmt;
extern int TxtDateFmt;
extern int DefaultYear;
extern int DefaultMonth;

extern CompFunc SAry_OrgFCMP;
extern int    SAry_PtrContainer;
extern long   SAry_SortExtraData;

#endif // } _WIN32_WCE
//
// Descr: Структура географических координат, выраженных широтой и долготой (double).
//
struct SGeoPosLL { // @persistent
	SGeoPosLL();
	SGeoPosLL(double lat, double lon);
	int    FASTCALL operator == (const SGeoPosLL & rS) const;
	int    FASTCALL operator != (const SGeoPosLL & rS) const;
	int    FASTCALL Cmp(const SGeoPosLL & rS) const;
	int    Valid() const;
	SString & FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL FromStr(const char * pStr);

	double Lat;
	double Lon;
};

class SGeoPosLL_Int { // @persistent
public:
	SGeoPosLL_Int();
	SGeoPosLL_Int(const SGeoPosLL_Int & rS);
	SGeoPosLL_Int(const SGeoPosLL & rS);
	SGeoPosLL_Int(double lat, double lon);
	double GetLat() const;
	double GetLon() const;
	long   GetIntLat() const;
	long   GetIntLon() const;
	int    FASTCALL operator == (const SGeoPosLL_Int & rS) const;
	int    FASTCALL operator != (const SGeoPosLL_Int & rS) const;
	int    FASTCALL operator == (const SGeoPosLL & rS) const;
	int    FASTCALL operator != (const SGeoPosLL & rS) const;
	int    FASTCALL Cmp(const SGeoPosLL_Int & rS) const;
	int    FASTCALL Cmp(const SGeoPosLL & rS) const;
	int    Set(double lat, double lon);
	int    SetInt(long lat, long lon);
	int    Valid() const;
	SString & FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL FromStr(const char * pStr);
private:
	long   Lat;
	long   Lon;
};
//
// Descr: Механизмы расчета, обратного преобразования и иных операций над 2х-мерным Z-index'ом
//
class SZIndex2 {
public:
	struct P16 {
        uint16 X;
        uint16 Y;
	};
	struct R16 {
		P16    LU;
		P16    RL;
	};
    static uint32 FASTCALL Combine(uint16 x, uint16 y);
    static uint32 FASTCALL Combine(const P16 & rP)
    {
    	return Combine(rP.X, rP.Y);
    }
    static uint64 FASTCALL Combine(uint32 x, uint32 y);
};
//
//
//
class SGeoGridTab {
public:
	//
	// Descr: Подкласс Finder необходим для обеспечения константности экземпляра
	//   SGeoGridTab при поиске значений решетки.
	// Note: Штатный поиск осуществляется бинарным методом, однако, мы воспользуемся
	//   высокой локальностью запрашиваемых точек и прежде всего будем искать требуемое
	//   значение в той ячейке, которая была запрошена предыдущим вызовом.
	//
	class Finder {
	public:
		SLAPI  Finder(const SGeoGridTab & rTab);
        uint   FASTCALL GetIdxLat(long lat);
        uint   FASTCALL GetIdxLon(long lon);
        int    SLAPI GetIdx(const SGeoPosLL_Int & rC, uint & rIdxLat, uint & rIdxLon);
        uint32 FASTCALL GetZIdx32(const SGeoPosLL_Int & rC);
        uint64 FASTCALL GetZIdx64(const SGeoPosLL_Int & rC);
	private:
		const SGeoGridTab & R_Tab;
        uint   LastPosLat;
        uint   LastPosLon;
	};

	friend class SGeoGridTab::Finder;

	SLAPI  SGeoGridTab(uint dim);
	int    FASTCALL IsEqual(const SGeoGridTab & rS) const;
	int    FASTCALL operator == (const SGeoGridTab & rS) const;
	int    FASTCALL operator != (const SGeoGridTab & rS) const;
	void   SLAPI SetSrcCountLat(uint64 c);
	void   SLAPI SetSrcCountLon(uint64 c);
	uint   SLAPI GetDim() const;
    uint   SLAPI GetDensityLat() const;
    uint   SLAPI GetDensityLon() const;
    int    FASTCALL AddThresholdLat(long coord);
    int    FASTCALL AddThresholdLon(long coord);
    uint   SLAPI GetCountLat() const;
    uint   SLAPI GetCountLon() const;
	//
	int    SLAPI Save(const char * pFileName);
	int    SLAPI Load(const char * pFileName);
private:
    uint   Dim; // Размерность решетки (бит)
    uint64 SrcCountLat;
    uint64 SrcCountLon;
	LongArray LatIdx;
	LongArray LonIdx;
};

class SGeo {
public:
	static const double A_WGS84; // Радиус Земли в метрах (согласно WGS84)
	static const double Flattening_WGS84; // Фактор приплюснотости Земли: ƒ = (a − b)/a (согласно WGS84).
		// Где a - экваториальный радиус, b - полярный радиус.
	//
	// Descr: mask values for the \e caps argument to geod_lineinit()
	//
	enum /*geod_mask*/ {
		GEOD_NONE         = 0U,                     // Calculate nothing
		GEOD_LATITUDE     = 1U<<7  | 0U,            // Calculate latitude
		GEOD_LONGITUDE    = 1U<<8  | 1U<<3,         // Calculate longitude
		GEOD_AZIMUTH      = 1U<<9  | 0U,            // Calculate azimuth
		GEOD_DISTANCE     = 1U<<10 | 1U<<0,         // Calculate distance
		GEOD_DISTANCE_IN  = 1U<<11 | 1U<<0 | 1U<<1, // Allow distance as input
		GEOD_REDUCEDLENGTH= 1U<<12 | 1U<<0 | 1U<<2, // Calculate reduced length
		GEOD_GEODESICSCALE= 1U<<13 | 1U<<0 | 1U<<2, // Calculate geodesic scale
		GEOD_AREA         = 1U<<14 | 1U<<4,         // Calculate reduced length
		GEOD_ALL          = 0x7F80U| 0x1FU          // Calculate everything
	};
	//
	// Descr: flag values for the \e flags argument to geod_gendirect() and geod_genposition()
	//
	enum /*geod_flags*/ {
		GEOD_NOFLAGS      = 0U,     // No flags
		GEOD_ARCMODE      = 1U,  // Position given in terms of arc distance
		GEOD_LONG_UNROLL  = 1U<<15  // Unroll the longitude
	};
	enum /*captype*/ {
		CAP_NONE = 0x0000U,
		CAP_C1   = 0x0001U,
		CAP_C1p  = 0x0002U,
		CAP_C2   = 0x0004U,
		CAP_C3   = 0x0008U,
		CAP_C4   = 0x0010U,
		CAP_ALL  = 0x001FU,
		OUT_ALL  = 0x7F80U
	};
	SGeo();
	//
	// The general direct geodesic problem.
	//
	// ARG(rP1 in) latitude and longitude of point 1 (degrees).
	// ARG(azi1 in) azimuth at point 1 (degrees).
	// ARG(flags in) bitor'ed combination of geod_flags(); flags & GEOD_ARCMODE
	//   determines the meaning of s12_a12 and flags & GEOD_LONG_UNROLL "unrolls" lon2.
	// ARG(s12_a12 in) if flags & GEOD_ARCMODE is 0, this is the distance
	//   from point 1 to point 2 (meters); otherwise it is the arc length
	//   from point 1 to point 2 (degrees); it can be negative.
	// ARG(pP2 out) pointer to the latitude and longitude of point 2 (degrees).
	// ARG(pazi2 out) pointer to the (forward) azimuth at point 2 (degrees).
	// ARG(ps12 out) pointer to the distance from point 1 to point 2 (meters).
	// ARG(pm12 out) pointer to the reduced length of geodesic (meters).
	// ARG(pM12 out) pointer to the geodesic scale of point 2 relative to point 1 (dimensionless).
	// ARG(pM21 out) pointer to the geodesic scale of point 1 relative to point 2 (dimensionless).
	// ARG(pS12 out) pointer to the area under the geodesic (meters<sup>2</sup>).
	// Returns: \e a12 arc length from point 1 to point 2 (degrees).
	//
	// \e g must have been initialized with a call to geod_init().  \e lat1
	// should be in the range [&minus;90&deg;, 90&deg;].  The function value \e
	// a12 equals \e s12_a12 if \e flags & GEOD_ARCMODE.  Any of the "return"
	// arguments, \e plat2, etc., may be replaced by 0, if you do not need some quantities computed.
	//
	// With \e flags & GEOD_LONG_UNROLL bit set, the longitude is "unrolled" so
	// that the quantity \e lon2 &minus; \e lon1 indicates how many times and in
	// what sense the geodesic encircles the ellipsoid.
	//
	double Direct(const SGeoPosLL & rP1, double azi1, uint flags, double s12_a12, SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12);
	//
	// The general inverse geodesic calculation.
	//
	// ARG(rP1    IN): latitude and longitude of point 1 (degrees).
	// ARG(rP2    IN): latitude and longitude of point 2 (degrees).
	// ARG(ps12  OUT): pointer to the distance from point 1 to point 2 (meters).
	// ARG(pAzi1 OUT): pointer to the azimuth at point 1 (degrees).
	// ARG(pAzi2 OUT): pointer to the (forward) azimuth at point 2 (degrees).
	// ARG(pm12  OUT): pointer to the reduced length of geodesic (meters).
	// ARG(pM12  OUT): pointer to the geodesic scale of point 2 relative to point 1 (dimensionless).
	// ARG(pM21  OUT): pointer to the geodesic scale of point 1 relative to point 2 (dimensionless).
	// ARG(pS12  OUT): pointer to the area under the geodesic (meters<sup>2</sup>).
	// @return \e a12 arc length from point 1 to point 2 (degrees).
	//
	// \e g must have been initialized with a call to geod_init().  \e lat1 and
	// \e lat2 should be in the range [&minus;90&deg;, 90&deg;].  Any of the
	// "return" arguments \e ps12, etc., may be replaced by 0, if you do not need
	// some quantities computed.
	//
	double Inverse(const SGeoPosLL & rP1, const SGeoPosLL & rP2, double * ps12, double * pAzi1, double * pAzi2, double * pm12, double * pM12, double * pM21, double * pS12);
private:
	struct SinCosPair {
		SinCosPair();
		SinCosPair(const SinCosPair & rS);
		SinCosPair(double angle);
		SinCosPair(double _sin, double _cos);
		SinCosPair & FASTCALL operator = (const SinCosPair & rS);
		void   Set(double _sin, double _cos)
		{
			S = _sin;
			C = _cos;
		}
		void   SetRad(double angle);
		void   Set_SinCosDX(double x);
		SinCosPair & Sum_(const SinCosPair & r1, const SinCosPair & r2);
		double ATan2() const;
		void   Norm2();

		double S;
		double C;
	};
	//
	// The struct containing information about a single geodesic.  This must be
	// initialized by geod_lineinit(), geod_directline(), geod_gendirectline(),
	// or geod_inverseline() before use.
	//
	struct GeodesicLine /*geod_geodesicline*/ {
		SGeoPosLL P1; // The starting point
		double Azi1; // the starting azimuth
		double A;    // the equatorial radius
		double F;    // the flattening
		//double SAlp1; // sine of \e azi1
		//double CAlp1; // cosine of \e azi1
		SinCosPair Alp1;
		double A13;   // arc length to reference point
		double S13;   // distance to reference point
		// @cond SKIP
		double B;
		double C2;
		double F1;
		//double SAlp0;
		//double CAlp0;
		SinCosPair Alp0;
		double K2;
		//double SSig1;
		//double CSig1;
		SinCosPair Sig1;
		double Dn1;
		//double STau1;
		//double CTau1;
		SinCosPair Tau1;
		//double SOmg1;
		//double COmg1;
		SinCosPair Omg1;
		double A1m1;
		double A2m1;
		double A3c;
		double B11;
		double B21;
		double B31;
		double A4;
		double B41;
		double C1a[6+1];
		double C1pa[6+1];
		double C2a[6+1];
		double C3a[6];
		double C4a[6];
		// @endcond
		uint   Caps; // the capabilities
	};
	double SinCosSeries(int sinp, const SinCosPair & rSC/*double sinx, double cosx*/, const double c[], int n);
	void   LineInit(GeodesicLine * pLine, const SGeoPosLL & rP1, double azi1, uint caps);
	double GenPosition(const GeodesicLine * l, uint flags, double s12_a12,
		SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12);
	void   LineInit_Int(GeodesicLine * l, const SGeoPosLL & rP1, double azi1, const SinCosPair & rAlp1/*double salp1, double calp1*/, uint caps);
	double Inverse_Int(SGeoPosLL & rP1, SGeoPosLL & rP2,
		double * ps12, SinCosPair * pAlp1/*double * psalp1, double * pcalp1*/, SinCosPair * pAlp2/*double * psalp2, double* pcalp2*/, double* pm12, double* pM12, double* pM21, double* pS12);
	void   Lengths(double eps, double sig12, const SinCosPair & rSig1, double dn1, const SinCosPair & rSig2, double dn2,
		double cbet1, double cbet2, double* ps12b, double* pm12b, double* pm0, double* pM12, double* pM21, /* Scratch area of the right size */ double Ca[]);
	double InverseStart(const SinCosPair & rScBet1, double dn1, const SinCosPair & rScBet2, double dn2, double lam12, const SinCosPair & rLam12/*double slam12, double clam12*/,
		SinCosPair * pAlp1/*double * psalp1, double * pcalp1*/, /* Only updated if return val >= 0 */ SinCosPair * pAlp2/*double* psalp2, double* pcalp2*/,
		/* Only updated for short lines */ double* pdnm, /* Scratch area of the right size */ double Ca[]);
	double Lambda12(const SinCosPair & rScBet1, double dn1, const SinCosPair & rScBet2, double dn2,
		SinCosPair scAlp1, double slam120, double clam120, double* psalp2, double* pcalp2, double* psig12,
		double* pssig1, double* pcsig1, double* pssig2, double* pcsig2, double* peps, double* psomg12, double* pcomg12,
		int/*bool*/ diffp, double* pdlam12, /* Scratch area of the right size */ double Ca[]);
	//
	// The struct containing information about the ellipsoid.  This must be
	// initialized by geod_init() before use.
	//
	class Geodesic /*geod_geodesic*/ {
	public:
		Geodesic(double _a, double _f);
		Geodesic();
		double A3f_(double eps) const;
		void   C3f_(double eps, double c[]) const;
		void   C4f_(double eps, double c[]) const;

		double A; // the equatorial radius
		double F; // the flattening
		// @cond SKIP
		double F1;
		double E2;
		double Ep2;
		double N;
		double B;
		double C2;
		double Etol2;
		double A3x[6];
		double C3x[15];
		double C4x[21];
		// @endcond
	private:
		void   Init(double _a, double _f);
		void   C4coeff_();
		void   C3coeff_();
		void   A3coeff_();
	};
	const double _RealMin;
	const double _Tiny;
	const double _Tol0; // = epsilon;
	// Increase multiplier in defn of tol1 from 100 to 200 to fix inverse case
	// 52.784459512564 0 -52.784459512563990912 179.634407464943777557
	// which otherwise failed for Visual Studio 10 (Release and Debug)
	const double _Tol1; //200 * tol0;
	const double _Tol2; // sqrt(tol0);
	// Check on bisection interval
	const double _Tolb; // tol0 * tol2;
	const uint _MaxIt1;
	const uint _MaxIt2;
	const Geodesic G;
};
//
//
//
class SRawInputInitArray : private SArray {
public:
	friend class SRawInputData;

	SRawInputInitArray();
	int    Add(uint16 usagePage, uint16 usage, uint flags, HWND target);
};
//
// Descr: Класс, реализующий буфер переменной длины для обработки сообщения WINDOWS WM_INPUT
//
class SRawInputData {
public:
	static int Register(SRawInputInitArray * pRiia);

	SRawInputData();
	~SRawInputData();
	operator RAWINPUT * ();
	//
	// Descr: Получает данные ввода вызовом GetRawInputData
	//
	int    FASTCALL Get(long rawInputHandle);
	//
	// Descr: Получает строку наименования устройства, из которого приняты данные предшествующим вызовом
	//   this->Get(long).
	//
	int    FASTCALL GetDeviceName(SString & rBuf);
private:
	void   Reset();

	size_t AllocatedSize;
	void * P_Buf;
	uint8  FixedBuffer[256];

	static int InitRawInputProc(int unreg);
};
//
// Descr: Класс для работы с устройствами через USB
//
struct UsbDevDescrSt {
	UsbDevDescrSt();
	UsbDevDescrSt(const UsbDevDescrSt & rSrc);
	UsbDevDescrSt & Clear();

	SString Path;
	SString Type;
	SString Class;
	S_GUID ClassGUID;
	SString Description;
	SString SerialNumber;
	SString Driver;
	SString HardwareID;
	SString Manufacturer;
	SString PDOName;
	SString ServiceName;
};

struct UsbBasicDescrSt {
	UsbBasicDescrSt();
	UsbBasicDescrSt(const UsbBasicDescrSt & rSrc);
	UsbBasicDescrSt & Clear();
	int    operator == (const UsbBasicDescrSt & s) const;

	SUsbDevice * P_Parent; // @notowned
	SString Pid; // Совпадает с Pid родителя
	SString Vid; // Совпадает с Vid родителя
	SString SerialNumber; // Соответствует подстроке из SymbName родителя
};

class SUsbDevice {
public:
	//
	// Descr: Возвращает количество утсройств или -1 в случае ошибки
	//
	static int GetDeviceList(TSCollection <SUsbDevice> & rList);
	//
	// Descr: Разбирает путь устройства на Pid, Vid и серийный номер.
	//	Работает путями вида:
	//		\\?\usb#vid_05f9&pid_2203#s#n_e12g14133#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
	//
	static int ParsePath(const char * pPath, UsbBasicDescrSt & rDescr);
	//
	// Descr: Разбирает путь устройства на Pid, Vid и серийный номер.
	//	Работает путями вида:
	//		HID\VID_05F9&PID_2203\7&3B4F0974&0&0000
	//
	static int ParseSymbPath(const char * pPath, UsbBasicDescrSt & rDescr);

	enum {
		clsHid = 1,
		clsUsb
	};
	SUsbDevice();
	SUsbDevice(const UsbDevDescrSt * pDevDescr);
	SUsbDevice(const SUsbDevice & rSrc);
	~SUsbDevice();
	//
	// Descr: Здесь определим имя устройства через GetPath(), определим его класс и откроем. В зависимости от класса вызываем или нет SetConfig()
	//
	int    Open(/*const char * pPid, const char * pVid*/);
	int    Close();
	//
	// Descr: Сравниваем с Description.HardwareID. Наше устройство - возвращаем 1, иначе - 0. Параметры в виде Pid_номер и Vid_номер
	//
	int    IsDev(const char * pPid, const char * pVid);
	//
	// Descr: Здесь разбиваем строку по кускам и, в зависимости от класса устройства, форматируем эти куски. Отправляем на устройство
	//
	int    Write(const void * pBuf, size_t bufSize);
	//
	// Descr: Считываем с устройства
	//
	int    Read(void * pBuf, size_t dataSize);
	const  UsbDevDescrSt & GetDescription() const;
	const  TSCollection <UsbBasicDescrSt> & GetChildren() const;
	//
	// Descr: Возвращает IntputReportByteLength. Полезно вызвать эту функцию перед Read
	//
	int    GetInputReportDataLength();
	SBuffer & GetExtBuf()
	{
		return ExtBuffer;
	}
private:
	// Descr: Перебирает все подключенные usb-устройства, получает их HID-идентификатор и сравнивает его с заданными PID и VID
	//		 (ибо HID-идентификатор соcтоит из PID и VID и прочих других идентификаторов)
	// ARG(usbClass	IN): Класс устройства
	// Returns:
	//		-1 - если устройство с заданными PID и VID не найдено
	//		 0 - ошибка
	//		 1 - устройство найдено
	//int GetDevPath(uint usbClass);	// Находим имя устройства
	int SetConfig();		// Настраиваем HID-устройство

	uint   DevClass;	// clsXXX
	uint   OutputReportByteLength; // Длина пакета передаваемых данных (для HID-устройств определяется в SetConfig()) (обычно для высокоскоростных устройств это 65 байт)
	uint   IntputReportByteLength; // Длина пакета передаваемых данных (обычно для высокоскоростных устройств это 65 байт)
	HANDLE Handle;
	HANDLE Event;	// new
	OVERLAPPED Ovl; // new
	UsbDevDescrSt Description;
	//
	// Children исползуются для устройств, работа с которыми осуществляется через WM_INPUT.
	// Параметры Children позыоляют, во-первых, отличить данный класс устройства от другого (например,
	// сканер штрихкода от клавиатуры), во-вторых, отличить два одинаковых устройства (через связь серийного
	// номера Children и символьного имени родителя)
	//
	TSCollection <UsbBasicDescrSt> Children;
	SBuffer ExtBuffer;
};

//
// Debug
//
/* @v9.4.11
#ifndef __DEBUG
	#define __DEBUG 2
#endif
#undef CHECK
#if (__DEBUG < 2)
	#define CHECKE(p) (p)
	#define CHECK(p)  ((void)0)
#else
	#define CHECKE(p) assert(p)
	#define CHECK(p)  assert(p)
#endif
*/
//
//
//
class MemLeakTracer {
public:
	SLAPI  MemLeakTracer();
	SLAPI ~MemLeakTracer();
private:
	void * P_State;
};

class MemHeapTracer {
public:
	struct Stat {
		ulong  UsedBlockCount;   // Количество используемых блоков
		ulong  UnusedBlockCount; // Количество неиспользуемых блоков
		ulong  UsedSize;         // Общий размер используемых блоков
		ulong  UnusedSize;       // Общий размер неиспользуемых блоков
	};
	//
	// Descr: Проверяет валидность кучи.
	// Returns:
	//   !0 - куча в порядке
	//   0  - проблемы
	//
	static int Check();

	SLAPI  MemHeapTracer();
	int    SLAPI CalcStat(Stat * pSize);
	int    SLAPI Dump(const char * pFileName, long flags);
};

int SLAPI TraceFunc(const char * pFuncName, const char * pAddedMsg);

#ifdef _DEBUG
	#define TRACE_FUNC()    TraceFunc(__FUNCTION__, 0)
	#define TRACE_FUNC_S(s) TraceFunc(__FUNCTION__, (s))
#else
	#define TRACE_FUNC()
	#define TRACE_FUNC_S(s)
#endif

struct SInvariantParam {
	int    LocalOk;
	ulong  Time;           // Время исполнения //
	SString MsgBuf;
};

#define S_INVARIANT_PROLOG(p)    if(p) (p)->LocalOk = 1
#define S_INVARIANT_EPILOG(p)    return (p) ? (p)->LocalOk : 1
#define S_ASSERT(condition)      if(!(condition)) {return 0;}
#define S_ASSERT_P(condition, p) if(!(condition)) {if(p) (p)->LocalOk = 0; else return 0;}
#define S_ERROR_P(p)             if(p) (p)->LocalOk = 0; else return 0;
//
//
//
template <class C> int TestInvariantC(const C * p, SInvariantParam * pParam) { return p->InvariantC(pParam); }
template <class C> int TestInvariant(C * p, SInvariantParam * pParam) { return p->Invariant(pParam); }
//
// Descr: Класс, управляющий наборами тестовых данных.
//
#define STEST_DATA_MAXVAL 12

class STestDataArray {
public:
	//
	// Descr: Индексы массива ValPos
	//
	enum {
        pIn    = 0,
        pOut   = 1,
        pKey   = 2,
        pNonce = 3
	};
	struct Item {
		Item();
		uint   Count;
		uint   ValPos[STEST_DATA_MAXVAL];
	};
	STestDataArray();
	const  Item & Get(uint idx) const;
	uint   GetCount() const;
	int    GetDataByPos(uint pos, SString & rData) const;
	//
	// Descr: Считывает тестовые данные из файлов пакета botan.
	//   Старые версии botan и более новые имеют различный формат представления данных.
	//   Для того, чтобы отличить один формат от другого применяется параметр formatVer.
	//   formatVer=1 - считывается старое представление XXXX:XXXX[:XXXX]
	//   formatVer=2 - считывается новое представление
	//     in=XXXX
	//     out=XXXX
	//     key=XXXX
	//     Блоки данных разделяются пустой строкой
	//
	int    ReadBotanTestSequence(int formatVer, const char * pFileName, const char * pZone);
private:
	const  Item EmptyItem;
	TSArray <Item> L;
	StringSet HexPool;
};
//
// Descr: Класс для реализации наборов тестов.
//
class STestSuite {
public:
	struct Benchmark {
		MemHeapTracer::Stat HeapBefore;
		MemHeapTracer::Stat HeapAfter;
		int64  Timing;       // Чистое время исполнения потока
		int64  SysTiming;    // Системное время исполнения //
	};
	struct Entry {
		Entry();

		SString TestName;
		SString Descr;        // descr=
		SString InPath;       // input=
		SString OutPath;      // output=
		SString DefTabName;   // tab=
		StringSet ArgList;
		StringSet BenchmarkList;
		uint   MaxCount;
		uint   SuccCount;
		uint   FailCount;
		MemHeapTracer::Stat HeapBefore;
		MemHeapTracer::Stat HeapAfter;
		int64  Timing;    // Чистое время исполнения потока
		int64  SysTiming; // Системное время исполнения //
		SArray BmrList;   // @#{BmrList.getCount() == BenchmarkList.getCount()} Результаты бенчмарков
	};
	STestSuite();
	~STestSuite();
	int    Run(const char * pIniFileName);
	void   PutCaseInfo(const char *);
	const SString & GetTabFileName() const;
	const  Entry * GetCurEntry() const;
private:
	int    LoadTestList(const char * pIniFileName);
	int    ReportTestEntry(int title, const Entry * pEntry);
	void * P_List; // Список тестов. Тип этого списка определен в реализации класса
	uint   CurIdx;
	//
	// Формат хранения параметров теста:
	// [Common]
	// logfile=filename
	// tabfile=filename
	// input=path         // Каталог входных данных по умолчанию (может быть переопределен конкретным тестом)
	// output=path        // Каталог выходных данных по умолчанию (может быть переопределен конкретным тестом)
	// [TestName]
	// descr=             // Описание теста (произвольная строка)
	// input=path         // Каталог входных данных
	// output=path        // Каталог выходных данных
	// tab=tab_name       // Наименование таблицы, используемой для теста
	// count=number_of_passes
	// arglist=arg1;arg2;..;argN
	// benchmark=mark1;mark2;..;markN
	//
	SString LogFileName;
	SString TabFileName; // Полное имя файла, содержащего тестовые таблицы. Если параметр tabfile не
		// определен, то считается, что имя файла таблиц совпадает с именем файла описания тестов, но
		// с расширением .tab
	SString CaseBuffer;  // Строка, в которую тест может вывести какую-либо информацию
	MemHeapTracer Mht;
};
//
// Descr: Базовый класс для реализации тест-кейса.
//
class STestCase {
public:
	STestCase(STestSuite *);
	virtual ~STestCase();
	//
	// Descr: Функция должна выполнить собственно процедуру тестирования.
	//   Если параметр pBenchmark != 0, то STestSuite обращается к функции с целью
	//   запуска процедуры измерения производительности с именем pBenchmark.
	//   Для информирования о результатах тестирования функция может вызывать SetInfo(),
	//   которая занесет текст в строку информации теста, которая потом будет выведена в журнал.
	// Returns:
	//   !0 - функция отработала успешно (тест успешный)
	//   0  - тест не пройден.
	//
	virtual int Run(const char * pBenchmark);
	int    EnumArg(uint * pArgNo, SString &) const;
	int    SetInfo(const char *, int currentStatus = -1);
	int    _check_nz(int result, const char * pV);
	int    _check_z(int result, const char * pV);
	int    _check_eq(const void * a, const void * b, const char * pA, const char * pB);
	int    _check_eq(uint8 a, uint8 b, const char * pA, const char * pB);
	int    _check_eq(uint a, uint b, const char * pA, const char * pB);
	int    _check_eq(long a, long b, const char * pA, const char * pB);
	int    _check_eq(ulong a, ulong b, const char * pA, const char * pB);
	int    _check_eq(int64 a, int64 b, const char * pA, const char * pB);
	int    _check_eq(uint64 a, uint64 b, const char * pA, const char * pB);
	int    _check_eq(double a, double b, const char * pA, const char * pB);
	int    _check_eq(float a, float b, const char * pA, const char * pB);
	int    _check_eq_tolerance(double a, double b, double tol, const char * pA, const char * pB);
	int    _check_eq_tolerance(float a, float b, float tol, const char * pA, const char * pB);
	int    _check_eq(LDATE a, LDATE b, const char * pA, const char * pB);
	int    _check_eq(const SString & rVA, const SString & rVB, const char * pA, const char * pB);
	int    _check_eq(const SString & rVA, const char * pVB, const char * pA, const char * pB);
	int    _check_le(long a, long b, const char * pA, const char * pB);
	int    _check_le(ulong a, ulong b, const char * pA, const char * pB);
	int    _check_le(double a, double b, const char * pA, const char * pB);
	int    _check_lt(long a, long b, const char * pA, const char * pB);
	int    _check_lt(double a, double b, const char * pA, const char * pB);
	int    _check_lt(float a, float b, const char * pA, const char * pB);
	int    _check_range(long a, long low, long upp, const char * pA, const char * pLow, const char * pUpp);
	int    _check_range(ulong a, ulong low, ulong upp, const char * pA, const char * pLow, const char * pUpp);
	int    _check_range(double a, double low, double upp, const char * pA, const char * pLow, const char * pUpp);
	int    _check_range(float a, float low, float upp, const char * pA, const char * pLow, const char * pUpp);
	int    _check_mem(const void * pMem, size_t sz, uint8 pattern, const char * pV);
	int    _check_math_result(SMathResult & r, double val, double tol, const char * pF);
	//
	//		Snippet:
	//		{
	//			SEnum en = EnumTab("some_tab");
	//			if(!en) {
	//				// error
	//			}
	//			else {
	//				STab::Row row;
	//				while(en.Next(&row)) {
	//					// Processing for row of table
	//				}
	//			}
	//		}
	//
	SEnumImp * EnumTab(const char * pTabName);
	int    GetCurrentStatus() const
	{
		return CurrentStatus;
	}
protected:
	const  STestSuite::Entry * GetSuiteEntry() const;
	const char * MakeInputFilePath(const char * pFileName);
	const char * MakeOutputFilePath(const char * pFileName);
	const char * GetTestName() const;
	int    CurrentStatus;
private:
	template <class T> int Implement_check_eq(const T a, const T b, const char * pA, const char * pB)
	{
		if(a != b) {
			SString buf;
			SetInfo(catval(b, pB, catval(a, pA, buf).Cat("!=")), 0);
			return 0;
		}
		else
			return 1;
	}

	STestSuite * P_Suite;
	SString TempBuf;

	class TabEnum : public SEnumImp {
	public:
		TabEnum(const char * pTabFileName, const char * pTabName);
		virtual int Next(void * pData);
		enum {
			stError = 0x0001
		};
		STab   Tab;
		uint   RowIdx;
		long   State;
	};
};
//
// @vmiller {
//
#if 0 // {

struct KeyValuePair {
	SString Key;
	SString Value;
};

class KeyValueCollection {
public:
	KeyValueCollection(SString & rStr)
	{
		Parse(rStr);
	}
	int getValue(const char * pKey, SString & rValue)
	{
		int    ok = 0;
		for(uint i = 0, n = Coll.getCount(); i < n; i++) {
			KeyValuePair * p_pair = Coll.at(i);
			if(p_pair) {
				if(p_pair->Key == pKey) {
					rValue = p_pair->Value;
					ok = 1;
					break;
				}
			}
		}
		return ok;
	}
private:
	int Parse(SString & rStr)
	{
		int    ok = 1;
		SString rec, tail, s1, s2, temp_buf;
		if(!rStr.Empty()) {
			int    r = rStr.Divide(';', rec, tail);
			if(r != 0) {
				r = rec.Divide('=', s1, s2);
				if(r != 0) {
					if(r > 0) {
						KeyValuePair * p_pair = new KeyValuePair();
						p_pair->Key = s1;
						p_pair->Value = s2;
						Coll.insert(p_pair);
					}
					if(!tail.Empty())
						Parse(tail);    // @recursion
				}
				else {
					ok = 0;
				}
			}
			else {
				ok = 0;
			}
		}
		return ok;
	}

	TSCollection <KeyValuePair> Coll;
};
#endif // } 0
// } @vmiller

extern "C" typedef STestCase * (*FN_SLTEST_FACTORY)(STestSuite * pSuite);

#define SLTEST_CHECK_NZ(v)                _check_nz(!!(v), #v)
#define SLTEST_CHECK_Z(v)                 _check_z(!!(v), #v)
#define SLTEST_CHECK_EQ(a, b)             _check_eq((a), (b), #a, #b)
#define SLTEST_CHECK_EQ_TOL(a, b, tol)    _check_eq_tolerance((a), (b), (tol), #a, #b)
#define SLTEST_CHECK_LT(a, b)             _check_lt((a), (b), #a, #b)
#define SLTEST_CHECK_LE(a, b)             _check_le((a), (b), #a, #b)
#define SLTEST_CHECK_CRANGE(a, low, upp)  _check_range((a), (low), (upp), #a, #low, #upp)
#define SLTEST_CHECK_MEM(ptr, size, pattern) _check_mem((ptr), (size), (pattern), #ptr)

#define SLTEST_CLS(s)     SLTC_##s
#define SLTEST_FACTORY(s) SLTCF_##s
#define IMPLEMENT_SLTEST_FACTORY(test) \
	extern "C" __declspec(dllexport) STestCase * SLTEST_FACTORY(test)(STestSuite * pSuite) \
	{ return new SLTEST_CLS(test)(pSuite); }
#define DECL_SLTEST(test)                 class SLTEST_CLS(test) : public STestCase { \
	public: SLTEST_CLS(test)(STestSuite * pS) : STestCase(pS) {} virtual int Run(const char * pBenchmark);};
#define DECL_SLTEST_FIXTURE(test,fixture) class SLTEST_CLS(test) : public STestCase { \
	fixture F; public: SLTEST_CLS(test)(STestSuite * pS) : STestCase(pS) {} virtual int Run(const char * pBenchmark);};

#define SLTEST_R(test)                 \
	DECL_SLTEST(test)                  \
	IMPLEMENT_SLTEST_FACTORY(test)     \
	int SLTEST_CLS(test)::Run(const char * pBenchmark)

#define SLTEST_FIXTURE(test,fixture)   \
	DECL_SLTEST_FIXTURE(test,fixture)  \
	IMPLEMENT_SLTEST_FACTORY(test)     \
	int SLTEST_CLS(test)::Run(const char * pBenchmark)

#endif /* RC_INVOKED */

#endif /* __SLIB_H */
