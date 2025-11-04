// PPREPORT.CPP
// Copyright (C) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
// @v12.4.1 #include <crpe.h>
// @v12.4.1 #include <crpe2.h>
//
// Закомментировать, если немодальный предварительный просмотр печати будет сбоить
//
//#define MODELESS_REPORT_PREVIEW
//
//
//
bool FindExeByExt2(const char * pExt, SString & rResult, const char * pAddedSearchString)
{
	rResult.Z();
	bool   ok = false;
	if(pExt) {
		SString temp_buf;
		SString val_buf;
		WinRegKey reg_key(HKEY_CLASSES_ROOT, pExt, 1);
		if(reg_key.GetString(0, val_buf) > 0) {
			if(pAddedSearchString && val_buf.IsEqNC(pAddedSearchString)) {
				WinRegKey reg_key_app(HKEY_CLASSES_ROOT, val_buf, 1);
				if(reg_key_app.GetString(0, temp_buf) > 0) {
					temp_buf.ReplaceStr(" ", 0, 0);
					val_buf = temp_buf;
				}
			}
			val_buf.SetLastSlash().Cat("shell").SetLastSlash().Cat("open").SetLastSlash().Cat("command");
			WinRegKey reg_key_exe(HKEY_CLASSES_ROOT, val_buf, 1);
			if(reg_key_exe.GetString(0, val_buf) > 0) {
				val_buf.Strip();
				if(val_buf.C(0) == '"') {
					val_buf.ShiftLeft();
					size_t s_pos = 0;
					if(val_buf.SearchChar('"', &s_pos))
						val_buf.Excise(s_pos, 1);
					s_pos = 0;
					if(val_buf.SearchChar('.', &s_pos)) {
						size_t space_pos = 0;
						if(val_buf.SearchCharPos(s_pos+1, ' ', &space_pos))
							val_buf.Trim(space_pos);
					}
					rResult = val_buf;
					ok = true;
				}
			}
		}
	}
	return ok;
}
//
//
//
PPReportEnv::PPReportEnv(long prnFlags, long sort) : PrnFlags(prnFlags), Sort(sort)
{
}
//
//
//
PrnDlgAns::PrnDlgAns(const char * pReportName) : Dest(0), Selection(0), NumCopies(1), Flags(0), ReportName(pReportName), P_DevMode(0) // @erik
{
}

PrnDlgAns::PrnDlgAns(const PrnDlgAns & rS) : Dest(0), Selection(0), NumCopies(1), Flags(0), ReportName(rS.ReportName), DefPrnForm(rS.DefPrnForm), P_DevMode(0)
{
	Copy(rS);
}

PrnDlgAns::~PrnDlgAns()
{
	ZDELETE(P_DevMode);
}

PrnDlgAns & FASTCALL PrnDlgAns::operator = (const PrnDlgAns & rS)
{
	return Copy(rS);
}

PrnDlgAns & FASTCALL PrnDlgAns::Copy(const PrnDlgAns & rS)
{
	Dest = rS.Dest;
	Selection = rS.Selection;
	NumCopies = rS.NumCopies;
	Flags = rS.Flags;
	ReportName = rS.ReportName;
	DefPrnForm = rS.DefPrnForm;
	PrepareDataPath = rS.PrepareDataPath;
	Printer = rS.Printer;
	EmailAddr = rS.EmailAddr;
	ContextSymb = rS.ContextSymb;
	ExpParam = rS.ExpParam; // @v12.3.10
	TSCollection_Copy(Entries, rS.Entries);
	ZDELETE(P_DevMode);
	if(rS.P_DevMode) {
		P_DevMode = new DEVMODEA;
		*P_DevMode = *rS.P_DevMode;
	}
	return *this;
}
//
//
//
int GetReportIDByName(const char * pRptName, uint * pRptID)
{
	int    ok = 0;
	ulong  ofs = 0;
	SString name_buf;
	TVRez::WResHeaderInfo hdr;
	ASSIGN_PTR(pRptID, 0);
	if(P_SlRez) {
		for(ofs = 0; !ok && P_SlRez->readHeader(ofs, &hdr, TVRez::beginOfData); ofs = hdr.Next)
			if(hdr.Type == PP_RCDECLREPORTSTUB && P_SlRez->getString(name_buf, 0).CmpNC(pRptName) == 0) {
				ASSIGN_PTR(pRptID, hdr.IntID);
				ok = 1;
			}
	}
	return ok;
}
//
//
//
struct PPReportStub {
	PPReportStub() : ID(0)
	{
	}
	long   ID;
	SString Name;
	SString DataName;
	SString Descr;
};

int PPLoadReportStub(long rptID, PPReportStub * pData)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_REZNOTINITED);
	THROW_PP(p_rez->findResource(rptID, PP_RCDECLREPORTSTUB), PPERR_RESFAULT);
	pData->ID = rptID;
	pData->Name = p_rez->getString(pData->Name, 0);
	pData->DataName = p_rez->getString(pData->DataName, 0);
	pData->Descr = p_rez->getString(pData->Descr, 0);
	CATCHZOK
	return ok;
}

/*static*/bool SReport::GetReportAttributes(uint reportId, SString * pReportName, SString * pDataName)
{
	bool    ok = false;
	SString report_name;
	SString data_name;
	if(reportId > 1000) {
		PPReportStub stub;
		if(PPLoadReportStub(reportId, &stub)) {
			report_name = stub.Name;
			data_name = stub.DataName;
			ok = true;
		}
	}
	ASSIGN_PTR(pReportName, report_name);
	ASSIGN_PTR(pDataName, data_name);
	return ok;
}

SReport::SReport(uint rezID, long flags) : Error(0), NumCopies(1)
{
	if(rezID > 1000) {
		PPReportStub stub;
		if(PPLoadReportStub(rezID, &stub)) {
			Name = stub.Name;
			DataName = stub.DataName;
		}
		else
			Error = 1;
	}
}

SReport::~SReport()
{
}

