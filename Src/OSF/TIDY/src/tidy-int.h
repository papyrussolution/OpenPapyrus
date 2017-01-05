#ifndef __TIDY_INT_H__
#define __TIDY_INT_H__

/* tidy-int.h -- internal library declarations

   (c) 1998-2007 (W3C) MIT, ERCIM, Keio University
   See tidy.h for the copyright notice.

   CVS Info :

    $Author: arnaud02 $
    $Date: 2007/02/11 09:45:52 $
    $Revision: 1.13 $

 */
#include <slib.h>
#include "tidy.h"
#include "config.h"
#include "lexer.h"
#include "attrs.h"
#include "pprint.h"
#include "access.h"
//
// TAGS {
//
#include "forward.h"
#include "attrdict.h"

typedef void (Parser)(TidyDocImpl* doc, Node * node, GetTokenMode mode);
typedef void (CheckAttribs)(TidyDocImpl* doc, Node * node);
//
// Tag dictionary node
//
// types of tags that the user can define
//
typedef enum {
	tagtype_null = 0,
	tagtype_empty = 1,
	tagtype_inline = 2,
	tagtype_block = 4,
	tagtype_pre = 8
} UserTagType;

struct _Dict {
	TidyTagId id;
	tmbstr name;
	uint   versions;
	AttrVersion const * attrvers;
	uint   model;
	Parser * parser;
	CheckAttribs * chkattrs;
	Dict * next;
};

#if !defined(ELEMENT_HASH_LOOKUP)
	#define ELEMENT_HASH_LOOKUP 1
#endif
#if ELEMENT_HASH_LOOKUP
enum {
	ELEMENT_HASH_SIZE = 178u
};

struct _DictHash {
	Dict const*         tag;
	struct _DictHash*   next;
};

typedef struct _DictHash DictHash;
#endif

struct _TidyTagImpl {
	Dict* xml_tags;            /* placeholder for all xml tags */
	Dict* declared_tag_list;   /* User declared tags */
#if ELEMENT_HASH_LOOKUP
	DictHash* hashtab[ELEMENT_HASH_SIZE];
#endif
};

typedef struct _TidyTagImpl TidyTagImpl;
//
// interface for finding tag by name
//
const Dict* TY_(LookupTagDef) (TidyTagId tid);
bool TY_(FindTag) (TidyDocImpl* doc, Node *node);
Parser* TY_(FindParser) (TidyDocImpl* doc, Node *node);
void TY_(DefineTag) (TidyDocImpl* doc, UserTagType tagType, ctmbstr name);
void TY_(FreeDeclaredTags) (TidyDocImpl* doc, UserTagType tagType);     /* tagtype_null to free all */

TidyIterator TY_(GetDeclaredTagList) (TidyDocImpl* doc);
ctmbstr TY_(GetNextDeclaredTag) (TidyDocImpl* doc, UserTagType tagType, TidyIterator* iter);

void TY_(InitTags) (TidyDocImpl* doc);
void TY_(FreeTags) (TidyDocImpl* doc);
//
// Parser methods for tags
//
Parser TY_(ParseHTML);
Parser TY_(ParseHead);
Parser TY_(ParseTitle);
Parser TY_(ParseScript);
Parser TY_(ParseFrameSet);
Parser TY_(ParseNoFrames);
Parser TY_(ParseBody);
Parser TY_(ParsePre);
Parser TY_(ParseList);
Parser TY_(ParseDefList);
Parser TY_(ParseBlock);
Parser TY_(ParseInline);
Parser TY_(ParseEmpty);
Parser TY_(ParseTableTag);
Parser TY_(ParseColGroup);
Parser TY_(ParseRowGroup);
Parser TY_(ParseRow);
Parser TY_(ParseSelect);
Parser TY_(ParseOptGroup);
Parser TY_(ParseText);

CheckAttribs TY_(CheckAttributes);

/* 0 == TidyTag_UNKNOWN */
#define TagId(node)        ((node) && (node)->tag ? (node)->tag->id : TidyTag_UNKNOWN)
#define TagIsId(node, tid) ((node) && (node)->tag && (node)->tag->id == tid)

bool TY_(nodeIsText) (Node* node);
bool TY_(nodeIsElement) (Node* node);

bool TY_(nodeHasText) (TidyDocImpl* doc, Node* node);

#if 0
//
// Compare & result to operand.  If equal, then all bits
// requested are set.
//
bool nodeMatchCM(Node* node, uint contentModel);
#endif
//
// True if any of the bits requested are set.
//
bool TY_(nodeHasCM) (Node* node, uint contentModel);

