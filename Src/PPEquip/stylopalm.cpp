// STYLOPALM.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2015, 2018, 2019
//
#pragma hdrstop
#include <stylopalm.h>
#include <ctype.h>

const char * P_PalmConfigFileName     = "palmcfg.bin";
const char * P_StyloPalmCreatorID     = "SPII";
const char * P_PalmArcTblName         = "SpiiUcl.pdb";
const char * P_PalmPackedDataTblName  = "SpiiPD.pdb";
const char * P_PalmProgramFileName    = "StyloWce.exe";  // обновление файлов поддерживают только устройства с ОС Windows Mobile
extern const char * P_PalmDllFileName = "TodayItem.dll"; // обновление файлов поддерживают только устройства с ОС Windows Mobile

PalmConfig::PalmConfig()
{
	Init();
}

void PalmConfig::Init()
{
	THISZERO();
	Size = sizeof(*this);
	Ver = 7;
	SendBufSize = PALMPACKRECLEN;
	RecvBufSize = PALMARCBUFSIZE;
}
//
int PalmConfig::CompressData() const
{
	return BIN(Flags & CFGF_COMPRESSDATA);
}

int PalmConfig::PalmCompressedData() const
{
	return BIN(Flags & CFGF_PALMCOMPRESSEDDATA);
}

void PalmConfig::ToHostRec()
{
	Size    = ntohl(Size);
	Ver     = ntohl(Ver);
	InvCode = ntohl(InvCode);
	OrdCode = ntohl(OrdCode);
	Shift   = ntohl(Shift);
	Flags   = ntohl(Flags);
	TmLastXchg.d.v = (ulong)ntohl(TmLastXchg.d.v);
	TmClient.d.v   = (ulong)ntohl(TmClient.d.v);
	TmGoods.d.v    = (ulong)ntohl(TmGoods.d.v);
	TmCliDebt.d.v  = (ulong)ntohl(TmCliDebt.d.v);
	TmCliSell.d.v  = (ulong)ntohl(TmCliSell.d.v);
	TmToDo.d.v     = (ulong)ntohl(TmToDo.d.v);
	/*
	TmLastXchg.t.v = (ulong)ntohl(TmLastXchg.t.v);
	TmClient.t.v   = (ulong)ntohl(TmClient.t.v);
	TmGoods.t.v    = (ulong)ntohl(TmGoods.t.v);
	TmCliDebt.t.v  = (ulong)ntohl(TmCliDebt.t.v);
	TmCliSell.t.v  = (ulong)ntohl(TmCliSell.t.v);
	TmToDo.t.v     = (ulong)ntohl(TmToDo.t.v);
	*/
	RecvBufSize = ntohl(RecvBufSize);
	SendBufSize = ntohl(SendBufSize);
}

void PalmConfig::ToPalmRec()
{
	Size    = htonl(Size);
	Ver     = htonl(Ver);
	InvCode = htonl(InvCode);
	OrdCode = htonl(OrdCode);
	Shift   = htonl(Shift);
	Flags   = htonl(Flags);
	TmLastXchg.d.v = htonl(TmLastXchg.d.v);
	TmClient.d.v   = htonl(TmClient.d.v);
	TmGoods.d.v    = htonl(TmGoods.d.v);
	TmCliDebt.d.v  = htonl(TmCliDebt.d.v);
	TmCliSell.d.v  = htonl(TmCliSell.d.v);
	TmToDo.d.v     = htonl(TmToDo.d.v);
	/*
	TmLastXchg.t.v = htonl(TmLastXchg.t.v);
	TmClient.t.v   = htonl(TmClient.t.v);
	TmGoods.t.v    = htonl(TmGoods.t.v);
	TmCliDebt.t.v  = htonl(TmCliDebt.t.v);
	TmCliSell.t.v  = htonl(TmCliSell.t.v);
	TmToDo.t.v     = htonl(TmToDo.t.v);
	*/
	SendBufSize    = htonl(SendBufSize);
	RecvBufSize    = htonl(RecvBufSize);
}

