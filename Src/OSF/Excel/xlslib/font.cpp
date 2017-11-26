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

/*
 **********************************
 *  font_t class implementation
 **********************************
 */
const uint16 font_t::BOLD_OPTION_TABLE[] = {
	FONT_BOLDNESS_BOLD,
	FONT_BOLDNESS_HALF,
	FONT_BOLDNESS_NORMAL,
	FONT_BOLDNESS_DOUBLE
};

const uint16 font_t::SCRIPT_OPTION_TABLE[] = {
	FONT_SCRIPT_NONE,
	FONT_SCRIPT_SUPER,
	FONT_SCRIPT_SUB
};

const uint8 font_t::UNDERLINE_OPTION_TABLE[] = {
	FONT_UNDERLINE_NONE,
	FONT_UNDERLINE_SINGLE,
	FONT_UNDERLINE_DOUBLE,
	FONT_UNDERLINE_SINGLEACC,
	FONT_UNDERLINE_DOUBLEACC
};

const uint8 font_t::COLOR_OPTION_TABLE[] =
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


font_t::font_t(CGlobalRecords& gRecords) :
	m_GlobalRecords(gRecords),
	name(FONT_DFLT_FONTNAME),
	index(0x0000),
	height(FONT_DFLT_HEIGHT),
	boldstyle(FONT_BOLDNESS_NORMAL),
	script(FONT_SCRIPT_NONE),
	attributes(FONT_DFLT_ATTRIBUTES),
	color(FONT_DFLT_PALETTE),
	underline(FONT_UNDERLINE_NONE),
	family(FONT_DFLT_FAMILY),
	charset(FONT_DFLT_CHARSET),
	m_usage_counter(0)
{
}

font_t::font_t(const font_t& right) :
	m_GlobalRecords(right.m_GlobalRecords),
	name(right.name),
	index(0x0000),
	height(right.height),
	boldstyle(right.boldstyle),
	script(right.script),
	attributes(right.attributes),
	color(right.color),
	underline(right.underline),
	family(right.family),
	charset(right.charset),
	m_usage_counter(0)
{
	m_GlobalRecords.AddFont(this);
}

// only used by globalRec for defaults
font_t::font_t(CGlobalRecords& gRecords,
			   uint16 index_,
			   const std::string& name_,
			   uint16 height_,
			   boldness_option_t boldstyle_,
			   underline_option_t underline_,
			   script_option_t script_,
			   color_name_t color_,
			   uint16 attributes_,
			   uint8 family_,
			   uint8 charset_) :
	m_GlobalRecords(gRecords),
	//index(0x0000),
	//name(FONT_DFLT_FONTNAME),
	//height(FONT_DFLT_HEIGHT),
	//boldstyle(FONT_BOLDNESS_NORMAL),
	//script(FONT_SCRIPT_NONE),
	attributes(attributes_),
	//color(FONT_DFLT_PALETTE),
	//underline(FONT_UNDERLINE_NONE),
	//family(FONT_DFLT_FAMILY),
	//charset(FONT_DFLT_CHARSET),
	m_usage_counter(0)
{
	SetIndex(index_);
	SetName(name_);
	SetHeight(height_);
	SetBoldStyle(boldstyle_);
	SetUnderlineStyle(underline_);
	SetScriptStyle(script_);
	SetColor(color_);
	SetFamily(family_);
	SetCharset(charset_);
}

font_t &font_t::operator =(const font_t &src)
{
	(void)src; // stop warning
	throw std::string("Should never have invoked the font_t copy operator!");
}

/*
 **********************************
 **********************************
 */
void font_t::MarkUsed(void)
{
	m_usage_counter++;
}

void font_t::UnMarkUsed(void)
{
	if(m_usage_counter) {
		m_usage_counter--;
	}
}

uint32 font_t::Usage(void) const
{
	return m_usage_counter;
}

/*
 **********************************
 **********************************
 */
void font_t::SetItalic(bool italic)
{
	if(italic) {
		attributes |= FONT_ATTR_ITALIC;
	} else {
		attributes &= (~FONT_ATTR_ITALIC);
	}

	// m_sigchanged = true;
}

void font_t::SetStrikeout(bool so)
{
	if(so) {
		attributes |= FONT_ATTR_STRIKEOUT;
	} else {
		attributes &= (~FONT_ATTR_STRIKEOUT);
	}

	// m_sigchanged = true;
}

// OSX (Mac) only
void font_t::SetOutline(bool ol)
{
	if(ol) {
		attributes |= FONT_ATTR_OUTLINEMACH;
	} else {
		attributes &= (~FONT_ATTR_OUTLINEMACH);
	}

	// m_sigchanged = true;
}

