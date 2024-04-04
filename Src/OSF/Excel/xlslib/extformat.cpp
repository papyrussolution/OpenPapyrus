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
/*
 **********************************
 * xf_t class implementation
 **********************************
 */

const xf_init_t xf_t::xfiInit;

const uint8 xf_t::HALIGN_OPTIONS_TABLE[_NUM_HALIGN_OPTIONS] =
{
	XF_HALIGN_GENERAL,
	XF_HALIGN_LEFT,
	XF_HALIGN_CENTER,
	XF_HALIGN_RIGHT,
	XF_HALIGN_FILL,
	XF_HALIGN_JUSTIFY,
	XF_HALIGN_CENTERACCROSS
};

const uint8 xf_t::VALIGN_OPTIONS_TABLE[_NUM_VALIGN_OPTIONS] =
{
	XF_VALIGN_TOP,
	XF_VALIGN_CENTER,
	XF_VALIGN_BOTTOM,
	XF_VALIGN_JUSTIFY
};

const uint8 xf_t::INDENT_OPTIONS_TABLE[_NUM_INDENT_OPTIONS] =
{
	XF_INDENT_0,
	XF_INDENT_1,
	XF_INDENT_2,
	XF_INDENT_3,
	XF_INDENT_4,
	XF_INDENT_5,
	XF_INDENT_6,
	XF_INDENT_7,
	XF_INDENT_8,
	XF_INDENT_9,
	XF_INDENT_10,
	XF_INDENT_11,
	XF_INDENT_12,
	XF_INDENT_13,
	XF_INDENT_14,
	XF_INDENT_15,
	XF_INDENT_SHRINK2FIT,
	XF_INDENT_L2R,
	XF_INDENT_R2L
};

const uint8 xf_t::TXTORI_OPTIONS_TABLE[_NUM_TXTORI_OPTIONS] =
{
	XF_ORI_NONE,
	XF_ORI_TOPBOTTOMTXT,
	XF_ORI_90NOCLOCKTXT,
	XF_ORI_90CLOCKTXT
};

const uint8 xf_t::COLOR_OPTIONS_TABLE[_NUM_COLOR_NAMES] =
{
	0,  // Black as used in the default fonts
	COLOR_CODE_BLACK,
	COLOR_CODE_BROWN,
	COLOR_CODE_OLIVE_GREEN,
	COLOR_CODE_DARK_GREEN,
	COLOR_CODE_DARK_TEAL,
	COLOR_CODE_DARK_BLUE,
	COLOR_CODE_INDIGO,
	COLOR_CODE_GRAY80,

	COLOR_CODE_DARK_RED,
	COLOR_CODE_ORANGE,
	COLOR_CODE_DARK_YELLOW,
	COLOR_CODE_GREEN,
	COLOR_CODE_TEAL,
	COLOR_CODE_BLUE,
	COLOR_CODE_BLUE_GRAY,
	COLOR_CODE_GRAY50,

	COLOR_CODE_RED,
	COLOR_CODE_LIGHT_ORANGE,
	COLOR_CODE_LIME,
	COLOR_CODE_SEA_GREEN,
	COLOR_CODE_AQUA,
	COLOR_CODE_LIGHT_BLUE,
	COLOR_CODE_VIOLET,
	COLOR_CODE_GRAY40,

	COLOR_CODE_PINK,
	COLOR_CODE_GOLD,
	COLOR_CODE_YELLOW,
	COLOR_CODE_BRIGHT_GREEN,
	COLOR_CODE_TURQUOISE,
	COLOR_CODE_SKY_BLUE,
	COLOR_CODE_PLUM,
	COLOR_CODE_GRAY25,

	COLOR_CODE_ROSE,
	COLOR_CODE_TAN,
	COLOR_CODE_LIGHT_YELLOW,
	COLOR_CODE_LIGHT_GREEN,
	COLOR_CODE_LIGHT_TURQUOISE,
	COLOR_CODE_PALEBLUE,
	COLOR_CODE_LAVENDER,
	COLOR_CODE_WHITE,

	COLOR_CODE_PERIWINKLE,
	COLOR_CODE_DARK_BLUE2,
	COLOR_CODE_PLUM2,
	COLOR_CODE_PINK2,
	COLOR_CODE_IVORY,
	COLOR_CODE_YELLOW2,
	COLOR_CODE_LIGHT_TURQUOISE2,
	COLOR_CODE_TURQUOISE2,

	COLOR_CODE_DARK_PURPLE,
	COLOR_CODE_VIOLET2,
	COLOR_CODE_CORAL,
	COLOR_CODE_DARK_RED2,
	COLOR_CODE_OCEAN_BLUE,
	COLOR_CODE_TEAL2,
	COLOR_CODE_ICE_BLUE,
	COLOR_CODE_BLUE2,

	COLOR_CODE_SYS_WIND_FG,
	COLOR_CODE_SYS_WIND_BG
};

const uint8 xf_t::FILL_OPTIONS_TABLE[_NUM_FILL_OPTIONS] =
{
	XF_FILL_NONE,
	XF_FILL_SOLID,
	XF_FILL_ATEN75,
	XF_FILL_ATEN50,
	XF_FILL_ATEN25,
	XF_FILL_ATEN12,
	XF_FILL_ATEN06,
	XF_FILL_HORIZ_LIN,
	XF_FILL_VERTICAL_LIN,
	XF_FILL_DIAG,
	XF_FILL_INV_DIAG,
	XF_FILL_INTER_DIAG,
	XF_FILL_DIAG_THICK_INTER,
	XF_FILL_HORIZ_LINES_THIN,
	XF_FILL_VERTICAL_LINES_THIN,
	XF_FILL_DIAG_THIN,
	XF_FILL_INV_DIAG_THIN,
	XF_FILL_HORIZ_INT_THIN,
	XF_FILL_HORIZ_INTER_THICK
};

