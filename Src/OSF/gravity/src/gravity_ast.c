//  gravity_ast.c
//  gravity
//
//  Created by Marco Bambini on 02/09/14.
//  Copyright (c) 2014 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

//#define SETBASE(node, tagv, _tok) node->Tag = tagv; node->Token = _tok
//#define SETDECL(node, _decl)      node->P_Decl = _decl
#define CHECK_REFCOUNT(_node)     if(_node->RefCount > 0) {--_node->RefCount; return;}

// MARK: -

GravityArray <void *> * void_array_create() 
{
	GravityArray <void *> * r = new GravityArray <void *>();
	// @ctr marray_init(*r);
	return r;
}

GravityArray <const char *> * cstring_array_create()
{
	GravityArray <const char *> * r = new GravityArray <const char *>();
	// @ctr gnode_array_init(r);
	return r;
}

GravityArray <gnode_t *> * gnode_array_create() 
{
	GravityArray <gnode_t *> * r = new GravityArray <gnode_t *>();
	// @ctr gnode_array_init(r);
	return r;
}

void gnode_array_sethead(GravityArray <gnode_t *> * list, gnode_t * node) 
{
	if(list && node) {
		// get old size
		size_t list_size = gnode_array_size(list);
		// push node at the end to trigger memory allocation (if needed)
		gnode_array_push(list, node);
		// shift elements in array
		for(size_t i = list_size; i > 0; --i) {
			list->p[i] = list->p[i-1];
		}
		// set new array head
		list->p[0] = node;
	}
}

GravityArray <gnode_t *> * gnode_array_remove_byindex(GravityArray <gnode_t *> * old_list, size_t index) 
{
	// get old size
	const uint list_size = gnode_array_size(old_list);
	if(index >= list_size) 
		return NULL;
	GravityArray <gnode_t *> * new_list = gnode_array_create();
	for(uint i = 0; i < list_size; ++i) {
		if(i == index) 
			continue;
		gnode_t * node = gnode_array_get(old_list, i);
		gnode_array_push(new_list, node);
	}
	gnode_array_free(old_list);
	return new_list;
}

gupvalue_t * gnode_function_add_upvalue(gnode_function_decl_t * f, gnode_var_t * symbol, uint16 n) 
{
	// create uplist if necessary
	if(!f->uplist) {
		f->uplist = new GravityArray <gupvalue_t *>();
		// @ctr gnode_array_init(f->uplist);
	}
	// lookup symbol in uplist (if any)
	gtype_array_each(f->uplist, {
		// symbol already found in uplist so return its index
		gnode_var_t * node = (gnode_var_t *)val->node;
		if(sstreq(node->identifier, symbol->identifier)) 
			return val;
	}, gupvalue_t *);

	// symbol not found in uplist so add it
	gupvalue_t * upvalue = (gupvalue_t *)mem_alloc(NULL, sizeof(gupvalue_t));
	upvalue->node = symbol;
	upvalue->index = (n == 1) ? symbol->index : (uint32)gnode_array_size(f->uplist);
	upvalue->selfindex = (uint32)gnode_array_size(f->uplist);
	upvalue->is_direct = (n == 1);
	f->uplist->insert(upvalue);
	// return symbol position in uplist
	return upvalue;
}

gnode_t * gnode2class(gnode_t * node, bool * isextern) 
{
	ASSIGN_PTR(isextern, false);
	if(gnode_t::IsA(node, NODE_CLASS_DECL)) {
		gnode_class_decl_t * c = (gnode_class_decl_t *)node;
		ASSIGN_PTR(isextern, (c->storage == TOK_KEY_EXTERN));
		return node;
	}
	else if(gnode_t::IsA(node, NODE_VARIABLE)) {
		gnode_var_t * var = (gnode_var_t *)node;
		const char * class_manifest_type = GravityEnv.P_ClsClass->identifier;
		if(var->annotation_type && sstreq(var->annotation_type, class_manifest_type) && gnode_t::IsA(var->expr, NODE_CLASS_DECL)) 
			return var->expr;
		else {
			gnode_variable_decl_t * vdecl = var->vdecl;
			if(vdecl && isextern && (vdecl->storage == TOK_KEY_EXTERN)) {
				*isextern = true;
				return node;
			}
		}
	}
	return NULL;
}

