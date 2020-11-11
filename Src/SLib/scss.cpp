// SCSS.CPP
// Copyright (c) A.Sobolev 2020
// CSS-parsing and processing
//
#include <slib-internal.h>
#pragma hdrstop

class SCss : SStrGroup {
public:
	enum {
		propUnkn = 0,
	};
	enum {
		proptypEmpty = 0,
		proptypInherit, // "inherit"
		proptypText,
		proptypInt,
		proptypReal,
		proptypColor
	};
	
	//#define media_orientation_strings		_t("portrait;landscape")
	/*enum MediaOrientation {
		mediaorientationPortrait,
		mediaorientationLandscape,
	};*/

	//#define media_feature_strings	_t("none;width;min-width;max-width;height;min-height;max-height;device-width;min-device-width;max-device-width;device-height;min-device-height;max-device-height;orientation;aspect-ratio;min-aspect-ratio;max-aspect-ratio;device-aspect-ratio;min-device-aspect-ratio;max-device-aspect-ratio;color;min-color;max-color;color-index;min-color-index;max-color-index;monochrome;min-monochrome;max-monochrome;resolution;min-resolution;max-resolution")
	enum MediaFeature {
		mediafeatureNone,
		mediafeatureWidth,
		mediafeatureMinWidth,
		mediafeatureMaxWidth,
		mediafeatureHeight,
		mediafeatureMinHeight,
		mediafeatureMaxHeight,
		mediafeatureDeviceWidth,
		mediafeatureMinDeviceWidth,
		mediafeatureMaxDeviceWidth,
		mediafeatureDeviceHeight,
		mediafeatureMinDeviceHeight,
		mediafeatureMaxDeviceHeight,
		mediafeatureOrientation,
		mediafeatureAspectRatio,
		mediafeatureMinAspectRatio,
		mediafeatureMaxAspectRatio,
		mediafeatureDeviceAspectRatio,
		mediafeatureMinDeviceAspectRatio,
		mediafeatureMaxDeviceAspectRatio,
		mediafeatureColor,
		mediafeatureMinColor,
		mediafeatureMaxColor,
		mediafeatureColorIndex,
		mediafeatureMinColorIndex,
		mediafeatureMaxColorIndex,
		mediafeatureMonochrome,
		mediafeatureMinMonochrome,
		mediafeatureMaxMonochrome,
		mediafeatureResolution,
		mediafeatureMinResolution,
		mediafeatureMaxResolution,
	};

	//#define box_sizing_strings		_t("content-box;border-box")
	enum BoxSizing {
		boxsizingContentBox,
		boxsizingBorderBox,
	};

	//#define media_type_strings		_t("none;all;screen;print;braille;embossed;handheld;projection;speech;tty;tv")
	enum MediaType {
		mediatypeNone,
		mediatypeAll,
		mediatypeScreen,
		mediatypePrint,
		mediatypeBraille,
		mediatypeEmbossed,
		mediatypeHandheld,
		mediatypeProjection,
		mediatypeSpeech,
		mediatypeTty,
		mediatypeTv,
	};