int PalmConfig::ToBuf(void * pBuf, size_t * pBufSize)
{
	int ok = 0;
	if(pBufSize) {
		if(*pBufSize < 96) {
			*pBufSize = 96;
			ok = -1;
		}
		else {
			char * p_buf = (char *)pBuf;
			size_t bytes = 0;
			memcpy(p_buf + bytes,                             &Size,           sizeof(Size));
			memcpy(p_buf + (bytes += sizeof(Size)),           &Ver,            sizeof(Ver));
			memcpy(p_buf + (bytes += sizeof(Ver)),            &InvCode,        sizeof(InvCode));
			memcpy(p_buf + (bytes += sizeof(InvCode)),        &OrdCode,        sizeof(OrdCode));
			memcpy(p_buf + (bytes += sizeof(OrdCode)),        &Shift,          sizeof(Shift));
			memcpy(p_buf + (bytes += sizeof(Shift)),          &Flags,          sizeof(Flags));
			memcpy(p_buf + (bytes += sizeof(Flags)),          &TmLastXchg,     sizeof(TmLastXchg));
			memcpy(p_buf + (bytes += sizeof(TmLastXchg)),     &TmClient,       sizeof(TmClient));
			memcpy(p_buf + (bytes += sizeof(TmClient)),       &TmGoods,        sizeof(TmGoods));
			memcpy(p_buf + (bytes += sizeof(TmGoods)),        &TmCliDebt,      sizeof(TmCliDebt));
			memcpy(p_buf + (bytes += sizeof(TmCliDebt)),      &TmCliSell,      sizeof(TmCliSell));
			memcpy(p_buf + (bytes += sizeof(TmCliSell)),      &GTblViewColsID, sizeof(GTblViewColsID));
			memcpy(p_buf + (bytes += sizeof(GTblViewColsID)), &TmToDo,         sizeof(TmToDo));
			memcpy(p_buf + (bytes += sizeof(TmToDo)),         &NumSellWeeks,   sizeof(NumSellWeeks));
			memcpy(p_buf + (bytes += sizeof(NumSellWeeks)),   &CardNo,         sizeof(CardNo));
			memcpy(p_buf + (bytes += sizeof(CardNo)),         &ActiveNetCfgID, sizeof(ActiveNetCfgID));
			memcpy(p_buf + (bytes += sizeof(ActiveNetCfgID)), &SendBufSize,    sizeof(SendBufSize));
			memcpy(p_buf + (bytes += sizeof(SendBufSize)),    &RecvBufSize,    sizeof(RecvBufSize));
			memcpy(p_buf + (bytes += sizeof(RecvBufSize)),    &MaxNotSentOrd,  sizeof(MaxNotSentOrd));
			memcpy(p_buf + (bytes += sizeof(MaxNotSentOrd)),  &Reserve2,       sizeof(Reserve2));
			*pBufSize = 96;
			ok = 1;
		}
	}
	return ok;
}

int PalmConfig::FromBuf(const void * pBuf)
{
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&Size,           p_buf + bytes,                             sizeof(Size));
	memcpy(&Ver,            p_buf + (bytes += sizeof(Size)),           sizeof(Ver));
	memcpy(&InvCode,        p_buf + (bytes += sizeof(Ver)),            sizeof(InvCode));
	memcpy(&OrdCode,        p_buf + (bytes += sizeof(InvCode)),        sizeof(OrdCode));
	memcpy(&Shift,          p_buf + (bytes += sizeof(OrdCode)),        sizeof(Shift));
	memcpy(&Flags,          p_buf + (bytes += sizeof(Shift)),          sizeof(Flags));
	memcpy(&TmLastXchg,     p_buf + (bytes += sizeof(Flags)),          sizeof(TmLastXchg));
	memcpy(&TmClient,       p_buf + (bytes += sizeof(TmLastXchg)),     sizeof(TmClient));
	memcpy(&TmGoods,        p_buf + (bytes += sizeof(TmClient)),       sizeof(TmGoods));
	memcpy(&TmCliDebt,      p_buf + (bytes += sizeof(TmGoods)),        sizeof(TmCliDebt));
	memcpy(&TmCliSell,      p_buf + (bytes += sizeof(TmCliDebt)),      sizeof(TmCliSell));
	memcpy(&GTblViewColsID, p_buf + (bytes += sizeof(TmCliSell)),      sizeof(GTblViewColsID));
	memcpy(&TmToDo,         p_buf + (bytes += sizeof(GTblViewColsID)), sizeof(TmToDo));
	memcpy(&NumSellWeeks,   p_buf + (bytes += sizeof(TmToDo)),         sizeof(NumSellWeeks));
	memcpy(&CardNo,         p_buf + (bytes += sizeof(NumSellWeeks)),   sizeof(CardNo));
	memcpy(&ActiveNetCfgID, p_buf + (bytes += sizeof(CardNo)),         sizeof(ActiveNetCfgID));
	memcpy(&SendBufSize,    p_buf + (bytes += sizeof(ActiveNetCfgID)), sizeof(SendBufSize));
	memcpy(&RecvBufSize,    p_buf + (bytes += sizeof(SendBufSize)),    sizeof(RecvBufSize));
	memcpy(&MaxNotSentOrd,  p_buf + (bytes += sizeof(RecvBufSize)),    sizeof(MaxNotSentOrd));
	memcpy(&Reserve2,       p_buf + (bytes += sizeof(MaxNotSentOrd)),  sizeof(Reserve2));
	return 1;
}

