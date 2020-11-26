//
//  gravity_codegen.c
//  gravity
//
//  Created by Marco Bambini on 09/10/14.
//  Copyright (c) 2014 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

//typedef marray_t(gnode_class_decl_t *)  gnode_class_r;
typedef GravityArray<gnode_class_decl_t *>  gnode_class_r;

struct codegen_t {
	codegen_t(gravity_vm * pVm) : vm(pVm), lasterror(0)
	{
		// @ctr context.m = 0;
		// @ctr context.n = 0;
		// @ctr context.p = 0;
	}
	GravityArray <gravity_class_t *> context;
	gnode_class_r superfix;
	uint32 lasterror;
	gravity_vm * vm;
};

typedef struct codegen_t codegen_t;

#define CONTEXT_PUSH(x)                 ((codegen_t *)self->data)->context.insert(reinterpret_cast<gravity_class_t *>(x))
#define CONTEXT_POP()                   (static_cast<codegen_t *>(self->data)->context.pop())
#define CONTEXT_GET()                   (static_cast<codegen_t *>(self->data)->context.getLast())
#define CONTEXT_IS_MODULE(x)            ((x)->IsFunction() && sstreq(((gravity_function_t *)x)->identifier, INITMODULE_NAME))

#define DECLARE_CONTEXT()               gravity_class_t * context_object = CONTEXT_GET();
#define DECLARE_FUNCTION_CONTEXT()      DECLARE_CONTEXT();                                                                      \
	if(!context_object || !context_object->IsFunction()) { report_error(self, node, "Invalid code context."); return;} \
	gravity_function_t * context_function = reinterpret_cast<gravity_function_t *>(context_object);
#define DECLARE_CLASS_CONTEXT()         DECLARE_CONTEXT();                                                                      \
	assert(context_object->IsClass());                                               \
	gravity_class_t * context_class = reinterpret_cast<gravity_class_t *>(context_object);
#define DECLARE_CODE()                  DECLARE_FUNCTION_CONTEXT(); ircode_t * code = (ircode_t *)context_function->U.Nf.bytecode;

#define IS_IMPLICIT_SELF(_expr)         (gnode_t::IsA(_expr, NODE_IDENTIFIER_EXPR) &&                                               \
	(((gnode_identifier_expr_t*)_expr)->location.type == LOCATION_CLASS_IVAR_SAME) &&      \
	(((gnode_identifier_expr_t*)_expr)->location.nup == 0) &&                              \
	(((gnode_identifier_expr_t*)_expr)->location.index == UINT16_MAX))

#define IS_SUPER(_expr)                 (gnode_t::IsA(_expr, NODE_KEYWORD_EXPR) && (((gnode_keyword_expr_t*)_expr)->Token.type == TOK_KEY_SUPER))
#define GET_VM()                        (static_cast<codegen_t *>(self->data)->vm)
#define IS_LAST_LOOP(n1, n2)             (n1+1==n2)
#define LINE_NUMBER(__node)             ((node) ? (__node)->Token.lineno : 0)

#if 0
	#define CODEGEN_COUNT_REGISTERS(_n)                     uint32 _n = ircode_register_count(code)
	#define CODEGEN_ASSERT_REGISTERS(_n1, _n2, _v)            assert(_n2 -_n1 == (_v))
#else
	#define CODEGEN_COUNT_REGISTERS(_n)
	#define CODEGEN_ASSERT_REGISTERS(_n1, _n2, _v)
#endif

// MARK: -
static void report_error(gvisitor_t * self, const gnode_t * node, const char * format, ...) 
{
	// increment internal error counter
	++self->nerr;
	// check last error line in order to prevent to emit multiple errors for the same row
	codegen_t * current = (codegen_t *)self->data;
	if(node && node->Token.lineno != current->lasterror) {
		current->lasterror = node->Token.lineno;
		// get error callback (if any)
		void * data = (self->delegate) ? ((gravity_delegate_t *)self->delegate)->xdata : NULL;
		gravity_error_callback error_fn = (self->delegate) ? ((gravity_delegate_t *)self->delegate)->error_callback : NULL;
		// build error message
		char buffer[1024];
		va_list arg;
		if(format) {
			va_start(arg, format);
			vsnprintf(buffer, sizeof(buffer), format, arg);
			va_end(arg);
		}
		// setup error struct
		GravityErrorDescription error_desc((node) ? node->Token.lineno : 0, (node) ? node->Token.colno : 0, (node) ? node->Token.fileid : 0, (node) ? node->Token.position : 0);
		// finally call error callback
		if(error_fn) 
			error_fn(NULL, GRAVITY_ERROR_SEMANTIC, buffer, &error_desc, data);
		else 
			printf("%s\n", buffer);
	}
}

// MARK: -
static opcode_t token2opcode(gtoken_t op) 
{
	switch(op) {
		// BIT
		case TOK_OP_SHIFT_LEFT: return LSHIFT;
		case TOK_OP_SHIFT_RIGHT: return RSHIFT;
		case TOK_OP_BIT_NOT: return BNOT;
		case TOK_OP_BIT_AND: return BAND;
		case TOK_OP_BIT_OR: return BOR;
		case TOK_OP_BIT_XOR: return BXOR;

		// MATH
		case TOK_OP_ADD: return ADD;
		case TOK_OP_SUB: return SUB;
		case TOK_OP_DIV: return DIV;
		case TOK_OP_MUL: return MUL;
		case TOK_OP_REM: return REM;
		// NEG not handled here

		// COMPARISON
		case TOK_KEY_ISA: return ISA;
		case TOK_OP_LESS: return LT;
		case TOK_OP_GREATER: return GT;
		case TOK_OP_LESS_EQUAL: return LEQ;
		case TOK_OP_GREATER_EQUAL: return GEQ;
		case TOK_OP_ISEQUAL: return EQ;
		case TOK_OP_ISNOTEQUAL: return NEQ;
		case TOK_OP_ISIDENTICAL: return EQQ;
		case TOK_OP_ISNOTIDENTICAL: return NEQQ;
		case TOK_OP_PATTERN_MATCH: return MATCH;
		// LOGICAL
		case TOK_OP_AND: return AND;
		case TOK_OP_NOT: return NOT;
		case TOK_OP_OR: return OR;
		default: assert(0); break; // should never reach this point
	}
	assert(0); // should never reach this point
	return NOT;
}

static GravityValue literal2value(gnode_literal_expr_t * node) 
{
	if(node->type == LITERAL_STRING) 
		return VALUE_FROM_STRING(NULL, node->value.str, node->len);
	else if(node->type == LITERAL_FLOAT) 
		return GravityValue::from_float(node->value.d);
	else if(node->type == LITERAL_INT) 
		return GravityValue::from_int(node->value.n64);
	else if(node->type == LITERAL_BOOL) 
		return GravityValue::from_bool(node->value.n64);
	else
		return GravityValue::from_null();
}

#if 0
static gravity_list_t * literals2list(gvisitor_t * self, GravityArray <gnode_t *> * r, uint32 start, uint32 stop) 
{
	gravity_list_t * list = gravity_list_new(GET_VM(), stop-start);
	for(uint32 i = start; i<stop; ++i) {
		gnode_literal_expr_t * node = (gnode_literal_expr_t *)r->at(i);
		GravityValue value = literal2value(node);
		list->array.insert(value);
	}
	return list;
}

static gravity_map_t * literals2map(gvisitor_t * self, GravityArray <gnode_t *> * r1, GravityArray <gnode_t *> * r2, uint32 start, uint32 stop) 
{
	gravity_map_t * map = gravity_map_new(GET_VM(), stop-start);
	for(uint32 i = start; i<stop; ++i) {
		gnode_literal_expr_t * _key = (gnode_literal_expr_t *)r1->at(i);
		gnode_literal_expr_t * _value = (gnode_literal_expr_t *)r2->at(i);
		// when here I am sure that both key and value are literals
		// so they can be LITERAL_STRING, LITERAL_FLOAT, LITERAL_INT, LITERAL_BOOL
		GravityValue key = literal2value(_key);
		GravityValue value = literal2value(_value);
		gravity_map_insert(NULL, map, key, value);
	}

	return map;
}

static gravity_map_t * enum2map(gvisitor_t * self, gnode_enum_decl_t * node) 
{
	uint32 count = symboltable_count(node->symtable, 0);
	gravity_map_t * map = gravity_map_new(GET_VM(), count);
	// FixMe
	return map;
}

static bool check_literals_list(gvisitor_t * self, gnode_list_expr_t * node, bool ismap, size_t idxstart, size_t idxend, uint32 dest) 
{
	DEBUG_CODEGEN("check_literal_list_expr");
	DECLARE_CODE();
	// first check if all nodes inside this chuck are all literals
	// and in this case apply a more efficient SETLIST variant
	for(size_t j = idxstart; j < idxend; ++j) {
		gnode_t * e = gnode_array_get(node->list1, j);
		if(!gnode_is_literal(e)) return false;
		if(ismap) {
			// additional check on key that must be a string literal in case of a map
			if(!gnode_is_literal_string(e)) 
				return false;
			e = gnode_array_get(node->list2, j);
			if(!gnode_is_literal(e)) 
				return false;
		}
	}

	// emit optimized (cpoll based) version
	GravityValue v;

	if(ismap) {
		gravity_map_t * map = literals2map(self, node->list1, node->list2, (uint32)idxstart, (uint32)idxend);
		v = GravityValue::from_object(map);
	}
	else {
		gravity_list_t * list = literals2list(self, node->list1, (uint32)idxstart, (uint32)idxend);
		v = GravityValue::from_object(list);
	}
	uint16 index = gravity_function_cpool_add(GET_VM(), context_function, v);
	ircode_add(code, SETLIST, dest, 0, index);

	return true;
}
#endif

static uint32 node2index(gnode_t * node) 
{
	// node can be a VARIABLE declaration or a local IDENTIFIER
	if(gnode_t::IsA(node, NODE_VARIABLE_DECL)) {
		gnode_variable_decl_t * expr = (gnode_variable_decl_t*)node;
		assert(gnode_array_size(expr->decls) == 1);
		gnode_var_t * var = (gnode_var_t *)gnode_array_get(expr->decls, 0);
		return var->index;
	}
	if(gnode_t::IsA(node, NODE_IDENTIFIER_EXPR)) {
		gnode_identifier_expr_t * expr = (gnode_identifier_expr_t*)node;
		assert(expr->location.type == LOCATION_LOCAL);
		return expr->location.index;
	}
	assert(0); // should never reach this point because semacheck2 should take care of the check
	return UINT32_MAX;
}

