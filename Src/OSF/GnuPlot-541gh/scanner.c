// GNUPLOT - scanner.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

//static void substitute(char ** strp, size_t * str_lenp, int current);

#define isident(c) (isalnum((uchar)(c)) || (c) == '_' || ALLOWED_8BITVAR(c))

#define LBRACE '{'
#define RBRACE '}'

//int curly_brace_count;
//static int __TNum; // @global number of token I'm working on 

bool legal_identifier(char * p)
{
	if(!p || !(*p) || isdigit((uchar)*p))
		return FALSE;
	while(*p) {
		if(!isident(*p))
			return FALSE;
		p++;
	}
	return TRUE;
}
/*
 * scanner() breaks expression[] into lexical units, storing them in token[].
 *   The total number of tokens found is returned as the function
 *   value.  Scanning will stop when '\0' is found in expression[], or
 *   when token[] is full.  ExtendInputLine() is called to extend
 *   expression array if needed.
 *
 * Scanning is performed by following rules:
 *
 *      Current char    token should contain
 *     -------------    -----------------------
 *      1.  alpha,_     all following alpha-numerics
 *      2.  digit       0 or more following digits, 0 or 1 decimal point,
 *                        0 or more digits, 0 or 1 'e' or 'E',
 *                        0 or more digits.
 *      3.  ^,+,-,/     only current char
 *    %,~,(,)
 *    [,],;,:,
 *    ?,comma
 *    $
 *      4.  &,|,=,*     current char; also next if next is same
 *      5.  !,<,>       current char; also next if next is =
 *      6.  ", '        all chars up until matching quote
 *      7.  #           this token cuts off scanning of the line (DFK).
 *      8.  `           (command substitution: all characters through the
 *                matching backtic are replaced by the output of
 *                the contained command, then scanning is restarted.)
 * EAM Jan 2010:	Bugfix. No rule covered an initial period. This caused
 *			string concatenation to fail for variables whose first
 *			character is 'E' or 'e'.  Now we add a 9th rule:
 *	9.  .		A period may be a token by itself (string concatenation)
 *			or the start of a decimal number continuing with a digit
 *
 *                white space between tokens is ignored
 */
