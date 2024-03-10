// SSTRTRANSFORM.CPP
// Copyright (c) A.Sobolev 2024
//
#include <slib-internal.h>
#pragma hdrstop

static const char fi_digits[] =
	"0001020304050607080910111213141516171819"
	"2021222324252627282930313233343536373839"
	"4041424344454647484950515253545556575859"
	"6061626364656667686970717273747576777879"
	"8081828384858687888990919293949596979899";
//
// Integer division is slow so do it for a group of two digits instead
// of for every digit. The idea comes from the talk by Alexandrescu
// "Three Optimization Tips for C++". See speed-test for a comparison.
//
char * format_decimal64(uint64 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value >= 100) {
		uint index = static_cast<uint>((value % 100) * 2);
		value /= 100;
		*--ptr = fi_digits[index+1];
		*--ptr = fi_digits[index];
	}
	if(value < 10) {
		*--ptr = static_cast<char>('0' + value);
	}
	else {
		uint index = static_cast<uint>(value * 2);
		*--ptr = fi_digits[index+1];
		*--ptr = fi_digits[index];
	}
	assert(ptr >= pBuf);
	return ptr;
}

static char * format_hex64(uint64 value, bool upperCase, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	const char * p_hxdig = upperCase ? SlConst::P_HxDigU : SlConst::P_HxDigL;
	while(value) {
		assert(ptr > pBuf);
		*--ptr = p_hxdig[static_cast<uint>(value & 0xf)];
		value = value >> 4;
	}
	return ptr;
}

char * format_oct64(uint64 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value) {
		assert(ptr > pBuf);
		*--ptr = SlConst::P_HxDigL[static_cast<uint>(value & 0x7)];
		value = value >> 3;
	}
	return ptr;
}

char * format_bin64(uint64 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value) {
		assert(ptr > pBuf);
		*--ptr = SlConst::P_HxDigL[static_cast<uint>(value & 0x1)];
		value = value >> 1;
	}
	return ptr;
}

char * format_decimal32(uint32 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value >= 100) {
		uint index = static_cast<uint>((value % 100) * 2);
		value /= 100;
		*--ptr = fi_digits[index+1];
		*--ptr = fi_digits[index];
	}
	if(value < 10) {
		*--ptr = static_cast<char>('0' + value);
	}
	else {
		uint index = static_cast<uint>(value * 2);
		*--ptr = fi_digits[index+1];
		*--ptr = fi_digits[index];
	}
	assert(ptr >= pBuf);
	return ptr;
}

char * format_hex32(uint32 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value) {
		assert(ptr > pBuf);
		*--ptr = SlConst::P_HxDigL[static_cast<uint>(value & 0xf)];
		value = value >> 4;
	}
	return ptr;
}

char * format_oct32(uint32 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value) {
		assert(ptr > pBuf);
		*--ptr = SlConst::P_HxDigL[static_cast<uint>(value & 0x7)];
		value = value >> 3;
	}
	return ptr;
}

char * format_bin32(uint32 value, char * pBuf, size_t bufSize) 
{
	char * ptr = pBuf + (bufSize-1); // Parens to workaround MSVC bug.
	while(value) {
		assert(ptr > pBuf);
		*--ptr = SlConst::P_HxDigL[static_cast<uint>(value & 0x1)];
		value = value >> 1;
	}
	return ptr;
}

char * format_signed64(int64 value, char * pBuf, size_t bufSize) 
{
	const bool is_negative = value < 0;
	char * p_result = format_decimal64(is_negative ? (0ULL - static_cast<uint64>(value)) : static_cast<uint64>(value), pBuf, bufSize);
	if(is_negative) 
		*--p_result = '-';
	return p_result;
}

