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

#ifndef GLOBALREC_H
#define GLOBALREC_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
/*
 ***********************************
 *  CGlobalRecords class declaration
 ***********************************
 */

#define GLOBAL_NUM_DEFAULT_FONT    5
#define GLOBAL_NUM_DEFAULT_FORMATS 8
#define GLOBAL_NUM_DEFAULT_XFS     16
#define GLOBAL_NUM_DEFAULT_STYLES  6
#define GLOBAL_INVALID_STORE_INDEX ((int32)0x80000000)

	class xf_t;

	typedef enum
	{
		GLOBAL_INIT = 0,
		GLOBAL_BOF,
		GLOBAL_CODEPAGE,
		GLOBAL_WINDOW1,
		GLOBAL_DATEMODE,
		GLOBAL_DEFAULTFONTS,
		GLOBAL_FONTS,
		GLOBAL_FORMATS,
		GLOBAL_DEFAULTXFS,
		GLOBAL_XFS,
		GLOBAL_STYLES,
		GLOBAL_PALETTE,
		GLOBAL_BOUNDSHEETS,
        GLOBAL_LINK1,
        GLOBAL_LINK2,
		GLOBAL_SST,
		GLOBAL_EOF,
		GLOBAL_FINISH
	} GlobalRecordDumpState_t;

	class CGlobalRecords : public CBiffSection {
		friend class cell_t;

	public:
		CGlobalRecords();
		~CGlobalRecords();
//      static		CGlobalRecords& Instance();
//      static void	Clean();
#if defined(HAVE_WORKING_ICONV)
		void		  SetIconvCode(const std::string& code){iconv_code=code; }
#endif
		void		  AddBoundingSheet(uint32 streampos,
									   uint16 attributes,
									   xlslib_strings::u16string& sheetname
									   );
		void		  AddBoundingSheet(boundsheet_t* bsheetdef);
		void		  AddFont(font_t* newfont);
		void		  AddFormat(format_t*);
		void		  AddXFormat(xf_t* xf);
		void		  AddLabelSST(const label_t& label);
		size_t		  GetLabelSSTIndex(const label_t& labeldef);
		void		  DeleteLabelSST(const label_t& label);

		bool		  SetColor(uint8 r, uint8 g, uint8 b, uint8 idx);

		void		  GetBoundingSheets(Boundsheet_Vect_Itor_t &bs);

		// A Kind of state machine that will return
		// a non-null pointer to the data unit until all data has been retrieved.
		CUnit*		  DumpData(CDataStorage &datastore);

		Boundsheet_Vect_Itor_t GetFirstBoundSheet();
		Boundsheet_Vect_Itor_t GetEndBoundSheet();
		Boundsheet_Vect_Itor_t GetBoundSheetAt(uint32 idx);

		font_t*		  GetDefaultFont() const;
		xf_t*		  GetDefaultXF() const;
		window1&	  GetWindow1() {return m_window1; }

		font_t*		  fontdup(uint8 fontnum) const;

		size_t		  EstimateNumBiffUnitsNeeded4Header(void);


		void wide2str16(const xlslib_strings::ustring& str1, xlslib_strings::u16string& str2);
		void char2str16(const std::string& str1, xlslib_strings::u16string& str2);
		void str16toascii(const xlslib_strings::u16string& str1, std::string& str2);

		static bool IsASCII(const std::string& str);
		static bool IsASCII(const xlslib_strings::u16string& str);

	private:
		CGlobalRecords(const CGlobalRecords& that);
		CGlobalRecords& operator=(const CGlobalRecords& right);

	protected:
		xf_t* findXF(xf_t *);

	private:
		Font_Vect_t	m_Fonts;
		Font_Vect_t	m_DefaultFonts;
		Format_Vect_t m_Formats;
		XF_Vect_t m_XFs;
		XF_Vect_t m_DefaultXFs;
		Style_Vect_t m_Styles;
		Boundsheet_Vect_t m_BoundSheets;
		Label_Vect_t m_Labels;        // SST strings
		window1	m_window1;
		colors_t m_palette;

		xf_t *defaultXF;         // 15th xfFormat is the default cell format

#ifdef HAVE_WORKING_ICONV
		std::string	iconv_code;
#endif
		// State Machine variable
		GlobalRecordDumpState_t m_DumpState;

		/*
		 *  static const font_init_t   m_Default_Fonts[GLOBAL_NUM_DEFAULT_FONT];
		 *  static const format_t m_Default_Formats[GLOBAL_NUM_DEFAULT_FORMATS];
		 *  static const xf_init_t     m_Default_XFs[GLOBAL_NUM_DEFAULT_XFS];
		 *  static const style_t  m_Default_Styles[GLOBAL_NUM_DEFAULT_STYLES];
		 */

		Font_Vect_Itor_t font;
		Font_Vect_Itor_t font_dflt;
		uint16 fontIndex;

		uint16 formatIndex;
		Format_Vect_Itor_t format;

		XF_Vect_Itor_t xf;
		XF_Vect_Itor_t xf_dflt;
		uint16 xfIndex;

		Style_Vect_Itor_t style;
		Boundsheet_Vect_Itor_t bsheet;
		Label_Vect_Itor_t label;
	};
}

// #include "xls_poppack.h"

#endif
