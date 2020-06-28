//  gravity_ircode.c
//  gravity
//
//  Created by Marco Bambini on 06/11/14.
//  Copyright (c) 2014 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

//typedef marray_t(inst_t *)      code_r;
//typedef marray_t(bool *)        context_r;
typedef GravityArray <inst_t *> code_r;
typedef GravityArray <bool *>   context_r;

struct ircode_t {
	code_r * list;        // array of ircode instructions
	uint32_r label_true;  // labels used in loops
	uint32_r label_false;
	uint32_r label_check;
	uint32 label_counter;
	uint32 maxtemp;       // maximum number of temp registers used in this ircode
	uint32 ntemps;        // current number of temp registers in use
	uint16 nlocals;       // number of local registers (params + local variables)
	bool   error;         // error flag set when no more registers are availables
	uint8  Reserve[1];    // @alignment
	bool   state[MAX_REGISTERS];          // registers mask
	bool   skipclear[MAX_REGISTERS];      // registers protection for temps used in for loop
	uint32_r registers;                 // registers stack
	context_r context;                  // context array
};

ircode_t * ircode_create(uint16 nlocals) 
{
	ircode_t * code = static_cast<ircode_t *>(mem_alloc(NULL, sizeof(ircode_t)));
	if(code) {
		code->label_counter = 0;
		code->nlocals = nlocals;
		code->ntemps = 0;
		code->maxtemp = 0;
		code->error = false;
		code->list = static_cast<code_r *>(mem_alloc(NULL, sizeof(code_r)));
		if(!code->list) 
			return NULL;
		marray_init(*code->list);
		marray_init(code->label_true);
		marray_init(code->label_false);
		marray_init(code->label_check);
		marray_init(code->registers);
		marray_init(code->context);
		// init state array (register 0 is reserved)
		memzero(code->state, MAX_REGISTERS * sizeof(bool));
		code->state[0] = true;
		for(uint32 i = 0; i < nlocals; ++i) {
			code->state[i] = true;
		}
	}
	return code;
}

void ircode_free(ircode_t * code) 
{
	if(code) {
		uint32 count = ircode_count(code);
		for(uint32 i = 0; i<count; ++i) {
			inst_t * inst = code->list->at(i);
			mem_free(inst);
		}
		code->list->Z();
		code->context.Z();
		code->registers.Z();
		code->label_true.Z();
		code->label_false.Z();
		code->label_check.Z();
		mem_free(code->list);
		mem_free(code);
	}
}

uint32 ircode_ntemps(const ircode_t * code) { return code->ntemps; }
uint32 ircode_count(const ircode_t * code) { return code->list->getCount(); }
bool   ircode_iserror(const ircode_t * code) { return code->error; }

inst_t * ircode_get(ircode_t * code, uint32 index) 
{
	uint32 n = code->list->getCount();
	return (index >= n) ? NULL : code->list->at(index);
}

// MARK: -

static inst_t * FASTCALL inst_new(opcode_t op, uint32 p1, uint32 p2, uint32 p3, optag_t tag, int64_t n, double d, uint32 lineno) 
{
	// debug code
    #if GRAVITY_OPCODE_DEBUG
	if(tag == LABEL_TAG) {
		DEBUG_OPCODE("LABEL %d", p1);
	}
	else if(tag != PRAGMA_MOVE_OPTIMIZATION) {
		const char * op_name = opcode_name(op);
		if(op == LOADI) {
			if(tag == DOUBLE_TAG)
				printf("%s %d %.2f\n", op_name, p1, d);
			else
				printf("%s %d %lld\n", op_name, p1, n);
		}
		else {
			int nop = opcode_numop(op);
			if(nop == 0) {
				DEBUG_OPCODE("%s", op_name);
			}
			else if(nop == 1) {
				DEBUG_OPCODE("%s %d", op_name, p1);
			}
			else if(nop == 2) {
				DEBUG_OPCODE("%s %d %d", op_name, p1, p2);
			}
			else if(nop == 3) {
				DEBUG_OPCODE("%s %d %d %d", opcode_name(op), p1, p2, p3);
			}
		}
	}
    #endif
	inst_t * inst = static_cast<inst_t *>(mem_alloc(NULL, sizeof(inst_t)));
	inst->op = op;
	inst->tag = tag;
	inst->p1 = p1;
	inst->p2 = p2;
	inst->p3 = p3;
	inst->lineno = lineno;
	if(tag == DOUBLE_TAG) 
		inst->d = d;
	else if(tag == INT_TAG) 
		inst->n = n;
	assert(inst);
	return inst;
}

