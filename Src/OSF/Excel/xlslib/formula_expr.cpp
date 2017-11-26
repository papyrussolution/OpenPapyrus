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

expression_node_t::expression_node_t(CGlobalRecords& gRecords)
{
	(void)gRecords;
}

expression_node_t::~expression_node_t()
{
}

expression_node_t* expression_node_t::GetChild(uint16 index) const
{
	(void)index;
	return NULL;
}

uint16 expression_node_t::GetNumberOfChilds(void) const
{
	return 0;
}

void expression_node_t::DestroyAST(void)
{
	// first destroy the children, bottom-up recursively.
	uint16 chn = GetNumberOfChilds();
	while (chn-- > 0) {
		expression_node_t* ch = GetChild(chn);

		XL_ASSERT(ch);
		if (ch)	{
			ch->DestroyAST();
		}
	}

	// then destroy this instance itself
	delete this;
}

void expression_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	(void)dst;
	XL_ASSERTS("Should never get here! MUST be handled by the derived class' method of similar name!");

	// do nothing: propagate the currently set expected value.
}

size_t expression_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	XL_ASSERTS("Should never get here! MUST be handled by the derived class' method of similar name!");
	return 0;
}

int8 expression_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)stack;
	(void)include_subtree;
	XL_ASSERTS("Should never get here! MUST be handled by the derived class' method of similar name!");
	return NO_ERRORS;
}

terminal_node_t::terminal_node_t(CGlobalRecords& gRecords) :
	expression_node_t(gRecords)
{
}

terminal_node_t::~terminal_node_t()
{
}

boolean_value_node_t::boolean_value_node_t(CGlobalRecords& gRecords, bool v) :
	terminal_node_t(gRecords),
	value(v)
{
}

boolean_value_node_t::~boolean_value_node_t()
{
}

void boolean_value_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetBoolean(value);
}

size_t boolean_value_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 2;
}

int8 boolean_value_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;

    return stack.PushBoolean(value);
}

integer_value_node_t::integer_value_node_t(CGlobalRecords& gRecords, int32 v) :
	terminal_node_t(gRecords),
	value(v)
{
}

integer_value_node_t::~integer_value_node_t()
{
}

void integer_value_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetInteger(value);
}

size_t integer_value_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	if (value >= 0 && value <= 65535) {
		return 2+1; // ptgInt
	}
	return 8+1; // ptgNum
}

int8 integer_value_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushInteger(value);
}

float_value_node_t::float_value_node_t(CGlobalRecords& gRecords, double v) :
	terminal_node_t(gRecords),
	value(v)
{
}

float_value_node_t::~float_value_node_t()
{
}

void float_value_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetFloatingPoint(value);
}

size_t float_value_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 8+1;
}

int8 float_value_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushFloatingPoint(value);
}

error_value_node_t::error_value_node_t(CGlobalRecords& gRecords, errcode_t v) :
	terminal_node_t(gRecords),
	value(v)
{
}

error_value_node_t::~error_value_node_t()
{
}

void error_value_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetErrorCode(value);
}

size_t error_value_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 1+1;
}

int8 error_value_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushError(value);
}

missing_arg_node_t::missing_arg_node_t(CGlobalRecords& gRecords) :
	terminal_node_t(gRecords)
{
}

missing_arg_node_t::~missing_arg_node_t()
{
}

void missing_arg_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetErrorCode(XLERR_N_A);
}

size_t missing_arg_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 1;
}

int8 missing_arg_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushMissingArgument();
}

text_value_node_t::text_value_node_t(CGlobalRecords& gRecords, const std::string& v) :
	terminal_node_t(gRecords),
	m_GlobalRecords(gRecords)
{
	m_GlobalRecords.char2str16(v, this->value);
}

text_value_node_t::text_value_node_t(CGlobalRecords& gRecords, const ustring& v) :
	terminal_node_t(gRecords),
	m_GlobalRecords(gRecords)
{
	m_GlobalRecords.wide2str16(v, this->value);
}

#ifndef __FRAMEWORK__
text_value_node_t::text_value_node_t(CGlobalRecords& gRecords, const u16string& v) :
	terminal_node_t(gRecords),
	value(v),
	m_GlobalRecords(gRecords)
{
}

#endif
text_value_node_t::~text_value_node_t()
{
}