// MARK: - Statements initializers -

/*gnode_t * gnode_jump_stat_create(gtoken_s token, gnode_t * expr, gnode_t * decl) 
{
	return new gnode_jump_stmt_t(token, decl, expr);
	//gnode_jump_stmt_t * node = (gnode_jump_stmt_t*)mem_alloc(NULL, sizeof(gnode_jump_stmt_t));
	//node->Setup(NODE_JUMP_STAT, token, decl);
	//node->expr = expr;
	//return node;
}*/

gnode_t * gnode_label_stat_create(gtoken_s token, gnode_t * expr, gnode_t * stmt, gnode_t * decl) 
{
	gnode_label_stmt_t * node = (gnode_label_stmt_t*)mem_alloc(NULL, sizeof(gnode_label_stmt_t));
	node->Setup(NODE_LABEL_STAT, token, decl);
	node->expr = expr;
	node->stmt = stmt;
	return node;
}

gnode_t * gnode_flow_stat_create(gtoken_s token, gnode_t * cond, gnode_t * stmt1, gnode_t * stmt2, gnode_t * decl, uint32 block_length) 
{
	gnode_flow_stmt_t * node = (gnode_flow_stmt_t*)mem_alloc(NULL, sizeof(gnode_flow_stmt_t));
	node->Setup(NODE_FLOW_STAT, token, decl);
	node->BlockLength = block_length;
	node->cond = cond;
	node->stmt = stmt1;
	node->elsestmt = stmt2;
	return node;
}

gnode_t * gnode_loop_stat_create(gtoken_s token, gnode_t * cond, gnode_t * stmt, gnode_t * expr, gnode_t * decl, uint32 block_length) 
{
	gnode_loop_stmt_t * node = (gnode_loop_stmt_t*)mem_alloc(NULL, sizeof(gnode_loop_stmt_t));
	node->Setup(NODE_LOOP_STAT, token, decl);
	node->BlockLength = block_length;
	node->cond = cond;
	node->stmt = stmt;
	node->expr = expr;
	node->nclose = UINT32_MAX;
	return node;
}

gnode_t * gnode_block_stat_create(gnode_n type, gtoken_s token, GravityArray <gnode_t *> * stmts, gnode_t * decl, uint32 block_length) 
{
	gnode_compound_stmt_t * node = (gnode_compound_stmt_t*)mem_alloc(NULL, sizeof(gnode_compound_stmt_t));
	node->Setup(type, token, decl);
	node->BlockLength = block_length;
	node->stmts = stmts;
	node->nclose = UINT32_MAX;
	return node;
}

gnode_t * gnode_empty_stat_create(gtoken_s token, gnode_t * decl) 
{
	gnode_empty_stmt_t * node = (gnode_empty_stmt_t*)mem_alloc(NULL, sizeof(gnode_empty_stmt_t));
	node->Setup(NODE_EMPTY_STAT, token, decl);
	return node;
}

// MARK: - Declarations initializers -

gnode_t * gnode_class_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier,
    gtoken_t storage_specifier, gnode_t * superclass, GravityArray <gnode_t *> * protocols, GravityArray <gnode_t *> * declarations, bool is_struct, gnode_t * decl) 
{
	gnode_class_decl_t * node = (gnode_class_decl_t *)mem_alloc(NULL, sizeof(gnode_class_decl_t));
	node->is_struct = is_struct;
	node->Setup(NODE_CLASS_DECL, token, decl);
	node->bridge = false;
	node->identifier = identifier;
	node->access = access_specifier;
	node->storage = storage_specifier;
	node->superclass = superclass;
	node->protocols = protocols;
	node->decls = declarations;
	node->nivar = 0;
	node->nsvar = 0;
	return node;
}