void FASTCALL inst_setskip(inst_t * inst) { inst->tag = SKIP_TAG; }

void ircode_patch_init(ircode_t * code, uint16 index) 
{
	// prepend call instructions to code
	// LOADK temp index
	// LOAD  temp 0 temp
	// MOVE  temp+1 0
	// CALL  temp temp 1
	// load constant
	uint32 dest = ircode_register_push_temp(code);
	inst_t * inst1 = inst_new(LOADK, dest, index, 0, NO_TAG, 0, 0.0, 0);
	// load from lookup
	inst_t * inst2 = inst_new(LOAD, dest, 0, dest, NO_TAG, 0, 0.0, 0);
	// prepare parameter
	uint32 dest2 = ircode_register_push_temp(code);
	inst_t * inst3 = inst_new(MOVE, dest2, 0, 0, NO_TAG, 0, 0.0, 0);
	ircode_register_pop(code);
	// execute call
	inst_t * inst4 = inst_new(CALL, dest, dest, 1, NO_TAG, 0, 0.0, 0);
	// pop temps used
	ircode_register_pop(code);
	// create new instruction list
	code_r * list = (code_r *)(mem_alloc(NULL, sizeof(code_r)));
	marray_init(*list);
	// add newly create instructions
	list->insert(inst1);
	list->insert(inst2);
	list->insert(inst3);
	list->insert(inst4);
	// then copy original instructions
	code_r * orig_list = code->list;
	const uint32 count = ircode_count(code);
	for(uint32 i = 0; i < count; ++i) {
		inst_t * inst = orig_list->at(i);
		list->insert(inst);
	}
	// free dest list
	orig_list->Z();
	mem_free(code->list);
	// replace dest list with the newly created list
	code->list = list;
}

uint8_t FASTCALL opcode_numop(opcode_t op) 
{
	switch(op) {
		case HALT: return 0;
		case NOP: return 0;
		case RET0: return 0;
		case RET: return 1;
		case CALL: return 3;
		case SETLIST: return 3;
		case LOADK: return 2;
		case LOADG: return 2;
		case LOADI: return 2;
		case LOADAT: return 3;
		case LOADS: return 3;
		case LOAD: return 3;
		case LOADU: return 2;
		case MOVE: return 2;
		case STOREG: return 2;
		case STOREAT: return 3;
		case STORE: return 3;
		case STOREU: return 2;
		case JUMP: return 1;
		case JUMPF: return 2;
		case SWITCH: return 1;
		case ADD: return 3;
		case SUB: return 3;
		case DIV: return 3;
		case MUL: return 3;
		case REM: return 3;
		case AND: return 3;
		case OR: return 3;
		case LT: return 3;
		case GT: return 3;
		case EQ: return 3;
		case ISA: return 3;
		case MATCH: return 3;
		case EQQ: return 3;
		case LEQ: return 3;
		case GEQ: return 3;
		case NEQ: return 3;
		case NEQQ: return 3;
		case NEG: return 2;
		case NOT: return 2;
		case LSHIFT: return 3;
		case RSHIFT: return 3;
		case BAND: return 3;
		case BOR: return 3;
		case BXOR: return 3;
		case BNOT: return 2;
		case MAPNEW: return 2;
		case LISTNEW: return 2;
		case RANGENEW: return 3;
		case CLOSURE: return 2;
		case CLOSE: return 1;
		case CHECK: return 1;
		case RESERVED2:
		case RESERVED3:
		case RESERVED4:
		case RESERVED5:
		case RESERVED6: return 0;
	}
	assert(0);
	return 0;
}

