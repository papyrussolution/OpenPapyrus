// BISON.H
//
#include <slib.h>
#include <stdbool.h>
#include <locale.h>
#include <process.h>
#include <flexbison_common_config.h>
#include <config.h>
#include "system.h"
#include <error.h>
#include <bitset.h>
#include <bitsetv.h>
#include <bitset/stats.h>
#include <attribute.h>
#include <verify.h>
#include <xalloc.h>
#include <get-errno.h>
#include <gl_list.h>
#include <gl_linked_list.h>
#include <gl_array_list.h>
#include <gl_xlist.h>
#include <gl_rbtreehash_list.h>
#include <hash.h>
#include <mbfile.h>
#include <mbswidth.h>
#include <quote.h>
#include <quotearg.h>
#include <timevar.h>
#include <fstrcmp.h>
#include <relocatable.h>
#include <argmatch.h>
#include <c-ctype.h>
#include <c-strcase.h>
#include <vasnprintf.h>
#include <xmemdup0.h>
#include <intprops.h>
#include <filename.h>
#include <path-join.h>
#include <closeout.h>
#include <unicodeio.h>
#include <dirname.h>
#include <getopt.h>
#include "glyphs.h"
#include "uniqstr.h"
//#include "assoc.h"
	//
	// Associativity values for tokens and rules
	//
	typedef enum {
		undef_assoc, /** Not defined. */
		right_assoc, /** %right */
		left_assoc, /** %left */
		non_assoc, /** %nonassoc */
		precedence_assoc /** %precedence */
	} assoc;

	char const * assoc_to_string(assoc a);
//
#include "location.h"
//#include "named-ref.h"
	//
	// Named reference object. Keeps information about a symbolic name of a symbol in a rule.
	//
	struct named_ref {
		uniqstr id; /* Symbolic named given by user. */
		Location loc; /* Location of the symbolic name. Not including brackets. */
	};

	named_ref * named_ref_new(uniqstr id, Location loc); /* Allocate a named reference object. */
	named_ref * named_ref_copy(const named_ref * r); /* Allocate and return a copy.  */
	void named_ref_free(named_ref * r); /* Free a named reference object. */
//
#include "getargs.h"
//#include "nullable.h"
	extern bool * nullable; // @global A vector saying which nonterminals can expand into the null string. NULLABLE[I - NTOKENS] is nonzero if symbol I can do so.  */
	extern void nullable_compute(); /* Set up NULLABLE. */
	extern void nullable_free(); /* Free NULLABLE. */
//
#include "complain.h"
//#include "files.h"
	extern char const * spec_outfile; // @global File name specified with -o for the output file, or 0 if no -o. 
	extern char * parser_file_name;   // @global File name for the parser (i.e., the one above, or its default.)
	extern const char * spec_name_prefix; // @global Symbol prefix specified with -p, or 0 if no -p. 
	extern Location spec_name_prefix_loc;
	extern char const * spec_file_prefix; // @global File name prefix specified with -b, or 0 if no -b. 
	extern Location spec_file_prefix_loc;
	extern char * spec_verbose_file; /* --verbose. */
	extern char * spec_graph_file; /* File name specified for the output graph.  */
	extern char * spec_xml_file; /* File name specified for the xml output.  */
	extern char * spec_header_file; /* File name specified with --defines.  */
	extern char * spec_mapped_header_file; /* File name specified with --defines, adjusted for mapped prefixes. */
	extern char * dir_prefix; /* Directory prefix of output file names.  */
	extern char * mapped_dir_prefix; /* Directory prefix of output file name, adjusted for mapped prefixes. */

	/* The file name as given on the command line.
	   Not named "input_file" because Flex uses this name for an argument,
	   and therefore GCC warns about a name clash. */
	extern uniqstr grammar_file;
	extern char * all_but_ext; /* The computed base for output file names.  */
	char const * pkgdatadir(); /* Where our data files are installed.  */
	char const * m4path(); /* Where the m4 program is installed.  */
	void compute_output_file_names();
	void output_file_names_free();

	/** Record that we generate a file.
	 *
	 *  \param file_name  the name of file being generated.
	 *  \param source whether this is a source file (*c, *.java...)
	 *                as opposed to a report (*.output, *.dot...).
	 */
	void output_file_name_check(char ** file_name, bool source);
	void unlink_generated_sources(); // Remove all the generated source files
	FILE * xfopen(const char * name, char const * mode);
	void xfclose(FILE * ptr);
	FILE * xfdopen(int fd, char const * mode);
	char * map_file_name(char const * filename);
	void add_prefix_map(char const * oldprefix, char const * newprefix);
