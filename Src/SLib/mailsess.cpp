// MAILSESS.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2010, 2011, 2012, 2013, 2014, 2016, 2017, 2018, 2020, 2023
//
#include <slib-internal.h>
#pragma hdrstop
// @v11.7.0 #include <snet.h>

#if 0 // @v9.9.0 (obsolete) {

#define DEF_RDBUFSIZ 16384
#define DEF_TIMEOUT  0

int MailSession::init()
{
	Err = 0;
	clisock = INVALID_SOCKET;
	BufSize = 0;
	RealBufSize = 0;
	RdOffs = 0;
	WrOffs = 0;
	P_Buf  = new char[DEF_RDBUFSIZ];
	if(P_Buf) {
		RealBufSize = DEF_RDBUFSIZ;
		BufSize = RealBufSize - 1024;
		return 1;
	}
	else
		return (SLibError = SLERR_NOMEM, 0);
}

MailSession::MailSession(SOCKET s, struct sockaddr_in r)
{
	init();
	Timeout = DEF_TIMEOUT;
	remote = r;
	clisock = s;
}

MailSession::MailSession(const char * dest, int port, int timeout) : P_Buf(0), BufSize(0), RealBufSize(0), Timeout(timeout ? timeout : DEF_TIMEOUT)
{
	HOSTENT * p_host = gethostbyname(dest);
	if(!p_host) {
		SString added_msg;
		added_msg.Cat(dest);
		if(port)
			added_msg.CatDiv(':', 0).Cat(port);
		SLS.SetError(Err = SLERR_SOCK_HOSTRESLVFAULT, added_msg);
	}
	else
		doConnect(*(int *)p_host->h_addr, port);
}

MailSession::MailSession(u_long ip, int port, int timeout) : P_Buf(0), BufSize(0), RealBufSize(0), Timeout(timeout ? timeout : DEF_TIMEOUT)
{
	doConnect(ip, port);
}

MailSession::~MailSession()
{
	delete P_Buf;
	/*
	NOT CLOSING AUTOMATICALLY ANYMORE!
	if(clisock != INVALIDSOCKET)
		close(clisock);
	*/
}

int MailSession::close()
{
	int    rc = 0;
	if(clisock != INVALID_SOCKET) {
		::shutdown(clisock, SD_BOTH); // helps against CLOSE_WAIT problems
		rc = ::closesocket(clisock);
	}
	clisock = INVALID_SOCKET;
	return rc;
}

void MailSession::setTimeout(int seconds)
{
	Timeout = seconds;
}

// gets called from constructor context, should clean up *everything*
int MailSession::doConnect(u_long ip, int port)
{
	int    ok = 1, err;
	THROW(init());
	clisock = socket(AF_INET, SOCK_STREAM, 0);
	THROW_S(clisock != INVALID_SOCKET, SLERR_SOCK_UNABLEOPEN);
	memzero(&remote, sizeof(remote));
	remote.sin_family = AF_INET;
	remote.sin_port   = htons(port);
	remote.sin_addr.s_addr = ip;
	if(connect(clisock, (struct sockaddr *)&remote, sizeof(remote)) == SOCKET_ERROR) {
		int    len;
		fd_set rset, wset;
		struct timeval tval;
		err = WSAGetLastError();
		THROW_S(!err || err == WSAEWOULDBLOCK, SLERR_SOCK_CONNECT);
		FD_ZERO(&rset);
		FD_SET(clisock, &rset);
		wset = rset;
		tval.tv_sec = Timeout;
		tval.tv_usec = 0;
		THROW_S(select(clisock+1 /* ignored */, &rset, &wset, 0, tval.tv_sec ? &tval : 0), SLERR_SOCK_TIMEOUT);
		THROW_S(FD_ISSET(clisock, &rset) || FD_ISSET(clisock, &wset), SLERR_SOCK_NONBLOCKINGCONN);
		len = sizeof(err);
		THROW_S(getsockopt(clisock, SOL_SOCKET, SO_ERROR, (char *)&err, &len) >= 0, SLERR_SOCK_CONNECT);
		THROW_S(err == 0, SLERR_SOCK_CONNECT);
	}
	CATCH
		if(clisock != INVALID_SOCKET) {
			closesocket(clisock);
			clisock = INVALID_SOCKET;
		}
		ok = 0;
		Err = SLibError;
	ENDCATCH
	return ok;
}

