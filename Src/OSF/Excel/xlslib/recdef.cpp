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

boundsheet_t::boundsheet_t(CGlobalRecords& gRecords) :
	sheetname(),
	streampos(0),
	sheetData(NULL),
	m_GlobalRecords(gRecords)
{
	SetAttributes(0);
}

boundsheet_t::boundsheet_t(CGlobalRecords& gRecords, const u16string& sn, uint16 attributes, uint32 sp) :
	sheetname(sn),
	streampos(sp),
	sheetData(NULL),
	m_GlobalRecords(gRecords)
{
	SetAttributes(attributes);
}

boundsheet_t::~boundsheet_t()
{
}

void boundsheet_t::SetAttributes(uint16 attributes)
{
	worksheet = ((attributes & BSHEET_ATTR_WORKSHEET) == BSHEET_ATTR_WORKSHEET);
	ex4macro = ((attributes & BSHEET_ATTR_EX4MACRO) == BSHEET_ATTR_EX4MACRO);
	chart = ((attributes & BSHEET_ATTR_CHART) == BSHEET_ATTR_CHART);
	vbmodule = ((attributes & BSHEET_ATTR_VBMODULE) == BSHEET_ATTR_VBMODULE);
	visible = ((attributes & BSHEET_ATTR_VISIBLE) == BSHEET_ATTR_VISIBLE);
	hidden = ((attributes & BSHEET_ATTR_HIDDEN) == BSHEET_ATTR_HIDDEN);
	veryhidden = ((attributes & BSHEET_ATTR_VERYHIDDEN) == BSHEET_ATTR_VERYHIDDEN);
}

boundsheet_t::boundsheet_t(const boundsheet_t& that) :
	sheetname(that.sheetname),
	streampos(that.streampos),
	worksheet(that.worksheet),
	ex4macro(that.ex4macro),
	chart(that.chart),
	vbmodule(that.vbmodule),
	visible(that.visible),
	hidden(that.hidden),
	veryhidden(that.veryhidden),
	sheetData(that.sheetData),
	m_GlobalRecords(that.m_GlobalRecords)
{
}

boundsheet_t& boundsheet_t::operator=(const boundsheet_t& right)
{
	(void)right; // stop warning
	throw std::string("Should never have invoked the boundsheet_t copy operator!");
}

void boundsheet_t::SetSheetStreamPosition(size_t offset)
{
	sheetData->SetStreamPosition(offset);
}

/*
 ******************************
 * CBof class implementation
 ******************************
 */
CBof::CBof(CDataStorage &datastore, uint16 boftype) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_BOF);

	AddValue16(VERSION_BIFF);
	AddValue16(boftype);
	AddValue16(BOF_BUILD_DFLT);
	AddValue16(BOF_YEAR_DFLT);

	AddValue32(0);                  // The file history flags are all set to zero
	AddValue32(VERSION_BIFF);       // The lowest BIFF version

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CBof::~CBof()
{}

/*
 ******************************
 * CEof class implementation
 ******************************
 */
CEof::CEof(CDataStorage &datastore) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_EOF);
	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CEof::~CEof()
{
}

/*
 **********************************
 *  CCodePage class implementation
 **********************************
 */
CCodePage::CCodePage(CDataStorage &datastore, uint16 boftype) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_CODENAME);

	AddValue16(boftype);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CCodePage::~CCodePage()
{}

/*
 **********************************
 *  window1 class implementation
 **********************************
 */
window1::window1() :
	horzPos(0), vertPos(0),
	windWidth(0x37e0/TWIP), windHeight(0x25e0/TWIP),
	activeSheet(0),
	firstVisibleTab(0),
	tabBarWidth(500)
{
}

window1::~window1()
{
}

/*
 **********************************
 *  CWindow1 class implementation
 **********************************
 */
CWindow1::CWindow1(CDataStorage &datastore, const window1& wind1) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_WINDOW1);

	AddValue16(wind1.horzPos*TWIP);
	AddValue16(wind1.vertPos*TWIP);
	AddValue16(wind1.windWidth*TWIP);
	AddValue16(wind1.windHeight*TWIP);
	AddValue16(0x0038);                 // FLAGS: tabBar, vertScroller, horzScroller
	AddValue16(wind1.activeSheet);
	AddValue16(wind1.firstVisibleTab);      // only useful for when you have so many tabs the tab scroller is active
	AddValue16(1);                      // number of selected sheets
	AddValue16(wind1.tabBarWidth);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CWindow1::~CWindow1()
{
}

/*
 **********************************
 *  CDateMode class implementation
 **********************************
 */

CDateMode::CDateMode(CDataStorage &datastore) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_DATEMODE);

	AddValue16(Is_In_1904_Mode() ? 1 : 0);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CDateMode::~CDateMode()
{}


bool CDateMode::Is_In_1904_Mode(void)
{
#ifdef __APPLE__
	return true; // 1904  [i_a]

#else
	return false;   // 1900

#endif
}

/*
 **********************************
 *  CWindow2 class implementation
 **********************************
 */
