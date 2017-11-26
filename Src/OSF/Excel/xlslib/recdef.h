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

#ifndef RECDEF_H
#define RECDEF_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
/*
 ******************************
 * CBof class declaration
 ******************************
 */
#define  VERSION_BIFF				0x0600
// BIFF8, the last one!

#define BOF_TYPE_WBGLOBALS			0x0005
#define BOF_TYPE_VBMODULE			0x0006
#define BOF_TYPE_WORKSHEET			0x0010
#define BOF_TYPE_CHART				0x0020
#define BOF_TYPE_EXCEL4_MACROSH		0x0040
#define BOF_TYPE_WSFILE				0x0100

#define BOF_BUILD_DFLT				0x1d5f
// from some old file???
#define BOF_YEAR_DFLT				0x07cd
// 1997

#define TWIP						20


#if 0
#define CODEPAGE_IBMPC				0x01b5
#define CODEPAGE_APPLE				0x8000
#define CODEPAGE_ANSI				0x04e4
#endif

	class CBof : public CRecord {
		friend class CDataStorage;
	protected:
		CBof(CDataStorage &datastore, uint16 boftype);
	private:
		virtual ~CBof();
	};

/*
 ******************************
 * CEof class declaration
 ******************************
 */
	class CEof : public CRecord {
		friend class CDataStorage;
	protected:
		CEof(CDataStorage &datastore);
	private:
		virtual ~CEof();
	};

/*
 ******************************
 * CCodePage class declaration
 ******************************
 */
	class CCodePage : public CRecord
	{
		friend class CDataStorage;

	protected:
		CCodePage(CDataStorage &datastore, uint16 boftype);
	private:
		virtual ~CCodePage();
	};

/*
 ******************************
 * CWindow1 class declaration
 ******************************
 */
	class CWindow1;

	class window1
	{
		friend class CWindow1;

	private:
		uint16 horzPos, vertPos;      // points
		uint16 windWidth, windHeight;         // points
		uint16 activeSheet;           // 0 offset
		uint16 firstVisibleTab;       // 0 offset
		uint16 tabBarWidth;           // 0 - 1000, from no tab bar up to no scroll bar

	public:
		window1();
		virtual ~window1();

		// access from workBook
		void SetPosition(uint16 horz, uint16 vert) { horzPos=horz; vertPos=vert; }
		void SetSize(uint16 width, uint16 height) { windWidth=width; windHeight=height; }
		void SetFirstTab(uint16 firstTab) { firstVisibleTab=firstTab; }
		void SetTabBarWidth(uint16 width) { tabBarWidth = (width > 1000) ? 1000 : width; }

		// access from workSheet
		void SetActiveSheet(uint16 theSheet) { activeSheet=theSheet; }
		uint16 GetActiveSheet() const { return activeSheet; }
	};

	class CWindow1 : public CRecord
	{
		friend class CDataStorage;

	protected:
		CWindow1(CDataStorage &datastore, const window1& wind1);
	private:
		virtual ~CWindow1();
	};
/*
 ******************************
 * CDateMode class declaration
 ******************************
 */
	class CDateMode : public CRecord
	{
		friend class CDataStorage;

	protected:
		CDateMode(CDataStorage &datastore);
	private:
		virtual ~CDateMode();

	public:
		static bool Is_In_1904_Mode(void);
	};



/*
 ******************************
 * CWindow2 class declaration
 ******************************
 */

#define W2_OFFSET_GRBIT          4
#define W2_OFFSET_TOPROW         6
#define W2_OFFSET_LEFTCOL        8
#define W2_OFFSET_COLOR          10
#define W2_OFFSET_ZOOMPREVIEW    14
#define W2_OFFSET_ZOOMNORMAL     16
#define W2_OFFSET_RESERVED       18


#define W2_DFLT_TOPROW     0x0000
#define W2_DFLT_LEFTCOL    0x0000
#define W2_DFLT_COLOR      0x00000000
	// NOTE: Check a BIFF8 example to verify the units of the two following values
#define W2_DFLT_ZOOMPBPREV 0x0100
#define W2_DFLT_ZOOMNORMAL 0x0100
#define W2_DFLT_RESERVED   0x00000000

	// GRBIT mask-flags:
