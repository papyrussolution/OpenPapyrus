// STCP.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
// @v11.7.0 #include <wininet.h> 
#include <slib-ossl.h>
//
// @v7.3.0 При большом размере буфера чтения не стабильно принимаются значительные по объему данные без терминатора
// и без информации о требуемом размере чтения (т.е. сколько-есть).
//
size_t TcpSocket::DefaultReadFrame  = 8192;
size_t TcpSocket::DefaultWriteFrame = 8192;

TcpSocket::SslBlock::SslBlock() : State(0), LastError(0), P_S(0)
{
	SLS.InitSSL();
	P_Ctx = SSL_CTX_new(SSLv23_client_method());
}

TcpSocket::SslBlock::~SslBlock()
{
	Shutdown();
}

int TcpSocket::SslBlock::Connect(SOCKET s)
{
	int    ok = 1;
	SSL  * p_ssl_s = 0;
	THROW(P_Ctx);
	THROW(p_ssl_s = SSL_new(static_cast<SSL_CTX *>(P_Ctx)));
	SSL_set_fd(p_ssl_s, s);
	THROW(SSL_connect(p_ssl_s) == 1);
	P_S = p_ssl_s;
	CATCHZOK
	return ok;
}

int TcpSocket::SslBlock::Shutdown()
{
	int    ok = -1;
	if(P_S) {
		SSL_shutdown(static_cast<SSL *>(P_S));
		SSL_free(static_cast<SSL *>(P_S));
		P_S = 0;
		ok = 1;
	}
	if(P_Ctx) {
		SSL_CTX_free(static_cast<SSL_CTX *>(P_Ctx));
		P_Ctx = 0;
		ok = 1;
	}
	return ok;
}

int TcpSocket::SslBlock::Accept()
{
	LastError = P_S ? SSL_accept(static_cast<SSL *>(P_S)) : -1;
	return LastError ? 0 : 1;
}

int TcpSocket::SslBlock::Select(int mode /* TcpSocket::mXXX */, int timeout, size_t * pAvailableSize) { return BIN(P_S); }
int TcpSocket::SslBlock::Read(void * pBuf, int bufLen) { return P_S ? SSL_read(static_cast<SSL *>(P_S), pBuf, bufLen) : -1; }
int TcpSocket::SslBlock::Write(const void * pBuf, int bufLen) { return P_S ? SSL_write(static_cast<SSL *>(P_S), pBuf, bufLen) : -1; }

TcpSocket::TcpSocket(int timeout, int maxConn) : InBuf(DefaultReadFrame), OutBuf(DefaultWriteFrame), Timeout(timeout), MaxConn(maxConn), LastSockErr(0), P_Ssl(0)
{
	Reset();
}

TcpSocket::~TcpSocket()
{
	Disconnect();
}

bool TcpSocket::IsValid() const { return (S != INVALID_SOCKET); }
int  TcpSocket::GetTimeout() const { return Timeout; }
int  TcpSocket::Connect(SslMode sslm, const InetAddr & rAddr) { return Helper_Connect(sslm, rAddr); }
int  TcpSocket::Connect(const InetAddr & rAddr) { return Helper_Connect(sslmNone, rAddr); }

void TcpSocket::Reset()
{
	ZDELETE(P_Ssl);
	S = INVALID_SOCKET;
	MEMSZERO(StatData);
}

int TcpSocket::MoveToS(TcpSocket & rDest, int force /*=0*/)
{
	int    ok = 0;
	if(!rDest.IsValid()) {
		rDest.S = S;
		rDest.Timeout = Timeout;
		rDest.StatData = StatData;
		Reset();
		ok = rDest.IsValid() ? 1 : -1;
	}
	else if(force) {
		rDest.S = S;
		rDest.Timeout = Timeout;
		rDest.StatData = StatData;
		ok = rDest.IsValid() ? 1 : -1;
	}
	else
		ok = 0;
	return ok;
}

int TcpSocket::CopyS(TcpSocket & rSrc)
{
	int    ok = 0;
	WSAPROTOCOL_INFO sock_info;
	if(WSADuplicateSocket(rSrc.S, GetCurrentProcessId(), &sock_info) == 0) {
		Disconnect();
		S = WSASocket(PF_INET, SOCK_STREAM, 0, &sock_info, 0, 0);
		Timeout     = rSrc.Timeout;
		MaxConn     = rSrc.MaxConn;
		LastSockErr = rSrc.LastSockErr;
		StatData    = rSrc.StatData;
		InBuf = rSrc.InBuf;
		OutBuf = rSrc.OutBuf;
		SetHandleInformation((HANDLE)S, HANDLE_FLAG_INHERIT, 0);
		rSrc.Reset();
		ok = IsValid();
	}
	return ok;
}

// private
int TcpSocket::Init(SOCKET s)
{
	Disconnect();
	Reset();
	S = s;
	{
		int bufsize;
		int len = sizeof(bufsize);
		::getsockopt(S, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, &len);
		StatData.RcvBufSize = bufsize;
		len = sizeof(bufsize);
		::getsockopt(S, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, &len);
		StatData.SndBufSize = bufsize;
	}
	return IsValid();
}

int TcpSocket::CheckErrorStatus()
{
	int    ok = 1;
	int    err = 0;
	int    len = sizeof(err);
	if(getsockopt(S, SOL_SOCKET, SO_ERROR, (char *)&err, &len) >= 0) {
		if(err) {
			LastSockErr = err;
			ok = 0;
		}
	}
	else
		ok = SLS.SetError(SLERR_SOCK_WINSOCK);
	return ok;
}

int FASTCALL TcpSocket::SetTimeout(int timeout)
{
	if(timeout >= 0) {
		Timeout = timeout;
		return 1;
	}
	else
		return 0;
}