static void fix_superclasses(gvisitor_t * self) 
{
	// this function cannot fail because superclasses was already checked in samecheck2 so I am sure that they exist somewhere
	codegen_t * data = (codegen_t *)self->data;
	gnode_class_r * superfix = &data->superfix;
	const uint count = gnode_array_size(superfix);
	for(uint i = 0; i < count; ++i) {
		gnode_class_decl_t * node = (gnode_class_decl_t *)gnode_array_get(superfix, i);
		gnode_class_decl_t * super = (gnode_class_decl_t *)node->superclass;
		gravity_class_t * c = static_cast<gravity_class_t *>(node->data);
		gravity_class_setsuper(c, static_cast<gravity_class_t *>(super->data));
	}
}

static const char * lookup_superclass_identifier(gvisitor_t * self, gravity_class_t * c) 
{
	codegen_t * data = (codegen_t *)self->data;
	gnode_class_r * superfix = &data->superfix;
	const uint count = gnode_array_size(superfix);
	for(uint i = 0; i < count; ++i) {
		gnode_class_decl_t * node = (gnode_class_decl_t *)gnode_array_get(superfix, i);
		gravity_class_t * target = static_cast<gravity_class_t *>(node->data);
		if(target == c) {
			gnode_class_decl_t * super = (gnode_class_decl_t *)node->superclass;
			return (super) ? super->identifier : NULL;
		}
	}
	return NULL;
}

static gravity_class_t * context_get_class(gvisitor_t * self) 
{
	GravityArray <gravity_class_t *> ctx = ((codegen_t *)self->data)->context;
	size_t len = ctx.getCount();
	for(int i = (int)len-1; i>=0; --i) {
		gravity_class_t * context_object = ctx.at(i);
		if(context_object->IsClass()) 
			return context_object;
	}
	return NULL;
}

static bool node_is_closure(gnode_t * node, uint16 * local_index) 
{
	DEBUG_CODEGEN("node_is_closure");
	// init local_index to 0 (means not a local index)
	ASSIGN_PTR(local_index, 0);
	if(node && gnode_t::IsA(node, NODE_IDENTIFIER_EXPR)) {
		gnode_identifier_expr_t * identifier = (gnode_identifier_expr_t*)node;
		node = identifier->symbol;
	}
	if(node && gnode_t::IsA(node, NODE_VARIABLE)) {
		gnode_var_t * var = (gnode_var_t *)node;
		ASSIGN_PTR(local_index, var->index);
		node = var->expr;
	}
	if(node && gnode_t::IsA(node, NODE_FUNCTION_DECL)) {
		gnode_function_decl_t * f = (gnode_function_decl_t*)node;
		return f->is_closure;
	}
	return false;
}

// this function can be called ONLY from visit_postfix_expr where a context has been pushed
static uint32 compute_self_register(gvisitor_t * self, ircode_t * code, gnode_t * node, uint32 target_register, GravityArray <gnode_t *> * list) 
{
	DEBUG_CODEGEN("compute_self_register");
	// check for special implicit self slot
	if(IS_IMPLICIT_SELF(node)) 
		return 0;
	// check for super keyword
	if(IS_SUPER(node)) 
		return 0;
	// examine next node in list
	gnode_postfix_subexpr_t * next = (gnode_array_size(list) > 0) ? (gnode_postfix_subexpr_t*)gnode_array_get(list, 0) : NULL;
	bool next_is_access = (next && next->Tag == NODE_ACCESS_EXPR);
	// if node refers to an outer class then load outer class from hidden _outer ivar and return its register
	if((gnode_t::IsA(node, NODE_IDENTIFIER_EXPR) && ((gnode_identifier_expr_t*)node)->location.type == LOCATION_CLASS_IVAR_OUTER) && !next_is_access) {
		gnode_identifier_expr_t * expr = (gnode_identifier_expr_t*)node;
		uint32 dest = ircode_register_push_temp(code);
		uint32 target = 0;
		for(uint16 i = 0; i<expr->location.nup; ++i) {
			ircode_add(code, LOAD, dest, target, 0 + MAX_REGISTERS, LINE_NUMBER(expr));
			target = dest;
		}
		uint32 reg = ircode_register_pop_context_protect(code, true);
		if(reg == REGISTER_ERROR) {
			report_error(self, node, "Unexpected register error.");
			return UINT32_MAX;
		}
		else
			return reg;
	}
	// no special register found
	// check if node is a closure defined inside a class context (so self is needed)
	uint16 local_index = 0;
	if(node_is_closure(node, &local_index)) {
		if(next_is_access) 
			return local_index;
		else if(context_get_class(self) != NULL) 
			return 0;
	}
	// default case is to return the target register
	return target_register;
}

// MARK: - Statements -

static void visit_list_stmt(gvisitor_t * self, gnode_compound_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_list_stmt");
	gnode_array_each(node->stmts, {gvisit(self, val);});
}

static void visit_compound_stmt(gvisitor_t * self, gnode_compound_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_compound_stmt");
	gnode_array_each(node->stmts, {
		gvisit(self, val);
		// check if context is a function
		DECLARE_CONTEXT();
		bool is_func_ctx = context_object->IsFunction();
		if(!is_func_ctx) 
			continue;
		// in case of function context cleanup temporary registers
		gravity_function_t * f = (gravity_function_t *)context_object;
		ircode_t * code = (ircode_t *)f->U.Nf.bytecode;
		ircode_register_temps_clear(code);
	});

	if(node->nclose != UINT32_MAX) {
		DECLARE_CODE();
		ircode_add(code, CLOSE, node->nclose, 0, 0, LINE_NUMBER(node));
	}
}

static void visit_label_stmt(gvisitor_t * self, gnode_label_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_label_stmt");
	gtoken_t type = node->GetTokenType();
	assert(oneof2(type, TOK_KEY_DEFAULT, TOK_KEY_CASE));
	if(type == TOK_KEY_DEFAULT) {
		gvisit(self, node->stmt);
	}
	else if(type == TOK_KEY_CASE) {
		gvisit(self, node->expr); gvisit(self, node->stmt);
	}
}

static void visit_flow_if_stmt(gvisitor_t * self, gnode_flow_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_flow_if_stmt");
	DECLARE_CODE();
	/*
	     <condition>
	     if-false: goto $end
	     <then-part>
	     goto $true
	     $end:
	     <else-part>
	     $true:

	     TRUE  := getLabel
	     FALSE := getLabel
	     compile condition
	     emit ifeq FALSE
	     compile stm1
	     emit goto TRUE
	     emit FALSE
	     compile stm2
	     emit TRUE
	 */
	uint32 labelTrue  = ircode_newlabel(code);
	uint32 labelFalse = ircode_newlabel(code);
	gvisit(self, node->cond);
	uint32 reg = ircode_register_pop(code);
	if(reg == REGISTER_ERROR) 
		report_error(self, node, "Invalid if condition expression.");
	ircode_add(code, JUMPF, reg, labelFalse, 0, LINE_NUMBER(node));
	gvisit(self, node->stmt);
	if(node->elsestmt) ircode_add(code, JUMP, labelTrue, 0, 0, LINE_NUMBER(node));
	ircode_marklabel(code, labelFalse, LINE_NUMBER(node));
	if(node->elsestmt) {
		gvisit(self, node->elsestmt);
		ircode_marklabel(code, labelTrue, LINE_NUMBER(node));
	}
}

static void visit_flow_switch_stmt(gvisitor_t * self, gnode_flow_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_flow_switch_stmt");
	return;
	gvisit(self, node->cond);
	gvisit(self, node->stmt);
}

static void visit_flow_stmt(gvisitor_t * self, gnode_flow_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_flow_stmt");
	gtoken_t type = node->GetTokenType();
	assert((type == TOK_KEY_IF) || (type == TOK_KEY_SWITCH) || (type == TOK_OP_TERNARY));
	if(type == TOK_KEY_IF)
		visit_flow_if_stmt(self, node);
	else if(type == TOK_KEY_SWITCH)
		visit_flow_switch_stmt(self, node);
	else if(type == TOK_OP_TERNARY)
		report_error(self, node, "Ternary operator not yet supported.");
}

static void visit_loop_while_stmt(gvisitor_t * self, gnode_loop_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_loop_while_stmt");
	DECLARE_CODE();
	/*
	     $start:    <condition>
	     if-false: goto $end
	     <body>
	     goto $start
	     $end:

	     START := getLabel
	     END  := getLabel
	     emit START
	     compile exp
	     emit ifeq END
	     compile stm
	     emit goto START
	     emit END
	 */
	uint32 labelTrue  = ircode_newlabel(code);
	uint32 labelFalse = ircode_newlabel(code);
	uint32 labelCheck = ircode_newlabel(code);
	ircode_setlabel_true(code, labelTrue);
	ircode_setlabel_false(code, labelFalse);
	ircode_setlabel_check(code, labelCheck);
	ircode_marklabel(code, labelTrue, LINE_NUMBER(node));
	ircode_marklabel(code, labelCheck, LINE_NUMBER(node));
	gvisit(self, node->cond);
	uint32 reg = ircode_register_pop(code);
	if(reg == REGISTER_ERROR) 
		report_error(self, node, "Invalid while condition expression.");
	ircode_add(code, JUMPF, reg, labelFalse, 0, LINE_NUMBER(node));
	gvisit(self, node->stmt);
	ircode_add(code, JUMP, labelTrue, 0, 0, LINE_NUMBER(node));
	ircode_marklabel(code, labelFalse, LINE_NUMBER(node));
	ircode_unsetlabel_true(code);
	ircode_unsetlabel_false(code);
	ircode_unsetlabel_check(code);
}

static void visit_loop_repeat_stmt(gvisitor_t * self, gnode_loop_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_loop_repeat_stmt");
	DECLARE_CODE();
	/*
	     $start:    <body>
	     <expression>
	     if-false: goto $start
	     $end:
	 */

	uint32 labelTrue  = ircode_newlabel(code);
	uint32 labelFalse = ircode_newlabel(code); // end label is necessary to handle optional break statement
	uint32 labelCheck = ircode_newlabel(code);
	ircode_setlabel_true(code, labelTrue);
	ircode_setlabel_false(code, labelFalse);
	ircode_setlabel_check(code, labelCheck);

	ircode_marklabel(code, labelTrue, LINE_NUMBER(node));
	ircode_marklabel(code, labelCheck, LINE_NUMBER(node));
	gvisit(self, node->stmt);
	gvisit(self, node->expr);
	uint32 reg = ircode_register_pop(code);
	if(reg == REGISTER_ERROR) 
		report_error(self, node, "Invalid repeat condition expression.");
	ircode_add(code, JUMPF, reg, labelFalse, 0, LINE_NUMBER(node));
	ircode_add(code, JUMP, labelTrue, 0, 0, LINE_NUMBER(node));
	ircode_marklabel(code, labelFalse, LINE_NUMBER(node));
	ircode_unsetlabel_true(code);
	ircode_unsetlabel_false(code);
	ircode_unsetlabel_check(code);
}

