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


static const uint16 format2index_arr[] =
{
	FMTCODE_GENERAL,
	FMTCODE_NUMBER1,
	FMTCODE_NUMBER2,
	FMTCODE_NUMBER3,
	FMTCODE_NUMBER4,
	FMTCODE_CURRENCY1,
	FMTCODE_CURRENCY2,
	FMTCODE_CURRENCY3,
	FMTCODE_CURRENCY4,
	FMTCODE_PERCENT1,
	FMTCODE_PERCENT2,
	FMTCODE_SCIENTIFIC1,
	FMTCODE_FRACTION1,
	FMTCODE_FRACTION2,
	FMTCODE_DATE1,
	FMTCODE_DATE2,
	FMTCODE_DATE3,
	FMTCODE_DATE4,
	FMTCODE_HOUR1,
	FMTCODE_HOUR2,
	FMTCODE_HOUR3,
	FMTCODE_HOUR4,
	FMTCODE_HOURDATE,
	FMTCODE_ACCOUNTING1,
	FMTCODE_ACCOUNTING2,
	FMTCODE_ACCOUNTING3,
	FMTCODE_ACCOUNTING4,
	FMTCODE_CURRENCY5,
	FMTCODE_CURRENCY6,
	FMTCODE_CURRENCY7,
	FMTCODE_CURRENCY8,
	FMTCODE_HOUR5,
	FMTCODE_HOUR6,
	FMTCODE_HOUR7,
	FMTCODE_SCIENTIFIC2,
	FMTCODE_TEXT
};

uint16 format_t::format2index(format_number_t idx)
{
	if ((uint16)idx > FMT_TEXT) {
		idx = FMT_GENERAL;
	}

	return format2index_arr[idx];
}

/*
 **********************************
 **********************************
 */

format_t::format_t(const format_t& orig) :
	formatstr(orig.formatstr),
	index(0),
	m_usage_counter(0),
	m_GlobalRecords(orig.m_GlobalRecords)
{
	m_GlobalRecords.AddFormat(this);
}

format_t::format_t(CGlobalRecords& gRecords, const std::string& fmtstr) :
	formatstr(),
	index(0),
	m_usage_counter(0),
	m_GlobalRecords(gRecords)
{
	gRecords.char2str16(fmtstr, formatstr);
}

format_t::format_t(CGlobalRecords& gRecords, const ustring& fmtstr) :
	formatstr(),
	index(0),
	m_usage_counter(0),
	m_GlobalRecords(gRecords)
{
	gRecords.wide2str16(fmtstr, formatstr);
}

#ifndef __FRAMEWORK__
format_t::format_t(CGlobalRecords& gRecords, const u16string& fmtstr) :
	formatstr(fmtstr),
	index(0),
	m_usage_counter(0),
	m_GlobalRecords(gRecords)
{
}
#endif

format_t &format_t::operator =(const format_t &src)
{
	(void)src; // stop warning
	throw std::string("Should never have invoked the format_t copy operator!");
}

void format_t::MarkUsed(void)
{
	m_usage_counter++;
}

void format_t::UnMarkUsed(void)
{
	if(m_usage_counter) {
		m_usage_counter--;
	}
}

uint32 format_t::Usage(void) const
{
	return m_usage_counter;
}

/*
 **********************************
 *  CFormat class implementation
 **********************************
 */
CFormat::CFormat(CDataStorage &datastore, const format_t* formatdef) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_FORMAT);

	AddValue16(formatdef->GetIndex());
	//cerr << "Format: index=" << formatdef->GetIndex() << endl << flush;

	AddUnicodeString(formatdef->GetFormatStr(), LEN2_FLAGS_UNICODE);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CFormat::~CFormat()
{
}
