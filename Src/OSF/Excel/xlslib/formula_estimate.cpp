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

estimated_formula_result_t::estimated_formula_result_t(CGlobalRecords& gRecords) :
	value_type(ESTVAL_UNKNOWN),
	always_calc(0),
	calc_on_load(0),
	m_GlobalRecords(gRecords)
{
/*
 *   union
 *   {
 *       bool b;
 *       int32 i;
 *       double f;
 *       u16string *s;
 *       errcode_t e;
 *   } value;
 *   enum
 *   {
 *       ESTVAL_UNKNOWN = 0,
 *       ESTVAL_BOOLEAN,
 *       ESTVAL_INTEGER,
 *       ESTVAL_FLOATINGPOINT,
 *       ESTVAL_STRING,
 *       ESTVAL_ERRORCODE,
 *   } value_type;
 *
 *   bool always_calc : 1;
 *   bool calc_on_load: 1;
 */
	memset(&value, 0, sizeof(value));
}

estimated_formula_result_t::~estimated_formula_result_t()
{
	clear_value(ESTVAL_UNKNOWN);
}

estimated_formula_result_t& estimated_formula_result_t::operator =(const estimated_formula_result_t &src)
{
	(void)src; // stop warning
	throw std::string("Should never have invoked the estimated_formula_result_t copy operator!");
}

void estimated_formula_result_t::clear_value(estval_type_t incoming_type)
{
	if (value_type == ESTVAL_STRING && incoming_type != ESTVAL_STRING) {
		delete value.s;
	} else
	if (value_type != ESTVAL_STRING && incoming_type == ESTVAL_STRING) {
		value.s = new u16string;
	}
	value_type = incoming_type;
}

void estimated_formula_result_t::SetCalcOnLoad(void)
{
	calc_on_load = 1;
}

void estimated_formula_result_t::SetCalcAlways(void)
{
	always_calc = 1;
}

uint16 estimated_formula_result_t::GetOptionFlags(void) const
{
	return (uint16)(calc_on_load << 1 | always_calc);
}

bool estimated_formula_result_t::SetBoolean(bool v)
{
	clear_value(ESTVAL_BOOLEAN);
	return value.b = v;
}

int32 estimated_formula_result_t::SetInteger(int32 v)
{
	clear_value(ESTVAL_INTEGER);
	return value.i = v;
}

double estimated_formula_result_t::SetFloatingPoint(double v)
{
	clear_value(ESTVAL_FLOATINGPOINT);
	return value.f = v;
}

const u16string& estimated_formula_result_t::SetText(const std::string& v)
{
	clear_value(ESTVAL_STRING);
	m_GlobalRecords.char2str16(v, *value.s);
	return *value.s;
}

const u16string& estimated_formula_result_t::SetText(const ustring& v)
{
	clear_value(ESTVAL_STRING);
	m_GlobalRecords.wide2str16(v, *value.s);
	return *value.s;
}

#ifndef __FRAMEWORK__
const u16string& estimated_formula_result_t::SetText(const u16string& v)
{
	clear_value(ESTVAL_STRING);
	return *value.s = v;
}

#endif
errcode_t estimated_formula_result_t::SetErrorCode(errcode_t v)
{
	clear_value(ESTVAL_ERRORCODE);
	return value.e = v;
}

uint64 estimated_formula_result_t::GetEncodedValue(void) const
{
	uint64 rv;

	switch (value_type)	{
	default:
	case ESTVAL_UNKNOWN:
		XL_ASSERTS("Should never get here!");

	case ESTVAL_BOOLEAN:
		rv = 0xFFFF000000000001ULL;
		rv |= ((uint32)value.b) << 16;
		break;

	case ESTVAL_INTEGER:
		rv = CUnit::EncodeFP2I64(value.i);
		break;

	case ESTVAL_FLOATINGPOINT:
		rv = CUnit::EncodeFP2I64(value.f);
		break;

	case ESTVAL_STRING:
		rv = 0xFFFF000000000000ULL;
		break;

	case ESTVAL_ERRORCODE:
		rv = 0xFFFF000000000002ULL;
		rv |= ((uint32)value.e) << 16;
		break;
	}
	return rv;
}

bool estimated_formula_result_t::EncodedValueIsString(void) const
{
	return value_type == ESTVAL_STRING;
}

const u16string* estimated_formula_result_t::GetStringValue(void) const
{
	if (value_type == ESTVAL_STRING) {
		return value.s;
	}
	return NULL;
}

