// ustream.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2001-2016, International Business Machines Corporation and others.  All Rights Reserved.
// Modification History:
// Date        Name        Description
// 06/25/2001  grhoten     Move iostream from unistr.h to here
// 
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_CONVERSION

#include "ustr_cnv.h"

// console IO

#define STD_NAMESPACE std::
#define STD_OSTREAM STD_NAMESPACE ostream
#define STD_ISTREAM STD_NAMESPACE istream

U_NAMESPACE_BEGIN

U_IO_API STD_OSTREAM & U_EXPORT2 operator<<(STD_OSTREAM& stream, const UnicodeString & str)
{
	if(str.length() > 0) {
		char buffer[200];
		UErrorCode errorCode = U_ZERO_ERROR;
		// use the default converter to convert chunks of text
		UConverter * converter = u_getDefaultConverter(&errorCode);
		if(U_SUCCESS(errorCode)) {
			const char16_t * us = str.getBuffer();
			const char16_t * uLimit = us + str.length();
			char * s, * sLimit = buffer + (sizeof(buffer) - 1);
			do {
				errorCode = U_ZERO_ERROR;
				s = buffer;
				ucnv_fromUnicode(converter, &s, sLimit, &us, uLimit, 0, FALSE, &errorCode);
				*s = 0;
				// write this chunk
				if(s > buffer) {
					stream << buffer;
				}
			} while(errorCode == U_BUFFER_OVERFLOW_ERROR);
			u_releaseDefaultConverter(converter);
		}
	}
/* stream.flush();*/
	return stream;
}

U_IO_API STD_ISTREAM & U_EXPORT2 operator>>(STD_ISTREAM& stream, UnicodeString & str)
{
	// This is like ICU status checking.
	if(stream.fail()) {
		return stream;
	}
	/* ipfx should eat whitespace when ios::skipws is set */
	char16_t uBuffer[16];
	char buffer[16];
	int32_t idx = 0;
	UErrorCode errorCode = U_ZERO_ERROR;
	// use the default converter to convert chunks of text
	UConverter * converter = u_getDefaultConverter(&errorCode);
	if(U_SUCCESS(errorCode)) {
		char16_t * us = uBuffer;
		const char16_t * uLimit = uBuffer + SIZEOFARRAYi(uBuffer);
		const char * s, * sLimit;
		char ch;
		char16_t ch32;
		bool initialWhitespace = TRUE;
		bool continueReading = TRUE;
		/* We need to consume one byte at a time to see what is considered whitespace. */
		while(continueReading) {
			ch = stream.get();
			if(stream.eof()) {
				// The EOF is only set after the get() of an unavailable byte.
				if(!initialWhitespace) {
					stream.clear(stream.eofbit);
				}
				continueReading = FALSE;
			}
			sLimit = &ch + (int)continueReading;
			us = uBuffer;
			s = &ch;
			errorCode = U_ZERO_ERROR;
			/*
			   Since we aren't guaranteed to see the state before this call,
			   this code won't work on stateful encodings like ISO-2022 or an EBCDIC stateful encoding.
			   We flush on the last byte to ensure that we output truncated multibyte characters.
			 */
			ucnv_toUnicode(converter, &us, uLimit, &s, sLimit, 0, !continueReading, &errorCode);
			if(U_FAILURE(errorCode)) {
				/* Something really bad happened. setstate() isn't always an available API */
				stream.clear(stream.failbit);
				goto STOP_READING;
			}
			/* Was the character consumed? */
			if(us != uBuffer) {
				/* Reminder: ibm-1390 & JISX0213 can output 2 Unicode code points */
				int32_t uBuffSize = static_cast<int32_t>(us-uBuffer);
				int32_t uBuffIdx = 0;
				while(uBuffIdx < uBuffSize) {
					U16_NEXT(uBuffer, uBuffIdx, uBuffSize, ch32);
					if(u_isWhitespace(ch32)) {
						if(!initialWhitespace) {
							buffer[idx++] = ch;
							while(idx > 0) {
								stream.putback(buffer[--idx]);
							}
							goto STOP_READING;
						}
						/* else skip intialWhitespace */
					}
					else {
						if(initialWhitespace) {
							/*
							   When initialWhitespace is TRUE, we haven't appended any
							   character yet.  This is where we truncate the string,
							   to avoid modifying the string before we know if we can
							   actually read from the stream.
							 */
							str.truncate(0);
							initialWhitespace = FALSE;
						}
						str.append(ch32);
					}
				}
				idx = 0;
			}
			else {
				buffer[idx++] = ch;
			}
		}
STOP_READING:
		u_releaseDefaultConverter(converter);
	}
/* stream.flush();*/
	return stream;
}

U_NAMESPACE_END

#endif
