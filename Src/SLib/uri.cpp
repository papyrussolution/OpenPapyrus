// URI.CPP
// uriparser - RFC 3986 URI parsing library
// Copyright(C) 2007, Weijia Song <songweijia@gmail.com>, Sebastian Pipping <webmaster@hartwork.org> All rights reserved.
// Adopted to SLIB by A.Sobolev 2010..2021
// 
#include <slib-internal.h>
#pragma hdrstop
#include <uri.h>

#define _UT(x) x
//
// Used to point to from empty path segments.
// X.first and X.afterLast must be the same non-NULL value then.
//
static const char * const UriSafeToPointTo = _UT("X");
static const char * const UriConstPwd = _UT(".");
static const char * const UriConstParent = _UT("..");
//
//
//
static void FASTCALL uriWriteQuadToDoubleByte(const uchar * hexDigits, int digitCount, uchar * output)
{
	switch(digitCount) {
	    case 1: // 0x___? -> \x00 \x0? 
			output[0] = 0;
			output[1] = hexDigits[0];
			break;
	    case 2: // 0x__?? -> \0xx \x?? 
			output[0] = 0;
			output[1] = 16*hexDigits[0]+hexDigits[1];
			break;
	    case 3: // 0x_??? -> \0x? \x?? 
			output[0] = hexDigits[0];
			output[1] = 16*hexDigits[1]+hexDigits[2];
			break;
	    case 4: // 0x???? -> \0?? \x?? 
			output[0] = 16*hexDigits[0]+hexDigits[1];
			output[1] = 16*hexDigits[2]+hexDigits[3];
			break;
	}
}

static uchar FASTCALL uriGetOctetValue(const uchar * digits, int digitCount)
{
	switch(digitCount) {
	    case 1: return digits[0];
	    case 2: return 10*digits[0]+digits[1];
	    case 3:
		default: return 100*digits[0]+10*digits[1]+digits[2];
	}
}
// 
// Specifies which component of a %URI has to be normalized.
// 
typedef enum UriNormalizationMaskEnum {
	URI_NORMALIZED = 0, /**< Do not normalize anything */
	URI_NORMALIZE_SCHEME = 1<<0,   /**< Normalize scheme (fix uppercase letters) */
	URI_NORMALIZE_USER_INFO = 1<<1,   /**< Normalize user info (fix uppercase percent-encodings) */
	URI_NORMALIZE_HOST = 1<<2,   /**< Normalize host (fix uppercase letters) */
	URI_NORMALIZE_PATH = 1<<3,   /**< Normalize path (fix uppercase percent-encodings and redundant dot segments) */
	URI_NORMALIZE_QUERY = 1<<4,   /**< Normalize query (fix uppercase percent-encodings) */
	URI_NORMALIZE_FRAGMENT = 1<<5   /**< Normalize fragment (fix uppercase percent-encodings) */
} UriNormalizationMask; /**< @copydoc UriNormalizationMaskEnum */

