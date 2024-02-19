/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

static uchar * ngx_sprintf_num(uchar * buf, uchar * last, uint64_t ui64, uchar zero, ngx_uint_t hexadecimal, ngx_uint_t width);
static void ngx_encode_base64_internal(ngx_str_t * dst, const ngx_str_t * src, const uchar * basis, ngx_uint_t padding);
static ngx_int_t ngx_decode_base64_internal(ngx_str_t * dst, const ngx_str_t * src, const uchar * basis);

void FASTCALL ngx_strlow(uchar * dst, const uchar * src, size_t n)
{
	while(n) {
		*dst = stolower_ascii(*src);
		dst++;
		src++;
		n--;
	}
}

uchar * FASTCALL ngx_cpystrn(uchar * dst, const uchar * src, size_t n)
{
	if(n) {
		while(--n) {
			*dst = *src;
			if(*dst == '\0') {
				return dst;
			}
			dst++;
			src++;
		}
		*dst = '\0';
	}
	return dst;
}

uchar * FASTCALL ngx_pstrdup(ngx_pool_t * pool, const ngx_str_t * src)
{
	uchar * dst = (uchar *)ngx_pnalloc(pool, src->len);
	if(dst)
		memcpy(dst, src->data, src->len);
	return dst;
}

int SStrDupToNgxStr(ngx_pool_t * pPool, const SString * pSrc, ngx_str_t * pDest)
{
	int    ok = 1;
	if(pSrc && pSrc->Len()) {
		pDest->data = (uchar *)ngx_pnalloc(pPool, pSrc->Len()+1);
		if(pDest->data) {
			pDest->len = pSrc->Len();
			memcpy(pDest->data, pSrc->ucptr(), pSrc->Len()+1);
		}
		else {
			pDest->len = 0;
			ok = 0;
		}
	}
	else {
		pDest->data = 0;
		pDest->len = 0;
	}
	return ok;
}
/*
 * supported formats:
 *  %[0][width][x][X]O        nginx_off_t
 *  %[0][width]T              time_t
 *  %[0][width][u][x|X]z      ssize_t/size_t
 *  %[0][width][u][x|X]d      int/u_int
 *  %[0][width][u][x|X]l      long
 *  %[0][width|m][u][x|X]i    ngx_int_t/ngx_uint_t
 *  %[0][width][u][x|X]D      int32_t/uint32_t
 *  %[0][width][u][x|X]L      int64_t/uint64_t
 *  %[0][width|m][u][x|X]A    ngx_atomic_int_t/ngx_atomic_uint_t
 *  %[0][width][.width]f      double, max valid number fits to %18.15f
 *  %P                        ngx_pid_t
 *  %M                        ngx_msec_t
 *  %r                        rlim_t
 *  %p                        void *
 *  %V                        ngx_str_t *
 *  %v                        ngx_variable_value_t *
 *  %s                        null-terminated string
 *  %*s                       length and string
 *  %Z                        '\0'
 *  %N                        '\n'
 *  %c                        char
 *  %%                        %
 *
 *  reserved:
 *  %t                        ptrdiff_t
 *  %S                        null-terminated wchar string
 *  %C                        wchar
 */

uchar * ngx_cdecl ngx_sprintf(uchar * buf, const char * fmt, ...)
{
	uchar * p;
	va_list args;
	va_start(args, fmt);
	p = ngx_vslprintf(buf, (uchar *)-1, fmt, args);
	va_end(args);
	return p;
}

uchar * ngx_cdecl ngx_snprintf(uchar * buf, size_t max, const char * fmt, ...)
{
	uchar * p;
	va_list args;
	va_start(args, fmt);
	p = ngx_vslprintf(buf, buf + max, fmt, args);
	va_end(args);
	return p;
}

uchar * ngx_cdecl ngx_slprintf(uchar * buf, uchar * last, const char * fmt, ...)
{
	uchar * p;
	va_list args;
	va_start(args, fmt);
	p = ngx_vslprintf(buf, last, fmt, args);
	va_end(args);
	return p;
}

