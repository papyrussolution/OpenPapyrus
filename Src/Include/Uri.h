// URI.H
// Copyright (C) 2007, Weijia Song <songweijia@gmail.com>
// Copyright (C) 2007, Sebastian Pipping <webmaster@hartwork.org>
// All rights reserved.
//
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <slib.h>

/* Version helper macro */
#define URI_ANSI_TO_UNICODE(x) L ## x

/* Version */
#define URI_VER_MAJOR           0
#define URI_VER_MINOR           7
#define URI_VER_RELEASE         6
#define URI_VER_SUFFIX_ANSI     ""
#define URI_VER_SUFFIX_UNICODE  URI_ANSI_TO_UNICODE(URI_VER_SUFFIX_ANSI)

/* More version helper macros */
#define URI_INT_TO_ANSI_HELPER(x) # x
#define URI_INT_TO_ANSI(x) URI_INT_TO_ANSI_HELPER(x)

#define URI_INT_TO_UNICODE_HELPER(x) URI_ANSI_TO_UNICODE(# x)
#define URI_INT_TO_UNICODE(x) URI_INT_TO_UNICODE_HELPER(x)
#define URI_VER_ANSI_HELPER(ma, mi, r, s)    URI_INT_TO_ANSI(ma) "." URI_INT_TO_ANSI(mi) "." URI_INT_TO_ANSI(r) s
#define URI_VER_UNICODE_HELPER(ma, mi, r, s) URI_INT_TO_UNICODE(ma) L"." URI_INT_TO_UNICODE(mi) L"." URI_INT_TO_UNICODE(r) s

/* Full version strings */
#define URI_VER_ANSI     URI_VER_ANSI_HELPER(URI_VER_MAJOR, URI_VER_MINOR, URI_VER_RELEASE, URI_VER_SUFFIX_ANSI)
#define URI_VER_UNICODE  URI_VER_UNICODE_HELPER(URI_VER_MAJOR, URI_VER_MINOR, URI_VER_RELEASE, URI_VER_SUFFIX_UNICODE)

/* Unused parameter macro */
#ifdef __GNUC__
	#define URI_UNUSED(x) unused_ ## x __attribute__((unused))
#else
	#define URI_UNUSED(x) x
#endif

#if 0 // {
// Shared errors 
#define URI_SUCCESS                        0
#define URI_ERROR_SYNTAX                   1 // Parsed text violates expected format
#define URI_ERROR_NULL                     2 // One of the params passed was NULL although it mustn't be
#define URI_ERROR_MALLOC                   3 // Requested memory could not be allocated
#define URI_ERROR_OUTPUT_TOO_LARGE         4 // Some output is to large for the receiving buffer
#define URI_ERROR_NOT_IMPLEMENTED          8 // The called function is not implemented yet
#define URI_ERROR_RANGE_INVALID            9 // The parameters passed contained invalid ranges
// Errors specific to ToString 
#define URI_ERROR_TOSTRING_TOO_LONG        URI_ERROR_OUTPUT_TOO_LARGE /* Deprecated, test for URI_ERROR_OUTPUT_TOO_LARGE instead */
// Errors specific to AddBaseUri 
#define URI_ERROR_ADDBASE_REL_BASE         5 // Given base is not absolute 
// Errors specific to RemoveBaseUri 
#define URI_ERROR_REMOVEBASE_REL_BASE      6 // Given base is not absolute 
#define URI_ERROR_REMOVEBASE_REL_SOURCE    7 // Given base is not absolute 
#endif // } 0
/**
 * Holds an IPv4 address.
 */
typedef struct UriIp4Struct {
	uchar data[4]; /**< Each octet in one byte */
} UriIp4; /**< @copydoc UriIp4Struct */
/**
 * Holds an IPv6 address.
 */
typedef struct UriIp6Struct {
	uchar data[16]; /**< Each quad in two bytes */
} UriIp6; /**< @copydoc UriIp6Struct */

/**
 * Specifies a line break conversion mode
 */