//
//#include "print-graph.h"
	void print_graph();
//
//#include "print-xml.h"
	void xml_indent(FILE * out, int level);
	void xml_puts(FILE *, int, char const *);
	void xml_printf(FILE *, int, char const *, ...);
	char const * xml_escape_n(int n, char const * str);
	char const * xml_escape(char const * str);
	void print_xml();
//
//#include "print.h"
	void print_results();
//
//#include "output.h"
	void output(); // Output the parsing tables and the parser code to FTABLE
//
#include "muscle-tab.h"
#include "scan-code.h"
#include "symtab.h"
#include "gram.h"
//#include "derives.h"
	/* DERIVES[SYMBOL - NTOKENS] points to a vector of the rules that SYMBOL derives, terminated with NULL.  */
	extern rule *** derives; // @global
	/* Compute DERIVES.  */
	void derives_compute();
	void derives_free();
//
//#include "closure.h"
	// 
	// Descr: Allocates the itemset and ruleset vectors, and precomputes useful
	//   data so that closure can be called.  n is the number of elements to allocate for itemset.
	// 
	void closure_new(int n);
	/* Given the kernel (aka core) of a state (a sorted vector of item indices
	   ITEMS, of length N), set up RULESET and ITEMSET to indicate what
	   rules could be run and which items could be accepted when those
	   items are the active ones.  */
	void closure(item_index const * items, size_t n);
	void closure_free(); /* Free ITEMSET, RULESET and internal data.  */

	/* ITEMSET is a sorted vector of item indices; NITEMSET is its size
	   (actually, points to just beyond the end of the part of it that is
	   significant).  CLOSURE places there the indices of all items which
	   represent units of input that could arrive next.  */
	extern item_index * itemset; // @global
	extern size_t nitemset; // @global
//
#include "parse-gram.h"
#include "scan-gram.h"
#include "symlist.h"
#include "state.h"
#include "state-item.h"
//#include "conflicts.h"
	void conflicts_solve();
	/**
	 * Update state numbers recorded in internal arrays such that:
	 *   - \c nstates_old is the old number of states.
	 *   - Where \c i is the old state number, <tt>old_to_new[i]</tt> is either:
	 *     - \c nstates_old if state \c i is removed because it is unreachable.
	 *     - The new state number.
	 *   - The highest new state number is the number of remaining states - 1.
	 *   - The numerical order of the remaining states has not changed.
	 */
	void conflicts_update_state_numbers(state_number old_to_new[], state_number nstates_old);
	void conflicts_print();
	int conflicts_total_count();
	void conflicts_output(FILE * out);
	void conflicts_free();
	bool has_conflicts(const state * s);
	// Were there conflicts? 
	extern int expected_sr_conflicts; // @global
	extern int expected_rr_conflicts; // @global
//
//#include "reader.h"
	struct merger_list {
		merger_list * next;
		uniqstr name;
		uniqstr type;
		Location type_declaration_loc;
	};

	void grammar_start_symbol_set(Symbol * sym, Location loc);
	void grammar_current_rule_begin(Symbol * lhs, Location loc, named_ref * lhs_named_ref);
	void grammar_current_rule_end(Location loc);
	void grammar_midrule_action();
	/* Apply %empty to the current rule.  */
	void grammar_current_rule_empty_set(Location loc);
	void grammar_current_rule_prec_set(Symbol * precsym, Location loc);
	void grammar_current_rule_dprec_set(int dprec, Location loc);
	void grammar_current_rule_merge_set(uniqstr name, Location loc);
	void grammar_current_rule_expect_sr(int count, Location loc);
	void grammar_current_rule_expect_rr(int count, Location loc);
	void grammar_current_rule_symbol_append(Symbol * sym, Location loc,
		named_ref * nref);
	/* Attach an ACTION to the current rule.  */
	void grammar_current_rule_action_append(const char * action, Location loc, named_ref * nref, uniqstr tag);
	/* Attach a PREDICATE to the current rule.  */
	void grammar_current_rule_predicate_append(const char * predicate, Location loc);
	void reader(const char * gram);
	void free_merger_functions();

	extern merger_list * merge_functions; // @global
	extern bool union_seen; // @global Was %union seen?
	extern bool default_prec; // @global Should rules have a default precedence?
