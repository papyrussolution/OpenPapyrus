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

#define CHANGE_DUMPSTATE(state) { m_PreviousDumpState = m_DumpState; m_DumpState = state; }

/*
 **********************************************************************
 *  workbook class implementation
 **********************************************************************
 */
workbook::workbook() :
	m_GlobalRecords(),
	m_ExprFactory(m_GlobalRecords),
	m_SummaryInfo(),
	m_DocSummaryInfo(),
	m_Sheets(),
	m_DumpState(WB_INIT),
	m_PreviousDumpState(WB_FINISH),
	m_sheetIndex(0),
	m_pCurrentData(NULL),
	m_pContinueRecord(NULL),
	m_ContinueIndex(0),
	m_writeLen(0),
	m_offset(0),
	m_current_sheet(0)
{
#if defined(HAVE_WORKING_ICONV)
	m_GlobalRecords.SetIconvCode("wchar_t");
#endif
}

workbook::~workbook()
{
	if(!m_Sheets.empty()) {
		for(size_t i = 0; i<m_Sheets.size(); i++) {
			delete m_Sheets[i];
		}
	}
	//  CGlobalRecords::Clean();
}

#ifdef HAVE_WORKING_ICONV
int workbook::iconvInType(const char *inType)
{
	int	ret;
	iconv_t	cd;

	cd = iconv_open(UCS_2_INTERNAL, inType);
	if(cd != (iconv_t)(-1)) {
		iconv_close(cd);
		m_GlobalRecords.SetIconvCode(inType);
		ret = 0;
	} else {
		ret = errno;
	}
	return ret;
}
#endif

worksheet* workbook::sheet(const string& sheetname)
{
	u16string str16;

	worksheet* pnewsheet = new worksheet(m_GlobalRecords, m_sheetIndex++);
	m_GlobalRecords.char2str16(sheetname, str16);

	m_Sheets.push_back(pnewsheet);

	// NOTE: Streampos defaults to 0
	// It has to be set somewhere else
	m_GlobalRecords.AddBoundingSheet(0, BSHEET_ATTR_WORKSHEET, str16);

	// Return a pointer to the just added sheet
	return m_Sheets.back();
}

worksheet* workbook::sheet(const ustring& sheetname)
{
	u16string str16;

	worksheet* pnewsheet = new worksheet(m_GlobalRecords, m_sheetIndex++);
	m_GlobalRecords.wide2str16(sheetname, str16);

	m_Sheets.push_back(pnewsheet);

	// NOTE: Streampos defaults to 0
	// It has to be set somewhere else
	m_GlobalRecords.AddBoundingSheet(0, BSHEET_ATTR_WORKSHEET, str16);

	// Return a pointer to the just added sheet
	return m_Sheets.back();
}

worksheet* workbook::GetSheet(uint16 sheetnum)
{
	if(sheetnum < m_Sheets.size()) {
		return m_Sheets[sheetnum];
	} else {
		return NULL;
	}
}

expression_node_factory_t& workbook::GetFormulaFactory(void)
{
	return m_ExprFactory;
}

font_t* workbook::font(const string& name)
{
	font_t* newfont = new font_t(m_GlobalRecords);
	newfont->SetName(name);
	m_GlobalRecords.AddFont(newfont);
	return newfont;
}

font_t* workbook::font(uint8 fontnum)
{
	return m_GlobalRecords.fontdup(fontnum);
}

format_t* workbook::format(const string& formatstr)
{
	u16string str16;
	format_t* newformat;
	m_GlobalRecords.char2str16(formatstr, str16);
	newformat = new format_t(m_GlobalRecords, str16);
	m_GlobalRecords.AddFormat(newformat);
	return newformat;
}

format_t* workbook::format(const ustring& formatstr)
{
	u16string str16;
	format_t* newformat;
	m_GlobalRecords.wide2str16(formatstr, str16);
	newformat = new format_t(m_GlobalRecords, str16);
	m_GlobalRecords.AddFormat(newformat);
	return newformat;
}

xf_t* workbook::xformat(void)
{
	xf_t* newxf = new xf_t(m_GlobalRecords, true);      // bool userXF=true, bool isCell=true, bool isMasterXF=false
	return newxf;
}

xf_t* workbook::xformat(font_t* fnt)
{
	xf_t* newxf = new xf_t(m_GlobalRecords, true);
	newxf->SetFont(fnt);
	return newxf;
}

xf_t* workbook::xformat(format_t* fmt)
{
	xf_t* newxf = new xf_t(m_GlobalRecords, true);
	newxf->SetFormat(fmt);
	return newxf;
}

