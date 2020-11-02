// PPSPROT.CPP
// Copyright (c) A.Sobolev 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
PPJobSrvReply::PPJobSrvReply(PPServerSession * pSess) : PPJobSrvProtocol(), P_Sess(pSess)
{
}

int PPJobSrvReply::StartWriting()
{
	int    ok = 1;
	SetDataType(htGeneric, 0);
	MEMSZERO(H);
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = DataType;
	Z();
	THROW_SL(Write(&H, sizeof(H)));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

void PPJobSrvReply::SetDataType(int dataType, const char * pDataTypeText)
{
	DataType = dataType;
	DataTypeText = pDataTypeText;
}

int FASTCALL PPJobSrvReply::FinishWriting(int hdrFlags)
{
	int    ok = 1;
	if(State & stStructured) {
		Header * p_hdr = static_cast<Header *>(Ptr(GetRdOffs()));
		p_hdr->DataLen = static_cast<int32>(GetAvailableSize());
		p_hdr->Type = DataType;
		if(hdrFlags)
			p_hdr->Flags |= hdrFlags;
	}
	else {
		Write("\xD\xA", 2);
	}
	return ok;
}

void FASTCALL PPJobSrvReply::SetString(const char * pStr)
{
	Z();
	if(pStr)
		Write(pStr, sstrlen(pStr));
	State &= ~stStructured;
	State &= ~stReading;
}

void FASTCALL PPJobSrvReply::SetString(const SString & rStr)
{
	Z();
	Write(rStr, rStr.Len());
	State &= ~stStructured;
	State &= ~stReading;
}

int FASTCALL PPJobSrvReply::SetInformer(const char * pMsg)
{
	int    ok = 1;
	THROW(StartWriting());
	if(pMsg) {
		SetDataType(htGenericText, 0);
		THROW_SL(Write(pMsg, sstrlen(pMsg)));
	}
	THROW(FinishWriting(hfInformer));
	State &= ~stReading;
	CATCHZOK
	return ok;
}

void PPJobSrvReply::SetAck()
{
	SetString(P_TokAck);
}
//
//
//
/*static*/const int16 PPJobSrvProtocol::CurrentProtocolVer = 1;

PPJobSrvProtocol::Header::Header() : Zero(0), ProtocolVer(0), DataLen(0), Type(0), Flags(0)
{
}

SString & FASTCALL PPJobSrvProtocol::Header::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(Zero == 0) {
		rBuf.CatEq("ProtocolVer", (long)ProtocolVer).CatDiv(';', 2).CatEq("DataLen", DataLen).CatDiv(';', 2).CatEq("Type", Type).CatDiv(';', 2).Cat("Flags").Eq().CatHex(Flags);
	}
	else {
		rBuf.CatChar(reinterpret_cast<const char *>(&Zero)[0]);
		rBuf.CatChar(reinterpret_cast<const char *>(&Zero)[1]);
	}
	return rBuf;
}

PPJobSrvProtocol::PPJobSrvProtocol() : SBuffer(), State(0), P_TokAck("ACK"), P_TokErr("ERROR")
{
}

int PPJobSrvProtocol::TestSpecToken(const char * pTok)
{
	int    yes = 0;
	size_t spec_sz = sstrlen(pTok);
	if(GetAvailableSize() == (spec_sz+2)) {
		STempBuffer temp_buf(spec_sz+2+1);
		THROW_SL(ReadStatic(temp_buf, temp_buf.GetSize()-1));
		temp_buf[temp_buf.GetSize()-1] = 0;
		if(strncmp(temp_buf, pTok, spec_sz) == 0)
			yes = 1;
	}
	CATCH
		yes = 0;
	ENDCATCH
	return yes;
}

int PPJobSrvProtocol::StartReading(SString * pRepString)
{
	int    ok = 1;
	ASSIGN_PTR(pRepString, 0);
	ErrText.Z();
	State |= stReading;
	size_t avl_sz = GetAvailableSize();
	THROW_PP(avl_sz >= 2, PPERR_JOBSRV_INVREPLYSIZE);
	THROW_SL(ReadStatic(&H, 2));
	if(H.Zero == 0) {
		THROW_SL(Read(&H, sizeof(H)));
		THROW_PP(H.DataLen == avl_sz, PPERR_JOBSRV_MISSMATCHREPLYSIZE);
		if(H.Flags & hfRepError) {
			if(H.Type == htGenericText)
				ErrText.CatN(static_cast<const char *>(Ptr(GetRdOffs())), GetAvailableSize());
		}
	}
	else {
		MEMSZERO(H);
		H.Flags  |= hfSlString;
		H.DataLen = static_cast<int32>(avl_sz);
		if(TestSpecToken(P_TokErr))
			H.Flags |= hfRepError;
		else if(TestSpecToken(P_TokAck))
			H.Flags |= hfAck;
		CALLPTRMEMB(pRepString, CatN(static_cast<const char *>(Ptr(GetRdOffs())), avl_sz));
		ok = 2;
	}
	CATCHZOK
	return ok;
}

