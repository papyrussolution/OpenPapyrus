// PPDRVAPI.CPP
// Copyright (c) A.Sobolev 2013, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#pragma hdrstop
#include <ppdrvapi.h>

/*static*/SString & PPDrvInputParamBlock::EncodeText(SString & rBuf)
{
	// ';' encoded as &{smcol}
	// '=' encoded as &{equal}
	return rBuf.ReplaceStr(";", "&{smcol}", 0).ReplaceStr("=", "&{equal}", 0);
}

/*static*/SString & PPDrvInputParamBlock::DecodeText(SString & rBuf)
{
	return rBuf.ReplaceStr("&{smcol}", ";", 0).ReplaceStr("&{equal}", "=", 0);
}

PPDrvInputParamBlock::PPDrvInputParamBlock(const char * pInputText) : Ss(';', pInputText)
{
}

int PPDrvInputParamBlock::Add(const char * pParam, const char * pValue)
{
	int    ok = 1;
	if(!isempty(pParam) && !isempty(pValue)) {
		TempBuf.Z().Cat(pParam).Strip();
		TempBuf.CatChar('=').Cat(EncodeText(ValBuf.Z().Cat(pValue).Strip()));
		Ss.add(TempBuf);
	}
	else
		ok = 0;
	return ok;
}

SString & PPDrvInputParamBlock::GetRawBuf(SString & rBuf) const
{
	rBuf = Ss.getBuf();
	return rBuf;
}

int PPDrvInputParamBlock::Get(const char * pParam, SString & rValue)
{
	rValue.Z();
	for(uint i = 0; Ss.get(&i, TempBuf);) {
		TempBuf.Divide('=', KeyBuf, ValBuf);
		if(KeyBuf.Strip().IsEqiAscii(pParam)) {
			rValue = DecodeText(ValBuf.Strip());
			return rValue.NotEmpty() ? 1 : -1;
		}
	}
	return 0;
}
//
//
//
int PPDrvSession::ImplementDllMain(HANDLE hModule, DWORD dwReason)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH: Init((HINSTANCE)hModule); break;
		case DLL_THREAD_ATTACH: InitThread(); break;
		case DLL_THREAD_DETACH: ReleaseThread(); break;
		case DLL_PROCESS_DETACH: break;
	}
	return TRUE;
}

SLAPI PPDrvSession::PPDrvSession(const char * pName, PPDrv_CreateInstanceProc proc,
	uint verMajor, uint verMinor, uint errMsgCount, const SIntToSymbTabEntry * pErrMsgTab) :
	Name(pName), Proc(proc), VerMajor(verMajor), VerMinor(verMinor)
{
	Id = 1;
	TlsIdx = -1L;
#ifdef _MT
	TlsIdx = TlsAlloc();
	InitThread();
#endif
	{
		DefineErrText(PPBaseDriver::serrNotInited, "Интерфейс драйвера не инициализирован");
		DefineErrText(PPBaseDriver::serrInvCommand, "Недопустимая команда");
		DefineErrText(PPBaseDriver::serrNoMem, "Недостаточно памяти");
		DefineErrText(PPBaseDriver::serrNotEnoughRetBufLen, "Недостаточен размер буфера возвращаемого результата");
	}
	if(errMsgCount && pErrMsgTab)
		DefineErrTextTable(errMsgCount, pErrMsgTab);
}

#define SIGN_PPDRVTLA 0x7D08E312L

SLAPI PPDrvThreadLocalArea::PPDrvThreadLocalArea() : Sign(SIGN_PPDRVTLA), Id(0), TId(0), LastErr(0), State(0), I(0), P_FinishEvnt(0)
{
}

SLAPI PPDrvThreadLocalArea::~PPDrvThreadLocalArea()
{
	Sign = 0;
	ZDELETE(I);
	ZDELETE(P_FinishEvnt);
}

int PPDrvThreadLocalArea::ResetFinishEvent()
{
	if(P_FinishEvnt) {
		P_FinishEvnt->Reset();
		return 1;
	}
	else
		return 0;
}

int PPDrvThreadLocalArea::SignalFinishEvent()
{
	if(P_FinishEvnt) {
		P_FinishEvnt->Signal();
		return 1;
	}
	else
		return 0;
}

long SLAPI PPDrvThreadLocalArea::GetId() const { return Id; }
ThreadID SLAPI PPDrvThreadLocalArea::GetThreadID() const { return TId; }
int SLAPI PPDrvThreadLocalArea::IsConsistent() const { return BIN(Sign == SIGN_PPDRVTLA); }

SLAPI PPDrvSession::~PPDrvSession()
{
#ifdef _MT
	ReleaseThread();
	TlsFree(TlsIdx);
#endif
}

int SLAPI PPDrvSession::Init(HINSTANCE hInst)
{
	int    ok = 1;
	SString product_name;
	(product_name = "ppdrv").CatChar('-').Cat(Name);
	SLS.Init(product_name, hInst);
	return ok;
}

