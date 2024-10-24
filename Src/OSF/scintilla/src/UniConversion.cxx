// Scintilla source code edit control
/** @file UniConversion.cxx
** Functions to handle UTF-8 and UTF-16 strings.
**/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

int FASTCALL UnicodeFromUTF8(const uchar * us)
{
	if(us[0] < 0xC2)
		return us[0];
	else if(us[0] < 0xE0)
		return ((us[0] & 0x1F) << 6) + (us[1] & 0x3F);
	else if(us[0] < 0xF0) 
		return ((us[0] & 0xF) << 12) + ((us[1] & 0x3F) << 6) + (us[2] & 0x3F);
	else if(us[0] < 0xF5)
		return ((us[0] & 0x7) << 18) + ((us[1] & 0x3F) << 12) + ((us[2] & 0x3F) << 6) + (us[3] & 0x3F);
	else
		return us[0];
}

bool FASTCALL UTF8IsTrailByte(int ch) 
{
	return (ch >= 0x80) && (ch < 0xc0);
}

bool FASTCALL UTF8IsAscii(int ch) 
{
	return ch < 0x80;
}

bool FASTCALL UTF8IsSeparator(const uchar *us) 
{
	return (us[0] == 0xe2) && (us[1] == 0x80) && ((us[2] == 0xa8) || (us[2] == 0xa9));
}

bool FASTCALL UTF8IsNEL(const uchar *us) 
{
	return (us[0] == 0xc2) && (us[1] == 0x85);
}

uint FASTCALL UTF16CharLength(wchar_t uch) 
{
	return ((uch >= SURROGATE_LEAD_FIRST) && (uch <= SURROGATE_LEAD_LAST)) ? 2 : 1;
}

