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

#ifndef DATAST_H
#define DATAST_H

// #include "xls_pshpack2.h"

/*
 ******************************
 * COleFile class declaration
 ******************************
 */

namespace xlslib_core
{
#define FIXEDWIDTH_STORAGEUNIT_SIZE  18
	// the most frequent BIFF fixed-width records are BLANK, RK and NUMBER, which top out at 18 bytes a head

	/*
	 *  This structure is constructed to give the least amount of storage overhead per [BIFF] data unit:
	 *
	 *  4 bytes for simple types (fixed width, up to 18 bytes of storage per unit: BLANK/RK/NUMBER
	 *
	 *  4+18+A / 4+18+A bytes per variabel/larger sized units on 32-/64-bit architectures, where 'A' is
	 *   the number of bytes overhead per malloc(), which will very probably be >= 4
	 *
	 *  Note: bitfields are used to allow the compiler to roll multiple variables into one integer, thus
	 *     further reducing overhead. Given the limitations of Excel (including Office 2007), an upper
	 *     bound of 2^28 ~ 500M bytes for each storage unit's size is quite acceptable.
	 *
	 *  Note #2: the whole fixed vs. varying(large) width business was changed at the last minute
	 *     to use the inverse of the flag (!m_varying_width --renamed--> m_varying_width) so that the
	 *     copy constructor ASSERTs don't trigger when this stupid M$ vector<> calls the copy
	 *     constructor for a memset(0,size)-all-zeroes 'object instance' (which isn't: the constructor
	 *     wasn't called on the original!). Yes, the M$ MSVC2008 STL vector.end() only works out
	 *     when your default class init is all zeroes everywhere. Now, with m_varying_width==0,
	 *     the copy constructor will take the harmless 'local storage copy' route for this bogus
	 *     class instance vector<>.end().
	 */

	class CHPSFdoc;
	class hpsf_doc_t;
	class CUnit;

	class CUnitStore
	{
		friend class CUnit;

	private:
		// 32-bits:
		bool m_varying_width : 1;
		bool m_is_in_use : 1;		// used to flag a 'deleted' unit store
		bool m_is_sticky : 1;		// used to flag a 'sticky' unit store: that's a store which will remain after the accompanying CUnit is deleted. It will only be discarded by the time the CDataStorage container is deleted.
		unsigned m_nDataSize : 29;	// Actual use

		union
		{
			struct
			{
				uint8	m_pData[FIXEDWIDTH_STORAGEUNIT_SIZE];   // Data storage
			} fixed;
			struct
			{
				size_t m_nSize;			// Size of data store (maybe larger than actual used data)
				uint8* m_pData;	// Data storage
			} vary;
		} s;

	public:
		CUnitStore();
		~CUnitStore();
		CUnitStore(const CUnitStore &src); // must provide a copy constructor or otherwise STL vector will nuke us

		int8 Prepare(size_t minimum_size);
		void Reset(void);
		int8 Resize(size_t newlen);
		int8 Init(const uint8 *data, size_t size, size_t datasize);
		int8 InitWithValue(uint8 value, size_t size);

		inline uint8 *GetBuffer(void)
		{
			XL_ASSERT(m_is_in_use);
			if (!m_varying_width) {
				return s.fixed.m_pData;
			} else {
				return s.vary.m_pData;
			}
		}

		inline const uint8 *GetBuffer(void) const
		{
			XL_ASSERT(m_is_in_use);
			if (!m_varying_width) {
				return s.fixed.m_pData;
			} else {
				return s.vary.m_pData;
			}
		}

		inline void SetDataSize(size_t len)
		{
			XL_ASSERT(m_is_in_use);
			XL_ASSERT(len <= GetSize());
			m_nDataSize = (unsigned)len;
		}

		inline size_t GetSize(void) const
		{
			XL_ASSERT(m_is_in_use);
			if (!m_varying_width) {
				return FIXEDWIDTH_STORAGEUNIT_SIZE;
			} else {
				return s.vary.m_nSize;
			}
		}

		inline size_t GetDataSize(void) const
		{
			XL_ASSERT(m_is_in_use);
			return m_nDataSize;
		}

		inline bool IsSticky(void) const
		{
			XL_ASSERT(m_is_in_use);
			return m_is_sticky;
		}

		inline void MakeSticky(void)
		{
			XL_ASSERT(m_is_in_use);
			m_is_sticky = true;
		}
	};

	/*
	 *  To reduce storage overhead and cut down on malloc()/free() library abuse (which is also hit by
	 *  each new()/delete()), this vector stores the units as is, NOT pointers to new() allocated units!
	 */
	typedef std::vector<xlslib_core::CUnitStore XLSLIB_DFLT_ALLOCATOR> StoreList_t;
	typedef StoreList_t::iterator StoreList_Itor_t;

	typedef std::vector<xlslib_core::CUnit* XLSLIB_DFLT_ALLOCATOR> UnitList_t;
	typedef UnitList_t::iterator UnitList_Itor_t;

