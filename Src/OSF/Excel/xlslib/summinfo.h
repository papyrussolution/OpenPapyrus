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

#ifndef SUMMARYINFO_H
#define SUMMARYINFO_H

// #include "xls_pshpack2.h"

// all of these defined, not all used
#define SumInfo_Unknown                      1
#define SumInfo_Title                        2
#define SumInfo_Subject                      3
#define SumInfo_Author                       4
#define SumInfo_Keywords                     5
#define SumInfo_Comments                     6
#define SumInfo_Template                     7
#define SumInfo_LastSavedBy                  8
#define SumInfo_RevisionNumber               9
#define SumInfo_TotalEditingTime			10
#define SumInfo_LastPrinted					11
#define SumInfo_CreateTime_Date				12
#define SumInfo_LastSavedTime_Date			13
#define SumInfo_NumberofPages				14
#define SumInfo_NumberofWords				15
#define SumInfo_NumberofCharacters			16
#define SumInfo_Thumbnail					17
#define SumInfo_NameofCreatingApplication	18
#define SumInfo_Security					19
#define		READONLY_RECOMMENDED			0x02
#define		READONLY_ENFORCED				0x04
#define SumInfo_Max							SumInfo_Security

namespace xlslib_core
{
	typedef enum {
		PROP_AUTHOR = 1,
		PROP_CATEGORY,
		PROP_COMMENTS,
		PROP_COMPANY,
		PROP_CREATINGAPPLICATION,   // Cannot see anywhere this is displayed [DFH: need the enum to increase by 1]
		PROP_KEYWORDS,
		PROP_MANAGER,
		PROP_REVISION,
		PROP_SUBJECT,
		PROP_TITLE,

		PROP_LAST
	} property_t;

	extern const int32 property2summary[];
	/*
	 ********************************
	 *  CSummaryInfo class declaration
	 ********************************
	 */
	class CSummaryInfo //: public CDataStorage
	{
	private:
		static const uint8 summ_info_data[];
		hpsf_doc_t *hpsf;

	public:
		CSummaryInfo();
		virtual ~CSummaryInfo();

		bool property(property_t prop, const std::string& content);
		int DumpData(CDataStorage &datastore);

	private:
		CSummaryInfo(const CSummaryInfo& that);
		CSummaryInfo& operator=(const CSummaryInfo& right);
	};
}

// #include "xls_poppack.h"

#endif
