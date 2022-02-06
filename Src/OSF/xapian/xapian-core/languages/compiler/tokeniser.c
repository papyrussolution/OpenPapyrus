//
//
#include <slib.h>
#include "header.h"
#pragma hdrstop

struct system_word {
	int s_size;  /* size of system word */
	const byte * s; /* pointer to the system word */
	int code;    /* its internal code */
};

/* ASCII collating assumed in syswords.c */

#include "syswords.h"

#define INITIAL_INPUT_BUFFER_SIZE 8192

//static int smaller(int a, int b) { return a < b ? a : b; }

extern symbol * get_input(const char * filename) 
{
	symbol * u = 0;
	FILE * input = fopen(filename, "r");
	if(input) {
		u = create_b(INITIAL_INPUT_BUFFER_SIZE);
		int size = 0;
		while(true) {
			int ch = getc(input);
			if(ch == EOF) 
				break;
			if(size >= CAPACITY(u)) 
				u = increase_capacity(u, size);
			u[size++] = ch;
		}
		fclose(input);
		SIZE(u) = size;
	}
	return u;
}

static int compare_words(int m, symbol * p, int n, const byte * q) 
{
	if(m != n) 
		return m - n;
	else {
		for(int i = 0; i < n; i++) {
			const int diff = p[i] - q[i];
			if(diff) 
				return diff;
		}
		return 0;
	}
}

static int find_word(int n, symbol * p) 
{
	int i = 0; 
	int j = vocab->code;
	do {
		int k = i + (j - i)/2;
		const struct system_word * w = vocab + k;
		const int diff = compare_words(n, p, w->s_size, w->s);
		if(diff == 0) 
			return w->code;
		else if(diff < 0) 
			j = k; 
		else 
			i = k;
	} while(j - i != 1);
	return -1;
}

static int get_number(int n, symbol * p) 
{
	int x = 0;
	for(int i = 0; i < n; i++) 
		x = 10*x + p[i] - '0';
	return x;
}

static int eq_s(Tokeniser * t, const char * s) 
{
	const int l = strlen(s);
	if(SIZE(t->P_Symb) - t->C < l) 
		return false;
	else {
		{
			for(int i = 0; i < l; i++) 
				if(t->P_Symb[t->C + i] != s[i]) 
					return false;
		}
		t->C += l; 
		return true;
	}
}

static int read_literal_string(Tokeniser * t, int c) 
{
	symbol * p = t->P_Symb;
	int ch;
	SIZE(t->P_b) = 0;
	while(true) {
		if(c >= SIZE(p)) {
			t->Error2("'"); 
			return c;
		}
		ch = p[c];
		if(ch == '\n') {
			t->Error1("string not terminated"); return c;
		}
		c++;
		if(ch == t->m_start) {
			// Inside insert characters
			int c0 = c;
			int newlines = false; /* no newlines as yet */
			int black_found = false; /* no printing chars as yet */
			while(true) {
				if(c >= SIZE(p)) {
					t->Error2("'"); return c;
				}
				ch = p[c]; c++;
				if(ch == t->m_end) 
					break;
				if(!t->WhiteSpace(ch)) 
					black_found = true;
				if(ch == '\n') 
					newlines = true;
				if(newlines && black_found) {
					t->Error1("string not terminated");
					return c;
				}
			}
			if(!newlines) {
				int n = c - c0 - 1; /* macro size */
				int firstch = p[c0];
				const symbol * q = t->FindInM(n, p + c0);
				if(q == 0) {
					if(n == 1 && (firstch == '\'' || firstch == t->m_start))
						t->P_b = add_to_b(t->P_b, 1, p + c0);
					else if(n >= 3 && firstch == 'U' && p[c0 + 1] == '+') {
						int codepoint = 0;
						int x;
						if(t->UPlusMode == UPLUS_DEFINED) {
							// See if found with xxxx upper-cased. 
							symbol * uc = create_b(n);
							for(int i = 0; i != n; ++i) {
								uc[i] = toupper(p[c0 + i]);
							}
							q = t->FindInM(n, uc);
							lose_b(uc);
							if(q != 0) {
								t->P_b = add_to_b(t->P_b, SIZE(q), q);
								continue;
							}
							t->Error1("Some U+xxxx stringdefs seen but not this one");
						}
						else {
							t->UPlusMode = UPLUS_UNICODE;
						}
						for(x = c0 + 2; x != c - 1; ++x) {
							int hex = Tokeniser::HexToNum(p[x]);
							if(hex < 0) {
								t->Error1("Bad hex digit following U+");
								break;
							}
							codepoint = (codepoint << 4) | hex;
						}
						if(t->Encoding == ENC_UTF8) {
							if(codepoint < 0 || codepoint > 0x01ffff) {
								t->Error1("character values exceed 0x01ffff");
							}
							// Ensure there's enough space for a max length UTF-8 sequence. 
							if(CAPACITY(t->P_b) < SIZE(t->P_b) + 3) {
								t->P_b = increase_capacity(t->P_b, 3);
							}
							SIZE(t->P_b) += put_utf8(codepoint, t->P_b + SIZE(t->P_b));
						}
						else {
							if(t->Encoding == ENC_SINGLEBYTE) {
								/* Only ISO-8859-1 is handled this way - for
								 * other single-byte character sets you need
								 * stringdef all the U+xxxx codes you use
								 * like - e.g.:
								 *
								 * stringdef U+0171   hex 'FB'
								 */
								if(codepoint < 0 || codepoint > 0xff) {
									t->Error1("character values exceed 256");
								}
							}
							else {
								if(codepoint < 0 || codepoint > 0xffff) {
									t->Error1("character values exceed 64K");
								}
							}
							symbol sym = codepoint;
							t->P_b = add_to_b(t->P_b, 1, &sym);
						}
					}
					else
						t->Error("string macro '", n, p + c0, "' undeclared");
				}
				else
					t->P_b = add_to_b(t->P_b, SIZE(q), q);
			}
		}
		else {
			if(ch == '\'') return c;
			if(ch < 0 || ch >= 0x80) {
				if(t->Encoding != ENC_WIDECHARS) {
					/* We don't really want people using non-ASCII literal
					 * strings, but historically it's worked for single-byte
					 * and UTF-8 if the source encoding matches what the
					 * generated stemmer works in and it seems unfair to just
					 * suddenly make this a hard error.`
					 */
					slfprintf_stderr("%s:%d: warning: Non-ASCII literal strings aren't portable - use stringdef instead\n", t->P_FileName, t->LineNumber);
				}
				else {
					t->Error1("Non-ASCII literal strings aren't portable - use stringdef instead");
				}
			}
			t->P_b = add_to_b(t->P_b, 1, p + c - 1);
		}
	}
}

