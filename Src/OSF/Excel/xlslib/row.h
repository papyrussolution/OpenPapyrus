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

#ifndef ROW_H
#define ROW_H

// #include "xls_pshpack2.h"

/*
 ******************************
 * CRow class declaration
 ******************************
 */

namespace xlslib_core
{
#define ROW_DFLT_HEIGHT        0x0108
#define ROW_DFLT_GRBIT         0x0080
#define ROW_GRBIT_UNSYNC       0x0040
#define ROW_DFLT_IXFE          XF_PROP_XF_DEFAULT_CELL
#define ROW_OFFSET_FIRSTCOL    6
#define ROW_OFFSET_LASTCOL     8
#define ROW_MASK_STDHEIGHT     0x0108
#define ROW_RECORD_SIZE        20

	class rowheight_t {
	public:
		rowheight_t();
		rowheight_t(uint32 rownum, uint16 rowheight = ROW_DFLT_HEIGHT, xf_t *pxformat = NULL);
		virtual ~rowheight_t();

		uint32 GetRowNum() {return num; }
		void SetRowNum(uint32 rownum) {num = rownum; }

		uint16 GetRowHeight() {return height; }
		void SetRowHeight(uint16 rowheight) {height = rowheight; }

		xf_t* GetXF(void) const {return xformat; }

		bool operator<(const rowheight_t& right) const
		{
			return num < right.num;
		}

		bool operator>(const rowheight_t& right) const
		{
			return num > right.num;
		}

		bool operator==(const rowheight_t& right) const
		{
			return num == right.num;
		}

		bool operator!=(const rowheight_t& right) const
		{
			return num != right.num;
		}

	private:
		rowheight_t(const rowheight_t& that);
		rowheight_t& operator=(const rowheight_t& right);

	private:
		xf_t* xformat;
		uint32 num;
		uint16 height;
	};

	class rowheightsort
	{
	public:
		bool operator()(rowheight_t* const &a, rowheight_t* const &b) const
		{
			return a->GetRowNum() < b->GetRowNum();
		}
	};

	typedef std::set<xlslib_core::rowheight_t*, rowheightsort XLSLIB_DFLT_ALLOCATOR> RowHeight_Vect_t;
	typedef RowHeight_Vect_t::iterator RowHeight_Vect_Itor_t;

	class CRow : public CRecord
	{
		friend class CDataStorage;

	protected:
		CRow(CDataStorage &datastore,
			 uint32 rownum,
			 uint32 firstcol,
			 uint32 lastcol,
			 uint16 rowheight  = ROW_DFLT_HEIGHT,
			 const xf_t* xformat = NULL);

	private:
		virtual ~CRow();
	};


/*
 ******************************
 * CDBCell class declaration
 ******************************
 */

#define DBC_DFLT_STARTBLOCK  (0x00000000)

	class CDBCell : public CRecord
	{
		friend class CDataStorage;

	protected:
		CDBCell(CDataStorage &datastore, size_t startblock = DBC_DFLT_STARTBLOCK);
	private:
		virtual ~CDBCell();

	public:
		void AddRowOffset(size_t rowoffset);
	};
}


// #include "xls_poppack.h"

#endif
