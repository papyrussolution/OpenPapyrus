//
// Copyright (C) 1991 Texas Instruments Incorporated.
//  Permission is granted to any individual or institution to use, copy, modify,
//  and distribute this software, provided that this complete copyright and
//  permission notice is maintained, intact, in all copies and supporting documentation.
//  Texas Instruments Incorporated provides this software "as is" without express or implied warranty.
//
//  Created: MNF 06/13/89 -- Initial Design and Implementation ;
//  Updated: MBN 09/08/89 -- Added conditional exception handling ;
//  Updated: MBN 12/15/89 -- Sprinkled "const" qualifiers all over the place! ;
//  Updated: MJF 03/12/90 -- Added group names to RAISE ;
//  Updated: DLS 03/22/91 -- New lite version ;
//  This file contains the implementation for the member functions of the CRegExp class.
//  The CRegExp class is defined in the CRegExp.h (slib.h) header file. More
//  documentation is also available in that file. A significant part of this
//  file is derived directly from other work done on regular expressions. That part is so marked.
// -----------------------
// Adopted to slib by Anton Sobolev
//
#include <slib-internal.h>
#pragma hdrstop

#define MAGIC   0234

CRegExp_Depricated::CRegExp_Depricated() : ErrCode(SLERR_RE_UNINIT), P_Program(0)
{
}

CRegExp_Depricated::CRegExp_Depricated(const char * s) : ErrCode(SLERR_RE_UNINIT), P_Program(0)
{
	Compile(s);
}

CRegExp_Depricated::~CRegExp_Depricated()
{
	delete P_Program;
}

int  CRegExp_Depricated::GetLastErr() const { return ErrCode; }
bool CRegExp_Depricated::IsValid() const { return (ErrCode == 0); }
void CRegExp_Depricated::set_invalid() { ZDELETE(P_Program); }

bool FASTCALL CRegExp_Depricated::operator == (const CRegExp_Depricated & rxp) const
{
	return (ProgSize == rxp.ProgSize && memcmp(P_Program, rxp.P_Program, ProgSize) == 0);
}
//
// Descr: Two CRegExp objects that are deep_equal are both == and have ;
//   the same startp[0] and endp[0] pointers. This means that they ;
//   have the same compiled regular expression and they last ;
//   matched on the same identical string. ;
// Input: A reference to a CRegExp object. ;
// Output: Boolean TRUE/FALSE
//
int FASTCALL CRegExp_Depricated::deep_equal(const CRegExp_Depricated & rxp) const
{
	return BIN(ProgSize == rxp.ProgSize && memcmp(P_Program, rxp.P_Program, ProgSize) == 0 && 
		(startp[0] == rxp.startp[0] && /* Else if same start/end ptrs, */ endp[0] == rxp.endp[0]));
}
//
//  The remaining code in this file is derived from the regular expression code
//  whose copyright statement appears below. It has been changed to work
//  with the class concepts of C++ and COOL.
//  Compile and find Copyright (c) 1986 by University of Toronto. Written by Henry Spencer.
//  Not derived from licensed software. Permission is granted to anyone to
//  use this software for any purpose on any computer system, and to redistribute
//  it freely, subject to the following restrictions: 1. The author is not
//  responsible for the consequences of use of this software, no matter how awful,
//  even if they arise from defects in it. 2. The origin of this software must not
//  be misrepresented, either by explicit claim or by omission. 3. Altered versions
//  must be plainly marked as such, and must not be misrepresented as being the
//  original software. Beware that some of this code is subtly aware of the way
//  operator precedence is structured in regular expressions. Serious changes in
//  regular-expression syntax might require a total rethink.
//
//  The "internal use only" fields in regexp.h are present to pass info from
//  Compile to execute that permits the execute phase to run lots faster on simple
//  cases. They are: RegStart char that must begin a match '\0' if none obvious
//  RegAnch is the match anchored (at beginning-of-line only)? P_RegMust string
//  (pointer into P_Program) that match must include, or NULL RegMLen
//  length of P_RegMust string Regstart and RegAnch permit very fast decisions on
//  suitable starting points for a match, cutting down the work a lot. Regmust
//  permits fast rejection of lines that cannot possibly match. The P_RegMust tests
//  are costly enough that Compile() supplies a P_RegMust only if the r.e. contains
//  something potentially expensive (at present, the only such thing detected is//
//  or at the start of the r.e., which can involve a lot of backup). Regmlen is
//  supplied because the test in Find() needs it and Compile() is computing it anyway.
//
//  Structure for regexp "P_Program".
//  This is essentially a linear encoding of a nondeterministic finite-state machine
//  (aka syntax charts or "railroad normal form" in parsing technology).
//  Each node is an opcode plus a "next" pointer, possibly plus an operand.
//  "Next" pointers of all nodes except BRANCH implement concatenation a "next" pointer
//  with a BRANCH on both ends of it is connecting two alternatives.
//  (Here we have one of the subtle syntax dependencies: an individual BRANCH (as opposed
//  to a collection of them) is never concatenated with anything because of operator precedence.)
//  The operand of some types of node is a literal string for others, it is a node leading into
//  a sub-FSM. In particular, the operand of a BRANCH node is the first node of the branch.
//  (NB this is *not* a tree structure: the tail of the branch connects to the thing following the set of
//  BRANCHes.) The opcodes are: definition number opnd? meaning
//
#define END		0						/* no End of P_Program. */
#define BOL		1						/* no Match "" at beginning of line. */
#define EOL		2						/* no Match "" at end of line. */
#define ANY		3						/* no Match any one character. */
#define ANYOF	4						/* str Match any character in this string. */
#define ANYBUT	5						/* str Match any character not in this */