int MailSession::putBuffer(const void * pBuf, size_t bufLen)
{
	const char * p_buf = static_cast<const char *>(pBuf);
	for(size_t written = 0; written < bufLen;) {
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(clisock, &wset);
		struct timeval tval;
		tval.tv_sec  = Timeout;
		tval.tv_usec = 0;
		if(!select(clisock+1 /* ignored */, 0, &wset, 0, tval.tv_sec ? &tval : 0))
			return (SLibError = SLERR_SOCK_TIMEOUT, 0);
		if(FD_ISSET(clisock, &wset)) {
			int err, len = sizeof(err);
			if(getsockopt(clisock, SOL_SOCKET, SO_ERROR, (char *)&err, &len) < 0 || err)
				return (SLibError = SLERR_SOCK_OPTERR, 0);
		}
		else
			return (SLibError = SLERR_SOCK_NONBLOCKINGWR, 0);
		int bytes = send(clisock, p_buf + written, bufLen - written, 0);
		if(bytes < 0)
			return (SLibError = SLERR_SOCK_SEND, 0);
		written += bytes;
	}
	return 1;
}

int MailSession::putLine(const char * pStr)
{
	return putBuffer(pStr, strlen(pStr));
}

static char * FASTCALL searcheol(char * p, size_t len)
{
	for(size_t n = 1; n < len; n++)
		if(p[n] == '\x0A' && p[n-1] == '\x0D')
			return p+n-1;
	return 0;
}

int MailSession::timeoutRead(int s, char *buf, size_t len, size_t * pRdBytes)
{
	int    ok = 1;
	int    read_bytes = 0;
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(clisock, &rset);
	struct timeval tval;
	tval.tv_sec = Timeout;
	tval.tv_usec = 0;
	int    err, _l = sizeof(err);
	THROW_S(select(clisock+1 /* ignored */, &rset, 0, 0, tval.tv_sec ? &tval : 0), SLERR_SOCK_TIMEOUT);
	THROW_S(FD_ISSET(clisock, &rset), SLERR_SOCK_NONBLOCKINGRD);
	THROW_S(getsockopt(clisock, SOL_SOCKET, SO_ERROR, (char *)&err, &_l) >= 0 && !err, SLERR_SOCK_OPTERR);
	read_bytes = ::recv(s, buf, len, 0);
	THROW_S(read_bytes != SOCKET_ERROR, SLERR_SOCK_RECV);
	if(read_bytes == 0)
		ok = -1;
	CATCH
		ok = 0;
		read_bytes = 0;
	ENDCATCH
	ASSIGN_PTR(pRdBytes, (size_t)read_bytes);
	return ok;
}

int MailSession::haveLine()
{
	return (WrOffs != RdOffs && searcheol(P_Buf+RdOffs, WrOffs-RdOffs));
}

