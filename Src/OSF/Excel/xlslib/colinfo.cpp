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

using namespace std;
using namespace xlslib_core;

CColInfo::CColInfo(CDataStorage &datastore, const colinfo_t* newci) : CRecord(datastore)
{
	SetRecordType(RECTYPE_COLINFO);
	AddValue16((uint16)newci->colfirst);
	AddValue16((uint16)newci->collast);
	AddValue16(newci->width);
	if(newci->xformat != NULL) {
		AddValue16(newci->xformat->GetIndex());
	}
	else {
		AddValue16(0x0F); // was 0 - but default CELL format is 15
	}
	AddValue16(newci->flags);
	AddValue16(0);
	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}
