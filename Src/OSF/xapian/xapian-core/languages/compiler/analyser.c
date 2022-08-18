//
//
#include <slib.h>
#include "header.h"
#pragma hdrstop

Node * Analyser::NewNode(int type) 
{
	Node * p = static_cast<Node *>(SAlloc::M(sizeof(Node)));
	p->P_Next = P_Nodes; 
	P_Nodes = p;
	p->P_Left = 0;
	p->P_Right = 0;
	p->P_Aux = 0;
	p->AE = 0;
	p->name = 0;
	p->P_LiteralString = 0;
	p->mode = Mode;
	p->line_number = P_Tokeniser->LineNumber;
	p->type = type;
	return p;
}

symbol * Analyser::NewLiteralString() 
{
	LiteralString * p = static_cast<LiteralString *>(SAlloc::M(sizeof(LiteralString)));
	p->b = copy_b(P_Tokeniser->P_b);
	p->next = literalstrings;
	literalstrings = p;
	return p->b;
}

Node * Analyser::ReadLiteralString() 
{
	Node * p = NewNode(c_literalstring);
	p->P_LiteralString = NewLiteralString();
	return p;
}

typedef enum {
	e_token_omitted = 0,
	e_unexpected_token = 1,
	e_string_omitted = 2,
	e_unexpected_token_in_among = 3,
	/* For codes above here, report "after " t->previous_token after the error. */
	e_unresolved_substring = 14,
	e_not_allowed_inside_reverse = 15,
	e_empty_grouping = 16,
	e_already_backwards = 17,
	e_empty_among = 18,
	e_adjacent_bracketed_in_among = 19,
	e_substring_preceded_by_substring = 20,
	/* For codes below here, tokeniser->b is printed before the error. */
	e_redeclared = 30,
	e_undeclared = 31,
	e_declared_as_different_mode = 32,
	e_not_of_type_x = 33,
	e_not_of_type_string_or_integer = 34,
	e_misplaced = 35,
	e_redefined = 36,
	e_misused = 37
} error_code;

/* recursive usage: */

static void read_program_(Analyser * a, int terminator);
static Node * read_C(Analyser * a);
static Node * C_style(Analyser * a, const char * s, int token);

static void print_node_(const Node * p, int n, const char * s) 
{
	if(p) {
		for(int i = 0; i < n; i++) 
			fputs(i == n - 1 ? s : "  ", stdout);
		printf("%s ", name_of_token(p->type));
		if(p->name) 
			report_b(stdout, p->name->b);
		if(p->P_LiteralString) {
			printf("'");
			report_b(stdout, p->P_LiteralString);
			printf("'");
		}
		printf("\n");
		print_node_(p->AE, n+1, "# "); // @recursion
		print_node_(p->P_Left, n+1, "  "); // @recursion
		print_node_(p->P_Aux, n+1, "@ "); // @recursion
		print_node_(p->P_Right, n, "  "); // @recursion
	}
}

extern void print_program(Analyser * a) 
{
	print_node_(a->P_Program, 0, "  ");
}

static const char * name_of_mode(int n) 
{
	switch(n) {
		case m_backward: return "string backward";
		case m_forward:  return "string forward";
		    /*  case m_integer:  return "integer";  */
	}
	slfprintf_stderr("Invalid mode %d in name_of_mode()\n", n);
	exit(1);
}

static const char * name_of_type(int n) 
{
	switch(n) {
		case 's': return "string";
		case 'i': return "integer";
		case 'r': return "routine";
		case 'R': return "routine or grouping";
		case 'g': return "grouping";
	}
	slfprintf_stderr("Invalid type %d in name_of_type()\n", n);
	exit(1);
}

static const char * name_of_name_type(int code) 
{
	switch(code) {
		case t_string: return "string";
		case t_boolean: return "boolean";
		case t_integer: return "integer";
		case t_routine: return "routine";
		case t_external: return "external";
		case t_grouping: return "grouping";
	}
	slfprintf_stderr("Invalid type code %d in name_of_name_type()\n", code);
	exit(1);
}

void Analyser::CountError()
{
	Tokeniser * t = P_Tokeniser;
	if(t->ErrCount >= 20) {
		slfprintf_stderr("... etc\n"); 
		exit(1);
	}
	t->ErrCount++;
}

