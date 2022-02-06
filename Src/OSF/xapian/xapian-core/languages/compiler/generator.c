//
//
#include <slib.h>
#include "header.h"
#pragma hdrstop

/* Define this to get warning messages when optimisations can't be used. */
/* #define OPTIMISATION_WARNINGS */

/* recursive use: */

static void generate(Generator * g, const Node * p);

/* Write routines for simple entities */

/* Write a space if the preceding character was not whitespace */
static void ws_opt_space(Generator * g, const char * s) 
{
	int ch = str_back(g->outbuf);
	if(ch != ' ' && ch != '\n' && ch != '\t' && ch != -1)
		write_char(g, ' ');
	write_string(g, s);
}

static void wi3(Generator * g, int i) 
{
	if(i < 100) 
		write_char(g, ' ');
	if(i < 10) 
		write_char(g, ' ');
	write_int(g, i); /* integer (width 3) */
}
//
// Write routines for items from the syntax tree 
//
static void write_varname(Generator * g, const Name * p) 
{
	int ch = "SIIrxg"[p->type];
	switch(p->type) {
		case t_external:
		    write_string(g, g->P_Options->externals_prefix); break;
		case t_string:
		case t_boolean:
		case t_integer: {
		    if(g->P_Options->make_lang != Options::LANG_C) 
				goto use_name;
		    int count = p->count;
		    if(count < 0) {
			    slfprintf_stderr("Reference to optimised out variable ");
			    report_b(stderr, p->b);
			    slfprintf_stderr(" attempted\n");
			    exit(1);
		    }
		    if(p->type == t_boolean) {
			    // We use a single array for booleans and integers, with the integers first.
			    count += g->analyser->NameCount[t_integer];
		    }
		    write_char(g, ch);
		    write_char(g, '[');
		    write_int(g, count);
		    write_char(g, ']');
		    return;
	    }
		default:
use_name:
		    write_char(g, ch); 
			write_char(g, '_');
	}
	write_b(g, p->b);
}

static void write_varref(Generator * g, const Name * p) 
{  /* reference to variable */
	if(g->P_Options->make_lang == Options::LANG_C && p->type < t_routine)
		write_string(g, "z->");
	write_varname(g, p);
}

static void write_hexdigit(Generator * g, int i) 
{
	str_append_ch(g->outbuf, "0123456789ABCDEF"[i & 0xF]); /* hexchar */
}

static void write_hex(Generator * g, int i) 
{
	if(i >> 4) 
		write_hex(g, i >> 4);
	write_hexdigit(g, i); /* hex integer */
}

/* write character literal */
static void wlitch(Generator * g, int ch) 
{
	if(32 <= ch && ch < 127) {
		write_char(g, '\'');
		if(ch == '\'' || ch == '\\') {
			write_char(g, '\\');
		}
		write_char(g, ch);
		write_char(g, '\'');
	}
	else {
		write_string(g, "0x"); write_hex(g, ch);
	}
}

static void wlitarray(Generator * g, const symbol * p) // write literal array 
{
	for(int i = 0; i < SIZE(p); i++) {
		wlitch(g, p[i]);
		if(i < SIZE(p)-1) 
			write_string(g, ", ");
	}
}

static void wlitref(Generator * g, const symbol * p) // write ref to literal array 
{
	if(SIZE(p) == 0) {
		write_char(g, '0');
	}
	else {
		struct str * s = g->outbuf;
		g->outbuf = g->declarations;
		write_string(g, "static const symbol s_"); 
		write_int(g, g->literalstring_count); 
		write_string(g, "[] = { ");
		wlitarray(g, p);
		write_string(g, " };\n");
		g->outbuf = s;
		write_string(g, "s_"); 
		write_int(g, g->literalstring_count);
		g->literalstring_count++;
	}
}

static void write_margin(Generator * g) 
{
	for(int i = 0; i < g->margin; i++) 
		write_string(g, "\t");
}

void write_comment_content(Generator * g, const Node * p) 
{
	switch(p->type) {
		case c_mathassign:
		case c_plusassign:
		case c_minusassign:
		case c_multiplyassign:
		case c_divideassign:
		    if(p->name) {
			    write_char(g, '$');
			    write_b(g, p->name->b);
			    write_char(g, ' ');
		    }
		    write_string(g, name_of_token(p->type));
		    write_string(g, " <integer expression>");
		    break;
		case c_eq:
		case c_ne:
		case c_gr:
		case c_ge:
		case c_ls:
		case c_le:
		    write_string(g, "$(<integer expression> ");
		    write_string(g, name_of_token(p->type));
		    write_string(g, " <integer expression>)");
		    break;
		default:
		    write_string(g, name_of_token(p->type));
		    if(p->name) {
			    write_char(g, ' ');
			    write_b(g, p->name->b);
		    }
	}
	write_string(g, ", line ");
	write_int(g, p->line_number);
}

static void write_comment(Generator * g, const Node * p) 
{
	if(g->P_Options->comments) {
		ws_opt_space(g, "/* ");
		write_comment_content(g, p);
		write_string(g, " */");
	}
	write_newline(g);
}

static void wms(Generator * g, const char * s) 
{
	write_margin(g); write_string(g, s);
} /* margin + string */

static void write_block_start(Generator * g)  /* block start */
{ 
	wms(g, "{   ");
	g->margin++;
}

static void write_block_end(Generator * g) /* block end */
{
	if(g->line_labelled == g->line_count) {
		wms(g, ";"); 
		write_newline(g);
	}
	g->margin--;
	wms(g, "}"); write_newline(g);
}

static void w(Generator * g, const char * s);

/* keep c */
static void wk(Generator * g, const Node * p, int keep_limit) 
{
	++g->keep_count;
	if(p->mode == m_forward) {
		write_string(g, "int c");
		write_int(g, g->keep_count);
		w(g, " = ~zc");
		if(keep_limit) {
			write_string(g, ", mlimit");
			write_int(g, g->keep_count);
		}
		write_char(g, ';');
	}
	else {
		write_string(g, "int m");
		write_int(g, g->keep_count);
		w(g, " = ~zl - ~zc");
		if(keep_limit) {
			write_string(g, ", mlimit");
			write_int(g, g->keep_count);
		}
		write_string(g, "; (void)m");
		write_int(g, g->keep_count);
		write_char(g, ';');
	}
}

static void wrestore(Generator * g, const Node * p, int keep_token) // restore c 
{
	if(p->mode == m_forward) {
		w(g, "~zc = c");
	}
	else {
		w(g, "~zc = ~zl - m");
	}
	write_int(g, keep_token); 
	write_char(g, ';');
}

static void wrestorelimit(Generator * g, const Node * p, int keep_token) // restore limit 
{
	if(p->mode == m_forward) {
		w(g, "~zl += mlimit");
	}
	else {
		w(g, "~zlb = mlimit");
	}
	write_int(g, keep_token); write_string(g, ";");
}

static void winc(Generator * g, const Node * p) // increment c 
{
	w(g, p->mode == m_forward ? "~zc++;" : "~zc--;");
}

static void wsetl(Generator * g, int n) 
{
	g->margin--;
	wms(g, "lab"); 
	write_int(g, n); 
	write_char(g, ':'); 
	write_newline(g);
	g->line_labelled = g->line_count;
	g->margin++;
}

static void wgotol(Generator * g, int n) 
{
	wms(g, "goto lab"); 
	write_int(g, n); 
	write_char(g, ';'); 
	write_newline(g);
}

static void write_failure(Generator * g, const Node * p) // fail
{
	if(g->failure_keep_count != 0) {
		write_string(g, "{ ");
		if(g->failure_keep_count > 0) {
			wrestore(g, p, g->failure_keep_count);
		}
		else {
			wrestorelimit(g, p, -g->failure_keep_count);
		}
		write_char(g, ' ');
	}
	switch(g->failure_label) {
		case x_return:
		    write_string(g, "return 0;");
		    break;
		default:
		    write_string(g, "goto lab");
		    write_int(g, g->failure_label);
		    write_char(g, ';');
		    g->label_used = 1;
	}
	if(g->failure_keep_count != 0) 
		write_string(g, " }");
}