void ircode_dump(const void * _code) // code_dump_function
{
	const ircode_t * code = static_cast<const ircode_t *>(_code);
	const code_r * list = code->list;
	uint32 count = ircode_count(code);
	if(count == 0) {
		printf("NONE\n");
		return;
	}
	for(uint32 i = 0, line = 0; i<count; ++i) {
		const inst_t * inst = list->at(i);
		opcode_t op = inst->op;
		int32 p1 = inst->p1;
		int32 p2 = inst->p2;
		int32 p3 = inst->p3;
		if(inst->tag == SKIP_TAG) continue;
		if(inst->tag == PRAGMA_MOVE_OPTIMIZATION) continue;
		if(inst->tag == LABEL_TAG) {
			printf("LABEL %d:\n", p1); continue;
		}
		uint8_t n = opcode_numop(op);
		if((op == SETLIST) && (p2 == 0)) n = 2;

		// set to 1 to debug line number for each instruction
	#if 0
		printf("(%d)\t\t", inst->lineno);
	#endif
		switch(n) {
			case 0: 
			    printf("%05d\t%s\n", line, opcode_name(op));
			case 1:
			    printf("%05d\t%s %d\n", line, opcode_name(op), p1);
				break;
			case 2: 
				{
					if(op == LOADI) {
						if(inst->tag == DOUBLE_TAG) printf("%05d\t%s %d %.2f\n", line, opcode_name(op), p1, inst->d);
				#if defined(_WIN32)
						else printf("%05d\t%s %d %I64d\n", line, opcode_name(op), p1, inst->n);
				#else
						else printf("%05d\t%s %d %" PRId64 "\n", line, opcode_name(op), p1, inst->n);
				#endif
					}
					else if(op == LOADK) {
						if(p2 < CPOOL_INDEX_MAX) printf("%05d\t%s %d %d\n", line, opcode_name(op), p1, p2);
						else printf("%05d\t%s %d %s\n", line, opcode_name(op), p1, opcode_constname(p2));
					}
					else {
						printf("%05d\t%s %d %d\n", line, opcode_name(op), p1, p2);
					}
				} 
				break;

			case 3: 
			    printf("%05d\t%s %d %d %d\n", line, opcode_name(op), p1, p2, p3);
				break;
			default: 
				assert(0);
		}
		++line;
	}
}

// MARK: -

uint32 FASTCALL ircode_newlabel(ircode_t * code) { return ++code->label_counter; }
void ircode_setlabel_true(ircode_t * code, uint32 nlabel) { code->label_true.insert(nlabel); }
void ircode_setlabel_false(ircode_t * code, uint32 nlabel) { code->label_false.insert(nlabel); }
void ircode_setlabel_check(ircode_t * code, uint32 nlabel) { code->label_check.insert(nlabel); }
void ircode_unsetlabel_true(ircode_t * code) { code->label_true.pop(); }
void ircode_unsetlabel_false(ircode_t * code) { code->label_false.pop(); }
void ircode_unsetlabel_check(ircode_t * code) { code->label_check.pop(); }

uint32 ircode_getlabel_true(ircode_t * code) 
{
	size_t n = code->label_true.getCount();
	uint32 v = code->label_true.at(n-1);
	return v;
}

uint32 ircode_getlabel_false(ircode_t * code) 
{
	size_t n = code->label_false.getCount();
	uint32 v = code->label_false.at(n-1);
	return v;
}

uint32 ircode_getlabel_check(ircode_t * code) 
{
	size_t n = code->label_check.getCount();
	uint32 v = code->label_check.at(n-1);
	return v;
}