uchar * ngx_vslprintf(uchar * buf, uchar * last, const char * fmt, va_list args)
{
	uchar  * p, zero;
	int d;
	double f;
	size_t len, slen;
	int64_t i64;
	uint64_t ui64, frac;
	ngx_msec_t ms;
	ngx_uint_t width, sign, hex, max_width, frac_width, scale, n;
	ngx_str_t   * v;
	ngx_variable_value_t  * vv;
	while(*fmt && buf < last) {
		// "buf < last" means that we could copy at least one character: the plain character, "%%", "%c", and minus without the checking
		if(*fmt == '%') {
			i64 = 0;
			ui64 = 0;
			zero = (uchar)((*++fmt == '0') ? '0' : ' ');
			width = 0;
			sign = 1;
			hex = 0;
			max_width = 0;
			frac_width = 0;
			slen = (size_t)-1;
			while(*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt++ - '0');
			}
			for(;;) {
				switch(*fmt) {
					case 'u':
					    sign = 0;
					    fmt++;
					    continue;
					case 'm':
					    max_width = 1;
					    fmt++;
					    continue;
					case 'X':
					    hex = 2;
					    sign = 0;
					    fmt++;
					    continue;
					case 'x':
					    hex = 1;
					    sign = 0;
					    fmt++;
					    continue;
					case '.':
					    fmt++;
					    while(*fmt >= '0' && *fmt <= '9') {
						    frac_width = frac_width * 10 + (*fmt++ - '0');
					    }
					    break;
					case '*':
					    slen = va_arg(args, size_t);
					    fmt++;
					    continue;
					default:
					    break;
				}
				break;
			}
			switch(*fmt) {
				case 'V':
				    v = va_arg(args, ngx_str_t *);
				    len = MIN(((size_t)(last - buf)), v->len);
				    buf = ngx_cpymem(buf, v->data, len);
				    fmt++;
				    continue;
				case 'v':
				    vv = va_arg(args, ngx_variable_value_t *);
				    len = MIN(((size_t)(last - buf)), vv->len);
				    buf = ngx_cpymem(buf, vv->data, len);
				    fmt++;
				    continue;
				case 's':
				    p = va_arg(args, uchar *);
				    if(slen == (size_t)-1) {
					    while(*p && buf < last) {
						    *buf++ = *p++;
					    }
				    }
				    else {
					    len = MIN(((size_t)(last - buf)), slen);
					    buf = ngx_cpymem(buf, p, len);
				    }
				    fmt++;
				    continue;
				case 'O':
				    i64 = (int64_t)va_arg(args, nginx_off_t);
				    sign = 1;
				    break;
				case 'P':
				    i64 = (int64_t)va_arg(args, ngx_pid_t);
				    sign = 1;
				    break;
				case 'T':
				    i64 = (int64_t)va_arg(args, time_t);
				    sign = 1;
				    break;
				case 'M':
				    ms = (ngx_msec_t)va_arg(args, ngx_msec_t);
				    if((ngx_msec_int_t)ms == -1) {
					    sign = 1;
					    i64 = -1;
				    }
				    else {
					    sign = 0;
					    ui64 = (uint64_t)ms;
				    }
				    break;
				case 'z':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, ssize_t);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, size_t);
				    }
				    break;
				case 'i':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, ngx_int_t);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, ngx_uint_t);
				    }
				    if(max_width) {
					    width = NGX_INT_T_LEN;
				    }
				    break;
				case 'd':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, int);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, uint);
				    }
				    break;
				case 'l':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, long);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, ulong);
				    }
				    break;
				case 'D':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, int32_t);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, uint32_t);
				    }
				    break;
				case 'L':
				    if(sign) {
					    i64 = va_arg(args, int64_t);
				    }
				    else {
					    ui64 = va_arg(args, uint64_t);
				    }
				    break;
				case 'A':
				    if(sign) {
					    i64 = (int64_t)va_arg(args, ngx_atomic_int_t);
				    }
				    else {
					    ui64 = (uint64_t)va_arg(args, ngx_atomic_uint_t);
				    }
				    if(max_width) {
					    width = NGX_ATOMIC_T_LEN;
				    }
				    break;
				case 'f':
				    f = va_arg(args, double);
				    if(f < 0) {
					    *buf++ = '-';
					    f = -f;
				    }
				    ui64 = (int64_t)f;
				    frac = 0;
				    if(frac_width) {
					    scale = 1;
					    for(n = frac_width; n; n--) {
						    scale *= 10;
					    }
					    frac = (uint64_t)((f - (double)ui64) * scale + 0.5);
					    if(frac == scale) {
						    ui64++;
						    frac = 0;
					    }
				    }
				    buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);
				    if(frac_width) {
					    if(buf < last) {
						    *buf++ = '.';
					    }
					    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width);
				    }
				    fmt++;
				    continue;
#if !(NGX_WIN32)
				case 'r':
				    i64 = (int64_t)va_arg(args, rlim_t);
				    sign = 1;
				    break;
#endif
				case 'p':
				    ui64 = (uintptr_t)va_arg(args, void *);
				    hex = 2;
				    sign = 0;
				    zero = '0';
				    width = 2 * sizeof(void *);
				    break;
				case 'c':
				    d = va_arg(args, int);
				    *buf++ = (uchar)(d & 0xff);
				    fmt++;
				    continue;
				case 'Z':
				    *buf++ = '\0';
				    fmt++;
				    continue;
				case 'N':
#if (NGX_WIN32)
				    *buf++ = __CR;
				    if(buf < last) {
					    *buf++ = LF;
				    }
#else
				    *buf++ = LF;
#endif
				    fmt++;
				    continue;
				case '%':
				    *buf++ = '%';
				    fmt++;
				    continue;
				default:
				    *buf++ = *fmt++;
				    continue;
			}
			if(sign) {
				if(i64 < 0) {
					*buf++ = '-';
					ui64 = (uint64_t)-i64;
				}
				else {
					ui64 = (uint64_t)i64;
				}
			}
			buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);
			fmt++;
		}
		else {
			*buf++ = *fmt++;
		}
	}
	return buf;
}

