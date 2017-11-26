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

#if defined(HAVE_WORKING_ICONV)
static const uint16 convFail[] = { 'i', 'c', 'o', 'n', 'v', ' ', 'f', 'a', 'i', 'l', 'e', 'd', '!', 0 };
#endif

/*
 **********************************************************************
 *  CGlobalRecords class implementation
 **********************************************************************
 */

CGlobalRecords::CGlobalRecords() :
	m_Fonts(),
	m_DefaultFonts(),
	m_Formats(),
	m_XFs(),
	m_DefaultXFs(),
	m_Styles(),
	m_BoundSheets(),
	m_Labels(),
	m_window1(),
	m_palette(),

	defaultXF(NULL),

#if defined(HAVE_WORKING_ICONV)
	iconv_code(),
#endif
	m_DumpState(GLOBAL_INIT),
	font(),
	font_dflt(),
	fontIndex(0),
	formatIndex(FMTCODE_GENERAL),
	format(),
	xf(),
	xf_dflt(),
	xfIndex(0),
	style(),
	bsheet(),
	label()
{
	// set to what Excel 2004 on Mac outputs 12/12/2008

	// Initialize default fonts
	font_t *newfont;
	font_t *font0;
	font_t *font1;
	font_t *font2;
	font_t *font4;

	newfont = new font_t(*this, 0, "Verdana", 200, BOLDNESS_NORMAL, UNDERLINE_NONE, SCRIPT_NONE, ORIG_COLOR_BLACK, FONT_DFLT_ATTRIBUTES, FONT_DFLT_FAMILY,
			FONT_DFLT_CHARSET);
	// mark as used TWICE to ensure these fonts are never discarded, even when 'unused'
	newfont->MarkUsed();
	newfont->MarkUsed();
	m_DefaultFonts.push_back(newfont);
	font0 = newfont;

	newfont = new font_t(*this, 1, "Verdana", 200, BOLDNESS_BOLD, UNDERLINE_NONE, SCRIPT_NONE, ORIG_COLOR_BLACK, FONT_ATTR_BOLD, FONT_DFLT_FAMILY,
			FONT_DFLT_CHARSET);
	newfont->MarkUsed();
	newfont->MarkUsed();
	m_DefaultFonts.push_back(newfont);
	font1 = newfont;

	newfont = new font_t(*this, 2, "Verdana", 200, BOLDNESS_NORMAL, UNDERLINE_NONE, SCRIPT_NONE, ORIG_COLOR_BLACK, FONT_ATTR_ITALIC, FONT_DFLT_FAMILY,
			FONT_DFLT_CHARSET);
	newfont->MarkUsed();
	newfont->MarkUsed();
	m_DefaultFonts.push_back(newfont);
	font2 = newfont;

	newfont = new font_t(*this, 3, "Verdana", 200, BOLDNESS_BOLD, UNDERLINE_NONE, SCRIPT_NONE, ORIG_COLOR_BLACK, FONT_ATTR_BOLD|FONT_ATTR_ITALIC,
			FONT_DFLT_FAMILY, FONT_DFLT_CHARSET);
	newfont->MarkUsed();
	newfont->MarkUsed();
	m_DefaultFonts.push_back(newfont);

	// Excel spec for FONT says ignore 4
	newfont = new font_t(*this, 5, "Verdana", 200, BOLDNESS_NORMAL, UNDERLINE_NONE, SCRIPT_NONE, ORIG_COLOR_BLACK, FONT_DFLT_ATTRIBUTES, FONT_DFLT_FAMILY,
			FONT_DFLT_CHARSET);
	newfont->MarkUsed();
	newfont->MarkUsed();
	m_DefaultFonts.push_back(newfont);
	font4 = newfont;

	fontIndex = 6;  // this will be 1 more than last standard font

	for(xfIndex=0; xfIndex<21; ++xfIndex) {
		xf_t*			newxf;
		font_t			*fnt;
		format_number_t	fmt;
		bool is_cell;

		fnt		= font0;
		fmt		= FMT_GENERAL;
		is_cell	= XF_IS_STYLE;

		switch(xfIndex)	{
		case 0:
			fnt		= NULL;
			break;
		case 1:
		case 2:
			fnt		= font1;
			break;
		case 3:
		case 4:
			fnt		= font2;
			break;
		case 15:
			fnt		= NULL;
			is_cell	= XF_IS_CELL;
			break;
		case 16:
			fnt		= font4;
			fmt		= FMT_CURRENCY7;
			break;
		case 17:
			fnt		= font4;
			fmt		= FMT_CURRENCY5;
			break;
		case 18:
			fnt		= font4;
			fmt		= FMT_CURRENCY8;
			break;
		case 19:
			fnt		= font4;
			fmt		= FMT_CURRENCY6;
			break;
		case 20:
			fnt		= font4;
			fmt		= FMT_PERCENT1;
			break;
		}

//		newxf = is_cell == XF_IS_CELL ? new xf_t(false) : new xf_t(xfi, false/*userXF*/, is_cell, xfIndex?false:true);
		newxf = new xf_t(*this, false /*userXF*/, is_cell, xfIndex ? false : true);

		// override defaults
		if(fnt != NULL) {newxf->SetFont(fnt); }
		if(fnt == font4) {
			newxf->ClearFlag(XF_ALIGN_ATRFONT);                     // Ask Mr Bill why...Done to make binary the same
		}
		if(fmt != FMT_GENERAL) {newxf->SetFormat(fmt); }

		// mark as used TWICE to ensure these formats are never discarded, even when 'unused'
		newxf->MarkUsed();
		newxf->MarkUsed();
		m_DefaultXFs.push_back(newxf);

		if(xfIndex == XF_PROP_XF_DEFAULT_CELL) {
			newxf->SetIndex(XF_PROP_XF_DEFAULT_CELL);
			defaultXF = newxf;
		}
		newxf->SetIndex(xfIndex);   // for debugging - not really needed here
	}
	XL_ASSERT(defaultXF);

	formatIndex = FMT_CODE_FIRST_USER;

	style_t* newstyle;
	newstyle = new style_t;
	newstyle->xfindex = 0x0010;
	newstyle->builtintype = 0x03;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	newstyle = new style_t;
	newstyle->xfindex = 0x0011;
	newstyle->builtintype = 0x06;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	newstyle = new style_t;
	newstyle->xfindex = 0x0012;
	newstyle->builtintype = 0x04;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	newstyle = new style_t;
	newstyle->xfindex = 0x0013;
	newstyle->builtintype = 0x07;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	newstyle = new style_t;
	newstyle->xfindex = 0x0000;
	newstyle->builtintype = 0x00;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	newstyle = new style_t;
	newstyle->xfindex = 0x0014;
	newstyle->builtintype = 0x05;
	newstyle->level = 0xFF;
	m_Styles.push_back(newstyle);

	// Initialize former static variables
	font	= m_Fonts.begin();
	format	= m_Formats.begin();
	xf		= m_XFs.begin();
	style	= m_Styles.begin();
	bsheet	= m_BoundSheets.begin();
	label   = m_Labels.begin();
}

