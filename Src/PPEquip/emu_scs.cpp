// EMU_SCS.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2003, 2009, 2011, 2012, 2013, 2015, 2016, 2017, 2019, 2020, 2021, 2022, 2024
// @codepage UTF-8
// Интерфейс эмулятора синхронного кассового аппарата
//
#include <pp.h>
#pragma hdrstop

#define max MAX
#define min MIN
#include <gdiplus.h>
using namespace Gdiplus;
//
//
//
struct PrnLineStruc {
	SString PrnBuf;
	SlipLineParam Param;
};

typedef TSCollection <PrnLineStruc> PrnLinesArray;

class SCS_SYNCSYM : public PPSyncCashSession {
public:
	SCS_SYNCSYM(PPID n, char * name, char * port);
	~SCS_SYNCSYM();
	virtual int PrintCheck(CCheckPacket * pPack, uint flags);
	virtual int PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int PrintXReport(const CSessInfo *);
	virtual int PrintZReportCopy(const CSessInfo *);
	virtual int CloseSession(PPID sessID);
	virtual int GetSummator(double * val);
	virtual int PrintBnkTermReport(const char * pZCheck); // @vmiller
private:
	int    SendToPrinter(PrnLinesArray * pPrnLines);
	int    OpenBox(); // @vmiller

	uint   TextOutput; // @vmiller
	SString PrinterPort;
	HFONT  OldPrinterFont;
	HDC    PrinterDC;
	PPObjCSession CsObj;
};