static uchar * ngx_sprintf_num(uchar * buf, uchar * last, uint64_t ui64, uchar zero, ngx_uint_t hexadecimal, ngx_uint_t width)
{
	uchar * p, temp[NGX_INT64_LEN + 1];
	// we need temp[NGX_INT64_LEN] only, but icc issues the warning
	size_t len;
	uint32_t ui32;
	// @sobolev static uchar hex[] = "0123456789abcdef";
	// @sobolev static uchar HEX[] = "0123456789ABCDEF";
	p = temp + NGX_INT64_LEN;
	if(hexadecimal == 0) {
		if(ui64 <= (uint64_t)NGX_MAX_UINT32_VALUE) {
			/*
			 * To divide 64-bit numbers and to find remainders
			 * on the x86 platform gcc and icc call the libc functions
			 * [u]divdi3() and [u]moddi3(), they call another function
			 * in its turn.  On FreeBSD it is the qdivrem() function,
			 * its source code is about 170 lines of the code.
			 * The glibc counterpart is about 150 lines of the code.
			 *
			 * For 32-bit numbers and some divisors gcc and icc use
			 * a inlined multiplication and shifts.  For example,
			 * unsigned "i32 / 10" is compiled to
			 *
			 *   (i32 * 0xCCCCCCCD) >> 35
			 */
			ui32 = (uint32_t)ui64;
			do {
				*--p = (uchar)(ui32 % 10 + '0');
			} while(ui32 /= 10);
		}
		else {
			do {
				*--p = (uchar)(ui64 % 10 + '0');
			} while(ui64 /= 10);
		}
	}
	else if(hexadecimal == 1) {
		do {
			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = SlConst::P_HxDigL[(uint32_t)(ui64 & 0xf)];
		} while(ui64 >>= 4);
	}
	else { /* hexadecimal == 2 */
		do {
			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = SlConst::P_HxDigU[(uint32_t)(ui64 & 0xf)];
		} while(ui64 >>= 4);
	}
	/* zero or space padding */
	len = (temp + NGX_INT64_LEN) - p;
	while(len++ < width && buf < last) {
		*buf++ = zero;
	}
	/* number safe copy */
	len = (temp + NGX_INT64_LEN) - p;
	if(buf + len > last) {
		len = last - buf;
	}
	return ngx_cpymem(buf, p, len);
}
/* @sobolev replaced with sstreqi_ascii
// 
// We use ngx_strcasecmp()/ngx_strncasecmp() for 7-bit ASCII strings only,
// and implement our own ngx_strcasecmp()/ngx_strncasecmp()
// to avoid libc locale overhead.  Besides, we use the ngx_uint_t's
// instead of the uchar's, because they are slightly faster.
// 
ngx_int_t FASTCALL ngx_strcasecmp(const uchar * s1, const uchar * s2)
{
	ngx_uint_t c1, c2;
	for(;;) {
		c1 = (ngx_uint_t)*s1++;
		c2 = (ngx_uint_t)*s2++;
		c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
		if(c1 == c2) {
			if(c1) {
				continue;
			}
			return 0;
		}
		return c1 - c2;
	}
}*/

ngx_int_t FASTCALL ngx_strncasecmp(const uchar * s1, const uchar * s2, size_t n)
{
	while(n) {
		uchar c1 = *s1++;
		uchar c2 = *s2++;
		c1 = isasciiupr(c1) ? (c1 | 0x20) : c1;
		c2 = isasciiupr(c2) ? (c2 | 0x20) : c2;
		if(c1 == c2) {
			if(c1) {
				n--;
				continue;
			}
			return 0;
		}
		return static_cast<int>(c1) - static_cast<int>(c2);
	}
	return 0;
}

const uchar * FASTCALL ngx_strnstr(const uchar * s1, const char * s2, size_t len)
{
	uchar c1;
	uchar c2 = *(uchar *)s2++;
	size_t n = ngx_strlen(s2);
	do {
		do {
			if(len-- == 0) {
				return NULL;
			}
			c1 = *s1++;
			if(c1 == 0) {
				return NULL;
			}
		} while(c1 != c2);
		if(n > len) {
			return NULL;
		}
	} while(ngx_strncmp(s1, (uchar *)s2, n) != 0);
	return --s1;
}
/*
 * ngx_strstrn() and ngx_strcasestrn() are intended to search for static
 * substring with known length in null-terminated string. The argument n
 * must be length of the second substring - 1.
 */
uchar * FASTCALL ngx_strstrn(uchar * s1, const char * s2, size_t n)
{
	uchar c1;
	uchar c2 = *(const uchar *)s2++;
	do {
		do {
			c1 = *s1++;
			if(c1 == 0) {
				return NULL;
			}
		} while(c1 != c2);
	} while(ngx_strncmp(s1, (uchar *)s2, n) != 0);
	return --s1;
}