text_value_node_t& text_value_node_t::operator =(const text_value_node_t &src)
{
	(void)src;
	throw std::string("Should never have invoked the text_value_node_t copy operator!");
}

void text_value_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	dst.SetText(value);
}

size_t text_value_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 1 + 1 + CGlobalRecords::IsASCII(value) * value.length() + value.length();
}

int8 text_value_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushText(value);
}

cell_deref_node_t::cell_deref_node_t(CGlobalRecords& gRecords, const cell_t& v, cell_addr_mode_t opt, cell_op_class_t opclass) :
	terminal_node_t(gRecords),
	row_(v.GetRow()),
	col_(v.GetCol()),
	idx_(v.GetWorksheet() ? v.GetWorksheet()->GetIndex() : invalidIndex),
	worksheet_ref(NULL),
	attr(opt),
	operand_class(opclass)
{
}

cell_deref_node_t::cell_deref_node_t(CGlobalRecords& gRecords, const cell_t& v, const worksheet* ws, cell_addr_mode_t opt, cell_op_class_t opclass) :
	terminal_node_t(gRecords),
	row_(v.GetRow()),
	col_(v.GetCol()),
	idx_(v.GetWorksheet() ? v.GetWorksheet()->GetIndex() : invalidIndex),
	worksheet_ref(ws),
	attr(opt),
	operand_class(opclass)
{
}

cell_deref_node_t::~cell_deref_node_t()
{
}

void cell_deref_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	/*
	 *  We don't want to spend the effort to 'know' (~ make this code aware) which cell reference produces what result, neither in value nor in type, so we
	 *  fake it and make it easy to ourselves: we 'guestimate' the cell/reference will return an
	 *  error code and we mark the expression as 'calc on load' to mask our ignorance.
	 */
	dst.SetCalcOnLoad();
	dst.SetErrorCode(XLERR_VALUE);
}

size_t cell_deref_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 1+2+2;
}

int8 cell_deref_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree;
    return stack.PushReference(row_, col_, idx_, attr);
}

cellarea_deref_node_t::cellarea_deref_node_t(CGlobalRecords& gRecords, const cell_t& u_l_c, const cell_t& l_r_c, cell_addr_mode_t attr,
											 cell_op_class_t opclass) :
	cell_deref_node_t(gRecords, u_l_c, attr, opclass),
	lrrow_(l_r_c.GetRow()),
	lrcol_(l_r_c.GetCol()),
	lridx_(l_r_c.GetWorksheet() ? l_r_c.GetWorksheet()->GetIndex() : invalidIndex)
{
}

cellarea_deref_node_t::cellarea_deref_node_t(CGlobalRecords& gRecords, const cell_t& u_l_c, const cell_t& l_r_c, const worksheet* ws, cell_addr_mode_t attr,
											 cell_op_class_t opclass) :
	cell_deref_node_t(gRecords, u_l_c, ws, attr, opclass),
	lrrow_(l_r_c.GetRow()),
	lrcol_(l_r_c.GetCol()),
	lridx_(l_r_c.GetWorksheet() ? l_r_c.GetWorksheet()->GetIndex() : invalidIndex)
{
}

cellarea_deref_node_t::~cellarea_deref_node_t()
{
}

size_t cellarea_deref_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;

	return 1+2*(2+2);
}

int8 cellarea_deref_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)include_subtree; // stop warning
    return stack.PushAreaReference(row_, col_, idx_, lrrow_, lrcol_, lridx_, attr);
}

void cellarea_deref_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	/*
	 *  We don't want to spend the effort to 'know' (~ make this code aware) which cell reference produces what result, neither in value nor in type, so we
	 *  fake it and make it easy to ourselves: we 'guestimate' the cell/reference will return an
	 *  error code and we mark the expression as 'calc on load' to mask our ignorance.
	 */
	dst.SetCalcOnLoad();
	dst.SetErrorCode(XLERR_VALUE);
}

operator_basenode_t::operator_basenode_t(CGlobalRecords& gRecords, expr_operator_code_t o) :
	expression_node_t(gRecords),
	op(o)
{
}

operator_basenode_t::~operator_basenode_t()
{
}