//int scanner(char ** expressionp, size_t * expressionlenp)
int GnuPlot::Scanner(char ** ppExpression, size_t * pExpressionLen)
{
#define APPEND_TOKEN { Pgm.Tok().Len++; current++;}
	int current; // index of current char in expression[] 
	char * p_expression = *ppExpression;
	int quote;
	char brace;
	Pgm.CurlyBraceCount = 0;
	for(current = Pgm.__TNum = 0; p_expression[current]; current++) {
		if((Pgm.__TNum+1) >= Pgm.TokenTableSize) {
			Pgm.ExtendTokenTable(); // leave space for dummy end token 
		}
		if(isspace((uchar)p_expression[current]))
			continue; // skip the whitespace 
		Pgm.Tok().StartIdx = current;
		Pgm.Tok().Len = 1;
		Pgm.Tok().IsToken = TRUE; /* to start with... */
		if(p_expression[current] == '`') {
			Substitute(ppExpression, pExpressionLen, current);
			p_expression = *ppExpression; // expression might have moved 
			current--;
			continue;
		}
		// allow _ to be the first character of an identifier 
		// allow 8bit characters in identifiers 
		if(isalpha((uchar)p_expression[current]) || (p_expression[current] == '_') || ALLOWED_8BITVAR(p_expression[current])) {
			while(isident(p_expression[current+1]))
				APPEND_TOKEN;
		}
		else if(isdigit((uchar)p_expression[current])) {
			Pgm.Tok().IsToken = FALSE;
			Pgm.Tok().Len = GetNum(&p_expression[current]);
			current += (Pgm.Tok().Len - 1);
		}
		else if(p_expression[current] == '.') {
			// Rule 9 
			if(isdigit((uchar)p_expression[current+1])) {
				Pgm.Tok().IsToken = FALSE;
				Pgm.Tok().Len = GetNum(&p_expression[current]);
				current += (Pgm.Tok().Len - 1);
			} // do nothing if the . is a token by itself 
		}
		else if(p_expression[current] == LBRACE) {
			int partial;
			Pgm.Tok().IsToken = FALSE;
			Pgm.Tok().LVal.Type = CMPLX;
			partial = sscanf(&p_expression[++current], "%lf , %lf %c", &Pgm.Tok().LVal.v.cmplx_val.real, &Pgm.Tok().LVal.v.cmplx_val.imag, &brace);
			if(partial <= 0) {
				Pgm.CurlyBraceCount++;
				Pgm.P_Token[Pgm.__TNum++].IsToken = TRUE;
				current--;
				continue;
			}
			if(partial != 3 || brace != RBRACE)
				IntError(Pgm.__TNum, "invalid complex constant");
			Pgm.Tok().Len += 2;
			while(p_expression[++current] != RBRACE) {
				Pgm.Tok().Len++;
				if(!p_expression[current]) // { for vi % 
					IntError(Pgm.__TNum, "no matching '}'");
			}
		}
		else if(p_expression[current] == '\'' || p_expression[current] == '\"') {
			Pgm.Tok().Len++;
			quote = p_expression[current];
			while(p_expression[++current] != quote) {
				if(!p_expression[current]) {
					p_expression[current] = quote;
					p_expression[current+1] = '\0';
					break;
				}
				else if(quote == '\"' && p_expression[current] == '\\' && p_expression[current+1]) {
					current++;
					Pgm.Tok().Len += 2;
				}
				else if(quote == '\"' && p_expression[current] == '`') {
					Substitute(ppExpression, pExpressionLen, current);
					p_expression = *ppExpression; /* it might have moved */
					current--;
				}
				else if(quote == '\'' && p_expression[current+1] == '\'' && p_expression[current+2] == '\'') {
					// look ahead: two subsequent single quotes -> take them in
					current += 2;
					Pgm.Tok().Len += 3;
				}
				else
					Pgm.Tok().Len++;
			}
		}
		else
			switch(p_expression[current]) {
				case '#':
				    goto endline; /* ignore the rest of the line */
				case '^':
				case '+':
				case '-':
				case '/':
				case '%':
				case '~':
				case '(':
				case ')':
				case '[':
				case ']':
				case ';':
				case ':':
				case '?':
				case ',':
				case '$':
				    break;
				case '}': // complex constants will not end up here 
				    Pgm.CurlyBraceCount--;
				    break;
				case '&':
				case '|':
				case '=':
				case '*':
				    if(p_expression[current] == p_expression[current+1])
					    APPEND_TOKEN;
				    break;
				case '!':
				case '>':
				    if(p_expression[current+1] == '=')
					    APPEND_TOKEN;
				    if(p_expression[current+1] == '>')
					    APPEND_TOKEN;
				    break;
				case '<':
				    if(p_expression[current+1] == '=')
					    APPEND_TOKEN;
				    if(p_expression[current+1] == '<')
					    APPEND_TOKEN;
				    break;
				default:
				    IntError(Pgm.__TNum, "invalid character %c", p_expression[current]);
			}
		++Pgm.__TNum; // next token if not white space 
	}
endline:                        /* comments jump here to ignore line */
	// Now kludge an extra token which points to '\0' at end of expression[].
	// This is useful so printerror() looks nice even if we've fallen off the line. 
	Pgm.Tok().StartIdx = current;
	Pgm.Tok().Len = 0;
	// print 3+4  then print 3+  is accepted without
	// this, since string is ignored if it is not a token
	Pgm.Tok().IsToken = TRUE;
	return Pgm.__TNum;
#undef APPEND_TOKEN 
}

