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
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * File description:
 *
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef RECTYPES_H
#define RECTYPES_H

/*
 *  sed -e 's/\(^\([A-Z0-9]*\)\:.*$\)/\/\* \1 \*\/\n#define RECTYPE_\2 \\/' biff_records_alpha.txt  | sed -e 's/^ \([A-F0-9]*\)\h$/                                              0x\1/' >recordtypes.h
 */

//Í#include "xlsys.h"
//#include "systype.h"

// SIZES
#define RECORD_HEADER_SIZE     4
#define MAX_RECORD_SIZE		8224
// see OpenOffice Doc, CONTINUE record 5.21. This is the payload size not including the header
#define BOF_SIZE              20
#define EOF_SIZE               4
#define WINDOW2_SIZE          22
#define DIMENSION_SIZE        18
#define DEF_ROW_HEIGHT_SIZE    8
#define DEF_COL_WIDTH_SIZE     6
#define BOF_RECORD_SIZE		  12  // used when computing blocks during output stage
#define COL_INFO_SIZE		  16

// End SIZES

#define RECTYPE_NULL							\
	0x00

/* DATEMODE: 1900 / 1904 Date System (PC=0, MAC=1, still) */
#define RECTYPE_DATEMODE						\
	0x22

/* ADDIN: Workbook Is an Add-in Macro */
#define RECTYPE_ADDIN							\
	0x87

/* ADDMENU: Menu Addition */
#define RECTYPE_ADDMENU							\
	0xC2

/* ARRAY: Array-Entered Formula */
#define RECTYPE_ARRAY							\
	0x221

/* AUTOFILTER: AutoFilter Data */
#define RECTYPE_AUTOFILTER						\
	0x9E

/* AUTOFILTERINFO: Drop-Down Arrow Count */
#define RECTYPE_AUTOFILTERINFO					\
	0x9D

/* BACKUP: Save Backup Version of the File */
#define RECTYPE_BACKUP							\
	0x40

/* BLANK: Cell Value, Blank Cell */
#define RECTYPE_BLANK							\
	0x201

/* BOF: Beginning of File */
#define RECTYPE_BOF								\
	0x809

/* BOOKBOOL: Workbook Option Flag */
#define RECTYPE_BOOKBOOL						\
	0xDA

/* BOOLERR: Cell Value, Boolean or Error */
#define RECTYPE_BOOLERR							\
	0x205

/* BOTTOMMARGIN: Bottom Margin Measurement */
#define RECTYPE_BOTTOMMARGIN					\
	0x29

/* BOUNDSHEET: Sheet Information */
#define RECTYPE_BOUNDSHEET						\
	0x85

/* CALCCOUNT: Iteration Count */
#define RECTYPE_CALCCOUNT						\
	0x0C

/* CALCMODE: Calculation Mode */
#define RECTYPE_CALCMODE						\
	0x0D

/* CF
 *  : Conditional Formatting Conditions */
#define RECTYPE_CF								\
	0x1B1

/* CONDFMT: Conditional Formatting Range Information */
#define RECTYPE_CONDFMT							\
	0x1B0

/* CODENAME: VBE Object Name */
#define RECTYPE_CODENAME						\
	0x42

/* CODEPAGE: Default Code Page */
#define RECTYPE_CODEPAGE						\
	0x42

/* COLINFO: Column Formatting Information */
#define RECTYPE_COLINFO							\
	0x7D

/* CONTINUE: Continues Long Records */
#define RECTYPE_CONTINUE						\
	0x3C

/* COORDLIST: Polygon Object Vertex Coordinates */
#define RECTYPE_COORDLIST						\
	0xA9

/* COUNTRY: Default Country and WIN.INI Country */
#define RECTYPE_COUNTRY							\
	0x8C

/* CRN: Nonresident Operands */
#define RECTYPE_CRN								\
	0x5A

/* DBCELL: Stream Offsets */
#define RECTYPE_DBCELL							\
	0xD7

/* DCON: Data Consolidation Information */
#define RECTYPE_DCON							\
	0x50

/* DCONBIN: Data Consolidation Information */
#define RECTYPE_DCONBIN							\
	0x1B5

/* DCONNAME: Data Consolidation Named References */
#define RECTYPE_DCONNAME						\
	0x52