void operator_basenode_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	/*
	 *  We don't want to spend the effort to 'know' (~ make this code aware) which operation produces what result, neither in value nor in type, so we
	 *  fake it and make it easy to ourselves: we 'guestimate' the operation will return an
	 *  error code and we mark the expression as 'calc on load' to mask our ignorance.
	 *
	 *  Remark: there's just a few operations we care about: those we are willing to take to the dance.
	 */
	dst.SetCalcOnLoad();
	dst.SetErrorCode(XLERR_VALUE);
}

unary_op_node_t::unary_op_node_t(CGlobalRecords& gRecords, expr_operator_code_t op, expression_node_t* a) :
	operator_basenode_t(gRecords, op),
	arg(a)
{
	XL_ASSERT(a);

	switch (op)	{
	default:
		throw std::string("Not a valid unary operator");

	case OP_UPLUS: // Unary Plus
	case OP_UMINUS: // Unary Minus
	case OP_PERCENT: // Percent Sign
	case OP_PAREN:  // Enclose into parentheses
		// okay
		break;
	}
}

unary_op_node_t::~unary_op_node_t()
{
}

expression_node_t* unary_op_node_t::GetChild(uint16 index) const
{
	if (index == 0) {
		return arg;
	}
	return NULL;
}

uint16 unary_op_node_t::GetNumberOfChilds(void) const
{
	return 1;
}

size_t unary_op_node_t::GetSize(bool include_subtree) const
{
	size_t len = 1;

	if (include_subtree) {
		XL_ASSERT(GetChild(0));
		len += GetChild(0)->GetSize(include_subtree);
	}
	return len;
}

int8 unary_op_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	int8 errcode = NO_ERRORS;

	if (include_subtree) {
		XL_ASSERT(GetChild(0));
		errcode |= GetChild(0)->DumpData(stack, include_subtree);
	}

	errcode |= stack.PushOperator(op);

#ifdef XL_WITH_ASSERTIONS
	switch (op)	{
	default:
		XL_ASSERTS("Should never get here!");

	case OP_UPLUS: // Unary Plus
	case OP_UMINUS: // Unary Minus
	case OP_PERCENT: // Percent Sign
	case OP_PAREN: // Enclose into parentheses
		// okay
		break;
	}
#endif

	return errcode;
}

binary_op_node_t::binary_op_node_t(CGlobalRecords& gRecords, expr_operator_code_t op, expression_node_t* arg1, expression_node_t* arg2) :
	operator_basenode_t(gRecords, op)
{
	args[0] = arg1;
	args[1] = arg2;

	switch (op)	{
	default:
		throw std::string("Not a valid binary operator");

	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_POWER:
	case OP_CONCAT:
	case OP_LE:
	case OP_LT:
	case OP_EQ:
	case OP_GE:
	case OP_GT:
	case OP_NE:
	case OP_ISECT:  // intersection ~ Excel 'space' operator
	case OP_UNION:  // union ~ Excel 'comma' operator
	case OP_RANGE:  // minimal bounding rectangle ~ Excel 'colon' operator
		// okay
		break;
	}
}

binary_op_node_t::~binary_op_node_t()
{
}

expression_node_t* binary_op_node_t::GetChild(uint16 index) const
{
	if (index <= 1) {
		return args[index];
	}
	return NULL;
}

uint16 binary_op_node_t::GetNumberOfChilds(void) const
{
	return 2;
}

size_t binary_op_node_t::GetSize(bool include_subtree) const
{
	size_t len = 1;

	if (include_subtree) {
		XL_ASSERT(GetChild(0));
		len += GetChild(0)->GetSize(include_subtree);
		XL_ASSERT(GetChild(1));
		len += GetChild(1)->GetSize(include_subtree);
	}
	return len;
}

int8 binary_op_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	int8 errcode = NO_ERRORS;

	if (include_subtree) {
		XL_ASSERT(GetChild(0));
		errcode |= GetChild(0)->DumpData(stack, include_subtree);
		XL_ASSERT(GetChild(1));
		errcode |= GetChild(1)->DumpData(stack, include_subtree);
	}

	errcode |= stack.PushOperator(op);

#ifdef XL_WITH_ASSERTIONS
	switch (op)	{
	default:
		XL_ASSERTS("Not a valid binary operator");

	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_POWER:
	case OP_CONCAT:
	case OP_LE:
	case OP_LT:
	case OP_EQ:
	case OP_GE:
	case OP_GT:
	case OP_NE:
	case OP_ISECT:  // intersection ~ Excel 'space' operator
	case OP_UNION:  // union ~ Excel 'comma' operator
	case OP_RANGE:  // minimal bounding rectangle ~ Excel 'colon' operator
		// okay
		break;
	}