bool SReport::IsValid() const { return !Error; }
//
//
//
int CrystalReportExportParam::LoadFromIniFile(const char * pReportName)
{
	Z();
	int    ok = -1;
	SString temp_buf;
	SString fname;
	PPGetFilePath(PPPATH_BIN, PPFILNAM_REPORT_INI, fname);
	if(fileExists(fname)) {
		int   r = 0;
		int   int_val = 0;
		const char * p_ext = 0;
		SString param_symb;
		SString left_buf, right_buf;
		PPIniFile ini_file(fname);
		PPIniFile::GetParamSymb(PPINIPARAM_REPORT_SILENT, param_symb);
		if(ini_file.GetIntParam(pReportName, param_symb, &int_val) > 0) {
			SETFLAG(Flags, fSilent, int_val == 1);
		}
		else if(ini_file.GetIntParam("default", param_symb, &int_val) > 0) {
			SETFLAG(Flags, fSilent, int_val == 1);
		}
		//
		{
			bool   format_defined = false;
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_FORMAT, param_symb);
			if(ini_file.GetParam(pReportName, param_symb, temp_buf) > 0) {
				format_defined = true;
			}
			else if(ini_file.GetParam("default", param_symb, temp_buf) > 0) {
				format_defined = true;
			}
			if(format_defined) {
				StringSet ss(',', temp_buf);
				uint    ssp = 0;
				if(ss.get(&ssp, temp_buf)) {
					uint internal_param_no = 0;
					if(temp_buf.IsEqiAscii("pdf")) {
						Format = crexpfmtPdf;
						p_ext = "pdf";
						// no-params 
						/*while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							//
						}*/
					}
					else if(temp_buf.IsEqiAscii("doc") || temp_buf.IsEqiAscii("wordwin") || temp_buf.IsEqiAscii("winword") || temp_buf.IsEqiAscii("word")) {
						Format = crexpfmtWinWord;
						p_ext = "doc";
						// no-params 
						/*while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							//
						}*/
					}
					else if(temp_buf.IsEqiAscii("rtf") || temp_buf.IsEqiAscii("RichTextFormat")) {
						Format = crexpfmtRtf;
						p_ext = "rtf";
						// no-params 
						/*while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							//
						}*/
					}
					else if(temp_buf.IsEqiAscii("xls")) {
						Format = crexpfmtExcel;
						p_ext = "xls";
						while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							if(temp_buf.Divide('=', left_buf, right_buf) > 0) {
								right_buf.Strip().ToLower();
								if(left_buf.IsEqiAscii("constcolumnwidth")) {
									XlsConstColumnWidth = right_buf.ToLong();
								}
								else if(left_buf.IsEqiAscii("columnheadings")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fXlsColumnHeadings;
								}
								else if(left_buf.IsEqiAscii("tabularformat")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fXlsUseConstColumnWidth;
								}
								else if(left_buf.IsEqiAscii("pagebreak")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fXlsCreatePgBrkForEachPage;
								}
								else if(left_buf.IsEqiAscii("datetostring")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fXlsCvtDateToString;
								}
								else if(left_buf.IsEqiAscii("showgrid")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fXlsShowGrid;
								}
							}
							else if(left_buf.NotEmptyS()) {
								if(left_buf.IsEqiAscii("columnheadings")) {
									Flags |= fXlsColumnHeadings;
								}
								else if(left_buf.IsEqiAscii("tabularformat")) {
									Flags |= fXlsUseConstColumnWidth;
								}
								else if(left_buf.IsEqiAscii("pagebreak")) {
									Flags |= fXlsCreatePgBrkForEachPage;
								}
								else if(left_buf.IsEqiAscii("datetostring")) {
									Flags |= fXlsCvtDateToString;
								}
								else if(left_buf.IsEqiAscii("showgrid")) {
									Flags |= fXlsShowGrid;
								}
							}
							//
							/*
								fXlsColumnHeadings         = 0x0008,
								fXlsUseConstColumnWidth    = 0x0010,
								fXlsTabularFormat          = 0x0020,
								fXlsCreatePgBrkForEachPage = 0x0040,
								fXlsCvtDateToString        = 0x0080,
								fXlsShowGrid               = 0x0100,
							*/ 
						}
					}
					else if(temp_buf.IsEqiAscii("csv") || temp_buf.IsEqiAscii("CommaSeparated")) {
						Format = crexpfmtCsv;
						CsvFieldSeparator = ';';
						p_ext = "csv";
						while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							if(temp_buf.Divide('=', left_buf, right_buf) > 0) {
								right_buf.Strip().ToLower();
								if(left_buf.IsEqiAscii("div")) {
									if(oneof5(right_buf, ",", ";", "|", "\t", " ")) {
										CsvFieldSeparator = right_buf.C(0);
									}
								}
								else if(left_buf.IsEqiAscii("userepnumformat")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvUseReportNumberFormat;
								}
								else if(left_buf.IsEqiAscii("userepdateformat")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvUseReportDateFormat;
								}
								else if(left_buf.IsEqiAscii("quotetext")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvQuoteText;
								}
							}
							else if(left_buf.NotEmptyS()) {
								if(left_buf.IsEqiAscii("userepnumformat"))
									Flags |= fCsvUseReportNumberFormat;
								else if(left_buf.IsEqiAscii("userepdateformat"))
									Flags |= fCsvUseReportDateFormat;
								else if(left_buf.IsEqiAscii("quotetext"))
									Flags |= fCsvQuoteText;
							}
						}
					}
					else if(temp_buf.IsEqiAscii("tsv") || temp_buf.IsEqiAscii("TabSeparated")) {
						Format = crexpfmtCsv;
						CsvFieldSeparator = '\t';
						p_ext = "tsv";
						while(ss.get(&ssp, temp_buf)) {
							internal_param_no++;
							if(temp_buf.Divide('=', left_buf, right_buf) > 0) {
								right_buf.Strip().ToLower();
								if(left_buf.IsEqiAscii("div")) {
									if(oneof5(right_buf, ",", ";", "|", "\t", " ")) {
										CsvFieldSeparator = right_buf.C(0);
									}
								}
								else if(left_buf.IsEqiAscii("userepnumformat")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvUseReportNumberFormat;
								}
								else if(left_buf.IsEqiAscii("userepdateformat")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvUseReportDateFormat;
								}
								else if(left_buf.IsEqiAscii("quotetext")) {
									if(oneof3(right_buf, "yes", "1", "true"))
										Flags |= fCsvQuoteText;
								}
							}
							else if(left_buf.NotEmptyS()) {
								if(left_buf.IsEqiAscii("userepnumformat"))
									Flags |= fCsvUseReportNumberFormat;
								else if(left_buf.IsEqiAscii("userepdateformat"))
									Flags |= fCsvUseReportDateFormat;
								else if(left_buf.IsEqiAscii("quotetext"))
									Flags |= fCsvQuoteText;
							}
						}
					}
					else
						format_defined = false;
				}
			}
		}
		if(Format) {
			// Destination=Disk,d:/temp/zajavka$$$.pdf
			{
				bool   destination_defined = false;
				PPIniFile::GetParamSymb(PPINIPARAM_REPORT_DESTINATION, param_symb);
				if(ini_file.GetParam(pReportName, param_symb, temp_buf) > 0) {
					destination_defined = true;
				}
				else if(ini_file.GetParam("default", param_symb, temp_buf) > 0) {
					destination_defined = true;
				}
				if(destination_defined) {
					StringSet ss(',', temp_buf);
					uint ssp = 0;
					if(ss.get(&ssp, temp_buf)) { // тип назначения (файл или приложение)
						//PPTXT_EXPORT_DESTTYPES                /!/ "Application;Disk;MAPI"
						if(temp_buf.IsEqiAscii("application") || temp_buf.IsEqiAscii("app")) {
							Destination = destApp;
						}
						else if(temp_buf.IsEqiAscii("disk") || temp_buf.IsEqiAscii("file")) {
							Destination = destFile;
						}
						else {
							Destination = destFile;
						}
						if(ss.get(&ssp, temp_buf)) { // имя файла назначения //
							SString dir;
							if(temp_buf.NotEmptyS()) {
								if(SFile::IsDir(temp_buf.RmvLastSlash())) {
									MakeTempFileName(dir = temp_buf, "exp", p_ext, 0, temp_buf);
								}
								else {
									SFsPath::ReplaceExt(temp_buf, p_ext, 1);
								}
							}
							else {
								PPGetPath(PPPATH_OUT, dir);
								MakeTempFileName(dir, "exp", p_ext, 0, temp_buf);
							}
							DestFileName = temp_buf;
						}
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}

CrystalReportExportParam::Crr_Destination::Crr_Destination() : Size(sizeof(*this)), FileNamePtr(0)
{
}
		
void CrystalReportExportParam::Crr_Destination::SetFileName(const char * pFileName)
{
	assert(sizeof(void *) == 4);
	if(sizeof(void *) == 4) {
		FileNamePtr = reinterpret_cast<uint32>(newStr(pFileName));
	}
	else {
		FileNamePtr = 0;
	}
}

CrystalReportExportParam::Crr_ExportOptions_PdfRtfDoc::Crr_ExportOptions_PdfRtfDoc() : Size(sizeof(*this)), SelectedPages(0), FirstPage(0), LastPage(0)
{
	memzero(Unknown, sizeof(Unknown));
}

CrystalReportExportParam::Crr_ExportOptions_Xls::Crr_ExportOptions_Xls()
{
	THISZERO();
	Size = sizeof(*this);
}

CrystalReportExportParam::Crr_ExportOptions_Csv::Crr_ExportOptions_Csv()
{
	THISZERO();
	Size = sizeof(*this);
	FieldSep = ';';
	QuoteChar = '\"';
}

/*static*/bool CrystalReportExportParam::GetDestDll(uint dest, SString & rDllModuleName)
{
	rDllModuleName.Z();
	switch(dest) {
		case destApp: rDllModuleName = "u2dapp"; break;
		case destFile: rDllModuleName = "u2ddisk"; break;
	}
	return rDllModuleName.NotEmpty();
}

static constexpr uint32 CrystalReportExportParam_Version = 0;

CrystalReportExportParam::CrystalReportExportParam() : Ver(CrystalReportExportParam_Version), 
	Flags(0), Format(0), Destination(0), XlsConstColumnWidth(0), XlsBaseAreaType(0), XlsBaseAreaGroupNum(0), CsvFieldSeparator(0)
{
}

CrystalReportExportParam::CrystalReportExportParam(const CrystalReportExportParam & rS) : Ver(CrystalReportExportParam_Version), 
	Flags(rS.Flags), Format(rS.Format), Destination(rS.Destination), XlsConstColumnWidth(rS.XlsConstColumnWidth), XlsBaseAreaType(rS.XlsBaseAreaType), 
	XlsBaseAreaGroupNum(rS.XlsBaseAreaGroupNum), CsvFieldSeparator(rS.CsvFieldSeparator)
{
}

CrystalReportExportParam & FASTCALL CrystalReportExportParam::operator = (const CrystalReportExportParam & rS)
{
	Copy(rS);
	return *this;
}
	
CrystalReportExportParam & FASTCALL CrystalReportExportParam::Z()
{
	Flags = 0;
	Format = 0;
	Destination = 0;
	XlsConstColumnWidth = 0;
	XlsBaseAreaType = 0;
	XlsBaseAreaGroupNum = 0;
	CsvFieldSeparator = 0;
	PageRange.Z();
	DestFileName.Z();
	return *this;
}
	
bool FASTCALL CrystalReportExportParam::Copy(const CrystalReportExportParam & rS)
{
	#define I(f) f = rS.f
	I(Flags);
	I(Format);
	I(Destination);
	I(XlsConstColumnWidth);
	I(XlsBaseAreaType);
	I(XlsBaseAreaGroupNum);
	I(CsvFieldSeparator);
	I(PageRange);
	I(DestFileName);
	#undef I
	return true;
}

int CrystalReportExportParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Ver, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Format, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Destination, rBuf));
	THROW_SL(pSCtx->Serialize(dir, XlsConstColumnWidth, rBuf));
	THROW_SL(pSCtx->Serialize(dir, XlsBaseAreaType, rBuf));
	THROW_SL(pSCtx->Serialize(dir, XlsBaseAreaGroupNum, rBuf));
	THROW_SL(pSCtx->Serialize(dir, CsvFieldSeparator, rBuf));
	THROW_SL(PageRange.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, DestFileName, rBuf));
	CATCHZOK
	return ok;
}

bool CrystalReportExportParam::GetDefaultExportFilePath(const char * pReportName, SString & rBuf) const
{
	rBuf.Z();
	bool    ok = false;
	if(Format && !isempty(pReportName)) {
		SString path;
		SString prefix;
		SString ext;
		switch(Format) {
			case crexpfmtPdf: ext = "pdf"; break;
			case crexpfmtRtf: ext = "rtf"; break;
			case crexpfmtHtml: ext = "html"; break;
			case crexpfmtExcel: ext = "xls"; break;
			case crexpfmtWinWord: ext = "doc"; break;
			case crexpfmtCsv: ext = "csv"; break;
		}
		(prefix = "crrexp").CatChar('-').Cat(pReportName);
		PPGetPath(PPPATH_OUT, path);
		long   seq = 1;
		MakeTempFileName(path, prefix, ext, &seq, rBuf);
		if(rBuf.NotEmpty())
			ok = true;
	}
	return ok;
}

ReportDescrEntry::ReportDescrEntry() : Flags(0)
{
}

/*static*/int FASTCALL ReportDescrEntry::GetIniToken(const char * pLine, SString * pFileName)
{
	int   tok = tUnkn;
	SString buf(pLine);
	buf.Strip().ToLower();
	const char c_first = buf.C(0);
	const char c_second = buf.C(1);
	if(c_first == ';' || (c_first == '/' && c_second == '/'))
		tok = tComment;
	else if(buf == "data")
		tok = tData;
	else if(buf == "descr")
		tok = tDescr;
	else if(buf == "diffidbyscope")
		tok = tDiffIdByScope;
	else if(buf == "modifdate")
		tok = tModifDate;
	else if(buf == "std")
		tok = tStd;
	else if(buf.HasPrefix("format"))
		tok = tFormat;
	else if(buf.HasPrefix("destination"))
		tok = tDestination;
	else if(buf.HasPrefix("silent"))
		tok = tSilent;
	else {
		SString file_name;
		// @v11.9.2 {
		SString temp_buf;
		SFsPath ps(buf);
		ps.Merge(temp_buf);
		if(ps.Drv.NotEmpty()) {
			temp_buf.RmvLastSlash();
			DS.ConvertPathToUnc(temp_buf);
		}
		else {
			PPGetFilePath(PPPATH_BIN, buf, temp_buf);
		}
		file_name = temp_buf;
		// } @v11.9.2 
		/* @v11.9.2 if(c_second != ':')
			PPGetFilePath(PPPATH_BIN, buf, file_name);
		else
			file_name = buf;*/
		if(fileExists(file_name)) {
			ASSIGN_PTR(pFileName, file_name);
			tok = tExistFile;
		}
		else
			tok = tUnkn;
	}
	return tok;
}