/* string. */
#define BRANCH	6						/* node Match this alternative, or the */

/* next... */
#define BACK	7						/* no Match "", "next" ptr points backward. */
#define EXACTLY 8						/* str Match this string. */
#define NOTHING 9						/* no Match empty string. */
#define STAR	10						/* node Match this (simple) thing 0 or more */
/* times. */
#define PLUS	11						/* node Match this (simple) thing 1 or more */
/* times. */
#define OPEN	20						/* no Mark this point in input as start of */

/*
 * #n. ;
 * OPEN+1 is number 1, etc.
 */
#define CLOSE	30						/* no Analogous to OPEN. */
//
//  Opcode notes: BRANCH The set of branches constituting a single choice are
//  hooked together with their "next" pointers, since precedence prevents anything
//  being concatenated to any individual branch. The "next" pointer of the last
//  BRANCH in a choice points to the thing following the whole choice. This is also
//  where the final "next" pointer of each individual branch points
//  each branch starts with the operand node of a BRANCH node. BACK Normal "next"
//  pointers all implicitly point forward
//  BACK exists to make loop structures possible. STAR,PLUS '?', and complex '*'
//  and '+', are implemented as circular BRANCH structures using BACK. Simple cases
//  (one character per match) are implemented with STAR and PLUS for speed and to
//  minimize recursive plunges. OPEN,CLOSE ...are numbered at compile time. ;
//  A node is one char of opcode followed by two chars of "next" pointer. "Next"
//  pointers are stored as two 8-bit pieces, high order first. The value is a
//  positive offset from the opcode of the node containing it. An operand, if any,
//  simply follows the node. (Note that much of the code generation knows about
//  this implicit relationship.) Using two bytes for the "next" pointer is vast
//  overkill for most things, but allows patterns to get big without disasters.
//
#define OP(p)		(*(p))
#define OPERAND(p)	((p) + 3)
//
// Utility definitions.
//
#ifndef CHARBITS
	#define UCHARAT(p)	((int) * (const uchar *)(p))
#else
	#define UCHARAT(p)	((int) *(p) & CHARBITS)
