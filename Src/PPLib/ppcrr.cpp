// PPCRR.CPP
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Модуль изоляции обращений к CrystalReports
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
#include <crpe2.h>

int  CopyDataStruct(const char *pSrc, const char *pDest, const char *pFileName); // Prototype(pputil.cpp)
bool FindExeByExt2(const char * pExt, SString & rResult, const char * pAddedSearchString); // Prototype(ppreport.cpp)
/*
PEClosePrintJob
PECloseSubreport
PEEnableEvent
PEExportTo
-PEGetErrorCode
-PEGetErrorText
PEGetExportOptions
PEGetGroupOptions
PEGetHandleString
PEGetNSubreportsInSection
PEGetNthSubreportInSection
PEGetReportOptions
PEGetSectionFormat
PEGetSelectedPrinter
PEGetSubreportInfo
PEGetVersion
PEOpenPrintJob
PEOpenSubreport
PEOutputToPrinter
PEOutputToWindow
PESelectPrinter
PESetEventCallback
PESetGroupOptions
PESetNthTableLocation
PESetReportOptions
PESetSectionFormat
PEStartPrintJob
*/ 

SVerT QueryCrr32Version() // @v12.4.1
{
	// @todo Необходима реализация посредством запроса к crr32_support
	SVerT result;
	const uint16 crrv = PEGetVersion(PE_GV_ENGINE);
	const uint8 major = HiByte(crrv);
	const uint8 minor = LoByte(crrv);
	if(major >= 6 && major < 100) {
		result.Set(major, minor, 0);
	}
	return result;
}

void ReportError(short printJob)
{
	HANDLE text_handle;
	short  text_length;
	short  error_code = PEGetErrorCode(printJob);
	PEGetErrorText(printJob, &text_handle, &text_length);
	char * p_error_text = static_cast<char *>(SAlloc::M(text_length));
	PEGetHandleString(text_handle, p_error_text, text_length);
	::MessageBox(0, SUcSwitch(p_error_text), _T("Print Job Failed"), MB_OK|MB_ICONEXCLAMATION); // @unicodeproblem
	SAlloc::F(p_error_text);
}

static void SetupGroupSkipping(short hJob)
{
	PESectionOptions sectopt;
	sectopt.StructSize = sizeof(PESectionOptions);
	short  sc = PE_AREA_CODE(PE_SECT_GROUP_HEADER, 0);
	PEGetSectionFormat(hJob, sc, &sectopt);
	sectopt.visible = 0;
	PESetSectionFormat(hJob, sc, &sectopt);
	sc = PE_AREA_CODE(PE_SECT_GROUP_FOOTER, 0);
	PEGetSectionFormat(hJob, sc, &sectopt);
	sectopt.visible = 0;
	PESetSectionFormat(hJob, sc, &sectopt);
	PEGroupOptions grpopt;
	grpopt.StructSize = sizeof(PEGroupOptions);
	PEGetGroupOptions(hJob, 0, &grpopt);
	char * ch = sstrchr(grpopt.fieldName, '.');
	if(ch) {
		*ch = 0;
		strcat(grpopt.fieldName, "._ID_}");
	}
	PESetGroupOptions(hJob, 0, &grpopt);
}

static SString & GetTempFileName_(const char * pFileName, SString & rDest)
{
	GetKnownFolderPath(UED_FSKNOWNFOLDER_TEMPORARY, rDest);
	return rDest.SetLastSlash().Cat(pFileName);
}

static void GetDataFilePath(int locN, const char * pPath, int isPrint, SString & rBuf)
{
	if(pPath) {
		const  char * p_fname = (locN == 0) ? "head" : "iter";
		const  char * p_ext = "btr";
		SString path;
		SFsPath::ReplaceExt((path = pPath).SetLastSlash().Cat(p_fname), p_ext, 1);
		if(isPrint) {
			long   cnt = 0;
			MakeTempFileName(pPath, p_fname, p_ext, &cnt, rBuf);
			copyFileByName(path, rBuf);
			SFile::Remove(path);
		}
		else
			rBuf = path;
	}
	else {
		// Этот блок устаревший
		const  char * p_fname = (locN == 0) ? "rpt_body.dbf" : "rpt_var.dbf";
		GetTempFileName_(p_fname, rBuf);
	}
}

struct RptTblLoc {
	PETableLocation Hdr;
	PETableLocation Iter;
};

static int SetupSubReportLocations(short hJob, RptTblLoc & rLoc, int section)
{
	int    ok = 1;
	for(short sn = 0; sn < 3; sn++) {
		const short  sect_code = PE_SECTION_CODE(/*PE_SECT_REPORT_HEADER*/section, 0, sn);
		const short  num_sub_rep = PEGetNSubreportsInSection(hJob, sect_code);
		for(short i = 0; i < num_sub_rep; i++) {
			PESubreportInfo sub_info;
			MEMSZERO(sub_info);
			sub_info.StructSize = sizeof(sub_info);
			const DWORD  sub_id = PEGetNthSubreportInSection(hJob, sect_code, i);
			if(sub_id > 0 && PEGetSubreportInfo(hJob, sub_id, &sub_info)) {
				short hjob_sub = PEOpenSubreport(hJob, sub_info.name);
				if(hjob_sub) {
					PESetNthTableLocation(hjob_sub, 0, &rLoc.Hdr);
					PESetNthTableLocation(hjob_sub, 1, &rLoc.Iter);
				}
				PECloseSubreport(hjob_sub);
			}
		}
	}
	return ok;
}

static int SetupReportLocations(short hJob, const char * pPath, int isPrint)
{
	SString temp_buf;
	RptTblLoc loc;
	MEMSZERO(loc);
	loc.Hdr.StructSize = sizeof(PETableLocation);
	GetDataFilePath(0, pPath, isPrint, temp_buf);
	temp_buf.CopyTo(loc.Hdr.Location, sizeof(loc.Hdr.Location));
	loc.Iter.StructSize = sizeof(PETableLocation);
	GetDataFilePath(1, pPath, isPrint, temp_buf);
	temp_buf.CopyTo(loc.Iter.Location, sizeof(loc.Iter.Location));
	if(hJob) {
		PESetNthTableLocation(hJob, 0, &loc.Hdr);
		PESetNthTableLocation(hJob, 1, &loc.Iter);
	}
	{
		const int __sections[2] = { PE_SECT_REPORT_HEADER, PE_SECT_REPORT_FOOTER };
		for(uint j = 0; j < SIZEOFARRAY(__sections); j++) {
			for(short sn = 0; sn < 3; sn++) {
				const short  sect_code = PE_SECTION_CODE(/*PE_SECT_REPORT_HEADER*/__sections[j], 0, sn);
				const short  num_sub_rep = PEGetNSubreportsInSection(hJob, sect_code);
				for(short i = 0; i < num_sub_rep; i++) {
					PESubreportInfo sub_info;
					MEMSZERO(sub_info);
					sub_info.StructSize = sizeof(sub_info);
					DWORD  sub_id = PEGetNthSubreportInSection(hJob, sect_code, i);
					if(sub_id > 0 && PEGetSubreportInfo(hJob, sub_id, &sub_info)) {
						const short hjob_sub = PEOpenSubreport(hJob, sub_info.name);
						if(hjob_sub) {
							PESetNthTableLocation(hjob_sub, 0, &loc.Hdr);
							PESetNthTableLocation(hjob_sub, 1, &loc.Iter);
						}
						PECloseSubreport(hjob_sub);
					}
				}
			}
		}
		//SetupSubReportLocations(hJob, loc, PE_SECT_REPORT_HEADER);
		//SetupSubReportLocations(hJob, loc, PE_SECT_REPORT_FOOTER);
	}
	return 1;
}

