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
 *********************************
 *  note_t class implementation
 *********************************
 */
note_t::note_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, const std::string& msg, const std::string& auth, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval)
{
	gRecords.char2str16(msg, this->text);
	gRecords.char2str16(auth, this->author);
}

note_t::note_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, const ustring& msg, const ustring& auth, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval)
{
	gRecords.wide2str16(msg, this->text);
	gRecords.wide2str16(auth, this->author);
}

#ifndef __FRAMEWORK__
note_t::note_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, const u16string& msg, const u16string& auth, xf_t* pxfval) :
	cell_t(gRecords, rowval, colval, pxfval),
	text(msg),
	author(auth)
{
}

#endif

note_t::~note_t()
{
}

size_t note_t::GetSize(void) const
{
	return 12;
}

CUnit* note_t::GetData(CDataStorage &datastore) const
{
	return datastore.MakeCNote(*this);  // NOTE: this pointer HAS to be deleted elsewhere.
}

/*
 *********************************
 *  CNote class implementation
 *********************************
 *
 *  BIFF ROW (208h)  16  00 00 00 00 03 00 2C 01  00 00 00 00 00 01 0F 00
 *  BIFF 6 (06h)  43  00 00 00 00 0F 00 8C 16  22 AA FD 90 F8 3F 00 00
 *  C0 00 00 FD 15 00 1E 01  00 1F 00 00 00 00 00 00
 *  D0 3F 04 41 13 00 1E 04  00 06 03
 *  BIFF 6 (06h)  39  00 00 01 00 0F 00 8C 16  22 AA FD 90 98 40 00 00
 *  00 00 00 FE 11 00 44 00  00 00 C0 44 00 00 02 C0
 *  05 44 00 00 02 C0 05
 *  BIFF RK (27Eh)  10  00 00 02 00 0F 00 00 00  40 40
 *  BIFF DBCELL (D7h)  6  7C 00 00 00 00 00
 *  BIFF MSODRAWING (ECh)  224  0F 00 02 F0 7E 01 00 00  10 00 08 F0 08 00 00 00
 *  03 00 00 00 02 04 00 00  0F 00 03 F0 66 01 00 00
 *  0F 00 04 F0 28 00 00 00  01 00 09 F0 10 00 00 00
 *  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
 *  02 00 0A F0 08 00 00 00  00 04 00 00 05 00 00 00
 *  0F 00 04 F0 90 00 00 00  A2 0C 0A F0 08 00 00 00
 *  01 04 00 00 00 0A 00 00  D3 00 0B F0 4E 00 00 00
 *  80 00 A8 92 41 0C 8B 00  02 00 00 00 BF 00 08 00
 *  08 00 58 01 00 00 00 00  81 01 FF FF E1 00 83 01
 *  FF FF E1 00 85 01 F4 00  00 10 BF 01 10 00 10 00
 *  C3 01 F4 00 00 10 01 02  00 00 00 00 03 02 F4 00
 *  00 10 3F 02 03 00 03 00  BF 03 02 00 02 00 00 00
 *  10 F0 12 00 00 00 03 00  00 00 60 02 01 00 40 00
 *  02 00 60 02 04 00 F3 00  00 00 11 F0 00 00 00 00
 *  BIFF OBJ (5Dh)  52  15 00 12 00 19 00 01 00  11 40 A8 92 41 0C 80 8D
 *  42 0C 00 00 00 00 0D 00  16 00 D8 AC 8C FF 20 14
 *  4F 47 BA 90 F3 6F D0 9F  7B C0 00 00 10 00 00 00
 *  00 00 00 00
 *  BIFF MSODRAWING (ECh)  8  00 00 0D F0 00 00 00 00
 *  BIFF TXO (1B6h)  18  12 02 00 00 00 00 00 00  00 00 1A 00 10 00 00 00
 *  00 00
 *  BIFF CONTINUE (3Ch)  27  00 41 42 43 44 45 46 47  48 49 4A 4B 4C 4D 4E 4F
 *  50 51 52 53 54 55 56 57  58 59 5A
 *  BIFF CONTINUE (3Ch)  16  00 00 05 00 BF 00 0E 00  1A 00 00 00 00 00 00 00
 *  BIFF MSODRAWING (ECh)  150  0F 00 04 F0 96 00 00 00  A2 0C 0A F0 08 00 00 00
 *  02 04 00 00 00 0A 00 00  E3 00 0B F0 54 00 00 00
 *  80 00 28 C8 3F 04 85 00  01 00 00 00 8B 00 02 00
 *  00 00 BF 00 08 00 0A 00  58 01 00 00 00 00 81 01
 *  FF FF E1 00 83 01 FF FF  E1 00 85 01 F4 00 00 10
 *  BF 01 10 00 10 00 C3 01  F4 00 00 10 01 02 00 00
 *  00 00 03 02 F4 00 00 10  3F 02 03 00 03 00 BF 03
 *  00 00 02 00 00 00 10 F0  12 00 00 00 03 00 01 00
 *  90 01 01 00 4D 00 02 00  20 03 02 00 33 00 00 00
 *  11 F0 00 00 00 00
 *  BIFF OBJ (5Dh)  52  15 00 12 00 19 00 02 00  11 40 28 C8 3F 04 20 8A
 *  42 0C 00 00 00 00 0D 00  16 00 19 B5 48 49 52 4A
 *  AF 49 9E AB 90 6C 27 12  73 34 00 00 8B 00 02 00
 *  00 00 00 00
 *  BIFF MSODRAWING (ECh)  8  00 00 0D F0 00 00 00 00
 *  BIFF TXO (1B6h)  18  12 02 00 00 00 00 00 00  00 00 0A 00 10 00 00 00
 *  00 00
 *  BIFF CONTINUE (3Ch)  11  00 30 31 32 33 34 35 36  37 38 39
 *  BIFF CONTINUE (3Ch)  16  00 00 05 00 46 00 0E 00  0A 00 72 00 00 00 00 00
 *  BIFF NOTE (1Ch)  16  00 00 00 00 00 00 01 00  04 00 00 75 73 65 72 00
 *  BIFF NOTE (1Ch)  16  00 00 01 00 00 00 02 00  04 00 00 75 73 65 72 00
 *  BIFF WINDOW2 (23Eh)  18  B6 00 00 00 00 00 40 00  00 00 00 00 00 00 72 00
 *  00 00
 *
 */