int ReportDescrEntry::SetReportFileName(const char * pFileName)
{
	int    ok = -1;
	SString file_name(pFileName);
	SVerT  crr_ver = QueryCrrVersion();
	if(crr_ver.GetMajor() >= 10) {
		SFsPath ps(file_name);
		if(ps.Nam.CmpSuffix("-c10", 1) != 0) {
			ps.Nam.Cat("-c10");
			SString temp_buf;
			ps.Merge(temp_buf);
			if(fileExists(temp_buf)) {
				file_name = temp_buf;
				ok = 1;
			}
		}
	}
	{
		SFsPath ps(file_name);
		SETFLAG(Flags, fTddoResource, ps.Ext.ToLower() == "tddo");
	}
	ReportPath_ = file_name;
	return ok;
}

int PrnDlgAns::SetupReportEntries(const char * pContextSymb)
{
	Entries.freeAll();
	(ContextSymb = pContextSymb).Strip();
	int    ok = 1;
	uint   pos = 0;
	int    diffidbyscope = 0;
	int    force_ddf = 0;
	//const uint16 cr_dll_ver = PEGetVersion(PE_GV_DLL);
	SString temp_buf, fname, buf2;
	SString left; // Временные переменные для деления буферов по символу
	if(ReportName.IsEmpty())
		ok = -1;
	else {
		SString param_buf;
		PPIniFile ifile;
		ifile.Get(PPINISECT_PATH, PPINIPARAM_WINLOCALRPT, buf2);
		if(buf2.IsEmpty())
			(buf2 = "RPT").SetLastSlash().Cat("LOCAL");
		ifile.GetInt(PPINISECT_CONFIG, PPINIPARAM_REPORT_FORCE_DDF, &force_ddf);
		SFsPath::ReplaceExt((fname = ReportName), "RPT", 1);
		SFsPath::ReplacePath(fname, buf2, 1);
		PPGetFilePath(PPPATH_BIN, fname, temp_buf);
		if(!fileExists(temp_buf)) {
			ifile.Get(PPINISECT_PATH, PPINIPARAM_WINDEFAULTRPT, buf2 = 0);
			buf2.SetIfEmpty("RPT");
			fname = ReportName;
			SFsPath::ReplaceExt(fname, "RPT", 1);
			SFsPath::ReplacePath(fname, buf2, 1);
			PPGetFilePath(PPPATH_BIN, fname, temp_buf);
		}
		{
			ReportDescrEntry * p_new_entry = new ReportDescrEntry;
			THROW_MEM(p_new_entry);
			p_new_entry->SetReportFileName(temp_buf);
			PPLoadString("standard", p_new_entry->Description_);
			p_new_entry->Description_.Transf(CTRANSF_INNER_TO_OUTER);
			THROW_SL(Entries.insert(p_new_entry));
		}
		PPGetFilePath(PPPATH_BIN, PPFILNAM_STDRPT_INI, fname);
		if(fileExists(fname)) {
			StringSet std_ss;
			SString section_name;
			PPIniFile std_ini_file(fname);
			THROW(std_ini_file.IsValid());
			{
				if(ContextSymb.NotEmpty()) {
					(section_name = ReportName).Strip().Colon().Cat(ContextSymb);
					std_ini_file.GetEntries2(section_name, &std_ss, 0/*flags*/);
				}
				if(std_ss.getCount() == 0) {
					(section_name = ReportName).Strip();
					std_ini_file.GetEntries2(section_name, &std_ss, 0/*flags*/);
				}
			}
			for(pos = 0; std_ss.get(&pos, param_buf);) {
				const int tok = ReportDescrEntry::GetIniToken(param_buf, &fname);
				switch(tok) {
					case ReportDescrEntry::tData:
						std_ini_file.GetParam(section_name, param_buf, Entries.at(0)->DataName_);
						Entries.at(0)->DataName_.Strip();
						Entries.at(0)->DataName_.Transf(CTRANSF_INNER_TO_OUTER);
						break;
					case ReportDescrEntry::tDescr:
						std_ini_file.GetParam(section_name, param_buf, Entries.at(0)->Description_);
						Entries.at(0)->Description_.Strip();
						Entries.at(0)->Description_.Transf(CTRANSF_INNER_TO_OUTER);
						break;
					case ReportDescrEntry::tDiffIdByScope:
						{
							int    val = 0;
							std_ini_file.GetIntParam(section_name, param_buf, &val);
							if(val)
								diffidbyscope = 1;
						}
						break;
					case ReportDescrEntry::tModifDate:
						break;
					case ReportDescrEntry::tComment:
						break;
					case ReportDescrEntry::tExistFile:
						{
							ReportDescrEntry * p_new_entry = new ReportDescrEntry;
							THROW_MEM(p_new_entry);
							p_new_entry->SetReportFileName(fname);
							std_ini_file.GetParam(section_name, param_buf, temp_buf);
							temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
							StringSet tok_list(';', temp_buf);
							uint   tok_p = 0;
							if(tok_list.get(&tok_p, left)) {
								p_new_entry->Description_ = left.Strip();
								if(tok_list.get(&tok_p, left)) {
									p_new_entry->DataName_ = (left.NotEmptyS() ? left : Entries.at(0)->DataName_);
									if(tok_list.get(&tok_p, left))
										p_new_entry->OutputFormat = left.Strip();
								}
								else
									p_new_entry->DataName_ = Entries.at(0)->DataName_;
							}
							THROW_SL(Entries.insert(p_new_entry));
							p_new_entry = 0;
						}
						break;
				}
			}
		}
		PPGetFilePath(PPPATH_BIN, PPFILNAM_REPORT_INI, fname);
		if(fileExists(fname)) {
			SString format_p, dest_p, silent_p;
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_FORMAT, format_p);
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_DESTINATION, dest_p);
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_SILENT, silent_p);
			StringSet ss;
			SString section_name;
			PPIniFile ini_file(fname);
			THROW(ini_file.IsValid());
			{
				if(ContextSymb.NotEmpty()) {
					(section_name = ReportName).Strip().Colon().Cat(ContextSymb);
					ini_file.GetEntries2(section_name, &ss, 0/*flags*/);
				}
				if(!ss.IsCountGreaterThan(0)) {
					(section_name = ReportName).Strip();
					ini_file.GetEntries2(section_name, &ss, 0/*flags*/);
				}
			}
			for(pos = 0; ss.get(&pos, param_buf);) {
				const int tok = ReportDescrEntry::GetIniToken(param_buf.Strip(), &fname);
				switch(tok) {
					case ReportDescrEntry::tStd:
						ini_file.GetParam(section_name, param_buf, fname);
						fname.Divide(';', left, temp_buf);
						fname = left.Strip();
						if(fname[0] && fname[1] != ':')
							PPGetFilePath(PPPATH_BIN, left, fname);
						if(fname[0] == 0 || fileExists(fname)) { // @todo unc-names
							if(fname[0])
								Entries.at(0)->SetReportFileName(fname);
							if(temp_buf[0])
								Entries.at(0)->DataName_ = temp_buf;
						}
						break;
					case ReportDescrEntry::tData:
					case ReportDescrEntry::tFormat:
					case ReportDescrEntry::tDestination:
					case ReportDescrEntry::tSilent:
						break;
					case ReportDescrEntry::tExistFile:
						{
							ReportDescrEntry * p_new_entry = new ReportDescrEntry;
							THROW_MEM(p_new_entry);
							p_new_entry->SetReportFileName(fname);
							ini_file.GetParam(section_name, param_buf, temp_buf);
							temp_buf.Strip();
							temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
							int    is_equal = 0;
							StringSet tok_list(';', temp_buf);
							uint   tok_p = 0;
							if(tok_list.get(&tok_p, left)) {
								temp_buf = left.Strip();
								for(uint j = 0; !is_equal && j < Entries.getCount(); j++)
									if(temp_buf.CmpNC(Entries.at(j)->Description_) == 0)
										is_equal = 1;
								if(!is_equal) {
									p_new_entry->Description_ = temp_buf;
									if(tok_list.get(&tok_p, left)) {
										p_new_entry->DataName_ = (left.NotEmptyS() ? left : Entries.at(0)->DataName_);
										if(tok_list.get(&tok_p, left))
											p_new_entry->OutputFormat = left.Strip();
									}
									else
										p_new_entry->DataName_ = Entries.at(0)->DataName_;
									p_new_entry->Flags |= ReportDescrEntry::fInheritedTblNames;
									THROW_SL(Entries.insert(p_new_entry));
									p_new_entry = 0;
								}
							}
							delete p_new_entry; // Если p_new_entry была добавлена в Entries, то 0 (см. выше)
						}
						break;
				}
			}
		}
		SForEachVectorItem(Entries, j) { SETFLAG(Entries.at(j)->Flags, ReportDescrEntry::fDiff_ID_ByScope, diffidbyscope); }
	}
	CATCHZOK
	SETFLAG(Flags, fForceDDF, force_ddf);
	return ok;
}