// if at limit fail 
static void write_check_limit(Generator * g, const Node * p) 
{
	w(g, p->mode == m_forward ? "if(~zc >= ~zl) " : "if(~zc <= ~zlb) ");
	write_failure(g, p);
}

static void write_data_address(Generator * g, const Node * p) 
{
	const symbol * b = p->P_LiteralString;
	if(b != 0) {
		write_int(g, SIZE(b)); 
		w(g, ", ");
		wlitref(g, b);
	}
	else {
		write_varref(g, p->name);
	}
}

// Formatted write. 
static void writef(Generator * g, const char * input, const Node * p) 
{
	int i = 0;
	int l = strlen(input);
	while(i < l) {
		int ch = input[i++];
		if(ch != '~') {
			write_char(g, ch);
			continue;
		}
		switch(input[i++]) {
			default: write_char(g, input[i - 1]); continue;
			case 'C': write_comment(g, p); continue;
			case 'k': wk(g, p, false); continue;
			case 'K': wk(g, p, true); continue;
			case 'i': winc(g, p); continue;
			case 'l': write_check_limit(g, p); continue;
			case 'f': write_failure(g, p); continue;
			case 'M': write_margin(g); continue;
			case 'N': write_newline(g); continue;
			case '{': write_block_start(g); continue;
			case '}': write_block_end(g); continue;
			case 'S': write_string(g, g->S[input[i++] - '0']); continue;
			case 'I': write_int(g, g->I[input[i++] - '0']); continue;
			case 'J': wi3(g, g->I[input[i++] - '0']); continue;
			case 'V': write_varref(g, g->V[input[i++] - '0']); continue;
			case 'W': write_varname(g, g->V[input[i++] - '0']); continue;
			case 'L': wlitref(g, g->L[input[i++] - '0']); continue;
			case 'A': wlitarray(g, g->L[input[i++] - '0']); continue;
			case 'c': wlitch(g, g->I[input[i++] - '0']); continue;
			case 'a': write_data_address(g, p); continue;
			case '+': g->margin++; continue;
			case '-': g->margin--; continue;
			case '$': /* insert_s, insert_v etc */
			    write_char(g, p->P_LiteralString == 0 ? 'v' : 's');
			    continue;
			case 'z':
			    if(g->P_Options->make_lang == Options::LANG_C)
				    write_string(g, "z->");
			    continue;
			case 'Z':
			    if(g->P_Options->make_lang == Options::LANG_C)
				    write_string(g, input[i] == ')' ? "z" : "z, ");
			    continue;
			case 'p': write_string(g, g->P_Options->externals_prefix); continue;
		}
	}
}

static void w(Generator * g, const char * s) 
{
	writef(g, s, 0);
}

static void generate_AE(Generator * g, const Node * p) 
{
	const char * s;
	switch(p->type) {
		case c_name: write_varref(g, p->name); break;
		case c_number: write_int(g, p->number); break;
		case c_maxint: write_string(g, "MAXINT"); break;
		case c_minint: write_string(g, "MININT"); break;
		case c_neg: 
			write_char(g, '-'); 
			generate_AE(g, p->P_Right); // @recursion
			break;
		case c_multiply: s = " * "; goto label0;
		case c_plus: s = " + "; goto label0;
		case c_minus: s = " - "; goto label0;
		case c_divide: 
			s = " / ";
label0:
		    write_char(g, '('); 
			generate_AE(g, p->P_Left); // @recursion
		    write_string(g, s); 
			generate_AE(g, p->P_Right); // @recursion
			write_char(g, ')'); 
			break;
		case c_cursor:
		    w(g, "~zc"); 
			break;
		case c_limit:
		    w(g, p->mode == m_forward ? "~zl" : "~zlb"); 
			break;
		case c_len:
		    if(g->P_Options->encoding == ENC_UTF8) {
			    w(g, "len_utf8(~zp)");
			    break;
		    }
		/* FALLTHRU */
		case c_size:
		    w(g, "SIZE(~zp)");
		    break;
		case c_lenof:
		    if(g->P_Options->encoding == ENC_UTF8) {
			    g->V[0] = p->name;
			    w(g, "len_utf8(~V0)");
			    break;
		    }
		/* FALLTHRU */
		case c_sizeof:
		    g->V[0] = p->name;
		    w(g, "SIZE(~V0)");
		    break;
	}
}
/* K_needed() tests to see if we really need to keep c. Not true when the
   command does not touch the cursor. This and repeat_score() could be
   elaborated almost indefinitely.
 */
static int K_needed_(Generator * g, Node * p, int call_depth) 
{
	while(p) {
		switch(p->type) {
			case c_atlimit:
			case c_do:
			case c_dollar:
			case c_leftslice:
			case c_rightslice:
			case c_mathassign:
			case c_plusassign:
			case c_minusassign:
			case c_multiplyassign:
			case c_divideassign:
			case c_eq:
			case c_ne:
			case c_gr:
			case c_ge:
			case c_ls:
			case c_le:
			case c_sliceto:
			case c_booltest:
			case c_set:
			case c_unset:
			case c_true:
			case c_false:
			case c_debug:
			    break;

			case c_call:
			    /* Recursive functions aren't typical in snowball programs, so
			     * make the pessimistic assumption that keep is needed if we
			     * hit a generous limit on recursion.  It's not likely to make
			     * a difference to any real world program, but means we won't
			     * recurse until we run out of stack for pathological cases.
			     */
			    if(call_depth >= 100) 
					return true;
			    if(K_needed_(g, p->name->definition, call_depth + 1))
				    return true;
			    break;

			case c_bra:
			    if(K_needed_(g, p->P_Left, call_depth)) 
					return true;
			    break;

			default: return true;
		}
		p = p->P_Right;
	}
	return false;
}

extern int K_needed(Generator * g, Node * p) 
{
	return K_needed_(g, p, 0);
}

static int repeat_score(Generator * g, Node * p, int call_depth) 
{
	int score = 0;
	while(p) {
		switch(p->type) {
			case c_dollar:
			case c_leftslice:
			case c_rightslice:
			case c_mathassign:
			case c_plusassign:
			case c_minusassign:
			case c_multiplyassign:
			case c_divideassign:
			case c_eq:
			case c_ne:
			case c_gr:
			case c_ge:
			case c_ls:
			case c_le:
			case c_sliceto: /* case c_not: must not be included here! */
			case c_debug:
			    break;

			case c_call:
			    /* Recursive functions aren't typical in snowball programs, so
			     * make the pessimistic assumption that repeat requires cursor
			     * reinstatement if we hit a generous limit on recursion.  It's
			     * not likely to make a difference to any real world program,
			     * but means we won't recurse until we run out of stack for
			     * pathological cases.
			     */
			    if(call_depth >= 100) {
				    return 2;
			    }
			    score += repeat_score(g, p->name->definition, call_depth + 1);
			    if(score >= 2)
				    return score;
			    break;

			case c_bra:
			    score += repeat_score(g, p->P_Left, call_depth);
			    if(score >= 2)
				    return score;
			    break;

			case c_name:
			case c_literalstring:
			case c_next:
			case c_grouping:
			case c_non:
			case c_hop:
			    if(++score >= 2)
				    return score;
			    break;

			default:
			    return 2;
		}
		p = p->P_Right;
	}
	return score;
}
//
// tests if an expression requires cursor reinstatement in a repeat 
//
extern int repeat_restore(Generator * g, Node * p) 
{
	return repeat_score(g, p, 0) >= 2;
}

static void generate_bra(Generator * g, const Node * p) 
{
	p = p->P_Left;
	while(p) {
		generate(g, p);
		p = p->P_Right;
	}
}

static void generate_and(Generator * g, const Node * p) 
{
	int keep_c = 0;
	if(K_needed(g, p->P_Left)) {
		writef(g, "~{~k~C", p);
		keep_c = g->keep_count;
	}
	else {
		writef(g, "~M~C", p);
	}
	p = p->P_Left;
	while(p) {
		generate(g, p);
		if(keep_c && p->P_Right) {
			w(g, "~M"); 
			wrestore(g, p, keep_c); 
			w(g, "~N");
		}
		p = p->P_Right;
	}
	if(keep_c) 
		w(g, "~}");
}