int CrystalReportExportParam::TranslateToCrrExportOptions(PEExportOptions & rEo) const
{
	static_assert(sizeof(Crr_ExportOptions_PdfRtfDoc) == 24);
	static_assert(sizeof(Crr_ExportOptions_Xls) == 84);
	static_assert(offsetof(Crr_ExportOptions_Xls, FirstPage) == 0x32);
	int    ok = 0;
	SString temp_buf;
	memzero(&rEo, sizeof(rEo));
	rEo.StructSize = sizeof(rEo);
	{
		switch(Format) {
			case crexpfmtPdf:
				rEo.formatType = UXFPdfType; 
				{
					rEo.formatOptions = new Crr_ExportOptions_PdfRtfDoc;
					rEo.nFormatOptionsBytes = sizeof(Crr_ExportOptions_PdfRtfDoc);
				}
				STRNSCPY(rEo.formatDLLName, "u2fpdf");
				break;
			case crexpfmtRtf: 
				rEo.formatType = UXFRichTextFormatType; 
				{
					rEo.formatOptions = new Crr_ExportOptions_PdfRtfDoc;
					rEo.nFormatOptionsBytes = sizeof(Crr_ExportOptions_PdfRtfDoc);
				}
				STRNSCPY(rEo.formatDLLName, "u2frtf");
				break;
			case crexpfmtWinWord: 
				rEo.formatType = UXFWordWinType; 
				{
					rEo.formatOptions = new Crr_ExportOptions_PdfRtfDoc;
					rEo.nFormatOptionsBytes = sizeof(Crr_ExportOptions_PdfRtfDoc);
				}
				STRNSCPY(rEo.formatDLLName, "u2fwordw");
				break;
			case crexpfmtHtml: 
				rEo.formatType = 3; 
				{
					rEo.formatOptions = 0;
					rEo.nFormatOptionsBytes = 0;
				}
				STRNSCPY(rEo.formatDLLName, "u2fhtml");
				break; 
			case crexpfmtExcel: 
				
				{
					Crr_ExportOptions_Xls * p_o = new Crr_ExportOptions_Xls;
					rEo.formatOptions = p_o;
					rEo.nFormatOptionsBytes = sizeof(Crr_ExportOptions_Xls);
					if(Flags & fXlsDataOnly) {
						rEo.formatType = UXFXls7ExtType;
						p_o->baseAreaType = PE_SECT_DETAIL;
					}
					else {
						rEo.formatType = 9; // there is no mnemonic 
						p_o->baseAreaType = 0x00ff;
						if(PageRange.low > 0 && PageRange.upp >= PageRange.low) {
							p_o->AllPages = 0;
							p_o->FirstPage = PageRange.low;
							p_o->LastPage = PageRange.upp;
						}
						else {
							p_o->AllPages = 1;
						}
					}
					p_o->fConstColWidth = 720.0;
					p_o->bColumnHeadings = 1;//BIN(Flags & fXlsColumnHeadings);
					p_o->bConvertDateToStr = BIN(Flags & fXlsCvtDateToString);
					p_o->bCreatePgBrkForEachPage = BIN(Flags & fXlsCreatePgBrkForEachPage);
					p_o->bShowGrid = BIN(Flags & fXlsShowGrid);
					p_o->bTabularFormat = BIN(Flags & fXlsTabularFormat);
					p_o->Unkn_One2 = 1;
					p_o->Unkn_One = 1;
					p_o->Unkn_One3 = 1;
				}
				STRNSCPY(rEo.formatDLLName, "u2fxls");
				break;
			case crexpfmtCsv: 
				rEo.formatType = UXFCommaSeparatedType; 
				{
					rEo.formatOptions = new Crr_ExportOptions_Csv;
					rEo.nFormatOptionsBytes = sizeof(Crr_ExportOptions_Csv);
					//
					static_cast<Crr_ExportOptions_Csv *>(rEo.formatOptions)->FieldSep = static_cast<uint8>(CsvFieldSeparator);
					static_cast<Crr_ExportOptions_Csv *>(rEo.formatOptions)->QuoteChar = (Flags & fCsvQuoteText) ? '\"' : 0;
					static_cast<Crr_ExportOptions_Csv *>(rEo.formatOptions)->UseReportNumberFormat = BIN(Flags & fCsvUseReportNumberFormat);
					static_cast<Crr_ExportOptions_Csv *>(rEo.formatOptions)->UseReportDateFormat = BIN(Flags & fCsvUseReportDateFormat);
				}
				STRNSCPY(rEo.formatDLLName, "u2fsepv");
				break;
		}
	}
	{
		temp_buf.Z();
		switch(Destination) {
			case destApp:
				rEo.destinationType = UXDApplicationType;
				CrystalReportExportParam::GetDestDll(Destination, temp_buf);
				break;
			case destFile:
				rEo.destinationType = UXDDiskType;
				CrystalReportExportParam::GetDestDll(Destination, temp_buf);
				break;
			default:
				rEo.destinationType = UXDDiskType;
				CrystalReportExportParam::GetDestDll(destFile, temp_buf);
				break;
		}
		if(DestFileName.NotEmpty()) {
			Crr_Destination * p_dest_block = new Crr_Destination();
			p_dest_block->SetFileName(DestFileName);
			rEo.destinationOptions = p_dest_block;
			rEo.nDestinationOptionsBytes = sizeof(*p_dest_block);
		}
		STRNSCPY(rEo.destinationDLLName, temp_buf);
	}
	if(!isempty(rEo.formatDLLName) && !isempty(rEo.destinationDLLName)) {
		ok = 1;
	}
	return ok;
}

