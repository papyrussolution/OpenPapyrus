// uriparser - RFC 3986 URI parsing library
// 
// Copyright(C) 2007, Weijia Song <songweijia@gmail.com>
// Copyright(C) 2007, Sebastian Pipping <webmaster@hartwork.org>
// All rights reserved.
// 
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <uri.h>

#define _UT(x) x
//
//
//
void uriWriteQuadToDoubleByte(const uchar * hexDigits, int digitCount, uchar * output)
{
	switch(digitCount) {
	    case 1:
			/* 0x___? -> \x00 \x0? */
			output[0] = 0;
			output[1] = hexDigits[0];
			break;
	    case 2:
			/* 0x__?? -> \0xx \x?? */
			output[0] = 0;
			output[1] = 16*hexDigits[0]+hexDigits[1];
			break;
	    case 3:
			/* 0x_??? -> \0x? \x?? */
			output[0] = hexDigits[0];
			output[1] = 16*hexDigits[1]+hexDigits[2];
			break;
	    case 4:
			/* 0x???? -> \0?? \x?? */
			output[0] = 16*hexDigits[0]+hexDigits[1];
			output[1] = 16*hexDigits[2]+hexDigits[3];
			break;
	}
}

uchar uriGetOctetValue(const uchar * digits, int digitCount)
{
	switch(digitCount) {
	    case 1: return digits[0];
	    case 2: return 10*digits[0]+digits[1];
	    case 3:
		default: return 100*digits[0]+10*digits[1]+digits[2];
	}
}

int FASTCALL uriIsUnreserved(int code)
{
	switch(code) {
	    case L'a': /* ALPHA */
	    case L'A':
	    case L'b':
	    case L'B':
	    case L'c':
	    case L'C':
	    case L'd':
	    case L'D':
	    case L'e':
	    case L'E':
	    case L'f':
	    case L'F':
	    case L'g':
	    case L'G':
	    case L'h':
	    case L'H':
	    case L'i':
	    case L'I':
	    case L'j':
	    case L'J':
	    case L'k':
	    case L'K':
	    case L'l':
	    case L'L':
	    case L'm':
	    case L'M':
	    case L'n':
	    case L'N':
	    case L'o':
	    case L'O':
	    case L'p':
	    case L'P':
	    case L'q':
	    case L'Q':
	    case L'r':
	    case L'R':
	    case L's':
	    case L'S':
	    case L't':
	    case L'T':
	    case L'u':
	    case L'U':
	    case L'v':
	    case L'V':
	    case L'w':
	    case L'W':
	    case L'x':
	    case L'X':
	    case L'y':
	    case L'Y':
	    case L'z':
	    case L'Z':
	    case L'0': /* DIGIT */
	    case L'1':
	    case L'2':
	    case L'3':
	    case L'4':
	    case L'5':
	    case L'6':
	    case L'7':
	    case L'8':
	    case L'9':
	    case L'-': /* "-" / "." / "_" / "~" */
	    case L'.':
	    case L'_':
	    case L'~':
			return TRUE;
	    default:
			return FALSE;
	}
}

void FASTCALL uriStackToOctet(UriIp4Parser * parser, uchar * octet)
{
	switch(parser->stackCount) {
	    case 1: *octet = parser->stackOne; break;
	    case 2: *octet = (parser->stackOne*10 + parser->stackTwo); break;
	    case 3: *octet = (parser->stackOne*100 + parser->stackTwo*10 + parser->stackThree); break;
	    default: break;
	}
	parser->stackCount = 0;
}