static void generate_or(Generator * g, const Node * p) 
{
	int keep_c = 0;
	int used = g->label_used;
	int a0 = g->failure_label;
	int a1 = g->failure_keep_count;
	int out_lab = g->NewLabel();
	if(K_needed(g, p->P_Left)) {
		writef(g, "~{~k~C", p);
		keep_c = g->keep_count;
	}
	else {
		writef(g, "~M~C", p);
	}
	p = p->P_Left;
	g->failure_keep_count = 0;
	while(p->P_Right) {
		g->failure_label = g->NewLabel();
		g->label_used = 0;
		generate(g, p);
		wgotol(g, out_lab);
		if(g->label_used)
			wsetl(g, g->failure_label);
		if(keep_c) {
			w(g, "~M"); wrestore(g, p, keep_c); w(g, "~N");
		}
		p = p->P_Right;
	}
	g->label_used = used;
	g->failure_label = a0;
	g->failure_keep_count = a1;
	generate(g, p);
	if(keep_c) 
		w(g, "~}");
	wsetl(g, out_lab);
}

static void generate_backwards(Generator * g, const Node * p) 
{
	writef(g, "~M~zlb = ~zc; ~zc = ~zl;~C~N", p);
	generate(g, p->P_Left);
	w(g, "~M~zc = ~zlb;~N");
}

static void generate_not(Generator * g, const Node * p) 
{
	int keep_c = 0;
	int used = g->label_used;
	int a0 = g->failure_label;
	int a1 = g->failure_keep_count;
	if(K_needed(g, p->P_Left)) {
		writef(g, "~{~k~C", p);
		keep_c = g->keep_count;
	}
	else {
		writef(g, "~M~C", p);
	}
	g->failure_label = g->NewLabel();
	g->label_used = 0;
	g->failure_keep_count = 0;
	generate(g, p->P_Left);
	{
		int l = g->failure_label;
		int u = g->label_used;
		g->label_used = used;
		g->failure_label = a0;
		g->failure_keep_count = a1;
		writef(g, "~M~f~N", p);
		if(u)
			wsetl(g, l);
	}
	if(keep_c) {
		w(g, "~M"); 
		wrestore(g, p, keep_c); 
		w(g, "~N~}");
	}
}

static void generate_try(Generator * g, const Node * p) 
{
	int keep_c = 0;
	if(K_needed(g, p->P_Left)) {
		writef(g, "~{~k~C", p);
		keep_c = g->keep_count;
	}
	else {
		writef(g, "~M~C", p);
	}
	g->failure_keep_count = keep_c;
	g->failure_label = g->NewLabel();
	g->label_used = 0;
	generate(g, p->P_Left);
	if(g->label_used)
		wsetl(g, g->failure_label);
	if(keep_c) 
		w(g, "~}");
}

static void generate_set(Generator * g, const Node * p) 
{
	g->V[0] = p->name; 
	writef(g, "~M~V0 = 1;~C", p);
}

static void generate_unset(Generator * g, const Node * p) 
{
	g->V[0] = p->name; 
	writef(g, "~M~V0 = 0;~C", p);
}

static void generate_fail(Generator * g, const Node * p) 
{
	generate(g, p->P_Left);
	writef(g, "~M~f~C", p);
}

/* generate_test() also implements 'reverse' */

static void generate_test(Generator * g, const Node * p) 
{
	int keep_c = 0;
	if(K_needed(g, p->P_Left)) {
		keep_c = ++g->keep_count;
		w(g, p->mode == m_forward ? "~{int c_test" : "~{int m_test");
		write_int(g, keep_c);
		w(g, p->mode == m_forward ? " = ~zc;" : " = ~zl - ~zc;");
		writef(g, "~C", p);
	}
	else 
		writef(g, "~M~C", p);
	generate(g, p->P_Left);
	if(keep_c) {
		w(g, p->mode == m_forward ? "~M~zc = c_test" : "~M~zc = ~zl - m_test");
		write_int(g, keep_c);
		writef(g, ";~N~}", p);
	}
}

static void generate_do(Generator * g, const Node * p) 
{
	int keep_c = 0;
	if(K_needed(g, p->P_Left)) {
		writef(g, "~{~k~C", p);
		keep_c = g->keep_count;
	}
	else {
		writef(g, "~M~C", p);
	}
	if(p->P_Left->type == c_call) {
		// Optimise do <call> 
		g->V[0] = p->P_Left->name;
		writef(g, "~{int ret = ~V0(~z);~C", p->P_Left);
		w(g, "~Mif(ret < 0) return ret;~N~}");
	}
	else {
		g->failure_label = g->NewLabel();
		g->label_used = 0;
		g->failure_keep_count = 0;
		generate(g, p->P_Left);
		if(g->label_used)
			wsetl(g, g->failure_label);
	}
	if(keep_c) {
		w(g, "~M"); 
		wrestore(g, p, keep_c);
		w(g, "~N~}");
	}
}

static void generate_next(Generator * g, const Node * p) 
{
	if(g->P_Options->encoding == ENC_UTF8) {
		if(p->mode == m_forward)
			w(g, "~{ int ret = skip_utf8(~zp, ~zc, 0, ~zl, 1");
		else
			w(g, "~{ int ret = skip_utf8(~zp, ~zc, ~zlb, 0, -1");
		writef(g, ");~N"
		    "~Mif(ret < 0) ~f~N"
		    "~M~zc = ret;~C"
		    "~}", p);
	}
	else
		writef(g, "~M~l~N~M~i~C", p);
}

static void generate_GO_grouping(Generator * g, Node * p, int is_goto, int complement) 
{
	Grouping * q = p->name->grouping;
	g->S[0] = p->mode == m_forward ? "" : "_b";
	g->S[1] = complement ? "in" : "out";
	g->S[2] = g->P_Options->encoding == ENC_UTF8 ? "_U" : "";
	g->V[0] = p->name;
	g->I[0] = q->smallest_ch;
	g->I[1] = q->largest_ch;
	if(is_goto) {
		writef(g, "~Mif(~S1_grouping~S0~S2(~Z~V0, ~I0, ~I1, 1) < 0) ~f~C", p);
	}
	else {
		writef(g, "~{~C~Mint ret = ~S1_grouping~S0~S2(~Z~V0, ~I0, ~I1, 1);~N~Mif(ret < 0) ~f~N", p);
		if(p->mode == m_forward)
			w(g, "~M~zc += ret;~N");
		else
			w(g, "~M~zc -= ret;~N");
		w(g, "~}");
	}
}

static void generate_GO(Generator * g, const Node * p, int style) 
{
	int keep_c = 0;
	int used = g->label_used;
	int a0 = g->failure_label;
	int a1 = g->failure_keep_count;
	if(p->P_Left->type == c_grouping || p->P_Left->type == c_non) {
		/* Special case for "goto" or "gopast" when used on a grouping or an
		 * inverted grouping - the movement of c by the matching action is
		 * exactly what we want! */
#ifdef OPTIMISATION_WARNINGS
		printf("Optimising %s %s\n", style ? "goto" : "gopast", p->left->type == c_non ? "non" : "grouping");
#endif
		if(g->P_Options->comments) {
			writef(g, "~M~C", p);
		}
		generate_GO_grouping(g, p->P_Left, style, p->P_Left->type == c_non);
		return;
	}
	w(g, "~Mwhile(1) {"); 
	writef(g, "~C~+", p);
	if(style == 1 || repeat_restore(g, p->P_Left)) {
		writef(g, "~M~k~N", p);
		keep_c = g->keep_count;
	}
	g->failure_label = g->NewLabel();
	g->label_used = 0;
	generate(g, p->P_Left);
	if(style == 1) {
		/* include for goto; omit for gopast */
		w(g, "~M"); wrestore(g, p, keep_c); w(g, "~N");
	}
	w(g, "~Mbreak;~N");
	if(g->label_used)
		wsetl(g, g->failure_label);
	if(keep_c) {
		w(g, "~M"); wrestore(g, p, keep_c); w(g, "~N");
	}
	g->label_used = used;
	g->failure_label = a0;
	g->failure_keep_count = a1;
	/*  writef(g, "~M~l~N" "~M~i~N", p);  */
	generate_next(g, p);
	w(g, "~}");
}