CGlobalRecords::~CGlobalRecords()
{
	// Delete dynamically created lists elements

	if(!m_DefaultFonts.empty()) {
		for(Font_Vect_Itor_t fnt = m_DefaultFonts.begin(); fnt != m_DefaultFonts.end(); fnt++) {
			delete *fnt;
		}
		m_DefaultFonts.clear();
	}
	if(!m_Fonts.empty()) {
		for(Font_Vect_Itor_t fnt = m_Fonts.begin(); fnt != m_Fonts.end(); fnt++) {
			delete *fnt;
		}
		m_Fonts.clear();
	}
	if(!m_Formats.empty()) {
		for(Format_Vect_Itor_t fnt = m_Formats.begin(); fnt != m_Formats.end(); fnt++) {
			delete *fnt;
		}
		m_Formats.clear();
	}
	if(!m_DefaultXFs.empty()) {
		for(XF_Vect_Itor_t xfi = m_DefaultXFs.begin(); xfi != m_DefaultXFs.end(); xfi++) {
			delete *xfi;
		}
		m_DefaultXFs.clear();
	}
	if(!m_XFs.empty()) {
		for(XF_Vect_Itor_t xfi = m_XFs.begin(); xfi != m_XFs.end(); xfi++) {
			delete *xfi;
		}
		m_XFs.clear();
	}
	if(!m_Styles.empty()) {
		for(Style_Vect_Itor_t xfi = m_Styles.begin(); xfi != m_Styles.end(); xfi++) {
			delete *xfi;
		}
		m_Styles.clear();
	}

	if(!m_BoundSheets.empty()) {
		for(Boundsheet_Vect_Itor_t xfi = m_BoundSheets.begin(); xfi != m_BoundSheets.end(); xfi++) {
			delete *xfi;
		}
		m_BoundSheets.clear();
	}
}

