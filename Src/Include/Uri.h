// URI.H
// Copyright (C) 2007, Weijia Song <songweijia@gmail.com>, Sebastian Pipping <webmaster@hartwork.org> All rights reserved.
// Adopted to SLIB by A.Sobolev 2011-2021, 2024
//
#ifndef __URI_H // {
#define __URI_H

#include <slib.h>
// 
// Specifies a line break conversion mode
// 
enum UriBreakConversion {
	URI_BR_TO_LF, /**< Convert to Unix line breaks ("\\x0a") */
	URI_BR_TO_CRLF, /**< Convert to Windows line breaks ("\\x0d\\x0a") */
	URI_BR_TO_CR, /**< Convert to Macintosh line breaks ("\\x0d") */
	URI_BR_TO_UNIX = URI_BR_TO_LF, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_LF */
	URI_BR_TO_WINDOWS = URI_BR_TO_CRLF, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_CRLF */
	URI_BR_TO_MAC = URI_BR_TO_CR, /**< @copydoc UriBreakConversionEnum::URI_BR_TO_CR */
	URI_BR_DONT_TOUCH /**< Copy line breaks unmodified */
}; /**< @copydoc UriBreakConversionEnum */
//
//
//
struct UriTextRange {
	UriTextRange();
	UriTextRange(const char * pFirst, const char * pAfterLast);
	void   Clear();
	UriTextRange & FASTCALL operator = (const UriTextRange & rS);
	int    Len() const { return (int)(P_AfterLast - P_First); }

	const char * P_First;     // Pointer to first character 
	const char * P_AfterLast; // Pointer to character after the last one still in 
};

struct UriUri {
	UriUri()
	{
		THISZERO();
	}
	~UriUri()
	{
		Destroy();
	}
	void   Destroy();
	// 
	// Descr: Holds an IPv4 address.
	// 
	struct UriIp4 {
		uchar data[4]; // Each octet in one byte
	};
	// 
	// Descr: Holds an IPv6 address.
	// 
	struct UriIp6 {
		uchar data[16]; // Each quad in two bytes
	};
	struct HostData {
		UriIp4 * ip4; // IPv4 address 
		UriIp6 * ip6; // IPv6 address 
		UriTextRange ipFuture; // IPvFuture address 
	};
	struct PathSegment {
		PathSegment(const char * pFirst, const char * pAfterLast);
		UriTextRange text;     // Path segment name 
		PathSegment * next;    // Pointer to the next path segment in the list, can be NULL if last already 
		void * reserved;       // Reserved to the parser 
	};
	UriTextRange Scheme;       // Scheme (e.g. "http") 
	UriTextRange UserInfo;     // User info (e.g. "user:pass") 
	UriTextRange HostText;     // Host text (set for all hosts, excluding square brackets) 
	HostData   HostData;       // Structured host type specific data 
	UriTextRange PortText;     // Port (e.g. "80") 
	PathSegment * pathHead;    // Head of a linked list of path segments 
	PathSegment * pathTail;    // Tail of the list behind pathHead 
	UriTextRange query;        // Query without leading "?" 
	UriTextRange fragment;     // Query without leading "#" 
	int    IsAbsolutePath;     // Absolute path flag, distincting "a" and "/a" 
	int    IsOwner;            // Memory owner flag 
	void * reserved;           // Reserved to the parser 
};