gnode_t * gnode_module_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier, gtoken_t storage_specifier,
    GravityArray <gnode_t *> * declarations, gnode_t * decl) 
{
	gnode_module_decl_t * node = (gnode_module_decl_t*)mem_alloc(NULL, sizeof(gnode_module_decl_t));
	node->Setup(NODE_MODULE_DECL, token, decl);
	node->identifier = identifier;
	node->access = access_specifier;
	node->storage = storage_specifier;
	node->decls = declarations;
	return node;
}

gnode_t * gnode_enum_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier,
    gtoken_t storage_specifier, symboltable_t * symtable, gnode_t * decl) 
{
	gnode_enum_decl_t * node = (gnode_enum_decl_t*)mem_alloc(NULL, sizeof(gnode_enum_decl_t));
	node->Setup(NODE_ENUM_DECL, token, decl);
	node->identifier = identifier;
	node->access = access_specifier;
	node->storage = storage_specifier;
	node->symtable = symtable;
	return node;
}

gnode_t * gnode_function_decl_create(gtoken_s token, const char * identifier, gtoken_t access_specifier,
    gtoken_t storage_specifier, GravityArray <gnode_t *> * params, gnode_compound_stmt_t * block, gnode_t * decl) 
{
	gnode_function_decl_t * node = (gnode_function_decl_t*)mem_alloc(NULL, sizeof(gnode_function_decl_t));
	node->Setup(NODE_FUNCTION_DECL, token, decl);
	node->identifier = identifier;
	node->access = access_specifier;
	node->storage = storage_specifier;
	node->params = params;
	node->block = block;
	node->nlocals = 0;
	node->uplist = NULL;
	return node;
}

gnode_t * gnode_variable_decl_create(gtoken_s token, gtoken_t type, gtoken_t access_specifier, gtoken_t storage_specifier,
    GravityArray <gnode_t *> * declarations, gnode_t * decl) 
{
	gnode_variable_decl_t * node = (gnode_variable_decl_t*)mem_alloc(NULL, sizeof(gnode_variable_decl_t));
	node->Setup(NODE_VARIABLE_DECL, token, decl);
	node->type = type;
	node->access = access_specifier;
	node->storage = storage_specifier;
	node->decls = declarations;
	return node;
}

gnode_t * gnode_variable_create(gtoken_s token, const char * identifier, const char * annotation_type,
    gnode_t * expr, gnode_t * decl, gnode_variable_decl_t * vdecl) 
{
	gnode_var_t * node = (gnode_var_t *)mem_alloc(NULL, sizeof(gnode_var_t));
	node->Setup(NODE_VARIABLE, token, decl);
	node->identifier = identifier;
	node->annotation_type = annotation_type;
	node->expr = expr;
	node->vdecl = vdecl;
	node->iscomputed = false;
	return node;
}

// MARK: - Expressions initializers -

bool gnode_is_equal(const gnode_t * node1, const gnode_t * node2) 
{
	// very simple gnode verification for map key uniqueness
	const gnode_base_t * _node1 = reinterpret_cast<const gnode_base_t *>(node1);
	const gnode_base_t * _node2 = reinterpret_cast<const gnode_base_t *>(node2);
	if(_node1->Tag != _node2->Tag) 
		return false;
	else if(gnode_is_literal(node1)) {
		const gnode_literal_expr_t * e1 = reinterpret_cast<const gnode_literal_expr_t *>(node1);
		const gnode_literal_expr_t * e2 = reinterpret_cast<const gnode_literal_expr_t *>(node2);
		if(e1->type != e2->type) 
			return false;
		// LITERAL_STRING, LITERAL_FLOAT, LITERAL_INT, LITERAL_BOOL, LITERAL_STRING_INTERPOLATED
		else if(e1->type == LITERAL_BOOL) 
			return (e1->value.n64 == e2->value.n64);
		else if(e1->type == LITERAL_INT) 
			return (e1->value.n64 == e2->value.n64);
		else if(e1->type == LITERAL_FLOAT) 
			return (e1->value.d == e2->value.d);
		else if(e1->type == LITERAL_STRING) 
			return LOGIC(sstreq(e1->value.str, e2->value.str));
		// there is no way to check node equality for a LITERAL_STRING_INTERPOLATED at compile time
	}
	return false;
}