bool TY_(nodeCMIsBlock) (Node* node);
bool TY_(nodeCMIsInline) (Node* node);
bool TY_(nodeCMIsEmpty) (Node* node);

bool TY_(nodeIsHeader) (Node* node);      /* H1, H2, ..., H6 */
uint TY_(nodeHeaderLevel) (Node* node);   /* 1, 2, ..., 6 */

#define nodeIsHTML(node)       TagIsId(node, TidyTag_HTML)
#define nodeIsHEAD(node)       TagIsId(node, TidyTag_HEAD)
#define nodeIsTITLE(node)      TagIsId(node, TidyTag_TITLE)
#define nodeIsBASE(node)       TagIsId(node, TidyTag_BASE)
#define nodeIsMETA(node)       TagIsId(node, TidyTag_META)
#define nodeIsBODY(node)       TagIsId(node, TidyTag_BODY)
#define nodeIsFRAMESET(node)   TagIsId(node, TidyTag_FRAMESET)
#define nodeIsFRAME(node)      TagIsId(node, TidyTag_FRAME)
#define nodeIsIFRAME(node)     TagIsId(node, TidyTag_IFRAME)
#define nodeIsNOFRAMES(node)   TagIsId(node, TidyTag_NOFRAMES)
#define nodeIsHR(node)         TagIsId(node, TidyTag_HR)
#define nodeIsH1(node)         TagIsId(node, TidyTag_H1)
#define nodeIsH2(node)         TagIsId(node, TidyTag_H2)
#define nodeIsPRE(node)        TagIsId(node, TidyTag_PRE)
#define nodeIsLISTING(node)    TagIsId(node, TidyTag_LISTING)
#define nodeIsP(node)          TagIsId(node, TidyTag_P)
#define nodeIsUL(node)         TagIsId(node, TidyTag_UL)
#define nodeIsOL(node)         TagIsId(node, TidyTag_OL)
#define nodeIsDL(node)         TagIsId(node, TidyTag_DL)
#define nodeIsDIR(node)        TagIsId(node, TidyTag_DIR)
#define nodeIsLI(node)         TagIsId(node, TidyTag_LI)
#define nodeIsDT(node)         TagIsId(node, TidyTag_DT)
#define nodeIsDD(node)         TagIsId(node, TidyTag_DD)
#define nodeIsTABLE(node)      TagIsId(node, TidyTag_TABLE)
#define nodeIsCAPTION(node)    TagIsId(node, TidyTag_CAPTION)
#define nodeIsTD(node)         TagIsId(node, TidyTag_TD)
#define nodeIsTH(node)         TagIsId(node, TidyTag_TH)
#define nodeIsTR(node)         TagIsId(node, TidyTag_TR)
#define nodeIsCOL(node)        TagIsId(node, TidyTag_COL)
#define nodeIsCOLGROUP(node)   TagIsId(node, TidyTag_COLGROUP)
#define nodeIsBR(node)         TagIsId(node, TidyTag_BR)
#define nodeIsA(node)          TagIsId(node, TidyTag_A)
#define nodeIsLINK(node)       TagIsId(node, TidyTag_LINK)
#define nodeIsB(node)          TagIsId(node, TidyTag_B)
#define nodeIsI(node)          TagIsId(node, TidyTag_I)
#define nodeIsSTRONG(node)     TagIsId(node, TidyTag_STRONG)
#define nodeIsEM(node)         TagIsId(node, TidyTag_EM)
#define nodeIsBIG(node)        TagIsId(node, TidyTag_BIG)
#define nodeIsSMALL(node)      TagIsId(node, TidyTag_SMALL)
#define nodeIsPARAM(node)      TagIsId(node, TidyTag_PARAM)
#define nodeIsOPTION(node)     TagIsId(node, TidyTag_OPTION)
#define nodeIsOPTGROUP(node)   TagIsId(node, TidyTag_OPTGROUP)
#define nodeIsIMG(node)        TagIsId(node, TidyTag_IMG)
#define nodeIsMAP(node)        TagIsId(node, TidyTag_MAP)
#define nodeIsAREA(node)       TagIsId(node, TidyTag_AREA)
#define nodeIsNOBR(node)       TagIsId(node, TidyTag_NOBR)
#define nodeIsWBR(node)        TagIsId(node, TidyTag_WBR)
#define nodeIsFONT(node)       TagIsId(node, TidyTag_FONT)
#define nodeIsLAYER(node)      TagIsId(node, TidyTag_LAYER)
#define nodeIsSPACER(node)     TagIsId(node, TidyTag_SPACER)
#define nodeIsCENTER(node)     TagIsId(node, TidyTag_CENTER)
#define nodeIsSTYLE(node)      TagIsId(node, TidyTag_STYLE)
#define nodeIsSCRIPT(node)     TagIsId(node, TidyTag_SCRIPT)
#define nodeIsNOSCRIPT(node)   TagIsId(node, TidyTag_NOSCRIPT)
#define nodeIsFORM(node)       TagIsId(node, TidyTag_FORM)
#define nodeIsTEXTAREA(node)   TagIsId(node, TidyTag_TEXTAREA)
#define nodeIsBLOCKQUOTE(node) TagIsId(node, TidyTag_BLOCKQUOTE)
#define nodeIsAPPLET(node)     TagIsId(node, TidyTag_APPLET)
#define nodeIsOBJECT(node)     TagIsId(node, TidyTag_OBJECT)
#define nodeIsDIV(node)        TagIsId(node, TidyTag_DIV)
#define nodeIsSPAN(node)       TagIsId(node, TidyTag_SPAN)
#define nodeIsINPUT(node)      TagIsId(node, TidyTag_INPUT)
#define nodeIsQ(node)          TagIsId(node, TidyTag_Q)
#define nodeIsLABEL(node)      TagIsId(node, TidyTag_LABEL)
#define nodeIsH3(node)         TagIsId(node, TidyTag_H3)
#define nodeIsH4(node)         TagIsId(node, TidyTag_H4)
#define nodeIsH5(node)         TagIsId(node, TidyTag_H5)
#define nodeIsH6(node)         TagIsId(node, TidyTag_H6)
#define nodeIsADDRESS(node)    TagIsId(node, TidyTag_ADDRESS)
#define nodeIsXMP(node)        TagIsId(node, TidyTag_XMP)
#define nodeIsSELECT(node)     TagIsId(node, TidyTag_SELECT)
#define nodeIsBLINK(node)      TagIsId(node, TidyTag_BLINK)
#define nodeIsMARQUEE(node)    TagIsId(node, TidyTag_MARQUEE)
#define nodeIsEMBED(node)      TagIsId(node, TidyTag_EMBED)
#define nodeIsBASEFONT(node)   TagIsId(node, TidyTag_BASEFONT)
#define nodeIsISINDEX(node)    TagIsId(node, TidyTag_ISINDEX)
#define nodeIsS(node)          TagIsId(node, TidyTag_S)
#define nodeIsSTRIKE(node)     TagIsId(node, TidyTag_STRIKE)
#define nodeIsSUB(node)        TagIsId(node, TidyTag_SUB)
#define nodeIsSUP(node)        TagIsId(node, TidyTag_SUP)
#define nodeIsU(node)          TagIsId(node, TidyTag_U)
#define nodeIsMENU(node)       TagIsId(node, TidyTag_MENU)
#define nodeIsBUTTON(node)     TagIsId(node, TidyTag_BUTTON)
//
// } TAGS
//
struct _TidyDocImpl {
	/* The Document Tree (and backing store buffer) */
	Node root; // This MUST remain the first declared variable in this structure
	Lexer * lexer;
	/* Config + Markup Declarations */
	TidyConfigImpl config;
	TidyTagImpl tags;
	TidyAttribImpl attribs;
#if SUPPORT_ACCESSIBILITY_CHECKS
	/* Accessibility Checks state */
	TidyAccessImpl access;
#endif
	/* The Pretty Print buffer */
	TidyPrintImpl pprint;
	/* I/O */
	StreamIn*           docIn;
	StreamOut*          docOut;
	StreamOut*          errout;
	TidyReportFilter mssgFilt;
	TidyOptCallback pOptCallback;