const uint8 xf_t::BORDERSTYLE_OPTIONS_TABLE[_NUM_BORDER_STYLES] =
{
	XF_BRDOPTION_NONE,
	XF_BRDOPTION_THIN,
	XF_BRDOPTION_MEDIUM,
	XF_BRDOPTION_DASHED,
	XF_BRDOPTION_DOTTED,
	XF_BRDOPTION_THICK,
	XF_BRDOPTION_DOUBLE,
	XF_BRDOPTION_HAIR
};

/*
 ******************************************************
 * class xf_init_t: convenience structure to init an xf_t
 ******************************************************
 */
xf_init_t::xf_init_t() :
	font(NULL),
	formatIndex(FMTCODE_GENERAL),

	locked(XF_LOCKED),
	hidden(XF_NO_HIDDEN),
	wrap(XF_NO_WRAPPED),

	halign(HALIGN_GENERAL),
	valign(VALIGN_BOTTOM),

	indent(INDENT_0),
	txt_orient(ORI_NONE),

	fillstyle(FILL_NONE),
	fill_fgcolor(CLR_SYS_WIND_FG),
	fill_bgcolor(CLR_SYS_WIND_BG),

	border_style(),
	border_color()
{
	for(int i = 0; i<_NUM_BORDERS; ++i) {
		border_style[i] = BORDER_NONE;
		border_color[i] = ORIG_COLOR_BLACK;
	}
}

xf_init_t::~xf_init_t()
{
}

xf_init_t::xf_init_t(const xf_init_t &that) :
	font(that.font),
	formatIndex(that.formatIndex),

	locked(that.locked),
	hidden(that.hidden),
	wrap(that.wrap),

	halign(that.halign),
	valign(that.valign),

	indent(that.indent),
	txt_orient(that.txt_orient),

	fillstyle(that.fillstyle),
	fill_fgcolor(that.fill_fgcolor),
	fill_bgcolor(that.fill_bgcolor),

	border_style(),
	border_color()
{
	for(int i = 0; i<_NUM_BORDERS; ++i) {
		border_style[i] = that.border_style[i];
		border_color[i] = that.border_color[i];
	}
}

xf_init_t& xf_init_t::operator=(const xf_init_t& right)
{
	(void)right;    // stop warning
	throw std::string("Should never have invoked the xf_init_t copy operator!");
}

bool xf_init_t::operator==(const xf_init_t& right)
{
	// used by "range" in doing mass changes. Try to arrange so most
	// likely failures occur early

	if(font != right.font) {
		return false;
	}

	if(fill_fgcolor != right.fill_fgcolor) {
		return false;
	}
	if(fill_bgcolor != right.fill_bgcolor) {
		return false;
	}
	if(fillstyle != right.fillstyle) {
		return false;
	}

	for(int i = 0; i<_NUM_BORDERS; ++i) {
		if(border_style[i] != right.border_style[i]) {
			return false;
		}
		if(border_color[i] != right.border_color[i]) {
			return false;
		}
	}

	if(halign != right.halign) {
		return false;
	}
	if(valign != right.valign) {
		return false;
	}
	if(indent != right.indent) {
		return false;
	}
	if(txt_orient != right.txt_orient) {
		return false;
	}

	if(locked != right.locked) {
		return false;
	}
	if(hidden != right.hidden) {
		return false;
	}
	if(wrap != right.wrap) {
		return false;
	}

	return true;
}

const xf_init_t &xf_t::GetDefaultXFshadow(CGlobalRecords& gRecords, bool userXF, bool isCell)
{
	(void)userXF; // remove warning
	if(!isCell) {
		const xf_t * xf = gRecords.GetDefaultXF();
		if(xf) {
			return xf->GetXFshadow();
		}
	}
	return xfiInit;
}

/*
 ******************************************
 * GlobalRec, xftDup, and range(userXF==no)
 ******************************************
 */
xf_t::xf_t(xlslib_core::CGlobalRecords& gRecords, bool userXF, bool isCell, bool isMasterXF) :
	m_GlobalRecords(gRecords),
	xfi(GetDefaultXFshadow(gRecords, userXF, isCell)),
	m_usage_counter(0),
	index(0),
	parent_index(0),

	formatIndex(FMTCODE_GENERAL),
	font(NULL),
	format(NULL),
	halign(XF_HALIGN_GENERAL),
	valign(XF_VALIGN_BOTTOM),
	indent(XF_INDENT_0),
	txt_orient(XF_ORI_NONE),
	fillstyle(XF_FILL_NONE),
	fill_fgcolor(ORIG_COLOR_BLACK),
	fill_bgcolor(ORIG_COLOR_BLACK),

	locked(XF_NO_LOCKED),
	hidden(XF_NO_HIDDEN),
	wrap(XF_NO_WRAPPED),
	is_cell(isCell),
	is_userXF(userXF),

	border_style(),
	border_color(),

	flags(0)
{
	//	SetCellMode(isCell);

	// Flags have different meanings for Cell or Style formats
	flags = (isCell || isMasterXF) ? 0 : XF_ALIGN_ALL;

	// Set a default value for class members
	SetFormatIndex(xfi.formatIndex);
	SetHAlign(xfi.halign);
	SetVAlign(xfi.valign);
	SetIndent(xfi.indent);
	SetTxtOrientation(xfi.txt_orient);
	SetFillFGColor(xfi.fill_fgcolor);
	SetFillBGColor(xfi.fill_bgcolor);
	SetFillStyle(xfi.fillstyle);

	SetLocked(xfi.locked);
	SetHidden(xfi.hidden);
	SetWrap(xfi.wrap);

	for(int i = 0; i<_NUM_BORDERS; ++i) {
		SetBorderStyle((border_side_t)i, xfi.border_style[i]);
		SetBorderColor((border_side_t)i, xfi.border_color[i]);
	}

	if(is_userXF) {
		m_GlobalRecords.AddXFormat(this);
	}
}