size_t CGlobalRecords::EstimateNumBiffUnitsNeeded4Header(void)
{
	size_t ret = 5;

	ret += m_Fonts.size();
	ret += m_DefaultFonts.size();
	ret += m_Formats.size();
	ret += m_XFs.size();
	ret += m_DefaultXFs.size();
	ret += m_Styles.size();
	ret += m_BoundSheets.size();
	ret += m_Labels.size();

	return ret;
}

CUnit* CGlobalRecords::DumpData(CDataStorage &datastore)
{
	CUnit*	m_pCurrentData	= NULL;
	bool repeat			 = false;

	XTRACE("CGlobalRecords::DumpData");

	do {
		switch(m_DumpState) {
		case GLOBAL_INIT:
			XTRACE("\tINIT");

			repeat = true;

			font		= m_Fonts.begin();
			font_dflt	= m_DefaultFonts.begin();
			format		= m_Formats.begin();
			xf			= m_XFs.begin();
			xf_dflt		= m_DefaultXFs.begin();
			style		= m_Styles.begin();
			bsheet		= m_BoundSheets.begin();

			m_DumpState = GLOBAL_BOF;
			break;

		case GLOBAL_BOF:            // ********** STATE 1A *************
			XTRACE("\tBOF");

			repeat = false;

			m_pCurrentData = datastore.MakeCBof(BOF_TYPE_WBGLOBALS);
			m_DumpState = GLOBAL_CODEPAGE; // DFH GLOBAL_WINDOW1;
			break;

		case GLOBAL_CODEPAGE:       // ********** STATE 1B *************
			XTRACE("\tCODEPAGE");

			repeat = false;

			m_pCurrentData = datastore.MakeCCodePage(1200); // UTF-16
			m_DumpState = GLOBAL_WINDOW1;
			break;

		case GLOBAL_WINDOW1:        // ********** STATE 2A *************
			XTRACE("\tWINDOW1");

			repeat = false;

			m_pCurrentData = datastore.MakeCWindow1(m_window1);
			m_DumpState = GLOBAL_DATEMODE; // GLOBAL_DEFAULTFONTS;
			break;

		case GLOBAL_DATEMODE:       // ********** STATE 2B *************
			XTRACE("\tDATEMODE");

			repeat = false;

			m_pCurrentData = datastore.MakeCDateMode();
			m_DumpState = GLOBAL_DEFAULTFONTS;
			break;

		case GLOBAL_DEFAULTFONTS:   // ********** STATE 3A *************
			XTRACE("\tDEFAULTFONTS");

			repeat = false;
			m_pCurrentData = datastore.MakeCFont(*font_dflt);

			if(font_dflt != (--m_DefaultFonts.end())) {
				// if it wasn't the last font from the list, increment to get the next one
				font_dflt++;
			} else {
				// if it was the last from the list, change the DumpState
				m_DumpState = GLOBAL_FONTS;
				// font_dflt = m_DefaultFonts.begin();
			}
			break;

		case GLOBAL_FONTS: // ********** STATE 3B *************
			XTRACE("\tFONTS");
			// First check if the list of fonts is not empty...
			if(!m_Fonts.empty()) {
				m_pCurrentData = datastore.MakeCFont(*font);
				if(font != (--m_Fonts.end())) {
					// if it was'nt the last font from the list, increment to get the next one
					font++;
				} else {
					// if it was the last from the list, change the DumpState
					m_DumpState = GLOBAL_FORMATS;
					font = m_Fonts.begin();
				}
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_FORMATS;
				//font = m_Fonts.begin();
				repeat = true;
			}
			break;

		case GLOBAL_FORMATS:  // ********** STATE 4 *************
			XTRACE("\tFORMATS");

			if(!m_Formats.empty()) {
				m_pCurrentData = datastore.MakeCFormat(*format);
				if(format != (--m_Formats.end())) {
					// if it wasn't the last font from the list, increment to get the next one
					format++;
				} else {
					// if it was the last from the list, change the DumpState
					m_DumpState = GLOBAL_DEFAULTXFS;
					format = m_Formats.begin();
				}
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_DEFAULTXFS;
				// format = m_Formats.begin();
				repeat = true;
			}
			break;

		case GLOBAL_DEFAULTXFS:  // ********** STATE 5a *************
			XTRACE("\tXDEFAULTFS");
			m_pCurrentData = datastore.MakeCExtFormat(*xf_dflt);

			if(xf_dflt != (--m_DefaultXFs.end())) {
				// if it wasn't the last font from the list, increment to get the next one
				xf_dflt++;
				repeat = false;
			} else {
				// if it was the last from the list, change the DumpState
				m_DumpState = GLOBAL_XFS;
				//xf_dflt = m_DefaultXFs.begin();
				repeat = false;
			}
			break;

		case GLOBAL_XFS:  // ********** STATE 5b *************
			XTRACE("\tXFS");
			if(!m_XFs.empty()) {
				m_pCurrentData = datastore.MakeCExtFormat(*xf);

				if(xf != (--m_XFs.end())) {
					// if it wasn't the last font from the list, increment to get the next one
					xf++;
				} else {
					// if it was the last from the list, change the DumpState
					m_DumpState = GLOBAL_STYLES;
					xf = m_XFs.begin();
				}
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_STYLES;
				//xf = m_XFs.begin();
				repeat = true;
			}
			break;

		case GLOBAL_STYLES:  // ********** STATE 6 *************
			XTRACE("\tSTYLES");

			if(!m_Styles.empty()) {
				// First check if the list of fonts is not empty...
				m_pCurrentData = datastore.MakeCStyle(*style);

				if(style != (--m_Styles.end()))	{
					// if it wasn't the last font from the list, increment to get the next one
					style++;
				} else {
					// if it was the last from the list, change the DumpState
					m_DumpState = GLOBAL_PALETTE;
					//style = m_Styles.begin();
				}
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_PALETTE;
				//style = m_Styles.begin();
				repeat = true;
			}
			break;

		case GLOBAL_PALETTE:  // ********** STATE 7 *************
			XTRACE("\tPALETTE");

			repeat = false;

			m_pCurrentData = m_palette.GetData(datastore);
			//Delete_Pointer(m_pCurrentData);
			//m_pCurrentData = (CUnit*)(new CPalette(datastore, (colors ? colors : default_palette)));
			m_DumpState = GLOBAL_BOUNDSHEETS;
			break;

		case GLOBAL_BOUNDSHEETS:  // ********** STATE 8 *************
			XTRACE("\tBOUNDSHEETS");
			if(!m_BoundSheets.empty()) {
				// First check if the list of sheets is not empty...
				m_pCurrentData = (*bsheet)->SetSheetData(datastore.MakeCBSheet(*bsheet));

				if(bsheet != (--m_BoundSheets.end())) {
					// if it wasn't the last sheet from the list, increment to get the next one
					bsheet++;
				} else {
					// if it was the last from the list, change the DumpState
					m_DumpState = GLOBAL_LINK1;
					bsheet = m_BoundSheets.begin();
				}
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_LINK1;
				bsheet = m_BoundSheets.begin();
				repeat = true;
			}
			break;

        case GLOBAL_LINK1:
			XTRACE("\tSUPBOOK");
            m_pCurrentData = datastore.MakeCExternBook(static_cast<uint16>(m_BoundSheets.size()));
            repeat = false;
			m_DumpState = GLOBAL_LINK2;
            break;

        case GLOBAL_LINK2:
			XTRACE("\tEXTERNSHEET");
            m_pCurrentData = datastore.MakeCExternSheet(m_BoundSheets);
            repeat = false;
			m_DumpState = GLOBAL_SST;
            break;

		case GLOBAL_SST:  // ********** STATE 9 *************
			XTRACE("\tBOUNDSHEETS");

			if(!m_Labels.empty()) {
				// First check if the list of sheets is not empty...
				m_pCurrentData = datastore.MakeSST(m_Labels);
				// if it was the last from the list, change the DumpState
				m_DumpState = GLOBAL_EOF;
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				m_DumpState = GLOBAL_EOF;
				label = m_Labels.begin();
				repeat = true;
			}
			break;

		case GLOBAL_EOF: // ********** STATE 10 *************
			XTRACE("\tEOF");

			repeat = false;

			m_pCurrentData = datastore.MakeCEof();
			m_DumpState = GLOBAL_FINISH;
			break;

		case GLOBAL_FINISH:  // ********** STATE 11 *************
			XTRACE("\tFINISH");

			repeat = false;

			m_pCurrentData = NULL;
			m_DumpState = GLOBAL_INIT;
			break;

		default:
			/* It shouldn't get here */
			XTRACE("\tDEFAULT");
			break;
		}
	} while(repeat);

	return m_pCurrentData;
}