static int FASTCALL uriIsUnreserved(int code)
{
	switch(code) {
		// ALPHA 
	    case L'a': case L'A': case L'b': case L'B': case L'c': case L'C': case L'd': case L'D':
	    case L'e': case L'E': case L'f': case L'F': case L'g': case L'G': case L'h': case L'H':
	    case L'i': case L'I': case L'j': case L'J': case L'k': case L'K': case L'l': case L'L':
	    case L'm': case L'M': case L'n': case L'N': case L'o': case L'O': case L'p': case L'P':
	    case L'q': case L'Q': case L'r': case L'R': case L's': case L'S': case L't': case L'T':
	    case L'u': case L'U': case L'v': case L'V': case L'w': case L'W': case L'x': case L'X':
	    case L'y': case L'Y': case L'z': case L'Z':
		// DIGIT 
	    case L'0': case L'1': case L'2': case L'3': case L'4': case L'5':
	    case L'6': case L'7': case L'8': case L'9':
		// "-" / "." / "_" / "~" 
	    case L'-': case L'.': case L'_': case L'~':
			return TRUE;
	    default:
			return FALSE;
	}
}
//
// Appends a relative URI to an absolute. The last path segement of
// the absolute URI is replaced.
//
static int FASTCALL UriMergePath(UriUri * absWork, const UriUri * relAppend)
{
	int    ok = 1;
	if(relAppend->pathHead) {
		UriUri::PathSegment * sourceWalker;
		UriUri::PathSegment * destPrev;
		// Replace last segment("" if trailing slash) with first of append chain 
		if(absWork->pathHead == NULL) {
			UriUri::PathSegment * const dup = new UriUri::PathSegment(0, 0);
			THROW(dup); // Raises SAlloc::M error 
			absWork->pathHead = dup;
			absWork->pathTail = dup;
		}
		absWork->pathTail->text = relAppend->pathHead->text;
		// Append all the others 
		sourceWalker = relAppend->pathHead->next;
		if(sourceWalker) {
			destPrev = absWork->pathTail;
			for(; ok;) {
				UriUri::PathSegment * const dup = new UriUri::PathSegment(sourceWalker->text.P_First, sourceWalker->text.P_AfterLast);
				if(dup == NULL) {
					destPrev->next = NULL;
					absWork->pathTail = destPrev;
					CALLEXCEPT(); // Raises SAlloc::M error
				}
				else {
					destPrev->next = dup;
					if(sourceWalker->next == NULL) {
						absWork->pathTail = dup;
						absWork->pathTail->next = NULL;
						break;
					}
					else {
						destPrev = dup;
						sourceWalker = sourceWalker->next;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
static int FASTCALL UriAppendSegment(UriUri * uri, const char * first, const char * afterLast)
{
	int    ok = 1;
	// Create segment 
	UriUri::PathSegment * p_segment = new UriUri::PathSegment(first, afterLast);
	if(p_segment == NULL)
		ok = 0;
	else {
		// Put into chain 
		if(uri->pathTail == NULL)
			uri->pathHead = p_segment;
		else
			uri->pathTail->next = p_segment;
		uri->pathTail = p_segment;
	}
	return ok;
}

static int FASTCALL UriEqualsAuthority(const UriUri * pFirst, const UriUri * second)
{
	
	if(pFirst->HostData.ip4) // IPv4 
		return (second->HostData.ip4 && !memcmp(pFirst->HostData.ip4->data, second->HostData.ip4->data, 4)) ? TRUE : FALSE;
	else if(pFirst->HostData.ip6) // IPv6 
		return (second->HostData.ip6 && !memcmp(pFirst->HostData.ip6->data, second->HostData.ip6->data, 16)) ? TRUE : FALSE;
	else if(pFirst->HostData.ipFuture.P_First) // IPvFuture 
		return (second->HostData.ipFuture.P_First && !strncmp(pFirst->HostData.ipFuture.P_First,
			second->HostData.ipFuture.P_First, pFirst->HostData.ipFuture.P_AfterLast-pFirst->HostData.ipFuture.P_First)) ? TRUE : FALSE;
	else if(pFirst->HostText.P_First)
		return (second->HostText.P_First && !strncmp(pFirst->HostText.P_First, second->HostText.P_First,
			pFirst->HostText.P_AfterLast-pFirst->HostText.P_First)) ? TRUE : FALSE;
	else
		return (second->HostText.P_First == NULL);
}
//
//
//
static char * FASTCALL UriEscapeEx(const char * inFirst, const char * inAfterLast, char * out, int spaceToPlus, int normalizeBreaks)
{
	const char * read = inFirst;
	char * write = out;
	int prevWasCr = FALSE;
	if(!out || inFirst == out) {
		return NULL;
	}
	else if(inFirst == NULL) {
		ASSIGN_PTR(out, _UT('\0'));
		return out;
	}
	else {
		for(;;) {
			if(inAfterLast && read >= inAfterLast) {
				write[0] = _UT('\0');
				return write;
			}
			else {
				switch(read[0]) {
					case _UT('\0'):
						write[0] = _UT('\0');
						return write;
					case _UT(' '):
						if(spaceToPlus) {
							write[0] = _UT('+');
							write++;
						}
						else {
							write[0] = _UT('%');
							write[1] = _UT('2');
							write[2] = _UT('0');
							write += 3;
						}
						prevWasCr = FALSE;
						break;
					// ALPHA 
					case _UT('a'): case _UT('A'): case _UT('b'): case _UT('B'): case _UT('c'): case _UT('C'): case _UT('d'): case _UT('D'):
					case _UT('e'): case _UT('E'): case _UT('f'): case _UT('F'): case _UT('g'): case _UT('G'): case _UT('h'): case _UT('H'):
					case _UT('i'): case _UT('I'): case _UT('j'): case _UT('J'): case _UT('k'): case _UT('K'): case _UT('l'): case _UT('L'):
					case _UT('m'): case _UT('M'): case _UT('n'): case _UT('N'): case _UT('o'): case _UT('O'): case _UT('p'): case _UT('P'):
					case _UT('q'): case _UT('Q'): case _UT('r'): case _UT('R'): case _UT('s'): case _UT('S'): case _UT('t'): case _UT('T'):
					case _UT('u'): case _UT('U'): case _UT('v'): case _UT('V'): case _UT('w'): case _UT('W'): case _UT('x'): case _UT('X'):
					case _UT('y'): case _UT('Y'): case _UT('z'): case _UT('Z'):
					// DIGIT 
					case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'):
					// "-" / "." / "_" / "~" 
					case _UT('-'): case _UT('.'): case _UT('_'): case _UT('~'):
						// Copy unmodified 
						write[0] = read[0];
						write++;
						prevWasCr = FALSE;
						break;
					case _UT('\x0A'):
						if(normalizeBreaks) {
							if(!prevWasCr) {
								write[0] = _UT('%');
								write[1] = _UT('0');
								write[2] = _UT('D');
								write[3] = _UT('%');
								write[4] = _UT('0');
								write[5] = _UT('A');
								write += 6;
							}
						}
						else {
							write[0] = _UT('%');
							write[1] = _UT('0');
							write[2] = _UT('A');
							write += 3;
						}
						prevWasCr = FALSE;
						break;
					case _UT('\x0D'):
						if(normalizeBreaks) {
							write[0] = _UT('%');
							write[1] = _UT('0');
							write[2] = _UT('D');
							write[3] = _UT('%');
							write[4] = _UT('0');
							write[5] = _UT('A');
							write += 6;
						}
						else {
							write[0] = _UT('%');
							write[1] = _UT('0');
							write[2] = _UT('D');
							write += 3;
						}
						prevWasCr = TRUE;
						break;
					default:
						{ // Percent encode 
							const uchar code = static_cast<uchar>(read[0]);
							write[0] = _UT('%');
							write[1] = UriHexToLetter(code>>4);
							write[2] = UriHexToLetter(code&0x0f);
							write += 3;
						}
						prevWasCr = FALSE;
						break;
				}
				read++;
			}
		}
	}
}

static int UriComposeQueryEngine(char * dest, const UriQueryList*queryList, int maxChars,
	int * charsWritten, int * charsRequired, int spaceToPlus, int normalizeBreaks);
static int UriAppendQueryItem(UriQueryList**prevNext, int * itemCount, const char * keyFirst, const char * keyAfter,
	const char * valueFirst, const char * valueAfter, int plusToSpace, UriBreakConversion breakConversion);

int UriComposeQueryCharsRequired(const UriQueryList*queryList, int * charsRequired)
{
	const int spaceToPlus = TRUE;
	const int normalizeBreaks = TRUE;
	return UriComposeQueryCharsRequiredEx(queryList, charsRequired, spaceToPlus, normalizeBreaks);
}

int UriComposeQueryCharsRequiredEx(const UriQueryList*queryList, int * charsRequired, int spaceToPlus, int normalizeBreaks)
{
	return (queryList && charsRequired) ? UriComposeQueryEngine(NULL, queryList, 0, NULL, charsRequired, spaceToPlus, normalizeBreaks) : SLERR_URI_NULL;
}

int UriComposeQuery(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten)
{
	const int spaceToPlus = TRUE;
	const int normalizeBreaks = TRUE;
	return UriComposeQueryEx(dest, queryList, maxChars, charsWritten, spaceToPlus, normalizeBreaks);
}

int UriComposeQueryEx(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten, int spaceToPlus, int normalizeBreaks)
{
	if(!dest || !queryList)
		return SLERR_URI_NULL;
	else if(maxChars < 1)
		return SLERR_URI_OUTPUT_TOO_LARGE;
	else
		return UriComposeQueryEngine(dest, queryList, maxChars, charsWritten, NULL, spaceToPlus, normalizeBreaks);
}

int UriComposeQueryMalloc(char ** dest, const UriQueryList * queryList)
{
	const int spaceToPlus = TRUE;
	const int normalizeBreaks = TRUE;
	return UriComposeQueryMallocEx(dest, queryList, spaceToPlus, normalizeBreaks);
}

int UriComposeQueryMallocEx(char * *dest, const UriQueryList*queryList, int spaceToPlus, int normalizeBreaks)
{
	int charsRequired;
	int res;
	char * queryString;
	if(!dest) {
		return SLERR_URI_NULL;
	}
	/* Calculate space */
	res = UriComposeQueryCharsRequiredEx(queryList, &charsRequired, spaceToPlus, normalizeBreaks);
	if(res != SLERR_SUCCESS) {
		return res;
	}
	charsRequired++;
	/* Allocate space */
	queryString =static_cast<char *>(SAlloc::M(charsRequired*sizeof(char)));
	if(queryString == NULL) {
		return SLERR_NOMEM;
	}
	/* Put query in */
	res = UriComposeQueryEx(queryString, queryList, charsRequired, NULL, spaceToPlus, normalizeBreaks);
	if(res != SLERR_SUCCESS) {
		SAlloc::F(queryString);
		return res;
	}
	*dest = queryString;
	return SLERR_SUCCESS;
}

int UriComposeQueryEngine(char * dest, const UriQueryList*queryList,
	int maxChars, int * charsWritten, int * charsRequired, int spaceToPlus, int normalizeBreaks)
{
	int firstItem = TRUE;
	int ampersandLen = 0;
	char * write = dest;
	/* Subtract terminator */
	if(!dest) {
		*charsRequired = 0;
	}
	else {
		maxChars--;
	}
	while(queryList) {
		const char * const key = queryList->key;
		const char * const value = queryList->value;
		const int worstCase =(normalizeBreaks == TRUE ? 6 : 3);
		const int keyLen = sstrleni(key);
		const int keyRequiredChars = worstCase*keyLen;
		const int valueLen = sstrleni(value);
		const int valueRequiredChars = worstCase*valueLen;
		if(!dest) {
			if(firstItem == TRUE) {
				ampersandLen = 1;
				firstItem = FALSE;
			}
			(*charsRequired) += ampersandLen+keyRequiredChars+((value == NULL) ? 0 : 1+valueRequiredChars);
		}
		else {
			char * afterKey;
			if((write-dest)+ampersandLen+keyRequiredChars > maxChars) {
				return SLERR_URI_OUTPUT_TOO_LARGE;
			}
			/* Copy key */
			if(firstItem == TRUE) {
				firstItem = FALSE;
			}
			else {
				write[0] = _UT('&');
				write++;
			}
			afterKey = UriEscapeEx(key, key+keyLen, write, spaceToPlus, normalizeBreaks);
			write +=(afterKey-write);
			if(value) {
				char * afterValue;
				if((write-dest)+1+valueRequiredChars > maxChars) {
					return SLERR_URI_OUTPUT_TOO_LARGE;
				}
				/* Copy value */
				write[0] = _UT('=');
				write++;
				afterValue = UriEscapeEx(value, value+valueLen, write, spaceToPlus, normalizeBreaks);
				write += (afterValue-write);
			}
		}
		queryList = queryList->next;
	}
	if(dest) {
		write[0] = _UT('\0');
		if(charsWritten)
			*charsWritten = (int)(write-dest)+1; /* .. for terminator */
	}
	return SLERR_SUCCESS;
}

static const char * FASTCALL UriUnescapeInPlaceEx(char * inout, int plusToSpace, UriBreakConversion breakConversion)
{
	char * p_write = inout;
	char * p_read = inout;
	int prevWasCr = FALSE;
	THROW(inout);
	while(p_read[0] != _UT('\0')) {
		switch(p_read[0]) {
		    /*case _UT('\0'):
				if(read > p_write)
					p_write[0] = _UT('\0');
				return p_write;*/
		    case _UT('%'):
			switch(p_read[1]) {
			    case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'):
			    case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'):
			    case _UT('a'): case _UT('b'): case _UT('c'): case _UT('d'): case _UT('e'):
			    case _UT('f'): case _UT('A'): case _UT('B'): case _UT('C'): case _UT('D'):
			    case _UT('E'): case _UT('F'):
				switch(p_read[2]) {
				    case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): 
					case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'): 
					case _UT('a'): case _UT('b'): case _UT('c'): case _UT('d'): case _UT('e'):
				    case _UT('f'): case _UT('A'): case _UT('B'): case _UT('C'): case _UT('D'):
				    case _UT('E'): case _UT('F'):
				    {
					    // Percent group found 
					    const uchar left = UriHexdigToInt(p_read[1]);
					    const uchar right = UriHexdigToInt(p_read[2]);
					    const int code = 16*left+right;
					    switch(code) {
							case 10:
								switch(breakConversion) {
									case URI_BR_TO_LF:
										if(!prevWasCr) {
											p_write[0] = 10;
											p_write++;
										}
										break;
									case URI_BR_TO_CRLF:
										if(!prevWasCr) {
											p_write[0] = 13;
											p_write[1] = 10;
											p_write += 2;
										}
										break;
									case URI_BR_TO_CR:
										if(!prevWasCr) {
											p_write[0] = 13;
											p_write++;
										}
										break;
									case URI_BR_DONT_TOUCH:
									default:
										p_write[0] = 10;
										p_write++;
								}
								prevWasCr = FALSE;
								break;
							case 13:
								switch(breakConversion) {
									case URI_BR_TO_LF:
										p_write[0] = 10;
										p_write++;
										break;
									case URI_BR_TO_CRLF:
										p_write[0] = 13;
										p_write[1] = 10;
										p_write += 2;
										break;
									case URI_BR_TO_CR:
										p_write[0] = 13;
										p_write++;
										break;
									case URI_BR_DONT_TOUCH:
									default:
										p_write[0] = 13;
										p_write++;
								}
								prevWasCr = TRUE;
								break;
							default:
								p_write[0] = static_cast<char>(code);
								p_write++;
								prevWasCr = FALSE;
							}
							p_read += 3;
						}
						break;
				    default:
						// Copy two chars unmodified and  look at this char again 
						if(p_read > p_write) {
							p_write[0] = p_read[0];
							p_write[1] = p_read[1];
						}
						p_read += 2;
						p_write += 2;
						prevWasCr = FALSE;
				}
				break;
			    default:
					// Copy one char unmodified and  look at this char again 
					if(p_read > p_write)
						p_write[0] = p_read[0];
					p_read++;
					p_write++;
					prevWasCr = FALSE;
			}
			break;
		    case _UT('+'):
				if(plusToSpace) // Convert '+' to ' ' 
					p_write[0] = _UT(' ');
				else { // Copy one char unmodified 
					if(p_read > p_write)
						p_write[0] = p_read[0];
				}
				p_read++;
				p_write++;
				prevWasCr = FALSE;
				break;
		    default:
				// Copy one char unmodified 
				if(p_read > p_write) {
					p_write[0] = p_read[0];
				}
				p_read++;
				p_write++;
				prevWasCr = FALSE;
		}
	}
	if(p_read > p_write)
		p_write[0] = _UT('\0');
	CATCH
		p_write = 0;
	ENDCATCH
	return p_write;
}

int UriAppendQueryItem(UriQueryList ** ppPrevNext, int * pItemCount, const char * pKeyFirst, const char * pKeyAfter,
	const char * pValueFirst, const char * pValueAfter, int plusToSpace, UriBreakConversion breakConversion)
{
	int    ok = 1;
	const  int key_len = (int)(pKeyAfter-pKeyFirst);
	const  int value_len = (int)(pValueAfter-pValueFirst);
	char * p_key = 0;
	char * p_value = 0;
	ASSIGN_PTR(ppPrevNext, 0);
	/*if(!ppPrevNext || !pItemCount || !pKeyFirst || !pKeyAfter || (pKeyFirst > pKeyAfter) ||(pValueFirst > pValueAfter) || ((pKeyFirst == pKeyAfter) && !pValueFirst && !pValueAfter)) {
		return TRUE;
	}*/
	THROW(ppPrevNext && pItemCount && pKeyFirst && pKeyAfter && (pKeyFirst <= pKeyAfter) && (pValueFirst <= pValueAfter) && ((pKeyFirst != pKeyAfter) || pValueFirst || pValueAfter));
	// Append new empty item 
	THROW(*ppPrevNext = static_cast<UriQueryList *>(SAlloc::M(1*sizeof(UriQueryList))));
	(*ppPrevNext)->next = NULL;
	// Fill key 
	THROW(p_key = static_cast<char *>(SAlloc::M((key_len+1)*sizeof(char))));
	p_key[key_len] = _UT('\0');
	if(key_len > 0) {
		memcpy(p_key, pKeyFirst, key_len*sizeof(char));
		UriUnescapeInPlaceEx(p_key, plusToSpace, breakConversion);
	}
	(*ppPrevNext)->key = p_key;
	// Fill value 
	if(pValueFirst) {
		THROW(p_value = static_cast<char *>(SAlloc::M((value_len+1)*sizeof(char))));
		p_value[value_len] = _UT('\0');
		if(value_len > 0) {
			memcpy(p_value, pValueFirst, value_len*sizeof(char));
			UriUnescapeInPlaceEx(p_value, plusToSpace, breakConversion);
		}
		(*ppPrevNext)->value = p_value;
	}
	else
		p_value = NULL;
	(*ppPrevNext)->value = p_value;
	(*pItemCount)++;
	CATCH
		SAlloc::F(p_key);
		if(ppPrevNext) {
			SAlloc::F(*ppPrevNext);
			*ppPrevNext = NULL;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

void UriFreeQueryList(UriQueryList * pQueryList) 
{
	while(pQueryList) {
		UriQueryList * p_next_backup = pQueryList->next;
		SAlloc::F((char *)pQueryList->key); // @badcast
		SAlloc::F((char *)pQueryList->value); // @badcast
		SAlloc::F(pQueryList);
		pQueryList = p_next_backup;
	}
}

int UriDissectQueryMalloc(UriQueryList ** dest, int * itemCount, const char * pFirst, const char * afterLast) 
{
	const int plusToSpace = TRUE;
	const UriBreakConversion breakConversion = URI_BR_DONT_TOUCH;
	return UriDissectQueryMallocEx(dest, itemCount, pFirst, afterLast, plusToSpace, breakConversion);
}

int UriDissectQueryMallocEx(UriQueryList ** ppDest, int * pItemCount, const char * pFirst, const char * pAfterLast, int plusToSpace, UriBreakConversion breakConversion) 
{
	int    ok = 1;
	const char * p_walk = pFirst;
	const char * p_key_first = pFirst;
	const char * p_key_after = NULL;
	const char * p_value_first = NULL;
	const char * p_value_after = NULL;
	UriQueryList ** pp_prev_next = ppDest;
	int   nullCounter;
	int * p_items_appended = NZOR(pItemCount, &nullCounter);
	ASSIGN_PTR(ppDest, 0);
	*p_items_appended = 0;
	THROW_S(ppDest && pFirst && pAfterLast, SLERR_URI_NULL);
	THROW_S(pFirst <= pAfterLast, SLERR_URI_RANGE_INVALID);
	// Parse query string 
	for(; p_walk < pAfterLast; p_walk++) {
		switch(*p_walk) {
		    case _UT('&'):
				if(p_value_first)
					p_value_after = p_walk;
				else
					p_key_after = p_walk;
				THROW(UriAppendQueryItem(pp_prev_next, p_items_appended, p_key_first, p_key_after, p_value_first, p_value_after, plusToSpace, breakConversion));
				// Make future items children of the current 
				if(pp_prev_next && *pp_prev_next) {
					pp_prev_next = &((*pp_prev_next)->next);
				}
				p_key_first = (p_walk+1 < pAfterLast) ? (p_walk+1) : NULL;
				p_key_after = NULL;
				p_value_first = NULL;
				p_value_after = NULL;
				break;
		    case _UT('='): // NOTE: WE treat the first '=' as a separator,  all following go into the value part 
				if(!p_key_after) {
					p_key_after = p_walk;
					if(p_walk+1 < pAfterLast) {
						p_value_first = p_walk+1;
						p_value_after = p_walk+1;
					}
				}
				break;
		    default:
				break;
		}
	}
	if(p_value_first)
		p_value_after = p_walk; // Must be key/value pair 
	else
		p_key_after = p_walk; // Must be key only 
	THROW(UriAppendQueryItem(pp_prev_next, p_items_appended, p_key_first, p_key_after, p_value_first, p_value_after, plusToSpace, breakConversion));
	CATCH
		// Free list we built 
		*p_items_appended = 0;
		UriFreeQueryList(*ppDest);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
static int UriFilenameToUriString(const char * pFileName, char * uriString, int fromUnix)
{
	int    ok = 1;
	const char * input = pFileName;
	const char * lastSep = input-1;
	int firstSegment = TRUE;
	char * output = uriString;
	const int absolute = (pFileName) && ((fromUnix && (pFileName[0] == _UT('/'))) || (!fromUnix &&(pFileName[0] != _UT('\0')) &&(pFileName[1] == _UT(':'))));
	THROW_S(pFileName && uriString, SLERR_URI_NULL);
	if(absolute) {
		const char * const prefix = fromUnix ? _UT("file://") : _UT("file:///");
		const int prefixLen = fromUnix ? 7 : 8;
		// Copy prefix 
		memcpy(uriString, prefix, prefixLen*sizeof(char));
		output += prefixLen;
	}
	// Copy and escape on the fly 
	for(;;) {
		if((input[0] == _UT('\0')) ||(fromUnix && input[0] == _UT('/')) ||(!fromUnix && input[0] == _UT('\\'))) {
			// Copy text after last seperator 
			if(lastSep+1 < input) {
				if(!fromUnix && absolute &&(firstSegment == TRUE)) {
					// Quick hack to not convert "C:" to "C%3A" 
					const int charsToCopy =(int)(input-(lastSep+1));
					memcpy(output, lastSep+1, charsToCopy*sizeof(char));
					output += charsToCopy;
				}
				else
					output = UriEscapeEx(lastSep+1, input, output, FALSE, FALSE);
			}
			firstSegment = FALSE;
		}
		if(input[0] == _UT('\0')) {
			output[0] = _UT('\0');
			break;
		}
		else if(fromUnix &&(input[0] == _UT('/'))) {
			// Copy separators unmodified 
			output[0] = _UT('/');
			output++;
			lastSep = input;
		}
		else if(!fromUnix &&(input[0] == _UT('\\'))) {
			// Convert backslashes to forward slashes 
			output[0] = _UT('/');
			output++;
			lastSep = input;
		}
		input++;
	}
	CATCHZOK
	return ok;
}

static int UriUriStringToFilename(const char * uriString, char * filename, int toUnix)
{
	const char * const prefix = toUnix ? _UT("file://") : _UT("file:///");
	const int prefixLen = toUnix ? 7 : 8;
	char * walker = filename;
	size_t charsToCopy;
	const int absolute =(strncmp(uriString, prefix, prefixLen) == 0);
	const int charsToSkip =(absolute ? prefixLen : 0);
	charsToCopy = sstrlen(uriString+charsToSkip)+1;
	memcpy(filename, uriString+charsToSkip, charsToCopy*sizeof(char));
	UriUnescapeInPlaceEx(filename, FALSE, URI_BR_DONT_TOUCH);
	// Convert forward slashes to backslashes 
	if(!toUnix) {
		while(walker[0] != _UT('\0')) {
			if(walker[0] == _UT('/')) {
				walker[0] = _UT('\\');
			}
			walker++;
		}
	}
	return 1;
}

int UriUnixFilenameToUriString(const char * filename, char * uriString)
	{ return UriFilenameToUriString(filename, uriString, TRUE); }
int UriWindowsFilenameToUriString(const char * filename, char * uriString)
	{ return UriFilenameToUriString(filename, uriString, FALSE); }
int UriUriStringToUnixFilename(const char * uriString, char * filename)
	{ return UriUriStringToFilename(uriString, filename, TRUE); }
int UriUriStringToWindowsFilename(const char * uriString, char * filename)
	{ return UriUriStringToFilename(uriString, filename, FALSE); }
//
// Compares two text ranges for equal text content 
//
static int FASTCALL UriCompareRange(const UriTextRange * a, const UriTextRange * b)
{
	// NOTE: Both NULL means equal! 
	if(!a || !b)
		return BIN(!a && !b);
	else {
		const  int a_len = a->Len();
		if(a_len > b->Len())
			return 1;
		else if(a_len < b->Len())
			return -1;
		else
			return strncmp(a->P_First, b->P_First, a_len);
	}
}

int UriEqualsUri(const UriUri * a, const UriUri * b)
{
	
	if(!a || !b) // NOTE: Both NULL means equal! 
		return BIN(!a && !b);
	else if(UriCompareRange(&(a->Scheme), &(b->Scheme))) // scheme 
		return FALSE;
	else if(!a->Scheme.P_First && (a->IsAbsolutePath != b->IsAbsolutePath)) // absolutePath 
		return FALSE;
	else if(UriCompareRange(&(a->UserInfo), &(b->UserInfo))) // userInfo 
		return FALSE;
	else if(((a->HostData.ip4 == NULL) != (b->HostData.ip4 == NULL)) || ((a->HostData.ip6 == NULL) != (b->HostData.ip6 == NULL)) || ((a->HostData.ipFuture.P_First == NULL)
	    !=(b->HostData.ipFuture.P_First == NULL))) { // Host 
		return FALSE;
	}
	if(a->HostData.ip4) {
		if(memcmp(a->HostData.ip4->data, b->HostData.ip4->data, 4)) {
			return FALSE;
		}
	}
	if(a->HostData.ip6) {
		if(memcmp(a->HostData.ip6->data, b->HostData.ip6->data, 16)) {
			return FALSE;
		}
	}
	if(a->HostData.ipFuture.P_First) {
		if(UriCompareRange(&(a->HostData.ipFuture), &(b->HostData.ipFuture))) {
			return FALSE;
		}
	}
	if(!a->HostData.ip4 && !a->HostData.ip6 && !a->HostData.ipFuture.P_First) {
		if(UriCompareRange(&(a->HostText), &(b->HostText))) {
			return FALSE;
		}
	}
	if(UriCompareRange(&(a->PortText), &(b->PortText))) { // portText 
		return FALSE;
	}
	else if((a->pathHead == NULL) !=(b->pathHead == NULL)) { // Path 
		return FALSE;
	}
	if(a->pathHead) {
		UriUri::PathSegment * walkA = a->pathHead;
		UriUri::PathSegment * walkB = b->pathHead;
		do {
			if(UriCompareRange(&(walkA->text), &(walkB->text)))
				return FALSE;
			else if((walkA->next == NULL) != (walkB->next == NULL))
				return FALSE;
			else {
				walkA = walkA->next;
				walkB = walkB->next;
			}
		} while(walkA);
	}
	if(UriCompareRange(&(a->query), &(b->query))) // query
		return FALSE;
	else if(UriCompareRange(&(a->fragment), &(b->fragment))) // fragment 
		return FALSE;
	//
	return TRUE; // Equal
}
//
//
//
void UriResetUri(UriUri * pUri)
	{ memzero(pUri, sizeof(*pUri)); }

// Properly removes "." and ".." path segments 
int UriRemoveDotSegments(UriUri * uri, int relative)
	{ return uri ? UriRemoveDotSegmentsEx(uri, relative, uri->IsOwner) : TRUE; }

int UriRemoveDotSegmentsEx(UriUri * pUri, int relative, int pathOwned)
{
	int    ok = 1;
	UriUri::PathSegment * p_walker = 0;
	UriUri::PathSegment * p_prev = 0;
	if(pUri && pUri->pathHead) {
		p_walker = pUri->pathHead;
		p_walker->reserved = NULL; /* Prev pointer */
		do {
			int removeSegment = FALSE;
			const int len = (int)(p_walker->text.P_AfterLast-p_walker->text.P_First);
			if(len == 1) {
				if((p_walker->text.P_First)[0] == _UT('.')) {
					// "." segment -> remove if not essential 
					p_prev = (UriUri::PathSegment *)p_walker->reserved;
					UriUri::PathSegment * const nextBackup = p_walker->next;
					// Is this dot segment essential? 
					removeSegment = TRUE;
					if(relative && p_walker == pUri->pathHead && p_walker->next) {
						for(const char * ch = p_walker->next->text.P_First; ch < p_walker->next->text.P_AfterLast; ch++) {
							if(*ch == _UT(':')) {
								removeSegment = FALSE;
								break;
							}
						}
					}
					if(removeSegment) {
						// Last segment? 
						if(p_walker->next) { // Not last segment 
							p_walker->next->reserved = p_prev;
							if(p_prev == NULL)
								pUri->pathHead = p_walker->next; // First but not last segment
							else
								p_prev->next = p_walker->next; // Middle segment
							if(pathOwned &&(p_walker->text.P_First != p_walker->text.P_AfterLast))
								SAlloc::F((char *)p_walker->text.P_First);
							delete p_walker;
						}
						else { // Last segment 
							if(pathOwned && (p_walker->text.P_First != p_walker->text.P_AfterLast)) {
								SAlloc::F((char *)p_walker->text.P_First);
							}
							if(p_prev == NULL) {
								// Last and first 
								if(UriIsHostSet(pUri)) {
									// Replace "." with empty segment to represent trailing slash 
									p_walker->text.P_First = UriSafeToPointTo;
									p_walker->text.P_AfterLast = UriSafeToPointTo;
								}
								else {
									delete p_walker;
									pUri->pathHead = NULL;
									pUri->pathTail = NULL;
								}
							}
							else {
								// Last but not first, replace "." with empty segment to represent trailing slash 
								p_walker->text.P_First = UriSafeToPointTo;
								p_walker->text.P_AfterLast = UriSafeToPointTo;
							}
						}
						p_walker = nextBackup;
					}
				}
			}
			else if(len == 2) {
				if((p_walker->text.P_First[0] == _UT('.')) && (p_walker->text.P_First[1] == _UT('.'))) {
					// Path ".." -> remove this and the previous segment 
					p_prev = (UriUri::PathSegment *)p_walker->reserved;
					UriUri::PathSegment * prevPrev;
					UriUri::PathSegment * const nextBackup = p_walker->next;
					removeSegment = TRUE;
					if(relative) {
						if(p_prev == NULL)
							removeSegment = FALSE;
						else if(p_prev->text.Len() == 2 && (p_prev->text.P_First[0] == _UT('.')) && (p_prev->text.P_First[1] == _UT('.')))
							removeSegment = FALSE;
					}
					if(removeSegment) {
						if(p_prev) { // Not first segment 
							prevPrev = (UriUri::PathSegment *)p_prev->reserved;
							if(prevPrev) {
								// Not even prev is the first one 
								prevPrev->next = p_walker->next;
								if(p_walker->next)
									p_walker->next->reserved = prevPrev;
								else {
									// Last segment -> insert "" segment to represent trailing slash, update tail 
									UriUri::PathSegment * const p_segment = new UriUri::PathSegment(UriSafeToPointTo, UriSafeToPointTo);
									THROW(p_segment);
									//memzero(p_segment, sizeof(*p_segment));
									//p_segment->text.P_First = UriSafeToPointTo;
									//p_segment->text.P_AfterLast = UriSafeToPointTo;
									prevPrev->next = p_segment;
									pUri->pathTail = p_segment;
								}
								if(pathOwned && p_walker->text.Len())
									SAlloc::F((char *)p_walker->text.P_First);
								delete p_walker;
								if(pathOwned && p_prev->text.Len())
									SAlloc::F((char *)p_prev->text.P_First);
								delete p_prev;
								p_walker = nextBackup;
							}
							else {
								if(p_walker->next) { // Prev is the first segment 
									pUri->pathHead = p_walker->next;
									p_walker->next->reserved = NULL;
									if(pathOwned && p_walker->text.Len())
										SAlloc::F((char *)p_walker->text.P_First);
									delete p_walker;
								}
								else {
									// Re-use segment for "" path segment to represent trailing slash, update tail 
									UriUri::PathSegment * const segment = p_walker;
									if(pathOwned && segment->text.Len())
										SAlloc::F((char *)segment->text.P_First);
									segment->text.P_First = UriSafeToPointTo;
									segment->text.P_AfterLast = UriSafeToPointTo;
									pUri->pathHead = segment;
									pUri->pathTail = segment;
								}
								if(pathOwned && p_prev->text.Len())
									SAlloc::F((char *)p_prev->text.P_First);
								delete p_prev;
								p_walker = nextBackup;
							}
						}
						else {
							UriUri::PathSegment * const nextBackup = p_walker->next;
							// First segment -> update head pointer 
							pUri->pathHead = p_walker->next;
							if(p_walker->next)
								p_walker->next->reserved = NULL;
							else
								pUri->pathTail = NULL; // Last segment -> update tail
							if(pathOwned && p_walker->text.Len())
								SAlloc::F((char *)p_walker->text.P_First);
							delete p_walker;
							p_walker = nextBackup;
						}
					}
				}
			}
			if(!removeSegment) {
				if(p_walker->next)
					p_walker->next->reserved = p_walker;
				else // Last segment -> update tail 
					pUri->pathTail = p_walker; 
				p_walker = p_walker->next;
			}
		} while(p_walker);
	}
	CATCH
		if(p_walker) {
			if(pathOwned && p_walker->text.Len())
				SAlloc::F((char *)p_walker->text.P_First);
			delete p_walker;
		}
		if(p_prev) {
			if(pathOwned && p_prev->text.Len())
				SAlloc::F((char *)p_prev->text.P_First);
			delete p_prev;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

// Properly removes "." and ".." path segments 
int UriRemoveDotSegmentsAbsolute(UriUri * uri)
{
	const int absolute = FALSE;
	return UriRemoveDotSegments(uri, absolute);
}

uchar FASTCALL UriHexdigToInt(char hexdig) 
{
	switch(hexdig) {
	    case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'):
			return (uchar)(9+hexdig-_UT('9'));
	    case _UT('a'): case _UT('b'): case _UT('c'): case _UT('d'): case _UT('e'): case _UT('f'):
			return (uchar)(15+hexdig-_UT('f'));
	    case _UT('A'): case _UT('B'): case _UT('C'): case _UT('D'): case _UT('E'): case _UT('F'):
			return (uchar)(15+hexdig-_UT('F'));
	    default:
			return 0;
	}
}

char FASTCALL UriHexToLetter(uint value) 
{
	// Uppercase recommended in section 2.1. of RFC 3986 http://tools.ietf.org/html/rfc3986#section-2.1
	return UriHexToLetterEx(value, TRUE);
}

char FASTCALL UriHexToLetterEx(uint value, int uppercase) 
{
	switch(value) {
	    case  0: return _UT('0');
	    case  1: return _UT('1');
	    case  2: return _UT('2');
	    case  3: return _UT('3');
	    case  4: return _UT('4');
	    case  5: return _UT('5');
	    case  6: return _UT('6');
	    case  7: return _UT('7');
	    case  8: return _UT('8');
	    case  9: return _UT('9');
	    case 10: return uppercase ? _UT('A') : _UT('a');
	    case 11: return uppercase ? _UT('B') : _UT('b');
	    case 12: return uppercase ? _UT('C') : _UT('c');
	    case 13: return uppercase ? _UT('D') : _UT('d');
	    case 14: return uppercase ? _UT('E') : _UT('e');
	    default: return uppercase ? _UT('F') : _UT('f');
	}
}
//
// Checks if a URI has the host component set
//
int UriIsHostSet(const UriUri * uri)
	{ return (uri && (uri->HostText.P_First || uri->HostData.ip4 || uri->HostData.ip6 || uri->HostData.ipFuture.P_First)); }
//
// Copies the path segment list from one URI to another.
//
int UriCopyPath(UriUri * dest, const UriUri * source)
{
	if(source->pathHead == NULL) {
		// No path component 
		dest->pathHead = NULL;
		dest->pathTail = NULL;
	}
	else {
		/* Copy list but not the text contained */
		UriUri::PathSegment * sourceWalker = source->pathHead;
		UriUri::PathSegment * destPrev = NULL;
		do {
			UriUri::PathSegment * cur = new UriUri::PathSegment(sourceWalker->text.P_First, sourceWalker->text.P_AfterLast);
			if(cur == NULL) {
				// Fix broken list 
				if(destPrev)
					destPrev->next = NULL;
				return FALSE; // Raises SAlloc::M error 
			}
			else {
				// From this functions usage we know that the dest URI cannot be uri->owner
				if(destPrev == NULL)
					dest->pathHead = cur; // First segment ever 
				else
					destPrev->next = cur;
				destPrev = cur;
				sourceWalker = sourceWalker->next;
			}
		} while(sourceWalker);
		dest->pathTail = destPrev;
		dest->pathTail->next = NULL;
	}
	dest->IsAbsolutePath = source->IsAbsolutePath;
	return TRUE;
}
//
// Copies the authority part of an URI over to another. 
//
int UriCopyAuthority(UriUri * dest, const UriUri * source)
{
	// From this functions usage we know that the dest URI cannot be uri->owner  
	dest->UserInfo = source->UserInfo; /* Copy userInfo */
	dest->HostText = source->HostText; /* Copy hostText */
	/* Copy hostData */
	if(source->HostData.ip4) {
		dest->HostData.ip4 = static_cast<UriUri::UriIp4 *>(SAlloc::M(sizeof(UriUri::UriIp4)));
		if(!dest->HostData.ip4) {
			return FALSE; // Raises SAlloc::M error 
		}
		*(dest->HostData.ip4) = *(source->HostData.ip4);
		dest->HostData.ip6 = NULL;
		dest->HostData.ipFuture.Clear();
	}
	else if(source->HostData.ip6) {
		dest->HostData.ip4 = NULL;
		dest->HostData.ip6 = static_cast<UriUri::UriIp6 *>(SAlloc::M(sizeof(UriUri::UriIp6)));
		if(!dest->HostData.ip6) {
			return FALSE; // Raises SAlloc::M error 
		}
		*(dest->HostData.ip6) = *(source->HostData.ip6);
		dest->HostData.ipFuture.Clear();
	}
	else {
		dest->HostData.ip4 = NULL;
		dest->HostData.ip6 = NULL;
		dest->HostData.ipFuture = source->HostData.ipFuture;
	}
	dest->PortText = source->PortText; /* Copy portText */
	return TRUE;
}

int UriFixAmbiguity(UriUri * uri)
{
	if(/* Case 1: absolute path, empty first segment */
	       (uri->IsAbsolutePath && uri->pathHead && uri->pathHead->text.Len() == 0)
	        /* Case 2: relative path, empty first and second segment */
	       || (!uri->IsAbsolutePath && uri->pathHead && uri->pathHead->next && uri->pathHead->text.Len() == 0) &&
	           (uri->pathHead->next->text.Len() == 0)) {
		/* NOOP */
	}
	else {
		return TRUE;
	}
	{
		UriUri::PathSegment * segment = new UriUri::PathSegment(UriConstPwd, UriConstPwd+1);
		if(segment == NULL)
			return FALSE; // Raises SAlloc::M error 
		else {
			// Insert "." segment in front 
			segment->next = uri->pathHead;
			uri->pathHead = segment;
		}
	}
	return TRUE;
}

void UriFixEmptyTrailSegment(UriUri * uri)
{
	/* Fix path if only one empty segment */
	if(!uri->IsAbsolutePath && !UriIsHostSet(uri) && uri->pathHead && !uri->pathHead->next && uri->pathHead->text.Len() == 0) {
		ZFREE(uri->pathHead);
		uri->pathTail = NULL;
	}
}
//
//
//
static void FASTCALL UriPreventLeakage(UriUri * uri, uint revertMask)
{
	if(revertMask&URI_NORMALIZE_SCHEME) {
		SAlloc::F((char *)uri->Scheme.P_First); // @badcast
		uri->Scheme.Clear();
	}
	if(revertMask&URI_NORMALIZE_USER_INFO) {
		SAlloc::F((char *)uri->UserInfo.P_First); // @badcast
		uri->UserInfo.Clear();
	}
	if(revertMask&URI_NORMALIZE_HOST) {
		if(uri->HostData.ipFuture.P_First) {
			// IPvFuture 
			SAlloc::F((char *)uri->HostData.ipFuture.P_First); // @badcast
			uri->HostData.ipFuture.Clear();
			uri->HostText.Clear();
		}
		else if(uri->HostText.P_First && !uri->HostData.ip4 && !uri->HostData.ip6) {
			// Regname 
			SAlloc::F((char *)uri->HostText.P_First); // @badcast
			uri->HostText.Clear();
		}
	}
	// NOTE: Port cannot happen! 
	if(revertMask&URI_NORMALIZE_PATH) {
		UriUri::PathSegment * p_walker = uri->pathHead;
		while(p_walker) {
			UriUri::PathSegment * const next = p_walker->next;
			if(p_walker->text.Len() > 0) {
				SAlloc::F((char *)p_walker->text.P_First); // @badcast
			}
			delete p_walker;
			p_walker = next;
		}
		uri->pathHead = NULL;
		uri->pathTail = NULL;
	}
	if(revertMask&URI_NORMALIZE_QUERY) {
		SAlloc::F((char *)uri->query.P_First); // @badcast
		uri->query.Clear();
	}
	if(revertMask&URI_NORMALIZE_FRAGMENT) {
		SAlloc::F((char *)uri->fragment.P_First); // @badcast
		uri->fragment.Clear();
	}
}

static int FASTCALL UriContainsUppercaseLetters(/*const char * pFirst, const char * afterLast*/const UriTextRange & rR)
{
	//if(pFirst && afterLast && (afterLast > pFirst)) {
	if(rR.P_First && rR.P_AfterLast && (rR.P_AfterLast > rR.P_First)) {
		for(const char * i = rR.P_First; i < rR.P_AfterLast; i++) {
			// 6.2.2.1 Case Normalization: uppercase letters in scheme or host 
			if((*i >= _UT('A')) &&(*i <= _UT('Z'))) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static int FASTCALL UriContainsUglyPercentEncoding(const char * pFirst, const char * afterLast)
{
	if(pFirst && afterLast && (afterLast > pFirst)) {
		const char * i = pFirst;
		for(; i+2 < afterLast; i++) {
			if(i[0] == _UT('%')) {
				/* 6.2.2.1 Case Normalization: *
				* lowercase percent-encodings */
				if(((i[1] >= _UT('a')) &&(i[1] <= _UT('f'))) || ((i[2] >= _UT('a')) &&(i[2] <= _UT('f')))) {
					return TRUE;
				}
				else {
					/* 6.2.2.2 Percent-Encoding Normalization: *
					* percent-encoded unreserved characters   */
					const uchar left = UriHexdigToInt(i[1]);
					const uchar right = UriHexdigToInt(i[2]);
					const int code = 16*left+right;
					if(uriIsUnreserved(code)) {
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

static void FASTCALL UriLowercaseInplace(/*const char * pFirst, const char * afterLast*/UriTextRange & rR)
{
	//if(pFirst && afterLast && (afterLast > pFirst)) {
	if(rR.P_First && rR.P_AfterLast && rR.Len() > 0) {
		char * i = (char *)rR.P_First; // @badcast
		const int lowerUpperDiff =(_UT('a')-_UT('A'));
		for(; i < rR.P_AfterLast; i++) {
			if((*i >= _UT('A')) &&(*i <=_UT('Z'))) {
				*i = (char)(*i+lowerUpperDiff);
			}
		}
	}
}

static int FASTCALL UriLowercaseMalloc(const char ** ppFirst, const char ** afterLast)
{
	int    ok = 1;
	const  int lowerUpperDiff =(_UT('a')-_UT('A'));
	char * buffer;
	int i = 0;
	if(!ppFirst || !afterLast || (*ppFirst == NULL) || (*afterLast == NULL)) {
		ok = 0;
	}
	else {
		int    lenInChars =(int)(*afterLast-*ppFirst);
		if(lenInChars < 0) {
			ok = 0;
		}
		else if(lenInChars > 0) {
			buffer = static_cast<char *>(SAlloc::M(lenInChars * sizeof(char)));
			if(!buffer) {
				ok = 0;
			}
			else {
				for(; i < lenInChars; i++) {
					if(((*ppFirst)[i] >= _UT('A')) &&((*ppFirst)[i] <=_UT('Z'))) {
						buffer[i] =(char)((*ppFirst)[i]+lowerUpperDiff);
					}
					else {
						buffer[i] =(*ppFirst)[i];
					}
				}
				*ppFirst = buffer;
				*afterLast = buffer+lenInChars;
			}
		}
	}
	return ok;
}
//
// NOTE: Implementation must stay inplace-compatible 
//
static void UriFixPercentEncodingEngine(const char * pInFirst, const char * pInAfterLast, const char * pOutFirst, const char ** ppOutAfterLast)
{
	char * p_write =(char *)pOutFirst;
	const int lenInChars =(int)(pInAfterLast-pInFirst);
	int i = 0;
	// All but last two 
	for(; i+2 < lenInChars; i++) {
		if(pInFirst[i] != _UT('%')) {
			p_write[0] = pInFirst[i];
			p_write++;
		}
		else {
			// 6.2.2.2 Percent-Encoding Normalization: percent-encoded unreserved characters 
			const char one = pInFirst[i+1];
			const char two = pInFirst[i+2];
			const uchar left = UriHexdigToInt(one);
			const uchar right = UriHexdigToInt(two);
			const int code = 16*left+right;
			if(uriIsUnreserved(code)) {
				p_write[0] =(char)(code);
				p_write++;
			}
			else {
				// 6.2.2.1 Case Normalization: lowercase percent-encodings 
				p_write[0] = _UT('%');
				p_write[1] = UriHexToLetter(left);
				p_write[2] = UriHexToLetter(right);
				p_write += 3;
			}
			i += 2; // For the two chars of the percent group we just ate
		}
	}
	// Last two 
	for(; i < lenInChars; i++) {
		p_write[0] = pInFirst[i];
		p_write++;
	}
	*ppOutAfterLast = p_write;
}

static void FASTCALL UriFixPercentEncodingInplace(const char * first, const char ** afterLast) 
{
	// Death checks 
	if(first && afterLast && *afterLast) {
		// Fix inplace 
		UriFixPercentEncodingEngine(first, *afterLast, first, afterLast);
	}
}

UriTextRange::UriTextRange() : P_First(0), P_AfterLast(0)
{
}

UriTextRange::UriTextRange(const char * pFirst, const char * pAfterLast) : P_First(pFirst), P_AfterLast(pAfterLast)
{
}

void UriTextRange::Clear()
{
	P_First = 0;
	P_AfterLast = 0;
}

UriTextRange & FASTCALL UriTextRange::operator = (const UriTextRange & rS)
{
	P_First = rS.P_First;
	P_AfterLast = rS.P_AfterLast;
	return *this;
}

int UriTextRange::Len() const
{
	return (int)(P_AfterLast - P_First);
}

/* @construction
int UriTextRange::FixPercentEncodingMalloc()
{
	// Death checks 
	if(!P_First || !P_AfterLast) {
		return FALSE;
	}
	else {
		// Old text length 
		int    lenInChars = Len();
		if(lenInChars == 0)
			return TRUE;
		else if(lenInChars < 0)
			return FALSE;
		else {
			// New buffer 
			char * p_buffer = (char *)SAlloc::M(lenInChars * sizeof(char));
			if(!p_buffer)
				return FALSE;
			else {
				// Fix on copy 
				UriFixPercentEncodingEngine(*ppFirst, *ppAfterLast, p_buffer, ppAfterLast);
				*ppFirst = p_buffer;
				return TRUE;
			}
		}
	}
}
*/

static int FASTCALL UriFixPercentEncodingMalloc(const char ** ppFirst, const char ** ppAfterLast)
{
	// Death checks 
	if(!ppFirst || !ppAfterLast || (*ppFirst == NULL) || (*ppAfterLast == NULL)) {
		return FALSE;
	}
	else {
		// Old text length 
		int lenInChars = static_cast<int>(*ppAfterLast-*ppFirst);
		if(lenInChars == 0)
			return TRUE;
		else if(lenInChars < 0)
			return FALSE;
		else {
			// New buffer 
			char * p_buffer = static_cast<char *>(SAlloc::M(lenInChars * sizeof(char)));
			if(!p_buffer)
				return FALSE;
			else {
				// Fix on copy 
				UriFixPercentEncodingEngine(*ppFirst, *ppAfterLast, p_buffer, ppAfterLast);
				*ppFirst = p_buffer;
				return TRUE;
			}
		}
	}
}

static int FASTCALL UriMakeRangeOwner(uint * doneMask, uint maskTest, UriTextRange * range)
{
	if(((*doneMask&maskTest) == 0) && range->P_First && range->P_AfterLast) {
		const int len_in_chars = static_cast<int>(range->Len());
		if(len_in_chars > 0) {
			const int lenInBytes = len_in_chars * sizeof(char);
			char * p_dup = static_cast<char *>(SAlloc::M(lenInBytes));
			if(p_dup == NULL) 
				return FALSE; // Raises SAlloc::M error 
			else {
				memcpy(dup, range->P_First, lenInBytes);
				range->P_First = p_dup;
				range->P_AfterLast = p_dup+len_in_chars;
				*doneMask |= maskTest;
			}
		}
	}
	return TRUE;
}

static int FASTCALL UriMakeOwner(UriUri * uri, uint * doneMask)
{
	int    ok = 1;
	UriUri::PathSegment * p_walker = uri->pathHead;
	THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_SCHEME, &(uri->Scheme)));
	THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_USER_INFO, &(uri->UserInfo)));
	THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_QUERY, &(uri->query)));
	THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_FRAGMENT, &(uri->fragment)));
	// Host 
	if(!(*doneMask & URI_NORMALIZE_HOST)) {
		if(!uri->HostData.ip4 && !uri->HostData.ip6) {
			if(uri->HostData.ipFuture.P_First) {
				// IPvFuture 
				THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_HOST, &(uri->HostData.ipFuture)));
				uri->HostText.P_First = uri->HostData.ipFuture.P_First;
				uri->HostText.P_AfterLast = uri->HostData.ipFuture.P_AfterLast;
			}
			else if(uri->HostText.P_First) {
				// Regname 
				THROW(UriMakeRangeOwner(doneMask, URI_NORMALIZE_HOST, &(uri->HostText)));
			}
		}
	}
	// Path 
	if(!(*doneMask & URI_NORMALIZE_PATH)) {
		for(; p_walker; p_walker = p_walker->next) {
			if(!UriMakeRangeOwner(doneMask, 0, &(p_walker->text))) {
				// Kill path to one before walker 
				for(UriUri::PathSegment * p_ranger = uri->pathHead; p_ranger->next != p_walker;) {
					UriUri::PathSegment * const next = p_ranger->next;
					if(p_ranger->text.P_First && p_ranger->text.P_AfterLast && p_ranger->text.Len() > 0) {
						SAlloc::F((char *)p_ranger->text.P_First); // @badcast
						delete p_ranger;
					}
					p_ranger = next;
				}
				// Kill path from walker 
				while(p_walker) {
					UriUri::PathSegment * const next = p_walker->next;
					delete p_walker;
					p_walker = next;
				}
				uri->pathHead = NULL;
				uri->pathTail = NULL;
				CALLEXCEPT(); // Raises error 
			}
		}
		*doneMask |= URI_NORMALIZE_PATH;
	}
	// Port text, must come last so we don't have to undo that one if it fails.
	// Otherwise we would need and extra enum flag for it although the port      
	// cannot go unnormalized...  
	THROW(UriMakeRangeOwner(doneMask, 0, &(uri->PortText)));
	CATCHZOK
	return ok;
}

static int FASTCALL UriNormalizeSyntaxEngine(UriUri * uri, uint inMask, uint * outMask)
{
	uint doneMask = URI_NORMALIZED;
	if(!uri) {
		if(outMask) {
			*outMask = URI_NORMALIZED;
			return SLERR_SUCCESS;
		}
		else
			return SLERR_URI_NULL;
	}
	if(outMask)
		*outMask = URI_NORMALIZED; // Reset mask 
	else if(inMask == URI_NORMALIZED)
		return SLERR_SUCCESS; // Nothing to do 
	// Scheme, host 
	if(outMask) {
		const int normalizeScheme = UriContainsUppercaseLetters(uri->Scheme);
		const int normalizeHostCase = UriContainsUppercaseLetters(uri->HostText);
		if(normalizeScheme)
			*outMask |= URI_NORMALIZE_SCHEME;
		if(normalizeHostCase)
			*outMask |= URI_NORMALIZE_HOST;
		else {
			const int normalizeHostPrecent = UriContainsUglyPercentEncoding(uri->HostText.P_First, uri->HostText.P_AfterLast);
			if(normalizeHostPrecent)
				*outMask |= URI_NORMALIZE_HOST;
		}
	}
	else {
		// Scheme 
		if((inMask&URI_NORMALIZE_SCHEME) && uri->Scheme.P_First) {
			if(uri->IsOwner) {
				UriLowercaseInplace(uri->Scheme);
			}
			else {
				if(!UriLowercaseMalloc(&(uri->Scheme.P_First), &(uri->Scheme.P_AfterLast))) {
					UriPreventLeakage(uri, doneMask);
					return SLERR_NOMEM;
				}
				doneMask |= URI_NORMALIZE_SCHEME;
			}
		}
		// Host 
		if(inMask&URI_NORMALIZE_HOST) {
			if(uri->HostData.ipFuture.P_First) {
				// IPvFuture 
				if(uri->IsOwner)
					UriLowercaseInplace(uri->HostData.ipFuture);
				else {
					if(!UriLowercaseMalloc(&(uri->HostData.ipFuture.P_First), &(uri->HostData.ipFuture.P_AfterLast))) {
						UriPreventLeakage(uri, doneMask);
						return SLERR_NOMEM;
					}
					doneMask |= URI_NORMALIZE_HOST;
				}
				uri->HostText.P_First = uri->HostData.ipFuture.P_First;
				uri->HostText.P_AfterLast = uri->HostData.ipFuture.P_AfterLast;
			}
			else if(uri->HostText.P_First && !uri->HostData.ip4 && !uri->HostData.ip6) {
				// Regname 
				if(uri->IsOwner)
					UriFixPercentEncodingInplace(uri->HostText.P_First, &(uri->HostText.P_AfterLast));
				else {
					if(!UriFixPercentEncodingMalloc(&(uri->HostText.P_First), &(uri->HostText.P_AfterLast))) {
						UriPreventLeakage(uri, doneMask);
						return SLERR_NOMEM;
					}
					doneMask |= URI_NORMALIZE_HOST;
				}
				UriLowercaseInplace(uri->HostText);
			}
		}
	}
	/* User info */
	if(outMask) {
		const int normalizeUserInfo = UriContainsUglyPercentEncoding(uri->UserInfo.P_First, uri->UserInfo.P_AfterLast);
		if(normalizeUserInfo) {
			*outMask |= URI_NORMALIZE_USER_INFO;
		}
	}
	else {
		if((inMask&URI_NORMALIZE_USER_INFO) && uri->UserInfo.P_First) {
			if(uri->IsOwner)
				UriFixPercentEncodingInplace(uri->UserInfo.P_First, &(uri->UserInfo.P_AfterLast));
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->UserInfo.P_First), &(uri->UserInfo.P_AfterLast))) {
					UriPreventLeakage(uri, doneMask);
					return SLERR_NOMEM;
				}
				doneMask |= URI_NORMALIZE_USER_INFO;
			}
		}
	}
	// Path 
	if(outMask) {
		for(const UriUri::PathSegment * p_walker = uri->pathHead; p_walker; p_walker = p_walker->next) {
			const char * const first = p_walker->text.P_First;
			const char * const afterLast = p_walker->text.P_AfterLast;
			if(first && afterLast && (afterLast > first) && ((((afterLast-first) == 1) &&(first[0] == _UT('.'))) ||
				(((afterLast-first) == 2) && (first[0] == _UT('.')) && (first[1] == _UT('.'))) || UriContainsUglyPercentEncoding(first, afterLast))) {
				*outMask |= URI_NORMALIZE_PATH;
				break;
			}
		}
	}
	else if(inMask&URI_NORMALIZE_PATH) {
		const int relative = (!uri->Scheme.P_First && !uri->IsAbsolutePath) ? TRUE : FALSE;
		// Fix percent-encoding for each segment 
		UriUri::PathSegment * p_walker = uri->pathHead;
		if(uri->IsOwner) {
			while(p_walker) {
				UriFixPercentEncodingInplace(p_walker->text.P_First, &(p_walker->text.P_AfterLast));
				p_walker = p_walker->next;
			}
		}
		else {
			while(p_walker) {
				if(!UriFixPercentEncodingMalloc(&p_walker->text.P_First, &p_walker->text.P_AfterLast)) {
					UriPreventLeakage(uri, doneMask);
					return SLERR_NOMEM;
				}
				p_walker = p_walker->next;
			}
			doneMask |= URI_NORMALIZE_PATH;
		}
		// 6.2.2.3 Path Segment Normalization 
		if(!UriRemoveDotSegmentsEx(uri, relative, uri->IsOwner || ((doneMask&URI_NORMALIZE_PATH) != 0))) {
			UriPreventLeakage(uri, doneMask);
			return SLERR_NOMEM;
		}
		UriFixEmptyTrailSegment(uri);
	}
	// Query, fragment 
	if(outMask) {
		const int normalizeQuery = UriContainsUglyPercentEncoding(uri->query.P_First, uri->query.P_AfterLast);
		const int normalizeFragment = UriContainsUglyPercentEncoding(uri->fragment.P_First, uri->fragment.P_AfterLast);
		if(normalizeQuery)
			*outMask |= URI_NORMALIZE_QUERY;
		if(normalizeFragment)
			*outMask |= URI_NORMALIZE_FRAGMENT;
	}
	else {
		/* Query */
		if((inMask&URI_NORMALIZE_QUERY) && uri->query.P_First) {
			if(uri->IsOwner)
				UriFixPercentEncodingInplace(uri->query.P_First, &(uri->query.P_AfterLast));
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->query.P_First), &(uri->query.P_AfterLast))) {
					UriPreventLeakage(uri, doneMask);
					return SLERR_NOMEM;
				}
				doneMask |= URI_NORMALIZE_QUERY;
			}
		}
		/* Fragment */
		if((inMask&URI_NORMALIZE_FRAGMENT) && uri->fragment.P_First) {
			if(uri->IsOwner)
				UriFixPercentEncodingInplace(uri->fragment.P_First, &(uri->fragment.P_AfterLast));
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->fragment.P_First), &(uri->fragment.P_AfterLast))) {
					UriPreventLeakage(uri, doneMask);
					return SLERR_NOMEM;
				}
				doneMask |= URI_NORMALIZE_FRAGMENT;
			}
		}
	}
	// Dup all not duped yet 
	if(!outMask && !uri->IsOwner) {
		if(!UriMakeOwner(uri, &doneMask)) {
			UriPreventLeakage(uri, doneMask);
			return SLERR_NOMEM;
		}
		uri->IsOwner = TRUE;
	}
	return SLERR_SUCCESS;
}