/*
 **********************************
 * Only for use by users
 **********************************
 */
xf_t::xf_t(const xf_t& orig) :
	m_GlobalRecords(orig.m_GlobalRecords),
	xfi(orig.xfi),
	m_usage_counter(0),
	index(0),
	parent_index(0),

	formatIndex(orig.formatIndex),
	font(NULL),                         // yes, need this as SetFont below depends on a set value
	format(NULL),
	halign(orig.halign),
	valign(orig.valign),
	indent(orig.indent),
	txt_orient(orig.txt_orient),
	fillstyle(orig.fillstyle),
	fill_fgcolor(orig.fill_fgcolor),
	fill_bgcolor(orig.fill_bgcolor),

	locked(orig.locked),
	hidden(orig.hidden),
	wrap(orig.wrap),
	is_cell(true),
	is_userXF(true),

	border_style(),
	border_color(),

	flags(orig.flags)
{
	SetFont(orig.font); // side effects
	if(orig.format) {
		SetFormat(orig.format); // side effects
	}

	for(int i = 0; i<_NUM_BORDERS; ++i) {
		border_style[i] = orig.border_style[i];
		border_color[i] = orig.border_color[i];
	}

	m_GlobalRecords.AddXFormat(this);
}

/*
 **********************************
 * Constructor using pre-set values: range
 **********************************
 */
xf_t::xf_t(CGlobalRecords& gRecords, const xf_init_t& xfinit) :
	m_GlobalRecords(gRecords),
	xfi(xfinit),
	m_usage_counter(0),
	index(0),
	parent_index(0),

	formatIndex(FMTCODE_GENERAL),
	font(NULL),
	format(NULL),
	halign(XF_HALIGN_GENERAL),
	valign(XF_VALIGN_BOTTOM),
	indent(XF_INDENT_0),
	txt_orient(XF_ORI_NONE),
	fillstyle(XF_FILL_NONE),
	fill_fgcolor(ORIG_COLOR_BLACK),
	fill_bgcolor(ORIG_COLOR_BLACK),

	locked(XF_NO_LOCKED),
	hidden(XF_NO_HIDDEN),
	wrap(XF_NO_WRAPPED),
	is_cell(true),
	is_userXF(true),

	border_style(),
	border_color(),

	flags(0)
{
	SetFont(xfinit.font);   // side effects
	//SetFormat(xfinit.format);	// side effects
	SetFormatIndex(xfinit.formatIndex);

	SetHAlign(xfinit.halign);
	SetVAlign(xfinit.valign);
	SetIndent(xfinit.indent);
	SetTxtOrientation(xfinit.txt_orient);
	SetFillFGColor(xfinit.fill_fgcolor);
	SetFillBGColor(xfinit.fill_bgcolor);
	SetFillStyle(xfinit.fillstyle);
	SetLocked(xfinit.locked);
	SetHidden(xfinit.hidden);
	SetWrap(xfinit.wrap);

	for(int i = 0; i<_NUM_BORDERS; ++i) {
		SetBorderStyle((border_side_t)i, xfinit.border_style[i]);
		SetBorderColor((border_side_t)i, xfinit.border_color[i]);
	}
	m_GlobalRecords.AddXFormat(this);
}

xf_t::~xf_t()
{
}

void xf_t::SetFlag(uint8 flag)
{
	if(IsCell()) {
#if 0
		if((flags & flag) == 0) {
			// The flag forces all these fields to be defined, so init them to defaults
			switch(flag) {
				case XF_ALIGN_ATRALC:
				    XL_ASSERT(valign == XF_VALIGN_BOTTOM); /* is already set as default through xfinit() */
				    break;
				case XF_ALIGN_ATRPAT:
				    XL_ASSERT(fill_fgcolor == COLOR_CODE_SYS_WIND_FG);
				    XL_ASSERT(fill_bgcolor == COLOR_CODE_SYS_WIND_BG);
				    break;
				case XF_ALIGN_ATRBDR:
				    XL_ASSERT(valign == XF_VALIGN_BOTTOM);
				    break;
			}
		}
#endif
		// Cells indicate that a characteristic is not equal
		//  from its parent with the flag set.
		flags |= flag;
	}
	else {
		// Styles indicate that a characteristic is
		// being implemented with the flag cleared.
		flags &= ~flag;
	}
}

void xf_t::ClearFlag(uint8 flag)
{
	if(!IsCell()) {
		// Cells indicate that a characteristic is not equal
		//  from its parent with the flag set.
		flags |= flag;
	}
	else {
		// Styles indicate that a characteristic is
		// being implemented with the flag cleared.
		flags &= ~flag;
	}
}

uint8 xf_t::GetFlags() const
{
	return flags;
}

void xf_t::AtuneXF(void)
{
	if(font != xfi.font) {
		SetFlag(XF_ALIGN_ATRFONT);
	}
	if(formatIndex != xfi.formatIndex) {
		SetFlag(XF_ALIGN_ATRNUM);
	}

	for(int side = 0; side < _NUM_BORDERS; ++side) {
		if(border_style[side] != xfi.border_style[side]) {
			SetFlag(XF_ALIGN_ATRBDR);
		}
		if(border_color[side] != xfi.border_color[side]) {
			SetFlag(XF_ALIGN_ATRBDR);
		}
	}

	if(halign != xfi.halign) {
		SetFlag(XF_ALIGN_ATRALC);
	}
	if(valign != xfi.valign) {
		SetFlag(XF_ALIGN_ATRALC);
	}
	if(indent != xfi.indent) {
		SetFlag(XF_ALIGN_ATRALC);
	}
	if(txt_orient != xfi.txt_orient) {
		SetFlag(XF_ALIGN_ATRALC);
	}
	if(fill_fgcolor != xfi.fill_fgcolor) {
		SetFlag(XF_ALIGN_ATRPAT);
	}
	if(fill_bgcolor != xfi.fill_bgcolor) {
		SetFlag(XF_ALIGN_ATRPAT);
	}
	if(fillstyle != xfi.fillstyle) {
		SetFlag(XF_ALIGN_ATRPAT);
	}
	if(locked != xfi.locked) {
		SetFlag(XF_ALIGN_ATRPROT);
	}
	if(hidden != xfi.hidden) {
		SetFlag(XF_ALIGN_ATRPROT);
	}
	if(wrap != xfi.wrap) {
		SetFlag(XF_ALIGN_ATRALC);
	}
}

