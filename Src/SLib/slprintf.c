// SLPRINTF.C
// (c) Marco Paland (info@paland.com) 2014-2019, PALANDesign Hannover, Germany
// license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
// embedded systems with a very limited resources. These routines are thread safe and reentrant!
// Use this instead of the bloated standard/newlib printf cause these use malloc for printf (and may not be thread safe).
// ---------------------------------
// Adopted to slib by A.Sobolev 2020..2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop

//int FASTCALL dconvstr_print(char ** ppOutBuf, int * pOutBufSize, double value, int formatChar, uint formatFlags, int formatWidth, int formatPrecision); // @prototype

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack). default: 32 byte
//#define PRINTF_FTOA_BUFFER_SIZE    64U // @sobolev 32-->64
#define PRINTF_DEFAULT_FLOAT_PRECISION  6U // define the default floating point precision. default: 6 digits
//#define PRINTF_MAX_FLOAT  1.0e25f // @sobolev 1e9-->1e25 // define the largest float suitable to print with %f. default: 1e9

// internal flag definitions
#define FLAGS_ZEROPAD   0x0001
#define FLAGS_LEFT      0x0002
#define FLAGS_PLUS      0x0004
#define FLAGS_SPACE     0x0008
#define FLAGS_HASH      0x0010
#define FLAGS_UPPERCASE 0x0020
#define FLAGS_CHAR      0x0040
#define FLAGS_SHORT     0x0080
#define FLAGS_LONG      0x0100
#define FLAGS_LONG_LONG 0x0200
#define FLAGS_PRECISION 0x0400
#define FLAGS_ADAPT_EXP 0x0800

static void _ntoa_format(SString & rBuf, char * pSrcBuf, size_t srcBufSize, size_t len, bool negative, uint base, uint prec, uint width, uint flags)
{
	// pad leading zeros
	if(!(flags & FLAGS_LEFT)) {
		if(width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS|FLAGS_SPACE))))
			width--;
		while((len < prec) && (len < srcBufSize))
			pSrcBuf[len++] = '0';
		while((flags & FLAGS_ZEROPAD) && (len < width) && (len < srcBufSize))
			pSrcBuf[len++] = '0';
	}
	// handle hash
	if(flags & FLAGS_HASH) {
		if(!(flags & FLAGS_PRECISION) && len && oneof2(len, prec, width)) {
			len--;
			if(len && base == 16U)
				len--;
		}
		if(len < srcBufSize) {
			if(base == 16U)
				pSrcBuf[len++] = (flags & FLAGS_UPPERCASE) ? 'X' : 'x';
			else if(base == 2U)
				pSrcBuf[len++] = 'b';
			if(len < srcBufSize)
				pSrcBuf[len++] = '0';
		}
	}
	if(len < srcBufSize) {
		if(negative)
			pSrcBuf[len++] = '-';
		else if(flags & FLAGS_PLUS)
			pSrcBuf[len++] = '+'; // ignore the space if the '+' exists
		else if(flags & FLAGS_SPACE)
			pSrcBuf[len++] = ' ';
	}
	//_out_rev(rBuf, pSrcBuf, len, width, flags);
	// output the specified string in reverse, taking care of any zero-padding
	//static void _out_rev(SString & rBuf, const char * pSrcBuf, size_t len, uint width, uint flags)
	{
		const size_t start_idx = rBuf.Len();
		// pad spaces up to given width
		if(!(flags & (FLAGS_LEFT|FLAGS_ZEROPAD)) && (width > len))
			rBuf.CatCharN(' ', width-len);
		// reverse string
		while(len) {
			rBuf.CatChar(pSrcBuf[--len]);
		}
		// append pad spaces up to given width
		if(flags & FLAGS_LEFT) {
			const size_t cur_len = rBuf.Len() - start_idx;
			if(width > cur_len)
				rBuf.CatCharN(' ', width - cur_len);
		}
	}
}

// internal itoa for 'long' type
static void _ntoa_long(SString & rBuf, ulong value, bool negative, uint base, uint prec, uint width, uint flags)
{
	char   buf[128];
	size_t len = 0U;
	// no hash for 0 values
	if(!value)
		flags &= ~FLAGS_HASH;
	// write if precision != 0 and value is != 0
	if(!(flags & FLAGS_PRECISION) || value) {
		if(base == 10) {
			do {
				buf[len++] = ('0' + static_cast<char>(value % 10));
				value /= 10;
			} while(value);
		}
		else {
			const char the_first_letter = (flags & FLAGS_UPPERCASE) ? 'A' : 'a';
			do {
				const char digit = static_cast<char>(value % base);
				buf[len++] = (digit < 10) ? ('0' + digit) : (the_first_letter + digit - 10);
				value /= base;
			} while(value);
		}
	}
	_ntoa_format(rBuf, buf, sizeof(buf), len, negative, base, prec, width, flags);
}

// internal itoa for 'long long' type
static void _ntoa_long_long(SString & rBuf, uint64 value, bool negative, uint base, uint prec, uint width, uint flags)
{
	char   buf[128];
	size_t len = 0U;
	// no hash for 0 values
	if(!value)
		flags &= ~FLAGS_HASH;
	// write if precision != 0 and value is != 0
	if(!(flags & FLAGS_PRECISION) || value) {
		if(base == 10) {
			do {
				buf[len++] = ('0' + static_cast<char>(value % 10));
				value /= 10;
			} while(value);
		}
		else {
			const char the_first_letter = (flags & FLAGS_UPPERCASE) ? 'A' : 'a';
			do {
				const char digit = static_cast<char>(value % base);
				buf[len++] = (digit < 10) ? ('0' + digit) : (the_first_letter + digit - 10);
				value /= base;
			} while(value);
		}
	}
	_ntoa_format(rBuf, buf, sizeof(buf), len, negative, base, prec, width, flags);
}