uint FASTCALL UriNormalizeSyntaxMaskRequired(const UriUri * uri)
{
	uint res;
 #if defined(__GNUC__) &&((__GNUC__ > 4) || ((__GNUC__ == 4) && defined(__GNUC_MINOR__) &&(__GNUC_MINOR__ >= 2)))
	// Slower code that fixes a warning, not sure if this is a smart idea 
	UriUri writeableClone;
	memcpy(&writeableClone, uri, 1*sizeof(UriUri));
	UriNormalizeSyntaxEngine(&writeableClone, 0, &res);
 #else
	UriNormalizeSyntaxEngine(const_cast<UriUri *>(uri), 0, &res); // @badcast
 #endif
	return res;
}

int FASTCALL UriNormalizeSyntaxEx(UriUri * uri, uint mask)
{
	return UriNormalizeSyntaxEngine(uri, mask, 0);
}

int FASTCALL UriNormalizeSyntax(UriUri * uri)
{
	return UriNormalizeSyntaxEx(uri, static_cast<uint>(-1));
}
//
//
//
static int UriToStringEngine(char * dest, const UriUri * uri, int maxChars, int * charsWritten, int * charsRequired);

int FASTCALL UriToStringCharsRequired(const UriUri * uri, int * charsRequired)
{
	const int MAX_CHARS =(static_cast<uint>(-1))>>1;
	return UriToStringEngine(NULL, uri, MAX_CHARS, NULL, charsRequired);
}

