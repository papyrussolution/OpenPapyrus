/* win32tc.c -- Interface to Win32 transcoding routines

   (c) 1998-2008 (W3C) MIT, ERCIM, Keio University
   See tidy.h for the copyright notice.

   $Id: win32tc.c,v 1.12 2008/08/09 11:55:27 hoehrmann Exp $
 */

/* keep these here to keep file non-empty */
#include "tidy-int.h"
#pragma hdrstop
#include "streamio.h"
#include "tmbstr.h"
#include "utf8.h"

#ifdef TIDY_WIN32_MLANG_SUPPORT

#define VC_EXTRALEAN
#define CINTERFACE
#define COBJMACROS

//#include <windows.h>
#include <mlang.h>

#undef COBJMACROS
#undef CINTERFACE
#undef VC_EXTRALEAN

/* maximum number of bytes for a single character */
#define TC_INBUFSIZE  16

/* maximum number of characters per byte sequence */
#define TC_OUTBUFSIZE 16

#define CreateMLangObject(p) \
	CoCreateInstance( \
	    &CLSID_CMLangConvertCharset, \
	    NULL, \
	    CLSCTX_ALL,	\
	    &IID_IMLangConvertCharset, \
	    (VOID**)&p);

/* Character Set to Microsoft Windows Codepage Identifier map,     */
/* from <rotor/sscli/clr/src/classlibnative/nls/encodingdata.cpp>. */

/* note: the 'safe' field indicates whether this encoding can be   */
/* read/written character-by-character; this does not apply to     */
/* various stateful encodings such as ISO-2022 or UTF-7, these     */
/* must be read/written as a complete stream. It is possible that  */
/* some 'unsafe' encodings are marked as 'save'.                   */

/* todo: cleanup; Tidy should use only a single mapping table to   */
/* circumvent unsupported aliases in other transcoding libraries,  */
/* enable reverse lookup of encoding names and ease maintenance.   */