#endif

	return errcode;
}

function_basenode_t::function_basenode_t(CGlobalRecords& gRecords, expr_function_code_t f) :
	expression_node_t(gRecords),
	func(f)
{
}

function_basenode_t::~function_basenode_t()
{
}

void function_basenode_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	/*
	 *  We don't want to spend the effort to 'know' (~ make this code aware) which function produces what result, neither in value nor in type, so we
	 *  fake it and make it easy to ourselves: we 'guestimate' the UDF will return an
	 *  error code and we mark the expression as 'calc on load' to mask our ignorance.
	 *
	 *  Remark: there's just a few functions we care about: RANDOM() and a few others, which we are willing to take to the dance.
	 */
	switch (func) {
	default:
		dst.SetCalcOnLoad();
		dst.SetErrorCode(XLERR_VALUE);
		break;

	case FUNC_UDF:
		XL_ASSERTS("Should've been handled by the udf class!");

	case FUNC_IF:
	case FUNC_ISNA:
	case FUNC_ISERROR:
	case FUNC_TRUE:
	case FUNC_FALSE:
	case FUNC_AND:
	case FUNC_OR:
	case FUNC_NOT:
	case FUNC_ISREF:
	case FUNC_ISERR:
	case FUNC_ISTEXT:
	case FUNC_ISNUMBER:
	case FUNC_ISBLANK:
	case FUNC_T:
	case FUNC_N:
	case FUNC_ISNONTEXT:
	case FUNC_ISLOGICAL:
	case FUNC_ISPMT:
	case FUNC_ISTHAIDIGIT:
		dst.SetCalcOnLoad();
		dst.SetBoolean(false);     // faked value estimate!
		break;

	case FUNC_COUNT:
	case FUNC_ROW:
	case FUNC_COLUMN:
	case FUNC_DCOUNT:
	case FUNC_DAY:
	case FUNC_MONTH:
	case FUNC_YEAR:
	case FUNC_WEEKDAY:
	case FUNC_HOUR:
	case FUNC_MINUTE:
	case FUNC_SECOND:
	case FUNC_COUNTA:
	case FUNC_DCOUNTA:
	case FUNC_COUNTIF:
	case FUNC_COUNTBLANK:
	case FUNC_WEEKNUM:
	case FUNC_COUNTIFS:
		dst.SetCalcOnLoad();
		dst.SetInteger(42);     // faked value estimate!
		break;

	case FUNC_PI:
		dst.SetCalcOnLoad();
		dst.SetFloatingPoint(3.1415);     // faked value estimate!
		break;

	case FUNC_SUM:
	case FUNC_AVERAGE:
	case FUNC_MIN:
	case FUNC_MAX:
	case FUNC_STDEV:
	case FUNC_SIN:
	case FUNC_COS:
	case FUNC_TAN:
	case FUNC_ATAN:
	case FUNC_SQRT:
	case FUNC_EXP:
	case FUNC_LN:
	case FUNC_LOG10:
	case FUNC_ABS:
	case FUNC_ROUND:
	case FUNC_MOD:
	case FUNC_DSUM:
	case FUNC_DAVERAGE:
	case FUNC_DMIN:
	case FUNC_DMAX:
	case FUNC_DSTDEV:
	case FUNC_VAR:
	case FUNC_DVAR:
	case FUNC_DATE:
	case FUNC_TIME:
	case FUNC_NOW:
	case FUNC_OFFSET:
	case FUNC_ATAN2:
	case FUNC_ASIN:
	case FUNC_ACOS:
	case FUNC_LOG:
	case FUNC_PRODUCT:
	case FUNC_FACT:
	case FUNC_STDEVP:
	case FUNC_VARP:
	case FUNC_DSTDEVP:
	case FUNC_ROUNDUP:
	case FUNC_ROUNDDOWN:
	case FUNC_DAYS360:
	case FUNC_TODAY:
	case FUNC_MEDIAN:
	case FUNC_SUMPRODUCT:
	case FUNC_SINH:
	case FUNC_COSH:
	case FUNC_TANH:
	case FUNC_ASINH:
	case FUNC_ACOSH:
	case FUNC_ATANH:
	case FUNC_FLOOR:
	case FUNC_KURT:
	case FUNC_SKEW:
	case FUNC_POWER:
	case FUNC_RADIANS:
	case FUNC_DEGREES:
	case FUNC_SUMIF:
	case FUNC_AVERAGEA:
	case FUNC_MAXA:
	case FUNC_MINA:
	case FUNC_STDEVPA:
	case FUNC_VARPA:
	case FUNC_STDEVA:
	case FUNC_VARA:
	case FUNC_SERIESSUM:
	case FUNC_ERF:
	case FUNC_ERFC:
	case FUNC_WORKDAY:
	case FUNC_NETWORKDAYS:
	case FUNC_GCD:
	case FUNC_SUMIFS:
	case FUNC_AVERAGEIF:
	case FUNC_AVERAGEIFS:
	case FUNC_ERF_PRECISE:
	case FUNC_ERFC_PRECISE:
	case FUNC_GAMMALN_PRECISE:
	case FUNC_CEILING_PRECISE:
	case FUNC_FLOOR_PRECISE:
		dst.SetCalcOnLoad();
		dst.SetFloatingPoint(42.0);     // faked value estimate!
		break;

	case FUNC_LOWER:
	case FUNC_UPPER:
	case FUNC_LEFT:
	case FUNC_RIGHT:
	case FUNC_TRIM:
	case FUNC_REPLACE:
	case FUNC_CONCATENATE:
	case FUNC_DATESTRING:
	case FUNC_NUMBERSTRING:
	case FUNC_ROMAN:
	case FUNC_HYPERLINK:
	case FUNC_PHONETIC:
	case FUNC_BAHTTEXT:
	case FUNC_HEX2BIN:
	case FUNC_HEX2DEC:
	case FUNC_HEX2OCT:
	case FUNC_DEC2BIN:
	case FUNC_DEC2HEX:
	case FUNC_DEC2OCT:
	case FUNC_OCT2BIN:
	case FUNC_OCT2HEX:
	case FUNC_OCT2DEC:
	case FUNC_BIN2DEC:
	case FUNC_BIN2OCT:
	case FUNC_BIN2HEX:
		dst.SetCalcOnLoad();
		dst.SetText("???");     // faked value estimate!
		break;

	case FUNC_RAND:
	case FUNC_VOLATILE:
	case FUNC_RANDBETWEEN:
		dst.SetCalcAlways();
		dst.SetCalcOnLoad();
		dst.SetFloatingPoint(0.5);     // faked random value estimate!
		break;
	}
}