typedef enum UriBreakConversionEnum {
	URI_BR_TO_LF, /**< Convert to Unix line breaks ("\\x0a") */
	URI_BR_TO_CRLF, /**< Convert to Windows line breaks ("\\x0d\\x0a") */
	URI_BR_TO_CR, /**< Convert to Macintosh line breaks ("\\x0d") */
	URI_BR_TO_UNIX = URI_BR_TO_LF, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_LF */
	URI_BR_TO_WINDOWS = URI_BR_TO_CRLF, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_CRLF */
	URI_BR_TO_MAC = URI_BR_TO_CR, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_CR */
	URI_BR_DONT_TOUCH /**< Copy line breaks unmodified */
} UriBreakConversion; /**< @copydoc UriBreakConversionEnum */
//
//
//
struct UriTextRange {
	UriTextRange();
	void   Clear();
	UriTextRange & operator = (const UriTextRange & rS);
	int    Len() const;
	int    FixPercentEncodingMalloc();

	const char * P_First;     // Pointer to first character 
	const char * P_AfterLast; // Pointer to character after the last one still in 
};

struct UriHostData {
	UriIp4 * ip4; // IPv4 address 
	UriIp6 * ip6; // IPv6 address 
	UriTextRange ipFuture; // IPvFuture address 
};

struct UriUri {
	void   Destroy();

	struct PathSegment {
		PathSegment(const char * pFirst, const char * pAfterLast)
		{
			text.P_First = pFirst;
			text.P_AfterLast = pAfterLast;
			next = 0;
			reserved = 0;
		}
		UriTextRange text;     // Path segment name 
		PathSegment * next;    // Pointer to the next path segment in the list, can be NULL if last already 
		void * reserved;       // Reserved to the parser 
	};
	UriTextRange Scheme;       // Scheme (e.g. "http") 
	UriTextRange UserInfo;     // User info (e.g. "user:pass") 
	UriTextRange HostText;     // Host text (set for all hosts, excluding square brackets) 
	UriHostData  HostData;     // Structured host type specific data 
	UriTextRange PortText;     // Port (e.g. "80") 
	PathSegment * pathHead;    // Head of a linked list of path segments 
	PathSegment * pathTail;    // Tail of the list behind pathHead 
	UriTextRange query;        // Query without leading "?" 
	UriTextRange fragment;     // Query without leading "#" 
	int    IsAbsolutePath;     // Absolute path flag, distincting "a" and "/a" 
	int    IsOwner;            // Memory owner flag 
	void * reserved;           // Reserved to the parser 
};