void CGlobalRecords::AddBoundingSheet(uint32 streampos,
									  uint16 attributes,
									  u16string& sheetname)
{
	boundsheet_t* bsheetdef = new boundsheet_t(*this, sheetname, attributes, streampos);

	m_BoundSheets.push_back(bsheetdef);
}

void CGlobalRecords::AddBoundingSheet(boundsheet_t* bsheetdef)
{
	m_BoundSheets.push_back(bsheetdef);
}

/*
 ****************************************
 *  It returns pointers to BoundingSheets one by one until
 *  all are spanned, in which case the returned pointer is NULL
 ****************************************
 */
void CGlobalRecords::GetBoundingSheets(Boundsheet_Vect_Itor_t& bs)
{
	if(bs != m_BoundSheets.end()) {
		bs++;
	} else {
		bs = m_BoundSheets.begin();
	}
}

Boundsheet_Vect_Itor_t CGlobalRecords::GetFirstBoundSheet()
{
	return m_BoundSheets.begin();
}

Boundsheet_Vect_Itor_t CGlobalRecords::GetBoundSheetAt(uint32 idx)
{
	Boundsheet_Vect_Itor_t bs;

	bs = m_BoundSheets.begin();
	while(idx--) {
		bs++;
	}

	return bs;
}

Boundsheet_Vect_Itor_t CGlobalRecords::GetEndBoundSheet()
{
	return m_BoundSheets.end();
}