static void visit_loop_for_stmt(gvisitor_t * self, gnode_loop_stmt_t * node) 
{
	// https://www.natashatherobot.com/swift-alternatives-to-c-style-for-loops/

	DEBUG_CODEGEN("visit_loop_for_stmt");
	DECLARE_CODE();

	// FOR loop is transformed to a WHILE loop
	//
	// from:
	// for (cond in expr) {
	//    stmp;
	// }
	//
	// to:
	// {
	//    var $expr = expr;
	//    var $value = $expr.iterate(null);
	//    while ($value) {
	//        cond = $expr.next($value);
	//        stmp;
	//        $value = $expr.iterate($value);
	//    }
	// }

	// $expr and $value are temporary registers that must be protected, otherwise
	// in visit_compound_statement, all temp registers are released within ircode_register_temps_clear
	uint32 $expr = ircode_register_push_temp_protected(code); // ++TEMP => 1
	uint32 $value = ircode_register_push_temp_protected(code); // ++TEMP => 2

	// get cpool index for the required methods
	uint16 iterate_idx = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, ITERATOR_INIT_FUNCTION));
	uint16 next_idx = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, ITERATOR_NEXT_FUNCTION));
	uint32 cond_idx = node2index(node->cond);

	// generate code for $expr = expr (so expr is only evaluated once)
	gvisit(self, node->expr);
	uint32 once_expr = ircode_register_pop(code);
	if(once_expr == REGISTER_ERROR) 
		report_error(self, node, "Invalid for expression.");
	ircode_add(code, MOVE, $expr, once_expr, 0, LINE_NUMBER(node));

	// generate code for $value = $expr.iterate(null);
	uint32 iterate_fn = ircode_register_push_temp_protected(code); // ++TEMP => 3
	ircode_add(code, LOADK, iterate_fn, iterate_idx, 0, LINE_NUMBER(node));
	ircode_add(code, LOAD, iterate_fn, $expr, iterate_fn, LINE_NUMBER(node));

	uint32 next_fn = ircode_register_push_temp_protected(code);   // ++TEMP => 4
	ircode_add(code, LOADK, next_fn, next_idx, 0, LINE_NUMBER(node));
	ircode_add(code, LOAD, next_fn, $expr, next_fn, LINE_NUMBER(node));

	uint32 temp1 = ircode_register_push_temp(code);               // ++TEMP => 5
	ircode_add(code, MOVE, temp1, iterate_fn, 0, LINE_NUMBER(node));
	uint32 temp2 = ircode_register_push_temp(code);               // ++TEMP => 6
	ircode_add(code, MOVE, temp2, $expr, 0, LINE_NUMBER(node));
	uint32 temp3 = ircode_register_push_temp(code);               // ++TEMP => 7
	ircode_add(code, LOADK, temp3, CPOOL_VALUE_NULL, 0, LINE_NUMBER(node));
	ircode_add(code, CALL, $value, temp1, 2, LINE_NUMBER(node));
	uint32 temp = ircode_register_pop(code);                      // --TEMP => 6
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 5
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 4
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");

	// while code
	uint32 labelTrue  = ircode_newlabel(code);
	uint32 labelFalse = ircode_newlabel(code);
	uint32 labelCheck = ircode_newlabel(code);
	ircode_setlabel_true(code, labelTrue);
	ircode_setlabel_false(code, labelFalse);
	ircode_setlabel_check(code, labelCheck);

	ircode_marklabel(code, labelTrue, LINE_NUMBER(node));
	ircode_add(code, JUMPF, $value, labelFalse, 1, LINE_NUMBER(node)); // flag JUMPF instruction to check ONLY BOOL values

	// cond = $expr.next($value);
	// cond is a local variable
	temp1 = ircode_register_push_temp_protected(code);              // ++TEMP => 5
	ircode_add(code, MOVE, temp1, next_fn, 0, LINE_NUMBER(node));
	temp2 = ircode_register_push_temp_protected(code);              // ++TEMP => 6
	ircode_add(code, MOVE, temp2, $expr, 0, LINE_NUMBER(node));
	temp3 = ircode_register_push_temp_protected(code);              // ++TEMP => 7
	ircode_add(code, MOVE, temp3, $value, 0, LINE_NUMBER(node));
	ircode_add(code, CALL, cond_idx, temp1, 2, LINE_NUMBER(node));

	// process statement
	gvisit(self, node->stmt);

	// temp registers can now be released
	ircode_register_temp_unprotect(code, temp3);
	ircode_register_temp_unprotect(code, temp2);
	ircode_register_temp_unprotect(code, temp1);

	// pop next_fn temp register AFTER user code because function ptr must be protected inside loop
	temp = ircode_register_pop(code);                               // --TEMP => 6
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 5
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 4
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");

	ircode_marklabel(code, labelCheck, LINE_NUMBER(node));
	// update $value for the next check
	// $value = $expr.iterate($value);
	temp1 = ircode_register_push_temp(code);                        // ++TEMP => 5
	ircode_add(code, MOVE, temp1, iterate_fn, 0, LINE_NUMBER(node));
	temp2 = ircode_register_push_temp(code);                        // ++TEMP => 6
	ircode_add(code, MOVE, temp2, $expr, 0, LINE_NUMBER(node));
	temp2 = ircode_register_push_temp(code);                        // ++TEMP => 7
	ircode_add(code, MOVE, temp2, $value, 0, LINE_NUMBER(node));
	ircode_add(code, CALL, $value, temp1, 2, LINE_NUMBER(node));
	temp = ircode_register_pop(code);                               // --TEMP => 6
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 5
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 4
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");

	ircode_add(code, JUMP, labelTrue, 0, 0, LINE_NUMBER(node));

	ircode_marklabel(code, labelFalse, LINE_NUMBER(node));

	ircode_unsetlabel_true(code);
	ircode_unsetlabel_false(code);
	ircode_unsetlabel_check(code);

	temp = ircode_register_pop(code);                               // --TEMP => 3
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 2
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 1
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
	temp = ircode_register_pop(code);                               // --TEMP => 0
	DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");

	// mark main for registers as reusable
	ircode_register_temp_unprotect(code, $expr);
	ircode_register_temp_unprotect(code, $value);
	ircode_register_temp_unprotect(code, iterate_fn);
	ircode_register_temp_unprotect(code, next_fn);

	if(node->nclose != UINT32_MAX) {
		ircode_add(code, CLOSE, node->nclose, 0, 0, LINE_NUMBER(node));
	}
}

static void visit_loop_stmt(gvisitor_t * self, gnode_loop_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_loop_stmt");
	gtoken_t type = node->GetTokenType();
	assert((type == TOK_KEY_WHILE) || (type == TOK_KEY_REPEAT) || (type == TOK_KEY_FOR));
	if(type == TOK_KEY_WHILE) {
		visit_loop_while_stmt(self, node);
	}
	else if(type == TOK_KEY_REPEAT) {
		visit_loop_repeat_stmt(self, node);
	}
	else if(type == TOK_KEY_FOR) {
		visit_loop_for_stmt(self, node);
	}
}

static void visit_jump_stmt(gvisitor_t * self, gnode_jump_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_jump_stmt");
	DECLARE_CODE();
	gtoken_t type = node->GetTokenType();
	assert((type == TOK_KEY_BREAK) || (type == TOK_KEY_CONTINUE) || (type == TOK_KEY_RETURN));
	if(type == TOK_KEY_BREAK) {
		uint32 label = ircode_getlabel_false(code);
		ircode_add(code, JUMP, label, 0, 0, LINE_NUMBER(node)); // goto $end;
	}
	else if(type == TOK_KEY_CONTINUE) {
		uint32 label = ircode_getlabel_check(code);
		ircode_add(code, JUMP, label, 0, 0, LINE_NUMBER(node)); // goto $start;
	}
	else if(type == TOK_KEY_RETURN) {
		if(node->expr) {
			gvisit(self, node->expr);
			uint32 reg = ircode_register_pop(code);
			if(reg == REGISTER_ERROR) 
				report_error(self, node, "Invalid return expression.");
			ircode_add(code, RET, reg, 0, 0, LINE_NUMBER(node));
		}
		else {
			ircode_add(code, RET0, 0, 0, 0, LINE_NUMBER(node));
		}
	}
}

static void visit_empty_stmt(gvisitor_t * self, gnode_empty_stmt_t * node) 
{
	DEBUG_CODEGEN("visit_empty_stmt");
	DECLARE_CODE();
	ircode_add(code, NOP, 0, 0, 0, LINE_NUMBER(node));
}

// MARK: - Declarations -

static void store_declaration(gvisitor_t * self, gravity_class_t * obj, bool is_static, gnode_function_decl_t * node) 
{
	DEBUG_CODEGEN("store_object_declaration");
	assert(obj);
	DECLARE_CONTEXT();
	bool is_module = CONTEXT_IS_MODULE(context_object);
	bool is_class = context_object->IsClass();
	bool is_local = (!is_module && !is_class);
	if(is_static && !is_class) {
		// static makes sense only for class objects
		report_error(self, node, "Static storage specifier does not make sense in non class context.");
		return;
	}
	else if(is_local || is_module) {
		gravity_function_t * context_function = (gravity_function_t *)context_object;
		ircode_t * code = (ircode_t *)context_function->U.Nf.bytecode;
		// add object to cpool and get its index
		uint16 index = gravity_function_cpool_add(NULL, context_function, GravityValue::from_object(obj));
		// if it is a function then generate a CLOSURE opcode instead of LOADK
		if(obj->IsFunction()) {
			assert(node);
			gravity_function_t * f = (gravity_function_t *)obj;
			uint32 regnum = ircode_register_push_temp(code);
			ircode_add(code, CLOSURE, regnum, index, 0, LINE_NUMBER(node));
			uint32 upindex = 0;
			for(uint16 i = 0; i<f->nupvalues; ++i) {
				gupvalue_t * upvalue = gnode_array_get(node->uplist, i);
				uint32 opindex = (upvalue->is_direct) ? upvalue->index : upindex++;
				ircode_add(code, MOVE, opindex, (upvalue->is_direct) ? 1 : 0, 0, LINE_NUMBER(node));
			}
		}
		else {
			ircode_add_constant(code, index, LINE_NUMBER(node));
		}
		if(is_module && obj->identifier) {
			index = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, obj->identifier));
			uint32 reg = ircode_register_pop(code);
			if(reg == REGISTER_ERROR) 
				report_error(self, node, "Invalid declaration expression.");
			ircode_add(code, STOREG, reg, index, 0, LINE_NUMBER(node));
		}
		return;
	}
	else if(is_class) {
		gravity_class_t * context_class = context_object;
		context_class = (is_static) ? context_class->objclass : context_class;
		gravity_class_bind(context_class, obj->identifier, GravityValue::from_object(obj));
		return;
	}
	else {
		assert(0); // should never reach this point
	}
}