static void error2(Analyser * a, error_code n, int x) 
{
	Tokeniser * t = a->P_Tokeniser;
	a->CountError();
	slfprintf_stderr("%s:%d: ", t->P_FileName, t->LineNumber);
	if((int)n >= (int)e_redeclared) 
		report_b(stderr, t->P_b);
	switch(n) {
		case e_token_omitted:
		    slfprintf_stderr("%s omitted", name_of_token(t->Omission)); break;
		case e_unexpected_token_in_among:
		    slfprintf_stderr("in among(...), ");
		/* fall through */
		case e_unexpected_token:
		    slfprintf_stderr("unexpected %s", name_of_token(t->Token));
		    if(t->Token == c_number) 
				slfprintf_stderr(" %d", t->Number);
		    if(t->Token == c_name) {
			    slfprintf_stderr(" ");
			    report_b(stderr, t->P_b);
		    }
		    break;
		case e_string_omitted: slfprintf_stderr("string omitted"); break;
		case e_unresolved_substring: slfprintf_stderr("unresolved substring on line %d", x); break;
		case e_not_allowed_inside_reverse: slfprintf_stderr("%s not allowed inside reverse(...)", name_of_token(t->Token)); break;
		case e_empty_grouping: slfprintf_stderr("empty grouping"); break;
		case e_already_backwards: slfprintf_stderr("backwards used when already in this mode"); break;
		case e_empty_among: slfprintf_stderr("empty among(...)"); break;
		case e_adjacent_bracketed_in_among: slfprintf_stderr("two adjacent bracketed expressions in among(...)"); break;
		case e_substring_preceded_by_substring: slfprintf_stderr("substring preceded by another substring on line %d", x); break;
		case e_redeclared: slfprintf_stderr(" re-declared"); break;
		case e_undeclared: slfprintf_stderr(" undeclared"); break;
		case e_declared_as_different_mode: slfprintf_stderr(" declared as %s mode; used as %s mode", name_of_mode(a->Mode), name_of_mode(x)); break;
		case e_not_of_type_x: slfprintf_stderr(" not of type %s", name_of_type(x)); break;
		case e_not_of_type_string_or_integer: slfprintf_stderr(" not of type string or integer"); break;
		case e_misplaced: slfprintf_stderr(" misplaced"); break;
		case e_redefined: slfprintf_stderr(" redefined"); break;
		case e_misused: slfprintf_stderr(" mis-used as %s mode", name_of_mode(x)); break;
	}
	if((int)n < (int)e_unresolved_substring && t->PreviousToken > 0)
		slfprintf_stderr(" after %s", name_of_token(t->PreviousToken));
	slfprintf_stderr("\n");
}

static void error(Analyser * a, error_code n) { error2(a, n, 0); }

static void error4(Analyser * a, Name * q) 
{
	a->CountError();
	slfprintf_stderr("%s:%d: ", a->P_Tokeniser->P_FileName, q->P_Used->line_number);
	report_b(stderr, q->b);
	slfprintf_stderr(" undefined\n");
}

static void omission_error(Analyser * a, int n) 
{
	a->P_Tokeniser->Omission = n;
	error(a, e_token_omitted);
}

static int check_token(Analyser * a, int code) 
{
	Tokeniser * t = a->P_Tokeniser;
	if(t->Token != code) {
		omission_error(a, code); return false;
	}
	return true;
}

static int get_token(Analyser * a, int code) 
{
	Tokeniser * t = a->P_Tokeniser;
	t->ReadToken();
	{
		int x = check_token(a, code);
		if(!x) 
			t->TokenHeld = true;
		return x;
	}
}

Name * Analyser::LookForName()
{
	symbol * q = P_Tokeniser->P_b;
	for(Name * p = P_Names; p; p = p->next) {
		symbol * b = p->b;
		int n = SIZE(b);
		if(n == SIZE(q) && memcmp(q, b, n * sizeof(symbol)) == 0) {
			p->referenced = true;
			return p;
		}
	}
	return 0;
}

Name * Analyser::FindName()
{
	Name * p = LookForName();
	if(p == 0) 
		error(this, e_undeclared);
	return p;
}

static void check_routine_mode(Analyser * a, Name * p, int mode) 
{
	if(p->mode < 0) 
		p->mode = mode; 
	else if(p->mode != mode) 
		error2(a, e_misused, mode);
}

static void check_name_type(Analyser * a, const Name * p, int type) 
{
	switch(type) {
		case 's':
		    if(p->type == t_string) return;
		    break;
		case 'i':
		    if(p->type == t_integer) return;
		    break;
		case 'b':
		    if(p->type == t_boolean) return;
		    break;
		case 'R':
		    if(p->type == t_grouping) return;
		/* FALLTHRU */
		case 'r':
		    if(p->type == t_routine || p->type == t_external) return;
		    break;
		case 'g':
		    if(p->type == t_grouping) return;
		    break;
	}
	error2(a, e_not_of_type_x, type);
}

static void read_names(Analyser * a, int type) 
{
	Tokeniser * t = a->P_Tokeniser;
	if(!get_token(a, c_bra)) 
		return;
	while(true) {
		int token = t->ReadToken();
		switch(token) {
			case c_len: {
			    // Context-sensitive token - once declared as a name, it loses
			    // its special meaning, for compatibility with older versions of snowball.
			    static const symbol c_len_lit[] = { 'l', 'e', 'n' };
			    MOVE_TO_B(t->P_b, c_len_lit);
			    goto handle_as_name;
		    }
			case c_lenof: {
			    // Context-sensitive token - once declared as a name, it loses
			    // its special meaning, for compatibility with older versions of snowball.
			    static const symbol c_lenof_lit[] = { 'l', 'e', 'n', 'o', 'f' };
			    MOVE_TO_B(t->P_b, c_lenof_lit);
			    goto handle_as_name;
		    }
			case c_name:
handle_as_name:
			    if(a->LookForName() != 0) 
					error(a, e_redeclared); 
				else {
				    Name * p = static_cast<Name *>(SAlloc::M(sizeof(Name)));
				    p->b = copy_b(t->P_b);
				    p->type = type;
				    p->mode = -1; /* routines, externals */
				    // We defer assigning counts until after we've eliminated
				    // variables whose values are never used.
				    p->count = -1;
				    p->referenced = false;
				    p->used_in_among = false;
				    p->P_Used = 0;
				    p->value_used = false;
				    p->initialised = false;
				    p->used_in_definition = false;
				    p->local_to = 0;
				    p->grouping = 0;
				    p->definition = 0;
				    p->declaration_line_number = t->LineNumber;
				    p->next = a->P_Names;
				    a->P_Names = p;
				    if(token != c_name) {
					    disable_token(t, token);
				    }
			    }
			    break;
			default:
			    if(!check_token(a, c_ket)) 
					t->TokenHeld = true;
			    return;
		}
	}
}