int TcpSocket::Helper_Connect(SslMode sslm, const InetAddr & rAddr)
{
	int    ok = 1, err;
	sockaddr_in addr;
	InetAddr self_addr;
	self_addr.Set(ADDR_ANY);
	THROW(Bind(self_addr));
	if(::connect(S, rAddr.Get(&addr), sizeof(addr)) == SOCKET_ERROR) {
		int    len;
		int    bufsize;
		fd_set rset, wset;
		SString addr_buf;
		err = WSAGetLastError();
		rAddr.ToStr(InetAddr::fmtHost|InetAddr::fmtPort, addr_buf);
		THROW_S_S(!err || err == WSAEWOULDBLOCK, SLERR_SOCK_CONNECT, addr_buf);
		FD_ZERO(&rset);
		FD_SET(S, &rset);
		wset = rset;
		{
			struct timeval tval, * p_tval = 0;
			tval.tv_sec = 0;
			tval.tv_usec = 0;
			if(Timeout >= 0) {
				tval.tv_sec = Timeout / 1000;
				tval.tv_usec = (Timeout % 1000) * 10;
				p_tval = &tval;
			}
			THROW_S(select(S+1 /* ignored */, &rset, &wset, 0, p_tval), SLERR_SOCK_TIMEOUT);
		}
		THROW_S(FD_ISSET(S, &rset) || FD_ISSET(S, &wset), SLERR_SOCK_NONBLOCKINGCONN);
		len = sizeof(err);
		THROW(CheckErrorStatus());
		len = sizeof(bufsize);
		::getsockopt(S, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, &len);
		StatData.RcvBufSize = bufsize;
		len = sizeof(bufsize);
		::getsockopt(S, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, &len);
		StatData.SndBufSize = bufsize;
	}
	if(sslm == sslmClient) {
        THROW_S(P_Ssl = new SslBlock, SLERR_NOMEM);
        THROW(P_Ssl->Connect(S));
	}
	CATCH
		ZDELETE(P_Ssl);
		if(S != INVALID_SOCKET) {
			closesocket(S);
			Reset();
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int TcpSocket::Bind(const InetAddr & rAddr)
{
	int    ok = 1;
	sockaddr_in addr;
	S = socket(AF_INET, SOCK_STREAM, 0);
	if(!IsValid() || bind(S, rAddr.Get(&addr), sizeof(addr)) != 0) {
		closesocket(S);
		Reset();
		ok = (SLibError = SLERR_SOCK_UNABLEOPEN, 0);
	}
	return ok;
}

int TcpSocket::GetSockName(InetAddr * pAddr, int peer)
{
	int    ok = 1;
	struct sockaddr_in addr;
	int    addrlen = sizeof(addr);
	if(peer)
		ok = !getpeername(S, (struct sockaddr *)&addr, &addrlen);
	else
		ok = !getsockname(S, (struct sockaddr *)&addr, &addrlen);
	if(ok > 0) {
		uchar  str_addr[16];
		SString addr_buf;
		memzero(str_addr, sizeof(str_addr));
		memcpy(str_addr, &addr.sin_addr, 4);
		addr_buf.Cat((int)str_addr[0]).Dot().Cat((int)str_addr[1]).Dot().Cat((int)str_addr[2]).Dot().Cat((int)str_addr[3]);
		pAddr->Set(addr_buf, addr.sin_port);
	}
	else
		ok = SLS.SetError(SLERR_SOCK_WINSOCK);
	return ok;
}

int TcpSocket::Disconnect()
{
	int    ok = 1;
	ZDELETE(P_Ssl);
	if(S != INVALID_SOCKET) {
		::shutdown(S, SD_BOTH); // helps against CLOSE_WAIT problems
		ok = ::closesocket(S) ? 0 : 1;
		S = INVALID_SOCKET;
	}
	else
		ok = -1;
	return ok;
}

int TcpSocket::Listen()
{
	return ::listen(S, MaxConn) ? (SLibError = SLERR_SOCK_LISTEN, 0) : 1;
}

int TcpSocket::Accept(TcpSocket & rCliSock, InetAddr & rCliAdr)
{
	int    ok = 1;
	struct sockaddr_in cli_addr;
	int    addr_size = sizeof(cli_addr);
	SOCKET cli_sock = ::accept(S, reinterpret_cast<sockaddr *>(&cli_addr), &addr_size);
	if(cli_sock != INVALID_SOCKET) {
		rCliSock.Init(cli_sock);
		rCliAdr.Set(&cli_addr);
	}
	else {
#ifdef _DEBUG
		SlThreadLocalArea * p_tla = &SLS.GetTLA();
		if(p_tla == 0) {
			SLS.LogMessage(0, "&SLS.GetTLA() == 0");
		}
#endif
		ok = (SLibError = SLERR_SOCK_ACCEPT, 0);
	}
	return ok;
}
//
//
//
int TcpSocket::Select(int mode /* TcpSocket::mXXX */, int timeout, size_t * pAvailableSize)
{
	int    ok = 1;
	if(P_Ssl) {
		ok = P_Ssl->Select(mode, timeout, pAvailableSize);
	}
	else {
		int    pending_ms = 0;
		size_t av_size = 0;
		fd_set rset;
		fd_set wset;
		fd_set eset;
		fd_set * p_rset = 0;
		fd_set * p_wset = 0;
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&eset);
		if(mode == mRead) {
			FD_SET(S, &rset);
			FD_SET(S, &eset);
			p_rset = &rset;
			assert(FD_ISSET(S, &rset));
		}
		else {
			FD_SET(S, &wset);
			FD_SET(S, &eset);
			p_wset = &wset;
			assert(FD_ISSET(S, &wset));
		}
		struct timeval tval, * p_tval = 0;
		tval.tv_sec = 0;
		tval.tv_usec = 0;
		if(timeout >= 0) {
			tval.tv_sec = timeout / 1000;
			tval.tv_usec = (timeout % 1000) * 10;
			p_tval = &tval;
		}
		else if(Timeout >= 0) {
			tval.tv_sec = Timeout / 1000;
			tval.tv_usec = (Timeout % 1000) * 10;
			p_tval = &tval;
		}
		int    r = ::select(S+1/* ignored */, p_rset, p_wset, &eset, p_tval);
		if(!r)
			ok = (SLibError = SLERR_SOCK_TIMEOUT, 0);
		else if(r == SOCKET_ERROR)
			ok = SLS.SetError(SLERR_SOCK_WINSOCK);
		else {
			av_size = (size_t)r; // !!!
			if(p_rset && !FD_ISSET(S, p_rset)) {
				ok = (SLibError = SLERR_SOCK_NONBLOCKINGRD, 0);
			}
			else if(p_wset && !FD_ISSET(S, p_wset)) {
				ok = (SLibError = SLERR_SOCK_NONBLOCKINGWR, 0);
			}
		}
		ASSIGN_PTR(pAvailableSize, av_size);
	}
	return ok;
}

int FASTCALL TcpSocket::Helper_Recv(void * pBuf, size_t size)
{
	return (P_Ssl == 0) ? ::recv(S, (char *)pBuf, size, 0) : P_Ssl->Read(pBuf, size);
}

int TcpSocket::Recv(void * pBuf, size_t size, size_t * pRcvdSize)
{
	int    ok = 1;
	size_t rcvd_size = 0;
	int    local_len;
	THROW(CheckErrorStatus());
	THROW(Select(mRead, -1, 0));
	local_len = Helper_Recv(pBuf, size);
	THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
	rcvd_size += local_len;
	StatData.RdCount += local_len;
	CATCH
		ok = 0;
		rcvd_size = 0;
	ENDCATCH
	ASSIGN_PTR(pRcvdSize, rcvd_size);
	return ok;
}

int TcpSocket::RecvUntil(SBuffer & rBuf, const char * pTerminator, size_t * pRcvdSize)
{
	int    ok = 1;
	const  size_t term_len = (pTerminator == 0) ? 0 : ((pTerminator[0] == 0) ? 1 : sstrlen(pTerminator));
	size_t total_sz = 0;
	if(term_len) {
		size_t match_len = 0;
		THROW(CheckErrorStatus());
		while(match_len < term_len && Select(mRead, -1, 0)) {
			size_t rd_len = 0;
			size_t recv_sz;
			do {
				recv_sz = (term_len - match_len);
				rd_len = 0;
				{
					int    local_len = Helper_Recv(InBuf.vptr(), recv_sz);
					THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
					rd_len = (size_t)local_len;
					StatData.RdCount += local_len;
				}
				if(rd_len) {
					THROW(rBuf.Write(InBuf.vcptr(), rd_len));
					total_sz += rd_len;
					assert(rBuf.GetWrOffs() >= term_len);
					const char * p_buf = static_cast<const char *>(rBuf.GetBuf(rBuf.GetWrOffs()-term_len));
					match_len = 0;
					for(uint i = 0; !match_len && i < term_len; i++)
						if(memcmp(p_buf+i, pTerminator, term_len-i) == 0)
							match_len = (term_len-i);
				}
			} while(match_len < term_len && (rd_len == recv_sz));
		}
		ok = (match_len == term_len) ? 1 : -1;
	}
	else
		ok = RecvBuf(rBuf, 0, &total_sz);
	CATCHZOK
	ASSIGN_PTR(pRcvdSize, total_sz);
	return ok;
}

int TcpSocket::RecvBuf(SBuffer & rBuf, size_t size, size_t * pRcvdSize)
{
	int    ok = 1;
	size_t total_sz = 0;
	size_t sz = size ? size : InBuf.GetSize();
	if(sz) {
		THROW(CheckErrorStatus());
		while(Select(mRead, -1, 0)) {
			size_t recv_sz = MIN(InBuf.GetSize(), sz);
			size_t rd_len = 0;
			{
				int    local_len = Helper_Recv(InBuf.vptr(), recv_sz);
				THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
				rd_len = (size_t)local_len;
				StatData.RdCount += local_len;
			}
			THROW(rBuf.Write(InBuf.vcptr(), rd_len));
			total_sz += rd_len;
			if(size) {
				if(total_sz < size)
					sz = size - total_sz;
				else
					break;
			}
			else if(!rd_len || rd_len < recv_sz)
				break;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pRcvdSize, total_sz);
	return ok;
}

int TcpSocket::RecvBlock(void * pBuf, size_t size, size_t * pRcvdSize)
{
	int    ok = 1;
	size_t rcvd_size = 0;
	int    local_len;
	THROW(CheckErrorStatus());
	do {
		THROW(Select(mRead, -1, 0));
		local_len = Helper_Recv(PTR8(pBuf)+rcvd_size, size-rcvd_size);
		THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
		rcvd_size += local_len;
		StatData.RdCount += local_len;
	} while(rcvd_size < size && local_len > 0);
	CATCH
		ok = 0;
		rcvd_size = 0;
	ENDCATCH
	ASSIGN_PTR(pRcvdSize, rcvd_size);
	return ok;
}

int TcpSocket::Send(const void * pBuf, size_t size, size_t * pSendedSize)
{
	int    ok = 1;
	int    len = 0;
	THROW(Select(mWrite));
	THROW(CheckErrorStatus());
	if(P_Ssl == 0) {
		len = ::send(S, static_cast<const char *>(pBuf), size, 0);
	}
	else {
		len = P_Ssl->Write(pBuf, size);
	}
	THROW_S(len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
	StatData.WrCount += len;
	CATCH
		ok = 0;
		len = 0;
	ENDCATCH
	ASSIGN_PTR(pSendedSize, len);
	return ok;
}

int TcpSocket::SendBuf(SBuffer & rBuf, size_t * pSendedSize)
{
	int    ok = -1;
	size_t sz, total_sz = 0;
	while((sz = rBuf.GetAvailableSize()) != 0) {
		size_t sended_sz = 0;
		SETMIN(sz, OutBuf.GetSize());
		rBuf.Read(OutBuf.vptr(), sz);
		THROW(Send(OutBuf.vcptr(), sz, &sended_sz));
		THROW_S(sended_sz != 0, SLERR_SOCK_SEND);
		total_sz += sended_sz;
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pSendedSize, total_sz);
	return ok;
}

int TcpSocket::GetStat(long * pRdCount, long * pWrCount)
{
	ASSIGN_PTR(pRdCount, StatData.RdCount);
	ASSIGN_PTR(pWrCount, StatData.WrCount);
	return 1;
}
//
//
//
bool IpServerListeningEntry::Parse(const char * pText)
{
	bool    ok = false;
	if(!isempty(pText)) {
		SStrScan scan(pText);
	}
	return ok;
}
//
//
//
TcpServer::TcpServer(const /*InetAddr & rAddr*/IpServerListeningEntry & rSle) : TcpSocket(1000), /*Addr(rAddr)*/Sle(rSle)
{
}

TcpServer::~TcpServer()
{
}

int TcpServer::ExecSession(TcpSocket & rSock, InetAddr & rAddr)
{
	return 1;
}

int TcpServer::Run()
{
	int    ok = 1;
	SString msg_buf, temp_buf;
	THROW(Bind(/*Addr*/Sle.A));
	THROW(Listen());
	while(!SLS.CheckStopFlag()) {
		if(Select(TcpSocket::mRead, 10000) > 0) { // @v6.1.4 timeout: -1-->10000
			ENTER_CRITICAL_SECTION
			InetAddr cli_addr;
			TcpSocket cli_sock(60000); // @v6.1.2 timeout: 0-->300000 @v7.8.10 timeout: 300000-->60000
			if(Accept(cli_sock, cli_addr)) {
				ExecSession(cli_sock, cli_addr);
			}
			else {
				cli_addr.ToStr(InetAddr::fmtAddr|InetAddr::fmtHost|InetAddr::fmtPort, temp_buf);
				(msg_buf = "TcpServer accept error").Space().Cat(temp_buf);
				SLS.LogMessage(0, msg_buf);
			}
			LEAVE_CRITICAL_SECTION
		}
	}
#ifdef _DEBUG
	SLS.LogMessage(0, "TcpServer stopped correctly");
#endif
	CATCHZOK
	return ok;
}
//
//
//
static const SIntToSymbTabEntry ContentDispositionTypeNameList_[] = {
	{ SMailMessage::ContentDispositionBlock::tInline, "inline" },
	{ SMailMessage::ContentDispositionBlock::tAttachment, "attachment" },
	{ SMailMessage::ContentDispositionBlock::tFormData, "form-data" },
	{ SMailMessage::ContentDispositionBlock::tSignal, "signal" },
	{ SMailMessage::ContentDispositionBlock::tAlert, "alert" },
	{ SMailMessage::ContentDispositionBlock::tIcon, "icon" },
	{ SMailMessage::ContentDispositionBlock::tRender, "render" },
	{ SMailMessage::ContentDispositionBlock::tRecipientListHistory, "recipient-list-history" },
	{ SMailMessage::ContentDispositionBlock::tSession, "session" },
	{ SMailMessage::ContentDispositionBlock::tAIB, "aib" },
	{ SMailMessage::ContentDispositionBlock::tEarlySession, "early-session" },
	{ SMailMessage::ContentDispositionBlock::tRecipientList, "recipient-list" },
	{ SMailMessage::ContentDispositionBlock::tNotification, "notification" },
	{ SMailMessage::ContentDispositionBlock::tByReference, "by-reference" },
	{ SMailMessage::ContentDispositionBlock::tRecordingSession, "recording-session" }
};

/*static*/int FASTCALL SMailMessage::ContentDispositionBlock::GetTypeName(int t, SString & rBuf)
{
	return SIntToSymbTab_GetSymb(ContentDispositionTypeNameList_, SIZEOFARRAY(ContentDispositionTypeNameList_), t, rBuf);
}

/*static*/int FASTCALL SMailMessage::ContentDispositionBlock::IdentifyType(const char * pTypeName)
{
	// SMailMessage::ContentDispositionBlock::tUnkn == 0
	int      type = SIntToSymbTab_GetId(ContentDispositionTypeNameList_, SIZEOFARRAY(ContentDispositionTypeNameList_), pTypeName); 
	SETIFZ(type, SMailMessage::ContentDispositionBlock::tUnkn);
	return type;
}

SMailMessage::ContentDispositionBlock::ContentDispositionBlock()
{
	Destroy();
}

void SMailMessage::ContentDispositionBlock::Destroy()
{
	Type = tUnkn;
	NameP = 0;
	FileNameP = 0;
	Size = 0;
	ModifDtm.Z();
	CrDtm.Z();
	RdDtm.Z();
}

SMailMessage::ContentTypeBlock::ContentTypeBlock()
{
	Destroy();
}

void SMailMessage::ContentTypeBlock::Destroy()
{
	MimeP = 0;
	TypeP = 0;
	NameP = 0;
	BoundaryP = 0;
	Cp = cpUndef;
}

SMailMessage::Boundary::Boundary() : LineNo_Start(0), LineNo_Finish(0), OuterFileNameP(0), P_Parent(0), ContentTransfEnc(SFileFormat::cteUndef),
	ContentDescrP(0), ContentIdP(0)
{
}

void SMailMessage::Boundary::Destroy()
{
	Ct.Destroy();
	Cd.Destroy();
	ContentTransfEnc = SFileFormat::cteUndef;
	ContentDescrP = 0;
	ContentIdP = 0;
	LineNo_Start = 0;
	LineNo_Finish = 0;
	OuterFileNameP = 0;
	P_Parent = 0;
	Data.Destroy();
	Children.freeAll();
}

uint SMailMessage::Boundary::GetAttachmentCount() const
{
	uint   c = 0;
	if(Cd.Type == Cd.tAttachment)
		c++;
	for(uint i = 0; i < Children.getCount(); i++) {
		const Boundary * p_child = Children.at(i);
		if(p_child)
			c += p_child->GetAttachmentCount(); // @recursion
	}
	return c;
}

const SMailMessage::Boundary * FASTCALL SMailMessage::Boundary::Helper_GetAttachmentByIndex(int & rIdx /*0..*/) const
{
	const SMailMessage::Boundary * p_result = 0;
	if(Cd.Type == Cd.tAttachment) {
		if(rIdx == 0)
			p_result = this;
		else
			rIdx--;
	}
	for(uint i = 0; !p_result && rIdx >= 0 && i < Children.getCount(); i++) {
		const Boundary * p_child = Children.at(i);
		if(p_child)
			p_result = p_child->Helper_GetAttachmentByIndex(rIdx);
	}
	return p_result;
}

const SMailMessage::Boundary * FASTCALL SMailMessage::Boundary::GetAttachmentByIndex(uint idx /*0..*/) const
{
	int    iidx = (int)idx;
	return Helper_GetAttachmentByIndex(iidx);
}

SMailMessage::ParserBlock::ParserBlock()
{
	Destroy();
}

void SMailMessage::ParserBlock::Destroy()
{
	State = 0;
	LineNo = 0;
	P_B = 0;
}

SMailMessage::SMailMessage() : Flags(0), Size(0)
{
	memzero(Zero, sizeof(Zero));
	Init();
}

SMailMessage::~SMailMessage()
{
	Init();
}

void SMailMessage::Init()
{
	Flags = 0;
	Size = 0;
	MEMSZERO(HFP);
	B.Destroy();
	AttachPosL.clear();
	ReceivedChainL.clear();
	ClearS();
}

SString & SMailMessage::GetBoundary(int start, SString & rBuf) const
{
	rBuf.Z();
	if(B.Ct.BoundaryP) {
		GetField(fldBoundary, rBuf);
		if(start == 1 || start == 2)
			rBuf.Insert(0, "--");
		if(start == 2)
			rBuf.Cat("--");
	}
	return rBuf;
}

int SMailMessage::IsPpyData() const
{
	return BIN(Flags & (fPpyOrder|fPpyObject));
}

int SMailMessage::AttachFile(const char * pFileName)
{
	int    ok = 0;
	if(fileExists(pFileName)) {
		uint   _p = 0;
		AddS(pFileName, &_p);
		AttachPosL.insert(&_p);
		Flags |= fMultipart;
		ok = 1;
	}
	return ok;
}

int SMailMessage::EnumAttach(uint * pPos, SString & rFileName, SString & rFullPath)
{
	rFileName.Z();
	rFullPath.Z();
	int    ok = 0;
	uint   pos = DEREFPTRORZ(pPos);
	if(pos < AttachPosL.getCount()) {
		//rFullPath = AttList.at(pos);
		GetS(AttachPosL.at(pos), rFullPath);
		const SFsPath ps(rFullPath);
		ps.Merge(SFsPath::fNam|SFsPath::fExt, rFileName);
		pos++;
		ASSIGN_PTR(pPos, pos);
		ok = 1;
	}
	return ok;
}

int SMailMessage::SetField(int fldId, const char * pVal)
{
	SString temp_buf(pVal);
	switch(fldId) {
		case fldFrom: AddS(temp_buf.Strip(), &HFP.FromP); break;
		case fldTo: AddS(temp_buf.Strip(), &HFP.ToP); break;
		case fldCc: AddS(temp_buf.Strip(), &HFP.CcP); break;
		case fldSubj: AddS(temp_buf.Strip(), &HFP.SubjP); break;
		case fldMailer: AddS(temp_buf.Strip(), &HFP.MailerP); break;
		case fldBoundary: AddS(temp_buf.Strip(), &B.Ct.BoundaryP); break;
		case fldText:
			//(Text = pVal).Strip();
			//AddS(temp_buf.Strip(), &HFP.TextP);
			break;
		default:
			return 0;
	}
	return 1;
}

int FASTCALL SMailMessage::IsField(int fldId) const
{
	switch(fldId) {
		/*case fldFrom: return From.NotEmpty();
		case fldTo: return To.NotEmpty();
		case fldCc: return Cc.NotEmpty();
		case fldSubj: return Subj.NotEmpty();
		case fldMailer: return Mailer.NotEmpty();
		case fldBoundary: return Boundary.NotEmpty();
		case fldText: return Text.NotEmpty();*/
		case fldFrom: return BIN(HFP.FromP);
		case fldTo: return BIN(HFP.ToP);
		case fldCc: return BIN(HFP.CcP);
		case fldSubj: return BIN(HFP.SubjP);
		case fldMailer: return BIN(HFP.MailerP);
		case fldBoundary: return BIN(B.Ct.BoundaryP);
		//case fldText: return Text.NotEmpty();
	}
	return 0;
}

SString & SMailMessage::GetField(int fldId, SString & rBuf) const
{
	uint   _p = 0;
	switch(fldId) {
		/*
		case fldFrom: return From.NotEmpty() ? From.cptr() : Zero;
		case fldTo: return To.NotEmpty() ? To.cptr() : Zero;
		case fldCc: return Cc.NotEmpty() ? Cc.cptr() : Zero;
		case fldSubj: return Subj.NotEmpty() ? Subj.cptr() : Zero;
		case fldMailer: return Mailer.NotEmpty() ? Mailer.cptr() : Zero;
		case fldBoundary: return Boundary.NotEmpty() ? Boundary.cptr() : Zero;
		case fldText: return Text.NotEmpty() ? Text.cptr() : Zero;
		*/
		case fldFrom: _p = HFP.FromP; break;
		case fldTo:   _p = HFP.ToP; break;
		case fldCc:   _p = HFP.CcP; break;
		case fldSubj: _p = HFP.SubjP; break;
		case fldMailer: _p = HFP.MailerP; break;
		case fldBoundary: _p = B.Ct.BoundaryP; break;
		//case fldText: return Text.NotEmpty() ? Text.cptr() : Zero;
	}
	GetS(_p, rBuf);
	return rBuf;
}

int SMailMessage::CmpField(int fldId, const char * pStr, size_t len) const
{
	SString temp_buf;
	GetField(fldId, temp_buf);
	return temp_buf.NotEmpty() ? strnicmp(pStr, temp_buf, (len) ? len : temp_buf.Len()) : 0;
}

int SMailMessage::IsFrom(const char * pEmail) const
{
	int    ok = 0;
	if(!isempty(pEmail)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pEmail), -1, nta, 0);
		if(nta.Has(SNTOK_EMAIL) > 0.0f) {
			SString addr;
			GetS(HFP.FromP, addr);
			if(addr.NotEmptyS() && addr.Search(pEmail, 0, 1, 0)) {
				ok = 1;
			}
		}
	}
	return ok;
}

int SMailMessage::IsSubj(const char * pSubj, int substr) const
{
	int    ok = 0;
	if(!isempty(pSubj)) {
		SString subj;
		GetS(HFP.SubjP, subj);
		if(subj.NotEmptyS()) {
			if(substr) {
				if(subj.Search(pSubj, 0, 1, 0))
					ok = 1;
			}
			else {
				if(subj.CmpNC(pSubj) == 0)
					ok = 1;
			}
		}
	}
	return ok;
}

SString & SMailMessage::PutField(const char * pFld, const char * pVal, SString & rBuf)
{
	rBuf.Z();
	if(pVal) {
		rBuf.Cat(pFld).CatDiv(':', 2).Cat(pVal);
	}
	return rBuf;
}

/*static*/int SMailMessage::IsFieldHeader(const SString & rLineBuf, const char * pHeader, SString & rValue)
{
	rValue.Z();
	size_t hl = sstrlen(pHeader);
	if(hl && rLineBuf.HasPrefixNC(pHeader)) {
		if(rLineBuf[hl] == ':') {
			hl++;
			while(rLineBuf[hl] == ' ')
				hl++;
			rValue = rLineBuf.cptr() + hl;
			return 1;
		}
	}
	return 0;
}

int DecodeMimeStringToBuffer(const SString & rLine, int contentTransfEnc, SBuffer & rBuf)
{
	int    ok = 1;
	const size_t src_len = rLine.Len();
	if(src_len) {
		const char * p_src_buf = rLine.cptr();
		if(contentTransfEnc == SFileFormat::cteQuotedPrintable) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			rLine.Decode_QuotedPrintable(r_temp_buf);
			THROW(rBuf.Write(r_temp_buf.cptr(), r_temp_buf.Len()));
		}
		else if(contentTransfEnc == SFileFormat::cteBase64) {
			char   mime_buf[1024];
			size_t mime_len = 0;
			rLine.DecodeMime64(mime_buf, sizeof(mime_buf), &mime_len);
			THROW(rBuf.Write(mime_buf, mime_len));
		}
		else {
			THROW(rBuf.Write(rLine.cptr(), src_len));
		}
	}
	CATCHZOK
	return ok;
}

