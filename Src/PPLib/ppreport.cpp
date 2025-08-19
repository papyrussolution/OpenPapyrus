// PPREPORT.CPP
// Copyright (C) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
#include <crpe2.h>

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
//
// Закомментировать, если немодальный предварительный просмотр печати будет сбоить
//
//#define MODELESS_REPORT_PREVIEW

#define BODY_DBF_NAME "rpt_body.dbf"
#define VAR_DBF_NAME  "rpt_var.dbf"
//
// Prototype
int  FASTCALL CopyDataStruct(const char *pSrc, const char *pDest, const char *pFileName);
int  SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName);
//
//
//
static bool FindExeByExt2(const char * pExt, SString & rResult, const char * pAddedSearchString)
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
PPReportEnv::PPReportEnv() : Sort(0), PrnFlags(0)
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
static TVRez * PPOpenReportResource()
{
	SString path;
	makeExecPathFileName("PPRPT", "RES", path);
	TVRez * p_rez = new TVRez(path, 0 /* useIndex */);
	if(p_rez == 0)
		PPSetErrorNoMem();
	else if(p_rez->error) {
		ZDELETE(p_rez);
		PPSetErrorSLib();
	}
	return p_rez;
}

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

int SReport::defaultIterator(int)
{
	return 0;
}

SReport::SReport(const char * pName)
{
	THISZERO();
	Name = pName;
	iterator   = defaultIterator;
	P_Prn    = 0;
	PageLen  = 0;
	LeftMarg = 0;
	PrnOptions = SPRN_EJECTAFTER;
	PrnDest = 0;
}

SReport::SReport(uint rezID, long flags)
{
	THISZERO();
	iterator = defaultIterator;
	if(rezID > 1000) {
		PPReportStub stub;
		if(PPLoadReportStub(rezID, &stub)) {
			Name = stub.Name;
			DataName = stub.DataName;
		}
		else
			Error = 1;
	}
	else {
		TVRez * p_rez = PPOpenReportResource();
		if(p_rez) {
			if(readResource(p_rez, rezID) == 0) {
				fldCount = 0;
				Error = 1;
			}
			delete p_rez;
		}
		else
			Error = 1;
	}
	NumCopies = 1;
	if(!Error) {
		THROW_MEM(P_Prn = new SPrinter);
		P_Prn->pgl      = PageLen;
		P_Prn->leftMarg = LeftMarg;
		P_Prn->options  = PrnOptions;
		if(flags & INIREPF_FORCELANDSCAPE) {
			P_Prn->pgl = 44;
			P_Prn->options |= SPRN_LANDSCAPE;
			P_Prn->options &= ~SPRN_CONDS;
		}
		if(P_Prn->pgl == 0)
			P_Prn->options &= ~SPRN_EJECTAFTER;
	}
	CATCH
		ZDELETE(P_Prn);
	ENDCATCH
}

SReport::~SReport()
{
	delete fields;
	for(int grpidx = 0; grpidx < grpCount; grpidx++) {
		delete groups[grpidx].fields;
		delete groups[grpidx].lastval;
	}
	delete agrs;
	delete groups;
	for(int bandidx = 0; bandidx < bandCount; bandidx++)
		delete bands[bandidx].fields;
	delete bands;
	delete P_Prn;
	SAlloc::F(P_Text);
}

bool SReport::IsValid() const { return !Error; }

int SReport::createDataFiles(const char * pDataName, const char * pRptPath)
{
	int    ok = -1;
	DlRtm * p_rtm = 0;
	if(!isempty(pDataName)) {
		SString path(pRptPath);
		path.SetLastSlash().Cat(pDataName);
		DlContext ctx;
		DlRtm::ExportParam ep;
		THROW(ctx.InitSpecial(DlContext::ispcExpData));
		THROW(ctx.CreateDlRtmInstance(pDataName, &p_rtm));
		ep.DestPath = path;
		ep.Flags |= DlRtm::ExportParam::fForceDDF;
		THROW(p_rtm->Export(ep));
	}
	else if(getDataName().NotEmpty()) {
		DlContext ctx;
		DlRtm::ExportParam ep;
		THROW(ctx.InitSpecial(DlContext::ispcExpData));
		THROW(ctx.CreateDlRtmInstance(getDataName(), &p_rtm));
		ep.Flags |= DlRtm::ExportParam::fForceDDF;
		THROW(p_rtm->Export(ep));
	}
	/* @v12.3.9 @obsolete else {
		SString fname;
		SCollection fld_ids(aryPtrContainer);
		createBodyDataFile(fname, &fld_ids);
		createVarDataFile(fname, &fld_ids);
	}*/
	ok = 1;
	CATCHZOK
	delete p_rtm;
	return ok;
}