class CM_SYNCSYM : public PPCashMachine {
public:
	CM_SYNCSYM(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPSyncCashSession * SyncInterface();
};

PPSyncCashSession * CM_SYNCSYM::SyncInterface()
{
	PPSyncCashSession * cs = 0;
	if(IsValid()) {
		cs = new SCS_SYNCSYM(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return cs;
}

REGISTER_CMT(SYNCSYM, true, false);

SCS_SYNCSYM::SCS_SYNCSYM(PPID n, char * pName, char * pPort) : PPSyncCashSession(n, pName, pPort),
	PrinterDC(0), OldPrinterFont(0), PrinterPort(SCn.PrinterPort), TextOutput(0)
{
}

SCS_SYNCSYM::~SCS_SYNCSYM()
{
	if(PrinterDC) {
		if(OldPrinterFont)
			::SelectObject(PrinterDC, OldPrinterFont);
		::DeleteDC(PrinterDC);
	}
}

#define AXIOHM_CMD_SETCHARTBL_BYTE1  0x1B
#define AXIOHM_CMD_SETCHARTBL_BYTE2  0x52
#define AXIOHM_CMD_CODETABL_CP866_ID 0x07 // CP-866
#define AXIOHM_CMD_LINESPACE_BYTE1   0x1B
#define AXIOHM_CMD_LINESPACE_BYTE2   0x33
#define AXIOHM_CMD_PRINTANDFEEDLINE  0x0A
#define AXIOHM_CMD_FULLCUT           0x19

int SCS_SYNCSYM::SendToPrinter(PrnLinesArray * pPrnLines)
{
	int    ok = -1;
	int    text_output = 0;
	THROW_INVARG(pPrnLines);
	if(PrinterPort.Len()) {
		SFsPath ps(PrinterPort);
		if(ps.Drv.NotEmpty() && ps.Dir.NotEmpty() && ps.Nam.NotEmpty() && ps.Ext.NotEmpty()) { // @v11.1.1
			SFile f_out(PrinterPort, SFile::mAppend);
			for(uint i = 0; i < pPrnLines->getCount(); i++) {
				PrnLineStruc * p_prn_line = pPrnLines->at(i);
				p_prn_line->PrnBuf.Transf(CTRANSF_OUTER_TO_UTF8).CR();
				f_out.WriteLine(p_prn_line->PrnBuf);
			}
		}
		else if(TextOutput) {
			int    port_no = 0;
			DWORD  sz = 0;
			SString name;
			HANDLE h_port = INVALID_HANDLE_VALUE;
			HANDLE printer = INVALID_HANDLE_VALUE;
			THROW(OpenPrinter(const_cast<TCHAR *>(SUcSwitch(PrinterPort)), &printer, NULL)); // @unicodeproblem
			if(printer != INVALID_HANDLE_VALUE) {
				DWORD info_size = 0;
				GetPrinter(printer, 2, NULL, info_size, &info_size);
				if(info_size) {
					PRINTER_INFO_2 * p_prn_info = static_cast<PRINTER_INFO_2 *>(SAlloc::M(info_size));
					if(p_prn_info) {
						memzero(p_prn_info, info_size);
						if(GetPrinter(printer, 2, PTR8(p_prn_info), info_size, &info_size))
							GetPort(SUcSwitch(p_prn_info->pPortName), &port_no); // @unicodeproblem
						SAlloc::F(p_prn_info);
					}
				}
			}
			ClosePrinter(printer);
			// Если номер com-порта не определен, то по умолчанию будет com1
			name.Z().CatCharN('\\', 2).Dot().BSlash().Cat("COM").Cat(port_no+1);
			if(h_port != INVALID_HANDLE_VALUE) {
				CloseHandle(h_port);
				h_port = INVALID_HANDLE_VALUE;
			}
			h_port = ::CreateFile(SUcSwitch(name), GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0); // @unicodeproblem
			// Ставим кодовую таблицу CP-866
			{
				const char cmd[] = { AXIOHM_CMD_SETCHARTBL_BYTE1, AXIOHM_CMD_SETCHARTBL_BYTE2, AXIOHM_CMD_CODETABL_CP866_ID };
				THROW(WriteFile(h_port, cmd, sizeof(cmd), &sz, 0));
			}
			// Установим меньшее расстояние между строками (специально для евреев)
			{
				const char cmd[] = { AXIOHM_CMD_LINESPACE_BYTE1, AXIOHM_CMD_LINESPACE_BYTE2, 1 };
				THROW(WriteFile(h_port, cmd, sizeof(cmd), &sz, 0));
			}
			for(uint i = 0; i < pPrnLines->getCount(); i++) {
				PrnLineStruc * p_prn_line = pPrnLines->at(i);
				p_prn_line->PrnBuf.CatChar(AXIOHM_CMD_PRINTANDFEEDLINE).Transf(CTRANSF_OUTER_TO_INNER);
				THROW(WriteFile(h_port, p_prn_line->PrnBuf, p_prn_line->PrnBuf.Len(), &sz, 0));
				sz = 0;
			}
			// В конце шлем команду отрезки чека
			{
				const char cmd = AXIOHM_CMD_FULLCUT;
				sz = 0;
				THROW(WriteFile(h_port, &cmd, 1, &sz, 0));
			}
			CloseHandle(h_port);
			ok = 1;
		}
		else {
			PrinterDC = ::CreateDC(_T("WINSPOOL\0"), SUcSwitch(PrinterPort), 0, 0);
			if(PrinterDC) {
				const char * p_font_face = "Courier";
				DOCINFO di;
				COLORREF old_color = SetTextColor(PrinterDC, GetColorRef(SClrBlack));
				//
				// Set printer font
				//
				SetBkMode(PrinterDC, TRANSPARENT);
				INITWINAPISTRUCT(di);
				di.lpszDocName = _T("Check");
				THROW(StartDoc(PrinterDC, &di) != SP_ERROR);
				{
					const  int w = GetDeviceCaps(PrinterDC, HORZRES);
					const  int h = GetDeviceCaps(PrinterDC, VERTRES);
					HFONT  font = 0;
					HFONT  old_font = 0;
					RECT   rc;
					rc.top    = 2;
					rc.left   = 2;
					rc.right  = w - 4;
					rc.bottom = h - 4;
					StartPage(PrinterDC);
					for(uint i = 0; i < pPrnLines->getCount(); i++) {
						long   height = 0;
						PrnLineStruc * p_prn_line = pPrnLines->at(i);
						Graphics graphics(PrinterDC);
						if(p_prn_line->Param.PictPath.NotEmpty()) {
							RECT coord = p_prn_line->Param.PictCoord;
							SImage img;
							img.Load(p_prn_line->Param.PictPath);
							coord.top  = rc.top;
							SETIFZ(coord.right,  (long)img.GetWidth());
							SETIFZ(coord.bottom, (long)img.GetHeight());
							SETIFZ(coord.left, rc.left);
							img.Draw(PrinterDC, &coord, 0, 1);
							height = coord.bottom;
						}
						else {
							Gdiplus::REAL font_height = (p_prn_line->Param.FontSize) ? (Gdiplus::REAL)p_prn_line->Param.FontSize : (Gdiplus::REAL)8.0;
							WCHAR  font_name[64];
							memzero(font_name, sizeof(font_name));
							if(p_prn_line->Param.FontName.NotEmpty())
								MultiByteToWideChar(1251, 0, p_prn_line->Param.FontName, p_prn_line->Param.FontName.Len(), font_name, SIZEOFARRAY(font_name));
							else
								MultiByteToWideChar(1251, 0, p_font_face, sstrlen(p_font_face), font_name, SIZEOFARRAY(font_name));
							{
								PointF start_coord;
								Font ffont(font_name, font_height);
								SolidBrush black_brush(Color(255, 0, 0, 0));
								WCHAR  out_buf[512];
								StringFormat format;
								format.SetAlignment(StringAlignmentNear);
								start_coord.X = (Gdiplus::REAL)rc.left;
								start_coord.Y = (Gdiplus::REAL)rc.top;
								memzero(out_buf, sizeof(out_buf));
								MultiByteToWideChar(1251, 0, p_prn_line->PrnBuf, sstrlen(p_prn_line->PrnBuf), out_buf, SIZEOFARRAY(out_buf));
								graphics.DrawString(out_buf, sstrlen(p_prn_line->PrnBuf), &ffont, start_coord, &format, &black_brush);
							}
							height = (long)font_height + 4;
						}
						rc.top += labs(height);
						if((rc.top + labs(height)) > rc.bottom) {
							rc.top = 2;
							EndPage(PrinterDC);
							StartPage(PrinterDC);
						}
					}
					EndPage(PrinterDC);
					EndDoc(PrinterDC);
				}
				SetTextColor(PrinterDC, old_color);
				ok = 1;
			}
		}
	}

	//if(PrinterPort.Len() && !PrinterDC)
	//	PrinterDC = CreateDC("WINSPOOL\0", PrinterPort, 0, 0);
	//if(PrinterDC) {
	//	const char * p_font_face = "Courier";
	//	DOCINFO di;
	//	COLORREF old_color = SetTextColor(PrinterDC, GetColorRef(SClrBlack));
	//	//
	//	// Set printer font
	//	//
	//	SetBkMode(PrinterDC, TRANSPARENT);
	//	MEMSZERO(di);
	//	di.cbSize = sizeof(DOCINFO);
	//	di.lpszDocName = "Check";
	//	di.lpszOutput = (LPTSTR)NULL;
	//	di.lpszDatatype = (LPTSTR)NULL;
	//	di.fwType = 0;
	//	THROW(StartDoc(PrinterDC, &di) != SP_ERROR);
	//	{
	//		int    w = GetDeviceCaps(PrinterDC, HORZRES), h = GetDeviceCaps(PrinterDC, VERTRES);
	//		HFONT  font = 0, old_font = 0;
	//		RECT   rc;
	//		MEMSZERO(rc);
	//		rc.top    = 2;
	//		rc.left   = 2;
	//		rc.right  = w - 4;
	//		rc.bottom = h - 4;
	//		StartPage(PrinterDC);
	//		for(uint i = 0; i < pPrnLines->getCount(); i++) {
	//			long   height = 0;
	//			PrnLineStruc * p_prn_line = pPrnLines->at(i);
	//			Graphics graphics(PrinterDC);
	//			if(p_prn_line->Param.PictPath.NotEmpty()) {
	//				RECT coord = p_prn_line->Param.PictCoord;
	//				SImage img;
	//				img.LoadImage(p_prn_line->Param.PictPath);
	//				coord.top  = rc.top;
	//				SETIFZ(coord.right,  (long)img.GetWidth());
	//				SETIFZ(coord.bottom, (long)img.GetHeight());
	//				SETIFZ(coord.left, rc.left);
	//				img.Draw(PrinterDC, &coord, 0, 1);
	//				height = coord.bottom;
	//			}
	//			else {
	//				Gdiplus::REAL font_height = (p_prn_line->Param.FontSize) ? (Gdiplus::REAL)p_prn_line->Param.FontSize : (Gdiplus::REAL)8.0;
	//				WCHAR  font_name[64];
	//				memzero(font_name, sizeof(font_name));
	//				if(p_prn_line->Param.FontName.NotEmpty())
	//					MultiByteToWideChar(1251, 0, p_prn_line->Param.FontName, p_prn_line->Param.FontName.Len(), font_name, SIZEOFARRAY(font_name));
	//				else
	//					MultiByteToWideChar(1251, 0, p_font_face, sstrlen(p_font_face), font_name, SIZEOFARRAY(font_name));
	//				{
	//					PointF start_coord;
	//					Font ffont(font_name, font_height);
	//					SolidBrush black_brush(Color(255, 0, 0, 0));
	//					WCHAR  out_buf[512];
	//					StringFormat format;
	//					format.SetAlignment(StringAlignmentNear);
	//					start_coord.X = (Gdiplus::REAL)rc.left;
	//					start_coord.Y = (Gdiplus::REAL)rc.top;
	//					memzero(out_buf, sizeof(out_buf));
	//					MultiByteToWideChar(1251, 0, p_prn_line->PrnBuf, sstrlen(p_prn_line->PrnBuf), out_buf, SIZEOFARRAY(out_buf));
	//					graphics.DrawString(out_buf, sstrlen(p_prn_line->PrnBuf), &ffont, start_coord, &format, &black_brush);
	//				}
	//				height = (long)font_height + 4;
	//			}
	//			rc.top += labs(height);
	//			if((rc.top + labs(height)) > rc.bottom) {
	//				rc.top = 2;
	//				EndPage(PrinterDC);
	//				StartPage(PrinterDC);
	//			}
	//		}
	//		EndPage(PrinterDC);
	//		EndDoc(PrinterDC);
	//	}
	//	SetTextColor(PrinterDC, old_color);
	//	ok = 1;
	//}
	CATCHZOK
	return ok;
}

/*virtual*/int SCS_SYNCSYM::PrintCheck(CCheckPacket * pPack, uint flags)
{
	int     ok = 1;
	THROW_INVARG(pPack);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
		double sum = fabs(pPack->_Cash) + 0.001;
		double running_total = 0.0;
		SlipDocCommonParam sdc_param;
		if(P_SlipFmt) {
			int    r = 0;
			SString temp_buf;
			SString line_buf;
			const SString format_name((flags & PRNCHK_RETURN) ? "CCheckRet" : "CCheck");
			SlipLineParam sl_param;
			// @v12.2.9 {
			if(PPRef->Ot.GetTagStr(PPOBJ_CASHNODE, SCn.ID, PPTAG_POSNODE_TEST, temp_buf) > 0) {
				OfdFactors ofdf;
				GetOfdFactors(ofdf);
				SJson * p_js = AtolDrv_MakeJson_CCheck(ofdf, pPack, P_SlipFmt, 0/*flags*/);
				if(p_js) {
					SString file_name;
					(temp_buf = "posnode_debug_ccheck").CatChar('_').CatChar('#').Cat(SCn.ID).DotCat("txt");
					PPGetFilePath(PPPATH_OUT, temp_buf, file_name);
					if(p_js->ToStr(temp_buf)) {
						SFile f_out(file_name, SFile::mAppend);
						if(f_out.IsValid()) {
							f_out.WriteLine(temp_buf.CR());
						}
					}
					ZDELETE(p_js);
				}
			} // } @v12.2.9
			else if(PrinterPort.Len()) {
				PrnLinesArray prn_list;
				TextOutput = sdc_param.TextOutput; // @vmiller
				SETIFZ(pPack->Rec.Dt, getcurdate_());
				SETIFZ(pPack->Rec.Tm, getcurtime_());
				THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
				if(r > 0) {
					for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
						if(sl_param.Flags & SlipLineParam::fRegFiscal) {
							double _q = sl_param.Qtty;
							double _p = sl_param.Price;
							running_total += (_q * _p);
						}
						{
							PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
							THROW_SL(p_prn_ls);
							p_prn_ls->PrnBuf = line_buf;
							p_prn_ls->Param = sl_param;
						}
					}
					temp_buf.Z().Dot();
					sl_param.Z();
					{
						PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
						THROW_SL(p_prn_ls);
						p_prn_ls->PrnBuf = temp_buf;
						p_prn_ls->Param  = sl_param;
					}
					SendToPrinter(&prn_list);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SCS_SYNCSYM::PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	int     ok = 1;
	if(PrinterPort.Len()) {
		SlipDocCommonParam  sdc_param;
		THROW_INVARG(pPack);
		if(P_SlipFmt) {
			int   r = 0;
			SString line_buf;
			const SString format_name(isempty(pFormatName) ? ((flags & PRNCHK_RETURN) ? "CCheckRetCopy" : "CCheckCopy") : pFormatName);
			SlipLineParam sl_param;
			THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
			if(r > 0) {
				SString buf;
				PrnLinesArray prn_list;
				TextOutput = sdc_param.TextOutput; // @vmiller
				for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
					p_prn_ls->PrnBuf = line_buf;
					p_prn_ls->Param = sl_param;
				}
				{
					buf.Z().Dot();
					PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
					p_prn_ls->PrnBuf = buf;
					p_prn_ls->Param  = sl_param.Z();
				}
				SendToPrinter(&prn_list);
			}
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SCS_SYNCSYM::PrintXReport(const CSessInfo * pSessInfo)
{
	int     ok = 1;
	SlipDocCommonParam  sdc_param;
	if(PrinterPort.Len() && pSessInfo && P_SlipFmt) {
		int   r = 0;
		SString line_buf;
		const SString format_name("XReport");
		SlipLineParam sl_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SString buf;
			PrnLinesArray prn_list;
			TextOutput = sdc_param.TextOutput; // @vmiller
			for(P_SlipFmt->InitIteration(pSessInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
				p_prn_ls->PrnBuf = line_buf;
				p_prn_ls->Param = sl_param;
			}
			{
				buf.Z().Dot();
				PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
				p_prn_ls->PrnBuf = buf;
				p_prn_ls->Param  = sl_param.Z();
			}
			SendToPrinter(&prn_list);
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SCS_SYNCSYM::PrintZReportCopy(const CSessInfo * pSessInfo)
{
	int     ok = 1;
	if(PrinterPort.Len()) {
		SlipDocCommonParam  sdc_param;
		THROW_INVARG(pSessInfo);
		if(P_SlipFmt) {
			int   r = 0;
			SString line_buf;
			const SString format_name("ZReportCopy");
			SlipLineParam sl_param;
			THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
			if(r > 0) {
				SString buf;
				PrnLinesArray prn_list;
				TextOutput = sdc_param.TextOutput; // @vmiller
				for(P_SlipFmt->InitIteration(pSessInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
					p_prn_ls->PrnBuf = line_buf;
					p_prn_ls->Param = sl_param;
				}
				{
					buf.Z().Dot();
					PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
					p_prn_ls->PrnBuf = buf;
					p_prn_ls->Param  = sl_param.Z();
				}
				SendToPrinter(&prn_list);
			}
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SCS_SYNCSYM::CloseSession(PPID sessID)
{
	int    ok = -1;
	CSessInfo cs_info;
	if(sessID && CsObj.Search(sessID, &cs_info.Rec) > 0) {
		long _f = CsObj.P_Tbl->GetCcGroupingFlags(cs_info.Rec, 0);
		_f |= CCheckCore::gglfUseFullCcPackets;
		if(CsObj.P_Cc->GetSessTotal(sessID, _f, &cs_info.Total, 0) > 0)
			ok = PrintZReportCopy(&cs_info);
	}
	return ok;
}

int SCS_SYNCSYM::GetSummator(double * val)
{
	ASSIGN_PTR(val, -1.0);
	return 1;
}

// @vmiller
int SCS_SYNCSYM::OpenBox()
{
	int    ok = 1, port_no = 0, c = 0, is_err = 0;
	SCommPort comm_port;
	SString drawer_line, drawer_port, drawer_cmd, name, word;
	long   s_com = 0x004D4F43L; // "COM"
	DWORD  sz = 0;
	HANDLE h_port = INVALID_HANDLE_VALUE;
	if(PrinterPort.Len() && SCn.Flags & CASHF_OPENBOX){
		SCn.GetPropString(SCN_CASHDRAWER_CMD, drawer_cmd);
		if(!drawer_cmd.NotEmptyS())
			drawer_cmd = "1B70001015";
		SCn.GetPropString(SCN_CASHDRAWER_PORT, drawer_port);
		if(!drawer_port.NotEmptyS())
			drawer_port = "com1";
		GetPort(drawer_port, &port_no);
		name.Z().CatCharN('\\', 2).Dot().BSlash().Cat(reinterpret_cast<const char *>(&s_com)).Cat(port_no+1);
		if(h_port != INVALID_HANDLE_VALUE) {
			CloseHandle(h_port);
			h_port = INVALID_HANDLE_VALUE;
		}
		h_port = ::CreateFile(SUcSwitch(name), GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0); // @unicodeproblem
		SLS.SetAddedMsgString(name);
		THROW(h_port != INVALID_HANDLE_VALUE);
		if(drawer_cmd.Len() % 2 != 0)
			is_err = 1;
		else
			for(uint i = 0; i < drawer_cmd.Len(); i++)
				if(!ishex(drawer_cmd.C(i))) {
					is_err = 1;
					break; // @error
				}
		if(is_err) {
			drawer_cmd = 0;
			PPSetError(PPERR_INVOPENDRAWERCMD, drawer_cmd);
		}
		else {
			const char * p = drawer_cmd;
			while(p[0] && p[1]) {
				uint8 byte = (hex(p[0]) << 4) | hex(p[1]);
				THROW(WriteFile(h_port,  word.Z().CatChar(byte), 1, &sz, 0));
				p += 2;
			}
		}
		CloseHandle(h_port);
	}
	CATCHZOK;
	return ok;
}

// @vmiller
int SCS_SYNCSYM::PrintBnkTermReport(const char * pZCheck)
{
	int     ok = 1;
	if(PrinterPort.Len()) {
		SlipLineParam sl_param;
		PrnLinesArray prn_list;
		StringSet str_set('\n', pZCheck);
		SString str;
		for(uint pos = 0; str_set.get(&pos, str);) {
			sl_param.FontSize = 8; // какое значение? // @v9.7.6 1-->8
			sl_param.Flags = SlipLineParam::fRegRegular;
			PrnLineStruc * p_prn_ls = prn_list.CreateNewItem();
			if(p_prn_ls) {
				p_prn_ls->PrnBuf = str;
				p_prn_ls->Param = sl_param;
			}
		}
		SendToPrinter(&prn_list);
	}
	return ok;
}