int SMailMessage::ProcessInputLine(ParserBlock & rBlk, const SString & rLineBuf)
{
	int    ok = 1;
	int    do_read_field = 1;
	size_t prefix_len = 0;
	SString temp_buf;
	SString decode_buf;
	if(rLineBuf.IsEmpty()) {
		if(rBlk.State == rBlk.stHeader)
			rBlk.State = rBlk.stBody;
		else if(rBlk.State == rBlk.stMimePartHeader)
			rBlk.State = rBlk.stMimePartBody;
		else if(oneof2(rBlk.State, rBlk.stBody, rBlk.stMimePartBody)) {
			if(rBlk.P_B) {
				THROW(DecodeMimeStringToBuffer(rLineBuf, rBlk.P_B->ContentTransfEnc, rBlk.P_B->Data));
			}
		}
	}
	else if(rLineBuf[0] == '-' && rLineBuf[1] == '-') {
		temp_buf = rLineBuf+2;
		SMailMessage::Boundary * p_b = SearchBoundary(temp_buf);
		if(p_b) {
			SString boundary_ident;
			GetS(p_b->Ct.BoundaryP, boundary_ident);
			const size_t bl = boundary_ident.Len();
			if(temp_buf.Len() > bl && temp_buf[bl] == '-' && temp_buf[bl+1] == '-') {
				//assert(p_b == rBlk.P_B);
				if(oneof2(rBlk.State, rBlk.stMimePartBody, rBlk.stMimePartHeader)) {
					rBlk.State = rBlk.stBody;
				}
				rBlk.P_B->LineNo_Finish = rBlk.LineNo;
				rBlk.P_B = rBlk.P_B->P_Parent;
			}
			else {
				rBlk.State = rBlk.stMimePartHeader;
				Boundary * p_new_b = p_b->Children.CreateNewItem();
				THROW(p_new_b);
				p_new_b->P_Parent = p_b;//rBlk.P_B;
				rBlk.P_B->LineNo_Finish = rBlk.LineNo;
				rBlk.P_B = p_new_b;
				rBlk.P_B->LineNo_Start = rBlk.LineNo;
			}
		}
		else { // Лидирующие "--" могут быть просто текстом (но только если мы в rBlk.stBody или rBlk.stMimePartBody
			if(oneof2(rBlk.State, rBlk.stBody, rBlk.stMimePartBody)) {
				THROW(DecodeMimeStringToBuffer(rLineBuf, rBlk.P_B->ContentTransfEnc, rBlk.P_B->Data));
			}
			else {
				; // @error
			}
		}
	}
	else if(oneof2(rBlk.State, rBlk.stBody, rBlk.stMimePartBody)) {
		THROW(DecodeMimeStringToBuffer(rLineBuf, rBlk.P_B->ContentTransfEnc, rBlk.P_B->Data));
	}
	else if(IsFieldHeader(rLineBuf, "Received", temp_buf)) {
		uint _p = 0;
		AddS(temp_buf, &_p);
		ReceivedChainL.insert(&_p);
	}
	else if(IsFieldHeader(rLineBuf, "Delivered-To", temp_buf)) {
		AddS(temp_buf, &HFP.DeliveredToP);
	}
	else if(IsFieldHeader(rLineBuf, "Reply-To", temp_buf)) {
		AddS(temp_buf, &HFP.ReplyToP);
	}
	else if(IsFieldHeader(rLineBuf, "X-Original-To", temp_buf)) {
	}
	else if(IsFieldHeader(rLineBuf, "From", temp_buf)) {
		temp_buf.Decode_EncodedWordRFC2047(decode_buf, 0, 0);
		AddS(decode_buf, &HFP.FromP);
	}
	else if(IsFieldHeader(rLineBuf, "To", temp_buf)) {
		temp_buf.Decode_EncodedWordRFC2047(decode_buf, 0, 0);
		AddS(decode_buf, &HFP.ToP);
	}
	else if(IsFieldHeader(rLineBuf, "CC", temp_buf)) {
		temp_buf.Decode_EncodedWordRFC2047(decode_buf, 0, 0);
		AddS(decode_buf, &HFP.CcP);
	}
	else if(IsFieldHeader(rLineBuf, "Return-Path", temp_buf)) {
		AddS(temp_buf, &HFP.ReturnPathP);
	}
	else if(IsFieldHeader(rLineBuf, "Subject", temp_buf)) {
		temp_buf.Decode_EncodedWordRFC2047(decode_buf, 0, 0);
		AddS(decode_buf, &HFP.SubjP);
	}
	else if(IsFieldHeader(rLineBuf, "Thread-Topic", temp_buf)) {
	}
	else if(IsFieldHeader(rLineBuf, "Thread-Index", temp_buf)) {
	}
	else if(IsFieldHeader(rLineBuf, "Organization", temp_buf)) {
		AddS(temp_buf, &HFP.OrganizationP);
	}
	else if(IsFieldHeader(rLineBuf, "Message-ID", temp_buf)) {
		AddS(temp_buf, &HFP.MsgIdP);
	}
	else if(IsFieldHeader(rLineBuf, "Date", temp_buf)) {
		strtodatetime(temp_buf, &HFP.Dtm, DATF_INTERNET, TIMF_HMS);
	}
	else if(IsFieldHeader(rLineBuf, "User-Agent", temp_buf)) {
		AddS(temp_buf, &HFP.UserAgentP);
	}
	else if(IsFieldHeader(rLineBuf, "x-originating-ip", temp_buf)) {
		AddS(temp_buf, &HFP.XOrgIpP);
	}
	else if(IsFieldHeader(rLineBuf, "MIME-Version", temp_buf)) {
		HFP.MimeVer.FromStr(temp_buf);
	}
	else if(IsFieldHeader(rLineBuf, "Content-Type", temp_buf)) {
		SString left, right;
		StringSet ss;
		temp_buf.Tokenize(";", ss);
		uint   ssp = 0;
		if(ss.get(&ssp, temp_buf)) {
			SString boundary_ident;
			AddS(temp_buf.Strip(), &rBlk.P_B->Ct.MimeP);
			while(ss.get(&ssp, temp_buf)) {
				if(temp_buf.Divide('=', left, right)) {
					left.Strip();
					if(left.IsEqiAscii("type")) {
						AddS(right.Strip().StripQuotes().Strip(), &rBlk.P_B->Ct.TypeP);
					}
					if(left.IsEqiAscii("name")) {
						right.Strip().StripQuotes().Strip().Decode_EncodedWordRFC2047(decode_buf, 0, 0);
						AddS(decode_buf, &rBlk.P_B->Ct.NameP);
					}
					else if(left.IsEqiAscii("boundary")) {
						boundary_ident = right.Strip().StripQuotes().Strip();
						//AddS(boundary_ident, &rBlk.P_B->Ct.BoundaryP);
					}
					else if(left.IsEqiAscii("charset")) {
						rBlk.P_B->Ct.Cp.FromStr(right.Strip().StripQuotes());
					}
				}
			}
			if(boundary_ident.NotEmpty()) {
				if(rBlk.State == rBlk.stMimePartHeader) {
					Boundary * p_new_b = rBlk.P_B->Children.CreateNewItem();
					THROW(p_new_b);
					AddS(boundary_ident, &p_new_b->Ct.BoundaryP);
					p_new_b->P_Parent = rBlk.P_B;
				}
				else if(rBlk.State == rBlk.stHeader) {
					AddS(boundary_ident, &rBlk.P_B->Ct.BoundaryP);
				}
			}
		}
	}
	else if(IsFieldHeader(rLineBuf, "Content-Language", temp_buf)) {
	}
	else if(IsFieldHeader(rLineBuf, "Accept-Language", temp_buf)) {
	}
	else if(IsFieldHeader(rLineBuf, "Content-Transfer-Encoding", temp_buf)) {
		rBlk.P_B->ContentTransfEnc = SFileFormat::IdentifyContentTransferEnc(temp_buf.Strip());
	}
	else if(IsFieldHeader(rLineBuf, "Content-Disposition", temp_buf)) {
		// Content-Disposition: inline; filename="image002.png"; size=4980;creation-date="Tue, 03 Oct 2017 08:41:39 GMT";modification-date="Tue, 03 Oct 2017 08:41:39 GMT"
		SString left, right;
		StringSet ss;
		temp_buf.Tokenize(";", ss);
		uint   ssp = 0;
		if(ss.get(&ssp, temp_buf)) {
			rBlk.P_B->Cd.Type = ContentDispositionBlock::IdentifyType(temp_buf.Strip());
			while(ss.get(&ssp, temp_buf)) {
				if(temp_buf.Divide('=', left, right)) {
					left.Strip();
					if(left.IsEqiAscii("filename")) {
						right.Strip().StripQuotes().Strip().Decode_EncodedWordRFC2047(decode_buf, 0, 0);
						AddS(decode_buf, &rBlk.P_B->Cd.FileNameP);
					}
					if(left.IsEqiAscii("name")) {
						right.Strip().StripQuotes().Strip().Decode_EncodedWordRFC2047(decode_buf, 0, 0);
						AddS(decode_buf, &rBlk.P_B->Cd.NameP);
					}
					else if(left.IsEqiAscii("size")) {
						rBlk.P_B->Cd.Size = (uint64)right.ToInt64();
					}
					else if(left.IsEqiAscii("creation-date")) {
						strtodatetime(right, &rBlk.P_B->Cd.CrDtm, DATF_INTERNET, TIMF_HMS);
					}
					else if(left.IsEqiAscii("modification-date")) {
						strtodatetime(right, &rBlk.P_B->Cd.ModifDtm, DATF_INTERNET, TIMF_HMS);
					}
					else if(left.IsEqiAscii("read-date")) {
						strtodatetime(right, &rBlk.P_B->Cd.RdDtm, DATF_INTERNET, TIMF_HMS);
					}
				}
			}
		}
	}
	else if(IsFieldHeader(rLineBuf, "Content-Description", temp_buf)) {
		AddS(temp_buf, &rBlk.P_B->ContentDescrP);
	}
	else if(IsFieldHeader(rLineBuf, "Content-ID", temp_buf)) {
		AddS(temp_buf, &rBlk.P_B->ContentIdP);
	}
	CATCHZOK
	return ok;
}

const SMailMessage::Boundary * FASTCALL SMailMessage::SearchBoundary(const Boundary * pB) const
{
	return Helper_SearchBoundary(pB, &B);
}

const SMailMessage::Boundary * FASTCALL SMailMessage::Helper_SearchBoundary(const Boundary * pB, const Boundary * pParent) const
{
	const Boundary * p_result = 0;
	if(pParent) {
		if(pParent == pB)
			p_result = pB;
		else
			for(uint i = 0; !p_result && i < pParent->Children.getCount(); i++)
				p_result = Helper_SearchBoundary(pB, pParent->Children.at(i)); // @recursion
	}
	return p_result;
}

SMailMessage::Boundary * FASTCALL SMailMessage::SearchBoundary(const SString & rIdent)
{
	return Helper_SearchBoundary(rIdent, &B);
}

SMailMessage::Boundary * SMailMessage::Helper_SearchBoundary(const SString & rIdent, Boundary * pParent)
{
	Boundary * p_result = 0;
	if(pParent) {
		SString boundary_ident;
		if(pParent->Ct.BoundaryP && GetS(pParent->Ct.BoundaryP, boundary_ident) && rIdent.HasPrefixNC(boundary_ident)) {
			p_result = pParent;
		}
		else {
			for(uint i = 0; !p_result && i < pParent->Children.getCount(); i++)
				p_result = Helper_SearchBoundary(rIdent, pParent->Children.at(i)); // @recursion
		}
	}
	return p_result;
}

int SMailMessage::ReadFromFile(SFile & rF)
{
	int    ok = -1;
	int    prev_full_line_was_empty = 0;
	SString line_buf;
	SString prev_line;
	SString full_line;
	TSStack <Boundary *> boundary_pos_stk;
	ParserBlock blk;
	blk.P_B = &B;
	while(rF.ReadLine(line_buf)) {
		blk.LineNo++;
		line_buf.Chomp();
		if(line_buf.Len() && oneof2(line_buf[0], ' ', '\t')) {
			if(line_buf[0] == '\t')
				full_line./*Space().*/Cat(line_buf.cptr()+1);
			else
				full_line.Cat(line_buf.cptr()+1);
		}
		else {
			if(blk.LineNo > 1 && (!full_line.IsEmpty() || !prev_full_line_was_empty)) {
				THROW(ProcessInputLine(blk, full_line));
			}
			prev_full_line_was_empty = full_line.IsEmpty();
			full_line = line_buf;
		}
		ok = 1;
	}
	THROW(ProcessInputLine(blk, full_line));
	CATCHZOK
	return ok;
}

uint SMailMessage::GetAttachmentCount() const
{
	return B.GetAttachmentCount();
}

const SMailMessage::Boundary * FASTCALL SMailMessage::GetAttachmentByIndex(uint attIdx) const
{
	return B.GetAttachmentByIndex(attIdx);
}

int SMailMessage::GetAttachmentFileName(const SMailMessage::Boundary * pB, SString & rFileName) const
{
	rFileName.Z();
	int    ok = 0;
	if(pB) {
		if(pB->Cd.FileNameP) {
			GetS(pB->Cd.FileNameP, rFileName);
			if(rFileName.NotEmpty())
				ok = 1;
		}
		if(!ok && pB->Cd.NameP) {
			GetS(pB->Cd.NameP, rFileName);
			if(rFileName.NotEmpty())
				ok = 2;
		}
		if(!ok && pB->Ct.NameP) {
			GetS(pB->Ct.NameP, rFileName);
			if(rFileName.NotEmpty())
				ok = 3;
		}
	}
	return ok;
}

