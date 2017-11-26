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

//#define __DEBUG__

#include "xlslib-internal.h"
#pragma hdrstop

using namespace xlslib_core;
using namespace xlslib_strings;

#define MAX_ROWBLOCK_SIZE            16
// was: 32, but CONTINUE-d DBCELLs are not liked by 2003 ???

#define RB_DBCELL_MINSIZE             8
#define RB_DBCELL_CELLSIZEOFFSET      2

// People using xlslib with at least 1200 columns maybe more - bug fix to such a file - so this comment has to be incorrect for current Excel versions
#define MAX_COLUMNS_PER_ROW			256
// (out of date): Excel 2003 limit: 256 columns per row. (Update this when we upgrade this lib to support BIFF12 !)

// Adapted from Workbook macro - useful if we need to add log messages etc
#define CHANGE_DUMPSTATE(state) {				\
		m_DumpState = state;						 \
}

#define DEFAULT_ROW_HEIGHT (300/TWIP)

/*
 **********************************************************************
 *  worksheet class implementation
 **********************************************************************
 */
worksheet::worksheet(CGlobalRecords& gRecords, uint16 idx) :
	m_GlobalRecords(gRecords),
	m_DumpState(SHEET_INIT),
	m_pCurrentData(NULL),

	m_MergedRanges(),
	m_Colinfos(),
	m_Current_Colinfo(),
	m_RowHeights(),
	m_Current_RowHeight(),
	minRow((uint32)(-1)), minCol((uint32)(-1)),
	maxRow(0), maxCol(0),
	sheetIndex(idx),
	m_Cells(),
	m_CurrentCell(),
	m_CurrentSizeCell(),
	m_Ranges(),
	m_RBSizes(),
	m_Current_RBSize(),
	m_SizesCalculated(false),
	m_DumpRBState(RB_INIT),
	m_RowCounter(0),
	m_CellCounter(0),
	m_DBCellOffset(0),
	m_FirstRowOffset(0),
	m_CellOffsets(),
	m_Starting_RBCell(),
	cellIterHint(),
	cellHint(NULL),
	defRowsHidden(false),
	defRowHeight(DEFAULT_ROW_HEIGHT),   // MS default
	defColWidth(10),    // MS default
    m_FormulaStacks(),
    m_DataValidations(),
    m_CurrentDval(),
	m_HyperLinks(),
	m_CurrentHlink()
{
}

worksheet::~worksheet()
{
	// Delete the dynamically created cell objects (pointers)
	if(!m_Cells.empty()) {
		// cout<<"worksheet::~worksheet(), this = "<<this<<endl;
		for(Cell_Set_Itor_t cell = m_Cells.begin(); cell != m_Cells.end(); cell++) {
			delete *cell;
		}
		m_Cells.clear();
	}

	// Delete dynamically allocated ranges of merged cells.
	if(!m_MergedRanges.empty()) {
		// cout<<"worksheet::~worksheet(), this = "<<this<<endl;
		for(Range_Vect_Itor_t mr = m_MergedRanges.begin(); mr != m_MergedRanges.end(); mr++) {
			delete *mr;
		}
		m_MergedRanges.clear();
	}

	// Delete dynamically allocated Cellinfo record definitions
	if(!m_Colinfos.empty()) {
		for(Colinfo_Set_Itor_t ci = m_Colinfos.begin(); ci != m_Colinfos.end(); ci++) {
			delete *ci;
		}
		m_Colinfos.clear();
	}

/*
 *  if(!m_Colinfos.empty())
 *  {
 *  do
 *  {
 *  delete m_Colinfos.front();
 *  m_Colinfos.pop_front();
 *  }while(!m_Colinfos.empty());
 *
 *  }
 */

	// Delete dynamically allocated Cellinfo record definitions
	if(!m_RowHeights.empty()) {
		for(RowHeight_Vect_Itor_t rh = m_RowHeights.begin(); rh != m_RowHeights.end(); rh++) {
			delete *rh;
		}
		m_RowHeights.clear();
	}

/*
 *  if(!m_RowHeights.empty())
 *  {
 *  do
 *  {
 *  delete m_RowHeights.front();
 *  m_RowHeights.pop_front();
 *  }while(!m_RowHeights.empty());
 *
 *  }
 */
	// Delete dynamically allocated range definitions
	if(!m_Ranges.empty()) {
		for(RangeObj_Vect_Itor_t rh = m_Ranges.begin(); rh != m_Ranges.end(); rh++) {
			delete *rh;
		}
		m_Ranges.clear();
	}

    if (!m_FormulaStacks.empty()) {
		for(FormulaStackList_Itor_t rh = m_FormulaStacks.begin(); rh != m_FormulaStacks.end(); rh++) {
			delete *rh;
		}
		m_FormulaStacks.clear();
    }

	if(!m_DataValidations.empty()) {
		for(DataValidationList_Itor_t rh = m_DataValidations.begin(); rh != m_DataValidations.end(); rh++) {
			delete *rh;
		}
		m_DataValidations.clear();
	}

	if(!m_HyperLinks.empty()) {
		for(HyperLinkList_Itor_t rh = m_HyperLinks.begin(); rh != m_HyperLinks.end(); rh++) {
			delete *rh;
		}
		m_HyperLinks.clear();
	}
}

#define RB_INDEX_MINSIZE 20

