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
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
// embedded systems with a very limited resources. These routines are thread safe and reentrant!
// Use this instead of the bloated standard/newlib printf cause these use malloc for printf (and may not be thread safe).
// ---------------------------------
// Adopted to slib by A.Sobolev 2020
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
									dconvstr_flags |= SRealConversion::fUppercase;
								if(flags & FLAGS_PLUS)
									dconvstr_flags |= SRealConversion::fPrintPlus;
								if(flags & FLAGS_LEFT)
									dconvstr_flags |= SRealConversion::fLeftJustify;
								if(flags & FLAGS_ZEROPAD)
									dconvstr_flags |= SRealConversion::fPadWithZero;
								if(width != 0)
									dconvstr_flags |= SRealConversion::fHaveWidth;
								if(!(flags & FLAGS_PRECISION))
									precision = PRINTF_DEFAULT_FLOAT_PRECISION;
								SRealConversion::Print(&p_fout_buf, &fout_buf_size, value, 'f', dconvstr_flags, width, precision);
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
									dconvstr_flags |= SRealConversion::fUppercase;
								if(flags & FLAGS_PLUS)
									dconvstr_flags |= SRealConversion::fPrintPlus;
								if(flags & FLAGS_LEFT)
									dconvstr_flags |= SRealConversion::fLeftJustify;
								if(flags & FLAGS_ZEROPAD)
									dconvstr_flags |= SRealConversion::fPadWithZero;
								if(width != 0)
									dconvstr_flags |= SRealConversion::fHaveWidth;
								if(!(flags & FLAGS_PRECISION))
									precision = PRINTF_DEFAULT_FLOAT_PRECISION;
								SRealConversion::Print(&p_fout_buf, &fout_buf_size, value, dconv_format_char, dconvstr_flags, width, precision);
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
							const char * p = va_arg(va, char*);
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
								_ntoa_long_long(rBuf, (uintptr_t)va_arg(va, void*), false, 16U, precision, width, flags);
							else
								_ntoa_long(rBuf, (ulong)((uintptr_t)va_arg(va, void*)), false, 16U, precision, width, flags);
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
			SLTEST_CHECK_EQ(sbuf, "\n");
			slprintf(sbuf.Z(), "");
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%%");
			SLTEST_CHECK_EQ(sbuf, "%");
			slprintf(sbuf.Z(), "%%%s", "beef");
			SLTEST_CHECK_EQ(sbuf, "%beef");
			slprintf(sbuf.Z(), "zero ptr string=\"%s\"", 0);
			SLTEST_CHECK_EQ(sbuf, "zero ptr string=\"\"");
		}
		{ // TEST_CASE("printf", "[]" )
			SLTEST_CHECK_EQ(slprintf(sbuf.Z(), "% d", 4232), 5L);
			SLTEST_CHECK_EQ(sbuf.Len(), 5U);
			SLTEST_CHECK_EQ(sbuf.cptr()[5], 0L);
			SLTEST_CHECK_EQ(sbuf, " 4232");
		}
		{ // TEST_CASE("fctprintf", "[]" )
			SLTEST_CHECK_EQ(slprintf(sbuf.Z(), "This is a test of %X", 0x12EFU), 22L);
			SLTEST_CHECK_EQ(sbuf.Len(), 22U);
			SLTEST_CHECK_EQ(sbuf, "This is a test of 12EF");
		}
		{ // TEST_CASE("snprintf", "[]" )
			slprintf(sbuf.Z(), "%d", -1000);
			SLTEST_CHECK_EQ(sbuf, "-1000");
			//slprintf(buffer, 3U, "%d", -1000);
			//SLTEST_CHECK_EQ(sbuf, "-1"));
		}
		{ // TEST_CASE("space flag", "[]" )
			slprintf(sbuf.Z(), "% d", 42);
			SLTEST_CHECK_EQ(sbuf, " 42");
			slprintf(sbuf.Z(), "% d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "% 5d", 42);
			SLTEST_CHECK_EQ(sbuf, "   42");
			slprintf(sbuf.Z(), "% 5d", -42);
			SLTEST_CHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "% 15d", 42);
			SLTEST_CHECK_EQ(sbuf, "             42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLTEST_CHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLTEST_CHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15.3f", -42.987);
			SLTEST_CHECK_EQ(sbuf, "        -42.987");
			slprintf(sbuf.Z(), "% 15.3f", 42.987);
			SLTEST_CHECK_EQ(sbuf, "         42.987");
			slprintf(sbuf.Z(), "% s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "% d", 1024);
			SLTEST_CHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% i", 1024);
			SLTEST_CHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "% u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "% o", 511);
			SLTEST_CHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "% o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "% x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "% x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "% X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "% X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "% c", 'x');
			SLTEST_CHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("+ flag", "[]" )
			slprintf(sbuf.Z(), "%+d", 42);
			SLTEST_CHECK_EQ(sbuf, "+42");
			slprintf(sbuf.Z(), "%+d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%+5d", 42);
			SLTEST_CHECK_EQ(sbuf, "  +42");
			slprintf(sbuf.Z(), "%+5d", -42);
			SLTEST_CHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "%+15d", 42);
			SLTEST_CHECK_EQ(sbuf, "            +42");
			slprintf(sbuf.Z(), "%+15d", -42);
			SLTEST_CHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "%+s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%+d", 1024);
			SLTEST_CHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+i", 1024);
			SLTEST_CHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%+u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%+o", 511);
			SLTEST_CHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%+o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%+x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%+x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%+X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%+X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%+c", 'x');
			SLTEST_CHECK_EQ(sbuf, "x");
			slprintf(sbuf.Z(), "%+.0d", 0);
			SLTEST_CHECK_EQ(sbuf, "+");
		}
		{ // TEST_CASE("0 flag", "[]" )
			slprintf(sbuf.Z(), "%0d", 42);
			SLTEST_CHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0ld", 42L);
			SLTEST_CHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%05d", 42);
			SLTEST_CHECK_EQ(sbuf, "00042");
			slprintf(sbuf.Z(), "%05d", -42);
			SLTEST_CHECK_EQ(sbuf, "-0042");
			slprintf(sbuf.Z(), "%015d", 42);
			SLTEST_CHECK_EQ(sbuf, "000000000000042");
			slprintf(sbuf.Z(), "%015d", -42);
			SLTEST_CHECK_EQ(sbuf, "-00000000000042");
			slprintf(sbuf.Z(), "%015.2f", 42.1234);
			SLTEST_CHECK_EQ(sbuf, "000000000042.12");
			slprintf(sbuf.Z(), "%015.3f", 42.9876);
			SLTEST_CHECK_EQ(sbuf, "00000000042.988");
			slprintf(sbuf.Z(), "%015.5f", -42.9876);
			SLTEST_CHECK_EQ(sbuf, "-00000042.98760");
		}
		{ // TEST_CASE("- flag", "[]" )
			slprintf(sbuf.Z(), "%-d", 42);
			SLTEST_CHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-5d", 42);
			SLTEST_CHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-5d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-15d", 42);
			SLTEST_CHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-15d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%-0d", 42);
			SLTEST_CHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-0d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-05d", 42);
			SLTEST_CHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-05d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-015d", 42);
			SLTEST_CHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-015d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-d", 42);
			SLTEST_CHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0-d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%0-5d", 42);
			SLTEST_CHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%0-5d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%0-15d", 42);
			SLTEST_CHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%0-15d", -42);
			SLTEST_CHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-15.3e", -42.);
			SLTEST_CHECK_EQ(sbuf, "-4.200e+01     ");
			sprintf(flat_buffer, "%0-15.3g", -42.); // @proof
			slprintf(sbuf.Z(), "%0-15.3g", -42.);
			SLTEST_CHECK_EQ(sbuf, "-42            ");
		}
		{ // TEST_CASE("# flag", "[]" )
			slprintf(sbuf.Z(), "%#.0x", 0);
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.1x", 0);
			SLTEST_CHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%#.0llx", (int64)0);
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.8x", 0x614e);
			SLTEST_CHECK_EQ(sbuf, "0x0000614e");
			slprintf(sbuf.Z(), "%#b", 6);
			SLTEST_CHECK_EQ(sbuf, "0b110");
		}
		{ // TEST_CASE("specifier", "[]" )
			slprintf(sbuf.Z(), "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%d", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%i", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%o", 511);
			SLTEST_CHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%%");
			SLTEST_CHECK_EQ(sbuf, "%");
		}
		{ // TEST_CASE("width", "[]" )
			slprintf(sbuf.Z(), "%1s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%1d", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1i", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%1o", 511);
			SLTEST_CHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%1o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%1x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%1x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%1X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%1X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%1c", 'x');
			SLTEST_CHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("width 20", "[]" )
			slprintf(sbuf.Z(), "%20s", "Hello");
			SLTEST_CHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%20d", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20d", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20i", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20i", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20u", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20o", 511);
			SLTEST_CHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20c", 'x');
			SLTEST_CHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width *20", "[]" )
			slprintf(sbuf.Z(), "%*s", 20, "Hello");
			SLTEST_CHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%*d", 20, 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*d", 20, -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*i", 20, 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*i", 20, -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*u", 20, 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*u", 20, 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%*o", 20, 511);
			SLTEST_CHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%*o", 20, 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%*x", 20, 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%*x", 20, 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%*X", 20, 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%*X", 20, 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%*c", 20,'x');
			SLTEST_CHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width -20", "[]" )
			slprintf(sbuf.Z(), "%-20s", "Hello");
			SLTEST_CHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%-20d", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20i", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20.4f", 1024.1234);
			SLTEST_CHECK_EQ(sbuf, "1024.1234           ");
			slprintf(sbuf.Z(), "%-20u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%-20o", 511);
			SLTEST_CHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%-20o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%-20x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%-20x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%-20X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%-20X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%-20c", 'x');
			SLTEST_CHECK_EQ(sbuf, "x                   ");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 9, 9, 9);
			SLTEST_CHECK_EQ(sbuf, "|    9| |9 | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 10, 10, 10);
			SLTEST_CHECK_EQ(sbuf, "|   10| |10| |   10|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 9, 9, 9);
			SLTEST_CHECK_EQ(sbuf, "|    9| |9           | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 10, 10, 10);
			SLTEST_CHECK_EQ(sbuf, "|   10| |10          | |   10|");
		}
		{ // TEST_CASE("width 0-20", "[]" )
			slprintf(sbuf.Z(), "%0-20s", "Hello");
			SLTEST_CHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%0-20d", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20i", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20u", 1024);
			SLTEST_CHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%0-20o", 511);
			SLTEST_CHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%0-20o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%0-20x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%0-20x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%0-20X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%0-20X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%0-20c", 'x');
			SLTEST_CHECK_EQ(sbuf, "x                   ");
		}
		{ // TEST_CASE("padding 20", "[]" ) 
			slprintf(sbuf.Z(), "%020d", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020i", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020u", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%020o", 511);
			SLTEST_CHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%020o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%020x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%020x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%020X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%020X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding .20", "[]" )
			slprintf(sbuf.Z(), "%.20d", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%.20o", 511);
			SLTEST_CHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%.20o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%.20x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%.20x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%.20X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%.20X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding #020", "[]" )
			slprintf(sbuf.Z(), "%#020d", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020d", -1024);
			SLTEST_CHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020i", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020i", -1024);
			SLTEST_CHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 1024);
			SLTEST_CHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%#020o", 511);
			SLTEST_CHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%#020o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%#020x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0x00000000001234abcd");
			slprintf(sbuf.Z(), "%#020x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "0x0000000000edcb5433");
			slprintf(sbuf.Z(), "%#020X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "0X00000000001234ABCD");
			slprintf(sbuf.Z(), "%#020X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "0X0000000000EDCB5433");
		}
		{ // TEST_CASE("padding #20", "[]" )
			slprintf(sbuf.Z(), "%#20d", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20d", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20i", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20i", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20u", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%#20o", 511);
			SLTEST_CHECK_EQ(sbuf, "                0777");
			slprintf(sbuf.Z(), "%#20o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "        037777777001");
			slprintf(sbuf.Z(), "%#20x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "          0x1234abcd");
			slprintf(sbuf.Z(), "%#20x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "          0xedcb5433");
			slprintf(sbuf.Z(), "%#20X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "          0X1234ABCD");
			slprintf(sbuf.Z(), "%#20X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "          0XEDCB5433");
		}
		{ // TEST_CASE("padding 20.5", "[]" )
			slprintf(sbuf.Z(), "%20.5d", 1024);
			SLTEST_CHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5d", -1024);
			SLTEST_CHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5i", 1024);
			SLTEST_CHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5i", -1024);
			SLTEST_CHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5u", 1024);
			SLTEST_CHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.5o", 511);
			SLTEST_CHECK_EQ(sbuf, "               00777");
			slprintf(sbuf.Z(), "%20.5o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.5x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20.10x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "          00edcb5433");
			slprintf(sbuf.Z(), "%20.5X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.10X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "          00EDCB5433");
		}
		{ // TEST_CASE("padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 1d", -5);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 2d", -5);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 3d", -5);
			SLTEST_CHECK_EQ(sbuf, " -5");
			slprintf(sbuf.Z(), "% 4d", -5);
			SLTEST_CHECK_EQ(sbuf, "  -5");
			// zero padding
			slprintf(sbuf.Z(), "%01d", -5);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02d", -5);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03d", -5);
			SLTEST_CHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%04d", -5);
			SLTEST_CHECK_EQ(sbuf, "-005");
		}
		{ // TEST_CASE("float padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 3.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 4.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 5.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, " -5.0");
			slprintf(sbuf.Z(), "% 6.1g", -5.);
			SLTEST_CHECK_EQ(sbuf, "    -5");
			slprintf(sbuf.Z(), "% 6.1e", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5.0e+00");
			slprintf(sbuf.Z(), "% 10.1e", -5.);
			SLTEST_CHECK_EQ(sbuf, "  -5.0e+00");
			// zero padding
			slprintf(sbuf.Z(), "%03.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%04.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%05.1f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-05.0");
			// zero padding no decimal point
			slprintf(sbuf.Z(), "%01.0f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02.0f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03.0f", -5.);
			SLTEST_CHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%010.1e", -5.);
			SLTEST_CHECK_EQ(sbuf, "-005.0e+00");
			slprintf(sbuf.Z(), "%07.0E", -5.);
			SLTEST_CHECK_EQ(sbuf, "-05E+00");
			slprintf(sbuf.Z(), "%03.0g", -5.);
			SLTEST_CHECK_EQ(sbuf, "-05");
		}
		{ // TEST_CASE("length", "[]" )
			slprintf(sbuf.Z(), "%.0s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.0s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%.s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.s", "Hello testing");
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0d", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0d", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.d", 0);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0i", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.i", -1024);
			SLTEST_CHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.i", 0);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.u", 1024);
			SLTEST_CHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0u", 4294966272U);
			SLTEST_CHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.u", 0U);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.o", 511);
			SLTEST_CHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20.0o", 4294966785U);
			SLTEST_CHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.o", 0U);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%50.x", 305441741);
			SLTEST_CHECK_EQ(sbuf, "                                          1234abcd");
			slprintf(sbuf.Z(), "%50.x%10.u", 305441741, 12345);
			SLTEST_CHECK_EQ(sbuf, "                                          1234abcd     12345");
			slprintf(sbuf.Z(), "%20.0x", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20.x", 0U);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.X", 305441741);
			SLTEST_CHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.0X", 3989525555U);
			SLTEST_CHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20.X", 0U);
			SLTEST_CHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%02.0u", 0U);
			SLTEST_CHECK_EQ(sbuf, "  ");
			slprintf(sbuf.Z(), "%02.0d", 0);
			SLTEST_CHECK_EQ(sbuf, "  ");
		}
		{ // TEST_CASE("float", "[]" )
			// test special-case floats using math.h macros
			slprintf(sbuf.Z(), "%8f", fgetnan());
			SLTEST_CHECK_EQ(sbuf, "     nan");
			slprintf(sbuf.Z(), "%8f", fgetposinf());
			SLTEST_CHECK_EQ(sbuf, "     inf");
			slprintf(sbuf.Z(), "%-8f", fgetneginf());
			SLTEST_CHECK_EQ(sbuf, "-inf    ");
			slprintf(sbuf.Z(), "%+8e", fgetposinf());
			SLTEST_CHECK_EQ(sbuf, "    +inf");
			slprintf(sbuf.Z(), "%.4f", 3.1415354);
			SLTEST_CHECK_EQ(sbuf, "3.1415");
			slprintf(sbuf.Z(), "%.3f", 30343.1415354);
			SLTEST_CHECK_EQ(sbuf, "30343.142");
			slprintf(sbuf.Z(), "%.0f", 34.1415354);
			SLTEST_CHECK_EQ(sbuf, "34");
			slprintf(sbuf.Z(), "%.0f", 1.3);
			SLTEST_CHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.0f", 1.55);
			SLTEST_CHECK_EQ(sbuf, "2");
			slprintf(sbuf.Z(), "%.1f", 1.64);
			SLTEST_CHECK_EQ(sbuf, "1.6");
			slprintf(sbuf.Z(), "%.2f", 42.8952);
			SLTEST_CHECK_EQ(sbuf, "42.90");
			slprintf(sbuf.Z(), "%.9f", 42.8952);
			SLTEST_CHECK_EQ(sbuf, "42.895200000");
			slprintf(sbuf.Z(), "%.10f", 42.895223);
			SLTEST_CHECK_EQ(sbuf, "42.8952230000");
			// this testcase checks, that the precision is truncated to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522312345678);
			SLTEST_CHECK_EQ(sbuf, "42.895223123457"); // @sobolev "42.895223123000"-->"42.895223123457"
			// this testcase checks, that the precision is truncated AND rounded to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522387654321);
			SLTEST_CHECK_EQ(sbuf, "42.895223876543"); // @sobolev "42.895223877000"-->"42.895223876543"
			slprintf(sbuf.Z(), "%6.2f", 42.8952);
			SLTEST_CHECK_EQ(sbuf, " 42.90");
			slprintf(sbuf.Z(), "%+6.2f", 42.8952);
			SLTEST_CHECK_EQ(sbuf, "+42.90");
			slprintf(sbuf.Z(), "%+5.1f", 42.9252);
			SLTEST_CHECK_EQ(sbuf, "+42.9");
			sprintf(flat_buffer, "%f", 42.5); // @proof
			slprintf(sbuf.Z(), "%f", 42.5);
			SLTEST_CHECK_EQ(sbuf, "42.500000");
			slprintf(sbuf.Z(), "%.1f", 42.5);
			SLTEST_CHECK_EQ(sbuf, "42.5");
			slprintf(sbuf.Z(), "%f", 42167.0);
			SLTEST_CHECK_EQ(sbuf, "42167.000000");
			slprintf(sbuf.Z(), "%.9f", -12345.987654321);
			SLTEST_CHECK_EQ(sbuf, "-12345.987654321");
			slprintf(sbuf.Z(), "%.1f", 3.999);
			SLTEST_CHECK_EQ(sbuf, "4.0");
			slprintf(sbuf.Z(), "%.0f", 3.5);
			SLTEST_CHECK_EQ(sbuf, "4");
			sprintf(flat_buffer, "%.0f", 4.5); // @proof
			slprintf(sbuf.Z(), "%.0f", 4.5);
			SLTEST_CHECK_EQ(sbuf, "5"); // @sobolev "4"-->"5"
			slprintf(sbuf.Z(), "%.0f", 3.49);
			SLTEST_CHECK_EQ(sbuf, "3");
			slprintf(sbuf.Z(), "%.1f", 3.49);
			SLTEST_CHECK_EQ(sbuf, "3.5");
			slprintf(sbuf.Z(), "a%-5.1f", 0.5);
			SLTEST_CHECK_EQ(sbuf, "a0.5  ");
			slprintf(sbuf.Z(), "a%-5.1fend", 0.5);
			SLTEST_CHECK_EQ(sbuf, "a0.5  end");
			slprintf(sbuf.Z(), "%G", 12345.678);
			SLTEST_CHECK_EQ(sbuf, "12345.7");
			slprintf(sbuf.Z(), "%.7G", 12345.678);
			SLTEST_CHECK_EQ(sbuf, "12345.68");
			slprintf(sbuf.Z(), "%.5G", 123456789.);
			SLTEST_CHECK_EQ(sbuf, "1.2346E+08");
			sprintf(flat_buffer, "%.6G", 12345.); // @proof
			slprintf(sbuf.Z(), "%.6G", 12345.);
			SLTEST_CHECK_EQ(sbuf, flat_buffer);
			SLTEST_CHECK_EQ(sbuf, "12345"); // @sobolev "12345.0"-->"12345"
			slprintf(sbuf.Z(), "%+12.4g", 123456789.);
			SLTEST_CHECK_EQ(sbuf, "  +1.235e+08");
			slprintf(sbuf.Z(), "%.2G", 0.001234);
			SLTEST_CHECK_EQ(sbuf, "0.0012");
			slprintf(sbuf.Z(), "%+10.4G", 0.001234);
			SLTEST_CHECK_EQ(sbuf, " +0.001234");
			slprintf(sbuf.Z(), "%+012.4g", 0.00001234);
			SLTEST_CHECK_EQ(sbuf, "+001.234e-05");
			slprintf(sbuf.Z(), "%.3g", -1.2345e-308);
			SLTEST_CHECK_EQ(sbuf, "-1.23e-308");
			slprintf(sbuf.Z(), "%+.3E", 1.23e+308);
			SLTEST_CHECK_EQ(sbuf, "+1.230E+308");
			// out of range for float: should switch to exp notation if supported, else empty
			sprintf(flat_buffer, "%.1f", 1E20); // @proof
			slprintf(sbuf.Z(), "%.1f", 1E20);
			SLTEST_CHECK_EQ(sbuf, flat_buffer);
			SLTEST_CHECK_EQ(sbuf, "100000000000000000000.0"); // @sobolev "1.0e+20"-->"100000000000000000000.0"

			// brute force float
			for(float i = -100000.0f; i < 100000.0f; i += 1.0f) {
				slprintf(sbuf.Z(), "%.5f", i / 10000.0f);
				sprintf(flat_buffer, "%.5f", i / 10000.0f);
				SLTEST_CHECK_EQ(sbuf, flat_buffer);
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
				SLTEST_CHECK_EQ(sbuf, flat_buffer);
			}*/
			for(float i = -1.0e15f; i < 1e15f;) {
				slprintf(sbuf.Z(), "%.5f", i);
				sprintf(flat_buffer, "%.5f", i);
				SLTEST_CHECK_EQ(sbuf, flat_buffer);
				float rn = static_cast<float>(SLS.GetTLA().Rg.GetReal()) * 1.0e9f;
				i += rn;
			}
		}
		{ // TEST_CASE("types", "[]" )
			slprintf(sbuf.Z(), "%i", 0);
			SLTEST_CHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%i", 1234);
			SLTEST_CHECK_EQ(sbuf, "1234");
			slprintf(sbuf.Z(), "%i", 32767);
			SLTEST_CHECK_EQ(sbuf, "32767");
			slprintf(sbuf.Z(), "%i", -32767);
			SLTEST_CHECK_EQ(sbuf, "-32767");
			slprintf(sbuf.Z(), "%li", 30L);
			SLTEST_CHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%li", -2147483647L);
			SLTEST_CHECK_EQ(sbuf, "-2147483647");
			slprintf(sbuf.Z(), "%li", 2147483647L);
			SLTEST_CHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%lli", 30LL);
			SLTEST_CHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%lli", -9223372036854775807LL);
			SLTEST_CHECK_EQ(sbuf, "-9223372036854775807");
			slprintf(sbuf.Z(), "%lli", 9223372036854775807LL);
			SLTEST_CHECK_EQ(sbuf, "9223372036854775807");
			slprintf(sbuf.Z(), "%lu", 100000L);
			SLTEST_CHECK_EQ(sbuf, "100000");
			slprintf(sbuf.Z(), "%lu", 0xFFFFFFFFL);
			SLTEST_CHECK_EQ(sbuf, "4294967295");
			slprintf(sbuf.Z(), "%llu", 281474976710656ULL);
			SLTEST_CHECK_EQ(sbuf, "281474976710656");
			slprintf(sbuf.Z(), "%llu", 18446744073709551615ULL);
			SLTEST_CHECK_EQ(sbuf, "18446744073709551615");
			slprintf(sbuf.Z(), "%zu", 2147483647UL);
			SLTEST_CHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%zd", 2147483647UL);
			SLTEST_CHECK_EQ(sbuf, "2147483647");
			if(sizeof(size_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%zi", -2147483647L);
				SLTEST_CHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%zi", -2147483647LL);
				SLTEST_CHECK_EQ(sbuf, "-2147483647");
			}
			slprintf(sbuf.Z(), "%b", 60000);
			SLTEST_CHECK_EQ(sbuf, "1110101001100000");
			slprintf(sbuf.Z(), "%lb", 12345678L);
			SLTEST_CHECK_EQ(sbuf, "101111000110000101001110");
			slprintf(sbuf.Z(), "%o", 60000);
			SLTEST_CHECK_EQ(sbuf, "165140");
			slprintf(sbuf.Z(), "%lo", 12345678L);
			SLTEST_CHECK_EQ(sbuf, "57060516");
			slprintf(sbuf.Z(), "%lx", 0x12345678L);
			SLTEST_CHECK_EQ(sbuf, "12345678");
			slprintf(sbuf.Z(), "%llx", 0x1234567891234567ULL);
			SLTEST_CHECK_EQ(sbuf, "1234567891234567");
			slprintf(sbuf.Z(), "%lx", 0xabcdefabL);
			SLTEST_CHECK_EQ(sbuf, "abcdefab");
			slprintf(sbuf.Z(), "%lX", 0xabcdefabL);
			SLTEST_CHECK_EQ(sbuf, "ABCDEFAB");
			slprintf(sbuf.Z(), "%c", 'v');
			SLTEST_CHECK_EQ(sbuf, "v");
			slprintf(sbuf.Z(), "%cv", 'w');
			SLTEST_CHECK_EQ(sbuf, "wv");
			slprintf(sbuf.Z(), "%s", "A Test");
			SLTEST_CHECK_EQ(sbuf, "A Test");
			slprintf(sbuf.Z(), "%hhu", 0xFFFFUL);
			SLTEST_CHECK_EQ(sbuf, "255");
			slprintf(sbuf.Z(), "%hu", 0x123456UL);
			SLTEST_CHECK_EQ(sbuf, "13398");
			slprintf(sbuf.Z(), "%s%hhi %hu", "Test", 10000, 0xFFFFFFFF);
			SLTEST_CHECK_EQ(sbuf, "Test16 65535");
			slprintf(sbuf.Z(), "%tx", &flat_buffer[10] - &flat_buffer[0]);
			SLTEST_CHECK_EQ(sbuf, "a");
			// TBD
			if(sizeof(intmax_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%ji", -2147483647L);
				SLTEST_CHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%ji", -2147483647LL);
				SLTEST_CHECK_EQ(sbuf, "-2147483647");
			}
		}
		{ // TEST_CASE("pointer", "[]" )
			slprintf(sbuf.Z(), "%p", (void *)0x1234U);
			if(sizeof(void *) == 4U) {
				SLTEST_CHECK_EQ(sbuf, "00001234");
			}
			else {
				SLTEST_CHECK_EQ(sbuf, "0000000000001234");
			}
			slprintf(sbuf.Z(), "%p", (void *)0x12345678U);
			if(sizeof(void *) == 4U) {
				SLTEST_CHECK_EQ(sbuf, "12345678");
			}
			else {
				SLTEST_CHECK_EQ(sbuf, "0000000012345678");
			}
			slprintf(sbuf.Z(), "%p-%p", (void *)0x12345678U, (void *)0x7EDCBA98U);
			if(sizeof(void *) == 4U) {
				SLTEST_CHECK_EQ(sbuf, "12345678-7EDCBA98");
			}
			else {
				SLTEST_CHECK_EQ(sbuf, "0000000012345678-000000007EDCBA98");
			}
			if(sizeof(uintptr_t) == sizeof(uint64_t)) {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLTEST_CHECK_EQ(sbuf, "00000000FFFFFFFF");
			}
			else {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLTEST_CHECK_EQ(sbuf, "FFFFFFFF");
			}
		}
		{ // TEST_CASE("unknown flag", "[]" )
			slprintf(sbuf.Z(), "%kmarco", 42, 37);
			SLTEST_CHECK_EQ(sbuf, "kmarco");
		}
		{ // TEST_CASE("string length", "[]" )
			slprintf(sbuf.Z(), "%.4s", "This is a test");
			SLTEST_CHECK_EQ(sbuf, "This");
			slprintf(sbuf.Z(), "%.4s", "test");
			SLTEST_CHECK_EQ(sbuf, "test");
			slprintf(sbuf.Z(), "%.7s", "123");
			SLTEST_CHECK_EQ(sbuf, "123");
			slprintf(sbuf.Z(), "%.7s", "");
			SLTEST_CHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%.4s%.2s", "123456", "abcdef");
			SLTEST_CHECK_EQ(sbuf, "1234ab");
			slprintf(sbuf.Z(), "%.4.2s", "123456");
			SLTEST_CHECK_EQ(sbuf, ".2s");
			slprintf(sbuf.Z(), "%.*s", 3, "123456");
			SLTEST_CHECK_EQ(sbuf, "123");
		}
		{ // TEST_CASE("misc", "[]" )
			slprintf(sbuf.Z(), "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit");
			SLTEST_CHECK_EQ(sbuf, "53000atest-20 bit");
			slprintf(sbuf.Z(), "%.*f", 2, 0.33333333);
			SLTEST_CHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*d", -1, 1);
			SLTEST_CHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.3s", "foobar");
			SLTEST_CHECK_EQ(sbuf, "foo");
			slprintf(sbuf.Z(), "% .0d", 0);
			SLTEST_CHECK_EQ(sbuf, " ");
			slprintf(sbuf.Z(), "%10.5d", 4);
			SLTEST_CHECK_EQ(sbuf, "     00004");
			slprintf(sbuf.Z(), "%*sx", -3, "hi");
			SLTEST_CHECK_EQ(sbuf, "hi x");
			slprintf(sbuf.Z(), "%.*g", 2, 0.33333333);
			SLTEST_CHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*e", 2, 0.33333333);
			SLTEST_CHECK_EQ(sbuf, "3.33e-01");
		}
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