int SMailMessage::SaveAttachmentTo(uint attIdx, const char * pDestPath, SString * pResultFileName) const
{
	int    ok = 0;
	SString result_file_name;
	const Boundary * p_attm = B.GetAttachmentByIndex(attIdx);
	if(p_attm) {
		SString mime_type;
		SString mime_ext;
		SString path;
		SString file_name;
		if(p_attm->Cd.FileNameP)
			GetS(p_attm->Cd.FileNameP, file_name);
		if(file_name.IsEmpty() && p_attm->Cd.NameP)
			GetS(p_attm->Cd.NameP, file_name);
		if(file_name.IsEmpty() && p_attm->Ct.NameP)
			GetS(p_attm->Ct.NameP, file_name);
		if(p_attm->Ct.MimeP) {
			GetS(p_attm->Ct.MimeP, mime_type);
			if(mime_type.NotEmpty()) {
				SFileFormat ff;
				if(ff.IdentifyMime(mime_type)) {
					SFileFormat::GetExt(ff, mime_ext);
				}
			}
		}
		{
			const SFsPath ps(pDestPath);
			if(ps.Nam.NotEmpty()) {
				result_file_name = pDestPath;
			}
			else {
				ps.Merge(SFsPath::fDrv|SFsPath::fDir, path);
				if(file_name.IsEmpty())
					MakeTempFileName(path, "eml", mime_ext, 0, result_file_name);
				else {
					file_name.Transf(CTRANSF_UTF8_TO_OUTER);
					(result_file_name = path).SetLastSlash().Cat(file_name);
				}
			}
			SFile f_out(result_file_name, SFile::mWrite|SFile::mBinary);
			THROW(f_out.IsValid());
			THROW(f_out.Write(p_attm->Data.GetBuf(p_attm->Data.GetRdOffs()), p_attm->Data.GetAvailableSize()));
			ASSIGN_PTR(pResultFileName, result_file_name);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

SMailMessage::Boundary * SMailMessage::Helper_CreateBoundary(SMailMessage::Boundary * pParent, int format)
{
	SMailMessage::Boundary * p_result = 0;
	SString temp_buf;
	THROW(!pParent || SearchBoundary(pParent));
	SETIFZ(pParent, &B);
	THROW(p_result = new Boundary);
	THROW(SFileFormat::GetMime(format, temp_buf) > 0 || SFileFormat::GetMime(SFileFormat::Unkn, temp_buf) > 0);
	AddS(temp_buf, &p_result->Ct.MimeP);
	THROW(pParent->Children.insert(p_result));
	p_result->P_Parent = pParent;
	if(!pParent->Ct.BoundaryP) {
		S_GUID(SCtrGenerate()).ToStr(S_GUID::fmtIDL, temp_buf);
		AddS(temp_buf, &pParent->Ct.BoundaryP);
		//
		GetS(pParent->Ct.MimeP, temp_buf);
		if(temp_buf.IsEmpty())
			AddS("multipart/mixed", &pParent->Ct.MimeP);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

SMailMessage::Boundary * SMailMessage::AttachContent(SMailMessage::Boundary * pB, int format, SCodepageIdent cp, const void * pData, size_t dataSize)
{
	SMailMessage::Boundary * p_result = Helper_CreateBoundary(pB, format);
	THROW(p_result);
	p_result->Ct.Cp = cp;
	if(pData && dataSize) {
		THROW(p_result->Data.Write(pData, dataSize));
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

SMailMessage::Boundary * SMailMessage::AttachFile(Boundary * pB, int format, const char * pFilePath)
{
	SMailMessage::Boundary * p_result = 0;
	THROW(fileExists(pFilePath));
	if(format == SFileFormat::Unkn) {
		SFileFormat ff;
		int ffr = ff.Identify(pFilePath, 0);
		if(ffr > 0)
			format = ff;
	}
	{
		THROW(!pB || SearchBoundary(pB));
		SETIFZ(pB, &B);
		if(pB->Children.getCount() == 0) {
			//
			// Если в boundary нет ни одного дочернего элемента, то необходимо создать
			// текстовый дочерний boundary иначе наш attachment может быть не верно истолкован
			//
			THROW(AttachContent(pB, SFileFormat::Txt, cpUTF8, "", 0));
		}
	}
	THROW(p_result = Helper_CreateBoundary(pB, format));
	{
		SString temp_buf;
		SFsPath::NormalizePath(pFilePath, SFsPath::npfSlash, temp_buf);
		AddS(temp_buf, &p_result->OuterFileNameP);
		{
			SFile::Stat fs;
			SFile::GetStat(pFilePath, 0, &fs, 0);
			p_result->Cd.CrDtm.SetNs100(fs.CrtTm_);
			p_result->Cd.ModifDtm.SetNs100(fs.ModTm_);
			p_result->Cd.Size = fs.Size;

			SFsPath ps(pFilePath);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
			AddS(temp_buf, &p_result->Cd.NameP);
			p_result->Cd.FileNameP = p_result->Cd.NameP;
			p_result->Ct.NameP = p_result->Cd.NameP; // В Content-Type тоже надо имя вставить
		}
		p_result->Cd.Type = ContentDispositionBlock::tAttachment;
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int SMailMessage::PreprocessEmailAddrString(const SString & rSrc, SString & rResult, StringSet * pSs) const
{
	rResult.Z();

	int    ok = 0;
	int    is_err = 0;
	StringSet ss_to;
	rSrc.Tokenize(",", ss_to);

	STokenRecognizer tr;
	SNaturalTokenArray nta;
	SString name_buf, addr_buf;
	SString temp_buf;
	SString fragment_buf;
	for(uint ssp = 0; ss_to.get(&ssp, temp_buf);) {
		size_t ang_pos = 0;
		nta.clear();
		tr.Run(temp_buf.Strip().ucptr(), -1, nta, 0);
		if(nta.Has(SNTOK_EMAIL) > 0.0f) {
			rResult.CatDivIfNotEmpty(',', 0);
			if(rResult.Len() > 70)
				rResult.CRB().Space();
			rResult.Cat(temp_buf);
			CALLPTRMEMB(pSs, add(temp_buf));
			ok = 1;
		}
		else if(temp_buf.SearchChar('<', &ang_pos)) {
			//
			// Предполагаем, что имя получателя (name_buf) задано в UTF8
			//
			temp_buf.Sub(0, ang_pos, name_buf);
			temp_buf.Sub(ang_pos, temp_buf.Len()-ang_pos, addr_buf);
			assert(addr_buf.C(0) == '<');
			if(addr_buf.Last() == '>') {
				addr_buf.TrimRight().ShiftLeft();
				nta.clear();
				tr.Run(addr_buf.ucptr(), -1, nta, 0);
				if(nta.Has(SNTOK_EMAIL) > 0.0f) {
					temp_buf.Encode_EncodedWordRFC2047(name_buf, cpUTF8, SString::rfc2207encMime64);
					rResult.CatDivIfNotEmpty(',', 0);
					if(rResult.Len() > 70)
						rResult.CRB().Space();
					rResult.Cat(temp_buf);
					if(rResult.Len() > 70)
						rResult.CRB().Space();
					else
						rResult.Space();
					rResult.CatChar('<').Cat(addr_buf).CatChar('>');
					if(pSs) {
						//pSs->add(fragment_buf.Z().Cat(temp_buf).Space().CatChar('<').Cat(addr_buf).CatChar('>'));
						pSs->add(fragment_buf.Z().Cat(addr_buf));
					}
					ok = 1;
				}
				else
					is_err = 1;
			}
			else
				is_err = 1;
		}
		else
			is_err = 1;
	}
	return ok;
}

static void EncodedStringWithWrapping(const char * pOrgBuf, uint hdrLen, SString & rResult)
{
	const  size_t org_len_mb = sstrlen(pOrgBuf);
	if(org_len_mb) {
		SString temp_buf;
		temp_buf.Encode_EncodedWordRFC2047(pOrgBuf, cpUTF8, SString::rfc2207encMime64);
		if((temp_buf.Len() + hdrLen) >= 78) {
			SStringU org_buf_u;
			org_buf_u.CopyFromUtf8(pOrgBuf, org_len_mb);
			const size_t org_len_u = org_buf_u.Len();
			const size_t ovrhd_sz = 12;
			const double rel = fdivui(org_len_u, temp_buf.Len() - ovrhd_sz);
			SString chunk_buf;
			size_t offs = (size_t)((78 - hdrLen - ovrhd_sz) * rel);
			chunk_buf.CopyUtf8FromUnicode(org_buf_u, offs, 0);
			temp_buf.Encode_EncodedWordRFC2047(chunk_buf, cpUTF8, SString::rfc2207encMime64);
			rResult.Cat(temp_buf);
			if(offs < org_len_u) {
				rResult.CRB().Space();
				temp_buf.CopyUtf8FromUnicode(org_buf_u + offs, org_len_u - offs, 0);
				EncodedStringWithWrapping(temp_buf, 1, rResult); // @recursion
			}
		}
		else
			rResult.Cat(temp_buf);
	}
}

SMailMessage::WriterBlock::WriterBlock(const SMailMessage & rMsg) : R_Msg(rMsg), Phase(phsUndef), P_Cb(0), RdDataOff(0), P_InStream(0)
{
}

SMailMessage::WriterBlock::~WriterBlock()
{
	ZDELETE(P_InStream);
}

int SMailMessage::WriterBlock::Read(size_t maxChunkSize, SBuffer & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	SString line_buf;
	SString result_buf;
	SString out_buf;
	if(Phase == phsUndef) {
		Phase = phsHeader;
	}
	if(Phase == phsStop)
		ok = -1;
	else if(Phase == phsHeader) {
		temp_buf.Z().Cat("Date").CatDiv(':', 2).CatCurDateTime(DATF_INTERNET, TIMF_HMS|TIMF_TIMEZONE);
		out_buf.Cat(temp_buf).CRB();
		R_Msg.GetField(SMailMessage::fldFrom, temp_buf);
		if(temp_buf.NotEmptyS()) {
			if(R_Msg.PreprocessEmailAddrString(temp_buf, result_buf, 0))
				out_buf.Cat("From").CatDiv(':', 2).Cat(result_buf).CRB();
		}
		R_Msg.GetField(SMailMessage::fldMailer, temp_buf);
		if(temp_buf.NotEmptyS())
			out_buf.Cat("X-Mailer").CatDiv(':', 2).Cat(temp_buf).CRB();
		{
			out_buf.Cat("Message-ID").CatDiv(':', 2).CatChar('<').Cat(S_GUID(SCtrGenerate()).ToStr(S_GUID::fmtIDL, temp_buf)).CatChar('>').CRB();
		}
		{
			R_Msg.GetField(SMailMessage::fldTo, temp_buf);
			if(R_Msg.PreprocessEmailAddrString(temp_buf, result_buf, 0))
				out_buf.Cat("To").CatDiv(':', 2).Cat(result_buf).CRB();
			R_Msg.GetField(SMailMessage::fldCc, temp_buf);
			if(R_Msg.PreprocessEmailAddrString(temp_buf, result_buf, 0))
				out_buf.Cat("Cc").CatDiv(':', 2).Cat(result_buf).CRB();
		}
		R_Msg.GetField(SMailMessage::fldSubj, temp_buf);
		if(temp_buf.NotEmptyS()) {
			const char * p_subj = "Subject";
			EncodedStringWithWrapping(temp_buf, sstrlen(p_subj) + 2, line_buf.Z());
			//result_buf.Encode_EncodedWordRFC2047(temp_buf, cpUTF8, SString::rfc2207encMime64);
			out_buf.Cat(p_subj).CatDiv(':', 2).Cat(/*result_buf*/line_buf).CRB();
		}
		rBuf.Write(out_buf.ucptr(), out_buf.Len());
		//
		// Переключаемся на следующую фазу
		//
		Phase = phsBoundaryHeader;
		P_Cb = &R_Msg.B;
	}
	else if(Phase == phsBoundaryHeader) {
		SETIFZ(P_Cb, &R_Msg.B);
		{
			if(P_Cb->P_Parent) {
				out_buf.CRB();
				R_Msg.GetS(P_Cb->P_Parent->Ct.BoundaryP, temp_buf);
				if(temp_buf.NotEmptyS())
					out_buf.Cat("--").Cat(temp_buf).CRB();
			}
			R_Msg.GetS(P_Cb->Ct.MimeP, temp_buf);
			out_buf.Cat("Content-Type").CatDiv(':', 2);
			int    do_put_semicol = 0;
			if(temp_buf.NotEmptyS()) {
				out_buf.Cat(temp_buf);
				do_put_semicol = 1;
			}
			if(P_Cb->Ct.Cp != cpUndef) {
				P_Cb->Ct.Cp.ToStr(SCodepageIdent::fmtXML, temp_buf);
				if(temp_buf.NotEmptyS()) {
					if(do_put_semicol)
						out_buf.CatDiv(';', 2);
					out_buf.CatEqQ("charset", temp_buf);
					do_put_semicol = 1;
				}
			}
			R_Msg.GetS(P_Cb->Ct.NameP, temp_buf);
			if(temp_buf.NotEmptyS()) {
				if(do_put_semicol)
					out_buf.CatDiv(';', 2);
				out_buf.CatEqQ("name", temp_buf);
				do_put_semicol = 1;
			}
			R_Msg.GetS(P_Cb->Ct.BoundaryP, temp_buf);
			if(temp_buf.NotEmptyS()) {
				if(do_put_semicol)
					out_buf.CatDiv(';', 2);
				out_buf.CatEqQ("boundary", temp_buf);
				do_put_semicol = 1;
			}
			out_buf.CRB();
		}
		{
			SFileFormat::GetContentTransferEncName(/*P_Cb->ContentTransfEnc*/SFileFormat::cteBase64, temp_buf);
			if(temp_buf.NotEmptyS())
				out_buf.Cat("Content-Transfer-Encoding").CatDiv(':', 2).Cat(temp_buf).CRB();
		}
		if(ContentDispositionBlock::GetTypeName(P_Cb->Cd.Type, temp_buf)) {
			line_buf.Z();
			line_buf.Cat("Content-Disposition").CatDiv(':', 2).Cat(temp_buf);
			R_Msg.GetS(P_Cb->Cd.NameP, temp_buf);
			if(temp_buf.NotEmptyS()) {
				if(line_buf.Len() >= 77) {
					out_buf.Cat(line_buf.Semicol().CRB());
					line_buf.Z().Space();
				}
				else
					line_buf.CatDiv(';', 2);
				line_buf.CatEqQ("name", temp_buf);
			}
			R_Msg.GetS(P_Cb->Cd.FileNameP, temp_buf);
			if(temp_buf.NotEmptyS())
				line_buf.CatDiv(';', 2).CatEqQ("filename", temp_buf);
			if(P_Cb->Cd.Size) {
				temp_buf.Z().Cat(P_Cb->Cd.Size);
				if(line_buf.Len() >= 77) {
					out_buf.Cat(line_buf.Semicol().CRB());
					line_buf.Z().Space();
				}
				else
					line_buf.CatDiv(';', 2);
				line_buf.CatEqQ("size", temp_buf);
			}
			if(!!P_Cb->Cd.CrDtm) {
				temp_buf.Z().Cat(P_Cb->Cd.CrDtm, DATF_INTERNET, TIMF_HMS);
				if(line_buf.Len() >= 77) {
					out_buf.Cat(line_buf.Semicol().CRB());
					line_buf.Z().Space();
				}
				else
					line_buf.CatDiv(';', 2);
				line_buf.CatEqQ("creation-date", temp_buf);
			}
			if(!!P_Cb->Cd.ModifDtm) {
				temp_buf.Z().Cat(P_Cb->Cd.ModifDtm, DATF_INTERNET, TIMF_HMS);
				if(line_buf.Len() >= 77) {
					out_buf.Cat(line_buf.Semicol().CRB());
					line_buf.Z().Space();
				}
				else
					line_buf.CatDiv(';', 2);
				line_buf.CatEqQ("modification-date", temp_buf);
			}
			if(!!P_Cb->Cd.RdDtm) {
				temp_buf.Z().Cat(P_Cb->Cd.RdDtm, DATF_INTERNET, TIMF_HMS);
				if(line_buf.Len() >= 77) {
					out_buf.Cat(line_buf.Semicol().CRB());
					line_buf.Z().Space();
				}
				else
					line_buf.CatDiv(';', 2);
				line_buf.CatEqQ("read-date", temp_buf);
			}
			line_buf.CRB();

			out_buf.Cat(line_buf);
		}
		R_Msg.GetS(P_Cb->ContentIdP, temp_buf);
		if(temp_buf.NotEmptyS())
			out_buf.Cat("Content-ID").CatDiv(':', 2).Cat(temp_buf).CRB();
		R_Msg.GetS(P_Cb->ContentDescrP, temp_buf);
		if(temp_buf.NotEmptyS())
			out_buf.Cat("Content-Description").CatDiv(':', 2).Cat(temp_buf).CRB();
		out_buf.CRB(); // Перевод каретки перед содержанием
		rBuf.Write(out_buf.ucptr(), out_buf.Len());
		//
		Phase = phsBoundaryBody;
		RdDataOff = 0;
	}
	else if(Phase == phsBoundaryBody) {
		assert(P_Cb);
		uint8  mime_buf[256];
		int   goto_next_boundary = 0;
		const size_t start_out_offs = rBuf.GetWrOffs();
		if(!P_Cb->OuterFileNameP) {
			const size_t avail_size = P_Cb->Data.GetAvailableSize();
			if(RdDataOff < avail_size) {
				//
				// Note: Из буфера P_Cb->Data мы здесь читаем не меняя позиции его внутреннего указателя чтения
				// для того, чтобы сохранить константность объекта.
				//
				int    is_broken = 0; // Признак того, что процесс чтения был искусственно прерван
				const uint8 * p_src_buf = PTR8C(P_Cb->Data.GetBuf(P_Cb->Data.GetRdOffs()));
				while(!is_broken && RdDataOff < avail_size) {
					const size_t actual_size = MIN((avail_size-RdDataOff), 57);
					memcpy(mime_buf, (p_src_buf + RdDataOff), actual_size);
					RdDataOff += actual_size;
					temp_buf.EncodeMime64(mime_buf, actual_size).CRB();
					rBuf.Write(temp_buf.ucptr(), temp_buf.Len());
					if(maxChunkSize && (rBuf.GetWrOffs() - start_out_offs) >= maxChunkSize)
						is_broken = 1;
				}
				if(!is_broken)
					goto_next_boundary = 1;
			}
			else
				goto_next_boundary = 1;
		}
		else {
			if(!P_InStream) {
				R_Msg.GetS(P_Cb->OuterFileNameP, temp_buf);
				THROW(fileExists(temp_buf));
				THROW(P_InStream = new SFile(temp_buf, SFile::mRead|SFile::mBinary));
				THROW(P_InStream->IsValid());
			}
			{
				size_t actual_size = 0;
				int    is_broken = 0; // Признак того, что процесс чтения был искусственно прерван
				int    rr = 0;
				while(!is_broken && (rr = P_InStream->Read(mime_buf, 57, &actual_size)) > 0) {
					temp_buf.EncodeMime64(mime_buf, actual_size).CRB();
					rBuf.Write(temp_buf.ucptr(), temp_buf.Len());
					if(maxChunkSize && (rBuf.GetWrOffs() - start_out_offs) >= maxChunkSize)
						is_broken = 1;
				}
				if(!is_broken)
					goto_next_boundary = 1;
			}
		}
		if(goto_next_boundary) {
			int   do_close_boundary = 0;
			ZDELETE(P_InStream);
			RdDataOff = 0;
			if(P_Cb->Children.getCount()) {
				if(P_Cb->P_Parent)
					do_close_boundary = 1;
				Phase = phsBoundaryHeader;
				P_Cb = P_Cb->Children.at(0);
			}
			else if(P_Cb->P_Parent) {
				const Boundary * p_next = 0;
				const Boundary * p_parent = P_Cb->P_Parent;
				while(p_parent && !p_next) {
					const uint par_cnt = p_parent->Children.getCount();
					int   is_found = 0;
					for(uint i = 0; !is_found && i < par_cnt; i++) {
						const Boundary * p_item = p_parent->Children.at(i);
						if(p_item == P_Cb) {
							is_found = 1;
							p_next = (i < (par_cnt-1)) ? p_parent->Children.at(i+1) : 0;
						}
					}
					assert(is_found);
					if(!p_next)
						p_parent = p_parent->P_Parent;
				}
				if(p_next) {
					Phase = phsBoundaryHeader;
					P_Cb = p_next;
				}
				else {
					Phase = phsStop;
					do_close_boundary = 1;
				}
			}
			else {
				Phase = phsStop;
				do_close_boundary = 1;
			}
			if(do_close_boundary && P_Cb->P_Parent) {
				out_buf.Z(); // Пустая строка не нужна - перевод каретки не добавляем
				R_Msg.GetS(P_Cb->P_Parent->Ct.BoundaryP, temp_buf);
				if(temp_buf.NotEmptyS())
					out_buf.Cat("--").Cat(temp_buf).Cat("--").CRB();
				rBuf.Write(out_buf.ucptr(), out_buf.Len());
			}
		}
	}
	else
		ok = -1;
	CATCH
		ZDELETE(P_InStream);
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
static const SIntToSymbTabEntry HttpHeaderTitles[] = {
	{ SHttpProtocol::hdrNone, "" },
	{ SHttpProtocol::hdrCacheControl, "Cache-Control" },
	{ SHttpProtocol::hdrConnection, "Connection" },
	{ SHttpProtocol::hdrDate, "Date" },
	{ SHttpProtocol::hdrPragma, "Pragma" },
	{ SHttpProtocol::hdrTrailer, "Trailer" },
	{ SHttpProtocol::hdrTransferEnc, "Transfer-Encoding" },
	{ SHttpProtocol::hdrUpgrade, "Upgrade" },
	{ SHttpProtocol::hdrVia, "Via" },
	{ SHttpProtocol::hdrWarning, "Warning" },
	{ SHttpProtocol::hdrAllow, "Allow" },
	{ SHttpProtocol::hdrContentEnc, "Content-Encoding" },
	{ SHttpProtocol::hdrContentLang, "Content-Language" },
	{ SHttpProtocol::hdrContentLen, "Content-Length" },
	{ SHttpProtocol::hdrContentLoc, "Content-Location" },
	{ SHttpProtocol::hdrContentMD5, "Content-MD5" },
	{ SHttpProtocol::hdrContentRange, "Content-Range" },
	{ SHttpProtocol::hdrContentType, "Content-Type" },
	{ SHttpProtocol::hdrExpires, "Expires" },
	{ SHttpProtocol::hdrLastModif, "Last-Modified" },
	{ SHttpProtocol::hdrAcceptRanges, "Accept-Ranges" },
	{ SHttpProtocol::hdrAge, "Age" },
	{ SHttpProtocol::hdrExtTag, "ETag" },
	{ SHttpProtocol::hdrLoc, "Location" },
	{ SHttpProtocol::hdrAuthent, "Proxy-Authenticate" },
	{ SHttpProtocol::hdrRetryAfter, "Retry-After" },
	{ SHttpProtocol::hdrServer, "Server" },
	{ SHttpProtocol::hdrVary, "Vary" },
	{ SHttpProtocol::hdrWwwAuthent, "WWW-Authenticate" },
	{ SHttpProtocol::hdrAccept, "Accept" },
	{ SHttpProtocol::hdrAcceptCharset, "Accept-Charset" },
	{ SHttpProtocol::hdrAcceptEnc, "Accept-Encoding" },
	{ SHttpProtocol::hdrAcceptLang, "Accept-Language" },
	{ SHttpProtocol::hdrAuthorization, "Authorization" },
	{ SHttpProtocol::hdrExpect, "Expect" },
	{ SHttpProtocol::hdrFrom, "From" },
	{ SHttpProtocol::hdrHost, "Host" },
	{ SHttpProtocol::hdrIfMatch, "If-Match" },
	{ SHttpProtocol::hdrIfModifSince, "If-Modified-Since" },
	{ SHttpProtocol::hdrIfNonMatch, "If-None-Match" },
	{ SHttpProtocol::hdrIfRange, "If-Range" },
	{ SHttpProtocol::hdrIfUnmodifSince, "If-Unmodified-Since" },
	{ SHttpProtocol::hdrMaxForwards, "Max-Forwards" },
	{ SHttpProtocol::hdrProxiAuth, "Proxy-Authorization" },
	{ SHttpProtocol::hdrRange, "Range" },
	{ SHttpProtocol::hdrReferer, "Referer" },
	{ SHttpProtocol::hdrTransferExt, "TE" },
	{ SHttpProtocol::hdrUserAgent, "User-Agent" },
	{ SHttpProtocol::hdrSoapAction, "SoapAction" },
	{ SHttpProtocol::hdrAuthToken, "authorization-token" },
	{ SHttpProtocol::hdrAuthSecret, "authorization-secret" },
	{ SHttpProtocol::hdrXOriginRequestId, "X-Origin-Request-Id" },
	{ SHttpProtocol::hdrXTimestamp, "X-Timestamp" },
	{ SHttpProtocol::hdrXApiKey, "X-API-KEY" }, // @v11.9.11
};

/*static*/bool FASTCALL SHttpProtocol::GetHeaderTitle(int hdr, SString & rTitle)
	{ return SIntToSymbTab_GetSymb(HttpHeaderTitles, SIZEOFARRAY(HttpHeaderTitles), hdr, rTitle); }
/*static*/int FASTCALL SHttpProtocol::GetHeaderId(const char * pTitle)
	{ return SIntToSymbTab_GetId(HttpHeaderTitles, SIZEOFARRAY(HttpHeaderTitles), pTitle); }

/*static*/bool SHttpProtocol::SetHeaderField(StrStrAssocArray & rFldList, int titleId, const char * pValue)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	bool   ok = GetHeaderTitle(titleId, r_temp_buf);
	if(ok)
		rFldList.Add(r_temp_buf, pValue);
	return ok;
}

/*static*/bool SHttpProtocol::IsThereHeaderField(const StrStrAssocArray & rFldList, int titleId, SString * pValue)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	return GetHeaderTitle(titleId, r_temp_buf) ? rFldList.Search(r_temp_buf, pValue, 0) : false;
}

/*static*/uint FASTCALL SHttpProtocol::PutHeaderFieldsIntoString(const StrStrAssocArray & rFldList, SString & rBuf)
{
	rBuf.Z();
	uint   c = 0;
	for(uint i = 0; i < rFldList.getCount(); i++) {
		SStrToStrAssoc item = rFldList.at(i);
		if(!isempty(item.Key)) {
			rBuf.CatDivIfNotEmpty('\n', 0); //if(rBuf.NotEmpty()) rBuf.Cat("\xD\xA");
			rBuf.Cat(item.Key);
			if(!isempty(item.Val))
				rBuf.CatDiv(':', 2).Cat(item.Val);
			c++;
		}
	}
	return c;
}

/*static*/int FASTCALL SHttpProtocol::ParseAuth(const char * pAuthParam, SHttpProtocol::Auth & rResult)
{
	int    ok = 0;
	rResult.Type = authtUnkn;
	rResult.Login.Z();
	rResult.Password.Z();
	SString temp_buf(pAuthParam);
	if(temp_buf.NotEmptyS()) {
		const char * p_basic_prefix = "Basic";
		if(temp_buf.HasPrefixIAscii(p_basic_prefix)) {
			rResult.Type = authtBasic;
			temp_buf.ShiftLeft(sstrlen(p_basic_prefix)).Strip();
			size_t real_len = 0;
			char   auth_buf[512];
			temp_buf.DecodeMime64(auth_buf, sizeof(auth_buf), &real_len);
			temp_buf.Z().CatN(auth_buf, real_len);
			temp_buf.Divide(':', rResult.Login, rResult.Password);
			rResult.Login.Strip();
			rResult.Password.Strip();
			ok = 1;
		}
	}
	return ok;
}

#if 0 // {

content_type_t * content_type_new(const char * content_type_str)
{
	hpair_t * pair = NULL, * last = NULL;
	content_type_t * ct;
	char ch, key[256], value[256];
	int inQuote = 0, i = 0, c = 0, begin = 0, len;
	int mode = 0;
	/* 0: searching ';' 1: process key 2: process value */

	/* Create object */
	ct = (content_type_t *)SAlloc::M(sizeof(content_type_t));
	ct->params = NULL;
	len = (int)strlen(content_type_str);
	while(i <= len) {
		if(i != len)
			ch = content_type_str[i++];
		else {
			ch = ' ';
			i++;
		}
		switch(mode) {
		    case 0:
				if(ch == ';') {
					ct->type[c] = '\0';
					c = 0;
					mode = 1;
				}
				else if(ch != ' ' && ch != '\t' && ch != '\r')
					ct->type[c++] = ch;
				break;
		    case 1:
				if(ch == '=') {
					key[c] = '\0';
					c = 0;
					mode = 2;
				}
				else if(ch != ' ' && ch != '\t' && ch != '\r')
					key[c++] = ch;
				break;
		    case 2:
				if(ch != ' ')
					begin = 1;
				if((ch == ' ' || ch == ';') && !inQuote && begin) {
					value[c] = '\0';
					pair = hpairnode_new(key, value, 0);
					if(ct->params == NULL)
						ct->params = pair;
					else
						last->next = pair;
					last = pair;
					c = 0;
					begin = 0;
					mode = 1;
				}
				else if(ch == '"')
					inQuote = !inQuote;
				else if(begin && ch != '\r')
					value[c++] = ch;
				break;
		}
	}
	return ct;
}

herror_t hresponse_new_from_socket(hsocket_t * sock, hresponse_t ** out)
{
	int i = 0, count;
	herror_t status;
	hresponse_t * res;
	attachments_t * mimeMessage;
	char buffer[MAX_HEADER_SIZE+1];

read_header:                   /* for errorcode: 100 (continue) */
	/* Read header */
	while(i < MAX_HEADER_SIZE) {
		if((status = hsocket_read(sock, (byte_t *)&(buffer[i]), 1, 1, &count)) != H_OK) {
			log_error1("Socket read error");
			return status;
		}
		buffer[i+1] = '\0'; /* for strmp */
		if(i > 3) {
			if(sstreq(&(buffer[i-1]), "\n\n") || sstreq(&(buffer[i-2]), "\n\r\n"))
				break;
		}
		i++;
	}
	/* Create response */
	res = _hresponse_parse_header(buffer);
	if(!res) {
		log_error1("Header parse error");
		return herror_new("hresponse_new_from_socket", GENERAL_HEADER_PARSE_ERROR, "Can not parse response header");
	}
	/* Chec for Errorcode: 100 (continue) */
	if(res->errcode == 100) {
		hresponse_free(res);
		i = 0;
		goto read_header;
	}
	/* Create input stream */
	res->in = http_input_stream_new(sock, res->header);
	/* Check for MIME message */
	if((res->content_type && sstreq(res->content_type->type, "multipart/related"))) {
		status = mime_get_attachments(res->content_type, res->in, &mimeMessage);
		if(status != H_OK) {
			/* @todo (#1#): Handle error */
			hresponse_free(res);
			return status;
		}
		else {
			res->attachments = mimeMessage;
			http_input_stream_free(res->in);
			res->in = http_input_stream_new_from_file(mimeMessage->root_part->filename);
			if(!res->in) {
				/* @todo (#1#): Handle error */
			}
			else {
				/* res->in->deleteOnExit = 1; */
			}
		}
	}
	*out = res;
	return H_OK;
}

#endif // } 0
//
//
//
#define CURL_STATICLIB
#include <curl/curl.h>

ScURL::HttpForm::HttpForm(ScURL & rC) : R_C(rC), FH(0), LH(0), P_CMime(0)
{
}

ScURL::HttpForm::~HttpForm()
{
	curl_formfree(static_cast<struct curl_httppost *>(FH));
	curl_mime_free(static_cast<curl_mime *>(P_CMime)); // @v12.2.6
}

#if 0 // {
int ScURL::HttpForm::Add(int tagId, void * pData)
{
	int    ok = 1;
    int    ct = 0;
    switch(tagId) {
		case tCopyName: ct = CURLFORM_COPYNAME; break;
		case tPtrName: ct = CURLFORM_PTRNAME; break;
		case tCopyContents: ct = CURLFORM_COPYCONTENTS; break;
		case tPtrContents: ct = CURLFORM_PTRCONTENTS; break;
		case tContentsLength: ct = CURLFORM_CONTENTSLENGTH; break;
		case tFileContent: ct = CURLFORM_FILECONTENT; break;
		case tFile: ct = CURLFORM_FILE; break;
		case tContentType: ct = CURLFORM_CONTENTTYPE; break;
		case tFileName: ct = CURLFORM_FILENAME; break;
		case tBuffer: ct = CURLFORM_BUFFER; break;
		case tBufferPtr: ct = CURLFORM_BUFFERPTR; break;
		case tBufferLength: ct = CURLFORM_BUFFERLENGTH; break;
		case tStream: ct = CURLFORM_STREAM; break;
		case tArray: ct = CURLFORM_ARRAY; break;
		case tContentHeader: ct = CURLFORM_CONTENTHEADER; break;
    }
	if(ct) {
        if(curl_formadd((struct curl_httppost **)&FH, (struct curl_httppost **)&LH, ct, pData, CURLFORM_END) != 0)
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}
#endif // } 0

int ScURL::HttpForm::AddContentFile(const char * pFileName, const char * pContentType, const char * pContentName)
{
	int    ok = 1;
	ContentType = pContentType; // @v12.2.6
    // @v12.2.6 SString ctype(pContentType);
    SString cname(pContentName);
    if(fileExists(pFileName)) {
		if(!cname.NotEmptyS()) {
            S_GUID(SCtrGenerate()).ToStr(S_GUID::fmtIDL, cname);
		}
		/*@v12.2.6
		if(!curl_formadd((struct curl_httppost **)&FH, (struct curl_httppost **)&LH,
			CURLFORM_COPYNAME, cname.cptr(), CURLFORM_FILE, pFileName, CURLFORM_CONTENTTYPE, ctype.cptr(), CURLFORM_END))
			ok = 0;
		*/
		//
		// @v12.2.6 @construction 
		{
			bool   local_fault = false;
			CURL * h = static_cast<CURL *>(R_C.GetHandle());
			curl_mimepart * p_part = 0;
			P_CMime = curl_mime_init(h);
			if(!P_CMime) 
				local_fault = true;
			else {
				p_part = curl_mime_addpart(static_cast<curl_mime *>(P_CMime));
				if(p_part) {
					if(curl_mime_filedata(p_part, pFileName) == 0) {
						if(curl_mime_name(p_part, /*"file"*/cname) == 0) {
							//if(!curl_easy_setopt(h, CURLOPT_MIMEPOST, static_cast<curl_mime *>(P_CMime)))
							//	local_fault = true;
						}
						else
							local_fault = true;
					}
					else
						local_fault = true;
				}
				else {
					local_fault = true;
				}
			}
			if(local_fault) {
				curl_mime_free(static_cast<curl_mime *>(P_CMime));
				P_CMime = 0;
				ok = 0;
			}
		}
    }
    else
		ok = 0;
    return ok;
}

/*static*/int ScURL::_GlobalInitDone = 0;

/*static*/size_t ScURL::CbRead(char * pBuffer, size_t size, size_t nitems, void * pExtra)
{
	size_t ret = CURL_READFUNC_ABORT;
	if(pExtra) {
        SFile * p_f = static_cast<SFile *>(pExtra);
        size_t actual_size = 0;
        if(p_f->Read(pBuffer, size * nitems, &actual_size))
			ret = actual_size;
	}
	return ret;
}

/*static*/size_t ScURL::CbWrite(char * pBuffer, size_t size, size_t nmemb, void * pExtra)
{
	size_t ret = CURLE_WRITE_ERROR;
	if(pExtra) {
		SFile * p_f = static_cast<SFile *>(pExtra);
		if(p_f->Write(pBuffer, size * nmemb))
			ret = (size * nmemb);
	}
	return ret;
}

#define _CURLH (static_cast<CURL *>(H))

ScURL::ScURL() : NullWrF(0, SFile::mNullWrite), P_LogF(0)
{
	if(!_GlobalInitDone) {
		ENTER_CRITICAL_SECTION
		if(!_GlobalInitDone) {
			curl_global_init(CURL_GLOBAL_ALL);
			_GlobalInitDone = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	H = curl_easy_init();
	if(!H)
		SetError(CURLE_FAILED_INIT);
}

ScURL::~ScURL()
{
	if(H)
		curl_easy_cleanup(_CURLH);
	delete P_LogF;
}

int FASTCALL ScURL::SetError(int errCode)
{
	if(errCode) {
		SlThreadLocalArea & r_tla = SLS.GetTLA();
		if(&r_tla) {
			r_tla.LastErr = SLERR_CURL;
			r_tla.LastCurlErr = errCode;
		}
		return 0;
	}
	else
		return 1;
}

int ScURL::SetupCbRead(SFile * pF)
{
	int    ok = -1;
	if(pF) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_READFUNCTION, ScURL::CbRead)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_READDATA, pF)));
	}
	else {
		curl_easy_setopt(_CURLH, CURLOPT_READFUNCTION, 0);
		curl_easy_setopt(_CURLH, CURLOPT_READDATA, 0);
	}
	CATCHZOK
	return ok;
}

void ScURL::CleanCbRW()
{
	SetupCbRead(0);
	SetupCbWrite(0);
}

int ScURL::SetupCbWrite(SFile * pF)
{
	int    ok = 1;
	if(pF) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEFUNCTION, ScURL::CbWrite)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEDATA, pF)));
	}
	else {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEFUNCTION, ScURL::CbWrite)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEDATA, &NullWrF)));
	}
	CATCHZOK
	return ok;
}