void PalmConfig::SetupBuffers(uint32 recvBufSize, uint32 sendBufSize)
{
	RecvBufSize = (recvBufSize >= MIN_RECVBUFSIZE && recvBufSize <= PALMARCBUFSIZE) ? recvBufSize : PALMARCBUFSIZE;
	SendBufSize = (sendBufSize >= MIN_SENDBUFSIZE && sendBufSize <= PALMPACKRECLEN) ? sendBufSize : PALMPACKRECLEN;
}
//
int PalmConfig::Write(const char * pPath)
{
	int    ok = 1;
#if !defined(__palmos__) && !defined(_WIN32_WCE) // {
	SString fname;
	SPathStruc ps(pPath);
	if(ps.Ext.Len())
		fname = pPath;
	else {
		ps.Nam = P_PalmConfigFileName;
		ps.Merge(fname);
	}
	FILE * f = fopen(fname, "wb");
	if(f) {
		if(fwrite(this, sizeof(*this), 1, f) != 1)
			ok = 0;
		SFile::ZClose(&f);
	}
	else
		ok = 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return ok;
}

int PalmConfig::Read(const char * pPath)
{
	int    ok = 1;
#if !defined(__palmos__) &&  !defined(_WIN32_WCE) // {
	SString fname;
	SPathStruc ps(pPath);
	if(ps.Ext.Len())
		fname = pPath;
	else
		(fname = pPath).SetLastSlash().Cat(P_PalmConfigFileName);
	FILE * f = fopen(fname, "rb");
	if(f) {
		if(fread(this, sizeof(*this), 1, f) != 1)
			ok = 0;
		SFile::ZClose(&f);
	}
	else
		ok = 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return ok;
}
//
//
//
int PalmArcHdr::Check() const
{
	if(Ver != 1)
		return 0;
	if(NumRecs > PALM_MAXRECCOUNT)
		return 0;
	size_t i;
	for(i = 0; i < sizeof(Reserve); i++)
		if(Reserve[i] != 0)
			return 0;
#if !defined(__palmos__)  && !defined(_WIN32_WCE) // {
	const size_t len = sstrlen(Name);
	for(i = 0; i < len; i++)
		if(!isalpha(Name[i]) && Name[i] != '.' && Name[i] != '_')
			return 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return 1;
}
//
//
//
void PalmArcHdr::ToPalmRec()
{
	Ver     = htons(Ver);
	NumRecs = htonl(NumRecs);
}

void PalmArcHdr::ToHostRec()
{
	Ver     = ntohs(Ver);
	NumRecs = ntohl(NumRecs);
}

int PalmArcHdr::ToBuf(void * pBuf, size_t * pBufSize)
{
	int ok = 0;
	if(pBufSize) {
		if(*pBufSize < 64) {
			*pBufSize = 64;
			ok = -1;
		}
		else {
			char * p_buf = (char *)pBuf;
			size_t bytes = 0;
			memcpy(p_buf,                               &Ver,     sizeof(Ver));
			memcpy(p_buf + (bytes += sizeof(Ver)),      &NumRecs, sizeof(NumRecs));
			memcpy(p_buf + (bytes += sizeof(NumRecs)),  Reserve,  sizeof(Reserve));
			memcpy(p_buf + (bytes += sizeof(Reserve)),  Name,     sizeof(Name));
			*pBufSize = 64;
			ok = 1;
		}
	}
	return ok;
}

