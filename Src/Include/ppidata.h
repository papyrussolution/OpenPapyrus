// PPYIDATA.H
// Copyright (c) A.Starodub 2002, 2003, 2007
//
#ifndef __PPIDATA_H
#define __PPIDATA_H

#include <slib.h>

#ifdef __WIN32__

#include <wininet.h>

#define PPDWNLDATA_MAXTAGVALSIZE   256
#define PPDWNLDATA_MAXTAGSIZE      64
#define PPDWNLDATA_MAXTAGNAMESSIZE 2048
//
// CurrListTagParser
//
typedef TSArray <PPCurrency> CurrencyArray;

class CurrListTagParser : XTagParser {
public:
	SLAPI  CurrListTagParser();
	SLAPI ~CurrListTagParser();
	int    SLAPI ProcessNext(CurrencyArray * pCurrAry, const char * pPath);
protected:
	virtual int SLAPI ProcessTag(const char * pTag, long);
private:
	int    SLAPI SaveTagVal(const char * pTag);
	CurrencyArray CurrAry;
	PPCurrency CurrItem;
	PPIDArray ParentTags;
	SString TagValBuf;
	SString TagNamesStr;
};
//
// PpyInetDataPrcssr
//
const static char WinInetDLLPath[] = "wininet.dll";

#endif // __WIN32__

class PpyInetDataPrcssr {
public:
	SLAPI  PpyInetDataPrcssr();
	SLAPI ~PpyInetDataPrcssr();
#ifdef __WIN32__
	int    SLAPI Init();
	void   SLAPI Uninit();
	int    SLAPI ImportCurrencyList(ulong * pAcceptedRows, int use_ta);
	//int  SLAPI ImportBankList();
#endif // __WIN32__
	static int SLAPI EditCfg();
	static int SLAPI GetCfg(PPInetConnConfig * pCfg);
	static int SLAPI PutCfg(const PPInetConnConfig * pCfg, int use_ta);
protected:
#ifdef __WIN32__
	int    SLAPI DownloadData(const char * pURL, const char * pPath);
	void   SLAPI SetInetError();
	HINTERNET InetSession;
	HANDLE    WinInetDLLHandle;
	PPInetConnConfig IConnCfg;
#endif // __WIN32__
};
//
// FTP via wininet.lib implementation
//
class WinInetFTP {
public:
	WinInetFTP();
	~WinInetFTP()
	{
		UnInit();
	}
	int    Init();
	int    Init(PPInetConnConfig * pCfg);
	int    UnInit();
	int    ReInit();
	int    Connect(PPInternetAccount * pAccount);
	int    Disconnect();
	int    Get(const char * pLocalPath, const char * pFTPPath, int checkDtTm = 0, PercentFunc pf = 0);
	int    Put(const char * pLocalPath, const char * pFTPPath, int checkDtTm = 0, PercentFunc pf = 0);
	int    Delete(const char * pPath);
	int    DeleteWOCD(const char * pPath);
	int    CD(const char * pDir, int isFullPath = 1);
	int    CreateDir(const char * pDir);
	int    CheckSizeAfterCopy(const char * pLocalPath, const char * pFTPPath);
	int    Exists(const char * pPath);
	int    GetFileList(const char * pDir, StrAssocArray * pFileList, const char * pMask = 0);
	int    SafeGet(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf, PPLogger * pLogger);
	int    SafePut(const char * pLocalPath, const char * pFTPPath, int checkDtTm, PercentFunc pf, PPLogger * pLogger);
	int    SafeCD(const char * pPath, int isFullPath, PPLogger * pLogger);
	int    SafeDelete(const char * pPath, PPLogger * pLogger);
	int    SafeDeleteWOCD(const char * pPath, PPLogger * pLogger);
	int    SafeCreateDir(const char * pDir, PPLogger * pLogger); // @5.9.4
	int    SafeGetFileList(const char * pDir, StrAssocArray * pFileList, const char * pMask, PPLogger * pLogger);
private:
	int    TransferFile(const char * pLocalPath, const char * pFTPPath, int send, int checkDtTm, PercentFunc pf);
	int    ReadResponse();

	HINTERNET InetSession, Connection;
	PPInetConnConfig IConnCfg;
	HANDLE    WinInetDLLHandle;
	PPInternetAccount Account;
};

#endif // __PPIDATA_H