void xf_t::MarkUsed(void)
{
	m_usage_counter++;
}

void xf_t::UnMarkUsed(void)
{
	if(m_usage_counter) {
		m_usage_counter--;
	}

	if(m_usage_counter == 0) {
		if(font && font->Usage()) {
			font->UnMarkUsed();
		}
		if(format && format->Usage()) {
			format->UnMarkUsed();
		}
	}
}

uint32 xf_t::Usage(void) const
{
	return m_usage_counter;
}

void xf_t::SetParent(const xf_t* parent)
{
	if(parent) {
		xfi = parent->xfi;

		// now make sure all the flags are properly set, given that we probably have altered xfi 'shadow base' by now.
		AtuneXF();
	}
}

uint16 xf_t::GetParentIndex(void) const
{
	/* TBD: derive XF's from other XF's, including the standard ones */
	return 0;
}

const xf_t* xf_t::GetParent(void) const
{
	return NULL;
}

void xf_t::SetFont(font_t* newfont)
{
	// Set the related flag
	if(newfont != xf_t::xfiInit.font) {
		SetFlag(XF_ALIGN_ATRFONT);
	}
	if(font) {
		font->UnMarkUsed();
	}

	font = newfont;
	if(font) {
		font->MarkUsed();
	}
}

font_t* xf_t::GetFont(void) const
{
	return font;
}

uint16 xf_t::GetFontIndex(void) const
{
	if(font != NULL) {
		return font->GetIndex();
	}
	else {
		return 0;
	}
}

void xf_t::SetFormatIndex(uint16 formatidx)
{
	// Set the related flag.
	if(formatidx != xf_t::xfiInit.formatIndex) {
		SetFlag(XF_ALIGN_ATRNUM);
	}
	formatIndex = formatidx;
	format = NULL;
}

void xf_t::SetFormat(format_number_t fmt)
{
	uint16 idx;

	if(fmt > FMT_TEXT) {
		fmt = FMT_GENERAL;
	}
	idx = format_t::format2index(fmt);

	// Set the related flag.
	if(idx != xf_t::xfiInit.formatIndex) {
		SetFlag(XF_ALIGN_ATRNUM);
	}
	formatIndex = idx;
	format = NULL;
}

void xf_t::SetFormat(format_t * fmt)
{
	if(!fmt) {
		return;
	}

	if(format) {
		format->UnMarkUsed();
	}

	uint16 idx = fmt->GetIndex();

	// Set the related flag.
	if(idx != xf_t::xfiInit.formatIndex) {
		SetFlag(XF_ALIGN_ATRNUM);
	}

	formatIndex = idx;
	format = fmt;
	format->MarkUsed();
	//cerr << "ndx=" << formatIndex << endl << flush;
}

uint16 xf_t::GetFormatIndex(void) const
{
	return formatIndex;
}

format_number_t xf_t::GetFormat(void) const
{
	int frmt;
	for(frmt = (int)FMT_GENERAL; frmt<=(int)FMT_TEXT; ++frmt) {
		if(formatIndex == format_t::format2index((format_number_t)frmt)) {
			return (format_number_t)frmt;
		}
	}
	return FMT_GENERAL; // should never get here...
}

std::string xf_t::Description() const
{
	basic_ostringstream <char> buf;

	buf << "-----------------------------------------" << endl;

	buf << "      INDEX: " << index << " parent=" << parent_index << " usage=" <<  m_usage_counter << endl;
	buf << "       Font: " << hex << font->GetName() << dec << endl; // @sobolev font-->font->GetName()
	buf << "  FormatIdx: " << formatIndex << endl;
	buf << "      Align: " << "h=" << hex << (int)halign << " v=" << (int)valign << " indent=" << (int)indent << " orient=" << (int)txt_orient << dec << endl;
	buf << "       Fill: " << "fgClr=" << (int)fill_fgcolor << " bgClr=" << (int)fill_bgcolor << " style=" << (int)fillstyle << dec << endl;
	buf << "  TopBorder: " << "style=" << hex << (int)border_style[BORDER_TOP] << " color=" << (int)border_color[BORDER_TOP] << dec << endl;
	buf << "  BotBorder: " << "style=" << hex << (int)border_style[BORDER_BOTTOM] << " color=" << (int)border_color[BORDER_BOTTOM] << dec << endl;
	buf << " LeftBorder: " << "style=" << hex << (int)border_style[BORDER_LEFT] << " color=" << (int)border_color[BORDER_LEFT] << dec << endl;
	buf << "RightBorder: " << "style=" << hex << (int)border_style[BORDER_RIGHT] << " color=" << (int)border_color[BORDER_RIGHT] << dec << endl;
	buf << "      Logic: " << "locked=" << locked << " hidden=" << hidden << " wrap=" << wrap << " isCell=" << is_cell << " isUserXF=" << is_userXF << endl;
	buf << "      FLAGS: " << hex << (int)flags << dec << endl;

	return buf.str();
}