size_t function_basenode_t::GetSize(bool include_subtree) const
{
	size_t len = 1+2; // OP_FUNC
	uint16 argcntmask = NumberOfArgsForExcelFunction(func);
	size_t chcnt = GetNumberOfChilds();

	// XL_ASSERT(argcntmask & (1U << (chcnt > 15 ? 15 : chcnt)));
	if (chcnt >= 15 || (argcntmask & ~(1U << chcnt))) {
		len += 1;
	}

	if (include_subtree) {
		while (chcnt-- > 0)	{
			XL_ASSERT(GetChild((uint16)chcnt));
			len += GetChild((uint16)chcnt)->GetSize(include_subtree);
		}
	}
	return len;
}

int8 function_basenode_t::DumpData(formula_t &stack, bool include_subtree) const
{
	int8 errcode = NO_ERRORS;
	uint16 argcntmask = NumberOfArgsForExcelFunction(func);
	size_t chcnt = GetNumberOfChilds();

	if (include_subtree) {
		size_t idx;

		for (idx = 0; idx < chcnt; idx++) {
			XL_ASSERT(GetChild((uint16)idx));
			errcode |= GetChild((uint16)idx)->DumpData(stack, include_subtree);
		}
	}

	// XL_ASSERT(argcntmask & (1U << (chcnt > 15 ? 15 : chcnt)));
	if (chcnt >= 15 || (argcntmask & ~(1U << chcnt))) {
        errcode |= stack.PushFunction(func, chcnt);
	} else {
        errcode |= stack.PushFunction(func);
	}

	return errcode;
}

z_ary_func_node_t::z_ary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t func) :
	function_basenode_t(gRecords, func)
{
}