const uchar * FASTCALL ngx_strcasestrn(const uchar * s1, const char * s2, size_t n)
{
	uchar c1;
	char c2 = *s2++;
	c2 = isasciiupr(c2) ? (c2 | 0x20) : c2;
	do {
		do {
			c1 = *s1++;
			if(c1 == 0) {
				return NULL;
			}
			c1 = isasciiupr(c1) ? (c1 | 0x20) : c1;
		} while(c1 != c2);
	} while(ngx_strncasecmp(s1, (uchar *)s2, n) != 0);
	return --s1;
}
/*
 * ngx_strlcasestrn() is intended to search for static substring
 * with known length in string until the argument last. The argument n
 * must be length of the second substring - 1.
 */
uchar * FASTCALL ngx_strlcasestrn(uchar * s1, uchar * last, const uchar * s2, size_t n)
{
	uchar c1, c2;
	c2 = *s2++;
	c2 = isasciiupr(c2) ? (c2 | 0x20) : c2;
	last -= n;
	do {
		do {
			if(s1 >= last) {
				return NULL;
			}
			c1 = *s1++;
			c1 = isasciiupr(c1) ? (c1 | 0x20) : c1;
		} while(c1 != c2);
	} while(ngx_strncasecmp(s1, s2, n) != 0);
	return --s1;
}

ngx_int_t ngx_rstrncmp(uchar * s1, uchar * s2, size_t n)
{
	if(n == 0) {
		return 0;
	}
	n--;
	for(;;) {
		if(s1[n] != s2[n]) {
			return s1[n] - s2[n];
		}
		if(n == 0) {
			return 0;
		}
		n--;
	}
}

ngx_int_t ngx_rstrncasecmp(uchar * s1, uchar * s2, size_t n)
{
	uchar c1, c2;
	if(n == 0) {
		return 0;
	}
	n--;
	for(;;) {
		c1 = s1[n];
		if(c1 >= 'a' && c1 <= 'z') {
			c1 -= 'a' - 'A';
		}
		c2 = s2[n];
		if(c2 >= 'a' && c2 <= 'z') {
			c2 -= 'a' - 'A';
		}
		if(c1 != c2) {
			return c1 - c2;
		}
		if(n == 0) {
			return 0;
		}
		n--;
	}
}

ngx_int_t ngx_memn2cmp(uchar * s1, uchar * s2, size_t n1, size_t n2)
{
	size_t n;
	ngx_int_t m, z;
	if(n1 <= n2) {
		n = n1;
		z = -1;
	}
	else {
		n = n2;
		z = 1;
	}
	m = memcmp(s1, s2, n);
	if(m || n1 == n2) {
		return m;
	}
	return z;
}

ngx_int_t FASTCALL ngx_dns_strcmp(const uchar * s1, const uchar * s2)
{
	uchar c1, c2;
	for(;;) {
		c1 = *s1++;
		c2 = *s2++;
		c1 = isasciiupr(c1) ? (c1 | 0x20) : c1;
		c2 = isasciiupr(c2) ? (c2 | 0x20) : c2;
		if(c1 == c2) {
			if(c1) {
				continue;
			}
			return 0;
		}
		/* in ASCII '.' > '-', but we need '.' to be the lowest character */
		c1 = (c1 == '.') ? ' ' : c1;
		c2 = (c2 == '.') ? ' ' : c2;
		return c1 - c2;
	}
}

ngx_int_t FASTCALL ngx_filename_cmp(const uchar * s1, const uchar * s2, size_t n)
{
	ngx_uint_t c1, c2;
	while(n) {
		c1 = (ngx_uint_t)*s1++;
		c2 = (ngx_uint_t)*s2++;
#if (NGX_HAVE_CASELESS_FILESYSTEM)
		c1 = tolower(c1);
		c2 = tolower(c2);
#endif
		if(c1 == c2) {
			if(c1) {
				n--;
				continue;
			}
			return 0;
		}
		/* we need '/' to be the lowest character */
		if(c1 == 0 || c2 == 0) {
			return c1 - c2;
		}
		c1 = (c1 == '/') ? 0 : c1;
		c2 = (c2 == '/') ? 0 : c2;
		return c1 - c2;
	}
	return 0;
}

ngx_int_t FASTCALL ngx_atoi(const uchar * line, size_t n)
{
	ngx_int_t value, cutoff, cutlim;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = NGX_MAX_INT_T_VALUE / 10;
	cutlim = NGX_MAX_INT_T_VALUE % 10;
	for(value = 0; n--; line++) {
		if(!isdec(*line)) {
			return NGX_ERROR;
		}
		if(value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return NGX_ERROR;
		}
		value = value * 10 + (*line - '0');
	}
	return value;
}
//
// parse a fixed point number, e.g., ngx_atofp("10.5", 4, 2) returns 1050 
//
ngx_int_t FASTCALL ngx_atofp(const uchar * line, size_t n, size_t point)
{
	ngx_int_t value, cutoff, cutlim;
	ngx_uint_t dot;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = NGX_MAX_INT_T_VALUE / 10;
	cutlim = NGX_MAX_INT_T_VALUE % 10;
	dot = 0;
	for(value = 0; n--; line++) {
		if(point == 0) {
			return NGX_ERROR;
		}
		if(*line == '.') {
			if(dot) {
				return NGX_ERROR;
			}
			dot = 1;
			continue;
		}
		if(!isdec(*line)) {
			return NGX_ERROR;
		}
		if(value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return NGX_ERROR;
		}
		value = value * 10 + (*line - '0');
		point -= dot;
	}
	while(point--) {
		if(value > cutoff) {
			return NGX_ERROR;
		}
		value = value * 10;
	}
	return value;
}