static struct _nameWinCPMap {
	tmbstr name;
	uint wincp;
	bool safe;
} const NameWinCPMap[] = {
	{ "cp037",                                            37, true },
	{ "csibm037",                                         37, true },
	{ "ebcdic-cp-ca",                                     37, true },
	{ "ebcdic-cp-nl",                                     37, true },
	{ "ebcdic-cp-us",                                     37, true },
	{ "ebcdic-cp-wt",                                     37, true },
	{ "ibm037",                                           37, true },
	{ "cp437",                                           437, true },
	{ "cspc8codepage437",                                437, true },
	{ "ibm437",                                          437, true },
	{ "cp500",                                           500, true },
	{ "csibm500",                                        500, true },
	{ "ebcdic-cp-be",                                    500, true },
	{ "ebcdic-cp-ch",                                    500, true },
	{ "ibm500",                                          500, true },
	{ "asmo-708",                                        708, true },
	{ "dos-720",                                         720, true },
	{ "ibm737",                                          737, true },
	{ "ibm775",                                          775, true },
	{ "cp850",                                           850, true },
	{ "ibm850",                                          850, true },
	{ "cp852",                                           852, true },
	{ "ibm852",                                          852, true },
	{ "cp855",                                           855, true },
	{ "ibm855",                                          855, true },
	{ "cp857",                                           857, true },
	{ "ibm857",                                          857, true },
	{ "ccsid00858",                                      858, true },
	{ "cp00858",                                         858, true },
	{ "cp858",                                           858, true },
	{ "ibm00858",                                        858, true },
	{ "pc-multilingual-850+euro",                        858, true },
	{ "cp860",                                           860, true },
	{ "ibm860",                                          860, true },
	{ "cp861",                                           861, true },
	{ "ibm861",                                          861, true },
	{ "cp862",                                           862, true },
	{ "dos-862",                                         862, true },
	{ "ibm862",                                          862, true },
	{ "cp863",                                           863, true },
	{ "ibm863",                                          863, true },
	{ "cp864",                                           864, true },
	{ "ibm864",                                          864, true },
	{ "cp865",                                           865, true },
	{ "ibm865",                                          865, true },
	{ "cp866",                                           866, true },
	{ "ibm866",                                          866, true },
	{ "cp869",                                           869, true },
	{ "ibm869",                                          869, true },
	{ "cp870",                                           870, true },
	{ "csibm870",                                        870, true },
	{ "ebcdic-cp-roece",                                 870, true },
	{ "ebcdic-cp-yu",                                    870, true },
	{ "ibm870",                                          870, true },
	{ "dos-874",                                         874, true },
	{ "iso-8859-11",                                     874, true },
	{ "tis-620",                                         874, true },
	{ "windows-874",                                     874, true },
	{ "cp875",                                           875, true },
	{ "csshiftjis",                                      932, true },
	{ "cswindows31j",                                    932, true },
	{ "ms_kanji",                                        932, true },
	{ "shift-jis",                                       932, true },
	{ "shift_jis",                                       932, true },
	{ "sjis",                                            932, true },
	{ "x-ms-cp932",                                      932, true },
	{ "x-sjis",                                          932, true },
	{ "chinese",                                         936, true },
	{ "cn-gb",                                           936, true },
	{ "csgb2312",                                        936, true },
	{ "csgb231280",                                      936, true },
	{ "csiso58gb231280",                                 936, true },
	{ "gb2312",                                          936, true },
	{ "gb2312-80",                                       936, true },
	{ "gb231280",                                        936, true },
	{ "gb_2312-80",                                      936, true },
	{ "gbk",                                             936, true },
	{ "iso-ir-58",                                       936, true },
	{ "csksc56011987",                                   949, true },
	{ "iso-ir-149",                                      949, true },
	{ "korean",                                          949, true },
	{ "ks-c-5601",                                       949, true },
	{ "ks-c5601",                                        949, true },
	{ "ks_c_5601",                                       949, true },
	{ "ks_c_5601-1987",                                  949, true },
	{ "ks_c_5601-1989",                                  949, true },
	{ "ks_c_5601_1987",                                  949, true },
	{ "ksc5601",                                         949, true },
	{ "ksc_5601",                                        949, true },
	{ "big5",                                            950, true },
	{ "big5-hkscs",                                      950, true },
	{ "cn-big5",                                         950, true },
	{ "csbig5",                                          950, true },
	{ "x-x-big5",                                        950, true },
	{ "cp1026",                                         1026, true },
	{ "csibm1026",                                      1026, true },
	{ "ibm1026",                                        1026, true },
	{ "ibm01047",                                       1047, true },
	{ "ccsid01140",                                     1140, true },
	{ "cp01140",                                        1140, true },
	{ "ebcdic-us-37+euro",                              1140, true },
	{ "ibm01140",                                       1140, true },
	{ "ccsid01141",                                     1141, true },
	{ "cp01141",                                        1141, true },
	{ "ebcdic-de-273+euro",                             1141, true },
	{ "ibm01141",                                       1141, true },
	{ "ccsid01142",                                     1142, true },
	{ "cp01142",                                        1142, true },
	{ "ebcdic-dk-277+euro",                             1142, true },
	{ "ebcdic-false-277+euro",                             1142, true },
	{ "ibm01142",                                       1142, true },
	{ "ccsid01143",                                     1143, true },
	{ "cp01143",                                        1143, true },
	{ "ebcdic-fi-278+euro",                             1143, true },
	{ "ebcdic-se-278+euro",                             1143, true },
	{ "ibm01143",                                       1143, true },
	{ "ccsid01144",                                     1144, true },
	{ "cp01144",                                        1144, true },
	{ "ebcdic-it-280+euro",                             1144, true },
	{ "ibm01144",                                       1144, true },
	{ "ccsid01145",                                     1145, true },
	{ "cp01145",                                        1145, true },
	{ "ebcdic-es-284+euro",                             1145, true },
	{ "ibm01145",                                       1145, true },
	{ "ccsid01146",                                     1146, true },
	{ "cp01146",                                        1146, true },
	{ "ebcdic-gb-285+euro",                             1146, true },
	{ "ibm01146",                                       1146, true },
	{ "ccsid01147",                                     1147, true },
	{ "cp01147",                                        1147, true },
	{ "ebcdic-fr-297+euro",                             1147, true },
	{ "ibm01147",                                       1147, true },
	{ "ccsid01148",                                     1148, true },
	{ "cp01148",                                        1148, true },
	{ "ebcdic-international-500+euro",                  1148, true },
	{ "ibm01148",                                       1148, true },
	{ "ccsid01149",                                     1149, true },
	{ "cp01149",                                        1149, true },
	{ "ebcdic-is-871+euro",                             1149, true },
	{ "ibm01149",                                       1149, true },
	{ "iso-10646-ucs-2",                                1200, true },
	{ "ucs-2",                                          1200, true },
	{ "unicode",                                        1200, true },
	{ "utf-16",                                         1200, true },
	{ "utf-16le",                                       1200, true },
	{ "unicodefffe",                                    1201, true },
	{ "utf-16be",                                       1201, true },
	{ "windows-1250",                                   1250, true },
	{ "x-cp1250",                                       1250, true },
	{ "windows-1251",                                   1251, true },
	{ "x-cp1251",                                       1251, true },
	{ "windows-1252",                                   1252, true },
	{ "x-ansi",                                         1252, true },
	{ "windows-1253",                                   1253, true },
	{ "windows-1254",                                   1254, true },
	{ "windows-1255",                                   1255, true },
	{ "cp1256",                                         1256, true },
	{ "windows-1256",                                   1256, true },
	{ "windows-1257",                                   1257, true },
	{ "windows-1258",                                   1258, true },
	{ "johab",                                          1361, true },
	{ "macintosh",                                     10000, true },
	{ "x-mac-japanese",                                10001, true },
	{ "x-mac-chinesetrad",                             10002, true },
	{ "x-mac-korean",                                  10003, true },
	{ "x-mac-arabic",                                  10004, true },
	{ "x-mac-hebrew",                                  10005, true },
	{ "x-mac-greek",                                   10006, true },
	{ "x-mac-cyrillic",                                10007, true },
	{ "x-mac-chinesesimp",                             10008, true },
	{ "x-mac-romanian",                                10010, true },
	{ "x-mac-ukrainian",                               10017, true },
	{ "x-mac-thai",                                    10021, true },
	{ "x-mac-ce",                                      10029, true },
	{ "x-mac-icelandic",                               10079, true },
	{ "x-mac-turkish",                                 10081, true },
	{ "x-mac-croatian",                                10082, true },
	{ "x-chinese-cns",                                 20000, true },
	{ "x-cp20001",                                     20001, true },
	{ "x-chinese-eten",                                20002, true },
	{ "x-cp20003",                                     20003, true },
	{ "x-cp20004",                                     20004, true },
	{ "x-cp20005",                                     20005, true },
	{ "irv",                                           20105, true },
	{ "x-ia5",                                         20105, true },
	{ "din_66003",                                     20106, true },
	{ "german",                                        20106, true },
	{ "x-ia5-german",                                  20106, true },
	{ "sen_850200_b",                                  20107, true },
	{ "swedish",                                       20107, true },
	{ "x-ia5-swedish",                                 20107, true },
	{ "norwegian",                                     20108, true },
	{ "ns_4551-1",                                     20108, true },
	{ "x-ia5-norwegian",                               20108, true },
	{ "ansi_x3.4-1968",                                20127, true },
	{ "ansi_x3.4-1986",                                20127, true },
	{ "ascii",                                         20127, true },
	{ "cp367",                                         20127, true },
	{ "csascii",                                       20127, true },
	{ "ibm367",                                        20127, true },
	{ "iso-ir-6",                                      20127, true },
	{ "iso646-us",                                     20127, true },
	{ "iso_646.irv:1991",                              20127, true },
	{ "us",                                            20127, true },
	{ "us-ascii",                                      20127, true },
	{ "x-cp20261",                                     20261, true },
	{ "x-cp20269",                                     20269, true },
	{ "cp273",                                         20273, true },
	{ "csibm273",                                      20273, true },
	{ "ibm273",                                        20273, true },
	{ "csibm277",                                      20277, true },
	{ "ebcdic-cp-dk",                                  20277, true },
	{ "ebcdic-cp-false",                                  20277, true },
	{ "ibm277",                                        20277, true },
	{ "cp278",                                         20278, true },
	{ "csibm278",                                      20278, true },
	{ "ebcdic-cp-fi",                                  20278, true },
	{ "ebcdic-cp-se",                                  20278, true },
	{ "ibm278",                                        20278, true },
	{ "cp280",                                         20280, true },
	{ "csibm280",                                      20280, true },
	{ "ebcdic-cp-it",                                  20280, true },
	{ "ibm280",                                        20280, true },
	{ "cp284",                                         20284, true },
	{ "csibm284",                                      20284, true },
	{ "ebcdic-cp-es",                                  20284, true },
	{ "ibm284",                                        20284, true },
	{ "cp285",                                         20285, true },
	{ "csibm285",                                      20285, true },
	{ "ebcdic-cp-gb",                                  20285, true },
	{ "ibm285",                                        20285, true },
	{ "cp290",                                         20290, true },
	{ "csibm290",                                      20290, true },
	{ "ebcdic-jp-kana",                                20290, true },
	{ "ibm290",                                        20290, true },
	{ "cp297",                                         20297, true },
	{ "csibm297",                                      20297, true },
	{ "ebcdic-cp-fr",                                  20297, true },
	{ "ibm297",                                        20297, true },
	{ "cp420",                                         20420, true },
	{ "csibm420",                                      20420, true },
	{ "ebcdic-cp-ar1",                                 20420, true },
	{ "ibm420",                                        20420, true },
	{ "cp423",                                         20423, true },
	{ "csibm423",                                      20423, true },
	{ "ebcdic-cp-gr",                                  20423, true },
	{ "ibm423",                                        20423, true },
	{ "cp424",                                         20424, true },
	{ "csibm424",                                      20424, true },
	{ "ebcdic-cp-he",                                  20424, true },
	{ "ibm424",                                        20424, true },
	{ "x-ebcdic-koreanextended",                       20833, true },
	{ "csibmthai",                                     20838, true },
	{ "ibm-thai",                                      20838, true },
	{ "cskoi8r",                                       20866, true },
	{ "koi",                                           20866, true },
	{ "koi8",                                          20866, true },
	{ "koi8-r",                                        20866, true },
	{ "koi8r",                                         20866, true },
	{ "cp871",                                         20871, true },
	{ "csibm871",                                      20871, true },
	{ "ebcdic-cp-is",                                  20871, true },
	{ "ibm871",                                        20871, true },
	{ "cp880",                                         20880, true },
	{ "csibm880",                                      20880, true },
	{ "ebcdic-cyrillic",                               20880, true },
	{ "ibm880",                                        20880, true },
	{ "cp905",                                         20905, true },
	{ "csibm905",                                      20905, true },
	{ "ebcdic-cp-tr",                                  20905, true },
	{ "ibm905",                                        20905, true },
	{ "ccsid00924",                                    20924, true },
	{ "cp00924",                                       20924, true },
	{ "ebcdic-latin9--euro",                           20924, true },
	{ "ibm00924",                                      20924, true },
	{ "x-cp20936",                                     20936, true },
	{ "x-cp20949",                                     20949, true },
	{ "cp1025",                                        21025, true },
	{ "x-cp21027",                                     21027, true },
	{ "koi8-ru",                                       21866, true },
	{ "koi8-u",                                        21866, true },
	{ "cp819",                                         28591, true },
	{ "csisolatin1",                                   28591, true },
	{ "ibm819",                                        28591, true },
	{ "iso-8859-1",                                    28591, true },
	{ "iso-ir-100",                                    28591, true },
	{ "iso8859-1",                                     28591, true },
	{ "iso_8859-1",                                    28591, true },
	{ "iso_8859-1:1987",                               28591, true },
	{ "l1",                                            28591, true },
	{ "latin1",                                        28591, true },
	{ "csisolatin2",                                   28592, true },
	{ "iso-8859-2",                                    28592, true },
	{ "iso-ir-101",                                    28592, true },
	{ "iso8859-2",                                     28592, true },
	{ "iso_8859-2",                                    28592, true },
	{ "iso_8859-2:1987",                               28592, true },
	{ "l2",                                            28592, true },
	{ "latin2",                                        28592, true },
	{ "csisolatin3",                                   28593, true },
	{ "iso-8859-3",                                    28593, true },
	{ "iso-ir-109",                                    28593, true },
	{ "iso_8859-3",                                    28593, true },
	{ "iso_8859-3:1988",                               28593, true },
	{ "l3",                                            28593, true },
	{ "latin3",                                        28593, true },
	{ "csisolatin4",                                   28594, true },
	{ "iso-8859-4",                                    28594, true },
	{ "iso-ir-110",                                    28594, true },
	{ "iso_8859-4",                                    28594, true },
	{ "iso_8859-4:1988",                               28594, true },
	{ "l4",                                            28594, true },
	{ "latin4",                                        28594, true },
	{ "csisolatincyrillic",                            28595, true },
	{ "cyrillic",                                      28595, true },
	{ "iso-8859-5",                                    28595, true },
	{ "iso-ir-144",                                    28595, true },
	{ "iso_8859-5",                                    28595, true },
	{ "iso_8859-5:1988",                               28595, true },
	{ "arabic",                                        28596, true },
	{ "csisolatinarabic",                              28596, true },
	{ "ecma-114",                                      28596, true },
	{ "iso-8859-6",                                    28596, true },
	{ "iso-ir-127",                                    28596, true },
	{ "iso_8859-6",                                    28596, true },
	{ "iso_8859-6:1987",                               28596, true },
	{ "csisolatingreek",                               28597, true },
	{ "ecma-118",                                      28597, true },
	{ "elot_928",                                      28597, true },
	{ "greek",                                         28597, true },
	{ "greek8",                                        28597, true },
	{ "iso-8859-7",                                    28597, true },
	{ "iso-ir-126",                                    28597, true },
	{ "iso_8859-7",                                    28597, true },
	{ "iso_8859-7:1987",                               28597, true },
	{ "csisolatinhebrew",                              28598, true },
	{ "hebrew",                                        28598, true },
	{ "iso-8859-8",                                    28598, true },
	{ "iso-ir-138",                                    28598, true },
	{ "iso_8859-8",                                    28598, true },
	{ "iso_8859-8:1988",                               28598, true },
	{ "logical",                                       28598, true },
	{ "visual",                                        28598, true },
	{ "csisolatin5",                                   28599, true },
	{ "iso-8859-9",                                    28599, true },
	{ "iso-ir-148",                                    28599, true },
	{ "iso_8859-9",                                    28599, true },
	{ "iso_8859-9:1989",                               28599, true },
	{ "l5",                                            28599, true },
	{ "latin5",                                        28599, true },
	{ "iso-8859-13",                                   28603, true },
	{ "csisolatin9",                                   28605, true },
	{ "iso-8859-15",                                   28605, true },
	{ "iso_8859-15",                                   28605, true },
	{ "l9",                                            28605, true },
	{ "latin9",                                        28605, true },
	{ "x-europa",                                      29001, true },
	{ "iso-8859-8-i",                                  38598, true },
	{ "iso-2022-jp",                                   50220,  false },
	{ "csiso2022jp",                                   50221,  false },
	{ "csiso2022kr",                                   50225,  false },
	{ "iso-2022-kr",                                   50225,  false },
	{ "iso-2022-kr-7",                                 50225,  false },
	{ "iso-2022-kr-7bit",                              50225,  false },
	{ "cp50227",                                       50227,  false },
	{ "x-cp50227",                                     50227,  false },
	{ "cp930",                                         50930, true },
	{ "x-ebcdic-japaneseanduscanada",                  50931, true },
	{ "cp933",                                         50933, true },
	{ "cp935",                                         50935, true },
	{ "cp937",                                         50937, true },
	{ "cp939",                                         50939, true },
	{ "cseucpkdfmtjapanese",                           51932, true },
	{ "euc-jp",                                        51932, true },
	{ "extended_unix_code_packed_format_for_japanese", 51932, true },
	{ "iso-2022-jpeuc",                                51932, true },
	{ "x-euc",                                         51932, true },
	{ "x-euc-jp",                                      51932, true },
	{ "euc-cn",                                        51936, true },
	{ "x-euc-cn",                                      51936, true },
	{ "cseuckr",                                       51949, true },
	{ "euc-kr",                                        51949, true },
	{ "iso-2022-kr-8",                                 51949, true },
	{ "iso-2022-kr-8bit",                              51949, true },
	{ "hz-gb-2312",                                    52936,  false },
	{ "gb18030",                                       54936, true },
	{ "x-iscii-de",                                    57002, true },
	{ "x-iscii-be",                                    57003, true },
	{ "x-iscii-ta",                                    57004, true },
	{ "x-iscii-te",                                    57005, true },
	{ "x-iscii-as",                                    57006, true },
	{ "x-iscii-or",                                    57007, true },
	{ "x-iscii-ka",                                    57008, true },
	{ "x-iscii-ma",                                    57009, true },
	{ "x-iscii-gu",                                    57010, true },
	{ "x-iscii-pa",                                    57011, true },
	{ "csunicode11utf7",                               65000,  false },
	{ "unicode-1-1-utf-7",                             65000,  false },
	{ "unicode-2-0-utf-7",                             65000,  false },
	{ "utf-7",                                         65000,  false },
	{ "x-unicode-1-1-utf-7",                           65000,  false },
	{ "x-unicode-2-0-utf-7",                           65000,  false },
	{ "unicode-1-1-utf-8",                             65001, true },
	{ "unicode-2-0-utf-8",                             65001, true },
	{ "utf-8",                                         65001, true },
	{ "x-unicode-1-1-utf-8",                           65001, true },
	{ "x-unicode-2-0-utf-8",                           65001, true },

	/* final entry */
	{ NULL,                                                0,  false }
};