CNote::CNote(CDataStorage &datastore, const note_t& notedef) :
	CRecord(datastore)
{
	uint16 idx = 1; // OBJ reference

	SetRecordType(RECTYPE_NOTE);
	AddValue16((uint16)notedef.GetRow());
	AddValue16((uint16)notedef.GetCol());
	AddValue16(0); // grBit
	AddValue16(idx);
	AddUnicodeString(notedef.GetAuthor(), LEN2_NOFLAGS_PADDING_UNICODE);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CNote::~CNote()
{
}

// make an OBJ record:
void CNote::mk_obj_Record(const note_t* notedef)
{
	uint16 idx = 1;

	SetRecordType(RECTYPE_OBJ);
	AddValue16((uint16)notedef->GetRow());
	AddValue16((uint16)notedef->GetCol());
	AddValue16(0); // grBit
	AddValue16(idx);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

// start
void CNote::mk_obj_CMO_SubRecord(const note_t* notedef)
{
	(void)notedef; // stop warning
	AddValue16(0x15);  // ftCmo
	AddValue16(14+12-4);

	AddValue16(0x19); // ot = Comment
	AddValue16(1);    // id = OBJ id = 1
	AddValue16(0);    // flags
	AddFixedDataArray(0, 14+12-10);    // reserved

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

// end
void CNote::mk_obj_END_SubRecord(const note_t* notedef)
{
	(void)notedef; // stop warning
	AddValue16(0x00); // ftEnd
	AddValue16(0);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

// note structure	???
void CNote::mk_obj_NTS_SubRecord(const note_t* notedef)
{
	(void)notedef; // stop warning
	AddValue16(0x0D); // ftNts
	AddValue16(0);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}