ssize_t FASTCALL ngx_atosz(const uchar * line, size_t n)
{
	ssize_t value, cutoff, cutlim;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = NGX_MAX_SIZE_T_VALUE / 10;
	cutlim = NGX_MAX_SIZE_T_VALUE % 10;
	for(value = 0; n--; line++) {
		if(!isdec(*line)) {
			return NGX_ERROR;
		}
		if(value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return NGX_ERROR;
		}
		value = value * 10 + (*line - '0');
	}
	return value;
}

nginx_off_t FASTCALL ngx_atoof(const uchar * line, size_t n)
{
	nginx_off_t value, cutoff, cutlim;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = NGX_MAX_OFF_T_VALUE / 10;
	cutlim = NGX_MAX_OFF_T_VALUE % 10;
	for(value = 0; n--; line++) {
		if(!isdec(*line)) {
			return NGX_ERROR;
		}
		if(value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return NGX_ERROR;
		}
		value = value * 10 + (*line - '0');
	}
	return value;
}

time_t FASTCALL ngx_atotm(const uchar * line, size_t n)
{
	time_t value, cutoff, cutlim;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = (time_t)(NGX_MAX_TIME_T_VALUE / 10);
	cutlim = NGX_MAX_TIME_T_VALUE % 10;
	for(value = 0; n--; line++) {
		if(!isdec(*line)) {
			return NGX_ERROR;
		}
		if(value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return NGX_ERROR;
		}
		value = value * 10 + (*line - '0');
	}
	return value;
}

ngx_int_t FASTCALL ngx_hextoi(const uchar * line, size_t n)
{
	uchar c, ch;
	ngx_int_t value, cutoff;
	if(n == 0) {
		return NGX_ERROR;
	}
	cutoff = NGX_MAX_INT_T_VALUE / 16;
	for(value = 0; n--; line++) {
		if(value > cutoff) {
			return NGX_ERROR;
		}
		else {
			ch = *line;
			// @v11.9.6 {
			if(ishex(ch)) {
				value = (value << 4) + hex(ch);
			}
			else {
				return NGX_ERROR;
			}
			// } @v11.9.6
			/* @v11.9.6
			if(isdec(ch)) {
				value = value * 16 + (ch - '0');
				continue;
			}
			c = (uchar)(ch | 0x20);
			if(c >= 'a' && c <= 'f') {
				value = value * 16 + (c - 'a' + 10);
				continue;
			}
			return NGX_ERROR;
			*/
		}
	}
	return value;
}

uchar * ngx_hex_dump(uchar * dst, const uchar * src, size_t len)
{
	// @sobolev static uchar hex[] = "0123456789abcdef";
	while(len--) {
		*dst++ = SlConst::P_HxDigL[*src >> 4];
		*dst++ = SlConst::P_HxDigL[*src++ & 0xf];
	}
	return dst;
}

void FASTCALL ngx_encode_base64(ngx_str_t * dst, const ngx_str_t * src)
{
	//static uchar basis64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const uchar * p_basis = (const uchar *)STextConst::Get(STextConst::cBasis64, 0);
	ngx_encode_base64_internal(dst, src, p_basis, 1);
}

void FASTCALL ngx_encode_base64url(ngx_str_t * dst, const ngx_str_t * src)
{
	//static uchar basis64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	const uchar * p_basis = (const uchar *)STextConst::Get(STextConst::cBasis64Url, 0);
	ngx_encode_base64_internal(dst, src, p_basis, 0);
}

static void ngx_encode_base64_internal(ngx_str_t * dst, const ngx_str_t * src, const uchar * basis, ngx_uint_t padding)
{
	size_t len = src->len;
	const uchar * s = src->data;
	uchar * d = dst->data;
	while(len > 2) {
		*d++ = basis[(s[0] >> 2) & 0x3f];
		*d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
		*d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
		*d++ = basis[s[2] & 0x3f];
		s += 3;
		len -= 3;
	}
	if(len) {
		*d++ = basis[(s[0] >> 2) & 0x3f];
		if(len == 1) {
			*d++ = basis[(s[0] & 3) << 4];
			if(padding) {
				*d++ = '=';
			}
		}
		else {
			*d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
			*d++ = basis[(s[1] & 0x0f) << 2];
		}
		if(padding) {
			*d++ = '=';
		}
	}
	dst->len = d - dst->data;
}