uint TY_(Win32MLangGetCPFromName) (TidyAllocator *allocator, ctmbstr encoding)
{
	uint i;
	tmbstr enc;

	/* ensure name is in lower case */
	enc = TY_(tmbstrdup) (allocator, encoding);
	enc = TY_(tmbstrtolower) (enc);

	for(i = 0; NameWinCPMap[i].name; ++i) {
		if(TY_(tmbstrcmp) (NameWinCPMap[i].name, enc) == 0) {
			IMLangConvertCharset * p = NULL;
			uint wincp = NameWinCPMap[i].wincp;
			HRESULT hr;

			TidyFree(allocator, enc);

			/* currently false support for unsafe encodings */
			if(!NameWinCPMap[i].safe)
				return 0;

			/* hack for config.c */
			CoInitialize(NULL);
			hr = CreateMLangObject(p);

			if(hr != S_OK || !p) {
				wincp = 0;
			}
			else{
				hr = IMLangConvertCharset_Initialize(p, wincp, 1200, 0);

				if(hr != S_OK)
					wincp = 0;

				IMLangConvertCharset_Release(p);
				p = NULL;
			}

			CoUninitialize();

			return wincp;
		}
	}

	TidyFree(allocator, enc);
	return 0;
}

bool TY_(Win32MLangInitInputTranscoder) (StreamIn * in, uint wincp)
{
	IMLangConvertCharset * p = NULL;
	HRESULT hr;

	assert(in != NULL);

	CoInitialize(NULL);

	if(wincp == 0) {
		/* false codepage found for this encoding */
		return false;
	}

	hr = CreateMLangObject(p);

	if(hr != S_OK || !p) {
		/* MLang not supported */
		return false;
	}

	hr = IMLangConvertCharset_Initialize(p, wincp, 1200, 0);

	if(hr != S_OK) {
		/* encoding not supported, insufficient memory, etc. */
		return false;
	}

	in->mlang = p;

	return true;
}