	/* Parse + Repair Results */
	uint optionErrors;
	uint errors;
	uint warnings;
	uint accessErrors;
	uint infoMessages;
	uint docErrors;
	int parseStatus;

	uint badAccess;              /* for accessibility errors */
	uint badLayout;              /* for bad style errors */
	uint badChars;               /* for bad char encodings */
	uint badForm;                /* for badly placed form tags */

	/* Memory allocator */
	TidyAllocator*      allocator;

	/* Miscellaneous */
	void*               appData;
	uint nClassId;
	bool inputHadBOM;
#ifdef TIDY_STORE_ORIGINAL_TEXT
	bool storeText;
#endif
#if PRESERVE_FILE_TIMES
	struct utimbuf filetimes;
#endif
	tmbstr givenDoctype;
};
//
// Twizzle internal/external types
//
#ifdef NEVER
	TidyDocImpl * tidyDocToImpl(TidyDoc tdoc);
	TidyDoc      tidyImplToDoc(TidyDocImpl* impl);
	Node*        tidyNodeToImpl(TidyNode tnod);
	TidyNode     tidyImplToNode(Node* node);
	AttVal*      tidyAttrToImpl(TidyAttr tattr);
	TidyAttr     tidyImplToAttr(AttVal* attval);
	const TidyOptionImpl* tidyOptionToImpl(TidyOption topt);
	TidyOption   tidyImplToOption(const TidyOptionImpl* option);