bool gnode_is_expression(const gnode_t * node) 
{
	const gnode_base_t * _node = reinterpret_cast<const gnode_base_t *>(node);
	return ((_node->Tag >= NODE_BINARY_EXPR) && (_node->Tag <= NODE_KEYWORD_EXPR));
}

bool FASTCALL gnode_is_literal(const gnode_t * node) 
{
	const gnode_base_t * _node = reinterpret_cast<const gnode_base_t *>(node);
	return (_node->Tag == NODE_LITERAL_EXPR);
}

bool gnode_is_literal_int(const gnode_t * node) 
{
	if(gnode_is_literal(node) == false) 
		return false;
	else {
		const gnode_literal_expr_t * _node = reinterpret_cast<const gnode_literal_expr_t *>(node);
		return (_node->type == LITERAL_INT);
	}
}

bool gnode_is_literal_string(const gnode_t * node) 
{
	if(gnode_is_literal(node) == false) 
		return false;
	else {
		gnode_literal_expr_t * _node = (gnode_literal_expr_t *)node;
		return (_node->type == LITERAL_STRING);
	}
}

bool gnode_is_literal_number(gnode_t * node) 
{
	if(gnode_is_literal(node) == false) 
		return false;
	else {
		gnode_literal_expr_t * _node = (gnode_literal_expr_t *)node;
		return (_node->type != LITERAL_STRING && _node->type != LITERAL_STRING_INTERPOLATED);
	}
}

gnode_t * gnode_binary_expr_create(gtoken_t op, gnode_t * left, gnode_t * right, gnode_t * decl) 
{
	gnode_binary_expr_t * node = 0;
	if(left && right) {
		node = (gnode_binary_expr_t*)mem_alloc(NULL, sizeof(gnode_binary_expr_t));
		node->Setup(NODE_BINARY_EXPR, left->Token, decl);
		node->op = op;
		node->left = left;
		node->right = right;
	}
	return node;
}

gnode_t * gnode_unary_expr_create(gtoken_t op, gnode_t * expr, gnode_t * decl) 
{
	gnode_unary_expr_t * node = 0;
	if(expr) {
		node = static_cast<gnode_unary_expr_t *>(mem_alloc(NULL, sizeof(gnode_unary_expr_t)));
		node->Setup(NODE_UNARY_EXPR, expr->Token, decl);
		node->op = op;
		node->expr = expr;
	}
	return node;
}

gnode_t * gnode_file_expr_create(gtoken_s token, GravityArray <const char *> * list, gnode_t * decl) 
{
	gnode_file_expr_t * node = 0;
	if(list) {
		node = (gnode_file_expr_t*)mem_alloc(NULL, sizeof(gnode_file_expr_t));
		node->Setup(NODE_FILE_EXPR, token, decl);
		node->identifiers = list;
	}
	return node;
}

gnode_t * gnode_identifier_expr_create(gtoken_s token, const char * identifier, const char * identifier2, gnode_t * decl) 
{
	gnode_identifier_expr_t * node = 0;
	if(identifier) {
		node = (gnode_identifier_expr_t *)mem_alloc(NULL, sizeof(gnode_identifier_expr_t));
		node->Setup(NODE_IDENTIFIER_EXPR, token, decl);
		node->value = identifier;
		node->value2 = identifier2;
	}
	return node;
}

