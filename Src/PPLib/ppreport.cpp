// PPREPORT.CPP
// Copyright (C) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
#include <crpe.h>
#include <crpe2.h>
#include <shlwapi.h> // Vadim 03.09.02 - надо подключить shlwapi.lib
//
// Закомментировать, если немодальный предварительный просмотр печати будет сбоить
//
//#define MODELESS_REPORT_PREVIEW

#define BODY_DBF_NAME "rpt_body.dbf"
#define VAR_DBF_NAME  "rpt_var.dbf"
//
// Prototype
int  FASTCALL CopyDataStruct(const char *pSrc, const char *pDest, const char *pFileName);
int  SLAPI SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName);
//
//
//
static int FindExeByExt(const char * pExt, char * pExe, size_t buflen, char * pAddedSearchString)
{
	//
	// @todo Перестроить функцию с использованием WinRegKey
	//
	int    ok = 0;
	char   buf[MAXPATH], * p_chr = 0;
	SString temp_buf;
	DWORD  v_type = REG_SZ ;
	DWORD  bufsize = MAXPATH;
	if(pExe) {
		buf[0] = 0;
		if(SHGetValue(HKEY_CLASSES_ROOT, pExt, NULL, &v_type, buf, &bufsize) == ERROR_SUCCESS) { // @unicodeproblem
			v_type = REG_SZ;
			bufsize = MAXPATH;
			if(pAddedSearchString && stricmp(pAddedSearchString, buf)== 0 &&
				SHGetValue(HKEY_CLASSES_ROOT, buf, NULL, &v_type, buf, &bufsize) == ERROR_SUCCESS) { // @unicodeproblem
				v_type = REG_SZ;
				bufsize = MAXPATH;
				strip(buf);
				while((p_chr = strchr(buf, ' ')) != NULL)
					strcpy(p_chr, p_chr + 1);
			}
			temp_buf.Z().CatChar('\\').Cat("shell").SetLastSlash().Cat("open").SetLastSlash().Cat("command");
			strcat(buf, temp_buf);
			if(SHGetValue(HKEY_CLASSES_ROOT, buf, NULL, &v_type, buf, &bufsize) == ERROR_SUCCESS) { // @unicodeproblem
				strip(buf);
				if(buf[0] == '"') {
					strcpy(buf, buf + 1);
					if((p_chr = strchr(buf, '"')) != NULL)
						strcpy(p_chr, p_chr + 1);
				}
				if((p_chr = strchr(buf, '.')) != NULL)
					if((p_chr = strchr(p_chr, ' ')) != NULL)
						*p_chr = 0;
				if(buflen > sstrlen(buf)) {
					strcpy(pExe, buf);
					ok = 1;
				}
				else
					pExe[0] = 0;
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
SLAPI PrnDlgAns::PrnDlgAns(const char * pReportName) : Dest(0), Selection(0), NumCopies(1), Flags(0), P_ReportName(pReportName), P_DefPrnForm(0)
{
}

SLAPI PrnDlgAns::~PrnDlgAns()
{
}
//
//
//
static TVRez * SLAPI PPOpenReportResource()
{
	SString path;
	long   EXT = 0x00534552L; // "RES"
	makeExecPathFileName("PPRPT", (const char*)&EXT, path);
	TVRez * p_rez = new TVRez(path, 0 /* useIndex */);
	if(p_rez == 0)
		PPSetErrorNoMem();
	else if(p_rez->error) {
		ZDELETE(p_rez);
		PPSetErrorSLib();
	}
	return p_rez;
}

int SLAPI GetReportIDByName(const char * pRptName, uint * pRptID)
{
	int    ok = 0;
	ulong  ofs = 0;
	SString name_buf;
	TVRez::WResHeaderInfo hdr;
	ASSIGN_PTR(pRptID, 0);
#if 0 // @v8.9.8 {
	TVRez * p_rez = PPOpenReportResource();
	if(p_rez)
		for(ofs = 0; !ok && p_rez->readHeader(ofs, &hdr, TVRez::beginOfData); ofs = hdr.Next)
			if(hdr.Type == TV_REPORT && p_rez->getString(name_buf, 0).CmpNC(pRptName) == 0) {
				ASSIGN_PTR(pRptID, hdr.IntID);
				ok = 1;
			}
	delete p_rez;
#endif // } 0 @v8.9.8
	//
	//
	//
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
	long   ID;
	SString Name;
	SString DataName;
	SString Descr;
};

int SLAPI PPLoadReportStub(long rptID, PPReportStub * pData)
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

SLAPI SReport::SReport(const char * pName)
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

SLAPI SReport::SReport(uint rezID, long flags)
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

SLAPI SReport::~SReport()
{
	int    i;
	delete fields;
	for(i = 0; i < grpCount; i++) {
		delete groups[i].fields;
		delete groups[i].lastval;
	}
	delete agrs;
	delete groups;
	for(i = 0; i < bandCount; i++)
		delete bands[i].fields;
	delete bands;
	delete P_Prn;
	SAlloc::F(P_Text);
}

int SLAPI SReport::IsValid() const
{
	return !Error;
}

int SLAPI SReport::createDataFiles(const char * pDataName, const char * pRptPath)
{
	int    ok = -1;
	DlRtm * p_rtm = 0;
	if(!isempty(pDataName)) {
		SString path = pRptPath;
		path.SetLastSlash().Cat(pDataName);
		DlContext ctx;
		DlRtm::ExportParam ep;
		THROW(ctx.InitSpecial(DlContext::ispcExpData));
		THROW(ctx.CreateDlRtmInstance(pDataName, &p_rtm));
		ep.DestPath = path;
		ep.Flags |= DlRtm::ExportParam::fForceDDF;
		THROW(p_rtm->Export(ep));
	}
	else if(getDataName()[0]) {
		DlContext ctx;
		DlRtm::ExportParam ep;
		THROW(ctx.InitSpecial(DlContext::ispcExpData));
		THROW(ctx.CreateDlRtmInstance(getDataName(), &p_rtm));
		ep.Flags |= DlRtm::ExportParam::fForceDDF;
		THROW(p_rtm->Export(ep));
	}
	else {
		SString fname;
		SCollection fld_ids(/*DEFCOLLECTDELTA,*/aryPtrContainer);
		createBodyDataFile(fname, &fld_ids);
		createVarDataFile(fname, &fld_ids);
	}
	ok = 1;
	CATCHZOK
	delete p_rtm;
	return ok;
}

int SLAPI SReport::readResource(TVRez * rez, uint resID)
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
	THROW_MEM(P_Text = (char *)SAlloc::R(P_Text, len));
	for(i = 0; i < (len / 2); i++)
		((int16 *)P_Text)[i] = rez->getUINT();
	fldCount = rez->getUINT();
	THROW_MEM(fields = (Field *)SAlloc::R(fields, sizeof(Field) * fldCount));
	for(i = 0; i < fldCount; i++) {
		fields[i].id      = rez->getUINT();
		fields[i].name    = rez->getUINT(); // @
		fields[i].type    = rez->getUINT();
		lw                = rez->getUINT();
		hw                = rez->getUINT();
		fields[i].format  = MakeLong(lw, hw);
		fields[i].fldfmt  = rez->getUINT();
		lw                = rez->getUINT();
		hw                = rez->getUINT();
		fields[i].offs    = MakeLong(lw, hw);
		fields[i].lastval = 0;
	}
	if((agrCount = rez->getUINT()) != 0) {
		THROW_MEM(agrs = (Aggr *) SAlloc::R(agrs, sizeof(Aggr) * agrCount));
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
		THROW_MEM(groups = (Group *)SAlloc::R(groups, sizeof(Group) * grpCount));
		for(i = 0; i < grpCount; i++) {
			Group * g = &groups[i];
			g->band   = rez->getUINT();
			if((len = rez->getUINT()) != 0) {
				THROW_MEM(g->fields = (int16 *)SAlloc::M(sizeof(int16) * (len + 1)));
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
		THROW_MEM(bands = (Band *)SAlloc::R(bands, sizeof(Band) * bandCount));
		for(i = 0; i < bandCount; i++) {
			Band * b   = &bands[i];
			b->kind    = rez->getUINT();
			b->ht      = rez->getUINT();
			b->group   = rez->getUINT();
			b->options = rez->getUINT();
			if((len = rez->getUINT()) != 0) {
				THROW_MEM(b->fields = (int16 *)SAlloc::M(sizeof(int16) * (len + 1)));
				b->fields[0] = len;
				for(j = 1; j <= len; j++)
					b->fields[j] = (int16)rez->getUINT();
			}
			else
				ZFREE(b->fields);
		}
	}
	else
		ZFREE(bands);
	PageLen    = (int)rez->getUINT();
	LeftMarg   = (int)rez->getUINT();
	PrnOptions = (int)rez->getUINT();
	CATCHZOK
	return ok;
}

int SLAPI SReport::setPrinter(SPrinter * p)
{
	P_Prn = p;
	return 1;
}

int SLAPI SReport::setData(int i, void *d)
{
	i--;
	if(i >= 0 && i < fldCount && fields[i].type) {
		fields[i].data = d;
		return 1;
	}
	return 0;
}

void SLAPI SReport::disableGrouping()
{
	PrnOptions |= SPRN_SKIPGRPS;
}

int SLAPI SReport::skipField(int i, int skip)
{
	i--;
	if(i >= 0 && i < fldCount) {
		SETFLAG(fields[i].fldfmt, FLDFMT_SKIP, skip);
		return 1;
	}
	return 0;
}

int SLAPI SReport::check()
{
	return 1;
}

int SLAPI SReport::calcAggr(int grp, int mode)
{
	double dd = 0;
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
				case AGGR_COUNT:
					if(mode == 0)
						agr->rtemp = 0;
					else if(mode == 1)
						agr->rtemp += 1;
					break;
				case AGGR_SUM:
					if(mode == 0)
						agr->rtemp = 0;
					else if(mode == 1)
						agr->rtemp += dd;
					break;
				case AGGR_AVG:
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
							if(agr->ptemp && agr->ptemp[0] != 0)
								dd = agr->ptemp[1] / agr->ptemp[0];
							else
								dd = 0;
							delete agr->ptemp;
							agr->rtemp = dd;
							break;
					}
					break;
				case AGGR_MIN:
					if(mode == 0)
						agr->rtemp = SMathConst::Max;
					else if(mode == 1)
						if(dd < agr->rtemp)
							agr->rtemp = dd;
					break;
				case AGGR_MAX:
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
int SLAPI LoadExportOptions(const char * pReportName, PEExportOptions * pOptions, int * pSilent, SString & rPath)
{
	int    ok = 1;
	int    silent = 0;
	PEExportOptions options;
	SString fname;
	PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_REPORT_INI, fname);
	MEMSZERO(options);
	options.StructSize = sizeof(options);
	if(fileExists(fname)) {
		int    idx = -1, r = 0;
		uint   i = 0;
		SString types, format, dest, buf;
		SString param_symb;
		PPIniFile ini_file(fname);
		PPIniFile::GetParamSymb(PPINIPARAM_REPORT_SILENT, param_symb);
		THROW(r = ini_file.GetIntParam(pReportName, param_symb, &silent));
		if(r == -1)
			THROW(ini_file.GetIntParam("default", param_symb, &silent));
		if(silent) {
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_FORMAT, param_symb);
			THROW(ini_file.GetParam(pReportName, param_symb, format));
			if(!format.NotEmptyS())
				THROW(ini_file.GetParam("default", param_symb, format));
			PPLoadText(PPTXT_EXPORT_FORMATTYPES, types);
			{
				StringSet ss(',', format);
				memzero(options.formatDLLName, PE_DLL_NAME_LEN);
				memzero(options.destinationDLLName, PE_DLL_NAME_LEN);
				if(ss.get(&(i = 0), buf) > 0) {
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
								((UXFCommaTabSeparatedOptions*)options.formatOptions)->structSize = UXFCommaTabSeparatedOptionsSize;
								break;
							case 4: // FormatTypes[4] == UXFCharSeparatedType
								options.formatOptions = new UXFCharSeparatedOptions;
								((UXFCharSeparatedOptions*)options.formatOptions)->structSize = UXFCharSeparatedOptionsSize;
								break;
							case 6: // FormatTypes[6] == UXFXls7ExtType
								options.formatOptions = new UXFXlsOptions;
								((UXFXlsOptions*)options.formatOptions)->structSize = UXFXlsOptionsSize;
								break;
							case 7: // FormatTypes[7] == UXFPdfType
								options.formatOptions = new UXFPdfOptions;
								((UXFPdfOptions*)options.formatOptions)->structSize = UXFPdfOptionsSize;
								break;
							default:
								options.formatOptions = 0;
								break;
						}
					}
				}
				switch(idx) {
					case 2: // FormatTypes[2] == UXFCommaSeparatedType
					case 3: // FormatTypes[3] == UXFTabSeparatedType
						{
							UXFCommaTabSeparatedOptions * p_options = (UXFCommaTabSeparatedOptions*)options.formatOptions;
							p_options->useReportDateFormat = 0;
							if(ss.get(&i, buf) > 0)
								p_options->useReportDateFormat = buf.ToLong();
							p_options->useReportNumberFormat = 0;
							if(ss.get(&i, buf) > 0)
								p_options->useReportNumberFormat = buf.ToLong();
						}
						break;
					case 4: // FormatTypes[4] == UXFCharSeparatedType
						{
							char host_buf[128];
							UXFCharSeparatedOptions * p_options = (UXFCharSeparatedOptions*)options.formatOptions;
							p_options->useReportDateFormat = 0;
							if(ss.get(&i, buf) > 0)
								p_options->useReportDateFormat = buf.ToLong();
							p_options->useReportNumberFormat = 0;
							if(ss.get(&i, buf) > 0)
								p_options->useReportNumberFormat = buf.ToLong();
							p_options->stringDelimiter = '=';
							if(ss.get(&i, buf) > 0) {
								hostrtocstr(buf, host_buf, sizeof(host_buf));
								p_options->stringDelimiter = host_buf[0];
							}
							p_options->fieldDelimiter = 0;
							if(ss.get(&i, buf) > 0) {
								hostrtocstr(buf, host_buf, sizeof(host_buf));
								p_options->fieldDelimiter = new char[sizeof(host_buf)];
								strnzcpy(p_options->fieldDelimiter, host_buf, sizeof(host_buf));
							}
						}
						break;
					case 6: // FormatTypes[6] == UXFXls7ExtType
						{
							UXFXlsOptions * p_options = (UXFXlsOptions*)options.formatOptions;
							p_options->bColumnHeadings = 0;
							if(ss.get(&i, buf) > 0)
								p_options->bColumnHeadings = buf.ToLong();
							p_options->bTabularFormat = 0;
							if(ss.get(&i, buf) > 0)
								p_options->bTabularFormat = buf.ToLong();
							p_options->bUseConstColWidth = 0;
							if(ss.get(&i, buf) > 0)
								p_options->bUseConstColWidth = buf.ToLong();
							if(ss.get(&i, buf) > 0)
								p_options->baseAreaGroupNum = (WORD)buf.ToLong();
							if(ss.get(&i, buf) > 0)
								p_options->baseAreaType = (WORD)buf.ToLong();
							if(ss.get(&i, buf) > 0)
								p_options->fConstColWidth = buf.ToReal();
						}
						break;
					case 7: // FormatTypes[7] == UXFPdfType
						{
							UXFPdfOptions * p_options = (UXFPdfOptions*)options.formatOptions;
							p_options->UsePageRange = false;
						}
						break;
					default:
						if(idx < 0)
							ok = -1;
						break;
				}
				char   ext[MAXEXT];
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
					if(ss2.get(&(i = 0), buf) > 0 && PPSearchSubStr(types, &idx, buf, 1) > 0) {
						PPGetSubStr(PPTXT_EXPORT_DESTDLL32, idx, options.destinationDLLName, PE_DLL_NAME_LEN);
						options.destinationType = DestinationTypes[idx];
						switch(idx) {
							case 0: // DestinationTypes[0] == UXDApplicationType
								options.destinationOptions = new UXDApplicationOptions;
								((UXDApplicationOptions*)options.destinationOptions)->structSize = UXDApplicationOptionsSize;
								break;
							case 1: // DestinationTypes[1] == UXDDiskType
								options.destinationOptions = new UXDDiskOptions;
								((UXDDiskOptions*)options.destinationOptions)->structSize = UXDDiskOptionsSize;
								break;
							default:
								options.destinationOptions = 0;
								break;
						}
					}
					buf = 0;
					if(ss2.get(&i, buf) > 0) {
						SString dir;
						if(buf.NotEmptyS()) {
							if(isDir(buf.RmvLastSlash())) {
								THROW_SL(MakeTempFileName(dir = buf, "exp", ext, 0, buf));
							}
							else {
								SPathStruc::ReplaceExt(buf, ext, 1);
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
									((UXDApplicationOptions*)options.destinationOptions)->fileName = newStr(buf);
								break;
							case 1: // DestinationTypes[1] == UXDDiskType
								if(options.destinationOptions)
									((UXDDiskOptions*)options.destinationOptions)->fileName = newStr(buf);
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
	ASSIGN_PTR(pSilent, ok > 0 ? silent : 0);
	return ok;
}

SLAPI ReportDescrEntry::ReportDescrEntry() : Flags(0)
{
}

// static
int FASTCALL ReportDescrEntry::GetIniToken(const char * pLine, SString * pFileName)
{
	int   tok = tUnkn;
	SString buf = pLine;
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
	else if(buf.CmpPrefix("format", 0) == 0)
		tok = tFormat;
	else if(buf.CmpPrefix("destination", 0) == 0)
		tok = tDestination;
	else if(buf.CmpPrefix("silent", 0) == 0)
		tok = tSilent;
	else {
		SString file_name;
		if(c_second != ':')
			PPGetFilePath(PPPATH_BIN, buf, file_name);
		else
			file_name = buf;
		if(fileExists(file_name)) {
			ASSIGN_PTR(pFileName, file_name);
			tok = tExistFile;
		}
		else
			tok = tUnkn;
	}
	return tok;
}

int SLAPI ReportDescrEntry::ParseIniString(const char * pLine, const ReportDescrEntry * pBaseEntry)
{
	int    ok = 1;
	SString line_buf;
	(line_buf = pLine).Chomp();
	return ok;
}

int SLAPI ReportDescrEntry::SetReportFileName(const char * pFileName)
{
	int    ok = -1;
	SString file_name = pFileName;
	const uint16 cr_eng_ver = PEGetVersion(PE_GV_ENGINE);
	if(HiByte(cr_eng_ver) >= 10) {
		SPathStruc ps(file_name);
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
		SPathStruc ps(file_name);
		SETFLAG(Flags, fTddoResource, ps.Ext.ToLower() == "tddo");
	}
	ReportPath_ = file_name;
	return ok;
}

int SLAPI PrnDlgAns::SetupReportEntries(const char * pContextSymb)
{
	Entries.freeAll();
	(ContextSymb = pContextSymb).Strip();

	int    ok = 1;
	uint   pos = 0;
	int    win_coding = 0;
	int    diffidbyscope = 0;
	int    force_ddf = 0;

	const uint16 cr_dll_ver = PEGetVersion(PE_GV_DLL);

	SString temp_buf, fname, buf2;
	SString left; // Временные переменные для деления буферов по символу
	if(isempty(P_ReportName))
		ok = -1;
	else {
		SString param_buf;
		PPIniFile ifile;
		ifile.Get(PPINISECT_PATH, PPINIPARAM_WINLOCALRPT, buf2);
		if(buf2.Empty())
			(buf2 = "RPT").SetLastSlash().Cat("LOCAL");
		ifile.GetInt(PPINISECT_CONFIG, PPINIPARAM_REPORT_FORCE_DDF, &force_ddf);
		SPathStruc::ReplaceExt((fname = P_ReportName), "RPT", 1);
		SPathStruc::ReplacePath(fname, buf2, 1);
		PPGetFilePath(PPPATH_BIN, fname, temp_buf);
		if(!fileExists(temp_buf)) {
			ifile.Get(PPINISECT_PATH, PPINIPARAM_WINDEFAULTRPT, buf2 = 0);
			buf2.SetIfEmpty("RPT");
			fname = P_ReportName;
			SPathStruc::ReplaceExt(fname, "RPT", 1);
			SPathStruc::ReplacePath(fname, buf2, 1);
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
		PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_STDRPT_INI, fname);
		if(fileExists(fname)) {
			StringSet std_ss;
			SString section_name;
			PPIniFile std_ini_file(fname);
			THROW(std_ini_file.IsValid());
			win_coding = std_ini_file.IsWinCoding();
			{
				if(ContextSymb.NotEmpty()) {
					(section_name = P_ReportName).Strip().CatChar(':').Cat(ContextSymb);
					std_ini_file.GetEntries(section_name, &std_ss);
				}
				if(std_ss.getCount() == 0) {
					(section_name = P_ReportName).Strip();
					std_ini_file.GetEntries(section_name, &std_ss);
				}
			}
			for(pos = 0; std_ss.get(&pos, param_buf);) {
				const int tok = ReportDescrEntry::GetIniToken(param_buf, &fname);
				switch(tok) {
					case ReportDescrEntry::tData:
						std_ini_file.GetParam(section_name, param_buf, Entries.at(0)->DataName_);
						Entries.at(0)->DataName_.Strip();
						if(!win_coding)
							Entries.at(0)->DataName_.Transf(CTRANSF_INNER_TO_OUTER);
						break;
					case ReportDescrEntry::tDescr:
						std_ini_file.GetParam(section_name, param_buf, Entries.at(0)->Description_);
						Entries.at(0)->Description_.Strip();
						if(!win_coding)
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
							if(!win_coding)
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
		PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_REPORT_INI, fname);
		if(fileExists(fname)) {
			SString format_p, dest_p, silent_p;
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_FORMAT, format_p);
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_DESTINATION, dest_p);
			PPIniFile::GetParamSymb(PPINIPARAM_REPORT_SILENT, silent_p);
			StringSet ss;
			SString section_name;
			PPIniFile ini_file(fname);
			THROW(ini_file.IsValid());
			win_coding = ini_file.IsWinCoding();
			{
				if(ContextSymb.NotEmpty()) {
					(section_name = P_ReportName).Strip().CatChar(':').Cat(ContextSymb);
					ini_file.GetEntries(section_name, &ss);
				}
				if(ss.getCount() == 0) {
					(section_name = P_ReportName).Strip();
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
						if(fname[0] == 0 || fileExists(fname)) {
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
							if(!win_coding)
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
		{
			for(uint j = 0; j < Entries.getCount(); j++)
				SETFLAG(Entries.at(j)->Flags, ReportDescrEntry::fDiff_ID_ByScope, diffidbyscope);
		}
	}
	CATCHZOK
	SETFLAG(Flags, fForceDDF, force_ddf);
	return ok;
}

class Print2Dialog : public TDialog {
public:
	Print2Dialog() : TDialog(DLG_PRINT2), EnableEMail(0)
	{
	}
	int    setDTS(PrnDlgAns * pData)
	{
		int    ok = 1;
		SString temp_buf;
		char   cr_path[MAXPATH];
		StrAssocArray list;
		P_Data = pData;
		SETIFZ(P_Data->Dest, 1);
		P_Data->Selection = 0;
		{
			int    silent = 0;
			if(LoadExportOptions(P_Data->P_ReportName, 0, &silent, temp_buf.Z()) > 0)
				EnableEMail = 1;
		}
		AddClusterAssoc(CTL_PRINT2_ACTION, 0, PrnDlgAns::aPrint);
		AddClusterAssoc(CTL_PRINT2_ACTION, 1, PrnDlgAns::aExport);
		AddClusterAssoc(CTL_PRINT2_ACTION, 2, PrnDlgAns::aPreview);
		AddClusterAssoc(CTL_PRINT2_ACTION, 3, PrnDlgAns::aExportXML);
		AddClusterAssoc(CTL_PRINT2_ACTION, 4, PrnDlgAns::aPrepareData);
		AddClusterAssoc(CTL_PRINT2_ACTION, 5, PrnDlgAns::aPrepareDataAndExecCR);
		SetClusterData(CTL_PRINT2_ACTION, P_Data->Dest);
		int    is_there_cr = FindExeByExt(".rpt", cr_path, sizeof(cr_path), "CrystalReports.9.1");
		if(!is_there_cr)
			DisableClusterItem(CTL_PRINT2_ACTION, 5, 1);
		THROW(P_Data->SetupReportEntries(0));
		if(P_Data->P_DefPrnForm) {
			//
			// Если задано имя формы по умолчанию и в списке форм есть файл с именем, совпадающем
			// с именем формы по умолчанию, то перемещаем эту форму на самый верх списка.
			//
			SPathStruc ps;
			for(uint i = 0; i < P_Data->Entries.getCount(); i++) {
				ps.Split(P_Data->Entries.at(i)->ReportPath_);
				if(ps.Nam.CmpNC(P_Data->P_DefPrnForm) == 0) {
					for(int j = i; j > 0; j--)
						P_Data->Entries.swap(j, j-1);
					break;
				}
			}
		}
		for(uint i = 0; i < P_Data->Entries.getCount(); i++) {
			(temp_buf = P_Data->Entries.at(i)->Description_).Transf(CTRANSF_OUTER_TO_INNER);
			list.Add(i+1, temp_buf);
		}
		SetupStrAssocCombo(this, CTLSEL_PRINT2_REPORT, &list, 1, 0);
		{
			SPrinting::GetListOfPrinters(&PrnList);
			list.Clear();
			long   sel_prn_id = 0;
			//
			// Перемещаем принтер по умолчанию на верх списка
			//
			long   def_prn_id = 0;
			for(uint j = 0; j < PrnList.getCount(); j++) {
				if(PrnList.at(j).Flags & SPrinting::PrnInfo::fDefault) {
					def_prn_id = j+1;
					break;
				}
			}
			if(def_prn_id > 1)
				PrnList.swap(0, (uint)(def_prn_id-1));
			//
			for(uint j = 0; j < PrnList.getCount(); j++) {
				temp_buf.Z().Cat(PrnList.at(j).PrinterName).ToOem();
				list.Add(j+1, temp_buf);
			}
			SetupStrAssocCombo(this, CTLSEL_PRINT2_PRINTER, &list, sel_prn_id, 0);
		}
		setCtrlLong(CTL_PRINT2_NUMCOPIES, P_Data->NumCopies);
		{
			PPPrinterCfg prn_cfg;
			PPGetPrinterCfg(0, 0, &prn_cfg);
			SETFLAG(P_Data->Flags, PrnDlgAns::fUseDuplexPrinting, BIN(prn_cfg.Flags & PPPrinterCfg::fUseDuplexPrinting));
			AddClusterAssoc(CTL_PRINT2_DUPLEX, 0, PrnDlgAns::fUseDuplexPrinting);
			SetClusterData(CTL_PRINT2_DUPLEX, P_Data->Flags);
		}
		SetupReportEntry();
		CATCHZOK
		return ok;
	}
	int    getDTS(PrnDlgAns * pData)
	{
		int    ok = 1;
		GetClusterData(CTL_PRINT2_ACTION, &pData->Dest);
		pData->Selection = getCtrlLong(CTLSEL_PRINT2_REPORT)-1;
		pData->NumCopies = getCtrlLong(CTL_PRINT2_NUMCOPIES);
		pData->Flags &= ~PrnDlgAns::fEMail;
		if(pData->Dest == PrnDlgAns::aExport && EnableEMail) {
			if(getCtrlUInt16(CTL_PRINT2_DOMAIL)) {
				pData->Flags |= PrnDlgAns::fEMail;
				getCtrlString(CTL_PRINT2_MAKEDATAPATH, pData->EmailAddr);
			}
		}
		else
			getCtrlString(CTL_PRINT2_MAKEDATAPATH, pData->PrepareDataPath);
		GetClusterData(CTL_PRINT2_DUPLEX, &P_Data->Flags);
		long   sel_id = getCtrlLong(CTLSEL_PRINT2_PRINTER);
		pData->Printer = (sel_id && sel_id <= (long)PrnList.getCount()) ? PrnList.at(sel_id-1).PrinterName : 0;
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
	}
	void   SetupReportEntry()
	{
		uint   id = (uint)getCtrlLong(CTLSEL_PRINT2_REPORT);
		int    enable_email = 0;
		SString data_name, path;
		GetClusterData(CTL_PRINT2_ACTION, &P_Data->Dest);
		if(id <= P_Data->Entries.getCount()) {
			SString stat_buf;
			const ReportDescrEntry * p_entry = P_Data->Entries.at(id-1);
			setStaticText(CTL_PRINT2_ST_FILENAME, p_entry->ReportPath_);
			SFileUtil::Stat fs;
			SFileUtil::GetStat(p_entry->ReportPath_, &fs);
			stat_buf.Cat(fs.ModTime);
			setStaticText(CTL_PRINT2_ST_FILEDTTM, stat_buf);
			setStaticText(CTL_PRINT2_ST_DATANAME, p_entry->DataName_);
			data_name = p_entry->DataName_;
		}
		else {
			setStaticText(CTL_PRINT2_ST_FILENAME, 0);
			setStaticText(CTL_PRINT2_ST_FILEDTTM, 0);
			setStaticText(CTL_PRINT2_ST_DATANAME, 0);
		}
		if(oneof2(P_Data->Dest, PrnDlgAns::aPrepareData, PrnDlgAns::aPrepareDataAndExecCR)) {
			/* @v9.8.9 PPIniFile ini_file;
			ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_REPORTDATAPATH, path);
			if(path.Empty())
				PPGetPath(PPPATH_TEMP, path);*/
			PPGetPath(PPPATH_REPORTDATA, path); // @v9.8.9
			if(data_name.NotEmpty())
				path.SetLastSlash().Cat(data_name);
			disableCtrl(CTL_PRINT2_MAKEDATAPATH, 0);
		}
		else if(P_Data->Dest == PrnDlgAns::aExport) {
			if(EnableEMail) {
				enable_email = 1;
				disableCtrl(CTL_PRINT2_MAKEDATAPATH, 0);
				path = P_Data->EmailAddr;
			}
		}
		else {
			disableCtrl(CTL_PRINT2_MAKEDATAPATH, 1);
		}
		disableCtrl(CTL_PRINT2_DOMAIL, !enable_email);
		setCtrlString(CTL_PRINT2_MAKEDATAPATH, path);
	}

	int    EnableEMail;
	SString InitPrepareDataPath;
	TSVector <SPrinting::PrnInfo> PrnList; // @v9.8.4 TSArray-->TSVector
	PrnDlgAns * P_Data;
};

int SLAPI EditPrintParam(PrnDlgAns * pData)
{
	return PPDialogProcBody <Print2Dialog, PrnDlgAns> (pData);
}

int SLAPI SReport::setNumCopies(int n)
{
	NumCopies = (n > 0 && n <= 10) ? n : 1;
	return 1;
}

int SLAPI SReport::getNumCopies() const
{
	return NumCopies;
}

static SString & GetTempFileName(const char * pFileName, SString & rDest)
{
	char * t = getenv("TEMP");
	if(!t)
		getenv("TMP");
	return (rDest = t).SetLastSlash().Cat(pFileName);
}

int SLAPI SReport::createBodyDataFile(SString & rFileName, SCollection * fldIDs)
{
	int    ok = 1, i;
	Band * b;
	page = line = 1;
	Field * f = 0;
	DBFCreateFld * flds=0;
	SString dbfname;
	int    flds_count = 0;

	fldIDs->freeAll();
	GetTempFileName(BODY_DBF_NAME, dbfname);
	rFileName = dbfname;
	DbfTable  * dbf = new DbfTable(dbfname);
	Field    ** dbf_fields_id=(Field **)SAlloc::M(sizeof(void *));
	for(int type = 0; type < (sizeof(row_band_types)/sizeof(int)); type++) {
		f = 0;
		for(b = searchBand(row_band_types[type], 0); enumFields(&f, b, &i);) {
			int    used = 0;
			const char * p_fld_name = P_Text+f->name;
			if(f->type != 0) {
				for(int di = 0; !used && di < flds_count; di++)
					if(strcmp(flds[di].Name, p_fld_name) == 0)
						used = 1;
				if(!used) {
					flds = (DBFCreateFld *)SAlloc::R(flds, sizeof(DBFCreateFld)*(1+flds_count));
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
		flds = (DBFCreateFld *)SAlloc::R(flds, sizeof(DBFCreateFld));
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
				if(strcmp(p_fld_name, flds[di].Name) == 0) {
					used = 1;
					break;
				}
			if(!used) {
				flds = (DBFCreateFld *)SAlloc::R(flds, sizeof(DBFCreateFld)*(1+flds_count));
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

int SLAPI SReport::createVarDataFile(SString & rFileName, SCollection * fldIDs)
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
	GetTempFileName(VAR_DBF_NAME, dbfname);
	rFileName = dbfname;
	DbfTable * dbf = new DbfTable(dbfname);
	flds = new DBFCreateFld[2];
	for(type = 0; type < (sizeof(hdr_band_types)/sizeof(int)); type++) {
		f = 0;
		for(Band * b = searchBand(hdr_band_types[type], 0); enumFields(&f, b, &i);) {
			int    used = 0;
			const char * p_fld_name = P_Text+f->name;
			if(f->type != 0) {
				for(int di = 0; !used && di < flds_count; di++)
					if(strcmp(flds[di].Name, p_fld_name) == 0)
						used = 1;
				if(!used) {
					int    fl = 0;
					flds = (DBFCreateFld *)SAlloc::R(flds,sizeof(DBFCreateFld)*(1+flds_count));
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
		flds = (DBFCreateFld *)SAlloc::R(flds, sizeof(DBFCreateFld));
		flds[flds_count].Init("dummy", 'C', 5, 0);
		fldIDs->insert(f);
		flds_count++;
	}
	dbf->create(flds_count, flds);
	delete dbf;
	delete [] flds;
	return 1;
}

int SLAPI SReport::prepareData()
{
	page = line = 1;

	int    ok = 1, i;
	uint   di;
	Field * f = 0;
	SString dbfname;
	SCollection fld_ids(/*DEFCOLLECTDELTA,*/aryPtrContainer);
	DbfTable * dbf = 0;
	createBodyDataFile(dbfname, &fld_ids);
	dbf = new DbfTable(dbfname);
	DbfRecord rec(dbf);
	calcAggr(-1, 0);
	if((i = iterator(1)) > 0)
		do {
			Count++;
			int    fldn = 1;
			for(di = 0; di < fld_ids.getCount(); di++) {
				f = (Field*)fld_ids.at(di);
				if(f->type != 0) {
					char buf[256];
					int c;
					if(!(PrnOptions & SPRN_SKIPGRPS))
						for(int i = 0; i < grpCount; i++)
							if((c = checkval(groups[i].fields, &groups[i].lastval)) > 0) {
								// Значение изменилось
								THROW(printGroupHead(GROUP_FOOT, i));
								THROW(printGroupHead(GROUP_HEAD, i));
							}
							else if(c < 0) { // Первая запись
								THROW(printGroupHead(GROUP_HEAD, i));
							}
					if(!(f->fldfmt & FLDFMT_SKIP))
						if(f->data) {
							int _s  = GETSSIZE(f->type);
							int _sd = GETSSIZED(f->type);
							switch (GETSTYPE(f->type)) {
								case S_INT:
									if(_s == 1)
										rec.put(di+1, (long)*(int8*)f->data);
									else if(_s == 2)
										rec.put(di+1, (long)*(int16*)f->data);
									else if(_s == 4)
										rec.put(di+1, (long)*(int32*)f->data);
									break;
								case S_FLOAT:
									if(_s == 4)
										rec.put(di+1, (double)*(float*)f->data);
									else if(_s == 8)
										rec.put(di+1, *(double*)f->data);
									break;
								case S_DEC:
								case S_MONEY: {
										double _dbl_temp = dectobin((char*)f->data, (int16)GETSSIZED(f->type),
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
									rec.put(di+1, (char*)f->data);
									break;
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
	if(!(PrnOptions & SPRN_SKIPGRPS))
		for(i = 0; i < grpCount; i++) {
			THROW(printGroupHead(GROUP_FOOT, i));
		}
	calcAggr(-1, 2);
	dbf->close();
	ZDELETE(dbf);
	createVarDataFile(dbfname, &fld_ids);
	dbf = new DbfTable(dbfname);
	{
		int  fldn = 0;
		char buf[256];
		DbfRecord rec(dbf);
		for(di = 0; di < fld_ids.getCount(); di++) {
			f = (Field*)fld_ids.at(di);
			if(f->type != 0) {
				if(f->data)
					if(GETSTYPE(f->type) == S_ZSTRING)
						STRNSCPY(buf, (char*)f->data);
					else
						sttostr(f->type, f->data, f->format, buf);
				else {
					memset(buf, ' ', SFMTLEN(f->format));
					buf[SFMTLEN(f->format)] = 0;
				}
				rec.put(di+1, buf);
			}
		}
		dbf->appendRec(&rec);
	}
	CATCHZOK
	delete dbf;
	return ok;
}

void ReportError(short printJob)
{
	HANDLE text_handle;
	short  text_length;
	short  error_code = PEGetErrorCode(printJob);
	PEGetErrorText(printJob, &text_handle, &text_length);
	char * p_error_text = (char *)SAlloc::M(text_length);
	PEGetHandleString(text_handle, p_error_text, text_length);
	::MessageBox(0, p_error_text, _T("Print Job Failed"), MB_OK|MB_ICONEXCLAMATION); // @unicodeproblem
	SAlloc::F(p_error_text);
}

static int SLAPI SetupGroupSkipping(short hJob)
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
	char * ch = strchr(grpopt.fieldName, '.');
	if(ch) {
		*ch = 0;
		strcat(grpopt.fieldName, "._ID_}");
	}
	PESetGroupOptions(hJob, 0, &grpopt);
	return 1;
}

static int SLAPI GetDataFilePath(int locN, const char * pPath, int isPrint, SString & rBuf /*char * pBuf, size_t bufLen*/)
{
	if(pPath) {
		const  char * p_fname = (locN == 0) ? "head" : "iter";
		const  char * p_ext = "btr";
		SString path;
		SPathStruc::ReplaceExt((path = pPath).SetLastSlash().Cat(p_fname), p_ext, 1);
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
		GetTempFileName(p_fname, rBuf);
	}
	return 1;
}

struct RptTblLoc {
	PETableLocation Hdr;
	PETableLocation Iter;
};

static int SLAPI SetupSubReportLocations(short hJob, RptTblLoc & rLoc, int section)
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

static int SLAPI SetupReportLocations(short hJob, const char * pPath, int isPrint)
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

static int SLAPI RemoveCompName(SString & rPrintDevice)
{
	char   buf[256];
	SString sbuf;
	DWORD  buf_size = sizeof(buf);
	memzero(buf, sizeof(buf));
	GetComputerNameEx(ComputerNameNetBIOS, buf, &buf_size);
	(sbuf = "\\\\").Cat(buf).CatChar('\\');
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

static int SLAPI SetPrinterParam(short hJob, const char * pPrinter, long options)
{
	int    ok = 1;
	SString print_device = isempty(pPrinter) ? DS.GetConstTLA().PrintDevice : pPrinter;
	RemoveCompName(print_device);
	DEVMODE * p_dm = 0, dm;
	if(options & SPRN_USEDUPLEXPRINTING || print_device.NotEmptyS()) {
		char   device_name[128], port_name[128], drv_name[64];
		HANDLE h_drv = 0;
		HANDLE h_prn = 0;
		HANDLE h_port = 0;
		short  drv_len, prn_len, port_len;
		memzero(device_name, sizeof(device_name));
		memzero(port_name, sizeof(port_name));
		memzero(drv_name, sizeof(drv_name));
		THROW_PP(PEGetSelectedPrinter(hJob, &h_drv, &drv_len, &h_prn, &prn_len, &h_port, &port_len, &p_dm), PPERR_CRYSTAL_REPORT);
		if(!RVALUEPTR(dm, p_dm))
			MEMSZERO(dm);
		PEGetHandleString(h_prn,  device_name, sizeof(device_name));
		PEGetHandleString(h_port, port_name, sizeof(port_name));
		PEGetHandleString(h_drv,  drv_name, sizeof(drv_name));
		if(print_device.NotEmpty())
			STRNSCPY(device_name, print_device);
		if(options & SPRN_USEDUPLEXPRINTING) {
			DWORD  is_duplex_device = DeviceCapabilities(device_name, port_name, DC_DUPLEX, 0, p_dm);
			if(is_duplex_device) {
				RVALUEPTR(dm, p_dm);
				dm.dmFields |= DM_DUPLEX;
				dm.dmDuplex = DMDUP_VERTICAL;
			}
		}
		THROW_PP(PESelectPrinter(hJob, drv_name, device_name, port_name, &dm), PPERR_CRYSTAL_REPORT);
	}
	CATCH
		CrwError = PEGetErrorCode(hJob);
		ok = 0;
	ENDCATCH
	return ok;
}

const char * DefaultWindowsPrinter = "DefaultWindowsPrinter";

int SLAPI GetWindowsPrinter(PPID * pPrnID, SString * pPort)
{
	int    ok = -1;
	WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
	uint32 loc_prn_id = 0;
	PPLocPrinter loc_prn;
 	PPObjLocPrinter obj_locprn;
 	MEMSZERO(loc_prn);
 	reg_key.GetDWord(DefaultWindowsPrinter, &loc_prn_id);
 	if(loc_prn_id && (ok = obj_locprn.Search(loc_prn_id, &loc_prn)) > 0) {
		ASSIGN_PTR(pPrnID, (PPID)loc_prn_id);
		CALLPTRMEMB(pPort, CopyFrom(loc_prn.Port));
	}
	return ok;
}

int SLAPI CrystalReportPrint(const char * pReportName, const char * pDir, const char * pPrinter, int numCopies, int options)
{
	int    ok = 1;
	int    zero_print_device = 0;
	short  h_job = PEOpenPrintJob(pReportName);
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
	THROW(SetPrinterParam(h_job, pPrinter, options));
	THROW(SetupReportLocations(h_job, pDir, (options & SPRN_DONTRENAMEFILES) ? 0 : 1));
	if(options & SPRN_PREVIEW) {
		THROW_PP(PEOutputToWindow(h_job, "", CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, WS_MAXIMIZE|WS_VISIBLE|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU, 0), PPERR_CRYSTAL_REPORT);
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
					((PreviewEventParam *)pUserData)->StopPreview++;
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
			msg_buf.Z().CatEq("Report", pReportName);
			if(!isempty(pPrinter))
				msg_buf.CatDiv(';', 2).CatEq("Printer", pPrinter);
			if(numCopies > 1)
				msg_buf.CatDiv(';', 2).CatEq("Copies", (long)numCopies);
			msg_buf.CatDiv(';', 2).CatEq("Mks", (int64)(profile_end - profile_start));
			PPLogMessage(PPFILNAM_REPORTING_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME|LOGMSGF_DBINFO);
		}
	}
	CATCH
		CrwError = PEGetErrorCode(h_job);
		// @v8.3.4 @debug {
		{
			// @v9.4.9 (msg_buf = "Crystal Reports error:").CatDiv(':', 2).Cat(CrwError);
			// @v9.4.9 {
			PPLoadString("err_crpe", msg_buf);
			msg_buf.Transf(CTRANSF_INNER_TO_OUTER);
			msg_buf.CatDiv(':', 2).Cat(CrwError);
			// } @v9.4.9
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_COMP|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		}
		// } @v8.3.4 @debug
		ok = 0;
	ENDCATCH
	if(h_job)
		PEClosePrintJob(h_job);
	if(zero_print_device)
		DS.GetTLA().PrintDevice = 0;
	return ok;
}

int SLAPI CrystalReportExport(const char * pReportPath, const char * pDir, const char * pReportName,
	const char * pEMailAddr, int options)
{
	int    ok = 1, silent = 0;
	SString path;
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
	if(silent || PEGetExportOptions(h_job, &eo)) {
		PEExportTo(h_job, &eo);
		THROW(SetupReportLocations(h_job, pDir, 0));
		if(options & SPRN_SKIPGRPS)
			SetupGroupSkipping(h_job);
		THROW_PP(PEStartPrintJob(h_job, TRUE), PPERR_CRYSTAL_REPORT);
		if(silent && pEMailAddr && fileExists(path)) {
			//
			// Отправка на определенный почтовый адрес
			//
			PPAlbatrosConfig alb_cfg;
			THROW(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0);
			if(alb_cfg.Hdr.MailAccID) {
				THROW(SendMailWithAttach(pReportName, path, pReportName, pEMailAddr, alb_cfg.Hdr.MailAccID));
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

SReport::Band * SLAPI SReport::searchBand(int kind, int grp)
{
	Band *b = 0;
	for(int i = 0; i < bandCount && b == 0; i++)
		if(bands[i].kind == kind && bands[i].group == grp)
			b = bands + i;
	return b;
}

int SLAPI SReport::printDataField(SReport::Field * f)
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

int SLAPI SReport::enumFields(SReport::Field ** f, SReport::Band * b, int *i)
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

int SLAPI SReport::printPageHead(int kind, int _newpage)
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
	while(enumFields(&f, b, &i))
		if(f->type == 0) {
			const char * p, * t;
			p = t = P_Text+f->offs;
			while((p = strchr(p, '\n')) != 0)
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
	CATCHZOK
	return ok;
}

int SLAPI SReport::printGroupHead(int kind, int grp)
{
	if(searchBand(kind, grp))
		if(kind == GROUP_HEAD)
			calcAggr(grp, 0);
		else if(kind == GROUP_FOOT)
			calcAggr(grp, 2);
	return 1;
}

int SLAPI SReport::checkval(int16 *flds, char **ptr)
{
	int     i, r;
	uint    s, ofs;
	TYPEID  t;
	Field * f;
	if(*ptr == 0)
		if(flds && flds[0]) {
			r = -1;
			s = 0;
			for(i = 1; i <= flds[0]; i++)
				s += stsize(fields[flds[i] - 1].type);
			(*ptr) = (char *)SAlloc::M(s);
		}
		else
			r = 0;
	else if(flds && flds[0])
		for(ofs = r = 0, i = 1; i <= flds[0] && r == 0; i++) {
			f = fields + flds[i] - 1;
			t = f->type;
			if(f->data && stcomp(t, f->data, (*ptr) + ofs))
				r = 1;
			else
				ofs += stsize(t);
		}
	else
		r = 0;
	if(r && *ptr)
		for(ofs = 0, i = 1; i <= flds[0]; i++) {
			f = fields + flds[i] - 1;
			s = stsize(f->type);
			if(f->data)
				memcpy((*ptr) + ofs, f->data, s);
			else
				memzero((*ptr) + ofs, s);
			ofs += s;
		}
	return r;
}

int SLAPI SReport::printDetail()
{
	int     ok = 1, i, c;
	Band  * b;
	Field * f = 0;
	if(!(PrnOptions & SPRN_SKIPGRPS))
		for(i = 0; i < grpCount; i++)
			if((c = checkval(groups[i].fields, &groups[i].lastval)) > 0) {
				// Значение изменилось
				THROW(printGroupHead(GROUP_FOOT, i));
				THROW(printGroupHead(GROUP_HEAD, i));
			}
			else if(c < 0) { // Первая запись
				THROW(printGroupHead(GROUP_HEAD, i));
			}
	for(b = searchBand(DETAIL_BODY, 0); enumFields(&f, b, &i);)
		if(f->type == 0) {
			const char * p, * t;
			p = t = P_Text+f->offs;
			while((p = strchr(p, '\n')) != 0)
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
			THROW(P_Prn->printLine(t, 0));
		}
		else
			THROW(printDataField(f));
	CATCHZOK
	return ok;
}

int SLAPI SReport::printTitle(int kind)
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
		while(enumFields(&f, b, &i))
			if(f->type == 0) {
				const char * p, * t;
				p = t = P_Text+f->offs;
				while((p = strchr(p, '\n')) != 0)
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
				THROW(P_Prn->printLine(t, 0));
			}
			else
				THROW(printDataField(f));
	}
	CATCHZOK
	return ok;
}

int SLAPI SReport::getFieldName(SReport::Field * f, char * buf, size_t buflen)
{
	if(f->name >= 0) {
		strnzcpy(buf, P_Text + f->name, buflen);
		return 1;
	}
	else if(buf)
		buf[0] = 0;
	return -1;
}

int SLAPI SReport::getFieldName(int i, char * buf, size_t buflen)
{
	return (i >= 0 && i < fldCount) ? getFieldName(&fields[i], buf, buflen) : 0;
}
//
//	@Vadim 13.09.02
//
struct SvdtStrDlgAns {
	int   SvDt;
	int   EdRep;
	char  SvDtPath[MAXPATH];
	char  EdRepPath[MAXPATH];
};

#define FBB_GROUP1	1
#define FBB_GROUP2	2

class SvdtStrDialog : public TDialog {
public:
	SvdtStrDialog(uint dlgID) : TDialog(dlgID)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_SAVEDATA_SVDTPATH, CTL_SAVEDATA_SVDTPATH, FBB_GROUP1, 
			PPTXT_TITLE_DATASTRUCSAVING, PPTXT_FILPAT_DDFBTR, FileBrowseCtrlGroup::fbcgfPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_SAVEDATA_EDREPPTH, CTL_SAVEDATA_EDREPPATH, FBB_GROUP2, 
			0, PPTXT_FILPAT_REPORT, FileBrowseCtrlGroup::fbcgfFile);
	}
	int   setDTS(const SvdtStrDlgAns *);
	int   getDTS(SvdtStrDlgAns *);
	SvdtStrDlgAns Ssda;
};

int SvdtStrDialog::setDTS(const SvdtStrDlgAns *pSsda)
{
	Ssda = *pSsda;
	setCtrlData(CTL_SAVEDATA_SVDT, &Ssda.SvDt);
	setCtrlData(CTL_SAVEDATA_SVDTPATH, Ssda.SvDtPath);
	setCtrlData(CTL_SAVEDATA_EDREPPATH, Ssda.EdRepPath);
	if(Ssda.EdRep)
		setCtrlData(CTL_SAVEDATA_EDREP, &Ssda.EdRep);
	else {
		disableCtrl(CTL_SAVEDATA_EDREP, 1);
		disableCtrl(CTL_SAVEDATA_EDREPPATH, 1);
		showCtrl(CTLBRW_SAVEDATA_EDREPPTH, 0);
	}
	return 1;
}

int SvdtStrDialog::getDTS(SvdtStrDlgAns *pSsda)
{
	getCtrlData(CTL_SAVEDATA_SVDT, &Ssda.SvDt);
	getCtrlData(CTL_SAVEDATA_SVDTPATH, Ssda.SvDtPath);
	getCtrlData(CTL_SAVEDATA_EDREP, &Ssda.EdRep);
	getCtrlData(CTL_SAVEDATA_EDREPPATH, Ssda.EdRepPath);
	ASSIGN_PTR(pSsda, Ssda);
	return 1;
}

static int SLAPI GetSvdtStrOpt(SvdtStrDlgAns * pSsda) { DIALOG_PROC_BODY_P1(SvdtStrDialog, DLG_SAVEDATA, pSsda); }

int SLAPI SaveDataStruct(const char *pDataName, const char *pTempPath, const char *pRepFileName)
{
	int    ok = -1;
	SString path, fname;
	char   cr_path[MAXPATH];
	SvdtStrDlgAns * p_ssda = 0;
	// @v9.8.9 PPIniFile ini_file;
	// @v9.8.9 if(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_REPORTDATAPATH, path) > 0 && (CrwError == PE_ERR_ERRORINDATABASEDLL)) {
	// @v9.8.9 {
	PPGetPath(PPPATH_REPORTDATA, path); 
	if(path.NotEmptyS() && CrwError == PE_ERR_ERRORINDATABASEDLL) {
	// } @v9.8.9
		path.SetLastSlash().Cat(pDataName);
		char * p_chr = (char *)pRepFileName;
		THROW_MEM(p_ssda = new SvdtStrDlgAns);
		p_ssda->SvDt = 1;
		strnzcpy(p_ssda->SvDtPath, path, MAXPATH);
		if((p_chr = strchr(p_chr, '.')) != NULL)
			p_ssda->EdRep = FindExeByExt(p_chr, cr_path, sizeof(cr_path), "CrystalReports.9.1");
		else
			p_ssda->EdRep = 0;
		strnzcpy(p_ssda->EdRepPath, pRepFileName, MAXPATH);
		if(GetSvdtStrOpt(p_ssda) > 0) {
			if(p_ssda->SvDt) {
				path = p_ssda->SvDtPath;
				if(!fileExists(path))
					THROW_SL(::createDir(path));
				CopyDataStruct(pTempPath, path, BDictionary::DdfTableFileName);
				CopyDataStruct(pTempPath, path, BDictionary::DdfFieldFileName);
				CopyDataStruct(pTempPath, path, BDictionary::DdfIndexFileName);
				CopyDataStruct(pTempPath, path, PPGetFileName(PPFILNAM_HEAD_BTR, fname));
				CopyDataStruct(pTempPath, path, PPGetFileName(PPFILNAM_ITER_BTR, fname));
			}
			if(p_ssda->EdRep)
				spawnl(_P_NOWAIT, cr_path, cr_path, p_ssda->EdRepPath, 0);
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
int SLAPI EditDefaultPrinterCfg()
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
	SETFLAG(v, 0x01, cfg.Flags & PPPrinterCfg::fUseDuplexPrinting);
	dlg->setCtrlData(CTL_PRNCFG_FLAGS, &v);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_PRNCFG_PRINTER, &cfg.PrnCmdSet);
		dlg->getCtrlData(CTL_PRNCFG_PORT, cfg.Port);
		dlg->getCtrlData(CTL_PRNCFG_FLAGS, &(v = 0));
		{
			uint32 dw_loc_prn_id = 0;
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
			dlg->getCtrlData(CTLSEL_PRNCFG_WINPRINTER, &loc_prn_id);
			dw_loc_prn_id = (uint32)loc_prn_id;
			reg_key.PutDWord(DefaultWindowsPrinter, dw_loc_prn_id);
		}
		SETFLAG(cfg.Flags, PPPrinterCfg::fUseDuplexPrinting, v & 0x01);
		THROW(PPSetPrinterCfg(PPOBJ_CONFIG, PPCFG_MAIN, &cfg));
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI VerifyCrpt(const char * pRptPath, const char * pDataPath)
{
	int ok = -1;
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
			(buf = pDataPath).SetLastSlash().Cat("file.ddf");
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

int SLAPI MakeCRptDataFiles(int verifyAll /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_MKRPTFLS);
	if(CheckDialogPtrErr(&dlg)) {
		SString rpt_name, rpt_path, fname;
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_MKRPTFLS_RPTPATH, CTL_MKRPTFLS_RPTPATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		/* @v9.8.9 PPIniFile ini_file;
		ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_REPORTDATAPATH, rpt_path);
		if(rpt_path.Empty())
			PPGetPath(PPPATH_TEMP, rpt_path); */
		PPGetPath(PPPATH_REPORTDATA, rpt_path); // @v9.8.9
		if(verifyAll == 1) {
			rpt_name = "ALL";
			dlg->disableCtrl(CTL_MKRPTFLS_RPTNAME, 1);
		}
		dlg->setCtrlString(CTL_MKRPTFLS_RPTNAME, rpt_name);
		dlg->setCtrlString(CTL_MKRPTFLS_RPTPATH, rpt_path);
		while(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_MKRPTFLS_RPTNAME, rpt_name);
			dlg->getCtrlString(CTL_MKRPTFLS_RPTPATH, rpt_path);
			PPWait(1);
			if(rpt_name.NotEmptyS()) {
				uint      pos = 0;
				SReport rpt((char*)0);
				StringSet ss;
				SStrCollection ss_col;
				SString data_path;
				if(rpt_name.CmpNC("ALL") == 0) {
					PPGetFilePath(PPPATH_BIN, (uint)PPFILNAM_STDRPT_INI, fname);
					PPIniFile std_ini_file(fname);
					THROW_SL(std_ini_file.IsValid());
					THROW_SL(std_ini_file.GetSections(&ss));
				}
				else
					THROW_SL(ss.add(rpt_name));
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
			PPWait(0);
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
	int    inherited_tbl_names = 0;
	int    diffidbyscope = 0;
	DlRtm   * p_rtm = 0;
	SString data_name, fn, printer_name;
	SString temp_buf;
	long   fl = 0;
	const  uint sur_key_last_count = DS.GetTLA().SurIdList.getCount(); // См. коммент в конце функции
	if(pEnv) {
		if(pEnv->PrnFlags & SReport::Landscape)
			fl |= INIREPF_FORCELANDSCAPE;
		if(pEnv->PrnFlags & SReport::PrintingNoAsk)
			fl |= INIREPF_NOSHOWDIALOG;
	}
	SReport rpt(rptId, fl);
	THROW(rpt.IsValid());
	{
		PrnDlgAns pans(rpt.Name);
		const ReportDescrEntry * p_sel_entry = 0;
		if(pEnv) {
			pans.P_DefPrnForm = pEnv->DefPrnForm;
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
				diffidbyscope = BIN(p_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
			}
		}
		else
			pans.P_DefPrnForm = 0;
		if(!rpt.PrnDest) {
			if(EditPrintParam(&pans) > 0) {
				pans.Flags &= ~pans.fForceDDF;
				p_sel_entry = pans.Entries.at(pans.Selection);
				fn                  = p_sel_entry->ReportPath_;
				data_name           = p_sel_entry->DataName_;
				inherited_tbl_names = BIN(p_sel_entry->Flags & ReportDescrEntry::fInheritedTblNames);
				diffidbyscope       = BIN(p_sel_entry->Flags & ReportDescrEntry::fDiff_ID_ByScope);
				rpt.PrnDest         = pans.Dest;
				printer_name        = pans.Printer;
				SETFLAG(rpt.PrnOptions, SPRN_USEDUPLEXPRINTING, BIN(pans.Flags & PrnDlgAns::fUseDuplexPrinting));
			}
			else
				ok = -1;
		}
		if(ok > 0) {
			data_name.SetIfEmpty(rpt.getDataName());
			SString out_file_name;
			DlContext ctx;
			DlRtm::ExportParam ep;
			THROW(ctx.InitSpecial(DlContext::ispcExpData));
			THROW(ctx.CreateDlRtmInstance(data_name, &p_rtm));
			PPWait(1);
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
                	SPathStruc ps(fn);
                	ps.Drv.Z();
                	ps.Dir.Z();
					const SString nam = ps.Nam;
					if(p_sel_entry->OutputFormat.CmpNC("html") == 0) {
						ps.Ext.Z();
                		ps.Merge(inner_fn);
						PPGetFilePath(PPPATH_OUT, inner_fn, out_file_name);
						SFile::Remove(out_file_name);
						THROW_SL(::createDir(out_file_name));
						ep.DestPath = out_file_name;
						ep.OutputFormat = SFileFormat(SFileFormat::Html);
						out_file_name.SetLastSlash().Cat("index.html");
					}
					else if(p_sel_entry->OutputFormat.CmpNC("latex") == 0) {
						ps.Ext.Z();
                		ps.Merge(inner_fn);
						PPGetFilePath(PPPATH_OUT, inner_fn, out_file_name);
						SFile::Remove(out_file_name);
						THROW_SL(::createDir(out_file_name));
						ep.DestPath = out_file_name;
						ep.OutputFormat = SFileFormat(SFileFormat::Latex);
						out_file_name.SetLastSlash().Cat(nam).Dot().Cat("tex");
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
                	size_t sz = result.GetAvailableSize();
					SFile f_out(out_file_name, SFile::mWrite);
					THROW_SL(f_out.IsValid());
					THROW_SL(f_out.Write(result.GetBuf(), sz));
                }
			}
			else if(rpt.PrnDest == PrnDlgAns::aExportXML) {
				ep.Cp = DS.GetConstTLA().DL600XmlCp; // @v9.4.6
				THROW(p_rtm->ExportXML(ep, out_file_name));
			}
			else {
				if(oneof2(rpt.PrnDest, PrnDlgAns::aPrepareData, PrnDlgAns::aPrepareDataAndExecCR)) {
					ep.Flags |= DlRtm::ExportParam::fForceDDF;
					ep.DestPath = pans.PrepareDataPath;
				}
				else {
					// @v9.8.9 {
					{
						//
						// Для Crystal Reports 10 и выше удаляем каталог подготовки данных (если существует)
						// ибо почему-то Crystal Reports использует его с приоритетом в некоторых случаях.
						//
						PPGetPath(PPPATH_REPORTDATA, temp_buf);
						if(data_name.NotEmpty())
							temp_buf.SetLastSlash().Cat(data_name);
						if(temp_buf.NotEmpty() && isDir(temp_buf)) {
							const uint16 cr_eng_ver = PEGetVersion(PE_GV_ENGINE);
							if(HiByte(cr_eng_ver) >= 10)
								RemoveDir(temp_buf);
						}
					}
					// } @v9.8.9 
					ep.Flags &= ~DlRtm::ExportParam::fForceDDF;
				}
				THROW(p_rtm->Export(ep));
			}
			PPWait(0);
			switch(rpt.PrnDest) {
				case PrnDlgAns::aPrint:
					ok = CrystalReportPrint(fn, ep.Path, printer_name, pans.NumCopies, rpt.PrnOptions);
					break;
				case PrnDlgAns::aPreview:
					ok = CrystalReportPrint(fn, ep.Path, printer_name, 1, rpt.PrnOptions | SPRN_PREVIEW);
					break;
				case PrnDlgAns::aExport:
					{
						const char * p_mail_addr = 0;
						if(pans.Flags & pans.fEMail && pans.EmailAddr.NotEmptyS())
							p_mail_addr = pans.EmailAddr;
						ok = CrystalReportExport(fn, ep.Path, pans.P_ReportName, p_mail_addr, rpt.PrnOptions);
					}
					break;
				case PrnDlgAns::aExportXML:
				case PrnDlgAns::aExportTDDO:
				case PrnDlgAns::aPrepareData:
					break;
				case PrnDlgAns::aPrepareDataAndExecCR:
					{
						char   cr_path[MAXPATH];
						int    is_there_cr = FindExeByExt(".rpt", cr_path, sizeof(cr_path), "CrystalReports.9.1");
						if(is_there_cr) {
							(temp_buf = cr_path).Space().Cat(fn);
							STempBuffer cmd_line(temp_buf.Len() * 2);
							strnzcpy(cmd_line, temp_buf, cmd_line.GetSize());
							STARTUPINFO si;
							PROCESS_INFORMATION pi;
							MEMSZERO(si);
							si.cb = sizeof(si);
							MEMSZERO(pi);
							int    r = ::CreateProcess(0, cmd_line, 0, 0, FALSE, 0, 0, 0, &si, &pi);
							if(!r) {
								SLS.SetOsError(0);
								PPSetErrorSLib();
								CALLEXCEPT();
							}
						}
					}
					break;
				default:
					ok = -1;
					break;
			}
			if(!ok) {
				PPError();
			}
		}
	}
	CATCH
		if(!pEnv || !(pEnv->PrnFlags & SReport::NoRepError))
			PPError();
		ok = 0;
	ENDCATCH
	PPWait(0);
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

int FASTCALL PPAlddPrint(int rptId, PPFilt * pf, const PPReportEnv * pEnv)
	{ return __PPAlddPrint(rptId, pf, 0, pEnv); }
int FASTCALL PPAlddPrint(int rptId, PView * pview, const PPReportEnv * pEnv)
	{ return __PPAlddPrint(rptId, (PPFilt*)pview, 1, pEnv); }

static int SLAPI Implement_ExportDL600DataToBuffer(const char * pDataName, long id, void * pPtr, SCodepageIdent cp, SString & rBuf)
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
	ep.Flags = 0;
	ep.Cp = cp;
	SETFLAG(ep.Flags, DlRtm::ExportParam::fInheritedTblNames, 1);
	SETFLAG(ep.Flags, DlRtm::ExportParam::fDontWriteXmlDTD, 1);
	THROW(p_rtm->PutToXmlBuffer(ep, rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL PPExportDL600DataToBuffer(const char * pDataName, long id, SCodepageIdent cp, SString & rBuf)
{
	return Implement_ExportDL600DataToBuffer(pDataName, id, 0, cp, rBuf);
}

int FASTCALL PPExportDL600DataToBuffer(const char * pDataName, void * ptr, SCodepageIdent cp, SString & rBuf)
{
	return Implement_ExportDL600DataToBuffer(pDataName, 0, ptr, cp, rBuf);
}

int FASTCALL PPExportDL600DataToJson(const char * pDataName, StrAssocArray * pStrAssocAry, void * ptr, SString & rBuf)
{
	int         ok = 1;
	DlContext * p_ctx = 0;
	DlRtm     * p_rtm = 0;
	THROW(p_ctx = DS.GetInterfaceContext(PPSession::ctxtExportData));
	if(pStrAssocAry) {
		THROW(p_rtm = p_ctx->GetRtm(isempty(pDataName) ? "StrAssocArray" : pDataName));
		THROW(p_rtm->PutToJsonBuffer(pStrAssocAry, (rBuf = 0), 0 /* flags */));
	}
	else if(ptr) {
		THROW(p_rtm = p_ctx->GetRtm(pDataName));
		THROW(p_rtm->PutToJsonBuffer(ptr, (rBuf = 0), 0 /* flags */));
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}