int LoadExportOptions(const char * pReportName, PEExportOptions * pOptions, bool * pSilent, SString & rPath)
{
	int    ok = 1;
	int    silent = 0;
	PEExportOptions options;
	SString fname;
	PPGetFilePath(PPPATH_BIN, PPFILNAM_REPORT_INI, fname);
	MEMSZERO(options);
	options.StructSize = sizeof(options);
	if(fileExists(fname)) {
		int    idx = -1;
		int    r = 0;
		uint   i = 0;
		SString types;
		SString format;
		SString dest;
		SString buf;
		SString param_symb;
		PPIniFile ini_file(fname);
		PPIniFile::GetParamSymb(PPINIPARAM_REPORT_SILENT, param_symb);
		THROW(r = ini_file.GetIntParam(pReportName, param_symb, &silent));
		if(r == -1) {
			THROW(ini_file.GetIntParam("default", param_symb, &silent));
		}
		if(silent) {
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_FORMAT, param_symb);
			THROW(ini_file.GetParam(pReportName, param_symb, format));
			if(!format.NotEmptyS()) {
				THROW(ini_file.GetParam("default", param_symb, format));
			}
			PPLoadText(PPTXT_EXPORT_FORMATTYPES, types);
			{
				StringSet ss(',', format);
				memzero(options.formatDLLName, PE_DLL_NAME_LEN);
				memzero(options.destinationDLLName, PE_DLL_NAME_LEN);
				if(ss.get(&(i = 0), buf)) {
					if(PPSearchSubStr(types, &idx, buf, 1) > 0) {
						const static DWORD FormatTypes[] = {
							UXFWordWinType,
							UXFRichTextFormatType,
							UXFCommaSeparatedType,
							UXFTabSeparatedType,
							UXFCharSeparatedType,
							UXFTextType,
							UXFXls7ExtType,
							UXFPdfType
						};
						PPGetSubStr(PPTXT_EXPORT_FORMATDLL32, idx, options.formatDLLName, PE_DLL_NAME_LEN);
						options.formatType = FormatTypes[idx];
						switch(idx) {
							case 2: // FormatTypes[2] == UXFCommaSeparatedType
							case 3: // FormatTypes[3] == UXFTabSeparatedType
								options.formatOptions = new UXFCommaTabSeparatedOptions;
								options.nFormatOptionsBytes = sizeof(UXFCommaTabSeparatedOptions); // @v11.3.1
								static_cast<UXFCommaTabSeparatedOptions *>(options.formatOptions)->structSize = UXFCommaTabSeparatedOptionsSize;
								break;
							case 4: // FormatTypes[4] == UXFCharSeparatedType
								options.formatOptions = new UXFCharSeparatedOptions;
								options.nFormatOptionsBytes = sizeof(UXFCharSeparatedOptions); // @v11.3.1
								static_cast<UXFCharSeparatedOptions *>(options.formatOptions)->structSize = UXFCharSeparatedOptionsSize;
								break;
							case 6: // FormatTypes[6] == UXFXls7ExtType
								options.formatOptions = new UXFXlsOptions;
								options.nFormatOptionsBytes = sizeof(UXFXlsOptions); // @v11.3.1
								static_cast<UXFXlsOptions *>(options.formatOptions)->structSize = UXFXlsOptionsSize;
								break;
							case 7: // FormatTypes[7] == UXFPdfType
								options.formatOptions = new UXFPdfOptions;
								options.nFormatOptionsBytes = sizeof(UXFPdfOptions); // @v11.3.1
								static_cast<UXFPdfOptions *>(options.formatOptions)->structSize = UXFPdfOptionsSize;
								break;
							default:
								options.formatOptions = 0;
								options.nFormatOptionsBytes = 0; // @v11.3.1
								break;
						}
					}
				}
				switch(idx) {
					case 2: // FormatTypes[2] == UXFCommaSeparatedType
					case 3: // FormatTypes[3] == UXFTabSeparatedType
						{
							UXFCommaTabSeparatedOptions * p_options = static_cast<UXFCommaTabSeparatedOptions *>(options.formatOptions);
							p_options->useReportDateFormat = 0;
							if(ss.get(&i, buf))
								p_options->useReportDateFormat = buf.ToLong();
							p_options->useReportNumberFormat = 0;
							if(ss.get(&i, buf))
								p_options->useReportNumberFormat = buf.ToLong();
						}
						break;
					case 4: // FormatTypes[4] == UXFCharSeparatedType
						{
							char host_buf[128];
							UXFCharSeparatedOptions * p_options = static_cast<UXFCharSeparatedOptions *>(options.formatOptions);
							p_options->useReportDateFormat = 0;
							if(ss.get(&i, buf))
								p_options->useReportDateFormat = buf.ToLong();
							p_options->useReportNumberFormat = 0;
							if(ss.get(&i, buf))
								p_options->useReportNumberFormat = buf.ToLong();
							p_options->stringDelimiter = '=';
							if(ss.get(&i, buf)) {
								hostrtocstr(buf, host_buf, sizeof(host_buf));
								p_options->stringDelimiter = host_buf[0];
							}
							p_options->fieldDelimiter = 0;
							if(ss.get(&i, buf)) {
								hostrtocstr(buf, host_buf, sizeof(host_buf));
								p_options->fieldDelimiter = new char[sizeof(host_buf)];
								strnzcpy(p_options->fieldDelimiter, host_buf, sizeof(host_buf));
							}
						}
						break;
					case 6: // FormatTypes[6] == UXFXls7ExtType
						{
							UXFXlsOptions * p_options = static_cast<UXFXlsOptions *>(options.formatOptions);
							p_options->bColumnHeadings = 0;
							if(ss.get(&i, buf))
								p_options->bColumnHeadings = buf.ToLong();
							p_options->bTabularFormat = 0;
							if(ss.get(&i, buf))
								p_options->bTabularFormat = buf.ToLong();
							p_options->bUseConstColWidth = 0;
							if(ss.get(&i, buf))
								p_options->bUseConstColWidth = buf.ToLong();
							if(ss.get(&i, buf))
								p_options->baseAreaGroupNum = (WORD)buf.ToLong();
							if(ss.get(&i, buf))
								p_options->baseAreaType = (WORD)buf.ToLong();
							if(ss.get(&i, buf))
								p_options->fConstColWidth = buf.ToReal();
						}
						break;
					case 7: // FormatTypes[7] == UXFPdfType
						{
							UXFPdfOptions * p_options = static_cast<UXFPdfOptions *>(options.formatOptions);
							p_options->UsePageRange = false;
						}
						break;
					default:
						if(idx < 0)
							ok = -1;
						break;
				}
				char   ext[/*MAXEXT*/32];
				PPGetSubStr(PPTXT_EXPORT_EXTS, idx, ext, sizeof(ext));
				PPIniFile::GetParamSymb(PPINIPARAM_REPORT_DESTINATION, param_symb);
				THROW(ini_file.GetParam(pReportName, param_symb, dest));
				if(!dest.NotEmptyS())
					THROW(ini_file.GetParam("default", param_symb, dest));
				PPLoadText(PPTXT_EXPORT_DESTTYPES, types);
				idx = -1;
				{
					static const DWORD DestinationTypes[] = { UXDApplicationType, UXDDiskType, UXDMailType };

					StringSet ss2(',', dest);
					if(ss2.get(&(i = 0), buf) && PPSearchSubStr(types, &idx, buf, 1) > 0) {
						PPGetSubStr(PPTXT_EXPORT_DESTDLL32, idx, options.destinationDLLName, PE_DLL_NAME_LEN);
						options.destinationType = DestinationTypes[idx];
						switch(idx) {
							case 0: // DestinationTypes[0] == UXDApplicationType
								options.destinationOptions = new UXDApplicationOptions;
								options.nDestinationOptionsBytes = sizeof(UXDApplicationOptions); // @v11.3.1
								static_cast<UXDApplicationOptions *>(options.destinationOptions)->structSize = UXDApplicationOptionsSize;
								break;
							case 1: // DestinationTypes[1] == UXDDiskType
								options.destinationOptions = new UXDDiskOptions;
								options.nDestinationOptionsBytes = sizeof(UXDDiskOptions); // @v11.3.1
								static_cast<UXDDiskOptions *>(options.destinationOptions)->structSize = UXDDiskOptionsSize;
								break;
							default:
								options.destinationOptions = 0;
								break;
						}
					}
					buf.Z();
					if(ss2.get(&i, buf)) {
						SString dir;
						if(buf.NotEmptyS()) {
							if(SFile::IsDir(buf.RmvLastSlash())) {
								THROW_SL(MakeTempFileName(dir = buf, "exp", ext, 0, buf));
							}
							else {
								SFsPath::ReplaceExt(buf, ext, 1);
							}
						}
						else {
							PPGetPath(PPPATH_OUT, dir);
							THROW_SL(MakeTempFileName(dir, "exp", ext, 0, buf));
						}
						rPath = buf;
						switch(idx) {
							case 0: // DestinationTypes[0] == UXDApplicationType
								if(options.destinationOptions)
									static_cast<UXDApplicationOptions *>(options.destinationOptions)->fileName = newStr(buf);
								break;
							case 1: // DestinationTypes[1] == UXDDiskType
								if(options.destinationOptions)
									static_cast<UXDDiskOptions *>(options.destinationOptions)->fileName = newStr(buf);
								break;
							case 2: // DestinationTypes[1] == UXDMapi
								break;
							default:
								if(idx < 0)
									ok = -1;
								break;
						}
					}
					else
						ok = -1;
				}
			}
		}
		else
			ok = -1;
	}
	CATCHZOK
	ASSIGN_PTR(pOptions, options);
	ASSIGN_PTR(pSilent, (ok > 0) ? LOGIC(silent) : false);
	return ok;
}
//
//
//
static int RemoveCompName(SString & rPrintDevice)
{
	wchar_t buf[256];
	SString sbuf;
	DWORD  buf_size = SIZEOFARRAY(buf);
	buf[0] = 0;
	GetComputerNameExW(ComputerNameNetBIOS, buf, &buf_size);
	(sbuf = "\\\\").Cat(SUcSwitchW(buf)).BSlash();
	if(rPrintDevice.CmpPrefix(sbuf, 1) == 0)
		rPrintDevice.ShiftLeft(sbuf.Len());
	else if(rPrintDevice.C(0) == '\\' && rPrintDevice.C(1) == '\\') {
		//long val = atol(onecstr(rPrintDevice.C(2)));
		if(isdec(rPrintDevice.C(2))) {
			long val = rPrintDevice.C(2) - '0';
			InetAddr addr;
			// @todo Исправить ошибку в GetFirstHostByMACAddr
			//MACAddr  mac_addr;
			//GetFirstMACAddr(&mac_addr);
			//GetFirstHostByMACAddr(&mac_addr, &addr);
			//addr.ToStr(InetAddr::fmtHost, sbuf);
			//rPrintDevice.ReplaceStr(sbuf, "", 1);
		}
	}
	return 1;
}