void CGlobalRecords::AddFormat(format_t* newformat)
{
	newformat->SetIndex(formatIndex++);
	m_Formats.push_back(newformat);
}

void CGlobalRecords::AddFont(font_t* newfont)
{
	newfont->SetIndex(fontIndex++);
	m_Fonts.push_back(newfont);
}

font_t* CGlobalRecords::GetDefaultFont() const
{
	return *m_DefaultFonts.begin();
}

void CGlobalRecords::AddXFormat(xf_t* xfi)
{
	xfi->SetIndex(xfIndex++);
	m_XFs.push_back(xfi);
}

void CGlobalRecords::AddLabelSST(const label_t& labeldef)
{
	if(labeldef.GetInSST()) {
		m_Labels.push_back(&labeldef);
	}
}

size_t CGlobalRecords::GetLabelSSTIndex(const label_t& labeldef)
{
	size_t idx = 0;
	cLabel_Vect_Itor_t label_end = m_Labels.end();
	for(cLabel_Vect_Itor_t lbl = m_Labels.begin(); lbl != label_end; ++lbl) {
		if(&labeldef == (*lbl)) {
			return idx;
		}
		++idx;
	}
	XL_ASSERTS("Did not find a label");
	return (size_t)GLOBAL_INVALID_STORE_INDEX;
}

