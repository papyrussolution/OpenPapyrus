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
// Adopted to slib by A.Sobolev 2020..2022, 2023, 2025
//
#include <slib-internal.h>
#pragma hdrstop

//int FASTCALL dconvstr_print(char ** ppOutBuf, int * pOutBufSize, double value, int formatChar, uint formatFlags, int formatWidth, int formatPrecision); // @prototype

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack). default: 32 byte
//#define PRINTF_FTOA_BUFFER_SIZE    64U // @sobolev 32-->64
// @v11.8.6 (replaced with SlConst::DefaultPrintfFloatPrec) #define PRINTF_DEFAULT_FLOAT_PRECISION  6U // define the default floating point precision. default: 6 digits
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

static int sl_printf_implementation(SString & rBuf, const char * pFormat, va_list va)
{
	const   uint org_len = rBuf.Len();
	const   size_t format_len = strlen(pFormat);
	if(format_len) {
		char    fout_buf[256];
		const   char * p_format_end = pFormat + format_len + 1;
		while(*pFormat) {
			// format specifier?  %[flags][width][.precision][length]
			if(*pFormat != '%') {
				//rBuf.CatChar(*pFormat);
				//pFormat++;
				const char * p_next_pct = static_cast<const char *>(smemchr(pFormat, '%', p_format_end-pFormat));
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
							default: n = 0U; break;
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
									precision = SlConst::DefaultPrintfFloatPrec;
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
									precision = SlConst::DefaultPrintfFloatPrec;
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
								zero[0] = 0;
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
