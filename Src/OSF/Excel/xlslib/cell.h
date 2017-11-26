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

#ifndef CELL_H
#define CELL_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	class insertsort;
	class xf_t;
	class font_t;
    class worksheet;

	class cell_t  // : public xf_i , public font_i // Bugs item #2840335 - appears not required (this was here when I [i_a] got the project :-)
	{
		friend class insertsort;
        friend class worksheet;

	protected:
		cell_t(CGlobalRecords& gRecord, uint32 row, uint32 col, xf_t* pxfval);
		virtual ~cell_t();    // "C++ Coding Standards" rule 50 (protected and non-virtual)     [i_a] MUST be virtual or you'll get blown out of the sky by memleaks (label_t instances in cell_t lists)

	private:
		cell_t(const cell_t& that);
		cell_t& operator=(const cell_t& right);

		void set_xf_common(void);
		void set_cell_font(void);

	public:
		uint16 GetXFIndex(void) const;
		uint32 GetRow(void) const
		{
			return row;
		}

		uint32 GetCol(void) const
		{
			return col;
		}

		void SetXF(xf_t* pxfval);
		xf_t* GetXF(void) const;

        worksheet* GetWorksheet(void) const { return ws; };

		virtual size_t GetSize(void) const = 0;
		virtual CUnit* GetData(CDataStorage &datastore) const = 0;

	protected:
		CGlobalRecords& m_GlobalRecords;
		static const uint16 FORMAT_NUM_OPTIONS_TABLE[];
		xf_t* pxf;
		uint32 row;
		uint32 col;
        worksheet *ws;

	public: // xf_i interface
		void font(font_t* font);
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
		void bordercolor(border_side_t side, color_name_t color);
		void bordercolor(border_side_t side, uint8 color);
	public: //font_i interface
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
#if defined(DEPRECATED)
		void fontattr(uint16 attr);
#endif

	public:
		CGlobalRecords& GetGlobalRecords(void) const
		{
			return m_GlobalRecords;
		}
	};

	class insertsort
	{
	public:
		bool operator()(cell_t* a, cell_t* b) const
		{
			if(a->row != b->row) {
				return a->row < b->row;
			} else {
				return a->col < b->col;
			}
		}
	};

	typedef std::set<xlslib_core::cell_t*, insertsort XLSLIB_DFLT_ALLOCATOR> Cell_Set_t;
	typedef Cell_Set_t::iterator Cell_Set_Itor_t;
	typedef Cell_Set_t::const_iterator Cell_Set_CItor_t;
}

// #include "xls_poppack.h"

#endif