int SLAPI PPDrvSession::InitThread()
{
	PPDrvThreadLocalArea * p_tla = 0;
#ifdef _MT
	SLS.InitThread();
	p_tla = new PPDrvThreadLocalArea;
	TlsSetValue(TlsIdx, p_tla);
	p_tla->TId = 0;
#else
	p_tla = &Tla;
	p_tla->TId = 0;
#endif
	p_tla->Id = LastThread.Incr();
	return 1;
}

int SLAPI PPDrvSession::ReleaseThread()
{
#ifdef _MT
	PPDrvThreadLocalArea * p_tla = static_cast<PPDrvThreadLocalArea *>(TlsGetValue(TlsIdx));
	if(p_tla) {
		delete p_tla;
		TlsSetValue(TlsIdx, 0);
	}
	SLS.ReleaseThread();
#endif
	return 1;
}

PPDrvThreadLocalArea & SLAPI PPDrvSession::GetTLA() { return *static_cast<PPDrvThreadLocalArea *>(TlsGetValue(TlsIdx)); }
const PPDrvThreadLocalArea & SLAPI PPDrvSession::GetConstTLA() const { return *static_cast<const PPDrvThreadLocalArea *>(TlsGetValue(TlsIdx)); }
PPBaseDriver * PPDrvSession::GetI() { return GetTLA().I; }
void PPDrvSession::SetErrCode(int errCode) { GetTLA().LastErr = errCode; }
int PPDrvSession::GetLastErr() { return GetConstTLA().LastErr; }

void PPDrvSession::DefineErrText(int errCode, const char * pText)
{
	ErrText.Add(errCode, pText, 1);
}

void PPDrvSession::DefineErrTextTable(uint numEntries, const SIntToSymbTabEntry * pTab)
{
	if(pTab)
		for(uint i = 0; i < numEntries; i++)
			ErrText.Add(pTab[i].Id, pTab[i].P_Symb, 1);
}

int PPDrvSession::GetErrText(int errCode, SString & rBuf)
{
	if(errCode < 0)
		errCode = GetConstTLA().LastErr;
	int    ok = ErrText.GetText(errCode, rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_OUTER); // @v10.4.5
	return ok;
}

void PPDrvSession::SetLogFileName(const char * pFileName)
{
	Cs_Log.Enter();
	(LogFileName = pFileName).Strip();
	Cs_Log.Leave();
}

int PPDrvSession::Log(const char * pMsg, long flags)
{
	int    ok = 1;
	Cs_Log.Enter();
	SString msg_buf;
	if(flags & logfTime) {
		msg_buf.Cat(getcurdatetime_(), DATF_DMY, TIMF_HMS).Tab();
	}
	if(flags & logfName) {
		msg_buf.Cat(Name).Tab();
	}
	if(flags & logfThreadId) {
		const PPDrvThreadLocalArea & r_tla = GetConstTLA();
		msg_buf.CatEq("ThreadId", r_tla.GetId()).Tab();
	}
	msg_buf.Cat(pMsg);
	SLS.LogMessage(LogFileName.NotEmpty() ? (const char *)LogFileName : 0, msg_buf, 8192);
	Cs_Log.Leave();
	return ok;
}

int PPDrvSession::Helper_ProcessCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	PPDrvThreadLocalArea & r_tla = GetTLA();

	int    err = 0;
	SString cmd_buf, output;
	SString name_buf;
	(cmd_buf = pCmd).Strip().ToUpper();
	if(cmd_buf.NotEmpty() && pOutputData && outSize) {
		if(cmd_buf == "INIT") {
			if(Proc) {
				ZDELETE(r_tla.I);
				r_tla.I = Proc();
			}
			if(r_tla.I)
				err = r_tla.I->ProcessCommand(cmd_buf, pInputData, output);
			else
				err = 1;
		}
		else if(cmd_buf == "GETLASTERRORTEXT") {
			GetErrText(GetLastErr(), output);
		}
		else if(cmd_buf == "SETLOGFILE") {
			PPDrvInputParamBlock ipb(pInputData);
			if(ipb.Get("NAME", name_buf) > 0) {
				SetLogFileName(name_buf);
			}
		}
		else if(r_tla.I) {
			if(cmd_buf == "RELEASE") {
				r_tla.I->ProcessCommand(cmd_buf, pInputData, output);
				ZDELETE(r_tla.I);
			}
			else if(cmd_buf == "SETFINISHEVENT") {
				PPDrvInputParamBlock ipb(pInputData);
				ZDELETE(r_tla.P_FinishEvnt);
				if(ipb.Get("NAME", name_buf) > 0 && !name_buf.IsEqiAscii("NULL")) {
					r_tla.P_FinishEvnt = new Evnt(name_buf, Evnt::modeOpen);
				}
			}
			else {
				r_tla.ResetFinishEvent();
				err = r_tla.I->ProcessCommand(cmd_buf, pInputData, output);
				r_tla.SignalFinishEvent();
			}
		}
	}
	if(err) {
		output.Z().Cat(r_tla.LastErr);
	}
	if((output.Len()+1) > outSize) {
		SETIFZ(err, 2);
	}
	output.CopyTo(pOutputData, outSize);
	return err;
}

PPBaseDriver::PPBaseDriver() : Capability(0)
{
}

PPBaseDriver::~PPBaseDriver()
{
}

int PPBaseDriver::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	return 0;
}