	struct MediaFeatures {
		MediaType Type;
		int Width;        // (pixels) For continuous media, this is the width of the viewport including the size of a rendered scroll bar (if any). For paged media, this is the width of the page box.
		int Height;       // (pixels) The height of the targeted display area of the output device. For continuous media, this is the height of the viewport including the size of a rendered scroll bar (if any). For paged media, this is the height of the page box.
		int DeviceWidth;  // (pixels) The width of the rendering surface of the output device. For continuous media, this is the width of the screen. For paged media, this is the width of the page sheet size.
		int DeviceHeight; // (pixels) The height of the rendering surface of the output device. For continuous media, this is the height of the screen. For paged media, this is the height of the page sheet size.
		int Color;        // The number of bits per color component of the output device. If the device is not a color device, the value is zero.
		int ColorIndex;   // The number of entries in the color lookup table of the output device. If the device does not use a color lookup table, the value is zero.
		int Monochrome;   // The number of bits per pixel in a monochrome frame buffer. If the device is not a monochrome device, the output device value will be 0.
		int Resolution;   // The resolution of the output device (in DPI)
	};
	enum SelectorCombinator {
		scopSpace,   // SPACE Descendant combinator
		scopPlus,    // + Next-sibling combinator
		scopGreater, // > Child combinators
		scopTilde,   // ~ Subsequent-sibling combinator 
	};
	struct SelectorSpecificity {
		SelectorSpecificity() : A(0), B(0), C(0), D(0)
		{
		}
		SelectorSpecificity(int va, int vb, int vc, int vd) : A(va), B(vb), C(vc), D(vd)
		{
		}
		void operator += (const SelectorSpecificity & rS)
		{
			A += rS.A;
			B += rS.B;
			C += rS.C;
			D += rS.D;
		}
		int    FASTCALL Compare(const SelectorSpecificity & rS) const
		{
			int    si = 0;
			CMPCASCADE4(si, this, &rS, A, B, C, D);
			return si;
		}
		int    A;
		int    B;
		int    C;
		int    D;
	};
	struct Selector { // @flat
		struct Attr {
			enum { // select-condition
				mtExist,         // [attribute] 
				mtExact,         // [href="https://example.org"]
				mtContains,      // [href*="example"]
				mtBegins,        // [href^=".org"]
				mtEnds,          // [href$=".org"]
				mtContainsWhw,   // [attr~=value]  
				mtHyphen,        // [attr|=value] Represents elements with an attribute name of attr whose value can be 
					// exactly value or can begin with value immediately followed by a hyphen, - (U+002D). It is often used for language subcode matches.
				mtPseudoClass,
				mtPseudoElement
			};
			uint   AttrP;
			uint   MatchType;
			uint   ValueP;
		};
		enum {
			tOrdinary = 0,
			tClass,
			tId,
			tAll,
		};
		enum {
			conditionNone = 0,
			conditionNot,
			conditionIs,
			conditionWhere,
			conditionHas
		};
		uint   SymbP;
		uint   AttrIdx;   // Если селектор имеет атрибуты, то AttrIdx - индекс первого атрибута (Attr), AttrCount - количество атрибутов
		uint16 AttrCount;
		uint16 Type;      // Специальный тип селектора (Selector::tXXX)
		uint8  NextCombinator; // SelectorCombinator
		uint8  Reserve[3]; // @alignment
		uint   NextIdx;    // Индекс следующего селектора, связанного с этим с помощью комбинатора NextCombinator
	};

	struct Property { // @flat
		int    Id; // propXXX
		uint   PropTextP; 
		uint   ValueP;
		int    ValueType; // proptypXXX
		union {
			int    I;
			double R;
			SColor C;
		} ExtValue;
	};
	struct AtRule {
		enum {
			tEmpty = 0,
			tUnkn, // string
			tImport,
			tMedia,
		};
		int    AtRuleType; // tMedia or etc
		uint   AtRuleTypeNameP; // Ссылка на текстовое представление типа
		int    MediaType;
	};
	//
	// Один юнит таблицы стилей. Содержит соответствие набора селекторов с набором свойств.
	//
	struct Block {
		LongArray AtrIdxList;
		LongArray SeleIdxList;
		LongArray PropIdxList;
	};
	TSVector <AtRule> AtRL; // Общий контейнер at-правил
	TSVector <Selector> SeleL; // Общий контейнер селекторов
	TSVector <Property> PropL; // Общий контейнер свойств
	TSCollection <Block> Ss; // style-sheet: собственно, таблица стилей. 
};

class SCssParser {
public:
	SCssParser()
	{
	}
	~SCssParser()
	{
	}
	int Parse(const char * pText, SCss & rS);
private:
	enum {
		tokUnkn = 0,
		tokNewLine,
		tokWhitespace,
		tokIdent,         // abc
		tokFunction,      // abc(
		tokHash,          // #abc
		tokAtKeyword,     // @abc
		tokNsPrefix,      // (ident | *) '|'
		tokAsterisk,      // '*'
		tokColon,         // ':'
		tokClassSelector, // '.' ident
		tokComment,       // /**/
	};
	int    GetToken(SStrScan & rS);
	int    GetCompaundSelector(SStrScan & rS);
	int    SkipComment(SStrScan & rScan);
	//
	// Returns:
	//   0 - терм селектора не идентифицирован
	//  !0 - терм селектора идентифицирован
	//
	int    GetSelectorTerm(SStrScan & rScan, SString & rBuf);
	int    GetSelectorCondition(SStrScan & rScan);
	int    Parse_Starting_AtRule(SStrScan & rScan, SCss::Block & rB);
	int    Parse_Starting_Selector(SStrScan & rScan, SCss::Block & rB);
	int    ParseProperties(SStrScan & rScan, SCss::Block & rB);
};

