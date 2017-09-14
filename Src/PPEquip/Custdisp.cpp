// CUSTDISP.CPP
//
#include <pp.h>
#pragma hdrstop

int DispArrAdd(StrAssocArray & rArr, int pos, int value)
{
	SString str;
	return rArr.Add(pos, str.Cat(value), 1);
}

int DispArrAdd(StrAssocArray & rArr, int pos, const char* pValue)
{
	return rArr.Add(pos, pValue, 1);
}

// static
int PPCustDisp::IsComPort(const char * pPortName)
{
	int    ok = 0;
	if(pPortName) {
		int  port = 0;
		int  comdvcs = IsComDvcSymb(pPortName, &port);
		if(comdvcs == comdvcsCom && port > 0 && port < 32)
			ok = 1;
	}
	return ok;
}

SLAPI PPCustDisp::PPCustDisp(int dispType, int portNo, long flags, int usb /*=0*/)
{
	Port = portNo;
	DispStrLen = 0;
	State = 0;
	Flags = flags;
	P_DispBuf  = 0;
	P_AbstrDvc = new PPAbstractDevice(0);
	P_AbstrDvc->PCpb.Cls = DVCCLS_DISPLAY;
	P_AbstrDvc->GetDllName(DVCCLS_DISPLAY, dispType, P_AbstrDvc->PCpb.DllName);
	THROW(P_AbstrDvc->IdentifyDevice(P_AbstrDvc->PCpb.DllName));
	THROW(Init(usb));
	CATCH
		State |= stError;
	ENDCATCH
}

SLAPI PPCustDisp::~PPCustDisp()
{
	delete P_DispBuf;
	ExecOper(DVCCMD_DISCONNECT, Arr_In.Clear(), Arr_Out.Clear());
	ExecOper(DVCCMD_RELEASE, Arr_In.Clear(), Arr_Out.Clear());
	ZDELETE(P_AbstrDvc);
}

int SLAPI PPCustDisp::GetConfig()
{
	int    ok = 1;
	SString buf, param_name, param_val;
	Arr_In.Clear();
	THROW(State & stConnected);
	THROW(ExecOper(DVCCMD_GETCONFIG, Arr_In, Arr_Out));
	if(Arr_Out.getCount())
		for(uint i = 0; Arr_Out.Get(i, buf) > 0; i++) {
			buf.Divide('=', param_name, param_val);
			if(strcmpi(param_name, "STRLEN") == 0)
				DispStrLen = param_val.ToLong();
		}
	CATCHZOK;
	return ok;
}

