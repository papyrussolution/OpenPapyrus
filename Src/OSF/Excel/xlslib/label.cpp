/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2013 David Hoerl All Rights Reserved.
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
using namespace xlslib_strings;
/*
 ******************************
 * label_t class implementation
 ******************************
 */

xlslib_core::label_t::label_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, const u16string& labelstrval, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
	strLabel(labelstrval),
	inSST(false)
{
	setType();
}

xlslib_core::label_t::label_t(CGlobalRecords& gRecords,
							  uint32 rowval, uint32 colval, const std::string& labelstrval, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
	strLabel(),
	inSST(false)
{
	gRecords.char2str16(labelstrval, strLabel);
	setType();
}

#ifndef __FRAMEWORK__
xlslib_core::label_t::label_t(CGlobalRecords& gRecords,
							  uint32 rowval, uint32 colval, const ustring& labelstrval, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
	strLabel(),
	inSST(false)
{
	gRecords.wide2str16(labelstrval, strLabel);
	setType();
}

#endif

void xlslib_core::label_t::setType()
{
	if(strLabel.length() > 255) {
		inSST = true;
		m_GlobalRecords.AddLabelSST(*this);
	}
}

xlslib_core::label_t::~label_t()
{
	// suppose someone creates a SST string, then overwrites it or overwrites cell with something else.
	// Unlikely, but this protects against that case.
	if(inSST) {
		m_GlobalRecords.DeleteLabelSST(*this);
	}
}

size_t xlslib_core::label_t::GetSize(void) const
{
	size_t size;
	if(inSST) {
		size = 4 + 8;   // =2 + 2 + 2 + 2 + 4
	} 
	else {
		size = 4 + 6;   // 4:type/size 2:row 2:col 2:xtnded
		// 2:size 1:flag len:(ascii or not)
		size += (sizeof(uint16) + 1 + strLabel.length() * (CGlobalRecords::IsASCII(strLabel) ? sizeof(uint8) : sizeof(uint16)));
	}
	return size;
}

CUnit* xlslib_core::label_t::GetData(CDataStorage &datastore) const
{
	return datastore.MakeCLabel(*this); // NOTE: this pointer HAS to be deleted elsewhere.
}

CLabel::CLabel(CDataStorage &datastore, const label_t& labeldef) :
	CRecord(datastore)
{
	SetRecordType(labeldef.GetInSST() ? RECTYPE_LABELSST : RECTYPE_LABEL);
	AddValue16((uint16)labeldef.GetRow());
	AddValue16((uint16)labeldef.GetCol());
	AddValue16(labeldef.GetXFIndex());
	if(labeldef.GetInSST()) {
		size_t index = labeldef.GetGlobalRecords().GetLabelSSTIndex(labeldef);
		AddValue32((uint32)index);
	} else {
		AddUnicodeString(labeldef.GetStrLabel(), LEN2_FLAGS_UNICODE);
	}
	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CLabel::~CLabel()
{
}