/*static*/int ScURL::CbProgress(void * extraPtr, int64 dltotal, int64 dlnow, int64 ultotal, int64 ulnow)
{
	int    ret = 0;
	SDataMoveProgressInfo * p_data = static_cast<SDataMoveProgressInfo *>(extraPtr);
	if(p_data && p_data->Proc) {
		if(dltotal > ultotal) {
			p_data->SizeTotal = dltotal;
			p_data->SizeDone = dlnow;
		}
		else if(ultotal > dltotal) {
			p_data->SizeTotal = ultotal;
			p_data->SizeDone = ulnow;
		}
		int   r = p_data->Proc(p_data);
		if(r != SPRGRS_CONTINUE)
			ret = 1;
	}
	return ret;
}

int ScURL::SetupCbProgress(SDataMoveProgressInfo * pProgress)
{
	int    ok = 1;
	if(pProgress && pProgress->Proc) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_XFERINFOFUNCTION, ScURL::CbProgress)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_XFERINFODATA, pProgress)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_NOPROGRESS, 0)));
	}
	else {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_XFERINFOFUNCTION, nullptr)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_XFERINFODATA, nullptr)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_NOPROGRESS, 1)));
	}
	CATCHZOK
	return ok;
}

int ScURL::SetHttpFormOptions(HttpForm & rF)
{
	int    ok = 1;
	if(rF.P_CMime) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_MIMEPOST, static_cast<curl_mime *>(rF.P_CMime))));
	}
	else if(rF.FH) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_HTTPPOST, static_cast<struct curl_httppost *>(rF.FH))));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int ScURL::SetCommonOptions(int mflags, int bufferSize, const char * pUserAgent)
{
	int    ok = 1;
	if(mflags & mfDontVerifySslPeer) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 0)));
	}
	if(mflags & mfTcpKeepAlive) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_TCP_KEEPALIVE, 1L)));
	}
	if(mflags & mfNoProgerss) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_NOPROGRESS, 1L)));
	}
	if(bufferSize > 0) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_BUFFERSIZE, bufferSize)));
	}
	if(!isempty(pUserAgent)) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_USERAGENT, pUserAgent)));
	}
	if(P_LogF || LogFileName.NotEmpty() || mflags & mfVerbose) {
		if(!P_LogF) {
			SString file_name = LogFileName;
			if(file_name.IsEmpty()) {
				SLS.QueryPath("log", file_name);
				if(file_name.NotEmpty())
					file_name.SetLastSlash().Cat("curl.log");
			}
			if(file_name.NotEmpty()) {
				P_LogF = new SFile(file_name, SFile::mAppend);
			}
		}
		if(P_LogF) {
			if(P_LogF->IsValid()) {
				THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_STDERR, (FILE *)*P_LogF)));
				if(mflags & mfVerbose) {
					THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_VERBOSE, 1L)));
				}
			}
			else {
				ZDELETE(P_LogF);
				THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_STDERR, (FILE *)0)));
			}
		}
	}
	CATCHZOK
	return ok;
}

