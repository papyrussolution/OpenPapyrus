// PPFTP.CPP
// Copyright (c) A. Starodub 2005, 2006, 2009, 2010, 2014, 2016
//
#include <pp.h>
#pragma hdrstop
#include <sys/stat.h>
#include <fcntl.h>
#include <ppftp.h>
#include <idea.h>
//
//
//
static void mkftpcmd(char * pBuf, const char * pCmd, int type, long addInfo)
{
	char * p = pBuf;
	p = stpcpy(pBuf, pCmd);
	if(type == 1) {
		*p++ = ' ';
		p = stpcpy(p, (const char *)addInfo);
	}
	else if(type == 2) 	{
		*p++ = ' ';
		p += strlen(ltoa(addInfo, p, 10));
	}
	else if(type == 3)
		p = stpcpy(p, (const char *)addInfo);
	*p++ = '\xD';
	*p++ = '\xA';
	*p = 0;
}

int SLAPI PPFTP::Connect(PPInternetAccount * pAccount)
{
	int    ok = 1;
	char   pwd[64];
	SString reply, temp_buf;
	InetAddr addr;
	OurAddr.Set(ADDR_ANY);
	THROW_PP(pAccount, PPERR_INVPARAM);
	Account = *pAccount;
	Account.GetExtField(FTPAEXSTR_HOST, temp_buf);
	addr.Set(temp_buf, Account.GetSendPort());
	THROW_MEM(P_ControlConn = new TcpSocket(Account.Timeout*1000, 1));
	THROW_SL(P_ControlConn->Connect(addr));
	THROW(GetReply(reply));
	THROW(CheckReply(reply));
	Account.GetExtField(FTPAEXSTR_USER, temp_buf);
	THROW(SendCmd(FTPCMD_USER, (long)(const char *)temp_buf));
	Account.GetPassword(pwd, sizeof(pwd), FTPAEXSTR_PASSWORD);
	THROW(SendCmd(FTPCMD_PASS, (long)pwd));
	THROW(GetOurAddr(&OurAddr));
	CATCH
		ok = (Disconnect(), 0);
	ENDCATCH
	return ok;
}

int SLAPI PPFTP::CD(const char * pNewDir)
{
	return SendCmd(FTPCMD_CWD, (long)pNewDir);
}

int SLAPI PPFTP::Delete(const char * pPath)
{
	int    ok = 1;
	char   drv[MAXDRIVE], dir[MAXDIR], name[MAXFILE + MAXEXT], ext[MAXEXT];
	SString reply;
	THROW_PP(pPath, PPERR_INVPARAM);
	fnsplit(pPath, drv, dir, name, ext);
	strcat(setLastSlash(name), ext);
	if(dir[0] != '\0')
		THROW(CD(dir));
	THROW(SendCmd(FTPCMD_DELE, (long)name, &reply) || reply.ToLong() == 550);
	ok = (reply.ToLong() == 550) ? -1 : ok;
	CATCHZOK
	return ok;
}

int SLAPI PPFTP::MkDir(const char * pDir)
{
	return SendCmd(FTPCMD_MKD, (long)pDir);
}

int SLAPI PPFTP::RemoveDir(const char * pDir)
{
	return SendCmd(FTPCMD_RMD, (long)pDir);
}

static void GetNextEntry(uint * pPos, StringSet * pSS, SString & rNextEntry)
{
	do {
		if(pSS->get(pPos, rNextEntry) <= 0)
			break;
	} while(rNextEntry.Len() == 0);
}