int SCssParser::SkipComment(SStrScan & rScan)
{
	int    ok = 1;
	while(rScan.Is("/*")) {
		rScan.Incr(2);
		THROW(rScan.Search("*/")); // err: unterminated comment
		rScan.IncrLen();
		rScan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine);
	}
	CATCHZOK
	return ok;
}

/*
h		[0-9a-f]
nonascii	[\200-\377] // 128..257
unicode		\\{h}{1,6}[ \t\r\n\f]?
escape		{unicode}|\\[ -~\200-\377]
nmstart		[a-z]|{nonascii}|{escape}
nmchar		[a-z0-9-]|{nonascii}|{escape}

ident		[-]?{nmstart}{nmchar}*
*/

/*
<selector-list> = <complex-selector-list>
<complex-selector-list> = <complex-selector>(1..)
<compound-selector-list> = <compound-selector>(1..)
<simple-selector-list> = <simple-selector>(1..)
<relative-selector-list> = <relative-selector>(1..)
<complex-selector> = <compound-selector> [ <combinator>? <compound-selector> ]*
<relative-selector> = <combinator>? <complex-selector>
<compound-selector> = [ <type-selector>? <subclass-selector>* [ <pseudo-element-selector> <pseudo-class-selector>* ]* ]!
<simple-selector> = <type-selector> | <subclass-selector>
<combinator> = '>' | '+' | '~' | [ '|' '|' ]
<type-selector> = <wq-name> | <ns-prefix>? '*'
<ns-prefix> = [ <ident-token> | '*' ]? '|'
<wq-name> = <ns-prefix>? <ident-token>
<subclass-selector> = <id-selector> | <class-selector> | <attribute-selector> | <pseudo-class-selector>
<id-selector> = <hash-token>
<class-selector> = '.' <ident-token> // tokClassSelector
<attribute-selector> = '[' <wq-name> ']' | '[' <wq-name> <attr-matcher> [ <string-token> | <ident-token> ] <attr-modifier>? ']'
<attr-matcher> = [ '~' | '|' | '^' | '$' | '*' ]? '='
<attr-modifier> = i | s
<pseudo-class-selector> = ':' <ident-token> | ':' <function-token> <any-value> ')'
<pseudo-element-selector> = ':' <pseudo-class-selector>
*/

#if 0 //
TOKENS
	COMMENT: /* ... */
	NEWLINE: \n || \r\n || \r || \f
	WHITESPACE: ' ' || \t || NEWLINE
	
#endif 

int SCssParser::GetCompaundSelector(SStrScan & rS)
{
	//<subclass-selector> = <id-selector> | <class-selector> | <attribute-selector> | <pseudo-class-selector>
	//<compound-selector> = [ <type-selector>? <subclass-selector>* [ <pseudo-element-selector> <pseudo-class-selector>* ]* ]!
	int    ok = 0;
	int    tok = tokUnkn;
	tok = GetToken(rS);
	switch(tok) {
		case tokHash:
			break;

	}
	return ok;
}