int UriToString(char * dest, const UriUri*uri, int maxChars, int * charsWritten)
{
	return UriToStringEngine(dest, uri, maxChars, charsWritten, 0);
}

static int UriToStringEngine(char * dest, const UriUri*uri, int maxChars, int * charsWritten, int * charsRequired)
{
	int written = 0;
	if((uri == NULL) ||((dest == NULL) &&(charsRequired == NULL))) {
		ASSIGN_PTR(charsWritten, 0);
		return SLERR_URI_NULL;
	}
	if(maxChars < 1) {
		ASSIGN_PTR(charsWritten, 0);
		return SLERR_URI_TOSTRING_TOO_LONG;
	}
	maxChars--; // So we don't have to substract 1 for '\0' all the time 
	// [01/19] result = "" 
	if(dest)
		dest[0] = _UT('\0');
	else {
		(*charsRequired) = 0;
	}
	// [02/19] if defined(scheme) then 
	if(uri->Scheme.P_First) {
		// [03/19] append scheme to result; 
		const int charsToWrite = static_cast<int>(uri->Scheme.P_AfterLast-uri->Scheme.P_First);
		if(dest) {
			if((written+charsToWrite) <= maxChars) {
				memcpy(dest+written, uri->Scheme.P_First, charsToWrite*sizeof(char));
				written += charsToWrite;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += charsToWrite;
		}
		/* [04/19]		append ":" to result; */
		if(dest) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT(":"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
		/* [05/19]	endif; */
	}
	/* [06/19]	if defined(authority) then */
	if(UriIsHostSet(uri)) {
		/* [07/19]		append "//" to result; */
		if(dest) {
			if(written+2 <= maxChars) {
				memcpy(dest+written, _UT("//"), 2*sizeof(char));
				written += 2;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 2;
		}
		// [08/19] append authority to result; 
		// UserInfo 
		if(uri->UserInfo.P_First) {
			const int charsToWrite = static_cast<int>(uri->UserInfo.P_AfterLast-uri->UserInfo.P_First);
			if(dest) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->UserInfo.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("@"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite+1;
			}
		}
		/* Host */
		if(uri->HostData.ip4) {
			/* IPv4 */
			int i = 0;
			for(; i < 4; i++) {
				const uchar value = uri->HostData.ip4->data[i];
				const int charsToWrite =(value > 99) ? 3 :((value > 9) ? 2 : 1);
				if(dest) {
					if(written+charsToWrite <= maxChars) {
						char text[4];
						if(value > 99) {
							text[0] = _UT('0')+(value/100);
							text[1] = _UT('0')+((value%100)/10);
							text[2] = _UT('0')+(value%10);
						}
						else if(value > 9) {
							text[0] = _UT('0')+(value/10);
							text[1] = _UT('0')+(value%10);
						}
						else {
							text[0] = _UT('0')+value;
						}
						text[charsToWrite] = _UT('\0');
						memcpy(dest+written, text, charsToWrite*sizeof(char));
						written += charsToWrite;
					}
					else {
						dest[0] = _UT('\0');
						ASSIGN_PTR(charsWritten, 0);
						return SLERR_URI_TOSTRING_TOO_LONG;
					}
					if(i < 3) {
						if(written+1 <= maxChars) {
							memcpy(dest+written, _UT("."), 1*sizeof(char));
							written += 1;
						}
						else {
							dest[0] = _UT('\0');
							ASSIGN_PTR(charsWritten, 0);
							return SLERR_URI_TOSTRING_TOO_LONG;
						}
					}
				}
				else {
					(*charsRequired) += charsToWrite+1;
				}
			}
		}
		else if(uri->HostData.ip6) {
			/* IPv6 */
			int i = 0;
			if(dest) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("["), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1;
			}
			for(; i < 16; i++) {
				const uchar value = uri->HostData.ip6->data[i];
				if(dest) {
					if(written+2 <= maxChars) {
						char text[3];
						text[0] = UriHexToLetterEx(value/16, FALSE);
						text[1] = UriHexToLetterEx(value%16, FALSE);
						text[2] = _UT('\0');
						memcpy(dest+written, text, 2*sizeof(char));
						written += 2;
					}
					else {
						dest[0] = _UT('\0');
						ASSIGN_PTR(charsWritten, 0);
						return SLERR_URI_TOSTRING_TOO_LONG;
					}
				}
				else {
					(*charsRequired) += 2;
				}
				if(((i&1) == 1) &&(i < 15)) {
					if(dest) {
						if(written+1 <= maxChars) {
							memcpy(dest+written, _UT(":"), 1*sizeof(char));
							written += 1;
						}
						else {
							dest[0] = _UT('\0');
							ASSIGN_PTR(charsWritten, 0);
							return SLERR_URI_TOSTRING_TOO_LONG;
						}
					}
					else {
						(*charsRequired) += 1;
					}
				}
			}
			if(dest) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("]"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1;
			}
		}
		else if(uri->HostData.ipFuture.P_First) {
			/* IPvFuture */
			const int charsToWrite = static_cast<int>(uri->HostData.ipFuture.P_AfterLast-uri->HostData.ipFuture.P_First);
			if(dest) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("["), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->HostData.ipFuture.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("]"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1+charsToWrite+1;
			}
		}
		else if(uri->HostText.P_First) {
			// Regname 
			const int charsToWrite = static_cast<int>(uri->HostText.Len());
			if(dest) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->HostText.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		if(uri->PortText.P_First) { // Port 
			const int charsToWrite = (int)uri->PortText.Len();
			if(dest) {
				/* Leading ':' */
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT(":"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
				/* Port number */
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->PortText.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1+charsToWrite;
			}
		}
		/* [09/19]	endif; */
	}
	// [10/19] append path to result; 
	// Slash needed here? 
	if(uri->IsAbsolutePath || (uri->pathHead && UriIsHostSet(uri))) {
		if(dest) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("/"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
	}
	if(uri->pathHead) {
		UriUri::PathSegment * walker = uri->pathHead;
		do {
			const int charsToWrite =(int)(walker->text.P_AfterLast-walker->text.P_First);
			if(dest) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, walker->text.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
			/* Not last segment -> append slash */
			if(walker->next) {
				if(dest) {
					if(written+1 <= maxChars) {
						memcpy(dest+written, _UT("/"), 1*sizeof(char));
						written += 1;
					}
					else {
						dest[0] = _UT('\0');
						ASSIGN_PTR(charsWritten, 0);
						return SLERR_URI_TOSTRING_TOO_LONG;
					}
				}
				else {
					(*charsRequired) += 1;
				}
			}
			walker = walker->next;
		} while(walker != NULL);
	}
	/* [11/19]	if defined(query) then */
	if(uri->query.P_First != NULL) {
		/* [12/19]		append "?" to result; */
		if(dest) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("?"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
		/* [13/19]		append query to result; */
		{
			const int charsToWrite = static_cast<int>(uri->query.P_AfterLast-uri->query.P_First);
			if(dest) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->query.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		/* [14/19]	endif; */
	}
	/* [15/19]	if defined(fragment) then */
	if(uri->fragment.P_First != NULL) {
		/* [16/19]		append "#" to result; */
		if(dest) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("#"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				ASSIGN_PTR(charsWritten, 0);
				return SLERR_URI_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
		/* [17/19]		append fragment to result; */
		{
			const int charsToWrite = static_cast<int>(uri->fragment.P_AfterLast-uri->fragment.P_First);
			if(dest) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->fragment.P_First, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					ASSIGN_PTR(charsWritten, 0);
					return SLERR_URI_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		/* [18/19]	endif; */
	}
	/* [19/19]	return result; */
	if(dest) {
		dest[written++] = _UT('\0');
		ASSIGN_PTR(charsWritten, written);
	}
	return SLERR_SUCCESS;
}
// 
// [ipFourAddress]->[decOctet]<.>[decOctet]<.>[decOctet]<.>[decOctet]
// 
int UriParseIpFourAddress(uchar * pOctetOutput, const char * pFirst, const char * pAfterLast)
{
	class UriIp4Parser {
	public:
		UriIp4Parser() : stackCount(0), stackOne(0), stackTwo(0), stackThree(0)
		{
		}
		// 
		// [decOctet]-><0>
		// [decOctet]-><1>[decOctetOne]
		// [decOctet]-><2>[decOctetTwo]
		// [decOctet]-><3>[decOctetThree]
		// [decOctet]-><4>[decOctetThree]
		// [decOctet]-><5>[decOctetThree]
		// [decOctet]-><6>[decOctetThree]
		// [decOctet]-><7>[decOctetThree]
		// [decOctet]-><8>[decOctetThree]
		// [decOctet]-><9>[decOctetThree]
		// 
		const char * UriParseDecOctet(const char * pFirst, const char * pAfterLast)
		{
			if(pFirst >= pAfterLast)
				return NULL;
			else {
				switch(*pFirst) {
					case _UT('0'):
						uriPushToStack(0);
						return pFirst+1;
					case _UT('1'):
						uriPushToStack(1);
						return UriParseDecOctetOne(pFirst+1, pAfterLast);
					case _UT('2'):
						uriPushToStack(2);
						return UriParseDecOctetTwo(pFirst+1, pAfterLast);
					case _UT('3'):
					case _UT('4'):
					case _UT('5'):
					case _UT('6'):
					case _UT('7'):
					case _UT('8'):
					case _UT('9'):
						uriPushToStack((uchar)(9+*pFirst-_UT('9')));
						return UriParseDecOctetThree(pFirst+1, pAfterLast);
					default:
						return NULL;
				}
			}
		}
		void FASTCALL uriStackToOctet(uchar * pOctet)
		{
			switch(stackCount) {
				case 1: *pOctet = stackOne; break;
				case 2: *pOctet = (stackOne*10 + stackTwo); break;
				case 3: *pOctet = (stackOne*100 + stackTwo*10 + stackThree); break;
				default: break;
			}
			stackCount = 0;
		}
	private:
		void FASTCALL uriPushToStack(uchar digit)
		{
			switch(stackCount) {
				case 0:
					stackOne = digit;
					stackCount = 1;
					break;
				case 1:
					stackTwo = digit;
					stackCount = 2;
					break;
				case 2:
					stackThree = digit;
					stackCount = 3;
					break;
				default:
					break;
			}
		}
		// 
		// [decOctetThree]-><NULL>
		// [decOctetThree]->[DIGIT]
		// 
		const char * UriParseDecOctetThree(const char * first, const char * afterLast)
		{
			if(first >= afterLast) {
				return afterLast;
			}
			else {
				switch(*first) {
					case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'):
						uriPushToStack((uchar)(9+*first-_UT('9')));
						return first+1;
					default:
						return first;
				}
			}
		}
		// 
		// [decOctetOne]-><NULL>
		// [decOctetOne]->[DIGIT][decOctetThree]
		// 
		const char * UriParseDecOctetOne(const char * pFirst, const char * pAfterLast)
		{
			if(pFirst >= pAfterLast)
				return pAfterLast;
			else {
				switch(*pFirst) {
					case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'):
					case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9'):
						uriPushToStack((uchar)(9+*pFirst-_UT('9')));
						return UriParseDecOctetThree(pFirst+1, pAfterLast);
					default:
						return pFirst;
				}
			}
		}
		// 
		// [decOctetFour]-><NULL>
		// [decOctetFour]-><0>
		// [decOctetFour]-><1>
		// [decOctetFour]-><2>
		// [decOctetFour]-><3>
		// [decOctetFour]-><4>
		// [decOctetFour]-><5>
		// 
		const char * UriParseDecOctetFour(const char * first, const char * afterLast)
		{
			if(first >= afterLast)
				return afterLast;
			else {
				switch(*first) {
					case _UT('0'): case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): case _UT('5'):
						uriPushToStack((uchar)(9+*first-_UT('9')));
						return first+1;
					default:
						return first;
				}
			}
		}
		// 
		// [decOctetTwo]-><NULL>
		// [decOctetTwo]-><0>[decOctetThree]
		// [decOctetTwo]-><1>[decOctetThree]
		// [decOctetTwo]-><2>[decOctetThree]
		// [decOctetTwo]-><3>[decOctetThree]
		// [decOctetTwo]-><4>[decOctetThree]
		// [decOctetTwo]-><5>[decOctetFour]
		// [decOctetTwo]-><6>
		// [decOctetTwo]-><7>
		// [decOctetTwo]-><8>
		// [decOctetTwo]-><9>
		// 
		const char * UriParseDecOctetTwo(const char * pFirst, const char * pAfterLast)
		{
			if(pFirst >= pAfterLast)
				return pAfterLast;
			else {
				switch(*pFirst) {
					case _UT('0'):
					case _UT('1'):
					case _UT('2'):
					case _UT('3'):
					case _UT('4'):
						uriPushToStack((uchar)(9+*pFirst-_UT('9')));
						return UriParseDecOctetThree(pFirst+1, pAfterLast);
					case _UT('5'):
						uriPushToStack(5);
						return UriParseDecOctetFour(pFirst+1, pAfterLast);
					case _UT('6'):
					case _UT('7'):
					case _UT('8'):
					case _UT('9'):
						uriPushToStack((uchar)(9+*pFirst-_UT('9')));
						return (pFirst+1);
					default:
						return pFirst;
				}
			}
		}
		uchar stackCount;
		uchar stackOne;
		uchar stackTwo;
		uchar stackThree;
	};
	// Essential checks 
	if(!pOctetOutput || !pFirst || (pAfterLast <= pFirst)) {
		SLS.SetError(SLERR_URI_SYNTAX, pFirst);
		return SLERR_URI_SYNTAX;
	}
	else {
		UriIp4Parser parser;
		// Octet #1 
		const char * p_after = parser.UriParseDecOctet(pFirst, pAfterLast);
		if(!p_after || (p_after >= pAfterLast) || (*p_after != _UT('.'))) {
			SLS.SetError(SLERR_URI_SYNTAX, pFirst);
			return SLERR_URI_SYNTAX;
		}
		parser.uriStackToOctet(pOctetOutput);
		// Octet #2 
		p_after = parser.UriParseDecOctet(p_after+1, pAfterLast);
		if(!p_after || (p_after >= pAfterLast) || (*p_after != _UT('.'))) {
			SLS.SetError(SLERR_URI_SYNTAX, pFirst);
			return SLERR_URI_SYNTAX;
		}
		parser.uriStackToOctet(pOctetOutput+1);
		// Octet #3 
		p_after = parser.UriParseDecOctet(p_after+1, pAfterLast);
		if(!p_after || (p_after >= pAfterLast) || (*p_after != _UT('.'))) {
			SLS.SetError(SLERR_URI_SYNTAX, pFirst);
			return SLERR_URI_SYNTAX;
		}
		parser.uriStackToOctet(pOctetOutput+2);
		// Octet #4 
		p_after = parser.UriParseDecOctet(p_after+1, pAfterLast);
		if(p_after != pAfterLast) {
			SLS.SetError(SLERR_URI_SYNTAX, pFirst);
			return SLERR_URI_SYNTAX;
		}
		parser.uriStackToOctet(pOctetOutput+3);
		return SLERR_SUCCESS;
	}
}
//
//
//
 #define URI_SET_DIGIT _UT('0') : case _UT('1'): case _UT('2'): case _UT('3'): case _UT('4'): case _UT('5'): case _UT('6'): case _UT('7'): case _UT('8'): case _UT('9')
 #define URI_SET_HEX_LETTER_UPPER _UT('A'): case _UT('B'): case _UT('C'): case _UT('D'): case _UT('E'): case _UT('F')
 #define URI_SET_HEX_LETTER_LOWER _UT('a'): case _UT('b'): case _UT('c'): case _UT('d'): case _UT('e'): case _UT('f')
 #define URI_SET_HEXDIG URI_SET_DIGIT: case URI_SET_HEX_LETTER_UPPER: case URI_SET_HEX_LETTER_LOWER
 #define URI_SET_ALPHA URI_SET_HEX_LETTER_UPPER: \
    case URI_SET_HEX_LETTER_LOWER: \
    case _UT('g'): case _UT('G'): case _UT('h'): case _UT('H'): case _UT('i'): case _UT('I'): case _UT('j'): case _UT('J'): \
    case _UT('k'): case _UT('K'): case _UT('l'): case _UT('L'): case _UT('m'): case _UT('M'): case _UT('n'): case _UT('N'): \
    case _UT('o'): case _UT('O'): case _UT('p'): case _UT('P'): case _UT('q'): case _UT('Q'): case _UT('r'): case _UT('R'): \
    case _UT('s'): case _UT('S'): case _UT('t'): case _UT('T'): case _UT('u'): case _UT('U'): case _UT('v'): case _UT('V'): \
    case _UT('w'): case _UT('W'): case _UT('x'): case _UT('X'): case _UT('y'): case _UT('Y'): case _UT('z'): case _UT('Z')

UriParserState::UriParserState()
{
	Clear();
}

void UriParserState::Clear()
{
	P_Uri = 0;
	ErrorCode = 0;
	P_ErrorPos = 0;
	P_Reserved = 0;
}

void UriParserState::Reset()
{
	UriUri * const p_uri_backup = P_Uri;
	Clear();
	P_Uri = p_uri_backup;
}

int FASTCALL UriParserState::PushPathSegment(const char * first, const char * afterLast)
{
	int    ok = TRUE;
	UriUri::PathSegment * segment = new UriUri::PathSegment(0, 0);
	if(!segment)
		ok = FALSE; // Raises SAlloc::M error
	else {
		//memzero(segment, sizeof(*segment));
		if(first == afterLast) {
			segment->text.P_First = UriSafeToPointTo;
			segment->text.P_AfterLast = UriSafeToPointTo;
		}
		else {
			segment->text.P_First = first;
			segment->text.P_AfterLast = afterLast;
		}
		if(P_Uri->pathHead == NULL) { // First segment ever?
			// First segement ever, set head and tail
			P_Uri->pathHead = segment;
			P_Uri->pathTail = segment;
		}
		else {
			// Append, update tail
			P_Uri->pathTail->next = segment;
			P_Uri->pathTail = segment;
		}
	}
	return ok;
}

const char * FASTCALL UriParserState::StopSyntax(const char * pErrorPos)
{
	CALLPTRMEMB(P_Uri, Destroy());
	P_ErrorPos = pErrorPos;
	SLS.SetAddedMsgString(pErrorPos); // @v10.3.10
	ErrorCode = SLERR_URI_SYNTAX;
	return 0;
}

void UriParserState::StopMalloc()
{
	CALLPTRMEMB(P_Uri, Destroy());
	P_ErrorPos = NULL;
	ErrorCode = SLERR_NOMEM;
}
// 
// [authority]-><[>[ipLit2][authorityTwo]
// [authority]->[ownHostUserInfoNz]
// [authority]-><NULL>
// 
const char * FASTCALL UriParserState::ParseAuthority(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast) { // "" regname host 
		P_Uri->HostText.P_First = UriSafeToPointTo;
		P_Uri->HostText.P_AfterLast = UriSafeToPointTo;
		p_ret = pAfterLast;
	}
	else {
		switch(*pFirst) {
			case _UT('['):
				{
					const char * const p_after_ip_lit2 = ParseIpLit2(pFirst+1, pAfterLast);
					if(p_after_ip_lit2) {
						P_Uri->HostText.P_First = pFirst+1; // HOST BEGIN 
						p_ret = ParseAuthorityTwo(p_after_ip_lit2, pAfterLast);
					}
				}
				break;
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				P_Uri->UserInfo.P_First = pFirst; /* USERINFO BEGIN */
				p_ret = ParseOwnHostUserInfoNz(pFirst, pAfterLast);
				break;
			default: // "" regname host 
				P_Uri->HostText.P_First = UriSafeToPointTo;
				P_Uri->HostText.P_AfterLast = UriSafeToPointTo;
				p_ret = pFirst;
				break;
		}
	}
	return p_ret;
}
/*
 * [authorityTwo]-><:>[port]
 * [authorityTwo]-><NULL>
 */
const char * FASTCALL UriParserState::ParseAuthorityTwo(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast)
		p_ret = pAfterLast;
	else if(*pFirst == _UT(':')) {
		const char * const p_after_port = ParsePort(pFirst+1, pAfterLast);
		if(p_after_port) {
			P_Uri->PortText.P_First = pFirst+1; // PORT BEGIN 
			P_Uri->PortText.P_AfterLast = p_after_port; // PORT END 
			p_ret = p_after_port;
		}
	}
	else
		p_ret = pFirst;
	return p_ret;
}
/*
 * [hexZero]->[HEXDIG][hexZero]
 * [hexZero]-><NULL>
 */
const char * FASTCALL UriParserState::ParseHexZero(const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(ishex(*first))
		return ParseHexZero(first+1, afterLast); // @recursion
	else
		return first;
}
/*
 * [hierPart]->[pathRootless]
 * [hierPart]-></>[partHelperTwo]
 * [hierPart]-><NULL>
 */
const char * FASTCALL UriParserState::ParseHierPart(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast)
		return afterLast;
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				return ParsePathRootless(pFirst, afterLast);
			case _UT('/'):
				return ParsePartHelperTwo(pFirst+1, afterLast);
			default:
				return pFirst;
		}
	}
}
/*
 * [ipFutLoop]->[subDelims][ipFutStopGo]
 * [ipFutLoop]->[unreserved][ipFutStopGo]
 * [ipFutLoop]-><:>[ipFutStopGo]
 */
const char * FASTCALL UriParserState::ParseIpFutLoop(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast) {
		return StopSyntax(pFirst);
	}
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('): case _UT(')'):
			case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'): case _UT(':'):
			case _UT(';'): case _UT('\''): case _UT('_'): case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA: return ParseIpFutStopGo(pFirst+1, afterLast);
			default: return StopSyntax(pFirst);
		}
	}
}
/*
 * [ipFutStopGo]->[ipFutLoop]
 * [ipFutStopGo]-><NULL>
 */