z_ary_func_node_t::~z_ary_func_node_t()
{
}

unary_func_node_t::unary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op, expression_node_t* a) :
	function_basenode_t(gRecords, op),
	arg(a)
{
}

unary_func_node_t::~unary_func_node_t()
{
}

expression_node_t* unary_func_node_t::GetChild(uint16 index) const
{
	(void)index; // stop warning
	return arg;
}

uint16 unary_func_node_t::GetNumberOfChilds(void) const
{
	return 1;
}

binary_func_node_t::binary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op, expression_node_t* arg1, expression_node_t* arg2) :
	function_basenode_t(gRecords, op)
{
	args[0] = arg1;
	args[1] = arg2;
}

binary_func_node_t::~binary_func_node_t()
{
}

expression_node_t* binary_func_node_t::GetChild(uint16 index) const
{
	return args[index];
}

uint16 binary_func_node_t::GetNumberOfChilds(void) const
{
	return 2;
}

n_ary_func_node_t::n_ary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t func, size_t count, expression_node_t** arr) :
	function_basenode_t(gRecords, func),
	arg_arrsize((uint16)count),
	arg_count(0),
	arg_arr(NULL)
{
	if (count > 0) {
		//XL_ASSERT(arr);
		arg_arr = (expression_node_t **)calloc(count, sizeof(arg_arr[0]));
		//arg_arrsize = count;

		if (arr) {
			arg_count = (uint16)count;

			while (count-- > 0)	{
				arg_arr[count] = arr[count];
			}
		}
	}
}

n_ary_func_node_t::~n_ary_func_node_t()
{
	if (arg_arr) {
		free((void *)arg_arr);
	}
}

expression_node_t* n_ary_func_node_t::GetChild(uint16 index) const
{
	return arg_arr[index];
}

uint16 n_ary_func_node_t::GetNumberOfChilds(void) const
{
	return arg_count;
}

function_basenode_t& n_ary_func_node_t::PushArg(expression_node_t* arg)
{
	if (arg_arr == NULL) {
		arg_arrsize = 2;
		XL_ASSERT(arg_count == 0);
		arg_arr = (expression_node_t **)calloc(2, sizeof(arg_arr[0]));
	} else
	if (arg_count >= arg_arrsize) {
		while (arg_count >= arg_arrsize) {
			arg_arrsize += 2;
		}
		arg_arr = (expression_node_t **)realloc((void *)arg_arr, arg_arrsize * sizeof(arg_arr[0]));
		for (int i = arg_count; i < arg_arrsize; i++) {
			arg_arr[i] = NULL;
		}
	}

	arg_arr[arg_count++] = arg;

	return *this;
}

userdef_func_node_t::userdef_func_node_t(CGlobalRecords& gRecords, int udf_num, size_t count, expression_node_t** arr) :
	n_ary_func_node_t(gRecords, FUNC_UDF, count, arr),
	expr_user_function_code(udf_num)
{
}

userdef_func_node_t::~userdef_func_node_t()
{
}

void userdef_func_node_t::GetResultEstimate(estimated_formula_result_t &dst) const
{
	/*
	 *  We don't know what the heck a UDF will spit out as a result, not in value nor in type, so we
	 *  take a pod shot at this and make it easy to ourselves: we 'guestimate' the UDF will return an
	 *  error code and we mark the expression as 'calc on load' to mask our ignorance.
	 */
	dst.SetCalcOnLoad();
	dst.SetErrorCode(XLERR_VALUE);
}

size_t userdef_func_node_t::GetSize(bool include_subtree) const
{
	(void)include_subtree;
	return 0;
}

int8 userdef_func_node_t::DumpData(formula_t &stack, bool include_subtree) const
{
	(void)stack; // stop warnings
	(void)include_subtree;
	//int8 errcode = NO_ERRORS;

	/*
	 *  looks like the UDF is encoded like this:
	 *
	 *  first argument is a ptgNameX record pointing at the name of the UDF, then
	 *  followed by the arguments pushed onto the stack, and then at the
	 *  very end the ptgFuncVarV record with function id 0x00FF (UDF) and the total number
	 *  of arguments there INCLUDING that extra ptgNameX argument at the start.
	 *
	 *  In other words: UDF is a single function, which does indirection through the initial
	 *  argument to the actual user defined function, i.e. UDF(<referenced_user_function_name>, <arguments>...)
	 */

	return GENERAL_ERROR; // not supported yet...
}