extern const char * name_of_token(int code) 
{
	for(int i = 1; i < vocab->code; i++)
		if((vocab + i)->code == code) 
			return (const char*)(vocab + i)->s;
	switch(code) {
		case c_mathassign:   return "=";
		case c_name:         return "name";
		case c_number:       return "number";
		case c_literalstring: return "literal";
		case c_neg:          return "neg";
		case c_grouping:     return "grouping";
		case c_call:         return "call";
		case c_booltest:     return "Boolean test";
		case -2:             return "start of text";
		case -1:             return "end of text";
		default:             return "?";
	}
}

extern void disable_token(Tokeniser * t, int code) 
{
	t->TokenDisabled[code] = 1;
}

extern Tokeniser * create_tokeniser(symbol * p, const char * file) 
{
	NEW(Tokeniser, t);
	t->P_Next = 0;
	t->P_Symb = p;
	t->C = 0;
	t->P_FileName = file;
	t->FileNeedsFreeing = false;
	t->LineNumber = 1;
	t->P_b = create_b(0);
	t->P_b2 = create_b(0);
	t->m_start = -1;
	t->m_pairs = 0;
	t->GetDepth = 0;
	t->ErrCount = 0;
	t->TokenHeld = false;
	t->Token = -2;
	t->PreviousToken = -2;
	t->UPlusMode = UPLUS_NONE;
	memzero(t->TokenDisabled, sizeof(t->TokenDisabled));
	return t;
}

extern void close_tokeniser(Tokeniser * t) 
{
	if(t) {
		lose_b(t->P_b);
		lose_b(t->P_b2);
		{
			for(MPair * q = t->m_pairs; q;) {
				MPair * q_next = q->next;
				lose_b(q->name);
				lose_b(q->value);
				FREE(q);
				q = q_next;
			}
		}
		{
			for(Input * q = t->P_Next; q;) {
				Input * q_next = q->P_Next;
				FREE(q);
				q = q_next;
			}
		}
		if(t->FileNeedsFreeing) 
			SAlloc::F(const_cast<char *>(t->P_FileName)); // @badcast
		FREE(t);
	}
}

/*static*/int Tokeniser::HexToNum(int ch) 
{
	if('0' <= ch && ch <= '9') return ch - '0';
	if('a' <= ch && ch <= 'f') return ch - 'a' + 10;
	if('A' <= ch && ch <= 'F') return ch - 'A' + 10;
	return -1;
}