int SLAPI PPFTP::GetFileInfo(const char * pServerPath, LDATETIME * pFileDtTm, ulong * pFileSize)
{
	int    ok = 1;
	char   type_i[2];
	char   drv[MAXDRIVE], dir[MAXDIR], fname[MAXFILE + MAXEXT], ext[MAXEXT];
	SString reply;
	InetAddr addr;
	TcpSocket ftp_conn(Account.Timeout, 1);
	STRNSCPY(type_i, onecstr(FTPTRFRTYPE_IMAGE));
	THROW_PP(pServerPath, PPERR_INVPARAM);
	fnsplit(pServerPath, drv, dir, fname, ext);
	strcat(fname, ext);
	if(dir[0] != '\0')
		THROW(CD(dir));
	addr.Set(ADDR_ANY);
	THROW_SL(ftp_conn.Bind(addr));
	THROW_SL(ftp_conn.GetSockName(&addr, 0));
	addr.Set((ulong)OurAddr, addr.GetPort());
	THROW(SendPort(&addr, FTPTRFRTYPE_ASCII));
	THROW(SendCmd(FTPCMD_LIST, (long)(const char*)fname, &reply) || reply.ToLong() == 550);
	ok = (reply.ToLong() == 550) ? -1 : ok;
	if(ok > 0) {
		THROW_SL(ftp_conn.Listen());
		if(ftp_conn.Select(TcpSocket::mRead)) {
			InetAddr cli_addr;
			TcpSocket cli_sock;
			if(ftp_conn.Accept(&cli_sock, &cli_addr)) {
				char buf[1024];
				SString info;
				size_t len = 0;
				while(cli_sock.Recv(buf, sizeof(buf), &len) > 0 && len != 0)
					info.Cat(buf);
				cli_sock.Disconnect();
				{
					const char * mon[12] = {
						"Jan", "Feb", "Mar", "Apr", "May", "Jun",
						"Jul", "Aug", "Seb", "Oct", "Nov", "Dec"
					};

					int    d = 0, m = 0, y = 0;
					uint   p = 0;
					ulong  fsize = 0;
					LDATETIME f_dttm;
					StringSet ss(' ', info);
					GetNextEntry(&p, &ss, info);
					GetNextEntry(&p, &ss, info);
					GetNextEntry(&p, &ss, info);
					GetNextEntry(&p, &ss, info);
					GetNextEntry(&p, &ss, info);
					fsize = (ulong)info.ToLong();
					GetNextEntry(&p, &ss, info);
					for(int i = 0; i < 12; i++)
						if(info.CmpNC(mon[i]) == 0) {
							m = i + 1;
							break;
						}
					GetNextEntry(&p, &ss, info);
					d = (int)info.ToLong();
					GetNextEntry(&p, &ss, info);
					y = (int)info.ToLong();
					encodedate(d, m, y, &f_dttm.d);
					ASSIGN_PTR(pFileSize, fsize);
					ASSIGN_PTR(pFileDtTm, f_dttm);
				}
			}
			else
				ok = 0;
			THROW(GetReply(reply));
			THROW(CheckReply(reply));
		}
	}
	CATCHZOK
	SendCmd(FTPCMD_TYPE, (long)type_i);
	return ok;
}

int SLAPI PPFTP::Put(const char * pLocalPath, const char * pServerPath, int checkDtTm, PercentFunc pf)
{
	return (pLocalPath && pServerPath) ?
		FileTransfer(pLocalPath, pServerPath, 1, checkDtTm, pf) : PPSetError(PPERR_INVPARAM);
}

int SLAPI PPFTP::Get(const char * pLocalPath, const char * pServerPath, int checkDtTm, PercentFunc pf)
{
	return (pLocalPath && pServerPath) ?
		FileTransfer(pLocalPath, pServerPath, 0, checkDtTm, pf) : PPSetError(PPERR_INVPARAM);
}