static int read_AE_test(Analyser * a) 
{
	Tokeniser * t = a->P_Tokeniser;
	switch(t->ReadToken()) {
		case c_assign: return c_mathassign;
		case c_plusassign:
		case c_minusassign:
		case c_multiplyassign:
		case c_divideassign:
		case c_eq:
		case c_ne:
		case c_gr:
		case c_ge:
		case c_ls:
		case c_le: return t->Token;
		default: 
			error(a, e_unexpected_token); 
			t->TokenHeld = true; 
			return c_eq;
	}
}

static int binding(int t) 
{
	switch(t) {
		case c_plus: case c_minus: return 1;
		case c_multiply: case c_divide: return 2;
		default: return -2;
	}
}

static void mark_used_in(Analyser * a, Name * q, const Node * p) 
{
	if(a && q) {
		if(!q->P_Used) {
			q->P_Used = p;
			q->local_to = a->P_ProgramEnd->name;
		}
		else if(q->local_to) {
			if(q->local_to != a->P_ProgramEnd->name)
				q->local_to = NULL; // Used in more than one routine/external
		}
	}
}

static void name_to_node(Analyser * a, Node * p, int type) 
{
	Name * q = a->FindName();
	if(q) {
		check_name_type(a, q, type);
		mark_used_in(a, q, p);
	}
	p->name = q;
}

static Node * read_AE(Analyser * a, int B) 
{
	Tokeniser * t = a->P_Tokeniser;
	Node * p;
	Node * q;
	switch(t->ReadToken()) {
		case c_minus: /* monadic */
		    q = read_AE(a, 100);
		    if(q->type == c_neg) {
			    /* Optimise away double negation, which avoids generators
			     * having to worry about generating "--" (decrement operator
			     * in many languages).
			     */
			    p = q->P_Right;
			    /* Don't free q, it's in the linked list a->nodes. */
			    break;
		    }
		    p = a->NewNode(c_neg);
		    p->P_Right = q;
		    break;
		case c_bra:
		    p = read_AE(a, 0);
		    get_token(a, c_ket);
		    break;
		case c_name:
		    p = a->NewNode(c_name);
		    name_to_node(a, p, 'i');
		    if(p->name) 
				p->name->value_used = true;
		    break;
		case c_maxint:
		case c_minint:
		    a->IntLimitsUsed = true;
			// fall through
		case c_cursor:
		case c_limit:
		case c_len:
		case c_size:
		    p = a->NewNode(t->Token);
		    break;
		case c_number:
		    p = a->NewNode(c_number);
		    p->number = t->Number;
		    break;
		case c_lenof:
		case c_sizeof:
		    p = C_style(a, "s", t->Token);
		    break;
		default:
		    error(a, e_unexpected_token);
		    t->TokenHeld = true;
		    return 0;
	}
	while(true) {
		int token = t->ReadToken();
		int b = binding(token);
		if(binding(token) <= B) {
			t->TokenHeld = true;
			return p;
		}
		q = a->NewNode(token);
		q->P_Left = p;
		q->P_Right = read_AE(a, b);
		p = q;
	}
}

static Node * read_C_connection(Analyser * a, Node * q, int op) 
{
	Tokeniser * t = a->P_Tokeniser;
	Node * p = a->NewNode(op);
	Node * p_end = q;
	p->P_Left = q;
	do {
		q = read_C(a);
		p_end->P_Right = q; 
		p_end = q;
	} while(t->ReadToken() == op);
	t->TokenHeld = true;
	return p;
}

static Node * read_C_list(Analyser * a) 
{
	Tokeniser * t = a->P_Tokeniser;
	Node * p = a->NewNode(c_bra);
	Node * p_end = 0;
	while(true) {
		int token = t->ReadToken();
		if(token == c_ket) return p;
		if(token < 0) {
			omission_error(a, c_ket); return p;
		}
		t->TokenHeld = true;
		{
			Node * q = read_C(a);
			while(true) {
				token = t->ReadToken();
				if(token != c_and && token != c_or) {
					t->TokenHeld = true;
					break;
				}
				q = read_C_connection(a, q, token);
			}
			if(p_end == 0)
				p->P_Left = q; 
			else 
				p_end->P_Right = q;
			p_end = q;
		}
	}
}