uint UTF8Length(const wchar_t * uptr, uint tlen)
{
	uint len = 0;
	for(uint i = 0; i < tlen && uptr[i];) {
		uint uch = uptr[i];
		if(uch < 0x80)
			len++;
		else if(uch < 0x800)
			len += 2;
		else if((uch >= SURROGATE_LEAD_FIRST) && (uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		}
		else
			len += 3;
		i++;
	}
	return len;
}

void UTF8FromUTF16(const wchar_t * uptr, uint tlen, char * putf, uint len)
{
	uint k = 0;
	for(uint i = 0; i < tlen && uptr[i];) {
		uint uch = uptr[i];
		if(uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		}
		else if(uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		else if((uch >= SURROGATE_LEAD_FIRST) && (uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			uint xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		}
		else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
	if(k < len)
		putf[k] = '\0';
}

uint UTF8CharLength(uchar ch)
{
	if(ch < 0x80)
		return 1;
	else if(ch < 0x80 + 0x40 + 0x20)
		return 2;
	else if(ch < 0x80 + 0x40 + 0x20 + 0x10)
		return 3;
	else
		return 4;
}

size_t UTF16Length(const char * s, size_t len)
{
	size_t ulen = 0;
	size_t charLen;
	for(size_t i = 0; i < len;) {
		uchar ch = static_cast<uchar>(s[i]);
		if(ch < 0x80)
			charLen = 1;
		else if(ch < (0x80 + 0x40 + 0x20))
			charLen = 2;
		else if(ch < (0x80 + 0x40 + 0x20 + 0x10))
			charLen = 3;
		else {
			charLen = 4;
			ulen++;
		}
		i += charLen;
		ulen++;
	}
	return ulen;
}

size_t UTF16FromUTF8(const char * s, size_t len, wchar_t * tbuf, size_t tlen)
{
	size_t ui = 0;
	const uchar * us = reinterpret_cast<const uchar *>(s);
	size_t i = 0;
	while((i < len) && (ui<tlen)) {
		uchar ch = us[i++];
		if(ch < 0x80) {
			tbuf[ui] = ch;
		}
		else if(ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		}
		else if(ch < 0x80 + 0x40 + 0x20 + 0x10) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		}
		else {
			// Outside the BMP so need two surrogates
			int val = (ch & 0x7) << 18;
			ch = us[i++];
			val += (ch & 0x3F) << 12;
			ch = us[i++];
			val += (ch & 0x3F) << 6;
			ch = us[i++];
			val += (ch & 0x3F);
			tbuf[ui] = static_cast<wchar_t>(((val - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
			ui++;
			tbuf[ui] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		}
		ui++;
	}
	return ui;
}

uint UTF32FromUTF8(const char * s, uint len, uint * tbuf, uint tlen)
{
	uint ui = 0;
	const uchar * us = reinterpret_cast<const uchar *>(s);
	uint i = 0;
	while((i < len) && (ui<tlen)) {
		uchar ch = us[i++];
		uint value = 0;
		if(ch < 0x80) {
			value = ch;
		}
		else if(((len-i) >= 1) && (ch < 0x80 + 0x40 + 0x20)) {
			value = (ch & 0x1F) << 6;
			ch = us[i++];
			value += ch & 0x7F;
		}
		else if(((len-i) >= 2) && (ch < 0x80 + 0x40 + 0x20 + 0x10)) {
			value = (ch & 0xF) << 12;
			ch = us[i++];
			value += (ch & 0x7F) << 6;
			ch = us[i++];
			value += ch & 0x7F;
		}
		else if((len-i) >= 3) {
			value = (ch & 0x7) << 18;
			ch = us[i++];
			value += (ch & 0x3F) << 12;
			ch = us[i++];
			value += (ch & 0x3F) << 6;
			ch = us[i++];
			value += ch & 0x3F;
		}
		tbuf[ui] = value;
		ui++;
	}
	return ui;
}

uint UTF16FromUTF32Character(uint val, wchar_t * tbuf)
{
	if(val < SUPPLEMENTAL_PLANE_FIRST) {
		tbuf[0] = static_cast<wchar_t>(val);
		return 1;
	}
	else {
		tbuf[0] = static_cast<wchar_t>(((val - SUPPLEMENTAL_PLANE_FIRST) >> 10) + SURROGATE_LEAD_FIRST);
		tbuf[1] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		return 2;
	}
}

int UTF8BytesOfLead[256];
static bool initialisedBytesOfLead = false;

static int FASTCALL BytesFromLead(int leadByte)
{
	if(leadByte < 0xC2) // Single byte or invalid
		return 1;
	else if(leadByte < 0xE0)
		return 2;
	else if(leadByte < 0xF0)
		return 3;
	else if(leadByte < 0xF5)
		return 4;
	else // Characters longer than 4 bytes not possible in current UTF-8
		return 1;
}

void UTF8BytesOfLeadInitialise()
{
	if(!initialisedBytesOfLead) {
		for(int i = 0; i < 256; i++) {
			UTF8BytesOfLead[i] = BytesFromLead(i);
		}
		initialisedBytesOfLead = true;
	}
}

// Return both the width of the first character in the string and a status
// saying whether it is valid or invalid.
// Most invalid sequences return a width of 1 so are treated as isolated bytes but
// the non-characters *FFFE, *FFFF and FDD0 .. FDEF return 3 or 4 as they can be
// reasonably treated as code points in some circumstances. They will, however,
// not have associated glyphs.
int FASTCALL UTF8Classify(const uchar * us, int len)
{
	// For the rules: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
	if(*us < 0x80) {
		// Single bytes easy
		return 1;
	}
	else if(*us > 0xf4) {
		// Characters longer than 4 bytes not possible in current UTF-8
		return UTF8MaskInvalid | 1;
	}
	else if(*us >= 0xf0) {
		// 4 bytes
		if(len < 4)
			return UTF8MaskInvalid | 1;
		if(UTF8IsTrailByte(us[1]) && UTF8IsTrailByte(us[2]) && UTF8IsTrailByte(us[3])) {
			if(((us[1] & 0xf) == 0xf) && (us[2] == 0xbf) && ((us[3] == 0xbe) || (us[3] == 0xbf))) {
				// *FFFE or *FFFF non-character
				return UTF8MaskInvalid | 4;
			}
			if(*us == 0xf4) {
				// Check if encoding a value beyond the last Unicode character 10FFFF
				if(us[1] > 0x8f) {
					return UTF8MaskInvalid | 1;
				}
				else if(us[1] == 0x8f) {
					if(us[2] > 0xbf) {
						return UTF8MaskInvalid | 1;
					}
					else if(us[2] == 0xbf) {
						if(us[3] > 0xbf) {
							return UTF8MaskInvalid | 1;
						}
					}
				}
			}
			else if((*us == 0xf0) && ((us[1] & 0xf0) == 0x80)) {
				// Overlong
				return UTF8MaskInvalid | 1;
			}
			return 4;
		}
		else {
			return UTF8MaskInvalid | 1;
		}
	}
	else if(*us >= 0xe0) {
		// 3 bytes
		if(len < 3)
			return UTF8MaskInvalid | 1;
		else if(UTF8IsTrailByte(us[1]) && UTF8IsTrailByte(us[2])) {
			if((*us == 0xe0) && ((us[1] & 0xe0) == 0x80)) // Overlong
				return UTF8MaskInvalid | 1;
			else if((*us == 0xed) && ((us[1] & 0xe0) == 0xa0)) // Surrogate
				return UTF8MaskInvalid | 1;
			else if((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbe)) // U+FFFE non-character - 3 bytes long
				return UTF8MaskInvalid | 3;
			else if((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbf)) // U+FFFF non-character - 3 bytes long
				return UTF8MaskInvalid | 3;
			else if((*us == 0xef) && (us[1] == 0xb7) && (((us[2] & 0xf0) == 0x90) || ((us[2] & 0xf0) == 0xa0))) // U+FDD0 .. U+FDEF
				return UTF8MaskInvalid | 3;
			else 
				return 3;
		}
		else
			return UTF8MaskInvalid | 1;
	}
	else if(*us >= 0xc2) {
		// 2 bytes
		if(len < 2)
			return UTF8MaskInvalid | 1;
		if(UTF8IsTrailByte(us[1])) {
			return 2;
		}
		else {
			return UTF8MaskInvalid | 1;
		}
	}
	else {
		// 0xc0 .. 0xc1 is overlong encoding
		// 0x80 .. 0xbf is trail byte
		return UTF8MaskInvalid | 1;
	}
}

int FASTCALL UTF8DrawBytes(const uchar * us, int len)
{
	int utf8StatusNext = UTF8Classify(us, len);
	return (utf8StatusNext & UTF8MaskInvalid) ? 1 : (utf8StatusNext & UTF8MaskWidth);
}

#ifdef SCI_NAMESPACE
}
#endif
