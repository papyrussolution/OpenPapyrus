#ifndef __CPTPROT_H
#define __CPTPROT_H

#include <slib.h>
#include <dbf.h>

//
// file names
//
#define CPT720_FILNAM_BILL   "PPBILL.DAT"
#define CPT720_FILNAM_BLINE  "PPBLINE.DAT"
#define CPT720_FILNAM_INVENT "PPINVENT.DAT"
#define CPT720_FILNAM_ILINE  "PPILINE.DAT"
#define CPT720_FILNAM_GOODS  "PPGOODS.DAT"
#define CPT720_FILNAM_SUPPL  "PPSUPPL.DAT"

//
//
//
#define DBF_BILL_NUMFLDS   3
#define DBF_BLINE_NUMFLDS  5
#define DBF_INVENT_NUMFLDS 2
#define DBF_ILINE_NUMFLDS  4

#define THROW_CPT(expr,val)  {if(!(expr)){CPT720ErrCode=val;goto __scatch;}}
#define THROW_CMEM(expr)     {if(!(expr)){CPT720ErrCode=E_OUTOFMEMORY;goto __scatch;}}

class BhtRecord {
public:
	SLAPI  BhtRecord();
	SLAPI  BhtRecord(const BhtRecord &);
	int    SLAPI Reset();
	int    SLAPI Init(const BhtRecord &);
	int    SLAPI AddFld(uint);
	int    SLAPI PutInt(uint fldNo, long);
	int    SLAPI PutStr(uint fldNo, const char *);
	int    SLAPI PutDbl(uint fldNo, uint prec, double);
	int    SLAPI GetInt(uint fldNo, long *);
	int    SLAPI GetStr(uint fldNo, char *, size_t);
	int    SLAPI GetDbl(uint fldNo, double *);
	size_t SLAPI FldOfs(uint fldNo) const;
	size_t SLAPI GetBufLen() const;
	const char * GetBuf() const;
	int    SLAPI GetHeader(const char * pFileName, uint numRecs,
		size_t * pDataLen, char * pBuf) const;
	int    SLAPI SetHeader(const char * pBuf);
	int    SLAPI SetBuf(size_t dataSize, const char * pBuf);
private:
	uint   NumFlds;
	uint   Lens[16];
	char   Buf[256];
};

class BhtFile {
public:
	SLAPI  BhtFile(const char * pFileName);
	SLAPI ~BhtFile();
	int    SLAPI Init(const char * pFileName);
	int    SLAPI InitRecord(BhtRecord *);
	int    SLAPI EnumRecords(uint * pRecNo, BhtRecord *);

	FILE * Stream;
	char   Name[32];
	uint   NumRecs;
	BhtRecord * P_RecTmpl;
};

#define FNAMELEN   12
#define NUMRECSLEN  5
#define RECNOLEN    5
#define NUMFLDSLEN  2
#define FLDLENLEN   2
#define MAXBHTRECS  32767

// Percent function
typedef void (*CallbackPrctFunc)(long , long, const char *, size_t);

class CipherProtocol : public SCommPort {
public:
	SLAPI  CipherProtocol();
	SLAPI  ~CipherProtocol();
	int    SLAPI SetProtParams(uint timeout, uint maxTries, long flags);
	//
	// Sender
	//
	int    SLAPI SetConnection();
	int    SLAPI ReleaseConnection();
	int    SLAPI SendRecord(uint recNo, const BhtRecord *);
	int    SLAPI SendDataHeadingText(const char * pFileName, uint numRecs, const BhtRecord *);

	//
	// Receiver
	//
	int    SLAPI WaitOnConnection(int releaseLink, long timeout);
	int    SLAPI ReceiveBlock(uint * pRecNo, size_t * pDataLen, char * pBuf, size_t bufLen);
	static int   SLAPI ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs);
	//
	// High level
	//
	int    SLAPI SendFile(const char * pFileName, const BhtRecord *, CallbackPrctFunc);
	int    SLAPI ReceiveFile(const char * pFileName, long timeout);
private:
	int    SLAPI SendBlock(uint, size_t, const char * pBlk);
	int    SLAPI PutChrEx(int c);
	int    SLAPI GetChrEx();

	uint16 Timeout;
	uint16 MaxTries;
	long   Flags;
};

int SLAPI PutBhtRecToFile(const BhtRecord * pBhtRec, FILE * stream);

#endif