// used ONLY by CODEGEN
static gravity_function_t * class_lookup_nosuper(gravity_class_t * c, const char * name) 
{
	STATICVALUE_FROM_STRING(key, name, strlen(name));
	GravityValue * v = gravity_hash_lookup(c->htable, key);
	return v ? reinterpret_cast<gravity_function_t *>(v->Ptr) : NULL;
}

static void process_constructor(gvisitor_t * self, gravity_class_t * c, gnode_t * node) 
{
	DEBUG_CODEGEN("process_constructor");
	// $init is an internal function used to initialize instance variables to a default value
	// in case of subclasses USER is RESPONSIBLE to call super.init();
	// in case of subclasses COMPILER is RESPONSIBLE to create the appropriate $init call chain

	// check internal $init function
	gravity_function_t * internal_init_function = class_lookup_nosuper(c, CLASS_INTERNAL_INIT_NAME);
	// check for user constructor function
	gravity_function_t * constructor_function = class_lookup_nosuper(c, CLASS_CONSTRUCTOR_NAME);
	// build appropriate $init function
	gravity_class_t * super = c->superclass;
	uint32 ninit = 2;
	while(super) {
		gravity_function_t * super_init = class_lookup_nosuper(super, CLASS_INTERNAL_INIT_NAME);
		if(super_init) {
			// copy super init code to internal_init code
			SETIFZ(internal_init_function, gravity_function_new(NULL, CLASS_INTERNAL_INIT_NAME, 1, 0, 0, ircode_create(1)));
			// build unique internal init name ($init2, $init3 and so on)
			char name[256];
			snprintf(name, sizeof(name), "%s%d", CLASS_INTERNAL_INIT_NAME, ninit++);
			// add new internal init to class and call it from main $init function
			// super_init should not be duplicated here because class hash table values are not freed (only
			// keys are freed)
			gravity_class_bind(c, name, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(super_init)));
			uint16 index = gravity_function_cpool_add(NULL, internal_init_function, gravity_zstring_to_value(GET_VM(), name));
			ircode_patch_init((ircode_t *)internal_init_function->U.Nf.bytecode, index);
		}
		super = super->superclass;
	}
	// 4 cases to handle:

	// 1. internal_init and constuctor are not present, so nothing to do here
	if((!internal_init_function) && (!constructor_function)) 
		goto check_meta;
//    // 2. internal init is present and constructor is not used
//    if ((internal_init_function) && (!constructor_function)) {
//        // add a RET0 command
//        ircode_t *code = (ircode_t *)internal_init_function->bytecode;
//        ircode_add(code, RET0, 0, 0, 0);
//
//        // bind internal init as constructor
//        gravity_class_bind(c, CLASS_CONSTRUCTOR_NAME, GravityValue::from_object(internal_init_function));
//        gravity_object_setenclosing((gravity_class_t *)internal_init_function, (gravity_class_t *)c);
//        goto process_funcs;
//    }

	// 3. internal init is present so constructor is mandatory
	if(internal_init_function) {
		// convert ircode to bytecode for $init special function and add a RET0 command
		ircode_t * code = (ircode_t *)internal_init_function->U.Nf.bytecode;
		if(!code) {
			report_error(self, node, "Invalid code context."); return;
		}
		ircode_add(code, RET0, 0, 0, 0, LINE_NUMBER(node));
		if(constructor_function == NULL) {
			constructor_function = gravity_function_new(NULL, CLASS_CONSTRUCTOR_NAME, 1, 0, 2, ircode_create(1));
			ircode_t * code2 = (ircode_t *)constructor_function->U.Nf.bytecode;
			ircode_add_skip(code2, LINE_NUMBER(node));      // LOADK
			ircode_add_skip(code2, LINE_NUMBER(node));      // LOAD
			ircode_add_skip(code2, LINE_NUMBER(node));      // MOVE
			ircode_add_skip(code2, LINE_NUMBER(node));      // CALL
			gravity_class_bind(c, CLASS_CONSTRUCTOR_NAME, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(constructor_function)));
		}
	}

	// 4. constructor is present so internal init is optional
	if(constructor_function) {
		// add an implicit RET 0 (RET self) to the end of the constructor
		ircode_t * code = (ircode_t *)constructor_function->U.Nf.bytecode;
		if(!code) {
			report_error(self, node, "Invalid code context."); return;
		}
		ircode_add(code, RET, 0, 0, 0, LINE_NUMBER(node));
		if(internal_init_function) {
			// if an internal init function is present ($init) then add a call to it as a first instruction
			uint16 index = gravity_function_cpool_add(GET_VM(), constructor_function, gravity_zstring_to_value(NULL, CLASS_INTERNAL_INIT_NAME));
			// load constant
			uint32 dest = ircode_register_push_temp(code);
			ircode_set_index(0, code, LOADK, dest, index, 0);
			// load from lookup
			ircode_set_index(1, code, LOAD, dest, 0, dest);
			// prepare parameters
			uint32 dest2 = ircode_register_push_temp(code);
			ircode_set_index(2, code, MOVE, dest2, 0, 0);
			uint32 temp = ircode_register_pop(code);
			DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
			// execute call
			ircode_set_index(3, code, CALL, dest, dest, 1);
		}
	}
	if(internal_init_function) 
		gravity_optimizer(internal_init_function, false);
	if(constructor_function) 
		gravity_optimizer(constructor_function, false);
check_meta:
	// recursively process constructor but stop when object or class class is found, otherwise an infinite loop is
	// triggered
	if((c->objclass) && (c->objclass->isa) && (c->objclass->isa != c->objclass->objclass)) 
		process_constructor(self, c->objclass, node);
}

static void process_getter_setter(gvisitor_t * self, gnode_var_t * p, gravity_class_t * c) 
{
	gnode_compound_stmt_t * expr = (gnode_compound_stmt_t*)p->expr;
	gnode_function_decl_t * getter = (gnode_function_decl_t*)gnode_array_get(expr->stmts, 0);
	gnode_function_decl_t * setter = (gnode_function_decl_t*)gnode_array_get(expr->stmts, 1);
	gnode_function_decl_t * f1[2] = {getter, setter};
	gravity_function_t * f2[2] = {NULL, NULL};
	for(uint16 i = 0; i < 2; ++i) {
		gnode_function_decl_t * node = f1[i];
		if(node) {
			// create gravity function
			uint16 nparams = (node->params) ? (uint16)node->params->getCount() : 0;
			f2[i] = gravity_function_new(NULL, NULL, nparams, node->nlocals, 0, ircode_create(node->nlocals+nparams));
			// process inner block
			CONTEXT_PUSH(f2[i]);
			gnode_compound_stmt_t * block = node->block;
			if(block) {
				gnode_array_each(block->stmts, {gvisit(self, val);});
			}
			CONTEXT_POP();
			gravity_optimizer(f2[i], self->bflag);
		}
	}
	// getter and setter NULL means default
	// since getter and setter are methods and not simple functions, do not transfer to VM
	gravity_function_t * f = gravity_function_new_special(NULL, NULL, GRAVITY_COMPUTED_INDEX, f2[0], f2[1]);
	gravity_class_bind(c, p->identifier, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(f)));
}

static void visit_function_decl(gvisitor_t * self, gnode_function_decl_t * node) 
{
	DEBUG_CODEGEN("visit_function_decl %s", node->identifier);
	// extern means it will be provided at runtime by the delegate
	if(node->storage == TOK_KEY_EXTERN) return;
	DECLARE_CONTEXT();
	bool is_class_ctx = context_object->IsClass();
	// create new function object
	uint16 nparams = (node->params) ? (uint16)node->params->getCount() : 0;
	gravity_function_t * f = gravity_function_new((is_class_ctx) ? NULL : GET_VM(), node->identifier, nparams, node->nlocals, 0, (void*)ircode_create(node->nlocals+nparams));
	// check if f is a special constructor function (init)
	// name must be CLASS_CONSTRUCTOR_NAME and context_object must be a class
	bool is_constructor = (sstreq(node->identifier, CLASS_CONSTRUCTOR_NAME) && is_class_ctx);
	// loop at each param to save name and check for default values
	// loop from 1 in order to skip implicit self argument
	const uint len = gnode_array_size(node->params);
	for(uint i = 1; i < len; ++i) {
		gnode_var_t * param = (gnode_var_t *)gnode_array_get(node->params, i);
		GravityValue name = gravity_zstring_to_value(NULL, param->identifier);
		f->U.Nf.pname.insert(name);
		if(node->has_defaults) {
			// LOOP for each node->params[i]->expr and convert literal to GravityValue
			// if expr is NULL then compute value as UNDEFINED (and add each one to f->defaults preserving
			// the index)
			gnode_literal_expr_t * literal = (gnode_literal_expr_t *)param->expr;
			GravityValue value = (literal) ? literal2value(literal) : GravityValue::from_undefined();
			f->U.Nf.pvalue.insert(value);
		}
	}
	CONTEXT_PUSH(f);
	if(is_constructor) {
		// reserve first four instructions that could be later filled with a CALL to $init
		// see process_constructor for more information
		ircode_t * code = (ircode_t *)f->U.Nf.bytecode;
		if(!code) {
			report_error(self, node, "Invalid code context."); 
			return;
		}
		ircode_add_skip(code, LINE_NUMBER(node));
		ircode_add_skip(code, LINE_NUMBER(node));
		ircode_add_skip(code, LINE_NUMBER(node));
		ircode_add_skip(code, LINE_NUMBER(node));
	}
	// process inner block
	ircode_t * code = (ircode_t *)f->U.Nf.bytecode;
	if(node->block) {
		gnode_array_each(node->block->stmts, {
			// process node
			gvisit(self, val);
			// reset temp registers after each node
			ircode_register_temps_clear(code);
		});
	}
	// check for upvalues
	if(node->uplist) 
		f->nupvalues = (uint16)gnode_array_size(node->uplist);
	// remove current function
	CONTEXT_POP();
	// check for ircode errors
	if(ircode_iserror((ircode_t *)f->U.Nf.bytecode))
		report_error(self, node, "Maximum number of available registers used in function %s.", f->identifier);
	// store function in current context
	store_declaration(self, (gravity_class_t *)f, (node->storage == TOK_KEY_STATIC), node);
	// convert ircode to bytecode (postpone optimization of the constructor)
	if(!is_constructor) 
		gravity_optimizer(f, self->bflag);
}