#endif
#define FAIL(m) { regerror(m); return NULL; }
#define ISMULT(c)	oneof3(c, '*', '+', '?')
#define META		"^$.[()|?+*\\"
//
// Flags to be passed up and down.
//
#define HASWIDTH	01					/* Known never to match null string. */
#define SIMPLE		02					/* Simple enough to be STAR/PLUS operand. */
#define SPSTART		04					/* Starts with * or +. */
#define WORST		0					/* Worst case. */
//
//
// COMPILE AND ASSOCIATED FUNCTIONS
//
//
//  Descr: compile a regular expression into internal code We can't allocate
//    space until we know how big the compiled form will be, but we can't compile it
//    (and thus know how big it is) until we've got a place to put the code. So we
//    cheat: we compile it twice, once with code generation turned off and size
//    counting turned on, and once "for real". This also means that we don't allocate
//    space until we are sure that the thing really will compile successfully, and we
//    never have to move the code and thus invalidate pointers into it. (Note that it
//    has to be in one piece because SAlloc::F() must be able to free it all.) Beware that
//    the optimization-preparation code in here knows about some of the structure of
//    the compiled regexp.
//
int CRegExp_Depricated::Compile(const char * pPattern)
{
	int    ok = 1;
	const  char * scan;
	const  char * longest;
	int    flags;
	ErrCode = 0;
	THROW_S(pPattern, SLERR_RE_NOEXPR);
	//
	// First pass: determine size, legality.
	//
	P_RegParse = pPattern;
	RegNPar = 1;
	RegSize = 0L;
	RegDummy = 0;
	P_RegCode = &RegDummy;
	regc((char)MAGIC);
	reg(0, &flags);
	THROW(ErrCode == 0);
	startp[0] = endp[0] = P_Text = 0;
	//
	// Small enough for pointer-storage convention?
	//
	THROW_S(RegSize < 32767L, SLERR_RE_EXPRTOOBIG);
	ZDELETE(P_Program);
	THROW_S(P_Program = new char[RegSize], SLERR_NOMEM);
	ProgSize = RegSize;
	//
	// Second pass: emit code.
	//
	P_RegParse = pPattern;
	RegNPar = 1;
	P_RegCode = P_Program;
	regc((char)MAGIC);
	reg(0, &flags);
	//
	// Dig out information for optimizations.
	//
	RegStart = '\0'; // Worst-case defaults.
	RegAnch = 0;
	P_RegMust = NULL;
	RegMLen = 0;
	scan = P_Program + 1;			/* First BRANCH. */
	if(OP(regnext(scan)) == END) {
		// Only one top-level choice.
		scan = OPERAND(scan);
		// Starting-point info.
		if(OP(scan) == EXACTLY)
			RegStart = *OPERAND(scan);
		else if(OP(scan) == BOL)
			RegAnch++;
		//
		//  If there's something expensive in the r.e., find the longest
		//  literal string that must appear and make it the P_RegMust. Resolve
		//  ties in favor of later strings, since the RegStart check works
		//  with the beginning of the r.e. and avoiding duplication
		//  strengthens checking. Not a strong reason, but sufficient in the
		//  absence of others.
		//
		if(flags & SPSTART) {
			longest = NULL;
			size_t len = 0;
			for(; scan; scan = regnext(scan))
				if(OP(scan) == EXACTLY) {
					size_t len2 = strlen(OPERAND(scan));
					if(len2 >= len) {
						longest = OPERAND(scan);
						len = len2;
					}
				}
			P_RegMust = longest;
			RegMLen = len;
		}
	}
	ErrCode = 0;
	CATCH
		ok = (ErrCode = SLibError, 0);
	ENDCATCH
	return ok;
}
//
// Descr: emit (if appropriate) a byte of code
//
void FASTCALL CRegExp_Depricated::regc(char b)
{
	if(P_RegCode != &RegDummy)
		*P_RegCode++ = b;
	else
		RegSize++;
}
//
// Descr: emit a node Location.
//
char * FASTCALL CRegExp_Depricated::regnode(char op)
{
	char * p_ret = P_RegCode;
	if(p_ret == &RegDummy)
		RegSize += 3;
	else {
		char * ptr = p_ret;
		*ptr++ = op;
		*ptr++ = '\0'; // Null "next" pointer.
		*ptr++ = '\0';
		P_RegCode = ptr;
	}
	return p_ret;
}
//
// Descr: insert an operator in front of already-emitted operand Means
//   relocating the operand.
//
void FASTCALL CRegExp_Depricated::reginsert(char op, char * pOpnd)
{
	if(P_RegCode == &RegDummy)
		RegSize += 3;
	else {
		const char * p_src = P_RegCode;
		P_RegCode += 3;
		char * p_dst = P_RegCode;
		while(p_src > pOpnd)
			*--p_dst = *--p_src;
		char * p_place = pOpnd; // Op node, where operand used to be.
		*p_place++ = op;
		*p_place++ = '\0';
		*p_place++ = '\0';
	}
}
//
// Descr: the lowest level Optimization: gobbles an entire sequence of
//   ordinary characters so that it can turn them into a single node, which is
//   smaller to store and faster to run. Backslashed characters are exceptions, each
//   becoming a separate node;
//   the code is simpler that way and it's not worth fixing.
//
char * FASTCALL CRegExp_Depricated::regatom(int * pFlag)
{
	char * p_ret = 0;
	if(ErrCode == 0) {
		const  char * p_preserve_ptr = P_RegParse;
		int    flags;
		*pFlag = WORST; // Tentatively.
		switch(*P_RegParse++) {
			case '^':
				p_ret = regnode(BOL);
				break;
			case '$':
				p_ret = regnode(EOL);
				break;
			case '.':
				p_ret = regnode(ANY);
				*pFlag |= HASWIDTH | SIMPLE;
				break;
			case '[':
				if(*P_RegParse == '^') {
					// Complement of range.
					p_ret = regnode(ANYBUT);
					P_RegParse++;
				}
				else
					p_ret = regnode(ANYOF);
				if(oneof2(*P_RegParse, ']', '-'))
					regc(*P_RegParse++);
				while(!oneof2(*P_RegParse, ']', '\0')) {
					if(*P_RegParse == '-') {
						P_RegParse++;
						if(oneof2(*P_RegParse, ']', '\0') || (P_RegParse-2) == p_preserve_ptr)
							regc('-');
						else {
							int rxpclass    = UCHARAT(P_RegParse - 2) + 1;
							int rxpclassend = UCHARAT(P_RegParse);
							if(rxpclass > rxpclassend + 1) {
								ErrCode = SLERR_RE_INVBRANGE;
								return 0;
							}
							for(; rxpclass <= rxpclassend; rxpclass++)
								regc(rxpclass);
							P_RegParse++;
						}
					}
					else
						regc(*P_RegParse++);
				}
				regc('\0');
				if(*P_RegParse != ']') {
					ErrCode = SLERR_RE_UNMATCHEDBR;
				}
				else {
					P_RegParse++;
					*pFlag |= (HASWIDTH | SIMPLE);
				}
				break;
			case '(':
				p_ret = reg(1, &flags);
				if(p_ret)
					*pFlag |= flags & (HASWIDTH | SPSTART);
				break;
			case '\0':
			case '|':
			case ')':
				ErrCode = SLERR_RE_INTERNAL;
				break;
			case '?':
			case '+':
			case '*':
				ErrCode = SLERR_RE_WCNOTHFOLLOWS;
				break;
			case '\\':
				if(*P_RegParse == '\0')
					ErrCode = SLERR_RE_TRAILINGBSL;
				else {
					p_ret = regnode(EXACTLY);
					regc(*P_RegParse++);
					regc('\0');
					*pFlag |= HASWIDTH | SIMPLE;
				}
				break;
			default:
				{
					P_RegParse--;
					int    len = (int)strcspn(P_RegParse, META);
					if(len <= 0)
						ErrCode = SLERR_RE_INTERNAL;
					else {
						char   ender = *(P_RegParse + len);
						if(len > 1 && ISMULT(ender))
							len--; // Back off clear of ?+* operand.
						*pFlag |= HASWIDTH;
						if(len == 1)
							*pFlag |= SIMPLE;
						p_ret = regnode(EXACTLY);
						while(len > 0) {
							regc(*P_RegParse++);
							len--;
						}
						regc('\0');
					}
				}
				break;
		}
	}
	return p_ret;
}
//
// Descr: set the next-pointer at the end of a node chain
//
void FASTCALL CRegExp_Depricated::regtail(char * p, const char * pVal)
{
	if(p != &RegDummy) {
		// Find last node.
		char * p_scan = p;
		char * p_temp;
		while((p_temp = regnext(p_scan)) != 0)
			p_scan = p_temp;
		size_t offset = (OP(p_scan) == BACK) ? (p_scan - pVal) : (pVal - p_scan);
		*(p_scan + 1) = (offset >> 8) & 0377;
		*(p_scan + 2) = offset & 0377;
	}
}
//
// Descr: regtail on operand of first argument
//   nop if operandless
//
void FASTCALL CRegExp_Depricated::regoptail(char * p, const char * val)
{
	// "Operandless" and "op != BRANCH" are synonymous in practice.
	if(p && p != &RegDummy && OP(p) == BRANCH)
		regtail(OPERAND(p), val);
}
//
//  Descr: something followed by possible [*+?] Note that the branching code
//    sequences used for ? and the general cases of//  and + are somewhat optimized:
//    they use the same NOTHING node as both the endmarker for their branch list and
//    the body of the last branch. It might seem that this node could be dispensed
//    with entirely, but the endmarker role is not redundant.
//
char * FASTCALL CRegExp_Depricated::regpiece(int * pFlag)
{
	char * p_ret = 0;
	if(ErrCode == 0) {
		int    flags = 0;
		p_ret = regatom(&flags);
		if(ErrCode == 0 && p_ret) {
			const char op = *P_RegParse;
			if(!ISMULT(op))
				*pFlag = flags;
			else if(!(flags & HASWIDTH) && op != '?')
				p_ret = (ErrCode = SLERR_RE_WCCOULDBEEMPT, 0);
			else {
				*pFlag = (op != '+') ? (WORST | SPSTART) : (WORST | HASWIDTH);
				if(op == '*' && (flags & SIMPLE))
					reginsert(STAR, p_ret);
				else if(op == '*') {
					//
					// Emit x* as (x&|), where & means "self".
					//
					reginsert(BRANCH, p_ret);         // Either x
					regoptail(p_ret, regnode(BACK));  // and loop
					regoptail(p_ret, p_ret);          // back
					regtail(p_ret, regnode(BRANCH));  // or
					regtail(p_ret, regnode(NOTHING)); // null.
				}
				else if(op == '+' && (flags & SIMPLE))
					reginsert(PLUS, p_ret);
				else if(op == '+') {
					//
					// Emit x+ as x(&|), where & means "self".
					//
					char * p_next = regnode(BRANCH); // Either
					regtail(p_ret, p_next);
					regtail(regnode(BACK), p_ret);    // loop back
					regtail(p_next, regnode(BRANCH)); // or
					regtail(p_ret, regnode(NOTHING)); // null.
				}
				else if(op == '?') {
					//
					// Emit x? as (x|)
					//
					reginsert(BRANCH, p_ret);         // Either x
					regtail(p_ret, regnode(BRANCH));  // or
					char * p_next = regnode(NOTHING); // null.
					regtail(p_ret, p_next);
					regoptail(p_ret, p_next);
				}
				P_RegParse++;
				if(ISMULT(*P_RegParse))
					p_ret = (ErrCode = SLERR_RE_NESTEDWC, 0);
			}
		}
		else
			p_ret = 0;
	}
	return p_ret;
}
//
// Descr: one alternative of an | operator Implements the concatenation operator.
//
char * FASTCALL CRegExp_Depricated::regbranch(int * pFlag)
{
	*pFlag = WORST; // Tentatively
	char * p_ret = regnode(BRANCH);
	char * p_chain = NULL;
	while(*P_RegParse != '\0' && *P_RegParse != '|' && *P_RegParse != ')') {
		int    flags;
		char * p_latest = regpiece(&flags);
		if(p_latest) {
			*pFlag |= flags & HASWIDTH;
			if(p_chain == NULL) // First piece.
				*pFlag |= flags & SPSTART;
			else
				regtail(p_chain, p_latest);
			p_chain = p_latest;
		}
		else
			return 0;
	}
	if(p_chain == NULL) // Loop ran zero times
		regnode(NOTHING);
	return p_ret;
}
//
// Descr: regular expression, i.e. main body or parenthesized thing Caller must
//   absorb opening parenthesis. Combining parenthesis handling with the base level
//   of regular expression is a trifle forced, but the need to tie the tails of the
//   branches to what follows makes it hard to avoid.
//
char * CRegExp_Depricated::reg(int paren, int * pFlag)
{
	char * p_ret = 0;
	char * p_ender;
	int    parno = 0;
	int    flags;
	if(ErrCode == 0) {
		*pFlag = HASWIDTH; // Tentatively.
		//
		// Make an OPEN node, if parenthesized.
		//
		if(paren) {
			if(RegNPar >= NSUBEXP)
				return (ErrCode = SLERR_RE_TOOMANYPAR, 0);
			else {
				parno = RegNPar++;
				p_ret = regnode(OPEN + parno);
			}
		}
		else
			p_ret = 0;
		//
		// Pick up the branches, linking them together.
		//
		char * p_br = regbranch(&flags);
		if(p_br == 0)
			p_ret = 0;
		else {
			if(p_ret)
				regtail(p_ret, p_br); // OPEN -> first.
			else
				p_ret = p_br;
			if(!(flags & HASWIDTH))
				*pFlag &= ~HASWIDTH;
			*pFlag |= flags & SPSTART;
			while(*P_RegParse == '|') {
				P_RegParse++;
				p_br = regbranch(&flags);
				if(p_br) {
					regtail(p_ret, p_br); // BRANCH -> BRANCH.
					if(!(flags & HASWIDTH))
						*pFlag &= ~HASWIDTH;
					*pFlag |= flags & SPSTART;
				}
				else
					return 0;
			}
			//
			// Make a closing node, and hook it on the end.
			//
			p_ender = regnode((paren) ? CLOSE + parno : END);
			regtail(p_ret, p_ender);
			//
			// Hook the tails of the branches to the closing node.
			//
			for(p_br = p_ret; p_br; p_br = regnext(p_br))
				regoptail(p_br, p_ender);
			//
			// Check for proper termination.
			//
			if(paren && *P_RegParse++ != ')')
				p_ret = (ErrCode = SLERR_RE_UNMATCHPAR, 0);
			else if(!paren && *P_RegParse != '\0') {
				ErrCode = (*P_RegParse == ')') ? SLERR_RE_UNMATCHPAR : SLERR_RE_INTERNAL;
				p_ret = 0;
				/* NOTREACHED */
			}
		}
	}
	return p_ret;
}
//
//
// find and friends
//
#ifdef DEBUG
int    regnarrate = 0;
void   regdump();
static char * regprop();
#endif
//
// Descr: repeatedly match something simple, report how many
//
int FASTCALL CRegExp_Depricated::regrepeat(const char * p)
{
	int    count = 0;
	const  char * scan = P_RegInput;
	const  char * opnd = OPERAND(p);
	switch(OP(p)) {
		case ANY:
			count = (int)strlen(scan);
			scan += count;
			break;
		case EXACTLY:
			while(*opnd == *scan) {
				count++;
				scan++;
			}
			break;
		case ANYOF:
			while(*scan != '\0' && sstrchr(opnd, *scan) != NULL) {
				count++;
				scan++;
			}
			break;
		case ANYBUT:
			while(*scan != '\0' && sstrchr(opnd, *scan) == NULL) {
				count++;
				scan++;
			}
			break;
		default:	/* Oh dear. Called inappropriately. */
			ErrCode = SLERR_RE_INTERNAL;
	}
	P_RegInput = scan;
	return count;
}
//
// Descr: main matching routine Conceptually the strategy is simple: check to see whether the current node matches,
//   call self recursively to see whether the rest matches, and then act accordingly. In practice we make some effort to
//   avoid recursion, in particular by going through "ordinary" nodes (that don't need to know whether the rest of the
//   match failed) by a loop instead of by recursion. 0 failure, 1 success
//
int FASTCALL CRegExp_Depricated::regmatch(const char * prog)
{
	const char * scan = prog; // Current node.
	while(scan) {
		const char * next = regnext(scan); // Next node.
		switch(OP(scan)) {
			case BOL:
				if(P_RegInput != P_RegBol)
					return 0;
				break;
			case EOL:
				if(*P_RegInput != '\0')
					return 0;
				break;
			case ANY:
				if(*P_RegInput == '\0')
					return 0;
				P_RegInput++;
				break;
			case EXACTLY:
				{
					const char * opnd = OPERAND(scan);
					/* Inline the first character, for speed. */
					if(*opnd != *P_RegInput)
						return 0;
					size_t len = strlen(opnd);
					if(len > 1 && strncmp(opnd, P_RegInput, len) != 0)
						return 0;
					P_RegInput += len;
				}
				break;
			case ANYOF:
				if(*P_RegInput == '\0' || sstrchr(OPERAND(scan), *P_RegInput) == NULL)
					return 0;
				P_RegInput++;
				break;
			case ANYBUT:
				if(*P_RegInput == '\0' || sstrchr(OPERAND(scan), *P_RegInput) != NULL)
					return 0;
				P_RegInput++;
				break;
			case NOTHING:
				break;
			case BACK:
				break;
			case OPEN + 1:
			case OPEN + 2:
			case OPEN + 3:
			case OPEN + 4:
			case OPEN + 5:
			case OPEN + 6:
			case OPEN + 7:
			case OPEN + 8:
			case OPEN + 9:
				{
					int   no = OP(scan) - OPEN;
					const char * save = P_RegInput;
					if(regmatch(next)) { // @recursion
						//
						// Don't set startp if some later invocation of the
						// same parentheses already has.
						//
						if(PP_RegStart[no] == NULL)
							PP_RegStart[no] = save;
						return 1;
					}
					else
						return 0;
				}
				break;
			case CLOSE + 1:
			case CLOSE + 2:
			case CLOSE + 3:
			case CLOSE + 4:
			case CLOSE + 5:
			case CLOSE + 6:
			case CLOSE + 7:
			case CLOSE + 8:
			case CLOSE + 9:
				{
					int    no = OP(scan) - CLOSE;
					const  char * save = P_RegInput;
					if(regmatch(next)) { // @recursion
						//
						// Don't set endp if some later invocation of the
						// same parentheses already has.
						//
						if(PP_RegEnd[no] == NULL)
							PP_RegEnd[no] = save;
						return 1;
					}
					else
						return 0;
				}
				break;
			case BRANCH:
				if(OP(next) != BRANCH)		/* No choice. */
					next = OPERAND(scan);	/* Avoid recursion. */
				else {
					do {
						const char * save = P_RegInput;
						if(regmatch(OPERAND(scan))) // @recursion
							return 1;
						else {
							P_RegInput = save;
							scan = regnext(scan);
						}
					} while(scan && OP(scan) == BRANCH);
					return 0;
					/* NOTREACHED */
				}
				break;
			case STAR:
			case PLUS:
				{
					//
					// Lookahead to avoid useless match attempts when we know
					// what character comes next.
					//
					char   nextch = (OP(next) == EXACTLY) ? *OPERAND(next) : 0;
					int    min_no = (OP(scan) == STAR) ? 0 : 1;
					const  char * save = P_RegInput;
					for(int no = regrepeat(OPERAND(scan)); no >= min_no;)
						// If it could work, try it.
						if((nextch == '\0' || *P_RegInput == nextch) && regmatch(next))
							return 1;
						else {
							// Couldn't or didn't -- back up.
							no--;
							P_RegInput = save + no;
						}
					return 0;
				}
				break;
			case END:
				return 1; // Success!
			default:
				return (ErrCode = SLERR_RE_INTERNAL, 0);
		}
		scan = next;
	}
	//
	//  We get here only if there's trouble -- normally "case END" is the terminating point.
	//
	ErrCode = SLERR_RE_PTRCORRUPT;
	return 0;
}
//
// Descr: try match at specific point 0 failure, 1 success
//
int CRegExp_Depricated::regtry(const char * string, const char ** start, const char ** end, const char * prog)
{
	P_RegInput  = string;
	PP_RegStart = start;
	PP_RegEnd   = end;
	const char ** sp1 = start;
	const char ** ep  = end;
	for(int i = NSUBEXP; i > 0; i--) {
		*sp1++ = NULL;
		*ep++ = NULL;
	}
	if(regmatch(prog + 1)) {
		start[0] = string;
		end[0] = P_RegInput;
		return 1;
	}
	else
		return 0;
}
//
// Descr: match a regexpr against a string
//
int CRegExp_Depricated::Find(const char * pText)
{
	if(pText) {
		P_Text = pText;
		// Check validity of P_Program.
		if(UCHARAT(P_Program) != MAGIC)
			return (ErrCode = SLERR_RE_BUFCORRUPT, 0);
		// If there is a "must appear" pText, look for it.
		if(P_RegMust) {
			const char * s = pText;
			for(; (s = sstrchr(s, P_RegMust[0])) != NULL; s++)
				if(strncmp(s, P_RegMust, RegMLen) == 0)
					break; // Found it.
			if(!s) // Not present.
				return 0;
		}
		P_RegBol = pText; // Mark beginning of line for ^ .
		//
		// Simplest case: anchored match need be tried only once.
		//
		if(RegAnch)
			return regtry(pText, startp, endp, P_Program);
		else {
			//
			// Messy cases: unanchored match.
			//
			const char * s = pText;
			if(RegStart != '\0') { // We know what char it must start with.
				for(; (s = sstrchr(s, RegStart)) != NULL; s++)
					if(regtry(s, startp, endp, P_Program))
						return 1;
			}
			else { // We don't -- general case.
				do {
					if(regtry(s, startp, endp, P_Program))
						return 1;
				} while(*s++ != '\0');
			}
			return 0;
		}
	}
	else
		return 0;
}