//
//#include "reduce.h"
	void reduce_grammar();
	void reduce_output(FILE * out);
	bool reduce_token_unused_in_grammar(symbol_number i);
	/** Whether symbol \a i is useless in the grammar.
	 * \pre  reduce_grammar was called before.
	 */
	bool reduce_nonterminal_useless_in_grammar(const sym_content * sym);
	void reduce_free();
	extern symbol_number * nterm_map; /** Map initial nterm numbers to the new ones.  Built by reduce_grammar.  Size nnterms + nuseless_nonterminals.  */
	extern int nuseless_nonterminals;
	extern int nuseless_productions;
//
#include "relation.h"
//#include "ielr.h"
	/**
	 * \pre
	 *   - \c ::states is of size \c ::nstates and defines an LR(0) parser
	 *     for the users's grammar.
	 *   - \c ::ntokens is the number of tokens in the grammar.
	 * \post
	 *   - \c ::states is of size \c ::nstates (which might be greater than
	 *     <tt>::nstates \@pre</tt>) and defines the type of parser specified by
	 *     the value of the \c \%define variable \c lr.type.  Its value can be:
	 *     - \c "lalr".
	 *     - \c "ielr".
	 *     - \c "canonical-lr".
	 */
	void ielr();
	bool ielr_item_has_lookahead(state *s, symbol_number lhs, size_t item, symbol_number lookahead, state ***predecessors, bitset **item_lookahead_sets);
//
//#include "lalr.h"
	/** Build the LALR(1) automaton.

	   Find which rules need lookahead in each state, and which lookahead
	   tokens they accept.

	   Also builds:
		 - #goto_map
		 - #from_state
		 - #to_state
		 - #goto_follows
	 */
	void lalr();
	/**
	 * Set #nLA and allocate all reduction lookahead sets.  Normally invoked by #lalr.
	 */
	void initialize_LA();
	/**
	 * Build only:
	 *   - #goto_map
	 *   - #from_state
	 *   - #to_state
	 * Normally invoked by #lalr.
	 */
	void set_goto_map();
	/**
	 * Update state numbers recorded in #goto_map, #from_state, and #to_state such
	 * that:
	 *   - \c nstates_old is the old number of states.
	 *   - Where \c i is the old state number, <tt>old_to_new[i]</tt> is either:
	 *     - \c nstates_old if state \c i is removed because it is unreachable.
	 *       Thus, remove all goto entries involving this state.
	 *     - The new state number.
	 */
	void lalr_update_state_numbers(state_number old_to_new[], state_number nstates_old);

	/** Release the information related to lookahead tokens.
	   Can be performed once the action tables are computed.  */
	void lalr_free();

	typedef size_t goto_number;
	#define GOTO_NUMBER_MAXIMUM ((goto_number) -1)

	/** Index into #from_state and #to_state.

	   All the transitions that accept a particular variable are grouped
	   together in FROM_STATE and TO_STATE, with indexes from GOTO_MAP[I -
	   NTOKENS] to GOTO_MAP[I - NTOKENS + 1] - 1 (including both).  */
	extern goto_number * goto_map;
	extern goto_number ngotos; /** The size of #from_state and #to_state.  */
	extern state_number * from_state; /** State number which a transition leads from.  */
	extern state_number * to_state; /** State number it leads to.  */
	goto_number map_goto(state_number src, symbol_number sym); /** The number of the goto from state SRC labeled with nterm SYM.  */
	extern bitsetv goto_follows; /* goto_follows[i] is the set of tokens following goto i.  */
//
//#include "lr0.h"
	void generate_states();