int SReport::readResource(TVRez * rez, uint resID)
{
	int    ok = 1;
	int16  i, j;
	int16  lw, hw;
	int16  len;
	SString msg_buf;
	THROW(rez && !rez->error);
	SLS.SetError(SLERR_REZNFOUND, msg_buf.Z().Cat(resID));
	THROW_SL(rez->findResource(resID, TV_REPORT));
	rez->getString(Name, 0);
	rez->getString(DataName, 0);
	main_id = rez->getUINT();
	main_id = (main_id << 16) | rez->getUINT();
	TextLen = len = rez->getUINT();
	if(len)
		len++;
	THROW_MEM(P_Text = static_cast<char *>(SAlloc::R(P_Text, len)));
	for(i = 0; i < (len / 2); i++)
		reinterpret_cast<int16 *>(P_Text)[i] = rez->getUINT();
	fldCount = rez->getUINT();
	THROW_MEM(fields = static_cast<Field *>(SAlloc::R(fields, sizeof(Field) * fldCount)));
	for(i = 0; i < fldCount; i++) {
		fields[i].id      = rez->getUINT();
		fields[i].name    = rez->getUINT(); // @
		fields[i].type    = rez->getUINT();
		lw        = rez->getUINT();
		hw        = rez->getUINT();
		fields[i].format  = MakeLong(lw, hw);
		fields[i].fldfmt  = rez->getUINT();
		lw        = rez->getUINT();
		hw        = rez->getUINT();
		fields[i].offs    = MakeLong(lw, hw);
		fields[i].lastval = 0;
	}
	if((agrCount = rez->getUINT()) != 0) {
		THROW_MEM(agrs = static_cast<Aggr *>(SAlloc::R(agrs, sizeof(Aggr) * agrCount)));
		for(i = 0; i < agrCount; i++) {
			Aggr * a = &agrs[i];
			a->fld   = rez->getUINT();
			a->aggr  = rez->getUINT();
			a->dpnd  = rez->getUINT();
			a->scope = rez->getUINT();
			a->ptemp = 0;
		}
	}
	else
		ZFREE(agrs);
	if((grpCount = rez->getUINT()) != 0) {
		THROW_MEM(groups = static_cast<Group *>(SAlloc::R(groups, sizeof(Group) * grpCount)));
		for(i = 0; i < grpCount; i++) {
			Group * g = &groups[i];
			g->band   = rez->getUINT();
			if((len = rez->getUINT()) != 0) {
				THROW_MEM(g->fields = static_cast<int16 *>(SAlloc::M(sizeof(int16) * (len+1))));
				g->fields[0] = len;
				for(j = 1; j <= len; j++)
					g->fields[j] = (int16)rez->getUINT();
			}
			else
				ZFREE(g->fields);
			g->lastval = 0;
		}
	}
	else
		ZFREE(groups);
	if((bandCount = rez->getUINT()) != 0) {
		THROW_MEM(bands = static_cast<Band *>(SAlloc::R(bands, sizeof(Band) * bandCount)));
		for(i = 0; i < bandCount; i++) {
			Band * b   = &bands[i];
			b->kind    = rez->getUINT();
			b->ht      = rez->getUINT();
			b->group   = rez->getUINT();
			b->options = rez->getUINT();
			if((len = rez->getUINT()) != 0) {
				THROW_MEM(b->fields = static_cast<int16 *>(SAlloc::M(sizeof(int16) * (len+1))));
				b->fields[0] = len;
				for(j = 1; j <= len; j++)
					b->fields[j] = static_cast<int16>(rez->getUINT());
			}
			else
				ZFREE(b->fields);
		}
	}
	else
		ZFREE(bands);
	PageLen    = static_cast<int>(rez->getUINT());
	LeftMarg   = static_cast<int>(rez->getUINT());
	PrnOptions = static_cast<int>(rez->getUINT());
	CATCHZOK
	return ok;
}

int SReport::setPrinter(SPrinter * p)
{
	P_Prn = p;
	return 1;
}

int SReport::setData(int i, void *d)
{
	i--;
	if(i >= 0 && i < fldCount && fields[i].type) {
		fields[i].data = d;
		return 1;
	}
	return 0;
}

void SReport::disableGrouping()
{
	PrnOptions |= SPRN_SKIPGRPS;
}

int SReport::skipField(int i, int skip)
{
	i--;
	if(i >= 0 && i < fldCount) {
		SETFLAG(fields[i].fldfmt, FLDFMT_SKIP, skip);
		return 1;
	}
	else
		return 0;
}

int SReport::check()
{
	return 1;
}

int SReport::calcAggr(int grp, int mode)
{
	double dd = 0.0;
	for(int i = 0; i < agrCount; i++) {
		Aggr  * agr = &agrs[i];
		Field * df  = agr->dpnd ? &fields[agr->dpnd - 1] : 0;
		if(mode == 1 && df && df->data) {
			if(!stcast(df->type, MKSTYPE(S_FLOAT, 8), df->data, &dd, 0))
				dd = 0;
		}
		else if(mode == 0 && agr->fld > 0 && agr->fld <= fldCount)
			fields[agr->fld - 1].type = MKSTYPE(S_FLOAT, 8);
		if(mode == 1 || agr->scope == grp) {
			switch(agr->aggr) {
				case AGGRFUNC_COUNT:
					if(mode == 0)
						agr->rtemp = 0;
					else if(mode == 1)
						agr->rtemp += 1;
					break;
				case AGGRFUNC_SUM:
					if(mode == 0)
						agr->rtemp = 0;
					else if(mode == 1)
						agr->rtemp += dd;
					break;
				case AGGRFUNC_AVG:
					switch(mode) {
						case 0:
							agr->ptemp = new double[2];
							if(agr->ptemp)
								agr->ptemp[0] = agr->ptemp[1] = 0;
							break;
						case 1:
							if(agr->ptemp) {
								agr->ptemp[0] += 1;
								agr->ptemp[1] += dd;
							}
							break;
						case 2:
							dd = (agr->ptemp && agr->ptemp[0] != 0.0) ? (agr->ptemp[1] / agr->ptemp[0]) : 0.0;
							delete agr->ptemp;
							agr->rtemp = dd;
							break;
					}
					break;
				case AGGRFUNC_MIN:
					if(mode == 0)
						agr->rtemp = SMathConst::Max;
					else if(mode == 1)
						if(dd < agr->rtemp)
							agr->rtemp = dd;
					break;
				case AGGRFUNC_MAX:
					if(mode == 0)
						agr->rtemp = -SMathConst::Max;
					else if(mode == 1)
						if(dd > agr->rtemp)
							agr->rtemp = dd;
					break;
				default:
					agr->rtemp = 0;
					break;
			}
			if(mode == 2)
				setData(agr->fld, &agr->rtemp);
		}
	}
	return 1;
}

