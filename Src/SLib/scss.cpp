// SCSS.CPP
// Copyright (c) A.Sobolev 2020
// CSS-parsing and processing
//
#include <slib.h>
#include <tv.h>
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
	enum SelectorCombinator {
		scopSpace,   // SPACE Descendant combinator
		scopPlus,    // + Next-sibling combinator
		scopGreater, // > Child combinators
		scopTilde,   // ~ Subsequent-sibling combinator 
	};
	struct Selector { // @flat
		struct Attr {
			enum {
				mtExist,         // [attribute] 
				mtExact,         // [href="https://example.org"]
				mtContains,      // [href*="example"]
				mtBegins,        // [href^=".org"]
				mtEnds,          // [href$=".org"]
				mtContainsWhw,   // [attr~=value]  
				mtHyphen         // [attr|=value] Represents elements with an attribute name of attr whose value can be 
					// exactly value or can begin with value immediately followed by a hyphen, - (U+002D). It is often used for language subcode matches.
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
	};
	//
	// Один юнит таблицы стилей. Содержит соответствие набора селекторов с набором свойств.
	//
	struct Block {
		LongArray SeleIdxList;
		LongArray PropIdxList;
	};

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
	int SkipComment(SStrScan & rScan);
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

int SCssParser::Parse(const char * pText, SCss & rS)
{
	int    ok = 1;
	SStrScan scan(pText);
	scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine);
	THROW(SkipComment(scan));
	if(scan.SearchChar('{')) {
	}
	CATCHZOK
	return ok;
}