/*
 **********************************
 **********************************
 */

/* Cell option wrappers*/
void xf_t::SetBorderStyle(border_side_t side, border_style_t style)
{
	XL_ASSERT(side >= 0);
	XL_ASSERT(side < _NUM_BORDERS);
	XL_ASSERT(style >= 0);
	XL_ASSERT(style < _NUM_BORDER_STYLES);
	border_style[side] = BORDERSTYLE_OPTIONS_TABLE[style];

	if(border_style[side] != xf_t::xfiInit.border_style[side]) {
		SetFlag(XF_ALIGN_ATRBDR);
	}
	// fix up XF record for Excel, who does not like color == 0 when the style is not NONE!
	if(IsCell() && border_color[side] == 0) {
		border_color[side] = COLOR_OPTIONS_TABLE[CLR_SYS_WIND_FG];
	}
}

void xf_t::SetBorderColor(border_side_t side, color_name_t color)
{
	XL_ASSERT(side >= 0);
	XL_ASSERT(side < _NUM_BORDERS);
	XL_ASSERT(color >= 0);
	XL_ASSERT(color < _NUM_COLOR_NAMES);
	border_color[side] = COLOR_OPTIONS_TABLE[color];

	if(border_color[side] != xf_t::xfiInit.border_color[side]) {
		SetFlag(XF_ALIGN_ATRBDR);
	}
}

void xf_t::SetBorderColor(border_side_t side, uint8 color)
{
	XL_ASSERT(side >= 0);
	XL_ASSERT(side < _NUM_BORDERS);
	border_color[side] = color;

	if(border_color[side] != xf_t::xfiInit.border_color[side]) {
		SetFlag(XF_ALIGN_ATRBDR);
	}
}

/*
 **********************************
 **********************************
 */
uint8 xf_t::GetBorderStyle(border_side_t side) const
{
	XL_ASSERT(side >= 0);
	XL_ASSERT(side < _NUM_BORDERS);
	return border_style[side];
}

/*
 **********************************
 **********************************
 */
uint16 xf_t::GetBorderColorIdx(border_side_t side) const
{
	XL_ASSERT(side >= 0);
	XL_ASSERT(side < _NUM_BORDERS);
	return border_color[side];
}

xf_t& xf_t::operator=(const xf_t& right)
{
	if(&right != this) {
		//m_GlobalRecords = right.GetGlobalRecords();
		xfi = right.xfi;

		index           = right.index; // or -1 or 0 ?
		parent_index = right.parent_index;

		font            = right.font;
		formatIndex = right.formatIndex;
		format      = right.format;

		halign = right.halign;
		valign = right.valign;
		indent = right.indent;

		txt_orient              = right.txt_orient;

		fill_fgcolor    = right.fill_fgcolor;
		fill_bgcolor    = right.fill_bgcolor;
		fillstyle               = right.fillstyle;

		locked          = right.locked;
		hidden          = right.hidden;
		wrap            = right.wrap;
		is_cell         = right.is_cell;
		is_userXF       = right.is_userXF;

		flags = right.flags;

		for(int i = 0; i<_NUM_BORDERS; ++i) {
			border_style[i] = right.border_style[i];
			border_color[i] = right.border_color[i];
		}
	}

	return *this;
}

bool xf_t::operator==(const xf_t& right)
{
	if(formatIndex != right.formatIndex)
		return false;
	if(font != right.font)
		return false;
	if(format != right.format)
		return false;

	if(halign != right.halign)
		return false;
	if(valign != right.valign)
		return false;
	if(indent != right.indent)
		return false;
	if(txt_orient != right.txt_orient)
		return false;

	if(fillstyle != right.fillstyle)
		return false;
	if(fill_fgcolor != right.fill_fgcolor)
		return false;
	if(fill_bgcolor != right.fill_bgcolor)
		return false;

	if(locked != right.locked)
		return false;
	if(hidden != right.hidden)
		return false;
	if(wrap != right.wrap)
		return false;
	if(is_cell != right.is_cell)
		return false;
	if(is_userXF != right.is_userXF)
		return false;

	for(int i = 0; i<_NUM_BORDERS; i++) {
		if(border_style[i] != right.border_style[i])
			return false;
		if(border_color[i] != right.border_color[i])
			return false;
	}

	if(flags != right.flags)
		return false;

	return true;
}

/* Horizontal Align option wrappers*/
void xf_t::SetHAlign(halign_option_t ha_option)
{
	// Set the related flag.
	if(ha_option != xf_t::xfiInit.halign) {
		SetFlag(XF_ALIGN_ATRALC);
	}

	XL_ASSERT(ha_option >= 0);
	XL_ASSERT(ha_option < _NUM_HALIGN_OPTIONS);
	halign = xf_t::HALIGN_OPTIONS_TABLE[ha_option];
}

uint8 xf_t::GetHAlign(void) const
{
	return halign;
}

/* Vertical Align option wrappers*/
void xf_t::SetVAlign(valign_option_t va_option)
{
	// Set the related flag.
	if(va_option != xf_t::xfiInit.valign) {
		SetFlag(XF_ALIGN_ATRALC);
	}

	XL_ASSERT(va_option >= 0);
	XL_ASSERT(va_option < _NUM_VALIGN_OPTIONS);
	valign = xf_t::VALIGN_OPTIONS_TABLE[va_option];
}

uint8 xf_t::GetVAlign(void) const
{
	return valign;
}

void xf_t::SetIndent(indent_option_t indent_option)
{
	// Set the related flag.
	if(indent_option != xf_t::xfiInit.indent) {
		SetFlag(XF_ALIGN_ATRALC);
	}

	XL_ASSERT(indent_option >= 0);
	XL_ASSERT(indent_option < _NUM_INDENT_OPTIONS);
	indent = xf_t::INDENT_OPTIONS_TABLE[indent_option];
}

