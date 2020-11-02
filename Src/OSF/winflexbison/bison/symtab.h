/* Definitions for symtab.c and callers, part of Bison.

   Copyright (C) 1984, 1989, 1992, 2000-2002, 2004-2015, 2018-2020 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/**
 * \file symtab.h
 * \brief Manipulating ::symbol.
 */

#ifndef SYMTAB_H_
#define SYMTAB_H_

/*----------.
| Symbols.  |
   `----------*/

/** Symbol classes.  */
typedef enum {
	/** Undefined.  */
	unknown_sym,
	/** Declared with %type: same as Undefined, but triggered a Wyacc if
	    applied to a terminal. */
	pct_type_sym,
	/** Terminal. */
	token_sym,
	/** Nonterminal. */
	nterm_sym
} symbol_class;

/** Internal token numbers. */
typedef int symbol_number;
#define SYMBOL_NUMBER_MAXIMUM INT_MAX

typedef struct symbol symbol;
typedef struct sym_content sym_content;

/* Declaration status of a symbol.

   First, it is "undeclared".  Then, if "undeclared" and used in a
   %printer/%destructor, it is "used".  If not "declared" but used in
   a rule, it is "needed".  Finally, if declared (via a rule for
   nonterminals, or %token), it is "declared".

   When status are checked at the end, "declared" symbols are fine,
   "used" symbols trigger warnings, otherwise it's an error.  */

typedef enum {
	undeclared, /** Used in the input file for an unknown reason (error).  */
	used, /** Used by %destructor/%printer but not defined (warning).  */
	needed, /** Used in the grammar (rules) but not defined (error).  */
	declared, /** Defined with %type or %token (good).  */
} declaration_status;

enum code_props_type {
	destructor = 0,
	printer = 1,
};

typedef enum code_props_type code_props_type;

enum { CODE_PROPS_SIZE = 2 };

struct Symbol {
	uniqstr tag; /** The key, name of the symbol.  */
	Location location; /** The "defining" location.  */
	bool translatable; /** Whether this symbol is translatable. */
	/** Whether \a location is about the first uses as left-hand side
	    symbol of a rule (true), or simply the first occurrence (e.g.,
	    in a %type, or as a rhs symbol of a rule).  The former type of
	    location is more natural in error messages.  This Boolean helps
	    moving from location of the first occurrence to first use as
	    lhs. */
	bool location_of_lhs;
	Symbol * alias; /** Points to the other in the symbol-string pair for an alias. */
	bool is_alias; /** Whether this symbol is the alias of another or not. */
	sym_content * content; /** All the info about the pointed symbol is there. */
};

struct sym_content {
	/** The main symbol that denotes this content (it contains the
	    possible alias). */
	Symbol * symbol;
	/** Its \c \%type.

	    Beware that this is the type_name as was entered by the user,
	    including silly things such as "]" if she entered "%token <]> t".
	    Therefore, when outputting type_name to M4, be sure to escape it
	    into "@}".  See quoted_output for instance.  */
	uniqstr type_name;
	/** Its \c \%type's location.  */
	Location type_loc;

	/** Any \c \%destructor (resp. \%printer) declared specifically for this
	    symbol.

	    Access this field only through <tt>symbol</tt>'s interface functions. For
	    example, if <tt>symbol::destructor = NULL</tt> (resp. <tt>symbol::printer
	    = NULL</tt>), a default \c \%destructor (resp. \%printer) or a per-type
	    \c symbol_destructor_printer_get will compute the correct one. */
	code_props props[CODE_PROPS_SIZE];
	symbol_number number;
	Location prec_loc;
	int prec;
	assoc assoc;
	/** Token code, possibly specified by the user (%token FOO 42).  */
	int code;
	symbol_class Cls;
	declaration_status status;
};

Symbol * symbol_from_uniqstr(const uniqstr key, Location loc); /** Fetch (or create) the symbol associated to KEY.  */
Symbol * symbol_get(const char * key, Location loc); /** Fetch (or create) the symbol associated to KEY.  */

/** Generate a dummy nonterminal.

   Its name cannot conflict with the user's names.  */
Symbol * dummy_symbol_get(Location loc);

/*--------------------.
| Methods on symbol.  |
   `--------------------*/

void symbol_print(Symbol const * s, FILE * f); /** Print a symbol (for debugging). */
bool symbol_is_dummy(Symbol const * sym); /** Is this a dummy nonterminal?  */
char const * code_props_type_string(code_props_type kind); /** The name of the code_props type: "\%destructor" or "\%printer".  */

/** The name of the symbol that can be used as an identifier.
** Consider the alias if needed.
** Return 0 if there is none (e.g., the symbol is only defined as
** a string). */
uniqstr symbol_id_get(Symbol const * sym);

/**
 * Make \c str the literal string alias of \c sym.  Copy token number,
 * symbol number, and type from \c sym to \c str.
 */
