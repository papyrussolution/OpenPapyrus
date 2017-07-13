// STCP.CPP
// Copyright (c) A.Sobolev 2005, 2007, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <uri.h>
#include <snet.h>
#include <winsock2.h>
#include <wininet.h>
// @v9.6.3 #include <idea.h>
#include <openssl/ssl.h>

// static
ulong SLAPI InetAddr::IPToULong(const char * pIP)
{
	ulong  addr = 0;
	SString buf;
	StringSet ss('.', pIP);
	THROW(ss.getCount() == 4);
	for(uint i = 0, j = 4; j > 0 && ss.get(&i, buf) > 0; j--) {
		ulong elem = buf.ToLong();
		THROW(elem >= 0 && elem <= 255);
		addr |= elem << 8 * (j - 1);
	}
	CATCH
		addr = 0;
	ENDCATCH
	return addr;
}

// static
int SLAPI InetAddr::ULongToIP(ulong ip, SString & rIP)
{
	rIP = 0;
	for(uint i = 4; i > 0; i--) {
		rIP.Cat((ip >> 8 * (i - 1)) & 0x000000FF);
		if(i != 1)
			rIP.Dot();
	}
	return 1;
}

SLAPI InetAddr::InetAddr()
{
	V4 = 0;
	Port = 0;
}

InetAddr & SLAPI InetAddr::Clear()
{
	V4 = 0;
	HostName = 0;
	Port = 0;
	return *this;
}

int FASTCALL InetAddr::IsEqual(const InetAddr & rS) const
{
	return BIN(V4 == rS.V4 && Port == rS.Port && HostName == rS.HostName);
}

int FASTCALL InetAddr::operator == (const InetAddr & rS) const
{
	return IsEqual(rS);
}

int FASTCALL InetAddr::operator != (const InetAddr & rS) const
{
	return BIN(!IsEqual(rS));
}