// OSX (Mac) only
void font_t::SetShadow(bool sh)
{
	if(sh) {
		attributes |= FONT_ATTR_SHADOWMACH;
	} else {
		attributes &= (~FONT_ATTR_SHADOWMACH);
	}

	// m_sigchanged = true;
}

/* FONT Index wrappers*/
void font_t::SetIndex(uint16 fntidx)
{
	index = fntidx;
	// m_sigchanged = true;
}

uint16 font_t::GetIndex(void) const
{
	return index;
}

/* FONT Index wrappers*/
void font_t::SetName(const std::string& fntname)
{
	name = fntname;
	//  m_sigchanged = true;
}

/* FONT height wrappers*/
void font_t::SetHeight(uint16 fntheight)
{
	height = fntheight;
	//  m_sigchanged = true;
}

uint16 font_t::GetHeight(void) const
{
	return height;
}

/* FONT boldstyle wrappers*/
void font_t::SetBoldStyle(boldness_option_t fntboldness)
{
	boldstyle = font_t::BOLD_OPTION_TABLE[fntboldness];
	//  m_sigchanged = true;
}

void font_t::_SetBoldStyle(uint16 fntboldness)
{
	XL_ASSERT(fntboldness >= 100);
	XL_ASSERT(fntboldness <= 1000);
	boldstyle = fntboldness;
	//  m_sigchanged = true;
}

uint16 font_t::GetBoldStyle(void) const
{
	return boldstyle;
}

/* FONT underline wrappers*/
void font_t::SetUnderlineStyle(underline_option_t fntunderline)
{
	underline = font_t::UNDERLINE_OPTION_TABLE[fntunderline];
	// m_sigchanged = true;
}

uint8 font_t::GetUnderlineStyle(void) const
{
	return underline;
}

/* FONT script wrappers*/
void font_t::SetScriptStyle(script_option_t fntscript)
{
	script = font_t::SCRIPT_OPTION_TABLE[fntscript];
	//  m_sigchanged = true;
}

uint16 font_t::GetScriptStyle(void) const
{
	return script;
}

/* FONT script wrappers*/
void font_t::SetColor(color_name_t fntcolor)
{
	color = font_t::COLOR_OPTION_TABLE[fntcolor];
	//   m_sigchanged = true;
}

void font_t::SetColor(uint8 fntcolor)
{
	color = fntcolor;
	//   m_sigchanged = true;
}

uint16 font_t::GetColorIdx(void) const
{
	return color;
}

/* FONT  attributes wrappers */
#if defined(DEPRECATED)
void font_t::SetAttributes(uint16 attr)
{
	attributes = attr;
	//   m_sigchanged = true;
}

#endif

uint16 font_t::GetAttributes(void) const
{
	return attributes;
}

// Miscellaneous;
void font_t::SetFamily(uint8 fam)
{
	family = fam;
	// m_sigchanged = true;
}

uint8 font_t::GetFamily(void) const
{
	return family;
}

void font_t::SetCharset(uint8 chrset)
{
	charset = chrset;
	//  m_sigchanged = true;
}

uint8 font_t::GetCharset(void) const
{
	return charset;
}

const std::string& font_t::GetName(void) const {return name; }
bool font_t::GetItalic() const {return (attributes & FONT_ATTR_ITALIC) ? true : false; }
bool font_t::GetStrikeout() const {return (attributes & FONT_ATTR_STRIKEOUT) ? true : false; }
bool font_t::GetOutline() const {return (attributes & FONT_ATTR_OUTLINEMACH) ? true : false; }
bool font_t::GetShadow() const {return (attributes & FONT_ATTR_SHADOWMACH) ? true : false; }

/*
 **********************************
 *  CFont class implementation
 **********************************
 */
CFont::CFont(CDataStorage &datastore, const font_t* fontdef) :
	CRecord(datastore)
{
	SetRecordType(RECTYPE_FONT);

	AddValue16(fontdef->GetHeight());
	AddValue16(fontdef->GetAttributes());
	AddValue16(fontdef->GetColorIdx());
	AddValue16(fontdef->GetBoldStyle());
	AddValue16(fontdef->GetScriptStyle());
	AddValue8(fontdef->GetUnderlineStyle());
	AddValue8(fontdef->GetFamily());
	AddValue8(fontdef->GetCharset());
	AddValue8(FONT_RESERVED);
	AddUnicodeString(fontdef->GetGlobalRecords(), fontdef->GetName(), LEN1_FLAGS_UNICODE);

	SetRecordLength(GetDataSize()-RECORD_HEADER_SIZE);
}

CFont::~CFont()
{
}