	class CRecord;
	class CRow;
	class CBof;
	class CEof;
	class CDimension;
	class CWindow1;
	class CWindow2;
	class CDateMode;
	class CStyle;
	class CBSheet;
	class CFormat;
	class CFont;
	class CNumber;
	class CBoolean;
	class CErr;
	class CNote;
	class CFormula;
	class CMergedCells;
	class CLabel;
	class CIndex;
	class CExtFormat;
	class CContinue;
	class CPalette;
	class CColInfo;
	class CBlank;
	class CCodePage;
	class HPSFdoc;

	class xf_t;
	class window1;
	struct style_t;
	class boundsheet_t;
	class format_t;
	class font_t;
	class number_t;
	class boolean_t;
	class err_t;
	class note_t;
	class formula_cell_t;
	class label_t;
	typedef std::vector<const label_t *>Label_Vect_t;
	struct color_entry_t;
	struct colinfo_t;
	class blank_t;

	class CDataStorage
	{
	private:

		StoreList_t store;

		size_t m_DataSize;

	public:
		CDataStorage();
		CDataStorage(size_t blobs);
		virtual ~CDataStorage();

		StoreList_Itor_t begin();
		StoreList_Itor_t end();

	private:
		CDataStorage(const CDataStorage& that);
		CDataStorage& operator=(const CDataStorage& right);

	public:
		void operator+=(CUnit* from);
		size_t GetDataSize() const;

		int32 RequestIndex(size_t minimum_size);
		CUnitStore& operator[](int32 index);

	private:
		/*
		 *  collect all CUnit instances here so we can discard them at
		 *  the appropriate time: some of them require late backpatching,
		 *  so lifetimes differ for certain Cunit instances
		 *
		 *  To speed up the 'FlushEm' process a bit over multiple invocations,
		 *  we recall the position the previous flush stopped - which is a
		 *  perfect starting point for the next flush when the levels are
		 *  identical.
		 */
		UnitList_t m_FlushStack;
		size_t /* UnitList_Itor_t */ m_FlushLastEndPos; // default: .begin()
		/*
		 *  ^^^^^ WARNING LEST I FORGET [Ger Hobbelt / i_a] ^^^^^
		 *
		 *  It's not safe, at least not with MSVC to keep a vector iterator around
		 *  while code calls push_back() to add items to the vector: at some point in
		 *  time the vector will be re-allocated and since the iterator is a pointer
		 *  (not an index!) that one suddenly points straight to Hell.
		 *
		 *  Another case of 'this almost never happens...' :-(
		 */
		uint16 m_FlushLastEndLevel;

	public:
		void Push(CUnit* unit);
		size_t Clip(CUnit* unit);   // When using Continue record
		static const uint16 BACKPATCH_LEVEL_EVERYONE = (uint16) ~0;
		void FlushEm(uint16 backpatch_level = 0);
		void FlushLowerLevelUnits(const CUnit *unit);

		// factory:
	public:
		CUnit* MakeCUnit();
		CRecord* MakeCRecord();
		CRow* MakeCRow(uint32 rownum, uint32 firstcol, uint32 lastcol,
					   uint16 rowheight = ROW_DFLT_HEIGHT,
					   const xf_t* xformat = NULL);
		CBof* MakeCBof(uint16 boftype);
		CEof* MakeCEof();
		CDimension* MakeCDimension(uint32 minRow, uint32 maxRow, uint32 minCol, uint32 maxCol);
		CWindow1* MakeCWindow1(const window1& wind1);
		CWindow2* MakeCWindow2(bool isActive);
		CDateMode* MakeCDateMode();
		CStyle* MakeCStyle(const style_t* styledef);
		CBSheet* MakeCBSheet(const boundsheet_t* bsheetdef);
		CFormat* MakeCFormat(const format_t* formatdef);
		CFont* MakeCFont(const font_t* fontdef);
		CNumber* MakeCNumber(const number_t& blankdef);
		CBoolean* MakeCBoolean(const boolean_t& booldef);
		CErr* MakeCErr(const err_t& errdef);
		CMergedCells* MakeCMergedCells();
		CNote* MakeCNote(const note_t& cmtdef);
		CFormula* MakeCFormula(const formula_cell_t& fdef);
		CLabel* MakeCLabel(const label_t& labeldef);
		CIndex* MakeCIndex(uint32 firstrow, uint32 lastrow);
		CExtFormat* MakeCExtFormat(const xf_t* xfdef);
		CContinue* MakeCContinue(CUnit* unit, const uint8* data, size_t size);
		CPalette* MakeCPalette(const color_entry_t *colors);
		CColInfo* MakeCColInfo(const colinfo_t* newci);
		CBlank* MakeCBlank(const blank_t& blankdef);
		CCodePage* MakeCCodePage(uint16 boftype);
		CDBCell* MakeCDBCell(size_t startblock = DBC_DFLT_STARTBLOCK);
		CHPSFdoc* MakeCHPSFdoc(const hpsf_doc_t &docdef);
        CUnit* MakeCExternBook(uint16 sheet_count);
        CUnit* MakeCExternSheet(const Boundsheet_Vect_t& sheets);
		CUnit *MakeSST(const Label_Vect_t& labels);
	};
}

// #include "xls_poppack.h"

#endif