static void generate_loop(Generator * g, const Node * p) 
{
	w(g, "~{int i; for(i = "); 
	generate_AE(g, p->AE); 
	writef(g, "; i > 0; i--)~C~{", p);
	generate(g, p->P_Left);
	w(g,    "~}~}");
}

static void generate_repeat_or_atleast(Generator * g, const Node * p, int atleast_case) 
{
	int keep_c = 0;
	if(atleast_case) {
		writef(g, "~Mwhile(1) {~+~N", p);
	}
	else {
		writef(g, "~Mwhile(1) {~+~C", p);
	}
	if(repeat_restore(g, p->P_Left)) {
		writef(g, "~M~k~N", p);
		keep_c = g->keep_count;
	}
	g->failure_label = g->NewLabel();
	g->label_used = 0;
	g->failure_keep_count = 0;
	generate(g, p->P_Left);
	if(atleast_case) 
		w(g, "~Mi--;~N");
	w(g, "~Mcontinue;~N");
	if(g->label_used)
		wsetl(g, g->failure_label);
	if(keep_c) {
		w(g, "~M"); 
		wrestore(g, p, keep_c); 
		w(g, "~N");
	}
	w(g, "~Mbreak;~N~}");
}

static void generate_repeat(Generator * g, const Node * p) 
{
	generate_repeat_or_atleast(g, p, false);
}

static void generate_atleast(Generator * g, const Node * p) 
{
	w(g, "~{ int i = "); 
	generate_AE(g, p->AE); 
	w(g, ";~C");
	{
		int used = g->label_used;
		int a0 = g->failure_label;
		int a1 = g->failure_keep_count;
		generate_repeat_or_atleast(g, p, true);
		g->label_used = used;
		g->failure_label = a0;
		g->failure_keep_count = a1;
	}
	writef(g, "~Mif(i > 0) ~f~N~}", p);
}

static void generate_setmark(Generator * g, const Node * p) 
{
	g->V[0] = p->name;
	writef(g, "~M~V0 = ~zc;~C", p);
}

static void generate_tomark(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? ">" : "<";
	w(g, "~Mif(~zc ~S0 "); 
	generate_AE(g, p->AE); 
	writef(g, ") ~f~N", p);
	w(g, "~M~zc = "); 
	generate_AE(g, p->AE); writef(g, ";~C", p);
}

static void generate_atmark(Generator * g, const Node * p) 
{
	w(g, "~Mif(~zc != "); 
	generate_AE(g, p->AE); 
	writef(g, ") ~f~C", p);
}

static void generate_hop(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "+" : "-";
	g->S[1] = p->mode == m_forward ? "0" : (g->P_Options->make_lang == Options::LANG_C ? "z->lb" : "lb");
	if(g->P_Options->encoding == ENC_UTF8) {
		w(g, "~{ int ret = skip_utf8(~zp, ~zc, ~S1, ~zl, ~S0 ");
		generate_AE(g, p->AE); 
		writef(g, ");~C", p);
		writef(g, "~Mif(ret < 0) ~f~N", p);
	}
	else {
		w(g, "~{ int ret = ~zc ~S0 ");
		generate_AE(g, p->AE); 
		writef(g, ";~C", p);
		writef(g, "~Mif(~S1 > ret || ret > ~zl) ~f~N", p);
	}
	writef(g, "~M~zc = ret;~N~}", p);
}

static void generate_delete(Generator * g, const Node * p) 
{
	writef(g, "~{ int ret = slice_del(~Z);~C", p);
	writef(g, "~Mif(ret < 0) return ret;~N~}", p);
}

static void generate_tolimit(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "" : "b";
	writef(g, "~M~zc = ~zl~S0;~C", p);
}

static void generate_atlimit(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "" : "b";
	g->S[1] = p->mode == m_forward ? "<" : ">";
	writef(g, "~Mif(~zc ~S1 ~zl~S0) ~f~C", p);
}

static void generate_leftslice(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "bra" : "ket";
	writef(g, "~M~z~S0 = ~zc;~C", p);
}

static void generate_rightslice(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "ket" : "bra";
	writef(g, "~M~z~S0 = ~zc;~C", p);
}

static void generate_assignto(Generator * g, const Node * p) 
{
	g->V[0] = p->name;
	writef(g, "~M~V0 = assign_to(~Z~V0);~C", p);
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~Mif(~V0 == 0) return -1;~C", p);
}

static void generate_sliceto(Generator * g, const Node * p) 
{
	g->V[0] = p->name;
	writef(g, "~{ symbol * ret = slice_to(~Z~V0);~C"
	    "~Mif(ret == 0) return -1;~N"
	    "~M~V0 = ret;~N"
	    "~}", p);
}

static void generate_insert(Generator * g, const Node * p, int style) 
{
	int keep_c = style == c_attach;
	if(p->mode == m_backward) keep_c = !keep_c;
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~{int ret;~N", p);
	if(keep_c) 
		w(g, "~{int saved_c = ~zc;~N");
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~Mret = insert_~$(~Z~zc, ~zc, ~a);~C", p);
	else
		writef(g, "~Minsert_~$(~Z~zc, ~zc, ~a);~C", p);
	if(keep_c) 
		w(g, "~M~zc = saved_c;~N~}");
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~Mif(ret < 0) return ret;~N~}", p);
}