static void visit_variable_decl(gvisitor_t * self, gnode_variable_decl_t * node) 
{
	DEBUG_CODEGEN("visit_variable_decl");
	DECLARE_CONTEXT();
	// no initialization for extern variables since the real value will be provided at runtime
	if(node->storage == TOK_KEY_EXTERN) 
		return;
	bool is_module = CONTEXT_IS_MODULE(context_object);
	bool is_class = context_object->IsClass();
	bool is_local = ((is_module == false) && (is_class == false));
	// loop through declarations
	const uint count = gnode_array_size(node->decls);
	for(uint i = 0; i < count; ++i) {
		gnode_var_t * p = (gnode_var_t *)gnode_array_get(node->decls, i);
		DEBUG_CODEGEN("visit_variable_decl %s", p->identifier);
		// variable declarations can be specified in:
		// FUNCTION (local variable)
		// MODULE (module variable)
		// CLASS (property)
		if(is_local) {
			// it is a local variable declaration (default initialized to NULL)
			// code is generate ONLY if an init expression is specified
			//
			//    example:
			//
			//    func foo () {
			//        var a = 10;
			//    }
			//
			//    LOADI    1 10    ; move 10 into register 1
			//    MOVE    0 1        ; move register 1 into register 0
			//
			// generate expression code (if it is not a local enum)
			if(gnode_t::IsA(p->expr, NODE_ENUM_DECL)) 
				continue;
			if(p->expr) 
				gvisit(self, p->expr); // context is a function
			gravity_function_t * context_function = (gravity_function_t *)context_object;
			ircode_t * code = (ircode_t *)context_function->U.Nf.bytecode;
			if(p->expr) {
				// assign to variable result of the expression
				uint32 reg = ircode_register_pop(code);
				if(reg == REGISTER_ERROR) 
					report_error(self, node, "Invalid var expression.");
				ircode_add(code, MOVE, p->index, reg, 0, LINE_NUMBER(node));
				// Checkpoint added to support STRUCT
				ircode_add_check(code);
			}
			else {
				// no default assignment expression found so initialize to NULL
				ircode_add(code, LOADK, p->index, CPOOL_VALUE_NULL, 0, LINE_NUMBER(node));
			}
			continue;
		}

		if(is_module) {
			// it is a module variable (default initialized to null)
			// code must ALWAYS be generated
			//
			// example 1:
			//    var a;
			//
			//    LOADK    0 NULL   ; move null into register 0
			//    STOREG    0 0     ; move register 0 into hash(constant_pool(0))
			//
			// example 2:
			//    var a = 10;
			//
			//    LOADI    0 10     ; move 10 into register 0
			//    STOREG    0 0     ; move register 0 into hash(constant_pool(0))
			//
			gravity_function_t * context_function = (gravity_function_t *)context_object;
			ircode_t * code = (ircode_t *)context_function->U.Nf.bytecode;
			uint16 index = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, p->identifier));
			if(p->expr) {
				gvisit(self, p->expr); // context is a function
			}
			else {
				ircode_add_constant(code, CPOOL_VALUE_NULL, LINE_NUMBER(node));
			}
			uint32 reg = ircode_register_pop(code);
			if(reg == REGISTER_ERROR) 
				report_error(self, node, "Invalid var expression.");
			ircode_add(code, STOREG, reg, index, 0, LINE_NUMBER(node));
			continue;
		}

		if(is_class) {
			bool is_static = (node->storage == TOK_KEY_STATIC);
			gravity_class_t * context_class = (is_static) ? ((gravity_class_t *)context_object)->objclass : (gravity_class_t *)context_object;
			// check computed property case first
			if((p->expr) && (p->iscomputed)) {
				process_getter_setter(self, p, context_class);
				continue;
			}

			// create ivar first
			uint16 ivar_index = (p->index >= 0) ? p->index : gravity_class_add_ivar(context_class, NULL);

			// add default getter and setter ONLY if property is public
			if(node->access == TOK_KEY_PUBLIC) {
				// since getter and setter are methods and not simple functions, do not transfer to VM
				gravity_function_t * f = gravity_function_new_special(NULL, NULL, ivar_index, NULL, NULL); // getter and setter NULL means default
				gravity_class_bind(context_class, p->identifier, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(f)));
			}
			DEBUG_CODEGEN("Class: %s (static: %d) property: %s index: %d", context_class->identifier, is_static, p->identifier, ivar_index);
			// it is a property (default initialized to null)
			// code must be generated ONLY if an init expression is specified

			//
			//  example:
			//     class foo {
			//         var a = 10;
			//     }
			//
			//  get $init function
			//  depending if variable has been created static
			//  and push it as current declaration then:
			//
			//     LOADI     0 10   ; move 10 into register 0
			//     STOREF    0 0    ; move register 0 into property 0

			if(p->expr) {
				// was gravity_class_lookup but that means than $init or init will be recursively
				// searched also in super classes
				gravity_function_t * init_function = class_lookup_nosuper(context_class, CLASS_INTERNAL_INIT_NAME);
				if(init_function == NULL) {
					// no $init method found so create a new one
					init_function = gravity_function_new(NULL, CLASS_INTERNAL_INIT_NAME, 1, 0, 0, ircode_create(1));
					gravity_class_bind(context_class, CLASS_INTERNAL_INIT_NAME, GravityValue::from_object(reinterpret_cast<gravity_class_t *>(init_function)));
				}
				CONTEXT_PUSH(init_function);
				ircode_t * code = (ircode_t *)init_function->U.Nf.bytecode;
				gvisit(self, p->expr);
				uint32 reg = ircode_register_pop(code);
				if(reg == REGISTER_ERROR) 
					report_error(self, node, "Invalid var expression.");
				ircode_add(code, STORE, reg, 0, p->index + MAX_REGISTERS, LINE_NUMBER(node));
				CONTEXT_POP();
			}
			continue;
		}
		assert(0); // should never reach this point
	}
}

static void visit_enum_decl(gvisitor_t * self, gnode_enum_decl_t * node) 
{
	DEBUG_CODEGEN("visit_enum_decl %s", node->identifier);
	// enum is a map at runtime
	// enum foo {a=1,b=2,...}
	// is translated to
	// var foo = [a:1,b:2,...]
}

static void visit_class_decl(gvisitor_t * self, gnode_class_decl_t * node) 
{
	DEBUG_CODEGEN("visit_class_decl %s", node->identifier);
	// extern means it will be provided at runtime by the delegate
	if(node->storage == TOK_KEY_EXTERN) 
		return;
	// create a new pair of classes (class itself and its meta)
	gravity_class_t * c = gravity_class_new_pair(GET_VM(), node->identifier, NULL, node->nivar, node->nsvar);
	// mark the class as a struct
	SETFLAG(c->Flags, GravityObjectBase::fIsStruct, node->is_struct); //c->is_struct = node->is_struct;
	// check if class has a declared superclass
	if(node->superclass) {
		// node->superclass should be a gnode_class_decl_t at this point
		if(gnode_t::IsA(node->superclass, NODE_CLASS_DECL)) {
			node->super_extern = (((gnode_class_decl_t *)node->superclass)->storage == TOK_KEY_EXTERN);
		}
		else {
			node->superclass = gnode2class(node->superclass, &node->super_extern);
			if(!node->superclass) 
				report_error(self, node, "Unable to set superclass to non class object.");
		}
		gnode_class_decl_t * super = (gnode_class_decl_t *)node->superclass;
		if(node->super_extern) {
			gravity_class_setsuper_extern(c, super->identifier);
		}
		else {
			if(super->data) {
				// means that superclass has already been processed and its runtime representation is
				// available
				gravity_class_setsuper(c, (gravity_class_t *)super->data);
			}
			else {
				// superclass has not yet processed so we need recheck the node at the end of the visit
				// add node to superfix for later processing
				codegen_t * data = (codegen_t *)self->data;
				node->data = (void*)c;
				data->superfix.insert(node);
			}
		}
	}
	CONTEXT_PUSH(c);
	// process inner declarations
	gnode_array_each(node->decls, {gvisit(self, val);});
	// adjust declaration stack
	CONTEXT_POP();
	// fix constructor chain
	process_constructor(self, c, node);
	// store class declaration in current context
	store_declaration(self, (gravity_class_t *)c, (node->storage == TOK_KEY_STATIC), NULL);
	// save runtime representation
	// since this class could be a superclass of another class
	// then save opaque gravity_class_t pointer to node->data
	node->data = (void*)c;
}

static void visit_module_decl(gvisitor_t * self, gnode_module_decl_t * node) 
{
	DEBUG_CODEGEN("visit_module_decl %s", node->identifier);
	// a module should be like a class with static entries
	// instantiated with import

//    gravity_module_t *module = gravity_module_new(GET_VM(), node->identifier);
//    CONTEXT_PUSH(module);
//
//    // process inner declarations
//    gnode_array_each(node->decls, {gvisit(self, val);});
//
//    // adjust declaration stack
//    CONTEXT_POP();
}

// MARK: - Expressions -

//static void process_special_enum (gvisitor_t *self, gnode_enum_decl_t *node, gnode_identifier_expr_t *identifier) {
//    symboltable_t *symtable = node->symtable;
//    gnode_t *v = symboltable_lookup(symtable, identifier->value);
//    assert(v);
//    assert(NODE_ISA(v, NODE_LITERAL_EXPR));
//    gvisit(self, v);
//}