void TY_(Win32MLangUninitInputTranscoder) (StreamIn * in)
{
	IMLangConvertCharset * p;

	assert(in != NULL);

	p = (IMLangConvertCharset*)in->mlang;
	if(p) {
		IMLangConvertCharset_Release(p);
		p = NULL;
		in->mlang = NULL;
	}

	CoUninitialize();
}

#if 0
bool Win32MLangInitOutputTranscoder(TidyAllocator * allocator, StreamOut * out, tmbstr encoding)
{
	IMLangConvertCharset * p = NULL;
	HRESULT hr;
	uint wincp;
	assert(out != NULL);
	CoInitialize(NULL);
	wincp = TY_(Win32MLangGetCPFromName) (allocator, encoding);
	if(wincp == 0) {
		/* false codepage found for this encoding */
		return false;
	}
	hr = CreateMLangObject(p);
	if(hr != S_OK || !p) {
		/* MLang not supported */
		return false;
	}
	IMLangConvertCharset_Initialize(p, 1200, wincp, MLCONVCHARF_NOBESTFITCHARS);
	if(hr != S_OK) {
		/* encoding not supported, insufficient memory, etc. */
		return false;
	}
	out->mlang = p;
	return true;
}

void Win32MLangUninitOutputTranscoder(StreamOut * out)
{
	IMLangConvertCharset * p;
	assert(out != NULL);
	p = (IMLangConvertCharset*)out->mlang;
	if(p) {
		IMLangConvertCharset_Release(p);
		p = NULL;
		out->mlang = NULL;
	}
	CoUninitialize();
}

