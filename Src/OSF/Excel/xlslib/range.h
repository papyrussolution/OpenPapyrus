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

#ifndef RANGE_H
#define RANGE_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	class range : public xf_i, public font_i, private range_t {
	public:
		range(uint32 row1, uint32 col1, // inclusive
			  uint32 row2, uint32 col2,
			  worksheet* pws);
		virtual ~range();
		void cellcolor(color_name_t color);
		void boxer(border_style_t border, fill_option_t fill, color_name_t borderColor, color_name_t fillFgColor, color_name_t fillBgColor);
	private:
		worksheet* m_pWorkSheet;
		bool m_Atomic;
	public:
		range(const range& that);
		range& operator=(const range& right);
	public: // xf_i interface declaration
		void font(font_t* fontidx);
		void format(format_number_t formatidx);
		void format(format_t* format);
		void halign(halign_option_t ha_option);
		void valign(valign_option_t va_option);
		void indent(indent_option_t indent_option);
		void orientation(txtori_option_t ori_option);
		void fillfgcolor(color_name_t color);
		void fillfgcolor(uint8 color);
		void fillbgcolor(color_name_t color);
		void fillbgcolor(uint8 color);
		void fillstyle(fill_option_t fill);
		void locked(bool locked_opt);
		void hidden(bool hidden_opt);
		void wrap(bool wrap_opt);
		void borderstyle(border_side_t side, border_style_t style);
		void bordercolor(border_side_t side, uint8 color);
		void bordercolor(border_side_t side, color_name_t color);
	public: // font_i interface declaration
		void fontname(const std::string& fntname);
		void fontheight(uint16 fntheight);
		void fontbold(boldness_option_t fntboldness);
		void fontunderline(underline_option_t fntunderline);
		void fontscript(script_option_t fntscript);
		void fontcolor(color_name_t fntcolor);
		void fontcolor(uint8 fntcolor);
		void fontitalic(bool italic);
		void fontstrikeout(bool so);
		void fontoutline(bool ol);
		void fontshadow(bool sh);
	};

	typedef enum {
		BORDER_BOTTOM_BIT	= 0x01,
		BORDER_TOP_BIT		= 0x02,
		BORDER_LEFT_BIT		= 0x04,
		BORDER_RIGHT_BIT	= 0x08
	} borderBits_t;

	typedef struct {
		uint32 flags;
		xf_t			*xft;
	} borderedXft;

	typedef std::pair<xlslib_core::xf_t *, borderedXft> xf_Pair_t;

	class xfSorter {
	public:
		bool operator() (const xf_Pair_t& left, const xf_Pair_t& right) const
		{
			if((left.first)->index < (right.first)->index) { return true; }
			if((left.first)->index > (right.first)->index) { return false; }
			return (left.second).flags < (right.second).flags;
		}
	};

	typedef std::set<xf_Pair_t, xlslib_core::xfSorter> xf_Pair_Set_t;
	typedef xf_Pair_Set_t::iterator xf_Pair_Set_Itor_t;
}

// #include "xls_poppack.h"

#endif