struct UriParserState {
	UriParserState();
	void   Clear();
	void   Reset();
	int    FASTCALL ParseUriEx(const char * pFirst, const char * pAfterLast);
	const  char * FASTCALL ParseUriReference(const char * pFirst, const char * pAfterLast);
	const  char * FASTCALL ParseSegmentNzNcOrScheme2(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseMustBeSegmentNzNc(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseHexZero(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseIPv6address2(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseOwnHost(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseOwnUserInfo(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseOwnPortUserInfo(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseOwnHostUserInfoNz(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseAuthorityTwo(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseAuthority(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePartHelperTwo(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseSegment(const char * pFirst, const char * afterLast);
	void   FASTCALL StopSyntax(const char * pErrorPos);
	void   StopMalloc();

	UriUri * P_Uri; // Plug in the %URI structure to be filled while parsing here 
	int    ErrorCode; // Code identifying the occured error 
	const  char * P_ErrorPos; // Pointer to position in case of a syntax error 
	void * P_Reserved; // Reserved to the parser 
private:
	const  char * FASTCALL ParseOwnHostUserInfo(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseOwnHost2(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePctSubUnres(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseHierPart(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePathAbsNoLeadSlash(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePathAbsEmpty(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePathRootless(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseIpLit2(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseIpFuture(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePctEncoded(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePort(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseUriTail(const char * pFirst, const char * pAfterLast);
	const  char * FASTCALL ParseUriTailTwo(const char * pFirst, const char * pAfterLast);
	const  char * FASTCALL ParseQueryFrag(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseZeroMoreSlashSegs(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseSegmentNz(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParsePchar(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseIpFutLoop(const char * pFirst, const char * afterLast);
	const  char * FASTCALL ParseIpFutStopGo(const char * pFirst, const char * afterLast);
	int    FASTCALL OnExitOwnPortUserInfo(const char * pFirst);
	int    FASTCALL OnExitOwnHostUserInfo(const char * pFirst);
	int    FASTCALL OnExitOwnHost2(const char * pFirst);
	int    FASTCALL OnExitSegmentNzNcOrScheme2(const char * pFirst);
	int    FASTCALL PushPathSegment(const char * pFirst, const char *  afterLast);
};

struct UriQueryList {
	const char * key; /**< Key of the query element */
	const char * value; /**< Value of the query element, can be NULL */
	UriQueryList * next; /**< Pointer to the next key/value pair in the list, can be NULL if last already */
};

//int UriParseUriEx(UriParserState * state, const char * pFirst, const char * afterLast);
int UriParseUri(UriParserState * state, const char * text);
//void UriFreeUriMembers(UriUri*uri);
char * UriEscapeEx(const char * inFirst, const char * inAfterLast, char * out, int spaceToPlus, int normalizeBreaks);
char * UriEscape(const char * in, char * out, int spaceToPlus, int normalizeBreaks);
const char * UriUnescapeInPlaceEx(char * inout, int plusToSpace, UriBreakConversion breakConversion);
const char * UriUnescapeInPlace(char * inout);
int UriAddBaseUri(UriUri * absoluteDest, const UriUri * relativeSource, const UriUri*absoluteBase);
int UriRemoveBaseUri(UriUri * dest, const UriUri * absoluteSource, const UriUri * absoluteBase, int domainRootMode);
int UriEqualsUri(const UriUri*a, const UriUri*b);
int FASTCALL UriToStringCharsRequired(const UriUri*uri, int * charsRequired);
int UriToString(char * dest, const UriUri * uri, int maxChars, int * charsWritten);
uint FASTCALL UriNormalizeSyntaxMaskRequired(const UriUri * uri);
int FASTCALL UriNormalizeSyntaxEx(UriUri * uri, uint mask);
int FASTCALL UriNormalizeSyntax(UriUri * uri);
int UriUnixFilenameToUriString(const char * filename, char * uriString);
int UriWindowsFilenameToUriString(const char * filename, char * uriString);
int UriUriStringToUnixFilename(const char * uriString, char * filename);
int UriUriStringToWindowsFilename(const char * uriString, char * filename);
int UriComposeQueryCharsRequired(const UriQueryList*queryList, int * charsRequired);
int UriComposeQueryCharsRequiredEx(const UriQueryList*queryList, int * charsRequired, int spaceToPlus, int normalizeBreaks);
int UriComposeQuery(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten);
int UriComposeQueryEx(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten, int spaceToPlus, int normalizeBreaks);
int UriComposeQueryMalloc(char ** dest, const UriQueryList*queryList);
int UriComposeQueryMallocEx(char ** dest, const UriQueryList * queryList, int spaceToPlus, int normalizeBreaks);
int UriDissectQueryMalloc(UriQueryList**dest, int * itemCount, const char * pFirst, const char * afterLast);
int UriDissectQueryMallocEx(UriQueryList**dest, int * itemCount, const char * pFirst, const char * afterLast, int plusToSpace, UriBreakConversion breakConversion);
void UriFreeQueryList(UriQueryList*queryList);
int UriParseIpFourAddress(uchar * octetOutput, const char * pFirst, const char * afterLast);

void UriResetUri(UriUri*uri);
int UriRemoveDotSegmentsAbsolute(UriUri*uri);
int UriRemoveDotSegments(UriUri*uri, int relative);
int UriRemoveDotSegmentsEx(UriUri*uri, int relative, int pathOwned);
uchar UriHexdigToInt(char hexdig);
char UriHexToLetter(uint value);
char UriHexToLetterEx(uint value, int uppercase);
int UriIsHostSet(const UriUri * uri);
int UriCopyPath(UriUri*dest, const UriUri*source);
int UriCopyAuthority(UriUri*dest, const UriUri*source);
int UriFixAmbiguity(UriUri*uri);
void UriFixEmptyTrailSegment(UriUri*uri);


