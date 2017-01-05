/* GNUPLOT - scanner.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/
#include <gnuplot.h>
#pragma hdrstop

int curly_brace_count;

//static int get_num(char str[]);
//static void substitute(char ** strp, size_t *str_lenp, int current);

#ifdef VMS
#include <descrip.h>
#define MAILBOX "PLOT$MAILBOX"
#ifdef __DECC
#include <lib$routines.h>       /* avoid some IMPLICITFNC warnings */
#include <starlet.h>
#endif /* __DECC */
#endif /* VMS */

#define isident(c) (isalnum((uchar)(c)) || (c) == '_' || ALLOWED_8BITVAR(c))

#define LBRACE '{'
#define RBRACE '}'
//#define APPEND_TOKEN { GpC.P_Token[t_num].length++; current++; }

static int t_num; // number of token I'm working on 

bool legal_identifier(char * p)
{
	if(!p || !(*p) || isdigit((uchar)*p))
		return false;
	while(*p) {
		if(!isident(*p))
			return false;
		p++;
	}
	return true;
}

/*
 * scanner() breaks expression[] into lexical units, storing them in token[].
 *   The total number of tokens found is returned as the function
 *   value.  Scanning will stop when '\0' is found in expression[], or
 *   when token[] is full.  extend_input_line() is called to extend
 *   expression array if needed.
 *
 *       Scanning is performed by following rules:
 *
 *      Current char    token should contain
 *     -------------    -----------------------
 *      1.  alpha,_     all following alpha-numerics
 *      2.  digit       0 or more following digits, 0 or 1 decimal point,
 *                              0 or more digits, 0 or 1 'e' or 'E',
 *                              0 or more digits.
 *      3.  ^,+,-,/     only current char
 *          %,~,(,)
 *          [,],;,:,
 *          ?,comma
 *          $
 *      4.  &,|,=,*     current char; also next if next is same
 *      5.  !,<,>       current char; also next if next is =
 *      6.  ", '        all chars up until matching quote
 *      7.  #           this token cuts off scanning of the line (DFK).
 *      8.  `           (command substitution: all characters through the
 *                      matching backtic are replaced by the output of
 *                      the contained command, then scanning is restarted.)
 * EAM Jan 2010:	Bugfix. No rule covered an initial period. This caused
 *			string concatenation to fail for variables whose first
 *			character is 'E' or 'e'.  Now we add a 9th rule:
 *	9.  .		A period may be a token by itself (string concatenation)
 *			or the start of a decimal number continuing with a digit
 *
 *                      white space between tokens is ignored
 */
//int scanner(char ** expressionp, size_t * expressionlenp)
int GpCommand::Scanner(char ** expressionp, size_t * expressionlenp)
{
	int    current;    // index of current char in expression[] 
	char * expression = *expressionp;
	int    quote;
	char   brace;
	curly_brace_count = 0;
	for(current = t_num = 0; expression[current] != NUL; current++) {
		if(t_num + 1 >= TokenTableSize) {
			ExtendTokenTable(); // leave space for dummy end token 
		}
		if(!isspace((uchar)expression[current])) { // skip the whitespace 
			P_Token[t_num].start_index = current;
			P_Token[t_num].length = 1;
			P_Token[t_num].is_token = true; // to start with... 
			if(expression[current] == '`') {
				Substitute(expressionp, expressionlenp, current);
				expression = *expressionp; // expression might have moved 
				current--;
			}
			else {
				// allow _ to be the first character of an identifier 
				// allow 8bit characters in identifiers 
				if(isalpha((uchar)expression[current]) || (expression[current] == '_') || ALLOWED_8BITVAR(expression[current])) {
					while(isident(expression[current + 1])) { 
						P_Token[t_num].length++; current++; 
					}
				}
				else if(isdigit((uchar)expression[current])) {
					P_Token[t_num].is_token = false;
					P_Token[t_num].length = GetNum(&expression[current]);
					current += (P_Token[t_num].length - 1);
				}
				else if(expression[current] == '.') {
					// Rule 9
					if(isdigit((uchar)expression[current+1])) {
						P_Token[t_num].is_token = false;
						P_Token[t_num].length = GetNum(&expression[current]);
						current += (P_Token[t_num].length - 1);
					} // do nothing if the . is a token by itself 
				}
				else if(expression[current] == LBRACE) {
					int partial;
					P_Token[t_num].is_token = false;
					P_Token[t_num].l_val.type = CMPLX;
					partial = sscanf(&expression[++current], "%lf , %lf %c", &P_Token[t_num].l_val.v.cmplx_val.real, &P_Token[t_num].l_val.v.cmplx_val.imag, &brace);
					if(partial <= 0) {
						curly_brace_count++;
						P_Token[t_num++].is_token = true;
						current--;
						continue;
					}
					if(partial != 3 || brace != RBRACE)
						GpGg.IntError(GpC, t_num, "invalid complex constant");
					P_Token[t_num].length += 2;
					while(expression[++current] != RBRACE) {
						P_Token[t_num].length++;
						if(expression[current] == NUL) /* { for vi % */
							GpGg.IntError(GpC, t_num, "no matching '}'");
					}
				}
				else if(expression[current] == '\'' || expression[current] == '\"') {
					P_Token[t_num].length++;
					quote = expression[current];
					while(expression[++current] != quote) {
						if(!expression[current]) {
							expression[current] = quote;
							expression[current + 1] = NUL;
							break;
						}
						else if(quote == '\"' && expression[current] == '\\' && expression[current + 1]) {
							current++;
							P_Token[t_num].length += 2;
						}
						else if(quote == '\"' && expression[current] == '`') {
							Substitute(expressionp, expressionlenp, current);
							expression = *expressionp; /* it might have moved */
							current--;
						}
						else if(quote == '\'' && expression[current+1] == '\'' && expression[current+2] == '\'') {
							// look ahead: two subsequent single quotes -> take them in
							current += 2;
							P_Token[t_num].length += 3;
						}
						else
							P_Token[t_num].length++;
					}
				}
				else {
					switch(expression[current]) {
						case '#':
		#ifdef OLD_STYLE_CALL_ARGS
							/* FIXME: This ugly exception handles the old-style syntatic  */
							/* entity $# (number of arguments in "call" statement), which */
							/* otherwise would be treated as introducing a comment.       */
							if((t_num == 0) || (P_InputLine[P_Token[t_num-1].start_index] != '$'))
		#endif
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
						case '}': /* complex constants will not end up here */
							curly_brace_count--;
							break;
						case '&':
						case '|':
						case '=':
						case '*':
							if(expression[current] == expression[current + 1]) {
								P_Token[t_num].length++; current++; 
							}
							break;
						case '!':
						case '>':
							if(expression[current + 1] == '=') {
								P_Token[t_num].length++; current++;
							}
							if(expression[current + 1] == '>') {
								P_Token[t_num].length++; current++; 
							}
							break;
						case '<':
							if(expression[current + 1] == '=') {
								P_Token[t_num].length++; current++; 
							}
							if(expression[current + 1] == '<') {
								P_Token[t_num].length++; current++; 
							}
							break;
						default:
							GpGg.IntError(GpC, t_num, "invalid character %c", expression[current]);
					}
				}
				++t_num; // next token if not white space 
			}
		}
	}
endline: // comments jump here to ignore line
	// Now kludge an extra token which points to '\0' at end of expression[].
	// This is useful so printerror() looks nice even if we've fallen off the line
	P_Token[t_num].start_index = current;
	P_Token[t_num].length = 0;
	// print 3+4  then print 3+  is accepted without this, since string is ignored if it is not a token
	P_Token[t_num].is_token = true;
	return (t_num);
}