SString & FASTCALL PPJobSrvProtocol::ToStr(SString & rBuf) const
{
	if(State & stStructured) {
		H.ToStr(rBuf);
	}
	else {
		size_t avl_sz = MIN(GetWrOffs(), 255);
		rBuf.Z().CatN(GetBufC(), avl_sz);
		rBuf.Chomp();
	}
	return rBuf;
}

int PPJobSrvProtocol::CheckRepError()
{
	if(H.Flags & hfRepError) {
		PPSetError(PPERR_JOBSRVCLI_ERR, ErrText);
		return 0;
	}
	else
		return 1;
}

PPJobSrvProtocol::StopThreadBlock::StopThreadBlock() : TId(0)
{
	MAddr.Init();
}

int PPJobSrvProtocol::StopThreadBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->SerializeBlock(dir, sizeof(TId)+sizeof(MAddr), &TId, rBuf, 0));
	THROW_SL(pCtx->Serialize(dir, HostName, rBuf));
	THROW_SL(pCtx->Serialize(dir, UserName, rBuf));
	CATCHZOK
	return ok;
}

PPJobSrvProtocol::TransmitFileBlock::TransmitFileBlock() : CrtTime(ZERODATETIME), AccsTime(ZERODATETIME), ModTime(ZERODATETIME),
	Size(0), Format(SFileFormat::Unkn), Flags(0), Cookie(0), PartSize(0), TransmType(ttGeneric), Reserve(0), ObjType(0), ObjID(0)
{
	memzero(Hash, sizeof(Hash));
	memzero(Reserve2, sizeof(Reserve));
	memzero(Name, sizeof(Name));
}

int PPJobSrvProtocol::Helper_Recv(TcpSocket & rSo, const char * pTerminal, size_t * pActualSize)
{
	int    ok = 1;
	const  size_t zs = sizeof(H.Zero);
	size_t actual_size = 0;
	MEMSZERO(H);
	Z();
	THROW_SL(rSo.RecvBlock(&H.Zero, zs, &actual_size));
	if(H.Zero == 0) {
		THROW_SL(rSo.RecvBlock(PTR8(&H) + zs, sizeof(H) - zs, &actual_size));
		THROW_SL(Write(&H, sizeof(H)));
		if(H.DataLen > sizeof(H))
			THROW_SL(rSo.RecvBuf(*this, H.DataLen - sizeof(H), &actual_size));
		State |= stStructured;
	}
	else {
		WriteByte(((char *)&H)[0]);
		WriteByte(((char *)&H)[1]);
		if(pTerminal != 0) {
			THROW_SL(rSo.RecvUntil(*this, pTerminal, &actual_size));
		}
		else {
			THROW_SL(rSo.RecvBuf(*this, 0, &actual_size));
		}
		actual_size += zs;
		State &= ~stStructured;
	}
	CATCHZOK
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}
//
//
//
PPJobSrvCmd::PPJobSrvCmd() : PPJobSrvProtocol()
{
}

int PPJobSrvCmd::StartWriting(int cmdId)
{
	int    ok = 1;
	MEMSZERO(H);
	H.ProtocolVer = CurrentProtocolVer;
	H.Type = cmdId;
	H.DataLen = sizeof(H);
	Z();
	THROW_SL(Write(&H, sizeof(H)));
	State |= stStructured;
	State &= ~stReading;
	CATCHZOK
	return ok;
}

int PPJobSrvCmd::StartWriting(const char * pStr)
{
	int    ok = 1;
	Z();
	if(pStr)
		Write(pStr, sstrlen(pStr));
	State &= ~stStructured;
	State &= ~stReading;
	return ok;
}

int PPJobSrvCmd::FinishWriting()
{
	int    ok = 1;
	if(State & stStructured) {
		static_cast<Header *>(Ptr(GetRdOffs()))->DataLen = static_cast<int32>(GetAvailableSize());
	}
	else {
		Write("\xD\xA", 2);
	}
	return ok;
}
//
//
//
PPJobSrvClient::PPJobSrvClient() : So(120000), SyncTimer(120000), // @v7.8.10 So(600000)-->So(60000) // @v8.7.4 60000-->120000
	State(0), SessId(0), InformerCallbackProc(0), P_InformerCallbackParam(0), InitPort(0)
{
}

