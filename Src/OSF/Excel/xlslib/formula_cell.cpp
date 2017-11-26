/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010-2013 Ger Hobbelt All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "xlslib-internal.h"
#pragma hdrstop

#ifdef __BCPLUSPLUS__
#include <malloc.h>
// malloc.h needed for calloc. RLN 111208
#include <memory.h>
// memory.h needed for memset. RLN 111215
// These may be needed for other compilers as well.
#endif

using namespace xlslib_core;
using namespace xlslib_strings;

/* For information on the XLS binary format, see
 * http://msdn.microsoft.com/en-us/library/cc313154(v=office.12).aspx
 *
 * The section on formulas is entitled "Microsoft Excel Formulas"
 */

formula_cell_t::formula_cell_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, expression_node_t* ast_val, bool autodes, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
	ast(ast_val),
	auto_destruct_expression_tree(autodes),
    stack(NULL)
{
	XL_ASSERT(ast_val);

#if !defined (HAVE_PRAGMA_PACK) && !defined (HAVE_PRAGMA_PACK_PUSH_POP)
	XL_ASSERTS("Must Have Pragma Pack to use formulas");
#endif
}

formula_cell_t::formula_cell_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, 
        formula_t *stack_val, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
    ast(NULL),
	stack(stack_val)
{
	XL_ASSERT(stack_val);

#if !defined (HAVE_PRAGMA_PACK) && !defined (HAVE_PRAGMA_PACK_PUSH_POP)
	XL_ASSERTS("Must Have Pragma Pack to use formulas");
#endif
}

formula_cell_t::~formula_cell_t()
{
	if (ast && auto_destruct_expression_tree) {
		ast->DestroyAST();
	}
}

void formula_cell_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	if (ast) {
		ast->GetResultEstimate(dst);
	} else if (stack) {
        stack->GetResultEstimate(dst);
    }
}

size_t formula_cell_t::GetSize(void) const
{
	estimated_formula_result_t estimate(m_GlobalRecords);
	const expression_node_t *expr = GetAST();
    XL_ASSERT(expr != NULL || stack != NULL);
	size_t len = 4+2+2+2+8+2+4+2;
    if (expr) {
        len += expr->GetSize();
    } else if (stack) {
        len += stack->GetSize();
    }
    GetResultEstimate(estimate);

	if (estimate.EncodedValueIsString()) {
		// FORMULA BIFF8 is immediately followed by a STRING BIFF8 record!
		const u16string* str = estimate.GetStringValue();

		XL_ASSERT(str);
		len += 4 + str->length() * (CGlobalRecords::IsASCII(*str) ? sizeof(uint8) : sizeof(uint16));
	}

	return len;
}

CUnit* formula_cell_t::GetData(CDataStorage &datastore) const
{
	return datastore.MakeCFormula(*this);   // NOTE: this pointer HAS to be deleted elsewhere.
}

void formula_cell_t::DumpData(CUnit &dst) const
{
    if (ast) {
        formula_t *fs = new formula_t(m_GlobalRecords, NULL);
        ast->DumpData(*fs, true); // rgce dump, length_of_parsed_expr
        fs->DumpData(dst);
        delete fs;
    } else if (stack) {
        stack->DumpData(dst);
    }
}

/*
 *********************************
 *  CFormula class implementation
 *********************************
 */

CFormula::CFormula(CDataStorage &datastore, const formula_cell_t& expr) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_FORMULA);  // followed by the RECTYPE_STRING record when the formula evaluates to a string!
	AddValue16((uint16)expr.GetRow());
	AddValue16((uint16)expr.GetCol());
	AddValue16(expr.GetXFIndex());

	estimated_formula_result_t estimate(expr.GetGlobalRecords());

	expr.GetResultEstimate(estimate);
	AddValue64(estimate.GetEncodedValue()); // current_value_of_formula
	AddValue16(estimate.GetOptionFlags()); // flags: grbit
	AddValue32(0); // chn

	size_t len_position = GetDataSize();
	AddValue16(0 /* expr.GetSize() */ ); // length_of_parsed_expr

	expr.DumpData(*this); 

	size_t end = GetDataSize();
	uint32 len_position32 = (uint32)len_position;
	SetValueAt16((uint16)(end - len_position32 - 2), len_position32);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);

	if (estimate.EncodedValueIsString()) {
		// FORMULA BIFF8 is immediately followed by a STRING BIFF8 record!
		//
		// fake it by appending it at the tail of the current record!
		size_t basepos = GetDataSize();

		AddValue16(RECTYPE_STRING);
		AddValue16(0);

		const u16string* str = estimate.GetStringValue();

		XL_ASSERT(str);
		XL_ASSERT(str->length() < 256); // dbg
		AddUnicodeString(*str, LEN2_FLAGS_UNICODE);

		SetValueAt16((uint16)(GetDataSize() - basepos - 4), (uint32)basepos + 2);
	}
}

CFormula::~CFormula()
{
}

/*
 *  BIFF 6 (06h)  41  72 01 02 00 0F 00 00 00  C8 7C 66 0C FF FF 00 00
 *  C0 00 00 FC 13 00 39 00  00 01 00 00 00 17 02 00
 *  46 38 1E 0A 00 42 03 FF  00
 *
 *  row=0172
 *  col = 0002
 *  ixfe = 000F
 *  num = FFFF0C667CC80000   --> string
 *  flags = 0000
 *  chn = FC00000C
 *  cce.len = 00013
 *  rgce = 39 00 00 01 00 00 00 17 02 00 46 38 1E 0A 00 42 03 FF 00
 *
 *  ptgNameX(39): ixti = 0000, ilbl = 0001, reserved = 0000
 *  ptgStr(17): cch.len = 02, flags = 0, str = 46 38 ("F8")
 *  ptgInt(1E): val = 000A (10)
 *  ptgFuncVarV(42): 00FF03 --> cargs = 3, prompt = 0, iftab = 00FF, CE = 0
 *
 *  BIFF STRING (207h)  13  0A 00 00 30 30 31 31 31  31 31 30 30 30
 *  BIFF 6 (06h)  38  73 01 02 00 0F 00 00 00  00 00 00 00 6F 40 00 00
 *  C0 00 00 FD 10 00 39 01  00 02 00 00 00 17 02 00
 *  46 38 42 02 FF 00
 *  rgce = 39 01 00 02 00 00 00 17 02 00 46 38 42 02 FF 00
 *
 *  ptgNameX(39): ixti = 0000, ilbl = 0001, reserved = 0000
 *  ptgStr(17): cch.len = 02, flags = 0, str = 46 38 ("F8")
 *  ptgFuncVarV(42): 00FF02 --> cargs = 2, prompt = 0, iftab = 00FF, CE = 0
 *
 *  BIFF 6 (06h)  45  74 01 02 00 0F 00 00 00  E0 D7 A6 04 FF FF 00 00
 *  72 01 02 FE 17 00 39 01  00 01 00 00 00 17 02 00
 *  46 38 19 40 00 01 1E 0A  00 42 03 FF 00
 *  BIFF STRING (207h)  13  0A 00 00 30 30 30 30 30  30 30 33 37 30
 *
 *
 *
 *
 *  BIFF 6 (06h)  37  6A 00 02 00 0F 00 F0 77  1B 7F CF EB BA 3F 00 00
 *  C0 00 00 FC 0F 00
 *  1E 01 00 1E 02 00 1E 03 00 1E 04 00 41 2E 01  012e  256+32+14  302
 */