//static int get_num(char str[])
int GpCommand::GetNum(char str[])
{
	int count = 0;
	P_Token[t_num].is_token = false;
	P_Token[t_num].l_val.type = INTGR; /* assume unless . or E found */
	while(isdigit((uchar)str[count]))
		count++;
	if(str[count] == '.') {
		P_Token[t_num].l_val.type = CMPLX;
		// swallow up digits until non-digit
		while(isdigit((uchar)str[++count])) ;
		// now str[count] is other than a digit 
	}
	if(str[count] == 'e' || str[count] == 'E') {
		P_Token[t_num].l_val.type = CMPLX;
		count++;
		if(str[count] == '-' || str[count] == '+')
			count++;
		if(!isdigit((uchar)str[count])) {
			P_Token[t_num].start_index += count;
			GpGg.IntError(GpC, t_num, "expecting exponent");
		} 
		while(isdigit((uchar)str[++count]))
			;
	}
	if(P_Token[t_num].l_val.type == INTGR) {
		char * endptr;
		int64 lval = _strtoi64(str, &endptr, 0); // @sobolev strtoll-->strtoi64
		errno = 0;
		if(!errno) {
			count = endptr - str;
			/* Linux and Windows implementations of POSIX function strtoll() differ.*/
			/* I don't know which is "correct" but returning 0 causes an infinite   */
			/* loop on input "0x" as the scanner fails to progress.                 */
			if(count == 0) count++;
			if((P_Token[t_num].l_val.v.int_val = (int)lval) == lval)
				return(count);
			if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
				if((ulong)(P_Token[t_num].l_val.v.int_val & 0xffffffff) == lval)
					return(count);
			}
		}
		int_warn(t_num, "integer overflow; changing to floating point");
		P_Token[t_num].l_val.type = CMPLX;
		// Fall through
	}
	P_Token[t_num].l_val.v.cmplx_val.imag = 0.0;
	P_Token[t_num].l_val.v.cmplx_val.real = atof(str);
	return (count);
}

/* substitute output from ` `
 * *strp points to the input string.  (*strp)[current] is expected to
 * be the initial back tic.  Characters through the following back tic
 * are replaced by the output of the command.  extend_input_line()
 * is called to extend *strp array if needed.
 */
//static void substitute(char ** strp, size_t * str_lenp, int current)
void GpCommand::Substitute(char ** ppStr, size_t * pStrLen, int curChrP)
{
	char   c;
	char * pgm, * rest = NULL;
	char * output;
	size_t pgm_len, rest_len = 0;
	int    output_pos;
	// forgive missing closing backquote at end of line 
	char * str = *ppStr + curChrP;
	char * last = str;
	while(*++last) {
		if(*last == '`')
			break;
	}
	pgm_len = last - str;
	pgm = (char *)malloc(pgm_len);
	strnzcpy(pgm, str + 1, pgm_len); /* omit ` to leave room for NUL */
	// save rest of line, if any 
	if(*last) {
		last++; // advance past `
		rest_len = strlen(last) + 1;
		if(rest_len > 1) {
			rest = (char *)malloc(rest_len);
			strcpy(rest, last);
		}
	}
	// do system call 
	(void)do_system_func(pgm, &output);
	free(pgm);
	// now replace ` ` with output 
	output_pos = 0;
	while((c = output[output_pos++])) {
		if(c != '\n' && c != '\r')
			(*ppStr)[curChrP++] = c;
		if(curChrP == *pStrLen)
			ExtendInputLine();
	}
	(*ppStr)[curChrP] = 0;
	free(output);
	// tack on rest of line to output 
	if(rest) {
		while(curChrP + rest_len > *pStrLen)
			ExtendInputLine();
		strcpy(*ppStr + curChrP, rest);
		free(rest);
	}
	GpGg.screen_ok = false;
}