int SCssParser::GetToken(SStrScan & rS)
{
	int    tok = tokUnkn;
	size_t p = 0;
	uchar  c = rS[p];
	int    is_space = 0;
	while(oneof5(c, '\n', '\f', '\r', ' ', '\t')) {
		uchar c2 = rS[++p];
		if(c == '\r' && c2 == '\n')
			c2 = rS[++p];
		c = c2;
		is_space = 1;
	}
	if(is_space) {
		rS.Len = p;
		tok = tokWhitespace;
	}
	else if(oneof2(c, '-', '_') || isasciialpha(c)) {
		do {
			uchar c2 = rS[++p];
			if(p == 1 && c == '-' && c2 == '-')
				c2 = rS[++p];
			c = c2;
		} while(c == '_' || isasciialpha(c) || isdec(c));
		if(c == '(') {
			rS.Len = p+1;
			tok = tokFunction;
		}
		else if(c == '|') {
			rS.Len = p+1;
			tok = tokNsPrefix;
		}
		else {
			rS.Len = p;
			tok = tokIdent;
		}
	}
	else if(c == '*') {
		uchar c2 = rS[++p];
		if(c2 == '|') {
			rS.Len = p+1;
			tok = tokNsPrefix;
		}
		else {
			rS.Len = p;
			tok = tokAsterisk;
		}
	}
	else if(c == ':') {
		rS.Len = 1;
		tok = tokColon;
	}
	else if(c == '@') {
		c = rS[++p];
		if(oneof2(c, '-', '_') || isasciialpha(c)) {
			do {
				uchar c2 = rS[++p];
				if(p == 1 && c == '-' && c2 == '-')
					c2 = rS[++p];
				c = c2;
			} while(c == '_' || isasciialpha(c) || isdec(c));
			rS.Len = p;
			tok = tokAtKeyword;
		}
	}
	else if(c == '.') {
		c = rS[++p];
		if(oneof2(c, '-', '_') || isasciialpha(c)) {
			do {
				uchar c2 = rS[++p];
				if(p == 1 && c == '-' && c2 == '-')
					c2 = rS[++p];
				c = c2;
			} while(c == '_' || isasciialpha(c) || isdec(c));
			rS.Len = p;
			tok = tokClassSelector;
		}
	}
	else if(c == '#') {
		do {
			c = rS[++p];
		} while(oneof2(c, '-', '_') || isasciialpha(c) || isdec(c));
		if(p > 1) {
			rS.Len = p;
			tok = tokHash;
		}
	}
	else if(c == '/' && rS[1] == '*') {
		p++;
		do {
			c = rS[++p];
		} while(c && c != '*' && rS[1] != '/');
		if(c) {
			rS.Len = p+1;
			tok = tokComment;
		}
	}
	return tok;
}

int SCssParser::GetSelectorCondition(SStrScan & rScan)
{
	int    ok = 0;
	if(rScan[0] == '[') {
	}
	else if(rScan.Is(":not(")) {
	}
	else if(rScan.Is(":is(")) {
	}
	else if(rScan.Is(":where(")) {
	}
	else if(rScan.Is(":has(")) {
	}
	return ok;
}

int SCssParser::GetSelectorTerm(SStrScan & rScan, SString & rBuf)
{
	uchar  ok = 0;
	rBuf.Z();
	size_t p = 0;
	uchar  c = rScan[p];
	if(oneof4(c, '-', '#', '.', '*') || isasciialpha(c)) {
		do {
			if(oneof4(c, '-', '#', '.', '*') || isasciialpha(c)) {
				do {
					rBuf.CatChar(c);
					c = rScan[++p];
				} while(oneof4(c, '-', '#', '.', ':') || isasciialpha(c) || isdec(c));
			}
			if(c == '[') {
				do {
					rBuf.CatChar(c);
					c = rScan[++p];
				} while(c && c != ']');
				THROW(c == ']');
				rBuf.CatChar(c);
			}
			else if(c == '(') {
				do {
					rBuf.CatChar(c);
					c = rScan[++p];
				} while(c && c != ')');
				THROW(c == ')');
				rBuf.CatChar(c);
			}
		} while(oneof4(c, '.', ':', '[', '('));
		rScan.Incr(p);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SCssParser::Parse_Starting_AtRule(SStrScan & rScan, SCss::Block & rB)
{
	int    ok = 1;
	SString temp_buf;
	char cur = rScan[0];
	THROW(cur == '@');
	while(cur && !oneof3(cur, ' ', '\t', '\r')) {
	}
	CATCHZOK
	return ok;
}

int SCssParser::Parse_Starting_Selector(SStrScan & rScan, SCss::Block & rB)
{
	int    ok = 1;
	return ok;
}

int SCssParser::ParseProperties(SStrScan & rScan, SCss::Block & rB)
{
	int    ok = 1;
	return ok;
}

int SCssParser::Parse(const char * pText, SCss & rS)
{
	/*
		[ @atrule rule { ] selector-list { (attr:value;)* }
	*/
	int    ok = 1;
	SStrScan scan(pText);
	scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine);
	THROW(SkipComment(scan));
	while(!scan.IsEnd() && !scan.Is('{')) {
		SCss::Block blk;
		if(scan.Is('@'))
			Parse_Starting_AtRule(scan, blk);
		else
			Parse_Starting_Selector(scan, blk);
	}
	CATCHZOK
	return ok;
}