//
//#include "lssi.h"
	/*
	   All state-item graph nodes should also include a precise follow set (follow_L).
	   However, ignoring follow_L saves a lot of memory and is a pretty good approximation.
	   These functions exist to enforce restrictions caused by follow_L sets.
	 */
	/*
	 * find shortest lookahead-sensitive path of state-items to target such that
	 * next_sym is in the follow_L set of target in that position.
	 */
	state_item_list shortest_path_from_start(state_item_number target, symbol_number next_sym);

	/**
	 * Determine if the given terminal is in the given symbol set or can begin
	 * a nonterminal in the given symbol set.
	 */
	bool intersect_symbol(symbol_number sym, bitset syms);
	/**
	 * Determine if any symbol in ts is in syms
	 * or can begin with a nonterminal in syms.
	 */
	bool intersect(bitset ts, bitset syms);
	/**
	 * Compute a set of sequences of state-items that can make production steps
	 * to this state-item such that the resulting possible lookahead symbols are
	 * as given.
	 */
	state_item_list lssi_reverse_production(const state_item * si, bitset lookahead);
//
//#include "counterexample.h"
	// Init/deinit this module.
	void counterexample_init();
	void counterexample_free();
	//
	// Print the counterexamples for the conflicts of state S.
	//
	// Used both for the warnings on the terminal (OUT = stderr, PREFIX =
	// ""), and for the reports (OUT != stderr, PREFIX != "").
	void counterexample_report_state(const state * s, FILE * out, const char * prefix);
//
//#include "fixits.h"
	void fixits_register(Location const * loc, char const* update); /* Declare a fix to apply.  */
	void fixits_run(); /* Apply the fixits: update the source file.  */
	bool fixits_empty(); /* Whether there are no fixits. */
	void fixits_free(); /* Free the registered fixits.  */
//
//#include "derivation.h"
	// 
	// Derivations are trees of symbols such that each nonterminal's
	// children are symbols that produce that nonterminal if they are
	// relevant to the counterexample.  The leaves of a derivation form a
	// counterexample when printed. 
	// 
	typedef gl_list_t derivation_list;
	typedef struct derivation derivation;

	static inline derivation_list derivation_list_new()
	{
		return gl_list_create_empty(GL_LINKED_LIST, NULL, NULL, NULL, true);
	}

	static inline bool derivation_list_next(gl_list_iterator_t * it, derivation ** d)
	{
		const void * p = NULL;
		bool res = gl_list_iterator_next(it, &p, NULL);
		if(res)
			*d = (derivation *)(p);
		else
			gl_list_iterator_free(it);
		return res;
	}

	void derivation_list_append(derivation_list dl, derivation * d);
	void derivation_list_prepend(derivation_list dl, derivation * d);
	void derivation_list_free(derivation_list dl);

	derivation * derivation_new(symbol_number sym, derivation_list children);

	static inline derivation * derivation_new_leaf(symbol_number sym)
	{
		return derivation_new(sym, NULL);
	}

	// Number of symbols.
	size_t derivation_size(const derivation * deriv);
	void derivation_print(const derivation * deriv, FILE * out, const char * prefix);
	void derivation_print_leaves(const derivation * deriv, FILE * out);
	void derivation_free(derivation * deriv);
	void derivation_retain(derivation * deriv);

	// A derivation denoting the position of the dot.
	derivation * derivation_dot();
//
#include "Sbitset.h"
#include "InadequacyList.h"
#include "AnnotationList.h"
#include "parse-simulation.h"
#include "scan-skel.h"
//#include "graphviz.h"
	/** Begin a Dot graph.
	 *
	 * \param fout   output stream.
	 */
	void start_graph(FILE * fout);

	/** Output a Dot node.
	 *
	 * \param id     identifier of the node
	 * \param label  human readable label of the node (no Dot escaping needed).
	 * \param fout   output stream.
	 */
	void output_node(int id, char const * label, FILE * fout);

	/** Output a Dot edge.
	 * \param source       id of the source node
	 * \param destination  id of the target node
	 * \param label        human readable label of the edge
	 *                     (no Dot escaping needed).  Can be 0.
	 * \param style        Dot style of the edge (e.g., "dotted" or "solid").
	 * \param fout         output stream.
	 */
	void output_edge(int source, int destination, char const * label, char const * style, FILE * fout);

	/** Output a reduction.
	 * \param s            current state
	 * \param reds         the set of reductions
	 * \param fout         output stream.
	 */
	void output_red(state const * s, reductions const * reds, FILE * fout);

	/** End a Dot graph.
	 *
	 * \param fout  output stream.
	 */
	void finish_graph(FILE * fout);
//
#include <progname.h>
#include <textstyle.h>
