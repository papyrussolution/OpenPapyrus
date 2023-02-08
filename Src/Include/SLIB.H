// SLIB.H
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#ifndef __SLIB_H
#define __SLIB_H
#define SLIBINCLUDED
// Специальное определение, необходимое на начальной фазе сборки openssl 3.0. Для этого сделан отдельный проект SLIB, в котором SLIB_OSSL_VER_30 определен.
#define SLIB_OSSL_VER_30 // @v11.6.2 turned-on

#ifndef __SLCONFIG_H
	#include <slconfig.h>
#endif
#define WINDOWS_IGNORE_PACKING_MISMATCH // @v10.9.11
#define U_STATIC_IMPLEMENTATION // @v11.3.0 ICU включается как статическая библиотека
#ifndef RC_INVOKED
	#include <slport.h>
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
	#include <errno.h>
	#include <signal.h> // @v11.2.12
	#include <setjmp.h> // @v11.2.12
	// @v10.9.6 {
	#if _MSC_VER < 1900
		// POSIX Supplement
		#ifndef _CRT_NO_POSIX_ERROR_CODES
			#define EADDRINUSE      100
			#define EADDRNOTAVAIL   101
			#define EAFNOSUPPORT    102
			#define EALREADY        103
			#define EBADMSG         104
			#define ECANCELED       105
			#define ECONNABORTED    106
			#define ECONNREFUSED    107
			#define ECONNRESET      108
			#define EDESTADDRREQ    109
			#define EHOSTUNREACH    110
			#define EIDRM           111
			#define EINPROGRESS     112
			#define EISCONN         113
			#define ELOOP           114
			#define EMSGSIZE        115
			#define ENETDOWN        116
			#define ENETRESET       117
			#define ENETUNREACH     118
			#define ENOBUFS         119
			#define ENODATA         120
			#define ENOLINK         121
			#define ENOMSG          122
			#define ENOPROTOOPT     123
			#define ENOSR           124
			#define ENOSTR          125
			#define ENOTCONN        126
			#define ENOTRECOVERABLE 127
			#define ENOTSOCK        128
			#define ENOTSUP         129
			#define EOPNOTSUPP      130
			#define EOTHER          131
			#define EOVERFLOW       132
			#define EOWNERDEAD      133
			#define EPROTO          134
			#define EPROTONOSUPPORT 135
			#define EPROTOTYPE      136
			#define ETIME           137
			#define ETIMEDOUT       138
			#define ETXTBSY         139
			#define EWOULDBLOCK     140
		#endif // _CRT_NO_POSIX_ERROR_CODES
	#endif
	// } @v10.9.6
	#include <fcntl.h>
	#include <sys\stat.h>
	#include <tchar.h>
	#include <wchar.h>
	#include <stdint.h> // @v10.8.1
	#include <string.h> // @v11.0.7
	// @v10.4.3
	#ifdef HAVE_UNISTD_H
		#include <unistd.h>
	#endif
	#include <process.h> // @v10.9.3
#endif
#ifdef _WIN32
	#ifndef __WIN32__
		#define __WIN32__ 1 // (__WIN32__)-->(__WIN32__ 1)
	#endif
	#define STRICT
	#define OEMRESOURCE
	#define _WIN32_WINDOWS 0x0410
	#define SECURITY_WIN32
	#ifndef _WIN32_IE
		#if _MSC_VER < 1900 // @v10.9.9
			#define _WIN32_IE 0x0500
		#else
			#define _WIN32_IE 0x0700 // =_WIN32_IE_IE70 @v10.9.9
		#endif
	#endif
	#if _M_X64
		#pragma pack(push)
		#pragma pack(8)
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h> // @v10.9.9
	#if _M_X64
		#pragma pack(pop)
	#endif
	#include <windows.h>
	#define _Windows
	// @v11.3.10 (for Visual C _MSC_VER == 1310) {
	#ifndef MAXUINT32
		#define MAXUINT32   ((uint32)~((uint32)0))
	#endif
	#ifndef MAXINT32
		#define MAXINT32    ((int32)(MAXUINT32 >> 1))
	#endif
	#ifndef MININT32
		#define MININT32    ((int32)~MAXINT32)
	#endif
	// } @v11.3.10 
	// @v10.3.10 #define pow10l(x) pow(10,(x))
	// @v10.3.10 #define pow10(x)  pow(10,(x))
	#define MAXDRIVE 3
	#define MAXFILE 257
	#define MAXEXT 5
	#define MAXDIR 257
	#define MAXPATH 261
#endif // _WIN32
// @v5.0.4 AHTOXA {
#ifdef _WIN32_WCE
	#define OEMRESOURCE
	#define _WIN32_WINDOWS 0x0410 // Papyrus @v4.1.12
	#include <winsock2.h>
	#include <windows.h>
	#define _Windows
	#define asm __asm
	#define stricmp         _stricmp
	#define strnicmp        _strnicmp
	#define stpcpy(s1, s2) (strcpy((s1), (s2)), s1 + strlen((s2)))
	#define strcmpi         stricmp
	#define strset          _strset
	#define strupr          _strupr
	#define strlwr          _strlwr
	#define strdup          _strdup
	#define fcvt _fcvt
	#define itoa            _itoa
	#define ltoa            _ltoa
	#define ultoa           _ultoa
	#define pow10l(x) pow(10,(x))
	#define pow10(x) pow(10,(x))
	#define MAXDRIVE 3
	#define MAXFILE 257
	#define MAXEXT 5
	#define MAXDIR 257
	#define MAXPATH 261
#endif
// } @v5.0.4 AHTOXA
#ifndef SLIB_DONT_UNDEF_MINMAX // @v11.1.11
	#undef min // @v9.8.11
	#undef max // @v9.8.11
#endif
#ifdef SLIB_INCLUDE_CPPSTDLIBS
	#include <list>
	#include <deque>
	#include <vector>
	#include <map>
	#include <unordered_map>
	#include <algorithm>
	#include <ios>
	#include <ostream>
	#include <istream>
	#include <string>
	#include <locale>
	#include <iostream>
	#include <sstream>
	#include <fstream>
	#include <type_traits>
	#include <iomanip>
	#include <exception>
	#include <stdexcept>
	#include <cassert>
	#include <cwchar>
	#include <codecvt>
	#include <typeinfo>
	#include <memory>
	#include <iterator>
	#include <utility>
	#include <regex>
	#include <unordered_set>
	#include <tuple>
	#include <array>
	#include <deque>
	#include <stack>
	#include <cctype>
	#include <cstdint>
	#include <cinttypes>
	#include <limits>
	#include <ratio>
	#include <chrono>
	#include <functional>
	#include <mutex>
	#include <random>
	#include <cwctype>
	#include <cstdlib>
	#include <cmath>
	#include <climits>
	#include <cstdarg>
	#include <cstddef>
	#include <set>
	#include <ostream>
	#include <clocale>
	#include <cstdio>
	#include <cstring>
	#include <iosfwd>
	#include <thread>
	#include <ios>
	#include <condition_variable>
	#include <ctime>
	#include <csignal>
	#include <future>
	#include <queue>
	#include <cerrno>
	#include <system_error>
	#include <forward_list>
	#include <numeric>
#endif

//#if _MSC_VER >= 1910
#if CXX_HAS_STATIC_ASSERT
	#define STATIC_ASSERT static_assert
#else
	#define STATIC_ASSERT(_boolExpr) { enum { __StaticAssertDummyEnum = 1/static_cast<int>(!!(_boolExpr)) }; } // note : use after variable declarations
	//#define STATIC_ASSERT assert
#endif

// @v10.8.2 #define FNF_WILDCARDS 0x01
// @v10.8.2 #define FNF_EXTENSION 0x02
// @v10.8.2 #define FNF_FILENAME  0x04
#define FNF_DIRECTORY 0x08
#define FNF_DRIVE     0x10

#define DMAXEXP     308
#define FMAXEXP     38
#define DMINEXP     -307
#define FMINEXP     -37
#if(__BORLANDC__ < 0x500)
	// @v10.8.2 (replaced with SMathConst::Max) #define MAXDOUBLE   1.797693E+308
	// @v10.8.2 (replaced with SMathConst::Min) #define MINDOUBLE   2.225074E-308
	// @v10.9.6 #define MAXFLOAT    3.37E+38f
	// @v10.9.6 #define MINFLOAT    8.43E-37
#endif
#define DSIGNIF     53
#define FSIGNIF     24
#define DMAXPOWTWO  0x3FF
#define FMAXPOWTWO  0x7F
#define _DEXPLEN    11
#define _FEXPLEN    8
#define _EXPBASE    2
#define _IEEE       1
#define _LENBASE    1
#define HIDDENBIT   1
// @v10.8.2 (use SMathConst::LogMax insted) #define LN_MAXDOUBLE    7.0978E+2
// @v10.8.2 (use SMathConst::LogMin insted) #define LN_MINDOUBLE    -7.0840E+2
#ifdef _MSC_VER
	#define _argc __argc
	#define _argv __argv
	#pragma warning(disable : 4068)
#endif
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
//
// @v11.2.3
// Descr: Мнемоники специальных символов
//
#define CHR_SOH 0x01 // Start Of Heading
#define CHR_STX 0x02 // Start Of Text
#define CHR_ETX 0x03 // End Of Text
#define CHR_EOT 0x04 // End Of Transmission
#define CHR_ENQ 0x05 // Enquiry
#define CHR_ACK 0x06 // Acknowledge
#define CHR_NAK 0x15 // Negative Acknowledge
#define CHR_CAN 0x18 // Прервать выполнение отчета
//
// Descr: Далее следует список макроопределений, используемых в разнообразных формулах и парсерах.
//   Крайне желательно при написании очередного парсера использовать именно эти определения.
// Note: Значения макросов НЕ МЕНЯТЬ! На некоторые из них ссылается программный код неявно!
// DON'T CHANGE MACROS VALUES. Some of them are refered by the program code implicitly!
//
#define _UNDEF_              -1 //
#define _NONE_                0 //
#define _EQ_                  1 // ==
#define _OUTER_EQ_          101 // outer join
#define _GT_                  2 // >
#define _LT_                  3 // <
#define _NE_                  4 // !=
#define _GE_                  5 // >=
#define _LE_                  6 // <=
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
#define _PLUS_               19 // a + b
#define _MINUS_              20 // a - b
#define _MULT_               21 // a * b
#define _DIVIDE_             22 // a / b
#define _MODULO_             23 // a % b
#define _XOR_                26 // ^
#define _SHIFTLEFT_          28 // <<
#define _SHIFTRIHT_          29 // >>
#define _ASSIGN_             30 // присваивание (что-то вроде a := b)
#define _DOT_                31 // Оператор . Чаще всего используется как ссылка на компонент сложного типа. Например data.member
#define _END___              32 // 0 //
#define _AND___              33 // 1 //
#define _OR___               34 // 2 //
#define _NOT___              35 // 3 // !
#define _ALL___              36 // 0 //
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
// Фронтальная ориентация прямоугольника по отношению к зрителю (проще говоря, ориентация листа или экрана)
//
#define RECTORIENTATION_PORTRAIT  DIREC_VERT
#define RECTORIENTATION_LANDSCAPE DIREC_HORZ
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
// Descr: Направления выравнивания //
//
#define ADJ_LEFT              0 // По левому краю
#define ADJ_RIGHT             1 // По правому краю
#define ADJ_CENTER            2 // По центру
#define ADJ_ALIGN             3 // @v10.4.8 Выравнивание по всему направлению (по длине или высоте) с внутренней набивкой пространством между элементами
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
#define UNIT_COLOR           14 // Специфическая единица - количество цветов в наборе
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
// Половая принадлежность
// @persistent
//
#define GENDER_UNDEF          0 // Не определена (в принципе не известно из-за того, что не было попыток идентифицировать)
#define GENDER_MALE           1 // Мужской
#define GENDER_FEMALE         2 // Женский
#define GENDER_QUESTIONING    3 // Под вопросом (попытка идентификации пола была, но окончилась неудачей)
//
// Descr: Агрегатный функции для комбинирования выборки значений
// @persistent
//
#define AGGRFUNC_NONE         0
#define AGGRFUNC_COUNT        1
#define AGGRFUNC_SUM          2
#define AGGRFUNC_AVG          3 // Среднее арифметическое
#define AGGRFUNC_MIN          4
#define AGGRFUNC_MAX          5
#define AGGRFUNC_STDDEV       6 // @v10.5.6 Стандартное отклонение
//
//
//
#define WIN32_LEAN_AND_MEAN
//#define _WIN32_WINNT 0x0501
//#define _WIN32_IE 0x0501 // @v9.0.8
//
/* @v10.9.3 (moved to slport.h) #if !defined(FORCEINLINE)
	#if defined(_MSC_VER)
		#define FORCEINLINE __forceinline
	#else
		#define FORCEINLINE
	#endif
#endif*/
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
	// @v10.9.1 #define SFAR
	#define SLAPI_unused    // (since @v10.9.1 unused) __stdcall Тип вызова __stdcall позволяет уменьшить размер бинарного кода, но
		// снижает скорость работы member-функция из-за того, что this передается на стеке, а не в регистре
	// @v11.2.0 (moved to slport.h) #define STDCALL  __stdcall // @v11.2.0 не применять для member-функций: снижает производительность
	// @v11.2.0 (moved to slport.h) #define SLAPIV   __cdecl
	// @v11.2.0 (moved to slport.h) #define FASTCALL __fastcall // Следует использовать для часто вызываемых функций с количеством параметров
		// от 1 до 3. Не совсем понятны побочные эффекты от использования регистров под параметры с точки
		// зрения оптимизации распределения регистров в вызывающих функциях.
		// @v11.2.0 Небольшое изучение замены __fastcall на __stdcall показало:
		//  размер кода при такой замене снижается значительно, но общее быстродействие становиться на несколько процентов меньше.
		//  Пока оставляем как было.
		//  При этом есть резон standalone-функции с любым числом аргументом объявлять как __stdcall - это снизит общий размер кода.
#else
	#ifndef PASCAL
		#define PASCAL           pascal
	#endif
	// @v10.9.1 #define SFAR                 far
	#define SLAPI_unused         pascal SFAR // (since @v10.9.1 unused)
	// @v11.2.0 (moved to slport.h) #define STDCALL              // @v11.2.0
	// @v11.2.0 (moved to slport.h) #define SLAPIV               cdecl SFAR
	// @v11.2.0 (moved to slport.h) #define FASTCALL
#endif
#ifdef _MSC_VER
	#define THREADLOCAL __declspec(thread)
#else
	#define THREADLOCAL
#endif
#ifndef MAXPATH
	#define MAXPATH 260 // @v10.9.1
	/* @v10.9.1
	#if defined(__WIN32__) || defined(_WIN32)
		#define MAXPATH 260
	#else
		#define MAXPATH  80
	#endif */
#endif
#define CALLPTRMEMB(ptr, memb)          if(ptr) ptr->memb;
#define CALLTYPEPTRMEMB(typ, ptr, memb) if(ptr) ((typ *)ptr)->memb;
//
//
//
#define ASSIGN_PTR(ptr,val)   if(ptr) *(ptr) = (val);
#define RVALUEPTR(val,ptr)    ((ptr)?((val = *(ptr)), 1) : 0)
#define DEREFPTROR(ptr,alt)   ((ptr)?(*(ptr)):(alt))
#define DEREFPTRORZ(ptr)      DEREFPTROR(ptr,0)
#define CENUMINCR(et, v)      (v) = static_cast<et>(static_cast<int>(v)+1) // @v11.3.8 Макрос для обхода ошибки c++ при инкременте enum-value, унаследованного из c-кода
#define PTR8(p)               (reinterpret_cast<uint8 *>(p))
#define PTR16(p)              (reinterpret_cast<uint16*>(p))
#define PTR32(p)              (reinterpret_cast<uint32 *>(p))
#define PTR64(p)              (reinterpret_cast<uint64*>(p))
#define PTRDBL(p)             (static_cast<double*>(p))
#define PTR8C(p)              (reinterpret_cast<const uint8 *>(p))
#define PTR16C(p)             (reinterpret_cast<const uint16*>(p))
#define PTR32C(p)             (reinterpret_cast<const uint32 *>(p))
#define PTR64C(p)             (reinterpret_cast<const uint64*>(p))
#define PTRDBLC(p)            (reinterpret_cast<const double*>(p))
#define PTRCHR(p)             (static_cast<char *>(p)) // @v10.7.8
#define PTRCHRC(p)            (static_cast<const char *>(p)) // @v10.7.8
#define PTRCHR_(p)            (reinterpret_cast<char *>(p)) // @v10.7.8
#define PTRCHRC_(p)           (reinterpret_cast<const char *>(p)) // @v10.7.8
#define PTR2U(p)              (static_cast<uint32>(p))
#define U2PTR(typ,u)          (static_cast<typ *>(u))
#define U2PTRV(u)             (static_cast<void *>(u))
#define ALIGNSIZE(s,bits)     ((((s) + ((1 << (bits))-1)) >> (bits)) << (bits))
#ifndef MIN
	#define MIN(a,b)          ((a)<(b)?(a):(b))
#endif
#ifndef MAX
	#define MAX(a,b)          ((a)>(b)?(a):(b))
#endif
#define LoByte(w)             (static_cast<uint8>(w))
#define HiByte(w)             (static_cast<uint8>(static_cast<uint16>(w) >> 8))
#define LoWord(l)             (static_cast<uint16>(l))
#define HiWord(l)             (static_cast<uint16>(static_cast<uint32>(l) >> 16))
#define MakeLong(low,high)    (static_cast<long>((static_cast<uint16>(low)) | ((static_cast<ulong>(static_cast<uint16>(high))) << 16)))

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
#define oneof12(f,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9)||(f)==(c10)||(f)==(c11)||(f)==(c12))
#define oneof13(f,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9)||(f)==(c10)||(f)==(c11)||(f)==(c12)||(f)==(c13))
#define oneof14(f,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14) ((f)==(c1)||(f)==(c2)||(f)==(c3)||(f)==(c4)||(f)==(c5)||(f)==(c6)||(f)==(c7)||(f)==(c8)||(f)==(c9)||(f)==(c10)||(f)==(c11)||(f)==(c12)||(f)==(c13)||(f)==(c14))
#define NZOR(v,a)                   ((v)?(v):(a))
#define NZSTR(v)                    ((v)?(v):"") // @v11.3.12
//#define NZOR_F(v,a)                 ((!!(v))?(v):(a))
// Использование SETIFZ может инициировать нежелательное замечание компилятора "assignment within conditional expression" (MSVC C4706).
// Если этот побочный эффект неприемлем, то используйте SETIFZQ (в этом случае применить его в условном выражении не удасться.
#define SETIFZ(v,a)  (!!(v) || !!((v)=(a)))
#define SETIFZQ(v,a) {if(!(v)) (v)=(a);} // @v11.1.12
#define SETMAX(v,b) {if((v)<(b)) (v)=(b);}
#define SETMIN(v,b) {if((v)>(b)) (v)=(b);}
#define SETMINMAX(v,i,x) {if((v)>(x)) (v)=(x); else if((v)<(i)) (v)=(i);}
#define MINMAX(v,i,x)               (((v)<(i)) ? (i) : (((v)>(x)) ? (x) : (v)))
#define ICEIL(v,quant)              (((v)+((quant)-1))/(quant)*(quant))
#define IFLOOR(v,quant)             ((v)/(quant)*(quant))

#define SETFLAG(v,f,b) {if(b) {(v)|=(f);} else {(v)&=~(f);}}
#define SETFLAGBYSAMPLE(v,f,b)      {if((b)&(f)) {(v)|=(f);} else {(v)&=~(f);}}
#define TESTFLAG(v1,v2,f)           (((v1)&(f))==((v2)&(f)))
#define TESTMULTFLAG(v,f)           (((v)&(f))==(f))
#define INVERSEFLAG(v,f) {if((v)&(f)) {(v)&=~(f);} else {(v)|=(f);}}
// Descr: Если в поле v установлены оба флага f1 и f2 либо не установлен ни один из них, то возвращает 0, в противном случае - значение одного из установленных флагов.
#define CHKXORFLAGS(v,f1,f2)        ((((v)&(f1)) ^ ((v)&(f2))) ? (((v)&(f1))?(f1):(f2)) : 0)
#define SETUPFLAGS(value, andFlags, notFlags) { (value) |= (andFlags); (value) &= ~(notFlags); }
#define BIN(x)                      ((x)?1:0)
#define LOGIC(x)                    ((x)?true:false)
#define BOOLXOR(p1,p2)              (((p1)&&(p2))||(!(p1)&&!(p2)))
#define DELETEANDASSIGN(p, n)       ((delete (p)), (p) = (n))
#define ZDELETE(p)                  DELETEANDASSIGN(p, 0)
#define ZDELETEFAST(p) {if(p) ((delete (p)), (p) = 0);}
#define ZDELETEARRAY(p)             ((delete [] (p)), (p) = 0)
#define FREEANDASSIGN(p, n)         (SAlloc::F(p), (p) = (n))
#define ZFREE(p)                    FREEANDASSIGN((p), 0)
//
// Exception handling
//
#define EXCEPTVAR(intvar)      int*__sexcept=&(intvar)
#define EXCEPT                 *__sexcept
#define CALLEXCEPT()           goto __scatch;
#define CALLEXCEPT_S(val)      {SLS.SetError(val);goto __scatch;}
#define CALLEXCEPT_S_S(val,s) {SLS.SetError(val, s);goto __scatch;}
#define CALLEXCEPTV(val) {*__sexcept=val;goto __scatch;}
#define THROW(expr) {if(!(expr)){goto __scatch;}}
#define THROWEC(expr) {if(!!(expr)){goto __scatch;}}
#define THROW_V(expr,val)      {if(!(expr)){*__sexcept=val;goto __scatch;}}
#define THROW_S(expr,val)      {if(!(expr)){SLS.SetError(val);goto __scatch;}}
#define THROW_S_S(expr,val,s) {if(!(expr)){SLS.SetError(val, s);goto __scatch;}}
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

#define SLERR_FILETOOBIG                95 // @v11.3.4 Файл '%s' слишком большой
#define SLERR_INVPARAM                  96 // Недопустимый параметр функции (%s)
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
#define SLERR_EXCL_SAVEFAULT           115 // @v10.9.6 Ошибка сохранения файла Excel '%s'

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
#define SLERR_BINSET_SRCIDNFOUND       197 // Ошибка SBinarySet: в исходном пуле отсутствует отрезок с идентификатором %s
#define SLERR_BINSET_UNSRLZ_SIGNATURE  198 // Ошибка чтения SBinarySet: неверный заголовок или сигнатура
#define SLERR_BINSET_UNSRLZ_ITEMHDR    199 // Ошибка чтения SBinarySet: инвалидный заголовок элемента

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
#define SLERR_LANGSYMBNFOUND           219 // Не найден символ языка '%s'
#define SLERR_DISPIFCCLI               220 // Ошибка вызова dispatch-интерфейса %s
//
//
//
#define SLERR_MATH_DOMAIN                      300 // Ошибка области определения функции (sqrt(-1))
#define SLERR_MATH_ROUND                       301 // Ошибка округления //
#define SLERR_MATH_OVERFLOW                    302 // Ошибка переполнения ieee-числа
#define SLERR_MATH_UNDERFLOW                   303 // Ошибка переполнения нижней границы точности ieee-числа
#define SLERR_MATH_MAXITER                     304 // Ошибка превышения максимального числа итераций
#define SLERR_MATH_TSVECBYSYMBNFOUND           305 // Вектор временной серии с символом '%s' не найден
#define SLERR_BASE64DECODEFAULT                306 // Ошибка декодирования base64
//
//
//
#define SLERR_INVDBSYMB                        400 // Неверный символ базы данных '%s'
#define SLERR_UTFCVT_SRCEXHAUSTED              401 // Не полный входной буфер UTF8
#define SLERR_UTFCVT_ILLUTF8                   402 // Недопустимый входной символ UTF8
#define SLERR_UTFCVT_ILLUTF16                  403 // Недопустимый входной символ UTF16
#define SLERR_TAB_NFOUND                       404 // Таблица STab '%s' не найдена
#define SLERR_INVIMAGEPIXFORMAT                405 // Недопустимый формат пикселя.
#define SLERR_IMAGEFILENOTJPEG                 406 // Файл '%s' не является JPEG-файлом.
#define SLERR_JPEGLOADFAULT                    407 // Ошибка загрузки JPEG-Файла '%s'
#define SLERR_IMAGEFILENOTPNG                  408 // Файл '%s' не является PNG-файлом.
#define SLERR_PNGLOADFAULT                     409 // Ошибка загрузки PNG-изображения: '%s'
#define SLERR_UNSUPPIMGFILEFORMAT              410 // Не поддерживаемый или недопустимый формат файла изображения: '%s'
#define SLERR_SFILRDSIZE                       411 // Ошибка чтения из файла: размер считанных данных не равен запрошенному (%s)
#define SLERR_IMAGEFILENOTICO                  412 // Файл '%s' не является ICO-файлом.
#define SLERR_PNGSTOREFAULT                    413 // Ошибка сохранения PNG-изображения: '%s'
#define SLERR_INVSDRAWFIGKIND                  414 // Internal: Недопустимый вид графической фигуры (%s)
#define SLERR_DUPDRAWGROUPSYMB                 415 // Дублирование символа '%s' элемента, добавляемого в SDrawGroup
#define SLERR_WTMTA_UNDEFFIG                   416 // Для элемента TWhatmanToolArray '%s' не заданы ни фигура, ни иконка.
#define SLERR_WTMTA_BADCRC                     417 // Нарушена целостность WTA-файла '%s'
#define SLERR_IMAGEFILENOTBMP                  418 // Файл '%s' не является BMP-файлом.
#define SLERR_INVBMPHEADER                     419 // Недопустимый BMP-заголок (%s)
#define SLERR_BMPCOMPRNSUPPORTED               420 // Сжатые BMP-изображения не поддерживаются.
#define SLERR_SOAPR_UNDEFREF                   421 // Неопределенная ссылка '%s' в SOAP-результате
#define SLERR_SOAPR_UNDEFTYPE                  422 // Неопределенный тип данных '%s' в SOAP-результате
#define SLERR_SOAPR_ITEMREFNFOUND              423 // Не найден элемент SOAP-результата по ссылке '%s'
#define SLERR_SOAPR_ITEMREFTYPECONFL           424 // Конфликт между атрибутами ref и type (%s) в элементе SOAP-ответа
#define SLERR_SOPAR_UNRESITEMHASNTREF          425 // Неразрешенный элемент структуры (%s) SAOP-результата не имеет ссылки
#define SLERR_SOAPR_UNRESOLVEDITEM             426 // Не удалось резрешить элемент структуры (%s) SOAP-результата
#define SLERR_SOAPR_ITEMNAMENFOUND             427 // Элемент SOAP-пакета '%s' не найден
#define SLERR_SOAPR_INVITEMPOS                 428 // Недопустимый индекс (%s) элемента SOAP-пакета
#define SLERR_WTMTA_INVSIGNATURE               429 // Неверная сигнатура WT-файла '%s'
#define SLERR_USB_HIDUSBCLASSFAILED	           430 // Устройство не относится к классу USB или HID
#define SLERR_SFILRDNULLOUTP                   431 // Попытка чтения из NullOutput-файла
#define SLERR_MAIL_NOTCONNECTED                432 // Внутренняя ошибка - не установлено соединение почтового клиента
#define SLERR_MAIL_INVPROTOCOL                 433 // Внутренняя ошибка - недопустимый протокол '%s' почтового клиента
#define SLERR_MAIL_SMTP_NOREPLY                434 // Почтовый сервер SMTP не ответил на запрос.
#define SLERR_MAIL_SMTP_REPLYERR               435 // Почтовый сервер SMTP вернул ошибку %s.
#define SLERR_MAIL_POP3_NOREPLY                436 // Почтовый сервер POP3 не ответил на запрос.
#define SLERR_MAIL_POP3_REPLYERR               437 // Почтовый сервер POP3 вернул ошибку '%s'
#define SLERR_MAIL_POP3_UNDEFREPLY             438 // Почтовый сервер POP3 вернул не известный ответ '%s'
#define SLERR_FTP_NOTCONNECTED                 451 // FTP-соединение не установлено

#define SLERR_PUNYCODE_BADINPUT                452 // PUNYCODE Input is invalid
#define SLERR_PUNYCODE_BIGOUTPUT               453 // PUNYCODE Output would exceed the space provided
#define SLERR_PUNYCODE_OVERFLOW                454 // PUNYCODE Wider integers needed to process input
#define SLERR_CURL                             455 // Ошибка библиотеки libcurl
#define SLERR_LIBXML                           456 // Ошибка библиотеки libxml
#define SLERR_DUPSYMBWITHUNEQID                457 // Дублирование символа '%s' элемента с отличным идентификатором
#define SLERR_INVIMAGESIZE                     458 // Недопустимый размер изображения ([1..30000], [1..30000])
#define SLERR_WTM_DUPLAYOUTSYMB                459 // @v10.4.9 Дублирование символа layout
#define SLERR_STRINGISNTUTF8                   460 // @v11.3.3 Строка - не в кодировке utf-8
#define SLERR_LMDB                             461 // @v11.3.6 Ошибка базы данных LMDB
#define SLERR_FILSTRG_INVHANDLE                462 // @v11.3.7 Внутренняя ошибка: недопустимый манипулятор чтения/записи объекта SFileStorage
#define SLERR_FILSTRG_INVNAME                  463 // @v11.3.7 Имя сохраняемого файла объекта SFileStorage должно быть не длиннее 255 символов и содержать только [0-9a-z_+-,.=@]
#define SLERR_FILSTRG_FILENFOUND               464 // @v11.3.7 Файл с именем '%s' в хранилище SFileStorage отсутствует
#define SLERR_INVORUNSUPPHASHFUNC              465 // @v11.3.7 Недопустимая либо неподдерживаемая хэш-функция // 
#define SLERR_INVALIDSFILTYP                   466 // @v11.4.3 Внутренняя ошибка: недопустимый тип экземпляра SFile

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
#define SLERR_FANN_INVLAYERCOUNT               521 // Недопустимое количество слоев нейронной сети
#define SLERR_FANN_INVLAYERSIZE                522 // Недопустимый размер слоя нейронной сети
#define SLERR_FANN_INVTRAINALG                 523 // Недопустимый алгоритм обучения нейронной сети
//#define SLERR_JSON_OK = 1,                  // everything went smoothly
//#define SLERR_JSON_MEMORY,                   // an error occurred when allocating memory
#define SLERR_JSON_INCOMPLETE_DOCUMENT         550 // the parsed document didn't ended
#define SLERR_JSON_WAITING_FOR_EOF             551 // A complete JSON document tree was already finished but needs to get to EOF. Other characters beyond whitespaces produce errors
#define SLERR_JSON_MALFORMED_DOCUMENT          552 // the JSON document which was fed to this parser is malformed
#define SLERR_JSON_INCOMPATIBLE_TYPE           553 // the currently parsed type does not belong here
#define SLERR_JSON_ILLEGAL_CHARACTER           554 // the currently parsed character does not belong here
#define SLERR_JSON_BAD_TREE_STRUCTURE          555 // the document tree structure is malformed
#define SLERR_JSON_MAXIMUM_LENGTH              556 // the parsed string reached the maximum allowed size
#define SLERR_JSON_UNKNOWN_PROBLEM             557 // some random, unaccounted problem occurred
#define SLERR_JSON_EMPTYDOCUMENT               558 // @v11.4.3 the parsed document is empty

#define SLERR_URI_SYNTAX                       570 // Parsed text violates expected format
#define SLERR_URI_NULL                         571 // One of the params passed was NULL although it mustn't be
//#define SLERR_URI_MALLOC                     572 // Requested memory could not be allocated
#define SLERR_URI_OUTPUT_TOO_LARGE             573 // Some output is to large for the receiving buffer
#define SLERR_URI_NOT_IMPLEMENTED              574 // The called function is not implemented yet
#define SLERR_URI_RANGE_INVALID                575 // The parameters passed contained invalid ranges
// Errors specific to ToStr
#define SLERR_URI_TOSTRING_TOO_LONG        SLERR_URI_OUTPUT_TOO_LARGE // Deprecated, test for URI_ERROR_OUTPUT_TOO_LARGE instead
// Errors specific to AddBaseUri
#define SLERR_URI_ADDBASE_REL_BASE             576 // Given base is not absolute
// Errors specific to RemoveBaseUri
#define SLERR_URI_REMOVEBASE_REL_BASE          577 // Given base is not absolute
#define SLERR_URI_REMOVEBASE_REL_SOURCE        578 // Given base is not absolute

//#define SLERR_ZIP_OK             0  // N No error
#define SLERR_ZIP_FIRSTERROR                   591  // @anchor
#define SLERR_ZIP_MULTIDISK                    591  // N Multi-disk zip archives not supported
#define SLERR_ZIP_RENAME                       592  // S Renaming temporary file failed
#define SLERR_ZIP_CLOSE                        593  // S Closing zip archive failed
#define SLERR_ZIP_SEEK                         594  // S Seek error
#define SLERR_ZIP_READ                         595  // S Read error
#define SLERR_ZIP_WRITE                        596  // S Write error
#define SLERR_ZIP_CRC                          597  // N CRC error
#define SLERR_ZIP_ZIPCLOSED                    598  // N Containing zip archive was closed
#define SLERR_ZIP_NOENT                        599  // N No such file
#define SLERR_ZIP_EXISTS                       600  // N File already exists
#define SLERR_ZIP_OPEN                         601  // S Can't open file
#define SLERR_ZIP_TMPOPEN                      602  // S Failure to create temporary file
#define SLERR_ZIP_ZLIB                         603  // Z Zlib error
#define SLERR_ZIP_MEMORY                       604  // N Malloc failure
#define SLERR_ZIP_CHANGED                      605  // N Entry has been changed
#define SLERR_ZIP_COMPNOTSUPP                  606  // N Compression method not supported
#define SLERR_ZIP_EOF                          607  // N Premature end of file
#define SLERR_ZIP_INVAL                        608  // N Invalid argument
#define SLERR_ZIP_NOZIP                        609  // N Not a zip archive
#define SLERR_ZIP_INTERNAL                     610  // N Internal error
#define SLERR_ZIP_INCONS                       611  // N Zip archive inconsistent
#define SLERR_ZIP_REMOVE                       612  // S Can't remove file
#define SLERR_ZIP_DELETED                      613  // N Entry has been deleted
#define SLERR_ZIP_ENCRNOTSUPP                  614  // N Encryption method not supported
#define SLERR_ZIP_RDONLY                       615  // N Read-only archive
#define SLERR_ZIP_NOPASSWD                     616  // N No password provided
#define SLERR_ZIP_WRONGPASSWD                  617  // N Wrong password provided
#define SLERR_ZIP_OPNOTSUPP                    618  // N Operation not supported
#define SLERR_ZIP_INUSE                        619  // N Resource still in use
#define SLERR_ZIP_TELL                         620  // S Tell error
#define SLERR_ZIP_COMPRESSED_DATA              621  // @v10.8.4 N Compressed data invalid
#define SLERR_ZIP_CANCELLED                    622  // @v10.8.4 N Operation cancelled
#define SLERR_ZIP_LASTERROR                    622  // @anchor @v10.8.4 620-->622
#define SLERR_ZLIB_BUFINFLATEFAULT             623  // @v11.2.9 Ошибка распаковки буфера данных

#define SLERR_CRYPTO_INVPARAM                  651  // @v11.0.11 Ошибка SLCRYPTO - недопустимый параметр функции
#define SLERR_CRYPTO_KEYBUFOVRFLW              652  // @v11.0.11 Ошибка SLCRYPTO - переполнение внутреннего буфера ключа
#define SLERR_CRYPTO_INVKEYSIZE                653  // @v11.0.11 Ошибка SLCRYPTO - недопустимая длина ключа шифрования
#define SLERR_CRYPTO_INVIVSIZE                 654  // @v11.0.11 Ошибка SLCRYPTO - недопустимая длина IV
#define SLERR_CRYPTO_OPENSSL                   655  // @v11.0.11 Ошибка OpenSSL
// Коды с префиксом SLERR_AMQP_ являются трансляцией кодов состояний библиотеки rabbitmq-c
#define SLERR_AMQP_UNKN                        700 // Неизвестный AMQP-статус
#define SLERR_AMQP_BAD_AMQP_DATA               701
#define SLERR_AMQP_UNKNOWN_CLASS               702
#define SLERR_AMQP_UNKNOWN_METHOD              703
#define SLERR_AMQP_HOSTNAME_RESOLUTION_FAILED  704
#define SLERR_AMQP_INCOMPATIBLE_AMQP_VERSION   705
#define SLERR_AMQP_CONNECTION_CLOSED           706
#define SLERR_AMQP_BAD_URL                     707
#define SLERR_AMQP_SOCKET                      708
#define SLERR_AMQP_INVALID_PARAMETER           709
#define SLERR_AMQP_TABLE_TOO_BIG               710
#define SLERR_AMQP_WRONG_METHOD                711
#define SLERR_AMQP_TIMEOUT                     712
#define SLERR_AMQP_TIMER_FAILURE               713
#define SLERR_AMQP_HEARTBEAT_TIMEOUT           714
#define SLERR_AMQP_UNEXPECTED_STATE            715
#define SLERR_AMQP_SOCKET_CLOSED               716
#define SLERR_AMQP_SOCKET_INUSE                717
#define SLERR_AMQP_BROKER_UNSUPP_SASL_METHOD   718
#define SLERR_AMQP_UNSUPPORTED                 719
#define SLERR_AMQP_TCP                         720
#define SLERR_AMQP_TCP_SOCKETLIB_INIT          721
#define SLERR_AMQP_SSL                         722
#define SLERR_AMQP_SSL_HOSTNAME_VERIFY_FAILED  723
#define SLERR_AMQP_SSL_PEER_VERIFY_FAILED      724
#define SLERR_AMQP_SSL_CONNECTION_FAILED       725
// Коды ошибок разбора телефонного номера библиотекой libphonenumber
#define SLERR_PHONENMB_INV_COUNTRY_CODE        760 // INVALID_COUNTRY_CODE in the java version.
#define SLERR_PHONENMB_NOT_A_NUMBER            761
#define SLERR_PHONENMB_TOO_SHORT_AFTER_IDD     762
#define SLERR_PHONENMB_TOO_SHORT_NSN           763
#define SLERR_PHONENMB_TOO_LONG_NSN            764 // TOO_LONG in the java version.

#define SLERR_ERRNO_OFFSET                    2000 // Смещение с которого начинаются коды ошибок, соответствующие posix-мнемоникам (errno)
#define SLERR_ERRNO_LAST                      2999 // Максимальное значение для posix errno (если я правильно понял, то errno не имеет официального максимуму потому использую свой собственный).

// LastError
//
// @ModuleDecl(SType) {
// Система динамических типов. Дескриптор типа является 4-байтовым
// значение, хранящим идентификатор типа и размер.
// Таким образом, максимальный размер одного поля данных составляет 0xffff байт.
// Для очень больших данных может быть использован S_BLOB или S_CLOB (управление этими типами достаточно сложное).
//
#define INVALID_DATA_SIZE 0
//
// @v11.0.4
// Type cast results
//
#define STCASTF_NORMAL             0 // Нормальное преобразование
#define STCASTF_UNDEF             -1 // Результат неопределен (невозможное преобразование)
#define STCASTF_POT_OVERFLOW  0x0001 // Потенциальное переполнение
#define STCASTF_OVERFLOW      0x0002 // Фактическое переполнение
#define STCASTF_POT_UNDERFLOW 0x0004 // Потенциальное нижнее переполнение
#define STCASTF_UNDERFLOW     0x0008 // Фактическое нижнее переполнение
#define STCASTF_POT_LOSS      0x0010 // Потенциальная потеря точности
#define STCASTF_LOSS          0x0020 // Фактическая потеря точности
//
// Base types. Базовые типы, используемые для приведения и преобразования разных типов.
//
#define BTS_VOID             0
#define BTS_STRING           1    // ASCIIZ
#define BTS_INT              2    // 4 byte
#define BTS_REAL             3    // 8 byte IEEE float
#define BTS_DATE             4    // 4 byte LDATE
#define BTS_TIME             5    // 4 byte LTIME
#define BTS_DATETIME         6    // 8 byte LDATETIME
#define BTS_POINT2           7    // 16 byte SPoint2R
#define BTS_BOOL             8    // 4 byte Специализированный вариант. Фактически не применяется как базовый тип, но используется в локальных ситуациях.
#define BTS_PTR              9    // Указатель. Важно, что размер его зависим от архитектуры (4 или 8 байт)
#define BTS_INT64_          10    // 8 byte integer. Не является базовым типом. Определен для использования в DBConst
//
// Data types
//
#define S_VOID               0
#define S_CHAR               1
#define S_INT                2
#define S_FLOAT              3
#define S_DATE               4
#define S_TIME               5
#define S_DEC                6
#define S_MONEY              7
#define S_LOGICAL            8
#define S_BOOL               S_LOGICAL
#define S_NUMERIC            9
#define S_BFLOAT            10
#define S_LSTRING           11
#define S_ZSTRING           12
#define S_NOTE              13      // Для BTRIEVE - символьное поле переменной длины (последнее в таблице), для SQL - VARCHAR. Максимальная бинарная (с завершающим нулем) длина специфицируется //
#define S_LVAR              14
#define S_UBINARY           15
#define S_UINT              S_UBINARY
#define S_AUTOINC           16
#define S_BIT               17
#define S_STS               18

#define S_INTRANGE          19		// int2 range only
#define S_REALRANGE         20		// double range only
#define S_DATERANGE         21      //
#define S_DATETIME	        22      //
#define S_ARRAY		        23		// not real STYPE, only ID. Массив, состоящий из данных одного типа
#define S_STRUCT	        24		//--//--
#define S_VARIANT           25      //
#define S_WCHAR             26      // wide char-строка (оканчивается 0). Размер данных указывается в байтах.
#define S_BLOB              27      // BLOB (для SQL-таблиц)
#define S_CLOB              28      // CLOB (для SQL-таблиц)
#define S_RAW               29      // RAW-data бинарное поле с произвольным содержимым
#define S_ROWID             30      // ROWID (для SQL-таблиц)
#define S_IPOINT2           31      // Целочисленная двумерная точка (x, y)
#define S_FPOINT2           32      // Вещественная двумерная точка (x, y)
#define S_WZSTRING          33      // Unicode-zstring
#define S_UUID_             34      // (Суффиксное подчеркивание необходимо для отличия от class S_GUID)
#define S_INT64             35      // @v10.6.3
#define S_UINT64            36      // @v10.6.3
#define S_COLOR_RGBA        37      // @v10.9.10 4-компонентное представление цвета ARGB class SColorBase
//
#define S_FIRSTUSERTYPE     50
//
// Максимальное количество типов в таблице
//
#define TYPE_TAB_SIZ 64

#define S_ARRAY_INT  0x82
#define S_ARRAY_DBL  0x83

#define MKSTYPE(typ,siz)      (((static_cast<uint>(siz))<<16)|(typ))
#define MKSTYPED(typ,siz,prc) (((prc)<<24)|((static_cast<uint>(siz))<<16)|(typ))
#define GETSTYPE(typ)         ( (typ)&0x0000ffff)
#define GETSSIZE(typ)         (((typ)&0xffff0000)>>16)
#define GETSSIZED(typ)        (((typ)&0x00ff0000)>>16)
#define GETSPRECD(typ)        (((typ)&0xff000000)>>24)

#define T_CHAR             MKSTYPE(S_CHAR,1)
#define T_INT              MKSTYPE(S_INT,2)
#define T_UINT             MKSTYPE(S_UBINARY,2)
#define T_UINT16           MKSTYPE(S_UBINARY,2)
#define T_UINT32           MKSTYPE(S_UBINARY,4)
#define T_UINT64           MKSTYPE(S_UBINARY,8)
#define T_INT16            MKSTYPE(S_INT,2)
#define T_INT32            MKSTYPE(S_INT,4)
#define T_INT64            MKSTYPE(S_INT64 ,8) // @v10.6.3 MKSTYPE(S_INT ,8)-->MKSTYPE(S_INT64 ,8)
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
#define T_BOOL             MKSTYPE(S_BOOL, 4)
#define T_GUID             MKSTYPE(S_UUID_, 16)
#define T_COLOR_RGBA       MKSTYPE(S_COLOR_RGBA, 4) // @v10.9.10
//
//
//
typedef signed char    int8;
typedef signed short   int16;
typedef signed long    int32;
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef __int64        int64;
typedef unsigned __int64 uint64;
typedef long double         LDBL;
typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ulonglong;
typedef char                BCD4[4];
typedef char                BCD8[8];
typedef char                BCD10[10];
typedef char   * BCD_T;
typedef unsigned short      RECORDSIZE;
typedef unsigned long       RECORDNUMBER;
typedef unsigned char       xmlChar; // Определение из libxml и для libxml (libxml now includes slib.h)
typedef int                 boolint; // @v10.5.2 Введен с целью унифицировать "рукотворные" целочисленные булевы типы в разных включенных библиотеках
typedef int32               TYPEID;

class  SBuffer;
class  SString;
class  SPoint2S;
class  TRect;
class  SPoint2R;
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
class  SVectorBase;
class  SArray;
class  SVector;
class  LongArray;
class  RAssocArray;
class  UintHashTable;
class  SPaintToolBox;
class  ObjStore;
struct SMathResult;
class  SUsbDevice;
class  SDrawImage;
class  TCanvas2;
class  TVRez;
class  WinServiceMngr;
class  StatBase;
class  SoapPacket;
class  SoapPacketStruc;
class  SoapPacketArray;
class  DBTable;
struct SJson;
struct SDataMoveProgressInfo;
class  SImageBuffer;
struct _finddatai64_t;
struct _cairo;
struct cairo_pattern_t;
struct cairo_surface_t;
typedef struct _cairo cairo_t;
typedef struct _cairo_font_face cairo_font_face_t;
typedef struct _cairo_scaled_font cairo_scaled_font_t;
struct Archive; // @v10.4.4 libarchive
//
//
//
class SlConstParam {
public:
	SlConstParam() :
		VectorImageMask(0x00008000U),
		Ssc_CompressionSignature(0xFDB975312468ACE0ULL),
		WaitableObjCheckTimeout(1 * 60 * 1000),
		SUiLayoutSignature(0x28785EB8U),
		ListBoxDefSignature_INVALID(0x00000000),  // Invalid (destroyed) instance
		ListBoxDefSignature_DEFAULT(0x1234ABCD),  // Instance of base class ListBoxDef or subclass hasn't own Sign
		ListBoxDefSignature_STD(0xABCD0001),      // StdListBoxDef
		ListBoxDefSignature_STRASSOC(0xABCD0002), // StrAssocListBoxDef
		ListBoxDefSignature_STDTREE(0xABCD0003),  // StdTreeListBoxDef
		ListBoxDefSignature_STRING(0xABCD0004),   // StringListBoxDef
		ListBoxDefSignature_DBQ(0xABCD0005),      // DBQListBoxDef
		ListBoxDefSignature_STDTREE2(0xABCD0006), // StdTreeListBoxDef2
		MagicHashPrime32(2654435761U)/*0x9e3779b1*/, // Knuth multiplicative hashing.  (Sorting & Searching, p. 510). the closest prime number to (2**32)*golden_ratio, where golden_ratio = (sqrt(5) - 1)/2.
		GoldenRatioInt32(2654435769U)/*0x9e3779b9*/, // The integral part of the Golden Ratio's fractional part 0.61803398875… (sqrt(5)-1)/2, multiplied by 2^32.
		MagicMurmurC1(0xcc9e2d51U), // @v11.4.0 Magic number C1 for 32-bit hashing from Murmur3
		MagicMurmurC2(0x1b873593U)  // @v11.4.0 Magic number C2 for 32-bit hashing from Murmur3
	{
	}
	const uint VectorImageMask;
	const uint64 Ssc_CompressionSignature; // Сигнатура сжатия в буфере SBuffer
	//
	// Descr: Контрольный таймаут ожидания для заданных бесконечных таймаутов (milliseconds).
	//   Если в течении этого таймаута ожидание на завершилось, то предпринимаются
	//   информирующие действия (например, вывод в журнал).
	//
	const long WaitableObjCheckTimeout;
	const uint32 SUiLayoutSignature; // @v11.2.4 Сигнатура экземпляров класса SUiLayout для верификации валидности
	const uint32 ListBoxDefSignature_INVALID;  //0x00000000 // Invalid (destroyed) instance
	const uint32 ListBoxDefSignature_DEFAULT;  //0x1234ABCD // Instance of base class ListBoxDef or subclass hasn't own Sign
	const uint32 ListBoxDefSignature_STD;      //0xABCD0001 // StdListBoxDef
	const uint32 ListBoxDefSignature_STRASSOC; //0xABCD0002 // StrAssocListBoxDef
	const uint32 ListBoxDefSignature_STDTREE;  //0xABCD0003 // StdTreeListBoxDef
	const uint32 ListBoxDefSignature_STRING;   //0xABCD0004 // StringListBoxDef
	const uint32 ListBoxDefSignature_DBQ;      //0xABCD0005 // DBQListBoxDef
	const uint32 ListBoxDefSignature_STDTREE2; //0xABCD0006 // StdTreeListBoxDef2
	const uint32 MagicHashPrime32;             // @v11.3.10 Knuth multiplicative hashing.  (Sorting & Searching, p. 510). the closest prime number to (2**32)*golden_ratio, where golden_ratio = (sqrt(5) - 1)/2.
	const uint32 GoldenRatioInt32;             // @v11.3.12 The integral part of the Golden Ratio's fractional part 0.61803398875… (sqrt(5)-1)/2, multiplied by 2^32.
	//0x9e3779b9 is the integral part of the Golden Ratio's fractional part 0.61803398875… (sqrt(5)-1)/2, multiplied by 2^32.
	const uint32 MagicMurmurC1; // @v11.4.0 0xcc9e2d51 // Magic number C1 for 32-bit hashing from Murmur3
	const uint32 MagicMurmurC2; // @v11.4.0 0x1b873593 // Magic number C2 for 32-bit hashing from Murmur3
};

extern const SlConstParam _SlConst;

template <class T> T * AssignPtr2Ptr(T *& prDest, const T * pSrc) // @v11.4.12
{
	if(pSrc) {
		if(prDest)
			delete prDest;
		prDest = new T;
		*prDest = *pSrc;
	}
	else if(prDest) {
		ZDELETE(prDest);
	}
	return prDest;
}
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

// @v11.4.1 #define CMPSIGN(p1, p2) ((p1)>(p2))?+1:(((p1)<(p2))?-1:0)
template <class T> int CMPSIGN(T p1, T p2) { return ((p1)>(p2))?+1:(((p1)<(p2))?-1:0); } // @v11.4.1 (размер бинарного кода стал меньше; скорость, очевидно, будет выше)
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

#define CMPARRAY2(si, a1, a2) { if(!(si = CMPSIGN(a1[0], a2[0]))) si = CMPSIGN(a1[1], a2[1]); }
#define CMPARRAY3(si, a1, a2) { if(!(si = CMPSIGN(a1[0], a2[0])) && !(si = CMPSIGN(a1[1], a2[1]))) si = CMPSIGN(a1[2], a2[2]); }
#define CMPARRAY4(si, a1, a2) { if(!(si = CMPSIGN(a1[0], a2[0])) && !(si = CMPSIGN(a1[1], a2[1])) && !(si = CMPSIGN(a1[2], a2[2]))) si = CMPSIGN(a1[3], a2[3]); }

#define RET_CMPCASCADE2(s1, s2, m1, m2) { int si = 0; CMPCASCADE2(si, s1, s2, m1, m2); return si; }
#define RET_CMPCASCADE3(s1, s2, m1, m2, m3) { int si = 0; CMPCASCADE3(si, s1, s2, m1, m2, m3); return si; }
#define RET_CMPCASCADE4(s1, s2, m1, m2, m3, m4) { int si = 0; CMPCASCADE4(si, s1, s2, m1, m2, m3, m4); return si; }
#define RET_CMPCASCADE5(s1, s2, m1, m2, m3, m4, m5) { int si = 0; CMPCASCADE5(si, s1, s2, m1, m2, m3, m4, m5); return si; }
#define RET_CMPCASCADE6(s1, s2, m1, m2, m3, m4, m5, m6) { int si = 0; CMPCASCADE6(si, s1, s2, m1, m2, m3, m4, m5, m6); return si; }
#define RET_CMPCASCADE7(s1, s2, m1, m2, m3, m4, m5, m6, m7) { int si = 0; CMPCASCADE7(si, s1, s2, m1, m2, m3, m4, m5, m6, m7); return si; }
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
#define DECL_INVARIANT_C()    int InvariantC(SInvariantParam * pInvP) const
#define IMPL_INVARIANT_C(cls) int cls::InvariantC(SInvariantParam * pInvP) const
//
#define SIZEOFARRAY(array)  (sizeof(array)/sizeof((array)[0]))
#define SIZEOFARRAYi(array) static_cast<int>(sizeof(array)/sizeof((array)[0]))
#define IS_ODD(n)          ((n) & 1)
#define IS_EVEN(n)         (!(IS_ODD(n)))

#define SKILOBYTE(v)       (1024 * (v))
#define SKILOBYTELL(v)     (1024i64 * (v))
#define SMEGABYTE(v)       (1024 * 1024 * (v))
#define SMEGABYTELL(v)     (1024i64 * 1024i64 * (v))
#define SGIGABYTE(v)       (1024i64 * 1024i64 * 1024i64 * (v))
#define	MEGABYTE	       SMEGABYTE(1)
#define	GIGABYTE	       SMEGABYTE(1024)

#define SSECSPERHOUR       3600
#define SSECSPERDAY        (24 * 60 * 60)
#define SSECSPERDAY64      (24LL * 60LL * 60LL)
#define SSECSPERDAYr       (24.0 * 60.0 * 60.0)
//
//
// @v11.2.0 { // @v11.4.0 (some modifications)
#if !defined(RC_INVOKED) && !defined(NO_CLANG_BUILTIN) && __has_builtin(__builtin_rotateleft32) && __has_builtin(__builtin_rotateleft64)
	uint32 FORCEINLINE slrotl32(uint32 x, uint n) { return __builtin_rotateleft32(x, n); }
	uint64 FORCEINLINE slrotl64(uint64 x, uint n) { return __builtin_rotateleft64(x, n); }
#elif defined(_MSC_VER)
	// don't use _rotl with MINGW. It can insert slow call to function.
	// #if(_MSC_VER >= 1200)
	#pragma intrinsic(_rotl)
	#pragma intrinsic(_rotr)
	#pragma intrinsic(_rotl64)
	#pragma intrinsic(_rotr64)
	// #endif
	uint32 FORCEINLINE slrotl32(uint32 x, uint n) { return _rotl(x, n); }
	uint32 FORCEINLINE slrotr32(uint32 x, uint n) { return _rotr((x), (n)); }
	uint64 FORCEINLINE slrotl64(uint64 x, uint n) { return _rotl64((x), (n)); }
	uint64 FORCEINLINE slrotr64(uint64 x, uint n) { return _rotr64((x), (n)); }
#else
	// new compilers can translate these macros to fast commands
	uint32 FORCEINLINE slrotl32(uint32 x, uint n) { return (((x) << (n)) | ((x) >> (32 - (n)))); }
	uint32 FORCEINLINE slrotr32(uint32 x, uint n) { return (((x) >> (n)) | ((x) << (32 - (n)))); }
	uint64 FORCEINLINE slrotl64(uint64 x, uint n) { return (((x) << (n)) | ((x) >> (64 - (n)))); }
	uint64 FORCEINLINE slrotr64(uint64 x, uint n) { return (((x) >> (n)) | ((x) << (64 - (n)))); }
#endif
// } @v11.2.0
//
//
//
#include <asmlib.h>

/* @v11.0.0 extern "C" {
	void * cdecl xeos_memchr(const void * s, int c, size_t n);
	void * cdecl xeos_memchr32(const void * s, int c, size_t n);
	void * cdecl xeos_memchr32_sse2(const void * s, int c, size_t n);
	uint   cdecl xeos_strlen(const char * s);
	uint   cdecl xeos_strlen32(const char * s);
	uint   cdecl xeos_strlen32_sse2(const char * s);
}*/

#if USE_ASMLIB
	#define memcpy   A_memcpy
	#define memmove  A_memmove
	#define memset   A_memset
	#define memcmp   A_memcmp
	//#define strcat   A_strcat
	//#define strlen   A_strlen
	//#define strcmp   A_strcmp
	//#define stricmp  A_stricmp
	//#define strstr   A_strstr
	//
#endif

void * operator new(size_t sz);
void operator delete(void * ptr);
//
// Descr: Перечисление, обеспечивающее типизированное применение "лекого" варианта конструктора
//   объекта. Применяется в специальных случаях, когда необходимо максимально быстро создать
//   упрощенный экземпляр "тяжелого" объекта.
//
enum SCtrLite {
	SConstructorDef = 0,
	SConstructorLite = 1
};
//
// Descr: Специализированный объект, используемый в качестве аргумента конструкторов с целью автоматической
//   автогенерации экземпляра. Пример: S_GUID
//
class SCtrGenerate {
public:
	SCtrGenerate()
	{
	}
};

const SCtrGenerate SCtrGenerate_;
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
	slangMeta    =   1, // Meta-language
	slangLA   	 =   2, // LATIN
	slangEN   	 =   3, // ENGLISH
	slangRU   	 =   4, // RUSSIAN
	slangDE   	 =   5, // GERMAN
	slangAA   	 =   6, // AFAR
	slangAB   	 =   7, // ABKHAZIAN
	slangACE  	 =   8, // ACHINESE
	slangACH  	 =   9, // ACOLI
	slangADA  	 =  10, // ADANGME
	slangADY  	 =  11, // ADYGHE
	slangAE   	 =  12, // AVESTAN
	slangAF   	 =  13, // AFRIKAANS
	slangAFA  	 =  14, // AFRO-ASIATIC LANGUAGE
	slangAFH  	 =  15, // AFRIHILI
	slangAGQ  	 =  16, // AGHEM
	slangAIN  	 =  17, // AINU
	slangAK   	 =  18, // AKAN
	slangAKK  	 =  19, // AKKADIAN
	slangALE  	 =  20, // ALEUT
	slangALG  	 =  21, // ALGONQUIAN LANGUAGE
	slangALT  	 =  22, // SOUTHERN ALTAI
	slangAM   	 =  23, // AMHARIC
	slangAN   	 =  24, // ARAGONESE
	slangANG  	 =  25, // OLD ENGLISH
	slangANP  	 =  26, // ANGIKA
	slangAPA  	 =  27, // APACHE LANGUAGE
	slangAR   	 =  28, // ARABIC
	slangARC  	 =  29, // ARAMAIC
	slangARN  	 =  30, // ARAUCANIAN
	slangARP  	 =  31, // ARAPAHO
	slangART  	 =  32, // ARTIFICIAL LANGUAGE
	slangARW  	 =  33, // ARAWAK
	slangAS   	 =  34, // ASSAMESE
	slangASA  	 =  35, // ASU
	slangAST  	 =  36, // ASTURIAN
	slangATH  	 =  37, // ATHAPASCAN LANGUAGE
	slangAUS  	 =  38, // AUSTRALIAN LANGUAGE
	slangAV   	 =  39, // AVARIC
	slangAWA  	 =  40, // AWADHI
	slangAY   	 =  41, // AYMARA
	slangAZ   	 =  42, // AZERBAIJANI
	slangBA   	 =  43, // BASHKIR
	slangBAD  	 =  44, // BANDA
	slangBAI  	 =  45, // BAMILEKE LANGUAGE
	slangBAL  	 =  46, // BALUCHI
	slangBAN  	 =  47, // BALINESE
	slangBAS  	 =  48, // BASAA
	slangBAT  	 =  49, // BALTIC LANGUAGE
	slangBE   	 =  50, // BELARUSIAN
	slangBEJ  	 =  51, // BEJA
	slangBEM  	 =  52, // BEMBA
	slangBER  	 =  53, // BERBER
	slangBEZ  	 =  54, // BENA
	slangBG   	 =  55, // BULGARIAN
	slangBH   	 =  56, // BIHARI
	slangBHO  	 =  57, // BHOJPURI
	slangBI   	 =  58, // BISLAMA
	slangBIK  	 =  59, // BIKOL
	slangBIN  	 =  60, // BINI
	slangBLA  	 =  61, // SIKSIKA
	slangBM   	 =  62, // BAMBARA
	slangBN   	 =  63, // BENGALI
	slangBNT  	 =  64, // BANTU
	slangBO   	 =  65, // TIBETAN
	slangBR   	 =  66, // BRETON
	slangBRA  	 =  67, // BRAJ
	slangBRX  	 =  68, // BODO
	slangBS   	 =  69, // BOSNIAN
	slangBTK  	 =  70, // BATAK
	slangBUA  	 =  71, // BURIAT
	slangBUG  	 =  72, // BUGINESE
	slangBYN  	 =  73, // BLIN
	slangCA   	 =  74, // CATALAN
	slangCAD  	 =  75, // CADDO
	slangCAI  	 =  76, // CENTRAL AMERICAN INDIAN LANGUAGE
	slangCAR  	 =  77, // CARIB
	slangCAU  	 =  78, // CAUCASIAN LANGUAGE
	slangCAY  	 =  79, // CAYUGA
	slangCCH  	 =  80, // ATSAM
	slangCE   	 =  81, // CHECHEN
	slangCEB  	 =  82, // CEBUANO
	slangCEL  	 =  83, // CELTIC LANGUAGE
	slangCGG  	 =  84, // CHIGA
	slangCH   	 =  85, // CHAMORRO
	slangCHB  	 =  86, // CHIBCHA
	slangCHG  	 =  87, // CHAGATAI
	slangCHK  	 =  88, // CHUUKESE
	slangCHM  	 =  89, // MARI
	slangCHN  	 =  90, // CHINOOK JARGON
	slangCHO  	 =  91, // CHOCTAW
	slangCHP  	 =  92, // CHIPEWYAN
	slangCHR  	 =  93, // CHEROKEE
	slangCHY  	 =  94, // CHEYENNE
	slangCMC  	 =  95, // CHAMIC LANGUAGE
	slangCO   	 =  96, // CORSICAN
	slangCOP  	 =  97, // COPTIC
	slangCPE  	 =  98, // ENGLISH-BASED CREOLE OR PIDGIN
	slangCPF  	 =  99, // FRENCH-BASED CREOLE OR PIDGIN
	slangCPP  	 = 100, // PORTUGUESE-BASED CREOLE OR PIDGIN
	slangCR   	 = 101, // CREE
	slangCRH  	 = 102, // CRIMEAN TURKISH
	slangCRP  	 = 103, // CREOLE OR PIDGIN
	slangCS   	 = 104, // CZECH
	slangCSB  	 = 105, // KASHUBIAN
	slangCU   	 = 106, // CHURCH SLAVIC
	slangCUS  	 = 107, // CUSHITIC LANGUAGE
	slangCV   	 = 108, // CHUVASH
	slangCY   	 = 109, // WELSH
	slangDA   	 = 110, // DANISH
	slangDAK  	 = 111, // DAKOTA
	slangDAR  	 = 112, // DARGWA
	slangDAV  	 = 113, // TAITA
	slangDAY  	 = 114, // DAYAK
	slangDE_AT   = 115, // AUSTRIAN GERMAN
	slangDE_CH 	 = 116, // SWISS HIGH GERMAN
	slangDEL   	 = 117, // DELAWARE
	slangDEN   	 = 118, // SLAVE
	slangDGR   	 = 119, // DOGRIB
	slangDIN   	 = 120, // DINKA
	slangDJE   	 = 121, // ZARMA
	slangDOI   	 = 122, // DOGRI
	slangDRA   	 = 123, // DRAVIDIAN LANGUAGE
	slangDSB   	 = 124, // LOWER SORBIAN
	slangDUA   	 = 125, // DUALA
	slangDUM   	 = 126, // MIDDLE DUTCH
	slangDV    	 = 127, // DIVEHI
	slangDYO   	 = 128, // JOLA-FONYI
	slangDYU   	 = 129, // DYULA
	slangDZ    	 = 130, // DZONGKHA
	slangEBU   	 = 131, // EMBU
	slangEE    	 = 132, // EWE
	slangEFI   	 = 133, // EFIK
	slangEGY   	 = 134, // ANCIENT EGYPTIAN
	slangEKA   	 = 135, // EKAJUK
	slangEL    	 = 136, // GREEK
	slangELX   	 = 137, // ELAMITE
	slangEN_AU 	 = 138, // AUSTRALIAN ENGLISH
	slangEN_CA 	 = 139, // CANADIAN ENGLISH
	slangEN_GB 	 = 140, // BRITISH ENGLISH
	slangEN_US 	 = 141, // U.S. ENGLISH
	slangENM   	 = 142, // MIDDLE ENGLISH
	slangEO    	 = 143, // ESPERANTO
	slangES      = 144, // SPANISH
	slangES_419  = 145, // LATIN AMERICAN SPANISH
	slangES_ES   = 146, // IBERIAN SPANISH
	slangET      = 147, // ESTONIAN
	slangEU  	 = 148, // BASQUE
	slangEWO 	 = 149, // EWONDO
	slangFA  	 = 150, // PERSIAN
	slangFAN 	 = 151, // FANG
	slangFAT 	 = 152, // FANTI
	slangFF  	 = 153, // FULAH
	slangFI  	 = 154, // FINNISH
	slangFIL 	 = 155, // FILIPINO
	slangFIU 	 = 156, // FINNO-UGRIAN LANGUAGE
	slangFJ  	 = 157, // FIJIAN
	slangFO  	 = 158, // FAROESE
	slangFON 	 = 159, // FON
	slangFR      = 160, // FRENCH
	slangFR_CA 	 = 161, // CANADIAN FRENCH
	slangFR_CH 	 = 162, // SWISS FRENCH
	slangFRM   	 = 163, // MIDDLE FRENCH
	slangFRO   	 = 164, // OLD FRENCH
	slangFRR   	 = 165, // NORTHERN FRISIAN
	slangFRS   	 = 166, // EASTERN FRISIAN
	slangFUR   	 = 167, // FRIULIAN
	slangFY    	 = 168, // WESTERN FRISIAN
	slangGA    	 = 169, // IRISH
	slangGAA     = 170, // GA
	slangGAY 	 = 171, // GAYO
	slangGBA 	 = 172, // GBAYA
	slangGD  	 = 173, // SCOTTISH GAELIC
	slangGEM 	 = 174, // GERMANIC LANGUAGE
	slangGEZ 	 = 175, // GEEZ
	slangGIL 	 = 176, // GILBERTESE
	slangGL  	 = 177, // GALICIAN
	slangGMH 	 = 178, // MIDDLE HIGH GERMAN
	slangGN  	 = 179, // GUARANI
	slangGOH 	 = 180, // OLD HIGH GERMAN
	slangGON 	 = 181, // GONDI
	slangGOR 	 = 182, // GORONTALO
	slangGOT 	 = 183, // GOTHIC
	slangGRB 	 = 184, // GREBO
	slangGRC 	 = 185, // ANCIENT GREEK
	slangGSW 	 = 186, // SWISS GERMAN
	slangGU  	 = 187, // GUJARATI
	slangGUZ 	 = 188, // GUSII
	slangGV  	 = 189, // MANX
	slangGWI 	 = 190, // GWICH?IN
	slangHA  	 = 191, // HAUSA
	slangHAI 	 = 192, // HAIDA
	slangHAW 	 = 193, // HAWAIIAN
	slangHE  	 = 194, // HEBREW
	slangHI  	 = 195, // HINDI
	slangHIL 	 = 196, // HILIGAYNON
	slangHIM 	 = 197, // HIMACHALI
	slangHIT 	 = 198, // HITTITE
	slangHMN 	 = 199, // HMONG
	slangHO  	 = 200, // HIRI MOTU
	slangHR  	 = 201, // CROATIAN
	slangHSB 	 = 202, // UPPER SORBIAN
	slangHT  	 = 203, // HAITIAN
	slangHU  	 = 204, // HUNGARIAN
	slangHUP 	 = 205, // HUPA
	slangHY  	 = 206, // ARMENIAN
	slangHZ  	 = 207, // HERERO
	slangIA  	 = 208, // INTERLINGUA
	slangIBA 	 = 209, // IBAN
	slangID  	 = 210, // INDONESIAN
	slangIE  	 = 211, // INTERLINGUE
	slangIG  	 = 212, // IGBO
	slangII  	 = 213, // SICHUAN YI
	slangIJO 	 = 214, // IJO
	slangIK  	 = 215, // INUPIAQ
	slangILO 	 = 216, // ILOKO
	slangINC 	 = 217, // INDIC LANGUAGE
	slangINE 	 = 218, // INDO-EUROPEAN LANGUAGE
	slangINH 	 = 219, // INGUSH
	slangIO  	 = 220, // IDO
	slangIRA 	 = 221, // IRANIAN LANGUAGE
	slangIRO 	 = 222, // IROQUOIAN LANGUAGE
	slangIS  	 = 223, // ICELANDIC
	slangIT  	 = 224, // ITALIAN
	slangIU  	 = 225, // INUKTITUT
	slangJA  	 = 226, // JAPANESE
	slangJBO 	 = 227, // LOJBAN
	slangJMC 	 = 228, // MACHAME
	slangJPR 	 = 229, // JUDEO-PERSIAN
	slangJRB 	 = 230, // JUDEO-ARABIC
	slangJV  	 = 231, // JAVANESE
	slangKA  	 = 232, // GEORGIAN
	slangKAA 	 = 233, // KARA-KALPAK
	slangKAB 	 = 234, // KABYLE
	slangKAC 	 = 235, // KACHIN
	slangKAJ 	 = 236, // JJU
	slangKAM 	 = 237, // KAMBA
	slangKAR 	 = 238, // KAREN
	slangKAW 	 = 239, // KAWI
	slangKBD 	 = 240, // KABARDIAN
	slangKCG 	 = 241, // TYAP
	slangKDE 	 = 242, // MAKONDE
	slangKEA 	 = 243, // KABUVERDIANU
	slangKFO 	 = 244, // KORO
	slangKG  	 = 245, // KONGO
	slangKHA 	 = 246, // KHASI
	slangKHI 	 = 247, // KHOISAN LANGUAGE
	slangKHO 	 = 248, // KHOTANESE
	slangKHQ 	 = 249, // KOYRA CHIINI
	slangKI  	 = 250, // KIKUYU
	slangKJ  	 = 251, // KUANYAMA
	slangKK  	 = 252, // KAZAKH
	slangKL  	 = 253, // KALAALLISUT
	slangKLN 	 = 254, // KALENJIN
	slangKM  	 = 255, // KHMER
	slangKMB 	 = 256, // KIMBUNDU
	slangKN  	 = 257, // KANNADA
	slangKO  	 = 258, // KOREAN
	slangKOK 	 = 259, // KONKANI
	slangKOS 	 = 260, // KOSRAEAN
	slangKPE 	 = 261, // KPELLE
	slangKR  	 = 262, // KANURI
	slangKRC 	 = 263, // KARACHAY-BALKAR
	slangKRL 	 = 264, // KARELIAN
	slangKRO 	 = 265, // KRU
	slangKRU 	 = 266, // KURUKH
	slangKS  	 = 267, // KASHMIRI
	slangKSB 	 = 268, // SHAMBALA
	slangKSF 	 = 269, // BAFIA
	slangKSH 	 = 270, // COLOGNIAN
	slangKU  	 = 271, // KURDISH
	slangKUM 	 = 272, // KUMYK
	slangKUT 	 = 273, // KUTENAI
	slangKV  	 = 274, // KOMI
	slangKW  	 = 275, // CORNISH
	slangKY  	 = 276, // KIRGHIZ
	slangLAD 	 = 277, // LADINO
	slangLAG 	 = 278, // LANGI
	slangLAH 	 = 279, // LAHNDA
	slangLAM 	 = 280, // LAMBA
	slangLB  	 = 281, // LUXEMBOURGISH
	slangLEZ 	 = 282, // LEZGHIAN
	slangLG  	 = 283, // GANDA
	slangLI  	 = 284, // LIMBURGISH
	slangLN  	 = 285, // LINGALA
	slangLO  	 = 286, // LAO
	slangLOL 	 = 287, // MONGO
	slangLOZ 	 = 288, // LOZI
	slangLT  	 = 289, // LITHUANIAN
	slangLU  	 = 290, // LUBA-KATANGA
	slangLUA 	 = 291, // LUBA-LULUA
	slangLUI 	 = 292, // LUISENO
	slangLUN 	 = 293, // LUNDA
	slangLUO 	 = 294, // LUO
	slangLUS 	 = 295, // LUSHAI
	slangLUY 	 = 296, // LUYIA
	slangLV  	 = 297, // LATVIAN
	slangMAD 	 = 298, // MADURESE
	slangMAG 	 = 299, // MAGAHI
	slangMAI 	 = 300, // MAITHILI
	slangMAK 	 = 301, // MAKASAR
	slangMAN 	 = 302, // MANDINGO
	slangMAP 	 = 303, // AUSTRONESIAN LANGUAGE
	slangMAS 	 = 304, // MASAI
	slangMDF 	 = 305, // MOKSHA
	slangMDR 	 = 306, // MANDAR
	slangMEN 	 = 307, // MENDE
	slangMER 	 = 308, // MERU
	slangMFE 	 = 309, // MORISYEN
	slangMG  	 = 310, // MALAGASY
	slangMGA 	 = 311, // MIDDLE IRISH
	slangMGH 	 = 312, // MAKHUWA-MEETTO
	slangMH  	 = 313, // MARSHALLESE
	slangMI  	 = 314, // MAORI
	slangMIC 	 = 315, // MICMAC
	slangMIN 	 = 316, // MINANGKABAU
	slangMIS 	 = 317, // MISCELLANEOUS LANGUAGE
	slangMK  	 = 318, // MACEDONIAN
	slangMKH 	 = 319, // MON-KHMER LANGUAGE
	slangML  	 = 320, // MALAYALAM
	slangMN  	 = 321, // MONGOLIAN
	slangMNC 	 = 322, // MANCHU
	slangMNI 	 = 323, // MANIPURI
	slangMNO 	 = 324, // MANOBO LANGUAGE
	slangMO  	 = 325, // MOLDAVIAN
	slangMOH 	 = 326, // MOHAWK
	slangMOS 	 = 327, // MOSSI
	slangMR  	 = 328, // MARATHI
	slangMS  	 = 329, // MALAY
	slangMT  	 = 330, // MALTESE
	slangMUA 	 = 331, // MUNDANG
	slangMUL 	 = 332, // MULTIPLE LANGUAGES
	slangMUN 	 = 333, // MUNDA LANGUAGE
	slangMUS 	 = 334, // CREEK
	slangMWL 	 = 335, // MIRANDESE
	slangMWR 	 = 336, // MARWARI
	slangMY  	 = 337, // BURMESE
	slangMYN 	 = 338, // MAYAN LANGUAGE
	slangMYV 	 = 339, // ERZYA
	slangNA  	 = 340, // NAURU
	slangNAH 	 = 341, // NAHUATL
	slangNAI 	 = 342, // NORTH AMERICAN INDIAN LANGUAGE
	slangNAP 	 = 343, // NEAPOLITAN
	slangNAQ 	 = 344, // NAMA
	slangNB  	 = 345, // NORWEGIAN BOKMAL
	slangND  	 = 346, // NORTH NDEBELE
	slangNDS 	 = 347, // LOW GERMAN
	slangNE  	 = 348, // NEPALI
	slangNEW 	 = 349, // NEWARI
	slangNG  	 = 350, // NDONGA
	slangNIA 	 = 351, // NIAS
	slangNIC 	 = 352, // NIGER-KORDOFANIAN LANGUAGE
	slangNIU 	 = 353, // NIUEAN
	slangNL  	 = 354, // DUTCH
	slangNL_BE   = 355, // FLEMISH
	slangNMG   	 = 356, // KWASIO
	slangNN    	 = 357, // NORWEGIAN NYNORSK
	slangNO    	 = 358, // NORWEGIAN
	slangNOG   	 = 359, // NOGAI
	slangNON   	 = 360, // OLD NORSE
	slangNQO   	 = 361, // N’KO
	slangNR    	 = 362, // SOUTH NDEBELE
	slangNSO   	 = 363, // NORTHERN SOTHO
	slangNUB   	 = 364, // NUBIAN LANGUAGE
	slangNUS   	 = 365, // NUER
	slangNV    	 = 366, // NAVAJO
	slangNWC   	 = 367, // CLASSICAL NEWARI
	slangNY    	 = 368, // NYANJA
	slangNYM   	 = 369, // NYAMWEZI
	slangNYN   	 = 370, // NYANKOLE
	slangNYO   	 = 371, // NYORO
	slangNZI   	 = 372, // NZIMA
	slangOC    	 = 373, // OCCITAN
	slangOJ    	 = 374, // OJIBWA
	slangOM    	 = 375, // OROMO
	slangOR    	 = 376, // ORIYA
	slangOS    	 = 377, // OSSETIC
	slangOSA   	 = 378, // OSAGE
	slangOTA   	 = 379, // OTTOMAN TURKISH
	slangOTO   	 = 380, // OTOMIAN LANGUAGE
	slangPA    	 = 381, // PUNJABI
	slangPAA   	 = 382, // PAPUAN LANGUAGE
	slangPAG   	 = 383, // PANGASINAN
	slangPAL   	 = 384, // PAHLAVI
	slangPAM   	 = 385, // PAMPANGA
	slangPAP   	 = 386, // PAPIAMENTO
	slangPAU   	 = 387, // PALAUAN
	slangPEO   	 = 388, // OLD PERSIAN
	slangPHI   	 = 389, // PHILIPPINE LANGUAGE
	slangPHN   	 = 390, // PHOENICIAN
	slangPI    	 = 391, // PALI
	slangPL    	 = 392, // POLISH
	slangPON   	 = 393, // POHNPEIAN
	slangPRA   	 = 394, // PRAKRIT LANGUAGE
	slangPRO   	 = 395, // OLD PROVENCAL
	slangPS    	 = 396, // PASHTO
	slangPT    	 = 397, // PORTUGUESE
	slangPT_BR 	 = 398, // BRAZILIAN PORTUGUESE
	slangPT_PT 	 = 399, // IBERIAN PORTUGUESE
	slangQU    	 = 400, // QUECHUA
	slangRAJ   	 = 401, // RAJASTHANI
	slangRAP   	 = 402, // RAPANUI
	slangRAR   	 = 403, // RAROTONGAN
	slangRM    	 = 404, // ROMANSH
	slangRN    	 = 405, // RUNDI
	slangRO    	 = 406, // ROMANIAN
	slangROA   	 = 407, // ROMANCE LANGUAGE
	slangROF   	 = 408, // ROMBO
	slangROM   	 = 409, // ROMANY
	slangROOT  	 = 410, // ROOT
	slangRUP   	 = 411, // AROMANIAN
	slangRW    	 = 412, // KINYARWANDA
	slangRWK   	 = 413, // RWA
	slangSA    	 = 414, // SANSKRIT
	slangSAD   	 = 415, // SANDAWE
	slangSAH   	 = 416, // SAKHA
	slangSAI   	 = 417, // SOUTH AMERICAN INDIAN LANGUAGE
	slangSAL   	 = 418, // SALISHAN LANGUAGE
	slangSAM   	 = 419, // SAMARITAN ARAMAIC
	slangSAQ   	 = 420, // SAMBURU
	slangSAS   	 = 421, // SASAK
	slangSAT   	 = 422, // SANTALI
	slangSBP   	 = 423, // SANGU
	slangSC    	 = 424, // SARDINIAN
	slangSCN   	 = 425, // SICILIAN
	slangSCO   	 = 426, // SCOTS
	slangSD    	 = 427, // SINDHI
	slangSE    	 = 428, // NORTHERN SAMI
	slangSEE   	 = 429, // SENECA
	slangSEH   	 = 430, // SENA
	slangSEL   	 = 431, // SELKUP
	slangSEM   	 = 432, // SEMITIC LANGUAGE
	slangSES   	 = 433, // KOYRABORO SENNI
	slangSG    	 = 434, // SANGO
	slangSGA   	 = 435, // OLD IRISH
	slangSGN   	 = 436, // SIGN LANGUAGE
	slangSH    	 = 437, // SERBO-CROATIAN
	slangSHI   	 = 438, // TACHELHIT
	slangSHN   	 = 439, // SHAN
	slangSI    	 = 440, // SINHALA
	slangSID   	 = 441, // SIDAMO
	slangSIO   	 = 442, // SIOUAN LANGUAGE
	slangSIT   	 = 443, // SINO-TIBETAN LANGUAGE
	slangSK    	 = 444, // SLOVAK
	slangSL    	 = 445, // SLOVENIAN
	slangSLA   	 = 446, // SLAVIC LANGUAGE
	slangSM    	 = 447, // SAMOAN
	slangSMA   	 = 448, // SOUTHERN SAMI
	slangSMI   	 = 449, // SAMI LANGUAGE
	slangSMJ   	 = 450, // LULE SAMI
	slangSMN   	 = 451, // INARI SAMI
	slangSMS   	 = 452, // SKOLT SAMI
	slangSN    	 = 453, // SHONA
	slangSNK   	 = 454, // SONINKE
	slangSO    	 = 455, // SOMALI
	slangSOG   	 = 456, // SOGDIEN
	slangSON   	 = 457, // SONGHAI
	slangSQ    	 = 458, // ALBANIAN
	slangSR    	 = 459, // SERBIAN
	slangSRN   	 = 460, // SRANAN TONGO
	slangSRR   	 = 461, // SERER
	slangSS    	 = 462, // SWATI
	slangSSA   	 = 463, // NILO-SAHARAN LANGUAGE
	slangSSY   	 = 464, // SAHO
	slangST    	 = 465, // SOUTHERN SOTHO
	slangSU    	 = 466, // SUNDANESE
	slangSUK   	 = 467, // SUKUMA
	slangSUS   	 = 468, // SUSU
	slangSUX   	 = 469, // SUMERIAN
	slangSV    	 = 470, // SWEDISH
	slangSW    	 = 471, // SWAHILI
	slangSWB   	 = 472, // COMORIAN
	slangSWC   	 = 473, // CONGO SWAHILI
	slangSYC   	 = 474, // CLASSICAL SYRIAC
	slangSYR   	 = 475, // SYRIAC
	slangTA    	 = 476, // TAMIL
	slangTAI   	 = 477, // TAI LANGUAGE
	slangTE    	 = 478, // TELUGU
	slangTEM   	 = 479, // TIMNE
	slangTEO   	 = 480, // TESO
	slangTER   	 = 481, // TERENO
	slangTET   	 = 482, // TETUM
	slangTG    	 = 483, // TAJIK
	slangTH    	 = 484, // THAI
	slangTI    	 = 485, // TIGRINYA
	slangTIG   	 = 486, // TIGRE
	slangTIV   	 = 487, // TIV
	slangTK    	 = 488, // TURKMEN
	slangTKL   	 = 489, // TOKELAU
	slangTL    	 = 490, // TAGALOG
	slangTLH   	 = 491, // KLINGON
	slangTLI   	 = 492, // TLINGIT
	slangTMH   	 = 493, // TAMASHEK
	slangTN    	 = 494, // TSWANA
	slangTO    	 = 495, // TONGAN
	slangTOG   	 = 496, // NYASA TONGA
	slangTPI   	 = 497, // TOK PISIN
	slangTR    	 = 498, // TURKISH
	slangTRV   	 = 499, // TAROKO
	slangTS    	 = 500, // TSONGA
	slangTSI   	 = 501, // TSIMSHIAN
	slangTT    	 = 502, // TATAR
	slangTUM   	 = 503, // TUMBUKA
	slangTUP   	 = 504, // TUPI LANGUAGE
	slangTUT   	 = 505, // ALTAIC LANGUAGE
	slangTVL   	 = 506, // TUVALU
	slangTW    	 = 507, // TWI
	slangTWQ   	 = 508, // TASAWAQ
	slangTY    	 = 509, // TAHITIAN
	slangTYV   	 = 510, // TUVINIAN
	slangTZM   	 = 511, // CENTRAL MOROCCO TAMAZIGHT
	slangUDM   	 = 512, // UDMURT
	slangUG    	 = 513, // UIGHUR
	slangUGA   	 = 514, // UGARITIC
	slangUK    	 = 515, // UKRAINIAN
	slangUMB   	 = 516, // UMBUNDU
	slangUND   	 = 517, // UNKNOWN LANGUAGE
	slangUR    	 = 518, // URDU
	slangUZ    	 = 519, // UZBEK
	slangVAI   	 = 520, // VAI
	slangVE    	 = 521, // VENDA
	slangVI    	 = 522, // VIETNAMESE
	slangVO    	 = 523, // VOLAPUK
	slangVOT   	 = 524, // VOTIC
	slangVUN   	 = 525, // VUNJO
	slangWA    	 = 526, // WALLOON
	slangWAE   	 = 527, // WALSER
	slangWAK   	 = 528, // WAKASHAN LANGUAGE
	slangWAL   	 = 529, // WALAMO
	slangWAR   	 = 530, // WARAY
	slangWAS   	 = 531, // WASHO
	slangWEN   	 = 532, // SORBIAN LANGUAGE
	slangWO    	 = 533, // WOLOF
	slangXAL   	 = 534, // KALMYK
	slangXH    	 = 535, // XHOSA
	slangXOG   	 = 536, // SOGA
	slangYAO   	 = 537, // YAO
	slangYAP   	 = 538, // YAPESE
	slangYAV   	 = 539, // YANGBEN
	slangYI    	 = 540, // YIDDISH
	slangYO    	 = 541, // YORUBA
	slangYPK   	 = 542, // YUPIK LANGUAGE
	slangYUE   	 = 543, // CANTONESE
	slangZA    	 = 544, // ZHUANG
	slangZAP   	 = 545, // ZAPOTEC
	slangZBL   	 = 546, // BLISSYMBOLS
	slangZEN   	 = 547, // ZENAGA
	slangZH    	 = 548, // CHINESE
	slangZH_HANS = 549, // SIMPLIFIED CHINESE
	slangZH_HANT = 550, // TRADITIONAL CHINESE
	slangZND     = 551, // ZANDE
	slangZU      = 552, // ZULU
	slangZUN     = 553, // ZUNI
	slangZXX     = 554, // NO LINGUISTIC CONTENT
	slangZZA     = 555  // ZAZA
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
uint32 FASTCALL GetLinguadent(int ident);
void   FASTCALL GetLinguaList(LongArray & rList);
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
	cpUndef   =   -1,
	cpANSI    =    0,  // == CP_ACP
	cpOEM     =    1,  // == CP_OEMCP
	cpUTF7    = 65000, // == CP_UTF7
	cpUTF8    = 65001, // == CP_UTF8
	cpUTF16   =  1200, // utf-16      Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications
	cpUTF16BE =  1201, // unicodeFFFE Unicode UTF-16, big endian byte order; available only to managed applications
	cpUTF32   = 12000, // utf-32      Unicode UTF-32, little endian byte order; available only to managed applications
	cpUTF32BE = 12001, // utf-32BE    Unicode UTF-32, big endian byte order; available only to managed applications

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
	cp950     =  950,  // 950 Chinese Windows (BIG5)
	cp1250    = 1250,  // 1250 Eastern European Windows
	cp1251    = 1251,  // 1251 Russian Windows
	cp1252    = 1252,  // 1252 Windows ANSI
	cp1253    = 1253,  // 1253 Greek Windows
	cp1254    = 1254,  // 1254 Turkish Windows
	cp1255    = 1255,  // 1255 Hebrew Windows
	cp1256    = 1256,  // 1256 Arabic Windows
	cp1257    = 1257,  // 1257 Windows Baltic
	cp1258    = 1258,  // 1258 ANSI/OEM Vietnamese Windows

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
	cpKOI8R = 20866, // synonim
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

	cpISO_8859_01 = cp28591, // synonim
	cpISO_8859_02 = cp28592, // synonim
	cpISO_8859_03 = cp28593, // synonim
	cpISO_8859_04 = cp28594, // synonim
	cpISO_8859_05 = cp28595, // synonim
	cpISO_8859_06 = cp28596, // synonim
	cpISO_8859_07 = cp28597, // synonim
	cpISO_8859_08 = cp28598, // synonim
	cpISO_8859_09 = cp28599, // synonim
	cpISO_8859_10 = 885910,
	cpISO_8859_11 = 885911,
	cpISO_8859_12 = 885912,
	cpISO_8859_13 = cp28603, // synonim
	cpISO_8859_14 = 885914,
	cpISO_8859_15 = cp28605, // synonim
	cpISO_8859_16 = 885916,

	cp29001 = 29001, // x-Europa	Europa 3
	cp38598 = 38598, // iso-8859-8-i	ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
	cp50220 = 50220, // iso-2022-jp	ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)
	cp50221 = 50221, // csISO2022JP	ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)
	cp50222 = 50222, // iso-2022-jp	ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)
	cp50225 = 50225, // iso-2022-kr	ISO 2022 Korean
	cp50227 = 50227, // x-cp50227	ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
	cp50229 = 50229, // ISO 2022 Traditional Chinese
	cp50930 = 50930, // EBCDIC Japanese (Katakana) Extended
	cp50931 = 50931, // EBCDIC US-Canada and Japanese
	cp50933 = 50933, // EBCDIC Korean Extended and Korean
	cp50935 = 50935, // EBCDIC Simplified Chinese Extended and Simplified Chinese
	cp50936 = 50936, // EBCDIC Simplified Chinese
	cp50937 = 50937, // EBCDIC US-Canada and Traditional Chinese
	cp50939 = 50939, // EBCDIC Japanese (Latin) Extended and Japanese
	cp51932 = 51932, // euc-jp	EUC Japanese
	cp51936 = 51936, // EUC-CN	EUC Simplified Chinese; Chinese Simplified (EUC)
	cp51949 = 51949, // euc-kr	EUC Korean
	cp51950 = 51950, // EUC Traditional Chinese
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
	cp57011 = 57011, // x-iscii-pa	ISCII Punjabi

	cpVISCII   = 60001, // Vietnamese Standard Code for Information Interchange (@sobolev: number 60001 is my own)
	cpShiftJIS = 60002, // shift_jis	ANSI/OEM Japanese; Japanese (Shift-JIS) (@sobolev: number 60002 is my own)
	cpTIS620   = 60620, // Thai Industrial Standard 620-2533 (@sobolev: number 60620 is my own)
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
	bool   FASTCALL operator == (SCodepage cp) const;
	bool   FASTCALL operator != (SCodepage cp) const;
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
	static const uint8  Utf8EncLen_RFC3629[256];
	static const uint8  Utf8EncLen_[256];
	static const uint32 OffsetsFromUTF8[6];
	static const uint8  FirstByteMark[7];
	static const uint32 HalfBase;
	static const uint32 HalfMask;
	static const int    HalfShift; // used for shifting by 10 bits
};
//
// Макро-константы конвертации UTF-символов
//
#define UNI_SUR_HIGH_START    static_cast<uint32>(0x0000D800U)     // SURROGATE LEAD FIRST
#define UNI_SUR_HIGH_END      static_cast<uint32>(0x0000DBFFU)     // SURROGATE LEAD LAST
#define UNI_SUR_LOW_START     static_cast<uint32>(0x0000DC00U)     // SURROGATE TRAIL FIRST
#define UNI_SUR_LOW_END       static_cast<uint32>(0x0000DFFFU)     // SURROGATE TRAIL LAST
#define UNI_REPLACEMENT_CHAR  static_cast<uint32>(0x0000FFFDU)
#define UNI_MAX_BMP           static_cast<uint32>(0x0000FFFFU)
#define UNI_MAX_UTF16         static_cast<uint32>(0x0010FFFFU)
#define UNI_MAX_UTF32         static_cast<uint32>(0x7FFFFFFFU)
#define UNI_MAX_LEGAL_UTF32   static_cast<uint32>(0x0010FFFFU)
#define UNI_SUPPL_PLANE_START static_cast<uint32>(0x00010000U)    // SUPPLEMENTAL PLANE FIRST

class SUnicode {
public:
	static bool  FASTCALL IsLegalUtf8(const uint8 * pSource, size_t length);
	//
	// Descr: Транслирует unicode32 в один или 2 широких символа wchar_t.
	// Returns:
	//   Количество широких символов, необходимое для того, чтобы вместить u32
	//
	static uint  FASTCALL Utf32ToUtf16(uint32 u32, wchar_t * pU16Buf);
	//
	// Descr: Рассчитывает длину строки в кодировке utf8, которая получится после
	//   преобразования unicode-строки pUcBuf длиной tlen или пока pUcBuf[0] != 0
	//
	static uint  FASTCALL Utf8Length(const wchar_t * pUcBuf, uint tlen);
	static uint  FASTCALL Utf32ToUtf8(uint32 u32, char * pUtf8Buf);
};

enum SEOLFormat {
	eolAny   = -1, // @v11.3.6 Любая комбинация ('\x0D'; '\x0A'; '\x0D\x0A'; '\x0A\x0D') трактуется как конец строки
	eolUndef = 0,  // Неопределен
	eolWindows,    // \x0D\x0A
	eolUnix,       // \x0A
	eolMac,        // \x0D
	eolSpcICalendar // @v11.0.3 Специальный тип строки: если после \xD\xA встречается пробел, то это трактуется как продолжение текущей строки
};
//
//
//
class SHandle {
public:
	SHandle() : P(0)
	{
	}
	explicit SHandle(void * ptr) : P(ptr)
	{
	}
	SHandle & operator = (void * ptr)
	{
		P = ptr;
		return *this;
	}
	bool   operator !() const { return (P == 0); }
	operator void * () const { return P; }
private:
	void * P;
};

template <typename T> class TSHandle {
public:
	TSHandle() : P(0)
	{
	}
	explicit TSHandle(T * ptr) : P(ptr)
	{
	}
	TSHandle & operator = (T * ptr)
	{
		P = ptr;
		return *this;
	}
	bool   operator !() const { return (P == 0); }
	operator T * () const { return P; }
private:
	T * P;
};
//
//
//
class S_GUID_Base { // @persistent @noctr @size=16
public:
	S_GUID_Base & FASTCALL Init(REFIID);
	operator GUID & ();
	enum {
		fmtIDL = 0,
		fmtC = 1,
		fmtPlain,    // "плоский" формат. то есть, без ограничителей и разделителей в 16-ричном формате
		// @construction fmtSUID27,   // @v10.9.0 Специальный внутренний формат (base32), умещающийся в 28 байт (с нулем в конце)
		fmtLower = 0x8000 // Флаг, предписывающий использовать буквы [a-f] в строчном регистре
	};
	bool   operator !() const { return IsZero(); }
	bool   FASTCALL operator == (const S_GUID_Base &) const;
	bool   FASTCALL operator != (const S_GUID_Base &) const;
	bool   IsZero() const;
	S_GUID_Base & Z();
	SString & ToStr(long fmt, SString &) const;
	//
	// Descr: Преобразует строку pStr в GUID.
	// Note: Если в преобразовать строку в GUID не удалось, то this обнуляется (SetZero())
	// Returns:
	//   !0 - преобразование осуществлено успешно
	//   0  - ошибка
	//
	int    FASTCALL FromStr(const char * pStr);
	int    Generate();

	uint32 Data[4];
};

class S_GUID : public S_GUID_Base {
public:
	S_GUID();
	explicit S_GUID(SCtrGenerate g);
	S_GUID(const S_GUID_Base & rS);
	S_GUID(const S_GUID & rS);
	explicit S_GUID(const char * pStr);
	S_GUID & FASTCALL operator = (const S_GUID_Base & rS);
};
//
// Descr: Константы, идентифицирующие значения, ассоциированные со временем и датой
// @v11.2.10 @construction
//
enum {
	timeconstMONTHSHIFT      = 20,
	timeconstDAYOFWEEKSHIFT  = 40,

	timeconstYear        =  1,
	timeconstMonth       =  2,
	timeconstDayOfMonth  =  3,
	timeconstDayOfWeek   =  4,
	timeconstHour        =  5,
	timeconstMinute      =  6,
	timeconstSecond      =  7,
	timeconstMillisecond =  8,
	timeconstZoneOffset  =  9,
	timeconstDstOffset   = 10,

	timeconstMonthJan    = 1+timeconstMONTHSHIFT,
	timeconstMonthFeb    = 2+timeconstMONTHSHIFT,
	timeconstMonthMar    = 3+timeconstMONTHSHIFT,
	timeconstMonthApr    = 4+timeconstMONTHSHIFT,
	timeconstMonthMay    = 5+timeconstMONTHSHIFT,
	timeconstMonthJun    = 6+timeconstMONTHSHIFT,
	timeconstMonthJul    = 7+timeconstMONTHSHIFT,
	timeconstMonthAug    = 8+timeconstMONTHSHIFT,
	timeconstMonthSep    = 9+timeconstMONTHSHIFT,
	timeconstMonthOct    = 10+timeconstMONTHSHIFT,
	timeconstMonthNov    = 11+timeconstMONTHSHIFT,
	timeconstMonthDec    = 12+timeconstMONTHSHIFT,

	timeconstDowMon      = 1+timeconstDAYOFWEEKSHIFT,
	timeconstDowTue      = 2+timeconstDAYOFWEEKSHIFT,
	timeconstDowWed      = 3+timeconstDAYOFWEEKSHIFT,
	timeconstDowThu      = 4+timeconstDAYOFWEEKSHIFT,
	timeconstDowFri      = 5+timeconstDAYOFWEEKSHIFT,
	timeconstDowSat      = 6+timeconstDAYOFWEEKSHIFT,
	timeconstDowSun      = 7+timeconstDAYOFWEEKSHIFT,
};
//
// Descr: Представление даты в формате MS-DOS.
// Note: obsolete (практически не используется)
//
struct SDosDate {
	int16  da_year; // Year - 1980
	int8   da_day;  // Day of the month
	int8   da_mon;  // Month (1 = Jan)
};
//
// Descr: Дата/время в формате Microsoft OLE (DATE)
//   The DATE type is implemented using an 8-byte floating-point number.
//   Days are represented by whole number increments starting with 30 December 1899, midnight as time zero.
//   Hour values are expressed as the absolute value of the fractional part of the number.
//   The following table illustrates this.
//
struct OleDate { // @noctr
	operator double() const { return v; }
	double v;
};
//
// Descr: Основное представление даты.
//   Два старших байта - год, средний байт - месяц, самый младший - день.
//   |yyyy|mm|dd|
//
struct LDATE { // @noctr
	enum {
		cZero = 0,         // Дата имеет нулевое значение
		cNormal,           // Дата имеет правильное актуальное значение
		cSpecial,          // Дата представлена в виде, трактуемом относительно заданной даты (see LDATE::getactual())
		cInvalid           // Дата имеет недопустимое значение
	};
	operator unsigned long() const { return v; }
	// @v10.3.4 (replaced with GetOleDate()) operator OleDate() const;
	OleDate GetOleDate() const;
	time_t GetTimeT() const;
	LDATE  Z() { v = 0; return *this; }
	LDATE  operator = (OleDate);
	LDATE  operator = (double); // OleDate
	int    day() const { return static_cast<int>(v & 0x00ff); }
	int    month() const { return static_cast<int>((v & 0xff00) >> 8); }
	int    year() const { return static_cast<int>(v >> 16); }
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
	//
	// Descr: Специализированная (и опасная) реализация кодировки даты,
	//   предполагающая, что все входные значения (d, m, y) заданы правильными
	//   величинами, соответственно, дня, месяца и года.
	//   Для использования только в критичных к скорости исполнения блоках!
	//
	void   FORCEINLINE EncodeRegular(int d, int m, int y)
	{
		v = static_cast<ulong>(MakeLong((m << 8) | d, y));
	}
	int    decode(int * pD, int * pM, int * pY) const;
	//
	// Descr: Возвращает эффективное значение даты. Если дата представлена
	//   относительным значение, то разворачивает его относительно даты rel.
	//   Если rel == ZERODATE, то разворачивает относительно getcurdate_().
	//   Если дата не содержит относительных факторов, то возвращает ее как есть.
	//
	LDATE  FASTCALL getactual(LDATE rel) const;
	LDATE  getactualcmp(LDATE rel, LDATE cmp) const;
	//
	// Descr: Возвращает !0 если день, месяц или год содержат шаблонизированный ANY-признак.
	//
	int    hasanycomponent() const;

	ulong  v;
private:
	LDATE  Helper_GetActual(LDATE rel, LDATE cmp) const;
	LDATE  operator = (int); // @v10.9.12 an empty inaccessable method (prevention of the mismatched assignment)
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
	bool   IsDate(LDATE) const;
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
	operator unsigned long() const { return v; }
	operator OleDate() const;
	LTIME  Z() { v = 0; return *this; }
	LTIME  operator = (OleDate);
	LTIME  operator = (double); // OleDate
	LTIME  encode(int h, int m, int s, int ms);
	int    hour() const { return static_cast<int>((reinterpret_cast<const uint8 *>(&v))[3]); }
	int    minut() const { return static_cast<int>((reinterpret_cast<const uint8 *>(&v))[2]); }
	int    sec() const { return static_cast<int>((reinterpret_cast<const uint8 *>(&v))[1]); }
	//
	// Descr: Возвращает количество сотых долей секунды в значении this.
	//
	int    hs() const { return static_cast<int>((reinterpret_cast<const uint8 *>(&v))[0]); }
	long   totalsec() const { return (sec() + minut() * 60 + hour() * 3600); }
	//
	// Descr: Устанавливает время в соответствии с количеством секунд, заданных параметром s.
	// Returns:
	//   Количество целых суток, содержащихся в s
	//
	long   FASTCALL settotalsec(long s);
	//
	// Descr: добавляет к значению времени n сотых долей секунды.
	//
	LTIME & FASTCALL addhs(long n);
	ulong  v;
private:
	LTIME  operator = (int); // @v10.9.12 an empty inaccessable method (prevention of the mismatched assignment)
};
//
// @construction {
// Descr: Представление, обеспечивающее хранение времени с точностью до миллисекунд
//   и с фактором временного пояса.
//
class STIME {
public:
	DECL_INVARIANT_C();
	operator unsigned long() const { return v; }
	operator OleDate() const;
	STIME  operator = (OleDate);
	STIME  operator = (double); // OleDate
	STIME  encode(int h, int m, int s, int ms, int tz);
	int    hour() const;
	int    minut() const;
	int    sec() const;
	int    ms() const;
	int    zone() const;
	long   totalsec() const;
	//
	// Descr: Устанавливает время в соответствии с количеством секунд, заданных параметром s.
	// Returns:
	//   Количество целых суток, содержащихся в s
	//
	long   FASTCALL settotalsec(long s);
	//
	// Descr: добавляет к значению времени ms миллисекунд.
	//
	STIME & FASTCALL addms(long ms);
private:
	ulong  v;
};
// } @construction
//
// Descr: Porting of the posix gettimeofday() and struct timezone
//
struct timezone {
	int    tz_minuteswest; // minutes W of Greenwich
	int    tz_dsttime;     // type of dst correction
};

int gettimeofday(struct timeval * pTv, struct timezone * pTz);
//
// Descr: Представление рабочего дня //
//   Календарные даты представлены индексами количества календарных дней после WorkDate::InitDate.
//   Дни недели представлены индексами меньшими нуля и большими -100 [-1..-7].       {-день недели}
//   Даты, не зависящие от года представлены индексами дня меньшими или равными -100 {-(день/месяц/1996)-100}
//
//   Специализированное представление той же сущности, для которой применяется CALDATE.
//   Нюанс использования состоит в очень сжатой форме хранения данного класса (2 байта).
//
class WorkDate { // @noctr @size=2
public:
	static int    FASTCALL ShrinkDate(LDATE dt);
	static LDATE  FASTCALL ExpandDate(int16 sdt);
	int    FASTCALL SetDate(LDATE dt);
	int    FASTCALL SetDayOfWeek(int dayOfWeek);
	int    SetDayOfYear(int day, int mon);
	LDATE  IsDate() const;
	int    IsDayOfWeek() const;
	LDATE  IsDayOfYear() const;
	bool   FASTCALL IsEq(LDATE) const;
	bool   FASTCALL IsEq(WorkDate) const;
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
	//LDATETIME();
	SYSTEMTIME & FASTCALL Get(SYSTEMTIME &) const;
	LDATETIME FASTCALL operator = (const SYSTEMTIME &);
	LDATETIME FASTCALL operator = (const FILETIME &);
	//
	// Descr: Возвращает !0 если d == ZERODATE && t == ZEROTIME
	//
	bool   operator !() const;
	bool   FASTCALL operator == (const LDATETIME & s) const;
	bool   FASTCALL operator != (const LDATETIME & s) const;
	operator LDATE () const { return d; }
	operator OleDate() const;
	LDATETIME FASTCALL operator = (OleDate);
	LDATETIME & Set(LDATE _d, LTIME _t);
	LDATETIME & Z();
	LDATETIME & SetMax(); // max-moment: (LDATE)d = MAXLONG, (LTIME)t = MAXLONG
	LDATETIME & SetFar(); // far-moment: (LDATE)d = MAXLONG, (LTIME)t = 0
	LDATETIME & FASTCALL SetTimeT(time_t t);
	time_t GetTimeT() const;
	bool   IsFar() const;
	//
	// Decr: Устанавливает дату-время, разбирая строку pText в соответствии с форматами datf и timf.
	//
	int    Set(const char * pText, long datf, long timef);
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
extern const LTIME MAXDAYTIME; // 23:59:59.99
extern const LTIME MAXDAYTIMESEC; // 23:59:59.00
extern const LDATETIME ZERODATETIME;
extern const LDATETIME MAXDATETIME;
extern const S_GUID    ZEROGUID; // @v10.9.3
//
// Descr: Отрезок времени. Используется для планирования, расчетов занятости и т.д.
//   Обеспечивает базовые примитивы для работы с временными отрезками.
//   -- Нулевое время верхней границы эквивалентно концу суток.
//   -- Верхняя граница Finish.IsFar() эквивалентна бесконечности
//   @attention Не должен включать виртуальных фукнций.
//
struct STimeChunk {
	STimeChunk();
	STimeChunk(const LDATETIME & rStart, const LDATETIME & rFinish);
	STimeChunk & Z();
	void   Init(const LDATETIME & rStart, const LDATETIME & rFinish);
	void   Init(const LDATETIME &, long cont);
	//
	// Descr: Определяет, совпадают ли точки начала и окончания //
	//   отрезков *this и rTest.
	// Returns:
	//   !0 - точки начала и окончания this и rTest совпадают
	//   0  - точки начала и окончания this и rTest не совпадают
	//
	bool   FASTCALL operator == (const STimeChunk & rTest) const;
	bool   FASTCALL operator != (const STimeChunk & rTest) const;
	//
	// Descr: Сравнивает *this с элементом rTest.
	//   Сравнение осуществляется каскадно: сначала Start, затем Finish.
	// Returns:
	//   >0  - *this > rTest
	//   <0  - *this < rTest
	//   ==0 - *this == rTest
	//
	int    FASTCALL cmp(const STimeChunk & rTest) const;
	bool   FASTCALL Has(const LDATETIME & rTm) const;
	//
	// Descr: Находит пересечение отрезков *this и rTest.
	// ARG(rTest    IN): Отрезок, на пересечение с которым проверяется *this.
	// ARG(pResult OUT): @#{vptr0} Если указатель не нулевой, то по нему присваивается //
	//   результат пересечения *this и rTest.
	// Returns:
	//   !0 - отрезки *this и rTest пересекаются //
	//   0  - отрезки *this и rTest не пересекаются //
	//
	bool   FASTCALL Intersect(const STimeChunk & rTest, STimeChunk * pResult) const;
	//
	// Descr: Если отрезки this и rOther пересекаются, то присваивает объединение этих отрезков 
	//   по указателю pResult и возвращает true. В противном случае возвращает false и не меняет 
	//   значение по указателю pResult.
	//
	bool   GetUnionIfIntersected(const STimeChunk & rOther, STimeChunk * pResult) const;
	//
	// Descr: Определяет, содержится ли период this в периоде rDur.
	//   Если this полностью содержится в rDur, то функция возвращает !0.
	//
	bool   FASTCALL ContainsIn(const STimeChunk & rDur) const;
	//
	// Descr: Возвращает продолжительность элемента в секундах.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	long   GetDuration() const;
	//
	// Descr: Возвращает продолжительность элемента в сутках.
	//   При вычислении в расчет берутся только даты Start.d и Finish.d.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	long   GetDurationDays() const;
	//
	// Descr: Возвращает продолжительность элемента в миллисекундах.
	//   Если продолжительность не определена (бесконечна), то возвращает -1.
	//
	int64  GetDurationMs() const;
	enum {
		fmtOmitSec  = 0x0001  // Не выводить секунды
	};
	SString & ToStr(SString & rBuf, long fmt = 0) const;

	LDATETIME Start;
	LDATETIME Finish;
};
//
// Descr: Класс, определяющий календарные периодичности.
//
class DateRepeating { // @persistent @noctr @novtbl @size=8
public:
	static int FASTCALL IsValidPrd(int prd);

	int    FASTCALL operator == (const DateRepeating & rS) const;
	int    FASTCALL operator != (const DateRepeating & rS) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    Init(int prd, int kind = 1, LDATE = ZERODATE);
	int    Next_(LDATE startDate, LDATE * pNextDate) const; // @obsolete use DateRepIterator
	SString & Format(int fmt, SString & rBuf) const;
	long   DtlToLong();
	int    LongToDtl(long);
	int    GetMonthlyPeriod(int * pMonthCount, int * pMonthNo) const;
	int    SetMonthly(int monthCount, int monthNo, int dayOfMonth);
	int    SetMonthly(int monthCount, int monthNo, int weekNo, int weekDay);
	int    DayOfWeekNo(LDATE dt, int weekNo, int dayOfWeek, LDATE * pResult) const;

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
	int    Init(int prd, int kind = 1, LDATE dt = ZERODATE, LTIME tm = ZEROTIME);
	int    Next_(LDATETIME startDtm, LDATETIME * pNextDtm) const; // @obsolete use DateTimeRepIterator
	SString & Format(int fmt, SString & rBuf) const;
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
	explicit SCycleTimer(uint32 msDelay);
	void   FASTCALL Restart(uint32 msDelay);
	int    FASTCALL Check(LDATETIME * pLast);
private:
	uint32 Delay;
	LDATETIME  Last;
};
//
// Descr: Универсальное представление времени, обеспечивающее хранение любых временных
//   отметок с (почти) любой точностью.
//
class SUniTime {
public:
	enum {
	    indfScale       = 0x80, // Если этот бит нулевой, то время представлено как FILETIME (100ns since 1601-01-01 UTC)
	    indfUncertainty = 0xC0, // Если бит установлен, то значение приблизительное. В этом случае предпоследний
		// байт может содержать дополнительные признаки представления (например, как диапазон или +/-)
	    indInvalid      = 0xff, // Используется как внутренний код возврата, означающий ошибку
	    indDefault      = 0x00, // 0.1mks since 1601-01-01 (UTC).
	    indMSec         = 0x80, // milliseconds since 1601-01-01 (UTC).
	    indSec          = 0x81, // seconds since 1601-01-01 (UTC).
	    indMin          = 0x82, // minuts  since 1601-01-01 (UTC).
	    indHr           = 0x83, // hours   since 1601-01-01 (UTC).
	    indMSecTz       = 0x84, // since 1601-01-01. // timezone - lowest 8 bit
	    indCSecTz       = 0x85, // since 1601-01-01. // timezone - lowest 8 bit
	    indSecTz        = 0x86, // since 1601-01-01. // timezone - lowest 8 bit
	    indMinTz        = 0x87, // since 1601-01-01. // timezone - lowest 8 bit
	    indHrTz         = 0x88, // since 1601-01-01. // timezone - lowest 8 bit
	    indDay          = 0x89, // since 0001-01-01
	    indMon          = 0x8A, // since 0001-01-01
	    indQuart        = 0x8B, // since 0001-01-01
	    indSmYr         = 0x8C, // since 0001-01-01
	    indYr           = 0x8D, // since 0001-01-01
	    indDYr          = 0x8E, // since 0001-01-01
	    indSmCent       = 0x8F, // since 0001-01-01
	    indCent         = 0x90, // since 0001-01-01
	    indMillennium   = 0x91, // since 0001-01-01
	    indDayBC        = 0x92, // before 0001-01-01
	    indMonBC        = 0x93, // before 0001-01-01
	    indYrBC         = 0x94, // before 0001-01-01
	    indDYrBC        = 0x95, // before 0001-01-01
	    indCentBC       = 0x96, // before 0001-01-01
	    indMillenniumBC = 0x97, // before 0001-01-01
	    indMlnYrAgo     = 0x98,
	    indBlnYrAgo     = 0x99
	};
	SUniTime();
	explicit SUniTime(LDATE d);
	explicit SUniTime(const LDATETIME & rD);
	bool   operator !() const;
	SUniTime & Z();
	//
	// Descr: Возвращает сигнатуру, идентифицирующую вариант представления времени.
	//
	uint   GetSignature() const;
	int64  ToInt64() const;
	int    FromInt64(int64 v);
	//
	// Descr: Устанавливает время с точность до даты (н.э.)
	//
	int    FASTCALL Set(LDATE d);
	int    FASTCALL Set(const LDATETIME & rD);
	//
	// Descr: Устанавливает время согласно rD с точностью, заданной сигнатурой signature.
	// ARG(rD        IN): Дата/время, которые следует установить
	// ARG(signature IN): Требуемая точность установки.
	//   Допускаются только следующие значения параметра:
	//   indSec, indMin, indHr, indDay, indMon, indQuart, indSmYr, indYr, indDYr, indSmCent, indCent
	//
	int    FASTCALL Set(const LDATETIME & rD, uint signature);
	//
	// Descr: Устанавливает время согласно rD с явным указанием часового пояса и с точностью, заданной сигнатурой signature.
	// ARG(rD        IN): Дата/время, которые следует установить
	// ARG(signature IN): Требуемая точность установки.
	// ARG(timezone  IN): Часовой пояс (-12..+14).
	//   Допускаются только следующие значения параметра:
	//   indMSecTz, indCSecTz, indSecTz, indMinTz, indHrTz
	//
	int    FASTCALL Set(const LDATETIME & rD, uint signature, int timezone);
	int    FASTCALL Set(time_t t);
	int    FASTCALL Set(time_t t, int timezone);
	int    FASTCALL Set(const FILETIME & rD);
	//
	// Descr: Устанавливает время с точность до года year (н.э.)
	//
	int    FASTCALL SetYear(int year);
	//
	// Descr: Устанавливает время с точность до месяца month года year (н.э.)
	//
	int    FASTCALL SetMonth(int year, int month);
	int    FASTCALL Get(LDATE & rD) const;
	int    FASTCALL Get(LDATETIME & rD) const;
	int    FASTCALL Get(time_t & rD) const;
	int    FASTCALL Get(FILETIME & rD) const;
	enum {
		cqUndef     =  0,
		cqSure      =  1,
		cqUncertain = -1
	};
	int    FASTCALL Compare(const SUniTime & rS, int * pQualification) const;
	//
	// Descr: Результаты сравнения значений
	//
	enum {
		cmprIncompat      = -1, // Значения несовместимы и несравнимы
		cmprSureTrue      = 1,  // Значения точно равны
		cmprSureFalse     = 0,  // Значения точно не равны
		cmprUncertainTrue = 2   // Значения равны с учетом того, что их точности различны ("5 июня 2018" vs "июнь 2018" например)
	};
	int    FASTCALL IsEq(const SUniTime & rS) const; // @construction
private:
	int    Implement_Set(uint8 signature, const void *);
	uint8  Implement_Get(void *) const;
	//
	// Самый старший байт D[7] - индикатор, определяющий разрешение представления и,
	// возможно, часовой пояс (для indMksSince1900).
	//
    uint8  D[8];
};

#if !defined(RC_INVOKED) /* { */

struct STypEx { // @persistent @size=16
	DECL_INVARIANT_C();
	void   Init();
	int    IsPure() const;
	bool   FASTCALL IsEq(const STypEx &) const;
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
	explicit DataType(uint32 sz) : S(sz) {}
	DataType() : S(0) {}
	virtual uint32 size() const; // @v10.2.1 size_t-->uint32
	virtual int    comp(const void * i1, const void * i2) const;
	virtual char * tostr(const void *, long, char * b) const;
	//
	// Descr: Функция должна преобразовать бинарное представление данные pData
	//   в строку rBuf в соответствии с форматом преобразования format.
	//   Функция ToStr обязана предварительно очистить буфер rBuf вызовом rBuf.Z()
	// Note: Эта функция замещает устаревшую tostr.
	//   На начальном этапе default-реализация функции вызывает tostr()
	//
	virtual SString & ToStr(const void * pData, long format, SString & rBuf) const;
	virtual int    fromstr(void *, long, const char *) const;
	virtual int    base() const;
	virtual void   tobase(const void * data, void * baseData) const;
	virtual int    baseto(void * data, const void * baseData) const;
	virtual void   FASTCALL destroy(void *) const;
	// DataType::minval and DataType::maxval sets to parameter
	// min and max values correspondly.
	// Returns !0 if function is supported, otherwise 0.
	virtual void   minval(void *) const;
	virtual void   maxval(void *) const;
	virtual int    Serialize(int dir, void * pData, uint8 * pInd, SBuffer & rBuf, SSerializeContext * pCtx);
	int    isnumber() const { return (base() == BTS_INT || base() == BTS_REAL); }

	uint32 S; // Size or other specific data
};

inline TYPEID GetSTypeID(char)   { return MKSTYPE(S_INT, 1); }
inline TYPEID GetSTypeID(int16) { return MKSTYPE(S_INT, 2); }
inline TYPEID GetSTypeID(int)    { return MKSTYPE(S_INT, 4); }
inline TYPEID GetSTypeID(long)   { return MKSTYPE(S_INT, 4); }
inline TYPEID GetSTypeID(float) { return MKSTYPE(S_FLOAT, 4); }
inline TYPEID GetSTypeID(double) { return MKSTYPE(S_FLOAT, 8); }
inline TYPEID GetSTypeID(LDBL)   { return MKSTYPE(S_FLOAT, 10); }
inline TYPEID GetSTypeID(char *,  int s) { return MKSTYPE(S_ZSTRING, s); }
inline TYPEID GetSTypeID(LDATE) { return MKSTYPE(S_DATE, 4); }
inline TYPEID GetSTypeID(LTIME) { return MKSTYPE(S_TIME, 4); }
inline TYPEID GetSTypeID(LDATETIME) { return MKSTYPE(S_DATETIME, 8); }
DataType & FASTCALL stype(TYPEID t, DataType *);
TYPEID     FASTCALL bt2st(int baseType);
int    FASTCALL btnumber(int bt);
uint32 FASTCALL stsize(TYPEID t); // @v10.2.1 size_t-->uint32
int    FASTCALL stcomp(TYPEID t, const void * i1, const void * i2);
char * FASTCALL sttostr(TYPEID t, const void * d, long f, char * b);
int    FASTCALL stfromstr(TYPEID t, void * d, long f, const char * b);
int    FASTCALL stbase(TYPEID t);
void   FASTCALL sttobase(TYPEID t, const void * data, void * baseData);
int    FASTCALL stbaseto(TYPEID t, void * data, const void * baseData);
void   FASTCALL stdestroy(TYPEID t, void * d);
int    FASTCALL stisnumber(TYPEID t);
void   FASTCALL stminval(TYPEID t, void * d);
void   FASTCALL stmaxval(TYPEID t, void * d);
int    FASTCALL stcast(TYPEID st, TYPEID dt, const void * sd, void * dd, long f);
//
// Descr: Register all BUILT-INS SLIB TYPES (BIST)
//
void   RegisterBIST();
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
SString & FASTCALL GetBaseTypeString(int baseTyp, long flags, SString & rBuf);
TYPEID FASTCALL IsSTypeName(SStrScan & rScan);
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
#define NMBF_DECCOMMA  0x0100  // Десятичный разделитель - запятая ',', иначе - точка '.'
#define NMBF_EXPLFLOAT 0x0200  // Если float-число не имеет знаков после точки, то оставлять в формате 999.0 (актуально только при флаге NMBF_NOTRAILZ)
#define NMBF_OMITEPS   0x0400  // Если за очередной десятичной цифрой следует OMITEPSDITITS или больше '9' либо '0' то
	// число форматируется с округлением до ближайшей соседней десятичной цифры с заменой последующих на нули.
	// Например:
	// 369,9000000000001  --> 369,9000000000000
	// 4043,1599999999994 --> 4043,1600000000000

#define OMITEPSDIGITS  5
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
// @persistent
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
#define DATF_SQL           12  // DATE 'YYYY-MM-DD'
#define DATF_INTERNET      13  // Wed, 27 Feb 2008
#define DATF_ISO8601       14  // yyyy-mm-dd
#define DATF_CENTURY   0x0010  // Century flag
#define DATF_NOZERO    0x0020  // Не отображать нулевое значение
#define DATF_NODIV     0x0040  // день, месяц и год следуют друг за другом без разделителей.
#define DATF_GERMANCENT    (DATF_GERMAN|DATF_CENTURY) // @v11.3.12
#define DATF_ISO8601CENT   (DATF_ISO8601|DATF_CENTURY) // @v11.3.12
	// Порядок следования определяется младшими битами (DATF_AMERICAN..DATF_YMD). Если установлен
	// флаг DATF_CENTURY, то год должен быть полным (2011), в противном случае - только последние две
	// цифры года (11).
//
// Time format flags
// @persistent
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
#define TIMF_NODIV     0x0040  //
#define TIMF_DOTDIV    0x0080  // Разделители часов, минут и секунд - точки. Например: 23.20.59
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
#define SFALIGNMASK  (static_cast<unsigned long>(ALIGN_LEFT|ALIGN_RIGHT|ALIGN_CENTER))
#define SFCOMMONMASK (static_cast<unsigned long>(SFALIGNMASK|COMF_FILLOVF))
#define MKSFMT(l,f)           (((static_cast<long>(l))<<16)|(static_cast<long>(f)))
#define MKSFMTD(l,p,f)        (((static_cast<long>(p))<<28)|((static_cast<long>(l))<<16)|(static_cast<long>(f)))
#define SFMTFLAG(fmt)         (static_cast<unsigned>((fmt)&SFFLAGMASK))
#define SFMTLEN(fmt)          (static_cast<uint16>(((fmt)&SFLENMASK)>>16))
#define SFMTPRC(fmt)          (((fmt)&SFPRCMASK)>>28)
#define SFMTALIGN(fmt)        ((fmt)&SFALIGNMASK)
#define SETSFMTLEN(fmt,l)     (fmt)=(((static_cast<long>(l))<<16)|(static_cast<long>((fmt)&~SFLENMASK)))
#define SETSFMTPRC(fmt,p)     (fmt)=(((static_cast<long>(p))<<28)|(static_cast<long>((fmt)&~SFPRCMASK)))
#define SETSFMTFLAG(fmt,f)    (fmt)=((static_cast<long>(f))|(static_cast<long>((fmt)&~SFFLAGMASK)))

#define SFMT_MONEY            MKSFMTD(0,2,0)
#define SFMT_QTTY             MKSFMTD(0,3,0)

#ifndef RC_INVOKED
//
// Descr: Блок форматов вывода различных типов данных в строку.
//   Используется при форматировании бинарных данных.
//
struct SFormatParam {
	SFormatParam();
	enum {
		fFloatSize = 0x0001, // Выводимая строка может иметь произвольную длину (длина поля, содержащаяся в форматах игнорируется).
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
	IntRange & Z();
	//
	// Descr: Возвращает !0 если (this->low == 0 && this->upp == 0)
	//
	bool   IsZero() const;
	//
	// Descr: Проверяет значение test на принадлежность закрытому диапазону this
	//   (то есть, равенство граничному значению означает принадлежность всему диапазону).
	//   Если low == 0, то нижняя граница не тестируется.
	//   Если upp == 0, то верхняя граница не тестируется.
	// Returns:
	//   !0 - значение test принадлежит закрытому диапазону low..upp
	//   0  - значение test не принадлежит диапазону
	//
	bool   FASTCALL CheckVal(int test) const;
	bool   FASTCALL operator == (const IntRange & rS) const { return (low == rS.low && upp == rS.upp); }
	bool   FASTCALL operator != (const IntRange & rS) const { return (low != rS.low || upp != rS.upp); }
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
	int    FASTCALL Intersect(const IntRange & rD, IntRange * pResult) const;
	int    FASTCALL Intersect(const IntRange & rD) const;
	SString & Format(long fmt, SString &) const;
	int    Clamp(int v) const;

	int32  low;
	int32  upp;
};
//
//
//
struct SSzChunk { // @flat @noctr @novtbl
	SSzChunk & Z();
	SSzChunk & Set(int begin, int len);
	SSzChunk & SetRange(int begin, int end);
	int    operator !() const;
	int    operator == (const SSzChunk & rS) const;
	int    GetEnd() const;
	bool   IsEmpty() const;

	int    Begin;
	int    Len;
};
//
//
//
struct RealRange { // @persistent @flat @noctr @novtbl @size=16
	/*
	RealRange() : low(0.0), upp(0.0)
	{
	}
	RealRange(double _lo, double _up) : low(_lo), upp(_up)
	{
	}
	*/
	bool   IsZero() const;
	//
	// Descr: Возвращает true если хоть один из компонентов экземпляра имеет значение NAN
	//
	bool   IsNan() const;
	//
	// Descr: Возвращает true если low == upp
	//
	bool   IsDegraded() const;
	//
	// Descr: Возвращает true если и low и upp строго больше нуля.
	//
	bool   IsPositive() const;
	//
	// Descr: Возвращает true если и low и upp строго меньше нуля.
	//
	bool   IsNegative() const;
	//
	// Descr: Возвращает true если low <= upp
	//
	bool   IsNormalized() const;
	//
	// Descr: Возвращает true если low < upp
	//
	bool   IsStrictlyNormalized() const;
	bool   FASTCALL IsEq(const RealRange & rS) const;
	bool   FASTCALL LessThan(double v) const;
	bool   FASTCALL GreaterThan(double v) const;
	double GetDistance() const;
	//
	// Descr: Возвращает наименьшее значение из (low, upp)
	//
	double GetActualLow() const;
	//
	// Descr: Возвращает наибольшее значение из (low, upp)
	//
	double GetActualUpp() const;
	RealRange & Z();
	//RealRange & FASTCALL operator = (const RealRange & rS);
	RealRange & FASTCALL operator = (const IntRange & rS);
	bool   FASTCALL operator != (const RealRange & rS) const;
	//
	// Descr: Проверяет значение v на принадлежность закрытому диапазону [low..upp]
	//
	bool   Check(double v) const;
	//
	// Descr: Проверяет значение v на принадлежность закрытому диапазону [low..upp]
	//   если low <= upp, в противном случае - на принадлежность диапазону [upp..low].
	//
	bool   CheckX(double v) const;
	//
	// Descr: Проверяет значение val на принадлежность закрытому диапазону [low..upp]
	//   с условием, что, если граница (low или upp) имеет значение 0.0, то она не проверяется.
	//   Например,
	//     -1 удовлетворяет диапазону [0..5.2]
	//     10 удовлетворяет диапазону [-3.1..0]
	//
	bool   CheckVal(double val) const;
	//
	// Descr: Проверяет значение val на принадлежность закрытому диапазону [low..upp]
	//   с условием, что, если граница (low или upp) имеет значение 0.0, то она не проверяется.
	//   Если разница между val и границей диапазона меньше eps, то функция считает условие
	//   выполненным.
	//
	bool   CheckValEps(double val, double eps) const;
	double Clip(double v) const;
	RealRange & SetVal(double);
	RealRange & Set(double, double);
	RealRange & SetupMinMax(double v);
	//
	// Descr: Устанавливает диапазон [lo..lo+delta]
	//
	RealRange & SetDelta(double lo, double delta);
	RealRange & Scale(double sc);
	RealRange & Normalize();
	//
	// Descr: Расширяет диапазон на величину delta в обе стороны.
	//  То есть, после исполнения low = low-delta; upp = upp+delta
	//  Note: Не проверяет валидность и упорядоченность low и upp
	// Returns:
	//  *this
	//
	RealRange & Extend(double delta);
	//
	// Descr: вызывает round(low, prec) и round(upp, prec)
	//
	RealRange & FASTCALL Round(int prec);
	//
	// Descr: Устанавливает диапазон [middle-delta..middle+delta]
	//
	RealRange & SetMiddle(double middle, double delta);
	double GetMiddle() const;
	int    FASTCALL Intersect(const RealRange & rTest, RealRange * pResult) const;

	double low;
	double upp;
};

struct DateRange { // @persistent @size=8
	DECL_INVARIANT_C();
	bool   FASTCALL operator == (const DateRange & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const DateRange & rS) const { return !IsEq(rS); }
	bool   FASTCALL IsEq(const DateRange & rS) const;
	int    FASTCALL Cmp(const DateRange & rS) const;
	//
	// Descr: Возвращает длину интервала в днях.
	//   Если интервал бесконечный с обеих сторон, то возвращает (MAXLONG-1),
	//   Если интервал открыт слева, но закрыт справа, то возвращает количество дней от начала эры до
	//   правой границы.
	//   Если интервал открыт справа, но закрыт слева, то возвращает (MAXLONG-2)
	//   Для интевалов, нижняя граница которых превышает верхнюю возвращает отрицательное значение.
	//
	long   GetLength() const;
	bool   IsZero() const;
	int    FASTCALL CheckDate(LDATE) const;
	int    FASTCALL Intersect(const DateRange *);
	int    IsIntersect(DateRange r) const;
	DateRange & Set(LDATE, LDATE);
	//
	// Descr: Если pSrc != 0, то присваивает экземпляру *this значение *pSrc,
	//   в противном случае вызывает SetZero()
	//
	DateRange & FASTCALL Set(const DateRange * pSrc);
	DateRange & Z();
	void   FASTCALL SetDate(LDATE);
	int    SetPeriod(LDATE date, int prd);
	//
	// Descr: Устанавливает предопределенный период, заданный параметром predefPrdId относительно даты rel.
	// ARG(predefPrdId IN): Одно из зарезервированных значений PREDEFPRD_XXX (кроме PREDEFPRD_NONE).
	// ARG(rel IN): дата, относительно которой вычисляется период. Если rel == 0, то применяется текущая системная дата.
	// Returns:
	//   >0 - период успешно установлен
	//   0  - ошибка. Ошибка может быть связана с недопустимыми значениями параметров predefPrdId или rel.
	//     В случае ошибки период this остается неизменным.
	//
	int    SetPredefined(int predefPrdId, LDATE rel);
	//
	// Descr: Если период в точности равен кварталу, то по указателю pQuartN
	//   присваивает номер квартала [1..4] а по указателю pYear - значение года.
	//   Если период не совпадает с кварталом, то по указателю pQuartN присваивает 0.
	// Returns:
	//   >0 - период this в точности совпадает с кварталом
	//    0 - период this не совпадает с кварталом
	//
	int    GetQuart(int * pQuartN, int * pYear) const;
	void   CheckAndSwap();
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
	DateRange & ActualizeCmp(LDATE rel, LDATE cmp);
	LDATE  low;
	LDATE  upp;
};

inline bool FASTCALL operator > (const DateRange & r1, const DateRange & r2) { return (r1.Cmp(r2) > 0); }
inline bool FASTCALL operator < (const DateRange & r1, const DateRange & r2) { return (r1.Cmp(r2) < 0); }

struct TimeRange { // @persistent
	void   Set(LTIME lowTm, LTIME uppTm);
	void   SetZero();
	bool   IsZero() const;
	//
	// Descr: Проверяет принадлежность момента tm диапазону low..upp.
	//   -- Если low == upp && low == ZEROTIME, то не зависимо от значения tm возвращается !0.
	//   -- Если low <= upp, то возвращается !0 если (tm >= low && tm <= upp) и 0 в противном случае
	//   -- Если low > upp, то !0 возвращается в случае, если tm принадлежит одному из двух периодов: [upp..24:00:00[ или [00:00:00..low].
	//
	int    FASTCALL Check(LTIME tm) const;
	//
	// Descr: Форматирует диапазон в строковый буфер rBuf. Время выводится в соответствии с форматом tmFormat.
	//   Разделитель начального и конечного времени - две точки (..)
	//   Если this->IsZero() то буфер rBuf останется пустым.
	//   Перед началом форматирования буфер rBuf очищается вызовом rBuf.Z().
	// Example:
	//   ToStr(TIMF_HMS, rBuf): 12:30:00..15:09:55; 12:30:00..; ..15:09:55
	// Returns:
	//   rBuf
	//
	SString & ToStr(long tmFormat, SString & rBuf) const;

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
bool   STDCALL checkirange(int nmb, int low, int upp);
bool   STDCALL checkirange(long nmb, long low, long upp);
bool   STDCALL checkirange(int64 nmb, int64 low, int64 upp);
//
// Descr: То же, что и checkirange но не инициирует ошибку SLIB (для тех случаев, когда производительность важна)
//
inline bool checkirangef(long nmb, long low, long upp) { return (nmb >= low && nmb <= upp); }
int    FASTCALL inrangeordefault(int val, int low, int upp, int def);
long   FASTCALL inrangeordefault(long val, long low, long upp, long def);
uint   FASTCALL inrangeordefault(uint val, uint low, uint upp, uint def);
double FASTCALL inrangeordefault(double val, double low, double upp, double def);
//
// Descr: Проверяет беззнаковое целое число на принадлежность диапазону [0..upper[.
//   Принадлежность НЕ включительная (nmb == upper - возвращает 0).
//
int    FASTCALL checkupper(uint nmb, uint upper);
int    FASTCALL checkfrange(double nmb, double low, double upp);
int    FASTCALL checkdrange(LDATE dt, LDATE low, LDATE upp);

// @construction int    FASTCALL BND(int v, int low, int upp);
// @construction int    FASTCALL BND(uint v, uint low, uint upp);
// @construction int    FASTCALL BND(double v, double low, double upp);
//
// Descr: Величина, ассоциированная с единицей измерения.
//
struct USize { // @noctr @novtbl
	USize & Set(double s, int unit = 0, int dir = DIREC_UNKN);
	void   SetInvalid();
	bool   IsValid() const;
	bool   operator !() const { return !IsValid(); }
	operator double() const { return S; }
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
	explicit SUnit(int id);
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
class SPoint2F { // @persistent
public:
	//
	// Descr: Конструктор по умолчанию. Ради скорости ничего не инициализирует.
	//
	SPoint2F();
	//
	// Descr: Конструктор. Инициализирует X и Y в xy.
	//
	explicit SPoint2F(float xy);
	SPoint2F(float x, float y);
	SPoint2F & FASTCALL operator = (const SPoint2S & p);
	SPoint2F & FASTCALL operator = (float f);
	bool   FASTCALL operator == (const SPoint2F & rS) const;
	bool   FASTCALL operator != (const SPoint2F & rS) const;
	bool   FASTCALL IsEq(const SPoint2F & rS) const;
	bool   IsZero() const;
	bool   IsPositive() const;
	SPoint2F Set(float xy);
	SPoint2F Set(float x, float y);
	SPoint2F SetZero();
	SPoint2F Neg() const;
	SPoint2F Scale(float);
	float  Ratio() const;
	float  Add() const;
	SPoint2F AddX(float _x) const;
	SPoint2F AddY(float _y) const;
	float  Sub() const;
	float  Sq() const;
	//
	// Descr: Возвращает гипотенузу прямого угла с катетами X и Y.
	// Note: Ошибки нет - функция возвращает double.
	//
	double Hypot() const;
	float  Hypotf() const;
	//
	// Descr: (a.X*X + a.Y*Y, b.X*X + b.Y*Y)
	//
	SPoint2F Combine(SPoint2F a, SPoint2F b) const;
	SPoint2F Swap() const;
	int    FASTCALL Write(SBuffer &) const;
	int    FASTCALL Read(SBuffer &);

	float  x/*X*/;
	float  y/*Y*/;
};

extern const SPoint2F ZEROFPOINT; // @global

// @v10.9.10 (replaced with SPoint2F::operator ==) int    FASTCALL operator == (SPoint2F p1, SPoint2F p2);
bool   FASTCALL operator <  (SPoint2F p1, SPoint2F p2);
bool   FASTCALL operator >  (SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL operator +  (SPoint2F p, float);
SPoint2F FASTCALL operator +  (SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL operator -  (SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL operator *  (SPoint2F p, float);
SPoint2F FASTCALL operator *  (SPoint2F p, double);
SPoint2F FASTCALL operator *  (SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL operator /  (SPoint2F p, float);
SPoint2F FASTCALL operator /  (SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL fmin(SPoint2F p1, SPoint2F p2);
SPoint2F FASTCALL fmax(SPoint2F p1, SPoint2F p2);
//
// Descr: Возвращает atan2f(p1.Y-p2.Y, p1.X-p2.X)
//
float  FASTCALL atan2(SPoint2F p1, SPoint2F p2);
//
// Descr: X = p.X + radius.X * sinf(angle), Y = p.Y - radius.Y * cosf(angle)
//
SPoint2F FASTCALL trans01(SPoint2F p, SPoint2F radius, float angle);
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
int  XMLWriteSpecSymbEntities(FILE * pStream);
int  XMLWriteSpecSymbEntities(void * pWriter);
//
//
//
extern const char * SlTxtOutOfMem; // @v11.3.12 Специальная константа для замены однотипных строк "out of memory", "no memory", etc в сторонних библиотеках.
//
//
//
class STextConst {
public:
	static const char * P_Digits;
	static const wchar_t * P_DigitsW;

	enum {
		cMon_En_Sh    = 1, // [12] Наименования месяцев (en сокращенные)
		cDow_En_Sh    = 2, // [7]  Наименования дней недели (en сокращенные) [0] = "Mon", [6] = "Sun"
		cDow_En       = 3, // [7]  Наименования дней недели (en полные) [0] = "Mon", [6] = "Sun"
		cAsciiCtrl    = 4, // [32] Мнемоники управляющих символов ASCII кодировки [0..31] "NUL", "SOH", "STX", "ETX", "EOT", "ENQ" eg.
		cBasis64      = 5, // [1] Таблица кодировки base64
		cBasis64Url   = 6, // [1] Таблица кодировки base64-url (в отличии от cBase64 последние два байта "-_")
		cAlphabetEngL = 7, // Английский алфавит в нижнем регистре
		cAlphabetEngU = 8, // Английский алфавит в верхнем регистре
	};
	//
	// Descr: Возвращает текст группы c с индексом idx.
	// Note: Если c или idx имеют инвалидное значение, то возвращает пустую строку "" (!NULL)
	//
	static const char * FASTCALL Get(int c, uint idx);
	//
	// Descr: Находит индекс текста pText в группе c.
	//   Поиск осуществляется без учета регистра символов.
	// Returns:
	//   >= 0 если текст найден
	//   -1 если текст не найден.
	//
	static int FASTCALL GetIdx(int c, const char * pText);
	//
	// Descr: Возвращает "true" если b != 0, и "false" в противном случае.
	//
	static const char * FASTCALL GetBool(int b);
};

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
	explicit SSrchPattern(const char * pPattern, long flags = 0, int algorithm = algBmGoodSfx);
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
#define STRINGIZE(var) #var // заключает выражение в кавычки
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
	SString();
	explicit SString(size_t initSize);
	explicit SString(const char *);
	SString(const char * pS, size_t len);
	SString(const SString &);
	~SString();
	void   Destroy();
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
	bool   FASTCALL operator == (const char * pS) const { return IsEq(pS); }
	bool   FASTCALL operator != (const char * pS) const { return !IsEq(pS); }
	bool   FASTCALL operator == (const SString & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const SString & rS) const { return !IsEq(rS); }
	//
	// Descr: Возвращает символ, находящийся в позиции n (n = 0..)
	//   Если n больше длины строки, то возвращает 0.
	//
	int    FASTCALL C(size_t n) const;
	//
	// Descr: Если строка имеет длину 1 символ, то возврашает этот символ,
	//   в противном случае возвращает 0.
	//
	int    Single() const;
	bool   IsEmpty() const { return (Len() == 0); }
	//int    operator !() const { return Empty(); }
	bool   NotEmpty() const { return (Len() != 0); }
	//
	// Descr: Возвращает true если после применения метода Strip строка становится пустой.
	//
	bool   NotEmptyS();
	bool   FASTCALL IsEq(const SString & rS) const;
	bool   FASTCALL IsEq(const char * pS) const;
	bool   FASTCALL IsEqNC(const SString & rS) const;
	bool   FASTCALL IsEqNC(const char * pS) const;
	bool   FASTCALL IsEqiAscii(const char * pS) const;
	bool   FASTCALL IsEqiUtf8(const char * pS) const;
	//
	// Descr: Возвращает !0 если начиная с позиции pos в строке содержится образец pPattern.
	//   Если pos выходит за границу строки this, то результат всегда 0.
	//   Если длина строки pPattern равна нулю и pos не выходит за границу строки this, то результат 1.
	//
	int    FASTCALL HasAt(uint pos, const char * pPattern) const;
	//
	// Descr: Возвращает !0 если начиная с позиции pos в строке содержится образец pPattern.
	//   При этом сравнение осуществляется без учета регистра символов в предположении, что
	//   все символы - ascii. То есть, для non-ascii символов игнор регистра не сработает!
	//
	int    FASTCALL HasAtiAscii(uint pos, const char * pPattern) const;
	operator const char * () const { return P_Buf; }
	//
	// Descr: Специализированное преобразование, сделанное главным образом ради (xmlChar *).
	// Note:
	//   Здесь нельзя создавать дополнительный перегруженный оператор operator const uchar * ()
	//   из-за того, что существующие использования класса столкнутся с неоднозначностью
	//   преобразования.
	//
	const uchar * ucptr() const { return reinterpret_cast<const uchar *>(P_Buf); }
	//
	// Descr: Сокращенная версия operator const char * ()
	//
	const char  * cptr() const { return reinterpret_cast<const char *>(P_Buf); }
	//
	// Descr: Сравнивает строку this со строкой pS.
	// ARG(pS         IN): Строка, с которой сравнивается this
	// ARG(ignoreCase IN): Сравнение реализуется без учета регистра символов
	// Returns:
	//  <0 - this < pS
	//  >0 - this > pS
	//  0  - this == pS
	//
	int    STDCALL  Cmp(const char * pS, int ignoreCase) const;
	int    FASTCALL CmpNC(const char * pS) const;
	//
	// Descr: Сравнивает префикс строки this со строкой pS.
	// Returns:
	//   0  - префикс строки this совпадает со строкой pS
	//   !0 - строка pS не совпадает с префиксом this
	//   Если pS == 0 или this->Len() < strlen(pS), то функция возвращает -1
	//
	int    STDCALL  CmpPrefix(const char * pS, int ignoreCase) const;
	bool   FASTCALL HasPrefix(const char * pS) const;
	bool   FASTCALL HasPrefixNC(const char * pS) const;
	bool   FASTCALL HasPrefixIAscii(const char * pS) const;
	//
	// Descr: Находит общий префикс максимальной длины между строкой this и pS.
	//   Результат возвращается в виде количества байт общего префикса.
	// Returns:
	//   0 - строки this и pS не имеют общего префикса
	//   >0 - длина наибольшего общего префикса this и pS.
	//
	int    FASTCALL GetLongestCommonPrefix(const char * pS) const;
	//
	// Descr: Сравнивает строку this c префиксом строки pS длиной this->Len().
	//   Если this->Len() == 0, то результат в любом случае -1.
	//
	int    STDCALL CmpL(const char * pS, int ignoreCase) const;
	int    STDCALL CmpSuffix(const char * pS, int ignoreCase) const;
	//
	// Descr: Ищет совпадения между строкой this и одной из подстрок
	//   строки pPattern. Подстроки разделяются символом div.
	// Returns:
	//   0 - строка this не совпадает ни с одной из подстрок pPattern
	//  !0 - @#[1..] номер подстроки из pPattern с которой совпала this.
	// Sample: OneOf(';', "yes;true;1", 1)
	//
	uint   OneOf(int div, const char * pPattern, int ignoreCase) const;
	int    Last() const;
	char * FASTCALL CopyTo(char *, size_t) const;
	BSTR   FASTCALL CopyToOleStr(BSTR * pBuf) const;
	SString & FASTCALL CopyFromOleStr(const BSTR s);
	SString & Z();
	//
	// Descr: Возвращает длину строки.
	//
	size_t Len() const;
	size_t BufSize() const;
	void   Obfuscate();
	const  char * SearchChar(int c, size_t * pPos) const;
	const  char * SearchCharPos(size_t startPos, int c, size_t * pPos) const;
	//
	// Descr: Определяет факт наличия в строке this символа c.
	//
	bool   FASTCALL HasChr(int c) const;
	//
	// Descr: проверяет на принадлежность символов строки диапазону [0-9]||[a-z]||[A..Z]
	// Returns:
	//   0 - не все символы из строки принадлежат указанному диапазону
	//   1 - все символы из строки принадлежат указанному диапазону
	//
	int    IsLatin() const;
	//
	// Descr: Сканирует слово, начиная с позиции *pPos. Если pPos == 0,
	//   то начинает сканирование с позиции 0. Разделителями слов являются все символы,
	//   которые не являются буквами и цифрами, а также символами '_'.
	// Returns:
	//   >0 - удалось извлечь слово, длина которого равна возвращаемому значению.
	//   0  - не удалось сосканировать ни одного символа.
	//
	int    GetWord(size_t * pPos, SString & rBuf) const;
	//
	// Descr: Разбивает строку this на отдельные фрагменты, разделенные одним из символов,
	//   перечисленных в строке pDelimChrSet. Результирующие фрагменты заносятся в
	//   набор строк (StringSet) rResult.
	//   Если pDelimChrSet == 0, то применяется набор разделителей по умолчанию " \t\n\r".
	//   Несколько разделителей идущих подряд считаются за один.
	//   Функция не очищает rResult перед тем как начнет туда вставлять новые элементы.
	// Returns:
	//   2 - в строке this встречен как минимум один разделитель
	//   1 - в строке this не было встречено ни одного разделителя - она полностью внесена в rResult
	//  -1 - строка this пуста - в rResult ничего не добавлено
	//
	int    Tokenize(const char * pDelimChrSet, StringSet & rResult) const;
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
	int    Search(const SSrchPattern * pBlk, size_t startPos, size_t * pPos) const;
#endif
		// @>>SSrchPattern::Search
	int    Search(const char * pPattern, size_t startPos, int ignoreCase, size_t * pPos) const;
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
	int    Divide(int divChr, SString & rLeft, SString & rRight) const;
	int    Wrap(uint maxLen, SString & rHead, SString & rTail) const;
	int    IsDigit() const;
	long   ToLong() const;
	ulong  ToULong() const;
	int64  ToInt64() const;
	//
	// Descr: Преобразует строку this в числовое значение типа double.
	//   Функция предварительно делает небольшую обработку строки, заменяя запятую на точку и удаляя разделители
	//   в виде пробелов и апострофов (2'000'100.86-->2000100.86). 
	//   Если такой препроцессинг не нужен или недопустим, то следует использовать функцию ToReal_Plain().
	// Note: Внутри применяет функцию strtodoub()
	// Returns:
	//   Результат преобразования. В случае ошибки функция вернет 0.0, что может быть иногда неприемлемо: 
	//   в таком случае следует использовать более низкоуровневые функции.
	//
	double ToReal() const;
	//
	// Descr: Преобразует строку this в числовое значение типа double.
	//   Функция полагает, что разделителем десятичных знаков является точка. Разделители разрядов
	//   будут трактоваться как недопустимые символы и приведут к остановке сканирования (2'000'100.86-->2).
	// Note: Внутри применяет функцию satof()
	//   В отличии от ToReal работает быстрее, но менее адаптивна.
	// Returns:
	//   Результат преобразования. В случае ошибки функция вернет 0.0, что может быть иногда неприемлемо: 
	//   в таком случае следует использовать более низкоуровневые функции.
	//
	double ToReal_Plain() const;
	float  ToFloat() const;
	//
	// Descr: Флаги функции ToIntRange
	//
	enum {
		torfDoubleDot   = 0x0001, // Допускается разделитель ..
		torfDoubleComma = 0x0002, // Допускается разделитель ,,
		torfColon       = 0x0004, // Допускается разделитель :
		torfHyphen      = 0x0008, // Допускается разделитель -
		torfAny = (torfDoubleDot|torfDoubleComma|torfColon|torfHyphen)
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
	//   Для гарантированной упорядоченности границ можно использовать вызов ExchangeForOrder(&range.low, &range.upp).
	//   Кроме того, функция не проверяет значения чисел на переполнение.
	// ARG(rRange OUT): Целочисленный диапазон, полученный в результате разбора строки
	// ARG(flags  IN): Флаги, регулирующие возможные форматы. See SString::torfXXX
	// Returns:
	//   0  - не удалось извлечь диапазон из строки. В этом случае значение rRange не изменяется.
	//   >0 - смещение от начала строки, на котором закончился успешный разбор диапазона.
	//
	int    ToIntRange(IntRange & rRange, long flags) const;
	SString & Sub(size_t startPos, size_t len, SString & rBuf) const;
	SString & Fmt(long fmt);
	SString & TrimRight();
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
	SString & TrimToDiv(size_t n, const char * pDivList);
	//
	// Descr: Вырезает из середины строки, начиная с позиции start, отрезок размером len.
	//
	SString & Excise(size_t start, size_t size);
	//
	// Descr: сдвигает строку на n позиций влево. n левых символов теряются.
	//   Если длина строки меньше n, то длина строки становится нулевой.
	//
	SString & FASTCALL ShiftLeft(size_t n = 1);
	//
	// Descr: сдвигает строку на 1 позицию влево если первый символ в строке равен chr.
	//
	SString & FASTCALL ShiftLeftChr(int chr);
	SString & Reverse();
	SString & PadLeft(size_t n, int pad);
	SString & Align(size_t width, int adj);
	SString & Chomp();
	SString & Quot(int leftQuotChar, int rightQuotChar);
	SString & ReplaceChar(int patternChr, int replaceChr);
	//
	// Descr: В строке this замещает подстроки pPattern подстроками pReplacer.
	//   Если once != 0, то производит только одну замену с начала строки.
	// Returns:
	//   Количество произведенных замен.
	//
	int    ReplaceStrR(const char * pPattern, const char * pReplacer, int once);
	//
	// Descr: В строке this замещает подстроки pPattern подстроками pReplacer.
	//   Если once != 0, то производит толко одну замену с начала строки.
	// Returns:
	//   *this.
	//
	SString & ReplaceStr(const char * pPattern, const char * pReplacer, int once);
	SString & FASTCALL ReplaceSpecSymb(int fileFormat);
	SString & FASTCALL RevertSpecSymb(int fileFormat);
	SString & ReplaceCR();
	SString & Escape();
	SString & Unescape();
	SString & ToUrl();
	SString & FromUrl();
	SString & ToOem();
	SString & ToChar();
	SString & ToUtf8();
	SString & Utf8ToChar();
	SString & Utf8ToOem();
	SString & FASTCALL Utf8ToCp(SCodepageIdent cp);
	SString & Utf8ToLower();
	SString & Utf8ToUpper();
	SString & ToUpper();
	SString & ToLower();
	SString & ToUpper1251();
	SString & ToLower1251();
	SString & FASTCALL SetCase(int ccas);
	//
	// Descr: Унифицированный транслятор кодировки строки.
	//   Основное назначение: универсализация преобразования кодировок
	//   внутри проекта Papyrus между т.н. INNER, OUTER и UTF8 кодировками.
	//
	SString & FASTCALL Transf(int ctransf /*CTRANSF_XXX*/);
	//
	// Descr:
	//
	SString & NumberToLat(uint value);
	//
	// ARG(dir IN): 1 - обрезать справа, 2 - обрезать слева, 0 - обрезать с обоих концов (аналог Strip())
	//
	SString & Strip(int dir);
	SString & Strip();
	SString & StripQuotes();
	SString & FASTCALL CopyFrom(const SString &);
	SString & FASTCALL CopyFrom(const char *);
	SString & FASTCALL SetIfEmpty(const char *);
	SString & FASTCALL SetIfEmpty(const SString &);
	int    IsAscii() const;
	bool   IsLegalUtf8() const;
	int    CopyUtf8FromUnicode(const wchar_t * pSrc, const size_t len, int strictConversion);
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
	int       GetSubFrom(const char * pStr, int div, int idx);
	int       GetIdxBySub(const char * pSubStr, int div);
	SString & FASTCALL SetInt(int val);
	SString & FASTCALL Cat(const SString &);
	SString & FASTCALL Cat(const char *);
	//
	// Descr: Эквивалент Dot().Cat(const char *)
	//
	SString & FASTCALL DotCat(const char *);
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
	SString & Space();   // @>>CatChar(' ')
	SString & FASTCALL Tab(uint c); // @>>CatChar('\t')
	SString & Tab();       // @>>CatChar('\t')
	SString & Dot();     // @>>CatChar('.')
	SString & Comma();   // @>>CatChar(',')
	SString & Semicol(); // @>>CatChar(';')
	SString & Colon();   // @v11.1.12 @>>CatChar(':')
	SString & Eq();      // @>>CatChar('=')
	SString & CR();      // @>>CatChar('\n')
	SString & CRB();     // "\xD\xA"
	SString & STDCALL  CatCharN(int chr, size_t n);
	SString & STDCALL  CatDiv(int c, int addSpaces/* = 0*//*, int ifNotEmpty = 0*/);
	SString & STDCALL  CatDivConditionally(int c, int addSpaces, bool condition);
	SString & STDCALL  CatDivIfNotEmpty(int c, int addSpaces/* = 0*/);
	SString & FASTCALL Cat(long);
	SString & FASTCALL Cat(ulong);
#ifndef _WIN32_WCE // {
	SString & Cat(int64);
	SString & Cat(uint64);
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
	SString & STDCALL  CatLongZ(long val, uint numDigits);
	SString & STDCALL  CatLongZ(int val, uint numDigits);
	SString & STDCALL  CatLongZ(uint val, uint numDigits);
	SString & STDCALL  CatLongZ(uint32 val, uint numDigits);
	SString & STDCALL  CatLongZ(int64 val, uint numDigits);
	SString & FASTCALL CatHex(long val);
	SString & FASTCALL CatHex(ulong val);
	SString & FASTCALL CatHex(int64 val);
	SString & FASTCALL CatHex(uint64 val);
	SString & FASTCALL CatHex(uint8 val);
	SString & FASTCALL CatHexUpper(uint8 val);
	SString & STDCALL  CatHex(const void * pBinary, size_t size);
	//
	// Descr: Конкатенация конструкции {"%d.%d.%d", ver, mjr, mnr}
	//
	SString & CatDotTriplet(int ver, int mjr, int mnr);
	//
	// Descr: Конкатенация конструкции {pMsg ' ' ((100 * p) / t) '%'}
	//   Если pMsg == 0, то конкатенируется только процент.
	//
	SString & CatPercentMsg(long p, long t, const char * pMsg);
	SString & Cat(double v, long fmt);
	SString & Cat(double v); // MKSFMTD(0, 6, NMBF_NOTRAILZ)
	SString & CatReal(double v);
	SString & Cat(const RealRange & rR, long fmt);
	SString & STDCALL  Cat(LDATE, long fmt);
	SString & STDCALL  Cat(LTIME, long fmt);
	//
	// Descr: == Cat(LDATE, DATF_DMY)
	//
	SString & FASTCALL Cat(LDATE);
	//
	// Descr: == Cat(LTIME, DATF_HMS)
	//
	SString & FASTCALL Cat(LTIME);
	SString & Cat(const LDATETIME &, long datFmt = DATF_DMY, long timFmt = TIMF_HMS);
	//
	// Descr: Специализированная функция, конкатенирующая текущие дату/время к строке.
	//   Полный аналог CatCurDateTime(dateFmt, timFmt)
	//
	SString & CatCurDateTime(long datFmt = DATF_DMY, long timFmt = TIMF_HMS);
	SString & Cat(const DateRange &, bool ext = false);
	SString & Cat(const S_GUID &, int fmt = S_GUID::fmtIDL);
	//
	// Descr: Конкатенация конструкции <text> с вариантами
	// ARG(kind IN):
	//   0 - <pTag>
	//   1 - </pTag>
	//   2 - <pTag/>
	//
	SString & CatTagBrace(const char * pTag, int kind);
	SString & CatTag(const char * pTag, const char * pData);
	SString & CatXmlElem(const char * pName, int typ, const StringSet * pList);
	SString & CatEq(const char * pKey, const char * pVal);
	SString & CatEqQ(const char * pKey, const char * pVal);
	SString & CatEq(const char * pKey, bool val);
	SString & CatEq(const char * pKey, uint16 val);
	SString & CatEq(const char * pKey, int val);
	SString & CatEq(const char * pKey, uint val);
	SString & CatEq(const char * pKey, long val);
	SString & CatEq(const char * pKey, ulong val);
	SString & CatEq(const char * pKey, int64 val);
	SString & CatEq(const char * pKey, uint64 val);
	SString & CatEq(const char * pKey, double val, long fmt = SFMT_QTTY);
	SString & CatEq(const char * pKey, LDATE val, long fmt = DATF_DMY|DATF_CENTURY);
	SString & CatEq(const char * pKey, LTIME val, long fmt = TIMF_HMS);
	SString & CatEq(const char * pKey, LDATETIME val, long dtFmt = DATF_DMY|DATF_CENTURY, long tmFmt = TIMF_HMS);
	//
	// Descr: Вставляет в позицию pos строки this другую строку pStr.
	//   Если pos > this->Len(), то ничего не делает,
	//   если pos == this->Len(), то this->Insert(this->Len(), pStr) == this->Cat(pStr)
	//
	SString & Insert(size_t pos, const char * pStr);
	SString & CDECL Printf(const char * pFormat, ...);
	SString & CDECL VPrintf(const char * pFormat, va_list);
	//
	// Descr: Добавляет в конец строки обратный слэш ('\\'). Если строка пустая или
	//   в конце строки уже стоит '\\' или '/', то ничего не делает.
	//
	SString & SetLastSlash();
	//
	// Descr: Добавляет в конец строки прямой слэш ('/'). Если строка пустая или
	//   в конце строки уже стоит '/', то ничего не делает.
	//   Если в конце строки стоит обратный слэш ('\\'), то заменяет его на прямой ('/').
	//
	SString & SetLastDSlash();
	//
	// Descr: Если в конце строки стоит прямой ('/') или обратный ('\\') слэш, то убирает его.
	//
	SString & RmvLastSlash();
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
	int    Read(FILE * fStream, size_t size);
	//
	// Descr: Кодирует (декодирует в зависимости от параметра decode) строку с помощью параметра pEncodeStr
	//   строка pEncodeStr содержит пары в виде: %01,254000;%02,255000,
	//   где 254000, 255000 и т.д. - кусок строки который нужно закодировать символами - %01, %02 и т.д
	//
	SString & EncodeString(const char * pSrc, const char * pEncodeStr, int decode);
	SString & EncodeMime64(const void * pBuf, size_t bufLen);
	int    DecodeMime64(void * pBuf, size_t bufLen, size_t * pRealLen) const;
	SString & Encode_QuotedPrintable(const char * pBuf, size_t maxLineLen);
	int    Decode_QuotedPrintable(SString & rBuf) const;
	int    FASTCALL Decode_XMLENT(SString & rBuf) const; // @construction
	//
	// Descr: Способы кодировки encoded-word RFC2047
	//
	enum {
		rfc2207encMime64 = 1, // mime64
		rfc2207encQP,         // quoted-printable
	};
	SString & Encode_EncodedWordRFC2047(const char * pSrcBuf, SCodepageIdent cp, int rfc2207enc);
	size_t Decode_EncodedWordRFC2047(SString & rBuf, SCodepageIdent * pCp, int * pRfc2207enc) const;
	SString & EncodeUrl(const char * pSrc, int mode);
	int    DecodeUrl(SString & rBuf) const;
	int    DecodeHex(int swapb, void * pBuf, size_t bufLen, size_t * pRealLen) const;
	//
	// Descr: Утилитная функция, формирующая унифицированную строку сообщения при разборе файла.
	//   Необходимость функции обусловлена способностью многих IDE-систем разбирать сообщения (в частности, об ошибках),
	//   формируемые сторонними программами, в формате "filename(lineno): message"
	//
	SString & FormatFileParsingMessage(const char * pFileName, int lineNo, const char * pAddedText);
	SString & Helper_MbToMb(uint srcCodepage, uint destCodepage);
	void   Cat_Unsafe(const uint8 * pChr, size_t numChr); // very-very dangerous function
	//
	// Descr: Если размер буфера менее size, то дораспределяет память до этого размера.
	// Note: Только для внутреннего использования!
	//
	bool   Ensure(uint size);
private:
	int    FASTCALL Alloc(size_t);
	SString & ShiftRight(size_t n, int padChar);
	//
	// Descr: Преобразует символ c в Quoted-Printable-формат.
	//   То есть, если c == ' ' или в диапазоне [33..126] (за исключением '=')
	//   то символ добавляется "как есть". В противном случае в виде "=HH' где HH -
	//   шестнадцатиричное представление значения c.
	// Returns:
	//   Реальное количество байт вставленных в строку this. То есть:
	//   { prev_len = this->Len(); assert(this->Len() == this->CharToQp(c) + prev_len); }
	//
	size_t FASTCALL CharToQp(char c);
	bool   Helper_IsEqiAscii(const char * pS, size_t slen) const;

	size_t L;     // @firstmember Длина строки вместе с завершающим нулем
	size_t Size;  // Размер распределенного буфера
	char * P_Buf; // Буфер, в котором хранится строка. @#if(P_Buf) then P_Buf[Len] == 0
};

int slvprintf(SString & rBuf, const char * pFormat, va_list va);
int slvsprintf_s(char * pBuf, size_t bufSize, const char * pFormat, va_list va);
int CDECL slprintf(SString & rBuf, const char * pFormat, ...);
int CDECL slprintf_cat(SString & rBuf, const char * pFormat, ...);
int CDECL slsprintf_s(char * pBuf, size_t bufSize, const char * pFormat, ...);
int CDECL slfprintf(FILE *, const char * pFormat, ...);
int CDECL slfprintf_stderr(const char * pFormat, ...);
//
//
//
class SStringU {
public:
	SStringU();
	SStringU(const SStringU &);
	SStringU(const wchar_t *);
	~SStringU();
	size_t Len() const;
	operator const wchar_t * () const { return P_Buf; }
	//
	// Descr: Сокращенная версия operator const wchar_t * ()
	//
	const wchar_t * ucptr() const { return P_Buf; }
	SStringU & FASTCALL operator = (const SStringU &);
	SStringU & FASTCALL operator = (const wchar_t *);
	bool   FASTCALL IsEq(const SStringU & rS) const;
	bool   FASTCALL IsEq(const wchar_t * pS) const;
	int    FASTCALL Cmp(const SStringU & rS) const;
	//
	// Descr: Сравнивает префикс строки this со строкой pS.
	// Returns:
	//   0  - префикс строки this совпадает со строкой pS
	//   !0 - строка pS не совпадает с префиксом this
	//   Если pS == 0 или this->Len() < strlen(pS), то функция возвращает -1
	//
	int    FASTCALL CmpPrefix(const wchar_t * pS) const;
	//
	// Descr: Определяет факт наличия в строке this символа c.
	//
	int    FASTCALL HasChr(wchar_t c) const;
	int    Search(const wchar_t * pPattern, size_t startPos, size_t * pPos) const;
	SStringU & Z();
	wchar_t Last() const;
	//
	// Descr: Возвращает символ, находящийся в позиции n (n = 0..)
	//   Если n больше длины строки, то возвращает 0.
	//
	wchar_t    FASTCALL C(size_t n) const;
	SStringU & FASTCALL CatChar(wchar_t);
	SStringU & CatN(const wchar_t * pS, size_t maxLen);
	SStringU & FASTCALL CopyFrom(const SStringU & rS);
	SStringU & FASTCALL CopyFrom(const wchar_t * pS);
	SStringU & CopyFromN(const wchar_t * pS, size_t maxLen);
	SStringU & CopyFromMb(int cp, const char * pS, size_t srcLen);
	//
	// Descr: Копирует мультибайт-строку pS в this предполагая, OUTER-кодировку pS.
	//
	SStringU & CopyFromMb_OUTER(const char * pS, size_t srcLen);
	//
	// Descr: Копирует мультибайт-строку pS в this предполагая, INNER-кодировку pS.
	//
	SStringU & CopyFromMb_INNER(const char * pS, size_t srcLen);
	wchar_t *  CopyTo(wchar_t * pS, size_t maxLen);
	SStringU & Sub(size_t startPos, size_t len, SStringU & rBuf) const;
	int    STDCALL  CopyFromUtf8(const char * pSrc, size_t srcSize);
	int    STDCALL  CopyFromUtf8R(const char * pSrc, size_t srcSize, size_t * pActualSrcSize);
	int    FASTCALL CopyFromUtf8(const SString & rS);
	int    FASTCALL CopyFromUtf8R(const SString & rS, size_t * pActualSrcSize);
	int    STDCALL  CopyFromUtf8Strict(const char * pSrc, size_t srcSize);
	int    STDCALL  CopyToUtf8(SString & rBuf, int strictConversion) const;
	int    CopyToMb(int cp, SString & rBuf) const;
	SStringU & FASTCALL Trim(size_t n);
	SStringU & ToUpper();
	SStringU & ToLower();
	SStringU & ReplaceChar(wchar_t patternChr, wchar_t replaceChr);

	enum {
		tcEmpty = 0,
		tcMix,
		tcLow,
		tcUpp,
		tcCapital
	};

	int    AnalyzeCase() const;
	long   ToLong() const;
	int64  ToInt64() const;
	//
	bool   Ensure(uint sz); // @internal
private:
	int    FASTCALL Alloc(size_t sz);
	int    Helper_CopyFromUtf8(const char * pSrc, size_t srcSize, int strictConversion, size_t * pActualSrcSize);

	size_t L; // @firstmember
	size_t Size;
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
	explicit StringSet(const char * pDelim = 0);
	StringSet(char delim, const char * pBuf);
	StringSet(const StringSet &);
	~StringSet();
	bool   FASTCALL IsEq(const StringSet & rS) const;
	bool   FASTCALL operator == (const StringSet & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const StringSet & rS) const { return !IsEq(rS); }
	//
	// Descr: Разрушает внутренний буфер экземпляра и обнуляет текущую длину данных (DataLen).
	//   Параметр Delim (разделитель элементов) не меняет.
	//
	void   destroy();
	//
	// Descr: Обнуляет текущую длину данных экземпляра, не освобождая буфер.
	// Note: Следует использовать при необходимости повторного использования объекта
	//   (распределенный ранее буфер улучшит производительность).
	//
	void   clear(/*int dontFreeBuf = 0*/);
	//
	// Descr: Функционально идентична clear(), семантически же, соответствует SString::Z()
	//   (не освобождает память и возвращает *this)
	//
	StringSet & Z();
	int    FASTCALL copy(const StringSet & rS);
	int    setBuf(const void * b, size_t len);
	int    FASTCALL setBuf(const SString & rBuf);
	//
	// Descr: устанавливает разделитель строк.
	//   Пользоваться этой функцией следует только если в наборе
	//   нет ни одной строки. Иначе начнутся сбои.
	//
	void   FASTCALL setDelim(const char * pDelim);
	uint   getDelimLen() const;
	//
	// Descr: Возвращает true если разделителем строк является бинарный ноль ('\0').
	//
	bool   isZeroDelim() const;
	uint   getCount() const;
	int    add(const char * pStr, uint * pPos);
	int    add(const char * pStr, uint32 * pPos) { return add(pStr, reinterpret_cast<uint *>(pPos)); }
	//
	// Descr: Функция добавляет в конец набора новую строку pStr.
	//   Единственное отличие от add(const char *, uint *) только в отсутствии последнего параметра.
	//   Так как вызов add(str, 0) крайне популярен, то мы расчитываем получить некоторую
	//   экономию в размере бинарного кода.
	//
	int    FASTCALL add(const char * pStr); // @>>StringSet::add(const char *, uint * pPos)
	//
	// Descr: Вставляет в конец набора this все строки из набора rS
	//
	int    FASTCALL add(const StringSet & rS);
	//
	// Descr: возвращает длину подстроки, начинающейся в позиции pos.
	//
	size_t FASTCALL getLen(uint pos) const;
	//
	// Descr: Ищет элемент набора с индексом idx [0..]. Если индекс находится
	//   в допустимом диапазоне, то присваивает по ссылке rBuf значение строки и возвращает true,
	//   в противном случае возвращает false и буфер rBuf обнуляется.
	// Note: 
	//   -- возможна ситуация, когда функция возвращает true и буфер rBuf при этом оказывается пустым
	//   поскольку в наборе могут содержаться пустые строки.
	//   -- буфер rBuf используется функцией как временный контейнер при переборе элементов набора, таким
	//   образом, возможно увеличение распределенной памяти в этом буфере. 
	//
	bool   getByIdx(uint idx, SString & rBuf) const;
	bool   get(uint * pPos, char * pStr, size_t maxlen) const;
	bool   get(uint * pPos, SString &) const;
	bool   get(uint pos, SString & rBuf) const;
	//
	// Descr: Специализированный вариант функции get(uint, SString &). Если pos == 0,
	//   то присваивает буферу rBuf нулевую строку, в остальных случаях работает
	//   как get(uint, SString &).
	//
	bool   getnz(uint pos, SString & rBuf) const;
	bool   search(const char * pPattern, uint * pPos, int ignoreCase) const;
	bool   search(const char * pPattern, CompFunc fcmp, uint * pPos, uint * pNextPos) const;
	//
	// Descr: Специальный, но часто встречаемый случай: поиск элемента, совпадающего
	//  с pPattern с точностью до регистра символов, исходя из предположения, что все
	//  символы в pPattern - ascii.
	//  ARG(pNextPos OUT): @#{vtpr0} По этому указателю присваивается индекс следующей подстроки
	//   набора.
	//
	bool   searchNcAscii(const char * pPattern, uint * pPos, uint * pNextPos = 0) const;
	//
	// Descr: Сортирует строки набора в лексикографическом порядке
	//
	void   sort();
	//
	// Descr: Сортирует строки набора в лексикографическом порядке и удаляет из него дубликаты
	//
	void   sortAndUndup();
	//
	// Descr: Переставляет элементы набора в обратном порядке: первый становится последним, последний - первым.
	//
	int    reverse();
	StringSet & FASTCALL operator =  (const StringSet &);
	StringSet & FASTCALL operator +  (const char * s);
	StringSet & FASTCALL operator += (const char * s);
	const char * getBuf() const;
	//
	// Descr: Возвращает размер данных в буфере.
	//
	size_t getDataLen() const { return DataLen; }
	size_t getSize() const;
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
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
#ifndef _WIN32_WCE // {
	int    Write(SFile & rFile, long) const;
	int    Read(SFile & rFile, long);
#endif // } _WIN32_WCE
protected:
	int    Init(const char * pDelim, size_t prealloc);
	int    FASTCALL Alloc(size_t sz);

	size_t DataLen; // @#{DataLen<=BufLen} Length of data with delimiters and trailings
	size_t Size;    // @#{BufLen>=DataLen} Length of buffer
	char * P_Buf;
	char   Delim[12];
};
//
//
//
struct SSearchReplaceParam {
	SSearchReplaceParam();
	SSearchReplaceParam & Z();

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
// Descr: Класс, обеспечивающий функционал разбора и сборки текстовых представлений путей в файловой системе.
//   Несколько статических функций обеспечивают сервисные потребности.
//
struct SPathStruc {
	//
	// Descr: Находит расширение имени файла rPath и возвращает его по указателю pExt (если тот не нулевой).
	// Returns:
	//   0 - путь rPath не содержит расширения
	//  !0 - позиция следующего за точкой первого символа расширения в rPath
	//
	static uint GetExt(const SString & rPath, SString * pExt);
	static int  ReplaceExt(SString & rPath, const char * pExt, int force);
	static int  ReplacePath(SString & rPath, const char * pNewPath, int force);
	static int  GetRelativePath(const char * pFrom, uint dwAttrFrom, const char * pTo, uint dwAttrTo, SString & rPath);
	//
	// Descr: Флаги функции SPathStruc::NormalizePath
	//
	enum {
		npfUpper    = 0x0001, // Все символы приводятся к верхнему регистру (иначе - к нижнему)
		npfSlash    = 0x0002, // Все обратные слэши заменяются на прямые (иначе - все обратные)
		npfOEM      = 0x0004, // Входящая строка pPath - в OEM-кодировке
		npfKeepCase = 0x0008, // Не меняет регистр символов
		npfCompensateDotDot = 0x0010  // @v11.2.4 Компенсировать ".." в середине пути если это возможно
			// Например: d:/abc/123/.. может быть заменено на d:/abc
	};
	//
	// Descr: Унифицирует текстовое представление пути pPath. Результат переносится в rNormalizedPath
	//
	static SString & NormalizePath(const char * pPath, long flags, SString & rNormalizedPath);
	int    Invariant(SInvariantParam *);
	SPathStruc();
	//
	// Descr: Создает экземпляр объекта и, если pPath != 0, то сразу вызывает SPathStruc::Split(pPath)
	//
	explicit SPathStruc(const char * pPath);
	//
	// Descr: Копирует в this компоненты пути pS заданные
	//   флагами flags.
	//   Если pS == 0, то соответствующие флагам компоненты
	//   обнуляются.
	//
	SPathStruc & Copy(const SPathStruc * pS, long flags);
	SPathStruc & Z();
	void   FASTCALL Split(const char * pPath);
	//
	// Descr: Комбинирует компоненты экземпляра в текстовое представление пути.
	// Note:
	//   -- Функция предварительно очищает rBuf.
	//   -- Для разделения подкаталогов используется обратный слэш ('\\')
	//   -- Инвариант по отношению к лидирующей точке компонента Ext
	//
	int    FASTCALL Merge(SString & rBuf) const;
	int    Merge(long mergeFlags, SString & rBuf) const;
	int    Merge(const SPathStruc * pPattern, long patternFlags, SString & rBuf);

	enum {
		fDrv = 0x0001, // Путь содержит имя устройства
		fDir = 0x0002, // Путь содержит имя каталога
		fNam = 0x0004, // Путь содержит имя файла
		fExt = 0x0008, // Путь содержит расширение файла
		fUNC = 0x0010  // Путь представлен в UNC-формате
	};
	SString Drv;
	SString Dir;
	SString Nam;
	SString Ext;
	long   Flags;
};
//
// Descr: Специализированный класс, хранящий целочисленный идентификатор сместе со строкой.
//    Используется ситуативно, когда это удобно.
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
	explicit SVerT(int = 0, int = 0, int = 0);
	int    Get(int *, int *, int *) const;
	int    GetMajor() const;
	int    GetMinor() const;
	int    GetRevision() const;
	void   Set(int, int, int);
	void   Set(uint32);
	bool   IsLt(int, int, int) const;
	bool   IsGt(int, int, int) const;
	bool   IsGe(int, int, int) const;
	bool   IsEq(int, int, int) const;
	int    FASTCALL Cmp(const SVerT *) const;
	SString FASTCALL ToStr(SString & rBuf) const;
	//
	// Descr: Преобразует строковое представление версии в данный экземпляр объекта.
	//   Возможен разбор 2 вариантов представления:
	//      9.9.9
	//      9.9
	//   Пробелы и табуляции перед первой цифрой незначимы.
	//   Любые символы после шаблона незначимы.
	// Returns:
	//   3 - распознано представление номера версии в формате 9.9.9
	//   2 - распознано представление номера версии в формате 9.9
	//   0 - не удалось распознать номер версии во входящей строке.
	//
	int    FASTCALL FromStr(const char * pStr);
	operator uint32() const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

	uint16 V;
	uint16 R;
};
//
// Параметр функции searchstr
//
#define SSPF_WORDS  0x0001
#define SSPF_NOCASE 0x0002

struct SSrchParam {
	SSrchParam(const char * pPattern, const char * pWordChars, int flags) : P_Pattern(pPattern), P_WordChars(pWordChars), Flags(flags)
	{
	}
	const  char * P_Pattern;
	const  char * P_WordChars; // Reserved. Must be NULL.
	int    Flags;
};

// short  bioscom(int cmd, char abyte, int port);
int    getdisk();
#ifndef  _WIN32_WCE // {
	char * FASTCALL stpcpy(char *to, const char *from);
#endif // _WIN32_WCE {
int    fnsplit(const char *, char *, char *, char *, char *);
void   fnmerge(char *, const char *, const char *, const char *nameP, const char *);

#define STRNSCPY(dest,src) strnzcpy(dest,src,SIZEOFARRAY(dest))
//
// If maxlen == 0, then call strcpy(dest, src)
// If (src == 0 && dest != 0) then dest[0] = 0;
//
char * STDCALL strnzcpy(char * pDest, const char * pSrc, size_t maxlen);
char * STDCALL strnzcpy(char * pDest, const uchar * pSrc, size_t maxlen);
char * STDCALL strnzcpy(char * pDest, const SString & rSrc, size_t maxlen);
//
// Descr: Копирует в конец строки pDest строку pSrc соблюдая ограничение на общий
//   размер destSize буфера pDest.
//   Если pDest == 0 || destSize == 0 || sstrlen(pSrc) == 0, то ничего не делает.
//   Завершающий ноль гарантированно устанавливается.
//
// Returns:
//   pDest
//
char * FASTCALL sstrncat(char * pDest, size_t destSize, const char * pSrc); // @v11.0.0
wchar_t * STDCALL  strnzcpy(wchar_t * pDest, const wchar_t * pSrc, size_t maxlen);
char    * FASTCALL sstrcpy(char * pDest, const char * pSrc);
uchar   * FASTCALL sstrcpy(uchar * pDest, const uchar * pSrc);
wchar_t * FASTCALL sstrcpy(wchar_t * pDest, const wchar_t * pSrc);
char * FASTCALL trimleft(char * pStr);
char * FASTCALL trimright(char * pStr);
//bool   FASTCALL isempty(const char * pStr); // BIN(pStr || pStr[0])
//bool   FASTCALL isempty(const uchar * pStr); // BIN(pStr || pStr[0])
//bool   FASTCALL isempty(const wchar_t * pStr); // BIN(pStr || pStr[0])
// @v11.3.0 Функции isempty() теперь в inline-варианте. Это снижает размер кода (и, очевидно, быстрее работает)
FORCEINLINE bool   isempty(const char * pStr) { return (pStr == 0 || pStr[0] == 0); }
FORCEINLINE bool   isempty(const uchar * pStr) { return (pStr == 0 || pStr[0] == 0); }
FORCEINLINE bool   isempty(const wchar_t * pStr) { return (pStr == 0 || pStr[0] == 0); }
FORCEINLINE size_t sstrlen(const char * pStr) { return pStr ? strlen(pStr) : 0; }
FORCEINLINE size_t sstrlen(const uchar * pStr) { return pStr ? strlen(reinterpret_cast<const char *>(pStr)) : 0; }
FORCEINLINE size_t sstrlen(const wchar_t * pStr) { return pStr  ? wcslen(pStr) : 0; }
FORCEINLINE int    sstrleni(const char * pStr) { return static_cast<int>(sstrlen(pStr)); }
FORCEINLINE int    sstrleni(const uchar * pStr) { return static_cast<int>(sstrlen(pStr)); }
FORCEINLINE int    sstrleni(const wchar_t * pStr) { return static_cast<int>(sstrlen(pStr)); }
//
// Descr: Если по указателю pStr находится символ (символы) перевода строки, то возвращает
//   длину комбинации (1 или 2), в противном случае возвращает 0.
//   Формат перевода строки определяется параметром eolf. 
//   Если указатель pStr нулевой, то возвращает 0.
// Attention! Функция может "заглядывать" на один символ вперед указателя pStr. Это значит, что
//   для полной безопасности (защиты от неавторизованного доступа к памяти) настоятельно рекомендуется //
//   передавать указатель на буфер, заканчивающийся каким либо валидным символом, отличным от \xD и \xA. Например, нулем.
//
uint   FASTCALL iseol(const char * pStr, SEOLFormat eolf);
size_t FASTCALL sstrnlen(const char * pStr, size_t maxLen);
const char * FASTCALL sstrchr(const char * pStr, char c);
char  * FASTCALL sstrchr(char * pStr, char c);
const wchar_t * FASTCALL sstrchr(const wchar_t * pStr, wchar_t c);
wchar_t  * FASTCALL sstrchr(wchar_t * pStr, wchar_t c);
char  * FASTCALL sstrdup(const char * pStr);
uchar * FASTCALL sstrdup(const uchar * pStr);
wchar_t * FASTCALL sstrdup(const wchar_t * pStr);
//
// Descr: определяет эквивалентность строк pS1 и pS2.
//   Если pS1 == pS2, то результат !0
// Note: Если один из указателей нелевой, а другая строка имеет нулевую длину (например, pS1 == 0 and pS2 != 0 && pS2[0] == 0) то такие строки считаются равными!
// Returns:
//   true - строки эквивалентны
//   false - строки не эквивалентны
//
bool   FASTCALL sstreq(const char * pS1, const char * pS2);
bool   FASTCALL sstreq(const uchar * pS1, const uchar * pS2);
bool   STDCALL  sstrneq(const char * pS1, const char * pS2, uint len);
bool   STDCALL  sstrneq(const uchar * pS1, const uchar * pS2, uint len);
//
// Descr: Специальный вариант для libxml - сравнение переменной типа xmlChar * с константой "xxx".
//
bool   FASTCALL sstreq(const uchar * pS1, const char * pS2);
bool   FASTCALL sstreq(const wchar_t * pS1, const wchar_t * pS2);

inline bool chreqi_ascii(int c1, int c2)
{
	if(c1 != c2) {
		if(c1 >= 'A' && c1 <= 'Z')
			c1 += ('a' - 'A');
		if(c2 >= 'A' && c2 <= 'Z')
			c2 += ('a' - 'A');
		if(c1 != c2)
			return false;
	}
	return true;
}
//
// Descr: определяет эквивалентность строк pS1 и pS2
//   без учета регистра, но только для ascii-символов.
// Returns:
//   !0 - строки эквивалентны
//   0  - строки не эквивалентны
//
bool   FASTCALL sstreqi_ascii(const char * pS1, const char * pS2);
bool   FASTCALL sstreqi_ascii(const uchar * pS1, const uchar * pS2);
bool   FASTCALL sstreqi_ascii(const wchar_t * pS1, const wchar_t * pS2);
bool   FASTCALL sstreqi_ascii(const wchar_t * pS1, const char * pS2);
//
// Descr: определяет эквивалентность строк pS1 и pS2 на отрезке не более maxlen символов
//   без учета регистра, но только для ascii-символов.
// Returns:
//   !0 - строки pS1[0..maxlen-1] и pS2[0..maxlen-1] эквивалентны
//   0  - строки не эквивалентны
//
bool   STDCALL  sstreqni_ascii(const char * pS1, const char * pS2, size_t maxlen);
//
// Descr: Определяет все ли len символов строки pS находятся в диапазоне [0..0x7f].
// Note: Функция может быть использована для быстрой идентификации необходимости
//   трансляции строки в UNICODE или для иных специализированных задач.
// Returns:
//   !0  - все len символов в pS находятся в диапазоне [0..0x7f]
//    0  - как минимум один символ в pS - не ascii (вне диапазона [0..0x7f])
//
bool   FASTCALL sisascii(const char * pS, size_t len);
char * FASTCALL strip(char * pStr);
char * FASTCALL chomp(char * pStr);
// @v10.0.0 const  char * skipws(const char * pStr, size_t * pPos);
//
// Descr: Сдвигает все символы строки pStr вправо на n символов и освободившееся
//   слева место забивает символами pad.
// Note: Предполает, что под указатель pStr выделено достаточно места чтобы вместить еще n символов.
// Returns:
//   pStr
//
char * STDCALL  padleft(char  * pStr, char pad, size_t n);
char * STDCALL  padright(char * pStr, char pad, size_t n);
char * alignstr(char * pStr, size_t width, int adj);
int    searchstr(const char * pStr, const SSrchParam & rParam, size_t * pBeg, size_t * pLen);
int    replacestr(char * pStr, const char * pReplacer, size_t * pPos, size_t * pLen, uint maxlen);
// AHTOXA convert hex or octal representation of symbol to symbol in string
int    hostrtocstr(const char * pInBuf, char * pOutBuf, size_t outBufSize);
//
// Descr: Низкоуровневая функция преобразования серии последовательных цифр в число.
//   Никаких проверок не делает и полагается на то, что в строке pT ей передается len
//   символов, являющихся десятичными цифрами ('0'..'9').
//   За счет такого минимализма очень быстрая.
// Returns:
//   Результат преобразования.
//
uint32 FASTCALL _texttodec32(const char * pT, uint len);
uint32 FASTCALL _texttodec32(const wchar_t * pT, uint len);
uint64 FASTCALL _texttodec64(const char * pT, uint len);
//
// Descr: Низкоуровневая функция преобразования серии последовательных шестнадцатиричных цифр в число.
//   Никаких проверок не делает и полагается на то, что в строке pT ей передается len
//   символов, являющихся шетнадцатиричными цифрами ('0'..'9','A'..'F','a'..'f').
//   За счет такого минимализма очень быстрая.
// Returns:
//   Результат преобразования.
//
uint32 FASTCALL _texttohex32(const char * pT, uint len);
uint32 FASTCALL _texttohex32(const wchar_t * pT, uint len);
int    FASTCALL satoi(const char * pT);
int    FASTCALL satoi(const wchar_t * pT);
int64  FASTCALL satoi64(const char * pT);
// 
// Descr: Преобразует строку в беззнаковое 8-байтовое целое число.
// Note: Пробелы, табуляция и знаки +- в начале строки пропускаются.
//   Префикс 0x в начале строки трактуется как факт того, что число представлено в шестнадцатиричном виде.
//
uint64 FASTCALL satou64(const char * pT);
//
// Descr: Преобразует строку pT в беззнаковое 8-байтовое целое число,
//   предполагая что число представлено в шестнадцатиричном виде.
// Note: Пробелы, табуляция и знаки +-, а так же префикс "0x" в начале строки пропускаются.
//
uint64 FASTCALL sxtou64(const char * pT);
int64  FASTCALL satoi64(const wchar_t * pT);
int    FASTCALL satof(const char * pBuf, double * pVal);
double FASTCALL satof(const char * pBuf);
SString & FASTCALL FormatInt(int value, SString & rBuf);
SString & FASTCALL FormatUInt(uint value, SString & rBuf);
SString & FASTCALL FormatInt64(int64 value, SString & rBuf);
SString & FASTCALL FormatUInt64(uint64 value, SString & rBuf);
int    SplitBuf(HDC hdc, SString & aBuf, size_t maxStrSize, size_t maxStrsCount);
//
//get string - used in network @vantonov
// @v10.3.0 (unused) int QuotedStringToStr(char **pStr, char * pBuf, int maxBytes, char dStr = '\"');
//
// Функции для классификации и конвертации символов
inline bool isdirslash(char c) { return oneof2(c, '/', '\\'); }
inline bool isdirslash(wchar_t c) { return oneof2(c, L'/', L'\\'); }
//
// Descr: Возвращает !0 если символ c является шестнадцатиричным знаком [0-9A-Fa-f]
//
bool   FASTCALL ishex(char c);
bool   FASTCALL ishexw(wchar_t c);
//
// Descr: Возвращает !0 если символ c является десятичным знаком [0-9]
//
bool   FASTCALL isdec(char c);
bool   FASTCALL isdecw(wchar_t c);
//
// Descr: Возвращает значение шестнадцатиричного знака c. Если c не является шестнацдатиричным знаком,
//   то возвращает 0 (для того, чтобы избежать путаницы с hex('0') следует проверять
//   символ функцией ishex(char)).
//
uint   FASTCALL hex(char c);
uint   FASTCALL hexw(wchar_t c);
//
// Descr: Преобразует два шестнадцатиричных символа pBuf[0] и pBuf[1] в
//    бинарный байт.
// Note: Если pBuf[0] или pBuf[1] не являются шестнадцатиричными символами,
//    то возвращает 0. Дабы избежать путаницы с "00" необходимо предварительно
//    проверить эти символы функцией ishex(char).
//
uint8  FASTCALL hextobyte(const char * pBuf);
inline bool isasciialpha(char ch) { return LOGIC(((uchar)(ch | 32) - 97) < 26U); }
inline bool isasciialpha(int ch) { return LOGIC(((uchar)(ch | 32) - 97) < 26U); }
inline bool isasciialpha(wchar_t ch) { return LOGIC(((uchar)(ch | 32) - 97) < 26U); }
inline bool isasciialnum(char ch) { return ((ch >= '0' && ch <= '9') || (((uchar)(ch | 32) - 97) < 26U)); }
// @v10.9.3 (replaced with isasciialpha) int    FASTCALL IsLetterASCII(int ch); // synonym of isasciialpha
int    FASTCALL IsLetter1251(int ch);
int    FASTCALL ToLower1251(int ch);
int    FASTCALL ToUpper1251(int ch);
char * FASTCALL strlwr1251(char *);
char * FASTCALL strupr1251(char *);
int    FASTCALL stricmp1251(const char *, const char *);
int    FASTCALL IsLetter866(int ch);
int    FASTCALL ToLower866(int ch);
int    FASTCALL ToUpper866(int ch);
int    FASTCALL stricmp866(const char *, const char *);
int    FASTCALL strnicmp866(const char *, const char *, size_t);
char * FASTCALL stristr866(const char *, const char *);
char * FASTCALL strlwr866(char *);
char * FASTCALL strupr866(char *);
int    FASTCALL __866_to_koi7(int);
char * FASTCALL _s_866_to_koi7(char *);
int    FASTCALL _koi8_to_866(int);
char * FASTCALL SOemToChar(char * pStr);
char * FASTCALL SCharToOem(char * pStr);
//
// Descr: Серия специальных функций и макросов для унификации компиляции UNICODE и multibyte версий.
//
const wchar_t * FASTCALL SUcSwitchW(const char * pStr);
const wchar_t * FASTCALL SUcSwitchW(const SString & rStr); // @v10.9.12
const char * FASTCALL SUcSwitchW(const wchar_t * pStr);
inline const char * FASTCALL SUcSwitchA(const char * pStr) { return pStr; }
inline const wchar_t * FASTCALL SUcSwitchA(const wchar_t * pStr) { return pStr; }
#ifdef _UNICODE
	#define SUcSwitch SUcSwitchW
#else
	#define SUcSwitch SUcSwitchA
#endif
//
// Formats for date binary representation
//
#define DF_DOS              1
#define DF_FAT              2
#define DF_XBASE            3
// @v10.2.8 #define DF_PARADOX          4
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
//
// Descr: Предопределенные периоды
//
#define PREDEFPRD_NONE         0 //
#define PREDEFPRD_TODAY        1 // Сегодня
#define PREDEFPRD_YESTERDAY    2 // Вчера
#define PREDEFPRD_LAST7DAYS    3 // Последние семь дней
#define PREDEFPRD_LASTWEEK     4 // Прошедшая неделя
#define PREDEFPRD_LASTWORKWEEK 5 // Прошедшая рабочая неделя (понедельник-пятница)
#define PREDEFPRD_THISMONTH    6 // Текущий месяц
#define PREDEFPRD_LASTMONTH    7 // Прошедший месяц
#define PREDEFPRD_THISWEEK     8 // @v11.5.4 Текущая неделя //

#define PRD_PRECDAYSMASK  0x7000
#define NUM_MONTHES        12

extern const char   daysPerMonth[NUM_MONTHES];
// @v10.2.3 (static now) extern const char * monthNames[NUM_MONTHES];
//
// Functions for date formats convertion
//
void   STDCALL  _encodedate(int d, int m, int y, void *, int);
void   STDCALL  _decodedate(int * pD, int * pM, int * pY, const void *, int);
int    STDCALL  _checkdate(int d, int m, int y);
char * STDCALL  _datefmt(int d, int m, int y, int format, char *);
void   STDCALL  encodedate(int, int, int, void *);
void   STDCALL  decodedate(int *, int *, int *, const void *);
void   STDCALL  decode_fat_datetime(uint16 fd, uint16 ft, LDATETIME *);
void   STDCALL  encode_fat_datetime(uint16 * fd, uint16 * ft, const LDATETIME *);
bool   FASTCALL IsLeapYear_Gregorian(uint y);
inline bool IsLeapYear_Julian(uint y) { return ((y % 4) == 0); }
int    FASTCALL checkdate(const void * pBinDate);
//
// Descr: Проверяет корректность даты dt. Если dt == ZERODATE и параметр zeroIsOk != 0,
//   то возвращает !0 (считает, что дата валидна).
//
int    FASTCALL checkdate(LDATE dt, int zeroIsOk);
//
// Descr: Проверяет корректность даты dt. Если dt == ZERODATE то возвращает 0 (считает, что дата не валидна).
//
int    FASTCALL checkdate(LDATE dt);
int    FASTCALL checktime(LTIME tm);
//
//
//
#define MONF_SHORT     0x0001 // Сокращенная форма
#define MONF_CASENOM   0x0002 // Полная форма (именительный падеж)
#define MONF_CASEGEN   0x0004 // Полная форма (родительный падеж)
#define MONF_OEM       0x0080 // OEM-coding

SString & STDCALL SGetMonthText(int mon, long fmt, SString & rBuf);
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
int    STDCALL strtoperiod(const char * pStr, DateRange * pPeriod, long flags);
//
// Current date getting
//
int    FASTCALL getcurdate(LDATE *);
int    FASTCALL getcurtime(LTIME *);
//
// Descr: Возвращает текущую системную дату
//
LDATE  getcurdate_();
//
// Descr: Возвращает текущее системное время //
//
LTIME  getcurtime_();
int    FASTCALL getcurdatetime(LDATETIME *);
int    FASTCALL getcurdatetime(LDATE * pDt, LTIME * pTm);
LDATETIME FASTCALL getcurdatetime_();
int    setcurdatetime(LDATETIME dtm);
int    gettimezone();
//
// Date arithmetic
//
long   STDCALL _diffdate(const void * pDest, const void * pSrc, int fmt, int _360); /* dest - src */
void   STDCALL _plusdate(void * pDest, int numdays, int format, int _360);
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
void   _plusperiod(void * pDest, int period, int numperiods, int format, int _360);
int    FASTCALL _dayofweek(const void * pDate, int format); // 0..6 (0=Sunday)
long   STDCALL diffdate(const void *, const void *, int _360);
void   STDCALL plusdate(void *, int, int _360);
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
void   STDCALL plusperiod(void * pDest, int period, int numperiods, int _360);
//
// Descr: возвращает день недели, на который приходится дата pDate.
// Parameters:
//   pDate - указатель на дату в бинарном формате BinDateFmt
//   sundayIsSeventh - если !0, то воскресенье является 7-м днем недели,
//     иначе, воскресение имеет значение 0.
// Returns:
//   Номер дня недели. См комментарии к параметру sundayIsSeventh
//
int    FASTCALL dayofweek(const void * pDate, int sundayIsSeventh = 0); // 0..6 (0=Sunday) // @>>_dayofweek
//
// Descr: возвращает количество дней в месяце month для года year.
// ARG(month IN): номер месяца @# [1..12]
// ARG(year  IN): год
// Returns:
//   номер последнего дня в месяце month года year
//   Если month < 1 || month > 12, то возвращает 0
//
int    FASTCALL dayspermonth(int month, int year);
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
int    STDCALL  GetDayOfWeekText(int options, int dayOfWeek, SString & rBuf);
int    FASTCALL GetDayOfWeekByText(const char * pText);
//
// Time format convertion
//
// @v10.0.02 int   FASTCALL _encodetime(int h, int m, int s, int ts, void *, int);
// @v10.0.02 int   FASTCALL _decodetime(int * h, int * m, int * s, int * ts, void *, int);
void   STDCALL encodetime(int h, int m, int s, int ts, void *);
void   STDCALL decodetime(int * h, int * m, int * s, int * ts, const void *);
LTIME  STDCALL encodetime(int h, int m, int s, int ts);
/*
	Result of difftime:
		(dim == 1) : hours
		(dim == 2) : minuts
		(dim == 3) : seconds
		(dim == 4) : thousandths of second
*/
long   STDCALL  DiffTime(LTIME t1, LTIME t2, int dim);
long   STDCALL  diffdatetime(LDATE d1, LTIME t1, LDATE d2, LTIME t2, int dim, long * pDiffDays);
long   STDCALL  diffdatetime(const LDATETIME &, const LDATETIME &, int dim, long * pDiffDays);
long   STDCALL  diffdatetimesec(LDATE d1, LTIME t1, LDATE d2, LTIME t2);
long   FASTCALL diffdatetimesec(const LDATETIME &, const LDATETIME &);
LDATETIME FASTCALL plusdatetime(const LDATETIME &, long plus, int dim);
LDATE  STDCALL  encodedate(int day, int month, int year);
LDATE  FASTCALL plusdate(LDATE d, long a);
long   FASTCALL diffdate(LDATE d, LDATE s);
char * FASTCALL _commfmt(long fmt, char * pBuf);
SString & FASTCALL _commfmt(long fmt, SString & rBuf);
//
// Descr: Реализация преобразования double-->string и string-->double.
// Позаимствована с модификациями у Mikhail Kupchik <Mikhail.Kupchik@prime-expert.com>. See copyright notice at dconvstr.c
//
class SIEEE754 {
public:
	//
	// Descr: Флаги состояний числа с плавающей точкой.
	// Attention: Мнемоника и значения флагов не соответствуют каким-либо фактическим 
	// или формальным стандартам и применимы только в рамках данной библиотеки.
	//
	enum {
		fSIGN          = 0x0001,
		fNAN           = 0x0002,
		fINF           = 0x0004,
		fInternalError = 0x8000  // Специальный флаг, индицирующий внутреннюю ошибку 
			// исполнения функций, которые возвращают значение в виде комбинации флагов
	};
	enum {
		fHaveWidth   = 0x0001, // DCONVSTR_FLAG_HAVE_WIDTH
		fLeftJustify = 0x0002, // DCONVSTR_FLAG_LEFT_JUSTIFY
		fSharp       = 0x0004, // DCONVSTR_FLAG_SHARP
		fPrintPlus   = 0x0008, // DCONVSTR_FLAG_PRINT_PLUS
		fSpaceIfPlus = 0x0010, // DCONVSTR_FLAG_SPACE_IF_PLUS
		fPadWithZero = 0x0020, // DCONVSTR_FLAG_PAD_WITH_ZERO
		fUppercase   = 0x0040  // DCONVSTR_FLAG_UPPERCASE
	};
	static const int DefaultPrecision; // DCONVSTR_DEFAULT_PRECISION
	// 
	// Descr: Unpack floating-point double precision binary value according to IEEE 754
	// 
	static uint STDCALL UnpackDouble(double input, uint64 * pOutBinaryMantissa, int32 * pOutBinaryExponent);
	//
	// Descr: Print IEEE 754 floating-point double precision value to string
	// @param  outbuf            Address of variable with a pointer to output buffer filled by the function.
	//   On entry, this variable is initialized by caller.
	//   On exit, this variable points to the end of printed string.
	// @param  outbuf_size       Size of output buffer filled by the function.
	//   On entry, this variable is initialized by caller to maximum allowed size.
	//   On exit, this variable contains size of unused portion of the output buffer.
	// @param  value             Input value (IEEE 754 floating-point double precision).
	// @param  format_char       Format char. Either 'e', or 'f', or 'g'. Refer to printf(3) manual for details.
	// @param  format_flags      Any combination of the above (DCONVSTR_FLAG_*).
	// @param  format_width      Format width. Used only if DCONVSTR_FLAG_HAVE_WIDTH bit is set in flags.
	//   Refer to printf(3) manual for details.
	// @param  format_precision  Format precision. Set it to DCONVSTR_DEFAULT_PRECISION if unsure.
	//   Refer to printf(3) manual for details.
	// @returns  1  if value was successfully converted to string.
	//   0  if there is not enough room in buffer or internal error happened during conversion.
	//
	static int STDCALL Print(char ** outbuf, int * outbuf_size, double value, int format_char, uint format_flags, int format_width, int format_precision);
	static uint STDCALL DoubleToDecimal(double input, uint64 * pMantissa, int32 * pExp);
	//
	// Descr: Convert string to IEEE 754 floating-point double precision value
	//   In general, interface of this function is similar to strtod(3), except for returning overflow
	//   condition instead of setting errno. If you want just to convert C-style string to double with
	//   error checking, then set input_end != NULL and use ( ret_value != 0)&&( **input_end == 0)
	//   condition as an indication of successful conversion.
	// ARG(pInput     IN): Input buffer, C-style string
	// ARG(ppInputEnd OUT): @#{vptr0} Address of pointer to end of scanned value in input buffer
	// ARG(pOutput    OUT): Conversion result (IEEE 754 floating-point double precision).
	//   Set to 0.0 if string in input buffer has syntax errors.
	// ARG(pOutputERange OUT): Address of overflow/underflow flag variable, filled by function.
	//   0  if there is no overflow/underflow condition
	//   1  if there is overflow/underflow condition: strtod(3) would set errno = ERANGE
	// Returns:
	//   1  if there were no internal errors
	//   0  if there was internal error during conversion.
	//
	static int STDCALL Scan(const char * pInput, const char ** ppInputEnd, double * pOutput, int * pOutputERange);
};

char * STDCALL realfmt(double val, long fmt, char * pBuf);
char * STDCALL intfmt(long val, long fmt, char * pBuf);
char * STDCALL int64fmt(int64 val, long fmt, char * pBuf);
//
// Descr: форматирует длинное целое число val в строку pBuf
//   Если numdigits > 0, то форматирутся с количеством символов равным numDigits
//   с набивкой слева нулями (например: longfmtz(5, 4, buf, sizeof(buf)) --> buf="0005")
//   Если numDigits <= 0, то просто переводит val в строку форматом %ld.
// Returns:
//   pBuf
//
char * STDCALL  longfmtz(long val, int numDigits, char * pBuf, size_t bufLen);
char * STDCALL  uintfmt(ulong val, long fmt, char * pBuf);
char * STDCALL  uint64fmt(uint64 val, long fmt, char * pBuf);
char * STDCALL  decfmt(const BCD_T val, int len, int prec, long fmt, char * pBuf);
char * STDCALL  datefmt(const void *, long fmt, char *);
char * STDCALL  timefmt(LTIME, long fmt, char *);
char * STDCALL  datetimefmt(LDATETIME dtm, long dtfmt, long tmfmt, char * pBuf, size_t bufLen);
char * FASTCALL periodfmt(const DateRange *, char *);
int    STDCALL  periodfmtex(const DateRange *, char *, size_t bufLen);
//
// Descr: Флаги функции TimeZoneFmt
//
enum {
	tzfmtCurrent = 0x0001, // Использовать системное значение временной зоны
	tzfmtColon   = 0x0002, // "+00:00" else "+0000"
	tzfmtSpace   = 0x0004, // " -0000"
	tzfmtConcat  = 0x0008  // Не очищать буфер результата в начале функции
};
//
// Descr: Форматирует часовую зону.
// Note: Буфер rBuf предварительно очищается функцией если не установлен флаг tzfmtConcat.
// Returns:
//    rBuf
//
SString & STDCALL TimeZoneFmt(int tz, int flags, SString & rBuf);
char * STDCALL strfmt(const char * str, long fmt, char * buf);
//char * numbertotext(double nmb, long fmt, char * pBuf);
int    STDCALL  strtorrng(const char * pBuf, double * pLow, double * pUpp);
int    FASTCALL strtorrng(const char * pBuf, RealRange & rRng);
int    FASTCALL strtodoub(const char * pBuf, double *);
int    FASTCALL strtolong(const char * pBuf, long *);
int    FASTCALL strtoulong(const char * pBuf, ulong *);
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
	strtodatefAnyDay   = 0x0100, //
	strtodatefAnyMon   = 0x0200, //
	strtodatefAnyYear  = 0x0400, //
	strtodatefInvalid  = 0x0800, // Дата, извлеченная из строки, инвалидная
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
int   STDCALL _strtodate(const char * pBuf, int format, int * pDay, int * pMonth, int * pYear, long * pRetFlags = 0);
int   STDCALL strtodate(const char * pBuf, long fmt, void *);
LDATE STDCALL strtodate_(const char * pBuf, long fmt);
//
// Descr: Заимствованная у CURL функция преобразования строки в дату/время.
// Note: поскольку встроенная библиотека libcurl зависит от SLIB, то оригинальный код
//   также использует эту функцию.
//
time_t FASTCALL Sl_Curl_GetDate(const char * p);
//
// Descr: Заимствованная у CURL функция преобразования ip-адреса в строку.
// Note: поскольку встроенная библиотека libcurl зависит от SLIB, то оригинальный код
//   также использует эту функцию.
//
char * STDCALL Sl_Curl_InetNtop(int af, const void * pSrc, char * pBuf, size_t bufSize);
//
// Descr: Преобразует строку pBuf в представление времени в формате LTIME
//   Строка может иметь вид "hh:mm:ss:hs", где
//     hh - часы; mm - минуты; ss - секунды; hs - сотые доли секунды
//   Количество цифр - не фиксированно (2:00 также допустимо как и 02:00)
//   Минуты, секунды, сотые доли секунд могут быть пропущены
//   В качестве разделителя вместо ':' можно использовать пробел или ';'
// ARG(pBuf   IN) - строка содержащая представление времени
// ARG(fmt    IN) - параметры форматирования
// ARG(pTime OUT) - указатель на структуру LTIME, в которую будет записано принятое значение времени
// Returns:
//   1
//
int   STDCALL strtotime(const char * pBuf, long fmt, LTIME * pTime); // @>>::strtok
int   STDCALL strtodatetime(const char * pBuf, LDATETIME * pDtm, long datFmt, long timFmt);
//
// Descr: Обобщанная структура поля данных. Используется в основном для работы
//   с таблицами баз данных.
// Note: Перенесено из db.h для реализации универсального сериализатора данных SSerializeContext.
//
struct BNField {
	uint   size() const { return stsize(T); }
	bool   IsEq(const BNField & rS, int nameAndTypeOnly = 0) const;
	int    getValue(const void * pDataBuf, void * pValBuf, size_t * pSize) const;
	int    setValue(void * pDataBuf, const void * pValBuf) const;
	int    putValueToString(const void * pDataBuf, char * pBuf) const;

	int32  Id;   // If object attached to dictionary then id = X$FIELD.XeId, else id = 0,1,..
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
	BNFieldList();
	BNFieldList(const BNFieldList & rS);
	~BNFieldList();
	BNFieldList & FASTCALL operator = (const BNFieldList &);
	int    FASTCALL copy(const BNFieldList *);
	bool   IsEq(const BNFieldList & rS, int nameAndTypeOnly = 0) const;
	void   reset();
	int    addField(const char * pName, TYPEID type, int id = UNDEF);
	int    FASTCALL addField(const BNField &);
	int    getFieldPosition(int fldId, uint * pPos) const;
	const  BNField & getField(uint, int byPosition = 1) const;
	const  BNField & getField(const char * pName, uint * pPos = 0) const;
	const  BNField & FASTCALL operator[](uint i) const { return getField(i); }
	uint   getCount() const { return count; }
	RECORDSIZE getRecSize() const;
	int    setFieldId(uint fldNo /* 0..(count-1) */, int fldId);
	int    IsEqualRecords(const void * pRec1, const void * pRec2, LongArray * pNeqFldList = 0) const;
	//
	// Descr: Выводит значения полей записи pRec в строковый буфер rBuf.
	//   Если pRec == 0, то в буфер выводятся наименования полей.
	//
	//void   RecordToStr(const void * pRec, SString & rBuf) const;
private:
	uint   count;
	BNField * fields;
};
//
// Descr: Универсальный итератор.
//   Механизм использования на примере:
// class Foo {
// private:
//	 class EnImp : public SEnum::Imp {
//	 public:
//	 	EnImp(const void * pArg) : I(0), P_Arg(pArg)
//		{
//		}
//		virtual int Next(void * pData)
//		{
//			if(I < 10) {
//				static_cast<double *>(pData) = I * 1.01;
//				I++;
//				return 1;
//			}
//			else
//				return 0;
//		}
//	private:
//		uint  I;
//		const void * P_Arg;
//	};
// public:
//	SEnum::Imp * Enum(const void * pArg)
//	{
//		return new EnImp(pArg);
//	}
// };
//
// void foo()
// {
//	Foo f;
//	double abc;
//	for(SEnum en = f.Enum(); en.Next(&abc);) {
//		// do something with abc
//	}
// }
//
// Класс Foo реализует механизм итерирования (от 0 до 9) посредством вложенного класса, унаследованного от SEnum::Imp, экземпляр
// которого возвращает публичной функцией Enum().
// Клиентская функция foo() не знает о реализации Foo::EnImp но знает о том, какого типа итерируемый объект (в примере double).
// Аргумент итератора pArg приведен только для примера и не используется.
//
class SEnum {
public:
	class Imp {
	public:
		virtual ~Imp();
		virtual int Next(void * pData) = 0;
	};
	SEnum(SEnum::Imp * pE = 0);
	SEnum & FASTCALL operator = (SEnum::Imp * pE);
	~SEnum();
	int    operator !() const;
	int    FASTCALL Next(void * pData);
private:
	Imp * P_E;
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
		sdCommonDocuments,
		sdTemporary // @v11.0.3
	};
	static int GetSysDir(int pathId, SString & rPath);
};

int	   pathToUNC(const char * pPath, SString & rUncPath);
// @v10.5.6 char * replacePath(char * fileName, const char * newPath, int force);
// @v10.5.6 char * replaceExt(char * fileName, const char * newExt, int force);
char * setLastSlash(char *);
char * rmvLastSlash(char *);
//
// Descr: Returns execution file name in buffer pBuf.
// Returns:
//   pBuf
//
SString & getExecPath(SString & rBuf);
bool   FASTCALL fileExists(const char * pFileName);
int    driveValid(const char * path);
int    FASTCALL IsDirectory(const char * pStr);
int    pathValid(const char * pPath, int existOnly);
// @v10.8.2 (unused) int    validFileName(const char * pFileName);
//
// Descr: формирует полное имя файла, состоящее из пути
//   до исполняемого файла и имени pName с расширением pExt.
//   Результат помещается в буфер rPath.
// Returns:
//   ссылка на rPath
// Requares:
//   @# &rPath != 0
//
SString & makeExecPathFileName(const char * pName, const char * pExt, SString & rPath);
bool   FASTCALL IsWild(const char * pFileName);
int    createDir(const char * pPath);
int    copyFileByName(const char * pSrcFileName, const char * pDestFileName);
//
// Descr: Создает пустой файл с именем pFileName.
// Returns:
//   1 - удалось создать файл с заданным именем
//   0 - pFileName == 0 || не удалось создать файл с заданным именем.
//
int    FASTCALL createEmptyFile(const char * pFileName);
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
char * MakeTempFileName(const char * pDir, const char * pPrefix,
	const char * pExt, long * pStart, char * pFileNameBuf, size_t fileNameBufLen); // @obsolete
*/
SString & STDCALL MakeTempFileName(const char * pDir, const char * pPrefix, const char * pExt, long * pStart, SString & rBuf);
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
long   SearchStrInFile(FILE *, long startOffset, const char * pStr, int ignoreCase);

#define SPRGRS_CONTINUE  0 // for WIN32 eq PROGRESS_CONTINUE
#define SPRGRS_CANCEL    1 // for WIN32 eq PROGRESS_CANCEL
#define SPRGRS_STOP      2 // for WIN32 eq PROGRESS_STOP
#define SPRGRS_QUITE     3 // for WIN32 eq PROGRESS_QUITE

typedef int (* SDataMoveProgressProc)(const SDataMoveProgressInfo *);
//
// Descr: Структура данных, информирующая о ходе выполнения процесса передачи данных.
//   Структура предполагает возможность передачи нескольких элементов данных. При этом
//   предполагается вероятность незнания относительно общего размера данных для передачи.
//
struct SDataMoveProgressInfo {
	SDataMoveProgressInfo();

	int64  SizeDone;  // Количество данных текущего элемента, переданных на момент вызова callback
	int64  SizeTotal; // Общий размер передаваемых данных в текущем элементе
	uint32 OverallItemsDone;  // Количество элементов, передача которых завершена к моменту вызова callback
	uint32 OverallItemsCount; // Общее количество элементов для передачи
	int64  OverallSizeDone;   // Количество данных всех элементов, переданное на момент вызова callback
	int64  OverallSizeTotal;  // Общий размер передаваемых данных всех элементов
	const  char * P_Src;  // Указатель на текст, информирующий об источнике данных
	const  char * P_Dest; // Указатель на текст, информирующий о получателе данных
	void * ExtraPtr;      // Указатель на прикладную структуру данных, поставляемую вызывающей функцией
	//
	SDataMoveProgressProc Proc; // На случай, если необходимо связать прогресс-функцию с внешним компонентом
		// этот указатель упрощает жизнь - внешнему компоненту передается указатель на SDataMoveProgressInfo
		// и вспомогательная функция (соответствующая спецификации внешнего компонента) сделает так:
		// { SDataMoveProgressInfo * p_info = (SDataMoveProgressInfo *)extraPtr; p_info->Proc(p_info); }
};

typedef int (* PercentFunc)(long, long, const char *, int);

int SCopyFile(const char * pSrcFileName, const char * pDestFileName, SDataMoveProgressProc, long shareMode, void * pExtra);
// @v10.8.0 int compress(char *, char *, int, PercentFunc, ulong*);
int PKZip(const char * pSrcPath, const char * pDestPath, const char * pZipDir);
int DoCompress(const char * pSrc, const char * pDest, int64 * pFileSize, int compress, PercentFunc pf);
//
//
//
struct ExecVDosParam {
	ExecVDosParam();
	enum {
		fExitAfter = 0x0001,
		fWait      = 0x0002
	};
	long   Flags;
	SString ExePath;
	SString StartUpPath;
	StringSet Batch;
};

int ExecVDos(const ExecVDosParam & rParam);
//
//
//
struct SDirEntry { // @flat
	SDirEntry();
	SDirEntry & FASTCALL operator = (const _wfinddatai64_t & rS);
	SDirEntry & FASTCALL operator = (const WIN32_FIND_DATAA & rS);
	SDirEntry & FASTCALL operator = (const WIN32_FIND_DATAW & rS);
	SString & GetNameUtf8(SString & rBuf) const;
	SString & GetNameA(SString & rBuf) const;
	SString & GetNameUtf8(const char * pPath, SString & rBuf) const;
	SString & GetNameA(const char * pPath, SString & rBuf) const;
	//
	// Descr: Возвращает !0 если является каталогом
	//
	bool   IsFolder() const;
	//
	// Descr: Возвращает !0 если является ссылкой на каталог верхнего уровня ("..")
	//
	bool   IsUpFolder() const;
	//
	// Descr: Возвращает !0 если является ссылкой на текущий каталог (".")
	//
	bool   IsSelf() const;
	//
	// Descr: Возвращает !0 если является файлом (== (!IsFolder() && !IsUpFolder() && !IsSelf()))
	//
	bool   IsFile() const;

	enum {
		attrRdOnly = 0x00000001, // dos/win _A_RDONLY
		attrHidden = 0x00000002, // dos/win _A_HIDDEN
		attrSystem = 0x00000004, // dos/win _A_SYSTEM
		attrSubdir = 0x00000010, // dos/win _A_SUBDIR / unix attr 'd'
		attrArch   = 0x00000020, // dos/win _A_ARCH
		//
		// Следующие 9 флагов имеют внутренние значения не сопоставленные с иными стандартами или канонизациями.
		//
		attrOwpR   = 0x00010000, // unix owner permission Read
		attrOwpW   = 0x00020000, // unix owner permission Write
		attrOwpX   = 0x00040000, // unix owner permission Execute
		attrGrpR   = 0x00080000, // unix group permission Read
		attrGrpW   = 0x00100000, // unix group permission Write
		attrGrpX   = 0x00200000, // unix group permission Execute
		attrUsrR   = 0x00400000, // unix user permission Read
		attrUsrW   = 0x00800000, // unix user permission Write
		attrUsrX   = 0x01000000, // unix user permission Execute
	};

	uint32 Attr;
	LDATETIME CreateTime;
	LDATETIME AccessTime;
	LDATETIME WriteTime;
	int64  Size;
	//char   FileName[1024];
	wchar_t Name[1024]; // @v11.6.0 char-->wchar_t
};

class SDirec {
public:
	static int GetSingle(const char * pPath, SDirEntry * pEntry);
	SDirec();
	explicit SDirec(const char * pathname, int enumDirs = 0);
	~SDirec();
	int    Init(const char * pathname, int enumDirs = 0);
	int    FASTCALL Next(SDirEntry *);
private:
	int    FindFirst();
	int    FindNext();
	int    FindClose();

	wchar_t InitPath[ALIGNSIZE(MAXPATH, 2)]; // @v10.8.0 MAXPATH-->ALIGNSIZE(MAXPATH, 2) // @v11.6.0 char-->wchar_t
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
	SFindFile(const char * pPath = 0, const char * pFileName = 0);
	//
	// Descr: Запускает механизм поиска. Предварительно должны быть инициализированы
	//   необходимые поля.
	//
	int    Run();
	//
	// Descr: вызывается на каждый найденный по заданным условиям файл, а также
	//   для всех каталогов, вложенных в стартовый каталог P_Path.
	//   Для каталогов поле pEntry->FileName[0] == 0.
	// Returns:
	//   !0 - метод Run должен продолжать работу
	//   0 - метод Run следует остановить. В этом случае Run() вернет 0.
	//
	virtual int CallbackProc(const char * pPath, SDirEntry * pEntry);

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
void * catmem(void * pDest, size_t destSz, const void * pSrc, size_t srcSz);
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
int    memdword(void * p, size_t size, uint32 k, size_t * pOffs);
//
// Descr: Вызов memzero эквивалентен memset(p, 0, s). Так как такие
//   вызовы весьма часты, то использование memzero даст выигрыш
//   в размере программного кода.
//   Кроме того, memzero проверяет указатель p на ноль, что увеличивает
//   безопасность кода.
//
void * FASTCALL memzero(void * p, size_t s);
bool   FASTCALL ismemzero(const void * p, size_t s);
//
// Descr: Обнуляет память по указателю &p размером sizeof(p).
// Attention! Не применять для классов с виртуальными функциями и динамически распределенных объектов.
//
#define MEMSZERO(p)  memzero(&(p), sizeof(p))
#define THISZERO()   memzero(this, sizeof(*this))
//
// Descr: Заполняет буфер pBuf длиной bufSize случайными данными.
//   Используется для обфусцирования блоков памяти с целью не дать возможности врагам узнать наши секреты :)
//   Если серьезно, то буферы, в которых хранились секретные данные должны затиратся этой функцией.
// Note: Функция использует локальный по отношению к потоку генератор случайных чисел (SLS.GetTLA().Rg)
//
void   FASTCALL SObfuscateBuffer(void * pBuf, size_t bufSize);
//
// Descr: Shortcut for the initializing Windows API structures
//
#define INITWINAPISTRUCT(p) { MEMSZERO(p); p.cbSize = sizeof(p); } // @v10.9.12
#define INITWINBITMAPINFO(p) { MEMSZERO(p); p.bmiHeader.biSize = sizeof(p.bmiHeader); } // @v11.0.2
//
// Descr: Меняет порядок байтов двухбайтового слова
//
uint16 FASTCALL swapw(uint16);
//
// Descr: Меняет местами слова в двойном слове dw
//
uint32 FASTCALL swapdw(uint32 dw);
//
//
//
inline uint16 FASTCALL sbswap16(uint16 n) { return static_cast<uint16>(((n & 0x00FFU) << 8) | ((n & 0xFF00U) >> 8)); }
inline uint32 FASTCALL sbswap32(uint32 n) { return static_cast<uint32>(((n & 0x000000FFU) << 24) | ((n & 0x0000FF00U) << 8) | ((n & 0x00FF0000U) >> 8) | ((n & 0xFF000000U) >> 24)); }
inline uint64 FASTCALL sbswap64(uint64 n) { return static_cast<uint64>(((n & 0x00000000000000FFULL) << 56) | ((n & 0x000000000000FF00ULL) << 40) |
	((n & 0x0000000000FF0000ULL) << 24) | ((n & 0x00000000FF000000ULL) << 8)  | ((n & 0x000000FF00000000ULL) >> 8) |
	((n & 0x0000FF0000000000ULL) >> 24) | ((n & 0x00FF000000000000ULL) >> 40) | ((n & 0xFF00000000000000ULL) >> 56)); }
//
// Descr: Находит самый старший установленный бит (MSB) в x.
// Returns:
//   Если x == 0, то 0, в противном случае единственный установленный
//   бит в возвращенном значении будет в позиции MSB(x)
//   Например:
//     msb32(0) == 0
//     msb32(1) == 0x01
//     msb32(0x8030) == 0x8000
//
uint32 FASTCALL msb32(uint32 x);
char * FASTCALL newStr(const char *);
//
// Descr: Меняет значения по указателям pA и pB один с другим
//
//void   FASTCALL Exchange(int * pA, int * pB);
//void   FASTCALL Exchange(uint * pA, uint * pB);
//void   FASTCALL ExchangeForOrder(int * pA, int * pB);
//void   FASTCALL Exchange(long * pA, long * pB);
//
// Descr: Если *pA > *pB, то меняет значения по этим указателям местами,
//   в противном случае ничего не делает
//
//void   FASTCALL ExchangeForOrder(long * pA, long * pB);
//void   FASTCALL Exchange(ulong * pA, ulong * pB);
//void   FASTCALL Exchange(int16 * pA, int16 * pB);
//void   FASTCALL ExchangeForOrder(int16 * pA, int16 * pB);
//void   FASTCALL Exchange(int64 * pA, int64 * pB);
//void   FASTCALL Exchange(float * pA, float * pB);
//void   FASTCALL Exchange(double * pA, double * pB);
//
// Descr: Меняет значения по указателям pA и pB один с другим
//
template <class T> void SExchange(T * pA, T * pB)
{
	T temp = *pA;
	*pA = *pB;
	*pB = temp;
}
//
// Descr: Если *pA > *pB, то меняет значения по этим указателям местами,
//   в противном случае ничего не делает
//
template <class T> void SExchangeForOrder(T * pA, T * pB)
{
	assert(pA && pB);
	if(*pA > *pB) {
		T temp = *pA;
		*pA = *pB;
		*pB = temp;
	}
}
//
// Descr: Если *pA > *pB, то меняет значения по этим указателям местами,
//   в противном случае ничего не делает
//
//void   FASTCALL ExchangeForOrder(double * pA, double * pB);
int    FASTCALL cmp_long(long a, long b);
int    FASTCALL cmp_ulong(ulong a, ulong b);
int    FASTCALL cmp_int64(int64 a, int64 b);
int    FASTCALL cmp_double(double a, double b);
//
// Decimal format (COBOL) functions
//
double STDCALL dectobin(const char * dec, int16 len, int16 prec);
char * STDCALL dectostr(const char * dec, int16 len, int16 prec, char * buf);
void   STDCALL dectodec(double v, char * dec, int16 len, int16 prec);
void   STDCALL dectobcd(char * dec, char * bcd, int16 len);
int    STDCALL deccmp(const char * dc1, const char * dc2, int16 len);
//
//
//
class IterCounter {
public:
	IterCounter();
	operator ulong() const { return Count; }
	void   FASTCALL Init(ulong total = 0L);
	void   FASTCALL SetTotal(ulong total) { Total = total; }
	IterCounter & Increment() { Count++; return *this; }
	IterCounter & FASTCALL Add(long a) { Count += a; return *this; }
	ulong  GetTotal() const { return Total; }
private:
	ulong  Count;
	ulong  Total;
};
//
// Descr: Просто удобная и часто используемая структура.
//   Так как не содержит конструктора и деструктора, может
//   быть использована в union'ах.
//
struct SBaseBuffer {
	// no constructor (used in unions)
	// no destructor  (used in unions)
	SBaseBuffer & Init();
	SBaseBuffer & Destroy();
	bool   FASTCALL IsEq(const SBaseBuffer & rS) const;
	//
	// Descr: Копирует содержимое буфера rS в буфер this.
	// Note: И rS и this должны быть "правильными" объектами. То есть, для них
	//   когда-то до этого должны были быть вызваны методы Init().
	//   Если указатели P_Buf или размеры Size этих экземпляров инвалидны, то
	//   последствия непредсказуемы.
	//
	bool   FASTCALL Copy(const SBaseBuffer & rS);
	void   Zero();
	void   Set(void * pBuf, size_t size);
	int    FASTCALL Alloc(size_t sz);
	int    Put(size_t offs, const void * pSrc, size_t size);
    int    Put(size_t offs, uint8 v);
    int    Put(size_t offs, uint16 v);
    int    Put(size_t offs, uint32 v);
    int    Put(size_t offs, uint64 v);
    int    Put(size_t offs, int8 v);
    int    Put(size_t offs, int16 v);
    int    Put(size_t offs, int32 v);
    int    Put(size_t offs, int64 v);
    int    Get(size_t offs, void * pDest, size_t size) const;
    int    Get(size_t offs, uint8 & rV) const;
    int    Get(size_t offs, uint16 & rV) const;
    int    Get(size_t offs, uint32 & rV) const;
    int    Get(size_t offs, uint64 & rV) const;
    int    Get(size_t offs, int8 & rV) const;
    int    Get(size_t offs, int16 & rV) const;
    int    Get(size_t offs, int32 & rV) const;
    int    Get(size_t offs, int64 & rV) const;
	//
	char * P_Buf; // @firstmember
	size_t Size;
};
//
// Descr: Простой буфер с минимальной защитой. Все, что он умеет - распределить
//   память и разрушить распределенную память в деструкторе.
//   Обращение к внутреннему указателю никак не защищено.
//
class STempBuffer : private SBaseBuffer {
public:
	explicit STempBuffer(size_t sz);
	~STempBuffer();
	STempBuffer & FASTCALL operator = (const STempBuffer & rS);
	bool   IsValid() const { return (P_Buf != 0); }
	size_t GetSize() const { return Size; }
	operator char * () { return P_Buf; }
	operator const char * () const { return P_Buf; }
	const  uchar * ucptr() const { return reinterpret_cast<const uchar *>(P_Buf); }
	const  char * cptr() const { return P_Buf; }
	const  void * vcptr() const { return P_Buf; }
	void * vptr() { return P_Buf; }
	void * vptr(size_t offs) { return (PTR8(P_Buf)+offs); }
	int    FASTCALL Alloc(size_t sz);
	//
	// Descr: Если текущий размер буфера больше или равен sz, то ничего не делает, в противном
	//   случае перераспределяет размер до sz байт.
	//
	int    FASTCALL AllocIncr(size_t sz);
};
//
// Descr: Буфер, хранящий бинарные данные определенной длины. Унаследован от SBaseBuffer, содержит
//   конструктор, деструктор и фактическую длину данных, которая меньше или равна распределенному
//   объему буфера.
//
class SBinaryChunk : private SBaseBuffer {
public:
	SBinaryChunk();
	SBinaryChunk(const void * pData, size_t len);
	SBinaryChunk(const SBinaryChunk & rS);
	~SBinaryChunk();
	SBinaryChunk & FASTCALL operator = (const SBinaryChunk & rS);
	bool   FASTCALL IsEq(const SBinaryChunk & rS) const;
	//
	// Descr: Сравнивает на равенство экземпляр this с участком памяти pData длиной len.
	//   Для выполнения равенства len должна быть равна this->Len() и данные pData должны
	//   быть эквивалентны данным по указателю this->P_Buf.
	//   Если len == 0 и this->Len() == 0, то равенство выполняется независимо от значений
	//   pData, this->P_Buf и данных по этим указателям.
	//
	bool   FASTCALL IsEq(const void * pData, size_t len) const;
	bool   FASTCALL operator == (const SBinaryChunk & rS) const;
	bool   FASTCALL operator != (const SBinaryChunk & rS) const;
	SBinaryChunk & Z();
	bool   operator !() const { return Len() == 0; }
	size_t Len() const;
	const void * PtrC() const;
	const void * PtrC(size_t offs) const;
	void * Ptr();
	void * Ptr(size_t offs);
	//
	// Descr: Генерирует отрезок случайных данных длиной len.
	//   Если произошла ошибка (например, недостаточно памяти), то обнуляет длину отрезка.
	//   Использует штатный генератор случайных чисел из текущего потока (SLS.GetTLA().Rg)
	// Returns:
	//   *this
	//
	SBinaryChunk & Randomize(size_t len);
	//
	// Descr: Преобразует все байты отрезка в строку в "сыром" виде. То есть, каждый байт
	//   передается в строку как есть. Предполагается, что вызывающая функция знает что делает
	//   и в отрезке действительно хранятся вразумительные символы.
	// Returns:
	//   Ссылку на rBuf
	//
	SString & ToRawStr(SString & rBuf) const;
	//
	// Descr: Кодирует содержимое буфера в формате mime64 и результат вносит в строку rBuf.
	//
	SString & Mime64(SString & rBuf) const;
	//
	// Descr: Заносит в буфер бинарные данные, закодированные как MIME64 в z-строке pMimeString.
	//   Буфер this предварительно очищается.
	//   Нулевой указатель pMimeString или нулевая длина строки считаются валидным случаем:
	//   в результате функция просто очистит буфер this.
	// Returns:
	//   >0 - данные успешно извлечены из строки и помещены в буфер.
	//   0  - ошибка (либо не удалось декодировать pMimeString, либо не достаточно памяти, либо ошибка во внутреннем состоянии буфера).
	//
	int    FromMime64(const char * pMimeString);
	//
	// Descr: Кодирует содержимое буфера в формате hex и результат вносит в строку rBuf.
	//
	SString & Hex(SString & rBuf) const;
	//
	// Descr: Заносит в буфер бинарные данные, закодированные как HEX в z-строке pHexString.
	//   Буфер this предварительно очищается.
	//   Нулевой указатель pHexString или нулевая длина строки считаются валидным случаем:
	//   в результате функция просто очистит буфер this.
	// Returns:
	//   >0 - данные успешно извлечены из строки и помещены в буфер.
	//   0  - ошибка (либо не удалось декодировать pHexString, либо не достаточно памяти, либо ошибка во внутреннем состоянии буфера).
	//
	int    FromHex(const char * pHexString);
	//
	// Descr: Гарантирует, что длина отрезка равна len не заботясь о содержимом отрезка.
	//   То есть, если перед вызовом Len() > len, то укорачивает отрезок до len,
	//   если перед вызовом Len() < len, то распределяет дополнительное пространство.
	//   Вызов Ensure(0) эквивалентен Z().
	//   После вызова Ensure() нельзя делать никаких предположений о содержимом отрезка.
	// Returns:
	//   true - функция успешно выполнена
	//   false - ошибка
	//
	bool   Ensure(size_t len);
	//
	// Descr: Формирует отрезок длиной len и копирует в него данные из указателя pData.
	//   Если pData == 0 и len == 0 то буфер становится пустым (память не освобождается, то есть вызов Put(0, 0) эквивалентен Z()).
	// Returns:
	//   true - функция выполнена успешно
	//   false - ошибка
	//
	bool   Put(const void * pData, size_t len);
	//
	// Descr: В хвост текущего отрезка дописывает данные из указателя pData длиной len.
	//   При успешном завершении длина данных в отрезке становится равной предшествующей длине плюс len.
	// Returns:
	//   true - функция выполнена успешно
	//   false - ошибка
	//
	bool   Cat(const void * pData, size_t len);
	bool   Cat(const SBinaryChunk & rS);
	bool   Cat(uint8 byte);
	//
	// Descr: Формирует отрезок длиной len и заполняет его содержимое байтом byte.
	//   При успешном завершении GetLen() == len.
	//   Если len == 0, то результат функции эквивалентен вызову Z()
	// Returns:
	//   >0 - функция выполнена успешно
	//    0 - ошибка
	//
	int    Set(uint8 byte, size_t len);
private:
	int    CheckInvariants() const;
	size_t L;
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
		fMovable   = 0x0002, // При записи в буфер можно считать, что все содержимое до RdOffs не нужно и его можно перезаписать
		fError     = 0x8000  // Ошибка. Внутренний флаг. При попытке передать этот флаг в конструктор, он сбрасывается.
	};
	//
	// Descr: Опции сохранения/восстановления объектов
	//
	enum {
		ffAryCount32    = 0x0001, // Количество элементов в массиве сохранять как 4-x байтовое значение (по умолчанию - 2 байта)
		ffAryForceEmpty = 0x0002  // Сохранить массив как имеющий 0 элементов, даже если он не пустой.
	};
	int    InvariantC(SInvariantParam *) const;
	explicit SBuffer(size_t initSize = 0, long flags = fMovable);
	SBuffer(const SBuffer &);
	~SBuffer();
	void   Destroy();
	int    FASTCALL Copy(const SBuffer &);
	SBuffer & FASTCALL operator = (const SBuffer & s);
	//
	// Desct: Возвращает 0 если в объекте произошла ошибка.
	// Note: non-const поскольку устанавливает флаг fError в случае обнаружения ошибки
	//
	bool   IsValid();
	//
	// Descr: Сравнивает буфер this c буфером rS.
	//   Сравнение осуществляется бинарным сопоставлением между участками памяти this->P_Buf[0..this->WrOffs-1]
	//   и rS.P_Buf[0..rS.WrOffs-1].
	//   Пустые буферы (WrOffs == 0) считаются равными.
	//
	bool   FASTCALL IsEq(const SBuffer & rS) const;
	//operator const void * () const { return P_Buf; }
	const void * constptr() const { return P_Buf; }
	size_t GetSize() const { return Size; }
	size_t GetRdOffs() const { return RdOffs; }
	size_t GetWrOffs() const { return WrOffs; }
	const  void * FASTCALL GetBuf(size_t offs = 0) const;
	const  char * FASTCALL GetBufC(size_t offs = 0) const;
	const  int8 * FASTCALL GetBufI8(size_t offs = 0) const;
	const  uint8 * FASTCALL GetBufU8(size_t offs = 0) const;
	void   FASTCALL SetRdOffs(size_t offs);
	void   FASTCALL SetWrOffs(size_t offs);
	//
	// Descr: Сбрасывает в ноль указатели WrOffs и RdOffs.
	//   Таким образом, после выполнения этой операции буфер сохраняет
	//   свой изначальный распределенный размер, но пуст.
	// Returns:
	//   *this
	//
	SBuffer & Z();
	//
	// Descr: Записывает в буфер данные, на которые указывает pSrc в размере srcLen.
	//
	int    FASTCALL Write(const void * pSrc, size_t srcLen);
	//
	// Descr: Возвращает количество доступных для считывания байт буфера.
	//
	size_t GetAvailableSize() const;
	int    GetAvailableSizeI() const { return static_cast<int>(GetAvailableSize()); }
	//
	// Descr: Считывает в буфер pBuf, начиная со смещения RdOffs, доступное количество
	//   байт данных, но не более чем bufLen.
	//   Увеличивает указатель RdOffs на количество байт, скопированных в буфер pBuf.
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
	size_t ReadTerm(const char * pTerm, void * pBuf, size_t bufLen);
	size_t ReadTermStr(const char * pTerm, SString & rBuf);
	size_t FASTCALL ReadLine(SString & rBuf);
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
	int    FASTCALL Write(const /*SArray*/SVectorBase * pAry, long options /*= 0*/);
	int    FASTCALL Read(SArray * pAry, long options /* = 0*/);
	int    FASTCALL Read(SVector * pAry, long options /* = 0*/);
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
	int    WriteToFile(FILE * f, uint sign, uint32 * pActualBytes);
	int    ReadFromFile(FILE * f, uint sign);
protected:
	void * FASTCALL Ptr(size_t) const;
private:
	int    FASTCALL Alloc(size_t);
	int    Search(const char * pStr, size_t * pPos) const;
	int    FASTCALL Helper_Read(SVectorBase * pAry, long options /* = 0*/);

	size_t Size;   // Размер буфера
	size_t WrOffs; // Смещение до которого буфер заполнен
	size_t RdOffs; // Смещение до которого осуществлено считывание содержимого буфера.
		// Теоретически, часть буфера [0..RdOffs] не нужна и это содержимое буфера может быть
		// удалено. Так происходит если Flags & fMovable.
	long   Flags;
	void * P_Buf;
};
//
// Descr: Класс, реализующий механизм хранения идентифицированных отрезков бинарных данных в общем пуле.
// Note: Значение идентификатора отрезка 0xffffffff зарезервировано для индикации конца набора и не может быть использовано
//   как регулярный идентификатор отрезка.
//
class SBinarySet : private SBaseBuffer {
public:
	//
	// Нулевой идентификатор обозначает свободный блок
	//
	SBinarySet();
	SBinarySet(const SBinarySet & rS);
	~SBinarySet();
	SBinarySet & Z();
	SBinarySet & FASTCALL operator = (const SBinarySet & rS);
	const  uint32 GetSize() const { return static_cast<uint32>(SBaseBuffer::Size); }
	const  uint32 GetDataLen() const { return static_cast<uint32>(DataLen); }
	int    FASTCALL GetChunkIdList(LongArray & rList) const;
	//
	// Descr: Функция выясняет эквивалентен ли экземпляр rS экземпляру this.
	//   Сравнение осуществляется по каждому отрезку, находящемуся в this.
	//
	bool   FASTCALL IsEq(const SBinarySet & rS) const;
	bool   IsValid() const;
	int    Ensure(size_t size);
	//
	// Descr: Реализует сериализацию объекта в буфер rBuf.
	// Note: Эта функция по спецификации отличается от большинства аналогичных в других классах -
	//   если указатель pSCtx == 0, то в зависимости от dir она пишет в буфер rBuf или читает из него
	//   данные в сыром виде. При чтении проверяются инварианты.
	//   В случае pSCtx == 0 чтение осуществляется начиная с текущей позиции чтения rBuf.
	//
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	//
	// Descr: Дескриптор стратегии сжатия больших отрезков при внесении в пул.
	//   На начальном этапе применяется только алгоритм сжатия zlib.
	//   Если размер отрезка, добавляемого в пул равен или превышает MinChunkSizeToCompress,
	//   то отрезок сжимается и впереди добавляется сигнатура Ssc_CompressionSignature
	//
	struct DeflateStrategy {
		DeflateStrategy() : MinChunkSizeToCompress(0), CompressMethod(0)
		{
		}
		explicit DeflateStrategy(uint16 minChunkSize) : MinChunkSizeToCompress(minChunkSize), CompressMethod(0)
		{
		}
		uint16 MinChunkSizeToCompress;
		int16  CompressMethod; // Reserved.
	};
	//
	// Descr: Сохраняет отрезок данных pData с идентификатором id и размером size в пуле.
	//   Если pData == 0 или size == 0, то, если отрезок с идентификатором id будет
	//   найден в пуле, то будет объявлен неиспользуемым (удален, другими словами).
	// Returns:
	//   >0 - данные pData успешно помещены в пул (или удалены, если pData == 0 или size == 0)
	//   <0 - попытка удалить отрезок с идентификатором id провалилась поскольку такой идентификатор в пуле не найден
	//    0 - ошибка (например, недостаточно памяти).
	//
	int    Put(uint32 id, const void * pData, uint32 size, const DeflateStrategy * pDs = 0);
	int    Put(uint32 id, const SBinaryChunk & rData, const DeflateStrategy * pDs = 0);
	//
	// Descr: Ищет отрезок с идентификатором id в пуле и, если таковой найден, то
	//   копирует содержимое отрезка в буфер по указателю pResult (если pResult != 0).
	// Returns:
	//   true - отрезок с идентификатором id найден
	//   false - либо идентификатор id не найден, либо возникли проблемы с распределением памяти для буфера
	//      результата pResult, либо какие-то сложности со внутрениим состояним пула.
	//
	bool   Get(uint32 id, SBinaryChunk * pResult) const;
	int    Enum(size_t * pPos, uint32 * pId, SBinaryChunk * pResult) const;
	//
	// Descr: Копирует из сета rS в сет this отрезки с идентификаторами, перечисленными в rIdList.
	//   Сет this перед выполнением функции не очищается (то, что там было, если не включено в rIdList, то остается нетронутым).
	//   Если отрезок с идентификатором, указанным в rIdList уже есть в this, то он перезаписывается.
	//   Если errOnAbsenseAny == true, то если какой-либо из идентификаторов, перечисленных в rIdList отсутствует
	//   в rS, то функция возвращает 0 ничего при этом в this не меняя.
	// Returns:
	//   >0 - по крайней мере один отрезок в this скопирован (создан или изменен)
	//   <0 - в this ничего не изменилось
	//    0 - ошибка
	//
	int    CopyFrom(const SBinarySet & rS, const LongArray & rIdList, bool errOnAbsenseAny);
	//
	// Descr: Возвращает указатель на отрезок данных с идентификатором id. Если такой отрезок найден,
	//   то по указателю pSize (если он не нулевой) присваивается размер отрезка.
	// Note: Функция опасная по 2 причинам:
	//   1. Указатель, возвращенный функцией может стать инвалидным при добавлении нового отрезка в пул.
	//   2. Вызывающая функция потенциально способна навредить целостности пула, так как получает
	//     доступ к его внутренним данным через возвращенный указатель.
	//   В связи со сказанным, в штатном режиме правильнее пользоваться методом Get().
	// Returns:
	//   !0 - отрезок с идентификатором id найден в пуле
	//    0 - идентификатор id не найден, либо нарушена целостность пула.
	//
	const void * GetPtr(uint32 id, uint32 * pSize) const;
	const void * GetPtr() const { return SBaseBuffer::P_Buf; }
	//
	// Descr: Упаковывает экземпляр с целью элминировать пустые отрезки.
	// Returns:
	//   >0 - функция успешно отработала, удалив существовавшие пустые отрезки
	//   <0 - функция отработала успешно, но пустых отрезков не было, потому ничего делать не пришлось
	//    0 - ошибка
	//
	int    Pack();
	//
	// Descr: Низкоуровневая функция, позволяющая перенести данные из внешнего буфера pOuterData в экземпляр this.
	//   Функция копирует эти данные, предварительно очистив собственный буфер а затем проверяет валидность
	//   получившегося экземпляра вызовом IsValid(). Если внешние данные не соответствовали формату SBinarySet,
	//   то функция вернет ошибку (при этом состояние this, предшествовашее вызову функции будет утрачено).
	// Note: Функция очень опасная и создана только для реализации сложных сценарием транспортировки (со сжатием и/или шифрованием).
	//   Если можно ее не использовать - не используйте.
	// Вызов функции с параметрами pOuterData == 0 || outerDataLen == 0 эквивалентен вызову Z().
	// В случае ошибки экземпляр полностью обнуляется (Z()).
	//
	int    SetOuterBuffer(const void * pOuterData, size_t outerDataLen);
private:
	//
	// Descr: Функция одновременно проверяет валидность экземпляра и получает список идентификаторов,
	//   которые в нем хранятся.
	//   Используется для обеих целей.
	//
	bool   FASTCALL Helper_GetChunkIdList(LongArray * pList) const;
	struct H { // Заголовок всего пула
		uint32 Magic;
		uint32 Flags;
	};
	struct BH { // Заголовок элемента данных
		uint32 I;
		uint32 S;
	};
	size_t DataLen;
};
//
//
//
template <size_t BYTESIZE> class TSBinary {
public:
	TSBinary()
	{
		memzero(D, sizeof(D));
	}
	TSBinary(const TSBinary <BYTESIZE> & rS)
	{
		memcpy(D, rS.D, sizeof(D));
	}
	bool   IsZero() const { return ismemzero(D, sizeof(D)); }
	TSBinary <BYTESIZE> & FASTCALL operator = (const TSBinary <BYTESIZE> & rS)
	{
		memcpy(D, rS.D, sizeof(D));
		return *this;
	}
	uint8  D[BYTESIZE];
};
//
// Descr: Идентификация алгоритмов хэширования.
// @persistent
// Данная классификация объединяет все алгоритмы, суть которых заключается в необратимом
// преобразовании данных произвольной длины в значение фиксированной длины.
// Каждый элемент списка гарантированно однозначно соответствует результату с определенной длиной.
// То есть, не должно такого быть, чтобы алгоритм, подпадающий под один из элементов списка мог
// сформировать результат разной длины.
// Note: Не все перечисленные алгоритмы реализованы в рамках данного проекта.
// Attention! Новые алгоритмы добавлять строго в конец списка с инкрементом последнего значения.  
// Note! Константы синхронизированы с таблицей сущностей UED
//
#define SHASHF_CRC8            1
#define SHASHF_CRC16           2
#define SHASHF_CRC24           3
#define SHASHF_CRC32           4
#define SHASHF_CRC64           5
#define SHASHF_CRCCCITT        6
#define SHASHF_CRCDNP          7
#define SHASHF_CRCKERMIT       8
#define SHASHF_CRCSICK         9  // CRC value that is used as dataprotection measure in communications with Sick electronic devices
#define SHASHF_ADLER32        10
#define SHASHF_XX32           11 
#define SHASHF_XX64           12
#define SHASHF_MURMUR2_32     13
#define SHASHF_MURMUR2_64     14
#define SHASHF_MURMUR3_32     15
#define SHASHF_MURMUR3_128X32 16
#define SHASHF_MURMUR3_128X64 17
#define SHASHF_MD2            18
#define SHASHF_MD4            19
#define SHASHF_MD5            20
#define SHASHF_SHA1           21
#define SHASHF_SHA256         22
#define SHASHF_SHA512         23
#define SHASHF_BLAKE_256      24
#define SHASHF_BLAKE_512      25
#define SHASHF_BLAKE2B        26
#define SHASHF_BLAKE2BP       27
#define SHASHF_BLAKE2S        28
#define SHASHF_BLAKE2SP       29
#define SHASHF_BLAKE3         30

typedef TSBinary <16> binary128;
typedef TSBinary <20> binary160;
typedef TSBinary <32> binary256;
typedef TSBinary <64> binary512;
//
// CRC32 functions
//
class SCRC32 {
public:
	SCRC32();
	~SCRC32();
	ulong  Calc(ulong, const void * pData, size_t dataLen);
private:
	int    MakeTab();
	ulong * P_Tab;
};
//
// Descr: Реализации хэш-функций
//
class SlHash {
public:
	static int GetAlgorithmSymb(int ident, SString & rBuf);
	static int IdentifyAlgorithmSymb(const char * pSymb);
	//
	// Descr: Проверяет соответствует ли хэш данных, заданных параметрами {pBuf; bufLen} значению
	//   хэш-функции hashFuncIdent заданному параметрами {pHash, hashLen}.
	// Note: Поддерживаются следующие хэш-функции: SHASHF_SHA1, SHASHF_SHA256, SHASHF_SHA512, SHASHF_MD5, SHASHF_CRC32.
	//   В дальнейшем количество функций увеличится.
	// Returns:
	//   >0 - значение хэш-функции соответствует заданному буферу данных
	//   0  - ошибка (возможно, не допустимые параметры, либо просто значение хэш-функции не соответствует заданному буферу данных).
	//
	static int VerifyBuffer(int hashFuncIdent, const void * pHash, size_t hashLen, const void * pBuf, size_t bufLen);
	static int CalcBufferHash(int hashFuncIdent, const void * pBuf, size_t bufLen, SBinaryChunk & rResult);
	//
	// Descr: Расчитывает хеш блока pData размером len по алгоритму Bob Jenkins
	//
	static uint32 FASTCALL BobJenc(const void * pData, size_t len);
	//
	// Descr: DJB Hash Function
	//   An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the
	//   usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
	//
	static uint32 FASTCALL DJB(const void * pData, size_t len);
	//
	// Descr: RS Hash Function
	//   A simple hash function from Robert Sedgwicks Algorithms in C book.
	//
	static uint32 FASTCALL RS(const void * pData, size_t len);
	//
	// Descr: JS Hash Function
	//   A bitwise hash function written by Justin Sobel
	//
	static uint32 FASTCALL JS(const void * pData, size_t len);
	//
	// Descr: P.J. Weinberger Hash Function
	//   This hash algorithm is based on work by Peter J. Weinberger of AT&T Bell Labs.
	//   The book Compilers (Principles, Techniques and Tools) by Aho, Sethi and Ulman, recommends
	//   the use of hash functions that employ the hashing methodology found in this particular algorithm.
	//
	static uint32 FASTCALL PJW(const void * pData, size_t len);
	//
	// Descr: ELF Hash Function
	//   Similar to the PJW Hash function, but tweaked for 32-bit processors.
	//   Its the hash function widely used on most UNIX systems.
	//
	static uint32 FASTCALL ELF(const void * pData, size_t len);
	//
	// Descr: BKDR Hash Function
	//   This hash function comes from Brian Kernighan and Dennis Ritchie's book "The C Programming Language".
	//   It is a simple hash function using a strange set of possible seeds which all constitute
	//   a pattern of 31....31...31 etc, it seems to be very similar to the DJB hash function.
	//
	static uint32 FASTCALL BKDR(const void * pData, size_t len);
	//
	// Descr: SDBM Hash Function
	//   This is the algorithm of choice which is used in the open source SDBM project.
	//   The hash function seems to have a good over-all distribution for many different data sets.
	//   It seems to work well in situations where there is a high variance in the MSBs of the elements in a data set.
	//
	static uint32 FASTCALL SDBM(const void * pData, size_t len);
	//
	// Descr: DEK Hash Function
	//   An algorithm proposed by Donald E. Knuth in The Art Of Computer Programming Volume 3,
	//   under the topic of sorting and search chapter 6.4.
	//
	static uint32 FASTCALL DEK(const void * pData, size_t len);
	//
	// Descr: BP Hash Function
	//
	static uint32 FASTCALL BP(const void * pData, size_t len);
	//
	// Descr: FNV Hash Function
	//
	static uint32 FASTCALL FNV(const void * pData, size_t len);
	//
	// Descr: End Of AP Hash Function
	//   An algorithm produced by me Arash Partow. I took ideas from all of the above hash functions
	//   making a hybrid rotative and additive hash function algorithm. There isn't any real mathematical
	//   analysis explaining why one should use this hash function instead of the others described above
	//   other than the fact that I tired to resemble the design as close as possible to a simple LFSR.
	//   An empirical result which demonstrated the distributive abilities of the hash algorithm
	//   was obtained using a hash-table with 100003 buckets, hashing The Project Gutenberg Etext
	//   of Webster's Unabridged Dictionary, the longest encountered chain length was 7, the average
	//   chain length was 2, the number of empty buckets was 4579.
	// Note: For uses where high throughput is a requirement for computing hashes using the algorithms
	//   described above, one should consider unrolling the internal loops and adjusting the hash value
	//    memory foot-print to be appropriate for the targeted architecture(s).
	//
	static uint32 FASTCALL AP(const void * pData, size_t len);
	static uint32 FASTCALL XX32(const void * pData, size_t len, uint seed);
	static uint64 FASTCALL XX64(const void * pData, size_t len, uint64 seed);
	static uint32 FASTCALL Murmur2_32(const void * pData, size_t len, uint32 seed);
	static uint64 FASTCALL Murmur2_64(const void * pData, size_t len, uint64 seed);
	static uint32 FASTCALL Murmur3_32(const void * pData, size_t len, uint32 seed);
	static binary128 FASTCALL Murmur3_128x32(const void * pData, size_t len, uint32 seed);
	static binary128 FASTCALL Murmur3_128x64(const void * pData, size_t len, uint32 seed);

	class State {
	public:
		friend class SlHash;
		friend struct ngx_sha1_t__; // @debug

		State();
		~State();
		State & Z();
		uint8  GetResult8() const { return Result.R8; }
		uint16 GetResult16() const { return Result.R16; }
		uint32 GetResult32() const { return Result.R32; }
		uint64 GetResult64() const { return Result.R64; }
	private:
		struct ShaCtx {
			ShaCtx & Z();
			uint32 H[8];
			uint64 Count;
			uint32 Data[16];
			uint   Num;
			uint   MdLen;
		};
		struct Sha512Ctx {
			Sha512Ctx & Z();
			uint64 Count;
			uint32 Num;
			uint64 H[8];
			uint8  Data[128];
		};
		struct Md5Ctx {
			Md5Ctx & Z();
			uint32 H[4];
			uint64 Count; // bytes
			uint32 Data[16];
		};
		union R {
			R();
			uint8  R8;
			uint16 R16;
			uint32 R32;
			uint64 R64;
			ShaCtx Sha;
			Sha512Ctx Sha512; // @v11.2.0
			Md5Ctx Md5; // @v11.0.10
		} Result;
		enum {
			fEmpty = 0x0001
		};
		//#define SHA256_CBLOCK   (SHA_LBLOCK*4) // SHA-256 treats input data as a contiguous array of 32 bit wide big-endian values.

		uint32 Flags;
		static uint8  * P_Tab_Crc8;
		static uint16 * P_Tab_Crc16;
		static uint32 * P_Tab_Crc32;
		static uint64 * P_Tab_Crc64[8];
	};
	//
	// Общий подход к использованию следующих функций:
	//  -- если надо получить хэш по фиксированному буферу за один проход, то вызываем
	//     result = Fun(0, pData, dataSize)
	//  -- если необходимо получить хэш в несколько итераций, то делаем так:
	//     SlHash hs;
	//     while(isThereMoreDate) {
	//       Func(&hs, data_chunk_ptr, data_chunk_size);
	//     }
	//     result = Func(&hs, 0, 0); // final
	//
	static uint8  STDCALL CRC8(State * pS, const void * pData, size_t len);
	static uint32 STDCALL CRC24(State * pS, const void * pData, size_t len);
	static uint32 STDCALL CRC32(State * pS, const void * pData, size_t len);
	static uint64 STDCALL CRC64(State * pS, const void * pData, size_t len);
	static uint32 STDCALL Adler32(State * pS, const void * pData, size_t len);
	static binary128 STDCALL Md5(State * pS, const void * pData, size_t len);
	static binary160 STDCALL Sha1(State * pS, const void * pData, size_t len);
	static binary256 STDCALL Sha256(State * pS, const void * pData, size_t len);
	static binary512 STDCALL Sha512(State * pS, const void * pData, size_t len);
private:
	static const  uchar * Sha1_Body(State::ShaCtx * pCtx, const uchar * data, size_t size);
	static void FASTCALL Md5TransformHelper(State::Md5Ctx * pCtx);
	static void FASTCALL __Sha256TransformHelper(State::ShaCtx * pCtx, const void * pBuffer);
	static void __Sha512TransformHelper(State::Sha512Ctx * pCtx, const void * pBuffer);
};
//
// Descr: Высокоуровневые функции для работы с шифрованием
//
class SSecretTagPool : public SBinarySet {
public:
	enum { // @persistent
		tagSessionPrivateKey =  1,  // Приватный ключ сессии обмена данными
		tagSessionPublicKey  =  2,  // Публичный ключ сессии обмена данными
		tagSessionSecret   =  3,  // Секрет, сгенерированный после обмена публичными ключами с контрагентом
		tagSvcAppellation  =  4,  //
		tagSvcIdent        =  5,  //
		tagSecret          =  7,  //
		tagSvcAccessPoint  =  8,  // URL
		tagSvcCommand      =  9,  // Заголовок сервисной команды
		tagSvcCommandParam = 10,  // Параметры сервисной команды
		tagSvcCapabities   = 11,  // Опции возможностей сервиса
		tagClientIdent     = 12,  // Идентификатор клиента
		tagSelfyFace       = 13,  // Собственный лик (json. see StyloQFace)
		tagSrpVerifierSalt = 14,  //
		tagSrpVerifier     = 15,  //
		tagSrpA            = 16,  // user-->host tagPublicIdent:tagSrpA
		tagSrpB            = 17,  // host-->user tagSrpVerifierSalt:tagSrpB
		tagSrpM            = 18,  // user-->host tagSrpM
		tagSrpHAMK         = 19,  // host-->user tagSrpHAMK
		tagPrimaryRN       = 20,  // Первичное очень большое случайное число, используемое для генерации уникальных для данной инсталляции величин
		tagAG              = 21,  // Дополнительное значение, используемое при генерации публичных идентификаторов
		tagFPI             = 22,  // Фейковый публичный сервисный идентификатор, для генерации собственного публичного идентификатора
		tagRawData         = 23,  // "Сырые" данные, прикрепленные к пакету (для передачи по сети)
		tagReplyStatus     = 24,  // int32   Статус ответа. 0 - OK, !0 - error_code
		tagReplyStatusText = 25,  // zstring Текстовое описание статуса ответа. Если tagReplyStatus == 0, то текста может не быть либо это какое-либо
			// приветствие или иной малозначительный текст.
		tagSessionExpirPeriodSec   = 26,  // uint32  Период истечения срока действия сессии в секундах.
		tagSessionPublicKeyOther   = 27,  // Публичный ключ сессии обмена данными противоположной стороны (в случае, если необходимо хранить
			// в одном пуле и свой и чужой публичные ключи, свой идентифицируется как tagSessionPublicKey, чужой - tagSessionPublicKeyOther
		tagFace            = 28,  // Параметры представления контрагента (json. see StyloQFace). Собственный лик хранится по тегу tagSelfyFace
		tagConfig          = 29,  // json Конфигурация
		tagPrivateConfig   = 30,  // @reserve json Приватная конфигурация //
		tagRoundTripIdent  = 31,  // @v11.1.12 Идентификатор сеанса обмена. Генерируется на стороне инициатора обмена (обычно, клиент)
			// и применяется для сопоставления запросов потокам при асинхронном режиме. Полагаю, в большинстве случаев это будет просто GUID
		tagDocDeclaration  = 32,  // @v11.2.0 JSON-декларация документа, включенного с тегом tagRawData
		tagSvcLoclAddendum = 33,  // @v11.2.3 Дополнительный идентификатор сервиса, определяющий локальный характер обслуживаемого запроса.
			// Участвует в формировании символа очереди в случае MQB-обмена.
		tagAssignedFaceRef = 34,  // @v11.2.4 Ссылка на собственный лик, ассоциированная с записью сервиса. Используется в StyloQ
			// на стороне клиента в записи сервиса для того, чтобы сообщить сервису приемлемый вариант лика.
		tagErrorCode       = 35,  // @v11.2.10 int32 Код ошибки (ответ)
		tagBlob            = 36,  // @v11.3.8  BLOB передаваемый с пакетом. По тегу tagRawData хранятся мета-данные этого blob'а в json-формате

	};
	SSecretTagPool();
	int    GeneratePrivateKey(uint bitCount);
	//
	// Descr: Утилитная функция, извлекающая тег tag из пула и пытающаяся преобразовать его содержимое в JSON.
	// Returns:
	//   0 - ошибка (либо нет такого тега, либо данные в нем не в JSON-формате)
	//  !0 - указатель на JSON. Объект по этому указателю должен быть разрушен вызывающей функцией.
	//
	SJson * GetJson(uint tag) const;
};

class SlCrypto {
public:
	enum {
		algUndef = 0,
		algAes,       // mod:
		algDes,       // mod:
		algDes_Ede,   // mod:
		algDes_Ede3,  // mod:
		algDesx,      // mod:
		algIdea,      // mod:
		algCamellia,  // mod:
		algBlowfish,  // mod: algmodEcb, algmodCbc, algmodOfb, (algmodCfb, algmodCfb64)
		algChaCha20   // mod: nothing|algmodPoly1305
	};
	enum {
		kblUndef = 0,
		kbl128 = 128,
		kbl192 = 192,
		kbl256 = 256
	};
	enum {
		algmodUndef = 0,
		algmodCbc,       // cipher block chaining
		algmodEcb,       // electronic code book
		algmodOcb,
		algmodOfb,       // output feed back
		algmodCcm,
		algmodGcm,
		algmodCtr,       // Counter Mode
		algmodXts,
		algmodWrap,
		algmodWrapPad,
		algmodCfb1,      // cipher feed back
		algmodCfb8,      // cipher feed back
		algmodCfb64,     // cipher feed back
		algmodCfb128,    // cipher feed back
		algmodCfb,       // cipher feed back (default)
		algmodPoly1305   // algChaCha20
	};
	struct CipherProperties {
		CipherProperties();
		uint32 BlockSize;
		uint32 KeySize;
		uint32 IvSize;
		uint32 AadSize;
	};
	class Key {
	public:
		Key();
		Key & Z();
		int    SetKey(const void * pData, size_t size);
		int    SetIV(const void * pData, size_t size);
		int    SetAAD(const void * pData, size_t size);
		const SBaseBuffer & GetKey() const;
		const SBaseBuffer & GetIV() const;
		const SBaseBuffer & GetAAD() const;
	private:
		SBaseBuffer KEY; // don't destroy - it's Ptr refers to Bin
		SBaseBuffer IV;  // don't destroy - it's Ptr refers to Bin
		SBaseBuffer AAD; // don't destroy - it's Ptr refers to Bin
		uint32 EndP;     // Смещение доступного пространства
		uint8  Bin[256];
	};
	SlCrypto(int alg, uint kbl, int algModif);
	~SlCrypto();
	const  CipherProperties & GetCipherProperties() const;
	int    SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen);
	int    SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen);
	int    SetupKey(SlCrypto::Key & rK, const void * pKey, size_t keyByteLen);
	int    SetupKey(SlCrypto::Key & rK, const char * pPassword);
	//int    SetupEncrypt(const char * pPassword);
	//int    SetupEncrypt(const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen);
	//int    SetupDecrypt(const char * pPassword);
	//int    SetupDecrypt(const void * pKey, size_t keyByteLen, const void * pIv, size_t ivLen, const void * pAad, size_t aadLen);
	//int    Encrypt(const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen);
	//int    Decrypt(const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen);
	int    Encrypt_(const SlCrypto::Key * pKey, const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen);
	int    Decrypt_(const SlCrypto::Key * pKey, const void * pData, size_t dataLen, void * pResult, size_t resultBufSize, size_t * pActualResultLen);
	int    Encrypt(const SlCrypto::Key & rKey, const void * pData, size_t dataLen, SBinaryChunk & rResult, SBinaryChunk * pTag);
	//
	// Note: параметр pTag - фактически константный, однако из-за обращения к интерфейсной функции пришлось сделать ее non-const.
	//
	int    Decrypt(const SlCrypto::Key & rKey, const void * pData, size_t dataLen, SBinaryChunk & rResult, SBinaryChunk * pTag);
private:
	enum {
		stError    = 0x0001,
		stInitEncr = 0x0002,
		stInitDecr = 0x0004
	};
	void * P_Ctx;
	const void * P_Cphr;
	const int  Alg;
	const uint KeyBitLen;
	const uint AlgModif;
	CipherProperties Cp;
	//uint8 Key_[64];
	//uint8 IV_[16];
	uint  State;
};

struct NGConstant;
#ifndef SHA512_DIGEST_LENGTH
	#define SHA512_DIGEST_LENGTH 64
#endif

class SlSRP {
public:
	enum HashAlgorithm {
		SRP_SHA1,
		SRP_SHA224,
		SRP_SHA256,
		SRP_SHA384,
		SRP_SHA512
	};
	enum NGType {
		SRP_NG_1024,
		SRP_NG_2048,
		SRP_NG_4096,
		SRP_NG_8192,
		SRP_NG_CUSTOM
	};

	class VerifierBase {
	public:
		int    IsAuthenticated() const { return Authenticated; }
	protected:
		explicit VerifierBase(HashAlgorithm alg);
		~VerifierBase();
		HashAlgorithm HashAlg;
		NGConstant * P_Ng;
		int    Authenticated;
		uchar  M[SHA512_DIGEST_LENGTH];
		uchar  H_AMK[SHA512_DIGEST_LENGTH];
		uchar  SessionKey[SHA512_DIGEST_LENGTH];
	};

	class Verifier : public VerifierBase {
	public:
		//
		// Out: bytes_B, len_B.
		//
		// On failure, bytes_B will be set to NULL and len_B will be set to 0
		//
		// The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type
		//
		Verifier(HashAlgorithm alg, NGType ngType, const char * pUserName,
			const SBinaryChunk & rS, const SBinaryChunk & rV, const SBinaryChunk & rA, SBinaryChunk & rB, const char * pNHex, const char * pGHex);
		~Verifier();
		const char * GetName() const;
		// user_M must be exactly SrpVerifier::GetSessionKeyLength() bytes in size
		void   VerifySession(const uchar * pUserM, const uchar ** ppBytesHAMK);
		const  uchar * GetSessionKey(int * pKeyLength) const;
		int    GetSessionKeyLength() const;
	private:
		SString UserName;
		SBinaryChunk BytesB;
	};

	class User : public VerifierBase {
	public:
		//
		// The n_hex and g_hex parameters should be 0 unless SRP_NG_CUSTOM is used for ng_type
		//
		User(HashAlgorithm alg, NGType ng_type, const char * username, const void * pPassword, int passwordLen, const char * pNHex, const char * pGHex);
		~User();
		//int    IsAuthenticated() const { return Authenticated; }
		const char * GetName() const;
		int    GetSessionKeyLength() const;
		const  uchar * GetSessionKey(int * pKeyLength) const;
		// bytes_HAMK must be exactly SrpUser::GetSessionKey() bytes in size
		void   VerifySession(const uchar * bytes_HAMK);
		// Output: username, bytes_A, len_A
		//void   StartAuthentication(char ** ppUserName, uchar ** ppBytesA, int * pLenA);
		void   StartAuthentication(char ** ppUserName, SBinaryChunk & rA);
		//void   ProcessChallenge(const uchar * pBytesS, int lenS, const uchar * pBytesB, int lenB, const uchar ** ppBytesM, int * pLenM);
		void   ProcessChallenge(const SBinaryChunk & rS, const SBinaryChunk & rB, SBinaryChunk & rM);
	private:
		/*BIGNUM*/void * P_a;
		/*BIGNUM*/void * P_A;
		/*BIGNUM*/void * P_S;
		//uchar * P_BytesA;
		SBinaryChunk BytesA;
		char * P_UserName;
		uchar * P_Password;
		int    PasswordLen;
	};

	//static void CreateSaltedVerificationKey(HashAlgorithm alg, NGType ng_type, const char * pUserName,
		//const uchar * pPassword, int lenPassword, uchar ** ppBytesS, int * len_s, uchar ** ppBytesV, int * len_v, const char * n_hex, const char * g_hex);
	static int  CreateSaltedVerificationKey2(HashAlgorithm alg, NGType ng_type, const char * pUserName,
		const uchar * pPassword, int lenPassword, SBinaryChunk & rS, SBinaryChunk & rV, const char * n_hex, const char * g_hex);
	//
	// This library will automatically seed the OpenSSL random number generator
	// using cryptographically sound random data on Windows & Linux. If this is
	// undesirable behavior or the host OS does not provide a /dev/urandom file,
	// this function may be called to seed the random number generator with
	// alternate data.
	//
	// The random data should include at least as many bits of entropy as the
	// largest hash function used by the application. So, for example, if a
	// 512-bit hash function is used, the random data requies at least 512
	// bits of entropy.
	//
	// Passing a null pointer to this function will cause this library to skip
	// seeding the random number generator. This is only legitimate if it is
	// absolutely known that the OpenSSL random number generator has already
	// been sufficiently seeded within the running application.
	//
	// Notes: This function is optional on Windows & Linux and mandatory on all other platforms.
	//
	static void RandomSeed(const uchar * pRandomData, int dataLength);
};
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
int    SCalcCheckDigit(int alg, const char * pInput, size_t inputLen);
char * FASTCALL SUpceToUpca(const char * pUpce, char * pUpca);
int    FASTCALL SCalcBarcodeCheckDigitL(const char * pBarcode, size_t len);

#endif // } _WIN32_WCE

void IdeaEncrypt(const char * password, void * buf, size_t len);
void IdeaDecrypt(const char * password, void * buf, size_t len);
// @v11.1.1 (replaced with SObfuscateBuffer()) void FASTCALL IdeaRandMem(void * buf, size_t len);
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
double STDCALL  round(double n, double prec, int dir);
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
double FASTCALL roundnev(double n, int prec);
double trunc(double n, int prec);
int    GetRandom(int min, int max);
//
// Int Money
// Операции с представлением числа с фиксированной точкой.
//
double FASTCALL intmnytodbl(long m);
long   FASTCALL dbltointmny(double r);
double FASTCALL inttodbl2(long v);
long   FASTCALL dbltoint2(double r);
//
// Bit string's operations
//
void   FASTCALL resetbitstring(void * pBuf, size_t len);
void   STDCALL  setbit32(void * pBuf, size_t len, size_t pos);
void   STDCALL  resetbit32(void * pBuf, size_t len, size_t pos);
int    STDCALL  getbit8(const void * pBuf, size_t len, size_t pos);
int    STDCALL  getbit32(const void * pBuf, size_t len, size_t pos);
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
int    STDCALL findbit(const void * pBuf, size_t count, int val, size_t * pPos);
void   insbit(void * pBuf, size_t len, size_t);
void   delbit(void * pBuf, size_t len, size_t);
uint8  FASTCALL bitscanforward(uint32 * pIdx, uint32 mask);
uint8  FASTCALL bitscanreverse(uint32 * pIdx, uint32 mask);
//
// Encrypt / Decrypt functions
//
void * encrypt(void *, size_t);
void * decrypt(void *, size_t);
ulong  _checksum__(const char *, size_t);
//
// MIME64 encoder/decoder
//
int    STDCALL  encode64(const char * pIn, size_t inLen, char * pOut, size_t outMax, size_t * pOutLen);
int    STDCALL  decode64(const char * pIn, size_t inLen, char * pOut, size_t * pOutLen);

class MIME64 {
public:
	MIME64();
	int    Encode(const void * pIn, size_t inLen, char * pOut, size_t outBufLen, size_t * pOutDataLen) const;
	int    Decode(const char * pIn, size_t inLen, char * pOut, size_t * pOutDataLen) const;
};
//
// Descr: Кодирует бинарную последовательность pData длиной dataLen в строку rBuf
//   в соответствии с base32-кодировкой.
//   Результирующий буфер rBuf будет содержать только символы a..z 2..7
//   В прологе функции rBuf очищается.
//
int    Base32_Encode(const uint8 * pData, size_t dataLen, SString & rBuf);
//
// Descr: Декодирует строку pCoded, содержащую данные в кодировке base32.
//   Декодирование заканчивается когда встречается недопустимый для этого типа кодировки.
//   Таким образом, строка с правильной кодировкой обязательно должна заканчиваться нулем.
//   Результат сбрасывается в буфер rResult. В прологе функции rResult очищается.
//
size_t Base32_Decode(const uchar * pCoded, SBinaryChunk & rResult);

size_t Hex_Encode(uint flags, const uint8 * pData, size_t dataLen, SString & rBuf);
//
// Descr: Декодирует строковое шестнадцатиричное представление бинарных данных
//   в буфер rResult.
// Note: Функция низкоуровневая - она не проверяет входные параметры.
// ARG(pCoded IN): Указатель на текстовую строку содержащую hex-кодированные данные
// ARG(srcLen IN): Длина данный во входящей строке pCoded. Важно: это - количество байт, а не байтовых пар.
// ARG(rResult OUT): Бинарный буфер, в который заносятся декодированные данные. Функция предварительно очищает буфер.
// Returns:
//   Количество декодированных байт.
//
size_t Hex_Decode(const uchar * pCoded, size_t srcLen, SBinaryChunk & rResult);
//
// Punycode
//
int SPunycodeEncode(const uint * input, size_t input_length, SString & rOut, const uchar * pCaseFlags);
int SPunycodeDecode(const char * input, size_t input_length, size_t * output_length, uint * output, uchar * pCaseFlags);
//
//
int    EquipInputDecode(const void * pInput, size_t inputSize, int binaryFormat, int outpFormat, SString & rBuf); // @construction
// @v10.3.3 @unused int checkabort();
// @v10.3.3 @unused int enableabort(int); // 0 - disable, 1 - enable, (-1) - check status
//
//
//
enum {
	comdvcsCom = 1,
	comdvcsLpt,
	comdvcsPrn,
	comdvcsCon
};

int    FASTCALL IsComDvcSymb(const char * pSymb, int * pCount);
//
// Descr: Формирует строку вида COM1, LPT2, CON и т.д. в зависимости от параметров
//   comdvcs, count. Если (option & 0x0001), тогда впереди строки добавляются символы \\.\
//   необходимые для открытия порта в Win32.
//
SString & GetComDvcSymb(int comdvcs, int count, int option, SString & rBuf);
int GetPort(const char * pPortName, int * pPortNo);
//
// RS232 Communication
//
enum ComBaudRate {
	cbrUndefined = 0,
	cbr110,   // =1
	cbr300,   // =2
	cbr600,   // =3
	cbr1200,  // =4
	cbr2400,  // =5
	cbr4800,  // =6
	cbr9600,  // =7
	cbr14400, // =8
	cbr19200, // =9
	cbr38400, // =10
	cbr56000,
	cbr57600,
	cbr115200,
	cbr128000,
	cbr256000
};

struct CommPortParams {    // @persistent size=6
	CommPortParams();
	int16  Cbr;            // ComBaudRate
	int8   ByteSize;       // [4..8]
	int8   Parity;         // [0..4] (no, odd, even, mark, space)
	int8   StopBits;       // 0, 1, 2 (1, 1.5, 2)
	uint8  Reserve;        // @reserve
};

struct CommPortTimeouts {
	uint16 Get_NumTries; /* (0..32000)      */
	uint16 Get_Delay; /* (0..1000)       */
	uint16 Put_NumTries; /* (0..32000)      */
	uint16 Put_Delay; /* (0..1000)       */

	uint16 Reserve1;
	uint16 W_Get_Delay; /* (0..1000) Win32 */
	uint16 Reserve2[2];
};

class SCommPort {
public:
	SCommPort();
	~SCommPort();
	void   FASTCALL GetParams(CommPortParams *) const;
	void   FASTCALL GetTimeouts(CommPortTimeouts *) const;
	void   FASTCALL SetParams(const CommPortParams *);
	void   FASTCALL SetTimeouts(const CommPortTimeouts *);
	int    InitPort(int portNo, int ctsControl, int rtsControl);
	int    ClosePort();
	int    FASTCALL PutChr(int c);
	int    GetChr();
	int    FASTCALL GetChr(int * pChr);
	void   SetReadCyclingParams(int cycleCount, int cycleDelay);
	int    GetPortNo() const { return PortNo; }
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
// Descr: Структура и 2 функции для извлечения значений из статических таблиц,
//   элементами которых являются пары {int, const char *}.
//   Это - очень популярная конструкция, потому данный блок (структура SIntToSymbTabEntry вкупе с
//   с функциями SIntToSymbTab_GetSymb и SIntToSymbTab_GetId) призван унифицировать ее применение.
//
struct SIntToSymbTabEntry {
	int    Id;
	const  char * P_Symb;
};
//
// Descr: Ищет в таблице pTab (размером tabSize элементов) первый элемент, имеющий значение
//   (Id == id) и в буфере rSymb возвращает значение соответствующего символа (SIntToSymbTabEntry::P_Symb).
// Returns:
//   >0 - искомый элемент найден
//   0  - искомый элемент не найден (к аргументу rSymb применен метод SString::Z())
//
int    STDCALL SIntToSymbTab_GetSymb(const SIntToSymbTabEntry * pTab, size_t tabSize, int id, SString & rSymb);
//
// Descr: Ищет в таблице pTab (размером tabSize элементов) первый элемент, имеющий значение
//   (Id == id) и возвращает указатель на соответствующий символ (SIntToSymbTabEntry::P_Symb).
// Returns:
//   !0 - указатель на символ, соответствующий идентификатору id
//   0  - искомый элемент не найден либо заданному идентификатору соответствует нулевая строка.
//
const char * STDCALL SIntToSymbTab_GetSymbPtr(const SIntToSymbTabEntry * pTab, size_t tabSize, int id);
//
// Descr: Возвращает true если таблица pTab содержит элемент с полем Id == id. В противном случае - false.
//
bool   STDCALL SIntToSymbTab_HasId(const SIntToSymbTabEntry * pTab, size_t tabSize, int id);
//
// Descr: Ищет в таблице pTab (размером tabSize элементов) первый элемент, имеющий значение
//   P_Symb равное параметру pSymb (сравнение осуществляется функцией sstreqi_ascii).
// Note: Чтобы избежать неоднозначности в работе функции таблица pTab не должна содержать
//   элементов, у которых Id == 0.
// Returns:
//   Если искомый символ найден, то возвращает соответствующее значение SIntToSymbTabEntry::Id,
//   в противном случае 0.
//   Если pSymb == 0 || pSymb[0] == 0, то возвращает 0.
//
int   STDCALL SIntToSymbTab_GetId(const SIntToSymbTabEntry * pTab, size_t tabSize, const char * pSymb);
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
	aryRing = 0x20,
	aryMorePerform  = 0x40  // @v9.1.1 @construction Поведенческий флаг: функции массива принимают меры для увеличения
		// производительности за счет большего объема используемой памяти
};
//
// Descr: Базовый класс для векторов, коллекций и иных структур данных.
//   Реализует минимальный набор функций, которые можно выполнить обходясь
//   без виртуальных методов и знания природы элементов.
// Note: Этот класс не умеет правильно удалять сложные элементы (для этого
//   унаследованный SArray имеет виртуальный метод freeItem(void *).
//   Другими словами, флаг aryEachItem этим объектом не обслуживается.
//
class SVectorBase { // @novtbl
public:
	//
	// Descr: Утилитная функция для замещения популярной конструкции (pV ? pV->getCount() : 0)
	//
	static uint FASTCALL GetCount(const SVectorBase * pB);

	int    operator ! () const { return (count == 0); }
	uint   getCount() const { return count; }
	int    getCountI() const { return static_cast<int>(count); }
	int    getEnd() const { return static_cast<int>(count-1); }
	uint   getItemSize() const { return isize; }
	uint   getFlags() const;
	void   setFlag(uint val, int set);
	//
	// Descr: Возвращает значение внутреннего указателя позиции
	//
	uint   getPointer() const { return pointer; }
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
	uint   incPointer();
	//
	// Descr: Изменяет значение внутреннего указателя позиции на величину incr.
	//   При этом не выходя за границы диапазона [0..getCount()-1].
	//   Если incr < 0, то указатель уменьшается, в противном случае - увеличивается.
	// Returns:
	//   Предшествующее изменению значение позиции
	//
	uint   FASTCALL incPointerSafe(int incr);
	void * dataPtr() const;
	void * FASTCALL at(uint) const;
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
	void * next();
	int    STDCALL  enumItems(uint * pIdx, void ** ppItem) const;
	void * atPut(uint, const void *);
	int    STDCALL  atInsert(uint, const void *);
	int    FASTCALL insert(const void * pItem);
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
	int    insertChunk(uint numItemsToInsert, const void * pItems);
	//
	// Descr: меняет местами элементы с индексами p1 и p2.
	// Returns:
	//   1 - обмен прошел успешно
	//   0 - один из индексов выходит за пределы размеров массива
	//
	int    swap(uint p1, uint p2);
	//
	// Descr: меняет порядок следования элементов массива [pos..pos+numItems-1].
	//   То есть:
	//      первый элемент становится последним, последний - первым
	//      второй элемент становится предпоследним, предпоследний - вторым и т.д.
	//
	void   reverse(uint pos, uint numItems);
	//
	// Descr: Сжимает объем используемой вектором памяти до ALIGNSIZE(count, 4).
	//   Если count == 0, то просто освобождает занимаемую динамическую память.
	//   Если Limit <= (1.2 * count) то ничего не делает.
	// Returns:
	//   >0 - память была успешно освобождена либо сжата и данные перенесены.
	//   <0 - ничего не было сделано по причине отсутствия значимого избытка занимаемой памяти
	//    0 - ошибка (скорее всего не удалось выделить новый буфер для переноса данных).
	//
	int    Shrink();
	//
	// Descr: Перемешивает элементы массива случайным образом
	//
	int    shuffle();
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
	int    moveItem(uint pos, int dir, uint * pNewPos); // @>>SArray::swap
	int    ordInsert(const void *, uint *, CompFunc, void * pExtraData = 0);
	//
	// Descr: осуществляет бинарный поиск по всем записям массива.
	//   Каждая запись просматривается со смещения ofs.
	//
	int    imp_bsearch(const void *, uint *, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: осуществляет последовательный поиск по
	//   всем записям массива, начиная с записи *pos. Каждая запись
	//   просматривается со смещения ofs.
	//
	int    imp_lsearch(const void *, uint * pos, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	bool   bsearch(const void * key, uint * pIdx, CompFunc, size_t ofs, void * pExtraData) const;
	bool   bsearch(const void * key, uint * pIdx, CompFunc fcmp) const;
	bool   lsearch(const void * key, uint * pIdx, CompFunc, size_t ofs = 0, void * pExtraData = 0) const;
	void   sort(CompFunc fcmp, void * pExtraData = 0);
	void   sort2(CompFunc fcmp, void * pExtraData = 0);
	double sumDouble(uint offs, uint firstIdx, uint lastIdx) const;
protected:
	SVectorBase(uint itemSize, uint o);
	~SVectorBase();
	//
	// Descr: Базовая функция копирования массива.
	// Note: Как упоминалось, копируются только собственно данные, хранящиеся в массиве.
	//   Глубокое копирование выполнено быть не может по определению.
	//   //
	//   Функция предполагает, что перед ее вызовом была осуществлена
	//   предварительная очистка this. Сама она этого не далает по то той же причине
	//   незнания природы хранящихся в ней данных.
	//
	int    FASTCALL CopyS(const SVectorBase & rS);
	int    FASTCALL expand(uint numNewRecs);
	int    Helper_Cmp(const void * i1, const void * i2, CompFunc fcmp, void * pExtraData) const;
	bool   isItemEq(const void * pKey, uint pos, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const;

	uint   count;   // Количество элементов, находящихся в массиве
	uint   pointer; // Указатель на текущий элемент массива (используется в стеках и очередях)
	uint   isize;   // Размер одного элемента (bytes)
	uint   Limit;   // Максимальное количество элементов (not bytes), которое может вместить буфер
	uint   VFlags;
	uint8 * P_VData;   //
};
//
// @v10.7.9
// Descr: Макрос, обеспечивающий короткую запись цикла по всем элементам.
// Note: Я скептически отношусь к такого рода макросам (равно как и к новым с++-итераторам) но ради эксперимента все же решил сделать.
//
#define SForEachVectorItem(__vec, __iteridx) for(uint (__iteridx) = 0; (__iteridx) < (__vec).getCount(); (__iteridx)++)
//
// Descr: Массив с "плоскими" элементами. Не имеет механизма индивидуального
//   разрушения элементов (в отличии от SArray, который имеет виртуальную функцию freeItem()).
//
class SVector : public SVectorBase { // @novtbl
public:
	SVector(uint itemSize, uint o /*= O_ARRAY*/);
	explicit SVector(uint itemSize);
	SVector(void * pData, uint itemSize, uint aCount, uint o = O_N_O_ARRAY);
	SVector(const SVector & rS);
	~SVector();
	SVector & FASTCALL operator = (const SVector &);
	int    FASTCALL copy(const SVector & rS);
	//
	// Descr: Побайтно сравнивает массив this с массивом src.
	//   Если this->getCount() != src.getCount || this->getItemSize() != src.getItemSize(),
	//   то считает, что объекты не равны. Если эта проверка успешна, то сравнивает
	//   побайтно все элементы массивов.
	//
	bool   FASTCALL IsEq(const SVector & src) const;
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
	int    freeLast();
	//
	// Descr: удаляет все элементы массива и освобождает занятую под них память.
	//
	void   freeAll();
	//
	// Descr: удаляет все элементы массива не освобождая при этом память, выделенную под data
	//
	void   clear();
	//
	// Descr: Удаляет из массива диапазон элементов, начинающийся с позиции lowPos и до uppPos включительно.
	//   Если lowPos > uppPos или lowPos >= getCount(), то не делает ничего
	//   Если lowPos == uppPos, то вызов эквивалентен freeItem(lowPos)
	//   Если lowPos < getCount(), то удаляет все элементы от позиции lowPos до MIN(uppPos, getCount()-1)
	// Note: Главное назначение функции - ускорить массовое удаление за счет сокращения издержек
	//   на перемещение участков памяти при серии единичных удалений.
	//
	int    freeChunk(uint lowPos, uint uppPos);
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
	int    Write(SFile & rFile, long) const;
	//
	// Descr: Функция считывает массив из бинарного файла rFile. Формат считывания //
	//   соответствует тому, что определен для функции SArray::Write(SFile &, long).
	// Note: Если размер записи в файле не соответсвует размеру записи this->isize,
	//   то функция возвращает ошибку.
	//   Перед считыванием массив this очищается.
	//   Этой функцией не следует подменять унаследованныю функцию ReadSArrayFromFile(SArray *, FILE *)
	//   поскольку формат чтения этих функций различается.
	//
	int    Read(SFile & rFile, long);
};

template <class T> class TSVector : public SVector {
public:
	TSVector() : SVector(sizeof(T)) {}
	TSVector(const TSVector & src) : SVector(src) {}
	T &    FASTCALL at(uint pos) const { return *reinterpret_cast<T *>(SVector::at(pos)); }
};
//
// SArray
//
// @attention Функции вставки и удаления элементов массива могут изменить местоположение данных в памяти.
// Если вы используете переменную, дублирующую адрес SArray::data, то ее следует актуализировать вызовом
// функции SArray::dataPtr()
//
class SArray : public SVectorBase {
public:
	SArray(uint itemSize, uint o /*= O_ARRAY*/);
	//
	// Descr: Создает простой массив (O_ARRAY) со значением delta по умолчанию (8)
	//   и размером элемента - itemSize.
	//
	explicit SArray(uint itemSize);
	SArray(void * pData, uint itemSize, uint aCount, uint o = O_N_O_ARRAY);
	SArray(const SArray & rS);
	SArray(const SVector & rS);
	virtual ~SArray();
	SArray & FASTCALL operator = (const SArray &);
	int    FASTCALL copy(const SArray & src);
	//
	// Descr: Побайтно сравнивает массив this с массивом src.
	//   Если this->getCount() != src.getCount || this->getItemSize() != src.getItemSize(),
	//   то считает, что объекты не равны. Если эта проверка успешна, то сравнивает
	//   побайтно все элементы массивов.
	//   Для массивов с признаками aryPtrContainer и aryEachItem возвращает 0 (считает, что
	//   невозможно корректно побайтно сравнить такие объекты).
	//
	bool   FASTCALL IsEq(const SArray & src) const;
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
	int    freeLast();
	//
	// Descr: удаляет все элементы массива и освобождает занятую под них память.
	//
	void   freeAll();
	//
	// Descr: удаляет все элементы массива не освобождая при этом память, выделенную под data
	//   ("глубокие" данные, разрушение которых определено функцией freeItem, освобождаются).
	//
	void   clear();
	//
	// Descr: Функция должна разрушить объект, по указателю p.
	//   Эта функция вызывается при разрушении массива или удалении его элемента в случае,
	//   если (flags & aryEachItem)
	//
	virtual	void FASTCALL freeItem(void * p);
	//
	// Descr: Удаляет из массива диапазон элементов, начинающийся с позиции lowPos и до uppPos включетельно.
	//   Если lowPos > uppPos или lowPos >= getCount(), то не делает ничего
	//   Если lowPos == uppPod, то вызов эквивалентен freeItem(lowPos)
	//   Если lowPos < getCount(), то удаляет все элементы от позиции lowPos до MIN(uppPos, getCount()-1)
	// Note: Главное назначение функции - ускорить массовое удаление за счет сокращения издержек
	//   на перемещение участков памяти при серии единичных удалений.
	//
	int    freeChunk(uint lowPos, uint uppPos);
#ifndef _WIN32_WCE // {
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
	int    Write(SFile & rFile, long) const;
	//
	// Descr: Функция считывает массив из бинарного файла rFile. Формат считывания //
	//   соответствует тому, что определен для функции SArray::Write(SFile &, long).
	// Note: Если размер записи в файле не соответсвует размеру записи this->isize,
	//   то функция возвращает ошибку.
	//   Перед считыванием массив this очищается.
	//   Этой функцией не следует подменять унаследованныю функцию ReadSArrayFromFile(SArray *, FILE *)
	//   поскольку формат чтения этих функций различается.
	//
	int    Read(SFile & rFile, long);
#endif // } !_WIN32_WCE
};
//
// @v9.7.11 Все 5 методов элиминированы (фактически, перенесены как static в dl200.cpp поскольку только там и используются).
// Streaming functions
//   { (uint16)strlen(pStr), pStr[] }
//   if(pStr == 0) then { (uint16)0 }
//
// { (uint16)count; data[count*isize] }
//
//int    WriteSArrayToFile(const SArray * pAry, FILE * pStream);
//int    ReadSArrayFromFile(SArray * pAry, FILE * pStream);
//int WritePStrToFile(const char * pStr, FILE * pStream);
//int ReadPStrFromFile(char ** ppStr, FILE * pStream);
//int ReadPStrFromFile(SString & rStr, FILE * pStream);
//
// SCollection
//
class SCollection : public SArray {
public:
	SCollection();
	explicit SCollection(uint o /*= O_COLLECTION*/);
	SCollection(const SCollection & src) : SArray(src) {}
protected:
	int    STDCALL  Helper_CreateNewItem(void * pNewItem, uint * pPos);
	int    FASTCALL Helper_CreateNewItem(void * pNewItem);
};
//
// SStack
//
class SStack : public SArray {
public:
	explicit SStack(uint itemSize, uint o = O_ARRAY);
	// @v11.1.12 int    FASTCALL push(const void *);
	void * FASTCALL push(const void *); // @v11.1.12 returns result item's ptr now
	int    FASTCALL pop(void *);
	void * pop();
	int    undo(uint savedPos);
	void * peek() const;
};

class SQueue : public SArray {
public:
	SQueue(uint itemSize, uint queueSize, uint o = O_ARRAY);
	SQueue(const SQueue &);
	void   clear();
	int    FASTCALL copy(const SQueue & src);
	int    FASTCALL push(const void *);
	void * pop();
	void * peek() const;
	uint   size() const { return QueueSize; }
	uint   getNumItems() const;
	void * get(uint) const;
private:
	uint   QueueSize;
	uint   Front;
	uint   End;
};

class STypArray : public SArray {
public:
	STypArray(TYPEID, /*uint aDelta = 8,*/ uint o /*= O_ARRAY*/);
	virtual	void FASTCALL freeItem(void *);
	TYPEID getType() const { return type; }
	int    search(const void *, uint *) const;
	void   sort();
	SString & itemToString(uint idx, long fmt, SString &) const;
protected:
	TYPEID type;
};

template <class T> class TSArray : public SArray {
public:
	explicit TSArray(uint o = O_ARRAY) : SArray(sizeof(T), /*8,*/ o) {}
	TSArray(const TSArray & src) : SArray(src) {}
	T &    FASTCALL at(uint pos) const { return *reinterpret_cast<T *>(SArray::at(pos)); }
};

template <class T> class TSCollection : public SCollection {
public:
	TSCollection() : SCollection(/*DEFCOLLECTDELTA, O_COLLECTION*/)
	{
	}
	~TSCollection()
	{
		freeAll();
	}
	T *  FASTCALL at(uint pos) const
	{
		return static_cast<T *>(SCollection::at(pos));
	}
	virtual void FASTCALL freeItem(void * pItem)
	{
		delete reinterpret_cast<T *>(pItem);
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
		if(Helper_CreateNewItem(p_new_item, pPosition))
			return p_new_item;
		else {
			delete p_new_item;
			return 0;
		}
	}
	T * FASTCALL CreateNewItem()
	{
		T * p_new_item = new T;
		if(Helper_CreateNewItem(p_new_item))
			return p_new_item;
		else {
			delete p_new_item;
			return 0;
		}
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
			ok = 0; //SLS.SetError(SLERR_NOMEM);
	}
	return ok;
}

template <class T> class TSStack : public SStack {
public:
	TSStack() : SStack(sizeof(T), O_ARRAY) {}
	void   FASTCALL push(const T & t) { SStack::push(&t); }
	int    FASTCALL pop(T & t) { return SStack::pop(&t); }
	T &    FASTCALL peek() const { return *static_cast<T *>(SStack::peek()); }
};

template <class T> class TSRingStack : public SStack {
public:
	explicit TSRingStack(uint cycle) : SStack(sizeof(T), /*aDelta,*/O_ARRAY|aryRing)
	{
		assert(cycle > 1);
		expand(cycle);
	}
	T &    FASTCALL push(const T & t) { return *static_cast<T *>(SStack::push(&t)); }
	int    FASTCALL pop(T & t) { return SStack::pop(&t); }
	T &    FASTCALL peek() const { return *static_cast<T *>(SStack::peek()); }
};
//
//
//
template <class T> class TSQueue : public SQueue {
public:
	explicit TSQueue(uint queueSize) : SQueue(sizeof(T), queueSize) {}
	TSQueue(const TSQueue & src) : SQueue(src) {}
	int    FASTCALL push(const T & t) { return SQueue::push(&t); }
	T &    FASTCALL pop() { return *static_cast<T *>(SQueue::pop()); }
	T &    FASTCALL peek() const { return *static_cast<T *>(SQueue::peek()); }
	T &    FASTCALL get(uint p) const { return *static_cast<T *>(SQueue::get(p)); }
};
//
// Descr: Шаблон для револьверных структур, обеспечивающих повторное использование
//   созданных и инициализированных ранее экземпляров объектов (SString, SStringU etc)
//
template <class T> class TSRevolver {
public:
	explicit TSRevolver(uint c) : Count(c), P_List(new T[c]), P(0), AccsCount(0)
	{
	}
	~TSRevolver()
	{
		delete [] P_List;
	}
protected:
	T & Implement_Get()
	{
        assert(P <= Count);
        if(P == Count)
			P = 0;
		AccsCount++;
		return P_List[P++];
	}
//private:
	uint   P;
	T    * P_List;
	const uint Count;
	uint64 AccsCount; // Общее количество обращений
};

class SRevolver_SString : public TSRevolver <SString> {
public:
	explicit SRevolver_SString(uint c);
	SString & Get();
	void   Saturate(uint minSize); // @debug
};

class SRevolver_SStringU : public TSRevolver <SStringU> {
public:
	explicit SRevolver_SStringU(uint c);
	SStringU & Get();
	void   Saturate(uint minSize); // @debug
};

DECL_CMPFUNC(PcharNoCase);
DECL_CMPFUNC(_PcharNoCase);
//
// Descr: Сравнивает файловые пути i1 и i2 заданные в кодировке utf-8. Перед сравнением строки унифицируются.
//
IMPL_CMPFUNC(FilePathUtf8, i1, i2);
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

class RealArray : public TSVector <double> {
public:
	RealArray();
	RealArray(const RealArray & s);
	RealArray & FASTCALL operator = (const RealArray & s);
	double & FASTCALL operator [] (size_t i) const;
	RealArray & Z();
	int    add(double v);
	//
	// Descr: Распределяет память ровно на numItems элементов.
	//   Все элементы обнуляются.
	//
	int    FASTCALL dim(uint numItems);
	double Sum() const;
	//
	// Descr: Вычисляет разницу между элементами [idx] и [idx-stride]
	//
	double StrideDifference(uint idx, uint stride) const;
	void   Sort();
};
//
//
//
class FloatArray : public TSVector <float> {
public:
	FloatArray();
	bool   FASTCALL operator == (const FloatArray & rS) const;
	bool   FASTCALL operator != (const FloatArray & rS) const;
	bool   FASTCALL IsEq(const FloatArray *) const;
	int    FASTCALL add(float);
	int    FASTCALL add(SPoint2F);
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
	const  SPoint2F & FASTCALL getPoint(uint i) const;
	float & FASTCALL operator [] (size_t i) const;
};
//
//
//
class LongArray : public TSVector <long> {
public:
	LongArray();
	LongArray(const long * pInitList, size_t initListCount); // @v11.6.2
	LongArray(const LongArray & s);
	LongArray & FASTCALL operator = (const LongArray & s);
	bool   FASTCALL operator == (const LongArray & rS) const;
	bool   FASTCALL operator != (const LongArray & rS) const;
	int    ordInsert(long, uint * pPos);
	bool   STDCALL  bsearch(long, uint * pPos) const;
	bool   STDCALL  lsearch(long, uint * pPos) const;
	bool   FASTCALL bsearch(long) const;
	bool   FASTCALL lsearch(long) const;
	//
	// Descr: Возвращает true если количество элементом массива более 1.
	//
	bool   isList() const;
	bool   FASTCALL IsEq(const LongArray *) const;
	long   FASTCALL get(uint) const;
	long   getSingle() const;
	long   getLast() const;
	int    FASTCALL setSingleNZ(long id);
	//
	// Descr: Функционально аналогична SVector::clear() и отличается только
	//   возвратом *this.
	//   Реализована для семантической идентичности с SString::Z()
	//
	LongArray & Z();
	int    freeByKey(long, int binarySearch); //
	int    FASTCALL removeByID(long);                  // @warning dup function (see above)
	void   sort();
	void   sortAndUndup();
	//
	// Descr: Возвращает true если элементы массива расположены в сортированном порядке (low to high)
	//
	bool   isSorted() const;
	//
	// Descr: оставляет в массиве только те элементы, которые одновременно содержатся в
	//   массиве pList. Если pList == 0, то массив this очищается (freeAll).
	// ARG(pList  IN): Массив, пересечение с которым необходимо получить
	// ARG(binary IN): Если этот параметр !0, то присутствие элемента массива this
	//   в массиве pList выясняется применением функции бинарного поиска, что
	//   значительно ускоряет выполнение, но предполагает, что pList отсортирован
	//   (@see LongArray::sort()).
	//
	void   intersect(const LongArray * pList, int binary = 0);
#ifndef _WIN32_WCE // {
	void   intersect(const UintHashTable * pList);
#endif // } _WIN32_WCE
	int    FASTCALL add(long value);
	LongArray & FASTCALL addr(long value);
	//
	// Descr: Вставляет элемент value в конец списка только если value != 0
	//
	int    FASTCALL addnz(long value);
	int    FASTCALL addUnique(long);
	int    CDECL    addzlist(long first, ...);
	int    FASTCALL add(const LongArray *);
	int    FASTCALL addUnique(const LongArray *);
	//
	// Descr: Добавляет в массив this все элементы из pS, которых нет в this, и которые
	//   отсутствуют в pExcl. Если pExcl == 0, то работает как addUnique.
	//   Элементы, присутствующие в pExcl из существующих до вызова функции в this
	//   элементов не удаляются.
	//
	int    addUniqueExclusive(const LongArray * pS, const LongArray * pExcl);
	//
	// Descr: То же, что и addUniqueExclusive(), но не проверяет добавляемые элементы на уникальность.
	//   За счет этого функция работает на много быстрее, чем addUniqueExclusive().
	//
	int    addExclusive(const LongArray * pS, const LongArray * pExcl);
	enum {
		srlzfSortAndUndup = 0x0001
	};
	int    Serialize__construction(int dir, long flags, SBuffer & rBuf, SSerializeContext * pSCtx);
};
//
//
//
class Int64Array : public TSVector <int64> {
public:
	Int64Array();
	Int64Array(const Int64Array & s);
	Int64Array & FASTCALL operator = (const Int64Array & s);
	bool   FASTCALL operator == (const Int64Array & rS) const;
	bool   FASTCALL operator != (const Int64Array & rS) const;
	bool   FASTCALL IsEq(const Int64Array * pS) const;
	int64  FASTCALL get(uint pos) const;
	bool   lsearch(int64 key, uint * pPos) const;
	bool   FASTCALL lsearch(int64 key) const;
	bool   FASTCALL bsearch(int64 key) const;
	int    FASTCALL add(int64);
	int    FASTCALL add(int32);
	int    FASTCALL addUnique(int64);
	int    FASTCALL add(const Int64Array *);
	void   sort();
	void   sortAndUndup();
	void   intersect(const Int64Array * pList, int binary = 0);
};
//
//
//
typedef TSVector <S_GUID> UuidArray;
//
//
//
class DateTimeArray : public TSVector <LDATETIME> {
public:
	DateTimeArray();
	int    FASTCALL add(LDATETIME dtm);
	LDATETIME FASTCALL get(uint idx) const;
	void   sort();
};
//
//
//
class FloatStack : public TSStack <float> {
public:
	FloatStack();
	void   pushPoint(SPoint2F p);
	int    popPoint(SPoint2F & rP);
};
//
//
//
struct LAssocBase { // @noctr
	int32  Key;
	int32  Val;
};

struct LAssoc : public LAssocBase {
	LAssoc();
	LAssoc(int32 key, int32 val);
	LAssoc(const LAssoc & rS);
	bool   FASTCALL operator == (const LAssoc &) const;
	bool   FASTCALL operator != (const LAssoc &) const; // @v11.3.2
	void   Set(int32 key, int32 val);
};

class LAssocArray : public TSVector <LAssoc> {
public:
	LAssocArray();
	LAssocArray(const LAssocArray &);
	LAssocArray & FASTCALL operator = (const LAssocArray &);
	bool   FASTCALL operator == (const LAssocArray &) const;
	//
	// Returns:
	//   0  - в массиве уже есть элемент с идентификатором id
	//   !0 - в массиве элемент с таким идентификатором отсутствует.
	//
	bool   CheckUnique(long key, int binary = 0) const;
	//
	// Descr: Считает количество элементов, соответствующих ключу key
	//
	long   FASTCALL Count(long key) const;
	int    GetListByKey(long key, LongArray & rList) const;
	int    GetListByVal(long val, LongArray & rList) const;
	//
	// Descr: Возвращает упорядоченный уникальный список ключей (Key)
	//
	int    GetKeyList(LongArray & rList) const;
	//
	// Descr: Возвращает упорядоченный уникальный список значений (Val)
	//
	int    GetValList(LongArray & rList) const;
	int    Add(long key, long val);
	int    Add(long key, long val, uint * pPos, int inOrder = 0);
	int    AddUnique(long key, long val, uint * pPos, int inOrder = 0);
	//
	// Descr: Если находит в массиве элемент со ключем key, то увеличивает соответствующее значение val на единицу,
	//   в противном случае добавляет новый элемент с ключем key и значением val.
	// Returns:
	//   1 - ключ key найден и соответствующее значение увеличено на 1
	//   2 - ключ key не найден - создан новый элемент со значением 1.
	//
	int    FASTCALL IncrementValueByKey(long key);
	int    Update(long key, long val, int binary = 0);
	//
	// Descr: Удаляет первый элемент массива, ключ которого равен key.
	// Note: Если в массиве имеется более одного элемента с ключом key, то все остальные останутся на месте
	// Returns:
	//   >0 - обнаружен и удален элемент с ключом key
	//    0 - в массиве нет ни одного элемента с ключом key (либо не был найден из-за параметра binary != 0 на неотсортированном массиве)
	//
	int    Remove(long key, int binary = 0);
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
	//  true  - ключ key найден в массиве
	//  false - ключ не найден
	//
	bool   Search(long key, long * pVal, uint * pPos/*, int binary = 0*/) const;
	//
	// Descr: Тоже что и Search, но поиск бинарный (предполагается, что список отсортирован по ключу в возрастающем порядке).
	//
	bool   BSearch(long key, long * pVal, uint * pPos) const;
	bool   SearchByVal(long val, long * pKey, uint * pPos) const;
	bool   SearchPair(long key, long val, uint * pPos) const;
	bool   CheckFlag(long key, long mask, int binary = 0) const;
	//
	// Descr: Сортирует массив по ключу (Key) в возрастающем порядке.
	//
	void   Sort();
	//
	// Descr: Сортирует массив по значению (Val) в возрастающем порядке.
	//
	void   SortByVal();
	//
	// Descr: Сортирует массив по паре {Key; Val} в возрастающем порядке.
	//
	void   SortByKeyVal();
	//
	// Descr: Сортирует массив по паре {Val; Key} в возрастающем порядке.
	//
	void   SortByValKey();
};
//
//
//
struct LLAssoc {
	bool   FASTCALL operator == (const LLAssoc &) const;
	int64  Key;
	int64  Val;
};

class LLAssocArray : public TSVector <LLAssoc> {
public:
	LLAssocArray();
	LLAssocArray(const LLAssocArray &);
	LLAssocArray & FASTCALL operator = (const LLAssocArray &);
	int    FASTCALL operator == (const LLAssocArray &) const;
	//
	// Returns:
	//   0  - в массиве уже есть элемент с идентификатором id
	//   !0 - в массиве элемент с таким идентификатором отсутствует.
	//
	int    CheckUnique(int64 key, int binary = 0) const;
	//
	// Descr: Считает количество элементов, соответствующих ключу key
	//
	long   FASTCALL Count(int64 key) const;
	int    GetListByKey(int64 key, Int64Array & rList) const;
	int    GetListByVal(int64 val, Int64Array & rList) const;
	int    Add(int64 key, int64 val, uint * pPos, int inOrder = 0);
	int    AddUnique(int64 key, int64 val, uint * pPos, int inOrder = 0);
	int    Update(int64 key, int64 val, int binary = 0);
	int    Remove(int64 key, int binary = 0);
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
	int    Search(int64 key, int64 * pVal, uint * pPos/*, int binary = 0*/) const;
	//
	// Descr: Тоже что и Search, но поиск бинарный (предполагается, что список отсортирован по ключу в возрастающем порядке).
	//
	int    BSearch(int64 key, int64 * pVal, uint * pPos) const;
	int    SearchByVal(int64 val, int64 * pKey, uint * pPos) const;
	int    SearchPair(int64 key, int64 val, uint * pPos) const;
	void   Sort();
};
//
//
//
struct RAssoc {
	RAssoc() : Key(0), Val(0.0)
	{
	}
	RAssoc(long k, double v) : Key(k), Val(v)
	{
	}
	long   Key;
	double Val;
};

class RAssocArray : public TSVector <RAssoc> {
public:
	RAssocArray();
	RAssocArray(const RAssocArray &);
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
	int    Search(long key, double * pVal, uint * pPos, int binary = 0) const;
	int    FASTCALL Has(long key) const;
	double Get(long key, int binary = 0) const; // @>>RAssocArray::Search
	int    GetList(LongArray & rList, int unique) const;
	void   Scale(double rate);
	int    SearchMaxVal(uint * pPos) const;

	enum {
		dfRound = 0x0001, // Округлять каждое распределенное значение до точность prec
		dfReset = 0x0002  // Сбросить остаток на последний элемент
	};
	int    Distribute(double amount, long flags, int prec, RAssocArray & rResult) const;
	double GetTotal() const;
	int    Add(long key, double val, int additive = 1, int binary = 0);
	int    Add(const RAssocArray & rList, int additive = 1, int binary = 0);
	int    Remove(long key, int binary = 0);
	void   SortByKey();
	//
	// Descr: Сортирует массив в порядке увеличения значения Val
	//
	void   SortByVal();
	//
	// Descr: Сортирует массив в порядке уменьшения значения Val
	//
	void   SortByValRev();
};
//
//
//
struct RPair {
	double X;
	double Y;
};

class RPairArray : public TSVector <RPair> {
public:
	RPairArray();
	RPairArray(const RPairArray &);
	RPairArray & FASTCALL operator = (const RPairArray &);
	RPair & FASTCALL operator [](size_t i) const;
	void   SortByX();
	void   SortByY();
};

#ifndef _WIN32_WCE // {
//
//
//
struct UUIDAssoc {
	long   Key;
	S_GUID Val;
};

class UUIDAssocArray : public TSVector <UUIDAssoc> {
public:
	UUIDAssocArray();
	int    Add(long key, const S_GUID & rVal, uint * pPos);
	bool   Search(long key, S_GUID * pVal, uint * pPos) const;
	bool   SearchVal(const S_GUID_Base &, long * pKey, uint * pPos) const;
	int    GetMaxKey(long * pMaxKey, uint * pPos) const;
};
#endif // } _WIN32_WCE
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
	SStrGroup();
	SStrGroup(const SStrGroup & rS);
	SStrGroup & FASTCALL operator = (const SStrGroup & rS);
	size_t GetPoolDataLen() const;
	size_t GetPoolSize() const;
	//
	// Descr: Очищает содержимое пула строк не освобождая память.
	//   Полезно применять при ожидаемом повторном использовании объекта.
	//
	void   ClearS();
	//
	// Descr: То же, что и ClearS, но с освобождением памяти.
	//
	void   DestroyS();
	SStrGroup & FASTCALL CopyS(const SStrGroup & rS);
	int    AddS(const char * pStr, uint * pPos);
	int    AddS(const char * pStr, uint32 * pPos);
	bool   GetS(uint pos, SString & rStr) const;
	int    SerializeS(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
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
	void * Pack_Start() const;
	int    Pack_Finish(void * pHandle);
	int    Pack_Replace(void * pHandle, uint & rPos) const;

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
class StrAssocArray : private LAssocArray, private SStrGroup {
public:
	friend int Helper_CmpFunc_StrAssocByText(const LAssoc *, const LAssoc *, StrAssocArray *, LAssocArray *);
	friend int Helper_CmpFunc_StrAssocByID(long, long, StrAssocArray *, LAssocArray *);

	struct Item {
		long   Id;
		long   ParentId;
		const  char * Txt;
	};
	StrAssocArray();
	StrAssocArray(const StrAssocArray & rS);
	~StrAssocArray();
	StrAssocArray & FASTCALL operator = (const StrAssocArray & rS);
	int    FASTCALL Copy(const StrAssocArray & rS);
	StrAssocArray & Z();
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
	bool   FASTCALL IsEq(const StrAssocArray & src) const;
	uint   getCount() const { return LAssocArray::count; }
	uint   getPointer() const;
	uint   setPointer(uint p);
	uint   incPointer();
	long   FASTCALL GetKeyByIdx(uint pos) const;
	Item   FASTCALL Get(uint pos) const;
	//
	// Descr: То же, что и StrAssocArray::at, только не пытается инициализировать
	//   родительский идентификатор. Благодаря этому функция выполняется значительно
	//   быстрее, чем предыдущая.
	//
	Item   FASTCALL at_WithoutParent(uint pos) const;
	int    AtFree(uint pos);
	int    Search(long id, uint * pPos) const;
	int    FASTCALL Search(long id) const;
	//
	// Descr: Ищет в массиве элемент, текст которого совпадает с параметром pTxt.
	//   поиск не чувствителен к регистру.
	//   Если в массиве содержится более одного элемента, соответствующего
	//   критерию то найден будет только самый первый.
	// Returns:
	//   !0 - элемент найден
	//   0  - элемент не найден
	//
	int    SearchByText(const char * pTxt, int ignoreCase, uint * pPos) const;
	int    SearchByText(const char * pTxt, CompFunc fcmp, uint * pPos) const;
	//
	// Descr: Извлекает из списка строку с идентификатором id.
	// ARG(id    IN): идентификатор искомой строки
	// ARG(rBuf OUT): ссылка на буфер, в который будет скопирована найденная строка.
	//   Если строка не найдена, то буфер гарантированно обрезается до нулевой длины.
	// Returns:
	//   >0 - элемент с идентификатором id найден и соответствующая ему строка не пустая.
	//   <0 - элемент с идентификатором id найден, но соответствующая ему строка пустая.
	//    0 - элемент с идентификатором id не найден (rBuf на выходе пуст)
	//
	int    GetText(long id, SString & rBuf) const;
	//
	// Descr: Функционально то же самое, что и GetText, но осуществляет поиск
	//   в расчете на то, что список пар {id, position} отсортирован по возрастанию
	//   идентификаторов.
	//   Работает значительно быстрее, чем GetText но требует ответственности вызывающего
	//   модуля за то, что список предварительно отсортирован.
	// Note: SortByID отсортирует список адвекватно этому методу только в случае,
	//   когда внутрений список родительских ссылок пуст (то есть данный экземляр не
	//   является иерархическим).
	//
	int    GetTextBinary(long id, SString & rBuf) const;
	//
	// Descr: Находит максимальное значение идентификатора элемента в массиве.
	// Returns:
	//   >0 - в массиве есть как минимум один элемент. По указателю pId присвоено
	//        максимальное значение идентификатора.
	//   <0 - в массиве нет ни одного элемента. По указателю pId ничего не присваивается.
	//
	int    GetMaxID(long * pId) const;
	int    GetMaxLength(uint * pMaxLen) const;
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
	int    Add(long id, const char * pStr, int replaceDup = 1);
	int    Add(long id, long parentId, const char * pStr, int replaceDup = 1);
	//
	// Descr: Функция, аналогичная Add(id, pStr, 0/*replaceDup*/).
	//   Важное отличие - не проверяет наличие идентификатора id в списке. За счет
	//   этого выполняется значительно быстрее, но может привести к появлению дубликатов.
	// ARG(id         IN): Идентификатор элемента
	// ARG(pStr       IN): Строка, соответствующая идентификатору id
	// Returns:
	//   1 - функция успешно добавила новый элемент в массив
	//   0 - ошибка (SLibError)
	//
	int    AddFast(long id, const char * pStr);
	//
	// Descr: Функция, аналогичная Add(id, parentId, pStr, 0/*replaceDup*/).
	//   Важное отличие - не проверяет наличие идентификатора id в списке. За счет
	//   этого выполняется значительно быстрее, но может привести к появлению дубликатов.
	// ARG(id         IN): Идентификатор элемента
	// ARG(parentId   IN): Идентификатор родительского элемента. Если 0, то считает
	//   новый элемент относящимся к самому верхнему уровню.
	//   Функция не проверяет является ли parentId действительным членом списка и не
	//   делает тестов на цикличность.
	// ARG(pStr       IN): Строка, соответствующая идентификатору id
	// Returns:
	//   1 - функция успешно добавила новый элемент в массив
	//   0 - ошибка (SLibError)
	//
	int    AddFast(long id, long parentId, const char * pStr);
	//
	// Descr: Изменяет значение идентификатора элемента в позиции pos на newId.
	// Attention: Функция опасная. Не следует использовать ее в тех случаях, когда
	//   массив содержит иерархические данные, а также не следует рассчитывать на то, что
	//   эта функция обеспечит уникальность идентификаторов.
	//
	int    UpdateByPos(uint pos, long newId);
	//
	// Descr: Вносит в массив rList все идентификаторы из this.
	// Note: rList предварительно очищается, а по завершении функции сортируется и лишается дубликатов (sortAndUndup)
	// Returns:
	//   >0 - в список rList внесен по крайней мере один элемент
	//   <0 - список this пустой (соответственно и rList при выходе из функции пуст)
	//
	int    GetIdList(LongArray & rList) const;
	int    GetListByParent(long parentId, int recursive, LongArray &) const;
	bool   FASTCALL HasChildren(long id) const;
	int    GetParent(long id, long * pParentId) const;
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
	int    Remove(long id);
	int    Swap(uint p1, uint p2);
	int    Move(uint idxFrom, uint idxTo);
	//
	// Descr: Обнуляет все родительские ссылки элементов.
	//
	void   ClearParents();
	int    Pack();
	void   SortByID();
	void   SortByText();
	void   SortByTextInTreeOrder();
	void   SortByLength(int descend = 0);
	void   Reverse();
	int    RemoveRecursion(LongArray * pBadList);
	//
	// Descr: Узкоспециализированная функция, извлекающая позицию текстовой строки
	//   по индексу записи.
	// Note: В общем случае следует избегать использования этой функции. В частности потому,
	//   что позиции текста могут изменяться.
	//
	uint   FASTCALL GetTextPos(uint idx) const;
	int    GetTextByPos(uint textPos, SString & rBuf) const;
	int    FASTCALL Write(SBuffer & rBuf, long o) const;
	int    FASTCALL Read(SBuffer & rBuf, long o);
#ifndef _WIN32_WCE // {
	int    Write(SFile & rFile, long) const;
	int    Read(SFile & rFile, long);
#endif // } _WIN32_WCE

	// @v10.7.11 int    Test_Cmp(const TaggedStringArray_obsolete &, int * pNEqPos) const;
	static int Helper_CmpFunc_StrAssocByText(const LAssoc * p1, const LAssoc * p2, StrAssocArray * pArray, LAssocArray * pRecurTracer); // @reallyprivate
protected:
	CompFunc TextCmpProc;
	LAssocArray ParentList;
private:
	int    Helper_RemoveRecursion(long parentId, LongArray & rRecurList, LongArray & rPassedList, LongArray * pBadList);
	void   Helper_SortByTextInTreeOrder(long parentId, StrAssocArray & rTempList);
	LAssocArray * P_ParentIndex; // @v11.2.3 @transient Индекс родительских элементов для ускорения сортировки
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
	StrStrAssocArray();
	StrStrAssocArray(const StrStrAssocArray & rS);
	StrStrAssocArray & FASTCALL operator = (const StrStrAssocArray &);
	int    FASTCALL Copy(const StrStrAssocArray &);
	StrStrAssocArray & Z();
	bool   FASTCALL IsEq(const StrStrAssocArray & src) const;
	uint   getCount() const;
	Item   FASTCALL at(uint pos) const;
	int    Search(const char * pKey, SString * pVal, uint * pPos) const;
	int    Add(const char * pKey, const char * pVal, int replaceDup = 1);
	int    Remove(const char * pKey);
protected:
	LAssocArray L;
};
//
// Descr: Древовидная коллекция ассоциаций {int32; string}.
//   Во-многом повторяет StrAssocArray но быстрее за счет хранения элементов в виде дерева, а не в виде
//   линейного массива.
//   Соответственно, есть и ограничения:
//     -- ключевой элемент строго не нулевой
//     -- дубликаты по ключу не допускаются
// Note: Судя по всему, этот класс будет использоваться только для отображения ирерархических структур в списках и т.д.
//   По-этому он включает ряд полей для "заточки" под этот вариант использования.
//
class StrAssocTree : private SStrGroup {
private:
	class ENTRY {
	public:
		class BaseList : public TSArray <ENTRY> {
		public:
			BaseList(const ENTRY * pParent);
			BaseList(const BaseList & rS);
			BaseList & Z();
			int    FASTCALL Copy(const BaseList & rS);
			uint   GetCount() const;
			bool   Implement_Get_Unsafe(SHandle h, int32 currentParentId, long * pId, long * pParentId, uint * pStrP) const;
			//
			// Descr: Реализует вставку в конец вектора нового элемента с идентификатором key и позицией строки strP.
			// Returns:
			//   !0 - Указатель на новый элемент в векторе.
			//    0 - Ошибка
			// Attention! Возвращаемое значение является нестабильным указателем - после вставки новых элементов он
			//   может оказаться инвалидным - будьте внимательны.
			//
			ENTRY * Implement_Add(int32 key, uint strP);
			ENTRY * Implement_Search(int32 key, int32 currentParentId, int32 * pResultParentId) const;
			int    Implement_SearchStrP(uint strP, TSVector <ENTRY *> & rResult) const;
			void   Implement_SortByID();
			void   Implement_SortByText(void * pMaster);
			bool   Implement_MakeStringIndex(LAssocArray & rIndex) const;
			//
			// Descr: Дополняет список rList собственными идентификаторами ключей.
			//   Если параметр recursive == true, то рекурсивно перебирает все дочерние деревья //
			//
			void   GetKeyList(bool recursive, LongArray & rList) const;
			bool   FASTCALL ValidateEntryPtr(const void * pEntry) const;
		private:
			virtual void FASTCALL freeItem(void * pItem);
			const ENTRY * P_Parent;
		};

		ENTRY();
		ENTRY(const BaseList * pParent, int32 key, uint32 strP, uintptr_t extra);
		void   Destroy();
		int    FASTCALL Copy(const ENTRY & rS);
		uint   GetCount() const;
		//
		int32  Key;
		uint32 StrP;
		uint32 Flags;
		BaseList * P_Children;
		const BaseList * P_ParentList;
		uintptr_t ExtraPtr;
	};
public:
	//
	// Заметки по поводу операций, базирующихся на SHandle:
	// 1. Физически, handle это - указатель на ENTRY
	// 2. Как следствие, при изменениях дерева (включение новых элементов, удаление существующих) эти манипуляторы могут стать инвалидными
	//
	//
	StrAssocTree();
	StrAssocTree(const StrAssocTree & rS);
	int    FASTCALL Copy(const StrAssocTree & rS);
	StrAssocTree & Z();
	uint   GetCount() const;
	bool   HasChildren(long id) const;
	bool   Search(long id, long * pParentId, SString * pBuf) const;
	SHandle FASTCALL Search(long id) const;
	SHandle SearchByText(const char * pTxt, int ignoreCase) const;
	bool   SearchByText(const char * pTxt, CompFunc fcmp, TSVector <SHandle> & rList) const;
	//
	// Методы получения атрибутов узла дерева по манипулятору
	//
	bool   Get_Unsafe(SHandle h, long * pId, long * pParentId, SString * pBuf) const;
	bool   HasNodeChildren(SHandle h) const;
	bool   SetNodeExtraPtr(SHandle h, uintptr_t extraPtr);
	uintptr_t GetNodeExtraPtr(SHandle h) const;
	int32  GetNodeKey(SHandle h) const;
	SString & GetNodeString(SHandle h, SString & rBuf) const;
	uint   GetNodeFlags(SHandle h) const;
	//
	// Descr: Формирует полный список ключевых идентификаторов, хранящихся в дереве.
	//  Порядок обхода элементов дерева - в глубину: перебираются элементы корневого уровня и для каждого элемента,
	//  имеющего потомков, перебираются эти потомки.
	//  Параметр rList в начале функции обнуляется. После выполнения функции список НЕ сортируется и
	//  из него НЕ удаляются дубликаты (нет вызова rList.sortAndUndup()).
	//
	int    GetList(LongArray & rList) const;
	int    GetListByParent(long parentId, bool recursive, LongArray & rList) const;
	int    GetListByParent_Unsafe(SHandle hParent, bool recursive, TSVector <SHandle> & rList) const;
	int    Add(long id, const char * pStr, int replaceDup);
	int    Add(long id, long parentId, const char * pStr, int replaceDup);
	//
	// Descr: Функция вставляет в новый элемент {id; pStr} в дерево this.
	//   Если hParent != 0, то новый элемент включается как лист hParent в противном
	//   случае элемент включается в корневой уровень дерева.
	// Note: Функция хотя и быстрая, но опасная. В режиме отладочной сборки hParent проверяется на валидность,
	//   в Release-варианте сборке - нет. Существование элемента с идентфикатором id ни в дереве, ни на уровне hParent
	//   не проверяется!
	//   Возвращаемый манипулятор является нестабильным - после вставки новых элементов он с высокой вероятностью
	//   может оказаться инвалидным!
	// Returns:
	//   !0 - функция завершилась успешно. Результат - манипулятор нового элемента.
	//    0 - ошибка
	//
	SHandle Add_Unsafe(SHandle hParent, long id, const char * pStr);
	bool   MakeStringIndex(LAssocArray & rIndex) const;
	bool   GetTextByPos(uint strP, SString & rBuf) const;
	void   SortByID();
	void   SortByText();
protected:
	CompFunc TextCmpProc;
private:
	ENTRY::BaseList L;
	static int Helper_CmpFunc_ByText(const void * p1, const void * p2, const StrAssocTree * pArray);
	friend DECL_CMPFUNC(StrAssocTreeEntry_ByText);
	bool   FASTCALL ValidateEntryHandle(const SHandle h) const;
};
//
// Descr: Пул элементов файлового каталога.
// Note: since v11.6.0 наименования файлов хранятся в кодировке utf8
//
class SFileEntryPool : public SStrGroup {
public:
	friend DECL_CMPCFUNC(SFileEntryPool_Entry);
	struct Entry {
		Entry();
		Entry & Z();

		uint32 Attr; // SDirEntry::attrXXX
		LDATETIME CreateTime;
		LDATETIME AccessTime;
		LDATETIME WriteTime;
		int64  Size;
		SString Name;
		SString Path;
		SString RemoteUrl; // @v10.8.2 Специальный "хвост" позволяющий сопоставить элемент пула с каким-либо серверным url-ом
	};
	SFileEntryPool();
	SFileEntryPool & Z();
	//
	// Descr: Ищет в коллекции имя файла (без пути) pFileName. Если такое имя найдено,
	//   то возвращает >0 и, если pEntry != 0, то присваивает по этому указателю полную структуру найденного элемента.
	// Returns:
	//   >0 - искомое имя найдено
	//    0 - имя не найдено (по указателю pEntry ничего не присваивается)
	//
	int    STDCALL  SearchName(const char * pFileName, Entry * pEntry) const;
	void   FASTCALL SetInitPath(const char * pPath);
	int    FASTCALL GetInitPath(SString & rPath) const;
	//
	// Descr: Флаги функции Scan()
	//
	enum {
		scanfRecursive = 0x0001 // Рекурсивно перебирать файлы в подкаталогах
	};
	//
	// Descr: Сканирует файловую систему начиная с каталога pPath по шаблону pWildcard.
	// Note: Если !isempty(pPath), то он сохраняется во внутреннем представленнии головного каталога.
	// Returns:
	//   >0 - найден по крайней мере один файл, удовлетворяющий условиям
	//   <0 - не найдено ни одного файла
	//   0  - ошибка
	//
	int    Scan(const char * pPath, const char * pWildcard, long flags);
	int    Add(const char * pPath, const SDirEntry & rDe);
	//
	// Descr: Вставляет в пул файл с полным путем pPath.
	// Returns:
	//   >0 - функция завершилась успешно
	//    0 - ошибка (в том числе если файл не найден)
	//
	int    Add(const char * pPath);
	int    Add(const char * pPath, const char * pRemoteUrl);
	int    Add(SFileEntryPool::Entry & rDe);
    uint   GetCount() const;
    int    Get(uint pos, SFileEntryPool::Entry * pEntry, SString * pFullPath) const;
	//
	// Descr: Удаляет элемент по индексу pos.
	// Note: Само-собой разумеется, что сам файл не удаляется.
	//
	int    FASTCALL Remove(uint pos);

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
    void   Sort(int criteria);
private:
	struct InnerEntry {
		uint   Attr;
		LDATETIME CreateTime;
		LDATETIME AccessTime;
		LDATETIME WriteTime;
		int64  Size;
		uint   NameP;
		uint   PathP;
		uint   RemoteUrlP; // @v10.8.2
	};

	void   ConvertEntry(const InnerEntry & rIEntry, Entry & rEntry) const;
	int    Implement_Add(const char * pPath, const SDirEntry & rDe, const char * pRemoteUrl);
	int    Helper_Scan(const char * pPath, const char * pWildcard, long flags);

    TSVector <InnerEntry> L;
    long   State;
	uint   InitPathP; // Опциональная позиция начального каталога. В общем случае
		// это поле не важно, но иногда функции "хотят" передавать друг другу
		// головной каталог вместе с набором файлов.
};
//
// Descr: Список временных отрезков.
// Remark: Класс спроектирован с учетом того, что порожденные классы могут использовать
//   элементы списка, отличные от STimeChunk, но при этом, такие элементы должны в начале
//   обязательно содержать структуру STimeChunk.
//
class STimeChunkArray : public SArray { // @attention dont't change to SVector
public:
	STimeChunkArray();
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
	int    Add(const STimeChunk * pItem, int checkForFree);
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
	int    Excise(const STimeChunk * pItem);
	//
	// Descr: Формирует список rResult состоящий из объединения временных отрезков this со списком отрезков pList.
	// Note: Функция не пытается объединять пересекающиеся элементы this но только лишь соединяет this с pList.
	//
	int    Union(const STimeChunkArray * pList, STimeChunkArray & rResult) const;
	//
	// Descr: Формирует список rResult состоящий из объединения временных отрезков this с отрезком rChunk.
	// Note: Функция не пытается объединять пересекающиеся элементы this но только лишь соединяет this с rChunk.
	//
	int    Union(const STimeChunk & rChunk, STimeChunkArray & rResult) const;
	int    Intersect(const STimeChunkArray * pList, STimeChunkArray & rResult) const;
	int    Intersect(const STimeChunk & rChunk, STimeChunkArray & rResult) const;
	//
	// Descr: Возвращает порядок пересечения отрезков в массиве.
	//   Порядок пересечения представляет количество "этажей" в которое можно выстроить
	//   отрезки так, что бы в каждом "этаже" не было пересечения //
	//
	uint   GetIntersectionOrder(LAssocArray * pOrderList, long tolerance = 0) const;
	//
	// Descr: Сортирует список
	//   Если desc == 0, то порядок сортировки - по возрастанию моменту начала элементов (PrcBusy::Start)
	//   в противном случае - по убыванию момента начала элементов
	//   Функция может быть использована в порожденных класса, элементы которых унаследованы от STimeChunk
	//
	void   Sort(int desc = 0);
	//
	// Descr: Ограничивает все элементы массива периодом pLimit.
	//   Те элементы, которые выходят за пределы pLimit удаляются из массива,
	//   те, которые пересекаются с pLimit - ограничиваются пересечением.
	//   Функция может быть использована в порожденных класса, элементы которых унаследованы от STimeChunk
	// Returns:
	//   >0 - по крайней мере один из элементов массива был либо удален, либо изменен.
	//   <0 - все элементы массива удовлетворяют ограничению. Ничего не изменено.
	//
	int    Limit(const STimeChunk * pLimit);
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
	int    IsFreeEntry(const STimeChunk & rEntry, uint * pPos) const;
	int    IsFreeEntry(const LDATETIME &, long cont, uint * pPos) const;
		// @>>STimeChunkArray::IsFreeEntry(const STimeChunk &, uint *);
	//
	// Descr: Получает список доступных периодов.
	//   До вызова этой функции массив this должен быть отсортирован функцией STimeChunkArray::Sort.
	//
	int    GetFreeList(STimeChunkArray * pList) const;
	//
	// Descr: Находит нижнюю и верхнюю границы списка. Если параметр update != 0, то
	//   устанавливает нижнюю границу не меньше, чем pBounds->Start,
	//   а верхнюю не больше чем pBounds->Finish.
	//
	int    GetBounds(STimeChunk * pBounds, int update) const;
protected:
	explicit STimeChunkArray(size_t itemSize);
private:
	int    Helper_Union(const STimeChunk & rChunk, STimeChunkArray & rResult) const;
	int    Helper_Intersect(const STimeChunk & rChunk, STimeChunkArray & rResult) const;
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
	STimeChunkAssoc();

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
	explicit STimeChunkAssocArray(long id);
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
	STimeChunkGrid & FASTCALL operator = (const STimeChunkGrid &);
	STimeChunkAssocArray * Get(long id, uint * pPos) const;
	int    GetChunk(long chunkId, long * pRowId, STimeChunkAssoc *) const;
	int    GetChunksByTime(const STimeChunk & rRange, STimeChunkAssocArray & rList) const;
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
		explicit HolidayArray(long id);
		long   Id;
	};
	TSCollection <HolidayArray> HL;
	STimeChunkArray * P_CollapseList;
	StrAssocArray NameList;
};
//
// Descr: Базовый класс для организации hash-таблиц.
//   Хранит пары {long; long} в таблице фиксированного размера, заданного при создании экземпляра.
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
		uint16 Count; // Количество элементов в точке кэша (включая Val). То есть, если Count == 0, то точка не содержит элементов.
		LAssoc * P_Ext;
	};

	explicit HashTableBase(size_t sz);
	HashTableBase(const HashTableBase & rS);
	~HashTableBase();
	void   Clear();
	int    FASTCALL Copy(const HashTableBase & rSrc);
	int    InitTab();
	void   DestroyTabItems();
	int    FASTCALL InitIteration(Iter * pI) const;

	uint32 Size; // Фиксированное количество слотов в хэше
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
// Descr: Хэш-таблица ассоциирующая целочисленные идентификаторы (token) со строковыми значениями.
//   Обратная ассоциация (строка->токен) не предусмотрена.
//
class TokenSymbHashTable : public HashTableBase, private SStrGroup {
public:
	explicit TokenSymbHashTable(size_t sz);
	TokenSymbHashTable(TokenSymbHashTable & rS);
	TokenSymbHashTable & FASTCALL operator = (const TokenSymbHashTable &);
	int    Put(long token, const char * pSymb);
	int    Get(long token, SString * pSymb) const;
private:
	size_t FASTCALL Hash(long key) const;
};
//
// Descr: строковая хэш-таблица. Реализует хранение ассоциаций строковых символов
//   с 4-байтовыми значениями.
//
class SymbHashTable : public HashTableBase { // @persistent
public:
	explicit SymbHashTable(size_t sz, int useAssoc = 0);
	SymbHashTable(const SymbHashTable & rS);
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
	//   true  - символ найден (по указателю pVal присвоено значение ассоциированного значения).
	//   false - символ не найден (значение по указатель pVal не меняется)
	//
	bool   Search(const char * pSymb, uint * pVal, uint * pPos) const;
	int    FASTCALL InitIteration(Iter * pI) const;
	int    NextIteration(Iter *, uint * pVal, uint * pPos, SString * pStr) const;
	//
	// Descr: Возвращает максимальное значение идентификатора Val из всех, хранящихся в таблице.
	//
	uint   GetMaxVal() const;
	//
	// Descr: Удаляет ассоцииации идентификаторов символов с позициями.
	//   После вызова функции хэш теряет статус useAssoc.
	//
	void   ResetAssoc();
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
class LAssocHashTable : public HashTableBase {
public:
	explicit LAssocHashTable(size_t sz, int useAssoc = 0);
	LAssocHashTable & FASTCALL operator = (const LAssocHashTable &);
	int    FASTCALL Copy(const LAssocHashTable & rSrc);
	void   Clear();
	int    Set(long key, long val);
	int    Del(long key, long * pVal);
	int    Search(long key, long * pVal) const;
	int    FASTCALL InitIteration(Iter * pI) const;
	int    NextIteration(Iter *, long * pKey, long * pVal) const;
private:
	size_t FASTCALL Hash(const long & rKey) const;
};
//
//
//
class GuidHashTable : public HashTableBase  { // @persistent
public:
	explicit GuidHashTable(size_t sz, int useAssoc = 0);
	GuidHashTable(const GuidHashTable & rS);
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
	explicit PtrHashTable(size_t sz, int useAssoc = 0);
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
	int    Del(const void * ptr, uint * pVal);
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
	int    FASTCALL Add(ulong value);
	//
	// Descr: Вставляет в таблицу новый элемент value при условии, что value != 0
	//
	int    FASTCALL AddNZ(ulong value);
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
	SVector List;
};
//
// BitArray
//
class BitArray : private SBaseBuffer {
public:
	BitArray();
	BitArray(const BitArray &);
	~BitArray();
	BitArray & FASTCALL operator = (const BitArray &);
	int    FASTCALL Copy(const BitArray &);
	int    Init(const void *, size_t count);
	//
	// Descr: Обнуляет внутренний буфер и сбрасывает счетчик битов в 0
	//
	void   Clear();
	size_t getCount() const;
	bool   FASTCALL IsEq(const BitArray &) const;
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
	size_t getBufSize() const;
	int    getBuf(void *, size_t maxLen) const;
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
		explicit Iter(uint32 parentP);
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
	explicit STree(uint itemSize, uint o = O_ARRAY);
	~STree();
	STree & Z();
	int    Insert(const void * pData, uint32 parentP, uint32 * pSvcPos);
	int    InsertBefore(const void * pData, uint32 prevP, uint32 * pSvcPos);
	int    UpdateNodeParent(uint32 nodeP, uint32 parentP);
	int    Delete(uint32 svcPos);
	int    Sort(CompFunc fcmp, long extraData = 0);
	uint32 FASTCALL GetFirstChildP(uint32 itemP) const;
	int    HasChildren(uint32 itemP) const;
	int    Search(const void * pKey, uint32 * pIdx, CompFunc fcmp, size_t ofs = 0, void * pExtraData = 0) const;
	int    Search(const void * pKey, Iter & rIter, CompFunc fcmp, size_t ofs = 0, void * pExtraData = 0) const;
	//
	// Descr: Функция обхода дерева. Реализует поиск в глубину. То есть, начиная с вершины, обходит все узлы дерева
	//   с текущим родителем, не имеющие потомков, пока не встретит тот, у которого потомки есть. Тогда спускается
	//   вниз, повторяя предыдущий сценарий. Когда все узлы потомков посещены, возвращается назад и продолжает
	//   движение.
	// Returns:
	//   >0 - получен очередной узел дерева
	//    0 - обход завершен
	//
	int    FASTCALL Enum(Iter & rIter) const;
	//
	// Descr: Возвращает указатель на блок данных, соответствующий позиции узла pos.
	//
	void * FASTCALL GetData(uint32 pos) const;
private:
	struct Item_ {
		uint32 DataP;       // @firstmember
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
	TSVector <Item_> TreeList; // Сервисный линейный список узлов дерева. Оперирует позиционными ссылками.
		// Так как ссылки позиционные, то удаление и перемешивание этого вектора недопустимо. В случае удаления элемента
		// его позиция попадает в SvcFreeList, но из вектора TreeList блок не удаляется.
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
		return static_cast<T *>(STree::GetData(pos));
	}
};
//
// Descr: Простой вектор, содержащий элементы uint32.
//   В отличии от SArray крайне примитивен и занимает очень мало памяти.
//   Так как не содержит конструктора и деструктора, может быть использован в union'ах.
//
class SV_Uint32 { // @noctr @nodestructor
public:
	void   Init();
	void   Destroy();
	int    FASTCALL Copy(const SV_Uint32 &);
	int    FASTCALL Add(uint32);
	int    Add(uint count, const uint32 * pSrc);
	bool   FASTCALL IsEq(const SV_Uint32 & rPat) const;
	bool   Search(uint val, uint * pPos) const;
	uint   GetCount() const;
	uint32 FASTCALL Get(uint idx) const;
	uint32 operator [] (size_t idx) const { return Get(static_cast<uint>(idx)); }
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
		fDivAlNum    = 0x0008, // Если после цифры идет нецифровой (и не разделительный) символ или
			// после нецифрового (не разделительного) символа идет цифра, то считать этот факт неявным разделителем.
		fRawOrgOffs  = 0x0010  // @v10.2.0 Поле Item::OrgOffs заполнять байтовым смещением токена, а не символьным
	};
	enum {
		coClearSymbTab = 0x0001 //
	};
	struct Param {
		Param();
		Param(long flags, int cp, const char * pDelim);
		long   Flags;
		int    Cp;
		SString Delim; // Список символов-разделителей. Если строка пустая, то используется " \t\n\r"
	};
	struct Item {
		int    Token;
		uint   TextId;     // Фактически, позиция текста во внутреннем буфере this. Может быть использовано как идентификатор текста в течении жизни this->T
		int64  OrgOffs;    // Смещение от начала текста в символах
		SString Resource;
		SString Text;
	};
	STokenizer();
	explicit STokenizer(const Param & rParam);
	~STokenizer();
	void   GetParam(Param * pParam) const;
	int    SetParam(const Param * pParam);
	STokenizer & Reset(long options);
	//
	// Descr: Очищает внутренний буфер ввода.
	//
	void   ClearInput();
	//
	// Descr: Вносит новую порцию данных для обработки.
	// ARG(pResource IN): Текстовый идентификатор источника данных. Вызывающий модуль самостоятельно определяет
	//   целесообразность использования и формат идентификатора. Сам же следит за уникальностью.
	// ARG(orgOffs   IN): Смещение до передаваемой порции данных в оригинальном источнике данных.
	// ARG(pS        IN): Собственно данные для обработки
	// ARG(sz        IN): Размер передаваемых данных в байтах
	//
	int    Write(const char * pResource, int64 orgOffs, const void * pS, size_t sz);

	struct ResultPosition {
		uint   Start;
		uint   Count;
	};
	//
	// Descr: Запускает разбор входного потока, начиная с очередного символа входного буфера.
	// ARG(pIdxFirst OUT): Индекс первого элемента результата разбора. Получить результат можно функцией Get(uint, Item &)
	// ARG(pIdxCount OUT): Количество элементов результатов разбора доступного в буфере.
	//
	int    Run(uint * pIdxFirst, uint * pIdxCount);
	//
	// Descr: Высокоуровневая функция, реализующая пару вызовов:
	//   Write(const char * pResource, int64 orgOffs, rS, rS.Len()+1);
	//   Run(pIdxFirst, pIdxCount);
	//
	int    RunSString(const char * pResource, int64 orgOffs, const SString & rS, uint * pIdxFirst, uint * pIdxCount);
	uint   GetCount() const;
	int    Get(uint idx, Item & rItem) const;
	int    Get_WithoutText(uint idx, STokenizer::Item & rItem) const;
	uint   GetCommCount() const;
	int    GetComm(uint idx, Item & rItem) const;
	int    GetTextById(uint txtId, SString & rBuf) const;
	int    GetSymbHashStat(SymbHashTable::Stat & rStat) const;
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
		TSVector <STokenizer::ResourceToken> RL; // Список идентификаторов ресурсов, в которых встречается токен T
	};
	int    Search(long flags, TSCollection <STokenizer::SearchBlockEntry> & rResult);
	int    IndexResources(int force);
private:
	uint16 NextChr();
	int    FASTCALL IsDelim(uint16 chr) const;
	int    FASTCALL AddToken(TSVector <uint16> & rBuf, int tok, int64 sp);
	int    ProcessSearchToken(TSVector <uint16> & rBuf, int tokType, TSCollection <STokenizer::SearchBlockEntry> & rResult);
	void   _CopySrcToAddTokenBuf(const TSVector <uint16> & rBuf);

	struct Token {
		int    T;  //
		uint   PP; // Позиция токена в STokenizer::T
		uint   RP; // Позиция идентификатора ресурса в STokenizer::T
		int64  SP; // Позиция токена в исходном буфере
	};

	struct CToken {
		int    T;
		uint   PP;
	};
	//
	// Descr: Элемент индекса, сопоставляющего позицию токена списку ресурсов, содержащих токен
	//
	struct ResourceIndexItem {
		int    PP;
		TSVector <STokenizer::ResourceToken> RL;
	};

	Param  P;
	uint   Tc;  // Счетчик для формирования уникального значения, ассоциированного с токенами, добавляемыми в T
	uint   RP;  // Позиция текущего идентификатора ресурса в T
	int64  SO;  // Смещение содержимого S в источнике данных: необходимо для сопоставления токена с источником данных
	SBuffer S;  // Буфер входящего потока
	SymbHashTable T;
	TSVector <Token> L;
	TSVector <CToken> CL;
	TSCollection <ResourceIndexItem> * P_ResourceIndex;
	SString  AddTokenBuf;      // @allocreuse Для ускорения AddToken()
	TSVector <uint16> TokenBuf; // @allocreuse Для ускорения Run()
	SStringU DelimU; // Для входящего потока в UTF-формате список разделителей должен быть в Unicode-формате
};
//
#ifndef _WIN32_WCE // {
//
//
//
class SSystem {
public:
	//
	// Классификатор систем программной инфраструктуры
	// Это - не операционные системы, а именно инфраструктурные понятия, в рамках которых
	// могут быть определены те или иные стандарты и классификаторы (например, кодовые страницы)
	//
	enum {
		ssisUnkn    =  0,
		ssisWindows =  1,
		ssisMAC     =  2,
		ssisIBM     =  3,
		ssisAIX     =  4,
		ssisJava    =  5,
		ssisSolaris =  6,
		ssisHPUX    =  7,
		ssisGLibC   =  8,
		ssisBSD     =  9, // @v10.4.4
		ssisGNU     = 10  // @v10.4.4
	};
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
		cpucsAVX2      = 13, // or above = AVX2 supported
		cpucsFMA3_F16C_BMI1_BMI2_LZCNT = 14, // or above = FMA3, F16C, BMI1, BMI2, LZCNT
		cpucsAVVX512F  = 15, // or above = AVVX512F
	};
	enum CpuArch {
		cpuarchX86     = 0x01000000,
		cpuarchARM     = 0x04000000,
	};
	enum CpuFeature {
		cpuftr_NONE        = 0,
		cpuftr_CPU_MASK    = 0x1f000000,
		cpuftr_X86         = 0x01000000,
		cpuftr_ARM         = 0x04000000,
		//
		// x86 CPU features are constructed as:
		//   (cpuftr_X86 | (eax << 16) | (ret_reg << 8) | (bit_position)
		// For example, SSE3 is determined by the fist bit in the ECX
		// register for a CPUID call with EAX=1, so we get:
		//   cpuftr_X86 | (1 << 16) | (2 << 8) | (0) = 0x01010200
		//
		// We should have information for inputs of EAX=0-7 w/ ECX=0.
		//
		cpuftr_X86_FPU     = 0x01010300,
		cpuftr_X86_VME     = 0x01010301,
		cpuftr_X86_DE      = 0x01010302,
		cpuftr_X86_PSE     = 0x01010303,
		cpuftr_X86_TSC     = 0x01010304,
		cpuftr_X86_MSR     = 0x01010305,
		cpuftr_X86_PAE     = 0x01010306,
		cpuftr_X86_MCE     = 0x01010307,
		cpuftr_X86_CX8     = 0x01010308,
		cpuftr_X86_APIC    = 0x01010309,
		cpuftr_X86_SEP     = 0x0101030b,
		cpuftr_X86_MTRR    = 0x0101030c,
		cpuftr_X86_PGE     = 0x0101030d,
		cpuftr_X86_MCA     = 0x0101030e,
		cpuftr_X86_CMOV    = 0x0101030f,
		cpuftr_X86_PAT     = 0x01010310,
		cpuftr_X86_PSE_36  = 0x01010311,
		cpuftr_X86_PSN     = 0x01010312,
		cpuftr_X86_CLFSH   = 0x01010313,
		cpuftr_X86_DS      = 0x01010314,
		cpuftr_X86_ACPI    = 0x01010316,
		cpuftr_X86_MMX     = 0x01010317,
		cpuftr_X86_FXSR    = 0x01010318,
		cpuftr_X86_SSE     = 0x01010319,
		cpuftr_X86_SSE2    = 0x0101031a,
		cpuftr_X86_SS      = 0x0101031b,
		cpuftr_X86_HTT     = 0x0101031c,
		cpuftr_X86_TM      = 0x0101031d,
		cpuftr_X86_IA64    = 0x0101031e,
		cpuftr_X86_PBE     = 0x0101031f,

		cpuftr_X86_SSE3    = 0x01010200,
		cpuftr_X86_PCLMULQDQ = 0x01010201,
		cpuftr_X86_DTES64  = 0x01010202,
		cpuftr_X86_MONITOR = 0x01010203,
		cpuftr_X86_DS_CPL  = 0x01010204,
		cpuftr_X86_VMX     = 0x01010205,
		cpuftr_X86_SMX     = 0x01010206,
		cpuftr_X86_EST     = 0x01010207,
		cpuftr_X86_TM2     = 0x01010208,
		cpuftr_X86_SSSE3   = 0x01010209,
		cpuftr_X86_CNXT_ID = 0x0101020a,
		cpuftr_X86_SDBG    = 0x0101020b,
		cpuftr_X86_FMA     = 0x0101020c,
		cpuftr_X86_CX16    = 0x0101020d,
		cpuftr_X86_XTPR    = 0x0101020e,
		cpuftr_X86_PDCM    = 0x0101020f,
		cpuftr_X86_PCID    = 0x01010211,
		cpuftr_X86_DCA     = 0x01010212,
		cpuftr_X86_SSE4_1  = 0x01010213,
		cpuftr_X86_SSE4_2  = 0x01010214,
		cpuftr_X86_X2APIC  = 0x01010215,
		cpuftr_X86_MOVBE   = 0x01010216,
		cpuftr_X86_POPCNT  = 0x01010217,
		cpuftr_X86_TSC_DEADLINE = 0x01010218,
		cpuftr_X86_AES     = 0x01010219,
		cpuftr_X86_XSAVE   = 0x0101021a,
		cpuftr_X86_OSXSAVE = 0x0101021b,
		cpuftr_X86_AVX     = 0x0101021c,
		cpuftr_X86_F16C    = 0x0101021d,
		cpuftr_X86_RDRND   = 0x0101021e,
		cpuftr_X86_HYPERVISOR = 0x0101021f,

		cpuftr_X86_FSGSBASE   = 0x01070100,
		cpuftr_X86_TSC_ADJ = 0x01070101,
		cpuftr_X86_SGX     = 0x01070102,
		cpuftr_X86_BMI1    = 0x01070103,
		cpuftr_X86_HLE     = 0x01070104,
		cpuftr_X86_AVX2    = 0x01070105,
		cpuftr_X86_SMEP    = 0x01070107,
		cpuftr_X86_BMI2    = 0x01070108,
		cpuftr_X86_ERMS    = 0x01070109,
		cpuftr_X86_INVPCID = 0x0107010a,
		cpuftr_X86_RTM     = 0x0107010b,
		cpuftr_X86_PQM     = 0x0107010c,
		cpuftr_X86_MPX     = 0x0107010e,
		cpuftr_X86_PQE     = 0x0107010f,
		cpuftr_X86_AVX512F = 0x01070110,
		cpuftr_X86_AVX512DQ = 0x01070111,
		cpuftr_X86_RDSEED   = 0x01070112,
		cpuftr_X86_ADX      = 0x01070113,
		cpuftr_X86_SMAP     = 0x01070114,
		cpuftr_X86_AVX512IFMA  = 0x01070115,
		cpuftr_X86_PCOMMIT     = 0x01070116,
		cpuftr_X86_CLFLUSHOPT  = 0x01070117,
		cpuftr_X86_CLWB        = 0x01070118,
		cpuftr_X86_INTEL_PT    = 0x01070119,
		cpuftr_X86_AVX512PF    = 0x0107011a,
		cpuftr_X86_AVX512ER    = 0x0107011b,
		cpuftr_X86_AVX512CD    = 0x0107011c,
		cpuftr_X86_SHA     = 0x0107011d,
		cpuftr_X86_AVX512BW    = 0x0107011e,
		cpuftr_X86_AVX512VL    = 0x0107011f,

		cpuftr_X86_PREFETCHWT1 = 0x01070200,
		cpuftr_X86_AVX512VBMI  = 0x01070201,
		cpuftr_X86_UMIP    = 0x01070202,
		cpuftr_X86_PKU     = 0x01070203,
		cpuftr_X86_OSPKE   = 0x01070204,
		cpuftr_X86_AVX512VPOPCNTDQ = 0x0107020e,
		cpuftr_X86_RDPID   = 0x01070215,
		cpuftr_X86_SGX_LC  = 0x0107021e,

		cpuftr_X86_AVX512_4VNNIW = 0x01070302,
		cpuftr_X86_AVX512_4FMAPS = 0x01070303,

		cpuftr_ARM_SWP     = cpuftr_ARM | 1,
		cpuftr_ARM_HALF    = cpuftr_ARM | 2,
		cpuftr_ARM_THUMB   = cpuftr_ARM | 3,
		cpuftr_ARM_26BIT   = cpuftr_ARM | 4,
		cpuftr_ARM_FAST_MULT = cpuftr_ARM | 5,
		cpuftr_ARM_FPA     = cpuftr_ARM | 6,
		cpuftr_ARM_VFP     = cpuftr_ARM | 7,
		cpuftr_ARM_EDSP    = cpuftr_ARM | 8,
		cpuftr_ARM_JAVA    = cpuftr_ARM | 9,
		cpuftr_ARM_IWMMXT  = cpuftr_ARM | 10,
		cpuftr_ARM_CRUNCH  = cpuftr_ARM | 11,
		cpuftr_ARM_THUMBEE = cpuftr_ARM | 12,
		cpuftr_ARM_NEON    = cpuftr_ARM | 13,
		cpuftr_ARM_VFPV3   = cpuftr_ARM | 14,
		cpuftr_ARM_VFPV3D16 = cpuftr_ARM | 15,
		cpuftr_ARM_TLS     = cpuftr_ARM | 16,
		cpuftr_ARM_VFPV4   = cpuftr_ARM | 17,
		cpuftr_ARM_IDIVA   = cpuftr_ARM | 18,
		cpuftr_ARM_IDIVT   = cpuftr_ARM | 19,
		cpuftr_ARM_VFPD32  = cpuftr_ARM | 20,
		cpuftr_ARM_LPAE    = cpuftr_ARM | 21,
		cpuftr_ARM_EVTSTRM = cpuftr_ARM | 22,

		cpuftr_ARM_AES     = cpuftr_ARM | 0x0100 | 1,
		cpuftr_ARM_PMULL   = cpuftr_ARM | 0x0100 | 2,
		cpuftr_ARM_SHA1    = cpuftr_ARM | 0x0100 | 3,
		cpuftr_ARM_SHA2    = cpuftr_ARM | 0x0100 | 4,
		cpuftr_ARM_CRC32   = cpuftr_ARM | 0x0100 | 5
	};
	enum {
		// @v11.3.11 (do use IsBigEndian member) fBigEndian    = 0x0001, // Процессор имеет архитектуру BIG ENDIAN (на текущий момент флаг всегда сброшен)
		fPerfFreqIsOk = 0x0002  // @v10.5.7
	};
	//
	// Descr: Версии сетевых протоколов SSL
	//
	enum {
		sslDefault = 0,
		sslTLS_v1x = 1,
		sslSSL_v2  = 2,
		sslSSL_v3  = 3,
		sslTLS_v10 = 4,
		sslTLS_v11 = 5,
		sslTLS_v12 = 6,
		sslTLS_v13 = 7
	};

	static int  BigEndian();
	static int  SGetModuleFileName(void * hModule, SString & rFileName);
	static uint SFormatMessage(int sysErrCode, SString & rMsg);
	static uint SFormatMessage(SString & rMsg);
	static char FASTCALL TranslateWmCharToAnsi(uintptr_t wparam);
	explicit SSystem(int imm);
	int    FASTCALL GetCpuInfo();
	int    CpuId(int feature, uint32 * pA, uint32 * pB, uint32 * pC, uint32 * pD) const;
	//
	// Descr: Возвращает значение системного таймера в микросекундах
	//
	int64  GetSystemTimestampMks() const;

	uint   IsBigEndian; // @firstmember 0xfffffff - big-endian arch, 0 - low-endian
	int64  PerfFreq;    // Frequency of the performance counter. Если !(Flags & fPerfFreqIsOk) то при инициализации PerfFreq произошла ошибка.
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
	uint32 CpuCount; // @v10.9.11
	uint32 PageSize; // @v10.9.11
};
//
// Descr: Тип функции, используемый для регистрации форматов файлов.
//
typedef int (*FileFormatSignatureFunc)(FILE * pF, long extraParam);
//
// Descr: Определители форматов файлов.
//
class SFileFormat { // @persistent @flat (новые зарезервированные форматы добавлять в конце списка перед FirstUser)
public:
	//
	// Descr: Типы кодировки контента (Content-Transfer-Encoding)
	//
	enum {
		cteUndef = 0,
		cte8bit,
		cte7bit,
		cteBinary,
		cteBase64,
		cteQuotedPrintable
	};
	//
	// Descr: MIME types (e.g. in application/pdf application is a type, pdf is a subtype)
	//
	enum {
		mtUndef = 0,
		mtMultipart,
		mtText,
		mtApplication,
		mtImage,
		mtMessage,
		mtVideo,
		mtAudio,
		mtFont,
		mtModel,
		mtExample
	};
	enum {
		Unkn     =  0,  //
		Txt      =  1,  //
		Jpeg     =  2,  //
		Png      =  3,  //
		Tiff     =  4,  //
		Gif      =  5,  //
		Bmp      =  6,  //
		Ico      =  7,  //
		Cur      =  8,  //
		Svg      =  9,  //
		Html     = 10,  //
		Xml      = 11,  //
		Ini      = 12,  //
		TxtBomUTF8       = 13,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF16BE    = 14,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF16LE    = 15,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF32BE    = 16,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF32LE    = 17,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF7       = 18,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF1       = 19,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomUTF_EBCDIC = 20,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomSCSU       = 21,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomBOCU1      = 22,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		TxtBomGB18030    = 23,  // Текстовый файл с сигнатурой типа кодировка (BOM)
		Latex    = 24,  // LATEX
		Pdf      = 25,  //
		Rtf      = 26,  //
		Mdb      = 27,  //
		AccDb    = 28,  // Access
		WbXml    = 29,  // Binary XML
		Wmf      = 30,  //
		Eps      = 31,  //
		Hlp      = 32,  //
		Ppd      = 33,  // PostScript
		PList    = 34,  // Property List
		Mat      = 35,  // Matlab
		Pdb      = 36,  //
		WcbffOld = 37,  // Windows Compound Binary File Format
		Zip      = 38,  // Archive
		Rar      = 39,  // Archive
		Gz       = 40,  // Archive
		Tar      = 41,  // Archive
		Bz2      = 42,  // Archive
		SevenZ   = 43,  // Archive
		Xz       = 44,  // Archive
		Z        = 45,  // Archive
		Cab      = 46,  // Archive
		Arj      = 47,  // Archive
		Lzh      = 48,  // Archive
		Xar      = 49,  // Archive
		Pmd      = 50,  // Archive
		Deb      = 51,  // Archive
		Rpm      = 52,  // Archive
		Chm      = 53,  // Archive
		Iso      = 54,  // Archive
		Vhd      = 55,  // Archive
		Wim      = 56,  // Archive
		Mdf      = 57,  // Archive
		Nri      = 58,  // Archive
		Swf      = 59,  // Archive
		Mar      = 60,  // Archive
		Mkv      = 61,  // Video
		Avi      = 62,  // Video
		Mp4      = 63,  // Video
		Wmv      = 64,  // Video
		Mpg      = 65,  // Video
		Flv      = 66,  // Video
		Mov      = 67,  // Video
		F4f      = 68,  // Video
		Class    = 69,  // binary:class 0:CAFEBABE
		Exe      = 70,  // binary:exe   0:4D5A
		Dll      = 71,  // binary:dll   0:4D5A
		Pcap     = 72,  // binary:pcap  0:D4C3B2A1
		Pyo      = 73,  // binary:pyo   0:03F30D0A
		So       = 74,  // binary:so    0:7F454C46
		Mo       = 75,  // binary:mo    0:DE120495
		Mui      = 76,  // binary:mui   0:50413330
		Cat      = 77,  // binary:cat   0:30 6:2A864886
		Xsb      = 78,  // binary:xsb   0:DA7ABABE
		Key      = 79,  // binary:key   0:4B4C737727
		Sq3      = 80,  // binary:sq3   0:53514C697465
		Qst      = 81,  // binary:qst   0:0401C4030000 binary:qst   0:040180040000
		Crx      = 82,  // binary:crx   0:43723234
		Utx      = 83,  // binary:utx   0:4C0069006E006500610067006500
		Rx3      = 84,  // binary:rx3   0:52583362
		Kdc      = 85,  // binary:kdc   0:44494646
		Xnb      = 86,  // binary:xnb   0:584E42
		Blp      = 87,  // binary:blp   0:424C5031 binary:blp   0:424C5032
		Big      = 88,  // binary:big   0:42494746
		Mdl      = 89,  // binary:mdl   0:49445354
		Spr      = 90,  // binary:spr   0:CDCC8C3F
		Sfo      = 91,  // binary:sfo   0:00505346
		Mpq      = 92,  // binary:mpq   0:4D50511A
		Nes      = 93,  // binary:nes   0:4E45531A
		Dmp      = 94,  // binary:dmp   0:4D444D5093A7
		Dex      = 95,  // binary:dex   0:6465780a30333500 binary:dex   0:6465780a30333600
		Gim      = 96,  // binary:gim   0:4D49472E30302E31505350
		Amxx     = 97,  // binary:amxx  0:58584D41
		Sln      = 98,  // Visual Studio Solution
		VCProj   = 99,  // Visual Studio Project
		Asm      = 100, // Assempbler source file
		C        = 101, // C source file
		CPP      = 102, // CPP source file
		H        = 103, // C header file
		Perl     = 104, // perl source code
		Php      = 105, // php source code
		Java     = 106, // java source code
		Py       = 107, // Python source code
		UnixShell        = 108, // Unix Shell script
		Msi      = 109, // Microsoft Installer package
		TxtUtf8  = 110, // Текст в формате utf8
		TxtAscii = 111, // Текст в котором только ASCII-символы
		Log      = 112, // Файл журнала (by ext only)
		Properties       = 113, // Файл properties. Обычно текстовый файл аналогичный ini, но без зональности ([]).
		Css      = 114, // CSS
		JavaScript       = 115, // JS
		Json     = 116, //
		Pbxproj  = 117, // Файл проекта xcode
		PapyruDbDivXchg  = 118, // Приватный формат проекта Papyrus: файл обмена данными между разделами
		Csv      = 119, // @v10.8.0 comma-separated-values
		Xsd      = 120, // @v10.8.0 xml-schema-documentation
		Gravity  = 121, // @v10.8.2 gravity-source (язык gravity)
		VCProjFilers     = 122, // @v10.9.9 Visual Studio Project Filters
		VCProjUser       = 123, // @v10.9.9 Visual Studio Project User
		CodeBlocks_Cbp   = 124, // @v10.9.9 Code::Blocks Project File
		M4               = 125, // @v10.9.9 m4 Macroprocessor
		Tsv              = 126, // @v10.9.9 tab-separated-values
		Webp             = 127, // @v11.3.4 webp graphics format
		Otf              = 128, // @v11.3.7 OpenType font file (4F 54 54 4F 00)
		Ttf              = 129, // @v11.3.7 TrueType font file (00 01 00 00 00)
		Ttc              = 130, // @v11.3.7 TrueType font Collection ttcf)
		Dfont            = 131, // @v11.3.7 Mac OS X Data Fork Font

		FirstUser        = 10000
	};
	static int FASTCALL Register(int id, int mimeType, const char * pMimeSubtype, const char * pExt, const char * pSign);
	static int FASTCALL Register(int id, const char * pExt, const char * pSign);
	static int Register();
	static int IdentifyContentTransferEnc(const char * pCte);
	static int IdentifyMimeType(const char * pMimeType);
	static int GetMimeTypeName(int mimeType, SString & rBuf);
	static int GetMime(int id, SString & rMime);
	static int GetExt(int id, SString & rExt);
	static int GetContentTransferEncName(int cte, SString & rBuf);

	SFileFormat();
	SFileFormat(int f); // no explicit
	operator int () const;
	int    operator !() const;
	//
	// Descr: Устанавливает значение формата в SFileFormat::Unkn
	// Note: Здесь нельзя применять семантику Z() из-за смешения с одноименным элементом перечисления.
	//
	void   Clear();
	int    GetMimeType() const;
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
	int    IdentifyBuffer(const void * pBuf, size_t bufLen);
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
size_t FASTCALL SGetUnicodeModeBomSize(SUnicodeMode m);
SEOLFormat FASTCALL SDetermineEOLFormat(const void * pBuf, size_t bufLen);

class STextEncodingStat {
public:
	//
	// Descr: Флаги состояния объекта
	//
	enum {
		fEmpty  = 0x0001,
		fAsciiOnly      = 0x0002,
		fLegalUtf8Only  = 0x0004,
		fMiscEolf       = 0x0008,
		fUseUCharDet    = 0x0010, // Декларативный флаг: предписывает использовать библиотеку uchardet для идентификации кодировки
		fUCharDetWorked = 0x0020, // Признак того, что модуль uchardet смог идентифицировать кодовую страницу текста
		fUseIcuCharDet  = 0x0040  // @v11.6.2 Использовать инструментарий ICU
	};
	//
	// Descr: @ctr
	// ARG(options IN): декларативные флаги. На текущий момент возможные значения: 0 или STextEncodingStat::fUseUCharDet
	//
	explicit STextEncodingStat(long options = 0);
	~STextEncodingStat();
	//
	// Descr: Инициализирует экземпляр объекта.
	// ARG(options IN): декларативные флаги. На текущий момент возможные значения: 0 или STextEncodingStat::fUseUCharDet
	// Returns: *this
	//
	STextEncodingStat & Init(long options = 0);
	int    Add(const void * pData, size_t size);
	void   Finish();
	SEOLFormat GetEolFormat() const { return Eolf; }
	SCodepageIdent GetCp() const { return Cp; }
	const char * GetCpName() const { return CpName; }
	bool   FASTCALL CheckFlag(long f) const { return LOGIC(Flags & f); }
	SCodepageIdent GetAutodetectedCp() const;
private:
	long   Flags;
	SCodepageIdent Cp;
	SEOLFormat Eolf;
	uint64 ChrFreq[256];
	char   CpName[32]; // Наименование кодировки, идентифицированное модулем uchardet.
		// Поле временное - будет существовать до тех пор, пока не адаптируем преобразование
		// этих наименований в SCodepageIdent
	int    LangID;

	SString EncDetectionBuf; // @v11.6.2
	uint8  Utf8Prefix[8];
	size_t Utf8PrefixLen;
	//void * P_UcdHandle; // uchardet handler
	//void * P_IcuUcdHandle; // @v11.6.2 ICU uchardetector handler
};
//
//
//
class SUnicodeTable : public SStrGroup {
public:
	enum {
		gcUnkn = 0,
		gcLu, // Uppercase_Letter 	an uppercase letter
		gcLl, // Lowercase_Letter 	a lowercase letter
		gcLt, // Titlecase_Letter 	a digraphic character, with first part uppercase
		gcLC, // Cased_Letter 	Lu | Ll | Lt
		gcLm, // Modifier_Letter 	a modifier letter
		gcLo, // Other_Letter 	other letters, including syllables and ideographs
		gcL,  // Letter 	Lu | Ll | Lt | Lm | Lo
		gcMn, // Nonspacing_Mark 	a nonspacing combining mark (zero advance width)
		gcMc, // Spacing_Mark 	a spacing combining mark (positive advance width)
		gcMe, // Enclosing_Mark 	an enclosing combining mark
		gcM,  // Mark 	Mn | Mc | Me
		gcNd, // Decimal_Number 	a decimal digit
		gcNl, // Letter_Number 	a letterlike numeric character
		gcNo, // Other_Number 	a numeric character of other type
		gcN,  // Number 	Nd | Nl | No
		gcPc, // Connector_Punctuation 	a connecting punctuation mark, like a tie
		gcPd, // Dash_Punctuation 	a dash or hyphen punctuation mark
		gcPs, // Open_Punctuation 	an opening punctuation mark (of a pair)
		gcPe, // Close_Punctuation 	a closing punctuation mark (of a pair)
		gcPi, // Initial_Punctuation 	an initial quotation mark
		gcPf, // Final_Punctuation 	a final quotation mark
		gcPo, // Other_Punctuation 	a punctuation mark of other type
		gcP,  // Punctuation 	Pc | Pd | Ps | Pe | Pi | Pf | Po
		gcSm, // Math_Symbol 	a symbol of mathematical use
		gcSc, // Currency_Symbol 	a currency sign
		gcSk, // Modifier_Symbol 	a non-letterlike modifier symbol
		gcSo, // Other_Symbol 	a symbol of other type
		gcS,  // Symbol 	Sm | Sc | Sk | So
		gcZs, // Space_Separator 	a space character (of various non-zero widths)
		gcZl, // Line_Separator 	U+2028 LINE SEPARATOR only
		gcZp, // Paragraph_Separator 	U+2029 PARAGRAPH SEPARATOR only
		gcZ,  // Separator 	Zs | Zl | Zp
		gcCc, // Control 	a C0 or C1 control code
		gcCf, // Format 	a format control character
		gcCs, // Surrogate 	a surrogate code point
		gcCo, // Private_Use 	a private-use character
		gcCn, // Unassigned 	a reserved unassigned code point or a noncharacter
		gcC,  // Other 	Cc | Cf | Cs | Co | Cn
	};
	enum {
		cccNot_Reordered        =   0, // Spacing and enclosing marks; also many vowel and consonant signs, even if nonspacing
		cccOverlay      =   1, // Marks which overlay a base letter or symbol
		cccNukta        =   7, // Diacritic nukta marks in Brahmi-derived scripts
		cccKana_Voicing =   8, // Hiragana/Katakana voicing marks
		cccVirama       =   9, // Viramas
		cccAttached_Below_Left  = 200, // Marks attached at the bottom left
		cccAttached_Below       = 202, // Marks attached directly below
		cccAttached_BottomRight = 204, // Marks attached at the bottom right
		cccAttached_Left        = 208, // Marks attached to the left
		cccAttached_Right       = 210, // Marks attached to the right
		cccAttached_TopLeft     = 212, // Marks attached at the top left
		cccAttached_Above       = 214, // Marks attached directly above
		cccAttached_Above_Right = 216, // Marks attached at the top right
		cccBelow_Left   = 218, // Distinct marks at the bottom left
		cccBelow        = 220, // Distinct marks directly below
		cccBelow_Right  = 222, // Distinct marks at the bottom right
		cccLeft         = 224, // Distinct marks to the left
		cccRight        = 226, // Distinct marks to the right
		cccAbove_Left   = 228, // Distinct marks at the top left
		cccAbove        = 230, // Distinct marks directly above
		cccAbove_Right  = 232, // Distinct marks at the top right
		cccDouble_Below = 233, // Distinct marks subtending two bases
		cccDouble_Above = 234, // Distinct marks extending above two bases
		cccIota_Subscript       = 240  // Greek iota subscript only
	};
	enum {
		bidiUnkn = 0,
		bidiL,   // Left_To_Right 	any strong left-to-right character
		bidiR,   // Right_To_Left 	any strong right-to-left (non-Arabic-type) character
		bidiAL,  // Arabic_Letter 	any strong right-to-left (Arabic-type) character
		bidiEN,  // European_Number 	any ASCII digit or Eastern Arabic-Indic digit
		bidiES,  // European_Separator 	plus and minus signs
		bidiET,  // European_Terminator 	a terminator in a numeric format context, includes currency signs
		bidiAN,  // Arabic_Number 	any Arabic-Indic digit
		bidiCS,  // Common_Separator 	commas, colons, and slashes
		bidiNSM, // Nonspacing_Mark 	any nonspacing mark
		bidiBN,  // Boundary_Neutral 	most format characters, control codes, or noncharacters
		bidiB,   // Paragraph_Separator 	various newline characters
		bidiS,   // Segment_Separator 	various segment-related control codes
		bidiWS,  // White_Space 	spaces
		bidiON,  // Other_Neutral 	most other symbols and punctuation marks
		bidiLRE, // Left_To_Right_Embedding 	U+202A: the LR embedding control
		bidiLRO, // Left_To_Right_Override 	U+202D: the LR override control
		bidiRLE, // Right_To_Left_Embedding 	U+202B: the RL embedding control
		bidiRLO, // Right_To_Left_Override 	U+202E: the RL override control
		bidiPDF, // Pop_Directional_Format 	U+202C: terminates an embedding or override control
		bidiLRI, // Left_To_Right_Isolate 	U+2066: the LR isolate control
		bidiRLI, // Right_To_Left_Isolate 	U+2067: the RL isolate control
		bidiFSI, // First_Strong_Isolate 	U+2068: the first strong isolate control
		bidiPDI  // Pop_Directional_Isolate 	U+2069: terminates an isolate control
	};
	enum {
		fBidiMirrored = 0x0001,
		fRange        = 0x0002
	};
	struct Item32 {
		Item32();
		uint32 U4;
		uint8  Gc;   // General category
		uint8  Ccc;  // Canonical Combining Class
		uint8  Bidi; // Bidi Class
        uint8  Flags;
		uint   DescrP;
		uint16 SNScriptId;
	};
	struct UPair {
		UPair(uint32 key, uint32 val);
		uint32 K;
		uint32 V;
	};

	static uint32 FASTCALL ParseUnicode(SString & rBuf);

	SUnicodeTable();
	uint32 FASTCALL GetU(uint idx) const;
	uint   FASTCALL GetIndex(uint32 u) const;
	const  Item32 * FASTCALL Get(uint32 u) const;

	int    ParseSource(const char * pFileName);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);

	uint32 FASTCALL ToLower(uint32 u) const;
	uint32 FASTCALL ToUpper(uint32 u) const;
	uint32 FASTCALL ToCapital(uint32 u) const;
private:
	int    PreprocessLine(SString & rLine) const;
	int    ParseDescription(SString & rDescr, SUnicodeTable::Item32 & rD);

	uint   LastIdx;
	TSVector <Item32> List32;
	TSVector <UPair> Ranges;
	TSVector <UPair> SimpleToUpAndCap;
	TSVector <UPair> SimpleToUp;
	TSVector <UPair> SimpleToCap;
	TSVector <UPair> SimpleToLo;
	TSVector <UPair> MirrorPairs;
};

class SCodepageMapPool : public SStrGroup {
private:
	struct CMapTranslIndexTest;
public:
	struct MapEntry {
		MapEntry();
		int    FASTCALL operator == (const MapEntry & rS) const;
		int    FASTCALL operator != (const MapEntry & rS) const;
		int    FASTCALL Cmp(const MapEntry & rS) const;

		uint8  B[4]; // от B[0] до B[3] в порядке следования символов в строке
		uint16 U2;
	};
	enum {
		fHas4U    = 0x0001, // Один или более элементов таблицы имеет unicode-символ шириной более 2 байт
		fSolid256 = 0x0002, // Таблица представляет собой сплошной (без разрывов) участок внутри 256-байтового диапазона.
			// Если таблица содержит менее 256 элементов, то поле Solid256Offs указывает на первый элемент.
		fHas3B    = 0x0004  // Один или более элементов таблицы имеет MB код, содержащий 3 байта
	};
	struct CpMap {
		CpMap();
		~CpMap();
		void   Clear();
		//
		// Descr: Ищет мультибайтовый символ b[] в таблице.
		//   Длина массива b[] не должна превышать 4 байт. Если
		//   в массиве менее 4 байт, то следующий за последним значащим байтом должен быть 0.
		//
		const  MapEntry * FASTCALL SearchC(const uint8 b[]) const; // Test - very slow func
		const  MapEntry * FASTCALL SearchU(wchar_t u, const LongArray * pIdx) const;
		const  MapEntry * FASTCALL SearchU(uint32 u) const; // Test - very slow func
		const  MapEntry * FASTCALL SearchFallback(uint32 u) const;
		int    TranslateToU(const uint8 * pSrc, size_t srcLen, SStringU & rBuf) const;
		//
		// Descr: Транслирует unicode-строку pSrc длиной srcLen в мультибайтовую строку rBuf
		//
		int    TranslateToB2(const wchar_t * pSrc, size_t srcLen, SString & rBuf);
		//
		// Descr: Testing function
		//
		const  MapEntry * Test_ToLower(const MapEntry * pSrc, const SUnicodeTable * pUt) const;
		const  MapEntry * Test_ToUpper(const MapEntry * pSrc, const SUnicodeTable * pUt) const;
		const  MapEntry * Test_ToCapital(const MapEntry * pSrc, const SUnicodeTable * pUt) const;

		int    MakeUIndex(LongArray & rIdx) const;

		SCodepage Id;
		uint8  MbMl;    // Максимальная длина мультибайтовых символов (в байтах)
		uint8  Solid256Offs;  // Если Flags & fSolid256, то это поле - смещение от нуля
		uint16 StraightCount; // Количество элементов с начала таблицы, порядковый номер которых совпадает с байтовым значением.
		uint16 CpSis;   //
		uint16 Flags;
		uint   MapCount;
		uint   FallbackCount;
		uint   NScriptCount;
		const  MapEntry * P_Map;
		const  MapEntry * P_Fallback;
		const  uint16 * P_NScript;
		SString Name;
		SString Code;
		SString Version;
	private:
		class CpUToBHash {
		public:
			CpUToBHash(const SCodepageMapPool::CpMap & rMap);
			int    Translate(const wchar_t * pSrc, size_t srcLen, SString & rResult) const;
		private:
			int    Helper_TranslateResultToBuf(const void * pResultBuf, uint resultBufSize, SString & rBuf) const;
			int    FillUpHash(const uint mapCount, const SCodepageMapPool::MapEntry * pMap, const uint entrySize, const uint hashSize, void * pHash);

			void * P_Buf;
			uint   Count;
			const  uint EntrySize;
		};
		int   FASTCALL Helper_BSearchB(uint32 b4, wchar_t * pU) const;
		CpUToBHash * P_U2B_Hash;
	};
	//
	// Descr: Временная структура для построения таблицы преобразования символьных таблиц
	//
	struct CMapTranslEntry {
		CMapTranslEntry();

		enum {
			fNone     = 0x0001,
			fFallback = 0x0002,
			fEqual    = 0x0004
		};
		uint8  S[4];
		uint8  D[4];
		uint8  F;
	};
	class TranslIndex {
	public:
		friend class SCodepageMapPool;

		TranslIndex();
		TranslIndex(const TranslIndex & rS);
		~TranslIndex();
		int    FASTCALL Copy(const TranslIndex & rS);
		TranslIndex & FASTCALL operator = (const TranslIndex & rS);
		const  uint8 * FASTCALL Search(const uint8 * pSrc) const;
	private:
		void   Reset();
		size_t GetEntrySize() const;
		int    Setup(const SCodepageMapPool::CMapTranslIndexTest & rS);

		enum {
			fIdentical = 0x0001,
			fEmpty     = 0x0002
		};
		void * P_Tab;
		uint32 Count; // Количество элементов в P_Tab
		uint8  SL;
		uint8  DL;
		uint16 IdenticalCount;
		uint32 Flags;
	};

	SCodepageMapPool();
	void   Clear();
	uint   GetCount() const;
	int    GetByPos(uint pos, CpMap * pM) const;
	int    Get(SCodepage cp, CpMap * pM) const;
	int    GetByName(const char * pName, CpMap * pM) const;
	//
	// Descr: Сравнивает две таблицы на идентичность.
	//
	uint   Compare(const CpMap & rS1, const CpMap & rS2) const;
	uint   Translate(const CpMap & rFrom, const CpMap & rTo, const uint8 * pSrc, size_t srcLen, SString & rDest);

	int    ParseXml(const char * pPath, SUnicodeTable * pUt);
	int    ParseXmlSingle(void * pXmlContext, const char * pFileName, const SUnicodeTable * pUt);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    Test(const SUnicodeTable * pUt, const char * pMapPoolFileName, const char * pMapTranslFileName);
	int    MakeTranslIndex(const CpMap & rFrom, const CpMap & rTo, TranslIndex & rIdx);
private:
	struct CpEntry {
		SCodepage Id;
		uint8  MbMl;          // Максимальная длина мультибайтовых символов (в байтах)
		uint8  Solid256Offs;  // Если Flags & fSolid256, то это поле - смещение от нуля
		uint16 StraightCount; // Количество элементов с начала таблицы, порядковый номер которых совпадает с байтовым значением.
		uint16 CpSis;         //
		uint16 Flags;         //
		uint32 NameP;         //
		uint32 CodeP;         //
		uint32 VersionP;      //
		uint32 MapStart;      // Начальная позиция в MeL
		uint32 MapCount;      // Количество позиций в MeL
		uint32 FallbackStart; // Начальная позиция в FbL
		uint32 FallbackCount; // Количество позиций в FbL
		uint32 NScriptStart;  // Начальная позиция списка натуральных скриптов в кодовой странице
		uint32 NScriptCount;  // Количество натуральных скриптов в кодовой странице
	};
	struct CMapTranslIndexTest : TSVector <CMapTranslEntry> {
		CMapTranslIndexTest();
		void   Reset();
		//
		// Descr: Сортирует элементы массива по возрастанию CMapTranslEntry::S
		//
		void   Sort();

		uint   MaxSLen;
		uint   MaxDLen;
		uint   IdenticalCount;
		uint   SuccCount;
		uint   FallbackCount;
	};
	int    ParseCpName(const SString & rName, int * pSis, SString & rCode, SString & rVersion) const;
	int    ParseSymbols(SString & rU, const SString & rMb, MapEntry & rEntry, uint8 * pMbMl) const;
	void   TranslateEntry(const CpEntry & rSrc, CpMap & rDest) const;
	int    SearchMapSeq(const TSVector <MapEntry> & rSeq, uint * pPos) const;
	int    Helper_MakeTranslIndex(const CpMap & rFrom, const CpMap & rTo, CMapTranslIndexTest & rIdx, TranslIndex & rFinalIdx);

	TSVector <CpEntry> CpL;  //
	TSVector <MapEntry> MeL; // Список соответствий multibyte-unicode. Отрезок каждой таблицы отсортирован по B[4]
	TSVector <MapEntry> FbL; // Список fallback-соответствий. Отрезок каждой таблицы отсортирован по U2
	TSVector <uint16> NScrL; // Список натуральных скриптов, присутствующих в кодовых страницах
};

class SUnicodeBlock {
public:
    SUnicodeBlock();
    ~SUnicodeBlock();

    int    ReadRaw(const char * pUnicodePath, const char * pCpPath);
    int    Read(const char * pFileName);
    int    Write(const char * pFileName);

	SUnicodeTable Ut;
	SCodepageMapPool Cpmp;
private:
    struct StrgHeader { // @size=32
        uint32 Signature; // UTCP = 0x50435455
        uint32 Version;
        uint32 Flags;
        uint32 CRC;       // CRC32 сериализованного буфера данных (заголовок не включен)
        uint8  Reserve[16]; // @reserve
    };
};
//
// Descr: Low level data alphabet analyzer
//
class SLldAlphabetAnalyzer {
public:
	struct Entry {
		Entry();
		void   Clear();

		uint64 C;
		uint64 LastP; // Последняя позиция потока, в которой встречался символ
		double P1; // Сумма периодов появления
		double P2; // Сумма квадратов периодов появления
	};
	SLldAlphabetAnalyzer();
	~SLldAlphabetAnalyzer();
	void   Clear();
	int    CollectFileData(const char * pFileName);
    int    FASTCALL AddSymb(uint8 s);

    uint64 GetCount() const;
	double FASTCALL GetFreq(uint8 s) const;
	double FASTCALL GetPeriodExp(uint8 s) const;
	double FASTCALL GetPeriodStdDev(uint8 s) const;
	//
	// Descr: Возвращает массив символов, ассоциированных с частотой появления,
	//   отсортированный в порядке уменьшения частоты.
	//
	int    FASTCALL GetFreqListOrdered(RAssocArray & rList) const;
private:
	TSVector <Entry> Alphabet;
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
//    *  Match preceding pattern zero or more times
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

class SRegExp2 { // @construction
public:
	enum {
		syntaxDefault = 0,
		syntaxAsis,
		syntaxPosixBasic,
		syntaxPosixExtended,
		syntaxEmacs,
		syntaxGrep,
		syntaxGnuRegex,
		syntaxJava,
		syntaxPerl,
		syntaxPerlNg,
		syntaxRuby,
		syntaxOniguruma
	};
	enum {
		fIgnoreCase                 = 0x00000001,
		fExtend           	   				= 0x00000002,
		fMultiLine        	   				= 0x00000004,
		fSingleLine       	   				= 0x00000008,
		fFindLongest      	   				= 0x00000010,
		fFindNotEmpty     	   				= 0x00000020,
		fNegateSingleLine 	   				= 0x00000040,
		fDontCaptureGroup 	   				= 0x00000080,
		fCaptureGroup          				= 0x00000100,
		// (search time)
		fNotBOL                				= 0x00000200,
		fNotEOL                				= 0x00000400,
		fPosixRegion           				= 0x00000800,
		fCheckValidityOfString 				= 0x00001000,
		fNotBeginString        				= 0x00002000,
		fNotEndString          				= 0x00004000,
		fNotBeginPosition      				= 0x00008000,
		// (compile time)
		fIgnoreCaseIsAscii     				= 0x00010000,
		fWordIsAscii           				= 0x00020000,
		fDigitIsAscii          				= 0x00040000,
		fSpaceIsAscii          				= 0x00080000,
		fPosixIsAscii          				= 0x00100000,
		fTextSegmentExtendedGraphemeCluster = 0x00200000,
		fTextSegmentWord            = 0x00400000,
	};
	struct FindResult : public TSVector <IntRange> {
		FindResult & Z();
	};
	struct Error {
		Error();
		Error & Z();
		int    Code;
		IntRange SrcTextR;
	};
	SRegExp2();
	explicit SRegExp2(const char * pPattern, SCodepage cp, int syntax, uint flags);
	~SRegExp2();
	SRegExp2 & Z();
	int    Compile(const char * pPattern, SCodepage cp, int syntax, uint flags); // Compiles char * --> regexp
	//
	// Descr: Ищет предварительно скомпилированный паттерн в тексте pText длиной textLen.
	// ARG(pText   IN): Указательн на строку, в которой необходимо найти скомпилированный образец
	// ARG(textLen IN): Длина текста в символах (без завершающего нуля)
	// ARG(flags   IN): Флаги поиска. Ноль или |-комбинация одного или нескольких флагов: fNotBOL, fNotEOL, fNotBeginString,
	//   fNotEndString, fNotBeginPosition.
	//
	bool   Find(const char * pText, size_t textLen, uint flags, FindResult * pResult); // TRUE if regexp in char * arg
	bool   Find(const char * pText);
	bool   Find(SStrScan * pScan, uint flags);
	bool   Find(SStrScan * pScan);
	Error  GetLastErr() const;
	bool   IsValid() const;
private:
	void * H;
	Error LastErr;
};

class CRegExp_Depricated {
public:
	CRegExp_Depricated();
	explicit CRegExp_Depricated(const char * s);
	CRegExp_Depricated(const CRegExp_Depricated &);
	~CRegExp_Depricated();
	int    Compile(const char * pPattern);      // Compiles char * --> regexp
	int    Find(const char * pText);            // TRUE if regexp in char * arg
	int    Find(SStrScan *);
	int    GetLastErr() const;
	bool   IsValid() const;
	//
	// Descr: Returns index into the searched string that corresponds to the  beginning pointer
	//
	long   start() const { return static_cast<long>(startp[0] - P_Text); }
	//
	// Descr: Returns index into the searched string that corresponds to the  end pointer
	//
	long   end() const { return static_cast<long>(endp[0] - P_Text); }
	int    FASTCALL operator == (const CRegExp_Depricated & r) const;
	int    FASTCALL operator != (const CRegExp_Depricated & r) const { return (!(*this == r)); }
	int    FASTCALL deep_equal(const CRegExp_Depricated & r) const; // Same regexp and state?
	void   set_invalid();
private:
	char * FASTCALL regatom(int * pFlag);   // @<<regpiece
	char * FASTCALL regpiece(int * pFlag);  // @<<regbranch
	char * FASTCALL regbranch(int * pFlag); // @<<reg
	char * reg(int paren, int * pFlag); // @<<compile,regatom
	char * FASTCALL regnode(char op);
	void   FASTCALL regc(char b);
	void   FASTCALL reginsert(char op, char * opnd);
	const  char * FASTCALL regnext(const char * p) const;
	char * FASTCALL regnext(char * p) const;
	void   FASTCALL regtail(char * p, const char * val);
	void   FASTCALL regoptail(char * p, const char * val);
	int    FASTCALL regrepeat(const char * p);
	int    regtry(const char * string, const char ** start, const char ** end, const char * prog);
	int    FASTCALL regmatch(const char * prog);

	const  char * startp[NSUBEXP];
	const  char * endp[NSUBEXP];
	int    ErrCode;           // Last error code (SLERR_XXX)
	char   RegStart;          // Internal use only
	char   RegAnch;           // Internal use only
	char   RegDummy;          //
	char   Reserve;           // @alignment
	const  char * P_RegMust;  // @*compile Internal use only
	const  char * P_RegParse; // @*compile Input-scan pointer.
	char * P_RegCode;         // @*compile Code-emit  * pointer;  * &RegDummy = don't.
	int    RegNPar;           // () count.
	size_t RegMLen;           // Internal use only
	size_t RegSize;           // Code size.
	char * P_Program;
	size_t ProgSize;          // Размер распределенного пространства под буфер P_Program
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
	int    InitRePhone(); // @v11.0.3
	/*
	CRegExp * P_ReQuotedStr;
	CRegExp * P_ReNumber;
	CRegExp * P_ReIdent;
	CRegExp * P_ReHex;
	CRegExp * P_ReDigits;
	CRegExp * P_ReEMail;
	CRegExp * P_ReDate;
	TSCollection <CRegExp> ReList; // Список регулярных выражений, регистрируемых вызывающим модулем.
	*/
	SRegExp2 * P_ReQuotedStr;
	SRegExp2 * P_ReNumber;
	SRegExp2 * P_ReIdent;
	SRegExp2 * P_ReHex;
	SRegExp2 * P_ReDigits;
	SRegExp2 * P_ReXDigits; // @v11.4.1
	SRegExp2 * P_ReEMail;
	SRegExp2 * P_ReDate;
	SRegExp2 * P_RePhone; // @v11.0.3
	TSCollection <SRegExp2> ReList; // Список регулярных выражений, регистрируемых вызывающим модулем.
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

	explicit SStrScan(const char * pBuf = 0, size_t offs = 0);
	~SStrScan();
	SStrScan & Z();
	void   FASTCALL Set(const char * pBuf, size_t offs);
	operator const char * () const { return P_Buf ? (P_Buf+Offs) : 0; }
	const char * GetBuf() const { return P_Buf; }
	const char * GetBuf(size_t offs) const { return P_Buf ? (P_Buf + offs) : 0; }
	size_t GetOffs() const { return Offs; }
	size_t GetLen() const { return Len; }
	void   FASTCALL Push(uint * pPrevPos = 0);
	int    FASTCALL Pop();
	int    FASTCALL Pop(uint prevPos);
	int    IsEnd() const;
	int    FASTCALL IsSpace(int ws) const;
	bool   IsNumber();
	bool   IsHex();
	//
	// Descr: сопоставляет текущий и, возможно, следующий символ с символом новой строки.
	//   Параметр eolf задает формат новой строки.
	//   Если eolf == eolUndef, то любая из трех комбинаций "\xD\xA" || "\xD" || "\xA" воспринимается
	//   как символ новой строки.
	// Returns:
	//   0  - текущий символ не является символом новой строки
	//   !0 - текущий символ и, возможно, последующий идентифицируют новую строку. Возвращаемое
	//     значение определяет количество символов, используемых для этого (1 или 2).
	//
	uint   FASTCALL IsEol(SEOLFormat eolf) const;
	//
	// Descr: Если текущий символ (или 2 символа) представляет перевод строки в согласии
	//   с форматом eolf, то функция смещает текущую позицию на соответствующее число
	//   символов и возвращает !0.
	// Returns:
	//   0  - текущий символ не является символом новой строки
	//   !0 - текущий символ и, возможно, последующий идентифицируют новую строку.
	//      Текущая позиция смещена на начало новой строки.
	//
	int    FASTCALL GetEol(SEOLFormat eolf);
	//
	// Descr: Возвращает в буфере rBuf строку, начинающуюся с текущего смещения и до первого встреченного
	//   символа (комбинации символов) перевода строки, либо до конца сканируемого текста.
	//   Формат перевода строки определяется параметром eolf.
	//   После выхода из функции текущее смещение Offs передвигается на символ, следующий за переводом каретки.
	//   Если отсканированная строка была последней (входной буфер завершился нулем), то текущее смещение
	//   окажется на завершающем нуле.
	//   Функция предварительно очищает содержимое переданного буфера rBuf.
	//   Символы перевода каретки в буфер результата rBuf не попадают.
	//   Специальный случай: если eolf == eolSpcICalendar, то символы перевода каретки плюс пробел трактуются
	//   как продолжение текущей строки.
	// Returns:
	//   >0 - идентифицирована строка, завершающаяся переводом каретки
	//   <0 - идентифицирована строка, завершающая сканируемый буфер
	//    0 - при входе в функцию текущая позиция была установлена на завершающем нуле - то есть, сканировать нечего.
	//
	int    FASTCALL GetLine(SEOLFormat eolf, SString & rBuf);
	//
	// Descr: Копирует в буфер rBuf участок сканируемого текста длиной this->Len, начиная с текущего смещения (this->Offs).
	//
	SString & FASTCALL Get(SString & rBuf) const;
	int    FASTCALL GetQuotedString(SString & rBuf);
	//
	// ARG(format IN): Формат данных, в соответствии с которым следует обрабатывать данные внутри кавычек.
	//   Принимаются следующие значения: SFileFormat::Json, SFileFormat::Csv, SFileFormat::Tsv, SFileFormat::Xml
	//
	int    GetQuotedString(SFileFormat format, SString & rBuf);
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
	int    FASTCALL GetXDigits(SString & rBuf); // @v11.4.1
	int    FASTCALL GetEMail(SString & rBuf);
	int    GetDate(long datefmt, LDATE & rDate);
	//
	// Descr: Если следующая последовательность символов равна pPattern (без учета регистра),
	//   то копирует ее в rBuf и перемещает текущий указатель в позицию, следующую за паттерном.
	// Returns:
	//   >0 - сработало обнаружение паттерна
	//   0  - паттерн не обнаружен в текущей позиции (ничего не изменилось)
	//
	int    Get(const char * pPattern, SString & rBuf);
	//
	// Descr: Сканирует все символы до тех пор пока не встретится один из разделителей,
	//   перечисленных в pDiv или конец строки.
	// Returns:
	//   >0 - если был сканирован по крайней мере один символ
	//   0  - очередной символ является разделителем или мы уперлись в конец строки.
	//
	int    GetWord(const char * pDiv, SString & rBuf);
	int    GetUntil(char divider, SString & rBuf);
	//
	// Descr: Если следующая последовательность равна: key = "value"
	//   то копирует key в rKey, value в rVal и перемещает указатель в позицию, следующую за "value"
	// Returns:
	//   >0 - сработало обнаружение конструкции
	//   0  - конструкция не обнаружена
	//
	int    GetEqQ(SString & rKey, SString & rVal);
	//
	// Descr: Если следующая последовательность равна: key = number
	//   то копирует key в rKey, number в rVal и перемещает указатель в позицию, следующую за "value"
	// Returns:
	//   >0 - сработало обнаружение конструкции
	//   0  - конструкция не обнаружена
	//
	int    GetEqN(SString & rKey, double & rVal);
	//
	// Descr: Если текущий символ вместе, возможно, с последующими, является допустимым UTF8 символом, то
	//   копирует все необходимые символы в буфер rBuf и возвращает количество скопированных символов.
	//   В противном случае возвращает 0.
	//   Текущая позиция перемещается вперед на возвращенное количество символов.
	// Note: Буфер rBuf предварительно очищается функцией.
	//
	int    FASTCALL GetUtf8(SString & rBuf);
	//
	// Descr: Устанавливает внутреннюю переменную Len, характеризующую длину
	//   форвардной распознанной конструкции.
	// Returns:
	//   Предыдущее знечение Len.
	//
	size_t FASTCALL SetLen(size_t newLen);
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
	SStrScan & FASTCALL Skip(int ws);
	SStrScan & Skip(int ws, uint * pLineCount);
	//
	// Descr: В большинстве случаев Skip(int) вызывается с параметром (wsSpace|wsTab) потому
	//   для оптимизации кода сделана эта функция.
	//
	SStrScan & Skip(); // wsSpace|wsTab
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
	//   член Len равным смещению между началом встретившейся подстроки и Offs.
	// Returns:
	//   0 - образец pPattern не найден: Len не изменяется.
	//   1 - образец pPattern найден
	//
	int    FASTCALL Search(const char * pPattern);
	//
	// Descr: Тоже, что и Search(const char *), но сравнивает образец без
	//   учета регистра в предположении, что pPattern состоит из ascii-символов.
	//
	// @construction int    FASTCALL SearchIAscii(const char * pPattern);
	//
	// Descr: Проверяет текущий символ на значение '<'.
	// Returns:
	//   !0 - текущий символ является началом ML-тега '<'
	//   0  - текущий символ не равен '<'
	//
	bool   FASTCALL IsTagBrace() const;
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
	bool   FASTCALL IsRe(long reHandler);
	int    GetRe(long reHandler, SString & rBuf);
	bool   FASTCALL Is(const char * pS) const;
	//
	// Descr: Проверяет, является ли следующим символом c.
	//
	int    FASTCALL Is(char c) const;
	bool   IsDigits();
	bool   IsDotPrefixedNumber();
	//
	// Descr: Проверяет, является ли текущий символ (P_Buf+Offs) вместе, возможно, со следующими
	//   за ним правильной комбинацией, представляющей UTF8 символ.
	//
	int    IsLegalUtf8() const; // return type must be int (not bool)!
	size_t Offs;
	size_t Len;
private:
	const  char * P_Buf;
private:
	TSStack <size_t> Stk;
};
//
//
//
struct ApproxStrSrchParam {
	ApproxStrSrchParam();
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
	BitArray Track1;
	BitArray Track2;
	ApproxStrSrchParam P;
};

int ApproxStrSrch(const char * pPattern, const char * pBuffer, ApproxStrSrchParam *);
int ApproxStrCmp(const char * pStr1, const char * pStr2, int noCase, double * pScore);
//
// Descr: Флаги функции ExtStrSrch
//
enum {
	essfCaseSensitive = 0x0001
};
//
// Descr: Ищет образец pPattern в строке pBuffer.
//
int FASTCALL ExtStrSrch(const char * pBuffer, const char * pPattern, uint flags);
//
// Descr: Идентификаторы распознаваемых натуральных токенов.
//   @persistent
//   Символьные обозначения токенов находятся в строке PPTXT_NATURALTOKENID
//
#define SNTOK_UNKN                       0
#define SNTOK_NATURALWORD                1
#define SNTOK_DIGITCODE                  2
#define SNTOK_EAN13                      3
#define SNTOK_EAN8                       4
#define SNTOK_UPCA                       5
#define SNTOK_UPCE                       6
#define SNTOK_RU_INN                     7
#define SNTOK_EGAISWARECODE              8 // Код алкогольной продукции ЕГАИС
#define SNTOK_EGAISMARKCODE              9 // Код акцизной марки алкогольной продукции ЕГАИС
#define SNTOK_LUHN                      10 // Цифровая последовательность с контрольной цифрой по алгоритму LUHN в конце
#define SNTOK_DIGLAT                    11 // Алфавитно-цифровая последовательность (содержащая только цифры и латинские символы)
#define SNTOK_GUID                      12 // Общепринятое текстовое представление GUID
#define SNTOK_EMAIL                     13 // Адрес электронной почты
#define SNTOK_PHONE                     14 // Номер телефона
#define SNTOK_IMEI                      15 // Код IMEI (с контрольной цифрой по алгоритму LUHN в конце)
#define SNTOK_IP4                       16 // ip4-address xx.xx.xx.xx
#define SNTOK_IP6                       17 // @todo ip6-address
#define SNTOK_MACADDR48                 18 // @todo MAC-address(48) xx-xx-xx-xx-xx-xx or xx:xx:xx:xx:xx:xx
#define SNTOK_DATE                      19 // date
#define SNTOK_TIME                      20 // time
#define SNTOK_SOFTWAREVER               21 // 9.9.9
#define SNTOK_COLORHEX                  22 // #hhhhhh
#define SNTOK_REALNUMBER                23 // 9.9 || .9 || 9. || 9,9
#define SNTOK_INTNUMBER                 24 // 9 || -9
#define SNTOK_PECENTAGE                 25 //
#define SNTOK_NUMERIC_DOT               26 // Десятичное число с десятичным разделителем точка и опциональными разделителями триад [,'`\x20]
	// Допускается лидирующий + или -. Экспоненциальная запись не допускается.
#define SNTOK_NUMERIC_COM               27 // Десятичное число с десятичным разделителем запятая и опциональными разделителями триад ['`\x20]
	// Допускается лидирующий + или -. Экспоненциальная запись не допускается.
	// Если число не имеет дробной части, то применяется SNTOK_NUMERIC_DOT
#define SNTOK_CHZN_GS1_GTIN             28 // честный знак Идентификационный номер GS1 для идентификации товаров regexp: "[0-9]{14}"
#define SNTOK_CHZN_SIGN_SGTIN           29 // честный знак Индивидуальный серийный номер вторичной упаковки regexp: "[0-9]{14}[&#x21;-&#x22;&#x25;-&#x2F;&#x30;-&#x39;&#x41;-&#x5A;&#x5F;&#x61;-&#x7A;]{13}"
#define SNTOK_CHZN_SSCC                 30 // честный знак Индивидуальный серийный номер третичной/транспортной упаковки regexp: "[0-9]{18}"
#define SNTOK_CHZN_CIGITEM              31 // честный знак Маркировка пачки сигарет
#define SNTOK_CHZN_CIGBLOCK             32 // честный знак Маркировка блока сигарет
#define SNTOK_RU_OKPO                   33 // @v10.8.1 Российский код ОКПО
#define SNTOK_RU_SNILS                  34 // @v10.8.1 Российский код СНИЛС
#define SNTOK_RU_BIC                    35 // @v10.8.1 Российский банковский идентификационный код (БИК)
#define SNTOK_RU_KPP                    36 // @v10.8.2 Российский код причины постановки на налоговый учет (КПП)
#define SNTOK_LINGUACODE                37 // @v11.3.12 Код естественного языка
#define SNTOK_CHZN_SURROGATE_GTIN       38 // честный знак Специальная суррогатная конструкция, не являющаяся маркой (02 GTIN14)
#define SNTOK_CHZN_SURROGATE_GTINCOUNT  39 // честный знак Специальная суррогатная конструкция, не являющаяся маркой (02 GTIN14 37 COUNT)
#define SNTOK_CL_RUT                    40 // @v11.6.1 Чилийский налоговый код RUT

#define SNTOKSEQ_DEC        0x00000001 // 0-9
#define SNTOKSEQ_LATLWR     0x00000002 // a-z
#define SNTOKSEQ_LATUPR     0x00000004 // A-Z
#define SNTOKSEQ_LAT        0x00000008 // A-Z||a-z
#define SNTOKSEQ_HEX        0x00000010 // 0-9||A-F||a-f
#define SNTOKSEQ_ASCII      0x00000020 // 1-127
#define SNTOKSEQ_UTF8       0x00000040 // Вся последовательность состоит из валидных UTF-8 символов
#define SNTOKSEQ_1251       0x00000080 // Вся последовательность состоит из символов, соответствующих кодировке cp1251
#define SNTOKSEQ_866        0x00000100 // Вся последовательность состоит из символов, соответствующих кодировке cp866
#define SNTOKSEQ_DECLAT     0x00000200 // 0-9||A-F||a-f
#define SNTOKSEQ_HEXHYPHEN  0x00000400 // ТОЛЬКО HEX && -
#define SNTOKSEQ_DECHYPHEN  0x00000800 // ТОЛЬКО DEC && -
#define SNTOKSEQ_HEXCOLON   0x00001000 // ТОЛЬКО HEX && :
#define SNTOKSEQ_DECCOLON   0x00002000 // ТОЛЬКО DEC && :
#define SNTOKSEQ_HEXDOT     0x00004000 // ТОЛЬКО HEX && .
#define SNTOKSEQ_DECDOT     0x00008000 // ТОЛЬКО DEC && .
#define SNTOKSEQ_DECSLASH   0x00010000 // ТОЛЬКО DEC && /
#define SNTOKSEQ_LEADSHARP  0x00020000 // Лидирующий символ # (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
#define SNTOKSEQ_LEADMINUS  0x00040000 // Лидирующий символ - (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
#define SNTOKSEQ_LEADDOLLAR 0x00080000 // Лидирующий символ $ (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
#define SNTOKSEQ_BACKPCT    0x00100000 // Последовательность завершается символом % (без отключения остальных флагов)
#define SNTOKSEQ_NUMERIC    0x00200000 // @v10.2.5 Многовариантное представление числа [+\-0-9.,' ]
	// -9,999.99  +99'999,9  9 999.999  999
#define SNTOKSEQ_LATHYPHENORUSCORE  0x00400000 // @v11.3.12 ТОЛЬКО LAT && (- || _)

struct SNaturalTokenStat {
	SNaturalTokenStat();
	SNaturalTokenStat & Z();
    uint32 Len;  // Длина токена в байтах
    uint32 Seq;
};

struct SNaturalToken { // @flat
	uint32 ID;
	float  Prob;
};

class SNaturalTokenArray : public TSVector <SNaturalToken> {
public:
	SNaturalTokenArray & Z();
	float  FASTCALL Has(uint32 tok) const;
	int    Add(uint32 tok, float prob);
};

class STokenRecognizer : public SRegExpSet {
public:
	static int EncodeChZn1162(uint16 productTypeBytes, const char * pGTIN, const char * pSerial, void * pResultBuf, size_t resultBufSize);
	STokenRecognizer();
	virtual ~STokenRecognizer();
	//
	// Descr: Функция пытается идентифицировать natural token в строке pToken длиной len.
	//   Если len < 0, то просматривается sstrlen(pToken) символов
	//
	int    Run(const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat);
protected:
	struct ImplementBlock {
		enum {
			fUtf8     = 0x0001, // Весь токен состоит из валидных UTF8-символов
			fPhoneSet = 0x0002, // Токен по набору символов может быть истолкован как номер телефона (но требуются дополнительные проверки)
			fClRut    = 0x0004  // @v11.6.0 Токен по набору символов может быть истолкован как чилийский налоговый идентификатор RUT (требуются дополнительные проверки)
		};
		ImplementBlock();
		ImplementBlock & Z();
		void   Init(const uchar * pToken, int tokenLen /* (<0) - strlen(pToken) */);
		long   F;
		uint   DecCount; // Количество десятичных цифр
		SNaturalTokenStat Stat;
		SString Temp;
		LAssocArray ChrList;
	};
	int    Implement(ImplementBlock & rIb, const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat);
	//
	// Descr: Виртуальная функция PostImplement вызывается после Implement с целью реализации дополнительных
	//   проверок, которые отсутствуют в базовой реализации Implement.
	// ARG(rIb IN/OUT): инициализированный методом Implement блок, содержащий значительное число важных параметров
	//   исследуемого токена.
	//
	virtual int PostImplement(ImplementBlock & rIb, const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat);
private:
	static int FASTCALL IsUtf8(const uchar * p, size_t restLen);
};
//
// Descr: wrapper-class for interface with libphonenumber
//
class SLibPhoneNumber {
public:
	SLibPhoneNumber();
	~SLibPhoneNumber();
	int    Parse(const char * pText, const char * pDefaultRegion);
private:
	void * H; // handle
	void * P; // processor
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
		crmCrNew  = (2 << 6),
		crmOpenAllways    = (3 << 6),
		crmOpenExist      = (4 << 6),
		crmTrancateExist  = (5 << 6)
	};

	enum {
		mAccsRead = 0x00000001,
		mAccsWrite        = 0x00000002,
		mAccsExec = 0x00000004,
		mShrRead  = 0x00000008,
		mShrWrite = 0x00000010,
		mShrDelete        = 0x00000020,
		mCreationMask     = 0x00000040|0x00000080|0x00000100,
		mAttrArch = 0x00000200,
		mAttrEncrypt      = 0x00000400,
		mAttrHidden       = 0x00000800,
		mAttrNormal       = 0x00001000,
		mAttrNoIndex      = 0x00002000,
		mAttrOffline      = 0x00004000,
		mAttrRO   = 0x00008000,
		mAttrSys  = 0x00010000,
		mAttrTemp = 0x00020000,
		mFBackupSemantics = 0x00040000,
		mFPosixSemantics  = 0x00080000,
		mFDelOnClose      = 0x00100000,
		mFNoBuff  = 0x00200000,
		mFOpenNoRecall    = 0x00400000,
		mFOpenReparsePt   = 0x00800000,
		mFOverlapped      = 0x01000000,
		mFRandomAccs      = 0x02000000,
		mFSeqScan = 0x04000000,
		mFWriteThrough    = 0x08000000
	};
	enum {
		mSecAnonym        = 0x00000001,
		mSecCtxTracking   = 0x00000002,
		mSecDelegation    = 0x00000004,
		mSecEffOnly       = 0x00000008,
		mSecIdent = 0x00000010,
		mSecImpersonation = 0x00000020
	};

	static void * SCreateFile(int ioType, const char * pName, uint32 mode, uint32 secMode = 0);
};
//
//
//
struct SFileSortProgressData {
	SFileSortProgressData();
	const  char * P_SrcFileName;
	int64  TotalFileSize;
	int    Phase;            // 1 - splitting, 2 - merging
	int64  SplitBytesRead;   // Количество байт считанных на фазе сплиттинга
	int    SplitThreadCount; // Количество потоков, осуществляющих сплиттинг
	uint   MergeChunkCount;  // Начальное количество блоков для слияния
	uint   MergeChunkLeft;   // Оставшееся число не слитых блоков
	const void * ExtraPtr;
};

typedef int (*SFileSortProgressCbProc)(const SFileSortProgressData *);

class SFile {
public:
	DECL_INVARIANT_C();

	static int FASTCALL Remove(const char * pFileName);
	static int Rename(const char * pFileName, const char * pNewFileName);
	static int Compare(const char * pFileName1, const char * pFileName2, long flags);
	static int FASTCALL WildcardMatch(const char * pPattern, const char * pStr);
	//
	// Descr: Структура дополнительных параметров сортировки файла
	//
	struct SortParam {
		SortParam();

        size_t MaxChunkSize;  // @def=(8*1024*1024)
        uint   MaxChunkCount; // @def=8
		uint   MaxThread;     // @def=0
        SFileSortProgressCbProc ProgressCbProc; // @def=0
		const void * ProgressCbExtraPtr; // @def=0
	};

	static int Sort(const char * pSrcFileName, const char * pOutFileName, CompFunc fcmp, SortParam * pExtraParam);
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
	static int GetTime(const char * pFileName, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif);
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
	static int GetTime(int fh, LDATETIME * pCreation, LDATETIME * pLastAccess, LDATETIME * pLastModif);
	static int SetTime(int fh, const LDATETIME * pCreation, const LDATETIME * pLastAccess, const LDATETIME * pLastModif);
	static int IdentifyFormat(const char * pFileName);
	static int IsOpenedForWriting(const char * pFileName);
	//
	// Descr: Ожидает момента, когда файл не открыт для записи.
	// Returns:
	//   <0 - файл не открыт для записи
	//   >0 - время в миллисекундах, в течении которого пришлось ждать освобождения файла
	//    0 - истек предельный timeout ожидания //
	//
	static int WaitForWriteSharingRelease(const char * pFileName, long timeout);
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
			// Если размер файла может превышать 2G, то использовать этот флаг обязательно!
		mNullWrite = 0x1000, // Файл открывается в режиме нулевого вывода - то есть запись в него ничего не делает, а чтение возвращает ошибку
		mBuffRd    = 0x2000  // При установленном mNoStd чтение из файла буферизуется.
	};
	SFile();
	SFile(const char * pName, long mode);
	SFile(SBuffer & rBuf, long mode);
	~SFile();
	bool   IsValid() const;
	operator FILE * ();
	operator SBuffer * ();
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
	int    GetBuffer(SBaseBuffer & rBuf) const;
	int    FileNo() const;
	const  SString & GetName() const;
	long   GetMode() const;
	int    Open(const char * pName, long mode);
	int    Open(SBuffer & rBuf, long mode);
	int    OpenNullOutput();
	int    Close();
	//
	// Descr: Устанавливает 32-битное значение текущей позиции в файле.
	// Note: При буферизованном чтении вызов функции сбрасывает буфер.
	// Returns:
	//   true - success
	//   false - error
	//
	bool   Seek(long offs, int origin = SEEK_SET);
	//
	// Descr: Устанавливает 64-битное значение текущей позиции в файле.
	// Attention: Если файл открыт в стандартном режиме (без флага fNoStd) то
	//   фактически применяется 32-битное значение позиции (младшие четыре байта offs).
	// Note: При буферизованном чтении вызов функции сбрасывает буфер.
	// Returns:
	//   true - success
	//   false - error
	//
	bool   Seek64(int64 offs, int origin = SEEK_SET);
	//
	// Descr: Возвращает значение текущей позиции в файле.
	//
	long   Tell();
	//
	// Descr: Возвращает 64-битное значение текущей позиции в файле.
	// Attention: В версии VC 7.1 если файл открыт в стандартном режиме (без флага fNoStd) то
	//   фактически возвращается 32-битное значение позиции. Это связано с отсутствием в той версии 
	//   Visual C функции _ftelli64().
	//
	int64  Tell64();
	int    Flush();
	int    Write(const void * pBuf, size_t size);
	//
	// Descr: Пытается считать size байт в буфер pBuf.
	//   Действительное количество считанных байт присваивается по адресу pActualSize
	//   (если pActualSize != 0).
	//
	int    Read(void * pBuf, size_t size, size_t * pActualSize = 0);
	//
	// Descr: То же, что и SFile::Read() но с проверкой равенства запрошенного
	//   размера считанному. Если считанное количество байт не равно запрошенному,
	//   то возвращает 0 и устанавливает код ошибки SLERR_SFILRDSIZE.
	//
	int    ReadV(void * pBuf, size_t size);
	int    FASTCALL WriteLine(const char *);
	int    WriteBlancLine();
	//
	// Descr: Флаги функции ReadLine
	//
	enum {
		rlfChomp = 0x0001, // Удалять завершающий строку перевод каретки (если присутствует)
		rlfStrip = 0x0002  // Удалять ведущие и завершающие пробелы если таковые присутствуют.
			// Если задан флаг rlfChomp, то вызов rBuf.Strip() осуществляется после rBuf.Chomp().
	};

	int    ReadLine(SString & rBuf, uint flags);
	int    FASTCALL ReadLine(SString & rBuf);

	class ReadLineCsvContext {
	public:
		friend class SFile;

		ReadLineCsvContext(char fieldDivider);
	private:
		const  char FieldDivider;
		uint8  Reserve[3]; // @alignment
		SString LineBuf;
		SStrScan Scan;
	};
	//
	// Descr: Высокоуровневая функция, считывающая строку текстового файла в предположении, что этот файл имеет csv-формат.
	//   Результат чтения заносится в экземляр StringSet.
	//
	int    ReadLineCsv(ReadLineCsvContext & rCtx, StringSet & rSs);
	//
	// Descr: Считывает все доступное содержание файла, начиная с текущей позиции, в буфер rBuf.
	//   Если maxSize != 0, то считывает не более maxSize. В случае успеха по указателю pActualSize
	//   возвращает количество считанных байт.
	// Returns:
	//   >0 - функция завершилась успешно
	//    0 - ошибка
	//   <0 - функция ничего не считала (*pActualSize == 0). Например из-за того, что файл пустой или текущая позиция файла уже в самом конце его.
	//
	int    ReadAll(STempBuffer & rBuf, size_t maxSize, size_t * pActualSize);
	//
	// Descr: Высокоуровневая функция записи буфера rBuf в поток.
	//   Сначала записывает размер буфера, затем - данные.
	// Returns:
	//   >0 - буфер записан успешно
	//   0  - ошибка
	//
	int    Write(const SBuffer & rBuf);
	//
	// Descr: Высокоуровневая функция чтения буфера rBuf из потока.
	//   Сначала считывает размер буфера, распределяет необходимый размер в буфере и считывает данные.
	// Returns:
	//   >0 - буфер считан успешно
	//   0  - ошибка
	//
	int    Read(SBuffer & rBuf);
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
	int    _Lock(int64 offs, int32 size, int mode);
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
	int    Lock(int64 offs, int32 size);
	//
	// Descr: Снимает блокировку, установленную перед этим функцией Lock(int64, int)
	//
	int    Unlock(int lckHandle);
	//
	// Descr: Рассчитывает размер файла. Размер определяется перемещением текущей позиции в конец файла
	//   и определением после этого текущей позиции, которая и является размером.
	//   После подсчета восстанавливает исходную текущую позицию файла.
	// Returns:
	//   true  - размер файла успешно вычислен
	//   false - ошибка
	//
	bool   CalcSize(int64 * pSize);
	//
	// Descr: Рассчитывает хэш-функцию, заданную алгоритмом hashFunc по файлу, начиная со смещения offs.
	//
	bool   CalcHash(int64 offs, int hashFunc/* SHASHF_XXX */, SBinaryChunk & rHash);
	int    GetDateTime(LDATETIME * pCreate, LDATETIME * pLastAccess, LDATETIME * pModif);
	//
	// Descr: Рассчитывает CRC32 файла, начиная со смещения offs.
	//   После подсчета восстанавливает исходную текущую позицию файла.
	// Returns:
	//   >0 - CRC32 успешно рассчитан
	//   <0 - размер файла меньше offs
	//   0  - ошибка
	//
	int    CalcCRC(long offs, uint32 * pCrc);
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
	void   Init();

	int    T;
	long   Mode;
	union {
		FILE * F;
		SBuffer * P_Sb;
	};
	int    IH;
	SString Name;
	SBuffer BufR; // Буфер чтения ((Mode&mNoStd)&&(Mode&mBuffRd))
	STempBuffer LB; // Временный буфер для чтения строк и т.д.
	//
	// Таблица дескрипторов блокировок. Таблица в течении жизни объекта никогда не уменьшается в размере.
	// Дескриптором блокировки является индекс позиции блока LckChunk в массиве, увеличенный на 1.
	// При снятии блокировки обнуляется значение Size что свидетельствует о том, что элемент свободен
	// для использования другой блокировкой.
	//
	TSVector <SFile::LckChunk> LckList;
};
//
// Descr: Незамысловатый класс, управляющий хранением большого числа файлов с жестко определенными наименованиями.
//   Файлы храняться в подкаталогах базовой директории BasePath. Директория разбивается на BucketCount подкаталогов,
//   место нахождение файла в конкретном подкаталоге определяется хэш-функцией от наименования файла.
//
class SFileStorage {
public:
	explicit SFileStorage(const char * pBasePath);
	~SFileStorage();
	bool   IsValid() const { return !(State & stError); }
	bool   operator !() const { return !IsValid(); }
	bool   Init(const char * pBasePath);
	SHandle Write_Start(const char * pName);
	bool   Write(SHandle handle, const void * pBuf, size_t size);
	bool   Write_End(SHandle handle, uint64 * pWrittenSize);
	int    PutFile(const char * pName, const void * pBuf, size_t size);
	int    PutFile(const char * pName, const char * pSourceFilePath);
	SHandle GetFile(const char * pName, int64 * pFileSize);
	bool   Read(SHandle handle, void * pBuf, size_t bufSize, size_t * pActualSize);
	//
	// Descr: Возвращает общий размер считанных с помощью манипулятора handle байт.
	//   Если манипулятор открыт для записи, то возвращает 0.
	//
	uint64 GetTotalRdSize(SHandle handle) const;
	//
	// Descr: Возвращает общий размер записанных с помощью манипулятора handle байт.
	//   Если манипулятор открыт для чтения, то возвращает 0.
	//
	uint64 GetTotalWrSize(SHandle handle) const;
	//
	// Descr: Закрывает файл, открытый либо функцией Write_Start() либо GetFile()
	//
	bool   CloseFile(SHandle handle);
private:
	bool   ValidateName(const char * pName) const;
	int    MakeFileEntry(const char * pName, SString & rEntryName, bool writing);

	enum {
		stError  = 0x0001,
		stInited = 0x0002
	};

	const  uint BucketCount;
	const  uint HashSeed;
	uint   State;
	SString BasePath;
	struct InnerWritingBlock {
		InnerWritingBlock();
		SFile  F;
		uint64 TotalWrSize;
	};
	struct InnerReadingBlock {
		InnerReadingBlock();
		SFile  F;
		int64  FileSize;
		uint64 TotalRdSize;
	};
	TSCollection <InnerWritingBlock> WrBlkList;
	TSCollection <InnerReadingBlock> RdBlkList;
};
//
//
//
class SCompressor {
public:
	enum {
		tUnkn = 0,
		tZLib = 1,
		tLz4  = 2
	};
	explicit SCompressor(int type);
	~SCompressor();
	int    SetMaxTempBufSize(size_t sz);
	size_t GetMaxTempBufSize() const { return MaxTempBufSize; }
	//
	// Descr: Сжимает данные из буфера pSrc размером srcSize в буфер rDest.
	// ARG(pSrc    IN): Указатель на исходные данные для сжатия
	// ARG(srcSize IN): Размер исходных данных для сжатия в байтах
	// ARG(rDest  OUT): Буфер, в который записывается результат сжатия. Буфер не сбрасывается и запись ведется начиная с текущей позиции.
	// ARG(rate    IN): Синтетический фактор плотности сжатия [1..100]. 0 - по умолчанию, 1 - минимальное сжатие, 100 - максимальное сжатие.
	// ARG(pExt    IN): Дополнительный блок данных, специфичный для конкретного алгоритма. Спецификация функции гарантирует,
	//   что любой алгоритм будет адекватно работать при передаче нулевого указателя данным аргументом.
	// Returns:
	//   >0 - успех. Значение показывает количество байт, записанных в буфер rDest этим вызовом.
	//    0 - ошибка
	//
	int    CompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest, int rate, const void * pExt);
	int    DecompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest);
	//void * CompressIterStart(void * pDest, size_t destSize, const void * pExt);
	//int    CompressIterNext(void * pCtx, const void * pSrc, size_t srcSize, void * pDest, size_t * destSize);
	//int    CompressIterEnd(void * pDest, size_t destSize, const void * pExt);

	enum {
		estszMax = 0x01, // Функция GetEstimatedXXXSize определила максимальный размер результата
		estszNr  = 0x02, // Функция GetEstimatedXXXSize определила ближайший размер результата
	};
private:
	const  int Type;
	size_t MaxTempBufSize; // @construction Максимальный размер внутреннего временного буфера для упаковки/распаковки.
	// @construction STempBuffer TBuf;
	void * P_Ctx;
};

class SArchive {
public:
	//
	// Descr: Базовые форматы архивов
	//
	enum {
		afCPIO = 1,
		afSHAR,
		afTAR,
		afISO9660,
		afZIP,
		afAR,
		afAR_GNU,
		afMTREE,
		afRAW,
		afXAR,
		afLHA,
		afCAB,
		afRAR,
		af7ZIP,
		afWARC
	};
	enum {
		tUnkn = 0,
		tZip  = 1, // LibZIP
		tBz2  = 2,
		tLA   = 3  // @v10.4.4 LibArchive
		//t7Z // @construction
	};
	//
	// Descr: Дескриптор формата архива
	//
	struct Format {
		int    BaseAf; // SArchive::afXXX
		int    Sis;    // ssisXXX
		int    Var;    // Вариация
	};
	explicit SArchive(int type);
	~SArchive();
	int    Open(const char * pName, int mode/*SFile::mXXX*/, SArchive::Format * pFmt);
	int    Close();
	bool   IsValid() const;
	//
	// Descr: Возвращает количество файлов в архиве
	//
	int64  GetEntriesCount() const;
	//
	// Descr: Возвращает имя файла в архиве с индексом idx
	//
    int    FASTCALL GetEntryName(int64 idx, SString & rName);
    //
    // Descr: Извлекает из архива файл с индексом idx и копирует его в pDestName
    //
    int    ExtractEntry(int64 idx, const char * pDestName);

    enum {
    	aefRecursive = 0x0001, // Для AddEntries: рекурсивно добавлять файлы из подкаталогов
    	aefDirectory = 0x0002  // Для AddEntry: добавляемый элемент является каталогом
    };
	//
	// Descr: Добавляет в архив новый элемент, соответствующий файлу pSrcFileName с именем pName.
	//   Если pName == 0 || pName[0] == 0, то имя нового элемента соответствует имени файла без пути
	//   в строчном регистре.
	//
    int    AddEntry(const char * pSrcFileName, const char * pName, int flags);
    int    AddEntries(const char * pMask, int flags);
private:
	// @v10.4.4
#if 1 // @construction {
	struct LaCbBlock {
		LaCbBlock(SArchive * pMaster, size_t bufSize);
		SArchive * P_Master;
		SFile F;
		STempBuffer Buf;
	};
	static SSIZE_T cdecl LaCbRead(Archive * pA, void * pClientData, const void ** ppBuf);
	static int64   cdecl LaCbSkip(Archive * pA, void * pClientData, int64 request);
	static int     cdecl LaCbOpen(Archive * pA, void * pClientData);
	static int     cdecl LaCbClose(Archive * pA, void * pClientData);
#endif // } 0 @construction
	// } @v10.4.4
	int    Helper_AddEntries(const SString & rRoot, const SString & rSub, const SString & rMask, int flags);

	const  int Type;
	void * H;
	LaCbBlock * P_Cb_Blk; // @v10.4.4
};
//
// JSON (Rui Maciel rui.maciel@gmail.com)
//
struct RcString;
struct JsonParsingBlock;
//
// The error messages produced by the JSON parsers
//
enum json_error {
	JSON_OK = 1,              // everything went smoothly
	JSON_INCOMPLETE_DOCUMENT, // the parsed document didn't ended
	JSON_WAITING_FOR_EOF,     // A complete JSON document tree was already finished but needs to get to EOF. Other characters beyond whitespaces produce errors
	JSON_MALFORMED_DOCUMENT,  // the JSON document which was fed to this parser is malformed
	JSON_INCOMPATIBLE_TYPE,   // the currently parsed type does not belong here
	JSON_MEMORY,              // an error occurred when allocating memory
	JSON_ILLEGAL_CHARACTER,   // the currently parsed character does not belong here
	JSON_BAD_TREE_STRUCTURE,  // the document tree structure is malformed
	JSON_MAXIMUM_LENGTH,      // the parsed string reached the maximum allowed size
	JSON_UNKNOWN_PROBLEM,     // some random, unaccounted problem occurred
	JSON_EMPTY_DOCUMENT,      // @v10.8.0 the parsed document is empty
};
//
// The JSON document tree node, which is a basic JSON type
//
struct SJson {
	//
	// Descr: The descriptions of the json_value node type
	//
	enum {
		tUnkn = 0,
		tSTRING = 1,
		tNUMBER,
		tOBJECT,
		tARRAY,
		tTRUE,
		tFALSE,
		tNULL
	};
	static SJson * CreateObj(); // { return new SJson(tOBJECT); }
	static SJson * CreateArr(); // { return new SJson(tARRAY); }
	static SJson * FASTCALL Parse(const char * pText);
	static bool FASTCALL IsObject(const SJson * pN) { return (pN && pN->Type == SJson::tOBJECT); }
	static bool FASTCALL IsArray(const SJson * pN) { return (pN && pN->Type == SJson::tARRAY); }
	static bool FASTCALL IsString(const SJson * pN) { return (pN && pN->Type == SJson::tSTRING); }
	static bool FASTCALL IsNumber(const SJson * pN) { return (pN && pN->Type == SJson::tNUMBER); }
	static bool FASTCALL IsNull(const SJson * pN) { return (pN && pN->Type == SJson::tNULL); }
	static bool FASTCALL IsTrue(const SJson * pN) { return (pN && pN->Type == SJson::tTRUE); }
	static bool FASTCALL IsFalse(const SJson * pN) { return (pN && pN->Type == SJson::tFALSE); }
	//
	// Descr: Если pN != 0 и имеет одно из логических значений SJson::tTRUE или SJson::tFALSE,
	//   то функция возвращает, соответственно, 1 или 0. Если pN == 0 или не является логической величиной,
	//   то возвращает -1.
	//
	static int  FASTCALL GetBoolean(const SJson * pN);
	//
	// Descr: Возвращает строку, в которой экранированные с помощью символа '\' знаки заменены
	//   на свои оригинальные значения.
	// Note: В случае, если фактически замена была осущетсвлена, то возвращается ссылка на
	//   револьверную строку (SLS.AcquireRvlStr()) по этому, результат должен быть использован как можно скорее.
	//
	static const SString & FASTCALL Unescape(const SString & rRawText);
	explicit SJson(int aType);
	~SJson();
	bool   IsValid() const;
	bool   IsObject() const { return (Type == SJson::tOBJECT); }
	bool   IsArray() const { return (Type == SJson::tARRAY); }
	bool   IsString() const { return (Type == SJson::tSTRING); }
	bool   IsNumber() const { return (Type == SJson::tNUMBER); }
	bool   IsNull() const { return (Type == SJson::tNULL); }
	bool   IsTrue() const { return (Type == SJson::tTRUE); }
	bool   IsFalse() const { return (Type == SJson::tFALSE); }
	const SJson * FASTCALL FindChildByKey(const char * pKey) const;
	int    FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL ToStr(SStringU & rBuf) const;
	void   FASTCALL AssignText(const SString & rT);
	int    Insert(const char * pTextLabel, SJson * pValue);
	//
	// Descr: То же, что и Insert(const char * pTextLabel, SJson * pValue), но ничего не делает, если pValue == 0
	// Returns:
	//   >0 - success
	//    0 - error
	//   <0 - pValue == 0
	//
	int    InsertNz(const char * pTextLabel, SJson * pValue);
	int    InsertString(const char * pTextLabel, const char * pStr);
	//
	// Descr: Вставляет в json-объект строку pStr с меткой pTextLabel при условии, что !isempty(pStr)
	// Returns:
	//   >0 - success
	//   <0 - строка pStr пустая
	//    0 - error
	//
	int    InsertStringNe(const char * pTextLabel, const char * pStr);
	int    InsertNumber(const char * pTextLabel, const char * pStr);
	int    InsertDouble(const char * pTextLabel, double val, long fmt);
	int    InsertInt(const char * pTextLabel, int val);
	int    InsertIntNz(const char * pTextLabel, int val);
	int    InsertInt64(const char * pTextLabel, int64 val);
	int    InsertBool(const char * pTextLabel, bool val);
	int    FASTCALL InsertNull(const char * pTextLabel);
	int    FASTCALL InsertChild(SJson * pChild);

	int    Type; // the type of node
	SString Text; // The text stored by the node. It stores UTF-8 strings and is used exclusively by the SJson::tSTRING and JSON_NUMBER node types
	//
	// FIFO queue data
	//
	SJson * P_Next;      // The pointer pointing to the next element in the FIFO sibling list
	SJson * P_Previous;  // The pointer pointing to the previous element in the FIFO sibling list
	SJson * P_Parent;    // The pointer pointing to the parent node in the document tree
	SJson * P_Child;     // The pointer pointing to the first child node in the document tree
	SJson * P_ChildEnd;  // The pointer pointing to the last child node in the document tree
private:
	uint   State;
};
//
// The structure which holds the pointers to the functions that will be called by the saxy parser whenever their evens are triggered
//
struct json_saxy_functions {
	int (*open_object)();
	int (*close_object)();
	int (*open_array)();
	int (*close_array)();
	int (*new_string)(char *text);
	int (*new_number)(char *text);
	int (*new_true)();
	int (*new_false)();
	int (*new_null)();
	int (*label_value_separator)();
	int (*sibling_separator)();
};
//
// The structure holding the information needed for json_saxy_parse to resume parsing
//
struct json_saxy_parser_status {
	json_saxy_parser_status();
	int    StoreCharInTempString(char c);
	void   FreeTempString();
	uint   State; // current parser state
	int    StringLengthLimitReached; // flag informing if the string limit length defined by JSON_MAX_STRING_LENGTH was reached
	RcString * P_Temp; // temporary string which will be used to build up parsed strings between parser runs.
};
//
// Buils a SJson document by parsing an open file
// @param file a pointer to an object controlling a stream, returned by fopen()
// @param document a reference to a SJson pointer, set to NULL, which will store the parsed document
// @return a json_error error code according to how the parsing operation went.
//
enum json_error json_stream_parse(FILE * file, SJson ** document);
//
// Creates a new JSON string and defines it's text
// @param text the value's text
// @return a pointer to the newly created JSON string value
//
SJson * FASTCALL json_new_string(const char * text);
//
// Creates a new JSON number and defines it's text. The user is responsible for the number string's correctness
// @param text the value's number
// @return a pointer to the newly created JSON string value
//
SJson * FASTCALL json_new_number(const char * text);
SJson * json_new_null();
//
// Frees the memory appointed to the value fed as the parameter, as well as all the child nodes
// @param value the root node of the tree being freed
//
void FASTCALL json_free_value(SJson ** value);
//
// Inserts a child node into a parent node, as well as performs some document tree integrity checks.
// @param parent the parent node
// @param child the node being added as a child to parent
// @return /*the error code corresponding to the operation result*/
//
/*enum json_error*/int FASTCALL json_insert_child(SJson * parent, SJson * child);
//
// Produces a JSON markup text document from a document tree
// @param root The document's root node
// @param text a pointer to a char string that will hold the JSON document text.
// @return  a json_error code describing how the operation went
//
//enum   json_error json_tree_to_string(SJson * root, char **text);
// @v11.2.7 (replaced with SJson::ToStr) int    FASTCALL json_tree_to_string(const SJson * pRoot, SString & rBuf);
//
// Produces a JSON markup text document from a SJson document tree to a text stream
// @param file a opened file stream
// @param root The document's root node
// @return  a json_error code describing how the operation went
//
enum json_error json_stream_output(/*FILE * file,*/SJson * root, SString & rBuf);
//
// Strips all JSON white spaces from the text string
// @param text a char string holding a JSON document or document snippet
//
void json_strip_white_spaces(char *text);
//
// Formats a JSON markup text contained in the given string
// @param text a JSON formatted document
// @return a pointer to a char string holding the formated document
//
int json_format_string(const char * pText, SString & rBuf);
//
// Produces a document tree sequentially from a JSON markup text fragment
// @param info the information necessary to resume parsing any incomplete document
// @param buffer a null-terminated c-string containing a JSON document fragment
// @return /*a code describing how the operation ended up*/
//
int FASTCALL json_parse_fragment(JsonParsingBlock & rBlk, const char * pBuffer);
//
// Produces a document tree from a JSON markup text string that contains a complete document
// @param root a reference to a pointer to a SJson type. The function allocates memory to the passed pointer and sets up the value
// @param text a c-string containing a complete JSON text document
// @return a pointer to the new document tree or NULL if some error occurred
//
enum json_error json_parse_document(SJson ** ppRoot, const char * text);
//
// Function to perform a SAX-like parsing of any JSON document or document fragment that is passed to it
// @param jsps a structure holding the status information of the current parser
// @param jsf a structure holding the function pointers to the event functions
// @param c the character to be parsed
// @return a json_error code informing how the parsing went
//
enum json_error json_saxy_parse(json_saxy_parser_status * jsps, json_saxy_functions * jsf, char c);
//
// Searches through the object's children for a label holding the text text_label
// @param object a json_value of type SJson::tOBJECT
// @param text_label the c-string to search for through the object's child labels
// @return a pointer to the first label holding a text equal to text_label or NULL if there is no such label or if object has no children
//
SJson *json_find_first_label(const SJson * object, const char *text_label);
//
// Helper
//
const char * json_get_value(const SJson *object, const char *text_label);
SJson * json_process(SJson *object);
//
// Mathematics {
//
class SMathConst {
public:
	static const double Pi;
	static const double Pi2;        // pi*2
	static const float  Pi2_f;      // pi*2
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
	static const double Min;     // 2.2250738585072014e-308          min double value
	static const double Max;     // 1.7976931348623158e+308          max double value
	static const float  Min_f;   // 1.175494351e-38F          min normalized positive float value
	static const float  Max_f;   // 3.402823466e+38F          max float value
	static const double Euler;   // 0.57721566490153286060651209008  Euler constant
	static const double Sqrt2;   // 1.41421356237309504880168872421      sqrt(2)
	static const double SqrtPi;  // 1.77245385090551602729816748334      sqrt(pi)
	static const double Sqrt1_2; // 0.707106781186547524400844362104849039 sqrt(0.5)
};

struct SMathResult {
	operator double() const { return V; }
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
typedef TSVector <unsigned long> UlongArray;
// @v10.9.1 {
#if !defined(INFINITY)
	#define INFINITY HUGE_VAL
#endif
// } @v10.9.1
bool   IsValidIEEE(double);
int    FASTCALL fisnan(double);
int    fisnanf(float);
int    fisinf(double);
double fgetnan();    // NAN
float  fgetnanf();    // NAN
double fgetposinf(); // +INF
double fgetneginf(); // -INF
//
// Descr: Раскладывает число val на простые множители.
//   Список простых множителей заносится в массив pList.
//   Сейчас используется самый наивный алгоритм перебора.
//
int    Factorize(ulong val, UlongArray * pList);
int    MutualReducePrimeMultiplicators(UlongArray & rA1, UlongArray & rA2, UlongArray * pReduceList);
long   GetTranspositionNumber(const long list[], long count);
int    GetNthTransposition(long list[], long count, long n);
//
//
// bits: 
//   64 = 16+48 (16 - descriptor, 48 - value)
//   48:
//       1 - fractional or floating decimal 
//       floating decimal:
//          5 - signed exponent
//         42 - signed mantissa
//       ??? fractional:
//          1 - sign
//         24 - nominator
//         23 - denominator  
//
class SDecimalFraction {
public:
	SDecimalFraction();
	explicit SDecimalFraction(double v);
	SDecimalFraction(int64 mantissa, int16 exp);
	double GetReal() const;
	bool   Normalize();
	SDecimalFraction & Z();
	bool   IsZero() const;
	static SDecimalFraction Neg(const SDecimalFraction & rV);
	int    Add(const SDecimalFraction & rA, const SDecimalFraction & rB);
	int    Sub(const SDecimalFraction & rA, const SDecimalFraction & rB);
	int    Mul(const SDecimalFraction & rA, const SDecimalFraction & rB);
	int    Div(const SDecimalFraction & rA, const SDecimalFraction & rB);
	static int Test();
private:
	int64  Mant;
	int16  Exp;
	uint16 Flags; // SIEEE754::fXXX
};

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
	SHistogram();
	~SHistogram();
	void   Setup();
	void   SetupDynamic(double leftEdge, double step);
	void   SetupDev(int even, double widthSigm, uint binCount = 0);
	int    AddBin(long binId, double lowBound);
	int    GetBin(long binId, Val *) const;
	int    GetTotal(Val *) const;
	int    GetBinByVal(double val, long * pBinId) const;
	//
	// Descr: Реализует первый проход добавления элемента в гистограмму с признаком kDeviation.
	//
	int    PreparePut(double val);
	long   Put(double val);
	const  StatBase * GetDeviationStat() const;
	int    GetDeviationParams(double * pMean, double * pWidth) const;
	uint   GetResultCount() const;
	int    GetResult(uint pos, Result * pResult) const;
private:
	enum {
		fDynBins   = 0x0001,
		fDeviation = 0x0002,
		fEven      = 0x0004  // Четная гистограмма отклонений о среднего.
	};
	long   Flags;
	RAssocArray BinList;
	TSVector <SHistogram::Val> ValList;
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
	friend class     LMatrix;

	enum {
		fNotOwner = 0x0001
	};
	LVect();
	explicit LVect(LMIDX dim);
	~LVect();
	int    init(LMIDX dim, const double * pVals = 0);
	int    FASTCALL copy(const LVect &);
	//const  char * getname() const { return P_Name; }
	//void   FASTCALL setname(const char * pName);
	LMIDX  size() const { return Dim; }
	double FASTCALL get(LMIDX) const;
	int    set(LMIDX, double);
	void   FillWithSequence(double startVal, double incr);
	int    zero(LMIDX = -1);
	void   mult(double val);           // this *= val
	void   div(double val);            // this /= val
	int    add(const LVect & s);       // this += s
	double dot(const LVect & s) const; // return this * s (scalar)
	void   saxpy(double a, const LVect & y); // this = this * a + y
	friend LVect   * FASTCALL operator * (const LMatrix &, const LVect &);
	friend LMatrix * FASTCALL operator * (const LMatrix &, const LMatrix &);
	friend LVect   * FASTCALL operator * (const LVect &, const LMatrix &);
	friend LMatrix * FASTCALL operator * (const LVect &, const LVect &);
private:
	//char * P_Name;
	LMIDX  Dim;
	double * P_Vals;
};

class LMatrix {
public:
	LMatrix();
	~LMatrix();
	int    init(LMIDX numRows, LMIDX numCols);
	int    FASTCALL copy(const LMatrix &);
	const  char * getname() const { return P_Name; }
	void   FASTCALL setname(const char * pName);
	int    checktarget(LMIDX row, LMIDX col) const;
	LMIDX  rows() const { return NumRows; }
	LMIDX  cols() const { return NumCols; }
	double get(LMIDX row, LMIDX col) const;
	LVect * getrow(LMIDX row) const;
	LVect * getcol(LMIDX col) const;
	int    set(LMIDX row, LMIDX col, double);
	int    setrow(LMIDX row, const LVect &);
	int    setcol(LMIDX col, const LVect &);
	void   zero(LMIDX row, LMIDX col);
	void   swaprows(LMIDX r1, LMIDX r2);
	int    transpose(const LMatrix &);
	double Jacobian() const;
	int    FASTCALL operator += (const LMatrix &);
	int    FASTCALL operator -= (const LMatrix &);
	friend LVect   * FASTCALL operator * (const LMatrix &, const LVect &);
	friend LMatrix * FASTCALL operator * (const LMatrix &, const LMatrix &);
	friend LVect   * FASTCALL operator * (const LVect &, const LMatrix &);
	friend LMatrix * FASTCALL operator * (const LVect &, const LVect &);
private:
	double * sget(LMIDX, LMIDX) const;
	int    add(const LMatrix &, int minus);
	size_t dim() const { return static_cast<size_t>(NumRows * NumCols); }

	char * P_Name;
	LMIDX  NumRows;
	LMIDX  NumCols;
	uint   Flags;
	double * P_Vals; // Column(0), Column(1), .., Column(NumCols-1)
};

void   lubksb(LMatrix & a, const LMIDX indx[/*b.size()*/], LVect & b);
int    ludcmp(LMatrix & a, LMIDX indx[/*a.rows()*/], int & d);
void   print(const LVect &, FILE *, long fmt);
void   print(const LMatrix &, FILE *, long fmt);
int    read(LVect *, FILE *);
int    read(LMatrix *, FILE *);

LVect   * FASTCALL operator * (const LMatrix &, const LVect &);
LMatrix * FASTCALL operator * (const LMatrix &, const LMatrix &);
//
// Descr: Product of row-vect[m] and matrix[m, n] : row-vect[n]
//
LVect   * FASTCALL operator * (const LVect &, const LMatrix &);
//
// Descr: Product of column-vect[n] and row-vect[m] : matrix[n, m]
//
LMatrix * FASTCALL operator * (const LVect &, const LVect &);
//
//
//
LVect * gaxpy(const LMatrix & a, const LVect & x, const LVect & y);
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
	LMatrix2D & InitTranslate(SPoint2F p);
	LMatrix2D & InitRotate(double teta);
	LMatrix2D & InitRotateDeg(double tetaDeg);
	LMatrix2D & InitSkew(double angleX, double angleY);
	LMatrix2D & InitSkewDeg(double angleXDeg, double angleYDeg);
	LMatrix2D & FASTCALL operator = (double s);
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
	SPoint2R & FASTCALL TransformDistance(SPoint2R & rP) const;
	SPoint2R & FASTCALL Transform(SPoint2R & rP) const;

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
	LMatrix3D();
	LMatrix3D & FASTCALL operator = (double s);
	LMatrix3D & FASTCALL operator = (const LMatrix3D & rS);
	LMatrix3D & InitUnit(double v);
	LMatrix3D & InitScale(double x, double y, double z);
	LMatrix3D & InitRotateX(double teta);
	LMatrix3D & InitRotateY(double teta);
	LMatrix3D & InitRotateZ(double teta);
	LMatrix3D & Mult(const LMatrix3D & rM1, const LMatrix3D & rM2);
public:
	double M[4][4];
};

class StatBase {
public:
	enum {
		fExt  = 0x0001, // Рассчитывать дополнительные статистики
		fGammaTest    = 0x0002, // Выполнить тест на гамма-распределение
		fGaussianTest = 0x0004, // @unimplemented Выполнить тест на нормальное распределение
		fStoreVals    = 0x0008  // Сохранять значения в Series
	};
	explicit StatBase(long flags);
	StatBase();
	virtual ~StatBase();
	virtual int Init(long flags);
	virtual int Finish();
	void   Step(double);
	void   Step(const RealArray & rVl);
	long   GetCount() const { return Count; }
	double GetMin() const { return Min; }
	double GetMax() const { return Max; }
	double GetExp() const { return Exp; }
	double GetVar() const { return Var; }
	double GetTestGamma() const { return Test_Z; }
	double GetTestGaussian() const { return Test_ChSq; }
	double GetSum() const;
	double GetVariance() const;
	double GetStdDev() const;
	int    GetValue(long idx, double * pVal) const;
	void   GetGammaParams(double * pAlpha, double * pBeta) const;
	const  RealArray & GetSeries() const { return Series; }
protected:
	long   Flags;
	long   IterCount;
	long   Count;
	double Min;
	double Max;
	double Sum[4]; // Sum[0] - линейная сумма, Sum[1] - сумма квадратов, Sum[2] - сумма кубов, Sum[3] - сумма четвертых степеней
	double Exp;    // Математическое ожидание
	double Var;    // Дисперсия //
	double Kurtosis; // if(Flags & fExt) ((Sum4 / Count) / pow(SumQ / Count, 2)) - 3.0;
	double Skew;     // if(Flags & fExt) (Sum3 / Count) / pow(SumQ / Count, 1.5)
	double Test_Z;
	double Test_ChSq;
	RealArray Series; // Если (Flags & (fGammaTest|fGaussianTest|fStoreVals)), то все поступающие значения сохраняются в этом массиве
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
	TimSerStat();
	virtual ~TimSerStat();
	virtual int Init(long flags);
	virtual int Finish();
	int    SetNumLags(long);
	int    Step(double, int whiteSpace = 0);
	uint   GetNumLags() const;
	double GetAutocorrel(uint lag) const;
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
	TimSerSpikes();
	~TimSerSpikes();

	int    Init(int sign /* > 0 - plus only, < 0 - minus only, 0 - both */);
	int    Step(long, double);
	long   GetNumSpikes() const;
	double GetSpike(long, long * pN) const;
	long   GetMostCommonDistance();
private:
	struct Spike { // @flat
		long   N;
		double V;
	};
	int    Sign;
	long   Count;
	Spike  Prev[2];
	SVector * P_Spikes;
};

class ARMA {
public:
	ARMA();
	~ARMA();
	void   Init(int p, int q, const double * pInitTSeries /* p items */);
	void   Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix *);
	int    Step(double);
	double Predict() const;
	int    GetModel(LVect *) const;
	int    GetPhi(LVect *) const;
	int    GetR(LMatrix *) const;
	void   GetPQ(int *, int *) const;
private:
	double ComputePredictError(double) const;
	int    ComputeGainFactor(LVect *) const;

	int    P;
	int    Q;
	long   IterCount;
	LVect  Phi;        // [P+Q+1] vect
	LMatrix R;         // [P+Q+1] x [P+Q+1] matrix
	LVect  Model;      // [P+Q+1] vect
};
//
// Descr: Новый 'хорошо продуманный :)' вариант представления временных рядов.
//   Для представления времени применена новая структура SUniTime, а значения
//   выражаются отдельными векторами с целью достижения гибкости и оптимизации
//   времени доступа и хранимого объема.
//
class STimeSeries {
public:
	STimeSeries();
	STimeSeries(const STimeSeries & rS);
	~STimeSeries();
	STimeSeries & FASTCALL operator = (const STimeSeries & rS);
	int    FASTCALL Copy(const STimeSeries & rS);
	void   FASTCALL SetSymb(const char * pSymb);
	const char * FASTCALL GetSymb() const;
	//
	// Descr: Удаляет данные из экземпляра, но спецификацию (колонки и проч.) оставляет не изменной.
	//   Данные удаляются методом clear() (то есть без освобождения памяти)
	//
	STimeSeries & Z();
	//
	// Descr: Полностью разрушает экземпляр (удаляет и данные и спецификацию).
	//   Вся память, занятая под данные, возвращается в кучу.
	//
	void   Destroy();
	//
	// Descr: Копирует спецификацию экземпляра (но не данные) из pSample.
	//   Если pSample == 0, то экземпляр this просто разрушается (вызовом Destroy())
	// Note: Функция исходит из предположения, что pSample (если не нулевой) полностью валиден.
	//
	int    SetupBySample(const STimeSeries * pSample);
	//
	// Descr: Возвращает количество элементов в серии (фактически, в STimeSeries::T)
	//
	uint   GetCount() const;
	int    SearchEntry(const SUniTime & rUt, uint startPos, uint * pIdx) const;
	//
	// Descr: Ищет первый элемент, время которого больше или равно rSince.
	// Note: Смысл (но не формальное поведение) функции предполагает, что элементы отсортированы
	//   по мере возрастания времени. Если это не так, то функция отработает, однако
	//   нет никакой гарантии, что элементы с индексами большими, чем *pIdx, имеют большее
	//   значение времени, чем rSince.
	// Returns:
	//   >0 - искомый элемент найден. По адресу pIdx присвоен его индекс.
	//    0 - нет ни одного элемента, время которого было бы больше или равно rSince
	//
	int    SearchFirstEntrySince(const SUniTime & rSince, uint * pIdx) const;
	int    SearchEntryReverse(const SUniTime & rUt, uint startPos, uint * pIdx) const;
	int    SearchEntryBinary(const SUniTime & rUt, uint * pIdx) const;
	int    FASTCALL GetTime(uint itemIdx, SUniTime * pT) const;
	int    AddValueVec(const char * pSymb, TYPEID typ, int fxPrec, uint * pVecIdx);
	uint   GetValueVecCount() const;
	int    GetValueVecParam(uint vecIdx, TYPEID * pTyp, SString * pSymb, int * pFxPrec, uint * pFlags) const;
	int    GetValueVecIndex(const char * pSymb, uint * pIdx) const;
    int    GetValue(uint itemIdx, uint vecIdx, double * pValue) const;
	int    GetValue(uint itemIdx, uint vecIdx, float * pValue) const;
	int    GetValue(uint itemIdx, uint vecIdx, int64 * pValue) const;
	int    GetValue(uint itemIdx, uint vecIdx, int32 * pValue) const;
	//
	// Descr: Извлекает из вектора с индексом vecIdx вектор значений размерности rV.size(), начиная с индекса startItemIdx.
	// Returns:
	//   >0 - функция выполнена успешно
	//   <0 - не нашлось достаточное количество элементов, начиная с startItemIdx
	//   0  - ошибка
	//
	int    GetLVect(uint vecIdx, uint startItemIdx, LVect & rV) const;
	int    GetRealArray(uint vecIdx, uint startItemIdx, uint idxCount, RealArray & rV) const;
	int    GetTimeArray(uint startItemIdx, uint idxCount, DateTimeArray & rV) const;
    int    AddItem(SUniTime tm, uint * pItemIdx);
	int    AddItemFromSample(const STimeSeries & rSample, uint samplePos);
	int    RemoveItem(uint idx);
	//
	// Descr: Структура, предназначенная для хранения одного элемента серии
	//   с целью обеспечения интерфеса.
	//
	struct SnapshotEntry {
		SnapshotEntry();
		SnapshotEntry & Z();

		uint   Idx;
		SUniTime Tm;
		RAssocArray Values;
	};
	int    GetSnapshotEntry(uint idx, SnapshotEntry & rEntry) const;
	int    SetSnapshotEntry(uint idx, const SnapshotEntry & rEntry);
	int    AppendSnapshotEntry(const SnapshotEntry & rEntry);

	struct AppendStat {
		AppendStat();
		uint   AppendCount;
		uint   UpdCount;
		uint   SrcFldsCount;
		uint   IntersectFldsCount;
		uint   SpreadSum;
		uint   SpreadCount;
		uint64 TmProfile; // Время выполнения функции
		int    UpdCountVecIdx; // @v10.4.7 (>=0) : индекс единственного вектора, по которому требуется статистика изменений
	};

	int    AddItems(const STimeSeries & rSrc, AppendStat * pStat);
	int    SetValue(uint itemIdx, uint vecIdx, double value);
	int    SetValue(uint itemIdx, uint vecIdx, float value);
	int    SetValue(uint itemIdx, uint vecIdx, int64 value);
	int    SetValue(uint itemIdx, uint vecIdx, int32 value);
	//
	// Descr: Сортирует элементы объекта в хронологическом порядке
	//
	int    Sort();
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);

	struct Stat : public StatBase {
		explicit Stat(long flags);
		enum {
			stSorted   = 0x0001,
			stHasTmDup = 0x0002
		};
		long   State;
		int16  LocalDevPtCount; // @v10.7.1 Ширина отрезка для измерения средней величины локального отклонения
		uint16 Reserve;         // @v10.7.1 @alignment
		double DeltaAvg;        // Среднее значение изменения относительно предыдущего значения в долях от единицы
		double LocalDevAvg;     // @v10.7.1 Среднее стандартное отклонение по узким отрезкам длиной LocalDevPtCount
	};

	int    Analyze(const char * pVecSymb, Stat & rS) const;
	int    Analyze(const char * pVecSymb, uint firstIdx, uint count, Stat & rS) const;

	struct AnalyzeFitParam {
		AnalyzeFitParam(uint distance, uint firstIdx, uint count);
		enum {
			fEnchMode = 0x0001 // Отладочный флаг (не использовать)
		};
		uint   Distance;
		uint   FirstIdx; // Индекс, с которого начинать анализ
		uint   IdxCount; // 0 - до конца серии
		long   Flags;
	};

	static int AnalyzeFit(const DateTimeArray * pTimeVec, const RealArray & rData, const AnalyzeFitParam & rP, RealArray * pTrendList,
		RealArray * pSumSqList, RealArray * pCov00, RealArray * pCov01, RealArray * pCov11);
	int    AnalyzeFit(const char * pVecSymb, const AnalyzeFitParam & rP, RealArray * pTrendList,
		RealArray * pSumSqList, RealArray * pCov00, RealArray * pCov01, RealArray * pCov11) const;
	int    Repair(const char * pCriticalVecSymb);
	//
	// Descr: Опции нормализации фрейма временного ряда
	//
	enum {
		nfOne       = 0x0001, // @default Нормализовать относительно единицы
		nfZero      = 0x0002, // Нормализовать относительно нуля
		nfBaseStart = 0x0004, // База нормализации - первое значение фрейма
		nfBaseAvg   = 0x0008  // База нормализации - среднее по всем значениям фрейма
	};
	//
	// Descr: Извлекает из вектора ряда с символом pVecSymb фрейм значений, начиная со startIdx в количестве count величин.
	// ARG(pVecSymb    IN): Символ вектора, из которого необходимо извлечь фрейм значений
	// ARG(startIdx    IN): Первый индекс вектора, начиная с которого извлекаются значения
	// ARG(count       IN): Количество значений, которые следует извлечь
	// ARG(normFlags   IN): Опции нормализации фрейма (STimeSeries::nfXXX)
	// ARG(rList      OUT): Вещественный вектор, в который заносятся значения фрейма.
	//   Функция предварительно очищает этот список.
	// Returns:
	//   !0 - функция выполнена успешно
	//   0 - ошибка
	//
	int    GetFrame(const char * pVecSymb, uint startIdx, uint count, long normFlags, RealArray & rList) const;
	int    GetFrame(const char * pVecSymb, uint startIdx, uint count, double diffScale, long normFlags, RealArray & rList) const;
	int    GetFrame(uint vecIdx, uint startIdx, uint count, long normFlags, RealArray & rList) const;
	int    GetFrame(uint vecIdx, uint startIdx, uint count, double diffScale, long normFlags, RealArray & rList) const;
	//
	// Descr: Извлекает подмножество временного ряда, состоящее из последних count элементов.
	//
	int    GetChunkRecentCount(uint count, STimeSeries & rResult) const;
	int    GetChunkRecentSince(const SUniTime & rSince, const SUniTime * pTill, STimeSeries & rResult) const;
private:
	static int FASTCALL VerifyValuVecType(TYPEID typ) { return oneof4(typ, T_FLOAT, T_DOUBLE, T_INT32, T_INT64); }
	struct ValuVec : public SVector {
	public:
		friend class STimeSeries;

		ValuVec(const char * pSymb, TYPEID typ, int fxPrec);
		ValuVec(); // really private. Is used only for serialization
		const SString & GetSymb() const { return Symb; }
		void   FASTCALL ConvertDoubleToInner(double outer, void * pInner) const;
		void   FASTCALL ConvertFloatToInner(float outer, void * pInner) const;
		void   FASTCALL ConvertInt32ToInner(int32 outer, void * pInner) const;
		void   FASTCALL ConvertInt64ToInner(int64 outer, void * pInner) const;
		double FASTCALL ConvertInnerToDouble(const void * pInner) const;
		float  FASTCALL ConvertInnerToFloat(const void * pInner) const;
		int32  FASTCALL ConvertInnerToInt32(const void * pInner) const;
		int64  FASTCALL ConvertInnerToInt64(const void * pInner) const;
		//int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	private:
        /*const*/TYPEID Typ; // Из-за необходимости сериализации элемент не может быть const (фактически, изменять запрещено)
		int16  FxPrec;
		uint16 Flags;
        SString Symb;
	};
	ValuVec * FASTCALL GetVecBySymb(const char * pSymb, uint * pVecIdx) const;
	ValuVec * FASTCALL GetVecByIdx(uint vecIdx) const;
	int    Helper_SetValue(uint itemIdx, ValuVec * pVec, const void * pValuePtr);
	int    FASTCALL Swap(uint p1, uint p2);
	int    Helper_GetFrame(const ValuVec * pVec, uint startIdx, uint count, double diffScale, long normFlags, RealArray & rList) const;

	enum {
		stOrdered = 0x0001
	};
	uint32 Ver; // Номер версии для обеспечения совместимости форматов сериализации в будущем
	uint8  Reserve[32];
	uint32 Id;
	uint32 State;
	SString Symb;
	TSVector <SUniTime> T;
	TSCollection <ValuVec> VL;
};
//
// Arima
//
class RDI {
public:
	RDI();
	~RDI();
	void   Init(const DblQueue * pInitQueue);
	void   Init(const double * pInitQueue, uint interval);
	double StepDiff(double) const;
	double StepIntg(double) const;
	int    Step(double newVal);
	int    GetQueue(DblQueue * pQueue) const;
	int    GetSeason() const { return P_Queue ? P_Queue->size() : 0; }
private:
	DblQueue * P_Queue;
};

class ARIMA : public ARMA {
public:
	ARIMA();
	void   Init(int p, int q, const double * pInitSeries, /* p items */ const DblQueue * pRDIInitQueue);
	void   Init(int p, int q, const LVect * pModel, const LVect * pPhi, const LMatrix *, const DblQueue * pRDIInitQueue);
	int    Step(double);
	double Predict() const;
private:
	RDI    Rdi;
};
//
// Approximation using Least Square Solution method
//
class LssLin {
public:
	LssLin();
	void   Solve(const LVect & x, const LVect & y);
	void   Solve(uint count, const double * pX, const double * pY);
	void   Solve_Simple(uint count, const double * pX, const double * pY);
	void   Solve_SSE(uint count, const double * pX, const double * pY);
	double Estimation(double x, double * pYErr) const;

	// y = A + x*B
	double A;
	double B;
	double Cov00;
	double Cov01;
	double Cov11;
	double SumSq;
private:

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
	virtual ~SRng();
	virtual void   Set(ulong seed) = 0;
	virtual ulong  Get() = 0;
	virtual double GetReal() = 0;
	//
	// Descr: Возвращает неотрицательное целое число МЕНЬШЕ ЧЕМ n.
	// Returns: [1..n-1]
	//
	ulong  FASTCALL GetUniformInt(ulong n);
	double GetUniformPos();
	ulong  GetMin() const;
	ulong  GetMax() const;
	double GetGaussian(double sigma);
	double GetGaussianZiggurat(double sigma);
	double GetGaussianPdf(double x, double sigma) const;
	ulong  GetBinomial(double p, ulong n);
	ulong  GetPoisson(double mu);
	double GetPoissonPdf(ulong k);
	double GetGamma(double a, double b);
	double GetGammaInt(uint a);
	double GetGammaPdf(double x, double a, double b);
protected:
	SRng(int alg, uint level, ulong rndMin, ulong rndMax);

	const int    Alg;
	const uint   Level;
	const ulong  RandMin;
	const ulong  RandMax;
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
	explicit SRandGenerator(SRng::Algorithm alg = SRng::algMT, uint level = 0);
	~SRandGenerator();
	void   Set(ulong seed);
	ulong  Get();
	double GetReal();
	//
	// Descr: Возвращает случайное не отрицательное целое число СТРОГО МЕНЬШЕ чем n.
	//
	ulong  FASTCALL GetUniformInt(ulong n);
	//
	// Descr: Возвращает не отрицательное целое число.
	//
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
	//
	// Descr: Заполняет случайными данными буфер pBuffer длиной bufferSize.
	//
	void   ObfuscateBuffer(void * pBuffer, size_t bufferSize) const;
	//
	// Descr: Возвращает 1 с вероятностью prob генерируя случайное число в диапазоне [0..1] и сравнивая его со значением prob.
	//   Если случайное число меньше или равно prob, то возвращает 1 в противном случае - 0.
	// ARG(prob IN): @#[0..1] Заданная вероятность получения 1.
	//
	int    GetProbabilityEvent(double prob);
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
// use next three functions for replacing (is_)power_of_two in assimilated projects
//
//FORCEINLINE bool IsPowerOfTwo(uint val) { return ((val != 0) && ((val & (val - 1)) == 0)); }
//FORCEINLINE bool IsPowerOfTwo(uint32 val) { return ((val != 0) && ((val & (val - 1)) == 0)); }
//FORCEINLINE bool IsPowerOfTwo(uint64 val) { return ((val != 0) && ((val & (val - 1)) == 0)); }
template <class T> CONSTEXPR bool IsPowerOfTwo(T val) { return ((val != 0) && ((val & (val - 1)) == 0)); }
//
// Descr: Если val - простое число, то возвращает !0, в противном случае - 0.
//
int    FASTCALL IsPrime(ulong val);
//
// Descr: Возвращает указатель на таблицу простых чисел. Количество элементов в таблице
//   возвращается по указателю pCount.
//
const  ushort * FASTCALL GetPrimeTab(size_t * pCount);
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
//int    FASTCALL smin(int a, int b);
//int    FASTCALL smax(int a, int b);
//int64  FASTCALL smin(int64 a, int64 b);
//int64  FASTCALL smax(int64 a, int64 b);
//uint   FASTCALL smin(uint a, uint b);
//uint   FASTCALL smax(uint a, uint b);
//uint64 FASTCALL smin(uint64 a, uint64 b);
//uint64 FASTCALL smax(uint64 a, uint64 b);
//double FASTCALL smin(double a, double b);
//double FASTCALL smax(double a, double b);
//float  FASTCALL smax(float a, float b);
//float  FASTCALL smin(float a, float b);
template <class T> T smin(T a, T b) { return MIN(a, b); }
template <class T> T smax(T a, T b) { return MAX(a, b); }
//
// Descr: Возвращает максимум 3-х аргументов.
//
double smax3(double a1, double a2, double a3);
//
// Descr: Возвращает максимум 4-х аргументов.
//
double smax4(double a1, double a2, double a3, double a4);
//
// Descr: Возвращает минимум 3-х аргументов.
//
double smin3(double a1, double a2, double a3);
//
// Descr: Возвращает минимум 4-х аргументов.
//
double smin4(double a1, double a2, double a3, double a4);
//
// Descr: Функция возвращает значение v в пределах диапазона [lo..up]. If v >= lo && v <= up, то результат v, elseif (v < lo) --> lo, else --> up
// Функции sclampx проверяют границы диапазона lo vs up: если lo < up то результат эквивалентен sclamp(v, lo, up) иначе - sclamp(v, up, lo)
//
//int    FASTCALL sclamp(int v, int lo, int up);
//uint   FASTCALL sclamp(uint v, uint lo, uint up);
//int64  FASTCALL sclamp(int64 v, int64 lo, int64 up);
//uint64 FASTCALL sclamp(uint64 v, uint64 lo, uint64 up);
//double FASTCALL sclamp(double v, double lo, double up);
//float  FASTCALL sclamp(float v, float lo, float up);
//int    FASTCALL sclampx(int v, int lo, int up);
//uint   FASTCALL sclampx(uint v, uint lo, uint up);
//int64  FASTCALL sclampx(int64 v, int64 lo, int64 up);
//uint64 FASTCALL sclampx(uint64 v, uint64 lo, uint64 up);
//double FASTCALL sclampx(double v, double lo, double up);
//float  FASTCALL sclampx(float v, float lo, float up);

template <class T> T sclamp(T v, T lo, T up) { return (v < lo) ? lo : ((v > up) ? up : v); }
template <class T> T sclampx(T v, T lo, T up) { return (lo < up) ? sclamp(v, lo, up) : sclamp(v, up, lo); }

double FASTCALL fdiv100r(double);
double FASTCALL fdiv100i(long);
double FASTCALL fdiv1000i(long);
//
// Descr: делит a на b в вещественном виде ((double)a) / ((double)b)
//   Если b == 0, то возвращает 0.
//
double FASTCALL fdivi(long a, long b);
double FASTCALL fdivui(uint a, uint b);
long   FASTCALL fmul100i(double);
long   FASTCALL fmul1000i(double);
//
// Descr: Проверяет равенство величин v1 и v2 с точностью до epsilon.
// Returns:
//   1 - fabs(v1-v2) < epsilon
//   0 - fabs(v1-v2) >= epsilon
//
inline bool feqeps(double v1, double v2, double epsilon) { return (fabs(v1-v2) < epsilon); }
//
// Descr: ret = (dr != 0) ? (dd / dr) : 0
//
//double FASTCALL fdivnz(double dd, double dr);
// @v11.3.0 Функция fdivnz() теперь в inline-варианте. Это снижает размер кода (и, очевидно, быстрее работает)
FORCEINLINE double fdivnz(double dd, double dr) { return (dr != 0.0) ? (dd / dr) : 0.0; }
//
// Descr: Возвращает целую часть аргумента v
//
double FASTCALL fint(double v); // @>>modf
//
// Descr: Возвращает дробную часть аргумента v
//
double FASTCALL ffrac(double v); // @>>modf
//
// Descr: Функция разбивает число с плавающей точкой value на фракции обыкновенной дроби в соответствии
//   с аргументами fragmentation и tolerance.
// ARG(value IN): Значение, которое следует разбить. Если value < 0, то функция приводит его к абсолютному значению
//   и лишь после этого обрабатывает. Таким образом, вызовы fsplitintofractions(x, ...) и fsplitintofractions(-x, ...) эквивалентны по результату.
// ARG(fragmentation IN): Целочисленный параметр фрагментации. Если fragmentation == 0 или fragmentation > 100000, то функция возвращает ошибку.
//   Этот параметр представляет знаменатель дроби, которой должно быть представлено число value. То есть, при допустимых
//   входных параметрах в результате выполнения функция должен соблюдаться инвариант *pDenominator == fragmentation
// ARG(tolerance IN): Параметр чувствительности разбивки к ошибке округления. Если при попытке разбивки ffrac(value) * fragmentation будет
//   отличаться от целого числа на величину большую чем tolerance, то функция вернет ошибку.
// ARG(pIntPart OUT): По указателю присваивается целая часть от fabs(value)
// ARG(pNumerator OUT): По указателю присваивается числитель дроби результата
// ARG(pDenominator OUT): По указателю присваивается знаменатель дроби результата
// Returns:
//   1 - SUCCESS
//   0 - ERROR
//
int    fsplitintofractions(double value, uint fragmentation, double tolerance, double * pIntPart, double * pNumerator, double * pDenominator);
FORCEINLINE int fceili(double v) { return static_cast<int>(ceil(v)); }
FORCEINLINE int ffloori(double v) { return static_cast<int>(floor(v)); }
FORCEINLINE int fceili(float v) { return static_cast<int>(ceil(v)); }
FORCEINLINE int ffloori(float v) { return static_cast<int>(floor(v)); }
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
float  fdotproduct(uint count, const float * pV1, const float * pV2);
double fdotproduct(uint count, const double * pV1, const double * pV2);
float  feuclideannorm(uint count, const float * pV);
double feuclideannorm(uint count, const double * pV);
float  feuclideandistance(uint count, const float * pV1, const float * pV2);
double feuclideandistance(uint count, const double * pV1, const double * pV2);

#define SQ(x) ((x)*(x))

double FASTCALL fpowi(double x, int n);
float  FASTCALL fpowfi(float x, int n);
double FASTCALL fpow10i(int n);
float  FASTCALL fpow10fi(int n);
uint64 FASTCALL ui64pow10(uint n);
inline double fpow2(double x) { return x * x; }
inline double fpow3(double x) { return x * x * x; }
//
// Descr: Возводит число x в степень n
//
uint64 ui64pow(uint64 x, uint n);

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
double sigmoid(double a, double x);
//
// Descr: Расчитывает коэффициент корреляции между последовательностями pData1 и pData2
//   длиной n значений.
//
double scorrelation(const double * pData1, const double * pData2, const size_t n);
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
double Gamma(double x);
double GammaIncompleteP(double a, double x);
double GammaIncompleteQ(double a, double x);
inline int fsign(double x) { return (x >= 0.0) ? +1 : -1; }
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
int  STDCALL  SIntersectRect(RECT & rDst, const RECT & rSrc1, const RECT & rSrc2);
//
// Descr: Идентифицирует факт пересечения прямоугольников rSrc1 и rSrc2.
// Returns:
//   1 - пересечение rSrc1 и rSrc2 не пустое
//   0 - пересечение rSrc1 и rSrc2 пустое
//
int  FASTCALL SIntersectRect(const RECT & rSrc1, const RECT & rSrc2);
void STDCALL  SInflateRect(RECT & rRect, int cx, int cy);
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

struct FRect { // @persistent
	FRect();
	FRect(float left, float top, float right, float bottom);
	FRect(float width, float height);
	FRect(const TRect & r);
	FRect(const RECT & r);
	bool   FASTCALL operator == (const FRect & rS) const;
	bool   FASTCALL operator != (const FRect & rS) const;
	FRect & Z();
	//
	// Descr: Присваивает всем 4-м координатам одно и тоже значение v.
	// Returns:
	//   *this
	//
	FRect & Set(float v);
	FRect & FASTCALL operator = (SPoint2F p);
	FRect & Around(SPoint2F p, SPoint2F size);
	FRect & Grow(float aDX, float aDY);
	FRect & Move__(float aDX, float aDY);
	FRect & FASTCALL MoveCenterTo(SPoint2F center);
	bool   FASTCALL IsEq(const FRect & rS) const;
	bool   IsEmpty() const;
	int    Contains(SPoint2F p) const;
	int    FASTCALL Contains(const FRect & rR) const;
	float  Width() const;
	float  Height() const;
	SPoint2F GetSize() const;
	SPoint2F GetCenter() const;
	//
	// Descr: Возвращает отношение высоты (Height) к ширине (Width) прямоугольника.
	//   Если ширина равна нулю, то возвращает SMathConst::Max.
	//
	double Ratio() const;
	//
	// Descr: Возвращает площадь прямоугольника.
	//
	double Square() const;
	FRect & FASTCALL Union(const FRect & rR);

	SPoint2F a;
	SPoint2F b;
};

class FShape { // @persistent @store(SSerializeContext)
public:
	typedef FRect Rect;

	struct Line {
		SPoint2F A;
		SPoint2F B;
	};
	struct Circle {
		Circle();
		SPoint2F C; // Center
		float  R; // Radius
	};
	struct Ellipse {
		SPoint2F C; // Center
		SPoint2F R; // Радиус по осям X и Y
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
		SPoint2F A;
		SPoint2F B;
		SPoint2F C;
	};
	struct RoundedRect : public FRect {
		SPoint2F R;     // Эллиптический (по осям X и Y) радиус скругления углов.
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
class SPoint2I {
public:
	SPoint2I() : x(0), y(0)
	{
	}
	SPoint2I(int _x, int _y) : x(_x), y(_y)
	{
	}
	SPoint2I & Set(int _x, int _y)
	{
		x = _x;
		y = _y;
		return *this;
	}
	//
	// Descr: Так как этот класс бинарно совместим с Win32 POINT, то
	//   следующий метод просто возвращает преобразованную ссылку на this.
	//
	operator POINT & ();
	//operator POINT() const;
	SPoint2I & Z();
	int    x;
	int    y;
};
//
//
//
class SPoint2S { // @persistent @store(SSerializeContext) @noctr @nodestructor @novtbl @size=4
public:
	operator SPoint2F () const;
	uint32 towparam() const;
	SPoint2S FASTCALL operator = (SPoint2S p);
	SPoint2S FASTCALL operator = (SPoint2F p);
	SPoint2S FASTCALL operator = (int v);
	SPoint2S FASTCALL operator = (POINT p);
	operator POINT() const;
	bool   IsZero() const;
	SPoint2S & Z();
	SPoint2S Set(int _x, int _y);
	SPoint2S FASTCALL setwparam(uint32 wp);
	SPoint2S Add(int add_x, int add_y);
	SPoint2S operator += (SPoint2S adder);
	SPoint2S operator -= (SPoint2S subber);

	friend SPoint2S operator - (SPoint2S one, SPoint2S two);
	friend SPoint2S operator + (SPoint2S one, SPoint2S two);
	friend SPoint2S operator + (SPoint2S pnt, int _x);
	friend bool operator == (SPoint2S one, SPoint2S two);
	friend bool operator != (SPoint2S one, SPoint2S two);

	int16  x;
	int16  y;
};

class TRect {
public:
	TRect();
	TRect(int ax, int ay, int bx, int by);
	TRect(SPoint2S p1, SPoint2S p2);
	TRect(SPoint2S sz);
	TRect(const RECT &);
	TRect & FASTCALL operator = (const RECT &);
	TRect & FASTCALL operator = (SPoint2S p);
	bool   IsEmpty() const;
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
	TRect & FASTCALL move(SPoint2S delta);
	TRect & FASTCALL movecenterto(SPoint2S center);
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
	TRect & Z();
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
	int    FASTCALL contains(SPoint2S p) const;
	bool   FASTCALL operator == (const TRect& r) const;
	bool   FASTCALL operator != (const TRect& r) const;

	SPoint2S a;
	SPoint2S b;
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
	bool   operator !() const;
	SRegion & FASTCALL operator = (const SRegion & rS);
	bool   FASTCALL operator == (const SRegion & rS) const;
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
class SPoint2R {
public:
	SPoint2R & Set(double _xy);
	SPoint2R & Set(double _x, double _y);
	//
	// Descr: Устанавливает полярные координаты точки.
	//   x = v * cos(rad);
	//   y = v * sin(rad);
	//
	SPoint2R & SetPolar(double v, double rad);
	void   GetInt(int * pX, int * pY) const;
	void   GetUInt(uint * pX, uint * pY) const;
	int    FASTCALL FromStr(const char * pStr);
	friend SPoint2R & FASTCALL operator - (const SPoint2R & one, const SPoint2R & two);
	friend SPoint2R & FASTCALL operator + (const SPoint2R & one, const SPoint2R & two);
	friend SPoint2R & FASTCALL operator + (const SPoint2R & p, double a);
	friend SPoint2R & FASTCALL operator * (const SPoint2R & p, double m);
	friend SPoint2R & FASTCALL operator / (const SPoint2R & p, double);

	double x;
	double y;
};

class SPoint3R {
public:
	SPoint3R() : x(0.0), y(0.0), z(0.0)
	{
	}
	SPoint3R(double _v) : x(_v), y(_v), z(_v)
	{
	}
	SPoint3R(double _x, double _y, double _z) : x(_x), y(_y), z(_z)
	{
	}
	SPoint3R & Set(double _x, double _y, double _z);
	SPoint3R & Set(double _v);
	SPoint3R & SetXY(double _x, double _y)
	{
		x = _x;
		y = _y;
		return *this;
	}
	SPoint2R & GetXY() { return *reinterpret_cast<SPoint2R *>(&x); }
	bool   FASTCALL IsEq(const SPoint3R & rS) const { return (x == rS.x && y == rS.y && z == rS.z); }
	friend SPoint3R & FASTCALL operator - (const SPoint3R & one, const SPoint3R & two);
	friend SPoint3R & FASTCALL operator + (const SPoint3R & one, const SPoint3R & two);
	friend SPoint3R & FASTCALL operator * (const SPoint3R & p, double m);
	friend SPoint3R & FASTCALL operator / (const SPoint3R & p, double);

	double x;
	double y;
	double z;
};

class SPoint3F {
public:
	SPoint3F() : x(0.0f), y(0.0f), z(0.0f)
	{
	}
	SPoint3F(float _v) : x(_v), y(_v), z(_v)
	{
	}
	SPoint3F & Set(float _x, float _y, float _z);
	SPoint3F & Set(float _v)
	{
		x = y = z = _v;
		return *this;
	}
	float  x;
	float  y;
	float  z;
};
//
//
//
struct UiCoord { // @noctr
	enum {
		dfAbs     = 0x0001, // Координата задана в абсолютных единицах
		dfRel     = 0x0002, // Координата или размер задана в долях от общего размера контейнера.
		dfGravity = 0x0004, // Координата притягивается к соответствующей границе контейнера.
		dfOpp     = 0x0008, // Координата отсчитывается от противоположной стороны контейнера.
		dfSize    = 0x0010, // Структура представляет линейный размер (не координату)
		dfContent = 0x0020  // @v11.0.4 Только для размера: он определяется содержимым
	};
	void   Set(float v, int f);
	void   Reset();
	bool   IsEmpty() const;

	float  Val;   // @v11.0.4 int16-->float
	int16  Flags; // UiCoord::dfXXX
	uint16 Reserve; // @alignment @v11.0.4
};

struct UiRelPoint { // @noctr
	UiRelPoint & FASTCALL Set(const SPoint2S & rP);

	UiCoord X;
	UiCoord Y;
};

struct UiRelRect { // @noctr
	void   Reset();
	bool   IsEmpty() const;
	UiRelRect & FASTCALL Set(const TRect & rR);

	UiRelPoint L;
	UiRelPoint R;
};
//
//
//
enum SColourCollection { // 0x00rrggbb
	SClrAliceblue            = 0x00f0f8ff,
	SClrAntiquewhite   		 = 0x00faebd7,
	SClrAqua           		 = 0x0000ffff,
	SClrAquamarine     		 = 0x007fffd4,
	SClrAzure          		 = 0x00f0ffff,
	SClrBeige          		 = 0x00f5f5dc,
	SClrBisque         		 = 0x00ffe4c4,
	SClrBlack          		 = 0x00000000,
	SClrBlanchedalmond 		 = 0x00ffebcd,
	SClrBlue           		 = 0x000000ff,
	SClrBlueviolet     		 = 0x008a2be2,
	SClrBrown          		 = 0x00a52a2a,
	SClrBurlywood      		 = 0x00deb887,
	SClrCadetblue      		 = 0x005f9ea0,
	SClrChartreuse     		 = 0x007fff00,
	SClrChocolate      		 = 0x00d2691e,
	SClrCoral          		 = 0x00ff7f50,
	SClrCornflowerblue 		 = 0x006495ed,
	SClrCornsilk       		 = 0x00fff8dc,
	SClrCrimson        		 = 0x00dc143c,
	SClrCyan           		 = 0x0000ffff,
	SClrDarkblue       		 = 0x0000008b,
	SClrDarkcyan       		 = 0x00008b8b,
	SClrDarkgoldenrod  		 = 0x00b8860b,
	SClrDarkgrey       		 = 0x00555555,
	SClrDarkgrey_npp = 0x00404040, // @v11.1.12 darkGrey according to notepad++
	SClrDarkgreen      		 = 0x00006400,
	SClrDarkkhaki      		 = 0x00bdb76b,
	SClrDarkmagenta    		 = 0x008b008b,
	SClrDarkolivegreen 		 = 0x00556b2f,
	SClrDarkorange     		 = 0x00ff8c00,
	SClrDarkorchid     		 = 0x009932cc,
	SClrDarkred        		 = 0x008b0000,
	SClrDarksalmon     		 = 0x00e9967a,
	SClrDarkseagreen   		 = 0x008fbc8f,
	SClrDarkslateblue  		 = 0x00483d8b,
	SClrDarkslategrey  		 = 0x002f4f4f,
	SClrDarkturquoise  		 = 0x0000ced1,
	SClrDarkviolet     		 = 0x009400d3,
	SClrDeeppink       		 = 0x00ff1493,
	SClrDeepskyblue    		 = 0x0000bfff,
	SClrDimgrey        		 = 0x00696969,
	SClrDodgerblue     		 = 0x001e90ff,
	SClrFirebrick      		 = 0x00b22222,
	SClrFloralwhite    		 = 0x00fffaf0,
	SClrForestgreen    		 = 0x00228b22,
	SClrFuchsia        		 = 0x00ff00ff,
	SClrGainsboro      		 = 0x00dcdcdc,
	SClrGold           		 = 0x00ffd700,
	SClrGoldenrod      		 = 0x00daa520,
	SClrGrey           		 = 0x00808080,
	SClrGreen          		 = 0x00008000,
	SClrGreenyellow    		 = 0x00adff2f,
	SClrHoneydew       		 = 0x00f0fff0,
	SClrHotpink        		 = 0x00ff69b4,
	SClrIndianred      		 = 0x00cd5c5c,
	SClrIndigo         		 = 0x004b0082,
	SClrIvory          		 = 0x00fffff0,
	SClrKhaki          		 = 0x00f0e68c,
	SClrLavender       		 = 0x00e6e6fa,
	SClrLavenderblush  		 = 0x00fff0f5,
	SClrLemonchiffon   		 = 0x00fffacd,
	SClrLightblue      		 = 0x00add8e6,
	SClrLightcoral     		 = 0x00f08080,
	SClrLightcyan      		 = 0x00e0ffff,
	SClrLightgoldenrodyellow = 0x00fafad2,
	SClrLightgreen   = 0x0090ee90,
	SClrLightgrey       	 = 0x00d3d3d3,
	SClrLightpink       	 = 0x00ffb6c1,
	SClrLightsalmon     	 = 0x00ffa07a,
	SClrLightseagreen   	 = 0x0020b2aa,
	SClrLightskyblue    	 = 0x0087cefa,
	SClrLightslategrey  	 = 0x00778899,
	SClrLightsteelblue  	 = 0x00b0c4de,
	SClrLightyellow     	 = 0x00ffffe0,
	SClrLightyellow_npp 	 = 0x00ffffd5, // @v11.1.12 lightYellow according to notepad++
	SClrLime            	 = 0x0000ff00,
	SClrLimegreen       	 = 0x0032cd32,
	SClrLinen           	 = 0x00faf0e6,
	SClrMagenta         	 = 0x00ff00ff,
	SClrMaroon          	 = 0x00800000,
	SClrMediumaquamarine     = 0x0066cdaa,
	SClrMediumblue        	 = 0x000000cd,
	SClrMediumorchid      	 = 0x00ba55d3,
	SClrMediumpurple      	 = 0x009370db,
	SClrMediumseagreen    	 = 0x003cb371,
	SClrMediumslateblue   	 = 0x007b68ee,
	SClrMediumspringgreen 	 = 0x0000fa9a,
	SClrMediumturquoise   	 = 0x0048d1cc,
	SClrMediumvioletred   	 = 0x00c71585,
	SClrMidnightblue = 0x00191970,
	SClrMintcream     		 = 0x00f5fffa, // Очень светло-голубой //
	SClrMistyrose     		 = 0x00ffe4e1, // Нежно-розовый
	SClrNavajowhite   		 = 0x00ffdead, // Телесный
	SClrNavy          		 = 0x00000080, // Темно-синий
	SClrOldlace       		 = 0x00fdf5e6,
	SClrOlive         		 = 0x00808000, // ok Оливковый
	SClrOlivedrab     		 = 0x006b8e23,
	SClrOrange        		 = 0x00ffa500,
	SClrOrange_npp    		 = 0x00ff8000, // @v11.1.12 orange according to notepad++
	SClrOrangered     		 = 0x00ff4500,
	SClrOrchid        		 = 0x00da70d6,
	SClrPalegoldenrod 		 = 0x00eee8aa,
	SClrPalegreen     		 = 0x0098fb98,
	SClrPaleturquoise 		 = 0x00afeeee,
	SClrPalevioletred 		 = 0x00db7093,
	SClrPapayawhip    		 = 0x00ffefd5,
	SClrPeachpuff     		 = 0x00ffdab9,
	SClrPeru          		 = 0x00cd853f,
	SClrPink          		 = 0x00ffc0cb,
	SClrPlum          		 = 0x00dda0dd,
	SClrPowderblue    		 = 0x00b0e0e6,
	SClrPurple        		 = 0x00800080,
	SClrPurple_npp    		 = 0x008000ff, // @v11.1.12 purple according to notepad++
	SClrRed           		 = 0x00ff0000,
	SClrRosybrown     		 = 0x00bc8f8f,
	SClrRoyalblue     		 = 0x004169e1,
	SClrSaddlebrown   		 = 0x008b4513,
	SClrSalmon        		 = 0x00fa8072,
	SClrSandybrown    		 = 0x00f4a460,
	SClrSeagreen      		 = 0x002e8b57,
	SClrSeashell      		 = 0x00fff5ee,
	SClrSienna        		 = 0x00a0522d,
	SClrSilver        		 = 0x00c0c0c0,
	SClrSkyblue       		 = 0x0087ceeb,
	SClrSlateblue     		 = 0x006a5acd,
	SClrSlategrey     		 = 0x00708090,
	SClrSnow          		 = 0x00fffafa,
	SClrSpringgreen   		 = 0x0000ff7f,
	SClrSteelblue     		 = 0x004682b4,
	SClrSteelblue4    		 = 0x00306080,
	SClrTan           		 = 0x00d2b48c,
	SClrTeal          		 = 0x00008080,
	SClrThistle       		 = 0x00d8bfd8,
	SClrTomato        		 = 0x00ff6347,
	SClrTurquoise     		 = 0x0040e0d0,
	SClrViolet        		 = 0x00ee82ee,
	SClrWheat         		 = 0x00f5deb3,
	SClrWhite         		 = 0x00ffffff,
	SClrWhitesmoke    		 = 0x00f5f5f5,
	SClrYellow        		 = 0x00ffff00,
	SClrYellowgreen   		 = 0x009acd32,
	SClrVeryLitegrey  		 = 0x00e0e0e0, // @v11.1.12 according to notepad++
};
//
// Descr: Структура для унификации сторонних библиотек
//
struct SColorRGB {
	bool   IsZero() const;
	SColorRGB Set(uint r, uint g, uint b);
	SColorRGB FASTCALL Set(uint v);

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
	bool   IsEmpty() const { return (!B && !G && !R && !Alpha); }
	SColorBase Z();
	SColorBase Set(uint r, uint g, uint b);
	SColorBase FASTCALL Set(uint v);
	SColorBase PremultiplyAlpha();
	enum {
		fmtHEX         = 1, // #rrggbb
		fmtRGB         = 2, // rgb(rr, gg, bb)
		fmtRgbHexWithoutPrefix = 3, // rrggbb
		fmtName        = 0x1000
	};
	int    FASTCALL FromStr(const char *);
	SString & ToStr(SString & rBuf, int format = fmtHEX) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * SCtx);
	//
	// Порядок полей существенен. Эквивалентен RGBQUAD (+alpha).
	//
	uint8  B;
	uint8  G;
	uint8  R;
	uint8  Alpha;
};
//
// Descr: То же, что и SColorBase, но с конструкторами
// Attention: Как и SColorBase эта структура перманентная. Ее размер фиксирован и новых member-переменных в нее добавлять нельзя.
// Размер верифицируется в run-time
//
struct SColor : public SColorBase { // @persistent
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
COLORREF FASTCALL GetGrayColorRef(float whitePart);
COLORREF FASTCALL LightenColor(COLORREF col, float factor);
COLORREF FASTCALL DarkenColor(COLORREF col, float factor);

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
		fSeparateDataStruct    = 0x0001,
		fDontProcessAttachment = 0x0002, // Запрет на сериализацию связанных blob'ов (которые могут быть пропущены)
		fUseCompression        = 0x0004  // @v10.2.3 Ряд функций сериализации может использовать сжатие данных
	};

	static size_t FASTCALL GetCompressPrefix(uint8 * pBuf); // size of pBuf >= 8
	static int    FASTCALL IsCompressPrefix(const void * pBuf);
	SSerializeContext();
	~SSerializeContext();
	void   Init(long flags, LDATE suppDate);
	bool   FASTCALL CheckFlag(long f) const;
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
	int    SerializeStateOfContext(int dir, SBuffer & rBuf);
	int    Serialize(const char * pDbtName, BNFieldList * pFldList, const void * pData, SBuffer & rBuf);
	//
	// Descr: Считывает из буфера rBuf запись таблицы БД в соответствии с внутренним описанием (в потоке)
	//   структуры данных. Данные считываются в буфер, на который указывает pData.
	// Note: Если pFldList == 0 тогда просто указатель считывания перемещается в конец
	//   записи, но по указателю pData ничего не записывается.
	//
	int    Unserialize(const char * pDbtName, const BNFieldList * pFldList, void * pData, SBuffer & rBuf);
	int    Serialize(int dir, TYPEID typ, void * pData, uint8 * pInd, SBuffer & rBuf);
	int    Serialize(int dir, SString & rStr, SBuffer & rBuf);
	int    Serialize(int dir, SStringU & rStr, SBuffer & rBuf);
	int    Serialize(int dir, int64 & rV, SBuffer & rBuf);
	int    Serialize(int dir, int32 & rV, SBuffer & rBuf);
	int    Serialize(int dir, int16 & rV, SBuffer & rBuf);
	int    Serialize(int dir, int8 & rV, SBuffer & rBuf);
	int    Serialize(int dir, int & rV, SBuffer & rBuf);
	int    Serialize(int dir, bool & rV, SBuffer & rBuf);
	int    Serialize(int dir, uint & rV, SBuffer & rBuf);
	int    Serialize(int dir, uint64 & rV, SBuffer & rBuf);
	int    Serialize(int dir, uint32 & rV, SBuffer & rBuf);
	int    Serialize(int dir, uint16 & rV, SBuffer & rBuf);
	int    Serialize(int dir, uint8 & rV, SBuffer & rBuf);
	int    Serialize(int dir, LDATE & rV, SBuffer & rBuf);
	int    Serialize(int dir, LTIME & rV, SBuffer & rBuf);
	int    Serialize(int dir, LDATETIME & rV, SBuffer & rBuf);
	int    Serialize(int dir, DateRange & rV, SBuffer & rBuf);
	int    Serialize(int dir, float & rV, SBuffer & rBuf);
	int    Serialize(int dir, double & rV, SBuffer & rBuf);
	int    Serialize(int dir, S_GUID & rV, SBuffer & rBuf);
	int    Serialize(int dir, SColor & rV, SBuffer & rBuf);
	int    Serialize(int dir, char * pV, size_t valBufLen, SBuffer & rBuf);
	int    SerializeFieldList(int dir, BNFieldList * pFldList, SBuffer & rBuf);
	int    Serialize(int dir, SPoint2S & rV, SBuffer & rBuf);
	int    Serialize(int dir, SPoint2F & rV, SBuffer & rBuf);
	int    Serialize(int dir, FRect & rV, SBuffer & rBuf);
	int    Serialize(int dir, SArray * pArray, SBuffer & rBuf);
	int    Serialize(int dir, SVector * pArray, SBuffer & rBuf);
	int    Serialize(int dir, SStrCollection * pColl, SBuffer & rBuf);
	int    Serialize(int dir, StrAssocArray & rArray, SBuffer & rBuf);
	int    Serialize(int dir, SBinaryChunk & rBc, SBuffer & rBuf);
	int    SerializeBlock(int dir, uint32 size, void * pData, SBuffer & rBuf, int skipMissizedBlock);
	//
	// Descr: Возвращает опорную дату.
	//   Опорная дата используется для сжатого хранения типа LDATE.
	//   В поток заносится разница в днях между сохраняемой датой и опорной датой.
	//
	LDATE  GetSupportingDate() const;
	int    AddDbtDescr(const char * pName, const BNFieldList * pList, uint32 * pID);
	int    GetDbtDescr(uint id, BNFieldList * pList) const;
	//
	// Локальные сигнатурные теги могут использоваться для информирования функций сеймейства Serialize
	//   об особенностях чтения/записи в поток.
	//   Идея использования следующая: функция верхнего уровня, вызывающая Serailize для какого-либо
	//   объекта, который "знает" о локальных сигнатурных тегах, устанавливает значение тега перед
	//   вызовом, сигнализируя соответствующей функции, а та в свою очередь учтет этот сигнал
	//   при чтении/записи.
	//   Фактически, это - способ дополнительной параметризации функций семейства Serialize
	//
	//   Например:
	//
	// class Abc {
	// public:
	// 	int Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
	// 	{
	// 		int    ok = 1;
	// 		uint32 signature_tag = 0;
	// 		Serialize(dir, Member01, pSCtx);
	// 		Serialize(dir, Member02, pSCtx);
	// 		// При записи в буфер работаем как обычно, при чтении - считываем Member03 только если
	// 		// сигнатурный тег соответствующий ABC_SIGNATIRE больше 10.
	// 		if(dir > 0 || (pSCtx->GetLocaSignatureTag(ABC_SIGNATURE, &signature_tag) && signature_tag > 10)) {
	// 			Serialize(dir, Member03, pSCtx);
	// 		}
	// 		return ok;
	// 	}
	// private:
	// 	long   Member01;
	// 	double Member02;
	// 	uint   Member03;
	// };
	//
	// void Foo(SBuffer & rBuf, int storageVer)
	// {
	// 	Abc abc;
	// 	SSerializeContext sctx;
	// 	//
	// 	// Функция Foo сигнализирует функции Abc::Serialize() о том, что версия записи в буфер
	// 	// равна storageVer. При этом, если Abc::Serialize() будет вызывать какие-либо иные
	// 	// методы, использующие контекст sctx, то они проигнорируют тег с сигнатурой ABC_SIGNATURE,
	// 	// ибо ничего о нем не знают.
	// 	//
	// 	sctx.SetLocalSignatureTag(ABC_SIGNATURE, (uint32)storageVer);
	// 	abc.Serialize(-1, rBuf, &sctx);
	// 	// после использования сбрасываем теги с сигнатурой ABC_SIGNATURE дабы избежать побочных эффектов
	// 	// при дальнейшем использовании экземпляра sctx (если таковое предполагается).
	// 	sctx.ResetLocalSignatureTag(ABC_SIGNATURE);
	// }
	//

	//
	// Descr: Устанавливает локальный сигнатурный тег signature в значение tag.
	//
	void   SetLocalSignatureTag(uint32 signature, uint32 tag);
	//
	// Descr: Удаляет локальный сигнатурный тег signature
	//
	void   ResetLocalSignatureTag(uint32 signature);
	//
	// Descr: Если локальный сигнатурный тег signature задан, то возвращает его
	//   значение по указателю pTag. В противном случае значение *pTag остается неизменным.
	//   Если pTag == 0, то функция работает так же, но ничего не присваивает по этому указателю.
	// Returns:
	//   >0 - локальный сигнатурный тег signature найден и его значение присвоено по указателю pTag
	//    0 - локальный сигнатурный тег signature не найден.
	//
	int    GetLocalSignatureTag(uint32 signature, uint32 * pTag) const;
private:
	long   State;
	long   Flags;
	uint32 LastSymbId;
	LDATE  SuppDate;   // Опорная дата.
	SymbHashTable SymbTbl;
	SCollection * P_DbtDescrList;
	LAssocArray LocalSignatureTagList; // @v10.9.9
	SString TempStrBuf;   // @allocreuse
	SBuffer TempSBuf;     // @allocreuse
	STempBuffer TempDataBuf;
};

class SSerializer {
public:
	SSerializer(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    STDCALL  Serialize(TYPEID typ, void * pData, uint8 * pInd);
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
	int    STDCALL  Serialize(char * pV, size_t valBufLen);
	int    FASTCALL SerializeFieldList(BNFieldList * pFldList);
	int    FASTCALL Serialize(SPoint2S & rV);
	int    FASTCALL Serialize(SPoint2F & rV);
	int    FASTCALL Serialize(SArray * pArray);
	int    FASTCALL Serialize(SStrCollection * pColl);
	int    FASTCALL Serialize(StrAssocArray & rArray);
	int    STDCALL  SerializeBlock(uint32 size, void * pData, int skipMissizedBlock);
private:
	int    Dir;
	SBuffer & R_Buf;
	SSerializeContext * P_SCtx;
};
//
// Descr: Специализированная функция для сериализации шаблонизированных массивов, элементы которых
//   поддерживают невиртуальный метод Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
//
#if 0 // @unused {
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
#endif // } 0
//
// Descr: Специализированная функция для сериализации шаблонизированных векторов, элементы которых
//   поддерживают невиртуальный метод Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
//
template <class T> int TSVector_Serialize(TSVector <T> & rList, int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
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
	explicit SIniFile(const char * pFileName, int fcreate = 0, int winCoding = 0, int useIniBuf = 0);
	~SIniFile();
	bool   IsValid() const; // Вызывать после конструктора
	SCodepageIdent GetCp() const { return Cp; }
	long   GetFlags() const;
	long   SetFlag(long f, int set);
	const  SString & GetFileName() const;
	int    GetSections(StringSet * pSects);
	int    IsSectExists(const char * pSect);
	int    GetEntries(const char * pSect, StringSet * pEntries, int storeAllString = 0);
	int    GetParam(const char * pSect, const char * pParam, SString & rBuf);
	int    GetIntParam(const char * pSect, const char * param, int *);
	int    GetDataSizeParam(const char * pSect, const char * pParam, int64 * pVal);
	int    AppendParam(const char * pSect, const char * pParam, const char * pVal, int overwrite);
	int    AppendIntParam(const char * pSect, const char * pParam, int val, int overwrite = 1);
	int    RemoveParam(const char * pSect, const char * param);
	int	   RemoveSection(const char * pSect);
	int    ClearSection(const char * pSect);
	int    FlashIniBuf(); // Saves buf to file
	//
	// Descr: Определяет изменилась ли дата модификации файла с момента
	//   открытия его экземпляром этого класса.
	// Returns:
	//   1 - файл был модифицирован
	//   2 - файл был удален (не удалось получить по нему статистику)
	//   0 - время модификации файла меньше или равно времени полседнего открытия экземпляром класса //
	//
	int    WasModified() const;

	enum {
		fWinCoding      = 0x0001,
		fIniBufInited   = 0x0002,
		fCpAutodetected = 0x0004 // @v10.3.11 Кодовая страница Cp идентифицирована автоматически
	};
protected:
	int    Init(const char * pFileName, int fcreate, int winCoding, int useIniBuf);
	SString & FASTCALL EncodeText(SString & rBuf) const;
	SString & FASTCALL DecodeText(SString & rBuf) const;

	long   Flags;
	SString TempBuf; // @allocreuse
private:
	//
	// Returns:
	//   >0 - файл не был открыт и функция сделала это успешно
	//   <0 - файл уже был открыт
	//    0 - ошибка
	//
	int    Open(const char * pFileName);
	int    Close();
	int    Create(const char * pFileName);
	int    SearchParam(const char * pSect, const char * pParam, SString & rVal);
	int    InitIniBuf();
	int    InitIniBuf2();
	int    IsSection(const SString & rLineBuf, const char * pPattern, SString * pRet);
	//
	// Descr: intermediate-функция, реализующая вставку, удаление параметра, а также удаление целой секции.
	// ARG(pSect     IN):
	// ARG(pParam    IN):
	// ARG(pVal      IN):
	// ARG(overwrite IN):
	// Returns:
	//
	int    SetParam(const char * pSect, const char * pParam, const char * pVal, int overwrite);

	SCodepageIdent Cp;
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

		Row();
		~Row();
		//
		// Descr: Возвращает !0 если экземпляр является валидным. То есть,
		//   это - действительно STab::Row и он не был разрушен деструктором.
		//   Валидность проверяется по сигнатуре Sign.
		//
		int    IsConsistent() const;
		void   Clear();
		int    FASTCALL Add(const char * pStr);
		int    FASTCALL Add(long intVal);
		int    Add(double number);
		uint   GetCount() const;
		int    Get(uint pos, SString & rStr) const;
		int    Get(uint pos, double & rNumber) const;
	private:
		int    FASTCALL ToStr(SString & rBuf) const;
		int    FASTCALL FromStr(const char *);

		uint32 Sign;
		LongArray PosList;
		StringSet Set;
	};

	STab();
	STab & Z();
	uint   GetCount() const;
	int    Find(uint columnPos, const char * pKey, uint * pRowPos) const;
	int    Find(uint columnPos, double key, uint * pRowPos) const;
	int    Find(uint columnPos, long key, uint * pRowPos) const;
	int    FASTCALL AddRow(const Row &);
	int    GetRow(uint pos, Row &) const;
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
	STabFile();
	STabFile(const char * pFileName, int updateMode = 0);
	bool   IsValid() const;
	int    Open(const char * pFileName, int updateMode);
	int    Close();
	int    GetTabList(StringSet * pResult);
	int    LoadTab(const char * pTabName, STab & rTab);
	//
	// Descr: Записывает таблицу pTag с именем pTagName в файл.
	//   Если pTag == 0 и таблица с именем pTabName в файле
	//   существует, то она удаляется.
	//
	int    WriteTab(const char * pTabName, const STab * pTab);
private:
	int    Helper_WriteTab(const char * pTabName, const STab * pTab, SFile & rFile);

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
	explicit Generator_CPP(const char * pFileName);
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
	SString TempBuf; // @allocreuse
};

#endif // } !_WIN32_WCE
//
//
//
class SClipboard {
public:
	static int OpenClipboardRetry(void * hWnd);
	static int Copy_Text(const char * pText, size_t len);
	static int Copy_TextUnicode(const wchar_t * pText, size_t len);
	static int Copy_SYLK(const SString & rText);
	static int Past_Text(SStringU & rBuf);
	static int Copy_Image(SImageBuffer & rImg);
	static int Paste_Image(SImageBuffer & rImg);
	static int CopyPaste(HWND hWnd, int copy, const char * pPath); // @todo to_remake
private:
	static int FASTCALL Helper_OpenClipboardForCopy(int & rHasBeenOpened);
	static int FASTCALL Helper_CloseClipboard(int hasBeenOpened);
};
//
//
//
class SPrinting {
public:
	struct PrnInfo { // @flat
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
	static int GetListOfPrinters(TSVector <PrnInfo> * pInfoList);
	explicit SPrinting(HWND);
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

int    SCheckSystemCredentials(const char * pDomain, const char * pUserName, const char * pPw);
//
// Descr: Возвращает ЛОКАЛЬНОЕ время с сервера pServerName.
// Returns:
//   >0 - время на сервере pServerName успешно определено и присвоено по указателю pDtm
//   <0 - указатель pServerName нулевой либо пустой (pServerName[0] == 0)
//   0  - ошибка. Если ошибка системная, то SLibError содержит ссылку на системную ошибку,
//        а SLS.GetTLA().LastOsErr содержит код этой ошибки.
//
int    SGetTimeFromRemoteServer(const char * pServerName, LDATETIME * pDtm);
int    SGetComputerName(SString & rName);
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
	//
	// Descr: Возвращает номер версии DLL, вызывая функцию DllGetVersion.
	//   Если все прошло успешно, то возвращаемый результат содержит не нулевой
	//   мажор/минор номер версии DLL, в противном случае все компоненты SVerT
	//   будут нулевыми.
	//
	static SVerT GetVersion(const char * pFileName);
	explicit SDynLibrary(const char * pFileName = 0);
	~SDynLibrary();
	bool   IsValid() const;
	int    FASTCALL Load(const char * pFileName);
	FARPROC FASTCALL GetProcAddr(const char * pProcName);
	FARPROC STDCALL  GetProcAddr(const char * pProcName, int unicodeSuffix);
private:
	HMODULE H;
};
//
//
//
class WinRegValue {
public:
	friend class WinRegKey;

	explicit WinRegValue(size_t bufSize = 0);
	~WinRegValue();
	int    Alloc(size_t);
	uint   GetType() const { return Type; }
	uint32 GetDWord() const;
	const  void * GetBinary(size_t * pDataLen) const;
	//const  char * GetString() const;
	int    GetString(SString & rBuf) const;
	int    PutDWord(uint32);
	int    PutBinary(const void * pBuf, size_t dataSize);
	int    PutString(const char * pStr);
private:
	uint   Type;
	void * P_Buf;
	size_t BufSize;
	size_t DataSize;
};

class WinRegKey {
public:
	WinRegKey();
	WinRegKey(HKEY, const char * pSubKey, int readOnly);
	~WinRegKey();
	//
	// Descr: Удаляет ветку реестра key\\pSubKey.
	//   Ветку открывать перед удалением не следует.
	//
	int    Delete(HKEY key, const char * pSubKey); // @>>SHLWAPI.SHDeleteKey
	//
	// Descr: Удаляет параметр pValue из реестра key\\pSubKey\\
	//   Ветку открывать перед удалением не следует.
	//
	int    DeleteValue(HKEY key, const char * pSubKey, const char * pValue);
	int    Open(HKEY, const char * pSubKey, int readOnly, int onlyOpen = 0);
	void   Close();
	int    GetDWord(const char * pParam, uint32 * pVal);
	//int    GetString(const char * pParam, char * pBuf, size_t bufLen);
	int    GetString(const char * pParam, SString & rBuf);
	int    GetBinary(const char * pParam, void * pBuf, size_t bufLen);
	int    GetRecSize(const char * pParam, size_t * pRecSize);
	int    EnumValues(uint * pIdx, SString * pParam, WinRegValue * pVal);
	int    EnumKeys(uint * pIdx, SString & rKey);
	int    PutDWord(const char * pParam, uint32 val);
	int    PutString(const char * pParam, const char *);
	int    PutBinary(const char * pParam, const void * pBuf, size_t bufLen);
	int    PutValue(const char * pParam, const WinRegValue *);
private:
	HKEY   Key;
};
//
// Descr: Реализует управление сервисами Windows
//
class WinService {
public:
	static int Install(const char * pServiceName, const char * pDisplayName, const char * pModuleName, const char * pLoginName, const char * pPassword);
	static int Uninstall(const char * pServiceName);
	static int Start(const char * pServiceName, int stop = 0);

	WinService(const WinServiceMngr &, const char * pServiceName, long desiredAccess = SERVICE_ALL_ACCESS);
	~WinService();
	operator SC_HANDLE () const { return H; }
	bool   IsValid() const;
	const  SString & GetName() const;
	int    Create(const char * pDisplayName, const char * pModuleName, const char * pLogin, const char * pPw);
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
	int    Delete();
	//
	// Descr:
	//   desiredAccess = SERVICE_START
	//
	int    Start();
	//
	// Descr:
	//   desiredAccess = SERVICE_STOP
	//
	int    Stop();
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
	SWmi();
	~SWmi();
	int    Connect(const char * pServer = 0, const char * pUserName = 0, const char * pPassword = 0);
	int    Method_CreateProcess(const char * pCmdLine);
	int    GetMethodList(IWbemClassObject * pCls, StrAssocArray * pList);
private:
	void   Release();
	int    GetSvcError(SString & rBuf);
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
	ACount();
	enum CtrOption {
		ctrDontInitialize
	};
	//
	// Descr: Этот конструктор используется тогда, когда не следует явно инициализировать переменную в ноль.
	//
	explicit ACount(CtrOption);
	operator long() const;
	long   FASTCALL Add(long add);
	long   FASTCALL Assign(long val);
	long   Incr();
	long   Decr();
private:
	long   C;
};
/*
class ACount64 { // @#size=4
public:
	ACount64()	{C = 0;}
	//
	// Descr: Этот конструктор используется тогда, когда
	//   не следует явно инициализировать переменную в ноль.
	//
	ACount64(int64) {}
	operator int64() const { return C; }
	int64  Incr() { return ::InterlockedIncrement64(&C); }
	int64  Decr() { return ::InterlockedDecrement64(&C); }
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
		explicit Data(int dontDestroy = 0);
		~Data();
		void   Enter();
		int    TryEnter();
		void   Leave();
	private:
		CRITICAL_SECTION C;
		int    DontDestroyOnDestruction;
	};
	SCriticalSection(Data & rCsd) : R_Cs(rCsd)
	{
		R_Cs.Enter();
	}
	~SCriticalSection()
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
	SWaitableObject();
	virtual ~SWaitableObject();
	bool   IsValid() const;
	operator HANDLE() const { return H; }
	bool   FASTCALL operator == (const SWaitableObject &) const;
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
	explicit SWaitableObject(HANDLE h);
	HANDLE H;
};
//
//
//
class DirChangeNotification : public SWaitableObject {
public:
	DirChangeNotification(const char * pName, int watchSubtree, long filtFlags);
	virtual ~DirChangeNotification();
	int    Next();
};
//
//
//
class SMutex : public SWaitableObject {
public:
	SMutex(int initialValue, const char * pName = 0);
	int    Release();
};
//
//
//
class STimer : public SWaitableObject {
public:
	STimer(const char * pName = 0);
	int    Set(const LDATETIME & rDtm, long periodTime);
	int    Cancel();
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
	explicit Evnt(const char * pName, int mode = modeCreate);
	explicit Evnt(int mode = modeCreate);
	//
	// Descr: Переводит событие в занятое состояние
	//
	int    Signal();
	//
	// Descr: Переводит событие в свободное состояние
	//
	int    Reset();
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
	explicit Sem(const char * pName, int mode = modeCreate, int initVal = 0);
	explicit Sem(int initVal = 0);
	int    Release(int count = 1);
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
	BlockingCounter();
	int    Value() const;
	int    IsClear() const;
	//
	// Descr: Blocks until the counter is clear
	//
	int    WaitUntilClear();
	//
	// Descr: Blocks until the counter is dirty
	//
	int    WaitUntilDirty();
	//
	// Descr: Non-blocking increment
	//
	int    Increment();
	//
	// Descr: Blocking decrement
	//
	int    BlockingDecrement();
	BlockingCounter & operator++ ();
	BlockingCounter & operator-- ();
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
	SemiMutex();
	//
	// Descr: Share execution with other "readers".
	//
	int    ReadLock();
	//
	// Descr: Terminate a shared execution.
	//
	int    ReadUnlock();
	//
	// Descr: Exclusive lock
	//
	int    Lock();
	//
	// Descr: Release an exclusive lock
	//
	int    Unlock();
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
	void   Lock() { Cs.Enter(); }
	int    TryLock() { return Cs.TryEnter(); }
	void   Unlock() { Cs.Leave(); }
private:
	SCriticalSection::Data Cs;
};
//
//
//
class ReadWriteLock {
public:
	ReadWriteLock();
	~ReadWriteLock();
	//
	// Descr: Блокирует объект на чтение с ожиданием в течении timeout миллисекунд.
	// ARG(timeout IN): время ожидания в миллисекундах. Если timeout < 0, то бесконечное ожидание.
	// Returns:
	//   >0 - блокировка успешно установлена
	//   <0 - блокировка не установлена из-за превышения ожидания timeout ms
	//   0  - ошибка
	//
	int    FASTCALL ReadLockT_(long timeout);
	//
	// Descr: Блокирует объект на запись с ожиданием в течении timeout миллисекунд.
	// ARG(timeout IN): время ожидания в миллисекундах. Если timeout < 0, то бесконечное ожидание.
	// Returns:
	//   >0 - блокировка успешно установлена
	//   <0 - блокировка не установлена из-за превышения ожидания timeout ms
	//   0  - ошибка
	//
	int    FASTCALL WriteLockT_(long timeout);
	//
	// Descr: Блокирует объект на чтение с бесконечным ожиданием.
	// Returns:
	//   >0 - блокировка успешно установлена
	//   0  - ошибка
	//
	int    ReadLock_();
	//
	// Descr: Блокирует объект на запись с бесконечным ожиданием.
	// Returns:
	//   >0 - блокировка успешно установлена
	//   0  - ошибка
	//
	int    WriteLock_();
	//
	// Descr: Снимает блокировку. Функция унифицированная и для блокировки на чтение и на запись.
	//   Функция должна быть вызвана после выполнения работы, следующей за WriteLock(T) или ReadLock(T).
	//
	int    Unlock_();
private:
	friend class SReadWriteLocker;
	int    FASTCALL Helper_ReadLock(long timeout);
	int    FASTCALL Helper_WriteLock(long timeout);

	SCriticalSection::Data Cs;
	int    ActiveCount; // Текущее число потоков, работающих с ресурсом
		// 0 - нет, >0 - читатели, <0 - писатели
	int    Dr; // Number of delayed readers
	int    Dw; // Number of delayed writers
	Sem    Sr; // Семафор для приостановки читателей
	Sem    Sw; // Семафор для приостановки писателей
};
//
// Descr: Вспомогательный класс, обеспечивающий удобную автоматизацию управления RW-блокировками.
//
class SReadWriteLocker {
public:
	//
	// Descr: Тип блокировки
	//
	enum Type {
		None = 0,  // Без блокировки
		Read = 1,  // Блокировка на чтение
		Write = 2  // Блокировка на запись
	};
	enum {
		stError   = 0x0001,
		stTimeout = 0x0002,
		stRLocked = 0x0004,
		stWLocked = 0x0008,
		stTraced  = 0x0010  // Объект трассируется отладочным стеком (необходим для правильной работы деструктора)
	};
	SReadWriteLocker(ReadWriteLock & rL, Type t, long timeout = -1);
	//
	// Descr: Специальный вариант конструктора для внесения информации о блокировке
	//   в отладочный стек.
	//
	SReadWriteLocker(ReadWriteLock & rL, Type t, long timeout, const char * pSrcFileName, uint srcLineNo);
	~SReadWriteLocker();
	int    operator !() const;
	int    GetState() const;
	int    FASTCALL Toggle(Type t);
	int    Toggle(Type t, const char * pSrcFileName, uint srcLineNo);
private:
	void   FASTCALL InitInstance(Type t);
	int    Unlock();

	ReadWriteLock & R_L;
	const  long Timeout;
	int    State;
};
//
// Descr: Отладочный класс, ведущий учет активных внутренних блокировок
//   таких как CRITICAL-SECTION, READ-WRITE etc.
//   Предназначен для того, чтобы идентифицировать (или доказать отсутствие) взаимных блокировок.
//
class SLockStack {
public:
	enum {
		ltNone = 0,
		ltCS = 1,
		ltRW_R,
		ltRW_W
	};
	//
	// Descr: Транслирует тип блокировки SReadWriteLocker::Type в SLockStack::ltXXX
	//
	static uint FASTCALL WRLT_to_LSLT(SReadWriteLocker::Type wrlt);
	SLockStack();
	void   Push(uint lockType, const char * pSrcFileName, uint lineNo);
	void   Pop();
	void   ToStr(SString & rBuf) const;
private:
	struct Entry {
        long   SrcFileSymbId; // Значение, ассоциированное с именем исходного файла, занесенным в SlSession::GlobSymbList
        uint32 SrcLineNo;
        uint64 TimeCount;
        int    LockType;
	};
	TSStack <Entry> S;
	SString TempBuf;
};
//
// Экземпляр SCriticalSection::Data создается с признаком
// DontDestroyOnDestruction который припятствует "преждевременному"
// разрушению внутреннего объекта.
// Это поможет избежать проблемы, когда какой-либо объект, разрушаемый при
// завершении программы, обращается к критической секции, которая была
// разрушена раньше (программа в этом случае выдает исключение, что раздражает).
//
#if SLTRACELOCKSTACK
	#define ENTER_CRITICAL_SECTION { static SCriticalSection::Data __csd(1); SLS.LockPush(SLockStack::ltCS, __FILE__, __LINE__); SCriticalSection __cs(__csd);
	#define LEAVE_CRITICAL_SECTION SLS.LockPop(); }
	#define DO_CRITICAL(op) { static SCriticalSection::Data __csd(1); SLS.LockPush(SLockStack::ltCS, __FILE__, __LINE__); SCriticalSection __cs(__csd); op; SLS.LockPop(); }
#else
	#define ENTER_CRITICAL_SECTION { static SCriticalSection::Data __csd(1); SCriticalSection __cs(__csd);
	#define LEAVE_CRITICAL_SECTION }
	#define DO_CRITICAL(op) { static SCriticalSection::Data __csd(1); SCriticalSection __cs(__csd); op; }
#endif

#define SRWLOCKERVAR__                 _rwl
#if SLTRACELOCKSTACK
	#define SRWLOCKER(lck, typ)            SReadWriteLocker SRWLOCKERVAR__((lck), (typ), -1, __FILE__, __LINE__)
	#define SRWLOCKERTIMEOUT(lck, typ, to) SReadWriteLocker SRWLOCKERVAR__((lck), (typ), (to), __FILE__, __LINE__)
	#define SRWLOCKER_TOGGLE(typ)          SRWLOCKERVAR__.Toggle((typ), __FILE__, __LINE__)
#else
	#define SRWLOCKER(lck, typ)            SReadWriteLocker SRWLOCKERVAR__((lck), (typ))
	#define SRWLOCKERTIMEOUT(lck, typ, to) SReadWriteLocker SRWLOCKERVAR__((lck), (typ), (to))
	#define SRWLOCKER_TOGGLE(typ)          SRWLOCKERVAR__.Toggle((typ))
#endif
//
//
//
class SAlloc {
public:
	struct VcCrtMemBlockHeader { // For research only! Блок распределенной памяти, используемый в vc 2017. 
		VcCrtMemBlockHeader * _block_header_next;
		VcCrtMemBlockHeader * _block_header_prev;
		char const * _file_name;
		int    _line_number;
		int    _block_use;
		size_t _data_size;
		long   _request_number;
		uchar  _gap[/*no_mans_land_size*/4];
		// Followed by:
		// unsigned char    _data[_data_size];
		// unsigned char    _another_gap[no_mans_land_size];
	};	
	class Stat { // @v10.5.12 @construction
	public:
		struct AllocEntry {
			uint   Size;
			uint64 Count;
		};
		struct ReallocEntry {
			uint   FromSize;
			uint   ToSize;
			uint64 Count;
		};
		Stat();
		void    FASTCALL RegisterAlloc(uint size);
		void    RegisterRealloc(uint fromSize, uint toSize);
		void    FASTCALL Merge(const Stat & rS);
		void    Shrink();
		int     Output(SString & rBuf);
	private:
		TSVector <AllocEntry> AL;
		TSVector <ReallocEntry> RL;
		SMtLock Lck_A; // AL-vector lock
		SMtLock Lck_R; // RL-vector lock
	};
	//
	// Descr: Функция, замещающая malloc.
	//
	static void * FASTCALL M(size_t sz);
	//
	// Descr: Функция, замещающая calloc.
	//
	static void * FASTCALL C(size_t n, size_t sz);
	//
	// Descr: Функция, замещающая realloc.
	//
	static void * FASTCALL R(void * ptr, size_t sz);
	//
	// Descr: Функция, замещающая free.
	//
	static void   FASTCALL F(void * ptr);
};
//
// Descr: Канал (pipe) позволяющий писать с одного конца и читать с другого.
//   Допускается не более одного читателя и не более одного писателя одновременно.
//   Естественно, читать и писать одновременно нельзя. Запрет реализуется простой
//   блокировкой SMtLock.
//   В целом, полностью повторяет по внутреннему устройству класс SBuffer с дополнением
//   в виде блокировки основных примитивов Put и Get.
// Attention: Предназначен только для взаимодействия внутри одного процесса.
//   Для межпроцессного взаимодействия не пригоден!.
//
class SBufferPipe : private SBuffer {
public:
	enum {
		statusEOT = 0x0001
	};
	SBufferPipe(size_t initSize = 0, long flags = fMovable);
	//
	// Descr: Записывает в буфер данные, на которые указывает pSrc в размере srcLen.
	//
	int    FASTCALL Put(const void * pSrc, size_t srcLen);
	//
	// Descr: Возвращает количество доступных для считывания байт буфера.
	// Note: Функция non-const в отличии от тезки в базовом классе (из-за блокировки).
	//
	size_t GetAvailableSize();
	//
	// Descr: Считывает в буфер pBuf, начиная со смещения RdOffs, доступное количество
	//   байт данных, но не более чем bufLen.
	//   Увеличивает указатель RdOffs на количество байт, скопированных в буфер pBuf.
	// Returns:
	//   Количество считанных байт.
	//
	size_t FASTCALL Get(void * pBuf, size_t bufLen);
	//
	// Descr: Сбрасывает в ноль указатели WrOffs и RdOffs.
	//   Таким образом, после выполнения этой операции буфер сохраняет
	//   свой изначальный распределенный размер, но пуст.
	//
	void   Reset();
	long   GetStatus();
	void   SetStatus(long st, int set);
private:
	long   Status;
	SMtLock Lck;
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
// Descr: Шаблонная функция, позволяющая реализовать глобальный циклический
//   стек для обеспечения примитивов, оперирующих простыми объектами, размеры
//   которых превышают возможности системного стека.
//   Например, такой стек позволяет реализовать быстрые операции над
//   точками координат (@see: SPoint2R & FASTCALL operator + (const SPoint2R & one, const SPoint2R & two)).
// @attention
//
template <class T, uint S> T & FASTCALL PushRecycledObject(T & rObj)
{
	static TSRingStack <T> * P_Stack; // @global @threadsafe
	T * p_ret = 0;
	ENTER_CRITICAL_SECTION
		if(SETIFZ(P_Stack, new TSRingStack <T> (S))) {
			P_Stack->push(rObj);
			p_ret = &P_Stack->peek();
		}
	LEAVE_CRITICAL_SECTION
	return *p_ret;
}
//
//
//
class SProfile {
public:
	class Measure {
	public:
		Measure();
		uint64 Get();
	private:
		uint64 Start;
	};
	static __int64 NSec100Clock();

	explicit SProfile(int singleThreaded = 0);
	~SProfile();
	uint64 GetAbsTimeMicroseconds();
protected:
	uint64 Helper_GetAbsTimeMicroseconds();

	const  int SingleThreaded;
	int64  StartClock; // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	int64  EndClock;   // время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	// @v11.2.0 uint64 ClockFrequency; // result of QueryPerformanceFrequency()
	//
	// Descr: Специализированный блок переменных, используемых только функцией Helper_GetAbsTimeMicroseconds
	//
	struct GetTimeBlock {
		uint64 StartHrc;       // start of QueryPerformanceCounter()
		uint64 PrevHrc;        //
		uint32 StartTick;      // start of GetTickCount()
	};
	GetTimeBlock Gtb;
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
	virtual void * Create() { return new T(); }
	virtual void Destroy(void * ptr) { delete static_cast<T *>(ptr); }
};
//
//
//
typedef HANDLE ThreadHandle;
typedef DWORD  ThreadID;
typedef uint   ThreadProcReturn;
#define THREADPROCCALL __stdcall
#define SLTHREAD_DEFAULT_STOP_TIMEOUT 30000

class SlThread {
public:
	//
	// Descr: Initialize the data members without creating an execution thread
	//
	explicit SlThread(void * pInitData = 0, long stopTimeout = SLTHREAD_DEFAULT_STOP_TIMEOUT);
	virtual ~SlThread();
	int    IsConsistent() const;
	operator HANDLE() const { return Handle; }
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
	void   Stop(long timeout);
	//
	// Descr: Terminate the execution thread brutally
	//
	int    Terminate();
	int    WaitUntilFinished();
	//
	// Descr: This implementation is guaranteed to be cancelable.
	//
	void   FASTCALL Sleep(uint milliseconds);
	void   SetStopState();
	ThreadID GetThreadID() const { return ID; }
	bool   IsRunning() const;
	bool   IsIdle() const;
	//
	// Descr: Если поток находится в состоянии остановки, то возвращает !0,
	//   в противном случае - 0.
	//
	bool   IsStopping() const;
protected:
	//
	// Descr: This method is invoked on behalf of the new thread before Run()
	//
	virtual void Startup();
	//
	// Descr: This method is invoked on behalf of the dying thread after Run()
	//
	virtual void Shutdown();
	//
	// Descr: This represents the main thread proc of the thread. It is invoked
	//   after Startup() and before Shutdown(). The default implementation is void.
	//
	virtual void Run();
	void   Reset(void * pInitData = 0, int withForce = 0, long stopTimeout = SLTHREAD_DEFAULT_STOP_TIMEOUT);
	int    InitStartupSignal();
	int    SignalStartup();
	void   SetIdleState();
	void   ResetIdleState();

	volatile void * P_InitData;
	const void * P_Tla; //
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
	volatile long State_Slt;
	volatile long StopTimeout; // Таймаут на остановку потока (ms)
	volatile ThreadHandle Handle;
	volatile ThreadID ID;
	Evnt * P_Creation;
	Evnt * P_StartupSignal;
};
//
// Descr: Утилитарный класс, реализующий функционал инициации и запуска StartupSignal.
//   Используется в простых потоках, предназначенных для распараллеливания тяжелых в вычислительном смысле задач.
//
class SlThread_WithStartupSignal : public SlThread {
public:
	explicit SlThread_WithStartupSignal(void * pInitData = 0, long stopTimeout = SLTHREAD_DEFAULT_STOP_TIMEOUT);
protected:
	virtual void Startup();
};

#endif // } _WIN32_WCE
//
//
//
#ifndef _WIN32_WCE

struct SGlobalSecureConfig {
	SGlobalSecureConfig();
	bool   IsEmpty() const;

	long   Flags;
	SString CaFile;
	SString CaPath;
};

//void * FASTCALL SGetTls(const long idx);
FORCEINLINE void * FASTCALL SGetTls(const long idx)
{
#ifdef NDEBUG
	return TlsGetValue(idx);
#else
	void * p = TlsGetValue(idx);
	if(p)
		return p;
	else {
		assert(0);
		return 0;
	}
#endif
}
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
typedef int (*CallHelpFunc)(void * hWnd, uint cmd, uint ctx);
typedef int (*CallCalculatorFunc)(void * hParentWnd, const char * pInitValue);
typedef int (*CallCalendarFunc)(void * hParentWnd, LDATE * pDate);
typedef int (*GetGlobalSecureConfigFunc)(SGlobalSecureConfig * pCfg);
typedef int (*GetDefaultEncrKeyFunc)(SString & rBuf);
//
// Descr: Функция, если определена, должна возвращать каталоги в соответствии с заданной
//   сигнатурой pSignature.
// ARG(pSignature IN): символьное обозначение запрашиваемого каталога.
//   Сигнатура не чувствительна к регистру символов.
//   Возможны следущие значения:
//   "bin" - бинарный каталог (тот, где находится исполняемый файл)
//   "local" - каталог локальный данных (зависит от операционной системы)
//   "temp" - каталог временных файлов
//   "in"   - каталог входящих данных
//   "out"  - каталог исходящих данных
//   "log"  - каталог файлов журналов
//   "testroot"  - головной каталог тестовых данных
//   "workspace" - головной каталог рабочего пространства пользователя
// ARG(rBuf      OUT): буфер, в который функция должна записать запрашиваемый путь.
//
typedef int (*QueryPathFunc)(const char * pSignature, SString & rBuf);
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
	QueryPathFunc F_QueryPath;
};
//
// Descr: Флаги пользовательского интерфейса (SlThreadLocalArea::UiFlags)
//
enum {
	sluifUseLargeDialogs = 0x0001 // Применять диалоги большого размера (ситуативное использование)
};

class SlThreadLocalArea {
public:
	SlThreadLocalArea();
	~SlThreadLocalArea();
	void   Destroy();
	int    RegisterTempFileName(const char *);
	void   RemoveTempFiles(bool dontStoreFailedItems);
	//
	// Descr: Возвращает контекст вывода, используемый
	//   для работы с Windows-функциями штрифтов.
	//
	HDC    GetFontDC();
	TVRez * GetRez();
	//
	// Descr: Устанавливает позицию левого верхнего угла диалогового окна, которое
	//   потенциально может быть открыто. Если left < 0 || top < 0, то при создании
	//   нового окна диалога программа будет считать, что такая позиция не определена.
	//
	void   SetNextDialogLuPos(int left, int top);
	//
	// Descr: Возвращает рекомендуемую позицию левого верхнего угла вновь открытого
	//   диалогового окна.
	// Note: Эта функция сразу же сбрасывает значение этого угла в неопределенное.
	//
	SPoint2S GetNextDialogLuPos();

	long   Id;
	int    LastErr;
	int    LastOsErr;       // Код ошибка операционной системы (сохраняет значение в момент инициирования LastErr = SLERR_WINDOWS)
	int    LastSockErr;     // Код последней ошибки WinSock
	int    LastCurlErr;     // Код последней ошибки библиотеки LibCURL
	int    BinDateFmt_;
	long   TxtDateFmt_;     //
	int    DefaultYear_;
	int    DefaultMonth_;
	int    UiLanguageId;
	long   UiFlags;            // sluifXXX
	SCodepageIdent CurrentCp;  //
	CompFunc SAry_OrgFCMP;
	int    SAry_PtrContainer;
	void * SAry_SortExtraData;
	SString AddedMsgString; // @anchor
	SString LogPath;        //
	StringSet TempFileList;
	SRandGenerator Rg;      // Генератор случайных чисел общего пользования. Инициализируется по моменту создания потока.
	SGlobalSecureConfig Gsc; // Конфигурация SSL, шифрования и др. для секьюритизации глобального обмена данными
	SRevolver_SString  RvlSStA; // Револьверная коллекция shortlived-строк
	SRevolver_SStringU RvlSStW; // Револьверная коллекция shortlived-строк(unicode)
	SProfile Prf;            // Локальный по отношению к потоку профайлер
	SLockStack LckStk;       // Отладочный стэк блокировок
private:
	TVRez * P_Rez;
	HDC    FontDc;          // Искусственный глобальный контекст вывода для манипулирования шрифтами.
		// (Window-функции, оперирующие со шрифтами требуют HDC с установленным в нем текущим шрифтом).
	SPoint2S NextDialogLuPos;  // Позиция, в которую необходимо установить верхний левый угол диалога.
		// Функция вызова диалога сразу после получения позиции сбрасывает ее в {-1, -1}
};

class SlSession {
public:
	SlSession();
	~SlSession();
	void   Init(const char * pAppName, HINSTANCE hInst = NULL);
	//
	// Descr: Устанавливает наименование приложения, ассоциированное с данным сеансом.
	// Note: В общем случае правильно указывать наименование приложения аргументом функции SlSession::Init,
	//   однако, если это не возможно по каким либо причинам, то это следует сделать вызовом данной функции.
	//
	void   SetAppName(const char * pAppName);
	int    InitWSA(); // @>>::WSAStartup
	int    InitSSL();
	//
	// Descr: Инициализирует новый поток. Функция должна быть вызвана
	//   при создании основного потока. Каждый новый поток так же должен вызвать
	//   эту функцию.
	// Returns: Указатель на локальную область данных потока SlThreadLocaArea.
	// Attention: Ни в коем случае нельзя менять никаких данных по этому указателю
	//   он может быть использован исключительно для промотра информации в отладочных целях.
	//
	const void * InitThread();
	void   ReleaseThread();
	FORCEINLINE SlThreadLocalArea & GetTLA() { return *static_cast<SlThreadLocalArea *>(SGetTls(TlsIdx)); }
	FORCEINLINE const SlThreadLocalArea & GetConstTLA() const { return *static_cast<SlThreadLocalArea *>(SGetTls(TlsIdx)); }
	const SString & GetExePath() const;
	const SString & GetAppName() const;
	void   SetUiLanguageId(int languageId, int currentThreadOnly);
	int    GetUiLanguageId() const;
	SString & GetStopEventName(SString & rBuf) const;
	//
	// Descr: Устанавливает флаг StopFlag и сигнализирует
	//   событием GetStopEventName() для того, чтобы все потоки
	//   завершили свою работу.
	// Returns:
	//   >0 - переключение в режим останова успешно реализовано
	//   <0 - класс уже находился в режиме останова
	//
	int    Stop();
    HINSTANCE GetHInst() const { return H_Inst; }
	//
	// Descr: Отменяет режим останова для процесса
	//
	int    ResetStopState();
	//
	// Descr: Проверяет, находится ли класс в режиме останова процесса
	//
	int    CheckStopFlag() const;
	void   FASTCALL SetExecutionContext(const char * pInfo);
	void   FASTCALL SetAddedMsgString(const char *);
	//
	// Returns: false
	//
	bool   FASTCALL SetError(int errCode, const char * pAddedMsg);
	//
	// Descr: Функция должна вызываться в случае, если ошибка описывается переменной errno.
	//   Фактически будет вызвана SetError(SLERR_ERRNO_OFFSET+errno, pAddedMsg)
	//
	bool   FASTCALL SetErrorErrno(const char * pAddedMsg);
	//
	// Returns: false
	//
	bool   FASTCALL SetError(int errCode);
	//
	// Returns: false
	//
	bool   FASTCALL SetOsError(const char * pAddedMsg = 0);
	int    GetOsError() const;
	const  SString & GetAddedMsgString() const;
	int    LogMessage(const char * pFileName, const char * pStr, ulong maxFileSize = 0);
	int    InitGdiplus();
	void   ShutdownGdiplus();
	int    LoadString_(const char * pSignature, SString & rBuf) const;
	int    ExpandString(SString & rBuf, int ctransf) const;
	int    SubstString(const char * pSrcStr, int ansiCoding, SString & rBuf); // @>>this->LoadString
	//
	// Descr: Обращается к функции запроса каталога, предоставляемой модулем верхнего уровня.
	// ARG(pSignature IN): текстовая сигатура запрашиваемого каталога.
	//   Список допустимых значений см. в описании callback-функции int (*QueryPathFunc)(const char *, SString &)
	//   Сигнатура не чувствительная к регистру символов.
	// ARG(rBuf OUT): Буфер, в который заносится значение каталога в случае успешного
	//   выполнения функции. При ошибке этот буфер усекается до нулевой длины.
	// Returns:
	//   !0 - запрос каталога успешно выполнен
	//   0  - ошибка (в т.ч. возможно, из-за того, что модуль верхнего уровня не предоставил
	//      соответствующей функции.
	//
	int    QueryPath(const char * pSignature, SString & rBuf) const;
	int    FASTCALL SetCodepage(SCodepage cp);
	SCodepage GetCodepage() const;
	int    CallHelp(void * hWnd, uint cmd, uint ctx);
	int    RegisterTempFileName(const char *);
	void   RemoveTempFiles();
	uint   CreateGlobalObject(SClassWrapper & rCls);
	int    DestroyGlobalObject(uint idx);
	void * FASTCALL GetGlobalObject(uint idx);
	//
	// Descr: Находит или, если нет, то создает ассоциацию глобального символа pSymb с целочисленным
	//   идентификатором ident.
	//   Если pSymb == 0 && ident > 0 функция ищет существующий символ по идентификатору ident
	//     и возвращает его по указателю pRetSymb.
	//   Если символ pSymb не найден, а ident == 0, то ассоциирует pSymb со значением,
	//     полученным увеличением внутреннего счетчика.
	//   Если символ pSymb не найден, а ident > 0, то ассоциирует pSymb с заданным значением ident.
	//   Если символ pSymb не найден, а ident < 0, то возвращает (<0).
	//
	// Returns:
	//   >0 - идентификатор, ассоциированный с pSymb
	//   <0 - если pSymb не найден, а ident < 0. Для случая, когда pSymb == 0 и ident > 0
	//     это означает, что идентификатор ident в таблице не найден.
	//   0  - возможен один из следующих случаев:
	//        -- если pSymb найден, ident > 0, но найденный идентификатор не равен ident
	//        -- если pSymb не найден и не удалось создать новую ассоциацию {pSymb, ident}
	// Note: Функция реализована как 2-in-1 (поиск по символу и по идентификатору) ради общей
	//   критической сессии, замкнутой внутри функции.
	//   Если бы функций было 2, пришлось бы держать отдельный объект критической сессии.
	//
	long   GetGlobalSymbol(const char * pSymb, long ident, SString * pRetSymb); // @cs
	const  S_GUID & GetSessUuid() const { return SessUuid; }
	//
	// Descr: Возвращает инкрементируемое целочисленное значение, уникальное в пределах данного процесса.
	//
	int64  GetSequenceValue();
	//const  SSystem & GetSSys() const { return SSys; }
	//
	// Descr: Возвращает абсолютное время в микросекундах для целей профилирования //
	// Note: использует локальный для потока объект SlThreadLocalArea::Prf
	//
	uint64 GetProfileTime();
	//
	// Descr: Устанавливает или снимает (в зависимости от параметра set) флаг пользовательского
	//   интерфейса для текущего потока.
	// Returns:
	//   Предыдущее значение поля флагов пользовательского интерфейса текущего потока.
	//
	long    SetUiFlag(long f, int set);
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
	int    SetupDragndropObj(int ddoType, void * pObj);
	int    IsThereDragndropObj(void ** ppObj);
	const  SGlobalSecureConfig & GetGlobalSecureConfig();
	void   SetLogPath(const char *);
	SString & GetLogPath(SString & rPath) const;
	void   GetExtraProcBlock(SlExtraProcBlock * pBlk) const;
	void   SetExtraProcBlock(const SlExtraProcBlock * pBlk);
	void   LockPush(int lockType, const char * pSrcFileName, uint srcLineNo);
	void   LockPop();
	//
	// Descr: Отладочная функция, насыщающая револьверный пул строк ради того, чтобы устранить его влияние на анализ утечки памяти.
	//
	void   SaturateRvlStrPool(uint minSize); // @debug
	//
	// Descr: То же, что и SaturateRvlStrPool но для пула unicode-строк
	//
	void   SaturateRvlStrUPool(uint minSize); // @debug
	SString  & AcquireRvlStr();
	SStringU & AcquireRvlStrU();
	SAlloc::Stat & GetAllocStat() { return AllocStat; }
private:
	long   TlsIdx;          // @firstmember
public:
	const  SSystem SSys;    // 
private:
	long   Id;              // @anchor
	ACount SeqValue;        // @anchor Последнее значение, возвращенное функций GetSequenceValue().
		// Положение переменной должно быть выровнено по 64-битной границе.
	int    WsaInitCounter;
	ACount SslInitCounter;  //
	int    StopFlag;        //
	uint   DragndropObjIdx; // Индекс объекта для перетаскивания в таблице глобальных объектов GlobalObjList
	uint32 HelpCookie;
	Evnt * P_StopEvnt;
	S_GUID SessUuid;        // Уникальный идентификатор сессии
	int    UiLanguageId;
	SlExtraProcBlock ExtraProcBlk;
	SString ExePath;
	SString AppName;
	HINSTANCE H_Inst;
	ULONG_PTR GdiplusToken;

	class GlobalObjectArray : public SVector {
	public:
		GlobalObjectArray();
		~GlobalObjectArray();
		void   Destroy();
		uint   CreateObject(SClassWrapper & rCls);
		int    DestroyObject(uint idx);
		void * FASTCALL GetObject(uint idx);
	private:
		SCriticalSection::Data Cs;
	};

	GlobalObjectArray GlobObjList;
	SymbHashTable GlobSymbList;
	SAlloc::Stat AllocStat;
};

extern SlSession SLS;

#define SLibError    (SLS.GetTLA().LastErr)
#define BinDateFmt   (SLS.GetTLA().BinDateFmt_)
#define TxtDateFmt   (SLS.GetTLA().TxtDateFmt_)
#define DefaultYear  (SLS.GetTLA().DefaultYear_)
#define DefaultMonth (SLS.GetTLA().DefaultMonth_)
#define P_SlRez      (SLS.GetTLA().GetRez())
//
// Descr: Объект, держащий относительно стабильное представление сущности, хранимой в файле.
//   Нужен для того, что бы не обращаться каждый раз за таким представлением к исходному файлу, но,
//   если файл вдруг изменился, то перестроить сущность и работать с ней дальше.
//   Примеры (потенциального) использования: pp.ini, stqc.xml etc.
//
class SCachedFileEntity {
public:
	SCachedFileEntity();
	virtual ~SCachedFileEntity();
	const char * GetFilePath() const;
	int    Init(const char * pFilePath);
	//
	// Descr: Выполняет проверку времени модификации файла и, если файл был изменен, то запускает 
	//   виртуальные функции DestroyEntity() и InitEntity().
	//   Функция защищена критической секцией поэтому может вызываться из разных потоков.
	// ARG(force IN): Если true, то перезагрузка entity осуществляется не взирая на время модификации файла
	// Returns:
	//   <0 - файл не был модифицирован, следовательно и попытки перезагрузки не предпринималось.
	//   >0 - перезагрузка осуществлена успешно
	//    0 - ошибка
	//
	int    Reload(bool force);
protected:
	virtual bool InitEntity();
	virtual void DestroyEntity();
	bool   InitFileModTime();
	bool   IsModified();
	void * P_Entity;
	mutable SMtLock Lck; // Блокирующи объект, используемый одновременно в общем методе Reload() и в методах получения копии 
		// сущности, реализуемых порожденными классами.
private:
	enum {
		stError       = 0x0001,
		stInitialized = 0x0002
	};
	uint   State;
	SString FilePath;
	LDATETIME ModTime; // Время последней модификации файла
};
//
// Descr: Интерфейс к некоторым методам библиотеки GDI+ (Microsoft)
//
class SImage {
public:
	SImage();
	~SImage();
	int    Init();
	bool   IsValid() const;
	int    Load(const char * pPicPath);
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
		SPoint2F Dpi;       // Разрешение экрана в точках на дюйм
		float  FontSize;
	};

	explicit SDrawContext(cairo_t * pCr);
	explicit SDrawContext(HDC);
	operator SDrawSystem () const { return S; }
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
		fEmpty = 0x0001, // ViewPort не определен
		fSlice = 0x0002, // else Meet
		fDontScale     = 0x0004, // Не масштабировать объект
		fDontEnlarge   = 0x0008, // Не увеличивать объект (можно только уменьшать)
	};

	explicit SViewPort(uint flags = 0);
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
		static COLORREF FASTCALL UniformToRGB(uint32 u);
		explicit PixF(int s/* = 0*/);
		bool   IsValid() const;
		uint   GetBpp() const;
		uint   FASTCALL GetStride(uint width) const;
		int    GetUniform(const void * pSrc, void * pUniformBuf, uint width, const Palette * pPalette) const;
		int    SetUniform(const void * pUniformBuf, void * pDest, uint width, Palette * pPalette) const;

		int32  S;
	};
	struct StoreParam {
		explicit StoreParam(int fmt);

		enum {
			fInterlaced = 0x0001
		};
		int   Fmt;
		long  Flags;
		uint  Quality; // [1..100]
	};

	static bool FASTCALL IsSupportedFormat(int fm);

	SImageBuffer();
	SImageBuffer(const SImageBuffer & rS);
	SImageBuffer(uint w, uint h); // pixformat: PixF::s32ARGB
	SImageBuffer(uint w, uint h, PixF f);
	~SImageBuffer();
	SImageBuffer & FASTCALL operator = (const SImageBuffer & rS);
	void   Destroy();
	int    Init(uint w, uint h); // pixformat: PixF::s32ARGB
	int    Init(uint w, uint h, PixF f);
	bool   FASTCALL IsEq(const SImageBuffer & rS) const;
	bool   FASTCALL Copy(const SImageBuffer & rS);
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    AddLines(const void * pSrc, PixF s, uint count, const Palette * pPalette);
	PixF   GetFormat() const;
	uint   GetWidth() const;
	uint   GetHeight() const;
	//
	// Descr: Возвращает расчетный размер буфера, определяемый как произведение GetHeight() * GetFormat().GetStride(GetWidth())
	//
	size_t GetNominalBufSize() const;
	SPoint2S GetDim() const;
	SPoint2F GetDimF() const;
	const  uint8 * GetData() const;
	int    GetSubImage(SImageBuffer & rDest, SPoint2S start, SPoint2S size) const;
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
	int    Store(const StoreParam &, SFile & rF) const;
	int    StoreMime_Base64(const StoreParam &, SString & rBuf) const;
	int    TransformToBounds(SPoint2S size, const SViewPort * pVp);
	int    TransformToGrayscale();
	//
	// Descr: Формирует Windows ICON изображение из буфера this.
	// Returns:
	//   0 - error
	//   !0 - абстрактный указатель, который в действительности является HICON
	//
	void * TransformToIcon() const;
	void * TransformToBitmap() const;
private:
	int    LoadJpeg(SFile & rF, int fileFmt);
	int    LoadPng(SFile & rF);
	int    LoadIco(SFile & rF, uint pageIdx);
	int    LoadBmp(SFile & rF);
	int    LoadGif(SFile & rF);
	int    LoadTiff(SFile & rF, int fileFmt);
	int    LoadWebp(SFile & rF);
	int    Helper_Load(SFile & rF, SFileFormat ff);
	int    Helper_LoadBmp(SBuffer & rBuf, const char * pAddedErrorInfo);
	int    StorePng(const StoreParam & rP, SFile & rF) const;
	int    StoreJpeg(const StoreParam & rP, SFile & rF) const;
	int    StoreWebp(const StoreParam & rP, SFile & rF) const;
	int    StoreBmp(const StoreParam & rP, SFile & rF) const;
	int    Flip();
	int    PremultiplyAlpha();
	uint8 * FASTCALL GetScanline(uint lineNo) const;

	PixF   F; // Формат пикселей
	SPoint2S S; // Размер образа
};
//
// Descr: Базовый класс для представления фигур рисования.
//
class SDrawFigure { // @persistent @store(SSerializeContext)
public:
	friend class SDrawGroup;

	static int FASTCALL CheckKind(int kind);
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
		// @v10.4.5 kExtern,    // Объект, использующий внешнее описание SDrawFigure
		kRef,       // @v10.4.5 Фигура, правила отрисовки которой определяются ссылкой на иной объект в контейнера.
			// Кроме собственно ссылки содержит ряд атрибутов, определяющий отрисовку данного экземпляра.
		kText       // Текст
	};
	enum {
		fOuterToolbox = 0x0001, // Объект использует внешний SPaintToolBox. Используется порожденными классами.
		fDraw = 0x0002, // Устанавливается классом SDraw для того,
			// чтобы члены контейнера могли распознать его как родителя, имеющего специфические атрибуты (SPaintToolBox, например).
		fDefinedSize  = 0x0004, // @!{SDrawFigure::SetSize} Размер элемента определен из-вне (то есть, он не должен рассчитываться).
		fNullBrush    = 0x0008, // Без заливки (IdBrush игнорируется)
		fSymbolGroup  = 0x0010  // @v10.4.5 Флаг устанавливается у экземпляров вида kGroup, которые сами по себе не отрисовываются
			// непосредственно, однако могут быть использованы как образцы для отрисовки по ссылке.
	};

	virtual ~SDrawFigure();
	virtual SDrawFigure * Dup() const = 0;
	virtual void SetTransform(const LMatrix2D * pMtx);
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    GetKind() const;
	long   GetFlags() const;
	const  SString & GetSid() const;
	void   SetSid(const char * pSid);
	void   SetStyle(int identPen, int identBrush, long flags);
	int    GetPen() const;
	int    GetBrush() const;
	void   SetViewPort(const SViewPort * pVp);
	void   SetSize(SPoint2F sz);
	//
	// Descr: Возвращает ссылку на внутреннюю структуру SViewPort (порт вывода).
	//
	const  SViewPort & GetViewPort() const;
	//
	// Descr: Присваивает по адресу pVp структуру эффективного порта вывода
	//   (с учетом контекста).
	//
	int    FASTCALL GetViewPort(SViewPort * pVp) const;
	const  SPoint2F   & GetSize() const;
	const  LMatrix2D & GetTransform() const;
	SPaintToolBox * GetToolBox() const;
	int    TransformToImage(const SViewPort * pVp, SImageBuffer & rImg) const;
	SDrawImage * DupToImage(SPoint2S size, const SViewPort * pVp, const char * pSid);
	const  SDrawFigure * SearchRef(const char * pSymb) const;
protected:
	SDrawFigure(int kind, const char * pSid);
	int    FASTCALL Copy(const SDrawFigure &);

	int32  Kind;
	int32  Flags;
	int32  IdPen;   // Если IdPen < 0, то это означает, что при отрисовке необходимо
		// внешнее определение карандаша (либо некоторое default-значение)
	int32  IdBrush;
	SPoint2F Size;
	SViewPort Vp;
	LMatrix2D Tf;
	SDrawFigure * P_Parent; // Родительский контейнер
	SString Sid;
};

class SDrawShape : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	explicit SDrawShape(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawShape & rS);

	FShape S;
};

class SDrawRef : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	explicit SDrawRef(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawRef & rS);

	SString Ref; // Текст, идентифицирующий объект для отрисовки
	SPoint2F Origin;
};

class SDrawText : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	explicit SDrawText(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawText & rS);

	int32  IdFont;
	SPoint2F Begin;  // Левый верхний угол текста. Размер прямоугольника определяется полем SDrawFigure::Size
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
		const  SPoint2F & Pnt(uint p) const { return *reinterpret_cast<const SPoint2F *>(P_ArgList+p); }
		int    Op;
		uint   ArgCount;
		const  float * P_ArgList;
	};
	explicit SDrawPath(const char * pSid = 0);
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawPath & rS);
	bool   IsEmpty() const;
	int    HasCurrent() const;
	const  SPoint2F & GetCurrent();
	void   Clear();
	uint   GetCount() const;
	const Item * FASTCALL Get(uint i, Item &) const;
	int    Nop();
	int    FASTCALL Move(const SPoint2F &);
	int    FASTCALL Line(const SPoint2F &);
	int    Curve(const SPoint2F & rMiddle1, const SPoint2F & rMiddle2, const SPoint2F & rEnd);
	int    Quad(const SPoint2F & rMiddle, const SPoint2F & rEnd);
	int    ArcSvg(const SPoint2F & rCenter, float xAxRotation, int largeFlag, int sweepFlag, const SPoint2F & rEnd);
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
	SPoint2F Cur;      // Текущая точка
	SPoint2F LastMove; // Начало последнего подпути
	LAssocArray OpList; // Key - op, Val - index of arg_list in ArgList;
	FloatArray ArgList;
};

class SDrawImage : public SDrawFigure { // @persistent @store(SSerializeContext)
public:
	explicit SDrawImage(const char * pSid = 0);
	SDrawImage(const SImageBuffer & rBuf, const char * pSid = 0);
	~SDrawImage();
	virtual SDrawFigure * Dup() const;
	virtual int  Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Copy(const SDrawImage & rS);
	int    SetBuffer(const SImageBuffer * pBuf);
	const  SImageBuffer & GetBuffer() const;
	int    LoadFile(const char * pFileName);
	int    LoadBuf(int fm, SBuffer & rInBuf);
	int    LoadMime_Base64(const char * pFormatStr, const SString & rS);
	int    Store(const SImageBuffer::StoreParam & rP, SFile & rF);
	int    TransformToBounds(SPoint2S size, const SViewPort * pVp);
	int    TransformToGrayscale();
private:
	SImageBuffer Buf;
};

class SDrawGroup : public SDrawFigure, private TSCollection <SDrawFigure> { // @persistent @store(SSerializeContext)
public:
	enum {
		dgtOrdinary = 0,
		dgtSymbol   = 1
	};
	explicit SDrawGroup(const char * pSid = 0, int dgt = dgtOrdinary);
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
	explicit SDraw(const char * pSid);
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
	explicit SylkWriter(const char * pFileName = 0);
	~SylkWriter();
	int    Open(const char * pFileName);
	void   Close();
	int    PutVal(const char *, int cvtOemToChr);
	int    PutVal(double);
	void   PutColumnWidth(int start, int end, int width);
	void   PutRec(const char * pTypeStr, const char *);
	void   PutRec(int typeChr, const char *);
	void   FASTCALL PutLine(const char *);
	void   PutFont(int symb, const char * pFontName, int size, uint fontStyle);
	void   PutFormat(const char * pBuf, int fontId, int col, int row);
	int    GetBuf(SString * pBuf) const;
private:
	SString Buf;
	FILE * Stream;
};

class SlVCard {
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
	SlVCard(const char * pFileName = 0, int forExport = 1);
	~SlVCard();
	int    Open(const char * pFileName, int forExport);
	int    Close();
	int    Put(const Rec *);
	int    Get(Rec *);
private:
	int    PutProp(Property prop, const void * pData, PropAttribute attrib = paNone);
	int    GetName(SString &);
	int    GetBirthDate(LDATE *);
	int    GetPhones(SString &);
	int    GetEmails(SString &);
	int    GetAddrs(SString &);

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
	enum {
		tokUnkn     = -1, // Неизвестный токен
		// Calendar properties
		tokCALSCALE = 1, // TEXT. ONCE. Defines the calendar scale used for the calendar information specified in the iCalendar object
		tokMETHOD,       // TEXT. ONCE. Defines the iCalendar object method associated with the calendar object.
		tokPRODID,       // TEXT. ONCE. Specifies the identifier for the product that created the iCalendar object.
		tokVERSION,      // TEXT. ONCE. Specifies the identifier corresponding to the highest version number or the minimum and maximum range of the
			// iCalendar specification that is required in order to interpret the iCalendar object.
		// Component properties
		tokATTACH,          // URI. MULTI. Provides the capability to associate a document object with a calendar component.
		tokCATEGORIES,      // TEXT. Defines the categories for a calendar component.
		tokCLASS,           // TEXT. Defines the access classification for a calendar component.
		tokCOMMENT,         // TEXT. Specifies non-processing information intended to provide a comment to the calendar user.
		tokDESCRIPTION,     // TEXT. Provides a more complete description of the calendar component than that provided by the "SUMMARY" property.
		tokGEO,             // FLOAT:FLOAT. Specifies information related to the global position for the activity specified by a calendar component
		tokLOCATION,        // TEXT. Defines the intended venue for the activity defined by a calendar component.
		tokPERCENTCOMPLETE, // "PERCENT-COMPLETE" INT. This property is used by an assignee or delegatee of a to-do to convey the percent completion
			// of a to-do to the "Organizer".
		tokPRIORITY,        // INT[0..9]. Defines the relative priority for a calendar component.
		tokRESOURCES,       // TEXT. ONCE. Defines the equipment or resources anticipated for an activity specified by a calendar component.
		tokSTATUS,          // TEXT. ONCE. Defines the overall status or confirmation for the calendar component.
			// todo: NEEDS-ACTION || COMPLETED || IN-PROCESS || CANCELLED
			// event: TENTATIVE || CONFIRMED || CANCELLED
			// journal: DRAFT || FINAL || CANCELLED
		tokSUMMARY,         // TEXT. Defines a short summary or subject for the calendar component.
		tokCOMPLETED,       // DATETIME. Defines the date and time that a to-do was actually completed
			                // also: todo status COMPLETED
		tokDTEND,           // DATETIME. Specifies the date and time that a calendar component ends.
		tokDUE,             // DATETIME. Defines the date and time that a to-do is expected to be completed.
		tokDTSTART,         // DATETIME. Specifies when the calendar component begins.
		tokDURATION,        // DURATION. Specifies a positive duration of time.
			// example: DURATION:PT1H0M0S
			// example: DURATION:PT15M
		tokFREEBUSY,        // PERIOD. Defines one or more free or busy time intervals.
			// examples:
			// FREEBUSY;FBTYPE=BUSY-UNAVAILABLE:19970308T160000Z/PT8H30M
			// FREEBUSY;FBTYPE=FREE:19970308T160000Z/PT3H,19970308T200000Z/PT1H
			// FREEBUSY;FBTYPE=FREE:19970308T160000Z/PT3H,19970308T200000Z/PT1H,19970308T230000Z/19970309T000000Z
		tokTRANSP,          // TEXT. ONCE. VEVENT. Defines whether or not an event is transparent to busy time searches.
		tokTZID,            // TEXT. MUSTHAVE at VTIMEZONE. Specifies the text value that uniquely identifies the "VTIMEZONE"
			// calendar component in the scope of an iCalendar object
		tokTZNAME,          // TEXT. Specifies the customary designation for a time zone description.
		tokTZOFFSETFROM,    // UTC-OFFSET. Specifies the offset that is in use prior to this time zone observance.
		tokTZOFFSETTO,      // UTC-OFFSET. Specifies the offset that is in use in this time zone observance.
		tokTZURL,           // URI. Provides a means for a "VTIMEZONE" component to point to a network location that can be used
			// to retrieve an upto-date version of itself.
		tokATTENDEE,        // CAL-ADDRESS. Defines an "Attendee" within a calendar component.
		tokCONTACT,         // TEXT. This property is used to represent contact information or alternately a reference
			// to contact information associated with the calendar component.
		tokORGANIZER,       // CAL-ADDRESS. Defines the organizer for a calendar component.
		tokRECURRENCEID,    // "RECURRENCE-ID"
		tokRELATEDTO,       // "RELATED-TO" TEXT. This property is used to represent a relationship or reference
			// between one calendar component and another.
		tokURL,             // URL. Defines a Uniform Resource Locator (URL) associated with the iCalendar object.
		tokUID,             // TEXT. Defines the persistent, globally unique identifier for the calendar component.
		tokDTSTAMP,         // DATETIME. In the case of an iCalendar object that specifies a "METHOD" property,
			// this property specifies the date and time that the instance of the iCalendar object was created. In the case of
			// an iCalendar object that doesn’t specify a "METHOD" property, this property specifies the date and time that the information
			// associated with the calendar component was last revised in the calendar store.
			// The property value MUST be specified in the UTC time format
		tokLASTMODIFIED,    // "LAST-MODIFIED" DATETIME. Specifies the date and time that the information associated with the calendar
			// component was last revised in the calendar store.
			// The property value MUST be specified in the UTC time format
		tokSEQUENCE,       // INT. Defines the revision sequence number of the calendar component within a sequence of revisions.
		tokDCREATED,       // unspecified. DATETIME.
		tokNEEDSACTION,    // todo status NEEDS-ACTION
		tokINPROCESS,      // todo status IN-PROCESS
		tokCANCELLED,      // todo|event|journal status CANCELLED
		tokTENTATIVE,      // event status TENTATIVE
		tokCONFIRMED,      // event status CONFIRMED
		tokDRAFT,          // journal status DRAFT
		tokFINAL,          // journal status FINAL
		tokCHAIR,          // role CHAIR
		tokREQPARTICIPANT, // role REQ-PARTICIPANT
		tokOPTPARTICIPANT, // role OPT-PARTICIPANT
		tokNONPARTICIPANT, // role NON-PARTICIPANT
		tokCREATED,        // DATETIME. Specifies the date and time that the calendar information was created by the calendar user agent in the calendar store.
		tokXPAPYRUSID,     // unspecified. X-PAPYRUS-ID
		tokXPAPYRUSCODE,   // unspecified. X-PAPYRUS-CODE
		tokBEGIN,          //
		tokEND,            //
		tokVCALENDAR,      //
		tokVEVENT,         //
		tokVTODO,          //
		tokVJOURNAL,       //
		tokVFREEBUSY,      //
		tokVTIMEZONE,      //
		tokVALARM,         //
		tokSTANDARD,       //
		tokDAYLIGHT,       //
		tokRSVP,           // RSVP : for indicating whether the favor of a reply is requested
		tokMEMBER,         // MEMBER : to indicate the groups that the attendee belongs to
		tokROLE,           // ROLE : for the intended role that the attendee will have in the calendar component
		tokPARTSTAT,       // PARTSTAT : for the status of the attendee’s participation
		tokCN,             // CN : for the common or displayable name associated with the calendar address
		tokDELEGATEDFROM,  // DELEGATED-FROM : to indicate whom the request was delegated from
		tokDELEGATEDTO,    // DELEGATED-TO : to indicate the calendar users that the original request was delegated to
		tokDIR,            // DIR : to indicate the URI that points to the directory information corresponding to the attendee
		tokCUTTYP,         // CUTYPE : to indicate the type of calendar user
		tokSENTBY,         // SENT-BY : to indicate whom is acting on behalf of the "ATTENDEE"
		tokLANGUAGE,       // LANGUAGE : property parameter is specified, the identified language applies to the "CN" parameter
		tokXRUINN,         // X-RU-INN
		tokXRUKPP,         // X-RU-KPP
		tokXPHONE          // X-PHONE
	};
	struct Entry {
		Entry();
		Entry & Z();
		int    Token;
		StrAssocArray ParamList;
		SString Value;
	};
	struct Todo {
		Todo();
		void   Init();

		enum {
			textpropDescription = 1, // DESCRIPTION
			textpropComment,         // COMMENT
		};
		enum {
			attendeeMember = 1,    // MEMBER : to indicate the groups that the attendee belongs to
			attendeeRole,          // ROLE : for the intended role that the attendee will have in the calendar component
			attendeePartStat,      // PARTSTAT : for the status of the attendee’s participation
			attendeeCN,            // CN : for the common or displayable name associated with the calendar address
			attendeeDelegatedFrom, // DELEGATED-FROM : to indicate whom the request was delegated from
			attendeeDelegatedTo,   // DELEGATED-TO : to indicate the calendar users that the original request was delegated to
			attendeeDir,           // DIR : to indicate the URI that points to the directory information corresponding to the attendee
			attendeeCuType,        // CUTYPE : to indicate the type of calendar user
			attendeeRSVP,          // RSVP : for indicating whether the favor of a reply is requested
			attendeeSentBy,        // SENT-BY : to indicate whom is acting on behalf of the "ATTENDEE"
			attendeeLanguage,      // LANGUAGE : property parameter is specified, the identified language applies to the "CN" parameter
		};

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
		SString Contact;
		SString Attendee;
		SString Summary;
		SString Location;
		SString Descr;
		// @construction TSCollection <StrAssocArray> AttendeeList;
	};
	explicit VCalendar(const char * pFileName = 0, int forExport = 1);
	~VCalendar();

	static int GetToken(const char * pText);
	static int WriteComponentProlog(int tok, const char * pProduct, const SVerT * pVer, SString & rBuf); // BEGIN:tok
	static int WriteComponentEpilog(int tok, SString & rBuf); // END:tok
	static int WriteToken(int tok, SString & rBuf);
	static int WriteDatetime(LDATETIME dtm, SString & rBuf);
	static SString & PreprocessText(SString & rBuf);
	static int ParseLine(const char * pBuf, Entry & rResult);
	static int ParseDatetime(const char * pBuf, LDATETIME & rDtm, int * pTz);

	int    Open(const char * pFileName, int forExport);
	void   Close();
	int    PutEvent(VCalendar::Event *);
	int    PutTodo(const VCalendar::Todo *);
	int    GetTodo(VCalendar::Todo *);
private:
	int    PutTodoProperty(TodoProperty prop, const void * pVal, long addedParam);
	int    GetDtm(const SString & rBuf, LDATETIME * pDtm);
	int    ReadProp(TodoProperty * pProp, SString & rVal, SString & rAttrib);

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
	SGeoPosLL & Z();
	int    FASTCALL operator == (const SGeoPosLL & rS) const;
	int    FASTCALL operator != (const SGeoPosLL & rS) const;
	int    FASTCALL Cmp(const SGeoPosLL & rS) const;
	bool   IsZero() const;
	bool   IsValid() const;
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
	bool   IsValid() const;
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
	static uint32 STDCALL  Combine(uint16 x, uint16 y);
	static uint32 FASTCALL Combine(const P16 & rP) { return Combine(rP.X, rP.Y); }
	static uint64 STDCALL  Combine(uint32 x, uint32 y);
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
		Finder(const SGeoGridTab & rTab);
        uint   FASTCALL GetIdxLat(long lat);
        uint   FASTCALL GetIdxLon(long lon);
        void   GetIdx(const SGeoPosLL_Int & rC, uint & rIdxLat, uint & rIdxLon);
        uint32 FASTCALL GetZIdx32(const SGeoPosLL_Int & rC);
        uint64 FASTCALL GetZIdx64(const SGeoPosLL_Int & rC);
	private:
		const SGeoGridTab & R_Tab;
        uint   LastPosLat;
        uint   LastPosLon;
	};

	friend class SGeoGridTab::Finder;

	SGeoGridTab(uint dim);
	bool   FASTCALL IsEq(const SGeoGridTab & rS) const;
	bool   FASTCALL operator == (const SGeoGridTab & rS) const;
	bool   FASTCALL operator != (const SGeoGridTab & rS) const;
	void   SetSrcCountLat(uint64 c);
	void   SetSrcCountLon(uint64 c);
	uint   GetDim() const;
	uint   GetDensityLat() const;
	uint   GetDensityLon() const;
	int    FASTCALL AddThresholdLat(long coord);
	int    FASTCALL AddThresholdLon(long coord);
	uint   GetCountLat() const;
	uint   GetCountLon() const;
	//
	int    Save(const char * pFileName);
	int    Load(const char * pFileName);
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
		GEOD_NONE = 0U,                     // Calculate nothing
		GEOD_LATITUDE     = 1U<<7  | 0U,            // Calculate latitude
		GEOD_LONGITUDE    = 1U<<8  | 1U<<3,         // Calculate longitude
		GEOD_AZIMUTH      = 1U<<9  | 0U,            // Calculate azimuth
		GEOD_DISTANCE     = 1U<<10 | 1U<<0,         // Calculate distance
		GEOD_DISTANCE_IN  = 1U<<11 | 1U<<0 | 1U<<1, // Allow distance as input
		GEOD_REDUCEDLENGTH= 1U<<12 | 1U<<0 | 1U<<2, // Calculate reduced length
		GEOD_GEODESICSCALE= 1U<<13 | 1U<<0 | 1U<<2, // Calculate geodesic scale
		GEOD_AREA = 1U<<14 | 1U<<4,         // Calculate reduced length
		GEOD_ALL  = 0x7F80U| 0x1FU          // Calculate everything
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
		SinCosPair Alp1; // sine and cosine of \e azi1
		double A13;   // arc length to reference point
		double S13;   // distance to reference point
		// @cond SKIP
		double B;
		double C2;
		double F1;
		SinCosPair Alp0;
		double K2;
		SinCosPair Sig1;
		double Dn1;
		SinCosPair Tau1;
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
class SRawInputInitArray : private SVector {
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
	int    FASTCALL Get(/*long*/void * rawInputHandle);
	//
	// Descr: Получает строку наименования устройства, из которого приняты данные предшествующим вызовом
	//   this->Get(long).
	//
	int    FASTCALL GetDeviceName(SString & rBuf);
private:
	static int InitRawInputProc(int unreg);
	void   Reset();

	size_t AllocatedSize;
	void * P_Buf;
	uint8  FixedBuffer[256];
};
//
// Descr: Класс для работы с устройствами через USB
//
struct UsbDevDescrSt {
	UsbDevDescrSt();
	UsbDevDescrSt(const UsbDevDescrSt & rSrc);
	UsbDevDescrSt & Z();

	S_GUID ClassGUID;
	SString Path;
	SString Type;
	SString Class;
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
	UsbBasicDescrSt & Z();
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
	SBuffer & GetExtBuf() { return ExtBuffer; }
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
extern bool SlBreakpointCondition[16]; // @v11.2.4 Логические переменные для установки условных точек отладки.

class MemLeakTracer {
public:
	MemLeakTracer();
	~MemLeakTracer();
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
	MemHeapTracer();
	int    CalcStat(Stat * pSize);
	int    Dump(const char * pFileName, long flags);
};

void FASTCALL TraceFunc(const char * pFuncName, const char * pAddedMsg);

#ifdef _DEBUG
	#define TRACE_FUNC()    TraceFunc(__FUNCTION__, 0)
	#define TRACE_FUNC_S(s) TraceFunc(__FUNCTION__, (s))
#else
	#define TRACE_FUNC()
	#define TRACE_FUNC_S(s)
#endif

struct SInvariantParam {
	SInvariantParam();
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
		uint   ValPos[12];
	};
	STestDataArray();
	const  Item & Get(uint idx) const;
	uint   GetCount() const;
	bool   GetDataByPos(uint pos, SString & rData) const;
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
	TSVector <Item> L;
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
		// определен, то считается, что имя файла таблиц совпадает с именем файла описания тестов, но с расширением .tab
	SString CaseBuffer;  // Строка, в которую тест может вывести какую-либо информацию
	MemHeapTracer Mht;
};
//
// Descr: Базовый класс для реализации тест-кейса.
//
class STestCase {
public:
	explicit STestCase(STestSuite *);
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
	void   SetInfo(const char *, int currentStatus = -1);
	int    _check_nz(int result, const char * pV);
	int    _check_z(int result, const char * pV);
	int    _check_eq(const void * a, const void * b, const char * pA, const char * pB);
	int    _check_eq(const void * a, const void * b, size_t sz, const char * pA, const char * pB);
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
	int    _check_lt(ulong a, ulong b, const char * pA, const char * pB);
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
	SEnum::Imp * EnumTab(const char * pTabName);
	int    GetCurrentStatus() const { return CurrentStatus; }
protected:
	const  STestSuite::Entry * GetSuiteEntry() const;
	const char * GetInputPath() const;
	const char * GetOutputPath() const;
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
	SString TempBuf; // @allocreuse

	class TabEnum : public SEnum::Imp {
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
		if(!rStr.IsEmpty()) {
			int    r = rStr.Divide(';', rec, tail);
			if(r) {
				r = rec.Divide('=', s1, s2);
				if(r) {
					if(r > 0) {
						KeyValuePair * p_pair = new KeyValuePair();
						p_pair->Key = s1;
						p_pair->Value = s2;
						Coll.insert(p_pair);
					}
					if(!tail.IsEmpty())
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
#define SLTEST_CHECK_EQMEM(a, b, sz)      _check_eq((a), (b), (sz), #a, #b)
#define SLTEST_CHECK_EQ_TOL(a, b, tol)    _check_eq_tolerance((a), (b), (tol), #a, #b)
#define SLTEST_CHECK_LT(a, b)             _check_lt((a), (b), #a, #b)
#define SLTEST_CHECK_LE(a, b)             _check_le((a), (b), #a, #b)
#define SLTEST_CHECK_CRANGE(a, low, upp)  _check_range((a), (low), (upp), #a, #low, #upp)
#define SLTEST_CHECK_MEM(ptr, size, pattern) _check_mem((ptr), (size), (pattern), #ptr)

#define SLTEST_CLS(s)     SLTC_##s
#define SLTEST_FACTORY(s) SLTCF_##s
#define IMPLEMENT_SLTEST_FACTORY(test) \
	extern "C" __declspec(dllexport) STestCase * SLTEST_FACTORY(test)(STestSuite * pSuite) { return new SLTEST_CLS(test)(pSuite); }
#define DECL_SLTEST(test)                 class SLTEST_CLS(test) : public STestCase { \
	public: SLTEST_CLS(test)(STestSuite * pS) : STestCase(pS) {} virtual int Run(const char * pBenchmark);};
#define DECL_SLTEST_FIXTURE(test,fixture) class SLTEST_CLS(test) : public STestCase { \
	fixture F; public: SLTEST_CLS(test)(STestSuite * pS) : STestCase(pS), F() {} virtual int Run(const char * pBenchmark);};

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