int SLAPI PPFTP::FileTransfer(const char * pLocalPath, const char * pServerPath, int send, int checkDtTm, PercentFunc pf)
{
	int    ok = 1, not_exist_onftp = 0;
	char   drv[MAXDRIVE], dir[MAXDIR], fname[MAXFILE + MAXEXT], ext[MAXEXT];
	ulong  ftp_file_size = 0;
	LDATETIME ftp_file_dttm;
	FILE * p_file = 0;
	SString reply;
	InetAddr addr;
	TcpSocket ftp_conn(Account.Timeout, 1);

	fnsplit(pServerPath, drv, dir, fname, ext);
	strcat(fname, ext);
	if(dir[0] != '\0')
		THROW(CD(dir));

	THROW(not_exist_onftp = (GetFileInfo(fname, &ftp_file_dttm, &ftp_file_size) <= 0));
	addr.Set(ADDR_ANY);
	THROW_SL(ftp_conn.Bind(addr));
	THROW_SL(ftp_conn.GetSockName(&addr, 0));

	addr.Set((ulong)OurAddr, addr.GetPort());
	THROW(SendPort(&addr, FTPTRFRTYPE_IMAGE));
	if(send) {
		THROW(SendCmd(FTPCMD_STOR, (long)fname));
	}
	else {
		THROW(SendCmd(FTPCMD_RETR, (long)fname, &reply) || reply.ToLong() == 550)
		ok = (reply.ToLong() == 550) ? -1 : ok;
	}
	if(ok > 0) {
		InetAddr cli_addr;
		TcpSocket cli_sock;
		THROW_SL(ftp_conn.Listen());
		if(ftp_conn.Select(TcpSocket::mRead)) {
			if(ftp_conn.Accept(&cli_sock, &cli_addr)) {
				char   buf[1024];
				SString msg_buf;
				size_t len = 0;
				long c_len = 0, t_len = 0;
				if(send) {
					SDirEntry entry;
					THROW(GetFileStat(pLocalPath, &entry));
					t_len = (long)entry.Size; // @64
					PPLoadText(PPTXT_PUTFILETOFTP, msg_buf);
					SLS.SetError(SLERR_OPENFAULT, pLocalPath);
					THROW_SL((p_file = fopen(pLocalPath, "rb")) != NULL);
					while((len = fread(buf, 1, sizeof(buf), p_file))) {
						c_len += len;
						THROW_SL(cli_sock.Send(buf, len, 0));
						if(pf)
							pf(c_len, t_len, msg_buf, 0);
					}
				}
				else {
					t_len = ftp_file_size;
					PPLoadText(PPTXT_GETFILEFROMFTP, msg_buf);
					SLS.SetError(SLERR_OPENFAULT, pLocalPath);
					THROW_SL((p_file = fopen(pLocalPath, "wb")) != NULL);
					while(cli_sock.Recv(buf, sizeof(buf), &len) > 0 && len != 0) {
						c_len += len;
						fwrite(buf, 1, len, p_file);
						if(pf)
							pf(c_len, t_len, msg_buf, 0);
					}
					fclose(p_file);
					if(not_exist_onftp != -1) {
						HANDLE srchdl = ::CreateFile(pLocalPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); // @unicodeproblem
						SLS.SetError(SLERR_OPENFAULT, pLocalPath);
						THROW_SL(srchdl >= 0);
						SFile::SetTime((int)srchdl, &ftp_file_dttm, &ftp_file_dttm, &ftp_file_dttm);
						if(srchdl > 0)
							CloseHandle(srchdl);
					}
				}
				cli_sock.Disconnect();
			}
		}
		else
			ok = 0;
		THROW(GetReply(reply));
		THROW(CheckReply(reply));
	}
	CATCHZOK
	SFile::ZClose(&p_file);
	return ok;
}

int SLAPI PPFTP::SendPort(InetAddr * pAddr, char aType)
{
	int    ok = 1;
	char   type[2];
	SString str_addr;
	THROW_PP(pAddr, PPERR_INVPARAM);
	type[0] = aType;
	type[1] = 0;
	pAddr->ToStr(InetAddr::fmtAddr, str_addr);
	str_addr.ReplaceChar('.', ',');
	str_addr.Comma().Cat((long)(pAddr->GetPort() & 0x00ff)).Comma();
	str_addr.Cat((long)((pAddr->GetPort() >> 8) & 0x00ff));
	SendCmd(FTPCMD_TYPE, (long)type);
	SendCmd(FTPCMD_PORT, (long)(const char*)str_addr);
	CATCHZOK
	return ok;
}