#define W2_GRBITMASK_FMLA          0x0001
#define W2_GRBITMASK_GRIDS         0x0002
#define W2_GRBITMASK_HROWCOL       0x0004
#define W2_GRBITMASK_FROZEN        0x0008
#define W2_GRBITMASK_ZEROS         0x0010
#define W2_GRBITMASK_DFLTHDRCOLOR  0x0020
#define W2_GRBITMASK_ARABIC        0x0040
#define W2_GRBITMASK_GUTS          0x0080
#define W2_GRBITMASK_FRZNOSPLIT    0x0100
#define W2_GRBITMASK_SELECTED      0x0200
#define W2_GRBITMASK_ACTIVE        0x0400
#define W2_GRBITMASK_PAGEBRK       0x0800
#define W2_GRBITMASK_RESERVED      0xf000

	/*
	 *  NOTE: Hardcoded from an excel example
	 * #define W2_DFLT_GRBIT ((uint16)0x06b6)
	 * #define W2_DFLT_GRBIT ((uint16) \
	 *  (W2_GRBITMASK_GRIDS|W2_GRBITMASK_HROWCOL|W2_GRBITMASK_DFLTHDRCOLOR))
	 */
	class CWindow2 : public CRecord
	{
		friend class CDataStorage;

	protected:
		// TODO: Create a constructor that gets user-defined arguments that specify the appearence
		// The following constructor establishes default values.
		CWindow2(CDataStorage &datastore, bool isActive);
	private:
		virtual ~CWindow2();

	public:
		void SetSelected();
		void SetPaged();
		void ClearSelected();
		void ClearPaged();
	};

/*
 ******************************
 * CDimension class declaration
 ******************************
 */
	class CDimension : public CRecord
	{
		friend class CDataStorage;

	protected:
		CDimension(CDataStorage &datastore,
				   uint32 minRow,
				   uint32 maxRow,
				   uint32 minCol,
				   uint32 maxCol);
	private:
		virtual ~CDimension();
	};

/*
 ******************************
 * CStyle class declaration
 ******************************
 */

	struct style_t
	{
		uint16 xfindex;
		uint8 builtintype;
		uint8 level;
	};

	typedef std::vector<xlslib_core::style_t* XLSLIB_DFLT_ALLOCATOR> Style_Vect_t;
	typedef Style_Vect_t::iterator Style_Vect_Itor_t;

	class CStyle : public CRecord
	{
		friend class CDataStorage;

	protected:
		CStyle(CDataStorage &datastore, const style_t* styledef);
	private:
		virtual ~CStyle();
	};


/*
 ******************************
 * CBSheet class declaration
 ******************************
 */

#define BSHEET_OFFSET_POSITION    4
#define BSHEET_OFFSET_FLAGS       8
#define BSHEET_OFFSET_NAMELENGHT  10
#define BSHEET_OFFSET_B7NAME      11
#define BSHEET_OFFSET_B8NAME      12

#define BSHEET_ATTR_WORKSHEET  0x0000
#define BSHEET_ATTR_EX4MACRO   0x0001
#define BSHEET_ATTR_CHART      0x0002
#define BSHEET_ATTR_VBMODULE   0x0006

#define BSHEET_ATTR_VISIBLE     0x0000
#define BSHEET_ATTR_HIDDEN      0x0100
#define BSHEET_ATTR_VERYHIDDEN  0x0200

	class CBSheet;
	class CGlobalRecords;

	class boundsheet_t
	{
	public:
		boundsheet_t(CGlobalRecords& gRecords);
		boundsheet_t(CGlobalRecords& gRecords, const xlslib_strings::u16string& sheetname, uint16 attributes, uint32 streampos);
		virtual ~boundsheet_t();

	private:
		boundsheet_t(const boundsheet_t& that);
		boundsheet_t& operator=(const boundsheet_t& right);

	protected:
		xlslib_strings::u16string sheetname;
		uint32 streampos;
		bool worksheet : 1;
		bool ex4macro : 1;
		bool chart : 1;
		bool vbmodule : 1;
		bool visible : 1;
		bool hidden : 1;
		bool veryhidden : 1;

		CBSheet	*sheetData;

		CGlobalRecords& m_GlobalRecords;

	public:
		uint32 GetStreamPos(void) const { return streampos; }
		const xlslib_strings::u16string& GetSheetName(void) const
		{
			return sheetname;
		}

		void SetAttributes(uint16 attributes);
		bool IsWorkSheet(void) const { return worksheet; }
		bool IsEx4macro(void) const { return ex4macro; }
		bool IsChart(void) const { return chart; }
		bool IsVBModule(void) const { return vbmodule; }
		bool IsVisible(void) const { return visible; }
		bool IsHidden(void) const { return hidden; }
		bool IsVeryHidden(void) const { return veryhidden; }

		CBSheet *SetSheetData(CBSheet *sh)
		{
			sheetData = sh;
			return sh;
		}

		void SetSheetStreamPosition(size_t offset);
		const CBSheet *GetSheetData(void) const { return sheetData; }

		CGlobalRecords& GetGlobalRecords(void) const { return m_GlobalRecords; }
	};

	typedef std::vector<xlslib_core::boundsheet_t* XLSLIB_DFLT_ALLOCATOR> Boundsheet_Vect_t;
	typedef Boundsheet_Vect_t::iterator Boundsheet_Vect_Itor_t;

	class CBSheet : public CRecord
	{
		friend class CDataStorage;

	protected:
		CBSheet(CDataStorage &datastore, const boundsheet_t* bsheetdef);
	private:
		virtual ~CBSheet();

	public:
		void SetStreamPosition(size_t pos);
	};
}

// #include "xls_poppack.h"

#endif