/* DCONREF: Data Consolidation References */
#define RECTYPE_DCONREF							\
	0x51

/* DEFAULTROWHEIGHT: Default Row Height */
#define RECTYPE_DEFAULTROWHEIGHT				\
	0x225

/* DEFCOLWIDTH: Default Width for Columns */
#define RECTYPE_DEFCOLWIDTH						\
	0x55

/* DELMENU: Menu Deletion */
#define RECTYPE_DELMENU							\
	0xC3

/* DELTA: Iteration Increment */
#define RECTYPE_DELTA							\
	0x10

/* DIMENSIONS: Cell Table Size */
#define RECTYPE_DIMENSIONS						\
	0x200

/* DOCROUTE: Routing Slip Information */
#define RECTYPE_DOCROUTE						\
	0xB8

/* DSF: Double Stream File */
#define RECTYPE_DSF								\
	0x161

/* DV: Data Validation Criteria */
#define RECTYPE_DV								\
	0x1BE

/* DVAL: Data Validation Information */
#define RECTYPE_DVAL							\
	0x1B2

/* EDG: Edition Globals */
#define RECTYPE_EDG								\
	0x88

/* EOF: End of File */
#define RECTYPE_EOF								\
	0x0A

/* EXTERNCOUNT: Number of External References */
#define RECTYPE_EXTERNCOUNT						\
	0x16

/* EXTERNNAME: Externally Referenced Name */
#define RECTYPE_EXTERNNAME						\
	0x223 // Excel2007 spits out 0x23 instead!

/* EXTERNSHEET: External Reference */
#define RECTYPE_EXTERNSHEET						\
	0x17

/* EXTSST: Extended Shared String Table */
#define RECTYPE_EXTSST							\
	0xFF

/* FILEPASS: File Is Password-Protected */
#define RECTYPE_FILEPASS						\
	0x2F

/* FILESHARING: File-Sharing Information */
#define RECTYPE_FILESHARING						\
	0x5B

/* FILESHARING2: File-Sharing Information for Shared Lists */
#define RECTYPE_FILESHARING2					\
	0x1A5

/* FILTERMODE: Sheet Contains Filtered List */
#define RECTYPE_FILTERMODE						\
	0x9B

/* FNGROUPCOUNT: Built-in Function Group Count */
#define RECTYPE_FNGROUPCOUNT					\
	0x9C

/* FNGROUPNAME: Function Group Name */
#define RECTYPE_FNGROUPNAME						\
	0x9A

/* FONT: Font Description */
// NOTE: Changed temporarily to 31h (the manual says is 231h) -- [i_a] inspection of the generated XLS files shows that 0x31 displays the fonts in Excel2003/2007, while 0x231 does /not/.
#define RECTYPE_FONT							\
	0x31

/* FOOTER: Print Footer on Each Page */
#define RECTYPE_FOOTER							\
	0x15

/* FORMAT: Number Format */
#define RECTYPE_FORMAT							\
	0x41E

/* FORMULA: Cell Formula */
#define RECTYPE_FORMULA							\
	0x06 // also known under 0x406 but MS Spec says it's 0x06!

/* GCW: Global Column-Width Flags */
#define RECTYPE_GCW								\
	0xAB

/* GRIDSET: State Change of Gridlines Option */
#define RECTYPE_GRIDSET							\
	0x82

/* GUTS: Size of Row and Column Gutters */
#define RECTYPE_GUTS							\
	0x80

/* HCENTER: Center Between Horizontal Margins */
#define RECTYPE_HCENTER							\
	0x83

/* HEADER: Print Header on Each Page */
#define RECTYPE_HEADER							\
	0x14

/* HIDEOBJ: Object Display Options */
#define RECTYPE_HIDEOBJ							\
	0x8D

/* HLINK: Hyperlink */
#define RECTYPE_HLINK							\
	0x1B8

/* HORIZONTALPAGEBREAKS: Explicit Row Page Breaks */
#define RECTYPE_HORIZONTALPAGEBREAKS			\
	0x1B

/* IMDATA: Image Data */
#define RECTYPE_IMDATA							\
	0x7F

/* INDEX: Index Record */
#define RECTYPE_INDEX							\
	0x20B

