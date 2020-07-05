//  gravity_debug.c
//  gravity
//
//  Created by Marco Bambini on 01/04/16.
//  Copyright (c) 2016 CreoLabs. All rights reserved.
//
#include <gravity_.h>
#pragma hdrstop

const char * opcode_constname(int n) {
	switch(n) {
		case CPOOL_VALUE_SUPER: return "SUPER";
		case CPOOL_VALUE_NULL: return "NULL";
		case CPOOL_VALUE_UNDEFINED: return "UNDEFINED";
		case CPOOL_VALUE_ARGUMENTS: return "ARGUMENTS";
		case CPOOL_VALUE_TRUE: return "TRUE";
		case CPOOL_VALUE_FALSE: return "FALSE";
		case CPOOL_VALUE_FUNC: return "FUNC";
	}
	return "N/A";
}

const char * opcode_name(opcode_t op) 
{
	static const char * optable[] = {
		"RET0", "HALT", "NOP", "RET", "CALL", "LOAD", "LOADS", "LOADAT",
		"LOADK", "LOADG", "LOADI", "LOADU", "MOVE", "STORE", "STOREAT",
		"STOREG", "STOREU", "JUMP", "JUMPF", "SWITCH", "ADD", "SUB", "DIV",
		"MUL", "REM", "AND", "OR", "LT", "GT", "EQ", "LEQ", "GEQ", "NEQ",
		"EQQ", "NEQQ", "IS", "MATCH", "NEG", "NOT", "LSHIFT", "RSHIFT", "BAND",
		"BOR", "BXOR", "BNOT", "MAPNEW", "LISTNEW", "RANGENEW", "SETLIST",
		"CLOSURE", "CLOSE", "CHECK", "RESERVED2", "RESERVED3", "RESERVED4",
		"RESERVED5", "RESERVED6"
	};
	return optable[op];
}

#define DUMP_VM(buffer, bindex, ...)                    bindex += snprintf(&buffer[bindex], balloc-bindex, "%06u\t", pc);    \
	bindex += snprintf(&buffer[bindex], balloc-bindex, __VA_ARGS__);        \
	bindex += snprintf(&buffer[bindex], balloc-bindex, "\n");


static void DumpVm(char * pBuffer, uint32 bufferSize, uint32 * pBufferIndex, uint32 pc, const char * pFmt, ...)
{
//#define DUMP_VM(buffer, bindex, ...)                  
	va_list arg;
	va_start(arg, pFmt);
	uint32 bindex = *pBufferIndex;
	bindex += snprintf(&pBuffer[bindex], bufferSize-bindex, "%06u\t", pc);
	bindex += vsnprintf(&pBuffer[bindex], bufferSize-bindex, pFmt, arg);
	bindex += snprintf(&pBuffer[bindex], bufferSize-bindex, "\n");
	*pBufferIndex = bindex;
	va_end(arg);
}

//#define DUMP_VM_NOCR(buffer, bindex, ...)               bindex += snprintf(&buffer[bindex], balloc-bindex, "%06u\t", pc);    \
	//bindex += snprintf(&buffer[bindex], balloc-bindex, __VA_ARGS__);

#define DUMP_VM_RAW(buffer, bindex, ...)                bindex += snprintf(&buffer[bindex], balloc-bindex, __VA_ARGS__);