static Node * C_style(Analyser * a, const char * s, int token) 
{
	Node * p = a->NewNode(token);
	for(int i = 0; s[i] != 0; i++) {
		switch(s[i]) {
			case 'C':
			    p->P_Left = read_C(a); continue;
			case 'D':
			    p->P_Aux = read_C(a); continue;
			case 'A':
			    p->AE = read_AE(a, 0); continue;
			case 'f':
			    get_token(a, c_for); continue;
			case 'S':
				{
					int str_token = a->P_Tokeniser->ReadToken();
					if(str_token == c_name) 
						name_to_node(a, p, 's');
					else if(str_token == c_literalstring) 
						p->P_LiteralString = a->NewLiteralString();
					else 
						error(a, e_string_omitted);
				}
			    continue;
			case 'b':
			case 's':
			case 'i':
			    if(get_token(a, c_name)) name_to_node(a, p, s[i]);
			    continue;
		}
	}
	return p;
}

static void reverse_b(symbol * b) 
{
	int i = 0; 
	int j = SIZE(b) - 1;
	while(i < j) {
		int ch1 = b[i]; 
		int ch2 = b[j];
		b[i++] = ch2; 
		b[j--] = ch1;
	}
}

static int compare_amongvec(const void * pv, const void * qv) 
{
	const AmongVec * p = (const AmongVec*)pv;
	const AmongVec * q = (const AmongVec*)qv;
	symbol * b_p = p->b; int p_size = p->size;
	symbol * b_q = q->b; int q_size = q->size;
	int smaller_size = p_size < q_size ? p_size : q_size;
	int i;
	for(i = 0; i < smaller_size; i++)
		if(b_p[i] != b_q[i]) return b_p[i] - b_q[i];
	if(p_size - q_size)
		return p_size - q_size;
	return p->line_number - q->line_number;
}

#define PTR_NULL_CHECK(P, Q) do { \
		if((Q) == NULL) { \
			if((P) != NULL) return 1; \
		} else { \
			if((P) == NULL) return -1; \
		} \
} while(0)

static int compare_node(const Node * p, const Node * q) {
	PTR_NULL_CHECK(p, q);
	if(q == NULL) {
		/* p must be NULL too. */
		return 0;
	}

	if(p->type != q->type) return p->type > q->type ? 1 : -1;
	if(p->mode != q->mode) return p->mode > q->mode ? 1 : -1;
	if(p->type == c_number) {
		if(p->number != q->number)
			return p->number > q->number ? 1 : -1;
	}
	PTR_NULL_CHECK(p->P_Left, q->P_Left);
	if(p->P_Left) {
		int r = compare_node(p->P_Left, q->P_Left);
		if(r != 0) 
			return r;
	}
	PTR_NULL_CHECK(p->AE, q->AE);
	if(p->AE) {
		int r = compare_node(p->AE, q->AE);
		if(r != 0) 
			return r;
	}
	PTR_NULL_CHECK(p->P_Aux, q->P_Aux);
	if(p->P_Aux) {
		int r = compare_node(p->P_Aux, q->P_Aux);
		if(r != 0) 
			return r;
	}
	PTR_NULL_CHECK(p->name, q->name);
	if(p->name) {
		int r;
		if(SIZE(p->name->b) != SIZE(q->name->b)) {
			return SIZE(p->name->b) - SIZE(q->name->b);
		}
		r = memcmp(p->name->b, q->name->b, SIZE(p->name->b) * sizeof(symbol));
		if(r != 0) 
			return r;
	}
	PTR_NULL_CHECK(p->P_LiteralString, q->P_LiteralString);
	if(p->P_LiteralString) {
		int r;
		if(SIZE(p->P_LiteralString) != SIZE(q->P_LiteralString)) {
			return SIZE(p->P_LiteralString) - SIZE(q->P_LiteralString);
		}
		r = memcmp(p->P_LiteralString, q->P_LiteralString, SIZE(p->P_LiteralString) * sizeof(symbol));
		if(r != 0) 
			return r;
	}
	return compare_node(p->P_Right, q->P_Right);
}