Analyser * Tokeniser::CreateAnalyser() 
{
	NEW(Analyser, a);
	a->P_Tokeniser = this;
	a->P_Nodes = 0;
	a->P_Names = 0;
	a->literalstrings = 0;
	a->P_Program = 0;
	a->P_Amongs = 0;
	a->AmongCount = 0;
	a->P_Groupings = 0;
	a->Mode = m_forward;
	a->Modifyable = true;
	{ 
		for(int i = 0; i < t_size; i++) 
			a->NameCount[i] = 0; 
	}
	a->P_Substring = 0;
	a->IntLimitsUsed = false;
	return a;
}

void Tokeniser::Error(const char * s1, int n, symbol * pSymb, const char * s2) 
{
	if(ErrCount == 20) {
		slfprintf_stderr("... etc\n"); exit(1);
	}
	slfprintf_stderr("%s:%d: ", P_FileName, LineNumber);
	if(s1) 
		slfprintf_stderr("%s", s1);
	if(pSymb) {
		for(int i = 0; i < n; i++) 
			slfprintf_stderr("%c", pSymb[i]);
	}
	if(s2) 
		slfprintf_stderr("%s", s2);
	slfprintf_stderr("\n");
	ErrCount++;
}

void Tokeniser::Error1(const char * s) 
{
	Error(s, 0, 0, 0);
}

void Tokeniser::Error2(const char * s) 
{
	Error("unexpected end of text after ", 0, 0, s);
}

int Tokeniser::NextChar() { return (C >= SIZE(P_Symb)) ? -1 : P_Symb[C++]; }

int Tokeniser::WhiteSpace(int ch) 
{
	switch(ch) {
		case '\n':
			LineNumber++;
		// fall through 
		case '\r':
		case '\t':
		case ' ':
			return true;
	}
	return false;
}

int Tokeniser::NextRealChar()
{
	while(true) {
		int ch = NextChar();
		if(!WhiteSpace(ch)) 
			return ch;
	}
}

void Tokeniser::ReadChars() 
{
	int ch = NextRealChar();
	if(ch < 0) {
		Error2("stringdef"); 
	}
	else {
		int c0 = C-1;
		while(true) {
			ch = NextChar();
			if(WhiteSpace(ch) || ch < 0) 
				break;
		}
		P_b2 = move_to_b(P_b2, C - c0 - 1, P_Symb + c0);
	}
}

const symbol * Tokeniser::FindInM(int n, const symbol * p) const
{
	for(const MPair * q = m_pairs; q; q = q->next) {
		const symbol * name = q->name;
		if(n == SIZE(name) && memcmp(name, p, n * sizeof(symbol)) == 0) 
			return q->value;
	}
	return 0;
}

void Tokeniser::ConvertNumericString(symbol * pSymb, int base) 
{
	int c = 0; 
	int d = 0;
	while(true) {
		while(c < SIZE(pSymb) && pSymb[c] == ' ') 
			c++;
		if(c == SIZE(pSymb)) 
			break;
		{
			int number = 0;
			while(c != SIZE(pSymb)) {
				int ch = pSymb[c];
				if(ch == ' ') 
					break;
				if(base == 10) {
					if(isdec(ch))
						ch = (ch - '0');
					else {
						Error1("decimal string contains non-digits");
						return;
					}
				}
				else {
					ch = HexToNum(ch);
					if(ch < 0) {
						Error1("hex string contains non-hex characters");
						return;
					}
				}
				number = base * number + ch;
				c++;
			}
			if(Encoding == ENC_SINGLEBYTE) {
				if(number < 0 || number > 0xff) {
					Error1("character values exceed 256");
					return;
				}
			}
			else {
				if(number < 0 || number > 0xffff) {
					Error1("character values exceed 64K");
					return;
				}
			}
			if(Encoding == ENC_UTF8)
				d += put_utf8(number, pSymb + d);
			else
				pSymb[d++] = number;
		}
	}
	SIZE(pSymb) = d;
}