int ScURL::Execute()
{
	return SetError(curl_easy_perform(_CURLH));
}

/*static*/int ScURL::ComposeFieldList(const StrStrAssocArray * pFields, SString & rBuf, uint * pCount)
{
	int    ok = -1;
	uint   flds_count = 0;
	rBuf.Z();
	if(pFields) {
		SString temp_buf;
		for(uint i = 0; i < pFields->getCount(); i++) {
			SStrToStrAssoc item = pFields->at(i);
            if(!isempty(item.Key)) {
				if(flds_count)
					rBuf.CatChar('&');
                rBuf.Cat((temp_buf = item.Key).Strip().ToUrl());
                if(!isempty(item.Val))
					rBuf.Eq().Cat((temp_buf = item.Val).Strip().ToUrl());
                flds_count++;
            }
		}
	}
	if(flds_count)
		ok = 1;
	ASSIGN_PTR(pCount, flds_count);
	return ok;
}

/*static*/void * ScURL::ComposeHeaderList(const StrStrAssocArray * pHttpHeaderFields)
{
	int    ok = 1;
	uint   flds_count = 0;
	struct curl_slist * p_chunk = 0;
	if(pHttpHeaderFields && pHttpHeaderFields->getCount()) {
		SString temp_buf;
		SString fld_buf;
		for(uint i = 0; i < pHttpHeaderFields->getCount(); i++) {
			SStrToStrAssoc item = pHttpHeaderFields->at(i);
			temp_buf = item.Key;
            if(temp_buf.NotEmptyS()) {
				fld_buf.Z().Cat(temp_buf);
				temp_buf = item.Val;
				if(temp_buf.NotEmptyS()) {
					fld_buf.CatDiv(':', 2);
					fld_buf.Cat(temp_buf);
				}
				else {
					fld_buf.Colon();
				}
				p_chunk = curl_slist_append(p_chunk, fld_buf.cptr());
                flds_count++;
            }
		}
	}
	return p_chunk;
}

int ScURL::HttpPatch(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const char * pBody, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	p_chunk = (struct curl_slist *)ComposeHeaderList(pHttpHeaderFields);
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "PATCH")));
	THROW(SetCommonOptions(mflags, 0, 0))
	if(pBody) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, sstrlen(pBody))));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, pBody)));
	}
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpPatch(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const StrStrAssocArray * pFields, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	uint   flds_count = 0;
	SString flds_buf;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	p_chunk = (struct curl_slist *)ComposeHeaderList(pHttpHeaderFields);
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	ComposeFieldList(pFields, flds_buf, &flds_count);
	url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "PATCH")));
	THROW(SetCommonOptions(mflags, 0, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, flds_buf.Len())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, flds_buf.cptr())));
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpPost(const char * pUrl, int mflags, HttpForm & rForm, SFile * pReplyStream)
{
	int    ok = 1;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	THROW(SetCommonOptions(mflags, 0, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "POST"))); // @v12.2.6
	// @v12.2.6 THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_HTTPPOST, static_cast<struct curl_httppost *>(rForm.FH))));
	THROW(SetHttpFormOptions(rForm)); // @v12.2.6
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::HttpPost(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, HttpForm & rForm, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	InetUrl url_local(rUrl);
	InnerUrlInfo url_info;
	StrStrAssocArray local_hdr_flds;
	if(pHttpHeaderFields) {
		local_hdr_flds = *pHttpHeaderFields;
	}
	if(rForm.GetContentType().NotEmpty() && !SHttpProtocol::IsThereHeaderField(local_hdr_flds, SHttpProtocol::hdrContentType, 0)) {
		SHttpProtocol::SetHeaderField(local_hdr_flds, SHttpProtocol::hdrContentType, rForm.GetContentType());
	}
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	p_chunk = static_cast<struct curl_slist *>(ComposeHeaderList(&local_hdr_flds));
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	{
		url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 256 * 1024, 0))
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "POST"))); // @v12.2.6
		// @v12.2.6 THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_HTTPPOST, (struct curl_httppost *)rForm.FH))); 
		THROW(SetHttpFormOptions(rForm)); // @v12.2.6
		THROW(SetupCbWrite(pReplyStream));
		THROW(Execute());
	}
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpPut(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const char * pBody, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	InetUrl url_local(rUrl);
	InnerUrlInfo url_info;
	StrStrAssocArray local_hdr_flds;
	if(pHttpHeaderFields) {
		local_hdr_flds = *pHttpHeaderFields;
	}
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	p_chunk = static_cast<struct curl_slist *>(ComposeHeaderList(&local_hdr_flds));
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "PUT")));
	THROW(SetCommonOptions(mflags, 0, 0))
	if(pBody) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, sstrlen(pBody))));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, pBody)));
	}
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpPost(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const char * pBody, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	StrStrAssocArray local_hdr_flds;
	if(pHttpHeaderFields) {
		local_hdr_flds = *pHttpHeaderFields;
	}
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	p_chunk = static_cast<struct curl_slist *>(ComposeHeaderList(&local_hdr_flds));
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "POST")));
	THROW(SetCommonOptions(mflags, 0, 0))
	if(pBody) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, sstrlen(pBody))));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, pBody)));
	}
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpPost(const char * pUrlBuf, int mflags, const StrStrAssocArray * pFields, SFile * pReplyStream)
{
	int    ok = 1;
	uint   flds_count = 0;
	SString flds_buf;
	ComposeFieldList(pFields, flds_buf, &flds_count);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrlBuf)));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "POST"))); // @v12.2.6
	THROW(SetCommonOptions(mflags, 0, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, flds_buf.Len())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, flds_buf.cptr())));
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::HttpGet(const char * pUrlBuf, int mflags, SFile * pReplyStream)
{
	int    ok = 1;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrlBuf)));
	THROW(SetCommonOptions(mflags, 0, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "GET")));
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::HttpGet(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protHttp, url_info));
	{
		url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		THROW(SetCommonOptions(mflags, 0, 0))
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "GET")));
		THROW(SetupCbWrite(pReplyStream));
		p_chunk = (struct curl_slist *)ComposeHeaderList(pHttpHeaderFields);
		if(p_chunk)
			curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
		THROW(Execute());
	}
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpGet(const char * pUrlBuf, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream)
{
	int    ok = 1;
	uint   flds_count = 0;
	struct curl_slist * p_chunk = 0;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrlBuf)));
	THROW(SetCommonOptions(mflags, 0, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "GET")));
	THROW(SetupCbWrite(pReplyStream));
	p_chunk = (struct curl_slist *)ComposeHeaderList(pHttpHeaderFields);
	if(p_chunk)
		curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	THROW(Execute());
	CATCHZOK
	curl_slist_free_all(p_chunk);
	CleanCbRW();
	return ok;
}

int ScURL::HttpDelete(const char * pUrl, int flags, SFile * pReplyStream)
{
	/*
  hnd = curl_easy_init();
  curl_easy_setopt(hnd, CURLOPT_URL, "http://localhost:8080/opt/in/QueryPartner/12");
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.45.0");
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
	*/
	int    ok = 1;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "DELETE")));
	THROW(SetupCbWrite(pReplyStream));
	THROW(Execute());
	CATCHZOK
	CleanCbRW();
	return ok;
}

void ScURL::SetLogFileName(const char * pFileName)
{
    LogFileName = pFileName;
}

int ScURL::SetAuth(int auth, const char * pUser, const char * pPassword)
{
    int    ok = 1;
    SString temp_buf;
    THROW_S(auth == authServer, SLERR_INVPARAM);
    temp_buf.Cat(pUser).Colon().Cat(pPassword);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_USERPWD, temp_buf.cptr())));
    CATCHZOK
    return ok;
}

int ScURL::SetupDefaultSslOptions(const char * pCertFilePath, int sslVer /* SSystem::sslXXX */, const StringSet * pSsCipherList)
{
	int    ok = 1;
	const SGlobalSecureConfig & r_cfg = SLS.GetGlobalSecureConfig();
	SString ca_file;
	SString ca_path;
	if(isempty(pCertFilePath)) {
		SFsPath::NormalizePath(r_cfg.CaFile, SFsPath::npfSlash, ca_file);
		SFsPath::NormalizePath(r_cfg.CaPath, SFsPath::npfSlash, ca_path);
	}
	else {
		SString temp_buf;
		const SFsPath ps(pCertFilePath);
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
		temp_buf.RmvLastSlash();
		SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash, ca_path);
		ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash, ca_file);
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CAINFO, ca_file.cptr())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CAPATH, ca_path.cptr())));
	{
		int    curl_ssl_ver = CURL_SSLVERSION_DEFAULT;
		switch(sslVer) {
			case SSystem::sslTLS_v1x: curl_ssl_ver = CURL_SSLVERSION_TLSv1; break;
			case SSystem::sslSSL_v2:  curl_ssl_ver = CURL_SSLVERSION_SSLv2; break;
			case SSystem::sslSSL_v3:  curl_ssl_ver = CURL_SSLVERSION_SSLv3; break;
			case SSystem::sslTLS_v10: curl_ssl_ver = CURL_SSLVERSION_TLSv1_0; break;
			case SSystem::sslTLS_v11: curl_ssl_ver = CURL_SSLVERSION_TLSv1_1; break;
			case SSystem::sslTLS_v12: curl_ssl_ver = CURL_SSLVERSION_TLSv1_2; break;
			case SSystem::sslTLS_v13: curl_ssl_ver = CURL_SSLVERSION_TLSv1_3; break;
		}
		if(curl_ssl_ver) {
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSLVERSION, curl_ssl_ver)));
		}
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 1L)));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYHOST, /*2L*/0L)));
	if(pSsCipherList) {
		/*SString temp_buf;
		for(uint ssp = 0; pSsCipherList->get(&ssp, temp_buf);) {
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_CIPHER_LIST, 2L)));
		}*/
	}
	CATCHZOK
	return ok;
}

int ParseFtpDirEntryLine(const SString & rLine, SFileEntryPool::Entry & rEntry)
{
	/*
		drwxr-xr-x   1 ftp      ftp             0 Aug 11  2015 Altarix
		drwxr-xr-x   1 ftp      ftp             0 May 15 11:54 ANONYMOUS
		drwxr-xr-x   1 ftp      ftp             0 Sep 22  2013 DataExch
		drwxr-xr-x   1 ftp      ftp             0 May 15 11:43 DISTRIB
		drwxr-xr-x   1 ftp      ftp             0 May 12  2014 frontol
		drwxr-xr-x   1 ftp      ftp             0 Aug 07 16:49 test

		-rw-r--r--   1 ftp      ftp             0 Aug 07 16:51 file-ascii.txt
		-rw-r--r--   1 ftp      ftp             0 Aug 07 16:52 file-cp1251-русские буквы.txt
	*/
	int    ok = 1;
	SString temp_buf;
	SStrScan scan(rLine);
	{ // attributes
		scan.Skip().GetWord(" \t", temp_buf.Z());
		THROW(temp_buf.Len() == 10);
		if(temp_buf[0] == 'd')
			rEntry.Attr |= SFile::attrSubdir;
		if(temp_buf[1] == 'r')
			rEntry.Attr |= SFile::attrOwpR;
		if(temp_buf[2] == 'w')
			rEntry.Attr |= SFile::attrOwpW;
		if(temp_buf[3] == 'x')
			rEntry.Attr |= SFile::attrOwpX;
		if(temp_buf[4] == 'r')
			rEntry.Attr |= SFile::attrGrpR;
		if(temp_buf[5] == 'w')
			rEntry.Attr |= SFile::attrGrpW;
		if(temp_buf[6] == 'x')
			rEntry.Attr |= SFile::attrGrpX;
		if(temp_buf[7] == 'r')
			rEntry.Attr |= SFile::attrUsrR;
		if(temp_buf[8] == 'w')
			rEntry.Attr |= SFile::attrUsrW;
		if(temp_buf[9] == 'x')
			rEntry.Attr |= SFile::attrUsrX;
	}
	// ? number
	scan.Skip().GetWord(" \t", temp_buf.Z());
	// owner
	scan.Skip().GetWord(" \t", temp_buf.Z());
	// group
	scan.Skip().GetWord(" \t", temp_buf.Z());
	// size
	scan.Skip().GetWord(" \t", temp_buf.Z());
	rEntry.Size = temp_buf.ToInt64();
	{ // time: mon day [year] time
		const LDATETIME now_dtm = getcurdatetime_();
		int    mon = 0;
		int    day = 0;
		int    year = 0;
		LDATETIME dtm;
		//LTIME  t = ZEROTIME;
		{ // mon
			scan.Skip().GetWord(" \t", temp_buf.Z());
			int mi = STextConst::GetIdx(STextConst::cMon_En_Sh, temp_buf);
			if(mi >= 0 && mi <= 11)
				mon = (mi+1);
		}
		{ // day
			scan.Skip().GetWord(" \t", temp_buf.Z());
			day = temp_buf.ToLong();
			if(day < 1 || day > 31)
				day = 0;
		}
		{ // year or time
			scan.Skip().GetWord(" \t", temp_buf.Z());
			if(temp_buf.HasChr(':')) {
				strtotime(temp_buf, TIMF_HMS, &dtm.t);
			}
			else {
				year = temp_buf.ToLong();
				if(year < 1990 && year > 2100)
					year = 0;
			}
		}
		SETIFZ(day, now_dtm.d.day());
		SETIFZ(mon, now_dtm.d.month());
		SETIFZ(year, now_dtm.d.year());
		dtm.d.encode(day, mon, year);
		rEntry.ModTm_ = SUniTime_Internal::EpochToNs100(dtm.GetTimeT());
	}
	{ // name
		scan.Skip().GetWord("" /*unitl EOL*/, temp_buf.Z());
		rEntry.Name = temp_buf;
	}
	CATCHZOK
	return ok;
}

