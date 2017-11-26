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

#ifndef DOCSUMMARYINFO_H
#define DOCSUMMARYINFO_H

// All known fields, but not all used
#define DocSumInfo_Dictionary                0
#define DocSumInfo_CodePage                  1
#define DocSumInfo_Category                  2
#define DocSumInfo_PresentationTarget        3
#define DocSumInfo_Bytes                     4
#define DocSumInfo_Lines                     5
#define DocSumInfo_Paragraphs                6
#define DocSumInfo_Slides                    7
#define DocSumInfo_Notes                     8
#define DocSumInfo_HiddenSlides              9
#define DocSumInfo_MMClips					10
#define DocSumInfo_ScaleCrop				11
#define DocSumInfo_HeadingPairs				12
#define DocSumInfo_TitlesofParts			13
#define DocSumInfo_Manager					14
#define DocSumInfo_Company					15
#define DocSumInfo_LinksUpToDate			16
#define DocSumInfo_Max						DocSumInfo_LinksUpToDate


// #include "xls_pshpack2.h"

namespace xlslib_core
{
	extern const int32 property2docSummary[];

	/*
	 ********************************
	 *  CDocSummaryInfo class declaration
	 ********************************
	 */

	class CDocSummaryInfo { //: public CDataStorage
	private:
		static const uint8 doc_summ_info_data[];
		hpsf_doc_t *hpsf;
	public:
		CDocSummaryInfo();
		virtual ~CDocSummaryInfo();
		bool property(property_t prop, const std::string& content);
		int DumpData(CDataStorage &datastore);
	private:
		CDocSummaryInfo(const CDocSummaryInfo& that);
		CDocSummaryInfo& operator=(const CDocSummaryInfo& right);
	};
}

// #include "xls_poppack.h"

#endif
