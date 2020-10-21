/* Input parser for Bison

   Copyright (C) 2000-2003, 2005-2007, 2009-2015, 2018-2020 Free Software Foundation, Inc.

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

#ifndef READER_H_
#define READER_H_

typedef struct merger_list {
	struct merger_list* next;
	uniqstr name;
	uniqstr type;
	Location type_declaration_loc;
} merger_list;

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

extern merger_list * merge_functions;

/* Was %union seen?  */
extern bool union_seen;

/* Should rules have a default precedence?  */
extern bool default_prec;

#endif /* !READER_H_ */