int CRegExp_Depricated::Find(SStrScan * pScan)
{
	const char * p = pScan ? static_cast<const char *>(*pScan) : 0;
	if(!isempty(p) && Find(p)) { // @v10.7.5 @fix !isempty(p)
		pScan->Incr(startp[0] - p);
		pScan->SetLen(endp[0] - startp[0]);
		return 1;
	}
	else
		return 0;
}
//
// Descr: dig the "next" pointer out of a node
//
#define NEXT(p) (((*((p) + 1) & 0377) << 8) + (*((p) + 2) & 0377))

const char * FASTCALL CRegExp_Depricated::regnext(const char * p) const
{
	const char * n = 0;
	if(p != &RegDummy) {
		int    offset = NEXT(p);
		if(offset)
			n = (OP(p) == BACK) ? (p - offset) : (p + offset);
	}
	return n;
}
//
//
//
char * FASTCALL CRegExp_Depricated::regnext(char * p) const
{
	char * n = 0;
	if(p != &RegDummy) {
		int    offset = NEXT(p);
		if(offset)
			n = (OP(p) == BACK) ? (p - offset) : (p + offset);
	}
	return n;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(CRegExp)
{
	int    ok = 1;
	SString file_name, temp_buf;
	uint   arg_no = 0;
	if(EnumArg(&arg_no, temp_buf))
		file_name = temp_buf;
	else
		file_name = temp_buf = "cregexp.txt";
	SFile file(MakeInputFilePath(file_name), SFile::mRead);
	if(file.IsValid()) {
		SString line_buf, re_buf, text_buf, temp_buf, out_line;
		SPathStruc::ReplaceExt(file_name, "out", 1);
		SFile out_file(MakeOutputFilePath(file_name), SFile::mWrite);
		while(file.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(line_buf.NotEmpty()) {
				StringSet ss(':', line_buf);
				uint   ssp = 0;
				ss.get(&ssp, re_buf);
				ss.get(&ssp, text_buf);
				ss.get(&ssp, temp_buf);
				const long right_count = temp_buf.ToLong();
				long   count = 0;
				SRegExp2 re;
				THROW(SLTEST_CHECK_NZ(re.Compile(re_buf, cp1251, SRegExp2::syntaxDefault, 0)));
				{
					const char * p = text_buf;
					out_line.Z().Cat(re_buf).CatDiv(':', 2).Cat(text_buf);
					SStrScan scan(text_buf);
					if(re.Find(&scan)) {
						out_line.CatDiv(':', 2);
						do {
							scan.Get(temp_buf);
							if(count)
								out_line.CatDiv(',', 2);
							out_line.CatQStr(temp_buf);
							scan.IncrLen();
							count++;
						} while(re.Find(&scan));
					}
					out_line.CatDiv(':', 2).Cat(count).CR();
					out_file.WriteLine(out_line);
					THROW(SLTEST_CHECK_EQ(count, right_count));
				}
			}
		}
	}
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