static void make_among(Analyser * a, Node * p, Node * substring) 
{
	Among * x = static_cast<Among *>(SAlloc::M(sizeof(Among)));
	NEWVEC(AmongVec, v, p->number);
	Node * q = p->P_Left;
	AmongVec * w0 = v;
	AmongVec * w1 = v;
	int result = 1;
	int direction = substring != 0 ? substring->mode : p->mode;
	int backward = direction == m_backward;
	if(a->P_Amongs == 0) 
		a->P_Amongs = x; 
	else 
		a->P_AmongsEnd->P_Next = x;
	a->P_AmongsEnd = x;
	x->P_Next = 0;
	x->P_Vec = v;
	x->number = a->AmongCount++;
	x->function_count = 0;
	x->starter = 0;
	x->NoCommandCount = 0;
	x->amongvar_needed = false;
	if(q->type == c_bra) {
		x->starter = q; 
		q = q->P_Right;
	}
	while(q) {
		if(q->type == c_literalstring) {
			symbol * b = q->P_LiteralString;
			w1->b = b; /* pointer to case string */
			w1->action = NULL; /* action gets filled in below */
			w1->line_number = q->line_number;
			w1->size = SIZE(b); /* number of characters in string */
			w1->i = -1; /* index of longest substring */
			w1->result = -1; /* number of corresponding case expression */
			if(q->P_Left) {
				Name * function = q->P_Left->name;
				w1->function = function;
				function->used_in_among = true;
				check_routine_mode(a, function, direction);
				x->function_count++;
			}
			else {
				w1->function = 0;
			}
			w1++;
		}
		else if(q->P_Left == 0) {
			// empty command: () 
			w0 = w1;
		}
		else {
			// Check for previous action which is the same as this one and use
			// the same action code if we find one.
			int among_result = -1;
			AmongVec * w;
			for(w = v; w < w0; ++w) {
				if(w->action && compare_node(w->action->P_Left, q->P_Left) == 0) {
					if(w->result <= 0) {
						printf("Among code %d isn't positive\n", w->result);
						exit(1);
					}
					among_result = w->result;
					break;
				}
			}
			if(among_result < 0) {
				among_result = result++;
			}
			while(w0 != w1) {
				w0->action = q;
				w0->result = among_result;
				w0++;
			}
		}
		q = q->P_Right;
	}
	if(w1-v != p->number) {
		slfprintf_stderr("oh! %d %d\n", (int)(w1-v), p->number); exit(1);
	}
	x->CommandCount = result - 1;
	{
		NEWVEC(Node *, commands, x->CommandCount);
		memzero(commands, x->CommandCount * sizeof(Node*));
		for(w0 = v; w0 < w1; w0++) {
			if(w0->result > 0) {
				// result == -1 when there's no command
				if(w0->result > x->CommandCount) {
					slfprintf_stderr("More among codes than expected\n");
					exit(1);
				}
				if(!commands[w0->result-1])
					commands[w0->result-1] = w0->action;
			}
			else {
				++x->NoCommandCount;
			}
			if(backward)
				reverse_b(w0->b);
		}
		x->commands = commands;
	}
	qsort(v, w1 - v, sizeof(AmongVec), compare_amongvec);

	/* the following loop is O(n squared) */
	for(w0 = w1 - 1; w0 >= v; w0--) {
		const symbol * b = w0->b;
		const int size = w0->size;
		for(AmongVec * w = w0 - 1; w >= v; w--) {
			if(w->size < size && memcmp(w->b, b, w->size * sizeof(symbol)) == 0) {
				w0->i = w - v; /* fill in index of longest substring */
				break;
			}
		}
	}
	if(backward) 
		for(w0 = v; w0 < w1; w0++) 
			reverse_b(w0->b);
	for(w0 = v; w0 < w1 - 1; w0++)
		if(w0->size == (w0 + 1)->size && memcmp(w0->b, (w0 + 1)->b, w0->size * sizeof(symbol)) == 0) {
			a->CountError();
			slfprintf_stderr("%s:%d: among(...) has repeated string '", a->P_Tokeniser->P_FileName, (w0 + 1)->line_number);
			report_b(stderr, (w0 + 1)->b);
			slfprintf_stderr("'\n");
			a->CountError();
			slfprintf_stderr("%s:%d: previously seen here\n", a->P_Tokeniser->P_FileName, w0->line_number);
		}

	x->LiteralStringCount = p->number;
	p->among = x;
	x->substring = substring;
	if(substring != 0) substring->among = x;
	if(x->CommandCount > 1 || (x->CommandCount == 1 && x->NoCommandCount > 0) || x->starter != 0) {
		// We need to set among_var rather than just checking if find_among*() returns zero or not.
		x->amongvar_needed = a->AmongVarNeeded = true;
	}
}

static Node * read_among(Analyser * a) 
{
	Tokeniser * t = a->P_Tokeniser;
	Node * p = a->NewNode(c_among);
	Node * p_end = 0;
	int previous_token = -1;
	Node * substring = a->P_Substring;
	a->P_Substring = 0;
	p->number = 0; /* counts the number of literals */
	if(!get_token(a, c_bra)) 
		return p;
	while(true) {
		Node * q;
		int token = t->ReadToken();
		switch(token) {
			case c_literalstring:
			    q = a->ReadLiteralString();
			    if(t->ReadToken() == c_name) {
				    Node * r = a->NewNode(c_name);
				    name_to_node(a, r, 'r');
				    q->P_Left = r;
			    }
			    else 
					t->TokenHeld = true;
			    p->number++; 
				break;
			case c_bra:
			    if(previous_token == c_bra) 
					error(a, e_adjacent_bracketed_in_among);
			    q = read_C_list(a); 
				break;
			default:
			    error(a, e_unexpected_token_in_among);
			    previous_token = token;
			    continue;
			case c_ket:
			    if(p->number == 0) 
					error(a, e_empty_among);
			    if(t->ErrCount == 0) 
					make_among(a, p, substring);
			    return p;
		}
		previous_token = token;
		if(p_end == 0) 
			p->P_Left = q; 
		else 
			p_end->P_Right = q;
		p_end = q;
	}
}

static Node * read_substring(Analyser * a) 
{
	Node * p = a->NewNode(c_substring);
	if(a->P_Substring != 0) 
		error2(a, e_substring_preceded_by_substring, a->P_Substring->line_number);
	a->P_Substring = p;
	return p;
}

static void check_modifyable(Analyser * a) 
{
	if(!a->Modifyable)
		error(a, e_not_allowed_inside_reverse);
}