uint8 xf_t::GetIndent(void) const
{
	return indent;
}

/* Text orientation option wrappers*/
void xf_t::SetTxtOrientation(txtori_option_t ori_option)
{
	// Set the related flag.
	if(ori_option != xf_t::xfiInit.txt_orient) {
		SetFlag(XF_ALIGN_ATRALC);
	}

	XL_ASSERT(ori_option >= 0);
	XL_ASSERT(ori_option < _NUM_TXTORI_OPTIONS);
	txt_orient = xf_t::TXTORI_OPTIONS_TABLE[ori_option];
}

uint8 xf_t::GetTxtOrientation(void) const
{
	return txt_orient;
}

/* Fill Foreground color option wrappers*/
void xf_t::SetFillFGColor(color_name_t color)
{
	// Set the related flag.
	if(color != xf_t::xfiInit.fill_fgcolor) {
		SetFlag(XF_ALIGN_ATRPAT);
	}

	XL_ASSERT(color >= 0);
	XL_ASSERT(color < _NUM_COLOR_NAMES);
	fill_fgcolor = xf_t::COLOR_OPTIONS_TABLE[color];
}

void xf_t::SetFillFGColor(uint8 color)
{
	SetFlag(XF_ALIGN_ATRPAT);

	fill_fgcolor = color;
}

uint16 xf_t::GetFillFGColorIdx(void) const
{
	return fill_fgcolor;
}

/* Fill Background color option wrappers*/
void xf_t::SetFillBGColor(color_name_t color)
{
	// Set the related flag.
	if(color != xf_t::xfiInit.fill_bgcolor) {
		SetFlag(XF_ALIGN_ATRPAT);
	}

	XL_ASSERT(color >= 0);
	XL_ASSERT(color < _NUM_COLOR_NAMES);
	fill_bgcolor = xf_t::COLOR_OPTIONS_TABLE[color];
}

void xf_t::SetFillBGColor(uint8 color)
{
	SetFlag(XF_ALIGN_ATRPAT);

	fill_bgcolor = color;
}

uint16 xf_t::GetFillBGColorIdx(void) const
{
	return fill_bgcolor;
}

/* Fill Style option wrappers*/
void xf_t::SetFillStyle(fill_option_t fill)
{
	// Set the related flag.
	if(fill != xf_t::xfiInit.fillstyle) {
		SetFlag(XF_ALIGN_ATRPAT);
	}

	XL_ASSERT(fill >= 0);
	XL_ASSERT(fill < _NUM_FILL_OPTIONS);
	fillstyle = xf_t::FILL_OPTIONS_TABLE[fill];
}

uint8 xf_t::GetFillStyle(void) const
{
	return fillstyle;
}

/* Locked option wrappers*/
void xf_t::SetLocked(bool locked_opt)
{
	// Set the related flag.
	if(locked_opt != xf_t::xfiInit.locked) {
		SetFlag(XF_ALIGN_ATRPROT);
	}

	locked = locked_opt;
}

bool xf_t::IsLocked(void) const
{
	return locked;
}

/* Hidden option wrappers*/
void xf_t::SetHidden(bool hidden_opt)
{
	// Set the related flag.
	if(hidden_opt != xf_t::xfiInit.hidden) {
		SetFlag(XF_ALIGN_ATRPROT);
	}

	hidden = hidden_opt;
}

bool xf_t::IsHidden(void) const
{
	return hidden;
}

/* Wrap option wrappers*/
void xf_t::SetWrap(bool wrap_opt)
{
	// Set the related flag.
	if(wrap_opt != xf_t::xfiInit.wrap) {
		SetFlag(XF_ALIGN_ATRALC);
	}

	wrap = wrap_opt;
}

bool xf_t::IsWrap(void) const
{
	return wrap;
}

/* Cell option wrappers*/
void xf_t::SetCellMode(bool cellmode)
{
	is_cell = cellmode;
}

bool xf_t::IsCell(void) const
{
	return is_cell;
}

CExtFormat::CExtFormat(CDataStorage &datastore, const xf_t* xfdef) :
	CRecord(datastore)
{
	bool is_cell = xfdef->IsCell();
	//cerr << "CExtFormat:" << endl << xfdef->Description() << endl;

	SetRecordType(RECTYPE_XF);
	InitDummy(is_cell);
	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);

	SetFontIndex(xfdef->GetFontIndex());
	SetFormatIndex(xfdef->GetFormatIndex());
	SetHorizAlign(xfdef->GetHAlign());
	SetVertAlign(xfdef->GetVAlign());
	SetIndent(xfdef->GetIndent());
	SetTxtOrientation(xfdef->GetTxtOrientation());

	SetFGColorIndex(xfdef->GetFillFGColorIdx());
	SetBGColorIndex(xfdef->GetFillBGColorIdx());
	SetFillPattern(xfdef->GetFillStyle());

	SetLocked(xfdef->IsLocked());
	SetHidden(xfdef->IsHidden());
	SetWrap(xfdef->IsWrap());

	SetBorder(BORDER_BOTTOM, xfdef->GetBorderStyle(BORDER_BOTTOM),
	    xfdef->GetBorderColorIdx(BORDER_BOTTOM));
	SetBorder(BORDER_TOP, xfdef->GetBorderStyle(BORDER_TOP),
	    xfdef->GetBorderColorIdx(BORDER_TOP));
	SetBorder(BORDER_LEFT, xfdef->GetBorderStyle(BORDER_LEFT),
	    xfdef->GetBorderColorIdx(BORDER_LEFT));
	SetBorder(BORDER_RIGHT, xfdef->GetBorderStyle(BORDER_RIGHT),
	    xfdef->GetBorderColorIdx(BORDER_RIGHT));

	SetFlags(xfdef->GetFlags());
	if(is_cell) {
		SetXFParent(xfdef->GetParentIndex());
	}
}

