// PARSCMP.CPP
// Copyright (c) Sobolev A. 1995-2001, 2008, 2010, 2011, 2016
//
//#define TEST_PARSER

#include <slib.h>
#include <tv.h>
#pragma hdrstop

/*  Syntax:

	Compare_Expression
		EOL
		Asterisk EOL
		Compare_Term_List
	Compare_Term_List
		Compare_Term EOL
		Compare_Term Delimiter Compare_Term_List EOL
	Compare_Term:
		Compare_Sign Data
		Double_Dot Data
		Data
		Data Double_Dot
		Data Double_Dot Data
	Asterisk
		*
		ALL
	Delimiter
		,
		;
	EOL
		\0
		\n
	Compare_Sign
		>
		<
		>=
		<=
		=
		!=
	Double_Dot
		..
*/

#define LIST_DELTA 4
/*
	Лексемы
*/
enum {
	lexError = -1,       // Ошибочная лексема                             //
	lexDelim =  0,       // Разделитель                                   //
	lexAsterisk,         // Звездочка ("*" )                            //
	lexDoubleDot,        // Две точки ("..")                            //
	lexLE,               // <=                                            //
	lexGE,               // >=                                            //
	lexEQ,               // =                                             //
	lexLT,               // <                                             //
	lexGT,               // >                                             //
	lexNE,               // !=
	lexNum,              // Число  }  Obsolete									 //
	lexDate,             // Дата   }  Obsolete
	lexAcct,             // Счет   }  Obsolete
	lexData,             // Данные
	lexEOL,              // Конец строки ('\0' или '\n')                //
	numLexArrayItems = 13
};

struct {
	char sym[12];
	int  lex;
	int  len;
} lexAssoc[] =
{
	{ ",", lexDelim, 1 },
	{ ";", lexDelim, 1 },
	{ "*", lexAsterisk, 1 },
	{ "ALL", lexAsterisk, 3 },
	{ "..", lexDoubleDot, 2 },
	{ ">=", lexGE, 2 },
	{ "<=", lexLE, 2 },
	{ ">", lexGT, 1 },
	{ "<", lexLT, 1 },
	{ "=", lexEQ, 1 },
	{ "!=", lexNE, 2 },
	{ "\0", lexEOL, 1 },
	{ "\n", lexEOL, 1 }
};

static is_data_func  is_data       = is_number;
static get_data_func get_data      = get_number;
static int           token         = 0;
static int           index         = 0;
static int           prevIndex     = 0;
static int           currentTerm   = 0;
static int           numberOfTerms = 0;
static CompareTerm * termList      = NULL;

int SLAPI set_data_func(is_data_func idf, get_data_func gdf)
{
	is_data  = idf;
	get_data = gdf;
	return 1;
}

int SLAPI is_number(char * buf)
{
	return (isdigit(*buf) || *buf == '-');
}

int SLAPI is_date(char * buf)
{
	return (isdigit(*buf));
}

int SLAPI get_number(char * str, int * index, char * data)
{
	int  i    = 0;
	int  pf   = 0;
	int  sign = 0;
	int  idx  = *index;
	int  ok   = -1;
	char ch;
	char vstr[32];
	LDBL number;

	do {
		if(!isdigit(ch = str[idx]))
			if(ch == '.' && str[idx + 1] != '.' && !pf)
				pf = 1;
			else
				if(ch != '-' || sign) {
					vstr[i] = '\0';
					if(sscanf(vstr, "%Lf", &number) == 0)
						ok = 0;
					break;
				}
		vstr[i++] = ch;
		idx++;
		sign = 1;
	} while(1);
	*index = idx;
	if(ok)
		memcpy(data, &number, 10);
	return ok;
}

int SLAPI get_date(char * str, int * index, char * data)
{
	int idx = *index;
	idx = strtodate(str + idx, TxtDateFmt, data);
	*index += idx;
	return checkdate(data);
}