ngx_int_t FASTCALL ngx_decode_base64(ngx_str_t * dst, const ngx_str_t * src)
{
	static uchar basis64[] = {
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
		77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
		77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
	};
	return ngx_decode_base64_internal(dst, src, basis64);
}

ngx_int_t FASTCALL ngx_decode_base64url(ngx_str_t * dst, const ngx_str_t * src)
{
	static uchar basis64[] = {
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
		77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 63,
		77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
		77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
	};
	return ngx_decode_base64_internal(dst, src, basis64);
}

static ngx_int_t ngx_decode_base64_internal(ngx_str_t * dst, const ngx_str_t * src, const uchar * basis)
{
	size_t len;
	uchar  * d, * s;
	for(len = 0; len < src->len; len++) {
		if(src->data[len] == '=') {
			break;
		}
		if(basis[src->data[len]] == 77) {
			return NGX_ERROR;
		}
	}
	if(len % 4 == 1) {
		return NGX_ERROR;
	}
	s = src->data;
	d = dst->data;
	while(len > 3) {
		*d++ = (uchar)(basis[s[0]] << 2 | basis[s[1]] >> 4);
		*d++ = (uchar)(basis[s[1]] << 4 | basis[s[2]] >> 2);
		*d++ = (uchar)(basis[s[2]] << 6 | basis[s[3]]);

		s += 4;
		len -= 4;
	}
	if(len > 1) {
		*d++ = (uchar)(basis[s[0]] << 2 | basis[s[1]] >> 4);
	}
	if(len > 2) {
		*d++ = (uchar)(basis[s[1]] << 4 | basis[s[2]] >> 2);
	}
	dst->len = d - dst->data;
	return NGX_OK;
}
/*
 * ngx_utf8_decode() decodes two and more bytes UTF sequences only
 * the return values:
 *  0x80 - 0x10ffff         valid character
 *  0x110000 - 0xfffffffd   invalid sequence
 *  0xfffffffe              incomplete sequence
 *  0xffffffff              error
 */
uint32_t FASTCALL ngx_utf8_decode(const uchar ** p, size_t n)
{
	size_t len;
	uint32_t i, valid;
	uint32_t u = **p;
	if(u >= 0xf0) {
		u &= 0x07;
		valid = 0xffff;
		len = 3;
	}
	else if(u >= 0xe0) {
		u &= 0x0f;
		valid = 0x7ff;
		len = 2;
	}
	else if(u >= 0xc2) {
		u &= 0x1f;
		valid = 0x7f;
		len = 1;
	}
	else {
		(*p)++;
		return 0xffffffff;
	}
	if(n - 1 < len) {
		return 0xfffffffe;
	}
	(*p)++;
	while(len) {
		i = *(*p)++;
		if(i < 0x80) {
			return 0xffffffff;
		}
		u = (u << 6) | (i & 0x3f);
		len--;
	}
	if(u > valid) {
		return u;
	}
	return 0xffffffff;
}

size_t FASTCALL ngx_utf8_length(const uchar * p, size_t n)
{
	size_t len;
	const uchar * last = p + n;
	for(len = 0; p < last; len++) {
		uchar c = *p;
		if(c < 0x80) {
			p++;
		}
		else if(ngx_utf8_decode(&p, n) > 0x10ffff) {
			// invalid UTF-8 
			return n;
		}
	}
	return len;
}

uchar * ngx_utf8_cpystrn(uchar * dst, const uchar * src, size_t n, size_t len)
{
	if(n) {
		while(--n) {
			uchar c = *src;
			*dst = c;
			if(c < 0x80) {
				if(c != '\0') {
					dst++;
					src++;
					len--;
				}
				else
					return dst;
			}
			else {
				const uchar * next = src;
				if(ngx_utf8_decode(&next, len) > 0x10ffff) {
					// invalid UTF-8 
					break;
				}
				while(src < next) {
					*dst++ = *src++;
					len--;
				}
			}
		}
		*dst = '\0';
	}
	return dst;
}