const char * FASTCALL UriParserState::ParseIpFutStopGo(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast)
		return afterLast;
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('): case _UT(')'):
			case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'): case _UT(':'):
			case _UT(';'): case _UT('\''): case _UT('_'): case _UT('~'): case _UT('+'):
			case _UT('='): 
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				return ParseIpFutLoop(pFirst, afterLast);
			default:
				return pFirst;
		}
	}
}
/*
 * [ipFuture]-><v>[HEXDIG][hexZero]<.>[ipFutLoop]
 */
const char * FASTCALL UriParserState::ParseIpFuture(const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return StopSyntax(first);
	// 
	// First character has already been checked before entering this rule.
	// 
	// switch(*first) {
	// case _UT('v'):
	// 
	else if(first+1 >= afterLast)
		return StopSyntax(first+1);
	else {
		switch(first[1]) {
			case URI_SET_HEXDIG:
			{
				const char * afterIpFutLoop;
				const char * const afterHexZero = ParseHexZero(first+2, afterLast);
				if(afterHexZero == NULL)
					return NULL;
				else if((afterHexZero >= afterLast) ||(*afterHexZero != _UT('.')))
					return StopSyntax(afterHexZero);
				else {
					P_Uri->HostText.P_First = first; // HOST BEGIN 
					P_Uri->HostData.ipFuture.P_First = first; // IPFUTURE BEGIN 
					afterIpFutLoop = ParseIpFutLoop(afterHexZero+1, afterLast);
					if(afterIpFutLoop == NULL)
						return NULL;
					else {
						P_Uri->HostText.P_AfterLast = afterIpFutLoop; // HOST END 
						P_Uri->HostData.ipFuture.P_AfterLast = afterIpFutLoop; // IPFUTURE END 
						return afterIpFutLoop;
					}
				}
			}
			default: return StopSyntax(first+1);
		}
		/*
		   default: return state->StopSyntax(first);
		   }
		 */
	}
}
/*
 * [ipLit2]->[ipFuture]<]>
 * [ipLit2]->[IPv6address2]
 */