char * format_signed32(int32 value, char * pBuf, size_t bufSize) 
{
	const bool is_negative = value < 0;
	char * p_result = format_decimal32(is_negative ? (0U - static_cast<uint32>(value)) : static_cast<uint32>(value), pBuf, bufSize);
	if(is_negative) 
		*--p_result = '-';
	return p_result;
}
//
// Descr: Попытка сформулировать хорошие спецификации для функций преобразования типизированных
//   бинарных данных (int, double, etc) в текст и обратно.
//   Идея заключается в том, чтобы этот класс был бы базовым механизмом преобразования данные<->текст,
//   все остальные функции пректа должны бы обращаться к этим методам за реализацией.
// 
class SStrTransform { // @v11.9.2 @construction
public:
	static bool ToText(int v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(uint v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(int64 v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(uint64 v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(double v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(float v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(SColorBase v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(const SUniTime_Internal & rV, long fmt, char * pBuf, size_t bufLen);
	static bool ToText(const S_GUID & rV, long fmt, char * pBuf, size_t bufLen);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, int * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, uint * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, int64 * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, uint64 * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, double * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, float * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, SColorBase * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, SUniTime_Internal * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, S_GUID * pV);
};

static FORCEINLINE bool SStrTransform_IntToText_Epilog(const char * pInternalBufStart, const char * pInternalBufDataStart, char * pBuf, size_t bufSize)
{
	const ssize_t len = (pInternalBufDataStart - pInternalBufStart);
	assert(len >= 0);
	if(len < static_cast<ssize_t>(bufSize)) { // Так как нам надо вместить еще терминальный ноль, то '<', а не '<='
		if(pBuf) { // Нулевой указатель на буфер, переданный с аргументом pBuf не является ошибкой
			if(len)
				memcpy(pBuf, pInternalBufDataStart, len);
			pBuf[len] = 0;
		}
		return true;
	}
	else
		return false;
}

/*static*/bool SStrTransform::ToText(int v, long fmt, char * pBuf, size_t bufSize)
{
	bool   ok = true;
	char   internal_buf[128];
	const  uint ff = SFMTFLAG(fmt);
	char * p = internal_buf + (sizeof(internal_buf) - 1);
	if(v == 0) {
		if(!(ff & INTF_NOZERO)) {
			*p-- = '0';
		}
	}
	else {
		const bool is_negative = v < 0;
		const uint av = is_negative ? (0U - static_cast<uint>(v)) : static_cast<uint>(v);
		switch(ff & INTF_BASEMASK) {
			case INTF_OCT: p = format_oct32(av, internal_buf, sizeof(internal_buf)); break;
			case INTF_HEX: p = format_hex32(av, internal_buf, sizeof(internal_buf)); break;
			case INTF_BIN: p = format_bin32(av, internal_buf, sizeof(internal_buf)); break;
			default: p = format_decimal32(av, internal_buf, sizeof(internal_buf)); break; // INTF_DECIMAL
		}
		if(is_negative) {
			*--p = '-';
		}
		else if(ff & INTF_FORCEPOS) {
			*--p = '+';
		}
	}
	return SStrTransform_IntToText_Epilog(internal_buf, p, pBuf, bufSize);
}

/*static*/bool SStrTransform::ToText(int64 v, long fmt, char * pBuf, size_t bufSize)
{
	bool   ok = true;
	char   internal_buf[128];
	const  uint ff = SFMTFLAG(fmt);
	char * p = internal_buf + (sizeof(internal_buf) - 1);
	if(v == 0) {
		if(!(ff & INTF_NOZERO)) {
			*p-- = '0';
		}
	}
	else {
		const bool is_negative = v < 0;
		const uint64 av = is_negative ? (0ULL - static_cast<uint64>(v)) : static_cast<uint64>(v);
		switch(ff & INTF_BASEMASK) {
			case INTF_OCT: p = format_oct64(av, internal_buf, sizeof(internal_buf)); break;
			case INTF_HEX: p = format_hex64(av, LOGIC(ff & INTF_UPPERCASE), internal_buf, sizeof(internal_buf)); break;
			case INTF_BIN: p = format_bin64(av, internal_buf, sizeof(internal_buf)); break;
			default: p = format_decimal64(av, internal_buf, sizeof(internal_buf)); break; // INTF_DECIMAL
		}
		if(is_negative) {
			*--p = '-';
		}
		else if(ff & INTF_FORCEPOS) {
			*--p = '+';
		}
	}
	return SStrTransform_IntToText_Epilog(internal_buf, p, pBuf, bufSize);
}

/*static*/bool SStrTransform::ToText(uint v, long fmt, char * pBuf, size_t bufSize)
{
	bool   ok = true;
	char   internal_buf[128];
	const  uint ff = SFMTFLAG(fmt);
	char * p = internal_buf + (sizeof(internal_buf) - 1);
	if(v == 0) {
		if(!(ff & INTF_NOZERO)) {
			*p-- = '0';
		}
	}
	else {
		switch(ff & INTF_BASEMASK) {
			case INTF_OCT: p = format_oct32(v, internal_buf, sizeof(internal_buf)); break;
			case INTF_HEX: p = format_hex32(v, internal_buf, sizeof(internal_buf)); break;
			case INTF_BIN: p = format_bin32(v, internal_buf, sizeof(internal_buf)); break;
			default: p = format_decimal32(v, internal_buf, sizeof(internal_buf)); break; // INTF_DECIMAL
		}
		if(ff & INTF_FORCEPOS) {
			*--p = '+';
		}
	}
	return SStrTransform_IntToText_Epilog(internal_buf, p, pBuf, bufSize);
}

/*static*/bool SStrTransform::ToText(uint64 v, long fmt, char * pBuf, size_t bufSize)
{
	bool   ok = true;
	char   internal_buf[128];
	const  uint ff = SFMTFLAG(fmt);
	char * p = internal_buf + (sizeof(internal_buf) - 1);
	if(v == 0) {
		if(!(ff & INTF_NOZERO)) {
			*p-- = '0';
		}
	}
	else {
		switch(ff & INTF_BASEMASK) {
			case INTF_OCT: p = format_oct64(v, internal_buf, sizeof(internal_buf)); break;
			case INTF_HEX: p = format_hex64(v, LOGIC(ff & INTF_UPPERCASE), internal_buf, sizeof(internal_buf)); break;
			case INTF_BIN: p = format_bin64(v, internal_buf, sizeof(internal_buf)); break;
			default: p = format_decimal64(v, internal_buf, sizeof(internal_buf)); break; // INTF_DECIMAL
		}
		if(ff & INTF_FORCEPOS) {
			*--p = '+';
		}
	}
	return SStrTransform_IntToText_Epilog(internal_buf, p, pBuf, bufSize);
}