class UriParserState {
public:
	explicit UriParserState(UriUri * pResultData);
	void   Clear();
	void   Reset();
	int    STDCALL ParseUriEx(const char * pFirst, const char * pAfterLast);
	const  char * STDCALL ParseIPv6address2(const char * pFirst, const char * afterLast);
private:
	const  char * STDCALL ParseUriReference(const char * pFirst, const char * pAfterLast);
	const  char * STDCALL ParseSegmentNzNcOrScheme2(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseMustBeSegmentNzNc(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseHexZero(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseOwnHost(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseOwnUserInfo(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseOwnPortUserInfo(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseOwnHostUserInfoNz(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseAuthorityTwo(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseAuthority(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePartHelperTwo(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseSegment(const char * pFirst, const char * afterLast);
	// Returns: NULL ALWAYS!
	const  char * FASTCALL StopSyntax(const char * pErrorPos);
	void   StopMalloc();

	UriUri * P_Uri; // Plug in the %URI structure to be filled while parsing here 
	int    ErrorCode; // Code identifying the occured error 
	const  char * P_ErrorPos; // Pointer to position in case of a syntax error 
	void * P_Reserved; // Reserved to the parser 
private:
	const  char * STDCALL ParseOwnHostUserInfo(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseOwnHost2(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePctSubUnres(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseHierPart(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePathAbsNoLeadSlash(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePathAbsEmpty(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePathRootless(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseIpLit2(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseIpFuture(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePctEncoded(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePort(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseUriTail(const char * pFirst, const char * pAfterLast);
	const  char * STDCALL ParseUriTailTwo(const char * pFirst, const char * pAfterLast);
	const  char * STDCALL ParseQueryFrag(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseZeroMoreSlashSegs(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseSegmentNz(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParsePchar(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseIpFutLoop(const char * pFirst, const char * afterLast);
	const  char * STDCALL ParseIpFutStopGo(const char * pFirst, const char * afterLast);
	int    FASTCALL OnExitOwnPortUserInfo(const char * pFirst);
	int    FASTCALL OnExitOwnHostUserInfo(const char * pFirst);
	int    FASTCALL OnExitOwnHost2(const char * pFirst);
	int    FASTCALL OnExitSegmentNzNcOrScheme2(const char * pFirst);
	int    STDCALL PushPathSegment(const char * pFirst, const char * afterLast);
};

struct UriQueryList {
	const char * key; /**< Key of the query element */
	const char * value; /**< Value of the query element, can be NULL */
	UriQueryList * next; /**< Pointer to the next key/value pair in the list, can be NULL if last already */
};

int    UriParseUri(UriParserState * state, const char * text);
int    UriEqualsUri(const UriUri*a, const UriUri*b);
int    FASTCALL UriToStringCharsRequired(const UriUri*uri, int * charsRequired);
int    UriToString(char * dest, const UriUri * uri, int maxChars, int * charsWritten);
uint   FASTCALL UriNormalizeSyntaxMaskRequired(const UriUri * uri);
int    FASTCALL UriNormalizeSyntaxEx(UriUri * uri, uint mask);
int    FASTCALL UriNormalizeSyntax(UriUri * uri);
int    UriUnixFilenameToUriString(const char * filename, char * uriString);
int    UriWindowsFilenameToUriString(const char * filename, char * uriString);
int    UriUriStringToUnixFilename(const char * uriString, char * filename);
int    UriUriStringToWindowsFilename(const char * uriString, char * filename);
int    UriComposeQueryCharsRequired(const UriQueryList*queryList, int * charsRequired);
int    UriComposeQueryCharsRequiredEx(const UriQueryList*queryList, int * charsRequired, int spaceToPlus, int normalizeBreaks);
int    UriComposeQuery(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten);
int    UriComposeQueryEx(char * dest, const UriQueryList*queryList, int maxChars, int * charsWritten, int spaceToPlus, int normalizeBreaks);
int    UriComposeQueryMalloc(char ** dest, const UriQueryList*queryList);
int    UriComposeQueryMallocEx(char ** dest, const UriQueryList * queryList, int spaceToPlus, int normalizeBreaks);
int    UriDissectQueryMalloc(UriQueryList**dest, int * itemCount, const char * pFirst, const char * afterLast);
int    UriDissectQueryMallocEx(UriQueryList**dest, int * itemCount, const char * pFirst, const char * afterLast, int plusToSpace, UriBreakConversion breakConversion);
void   UriFreeQueryList(UriQueryList*queryList);
int    UriParseIpFourAddress(uchar * octetOutput, const char * pFirst, const char * afterLast);
void   UriResetUri(UriUri*uri);
int    UriRemoveDotSegmentsAbsolute(UriUri * uri);
int    UriRemoveDotSegments(UriUri * uri, int relative);
int    UriRemoveDotSegmentsEx(UriUri * uri, int relative, int pathOwned);
uchar  FASTCALL UriHexdigToInt(char hexdig);
char   FASTCALL UriHexToLetter(uint value);
char   FASTCALL UriHexToLetterEx(uint value, int uppercase);
int    UriIsHostSet(const UriUri * uri);
int    UriCopyPath(UriUri * dest, const UriUri * source);
int    UriCopyAuthority(UriUri * dest, const UriUri * source);
int    UriFixAmbiguity(UriUri * uri);
void   UriFixEmptyTrailSegment(UriUri * uri);

#endif // } __URI_H