static void visit_binary_expr(gvisitor_t * self, gnode_binary_expr_t * node) 
{
	DEBUG_CODEGEN("visit_binary_expr %s", token_name(node->op));
	DECLARE_CODE();
	// assignment is right associative
	if(node->op == TOK_OP_ASSIGN) {
		CODEGEN_COUNT_REGISTERS(n1);
		gvisit(self, node->right);
		gvisit(self, node->left); // left expression can be: IDENTIFIER, FILE, POSTIFIX (not a call)
		CODEGEN_COUNT_REGISTERS(n2);
		CODEGEN_ASSERT_REGISTERS(n1, n2, 0);
		// Checkpoint added to support STRUCT
		ircode_add_check(code);
		return;
	}
	CODEGEN_COUNT_REGISTERS(n1);
	// visiting binary operation from left to right
	gvisit(self, node->left);
	gvisit(self, node->right);
	uint32 r3 = ircode_register_pop(code);
	if(r3 == REGISTER_ERROR) 
		report_error(self, node->right, "Invalid right expression.");
	uint32 r2 = ircode_register_pop(code);
	if(r2 == REGISTER_ERROR) 
		report_error(self, node->left, "Invalid left expression.");
	uint32 r1 = ircode_register_push_temp(code);
	// a special instruction needs to be generated for a binary expression of type RANGE
	if((node->op == TOK_OP_RANGE_INCLUDED) || (node->op == TOK_OP_RANGE_EXCLUDED)) {
		ircode_add_tag(code, RANGENEW, r1, r2, r3, (node->op == TOK_OP_RANGE_INCLUDED) ? RANGE_INCLUDE_TAG : RANGE_EXCLUDE_TAG, LINE_NUMBER(node));
		return;
	}

	// generate code for binary OP
	opcode_t op = token2opcode(node->op);
	ircode_add(code, op, r1, r2, r3, LINE_NUMBER(node));

	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, 1);
}

static void visit_unary_expr(gvisitor_t * self, gnode_unary_expr_t * node) {
	DEBUG_CODEGEN("visit_unary_expr %s", token_name(node->op));
	DECLARE_CODE();

	CODEGEN_COUNT_REGISTERS(n1);

	// unary expression can be:
	//    +    Unary PLUS
	//    -    Unary MINUS
	//    !    Logical NOT
	//    ~    Bitwise NOT

	gvisit(self, node->expr);
	if(node->op == TOK_OP_ADD) {
		// +1 is just 1 and more generally +expr is just expr so ignore + and proceed
		return;
	}
	uint32 r2 = ircode_register_pop(code);
	if(r2 == REGISTER_ERROR) 
		report_error(self, node, "Invalid unary expression.");
	uint32 r1 = ircode_register_push_temp(code);
	opcode_t op = (node->op == TOK_OP_SUB) ? NEG : token2opcode(node->op);
	ircode_add(code, op, r1, r2, 0, LINE_NUMBER(node));
	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, 1);
}

static void visit_postfix_expr(gvisitor_t * self, gnode_postfix_expr_t * node) 
{
	DEBUG_CODEGEN("visit_postfix_expr");
	// node->list usually cannot be NULL, it is NULL only as a result of a static enum transformation (see
	// semacheck2.c)
	if(node->list == NULL) {
		gvisit(self, node->id);
		return;
	}
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	ircode_push_context(code);

	// disable MOVE optimization
	ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 0, LINE_NUMBER(node));

	// generate code for the common id node
	gvisit(self, node->id);

	bool is_super = IS_SUPER(node->id);

	// register that contains callable object
	uint32 target_register = ircode_register_pop_context_protect(code, true);
	if(target_register == REGISTER_ERROR) {
		report_error(self, node->id, "Invalid postfix expression.");
		return;
	}
	// register where to store final result
	uint32 dest_register = target_register;
	// mandatory self register (initialized to 0 in case of implicit self or explicit super)
	GravityArray <uint32> self_list; 
	// @ctr marray_init(self_list);
	uint32 first_self_register = compute_self_register(self, code, node->id, target_register, node->list);
	if(first_self_register == UINT32_MAX) return;
	self_list.insert(first_self_register);

	// process each subnode and set is_assignment flag
	bool is_assignment = node->IsAssignment;
	const uint count = gnode_array_size(node->list);
	for(uint i = 0; i < count; ++i) {
		// a subnode can be a CALL_EXPR     => id.()
		// a NODE_ACCESS_EXPR               => id.id2
		// a NODE_SUBSCRIPT_EXPR            => id[]
		// or ANY combination of the them!  => id.id2.id3()[24]().id5()
		gnode_postfix_subexpr_t * subnode = (gnode_postfix_subexpr_t*)gnode_array_get(node->list, i);
		// identify postfix type: NODE_CALL_EXPR, NODE_ACCESS_EXPR, NODE_SUBSCRIPT_EXPR
		gnode_n tag = subnode->Tag;
		// check assignment flag
		bool is_real_assigment = (is_assignment && IS_LAST_LOOP(i, count));
		if(tag == NODE_CALL_EXPR) {
			// a CALL instruction needs to properly prepare stack before execution
			// format is CALL A B C

			// where A is the destination register
			// B is the callable object
			// and C is the number of arguments passed to the CALL
			// arguments must be on the stack (so gravity VM can use a register window in order to speed up
			// instruction)
			// and are expected to be starting from B+1

			// check dest register
			bool dest_is_temp = ircode_register_istemp(code, dest_register);
			if(!dest_is_temp) {
				dest_register = ircode_register_push_temp(code);
				dest_is_temp = true;
			}

			// add target register (must be temp)
			uint32 temp_target_register = ircode_register_push_temp(code);
			ircode_add(code, MOVE, temp_target_register, target_register, 0, LINE_NUMBER(node));
			uint32 treg = ircode_register_pop_context_protect(code, true);
			if(treg == REGISTER_ERROR) {
				report_error(self, subnode, "Unexpected register error.");
				return;
			}

			// always add SELF parameter (must be temp+1)
			uint32 self_register = self_list.pop();
			uint32 temp_self_register = ircode_register_push_temp(code);
			ircode_add(code, MOVE, temp_self_register, self_register, 0, LINE_NUMBER(node));
			treg = ircode_register_pop_context_protect(code, true);
			if(treg == REGISTER_ERROR) {
				report_error(self, subnode, "Unexpected register error.");
				return;
			}

			// process each parameter (each must be temp+2 ... temp+n)
			GravityArray <uint32> args;
			uint n = gnode_array_size(subnode->args);
			for(uint j = 0; j < n; ++j) {
				// process each argument
				gnode_t * arg = gnode_array_get(subnode->args, j);
				ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 1, LINE_NUMBER(node));
				gvisit(self, arg);
				ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 0, LINE_NUMBER(node));
				uint32 nreg = ircode_register_pop_context_protect(code, true);
				if(nreg == REGISTER_ERROR) {
					report_error(self, arg, "Invalid argument expression at index %d.", j+1);
					return;
				}

				// make sure args are in consecutive register locations (from temp_target_register + 1
				// to temp_target_register + n)
				if(nreg != temp_target_register + j + 2) {
					uint32 temp = ircode_register_push_temp(code);
					if(temp == 0) return; // temp value == 0 means codegen error (error will be
					                      // automatically reported later in visit_function_decl
					ircode_add(code, MOVE, temp, nreg, 0, LINE_NUMBER(node));
					ircode_register_clear(code, nreg);
					nreg = ircode_register_pop_context_protect(code, true);
					if(nreg == REGISTER_ERROR) {
						report_error(self, arg, "Invalid argument expression");
						return;
					}
				}
				if(nreg != temp_target_register + j + 2) {
					report_error(self, arg, "Invalid register computation in call expression.");
					return;
				}

				// a checkpoint should be added after each nreg computation in order to support STRUCT
				// I'll skip this overhead in this version because it should be a type based decision
				// ircode_add_check(code, nreg);
				args.insert(nreg);
			}

			// generate instruction CALL with count parameters (taking in account self)
			ircode_add(code, CALL, dest_register, temp_target_register, (uint32)n+1, LINE_NUMBER(node));

			// cleanup temp registers
			ircode_register_clear(code, temp_target_register);
			ircode_register_clear(code, temp_self_register);
			n = gnode_array_size(subnode->args);
			for(uint j = 0; j < n; ++j) {
				uint32 reg = args.at(j);
				ircode_register_clear(code, reg);
			}

			// update self list
			self_list.insert(dest_register);

			// a call returns a value
			if(IS_LAST_LOOP(i, count)) {
				if(ircode_register_count(code)) {
					// code added in order to protect the extra register pushed in case
					// of code like: f(20)(30)
					uint32 last_register = ircode_register_last(code);
					if(last_register == REGISTER_ERROR) {
						report_error(self, subnode, "Invalid call expression.");
						return;
					}
					if(dest_is_temp && last_register == dest_register) 
						dest_is_temp = false;
				}
				if(dest_is_temp) 
					ircode_register_push(code, dest_register);
				if(!ircode_register_protect_outside_context(code, dest_register)) {
					report_error(self, subnode, "Invalid register access.");
					return;
				}
			}
			args.Z(); // free temp args array
		}
		else if(tag == NODE_ACCESS_EXPR) {
			// process identifier node (semantic check assures that each node is an identifier)
			gnode_identifier_expr_t * expr = (gnode_identifier_expr_t*)subnode->expr;
			uint32 index = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, expr->value));
			uint32 index_register = ircode_register_push_temp(code);
			ircode_add(code, LOADK, index_register, index, 0, LINE_NUMBER(expr));
			uint32 temp = ircode_register_pop(code);
			if(temp == REGISTER_ERROR) {
				report_error(self, expr, "Invalid access expression.");
				return;
			}

			// generate LOAD/STORE instruction
			dest_register = (is_real_assigment) ? ircode_register_pop(code) : ircode_register_push_temp(code);
			if(dest_register == REGISTER_ERROR) {
				report_error(self, expr, "Invalid access expression.");
				return;
			}
			if(is_super) {
				gravity_class_t * p_class = context_get_class(self);
				if(!p_class) {
					report_error(self, node, "Unable to use super keyword in a non class context.");
					return;
				}
				// check if class has a superclass not yet processed
				const char * identifier = lookup_superclass_identifier(self, p_class);
				if(!identifier) {
					// it means that class superclass has already been processed
					p_class = p_class->superclass;
					identifier = (p_class) ? p_class->identifier : GRAVITY_CLASS_OBJECT_NAME;
				}
				uint32 cpool_index = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, identifier));
				ircode_add_constant(code, cpool_index, LINE_NUMBER(node));
				uint32 temp_reg = ircode_register_pop(code);
				ircode_add(code, LOADS, dest_register, temp_reg, index_register, LINE_NUMBER(node));
			}
			else {
				ircode_add(code, (is_real_assigment) ? STORE : LOAD, dest_register, target_register, index_register, LINE_NUMBER(node));
			}

			if(!is_real_assigment) {
				if(i+1<count) {
					uint32 rtemp = ircode_register_pop_context_protect(code, true);
					if(rtemp == REGISTER_ERROR) {
						report_error(self, expr, "Unexpected register error.");
						return;
					}
				}
				else {
					// last loop
					if(ircode_register_istemp(code, dest_register))
						ircode_register_protect_outside_context(code, dest_register);
				}
			}

			// update self list (if latest instruction)
			// this was added in order to properly emit instructions for nested_class.gravity unit test
			if(!IS_LAST_LOOP(i, count)) {
				gnode_postfix_subexpr_t * nextnode = (gnode_postfix_subexpr_t*)gnode_array_get(node->list, i+1);
				if(nextnode->Tag != NODE_CALL_EXPR) 
					self_list.insert(dest_register);
			}
		}
		else if(tag == NODE_SUBSCRIPT_EXPR) {
			// process index
			ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 1, LINE_NUMBER(node));
			gvisit(self, subnode->expr);
			ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 0, LINE_NUMBER(node));
			uint32 index = ircode_register_pop(code);
			if(index == REGISTER_ERROR) {
				report_error(self, subnode->expr, "Invalid subscript expression.");
				return;
			}

			// generate LOADAT/STOREAT instruction
			dest_register = (is_real_assigment) ? ircode_register_pop(code) : ircode_register_push_temp(code);
			if(dest_register == REGISTER_ERROR) {
				report_error(self, subnode->expr, "Invalid subscript expression.");
				return;
			}
			ircode_add(code, (is_real_assigment) ? STOREAT : LOADAT, dest_register, target_register, index, LINE_NUMBER(node));
			if((!is_real_assigment) && (i+1<count)) {
				uint32 rtemp = ircode_register_pop_context_protect(code, true);
				if(rtemp == REGISTER_ERROR) {
					report_error(self, subnode->expr, "Unexpected register error.");
					return;
				}
				self_list.insert(rtemp);
			}
		}
		is_super = false; // reset is_super flag
		target_register = dest_register; // update target
	}
	self_list.Z();
	ircode_pop_context(code);
	// temp fix for not optimal register allocation algorithm generated code
	uint32 temp_register = ircode_register_first_temp_available(code);
	if(temp_register < dest_register) {
		// free dest register
		ircode_register_pop(code);
		// allocate a new register (that I am now sure does not have holes)
		temp_register = ircode_register_push_temp(code);
		ircode_add(code, MOVE, temp_register, dest_register, 0, LINE_NUMBER(node));
		ircode_register_clear(code, dest_register);
	}

	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, (is_assignment) ? -1 : 1);

	// re-enable MOVE optimization
	ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 1, LINE_NUMBER(node));
}