int SLAPI PPFTP::SendCmd(int cmd, long extra, SString * pReply)
{
	int    ok = 1;
	SString reply_buf;
	if(P_ControlConn) {
		char buf[512];
		memzero(buf, sizeof(buf));
		switch(cmd) {
			case FTPCMD_USER:
				mkftpcmd(buf, "USER", 1, extra);
				break;
			case FTPCMD_PASS:
				mkftpcmd(buf, "PASS", 1, extra);
				break;
			case FTPCMD_CWD:
				mkftpcmd(buf, "CWD", 1, extra);
				break;
			case FTPCMD_QUIT:
				mkftpcmd(buf, "QUIT", 0, extra);
				break;
			case FTPCMD_RETR:
				mkftpcmd(buf, "RETR", 1, extra);
				break;
			case FTPCMD_STOR:
				mkftpcmd(buf, "STOR", 1, extra);
				break;
			case FTPCMD_DELE:
				mkftpcmd(buf, "DELE", 1, extra);
				break;
			case FTPCMD_STAT:
				if(extra)
					mkftpcmd(buf, "STAT", 1, extra);
				else
					mkftpcmd(buf, "STAT", 0, extra);
				break;
			case FTPCMD_LIST:
				if(extra)
					mkftpcmd(buf, "LIST", 1, extra);
				else
					mkftpcmd(buf, "LIST", 0, extra);
				break;
			case FTPCMD_PWD:
				mkftpcmd(buf, "PWD", 0, extra);
				break;
			case FTPCMD_TYPE:
				mkftpcmd(buf, "TYPE", 1, extra);
				break;
			case FTPCMD_PORT:
				mkftpcmd(buf, "PORT", 1, extra);
				break;
			case FTPCMD_SITE:
				mkftpcmd(buf, "SITE", 1, extra);
				break;
			case FTPCMD_MKD:
				mkftpcmd(buf, "MKD", 1, extra);
				break;
			case FTPCMD_RMD:
				mkftpcmd(buf, "RMD", 1, extra);
				break;
			case FTPCMD_SYST:
				mkftpcmd(buf, "SYST", 0, extra);
				break;
			default:
				ok = -1;
		}
		if(ok > 0) {
			THROW_SL(P_ControlConn->Send(buf, strlen(buf), 0));
			THROW(GetReply(reply_buf));
			THROW(CheckReply(reply_buf));
		}
	}
	else
		ok = -1;
	CATCHZOK
	if(pReply)
		*pReply = reply_buf;
	return ok;
}

int SLAPI PPFTP::GetOurAddr(InetAddr * pAddr)
{
	int    ok = 0;
	SString reply;
	if((ok = SendCmd(FTPCMD_STAT, 0, &reply)) > 0) {
		uint   p = 0;
		SString port;
		StringSet ss("\r\n");
		ss.setBuf(reply, reply.Len() + 1);
		ss.get(&p, reply);
		ss.get(&p, reply);
		//211- (195.218.227.121:1162 <-> 217.107.182.3:21)
		p = 0;
		ss.setDelim(" ");
		ss.setBuf(reply, reply.Len() + 1);
		ss.get(&p, reply);
		ss.get(&p, reply);
		p = 0;
		ss.setDelim(":");
		ss.setBuf(reply, reply.Len() + 1);
		ss.get(&p, reply);
		ss.get(&p, port);
		reply.ShiftLeft();
		OurAddr.Set(reply, (int)port.ToLong());
		ok = 1;
	}
	return ok;
}

int SLAPI PPFTP::GetReply(SString & aReply)
{
	int    ok = -1;
	if(P_ControlConn) {
		char buf[512];
		memzero(buf, sizeof(buf));
		THROW_SL(P_ControlConn->Recv(buf, sizeof(buf), 0) >= 0);
		aReply.CopyFrom(chomp(buf));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPFTP::CheckReply(SString & aReply)
{
	int    ok = -1;
	int    reply = aReply.ToLong();
	if(reply == 150 || reply == 200 || reply == 211 || reply == 215 || reply == 220 || reply == 221 ||
		reply == 226  || reply == 230 || reply == 250 || reply == 257 || reply == 331)
		ok = 1;
	else
		return PPSetError(PPERR_FTPSRVREPLYERR, aReply);
	return ok;
}

int SLAPI PPFTP::Disconnect()
{
	if(P_ControlConn) {
		P_ControlConn->Disconnect();
		ZDELETE(P_ControlConn);
	}
	OurAddr.Set(ADDR_ANY);
	return 1;
}
