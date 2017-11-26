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

#include "xlslib-internal.h"
#pragma hdrstop

using namespace xlslib_core;
using namespace xlslib_strings;

const int32 xlslib_core::property2summary[] =
{
0,
SumInfo_Author,
-1,
SumInfo_Comments,
-1,
SumInfo_NameofCreatingApplication,                  // Does not seem to do anything
SumInfo_Keywords,
-1,
SumInfo_RevisionNumber,
SumInfo_Subject,
SumInfo_Title
};

/*
 **********************************************************************
 *  CSummaryInfo class implementation
 **********************************************************************
 */
CSummaryInfo::CSummaryInfo()
{
	uint64 msTime;
	string s;
	XTRACE("WRITE_SUMMARY");
	hpsf = new hpsf_doc_t(HPSF_SUMMARY);
	if(hpsf) {
		msTime = hpsf->unix2mstime(time(NULL));
		hpsf->addItem(SumInfo_Unknown, (uint16)1200);                 // Excel 2004 on Mac writes this 0xfde9
		hpsf->addItem(SumInfo_CreateTime_Date, msTime);                     // should be "right now"
		hpsf->addItem(SumInfo_LastSavedTime_Date, msTime);                  // should be "right now"
		hpsf->addItem(SumInfo_Security, (uint32)0);                   // Default
		hpsf->addItem(SumInfo_NameofCreatingApplication, s = PACKAGE_NAME); // Default
	}
}

CSummaryInfo::~CSummaryInfo()
{
	delete hpsf;
}

bool CSummaryInfo::property(property_t prop, const string& content)
{
	uint16 val = static_cast<uint16>(property2summary[prop]);
	hpsf->addItem(val, content);
	return true;
}

int CSummaryInfo::DumpData(CDataStorage &datastore)
{
	XTRACE("\tCSummaryInfo::DumpData");

	CUnit* ret = hpsf->GetData(datastore);
	if(ret != NULL) {
		datastore += ret;
		// hpsf = NULL;	// DataStore owns it now
	}
	return NO_ERRORS;
}