void ircode_marklabel(ircode_t * code, uint32 nlabel, uint32 lineno) 
{
	inst_t * inst = inst_new(RET0, nlabel, 0, 0, LABEL_TAG, 0, 0.0, lineno);
	code->list->insert(inst);
}

// MARK: -
void ircode_pragma(ircode_t * code, optag_t tag, uint32 value, uint32 lineno) 
{
	ircode_add_tag(code, RET0, value, 0, 0, tag, lineno);
}

// MARK: -

void ircode_set_index(uint32 index, ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3) 
{
	inst_t * inst = code->list->at(index);
	inst->op = op;
	inst->p1 = p1;
	inst->p2 = p2;
	inst->p3 = p3;
	inst->tag = NO_TAG;
}

void FASTCALL ircode_add(ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3, uint32 lineno) 
{
	ircode_add_tag(code, op, p1, p2, p3, NO_TAG, lineno);
}

void FASTCALL ircode_add_tag(ircode_t * code, opcode_t op, uint32 p1, uint32 p2, uint32 p3, optag_t tag, uint32 lineno) 
{
	inst_t * inst = inst_new(op, p1, p2, p3, tag, 0, 0.0, lineno);
	code->list->insert(inst);
}

void ircode_add_double(ircode_t * code, double d, uint32 lineno) 
{
	uint32 regnum = ircode_register_push_temp(code);
	inst_t * inst = inst_new(LOADI, regnum, 0, 0, DOUBLE_TAG, 0, d, lineno);
	code->list->insert(inst);
}

void FASTCALL ircode_add_constant(ircode_t * code, uint32 index, uint32 lineno) 
{
	uint32 regnum = ircode_register_push_temp(code);
	inst_t * inst = inst_new(LOADK, regnum, index, 0, NO_TAG, 0, 0, lineno);
	code->list->insert(inst);
}

void ircode_add_int(ircode_t * code, int64_t n, uint32 lineno) 
{
	uint32 regnum = ircode_register_push_temp(code);
	inst_t * inst = inst_new(LOADI, regnum, 0, 0, INT_TAG, n, 0, lineno);
	code->list->insert(inst);
}

void ircode_add_skip(ircode_t * code, uint32 lineno) 
{
	inst_t * inst = inst_new(RET0, 0, 0, 0, NO_TAG, 0, 0, lineno);
	inst_setskip(inst);
	code->list->insert(inst);
}

void ircode_add_check(ircode_t * code) 
{
	inst_t * inst = code->list->getLast();
	if((inst) && (inst->op == MOVE)) {
		inst_t * newinst = inst_new(CHECK, inst->p1, 0, 0, NO_TAG, 0, 0, inst->lineno);
		code->list->insert(newinst);
	}
}

// MARK: - Context based functions -
#if 0
	static void dump_context(bool * context) 
	{
		for(uint32 i = 0; i<MAX_REGISTERS; ++i) {
			printf("%d ", context[i]);
		}
		printf("\n");
	}
#endif

void ircode_push_context(ircode_t * code) 
{
	bool * context = static_cast<bool *>(mem_alloc(NULL, sizeof(bool) * MAX_REGISTERS));
	code->context.insert(context);
}

void ircode_pop_context(ircode_t * code) 
{
	bool * context = code->context.pop();
	// apply context mask
	for(uint32 i = 0; i < MAX_REGISTERS; ++i) {
		if(context[i]) 
			code->state[i] = false;
	}
	mem_free(context);
}

uint32 ircode_register_pop_context_protect(ircode_t * code, bool protect) 
{
	if(code->registers.getCount() == 0) 
		return REGISTER_ERROR;
	uint32 value = code->registers.pop();
	if(protect) 
		code->state[value] = true;
	else if(value >= code->nlocals) 
		code->state[value] = false;
	if(protect && value >= code->nlocals) {
		bool * context = code->context.getLast();
		context[value] = true;
	}
	DEBUG_REGISTER("POP REGISTER %d", value);
	return value;
}