//static int get_num(char str[])
int FASTCALL GnuPlot::GetNum(char pStr[])
{
	int count = 0;
	char * endptr;
	Pgm.Tok().IsToken = FALSE;
	Pgm.Tok().LVal.Type = INTGR; /* assume unless . or E found */
	while(isdigit((uchar)pStr[count]))
		count++;
	if(pStr[count] == '.') {
		Pgm.Tok().LVal.Type = CMPLX;
		// swallow up digits until non-digit 
		while(isdigit((uchar)pStr[++count]))
			;
		// now str[count] is other than a digit 
	}
	if(pStr[count] == 'e' || pStr[count] == 'E') {
		Pgm.Tok().LVal.Type = CMPLX;
		count++;
		if(pStr[count] == '-' || pStr[count] == '+')
			count++;
		if(!isdigit((uchar)pStr[count])) {
			Pgm.Tok().StartIdx += count;
			IntError(Pgm.__TNum, "expecting exponent");
		} 
		while(isdigit((uchar)pStr[++count]))
			;
	}
	if(Pgm.Tok().LVal.Type == INTGR) {
		errno = 0;
		int64 lval = strtoll(pStr, &endptr, 0);
		if(!errno) {
			count = endptr - pStr;
			// Linux and Windows implementations of POSIX function strtoll() differ.
			// I don't know which is "correct" but returning 0 causes an infinite   
			// loop on input "0x" as the scanner fails to progress.                 
			if(count == 0) 
				count++;
			if((Pgm.Tok().LVal.v.int_val = static_cast<intgr_t>(lval)) == lval)
				return(count);
			if(pStr[0] == '0' && (pStr[1] == 'x' || pStr[1] == 'X')) {
				if((uint64)lval == (uint64)Pgm.Tok().LVal.v.int_val)
					return(count);
			}
		}
		IntWarn(Pgm.__TNum, "integer overflow; changing to floating point");
		Pgm.Tok().LVal.Type = CMPLX;
		// @fallthrough 
	}
	Pgm.Tok().LVal.v.cmplx_val.imag = 0.0;
	Pgm.Tok().LVal.v.cmplx_val.real = strtod(pStr, &endptr);
	count = endptr - pStr;
	return (count);
}
// 
// substitute output from ` `
// *strp points to the input string.  (*strp)[current] is expected to
// be the initial back tic.  Characters through the following back tic
// are replaced by the output of the command.  extend_input_line()
// is called to extend *strp array if needed.
// 
//static void substitute(char ** ppStr, size_t * pStrLen, int current)
void GnuPlot::Substitute(char ** ppStr, size_t * pStrLen, int current)
{
	char c;
	char * pgm, * rest = NULL;
	char * output;
	size_t pgm_len, rest_len = 0;
	int output_pos;
	// forgive missing closing backquote at end of line 
	char * str = *ppStr + current;
	char * last = str;
	while(*++last) {
		if(*last == '`')
			break;
	}
	pgm_len = last - str;
	pgm = (char *)SAlloc::M(pgm_len);
	strnzcpy(pgm, str + 1, pgm_len); // omit ` to leave room for NUL 
	// save rest of line, if any 
	if(*last) {
		last++; // advance past `
		rest_len = strlen(last) + 1;
		if(rest_len > 1) {
			rest = (char *)SAlloc::M(rest_len);
			strcpy(rest, last);
		}
	}
	// do system call 
	DoSystemFunc(pgm, &output);
	SAlloc::F(pgm);
	// now replace ` ` with output 
	output_pos = 0;
	while((c = output[output_pos++])) {
		if(output[output_pos] != '\0' || c != '\n')
			(*ppStr)[current++] = c;
		if(current == *pStrLen)
			ExtendInputLine();
	}
	(*ppStr)[current] = 0;
	SAlloc::F(output);
	// tack on rest of line to output
	if(rest) {
		while(current + rest_len > *pStrLen)
			ExtendInputLine();
		strcpy(*ppStr + current, rest);
		SAlloc::F(rest);
	}
	GpU.screen_ok = FALSE;
}