const char * FASTCALL UriParserState::ParseIpLit2(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		StopSyntax(first);
	else {
		switch(*first) {
			case _UT('v'):
				{
					const char * const afterIpFuture = ParseIpFuture(first, afterLast);
					if(afterIpFuture) {
						if(afterIpFuture >= afterLast || *afterIpFuture != _UT(']'))
							StopSyntax(first);
						else
							p_ret = afterIpFuture+1;
					}
				}
				break;
			case _UT(':'):
			case _UT(']'):
			case URI_SET_HEXDIG:
				P_Uri->HostData.ip6 = static_cast<UriUri::UriIp6 *>(SAlloc::M(1*sizeof(UriUri::UriIp6))); // Freed when stopping on parse error 
				if(!P_Uri->HostData.ip6)
					StopMalloc();
				else
					p_ret = ParseIPv6address2(first, afterLast);
				break;
			default:
				StopSyntax(first);
				break;
		}
	}
	return p_ret;
}
/*
 * [IPv6address2]->..<]>
 */
const char * FASTCALL UriParserState::ParseIPv6address2(const char * first, const char * afterLast)
{
	int    zipperEver = 0;
	int    quadsDone = 0;
	int    digitCount = 0;
	uchar  digitHistory[4];
	int    ip4OctetsDone = 0;
	uchar  quadsAfterZipper[14];
	int    quadsAfterZipperCount = 0;
	for(;;) {
		if(first >= afterLast) {
			return StopSyntax(first);
		}
		// Inside IPv4 part? 
		if(ip4OctetsDone > 0) {
			// Eat rest of IPv4 address 
			for(;;) {
				switch(*first) {
				    case URI_SET_DIGIT:
						if(digitCount == 4) {
							return StopSyntax(first);
						}
						digitHistory[digitCount++] =static_cast<uchar>(9+*first-_UT('9'));
						break;
				    case _UT('.'):
						if((ip4OctetsDone == 4) || /* NOTE! */ (digitCount == 0) || (digitCount == 4))
							return StopSyntax(first); // Invalid digit or octet count 
						else if((digitCount > 1) && (digitHistory[0] == 0))
							return StopSyntax(first-digitCount); // Leading zero 
						else if((digitCount > 2) && (digitHistory[1] == 0))
							return StopSyntax(first-digitCount+1); // Leading zero 
						else if((digitCount == 3) && (100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
							// Octet value too large 
							if(digitHistory[0] > 2)
								StopSyntax(first-3);
							else if(digitHistory[1] > 5)
								StopSyntax(first-2);
							else
								StopSyntax(first-1);
							return NULL;
						}
						// Copy IPv4 octet 
						P_Uri->HostData.ip6->data[16-4+ip4OctetsDone] = uriGetOctetValue(digitHistory, digitCount);
						digitCount = 0;
						ip4OctetsDone++;
						break;
				    case _UT(']'):
						if((ip4OctetsDone != 3) || /* NOTE! */ (digitCount == 0) || (digitCount == 4))
							return StopSyntax(first); // Invalid digit or octet count 
						else if((digitCount > 1) && (digitHistory[0] == 0))
							return StopSyntax(first-digitCount); // Leading zero 
						else if((digitCount > 2) && (digitHistory[1] == 0))
							return StopSyntax(first-digitCount+1); // Leading zero
						else if((digitCount == 3) && (100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
							// Octet value too large
							if(digitHistory[0] > 2)
								StopSyntax(first-3);
							else if(digitHistory[1] > 5)
								StopSyntax(first-2);
							else
								StopSyntax(first-1);
							return NULL;
						}
						P_Uri->HostText.P_AfterLast = first; /* HOST END */
						// Copy missing quads right before IPv4 
						memcpy(P_Uri->HostData.ip6->data+16-4-2*quadsAfterZipperCount, quadsAfterZipper, 2*quadsAfterZipperCount);
						// Copy last IPv4 octet 
						P_Uri->HostData.ip6->data[16-4+3] = uriGetOctetValue(digitHistory, digitCount);
						return first+1;
				    default: return StopSyntax(first);
				}
				first++;
			}
		}
		else {
			// Eat while no dot in sight 
			int    letterAmong = 0;
			int    walking = 1;
			do {
				switch(*first) {
				    case URI_SET_HEX_LETTER_LOWER:
						letterAmong = 1;
						if(digitCount == 4)
							return StopSyntax(first);
						digitHistory[digitCount] = static_cast<uchar>(15+*first-_UT('f'));
						digitCount++;
						break;
				    case URI_SET_HEX_LETTER_UPPER:
						letterAmong = 1;
						if(digitCount == 4)
							return StopSyntax(first);
						digitHistory[digitCount] = static_cast<uchar>(15+*first-_UT('F'));
						digitCount++;
						break;
				    case URI_SET_DIGIT:
						if(digitCount == 4)
							return StopSyntax(first);
						digitHistory[digitCount] = static_cast<uchar>(9+*first-_UT('9'));
						digitCount++;
						break;
				    case _UT(':'):
						{
							int setZipper = 0;
							if(quadsDone > 8-zipperEver) // Too many quads? 
								return StopSyntax(first);
							else if(first+1 >= afterLast) // "::"? 
								return StopSyntax(first+1);
							else {
								if(first[1] == _UT(':')) {
									const int resetOffset = 2*(quadsDone+(digitCount > 0));
									first++;
									if(zipperEver)
										return StopSyntax(first); // "::.+::" 
									else {
										// Zero everything after zipper 
										memzero(P_Uri->HostData.ip6->data+resetOffset, 16-resetOffset);
										setZipper = 1;
										// ":::+"? 
										if(first+1 >= afterLast)
											return StopSyntax(first+1); // No ']' yet 
										else if(first[1] == _UT(':'))
											return StopSyntax(first+1); // ":::+ "
									}
								}
								if(digitCount > 0) {
									if(zipperEver) {
										uriWriteQuadToDoubleByte(digitHistory, digitCount, quadsAfterZipper+2*quadsAfterZipperCount);
										quadsAfterZipperCount++;
									}
									else {
										uriWriteQuadToDoubleByte(digitHistory, digitCount, P_Uri->HostData.ip6->data+2*quadsDone);
									}
									quadsDone++;
									digitCount = 0;
								}
								letterAmong = 0;
								if(setZipper) {
									zipperEver = 1;
								}
							}
						}
						break;
				    case _UT('.'):
						if((quadsDone > 6) || /* NOTE */(!zipperEver &&(quadsDone < 6)) || letterAmong ||(digitCount == 0) ||(digitCount == 4))
							return StopSyntax(first); // Invalid octet before 
						else if((digitCount > 1) &&(digitHistory[0] == 0))
							return StopSyntax(first-digitCount); // Leading zero 
						else if((digitCount > 2) &&(digitHistory[1] == 0))
							return StopSyntax(first-digitCount+1); // Leading zero 
						else if((digitCount == 3) &&(100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
							// Octet value too large 
							if(digitHistory[0] > 2)
								StopSyntax(first-3);
							else if(digitHistory[1] > 5)
								StopSyntax(first-2);
							else
								StopSyntax(first-1);
							return NULL;
						}
						// Copy first IPv4 octet 
						P_Uri->HostData.ip6->data[16-4] = uriGetOctetValue(digitHistory, digitCount);
						digitCount = 0;
						// Switch over to IPv4 loop 
						ip4OctetsDone = 1;
						walking = 0;
						break;
				    case _UT(']'): // Too little quads? 
						if(!zipperEver && !((quadsDone == 7) && (digitCount > 0)))
							return StopSyntax(first);
						if(digitCount > 0) {
							if(zipperEver) {
								uriWriteQuadToDoubleByte(digitHistory, digitCount, quadsAfterZipper+2*quadsAfterZipperCount);
								quadsAfterZipperCount++;
							}
							else {
								uriWriteQuadToDoubleByte(digitHistory, digitCount, P_Uri->HostData.ip6->data+2*quadsDone);
							}
							/*
							   quadsDone++;
							   digitCount = 0;
							 */
						}
						// Copy missing quads to the end 
						memcpy(P_Uri->HostData.ip6->data+16-2*quadsAfterZipperCount, quadsAfterZipper, 2*quadsAfterZipperCount);
						P_Uri->HostText.P_AfterLast = first; /* HOST END */
						return first+1; /* Fine */
				    default: return StopSyntax(first);
				}
				first++;
				if(first >= afterLast)
					return StopSyntax(first); // No ']' yet 
			} while(walking);
		}
	}
}
/*
 * [mustBeSegmentNzNc]->[pctEncoded][mustBeSegmentNzNc]
 * [mustBeSegmentNzNc]->[subDelims][mustBeSegmentNzNc]
 * [mustBeSegmentNzNc]->[unreserved][mustBeSegmentNzNc]
 * [mustBeSegmentNzNc]->[uriTail] // can take <NULL>
 * [mustBeSegmentNzNc]-></>[segment][zeroMoreSlashSegs][uriTail]
 * [mustBeSegmentNzNc]-><@>[mustBeSegmentNzNc]
 */
const char * FASTCALL UriParserState::ParseMustBeSegmentNzNc(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast) {
		if(!PushPathSegment(P_Uri->Scheme.P_First, pFirst)) { /* SEGMENT BOTH */
			StopMalloc();
			return NULL;
		}
		else {
			P_Uri->Scheme.P_First = NULL; // Not a scheme, reset 
			return afterLast;
		}
	}
	else {
		switch(*pFirst) {
			case _UT('%'):
				{
					const char * const afterPctEncoded = ParsePctEncoded(pFirst, afterLast);
					return afterPctEncoded ? ParseMustBeSegmentNzNc(afterPctEncoded, afterLast) : 0;
				}
			case _UT('@'): case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('*'): case _UT(','): case _UT(';'): case _UT('\''):
			case _UT('+'): case _UT('='): case _UT('-'): case _UT('.'): case _UT('_'): case _UT('~'):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				return ParseMustBeSegmentNzNc(pFirst+1, afterLast);
			case _UT('/'):
				{
					const char * afterZeroMoreSlashSegs;
					const char * afterSegment;
					if(!PushPathSegment(P_Uri->Scheme.P_First, pFirst)) { /* SEGMENT BOTH */
						StopMalloc();
						return NULL;
					}
					else {
						P_Uri->Scheme.P_First = NULL; // Not a scheme, reset 
						afterSegment = ParseSegment(pFirst+1, afterLast);
						if(afterSegment == NULL) {
							return NULL;
						}
						else if(!PushPathSegment(pFirst+1, afterSegment)) { /* SEGMENT BOTH */
							StopMalloc();
							return NULL;
						}
						else {
							afterZeroMoreSlashSegs = ParseZeroMoreSlashSegs(afterSegment, afterLast);
							return afterZeroMoreSlashSegs ? ParseUriTail(afterZeroMoreSlashSegs, afterLast) : 0;
						}
					}
				}
			default:
				if(!PushPathSegment(P_Uri->Scheme.P_First, pFirst)) { // SEGMENT BOTH 
					StopMalloc();
					return NULL;
				}
				else {
					P_Uri->Scheme.P_First = NULL; // Not a scheme, reset 
					return ParseUriTail(pFirst, afterLast);
				}
		}
	}
}
/*
 * [ownHost]-><[>[ipLit2][authorityTwo]
 * [ownHost]->[ownHost2] // can take <NULL>
 */
const char * FASTCALL UriParserState::ParseOwnHost(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast)
		return afterLast;
	else if(*pFirst == _UT('[')) {
		const char * const afterIpLit2 = ParseIpLit2(pFirst+1, afterLast);
		if(!afterIpLit2)
			return NULL;
		else {
			P_Uri->HostText.P_First = pFirst+1; // HOST BEGIN 
			return ParseAuthorityTwo(afterIpLit2, afterLast);
		}
	}
	else
		return ParseOwnHost2(pFirst, afterLast);
}

int FASTCALL UriParserState::OnExitOwnHost2(const char * first)
{
	P_Uri->HostText.P_AfterLast = first; // HOST END 
	// Valid IPv4 or just a regname? 
	P_Uri->HostData.ip4 = static_cast<UriUri::UriIp4 *>(SAlloc::M(1*sizeof(UriUri::UriIp4))); // Freed when stopping on parse error 
	if(P_Uri->HostData.ip4 == NULL)
		return FALSE; // Raises SAlloc::M error 
	else {
		if(UriParseIpFourAddress(P_Uri->HostData.ip4->data, P_Uri->HostText.P_First, P_Uri->HostText.P_AfterLast)) {
			// Not IPv4 
			SAlloc::F(P_Uri->HostData.ip4);
			P_Uri->HostData.ip4 = NULL;
		}
		return TRUE; // Success
	}
}
/*
 * [ownHost2]->[authorityTwo] // can take <NULL>
 * [ownHost2]->[pctSubUnres][ownHost2]
 */
const char * FASTCALL UriParserState::ParseOwnHost2(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast) {
		if(!OnExitOwnHost2(pFirst))
			StopMalloc();
		else
			p_ret = pAfterLast;
	}
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(';'): case _UT('\''): case _UT('_'): case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPctSubUnres = ParsePctSubUnres(pFirst, pAfterLast);
					p_ret = afterPctSubUnres ? ParseOwnHost2(afterPctSubUnres, pAfterLast) : 0; // @recursion
				}
				break;
			default:
				if(!OnExitOwnHost2(pFirst))
					StopMalloc();
				else
					p_ret = ParseAuthorityTwo(pFirst, pAfterLast);
				break;
		}
	}
	return p_ret;
}