static void visit_file_expr(gvisitor_t * self, gnode_file_expr_t * node) 
{
	DEBUG_CODEGEN("visit_file_expr");
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	// check if the node is a left expression of an assignment
	bool is_assignment = node->IsAssignment;
	const uint count = gnode_array_size(node->identifiers);
	for(uint i = 0; i < count; ++i) {
		const char * identifier = gnode_array_get(node->identifiers, i);
		uint16 kindex = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, identifier));
		if((is_assignment) && (IS_LAST_LOOP(i, count))) {
			uint32 reg = ircode_register_pop(code);
			if(reg == REGISTER_ERROR) 
				report_error(self, node, "Invalid file expression.");
			ircode_add(code, STOREG, reg, kindex, 0, LINE_NUMBER(node));
		}
		else {
			ircode_add(code, LOADG, ircode_register_push_temp(code), kindex, 0, LINE_NUMBER(node));
		}
	}
	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, (is_assignment) ? -1 : 1);
}

static void visit_literal_expr(gvisitor_t * self, gnode_literal_expr_t * node) 
{
	/*
	   NOTE:
	   doubles and int64 should be added to the constant pool but I avoid
	   adding them here so the optimizer has a way to perform better constant folding:
	   http://en.wikipedia.org/wiki/Constant_folding
	   http://www.compileroptimizations.com/category/constant_folding.htm

	 */
	DEBUG_CODEGEN("visit_literal_expr");
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	switch(node->type) {
		case LITERAL_STRING: 
			{
				// LOADK temp, s
				uint16 index = gravity_function_cpool_add(GET_VM(), context_function, VALUE_FROM_STRING(NULL, node->value.str, node->len));
				ircode_add_constant(code, index, LINE_NUMBER(node));
				DEBUG_CODEGEN("visit_literal_expr (string) %s", node->value.str);
			} 
			break;
		case LITERAL_FLOAT:
		    // LOADI temp, d
		    ircode_add_double(code, node->value.d, LINE_NUMBER(node));
		    DEBUG_CODEGEN("visit_literal_expr (float) %.5f", node->value.d);
		    break;
		case LITERAL_INT:
		    // LOADI temp, n
		    ircode_add_int(code, node->value.n64, LINE_NUMBER(node));
		    DEBUG_CODEGEN("visit_literal_expr (int) %lld", node->value.n64);
		    break;
		case LITERAL_BOOL: 
			{
				uint32 value = (node->value.n64 == 0) ? CPOOL_VALUE_FALSE : CPOOL_VALUE_TRUE;
				ircode_add_constant(code, value, LINE_NUMBER(node));
				DEBUG_CODEGEN("visit_literal_expr (bool) %lld", node->value.n64);
			} 
			break;
		case LITERAL_STRING_INTERPOLATED: {
		    // codegen for string interpolation is like a list.join()
		    gnode_list_expr_t * list = (gnode_list_expr_t*)gnode_list_expr_create(node->Token, node->value.r, NULL, false, static_cast<gnode_t *>(node->P_Decl));
		    gvisit(self, list);
		    // list
		    uint32 listreg = ircode_register_last(code);
		    if(listreg == REGISTER_ERROR) 
				report_error(self, node, "Invalid string interpolated expression.");
		    // LOADK
		    uint16 index = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, "join"));
		    ircode_add_constant(code, index, LINE_NUMBER(node));
		    uint32 temp1 = ircode_register_last(code);
		    DEBUG_ASSERT(temp1 != REGISTER_ERROR, "Unexpected register error.");
		    ircode_add(code, LOAD, temp1, listreg, temp1, LINE_NUMBER(node)); // LOAD
		    // temp1+1 register used for parameter passing
		    uint32 temp2 = ircode_register_push_temp(code);
		    ircode_add(code, MOVE, temp2, listreg, 0, LINE_NUMBER(node)); // MOVE
		    ircode_add(code, CALL, listreg, temp1, 1, LINE_NUMBER(node)); // CALL
		    // cleanup
		    mem_free(list);
		    uint32 temp = ircode_register_pop(code); // temp2
		    DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
		    temp = ircode_register_pop(code);    // temp1
		    DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
		    /*

		       00012    LOADK 6 4
		       00013    LOAD 6 4 6
		       00014    MOVE 7 6
		       00015    MOVE 8 4
		       00016    CALL 6 7 1
		     */
		    break;
	    }

		default: assert(0);
	}

	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, 1);
}

static void visit_identifier_expr(gvisitor_t * self, gnode_identifier_expr_t * node) 
{
	DEBUG_CODEGEN("visit_identifier_expr %s", node->value);
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	// check if the node is a left expression of an assignment
	bool is_assignment = node->IsAssignment;
	const char * identifier = node->value;   // identifier as c string
	gnode_location_type type = node->location.type;       // location type
	uint16 index = node->location.index;           // symbol index
	uint16 nup = node->location.nup;               // upvalue index or outer index
	switch(type) {
		// local variable
		case LOCATION_LOCAL: {
		    if(is_assignment) {
			    uint32 reg = ircode_register_pop(code);
			    if(reg == REGISTER_ERROR) 
					report_error(self, node, "Invalid identifier expression.");
			    ircode_add(code, MOVE, index, reg, 0, LINE_NUMBER(node));
		    }
		    else {
			    ircode_register_push(code, index);
		    }
	    } break;

		// module (global) variable
		case LOCATION_GLOBAL: {
		    uint16 kindex = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, identifier));
		    if(is_assignment) {
			    uint32 reg = ircode_register_pop(code);
			    if(reg == REGISTER_ERROR) 
					report_error(self, node, "Invalid identifier expression.");
			    ircode_add(code, STOREG, reg, kindex, 0, LINE_NUMBER(node));
		    }
		    else {
			    ircode_add(code, LOADG, ircode_register_push_temp(code), kindex, 0, LINE_NUMBER(node));
		    }
	    } break;

		// upvalue access
		case LOCATION_UPVALUE: {
		    gupvalue_t * upvalue = node->upvalue;
		    if(is_assignment) {
			    uint32 reg = ircode_register_pop(code);
			    if(reg == REGISTER_ERROR) 
					report_error(self, node, "Invalid identifier expression.");
			    ircode_add(code, STOREU, reg, upvalue->selfindex, 0, LINE_NUMBER(node));
		    }
		    else {
			    ircode_add(code, LOADU, ircode_register_push_temp(code), upvalue->selfindex, 0, LINE_NUMBER(node));
		    }
	    } break;

		// class ivar case (outer case just need a loop lookup before)
		case LOCATION_CLASS_IVAR_OUTER:
		case LOCATION_CLASS_IVAR_SAME: {
		    // needs to differentiate ivar (indexed by an integer) from other cases (indexed by a string)
		    bool is_ivar = (index != UINT16_MAX);
		    uint32 dest = 0;
		    uint32 target = 0;

		    if(type == LOCATION_CLASS_IVAR_OUTER) {
			    dest = ircode_register_push_temp(code);
			    for(uint16 i = 0; i<nup; ++i) {
				    ircode_add(code, LOAD, dest, target, 0 + MAX_REGISTERS, LINE_NUMBER(node));
				    target = dest;
			    }
			    if(is_assignment) {
				    uint32 temp = ircode_register_pop(code);
				    DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
			    }
		    }

		    uint32 index_register;
		    if(is_ivar) {
			    // ivar case, use an index to load/retrieve it
			    index_register = index + MAX_REGISTERS;
		    }
		    else {
			    // not an ivar so it could be another class declaration like a func, a class or an enum
			    // use lookup in order to retrieve it (assignment is handled here so you can change a
			    // first class citizen at runtime too)
			    uint16 kindex = gravity_function_cpool_add(GET_VM(), context_function, gravity_zstring_to_value(NULL, identifier));
			    index_register = ircode_register_push_temp(code);
			    ircode_add(code, LOADK, index_register, kindex, 0, LINE_NUMBER(node));
			    uint32 temp = ircode_register_pop(code);
			    DEBUG_ASSERT(temp != REGISTER_ERROR, "Unexpected register error.");
		    }

		    if(is_assignment) {
			    // should be prohibited by semantic to store something into a non ivar slot?
			    dest = ircode_register_pop(code); // consume temp register
			    if(dest == REGISTER_ERROR) 
					report_error(self, node, "Invalid identifier expression.");
			    ircode_add(code, STORE, dest, target, index_register, LINE_NUMBER(node));
		    }
		    else {
			    dest = (type == LOCATION_CLASS_IVAR_OUTER) ? target : ircode_register_push_temp(code);
			    ircode_add(code, LOAD, dest, target, index_register, LINE_NUMBER(node));
		    }
	    } break;
	}

	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, (is_assignment) ? -1 : 1);
}