int PalmArcHdr::FromBuf(const void * pBuf)
{
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&Ver,     p_buf,                              sizeof(Ver));
	memcpy(&NumRecs, p_buf + (bytes += sizeof(Ver)),     sizeof(NumRecs));
	memcpy(Reserve,  p_buf + (bytes += sizeof(NumRecs)), sizeof(Reserve));
	memcpy(Name,     p_buf + (bytes += sizeof(Reserve)), sizeof(Name));
	return 1;
}

SpiiCmdBuf::SpiiCmdBuf()
{
	memzero(this, sizeof(SpiiCmdBuf));
}

#ifndef __palmos__
void SpiiCmdBuf::ToPalmRec()
{
	Cmd     = htons(Cmd);
	BufSize = htonl(BufSize);
	RetCode = htonl(RetCode);
}

void SpiiCmdBuf::ToHostRec()
{
	Cmd     = ntohs(Cmd);
	BufSize = ntohl(BufSize);
	RetCode = ntohl(RetCode);
}

SString & SpiiCmdBuf::ToStr(SString & rBuf) const
{
	return rBuf.Z().Cat((long)Cmd).CatDiv(';', 0).Cat(RetCode).CatDiv(';', 0).
		Cat(Hdl[0]).CatChar('-').Cat(Hdl[1]).CatDiv(';', 0).Cat(BufSize);
}
#endif

SpGoodsStruc::SpGoodsStruc() : P_Quots(0)
{
	Init();
}

SpGoodsStruc::~SpGoodsStruc() 
{
	ZDELETE(P_Quots);
}
	
void SpGoodsStruc::ToHostRec()
{
	ID           = ntohl(ID);
	GoodsGrpID   = ntohl(GoodsGrpID);
	Pack         = ntohl(Pack);
	Price        = ntohl(Price);
	Rest         = ntohl(Rest);
	BrandID      = ntohl(BrandID);
	BrandOwnerID = ntohl(BrandOwnerID);
	MinOrd       = ntohl(MinOrd);
	// MultMinOrd   = ntohs(MultMinOrd);
	QuotCount    = ntohs(QuotCount);
	for(int16 i = 0; i < QuotCount; i++) {
		P_Quots[i].QuotKindID = ntohl(P_Quots[i].QuotKindID);
		P_Quots[i].Price      = ntohl(P_Quots[i].Price);
	}
}

int SpGoodsStruc::FromBuf(const void * pBuf)
{
	int    ok = 1;
	const char * p_buf = static_cast<const char*>(pBuf);
	size_t bytes = 0;
	memcpy(&ID,           p_buf,                                   sizeof(ID));
	memcpy(Code,          p_buf + (bytes += sizeof(ID)),           sizeof(Code));
	memcpy(&GoodsGrpID,   p_buf + (bytes += sizeof(Code)),         sizeof(GoodsGrpID));
	memcpy(&Pack,         p_buf + (bytes += sizeof(GoodsGrpID)),   sizeof(Pack));
	memcpy(&Price,        p_buf + (bytes += sizeof(Pack)),         sizeof(Price));
	memcpy(&Rest,         p_buf + (bytes += sizeof(Price)),        sizeof(Rest));
	memcpy(Name,          p_buf + (bytes += sizeof(Rest)),         sizeof(Name));
	memcpy(&BrandID,      p_buf + (bytes += sizeof(Name)),         sizeof(BrandID));
	memcpy(&BrandOwnerID, p_buf + (bytes += sizeof(BrandID)),      sizeof(BrandOwnerID));
	memcpy(&MinOrd,       p_buf + (bytes += sizeof(BrandOwnerID)), sizeof(MinOrd));
	memcpy(&MultMinOrd,   p_buf + (bytes += sizeof(MinOrd)),       sizeof(MultMinOrd));
	memcpy(&QuotCount,    p_buf + (bytes += sizeof(MultMinOrd)),   sizeof(QuotCount));
	QuotCount = ntohs(QuotCount);
	bytes += sizeof(QuotCount);
	ZDELETE(P_Quots);
	if(QuotCount) {
		size_t quot_size = 8;
		THROW(P_Quots = new Quot[QuotCount * sizeof(Quot)]);
		for(int16 i = 0; i < QuotCount; i++) {
			Quot * p_q = &P_Quots[i];
			memcpy(&p_q->QuotKindID, p_buf + bytes,                           sizeof(p_q->QuotKindID));
			memcpy(&p_q->Price,      p_buf + bytes + sizeof(p_q->QuotKindID), sizeof(p_q->Price));
			bytes += quot_size;
		}
	}
	CATCHZOK
	QuotCount = htons(QuotCount);
	return ok;
}
	