class Print2Dialog : public TDialog {
	DECL_DIALOG_DATA(PrnDlgAns);
public:
	Print2Dialog() : TDialog(DLG_PRINT2), EnableEMail(0), Data(0)
	{
		PPGetPrinterCfg(0, 0, &PrnCfg);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		SString cr_path_;
		StrAssocArray list;
		RVALUEPTR(Data, pData);
		SETIFZ(Data.Dest, 1);
		Data.Selection = 0;
		{
			// @v12.4.1 bool   silent = false;
			// @v12.4.1 LoadExportOptions(Data.ReportName, 0, &silent, temp_buf.Z());
			EnableEMail = 1;
		}
		AddClusterAssoc(CTL_PRINT2_ACTION, 0, PrnDlgAns::aPrint);
		AddClusterAssoc(CTL_PRINT2_ACTION, 1, PrnDlgAns::aExport);
		AddClusterAssoc(CTL_PRINT2_ACTION, 2, PrnDlgAns::aPreview);
		AddClusterAssoc(CTL_PRINT2_ACTION, 3, PrnDlgAns::aExportXML);
		AddClusterAssoc(CTL_PRINT2_ACTION, 4, PrnDlgAns::aPrepareData);
		AddClusterAssoc(CTL_PRINT2_ACTION, 5, PrnDlgAns::aPrepareDataAndExecCR);
		SetClusterData(CTL_PRINT2_ACTION, Data.Dest);
		const bool is_there_cr = FindExeByExt2(".rpt", cr_path_, "CrystalReports.9.1");
		if(!is_there_cr)
			DisableClusterItem(CTL_PRINT2_ACTION, 5, 1);
		THROW(Data.SetupReportEntries(0));
		if(Data.DefPrnForm.NotEmpty()) {
			//
			// Если задано имя формы по умолчанию и в списке форм есть файл с именем, совпадающем 
			// с именем формы по умолчанию, то перемещаем эту форму на самый верх списка. 
			// 
			SFsPath ps;
			for(uint i = 0; i < Data.Entries.getCount(); i++) {
				ps.Split(Data.Entries.at(i)->ReportPath_);
				if(ps.Nam.IsEqNC(Data.DefPrnForm)) {
					for(int j = i; j > 0; j--)
						Data.Entries.swap(j, j-1);
					break;
				}
			}
		}
		SForEachVectorItem(Data.Entries, i) { list.Add(i+1, temp_buf.Z().Cat(Data.Entries.at(i)->Description_).Transf(CTRANSF_OUTER_TO_INNER)); }
		SetupStrAssocCombo(this, CTLSEL_PRINT2_REPORT, list, 1, 0);
		{
			SString last_selected_printer;
			SPrinting::GetListOfPrinters(&PrnList);
			list.Z();
			long   sel_prn_id = 0;
			if(PrnCfg.Flags & PrnCfg.fStoreLastSelPrn) {
				WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 0);
				reg_key.GetString(PPConst::WrParam_LastSelectedPrinter, last_selected_printer);
			}
			//
			// Перемещаем принтер по умолчанию на верх списка
			//
			long   def_prn_id = 0;
			if(last_selected_printer.NotEmpty()) {
				for(uint j = 0; j < PrnList.getCount(); j++) {
					if(last_selected_printer.IsEqNC(PrnList.at(j).PrinterName)) {
						def_prn_id = j+1;
						sel_prn_id = j+1;
						break;
					}
				}
			}
			if(!def_prn_id) {
				for(uint j = 0; j < PrnList.getCount(); j++) {
					if(PrnList.at(j).Flags & SPrinting::PrnInfo::fDefault) {
						def_prn_id = j+1;
						break;
					}
				}
			}
			if(def_prn_id > 1)
				PrnList.swap(0, static_cast<uint>(def_prn_id-1));
			//
			SForEachVectorItem(PrnList, j) { list.Add(j+1, temp_buf.Z().Cat(PrnList.at(j).PrinterName).Transf(CTRANSF_OUTER_TO_INNER)); }
			SetupStrAssocCombo(this, CTLSEL_PRINT2_PRINTER, list, sel_prn_id, 0);
		}
		setCtrlLong(CTL_PRINT2_NUMCOPIES, Data.NumCopies);
		{
			SETFLAG(Data.Flags, PrnDlgAns::fUseDuplexPrinting, BIN(PrnCfg.Flags & PPPrinterCfg::fUseDuplexPrinting));
			AddClusterAssoc(CTL_PRINT2_DUPLEX, 0, PrnDlgAns::fUseDuplexPrinting);
			SetClusterData(CTL_PRINT2_DUPLEX, Data.Flags);
		}
		SetupReportEntry();
		CATCHZOK
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetClusterData(CTL_PRINT2_ACTION, &Data.Dest);
		Data.Selection = getCtrlLong(CTLSEL_PRINT2_REPORT)-1;
		Data.NumCopies = getCtrlLong(CTL_PRINT2_NUMCOPIES);
		Data.Flags &= ~PrnDlgAns::fEMail;
		if(oneof2(Data.Dest, PrnDlgAns::aExport, PrnDlgAns::aExportXML) && EnableEMail) {
			if(getCtrlUInt16(CTL_PRINT2_DOMAIL)) {
				Data.Flags |= PrnDlgAns::fEMail;
				getCtrlString(CTL_PRINT2_MAKEDATAPATH, Data.EmailAddr);
			}
		}
		else
			getCtrlString(CTL_PRINT2_MAKEDATAPATH, Data.PrepareDataPath);
		// @v12.3.10 {
		if(Data.Dest == PrnDlgAns::aExport) {
			Data.ExpParam.Format = static_cast<uint>(getCtrlLong(CTLSEL_PRINT2_EXPFMT));
		}
		// } @v12.3.10 
		GetClusterData(CTL_PRINT2_DUPLEX, &Data.Flags);
		long   sel_id = getCtrlLong(CTLSEL_PRINT2_PRINTER);
		Data.Printer = (sel_id && sel_id <= PrnList.getCountI()) ? PrnList.at(sel_id-1).PrinterName : 0;
		Data.Printer.Strip();
		if(PrnCfg.Flags & PrnCfg.fStoreLastSelPrn) {
			WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 0);
			reg_key.PutString(PPConst::WrParam_LastSelectedPrinter, Data.Printer);
		}
		// @v12.4.1 {
		if(Data.Dest == PrnDlgAns::aExport) {
			if(Data.ExpParam.Format && Data.ReportName.NotEmpty() && Data.ReportName.IsAscii()) {
				SSerializeContext sctx;
				SBuffer sbuf;
				if(Data.ExpParam.Serialize(+1, sbuf, &sctx)) {
					WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_RptExpParam, 0);
					if(reg_key.IsValid()) {
						reg_key.PutBinary(Data.ReportName, sbuf.GetBufC(), sbuf.GetAvailableSize());
					}
				}
			}
		}
		// } @v12.4.1
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PRINT2_REPORT)) {
			SetupReportEntry();
			clearEvent(event);
		}
		else if(event.isClusterClk(CTL_PRINT2_ACTION)) {
			SetupReportEntry();
			clearEvent(event);
		}
		else if(event.isCbSelected(CTLSEL_PRINT2_EXPFMT)) { // @v12.3.10
			const uint preserve_format = Data.ExpParam.Format;
			Data.ExpParam.Format = static_cast<uint>(getCtrlLong(CTLSEL_PRINT2_EXPFMT));
			enableCommand(cmPrintExportParam, Data.Dest == PrnDlgAns::aExport && Data.ExpParam.Format); 
			if(Data.ExpParam.Format != preserve_format) {
				SString exp_file_path;
				if(Data.ExpParam.GetDefaultExportFilePath(Data.ReportName, exp_file_path))
					Data.ExpParam.DestFileName = exp_file_path;
			}
		}
		else if(event.isCmd(cmPrintExportParam)) {
			Data.ExpParam.Format = static_cast<uint>(getCtrlLong(CTLSEL_PRINT2_EXPFMT));
			uint   dlg_id = 0;
			switch(Data.ExpParam.Format) {
				case CrystalReportExportParam::crexpfmtPdf: dlg_id = DLG_CRREXPPARAM_PDF; break;
				case CrystalReportExportParam::crexpfmtRtf: dlg_id = DLG_CRREXPPARAM_RTF; break;
				case CrystalReportExportParam::crexpfmtHtml: dlg_id = DLG_CRREXPPARAM_HTML; break;
				case CrystalReportExportParam::crexpfmtExcel: dlg_id = DLG_CRREXPPARAM_EXCEL; break;
				case CrystalReportExportParam::crexpfmtWinWord: dlg_id = DLG_CRREXPPARAM_WINWORD; break;
				case CrystalReportExportParam::crexpfmtCsv: dlg_id = DLG_CRREXPPARAM_CSV; break;
			}
			if(dlg_id) {
				class CrystalReportExportParamDialog : public TDialog {
					DECL_DIALOG_DATA(CrystalReportExportParam);
				public:
					CrystalReportExportParamDialog(uint dlgId) : TDialog(dlgId)
					{
					}
					DECL_DIALOG_SETDTS()
					{
						int   ok = 1;
						SString temp_buf;
						RVALUEPTR(Data, pData);
						//
						AddClusterAssocDef(CTL_CRREXPPARAM_DEST, 0, CrystalReportExportParam::destFile);
						AddClusterAssoc(CTL_CRREXPPARAM_DEST, 1, CrystalReportExportParam::destApp);
						SetClusterData(CTL_CRREXPPARAM_DEST, Data.Destination);
						setCtrlString(CTL_CRREXPPARAM_FILE, Data.DestFileName);
						SetIntRangeInput(this, CTL_CRREXPPARAM_PGRANGE, &Data.PageRange);
						if(Data.Format == CrystalReportExportParam::crexpfmtExcel) {
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 0, CrystalReportExportParam::fXlsDataOnly);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 1, CrystalReportExportParam::fXlsTabularFormat);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 2, CrystalReportExportParam::fXlsColumnHeadings);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 3, CrystalReportExportParam::fXlsCreatePgBrkForEachPage);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 4, CrystalReportExportParam::fXlsCvtDateToString);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 5, CrystalReportExportParam::fXlsShowGrid);
							SetClusterData(CTL_CRREXPPARAM_FLAGS, Data.Flags);
						}
						else if(Data.Format == CrystalReportExportParam::crexpfmtCsv) {
							temp_buf.Z();
							if(Data.CsvFieldSeparator == '\t')
								temp_buf = "tab";
							else if(Data.CsvFieldSeparator == ' ')
								temp_buf = "space";
							setCtrlString(CTL_CRREXPPARAM_CSVFS, temp_buf);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 0, CrystalReportExportParam::fCsvUseReportNumberFormat);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 1, CrystalReportExportParam::fCsvUseReportDateFormat);
							AddClusterAssoc(CTL_CRREXPPARAM_FLAGS, 2, CrystalReportExportParam::fCsvQuoteText);
							SetClusterData(CTL_CRREXPPARAM_FLAGS, Data.Flags);
						}
						return ok;
					}
					DECL_DIALOG_GETDTS()
					{
						int   ok = 1;
						SString temp_buf;
						//
						GetClusterData(CTL_CRREXPPARAM_DEST, &Data.Destination);
						getCtrlString(CTL_CRREXPPARAM_FILE, Data.DestFileName);
						GetIntRangeInput(this, CTL_CRREXPPARAM_PGRANGE, &Data.PageRange);
						if(Data.Format == CrystalReportExportParam::crexpfmtExcel) {
							GetClusterData(CTL_CRREXPPARAM_FLAGS, &Data.Flags);
						}
						else if(Data.Format == CrystalReportExportParam::crexpfmtCsv) {
							getCtrlString(CTL_CRREXPPARAM_CSVFS, temp_buf);
							if(temp_buf.IsEqiAscii("tab") || temp_buf.IsEqiAscii("\\t") || temp_buf.IsEqiAscii("09") || temp_buf.IsEqiAscii("x09"))
								Data.CsvFieldSeparator = '\t';
							else if(temp_buf.IsEqiAscii("space") || temp_buf.IsEqiAscii("32") || temp_buf.IsEqiAscii("x20"))
								Data.CsvFieldSeparator = ' ';
							else
								Data.CsvFieldSeparator = temp_buf.C(0);
							GetClusterData(CTL_CRREXPPARAM_FLAGS, &Data.Flags);
						}
						ASSIGN_PTR(pData, Data);
						return ok;
					}
				private:
					DECL_HANDLE_EVENT
					{
						TDialog::handleEvent(event);
					}
				};
				//const SString preserve_dest_fname(Data.ExpParam.DestFileName);
				int   edit_result = -1;
				CrystalReportExportParamDialog * dlg = new CrystalReportExportParamDialog(dlg_id);
				SString temp_buf;
				dlg->setDTS(&Data.ExpParam);
				if(edit_result < 0 && ExecView(dlg) == cmOK) {
					const int getdtsr = dlg->getDTS(&Data.ExpParam);
					if(getdtsr)
						edit_result = 1;
				}
				delete dlg;
			}
		}
		// @erik v10.4.10 {
		else if(event.isCmd(cmPrntCfg)) {
			PRINTDLGA pd;
			memzero(&pd, sizeof(pd));
			pd.lStructSize = sizeof(pd);
			//pd.hDevMode = NULL;
			//pd.hDevNames = NULL;
			pd.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
			pd.nCopies = 1;
			pd.nFromPage = 0xFFFF;
			pd.nToPage = 0xFFFF;
			pd.nMinPage = 1;
			pd.nMaxPage = 0xFFFF;
			if(PrintDlgA(&pd) == TRUE) {
				DEVMODEA * lpMode = static_cast<DEVMODEA *>(::GlobalLock(pd.hDevMode));
				DEVNAMES * lpNames = static_cast<DEVNAMES *>(::GlobalLock(pd.hDevNames));
				DEVMODEA mode = *lpMode;
				::GlobalUnlock(pd.hDevMode);
				::GlobalUnlock(pd.hDevNames);
				::GlobalFree(pd.hDevMode);
				::GlobalFree(pd.hDevNames);
				SETIFZ(Data.P_DevMode, new DEVMODEA);
				*(Data.P_DevMode) = mode;
				::DeleteDC(pd.hDC); // ?
			}
			else
				ZDELETE(Data.P_DevMode);
		}
		// } @erik
	}
	void   SetupReportEntry()
	{
		const  uint id = static_cast<uint>(getCtrlLong(CTLSEL_PRINT2_REPORT));
		bool   enable_email = false;
		SString temp_buf;
		SString data_name;
		SString path;
		GetClusterData(CTL_PRINT2_ACTION, &Data.Dest);
		if(id <= Data.Entries.getCount()) {
			SString stat_buf;
			const ReportDescrEntry * p_entry = Data.Entries.at(id-1);
			setStaticText(CTL_PRINT2_ST_FILENAME, p_entry->ReportPath_);
			SFile::Stat fs;
			SFile::GetStat(p_entry->ReportPath_, 0, &fs, 0);
			{
				LDATETIME dtm_mod;
				stat_buf.Cat(dtm_mod.SetNs100(fs.ModTm_));
			}
			setStaticText(CTL_PRINT2_ST_FILEDTTM, stat_buf);
			setStaticText(CTL_PRINT2_ST_DATANAME, p_entry->DataName_);
			data_name = p_entry->DataName_;
		}
		else {
			setStaticText(CTL_PRINT2_ST_FILENAME, 0);
			setStaticText(CTL_PRINT2_ST_FILEDTTM, 0);
			setStaticText(CTL_PRINT2_ST_DATANAME, 0);
		}
		if(oneof2(Data.Dest, PrnDlgAns::aPrepareData, PrnDlgAns::aPrepareDataAndExecCR)) {
			PPGetPath(PPPATH_REPORTDATA, path);
			if(data_name.NotEmpty())
				path.SetLastSlash().Cat(data_name);
			disableCtrl(CTL_PRINT2_MAKEDATAPATH, false);
		}
		else if(oneof2(Data.Dest, PrnDlgAns::aExport, PrnDlgAns::aExportXML)) {
			if(EnableEMail) {
				enable_email = true;
				disableCtrl(CTL_PRINT2_MAKEDATAPATH, false);
				path = Data.EmailAddr;
			}
			if(Data.Dest == PrnDlgAns::aExport) {
				TView * p_view = getCtrlView(CTLSEL_PRINT2_EXPFMT);
				if(TView::IsSubSign(p_view, TV_SUBSIGN_COMBOBOX)) {
					ComboBox * p_cb = static_cast<ComboBox *>(p_view);
					if(!p_cb->GetSettledTag()) {
						const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
						if(p_uedc) {
							StrAssocArray expfmt_list;
							const long fmt_id_list[] = {
								CrystalReportExportParam::crexpfmtPdf, CrystalReportExportParam::crexpfmtRtf, CrystalReportExportParam::crexpfmtHtml,
								CrystalReportExportParam::crexpfmtExcel, CrystalReportExportParam::crexpfmtWinWord, CrystalReportExportParam::crexpfmtCsv 
							};
							for(uint i = 0; i < SIZEOFARRAY(fmt_id_list); i++) {
								const long fmt_id = fmt_id_list[i];
								const uint64 ued = UED::ApplyMetaToRawValue32(UED_META_DATAFORMAT, fmt_id);
								if(ued && p_uedc->GetText(ued, UED_LINGUALOCUS_EN, temp_buf)) {
									expfmt_list.AddFast(fmt_id, temp_buf);
								}
							}
							if(expfmt_list.getCount()) {
								CrystalReportExportParam saved_exp_param;
								bool   exp_param_restored = false;
								if(Data.ReportName.NotEmpty() && Data.ReportName.IsAscii()) {
									WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_RptExpParam, 1);
									if(reg_key.IsValid()) {
										SBuffer sbuf;
										if(reg_key.GetBinary(Data.ReportName, sbuf)) {
											if(sbuf.GetAvailableSize()) {
												SSerializeContext sctx;
												if(saved_exp_param.Serialize(-1, sbuf, &sctx)) {
													exp_param_restored = true;
												}
											}
										}
									}
								}
								if(!Data.ExpParam.Format && exp_param_restored) {
									Data.ExpParam.Format = saved_exp_param.Format;
									Data.ExpParam.Destination = saved_exp_param.Destination;
									{
										SString exp_file_path;
										if(Data.ExpParam.GetDefaultExportFilePath(Data.ReportName, exp_file_path))
											Data.ExpParam.DestFileName = exp_file_path;
									}
								}
								SetupStrAssocCombo(this, CTLSEL_PRINT2_EXPFMT, expfmt_list, Data.ExpParam.Format, 0);
							}
						}
						p_cb->SetSettledTag(true);
					}
				}				
			}
		}
		else {
			disableCtrl(CTL_PRINT2_MAKEDATAPATH, true);
		}
		disableCtrl(CTLSEL_PRINT2_EXPFMT, Data.Dest != PrnDlgAns::aExport); // @v12.3.9
		enableCommand(cmPrintExportParam, Data.Dest == PrnDlgAns::aExport && Data.ExpParam.Format); // @v12.3.10
		disableCtrl(CTL_PRINT2_DOMAIL, !enable_email);
		SetupWordSelector(CTL_PRINT2_MAKEDATAPATH, (enable_email ? new TextHistorySelExtra("email-common") : 0), 0, 2, WordSel_ExtraBlock::fFreeText);
		setCtrlString(CTL_PRINT2_MAKEDATAPATH, path);
	}

	int    EnableEMail;
	PPPrinterCfg PrnCfg;
	SString InitPrepareDataPath;
	TSVector <SPrinting::PrnInfo> PrnList;
};

