// BNKTERMINAL.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

int BnkTermArrAdd(StrAssocArray & rArr, int pos, int value)
{
	SString str;
	return rArr.Add(pos, str.Cat(value), 1);
}

int BnkTermArrAdd(StrAssocArray & rArr, int pos, const char * pValue)
{
	return rArr.Add(pos, pValue, 1);
}

PPBnkTerminal::PPBnkTerminal(PPID bnkTermID, uint logNum, int port, const char * pPath) : State(0), P_AbstrDvc(new PPAbstractDevice(""))
{
	P_AbstrDvc->PCpb.Cls = DVCCLS_BNKTERM;
	P_AbstrDvc->GetDllName(DVCCLS_BNKTERM, bnkTermID, P_AbstrDvc->PCpb.DllName);
	if(P_AbstrDvc->IdentifyDevice(P_AbstrDvc->PCpb.DllName)) {
		Init(pPath);
		if(State & stInited) {
			Connect(port);
			if(State & stConnected)
				SetConfig(logNum);
		}
	}
}

PPBnkTerminal::~PPBnkTerminal()
{
	Disconnect();
	Release();
	ZDELETE(P_AbstrDvc);
}

int PPBnkTerminal::IsInited() const
{
	return BIN(State & stInited);
}

int PPBnkTerminal::IsConnected() const
{
	return BIN(State & stConnected);
}

int PPBnkTerminal::Init(const char * pPath)
{
	int    ok = 1;
	Arr_In.Clear();
	if(!isempty(pPath))
		BnkTermArrAdd(Arr_In, DVCPARAM_DLLPATH, pPath);
	Arr_Out.Clear();
	ok = ExecOper(DVCCMD_INIT, Arr_In, Arr_Out);
	if(ok)
		State |= stInited;
	else
		PPError();
	return ok;
}

int PPBnkTerminal::Release()
{
	int    ok = 1;
	Arr_In.Clear();
	Arr_Out.Clear();
	ok = ExecOper(DVCCMD_RELEASE, Arr_In, Arr_Out);
	return ok;
}

int PPBnkTerminal::Connect(int port)
{
	int    ok = 1;
	SString msg;
	Arr_In.Clear();
	BnkTermArrAdd(Arr_In, DVCPARAM_PORT, port);
	Arr_Out.Clear();
	PPWait(1);
	PPWaitMsg(PPLoadTextS(PPTXT_BNKTRM_TESTCONN, msg));
	ok = ExecOper(DVCCMD_CONNECT, Arr_In, Arr_Out);
	PPWait(0);
	if(ok)
		State |= stConnected;
	else
		PPError();
	return ok;
}

int PPBnkTerminal::Disconnect()
{
	return 1;
}

int PPBnkTerminal::SetConfig(uint logNum)
{
	Arr_In.Clear();
	Arr_Out.Clear();
	BnkTermArrAdd(Arr_In, DVCPARAM_LOGNUM, logNum);
	return ExecOper(DVCCMD_SETCFG, Arr_In, Arr_Out);
}

int PPBnkTerminal::Pay(double amount, SString & rSlip)
{
	rSlip = 0;
	int    ok = 1;
	SString msg;
	Arr_In.Clear();
	Arr_Out.Clear();
	amount *= 100; // Переведем в копейки
	BnkTermArrAdd(Arr_In, DVCPARAM_AMOUNT, (int)amount);
	PPWait(1);
	PPWaitMsg(PPLoadTextS(PPTXT_BNKTRM_PAYMENT, msg));
	ok = ExecOper(DVCCMD_PAY, Arr_In, Arr_Out);
	if(ok) {
		Arr_Out.Get(0, rSlip);
	}
	PPWait(0);
	return ok;
}

int PPBnkTerminal::Refund(double amount, SString & rSlip)
{
	rSlip = 0;
	int    ok = 1;
	SString msg;
	Arr_In.Clear();
	Arr_Out.Clear();
	amount *= 100; // Переведем в копейки
	BnkTermArrAdd(Arr_In, DVCPARAM_AMOUNT, (int)amount);
	PPWait(1);
	PPWaitMsg(PPLoadTextS(PPTXT_BNKTRM_RETURN, msg));
	ok = ExecOper(DVCCMD_REFUND, Arr_In, Arr_Out);
	if(ok) {
		Arr_Out.Get(0, rSlip);
	}
	PPWait(0);
	return ok;
}

int PPBnkTerminal::GetSessReport(SString & rZCheck)
{
	int    ok = 1;
	SString msg;
	Arr_In.Clear();
	Arr_Out.Clear();
	PPLoadTextS(PPTXT_BNKTRM_CLOSESESS, msg);
	PPWait(1);
	PPWaitMsg(msg.Transf(CTRANSF_OUTER_TO_INNER));
	if(ok = ExecOper(DVCCMD_GETBANKREPORT, Arr_In, Arr_Out))
		Arr_Out.Get(0, rZCheck);
	PPWait(0);
	return ok;
}

int PPBnkTerminal::ExecOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	if(P_AbstrDvc->RunCmd__(cmd, rIn, rOut) != 1) {
		rIn.Clear();
		rOut.Clear();
		SString err_msg;
		if(P_AbstrDvc->RunCmd__(DVCCMD_GETLASTERRORTEXT, rIn, rOut))
			rOut.Get(0, err_msg);
		if(err_msg.NotEmpty())
			PPSetError(PPERR_BNKTERM, err_msg.Transf(CTRANSF_OUTER_TO_INNER));
		ok = 0;
	}
	return ok;
}

PPBnkTerminal * SLAPI GetBnkTerm(PPID bnkTermID, uint logNum, const char * pPort, const char * pPath)
{
	PPBnkTerminal * p_pos_term = 0;
	if(bnkTermID) {
		int    port = 0;
		GetPort(pPort, &port);
		THROW_MEM(p_pos_term = new PPBnkTerminal(bnkTermID, logNum, port, pPath));
		if(!p_pos_term->IsInited() || !p_pos_term->IsConnected())
			ZDELETE(p_pos_term);
	}
	CATCH
		ZDELETE(p_pos_term);
		PPError();
	ENDCATCH
	return p_pos_term;
}