void SpGoodsStruc::Init()
{
	ID = GoodsGrpID = Pack = Price = Rest = BrandID = BrandOwnerID = MinOrd = 0;
	MultMinOrd = 0;
	QuotCount = 0;
	ZDELETE(P_Quots);
	memzero(Code, sizeof(Code));
	memzero(Name, sizeof(Name));
}

SpBrandStruc::SpBrandStruc() 
{
	Init();
}

void SpBrandStruc::ToHostRec()
{
	ID      = ntohl(ID);
	OwnerID = ntohl(OwnerID);
}

int SpBrandStruc::FromBuf(const void * pBuf)
{
	int ok = 1;
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&ID,       p_buf,                              sizeof(ID));
	memcpy(&OwnerID,  p_buf + (bytes += sizeof(ID)),      sizeof(OwnerID));
	memcpy(Name,      p_buf + (bytes += sizeof(OwnerID)), sizeof(Name));
	memcpy(OwnerName, p_buf + (bytes += sizeof(Name)),    sizeof(OwnerName));
	return ok;
}

void SpBrandStruc::Init()
{
	ID = OwnerID = 0;
	memzero(Name,      sizeof(Name));
	memzero(OwnerName, sizeof(OwnerName));
}

SpLocStruc::SpLocStruc() 
{
	Init();
}

void SpLocStruc::ToHostRec()
{
	ID = ntohl(ID);
}

int SpLocStruc::FromBuf(const void * pBuf)
{
	int ok = 1;
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&ID,       p_buf,                         sizeof(ID));
	memcpy(Name,      p_buf + (bytes += sizeof(ID)), sizeof(Name));
	return ok;
}

void SpLocStruc::Init()
{
	ID = 0;
	memzero(Name, sizeof(Name));
}

SpGoodsGrpStruc::SpGoodsGrpStruc()
{
	Init();
}
	
void SpGoodsGrpStruc::ToHostRec()
{
	ID = ntohl(ID);
}
	
int SpGoodsGrpStruc::FromBuf(const void * pBuf)
{
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&ID,   p_buf, sizeof(ID));
	memcpy(&Code, p_buf + (bytes += sizeof(ID)), sizeof(Code));
	memcpy(Name,  p_buf + (bytes += sizeof(Code)), sizeof(Name));
	return 1;
}

void SpGoodsGrpStruc::Init()
{
	ID = 0;
	Code = 0;
	memzero(Name, sizeof(Name));
}

SpClientStruc::SpClientStruc() : P_Addrs(0)
{
	Init();
}

SpClientStruc::~SpClientStruc() 
{
	ZDELETE(P_Addrs);
}

void SpClientStruc::ToHostRec()
{
	ID         = ntohl(ID);
	QuotKindID = ntohl(QuotKindID);
	Debt       = ntohl(Debt);
	AddrCount  = ntohs(AddrCount);
	for(int16 i = 0; i < AddrCount; i++)
		P_Addrs[i].ID = ntohl(P_Addrs[i].ID);
	Flags = ntohl(Flags);
}

int SpClientStruc::FromBuf(const void * pBuf)
{
	int ok = 1;
	const char * p_buf = static_cast<const char *>(pBuf);
	size_t bytes = 0;
	memcpy(&ID,         p_buf,                                 sizeof(ID));
	memcpy(&QuotKindID, p_buf + (bytes += sizeof(ID)),         sizeof(QuotKindID));
	memcpy(&Debt,       p_buf + (bytes += sizeof(QuotKindID)), sizeof(Debt));
	memcpy(Code,        p_buf + (bytes += sizeof(Debt)),       sizeof(Code));
	memcpy(Name,        p_buf + (bytes += sizeof(Code)),       sizeof(Name));
	memcpy(&Flags,      p_buf + (bytes += sizeof(Name)),       sizeof(Flags));
	memcpy(&AddrCount,  p_buf + (bytes += sizeof(Flags)),      sizeof(AddrCount));
	AddrCount = ntohs(AddrCount);
	bytes += sizeof(AddrCount);
	ZDELETE(P_Addrs);
	if(AddrCount) {
		size_t addr_size = 68;
		THROW(P_Addrs = new Addr[AddrCount * sizeof(Addr)]);
		for(int16 i = 0; i < AddrCount; i++) {
			Addr * p_addr = &P_Addrs[i];
			memcpy(&p_addr->ID,  p_buf + bytes,                      sizeof(p_addr->ID));
			memcpy(&p_addr->Loc, p_buf + bytes + sizeof(p_addr->ID), sizeof(p_addr->Loc));
			bytes += addr_size;
		}
	}
	CATCHZOK
	AddrCount = htons(AddrCount);
	return ok;
}