int EditPrintParam(PrnDlgAns * pData) { return PPDialogProcBody <Print2Dialog, PrnDlgAns> (pData); }

int GetWindowsPrinter(PPID * pPrnID, SString * pPort)
{
	int    ok = -1;
	WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 1);
	uint32 loc_prn_id = 0;
	PPLocPrinter loc_prn;
 	PPObjLocPrinter obj_locprn;
 	MEMSZERO(loc_prn);
 	reg_key.GetDWord(PPConst::WrParam_DefaultWindowsPrinter, &loc_prn_id);
 	if(loc_prn_id && (ok = obj_locprn.Search(loc_prn_id, &loc_prn)) > 0) {
		ASSIGN_PTR(pPrnID, (PPID)loc_prn_id);
		CALLPTRMEMB(pPort, CopyFrom(loc_prn.Port));
	}
	return ok;
}

static constexpr uint32 CrystalReportPrintParamBlock_Version = 0;

CrystalReportPrintParamBlock::CrystalReportPrintParamBlock() : Ver(CrystalReportPrintParamBlock_Version), Action(actionUndef), InternalFlags(0), NumCopies(0), Options(0)
{
	MEMSZERO(DevMode);
}

int CrystalReportPrintParamBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Ver, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Action, rBuf));
	THROW_SL(pSCtx->Serialize(dir, InternalFlags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, NumCopies, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Options, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(DevMode), &DevMode, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, ReportPath, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ReportName, rBuf));
	THROW_SL(pSCtx->Serialize(dir, EmailAddr, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Dir, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Printer, rBuf));
	THROW_SL(InetAcc.Serialize(dir, rBuf, pSCtx));
	THROW_SL(ExpParam.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}