uintptr_t FASTCALL ngx_escape_uri(uchar * dst, const uchar * src, size_t size, ngx_uint_t type)
{
	ngx_uint_t n;
	uint32_t  * escape;
	// @sobolev static uchar hex[] = "0123456789ABCDEF";
	/* " ", "#", "%", "?", %00-%1F, %7F-%FF */
	static uint32_t uri[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x80000029, /* 1000 0000 0000 0000  0000 0000 0010 1001 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff /* 1111 1111 1111 1111  1111 1111 1111 1111 */
	};

	/* " ", "#", "%", "&", "+", "?", %00-%1F, %7F-%FF */

	static uint32_t args[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x88000869, /* 1000 1000 0000 0000  0000 1000 0110 1001 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff /* 1111 1111 1111 1111  1111 1111 1111 1111 */
	};
	/* not ALPHA, DIGIT, "-", ".", "_", "~" */
	static uint32_t uri_component[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0xfc009fff, /* 1111 1100 0000 0000  1001 1111 1111 1111 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff /* 1111 1111 1111 1111  1111 1111 1111 1111 */
	};
	/* " ", "#", """, "%", "'", %00-%1F, %7F-%FF */
	static uint32_t html[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x000000ad, /* 0000 0000 0000 0000  0000 0000 1010 1101 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff /* 1111 1111 1111 1111  1111 1111 1111 1111 */
	};
	/* " ", """, "%", "'", %00-%1F, %7F-%FF */
	static uint32_t refresh[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00000085, /* 0000 0000 0000 0000  0000 0000 1000 0101 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		0xffffffff /* 1111 1111 1111 1111  1111 1111 1111 1111 */
	};
	/* " ", "%", %00-%1F */
	static uint32_t memcached[] = {
		0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
		/* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
		0x00000021, /* 0000 0000 0000 0000  0000 0000 0010 0001 */
		/* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		/* ~}| {zyx wvut srqp  onml kjih gfed cba` */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
		0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
	};
	/* mail_auth is the same as memcached */
	static uint32_t  * map[] = { uri, args, uri_component, html, refresh, memcached, memcached };
	escape = map[type];
	if(dst == NULL) {
		/* find the number of the characters to be escaped */
		n = 0;
		while(size) {
			if(escape[*src >> 5] & (1U << (*src & 0x1f))) {
				n++;
			}
			src++;
			size--;
		}
		return (uintptr_t)n;
	}
	while(size) {
		if(escape[*src >> 5] & (1U << (*src & 0x1f))) {
			*dst++ = '%';
			*dst++ = SlConst::P_HxDigU[*src >> 4];
			*dst++ = SlConst::P_HxDigU[*src & 0xf];
			src++;
		}
		else {
			*dst++ = *src++;
		}
		size--;
	}
	return (uintptr_t)dst;
}

void FASTCALL ngx_unescape_uri(uchar ** dst, uchar ** src, size_t size, ngx_uint_t type)
{
	uchar c;
	enum {
		sw_usual = 0,
		sw_quoted,
		sw_quoted_second
	} state;
	uchar * d = *dst;
	uchar * s = *src;
	uchar decoded = 0;
	state = sw_usual;
	while(size--) {
		uchar ch = *s++;
		switch(state) {
			case sw_usual:
			    if(ch == '?' && (type & (NGX_UNESCAPE_URI|NGX_UNESCAPE_REDIRECT))) {
				    *d++ = ch;
				    goto done;
			    }
			    if(ch == '%') {
				    state = sw_quoted;
				    break;
			    }
			    *d++ = ch;
			    break;
			case sw_quoted:
			    if(isdec(ch)) {
				    decoded = (uchar)(ch - '0');
				    state = sw_quoted_second;
				    break;
			    }
			    c = (uchar)(ch | 0x20);
			    if(c >= 'a' && c <= 'f') {
				    decoded = (uchar)(c - 'a' + 10);
				    state = sw_quoted_second;
				    break;
			    }
			    /* the invalid quoted character */
			    state = sw_usual;
			    *d++ = ch;
			    break;
			case sw_quoted_second:
			    state = sw_usual;
			    if(isdec(ch)) {
				    ch = (uchar)((decoded << 4) + (ch - '0'));
				    if(type & NGX_UNESCAPE_REDIRECT) {
					    if(ch > '%' && ch < 0x7f) {
						    *d++ = ch;
						    break;
					    }
					    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
					    break;
				    }
				    *d++ = ch;
				    break;
			    }
			    c = (uchar)(ch | 0x20);
			    if(c >= 'a' && c <= 'f') {
				    ch = (uchar)((decoded << 4) + (c - 'a') + 10);
				    if(type & NGX_UNESCAPE_URI) {
					    if(ch == '?') {
						    *d++ = ch;
						    goto done;
					    }
					    *d++ = ch;
					    break;
				    }
				    if(type & NGX_UNESCAPE_REDIRECT) {
					    if(ch == '?') {
						    *d++ = ch;
						    goto done;
					    }
					    if(ch > '%' && ch < 0x7f) {
						    *d++ = ch;
						    break;
					    }
					    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
					    break;
				    }
				    *d++ = ch;
				    break;
			    }
			    /* the invalid quoted character */
			    break;
		}
	}
done:
	*dst = d;
	*src = s;
}