CUnit* worksheet::DumpData(CDataStorage &datastore, size_t offset, size_t writeLen /*, size_t &Last_BOF_offset*/)
{
	bool repeat = false;
	bool changeDumpState;

	XTRACE("worksheet::DumpData");

	do {
		//printf("OFFSET=%ld writeLen=%ld DATASIZE=%ld\n", offset, writeLen, datastore.GetDataSize());
		switch(m_DumpState) {
		case SHEET_INIT:
			m_Current_Colinfo = m_Colinfos.begin();
			m_CurrentSizeCell = m_Cells.begin();
			m_CurrentCell = m_Cells.begin();
			m_CurrentHlink = m_HyperLinks.begin();
			m_CurrentDval = m_DataValidations.begin();
			m_Current_Range = m_MergedRanges.begin();
			repeat  = true;
			CHANGE_DUMPSTATE(SHEET_BOF);
			break;

		case SHEET_BOF:
			XTRACE("\tBOF");
			m_pCurrentData = datastore.MakeCBof(BOF_TYPE_WORKSHEET);
			//Last_BOF_offset = offset + writeLen;
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_INDEX);
			break;

		case SHEET_INDEX:
			XTRACE("\tINDEX");
			{
				// printf("INDEX=%ld 0x%lx\n", offset, offset);

				// NOTE: GetNumRowBlocks() has the side-effect of coughing up first/last col/row for the given sheet :-)
				rowblocksize_t rbsize;
				size_t numrb = GetNumRowBlocks(&rbsize);

				// printf("NUM BLOCKS=%ld\n", (long)numrb);
				// Get first/last row from the list of row blocks
				//uint32 first_row, last_row;
				//GetFirstLastRowsAndColumns(&first_row, &last_row, NULL, NULL);

				uint32 colInfoSize = (uint32)m_Colinfos.size() * COL_INFO_SIZE;
				m_pCurrentData = datastore.MakeCIndex(rbsize.first_row, rbsize.last_row);

				size_t rb_size_acc = 0;
				size_t index_size = RB_INDEX_MINSIZE + 4*numrb;
				size_t dbcelloffset = offset + writeLen + index_size + DEF_ROW_HEIGHT_SIZE + colInfoSize;
				((CIndex*)m_pCurrentData)->AddDBCellOffset(dbcelloffset);
				// printf("GUESS DIMENSION IS AT %ld\n", (long)dbcelloffset);

				dbcelloffset += DEF_COL_WIDTH_SIZE + DIMENSION_SIZE;

				for(size_t rb = 0; rb < numrb; rb++) {
					// Get sizes of next RowBlock
					rowblocksize_t rbsize2;
#if defined(XL_WITH_ASSERTIONS)
					bool state =
#else
					(void)
#endif
						GetRowBlockSizes(rbsize2);
					XL_ASSERT(rb == numrb - 1 ? state == false : state == true);
					// Update the offset accumulator and create the next DBCELL's offset
					rb_size_acc += rbsize2.rowandcell_size;

					dbcelloffset += rb_size_acc;
					//printf("GUESS DBCELL AT %ld\n", (long)dbcelloffset);
					((CIndex*)m_pCurrentData)->AddDBCellOffset(dbcelloffset);

					// Update the offset for the next DBCELL's offset
					rb_size_acc += rbsize2.dbcell_size;
				}
				repeat = false;
				CHANGE_DUMPSTATE(SHEET_DFLT_ROW_HEIGHT); // Change to the next state
			}
			break;

		case SHEET_DFLT_ROW_HEIGHT:
			XTRACE("\tSHEET_DFLT_ROW_HEIGHT");
			m_pCurrentData = datastore.MakeCRecord();
			((CRecord  *)m_pCurrentData)->SetRecordType(RECTYPE_DEFAULTROWHEIGHT);
			((CRecord  *)m_pCurrentData)->SetRecordLength(4);
			((CRecord  *)m_pCurrentData)->AddValue16(defRowHeight == DEFAULT_ROW_HEIGHT ? 0 : 0x01);
			((CRecord  *)m_pCurrentData)->AddValue16(defRowHeight * TWIP);
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_COLINFO); // Change to the next state
			break;

		case SHEET_COLINFO:
			XTRACE2("\tCOLINFO %d", (int)m_Colinfos.size());
			if(!m_Colinfos.empty())	{
				// First check if the list of fonts is not empty... (old comment????)
				m_pCurrentData = datastore.MakeCColInfo(*m_Current_Colinfo++);
				changeDumpState = m_Current_Colinfo == m_Colinfos.end() ? true : false;
				repeat = false;
			} else {
				changeDumpState = true;
				repeat = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_DFLT_COL_WIDTH);
			}
			break;

		case SHEET_DFLT_COL_WIDTH:
			XTRACE("\tSHEET_DFLT_COL_WIDTH");
			m_pCurrentData = datastore.MakeCRecord();
			((CRecord  *)m_pCurrentData)->SetRecordType(RECTYPE_DEFCOLWIDTH);
			((CRecord  *)m_pCurrentData)->SetRecordLength(2);
			((CRecord  *)m_pCurrentData)->AddValue16(defColWidth);
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_DIMENSION); // Change to the next state
			break;

		case SHEET_DIMENSION:
			XTRACE("\tDIMENSION");
			//printf("DIMENSION IS AT %ld\n", datastore.GetDataSize());
			//printf("    DIM %ld\n", (unsigned long)writeLen + offset);

			//Delete_Pointer(m_pCurrentData);
			m_pCurrentData = datastore.MakeCDimension(minRow, maxRow, minCol, maxCol);
			//printf("DIMENSION %d %d %d %d\n", minRow, maxRow, minCol, maxCol);
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_ROWBLOCKS);
			break;

		case SHEET_ROWBLOCKS:
			//printf("ROW_BLOCKS %ld\n", (long)writeLen + offset);
			XTRACE("\tROWBLOCKS");
			if(!m_Cells.empty()) { // was if(GetNumRowBlocks())
				// First check if the list of RBs is not empty...
				m_pCurrentData = RowBlocksDump(datastore, writeLen + offset);

				if(m_pCurrentData == NULL) {
					repeat = true;
					changeDumpState = true;
				} else {
					repeat = false;
					changeDumpState = false;
				}
			} else {
				// if the list is empty, change the dump state.
				repeat = true;
				changeDumpState = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_MERGED);
			}
			break;

		case SHEET_MERGED:
			XTRACE("\tMERGED");
			if(!m_MergedRanges.empty())	{
				size_t m_ranges = static_cast<size_t>(m_MergedRanges.end() - m_Current_Range);
				if(m_ranges) {
					m_pCurrentData = datastore.MakeCMergedCells();
					if(m_ranges > 1027) {
						m_ranges = 1027;
					}

					((CMergedCells*)m_pCurrentData)->SetNumRanges(m_ranges);
					for(size_t i=0; i<m_ranges; ++i) {
						((CMergedCells*)m_pCurrentData)->AddRange(*m_Current_Range++);
					}
					repeat = false;
					changeDumpState = false;
				} else {
					repeat = true;
					changeDumpState = true;
				}
			} else {
				// if the list is empty, change the dump state.
				repeat = true;
				changeDumpState = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_WINDOW2);
			}
			break;

		case SHEET_WINDOW2:
			XTRACE("\tWINDOW2");
			m_pCurrentData = datastore.MakeCWindow2(sheetIndex == m_GlobalRecords.GetWindow1().GetActiveSheet());
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_H_LINKS);
			break;

		case SHEET_H_LINKS:
			XTRACE("\tSHEET_H_LINKS");
			if(!m_HyperLinks.empty()) {
				// First check if the list of fonts is not empty...
				m_pCurrentData = MakeHyperLink(datastore, *m_CurrentHlink++);
				changeDumpState = m_CurrentHlink == m_HyperLinks.end() ? true : false;
				repeat = false;
			} else {
				// if the list is empty, change the dump state.
				changeDumpState = true;
				repeat = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_VALIDITY_HEADER);
			}
			break;

		case SHEET_VALIDITY_HEADER:
			XTRACE("\tSHEET_VALIDITY_HEADER");
			if(!m_DataValidations.empty()) {
				m_pCurrentData = MakeDataValidationHeader(datastore, (uint32)m_DataValidations.size());
				changeDumpState = true;
				repeat = false;
			} else {
				changeDumpState = true;
				repeat = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_VALIDITY_BODY);
			}
			break;

		case SHEET_VALIDITY_BODY:
			XTRACE("\tSHEET_VALIDITY_BODY");
			if(!m_DataValidations.empty()) {
				m_pCurrentData = MakeDataValidationEntry(datastore, *m_CurrentDval++);
				changeDumpState = m_CurrentDval == m_DataValidations.end() ? true : false;
				repeat = false;
			} else {
				changeDumpState = true;
				repeat = true;
			}
			if(changeDumpState) {
				CHANGE_DUMPSTATE(SHEET_EOF);
			}
			break;

		case SHEET_EOF:
			XTRACE("\tEOF");
			m_pCurrentData = datastore.MakeCEof();
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_FINISH);
			break;

		case SHEET_FINISH:
			XTRACE("\tFINISH");
			m_pCurrentData = NULL;
			repeat = false;
			CHANGE_DUMPSTATE(SHEET_INIT);
			break;
		}
	} while(repeat);

	return m_pCurrentData;
}