int SLAPI PPCustDisp::Init(int usb /*=0*/)
{
	int    ok = 1;
	SString usb_dll_catalog;
	PPGetPath(PPPATH_BIN, usb_dll_catalog);
	Arr_In.Clear();
	THROW(DispArrAdd(Arr_In, DVCPARAM_DLLPATH, usb_dll_catalog));
	THROW(ExecOper(DVCCMD_INIT, Arr_In, Arr_Out.Clear()));
	THROW(DispArrAdd(Arr_In, DVCPARAM_FLAGS, usb));
	THROW(DispArrAdd(Arr_In, DVCPARAM_PORT, Port));
	THROW(ExecOper(DVCCMD_CONNECT, Arr_In, Arr_Out));
	State |= stConnected;
	THROW(GetConfig());
	THROW(PPLoadText(PPTXT_CUSTDISP_WORDS, Words));
	THROW_MEM(P_DispBuf = new char[DispStrLen + 2]);
	memzero(P_DispBuf, DispStrLen + 2);
	CATCH
		ZDELETE(P_DispBuf);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PPCustDisp::PutString(const char * pStr, int align/*= ALIGN_LEFT*/, int verTab/*= VERTAB_CURPOS*/)
{
	int    ok = 1;
	if(State & stConnected) {
		switch(align) {
			case ALIGN_RIGHT:
				align = HORZNT_RIGHT;
				break;
			case ALIGN_LEFT:
				align = HORZNT_LEFT;
				break;
			case ALIGN_CENTER:
				align = HORZNT_CENTER;
				break;
		}
		strnzcpy(P_DispBuf, pStr, DispStrLen + 1);
		Arr_In.Clear();
		DispArrAdd(Arr_In, DVCPARAM_TEXT, P_DispBuf);
		DispArrAdd(Arr_In, DVCPARAM_VERTAB, verTab);
		DispArrAdd(Arr_In, DVCPARAM_ALIGN, align);
		THROW(ExecOper(DVCCMD_PUTLINE, Arr_In, Arr_Out.Clear()));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::SetCurTime()
{
	int    ok = -1;
	if((Flags & fcdShowTime) && (State & stConnected)) {
		int   h, m;
		LTIME tm;
		SString time_str, minute;
		getcurtime(&tm);
		decodetime(&h, &m, 0, 0, &tm);
		minute.Z().Cat(m);
		if(minute.Len() == 1)
			minute.PadLeft(1, '0');
		time_str.CatChar('T').Cat(h).CatChar(':').Cat(minute);
		THROW(PutString(time_str, ALIGN_LEFT, down));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::ClearDisplay()
{
	int    ok = 1;
	if(State & stConnected)
		ok = ExecOper(DVCCMD_CLEARDISPLAY, Arr_In.Clear(), Arr_Out.Clear());
	return ok;
}

int SLAPI PPCustDisp::OpenOrCloseCash(int open)
{
	int     ok = 1;
	SString buf;
	if(State & stConnected) {
		THROW(ClearDisplay());
		THROW(PPGetSubStr(Words, open ? PPCDY_WELCOME : PPCDY_CASH_NOWORK, buf));
		THROW(PutString(buf, ALIGN_CENTER, upp));
		THROW(SetCurTime());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::OpenedCash()
{
	return OpenOrCloseCash(1);
}

int SLAPI PPCustDisp::ClosedCash()
{
	return OpenOrCloseCash(0);
}

int SLAPI PPCustDisp::SetGoodsName(const char * pName)
{
	int    ok = 1;
	THROW(PutString(pName, ALIGN_LEFT, upp));
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::SetPresent()
{
	int    ok = 1;
	char   buf[64];
	THROW(PPGetSubStr(Words, PPCDY_PRESENT, buf, sizeof(buf)));
	THROW(PutString(buf, ALIGN_LEFT, down));
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::SetAmt(double price, double qtty)
{
	int    ok = 1;
	char   buf[64], * p = 0;
	realfmt(R2(price), MKSFMTD(0, 2, ALIGN_LEFT), buf);
	p = buf + strlen(buf);
	*p++ = '*';
	realfmt(R3(fabs(qtty)), MKSFMTD(0, 3, ALIGN_LEFT | NMBF_NOTRAILZ), p);
	p = buf + strlen(buf);
	*p++ = '=';
	realfmt(R2(fabs(price * qtty)), MKSFMTD(0, 2, ALIGN_LEFT), p);
	THROW(PutString(buf, ALIGN_LEFT, down));
	CATCHZOK
	return ok;
}

static SString & TwoPartLine(SString & rLeft, const SString & rRight, uint maxLen)
{
	int    d = maxLen - (rLeft.Len() + rRight.Len());
	if(d > 0)
		rLeft.CatCharN(' ', d);
	else
		rLeft.Space();
	return rLeft.Cat(rRight);
}

int SLAPI PPCustDisp::SetDiscount(double percent, double discount)
{
	int    ok = 1;
	SString temp_buf, dscnt_buf;
	PPGetSubStr(Words, PPCDY_DISCOUNT, temp_buf);
	temp_buf.Space().Cat(percent, MKSFMTD(0, 2, ALIGN_LEFT | NMBF_NOTRAILZ)).CatChar('%');
	dscnt_buf.Cat(fabs(discount), MKSFMTD(0, 2, ALIGN_LEFT));
	TwoPartLine(temp_buf, dscnt_buf, DispStrLen);
	THROW(PutString(TwoPartLine(temp_buf, dscnt_buf, DispStrLen), ALIGN_LEFT, down));
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::SetTotal(double sum)
{
	int    ok = 1;
	SString temp_buf, val_buf;
	PPGetSubStr(Words, PPCDY_TOTAL, temp_buf);
	val_buf.Cat(fabs(sum), MKSFMTD(0, 2, ALIGN_LEFT));
	THROW(PutString(TwoPartLine(temp_buf, val_buf, DispStrLen), ALIGN_LEFT, upp));
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::SetChange(double cash, double change)
{
	int    ok = 1;
	SString temp_buf, val_buf;
	PPGetSubStr(Words, PPCDY_CASH, temp_buf);
	val_buf.Cat(fabs(cash), MKSFMTD(0, 2, ALIGN_LEFT));
	THROW(PutString(TwoPartLine(temp_buf, val_buf, DispStrLen), ALIGN_LEFT, upp));
	//
	PPGetSubStr(Words, PPCDY_CHANGE, temp_buf);
	val_buf.Z().Cat(fabs(change), MKSFMTD(0, 2, ALIGN_LEFT));
	THROW(PutString(TwoPartLine(temp_buf, val_buf, DispStrLen), ALIGN_LEFT, down));
	CATCHZOK
	return ok;
}

int SLAPI PPCustDisp::ExecOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	if((ok = P_AbstrDvc->RunCmd__(cmd, rIn, rOut)) != 1) {
		SString err_msg;
		rOut.Get(0, err_msg);
		if(P_AbstrDvc->RunCmd__(DVCCMD_GETLASTERRORTEXT, rIn.Clear(), rOut.Clear()))
			rOut.Get(0, err_msg);
		if(err_msg.NotEmpty())
			PPSetError(PPERR_CUSTDISP, err_msg.Transf(CTRANSF_OUTER_TO_INNER));
		ok = 0;
	}
	return ok;
}

PPCustDisp * SLAPI GetCustDisp(PPID custDispID, char * pPortName, int usb)
{
	PPCustDisp * p_cust_disp = 0;
	if(custDispID && pPortName) {
		int    port = 0;
		if(!(usb & PPSyncCashNode::cdfUsb)) {
			THROW_PP(PPCustDisp::IsComPort(strip(pPortName)), PPERR_CUSTDISP_INVPORT);
			GetPort(pPortName, &port);
		}
		/*
		if(custDispID == PPCUSTDISP_VFD)
			p_cust_disp = new PPCustDisp(PPCUSTDISP_VFD, port, PPCustDisp::fcdShowTime);
		else if(custDispID == PPCUSTDISP_DPD201)
			p_cust_disp = new PPCustDisp(PPCUSTDISP_DPD201, port, 0);
		else if(custDispID == PPCUSTDISP_POSIFLEX)
			p_cust_disp = new PPCustDisp(PPCUSTDISP_POSIFLEX, port, 0);
			else
		*/
		THROW_MEM(p_cust_disp = new PPCustDisp(custDispID, port, PPCustDisp::fcdShowTime, BIN(usb & PPSyncCashNode::cdfUsb)));
		THROW(!p_cust_disp->IsError());
	}
	CATCH
		PPSaveErrContext();
		ZDELETE(p_cust_disp);
		PPRestoreErrContext();
		PPError();
	ENDCATCH
	return p_cust_disp;
}