int ScURL::PrepareURL(InetUrl & rUrl, int defaultProt, ScURL::InnerUrlInfo & rInfo)
{
	int    ok = 1;
	int    prot = 0;
	SString temp_buf;
	rUrl.GetComponent(InetUrl::cUserName, 1/*urlDecode*/, rInfo.User);
	rUrl.GetComponent(InetUrl::cPassword, 1/*urlDecode*/, rInfo.Password);
	if(rInfo.User.NotEmpty()) {
		THROW(SetAuth(authServer, rInfo.User, rInfo.Password));
	}
	{
		if(rUrl.GetComponent(InetUrl::cScheme, 0, temp_buf)) {
			prot = rUrl.GetSchemeId(temp_buf);
			rUrl.SetProtocol(prot);
		}
		else {
			prot = rUrl.GetProtocol();
			if(prot)
				rUrl.SetComponent(InetUrl::cScheme, InetUrl::GetSchemeMnem(prot));
		}
		if(prot == 0 && defaultProt != 0) {
			prot = defaultProt/*InetUrl::protFtp*/;
			rUrl.SetProtocol(prot);
			rUrl.SetComponent(InetUrl::cScheme, InetUrl::GetSchemeMnem(prot));
		}
		switch(defaultProt) {
			case InetUrl::protFtp: THROW(oneof3(prot, InetUrl::protFtp, InetUrl::protFtps, InetUrl::protTFtp)); break;
			case InetUrl::protHttp: THROW(oneof2(prot, InetUrl::protHttp, InetUrl::protHttps)); break;
			case InetUrl::protPOP3: THROW(oneof2(prot, InetUrl::protPOP3, InetUrl::protPOP3S)); break;
			case InetUrl::protSMTP: THROW(oneof2(prot, InetUrl::protSMTP, InetUrl::protSMTPS)); break;
		}
	}
	{
		long   npf = SFsPath::npfSlash;
		if(oneof4(prot, InetUrl::protHttp, InetUrl::protHttps, InetUrl::protFtp, InetUrl::protFtps))
			npf |= SFsPath::npfKeepCase;
		rUrl.GetComponent(InetUrl::cPath, 0, temp_buf);
		SFsPath::NormalizePath(temp_buf, npf, rInfo.Path);
		if(temp_buf != rInfo.Path)
			rUrl.SetComponent(InetUrl::cPath, rInfo.Path);
	}
	CATCHZOK
	return ok;
}

int ScURL::FtpList(const InetUrl & rUrl, int mflags, SFileEntryPool & rPool)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	{
		if(url_info.Path.NotEmptyS()) {
			url_info.Path.SetLastDSlash();
			url_local.SetComponent(InetUrl::cPath, url_info.Path);
		}
		url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 256 * 1024, 0))
	{
		SBuffer reply_buf;
		SFile reply_stream(reply_buf, SFile::mWrite);
		THROW(SetupCbWrite(&reply_stream));
		THROW(Execute());
		{
			SString line_buf;
			SBuffer * p_result_buf = static_cast<SBuffer *>(reply_stream);
			SFileEntryPool::Entry entry;
			if(p_result_buf) {
				while(p_result_buf->ReadTermStr("\x0D\x0A", line_buf)) {
					(temp_buf = line_buf).Chomp();
					ParseFtpDirEntryLine(temp_buf, entry.Z());
					if(url_info.Path.NotEmpty())
						entry.Path = url_info.Path;
					rPool.Add(entry, 0);
				}
				temp_buf = ""; // @debug
			}
		}
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::FtpPut(const InetUrl & rUrl, int mflags, const char * pLocalFile, const char * pDestFileName, SDataMoveProgressInfo * pProgress)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(fileExists(pLocalFile));
	curl_easy_reset(_CURLH);
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	{
		if(!isempty(pDestFileName)) {
			(temp_buf = pDestFileName).Strip();
			if(oneof2(temp_buf.C(0), '/', '\\'))
				temp_buf.ShiftLeft();
		}
		else {
			SFsPath ps(pLocalFile);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
			if(!temp_buf.IsLegalUtf8()) // @v11.8.5
				temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
		}
		url_info.Path.SetLastDSlash().Cat(temp_buf);
		url_local.SetComponent(InetUrl::cPath, url_info.Path);
		//THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_TRANSFERTEXT, 1)));
		{
			SString url_buf;
			url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), url_buf);
			//url_buf.EncodeUrl(temp_buf.cptr(), 0);
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, url_buf.cptr())));
		}
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 256 * 1024, 0))
	{
		SBuffer reply_buf;
		SFile reply_stream(reply_buf, SFile::mWrite);
		SFile f_in(pLocalFile, SFile::mRead|SFile::mBinary);
		THROW(SetupCbRead(&f_in));
		THROW(SetupCbWrite(&reply_stream));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_UPLOAD, 1L)));
		THROW(Execute());
		{
			SString line_buf;
			SBuffer * p_result_buf = static_cast<SBuffer *>(reply_stream);
			SFileEntryPool::Entry entry;
			if(p_result_buf) {
				while(p_result_buf->ReadTermStr("\x0D\x0A", line_buf)) {
					(temp_buf = line_buf).Chomp();
				}
				temp_buf = ""; // @debug
			}
		}
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::FtpGet(const InetUrl & rUrl, int mflags, const char * pLocalFile, SString * pResultFileName, SDataMoveProgressInfo * pProgress)
{
	int    ok = 1;
	SString temp_buf;
	SString local_file_path(pLocalFile);
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	{
		SFsPath ps_local(pLocalFile);
		SFsPath ps_remote(url_info.Path);
		if(ps_remote.Nam.NotEmpty()) {
			if(ps_local.Nam.IsEmpty()) {
				ps_local.Nam = ps_remote.Nam;
				ps_local.Ext = ps_remote.Ext;
				ps_local.Merge(local_file_path);
			}
		}
		{
			url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		}
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 256 * 1024, 0))
	{
		SFile f_out(local_file_path, SFile::mWrite|SFile::mBinary);
		THROW(SetupCbWrite(&f_out));
		THROW(Execute());
		ASSIGN_PTR(pResultFileName, local_file_path);
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::FtpDelete(const InetUrl & rUrl, int mflags)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	SString file_name_to_delete;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	{
		file_name_to_delete = url_info.Path;
		THROW(file_name_to_delete.NotEmptyS());
		{
			url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword|InetUrl::stPath), temp_buf); // @notebene InetUrl::stPath
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		}
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 0, 0))
	{
		temp_buf.Z().Cat("DELE").Space().Cat(file_name_to_delete);
		p_chunk = curl_slist_append(p_chunk, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH,  CURLOPT_QUOTE, p_chunk)));
		THROW(Execute());
	}
	CATCHZOK
	curl_slist_free_all(p_chunk);
	return ok;
}

int ScURL::FtpDeleteDir(const InetUrl & rUrl, int mflags)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	SString file_name_to_delete;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	{
		file_name_to_delete = url_info.Path;
		THROW(file_name_to_delete.NotEmptyS());
		{
			url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword|InetUrl::stPath), temp_buf); // @notebene InetUrl::stPath
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		}
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 0, 0))
	{
		temp_buf.Z().Cat("RMD").Space().Cat(file_name_to_delete);
		p_chunk = curl_slist_append(p_chunk, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH,  CURLOPT_QUOTE, p_chunk)));
		THROW(Execute());
	}
	CATCHZOK
	curl_slist_free_all(p_chunk);
	return ok;
}

int ScURL::FtpChangeDir(const InetUrl & rUrl, int mflags)
{
	int    ok = 1;
	struct curl_slist * p_chunk = 0;
	SString temp_buf;
	SString path_to_cwd;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	CleanCbRW();
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	url_local.GetComponent(InetUrl::stPath, 0, path_to_cwd);
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 0, 0))
	{
		url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword|InetUrl::stPath), temp_buf); // @notebene InetUrl::stPath
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	{
		int    to_do = 0;
		path_to_cwd.ShiftLeftChr('/');
		path_to_cwd.RmvLastSlash();
		SString new_cmd_buf;
		if(path_to_cwd.NotEmptyS()) {
			StringSet ss_cwd('/', path_to_cwd);
			for(uint ssp = 0; ss_cwd.get(&ssp, temp_buf);) {
				new_cmd_buf.Z().Cat("CWD").Space().Cat(temp_buf.Strip());
				p_chunk = curl_slist_append(p_chunk, new_cmd_buf.cptr());
				to_do = 1;
			}
		}
		else {
			new_cmd_buf.Z().Cat("CWD").Space().Cat("/");
			p_chunk = curl_slist_append(p_chunk, new_cmd_buf.cptr());
			to_do = 1;
		}
		if(to_do) {
			THROW(SetError(curl_easy_setopt(_CURLH,  CURLOPT_QUOTE, p_chunk)));
			THROW(Execute());
		}
	}
	CATCHZOK
	curl_slist_free_all(p_chunk);
	return ok;
}

int ScURL::FtpCreateDir(const InetUrl & rUrl, int mflags)
{
	int    ok = 1;
	SString temp_buf;
	SString path_to_cwd;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	curl_easy_reset(_CURLH);
	CleanCbRW();
	THROW(PrepareURL(url_local, InetUrl::protFtp, url_info));
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 0, 0));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_FTP_CREATE_MISSING_DIRS, 1)));
	{
		url_local.Compose(InetUrl::stAll & ~(InetUrl::stUserName|InetUrl::stPassword), temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(Execute());
	CATCHZOK
	return ok;
}

int ScURL::Pop3List(const InetUrl & rUrl, int mflags, LAssocArray & rList) // LIST
{
	rList.clear();
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protPOP3, url_info));
	{
		url_local.SetComponent(InetUrl::cPath, 0);
		url_local.Compose(InetUrl::stScheme|InetUrl::stHost|InetUrl::stPort, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
		// @v11.3.8 @develop {
		/* (doesn't work) if(temp_buf.CmpSuffix(".mail.ru", 1) == 0) {
			if(url_info.Password.NotEmpty()) {
				THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SASL_AUTHZID, url_info.Password.cptr())));
			}
		}*/
		// } @v11.3.8
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 1024, 0))
	//THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "LIST"))); // @v9.8.11
	{
		SBuffer reply_buf;
		SFile reply_stream(reply_buf, SFile::mWrite);
		THROW(SetupCbWrite(&reply_stream));
		THROW(Execute());
		{
			SString line_buf;
			SBuffer * p_result_buf = (SBuffer *)reply_stream;
			if(p_result_buf) {
				SString left, right;
				while(p_result_buf->ReadLine(line_buf)) {
					line_buf.Chomp().Strip();
					if(line_buf.Divide(' ', left, right) > 0) {
						long   n = left.Strip().ToLong();
						long   s = right.Strip().ToLong();
						rList.Add(n, s, 0);
					}
				}
			}
		}
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::Pop3Top(const InetUrl & rUrl, int mflags, uint msgN, uint maxLines, SMailMessage & rMsg)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protPOP3, url_info));
	{
		url_local.Compose(InetUrl::stScheme|InetUrl::stHost|InetUrl::stPort, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 1024, 0))
	temp_buf.Z().Cat("TOP").Space().Cat(msgN).Space().Cat((maxLines > 1000) ? 0 : maxLines);
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, temp_buf.cptr())));
	{
		SBuffer reply_buf;
		SFile reply_stream(reply_buf, SFile::mWrite);
		THROW(SetupCbWrite(&reply_stream));
		THROW(Execute());
		{
			SBuffer * p_result_buf = (SBuffer *)reply_stream;
			if(p_result_buf) {
				ok = rMsg.ReadFromFile(reply_stream);
			}
		}
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::Pop3Get(const InetUrl & rUrl, int mflags, uint msgN, SMailMessage & rMsg, SDataMoveProgressInfo * pProgress)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protPOP3, url_info));
	{
		url_local.SetComponent(InetUrl::cPath, temp_buf.Z().Cat(msgN));
		url_local.Compose(InetUrl::stScheme|InetUrl::stHost|InetUrl::stPort|InetUrl::stPath, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 1024, 0));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, 0))); // По умолчанию cURL применяет retr.
	{
		SBuffer reply_buf;
		SFile reply_stream(reply_buf, SFile::mWrite);
		THROW(SetupCbWrite(&reply_stream));
		SetupCbProgress(pProgress);
		THROW(Execute());
		{
			SString line_buf;
			SBuffer * p_result_buf = (SBuffer *)reply_stream;
			if(p_result_buf) {
				ok = rMsg.ReadFromFile(reply_stream);
			}
		}
	}
	CATCHZOK
	CleanCbRW();
	return ok;
}

int ScURL::Pop3Delete(const InetUrl & rUrl, int mflags, uint msgN)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	THROW(PrepareURL(url_local, InetUrl::protPOP3, url_info));
	{
		url_local.SetComponent(InetUrl::cPath, temp_buf.Z().Cat(msgN));
		url_local.Compose(InetUrl::stScheme|InetUrl::stHost|InetUrl::stPort|InetUrl::stPath, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 1024, 0))
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "DELE"))); // Номер сообщения в пути (see above)
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_NOBODY, 1L)));
	THROW(Execute());
	CATCHZOK
	return ok;
}

struct CbRead_EMailMessage_Block {
	CbRead_EMailMessage_Block(const SMailMessage & rMsg) : Wb(rMsg)
	{
	}
	SMailMessage::WriterBlock Wb;
	SBuffer TBuf;
};

static size_t CbRead_EMailMessage(char * pBuffer, size_t size, size_t nitems, void * pExtra)
{
	size_t written_sz = 0;
	const size_t outer_buf_sz = (size * nitems);
	if(outer_buf_sz != 0) {
		CbRead_EMailMessage_Block * p_blk = (CbRead_EMailMessage_Block *)pExtra;
		size_t avl_sz = p_blk->TBuf.GetAvailableSize();
		int    r = 1;
		while(r > 0 && written_sz < outer_buf_sz) {
			size_t s = MIN(avl_sz, (outer_buf_sz-written_sz));
			if(s) {
				p_blk->TBuf.Read(pBuffer + written_sz, s);
				written_sz += s;
			}
			else {
				p_blk->TBuf.Z();
				r = p_blk->Wb.Read(4096, p_blk->TBuf);
			}
			avl_sz = p_blk->TBuf.GetAvailableSize();
		}
	}
	return written_sz;
}

int ScURL::SmtpSend(const InetUrl & rUrl, int mflags, const SMailMessage & rMsg)
{
	int    ok = 1;
	SString temp_buf;
	SString addr_buf;
	SString from_buf;
	InetUrl url_local = rUrl;
	InnerUrlInfo url_info;
	struct curl_slist * p_recipients = 0;
	{
		THROW(PrepareURL(url_local, InetUrl::protSMTP, url_info));
		url_local.Compose(InetUrl::stScheme|InetUrl::stHost|InetUrl::stPort|InetUrl::stPath, temp_buf);
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_USERNAME, url_info.User.cptr())));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_PASSWORD, url_info.Password.cptr())));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, temp_buf.cptr())));
	}
	THROW(SetCommonOptions(mflags|mfTcpKeepAlive, 1024, 0));
	{
		CbRead_EMailMessage_Block rd_blk(rMsg);
		{
			StringSet ss;
			rMsg.GetField(SMailMessage::fldFrom, temp_buf);
			if(temp_buf.NotEmptyS()) {
				ss.Z();
				if(rMsg.PreprocessEmailAddrString(temp_buf, addr_buf, &ss)) {
					uint ssp = 0;
					if(ss.get(&ssp, from_buf)) {
						THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_MAIL_FROM, from_buf.cptr())));
					}
				}
			}
			rMsg.GetField(SMailMessage::fldTo, temp_buf);
			if(temp_buf.NotEmptyS()) {
				ss.Z();
				if(rMsg.PreprocessEmailAddrString(temp_buf, addr_buf, &ss)) {
					for(uint ssp = 0; ss.get(&ssp, temp_buf);)
						p_recipients = curl_slist_append(p_recipients, temp_buf.cptr());
				}
			}
			rMsg.GetField(SMailMessage::fldCc, temp_buf);
			if(temp_buf.NotEmptyS()) {
				ss.Z();
				if(rMsg.PreprocessEmailAddrString(temp_buf, addr_buf, &ss)) {
					for(uint ssp = 0; ss.get(&ssp, temp_buf);)
						p_recipients = curl_slist_append(p_recipients, temp_buf.cptr());
				}
			}
			THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_MAIL_RCPT, p_recipients)));
		}
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_TRY)))); // @v9.9.1 CURLUSESSL_ALL-->CURLUSESSL_TRY
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_READFUNCTION, CbRead_EMailMessage)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_READDATA, &rd_blk)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_UPLOAD, 1L)));
		THROW(Execute());
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_STDERR, 0)));
	}
	CATCHZOK
	curl_slist_free_all(p_recipients);
	return ok;
}
//
//
//
SUniformFileTransmParam::ResultItem::ResultItem()
{
	THISZERO();
}