void gnode_literal_dump(gnode_literal_expr_t * node, char * buffer, int buffersize) 
{
	switch(node->type) {
		case LITERAL_STRING_INTERPOLATED: snprintf(buffer, buffersize, "INTERPOLATED: %d",
			(uint32)gnode_array_size(node->value.r)); break;
		case LITERAL_STRING: snprintf(buffer, buffersize, "STRING: %.*s", node->len, node->value.str); break;
		case LITERAL_FLOAT: snprintf(buffer, buffersize, "FLOAT: %.2f", node->value.d); break;
		case LITERAL_INT: snprintf(buffer, buffersize, "INT: %" PRId64, (int64_t)node->value.n64); break;
		case LITERAL_BOOL: snprintf(buffer, buffersize, "BOOL: %d", (int32)node->value.n64); break;
		default: assert(0); // should never reach this point
	}
}

static gnode_t * gnode_literal_value_expr_create(gtoken_s token, gliteral_t type, const char * s, double d, int64_t n64, gnode_t * decl) 
{
	gnode_literal_expr_t * node = (gnode_literal_expr_t *)mem_alloc(NULL, sizeof(gnode_literal_expr_t));
	node->Setup(NODE_LITERAL_EXPR, token, decl);
	node->type = type;
	node->len = 0;
	switch(type) {
		case LITERAL_STRING: node->value.str = (char *)s; break;
		case LITERAL_FLOAT: node->value.d = d; node->len = (d < FLT_MAX) ? 32 : 64; break;
		case LITERAL_INT: node->value.n64 = n64; node->len = (n64 < 2147483647) ? 32 : 64; break;
		case LITERAL_BOOL: node->value.n64 = n64; node->len = 32; break;
		case LITERAL_STRING_INTERPOLATED: break;
		default: assert(0); // should never reach this point
	}
	return node;
}

gnode_t * gnode_string_interpolation_create(gtoken_s token, GravityArray <gnode_t *> * r, gnode_t * decl) 
{
	gnode_literal_expr_t * node = (gnode_literal_expr_t *)gnode_literal_value_expr_create(token, LITERAL_STRING_INTERPOLATED, NULL, 0, 0, decl);
	node->value.r = r;
	return node;
}

gnode_t * gnode_literal_string_expr_create(gtoken_s token, char * s, uint32 len, bool allocated, gnode_t * decl) 
{
	gnode_literal_expr_t * node = (gnode_literal_expr_t *)gnode_literal_value_expr_create(token, LITERAL_STRING, NULL, 0, 0, decl);
	node->len = len;
	if(allocated) {
		node->value.str = s;
	}
	else {
		node->value.str = static_cast<char *>(mem_alloc(NULL, len+1));
		memcpy((void*)node->value.str, s, len);
	}
	return node;
}

gnode_t * gnode_literal_float_expr_create(gtoken_s token, double d, gnode_t * decl) 
	{ return gnode_literal_value_expr_create(token, LITERAL_FLOAT, NULL, d, 0, decl); }
gnode_t * gnode_literal_int_expr_create(gtoken_s token, int64_t n, gnode_t * decl) 
	{ return gnode_literal_value_expr_create(token, LITERAL_INT, NULL, 0, n, decl); }
gnode_t * gnode_literal_bool_expr_create(gtoken_s token, int32 n, gnode_t * decl) 
	{ return gnode_literal_value_expr_create(token, LITERAL_BOOL, NULL, 0, n, decl); }

gnode_t * gnode_keyword_expr_create(gtoken_s token, gnode_t * decl) 
{
	gnode_keyword_expr_t * node = static_cast<gnode_keyword_expr_t *>(mem_alloc(NULL, sizeof(gnode_keyword_expr_t)));
	node->Setup(NODE_KEYWORD_EXPR, token, decl);
	return node;
}

gnode_t * gnode_postfix_subexpr_create(gtoken_s token, gnode_n type, gnode_t * expr, GravityArray <gnode_t *> * list, gnode_t * decl) 
{
	gnode_postfix_subexpr_t * node = (gnode_postfix_subexpr_t*)mem_alloc(NULL, sizeof(gnode_postfix_subexpr_t));
	node->Setup(type, token, decl);
	if(type == NODE_CALL_EXPR)
		node->args = list;
	else
		node->expr = expr;
	return node;
}