int SLAPI InetAddr::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, V4, rBuf));
	THROW(pSCtx->Serialize(dir, HostName, rBuf));
	THROW(pSCtx->Serialize(dir, Port, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI InetAddr::IsEmpty() const
{
	return BIN(V4 == 0 && HostName.Empty());
}

SString & SLAPI InetAddr::ToStr(long flags, SString & rBuf) const
{
	rBuf = 0;
	if(flags & fmtAddr || !(flags & (fmtAddr | fmtHost)))
		rBuf.Cat(V4 & 0xff).Dot().Cat((V4 >> 8) & 0xff).Dot().Cat((V4 >> 16) & 0xff).Dot().Cat((V4 >> 24) & 0xff);
	else if(flags & fmtHost)
		rBuf = HostName;
	if(!rBuf.NotEmptyS()) {
		if(HostName.NotEmpty())
			rBuf = HostName;
		else if(V4)
			rBuf.Cat(V4 & 0xff).Dot().Cat((V4 >> 8) & 0xff).Dot().Cat((V4 >> 16) & 0xff).Dot().Cat((V4 >> 24) & 0xff);
	}
	if(flags & fmtPort && Port > 0)
		rBuf.CatChar(':').Cat(Port);
	return rBuf;
}

int InetAddr::FromStr(const char * pStr)
{
	int    ok = 1;
	if(pStr) {
		SString buf = pStr;
		SString addr_buf, port_buf;
		buf.Divide(':', addr_buf, port_buf);
		if(addr_buf.NotEmptyS())
			ok = Set(addr_buf, port_buf.ToLong());
		else {
			Port = port_buf.ToLong();
			ok = -1;
		}
	}
	else {
		V4 = 0;
		HostName = 0;
		Port = 0;
		ok = -1;
	}
	return ok;
}

int SLAPI InetAddr::SetPort(int port)
{
	int    preserve_port = Port;
	Port = port;
	return preserve_port;
}

int SLAPI InetAddr::Set(const sockaddr_in * pAddr)
{
	return Set(pAddr->sin_addr.s_addr, pAddr->sin_port);
}

sockaddr * SLAPI InetAddr::Get(sockaddr_in * pAddr) const
{
	sockaddr_in addr;
	MEMSZERO(addr);
	addr.sin_family = AF_INET;
	addr.sin_port   = htons((ushort)Port);
	addr.sin_addr.s_addr = V4;
	ASSIGN_PTR(pAddr, addr);
	return (sockaddr *)pAddr;
}

int SLAPI InetAddr::Set(ulong addr, int port)
{
	V4 = addr;
	Port = port;
	return 1;
}

// static
int SLAPI InetAddr::GetNameByAddr(const char * pIP, SString & aHost)
{
	int    ok = -1;
	if(pIP) {
		char ip[16], host[64];
		STRNSCPY(ip, pIP);
		struct sockaddr_in sin;
		sin.sin_family           = AF_INET;
		sin.sin_port             = htons(0);
		sin.sin_addr.S_un.S_addr = inet_addr(ip);
		memzero(host, sizeof(host));
		if(getnameinfo((sockaddr*)&sin, sizeof(sin), host, sizeof(host), 0, 0, 0) == 0) {
			aHost = host;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI InetAddr::Set(const char * pHostName, int port)
{
	int    ok = 0;
	Port = port;
	if(pHostName) {
		/*
			typedef struct addrinfo {
				int ai_flags;
				int ai_family;
				int ai_socktype;
				int ai_protocol;
				size_t ai_addrlen;
				char* ai_canonname;
				struct sockaddr* ai_addr;
				struct addrinfo* ai_next;
			} addrinfo;
		 */
		addrinfo * p_ai = 0;
		addrinfo hint;
		MEMSZERO(hint);
		hint.ai_flags = AI_CANONNAME;
		hint.ai_family = PF_INET;
		hint.ai_protocol = 0;
		hint.ai_socktype = SOCK_STREAM;
		int    r = getaddrinfo(pHostName, 0, &hint, &p_ai);
		if(r == 0)
			for(addrinfo * p = p_ai; p != 0; p = p->ai_next) {
				HostName = p->ai_canonname;
				V4 = ((sockaddr_in *)p->ai_addr)->sin_addr.s_addr;
				ok = 1;
				break;
			}
		freeaddrinfo(p_ai);
		if(!ok) {
			SString added_msg;
			added_msg.Cat(pHostName);
			if(port)
				added_msg.CatDiv(':', 0).Cat(port);
			SLS.SetError(SLERR_SOCK_HOSTRESLVFAULT, added_msg);
		}
	}
	return ok;
}
//
//
//
// @v7.3.0 При большом размере буфера чтения не стабильно принимаются значительные по объему данные без терминатора
// и без информации о требуемом размере чтения (т.е. сколько-есть).
//
size_t TcpSocket::DefaultReadFrame  = 8192;
size_t TcpSocket::DefaultWriteFrame = 8192;

TcpSocket::SslBlock::SslBlock()
{
	State = 0;
	LastError = 0;
	SLS.InitSSL();
	P_Ctx = SSL_CTX_new(SSLv23_client_method());
	P_S = 0;
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
	THROW(p_ssl_s = SSL_new((SSL_CTX *)P_Ctx));
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
		SSL_shutdown((SSL *)P_S);
		SSL_free((SSL *)P_S);
		P_S = 0;
		ok = 1;
	}
	if(P_Ctx) {
		SSL_CTX_free((SSL_CTX *)P_Ctx);
		P_Ctx = 0;
		ok = 1;
	}
	return ok;
}

int TcpSocket::SslBlock::Accept()
{
	if(P_S)
		LastError = SSL_accept((SSL *)P_S);
	else
		LastError = -1;
	return LastError ? 0 : 1;
}

int TcpSocket::SslBlock::Select(int mode /* TcpSocket::mXXX */, int timeout, size_t * pAvailableSize)
{
	return BIN(P_S);
}

int TcpSocket::SslBlock::Read(void * pBuf, int bufLen)
{
	int    ret_len = P_S ? SSL_read((SSL *)P_S, pBuf, bufLen) : -1;
	return ret_len;
}

int TcpSocket::SslBlock::Write(const void * pBuf, int bufLen)
{
	int    ret = P_S ? SSL_write((SSL *)P_S, pBuf, bufLen) : -1;
	return ret;
}

SLAPI TcpSocket::TcpSocket(int timeout, int maxConn) : InBuf(DefaultReadFrame), OutBuf(DefaultWriteFrame)
{
	Timeout = timeout;
	MaxConn = maxConn;
	LastSockErr = 0;
	P_Ssl = 0;
	Reset();
}

SLAPI TcpSocket::~TcpSocket()
{
	Disconnect();
}

void SLAPI TcpSocket::Reset()
{
	ZDELETE(P_Ssl);
	S = INVALID_SOCKET;
	MEMSZERO(StatData);
}

int SLAPI TcpSocket::MoveToS(TcpSocket & rDest, int force /*=0*/)
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

int SLAPI TcpSocket::CopyS(TcpSocket & rSrc)
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

int SLAPI TcpSocket::IsValid() const
{
	return BIN(S != INVALID_SOCKET);
}

// private
int SLAPI TcpSocket::Init(SOCKET s)
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

int SLAPI TcpSocket::CheckErrorStatus()
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

int SLAPI TcpSocket::GetTimeout() const
{
	return Timeout;
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

int SLAPI TcpSocket::Connect(SslMode sslm, const InetAddr & rAddr)
{
	return Helper_Connect(sslm, rAddr);
}

int SLAPI TcpSocket::Connect(const InetAddr & rAddr)
{
	return Helper_Connect(sslmNone, rAddr);
}

int SLAPI TcpSocket::Helper_Connect(SslMode sslm, const InetAddr & rAddr)
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

int SLAPI TcpSocket::Bind(const InetAddr & rAddr)
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

int SLAPI TcpSocket::GetSockName(InetAddr * pAddr, int peer)
{
	int    ok = 1;
	struct sockaddr_in addr;
	int    addrlen = sizeof(addr);
	if(peer)
		ok = !getpeername(S, (struct sockaddr*)&addr, &addrlen);
	else
		ok = !getsockname(S, (struct sockaddr*)&addr, &addrlen);
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

int SLAPI TcpSocket::Disconnect()
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

int SLAPI TcpSocket::Listen()
{
	return ::listen(S, MaxConn) ? (SLibError = SLERR_SOCK_LISTEN, 0) : 1;
}

int SLAPI TcpSocket::Accept(TcpSocket * pCliSock, InetAddr * pCliAdr)
{
	int    ok = 1;
	struct sockaddr_in cli_addr;
	int    addr_size = sizeof(cli_addr);
	SOCKET cli_sock = ::accept(S, pCliAdr ? (sockaddr *)&cli_addr : 0, pCliAdr ? &addr_size : 0);
	if(cli_sock != INVALID_SOCKET) {
		pCliSock->Init(cli_sock);
		if(pCliAdr)
			pCliAdr->Set(&cli_addr);
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

int SLAPI TcpSocket::Select(int mode /* TcpSocket::mXXX */, int timeout, size_t * pAvailableSize)
{
	int    ok = 1;
	if(P_Ssl) {
		ok = P_Ssl->Select(mode, timeout, pAvailableSize);
	}
	else {
		size_t av_size = 0;
		fd_set set, * p_rset = 0, * p_wset = 0;
		FD_ZERO(&set);
		FD_SET(S, &set);
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
		if(mode == mRead)
			p_rset = &set;
		else
			p_wset = &set;
		int    r = ::select(S+1 /* ignored */, p_rset, p_wset, 0, p_tval);
		if(r == 0)
			ok = (SLibError = SLERR_SOCK_TIMEOUT, 0);
		else if(r == SOCKET_ERROR)
			ok = SLS.SetError(SLERR_SOCK_WINSOCK);
		else {
			av_size = (size_t)r; // !!!
			if(!FD_ISSET(S, &set)) {
				if(mode == mRead)
					ok = (SLibError = SLERR_SOCK_NONBLOCKINGRD, 0);
				else if(mode == mWrite)
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

int SLAPI TcpSocket::Recv(void * pBuf, size_t size, size_t * pRcvdSize)
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

int SLAPI TcpSocket::RecvUntil(SBuffer & rBuf, const char * pTerminator, size_t * pRcvdSize)
{
	int    ok = 1;
	const  size_t term_len = (pTerminator == 0) ? 0 : ((pTerminator[0] == 0) ? 1 : strlen(pTerminator));
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
					int    local_len = Helper_Recv((char *)InBuf, recv_sz);
					THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
					rd_len = (size_t)local_len;
					StatData.RdCount += local_len;
				}
				if(rd_len) {
					THROW(rBuf.Write((char *)InBuf, rd_len));
					total_sz += rd_len;
					assert(rBuf.GetWrOffs() >= term_len);
					const char * p_buf = (const char *)rBuf.GetBuf(rBuf.GetWrOffs()-term_len);
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

int SLAPI TcpSocket::RecvBuf(SBuffer & rBuf, size_t size, size_t * pRcvdSize)
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
				int    local_len = Helper_Recv((char *)InBuf, recv_sz);
				THROW_S(local_len != SOCKET_ERROR, SLERR_SOCK_WINSOCK);
				rd_len = (size_t)local_len;
				StatData.RdCount += local_len;
			}
			THROW(rBuf.Write((char *)InBuf, rd_len));
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

int SLAPI TcpSocket::RecvBlock(void * pBuf, size_t size, size_t * pRcvdSize)
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

int SLAPI TcpSocket::Send(const void * pBuf, size_t size, size_t * pSendedSize)
{
	int    ok = 1;
	int    len = 0;
	THROW(Select(mWrite));
	THROW(CheckErrorStatus());
	if(P_Ssl == 0) {
		len = ::send(S, (const char *)pBuf, size, 0);
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

int SLAPI TcpSocket::SendBuf(SBuffer & rBuf, size_t * pSendedSize)
{
	int    ok = -1;
	size_t sz, total_sz = 0;
	while((sz = rBuf.GetAvailableSize()) != 0) {
		size_t sended_sz = 0;
		SETMIN(sz, OutBuf.GetSize());
		rBuf.Read((char *)OutBuf, sz);
		THROW(Send((char *)OutBuf, sz, &sended_sz));
		THROW_S(sended_sz != 0, SLERR_SOCK_SEND);
		total_sz += sended_sz;
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pSendedSize, total_sz);
	return ok;
}

int SLAPI TcpSocket::GetStat(long * pRdCount, long * pWrCount)
{
	ASSIGN_PTR(pRdCount, StatData.RdCount);
	ASSIGN_PTR(pWrCount, StatData.WrCount);
	return 1;
}
//
//
//
SLAPI TcpServer::TcpServer(const InetAddr & rAddr) : TcpSocket(1000)
{
	Addr = rAddr;
}

SLAPI TcpServer::~TcpServer()
{
}

int SLAPI TcpServer::ExecSession(TcpSocket & rSock, InetAddr & rAddr)
{
	return 1;
}

int SLAPI TcpServer::Run()
{
	int    ok = 1;
#ifndef _WIN32_WCE
	SString msg_buf, temp_buf;
	THROW(Bind(Addr));
	THROW(Listen());
	while(!SLS.CheckStopFlag()) {
		if(Select(TcpSocket::mRead, 10000) > 0) { // @v6.1.4 timeout: -1-->10000
			ENTER_CRITICAL_SECTION
			InetAddr cli_addr;
			TcpSocket cli_sock(60000); // @v6.1.2 timeout: 0-->300000 @v7.8.10 timeout: 300000-->60000
			if(Accept(&cli_sock, &cli_addr)) {
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
#endif // } _WIN32_WCE
	return ok;
}
//
//
//
//static
int FASTCALL InetUrl::GetDefProtocolPort(int protocol)
{
	int    port = 0;
	switch(protocol) {
		case protHttp:     port = 80; break;
		case protHttps:    port = 443; break;
		case protFtp:      port = 20; break;
		case protGopher:   port = 70; break;
		case protSvn:      port = 3690; break;
		case protSMTP:     port = 25; break;
		case protSMTPS:    port = 465; break;
		case protPOP3:     port = 110; break;
		case protPOP3S:    port = 995; break;
		case protIMAP:     port = 143; break;
		case protIMAPS:    port = 993; break;
		case protFtps:     port = 990; break;
		case protTFtp:     port = 69; break;
		case protDict:     port = 2628; break;
		case protSSH:      port = 22; break;
		case protSMB:      port = 445; break;
		case protSMBS:     port = 445; break;
		case protRTSP:     port = 554; break;
		case protRTMP:     port = 1935; break;
		case protRTMPT:    port = 80; break;
		case protRTMPS:    port = 443; break;
		case protLDAP:     port = 389; break;
		case protLDAPS:    port = 636; break;
	}
	return port;
}

InetUrl::InetUrl(const char * pUrl) : InetAddr()
{
	Clear();
	Parse(pUrl);
}

long InetUrl::GetState() const
{
	return State;
}

int InetUrl::Valid() const
{
	return BIN(!(State & stError));
}

int InetUrl::IsEmpty() const
{
	return BIN(State & stEmpty);
}

InetUrl & InetUrl::Clear()
{
	State = stEmpty;
	TermList.Clear();
	Org = 0;
	Protocol = protUnkn;
	return *this;
}

static const char * SchemeMnem[] = {
	"",
	"http",
	"https",
	"ftp",
	"gopher",
	"mailto",
	"news",
	"nntp",
	"irc",
	"prospero",
	"telnet",
	"wais",
	"xmpp",
	"file",
	"data",
	"svn",
	"socks4",
	"socks5",
	"smtp",
	"smtps",
	"pop3",
	"pop3s",
	"imap",
	"imaps",
	"ftps",
	"tftp",
	"dict",
	"ssh",
	"smb",
	"smbs",
	"rtsp"
	"rtmp",
	"rtmpt",
	"rtmps",
	"ldap",
	"ldaps"
};

//static
const char * FASTCALL InetUrl::GetSchemeMnem(int schemeId)
{
	return (schemeId >= 0 && schemeId < SIZEOFARRAY(SchemeMnem)) ? SchemeMnem[schemeId] : SchemeMnem[0];
}

//static
int FASTCALL InetUrl::GetSchemeId(const char * pSchemeMnem)
{
	for(uint i = 0; i < SIZEOFARRAY(SchemeMnem); i++) {
		if(sstreqi_ascii(pSchemeMnem, SchemeMnem[i])) {
			return (int)i;
		}
	}
	return protUnkn;
}

int InetUrl::Parse(const char * pUrl)
{
	// http://<host>:<port>/<context>
	// <схема>://<логин>:<пароль>@<хост>:<порт>/<URL-путь>?<параметры>#<якорь>
	//
	// Divisors: :, :/, ://, :\, :\\, /, \, ?, #, @

	int    ok = 1;
	Clear();
	if(isempty(pUrl)) {
		ok = -1;
	}
	else {
		UriParserState state;
		UriUri uri;
		state.uri = &uri;
		SString temp_buf;
		if(UriParseUri(&state, pUrl)) {
			temp_buf.CopyFromN(uri.scheme.first, (uri.scheme.afterLast-uri.scheme.first));
			if(temp_buf.NotEmpty()) {
				Protocol = GetSchemeId(temp_buf);
				TermList.Add(cScheme, temp_buf);
			}
			temp_buf.CopyFromN(uri.userInfo.first, (uri.userInfo.afterLast-uri.userInfo.first));
			if(temp_buf.NotEmpty()) {
				SString user_name, pw;
				temp_buf.Divide(':', user_name, pw);
				if(user_name.NotEmptyS())
					TermList.Add(cUserName, user_name);
				if(pw.NotEmptyS())
					TermList.Add(cPassword, pw);
			}
			temp_buf.CopyFromN(uri.hostText.first, (uri.hostText.afterLast-uri.hostText.first));
			if(temp_buf.NotEmpty())
				TermList.Add(cHost, temp_buf);
			temp_buf.CopyFromN(uri.portText.first, (uri.portText.afterLast-uri.portText.first));
			if(temp_buf.NotEmpty())
				TermList.Add(cPort, temp_buf);
			temp_buf = uri.pathHead ? uri.pathHead->text.first : 0;
			if(temp_buf.NotEmpty())
				TermList.Add(cPath, temp_buf);
			temp_buf.CopyFromN(uri.query.first, (uri.query.afterLast-uri.query.first));
			if(temp_buf.NotEmpty())
				TermList.Add(cQuery, temp_buf);
			temp_buf.CopyFromN(uri.fragment.first, (uri.fragment.afterLast-uri.fragment.first));
			if(temp_buf.NotEmpty())
				TermList.Add(cRef, temp_buf);
		}
		else {
			State |= stError;
			ok = 0;
		}
		UriFreeUriMembers(&uri);
	}
	return ok;
}

int InetUrl::SetProtocol(int protocol)
{
	int    preserve_val = Protocol;
	Protocol = protocol;
	return preserve_val;
}

int InetUrl::GetProtocol() const
{
	return Protocol;
}

int InetUrl::GetComponent(int c, SString & rBuf) const
{
	int    ok = 1;
	rBuf = 0;
	return TermList.Get(c, rBuf);
}
//
//
//
SProxiAuthParam::Entry::Entry()
{
	Protocol = 0;
	Mode = 0;
	Flags = 0;
}

SProxiAuthParam & SProxiAuthParam::Clear()
{
	Ver = 1;
	List.freeAll();
	return *this;
}

int SProxiAuthParam::ToStr(long fmt, SString & rBuf) const
{
	int    ok = 1;
	StringSet ss("\001");
	SString temp_buf;
	ss.add("SPAP");
	ss.add((temp_buf = 0).Cat(Ver));
	for(uint i = 0; i < List.getCount(); i++) {
		const Entry * p_entry = List.at(i);
		ss.add(temp_buf = InetUrl::GetSchemeMnem(p_entry->Protocol));
		ss.add((temp_buf = 0).Cat(p_entry->Mode));
		ss.add((temp_buf = 0).Cat(p_entry->Flags));
		ss.add(p_entry->Addr.ToStr(InetAddr::fmtHost|InetAddr::fmtPort, temp_buf));
		ss.add(p_entry->UserName);
		{
			temp_buf = p_entry->Password;
			char   pw_buf[128], pw_buf2[128];
			const  size_t bin_pw_size = sizeof(pw_buf2)/2;
			size_t pw_len = temp_buf.Len();
			IdeaRandMem(pw_buf2, sizeof(pw_buf2));
			temp_buf.CopyTo(pw_buf2, sizeof(pw_buf2));
			IdeaRandMem(pw_buf, sizeof(pw_buf));
			IdeaEncrypt(0, pw_buf2, bin_pw_size);
			(temp_buf = 0).EncodeMime64(pw_buf2, bin_pw_size);
			ss.add(temp_buf);
		}
	}
	rBuf = ss.getBuf();
	return ok;
}

int SProxiAuthParam::FromStr(long fmt, const char * pStr)
{
	int    ok = 1;

	SString temp_buf;
	StringSet ss('\001', pStr);
	uint   ssp = 0;
	THROW(ss.get(&ssp, temp_buf));
	THROW(temp_buf == "SPAP");
	THROW(ss.get(&ssp, temp_buf));
	THROW(temp_buf.ToLong() == 1);

	Clear();
	while(ss.get(&ssp, temp_buf)) {
		Entry * p_entry = new Entry;
		THROW(p_entry);
		p_entry->Protocol = InetUrl::GetSchemeId(temp_buf);
		THROW(ss.get(&ssp, temp_buf));
		p_entry->Mode = temp_buf.ToLong();
		THROW(ss.get(&ssp, temp_buf));
		p_entry->Flags = temp_buf.ToLong();
		THROW(ss.get(&ssp, temp_buf));
		p_entry->UserName = temp_buf;
		THROW(ss.get(&ssp, temp_buf));
		{
			char   pw_buf[128];
			size_t bin_pw_size = 0;
			THROW(temp_buf.DecodeMime64(pw_buf, sizeof(pw_buf), &bin_pw_size) > 0);
			IdeaDecrypt(0, pw_buf, bin_pw_size);
			p_entry->Password = pw_buf;
		}
	}
	CATCHZOK
	return ok;
}

SProxiAuthParam::SProxiAuthParam()
{
	Ver = 1;
}

int SProxiAuthParam::SetEntry(Entry & rEntry)
{
	int    ok = 0;
	uint   pos = 0;
	if(List.lsearch(&rEntry.Protocol, &pos, CMPF_LONG)) {
		Entry * p_entry = List.at(pos);
		*p_entry = rEntry;
		ok = 2;
	}
	else {
		Entry * p_entry = new Entry;
		*p_entry = rEntry;
		List.insert(p_entry);
		ok = 1;
	}
	return ok;
}

int SProxiAuthParam::GetEntry(int protocol, Entry & rEntry) const
{
	int    ok = 0;
	uint   pos = 0;
	if(List.lsearch(&protocol, &pos, CMPF_LONG)) {
		Entry * p_entry = List.at(pos);
		rEntry = *p_entry;
		ok = 1;
	}
	else if(protocol) {
		ok = GetEntry(0, rEntry); // @recursion
	}
	return ok;
}
//
//
//
SHttpClient::Response::Response()
{
	HttpVerMj = 0;
	HttpVerMn = 0;
	ErrCode = 0;
	TransferType = transftypUndef;
	ContentLength = 0;
	State = 0;
}

SHttpClient::Response & SHttpClient::Response::Reset()
{
	HttpVerMj = 0;
	HttpVerMn = 0;
	ErrCode = 0;
	TransferType = transftypUndef;
	ContentLength = 0;
	State = 0;
	Descr = 0;
	SBuffer::Clear();
	Header.Clear();
	return *this;
}
//
//
//
SHttpClient::SHttpClient()
{
	HttpVerMj = 1;
	HttpVerMn = 1;
}

SHttpClient::~SHttpClient()
{
}

int SHttpClient::SetHeader(int hdrTag, const char * pValue)
{
	return Header.Add(hdrTag, NZOR(pValue, ""));
}

void SHttpClient::ClearHeader()
{
	Header.Clear();
}

static const char * HttpHeader[] = {
	"",
	"Cache-Control",
	"Connection",
	"Date",
	"Pragma",
	"Trailer",
	"Transfer-Encoding",
	"Upgrade",
	"Via",
	"Warning",
	"Allow",
	"Content-Encoding",
	"Content-Language",
	"Content-Length",
	"Content-Location",
	"Content-MD5",
	"Content-Range",
	"Content-Type",
	"Expires",
	"Last-Modified",
	"Accept-Ranges",
	"Age",
	"ETag",
	"Location",
	"Proxy-Authenticate",
	"Retry-After",
	"Server",
	"Vary",
	"WWW-Authenticate",
	"Accept",
	"Accept-Charset",
	"Accept-Encoding",
	"Accept-Language",
	"Authorization",
	"Expect",
	"From",
	"Host",
	"If-Match",
	"If-Modified-Since",
	"If-None-Match",
	"If-Range",
	"If-Unmodified-Since",
	"Max-Forwards",
	"Proxy-Authorization",
	"Range",
	"Referer",
	"TE",
	"User-Agent",
	"SoapAction"
};

// static
int SHttpClient::GetHeaderTitle(int hdr, SString & rTitle)
{
	int    ok = 1;
	if(hdr > 0 && hdr < SIZEOFARRAY(HttpHeader)) {
		rTitle = HttpHeader[hdr];
		ok = 1;
	}
	else {
		rTitle = 0;
		ok = 0;
	}
	return ok;
}

// static
int SHttpClient::GetHeaderId(const char * pTitle)
{
	int   id = 0;
	if(!isempty(pTitle)) {
		for(uint i = 1; !id && i < SIZEOFARRAY(HttpHeader); i++) {
			if(stricmp(HttpHeader[i], pTitle) == 0)
				id = (int)i;
		}
	}
	return id;
}

int SHttpClient::TolkToServer(int method, const char * pUrl)
{
	int    ok = 1;
	SString temp_buf, line_buf, host_buf, context_buf;
	InetUrl url(pUrl);
	THROW(url.Valid());
	url.GetComponent(InetUrl::cHost, host_buf);
	THROW(host_buf.NotEmpty()); // @error host not defined
	{
		url.GetComponent(InetUrl::cPort, temp_buf);
		int    port = temp_buf.ToLong();
		if(port == 0) {
			url.GetComponent(InetUrl::cScheme, temp_buf);
			if(InetUrl::GetSchemeId(temp_buf) == InetUrl::protHttp)
				port = 80;
			else if(InetUrl::GetSchemeId(temp_buf) == InetUrl::protHttps)
				port = 443;
			else
				port = 80;
		}
		url.Set(host_buf, port);
	}
	url.GetComponent(InetUrl::cQuery, context_buf);
	{
		char   sd_buf[256];
		datetimefmt(getcurdatetime_(), DATF_INTERNET, TIMF_HMS|TIMF_TIMEZONE, sd_buf, sizeof(sd_buf));
		SetHeader(hdrDate, sd_buf);
		SetHeader(hdrHost, host_buf);
	}
	//
	THROW(S.Connect(url));
	{
		size_t sended_bytes = 0;
		//
		// Посылаем сам запрос
		//
		line_buf = 0;
		if(method == reqGet) {
			line_buf.Cat("GET").Space();
		}
		else if(method == reqPost) {
			line_buf.Cat("POST").Space();
		}
		else {
			CALLEXCEPT(); // Invalid request method
		}
		if(context_buf.NotEmpty())
			line_buf.Cat(context_buf);
		else
			line_buf.CatChar('/');
		line_buf.Space().Cat("HTTP").CatChar('/').Cat(HttpVerMj).Dot().Cat(HttpVerMn).CRB();
		//
		// Посылаем теги заголовка запроса
		//
		for(uint i = 0; i < Header.getCount(); i++) {
			StrAssocArray::Item hitem = Header.at(i);
			if(!isempty(hitem.Txt) && GetHeaderTitle(hitem.Id, temp_buf)) {
				line_buf.Cat(temp_buf).CatDiv(':', 2).Cat(hitem.Txt).CRB();
			}
		}
		line_buf.CRB();
		THROW(S.Send(line_buf, line_buf.Len(), &sended_bytes));
	}
	CATCHZOK
	return ok;
}

int SHttpClient::ReadResponse(Response & rRsp)
{
	//TcpSocket SBuffer
	int    ok = 1;
	SString temp_buf, left_buf, right_buf, content_type;
	size_t rcv_bytes = 0;
	THROW(S.RecvBuf(rRsp, 0, &rcv_bytes));
	{
		//
		// Разбираем заголовок ответа
		//
		SStrScan scan((const char *)(const void *)rRsp);
		THROW(scan.SearchChar(' '));
		scan.Get(temp_buf);
		scan.IncrLen(1);
		//
		// Считываем номер версии HTTP
		//
		THROW(temp_buf.CmpPrefix("HTTP/", 0) == 0);
		temp_buf.ShiftLeft(5).Divide('.', left_buf, right_buf);
		rRsp.HttpVerMj = (uint16)left_buf.ToLong();
		rRsp.HttpVerMn = (uint16)right_buf.ToLong();
		//
		// Считываем код ошибки
		//
		THROW(scan.SearchChar(' '));
		scan.Get(temp_buf);
		scan.IncrLen(1);
		rRsp.ErrCode = temp_buf.ToLong();
		//
		// Считываем строку описания ответа
		//
		THROW(scan.Search("\r\n"));
		scan.Get(rRsp.Descr);
		scan.IncrLen(2);
		//
		// Считываем теги ответа в формате [key]: [value]
		//
		while(scan.SearchChar('\n')) {
			scan.Get(temp_buf);
			scan.IncrLen(1);
			temp_buf.Divide(':', left_buf, right_buf);
			long   tag_id = SHttpClient::GetHeaderId(left_buf.Strip());
			if(tag_id)
				rRsp.Header.Add(tag_id, right_buf.Strip());
		}
		//
		// @todo Разобрать тип контента (hdrContentType)
		//
		// str = hpairnode_get(res->header, HEADER_CONTENT_TYPE);
		// if(str != NULL)
		//    res->content_type = content_type_new(str);
		//

		//
		// Получаем собственно содержание ответа
		//
		if(rRsp.Header.Get(hdrContentLen, temp_buf)) {
			rRsp.ContentLength = (size_t)temp_buf.ToLong();
			rRsp.TransferType = rRsp.transftypContentLength;
		}
		else if(rRsp.Header.Get(hdrTransferEnc, temp_buf) && temp_buf == "chunked") {
			rRsp.ContentLength = 0;
			rRsp.TransferType = rRsp.transftypChunked;
		}
		else {
			// Assume connection close
			rRsp.TransferType = rRsp.transftypConnectionClose;
		}
		if(rRsp.Header.Get(hdrContentType, temp_buf) && temp_buf.NotEmpty()) {
			// example: "Content-Type: text/html; charset=ISO-8859-4"
			StringSet ss(';', temp_buf);
			for(uint ss_pos = 0, ss_idx = 0; ss.get(&ss_pos, temp_buf); ss_idx++) {
				if(ss_idx == 0) {
					content_type = temp_buf.Strip();
				}
				else {
					temp_buf.Divide('=', left_buf, right_buf);
					// @todo
				}
			}
			if(content_type == "multipart/related") {

			}
		}
	}
	//
	//
	//
	CATCHZOK
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
					pair = hpairnode_new(key, value, NULL);
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
			if(!strcmp(&(buffer[i-1]), "\n\n") || !strcmp(&(buffer[i-2]), "\n\r\n"))
				break;
		}
		i++;
	}
	/* Create response */
	res = _hresponse_parse_header(buffer);
	if(res == NULL) {
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
	if((res->content_type && !strcmp(res->content_type->type, "multipart/related"))) {
		status = mime_get_attachments(res->content_type, res->in, &mimeMessage);
		if(status != H_OK) {
			/* TODO (#1#): Handle error */
			hresponse_free(res);
			return status;
		}
		else {
			res->attachments = mimeMessage;
			http_input_stream_free(res->in);
			res->in = http_input_stream_new_from_file(mimeMessage->root_part->filename);
			if(!res->in) {
				/* TODO (#1#): Handle error */
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
class SFtpClient {
public:
	enum {
		stConnected = 0x0001,
		stError     = 0x0002
	};
	SFtpClient();
	SFtpClient(const char * pAddr, const char * pLogin, const char * pPassword);
	~SFtpClient();
    int    operator !() const
    {
    	return BIN(State & stError);
    }
	int    Connect(const char * pAddr, const char * pLogin, const char * pPassword);
	int    Disconnect();
	int    PutFile(const char * pLocSrc, const char * pFtpDst);
	int    GetFile(const char * pFtpSrc, const char * pLocDst);
	int    DeleteFile(const char * pFtpFile);
	int    RenameFile(const char * pFtpFileName, const char * pNewFtpFileName);

    class Enum : public SEnumImp {
	public:
		Enum(SFtpClient * pCli, const char * pWildcard);
		virtual ~Enum();
        virtual int Next(void * pDirEntry);
	private:
        void * P;
        enum {
        	stFirst     = 0x0001,
        	stNothing   = 0x0002,
        	stConnected = 0x0004
        };
        long   State;
		SDirEntry CEntry; // Последняя найденная запись
    };
private:
	friend class SFtpClient::Enum;

	long   State;
	void * H_Conn;
	void * H_Sess;
};

SFtpClient::Enum::Enum(SFtpClient * pCli, const char * pWildcard)
{
	P = 0;
	State = 0;
	if(pCli->State & SFtpClient::stConnected) {
		State |= stConnected;
		WIN32_FIND_DATA find_data;
		HINTERNET h = FtpFindFirstFile((HINTERNET)pCli->H_Sess, pWildcard, &find_data, 0, NULL);
		if(h) {
			P = (void *)h;
			CEntry = find_data;
			State |= stFirst;
		}
		else {
			State |= stNothing;
		}
	}
}

SFtpClient::Enum::~Enum()
{
	if(P)
		InternetCloseHandle((HINTERNET)P);
}

int SFtpClient::Enum::Next(void * pDirEntry)
{
	int    ok = 0;
	if(P) {
        if(State & stFirst) {
        	ASSIGN_PTR((SDirEntry *)pDirEntry, CEntry);
        	State &= ~stFirst;
        	ok = 1;
        }
        else if(!(State & stNothing)) {
        	WIN32_FIND_DATA find_data;
			if(InternetFindNextFile((HINTERNET)P, &find_data)) {
				CEntry = find_data;
				ASSIGN_PTR((SDirEntry *)pDirEntry, CEntry);
				ok = 1;
			}
			else {
				State |= stNothing;
			}
        }
	}
	return ok;
}

SFtpClient::SFtpClient()
{
	State = 0;
	H_Conn = 0;
	H_Sess = 0;
}

SFtpClient::SFtpClient(const char * pAddr, const char * pLogin, const char * pPassword)
{
	State = 0;
	H_Conn = 0;
	H_Sess = 0;
	Connect(pAddr, pLogin, pPassword);
}

SFtpClient::~SFtpClient()
{
	Disconnect();
}

int SFtpClient::Connect(const char * pAddr, const char * pLogin, const char * pPassword)
{
	int    ok = 1;
	State &= ~stError;
	if(State & stConnected) {
		Disconnect();
	}
	{
		HINTERNET h = ::InternetOpen(0, INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);
		THROW_S(h, SLERR_WINDOWS);
		H_Conn = (void *)h;
		h = ::InternetConnect((HINTERNET)H_Conn, pAddr, INTERNET_DEFAULT_FTP_PORT, pLogin, pPassword, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
		THROW_S(h, SLERR_WINDOWS);
		H_Sess = (void *)h;
		State |= stConnected;
	}
    CATCH
		if(H_Conn) {
			::InternetCloseHandle((HINTERNET)H_Conn);
			H_Conn = 0;
		}
		State |= stError;
		ok = 0;
	ENDCATCH;
	return ok;
}

int SFtpClient::Disconnect()
{
	int    ok = -1;
	State &= ~stError;
	if(State & stConnected) {
		::InternetCloseHandle((HINTERNET)H_Sess);
		H_Sess = 0;
		::InternetCloseHandle((HINTERNET)H_Conn);
		H_Conn = 0;
		State &= ~stConnected;
		ok = 1;
	}
	return ok;
}

int SFtpClient::PutFile(const char * pLocSrc, const char * pFtpDst)
{
	int    ok = 1;
	THROW_S(State & stConnected, SLERR_FTP_NOTCONNECTED);
	THROW_S(FtpPutFile((HINTERNET)H_Sess, pLocSrc, pFtpDst, FTP_TRANSFER_TYPE_BINARY, 0), SLERR_WINDOWS);
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH;
	return ok;
}

int SFtpClient::GetFile(const char * pFtpSrc, const char * pLocDst)
{
	int    ok = 1;
	THROW_S(State & stConnected, SLERR_FTP_NOTCONNECTED);
	THROW_S(FtpGetFile((HINTERNET)H_Sess, pFtpSrc, pLocDst, 0, 0, 0, NULL), SLERR_WINDOWS);
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH;
    return ok;
}

int SFtpClient::DeleteFile(const char * pFtpFile)
{
	int    ok = 1;
	THROW_S(State & stConnected, SLERR_FTP_NOTCONNECTED);
	THROW_S(FtpDeleteFile((HINTERNET)H_Sess, pFtpFile), SLERR_WINDOWS);
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH;
    return ok;
}

int SFtpClient::RenameFile(const char * pFtpFileName, const char * pNewFtpFileName)
{
	int    ok = 1;
	THROW_S(State & stConnected, SLERR_FTP_NOTCONNECTED);
	THROW_S(FtpRenameFile((HINTERNET)H_Sess, pFtpFileName, pNewFtpFileName), SLERR_WINDOWS);
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH;
    return ok;
}
//
//
//
#define CURL_STATICLIB
#include <curl/curl.h>

ScURL::HttpForm::HttpForm()
{
	FH = 0;
	LH = 0;
}

ScURL::HttpForm::~HttpForm()
{
	curl_formfree((struct curl_httppost *)FH);
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
    SString ctype = pContentType;
    SString cname = pContentName;
    if(!isempty(pFileName) && fileExists(pFileName)) {
		if(!cname.NotEmptyS()) {
            S_GUID u;
            u.Generate();
            u.ToStr(S_GUID::fmtIDL, cname);
		}
		if(!curl_formadd((struct curl_httppost **)&FH, (struct curl_httppost **)&LH,
			CURLFORM_COPYNAME, (const char *)cname, CURLFORM_FILE, pFileName, CURLFORM_CONTENTTYPE, (const char *)ctype, CURLFORM_END))
			ok = 0;
    }
    else
		ok = 0;
    return ok;
}

//static
int ScURL::_GlobalInitDone = 0;

//static
size_t ScURL::CbRead(char * pBuffer, size_t size, size_t nitems, void * pExtra)
{
	size_t ret = CURL_READFUNC_ABORT;
	if(pExtra) {
        SFile * p_f = (SFile *)pExtra;
        size_t actual_size = 0;
        if(p_f->Read(pBuffer, size * nitems, &actual_size))
			ret = actual_size;
	}
	return ret;
}

//static
size_t ScURL::CbWrite(char * pBuffer, size_t size, size_t nmemb, void * pExtra)
{
	size_t ret = CURLE_WRITE_ERROR;
	if(pExtra) {
		SFile * p_f = (SFile *)pExtra;
		if(p_f->Write(pBuffer, size * nmemb))
			ret = (size * nmemb);
	}
	return ret;
}

#define _CURLH ((CURL *)H)

ScURL::ScURL()
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
	CATCHZOK
	return ok;
}

int ScURL::SetupCbWrite(SFile * pF)
{
	int    ok = 1;
	if(pF) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEFUNCTION, ScURL::CbWrite)));
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_WRITEDATA, pF)));
	}
	CATCHZOK
	return ok;
}

int ScURL::HttpPost(const char * pUrl, int mflags, HttpForm & rF, SFile * pReplyStream)
{
	int    ok = 1;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	if(mflags & mfDontVerifySslPeer) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 0)));
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_HTTPPOST, (struct curl_httppost *)rF.FH)));
	THROW(SetupCbWrite(pReplyStream));
	THROW(SetError(curl_easy_perform(_CURLH)));
	CATCHZOK
	return ok;
}

int ScURL::HttpPost(const char * pUrl, int mflags, const StrStrAssocArray * pFields, SFile * pReplyStream)
{
	int    ok = 1;
	uint   flds_count = 0;
	SString flds_buf;
	if(pFields) {
		SString temp_buf;
		for(uint i = 0; i < pFields->getCount(); i++) {
			StrStrAssocArray::Item item = pFields->at(i);
            if(!isempty(item.Key)) {
				if(flds_count)
					flds_buf.CatChar('&');
                flds_buf.Cat((temp_buf = item.Key).Strip().ToUrl());
                if(!isempty(item.Val)) {
					flds_buf.CatChar('=').Cat((temp_buf = item.Val).Strip().ToUrl());
                }
                flds_count++;
            }
		}
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	if(mflags & mfDontVerifySslPeer) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 0)));
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDSIZE, flds_buf.Len())));
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_POSTFIELDS, (const char *)flds_buf)));
	THROW(SetupCbWrite(pReplyStream));
	THROW(SetError(curl_easy_perform(_CURLH)));
	CATCHZOK
	return ok;
}