SUniformFileTransmParam::SUniformFileTransmParam() : Flags(0), Format(SFileFormat::Unkn), Pop3TopMaxLines(0)
{
}

int SUniformFileTransmParam::Run(SDataMoveProgressProc pf, void * extraPtr)
{
	Reply.Z();

	/*
#define SLERR_UFT_SRCISEMPTY                   581 // @v11.8.10 SUniformFileTransmParam: src is empty 
#define SLERR_UFT_DESTISEMPTY                  582 // @v11.8.10 SUniformFileTransmParam: dest is empty
#define SLERR_UFT_SRCORDESTMUSTBEFILE          583 // @v11.8.10 SUniformFileTransmParam: src or dest must be file

#define SLERR_UFT_INVSRCPROT                   584 // @v11.8.10 SUniformFileTransmParam: invalid src protocol
#define SLERR_UFT_INVDESTPROT                  585 // @v11.8.10 SUniformFileTransmParam: invalid dest protocol
	*/
	int    ok = -1;
    SString path_src = SrcPath;
    SString path_dest = DestPath;
    THROW_S(path_src.NotEmptyS(), SLERR_UFT_SRCISEMPTY);
    THROW_S(path_dest.NotEmptyS(), SLERR_UFT_DESTISEMPTY);
    {
		SString temp_buf;
		SString temp_fname;
		SFsPath ps;
		InetUrl url_src;
		InetUrl url_dest;
		THROW(url_src.Parse(path_src) > 0);
		THROW(url_dest.Parse(path_dest) > 0);
        const int prot_src = url_src.GetProtocol();
        const int prot_dest = url_dest.GetProtocol();
        THROW_S(oneof8(prot_src, InetUrl::protFile, InetUrl::protFtp, InetUrl::protFtps, 
			InetUrl::protTFtp, InetUrl::protHttp, InetUrl::protHttps, InetUrl::protPOP3, InetUrl::protPOP3S), SLERR_UFT_INVSRCPROT);
        THROW_S(oneof8(prot_dest, InetUrl::protFile, InetUrl::protFtp, InetUrl::protFtps, 
			InetUrl::protTFtp, InetUrl::protHttp, InetUrl::protHttps, InetUrl::protSMTP, InetUrl::protSMTPS), SLERR_UFT_INVDESTPROT);
		THROW_S(prot_src == InetUrl::protFile || prot_dest == InetUrl::protFile, SLERR_UFT_SRCORDESTMUSTBEFILE);
		if(prot_src == InetUrl::protFile) { // Отправка локального файла
			SString local_path_src;
			url_src.GetComponent(InetUrl::cPath, 0, local_path_src);
			ResultItem ri;
            THROW(fileExists(local_path_src));
			{
				SFile::Stat fs;
				SFile::GetStat(local_path_src, 0, &fs, 0);
				ri.Size = fs.Size;
				AddS(local_path_src, &ri.SrcPathP);
				AddS(local_path_src, &ri.RmvSrcCookieP);
			}
			if(prot_dest == InetUrl::protFile) { // @v12.2.5
				SFsPath ps_dest(path_dest);
				SFsPath ps_src(local_path_src);
				ps_dest.Nam = ps_src.Nam;
				ps_dest.Ext = ps_src.Ext;
				SString local_path_dest;
				ps_dest.Merge(&ps_src, SFsPath::fNam|SFsPath::fExt, local_path_dest);
				SCopyFile(local_path_src, local_path_dest, /*SDataMoveProgressProc*/0, /*shareMode*/0, /*pExtra*/0);
			}
			else if(oneof3(prot_dest, InetUrl::protFtp, InetUrl::protFtps, InetUrl::protTFtp)) {
				ScURL curl;
                if(AccsName.NotEmpty()) {
                    url_dest.SetComponent(InetUrl::cUserName, AccsName);
                    url_dest.SetComponent(InetUrl::cPassword, AccsPassword);
                }
				THROW(curl.FtpPut(url_dest, ScURL::mfVerbose, local_path_src, 0, 0));
				{
					ResultList.insert(&ri);
				}
			}
			else if(oneof2(prot_dest, InetUrl::protHttp, InetUrl::protHttps)) {
				ScURL curl;
				SBuffer ack_buf;
				SFile wr_stream(ack_buf, SFile::mWrite);
				ScURL::HttpForm hf(curl);
				{
					SFileFormat::GetMime(Format, temp_buf);
					ps.Split(local_path_src);
					ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_fname);
					hf.AddContentFile(local_path_src, temp_buf, temp_fname);
				}
				THROW(curl.HttpPost(url_dest, ScURL::mfVerbose|ScURL::mfDontVerifySslPeer, 0, hf, &wr_stream));
				{
					SBuffer * p_ret_buf = static_cast<SBuffer *>(wr_stream);
					if(p_ret_buf) {
						size_t ret_size = p_ret_buf->GetAvailableSize();
						if(ret_size) {
							Reply.CatN(p_ret_buf->GetBufC(), ret_size);
							{
								ResultList.insert(&ri);
							}
						}
					}
				}
			}
			else if(oneof2(prot_dest, InetUrl::protSMTP, InetUrl::protSMTPS)) {
				// mailto:abc@abc.com?subject=topic
				// smtp://yandex.ru?subject=topic&from=x@yandex.ru
				ScURL curl;
				SString _from;
				SString _to;
				SString _subj;
				url_src.GetQueryParam("from", 1, _from);
				url_src.GetQueryParam("to", 1, _to);
				url_src.GetQueryParam("subject", 1, _subj);
                if(AccsName.NotEmpty()) {
                    url_dest.SetComponent(InetUrl::cUserName, AccsName);
                    url_dest.SetComponent(InetUrl::cPassword, AccsPassword);
                }
			}
		}
		else if(prot_dest == InetUrl::protFile) { // Получение файла в локальный каталог
			SString local_path_dest;
			SString filt_filename;
			url_dest.GetComponent(InetUrl::cPath, 0, local_path_dest);
			if(oneof3(prot_src, InetUrl::protFtp, InetUrl::protFtps, InetUrl::protTFtp)) {
				ScURL curl;
                if(AccsName.NotEmpty()) {
                    url_src.SetComponent(InetUrl::cUserName, AccsName);
                    url_src.SetComponent(InetUrl::cPassword, AccsPassword);
                }
				{
					int   has_wildcard = 0;
					ResultItem ri;
					url_src.GetComponent(InetUrl::cPath, 1, temp_buf);
					ps.Split(temp_buf);
					if(ps.Nam.IsEmpty()) {
						has_wildcard = 1;
						ps.Nam = "*";
					}
					else if(ps.Nam.HasChr('*') || ps.Nam.HasChr('?'))
						has_wildcard = 1;
					if(ps.Ext.IsEmpty()) {
						has_wildcard = 1;
						ps.Ext = "*";
					}
					else if(ps.Ext.HasChr('*') || ps.Ext.HasChr('?'))
						has_wildcard = 1;
					if(has_wildcard) {
						(filt_filename = ps.Nam).Dot().Cat(ps.Ext);
						SFsPath ps_temp;
						SFileEntryPool fep;
						SFileEntryPool::Entry fe;
						ps.Merge(~(SFsPath::fNam|SFsPath::fExt), temp_buf);
						url_src.SetComponent(InetUrl::cPath, temp_buf);
						THROW(curl.FtpList(url_src, ScURL::mfVerbose, fep));
						for(uint i = 0; i < fep.GetCount(); i++) {
							if(fep.Get(i, &fe, 0) && SFile::WildcardMatch(filt_filename, fe.Name)) {
								ps.Nam = fe.Name;
								ps.Merge(~SFsPath::fExt, temp_buf);
								url_src.SetComponent(InetUrl::cPath, temp_buf);
								ps_temp.Split(local_path_dest);
								ps_temp.Nam = fe.Name;
								ps_temp.Merge(~SFsPath::fExt, local_path_dest);
								if(curl.FtpGet(url_src, 0, local_path_dest, &temp_buf, 0)) {
									AddS(temp_buf, &ri.DestPathP);
									url_src.Compose(0, temp_buf);
									AddS(temp_buf, &ri.SrcPathP);
									ResultList.insert(&ri);
								}
							}
						}
					}
					else {
						THROW(curl.FtpGet(url_src, ScURL::mfVerbose, local_path_dest, &temp_buf, 0));
						AddS(temp_buf, &ri.DestPathP);
						url_src.Compose(0, temp_buf);
						AddS(temp_buf, &ri.SrcPathP);
						ResultList.insert(&ri);
					}
				}
			}
			else if(oneof2(prot_src, InetUrl::protHttp, InetUrl::protHttps)) {
				ScURL curl;
                if(AccsName.NotEmpty()) {
                    url_src.SetComponent(InetUrl::cUserName, AccsName);
                    url_src.SetComponent(InetUrl::cPassword, AccsPassword);
                }
				// @v11.0.9 {
				{
					SFsPath ps_dest(local_path_dest);
					if(ps_dest.Nam.IsEmpty()) {
						url_src.GetComponent(InetUrl::cPath, 1, temp_buf);
						SFsPath ps_src(temp_buf);
						if(ps_src.Nam.NotEmptyS()) {
							ps_dest.Nam = ps_src.Nam;
							ps_dest.Ext = ps_src.Ext;
							ps_dest.Merge(local_path_dest);
						}
					}
				}
				// } @v11.0.9
				SFile wr_stream(local_path_dest, SFile::mWrite|SFile::mBinary);
				THROW(wr_stream.IsValid());
				THROW(curl.HttpGet(url_src, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, 0, &wr_stream));
				{
					ResultItem ri;
					AddS(local_path_dest, &ri.DestPathP);
					url_src.Compose(0, temp_buf);
					AddS(temp_buf, &ri.SrcPathP);
					ResultList.insert(&ri);
				}
				ok = 1;
			}
			else if(oneof2(prot_src, InetUrl::protPOP3, InetUrl::protPOP3S)) {
				// mailfrom:abc@abc.com/*.xml?subject=topic
				// pop3://yandex.ru/*.xml?subject=topic
				ScURL curl;
				SString filt_from;
				SString filt_subj;
				SString filt_subjsub;
				url_src.GetQueryParam("from", 1, filt_from);
				url_src.GetQueryParam("subject", 1, filt_subj);
				url_src.GetQueryParam("subjsub", 1, filt_subjsub);
				url_src.GetQueryParam("wildcard", 1, filt_filename);
				{
					url_src.GetComponent(InetUrl::cPath, 1, temp_buf);
					if(temp_buf.NotEmptyS()) {
						ps.Split(temp_buf);
						if(filt_filename.IsEmpty()) {
							if(ps.Nam.NotEmpty())
								ps.Merge(SFsPath::fNam|SFsPath::fExt, filt_filename);
						}
						if(filt_from.IsEmpty()) {
							if(ps.Dir.NotEmpty()) {
								(temp_buf = ps.Dir).RmvLastSlash();
								STokenRecognizer tr;
								SNaturalTokenArray nta;
								uint   last_email_len = 0;
								for(uint i = 1; i <= temp_buf.Len(); i++) {
									nta.clear();
									tr.Run(temp_buf.ucptr(), (int)i, nta, 0);
									if(nta.Has(SNTOK_EMAIL) > 0.0f)
										last_email_len = i;
									else if(last_email_len)
										break;
								}
								if(last_email_len) {
									filt_from = temp_buf.Trim(last_email_len);
								}
							}
						}
					}
				}
                if(AccsName.NotEmpty()) {
                    url_src.SetComponent(InetUrl::cUserName, AccsName);
                    url_src.SetComponent(InetUrl::cPassword, AccsPassword);
                }
				{
					LAssocArray mail_list;
					SFsPath ps_src; // Структура имени исходного файла (в письме)
					THROW(curl.Pop3List(url_src, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, mail_list));
					{
						SString _progress_src;
						SString _progress_dest;
						SDataMoveProgressInfo scfd;
						SDataMoveProgressInfo * p_scfd = 0;
						if(pf) {
							scfd.Proc = pf;
							scfd.ExtraPtr = extraPtr;
							url_src.Compose(InetUrl::stPort|InetUrl::stHost, _progress_src);
							_progress_dest = local_path_dest;

							scfd.P_Src = _progress_src.cptr();
							scfd.P_Dest = _progress_dest.cptr();
							scfd.OverallItemsCount = mail_list.getCount();
							for(uint i = 0; i < mail_list.getCount(); i++)
								scfd.OverallSizeTotal += mail_list.at(i).Val;
							p_scfd = &scfd;
						}
						SMailMessage msg;
						SString result_file_name;
						SString result_file_ext;
						LongArray processed_msg_idx_list;
						for(uint i = 0; i < mail_list.getCount(); i++) {
							const long msg_idx = mail_list.at(i).Key;
							msg.Init();
							const int topr = curl.Pop3Top(url_src, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, msg_idx, Pop3TopMaxLines, msg);
							if(topr) {
								bool   do_get = true;
								do_get = do_get && (!filt_from.NotEmptyS() || msg.IsFrom(filt_from));
								do_get = do_get && (!filt_subj.NotEmptyS() || msg.IsSubj(filt_subj, 0));
								do_get = do_get && (!filt_subjsub.NotEmptyS() || msg.IsSubj(filt_subjsub, 1));
								msg.Init();
								if(do_get && curl.Pop3Get(url_src, ScURL::mfDontVerifySslPeer, msg_idx, msg, p_scfd)) {
									uint attc = msg.GetAttachmentCount();
									for(uint ai = 0; ai < attc; ai++) {
										const SMailMessage::Boundary * p_att = msg.GetAttachmentByIndex(ai);
										if(p_att) {
											ResultItem ri;
											msg.GetAttachmentFileName(p_att, temp_buf);
											ps_src.Split(temp_buf);
											//
											AddS(temp_buf, &ri.SrcPathP);
											msg.GetS(p_att->Ct.MimeP, temp_buf);
											if(temp_buf.NotEmptyS()) {
												AddS(temp_buf, &ri.SrcMimeP);
											}
											//
											if(ps_src.Nam.NotEmpty()) {
												ps_src.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
												if(filt_filename.IsEmpty() || SFile::WildcardMatch(filt_filename, temp_buf)) {
													ps.Split(local_path_dest);
													ps.Nam.SetIfEmpty(ps_src.Nam);
													ps.Ext.SetIfEmpty(ps_src.Ext);
													result_file_ext = ps.Ext;
													if(ps.Nam.IsEmpty()) {
														ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
														MakeTempFileName(temp_buf, "eml", result_file_ext, 0, result_file_name);
													}
													else {
														long   fec = 0;
														temp_buf = ps.Nam; // original file name
														do {
															if(fec > 0) {
																(ps.Nam = temp_buf).CatChar('-').CatLongZ(fec, 4);
															}
															fec++;
															ps.Merge(result_file_name);
														} while((Flags & fRenameExistantFiles) && fileExists(result_file_name));
													}
													if(msg.SaveAttachmentTo(ai, result_file_name, &temp_buf)) {
														result_file_name = temp_buf;
														SFile::Stat fs;
														SFile::GetStat(result_file_name, 0, &fs, 0);
														ri.Size = fs.Size;
														AddS(result_file_name, &ri.DestPathP);
														temp_buf.Z().Cat(msg_idx);
														AddS(temp_buf, &ri.RmvSrcCookieP);
														ResultList.insert(&ri);
														processed_msg_idx_list.add(msg_idx);
														ok = 1;
													}
												}
											}
										}
									}
								}
							}
							if(pf) {
								scfd.OverallItemsDone++;
								scfd.OverallSizeDone += mail_list.at(i).Val;
								pf(&scfd);
							}
						}
						if(Flags & fDeleteAfter && processed_msg_idx_list.getCount()) {
							processed_msg_idx_list.sortAndUndup();
							for(uint j = 0; j < processed_msg_idx_list.getCount(); j++) {
								const long msg_idx = processed_msg_idx_list.get(j);
								curl.Pop3Delete(url_src, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, msg_idx);
							}
						}
					}
				}
			}
		}
    }
    CATCHZOK
	return ok;
}