CExtFormat::~CExtFormat()
{
}

/*
 **********************************
 **********************************
 */
void CExtFormat::InitDummy(bool is_cell)
{
	// An style-XF record is initialized as below
	// Each field has to be modified individually before use it

	//The default style is a dummy. The flags that indicate what the style affects (byte 11)
	// are disabled (set to 1).
	static const uint8 xfCellDefault[] = {
		/*    0         2         4         6         8         10        12        14         16       18        20 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	static const uint8 xfStyleDefault[] = {
		// Open Office offsets
		/*    0         2         4         6         8         10        12        14         16       18        20 */
		0x00, 0x00, 0x00, 0x00, 0xf4, 0xff, 0x20, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x20
		// STYLE_XF | INDEX=0xFFF -> Style (Excel BIFF spec says this should be FFF4, but we had the 'locked' bit turned on as well here before)
		// HALIGN -> General, VALIGN -> BOTTOM
		// Cells, use this value, Style, not used
		// Fill Pattern -> 1 Black, left shifted 2 bits, (well, fully colored, not a pattern)
		// 0x40 Pattern Color, then lowest bit of 0x41 (next)
		// 0x41 Pattern background color right shifted one bit
	};

	if(is_cell) {
		AddDataArray(xfCellDefault, sizeof(xfCellDefault));
	}
	else {
		AddDataArray(xfStyleDefault, sizeof(xfStyleDefault));
	}
}

/*
 **********************************
 * CExtFormat class implementation
 **********************************
 */

bool CExtFormat::IsCell()
{
	uint16 val;

	GetValue16From(&val, XF_OFFSET_PROP);

	XL_ASSERT((val & XF_PROP_STYLE) ? (val & XF_PROP_XFPARENT) == (XF_PROP_XFPARENT_VALUE4STYLE << XF_PROP_SHIFTPOS_PARENT) : 1);
	return !(val & XF_PROP_STYLE);
}

void CExtFormat::SetXFParent(uint16 parent)
{
	uint16 val;

	GetValue16From(&val, XF_OFFSET_PROP);
	val &= ~XF_PROP_XFPARENT;
	val |= (parent << XF_PROP_SHIFTPOS_PARENT) & XF_PROP_XFPARENT;
	SetValueAt16(val, XF_OFFSET_PROP);
}

int CExtFormat::SetFontIndex(uint16 fontindex)
{
	// Set the index value
	int errcode = SetValueAt16(fontindex, XF_OFFSET_FONT);

	return errcode;
}

/*
 **********************************
 **********************************
 */
uint16 CExtFormat::GetFontIndex(void)
{
	uint16 fontval;

	GetValue16From(&fontval, XF_OFFSET_FONT);

	return fontval;
}

int CExtFormat::SetFormatIndex(uint16 formatindex)
{
	// Set the index value
	int errcode = SetValueAt16(formatindex, XF_OFFSET_FORMAT);

	return errcode;
}

uint16 CExtFormat::GetFormatIndex(void)
{
	uint16 formatval;

	GetValue16From(&formatval, XF_OFFSET_FORMAT);

	return formatval;
}

void CExtFormat::SetLocked(bool locked)
{
	uint16 value;

	GetValue16From(&value, XF_OFFSET_PROP);
	if(locked) {
		value |= XF_PROP_LOCKED;
	}
	else {
		value &= ~XF_PROP_LOCKED;
	}
	SetValueAt16(value, XF_OFFSET_PROP);
}

void CExtFormat::SetHidden(bool hidden)
{
	uint16 value;

	GetValue16From(&value, XF_OFFSET_PROP);
	if(hidden) {
		value |= XF_PROP_HIDDEN;
	}
	else {
		value &= ~XF_PROP_HIDDEN;
	}
	SetValueAt16(value, XF_OFFSET_PROP);
}

void CExtFormat::SetHorizAlign(uint8 alignval)
{
	uint32 value;

	GetValue32From(&value, XF_OFFSET_ALIGN);
	XL_ASSERT(XF_ALIGN_SHIFTPOS_HALIGN == 0);
	value = (value & (~(uint32)XF_ALIGN_HORIZONTAL)) | (alignval & XF_ALIGN_HORIZONTAL);
	SetValueAt32(value, XF_OFFSET_ALIGN);
}

void CExtFormat::SetVertAlign(uint8 alignval)
{
	uint32 value, alignval32;

	GetValue32From(&value, XF_OFFSET_ALIGN);
	alignval32 = alignval;
	alignval32 <<= XF_ALIGN_SHIFTPOS_VALIGN;            // Place the option at the right bit position
	value = (value & (~(uint32)XF_ALIGN_VERTICAL)) | (alignval32 & XF_ALIGN_VERTICAL);
	SetValueAt32(value, XF_OFFSET_ALIGN);
}

void CExtFormat::SetWrap(bool wrap)
{
	uint32 value;

	GetValue32From(&value, XF_OFFSET_ALIGN);
	if(wrap) {
		value |= XF_ALIGN_WRAP;
	}
	else {
		value &= ~(uint32)XF_ALIGN_WRAP;
	}
	SetValueAt32(value, XF_OFFSET_ALIGN);
}