static Node * read_C(Analyser * a) 
{
	Tokeniser * t = a->P_Tokeniser;
	int token = t->ReadToken();
	switch(token) {
		case c_bra:
		    return read_C_list(a);
		case c_backwards:
	    {
		    const int mode = a->Mode;
		    if(a->Mode == m_backward) 
				error(a, e_already_backwards); 
			else 
				a->Mode = m_backward;
		    {   
				Node * p = C_style(a, "C", token);
				a->Mode = mode;
				return p;
			}
	    }
		case c_reverse:
	    {
		    const int  mode = a->Mode;
		    const bool modifyable = a->Modifyable;
		    a->Modifyable = false;
		    a->Mode = (mode == m_forward) ? m_backward : m_forward;
		    {
			    Node * p = C_style(a, "C", token);
			    a->Mode = mode;
			    a->Modifyable = modifyable;
			    return p;
		    }
	    }
		case c_not:
		case c_try:
		case c_fail:
		case c_test:
		case c_do:
		case c_goto:
		case c_gopast:
		case c_repeat:
		    return C_style(a, "C", token);
		case c_loop:
		case c_atleast:
		    return C_style(a, "AC", token);
		case c_setmark: {
		    Node * n = C_style(a, "i", token);
		    if(n->name) n->name->initialised = true;
		    return n;
	    }
		case c_tomark:
		case c_atmark:
		case c_hop:
		    return C_style(a, "A", token);
		case c_delete:
		    check_modifyable(a);
		/* fall through */
		case c_next:
		case c_tolimit:
		case c_atlimit:
		case c_leftslice:
		case c_rightslice:
		case c_true:
		case c_false:
		case c_debug:
		    return a->NewNode(token);
		case c_assignto:
		case c_sliceto: 
			{
				check_modifyable(a);
				Node * n = C_style(a, "s", token);
				if(n->name) 
					n->name->initialised = true;
				return n;
			}
		case c_assign:
		case c_insert:
		case c_attach:
		case c_slicefrom: 
			{
				check_modifyable(a);
				Node * n = C_style(a, "S", token);
				if(n->name) 
					n->name->value_used = true;
				return n;
			}
		case c_setlimit:
		    return C_style(a, "CfD", token);
		case c_set:
		case c_unset: 
			{
				Node * n = C_style(a, "b", token);
				if(n->name) 
					n->name->initialised = true;
				return n;
			}
		case c_dollar: 
			{
				Tokeniser * t = a->P_Tokeniser;
				t->ReadToken();
				if(t->Token == c_bra) {
					// Handle newer $(AE REL_OP AE) syntax. 
					Node * n = read_AE(a, 0);
					t->ReadToken();
					switch(t->Token) {
						case c_eq:
						case c_ne:
						case c_gr:
						case c_ge:
						case c_ls:
						case c_le: 
							{
								Node * lhs = n;
								n = a->NewNode(t->Token);
								n->P_Left = lhs;
								n->AE = read_AE(a, 0);
								get_token(a, c_ket);
							}
							break;
						default:
							error(a, e_unexpected_token);
							t->TokenHeld = true;
							break;
					}
					return n;
				}
				if(t->Token == c_name) {
					Node * p;
					Name * q = a->FindName();
					const int  mode = a->Mode;
					const bool modifyable = a->Modifyable;
					if(q && q->type == t_string) {
						// Assume for now that $ on string both initialises and uses the string variable.  FIXME: Can we do better?
						q->initialised = true;
						q->value_used = true;
						a->Mode = m_forward;
						a->Modifyable = true;
						p = a->NewNode(c_dollar);
						p->P_Left = read_C(a);
						p->name = q;
					}
					else {
						if(q && q->type != t_integer) {
							/* If $ is used on an unknown name or a name which
							 * isn't a string or an integer then we assume the
							 * unknown name is an integer as $ is used more often
							 * on integers than strings, so hopefully this it less
							 * likely to cause an error avalanche.
							 *
							 * For an unknown name, we'll already have reported an
							 * error.
							 */
							error(a, e_not_of_type_string_or_integer);
							q = NULL;
						}
						p = a->NewNode(read_AE_test(a));
						p->AE = read_AE(a, 0);

						if(q) {
							switch(p->type) {
								case c_mathassign:
								q->initialised = true;
								p->name = q;
								break;
								default:
								/* +=, etc don't "initialise" as they only
								 * amend an existing value.  Similarly, they
								 * don't count as using the value.
								 */
								p->name = q;
								break;
								case c_eq:
								case c_ne:
								case c_gr:
								case c_ge:
								case c_ls:
								case c_le:
								p->P_Left = a->NewNode(c_name);
								p->P_Left->name = q;
								q->value_used = true;
								break;
							}
						}
					}
					mark_used_in(a, q, p);
					a->Mode = mode;
					a->Modifyable = modifyable;
					return p;
				}
				error(a, e_unexpected_token);
				t->TokenHeld = true;
				return a->NewNode(c_dollar);
			}
		case c_name:
	    {
		    Name * q = a->FindName();
		    Node * p = a->NewNode(c_name);
		    if(q) {
			    mark_used_in(a, q, p);
			    switch(q->type) {
				    case t_boolean:
					p->type = c_booltest;
					q->value_used = true;
					break;
				    case t_integer:
					error(a, e_misplaced); /* integer name misplaced */
					break;
				    case t_string:
					q->value_used = true;
					break;
				    case t_routine:
				    case t_external:
					p->type = c_call;
					check_routine_mode(a, q, a->Mode);
					break;
				    case t_grouping:
					p->type = c_grouping; break;
			    }
		    }
		    p->name = q;
		    return p;
	    }
		case c_non:
	    {
		    Node * p = a->NewNode(token);
		    t->ReadToken();
		    if(t->Token == c_minus) 
				t->ReadToken();
		    if(!check_token(a, c_name)) {
			    omission_error(a, c_name); return p;
		    }
		    name_to_node(a, p, 'g');
		    return p;
	    }
		case c_literalstring: return a->ReadLiteralString();
		case c_among: return read_among(a);
		case c_substring: return read_substring(a);
		default: error(a, e_unexpected_token); return 0;
	}
}