const char * gravity_disassemble(gravity_vm * vm, gravity_function_t * f, const char * bcode, uint32 blen, bool deserialize) 
{
	uint32 * ip = NULL;
	uint32 pc = 0, inst = 0, ninsts = 0;
	opcode_t op;
	const int rowlen = 256;
	uint32 bindex = 0;
	uint32 balloc = 0;
	char * buffer = NULL;
	if(deserialize) {
		// decode textual buffer to real bytecode
		ip = gravity_bytecode_deserialize(bcode, blen, &ninsts);
		if(!ip || !ninsts) 
			goto abort_disassemble;
	}
	else {
		ip = (uint32 *)(bcode);
		ninsts = blen;
	}
	// allocate a buffer big enought to fit all disassembled bytecode
	// I assume that each instruction (each row) will be 256 chars long
	balloc = ninsts * rowlen;
	buffer = static_cast<char *>(mem_alloc(NULL, balloc));
	if(!buffer) 
		goto abort_disassemble;
	// conversion loop
	while(pc < ninsts) {
		inst = *ip++;
		op = (opcode_t)OPCODE_GET_OPCODE(inst);
		// for fixed N spaces in opcode name
		// replace %s with %-Ns
		switch(op) {
			case NOP: 
				{
					DUMP_VM(buffer, bindex, "%s", opcode_name(op));
				}
				break;
			case MOVE:
			case LOADG:
			case LOADU:
			case STOREG:
			case STOREU:
			case JUMPF:
			case MAPNEW:
			case LISTNEW:
			case CLOSURE: 
				{
					OPCODE_GET_ONE8bit_ONE18bit(inst, const uint32 r1, const uint32 r2);
					DUMP_VM(buffer, bindex, "%s %d %d", opcode_name(op), r1, r2);
				}
				break;
			case LOADI: 
				{
					OPCODE_GET_ONE8bit_SIGN_ONE17bit(inst, const uint32 r1, const int32 value);
					DUMP_VM(buffer, bindex, "%s %d %d", opcode_name(op), r1, value);
				}
				break;
			case LOADK: {
			    OPCODE_GET_ONE8bit_ONE18bit(inst, const uint32 r1, const uint32 index);
			    if(index < CPOOL_INDEX_MAX) {
				    DUMP_VM(buffer, bindex, "%s %d %d", opcode_name(op), r1, index);
				    if(f) {
					    char b[2018];
					    GravityValue constant = gravity_function_cpool_get(f, static_cast<uint16>(index));
					    gravity_value_dump(vm, constant, b, sizeof(b));
					    --bindex; // to replace the \n character
					    DUMP_VM_RAW(buffer, bindex, "\t\t;%s\n", b);
				    }
			    }
			    else {
				    const char * special;
				    switch(index) {
					    case CPOOL_VALUE_SUPER: special = "SUPER"; break;
					    case CPOOL_VALUE_NULL: special = "NULL"; break;
					    case CPOOL_VALUE_UNDEFINED: special = "UNDEFINED"; break;
					    case CPOOL_VALUE_ARGUMENTS: special = "ARGUMENTS"; break;
					    case CPOOL_VALUE_TRUE: special = "TRUE"; break;
					    case CPOOL_VALUE_FALSE: special = "FALSE"; break;
					    case CPOOL_VALUE_FUNC: special = "_FUNC"; break;
					    default: special = "Invalid index in LOADK opcode"; break;
				    }
				    DUMP_VM(buffer, bindex, "%s %d %s", opcode_name(op), r1, special);
			    }
			    break;
		    }
			case LOAD:
			case LOADS:
			case LOADAT:
			case STORE:
			case STOREAT:
			case EQQ:
			case NEQQ:
			case ISA:
			case MATCH:
			case LT:
			case GT:
			case EQ:
			case LEQ:
			case GEQ:
			case NEQ:
			case LSHIFT:
			case RSHIFT:
			case BAND:
			case BOR:
			case BXOR:
			case ADD:
			case SUB:
			case DIV:
			case MUL:
			case REM:
			case AND:
			case OR: 
				{
					OPCODE_GET_TWO8bit_ONE10bit(inst, const uint32 r1, const uint32 r2, const uint32 r3);
					DUMP_VM(buffer, bindex, "%s %d %d %d", opcode_name(op), r1, r2, r3);
				}
				break;
			// unary operators
			case BNOT:
			case NEG:
			case NOT: 
				{
					OPCODE_GET_TWO8bit_ONE10bit(inst, const uint32 r1, const uint32 r2, const uint32 r3);
					DUMP_VM(buffer, bindex, "%s %d %d", opcode_name(op), r1, r2);
				}
				break;
			case RANGENEW: 
				{
					OPCODE_GET_TWO8bit_ONE10bit(inst, const uint32 r1, const uint32 r2, const uint32 r3);
					DUMP_VM(buffer, bindex, "%s %d %d %d", opcode_name(op), r1, r2, r3);
				}
				break;
			case JUMP: 
				{
					OPCODE_GET_ONE26bit(inst, const uint32 value);
					DUMP_VM(buffer, bindex, "%s %d", opcode_name(op), value);
				}
				break;
			case CALL: 
				{
					// CALL A B C => R(A) = B(C0... CN)
					OPCODE_GET_THREE8bit(inst, const uint32 r1, const uint32 r2, uint32 r3);
					DUMP_VM(buffer, bindex, "%s %d %d %d", opcode_name(op), r1, r2, r3);
				}
				break;
			case RET0:
			case RET: 
				{
					if(op == RET0) {
						DUMP_VM(buffer, bindex, "%s", opcode_name(op));
					}
					else {
						OPCODE_GET_ONE8bit(inst, const uint32 r1);
						DUMP_VM(buffer, bindex, "%s %d", opcode_name(op), r1);
					}
				}
				break;
			case HALT: 
				{
					DUMP_VM(buffer, bindex, "%s", opcode_name(op));
				}
				break;
			case SWITCH: 
				{
					DUMP_VM(buffer, bindex, "SWITCH instruction not yet implemented");
				}
				break;
			case SETLIST: 
				{
					OPCODE_GET_TWO8bit_ONE10bit(inst, const uint32 r1, uint32 r2, const uint32 r3);
					DUMP_VM(buffer, bindex, "%s %d %d", opcode_name(op), r1, r2);
				}
				break;
			case CLOSE: 
				{
					OPCODE_GET_ONE8bit_ONE18bit(inst, const uint32 r1, const uint32 r2);
					DUMP_VM(buffer, bindex, "%s %d", opcode_name(op), r1);
				}
				break;
			case CHECK: 
				{
					OPCODE_GET_ONE8bit_ONE18bit(inst, const uint32 r1, const uint32 r2);
					DUMP_VM(buffer, bindex, "%s %d", opcode_name(op), r1);
				}
				break;
			case RESERVED2:
			case RESERVED3:
			case RESERVED4:
			case RESERVED5:
			case RESERVED6: 
				{
					DUMP_VM(buffer, bindex, "RESERVED");
				}
				break;
		}
		++pc;
	}
	return buffer;
abort_disassemble:
	if(deserialize) 
		mem_free(ip);
	mem_free(buffer);
	return NULL;
}