int ScURL::HttpGet(const char * pUrl, int mflags, SFile * pReplyStream)
{
	int    ok = 1;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	if(mflags & mfDontVerifySslPeer) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 0)));
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "GET")));
	THROW(SetupCbWrite(pReplyStream));
	THROW(SetError(curl_easy_perform(_CURLH)));
	CATCHZOK
	return ok;
}

int ScURL::HttpGet(const char * pUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream)
{
	int    ok = 1;
	uint   flds_count = 0;
	struct curl_slist * p_chunk = 0;
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_URL, pUrl)));
	if(mflags & mfDontVerifySslPeer) {
		THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_SSL_VERIFYPEER, 0)));
	}
	THROW(SetError(curl_easy_setopt(_CURLH, CURLOPT_CUSTOMREQUEST, "GET")));
	THROW(SetupCbWrite(pReplyStream));
	if(pHttpHeaderFields && pHttpHeaderFields->getCount()) {
		SString temp_buf;
		SString fld_buf;
		for(uint i = 0; i < pHttpHeaderFields->getCount(); i++) {
			StrStrAssocArray::Item item = pHttpHeaderFields->at(i);
			temp_buf = item.Key;
            if(temp_buf.NotEmptyS()) {
				fld_buf = 0;
				fld_buf.Cat(temp_buf);
				temp_buf = item.Val;
				if(temp_buf.NotEmptyS()) {
					fld_buf.CatDiv(':', 2);
					fld_buf.Cat(temp_buf);
				}
				else
					fld_buf.CatChar(':');
				p_chunk = curl_slist_append(p_chunk, (const char *)fld_buf);
                flds_count++;
            }
		}
		if(p_chunk)
			curl_easy_setopt(_CURLH, CURLOPT_HTTPHEADER, p_chunk);
	}
	THROW(SetError(curl_easy_perform(_CURLH)));
	CATCHZOK
	curl_slist_free_all(p_chunk);
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
	THROW(SetError(curl_easy_perform(_CURLH)));
	CATCHZOK
	return ok;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(Uri)
{
	const char * p_url_text[] = {
		"mailto:user@host?subject=blah",
		"file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/",
		"http://www.yahoo.com?name=%00%01",
		"https://www.yahoo.com/",
		"http://www.yahoo.com#bottom",
		"ftp://www.yahoo.com/hello",
		"http://www.yahoo.com?name=obi+wan&status=jedi",
		"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html",
		"uhtt.ru",
		"uhtt.ru:8080",
		"server:80"
		"\\\\adv16\\spii\file.xml"
	};
	int    ok = 1;
	SFile f_out(MakeOutputFilePath("url-out.txt"), SFile::mWrite);
	InetUrl url(0);
	SString out_buf, temp_buf;
	for(uint i = 0; i < SIZEOFARRAY(p_url_text); i++) {
		const char * p_url_str = p_url_text[i];
		(out_buf = 0).Cat(p_url_str);
		int    r = url.Parse(p_url_str);
		if(r > 0) {
			url.GetComponent(InetUrl::cScheme, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cUserName, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cPassword, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cHost, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cPort, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cPath, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cQuery, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			url.GetComponent(InetUrl::cRef, temp_buf);
			out_buf.Tab().Cat(temp_buf);

			f_out.WriteLine(out_buf.CR());
		}
	}
	return ok;
}

#endif // } SLTEST_RUNNING