static void visit_keyword_expr(gvisitor_t * self, gnode_keyword_expr_t * node) 
{
	DEBUG_CODEGEN("visit_keyword_expr %s", token_name(node->base.token.type));
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	gtoken_t type = node->GetTokenType();
	switch(type) {
		case TOK_KEY_CURRFUNC: ircode_add_constant(code, CPOOL_VALUE_FUNC, LINE_NUMBER(node)); break;
		case TOK_KEY_NULL: ircode_add_constant(code, CPOOL_VALUE_NULL, LINE_NUMBER(node)); break;
		case TOK_KEY_SUPER: ircode_register_push(code, 0); break;
		case TOK_KEY_CURRARGS:
		    // compiler can know in advance if arguments special array is used
		    context_function->U.Nf.useargs = true;
		    ircode_add_constant(code, CPOOL_VALUE_ARGUMENTS, LINE_NUMBER(node));
		    break;
		case TOK_KEY_UNDEFINED: ircode_add_constant(code, CPOOL_VALUE_UNDEFINED, LINE_NUMBER(node)); break;
		case TOK_KEY_TRUE: ircode_add_constant(code, CPOOL_VALUE_TRUE, LINE_NUMBER(node)); break;
		case TOK_KEY_FALSE: ircode_add_constant(code, CPOOL_VALUE_FALSE, LINE_NUMBER(node)); break;
		default: report_error(self, node, "Invalid keyword expression."); break;
	}
	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, 1);
}

static void visit_list_expr(gvisitor_t * self, gnode_list_expr_t * node) 
{
	DEBUG_CODEGEN("visit_list_expr");
	DECLARE_CODE();
	CODEGEN_COUNT_REGISTERS(n1);
	bool ismap = node->ismap;
	uint32 n = (uint32)gnode_array_size(node->list1);
	// a map requires twice registers than a list
	uint32 max_fields = (ismap) ? MAX_FIELDSxFLUSH : MAX_FIELDSxFLUSH*2;

	// destination register of a new instruction is ALWAYS a temp register
	// then the optimizer could decide to optimize and merge the step
	uint32 dest = ircode_register_push_temp(code);
	ircode_add(code, (ismap) ? MAPNEW : LISTNEW, dest, n, 0, LINE_NUMBER(node));
	if(n == 0) return;

	// this is just like Lua "fields per flush"
	// basically nodes are processed in a finite chunk
	// and then added to the list/map
	uint32 nloops = n / max_fields;
	if(n % max_fields != 0) 
		++nloops;
	uint32 nprocessed = 0;
	ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 0, LINE_NUMBER(node));
	while(nprocessed < n) {
		uint k = (n - nprocessed > max_fields) ? max_fields : (n - nprocessed);
		uint idxstart = nprocessed;
		uint idxend = nprocessed + k;
		nprocessed += (uint32)k;

		// check if this chunk can be optimized
		// if (check_literals_list(self, node, ismap, idxstart, idxend, dest)) continue;

		// save register context
		ircode_push_context(code);

		// process each node
		for(uint i = 1, j = idxstart; j < idxend; ++j) {
			gnode_t * e = gnode_array_get(node->list1, j);
			gvisit(self, e);
			uint32 nreg = ircode_register_pop_context_protect(code, true);
			if((nreg == REGISTER_ERROR) || ((nreg <= dest) && (ircode_register_istemp(code, nreg)))) {
				report_error(self, e, "Invalid list expression.");
				continue;
			}
			if(nreg != dest + i) {
				uint32 temp_register = ircode_register_push_temp(code);
				ircode_add(code, MOVE, temp_register, nreg, 0, LINE_NUMBER(node));
				ircode_register_clear(code, nreg);
				uint32 temp = ircode_register_pop_context_protect(code, true);
				if(temp == REGISTER_ERROR) {
					report_error(self, e, "Unexpected register error."); continue;
				}
				if(temp_register != dest + i) {
					report_error(self, e, "Unexpected register computation."); continue;
				}
			}

			if(ismap) {
				e = gnode_array_get(node->list2, j);
				ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 1, LINE_NUMBER(node));
				gvisit(self, e);
				ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 0, LINE_NUMBER(node));
				nreg = ircode_register_pop_context_protect(code, true);
				if((nreg == REGISTER_ERROR) || ((nreg <= dest) && (ircode_register_istemp(code, nreg)))) {
					report_error(self, e, "Invalid map expression.");
					continue;
				}
				if(nreg != dest + i + 1) {
					uint32 temp_register = ircode_register_push_temp(code);
					ircode_add(code, MOVE, temp_register, nreg, 0, LINE_NUMBER(node));
					ircode_register_clear(code, nreg);
					uint32 temp = ircode_register_pop_context_protect(code, true);
					if(temp == REGISTER_ERROR) {
						report_error(self, e, "Unexpected register error."); continue;
					}
					if(temp_register != dest + i + 1) {
						report_error(self, e, "Unexpected register computation."); continue;
					}
				}
			}

			i += (ismap) ? 2 : 1;
		}

		// emit proper SETLIST instruction
		// since in a map registers are always used in pairs (key, value) it is
		// extremely easier to just set reg1 to be always 0 and use r in a loop
		ircode_add(code, SETLIST, dest, (uint32)(idxend-idxstart), 0, LINE_NUMBER(node));

		// restore register context
		ircode_pop_context(code);
	}

	ircode_pragma(code, PRAGMA_MOVE_OPTIMIZATION, 1, LINE_NUMBER(node));
	CODEGEN_COUNT_REGISTERS(n2);
	CODEGEN_ASSERT_REGISTERS(n1, n2, 1);
}

// MARK: -

gravity_function_t * gravity_codegen(gnode_t * node, gravity_delegate_t * delegate, gravity_vm * vm, bool add_debug) 
{
	codegen_t data(vm);
	// @ctr marray_init(data.context);
	// @ctr marray_init(data.superfix);
	ircode_t * code = ircode_create(0);
	gravity_function_t * f = gravity_function_new(vm, INITMODULE_NAME, 0, 0, 0, code);
	data.context.insert((gravity_class_t *)f);
	gvisitor_t visitor(&data, delegate);
	visitor.bflag = add_debug;
	visitor.visit_list_stmt = visit_list_stmt;
	visitor.visit_compound_stmt = visit_compound_stmt;
	visitor.visit_label_stmt = visit_label_stmt;
	visitor.visit_flow_stmt = visit_flow_stmt;
	visitor.visit_loop_stmt = visit_loop_stmt;
	visitor.visit_jump_stmt = visit_jump_stmt;
	visitor.visit_empty_stmt = visit_empty_stmt;
	// DECLARATIONS: 5
	visitor.visit_function_decl = visit_function_decl;
	visitor.visit_variable_decl = visit_variable_decl;
	visitor.visit_enum_decl = visit_enum_decl;
	visitor.visit_class_decl = visit_class_decl;
	visitor.visit_module_decl = visit_module_decl;

	// EXPRESSIONS: 8
	visitor.visit_binary_expr = visit_binary_expr;
	visitor.visit_unary_expr = visit_unary_expr;
	visitor.visit_file_expr = visit_file_expr;
	visitor.visit_literal_expr = visit_literal_expr;
	visitor.visit_identifier_expr = visit_identifier_expr;
	visitor.visit_keyword_expr = visit_keyword_expr;
	visitor.visit_list_expr = visit_list_expr;
	visitor.visit_postfix_expr = visit_postfix_expr;
	/*gvisitor_t visitor = {
		.nerr = 0,              // used for internal codegen errors
		.data = &data,          // used to store a pointer to codegen struct
		.bflag = add_debug,     // flag used to decide if bytecode must include debug info
		.delegate = (void*)delegate, // compiler delegate to report errors

		// COMMON
		.visit_pre = NULL,
		.visit_post = NULL,

		// STATEMENTS: 7
		.visit_list_stmt = visit_list_stmt,
		.visit_compound_stmt = visit_compound_stmt,
		.visit_label_stmt = visit_label_stmt,
		.visit_flow_stmt = visit_flow_stmt,
		.visit_loop_stmt = visit_loop_stmt,
		.visit_jump_stmt = visit_jump_stmt,
		.visit_empty_stmt = visit_empty_stmt,

		// DECLARATIONS: 5
		.visit_function_decl = visit_function_decl,
		.visit_variable_decl = visit_variable_decl,
		.visit_enum_decl = visit_enum_decl,
		.visit_class_decl = visit_class_decl,
		.visit_module_decl = visit_module_decl,

		// EXPRESSIONS: 8
		.visit_binary_expr = visit_binary_expr,
		.visit_unary_expr = visit_unary_expr,
		.visit_file_expr = visit_file_expr,
		.visit_literal_expr = visit_literal_expr,
		.visit_identifier_expr = visit_identifier_expr,
		.visit_keyword_expr = visit_keyword_expr,
		.visit_list_expr = visit_list_expr,
		.visit_postfix_expr = visit_postfix_expr,
	};*/

	DEBUG_CODEGEN("=== BEGIN CODEGEN ===");
	gvisit(&visitor, node);
	DEBUG_CODEGEN("\n");
	// check for special superfix
	if(data.superfix.getCount()) {
		fix_superclasses(&visitor);
	}
	// pop globals instance init special function
	data.context.pop();
	assert(data.context.getCount() == 0);
	data.context.Z();
	// in case of codegen errors explicity free code and return NULL
	if(visitor.nerr != 0) {
		ircode_free(code); 
		f->U.Nf.bytecode = NULL;
	}
	return (visitor.nerr == 0) ? f : NULL;
}