/* INTERFACEEND: End of User Interface Records */
#define RECTYPE_INTERFACEEND					\
	0xE2

/* INTERFACEHDR: Beginning of User Interface Records */
#define RECTYPE_INTERFACEHDR					\
	0xE1

/* ITERATION: Iteration Mode */
#define RECTYPE_ITERATION						\
	0x11

/* LABEL: Cell Value, String Constant */
#define RECTYPE_LABEL							\
	0x204

/* LABELSST: Cell Value, String Constant/SST */
#define RECTYPE_LABELSST						\
	0xFD

/* LEFTMARGIN: Left Margin Measurement */
#define RECTYPE_LEFTMARGIN						\
	0x26

/* LHNGRAPH: Named Graph Information */
#define RECTYPE_LHNGRAPH						\
	0x95

/* LHRECORD: .WK? File Conversion Information */
#define RECTYPE_LHRECORD						\
	0x94

/* LPR: Sheet Was Printed Using LINE.PRINT( */
#define RECTYPE_LPR								\
	0x98

/* MMS: ADDMENU/DELMENU Record Group Count */
#define RECTYPE_MMS								\
	0xC1

/* MSODRAWING: Microsoft Office Drawing */
#define RECTYPE_MSODRAWING						\
	0xEC

/* MSODRAWINGGROUP: Microsoft Office Drawing Group */
#define RECTYPE_MSODRAWINGGROUP					\
	0xEB

/* MSODRAWINGSELECTION: Microsoft Office Drawing Selection */
#define RECTYPE_MSODRAWINGSELECTION				\
	0xED

/* MULBLANK: Multiple Blank Cells */
#define RECTYPE_MULBLANK						\
	0xBE

/* MULRK: Multiple RK Cells */
#define RECTYPE_MULRK							\
	0xBD

/* NAME: Defined Name */
#define RECTYPE_NAME							\
	0x218 // Excel 2007 spits out 0x18 instead!

/* NOTE: Comment Associated with a Cell */
#define RECTYPE_NOTE							\
	0x1C

/* NUMBER: Cell Value, Floating-Point Number */
#define RECTYPE_NUMBER							\
	0x203

/* OBJ: Describes a Graphic Object */
#define RECTYPE_OBJ								\
	0x5D

/* OBJPROTECT: Objects Are Protected */
#define RECTYPE_OBJPROTECT						\
	0x63

/* OBPROJ: Visual Basic Project */
#define RECTYPE_OBPROJ							\
	0xD3

/* OLESIZE: Size of OLE Object */
#define RECTYPE_OLESIZE							\
	0xDE

/* PALETTE: Color Palette Definition */
#define RECTYPE_PALETTE							\
	0x92

/* PANE: Number of Panes and Their Position */
#define RECTYPE_PANE							\
	0x41

/* PARAMQRY: Query Parameters */
#define RECTYPE_PARAMQRY						\
	0xDC

/* PASSWORD: Protection Password */
#define RECTYPE_PASSWORD						\
	0x13

/* PLS: Environment-Specific Print Record */
#define RECTYPE_PLS								\
	0x4D

/* PRECISION: Precision */
#define RECTYPE_PRECISION						\
	0x0E

/* PRINTGRIDLINES: Print Gridlines Flag */
#define RECTYPE_PRINTGRIDLINES					\
	0x2B

/* PRINTHEADERS: Print Row/Column Labels */
#define RECTYPE_PRINTHEADERS					\
	0x2A

/* PROTECT: Protection Flag */
#define RECTYPE_PROTECT							\
	0x12

/* PROT4REV: Shared Workbook Protection Flag */
#define RECTYPE_PROT4REV						\
	0x1AF

/* QSI: External Data Range */
#define RECTYPE_QSI								\
	0x1AD

/* RECIPNAME: Recipient Name */
#define RECTYPE_RECIPNAME						\
	0xB9

/* REFMODE: Reference Mode */
#define RECTYPE_REFMODE							\
	0x0F

/* REFRESHALL: Refresh Flag */
#define RECTYPE_REFRESHALL						\
	0x1B7

/* RIGHTMARGIN: Right Margin Measurement */
#define RECTYPE_RIGHTMARGIN						\
	0x27