CUnit* worksheet::RowBlocksDump(CDataStorage &datastore, const size_t offset)
{
	bool repeat = false;
	CUnit* rb_record = NULL;

	do {
		switch(m_DumpRBState) {
		case RB_INIT:
			XTRACE("\t\tINIT");
			m_DumpRBState = RB_ROWS;
			//m_CurrentRowBlock = 0;
			m_RowCounter = 0;
			//m_CurrentCell = m_Cells.begin(); // moved by DFH to sheet init, should be OK

			// Initialize the row widths
			m_Current_RowHeight = m_RowHeights.begin();
			m_DumpRBState = RB_FIRST_ROW;

			repeat = true;
			break;

		case RB_FIRST_ROW:
			XTRACE("\t\tFIRST_ROW");
			repeat = true;

			XL_ASSERT(!m_Cells.empty());
			if(m_CurrentCell != m_Cells.end()) { // [i_a]
				m_Starting_RBCell = m_CurrentCell;
				m_CellCounter = 0;
				m_DBCellOffset = 0;
				m_FirstRowOffset = offset;
				m_CellOffsets.clear();

				m_DumpRBState = RB_ROWS;
			} else {
				m_DumpRBState = RB_FINISH;
			}
			break;

		case RB_ROWS:
			XTRACE("\t\tROWS");
			{
				repeat = false;

				// Initialize first/last cols to impossible values
				// that are appropriate for the following detection algorithm
				uint32 first_col = ~(uint32)0;
				uint32 last_col  = 0;
				uint32 row_num = (*m_CurrentCell)->GetRow();

				if (m_Current_RowHeight != m_RowHeights.end()) {
					if (row_num > (*m_Current_RowHeight)->GetRowNum()) {
						row_num = (*m_Current_RowHeight)->GetRowNum();
					}
				}

				// Get the row number for the current row;
				// The order in the conditional statement is important
				for (;
					 m_CurrentCell != m_Cells.end() && (*m_CurrentCell)->GetRow() == row_num;
					 m_CurrentCell++) {
					// Determine the first/last column of the current row
					uint32 col_num = (*m_CurrentCell)->GetCol();
					if(col_num > last_col) {
						last_col = col_num;
					}
					if(col_num < first_col) {
						first_col = col_num;
					}

					m_CellCounter++;
				}

				// when there are NO cells in this row...
				if (last_col < first_col) {
					last_col = 0;
					first_col = 1;
				}

				// Check if the current row is in the list of height-set
				// rows.
				if (m_Current_RowHeight != m_RowHeights.end()) {
					XL_ASSERT((*m_Current_RowHeight)->GetRowNum() >= row_num);
					if((*m_Current_RowHeight)->GetRowNum() == row_num) {
						//printf("ROW OFFSET %ld\n", datastore.GetDataSize());

						rb_record = datastore.MakeCRow(row_num, first_col,
								last_col,
								(*m_Current_RowHeight)->GetRowHeight(),
								(*m_Current_RowHeight)->GetXF());
						m_Current_RowHeight++;
					} else {
						rb_record = datastore.MakeCRow(row_num, first_col, last_col);
					}
				} else {
					//printf("ROW OFFSET %ld\n", datastore.GetDataSize());
					rb_record = datastore.MakeCRow(row_num, first_col, last_col);
				}

				m_DBCellOffset += ROW_RECORD_SIZE;
				XL_ASSERT(rb_record->GetDataSize() == ROW_RECORD_SIZE);
				//printf("m_DBCellOffset[rows]=%ld OFFSET=%ld\n", m_DBCellOffset, offset);
				// If the current row-block is full OR there are no more cells
				if(++m_RowCounter >= MAX_ROWBLOCK_SIZE || m_CurrentCell == m_Cells.end()) {
					if(m_CurrentCell == (--m_Cells.end())) {
						m_CellCounter++;
					}
					m_RowCounter = 0;
					m_DumpRBState = RB_FIRSTCELL;
				}
				break;
			}

		case RB_FIRSTCELL:
			XTRACE("\t\tFIRST_CELL");
			rb_record = (*m_Starting_RBCell)->GetData(datastore);

			// Update the offset to be used in the DBCell Record
			m_DBCellOffset += rb_record->GetDataSize();
			//printf("m_DBCellOffset[firstCell]=%ld OFFSET=%ld\n", m_DBCellOffset, offset);

			// The first cell of the rowblock has an offset that includes (among others)	NO
			// the rows size (without counting the first row)								NO
			//m_CellOffsets.push_back(m_DBCellOffset - ROW_RECORD_SIZE);
			m_CellOffsets.push_back(offset - m_FirstRowOffset);
			//printf("FIRST CELL Pushback=%ld\n", offset - m_FirstRowOffset);
			//printf("m_CellOffsets[firstCell]=%ld\n", offset - m_FirstRowOffset);

			// Update the pointer (iterator) to the next cell
			if(--m_CellCounter == 0) {
				// The RowBlock's cells are done
				m_DumpRBState = RB_DBCELL;
			} else {
				m_Starting_RBCell++;
				m_DumpRBState = RB_CELLS;
			}
			break;

		case RB_CELLS:
			XTRACE("\t\tCELLS");
			repeat = false;
			if(m_CellCounter == 0) {
				// The RowBlock's cells are done
				m_DumpRBState = RB_DBCELL;

				repeat = true;
			} else {
				rb_record = (*m_Starting_RBCell)->GetData(datastore);

				m_DBCellOffset += rb_record->GetDataSize();

				//printf("PUSH_BACK[RB_CELLS]=%ld\n", rb_record->GetDataSize());
				m_CellOffsets.push_back(rb_record->GetDataSize());
				m_CellCounter--;
				m_Starting_RBCell++;
			}
			break;

		case RB_DBCELL:
			XTRACE("\t\tDBCELL");
			{
				repeat = false;
				//printf("DBCELL OFFSET %ld\n", datastore.GetDataSize());
				CDBCell* rec = datastore.MakeCDBCell(m_DBCellOffset);
				rb_record = rec;
				//printf("  DBCELL[1st Entry]=%ld\n", m_DBCellOffset);
				CellOffsets_Vect_Itor_t celloffset;
				for(celloffset = m_CellOffsets.begin(); celloffset != m_CellOffsets.end(); celloffset++) {
					rec->AddRowOffset(*celloffset);
					//printf("  DBOFF=%ld\n", *celloffset);
				}

				if(m_CurrentCell == (--m_Cells.end()) ) {
					m_DumpRBState = RB_FINISH;
				} else {
					m_DumpRBState =  RB_FIRST_ROW;
				}

				break;
			}
		case RB_FINISH:
			XTRACE("\t\tFINISH");

			repeat = false;
			rb_record = NULL;
			m_DumpRBState =  RB_INIT;
			break;

		default:
			break;
		}
	} while(repeat);

//XL_ASSERT(rb_record->GetDataSize() <= MAX_RECORD_SIZE);
	return rb_record;
}