int Tokeniser::NextToken() 
{
	symbol * p_symb = P_Symb;
	int c = C;
	int ch;
	int code = -1;
	while(true) {
		if(c >= SIZE(p_symb)) {
			C = c; 
			return -1;
		}
		ch = p_symb[c];
		if(WhiteSpace(ch)) {
			c++; 
			continue;
		}
		if(isalpha(ch)) {
			int c0 = c;
			while(c < SIZE(p_symb) && (isalnum(p_symb[c]) || p_symb[c] == '_')) 
				c++;
			code = find_word(c - c0, p_symb + c0);
			if(code < 0 || TokenDisabled[code]) {
				P_b = move_to_b(P_b, c - c0, p_symb + c0);
				code = c_name;
			}
		}
		else if(isdigit(ch)) {
			int c0 = c;
			while(c < SIZE(p_symb) && isdigit(p_symb[c])) 
				c++;
			Number = get_number(c - c0, p_symb + c0);
			code = c_number;
		}
		else if(ch == '\'')        {
			c = read_literal_string(this, c + 1);
			code = c_literalstring;
		}
		else {
			const int lim = smin(2, SIZE(p_symb) - c);
			for(int i = lim; i > 0; i--) {
				code = find_word(i, p_symb + c);
				if(code >= 0) {
					c += i; 
					break;
				}
			}
		}
		if(code >= 0) {
			C = c;
			return code;
		}
		Error("'", 1, p_symb + c, "' unknown");
		c++;
		continue;
	}
}

int Tokeniser::ReadToken() 
{
	symbol * p_symb = P_Symb;
	const bool held = TokenHeld;
	TokenHeld = false;
	if(held) 
		return Token;
	else {
		while(true) {
			int code = NextToken();
			switch(code) {
				case c_comment1: //  slash-slash comment 
					while(C < SIZE(p_symb) && p_symb[C] != '\n') 
						C++;
					continue;
				case c_comment2: // slash-star comment 
					while(true) {
						if(C >= SIZE(p_symb)) {
							Error1("/* comment not terminated");
							Token = -1;
							return -1;
						}
						if(p_symb[C] == '\n') 
							LineNumber++;
						if(eq_s(this, "*/")) 
							break;
						C++;
					}
					continue;
				case c_stringescapes: 
					{
						const int ch1 = NextRealChar();
						const int ch2 = NextRealChar();
						if(ch2 < 0) {
							Error2("stringescapes");
							continue;
						}
						if(ch1 == '\'') {
							Error1("first stringescape cannot be '");
							continue;
						}
						m_start = ch1;
						m_end = ch2;
						continue;
					}
				case c_stringdef: 
					{
						int base = 0;
						ReadChars();
						code = ReadToken(); // @recursion
						if(code == c_hex) {
							base = 16; 
							code = ReadToken(); // @recursion
						}
						else if(code == c_decimal) {
							base = 10; 
							code = ReadToken(); // @recursion
						}
						if(code != c_literalstring) {
							Error1("string omitted after stringdef");
							continue;
						}
						if(base > 0) 
							ConvertNumericString(P_b, base);
						{   
							NEW(MPair, q);
							q->next = m_pairs;
							q->name = copy_b(P_b2);
							q->value = copy_b(P_b);
							m_pairs = q;
							if(UPlusMode != UPLUS_DEFINED && (SIZE(P_b2) >= 3 && P_b2[0] == 'U' && P_b2[1] == '+')) {
								if(UPlusMode == UPLUS_UNICODE)
									Error1("U+xxxx already used with implicit meaning");
								else
									UPlusMode = UPLUS_DEFINED;
							}
						}
						continue;
					}
				case c_get:
					code = ReadToken(); // @recursion
					if(code != c_literalstring) {
						Error1("string omitted after get"); continue;
					}
					GetDepth++;
					if(GetDepth > 10) {
						slfprintf_stderr("get directives go 10 deep. Looping?\n");
						exit(1);
					}
					{
						NEW(Input, q);
						char * p_file_name = b_to_s(P_b);
						symbol * u = get_input(p_file_name);
						if(u == 0) {
							for(const Include * r = P_Includes; r; r = r->next) {
								symbol * b = copy_b(r->b);
								b = add_to_b(b, SIZE(P_b), P_b);
								SAlloc::F(p_file_name);
								p_file_name = b_to_s(b);
								u = get_input(p_file_name);
								lose_b(b);
								if(u != 0) 
									break;
							}
						}
						if(u == 0) {
							Error("Can't get '", SIZE(P_b), P_b, "'");
							exit(1);
						}
						memmove(q, this, sizeof(Input));
						P_Next = q;
						P_Symb = u;
						C = 0;
						P_FileName = p_file_name;
						FileNeedsFreeing = true;
						LineNumber = 1;
					}
					p_symb = P_Symb;
					continue;
				case -1:
					if(P_Next) {
						lose_b(p_symb);
						{
							Input * q = P_Next;
							memmove(this, q, sizeof(Input)); 
							p_symb = P_Symb;
							FREE(q);
						}
						GetDepth--;
						continue;
					}
				/* fall through */
				default:
					PreviousToken = Token;
					Token = code;
					return code;
			}
		}
	}
}