/* RK: Cell Value, RK Number */
#define RECTYPE_RK								\
	0x27E

/* ROW: Describes a Row */
#define RECTYPE_ROW								\
	0x208

/* RSTRING: Cell with Character Formatting */
#define RECTYPE_RSTRING							\
	0xD6

/* SAVERECALC: Recalculate Before Save */
#define RECTYPE_SAVERECALC						\
	0x5F

/* SCENARIO: Scenario Data */
#define RECTYPE_SCENARIO						\
	0xAF

/* SCENMAN: Scenario Output Data */
#define RECTYPE_SCENMAN							\
	0xAE

/* SCENPROTECT: Scenario Protection */
#define RECTYPE_SCENPROTECT						\
	0xDD

/* SCL: Window Zoom Magnification */
#define RECTYPE_SCL								\
	0xA0

/* SELECTION: Current Selection */
#define RECTYPE_SELECTION						\
	0x1D

/* SETUP: Page Setup */
#define RECTYPE_SETUP							\
	0xA1

/* SHRFMLA: Shared Formula */
#define RECTYPE_SHRFMLA							\
	0xBC

/* SORT: Sorting Options */
#define RECTYPE_SORT							\
	0x90

/* SOUND: Sound Note */
#define RECTYPE_SOUND							\
	0x96

/* SST: Shared String Table */
#define RECTYPE_SST								\
	0xFC

/* STANDARDWIDTH: Standard Column Width */
#define RECTYPE_STANDARDWIDTH					\
	0x99

/* STRING: String Value of a Formula */
#define RECTYPE_STRING							\
	0x207

/* STYLE: Style Information */
#define RECTYPE_STYLE							\
	0x293

/* SUB: Subscriber */
#define RECTYPE_SUB								\
	0x91

/* SUPBOOK: Supporting Workbook */
#define RECTYPE_SUPBOOK							\
	0x1AE

/* SXDB: PivotTable Cache Data */
#define RECTYPE_SXDB							\
	0xC6

/* SXDBEX: PivotTable Cache Data */
#define RECTYPE_SXDBEX							\
	0x122

/* SXDI: Data Item */
#define RECTYPE_SXDI							\
	0xC5

/* SXEX: PivotTable View Extended Information */
#define RECTYPE_SXEX							\
	0xF1

/* SXEXT: External Source Information */
#define RECTYPE_SXEXT							\
	0xDC

/* SXFDBTYPE: SQL Datatype Identifier */
#define RECTYPE_SXFDBTYPE						\
	0x1BB

/* SXFILT: PivotTable Rule Filter */
#define RECTYPE_SXFILT							\
	0xF2

/* SXFORMAT: PivotTable Format Record */
#define RECTYPE_SXFORMAT						\
	0xFB

/* SXFORMULA: PivotTable Formula Record */
#define RECTYPE_SXFORMULA						\
	0x103

/* SXFMLA: PivotTable Parsed Expression */
#define RECTYPE_SXFMLA							\
	0xF9

/* SXIDSTM: Stream ID */
#define RECTYPE_SXIDSTM							\
	0xD5

/* SXIVD: Row/Column Field IDs */
#define RECTYPE_SXIVD							\
	0xB4

/* SXLI: Line Item Array */
#define RECTYPE_SXLI							\
	0xB5

/* SXNAME: PivotTable Name  */
#define RECTYPE_SXNAME							\
	0xF6

/* SXPAIR: PivotTable Name Pair */
#define RECTYPE_SXPAIR							\
	0xF8

/* SXPI: Page Item */
#define RECTYPE_SXPI							\
	0xB6

/* SXRULE: PivotTable Rule Data */
#define RECTYPE_SXRULE							\
	0xF0

/* SXSTRING: String */
#define RECTYPE_SXSTRING						\
	0xCD

/* SXSELECT: PivotTable Selection Information */
#define RECTYPE_SXSELECT						\
	0xF7

/* SXTBL: Multiple Consolidation Source Info */
#define RECTYPE_SXTBL							\
	0xD0

/* SXTBPG: Page Item Indexes */
#define RECTYPE_SXTBPG							\
	0xD2

/* SXTBRGIITM: Page Item Name Count */
#define RECTYPE_SXTBRGIITM						\
	0xD1