static int SLAPI get_token(char * str)
{
	int i;
	int idx = index;

	token = lexError;
	prevIndex = index;
	while(str[idx] == ' ' || str[idx] == '\t')
		idx++;
	if(is_data(str + idx))
		if(get_data(str, &idx, termList[currentTerm].data))
			token = lexData;
		else {
			token = lexError;
			SLibError = SLERR_INVDATA;
		}
	else {
		for(i = 0; i < numLexArrayItems; i++)
			if(!memcmp(str+idx, lexAssoc[i].sym, lexAssoc[i].len)) {
				idx  += lexAssoc[i].len;
				token = lexAssoc[i].lex;
				break;
			}
		if(token == lexError)
			SLibError = SLERR_INVSYMBOL;
	}
	index = idx;
	return token;
}

static int SLAPI is_compare_sign(void)
{
	switch(token) {
		case lexGT: return _GT_;
		case lexGE: return _GE_;
		case lexLT: return _LT_;
		case lexLE: return _LE_;
		case lexEQ: return _EQ_;
		case lexNE: return _NE_;
		default: break;
	}
	return 0;
}

static int SLAPI alloc_list(void)
{
	int    ok = 1;
	currentTerm   = 0;
	numberOfTerms = LIST_DELTA;
	termList      = (CompareTerm *)SAlloc::C(numberOfTerms, sizeof(CompareTerm));
	if(!termList) {
		numberOfTerms = 0;
		ok = 0;
	}
	return ok;
}

static int SLAPI inc_list()
{
	int    ok = 1;
	int    itemSize;
	if(++currentTerm >= numberOfTerms) {
		itemSize = sizeof(CompareTerm);
		termList = (CompareTerm *)SAlloc::R(termList, (numberOfTerms + LIST_DELTA) * itemSize);
		if(termList == NULL) {
			currentTerm = 0;
			numberOfTerms = 0;
			ok = 0;
		}
		else {
			memzero(termList + numberOfTerms, LIST_DELTA * itemSize);
			numberOfTerms += LIST_DELTA;
		}
	}
	return ok;
}

static int SLAPI get_term(char * str)
{
	int ok = 0;
	int sign = is_compare_sign();
	if(sign || token == lexDoubleDot)
		if(get_token(str) == lexData) {
			termList[currentTerm].cmp = (sign ? sign : _LE_);
			if(inc_list()) {
				get_token(str);
				ok = 1;
			}
		}
		else {
			if(SLibError == SLERR_SUCCESS)
				SLibError = SLERR_DATAEXPECTED;
		}
	else
		if(token == lexData)
			if(get_token(str) == lexDoubleDot) {
				termList[currentTerm].cmp = _GE_;
				if(inc_list())
					if(get_token(str) == lexData) {
						termList[currentTerm - 1].link = _AND_;
						termList[currentTerm].cmp = _LE_;
						if(inc_list()) {
							get_token(str);
							ok = 1;
						}
					}
					else
						ok = 1;
			}
			else {
				termList[currentTerm].cmp = _EQ_;
				ok = inc_list();
			}
		else
			if(SLibError == SLERR_SUCCESS)
				SLibError = SLERR_TERMEXPECTED;
	return ok;
}

static int SLAPI get_term_list(char * str)
{
	int    ok = 0;
	if(get_term(str))
		if(token == lexEOL)
			ok = 1;
		else {
			if(token == lexDelim)
				do {
					termList[currentTerm - 1].link = _OR_;
					get_token(str);
					ok = get_term(str);
				} while(ok && token == lexDelim);
			else
				SLibError = SLERR_EOLEXPECTED;
			if(ok && token != lexEOL) {
				SLibError = SLERR_EOLEXPECTED;
				ok = 0;
			}
		}
	if(currentTerm)
		termList[currentTerm - 1].link = _END_;
	return ok;
}

static int SLAPI get_expression(Pchar str)
{
	int ok = 0;
	SLibError = 0;
	if(get_token(str) == lexEOL)
		ok = 1;
	else
		if(token == lexAsterisk) {
			termList[currentTerm].cmp = _ALL_;
			termList[currentTerm].link = _END_;
			if(inc_list())
				if(get_token(str) == lexEOL)
					ok = 1;
				else
					if(SLibError == SLERR_SUCCESS)
						SLibError = SLERR_EOLEXPECTED;
		}
		else
			ok = get_term_list(str);
	return ok;
}