size_t worksheet::EstimateNumBiffUnitsNeeded(void)
{
	size_t ret = 7;

	ret += m_Colinfos.size();
	ret += m_RowHeights.size();
	ret += m_MergedRanges.size();
	/*
	 *  and also add units for the number of rows: those DBCELL and ROW records...
	 *
	 *  Note the order of the calls is important here:
	 *  GetNumRowBlocks()
	 *  preps some important per-instance tracking cursors and the RBSize cache
	 *  which is used by the other calls to speed up the process!
	 *
	 *  (trackers are: m_CurrentSizeCell, m_Current_RBSize)
	 */
	rowblocksize_t sheet_total;
	size_t dbcells = GetNumRowBlocks(&sheet_total);
	ret += dbcells + (MAX_ROWBLOCK_SIZE * MAX_COLUMNS_PER_ROW * RB_DBCELL_CELLSIZEOFFSET) / MAX_RECORD_SIZE; // worst-case: DBCELL+CONTINUEs for full-width rows
	ret += sheet_total.rows_sofar; // 1 per ROW
	ret += sheet_total.cells_sofar; // 1 per cell

	return ret;
}

void worksheet::MakeActive()
{
	m_GlobalRecords.GetWindow1().SetActiveSheet(sheetIndex);
}

formula_t* worksheet::formula_data() {
    formula_t *fs;
    
    fs = new formula_t(m_GlobalRecords, this);
    m_FormulaStacks.push_back(fs);
    return fs;
}