gnode_t * gnode_postfix_expr_create(gtoken_s token, gnode_t * id, GravityArray <gnode_t *> * list, gnode_t * decl) 
{
	gnode_postfix_expr_t * node = (gnode_postfix_expr_t *)mem_alloc(NULL, sizeof(gnode_postfix_expr_t));
	node->Setup(NODE_POSTFIX_EXPR, token, decl);
	node->id = id;
	node->list = list;
	return node;
}

gnode_t * gnode_list_expr_create(gtoken_s token, GravityArray <gnode_t *> * list1, GravityArray <gnode_t *> * list2, bool ismap, gnode_t * decl) 
{
	gnode_list_expr_t * node = (gnode_list_expr_t*)mem_alloc(NULL, sizeof(gnode_list_expr_t));
	node->Setup(NODE_LIST_EXPR, token, decl);
	node->ismap = ismap;
	node->list1 = list1;
	node->list2 = list2;
	return node;
}

// MARK: -

gnode_t * gnode_duplicate(gnode_t * node, bool deep) 
{
	if(!node) 
		return NULL;
	if(deep == true) {
		// deep is true so I need to examine node and perform a real duplication (only of the outer nodes)
		// deep is true ONLY when node can also be part of an assignment and its assignment flag can be
		// true is node is on the left and false when node is on the right
		// true flag is used only by adjust_assignment_expression in parser.c

		// node can be: identifier, file or postfix
		if(gnode_t::IsA(node, NODE_IDENTIFIER_EXPR)) {
			gnode_identifier_expr_t * expr = (gnode_identifier_expr_t*)node;
			return gnode_identifier_expr_create(expr->Token, sstrdup(expr->value),
				   (expr->value2) ? sstrdup(expr->value2) : NULL, static_cast<gnode_t *>(expr->P_Decl));
		}
		else if(gnode_t::IsA(node, NODE_FILE_EXPR)) {
			gnode_file_expr_t * expr = (gnode_file_expr_t*)node;
			GravityArray <const char *> * list = cstring_array_create();
			uint count = gnode_array_size(expr->identifiers);
			for(uint i = 0; i < count; ++i) {
				const char * identifier = gnode_array_get(expr->identifiers, i);
				cstring_array_push(list, sstrdup(identifier));
			}
			return gnode_file_expr_create(expr->Token, list, static_cast<gnode_t *>(expr->P_Decl));
		}
		else if(gnode_t::IsA(node, NODE_POSTFIX_EXPR)) {
			gnode_postfix_expr_t * expr = (gnode_postfix_expr_t *)node;
			gnode_t * id = gnode_duplicate(expr->id, false);
			GravityArray <gnode_t *> * list = gnode_array_create();
			gnode_array_each(expr->list, {gnode_array_push(list, gnode_duplicate(val, false));});
			return gnode_postfix_expr_create(expr->Token, id, list, static_cast<gnode_t *>(expr->P_Decl));
		}
		else {
			// gnode_duplicate UNHANDLED case
			return NULL;
		}
		// just return the original node and since it is invalid for an assignment a semantic error will be
		// generated
	}
	// it means that I can perform a light duplication where
	// duplicating a node means increase its refcount so it isn't freed more than once
	++node->RefCount;
	return node;
}

// MARK: - AST deallocator -

// STATEMENTS
static void free_list_stmt(gvisitor_t * self, gnode_compound_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gnode_array_each(node->stmts, { gvisit(self, val); });
	if(node->stmts) 
		gnode_array_free(node->stmts);
	symboltable_free(node->symtable);
	mem_free(node);
}

static void free_compound_stmt(gvisitor_t * self, gnode_compound_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gnode_array_each(node->stmts, { gvisit(self, val); });
	if(node->stmts) 
		gnode_array_free(node->stmts);
	symboltable_free(node->symtable);
	mem_free(node);
}

static void free_label_stmt(gvisitor_t * self, gnode_label_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->expr);
	gvisit(self, node->stmt);
	mem_free(node);
}