int MailSession::getLine(SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	int    zero_bytes_readed = 0;
	// read data into a buffer
	// find first \xD\xA, and return that as string, store how far we were
	while(ok < 0) {
		char * p;
		if(WrOffs > RdOffs && (p = searcheol(P_Buf+RdOffs, WrOffs-RdOffs))) {
			// we have a full line in store, return that
			// from P_Buf+RdOffs to p should become the new line
			size_t linelength = p-(P_Buf+RdOffs);
			*p = 0; // terminate
			rBuf.Cat(P_Buf+RdOffs);
			RdOffs += linelength+2; // +2 = \xD\xA
			ok = 1;
		}
		else {
			size_t rd_bytes = 0;
			//
			// Проверка на предмет того, что при предыдущем чтении было считано 0 байт.
			// Если так и символа конца строки в считанном участке нет, то можно закругляться с индикацией ошибки.
			//
			THROW_S(zero_bytes_readed == 0, SLERR_SOCK_RECV);
			//
			if(WrOffs <= RdOffs)
				WrOffs = RdOffs = 0;
			// we need more data before we can return a line
			if(WrOffs >= BufSize) { // buffer is full, flush to left
				THROW_S(RdOffs, SLERR_SOCK_LINETOOLONG);
				memmove(P_Buf, P_Buf+RdOffs, WrOffs-RdOffs);
				WrOffs -= RdOffs;
				RdOffs = 0;
			}
			THROW(timeoutRead(clisock, P_Buf+WrOffs, BufSize-WrOffs, &rd_bytes));
			WrOffs += rd_bytes;
			if(!rd_bytes)
				zero_bytes_readed = 1;
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}
#endif // } 0 @v9.9.0 (obsolete)
//
//
//
SMailClient::SMailClient() : State(0), P_Pop3AuthSession(0)
{
}

SMailClient::~SMailClient()
{
	Disconnect();
}

int SMailClient::Connect(InetUrl & rUrl, int timeout)
{
	int    ok = 1;
	if(State & stConnected)
		ok = -1;
	else {
		C.Z();
		const int protocol = rUrl.GetProtocol();
		THROW_S_S(oneof4(protocol, InetUrl::protSMTP, InetUrl::protSMTPS, InetUrl::protPOP3, InetUrl::protPOP3S), SLERR_MAIL_INVPROTOCOL,
			InetUrl::GetSchemeMnem(protocol));
		So.SetTimeout((timeout > 0) ? timeout : 1000);
		if(oneof2(protocol, InetUrl::protSMTPS, InetUrl::protPOP3S)) {
			THROW(So.Connect(TcpSocket::sslmClient, rUrl));
		}
		else {
			THROW(So.Connect(rUrl));
		}
		Url = rUrl;
		State |= stConnected;
		{
			SString cmd_buf, reply_buf, temp_buf, word_buf;
			if(oneof2(protocol, InetUrl::protPOP3, InetUrl::protPOP3S)) {
				THROW(ReadLine(reply_buf));
				THROW(CheckReply(reply_buf));
			}
			else if(oneof2(protocol, InetUrl::protSMTP, InetUrl::protSMTPS)) {
				StringSet reply_tail;
				uint   line_no = 0;
				{
					THROW(ReadLine(reply_buf));
					THROW(CheckReply(reply_buf));
				}
				(cmd_buf = "EHLO").Space().Cat("Papyrus-Mail-Client");
				THROW(WriteLine(cmd_buf, 0));
                {
                	int    is_there_next_line = 0;
                	do {
						THROW(ReadLine(reply_buf));
						THROW(CheckReply(reply_buf));
						line_no++;
						{
							uint   p = 0;
							while(isdigit(reply_buf[p])) {
								p++;
							}
							temp_buf.Z();
							if(p) {
								if(reply_buf[p] == '-') {
									is_there_next_line = 1;
									temp_buf = reply_buf+p+1;
								}
								else {
									is_there_next_line = 0;
									if(reply_buf[p] == ' ') {
										temp_buf = reply_buf+p+1;
									}
								}
							}
							if(temp_buf.NotEmpty()) {
                                SStrScan scan(temp_buf);
                                if(scan.Get("SIZE", word_buf)) {
                                    scan.Skip();
									if(scan.GetNumber(word_buf)) {
                                        C.SmtpMaxSize = word_buf.ToLong();
									}
                                }
								else if(scan.Get("AUTH", word_buf)) {
									scan.Skip();
									while(scan.GetWord(0, word_buf)) {
                                        scan.Skip();
										if(word_buf.IsEqiAscii("PLAIN"))
											C.SmtpAuthTypeList.add(authtPlain);
										else if(word_buf.IsEqiAscii("LOGIN"))
											C.SmtpAuthTypeList.add(authtLogin);
										else if(word_buf.IsEqiAscii("CRAM-MD5"))
											C.SmtpAuthTypeList.add(authtCramMD5);
									}
								}
								else if(line_no == 1) {
									C.SmtpServerName = temp_buf.Strip();
								}
							}
						}
                	} while(is_there_next_line);
                }

			}
		}
	}
	CATCH
		So.Disconnect();
		State &= ~stConnected;
		ok = 0;
	ENDCATCH
	return ok;
}

int SMailClient::Disconnect()
{
	int    ok = 0;
	if(State & stConnected) {
		ZDELETE(P_Pop3AuthSession);
		if(State & stLoggedIn) {
			SString cmd_buf, reply_buf;
			WriteLine((cmd_buf = "QUIT"), &reply_buf);
			CheckReply(reply_buf);
			State &= ~stLoggedIn;
		}
		So.Disconnect();
		C.Z();
		State &= ~stConnected;
	}
	else
		ok = -1;
	return ok;
}

int SMailClient::CheckReply(const SString & rReplyBuf, int onlyValidCode)
{
	int   ok = 1;
	const int protocol = Url.GetProtocol();
	if(oneof2(protocol, InetUrl::protPOP3, InetUrl::protPOP3S)) {
		THROW_S(rReplyBuf.NotEmpty(), SLERR_MAIL_POP3_NOREPLY);
		THROW_S_S(!rReplyBuf.HasPrefixIAscii("-ERR"), SLERR_MAIL_POP3_REPLYERR, rReplyBuf);
		THROW_S_S(rReplyBuf.HasPrefixIAscii("+OK"), SLERR_MAIL_POP3_UNDEFREPLY, rReplyBuf);
	}
	else if(oneof2(protocol, InetUrl::protSMTP, InetUrl::protSMTPS)) {
		THROW_S(rReplyBuf.NotEmpty(), SLERR_MAIL_SMTP_NOREPLY);
		long   code = rReplyBuf.ToLong();
		if(onlyValidCode > 0) {
			THROW_S_S(code == onlyValidCode, SLERR_MAIL_SMTP_REPLYERR, rReplyBuf);
		}
		else {
			THROW_S_S(oneof8(code, 211, 214, 220, 221, 235, 250, 334, 354), SLERR_MAIL_SMTP_REPLYERR, rReplyBuf);
		}
	}
	CATCHZOK
	return ok;
}

int SMailClient::Auth(int authtype, const char * pName, const char * pPassword)
{
	int    ok = 1;
	SString cmd_buf, reply_buf, temp_buf;
	const int protocol = Url.GetProtocol();
	THROW_S(State & stConnected, SLERR_MAIL_NOTCONNECTED);
	THROW_S_S(oneof4(protocol, InetUrl::protSMTP, InetUrl::protSMTPS, InetUrl::protPOP3, InetUrl::protPOP3S), SLERR_MAIL_INVPROTOCOL,
		InetUrl::GetSchemeMnem(protocol));
	if(oneof2(protocol, InetUrl::protPOP3, InetUrl::protPOP3S)) {
		THROW(WriteLine(cmd_buf.Z().Cat("USER").Space().Cat(pName), &reply_buf));
		THROW(CheckReply(reply_buf));
		THROW(WriteLine(cmd_buf.Z().Cat("PASS").Space().Cat(pPassword), &reply_buf));
		THROW(CheckReply(reply_buf));
		State |= stLoggedIn;
	}
	else if(oneof2(protocol, InetUrl::protSMTP, InetUrl::protSMTPS)) {
		ZDELETE(P_Pop3AuthSession);
		switch(authtype) {
			case authtPlain:
				{
					const size_t user_len = strlen(pName);
					const size_t pw_len = strlen(pPassword);
					uint   i;
					size_t p = 0;
					char   param[512];
					param[p++] = 0;
					for(i = 0; i < user_len; i++)
						param[p++] = pName[i];
					param[p++] = 0;
					for(i = 0; i < pw_len; i++)
						param[p++] = pPassword[i];
					temp_buf.Z().EncodeMime64(param, p);
					(cmd_buf = "AUTH PLAIN").Space().Cat(temp_buf);
					THROW(WriteLine(cmd_buf, &reply_buf));
					THROW(CheckReply(reply_buf, 235));
				}
				break;
			case authtLogin:
				{
					THROW(WriteLine(cmd_buf = "AUTH LOGIN", &reply_buf));
					THROW(CheckReply(reply_buf));
					cmd_buf.Z().EncodeMime64(pName, strlen(pName));
					THROW(WriteLine(cmd_buf, &reply_buf));
					THROW(CheckReply(reply_buf));
					cmd_buf.Z().EncodeMime64(pPassword, strlen(pPassword));
					THROW(WriteLine(cmd_buf, &reply_buf));
					THROW(CheckReply(reply_buf, 235));
				}
				break;
			/*
			case authtCramMD5:
				{
					uchar digest[16];
					char param[256], param_64[512], time_stamp[128];
					SString buf;
					MD5 md5;
					memzero(digest, sizeof(digest));
					memzero(param, sizeof(param));
					memzero(time_stamp, sizeof(time_stamp));
					md5.Init();
					md5.Update((uchar *)pPassword, strlen(pPassword));
					md5.Final(digest);
					sprintf(param, "%s %s", user, digest);
					encode64(param, strlen(param), param_64, sizeof(param_64), 0);
					THROW(SendCmd(SMTPCMD_AUTH, "CRAM-MD5", reply_buf));
					decode64(sstrchr(reply_buf, ' '), strlen(sstrchr(reply_buf, ' ')), time_stamp, 0);
					THROW(SendCmd(SMTPCMD_STRING, (const char *)param_64, reply_buf));
				}
				break;
			*/
			case authtPOP3:
				{
					ZDELETE(P_Pop3AuthSession);
					InetUrl pop3_url = Url;
					if(protocol == InetUrl::protSMTP)
						pop3_url.SetProtocol(InetUrl::protPOP3);
					else if(protocol == InetUrl::protSMTPS)
						pop3_url.SetProtocol(InetUrl::protPOP3S);
					else {
						const int InvalidMailProtocol = 0;
						assert(InvalidMailProtocol);
					}
					THROW_S(P_Pop3AuthSession = new SMailClient, SLERR_NOMEM);
					THROW(P_Pop3AuthSession->Connect(pop3_url));
					THROW(P_Pop3AuthSession->Auth(0, pName, pPassword));
				}
				break;
		}
	}
	CATCH
		ZDELETE(P_Pop3AuthSession);
		ok = 0;
	ENDCATCH
	return ok;
}

int SMailClient::ReadLine(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	size_t rd_size = 0;
	THROW_S(State & stConnected, SLERR_MAIL_NOTCONNECTED);
	THROW(So.RecvUntil(RdBuf.Z(), "\xD\xA", &rd_size));
	{
		assert(rd_size == RdBuf.GetAvailableSize());
		const char * p_reply = (const char *)RdBuf.GetBuf(RdBuf.GetRdOffs());
		rBuf.CatN(p_reply, rd_size).Chomp();
	}
	CATCHZOK
	return ok;
}

int SMailClient::WriteLine(const char * pLine, SString * pReply)
{
	int    ok = 1;
	size_t wr_size = 0;
	THROW_S(State & stConnected, SLERR_MAIL_NOTCONNECTED);
	(WrLineBuf = pLine).CRB();
	THROW(So.Send(WrLineBuf, WrLineBuf.Len(), &wr_size));
	if(pReply) {
		THROW(ReadLine(*pReply));
	}
	CATCHZOK
	return ok;
}

int SMailClient::WriteBlock(const void * pData, size_t dataSize)
{
	int    ok = 1;
	size_t wr_size = 0;
	if(pData && dataSize) {
		THROW_S(State & stConnected, SLERR_MAIL_NOTCONNECTED);
		THROW(So.Send(pData, dataSize, &wr_size));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/SString & FASTCALL SMailClient::Pop3_SkipReplyStatus(SString & rBuf)
{
	rBuf.Strip();
	if(rBuf.HasPrefix("+OK"))
		rBuf.ShiftLeft(3);
	else if(rBuf.HasPrefix("-ERR"))
		rBuf.ShiftLeft(4);
	return rBuf.Strip();
}

int SMailClient::Pop3_GetStat(long * pCount, long * pSize)
{
	int    ok = 1;
	SString cmd_buf, reply_buf;
	long   msgs_count = 0, total_size = 0;
	THROW(WriteLine((cmd_buf = "STAT"), &reply_buf));
	THROW(CheckReply(reply_buf));
	{
		const char * p = Pop3_SkipReplyStatus(reply_buf);
		const char * q = sstrchr(p, ' ');
		if(q) {
			msgs_count = atol(p);
			total_size = atol(q+1);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pCount, msgs_count);
	ASSIGN_PTR(pSize, total_size);
	return ok;
}

int SMailClient::Pop3_GetMsgSize(long msgN, long * pSize)
{
	int    ok = 1;
	SString cmd_buf, reply_buf;
	long   msg_size = 0;
	THROW(WriteLine((cmd_buf = "LIST").Space().Cat(msgN), &reply_buf));
	THROW(CheckReply(reply_buf));
	Pop3_SkipReplyStatus(reply_buf);
	{
		size_t p = 0;
		if(reply_buf.SearchChar(' ', &p)) {
			msg_size = atol(reply_buf.cptr()+p);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pSize, msg_size);
	return ok;
}

int SMailClient::Pop3_DeleteMsg(long msgN)
{
	int    ok = 1;
	SString cmd_buf, reply_buf;
	THROW(WriteLine((cmd_buf = "DELE").Space().Cat(msgN), &reply_buf));
	THROW(CheckReply(reply_buf));
	CATCHZOK
	return ok;
}