cell_t* worksheet::label(uint32 row, uint32 col,
						 const std::string& strlabel, xf_t* pxformat)
{
	label_t* lbl;

	lbl = new label_t(m_GlobalRecords, row, col, strlabel, pxformat);
	AddCell(lbl);

	return lbl;
}

cell_t* worksheet::label(uint32 row, uint32 col,
						 const ustring& strlabel, xf_t* pxformat)
{
	label_t* lbl;

	lbl = new label_t(m_GlobalRecords, row, col, strlabel, pxformat);

	AddCell(lbl);

	return lbl;
}

cell_t* worksheet::number(uint32 row, uint32 col, // Deprecated
						  double numval, format_number_t fmtval,
						  xf_t* pxformat)
{
	// this routine cannot handle greater values
	XL_ASSERT(fmtval <= FMT_TEXT);

	number_t* num = new number_t(m_GlobalRecords, row, col, numval, pxformat);
	AddCell(num);

	if(fmtval == FMT_GENERAL && pxformat == NULL) {
		// OK
	} else
	if(pxformat == NULL || pxformat->GetFormat() != fmtval)   {
		// got to add it, will create a new xf_t
		//number->formatIndex(format2index(fmtval));
		num->format(fmtval);
	}
	return num;
}

cell_t* worksheet::number(uint32 row, uint32 col,
						  double numval, xf_t* pxformat)
{
	number_t* num = new number_t(m_GlobalRecords, row, col, numval, pxformat);
	AddCell(num);
	return num;
}

cell_t* worksheet::number(uint32 row, uint32 col, // 536870911 >= numval >= -536870912
						  int32 numval, xf_t* pxformat)
{
	number_t* num = new number_t(m_GlobalRecords, row, col, numval, pxformat);
	AddCell(num);
	return num;
}

cell_t* worksheet::number(uint32 row, uint32 col, // 536870911 >= numval >= -536870912
						  uint32 numval, xf_t* pxformat)
{
	number_t* num = new number_t(m_GlobalRecords, row, col, numval, pxformat);
	AddCell(num);
	return num;
}

cell_t* worksheet::boolean(uint32 row, uint32 col,
						   bool boolval, xf_t* pxformat)
{
	boolean_t* num = new boolean_t(m_GlobalRecords, row, col, boolval, pxformat);
	AddCell(num);
	return num;
}

cell_t* worksheet::error(uint32 row, uint32 col,
						 errcode_t errorcode, xf_t* pxformat)
{
	err_t* num = new err_t(m_GlobalRecords, row, col, errorcode, pxformat);
	AddCell(num);
	return num;
}

cell_t* worksheet::note(uint32 row, uint32 col,
						const std::string& remark, const std::string& author, xf_t* pxformat)
{
	note_t* note = new note_t(m_GlobalRecords, row, col, remark, author, pxformat);
	AddCell(note);
	return note;
}

cell_t* worksheet::note(uint32 row, uint32 col,
						const ustring& remark, const ustring& author, xf_t* pxformat)
{
	note_t* note = new note_t(m_GlobalRecords, row, col, remark, author, pxformat);
	AddCell(note);
	return note;
}

cell_t* worksheet::formula(uint32 row, uint32 col,
						   expression_node_t* expression_root,
						   bool auto_destruct_expression_tree,
						   xf_t* pxformat)
{
	formula_cell_t* expr = new formula_cell_t(m_GlobalRecords, row, col, 
            expression_root, auto_destruct_expression_tree, pxformat);
	AddCell(expr);
	return expr;
}

cell_t* worksheet::formula(uint32 row, uint32 col,
						   formula_t *formula, xf_t* pxformat)
{
	formula_cell_t* expr = new formula_cell_t(m_GlobalRecords, row, col, formula, pxformat);
	AddCell(expr);
	return expr;
}

cell_t* worksheet::blank(uint32 row, uint32 col, xf_t* pxformat)
{
	blank_t* blk = new blank_t(m_GlobalRecords, row, col, pxformat);
	AddCell(blk);
	return blk;
}

void worksheet::AddCell(cell_t* pcell)
{
	uint32 row, col;        // [i_a]

	row = pcell->GetRow();
	col = pcell->GetCol();

	if(row < minRow) {minRow = row; }
	if(row > maxRow) {maxRow = row; }
	if(col < minCol) {minCol = col; }
	if(col > maxCol) {maxCol = col; }

	//SortCells(); does nothing now

	bool success;
	do {
		Cell_Set_Itor_t	ret;

		if(cellHint && pcell->row >= cellHint->row) {
			ret		= m_Cells.insert(cellIterHint, pcell);
			success	= (*ret == pcell);
		} else {
			std::pair<Cell_Set_Itor_t, bool> pr;

			pr = m_Cells.insert(pcell);
			ret		= pr.first;
			success	= pr.second;
		}

		if(!success) {
			cell_t* existing_cell;

			// means we got a duplicate - the user is overwriting an existing cell
			existing_cell = *(ret);
			m_Cells.erase(existing_cell);   // Bugs item #2840227:  prevent crash: first erase, then delete
			delete existing_cell;

			cellHint = NULL;
		} else {
			cellIterHint = ret;
			cellHint = pcell;
		}
	} while(!success);

    pcell->ws = this;

	/*m_CellsSorted = false; */
	m_SizesCalculated = false;
	m_RBSizes.clear();
}

cell_t* worksheet::FindCell(uint32 row, uint32 col) const
{
	Cell_Set_CItor_t existing_cell;

	// need a cell to find a cell! So create simplest possible one
	blank_t cell(m_GlobalRecords, row, col);
	existing_cell = m_Cells.find(&cell);

	// The find operation returns the end() iterator
	// if the cell wasn't found
	if(existing_cell != m_Cells.end()) {
		return *existing_cell;
	} else {
		return NULL;
	}
}