int FASTCALL UriParserState::OnExitOwnHostUserInfo(const char * first)
{
	int    ok = 1;
	P_Uri->HostText.P_First = P_Uri->UserInfo.P_First; // Host instead of userInfo, update
	P_Uri->UserInfo.P_First = NULL; // Not a userInfo, reset 
	P_Uri->HostText.P_AfterLast = first; // HOST END 
	// Valid IPv4 or just a regname? 
	P_Uri->HostData.ip4 = static_cast<UriUri::UriIp4 *>(SAlloc::M(1*sizeof(UriUri::UriIp4))); // Freed when stopping on parse error 
	if(P_Uri->HostData.ip4 == NULL) {
		ok = 0; /* Raises SAlloc::M error */
	}
	else if(UriParseIpFourAddress(P_Uri->HostData.ip4->data, P_Uri->HostText.P_First, P_Uri->HostText.P_AfterLast)) {
		// Not IPv4 
		SAlloc::F(P_Uri->HostData.ip4);
		P_Uri->HostData.ip4 = NULL;
	}
	return ok; // Success 
}
/*
 * [ownHostUserInfo]->[ownHostUserInfoNz]
 * [ownHostUserInfo]-><NULL>
 */
const char * FASTCALL UriParserState::ParseOwnHostUserInfo(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		if(!OnExitOwnHostUserInfo(first))
			StopMalloc();
		else
			p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				p_ret = ParseOwnHostUserInfoNz(first, afterLast);
				break;
			default:
				if(!OnExitOwnHostUserInfo(first))
					StopMalloc();
				else
					p_ret = first;
				break;
		}
	}
	return p_ret;
}
/*
 * [ownHostUserInfoNz]->[pctSubUnres][ownHostUserInfo]
 * [ownHostUserInfoNz]-><:>[ownPortUserInfo]
 * [ownHostUserInfoNz]-><@>[ownHost]
 */
const char * FASTCALL UriParserState::ParseOwnHostUserInfoNz(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		StopSyntax(first);
	else {
		switch(*first) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(';'): case _UT('\''): case _UT('_'): case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPctSubUnres = ParsePctSubUnres(first, afterLast);
					p_ret = afterPctSubUnres ? ParseOwnHostUserInfo(afterPctSubUnres, afterLast) : 0;
				}
				break;
			case _UT(':'):
				P_Uri->HostText.P_AfterLast = first; /* HOST END */
				P_Uri->PortText.P_First = first+1; /* PORT BEGIN */
				p_ret = ParseOwnPortUserInfo(first+1, afterLast);
				break;
			case _UT('@'):
				P_Uri->UserInfo.P_AfterLast = first; /* USERINFO END */
				P_Uri->HostText.P_First = first+1; /* HOST BEGIN */
				p_ret = ParseOwnHost(first+1, afterLast);
				break;
			default:
				StopSyntax(first);
				break;
		}
	}
	return p_ret;
}

int FASTCALL UriParserState::OnExitOwnPortUserInfo(const char * pFirst)
{
	int    ok = 1;
	P_Uri->HostText.P_First = P_Uri->UserInfo.P_First; // Host instead of userInfo, update 
	P_Uri->UserInfo.P_First = NULL; // Not a userInfo, reset 
	P_Uri->PortText.P_AfterLast = pFirst; // PORT END 
	// Valid IPv4 or just a regname? 
	P_Uri->HostData.ip4 = static_cast<UriUri::UriIp4 *>(SAlloc::M(1*sizeof(UriUri::UriIp4))); // Freed when stopping on parse error 
	if(P_Uri->HostData.ip4 == NULL) {
		ok = 0; // Raises SAlloc::M error 
	}
	else if(UriParseIpFourAddress(P_Uri->HostData.ip4->data, P_Uri->HostText.P_First, P_Uri->HostText.P_AfterLast)) {
		ZFREE(P_Uri->HostData.ip4); // Not IPv4 
	}
	return ok;
}
/*
 * [ownPortUserInfo]->[ALPHA][ownUserInfo]
 * [ownPortUserInfo]->[DIGIT][ownPortUserInfo]
 * [ownPortUserInfo]-><.>[ownUserInfo]
 * [ownPortUserInfo]-><_>[ownUserInfo]
 * [ownPortUserInfo]-><~>[ownUserInfo]
 * [ownPortUserInfo]-><->[ownUserInfo]
 * [ownPortUserInfo]-><@>[ownHost]
 * [ownPortUserInfo]-><NULL>
 */
const char * FASTCALL UriParserState::ParseOwnPortUserInfo(const char * pFirst, const char * afterLast)
{
	const char * p_ret = 0;
	if(pFirst >= afterLast) {
		if(!OnExitOwnPortUserInfo(pFirst))
			StopMalloc();
		else
			p_ret = afterLast;
	}
	else {
		switch(*pFirst) {
			case _UT('.'): case _UT('_'): case _UT('~'): case _UT('-'): case URI_SET_ALPHA:
				P_Uri->HostText.P_AfterLast = NULL; // Not a host, reset 
				P_Uri->PortText.P_First = NULL; // Not a port, reset 
				p_ret = ParseOwnUserInfo(pFirst+1, afterLast);
				break;
			case URI_SET_DIGIT:
				p_ret = ParseOwnPortUserInfo(pFirst+1, afterLast); // @recursion
				break;
			case _UT('@'):
				P_Uri->HostText.P_AfterLast = NULL; // Not a host, reset 
				P_Uri->PortText.P_First = NULL; // Not a port, reset 
				P_Uri->UserInfo.P_AfterLast = pFirst; // USERINFO END 
				P_Uri->HostText.P_First = pFirst+1; // HOST BEGIN 
				p_ret = ParseOwnHost(pFirst+1, afterLast);
				break;
			default:
				if(!OnExitOwnPortUserInfo(pFirst))
					StopMalloc();
				else
					p_ret = pFirst;
				break;
		}
	}
	return p_ret;
}
/*
 * [ownUserInfo]->[pctSubUnres][ownUserInfo]
 * [ownUserInfo]-><:>[ownUserInfo]
 * [ownUserInfo]-><@>[ownHost]
 */
const char * FASTCALL UriParserState::ParseOwnUserInfo(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast)
		StopSyntax(pFirst);
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(';'): case _UT('\''): case _UT('_'): case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const after_pct_sub_unres = ParsePctSubUnres(pFirst, pAfterLast);
					p_ret = after_pct_sub_unres ? ParseOwnUserInfo(after_pct_sub_unres, pAfterLast) : 0; // @recursion
				}
				break;
			case _UT(':'):
				p_ret = ParseOwnUserInfo(pFirst+1, pAfterLast); // @recursion
				break;
			case _UT('@'): // SURE 
				P_Uri->UserInfo.P_AfterLast = pFirst; /* USERINFO END */
				P_Uri->HostText.P_First = pFirst+1; /* HOST BEGIN */
				p_ret = ParseOwnHost(pFirst+1, pAfterLast);
				break;
			default:
				StopSyntax(pFirst);
				break;
		}
	}
	return p_ret;
}

/*static void FASTCALL UriOnExitPartHelperTwo(UriParserState * pState)
{
	pState->P_Uri->IsAbsolutePath = TRUE;
}*/
/*
 * [partHelperTwo]->[pathAbsNoLeadSlash] // can take <NULL>
 * [partHelperTwo]-></>[authority][pathAbsEmpty]
 */
const char * FASTCALL UriParserState::ParsePartHelperTwo(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast) {
		P_Uri->IsAbsolutePath = TRUE; //UriOnExitPartHelperTwo(this);
		p_ret = pAfterLast;
	}
	else if(*pFirst == _UT('/')) {
		const char * const p_after_authority = ParseAuthority(pFirst+1, pAfterLast);
		if(p_after_authority) {
			p_ret = ParsePathAbsEmpty(p_after_authority, pAfterLast);
			UriFixEmptyTrailSegment(P_Uri);
		}
	}
	else {
		P_Uri->IsAbsolutePath = TRUE; //UriOnExitPartHelperTwo(this);
		p_ret = ParsePathAbsNoLeadSlash(pFirst, pAfterLast);
	}
	return p_ret;
}
// 
// [pathAbsEmpty]-></>[segment][pathAbsEmpty]
// [pathAbsEmpty]-><NULL>
// 
const char * FASTCALL UriParserState::ParsePathAbsEmpty(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast)
		p_ret = pAfterLast;
	else if(*pFirst == _UT('/')) {
		const char * const p_after_segment = ParseSegment(pFirst+1, pAfterLast);
		if(p_after_segment) {
			if(!PushPathSegment(pFirst+1, p_after_segment)) // SEGMENT BOTH
				StopMalloc();
			else
				p_ret = ParsePathAbsEmpty(p_after_segment, pAfterLast); // @recursion
		}
	}
	else
		p_ret = pFirst;
	return p_ret;
}
/*
 * [pathAbsNoLeadSlash]->[segmentNz][zeroMoreSlashSegs]
 * [pathAbsNoLeadSlash]-><NULL>
 */
const char * FASTCALL UriParserState::ParsePathAbsNoLeadSlash(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast)
		p_ret = pAfterLast;
	else {
		switch(*pFirst) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				/* @v11.1.11 {
					const char * const p_after_segment_nz = ParseSegmentNz(pFirst, pAfterLast);
					if(p_after_segment_nz) {
						if(!PushPathSegment(pFirst, p_after_segment_nz)) // SEGMENT BOTH
							StopMalloc();
						else
							p_ret = ParseZeroMoreSlashSegs(p_after_segment_nz, pAfterLast);
					}
				}*/
				p_ret = ParsePathRootless(pFirst, pAfterLast); // @v11.1.11
				break;
			default:
				p_ret = pFirst;
				break;
		}
	}
	return p_ret;
}
// 
// [pathRootless]->[segmentNz][zeroMoreSlashSegs]
// 
const char * FASTCALL UriParserState::ParsePathRootless(const char * pFirst, const char * pAfterLast)
{
	const char * p_result = 0;
	const char * const p_after_segment_nz = ParseSegmentNz(pFirst, pAfterLast);
	if(p_after_segment_nz) {
		if(!PushPathSegment(pFirst, p_after_segment_nz)) // SEGMENT BOTH 
			StopMalloc();
		else
			p_result = ParseZeroMoreSlashSegs(p_after_segment_nz, pAfterLast);
	}
	return p_result;
}
/*
 * [pchar]->[pctEncoded]
 * [pchar]->[subDelims]
 * [pchar]->[unreserved]
 * [pchar]-><:>
 * [pchar]-><@>
 */
const char * FASTCALL UriParserState::ParsePchar(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		StopSyntax(first);
	else {
		switch(*first) {
			case _UT('%'):
				p_ret = ParsePctEncoded(first, afterLast);
				break;
			case _UT(':'): case _UT('@'): case _UT('!'): case _UT('$'): case _UT('&'):
			case _UT('('): case _UT(')'): case _UT('*'): case _UT(','): case _UT(';'):
			case _UT('\''): case _UT('+'): case _UT('='): case _UT('-'): case _UT('.'):
			case _UT('_'): case _UT('~'):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				p_ret = first+1;
				break;
			default:
				if(IsLetter1251(*first))
					p_ret = first+1;
				else
					StopSyntax(first);
				break;
		}
	}
	return p_ret;
}
/*
 * [pctEncoded]-><%>[HEXDIG][HEXDIG]
 */
const char * FASTCALL UriParserState::ParsePctEncoded(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast)
		return StopSyntax(pFirst);
	// 
	// First character has already been checked before entering this rule.
	// switch(*first) {
	//   case _UT('%'):
	// 
	else if(pFirst+1 >= afterLast)
		return StopSyntax(pFirst+1);
	else {
		switch(pFirst[1]) {
			case URI_SET_HEXDIG:
				if(pFirst+2 >= afterLast)
					return StopSyntax(pFirst+2);
				else {
					switch(pFirst[2]) {
						case URI_SET_HEXDIG: return pFirst+3;
						default: return StopSyntax(pFirst+2);
					}
				}
			default: return StopSyntax(pFirst+1);
		}
		/*
		   default: return state->StopSyntax(first);
		   }
		 */
	}
}
/*
 * [pctSubUnres]->[pctEncoded]
 * [pctSubUnres]->[subDelims]
 * [pctSubUnres]->[unreserved]
 */