expression_node_factory_t::expression_node_factory_t(CGlobalRecords& glbl) :
	m_GlobalRecords(glbl)
{
}

expression_node_factory_t::~expression_node_factory_t()
{
}

expression_node_factory_t &expression_node_factory_t::operator =(const expression_node_factory_t &src)
{
	(void)src; // stop warning
	throw std::string("Should never have invoked the expression_node_factory_t copy operator!");
}

boolean_value_node_t *expression_node_factory_t::boolean(bool value)
{
	return new boolean_value_node_t(m_GlobalRecords, value);
}

integer_value_node_t *expression_node_factory_t::integer(int32 value)
{
	return new integer_value_node_t(m_GlobalRecords, value);
}

float_value_node_t *expression_node_factory_t::floating_point(double value)
{
	return new float_value_node_t(m_GlobalRecords, value);
}

error_value_node_t *expression_node_factory_t::error_value(errcode_t value)
{
	return new error_value_node_t(m_GlobalRecords, value);
}

missing_arg_node_t *expression_node_factory_t::missing_arg(void)
{
	return new missing_arg_node_t(m_GlobalRecords);
}

text_value_node_t *expression_node_factory_t::text(const std::string& value)
{
	return new text_value_node_t(m_GlobalRecords, value);
}

text_value_node_t *expression_node_factory_t::text(const u16string& value)
{
	return new text_value_node_t(m_GlobalRecords, value);
}

cell_deref_node_t *expression_node_factory_t::cell(const cell_t& cellref, cell_addr_mode_t attr, cell_op_class_t opclass)
{
	return new cell_deref_node_t(m_GlobalRecords, cellref, attr, opclass);
}

cell_deref_node_t *expression_node_factory_t::cell(const cell_t& cellref, const worksheet* ws, cell_addr_mode_t attr, cell_op_class_t opclass)
{
	return new cell_deref_node_t(m_GlobalRecords, cellref, ws, attr, opclass);
}

cellarea_deref_node_t *expression_node_factory_t::area(const cell_t& upper_left_corner, const cell_t& lower_right_corner, cell_addr_mode_t attr,
							  cell_op_class_t opclass)
{
	return new cellarea_deref_node_t(m_GlobalRecords, upper_left_corner, lower_right_corner, attr, opclass);
}

cellarea_deref_node_t *expression_node_factory_t::area(const cell_t& upper_left_corner, const cell_t& lower_right_corner, const worksheet* ws,
							  cell_addr_mode_t attr, cell_op_class_t opclass)
{
	return new cellarea_deref_node_t(m_GlobalRecords, upper_left_corner, lower_right_corner, ws, attr, opclass);
}

unary_op_node_t *expression_node_factory_t::op(expr_operator_code_t op, expression_node_t* arg)
{
	return new unary_op_node_t(m_GlobalRecords, op, arg);
}

binary_op_node_t *expression_node_factory_t::op(expr_operator_code_t op, expression_node_t* arg1, expression_node_t* arg2)
{
	return new binary_op_node_t(m_GlobalRecords, op, arg1, arg2);
}

z_ary_func_node_t *expression_node_factory_t::f(expr_function_code_t func)
{
	return new z_ary_func_node_t(m_GlobalRecords, func);
}

unary_func_node_t *expression_node_factory_t::f(expr_function_code_t func, expression_node_t* arg)
{
	return new unary_func_node_t(m_GlobalRecords, func, arg);
}

binary_func_node_t *expression_node_factory_t::f(expr_function_code_t func, expression_node_t* arg1, expression_node_t* arg2)
{
	return new binary_func_node_t(m_GlobalRecords, func, arg1, arg2);
}

n_ary_func_node_t *expression_node_factory_t::f(expr_function_code_t func, size_t argcount, expression_node_t** arg_arr)
{
	return new n_ary_func_node_t(m_GlobalRecords, func, argcount, arg_arr);
}

userdef_func_node_t *expression_node_factory_t::udf(int expr_user_function, size_t argcount, expression_node_t** arg_arr)
{
	return new userdef_func_node_t(m_GlobalRecords, expr_user_function, argcount, arg_arr);
}