bool ircode_register_protect_outside_context(ircode_t * code, uint32 nreg) 
{
	if(nreg < code->nlocals) 
		return true;
	if(!code->state[nreg]) 
		return false;
	bool * context = code->context.getLast();
	context[nreg] = false;
	return true;
}

void ircode_register_protect_in_context(ircode_t * code, uint32 nreg) 
{
	assert(code->state[nreg]);
	bool * context = code->context.getLast();
	context[nreg] = true;
}

// MARK: -

static uint32 ircode_register_new(ircode_t * code) 
{
	for(uint32 i = 0; i<MAX_REGISTERS; ++i) {
		if(code->state[i] == false) {
			code->state[i] = true;
			return i;
		}
	}
	// 0 means no registers available
	code->error = true;
	return 0;
}

uint32 ircode_register_push(ircode_t * code, uint32 nreg) 
{
	code->registers.insert(nreg);
	if(ircode_register_istemp(code, nreg)) 
		++code->ntemps;
	DEBUG_REGISTER("PUSH REGISTER %d", nreg);
	return nreg;
}

uint32 ircode_register_first_temp_available(ircode_t * code) 
{
	for(uint32 i = 0; i<MAX_REGISTERS; ++i) {
		if(code->state[i] == false) {
			return i;
		}
	}
	// 0 means no registers available
	code->error = true;
	return 0;
}

uint32 ircode_register_push_temp_protected(ircode_t * code) 
{
	uint32 value = ircode_register_push_temp(code);
	ircode_register_temp_protect(code, value);
	return value;
}

uint32 FASTCALL ircode_register_push_temp(ircode_t * code) 
{
	uint32 value = ircode_register_new(code);
	code->registers.insert(value);
	if(value > code->maxtemp) {
		code->maxtemp = value; ++code->ntemps;
	}
	DEBUG_REGISTER("PUSH TEMP REGISTER %d", value);
	return value;
}

uint32 FASTCALL ircode_register_pop(ircode_t * code) { return ircode_register_pop_context_protect(code, false); }

void ircode_register_clear(ircode_t * code, uint32 nreg) 
{
	if(nreg == REGISTER_ERROR) return;
	// cleanup busy mask only if it is a temp register
	if(nreg >= code->nlocals) code->state[nreg] = false;
}

void ircode_register_set(ircode_t * code, uint32 nreg) 
{
	if(nreg == REGISTER_ERROR) return;
	// set busy mask only if it is a temp register
	if(nreg >= code->nlocals) code->state[nreg] = true;
}

uint32 ircode_register_last(ircode_t * code) 
{
	if(code->registers.getCount() == 0) 
		return REGISTER_ERROR;
	return code->registers.getLast();
}

bool ircode_register_istemp(const ircode_t * code, uint32 nreg) { return (nreg >= (uint32)code->nlocals); }

void ircode_register_dump(ircode_t * code) 
{
	uint32 n = code->registers.getCount();
	if(n == 0) 
		printf("EMPTY\n");
	for(uint32 i = 0; i<n; ++i) {
		uint32 value = code->registers.at(i);
		printf("[%d]\t%d\n", i, value);
	}
}

uint32 ircode_register_count(const ircode_t * code) { return code->registers.getCount(); }

// MARK: -

void ircode_register_temp_protect(ircode_t * code, uint32 nreg) 
{
	code->skipclear[nreg] = true;
	DEBUG_REGISTER("SET SKIP REGISTER %d", nreg);
}

void ircode_register_temp_unprotect(ircode_t * code, uint32 nreg) 
{
	code->skipclear[nreg] = false;
	DEBUG_REGISTER("UNSET SKIP REGISTER %d", nreg);
}

void ircode_register_temps_clear(ircode_t * code) {
	// clear all temporary registers (if not protected)
	for(uint32 i = code->nlocals; i<=code->maxtemp; ++i) {
		if(!code->skipclear[i]) {
			code->state[i] = false;
			DEBUG_REGISTER("CLEAR TEMP REGISTER %d", i);
		}
	}
}