void CExtFormat::SetIndent(uint8 indentval)
{
	uint32 value, mask;

	if(indentval & XF_INDENT_LVL) {
		mask = XF_INDENT_LVL;
	}
	else if(indentval & XF_INDENT_SHRINK2FIT)        {
		mask = XF_INDENT_SHRINK2FIT;
	}
	else if(indentval & (XF_INDENT_CONTEXT|XF_INDENT_L2R|XF_INDENT_R2L))        {
		mask = XF_INDENT_DIR;
	}
	else {
		mask = XF_INDENT_LVL | XF_INDENT_SHRINK2FIT | XF_INDENT_DIR;
	}

	mask <<= XF_INDENT_SHIFTPOS;

	GetValue32From(&value, XF_OFFSET_ALIGN);

	uint32 indentval32 = indentval;
	indentval32 <<= XF_INDENT_SHIFTPOS; // Place the option at the right bit position
	value = (value & (~mask)) | (indentval32 & mask);

	SetValueAt32(value, XF_OFFSET_ALIGN);
}

void CExtFormat::SetTxtOrientation(uint8 alignval)
{
	uint32 value;

	GetValue32From(&value, XF_OFFSET_ALIGN);

	uint32 alignval32 = alignval;
	alignval32 <<= XF_ORI_SHIFTPOS; // Place the option at the right bit position
	value = (value & (~(uint32)XF_ORI_MASK)) | (alignval32 & XF_ORI_MASK);

	SetValueAt32(value, XF_OFFSET_ALIGN);
}

void CExtFormat::SetFGColorIndex(uint16 color)
{
	uint16 value;

	GetValue16From(&value, XF_OFFSET_COLOR);

	// Clear the field for Foreground color
	value &= (~XF_COLOR_FOREGROUND);
	// Set the new color
	value |= (color & XF_COLOR_FOREGROUND);

	SetValueAt16(value, XF_OFFSET_COLOR);
}

void CExtFormat::SetBGColorIndex(uint16 color)
{
	uint16 value;

	color <<= XF_COLOR_SHIFTPOS_BG;

	GetValue16From(&value, XF_OFFSET_COLOR);

	// Clear the field for Foreground color
	value &= (~XF_COLOR_BACKGROUND);
	// Set the new color
	value |= (color & XF_COLOR_BACKGROUND);

	SetValueAt16(value, XF_OFFSET_COLOR);
}

void CExtFormat::SetFillPattern(uint8 pattern)
{
	uint32 value, pattern32 = pattern;

	GetValue32From(&value, XF_OFFSET_BORDERB);
	value &= ~XF_BORDER_FILLPATTERN;
	pattern32 <<= XF_SHIFTPOS_FILLPATTERN;
	value |= (pattern32 & XF_BORDER_FILLPATTERN);
	SetValueAt32(value, XF_OFFSET_BORDERB);
}

void CExtFormat::SetBorder(border_side_t border, uint16 style, uint16 color)
{
	uint32 value, color32 = color, style32 = style;

	switch(border) {
		case BORDER_BOTTOM:
		    GetValue32From(&value, XF_OFFSET_BORDERA);
		    value &= (~(uint32)XF_BORDER_BOTTOMSTYLE);
		    style32 <<= XF_STYLE_SHIFTPOS_BOTTOM;
		    value |= (style32 & XF_BORDER_BOTTOMSTYLE);
		    SetValueAt32(value, XF_OFFSET_BORDERA);

		    GetValue32From(&value, XF_OFFSET_BORDERB);
		    value &= (~(uint32)XF_BORDER_BOTTOMCOLOR);
		    color32 <<= XF_COLOR_SHIFTPOS_BOTTOM;
		    value |= (color32 & XF_BORDER_BOTTOMCOLOR);
		    SetValueAt32(value, XF_OFFSET_BORDERB);
		    break;

		case BORDER_TOP:
		    GetValue32From(&value, XF_OFFSET_BORDERA);
		    value &= (~(uint32)XF_BORDER_TOPSTYLE);
		    style32 <<= XF_STYLE_SHIFTPOS_TOP;
		    value |= (style32 & XF_BORDER_TOPSTYLE);
		    SetValueAt32(value, XF_OFFSET_BORDERA);

		    GetValue32From(&value, XF_OFFSET_BORDERB);
		    value &= (~(uint32)XF_BORDER_TOPCOLOR);
		    color32 <<= XF_COLOR_SHIFTPOS_TOP;
		    value |= (color32 & XF_BORDER_TOPCOLOR);
		    SetValueAt32(value, XF_OFFSET_BORDERB);
		    break;

		case BORDER_LEFT:
		    GetValue32From(&value, XF_OFFSET_BORDERA);
		    value &= ~(uint32)(XF_BORDER_LEFTSTYLE|XF_BORDER_LEFTCOLOR);

		    color32 <<= XF_COLOR_SHIFTPOS_LEFT;
		    style32 <<= XF_STYLE_SHIFTPOS_LEFT;
		    value |= (color32 & XF_BORDER_LEFTCOLOR) | (style32 & XF_BORDER_LEFTSTYLE);

		    SetValueAt32(value, XF_OFFSET_BORDERA);
		    break;

		case BORDER_RIGHT:
		    GetValue32From(&value, XF_OFFSET_BORDERA);

		    value &= ~(uint32)(XF_BORDER_RIGHTSTYLE|XF_BORDER_RIGHTCOLOR);
		    color32 <<= XF_COLOR_SHIFTPOS_RIGHT;
		    style32 <<= XF_STYLE_SHIFTPOS_RIGHT;
		    value |= (color32 & XF_BORDER_RIGHTCOLOR) | (style32 & XF_BORDER_RIGHTSTYLE);

		    SetValueAt32(value, XF_OFFSET_BORDERA);
		    break;

		default:
		    break;
	}
}

void CExtFormat::SetFlags(uint8 flags)
{
	uint32 value;
	uint32 flags32 = flags;

	flags32 <<= XF_ALIGN_ATR_SHIFT;
	GetValue32From(&value, XF_OFFSET_ALIGN);
	value = (value & (~XF_ALIGN_ATR_MASK)) | (flags32 & XF_ALIGN_ATR_MASK);

	SetValueAt32(value, XF_OFFSET_ALIGN);
}