static int FASTCALL sl_printf_implementation(SString & rBuf, const char * pFormat, va_list va)
{
	const   uint org_len = rBuf.Len();
	if(pFormat) {
		char    fout_buf[256];
		const   size_t format_len = strlen(pFormat);
		const   char * p_format_end = pFormat + format_len + 1;
		while(*pFormat) {
			// format specifier?  %[flags][width][.precision][length]
			if(*pFormat != '%') {
				//rBuf.CatChar(*pFormat);
				//pFormat++;
				const char * p_next_pct = static_cast<const char *>(memchr(pFormat, '%', p_format_end-pFormat));
				if(p_next_pct) {
					const size_t next_pct_dist = p_next_pct-pFormat;
					rBuf.Cat_Unsafe(reinterpret_cast<const uint8 *>(pFormat), next_pct_dist);
					pFormat += next_pct_dist;
				}
				else {
					rBuf.Cat(pFormat);
					pFormat = p_format_end;
					break;
				}
			}
			else {
				pFormat++; // yes, evaluate it
				// evaluate flags
				uint width = 0;
				uint precision = 0;
				uint flags = 0U;
				{
					uint n;
					do {
						switch(*pFormat) {
							case '0': flags |= FLAGS_ZEROPAD; pFormat++; n = 1U; break;
							case '-': flags |= FLAGS_LEFT;    pFormat++; n = 1U; break;
							case '+': flags |= FLAGS_PLUS;    pFormat++; n = 1U; break;
							case ' ': flags |= FLAGS_SPACE;   pFormat++; n = 1U; break;
							case '#': flags |= FLAGS_HASH;    pFormat++; n = 1U; break;
							default:                                   n = 0U; break;
						}
					} while(n);
				}
				// evaluate width field
				if(isdec(*pFormat)) {
					do {
						width = width * 10U + (uint)(*(pFormat++) - '0');
					} while(isdec(*pFormat));
				}
				else if(*pFormat == '*') {
					const int w = va_arg(va, int);
					if(w < 0) {
						flags |= FLAGS_LEFT; // reverse padding
						width = static_cast<uint>(-w);
					}
					else
						width = static_cast<uint>(w);
					pFormat++;
				}
				// evaluate precision field
				if(*pFormat == '.') {
					flags |= FLAGS_PRECISION;
					pFormat++;
					if(isdec(*pFormat)) {
						do {
							precision = precision * 10U + (uint)(*(pFormat++) - '0');
						} while(isdec(*pFormat));
					}
					else if(*pFormat == '*') {
						const int prec = static_cast<int>(va_arg(va, int));
						precision = (prec > 0) ? static_cast<uint>(prec) : 0U;
						pFormat++;
					}
				}
				// evaluate length field
				switch(*pFormat) {
					case 'l':
						flags |= FLAGS_LONG;
						pFormat++;
						if(*pFormat == 'l') {
							flags |= FLAGS_LONG_LONG;
							pFormat++;
						}
						break;
					case 'h':
						flags |= FLAGS_SHORT;
						pFormat++;
						if(*pFormat == 'h') {
							flags |= FLAGS_CHAR;
							pFormat++;
						}
						break;
					case 't':
						flags |= (sizeof(ptrdiff_t) == sizeof(uint32) ? FLAGS_LONG : FLAGS_LONG_LONG);
						pFormat++;
						break;
					case 'j':
						flags |= (sizeof(intmax_t) == sizeof(uint32) ? FLAGS_LONG : FLAGS_LONG_LONG);
						pFormat++;
						break;
					case 'z':
						flags |= (sizeof(size_t) == sizeof(uint32) ? FLAGS_LONG : FLAGS_LONG_LONG);
						pFormat++;
						break;
					default:
						break;
				}
				// evaluate specifier
				switch(*pFormat) {
					case 'd':
					case 'i':
					case 'u':
					case 'x':
					case 'X':
					case 'o':
					case 'b': 
						{
							// set the base
							uint base;
							if(oneof2(*pFormat, 'x', 'X'))
								base = 16U;
							else if(*pFormat == 'o')
								base =  8U;
							else if(*pFormat == 'b')
								base =  2U;
							else {
								base = 10U;
								flags &= ~FLAGS_HASH; // no hash for dec format
							}
							if(*pFormat == 'X') // uppercase
								flags |= FLAGS_UPPERCASE;
							if(!oneof2(*pFormat, 'i', 'd')) // no plus or space flag for u, x, X, o, b
								flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
							if(flags & FLAGS_PRECISION) // ignore '0' flag when precision is given
								flags &= ~FLAGS_ZEROPAD;
							if(oneof2(*pFormat, 'i', 'd')) { // convert the integer
								// signed
								if(flags & FLAGS_LONG_LONG) {
									const int64 value = va_arg(va, int64);
									_ntoa_long_long(rBuf, (uint64)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
								}
								else if(flags & FLAGS_LONG) {
									const long value = va_arg(va, long);
									_ntoa_long(rBuf, (ulong)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
								}
								else {
									const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short)va_arg(va, int) : va_arg(va, int);
									_ntoa_long(rBuf, (uint)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
								}
							}
							else { // unsigned
								if(flags & FLAGS_LONG_LONG)
									_ntoa_long_long(rBuf, va_arg(va, uint64), false, base, precision, width, flags);
								else if(flags & FLAGS_LONG)
									_ntoa_long(rBuf, va_arg(va, ulong), false, base, precision, width, flags);
								else {
									const uint value = (flags & FLAGS_CHAR) ? (uchar)va_arg(va, uint) : (flags & FLAGS_SHORT) ? (ushort)va_arg(va, uint) : va_arg(va, uint);
									_ntoa_long(rBuf, value, false, base, precision, width, flags);
								}
							}
							pFormat++;
						}
						break;
					case 'f':
					case 'F':
						{
							const double value = va_arg(va, double);
							if(*pFormat == 'F') 
								flags |= FLAGS_UPPERCASE;
							{
								char  * p_fout_buf = fout_buf;
								int     fout_buf_size = sizeof(fout_buf);
								uint    dconvstr_flags = 0;
								if(flags & FLAGS_UPPERCASE)
									dconvstr_flags |= SIEEE754::fUppercase;
								if(flags & FLAGS_PLUS)
									dconvstr_flags |= SIEEE754::fPrintPlus;
								if(flags & FLAGS_LEFT)
									dconvstr_flags |= SIEEE754::fLeftJustify;
								if(flags & FLAGS_ZEROPAD)
									dconvstr_flags |= SIEEE754::fPadWithZero;
								if(width != 0)
									dconvstr_flags |= SIEEE754::fHaveWidth;
								if(!(flags & FLAGS_PRECISION))
									precision = PRINTF_DEFAULT_FLOAT_PRECISION;
								SIEEE754::Print(&p_fout_buf, &fout_buf_size, value, 'f', dconvstr_flags, width, precision);
								rBuf.Cat_Unsafe(reinterpret_cast<const uint8 *>(fout_buf), (sizeof(fout_buf) - fout_buf_size));
							}
							pFormat++;
						}
						break;
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						{
							const double value = va_arg(va, double);
							char dconv_format_char = 0;
							if(oneof2(*pFormat, 'g', 'G')) {
								flags |= FLAGS_ADAPT_EXP;
								dconv_format_char = 'g';
							}
							else 
								dconv_format_char = 'e';
							if(oneof2(*pFormat, 'E', 'G'))
								flags |= FLAGS_UPPERCASE;
							{
								char  * p_fout_buf = fout_buf;
								int     fout_buf_size = sizeof(fout_buf);
								uint    dconvstr_flags = 0;
								if(flags & FLAGS_UPPERCASE)
									dconvstr_flags |= SIEEE754::fUppercase;
								if(flags & FLAGS_PLUS)
									dconvstr_flags |= SIEEE754::fPrintPlus;
								if(flags & FLAGS_LEFT)
									dconvstr_flags |= SIEEE754::fLeftJustify;
								if(flags & FLAGS_ZEROPAD)
									dconvstr_flags |= SIEEE754::fPadWithZero;
								if(width != 0)
									dconvstr_flags |= SIEEE754::fHaveWidth;
								if(!(flags & FLAGS_PRECISION))
									precision = PRINTF_DEFAULT_FLOAT_PRECISION;
								SIEEE754::Print(&p_fout_buf, &fout_buf_size, value, dconv_format_char, dconvstr_flags, width, precision);
								rBuf.Cat_Unsafe(reinterpret_cast<const uint8 *>(fout_buf), (sizeof(fout_buf) - fout_buf_size));
							}
							pFormat++;
						}
						break;
					case 'c': 
						{
							uint l = 1U;
							// pre padding
							if(!(flags & FLAGS_LEFT)) {
								while(l++ < width)
									rBuf.Space();
							}
							rBuf.CatChar((char)va_arg(va, int));
							// post padding
							if(flags & FLAGS_LEFT) {
								while(l++ < width)
									rBuf.Space();
							}
							pFormat++;
						}
						break;
					case 's': 
						{
							char zero[16];
							const char * p = va_arg(va, char *);
							uint l;
							if(p) {
								if(precision) {
									uint __maxsize = precision;
									const char * p__s = p;
									while(*p__s && __maxsize--)
										++p__s;
									l = static_cast<uint>(p__s - p);
								}
								else
									l = strlen(p);
							}
							else {
								PTR32(zero)[0] = 0;
								p = zero;
								l = precision;
							}
							// pre padding
							if(flags & FLAGS_PRECISION) {
								l = (l < precision ? l : precision);
							}
							if(!(flags & FLAGS_LEFT)) {
								while(l++ < width)
									rBuf.Space();
							}
							// string output
							while(*p && (!(flags & FLAGS_PRECISION) || precision--)) {
								rBuf.CatChar(*(p++));
							}
							// post padding
							if(flags & FLAGS_LEFT) {
								while(l++ < width)
									rBuf.Space();
							}
							pFormat++;
						}
						break;
					case 'p': 
						{
							width = sizeof(void *) * 2U;
							flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
							if(sizeof(uintptr_t) == sizeof(int64))
								_ntoa_long_long(rBuf, (uintptr_t)va_arg(va, void *), false, 16U, precision, width, flags);
							else
								_ntoa_long(rBuf, (ulong)((uintptr_t)va_arg(va, void *)), false, 16U, precision, width, flags);
							pFormat++;
						}
						break;
					case '%':
						rBuf.CatChar('%');
						pFormat++;
						break;
					default:
						rBuf.CatChar(*pFormat);
						pFormat++;
						break;
				}
			}
		}
	}
	assert(rBuf.Len() >= org_len);
	return static_cast<int>(rBuf.Len() - org_len); // return written chars without terminating \0
}

int CDECL slprintf(SString & rBuf, const char * pFormat, ...)
{
	va_list va;
	va_start(va, pFormat);
	const int result = sl_printf_implementation(rBuf, pFormat, va);
	va_end(va);
	return result;
}

int CDECL slprintf_cat(SString & rBuf, const char * pFormat, ...)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	va_list va;
	va_start(va, pFormat);
	const int result = sl_printf_implementation(r_temp_buf, pFormat, va);
	va_end(va);
	rBuf.Cat(r_temp_buf);
	return result;
}

int CDECL slvsprintf_s(char * pBuf, size_t bufSize, const char * pFormat, va_list va)
{
	int    result = 0;
	if(!pBuf || !pFormat) {
		errno = EINVAL;
		result = -1;
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		const int impl_ret = sl_printf_implementation(r_temp_buf, pFormat, va);
		const size_t tb_len = r_temp_buf.Len();
		if(tb_len == 0)
			pBuf[0] = 0;
		else if(tb_len > (bufSize-1)) {
			memcpy(pBuf, r_temp_buf.cptr(), bufSize-1);
			pBuf[bufSize-1] = 0;
			result = static_cast<int>(bufSize-1);
		}
		else {
			memcpy(pBuf, r_temp_buf.cptr(), tb_len);
			pBuf[tb_len] = 0;
			result = static_cast<int>(tb_len);
		}
	}
	return result;
}

int CDECL slsprintf_s(char * pBuf, size_t bufSize, const char * pFormat, ...)
{
	va_list va;
	va_start(va, pFormat);
	const int result = slvsprintf_s(pBuf, bufSize, pFormat, va);
	va_end(va);
	return result;
}

int CDECL slfprintf(FILE * pStream, const char * pFormat, ...)
{
	int    result = 0;
	if(!pStream || !pFormat) {
		errno = EINVAL;
		result = -1;
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		va_list va;
		va_start(va, pFormat);
		const int impl_ret = sl_printf_implementation(r_temp_buf, pFormat, va);
		va_end(va);
		const size_t tb_len = r_temp_buf.Len();
		if(tb_len) {
			const int fpr = fputs(r_temp_buf.cptr(), pStream);
			result = (fpr >= 0) ? static_cast<int>(tb_len) : -1;
		}
	}
	return result;
}

int CDECL slfprintf_stderr(const char * pFormat, ...)
{
	int    result = 0;
	if(!stderr || !pFormat) {
		errno = EINVAL;
		result = -1;
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		va_list va;
		va_start(va, pFormat);
		const int impl_ret = sl_printf_implementation(r_temp_buf, pFormat, va);
		va_end(va);
		const size_t tb_len = r_temp_buf.Len();
		if(tb_len) {
			const int fpr = fputs(r_temp_buf.cptr(), stderr);
			result = (fpr >= 0) ? static_cast<int>(tb_len) : -1;
		}
	}
	return result;
}

int slvprintf(SString & rBuf, const char * pFormat, va_list va)
{
	return sl_printf_implementation(rBuf, pFormat, va);
}

#if SLTEST_RUNNING // {

SLTEST_R(slprintf)
{
	// dummy putchar
	char flat_buffer[100];
	SString sbuf;

	//benchmark=slprintf;sprintf
	int    bm = -1;
	if(pBenchmark == 0) bm = 0;
	else if(sstreqi_ascii(pBenchmark, "slprintf"))  bm = 1;
	else if(sstreqi_ascii(pBenchmark, "sprintf"))   bm = 2;
	else SetInfo("invalid benchmark");
	if(bm == 0) {
		{ // Trivial tests
			slprintf(sbuf.Z(), "\n");
			SLCHECK_EQ(sbuf, "\n");
			slprintf(sbuf.Z(), "");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%%");
			SLCHECK_EQ(sbuf, "%");
			slprintf(sbuf.Z(), "%%%s", "beef");
			SLCHECK_EQ(sbuf, "%beef");
			slprintf(sbuf.Z(), "zero ptr string=\"%s\"", 0);
			SLCHECK_EQ(sbuf, "zero ptr string=\"\"");
		}
		{ // TEST_CASE("printf", "[]" )
			SLCHECK_EQ(slprintf(sbuf.Z(), "% d", 4232), 5);
			SLCHECK_EQ(sbuf.Len(), 5U);
			SLCHECK_EQ(sbuf.cptr()[5], 0);
			SLCHECK_EQ(sbuf, " 4232");
		}
		{ // TEST_CASE("fctprintf", "[]" )
			SLCHECK_EQ(slprintf(sbuf.Z(), "This is a test of %X", 0x12EFU), 22);
			SLCHECK_EQ(sbuf.Len(), 22U);
			SLCHECK_EQ(sbuf, "This is a test of 12EF");
		}
		{ // TEST_CASE("snprintf", "[]" )
			slprintf(sbuf.Z(), "%d", -1000);
			SLCHECK_EQ(sbuf, "-1000");
			//slprintf(buffer, 3U, "%d", -1000);
			//SLCHECK_EQ(sbuf, "-1"));
		}
		{ // TEST_CASE("space flag", "[]" )
			slprintf(sbuf.Z(), "% d", 42);
			SLCHECK_EQ(sbuf, " 42");
			slprintf(sbuf.Z(), "% d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "% 5d", 42);
			SLCHECK_EQ(sbuf, "   42");
			slprintf(sbuf.Z(), "% 5d", -42);
			SLCHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "% 15d", 42);
			SLCHECK_EQ(sbuf, "             42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15.3f", -42.987);
			SLCHECK_EQ(sbuf, "        -42.987");
			slprintf(sbuf.Z(), "% 15.3f", 42.987);
			SLCHECK_EQ(sbuf, "         42.987");
			slprintf(sbuf.Z(), "% s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "% d", 1024);
			SLCHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% i", 1024);
			SLCHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "% u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "% o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "% o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "% x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "% x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "% X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "% X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "% c", 'x');
			SLCHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("+ flag", "[]" )
			slprintf(sbuf.Z(), "%+d", 42);
			SLCHECK_EQ(sbuf, "+42");
			slprintf(sbuf.Z(), "%+d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%+5d", 42);
			SLCHECK_EQ(sbuf, "  +42");
			slprintf(sbuf.Z(), "%+5d", -42);
			SLCHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "%+15d", 42);
			SLCHECK_EQ(sbuf, "            +42");
			slprintf(sbuf.Z(), "%+15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "%+s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%+d", 1024);
			SLCHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+i", 1024);
			SLCHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%+u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%+o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%+o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%+x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%+x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%+X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%+X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%+c", 'x');
			SLCHECK_EQ(sbuf, "x");
			slprintf(sbuf.Z(), "%+.0d", 0);
			SLCHECK_EQ(sbuf, "+");
		}
		{ // TEST_CASE("0 flag", "[]" )
			slprintf(sbuf.Z(), "%0d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0ld", 42L);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%05d", 42);
			SLCHECK_EQ(sbuf, "00042");
			slprintf(sbuf.Z(), "%05d", -42);
			SLCHECK_EQ(sbuf, "-0042");
			slprintf(sbuf.Z(), "%015d", 42);
			SLCHECK_EQ(sbuf, "000000000000042");
			slprintf(sbuf.Z(), "%015d", -42);
			SLCHECK_EQ(sbuf, "-00000000000042");
			slprintf(sbuf.Z(), "%015.2f", 42.1234);
			SLCHECK_EQ(sbuf, "000000000042.12");
			slprintf(sbuf.Z(), "%015.3f", 42.9876);
			SLCHECK_EQ(sbuf, "00000000042.988");
			slprintf(sbuf.Z(), "%015.5f", -42.9876);
			SLCHECK_EQ(sbuf, "-00000042.98760");
		}
		{ // TEST_CASE("- flag", "[]" )
			slprintf(sbuf.Z(), "%-d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-5d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-5d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-15d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-15d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%-0d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-0d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-05d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-05d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-015d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-015d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0-d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%0-5d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%0-5d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%0-15d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%0-15d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-15.3e", -42.);
			SLCHECK_EQ(sbuf, "-4.200e+01     ");
			sprintf(flat_buffer, "%0-15.3g", -42.); // @proof
			slprintf(sbuf.Z(), "%0-15.3g", -42.);
			SLCHECK_EQ(sbuf, "-42            ");
		}
		{ // TEST_CASE("# flag", "[]" )
			slprintf(sbuf.Z(), "%#.0x", 0);
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.1x", 0);
			SLCHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%#.0llx", (int64)0);
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.8x", 0x614e);
			SLCHECK_EQ(sbuf, "0x0000614e");
			slprintf(sbuf.Z(), "%#b", 6);
			SLCHECK_EQ(sbuf, "0b110");
		}
		{ // TEST_CASE("specifier", "[]" )
			slprintf(sbuf.Z(), "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%d", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%i", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%%");
			SLCHECK_EQ(sbuf, "%");
		}
		{ // TEST_CASE("width", "[]" )
			slprintf(sbuf.Z(), "%1s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%1d", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1i", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%1o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%1o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%1x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%1x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%1X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%1X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%1c", 'x');
			SLCHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("width 20", "[]" )
			slprintf(sbuf.Z(), "%20s", "Hello");
			SLCHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%20d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20o", 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20x", 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20X", 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20c", 'x');
			SLCHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width *20", "[]" )
			slprintf(sbuf.Z(), "%*s", 20, "Hello");
			SLCHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%*d", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*d", 20, -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*i", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*i", 20, -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*u", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*u", 20, 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%*o", 20, 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%*o", 20, 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%*x", 20, 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%*x", 20, 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%*X", 20, 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%*X", 20, 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%*c", 20,'x');
			SLCHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width -20", "[]" )
			slprintf(sbuf.Z(), "%-20s", "Hello");
			SLCHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%-20d", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20d", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20i", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20i", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20u", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20.4f", 1024.1234);
			SLCHECK_EQ(sbuf, "1024.1234           ");
			slprintf(sbuf.Z(), "%-20u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%-20o", 511);
			SLCHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%-20o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%-20x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%-20x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%-20X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%-20X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%-20c", 'x');
			SLCHECK_EQ(sbuf, "x                   ");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 9, 9, 9);
			SLCHECK_EQ(sbuf, "|    9| |9 | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 10, 10, 10);
			SLCHECK_EQ(sbuf, "|   10| |10| |   10|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 9, 9, 9);
			SLCHECK_EQ(sbuf, "|    9| |9           | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 10, 10, 10);
			SLCHECK_EQ(sbuf, "|   10| |10          | |   10|");
		}
		{ // TEST_CASE("width 0-20", "[]" )
			slprintf(sbuf.Z(), "%0-20s", "Hello");
			SLCHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%0-20d", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20d", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20i", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20i", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20u", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%0-20o", 511);
			SLCHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%0-20o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%0-20x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%0-20x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%0-20X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%0-20X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%0-20c", 'x');
			SLCHECK_EQ(sbuf, "x                   ");
		}
		{ // TEST_CASE("padding 20", "[]" ) 
			slprintf(sbuf.Z(), "%020d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020d", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020i", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%020o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%020o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%020x", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%020x", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%020X", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%020X", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding .20", "[]" )
			slprintf(sbuf.Z(), "%.20d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20d", -1024);
			SLCHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", -1024);
			SLCHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%.20o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%.20o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%.20x", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%.20x", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%.20X", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%.20X", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding #020", "[]" )
			slprintf(sbuf.Z(), "%#020d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020d", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020i", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%#020o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%#020o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%#020x", 305441741);
			SLCHECK_EQ(sbuf, "0x00000000001234abcd");
			slprintf(sbuf.Z(), "%#020x", 3989525555U);
			SLCHECK_EQ(sbuf, "0x0000000000edcb5433");
			slprintf(sbuf.Z(), "%#020X", 305441741);
			SLCHECK_EQ(sbuf, "0X00000000001234ABCD");
			slprintf(sbuf.Z(), "%#020X", 3989525555U);
			SLCHECK_EQ(sbuf, "0X0000000000EDCB5433");
		}
		{ // TEST_CASE("padding #20", "[]" )
			slprintf(sbuf.Z(), "%#20d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%#20o", 511);
			SLCHECK_EQ(sbuf, "                0777");
			slprintf(sbuf.Z(), "%#20o", 4294966785U);
			SLCHECK_EQ(sbuf, "        037777777001");
			slprintf(sbuf.Z(), "%#20x", 305441741);
			SLCHECK_EQ(sbuf, "          0x1234abcd");
			slprintf(sbuf.Z(), "%#20x", 3989525555U);
			SLCHECK_EQ(sbuf, "          0xedcb5433");
			slprintf(sbuf.Z(), "%#20X", 305441741);
			SLCHECK_EQ(sbuf, "          0X1234ABCD");
			slprintf(sbuf.Z(), "%#20X", 3989525555U);
			SLCHECK_EQ(sbuf, "          0XEDCB5433");
		}
		{ // TEST_CASE("padding 20.5", "[]" )
			slprintf(sbuf.Z(), "%20.5d", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5d", -1024);
			SLCHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5i", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5i", -1024);
			SLCHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5u", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.5o", 511);
			SLCHECK_EQ(sbuf, "               00777");
			slprintf(sbuf.Z(), "%20.5o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.5x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20.10x", 3989525555U);
			SLCHECK_EQ(sbuf, "          00edcb5433");
			slprintf(sbuf.Z(), "%20.5X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.10X", 3989525555U);
			SLCHECK_EQ(sbuf, "          00EDCB5433");
		}
		{ // TEST_CASE("padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 1d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 2d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 3d", -5);
			SLCHECK_EQ(sbuf, " -5");
			slprintf(sbuf.Z(), "% 4d", -5);
			SLCHECK_EQ(sbuf, "  -5");
			// zero padding
			slprintf(sbuf.Z(), "%01d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03d", -5);
			SLCHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%04d", -5);
			SLCHECK_EQ(sbuf, "-005");
		}
		{ // TEST_CASE("float padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 3.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 4.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 5.1f", -5.);
			SLCHECK_EQ(sbuf, " -5.0");
			slprintf(sbuf.Z(), "% 6.1g", -5.);
			SLCHECK_EQ(sbuf, "    -5");
			slprintf(sbuf.Z(), "% 6.1e", -5.);
			SLCHECK_EQ(sbuf, "-5.0e+00");
			slprintf(sbuf.Z(), "% 10.1e", -5.);
			SLCHECK_EQ(sbuf, "  -5.0e+00");
			// zero padding
			slprintf(sbuf.Z(), "%03.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%04.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%05.1f", -5.);
			SLCHECK_EQ(sbuf, "-05.0");
			// zero padding no decimal point
			slprintf(sbuf.Z(), "%01.0f", -5.);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02.0f", -5.);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03.0f", -5.);
			SLCHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%010.1e", -5.);
			SLCHECK_EQ(sbuf, "-005.0e+00");
			slprintf(sbuf.Z(), "%07.0E", -5.);
			SLCHECK_EQ(sbuf, "-05E+00");
			slprintf(sbuf.Z(), "%03.0g", -5.);
			SLCHECK_EQ(sbuf, "-05");
		}
		{ // TEST_CASE("length", "[]" )
			slprintf(sbuf.Z(), "%.0s", "Hello testing");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.0s", "Hello testing");
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%.s", "Hello testing");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.s", "Hello testing");
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.d", 0);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.i", 0);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.u", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.o", 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20.0o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.o", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%50.x", 305441741);
			SLCHECK_EQ(sbuf, "                                          1234abcd");
			slprintf(sbuf.Z(), "%50.x%10.u", 305441741, 12345);
			SLCHECK_EQ(sbuf, "                                          1234abcd     12345");
			slprintf(sbuf.Z(), "%20.0x", 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20.x", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.0X", 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20.X", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%02.0u", 0U);
			SLCHECK_EQ(sbuf, "  ");
			slprintf(sbuf.Z(), "%02.0d", 0);
			SLCHECK_EQ(sbuf, "  ");
		}
		{ // TEST_CASE("float", "[]" )
			// test special-case floats using math.h macros
			slprintf(sbuf.Z(), "%8f", fgetnan());
			SLCHECK_EQ(sbuf, "     nan");
			slprintf(sbuf.Z(), "%8f", fgetposinf());
			SLCHECK_EQ(sbuf, "     inf");
			slprintf(sbuf.Z(), "%-8f", fgetneginf());
			SLCHECK_EQ(sbuf, "-inf    ");
			slprintf(sbuf.Z(), "%+8e", fgetposinf());
			SLCHECK_EQ(sbuf, "    +inf");
			slprintf(sbuf.Z(), "%.4f", 3.1415354);
			SLCHECK_EQ(sbuf, "3.1415");
			slprintf(sbuf.Z(), "%.3f", 30343.1415354);
			SLCHECK_EQ(sbuf, "30343.142");
			slprintf(sbuf.Z(), "%.0f", 34.1415354);
			SLCHECK_EQ(sbuf, "34");
			slprintf(sbuf.Z(), "%.0f", 1.3);
			SLCHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.0f", 1.55);
			SLCHECK_EQ(sbuf, "2");
			slprintf(sbuf.Z(), "%.1f", 1.64);
			SLCHECK_EQ(sbuf, "1.6");
			slprintf(sbuf.Z(), "%.2f", 42.8952);
			SLCHECK_EQ(sbuf, "42.90");
			slprintf(sbuf.Z(), "%.9f", 42.8952);
			SLCHECK_EQ(sbuf, "42.895200000");
			slprintf(sbuf.Z(), "%.10f", 42.895223);
			SLCHECK_EQ(sbuf, "42.8952230000");
			// this testcase checks, that the precision is truncated to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522312345678);
			SLCHECK_EQ(sbuf, "42.895223123457"); // @sobolev "42.895223123000"-->"42.895223123457"
			// this testcase checks, that the precision is truncated AND rounded to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522387654321);
			SLCHECK_EQ(sbuf, "42.895223876543"); // @sobolev "42.895223877000"-->"42.895223876543"
			slprintf(sbuf.Z(), "%6.2f", 42.8952);
			SLCHECK_EQ(sbuf, " 42.90");
			slprintf(sbuf.Z(), "%+6.2f", 42.8952);
			SLCHECK_EQ(sbuf, "+42.90");
			slprintf(sbuf.Z(), "%+5.1f", 42.9252);
			SLCHECK_EQ(sbuf, "+42.9");
			sprintf(flat_buffer, "%f", 42.5); // @proof
			slprintf(sbuf.Z(), "%f", 42.5);
			SLCHECK_EQ(sbuf, "42.500000");
			slprintf(sbuf.Z(), "%.1f", 42.5);
			SLCHECK_EQ(sbuf, "42.5");
			slprintf(sbuf.Z(), "%f", 42167.0);
			SLCHECK_EQ(sbuf, "42167.000000");
			slprintf(sbuf.Z(), "%.9f", -12345.987654321);
			SLCHECK_EQ(sbuf, "-12345.987654321");
			slprintf(sbuf.Z(), "%.1f", 3.999);
			SLCHECK_EQ(sbuf, "4.0");
			slprintf(sbuf.Z(), "%.0f", 3.5);
			SLCHECK_EQ(sbuf, "4");
			sprintf(flat_buffer, "%.0f", 4.5); // @proof
			slprintf(sbuf.Z(), "%.0f", 4.5);
			SLCHECK_EQ(sbuf, "5"); // @sobolev "4"-->"5"
			slprintf(sbuf.Z(), "%.0f", 3.49);
			SLCHECK_EQ(sbuf, "3");
			slprintf(sbuf.Z(), "%.1f", 3.49);
			SLCHECK_EQ(sbuf, "3.5");
			slprintf(sbuf.Z(), "a%-5.1f", 0.5);
			SLCHECK_EQ(sbuf, "a0.5  ");
			slprintf(sbuf.Z(), "a%-5.1fend", 0.5);
			SLCHECK_EQ(sbuf, "a0.5  end");
			slprintf(sbuf.Z(), "%G", 12345.678);
			SLCHECK_EQ(sbuf, "12345.7");
			slprintf(sbuf.Z(), "%.7G", 12345.678);
			SLCHECK_EQ(sbuf, "12345.68");
			slprintf(sbuf.Z(), "%.5G", 123456789.);
			SLCHECK_EQ(sbuf, "1.2346E+08");
			sprintf(flat_buffer, "%.6G", 12345.); // @proof
			slprintf(sbuf.Z(), "%.6G", 12345.);
			SLCHECK_EQ(sbuf, flat_buffer);
			SLCHECK_EQ(sbuf, "12345"); // @sobolev "12345.0"-->"12345"
			slprintf(sbuf.Z(), "%+12.4g", 123456789.);
			SLCHECK_EQ(sbuf, "  +1.235e+08");
			slprintf(sbuf.Z(), "%.2G", 0.001234);
			SLCHECK_EQ(sbuf, "0.0012");
			slprintf(sbuf.Z(), "%+10.4G", 0.001234);
			SLCHECK_EQ(sbuf, " +0.001234");
			slprintf(sbuf.Z(), "%+012.4g", 0.00001234);
			SLCHECK_EQ(sbuf, "+001.234e-05");
			slprintf(sbuf.Z(), "%.3g", -1.2345e-308);
			SLCHECK_EQ(sbuf, "-1.23e-308");
			slprintf(sbuf.Z(), "%+.3E", 1.23e+308);
			SLCHECK_EQ(sbuf, "+1.230E+308");
			// out of range for float: should switch to exp notation if supported, else empty
			sprintf(flat_buffer, "%.1f", 1E20); // @proof
			slprintf(sbuf.Z(), "%.1f", 1E20);
			SLCHECK_EQ(sbuf, flat_buffer);
			SLCHECK_EQ(sbuf, "100000000000000000000.0"); // @sobolev "1.0e+20"-->"100000000000000000000.0"

			// brute force float
			for(float i = -100000.0f; i < 100000.0f; i += 1.0f) {
				slprintf(sbuf.Z(), "%.5f", i / 10000.0f);
				sprintf(flat_buffer, "%.5f", i / 10000.0f);
				SLCHECK_EQ(sbuf, flat_buffer);
			}
			//
			// brute force exp
			// @sobolev Пришлось модифицировать следующий тест,ограничив точность с 1e20 до 1e15 из-за того, что
			// разные реализации (даже в рамках одного компилятора разных версий) могут по-разному формировать
			// последние значащие цифры величин большой точности.
			// Для вящей убедительности я увеличил количество итераций инкрементируя величину случайным значением,
			// порядка 1e8 (1e-1 * 1e9).
			//
			/*for(float i = -1.0e20f; i < 1e20f; i += 1e15f) {
				slprintf(sbuf.Z(), "%.5f", i);
				sprintf(flat_buffer, "%.5f", i);
				SLCHECK_EQ(sbuf, flat_buffer);
			}*/
			for(float i = -1.0e15f; i < 1e15f;) {
				slprintf(sbuf.Z(), "%.5f", i);
				sprintf(flat_buffer, "%.5f", i);
				SLCHECK_EQ(sbuf, flat_buffer);
				float rn = static_cast<float>(SLS.GetTLA().Rg.GetReal()) * 1.0e9f;
				i += rn;
			}
		}
		{ // TEST_CASE("types", "[]" )
			slprintf(sbuf.Z(), "%i", 0);
			SLCHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%i", 1234);
			SLCHECK_EQ(sbuf, "1234");
			slprintf(sbuf.Z(), "%i", 32767);
			SLCHECK_EQ(sbuf, "32767");
			slprintf(sbuf.Z(), "%i", -32767);
			SLCHECK_EQ(sbuf, "-32767");
			slprintf(sbuf.Z(), "%li", 30L);
			SLCHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%li", -2147483647L);
			SLCHECK_EQ(sbuf, "-2147483647");
			slprintf(sbuf.Z(), "%li", 2147483647L);
			SLCHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%lli", 30LL);
			SLCHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%lli", -9223372036854775807LL);
			SLCHECK_EQ(sbuf, "-9223372036854775807");
			slprintf(sbuf.Z(), "%lli", 9223372036854775807LL);
			SLCHECK_EQ(sbuf, "9223372036854775807");
			slprintf(sbuf.Z(), "%lu", 100000L);
			SLCHECK_EQ(sbuf, "100000");
			slprintf(sbuf.Z(), "%lu", 0xFFFFFFFFL);
			SLCHECK_EQ(sbuf, "4294967295");
			slprintf(sbuf.Z(), "%llu", 281474976710656ULL);
			SLCHECK_EQ(sbuf, "281474976710656");
			slprintf(sbuf.Z(), "%llu", 18446744073709551615ULL);
			SLCHECK_EQ(sbuf, "18446744073709551615");
			slprintf(sbuf.Z(), "%zu", 2147483647UL);
			SLCHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%zd", 2147483647UL);
			SLCHECK_EQ(sbuf, "2147483647");
			if(sizeof(size_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%zi", -2147483647L);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%zi", -2147483647LL);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			slprintf(sbuf.Z(), "%b", 60000);
			SLCHECK_EQ(sbuf, "1110101001100000");
			slprintf(sbuf.Z(), "%lb", 12345678L);
			SLCHECK_EQ(sbuf, "101111000110000101001110");
			slprintf(sbuf.Z(), "%o", 60000);
			SLCHECK_EQ(sbuf, "165140");
			slprintf(sbuf.Z(), "%lo", 12345678L);
			SLCHECK_EQ(sbuf, "57060516");
			slprintf(sbuf.Z(), "%lx", 0x12345678L);
			SLCHECK_EQ(sbuf, "12345678");
			slprintf(sbuf.Z(), "%llx", 0x1234567891234567ULL);
			SLCHECK_EQ(sbuf, "1234567891234567");
			slprintf(sbuf.Z(), "%lx", 0xabcdefabL);
			SLCHECK_EQ(sbuf, "abcdefab");
			slprintf(sbuf.Z(), "%lX", 0xabcdefabL);
			SLCHECK_EQ(sbuf, "ABCDEFAB");
			slprintf(sbuf.Z(), "%c", 'v');
			SLCHECK_EQ(sbuf, "v");
			slprintf(sbuf.Z(), "%cv", 'w');
			SLCHECK_EQ(sbuf, "wv");
			slprintf(sbuf.Z(), "%s", "A Test");
			SLCHECK_EQ(sbuf, "A Test");
			slprintf(sbuf.Z(), "%hhu", 0xFFFFUL);
			SLCHECK_EQ(sbuf, "255");
			slprintf(sbuf.Z(), "%hu", 0x123456UL);
			SLCHECK_EQ(sbuf, "13398");
			slprintf(sbuf.Z(), "%s%hhi %hu", "Test", 10000, 0xFFFFFFFF);
			SLCHECK_EQ(sbuf, "Test16 65535");
			slprintf(sbuf.Z(), "%tx", &flat_buffer[10] - &flat_buffer[0]);
			SLCHECK_EQ(sbuf, "a");
			// TBD
			if(sizeof(intmax_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%ji", -2147483647L);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%ji", -2147483647LL);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
		}
		{ // TEST_CASE("pointer", "[]" )
			slprintf(sbuf.Z(), "%p", (void *)0x1234U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "00001234");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000000001234");
			}
			slprintf(sbuf.Z(), "%p", (void *)0x12345678U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "12345678");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000012345678");
			}
			slprintf(sbuf.Z(), "%p-%p", (void *)0x12345678U, (void *)0x7EDCBA98U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "12345678-7EDCBA98");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000012345678-000000007EDCBA98");
			}
			if(sizeof(uintptr_t) == sizeof(uint64_t)) {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLCHECK_EQ(sbuf, "00000000FFFFFFFF");
			}
			else {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLCHECK_EQ(sbuf, "FFFFFFFF");
			}
		}
		{ // TEST_CASE("unknown flag", "[]" )
			slprintf(sbuf.Z(), "%kmarco", 42, 37);
			SLCHECK_EQ(sbuf, "kmarco");
		}
		{ // TEST_CASE("string length", "[]" )
			slprintf(sbuf.Z(), "%.4s", "This is a test");
			SLCHECK_EQ(sbuf, "This");
			slprintf(sbuf.Z(), "%.4s", "test");
			SLCHECK_EQ(sbuf, "test");
			slprintf(sbuf.Z(), "%.7s", "123");
			SLCHECK_EQ(sbuf, "123");
			slprintf(sbuf.Z(), "%.7s", "");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%.4s%.2s", "123456", "abcdef");
			SLCHECK_EQ(sbuf, "1234ab");
			slprintf(sbuf.Z(), "%.4.2s", "123456");
			SLCHECK_EQ(sbuf, ".2s");
			slprintf(sbuf.Z(), "%.*s", 3, "123456");
			SLCHECK_EQ(sbuf, "123");
		}
		{ // TEST_CASE("misc", "[]" )
			slprintf(sbuf.Z(), "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit");
			SLCHECK_EQ(sbuf, "53000atest-20 bit");
			slprintf(sbuf.Z(), "%.*f", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*d", -1, 1);
			SLCHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.3s", "foobar");
			SLCHECK_EQ(sbuf, "foo");
			slprintf(sbuf.Z(), "% .0d", 0);
			SLCHECK_EQ(sbuf, " ");
			slprintf(sbuf.Z(), "%10.5d", 4);
			SLCHECK_EQ(sbuf, "     00004");
			slprintf(sbuf.Z(), "%*sx", -3, "hi");
			SLCHECK_EQ(sbuf, "hi x");
			slprintf(sbuf.Z(), "%.*g", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*e", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "3.33e-01");
		}
#if 0 // @construction {
		{
			//
			// Попытка проверить slprintf на тестах curl (там эти тесты верифицируют собственную имплементацию ..printf)
			//
			#if (SIZEOF_CURL_OFF_T > SIZEOF_LONG)
				#define MPRNT_SUFFIX_CURL_OFF_T  LL
			#else
				#define MPRNT_SUFFIX_CURL_OFF_T  L
			#endif
			#define MPRNT_OFF_T_C_HELPER2(Val, Suffix) Val ## Suffix 
			#define MPRNT_OFF_T_C_HELPER1(Val, Suffix) MPRNT_OFF_T_C_HELPER2(Val, Suffix)
			#define MPRNT_OFF_T_C(Val)  MPRNT_OFF_T_C_HELPER1(Val, MPRNT_SUFFIX_CURL_OFF_T)
			#define BUFSZ    256
			#define USHORT_TESTS_ARRSZ 1 + 100
			#define SSHORT_TESTS_ARRSZ 1 + 100
			#define UINT_TESTS_ARRSZ   1 + 100
			#define SINT_TESTS_ARRSZ   1 + 100
			#define ULONG_TESTS_ARRSZ  1 + 100
			#define SLONG_TESTS_ARRSZ  1 + 100
			#define COFFT_TESTS_ARRSZ  1 + 100

			struct unsshort_st {
				unsigned short num; /* unsigned short  */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct sigshort_st {
				short num;      /* signed short    */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct unsint_st {
				unsigned int num; /* unsigned int    */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct sigint_st {
				int num;        /* signed int      */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct unslong_st {
				unsigned long num; /* unsigned long   */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct siglong_st {
				long num;       /* signed long     */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};
			struct curloff_st {
				off_t num; /* curl_off_t      */
				const char * expected; /* expected string */
				char result[BUFSZ]; /* result string   */
			};

			static struct unsshort_st us_test[USHORT_TESTS_ARRSZ];
			static struct sigshort_st ss_test[SSHORT_TESTS_ARRSZ];
			static struct unsint_st ui_test[UINT_TESTS_ARRSZ];
			static struct sigint_st si_test[SINT_TESTS_ARRSZ];
			static struct unslong_st ul_test[ULONG_TESTS_ARRSZ];
			static struct siglong_st sl_test[SLONG_TESTS_ARRSZ];
			static struct curloff_st co_test[COFFT_TESTS_ARRSZ];

			class InnerBlock {
			public:
				static int test_unsigned_short_formatting(void)
				{
					int i, j;
					int num_ushort_tests = 0;
					int failed = 0;
				//#if (SIZEOF_SHORT == 1)
					if(sizeof(short) == 1) {
						i = 1; us_test[i].num = 0xFFU; us_test[i].expected = "256";
						i++; us_test[i].num = 0xF0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x0FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xE0U; us_test[i].expected = "224";
						i++; us_test[i].num = 0x0EU; us_test[i].expected = "14";
						i++; us_test[i].num = 0xC0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x0CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x01U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x00U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 2)
					else if(sizeof(short) == 2) {
						i = 1; us_test[i].num = 0xFFFFU; us_test[i].expected = "65535";
						i++; us_test[i].num = 0xFF00U; us_test[i].expected = "65280";
						i++; us_test[i].num = 0x00FFU; us_test[i].expected = "255";
						i++; us_test[i].num = 0xF000U; us_test[i].expected = "61440";
						i++; us_test[i].num = 0x0F00U; us_test[i].expected = "3840";
						i++; us_test[i].num = 0x00F0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x000FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xC000U; us_test[i].expected = "49152";
						i++; us_test[i].num = 0x0C00U; us_test[i].expected = "3072";
						i++; us_test[i].num = 0x00C0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x000CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x0001U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x0000U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 4)
					else if(sizeof(short) == 4) {
						i = 1; us_test[i].num = 0xFFFFFFFFU; us_test[i].expected = "4294967295";
						i++; us_test[i].num = 0xFFFF0000U; us_test[i].expected = "4294901760";
						i++; us_test[i].num = 0x0000FFFFU; us_test[i].expected = "65535";
						i++; us_test[i].num = 0xFF000000U; us_test[i].expected = "4278190080";
						i++; us_test[i].num = 0x00FF0000U; us_test[i].expected = "16711680";
						i++; us_test[i].num = 0x0000FF00U; us_test[i].expected = "65280";
						i++; us_test[i].num = 0x000000FFU; us_test[i].expected = "255";
						i++; us_test[i].num = 0xF0000000U; us_test[i].expected = "4026531840";
						i++; us_test[i].num = 0x0F000000U; us_test[i].expected = "251658240";
						i++; us_test[i].num = 0x00F00000U; us_test[i].expected = "15728640";
						i++; us_test[i].num = 0x000F0000U; us_test[i].expected = "983040";
						i++; us_test[i].num = 0x0000F000U; us_test[i].expected = "61440";
						i++; us_test[i].num = 0x00000F00U; us_test[i].expected = "3840";
						i++; us_test[i].num = 0x000000F0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x0000000FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xC0000000U; us_test[i].expected = "3221225472";
						i++; us_test[i].num = 0x0C000000U; us_test[i].expected = "201326592";
						i++; us_test[i].num = 0x00C00000U; us_test[i].expected = "12582912";
						i++; us_test[i].num = 0x000C0000U; us_test[i].expected = "786432";
						i++; us_test[i].num = 0x0000C000U; us_test[i].expected = "49152";
						i++; us_test[i].num = 0x00000C00U; us_test[i].expected = "3072";
						i++; us_test[i].num = 0x000000C0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x0000000CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x00000001U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x00000000U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#endif
					for(i = 1; i <= num_ushort_tests; i++) {
						for(j = 0; j < BUFSZ; j++)
							us_test[i].result[j] = 'X';
						us_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(us_test[i].result, sizeof(us_test[i].result), "%hu", us_test[i].num);
						if(memcmp(us_test[i].result, us_test[i].expected, strlen(us_test[i].expected))) {
							printf("unsigned short test #%.2d: Failed (Expected: %s Got: %s)\n", i, us_test[i].expected, us_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned short tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned short tests Failed!\n");
					return failed;
				}
				static int test_signed_short_formatting(void)
				{
					int i, j;
					int num_sshort_tests = 0;
					int failed = 0;
				//#if (SIZEOF_SHORT == 1)
					if(sizeof(short) == 1) {
						i = 1; ss_test[i].num = 0x7F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x70; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x07; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x50; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x05; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x01; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x00; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x70 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x07 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x50 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x05 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x00 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 2)
					else if(sizeof(short) == 2) {
						i = 1; ss_test[i].num = 0x7FFF; ss_test[i].expected = "32767";
						i++; ss_test[i].num = 0x7FFE; ss_test[i].expected = "32766";
						i++; ss_test[i].num = 0x7FFD; ss_test[i].expected = "32765";
						i++; ss_test[i].num = 0x7F00; ss_test[i].expected = "32512";
						i++; ss_test[i].num = 0x07F0; ss_test[i].expected = "2032";
						i++; ss_test[i].num = 0x007F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x7000; ss_test[i].expected = "28672";
						i++; ss_test[i].num = 0x0700; ss_test[i].expected = "1792";
						i++; ss_test[i].num = 0x0070; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x0007; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x5000; ss_test[i].expected = "20480";
						i++; ss_test[i].num = 0x0500; ss_test[i].expected = "1280";
						i++; ss_test[i].num = 0x0050; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x0005; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x0001; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x0000; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7FFF -1; ss_test[i].expected = "-32768";
						i++; ss_test[i].num = -0x7FFE -1; ss_test[i].expected = "-32767";
						i++; ss_test[i].num = -0x7FFD -1; ss_test[i].expected = "-32766";
						i++; ss_test[i].num = -0x7F00 -1; ss_test[i].expected = "-32513";
						i++; ss_test[i].num = -0x07F0 -1; ss_test[i].expected = "-2033";
						i++; ss_test[i].num = -0x007F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x7000 -1; ss_test[i].expected = "-28673";
						i++; ss_test[i].num = -0x0700 -1; ss_test[i].expected = "-1793";
						i++; ss_test[i].num = -0x0070 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x0007 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x5000 -1; ss_test[i].expected = "-20481";
						i++; ss_test[i].num = -0x0500 -1; ss_test[i].expected = "-1281";
						i++; ss_test[i].num = -0x0050 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x0005 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x0000 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 4)
					else if(sizeof(short) == 4) {
						i = 1; ss_test[i].num = 0x7FFFFFFF; ss_test[i].expected = "2147483647";
						i++; ss_test[i].num = 0x7FFFFFFE; ss_test[i].expected = "2147483646";
						i++; ss_test[i].num = 0x7FFFFFFD; ss_test[i].expected = "2147483645";
						i++; ss_test[i].num = 0x7FFF0000; ss_test[i].expected = "2147418112";
						i++; ss_test[i].num = 0x00007FFF; ss_test[i].expected = "32767";
						i++; ss_test[i].num = 0x7F000000; ss_test[i].expected = "2130706432";
						i++; ss_test[i].num = 0x007F0000; ss_test[i].expected = "8323072";
						i++; ss_test[i].num = 0x00007F00; ss_test[i].expected = "32512";
						i++; ss_test[i].num = 0x0000007F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x70000000; ss_test[i].expected = "1879048192";
						i++; ss_test[i].num = 0x07000000; ss_test[i].expected = "117440512";
						i++; ss_test[i].num = 0x00700000; ss_test[i].expected = "7340032";
						i++; ss_test[i].num = 0x00070000; ss_test[i].expected = "458752";
						i++; ss_test[i].num = 0x00007000; ss_test[i].expected = "28672";
						i++; ss_test[i].num = 0x00000700; ss_test[i].expected = "1792";
						i++; ss_test[i].num = 0x00000070; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x00000007; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x50000000; ss_test[i].expected = "1342177280";
						i++; ss_test[i].num = 0x05000000; ss_test[i].expected = "83886080";
						i++; ss_test[i].num = 0x00500000; ss_test[i].expected = "5242880";
						i++; ss_test[i].num = 0x00050000; ss_test[i].expected = "327680";
						i++; ss_test[i].num = 0x00005000; ss_test[i].expected = "20480";
						i++; ss_test[i].num = 0x00000500; ss_test[i].expected = "1280";
						i++; ss_test[i].num = 0x00000050; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x00000005; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x00000001; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x00000000; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7FFFFFFF -1; ss_test[i].expected = "-2147483648";
						i++; ss_test[i].num = -0x7FFFFFFE -1; ss_test[i].expected = "-2147483647";
						i++; ss_test[i].num = -0x7FFFFFFD -1; ss_test[i].expected = "-2147483646";
						i++; ss_test[i].num = -0x7FFF0000 -1; ss_test[i].expected = "-2147418113";
						i++; ss_test[i].num = -0x00007FFF -1; ss_test[i].expected = "-32768";
						i++; ss_test[i].num = -0x7F000000 -1; ss_test[i].expected = "-2130706433";
						i++; ss_test[i].num = -0x007F0000 -1; ss_test[i].expected = "-8323073";
						i++; ss_test[i].num = -0x00007F00 -1; ss_test[i].expected = "-32513";
						i++; ss_test[i].num = -0x0000007F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x70000000 -1; ss_test[i].expected = "-1879048193";
						i++; ss_test[i].num = -0x07000000 -1; ss_test[i].expected = "-117440513";
						i++; ss_test[i].num = -0x00700000 -1; ss_test[i].expected = "-7340033";
						i++; ss_test[i].num = -0x00070000 -1; ss_test[i].expected = "-458753";
						i++; ss_test[i].num = -0x00007000 -1; ss_test[i].expected = "-28673";
						i++; ss_test[i].num = -0x00000700 -1; ss_test[i].expected = "-1793";
						i++; ss_test[i].num = -0x00000070 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x00000007 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x50000000 -1; ss_test[i].expected = "-1342177281";
						i++; ss_test[i].num = -0x05000000 -1; ss_test[i].expected = "-83886081";
						i++; ss_test[i].num = -0x00500000 -1; ss_test[i].expected = "-5242881";
						i++; ss_test[i].num = -0x00050000 -1; ss_test[i].expected = "-327681";
						i++; ss_test[i].num = -0x00005000 -1; ss_test[i].expected = "-20481";
						i++; ss_test[i].num = -0x00000500 -1; ss_test[i].expected = "-1281";
						i++; ss_test[i].num = -0x00000050 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x00000005 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x00000000 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#endif
					for(i = 1; i <= num_sshort_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ss_test[i].result[j] = 'X';
						ss_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ss_test[i].result, sizeof(ss_test[i].result), "%hd", ss_test[i].num);
						if(memcmp(ss_test[i].result,
							ss_test[i].expected,
							strlen(ss_test[i].expected))) {
							printf("signed short test #%.2d: Failed (Expected: %s Got: %s)\n", i, ss_test[i].expected, ss_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed short tests OK!\n");
					else
						printf("Some curl_mprintf() signed short tests Failed!\n");
					return failed;
				}

				static int test_unsigned_int_formatting(void)
				{
					int i, j;
					int num_uint_tests = 0;
					int failed = 0;
				//#if (SIZEOF_INT == 2)
					if(sizeof(int) == 2) {
						i = 1; ui_test[i].num = 0xFFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x00FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x0F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x00F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x0C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x00C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x0001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x0000U; ui_test[i].expected = "0";
						num_uint_tests = i;
					}
				//#elif (SIZEOF_INT == 4)
					else if(sizeof(int) == 4) {
						i = 1; ui_test[i].num = 0xFFFFFFFFU; ui_test[i].expected = "4294967295";
						i++; ui_test[i].num = 0xFFFF0000U; ui_test[i].expected = "4294901760";
						i++; ui_test[i].num = 0x0000FFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF000000U; ui_test[i].expected = "4278190080";
						i++; ui_test[i].num = 0x00FF0000U; ui_test[i].expected = "16711680";
						i++; ui_test[i].num = 0x0000FF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x000000FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF0000000U; ui_test[i].expected = "4026531840";
						i++; ui_test[i].num = 0x0F000000U; ui_test[i].expected = "251658240";
						i++; ui_test[i].num = 0x00F00000U; ui_test[i].expected = "15728640";
						i++; ui_test[i].num = 0x000F0000U; ui_test[i].expected = "983040";
						i++; ui_test[i].num = 0x0000F000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x00000F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x000000F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x0000000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC0000000U; ui_test[i].expected = "3221225472";
						i++; ui_test[i].num = 0x0C000000U; ui_test[i].expected = "201326592";
						i++; ui_test[i].num = 0x00C00000U; ui_test[i].expected = "12582912";
						i++; ui_test[i].num = 0x000C0000U; ui_test[i].expected = "786432";
						i++; ui_test[i].num = 0x0000C000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x00000C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x000000C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x0000000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x00000001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x00000000U; ui_test[i].expected = "0";
						num_uint_tests = i;
					}
				//#elif (SIZEOF_INT == 8)
					else if(sizeof(int) == 8) {
						/* !checksrc! disable LONGLINE all */
						i = 1; ui_test[i].num = 0xFFFFFFFFFFFFFFFFU; ui_test[i].expected = "18446744073709551615";
						i++; ui_test[i].num = 0xFFFFFFFF00000000U; ui_test[i].expected = "18446744069414584320";
						i++; ui_test[i].num = 0x00000000FFFFFFFFU; ui_test[i].expected = "4294967295";
						i++; ui_test[i].num = 0xFFFF000000000000U; ui_test[i].expected = "18446462598732840960";
						i++; ui_test[i].num = 0x0000FFFF00000000U; ui_test[i].expected = "281470681743360";
						i++; ui_test[i].num = 0x00000000FFFF0000U; ui_test[i].expected = "4294901760";
						i++; ui_test[i].num = 0x000000000000FFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF00000000000000U; ui_test[i].expected = "18374686479671623680";
						i++; ui_test[i].num = 0x00FF000000000000U; ui_test[i].expected = "71776119061217280";
						i++; ui_test[i].num = 0x0000FF0000000000U; ui_test[i].expected = "280375465082880";
						i++; ui_test[i].num = 0x000000FF00000000U; ui_test[i].expected = "1095216660480";
						i++; ui_test[i].num = 0x00000000FF000000U; ui_test[i].expected = "4278190080";
						i++; ui_test[i].num = 0x0000000000FF0000U; ui_test[i].expected = "16711680";
						i++; ui_test[i].num = 0x000000000000FF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x00000000000000FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF000000000000000U; ui_test[i].expected = "17293822569102704640";
						i++; ui_test[i].num = 0x0F00000000000000U; ui_test[i].expected = "1080863910568919040";
						i++; ui_test[i].num = 0x00F0000000000000U; ui_test[i].expected = "67553994410557440";
						i++; ui_test[i].num = 0x000F000000000000U; ui_test[i].expected = "4222124650659840";
						i++; ui_test[i].num = 0x0000F00000000000U; ui_test[i].expected = "263882790666240";
						i++; ui_test[i].num = 0x00000F0000000000U; ui_test[i].expected = "16492674416640";
						i++; ui_test[i].num = 0x000000F000000000U; ui_test[i].expected = "1030792151040";
						i++; ui_test[i].num = 0x0000000F00000000U; ui_test[i].expected = "64424509440";
						i++; ui_test[i].num = 0x00000000F0000000U; ui_test[i].expected = "4026531840";
						i++; ui_test[i].num = 0x000000000F000000U; ui_test[i].expected = "251658240";
						i++; ui_test[i].num = 0x0000000000F00000U; ui_test[i].expected = "15728640";
						i++; ui_test[i].num = 0x00000000000F0000U; ui_test[i].expected = "983040";
						i++; ui_test[i].num = 0x000000000000F000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x0000000000000F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x00000000000000F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x000000000000000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC000000000000000U; ui_test[i].expected = "13835058055282163712";
						i++; ui_test[i].num = 0x0C00000000000000U; ui_test[i].expected = "864691128455135232";
						i++; ui_test[i].num = 0x00C0000000000000U; ui_test[i].expected = "54043195528445952";
						i++; ui_test[i].num = 0x000C000000000000U; ui_test[i].expected = "3377699720527872";
						i++; ui_test[i].num = 0x0000C00000000000U; ui_test[i].expected = "211106232532992";
						i++; ui_test[i].num = 0x00000C0000000000U; ui_test[i].expected = "13194139533312";
						i++; ui_test[i].num = 0x000000C000000000U; ui_test[i].expected = "824633720832";
						i++; ui_test[i].num = 0x0000000C00000000U; ui_test[i].expected = "51539607552";
						i++; ui_test[i].num = 0x00000000C0000000U; ui_test[i].expected = "3221225472";
						i++; ui_test[i].num = 0x000000000C000000U; ui_test[i].expected = "201326592";
						i++; ui_test[i].num = 0x0000000000C00000U; ui_test[i].expected = "12582912";
						i++; ui_test[i].num = 0x00000000000C0000U; ui_test[i].expected = "786432";
						i++; ui_test[i].num = 0x000000000000C000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x0000000000000C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x00000000000000C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x000000000000000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x00000001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x00000000U; ui_test[i].expected = "0";
						num_uint_tests = i;
				//#endif
					}
					for(i = 1; i <= num_uint_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ui_test[i].result[j] = 'X';
						ui_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ui_test[i].result, sizeof(ui_test[i].result), "%u", ui_test[i].num);
						if(memcmp(ui_test[i].result,
							ui_test[i].expected,
							strlen(ui_test[i].expected))) {
							printf("unsigned int test #%.2d: Failed (Expected: %s Got: %s)\n", i, ui_test[i].expected, ui_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned int tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned int tests Failed!\n");
					return failed;
				}

				static int test_signed_int_formatting(void)
				{
					int i, j;
					int num_sint_tests = 0;
					int failed = 0;
				//#if (SIZEOF_INT == 2)
					if(sizeof(int) == 2) {
						i = 1; si_test[i].num = 0x7FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7FFE; si_test[i].expected = "32766";
						i++; si_test[i].num = 0x7FFD; si_test[i].expected = "32765";
						i++; si_test[i].num = 0x7F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x07F0; si_test[i].expected = "2032";
						i++; si_test[i].num = 0x007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x7000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x0700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x0070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x0007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x5000; si_test[i].expected = "20480";
						i++; si_test[i].num = 0x0500; si_test[i].expected = "1280";
						i++; si_test[i].num = 0x0050; si_test[i].expected = "80";
						i++; si_test[i].num = 0x0005; si_test[i].expected = "5";
						i++; si_test[i].num = 0x0001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x0000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7FFE -1; si_test[i].expected = "-32767";
						i++; si_test[i].num = -0x7FFD -1; si_test[i].expected = "-32766";
						i++; si_test[i].num = -0x7F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x07F0 -1; si_test[i].expected = "-2033";
						i++; si_test[i].num = -0x007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x7000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x0700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x0070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x0007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num = -0x5000 -1; si_test[i].expected = "-20481";
						i++; si_test[i].num = -0x0500 -1; si_test[i].expected = "-1281";
						i++; si_test[i].num = -0x0050 -1; si_test[i].expected = "-81";
						i++; si_test[i].num = -0x0005 -1; si_test[i].expected = "-6";
						i++; si_test[i].num =  0x0000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
					}
				//#elif (SIZEOF_INT == 4)
					else if(sizeof(int) == 4) {
						i = 1; si_test[i].num = 0x7FFFFFFF; si_test[i].expected = "2147483647";
						i++; si_test[i].num = 0x7FFFFFFE; si_test[i].expected = "2147483646";
						i++; si_test[i].num = 0x7FFFFFFD; si_test[i].expected = "2147483645";
						i++; si_test[i].num = 0x7FFF0000; si_test[i].expected = "2147418112";
						i++; si_test[i].num = 0x00007FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7F000000; si_test[i].expected = "2130706432";
						i++; si_test[i].num = 0x007F0000; si_test[i].expected = "8323072";
						i++; si_test[i].num = 0x00007F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x0000007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x70000000; si_test[i].expected = "1879048192";
						i++; si_test[i].num = 0x07000000; si_test[i].expected = "117440512";
						i++; si_test[i].num = 0x00700000; si_test[i].expected = "7340032";
						i++; si_test[i].num = 0x00070000; si_test[i].expected = "458752";
						i++; si_test[i].num = 0x00007000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x00000700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x00000070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x00000007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x50000000; si_test[i].expected = "1342177280";
						i++; si_test[i].num = 0x05000000; si_test[i].expected = "83886080";
						i++; si_test[i].num = 0x00500000; si_test[i].expected = "5242880";
						i++; si_test[i].num = 0x00050000; si_test[i].expected = "327680";
						i++; si_test[i].num = 0x00005000; si_test[i].expected = "20480";
						i++; si_test[i].num = 0x00000500; si_test[i].expected = "1280";
						i++; si_test[i].num = 0x00000050; si_test[i].expected = "80";
						i++; si_test[i].num = 0x00000005; si_test[i].expected = "5";
						i++; si_test[i].num = 0x00000001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x00000000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFFFFFF -1; si_test[i].expected = "-2147483648";
						i++; si_test[i].num = -0x7FFFFFFE -1; si_test[i].expected = "-2147483647";
						i++; si_test[i].num = -0x7FFFFFFD -1; si_test[i].expected = "-2147483646";
						i++; si_test[i].num = -0x7FFF0000 -1; si_test[i].expected = "-2147418113";
						i++; si_test[i].num = -0x00007FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7F000000 -1; si_test[i].expected = "-2130706433";
						i++; si_test[i].num = -0x007F0000 -1; si_test[i].expected = "-8323073";
						i++; si_test[i].num = -0x00007F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x0000007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x70000000 -1; si_test[i].expected = "-1879048193";
						i++; si_test[i].num = -0x07000000 -1; si_test[i].expected = "-117440513";
						i++; si_test[i].num = -0x00700000 -1; si_test[i].expected = "-7340033";
						i++; si_test[i].num = -0x00070000 -1; si_test[i].expected = "-458753";
						i++; si_test[i].num = -0x00007000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x00000700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x00000070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x00000007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num = -0x50000000 -1; si_test[i].expected = "-1342177281";
						i++; si_test[i].num = -0x05000000 -1; si_test[i].expected = "-83886081";
						i++; si_test[i].num = -0x00500000 -1; si_test[i].expected = "-5242881";
						i++; si_test[i].num = -0x00050000 -1; si_test[i].expected = "-327681";
						i++; si_test[i].num = -0x00005000 -1; si_test[i].expected = "-20481";
						i++; si_test[i].num = -0x00000500 -1; si_test[i].expected = "-1281";
						i++; si_test[i].num = -0x00000050 -1; si_test[i].expected = "-81";
						i++; si_test[i].num = -0x00000005 -1; si_test[i].expected = "-6";
						i++; si_test[i].num =  0x00000000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
					}
				//#elif (SIZEOF_INT == 8)
					else if(sizeof(int) == 8) {
						i = 1; si_test[i].num = 0x7FFFFFFFFFFFFFFF; si_test[i].expected = "9223372036854775807";
						i++; si_test[i].num = 0x7FFFFFFFFFFFFFFE; si_test[i].expected = "9223372036854775806";
						i++; si_test[i].num = 0x7FFFFFFFFFFFFFFD; si_test[i].expected = "9223372036854775805";
						i++; si_test[i].num = 0x7FFFFFFF00000000; si_test[i].expected = "9223372032559808512";
						i++; si_test[i].num = 0x000000007FFFFFFF; si_test[i].expected = "2147483647";
						i++; si_test[i].num = 0x7FFF000000000000; si_test[i].expected = "9223090561878065152";
						i++; si_test[i].num = 0x00007FFF00000000; si_test[i].expected = "140733193388032";
						i++; si_test[i].num = 0x000000007FFF0000; si_test[i].expected = "2147418112";
						i++; si_test[i].num = 0x0000000000007FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7F00000000000000; si_test[i].expected = "9151314442816847872";
						i++; si_test[i].num = 0x007F000000000000; si_test[i].expected = "35747322042253312";
						i++; si_test[i].num = 0x00007F0000000000; si_test[i].expected = "139637976727552";
						i++; si_test[i].num = 0x0000007F00000000; si_test[i].expected = "545460846592";
						i++; si_test[i].num = 0x000000007F000000; si_test[i].expected = "2130706432";
						i++; si_test[i].num = 0x00000000007F0000; si_test[i].expected = "8323072";
						i++; si_test[i].num = 0x0000000000007F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x000000000000007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x7000000000000000; si_test[i].expected = "8070450532247928832";
						i++; si_test[i].num = 0x0700000000000000; si_test[i].expected = "504403158265495552";
						i++; si_test[i].num = 0x0070000000000000; si_test[i].expected = "31525197391593472";
						i++; si_test[i].num = 0x0007000000000000; si_test[i].expected = "1970324836974592";
						i++; si_test[i].num = 0x0000700000000000; si_test[i].expected = "123145302310912";
						i++; si_test[i].num = 0x0000070000000000; si_test[i].expected = "7696581394432";
						i++; si_test[i].num = 0x0000007000000000; si_test[i].expected = "481036337152";
						i++; si_test[i].num = 0x0000000700000000; si_test[i].expected = "30064771072";
						i++; si_test[i].num = 0x0000000070000000; si_test[i].expected = "1879048192";
						i++; si_test[i].num = 0x0000000007000000; si_test[i].expected = "117440512";
						i++; si_test[i].num = 0x0000000000700000; si_test[i].expected = "7340032";
						i++; si_test[i].num = 0x0000000000070000; si_test[i].expected = "458752";
						i++; si_test[i].num = 0x0000000000007000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x0000000000000700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x0000000000000070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x0000000000000007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x0000000000000001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x0000000000000000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFF -1; si_test[i].expected = "-9223372036854775808";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFE -1; si_test[i].expected = "-9223372036854775807";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFD -1; si_test[i].expected = "-9223372036854775806";
						i++; si_test[i].num = -0x7FFFFFFF00000000 -1; si_test[i].expected = "-9223372032559808513";
						i++; si_test[i].num = -0x000000007FFFFFFF -1; si_test[i].expected = "-2147483648";
						i++; si_test[i].num = -0x7FFF000000000000 -1; si_test[i].expected = "-9223090561878065153";
						i++; si_test[i].num = -0x00007FFF00000000 -1; si_test[i].expected = "-140733193388033";
						i++; si_test[i].num = -0x000000007FFF0000 -1; si_test[i].expected = "-2147418113";
						i++; si_test[i].num = -0x0000000000007FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7F00000000000000 -1; si_test[i].expected = "-9151314442816847873";
						i++; si_test[i].num = -0x007F000000000000 -1; si_test[i].expected = "-35747322042253313";
						i++; si_test[i].num = -0x00007F0000000000 -1; si_test[i].expected = "-139637976727553";
						i++; si_test[i].num = -0x0000007F00000000 -1; si_test[i].expected = "-545460846593";
						i++; si_test[i].num = -0x000000007F000000 -1; si_test[i].expected = "-2130706433";
						i++; si_test[i].num = -0x00000000007F0000 -1; si_test[i].expected = "-8323073";
						i++; si_test[i].num = -0x0000000000007F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x000000000000007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x7000000000000000 -1; si_test[i].expected = "-8070450532247928833";
						i++; si_test[i].num = -0x0700000000000000 -1; si_test[i].expected = "-504403158265495553";
						i++; si_test[i].num = -0x0070000000000000 -1; si_test[i].expected = "-31525197391593473";
						i++; si_test[i].num = -0x0007000000000000 -1; si_test[i].expected = "-1970324836974593";
						i++; si_test[i].num = -0x0000700000000000 -1; si_test[i].expected = "-123145302310913";
						i++; si_test[i].num = -0x0000070000000000 -1; si_test[i].expected = "-7696581394433";
						i++; si_test[i].num = -0x0000007000000000 -1; si_test[i].expected = "-481036337153";
						i++; si_test[i].num = -0x0000000700000000 -1; si_test[i].expected = "-30064771073";
						i++; si_test[i].num = -0x0000000070000000 -1; si_test[i].expected = "-1879048193";
						i++; si_test[i].num = -0x0000000007000000 -1; si_test[i].expected = "-117440513";
						i++; si_test[i].num = -0x0000000000700000 -1; si_test[i].expected = "-7340033";
						i++; si_test[i].num = -0x0000000000070000 -1; si_test[i].expected = "-458753";
						i++; si_test[i].num = -0x0000000000007000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x0000000000000700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x0000000000000070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x0000000000000007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num =  0x0000000000000000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
				//#endif
					}
					for(i = 1; i <= num_sint_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							si_test[i].result[j] = 'X';
						si_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(si_test[i].result, sizeof(si_test[i].result), "%d", si_test[i].num);
						if(memcmp(si_test[i].result, si_test[i].expected, strlen(si_test[i].expected))) {
							printf("signed int test #%.2d: Failed (Expected: %s Got: %s)\n", i, si_test[i].expected, si_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed int tests OK!\n");
					else
						printf("Some curl_mprintf() signed int tests Failed!\n");
					return failed;
				}

				static int test_unsigned_long_formatting(void)
				{
					int i, j;
					int num_ulong_tests = 0;
					int failed = 0;
				//#if (SIZEOF_LONG == 2)
					if(sizeof(long) == 2) {
						i = 1; ul_test[i].num = 0xFFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x00FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x0F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x00F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x0C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x00C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x0001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x0000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#elif (SIZEOF_LONG == 4)
					else if(sizeof(long) == 4) {
						i = 1; ul_test[i].num = 0xFFFFFFFFUL; ul_test[i].expected = "4294967295";
						i++; ul_test[i].num = 0xFFFF0000UL; ul_test[i].expected = "4294901760";
						i++; ul_test[i].num = 0x0000FFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF000000UL; ul_test[i].expected = "4278190080";
						i++; ul_test[i].num = 0x00FF0000UL; ul_test[i].expected = "16711680";
						i++; ul_test[i].num = 0x0000FF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x000000FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF0000000UL; ul_test[i].expected = "4026531840";
						i++; ul_test[i].num = 0x0F000000UL; ul_test[i].expected = "251658240";
						i++; ul_test[i].num = 0x00F00000UL; ul_test[i].expected = "15728640";
						i++; ul_test[i].num = 0x000F0000UL; ul_test[i].expected = "983040";
						i++; ul_test[i].num = 0x0000F000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x00000F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x000000F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x0000000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC0000000UL; ul_test[i].expected = "3221225472";
						i++; ul_test[i].num = 0x0C000000UL; ul_test[i].expected = "201326592";
						i++; ul_test[i].num = 0x00C00000UL; ul_test[i].expected = "12582912";
						i++; ul_test[i].num = 0x000C0000UL; ul_test[i].expected = "786432";
						i++; ul_test[i].num = 0x0000C000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x00000C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x000000C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x0000000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x00000001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x00000000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#elif (SIZEOF_LONG == 8)
					else if(sizeof(long) == 8) {
						i = 1; ul_test[i].num = 0xFFFFFFFFFFFFFFFFUL; ul_test[i].expected = "18446744073709551615";
						i++; ul_test[i].num = 0xFFFFFFFF00000000UL; ul_test[i].expected = "18446744069414584320";
						i++; ul_test[i].num = 0x00000000FFFFFFFFUL; ul_test[i].expected = "4294967295";
						i++; ul_test[i].num = 0xFFFF000000000000UL; ul_test[i].expected = "18446462598732840960";
						i++; ul_test[i].num = 0x0000FFFF00000000UL; ul_test[i].expected = "281470681743360";
						i++; ul_test[i].num = 0x00000000FFFF0000UL; ul_test[i].expected = "4294901760";
						i++; ul_test[i].num = 0x000000000000FFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF00000000000000UL; ul_test[i].expected = "18374686479671623680";
						i++; ul_test[i].num = 0x00FF000000000000UL; ul_test[i].expected = "71776119061217280";
						i++; ul_test[i].num = 0x0000FF0000000000UL; ul_test[i].expected = "280375465082880";
						i++; ul_test[i].num = 0x000000FF00000000UL; ul_test[i].expected = "1095216660480";
						i++; ul_test[i].num = 0x00000000FF000000UL; ul_test[i].expected = "4278190080";
						i++; ul_test[i].num = 0x0000000000FF0000UL; ul_test[i].expected = "16711680";
						i++; ul_test[i].num = 0x000000000000FF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x00000000000000FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF000000000000000UL; ul_test[i].expected = "17293822569102704640";
						i++; ul_test[i].num = 0x0F00000000000000UL; ul_test[i].expected = "1080863910568919040";
						i++; ul_test[i].num = 0x00F0000000000000UL; ul_test[i].expected = "67553994410557440";
						i++; ul_test[i].num = 0x000F000000000000UL; ul_test[i].expected = "4222124650659840";
						i++; ul_test[i].num = 0x0000F00000000000UL; ul_test[i].expected = "263882790666240";
						i++; ul_test[i].num = 0x00000F0000000000UL; ul_test[i].expected = "16492674416640";
						i++; ul_test[i].num = 0x000000F000000000UL; ul_test[i].expected = "1030792151040";
						i++; ul_test[i].num = 0x0000000F00000000UL; ul_test[i].expected = "64424509440";
						i++; ul_test[i].num = 0x00000000F0000000UL; ul_test[i].expected = "4026531840";
						i++; ul_test[i].num = 0x000000000F000000UL; ul_test[i].expected = "251658240";
						i++; ul_test[i].num = 0x0000000000F00000UL; ul_test[i].expected = "15728640";
						i++; ul_test[i].num = 0x00000000000F0000UL; ul_test[i].expected = "983040";
						i++; ul_test[i].num = 0x000000000000F000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x0000000000000F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x00000000000000F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x000000000000000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC000000000000000UL; ul_test[i].expected = "13835058055282163712";
						i++; ul_test[i].num = 0x0C00000000000000UL; ul_test[i].expected = "864691128455135232";
						i++; ul_test[i].num = 0x00C0000000000000UL; ul_test[i].expected = "54043195528445952";
						i++; ul_test[i].num = 0x000C000000000000UL; ul_test[i].expected = "3377699720527872";
						i++; ul_test[i].num = 0x0000C00000000000UL; ul_test[i].expected = "211106232532992";
						i++; ul_test[i].num = 0x00000C0000000000UL; ul_test[i].expected = "13194139533312";
						i++; ul_test[i].num = 0x000000C000000000UL; ul_test[i].expected = "824633720832";
						i++; ul_test[i].num = 0x0000000C00000000UL; ul_test[i].expected = "51539607552";
						i++; ul_test[i].num = 0x00000000C0000000UL; ul_test[i].expected = "3221225472";
						i++; ul_test[i].num = 0x000000000C000000UL; ul_test[i].expected = "201326592";
						i++; ul_test[i].num = 0x0000000000C00000UL; ul_test[i].expected = "12582912";
						i++; ul_test[i].num = 0x00000000000C0000UL; ul_test[i].expected = "786432";
						i++; ul_test[i].num = 0x000000000000C000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x0000000000000C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x00000000000000C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x000000000000000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x00000001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x00000000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#endif
					for(i = 1; i <= num_ulong_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ul_test[i].result[j] = 'X';
						ul_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ul_test[i].result, sizeof(ul_test[i].result), "%lu", ul_test[i].num);
						if(memcmp(ul_test[i].result, ul_test[i].expected, strlen(ul_test[i].expected))) {
							printf("unsigned long test #%.2d: Failed (Expected: %s Got: %s)\n", i, ul_test[i].expected, ul_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned long tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned long tests Failed!\n");
					return failed;
				}
				static int test_signed_long_formatting(void)
				{
					int i, j;
					int num_slong_tests = 0;
					int failed = 0;
				//#if (SIZEOF_LONG == 2)
					if(sizeof(long) == 2) {
						i = 1; sl_test[i].num = 0x7FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7FFEL; sl_test[i].expected = "32766";
						i++; sl_test[i].num = 0x7FFDL; sl_test[i].expected = "32765";
						i++; sl_test[i].num = 0x7F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x07F0L; sl_test[i].expected = "2032";
						i++; sl_test[i].num = 0x007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x7000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x0700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x0070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x0007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x5000L; sl_test[i].expected = "20480";
						i++; sl_test[i].num = 0x0500L; sl_test[i].expected = "1280";
						i++; sl_test[i].num = 0x0050L; sl_test[i].expected = "80";
						i++; sl_test[i].num = 0x0005L; sl_test[i].expected = "5";
						i++; sl_test[i].num = 0x0001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x0000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7FFEL -1L; sl_test[i].expected = "-32767";
						i++; sl_test[i].num = -0x7FFDL -1L; sl_test[i].expected = "-32766";
						i++; sl_test[i].num = -0x7F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x07F0L -1L; sl_test[i].expected = "-2033";
						i++; sl_test[i].num = -0x007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x7000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x0700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x0070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x0007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num = -0x5000L -1L; sl_test[i].expected = "-20481";
						i++; sl_test[i].num = -0x0500L -1L; sl_test[i].expected = "-1281";
						i++; sl_test[i].num = -0x0050L -1L; sl_test[i].expected = "-81";
						i++; sl_test[i].num = -0x0005L -1L; sl_test[i].expected = "-6";
						i++; sl_test[i].num =  0x0000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#elif (SIZEOF_LONG == 4)
					else if(sizeof(long) == 4) {
						i = 1; sl_test[i].num = 0x7FFFFFFFL; sl_test[i].expected = "2147483647";
						i++; sl_test[i].num = 0x7FFFFFFEL; sl_test[i].expected = "2147483646";
						i++; sl_test[i].num = 0x7FFFFFFDL; sl_test[i].expected = "2147483645";
						i++; sl_test[i].num = 0x7FFF0000L; sl_test[i].expected = "2147418112";
						i++; sl_test[i].num = 0x00007FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7F000000L; sl_test[i].expected = "2130706432";
						i++; sl_test[i].num = 0x007F0000L; sl_test[i].expected = "8323072";
						i++; sl_test[i].num = 0x00007F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x0000007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x70000000L; sl_test[i].expected = "1879048192";
						i++; sl_test[i].num = 0x07000000L; sl_test[i].expected = "117440512";
						i++; sl_test[i].num = 0x00700000L; sl_test[i].expected = "7340032";
						i++; sl_test[i].num = 0x00070000L; sl_test[i].expected = "458752";
						i++; sl_test[i].num = 0x00007000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x00000700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x00000070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x00000007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x50000000L; sl_test[i].expected = "1342177280";
						i++; sl_test[i].num = 0x05000000L; sl_test[i].expected = "83886080";
						i++; sl_test[i].num = 0x00500000L; sl_test[i].expected = "5242880";
						i++; sl_test[i].num = 0x00050000L; sl_test[i].expected = "327680";
						i++; sl_test[i].num = 0x00005000L; sl_test[i].expected = "20480";
						i++; sl_test[i].num = 0x00000500L; sl_test[i].expected = "1280";
						i++; sl_test[i].num = 0x00000050L; sl_test[i].expected = "80";
						i++; sl_test[i].num = 0x00000005L; sl_test[i].expected = "5";
						i++; sl_test[i].num = 0x00000001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x00000000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFFFFFL -1L; sl_test[i].expected = "-2147483648";
						i++; sl_test[i].num = -0x7FFFFFFEL -1L; sl_test[i].expected = "-2147483647";
						i++; sl_test[i].num = -0x7FFFFFFDL -1L; sl_test[i].expected = "-2147483646";
						i++; sl_test[i].num = -0x7FFF0000L -1L; sl_test[i].expected = "-2147418113";
						i++; sl_test[i].num = -0x00007FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7F000000L -1L; sl_test[i].expected = "-2130706433";
						i++; sl_test[i].num = -0x007F0000L -1L; sl_test[i].expected = "-8323073";
						i++; sl_test[i].num = -0x00007F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x0000007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x70000000L -1L; sl_test[i].expected = "-1879048193";
						i++; sl_test[i].num = -0x07000000L -1L; sl_test[i].expected = "-117440513";
						i++; sl_test[i].num = -0x00700000L -1L; sl_test[i].expected = "-7340033";
						i++; sl_test[i].num = -0x00070000L -1L; sl_test[i].expected = "-458753";
						i++; sl_test[i].num = -0x00007000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x00000700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x00000070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x00000007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num = -0x50000000L -1L; sl_test[i].expected = "-1342177281";
						i++; sl_test[i].num = -0x05000000L -1L; sl_test[i].expected = "-83886081";
						i++; sl_test[i].num = -0x00500000L -1L; sl_test[i].expected = "-5242881";
						i++; sl_test[i].num = -0x00050000L -1L; sl_test[i].expected = "-327681";
						i++; sl_test[i].num = -0x00005000L -1L; sl_test[i].expected = "-20481";
						i++; sl_test[i].num = -0x00000500L -1L; sl_test[i].expected = "-1281";
						i++; sl_test[i].num = -0x00000050L -1L; sl_test[i].expected = "-81";
						i++; sl_test[i].num = -0x00000005L -1L; sl_test[i].expected = "-6";
						i++; sl_test[i].num =  0x00000000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#elif (SIZEOF_LONG == 8)
					else if(sizeof(long) == 8) {
						i = 1; sl_test[i].num = 0x7FFFFFFFFFFFFFFFL; sl_test[i].expected = "9223372036854775807";
						i++; sl_test[i].num = 0x7FFFFFFFFFFFFFFEL; sl_test[i].expected = "9223372036854775806";
						i++; sl_test[i].num = 0x7FFFFFFFFFFFFFFDL; sl_test[i].expected = "9223372036854775805";
						i++; sl_test[i].num = 0x7FFFFFFF00000000L; sl_test[i].expected = "9223372032559808512";
						i++; sl_test[i].num = 0x000000007FFFFFFFL; sl_test[i].expected = "2147483647";
						i++; sl_test[i].num = 0x7FFF000000000000L; sl_test[i].expected = "9223090561878065152";
						i++; sl_test[i].num = 0x00007FFF00000000L; sl_test[i].expected = "140733193388032";
						i++; sl_test[i].num = 0x000000007FFF0000L; sl_test[i].expected = "2147418112";
						i++; sl_test[i].num = 0x0000000000007FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7F00000000000000L; sl_test[i].expected = "9151314442816847872";
						i++; sl_test[i].num = 0x007F000000000000L; sl_test[i].expected = "35747322042253312";
						i++; sl_test[i].num = 0x00007F0000000000L; sl_test[i].expected = "139637976727552";
						i++; sl_test[i].num = 0x0000007F00000000L; sl_test[i].expected = "545460846592";
						i++; sl_test[i].num = 0x000000007F000000L; sl_test[i].expected = "2130706432";
						i++; sl_test[i].num = 0x00000000007F0000L; sl_test[i].expected = "8323072";
						i++; sl_test[i].num = 0x0000000000007F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x000000000000007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x7000000000000000L; sl_test[i].expected = "8070450532247928832";
						i++; sl_test[i].num = 0x0700000000000000L; sl_test[i].expected = "504403158265495552";
						i++; sl_test[i].num = 0x0070000000000000L; sl_test[i].expected = "31525197391593472";
						i++; sl_test[i].num = 0x0007000000000000L; sl_test[i].expected = "1970324836974592";
						i++; sl_test[i].num = 0x0000700000000000L; sl_test[i].expected = "123145302310912";
						i++; sl_test[i].num = 0x0000070000000000L; sl_test[i].expected = "7696581394432";
						i++; sl_test[i].num = 0x0000007000000000L; sl_test[i].expected = "481036337152";
						i++; sl_test[i].num = 0x0000000700000000L; sl_test[i].expected = "30064771072";
						i++; sl_test[i].num = 0x0000000070000000L; sl_test[i].expected = "1879048192";
						i++; sl_test[i].num = 0x0000000007000000L; sl_test[i].expected = "117440512";
						i++; sl_test[i].num = 0x0000000000700000L; sl_test[i].expected = "7340032";
						i++; sl_test[i].num = 0x0000000000070000L; sl_test[i].expected = "458752";
						i++; sl_test[i].num = 0x0000000000007000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x0000000000000700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x0000000000000070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x0000000000000007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x0000000000000001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x0000000000000000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFFL -1L; sl_test[i].expected = "-9223372036854775808";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFEL -1L; sl_test[i].expected = "-9223372036854775807";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFDL -1L; sl_test[i].expected = "-9223372036854775806";
						i++; sl_test[i].num = -0x7FFFFFFF00000000L -1L; sl_test[i].expected = "-9223372032559808513";
						i++; sl_test[i].num = -0x000000007FFFFFFFL -1L; sl_test[i].expected = "-2147483648";
						i++; sl_test[i].num = -0x7FFF000000000000L -1L; sl_test[i].expected = "-9223090561878065153";
						i++; sl_test[i].num = -0x00007FFF00000000L -1L; sl_test[i].expected = "-140733193388033";
						i++; sl_test[i].num = -0x000000007FFF0000L -1L; sl_test[i].expected = "-2147418113";
						i++; sl_test[i].num = -0x0000000000007FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7F00000000000000L -1L; sl_test[i].expected = "-9151314442816847873";
						i++; sl_test[i].num = -0x007F000000000000L -1L; sl_test[i].expected = "-35747322042253313";
						i++; sl_test[i].num = -0x00007F0000000000L -1L; sl_test[i].expected = "-139637976727553";
						i++; sl_test[i].num = -0x0000007F00000000L -1L; sl_test[i].expected = "-545460846593";
						i++; sl_test[i].num = -0x000000007F000000L -1L; sl_test[i].expected = "-2130706433";
						i++; sl_test[i].num = -0x00000000007F0000L -1L; sl_test[i].expected = "-8323073";
						i++; sl_test[i].num = -0x0000000000007F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x000000000000007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x7000000000000000L -1L; sl_test[i].expected = "-8070450532247928833";
						i++; sl_test[i].num = -0x0700000000000000L -1L; sl_test[i].expected = "-504403158265495553";
						i++; sl_test[i].num = -0x0070000000000000L -1L; sl_test[i].expected = "-31525197391593473";
						i++; sl_test[i].num = -0x0007000000000000L -1L; sl_test[i].expected = "-1970324836974593";
						i++; sl_test[i].num = -0x0000700000000000L -1L; sl_test[i].expected = "-123145302310913";
						i++; sl_test[i].num = -0x0000070000000000L -1L; sl_test[i].expected = "-7696581394433";
						i++; sl_test[i].num = -0x0000007000000000L -1L; sl_test[i].expected = "-481036337153";
						i++; sl_test[i].num = -0x0000000700000000L -1L; sl_test[i].expected = "-30064771073";
						i++; sl_test[i].num = -0x0000000070000000L -1L; sl_test[i].expected = "-1879048193";
						i++; sl_test[i].num = -0x0000000007000000L -1L; sl_test[i].expected = "-117440513";
						i++; sl_test[i].num = -0x0000000000700000L -1L; sl_test[i].expected = "-7340033";
						i++; sl_test[i].num = -0x0000000000070000L -1L; sl_test[i].expected = "-458753";
						i++; sl_test[i].num = -0x0000000000007000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x0000000000000700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x0000000000000070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x0000000000000007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num =  0x0000000000000000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#endif
					for(i = 1; i <= num_slong_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							sl_test[i].result[j] = 'X';
						sl_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(sl_test[i].result, sizeof(sl_test[i].result), "%ld", sl_test[i].num);
						if(memcmp(sl_test[i].result, sl_test[i].expected, strlen(sl_test[i].expected))) {
							printf("signed long test #%.2d: Failed (Expected: %s Got: %s)\n", i, sl_test[i].expected, sl_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed long tests OK!\n");
					else
						printf("Some curl_mprintf() signed long tests Failed!\n");
					return failed;
				}
				static int test_curl_off_t_formatting(void)
				{
					int i, j;
					int num_cofft_tests = 0;
					int failed = 0;
				//#if (SIZEOF_CURL_OFF_T == 2)
					if(sizeof(off_t) == 2) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFE); co_test[i].expected = "32766";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFD); co_test[i].expected = "32765";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x07F0); co_test[i].expected = "2032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x5000); co_test[i].expected = "20480";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0500); co_test[i].expected = "1280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0050); co_test[i].expected = "80";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0005); co_test[i].expected = "5";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32767";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32766";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x07F0) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x5000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-20481";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0500) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0050) -MPRNT_OFF_T_C(1); co_test[i].expected = "-81";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0005) -MPRNT_OFF_T_C(1); co_test[i].expected = "-6";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#elif (SIZEOF_CURL_OFF_T == 4)
					else if(sizeof(off_t) == 4) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFF); co_test[i].expected = "2147483647";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFE); co_test[i].expected = "2147483646";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFD); co_test[i].expected = "2147483645";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFF0000); co_test[i].expected = "2147418112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F000000); co_test[i].expected = "2130706432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F0000); co_test[i].expected = "8323072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x70000000); co_test[i].expected = "1879048192";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x07000000); co_test[i].expected = "117440512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00700000); co_test[i].expected = "7340032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00070000); co_test[i].expected = "458752";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x50000000); co_test[i].expected = "1342177280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x05000000); co_test[i].expected = "83886080";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00500000); co_test[i].expected = "5242880";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00050000); co_test[i].expected = "327680";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00005000); co_test[i].expected = "20480";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000500); co_test[i].expected = "1280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000050); co_test[i].expected = "80";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000005); co_test[i].expected = "5";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483648";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483647";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483646";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147418113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2130706433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8323073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x70000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1879048193";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x07000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-117440513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00700000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7340033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00070000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-458753";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x50000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1342177281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x05000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-83886081";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00500000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-5242881";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00050000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-327681";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00005000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-20481";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000500) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000050) -MPRNT_OFF_T_C(1); co_test[i].expected = "-81";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000005) -MPRNT_OFF_T_C(1); co_test[i].expected = "-6";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#elif (SIZEOF_CURL_OFF_T == 8)
					else if(sizeof(off_t) == 8) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFF); co_test[i].expected = "9223372036854775807";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFE); co_test[i].expected = "9223372036854775806";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFD); co_test[i].expected = "9223372036854775805";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFF00000000); co_test[i].expected = "9223372032559808512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007FFFFFFF); co_test[i].expected = "2147483647";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFF000000000000); co_test[i].expected = "9223090561878065152";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007FFF00000000); co_test[i].expected = "140733193388032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007FFF0000); co_test[i].expected = "2147418112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F00000000000000); co_test[i].expected = "9151314442816847872";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F000000000000); co_test[i].expected = "35747322042253312";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007F0000000000); co_test[i].expected = "139637976727552";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007F00000000); co_test[i].expected = "545460846592";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007F000000); co_test[i].expected = "2130706432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000000007F0000); co_test[i].expected = "8323072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000000000007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7000000000000000); co_test[i].expected = "8070450532247928832";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0700000000000000); co_test[i].expected = "504403158265495552";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0070000000000000); co_test[i].expected = "31525197391593472";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0007000000000000); co_test[i].expected = "1970324836974592";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000700000000000); co_test[i].expected = "123145302310912";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000070000000000); co_test[i].expected = "7696581394432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007000000000); co_test[i].expected = "481036337152";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000700000000); co_test[i].expected = "30064771072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000070000000); co_test[i].expected = "1879048192";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000007000000); co_test[i].expected = "117440512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000700000); co_test[i].expected = "7340032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000070000); co_test[i].expected = "458752";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775808";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775807";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775806";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFF00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372032559808513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007FFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483648";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223090561878065153";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007FFF00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-140733193388033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007FFF0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147418113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F00000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9151314442816847873";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-35747322042253313";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007F0000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-139637976727553";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007F00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-545460846593";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007F000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2130706433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000000007F0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8323073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000000000007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7000000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8070450532247928833";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0700000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-504403158265495553";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0070000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-31525197391593473";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0007000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1970324836974593";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000700000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-123145302310913";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000070000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7696581394433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-481036337153";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000700000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-30064771073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000070000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1879048193";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000007000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-117440513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000700000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7340033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000070000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-458753";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x0000000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#endif
					for(i = 1; i <= num_cofft_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							co_test[i].result[j] = 'X';
						co_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(co_test[i].result, sizeof(co_test[i].result), "%" "lld", co_test[i].num);
						if(memcmp(co_test[i].result, co_test[i].expected, strlen(co_test[i].expected))) {
							printf("curl_off_t test #%.2d: Failed (Expected: %s Got: %s)\n", i, co_test[i].expected, co_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() curl_off_t tests OK!\n");
					else
						printf("Some curl_mprintf() curl_off_t tests Failed!\n");
					return failed;
				}
				static int _string_check(int linenumber, char * buf, const char * buf2)
				{
					if(strcmp(buf, buf2)) {
						/* they shouldn't differ */
						printf("sprintf line %d failed:\nwe      '%s'\nsystem: '%s'\n", linenumber, buf, buf2);
						return 1;
					}
					return 0;
				}
				#define string_check(x, y) _string_check(__LINE__, x, y)
				static int _strlen_check(int linenumber, char * buf, size_t len)
				{
					size_t buflen = strlen(buf);
					if(len != buflen) {
						/* they shouldn't differ */
						printf("sprintf strlen:%d failed:\nwe '%zu'\nsystem: '%zu'\n", linenumber, buflen, len);
						return 1;
					}
					return 0;
				}
				#define strlen_check(x, y) _strlen_check(__LINE__, x, y)
				/*
				 * The output strings in this test need to have been verified with a system
				 * sprintf() before used here.
				 */
				static int test_string_formatting(void)
				{
					int errors = 0;
					char buf[256];
					slsprintf_s(buf, sizeof(buf), "%0*d%s", 2, 9, "foo");
					errors += string_check(buf, "09foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 5, 2, "foo");
					errors += string_check(buf, "   fo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 2, 5, "foo");
					errors += string_check(buf, "foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 0, 10, "foo");
					errors += string_check(buf, "foo");
					slsprintf_s(buf, sizeof(buf), "%-10s", "foo");
					errors += string_check(buf, "foo       ");
					slsprintf_s(buf, sizeof(buf), "%10s", "foo");
					errors += string_check(buf, "       foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", -10, -10, "foo");
					errors += string_check(buf, "foo       ");
					if(!errors)
						printf("All curl_mprintf() strings tests OK!\n");
					else
						printf("Some curl_mprintf() string tests Failed!\n");
					return errors;
				}
				static int test_weird_arguments()
				{
					int errors = 0;
					char buf[256];
					int rc;
					/* MAX_PARAMETERS is 128, try exact 128! */
					rc = slsprintf_s(buf, sizeof(buf),
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 1 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 2 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 3 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 4 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 5 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 6 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 7 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 8 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 9 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 11 */
						"%d%d%d%d%d%d%d%d"           /* 8 */
						,
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 1 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 2 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 3 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 4 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 5 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 6 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 7 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 8 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 9 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 11 */
						0, 1, 2, 3, 4, 5, 6, 7);       /* 8 */
					if(rc != 128) {
						printf("curl_mprintf() returned %d and not 128!\n", rc);
						errors++;
					}
					errors += string_check(buf,
						"0123456789"          /* 10 */
						"0123456789"          /* 10 1 */
						"0123456789"          /* 10 2 */
						"0123456789"          /* 10 3 */
						"0123456789"          /* 10 4 */
						"0123456789"          /* 10 5 */
						"0123456789"          /* 10 6 */
						"0123456789"          /* 10 7 */
						"0123456789"          /* 10 8 */
						"0123456789"          /* 10 9 */
						"0123456789"          /* 10 10*/
						"0123456789"          /* 10 11 */
						"01234567"            /* 8 */
						);
					/* MAX_PARAMETERS is 128, try more! */
					buf[0] = 0;
					rc = slsprintf_s(buf, sizeof(buf),
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 1 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 2 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 3 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 4 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 5 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 6 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 7 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 8 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 9 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 11 */
						"%d%d%d%d%d%d%d%d%d"         /* 9 */
						,
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 1 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 2 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 3 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 4 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 5 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 6 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 7 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 8 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 9 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 11 */
						0, 1, 2, 3, 4, 5, 6, 7, 8);         /* 9 */

					if(rc != -1) {
						printf("curl_mprintf() returned %d and not -1!\n", rc);
						errors++;
					}
					errors += string_check(buf, "");
					/* Do not skip sanity checks with parameters! */
					buf[0] = 0;
					rc = slsprintf_s(buf, sizeof(buf), "%d, %.*1$d", 500, 1);
					if(rc != 256) {
						printf("curl_mprintf() returned %d and not 256!\n", rc);
						errors++;
					}
					errors += strlen_check(buf, 255);
					if(errors)
						printf("Some curl_mprintf() weird arguments tests failed!\n");
					return errors;
				}

				#define MAXIMIZE -1.7976931348623157081452E+308 // DBL_MAX value from Linux  !checksrc! disable PLUSNOSPACE 1 

				static int test_float_formatting(void)
				{
					int errors = 0;
					char buf[512]; /* larger than max float size */
					slsprintf_s(buf, sizeof(buf), "%f", 9.0);
					errors += string_check(buf, "9.000000");
					slsprintf_s(buf, sizeof(buf), "%.1f", 9.1);
					errors += string_check(buf, "9.1");
					slsprintf_s(buf, sizeof(buf), "%.2f", 9.1);
					errors += string_check(buf, "9.10");
					slsprintf_s(buf, sizeof(buf), "%.0f", 9.1);
					errors += string_check(buf, "9");
					slsprintf_s(buf, sizeof(buf), "%0f", 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%10f", 9.1);
					errors += string_check(buf, "  9.100000");
					slsprintf_s(buf, sizeof(buf), "%10.3f", 9.1);
					errors += string_check(buf, "     9.100");
					slsprintf_s(buf, sizeof(buf), "%-10.3f", 9.1);
					errors += string_check(buf, "9.100     ");
					slsprintf_s(buf, sizeof(buf), "%-10.3f", 9.123456);
					errors += string_check(buf, "9.123     ");
					slsprintf_s(buf, sizeof(buf), "%.-2f", 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 10, 9.1);
					errors += string_check(buf, "  9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 3, 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.2987654);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.298765);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.29876);
					errors += string_check(buf, "9.298760");
					slsprintf_s(buf, sizeof(buf), "%.*f", 6, 9.2987654);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%.*f", 5, 9.2987654);
					errors += string_check(buf, "9.29877");
					slsprintf_s(buf, sizeof(buf), "%.*f", 4, 9.2987654);
					errors += string_check(buf, "9.2988");
					slsprintf_s(buf, sizeof(buf), "%.*f", 3, 9.2987654);
					errors += string_check(buf, "9.299");
					slsprintf_s(buf, sizeof(buf), "%.*f", 2, 9.2987654);
					errors += string_check(buf, "9.30");
					slsprintf_s(buf, sizeof(buf), "%.*f", 1, 9.2987654);
					errors += string_check(buf, "9.3");
					slsprintf_s(buf, sizeof(buf), "%.*f", 0, 9.2987654);
					errors += string_check(buf, "9");
					/* very large precisions easily turn into system specific outputs so we only
					   check the output buffer length here as we know the internal limit */
					slsprintf_s(buf, sizeof(buf), "%.*f", (1<<30), 9.2987654);
					errors += strlen_check(buf, 325);
					slsprintf_s(buf, sizeof(buf), "%10000.10000f", 9.2987654);
					errors += strlen_check(buf, 325);
					slsprintf_s(buf, sizeof(buf), "%240.10000f", 123456789123456789123456789.2987654);
					errors += strlen_check(buf, 325);
					/* check negative when used signed */
					slsprintf_s(buf, sizeof(buf), "%*f", INT_MIN, 9.1);
					errors += string_check(buf, "9.100000");
					// slsprintf_s() limits a single float output to 325 bytes maximum width 
					slsprintf_s(buf, sizeof(buf), "%*f", (1<<30), 9.1);
					errors += string_check(buf, "                                                                                                                                                                                                                                                                                                                             9.100000");
					slsprintf_s(buf, sizeof(buf), "%100000f", 9.1);
					errors += string_check(buf, "                                                                                                                                                                                                                                                                                                                             9.100000");
					slsprintf_s(buf, sizeof(buf), "%f", MAXIMIZE);
					errors += strlen_check(buf, 317);
					slsprintf_s(buf, 2, "%f", MAXIMIZE);
					errors += strlen_check(buf, 1);
					slsprintf_s(buf, 3, "%f", MAXIMIZE);
					errors += strlen_check(buf, 2);
					slsprintf_s(buf, 4, "%f", MAXIMIZE);
					errors += strlen_check(buf, 3);
					slsprintf_s(buf, 5, "%f", MAXIMIZE);
					errors += strlen_check(buf, 4);
					slsprintf_s(buf, 6, "%f", MAXIMIZE);
					errors += strlen_check(buf, 5);
					if(!errors)
						printf("All float strings tests OK!\n");
					else
						printf("test_float_formatting Failed!\n");
					return errors;
				}
			};
		}
#endif // } 0 @construction
	}
	else if(bm == 1) { // slprintf
		SString temp_buf;
		{
			for(long i = -2000000; i <= +1000000; i++) {
				slprintf(temp_buf.Z(), "this is a long number %ld", i);
			}
		}
		{
			for(int64 i = -2000000; i <= +1000000; i++) {
				slprintf(temp_buf.Z(), "this is a long long number %llx", i);
			}
		}
		{
			for(double v = -1.0e15; v < 1.0e15;) {
				slprintf(temp_buf.Z(), "this is a real number %.5f", v);
				double rn = 0.8e9;
				v += rn;
			}
		}
	}
	else if(bm == 2) { // sprintf
		char  temp_buf[1024];
		{
			for(long i = -2000000; i <= +1000000; i++) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a long number %ld", i);
			}
		}
		{
			for(int64 i = -2000000; i <= +1000000; i++) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a long long number %llx", i);
			}
		}
		{
			for(double v = -1.0e15; v < 1.0e15;) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a real number %.5f", v);
				double rn = 0.8e9;
				v += rn;
			}
		}
	}
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