#endif

int TY_(Win32MLangGetChar) (byte firstByte, StreamIn * in, uint * bytesRead)
{
	IMLangConvertCharset * p;
	TidyInputSource * source;
	CHAR inbuf[TC_INBUFSIZE] = { 0 };
	WCHAR outbuf[TC_OUTBUFSIZE] = { 0 };
	HRESULT hr = S_OK;
	size_t inbufsize = 0;

	assert(in != NULL);
	assert(&in->source != NULL);
	assert(bytesRead != NULL);
	assert(in->mlang != NULL);

	p = (IMLangConvertCharset*)in->mlang;
	source = &in->source;

	inbuf[inbufsize++] = (CHAR)firstByte;

	while(inbufsize < TC_INBUFSIZE) {
		UINT outbufsize = TC_OUTBUFSIZE;
		UINT readNow = inbufsize;
		int nextByte = EndOfStream;

		hr = IMLangConvertCharset_DoConversionToUnicode(p, inbuf, &readNow, outbuf, &outbufsize);

		assert(hr == S_OK);
		assert(outbufsize <= 2);

		if(outbufsize == 2) {
			/* U+10000-U+10FFFF are returned as a pair of surrogates */
			tchar m = (tchar)outbuf[0];
			tchar n = (tchar)outbuf[1];
			assert(TY_(IsHighSurrogate) (n) && TY_(IsLowSurrogate) (m));
			*bytesRead = readNow;
			return (int)TY_(CombineSurrogatePair) (n, m);
		}

		if(outbufsize == 1) {
			/* we found the character   */
			/* set bytesRead and return */
			*bytesRead = readNow;
			return (int)outbuf[0];
		}

		/* we need more bytes */
		nextByte = source->getByte(source->sourceData);

		if(nextByte == EndOfStream) {
			/* todo: error message for broken stream? */

			*bytesRead = readNow;
			return EndOfStream;
		}

		inbuf[inbufsize++] = (CHAR)nextByte;
	}

	/* No full character found after reading TC_INBUFSIZE bytes, */
	/* give up to read this stream, it's obviously unreadable.   */

	/* todo: error message for broken stream? */
	return EndOfStream;
}