void FASTCALL uriPushToStack(UriIp4Parser * parser, uchar digit)
{
	switch(parser->stackCount) {
	    case 0:
			parser->stackOne = digit;
			parser->stackCount = 1;
			break;
	    case 1:
			parser->stackTwo = digit;
			parser->stackCount = 2;
			break;
	    case 2:
			parser->stackThree = digit;
			parser->stackCount = 3;
			break;
	    default:
			break;
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
		UriPathSegment * sourceWalker;
		UriPathSegment * destPrev;
		/* Replace last segment("" if trailing slash) with first of append chain */
		if(absWork->pathHead == NULL) {
			UriPathSegment * const dup =(UriPathSegment *)SAlloc::M(sizeof(UriPathSegment));
			THROW(dup); /* Raises SAlloc::M error */
			dup->next = NULL;
			absWork->pathHead = dup;
			absWork->pathTail = dup;
		}
		absWork->pathTail->text.first = relAppend->pathHead->text.first;
		absWork->pathTail->text.afterLast = relAppend->pathHead->text.afterLast;
		/* Append all the others */
		sourceWalker = relAppend->pathHead->next;
		if(sourceWalker) {
			destPrev = absWork->pathTail;
			for(; ok; ) {
				UriPathSegment * const dup =(UriPathSegment *)SAlloc::M(sizeof(UriPathSegment));
				if(dup == NULL) {
					destPrev->next = NULL;
					absWork->pathTail = destPrev;
					CALLEXCEPT(); // Raises SAlloc::M error
				}
				else {
					dup->text = sourceWalker->text;
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

static int UriAddBaseUriImpl(UriUri * absDest, const UriUri * relSource, const UriUri * absBase)
{
	if(absDest == NULL) {
		return URI_ERROR_NULL;
	}
	UriResetUri(absDest);
	if((relSource == NULL) ||(absBase == NULL)) {
		return URI_ERROR_NULL;
	}
	/* absBase absolute? */
	if(absBase->scheme.first == NULL) {
		return URI_ERROR_ADDBASE_REL_BASE;
	}
	/* [01/32]	if defined(R.scheme) then */
	if(relSource->scheme.first != NULL) {
		/* [02/32]		T.scheme = R.scheme; */
		absDest->scheme = relSource->scheme;
		/* [03/32]		T.authority = R.authority; */
		if(!UriCopyAuthority(absDest, relSource)) {
			return URI_ERROR_MALLOC;
		}
		/* [04/32]		T.path = remove_dot_segments(R.path); */
		if(!UriCopyPath(absDest, relSource)) {
			return URI_ERROR_MALLOC;
		}
		if(!UriRemoveDotSegmentsAbsolute(absDest)) {
			return URI_ERROR_MALLOC;
		}
		/* [05/32]		T.query = R.query; */
		absDest->query = relSource->query;
		/* [06/32]	else */
	}
	else {
		/* [07/32]		if defined(R.authority) then */
		if(UriIsHostSet(relSource)) {
			/* [08/32]			T.authority = R.authority; */
			if(!UriCopyAuthority(absDest, relSource)) {
				return URI_ERROR_MALLOC;
			}
			/* [09/32]			T.path = remove_dot_segments(R.path); */
			if(!UriCopyPath(absDest, relSource)) {
				return URI_ERROR_MALLOC;
			}
			if(!UriRemoveDotSegmentsAbsolute(absDest)) {
				return URI_ERROR_MALLOC;
			}
			/* [10/32]			T.query = R.query; */
			absDest->query = relSource->query;
			/* [11/32]		else */
		}
		else {
			/* [28/32]			T.authority = Base.authority; */
			if(!UriCopyAuthority(absDest, absBase)) {
				return URI_ERROR_MALLOC;
			}
			/* [12/32]			if(R.path == "") then */
			if(relSource->pathHead == NULL) {
				/* [13/32]				T.path = Base.path; */
				if(!UriCopyPath(absDest, absBase)) {
					return URI_ERROR_MALLOC;
				}
				/* [14/32]				if defined(R.query) then */
				if(relSource->query.first != NULL) {
					/* [15/32]					T.query = R.query; */
					absDest->query = relSource->query;
					/* [16/32]				else */
				}
				else {
					/* [17/32]					T.query = Base.query; */
					absDest->query = absBase->query;
					/* [18/32]				endif; */
				}
				/* [19/32]			else */
			}
			else {
				/* [20/32]				if(R.path starts-with "/") then */
				if(relSource->absolutePath) {
					/* [21/32]					T.path = remove_dot_segments(R.path); */
					if(!UriCopyPath(absDest, relSource)) {
						return URI_ERROR_MALLOC;
					}
					if(!UriRemoveDotSegmentsAbsolute(absDest)) {
						return URI_ERROR_MALLOC;
					}
					/* [22/32]				else */
				}
				else {
					/* [23/32]					T.path = merge(Base.path, R.path); */
					if(!UriCopyPath(absDest, absBase)) {
						return URI_ERROR_MALLOC;
					}
					if(!UriMergePath(absDest, relSource)) {
						return URI_ERROR_MALLOC;
					}
					/* [24/32]					T.path = remove_dot_segments(T.path); */
					if(!UriRemoveDotSegmentsAbsolute(absDest)) {
						return URI_ERROR_MALLOC;
					}
					if(!UriFixAmbiguity(absDest)) {
						return URI_ERROR_MALLOC;
					}
					/* [25/32]				endif; */
				}
				/* [26/32]				T.query = R.query; */
				absDest->query = relSource->query;
				/* [27/32]			endif; */
			}
			UriFixEmptyTrailSegment(absDest);
			/* [29/32]		endif; */
		}
		/* [30/32]		T.scheme = Base.scheme; */
		absDest->scheme = absBase->scheme;
		/* [31/32]	endif; */
	}
	/* [32/32]	T.fragment = R.fragment; */
	absDest->fragment = relSource->fragment;
	return URI_SUCCESS;
}

int UriAddBaseUri(UriUri * absDest, const UriUri * relSource, const UriUri*absBase)
{
	const int res = UriAddBaseUriImpl(absDest, relSource, absBase);
	if((res != URI_SUCCESS) && absDest) {
		UriFreeUriMembers(absDest);
	}
	return res;
}
//
//
//
static int UriAppendSegment(UriUri*uri, const char * first, const char * afterLast)
{
	int    ok = 1;
	/* Create segment */
	UriPathSegment * segment = (UriPathSegment *)SAlloc::M(1 * sizeof(UriPathSegment));
	if(segment == NULL)
		ok = 0;
	else {
		segment->next = NULL;
		segment->text.first = first;
		segment->text.afterLast = afterLast;
		/* Put into chain */
		if(uri->pathTail == NULL)
			uri->pathHead = segment;
		else
			uri->pathTail->next = segment;
		uri->pathTail = segment;
	}
	return ok;
}

static int FASTCALL UriEqualsAuthority(const UriUri * first, const UriUri * second)
{
	/* IPv4 */
	if(first->hostData.ip4 != NULL) {
		return((second->hostData.ip4 != NULL) && !memcmp(first->hostData.ip4->data, second->hostData.ip4->data, 4)) ? TRUE : FALSE;
	}
	/* IPv6 */
	if(first->hostData.ip6 != NULL) {
		return((second->hostData.ip6 != NULL) && !memcmp(first->hostData.ip6->data, second->hostData.ip6->data, 16)) ? TRUE : FALSE;
	}
	/* IPvFuture */
	if(first->hostData.ipFuture.first != NULL) {
		return((second->hostData.ipFuture.first != NULL) && !strncmp(first->hostData.ipFuture.first,
				second->hostData.ipFuture.first, first->hostData.ipFuture.afterLast-first->hostData.ipFuture.first)) ? TRUE : FALSE;
	}
	if(first->hostText.first != NULL) {
		return((second->hostText.first != NULL) && !strncmp(first->hostText.first, second->hostText.first,
			first->hostText.afterLast-first->hostText.first)) ? TRUE : FALSE;
	}
	return second->hostText.first == NULL;
}

int UriRemoveBaseUriImpl(UriUri * dest, const UriUri * absSource, const UriUri * absBase, int domainRootMode)
{
	if(dest == NULL) {
		return URI_ERROR_NULL;
	}
	UriResetUri(dest);
	if((absSource == NULL) ||(absBase == NULL)) {
		return URI_ERROR_NULL;
	}
	/* absBase absolute? */
	if(absBase->scheme.first == NULL) {
		return URI_ERROR_REMOVEBASE_REL_BASE;
	}
	/* absSource absolute? */
	if(absSource->scheme.first == NULL) {
		return URI_ERROR_REMOVEBASE_REL_SOURCE;
	}
	/* [01/50]	if(A.scheme != Base.scheme) then */
	if(strncmp(absSource->scheme.first, absBase->scheme.first, absSource->scheme.afterLast-absSource->scheme.first)) {
		/* [02/50]	   T.scheme    = A.scheme; */
		dest->scheme = absSource->scheme;
		/* [03/50]	   T.authority = A.authority; */
		if(!UriCopyAuthority(dest, absSource)) {
			return URI_ERROR_MALLOC;
		}
		/* [04/50]	   T.path      = A.path; */
		if(!UriCopyPath(dest, absSource)) {
			return URI_ERROR_MALLOC;
		}
		/* [05/50]	else */
	}
	else {
		/* [06/50]	   undef(T.scheme); */
		/* NOOP */
		/* [07/50]	   if(A.authority != Base.authority) then */
		if(!UriEqualsAuthority(absSource, absBase)) {
			/* [08/50]	      T.authority = A.authority; */
			if(!UriCopyAuthority(dest, absSource)) {
				return URI_ERROR_MALLOC;
			}
			/* [09/50]	      T.path      = A.path; */
			if(!UriCopyPath(dest, absSource)) {
				return URI_ERROR_MALLOC;
			}
			/* [10/50]	   else */
		}
		else {
			/* [11/50]	      if domainRootMode then */
			if(domainRootMode == TRUE) {
				/* [12/50]	         undef(T.authority); */
				/* NOOP */
				/* [13/50]	         if(first(A.path) == "") then */
				/* GROUPED */
				/* [14/50]	            T.path   = "/." + A.path; */
				/* GROUPED */
				/* [15/50]	         else */
				/* GROUPED */
				/* [16/50]	            T.path   = A.path; */
				/* GROUPED */
				/* [17/50]	         endif; */
				if(!UriCopyPath(dest, absSource)) {
					return URI_ERROR_MALLOC;
				}
				dest->absolutePath = TRUE;
				if(!UriFixAmbiguity(dest)) {
					return URI_ERROR_MALLOC;
				}
				/* [18/50]	      else */
			}
			else {
				const UriPathSegment*sourceSeg = absSource->pathHead;
				const UriPathSegment*baseSeg = absBase->pathHead;
				/* [19/50]	         bool pathNaked = true; */
				int pathNaked = TRUE;
				/* [20/50]	         undef(last(Base.path)); */
				/* NOOP */
				/* [21/50]	         T.path = ""; */
				dest->absolutePath = FALSE;
				/* [22/50]	         while(first(A.path) == first(Base.path)) do */
				while((sourceSeg != NULL) &&(baseSeg != NULL) &&
				      !strncmp(sourceSeg->text.first, baseSeg->text.first,
					      sourceSeg->text.afterLast-sourceSeg->text.first) &&
				      !((sourceSeg->text.first == sourceSeg->text.afterLast) &&
				       ((sourceSeg->next == NULL) !=(baseSeg->next == NULL)))) {
					/* [23/50]	            A.path++; */
					sourceSeg = sourceSeg->next;
					/* [24/50]	            Base.path++; */
					baseSeg = baseSeg->next;
					/* [25/50]	         endwhile; */
				}
				/* [26/50]	         while defined(first(Base.path)) do */
				while((baseSeg != NULL) &&(baseSeg->next != NULL)) {
					/* [27/50]	            Base.path++; */
					baseSeg = baseSeg->next;
					/* [28/50]	            T.path += "../"; */
					if(!UriAppendSegment(dest, UriConstParent, UriConstParent+2)) {
						return URI_ERROR_MALLOC;
					}
					/* [29/50]	            pathNaked = false; */
					pathNaked = FALSE;
					/* [30/50]	         endwhile; */
				}
				/* [31/50]	         while defined(first(A.path)) do */
				while(sourceSeg != NULL) {
					/* [32/50]	            if pathNaked then */
					if(pathNaked == TRUE) {
						/* [33/50]	               if(first(A.path) contains ":") then */
						int containsColon = FALSE;
						const char * ch = sourceSeg->text.first;
						for(; ch < sourceSeg->text.afterLast; ch++) {
							if(*ch == _UT(':')) {
								containsColon = TRUE;
								break;
							}
						}
						if(containsColon) {
							/* [34/50]	                  T.path += "./"; */
							if(!UriAppendSegment(dest, UriConstPwd, UriConstPwd+1)) {
								return URI_ERROR_MALLOC;
							}
							/* [35/50]	               elseif(first(A.path) == "") then */
						}
						else if(sourceSeg->text.first == sourceSeg->text.afterLast) {
							/* [36/50]	                  T.path += "/."; */
							if(!UriAppendSegment(dest, UriConstPwd, UriConstPwd+1)) {
								return URI_ERROR_MALLOC;
							}
							/* [37/50]	               endif; */
						}
						/* [38/50]	            endif; */
					}
					/* [39/50]	            T.path += first(A.path); */
					if(!UriAppendSegment(dest, sourceSeg->text.first, sourceSeg->text.afterLast)) {
						return URI_ERROR_MALLOC;
					}
					/* [40/50]	            pathNaked = false; */
					pathNaked = FALSE;
					/* [41/50]	            A.path++; */
					sourceSeg = sourceSeg->next;
					/* [42/50]	            if defined(first(A.path)) then */
					/* NOOP */
					/* [43/50]	               T.path += + "/"; */
					/* NOOP */
					/* [44/50]	            endif; */
					/* NOOP */
					/* [45/50]	         endwhile; */
				}
				/* [46/50]	      endif; */
			}
			/* [47/50]	   endif; */
		}
		/* [48/50]	endif; */
	}
	/* [49/50]	T.query     = A.query; */
	dest->query = absSource->query;
	/* [50/50]	T.fragment  = A.fragment; */
	dest->fragment = absSource->fragment;
	return URI_SUCCESS;
}

int UriRemoveBaseUri(UriUri * dest, const UriUri * absSource, const UriUri * absBase, int domainRootMode)
{
	const int res = UriRemoveBaseUriImpl(dest, absSource, absBase, domainRootMode);
	if((res != URI_SUCCESS) &&(dest != NULL)) {
		UriFreeUriMembers(dest);
	}
	return res;
}
//
//
//
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
	if((queryList == NULL) ||(charsRequired == NULL)) {
		return URI_ERROR_NULL;
	}
	return UriComposeQueryEngine(NULL, queryList, 0, NULL, charsRequired, spaceToPlus, normalizeBreaks);
}

int UriComposeQuery(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten)
{
	const int spaceToPlus = TRUE;
	const int normalizeBreaks = TRUE;
	return UriComposeQueryEx(dest, queryList, maxChars, charsWritten, spaceToPlus, normalizeBreaks);
}

int UriComposeQueryEx(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten, int spaceToPlus, int normalizeBreaks)
{
	if((dest == NULL) ||(queryList == NULL)) {
		return URI_ERROR_NULL;
	}
	if(maxChars < 1) {
		return URI_ERROR_OUTPUT_TOO_LARGE;
	}
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
	if(dest == NULL) {
		return URI_ERROR_NULL;
	}
	/* Calculate space */
	res = UriComposeQueryCharsRequiredEx(queryList, &charsRequired, spaceToPlus, normalizeBreaks);
	if(res != URI_SUCCESS) {
		return res;
	}
	charsRequired++;
	/* Allocate space */
	queryString =(char *)SAlloc::M(charsRequired*sizeof(char));
	if(queryString == NULL) {
		return URI_ERROR_MALLOC;
	}
	/* Put query in */
	res = UriComposeQueryEx(queryString, queryList, charsRequired, NULL, spaceToPlus, normalizeBreaks);
	if(res != URI_SUCCESS) {
		SAlloc::F(queryString);
		return res;
	}
	*dest = queryString;
	return URI_SUCCESS;
}

int UriComposeQueryEngine(char * dest, const UriQueryList*queryList,
	int maxChars, int * charsWritten, int * charsRequired, int spaceToPlus, int normalizeBreaks)
{
	int firstItem = TRUE;
	int ampersandLen = 0;
	char * write = dest;
	/* Subtract terminator */
	if(dest == NULL) {
		*charsRequired = 0;
	}
	else {
		maxChars--;
	}
	while(queryList != NULL) {
		const char * const key = queryList->key;
		const char * const value = queryList->value;
		const int worstCase =(normalizeBreaks == TRUE ? 6 : 3);
		const int keyLen =(key == NULL) ? 0 :(int)strlen(key);
		const int keyRequiredChars = worstCase*keyLen;
		const int valueLen =(value == NULL) ? 0 :(int)strlen(value);
		const int valueRequiredChars = worstCase*valueLen;
		if(dest == NULL) {
			if(firstItem == TRUE) {
				ampersandLen = 1;
				firstItem = FALSE;
			}
			(*charsRequired) += ampersandLen+keyRequiredChars+((value == NULL) ? 0 : 1+valueRequiredChars);
		}
		else {
			char * afterKey;
			if((write-dest)+ampersandLen+keyRequiredChars > maxChars) {
				return URI_ERROR_OUTPUT_TOO_LARGE;
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
			if(value != NULL) {
				char * afterValue;
				if((write-dest)+1+valueRequiredChars > maxChars) {
					return URI_ERROR_OUTPUT_TOO_LARGE;
				}
				/* Copy value */
				write[0] = _UT('=');
				write++;
				afterValue = UriEscapeEx(value, value+valueLen,
					write, spaceToPlus, normalizeBreaks);
				write +=(afterValue-write);
			}
		}
		queryList = queryList->next;
	}
	if(dest != NULL) {
		write[0] = _UT('\0');
		if(charsWritten != NULL) {
			*charsWritten =(int)(write-dest)+1;     /* .. for terminator */
		}
	}
	return URI_SUCCESS;
}

int UriAppendQueryItem(UriQueryList ** prevNext,
	int * itemCount, const char * keyFirst, const char * keyAfter,
	const char * valueFirst, const char * valueAfter,
	int plusToSpace, UriBreakConversion breakConversion)
{
	const int keyLen =(int)(keyAfter-keyFirst);
	const int valueLen =(int)(valueAfter-valueFirst);
	char * key;
	char * value;
	if((prevNext == NULL) ||(itemCount == NULL) ||(keyFirst == NULL) ||(keyAfter == NULL) ||
	  (keyFirst > keyAfter) ||(valueFirst > valueAfter) ||((keyFirst == keyAfter) &&(valueFirst == NULL) &&(valueAfter == NULL))) {
		return TRUE;
	}
	/* Append new empty item */
	*prevNext =(UriQueryList *)SAlloc::M(1*sizeof(UriQueryList));
	if(*prevNext == NULL) {
		return FALSE; /* Raises SAlloc::M error */
	}
	(*prevNext)->next = NULL;

	/* Fill key */
	key =(char *)SAlloc::M((keyLen+1)*sizeof(char));
	if(key == NULL) {
		SAlloc::F(*prevNext);
		*prevNext = NULL;
		return FALSE; /* Raises SAlloc::M error */
	}
	key[keyLen] = _UT('\0');
	if(keyLen > 0) {
		/* Copy 1:1 */
		memcpy(key, keyFirst, keyLen*sizeof(char));

		/* Unescape */
		UriUnescapeInPlaceEx(key, plusToSpace, breakConversion);
	}
	(*prevNext)->key = key;
	/* Fill value */
	if(valueFirst != NULL) {
		value =(char *)SAlloc::M((valueLen+1)*sizeof(char));
		if(value == NULL) {
			SAlloc::F(key);
			SAlloc::F(*prevNext);
			*prevNext = NULL;
			return FALSE; /* Raises SAlloc::M error */
		}
		value[valueLen] = _UT('\0');
		if(valueLen > 0) {
			/* Copy 1:1 */
			memcpy(value, valueFirst, valueLen*sizeof(char));

			/* Unescape */
			UriUnescapeInPlaceEx(value, plusToSpace, breakConversion);
		}
		(*prevNext)->value = value;
	}
	else {
		value = NULL;
	}
	(*prevNext)->value = value;

	(*itemCount)++;
	return TRUE;
}

void UriFreeQueryList(UriQueryList*queryList) {
	while(queryList != NULL) {
		UriQueryList*nextBackup = queryList->next;
		SAlloc::F((char *)queryList->key); /* const cast */
		SAlloc::F((char *)queryList->value); /* const cast */
		SAlloc::F(queryList);
		queryList = nextBackup;
	}
}

int UriDissectQueryMalloc(UriQueryList**dest, int * itemCount,
	const char * first, const char * afterLast) {
	const int plusToSpace = TRUE;
	const UriBreakConversion breakConversion = URI_BR_DONT_TOUCH;

	return UriDissectQueryMallocEx(dest, itemCount, first, afterLast,
		plusToSpace, breakConversion);
}

int UriDissectQueryMallocEx(UriQueryList**dest, int * itemCount,
	const char * first, const char * afterLast,
	int plusToSpace, UriBreakConversion breakConversion) {
	const char * walk = first;
	const char * keyFirst = first;
	const char * keyAfter = NULL;
	const char * valueFirst = NULL;
	const char * valueAfter = NULL;
	UriQueryList**prevNext = dest;
	int nullCounter;
	int * itemsAppended =(itemCount == NULL) ? &nullCounter : itemCount;
	if((dest == NULL) ||(first == NULL) ||(afterLast == NULL)) {
		return URI_ERROR_NULL;
	}
	if(first > afterLast) {
		return URI_ERROR_RANGE_INVALID;
	}
	*dest = NULL;
	*itemsAppended = 0;

	/* Parse query string */
	for(; walk < afterLast; walk++) {
		switch(*walk) {
		    case _UT('&'):
			if(valueFirst != NULL) {
				valueAfter = walk;
			}
			else {
				keyAfter = walk;
			}
			if(UriAppendQueryItem(prevNext, itemsAppended,
				   keyFirst, keyAfter, valueFirst, valueAfter,
				   plusToSpace, breakConversion)
			   == FALSE) {
				/* Free list we built */
				*itemsAppended = 0;
				UriFreeQueryList(*dest);
				return URI_ERROR_MALLOC;
			}
			/* Make future items children of the current */
			if((prevNext != NULL) &&(*prevNext != NULL)) {
				prevNext = &((*prevNext)->next);
			}
			if(walk+1 < afterLast) {
				keyFirst = walk+1;
			}
			else {
				keyFirst = NULL;
			}
			keyAfter = NULL;
			valueFirst = NULL;
			valueAfter = NULL;
			break;

		    case _UT('='):
			/* NOTE: WE treat the first '=' as a separator, */
			/*       all following go into the value part   */
			if(keyAfter == NULL) {
				keyAfter = walk;
				if(walk+1 < afterLast) {
					valueFirst = walk+1;
					valueAfter = walk+1;
				}
			}
			break;

		    default:
			break;
		}
	}
	if(valueFirst != NULL) {
		/* Must be key/value pair */
		valueAfter = walk;
	}
	else {
		/* Must be key only */
		keyAfter = walk;
	}
	if(UriAppendQueryItem(prevNext, itemsAppended, keyFirst, keyAfter, valueFirst, valueAfter, plusToSpace, breakConversion) == FALSE) {
		/* Free list we built */
		*itemsAppended = 0;
		UriFreeQueryList(*dest);
		return URI_ERROR_MALLOC;
	}
	return URI_SUCCESS;
}
//
//
//
static int UriFilenameToUriString(const char * filename, char * uriString, int fromUnix)
{
	const char * input = filename;
	const char * lastSep = input-1;
	int firstSegment = TRUE;
	char * output = uriString;
	const int absolute = (filename) && ((fromUnix && (filename[0] == _UT('/'))) || (!fromUnix &&(filename[0] != _UT('\0')) &&(filename[1] == _UT(':'))));
	if((filename == NULL) ||(uriString == NULL)) {
		return URI_ERROR_NULL;
	}
	if(absolute) {
		const char * const prefix = fromUnix ? _UT("file://") : _UT("file:///");
		const int prefixLen = fromUnix ? 7 : 8;
		/* Copy prefix */
		memcpy(uriString, prefix, prefixLen*sizeof(char));
		output += prefixLen;
	}
	/* Copy and escape on the fly */
	for(;; ) {
		if((input[0] == _UT('\0')) ||(fromUnix && input[0] == _UT('/')) ||(!fromUnix && input[0] == _UT('\\'))) {
			/* Copy text after last seperator */
			if(lastSep+1 < input) {
				if(!fromUnix && absolute &&(firstSegment == TRUE)) {
					/* Quick hack to not convert "C:" to "C%3A" */
					const int charsToCopy =(int)(input-(lastSep+1));
					memcpy(output, lastSep+1, charsToCopy*sizeof(char));
					output += charsToCopy;
				}
				else {
					output = UriEscapeEx(lastSep+1, input, output, FALSE, FALSE);
				}
			}
			firstSegment = FALSE;
		}
		if(input[0] == _UT('\0')) {
			output[0] = _UT('\0');
			break;
		}
		else if(fromUnix &&(input[0] == _UT('/'))) {
			/* Copy separators unmodified */
			output[0] = _UT('/');
			output++;
			lastSep = input;
		}
		else if(!fromUnix &&(input[0] == _UT('\\'))) {
			/* Convert backslashes to forward slashes */
			output[0] = _UT('/');
			output++;
			lastSep = input;
		}
		input++;
	}
	return URI_SUCCESS;
}

static int UriUriStringToFilename(const char * uriString, char * filename, int toUnix)
{
	const char * const prefix = toUnix ? _UT("file://") : _UT("file:///");
	const int prefixLen = toUnix ? 7 : 8;
	char * walker = filename;
	size_t charsToCopy;
	const int absolute =(strncmp(uriString, prefix, prefixLen) == 0);
	const int charsToSkip =(absolute ? prefixLen : 0);
	charsToCopy = strlen(uriString+charsToSkip)+1;
	memcpy(filename, uriString+charsToSkip, charsToCopy*sizeof(char));
	UriUnescapeInPlaceEx(filename, FALSE, URI_BR_DONT_TOUCH);
	/* Convert forward slashes to backslashes */
	if(!toUnix) {
		while(walker[0] != _UT('\0')) {
			if(walker[0] == _UT('/')) {
				walker[0] = _UT('\\');
			}
			walker++;
		}
	}
	return URI_SUCCESS;
}

int UriUnixFilenameToUriString(const char * filename, char * uriString)
{
	return UriFilenameToUriString(filename, uriString, TRUE);
}

int UriWindowsFilenameToUriString(const char * filename, char * uriString)
{
	return UriFilenameToUriString(filename, uriString, FALSE);
}

int UriUriStringToUnixFilename(const char * uriString, char * filename)
{
	return UriUriStringToFilename(uriString, filename, TRUE);
}

int UriUriStringToWindowsFilename(const char * uriString, char * filename)
{
	return UriUriStringToFilename(uriString, filename, FALSE);
}
//
//
//
char * UriEscape(const char * in, char * out, int spaceToPlus, int normalizeBreaks)
{
	return UriEscapeEx(in, NULL, out, spaceToPlus, normalizeBreaks);
}

char * UriEscapeEx(const char * inFirst, const char * inAfterLast, char * out, int spaceToPlus, int normalizeBreaks)
{
	const char * read = inFirst;
	char * write = out;
	int prevWasCr = FALSE;
	if((out == NULL) ||(inFirst == out)) {
		return NULL;
	}
	else if(inFirst == NULL) {
		if(out != NULL) {
			out[0] = _UT('\0');
		}
		return out;
	}
	for(;; ) {
		if((inAfterLast != NULL) &&(read >= inAfterLast)) {
			write[0] = _UT('\0');
			return write;
		}
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
		    case _UT('a'): /* ALPHA */
		    case _UT('A'):
		    case _UT('b'):
		    case _UT('B'):
		    case _UT('c'):
		    case _UT('C'):
		    case _UT('d'):
		    case _UT('D'):
		    case _UT('e'):
		    case _UT('E'):
		    case _UT('f'):
		    case _UT('F'):
		    case _UT('g'):
		    case _UT('G'):
		    case _UT('h'):
		    case _UT('H'):
		    case _UT('i'):
		    case _UT('I'):
		    case _UT('j'):
		    case _UT('J'):
		    case _UT('k'):
		    case _UT('K'):
		    case _UT('l'):
		    case _UT('L'):
		    case _UT('m'):
		    case _UT('M'):
		    case _UT('n'):
		    case _UT('N'):
		    case _UT('o'):
		    case _UT('O'):
		    case _UT('p'):
		    case _UT('P'):
		    case _UT('q'):
		    case _UT('Q'):
		    case _UT('r'):
		    case _UT('R'):
		    case _UT('s'):
		    case _UT('S'):
		    case _UT('t'):
		    case _UT('T'):
		    case _UT('u'):
		    case _UT('U'):
		    case _UT('v'):
		    case _UT('V'):
		    case _UT('w'):
		    case _UT('W'):
		    case _UT('x'):
		    case _UT('X'):
		    case _UT('y'):
		    case _UT('Y'):
		    case _UT('z'):
		    case _UT('Z'):
		    case _UT('0'): /* DIGIT */
		    case _UT('1'):
		    case _UT('2'):
		    case _UT('3'):
		    case _UT('4'):
		    case _UT('5'):
		    case _UT('6'):
		    case _UT('7'):
		    case _UT('8'):
		    case _UT('9'):
		    case _UT('-'): /* "-" / "." / "_" / "~" */
		    case _UT('.'):
		    case _UT('_'):
		    case _UT('~'):
				/* Copy unmodified */
				write[0] = read[0];
				write++;
				prevWasCr = FALSE;
				break;
		    case _UT('\x0a'):
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
		    case _UT('\x0d'):
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
				/* Percent encode */
		    {
			    const uchar code =(uchar)read[0];
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

const char * UriUnescapeInPlace(char * inout)
{
	return UriUnescapeInPlaceEx(inout, FALSE, URI_BR_DONT_TOUCH);
}

const char * UriUnescapeInPlaceEx(char * inout, int plusToSpace, UriBreakConversion breakConversion)
{
	char * read = inout;
	char * write = inout;
	int prevWasCr = FALSE;
	if(inout == NULL) {
		return NULL;
	}
	for(;; ) {
		switch(read[0]) {
		    case _UT('\0'):
				if(read > write)
					write[0] = _UT('\0');
				return write;
		    case _UT('%'):
			switch(read[1]) {
			    case _UT('0'):
			    case _UT('1'):
			    case _UT('2'):
			    case _UT('3'):
			    case _UT('4'):
			    case _UT('5'):
			    case _UT('6'):
			    case _UT('7'):
			    case _UT('8'):
			    case _UT('9'):
			    case _UT('a'):
			    case _UT('b'):
			    case _UT('c'):
			    case _UT('d'):
			    case _UT('e'):
			    case _UT('f'):
			    case _UT('A'):
			    case _UT('B'):
			    case _UT('C'):
			    case _UT('D'):
			    case _UT('E'):
			    case _UT('F'):
				switch(read[2]) {
				    case _UT('0'):
				    case _UT('1'):
				    case _UT('2'):
				    case _UT('3'):
				    case _UT('4'):
				    case _UT('5'):
				    case _UT('6'):
				    case _UT('7'):
				    case _UT('8'):
				    case _UT('9'):
				    case _UT('a'):
				    case _UT('b'):
				    case _UT('c'):
				    case _UT('d'):
				    case _UT('e'):
				    case _UT('f'):
				    case _UT('A'):
				    case _UT('B'):
				    case _UT('C'):
				    case _UT('D'):
				    case _UT('E'):
				    case _UT('F'):
				    {
					    /* Percent group found */
					    const uchar left = UriHexdigToInt(read[1]);
					    const uchar right = UriHexdigToInt(read[2]);
					    const int code = 16*left+right;
					    switch(code) {
							case 10:
								switch(breakConversion) {
									case URI_BR_TO_LF:
										if(!prevWasCr) {
											write[0] =(char)10;
											write++;
										}
										break;
									case URI_BR_TO_CRLF:
										if(!prevWasCr) {
											write[0] =(char)13;
											write[1] =(char)10;
											write += 2;
										}
										break;
									case URI_BR_TO_CR:
										if(!prevWasCr) {
											write[0] =(char)13;
											write++;
										}
										break;
									case URI_BR_DONT_TOUCH:
									default:
										write[0] =(char)10;
										write++;
								}
								prevWasCr = FALSE;
								break;
							case 13:
								switch(breakConversion) {
									case URI_BR_TO_LF:
										write[0] =(char)10;
										write++;
										break;
									case URI_BR_TO_CRLF:
										write[0] =(char)13;
										write[1] =(char)10;
										write += 2;
										break;
									case URI_BR_TO_CR:
										write[0] =(char)13;
										write++;
										break;
									case URI_BR_DONT_TOUCH:
									default:
										write[0] =(char)13;
										write++;
								}
								prevWasCr = TRUE;
								break;
							default:
								write[0] =(char)(code);
								write++;
								prevWasCr = FALSE;
							}
							read += 3;
						}
						break;
				    default:
						/* Copy two chars unmodified and */
						/* look at this char again */
						if(read > write) {
							write[0] = read[0];
							write[1] = read[1];
						}
						read += 2;
						write += 2;
						prevWasCr = FALSE;
				}
				break;
			    default:
					/* Copy one char unmodified and */
					/* look at this char again */
					if(read > write)
						write[0] = read[0];
					read++;
					write++;
					prevWasCr = FALSE;
			}
			break;
		    case _UT('+'):
				if(plusToSpace) {
					/* Convert '+' to ' ' */
					write[0] = _UT(' ');
				}
				else {
					/* Copy one char unmodified */
					if(read > write) {
						write[0] = read[0];
					}
				}
				read++;
				write++;
				prevWasCr = FALSE;
				break;
		    default:
				/* Copy one char unmodified */
				if(read > write) {
					write[0] = read[0];
				}
				read++;
				write++;
				prevWasCr = FALSE;
		}
	}
}
//
// Compares two text ranges for equal text content 
//
static int FASTCALL UriCompareRange(const UriTextRange * a, const UriTextRange * b)
{
	// NOTE: Both NULL means equal! 
	if((a == NULL) ||(b == NULL))
		return ((a == NULL) &&(b == NULL)) ? TRUE : FALSE;
	else {
		int    diff =((int)(a->afterLast-a->first)-(int)(b->afterLast-b->first));
		if(diff > 0)
			return 1;
		else if(diff < 0)
			return -1;
		else
			return strncmp(a->first, b->first,(a->afterLast-a->first));
	}
}

int UriEqualsUri(const UriUri * a, const UriUri * b)
{
	// NOTE: Both NULL means equal! 
	if((a == NULL) ||(b == NULL)) {
		return((a == NULL) &&(b == NULL)) ? TRUE : FALSE;
	}
	/* scheme */
	if(UriCompareRange(&(a->scheme), &(b->scheme))) {
		return FALSE;
	}
	/* absolutePath */
	if((a->scheme.first == NULL)&&(a->absolutePath != b->absolutePath)) {
		return FALSE;
	}
	/* userInfo */
	if(UriCompareRange(&(a->userInfo), &(b->userInfo))) {
		return FALSE;
	}
	/* Host */
	if(((a->hostData.ip4 == NULL) !=(b->hostData.ip4 == NULL)) || ((a->hostData.ip6 == NULL) !=(b->hostData.ip6 == NULL)) || ((a->hostData.ipFuture.first == NULL)
	    !=(b->hostData.ipFuture.first == NULL))) {
		return FALSE;
	}
	if(a->hostData.ip4 != NULL) {
		if(memcmp(a->hostData.ip4->data, b->hostData.ip4->data, 4)) {
			return FALSE;
		}
	}
	if(a->hostData.ip6 != NULL) {
		if(memcmp(a->hostData.ip6->data, b->hostData.ip6->data, 16)) {
			return FALSE;
		}
	}
	if(a->hostData.ipFuture.first != NULL) {
		if(UriCompareRange(&(a->hostData.ipFuture), &(b->hostData.ipFuture))) {
			return FALSE;
		}
	}
	if((a->hostData.ip4 == NULL) && (a->hostData.ip6 == NULL) && (a->hostData.ipFuture.first == NULL)) {
		if(UriCompareRange(&(a->hostText), &(b->hostText))) {
			return FALSE;
		}
	}
	/* portText */
	if(UriCompareRange(&(a->portText), &(b->portText))) {
		return FALSE;
	}
	/* Path */
	if((a->pathHead == NULL) !=(b->pathHead == NULL)) {
		return FALSE;
	}
	if(a->pathHead != NULL) {
		UriPathSegment*walkA = a->pathHead;
		UriPathSegment*walkB = b->pathHead;
		do {
			if(UriCompareRange(&(walkA->text), &(walkB->text))) {
				return FALSE;
			}
			else if((walkA->next == NULL) !=(walkB->next == NULL)) {
				return FALSE;
			}
			else {
				walkA = walkA->next;
				walkB = walkB->next;
			}
		} while(walkA != NULL);
	}
	/* query */
	if(UriCompareRange(&(a->query), &(b->query))) {
		return FALSE;
	}
	/* fragment */
	if(UriCompareRange(&(a->fragment), &(b->fragment))) {
		return FALSE;
	}
	return TRUE; /* Equal*/
}
//
//
//
/*extern*/ const char * const UriSafeToPointTo = _UT("X");
/*extern*/ const char * const UriConstPwd = _UT(".");
/*extern*/ const char * const UriConstParent = _UT("..");

void UriResetUri(UriUri * pUri)
{
	memzero(pUri, sizeof(*pUri));
}

/* Properly removes "." and ".." path segments */
int UriRemoveDotSegments(UriUri*uri, int relative)
{
	return(uri == NULL) ? TRUE : UriRemoveDotSegmentsEx(uri, relative, uri->owner);
}

int UriRemoveDotSegmentsEx(UriUri * uri, int relative, int pathOwned)
{
	if(!uri || !(uri->pathHead)) {
		return TRUE;
	}
	else {
		UriPathSegment * walker = uri->pathHead;
		walker->reserved = NULL; /* Prev pointer */
		do {
			int removeSegment = FALSE;
			int len =(int)(walker->text.afterLast-walker->text.first);
			switch(len) {
				case 1:
				if((walker->text.first)[0] == _UT('.')) {
					/* "." segment -> remove if not essential */
					UriPathSegment * const prev =(UriPathSegment *)walker->reserved;
					UriPathSegment * const nextBackup = walker->next;
					/* Is this dot segment essential? */
					removeSegment = TRUE;
					if(relative && walker == uri->pathHead && walker->next) {
						for(const char * ch = walker->next->text.first; ch < walker->next->text.afterLast; ch++) {
							if(*ch == _UT(':')) {
								removeSegment = FALSE;
								break;
							}
						}
					}
					if(removeSegment) {
						/* Last segment? */
						if(walker->next != NULL) {
							/* Not last segment */
							walker->next->reserved = prev;
							if(prev == NULL) {
								uri->pathHead = walker->next; // First but not last segment
							}
							else {
								prev->next = walker->next; // Middle segment
							}
							if(pathOwned &&(walker->text.first != walker->text.afterLast)) {
								SAlloc::F((char *)walker->text.first);
							}
							SAlloc::F(walker);
						}
						else {
							/* Last segment */
							if(pathOwned &&(walker->text.first != walker->text.afterLast)) {
								SAlloc::F((char *)walker->text.first);
							}
							if(prev == NULL) {
								/* Last and first */
								if(UriIsHostSet(uri)) {
									/* Replace "." with empty segment to represent trailing slash */
									walker->text.first = UriSafeToPointTo;
									walker->text.afterLast = UriSafeToPointTo;
								}
								else {
									SAlloc::F(walker);
									uri->pathHead = NULL;
									uri->pathTail = NULL;
								}
							}
							else {
								/* Last but not first, replace "." with empty segment to represent trailing slash */
								walker->text.first = UriSafeToPointTo;
								walker->text.afterLast = UriSafeToPointTo;
							}
						}
						walker = nextBackup;
					}
				}
				break;

				case 2:
				if(((walker->text.first)[0] == _UT('.')) &&((walker->text.first)[1] == _UT('.'))) {
					/* Path ".." -> remove this and the previous segment */
					UriPathSegment * const prev =(UriPathSegment *)walker->reserved;
					UriPathSegment * prevPrev;
					UriPathSegment * const nextBackup = walker->next;
					removeSegment = TRUE;
					if(relative) {
						if(prev == NULL) {
							removeSegment = FALSE;
						}
						else if((prev != NULL) &&((prev->text.afterLast-prev->text.first) == 2) &&
							((prev->text.first)[0] == _UT('.')) &&((prev->text.first)[1] == _UT('.'))) {
							removeSegment = FALSE;
						}
					}
					if(removeSegment) {
						if(prev != NULL) {
							/* Not first segment */
							prevPrev =(UriPathSegment *)prev->reserved;
							if(prevPrev != NULL) {
								/* Not even prev is the first one */
								prevPrev->next = walker->next;
								if(walker->next != NULL) {
									walker->next->reserved = prevPrev;
								}
								else {
									/* Last segment -> insert "" segment to represent trailing slash, update tail */
									UriPathSegment * const segment =(UriPathSegment *)SAlloc::M(1 * sizeof(UriPathSegment));
									if(segment == NULL) {
										if(pathOwned && (walker->text.first != walker->text.afterLast)) {
											SAlloc::F((char *)walker->text.first);
										}
										SAlloc::F(walker);
										if(pathOwned && (prev->text.first != prev->text.afterLast)) {
											SAlloc::F((char *)prev->text.first);
										}
										SAlloc::F(prev);
										return FALSE; /* Raises SAlloc::M error */
									}
									memzero(segment, sizeof(*segment));
									segment->text.first = UriSafeToPointTo;
									segment->text.afterLast = UriSafeToPointTo;
									prevPrev->next = segment;
									uri->pathTail = segment;
								}
								if(pathOwned && (walker->text.first != walker->text.afterLast)) {
									SAlloc::F((char *)walker->text.first);
								}
								SAlloc::F(walker);
								if(pathOwned &&(prev->text.first != prev->text.afterLast)) {
									SAlloc::F((char *)prev->text.first);
								}
								SAlloc::F(prev);
								walker = nextBackup;
							}
							else {
								/* Prev is the first segment */
								if(walker->next != NULL) {
									uri->pathHead = walker->next;
									walker->next->reserved = NULL;
									if(pathOwned && walker->text.first != walker->text.afterLast) {
										SAlloc::F((char *)walker->text.first);
									}
									SAlloc::F(walker);
								}
								else {
									/* Re-use segment for "" path segment to represent trailing slash, update tail */
									UriPathSegment * const segment = walker;
									if(pathOwned && segment->text.first != segment->text.afterLast) {
										SAlloc::F((char *)segment->text.first);
									}
									segment->text.first = UriSafeToPointTo;
									segment->text.afterLast = UriSafeToPointTo;
									uri->pathHead = segment;
									uri->pathTail = segment;
								}
								if(pathOwned &&(prev->text.first != prev->text.afterLast)) {
									SAlloc::F((char *)prev->text.first);
								}
								SAlloc::F(prev);
								walker = nextBackup;
							}
						}
						else {
							UriPathSegment * const nextBackup = walker->next;
							/* First segment -> update head pointer */
							uri->pathHead = walker->next;
							if(walker->next != NULL) {
								walker->next->reserved = NULL;
							}
							else {
								uri->pathTail = NULL; // Last segment -> update tail
							}
							if(pathOwned &&(walker->text.first != walker->text.afterLast)) {
								SAlloc::F((char *)walker->text.first);
							}
							SAlloc::F(walker);
							walker = nextBackup;
						}
					}
				}
				break;

			}
			if(!removeSegment) {
				if(walker->next != NULL) {
					walker->next->reserved = walker;
				}
				else {
					/* Last segment -> update tail */
					uri->pathTail = walker;
				}
				walker = walker->next;
			}
		} while(walker != NULL);
		return TRUE;
	}
}

/* Properly removes "." and ".." path segments */
int UriRemoveDotSegmentsAbsolute(UriUri * uri)
{
	const int absolute = FALSE;
	return UriRemoveDotSegments(uri, absolute);
}

uchar UriHexdigToInt(char hexdig) {
	switch(hexdig) {
	    case _UT('0'):
	    case _UT('1'):
	    case _UT('2'):
	    case _UT('3'):
	    case _UT('4'):
	    case _UT('5'):
	    case _UT('6'):
	    case _UT('7'):
	    case _UT('8'):
	    case _UT('9'):
		return(uchar)(9+hexdig-_UT('9'));

	    case _UT('a'):
	    case _UT('b'):
	    case _UT('c'):
	    case _UT('d'):
	    case _UT('e'):
	    case _UT('f'):
		return(uchar)(15+hexdig-_UT('f'));

	    case _UT('A'):
	    case _UT('B'):
	    case _UT('C'):
	    case _UT('D'):
	    case _UT('E'):
	    case _UT('F'):
		return(uchar)(15+hexdig-_UT('F'));

	    default:
		return 0;
	}
}

char UriHexToLetter(uint value) {
	/* Uppercase recommended in section 2.1. of RFC 3986 *
	* http://tools.ietf.org/html/rfc3986#section-2.1    */
	return UriHexToLetterEx(value, TRUE);
}

char UriHexToLetterEx(uint value, int uppercase) {
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

	    case 10: return(uppercase == TRUE) ? _UT('A') : _UT('a');
	    case 11: return(uppercase == TRUE) ? _UT('B') : _UT('b');
	    case 12: return(uppercase == TRUE) ? _UT('C') : _UT('c');
	    case 13: return(uppercase == TRUE) ? _UT('D') : _UT('d');
	    case 14: return(uppercase == TRUE) ? _UT('E') : _UT('e');
	    default: return(uppercase == TRUE) ? _UT('F') : _UT('f');
	}
}
//
// Checks if a URI has the host component set
//
int UriIsHostSet(const UriUri * uri)
{
	return (uri && (uri->hostText.first || uri->hostData.ip4 || uri->hostData.ip6 || uri->hostData.ipFuture.first));
}
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
		UriPathSegment * sourceWalker = source->pathHead;
		UriPathSegment * destPrev = NULL;
		do {
			UriPathSegment * cur =(UriPathSegment *)SAlloc::M(sizeof(UriPathSegment));
			if(cur == NULL) {
				// Fix broken list 
				if(destPrev != NULL)
					destPrev->next = NULL;
				return FALSE; // Raises SAlloc::M error 
			}
			else {
				// From this functions usage we know that the dest URI cannot be uri->owner
				cur->text = sourceWalker->text;
				if(destPrev == NULL) {
					dest->pathHead = cur; // First segment ever 
				}
				else {
					destPrev->next = cur;
				}
				destPrev = cur;
				sourceWalker = sourceWalker->next;
			}
		} while(sourceWalker != NULL);
		dest->pathTail = destPrev;
		dest->pathTail->next = NULL;
	}
	dest->absolutePath = source->absolutePath;
	return TRUE;
}

/* Copies the authority part of an URI over to another. */
int UriCopyAuthority(UriUri*dest, const UriUri*source)
{
	/* From this functions usage we know that *
	* the dest URI cannot be uri->owner      */

	/* Copy userInfo */
	dest->userInfo = source->userInfo;
	/* Copy hostText */
	dest->hostText = source->hostText;
	/* Copy hostData */
	if(source->hostData.ip4 != NULL) {
		dest->hostData.ip4 =(UriIp4 *)SAlloc::M(sizeof(UriIp4));
		if(dest->hostData.ip4 == NULL) {
			return FALSE; /* Raises SAlloc::M error */
		}
		*(dest->hostData.ip4) = *(source->hostData.ip4);
		dest->hostData.ip6 = NULL;
		dest->hostData.ipFuture.first = NULL;
		dest->hostData.ipFuture.afterLast = NULL;
	}
	else if(source->hostData.ip6 != NULL) {
		dest->hostData.ip4 = NULL;
		dest->hostData.ip6 =(UriIp6 *)SAlloc::M(sizeof(UriIp6));
		if(dest->hostData.ip6 == NULL) {
			return FALSE; /* Raises SAlloc::M error */
		}
		*(dest->hostData.ip6) = *(source->hostData.ip6);
		dest->hostData.ipFuture.first = NULL;
		dest->hostData.ipFuture.afterLast = NULL;
	}
	else {
		dest->hostData.ip4 = NULL;
		dest->hostData.ip6 = NULL;
		dest->hostData.ipFuture = source->hostData.ipFuture;
	}
	/* Copy portText */
	dest->portText = source->portText;
	return TRUE;
}

int UriFixAmbiguity(UriUri*uri)
{
	UriPathSegment*segment;
	if(     /* Case 1: absolute path, empty first segment */
	       (uri->absolutePath &&(uri->pathHead != NULL) &&(uri->pathHead->text.afterLast == uri->pathHead->text.first))

	        /* Case 2: relative path, empty first and second segment */
	        ||(!uri->absolutePath &&(uri->pathHead != NULL) &&(uri->pathHead->next != NULL) &&
	           (uri->pathHead->text.afterLast == uri->pathHead->text.first) &&
	           (uri->pathHead->next->text.afterLast == uri->pathHead->next->text.first))) {
		/* NOOP */
	}
	else {
		return TRUE;
	}
	segment =(UriPathSegment *)SAlloc::M(1 * sizeof(UriPathSegment));
	if(segment == NULL) {
		return FALSE; /* Raises SAlloc::M error */
	}
	/* Insert "." segment in front */
	segment->next = uri->pathHead;
	segment->text.first = UriConstPwd;
	segment->text.afterLast = UriConstPwd+1;
	uri->pathHead = segment;
	return TRUE;
}

void UriFixEmptyTrailSegment(UriUri*uri)
{
	/* Fix path if only one empty segment */
	if(!uri->absolutePath && !UriIsHostSet(uri) && (uri->pathHead != NULL) &&
	  (uri->pathHead->next == NULL) && (uri->pathHead->text.first == uri->pathHead->text.afterLast)) {
		ZFREE(uri->pathHead);
		uri->pathTail = NULL;
	}
}
//
//
//
static int UriNormalizeSyntaxEngine(UriUri*uri, uint inMask, uint * outMask);
static int UriMakeRangeOwner(uint * doneMask, uint maskTest, UriTextRange*range);
static int FASTCALL UriMakeOwner(UriUri*uri, uint * doneMask);
static void FASTCALL UriFixPercentEncodingInplace(const char * first, const char ** afterLast);
static void UriFixPercentEncodingEngine(const char * inFirst, const char * inAfterLast, const char * outFirst, const char * *outAfterLast);
static int FASTCALL UriContainsUglyPercentEncoding(const char * first, const char * afterLast);
static void FASTCALL UriLowercaseInplace(const char * first, const char * afterLast);
static int FASTCALL UriLowercaseMalloc(const char ** first, const char ** afterLast);
static void UriPreventLeakage(UriUri*uri, uint revertMask);

static void UriPreventLeakage(UriUri*uri, uint revertMask)
{
	if(revertMask&URI_NORMALIZE_SCHEME) {
		SAlloc::F((char *)uri->scheme.first);
		uri->scheme.first = NULL;
		uri->scheme.afterLast = NULL;
	}
	if(revertMask&URI_NORMALIZE_USER_INFO) {
		SAlloc::F((char *)uri->userInfo.first);
		uri->userInfo.first = NULL;
		uri->userInfo.afterLast = NULL;
	}
	if(revertMask&URI_NORMALIZE_HOST) {
		if(uri->hostData.ipFuture.first != NULL) {
			/* IPvFuture */
			SAlloc::F((char *)uri->hostData.ipFuture.first);
			uri->hostData.ipFuture.first = NULL;
			uri->hostData.ipFuture.afterLast = NULL;
			uri->hostText.first = NULL;
			uri->hostText.afterLast = NULL;
		}
		else if((uri->hostText.first != NULL) &&(uri->hostData.ip4 == NULL) &&(uri->hostData.ip6 == NULL)) {
			/* Regname */
			SAlloc::F((char *)uri->hostText.first);
			uri->hostText.first = NULL;
			uri->hostText.afterLast = NULL;
		}
	}
	/* NOTE: Port cannot happen! */
	if(revertMask&URI_NORMALIZE_PATH) {
		UriPathSegment*walker = uri->pathHead;
		while(walker != NULL) {
			UriPathSegment*const next = walker->next;
			if(walker->text.afterLast > walker->text.first) {
				SAlloc::F((char *)walker->text.first);
			}
			SAlloc::F(walker);
			walker = next;
		}
		uri->pathHead = NULL;
		uri->pathTail = NULL;
	}
	if(revertMask&URI_NORMALIZE_QUERY) {
		SAlloc::F((char *)uri->query.first);
		uri->query.first = NULL;
		uri->query.afterLast = NULL;
	}
	if(revertMask&URI_NORMALIZE_FRAGMENT) {
		SAlloc::F((char *)uri->fragment.first);
		uri->fragment.first = NULL;
		uri->fragment.afterLast = NULL;
	}
}

static int FASTCALL UriContainsUppercaseLetters(const char * first, const char * afterLast)
{
	if((first != NULL) &&(afterLast != NULL) &&(afterLast > first)) {
		const char * i = first;
		for(; i < afterLast; i++) {
			// 6.2.2.1 Case Normalization: uppercase letters in scheme or host 
			if((*i >= _UT('A')) &&(*i <= _UT('Z'))) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static int FASTCALL UriContainsUglyPercentEncoding(const char * first, const char * afterLast)
{
	if((first != NULL) &&(afterLast != NULL) &&(afterLast > first)) {
		const char * i = first;
		for(; i+2 < afterLast; i++) {
			if(i[0] == _UT('%')) {
				/* 6.2.2.1 Case Normalization: *
				* lowercase percent-encodings */
				if(((i[1] >= _UT('a')) &&(i[1] <= _UT('f'))) ||
				  ((i[2] >= _UT('a')) &&(i[2] <= _UT('f')))) {
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

static void FASTCALL UriLowercaseInplace(const char * first, const char * afterLast)
{
	if((first != NULL) &&(afterLast != NULL) &&(afterLast > first)) {
		char * i =(char *)first;
		const int lowerUpperDiff =(_UT('a')-_UT('A'));
		for(; i < afterLast; i++) {
			if((*i >= _UT('A')) &&(*i <=_UT('Z'))) {
				*i =(char)(*i+lowerUpperDiff);
			}
		}
	}
}

static int FASTCALL UriLowercaseMalloc(const char ** first, const char ** afterLast)
{
	int    ok = 1;
	const  int lowerUpperDiff =(_UT('a')-_UT('A'));
	char * buffer;
	int i = 0;
	if(!first || !afterLast || (*first == NULL) || (*afterLast == NULL)) {
		ok = 0;
	}
	else {
		int    lenInChars =(int)(*afterLast-*first);
		if(lenInChars < 0) {
			ok = 0;
		}
		else if(lenInChars > 0) {
			buffer =(char *)SAlloc::M(lenInChars * sizeof(char));
			if(buffer == NULL) {
				ok = 0;
			}
			else {
				for(; i < lenInChars; i++) {
					if(((*first)[i] >= _UT('A')) &&((*first)[i] <=_UT('Z'))) {
						buffer[i] =(char)((*first)[i]+lowerUpperDiff);
					}
					else {
						buffer[i] =(*first)[i];
					}
				}
				*first = buffer;
				*afterLast = buffer+lenInChars;
			}
		}
	}
	return ok;
}

/* NOTE: Implementation must stay inplace-compatible */
static void UriFixPercentEncodingEngine(const char * inFirst, const char * inAfterLast, const char * outFirst, const char * *outAfterLast)
{
	char * write =(char *)outFirst;
	const int lenInChars =(int)(inAfterLast-inFirst);
	int i = 0;
	/* All but last two */
	for(; i+2 < lenInChars; i++) {
		if(inFirst[i] != _UT('%')) {
			write[0] = inFirst[i];
			write++;
		}
		else {
			/* 6.2.2.2 Percent-Encoding Normalization: *
			* percent-encoded unreserved characters   */
			const char one = inFirst[i+1];
			const char two = inFirst[i+2];
			const uchar left = UriHexdigToInt(one);
			const uchar right = UriHexdigToInt(two);
			const int code = 16*left+right;
			if(uriIsUnreserved(code)) {
				write[0] =(char)(code);
				write++;
			}
			else {
				/* 6.2.2.1 Case Normalization: *
				* lowercase percent-encodings */
				write[0] = _UT('%');
				write[1] = UriHexToLetter(left);
				write[2] = UriHexToLetter(right);
				write += 3;
			}
			i += 2; /* For the two chars of the percent group we just ate */
		}
	}
	/* Last two */
	for(; i < lenInChars; i++) {
		write[0] = inFirst[i];
		write++;
	}
	*outAfterLast = write;
}

static void FASTCALL UriFixPercentEncodingInplace(const char * first, const char ** afterLast) {
	/* Death checks */
	if(first && afterLast && *afterLast) {
		/* Fix inplace */
		UriFixPercentEncodingEngine(first, *afterLast, first, afterLast);
	}
}

static int FASTCALL UriFixPercentEncodingMalloc(const char ** first, const char ** afterLast)
{
	// Death checks 
	if((first == NULL) ||(afterLast == NULL) ||(*first == NULL) ||(*afterLast == NULL)) {
		return FALSE;
	}
	else {
		// Old text length 
		int lenInChars =(int)(*afterLast-*first);
		if(lenInChars == 0) {
			return TRUE;
		}
		else if(lenInChars < 0) {
			return FALSE;
		}
		else {
			// New buffer 
			char * buffer =(char *)SAlloc::M(lenInChars * sizeof(char));
			if(buffer == NULL) {
				return FALSE;
			}
			else {
				// Fix on copy 
				UriFixPercentEncodingEngine(*first, *afterLast, buffer, afterLast);
				*first = buffer;
				return TRUE;
			}
		}
	}
}

static int UriMakeRangeOwner(uint * doneMask, uint maskTest, UriTextRange*range)
{
	if(((*doneMask&maskTest) == 0) &&(range->first != NULL) &&(range->afterLast != NULL) &&(range->afterLast > range->first)) {
		const int lenInChars =(int)(range->afterLast-range->first);
		const int lenInBytes = lenInChars*sizeof(char);
		char * dup =(char *)SAlloc::M(lenInBytes);
		if(dup == NULL) {
			return FALSE; /* Raises SAlloc::M error */
		}
		memcpy(dup, range->first, lenInBytes);
		range->first = dup;
		range->afterLast = dup+lenInChars;
		*doneMask |= maskTest;
	}
	return TRUE;
}

static int FASTCALL UriMakeOwner(UriUri * uri, uint * doneMask)
{
	UriPathSegment * walker = uri->pathHead;
	if(!UriMakeRangeOwner(doneMask, URI_NORMALIZE_SCHEME, &(uri->scheme)) ||
	   !UriMakeRangeOwner(doneMask, URI_NORMALIZE_USER_INFO, &(uri->userInfo)) ||
	   !UriMakeRangeOwner(doneMask, URI_NORMALIZE_QUERY, &(uri->query)) ||
	   !UriMakeRangeOwner(doneMask, URI_NORMALIZE_FRAGMENT, &(uri->fragment))) {
		return FALSE; /* Raises SAlloc::M error */
	}
	/* Host */
	if((*doneMask&URI_NORMALIZE_HOST) == 0) {
		if(!uri->hostData.ip4 && !uri->hostData.ip6) {
			if(uri->hostData.ipFuture.first != NULL) {
				/* IPvFuture */
				if(!UriMakeRangeOwner(doneMask, URI_NORMALIZE_HOST, &(uri->hostData.ipFuture))) {
					return FALSE; /* Raises SAlloc::M error */
				}
				uri->hostText.first = uri->hostData.ipFuture.first;
				uri->hostText.afterLast = uri->hostData.ipFuture.afterLast;
			}
			else if(uri->hostText.first != NULL) {
				/* Regname */
				if(!UriMakeRangeOwner(doneMask, URI_NORMALIZE_HOST, &(uri->hostText))) {
					return FALSE; /* Raises SAlloc::M error */
				}
			}
		}
	}
	/* Path */
	if((*doneMask&URI_NORMALIZE_PATH) == 0) {
		while(walker) {
			if(!UriMakeRangeOwner(doneMask, 0, &(walker->text))) {
				/* Kill path to one before walker */
				UriPathSegment * ranger = uri->pathHead;
				while(ranger->next != walker) {
					UriPathSegment * const next = ranger->next;
					if(ranger->text.first && ranger->text.afterLast && (ranger->text.afterLast > ranger->text.first)) {
						SAlloc::F((char *)ranger->text.first);
						SAlloc::F(ranger);
					}
					ranger = next;
				}
				/* Kill path from walker */
				while(walker) {
					UriPathSegment * const next = walker->next;
					SAlloc::F(walker);
					walker = next;
				}
				uri->pathHead = NULL;
				uri->pathTail = NULL;
				return FALSE; /* Raises SAlloc::M error */
			}
			walker = walker->next;
		}
		*doneMask |= URI_NORMALIZE_PATH;
	}
	// Port text, must come last so we don't have to undo that one if it fails.
	// Otherwise we would need and extra enum flag for it although the port      
	// cannot go unnormalized...  
	if(!UriMakeRangeOwner(doneMask, 0, &(uri->portText))) {
		return FALSE; /* Raises SAlloc::M error */
	}
	return TRUE;
}

uint FASTCALL UriNormalizeSyntaxMaskRequired(const UriUri * uri)
{
	uint res;
 #if defined(__GNUC__) &&((__GNUC__ > 4) ||((__GNUC__ == 4) && defined(__GNUC_MINOR__) &&(__GNUC_MINOR__ >= 2)))
	/* Slower code that fixes a warning, not sure if this is a smart idea */
	UriUri writeableClone;
	memcpy(&writeableClone, uri, 1*sizeof(UriUri));
	UriNormalizeSyntaxEngine(&writeableClone, 0, &res);
 #else
	UriNormalizeSyntaxEngine((UriUri *)uri, 0, &res);
 #endif
	return res;
}

int FASTCALL UriNormalizeSyntaxEx(UriUri * uri, uint mask)
{
	return UriNormalizeSyntaxEngine(uri, mask, NULL);
}

int FASTCALL UriNormalizeSyntax(UriUri * uri)
{
	return UriNormalizeSyntaxEx(uri,(uint)-1);
}

static int UriNormalizeSyntaxEngine(UriUri*uri, uint inMask, uint * outMask)
{
	uint doneMask = URI_NORMALIZED;
	if(uri == NULL) {
		if(outMask != NULL) {
			*outMask = URI_NORMALIZED;
			return URI_SUCCESS;
		}
		else {
			return URI_ERROR_NULL;
		}
	}
	if(outMask != NULL) {
		/* Reset mask */
		*outMask = URI_NORMALIZED;
	}
	else if(inMask == URI_NORMALIZED) {
		/* Nothing to do */
		return URI_SUCCESS;
	}
	/* Scheme, host */
	if(outMask != NULL) {
		const int normalizeScheme = UriContainsUppercaseLetters(uri->scheme.first, uri->scheme.afterLast);
		const int normalizeHostCase = UriContainsUppercaseLetters(uri->hostText.first, uri->hostText.afterLast);
		if(normalizeScheme) {
			*outMask |= URI_NORMALIZE_SCHEME;
		}
		if(normalizeHostCase) {
			*outMask |= URI_NORMALIZE_HOST;
		}
		else {
			const int normalizeHostPrecent = UriContainsUglyPercentEncoding(uri->hostText.first, uri->hostText.afterLast);
			if(normalizeHostPrecent) {
				*outMask |= URI_NORMALIZE_HOST;
			}
		}
	}
	else {
		/* Scheme */
		if((inMask&URI_NORMALIZE_SCHEME) &&(uri->scheme.first != NULL)) {
			if(uri->owner) {
				UriLowercaseInplace(uri->scheme.first, uri->scheme.afterLast);
			}
			else {
				if(!UriLowercaseMalloc(&(uri->scheme.first), &(uri->scheme.afterLast))) {
					UriPreventLeakage(uri, doneMask);
					return URI_ERROR_MALLOC;
				}
				doneMask |= URI_NORMALIZE_SCHEME;
			}
		}
		/* Host */
		if(inMask&URI_NORMALIZE_HOST) {
			if(uri->hostData.ipFuture.first != NULL) {
				/* IPvFuture */
				if(uri->owner) {
					UriLowercaseInplace(uri->hostData.ipFuture.first, uri->hostData.ipFuture.afterLast);
				}
				else {
					if(!UriLowercaseMalloc(&(uri->hostData.ipFuture.first), &(uri->hostData.ipFuture.afterLast))) {
						UriPreventLeakage(uri, doneMask);
						return URI_ERROR_MALLOC;
					}
					doneMask |= URI_NORMALIZE_HOST;
				}
				uri->hostText.first = uri->hostData.ipFuture.first;
				uri->hostText.afterLast = uri->hostData.ipFuture.afterLast;
			}
			else if((uri->hostText.first != NULL) &&(uri->hostData.ip4 == NULL) &&(uri->hostData.ip6 == NULL)) {
				/* Regname */
				if(uri->owner) {
					UriFixPercentEncodingInplace(uri->hostText.first, &(uri->hostText.afterLast));
				}
				else {
					if(!UriFixPercentEncodingMalloc(&(uri->hostText.first), &(uri->hostText.afterLast))) {
						UriPreventLeakage(uri, doneMask);
						return URI_ERROR_MALLOC;
					}
					doneMask |= URI_NORMALIZE_HOST;
				}
				UriLowercaseInplace(uri->hostText.first, uri->hostText.afterLast);
			}
		}
	}
	/* User info */
	if(outMask != NULL) {
		const int normalizeUserInfo = UriContainsUglyPercentEncoding(uri->userInfo.first, uri->userInfo.afterLast);
		if(normalizeUserInfo) {
			*outMask |= URI_NORMALIZE_USER_INFO;
		}
	}
	else {
		if((inMask&URI_NORMALIZE_USER_INFO) &&(uri->userInfo.first != NULL)) {
			if(uri->owner) {
				UriFixPercentEncodingInplace(uri->userInfo.first, &(uri->userInfo.afterLast));
			}
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->userInfo.first), &(uri->userInfo.afterLast))) {
					UriPreventLeakage(uri, doneMask);
					return URI_ERROR_MALLOC;
				}
				doneMask |= URI_NORMALIZE_USER_INFO;
			}
		}
	}
	/* Path */
	if(outMask != NULL) {
		const UriPathSegment*walker = uri->pathHead;
		while(walker != NULL) {
			const char * const first = walker->text.first;
			const char * const afterLast = walker->text.afterLast;
			if((first != NULL) &&(afterLast != NULL) &&(afterLast > first) &&(
				(((afterLast-first) == 1) &&(first[0] == _UT('.'))) ||
				(((afterLast-first) == 2) &&(first[0] == _UT('.')) &&(first[1] == _UT('.'))) ||
				UriContainsUglyPercentEncoding(first, afterLast))) {
				*outMask |= URI_NORMALIZE_PATH;
				break;
			}
			walker = walker->next;
		}
	}
	else if(inMask&URI_NORMALIZE_PATH) {
		UriPathSegment*walker;
		const int relative =((uri->scheme.first == NULL) && !uri->absolutePath) ? TRUE : FALSE;
		/* Fix percent-encoding for each segment */
		walker = uri->pathHead;
		if(uri->owner) {
			while(walker != NULL) {
				UriFixPercentEncodingInplace(walker->text.first, &(walker->text.afterLast));
				walker = walker->next;
			}
		}
		else {
			while(walker != NULL) {
				if(!UriFixPercentEncodingMalloc(&(walker->text.first), &(walker->text.afterLast))) {
					UriPreventLeakage(uri, doneMask);
					return URI_ERROR_MALLOC;
				}
				walker = walker->next;
			}
			doneMask |= URI_NORMALIZE_PATH;
		}
		/* 6.2.2.3 Path Segment Normalization */
		if(!UriRemoveDotSegmentsEx(uri, relative,(uri->owner == TRUE) ||((doneMask&URI_NORMALIZE_PATH) != 0))) {
			UriPreventLeakage(uri, doneMask);
			return URI_ERROR_MALLOC;
		}
		UriFixEmptyTrailSegment(uri);
	}
	/* Query, fragment */
	if(outMask != NULL) {
		const int normalizeQuery = UriContainsUglyPercentEncoding(uri->query.first, uri->query.afterLast);
		const int normalizeFragment = UriContainsUglyPercentEncoding(uri->fragment.first, uri->fragment.afterLast);
		if(normalizeQuery) {
			*outMask |= URI_NORMALIZE_QUERY;
		}
		if(normalizeFragment) {
			*outMask |= URI_NORMALIZE_FRAGMENT;
		}
	}
	else {
		/* Query */
		if((inMask&URI_NORMALIZE_QUERY) && uri->query.first) {
			if(uri->owner) {
				UriFixPercentEncodingInplace(uri->query.first, &(uri->query.afterLast));
			}
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->query.first), &(uri->query.afterLast))) {
					UriPreventLeakage(uri, doneMask);
					return URI_ERROR_MALLOC;
				}
				doneMask |= URI_NORMALIZE_QUERY;
			}
		}
		/* Fragment */
		if((inMask&URI_NORMALIZE_FRAGMENT) && uri->fragment.first) {
			if(uri->owner) {
				UriFixPercentEncodingInplace(uri->fragment.first, &(uri->fragment.afterLast));
			}
			else {
				if(!UriFixPercentEncodingMalloc(&(uri->fragment.first), &(uri->fragment.afterLast))) {
					UriPreventLeakage(uri, doneMask);
					return URI_ERROR_MALLOC;
				}
				doneMask |= URI_NORMALIZE_FRAGMENT;
			}
		}
	}
	/* Dup all not duped yet */
	if((outMask == NULL) && !uri->owner) {
		if(!UriMakeOwner(uri, &doneMask)) {
			UriPreventLeakage(uri, doneMask);
			return URI_ERROR_MALLOC;
		}
		uri->owner = TRUE;
	}
	return URI_SUCCESS;
}
//
//
//
static int UriToStringEngine(char * dest, const UriUri*uri, int maxChars, int * charsWritten, int * charsRequired);

int FASTCALL UriToStringCharsRequired(const UriUri*uri, int * charsRequired)
{
	const int MAX_CHARS =((uint)-1)>>1;
	return UriToStringEngine(NULL, uri, MAX_CHARS, NULL, charsRequired);
}

int UriToString(char * dest, const UriUri*uri, int maxChars, int * charsWritten)
{
	return UriToStringEngine(dest, uri, maxChars, charsWritten, NULL);
}

static int UriToStringEngine(char * dest, const UriUri*uri, int maxChars, int * charsWritten, int * charsRequired)
{
	int written = 0;
	if((uri == NULL) ||((dest == NULL) &&(charsRequired == NULL))) {
		if(charsWritten != NULL) {
			*charsWritten = 0;
		}
		return URI_ERROR_NULL;
	}
	if(maxChars < 1) {
		if(charsWritten != NULL) {
			*charsWritten = 0;
		}
		return URI_ERROR_TOSTRING_TOO_LONG;
	}
	maxChars--; /* So we don't have to substract 1 for '\0' all the time */
	/* [01/19]	result = "" */
	if(dest != NULL) {
		dest[0] = _UT('\0');
	}
	else {
		(*charsRequired) = 0;
	}
	/* [02/19]	if defined(scheme) then */
	if(uri->scheme.first != NULL) {
		/* [03/19]		append scheme to result; */
		const int charsToWrite =(int)(uri->scheme.afterLast-uri->scheme.first);
		if(dest != NULL) {
			if(written+charsToWrite <= maxChars) {
				memcpy(dest+written, uri->scheme.first, charsToWrite*sizeof(char));
				written += charsToWrite;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += charsToWrite;
		}
		/* [04/19]		append ":" to result; */
		if(dest != NULL) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT(":"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
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
		if(dest != NULL) {
			if(written+2 <= maxChars) {
				memcpy(dest+written, _UT("//"), 2*sizeof(char));
				written += 2;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 2;
		}
		/* [08/19]		append authority to result; */
		/* UserInfo */
		if(uri->userInfo.first != NULL) {
			const int charsToWrite =(int)(uri->userInfo.afterLast-uri->userInfo.first);
			if(dest != NULL) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->userInfo.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("@"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite+1;
			}
		}
		/* Host */
		if(uri->hostData.ip4 != NULL) {
			/* IPv4 */
			int i = 0;
			for(; i < 4; i++) {
				const uchar value = uri->hostData.ip4->data[i];
				const int charsToWrite =(value > 99) ? 3 :((value > 9) ? 2 : 1);
				if(dest != NULL) {
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
						if(charsWritten != NULL) {
							*charsWritten = 0;
						}
						return URI_ERROR_TOSTRING_TOO_LONG;
					}
					if(i < 3) {
						if(written+1 <= maxChars) {
							memcpy(dest+written, _UT("."), 1*sizeof(char));
							written += 1;
						}
						else {
							dest[0] = _UT('\0');
							if(charsWritten != NULL) {
								*charsWritten = 0;
							}
							return URI_ERROR_TOSTRING_TOO_LONG;
						}
					}
				}
				else {
					(*charsRequired) += charsToWrite+1;
				}
			}
		}
		else if(uri->hostData.ip6 != NULL) {
			/* IPv6 */
			int i = 0;
			if(dest != NULL) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("["), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1;
			}
			for(; i < 16; i++) {
				const uchar value = uri->hostData.ip6->data[i];
				if(dest != NULL) {
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
						if(charsWritten != NULL) {
							*charsWritten = 0;
						}
						return URI_ERROR_TOSTRING_TOO_LONG;
					}
				}
				else {
					(*charsRequired) += 2;
				}
				if(((i&1) == 1) &&(i < 15)) {
					if(dest != NULL) {
						if(written+1 <= maxChars) {
							memcpy(dest+written, _UT(":"), 1*sizeof(char));
							written += 1;
						}
						else {
							dest[0] = _UT('\0');
							if(charsWritten != NULL) {
								*charsWritten = 0;
							}
							return URI_ERROR_TOSTRING_TOO_LONG;
						}
					}
					else {
						(*charsRequired) += 1;
					}
				}
			}
			if(dest != NULL) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("]"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1;
			}
		}
		else if(uri->hostData.ipFuture.first != NULL) {
			/* IPvFuture */
			const int charsToWrite =(int)(uri->hostData.ipFuture.afterLast-uri->hostData.ipFuture.first);
			if(dest != NULL) {
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("["), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->hostData.ipFuture.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT("]"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1+charsToWrite+1;
			}
		}
		else if(uri->hostText.first != NULL) {
			/* Regname */
			const int charsToWrite =(int)(uri->hostText.afterLast-uri->hostText.first);
			if(dest != NULL) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->hostText.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		/* Port */
		if(uri->portText.first != NULL) {
			const int charsToWrite =(int)(uri->portText.afterLast-uri->portText.first);
			if(dest != NULL) {
				/* Leading ':' */
				if(written+1 <= maxChars) {
					memcpy(dest+written, _UT(":"), 1*sizeof(char));
					written += 1;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
				/* Port number */
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->portText.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += 1+charsToWrite;
			}
		}
		/* [09/19]	endif; */
	}
	/* [10/19]	append path to result; */
	/* Slash needed here? */
	if(uri->absolutePath ||((uri->pathHead != NULL) && UriIsHostSet(uri))) {
		if(dest != NULL) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("/"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
	}
	if(uri->pathHead != NULL) {
		UriPathSegment*walker = uri->pathHead;
		do {
			const int charsToWrite =(int)(walker->text.afterLast-walker->text.first);
			if(dest != NULL) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, walker->text.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
			/* Not last segment -> append slash */
			if(walker->next != NULL) {
				if(dest != NULL) {
					if(written+1 <= maxChars) {
						memcpy(dest+written, _UT("/"), 1*sizeof(char));
						written += 1;
					}
					else {
						dest[0] = _UT('\0');
						if(charsWritten != NULL) {
							*charsWritten = 0;
						}
						return URI_ERROR_TOSTRING_TOO_LONG;
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
	if(uri->query.first != NULL) {
		/* [12/19]		append "?" to result; */
		if(dest != NULL) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("?"),
					1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
		/* [13/19]		append query to result; */
		{
			const int charsToWrite =
			       (int)(uri->query.afterLast-uri->query.first);
			if(dest != NULL) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->query.first,
						charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		/* [14/19]	endif; */
	}
	/* [15/19]	if defined(fragment) then */
	if(uri->fragment.first != NULL) {
		/* [16/19]		append "#" to result; */
		if(dest != NULL) {
			if(written+1 <= maxChars) {
				memcpy(dest+written, _UT("#"), 1*sizeof(char));
				written += 1;
			}
			else {
				dest[0] = _UT('\0');
				if(charsWritten != NULL) {
					*charsWritten = 0;
				}
				return URI_ERROR_TOSTRING_TOO_LONG;
			}
		}
		else {
			(*charsRequired) += 1;
		}
		/* [17/19]		append fragment to result; */
		{
			const int charsToWrite = (int)(uri->fragment.afterLast-uri->fragment.first);
			if(dest != NULL) {
				if(written+charsToWrite <= maxChars) {
					memcpy(dest+written, uri->fragment.first, charsToWrite*sizeof(char));
					written += charsToWrite;
				}
				else {
					dest[0] = _UT('\0');
					if(charsWritten != NULL) {
						*charsWritten = 0;
					}
					return URI_ERROR_TOSTRING_TOO_LONG;
				}
			}
			else {
				(*charsRequired) += charsToWrite;
			}
		}
		/* [18/19]	endif; */
	}
	/* [19/19]	return result; */
	if(dest != NULL) {
		dest[written++] = _UT('\0');
		if(charsWritten != NULL) {
			*charsWritten = written;
		}
	}
	return URI_SUCCESS;
}
//
// Prototypes
//
static const char * UriParseDecOctet(UriIp4Parser*parser, const char * first, const char * afterLast);
static const char * UriParseDecOctetOne(UriIp4Parser*parser, const char * first, const char * afterLast);
static const char * UriParseDecOctetTwo(UriIp4Parser*parser, const char * first, const char * afterLast);
static const char * UriParseDecOctetThree(UriIp4Parser*parser, const char * first, const char * afterLast);
static const char * UriParseDecOctetFour(UriIp4Parser*parser, const char * first, const char * afterLast);
/*
 * [ipFourAddress]->[decOctet]<.>[decOctet]<.>[decOctet]<.>[decOctet]
 */
int UriParseIpFourAddress(uchar * octetOutput, const char * first, const char * afterLast)
{
	const char * after;
	UriIp4Parser parser;
	/* Essential checks */
	if((octetOutput == NULL) ||(first == NULL) ||(afterLast <= first)) {
		return URI_ERROR_SYNTAX;
	}
	/* Reset parser */
	parser.stackCount = 0;
	/* Octet #1 */
	after = UriParseDecOctet(&parser, first, afterLast);
	if((after == NULL) ||(after >= afterLast) ||(*after != _UT('.'))) {
		return URI_ERROR_SYNTAX;
	}
	uriStackToOctet(&parser, octetOutput);
	/* Octet #2 */
	after = UriParseDecOctet(&parser, after+1, afterLast);
	if((after == NULL) ||(after >= afterLast) ||(*after != _UT('.'))) {
		return URI_ERROR_SYNTAX;
	}
	uriStackToOctet(&parser, octetOutput+1);
	/* Octet #3 */
	after = UriParseDecOctet(&parser, after+1, afterLast);
	if((after == NULL) ||(after >= afterLast) ||(*after != _UT('.'))) {
		return URI_ERROR_SYNTAX;
	}
	uriStackToOctet(&parser, octetOutput+2);
	/* Octet #4 */
	after = UriParseDecOctet(&parser, after+1, afterLast);
	if(after != afterLast) {
		return URI_ERROR_SYNTAX;
	}
	uriStackToOctet(&parser, octetOutput+3);
	return URI_SUCCESS;
}
/*
 * [decOctet]-><0>
 * [decOctet]-><1>[decOctetOne]
 * [decOctet]-><2>[decOctetTwo]
 * [decOctet]-><3>[decOctetThree]
 * [decOctet]-><4>[decOctetThree]
 * [decOctet]-><5>[decOctetThree]
 * [decOctet]-><6>[decOctetThree]
 * [decOctet]-><7>[decOctetThree]
 * [decOctet]-><8>[decOctetThree]
 * [decOctet]-><9>[decOctetThree]
 */
static const char * UriParseDecOctet(UriIp4Parser*parser, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return NULL;
	else {
		switch(*first) {
			case _UT('0'):
				uriPushToStack(parser, 0);
				return first+1;
			case _UT('1'):
				uriPushToStack(parser, 1);
				return (const char *)UriParseDecOctetOne(parser, first+1, afterLast);
			case _UT('2'):
				uriPushToStack(parser, 2);
				return (const char *)UriParseDecOctetTwo(parser, first+1, afterLast);
			case _UT('3'):
			case _UT('4'):
			case _UT('5'):
			case _UT('6'):
			case _UT('7'):
			case _UT('8'):
			case _UT('9'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return (const char *)UriParseDecOctetThree(parser, first+1, afterLast);
			default:
				return NULL;
		}
	}
}
/*
 * [decOctetOne]-><NULL>
 * [decOctetOne]->[DIGIT][decOctetThree]
 */
static const char * UriParseDecOctetOne(UriIp4Parser*parser, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else {
		switch(*first) {
			case _UT('0'):
			case _UT('1'):
			case _UT('2'):
			case _UT('3'):
			case _UT('4'):
			case _UT('5'):
			case _UT('6'):
			case _UT('7'):
			case _UT('8'):
			case _UT('9'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return(const char *)UriParseDecOctetThree(parser, first+1, afterLast);
			default:
				return first;
		}
	}
}
/*
 * [decOctetTwo]-><NULL>
 * [decOctetTwo]-><0>[decOctetThree]
 * [decOctetTwo]-><1>[decOctetThree]
 * [decOctetTwo]-><2>[decOctetThree]
 * [decOctetTwo]-><3>[decOctetThree]
 * [decOctetTwo]-><4>[decOctetThree]
 * [decOctetTwo]-><5>[decOctetFour]
 * [decOctetTwo]-><6>
 * [decOctetTwo]-><7>
 * [decOctetTwo]-><8>
 * [decOctetTwo]-><9>
 */
static const char * UriParseDecOctetTwo(UriIp4Parser*parser, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else {
		switch(*first) {
			case _UT('0'):
			case _UT('1'):
			case _UT('2'):
			case _UT('3'):
			case _UT('4'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return(const char *)UriParseDecOctetThree(parser, first+1, afterLast);
			case _UT('5'):
				uriPushToStack(parser, 5);
				return(const char *)UriParseDecOctetFour(parser, first+1, afterLast);
			case _UT('6'):
			case _UT('7'):
			case _UT('8'):
			case _UT('9'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return first+1;
			default:
				return first;
		}
	}
}
/*
 * [decOctetThree]-><NULL>
 * [decOctetThree]->[DIGIT]
 */
static const char * UriParseDecOctetThree(UriIp4Parser * parser, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		return afterLast;
	}
	else {
		switch(*first) {
			case _UT('0'):
			case _UT('1'):
			case _UT('2'):
			case _UT('3'):
			case _UT('4'):
			case _UT('5'):
			case _UT('6'):
			case _UT('7'):
			case _UT('8'):
			case _UT('9'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return first+1;
			default:
				return first;
		}
	}
}
/*
 * [decOctetFour]-><NULL>
 * [decOctetFour]-><0>
 * [decOctetFour]-><1>
 * [decOctetFour]-><2>
 * [decOctetFour]-><3>
 * [decOctetFour]-><4>
 * [decOctetFour]-><5>
 */
static const char * UriParseDecOctetFour(UriIp4Parser*parser, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else {
		switch(*first) {
			case _UT('0'):
			case _UT('1'):
			case _UT('2'):
			case _UT('3'):
			case _UT('4'):
			case _UT('5'):
				uriPushToStack(parser,(uchar)(9+*first-_UT('9')));
				return first+1;
			default:
				return first;
		}
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
    case _UT('g'): \
    case _UT('G'): \
    case _UT('h'): \
    case _UT('H'): \
    case _UT('i'): \
    case _UT('I'): \
    case _UT('j'): \
    case _UT('J'): \
    case _UT('k'): \
    case _UT('K'): \
    case _UT('l'): \
    case _UT('L'): \
    case _UT('m'): \
    case _UT('M'): \
    case _UT('n'): \
    case _UT('N'): \
    case _UT('o'): \
    case _UT('O'): \
    case _UT('p'): \
    case _UT('P'): \
    case _UT('q'): \
    case _UT('Q'): \
    case _UT('r'): \
    case _UT('R'): \
    case _UT('s'): \
    case _UT('S'): \
    case _UT('t'): \
    case _UT('T'): \
    case _UT('u'): \
    case _UT('U'): \
    case _UT('v'): \
    case _UT('V'): \
    case _UT('w'): \
    case _UT('W'): \
    case _UT('x'): \
    case _UT('X'): \
    case _UT('y'): \
    case _UT('Y'): \
    case _UT('z'): \
    case _UT('Z')

static const char * UriParseAuthority(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseAuthorityTwo(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseHexZero(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseHierPart(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseIpFutLoop(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseIpFutStopGo(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseIpLit2(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseIPv6address2(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseMustBeSegmentNzNc(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseOwnHost(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseOwnHost2(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseOwnHostUserInfo(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseOwnHostUserInfoNz(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseOwnPortUserInfo(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseOwnUserInfo(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePartHelperTwo(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParsePathAbsEmpty(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePathAbsNoLeadSlash(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParsePathRootless(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePchar(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePctEncoded(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePctSubUnres(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParsePort(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseQueryFrag(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseSegment(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseSegmentNz(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseSegmentNzNcOrScheme2(UriParserState * state, const char * first, const char * afterLast);
static const char * UriParseUriReference(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseUriTail(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseUriTailTwo(UriParserState * state, const char * first, const char *  afterLast);
static const char * UriParseZeroMoreSlashSegs(UriParserState * state, const char * first, const char * afterLast);

static int FASTCALL UriOnExitOwnHost2(UriParserState * state, const char * first);
static int FASTCALL UriOnExitOwnHostUserInfo(UriParserState * state, const char * first);
static int FASTCALL UriOnExitOwnPortUserInfo(UriParserState * state, const char * first);
static int FASTCALL UriOnExitSegmentNzNcOrScheme2(UriParserState * state, const char * first);
static void FASTCALL UriOnExitPartHelperTwo(UriParserState * state);
static void FASTCALL UriResetParserState(UriParserState * state);
static int UriPushPathSegment(UriParserState * state, const char * first, const char * afterLast);
static void FASTCALL UriStopSyntax(UriParserState * state, const char * errorPos);
static void FASTCALL UriStopMalloc(UriParserState * state);

static void FASTCALL UriStopSyntax(UriParserState * state, const char * errorPos)
{
	UriFreeUriMembers(state->uri);
	state->errorPos = errorPos;
	state->errorCode = URI_ERROR_SYNTAX;
}

static void FASTCALL UriStopMalloc(UriParserState * state)
{
	UriFreeUriMembers(state->uri);
	state->errorPos = NULL;
	state->errorCode = URI_ERROR_MALLOC;
}
/*
 * [authority]-><[>[ipLit2][authorityTwo]
 * [authority]->[ownHostUserInfoNz]
 * [authority]-><NULL>
 */
static const char * UriParseAuthority(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		/* "" regname host */
		state->uri->hostText.first = UriSafeToPointTo;
		state->uri->hostText.afterLast = UriSafeToPointTo;
		return afterLast;
	}
	switch(*first) {
	    case _UT('['):
	    {
		    const char * const afterIpLit2 = UriParseIpLit2(state, first+1, afterLast);
		    if(afterIpLit2 == NULL)
			    return NULL;
			else {
				state->uri->hostText.first = first+1;       /* HOST BEGIN */
				return UriParseAuthorityTwo(state, afterIpLit2, afterLast);
			}
	    }
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('%'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('-'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT('.'):
	    case _UT(':'):
	    case _UT(';'):
	    case _UT('@'):
	    case _UT('\''):
	    case _UT('_'):
	    case _UT('~'):
	    case _UT('+'):
	    case _UT('='):
	    case URI_SET_DIGIT:
	    case URI_SET_ALPHA:
			state->uri->userInfo.first = first; /* USERINFO BEGIN */
			return UriParseOwnHostUserInfoNz(state, first, afterLast);
	    default:
			/* "" regname host */
			state->uri->hostText.first = UriSafeToPointTo;
			state->uri->hostText.afterLast = UriSafeToPointTo;
			return first;
	}
}
/*
 * [authorityTwo]-><:>[port]
 * [authorityTwo]-><NULL>
 */
static const char * UriParseAuthorityTwo(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_result = 0;
	if(first >= afterLast) {
		p_result = afterLast;
	}
	else if(*first == _UT(':')) {
		const char * const afterPort = UriParsePort(state, first+1, afterLast);
		if(afterPort) {
			state->uri->portText.first = first+1;       /* PORT BEGIN */
			state->uri->portText.afterLast = afterPort;     /* PORT END */
			p_result = afterPort;
		}
	}
	else
		p_result = first;
	return p_result;
}
/*
 * [hexZero]->[HEXDIG][hexZero]
 * [hexZero]-><NULL>
 */
static const char * UriParseHexZero(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(ishex(*first))
		return UriParseHexZero(state, first+1, afterLast);
	else
		return first;
}
/*
 * [hierPart]->[pathRootless]
 * [hierPart]-></>[partHelperTwo]
 * [hierPart]-><NULL>
 */
static const char * UriParseHierPart(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		return afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(':'):
			case _UT(';'):
			case _UT('@'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				return UriParsePathRootless(state, first, afterLast);
			case _UT('/'):
				return UriParsePartHelperTwo(state, first+1, afterLast);
			default:
				return first;
		}
	}
}
/*
 * [ipFutLoop]->[subDelims][ipFutStopGo]
 * [ipFutLoop]->[unreserved][ipFutStopGo]
 * [ipFutLoop]-><:>[ipFutStopGo]
 */
static const char * UriParseIpFutLoop(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		UriStopSyntax(state, first);
		return NULL;
	}
	switch(*first) {
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('-'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT('.'):
	    case _UT(':'):
	    case _UT(';'):
	    case _UT('\''):
	    case _UT('_'):
	    case _UT('~'):
	    case _UT('+'):
	    case _UT('='):
	    case URI_SET_DIGIT:
	    case URI_SET_ALPHA:
			return UriParseIpFutStopGo(state, first+1, afterLast);
	    default:
			UriStopSyntax(state, first);
			return NULL;
	}
}

/*
 * [ipFutStopGo]->[ipFutLoop]
 * [ipFutStopGo]-><NULL>
 */
static const char * UriParseIpFutStopGo(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		return afterLast;
	}
	switch(*first) {
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('-'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT('.'):
	    case _UT(':'):
	    case _UT(';'):
	    case _UT('\''):
	    case _UT('_'):
	    case _UT('~'):
	    case _UT('+'):
	    case _UT('='):
	    case URI_SET_DIGIT:
	    case URI_SET_ALPHA:
		return UriParseIpFutLoop(state, first, afterLast);

	    default:
		return first;
	}
}

/*
 * [ipFuture]-><v>[HEXDIG][hexZero]<.>[ipFutLoop]
 */
static const char * UriParseIpFuture(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		UriStopSyntax(state, first);
		return NULL;
	}
	/*
	   First character has already been
	   checked before entering this rule.

	   switch(*first) {
	   case _UT('v'):
	 */
	if(first+1 >= afterLast) {
		UriStopSyntax(state, first+1);
		return NULL;
	}
	switch(first[1]) {
	    case URI_SET_HEXDIG:
	    {
		    const char * afterIpFutLoop;
		    const char * const afterHexZero = UriParseHexZero(state, first+2, afterLast);
		    if(afterHexZero == NULL)
			    return NULL;
		    else if((afterHexZero >= afterLast) ||(*afterHexZero != _UT('.'))) {
			    UriStopSyntax(state, afterHexZero);
			    return NULL;
		    }
			else {
				state->uri->hostText.first = first;             /* HOST BEGIN */
				state->uri->hostData.ipFuture.first = first;             /* IPFUTURE BEGIN */
				afterIpFutLoop = UriParseIpFutLoop(state, afterHexZero+1, afterLast);
				if(afterIpFutLoop == NULL)
					return NULL;
				else {
					state->uri->hostText.afterLast = afterIpFutLoop;             /* HOST END */
					state->uri->hostData.ipFuture.afterLast = afterIpFutLoop;             /* IPFUTURE END */
					return afterIpFutLoop;
				}
			}
	    }

	    default:
		UriStopSyntax(state, first+1);
		return NULL;
	}
	/*
	   default:
	        UriStopSyntax(state, first);
	        return NULL;
	   }
	 */
}
/*
 * [ipLit2]->[ipFuture]<]>
 * [ipLit2]->[IPv6address2]
 */
static const char * UriParseIpLit2(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		UriStopSyntax(state, first);
		return NULL;
	}
	switch(*first) {
	    case _UT('v'):
	    {
		    const char * const afterIpFuture = UriParseIpFuture(state, first, afterLast);
		    if(afterIpFuture == NULL) {
			    return NULL;
		    }
		    else if((afterIpFuture >= afterLast) ||(*afterIpFuture != _UT(']'))) {
			    UriStopSyntax(state, first);
			    return NULL;
		    }
			else
				return afterIpFuture+1;
	    }
	    case _UT(':'):
	    case _UT(']'):
	    case URI_SET_HEXDIG:
			state->uri->hostData.ip6 =(UriIp6 *)SAlloc::M(1*sizeof(UriIp6));   /* Freed when stopping on parse error */
			if(state->uri->hostData.ip6 == NULL) {
				UriStopMalloc(state);
				return NULL;
			}
			else
				return UriParseIPv6address2(state, first, afterLast);
	    default:
			UriStopSyntax(state, first);
			return NULL;
	}
}
/*
 * [IPv6address2]->..<]>
 */
static const char * UriParseIPv6address2(UriParserState * state, const char * first, const char * afterLast)
{
	int zipperEver = 0;
	int quadsDone = 0;
	int digitCount = 0;
	uchar digitHistory[4];
	int ip4OctetsDone = 0;

	uchar quadsAfterZipper[14];
	int quadsAfterZipperCount = 0;

	for(;; ) {
		if(first >= afterLast) {
			UriStopSyntax(state, first);
			return NULL;
		}
		/* Inside IPv4 part? */
		if(ip4OctetsDone > 0) {
			/* Eat rest of IPv4 address */
			for(;; ) {
				switch(*first) {
				    case URI_SET_DIGIT:
					if(digitCount == 4) {
						UriStopSyntax(state, first);
						return NULL;
					}
					digitHistory[digitCount++] =(uchar)(9+*first-_UT('9'));
					break;

				    case _UT('.'):
					if((ip4OctetsDone == 4) || /* NOTE! */ (digitCount == 0) || (digitCount == 4)) {
						/* Invalid digit or octet count */
						UriStopSyntax(state, first);
						return NULL;
					}
					else if((digitCount > 1) && (digitHistory[0] == 0)) {
						/* Leading zero */
						UriStopSyntax(state, first-digitCount);
						return NULL;
					}
					else if((digitCount > 2) && (digitHistory[1] == 0)) {
						/* Leading zero */
						UriStopSyntax(state, first-digitCount+1);
						return NULL;
					}
					else if((digitCount == 3) && (100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
						/* Octet value too large */
						if(digitHistory[0] > 2) {
							UriStopSyntax(state, first-3);
						}
						else if(digitHistory[1] > 5) {
							UriStopSyntax(state, first-2);
						}
						else {
							UriStopSyntax(state, first-1);
						}
						return NULL;
					}
					/* Copy IPv4 octet */
					state->uri->hostData.ip6->data[16-4+ip4OctetsDone] = uriGetOctetValue(digitHistory, digitCount);
					digitCount = 0;
					ip4OctetsDone++;
					break;

				    case _UT(']'):
						if((ip4OctetsDone != 3) || /* NOTE! */ (digitCount == 0) || (digitCount == 4)) {
							/* Invalid digit or octet count */
							UriStopSyntax(state, first);
							return NULL;
						}
						else if((digitCount > 1) && (digitHistory[0] == 0)) {
							/* Leading zero */
							UriStopSyntax(state, first-digitCount);
							return NULL;
						}
						else if((digitCount > 2) && (digitHistory[1] == 0)) {
							// Leading zero
							UriStopSyntax(state, first-digitCount+1);
							return NULL;
						}
						else if((digitCount == 3) && (100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
							// Octet value too large
							if(digitHistory[0] > 2) {
								UriStopSyntax(state, first-3);
							}
							else if(digitHistory[1] > 5) {
								UriStopSyntax(state, first-2);
							}
							else {
								UriStopSyntax(state, first-1);
							}
							return NULL;
						}
						state->uri->hostText.afterLast = first; /* HOST END */

						/* Copy missing quads right before IPv4 */
						memcpy(state->uri->hostData.ip6->data+16-4-2*quadsAfterZipperCount, quadsAfterZipper, 2*quadsAfterZipperCount);
						/* Copy last IPv4 octet */
						state->uri->hostData.ip6->data[16-4+3] = uriGetOctetValue(digitHistory, digitCount);
						return first+1;
				    default:
						UriStopSyntax(state, first);
						return NULL;
				}
				first++;
			}
		}
		else {
			/* Eat while no dot in sight */
			int letterAmong = 0;
			int walking = 1;
			do {
				switch(*first) {
				    case URI_SET_HEX_LETTER_LOWER:
					letterAmong = 1;
					if(digitCount == 4) {
						UriStopSyntax(state, first);
						return NULL;
					}
					digitHistory[digitCount] =(uchar)(15+*first-_UT('f'));
					digitCount++;
					break;

				    case URI_SET_HEX_LETTER_UPPER:
					letterAmong = 1;
					if(digitCount == 4) {
						UriStopSyntax(state, first);
						return NULL;
					}
					digitHistory[digitCount] =(uchar)(15+*first-_UT('F'));
					digitCount++;
					break;

				    case URI_SET_DIGIT:
					if(digitCount == 4) {
						UriStopSyntax(state, first);
						return NULL;
					}
					digitHistory[digitCount] =(uchar)(9+*first-_UT('9'));
					digitCount++;
					break;

				    case _UT(':'):
				    {
					    int setZipper = 0;
					    if(quadsDone > 8-zipperEver) { /* Too many quads? */
						    UriStopSyntax(state, first);
						    return NULL;
					    }
					    else if(first+1 >= afterLast) { /* "::"? */
						    UriStopSyntax(state, first+1);
						    return NULL;
					    }
						else {
							if(first[1] == _UT(':')) {
								const int resetOffset = 2*(quadsDone+(digitCount > 0));
								first++;
								if(zipperEver) {
									UriStopSyntax(state, first);
									return NULL;     /* "::.+::" */
								}
								else {
									/* Zero everything after zipper */
									memzero(state->uri->hostData.ip6->data+resetOffset, 16-resetOffset);
									setZipper = 1;
									/* ":::+"? */
									if(first+1 >= afterLast) {
										UriStopSyntax(state, first+1);
										return NULL;     /* No ']' yet */
									}
									else if(first[1] == _UT(':')) {
										UriStopSyntax(state, first+1);
										return NULL;     /* ":::+ "*/
									}
								}
							}
							if(digitCount > 0) {
								if(zipperEver) {
									uriWriteQuadToDoubleByte(digitHistory, digitCount, quadsAfterZipper+2*quadsAfterZipperCount);
									quadsAfterZipperCount++;
								}
								else {
									uriWriteQuadToDoubleByte(digitHistory, digitCount, state->uri->hostData.ip6->data+2*quadsDone);
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
					if((quadsDone > 6) || /* NOTE */(!zipperEver &&(quadsDone < 6)) || letterAmong ||(digitCount == 0) ||(digitCount == 4)) {
						/* Invalid octet before */
						UriStopSyntax(state, first);
						return NULL;
					}
					else if((digitCount > 1) &&(digitHistory[0] == 0)) {
						/* Leading zero */
						UriStopSyntax(state, first-digitCount);
						return NULL;
					}
					else if((digitCount > 2) &&(digitHistory[1] == 0)) {
						/* Leading zero */
						UriStopSyntax(state, first-digitCount+1);
						return NULL;
					}
					else if((digitCount == 3) &&(100*digitHistory[0]+10*digitHistory[1]+digitHistory[2] > 255)) {
						/* Octet value too large */
						if(digitHistory[0] > 2) {
							UriStopSyntax(state, first-3);
						}
						else if(digitHistory[1] > 5) {
							UriStopSyntax(state, first-2);
						}
						else {
							UriStopSyntax(state, first-1);
						}
						return NULL;
					}
					/* Copy first IPv4 octet */
					state->uri->hostData.ip6->data[16-4] = uriGetOctetValue(digitHistory, digitCount);
					digitCount = 0;

					/* Switch over to IPv4 loop */
					ip4OctetsDone = 1;
					walking = 0;
					break;

				    case _UT(']'):
					/* Too little quads? */
					if(!zipperEver && !((quadsDone == 7) &&(digitCount > 0))) {
						UriStopSyntax(state, first);
						return NULL;
					}
					if(digitCount > 0) {
						if(zipperEver) {
							uriWriteQuadToDoubleByte(digitHistory, digitCount, quadsAfterZipper+2*quadsAfterZipperCount);
							quadsAfterZipperCount++;
						}
						else {
							uriWriteQuadToDoubleByte(digitHistory, digitCount, state->uri->hostData.ip6->data+2*quadsDone);
						}
						/*
						   quadsDone++;
						   digitCount = 0;
						 */
					}
					/* Copy missing quads to the end */
					memcpy(state->uri->hostData.ip6->data+16-2*quadsAfterZipperCount, quadsAfterZipper, 2*quadsAfterZipperCount);
					state->uri->hostText.afterLast = first; /* HOST END */
					return first+1;   /* Fine */

				    default:
					UriStopSyntax(state, first);
					return NULL;
				}
				first++;
				if(first >= afterLast) {
					UriStopSyntax(state, first);
					return NULL; /* No ']' yet */
				}
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
static const char * UriParseMustBeSegmentNzNc(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		if(!UriPushPathSegment(state, state->uri->scheme.first, first)) { /* SEGMENT BOTH */
			UriStopMalloc(state);
			return NULL;
		}
		else {
			state->uri->scheme.first = NULL; /* Not a scheme, reset */
			return afterLast;
		}
	}
	switch(*first) {
	    case _UT('%'):
			{
				const char * const afterPctEncoded = UriParsePctEncoded(state, first, afterLast);
				return afterPctEncoded ? UriParseMustBeSegmentNzNc(state, afterPctEncoded, afterLast) : 0;
			}
	    case _UT('@'):
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT(';'):
	    case _UT('\''):
	    case _UT('+'):
	    case _UT('='):
	    case _UT('-'):
	    case _UT('.'):
	    case _UT('_'):
	    case _UT('~'):
	    case URI_SET_DIGIT:
	    case URI_SET_ALPHA:
			return UriParseMustBeSegmentNzNc(state, first+1, afterLast);
	    case _UT('/'):
			{
				const char * afterZeroMoreSlashSegs;
				const char * afterSegment;
				if(!UriPushPathSegment(state, state->uri->scheme.first, first)) {     /* SEGMENT BOTH */
					UriStopMalloc(state);
					return NULL;
				}
				else {
					state->uri->scheme.first = NULL;     /* Not a scheme, reset */
					afterSegment = UriParseSegment(state, first+1, afterLast);
					if(afterSegment == NULL) {
						return NULL;
					}
					else if(!UriPushPathSegment(state, first+1, afterSegment)) {       /* SEGMENT BOTH */
						UriStopMalloc(state);
						return NULL;
					}
					else {
						afterZeroMoreSlashSegs = UriParseZeroMoreSlashSegs(state, afterSegment, afterLast);
						if(afterZeroMoreSlashSegs == NULL)
							return NULL;
						else
							return UriParseUriTail(state, afterZeroMoreSlashSegs, afterLast);
					}
				}
			}
	    default:
			if(!UriPushPathSegment(state, state->uri->scheme.first, first)) { /* SEGMENT BOTH */
				UriStopMalloc(state);
				return NULL;
			}
			else {
				state->uri->scheme.first = NULL; /* Not a scheme, reset */
				return UriParseUriTail(state, first, afterLast);
			}
	}
}
/*
 * [ownHost]-><[>[ipLit2][authorityTwo]
 * [ownHost]->[ownHost2] // can take <NULL>
 */
static const char * UriParseOwnHost(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		return afterLast;
	}
	else if(*first == _UT('[')) {
		const char * const afterIpLit2 = UriParseIpLit2(state, first+1, afterLast);
		if(afterIpLit2 == NULL) {
			return NULL;
		}
		else {
			state->uri->hostText.first = first+1;       /* HOST BEGIN */
			return UriParseAuthorityTwo(state, afterIpLit2, afterLast);
		}
	}
	else
		return UriParseOwnHost2(state, first, afterLast);
}

static int FASTCALL UriOnExitOwnHost2(UriParserState * state, const char * first)
{
	state->uri->hostText.afterLast = first; /* HOST END */
	/* Valid IPv4 or just a regname? */
	state->uri->hostData.ip4 =(UriIp4 *)SAlloc::M(1*sizeof(UriIp4));   /* Freed when stopping on parse error */
	if(state->uri->hostData.ip4 == NULL) {
		return FALSE; /* Raises SAlloc::M error */
	}
	else {
		if(UriParseIpFourAddress(state->uri->hostData.ip4->data, state->uri->hostText.first, state->uri->hostText.afterLast)) {
			/* Not IPv4 */
			SAlloc::F(state->uri->hostData.ip4);
			state->uri->hostData.ip4 = NULL;
		}
		return TRUE; /* Success */
	}
}
/*
 * [ownHost2]->[authorityTwo] // can take <NULL>
 * [ownHost2]->[pctSubUnres][ownHost2]
 */
static const char * UriParseOwnHost2(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		if(!UriOnExitOwnHost2(state, first))
			UriStopMalloc(state);
		else
			p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(';'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPctSubUnres = UriParsePctSubUnres(state, first, afterLast);
					p_ret = afterPctSubUnres ? UriParseOwnHost2(state, afterPctSubUnres, afterLast) : 0;
				}
				break;
			default:
				if(!UriOnExitOwnHost2(state, first))
					UriStopMalloc(state);
				else
					p_ret = UriParseAuthorityTwo(state, first, afterLast);
				break;
		}
	}
	return p_ret;
}

static int FASTCALL UriOnExitOwnHostUserInfo(UriParserState * state, const char * first)
{
	int    ok = 1;
	state->uri->hostText.first = state->uri->userInfo.first; /* Host instead of userInfo, update */
	state->uri->userInfo.first = NULL; /* Not a userInfo, reset */
	state->uri->hostText.afterLast = first; /* HOST END */
	/* Valid IPv4 or just a regname? */
	state->uri->hostData.ip4 =(UriIp4 *)SAlloc::M(1*sizeof(UriIp4));   /* Freed when stopping on parse error */
	if(state->uri->hostData.ip4 == NULL) {
		ok = 0; /* Raises SAlloc::M error */
	}
	else if(UriParseIpFourAddress(state->uri->hostData.ip4->data, state->uri->hostText.first, state->uri->hostText.afterLast)) {
		/* Not IPv4 */
		SAlloc::F(state->uri->hostData.ip4);
		state->uri->hostData.ip4 = NULL;
	}
	return ok; /* Success */
}
/*
 * [ownHostUserInfo]->[ownHostUserInfoNz]
 * [ownHostUserInfo]-><NULL>
 */
static const char * UriParseOwnHostUserInfo(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		if(!UriOnExitOwnHostUserInfo(state, first))
			UriStopMalloc(state);
		else
			p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(':'):
			case _UT(';'):
			case _UT('@'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				p_ret = UriParseOwnHostUserInfoNz(state, first, afterLast);
				break;
			default:
				if(!UriOnExitOwnHostUserInfo(state, first))
					UriStopMalloc(state);
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
static const char * UriParseOwnHostUserInfoNz(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		UriStopSyntax(state, first);
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(';'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPctSubUnres = UriParsePctSubUnres(state, first, afterLast);
					p_ret = afterPctSubUnres ? UriParseOwnHostUserInfo(state, afterPctSubUnres, afterLast) : 0;
				}
				break;
			case _UT(':'):
				state->uri->hostText.afterLast = first; /* HOST END */
				state->uri->portText.first = first+1;   /* PORT BEGIN */
				p_ret = UriParseOwnPortUserInfo(state, first+1, afterLast);
				break;
			case _UT('@'):
				state->uri->userInfo.afterLast = first; /* USERINFO END */
				state->uri->hostText.first = first+1;   /* HOST BEGIN */
				p_ret = UriParseOwnHost(state, first+1, afterLast);
				break;
			default:
				UriStopSyntax(state, first);
				break;
		}
	}
	return p_ret;
}

static int FASTCALL UriOnExitOwnPortUserInfo(UriParserState * state, const char * first)
{
	int    ok = 1;
	state->uri->hostText.first = state->uri->userInfo.first; /* Host instead of userInfo, update */
	state->uri->userInfo.first = NULL; /* Not a userInfo, reset */
	state->uri->portText.afterLast = first; /* PORT END */
	/* Valid IPv4 or just a regname? */
	state->uri->hostData.ip4 =(UriIp4 *)SAlloc::M(1*sizeof(UriIp4));   /* Freed when stopping on parse error */
	if(state->uri->hostData.ip4 == NULL) {
		ok = 0; /* Raises SAlloc::M error */
	}
	else if(UriParseIpFourAddress(state->uri->hostData.ip4->data, state->uri->hostText.first, state->uri->hostText.afterLast)) {
		/* Not IPv4 */
		SAlloc::F(state->uri->hostData.ip4);
		state->uri->hostData.ip4 = NULL;
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
static const char * UriParseOwnPortUserInfo(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		if(!UriOnExitOwnPortUserInfo(state, first))
			UriStopMalloc(state);
		else
			p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('.'):
			case _UT('_'):
			case _UT('~'):
			case _UT('-'):
			case URI_SET_ALPHA:
				state->uri->hostText.afterLast = NULL; /* Not a host, reset */
				state->uri->portText.first = NULL; /* Not a port, reset */
				p_ret = UriParseOwnUserInfo(state, first+1, afterLast);
				break;
			case URI_SET_DIGIT:
				p_ret = UriParseOwnPortUserInfo(state, first+1, afterLast);
				break;
			case _UT('@'):
				state->uri->hostText.afterLast = NULL; /* Not a host, reset */
				state->uri->portText.first = NULL; /* Not a port, reset */
				state->uri->userInfo.afterLast = first; /* USERINFO END */
				state->uri->hostText.first = first+1;   /* HOST BEGIN */
				p_ret = UriParseOwnHost(state, first+1, afterLast);
				break;
			default:
				if(!UriOnExitOwnPortUserInfo(state, first))
					UriStopMalloc(state);
				else
					p_ret = first;
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
static const char * UriParseOwnUserInfo(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		UriStopSyntax(state, first);
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(';'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const after_pct_sub_unres = UriParsePctSubUnres(state, first, afterLast);
					p_ret = after_pct_sub_unres ? UriParseOwnUserInfo(state, after_pct_sub_unres, afterLast) : 0;
				}
				break;
			case _UT(':'):
				p_ret = UriParseOwnUserInfo(state, first+1, afterLast);
				break;
			case _UT('@'):
				/* SURE */
				state->uri->userInfo.afterLast = first; /* USERINFO END */
				state->uri->hostText.first = first+1;   /* HOST BEGIN */
				p_ret = UriParseOwnHost(state, first+1, afterLast);
				break;
			default:
				UriStopSyntax(state, first);
				break;
		}
	}
	return p_ret;
}

static void FASTCALL UriOnExitPartHelperTwo(UriParserState * state)
{
	state->uri->absolutePath = TRUE;
}

/*
 * [partHelperTwo]->[pathAbsNoLeadSlash] // can take <NULL>
 * [partHelperTwo]-></>[authority][pathAbsEmpty]
 */
static const char * UriParsePartHelperTwo(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		UriOnExitPartHelperTwo(state);
		p_ret = afterLast;
	}
	else if(*first == _UT('/')) {
		const char * const afterAuthority = UriParseAuthority(state, first+1, afterLast);
		if(afterAuthority) {
			p_ret = UriParsePathAbsEmpty(state, afterAuthority, afterLast);
			UriFixEmptyTrailSegment(state->uri);
		}
	}
	else {
		UriOnExitPartHelperTwo(state);
		p_ret = UriParsePathAbsNoLeadSlash(state, first, afterLast);
	}
	return p_ret;
}
/*
 * [pathAbsEmpty]-></>[segment][pathAbsEmpty]
 * [pathAbsEmpty]-><NULL>
 */
static const char * UriParsePathAbsEmpty(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else if(*first == _UT('/')) {
		const char * const afterSegment = UriParseSegment(state, first+1, afterLast);
		if(afterSegment) {
			if(!UriPushPathSegment(state, first+1, afterSegment)) // SEGMENT BOTH
				UriStopMalloc(state);
			else
				p_ret = UriParsePathAbsEmpty(state, afterSegment, afterLast);
		}
	}
	else
		p_ret = first;
	return p_ret;
}
/*
 * [pathAbsNoLeadSlash]->[segmentNz][zeroMoreSlashSegs]
 * [pathAbsNoLeadSlash]-><NULL>
 */
static const char * UriParsePathAbsNoLeadSlash(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(':'):
			case _UT(';'):
			case _UT('@'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterSegmentNz = UriParseSegmentNz(state, first, afterLast);
					if(afterSegmentNz) {
						if(!UriPushPathSegment(state, first, afterSegmentNz)) // SEGMENT BOTH
							UriStopMalloc(state);
						else
							p_ret = UriParseZeroMoreSlashSegs(state, afterSegmentNz, afterLast);
					}
				}
				break;
			default:
				p_ret = first;
				break;
		}
	}
	return p_ret;
}

/*
 * [pathRootless]->[segmentNz][zeroMoreSlashSegs]
 */
static const char * UriParsePathRootless(UriParserState * state, const char * first, const char * afterLast)
{
	const char * const afterSegmentNz = UriParseSegmentNz(state, first, afterLast);
	if(afterSegmentNz == NULL) {
		return NULL;
	}
	else if(!UriPushPathSegment(state, first, afterSegmentNz)) { /* SEGMENT BOTH */
		UriStopMalloc(state);
		return NULL;
	}
	else
		return UriParseZeroMoreSlashSegs(state, afterSegmentNz, afterLast);
}
/*
 * [pchar]->[pctEncoded]
 * [pchar]->[subDelims]
 * [pchar]->[unreserved]
 * [pchar]-><:>
 * [pchar]-><@>
 */
static const char * UriParsePchar(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		UriStopSyntax(state, first);
	}
	else {
		switch(*first) {
			case _UT('%'):
				p_ret = UriParsePctEncoded(state, first, afterLast);
				break;
			case _UT(':'):
			case _UT('@'):
			case _UT('!'):
			case _UT('$'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('*'):
			case _UT(','):
			case _UT(';'):
			case _UT('\''):
			case _UT('+'):
			case _UT('='):
			case _UT('-'):
			case _UT('.'):
			case _UT('_'):
			case _UT('~'):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				p_ret = first+1;
				break;
			default:
				if(IsLetter1251(*first))
					p_ret = first+1;
				else
					UriStopSyntax(state, first);
				break;
		}
	}
	return p_ret;
}
/*
 * [pctEncoded]-><%>[HEXDIG][HEXDIG]
 */
static const char * UriParsePctEncoded(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		UriStopSyntax(state, first);
		return NULL;
	}
	/*
	   First character has already been
	   checked before entering this rule.

	   switch(*first) {
	   case _UT('%'):
	*/
	else if(first+1 >= afterLast) {
		UriStopSyntax(state, first+1);
		return NULL;
	}
	else {
		switch(first[1]) {
			case URI_SET_HEXDIG:
				if(first+2 >= afterLast) {
					UriStopSyntax(state, first+2);
					return NULL;
				}
				else {
					switch(first[2]) {
						case URI_SET_HEXDIG: return first+3;
						default: UriStopSyntax(state, first+2); return NULL;
					}
				}
			default:
				UriStopSyntax(state, first+1);
				return NULL;
		}
		/*
		   default:
				UriStopSyntax(state, first);
				return NULL;
		   }
		 */
	}
}
/*
 * [pctSubUnres]->[pctEncoded]
 * [pctSubUnres]->[subDelims]
 * [pctSubUnres]->[unreserved]
 */
static const char * UriParsePctSubUnres(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		UriStopSyntax(state, first);
		return NULL;
	}
	switch(*first) {
	    case _UT('%'):
			return UriParsePctEncoded(state, first, afterLast);
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT(';'):
	    case _UT('\''):
	    case _UT('+'):
	    case _UT('='):
	    case _UT('-'):
	    case _UT('.'):
	    case _UT('_'):
	    case _UT('~'):
	    case URI_SET_DIGIT:
	    case URI_SET_ALPHA:
			return first+1;
	    default:
			UriStopSyntax(state, first);
			return NULL;
	}
}
/*
 * [port]->[DIGIT][port]
 * [port]-><NULL>
 */
static const char * UriParsePort(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(isdec(*first))
		return UriParsePort(state, first+1, afterLast);
	else
		return first;
}
/*
 * [queryFrag]->[pchar][queryFrag]
 * [queryFrag]-></>[queryFrag]
 * [queryFrag]-><?>[queryFrag]
 * [queryFrag]-><NULL>
 */
static const char * UriParseQueryFrag(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(':'):
			case _UT(';'):
			case _UT('@'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPchar = UriParsePchar(state, first, afterLast);
					p_ret = afterPchar ? UriParseQueryFrag(state, afterPchar, afterLast) : 0;
				}
				break;
			case _UT('/'):
			case _UT('?'):
				p_ret = UriParseQueryFrag(state, first+1, afterLast);
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
static const char * UriParseSegment(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else {
		switch(*first) {
			case _UT('!'):
			case _UT('$'):
			case _UT('%'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('-'):
			case _UT('*'):
			case _UT(','):
			case _UT('.'):
			case _UT(':'):
			case _UT(';'):
			case _UT('@'):
			case _UT('\''):
			case _UT('_'):
			case _UT('~'):
			case _UT('+'):
			case _UT('='):
			case URI_SET_DIGIT:
			case URI_SET_ALPHA:
				{
					const char * const afterPchar = UriParsePchar(state, first, afterLast);
					p_ret = afterPchar ? UriParseSegment(state, afterPchar, afterLast) : 0;
				}
				break;
			default:
				if(IsLetter1251(*first)) {
					const char * const afterPchar = UriParsePchar(state, first, afterLast);
					p_ret = afterPchar ? UriParseSegment(state, afterPchar, afterLast) : 0;
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
static const char * UriParseSegmentNz(UriParserState * state, const char * first, const char * afterLast)
{
	const char * const afterPchar = UriParsePchar(state, first, afterLast);
	return afterPchar ? UriParseSegment(state, afterPchar, afterLast) : 0;
}

static int FASTCALL UriOnExitSegmentNzNcOrScheme2(UriParserState * state, const char * first)
{
	if(!UriPushPathSegment(state, state->uri->scheme.first, first)) // SEGMENT BOTH
		return FALSE; /* Raises SAlloc::M error*/
	else {
		state->uri->scheme.first = NULL; /* Not a scheme, reset */
		return TRUE; /* Success */
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
static const char * UriParseSegmentNzNcOrScheme2(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast) {
		if(!UriOnExitSegmentNzNcOrScheme2(state, first)) {
			UriStopMalloc(state);
			return NULL;
		}
		else
			return afterLast;
	}
	switch(*first) {
	    case _UT('.'):
	    case _UT('+'):
	    case _UT('-'):
	    case URI_SET_ALPHA:
	    case URI_SET_DIGIT:
			return UriParseSegmentNzNcOrScheme2(state, first+1, afterLast); // @recursion
	    case _UT('%'):
	    {
		    const char * const afterPctEncoded = UriParsePctEncoded(state, first, afterLast);
			return(afterPctEncoded == NULL) ? NULL : UriParseMustBeSegmentNzNc(state, afterPctEncoded, afterLast);
	    }
	    case _UT('!'):
	    case _UT('$'):
	    case _UT('&'):
	    case _UT('('):
	    case _UT(')'):
	    case _UT('*'):
	    case _UT(','):
	    case _UT(';'):
	    case _UT('@'):
	    case _UT('_'):
	    case _UT('~'):
	    case _UT('='):
	    case _UT('\''):
			return UriParseMustBeSegmentNzNc(state, first+1, afterLast);
	    case _UT('/'):
	    {
		    const char * afterZeroMoreSlashSegs;
		    const char * const afterSegment = UriParseSegment(state, first+1, afterLast);
		    if(afterSegment == NULL) {
			    return NULL;
		    }
		    else if(!UriPushPathSegment(state, state->uri->scheme.first, first)) {     /* SEGMENT BOTH */
			    UriStopMalloc(state);
			    return NULL;
		    }
			else {
				state->uri->scheme.first = NULL;     /* Not a scheme, reset */
				if(!UriPushPathSegment(state, first+1, afterSegment)) {       /* SEGMENT BOTH */
					UriStopMalloc(state);
					return NULL;
				}
				else {
					afterZeroMoreSlashSegs = UriParseZeroMoreSlashSegs(state, afterSegment, afterLast);
					return(afterZeroMoreSlashSegs == NULL) ? NULL : UriParseUriTail(state, afterZeroMoreSlashSegs, afterLast);
				}
			}
	    }
	    case _UT(':'):
	    {
		    const char * const afterHierPart = UriParseHierPart(state, first+1, afterLast);
		    state->uri->scheme.afterLast = first;     /* SCHEME END */
			return (afterHierPart == NULL) ? NULL : UriParseUriTail(state, afterHierPart, afterLast);
	    }
	    default:
			if(!UriOnExitSegmentNzNcOrScheme2(state, first)) {
				UriStopMalloc(state);
				return NULL;
			}
			else
				return UriParseUriTail(state, first, afterLast);
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
static const char * UriParseUriReference(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_ret = 0;
	if(first >= afterLast) {
		p_ret = afterLast;
	}
	else {
		switch(*first) {
			case URI_SET_ALPHA:
				state->uri->scheme.first = first; /* SCHEME BEGIN */
				p_ret = UriParseSegmentNzNcOrScheme2(state, first+1, afterLast);
				break;
			case URI_SET_DIGIT:
			case _UT('!'):
			case _UT('$'):
			case _UT('&'):
			case _UT('('):
			case _UT(')'):
			case _UT('*'):
			case _UT(','):
			case _UT(';'):
			case _UT('\''):
			case _UT('+'):
			case _UT('='):
			case _UT('.'):
			case _UT('_'):
			case _UT('~'):
			case _UT('-'):
			case _UT('@'):
				state->uri->scheme.first = first; /* SEGMENT BEGIN, ABUSE SCHEME POINTER */
				p_ret = UriParseMustBeSegmentNzNc(state, first+1, afterLast);
				break;
			case _UT('%'):
				{
					const char * const afterPctEncoded = UriParsePctEncoded(state, first, afterLast);
					if(afterPctEncoded) {
						state->uri->scheme.first = first;     /* SEGMENT BEGIN, ABUSE SCHEME POINTER */
						p_ret = UriParseMustBeSegmentNzNc(state, afterPctEncoded, afterLast);
					}
				}
				break;
			case _UT('/'):
				{
					const char * const afterPartHelperTwo = UriParsePartHelperTwo(state, first+1, afterLast);
					p_ret = afterPartHelperTwo ? UriParseUriTail(state, afterPartHelperTwo, afterLast) : 0;
				}
				break;
			default:
				p_ret = UriParseUriTail(state, first, afterLast);
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
static const char * UriParseUriTail(UriParserState * state, const char * first, const char * afterLast)
{
	const char * p_result = 0;
	if(first >= afterLast)
		p_result = afterLast;
	else if(*first == _UT('#')) {
		const char * const afterQueryFrag = UriParseQueryFrag(state, first+1, afterLast);
		if(afterQueryFrag) {
			state->uri->fragment.first = first+1;       /* FRAGMENT BEGIN */
			state->uri->fragment.afterLast = afterQueryFrag;     /* FRAGMENT END */
			p_result = afterQueryFrag;
		}
	}
	else if(*first == _UT('?')) {
		const char * const afterQueryFrag = UriParseQueryFrag(state, first+1, afterLast);
		if(afterQueryFrag) {
			state->uri->query.first = first+1;       /* QUERY BEGIN */
			state->uri->query.afterLast = afterQueryFrag;     /* QUERY END */
			p_result = UriParseUriTailTwo(state, afterQueryFrag, afterLast);
		}
	}
	else
		p_result = first;
	return p_result;
}
/*
 * [uriTailTwo]-><#>[queryFrag]
 * [uriTailTwo]-><NULL>
 */
static const char * UriParseUriTailTwo(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(*first == _UT('#')) {
		const char * const afterQueryFrag = UriParseQueryFrag(state, first+1, afterLast);
		if(afterQueryFrag == NULL)
			return NULL;
		else {
			state->uri->fragment.first = first+1;       /* FRAGMENT BEGIN */
			state->uri->fragment.afterLast = afterQueryFrag;     /* FRAGMENT END */
			return afterQueryFrag;
		}
	}
	else
		return first;
}
/*
 * [zeroMoreSlashSegs]-></>[segment][zeroMoreSlashSegs]
 * [zeroMoreSlashSegs]-><NULL>
 */
static const char * UriParseZeroMoreSlashSegs(UriParserState * state, const char * first, const char * afterLast)
{
	if(first >= afterLast)
		return afterLast;
	else if(*first == _UT('/')) {
		const char * const afterSegment = UriParseSegment(state, first+1, afterLast);
		if(afterSegment == NULL)
			return NULL;
		else if(!UriPushPathSegment(state, first+1, afterSegment)) { /* SEGMENT BOTH */
			UriStopMalloc(state);
			return NULL;
		}
		else
			return UriParseZeroMoreSlashSegs(state, afterSegment, afterLast);
	}
	else
		return first;
}

static void FASTCALL UriResetParserState(UriParserState * pState)
{
	UriUri * const p_uri_backup = pState->uri;
	memzero(pState, sizeof(*pState));
	pState->uri = p_uri_backup;
}

static int UriPushPathSegment(UriParserState * state, const char * first, const char *  afterLast)
{
	int    ok = TRUE;
	UriPathSegment * segment =(UriPathSegment *)SAlloc::M(1*sizeof(UriPathSegment));
	if(segment == NULL) {
		ok = FALSE; // Raises SAlloc::M error
	}
	else {
		memzero(segment, sizeof(*segment));
		if(first == afterLast) {
			segment->text.first = UriSafeToPointTo;
			segment->text.afterLast = UriSafeToPointTo;
		}
		else {
			segment->text.first = first;
			segment->text.afterLast = afterLast;
		}
		if(state->uri->pathHead == NULL) { // First segment ever?
			// First segement ever, set head and tail
			state->uri->pathHead = segment;
			state->uri->pathTail = segment;
		}
		else {
			// Append, update tail
			state->uri->pathTail->next = segment;
			state->uri->pathTail = segment;
		}
	}
	return ok;
}

int UriParseUriEx(UriParserState * state, const char * first, const char * afterLast)
{
	int    ok = 0;
	/* Check params */
	if(!state || !first || !afterLast)
		ok = URI_ERROR_NULL;
	else {
		UriUri * uri = state->uri;
		/* Init parser */
		UriResetParserState(state);
		UriResetUri(uri);
		/* Parse */
		const char * afterUriReference = UriParseUriReference(state, first, afterLast);
		if(afterUriReference == NULL)
			ok = state->errorCode;
		else if(afterUriReference != afterLast)
			ok = URI_ERROR_SYNTAX;
		else
			ok = URI_SUCCESS;
	}
	return ok;
}

int UriParseUri(UriParserState * pState, const char * pText)
{
	int    ok = 0;
	if(pState && pText) {
		const size_t len = strlen(pText);
		ok = UriParseUriEx(pState, pText, pText+len);
	}
	else
		ok = URI_ERROR_NULL;
	return ok;
}

void UriFreeUriMembers(UriUri * uri)
{
	if(uri) {
		if(uri->owner) {
			/* Scheme */
			if(uri->scheme.first) {
				if(uri->scheme.first != uri->scheme.afterLast)
					SAlloc::F((char *)uri->scheme.first);
				uri->scheme.first = NULL;
				uri->scheme.afterLast = NULL;
			}
			/* User info */
			if(uri->userInfo.first) {
				if(uri->userInfo.first != uri->userInfo.afterLast)
					SAlloc::F((char *)uri->userInfo.first);
				uri->userInfo.first = NULL;
				uri->userInfo.afterLast = NULL;
			}
			/* Host data - IPvFuture */
			if(uri->hostData.ipFuture.first) {
				if(uri->hostData.ipFuture.first != uri->hostData.ipFuture.afterLast)
					SAlloc::F((char *)uri->hostData.ipFuture.first);
				uri->hostData.ipFuture.first = NULL;
				uri->hostData.ipFuture.afterLast = NULL;
				uri->hostText.first = NULL;
				uri->hostText.afterLast = NULL;
			}
			/* Host text(if regname, after IPvFuture!) */
			if(uri->hostText.first && !uri->hostData.ip4 && !uri->hostData.ip6) {
				/* Real regname */
				if(uri->hostText.first != uri->hostText.afterLast)
					SAlloc::F((char *)uri->hostText.first);
				uri->hostText.first = NULL;
				uri->hostText.afterLast = NULL;
			}
		}
		ZFREE(uri->hostData.ip4); /* Host data - IPv4 */
		ZFREE(uri->hostData.ip6); /* Host data - IPv6 */
		/* Port text */
		if(uri->owner && uri->portText.first) {
			if(uri->portText.first != uri->portText.afterLast)
				SAlloc::F((char *)uri->portText.first);
			uri->portText.first = NULL;
			uri->portText.afterLast = NULL;
		}
		/* Path */
		if(uri->pathHead != NULL) {
			UriPathSegment*segWalk = uri->pathHead;
			while(segWalk != NULL) {
				UriPathSegment*const next = segWalk->next;
				if(uri->owner && segWalk->text.first && (segWalk->text.first < segWalk->text.afterLast))
					SAlloc::F((char *)segWalk->text.first);
				SAlloc::F(segWalk);
				segWalk = next;
			}
			uri->pathHead = NULL;
			uri->pathTail = NULL;
		}
		if(uri->owner) {
			/* Query */
			if(uri->query.first != NULL) {
				if(uri->query.first != uri->query.afterLast)
					SAlloc::F((char *)uri->query.first);
				uri->query.first = NULL;
				uri->query.afterLast = NULL;
			}
			/* Fragment */
			if(uri->fragment.first != NULL) {
				if(uri->fragment.first != uri->fragment.afterLast)
					SAlloc::F((char *)uri->fragment.first);
				uri->fragment.first = NULL;
				uri->fragment.afterLast = NULL;
			}
		}
	}
}

int Uri_TESTING_ONLY_ParseIpSix(const char * text)
{
	UriUri uri;
	UriParserState parser;
	const char * const afterIpSix = text+strlen(text);
	const char * res;
	UriResetParserState(&parser);
	UriResetUri(&uri);
	parser.uri = &uri;
	parser.uri->hostData.ip6 =(UriIp6 *)SAlloc::M(1*sizeof(UriIp6));
	res = UriParseIPv6address2(&parser, text, afterIpSix);
	UriFreeUriMembers(&uri);
	return res == afterIpSix ? TRUE : FALSE;
}

int Uri_TESTING_ONLY_ParseIpFour(const char * text)
{
	uchar octets[4];
	int res = UriParseIpFourAddress(octets, text, text+strlen(text));
	return(res == URI_SUCCESS) ? TRUE : FALSE;
}

#undef URI_SET_DIGIT
#undef URI_SET_HEX_LETTER_UPPER
#undef URI_SET_HEX_LETTER_LOWER
#undef URI_SET_HEXDIG
#undef URI_SET_ALPHA