xf_t* workbook::xformat(font_t* fnt, format_t* fmt)
{
	xf_t* newxf = new xf_t(m_GlobalRecords, true);
	newxf->SetFont(fnt);
	newxf->SetFormat(fmt);
	return newxf;
}

bool workbook::setColor(uint8 r, uint8 g, uint8 b, uint8 idx)
{
	return m_GlobalRecords.SetColor(r, g, b, idx);
}

bool workbook::property(property_t prop, const string& content)
{
	// who gets it?
	if(prop >= PROP_LAST) {
		return false; 
	}
	if(property2summary[prop] > 0) {
		return m_SummaryInfo.property(prop, content);
	} 
	else if(property2docSummary[prop] > 0) {
		return m_DocSummaryInfo.property(prop, content);
	} 
	else {
		return false;
	}
}

void workbook::windPosition(uint16 horz, uint16 vert) { m_GlobalRecords.GetWindow1().SetPosition(horz, vert); }
void workbook::windSize(uint16 width, uint16 height) { m_GlobalRecords.GetWindow1().SetSize(width, height); }
void workbook::firstTab(uint16 fTab) { m_GlobalRecords.GetWindow1().SetFirstTab(fTab); }
void workbook::tabBarWidth(uint16 width) { m_GlobalRecords.GetWindow1().SetTabBarWidth(width); }

int workbook::Dump(const string& filename)
{
	Sheets_Vector_Itor_t sBegin, sEnd, sIter;
	size_t cells;
	string name;
	int	errors;
	COleDoc dst;

	if(m_Sheets.empty()) {
		return GENERAL_ERROR;
	}

	// pre-allocate an approximation of what will be needed to store the data objects
	sBegin	= m_Sheets.begin();
	sEnd	= m_Sheets.end();
	cells	= 0;
	/*
	 *  Since it's VERY costly to redimension the cell unit store vector when
	 *  we're using lightweight CUnitStore elements et al
	 *  we do our utmost best to estimate the total amount of storage units
	 *  required to 'dump' our spreadsheet. The estimate should be conservative,
	 *  but not too much. After all, we're attempting to reduce the memory
	 *  footprint for this baby when we process multi-million cell spreadsheets...
	 */
	for(sIter=sBegin; sIter<sEnd; ++sIter) {
		// add a number of units for each worksheet,
		cells += (*sIter)->EstimateNumBiffUnitsNeeded();
	}
	// global units:
	cells += this->m_GlobalRecords.EstimateNumBiffUnitsNeeded4Header();
	cells += 1000; // safety margin

	XTRACE2("ESTIMATED: total storage unit count: ", cells);
#if OLE_DEBUG
	std::cerr << "ESTIMATED: total unit count: " << cells << std::endl;
#endif

	errors = dst.Open(filename);

	if(errors == NO_ERRORS)	{
		CDataStorage biffdata(cells);
		CUnit*				precorddata;
		bool keep = true;

		do {
			precorddata = DumpData(biffdata);

			if(precorddata != NULL) {
				biffdata += precorddata;
				// and we can already discard any previous units at lower backpatch levels
				biffdata.FlushLowerLevelUnits(precorddata);
			} else {
				keep = false;
			}
		} while(keep);

		dst.AddFile("/Workbook", &biffdata);

		CDataStorage summarydata;
		name = (char)0x05;
		name += "SummaryInformation";
		m_SummaryInfo.DumpData(summarydata);
		dst.AddFile(name, &summarydata);

		CDataStorage docdata;
		name = (char)0x05;
		name += "DocumentSummaryInformation";
		m_DocSummaryInfo.DumpData(docdata);
		dst.AddFile(name, &docdata);

		errors = dst.DumpOleFile();
		dst.Close();
	}
	return errors;
}

