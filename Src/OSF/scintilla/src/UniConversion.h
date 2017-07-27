// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UTF-8 and UTF-16 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef UNICONVERSION_H
#define UNICONVERSION_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

const int UTF8MaxBytes = 4;
const int unicodeReplacementChar = 0xFFFD;

extern int UTF8BytesOfLead[256];

enum { 
	UTF8MaskWidth=0x7, 
	UTF8MaskInvalid=0x8 
};
//
// Line separator is U+2028 \xe2\x80\xa8
// Paragraph separator is U+2029 \xe2\x80\xa9
//
const int UTF8SeparatorLength = 3;

// NEL is U+0085 \xc2\x85
const int UTF8NELLength = 2;

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_LEAD_LAST = 0xDBFF };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };
enum { SUPPLEMENTAL_PLANE_FIRST = 0x10000 };

uint   UTF8Length(const wchar_t *uptr, uint tlen);
void   UTF8FromUTF16(const wchar_t *uptr, uint tlen, char *putf, uint len);
uint   UTF8CharLength(uchar ch);
size_t UTF16Length(const char *s, size_t len);
size_t UTF16FromUTF8(const char *s, size_t len, wchar_t *tbuf, size_t tlen);
uint   UTF32FromUTF8(const char *s, uint len, uint *tbuf, uint tlen);
uint   UTF16FromUTF32Character(uint val, wchar_t *tbuf);
void   UTF8BytesOfLeadInitialise();
int    FASTCALL UTF8Classify(const uchar *us, int len);
//
// Similar to UTF8Classify but returns a length of 1 for invalid bytes
// instead of setting the invalid flag
//
int    FASTCALL UTF8DrawBytes(const uchar *us, int len);
bool   FASTCALL UTF8IsTrailByte(int ch);
bool   FASTCALL UTF8IsAscii(int ch);
bool   FASTCALL UTF8IsSeparator(const uchar *us);
bool   FASTCALL UTF8IsNEL(const uchar *us);
uint   FASTCALL UTF16CharLength(wchar_t uch);

#ifdef SCI_NAMESPACE
}
#endif

#endif
