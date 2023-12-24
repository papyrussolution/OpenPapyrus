/*
 * Copyright 2016 Nu-book Inc.
 */
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop
#include "libzueci/zueci.h"

namespace ZXing {
	void TextEncoder::GetBytes(const std::string& str, CharacterSet charset, std::string& bytes)
	{
		int eci = ToInt(ToECI(charset));
		const int str_len = narrow_cast<int>(str.length());
		int eci_len;
		if(eci == -1)
			eci = 899; // Binary
		bytes.clear();
		int error_number = zueci_dest_len_eci(eci, reinterpret_cast<const uchar *>(str.data()), str_len, &eci_len);
		if(error_number >= ZUECI_ERROR)  // Shouldn't happen
			throw std::logic_error("Internal error `zueci_dest_len_eci()`");
		bytes.resize(eci_len); // Sufficient but approximate length
		error_number = zueci_utf8_to_eci(eci, reinterpret_cast<const uchar *>(str.data()), str_len,
			reinterpret_cast<uchar *>(bytes.data()), &eci_len);
		if(error_number >= ZUECI_ERROR) {
			bytes.clear();
			throw std::invalid_argument("Unexpected charcode");
		}
		bytes.resize(eci_len); // Actual length
	}
	void TextEncoder::GetBytes(const std::wstring& str, CharacterSet charset, std::string& bytes)
	{
		GetBytes(ToUtf8(str), charset, bytes);
	}
	/*static*/std::string TextEncoder::FromUnicode(const std::string& str, CharacterSet charset)
	{
		std::string r;
		GetBytes(str, charset, r);
		return r;
	}
	/*static*/std::string TextEncoder::FromUnicode(const std::wstring& str, CharacterSet charset)
	{
		std::string r;
		GetBytes(str, charset, r);
		return r;
	}
} // ZXing