const char * FASTCALL UriParserState::ParsePctSubUnres(const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return StopSyntax(first);
	else {
		switch(*first) {
			case _UT('%'): return ParsePctEncoded(first, afterLast);
			case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('): case _UT(')'):
			case _UT('*'): case _UT(','): case _UT(';'): case _UT('\''): case _UT('+'):
			case _UT('='): case _UT('-'): case _UT('.'): case _UT('_'): case _UT('~'):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA: return first+1;
			default: return StopSyntax(first);
		}
	}
}
/*
 * [port]->[DIGIT][port]
 * [port]-><NULL>
 */
const char * FASTCALL UriParserState::ParsePort(const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(isdec(*first))
		return ParsePort(first+1, afterLast); // @recursion
	else
		return first;
}
/*
 * [queryFrag]->[pchar][queryFrag]
 * [queryFrag]-></>[queryFrag]
 * [queryFrag]-><?>[queryFrag]
 * [queryFrag]-><NULL>
 */
const char * FASTCALL UriParserState::ParseQueryFrag(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPchar = ParsePchar(first, afterLast);
					p_ret = afterPchar ? ParseQueryFrag(afterPchar, afterLast) : 0; // @recursion
				}
				break;
			case _UT('/'):
			case _UT('?'):
				p_ret = ParseQueryFrag(first+1, afterLast); // @recursion
				break;
			default:
				p_ret = first;
				break;
		}
	}
	return p_ret;
}
/*
 * [segment]->[pchar][segment]
 * [segment]-><NULL>
 */
const char * FASTCALL UriParserState::ParseSegment(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		p_ret = afterLast;
	else {
		switch(*first) {
			case _UT('!'): case _UT('$'): case _UT('%'): case _UT('&'): case _UT('('):
			case _UT(')'): case _UT('-'): case _UT('*'): case _UT(','): case _UT('.'):
			case _UT(':'): case _UT(';'): case _UT('@'): case _UT('\''): case _UT('_'):
			case _UT('~'): case _UT('+'): case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPchar = ParsePchar(first, afterLast);
					p_ret = afterPchar ? ParseSegment(afterPchar, afterLast) : 0; // @recursion
				}
				break;
			default:
				if(IsLetter1251(*first)) {
					const char * const afterPchar = ParsePchar(first, afterLast);
					p_ret = afterPchar ? ParseSegment(afterPchar, afterLast) : 0; // @recursion
				}
				else
					p_ret = first;
				break;
		}
	}
	return p_ret;
}
/*
 * [segmentNz]->[pchar][segment]
 */
const char * FASTCALL UriParserState::ParseSegmentNz(const char * first, const char * afterLast)
{
	const char * const afterPchar = ParsePchar(first, afterLast);
	return afterPchar ? ParseSegment(afterPchar, afterLast) : 0;
}

int FASTCALL UriParserState::OnExitSegmentNzNcOrScheme2(const char * first)
{
	if(!PushPathSegment(P_Uri->Scheme.P_First, first)) // SEGMENT BOTH
		return FALSE; // Raises SAlloc::M error
	else {
		P_Uri->Scheme.P_First = NULL; // Not a scheme, reset 
		return TRUE; // Success 
	}
}
/*
 * [segmentNzNcOrScheme2]->[ALPHA][segmentNzNcOrScheme2]
 * [segmentNzNcOrScheme2]->[DIGIT][segmentNzNcOrScheme2]
 * [segmentNzNcOrScheme2]->[pctEncoded][mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]->[uriTail] // can take <NULL>
 * [segmentNzNcOrScheme2]-><!>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><$>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><&>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><(>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><)>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><*>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><,>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><.>[segmentNzNcOrScheme2]
 * [segmentNzNcOrScheme2]-></>[segment][zeroMoreSlashSegs][uriTail]
 * [segmentNzNcOrScheme2]-><:>[hierPart][uriTail]
 * [segmentNzNcOrScheme2]-><;>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><@>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><_>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><~>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><+>[segmentNzNcOrScheme2]
 * [segmentNzNcOrScheme2]-><=>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><'>[mustBeSegmentNzNc]
 * [segmentNzNcOrScheme2]-><->[segmentNzNcOrScheme2]
 */
const char * UriParserState::ParseSegmentNzNcOrScheme2(const char * pFirst, const char * afterLast)
{
	if(pFirst >= afterLast) {
		if(!OnExitSegmentNzNcOrScheme2(pFirst)) {
			StopMalloc();
			return NULL;
		}
		else
			return afterLast;
	}
	switch(*pFirst) {
	    case _UT('.'):
	    case _UT('+'):
	    case _UT('-'):
	    case URI_SET_ALPHA:
	    case URI_SET_DIGIT:
			return ParseSegmentNzNcOrScheme2(pFirst+1, afterLast); // @recursion
	    case _UT('%'):
	    {
		    const char * const afterPctEncoded = ParsePctEncoded(pFirst, afterLast);
			return afterPctEncoded ? ParseMustBeSegmentNzNc(afterPctEncoded, afterLast) : 0;
	    }
	    case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('): case _UT(')'):
	    case _UT('*'): case _UT(','): case _UT(';'): case _UT('@'): case _UT('_'):
	    case _UT('~'): case _UT('='): case _UT('\''):
			return ParseMustBeSegmentNzNc(pFirst+1, afterLast);
	    case _UT('/'):
	    {
		    const char * const p_after_segment = ParseSegment(pFirst+1, afterLast);
		    if(!p_after_segment)
			    return NULL;
		    else if(!PushPathSegment(P_Uri->Scheme.P_First, pFirst)) { // SEGMENT BOTH 
			    StopMalloc();
			    return NULL;
		    }
			else {
				P_Uri->Scheme.P_First = NULL; // Not a scheme, reset 
				if(!PushPathSegment(pFirst+1, p_after_segment)) { // SEGMENT BOTH 
					StopMalloc();
					return NULL;
				}
				else {
					const char * p_after_zero_more_slash_segs = ParseZeroMoreSlashSegs(p_after_segment, afterLast);
					return p_after_zero_more_slash_segs ? ParseUriTail(p_after_zero_more_slash_segs, afterLast) : 0;
				}
			}
	    }
	    case _UT(':'):
	    {
		    const char * const p_after_hier_part = ParseHierPart(pFirst+1, afterLast);
		    P_Uri->Scheme.P_AfterLast = pFirst; // SCHEME END 
			return p_after_hier_part ? ParseUriTail(p_after_hier_part, afterLast) : 0;
	    }
	    default:
			if(!OnExitSegmentNzNcOrScheme2(pFirst)) {
				StopMalloc();
				return NULL;
			}
			else
				return ParseUriTail(pFirst, afterLast);
	}
}
/*
 * [uriReference]->[ALPHA][segmentNzNcOrScheme2]
 * [uriReference]->[DIGIT][mustBeSegmentNzNc]
 * [uriReference]->[pctEncoded][mustBeSegmentNzNc]
 * [uriReference]->[subDelims][mustBeSegmentNzNc]
 * [uriReference]->[uriTail] // can take <NULL>
 * [uriReference]-><.>[mustBeSegmentNzNc]
 * [uriReference]-></>[partHelperTwo][uriTail]
 * [uriReference]-><@>[mustBeSegmentNzNc]
 * [uriReference]-><_>[mustBeSegmentNzNc]
 * [uriReference]-><~>[mustBeSegmentNzNc]
 * [uriReference]-><->[mustBeSegmentNzNc]
 */
//static const char * UriParseUriReference(UriParserState * state, const char * first, const char * afterLast)
const char * FASTCALL UriParserState::ParseUriReference(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		p_ret = afterLast;
	else {
		switch(*first) {
			case URI_SET_ALPHA:
				P_Uri->Scheme.P_First = first; // SCHEME BEGIN 
				p_ret = ParseSegmentNzNcOrScheme2(first+1, afterLast);
				break;
			case URI_SET_DIGIT:
			case _UT('!'): case _UT('$'): case _UT('&'): case _UT('('):  case _UT(')'):
			case _UT('*'): case _UT(','): case _UT(';'): case _UT('\''): case _UT('+'):
			case _UT('='): case _UT('.'): case _UT('_'): case _UT('~'):  case _UT('-'): case _UT('@'):
				P_Uri->Scheme.P_First = first; // SEGMENT BEGIN, ABUSE SCHEME POINTER 
				p_ret = ParseMustBeSegmentNzNc(first+1, afterLast);
				break;
			case _UT('%'):
				{
					const char * const afterPctEncoded = ParsePctEncoded(first, afterLast);
					if(afterPctEncoded) {
						P_Uri->Scheme.P_First = first; // SEGMENT BEGIN, ABUSE SCHEME POINTER 
						p_ret = ParseMustBeSegmentNzNc(afterPctEncoded, afterLast);
					}
				}
				break;
			case _UT('/'):
				{
					const char * const p_after_part_helper_two = ParsePartHelperTwo(first+1, afterLast);
					p_ret = p_after_part_helper_two ? ParseUriTail(p_after_part_helper_two, afterLast) : 0;
				}
				break;
			default:
				p_ret = ParseUriTail(first, afterLast);
				break;
		}
	}
	return p_ret;
}
/*
 * [uriTail]-><#>[queryFrag]
 * [uriTail]-><?>[queryFrag][uriTailTwo]
 * [uriTail]-><NULL>
 */
const char * FASTCALL UriParserState::ParseUriTail(const char * pFirst, const char * pAfterLast)
{
	const char * p_ret = 0;
	if(pFirst >= pAfterLast)
		p_ret = pAfterLast;
	else if(*pFirst == _UT('#')) {
		const char * const p_after_query_frag = ParseQueryFrag(pFirst+1, pAfterLast);
		if(p_after_query_frag) {
			P_Uri->fragment.P_First = pFirst+1; // FRAGMENT BEGIN 
			P_Uri->fragment.P_AfterLast = p_after_query_frag; // FRAGMENT END 
			p_ret = p_after_query_frag;
		}
	}
	else if(*pFirst == _UT('?')) {
		const char * const p_after_query_frag = ParseQueryFrag(pFirst+1, pAfterLast);
		if(p_after_query_frag) {
			P_Uri->query.P_First = pFirst+1; // QUERY BEGIN 
			P_Uri->query.P_AfterLast = p_after_query_frag; // QUERY END 
			p_ret = ParseUriTailTwo(p_after_query_frag, pAfterLast);
		}
	}
	else
		p_ret = pFirst;
	return p_ret;
}
/*
 * [uriTailTwo]-><#>[queryFrag]
 * [uriTailTwo]-><NULL>
 */
const char * FASTCALL UriParserState::ParseUriTailTwo(const char * first, const char * pAfterLast)
{
	if(first >= pAfterLast)
		return pAfterLast;
	else if(*first == _UT('#')) {
		const char * const p_after_query_frag = ParseQueryFrag(first+1, pAfterLast);
		if(p_after_query_frag) {
			P_Uri->fragment.P_First = first+1; // FRAGMENT BEGIN 
			P_Uri->fragment.P_AfterLast = p_after_query_frag; // FRAGMENT END 
		}
		return p_after_query_frag;
	}
	else
		return first;
}
/*
 * [zeroMoreSlashSegs]-></>[segment][zeroMoreSlashSegs]
 * [zeroMoreSlashSegs]-><NULL>
 */
const char * FASTCALL UriParserState::ParseZeroMoreSlashSegs(const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast)
		p_ret = afterLast;
	else if(*first == _UT('/')) {
		const char * const after_segment = ParseSegment(first+1, afterLast);
		if(after_segment)
			if(!PushPathSegment(first+1, after_segment)) // SEGMENT BOTH 
				StopMalloc();
			else
				p_ret = ParseZeroMoreSlashSegs(after_segment, afterLast); // @recursion
	}
	else
		p_ret = first;
	return p_ret;
}

int FASTCALL UriParserState::ParseUriEx(const char * pFirst, const char * pAfterLast)
{
	int    ok = 1;
	// Check params 
	THROW_S(pFirst && pAfterLast, SLERR_URI_NULL);
	{
		UriUri * p_uri = P_Uri;
		// Init parser 
		Reset();
		UriResetUri(p_uri);
		// Parse 
		const char * p_after_uri_reference = ParseUriReference(pFirst, pAfterLast);
		THROW_S(p_after_uri_reference, ErrorCode);
		THROW_S_S(p_after_uri_reference == pAfterLast, SLERR_URI_SYNTAX, pFirst);
	}
	CATCHZOK
	return ok;
}

int UriParseUri(UriParserState * pState, const char * pText)
{
	const size_t len = sstrlen(pText);
	return (pState && len) ? pState->ParseUriEx(pText, pText+len) : SLS.SetError(SLERR_URI_NULL);
}

UriUri::PathSegment::PathSegment(const char * pFirst, const char * pAfterLast) : text(pFirst, pAfterLast), next(0), reserved(0)
{
}

void UriUri::Destroy()
{
	if(IsOwner) {
		// Scheme 
		if(Scheme.P_First) {
			if(Scheme.P_First != Scheme.P_AfterLast)
				SAlloc::F(const_cast<char *>(Scheme.P_First)); // @badcast
			Scheme.Clear();
		}
		// User info 
		if(UserInfo.P_First) {
			if(UserInfo.P_First != UserInfo.P_AfterLast)
				SAlloc::F(const_cast<char *>(UserInfo.P_First)); // @badcast
			UserInfo.Clear();
		}
		// Host data - IPvFuture 
		if(HostData.ipFuture.P_First) {
			if(HostData.ipFuture.Len())
				SAlloc::F(const_cast<char *>(HostData.ipFuture.P_First)); // @badcast
			HostData.ipFuture.Clear();
			HostText.Clear();
		}
		// Host text(if regname, after IPvFuture!) 
		if(HostText.P_First && !HostData.ip4 && !HostData.ip6) {
			// Real regname 
			if(HostText.Len())
				SAlloc::F(const_cast<char *>(HostText.P_First)); // @badcast
			HostText.Clear();
		}
	}
	ZFREE(HostData.ip4); // Host data - IPv4 
	ZFREE(HostData.ip6); // Host data - IPv6 
	// Port text 
	if(IsOwner && PortText.P_First) {
		if(PortText.Len())
			SAlloc::F(const_cast<char *>(PortText.P_First));
		PortText.Clear();
	}
	// Path 
	if(pathHead) {
		UriUri::PathSegment * p_seg_walk = pathHead;
		while(p_seg_walk) {
			UriUri::PathSegment * const next = p_seg_walk->next;
			if(IsOwner && p_seg_walk->text.P_First && p_seg_walk->text.Len() > 0)
				SAlloc::F(const_cast<char *>(p_seg_walk->text.P_First)); // @badcast
			delete p_seg_walk;
			p_seg_walk = next;
		}
		pathHead = NULL;
		pathTail = NULL;
	}
	if(IsOwner) {
		// Query 
		if(query.P_First) {
			if(query.Len())
				SAlloc::F(const_cast<char *>(query.P_First)); // @badcast
			query.Clear();
		}
		// Fragment 
		if(fragment.P_First) {
			if(fragment.Len())
				SAlloc::F(const_cast<char *>(fragment.P_First)); // @badcast
			fragment.Clear();
		}
	}
}

int Uri_TESTING_ONLY_ParseIpSix(const char * pText)
{
	UriUri uri;
	UriParserState parser;
	const char * const p_after_ip_six = pText + sstrlen(pText);
	const char * res;
	parser.Reset();
	UriResetUri(&uri);
	parser.P_Uri = &uri;
	parser.P_Uri->HostData.ip6 = static_cast<UriUri::UriIp6 *>(SAlloc::M(1*sizeof(UriUri::UriIp6)));
	res = parser.ParseIPv6address2(pText, p_after_ip_six);
	uri.Destroy();
	return BIN(res == p_after_ip_six);
}

int Uri_TESTING_ONLY_ParseIpFour(const char * pText)
{
	uchar  octets[4];
	int    res = UriParseIpFourAddress(octets, pText, pText + sstrlen(pText));
	return BIN(res == SLERR_SUCCESS);
}

#undef URI_SET_DIGIT
#undef URI_SET_HEX_LETTER_UPPER
#undef URI_SET_HEX_LETTER_LOWER
#undef URI_SET_HEXDIG
#undef URI_SET_ALPHA