static int next_symbol(symbol * p, symbol * W, int utf8) 
{
	if(utf8) {
		int ch;
		int j = get_utf8(p, &ch);
		W[0] = ch; 
		return j;
	}
	else {
		W[0] = p[0]; 
		return 1;
	}
}

static symbol * alter_grouping(symbol * p, symbol * q, int style, int utf8) {
	int j = 0;
	symbol W[1];
	int width;
	if(style == c_plus) {
		while(j < SIZE(q)) {
			width = next_symbol(q + j, W, utf8);
			p = add_to_b(p, 1, W);
			j += width;
		}
	}
	else {
		while(j < SIZE(q)) {
			int i;
			width = next_symbol(q + j, W, utf8);
			for(i = 0; i < SIZE(p); i++) {
				if(p[i] == W[0]) {
					memmove(p + i, p + i + 1, (SIZE(p) - i - 1) * sizeof(symbol));
					SIZE(p)--;
				}
			}
			j += width;
		}
	}
	return p;
}

static void read_define_grouping(Analyser * a, Name * q) 
{
	Tokeniser * t = a->P_Tokeniser;
	int style = c_plus;
	{
		Grouping * p = static_cast<Grouping *>(SAlloc::M(sizeof(Grouping)));
		if(a->P_Groupings == 0) 
			a->P_Groupings = p; 
		else 
			a->P_GroupingsEnd->next = p;
		a->P_GroupingsEnd = p;
		if(q) 
			q->grouping = p;
		p->next = 0;
		p->name = q;
		p->line_number = a->P_Tokeniser->LineNumber;
		p->b = create_b(0);
		while(true) {
			switch(t->ReadToken()) {
				case c_name:
			    {
				    Name * r = a->FindName();
				    if(r) {
					    check_name_type(a, r, 'g');
					    p->b = alter_grouping(p->b, r->grouping->b, style, false);
					    r->used_in_definition = true;
				    }
			    }
			    break;
				case c_literalstring:
				    p->b = alter_grouping(p->b, t->P_b, style, (a->Encoding == ENC_UTF8));
				    break;
				default: error(a, e_unexpected_token); return;
			}
			switch(t->ReadToken()) {
				case c_plus:
				case c_minus: style = t->Token; break;
				default: goto label0;
			}
		}
label0:
		{
			int i;
			int max = 0;
			int min = 1<<16;
			for(i = 0; i < SIZE(p->b); i++) {
				SETMAX(max, p->b[i]);
				SETMIN(min, p->b[i]);
			}
			p->largest_ch = max;
			p->smallest_ch = min;
			if(min == 1<<16) 
				error(a, e_empty_grouping);
		}
		t->TokenHeld = true; 
		return;
	}
}

static void read_define_routine(Analyser * a, Name * q) 
{
	Node * p = a->NewNode(c_define);
	a->AmongVarNeeded = false;
	if(q) {
		check_name_type(a, q, 'R');
		if(q->definition != 0) error(a, e_redefined);
		if(q->mode < 0) 
			q->mode = a->Mode;
		else if(q->mode != a->Mode) 
			error2(a, e_declared_as_different_mode, q->mode);
	}
	p->name = q;
	if(a->P_Program == 0) 
		a->P_Program = p; 
	else 
		a->P_ProgramEnd->P_Right = p;
	a->P_ProgramEnd = p;
	get_token(a, c_as);
	p->P_Left = read_C(a);
	if(q) 
		q->definition = p->P_Left;
	if(a->P_Substring != 0) {
		error2(a, e_unresolved_substring, a->P_Substring->line_number);
		a->P_Substring = 0;
	}
	p->AmongVarNeeded = a->AmongVarNeeded;
}

static void read_define(Analyser * a) 
{
	if(get_token(a, c_name)) {
		Name * q = a->FindName();
		int type;
		if(q) {
			type = q->type;
		}
		else {
			// No declaration, so sniff next token - if it is 'as' then parse as a routine, otherwise as a grouping.
			type = (a->P_Tokeniser->ReadToken() == c_as) ? t_routine : t_grouping;
			a->P_Tokeniser->TokenHeld = true;
		}
		if(type == t_grouping) {
			read_define_grouping(a, q);
		}
		else {
			read_define_routine(a, q);
		}
	}
}

static void read_backwardmode(Analyser * a) 
{
	int mode = a->Mode;
	a->Mode = m_backward;
	if(get_token(a, c_bra)) {
		read_program_(a, c_ket);
		check_token(a, c_ket);
	}
	a->Mode = mode;
}