//
//
//
int EditDefaultPrinterCfg()
{
	int    ok = 1;
	ushort v = 0;
	PPID   loc_prn_id = 0;
	PPPrinterCfg cfg;
	TDialog * dlg = new TDialog(DLG_PRNCFG);
	THROW(CheckDialogPtr(&dlg));
	THROW(PPGetPrinterCfg(PPOBJ_CONFIG, PPCFG_MAIN, &cfg));
	dlg->disableCtrl(CTLSEL_PRNCFG_PRINTER, true); // @v12.3.11 
	// @v12.3.11 SetupStringCombo(dlg, CTLSEL_PRNCFG_PRINTER, PPTXT_PRNTYPE, cfg.PrnCmdSet);
	GetWindowsPrinter(&loc_prn_id, 0);
	SetupPPObjCombo(dlg, CTLSEL_PRNCFG_WINPRINTER, PPOBJ_LOCPRINTER, loc_prn_id, 0, 0);
	dlg->setCtrlData(CTL_PRNCFG_PORT, cfg.Port);
	dlg->AddClusterAssoc(CTL_PRNCFG_FLAGS, 0, PPPrinterCfg::fUseDuplexPrinting);
	dlg->AddClusterAssoc(CTL_PRNCFG_FLAGS, 1, PPPrinterCfg::fStoreLastSelPrn);
	dlg->SetClusterData(CTL_PRNCFG_FLAGS, cfg.Flags);
	if(ExecView(dlg) == cmOK) {
		// @v12.3.11 dlg->getCtrlData(CTLSEL_PRNCFG_PRINTER, &cfg.PrnCmdSet);
		dlg->getCtrlData(CTL_PRNCFG_PORT, cfg.Port);
		{
			uint32 dw_loc_prn_id = 0;
			WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 0);
			dlg->getCtrlData(CTLSEL_PRNCFG_WINPRINTER, &loc_prn_id);
			dw_loc_prn_id = static_cast<uint32>(loc_prn_id);
			reg_key.PutDWord(PPConst::WrParam_DefaultWindowsPrinter, dw_loc_prn_id);
		}
		cfg.Flags = static_cast<int16>(dlg->GetClusterData(CTL_PRNCFG_FLAGS));
		THROW(PPSetPrinterCfg(PPOBJ_CONFIG, PPCFG_MAIN, &cfg));
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int MakeCRptDataFiles(int verifyAll/*=0*/)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_MKRPTFLS);
	if(CheckDialogPtrErr(&dlg)) {
		SString rpt_name;
		SString rpt_path;
		SString fname;
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_MKRPTFLS_RPTPATH, CTL_MKRPTFLS_RPTPATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		PPGetPath(PPPATH_REPORTDATA, rpt_path);
		if(verifyAll) {
			rpt_name = "ALL";
			dlg->disableCtrl(CTL_MKRPTFLS_RPTNAME, true);
		}
		dlg->setCtrlString(CTL_MKRPTFLS_RPTNAME, rpt_name);
		dlg->setCtrlString(CTL_MKRPTFLS_RPTPATH, rpt_path);
		while(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_MKRPTFLS_RPTNAME, rpt_name);
			dlg->getCtrlString(CTL_MKRPTFLS_RPTPATH, rpt_path);
			PPWaitStart();
			if(rpt_name.NotEmptyS()) {
				uint   pos = 0;
				StringSet ss;
				StringSet ss_data_name;
				SString data_path;
				if(rpt_name.IsEqiAscii("ALL")) {
					PPGetFilePath(PPPATH_BIN, PPFILNAM_STDRPT_INI, fname);
					PPIniFile std_ini_file(fname);
					THROW_SL(std_ini_file.IsValid());
					THROW_SL(std_ini_file.GetSections(&ss));
				}
				else {
					THROW_SL(ss.add(rpt_name));
				}
				for(pos = 0; ss.get(&pos, rpt_name);) {
					PrnDlgAns report_descr_data(rpt_name);
					report_descr_data.SetupReportEntries(0);
					report_descr_data.Flags |= PrnDlgAns::fForceDDF; // Создание файлов данных обязательно должно включать создание словаря //
					for(uint i = 0; i < report_descr_data.Entries.getCount(); i++) {
						const SString & r_data_name = report_descr_data.Entries.at(i)->DataName_;
						if(r_data_name.NotEmpty()) {
							PPWaitMsg(r_data_name);
							(data_path = rpt_path).SetLastSlash().Cat(r_data_name);
							if(!ss_data_name.searchNcAscii(r_data_name, 0, 0)) {
								DlRtm * p_rtm = 0;
								DlContext ctx;
								DlRtm::ExportParam ep;
								if(!ctx.InitSpecial(DlContext::ispcExpData)) {
									// @todo @err
								}
								else if(!ctx.CreateDlRtmInstance(r_data_name, &p_rtm)) {
									// @todo @err
								}
								else {
									ep.DestPath = data_path;
									ep.Flags |= DlRtm::ExportParam::fForceDDF;
									if(p_rtm->Export(ep) > 0) {
										ss_data_name.add(r_data_name);
										if(verifyAll) {
											// @v12.4.1 VerifyCrpt(report_descr_data.Entries.at(i)->ReportPath_, data_path);
										}
									}
									else {
										// @todo @err
									}
								}
								delete p_rtm;
							}
						}
					}
				}
				ok = 1;
			}
			PPWaitStop();
			if(verifyAll)
				break;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

static int FASTCALL __PPAlddPrint(int rptId, PPFilt * pF, int isView, const PPReportEnv * pEnv)
{
	int    ok = 1;
	bool   inherited_tbl_names = false;
	bool   diffidbyscope = false;
	DlRtm   * p_rtm = 0;
	SString temp_buf;
	SString data_name;
	SString /*fn*/report_file_name;
	SString printer_name;
	SString report_name; 
	SString report_data_name;
	const  uint sur_key_last_count = DS.GetTLA().SurIdList.getCount(); // См. коммент в конце функции
	THROW(SReport::GetReportAttributes(rptId, &report_name, &report_data_name));
	{
		int    output_destination = 0;
		int    report_options = 0;
		PrnDlgAns pans(report_name);
		const ReportDescrEntry * p_sel_entry = 0;
		if(pEnv) {
			pans.DefPrnForm = pEnv->DefPrnForm;
			pans.EmailAddr = pEnv->EmailAddr;
			if(pEnv->PrnFlags & SReport::DisableGrouping) {
				report_options |= SPRN_SKIPGRPS;
			}
			if(pEnv->PrnFlags & SReport::PrintingNoAsk && pans.SetupReportEntries(pEnv->ContextSymb) > 0) {
				const ReportDescrEntry * p_entry = pans.Entries.at(0);
				report_file_name = p_entry->ReportPath_;
				data_name = p_entry->DataName_;
				output_destination = (pEnv->PrnFlags & SReport::XmlExport) ? PrnDlgAns::aExportXML : PrnDlgAns::aPrint;
				diffidbyscope = LOGIC(p_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
			}
		}
		else
			pans.DefPrnForm.Z();
		if(!output_destination) {
			if(EditPrintParam(&pans) > 0) {
				if(pans.P_DevMode) {
					;
				}
				pans.Flags &= ~pans.fForceDDF;
				p_sel_entry = pans.Entries.at(pans.Selection);
				SFsPath::NormalizePath(p_sel_entry->ReportPath_, SFsPath::npfCompensateDotDot, report_file_name);
				data_name = p_sel_entry->DataName_;
				inherited_tbl_names = LOGIC(p_sel_entry->Flags & ReportDescrEntry::fInheritedTblNames);
				diffidbyscope = LOGIC(p_sel_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
				output_destination = pans.Dest;
				printer_name  = pans.Printer;
				SETFLAG(report_options, SPRN_USEDUPLEXPRINTING, BIN(pans.Flags & PrnDlgAns::fUseDuplexPrinting));
			}
			else
				ok = -1;
		}
		// @v11.8.8 {
		else if(output_destination == PrnDlgAns::aPrint) {
			if(pEnv)
				printer_name = pEnv->PrnPort;
		}
		// } @v11.8.8 
		if(ok > 0) {
			data_name.SetIfEmpty(report_data_name);
			SString out_file_name;
			DlContext ctx;
			DlRtm::ExportParam ep;
			THROW(ctx.InitSpecial(DlContext::ispcExpData));
			THROW(ctx.CreateDlRtmInstance(data_name, &p_rtm));
			PPWaitStart();
			ep.P_F = pF;
			ep.Sort = pEnv ? pEnv->Sort : 0;
			SETFLAG(ep.Flags, DlRtm::ExportParam::fIsView, isView);
			SETFLAG(ep.Flags, DlRtm::ExportParam::fInheritedTblNames, inherited_tbl_names);
			SETFLAG(ep.Flags, DlRtm::ExportParam::fDiff_ID_ByScope, diffidbyscope);
			SETFLAG(ep.Flags, DlRtm::ExportParam::fDontWriteXmlTypes, 1);
			if(p_sel_entry && p_sel_entry->Flags & ReportDescrEntry::fTddoResource) {
				output_destination = PrnDlgAns::aExportTDDO;
				Tddo t;
				StringSet ext_param_list;
				int    fld_n = 0;
				SBuffer result;
				THROW(Tddo::LoadFile(report_file_name, temp_buf));
				t.SetInputFileName(report_file_name);
				{
					SString inner_fn;
                	SFsPath ps(report_file_name);
                	ps.Drv.Z();
                	ps.Dir.Z();
					const SString nam = ps.Nam;
					if(p_sel_entry->OutputFormat.IsEqiAscii("html")) {
						ps.Ext.Z();
                		ps.Merge(inner_fn);
						PPGetFilePath(PPPATH_OUT, inner_fn, out_file_name);
						SFile::Remove(out_file_name);
						THROW_SL(SFile::CreateDir(out_file_name));
						ep.DestPath = out_file_name;
						ep.OutputFormat = SFileFormat(SFileFormat::Html);
						out_file_name.SetLastSlash().Cat("index.html");
					}
					else if(p_sel_entry->OutputFormat.IsEqiAscii("latex")) {
						ps.Ext.Z();
                		ps.Merge(inner_fn);
						PPGetFilePath(PPPATH_OUT, inner_fn, out_file_name);
						SFile::Remove(out_file_name);
						THROW_SL(SFile::CreateDir(out_file_name));
						ep.DestPath = out_file_name;
						ep.OutputFormat = SFileFormat(SFileFormat::Latex);
						out_file_name.SetLastSlash().Cat(nam).DotCat("tex");
					}
					else {
						ps.Ext = "txt";
                		ps.Merge(inner_fn);
						PPGetFilePath(PPPATH_OUT, inner_fn, out_file_name);
						PPGetPath(PPPATH_OUT, ep.DestPath);
					}
				}
				THROW(t.Process(data_name, temp_buf, ep, 0, result));
                {
                	const size_t sz = result.GetAvailableSize();
					SFile f_out(out_file_name, SFile::mWrite);
					THROW_SL(f_out.IsValid());
					THROW_SL(f_out.Write(result.GetBuf(), sz));
                }
			}
			else if(output_destination == PrnDlgAns::aExportXML) {
				ep.Cp = DS.GetConstTLA().DL600XmlCp;
				THROW(p_rtm->ExportXML(ep, out_file_name));
				if(pans.Flags & pans.fEMail && pans.EmailAddr.NotEmptyS() && fileExists(out_file_name)) {
					//
					// Отправка на определенный почтовый адрес
					//
					PPAlbatrossConfig alb_cfg;
					THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
					if(alb_cfg.Hdr.MailAccID) {
						THROW(SendMailWithAttachment(pans.ReportName, out_file_name, pans.ReportName, pans.EmailAddr, alb_cfg.Hdr.MailAccID));
					}
				}
			}
			else {
				THROW_PP_S(fileExists(report_file_name), PPERR_REPORTFILENOTFOUND, report_file_name); // @v12.4.1
				if(oneof2(output_destination, PrnDlgAns::aPrepareData, PrnDlgAns::aPrepareDataAndExecCR)) {
					ep.Flags |= DlRtm::ExportParam::fForceDDF;
					ep.DestPath = pans.PrepareDataPath;
				}
				else {
					{
						//
						// Для Crystal Reports 10 и выше удаляем каталог подготовки данных (если существует)
						// ибо почему-то Crystal Reports использует его с приоритетом в некоторых случаях.
						//
						PPGetPath(PPPATH_REPORTDATA, temp_buf);
						if(data_name.NotEmpty())
							temp_buf.SetLastSlash().Cat(data_name);
						if(temp_buf.NotEmpty() && SFile::IsDir(temp_buf)) {
							SVerT  crr_ver = QueryCrrVersion();
							if(crr_ver.GetMajor() >= 10)
								SFile::RemoveDir(temp_buf);
						}
					}
					ep.Flags &= ~DlRtm::ExportParam::fForceDDF;
				}
				THROW(p_rtm->Export(ep));
			}
			PPWaitStop();
			switch(output_destination) {
				case PrnDlgAns::aExportXML:
				case PrnDlgAns::aExportTDDO:
				case PrnDlgAns::aPrepareData:
					// done above
					break;
				case PrnDlgAns::aPrepareDataAndExecCR:
					{
						SString cr_path_;
						const bool is_there_cr = FindExeByExt2(".rpt", cr_path_, "CrystalReports.9.1");
						if(is_there_cr) {
							(temp_buf = cr_path_).Space().Cat(report_file_name);
							STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
							strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize() / sizeof(TCHAR));
							STARTUPINFO si;
							PROCESS_INFORMATION pi;
							MEMSZERO(si);
							si.cb = sizeof(si);
							MEMSZERO(pi);
							const int r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, 0, &si, &pi);
							if(!r) {
								SLS.SetOsError(0, 0);
								PPSetErrorSLib();
								CALLEXCEPT();
							}
						}
					}
					break;
				case PrnDlgAns::aPrint:
				case PrnDlgAns::aPreview:
				case PrnDlgAns::aExport:
					{
						CrystalReportPrintParamBlock req;
						CrystalReportPrintReply rep;
						req.Options = report_options;
						req.ReportPath = report_file_name;
						req.ReportName = pans.ReportName;
						req.Dir = ep.Path;
						req.Printer = printer_name;
						if(output_destination == PrnDlgAns::aExport) {
							req.Action = CrystalReportPrintParamBlock::actionExport;
							req.ExpParam = pans.ExpParam;
							if(pans.Flags & pans.fEMail && pans.EmailAddr.NotEmptyS())
								req.EmailAddr =  pans.EmailAddr;
						}
						else {
							if(pans.P_DevMode) {
								req.DevMode = *pans.P_DevMode;
								req.InternalFlags |= CrystalReportPrintParamBlock::intfDevModeValid;
							}
							if(output_destination == PrnDlgAns::aPrint) {
								req.Action = CrystalReportPrintParamBlock::actionPrint;
								req.NumCopies = pans.NumCopies;
							}
							else if(output_destination == PrnDlgAns::aPreview) {
								req.Action = CrystalReportPrintParamBlock::actionPreview;
							}
						}
						THROW(CrystalReportPrint2_ClientExecution(req, rep));
					}
					break;
				default:
					ok = -1;
					break;
			}
		}
	}
	CATCH
		if(!pEnv || !(pEnv->PrnFlags & SReport::NoRepError))
			PPError();
		ok = 0;
	ENDCATCH
	PPWaitStop();
	delete p_rtm;
	{
		//
		// Закладываемся на то, что данная функция не может вызываться рекурсивно:
		// удаляет все динамические идентификаторы из DS.GetTLA.SurIdList созданные
		// в течении вызова этой функции.
		//
		uint c = DS.GetTLA().SurIdList.getCount();
		while(c > sur_key_last_count)
			DS.GetTLA().SurIdList.atFree(--c);
	}
	return ok;
}

int FASTCALL PPAlddPrint(int rptId, PPFilt & rF, const PPReportEnv * pEnv) { return __PPAlddPrint(rptId, &rF, 0, pEnv); }
int FASTCALL PPAlddPrint(int rptId, PView & rV, const PPReportEnv * pEnv) { return __PPAlddPrint(rptId, reinterpret_cast<PPFilt *>(&rV), 1, pEnv); }

static int Implement_ExportDL600DataToBuffer(const char * pDataName, long id, void * pPtr, long epFlags, SCodepageIdent cp, SString & rBuf)
{
	rBuf.Z();

	int    ok = 1;
	PPFilt f;
	DlRtm::ExportParam ep;
	DlRtm   * p_rtm = 0;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxtExportData);
	THROW(p_ctx);
	THROW(p_rtm = p_ctx->GetRtm(pDataName));
	f.ID = id;
	f.Ptr = pPtr;
	ep.P_F = &f;
	ep.Sort = 0;
	ep.Flags = epFlags;
	ep.Cp = cp;
	SETFLAG(ep.Flags, DlRtm::ExportParam::fInheritedTblNames, 1);
	SETFLAG(ep.Flags, DlRtm::ExportParam::fDontWriteXmlDTD, 1);
	THROW(p_rtm->PutToXmlBuffer(ep, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL PPExportDL600DataToBuffer(const char * pDataName, long id, SCodepageIdent cp, SString & rBuf)
	{ return Implement_ExportDL600DataToBuffer(pDataName, id, 0, 0, cp, rBuf); }
int FASTCALL PPExportDL600DataToBuffer(const char * pDataName, void * ptr, SCodepageIdent cp, SString & rBuf)
	{ return Implement_ExportDL600DataToBuffer(pDataName, 0, ptr, 0, cp, rBuf); }
int FASTCALL PPExportDL600DataToBuffer(const char * pDataName, PPView * pView, SCodepageIdent cp, SString & rBuf)
	{ return Implement_ExportDL600DataToBuffer(pDataName, 0, pView, DlRtm::ExportParam::fIsView, cp, rBuf); }

int  FASTCALL PPExportDL600DataToJson(const char * pDataName, PPView * pV, SString & rBuf)
{
	int         ok = 1;
	DlContext * p_ctx = 0;
	DlRtm * p_rtm = 0;
	THROW(p_ctx = DS.GetInterfaceContext(PPSession::ctxtExportData));
	THROW(pV);
	THROW(p_rtm = p_ctx->GetRtm(pDataName));
	THROW(p_rtm->PutToJsonBuffer(pV, rBuf.Z(), 0 /* flags */));
	CATCHZOK
	return ok;
}

int FASTCALL PPExportDL600DataToJson(const char * pDataName, StrAssocArray * pStrAssocAry, void * ptr, SString & rBuf)
{
	int         ok = 1;
	DlContext * p_ctx = 0;
	DlRtm * p_rtm = 0;
	THROW(p_ctx = DS.GetInterfaceContext(PPSession::ctxtExportData));
	if(pStrAssocAry) {
		THROW(p_rtm = p_ctx->GetRtm(isempty(pDataName) ? "StrAssocArray" : pDataName));
		THROW(p_rtm->PutToJsonBuffer(pStrAssocAry, rBuf.Z(), 0/*flags*/));
	}
	else if(ptr) {
		THROW(p_rtm = p_ctx->GetRtm(pDataName));
		THROW(p_rtm->PutToJsonBuffer(ptr, rBuf.Z(), 0/*flags*/));
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}
//
//
//
SString & GetCrr32ProxiPipeName(SString & rBuf) { return SFile::MakeNamedPipeName(PPConst::PipeCrr32Proxi, rBuf); } // @v11.9.5
//
//
//
class Crr32SupportClient {
public:
	Crr32SupportClient(const char * pPipeNameUtf8) : BusyPipeTimeoutMs(20000), WrBuf(SKILOBYTE(4)), RdBuf(SKILOBYTE(4))
	{
		PipeNameU.CopyFromUtf8Strict(pPipeNameUtf8, sstrlen(pPipeNameUtf8));
	}
	SIntHandle Connect()
	{
		SIntHandle result;
		if(PipeNameU.NotEmpty()) {
			bool do_exit = false;
			do {
				result = ::CreateFileW(PipeNameU, GENERIC_READ|GENERIC_WRITE, 0/*no sharing*/,
					NULL/*default security attributes*/, OPEN_EXISTING/*opens existing pipe*/, 0/*default attributes*/, NULL/*no template file*/);
				if(!result) {
					if(GetLastError() == ERROR_PIPE_BUSY) {
						boolint w_ok = ::WaitNamedPipeW(PipeNameU, BusyPipeTimeoutMs);
						if(!w_ok) {
							do_exit = true;
						}
					}
					else {
						do_exit = true;
					}
				}
			} while(!result && !do_exit);
			if(!!result) {
				DWORD pipe_mode = PIPE_READMODE_MESSAGE; 
				boolint _ok = ::SetNamedPipeHandleState(result, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
				if(!_ok) {
					::CloseHandle(result);
					result.Z();
				}
					
			}
		}
		return result;
	}
	bool   SendQuitCommand(SIntHandle hPipe)
	{
		bool   ok = false;
		SJson * p_js_reply = SendCommand(hPipe, -1, "quit", 0);
		if(p_js_reply) {
			delete p_js_reply;
			ok = true;
		}
		return ok;
	}
	SJson * SendCommand(SIntHandle hPipe, int waitTimeout, const char * pCmdUtf8, const char * pParam)
	{
		SJson * p_js_reply = 0;
		SString temp_buf;
		SString reply_buf;
		THROW(hPipe); // @todo @err
		THROW(!isempty(pCmdUtf8)); // @todo @err
		{
			SJson js_query(SJson::tOBJECT);
			js_query.InsertString("cmd", pCmdUtf8);
			if(!isempty(pParam)) {
				js_query.InsertString("param", pParam);
			}
			if(sstreqi_ascii(pCmdUtf8, "run")) {
				js_query.InsertBool("quitafter", true);
			}
			//
			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = ::WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				reply_buf.Z();
				bool more_data = false;
				do {
					more_data = false;
					union {;
						DWORD rd_size;
						size_t actual_size;
					} rs;
					rs.actual_size = 0;
					RdBuf[0] = 0;
					int rd_result = SFile::AsyncReadFileWithTimeout(hPipe, RdBuf, RdBuf.GetSize(), waitTimeout, &rs.actual_size);
					if(rd_result > 0) {
						reply_buf.CatN(RdBuf, rs.actual_size);
					}
					else if(rd_result < 0) {
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertString("result", "timeout");
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
				if(!p_js_reply)
					p_js_reply = SJson::Parse(reply_buf);
			}
		}
		CATCH
			ZDELETE(p_js_reply);
		ENDCATCH
		return p_js_reply;
	}
private:
	SStringU PipeNameU;
	uint   BusyPipeTimeoutMs;
	STempBuffer WrBuf;
	STempBuffer RdBuf;
};

int CrystalReportPrint2_ClientExecution(CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply)
{
	struct InternalBlock {
		static int SendPrintCommandToServer(Crr32SupportClient & rCli, SIntHandle hPipe, CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply)
		{
			rReply.Z();
			int    result = 0;
			SSerializeContext sctx;
			int    srv_ret_result = -1;
			SString srv_ret_info;
			SBuffer sbuf;
			SString temp_buf;
			//slfprintf_stderr("Call on named-pipe #%u\n", i+1);
			rBlk.Serialize(+1, sbuf, &sctx);
			temp_buf.EncodeMime64(sbuf.GetBufC(sbuf.GetRdOffs()), sbuf.GetAvailableSize());
			SJson * p_js_reply = rCli.SendCommand(hPipe, 2000, "run", temp_buf);
			if(p_js_reply) {
				//p_js_reply->ToStr(temp_buf); // @debug
				if(SJson::IsObject(p_js_reply)) {
					for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Text.IsEqiAscii("status")) {
							SJson::GetChildTextUnescaped(p_cur, temp_buf);
							if(temp_buf.IsEqiAscii("ok")) {
								srv_ret_result = 1;
							}
							else if(temp_buf.IsEqiAscii("fail")) {
								srv_ret_result = 0;
							}
							else {
								; // ?
							}
						}
						else if(p_cur->Text.IsEqiAscii("info")) {
							SJson::GetChildTextUnescaped(p_cur, srv_ret_info);
							srv_ret_info.Transf(CTRANSF_UTF8_TO_INNER);
						}
						else if(p_cur->Text.IsEqiAscii("err")) {
							if(SJson::IsObject(p_cur->P_Child)) {
								rReply.FromJsonObj(p_cur->P_Child);
							}
						}
					}
				}
				if(srv_ret_result > 0)
					result = 1;
				else if(!srv_ret_result) {
					PPSetError(rReply.Code);
					if(rReply.Code == PPERR_CRYSTAL_REPORT) {
						CrwError = rReply.LocIdent;	
					}
					result = 0;
				}
				ZDELETE(p_js_reply);
			}
			return result;
		}
	};

	int    result = 0;
	const  char * p_ext_process_name = "crr32_support.exe";
	const  bool   force_ext_process = LOGIC(CConfig.Flags2 & CCFLG2_FORCE_CRR32_SUPPORT_SERVER);
	const  bool   use_ext_process = ((force_ext_process || SlDebugMode::CT()) && !(CConfig.Flags2 & CCFLG2_DISABLE_CRR32_SUPPORT_SERVER));
	bool   do_use_local_func = !use_ext_process;
	void * h_parent_window_for_preview = APPL ? APPL->H_TopOfStack : 0;
	SString temp_buf;
	{
		// Предварительная работа над CrystalReportPrintParamBlock для того, чтобы максимально упростить и изолировать CrystalReportPrint2()
		if(rBlk.Action == CrystalReportPrintParamBlock::actionExport) {
			if(rBlk.EmailAddr.NotEmpty()) {
				bool local_ok = false;
				//
				// Если задан email-адрес для отправки отчета, то нужно добыть аккаунт почтового клиента дабы осуществить отправку!
				//
				PPAlbatrossConfig alb_cfg;
				if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0) {
					if(alb_cfg.Hdr.MailAccID) {
						PPInternetAccount2 account;
						PPObjInternetAccount ia_obj;
						if(ia_obj.Get(alb_cfg.Hdr.MailAccID, &account) > 0) {
							rBlk.InetAcc = account;
							local_ok = true;
						}
						else {
							; //
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
				if(!local_ok) {
					rBlk.EmailAddr.Z();
					rBlk.InetAcc.Z();
				}
			}
		}
		else {
			SString inner_printer_buf;
			const char * p_inner_printer = 0;
			const  DEVMODEA * p_dev_mode = (rBlk.InternalFlags & CrystalReportPrintParamBlock::intfDevModeValid) ? &rBlk.DevMode : 0;
			if(p_dev_mode) {
				inner_printer_buf = reinterpret_cast<const char *>(p_dev_mode->dmDeviceName);
				if(inner_printer_buf.NotEmptyS())
					p_inner_printer = inner_printer_buf;
				rBlk.NumCopies = (p_dev_mode->dmCopies > 0 && p_dev_mode->dmCopies <= 1000) ? p_dev_mode->dmCopies : 1; // Мы изменили значение rBlk.NumCopies!
			}
			else
				p_inner_printer = rBlk.Printer;
			if(isempty(p_inner_printer)) {
				if(DS.GetConstTLA().PrintDevice.NotEmpty()) {
					rBlk.Printer = DS.GetConstTLA().PrintDevice; // Мы изменили значение rBlk.Printer
				}
				else {
					GetWindowsPrinter(0, &rBlk.Printer); // Мы, возможно, изменили значение rBlk.Printer
				}
			}
		}
	}
	if(use_ext_process) {
		bool   is_new_process = false;
		SString pipe_name;
		SString module_path;
		PPGetFilePath(PPPATH_BIN, p_ext_process_name, module_path);
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			S_GUID pipe_symb_uuid(SCtrGenerate_);
			SString pipe_symb;
			pipe_symb_uuid.ToStr(S_GUID::fmtPlain, pipe_symb);
			if(fileExists(module_path)) {
				SlProcess::Result process_result;
				SlProcess process;
				process.SetPath(module_path);
				SFsPath ps(module_path);
				ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
				process.SetWorkingDir(temp_buf);
				temp_buf.Z().Cat("/pipesymb").Colon().Cat(pipe_symb);
				process.AddArg(temp_buf);
				if(process.Run(&process_result)) {
					SDelay(20);
					is_new_process = true;
					//
					Crr32SupportClient cli(SFile::MakeNamedPipeName(pipe_symb, pipe_name));
					SIntHandle h_pipe(cli.Connect());
					uint  try_to_connect_count = 1;
					if(!h_pipe && is_new_process) {
						for(uint i = 0; !h_pipe && i < 10; i++) {
							SDelay(50);
							h_pipe = cli.Connect();
							try_to_connect_count++;
						}
					}
					if(!h_pipe) {
						// @todo @log
						do_use_local_func = true;
					}
					else {
						result = InternalBlock::SendPrintCommandToServer(cli, h_pipe, rBlk, rReply);
					}
					::CloseHandle(h_pipe);
				}
				else
					do_use_local_func = true;
			}
			else
				do_use_local_func = true;
		}
		else {
			bool   local_done = false;
			GetCrr32ProxiPipeName(pipe_name);
			uint prc_id = SlProcess::SearchProcessByName(p_ext_process_name);
			if(prc_id) {
				// Пытаемся отправить команду ping для того, чтоб удостовериться, что существует
				// общий процесс для обработки не-интерактивных команд печати
				Crr32SupportClient cli(pipe_name);
				SIntHandle h_pipe(cli.Connect());
				if(!!h_pipe) {
					SJson * p_js_reply = cli.SendCommand(h_pipe, 1000, "ping", 0);
					if(p_js_reply) {
						// ...
						//result = 1; // ???
						p_js_reply->ToStr(temp_buf); // @debug
						ZDELETE(p_js_reply);
						//
						result = InternalBlock::SendPrintCommandToServer(cli, h_pipe, rBlk, rReply);
						local_done = true;
					}
				}
			}
			if(!local_done) {
				if(fileExists(module_path)) {
					SlProcess::Result prc_result;
					SlProcess process;
					process.SetPath(module_path);
					SFsPath ps(module_path);
					ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
					process.SetWorkingDir(temp_buf);
					if(process.Run(&prc_result)) {
						SDelay(20);
						is_new_process = true;
						//
						Crr32SupportClient cli(pipe_name);
						SIntHandle h_pipe(cli.Connect());
						uint  try_to_connect_count = 1;
						if(!h_pipe) {
							for(uint i = 0; !h_pipe && i < 10; i++) {
								SDelay(50);
								h_pipe = cli.Connect();
								try_to_connect_count++;
							}
						}
						if(!h_pipe) {
							// @todo @log
							do_use_local_func = true;
						}
						else {
							result = InternalBlock::SendPrintCommandToServer(cli, h_pipe, rBlk, rReply);
							::CloseHandle(h_pipe);
						}
					}
				}
				else {
					assert(!do_use_local_func);
					do_use_local_func = true;
				}
			}
		}
	}
	if(do_use_local_func) {
		result = CrystalReportPrint2_Local(rBlk, rReply, h_parent_window_for_preview);
	}
	return result;
}