static int SetPrinterParam(short hJob, const char * pPrinter, long options, const DEVMODEA * pDevMode)
{
	int    ok = 1;
	// @v12.3.11 SString print_device(isempty(pPrinter) ? DS.GetConstTLA().PrintDevice : pPrinter);
	SString print_device(pPrinter); // @v12.3.11 Удалили зависимость от DS.GetConstTLA().PrintDevice (see CrystalReportPrint2_ClientExecution)
	RemoveCompName(print_device);
	DEVMODEA * p_dm = 0, dm;
	char   device_name[128];
	char   port_name[128];
	char   drv_name[64];
	HANDLE h_drv = 0;
	HANDLE h_prn = 0;
	HANDLE h_port = 0;
	short  drv_len, prn_len, port_len;
	memzero(device_name, sizeof(device_name));
	memzero(port_name, sizeof(port_name));
	memzero(drv_name, sizeof(drv_name));
	// @erik v10.4.10 {
	if(pDevMode) {
		RVALUEPTR(dm, pDevMode);
		THROW_PP(PESelectPrinter(hJob, drv_name, device_name, port_name, &dm), PPERR_CRYSTAL_REPORT); // @unicodeproblem
	}
	else { // } @erik v10.4.10 
		if(options & SPRN_USEDUPLEXPRINTING || print_device.NotEmptyS()) {
			THROW_PP(PEGetSelectedPrinter(hJob, &h_drv, &drv_len, &h_prn, &prn_len, &h_port, &port_len, &p_dm), PPERR_CRYSTAL_REPORT); // @unicodeproblem
			if(!RVALUEPTR(dm, p_dm))
				MEMSZERO(dm);
			PEGetHandleString(h_prn,  device_name, sizeof(device_name));
			PEGetHandleString(h_port, port_name, sizeof(port_name));
			PEGetHandleString(h_drv,  drv_name, sizeof(drv_name));
			if(print_device.NotEmpty())
				STRNSCPY(device_name, print_device);
			if(options & SPRN_USEDUPLEXPRINTING) {
				DWORD  is_duplex_device = DeviceCapabilitiesA(device_name, port_name, DC_DUPLEX, 0, p_dm); // @unicodeproblem
				if(is_duplex_device) {
					RVALUEPTR(dm, p_dm);
					dm.dmFields |= DM_DUPLEX;
					dm.dmDuplex = DMDUP_VERTICAL;
				}
			}
			THROW_PP(PESelectPrinter(hJob, drv_name, device_name, port_name, &dm), PPERR_CRYSTAL_REPORT); // @unicodeproblem
		}
	}
	CATCH
		CrwError = PEGetErrorCode(hJob);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//	
//
struct SvdtStrDlgAns { // @Vadim 13.09.02 @{savereportdata} 
	int   SvDt;
	int   EdRep;
	SString SvDtPath_;
	SString EdRepPath_;
};

static int GetSvdtStrOpt(SvdtStrDlgAns * pSsda) // @Vadim 13.09.02
{ 
	class SvdtStrDialog : public TDialog {
		DECL_DIALOG_DATA(SvdtStrDlgAns);
		enum {
			ctlgroupFbb1 = 1,
			ctlgroupFbb2 = 2
		};
	public:
		explicit SvdtStrDialog(uint dlgID) : TDialog(dlgID)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_SAVEDATA_SVDTPATH, CTL_SAVEDATA_SVDTPATH, ctlgroupFbb1,
				PPTXT_TITLE_DATASTRUCSAVING, PPTXT_FILPAT_DDFBTR, FileBrowseCtrlGroup::fbcgfPath);
			FileBrowseCtrlGroup::Setup(this, CTLBRW_SAVEDATA_EDREPPTH, CTL_SAVEDATA_EDREPPATH, ctlgroupFbb2,
				0, PPTXT_FILPAT_REPORT, FileBrowseCtrlGroup::fbcgfFile);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_SAVEDATA_SVDT, &Data.SvDt);
			setCtrlString(CTL_SAVEDATA_SVDTPATH, Data.SvDtPath_);
			setCtrlString(CTL_SAVEDATA_EDREPPATH, Data.EdRepPath_);
			if(Data.EdRep)
				setCtrlData(CTL_SAVEDATA_EDREP, &Data.EdRep);
			else {
				disableCtrl(CTL_SAVEDATA_EDREP, true);
				disableCtrl(CTL_SAVEDATA_EDREPPATH, true);
				showCtrl(CTLBRW_SAVEDATA_EDREPPTH, false);
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			getCtrlData(CTL_SAVEDATA_SVDT, &Data.SvDt);
			getCtrlString(CTL_SAVEDATA_SVDTPATH, Data.SvDtPath_);
			getCtrlData(CTL_SAVEDATA_EDREP, &Data.EdRep);
			getCtrlString(CTL_SAVEDATA_EDREPPATH, Data.EdRepPath_);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	};
	DIALOG_PROC_BODY_P1(SvdtStrDialog, DLG_SAVEDATA, pSsda); 
}

