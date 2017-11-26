/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
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

rowheight_t::rowheight_t() :
	xformat(NULL),
	num(0),
	height(ROW_DFLT_HEIGHT)
{
}

rowheight_t::rowheight_t(uint32 rownum, uint16 rowheight, xf_t *pxformat) :
	xformat(pxformat),
	num(rownum),
	height(rowheight)
{
}

rowheight_t::~rowheight_t()
{
	if(xformat) {
		xformat->UnMarkUsed();
	}
}

/*
 ******************************
 * CRow class implementation
 ******************************
 */

CRow::CRow(CDataStorage &datastore,
		   uint32 rownum,
		   uint32 firstcol,
		   uint32 lastcol,
		   uint16 rowheight,
		   const xf_t* xformat) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_ROW);
	AddValue16((uint16)rownum);
	AddValue16((uint16)firstcol);
	AddValue16((uint16)(lastcol+1));
	AddValue16(rowheight);

	// A field used by MS for "optimizing" (?) the loading of a file.
	// Doc says it shall be set to 0 if I'm creating a BIFF...
	AddValue16(0);
	// A reserved value:
	AddValue16(0);

	// TODO: The following flag-word can be used for outline cells.
	// As a default the GhostDirty flag is set, so the row has a default
	// format (set by the index of byte 18).
	if(rowheight == ROW_DFLT_HEIGHT) {
		AddValue16(ROW_DFLT_GRBIT /*|0x100*/); // [i_a] Excel2003 also sets bit 8: 0x100
	} else {
		AddValue16(ROW_DFLT_GRBIT|ROW_GRBIT_UNSYNC /*|0x100*/);
	}
	if(xformat == NULL) {
		AddValue16(ROW_DFLT_IXFE);
	} else {
		AddValue16(xformat->GetIndex());
	}

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CRow::~CRow()
{
}

/*
 ******************************
 * CDBCell class implementation
 ******************************
 */
CDBCell::CDBCell(CDataStorage &datastore, size_t startblock) :
	CRecord(datastore)
{
	m_Backpatching_Level = 1;

	// The new initializated DBCell record points to nowhere and has no
	// extra rows (the array of stream offsets is empty);
	SetRecordType(RECTYPE_DBCELL);
	AddValue32((uint32)startblock);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CDBCell::~CDBCell()
{
}

void CDBCell::AddRowOffset(size_t rowoffset)
{
	AddValue16((uint16) rowoffset);
	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}