int SLAPI parse_cmp_expression(char * str, int * pNumItems, CompareTerm * vect)
{
	int    ok = 0;
	index = prevIndex = SLibError = token = 0;
	if(alloc_list()) {
		if(get_expression(str)) {
			if(*pNumItems < currentTerm)
				SLibError = SLERR_BUFTOOSMALL;
			else
				ok = 1;
			if(*pNumItems > 0) {
				*pNumItems = MIN(*pNumItems, currentTerm);
				memcpy(vect, termList, *pNumItems * sizeof(CompareTerm));
			}
		}
		SAlloc::F(termList);
	}
	termList = NULL;
	return ok;
}

int SLAPI check_cmp_expression(char * pStr, IntRange * pErrLoc)
{
	int    ok = 1;
	int    numItems = 0;
	parse_cmp_expression(pStr, &numItems, 0);
	ok = (SLibError == SLERR_SUCCESS || SLibError == SLERR_BUFTOOSMALL);
	if(!ok) {
		pErrLoc->low = prevIndex;
		pErrLoc->upp = index;
	}
	return ok;
}

int SLAPI get_cmp_parse_error()
{
	return SLibError;
}

const char * SLAPI getcmpsign(int cmp, int set)
{
	static struct {
		int cmp;
		const char * str[3];
	} cmpsets[18] = {
		{_ALL_, {"", "", ""}},
		{_EQ_, {"EQ", "=", "=="}},
		{_GT_, {"GT", ">", ">"}},
		{_LT_, {"LT", "<", "<"}},
		{_NE_, {"NE", "!=", "!="}},
		{_GE_, {"GE", ">=", ">="}},
		{_LE_, {"LE", "<=", "<="}},
		{_IN_, {"", "IN", ""}},
		{_NIN_, {"", "NOT IN", ""}},
		{_BETWEEN_, {"", "BETWEEN", ""}},
		{_NBETWEEN_, {"", "NOT BETWEEN", ""}},
		{_BEGWITH_, {"", "BEGIN WITH", ""}},
		{_CONTAINS_, {"", "CONTAINS", ""}},
		{_NCONTAINS_, {"", "NOT CONTAINS", ""}},
		{_LIKE_, {"", "LIKE", ""}},
		{_NLIKE_, {"", "NOT LIKE", ""}},
		{_EXISTS_, {"", "EXISTS", ""}},
		{_NEXISTS_, {"", "NOT EXISTS", ""}}
	};
	int i;
	if(set >= 0 && set < 3)
		for(i = 0; i < 18; i++)
			if(cmpsets[i].cmp == cmp)
				return cmpsets[i].str[set];
	return 0;
}

const char * SLAPI getboolsign(int link, int set)
{
	static struct {
		int link;
		const char * str[2];
	} boolsets[4] = {
		{_END_, {"", ""}},
		{_AND_, {"AND", "&&"}},
		{_OR_, {"OR", "||"}},
		{_NOT_, {"NOT", "!"}}
	};
	int i;
	if(set >= 0 && set < 2)
		for(i = 0; i < 4; i++)
			if(boolsets[i].link == link)
				return boolsets[i].str[set];
	return 0;
}

#ifdef TEST_PARSER

void main(void)
{
	int i;
	int numItems = 12;
	pCompareTerm vect = (pCompareTerm)SAlloc::M(numItems * sizeof(CompareTerm));
	if(vect) {
		if(!parse_cmp_expression("1,4,6,9,,15..19,30,56..67,100..110",
			&numItems, vect))
			printf("Error %d\n", SLibError);
		else {
			for(i = 0; i < numItems; i++) {
				printf("XXX %s %Lf  %s\n", getcmpsign(vect[i].cmp, CMPSS_SQL),
					*(LDBL *)vect[i].data, getboolsign(vect[i].link, BOOLSS_SQL));
			}
		}
		farfree(vect);
	}
}

#endif /* TEST_PARSER */