void SpClientStruc::Init()
{
	ID = QuotKindID = Debt = Flags = 0;
	memzero(Code, sizeof(Code));
	memzero(Name, sizeof(Name));
	AddrCount = 0;
	ZDELETE(P_Addrs);
}

SpOrdHeaderStruc::SpOrdHeaderStruc() 
{
	Init();
}
	
void SpOrdHeaderStruc::Init()
{
	ID = ClientID = DlvrAddrID = QuotKindID = Amount = PctDis = 0;
	LocID = 0;
	Date = 0;
	memzero(Code, sizeof(Code));
	memzero(Memo, sizeof(Memo));
}
	
int SpOrdHeaderStruc::ToBuf(void * pBuf, size_t * pBufSize)
{
	int ok = 0;
	if(pBufSize) {
		if(*pBufSize < 200) {
			*pBufSize = 200;
			ok = -1;
		}
		else {
			char * p_buf = (char *)pBuf;
			size_t bytes = 0;
			memcpy(p_buf,                                 &ID,         sizeof(ID));
			memcpy(p_buf + (bytes += sizeof(ID)),         &Date,       sizeof(Date));
			memcpy(p_buf + (bytes += sizeof(Date)),       Code,        sizeof(Code));
			memcpy(p_buf + (bytes += sizeof(Code)),       &ClientID,   sizeof(ClientID));
			memcpy(p_buf + (bytes += sizeof(ClientID)),   &DlvrAddrID, sizeof(DlvrAddrID));
			memcpy(p_buf + (bytes += sizeof(DlvrAddrID)), &QuotKindID, sizeof(QuotKindID));
			memcpy(p_buf + (bytes += sizeof(QuotKindID)), &Amount,     sizeof(Amount));
			memcpy(p_buf + (bytes += sizeof(Amount)),     &PctDis,     sizeof(PctDis));
			memcpy(p_buf + (bytes += sizeof(PctDis)),     Memo,        sizeof(Memo));
			memcpy(p_buf + (bytes += sizeof(Memo)),       &LocID,      sizeof(LocID));
			*pBufSize = 200;
			ok = 1;
		}
	}
	return ok;
}
	
void SpOrdHeaderStruc::ToPalmRec()
{
	ID         = htonl(ID);
	Date       = htons(Date);
	ClientID   = htonl(ClientID);
	DlvrAddrID = htonl(DlvrAddrID);
	QuotKindID = htonl(QuotKindID);
	Amount     = htonl(Amount);
	PctDis     = htonl(PctDis);
	LocID      = htonl(LocID);
}

SpOrdLineStruc::SpOrdLineStruc() 
{
	Init();
}
	
void SpOrdLineStruc::Init() 
{
	ID = OrderID = GoodsID = Price = Qtty = 0;
}
	
int SpOrdLineStruc::ToBuf(void * pBuf, size_t * pBufSize)
{
	int ok = 0;
	if(pBufSize) {
		if(*pBufSize < 20) {
			*pBufSize = 20;
			ok = -1;
		}
		else {
			char * p_buf = (char *)pBuf;
			size_t bytes = 0;
			memcpy(p_buf,                              &ID,      sizeof(ID));
			memcpy(p_buf + (bytes += sizeof(ID)),      &OrderID, sizeof(OrderID));
			memcpy(p_buf + (bytes += sizeof(OrderID)), &GoodsID, sizeof(GoodsID));
			memcpy(p_buf + (bytes += sizeof(GoodsID)), &Price,   sizeof(Price));
			memcpy(p_buf + (bytes += sizeof(Price)),   &Qtty,    sizeof(Qtty));
			*pBufSize = 20;
			ok = 1;
		}
	}
	return ok;
}
	
void SpOrdLineStruc::ToPalmRec()
{
	ID       = htonl(ID);
	OrderID  = htonl(OrderID);
	GoodsID  = htonl(GoodsID);
	Price    = htonl(Price);
	Qtty     = htonl(Qtty);
}