CWindow2::CWindow2(CDataStorage &datastore, bool isActive) :
	CRecord(datastore)
{
	uint16 flags;

	SetRecordType(RECTYPE_WINDOW2);

	flags =	W2_GRBITMASK_GUTS|W2_GRBITMASK_DFLTHDRCOLOR|W2_GRBITMASK_ZEROS|     // 0x00B0
		W2_GRBITMASK_HROWCOL|W2_GRBITMASK_GRIDS;                                // 0x0006
	if(isActive) {
		flags |= W2_GRBITMASK_ACTIVE|W2_GRBITMASK_SELECTED;
	}

	AddValue16(flags);
	AddValue16(W2_DFLT_TOPROW);
	AddValue16(W2_DFLT_LEFTCOL);

	AddValue16(COLOR_CODE_SYS_WIND_FG); // grid color
	AddValue16(0);                      // UNUSED
	AddValue16(0);                      // zoom page break preview, default == 0 (W2_DFLT_ZOOMPBPREV ???)
	AddValue16(0);                      // zoom normal view, default == 0 (W2_DFLT_ZOOMNORMAL ???)
	AddValue32(W2_DFLT_RESERVED);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CWindow2::~CWindow2()
{
}

/*
 **********************************
 *  CDimension class implementation
 **********************************
 */
CDimension::CDimension(CDataStorage &datastore,
					   uint32 minRow,
					   uint32 maxRow,
					   uint32 minCol,
					   uint32 maxCol) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_DIMENSIONS);

	AddValue32(minRow);
	AddValue32(maxRow+1);
	AddValue16((uint16)minCol);
	AddValue16((uint16)(maxCol+1));                       // zoom, default == 0 (W2_DFLT_ZOOMPBPREV ???)
	AddValue16(W2_DFLT_RESERVED);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CDimension::~CDimension()
{
}

void CWindow2::SetSelected()
{
	uint16 grbitval;

	GetValue16From(&grbitval, W2_OFFSET_GRBIT);

	grbitval |= W2_GRBITMASK_SELECTED;

	SetValueAt16(grbitval, W2_OFFSET_GRBIT);
}

void CWindow2::SetPaged()
{
	uint16 grbitval;

	GetValue16From(&grbitval, W2_OFFSET_GRBIT);

	grbitval |= W2_GRBITMASK_PAGEBRK;

	SetValueAt16(grbitval, W2_OFFSET_GRBIT);
}

void CWindow2::ClearSelected()
{
	uint16 grbitval;

	GetValue16From(&grbitval, W2_OFFSET_GRBIT);

	grbitval &= ~W2_GRBITMASK_SELECTED;

	SetValueAt16(grbitval, W2_OFFSET_GRBIT);
}

void CWindow2::ClearPaged()
{
	uint16 grbitval;

	GetValue16From(&grbitval, W2_OFFSET_GRBIT);

	grbitval &= W2_GRBITMASK_PAGEBRK;

	SetValueAt16(grbitval, W2_OFFSET_GRBIT);
}

/*
 ******************************
 * CStyle class implementation
 ******************************
 */
#define STYLE_BUILTIN_NORMAL      ((uint8)0x00)
#define STYLE_BUILTIN_ROWLEVELN   ((uint8)0x01)
#define STYLE_BUILTIN_COLLEVELN   ((uint8)0x02)
#define STYLE_BUILTIN_COMMA       ((uint8)0x03)
#define STYLE_BUILTIN_CURRENCY    ((uint8)0x04)
#define STYLE_BUILTIN_PERCENT     ((uint8)0x05)
#define STYLE_BUILTIN_COMMAT      ((uint8)0x06)
#define STYLE_BUILTIN_CURRENCYT   ((uint8)0x07)

#define STYLE_BUILTIN_BIT   ((uint16)0x8000)

//#define STYLE_LEVEL_DUMMY         ((uint8)0x00)

CStyle::CStyle(CDataStorage &datastore, const style_t* styledef) :
	CRecord(datastore)
{
	// TODO: Implement user-defined styles. So far only built-in are used.
	SetRecordType(RECTYPE_STYLE);

	AddValue16(styledef->xfindex|STYLE_BUILTIN_BIT);
	AddValue8(styledef->builtintype);
	AddValue8(styledef->level);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CStyle::~CStyle()
{
}

/*
 ******************************
 * CBSheet class implementation
 ******************************
 */
CBSheet::CBSheet(CDataStorage &datastore, const boundsheet_t* bsheetdef) :
	CRecord(datastore)
{
	m_Backpatching_Level = 3;

	SetRecordType(RECTYPE_BOUNDSHEET);
	AddValue32(bsheetdef->GetStreamPos());

	// Set the flags in the attribute variables
	uint16 attrflags = 0;
	// attrflags |= (bsheetdef->IsWorkSheet()	? BSHEET_ATTR_WORKSHEET:0); // has no affect
	attrflags |= (bsheetdef->IsEx4macro()	? BSHEET_ATTR_EX4MACRO : 0);
	attrflags |= (bsheetdef->IsChart()		? BSHEET_ATTR_CHART : 0);
	attrflags |= (bsheetdef->IsVBModule()	? BSHEET_ATTR_VBMODULE : 0);
	attrflags |= (bsheetdef->IsVisible()	? BSHEET_ATTR_VISIBLE : BSHEET_ATTR_HIDDEN);
	attrflags |= (bsheetdef->IsHidden()		? BSHEET_ATTR_HIDDEN : 0);
	attrflags |= (bsheetdef->IsVeryHidden()	? BSHEET_ATTR_VERYHIDDEN : 0);

	AddValue16(attrflags);
	AddUnicodeString(bsheetdef->GetSheetName(), LEN1_FLAGS_UNICODE);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CBSheet::~CBSheet()
{
}

void CBSheet::SetStreamPosition(size_t pos)
{
	SetValueAt32((uint32)pos, BSHEET_OFFSET_POSITION);
}