int SaveReportDataStruct(const char * pDataName, const char * pTempPath, const char * pRepFileName)
{
	int    ok = -1;
	SString path;
	SString fname;
	SString cr_path_;
	SvdtStrDlgAns * p_ssda = 0;
	PPGetPath(PPPATH_REPORTDATA, path);
	if(path.NotEmptyS() && CrwError == PE_ERR_ERRORINDATABASEDLL) {
		path.SetLastSlash().Cat(pDataName);
		THROW_MEM(p_ssda = new SvdtStrDlgAns);
		p_ssda->SvDt = 1;
		p_ssda->SvDtPath_ = path;
		p_ssda->EdRep = FindExeByExt2(sstrchr(pRepFileName, '.'), cr_path_, "CrystalReports.9.1");
		p_ssda->EdRepPath_ = pRepFileName;
		if(GetSvdtStrOpt(p_ssda) > 0) {
			if(p_ssda->SvDt) {
				path = p_ssda->SvDtPath_;
				if(!fileExists(path))
					THROW_SL(SFile::CreateDir(path));
				CopyDataStruct(pTempPath, path, BDictionary::DdfTableFileName);
				CopyDataStruct(pTempPath, path, BDictionary::DdfFieldFileName);
				CopyDataStruct(pTempPath, path, BDictionary::DdfIndexFileName);
				CopyDataStruct(pTempPath, path, PPGetFileName(PPFILNAM_HEAD_BTR, fname));
				CopyDataStruct(pTempPath, path, PPGetFileName(PPFILNAM_ITER_BTR, fname));
			}
			if(p_ssda->EdRep)
				spawnl(_P_NOWAIT, cr_path_, cr_path_, p_ssda->EdRepPath_, 0);
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_ssda;
	return ok;
}

static void DebugOutputCrrExportOptions(const PEExportOptions & rEo, const char * pFileName)
{
	if(!isempty(pFileName)) {
		SString debug_file_path;
		PPGetFilePath(PPPATH_LOG, pFileName, debug_file_path);
		SString out_buf;
		out_buf.Cat(rEo.formatDLLName).Tab().Cat(rEo.nFormatOptionsBytes).CR();
		out_buf.Tab().Cat("fmtopt").CatDiv(':', 2).CatHex(rEo.formatOptions, rEo.nFormatOptionsBytes).CR();
		out_buf.Tab().Cat("dstopt").CatDiv(':', 2).CatHex(rEo.destinationOptions, rEo.nDestinationOptionsBytes).CR();
		SFile f_out(debug_file_path, SFile::mAppend);
		f_out.WriteLine(out_buf);
	}
}

int CrystalReportPrint2_Local(const CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply, void * hParentWindowForPreview) // @v11.9.5 
{
	const  bool modal_preview = true;
	int    ok = 1;
	short  h_job = 0;
	SString msg_buf;
	PEReportOptions ro;
	ro.StructSize = sizeof(ro);
	if(rBlk.Action == CrystalReportPrintParamBlock::actionExport) {
		bool   silent = false;
		bool   do_export = true;
		const char * p__dest_fn = 0;
		SString path;
		SString temp_buf;
		PEExportOptions eo;
		h_job = PEOpenPrintJob(rBlk.ReportPath);
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		{
			// @v12.3.11 {
			bool   export_options_done = false;
			if(rBlk.ExpParam.Format) {
				if(rBlk.ExpParam.TranslateToCrrExportOptions(eo)) {
					//DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions-own.log"); // @debug
					p__dest_fn = rBlk.ExpParam.DestFileName.cptr();
					export_options_done = true;
				}
			}
			// } @v12.3.11 
			if(!export_options_done) {
				eo.StructSize = sizeof(eo);
				eo.destinationOptions = 0;
				eo.formatOptions      = 0;
				THROW(LoadExportOptions(rBlk.ReportName, &eo, &silent, path));
				if(silent) {
					const char * p_dest_fn_2 = *reinterpret_cast<const char * const *>(PTR8C(eo.destinationOptions)+2);
					p__dest_fn = p_dest_fn_2;
				}
				else if(PEGetExportOptions(h_job, &eo)) {
					//DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions.log"); // @debug
					const char * p_dest_fn_1 = reinterpret_cast<const char *>(PTR8C(eo.destinationOptions)+2);
					p__dest_fn = p_dest_fn_1;
				}
				else
					do_export = false;
			}
		}
		if(do_export) {
			PEExportTo(h_job, &eo);
			THROW(SetupReportLocations(h_job, rBlk.Dir, 0));
			if(rBlk.Options & SPRN_SKIPGRPS)
				SetupGroupSkipping(h_job);
			THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
			if(rBlk.EmailAddr.NotEmpty()) {
				if(fileExists(p__dest_fn)) {
					//
					// Отправка на определенный почтовый адрес
					//
					// @v12.3.11 {
					if(/*rBlk.InetAcc.NotEmpty()*/true) {
						StrAssocArray mail_adr_list;
						SStrCollection file_name_collection;
						mail_adr_list.AddFast(1, rBlk.EmailAddr);
						file_name_collection.insert(newStr(p__dest_fn));
						if(SendMail(rBlk.ReportName, rBlk.ReportName, &mail_adr_list, &rBlk.InetAcc, &file_name_collection, 0/*pLogger*/)) {
							// Отправка отчета на электронную почту: success 
							PPLoadString("reportsendmail", temp_buf);
							temp_buf.CatDiv(':', 2).Cat("success").Space().Cat(rBlk.ReportName).Space().Cat(rBlk.EmailAddr);
							PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
						}
						else {
							// PPERR_REPORT_SENDMAIL_IMPL Ошибка отправки отчета на электронную почту"
							PPGetLastErrorMessage(1, temp_buf); // last error text
							PPGetMessage(mfError, PPERR_REPORT_SENDMAIL_IMPL, 0, 1, msg_buf);
							PPLogMessage(PPFILNAM_ERR_LOG, msg_buf.CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
						}
					}
					else {
						// @todo Здесь должен быть другой текст сообщения!
						// 
						// PPERR_REPORT_SENDMAIL_NOACC Отправка отчета на электронную почту: конфигурация глобального обмена не содержит ссылку на почтовую учетную запись
						PPSetError(PPERR_REPORT_SENDMAIL_NOACC);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					}
					// } @v12.3.11 
				}
				else {
					// PPERR_REPORT_SENDMAIL_NOFILE Отправка отчета на электронную почту: результирующий файл '%s' не найден
					PPSetError(PPERR_REPORT_SENDMAIL_NOFILE, p__dest_fn);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				}
			}
		}
	}
	else {
		const DEVMODEA * p_dev_mode = (rBlk.InternalFlags & CrystalReportPrintParamBlock::intfDevModeValid) ? &rBlk.DevMode : 0;
		h_job = PEOpenPrintJob(rBlk.ReportPath);
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		THROW(SetPrinterParam(h_job, rBlk.Printer, rBlk.Options, p_dev_mode));
		THROW(SetupReportLocations(h_job, rBlk.Dir, (rBlk.Options & SPRN_DONTRENAMEFILES) ? 0 : 1));
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			THROW_PP(PEOutputToWindow(h_job, "", CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT, WS_MAXIMIZE|WS_VISIBLE|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU, 0), PPERR_CRYSTAL_REPORT);
		}
		else {
			THROW_PP(PEOutputToPrinter(h_job, rBlk.NumCopies), PPERR_CRYSTAL_REPORT);
		}
		if(rBlk.Options & SPRN_SKIPGRPS)
			SetupGroupSkipping(h_job);
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			struct CrrPreviewEventParam_Local {
				static BOOL CALLBACK EventCallback(short eventID, void * pParam, void * pUserData)
				{
					CrrPreviewEventParam_Local * p_self = static_cast<CrrPreviewEventParam_Local *>(pUserData);
					if(eventID == PE_CLOSE_PRINT_WINDOW_EVENT && p_self && p_self->ModalPreview) {
						::EnableWindow(/*APPL->H_TopOfStack*/p_self->HwParent, TRUE);
						p_self->StopPreview++;
					}
					return TRUE;
				}
				CrrPreviewEventParam_Local(HWND hParent, bool modalPreview) : HwParent(hParent), ModalPreview(modalPreview), StopPreview(0)
				{
				}
				HWND   HwParent;
				int    StopPreview;
				bool   ModalPreview;
			};
			HWND h_parent_window = reinterpret_cast<HWND>(hParentWindowForPreview);
			CrrPreviewEventParam_Local pep(h_parent_window, modal_preview);
			PEEnableEventInfo event_info;
			event_info.StructSize = sizeof(PEEnableEventInfo);
			event_info.closePrintWindowEvent = TRUE;
			event_info.startStopEvent = TRUE;
			THROW_PP(PEEnableEvent(h_job, &event_info), PPERR_CRYSTAL_REPORT);
			THROW_PP(PESetEventCallback(h_job, CrrPreviewEventParam_Local::EventCallback, &pep), PPERR_CRYSTAL_REPORT);
			THROW_PP(PEStartPrintJob(h_job, /*TRUE*/modal_preview/*waitUntilDone*/), PPERR_CRYSTAL_REPORT);
			if(modal_preview) {
				if(h_parent_window) {
					::EnableWindow(h_parent_window, FALSE); // Запрещает работу в программе пока окно просмотра активно
					APPL->MsgLoop(0, pep.StopPreview);
				}
			}
		}
		else {
			const uint64 profile_start = SLS.GetProfileTime();
			THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
			const uint64 profile_end = SLS.GetProfileTime();
			{
				msg_buf.Z().CatEq("Report", rBlk.ReportPath);
				if(rBlk.Printer.NotEmpty()) // Ранее был pPrinter @erik v10.4.10
					msg_buf.CatDiv(';', 2).CatEq("Printer", rBlk.Printer.NotEmpty()); // Ранее был pPrinter @erik v10.4.10
				if(rBlk.NumCopies > 1)
					msg_buf.CatDiv(';', 2).CatEq("Copies", rBlk.NumCopies);
				msg_buf.CatDiv(';', 2).CatEq("Mks", (profile_end - profile_start));
				PPLogMessage(PPFILNAM_REPORTING_LOG, msg_buf, LOGMSGF_USER | LOGMSGF_TIME | LOGMSGF_DBINFO);
			}
		}
	}
	rReply.Z();
	CATCH
		{
			const short crw_err_code = PEGetErrorCode(h_job);
			CrwError = crw_err_code;
			rReply.Code = crw_err_code;
		}
		// @debug {
		if(rBlk.Action != CrystalReportPrintParamBlock::actionExport) {
			PPLoadString("err_crpe", msg_buf);
			msg_buf.CatDiv(':', 2).Cat(CrwError);
			rReply.Descr = msg_buf;
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_COMP|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		// } @debug
		ok = 0;
	ENDCATCH
	if(h_job)
		PEClosePrintJob(h_job);
	return ok;
}

static int SendReportLaunchingResultToClientsPipe(SIntHandle hPipe, const SJson & rJs)
{
	int    ok = 1;
	if(hPipe) {
		STempBuffer wr_buf(SKILOBYTE(1));
		SString temp_buf;
		rJs.ToStr(temp_buf);
		temp_buf.CopyTo(wr_buf, wr_buf.GetSize());
		DWORD reply_size = temp_buf.Len()+1;
		DWORD wr_size = 0;
		boolint wr_ok = ::WriteFile(hPipe, wr_buf, reply_size, &wr_size, NULL/*not overlapped I/O*/);
		if(wr_ok) {
			ok = 1;
		}
		else {
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}
//
// Эта функция вызывается только из процесса crr32_support!
//
int CrystalReportPrint2_Server(const CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply, void * hParentWindowForPreview, SIntHandle hPipe)
{
	int    ok = 1;
	short  h_job = 0;
	SString temp_buf;
	SString msg_buf;
	SJson  js_reply(SJson::tOBJECT); // @v12.3.12
	bool   is_reply_sent = false; // @v12.3.12
	PEReportOptions ro;
	ro.StructSize = sizeof(ro);
	if(rBlk.Action == CrystalReportPrintParamBlock::actionExport) {
		bool   silent = false;
		bool   do_export = true;
		const char * p__dest_fn = 0;
		SString path;
		PEExportOptions eo;
		h_job = PEOpenPrintJob(rBlk.ReportPath);
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);

		PEEnableProgressDialog(h_job, FALSE);
		PESetAllowPromptDialog(h_job, FALSE);
		PEShowPrintControls(h_job, FALSE);

		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		{
			// @v12.3.11 {
			bool   export_options_done = false;
			if(rBlk.ExpParam.Format) {
				if(rBlk.ExpParam.TranslateToCrrExportOptions(eo)) {
					//DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions-own.log"); // @debug
					p__dest_fn = rBlk.ExpParam.DestFileName.cptr();
					export_options_done = true;
				}
			}
			// } @v12.3.11 
			if(!export_options_done) {
				eo.StructSize = sizeof(eo);
				eo.destinationOptions = 0;
				eo.formatOptions      = 0;
				THROW(LoadExportOptions(rBlk.ReportName, &eo, &silent, path));
				if(silent) {
					const char * p_dest_fn_2 = *reinterpret_cast<const char * const *>(PTR8C(eo.destinationOptions)+2);
					p__dest_fn = p_dest_fn_2;
				}
				else if(PEGetExportOptions(h_job, &eo)) {
					//DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions.log"); // @debug
					const char * p_dest_fn_1 = reinterpret_cast<const char *>(PTR8C(eo.destinationOptions)+2);
					p__dest_fn = p_dest_fn_1;
				}
				else
					do_export = false;
			}
		}
		if(do_export) {
			PEExportTo(h_job, &eo);
			THROW(SetupReportLocations(h_job, rBlk.Dir, 0));
			if(rBlk.Options & SPRN_SKIPGRPS)
				SetupGroupSkipping(h_job);
			THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
			if(rBlk.EmailAddr.NotEmpty()) {
				if(fileExists(p__dest_fn)) {
					//
					// Отправка на определенный почтовый адрес
					//
					// @v12.3.11 {
					if(/*rBlk.InetAcc.NotEmpty()*/true) {
						StrAssocArray mail_adr_list;
						SStrCollection file_name_collection;
						mail_adr_list.AddFast(1, rBlk.EmailAddr);
						file_name_collection.insert(newStr(p__dest_fn));
						if(SendMail(rBlk.ReportName, rBlk.ReportName, &mail_adr_list, &rBlk.InetAcc, &file_name_collection, 0/*pLogger*/)) {
							// Отправка отчета на электронную почту: success 
							PPLoadString("reportsendmail", temp_buf);
							temp_buf.CatDiv(':', 2).Cat("success").Space().Cat(rBlk.ReportName).Space().Cat(rBlk.EmailAddr);
							PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
						}
						else {
							// PPERR_REPORT_SENDMAIL_IMPL Ошибка отправки отчета на электронную почту"
							PPGetLastErrorMessage(1, temp_buf); // last error text
							PPGetMessage(mfError, PPERR_REPORT_SENDMAIL_IMPL, 0, 1, msg_buf);
							PPLogMessage(PPFILNAM_ERR_LOG, msg_buf.CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
						}
					}
					else {
						// @todo Здесь должен быть другой текст сообщения!
						// 
						// PPERR_REPORT_SENDMAIL_NOACC Отправка отчета на электронную почту: конфигурация глобального обмена не содержит ссылку на почтовую учетную запись
						PPSetError(PPERR_REPORT_SENDMAIL_NOACC);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					}
					// } @v12.3.11 
				}
				else {
					// PPERR_REPORT_SENDMAIL_NOFILE Отправка отчета на электронную почту: результирующий файл '%s' не найден
					PPSetError(PPERR_REPORT_SENDMAIL_NOFILE, p__dest_fn);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				}
			}
		}
	}
	else {
		HWND h_parent_window = reinterpret_cast<HWND>(hParentWindowForPreview);
		const DEVMODEA * p_dev_mode = (rBlk.InternalFlags & CrystalReportPrintParamBlock::intfDevModeValid) ? &rBlk.DevMode : 0;
		h_job = PEOpenPrintJob(rBlk.ReportPath);
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		THROW(SetPrinterParam(h_job, rBlk.Printer, rBlk.Options, p_dev_mode));
		THROW(SetupReportLocations(h_job, rBlk.Dir, (rBlk.Options & SPRN_DONTRENAMEFILES) ? 0 : 1));
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			{
				//
				// Посылаем ответ клиенту до того как запустим предпросмотр отчета, иначе клиенту придется ждать пока пользователь не закроет окно просмотра.
				//
				js_reply.InsertString("status", "ok");
				if(SendReportLaunchingResultToClientsPipe(hPipe, js_reply) > 0) {
					is_reply_sent = true;
				}
			}
			THROW_PP(PEOutputToWindow(h_job, "", CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT, WS_MAXIMIZE|WS_VISIBLE|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU, 0), PPERR_CRYSTAL_REPORT);
		}
		else {
			THROW_PP(PEOutputToPrinter(h_job, rBlk.NumCopies), PPERR_CRYSTAL_REPORT);
		}
		if(rBlk.Options & SPRN_SKIPGRPS)
			SetupGroupSkipping(h_job);
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			struct CrrPreviewEventParam_Server {
				static BOOL CALLBACK EventCallback(short eventID, void * pParam, void * pUserData)
				{
					CrrPreviewEventParam_Server * p_self = static_cast<CrrPreviewEventParam_Server *>(pUserData);
					if(eventID == PE_CLOSE_PRINT_WINDOW_EVENT && p_self) {
						if(p_self->HwParent)
							::DestroyWindow(p_self->HwParent); // Завершаем процесс
					}
					return TRUE;
				}
				CrrPreviewEventParam_Server(HWND hParent, bool modalPreview) : HwParent(hParent), ModalPreview(modalPreview), StopPreview(0)
				{
				}
				HWND   HwParent;
				int    StopPreview;
				bool   ModalPreview;
			};
			const  bool modal_preview = true;
			CrrPreviewEventParam_Server * p_pep = new CrrPreviewEventParam_Server(h_parent_window, modal_preview);
			PEEnableEventInfo event_info;
			event_info.StructSize = sizeof(PEEnableEventInfo);
			event_info.closePrintWindowEvent = TRUE;
			event_info.startStopEvent = TRUE;
			THROW_PP(PEEnableEvent(h_job, &event_info), PPERR_CRYSTAL_REPORT);
			THROW_PP(PESetEventCallback(h_job, CrrPreviewEventParam_Server::EventCallback, p_pep), PPERR_CRYSTAL_REPORT);
			THROW_PP(PEStartPrintJob(h_job, /*TRUE*/modal_preview/*waitUntilDone*/), PPERR_CRYSTAL_REPORT);
			/*if(modal_preview) {
				if(h_parent_window) {
					::EnableWindow(h_parent_window, FALSE); // Запрещает работу в программе пока окно просмотра активно
					APPL->MsgLoop(0, pep.StopPreview);
				}
			}*/
		}
		else {
			const uint64 profile_start = SLS.GetProfileTime();
			THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
			const uint64 profile_end = SLS.GetProfileTime();
			{
				msg_buf.Z().CatEq("Report", rBlk.ReportPath);
				if(rBlk.Printer.NotEmpty()) // Ранее был pPrinter @erik v10.4.10
					msg_buf.CatDiv(';', 2).CatEq("Printer", rBlk.Printer.NotEmpty()); // Ранее был pPrinter @erik v10.4.10
				if(rBlk.NumCopies > 1)
					msg_buf.CatDiv(';', 2).CatEq("Copies", rBlk.NumCopies);
				msg_buf.CatDiv(';', 2).CatEq("Mks", (profile_end - profile_start));
				PPLogMessage(PPFILNAM_REPORTING_LOG, msg_buf, LOGMSGF_USER | LOGMSGF_TIME | LOGMSGF_DBINFO);
			}
		}
	}
	if(!is_reply_sent) {
		js_reply.InsertString("status", "ok");
	}
	rReply.Z();
	CATCH
		const short crw_err_code = PEGetErrorCode(h_job);
		CrwError = crw_err_code;
		rReply.Code = crw_err_code;
		// @debug {
		if(rBlk.Action != CrystalReportPrintParamBlock::actionExport) {
			PPLoadString("err_crpe", msg_buf);
			msg_buf.CatDiv(':', 2).Cat(CrwError);
			rReply.Descr = msg_buf;
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_COMP|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		// } @debug
		js_reply.InsertString("status", "fail");
		if(crw_err_code) {
			js_reply.InsertInt("errcode", PPERR_CRYSTAL_REPORT);
			js_reply.InsertInt("crr_errcode", crw_err_code);
		}
		else 
			js_reply.InsertInt("errcode", PPErrCode);
		if(rReply.Descr.NotEmpty()) {
			(temp_buf = rReply.Descr).Transf(CTRANSF_INNER_TO_UTF8);
			js_reply.InsertString("errmsg", temp_buf.Escape());
		}
		ok = 0;
	ENDCATCH
	if(h_job)
		PEClosePrintJob(h_job);
	if(!is_reply_sent) {
		SendReportLaunchingResultToClientsPipe(hPipe, js_reply);
	}
	return ok;
}
//
//
//
int VerifyCrpt(const char * pRptPath, const char * pDataPath)
{
	int    ok = -1;
#if 0 // {
	if(pRptPath && pDataPath && sstrlen(pRptPath) && sstrlen(pDataPath)) {
		short  h_job = PEOpenPrintJob(pRptPath);
		PEReportOptions ro;
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
		ro.StructSize = sizeof(ro);
		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		// THROW(SetupReportLocations(h_job, pDataPath, 1));
		{
			BOOL ret = FALSE;
			SString buf;
			PELogOnInfo logon_info;
			MEMSZERO(logon_info);
			logon_info.StructSize = PE_SIZEOF_LOGON_INFO;
			(buf = pDataPath).SetLastSlash().Cat(BDictionary::DdfTableFileName);
			buf.CopyTo(logon_info.ServerName, PE_SERVERNAME_LEN);
			(buf = pDataPath).SetLastSlash().Cat("head.btr");
			buf.CopyTo(logon_info.DatabaseName, PE_DATABASENAME_LEN);
			ret = PESetNthTableLogOnInfo(h_job, 0, &logon_info, FALSE);
			(buf = pDataPath).SetLastSlash().Cat("iter.btr");
			buf.CopyTo(logon_info.DatabaseName, PE_DATABASENAME_LEN);
			ret = PESetNthTableLogOnInfo(h_job, 1, &logon_info, FALSE);
			ok = 1;
		}
		PEVerifyDatabase(h_job);
		if(h_job)
			PEClosePrintJob(h_job);
	}
	CATCHZOK
#endif // } 0
	return ok;
}

#if 0 // @v12.3.11 (see comments at report.h) {
int CrystalReportPrint(const char * pReportPath, const char * pDir, const char * pPrinter, int numCopies, int options, const DEVMODEA * pDevMode) // @erik v10.4.10 {
{
	// __@erik v10.4.10 {
	SString inner_printer_buf;
	const char * p_inner_printer = 0;
	if(pDevMode) {
		inner_printer_buf = reinterpret_cast<const char *>(pDevMode->dmDeviceName);
		if(inner_printer_buf.NotEmptyS())
			p_inner_printer = inner_printer_buf;
		numCopies = (pDevMode->dmCopies > 0 && pDevMode->dmCopies <= 1000) ? pDevMode->dmCopies : 1;
	}
	else {
		p_inner_printer = pPrinter;
	}
	// } __@erik v10.4.10
	int    ok = 1;
	int    zero_print_device = 0;
	short  h_job = PEOpenPrintJob(pReportPath);
	SString msg_buf;
	PEReportOptions ro;
	THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
	ro.StructSize = sizeof(ro);
	PEGetReportOptions(h_job, &ro);
	ro.morePrintEngineErrorMessages = FALSE;
	PESetReportOptions(h_job, &ro);
	if(!DS.GetConstTLA().PrintDevice.Len()) {
		if(GetWindowsPrinter(0, &DS.GetTLA().PrintDevice) > 0)
			zero_print_device = 1;
	}
	THROW(SetPrinterParam(h_job, p_inner_printer, options, pDevMode));
	THROW(SetupReportLocations(h_job, pDir, (options & SPRN_DONTRENAMEFILES) ? 0 : 1));
	if(options & SPRN_PREVIEW) {
		THROW_PP(PEOutputToWindow(h_job, "", CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, WS_MAXIMIZE | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU, 0), PPERR_CRYSTAL_REPORT);
	}
	else {
		THROW_PP(PEOutputToPrinter(h_job, numCopies), PPERR_CRYSTAL_REPORT);
	}
	if(options & SPRN_SKIPGRPS)
		SetupGroupSkipping(h_job);
	if(options & SPRN_PREVIEW) {
		CrrPreviewEventParam pep;
		pep.StopPreview = 0;
		PEEnableEventInfo eventInfo;
		eventInfo.StructSize = sizeof(PEEnableEventInfo);
		eventInfo.closePrintWindowEvent = TRUE;
		eventInfo.startStopEvent = TRUE;
		THROW_PP(PEEnableEvent(h_job, &eventInfo), PPERR_CRYSTAL_REPORT);
		THROW_PP(PESetEventCallback(h_job, CrrPreviewEventParam::EventCallback, &pep), PPERR_CRYSTAL_REPORT);
		THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
#ifndef MODELESS_REPORT_PREVIEW
		EnableWindow(APPL->H_TopOfStack, 0); // Запрещает работу в программе пока окно просмотра активно
		APPL->MsgLoop(0, pep.StopPreview);
#endif
	}
	else {
		const uint64 profile_start = SLS.GetProfileTime();
		THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
		const uint64 profile_end = SLS.GetProfileTime();
		{
			msg_buf.Z().CatEq("Report", pReportPath);
			if(!isempty(p_inner_printer)) // Ранее был pPrinter @erik v10.4.10
				msg_buf.CatDiv(';', 2).CatEq("Printer", p_inner_printer); // Ранее был pPrinter @erik v10.4.10
			if(numCopies > 1)
				msg_buf.CatDiv(';', 2).CatEq("Copies", numCopies);
			msg_buf.CatDiv(';', 2).CatEq("Mks", (profile_end - profile_start));
			PPLogMessage(PPFILNAM_REPORTING_LOG, msg_buf, LOGMSGF_USER | LOGMSGF_TIME | LOGMSGF_DBINFO);
		}
	}
	CATCH
		CrwError = PEGetErrorCode(h_job);
		// @debug {
		{
			PPLoadString("err_crpe", msg_buf);
			// @v11.8.8 msg_buf.Transf(CTRANSF_INNER_TO_OUTER);
			msg_buf.CatDiv(':', 2).Cat(CrwError);
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_COMP|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		// } @debug
		ok = 0;
	ENDCATCH
	if(h_job)
		PEClosePrintJob(h_job);
	if(zero_print_device)
		DS.GetTLA().PrintDevice.Z();
	return ok;
} // }@erik v10.4.10

int CrystalReportExport(const char * pReportPath, const char * pDir, const char * pReportName, const char * pEMailAddr, int options)
{
	int    ok = 1;
	bool   silent = false;
	bool   do_export = true;
	const char * p__dest_fn = 0;
	SString path;
	SString temp_buf;
	PEExportOptions eo;
	PEReportOptions ro;
	short  h_job = PEOpenPrintJob(pReportPath);
	THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
	ro.StructSize = sizeof(ro);
	PEGetReportOptions(h_job, &ro);
	ro.morePrintEngineErrorMessages = FALSE;
	PESetReportOptions(h_job, &ro);
	eo.StructSize = sizeof(eo);
	eo.destinationOptions = 0;
	eo.formatOptions      = 0;
	THROW(LoadExportOptions(pReportName, &eo, &silent, path));
	// @v11.3.2 {
	if(silent) {
		const char * p_dest_fn_2 = *(const char **)(PTR8C(eo.destinationOptions)+2);
		p__dest_fn = p_dest_fn_2;
	}
	else if(PEGetExportOptions(h_job, &eo)) {
		const char * p_dest_fn_1 = reinterpret_cast<const char *>(PTR8C(eo.destinationOptions)+2);
		p__dest_fn = p_dest_fn_1;
	}
	else
		do_export = false;
	// } @v11.3.2 
	if(do_export) {
		PEExportTo(h_job, &eo);
		THROW(SetupReportLocations(h_job, pDir, 0));
		if(options & SPRN_SKIPGRPS)
			SetupGroupSkipping(h_job);
		THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
		if(!isempty(pEMailAddr)) {
			if(fileExists(p__dest_fn)) {
				//
				// Отправка на определенный почтовый адрес
				//
				PPAlbatrossConfig alb_cfg;
				if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0) {
					if(alb_cfg.Hdr.MailAccID) {
						if(SendMailWithAttachment(pReportName, /*path*/p__dest_fn, pReportName, pEMailAddr, alb_cfg.Hdr.MailAccID)) {
							// Отправка отчета на электронную почту: success 
							PPLoadString("reportsendmail", temp_buf);
							temp_buf.CatDiv(':', 2).Cat("success").Space().Cat(pReportName).Space().Cat(pEMailAddr);
							PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
						}
						else {
							// PPERR_REPORT_SENDMAIL_IMPL Ошибка отправки отчета на электронную почту"
							PPGetLastErrorMessage(1, temp_buf); // last error text
							SString msg_buf;
							PPGetMessage(mfError, PPERR_REPORT_SENDMAIL_IMPL, 0, 1, msg_buf);
							msg_buf.CatDiv(':', 2).Cat(temp_buf);
							PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
						}
					}
					else {
						// PPERR_REPORT_SENDMAIL_NOACC Отправка отчета на электронную почту: конфигурация глобального обмена не содержит ссылку на почтовую учетную запись
						PPSetError(PPERR_REPORT_SENDMAIL_NOACC);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					}
				}
				else {
					// PPERR_REPORT_SENDMAIL_NOCFG Отправка отчета на электронную почту: не удалось получить конфигурацию глобального обмена
					PPSetError(PPERR_REPORT_SENDMAIL_NOCFG);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				}
			}
			else {
				// PPERR_REPORT_SENDMAIL_NOFILE Отправка отчета на электронную почту: результирующий файл '%s' не найден
				PPSetError(PPERR_REPORT_SENDMAIL_NOFILE, p__dest_fn);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			}
		}
	}
	CATCH
		CrwError = PEGetErrorCode(h_job);
		ok = 0;
	ENDCATCH
	if(h_job)
		PEClosePrintJob(h_job);
	return ok;
}
#endif // } @v12.3.11 (see comments at report.h)