CUnit* workbook::DumpData(CDataStorage &datastore)
{
	bool repeat = false;

	XTRACE("\nworkbook::DumpData");

	do {
		switch(m_DumpState) {
		case WB_INIT:
			XTRACE("\tWB_INIT");

			m_writeLen = 0;
			m_current_sheet = 0;
			m_offset = 0;

			CHANGE_DUMPSTATE(WB_GLOBALRECORDS);

			repeat = true;
			break;

		case WB_GLOBALRECORDS:
			XTRACE("\tGLOBALRECORDS");

			m_pCurrentData = m_GlobalRecords.DumpData(datastore);
			if(m_pCurrentData == NULL) {
				m_offset = m_writeLen;
				m_writeLen = 0;
				//Last_BOF_offset = 0;

				repeat = true;
				CHANGE_DUMPSTATE(WB_SHEETS);
			} else {
				m_writeLen += m_pCurrentData->GetDataSize();
				// Do nothing. Continue in this state.
				repeat = false;
			}
			break;

		case WB_SHEETS:
		{
			XTRACE("\tSHEETS");

			//printf("DUMP SHEETS WITH DATASIZE=%ld m_offset=%ld m_writeLen=%ld\n", datastore.GetDataSize(), m_offset, m_writeLen );
			m_pCurrentData = m_Sheets[m_current_sheet]->DumpData(datastore, m_offset, m_writeLen /*, Last_BOF_offset*/);   // writelen passed as its cumulatively increased
			if(m_pCurrentData == NULL) {
				Boundsheet_Vect_Itor_t bs = m_GlobalRecords.GetBoundSheetAt(m_current_sheet);

				(*bs)->SetSheetStreamPosition(m_offset);

				if((m_current_sheet+1) < (uint16)m_Sheets.size()) { // [i_a]
					// Update the m_offset for the next sheet
					m_offset += m_writeLen;
					m_writeLen = 0;
					m_current_sheet++;
				} else {
					// I'm done with all the sheets
					CHANGE_DUMPSTATE(WB_FINISH);
				}
				repeat = true;
			} else {
				m_writeLen += m_pCurrentData->GetDataSize();
				repeat = false;
			}
		}  break;

		case WB_FINISH:
			XTRACE("\tFINISH");
			repeat = false;
			m_pCurrentData  = NULL;
			CHANGE_DUMPSTATE(WB_INIT);
			break;
		case WB_CONTINUE_REC:
			XTRACE("\tCONTINUE-REC");

			if(m_ContinueIndex == 0) {
				//Create a new data unit containing the max data size
				m_ContinuesRealRecordSize = datastore.Clip((CRecord*)m_pCurrentData);
				//m_pContinueRecord->SetValueAt(MAX_RECORD_SIZE-4,2);
				m_ContinueIndex++;
				return m_pCurrentData;
			} 
			else {
				//Delete_Pointer(m_pContinueRecord);

				// Get a pointer to the next chunk of data
				const uint8* pdata = (((CRecord*)m_pCurrentData)->GetRecordDataBuffer()) + m_ContinueIndex*MAX_RECORD_SIZE;
				// Get the size of the chunk of data (that is the MAX_REC_SIZE except by the last one)
				size_t csize = 0;
				if((m_ContinuesRealRecordSize/MAX_RECORD_SIZE) > m_ContinueIndex) {
					csize = MAX_RECORD_SIZE;
					m_ContinueIndex++;
					m_pContinueRecord = datastore.MakeCContinue(m_pCurrentData, pdata, csize);
					if(m_PreviousDumpState == WB_SHEETS) { m_writeLen += RECORD_HEADER_SIZE; }
					return m_pContinueRecord;
				} 
				else {
					CUnit *unit = m_pCurrentData;
					csize = m_ContinuesRealRecordSize - m_ContinueIndex * MAX_RECORD_SIZE;

					// Restore the previous state (*Don't use the macro*)
					m_DumpState = m_PreviousDumpState;
					m_PreviousDumpState = WB_CONTINUE_REC;
					m_pCurrentData = NULL;
					m_ContinueIndex = 0;
					// done with it now, so can delete it
					// Delete_Pointer(m_pCurrentData);

					if(csize) {
						m_pContinueRecord = datastore.MakeCContinue(unit, pdata, csize);
						if(m_PreviousDumpState == WB_SHEETS) { m_writeLen += RECORD_HEADER_SIZE; }
						return m_pContinueRecord;
					} 
					else {
						repeat = true;
					}
				}
			}

			break;
		default:
			XTRACE("\tDEFAULT");
			break;
		}
		if(m_pCurrentData != NULL) {
			// SST Table most likely record to exceed size, but its handled now in the record itself (breaks have to occur at defined places)
			// Should only happen with single cells having data > MAX_RECORD_SIZE. Have no idea if this works or not (DFH)
			if(!((CRecord*)m_pCurrentData)->AlreadyContinued() && ((CRecord*)m_pCurrentData)->GetRecordDataSize() > MAX_RECORD_SIZE && m_DumpState !=
			   WB_CONTINUE_REC) {
				// Save the current dump state and change to the CONTINUE Record state
				CHANGE_DUMPSTATE(WB_CONTINUE_REC);
				//printf("ALREADY=%d dataSize=%lu MAX=%d (dumpState == CONTINUE) = %d\n", ((CRecord*)m_pCurrentData)->AlreadyContinued(), ((CRecord*)m_pCurrentData)->GetRecordDataSize(), MAX_RECORD_SIZE ,m_DumpState == WB_CONTINUE_REC);
				m_ContinueIndex = 0;
				repeat = true;
			}
		}
	} while(repeat);
	return m_pCurrentData;
}