#else
	#define tidyDocToImpl(tdoc)       ((TidyDocImpl*)(tdoc))
	#define tidyImplToDoc(doc)        ((TidyDoc)(doc))
	#define tidyNodeToImpl(tnod)      ((Node*)(tnod))
	#define tidyImplToNode(node)      ((TidyNode)(node))
	#define tidyAttrToImpl(tattr)     ((AttVal*)(tattr))
	#define tidyImplToAttr(attval)    ((TidyAttr)(attval))
	#define tidyOptionToImpl(topt)    ((const TidyOptionImpl*)(topt))
	#define tidyImplToOption(option)  ((TidyOption)(option))
#endif

/** Wrappers for easy memory allocation using the document's allocator */
#define TidyDocAlloc(doc, size) TidyAlloc((doc)->allocator, size)
#define TidyDocRealloc(doc, block, size) TidyRealloc((doc)->allocator, block, size)
#define TidyDocFree(doc, block) TidyFree((doc)->allocator, block)
#define TidyDocPanic(doc, msg) TidyPanic((doc)->allocator, msg)

int TY_(DocParseStream) (TidyDocImpl* impl, StreamIn* in);
//
// CLEAN {
//
void TY_(FixNodeLinks) (Node *node);
void TY_(FreeStyles) (TidyDocImpl* doc);
//
// Add class="foo" to node
//
void TY_(AddStyleAsClass) (TidyDocImpl* doc, Node *node, ctmbstr stylevalue);
void TY_(AddStyleProperty) (TidyDocImpl* doc, Node *node, ctmbstr property);
void TY_(CleanDocument) (TidyDocImpl* doc);
/* simplifies <b><b> ... </b> ...</b> etc. */
void TY_(NestedEmphasis) (TidyDocImpl* doc, Node* node);
/* replace i by em and b by strong */
void TY_(EmFromI) (TidyDocImpl* doc, Node* node);
/*
   Some people use dir or ul without an li
   to indent the content. The pattern to
   look for is a list with a single implicit
   li. This is recursively replaced by an
   implicit blockquote.
 */
void TY_(List2BQ) (TidyDocImpl* doc, Node* node);
/*
   Replace implicit blockquote by div with an indent
   taking care to reduce nested blockquotes to a single
   div with the indent set to match the nesting depth
*/
void TY_(BQ2Div) (TidyDocImpl* doc, Node* node);
void TY_(DropSections) (TidyDocImpl* doc, Node* node);
/*
   This is a major clean up to strip out all the extra stuff you get
   when you save as web page from Word 2000. It doesn't yet know what
   to do with VML tags, but these will appear as errors unless you
   declare them as new tags, such as o:p which needs to be declared
   as inline.
*/
void TY_(CleanWord2000) (TidyDocImpl* doc, Node *node);
bool TY_(IsWord2000) (TidyDocImpl* doc);
/* where appropriate move object elements from head to body */
void TY_(BumpObject) (TidyDocImpl* doc, Node *html);
/* This is disabled due to http://tidy.sf.net/bug/681116 */
#if 0
	void TY_(FixBrakes) (TidyDocImpl* pDoc, Node *pParent);
#endif
void TY_(VerifyHTTPEquiv) (TidyDocImpl* pDoc, Node *pParent);
void TY_(DropComments) (TidyDocImpl* doc, Node* node);
void TY_(DropFontElements) (TidyDocImpl* doc, Node* node, Node **pnode);
void TY_(WbrToSpace) (TidyDocImpl* doc, Node* node);
void TY_(DowngradeTypography) (TidyDocImpl* doc, Node* node);
void TY_(ReplacePreformattedSpaces) (TidyDocImpl* doc, Node* node);
void TY_(NormalizeSpaces) (Lexer *lexer, Node *node);
void TY_(ConvertCDATANodes) (TidyDocImpl* doc, Node* node);
void TY_(FixAnchors) (TidyDocImpl* doc, Node *node, bool wantName, bool wantId);
void TY_(FixXhtmlNamespace) (TidyDocImpl* doc, bool wantXmlns);
void TY_(FixLanguageInformation) (TidyDocImpl* doc, Node* node, bool wantXmlLang, bool wantLang);
//
// } CLEAN
//
#endif /* __TIDY_INT_H__ */