cell_t* worksheet::FindCellOrMakeBlank(uint32 row, uint32 col)
{
	cell_t*	cell = worksheet::FindCell(row, col);

	return cell ? cell : blank(row, col);
}

/*
 ***********************************
 *  Returns FALSE when the last row block was processed (and the cursors have been reset
 *  to point at the start/top of the sheet again); returns TRUE when another row block
 *  awaits after this one.
 ***********************************
 */
bool worksheet::GetRowBlockSizes(rowblocksize_t& rbsize)
{
	//SortCells();

	size_t row_counter = 0;
	size_t cell_counter = 0;
	size_t payload;

	//Cell_Set_Itor_t beginning_cell = m_CurrentSizeCell;

	// Initialize the size values (since they work as accumulators)
	rbsize.rowandcell_size = 0;
	rbsize.dbcell_size = 0;

	if(!m_SizesCalculated) {
		// Check if there are no cells
		if(!m_Cells.empty()) {
			uint32 row_num;

			// The first cell is inside a row that has to be counted
			row_counter = 1;

			row_num = (*m_CurrentSizeCell)->GetRow();
			if (rbsize.first_row > row_num) {
				rbsize.first_row = row_num;
			}
			if (rbsize.last_row < row_num) {
				rbsize.last_row = row_num;
			}

			// Since the list of cells is sorted by rows, continuously equal cells (compared by row)
			// conform one row... if the next one is different, increment the row counter
			for (; m_CurrentSizeCell != m_Cells.end(); m_CurrentSizeCell++) {
				if ((*m_CurrentSizeCell)->GetRow() != row_num) {
					// exit the loop if we have run through MAX rows:
					if (row_counter >= MAX_ROWBLOCK_SIZE) {
						break;
					}

					row_counter++;
					row_num = (*m_CurrentSizeCell)->GetRow();

					if (rbsize.first_row > row_num) {
						rbsize.first_row = row_num;
					}
					if (rbsize.last_row < row_num) {
						rbsize.last_row = row_num;
					}
				}

				cell_counter++;
				// Get the size of the cell as well:
				rbsize.rowandcell_size += (*m_CurrentSizeCell)->GetSize();

				uint32 col_num = (*m_CurrentSizeCell)->GetCol();
				if (rbsize.first_col > col_num) {
					rbsize.first_col = col_num;
				}
				if (rbsize.last_col < col_num) {
					rbsize.last_col = col_num;
				}
			}

			rbsize.rows_sofar += row_counter;
			// Get the size of the rows
			rbsize.rowandcell_size += ROW_RECORD_SIZE*row_counter;

			rbsize.cells_sofar += cell_counter;

			// Now get the size of the DBCELL
			payload = RB_DBCELL_CELLSIZEOFFSET*cell_counter;

			rbsize.dbcell_size += RB_DBCELL_MINSIZE;
			rbsize.dbcell_size += payload;

			// Check the size of the data in the DBCELL record (without the header)
			// to take in account the overhead of the CONTINUE record (4bytes/CONTrec)
			if(payload > MAX_RECORD_SIZE) {
				size_t cont_overhead = ((payload + MAX_RECORD_SIZE - 1) / MAX_RECORD_SIZE); // continue payloads; account for trailing partial payloads: round up!

				rbsize.dbcell_size += (cont_overhead-1)*4;
			}

			//rbsize->rowandcell_size = *rowandcell_size;
			//rbsize->dbcell_size = *dbcell_size;
			//rbsize.rows_sofar = row_counter;
			//rbsize.cells_sofar = cell_counter;
			m_RBSizes.push_back(rbsize);

			// If it was the last block, reset the current-label pointer
			if(m_CurrentSizeCell == m_Cells.end()) {
				m_CurrentSizeCell = m_Cells.begin();
				m_Current_RBSize = m_RBSizes.begin();
				m_SizesCalculated = true;

				return false;
			}
		}

		// If there are no cells in the sheet, return sizes = 0.
		if(m_Cells.empty()) {
			return false;
		} else {
			return true;
		}
	} else {
		rbsize = *m_Current_RBSize;
		m_Current_RBSize++;

		// Reset the current RBSize
		if(m_Current_RBSize == m_RBSizes.end()) {
			m_Current_RBSize = m_RBSizes.begin();
			return false;
		}
	}
	return true;
}

void worksheet::GetFirstLastRowsAndColumns(uint32* first_row, uint32* last_row, uint32* first_col, uint32* last_col) /* [i_a] */
{
	// First check that the m_Cells list is not empty, so we won't dereference
	// an empty iterator.
	if(!m_Cells.empty()) {
		//SortCells();

		/*
		 *  speedup for when we are not interested to hear about the columns:
		 *  since this vector is sorted by row first, we can simply grab the
		 *  last unit to get the highest rownum:
		 */
		if (!first_col && !last_col) {
			cell_t *f = *m_Cells.begin();    // .front()
			cell_t *l = *(--m_Cells.end()); // .back()

			XL_ASSERT(f);
			XL_ASSERT(l);

			if (first_row) {
				*first_row = f->GetRow();
			}
			if (last_row) {
				*last_row = l->GetRow();
			}
		} else {
			rowblocksize_t rbsize;
			GetNumRowBlocks(&rbsize);

			if (first_row) {
				*first_row = rbsize.first_row;
			}
			if (last_row) {
				*last_row = rbsize.last_row;
			}
			if (first_col) {
				*first_col = rbsize.first_col;
			}
			if (last_col) {
				*last_col = rbsize.last_col;
			}
		}
	} else {
		// If there is no cells in the list the first/last rows
		// are defaulted to zero.
		if (first_row) {
			*first_row = 0;
		}
		if (last_row) {
			*last_row = 0;
		}
		if (first_col) {
			*first_col = 0;
		}
		if (last_col) {
			*last_col = 0;
		}
	}
}