uintptr_t FASTCALL ngx_escape_html(uchar * dst, uchar * src, size_t size)
{
	if(dst == NULL) {
		ngx_uint_t len = 0;
		while(size) {
			switch(*src++) {
				case '<': len += sizeof("&lt;") - 2; break;
				case '>': len += sizeof("&gt;") - 2; break;
				case '&': len += sizeof("&amp;") - 2; break;
				case '"': len += sizeof("&quot;") - 2; break;
				default: break;
			}
			size--;
		}
		return (uintptr_t)len;
	}
	else {
		while(size) {
			uchar ch = *src++;
			switch(ch) {
				case '<': *dst++ = '&'; *dst++ = 'l'; *dst++ = 't'; *dst++ = ';'; break;
				case '>': *dst++ = '&'; *dst++ = 'g'; *dst++ = 't'; *dst++ = ';'; break;
				case '&': *dst++ = '&'; *dst++ = 'a'; *dst++ = 'm'; *dst++ = 'p'; *dst++ = ';'; break;
				case '"': *dst++ = '&'; *dst++ = 'q'; *dst++ = 'u'; *dst++ = 'o'; *dst++ = 't'; *dst++ = ';'; break;
				default: *dst++ = ch; break;
			}
			size--;
		}
		return (uintptr_t)dst;
	}
}

uintptr_t FASTCALL ngx_escape_json(uchar * dst, uchar * src, size_t size)
{
	uchar ch;
	if(dst == NULL) {
		ngx_uint_t len = 0;
		while(size) {
			ch = *src++;
			if(oneof2(ch, '\\', '"')) {
				len++;
			}
			else if(ch <= 0x1f) {
				switch(ch) {
					case '\n': case '\r': case '\t': case '\b': case '\f':
					    len++;
					    break;
					default:
					    len += sizeof("\\u001F") - 2;
				}
			}
			size--;
		}
		return (uintptr_t)len;
	}
	else {
		while(size) {
			ch = *src++;
			if(ch > 0x1f) {
				if(oneof2(ch, '\\', '"')) {
					*dst++ = '\\';
				}
				*dst++ = ch;
			}
			else {
				*dst++ = '\\';
				switch(ch) {
					case '\n': *dst++ = 'n'; break;
					case '\r': *dst++ = 'r'; break;
					case '\t': *dst++ = 't'; break;
					case '\b': *dst++ = 'b'; break;
					case '\f': *dst++ = 'f'; break;
					default:
						*dst++ = 'u'; 
						*dst++ = '0'; 
						*dst++ = '0';
						*dst++ = '0' + (ch >> 4);
						ch &= 0xf;
						*dst++ = (ch < 10) ? ('0' + ch) : ('A' + ch - 10);
				}
			}
			size--;
		}
		return (uintptr_t)dst;
	}
}

void ngx_str_rbtree_insert_value(ngx_rbtree_node_t * temp, ngx_rbtree_node_t * node, ngx_rbtree_node_t * sentinel)
{
	ngx_rbtree_node_t  ** p;
	for(;;) {
		ngx_str_node_t * n = (ngx_str_node_t*)node;
		ngx_str_node_t * t = (ngx_str_node_t*)temp;
		if(node->key != temp->key) {
			p = (node->key < temp->key) ? &temp->left : &temp->right;
		}
		else if(n->str.len != t->str.len) {
			p = (n->str.len < t->str.len) ? &temp->left : &temp->right;
		}
		else {
			p = (memcmp(n->str.data, t->str.data, n->str.len) < 0) ? &temp->left : &temp->right;
		}
		if(*p == sentinel) {
			break;
		}
		temp = *p;
	}
	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	ngx_rbt_red(node);
}

ngx_str_node_t * ngx_str_rbtree_lookup(ngx_rbtree_t * rbtree, ngx_str_t * val, uint32_t hash)
{
	ngx_int_t rc;
	ngx_str_node_t * n;
	ngx_rbtree_node_t  * node = rbtree->root;
	ngx_rbtree_node_t  * sentinel = rbtree->sentinel;
	while(node != sentinel) {
		n = (ngx_str_node_t*)node;
		if(hash != node->key) {
			node = (hash < node->key) ? node->left : node->right;
			continue;
		}
		if(val->len != n->str.len) {
			node = (val->len < n->str.len) ? node->left : node->right;
			continue;
		}
		rc = memcmp(val->data, n->str.data, val->len);
		if(rc < 0) {
			node = node->left;
			continue;
		}
		if(rc > 0) {
			node = node->right;
			continue;
		}
		return n;
	}
	return NULL;
}
//
// ngx_sort() is implemented as insertion sort because we need stable sort 
//
void ngx_sort(void * base, size_t n, size_t size, ngx_int_t (* cmp)(const void *, const void *))
{
	uchar * p = (uchar *)ngx_alloc(size, ngx_cycle->log);
	if(p) {
		for(uchar * p1 = (uchar *)base + size; p1 < (uchar *)base + n * size; p1 += size) {
			uchar * p2;
			memcpy(p, p1, size);
			for(p2 = p1; p2 > (uchar *)base && cmp(p2 - size, p) > 0; p2 -= size) {
				memcpy(p2, p2 - size, size);
			}
			memcpy(p2, p, size);
		}
		SAlloc::F(p);
	}
}

#if (NGX_MEMCPY_LIMIT)
void * ngx_memcpy_Removed(void * dst, const void * src, size_t n)
{
	if(n > NGX_MEMCPY_LIMIT) {
		ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "memcpy %uz bytes", n);
		ngx_debug_point();
	}
	return memcpy(dst, src, n);
}
#endif