PPJobSrvClient::~PPJobSrvClient()
{
	Logout();
	Disconnect();
}

int FASTCALL PPJobSrvClient::Sync(int force)
{
	int    ok = -1;
	if((force || SyncTimer.Check(0)) && !(State & stLockExec)) {
		PPJobSrvReply reply;
		if(Exec("HELLO", reply) && reply.StartReading(0) && reply.CheckRepError())
			ok = 1;
		else
			ok = 0;
	}
	return ok;
}

void PPJobSrvClient::SetInformerProc(int (*proc)(const char * pMsg, void * pParam), void * pParam)
{
	InformerCallbackProc = proc;
	P_InformerCallbackParam = pParam;
}

int PPJobSrvClient::Disconnect()
{
	int    ok = 1;
	if(State & stConnected) {
		So.Disconnect();
		State &= ~stConnected;
		State &= ~stDebugMode;
	}
	else
		ok = -1;
	return ok;
}

int PPJobSrvClient::Reconnect(const char * pAddr, int port)
{
	int    ok = -1;
	Disconnect();
	if(pAddr == 0 && InitAddr.NotEmpty())
		pAddr = InitAddr;
	if(port <= 0 && InitPort > 0)
		port = InitPort;
	if(Connect(pAddr, port)) {
		if(AuthCookie.NotEmpty()) {
			PPJobSrvReply reply;
			SString temp_buf;
			(temp_buf = "RESUME").Space().Cat(AuthCookie);
			if(Exec(temp_buf, reply) && reply.StartReading(&temp_buf.Z()) && reply.CheckRepError()) {
				if(reply.GetH().Flags & PPJobSrvReply::hfAck)
					ok = 2;
			}
		}
		if(ok < 0) {
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int PPJobSrvClient::TestBreakConnection()
{
	So.Disconnect();
	return 1;
}
//
//
//
int PPJobSrvClient::Connect(const char * pAddr, int port)
{
	int    ok = 1;
	if(!(State & stConnected)) {
		int    timeout = -1;
		InetAddr addr;
		SString addr_buf;
		if(pAddr) {
			const char * p = sstrchr(pAddr, ':');
			if(p) {
				addr_buf.CatN(pAddr, (size_t)(p - pAddr));
				port = atoi(p+1);
			}
			else
				addr_buf = pAddr;
		}
#ifdef _PAPYRUS 
		{
			PPIniFile ini_file;
			if(pAddr == 0 || port <= 0) {
				if(port <= 0 && (ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_SERVER_PORT, &port) <= 0 || port <= 0))
					port = InetUrl::GetDefProtocolPort(InetUrl::prot_p_PapyrusServer);//DEFAULT_SERVER_PORT;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_CLIENTSOCKETTIMEOUT, &timeout) <= 0 || timeout <= 0) // @v8.7.4 PPINIPARAM_SERVER_SOCKETTIMEOUT-->PPINIPARAM_CLIENTSOCKETTIMEOUT
					timeout = -1;
				if(!pAddr) {
					if(ini_file.Get(PPINISECT_SERVER, PPINIPARAM_SERVER_NAME, addr_buf) <= 0)
						addr_buf = "localhost";
				}
				else
					addr_buf = pAddr;
			}
			{
				int   iv = 0;
				if(ini_file.GetInt(PPINISECT_SERVER, PPINIPARAM_DEBUG, &iv) > 0)
					State |= stDebugMode;
				else
					State &= ~stDebugMode;
			}
			PPGetFilePath(PPPATH_LOG, PPFILNAM_SERVERDEBUG_LOG, DebugLogFileName);
		}
#endif
		InitAddr = addr_buf;
		InitPort = port;
		addr.Set(addr_buf, port);
		if(timeout > 0)
			So.SetTimeout(timeout);
		THROW_SL(So.Connect(addr));
		State |= stConnected;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPJobSrvClient::Exec(PPJobSrvCmd & rCmd, const char * pTerminal, PPJobSrvReply & rReply)
{
	int    ok = 1;
	int    do_log_error = 0;
	const  int preserve_so_timeout = So.GetTimeout(); // @v9.1.8
	SString log_buf, temp_buf;
	ExecLock.Lock();
	State |= stLockExec;
	rReply.Z();
	THROW_PP(State & stConnected, PPERR_JOBSRVCLI_NOTCONN);
#ifdef _PAPYRUS 
	if(State & stDebugMode) {
		log_buf.Z().Cat("CLIENT REQ").CatDiv(':', 2);
		log_buf.Cat(rCmd.ToStr(temp_buf));
		PPLogMessage(DebugLogFileName, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
	}
#endif
	THROW_SL(So.SendBuf(rCmd, 0));
	So.SetTimeout(1000 * 3600 * 24); // @v9.1.8 Временно устанавливаем очень большой таймаут так как процесс может выполнятся сервером очень долго
	do {
		size_t actual_size = 0;
		do_log_error = 1;
		const int rr = rReply.Helper_Recv(So, pTerminal, &actual_size);
		THROW(rr);
		if(rReply.GetH().Flags & PPJobSrvReply::hfInformer) {
			TempBuf = 0;
			if(rReply.GetH().Type == PPJobSrvReply::htGenericText) {
				PPJobSrvReply::Header temp_hdr;
				rReply.Read(&temp_hdr, sizeof(temp_hdr));
				TempBuf.Cat(rReply);
			}
			if(InformerCallbackProc) {
				InformerCallbackProc(TempBuf, P_InformerCallbackParam);
			}
			//
			// Для информеров в журнал отладки ничего не выводим (слишком большой поток мало значащих сообщений)
			//
		}
		else {
#ifdef _PAPYRUS 
			if(State & stDebugMode) {
				log_buf.Z().Cat("CLIENT REP").CatDiv(':', 2);
				log_buf.Cat(rReply.ToStr(temp_buf));
				PPLogMessage(DebugLogFileName, log_buf, LOGMSGF_TIME|LOGMSGF_THREADINFO);
			}
#endif
		}
	} while(rReply.GetH().Flags & PPJobSrvReply::hfInformer);
	CATCH
#ifdef _PAPYRUS 
		if(do_log_error)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
#endif
		ok = 0;
	ENDCATCH
	So.SetTimeout(preserve_so_timeout); // @v9.1.8
	State &= ~stLockExec;
	ExecLock.Unlock(); // @v8.3.4 LEAVE_CRITICAL_SECTION-->ExecLock.Unlock()
	return ok;
}

int PPJobSrvClient::Exec(PPJobSrvCmd & rCmd, PPJobSrvReply & rReply)
{
	return Exec(rCmd, 0, rReply);
}

int PPJobSrvClient::Exec(const char * pCmd, const char * pTerminal, PPJobSrvReply & rReply)
{
	PPJobSrvCmd cmd;
	cmd.StartWriting(pCmd);
	cmd.FinishWriting();
	return Exec(cmd, pTerminal, rReply);
}

int PPJobSrvClient::Exec(const char * pCmd, PPJobSrvReply & rReply)
{
	PPJobSrvCmd cmd;
	cmd.StartWriting(pCmd);
	cmd.FinishWriting();
	return Exec(cmd, 0, rReply);
}

int PPJobSrvClient::GetLastErr(SString & rBuf)
{
	int    ok = 1;
	PPJobSrvReply reply;
	THROW(Exec("GETLASTERR", reply));
	THROW(reply.StartReading(&rBuf));
	CATCHZOK
	rBuf.Chomp();
	return ok;
}

int PPJobSrvClient::Login(const char * pDbSymb, const char * pUserName, const char * pPassword)
{
	int    ok = 1;
	SString cmd, reply_str;
	PPJobSrvReply reply;
	if(State & stLoggedIn) {
		THROW(Logout());
	}
	cmd.Cat("LOGIN").Space().Cat(pDbSymb).Space().Cat(pUserName).Space().Cat(pPassword);
	THROW(Exec(cmd, reply));
	THROW(reply.StartReading(0));
	THROW(reply.CheckRepError());
	{
		THROW(Exec("HSH", reply));
		THROW(reply.StartReading(&reply_str));
		THROW(reply.CheckRepError());
		AuthCookie = reply_str.Chomp();
	}
	State |= stLoggedIn;
	CATCHZOK
	return ok;
}

int PPJobSrvClient::Logout()
{
	int    ok = 1;
	if(State & stLoggedIn) {
		SString reply_str;
		PPJobSrvReply reply;
		THROW(Exec("LOGOUT", reply));
		THROW(reply.StartReading(0));
		if(reply.GetH().Flags & PPJobSrvReply::hfRepError) {
			GetLastErr(reply_str);
			PPSetError(PPERR_JOBSRVCLI_ERR, reply_str);
			CALLEXCEPT();
		}
		AuthCookie = 0;
		State &= ~stLoggedIn;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