size_t worksheet::GetNumRowBlocks(rowblocksize_t* rbsize_ref)
{
	size_t numrb;

	if (!m_Cells.empty()) {
		m_CurrentSizeCell = m_Cells.begin();
		m_Current_RBSize = m_RBSizes.begin();

		rowblocksize_t rbsize;
		if (rbsize_ref)	{
			rbsize_ref->reset();
		} else {
			rbsize_ref = &rbsize;
		}

		bool cont = false;
		do {
			rowblocksize_t rb1;
			cont = GetRowBlockSizes(rb1);

			rbsize_ref->cells_sofar += rb1.cells_sofar;
			rbsize_ref->dbcell_size += rb1.dbcell_size;
			if (rbsize_ref->first_col > rb1.first_col) {
				rbsize_ref->first_col = rb1.first_col;
			}
			if (rbsize_ref->first_row > rb1.first_row) {
				rbsize_ref->first_row = rb1.first_row;
			}
			if (rbsize_ref->last_col < rb1.last_col) {
				rbsize_ref->last_col = rb1.last_col;
			}
			if (rbsize_ref->last_row < rb1.last_row) {
				rbsize_ref->last_row = rb1.last_row;
			}
			rbsize_ref->rowandcell_size += rb1.rowandcell_size;
			rbsize_ref->rows_sofar += rb1.rows_sofar;
		} while(cont);

		numrb = m_RBSizes.size();
	} else {
		// If the m_Cell list is empty, there are no rowblocks in the sheet.
		numrb = 0;
	}

	return numrb;
}

void worksheet::merge(uint32 first_row, uint32 first_col,
					  uint32 last_row, uint32 last_col)
{
	range_t* newrange = new range_t;

	newrange->first_row = first_row;
	newrange->last_row  = last_row;
	newrange->first_col = first_col;
	newrange->last_col  = last_col;

	m_MergedRanges.push_back(newrange);
}

void worksheet::colwidth(uint32 col, uint16 width, xf_t* pxformat)
{
	colinfo_t* newci = new colinfo_t;
	Colinfo_Set_Itor_t existing_ci;

	if(pxformat) {
		pxformat->MarkUsed();
	}

	// TODO: not sure why these are all public...
	newci->colfirst	= col;
	newci->collast	= col;
	newci->flags	= 0x00;
	newci->width	= width;                // sets column widths to 1/256 x width of "0"
	newci->xformat	= pxformat;

	// already have one?
	existing_ci = m_Colinfos.find(newci);

	if(existing_ci != m_Colinfos.end())	{
		if((*existing_ci)->xformat) {
			(*existing_ci)->xformat->UnMarkUsed();
		}

		delete (*existing_ci);

		m_Colinfos.erase(existing_ci);
		m_Colinfos.insert(newci);
	} else {
		m_Colinfos.insert(newci);
	}
}

void worksheet::rowheight(uint32 row, uint16 heightInTwips, xf_t* pxformat)
{
	rowheight_t* newrh = new rowheight_t(row, heightInTwips, pxformat);
	RowHeight_Vect_Itor_t existing_rh;

	// should be in rowheight_t but too much trouble
	if(pxformat) {
		pxformat->MarkUsed();
	}

	//m_RowHeights.insert(newrh);
	existing_rh = m_RowHeights.find(newrh);

	if(existing_rh != m_RowHeights.end()) {
		if((*existing_rh)->GetXF()) {
			(*existing_rh)->GetXF()->UnMarkUsed();
		}

		delete (*existing_rh);
		m_RowHeights.erase(existing_rh);
		m_RowHeights.insert(newrh);
	} else {
		m_RowHeights.insert(newrh);
	}
}

range* worksheet::rangegroup(uint32 row1, uint32 col1,
							 uint32 row2, uint32 col2)
{
	range* newrange = new range(row1, col1, row2, col2, this);
	m_Ranges.push_back(newrange);

	return newrange;
}

void worksheet::validate(const range_t *crange, uint32 options,
        const formula_t *cond1, const formula_t *cond2,
        const std::string& promptTitle, const std::string& promptText,
        const std::string& errorTitle, const std::string& errorText) {
    struct DataValidation *dv = new DataValidation;
    dv->first_row = crange->first_row;
    dv->last_row = crange->last_row;
    dv->first_col = crange->first_col;
    dv->last_col = crange->last_col;
    dv->cond1 = cond1;
    dv->cond2 = cond2;
    dv->options = options;
    m_GlobalRecords.char2str16(promptTitle, dv->prompt_title);
    m_GlobalRecords.char2str16(promptText, dv->prompt_text);
    m_GlobalRecords.char2str16(errorTitle, dv->error_title);
    m_GlobalRecords.char2str16(errorText, dv->error_text);
    m_DataValidations.push_back(dv);
}

void worksheet::validate(const range_t *crange, uint32 options,
        const formula_t *cond1, const formula_t *cond2,
        const ustring& promptTitle, const ustring& promptText,
        const ustring& errorTitle, const ustring& errorText) {
    struct DataValidation *dv = new DataValidation;
    dv->first_row = crange->first_row;
    dv->last_row = crange->last_row;
    dv->first_col = crange->first_col;
    dv->last_col = crange->last_col;
    dv->cond1 = cond1;
    dv->cond2 = cond2;
    dv->options = options;
    m_GlobalRecords.wide2str16(promptTitle, dv->prompt_title);
    m_GlobalRecords.wide2str16(promptText, dv->prompt_text);
    m_GlobalRecords.wide2str16(errorTitle, dv->error_title);
    m_GlobalRecords.wide2str16(errorText, dv->error_text);
    m_DataValidations.push_back(dv);
}