void CGlobalRecords::DeleteLabelSST(const label_t& labeldef)
{
	cLabel_Vect_Itor_t label_end = m_Labels.end();
	for(Label_Vect_Itor_t lbl = m_Labels.begin(); lbl != label_end; ++lbl) {
		if(&labeldef == (*lbl)) {
			m_Labels.erase(lbl);
			break;
		}
	}
}

bool CGlobalRecords::SetColor(uint8 r, uint8 g, uint8 b, uint8 idx)
{
	return m_palette.setColor(r, g, b, idx);
}

xf_t* CGlobalRecords::GetDefaultXF() const
{
	return defaultXF;
}

font_t* CGlobalRecords::fontdup(uint8 fontnum) const
{
	return font_t::fontDup(m_DefaultFonts[fontnum]);
}

void CGlobalRecords::str16toascii(const u16string& str1, std::string& str2)
{
	u16string::const_iterator cBegin, cEnd;

	str2.clear();

	size_t len = str1.length();
	str2.reserve(len);


	cBegin	= str1.begin();
	cEnd	= str1.end();

	while(cBegin != cEnd) {
		uint16 c = *cBegin++;

		if (c > 0x7F) {
			c = '?';
		}
		str2.push_back((char)c);
	}
}

#if defined(HAVE_WORKING_ICONV)

#include <errno.h>

void CGlobalRecords::wide2str16(const ustring& str1, u16string& str2)
{
	size_t					resultSize, inbytesleft, outbytesleft;
	const uint8_t			*inbuf;
	iconv_t					cd;
	uint16			*outbuf;
	uint16			*origOutbuf = NULL;

	cd = iconv_open(UCS_2_INTERNAL, iconv_code.c_str());
	// user may have changed the conversion since the workbook was opened
	XL_ASSERT(!(cd == (iconv_t)(-1)));

	if(cd != (iconv_t)(-1)) {
		inbytesleft		= str1.size() * sizeof(unichar_t);
		outbytesleft	= str1.size() * sizeof(uint16);

		inbuf		= (uint8_t *)str1.data();
		origOutbuf	= (uint16 *)calloc(outbytesleft, 1);
		outbuf		= origOutbuf;

		resultSize = iconv(cd, (char **)&inbuf, &inbytesleft, (char **)&outbuf, &outbytesleft);
		iconv_close(cd);
	} else {
		resultSize = (size_t)-1;
	}

	if(resultSize == (size_t)-1) {
		str2 = (xchar16_t *)convFail;
	} else {
		str2.assign((xchar16_t *)origOutbuf, (size_t)(outbuf - origOutbuf));
	}
	free((void *)origOutbuf);
}

#else

void CGlobalRecords::wide2str16(const ustring& str1, u16string& str2)
{
	ustring::const_iterator	cBegin, cEnd;
	size_t len;

	str2.clear();

	len = str1.length();
	str2.reserve(len);

	cBegin	= str1.begin();
	cEnd	= str1.end();

	while(cBegin != cEnd) {
		str2.push_back((uint16)*cBegin++);
	}
	XL_ASSERT(str2.length() == str1.length());
}

