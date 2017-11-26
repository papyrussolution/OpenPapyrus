/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010 Ger Hobbelt All Rights Reserved.
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

using namespace xlslib_core;

/*
 *********************************
 * boolean_t class implementation
 *********************************
 */
boolean_t::boolean_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, bool value, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval)
{
	num = value;
}

CUnit* boolean_t::GetData(CDataStorage &datastore) const
{
	return datastore.MakeCBoolean(*this);   // NOTE: this pointer HAS to be deleted elsewhere.
}

/*
 *********************************
 * CBoolean class implementation
 *********************************
 */
CBoolean::CBoolean(CDataStorage &datastore, const boolean_t& booldef) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_BOOLERR);
	AddValue16((uint16)booldef.GetRow());
	AddValue16((uint16)booldef.GetCol());
	AddValue16((uint16)booldef.GetXFIndex());
	AddValue8((uint8)booldef.GetBoolean());
	AddValue8(0);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CBoolean::~CBoolean()
{
}