static void generate_assignfrom(Generator * g, const Node * p) 
{
	int keep_c = p->mode == m_forward; /* like 'attach' */
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~{int ret;~N", p);
	if(keep_c) writef(g, "~{int saved_c = ~zc;~N", p);
	w(g, g->P_Options->make_lang == Options::LANG_C ? "~Mret =" : "~M");
	writef(g, keep_c ? "insert_~$(~Z~zc, ~zl, ~a);~C" : "insert_~$(~Z~zlb, ~zc, ~a);~C", p);
	if(keep_c) 
		w(g, "~M~zc = saved_c;~N~}");
	if(g->P_Options->make_lang == Options::LANG_C)
		writef(g, "~Mif(ret < 0) return ret;~N~}", p);
}

/* bugs marked <======= fixed 22/7/02. Similar fixes required for Java */

static void generate_slicefrom(Generator * g, const Node * p) 
{
/*  w(g, "~Mslice_from_s(~Z");   <============= bug! should be: */
	writef(g, "~{ int ret = slice_from_~$(~Z~a);~C", p);
	writef(g, "~Mif(ret < 0) return ret;~N~}", p);
}

static void generate_setlimit(Generator * g, const Node * p) 
{
	int keep_c;
	if(p->P_Left && p->P_Left->type == c_tomark) {
		/* Special case for:
		 *
		 *   setlimit tomark AE for C
		 *
		 * All uses of setlimit in the current stemmers we ship follow this
		 * pattern, and by special-casing we can avoid having to save and
		 * restore c.
		 */
		Node * q = p->P_Left;
		++g->keep_count;
		writef(g, "~N~{ int mlimit", p);
		write_int(g, g->keep_count);
		writef(g, ";~C", p);
		keep_c = g->keep_count;
		g->S[0] = q->mode == m_forward ? ">" : "<";
		w(g, "~Mif(~zc ~S0 "); generate_AE(g, q->AE); writef(g, ") ~f~N", q);
		w(g, "~Mmlimit");
		write_int(g, keep_c);
		if(p->mode == m_forward) {
			w(g, " = ~zl - ~zc; ~zl = ");
		}
		else {
			w(g, " = ~zlb; ~zlb = ");
		}
		generate_AE(g, q->AE);
		w(g, ";~N");
	}
	else {
		writef(g, "~{~K~C", p);
		keep_c = g->keep_count;
		generate(g, p->P_Left);

		w(g, "~Mmlimit");
		write_int(g, keep_c);
		if(p->mode == m_forward)
			w(g, " = ~zl - ~zc; ~zl = ~zc;~N");
		else
			w(g, " = ~zlb; ~zlb = ~zc;~N");
		w(g, "~M"); 
		wrestore(g, p, keep_c); 
		w(g, "~N");
	}
	g->failure_keep_count = -keep_c;
	generate(g, p->P_Aux);
	w(g, "~M");
	wrestorelimit(g, p, -g->failure_keep_count);
	w(g, "~N~}");
}

static const char * vars[] = { "p", "c", "l", "lb", "bra", "ket", NULL };

/* dollar sets snowball up to operate on a string variable as if it were the
 * current string */
static void generate_dollar(Generator * g, const Node * p) 
{
	int used = g->label_used;
	int a0 = g->failure_label;
	int a1 = g->failure_keep_count;
	int keep_token;
	g->failure_label = g->NewLabel();
	g->label_used = 0;
	g->failure_keep_count = 0;
	keep_token = ++g->keep_count;
	g->I[0] = keep_token;
	if(g->P_Options->make_lang != Options::LANG_C) {
		writef(g, "~{~C", p);
		for(const char ** pp_var = vars; *pp_var; ++pp_var) {
			g->S[0] = *pp_var;
			w(g, "~Mint ~S0~I0 = ~S0;~N");
		}
	}
	else {
		writef(g, "~{ struct SN_env env~I0 = * z;~C", p);
	}
	g->V[0] = p->name;
	/* Assume failure. */
	writef(g, "~Mint failure = 1;~N"
	    "~M~zp = ~V0;~N"
	    "~M~zlb = ~zc = 0;~N"
	    "~M~zl = SIZE(~zp);~N", p);
	generate(g, p->P_Left);
	/* Mark success. */
	w(g, "~Mfailure = 0;~N");
	if(g->label_used)
		wsetl(g, g->failure_label);
	g->V[0] = p->name; /* necessary */

	g->label_used = used;
	g->failure_label = a0;
	g->failure_keep_count = a1;

	g->I[0] = keep_token;
	if(g->P_Options->make_lang != Options::LANG_C) {
		const char ** var;
		w(g, "~M~V0 = ~zp;~N");
		for(var = vars; *var; ++var) {
			g->S[0] = *var;
			w(g, "~M~S0 = ~S0~I0;~N");
		}
		writef(g, "~Mif(failure) ~f~N~}", p);
	}
	else {
		writef(g, "~M~V0 = z->p;~N"
		    "~M* z = env~I0;~N"
		    "~Mif(failure) ~f~N~}", p);
	}
}

static void generate_integer_assign(Generator * g, const Node * p, const char * s) 
{
	g->V[0] = p->name;
	g->S[0] = s;
	w(g, "~M~V0 ~S0 "); 
	generate_AE(g, p->AE); 
	writef(g, ";~C", p);
}

static void generate_integer_test(Generator * g, const Node * p, const char * s) 
{
	w(g, "~Mif(!(");
	generate_AE(g, p->P_Left);
	write_char(g, ' ');
	write_string(g, s);
	write_char(g, ' ');
	generate_AE(g, p->AE);
	writef(g, ")) ~f~C", p);
}

static void generate_call(Generator * g, const Node * p) 
{
	g->V[0] = p->name;
	writef(g, "~{ int ret = ~V0(~Z);~C", p);
	if(g->failure_keep_count == 0 && g->failure_label == x_return) {
		// Combine the two tests in this special case for better optimisation and clearer generated code. 
		writef(g, "~Mif(ret <= 0) return ret;~N~}", p);
	}
	else {
		writef(g, "~Mif(ret == 0) ~f~N~Mif(ret < 0) return ret;~N~}", p);
	}
}

static void generate_grouping(Generator * g, const Node * p, int complement) 
{
	const Grouping * q = p->name->grouping;
	g->S[0] = p->mode == m_forward ? "" : "_b";
	g->S[1] = complement ? "out" : "in";
	g->S[2] = g->P_Options->encoding == ENC_UTF8 ? "_U" : "";
	g->V[0] = p->name;
	g->I[0] = q->smallest_ch;
	g->I[1] = q->largest_ch;
	writef(g, "~Mif(~S1_grouping~S0~S2(~Z~V0, ~I0, ~I1, 0)) ~f~C", p);
}

static void generate_namedstring(Generator * g, const Node * p) 
{
	g->S[0] = p->mode == m_forward ? "" : "_b";
	g->V[0] = p->name;
	writef(g, "~Mif(!(eq_v~S0(~Z~V0))) ~f~C", p);
}

static void generate_literalstring(Generator * g, const Node * p) 
{
	symbol * b = p->P_LiteralString;
	if(SIZE(b) == 1) {
		/* It's quite common to compare with a single character literal string,
		 * so just inline the simpler code for this case rather than making a
		 * function call.  In UTF-8 mode, only do this for the ASCII subset,
		 * since multi-byte characters are more complex to test against.
		 */
		if(g->P_Options->encoding == ENC_UTF8 && *b >= 128) {
			printf("single byte %d\n", *b);
			exit(1);
		}
		g->I[0] = *b;
		if(p->mode == m_forward) {
			writef(g, "~Mif(~zc == ~zl || ~zp[~zc] != ~c0) ~f~C~M~zc++;~N", p);
		}
		else {
			writef(g, "~Mif(~zc <= ~zlb || ~zp[~zc - 1] != ~c0) ~f~C~M~zc--;~N", p);
		}
	}
	else {
		g->S[0] = p->mode == m_forward ? "" : "_b";
		g->I[0] = SIZE(b);
		g->L[0] = b;
		writef(g, "~Mif(!(eq_s~S0(~Z~I0, ~L0))) ~f~C", p);
	}
}

static void generate_define(Generator * g, const Node * p) 
{
	const Name * q = p->name;
	g->NextLabel = 0;
	if(g->P_Options->make_lang == Options::LANG_C)
		g->S[0] = (q->type == t_routine) ? "static" : "extern";
	else
		g->S[0] = g->P_Options->name;
	g->V[0] = q;
	if(g->P_Options->make_lang == Options::LANG_C)
		w(g, "~N~S0 int ~V0(struct SN_env * z) {");
	else
		w(g, "~Nint Xapian::~S0::~V0() {");
	if(g->P_Options->comments) {
		write_string(g, p->mode == m_forward ? " /* forwardmode */" : " /* backwardmode */");
	}
	w(g, "~N~+");
	if(p->AmongVarNeeded) 
		w(g, "~Mint among_var;~N");
	g->failure_keep_count = 0;
	g->failure_label = x_return;
	g->label_used = 0;
	g->keep_count = 0;
	generate(g, p->P_Left);
	w(g, "~Mreturn 1;~N~}");
}

static void generate_substring(Generator * g, const Node * p) 
{
	Among * x = p->among;
	int block = -1;
	uint bitmap = 0;
	AmongVec * among_cases = x->P_Vec;
	int c;
	int empty_case = -1;
	int n_cases = 0;
	symbol cases[2];
	int shortest_size = INT_MAX;
	int shown_comment = 0;

	g->S[0] = p->mode == m_forward ? "" : "_b";
	g->I[0] = x->number;
	g->I[1] = x->LiteralStringCount;
	// 
	// In forward mode with non-ASCII UTF-8 characters, the first character
	// of the string will often be the same, so instead look at the last
	// common character position.
	// 
	// In backward mode, we can't match if there are fewer characters before
	// the current position than the minimum length.
	// 
	for(c = 0; c < x->LiteralStringCount; ++c) {
		int size = among_cases[c].size;
		if(size != 0 && size < shortest_size) {
			shortest_size = size;
		}
	}
	for(c = 0; c < x->LiteralStringCount; ++c) {
		symbol ch;
		if(among_cases[c].size == 0) {
			empty_case = c;
			continue;
		}
		if(p->mode == m_forward) {
			ch = among_cases[c].b[shortest_size - 1];
		}
		else {
			ch = among_cases[c].b[among_cases[c].size - 1];
		}
		if(n_cases == 0) {
			block = ch >> 5;
		}
		else if(ch >> 5 != block) {
			block = -1;
			if(n_cases > 2) break;
		}
		if(block == -1) {
			if(n_cases > 0 && ch == cases[0]) continue;
			if(n_cases < 2) {
				cases[n_cases++] = ch;
			}
			else if(ch != cases[1]) {
				++n_cases;
				break;
			}
		}
		else {
			if((bitmap & (1u << (ch & 0x1f))) == 0) {
				bitmap |= 1u << (ch & 0x1f);
				if(n_cases < 2)
					cases[n_cases] = ch;
				++n_cases;
			}
		}
	}

	if(block != -1 || n_cases <= 2) {
		char buf[64];
		g->I[2] = block;
		g->I[3] = bitmap;
		g->I[4] = shortest_size - 1;
		if(p->mode == m_forward) {
			const char * z = g->P_Options->make_lang == Options::LANG_C ? "z->" : "";
			sprintf(buf, "%sp[%sc + %d]", z, z, shortest_size - 1);
			g->S[1] = buf;
			if(shortest_size == 1) {
				writef(g, "~Mif(~zc >= ~zl", p);
			}
			else {
				writef(g, "~Mif(~zc + ~I4 >= ~zl", p);
			}
		}
		else {
			if(g->P_Options->make_lang == Options::LANG_C)
				g->S[1] = "z->p[z->c-1]";
			else
				g->S[1] = "p[c-1]";
			if(shortest_size == 1) {
				writef(g, "~Mif(~zc <= ~zlb", p);
			}
			else {
				writef(g, "~Mif(~zc - ~I4 <= ~zlb", p);
			}
		}
		if(n_cases == 0) {
			/* We get this for the degenerate case: among { '' }
			 * This doesn't seem to be a useful construct, but it is
			 * syntactically valid.
			 */
		}
		else if(n_cases == 1) {
			g->I[4] = cases[0];
			writef(g, " || ~S1 != ~I4", p);
		}
		else if(n_cases == 2) {
			g->I[4] = cases[0];
			g->I[5] = cases[1];
			writef(g, " || (~S1 != ~I4 && ~S1 != ~I5)", p);
		}
		else {
			writef(g, " || ~S1 >> 5 != ~I2 || !((~I3 >> (~S1 & 0x1f)) & 1)", p);
		}
		write_string(g, ") ");
		if(empty_case != -1) {
			/* If the among includes the empty string, it can never fail
			 * so not matching the bitmap means we match the empty string.
			 */
			g->I[4] = among_cases[empty_case].result;
			writef(g, "among_var = ~I4; else~C", p);
		}
		else {
			writef(g, "~f~C", p);
		}
		shown_comment = 1;
	}
	else {
#ifdef OPTIMISATION_WARNINGS
		printf("Couldn't shortcut among %d\n", x->number);
#endif
	}

	if(!x->amongvar_needed) {
		writef(g, "~Mif(!(find_among~S0(s_pool, ~Za_~I0, ~I1, ", p);
		if(x->function_count) {
			w(g, "af_~I0, af");
		}
		else {
			write_string(g, "0, 0");
		}
		writef(g, "))) ~f", p);
		writef(g, shown_comment ? "~N" : "~C", p);
	}
	else {
		w(g, "~Mamong_var = find_among~S0(s_pool, ~Za_~I0, ~I1, ");
		if(x->function_count) {
			w(g, "af_~I0, af");
		}
		else {
			write_string(g, "0, 0");
		}
		writef(g, ");", p);
		writef(g, shown_comment ? "~N" : "~C", p);
		writef(g, "~Mif(!(among_var)) ~f~N", p);
	}
}

static void generate_among(Generator * g, const Node * p) 
{
	Among * x = p->among;
	if(x->substring == 0) 
		generate_substring(g, p);
	if(x->starter != 0) 
		generate(g, x->starter);
	if(x->CommandCount == 1 && x->NoCommandCount == 0) {
		// Only one outcome ("no match" already handled). 
		generate(g, x->commands[0]);
	}
	else if(x->CommandCount > 0) {
		writef(g, "~Mswitch(among_var) {~C~+", p);
		for(int i = 1; i <= x->CommandCount; i++) {
			g->I[0] = i;
			w(g, "~Mcase ~I0:~N~+");
			generate(g, x->commands[i - 1]);
			w(g, "~Mbreak;~N~-");
		}
		w(g, "~}");
	}
}

static void generate_booltest(Generator * g, const Node * p) 
{
	g->V[0] = p->name;
	writef(g, "~Mif(!(~V0)) ~f~C", p);
}

static void generate_false(Generator * g, const Node * p) 
{
	writef(g, "~M~f~C", p);
}

static void generate_debug(Generator * g, const Node * p) {
	g->I[0] = g->debug_count++;
	g->I[1] = p->line_number;
	writef(g, "~Mdebug(~Z~I0, ~I1);~C", p);
}

static void generate(Generator * g, const Node * p) 
{
	int used = g->label_used;
	int a0 = g->failure_label;
	int a1 = g->failure_keep_count;
	switch(p->type) {
		case c_define:        generate_define(g, p); break;
		case c_bra:           generate_bra(g, p); break;
		case c_and:           generate_and(g, p); break;
		case c_or:            generate_or(g, p); break;
		case c_backwards:     generate_backwards(g, p); break;
		case c_not:           generate_not(g, p); break;
		case c_set:           generate_set(g, p); break;
		case c_unset:         generate_unset(g, p); break;
		case c_try:           generate_try(g, p); break;
		case c_fail:          generate_fail(g, p); break;
		case c_reverse:
		case c_test:          generate_test(g, p); break;
		case c_do:            generate_do(g, p); break;
		case c_goto:          generate_GO(g, p, 1); break;
		case c_gopast:        generate_GO(g, p, 0); break;
		case c_repeat:        generate_repeat(g, p); break;
		case c_loop:          generate_loop(g, p); break;
		case c_atleast:       generate_atleast(g, p); break;
		case c_setmark:       generate_setmark(g, p); break;
		case c_tomark:        generate_tomark(g, p); break;
		case c_atmark:        generate_atmark(g, p); break;
		case c_hop:           generate_hop(g, p); break;
		case c_delete:        generate_delete(g, p); break;
		case c_next:          generate_next(g, p); break;
		case c_tolimit:       generate_tolimit(g, p); break;
		case c_atlimit:       generate_atlimit(g, p); break;
		case c_leftslice:     generate_leftslice(g, p); break;
		case c_rightslice:    generate_rightslice(g, p); break;
		case c_assignto:      generate_assignto(g, p); break;
		case c_sliceto:       generate_sliceto(g, p); break;
		case c_assign:        generate_assignfrom(g, p); break;
		case c_insert:
		case c_attach:        generate_insert(g, p, p->type); break;
		case c_slicefrom:     generate_slicefrom(g, p); break;
		case c_setlimit:      generate_setlimit(g, p); break;
		case c_dollar:        generate_dollar(g, p); break;
		case c_mathassign:    generate_integer_assign(g, p, "="); break;
		case c_plusassign:    generate_integer_assign(g, p, "+="); break;
		case c_minusassign:   generate_integer_assign(g, p, "-="); break;
		case c_multiplyassign: generate_integer_assign(g, p, "*="); break;
		case c_divideassign:  generate_integer_assign(g, p, "/="); break;
		case c_eq:            generate_integer_test(g, p, "=="); break;
		case c_ne:            generate_integer_test(g, p, "!="); break;
		case c_gr:            generate_integer_test(g, p, ">"); break;
		case c_ge:            generate_integer_test(g, p, ">="); break;
		case c_ls:            generate_integer_test(g, p, "<"); break;
		case c_le:            generate_integer_test(g, p, "<="); break;
		case c_call:          generate_call(g, p); break;
		case c_grouping:      generate_grouping(g, p, false); break;
		case c_non:           generate_grouping(g, p, true); break;
		case c_name:          generate_namedstring(g, p); break;
		case c_literalstring: generate_literalstring(g, p); break;
		case c_among:         generate_among(g, p); break;
		case c_substring:     generate_substring(g, p); break;
		case c_booltest:      generate_booltest(g, p); break;
		case c_false:         generate_false(g, p); break;
		case c_true:          break;
		case c_debug:         generate_debug(g, p); break;
		default: slfprintf_stderr("%d encountered\n", p->type);
		    exit(1);
	}
	if(g->failure_label != a0)
		g->label_used = used;
	g->failure_label = a0;
	g->failure_keep_count = a1;
}

void write_generated_comment_content(Generator * g) {
	w(g, "Generated by Snowball " SNOWBALL_VERSION " - https://snowballstem.org/");
}

void write_start_comment(Generator * g, const char * comment_start, const char * comment_end) 
{
	write_margin(g);
	w(g, comment_start);
	write_generated_comment_content(g);
	if(comment_end) {
		w(g, comment_end);
	}
	w(g, "~N~N");
}

static void generate_head(Generator * g) 
{
	if(g->P_Options->make_lang != Options::LANG_C) {
		const char * s = g->P_Options->output_file;
		const char * leaf;
		w(g, "~N"
			"#include <xapian-internal.h>~N" // @sobolev
			"#pragma hdrstop~N" // @sobolev
		    // @sobolev "#include <config.h>~N"
		    // @sobolev "#include <limits.h>~N"
		);
		if(!s) 
			abort(); /* checked in driver.c */
		leaf = strrchr(s, '/');
		if(leaf) ++leaf; else leaf = s;
		write_string(g, "#include \"");
		write_string(g, leaf);
		w(g, ".h\"~N~N");
		return;
	}

	w(g, "#include \"");
	if(g->P_Options->runtime_path) {
		write_string(g, g->P_Options->runtime_path);
		if(g->P_Options->runtime_path[strlen(g->P_Options->runtime_path) - 1] != '/')
			write_char(g, '/');
	}
	w(g, "header.h\"~N~N");
}

static void generate_routine_headers(Generator * g) 
{
	Name * q;
	for(q = g->analyser->P_Names; q; q = q->next) {
		g->V[0] = q;
		switch(q->type) {
			case t_routine:
			    w(g, "static int ~W0(struct SN_env * z);~N");
			    break;
			case t_external:
			    w(g,
				"#ifdef __cplusplus~N"
				"extern \"C\" {~N"
				"#endif~N"
				"extern int ~W0(struct SN_env * z);~N"
				"#ifdef __cplusplus~N"
				"}~N"
				"#endif~N"
				);
			    break;
		}
	}
}

static void generate_among_pool(Generator * g, Among * x) 
{
	/*static*/uint pool_size = 0; // @sobolev removed static: it prevented to build several source files at onece
	while(x) {
		AmongVec * v = x->P_Vec;
		int i;
		char * done = (char *)check_malloc(x->LiteralStringCount);
		memzero(done, x->LiteralStringCount);
		g->I[0] = x->number;
		for(i = 0; i < x->LiteralStringCount; i++, v++) {
			int j;
			if(v->size == 0 || done[i]) 
				continue;
			g->I[1] = i;
			// Eliminate entries which are just substrings of other entries 
			for(j = 0; j < x->LiteralStringCount; j++) {
				if(j == i) 
					continue;
				if(v->size <= v[j-i].size) {
					size_t offset = v[j - i].size - v->size;
					const  size_t len = v->size * sizeof(symbol);
					do {
						if(memcmp(v->b, v[j - i].b + offset, len) == 0) {
							g->I[2] = j;
							if(offset) {
								g->I[3] = offset;
								w(g, "#define s_~I0_~I1 (s_~I0_~I2 + ~I3)~N");
							}
							else {
								w(g, "#define s_~I0_~I1 s_~I0_~I2~N");
							}
							goto done;
						}
					} while(offset--);
				}
			}
			if(v->size) {
				if(pool_size == 0) {
					w(g, "static const symbol s_pool[] = {~N");
				}
				g->I[2] = pool_size;
				w(g, "#define s_~I0_~I1 ~I2~N");
				g->L[0] = v->b;
				w(g, "~A0,~N");
				pool_size += v->size;
			}
done:                   ;
		}
		check_free(done);
		x = x->P_Next;
	}
	if(pool_size != 0) {
		w(g, "};~N~N");
	}
}

static void generate_among_table(Generator * g, const Among * x) 
{
	const AmongVec * v = x->P_Vec;
	g->I[0] = x->number;
	g->I[1] = x->LiteralStringCount;
	w(g, "~N~Mstatic const Among a_~I0[~I1] = {~N");
	{
		for(int i = 0; i < x->LiteralStringCount; i++) {
			g->I[1] = i;
			g->I[2] = v->size;
			g->I[3] = v->i;
			g->I[4] = v->result;
			g->S[0] = i < (x->LiteralStringCount - 1) ? "," : "";
			w(g, "/*~J1 */ { ~I2, ");
			if(v->size == 0) 
				w(g, "0,");
			else 
				w(g, "s_~I0_~I1,");
			w(g, " ~I3, ~I4");
			w(g, "}~S0~N");
			v++;
		}
	}
	w(g, "};~N~N");
	if(x->function_count) {
		g->I[1] = x->LiteralStringCount;
		w(g, "~Mstatic const uchar af_~I0[~I1] = {~N");
		v = x->P_Vec;
		{
			for(int i = 0; i < x->LiteralStringCount; i++) {
				g->I[1] = i;
				w(g, "/*~J1 */ ");
				if(v[i].function == 0) {
					w(g, "0");
				}
				else {
					write_int(g, v[i].function->among_func_count);
					g->V[0] = v[i].function;
					w(g, " /* t~W0 */");
				}
				if(i < (x->LiteralStringCount-1))
					w(g, ",~N");
			}
		}
		w(g, "~N};~N~N");
	}
}

static void generate_amongs(Generator * g) 
{
	Name * q;
	int among_func_count = 0;
	g->S[0] = g->P_Options->name;
	for(q = g->analyser->P_Names; q; q = q->next) {
		if(q->type == t_routine && q->used_in_among) {
			q->among_func_count = ++among_func_count;
			g->V[0] = q;
			w(g, "static int t~V0(Xapian::StemImplementation * this_ptr) { return (static_cast<Xapian::~S0 *>(this_ptr))->~V0(); }~N~N");
		}
	}
	if(among_func_count) {
		g->I[0] = among_func_count;
		w(g, "~Mstatic const among_function af[~I0] =~N{~N");
		q = g->analyser->P_Names;
		g->S[0] = g->P_Options->name;
		for(q = g->analyser->P_Names; q; q = q->next) {
			if(q->type == t_routine && q->used_in_among) {
				g->V[0] = q;
				g->I[0] = q->among_func_count;
				w(g, "/*~J0 */ t~V0");
				if(q->among_func_count < among_func_count) 
					w(g, ",~N"); else w(g, "~N");
			}
		}
		w(g, "};~N~N");
	}
	generate_among_pool(g, g->analyser->P_Amongs);
	for(const Among * x = g->analyser->P_Amongs; x; x = x->P_Next) {
		generate_among_table(g, x);
	}
}

static void set_bit(symbol * b, int i) 
{
	b[i/8] |= 1 << i%8;
}

static void generate_grouping_table(Generator * g, const Grouping * q) 
{
	int range = q->largest_ch - q->smallest_ch + 1;
	int size = (range + 7)/ 8; /* assume 8 bits per symbol */
	const symbol * b = q->b;
	symbol * map = create_b(size);
	int i;
	for(i = 0; i < size; i++) 
		map[i] = 0;
	for(i = 0; i < SIZE(b); i++) 
		set_bit(map, b[i] - q->smallest_ch);
	g->V[0] = q->name;
	w(g, "static const uchar ~V0[] = { ");
	for(i = 0; i < size; i++) {
		write_int(g, map[i]);
		if(i < size - 1) 
			w(g, ", ");
	}
	w(g, " };~N~N");
	lose_b(map);
}

static void generate_groupings(Generator * g) 
{
	for(const Grouping * q = g->analyser->P_Groupings; q; q = q->next) {
		if(q->name->P_Used)
			generate_grouping_table(g, q);
	}
}

static void generate_create(Generator * g) 
{
	const int * p = g->analyser->NameCount;
	if(g->P_Options->make_lang != Options::LANG_C) {
		Name * q = g->analyser->P_Names;
		int first = true;
		const char * dtor;
		g->S[0] = g->P_Options->name;
		dtor = strrchr(g->P_Options->name, ':');
		if(dtor) ++dtor; else dtor = g->P_Options->name;
		g->S[1] = dtor;
		w(g, "~NXapian::~S0::~S1()");
		while(q) {
			if(q->type < t_routine) {
				w(g, first ? "~N    : " : ", ");
				first = false;
				g->V[0] = q;
				w(g, "~W0(0)");
			}
			q = q->next;
		}
		w(g, "~N{~N");
		q = g->analyser->P_Names;
		while(q) {
			if(q->type == t_string) {
				g->V[0] = q;
				w(g, "    ~W0 = create_s();~N");
			}
			q = q->next;
		}
		w(g, "}~N");

		return;
	}

	g->I[0] = p[t_string];
	g->I[1] = p[t_integer] + p[t_boolean];
	w(g, "~Nextern struct SN_env * ~pcreate_env(void) { return SN_create_env(~I0, ~I1, ~I2); }~N");
}

static void generate_close(Generator * g) 
{
	const int * p = g->analyser->NameCount;
	if(g->P_Options->make_lang != Options::LANG_C) {
		Name * q = g->analyser->P_Names;
		const char * dtor;
		const char * lang;
		g->S[0] = g->P_Options->name;
		dtor = strrchr(g->P_Options->name, ':');
		if(dtor) 
			++dtor; 
		else 
			dtor = g->P_Options->name;
		g->S[1] = dtor;
		lang = strrchr(g->P_Options->output_file, '/');
		if(lang) 
			++lang; 
		else 
			lang = g->P_Options->output_file;
		g->S[2] = lang;
		w(g, "~NXapian::~S0::~~~S1()~N{~N");
		while(q) {
			if(q->type == t_string) {
				g->V[0] = q;
				w(g, "    lose_s(~W0);~N");
			}
			q = q->next;
		}
		w(g, "}~N");
		w(g, "~Nstd::string Xapian::~S0::get_description() const { return \"~S2\"; }~N");
		return;
	}
	g->I[0] = p[t_string];
	w(g, "~Nextern void ~pclose_env(struct SN_env * z) { SN_close_env(z, ~I0); }~N~N");
}

static void generate_create_and_close_templates(Generator * g) 
{
	w(g, "~N"
	    "extern struct SN_env * ~pcreate_env(void);~N"
	    "extern void ~pclose_env(struct SN_env * z);~N~N");
}

static void generate_header_file(Generator * g) 
{
	Name * q;
	const char * vp = g->P_Options->variables_prefix;
	g->S[0] = vp;
	if(g->P_Options->make_lang != Options::LANG_C) {
		const char * p;
		w(g, "~N#include \"steminternal.h\"~N~Nnamespace Xapian {~N~N");
		g->S[1] = g->P_Options->name;
		w(g, "class ~S1 ");
		if(g->P_Options->parent_class_name) {
			g->S[1] = g->P_Options->parent_class_name;
			w(g, ": public ~S1 ");
		}
		w(g, "{~N");
		for(q = g->analyser->P_Names; q; q = q->next) {
			switch(q->type) {
				case t_string:  g->S[1] = "symbol *"; goto label1;
				case t_integer: g->S[1] = "int"; goto label1;
				case t_boolean: g->S[1] = "uchar";
label1:
				    g->V[0] = q;
				    w(g, "    ~S1 ~W0;~N");
				    break;
			}
		}
		for(q = g->analyser->P_Names; q; q = q->next) {
			if(q->type == t_routine && !q->used_in_among) {
				g->V[0] = q;
				w(g, "    int ~W0();~N");
			}
		}
		w(g, "~N  public:~N");
		// FIXME: We currently need to make any routines used in an among public.
		for(q = g->analyser->P_Names; q; q = q->next) {
			if(q->type == t_routine && q->used_in_among) {
				g->V[0] = q;
				w(g, "\tint ~W0();~N");
			}
		}
		w(g, "~N");
		p = strrchr(g->P_Options->name, ':');
		if(p) 
			++p; 
		else 
			p = g->P_Options->name;
		g->S[1] = p;
		w(g, "\t~S1();~N\t~~~S1();~N");
		for(q = g->analyser->P_Names; q; q = q->next) {
			if(q->type == t_external) {
				g->V[0] = q;
				w(g, "\tint ~W0();~N");
			}
		}
		w(g, "\tstd::string get_description() const;~N};~N~N}~N");
		return;
	}
	w(g, "~N#ifdef __cplusplus~Nextern \"C\" {~N#endif~N"); /* for C++ */
	generate_create_and_close_templates(g);
	for(q = g->analyser->P_Names; q; q = q->next) {
		g->V[0] = q;
		switch(q->type) {
			case t_external:
			    w(g, "extern int ~W0(struct SN_env * z);~N");
			    break;
			case t_string:
			case t_integer:
			case t_boolean:
			    if(vp) {
				    int count = q->count;
				    if(count < 0) {
					    /* Unused variables should get removed from `names`. */
					    slfprintf_stderr("Optimised out variable ");
					    report_b(stderr, q->b);
					    slfprintf_stderr(" still in names list\n");
					    exit(1);
				    }
				    if(q->type == t_boolean) {
					    /* We use a single array for booleans and integers,
					     * with the integers first.
					     */
					    count += g->analyser->NameCount[t_integer];
				    }
				    g->I[0] = count;
				    g->I[1] = "SIIrxg"[q->type];
				    w(g, "#define ~S0");
				    write_b(g, q->b);
				    w(g, " (~c1[~I0])~N");
			    }
			    break;
		}
	}
	w(g, "~N#ifdef __cplusplus~N}~N#endif~N"); /* for C++ */
	w(g, "~N");
}

void generate_program_c(Generator * g) 
{
	g->outbuf = str_new();
	write_start_comment(g, "/* ", " */");
	generate_head(g);
	if(g->P_Options->make_lang == Options::LANG_C) {
		generate_routine_headers(g);
		w(g, "#ifdef __cplusplus~Nextern \"C\" {~N#endif~N~N");
		generate_create_and_close_templates(g);
		w(g, "~N#ifdef __cplusplus~N}~N#endif~N");
	}
	generate_amongs(g);
	generate_groupings(g);
	g->declarations = g->outbuf;
	g->outbuf = str_new();
	g->literalstring_count = 0;
	{
		for(const Node * p = g->analyser->P_Program; p; p = p->P_Right) {
			generate(g, p); 
		}
	}
	generate_create(g);
	generate_close(g);
	output_str(g->P_Options->output_src, g->declarations);
	str_delete(g->declarations);
	output_str(g->P_Options->output_src, g->outbuf);
	str_clear(g->outbuf);

	write_start_comment(g, "/* ", " */");
	generate_header_file(g);
	output_str(g->P_Options->output_h, g->outbuf);
	str_delete(g->outbuf);
}
//
// Generator functions common to multiple languages.
//
extern Generator * create_generator(Analyser * a, const Options * pOpts) 
{
	NEW(Generator, g);
	g->analyser = a;
	g->P_Options = pOpts;
	g->margin = 0;
	g->debug_count = 0;
	g->copy_from_count = 0;
	g->line_count = 0;
	g->line_labelled = 0;
	g->failure_label = -1;
	g->unreachable = false;
#ifndef DISABLE_PYTHON
	g->max_label = 0;
#endif
	return g;
}

extern void close_generator(Generator * g) 
{
	FREE(g);
}
//
// Write routines for simple entities 
//
extern void write_char(Generator * g, int ch) 
{
	str_append_ch(g->outbuf, ch); /* character */
}

extern void write_newline(Generator * g) 
{
	str_append_ch(g->outbuf, '\n'); /* newline */
	g->line_count++;
}

extern void write_string(Generator * g, const char * s) { str_append_string(g->outbuf, s); }
extern void write_int(Generator * g, int i) { str_append_int(g->outbuf, i); }
extern void write_b(Generator * g, const symbol * b) { str_append_b(g->outbuf, b); }
extern void write_str(Generator * g, struct str * str) { str_append(g->outbuf, str); }