static void free_flow_stmt(gvisitor_t * self, gnode_flow_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->cond);
	gvisit(self, node->stmt);
	gvisit(self, node->elsestmt);
	mem_free(node);
}

static void free_loop_stmt(gvisitor_t * self, gnode_loop_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->stmt);
	gvisit(self, node->cond);
	gvisit(self, node->expr);
	mem_free(node);
}

static void free_jump_stmt(gvisitor_t * self, gnode_jump_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->expr);
	mem_free(node);
}

static void free_empty_stmt(gvisitor_t * self, gnode_empty_stmt_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free(node);
}

static void free_variable(gvisitor_t * self, gnode_var_t * p) 
{
	CHECK_REFCOUNT(p);
	mem_free((void*)p->identifier);
	mem_free((void*)p->annotation_type);
	gvisit(self, p->expr);
	mem_free((void*)p);
}

static void free_function_decl(gvisitor_t * self, gnode_function_decl_t * node) 
{
	CHECK_REFCOUNT(node);
	symboltable_free(node->symtable);
	mem_free((void*)node->identifier);
	if(node->params) {
		gnode_array_each(node->params, {free_variable(self, (gnode_var_t *)val);});
		gnode_array_free(node->params);
	}
	gvisit(self, node->block);
	if(node->uplist) {
		gtype_array_each(node->uplist, {mem_free(val);}, gupvalue_t*);
		gnode_array_free(node->uplist);
	}
	mem_free(node);
}

static void free_variable_decl(gvisitor_t * self, gnode_variable_decl_t * node) 
{
	CHECK_REFCOUNT(node);
	if(node->decls) {
		gnode_array_each(node->decls, {free_variable(self, (gnode_var_t *)val);});
		gnode_array_free(node->decls);
	}
	mem_free(node);
}

static void free_enum_decl(gvisitor_t * self, gnode_enum_decl_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free((void*)node->identifier);
	symboltable_free(node->symtable);
	mem_free(node);
}

static void free_class_decl(gvisitor_t * self, gnode_class_decl_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free((void*)node->identifier);
	if(node->decls) {
		gnode_array_each(node->decls, { gvisit(self, val); });
		gnode_array_free(node->decls);
	}
	symboltable_free(node->symtable);
	mem_free(node);
}

static void free_module_decl(gvisitor_t * self, gnode_module_decl_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free((void*)node->identifier);
	if(node->decls) {
		gnode_array_each(node->decls, {gvisit(self, val);});
		gnode_array_free(node->decls);
	}
	symboltable_free(node->symtable);
	mem_free(node);
}

static void free_binary_expr(gvisitor_t * self, gnode_binary_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->left);
	gvisit(self, node->right);
	mem_free(node);
}

static void free_unary_expr(gvisitor_t * self, gnode_unary_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->expr);
	mem_free(node);
}

static void free_postfix_subexpr(gvisitor_t * self, gnode_postfix_subexpr_t * subnode) 
{
	CHECK_REFCOUNT(subnode);
	gnode_n tag = subnode->Tag;
	if(tag == NODE_CALL_EXPR) {
		if(subnode->args) {
			gnode_array_each(subnode->args, gvisit(self, val); );
			gnode_array_free(subnode->args);
		}
	}
	else {
		gvisit(self, subnode->expr);
	}
	mem_free(subnode);
}

static void free_postfix_expr(gvisitor_t * self, gnode_postfix_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	gvisit(self, node->id);
	// node->list can be NULL due to enum static conversion
	const uint count = gnode_array_size(node->list);
	for(uint i = 0; i < count; ++i) {
		gnode_postfix_subexpr_t * subnode = (gnode_postfix_subexpr_t*)gnode_array_get(node->list, i);
		free_postfix_subexpr(self, subnode);
	}
	if(node->list) 
		gnode_array_free(node->list);
	mem_free(node);
}