bool Win32MLangIsConvertible(tchar c, StreamOut * out)
{
	IMLangConvertCharset * p;
	UINT i = 1;
	HRESULT hr;
	WCHAR inbuf[2] = { 0 };
	UINT inbufsize = 0;

	assert(c != 0);
	assert(c <= 0x10FFFF);
	assert(out != NULL);
	assert(out->mlang != NULL);

	if(c > 0xFFFF) {
		tchar high = 0;
		tchar low = 0;

		TY_(SplitSurrogatePair) (c, &low, &high);

		inbuf[inbufsize++] = (WCHAR)low;
		inbuf[inbufsize++] = (WCHAR)high;
	}
	else
		inbuf[inbufsize++] = (WCHAR)c;

	p = (IMLangConvertCharset*)out->mlang;
	hr = IMLangConvertCharset_DoConversionFromUnicode(p, inbuf, &inbufsize, NULL, NULL);

	return hr == S_OK ? true : false;
}

void Win32MLangPutChar(tchar c, StreamOut * out, uint * bytesWritten)
{
	IMLangConvertCharset * p;
	TidyOutputSink * sink;
	CHAR outbuf[TC_OUTBUFSIZE] = { 0 };
	UINT outbufsize = TC_OUTBUFSIZE;
	HRESULT hr = S_OK;
	WCHAR inbuf[2] = { 0 };
	UINT inbufsize = 0;
	uint i;

	assert(c != 0);
	assert(c <= 0x10FFFF);
	assert(bytesWritten != NULL);
	assert(out != NULL);
	assert(&out->sink != NULL);
	assert(out->mlang != NULL);

	p = (IMLangConvertCharset*)out->mlang;
	sink = &out->sink;

	if(c > 0xFFFF) {
		tchar high = 0;
		tchar low = 0;

		TY_(SplitSurrogatePair) (c, &low, &high);

		inbuf[inbufsize++] = (WCHAR)low;
		inbuf[inbufsize++] = (WCHAR)high;
	}
	else
		inbuf[inbufsize++] = (WCHAR)c;

	hr = IMLangConvertCharset_DoConversionFromUnicode(p, inbuf, &inbufsize, outbuf, &outbufsize);

	assert(hr == S_OK);
	assert(outbufsize > 0);
	assert(inbufsize == 1 || inbufsize == 2);

	for(i = 0; i < outbufsize; ++i)
		sink->putByte(sink->sinkData, (byte)(outbuf[i]));

	*bytesWritten = outbufsize;

	return;
}

#endif /* TIDY_WIN32_MLANG_SUPPORT */

/*
 * local variables:
 * mode: c
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * eval: (c-set-offset 'substatement-open 0)
 * end:
 */