static const int hdr_band_types[] = { RPT_HEAD, RPT_FOOT, PAGE_HEAD, PAGE_FOOT };
static const int row_band_types[] = { DETAIL_BODY, GROUP_HEAD, GROUP_FOOT };
//
//
//
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
	const uint16 cr_eng_ver = PEGetVersion(PE_GV_ENGINE);
	if(HiByte(cr_eng_ver) >= 10) {
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
	const uint16 cr_dll_ver = PEGetVersion(PE_GV_DLL);
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
					std_ini_file.GetEntries(section_name, &std_ss);
				}
				if(std_ss.getCount() == 0) {
					(section_name = ReportName).Strip();
					std_ini_file.GetEntries(section_name, &std_ss);
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
					ini_file.GetEntries(section_name, &ss);
				}
				if(ss.getCount() == 0) {
					(section_name = ReportName).Strip();
					ini_file.GetEntries(section_name, &ss);
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
			bool   silent = false;
			LoadExportOptions(Data.ReportName, 0, &silent, temp_buf.Z());
			EnableEMail = 1;
		}
		{
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
					SetupStrAssocCombo(this, CTLSEL_PRINT2_EXPFMT, expfmt_list, Data.ExpParam.Format, 0);
				}
			}
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
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PRINT2_REPORT) || event.isClusterClk(CTL_PRINT2_ACTION)) {
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
		uint   id = static_cast<uint>(getCtrlLong(CTLSEL_PRINT2_REPORT));
		bool   enable_email = false;
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

int  EditPrintParam(PrnDlgAns * pData) { return PPDialogProcBody <Print2Dialog, PrnDlgAns> (pData); }
void SReport::setNumCopies(int n) { NumCopies = (n > 0 && n <= 10) ? n : 1; }
int  SReport::getNumCopies() const { return NumCopies; }

static SString & GetTempFileName_(const char * pFileName, SString & rDest)
{
	GetKnownFolderPath(UED_FSKNOWNFOLDER_TEMPORARY, rDest);
	return rDest.SetLastSlash().Cat(pFileName);
}

#if 0 // @v12.3.9 @obsolete {
int SReport::createBodyDataFile(SString & rFileName, SCollection * fldIDs)
{
	int    ok = 1;
	int    i;
	Band * b;
	page = line = 1;
	Field * f = 0;
	DBFCreateFld * flds=0;
	SString dbfname;
	int    flds_count = 0;
	fldIDs->freeAll();
	GetTempFileName_(BODY_DBF_NAME, dbfname);
	rFileName = dbfname;
	DbfTable  * dbf = new DbfTable(dbfname);
	Field    ** dbf_fields_id = static_cast<Field **>(SAlloc::M(sizeof(void *)));
	for(size_t type = 0; type < SIZEOFARRAY(row_band_types); type++) {
		f = 0;
		for(b = searchBand(row_band_types[type], 0); enumFields(&f, b, &i);) {
			int    used = 0;
			const char * p_fld_name = P_Text+f->name;
			if(f->type != 0) {
				for(int di = 0; !used && di < flds_count; di++)
					if(sstreq(flds[di].Name, p_fld_name))
						used = 1;
				if(!used) {
					flds = static_cast<DBFCreateFld *>(SAlloc::R(flds, sizeof(DBFCreateFld)*(1+flds_count)));
					fldIDs->insert(f);
					TYPEID tp = GETSTYPE(f->type);
					switch(tp) {
						case S_INT:
							flds[flds_count].Init(p_fld_name, 'N', GETSSIZE(f->type), 0);
							break;
						case S_FLOAT:
							flds[flds_count].Init(p_fld_name, 'N', 20, 6);
							break;
						case S_DEC:
						case S_MONEY:
							flds[flds_count].Init(p_fld_name, 'N', 20, GETSPRECD(f->type));
							break;
						case S_DATE:
							flds[flds_count].Init(p_fld_name, 'D', 8, 0);
							break;
						case S_TIME:
							flds[flds_count].Init(p_fld_name, 'C', 12, 0);
							break;
						case S_ZSTRING:
							flds[flds_count].Init(p_fld_name, 'C', GETSSIZE(f->type), 0);
							break;
						default:
							flds[flds_count].Init(p_fld_name, 0, 0, 0);
							break;
					}
					flds_count++;
				}
			}
		}
	}
	if(flds_count == 0) {
		flds = static_cast<DBFCreateFld *>(SAlloc::R(flds, sizeof(DBFCreateFld)));
		flds[flds_count].Init("dummy", 'C', 5, 0);
		fldIDs->insert(f);
		flds_count++;
	}
	for(int gi = 0; gi < grpCount; gi++) {
		Group * g = &groups[gi];
		for(int gj = 1; gj <= g->fields[0]; gj++) {
			int    used = 0;
			f = & fields[g->fields[gj]-1];
			const char * p_fld_name = P_Text+f->name;
			for(int di = 0; di < flds_count; di++)
				if(sstreq(p_fld_name, flds[di].Name)) {
					used = 1;
					break;
				}
			if(!used) {
				flds = static_cast<DBFCreateFld *>(SAlloc::R(flds, sizeof(DBFCreateFld)*(1+flds_count)));
				flds[flds_count].Init(p_fld_name, 'C', SFMTLEN(f->format), 0);
				fldIDs->insert(f);
				flds_count++;
			}
		}
	}
	dbf->create(flds_count, flds);
	delete dbf;
	delete flds;
	return 1;
}

int SReport::createVarDataFile(SString & rFileName, SCollection * fldIDs)
{
	int    ok = 1, i;
	page = line = 1;
	Field * f = 0;
	DBFCreateFld * flds = 0;
	SString dbfname;
	int    flds_count=0;
	int    type;
	// Creating dbf for variables
	fldIDs->freeAll();
	GetTempFileName_(VAR_DBF_NAME, dbfname);
	rFileName = dbfname;
	DbfTable * dbf = new DbfTable(dbfname);
	flds = new DBFCreateFld[2];
	for(type = 0; type < SIZEOFARRAY(hdr_band_types); type++) {
		f = 0;
		for(Band * b = searchBand(hdr_band_types[type], 0); enumFields(&f, b, &i);) {
			int    used = 0;
			const char * p_fld_name = P_Text+f->name;
			if(f->type != 0) {
				for(int di = 0; !used && di < flds_count; di++)
					if(sstreq(flds[di].Name, p_fld_name))
						used = 1;
				if(!used) {
					int    fl = 0;
					flds = static_cast<DBFCreateFld *>(SAlloc::R(flds,sizeof(DBFCreateFld)*(1+flds_count)));
					if(GETSTYPE(f->type) == S_ZSTRING) {
						fl = GETSSIZE(f->type);
						fl = (fl > 0 && fl < 256) ? fl : 255;
					}
					else
						fl = SFMTLEN(f->format);
					flds[flds_count].Init(p_fld_name, 'C', fl, 0);
					fldIDs->insert(f);
					flds_count++;
				}
			}
		}
	}
	if(flds_count == 0) {
		flds = static_cast<DBFCreateFld *>(SAlloc::R(flds, sizeof(DBFCreateFld)));
		flds[flds_count].Init("dummy", 'C', 5, 0);
		fldIDs->insert(f);
		flds_count++;
	}
	dbf->create(flds_count, flds);
	delete dbf;
	delete [] flds;
	return 1;
}

int SReport::prepareData()
{
	page = line = 1;

	int    ok = 1;
	int    i;
	uint   di;
	Field * f = 0;
	SString dbfname;
	SCollection fld_ids(/*DEFCOLLECTDELTA,*/aryPtrContainer);
	DbfTable * dbf = 0;
	{
		createBodyDataFile(dbfname, &fld_ids);
		dbf = new DbfTable(dbfname);
		DbfRecord rec(dbf);
		calcAggr(-1, 0);
		if((i = iterator(1)) > 0) {
			do {
				Count++;
				int    fldn = 1;
				for(di = 0; di < fld_ids.getCount(); di++) {
					f = static_cast<Field *>(fld_ids.at(di));
					if(f->type != 0) {
						char   buf[256];
						if(!(PrnOptions & SPRN_SKIPGRPS)) {
							for(int i = 0; i < grpCount; i++) {
								const int c = checkval(groups[i].fields, &groups[i].lastval);
								if(c > 0) {
									// Значение изменилось
									THROW(printGroupHead(GROUP_FOOT, i));
									THROW(printGroupHead(GROUP_HEAD, i));
								}
								else if(c < 0) { // Первая запись
									THROW(printGroupHead(GROUP_HEAD, i));
								}
							}
						}
						if(!(f->fldfmt & FLDFMT_SKIP)) {
							if(f->data) {
								int _s  = GETSSIZE(f->type);
								int _sd = GETSSIZED(f->type);
								switch (GETSTYPE(f->type)) {
									case S_INT:
										if(_s == 1)
											rec.put(di+1, (long)*static_cast<const int8 *>(f->data));
										else if(_s == 2)
											rec.put(di+1, (long)*static_cast<const int16 *>(f->data));
										else if(_s == 4)
											rec.put(di+1, (long)*static_cast<int32 *>(f->data));
										break;
									case S_FLOAT:
										if(_s == 4)
											rec.put(di+1, (double)*static_cast<const float *>(f->data));
										else if(_s == 8)
											rec.put(di+1, *static_cast<const double *>(f->data));
										break;
									case S_DEC:
									case S_MONEY: {
											double _dbl_temp = dectobin(static_cast<const char *>(f->data), (int16)GETSSIZED(f->type),
												(int16)GETSPRECD(f->type));
											rec.put(di+1, _dbl_temp);
										}
										break;
									case S_DATE: {
											int _d, _m, _y;
											DBFDate _dt_temp;
											decodedate(&_d, &_m, &_y, f->data);
											_dt_temp.day   = _d;
											_dt_temp.month = _m;
											_dt_temp.year  = _y;
											rec.put(di+1, &_dt_temp);
										}
										break;
									case S_TIME:
										sttostr(f->type, f->data, f->format, buf);
										rec.put(di+1, buf);
										break;
									case S_ZSTRING:
										rec.put(di+1, static_cast<const char *>(f->data));
										break;
								}
							}
						}
						else {
							memset(buf, ' ', SFMTLEN(f->format));
							buf[SFMTLEN(f->format)] = 0;
							rec.put(di+1,buf);
						}
					}
				}
				if(i != 2)
					calcAggr(-1, 1);
				dbf->appendRec(&rec);
			} while((i = iterator(0)) > 0);
		}
		if(!(PrnOptions & SPRN_SKIPGRPS)) {
			for(i = 0; i < grpCount; i++) {
				THROW(printGroupHead(GROUP_FOOT, i));
			}
		}
		calcAggr(-1, 2);
		dbf->close();
		ZDELETE(dbf);
	}
	{
		createVarDataFile(dbfname, &fld_ids);
		dbf = new DbfTable(dbfname);
		{
			int    fldn = 0;
			char   buf[256];
			DbfRecord rec(dbf);
			for(di = 0; di < fld_ids.getCount(); di++) {
				f = static_cast<Field *>(fld_ids.at(di));
				if(f->type != 0) {
					if(f->data) {
						if(GETSTYPE(f->type) == S_ZSTRING)
							STRNSCPY(buf, static_cast<const char *>(f->data));
						else
							sttostr(f->type, f->data, f->format, buf);
					}
					else {
						memset(buf, ' ', SFMTLEN(f->format));
						buf[SFMTLEN(f->format)] = 0;
					}
					rec.put(di+1, buf);
				}
			}
			dbf->appendRec(&rec);
		}
	}
	CATCHZOK
	delete dbf;
	return ok;
}
#endif // } 0 @v12.3.9 @obsolete

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
		const  char * p_fname = (locN == 0) ? BODY_DBF_NAME : VAR_DBF_NAME;
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

static int RemoveCompName(SString & rPrintDevice)
{
	TCHAR  buf[256];
	SString sbuf;
	DWORD  buf_size = SIZEOFARRAY(buf);
	buf[0] = 0;
	GetComputerNameEx(ComputerNameNetBIOS, buf, &buf_size);
	(sbuf = "\\\\").Cat(SUcSwitch(buf)).BSlash();
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

static int SetPrinterParam(short hJob, const char * pPrinter, long options, const DEVMODEA *pDevMode)
{
	int    ok = 1;
	SString print_device(isempty(pPrinter) ? DS.GetConstTLA().PrintDevice : pPrinter);
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
	THROW_SL(ExpParam.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
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
	
int CrystalReportPrint2(const CrystalReportPrintParamBlock & rBlk, CrystalReportPrintReply & rReply) // @v11.9.5 @construction
{
	int    ok = 1;
	short  h_job = 0;
	bool   do_zero_print_device_on_epilog = false;
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
					DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions-own.log"); // @debug
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
					DebugOutputCrrExportOptions(eo, "debug-ppreport-expoptions.log"); // @debug
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
					PPAlbatrossConfig alb_cfg;
					if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0) {
						if(alb_cfg.Hdr.MailAccID) {
							if(SendMailWithAttach(rBlk.ReportName, /*path*/p__dest_fn, rBlk.ReportName, rBlk.EmailAddr, alb_cfg.Hdr.MailAccID)) {
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
	}
	else {
		SString inner_printer_buf;
		const char * p_inner_printer = 0;
		int    num_copies = rBlk.NumCopies;
		const  DEVMODEA * p_dev_mode = (rBlk.InternalFlags & CrystalReportPrintParamBlock::intfDevModeValid) ? &rBlk.DevMode : 0;
		if(p_dev_mode) {
			inner_printer_buf = reinterpret_cast<const char *>(p_dev_mode->dmDeviceName);
			if(inner_printer_buf.NotEmptyS())
				p_inner_printer = inner_printer_buf;
			num_copies = (p_dev_mode->dmCopies > 0 && p_dev_mode->dmCopies <= 1000) ? p_dev_mode->dmCopies : 1;
		}
		else
			p_inner_printer = rBlk.Printer;
		h_job = PEOpenPrintJob(rBlk.ReportPath);
		THROW_PP(h_job, PPERR_CRYSTAL_REPORT);
		PEGetReportOptions(h_job, &ro);
		ro.morePrintEngineErrorMessages = FALSE;
		PESetReportOptions(h_job, &ro);
		do_zero_print_device_on_epilog = (DS.GetConstTLA().PrintDevice.IsEmpty() && GetWindowsPrinter(0, &DS.GetTLA().PrintDevice) > 0);
		THROW(SetPrinterParam(h_job, p_inner_printer, rBlk.Options, p_dev_mode));
		THROW(SetupReportLocations(h_job, rBlk.Dir, (rBlk.Options & SPRN_DONTRENAMEFILES) ? 0 : 1));
		if(rBlk.Action == CrystalReportPrintParamBlock::actionPreview/*rBlk.Options & SPRN_PREVIEW*/) {
			THROW_PP(PEOutputToWindow(h_job, "", CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT, WS_MAXIMIZE | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU, 0), PPERR_CRYSTAL_REPORT);
		}
		else {
			THROW_PP(PEOutputToPrinter(h_job, num_copies), PPERR_CRYSTAL_REPORT);
		}
		if(rBlk.Options & SPRN_SKIPGRPS)
			SetupGroupSkipping(h_job);
		if(/**rBlk.Options & SPRN_PREVIEW ||*/rBlk.Action == CrystalReportPrintParamBlock::actionPreview) {
			struct PreviewEventParam {
				static BOOL CALLBACK EventCallback(short eventID, void * pParam, void * pUserData)
				{
					if(eventID == PE_CLOSE_PRINT_WINDOW_EVENT) {
	#ifndef MODELESS_REPORT_PREVIEW
						EnableWindow(APPL->H_TopOfStack, 1);
						static_cast<PreviewEventParam *>(pUserData)->StopPreview++;
	#endif
					}
					return TRUE;
				}
				int    StopPreview;
			};
			PreviewEventParam pep;
			pep.StopPreview = 0;
			PEEnableEventInfo eventInfo;
			eventInfo.StructSize = sizeof(PEEnableEventInfo);
			eventInfo.closePrintWindowEvent = TRUE;
			eventInfo.startStopEvent = TRUE;
			THROW_PP(PEEnableEvent(h_job, &eventInfo), PPERR_CRYSTAL_REPORT);
			THROW_PP(PESetEventCallback(h_job, PreviewEventParam::EventCallback, &pep), PPERR_CRYSTAL_REPORT);
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
				msg_buf.Z().CatEq("Report", rBlk.ReportPath);
				if(!isempty(p_inner_printer)) // Ранее был pPrinter @erik v10.4.10
					msg_buf.CatDiv(';', 2).CatEq("Printer", p_inner_printer); // Ранее был pPrinter @erik v10.4.10
				if(num_copies > 1)
					msg_buf.CatDiv(';', 2).CatEq("Copies", num_copies);
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
	if(do_zero_print_device_on_epilog)
		DS.GetTLA().PrintDevice.Z();
	return ok;
}

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
		struct PreviewEventParam {
			static BOOL CALLBACK EventCallback(short eventID, void * pParam, void * pUserData)
			{
				if(eventID == PE_CLOSE_PRINT_WINDOW_EVENT) {
#ifndef MODELESS_REPORT_PREVIEW
					EnableWindow(APPL->H_TopOfStack, 1);
					static_cast<PreviewEventParam *>(pUserData)->StopPreview++;
#endif
				}
				return TRUE;
			}
			int    StopPreview;
		};
		PreviewEventParam pep;
		pep.StopPreview = 0;
		PEEnableEventInfo eventInfo;
		eventInfo.StructSize = sizeof(PEEnableEventInfo);
		eventInfo.closePrintWindowEvent = TRUE;
		eventInfo.startStopEvent = TRUE;
		THROW_PP(PEEnableEvent(h_job, &eventInfo), PPERR_CRYSTAL_REPORT);
		THROW_PP(PESetEventCallback(h_job, PreviewEventParam::EventCallback, &pep), PPERR_CRYSTAL_REPORT);
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
						if(SendMailWithAttach(pReportName, /*path*/p__dest_fn, pReportName, pEMailAddr, alb_cfg.Hdr.MailAccID)) {
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

SReport::Band * SReport::searchBand(int kind, int grp)
{
	Band * b = 0;
	for(int i = 0; i < bandCount && b == 0; i++)
		if(bands[i].kind == kind && bands[i].group == grp)
			b = bands + i;
	return b;
}

int SReport::printDataField(SReport::Field * f)
{
	char   buf[256];
	if(f->fldfmt & FLDFMT_SKIP)
		return 1;
	if(f->data)
		sttostr(f->type, f->data, f->format, buf);
	else {
		memset(buf, ' ', SFMTLEN(f->format));
		buf[SFMTLEN(f->format)] = 0;
	}
	return P_Prn->printLine(buf, 0);
}

int SReport::enumFields(SReport::Field ** f, SReport::Band * b, int *i)
{
	if(b && b->fields) {
		if(*f == 0)
			*i = 1;
		if(*i <= b->fields[0]) {
			*f = fields + b->fields[(*i)++] - 1;
			return 1;
		}
	}
	return 0;
}

int SReport::printPageHead(int kind, int _newpage)
{
	int     ok = 1, i;
	Band  * b;
	Field * f = 0;
	if(kind == PAGE_HEAD && _newpage) {
		THROW(P_Prn->endPage());
		THROW(P_Prn->startPage());
		page++;
		line = 1;
	}
	b = searchBand(kind, 0);
	while(enumFields(&f, b, &i)) {
		if(f->type == 0) {
			const char * p, * t;
			p = t = P_Text+f->offs;
			while((p = sstrchr(p, '\n')) != 0)
				if(P_Prn->pgl == 0 || line < (P_Prn->pgl - PageFtHt)) {
					p++;
					line++;
				}
				else {
					THROW(P_Prn->printLine(t, 0));
					t = ++p;
					if(kind == PAGE_HEAD) {
						SString msg_buf;
						PPLoadText(PPTXT_PAGELENTOOSMALL, msg_buf);
						P_Prn->printLine(msg_buf, 0);
						return 0;
					}
					line++;
					break;
				}
			THROW(P_Prn->printLine(t, 0));
		}
		else {
			THROW(printDataField(f));
		}
	}
	CATCHZOK
	return ok;
}

int SReport::printGroupHead(int kind, int grp)
{
	if(searchBand(kind, grp))
		if(kind == GROUP_HEAD)
			calcAggr(grp, 0);
		else if(kind == GROUP_FOOT)
			calcAggr(grp, 2);
	return 1;
}

int SReport::checkval(int16 *flds, char **ptr)
{
	int     i;
	int     r;
	uint    s;
	uint    ofs;
	TYPEID  t;
	Field * f;
	if(*ptr == 0) {
		if(flds && flds[0]) {
			r = -1;
			s = 0;
			for(i = 1; i <= flds[0]; i++)
				s += stsize(fields[flds[i] - 1].type);
			(*ptr) = static_cast<char *>(SAlloc::M(s));
		}
		else
			r = 0;
	}
	else if(flds && flds[0]) {
		for(ofs = r = 0, i = 1; i <= flds[0] && r == 0; i++) {
			f = fields + flds[i] - 1;
			t = f->type;
			if(f->data && stcomp(t, f->data, (*ptr) + ofs))
				r = 1;
			else
				ofs += stsize(t);
		}
	}
	else
		r = 0;
	if(r && *ptr) {
		for(ofs = 0, i = 1; i <= flds[0]; i++) {
			f = fields + flds[i] - 1;
			s = stsize(f->type);
			if(f->data)
				memcpy((*ptr) + ofs, f->data, s);
			else
				memzero((*ptr) + ofs, s);
			ofs += s;
		}
	}
	return r;
}

int SReport::printDetail()
{
	int     ok = 1;
	int     c;
	Band  * b;
	Field * f = 0;
	if(!(PrnOptions & SPRN_SKIPGRPS)) {
		for(int i = 0; i < grpCount; i++) {
			if((c = checkval(groups[i].fields, &groups[i].lastval)) > 0) {
				// Значение изменилось
				THROW(printGroupHead(GROUP_FOOT, i));
				THROW(printGroupHead(GROUP_HEAD, i));
			}
			else if(c < 0) { // Первая запись
				THROW(printGroupHead(GROUP_HEAD, i));
			}
		}
	}
	{
		int    i = 0;
		for(b = searchBand(DETAIL_BODY, 0); enumFields(&f, b, &i);) {
			if(f->type == 0) {
				const char * p = P_Text+f->offs;
				const char * t = p;
				while((p = sstrchr(p, '\n')) != 0) {
					if(P_Prn->pgl == 0 || line < (P_Prn->pgl - PageFtHt)) {
						p++;
						line++;
					}
					else {
						THROW(P_Prn->printLine(t, 0));
						THROW(printPageHead(PAGE_FOOT, 0));
						THROW(printPageHead(PAGE_HEAD, 1));
						t = ++p;
						line++;
					}
				}
				THROW(P_Prn->printLine(t, 0));
			}
			else {
				THROW(printDataField(f));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SReport::printTitle(int kind)
{
	int     ok = 1, i;
	Band  * b = searchBand(kind, 0);
	Field * f = 0;
	if(b && b->fields) {
		if(b->options & GRPFMT_NEWPAGE) {
			THROW(P_Prn->endPage());
			THROW(P_Prn->startPage());
			page++;
			line = 1;
		}
		while(enumFields(&f, b, &i)) {
			if(f->type == 0) {
				const char * p = P_Text+f->offs;
				const char * t = p;
				while((p = sstrchr(p, '\n')) != 0) {
					if(P_Prn->pgl == 0 || line < P_Prn->pgl) {
						p++;
						line++;
					}
					else {
						THROW(P_Prn->printLine(t, p-t));
						t = ++p;
						line = 1;
						page++;
					}
				}
				THROW(P_Prn->printLine(t, 0));
			}
			else {
				THROW(printDataField(f));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SReport::getFieldName(SReport::Field * pFld, char * buf, size_t buflen)
{
	if(pFld->name >= 0) {
		strnzcpy(buf, P_Text + pFld->name, buflen);
		return 1;
	}
	else if(buf)
		buf[0] = 0;
	return -1;
}

int SReport::getFieldName(int i, char * buf, size_t buflen)
{
	return (i >= 0 && i < fldCount) ? getFieldName(&fields[i], buf, buflen) : 0;
}
//
//	@Vadim 13.09.02
//
struct SvdtStrDlgAns { // @{savereportdata}
	int   SvDt;
	int   EdRep;
	SString SvDtPath_;
	SString EdRepPath_;
};

static int GetSvdtStrOpt(SvdtStrDlgAns * pSsda) 
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

int SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName)
{
	int    ok = -1;
	SString path, fname;
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
//
//
//
int EditDefaultPrinterCfg()
{
	int    ok = 1;
	ushort v = 0;
	PPID   loc_prn_id = 0;
	PPPrinterCfg cfg;
	TDialog  * dlg = new TDialog(DLG_PRNCFG);
	THROW(CheckDialogPtr(&dlg));
	THROW(PPGetPrinterCfg(PPOBJ_CONFIG, PPCFG_MAIN, &cfg));
	SetupStringCombo(dlg, CTLSEL_PRNCFG_PRINTER, PPTXT_PRNTYPE, cfg.PrnCmdSet);
	GetWindowsPrinter(&loc_prn_id, 0);
	SetupPPObjCombo(dlg, CTLSEL_PRNCFG_WINPRINTER, PPOBJ_LOCPRINTER, loc_prn_id, 0, 0);
	dlg->setCtrlData(CTL_PRNCFG_PORT, cfg.Port);
	dlg->AddClusterAssoc(CTL_PRNCFG_FLAGS, 0, PPPrinterCfg::fUseDuplexPrinting);
	dlg->AddClusterAssoc(CTL_PRNCFG_FLAGS, 1, PPPrinterCfg::fStoreLastSelPrn);
	dlg->SetClusterData(CTL_PRNCFG_FLAGS, cfg.Flags);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_PRNCFG_PRINTER, &cfg.PrnCmdSet);
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

int MakeCRptDataFiles(int verifyAll /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_MKRPTFLS);
	if(CheckDialogPtrErr(&dlg)) {
		SString rpt_name, rpt_path, fname;
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_MKRPTFLS_RPTPATH, CTL_MKRPTFLS_RPTPATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		PPGetPath(PPPATH_REPORTDATA, rpt_path);
		if(verifyAll == 1) {
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
				uint      pos = 0;
				SReport rpt(static_cast<const char *>(0));
				StringSet ss;
				SStrCollection ss_col;
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
				ss_col.freeAll();
				for(pos = 0; ss.get(&pos, rpt_name);) {
					PrnDlgAns report_descr_data(rpt_name);
					uint   i;
					report_descr_data.SetupReportEntries(0);
					report_descr_data.Flags |= PrnDlgAns::fForceDDF; // Создание файлов данных обязательно должно включать создание словаря //
					for(i = 0; i < report_descr_data.Entries.getCount(); i++) {
						uint   p = 0;
						char * p_data_name = newStr(report_descr_data.Entries.at(i)->DataName_);
						THROW_MEM(p_data_name);
						PPWaitMsg(p_data_name);
						if(verifyAll && p_data_name[0])
							(data_path = rpt_path).SetLastSlash().Cat(p_data_name);
						if(p_data_name[0] && !ss_col.lsearch(p_data_name, &p, PTR_CMPFUNC(PcharNoCase)) && rpt.createDataFiles(p_data_name, rpt_path) > 0) {
							THROW_SL(ss_col.insert(p_data_name));
							if(verifyAll)
								VerifyCrpt(report_descr_data.Entries.at(i)->ReportPath_, data_path);
						}
						else
							delete p_data_name;
					}
				}
				ok = 1;
			}
			PPWaitStop();
			if(verifyAll == 1)
				break;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

static int FASTCALL __PPAlddPrint(int rptId, PPFilt * pF, int isView, const PPReportEnv * pEnv)
{
	MemLeakTracer mlt;
	int    ok = 1;
	bool   inherited_tbl_names = false;
	bool   diffidbyscope = false;
	DlRtm   * p_rtm = 0;
	SString data_name, fn;
	SString printer_name;
	SString temp_buf;
	long   fl = 0;
	const  uint sur_key_last_count = DS.GetTLA().SurIdList.getCount(); // См. коммент в конце функции
	SETFLAG(fl, INIREPF_FORCELANDSCAPE, pEnv && pEnv->PrnFlags & SReport::Landscape);
	SETFLAG(fl, INIREPF_NOSHOWDIALOG, pEnv && pEnv->PrnFlags & SReport::PrintingNoAsk);
	SReport rpt(rptId, fl);
	THROW(rpt.IsValid());
	{
		PrnDlgAns pans(rpt.Name);
		const ReportDescrEntry * p_sel_entry = 0;
		if(pEnv) {
			pans.DefPrnForm = pEnv->DefPrnForm;
			pans.EmailAddr = pEnv->EmailAddr;
			if(pEnv->PrnFlags & SReport::DisableGrouping)
				rpt.disableGrouping();
			if(pEnv->PrnFlags & SReport::FooterOnBottom) {
				SReport::Band * p_rb = rpt.searchBand(RPT_FOOT, 0);
				if(p_rb)
					p_rb->options |= GRPFMT_SUMMARYONBOTTOM;
			}
			if(pEnv->PrnFlags & SReport::PrintingNoAsk && pans.SetupReportEntries(pEnv->ContextSymb) > 0) {
				const ReportDescrEntry * p_entry = pans.Entries.at(0);
				fn = p_entry->ReportPath_;
				data_name = p_entry->DataName_;
				rpt.PrnDest = (pEnv->PrnFlags & SReport::XmlExport) ? PrnDlgAns::aExportXML : PrnDlgAns::aPrint;
				diffidbyscope = LOGIC(p_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
			}
		}
		else
			pans.DefPrnForm.Z();
		if(!rpt.PrnDest) {
			if(EditPrintParam(&pans) > 0) {
				if(pans.P_DevMode) {

				}
				pans.Flags &= ~pans.fForceDDF;
				p_sel_entry = pans.Entries.at(pans.Selection);
				// @v11.2.8 fn = p_sel_entry->ReportPath_;
				SFsPath::NormalizePath(p_sel_entry->ReportPath_, SFsPath::npfCompensateDotDot, fn); // @v11.2.8 
				data_name     = p_sel_entry->DataName_;
				inherited_tbl_names = LOGIC(p_sel_entry->Flags & ReportDescrEntry::fInheritedTblNames);
				diffidbyscope = LOGIC(p_sel_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
				rpt.PrnDest   = pans.Dest;
				printer_name  = pans.Printer;
				SETFLAG(rpt.PrnOptions, SPRN_USEDUPLEXPRINTING, BIN(pans.Flags & PrnDlgAns::fUseDuplexPrinting));
			}
			else
				ok = -1;
		}
		// @v11.8.8 {
		else if(rpt.PrnDest == PrnDlgAns::aPrint) {
			if(pEnv)
				printer_name = pEnv->PrnPort;
		}
		// } @v11.8.8 
		if(ok > 0) {
			data_name.SetIfEmpty(rpt.getDataName());
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
				rpt.PrnDest = PrnDlgAns::aExportTDDO;
				Tddo t;
				StringSet ext_param_list;
				int    fld_n = 0;
				SBuffer result;
				THROW(Tddo::LoadFile(fn, temp_buf));
				t.SetInputFileName(fn);
				{
					SString inner_fn;
                	SFsPath ps(fn);
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
			else if(rpt.PrnDest == PrnDlgAns::aExportXML) {
				ep.Cp = DS.GetConstTLA().DL600XmlCp;
				THROW(p_rtm->ExportXML(ep, out_file_name));
				if(pans.Flags & pans.fEMail && pans.EmailAddr.NotEmptyS() && fileExists(out_file_name)) {
					//
					// Отправка на определенный почтовый адрес
					//
					PPAlbatrossConfig alb_cfg;
					THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
					if(alb_cfg.Hdr.MailAccID) {
						THROW(SendMailWithAttach(pans.ReportName, out_file_name, pans.ReportName, pans.EmailAddr, alb_cfg.Hdr.MailAccID));
					}
				}
			}
			else {
				if(oneof2(rpt.PrnDest, PrnDlgAns::aPrepareData, PrnDlgAns::aPrepareDataAndExecCR)) {
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
							const uint16 cr_eng_ver = PEGetVersion(PE_GV_ENGINE);
							if(HiByte(cr_eng_ver) >= 10)
								SFile::RemoveDir(temp_buf);
						}
					}
					ep.Flags &= ~DlRtm::ExportParam::fForceDDF;
				}
				THROW(p_rtm->Export(ep));
			}
			PPWaitStop();
			switch(rpt.PrnDest) {
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
							(temp_buf = cr_path_).Space().Cat(fn);
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
						req.Options = rpt.PrnOptions;
						req.ReportPath = fn;
						req.ReportName = pans.ReportName;
						req.Dir = ep.Path;
						req.Printer = printer_name;
						if(rpt.PrnDest == PrnDlgAns::aExport) {
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
							if(rpt.PrnDest == PrnDlgAns::aPrint) {
								req.Action = CrystalReportPrintParamBlock::actionPrint;
								req.NumCopies = pans.NumCopies;
							}
							else if(rpt.PrnDest == PrnDlgAns::aPreview) {
								req.Action = CrystalReportPrintParamBlock::actionPreview;
							}
						}
						ok = CrystalReportPrint2(req, rep);
					}
					break;
				default:
					ok = -1;
					break;
			}
			if(!ok)
				PPError();
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
	if(pV) {
		THROW(p_rtm = p_ctx->GetRtm(pDataName));
		THROW(p_rtm->PutToJsonBuffer(pV, rBuf.Z(), 0 /* flags */));
	}
	else {
		ok = 0;
	}
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
SString & GetCrr32ProxiPipeName(SString & rBuf) { return rBuf.Z().Cat("\\\\.\\pipe\\").Cat(PPConst::PipeCrr32Proxi); } // @v11.9.5
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
				boolint _ok = SetNamedPipeHandleState(result, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
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
		SJson * p_js_reply = SendCommand(hPipe, "quit");
		if(p_js_reply) {
			ok = true;
		}
		return ok;
	}
	SJson * SendCommand(SIntHandle hPipe, const char * pCmdUtf8)
	{
		SJson * p_js_reply = 0;
		SString temp_buf;
		SString reply_buf;
		THROW(hPipe); // @todo @err
		THROW(!isempty(pCmdUtf8)); // @todo @err
		{
			SJson js_query(SJson::tOBJECT);
			js_query.InsertString("cmd", pCmdUtf8);
			//
			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				reply_buf.Z();
				bool more_data = false;
				do {
					more_data = false;
					DWORD rd_size = 0;
					RdBuf[0] = 0;
					boolint rd_ok = ReadFile(hPipe, RdBuf, RdBuf.GetSize(), &rd_size, NULL/*not overlapped*/);
					if(rd_ok) {
						reply_buf.CatN(RdBuf, rd_size);
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
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

int TestCrr32SupportServer()
{
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString pipe_name;
	GetCrr32ProxiPipeName(pipe_name);
	Crr32SupportClient cli(pipe_name);
	{
		SIntHandle h_pipe = cli.Connect();
		if(!h_pipe) {
			ok = 0;
		}
		else {
			for(uint i = 0; i < 10; i++) {
				//slfprintf_stderr("Call on named-pipe #%u\n", i+1);
				p_js_reply = cli.SendCommand(h_pipe, "test");
				if(p_js_reply) {
					ZDELETE(p_js_reply);
				}
			}
			cli.SendQuitCommand(h_pipe);
		}
	}
	ZDELETE(p_js_reply);
	return ok;
}