/* SXVD: View Fields */
#define RECTYPE_SXVD							\
	0xB1

/* SXVDEX: Extended PivotTable View Fields */
#define RECTYPE_SXVDEX							\
	0x100

/* SXVI: View Item */
#define RECTYPE_SXVI							\
	0xB2

/* SXVIEW: View Definition */
#define RECTYPE_SXVIEW							\
	0xB0

/* SXVS: View Source */
#define RECTYPE_SXVS							\
	0xE3

/* TABID: Sheet Tab Index Array */
#define RECTYPE_TABID							\
	0x13D

/* TABIDCONF: Sheet Tab ID of Conflict History */
#define RECTYPE_TABIDCONF						\
	0xEA

/* TABLE: Data Table */
#define RECTYPE_TABLE							\
	0x236

/* TEMPLATE: Workbook Is a Template */
#define RECTYPE_TEMPLATE						\
	0x60

/* TOPMARGIN: Top Margin Measurement */
#define RECTYPE_TOPMARGIN						\
	0x28

/* TXO: Text Object */
#define RECTYPE_TXO								\
	0x1B6

/* UDDESC: Description String for Chart Autoformat */
#define RECTYPE_UDDESC							\
	0xDF

/* UNCALCED: Recalculation Status */
#define RECTYPE_UNCALCED						\
	0x5E

/* USERBVIEW: Workbook Custom View Settings */
#define RECTYPE_USERBVIEW						\
	0x1A9

/* USERSVIEWBEGIN: Custom View Settings */
#define RECTYPE_USERSVIEWBEGIN					\
	0x1AA

/* USERSVIEWEND: End of Custom View Records */
#define RECTYPE_USERSVIEWEND					\
	0x1AB

/* USESELFS: Natural Language Formulas Flag */
#define RECTYPE_USESELFS						\
	0x160

/* VCENTER: Center Between Vertical Margins */
#define RECTYPE_VCENTER							\
	0x84

/* VERTICALPAGEBREAKS: Explicit Column Page Breaks */
#define RECTYPE_VERTICALPAGEBREAKS				\
	0x1A

/* WINDOW1: Window Information */
#define RECTYPE_WINDOW1							\
	0x3D

/* WINDOW2: Sheet Window Information */
#define RECTYPE_WINDOW2							\
	0x23E

/* WINDOWPROTECT: Windows Are Protected */
#define RECTYPE_WINDOWPROTECT					\
	0x19

/* WRITEACCESS: Write Access User Name */
#define RECTYPE_WRITEACCESS						\
	0x5C

/* WRITEPROT: Workbook Is Write-Protected */
#define RECTYPE_WRITEPROT						\
	0x86

/* WSBOOL: Additional Workspace Information */
#define RECTYPE_WSBOOL							\
	0x81

/* XCT: CRN Record Count */
#define RECTYPE_XCT								\
	0x59

/* XF: Extended Format */
#define RECTYPE_XF								\
	0xE0

/* XL5MODIFY: Flag for DSF */
#define RECTYPE_XL5MODIFY						\
	0x162

/* TOOLBARHDR: Not documented */
#define RECTYPE_TOOLBARHDR						\
	0xbf

/* TOOLBAREND: Not documented */
#define RECTYPE_TOOLBAREND						\
	0xc0

/* TOOLBAREND: Not documented */
#define RECTYPE_MERGEDCELLS						\
	0xe5

#endif
//RECTYPES_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Log: rectypes.h,v $
 * Revision 1.6  2009/01/23 16:09:55  dhoerl
 * General cleanup: headers and includes. Fixed issues building mainC and mainCPP
 *
 * Revision 1.5  2009/01/08 02:52:47  dhoerl
 * December Rework
 *
 * Revision 1.4  2008/12/20 15:49:05  dhoerl
 * 1.2.5 fixes
 *
 * Revision 1.3  2008/12/06 01:42:15  dhoerl
 * John Peterson changes along with lots of tweaks. Many bugs that causes Excel crashes fixed.
 *
 * Revision 1.2  2008/10/25 18:39:54  dhoerl
 * 2008
 *
 * Revision 1.1.1.1  2004/08/27 16:31:51  darioglz
 * Initial Import.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