void symbol_make_alias(Symbol * sym, Symbol * str, Location loc);

/**
 * This symbol is used as the lhs of a rule.  Record this location
 * as definition point, if not already done.
 */
void symbol_location_as_lhs_set(Symbol * sym, Location loc);

/** Set the \c type_name associated with \c sym.

    Do nothing if passed 0 as \c type_name.  */
void symbol_type_set(Symbol * sym, uniqstr type_name, Location loc);

/** Set the \c \%destructor or \c \%printer associated with \c sym.  */
void symbol_code_props_set(Symbol * sym, code_props_type kind, code_props const * destructor);

/** Get the computed \c \%destructor or \c %printer for \c sym, which was
    initialized with \c code_props_none_init if there's no \c \%destructor or
    \c %printer.  */
code_props * symbol_code_props_get(Symbol * sym, code_props_type kind);

/** Set the \c precedence associated with \c sym.

    Ensure that \a symbol is a terminal.
    Do nothing if invoked with \c undef_assoc as \c assoc.  */
void symbol_precedence_set(Symbol * sym, int prec, assoc a, Location loc);

/** Set the \c class associated with \c sym.

    Whether \c declaring means whether this class definition comes
    from %nterm or %token (but not %type, prec/assoc, etc.).  A symbol
    can have "declaring" set only at most once.  */
void symbol_class_set(Symbol * sym, symbol_class cls, Location loc, bool declaring);

/** Set the \c code associated with \c sym.  */
void symbol_code_set(Symbol * sym, int code, Location loc);

/*------------------.
| Special symbols.  |
   `------------------*/

extern Symbol * errtoken; /** The error token. */
extern Symbol * undeftoken; /** The token for unknown tokens.  */
extern Symbol * eoftoken; /** The end of input token.  */
/** The genuine start symbol.

   $accept: start-symbol $end */
extern Symbol * acceptsymbol;
extern Symbol * startsymbol; /** The user start symbol. */
extern Location startsymbol_loc; /** The location of the \c \%start declaration.  */

/** Whether a symbol declared with a type tag.  */
extern bool tag_seen;

/*-------------------.
| Symbol Relations.  |
   `-------------------*/

/* The symbol relations are represented by a directed graph. */

/* The id of a node */
typedef int graphid;
typedef struct symgraphlink symgraphlink;

struct symgraphlink {
	/** The second \c symbol or group of a precedence relation.
	 * See \c symgraph. */
	graphid id;
	symgraphlink * next;
};

/* Symbol precedence graph, to store the used precedence relations between symbols. */

typedef struct symgraph symgraph;

struct symgraph {
	graphid id; /** Identifier for the node: equal to the number of the symbol. */
	symgraphlink * succ; /** The list of related symbols that have a smaller precedence. */
	symgraphlink * pred; /** The list of related symbols that have a greater precedence. */
};

void register_precedence(graphid first, graphid snd); /** Register a new precedence relation as used. */
void print_precedence_warnings(); /** Print a warning for each symbol whose precedence and/or associativity is useless. */

/*----------------------.
| Symbol associativity  |
   `----------------------*/

void register_assoc(graphid i, graphid j);

/*-----------------.
| Semantic types.  |
   `-----------------*/

/** A semantic type and its associated \c \%destructor and \c \%printer.

   Access the fields of this struct only through the interface functions in
   this file.  \sa symbol::destructor  */
typedef struct {
	/** The key, name of the semantic type.  */
	uniqstr tag;

	/** The location of its first occurrence.  */
	Location location;

	/** Its status : "undeclared", "used" or "declared".
	    It cannot be "needed".  */
	declaration_status status;

	/** Any \c %destructor and %printer declared for this
	    semantic type.  */
	code_props props[CODE_PROPS_SIZE];
} semantic_type;

/** Fetch (or create) the semantic type associated to KEY.  */
semantic_type * semantic_type_from_uniqstr(const uniqstr key, const Location * loc);

/** Fetch (or create) the semantic type associated to KEY.  */
semantic_type * semantic_type_get(const char * key, const Location * loc);

/** Set the \c destructor or \c printer associated with \c type.  */
void semantic_type_code_props_set(semantic_type * type, code_props_type kind, code_props const * code);

/*----------------------------------.
| Symbol and semantic type tables.  |
   `----------------------------------*/

/** Create the symbol and semantic type tables, and the built-in symbols.  */
void symbols_new();

/** Free all the memory allocated for symbols and semantic types.  */
void symbols_free();

/** Check that all the symbols are defined.

    Report any undefined symbols and consider them nonterminals.  */
void symbols_check_defined();

/** Sanity checks and #token_translations construction.

   Perform various sanity checks, assign symbol numbers, and set up
 #token_translations.  */
void symbols_pack();

#endif /* !SYMTAB_H_ */
