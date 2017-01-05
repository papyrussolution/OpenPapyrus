// SBUFFER.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

size_t FASTCALL SnapUpSize(size_t i); // @prototype

int FASTCALL SBuffer::Alloc(size_t sz)
{
	int    ok = 1;
	if(sz == 0) {
		Reset(1);
	}
	else if(sz > Size) {
		size_t new_size = SnapUpSize(sz);
		char * p = 0;
		if((7 * Size) < (8 * WrOffs)) { // Assume probability of a non-moving realloc is 0.125
			// If L is close to Size in size then use realloc to reduce the memory defragmentation
			p = (char *)realloc(P_Buf, new_size);
		}
		else {
			// If L is not close to Size then avoid the penalty of copying
			// the extra bytes that are allocated, but not considered part of the string
			p = (char *)malloc(new_size);
			if(!p)
				p = (char *)realloc(P_Buf, new_size);
			else {
				if(WrOffs)
					memcpy(p, P_Buf, WrOffs);
				free(P_Buf);
			}
		}
		if(p) {
			Size = new_size;
			P_Buf = p;
		}
		else {
			Flags |= fError;
			ok = (SLibError = SLERR_NOMEM, 0);
		}
	}
	return ok;
}

IMPL_INVARIANT_C(SBuffer)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(Size >= WrOffs, pInvP);
	S_ASSERT_P(WrOffs >= RdOffs, pInvP);
	S_ASSERT_P(P_Buf || (!Size && !WrOffs && !RdOffs), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

SLAPI SBuffer::SBuffer(size_t initSize, long flags)
{
	Reset(0);
	Flags = (flags & ~fError);
	if(initSize)
		Alloc(initSize);
}

SLAPI SBuffer::SBuffer(const SBuffer & s)
{
	Reset(0);
	Copy(s);
}

int FASTCALL SBuffer::IsEqual(const SBuffer & rS) const
{
	int    ok = 1;
	if(WrOffs != rS.WrOffs)
		ok = 0;
	else if(WrOffs && memcmp(P_Buf, rS.P_Buf, WrOffs) != 0)
		ok = 0;
	return ok;
}

SBuffer & FASTCALL SBuffer::operator = (const SBuffer & s)
{
	Copy(s);
	return *this;
}

int FASTCALL SBuffer::Copy(const SBuffer & s)
{
	Reset(1);
	if(Alloc(s.WrOffs)) {
		memcpy(P_Buf, s.P_Buf, s.WrOffs);
		RdOffs = 0;
		WrOffs = s.WrOffs;
		Flags  = s.Flags;
		return 1;
	}
	else
		return 0;
}

int SLAPI SBuffer::IsValid()
{
	if(!(Flags & fError)) {
		SInvariantParam ip;
		if(!InvariantC(&ip))
			Flags |= fError;
	}
	return (Flags & fError) ? 0 : 1;
}

SBuffer & FASTCALL SBuffer::Reset(int freeBuf)
{
	Size = 0;
	WrOffs = RdOffs = 0;
	Flags &= ~fError;
	if(freeBuf)
		free(P_Buf);
	P_Buf = 0;
	return *this;
}

SLAPI SBuffer::~SBuffer()
{
	free(P_Buf);
}

void SBuffer::Destroy()
{
	Reset(1);
}

SBuffer & SLAPI SBuffer::Clear()
{
	if(WrOffs || RdOffs)
		WrOffs = RdOffs = 0;
	return *this;
}

void * FASTCALL SBuffer::Ptr(size_t offs) const
	{ return (((int8 *)P_Buf)+offs); }
const void * FASTCALL SBuffer::GetBuf(size_t offs) const
	{ return (const void *)Ptr(offs); }
SLAPI SBuffer::operator const void * () const
	{ return P_Buf; }
size_t SLAPI SBuffer::GetSize() const
	{ return Size; }
size_t SLAPI SBuffer::GetRdOffs() const
	{ return RdOffs; }
size_t SLAPI SBuffer::GetWrOffs() const
	{ return WrOffs; }
void FASTCALL SBuffer::SetRdOffs(size_t offs)
	{ RdOffs = MIN(offs, WrOffs); }
void FASTCALL SBuffer::SetWrOffs(size_t offs)
	{ WrOffs = MAX(MIN(offs, Size), RdOffs); }
size_t SLAPI SBuffer::GetAvailableSize() const
	{ return (WrOffs > RdOffs) ? (WrOffs - RdOffs) : 0; }

int FASTCALL SBuffer::Write(const void * pBuf, size_t size)
{
	int    ok = 1;
	if(size) {
		if(RdOffs && Flags & fMovable) {
			const size_t rd_offs = RdOffs;
			memmove(Ptr(0), Ptr(rd_offs), Size-rd_offs); // @v6.1.1 RdOffs-->Size-RdOffs (BUG)
			RdOffs -= rd_offs;
			WrOffs -= rd_offs; // @v7.8.0 Была ошибка: из WrOffs вычитался уже уменьшенный до нуля размер RdOffs
		}
		const size_t new_size = (WrOffs + size);
		if((new_size <= Size) || Alloc(new_size)) { // @v9.4.1 (new_size <= Size) с целью ускорения
			// @v8.4.2 {
			void * _ptr = Ptr(WrOffs);
			switch(size) {
				case 1: *PTR8(_ptr) = *PTR8(pBuf); break;
				case 2: *PTR16(_ptr) = *PTR16(pBuf); break;
				case 4: *PTR32(_ptr) = *PTR32(pBuf); break;
				case 8: *PTR64(_ptr) = *PTR64(pBuf); break;
				case 12:
					PTR32(_ptr)[0] = PTR32(pBuf)[0];
					PTR32(_ptr)[1] = PTR32(pBuf)[1];
					PTR32(_ptr)[2] = PTR32(pBuf)[2];
					break;
				case 16:
					PTR32(_ptr)[0] = PTR32(pBuf)[0];
					PTR32(_ptr)[1] = PTR32(pBuf)[1];
					PTR32(_ptr)[2] = PTR32(pBuf)[2];
					PTR32(_ptr)[3] = PTR32(pBuf)[3];
					break;
				default:
					memcpy(_ptr, pBuf, size);
			}
			// } @v8.4.2
			// @v8.4.2 memcpy(Ptr(WrOffs), pBuf, size);
			WrOffs += size;
		}
		else
			ok = 0;
	}
	return ok;
}

size_t FASTCALL SBuffer::ReadStatic(void * pBuf, size_t bufLen) const
{
	size_t sz = 0;
	if(pBuf) {
		size_t avl_size = GetAvailableSize();
		sz = bufLen ? MIN(avl_size, bufLen) : avl_size;
		if(sz)
			memcpy(pBuf, Ptr(RdOffs), sz);
	}
	return sz;
}

size_t FASTCALL SBuffer::Read(void * pBuf, size_t bufLen)
{
	size_t sz = ReadStatic(pBuf, bufLen);
	RdOffs += sz;
	return sz;
}

int FASTCALL SBuffer::ReadV(void * pBuf, size_t bufLen)
{
	int    ok = 1;
	size_t sz = ReadStatic(pBuf, bufLen);
	RdOffs += sz;
	if(sz != bufLen) {
		SString msg_buf;
		SLS.SetError(SLERR_SBUFRDSIZE, msg_buf.Cat(bufLen).Cat("<<").Cat(sz));
		ok = 0;
	}
	return ok;
}

int FASTCALL SBuffer::Unread(size_t offs)
{
	if(offs <= RdOffs) {
		RdOffs -= offs;
		return 1;
	}
	else
		return 0;
}

int SLAPI SBuffer::Search(const char * pStr, size_t * pPos) const
{
	int    ok = 0;
	const  size_t avl_size = GetAvailableSize();
	if(avl_size) {
		const size_t tlen = sstrlen(pStr);
		if(tlen == 1 || tlen == 0) {
			const int pat = pStr ? pStr[0] : 0;
			const  char * ptr = (const char *)memchr(Ptr(RdOffs), pat, avl_size); // @v9.4.1 @fix pStr[0]-->pat
			if(ptr) {
				ASSIGN_PTR(pPos, RdOffs + (ptr-(const char *)Ptr(RdOffs)));
				ok = 1;
			}
		}
		else {
			for(size_t p = RdOffs; !ok && p < RdOffs+avl_size-tlen+1; p++) {
				int found = 1;
				for(size_t i = 0; i < tlen && found; i++)
					if(*(const char *)Ptr(p+i) != pStr[i])
						found = 0;
				if(found) {
					ASSIGN_PTR(pPos, p);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

size_t SLAPI SBuffer::ReadTerm(const char * pTerm, void * pBuf, size_t bufLen)
{
	size_t sz = 0;
	if(pBuf) {
		size_t pos = 0;
		if(Search(pTerm, &pos)) {
			const size_t avl_size = pos + strlen(pTerm) - RdOffs; // @v6.1.1 -RdOffs
			sz = bufLen ? MIN(avl_size, bufLen) : avl_size;
			if(sz) {
				memcpy(pBuf, Ptr(RdOffs), sz);
				RdOffs += sz;
			}
		}
	}
	return sz;
}

size_t SLAPI SBuffer::ReadTermStr(const char * pTerm, SString & rBuf)
{
	rBuf = 0;

	size_t sz = 0;
	if(pTerm) {
		size_t pos = 0;
		if(Search(pTerm, &pos)) {
			size_t avl_size = pos + strlen(pTerm) - RdOffs;
			sz = avl_size;
			if(sz) {
				rBuf.CopyFromN((const char *)Ptr(RdOffs), sz);
				RdOffs += sz;
			}
		}
	}
	else {
		char c;
		while(Read(c)) {
			sz++;
			if(c)
				rBuf.CatChar(c);
			else
				break;
		}
	}
	return sz;
}

int FASTCALL SBuffer::WriteByte(char r)          { return Write(&r, sizeof(r)); }
int FASTCALL SBuffer::Read(char & r)             { return ReadV(&r, sizeof(r)); }
int FASTCALL SBuffer::Write(const uint8 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(uint8 & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const uint16 & v)    { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(uint16 & v)           { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const int16 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(int16 & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const int64 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(int64 & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const long & v)      { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(long & v)             { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const ulong & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(ulong & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::WriteFloat(float v)        { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(float & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const double & v)    { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(double & v)           { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LDATE & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LDATE & v)            { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LDATETIME & v) { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LDATETIME & v)        { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LTIME & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LTIME & v)            { return ReadV(&v, sizeof(v)); }

int FASTCALL SBuffer::Write(const SBuffer & v)
{
	int    ok = 1;
	uint32 sz = (uint32)v.GetAvailableSize();
	THROW(Write(&sz, sizeof(sz)));
	if(sz) {
		STempBuffer temp_buf(sz);
		THROW(temp_buf.IsValid());
		THROW(v.ReadStatic(temp_buf, sz));
		THROW(Write(temp_buf, sz));
	}
	CATCHZOK
	return ok;
}

int FASTCALL SBuffer::Read(SBuffer & v)
{
	int    ok = 1;
	uint32 sz = 0;
	Read(&sz, sizeof(sz)); // @v5.4.12 Здесь не проверяется возвращаемое значение, ибо считываемый размер легально может быть 0
	if(sz) {
		STempBuffer temp_buf(sz);
		THROW(temp_buf.IsValid());
		THROW(ReadV(temp_buf, sz));
		THROW(v.Write(temp_buf, sz));
	}
	CATCHZOK
	return ok;
}

int FASTCALL SBuffer::Write(const SArray * pAry, long options)
{
	int    ok = 1;
	uint32 c = (pAry && !(options & ffAryForceEmpty)) ? pAry->getCount() : 0;
	size_t item_size = pAry ? pAry->getItemSize() : 0;
	size_t beg_pos = WrOffs;
	if(options & ffAryCount32) {
		THROW(Write(&c, sizeof(c)));
	}
	else {
		uint16 c16 = (uint16)c;
		THROW(Write(&c16, sizeof(c16)));
	}
	/* @v9.4.1
	for(uint32 i = 0; i < c; i++)
		THROW(Write(pAry->at(i), item_size));
	*/
	if(c) {
		THROW(Write(pAry->dataPtr(), item_size * c)); // @v9.4.1
	}
	CATCH
		WrOffs = beg_pos;
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SBuffer::Read(SArray * pAry, long options)
{
	assert(pAry != 0);
	int    ok = 1;
	uint32 c = 0;
	const size_t item_size = pAry->getItemSize();
	const size_t beg_pos = RdOffs;
	pAry->freeAll();
	if(options & ffAryCount32) {
		THROW(ReadV(&c, sizeof(c)));
	}
	else {
		uint16 c16;
		THROW(ReadV(&c16, sizeof(c16)));
		c = (uint32)c16;
	}
	/* @v9.4.1
	{
		STempBuffer buf(item_size);
		THROW(buf.IsValid());
		for(uint32 i = 0; i < c; i++) {
			THROW(ReadV(buf, item_size));
			THROW(pAry->insert(buf));
		}
	}
	*/
	// @v9.4.1 {
	{
		const size_t total_size = c * item_size;
		if(GetAvailableSize() >= total_size) {
			THROW(pAry->insertChunk(c, GetBuf(GetRdOffs())));
			RdOffs += total_size;
		}
		else {
			SString msg_buf;
			SLS.SetError(SLERR_SBUFRDSIZE, msg_buf.Cat(total_size).Cat("<<").Cat(GetAvailableSize()));
			CALLEXCEPT();
		}
	}
	// } @v9.4.1
	CATCH
		RdOffs = beg_pos;
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SBuffer::Write(const SString & rBuf)
{
	uint16 sz = (uint16)rBuf.Len();
	return (Write(&sz, sizeof(sz)) && Write((const char *)rBuf, sz)) ? 1 : 0;
}

int FASTCALL SBuffer::Read(SString & rBuf)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 sz = 0;
	char * p_temp_buf = 0;
	char   shrt_buf[512];

	rBuf = 0;
	THROW(ReadV(&sz, sizeof(sz)));
	if(sz > sizeof(shrt_buf)) {
		THROW_V(p_temp_buf = (char *)malloc(sz), SLERR_NOMEM);
	}
	else
		p_temp_buf = shrt_buf;
	if(sz) {
		THROW(ReadV(p_temp_buf, sz));
		rBuf.CopyFromN(p_temp_buf, sz);
	}
	CATCHZOK
	if(p_temp_buf && p_temp_buf != shrt_buf)
		free(p_temp_buf);
	return ok;
}

int SLAPI SBuffer::WriteToFile(FILE * f, uint sign, uint32 * pActualBytes)
{
	int    ok = 1;
	uint32 actual_size = 0;
	THROW(f);
	if(sign > 0 && sign <= 0xffffU) {
		uint16 s = (uint16)sign;
		THROW_S(fwrite(&s, sizeof(s), 1, f) == 1, SLERR_WRITEFAULT);
		actual_size += sizeof(s);
	}
	{
		size_t offs = GetRdOffs();
		uint32 sz = GetAvailableSize();
		THROW_S(fwrite(&sz, sizeof(sz), 1, f) == 1, SLERR_WRITEFAULT);
		actual_size += sizeof(sz);
		THROW_S(fwrite(GetBuf(offs), sz, 1, f) == 1, SLERR_WRITEFAULT);
		actual_size += sz;
	}
	CATCHZOK
	ASSIGN_PTR(pActualBytes, actual_size);
	return ok;
}

int SLAPI SBuffer::ReadFromFile(FILE * f, uint sign)
{
	int    ok = 1;
	THROW(f);
	if(sign > 0 && sign <= 0xffffU) {
		uint16 s = (uint16)sign;
		THROW_S(fread(&s, sizeof(s), 1, f) == 1, SLERR_READFAULT);
		THROW_S(sign == s, SLERR_READFAULT);
	}
	{
		uint32 sz;
		THROW_S(fread(&sz, sizeof(sz), 1, f) == 1, SLERR_READFAULT);
		{
			const size_t temp_buf_len = 4096;
			STempBuffer temp_buf(temp_buf_len);
			THROW(temp_buf.IsValid());
			for(size_t tail = sz; tail != 0;) {
				const size_t pc = MIN(tail, temp_buf_len);
				THROW_S(fread(temp_buf, pc, 1, f) == 1, SLERR_READFAULT);
				THROW(Write(temp_buf, pc));
				tail -= pc;
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
void SLAPI SBaseBuffer::Init()
{
	P_Buf = 0;
	Size = 0;
}

void SLAPI SBaseBuffer::Destroy()
{
	ZFREE(P_Buf);
	Size = 0;
}

int FASTCALL SBaseBuffer::IsEqual(const SBaseBuffer & rS) const
{
	if(Size != rS.Size)
		return 0;
	else if(Size && memcmp(P_Buf, rS.P_Buf, Size) != 0)
		return 0;
	else
		return 1;
}

int FASTCALL SBaseBuffer::Copy(const SBaseBuffer & rS)
{
	if(Alloc(rS.Size)) {
		memcpy(P_Buf, rS.P_Buf, rS.Size);
		return 1;
	}
	else
		return 0;
}

int FASTCALL SBaseBuffer::Alloc(size_t sz)
{
	int    ok = -1;
	if(sz > Size) {
		void * p_temp = realloc(P_Buf, sz);
		if(p_temp) {
			P_Buf = (char *)p_temp;
			Size = sz;
		}
		else
			ok = (SLibError = SLERR_NOMEM, 0);
	}
	return ok;
}

void SLAPI SBaseBuffer::Zero()
{
	memzero(P_Buf, Size);
}

void SLAPI SBaseBuffer::Set(void * pBuf, size_t size)
{
	P_Buf = (char *)pBuf;
	Size = size;
}
//
//
//
SLAPI STempBuffer::STempBuffer(size_t sz)
{
	Init();
	Alloc(sz);
}

SLAPI STempBuffer::~STempBuffer()
{
	Destroy();
}

int FASTCALL STempBuffer::Alloc(size_t sz)
{
	if(sz == 0) {
		ZFREE(P_Buf);
		Size = 0;
		return 1;
	}
	else {
		char * p = (char *)realloc(P_Buf, sz);
		if(p) {
			P_Buf = p;
			Size = sz;
			return 1;
		}
		else {
			Size = 0;
			return (SLibError = SLERR_NOMEM, 0);
		}
	}
}

STempBuffer & FASTCALL STempBuffer::operator = (const STempBuffer & rS)
{
	SBaseBuffer::Copy(rS);
	return *this;
}

int SLAPI STempBuffer::IsValid() const
{
	return BIN(P_Buf);
}

size_t SLAPI STempBuffer::GetSize() const
{
	return Size;
}

SLAPI STempBuffer::operator char * ()
{
	return P_Buf;
}

SLAPI STempBuffer::operator const char * () const
{
	return P_Buf;
}

const uchar * SLAPI STempBuffer::ucptr() const
{
	return (const uchar *)P_Buf;
}
//
//
//
struct SscDbtItem {
	uint32 DbtID;
	BNFieldList Fields;
};

SLAPI SSerializeContext::SSerializeContext() : SymbTbl(2048, 1), TempDataBuf(0)
{
	P_DbtDescrList = new TSCollection <SscDbtItem>;
	LastSymbId = 0;
	SuppDate = ZERODATE;
	State = 0;
	Flags = 0;
}

SLAPI SSerializeContext::~SSerializeContext()
{
	ZDELETE(P_DbtDescrList);
}

int SLAPI SSerializeContext::Init(long flags, LDATE suppDate)
{
	Flags |= flags;
	SuppDate = suppDate;
	P_DbtDescrList->freeAll();
	LastSymbId = 0;
	SymbTbl.Clear();
	return 1;
}

LDATE SLAPI SSerializeContext::GetSupportingDate() const
{
	return SuppDate;
}

int SLAPI SSerializeContext::AddDbtDescr(const char * pName, const BNFieldList * pList, uint32 * pID)
{
	int    ok = -1;
	(TempBuf = pName).ToUpper();
	uint   dbt_id = 0;
	if(!SymbTbl.Search(TempBuf, &dbt_id, 0)) {
		dbt_id = ++LastSymbId;
		SymbTbl.Add(TempBuf, dbt_id, 0);
		SscDbtItem * p_new_item = new SscDbtItem;
		THROW_S(p_new_item, SLERR_NOMEM);
		p_new_item->DbtID = dbt_id;
		p_new_item->Fields = *pList;
		THROW(P_DbtDescrList->insert(p_new_item));
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pID, dbt_id);
	return ok;
}

int SLAPI SSerializeContext::GetDbtDescr(uint id, BNFieldList * pList) const
{
	for(uint i = 0; i < P_DbtDescrList->getCount(); i++) {
		SscDbtItem * p_item = (SscDbtItem *)P_DbtDescrList->at(i);
		if(p_item->DbtID == id) {
			ASSIGN_PTR(pList, p_item->Fields);
			return 1;
		}
	}
	return 0;
}

int SLAPI SSerializeContext::SerializeFieldList(int dir, BNFieldList * pFldList, SBuffer & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	uint16 fld_count;
	if(dir > 0) {
		fld_count = pFldList->getCount();
		rBuf.Write(fld_count);
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pFldList->getField(i);
			THROW(rBuf.Write(r_fld.Id));
			temp_buf = r_fld.Name;
			THROW(rBuf.Write(temp_buf));
			THROW(rBuf.Write(r_fld.Offs));
			THROW(rBuf.Write(r_fld.T));
		}
	}
	else if(dir < 0) {
		pFldList->reset();
		rBuf.Read(fld_count);
		for(uint i = 0; i < fld_count; i++) {
			BNField fld;
			THROW(rBuf.Read(fld.Id));
			THROW(rBuf.Read(temp_buf));
			temp_buf.CopyTo(fld.Name, sizeof(fld.Name));
			THROW(rBuf.Read(fld.Offs));
			THROW(rBuf.Read(fld.T));
			THROW(pFldList->addField(fld));
		}
	}
	CATCHZOK
	return ok;
}

enum SSrlzSign {
	ssrsignState = 1,
	ssrsignStructData
};

int SLAPI SSerializeContext::SerializeState(int dir, SBuffer & rBuf)
{
	int    ok = 1;
	uint16 sign = 0;
	struct StateBlock { // @persistent
		int16  Version;
		uint8  Reserve[14];
		int32  Flags;
		LDATE  SuppDate;
		uint32 LastSymbId;
		uint32 StructCount;
	};
	if(dir > 0) {
		sign = ssrsignState;
		StateBlock blk;
		MEMSZERO(blk);
		blk.Version = 1;
		blk.Flags = Flags;
		blk.SuppDate = SuppDate;
		blk.LastSymbId = LastSymbId;
		blk.StructCount = P_DbtDescrList->getCount();

		rBuf.Write(sign);
		rBuf.Write(&blk, sizeof(blk));
		if(Flags & fSeparateDataStruct) {
			for(uint i = 0; i < blk.StructCount; i++) {
				SscDbtItem * p_item = (SscDbtItem *)P_DbtDescrList->at(i);
				SymbTbl.GetByAssoc(p_item->DbtID, TempBuf);
				rBuf.Write(TempBuf);
				rBuf.Write(p_item->DbtID);
				THROW(SerializeFieldList(+1, &p_item->Fields, rBuf));
			}
		}
	}
	else if(dir < 0) {
		StateBlock blk;
		rBuf.Read(sign);
		THROW_S(sign == ssrsignState, SLERR_SRLZ_COMMRDFAULT);
		THROW_S(rBuf.Read(&blk, sizeof(blk)) == sizeof(blk), SLERR_SRLZ_COMMRDFAULT);

		Flags = blk.Flags;
		SuppDate = blk.SuppDate;
		LastSymbId = blk.LastSymbId;
		if(Flags & fSeparateDataStruct) {
			SymbTbl.Clear();
			P_DbtDescrList->freeAll();
			for(uint i = 0; i < blk.StructCount; i++) {
				SscDbtItem * p_new_item = new SscDbtItem;
				THROW(rBuf.Read(TempBuf));
				THROW(rBuf.Read(p_new_item->DbtID));
				THROW(SerializeFieldList(-1, &p_new_item->Fields, rBuf));
				THROW(P_DbtDescrList->insert(p_new_item));
				THROW(SymbTbl.Add(TempBuf, p_new_item->DbtID, 0));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, TYPEID typ, void * pData, uint8 * pInd, SBuffer & rBuf)
{
	int    ok = 1;
	DataType _t;
	if(pInd) {
		THROW(stype(typ, &_t).Serialize(dir, pData, pInd, rBuf, this));
	}
	else {
		uint8  ind = 0;
		if(dir > 0) {
			SBuffer temp_buf;
			THROW(stype(typ, &_t).Serialize(dir, pData, &ind, temp_buf, this));
			THROW(rBuf.Write(&ind, sizeof(ind)));
			const size_t avsz = temp_buf.GetAvailableSize();
			if(avsz)
				THROW(rBuf.Write(temp_buf.GetBuf(temp_buf.GetRdOffs()), avsz));
		}
		else if(dir < 0) {
			THROW(rBuf.ReadV(&ind, sizeof(ind)));
			THROW(stype(typ, &_t).Serialize(dir, pData, &ind, rBuf, this));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, int64  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, int32  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, int16  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, int8   & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, int    & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, uint   & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, uint64 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, uint32 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, uint16 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, uint8  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, LDATE  & rV, SBuffer & rBuf) { return Serialize(dir, T_DATE, &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, LTIME  & rV, SBuffer & rBuf) { return Serialize(dir, T_TIME, &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, LDATETIME & rV, SBuffer & rBuf) { return Serialize(dir, T_DATETIME, &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, float  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FLOAT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, double & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FLOAT, sizeof(rV)), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, S_GUID & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UUID_, sizeof(rV)), &rV, 0, rBuf); } // @v8.1.1
int SLAPI SSerializeContext::Serialize(int dir, TPoint & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_IPOINT2, 4), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, FPoint & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FPOINT2, 8), &rV, 0, rBuf); }
int SLAPI SSerializeContext::Serialize(int dir, char *   pV, size_t valBufLen, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_ZSTRING, valBufLen), pV, 0, rBuf); }

int SLAPI SSerializeContext::Serialize(int dir, DateRange & rV, SBuffer & rBuf)
{
    int    ok = 1;
    THROW(Serialize(dir, rV.low, rBuf));
    THROW(Serialize(dir, rV.upp, rBuf));
    CATCHZOK
    return ok;
}

int SLAPI SSerializeContext::Serialize(const char * pDbtName, BNFieldList * pFldList, const void * pData, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 dbt_id = 0;
	int    r = AddDbtDescr(pDbtName, pFldList, &dbt_id);
	uint8 * p_ind_list = 0;
	int    own_ind_list = 0;
	THROW(r);
	rBuf.Write(dbt_id);
	if(r > 0) {
		uint16 temp_val = 1;
		if(!(Flags & fSeparateDataStruct)) {
			rBuf.Write(temp_val = 1);
			THROW(SerializeFieldList(+1, pFldList, rBuf));
		}
		else
			rBuf.Write(temp_val = 0);
	}
	else {
		uint16 temp_val = 0;
		rBuf.Write(temp_val);
	}
	{
		const uint fld_count = pFldList->getCount();
		const uint ind_len = ALIGNSIZE(fld_count, 2);
		SBuffer temp_buf;
		uint8  st_ind_list[512];
		if(ind_len > SIZEOFARRAY(st_ind_list)) {
			//
			// Страховка от случая, когда количество полей больше, чем 512
			//
			THROW_S(p_ind_list = (uint8 *)malloc(ind_len), SLERR_NOMEM);
			own_ind_list = 1;
		}
		else
			p_ind_list = st_ind_list;
		memzero(p_ind_list, ind_len);
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pFldList->getField(i);
			THROW(Serialize(+1, r_fld.T, PTR8(pData)+r_fld.Offs, p_ind_list+i, temp_buf));
		}
		rBuf.Write(p_ind_list, ind_len);
		rBuf.Write(temp_buf);
	}
	CATCHZOK
	if(own_ind_list)
		free(p_ind_list);
	return ok;
}

int SLAPI SSerializeContext::Unserialize(const BNFieldList * pFldList, void * pData, SBuffer & rBuf)
{
	int    ok = 1, r;
	uint32 dbt_id = 0;
	uint16 temp_val = -999; // -999 - умышленный мусор
	uint8 * p_ind_list = 0;
	int    own_ind_list = 0;
	BNFieldList inner_fld_list;
	THROW(rBuf.Read(dbt_id));
	THROW(rBuf.Read(temp_val));
	THROW_S(oneof2(temp_val, 0, 1), SLERR_SRLZ_COMMRDFAULT);
	THROW(r = GetDbtDescr(dbt_id, &inner_fld_list));
	if(r > 0) {
		if(temp_val == 1) {
			BNFieldList temp_flist;
			THROW(SerializeFieldList(-1, &temp_flist, rBuf));
			THROW_S(temp_flist.IsEqual(inner_fld_list), SLERR_SRLZ_UNEQFLDLIST);
		}
	}
	else {
		//
		// Если в существующем списке структур наша не найдена, то далее должно
		// следовать описание структуры.
		//
		THROW(temp_val == 1);
		THROW(SerializeFieldList(-1, &inner_fld_list, rBuf));
	}
	{
		const uint fld_count = inner_fld_list.getCount();
		const uint ind_len = ALIGNSIZE(fld_count, 2);
		SBuffer temp_buf;
		uint8  st_ind_list[512];
		if(ind_len > SIZEOFARRAY(st_ind_list)) {
			//
			// Страховка от случая, когда количество полей больше, чем 512
			//
			THROW_S(p_ind_list = (uint8 *)malloc(ind_len), SLERR_NOMEM);
			own_ind_list = 1;
		}
		else
			p_ind_list = st_ind_list;
		THROW(rBuf.ReadV(p_ind_list, ind_len));
		THROW(rBuf.Read(temp_buf));
		if(pFldList) {
			const int is_eq_struc = inner_fld_list.IsEqual(*pFldList);
			if(!is_eq_struc) {
				//
				// При не эквивалентных структурах придется использовать временный буфер
				// для считывания данных из потока.
				//
				TempDataBuf.Alloc(8192);
			}
			for(uint i = 0; i < fld_count; i++) {
				const BNField & r_fld = inner_fld_list.getField(i);
				if(is_eq_struc) {
					THROW(Serialize(-1, r_fld.T, PTR8(pData)+r_fld.Offs, p_ind_list+i, temp_buf));
				}
				else {
					//
					// Если сохраненная структура и та, в которую считываются данные не эквивалентны,
					// то придется преобразовывать данные, ориентируясь на имена полей.
					//
					THROW(Serialize(-1, r_fld.T, TempDataBuf, p_ind_list+i, temp_buf));
					const BNField & r_dest_fld = pFldList->getField(r_fld.Name, 0);
					if(&r_dest_fld)
						stcast(r_fld.T, r_dest_fld.T, TempDataBuf, PTR8(pData)+r_dest_fld.Offs, 0);
				}
			}
		}
	}
	CATCHZOK
	if(own_ind_list)
		free(p_ind_list);
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, SStrCollection * pColl, SBuffer & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	if(dir > 0) {
		uint32 c = pColl ? pColl->getCount() : 0;
		THROW(rBuf.Write(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
			temp_buf = pColl->at(i);
			THROW(rBuf.Write(temp_buf));
		}
	}
	else if(dir < 0) {
		CALLPTRMEMB(pColl, freeAll());
		uint32 c = 0;
		THROW(rBuf.ReadV(&c, sizeof(c)));
		for(uint i = 0; i < c; i++) {
			THROW(rBuf.Read(temp_buf));
			if(pColl) {
				char * p_item = 0;
				if(temp_buf.NotEmpty()) {
					THROW_S(p_item = newStr(temp_buf), SLERR_NOMEM);
				}
				THROW(pColl->insert(p_item));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, SArray * pArray, SBuffer & rBuf)
{
	int    ok = 1;
	if(dir > 0) {
		ok = rBuf.Write(pArray, SBuffer::ffAryCount32);
	}
	else if(dir < 0) {
		ok = rBuf.Read(pArray, SBuffer::ffAryCount32);
	}
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, StrAssocArray & rArray, SBuffer & rBuf)
{
	int    ok = 1;
	if(dir > 0) {
		ok = rArray.Write(rBuf, 0);
	}
	else if(dir < 0) {
		ok = rArray.Read(rBuf, 0);
	}
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, SString & rStr, SBuffer & rBuf)
{
	int    ok = 1;
	if(dir > 0) {
		ok = rBuf.Write(rStr);
	}
	else if(dir < 0) {
		ok = rBuf.Read(rStr);
	}
	return ok;
}

int SLAPI SSerializeContext::Serialize(int dir, SStringU & rStr, SBuffer & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	if(dir > 0) {
		rStr.CopyToUtf8(temp_buf, 0);
		THROW(Serialize(dir, temp_buf, rBuf));
	}
	else if(dir < 0) {
		THROW(Serialize(dir, temp_buf, rBuf));
		rStr.CopyFromUtf8(temp_buf);
	}
	CATCHZOK
	return ok;
}

int SLAPI SSerializeContext::SerializeBlock(int dir, uint32 size, void * pData, SBuffer & rBuf, int skipMissizedBlock)
{
	int    ok = 1;
	uint32 sz;
	if(dir > 0) {
		sz = pData ? size : 0;
		THROW(rBuf.Write(sz));
		if(sz) {
			THROW(rBuf.Write(pData, sz));
		}
	}
	else if(dir < 0) {
		THROW(rBuf.Read(sz));
		if(sz) {
			if(!pData)
				rBuf.SetRdOffs(rBuf.GetRdOffs()+sz);
			else if(sz == size) {
				THROW(rBuf.ReadV(pData, sz));
			}
			else if(skipMissizedBlock) {
				rBuf.SetRdOffs(rBuf.GetRdOffs()+sz);
				ok = -1;
			}
			else {
				size_t ms = MIN(sz, size);
				THROW(rBuf.ReadV(pData, ms));
				if(sz > size)
					rBuf.SetRdOffs(rBuf.GetRdOffs()+(sz-size));
			}
		}
		else
			ok = -1;
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI  SSerializer::SSerializer(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) : R_Buf(rBuf)
{
	Dir = dir;
	P_SCtx = pSCtx;
}

int    SLAPI SSerializer::Serialize(TYPEID typ, void * pData, uint8 * pInd)
{
	return P_SCtx->Serialize(Dir, typ, pData, pInd, R_Buf);
}

int    FASTCALL SSerializer::Serialize(SString & rStr)
{
	return P_SCtx->Serialize(Dir, rStr, R_Buf);
}

int    FASTCALL SSerializer::Serialize(SStringU & rStr)
{
	return P_SCtx->Serialize(Dir, rStr, R_Buf);
}

int    FASTCALL SSerializer::Serialize(int64 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(int32 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(int16 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(int8 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(int & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(uint64 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(uint32 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(uint16 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(uint8 & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(LDATE & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(LTIME & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(LDATETIME & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(float & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(double & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(S_GUID & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    SLAPI SSerializer::Serialize(char * pV, size_t valBufLen)
{
	return P_SCtx->Serialize(Dir, pV, valBufLen, R_Buf);
}

int    SLAPI SSerializer::SerializeFieldList(BNFieldList * pFldList)
{
	return P_SCtx->SerializeFieldList(Dir, pFldList, R_Buf);
}

int    FASTCALL SSerializer::Serialize(TPoint & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(FPoint & rV)
{
	return P_SCtx->Serialize(Dir, rV, R_Buf);
}

int    FASTCALL SSerializer::Serialize(SArray * pArray)
{
	return P_SCtx->Serialize(Dir, pArray, R_Buf);
}

int    FASTCALL SSerializer::Serialize(SStrCollection * pColl)
{
	return P_SCtx->Serialize(Dir, pColl, R_Buf);
}

int FASTCALL SSerializer::Serialize(StrAssocArray & rArray)
{
	return P_SCtx->Serialize(Dir, rArray, R_Buf);
}

int    SLAPI SSerializer::SerializeBlock(uint32 size, void * pData, int skipMissizedBlock)
{
	return P_SCtx->SerializeBlock(Dir, size, pData, R_Buf, skipMissizedBlock);
}