static void read_program_(Analyser * a, int terminator) 
{
	Tokeniser * t = a->P_Tokeniser;
	while(true) {
		switch(t->ReadToken()) {
			case c_strings:     read_names(a, t_string); break;
			case c_booleans:    read_names(a, t_boolean); break;
			case c_integers:    read_names(a, t_integer); break;
			case c_routines:    read_names(a, t_routine); break;
			case c_externals:   read_names(a, t_external); break;
			case c_groupings:   read_names(a, t_grouping); break;
			case c_define:      read_define(a); break;
			case c_backwardmode: read_backwardmode(a); break;
			case c_ket:
			    if(terminator == c_ket) return;
			/* fall through */
			default:
			    error(a, e_unexpected_token); break;
			case -1:
			    if(terminator >= 0) omission_error(a, c_ket);
			    return;
		}
	}
}

static void remove_dead_assignments(Node * p, Name * q) 
{
	if(p) {
		if(p->name == q) {
			switch(p->type) {
				case c_assignto:
				case c_sliceto:
				case c_mathassign:
				case c_plusassign:
				case c_minusassign:
				case c_multiplyassign:
				case c_divideassign:
				case c_setmark:
				case c_set:
				case c_unset:
				case c_dollar:
					/* c_true is a no-op. */
					p->type = c_true;
					break;
				default:
					/* There are no read accesses to this variable, so any
					 * references must be assignments.
					 */
					slfprintf_stderr("Unhandled type of dead assignment via %s\n", name_of_token(p->type));
					exit(1);
			}
		}
		remove_dead_assignments(p->AE, q); // @recursion
		remove_dead_assignments(p->P_Left, q); // @recursion
		remove_dead_assignments(p->P_Aux, q); // @recursion
		remove_dead_assignments(p->P_Right, q); // @recursion
	}
}

void read_program(Analyser * a) 
{
	read_program_(a, -1);
	{
		Name * q = a->P_Names;
		while(q) {
			switch(q->type) {
				case t_external: case t_routine:
				    if(q->P_Used && q->definition == 0) 
						error4(a, q);
				    break;
				case t_grouping:
				    if(q->P_Used && q->grouping == 0) 
						error4(a, q);
				    break;
			}
			q = q->next;
		}
	}
	if(a->P_Tokeniser->ErrCount == 0) {
		Name * q = a->P_Names;
		Name ** ptr = &(a->P_Names);
		while(q) {
			if(!q->referenced) {
				slfprintf_stderr("%s:%d: warning: %s '", a->P_Tokeniser->P_FileName, q->declaration_line_number, name_of_name_type(q->type));
				report_b(stderr, q->b);
				if(q->type == t_routine || q->type == t_external || q->type == t_grouping) {
					slfprintf_stderr("' declared but not defined\n");
				}
				else {
					slfprintf_stderr("' defined but not used\n");
					q = q->next;
					*ptr = q;
					continue;
				}
			}
			else if(q->type == t_routine || q->type == t_grouping) {
				// It's OK to define a grouping but only use it to define other groupings.
				if(!q->P_Used && !q->used_in_definition) {
					const int line_num = (q->type == t_routine) ? q->definition->line_number : q->grouping->line_number;
					slfprintf_stderr("%s:%d: warning: %s '", a->P_Tokeniser->P_FileName, line_num, name_of_name_type(q->type));
					report_b(stderr, q->b);
					slfprintf_stderr("' defined but not used\n");
				}
			}
			else if(q->type == t_external) {
				/* Unused is OK. */
			}
			else if(!q->initialised) {
				slfprintf_stderr("%s:%d: warning: %s '", a->P_Tokeniser->P_FileName, q->declaration_line_number, name_of_name_type(q->type));
				report_b(stderr, q->b);
				slfprintf_stderr("' is never initialised\n");
			}
			else if(!q->value_used) {
				slfprintf_stderr("%s:%d: warning: %s '", a->P_Tokeniser->P_FileName, q->declaration_line_number, name_of_name_type(q->type));
				report_b(stderr, q->b);
				slfprintf_stderr("' is set but never used\n");
				remove_dead_assignments(a->P_Program, q);
				q = q->next;
				*ptr = q;
				continue;
			}
			ptr = &(q->next);
			q = q->next;
		}
		{
			// Now we've eliminated variables whose values are never used we
			// can number the variables, which is used by some generators.
			int * p_name_count = a->NameCount;
			for(Name * n = a->P_Names; n; n = n->next) {
				n->count = p_name_count[n->type]++;
			}
		}
	}
}

extern void close_analyser(Analyser * a) 
{
	if(a) {
		{
			for(Node * q = a->P_Nodes; q;) {
				Node * q_next = q->P_Next;
				FREE(q);
				q = q_next;
			}
		}
		{
			for(Name * q = a->P_Names; q;) {
				Name * q_next = q->next;
				lose_b(q->b); 
				FREE(q);
				q = q_next;
			}
		}
		{
			for(LiteralString * q = a->literalstrings; q;) {
				LiteralString * q_next = q->next;
				lose_b(q->b); 
				FREE(q);
				q = q_next;
			}
		}
		{
			for(Among * q = a->P_Amongs; q;) {
				Among * q_next = q->P_Next;
				FREE(q->P_Vec);
				FREE(q->commands);
				FREE(q);
				q = q_next;
			}
		}
		{
			for(Grouping * q = a->P_Groupings; q;) {
				Grouping * q_next = q->next;
				lose_b(q->b); 
				FREE(q);
				q = q_next;
			}
		}
		FREE(a);
	}
}
