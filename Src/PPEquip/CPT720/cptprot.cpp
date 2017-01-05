//
//
//
#include <cpt720\cptprot.h>

extern HRESULT CPT720ErrCode;
//
// Error codes defines
//
#define CPT720_ERR_INITCOMPORT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1000)
#define CPT720_ERR_NOHANDSHAKEACK MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1001)
#define CPT720_ERR_CLOSELINKFAULT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1002)
#define CPT720_ERR_NOTSOHSTXSYMB  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1003)
#define CPT720_ERR_NOREPLY        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1004)
#define CPT720_ERR_NAK            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1005)
#define CPT720_ERR_EOT            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1006)
#define CPT720_ERR_CANTOPENFILE   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1007)
#define CPT720_ERR_DBFWRFAULT     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1008)
#define CPT720_ERR_ERRCRTTBL      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1009)
#define CPT720_ERR_DBFOPENFLT     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1010)
//
// BhtFile
//
SLAPI BhtFile::BhtFile(const char * pFileName)
{
	Stream = 0;
	P_RecTmpl = 0;
	Init(pFileName);
}

SLAPI BhtFile::~BhtFile()
{
	if(Stream)
		fclose(Stream);
	delete P_RecTmpl;
}

int SLAPI BhtFile::Init(const char * pFileName)
{
	int ok = -1;
	if(Stream) {
		fclose(Stream);
		Stream = 0;
	}
	ZDELETE(P_RecTmpl);
	memset(Name, 0, sizeof(Name));
	NumRecs = 0;
	if(pFileName) {
		char line_buf[256];
		THROW_CPT(Stream = fopen(pFileName, "r"), CPT720_ERR_CANTOPENFILE);
		if(fgets(line_buf, sizeof(line_buf), Stream)) {
			CipherProtocol::ParseHeadingText(line_buf, Name, &NumRecs);
			strip(Name);
			if(Name[0] && NumRecs <= MAXBHTRECS) {
				THROW_CMEM(P_RecTmpl = new BhtRecord);
				THROW(P_RecTmpl->SetHeader(line_buf));
				ok = 1;
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	if(ok <= 0 && pFileName)
		Init(0);
	return ok;
}

int SLAPI BhtFile::InitRecord(BhtRecord * pRec)
{
	return P_RecTmpl ? pRec->Init(*P_RecTmpl) : 0;
}

int SLAPI BhtFile::EnumRecords(uint * pRecNo, BhtRecord * pRec)
{
	if(Stream) {
		char line_buf[256];
		rewind(Stream);
		for(uint i = 0; fgets(line_buf, sizeof(line_buf), Stream);)
			if(i++ != 0)
				if(i > *pRecNo) {
					pRec->SetBuf(strlen(line_buf), line_buf);
					(*pRecNo) = i;
					return 1;
				}
	}
	return -1;
}

//
//
//
static int SLAPI GetIntFromBuf(long * pVal, int fldLen, const char * pBuf)
{
	char val_buf[32];
	for(int i = 0; i < fldLen; i++)
		val_buf[i] = pBuf[i];
	val_buf[fldLen] = 0;
	*pVal = atol(val_buf);
	return 1;
}

static int SLAPI GetStrFromBuf(char * pStr, size_t fldLen, const char * pBuf)
{
	for(size_t i = 0; i < fldLen; i++)
		pStr[i] = pBuf[i];
	pStr[fldLen] = 0;
	trimright(pStr);
	return 1;
}

static int SLAPI PutIntToBuf(long val, size_t fldLen, char * pBuf)
{
	char tmp_buf[32];
	sprintf(tmp_buf, "%0*ld", (int)fldLen, val);
	strcpy(pBuf, tmp_buf);
	return (strlen(tmp_buf) > fldLen) ? 0 : 1;
}

static int SLAPI PutStrToBuf(const char * pStr, size_t fldLen, char * pBuf)
{
	size_t len = pStr ? strlen(pStr) : 0;
	len = MIN(len, fldLen);
	memcpy(pBuf, pStr, len);
	memset(pBuf+len, ' ', fldLen-len);
	return 1;
}

//
// BhtRecord
//
SLAPI BhtRecord::BhtRecord()
{
	Reset();
}

SLAPI BhtRecord::BhtRecord(const BhtRecord & s)
{
	Init(s);
}

int SLAPI BhtRecord::Init(const BhtRecord & s)
{
	Reset();
	for(uint i = 0; i < s.NumFlds; i++)
		AddFld(s.Lens[i]);
	return 1;
}

int SLAPI BhtRecord::Reset()
{
	NumFlds = 0;
	memset(Lens, 0, sizeof(Lens));
	memset(Buf, 0, sizeof(Buf));
	return 1;
}

int SLAPI BhtRecord::AddFld(uint len)
{
	if(NumFlds < 16 && len < 100) {
		Lens[NumFlds++] = len;
		return 1;
	}
	else
		return 0;
}

size_t SLAPI BhtRecord::FldOfs(uint fldNo) const
{
	size_t ofs = 0;
	if(fldNo < NumFlds)
		for(uint i = 0; i < fldNo; i++)
			ofs += Lens[i];
	return ofs;
}

int SLAPI BhtRecord::PutInt(uint fldNo, long val)
{
	if(fldNo < NumFlds)
		return PutIntToBuf(val, Lens[fldNo], Buf+FldOfs(fldNo));
	return 0;
}

int SLAPI BhtRecord::PutStr(uint fldNo, const char * pStr)
{
	if(fldNo < NumFlds)
		return PutStrToBuf(pStr, Lens[fldNo], Buf+FldOfs(fldNo));
	return 0;
}

int SLAPI BhtRecord::PutDbl(uint fldNo, uint prec, double val)
{
	if(fldNo < NumFlds) {
		char tmp_buf[32];
		realfmt(val, MKSFMTD(0, prec, 0), tmp_buf);
		return PutStrToBuf(tmp_buf, Lens[fldNo], Buf+FldOfs(fldNo));
	}
	return 0;
}

int SLAPI BhtRecord::GetInt(uint fldNo, long * pVal)
{
	if(fldNo < NumFlds) {
		char tmp_buf[128];
		memset(tmp_buf, 0, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		*pVal = atol(strip(tmp_buf));
		return 1;
	}
	return 0;
}

int SLAPI BhtRecord::GetStr(uint fldNo, char * pBuf, size_t bufLen)
{
	if(fldNo < NumFlds) {
		char tmp_buf[128];
		memset(tmp_buf, 0, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		strnzcpy(pBuf, strip(tmp_buf), bufLen);
		return 1;
	}
	return 0;
}

int SLAPI BhtRecord::GetDbl(uint fldNo, double * pVal)
{
	if(fldNo < NumFlds) {
		char tmp_buf[128];
		memset(tmp_buf, 0, sizeof(tmp_buf));
		memcpy(tmp_buf, Buf+FldOfs(fldNo), Lens[fldNo]);
		*pVal = atof(strip(tmp_buf));
		return 1;
	}
	return 0;
}

size_t SLAPI BhtRecord::GetBufLen() const
{
	size_t s = 0;
	for(uint i = 0; i < NumFlds; i++)
		s += Lens[i];
	return s;
}

const char * BhtRecord::GetBuf() const
{
	return Buf;
}

int SLAPI BhtRecord::GetHeader(const char * pFileName,
	uint numRecs, size_t * pDataLen, char * pBuf) const
{
	size_t p = 0;
	PutStrToBuf(pFileName, FNAMELEN, pBuf+p); p += FNAMELEN;
	PutIntToBuf(numRecs, NUMRECSLEN, pBuf+p); p += NUMRECSLEN;
	PutIntToBuf(NumFlds, NUMFLDSLEN, pBuf+p); p += NUMFLDSLEN;
	for(uint i = 0; i < NumFlds; i++) {
		PutIntToBuf(Lens[i], FLDLENLEN, pBuf+p);
		p += FLDLENLEN;
	}
	ASSIGN_PTR(pDataLen, p);
	return 1;
}

int SLAPI BhtRecord::SetHeader(const char * pBuf)
{
	long v = 0;

	Reset();
	GetIntFromBuf(&v, NUMFLDSLEN, pBuf+FNAMELEN+NUMRECSLEN);
	uint num_flds = (uint)v;
	for(uint i = 0; i < num_flds; i++) {
		GetIntFromBuf(&v, FLDLENLEN, pBuf+FNAMELEN+NUMRECSLEN+NUMFLDSLEN+i*FLDLENLEN);
		AddFld((uint)v);
	}
	return 1;
}

int SLAPI BhtRecord::SetBuf(size_t dataSize, const char * pBuf)
{
	memcpy(Buf, pBuf, dataSize);
	return 1;
}

//
// CipherProtocol
//
#define EOT 0x04 // End Of Transmission
#define ENQ 0x05 // Enquiry
#define ACK 0x06 // Acknowledge
#define NAK 0x15 // Negative Acknowledge

#define SOH 0x01 // Start Of Heading
#define STX 0x02 // Start Of Text
#define ETX 0x03 // End Of Text

#define EOS  0x0d      // End of string

static int SLAPI _StoreDataBlock(size_t dataLen, const char * pBuf, FILE * out)
{
	if(out) {
		for(size_t i = 0; i < dataLen; i++)
			fputc(pBuf[i], out);
		fputc('\n', out);
	}
	return 1;
}

SLAPI CipherProtocol::CipherProtocol()
{
	Timeout = 3000;
	MaxTries = 10;
	Flags = 0;
}

SLAPI CipherProtocol::~CipherProtocol()
{
}


int SLAPI CipherProtocol::SendDataHeadingText(const char * pFileName,
	uint numRecs, const BhtRecord * pStruc)
{
	char buf[64];
	size_t p = 0;
	pStruc->GetHeader(pFileName, numRecs, &p, buf);
	return SendBlock(0, p, buf);
}

int SLAPI CipherProtocol::SendFile(const char * pFileName, const BhtRecord * pStruc, CallbackPrctFunc pf)
{
	int ok = 1;

	const size_t line_size = 256;
	char line_buf[line_size];
	uint numrecs = 0, recno = 0;
	char fname[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	char sending_info[256];
	FILE * stream = 0;
	BhtRecord * p_rec = 0;

	THROW_CPT(stream = fopen(pFileName, "r"), CPT720_ERR_CANTOPENFILE);
	THROW_CMEM(p_rec = new BhtRecord(*pStruc));
	while(fgets(line_buf, sizeof(line_buf), stream))
		numrecs++;
	fnsplit(pFileName, 0, 0, nam, ext);
	sprintf(sending_info, "Loading %s%s", nam, ext);
	if(pf)
		pf(0, numrecs, sending_info, sizeof(sending_info));
	THROW(SetConnection());
	fnmerge(fname, 0, 0, nam, ext);
	THROW(SendDataHeadingText(strupr(fname), numrecs, pStruc) > 0);
	rewind(stream);
	while(fgets(line_buf, sizeof(line_buf), stream)) {
		recno++;
		chomp(line_buf);
		p_rec->SetBuf(strlen(line_buf), line_buf);
		THROW(SendRecord(recno, p_rec) > 0);
		if(pf)
			pf(recno, numrecs, sending_info, sizeof(sending_info));
	}
	THROW(ReleaseConnection());
	CATCH
		ok = 0;
	ENDCATCH
	delete p_rec;
	if(stream)
		fclose(stream);
	return ok;
}

int SLAPI CipherProtocol::SendRecord(uint recNo, const BhtRecord * pRec)
{
	return SendBlock(recNo, pRec->GetBufLen(), pRec->GetBuf());
}

int SLAPI CipherProtocol::SendBlock(uint recNo, size_t size, const char * pBlk)
{
	int ok = 0;

	char buf[512];
	uint sum = 0;
	size_t i, p = 0;
	int enq = 0, nak = 0, j;
	if(recNo > 0) {
		buf[p++] = STX;
		PutIntToBuf(recNo, RECNOLEN, buf+p); p += RECNOLEN;
	}
	else
		buf[p++] = SOH;
	for(i = 0; i < size; i++)
		buf[p++] = pBlk[i];
	for(i = 1; i < p; i++)
		sum += (uint)(uchar)buf[i];
	buf[p++] = (sum / 256 == EOS) ? 14 : sum / 256;
	buf[p++] = (sum % 256 == EOS) ? 14 : sum % 256;
	buf[p++] = EOS;
	for(j = 0; ok == 0 && j < MaxTries; j++) {
		nak = 0;
		if(!enq)
			for(i = 0; i < p; i++)
				THROW_CPT(PutChr(buf[i]), E_FAIL);
		switch(GetChrEx()) {
			case ACK: ok =  1; enq = 0; break;
			case NAK: ok =  0; nak = 1; break;
			case EOT: ok = (CPT720ErrCode = CPT720_ERR_EOT, -1); break;
			default:  ok =  0; enq = 1; delay(Timeout); break;
		}
	}
	if(enq) {
		PutChrEx(EOT);
		CPT720ErrCode = CPT720_ERR_NOREPLY;
	}
	if(nak) {
		ok = (CPT720ErrCode = CPT720_ERR_NAK, -1);
	}
	/*
	if(ok < 0)
		ReleaseConnection();
	*/
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

// static
int SLAPI CipherProtocol::ParseHeadingText(const char * pBuf, char * pFileName, uint * pNumRecs)
{
	char fname[64];
	long numrecs = 0;
	GetStrFromBuf(fname, FNAMELEN, pBuf);
	GetIntFromBuf(&numrecs, NUMRECSLEN, pBuf+FNAMELEN);
	strcpy(pFileName, fname);
	*pNumRecs = (uint)numrecs;
	return 1;
}

int SLAPI CipherProtocol::SetProtParams(uint timeout, uint maxTries, long flags)
{
	Timeout = timeout;
	MaxTries = maxTries;
	Flags = flags;
	return 1;
}

int SLAPI CipherProtocol::SetConnection()
{
	int ok = 0, c = 0;
	for(uint i = 0; !ok && i < MaxTries; i++) {
		PutChrEx(ENQ);
		if((c = GetChrEx()) == ACK)
			ok = 1;
		else
			delay(Timeout);
	}
	if(!ok) {
		PutChrEx(EOT);
		CPT720ErrCode = CPT720_ERR_NOHANDSHAKEACK;
	}
	return ok;
}

int SLAPI CipherProtocol::ReleaseConnection()
{
	int ok = 0, c;
	for(int i = 0; !ok && i < MaxTries; i++) {
		PutChrEx(EOT);
		if((c = GetChrEx()) == ACK)
			ok = 1;
		else
			delay(Timeout);
	}
	if(!ok)
		CPT720ErrCode = CPT720_ERR_CLOSELINKFAULT;
	return ok;
}

int SLAPI CipherProtocol::WaitOnConnection(int releaseLink, long timeout)
{
	int expected_chr = releaseLink ? EOT : ENQ;
	for(uint i = 0; i < 100; i++)
		if(GetChrEx() == expected_chr) {
			PutChrEx(ACK);
			return 1;
		}
		else {
			PutChrEx(NAK);
			delay((uint)(timeout / 100));
		}
	return 0;
}

int SLAPI CipherProtocol::ReceiveFile(const char * pFileName, long timeout)
{
	int ok = 1, r = 0;
	FILE * p_out = 0;
	
	THROW_CPT(p_out = fopen(pFileName, onecstr('w')), CPT720_ERR_CANTOPENFILE);
	if(WaitOnConnection(0, timeout)) {
		uint numrecs = 0, recno = 0, j;
		char buf[512], fname[32];
		size_t datalen = 0;
		THROW(r = ReceiveBlock(&recno, &datalen, buf, sizeof(buf)));
		if(r > 0) {
			_StoreDataBlock(datalen, buf, p_out);
			THROW(ParseHeadingText(buf, fname, &numrecs));
			for(j = 0; j < numrecs; j++) {
				THROW(ReceiveBlock(&recno, &datalen, buf, sizeof(buf)));
				_StoreDataBlock(datalen, buf, p_out);
			}
			THROW_CPT(WaitOnConnection(1, 1000L), CPT720_ERR_NOHANDSHAKEACK);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
	ENDCATCH
	if(p_out)
		fclose(p_out);
	if(ok == 0)
		::remove(pFileName);
	return ok;
}

int SLAPI CipherProtocol::ReceiveBlock(uint * pRecNo, size_t * pDataLen,
	char * pBuf, size_t bufLen)
{
	int  ok = 1, r = 0;
	uint recno = 0, sum = 0;
	char buf[512];
	char c, bcc = 0;
	size_t p = 0;

	for(int i = 0; !r && i < 100; i++) {
		delay(300);
		c = GetChr();
		if(c == SOH || c == STX) {
			if(c == STX) {
				char recno_buf[16];
				for(int i = 0; i < RECNOLEN; i++) {
		   			c = GetChr();
					THROW(c);
					recno_buf[i] = c;
					sum += (uint)(uchar)c;
				}
				recno_buf[RECNOLEN] = 0;
				recno = atoi(recno_buf);
				THROW(recno > 0);
			}
			c = GetChr();
			while(c != 0 && c != EOS) {
				buf[p++] = c;
				sum += (uint)(uchar)c;
				c = GetChr();
			}
			sum -= (uint)(uchar)buf[p-2] + (uint)(uchar)buf[p-1];
			THROW((uint)(uchar)buf[p-2] == ((sum / 256 == EOS) ? 14 : sum / 256));
			THROW((uint)(uchar)buf[p-1] == ((sum % 256 == EOS) ? 14 : sum % 256));
			buf[p-2] = '\0';
			p -= 2;
			PutChrEx(ACK);
			ASSIGN_PTR(pRecNo, recno);
			memcpy(pBuf, buf, MIN(p, bufLen));
			ASSIGN_PTR(pDataLen, p);
			r = 1;
		}
		else {
			PutChrEx(NAK);
			CPT720ErrCode = CPT720_ERR_NOTSOHSTXSYMB;
		}
	}
	THROW(r);
	CATCH
		ok = 0;
		PutChrEx(EOT);
	ENDCATCH
	return ok;
}

int SLAPI CipherProtocol::PutChrEx(int c)
{
	int ok = 1;
	THROW(PutChr(c));
	THROW(PutChr(EOS));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI CipherProtocol::GetChrEx()
{
	int c = 0;
	c = GetChr();
	if(c != 0)
		GetChr();
	return c;
}

//static
int SLAPI PutBhtRecToFile(const BhtRecord * pBhtRec, FILE * stream)
{
	size_t buf_len = pBhtRec->GetBufLen();
	const char * p_buf = pBhtRec->GetBuf();
	for(size_t i = 0; i < buf_len; i++)
		fputc(p_buf[i], stream);
	fputc('\n', stream);
	return 1;
}