static void free_file_expr(gvisitor_t * self, gnode_file_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	cstring_array_each(node->identifiers, {
		mem_free((void*)val);
	});

	if(node->identifiers) gnode_array_free(node->identifiers);
	mem_free((void*)node);
}

static void free_literal_expr(gvisitor_t * self, gnode_literal_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	if(node->type == LITERAL_STRING) mem_free((void*)node->value.str);
	else if(node->type == LITERAL_STRING_INTERPOLATED) {
		gnode_array_each(node->value.r, {gvisit(self, val);})
		gnode_array_free(node->value.r);
	}
	mem_free((void*)node);
}

static void free_identifier_expr(gvisitor_t * self, gnode_identifier_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free((void*)node->value);
	mem_free((void*)node->value2);
	mem_free((void*)node);
}

static void free_keyword_expr(gvisitor_t * self, gnode_keyword_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	mem_free((void*)node);
}

static void free_list_expr(gvisitor_t * self, gnode_list_expr_t * node) 
{
	CHECK_REFCOUNT(node);
	if(node->list1) {
		gnode_array_each(node->list1, {gvisit(self, val);});
		gnode_array_free(node->list1);
	}
	if(node->list2) {
		gnode_array_each(node->list2, {gvisit(self, val);});
		gnode_array_free(node->list2);
	}
	mem_free(node);
}

// MARK: -

void FASTCALL gnode_free(gnode_t * ast) 
{
	if(ast) {
		gvisitor_t visitor(0, 0);
		visitor.visit_list_stmt = free_list_stmt;
		visitor.visit_compound_stmt = free_compound_stmt;
		visitor.visit_label_stmt = free_label_stmt;
		visitor.visit_flow_stmt = free_flow_stmt;
		visitor.visit_loop_stmt = free_loop_stmt;
		visitor.visit_jump_stmt = free_jump_stmt;
		visitor.visit_empty_stmt = free_empty_stmt;
		// DECLARATIONS: 5
		visitor.visit_function_decl = free_function_decl;
		visitor.visit_variable_decl = free_variable_decl;
		visitor.visit_enum_decl = free_enum_decl;
		visitor.visit_class_decl = free_class_decl;
		visitor.visit_module_decl = free_module_decl;
		// EXPRESSIONS: 7+1
		visitor.visit_binary_expr = free_binary_expr;
		visitor.visit_unary_expr = free_unary_expr;
		visitor.visit_file_expr = free_file_expr;
		visitor.visit_literal_expr = free_literal_expr;
		visitor.visit_identifier_expr = free_identifier_expr;
		visitor.visit_keyword_expr = free_keyword_expr;
		visitor.visit_list_expr = free_list_expr;
		visitor.visit_postfix_expr = free_postfix_expr;
		/*= {
			.nerr = 0,
			.data = NULL,
			.delegate = NULL,

			// COMMON
			.visit_pre = NULL,
			.visit_post = NULL,

			// STATEMENTS: 7
			.visit_list_stmt = free_list_stmt,
			.visit_compound_stmt = free_compound_stmt,
			.visit_label_stmt = free_label_stmt,
			.visit_flow_stmt = free_flow_stmt,
			.visit_loop_stmt = free_loop_stmt,
			.visit_jump_stmt = free_jump_stmt,
			.visit_empty_stmt = free_empty_stmt,

			// DECLARATIONS: 5
			.visit_function_decl = free_function_decl,
			.visit_variable_decl = free_variable_decl,
			.visit_enum_decl = free_enum_decl,
			.visit_class_decl = free_class_decl,
			.visit_module_decl = free_module_decl,

			// EXPRESSIONS: 7+1
			.visit_binary_expr = free_binary_expr,
			.visit_unary_expr = free_unary_expr,
			.visit_file_expr = free_file_expr,
			.visit_literal_expr = free_literal_expr,
			.visit_identifier_expr = free_identifier_expr,
			.visit_keyword_expr = free_keyword_expr,
			.visit_list_expr = free_list_expr,
			.visit_postfix_expr = free_postfix_expr
		};*/
		gvisit(&visitor, ast);
	}
}
