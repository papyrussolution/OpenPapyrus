// PPFTP.H
// Copyright (c) A.Starodub 2002, 2003, 2005
//
#ifndef __PPMAIL_H
#define __PPMAIL_H

#include <slib.h>
#include <snet.h>
#include <pp.h>

#define FTPCMD_USER 1
#define FTPCMD_PASS 2
#define FTPCMD_CWD  3
#define FTPCMD_QUIT 4
#define FTPCMD_RETR 5
#define FTPCMD_STOR 6
#define FTPCMD_STAT 7
#define FTPCMD_LIST 8
#define FTPCMD_DELE 9
#define FTPCMD_PWD  10
#define FTPCMD_TYPE 11
#define FTPCMD_PORT 12
#define FTPCMD_SITE 13
#define FTPCMD_MKD  14
#define FTPCMD_RMD  15
#define FTPCMD_SYST 16

#define FTPTRFRTYPE_ASCII 'A'
#define FTPTRFRTYPE_IMAGE 'I'
#define FTPTRFRTYPE_LOCAL 'L'

class PPFTP {
public:
	PPFTP() {P_ControlConn = 0;}
	~PPFTP() {Disconnect();}

	int SLAPI Connect(PPInternetAccount * pAccount);
	int SLAPI Disconnect();
	int SLAPI Get(const char * pLocalPath, const char * pServerPath, int checkDtTm = 0, PercentFunc pf = 0);
	int SLAPI Put(const char * pLocalPath, const char * pServerPath, int checkDtTm = 0, PercentFunc pf = 0);
	int SLAPI CD(const char * pNewDir);
	int SLAPI Delete(const char * pPath);
	int SLAPI MkDir(const char * pDir);
	int SLAPI RemoveDir(const char * pDir);
	int SLAPI GetFileInfo(const char * pServerPath, LDATETIME * pFileDtTm, ulong * pFileSize);
private:
	int    SLAPI SendPort(InetAddr * pAddr, char type);
	int    SLAPI SendCmd(int cmd, long extra, SString * pReply = 0);
	int    SLAPI GetReply(SString & aReply);
	int    SLAPI CheckReply(SString & aReply);
	int    SLAPI FileTransfer(const char * pLocalPath, const char * pServerPath, int send, int checkDtTm = 0, PercentFunc pf = 0);
	int    SLAPI GetOurAddr(InetAddr * pAddr);

	InetAddr OurAddr;
	TcpSocket * P_ControlConn;
	PPInternetAccount Account;
};

#endif