#endif

#if defined(HAVE_WORKING_ICONV)

void CGlobalRecords::char2str16(const string& str1, u16string& str2)
{
	string::const_iterator cBegin, cEnd;
	size_t len;

	str2.clear();

	// test for UTF
	cBegin	= str1.begin();
	cEnd	= str1.end();

	uint8 c = 0;
	while(cBegin != cEnd) {
		c |= *cBegin++;
	}

	if(c & 0x80) {
		const char				*inbuf;
		iconv_t	cd;
		uint16			*outbuf, *origOutbuf;
		size_t resultSize, inbytesleft, outbytesleft;

		cd = iconv_open(UCS_2_INTERNAL, "UTF-8");
		XL_ASSERT(cd != (iconv_t)(-1));

		inbytesleft		= str1.size();
		outbytesleft	= inbytesleft * sizeof(uint16);

		inbuf		= str1.c_str();
		origOutbuf	= (uint16 *)calloc(outbytesleft, 1);
		outbuf		= origOutbuf;

		resultSize = iconv(cd, (char **)&inbuf, &inbytesleft, (char **)&outbuf, &outbytesleft);
		iconv_close(cd);

		if(resultSize == (size_t)-1) {
			str2 = (xchar16_t *)convFail;
		} else {
			str2.assign((xchar16_t *)origOutbuf, (size_t)(outbuf - origOutbuf));
		}
		free((void *)origOutbuf);
	} else {
		len = str1.length();
		str2.reserve(len);

		cBegin	= str1.begin();
		cEnd	= str1.end();

		while(cBegin != cEnd) {
			str2.push_back((uint16)*cBegin++);
		}
		XL_ASSERT(str2.length() == str1.length());
	}
}

#else

void CGlobalRecords::char2str16(const std::string& str1, u16string& str2)
{
	std::string::const_iterator cBegin, cEnd;
	size_t len;

	str2.clear();

	len = str1.length();
	str2.reserve(len);

	cBegin	= str1.begin();
	cEnd	= str1.end();

	while(cBegin != cEnd) {
		str2.push_back((uint16)*cBegin++);
	}
	XL_ASSERT(str2.length() == str1.length());
}

#endif

bool CGlobalRecords::IsASCII(const std::string& str)
{
	std::string::const_iterator cBegin, cEnd;

	cBegin	= str.begin();
	cEnd	= str.end();

	uint16 c = 0;

	while(cBegin != cEnd) {
		c |= *cBegin++;
	}

	return c <= 0x7F;
}

bool CGlobalRecords::IsASCII(const u16string& str)
{
	u16string::const_iterator cBegin, cEnd;

	cBegin	= str.begin();
	cEnd	= str.end();

	uint16 c = 0;

	while(cBegin != cEnd) {
		c |= *cBegin++;
	}

	return c <= 0x7F;
}

xf_t* CGlobalRecords::findXF(xf_t* xff)
{
  XF_Vect_Itor_t xfIt;
  xf_t* xf2;

  for (xfIt = m_XFs.begin(); xfIt != m_XFs.end(); xfIt++) {
    xf2 = *xfIt;
    if (*xf2 == *xff)
      break;
  }

  if (xfIt != m_XFs.end()) {
    if (*xfIt != xff) {
      XF_Vect_Itor_t xfIt2;

      xfIt2 = m_XFs.end();
      xfIt2--;
      delete *xfIt2;

      m_XFs.pop_back();
      xfIndex--;
    }
    return *xfIt;
  }

  return xff;
}

#if defined(_MSC_VER)
#undef new
void *operator_new_dbg(size_t count, const char *f, int l)
{
	return operator new(count, _CLIENT_BLOCK, f, l);
}

#endif