// define a cell (label, number, etc) - apply proper url (http://blah.blah), possible text mark too (minus the '#')
void worksheet::hyperLink(const cell_t *cell, const std::string& url, const std::string& mark)
{
	struct HyperLink *link = new HyperLink;

	link->row = (uint16)cell->GetRow();
	link->col = (uint16)cell->GetCol();
	m_GlobalRecords.char2str16(url, link->url);
	m_GlobalRecords.char2str16(mark, link->mark);

	m_HyperLinks.push_back(link);
}
void worksheet::hyperLink(const cell_t *cell, const ustring& url, const ustring& mark)
{
	struct HyperLink *link = new HyperLink;

	link->row = (uint16)cell->GetRow();
	link->col = (uint16)cell->GetCol();
	m_GlobalRecords.wide2str16(url, link->url);
	m_GlobalRecords.wide2str16(mark, link->mark);

	m_HyperLinks.push_back(link);
}

CUnit* worksheet::MakeDataValidationHeader(CDataStorage& datastore, uint32 dval_count)
{
	CRecord *dvRecord = datastore.MakeCRecord();
    dvRecord->Inflate(4+18);
    dvRecord->SetRecordType(RECTYPE_DVAL);
    dvRecord->AddValue16(0); /* options */
    dvRecord->AddValue32(0); /* hpos */
    dvRecord->AddValue32(0); /* vpos */
    dvRecord->AddValue32(0xFFFFFFFF); /* arrow */
    dvRecord->AddValue32(dval_count); /* count */
	dvRecord->SetRecordLength(18);
    return (CUnit *)dvRecord;
}

CUnit* worksheet::MakeDataValidationEntry(CDataStorage& datastore, DataValidation* dval)
{
	CRecord *dvRecord = datastore.MakeCRecord();
    dvRecord->Inflate(32);
    dvRecord->SetRecordType(RECTYPE_DV);
    dvRecord->AddValue32(dval->options);
    dvRecord->AddUnicodeString(dval->prompt_title, CUnit::LEN2_FLAGS_UNICODE);
    dvRecord->AddUnicodeString(dval->error_title, CUnit::LEN2_FLAGS_UNICODE);
    dvRecord->AddUnicodeString(dval->prompt_text, CUnit::LEN2_FLAGS_UNICODE);
    dvRecord->AddUnicodeString(dval->error_text, CUnit::LEN2_FLAGS_UNICODE);
    if (dval->cond1 == NULL) {
        dvRecord->AddValue16(3);
        dvRecord->AddValue16(0);
        dvRecord->AddValue8(OP_INT);
        dvRecord->AddValue16(0);
    } else {
        dvRecord->AddValue16((uint16)dval->cond1->GetSize());
        dvRecord->AddValue16(0);
        dval->cond1->DumpData(*dvRecord);
    }
    if (dval->cond2 == NULL) {
        dvRecord->AddValue16(3);
        dvRecord->AddValue16(0);
        dvRecord->AddValue8(OP_INT);
        dvRecord->AddValue16(0);
    } else {
        dvRecord->AddValue16((uint16)dval->cond2->GetSize());
        dvRecord->AddValue16(0);
        dval->cond2->DumpData(*dvRecord);
    }
    dvRecord->AddValue16(1);
    dvRecord->AddValue16(static_cast<uint16>(dval->first_row));
    dvRecord->AddValue16(static_cast<uint16>(dval->last_row));
    dvRecord->AddValue16(static_cast<uint16>(dval->first_col));
    dvRecord->AddValue16(static_cast<uint16>(dval->last_col));
	dvRecord->SetRecordLength(dvRecord->GetDataSize() - 4);
    return (CUnit *)dvRecord;
}

static unsigned char StdLinkGUID[16] = { 0xd0, 0xc9, 0xea, 0x79, 0xf9, 0xba, 0xce, 0x11, 0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b };
static unsigned char URLMonikerGUID[16] = { 0xe0, 0xc9, 0xea, 0x79, 0xf9, 0xba, 0xce, 0x11, 0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b };

CUnit* worksheet::MakeHyperLink(CDataStorage& datastore, HyperLink* link)
{
	bool hasMark = link->mark.length() ? true : false;
	u16string::const_iterator cBegin, cEnd;

	uint32 urlLen = (uint32)(link->url.length()+1) * 2;
	uint32 markLen = (uint32)(link->mark.length()+1) * 2;

	size_t newsize = 2 + 2 + 2 + 2 + 16 + 4 + 4 + 16 + 4 + urlLen;
	if(hasMark) {
		newsize += 4 + markLen;
	}

	CRecord *hyperRecord = datastore.MakeCRecord();
	hyperRecord->Inflate(newsize);

	hyperRecord->SetRecordType(RECTYPE_HLINK);

	hyperRecord->AddValue16(link->row);
	hyperRecord->AddValue16(link->row);
	hyperRecord->AddValue16(link->col);
	hyperRecord->AddValue16(link->col);
	hyperRecord->AddDataArray(StdLinkGUID, 16);
	hyperRecord->AddValue32(2); // Magic number
	hyperRecord->AddValue32(0x03 | (hasMark ? 0x08 : 0x00));

	hyperRecord->AddDataArray(URLMonikerGUID, 16);

	// URL byte length, then URL followed by NULL "character"
	hyperRecord->AddValue32(urlLen);

	cBegin	= link->url.begin();
	cEnd	= link->url.end();

	while(cBegin != cEnd) {
		hyperRecord->AddValue16(*cBegin++);
	}
	hyperRecord->AddValue16(0);

	if(hasMark) {
		hyperRecord->AddValue32(markLen/2); // character count
		cBegin	= link->mark.begin();
		cEnd	= link->mark.end();

		while(cBegin != cEnd) {
			hyperRecord->AddValue16(*cBegin++);
		}
		hyperRecord->AddValue16(0);
	}
	hyperRecord->SetRecordLength(hyperRecord->GetDataSize() - 4);

	return (CUnit *)hyperRecord;
}
