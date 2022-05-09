// SBUFFER.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

size_t FASTCALL SnapUpSize(size_t i); // @prototype

int FASTCALL SBuffer::Alloc(size_t sz)
{
	int    ok = 1;
	if(sz == 0) {
		Destroy();
	}
	else if(sz > Size) {
		size_t new_size = SnapUpSize(sz);
		char * p = 0;
		if((7 * Size) < (8 * WrOffs)) { // Assume probability of a non-moving realloc is 0.125
			// If L is close to Size in size then use realloc to reduce the memory defragmentation
			p = static_cast<char *>(SAlloc::R(P_Buf, new_size));
		}
		else {
			// If L is not close to Size then avoid the penalty of copying
			// the extra bytes that are allocated, but not considered part of the string
			p = static_cast<char *>(SAlloc::M(new_size));
			if(!p)
				p = static_cast<char *>(SAlloc::R(P_Buf, new_size));
			else {
				if(WrOffs)
					memcpy(p, P_Buf, WrOffs);
				SAlloc::F(P_Buf);
			}
		}
		if(p) {
			Size = new_size;
			P_Buf = p;
		}
		else {
			Flags |= fError;
			ok = 0;
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

SBuffer::SBuffer(size_t initSize, long flags) : Size(0), WrOffs(0), RdOffs(0), Flags(flags & ~fError), P_Buf(0)
{
	if(initSize)
		Alloc(initSize);
}

SBuffer::SBuffer(const SBuffer & s) : Size(0), WrOffs(0), RdOffs(0), Flags(0), P_Buf(0)
{
	Copy(s);
}

bool FASTCALL SBuffer::IsEq(const SBuffer & rS) const
{
	bool   ok = true;
	if(WrOffs != rS.WrOffs)
		ok = false;
	else if(WrOffs && memcmp(P_Buf, rS.P_Buf, WrOffs) != 0)
		ok = false;
	return ok;
}

SBuffer & FASTCALL SBuffer::operator = (const SBuffer & s)
{
	Copy(s);
	return *this;
}

int FASTCALL SBuffer::Copy(const SBuffer & s)
{
	Destroy();
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

bool SBuffer::IsValid()
{
	if(!(Flags & fError)) {
		SInvariantParam ip;
		if(!InvariantC(&ip))
			Flags |= fError;
	}
	return !(Flags & fError);
}

SBuffer::~SBuffer()
{
	SAlloc::F(P_Buf);
}

void SBuffer::Destroy()
{
	Size = 0;
	WrOffs = 0;
	RdOffs = 0;
	Flags &= ~fError;
	ZFREE(P_Buf);
}

SBuffer & SBuffer::Z()
{
	WrOffs = 0;
	RdOffs = 0;
	Flags &= ~fError;
	return *this;
}

void * FASTCALL SBuffer::Ptr(size_t offs) const { return (static_cast<int8 *>(P_Buf)+offs); }
const  void * FASTCALL SBuffer::GetBuf(size_t offs) const { return static_cast<const void *>(Ptr(offs)); }
const  char * FASTCALL SBuffer::GetBufC(size_t offs) const { return static_cast<const char *>(Ptr(offs)); }
const  int8 * FASTCALL SBuffer::GetBufI8(size_t offs) const { return static_cast<const int8 *>(Ptr(offs)); }
const  uint8 * FASTCALL SBuffer::GetBufU8(size_t offs) const { return static_cast<const uint8 *>(Ptr(offs)); }
void   FASTCALL SBuffer::SetRdOffs(size_t offs) { RdOffs = MIN(offs, WrOffs); }
void   FASTCALL SBuffer::SetWrOffs(size_t offs) { WrOffs = MAX(MIN(offs, Size), RdOffs); }
size_t SBuffer::GetAvailableSize() const { return (WrOffs > RdOffs) ? (WrOffs - RdOffs) : 0; }

int FASTCALL SBuffer::Write(const void * pBuf, size_t size)
{
	int    ok = 1;
	if(size) {
		if(RdOffs && Flags & fMovable) {
			const size_t rd_offs = RdOffs;
			memmove(Ptr(0), Ptr(rd_offs), Size-rd_offs);
			RdOffs -= rd_offs;
			WrOffs -= rd_offs;
		}
		const size_t new_size = (WrOffs + size);
		if((new_size <= Size) || Alloc(new_size)) { // @v9.4.1 (new_size <= Size) с целью ускорения
			void * _ptr = Ptr(WrOffs);
			switch(size) {
				case 1: *PTR8(_ptr) = *PTR8C(pBuf); break;
				case 2: *PTR16(_ptr) = *PTR16C(pBuf); break;
				case 4: *PTR32(_ptr) = *PTR32C(pBuf); break;
				case 8: *PTR64(_ptr) = *PTR64C(pBuf); break;
				case 12:
					PTR32(_ptr)[0] = PTR32C(pBuf)[0];
					PTR32(_ptr)[1] = PTR32C(pBuf)[1];
					PTR32(_ptr)[2] = PTR32C(pBuf)[2];
					break;
				case 16:
					PTR32(_ptr)[0] = PTR32C(pBuf)[0];
					PTR32(_ptr)[1] = PTR32C(pBuf)[1];
					PTR32(_ptr)[2] = PTR32C(pBuf)[2];
					PTR32(_ptr)[3] = PTR32C(pBuf)[3];
					break;
				default:
					memcpy(_ptr, pBuf, size);
			}
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
		const size_t avl_size = GetAvailableSize();
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
	const  size_t sz = ReadStatic(pBuf, bufLen);
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

int SBuffer::Search(const char * pStr, size_t * pPos) const
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

size_t SBuffer::ReadTerm(const char * pTerm, void * pBuf, size_t bufLen)
{
	size_t sz = 0;
	if(pBuf) {
		size_t pos = 0;
		if(Search(pTerm, &pos)) {
			const size_t avl_size = pos + sstrlen(pTerm) - RdOffs;
			sz = bufLen ? MIN(avl_size, bufLen) : avl_size;
			if(sz) {
				memcpy(pBuf, Ptr(RdOffs), sz);
				RdOffs += sz;
			}
		}
	}
	return sz;
}

size_t FASTCALL SBuffer::ReadLine(SString & rBuf)
{
	rBuf.Z();

	size_t sz = 0;
	const size_t avl_size = GetAvailableSize();
	const char * p_buf = static_cast<const char *>(P_Buf) + RdOffs;
	while(sz < avl_size) {
		const char c = p_buf[sz];
		if(c) {
			rBuf.CatChar(c);
			sz++;
			if(c == 0x0A)
				break;
			else if(c == 0x0D) {
				if(sz < avl_size && p_buf[sz] == 0x0A) {
					rBuf.CatChar(p_buf[sz]);
					sz++;
				}
				break;
			}
		}
		else
			break;
	}
	RdOffs += sz;
	return sz;
}

size_t SBuffer::ReadTermStr(const char * pTerm, SString & rBuf)
{
	rBuf.Z();

	size_t sz = 0;
	if(pTerm) {
		size_t pos = 0;
		if(Search(pTerm, &pos)) {
			const size_t avl_size = pos + sstrlen(pTerm) - RdOffs;
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

int FASTCALL SBuffer::WriteByte(char r) { return Write(&r, sizeof(r)); }
int FASTCALL SBuffer::Read(char & r) { return ReadV(&r, sizeof(r)); }
int FASTCALL SBuffer::Write(const int8 & v)      { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(int8 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const uint8 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(uint8 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const uint16 & v)    { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(uint16 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const int16 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(int16 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const int64 & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(int64 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const uint64 & v)    { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(uint64 & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const long & v)      { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(long & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const ulong & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(ulong & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::WriteFloat(float v) { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(float & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const double & v)    { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(double & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LDATE & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LDATE & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LDATETIME & v) { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LDATETIME & v) { return ReadV(&v, sizeof(v)); }
int FASTCALL SBuffer::Write(const LTIME & v)     { return Write(&v, sizeof(v)); }
int FASTCALL SBuffer::Read(LTIME & v) { return ReadV(&v, sizeof(v)); }

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

int FASTCALL SBuffer::Write(const /*SArray*/SVectorBase * pAry, long options)
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
	if(c) {
		THROW(Write(pAry->dataPtr(), item_size * c));
	}
	CATCH
		WrOffs = beg_pos;
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SBuffer::Helper_Read(SVectorBase * pAry, long options /* = 0*/)
{
	assert(pAry != 0);
	int    ok = 1;
	uint32 c = 0;
	const size_t item_size = pAry->getItemSize();
	const size_t beg_pos = RdOffs;
	if(options & ffAryCount32) {
		THROW(ReadV(&c, sizeof(c)));
	}
	else {
		uint16 c16;
		THROW(ReadV(&c16, sizeof(c16)));
		c = (uint32)c16;
	}
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
	CATCH
		RdOffs = beg_pos;
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SBuffer::Read(SArray * pAry, long options)
{
	assert(pAry != 0);
	if(pAry) {
		pAry->clear(); // @v11.3.0 freeAll()-->clear()
		return Helper_Read(pAry, options);
	}
	else
		return 0;
}

int FASTCALL SBuffer::Read(SVector * pAry, long options)
{
	assert(pAry != 0);
	if(pAry) {
		pAry->clear(); // @v11.3.0 freeAll()-->clear()
		return Helper_Read(pAry, options);
	}
	else
		return 0;
}

int FASTCALL SBuffer::Write(const SString & rBuf)
{
	uint16 sz = (uint16)rBuf.Len();
	return BIN(Write(&sz, sizeof(sz)) && Write(rBuf.cptr(), sz));
}

int FASTCALL SBuffer::Read(SString & rBuf)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 sz = 0;
	char * p_temp_buf = 0;
	char   shrt_buf[512];
	rBuf.Z();
	THROW(ReadV(&sz, sizeof(sz)));
	if(sz > sizeof(shrt_buf)) {
		THROW(p_temp_buf = static_cast<char *>(SAlloc::M(sz)));
	}
	else
		p_temp_buf = shrt_buf;
	if(sz) {
		THROW(ReadV(p_temp_buf, sz));
		rBuf.CopyFromN(p_temp_buf, sz);
	}
	CATCHZOK
	if(p_temp_buf && p_temp_buf != shrt_buf)
		SAlloc::F(p_temp_buf);
	return ok;
}

int SBuffer::WriteToFile(FILE * f, uint sign, uint32 * pActualBytes)
{
	int    ok = 1;
	uint32 actual_size = 0;
	THROW(f);
	if(sign > 0 && sign <= 0xffffU) {
		uint16 s = static_cast<uint16>(sign);
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

int SBuffer::ReadFromFile(FILE * f, uint sign)
{
	int    ok = 1;
	THROW(f);
	if(sign > 0 && sign <= 0xffffU) {
		uint16 s = static_cast<uint16>(sign);
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
SBaseBuffer & SBaseBuffer::Init()
{
	P_Buf = 0;
	Size = 0;
	return *this;
}

SBaseBuffer & SBaseBuffer::Destroy()
{
	ZFREE(P_Buf);
	Size = 0;
	return *this;
}

bool FASTCALL SBaseBuffer::IsEq(const SBaseBuffer & rS) const
{
	if(Size != rS.Size)
		return false;
	else if(Size && memcmp(P_Buf, rS.P_Buf, Size) != 0)
		return false;
	else
		return true;
}

bool FASTCALL SBaseBuffer::Copy(const SBaseBuffer & rS)
{
	if(Alloc(rS.Size)) {
		memcpy(P_Buf, rS.P_Buf, rS.Size);
		return true;
	}
	else
		return false;
}

int FASTCALL SBaseBuffer::Alloc(size_t sz)
{
	int    ok = -1;
	if(sz > Size) {
		char * p_temp = static_cast<char *>(SAlloc::R(P_Buf, sz));
		if(p_temp) {
			P_Buf = p_temp;
			Size = sz;
		}
		else
			ok = 0;
	}
	return ok;
}

void SBaseBuffer::Zero()
{
	memzero(P_Buf, Size);
}

void SBaseBuffer::Set(void * pBuf, size_t size)
{
	P_Buf = static_cast<char *>(pBuf);
	Size = size;
}

int SBaseBuffer::Put(size_t offs, const void * pSrc, size_t size)
{
	assert(pSrc);
	int    ok = 1;
    size_t new_size = offs + size;
    if(new_size <= Size || Alloc(new_size)) {
        memcpy(PTR8(P_Buf)+offs, pSrc, size);
    }
    else
		ok = 0;
	return ok;
}

int SBaseBuffer::Put(size_t offs, uint8 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, uint16 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, uint32 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, uint64 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, int8 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, int16 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, int32 v) { return Put(offs, &v, sizeof(v)); }
int SBaseBuffer::Put(size_t offs, int64 v) { return Put(offs, &v, sizeof(v)); }

int SBaseBuffer::Get(size_t offs, void * pDest, size_t size) const
{
	int    ok = 1;
    if((offs + size) <= Size) {
        memcpy(pDest, PTR8(P_Buf)+offs, size);
    }
    else
		ok = SLS.SetError(SLERR_BUFTOOSMALL);
	return ok;
}

int SBaseBuffer::Get(size_t offs, uint8 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, uint16 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, uint32 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, uint64 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, int8 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, int16 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, int32 & rV) const { return Get(offs, &rV, sizeof(rV)); }
int SBaseBuffer::Get(size_t offs, int64 & rV) const { return Get(offs, &rV, sizeof(rV)); }
//
//
//
SBinaryChunk::SBinaryChunk() : L(0)
{
	SBaseBuffer::Init();
}

SBinaryChunk::SBinaryChunk(const void * pData, size_t len) : L(0)
{
	SBaseBuffer::Init();
	Put(pData, len);
}

SBinaryChunk::~SBinaryChunk()
{
	SBaseBuffer::Destroy();
}

SBinaryChunk::SBinaryChunk(const SBinaryChunk & rS) : L(0)
{
	SBaseBuffer::Init();
	Put(rS.PtrC(), rS.Len());
}

SBinaryChunk & SBinaryChunk::Randomize(size_t len)
{
	if(len && Ensure(len))
		SLS.GetTLA().Rg.ObfuscateBuffer(P_Buf, Len());
	else
		Z();
	return *this;
}

bool FASTCALL SBinaryChunk::IsEq(const SBinaryChunk & rS) const
{
	const size_t _len = Len();
	return (_len == rS.Len() && (!_len || memcmp(P_Buf, rS.P_Buf, _len) == 0));
}

bool FASTCALL SBinaryChunk::IsEq(const void * pData, size_t len) const
{
	return (Len() == len && (!len || memcmp(P_Buf, pData, len) == 0));
}

bool FASTCALL SBinaryChunk::operator == (const SBinaryChunk & rS) const { return IsEq(rS); }
bool FASTCALL SBinaryChunk::operator != (const SBinaryChunk & rS) const { return !IsEq(rS); }

SBinaryChunk & FASTCALL SBinaryChunk::operator = (const SBinaryChunk & rS)
{
	Put(rS.PtrC(), rS.Len());
	return *this;
}

int SBinaryChunk::CheckInvariants() const
{
	assert(L <= Size);
	assert(P_Buf || Size == 0);
	return ((L <= Size) && (P_Buf || Size == 0));
}

SBinaryChunk & SBinaryChunk::Z()
{
	CheckInvariants();
	L = 0;
	return *this;
}

size_t SBinaryChunk::Len() const 
{ 
	CheckInvariants();
	return L; 
}

const void * SBinaryChunk::PtrC() const 
{ 
	CheckInvariants();
	return P_Buf; 
}

const void * SBinaryChunk::PtrC(size_t offs) const 
{ 
	CheckInvariants();
	return (offs < L) ? (PTR8C(P_Buf)+offs) : 0; 
}

void * SBinaryChunk::Ptr() 
{ 
	CheckInvariants();
	return P_Buf; 
}

void * SBinaryChunk::Ptr(size_t offs) 
{ 
	CheckInvariants();
	return (offs < L) ? (PTR8(P_Buf)+offs) : 0; 
}

SString & SBinaryChunk::ToRawStr(SString & rBuf) const
{
	return rBuf.Z().CatN(P_Buf, Len());
}

SString & SBinaryChunk::Mime64(SString & rBuf) const
{
	CheckInvariants();
	if(Len())
		rBuf.EncodeMime64(P_Buf, Len());
	else
		rBuf.Z();
	return rBuf;
}

int SBinaryChunk::FromMime64(const char * pMimeString)
{
	int    ok = 0;
	Z();
	if(!isempty(pMimeString)) {
		const size_t in_len = strlen(pMimeString);
		assert(in_len > 0); // see condition above
		THROW(Ensure((4+1) * in_len / 3)); // (+1) - extra insurance
		{
			assert(P_Buf); // (in_len > 0) and Ensure() garantee it
			size_t out_len = Len();
			ok = decode64(pMimeString, in_len, static_cast<char *>(P_Buf), &out_len);
			assert(out_len <= Len());
			if(ok)
				L = out_len;
			else
				Z();
		}
	}
	CATCHZOK
	return ok;
}

bool SBinaryChunk::Ensure(size_t len)
{
	bool   ok = true;
	CheckInvariants();
	if(len <= Size || Alloc(len))
		L = len;
	else
		ok = false;
	return ok;	
}

int SBinaryChunk::Set(uint8 byte, size_t len)
{
	int    ok = 1;
	CheckInvariants();
	if(len == 0)
		L = 0;
	else if(len <= Size || Alloc(len)) {
		memset(P_Buf, byte, len);
		L = len;
	}
	else
		ok = 0;
	return ok;	
}

bool SBinaryChunk::Put(const void * pData, size_t len)
{
	bool   ok = true;
	assert(pData || len == 0);
	CheckInvariants();
	if(len == 0 || !pData)
		L = 0;
	else if(len <= Size || Alloc(len)) {
		memcpy(P_Buf, pData, len);
		L = len;
	}
	else
		ok = false;
	return ok;
}

bool SBinaryChunk::Cat(uint8 byte)
{
	bool   ok = true;
	CheckInvariants();
	const  size_t new_len = L + 1;
	if(new_len <= Size || Alloc(new_len)) {
		PTR8(P_Buf)[L] = byte;
		L = new_len;
	}
	else
		ok = false;
	return ok;
}

bool SBinaryChunk::Cat(const void * pData, size_t len)
{
	bool   ok = true;
	CheckInvariants();
	if(len && pData) {
		const size_t new_len = L + len;
		if(new_len <= Size || Alloc(new_len)) {
			memcpy(PTR8(P_Buf)+L, pData, len);
			L = new_len;
		}
		else
			ok = false;
	}
	return ok;
}

bool SBinaryChunk::Cat(const SBinaryChunk & rS)
{
	return rS.Len() ? Cat(rS.PtrC(), rS.Len()) : true;
}
//
//
//
SBinarySet::SBinarySet() : DataLen(0)
{
	SBaseBuffer::Init();
}

SBinarySet::~SBinarySet()
{
	SBaseBuffer::Destroy();
}

SBinarySet::SBinarySet(const SBinarySet & rS) : DataLen(rS.DataLen)
{
	SBaseBuffer::Init();
	SBaseBuffer::Copy(rS);
}

SBinarySet & FASTCALL SBinarySet::operator = (const SBinarySet & rS)
{
	SBaseBuffer::Copy(rS);
	DataLen = rS.DataLen;
	return *this;
}

SBinarySet & SBinarySet::Z()
{
	DataLen = 0;
	return *this;
}

static const uint32 _BinarySetMagic = 0x5E4F7D1AU;

int SBinarySet::Ensure(size_t ensSize)
{
	return (Size < ensSize) ? Alloc(ensSize) : 1;
}

bool FASTCALL SBinarySet::Helper_GetChunkIdList(LongArray * pList) const
{
	bool   ok = true;
	CALLPTRMEMB(pList, Z());
	if(DataLen) {
		THROW(P_Buf);
		THROW(DataLen <= Size && DataLen >= sizeof(H));
		THROW(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic);
		{
			size_t offs = sizeof(H);
			BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
			do {
				if(pList) {
					THROW(pList->add(static_cast<long>(p_blk->I)));
				}
				offs += (sizeof(BH) + p_blk->S);
				THROW(offs <= DataLen);
				p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs);
			} while(offs < DataLen);
			THROW(offs == DataLen);
		}
	}
	CATCHZOK
	return ok;
}

bool SBinarySet::IsValid() const { return Helper_GetChunkIdList(0); }
int  FASTCALL SBinarySet::GetChunkIdList(LongArray & rList) const { return Helper_GetChunkIdList(&rList); }

bool FASTCALL SBinarySet::IsEq(const SBinarySet & rS) const
{
	bool   eq = true;
	LongArray id_list_my;
	LongArray id_list_other;
	if(GetChunkIdList(id_list_my) && rS.GetChunkIdList(id_list_other)) {
		if(id_list_my.getCount() != id_list_other.getCount())
			eq = false;
		else {
			id_list_my.sort();
			id_list_other.sort();
			if(!id_list_my.IsEq(&id_list_other))
				eq = false;
			else {
				SBinaryChunk cm;
				SBinaryChunk co;
				for(uint i = 0; eq && i < id_list_my.getCount(); i++) {
					uint cid = static_cast<uint>(id_list_my.get(i));
					assert(cid == id_list_other.get(i));
					int r1 = Get(cid, &cm);
					int r2 = rS.Get(cid, &co);
					assert(r1 > 0 && r2 > 0); // Не может такого быть, чтоб было иначе!
					if(r1 > 0 && r2 > 0) {
						if(!cm.IsEq(co))
							eq = false;
					}
					else
						eq = false;
				}
			}
		}
	}
	else
		eq = false;
	return eq;
}

int SBinarySet::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		THROW(Pack());
		{
			uint32 _datalen = DataLen;
			if(pSCtx) {
				THROW(pSCtx->Serialize(dir, _datalen, rBuf));
				if(_datalen) {
					THROW(rBuf.Write(P_Buf, _datalen));
				}
			}
			else {
				THROW(rBuf.Write(P_Buf, DataLen));
			}
		}
	}
	else if(dir < 0) {
		uint32 _datalen;
		Z();
		if(pSCtx) {
			THROW(pSCtx->Serialize(dir, _datalen, rBuf));
			if(_datalen) {
				THROW_S(_datalen >= sizeof(H) && reinterpret_cast<const H *>(rBuf.GetBufC(rBuf.GetRdOffs()))->Magic == _BinarySetMagic, SLERR_BINSET_UNSRLZ_SIGNATURE);
				THROW(SBaseBuffer::Alloc(_datalen));
				THROW_S(rBuf.Read(P_Buf, _datalen) == _datalen, SLERR_SBUFRDSIZE);
				DataLen = _datalen;
				assert(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic);
			}
		}
		else {
			_datalen = rBuf.GetAvailableSize();
			if(_datalen) {
				THROW_S(_datalen >= sizeof(H) && reinterpret_cast<const H *>(rBuf.GetBufC(rBuf.GetRdOffs()))->Magic == _BinarySetMagic, SLERR_BINSET_UNSRLZ_SIGNATURE);
				THROW(SBaseBuffer::Alloc(_datalen));
				THROW_S(rBuf.Read(P_Buf, _datalen) == _datalen, SLERR_SBUFRDSIZE);
				DataLen = _datalen;
				assert(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SBinarySet::SetOuterBuffer(const void * pOuterData, size_t outerDataLen)
{
	Z();
	int    ok = 1;
	if(pOuterData && outerDataLen) {
		THROW(outerDataLen >= sizeof(H));
		THROW(static_cast<const H *>(pOuterData)->Magic == _BinarySetMagic);
		THROW(Ensure(outerDataLen));
		memcpy(P_Buf, pOuterData, outerDataLen);
		DataLen = outerDataLen;
		THROW(IsValid());
	}
	CATCH
		Z();
		ok = 0;
	ENDCATCH
	return ok;
}

int SBinarySet::Pack()
{
	int    ok = -1;
	assert(DataLen <= SBaseBuffer::Size);
	if(DataLen) {
		bool is_there_empty_chunks = false;
		THROW(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic);
		{
			size_t offs = sizeof(H);
			BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
			do {
				if(p_blk->I == 0) {
					is_there_empty_chunks = true;
				}
				offs += (sizeof(BH) + p_blk->S);
				p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs);
			} while(!is_there_empty_chunks && offs < DataLen);
		}
		if(is_there_empty_chunks) {
			SBinarySet temp;
			size_t offs = sizeof(H);
			BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
			do {
				if(p_blk->I) {
					THROW(temp.Put(p_blk->I, p_blk+1, p_blk->S));
				}
				offs += (sizeof(BH) + p_blk->S);
				p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs);
			} while(offs < DataLen);
			assert(offs == DataLen);
			*this = temp;
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

const void * SBinarySet::GetPtr(uint32 id, uint32 * pSize) const
{
	const void * p_result = 0;
	uint32 size = 0;
	assert(DataLen <= SBaseBuffer::Size);
	if(id && DataLen) {
		assert(DataLen >= sizeof(H));
		if(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic) {
			size_t offs = sizeof(H);
			if(offs < DataLen) { // @v11.2.0
				BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
				do {
					if(p_blk->I == id) {
						size = p_blk->S;
						p_result = p_blk+1;
					}
					else {
						offs += (sizeof(BH) + p_blk->S);
						p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs);
					}
				} while(!p_result && offs < DataLen);
			}
			assert(p_result || offs == DataLen);
		}
	}
	ASSIGN_PTR(pSize, size);
	return p_result;
}

int SBinarySet::CopyFrom(const SBinarySet & rS, const LongArray & rIdList, bool errOnAbsenseAny)
{
	int    ok = -1;
	if(rIdList.getCount()) {
		SBinaryChunk temp_bch;
		if(errOnAbsenseAny) {
			//
			// Требование наличия всех идентификаторов проверяем предварительно дабы не ломать содержимое this в случае ошибки
			//
			SString & r_temp_buf = SLS.AcquireRvlStr();
			for(uint i = 0; i < rIdList.getCount(); i++) {
				const uint cid = static_cast<uint>(rIdList.get(i));
				THROW_S_S(rS.Get(rIdList.get(i), 0), SLERR_BINSET_SRCIDNFOUND, r_temp_buf.Z().Cat(cid));
			}
		}
		{
			for(uint i = 0; i < rIdList.getCount(); i++) {
				const uint cid = static_cast<uint>(rIdList.get(i));
				if(rS.Get(cid, &temp_bch)) {
					int r = Put(cid, temp_bch);
					THROW(r);
					if(r > 0)
						ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SBinarySet::Enum(size_t * pPos, uint32 * pId, SBinaryChunk * pResult) const
{
	int    ok = 0;
	assert(DataLen <= SBaseBuffer::Size);
	size_t cur_pos = pPos ? *pPos : 0;
	if(DataLen) {
		assert(DataLen >= sizeof(H));
		if(reinterpret_cast<const H *>(P_Buf)->Magic == _BinarySetMagic) {
			SETIFZ(cur_pos, sizeof(BH));
			if(cur_pos < DataLen) {
				const BH * p_blk = reinterpret_cast<const BH *>(PTR8C(P_Buf) + cur_pos);
				assert((cur_pos + p_blk->S) <= DataLen);
				if((cur_pos + p_blk->S) <= DataLen) {
					if(pResult) {
						pResult->Put(p_blk+1, p_blk->S); // @v11.0.10 @fix p_blk-->(p_blk+1)
					}
					ASSIGN_PTR(pId, p_blk->I);
					if(pPos) {
						cur_pos += (sizeof(BH) + p_blk->S);
						*pPos = cur_pos;
					}
					ok = 1;
				}
			}
		}
	}
	return ok;
}

bool SBinarySet::Get(uint32 id, SBinaryChunk * pResult) const
{
	int    ok = true;
	uint32 size = 0;
	const void * ptr = GetPtr(id, &size);
	THROW(ptr);
	if(pResult) {
		const size_t compress_prefix_size = SSerializeContext::GetCompressPrefix(0);
		if(size > compress_prefix_size && SSerializeContext::IsCompressPrefix(ptr)) {
			SCompressor compr(SCompressor::tZLib);
			SBuffer dbuf;
			THROW(compr.DecompressBlock(PTR8C(ptr)+compress_prefix_size, size-compress_prefix_size, dbuf));
			THROW(pResult->Put(dbuf.GetBuf(), dbuf.GetAvailableSize()));
		}
		else {
			THROW(pResult->Put(ptr, size));
		}
	}
	CATCHZOK
	return ok;
}

int SBinarySet::Put(uint32 id, const SBinaryChunk & rData, const DeflateStrategy * pDs/*= 0*/)
{
	return Put(id, rData.PtrC(), rData.Len(), pDs);
}

int SBinarySet::Put(uint32 id, const void * pData, uint32 size, const DeflateStrategy * pDs/*= 0*/)
{
	int    ok = 0;
	assert(id);
	if(id) {
		const bool do_remove = (!pData || !size); // Если указатель pData == 0 или размер size == 0, то считаем это требованием удалить блок
			// с заданным идентификатором.
		assert(DataLen <= SBaseBuffer::Size);
		if(pDs && size >= static_cast<uint32>(pDs->MinChunkSizeToCompress)) { // @v11.2.11
			assert(!do_remove);
			SCompressor compr(SCompressor::tZLib);
			SSerializeContext sctx;
			uint8 cs[32];
			size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
			SBuffer cbuf;
			THROW(cbuf.Write(cs, cs_size));
			THROW(compr.CompressBlock(pData, size, cbuf, 0, 0));
			THROW(Put(id, cbuf.GetBuf(), cbuf.GetAvailableSize(), 0/*!*/)); // @recursion
			ok = 1;
		}
		else {
			if(DataLen == 0) {
				if(!do_remove) {
					const size_t new_data_len = (size + sizeof(H) + sizeof(BH));
					if(SBaseBuffer::Alloc(new_data_len)) {
						reinterpret_cast<H *>(P_Buf)->Magic = _BinarySetMagic;
						reinterpret_cast<H *>(P_Buf)->Flags = 0;
						BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
						p_blk->I = id;
						p_blk->S = size;
						memcpy(p_blk+1, pData, size);
						DataLen = new_data_len;
						ok = 1;
					}
				}
				else
					ok = -1;
			}
			else {
				assert(DataLen >= sizeof(H));
				assert(P_Buf);
				if(reinterpret_cast<H *>(P_Buf)->Magic == _BinarySetMagic) {
					size_t offs = sizeof(H);
					size_t suited_unused_block_offs = 0; // Если по ходу перебора блоков мы встретим неиспользуемы блок требуемого размера,
						// то здесь сохраним его смещение, дабы, если не найдем существующего блока требуемого размера, то используем этот.
						// Смещение найденного таким образом блока мы увеличим на sizeof(BH) дабы отличить от 0 (отсутствие такового блока).
					BH * p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + sizeof(H));
					do {
						if(p_blk->I == id) {
							// Мы натолкнулись на уже существующий блок с нашим идентификатором
							if(do_remove) {
								// Если требуется просто удалить блок, то объявляем его неиспользуемым и уходим
								p_blk->I = 0;
								memzero(p_blk+1, p_blk->S);
								ok = 1;
							}
							else if(p_blk->S == size) {
								// Если размер существующего блока равен требуемому, то просто копируем новые данные и уходим: все сделано.
								if(memcmp(p_blk+1, pData, size) == 0) { // Чтобы вызывающая функция могла узнать изменилось что-либо или нет
									// сравним исходный блок с тем, что уже находится в пуле. Если они эквивалентны, то возвращает -1.
									ok = -1;
								}
								else {
									memcpy(p_blk+1, pData, size);
									ok = 1;
								}
							}
							else {
								// Если размер существующего блока отличается от требуемого, то существующий блок объявляем неиспользуемым
								// и продолжаем поиск.
								p_blk->I = 0;
								memzero(p_blk+1, p_blk->S);
							}
						}
						else if(!do_remove && p_blk->I == 0 && p_blk->S == size) {
							suited_unused_block_offs = offs+sizeof(BH);
						}
						offs += (sizeof(BH) + p_blk->S);
						p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs);
					} while(!ok && offs < DataLen);
					assert(ok || offs == DataLen);
					if(do_remove) {
						if(!ok)
							ok = -1;
					}
					else if(!ok) {
						if(suited_unused_block_offs) {
							assert(!do_remove);
							p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + suited_unused_block_offs - sizeof(BH));
							assert(p_blk->S == size);
							p_blk->S = size;
							p_blk->I = id;
							memcpy(p_blk+1, pData, size);
							ok = 1;
						}
						else {
							assert(PTR8C(p_blk) - PTR8C(P_Buf) == offs);
							const size_t new_data_len = (DataLen + sizeof(BH) + size);
							if(SBaseBuffer::Alloc(new_data_len)) {
								p_blk = reinterpret_cast<BH *>(PTR8(P_Buf) + offs); // ! Расположение буфера могло измениться
								p_blk->S = size;
								p_blk->I = id;
								memcpy(p_blk+1, pData, size);
								DataLen = new_data_len;
								ok = 1;
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
//
//
//
STempBuffer::STempBuffer(size_t sz)
{
	Init();
	Alloc(sz);
}

STempBuffer::~STempBuffer()
{
	Destroy();
}

int FASTCALL STempBuffer::Alloc(size_t sz)
{
	int    ok = 1;
	if(sz == 0) {
		ZFREE(P_Buf);
		Size = 0;
	}
	else if(Size != sz) {
		char * p = static_cast<char *>(SAlloc::R(P_Buf, sz));
		if(p) {
			P_Buf = p;
			Size = sz;
		}
		else {
			Size = 0;
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL STempBuffer::AllocIncr(size_t sz)
{
	return (sz > Size) ? Alloc(sz) : -1;
}

STempBuffer & FASTCALL STempBuffer::operator = (const STempBuffer & rS)
{
	SBaseBuffer::Copy(rS);
	return *this;
}
//
//
//
struct SscDbtItem {
	uint32 DbtID;
	BNFieldList Fields;
};

/*static*/size_t FASTCALL SSerializeContext::GetCompressPrefix(uint8 * pBuf) // size of pBuf >= 8
{
	if(pBuf)
		*reinterpret_cast<uint64 *>(pBuf) = _SlConst.Ssc_CompressionSignature;
	return sizeof(_SlConst.Ssc_CompressionSignature);
}

/*static*/int  FASTCALL SSerializeContext::IsCompressPrefix(const void * pBuf)
{
	return BIN(*reinterpret_cast<const uint64 *>(pBuf) == _SlConst.Ssc_CompressionSignature);
}

SSerializeContext::SSerializeContext() : SymbTbl(2048, 1), TempDataBuf(0), LastSymbId(0), SuppDate(ZERODATE), State(0), Flags(0), P_DbtDescrList(0)
{
}

SSerializeContext::~SSerializeContext()
{
	ZDELETE(P_DbtDescrList);
}

void SSerializeContext::Init(long flags, LDATE suppDate)
{
	Flags |= flags;
	SuppDate = suppDate;
	CALLPTRMEMB(P_DbtDescrList, freeAll());
	LastSymbId = 0;
	SymbTbl.Clear();
}

bool FASTCALL SSerializeContext::CheckFlag(long f) const { return LOGIC(Flags & f); }
LDATE SSerializeContext::GetSupportingDate() const { return SuppDate; }

int SSerializeContext::AddDbtDescr(const char * pName, const BNFieldList * pList, uint32 * pID)
{
	int    ok = -1;
	(TempStrBuf = pName).ToUpper();
	uint   dbt_id = 0;
	if(!SymbTbl.Search(TempStrBuf, &dbt_id, 0)) {
		dbt_id = ++LastSymbId;
		SymbTbl.Add(TempStrBuf, dbt_id, 0);
		SscDbtItem * p_new_item = new SscDbtItem;
		THROW_S(p_new_item, SLERR_NOMEM);
		p_new_item->DbtID = dbt_id;
		p_new_item->Fields = *pList;
		THROW(SETIFZ(P_DbtDescrList, new TSCollection <SscDbtItem>));
		THROW(P_DbtDescrList->insert(p_new_item));
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pID, dbt_id);
	return ok;
}

int SSerializeContext::GetDbtDescr(uint id, BNFieldList * pList) const
{
	int    ok = 0;
	if(P_DbtDescrList) {
		for(uint i = 0; !ok && i < P_DbtDescrList->getCount(); i++) {
			const SscDbtItem * p_item = (const SscDbtItem *)P_DbtDescrList->at(i);
			if(p_item->DbtID == id) {
				ASSIGN_PTR(pList, p_item->Fields);
				ok = 1;
			}
		}
	}
	return ok;
}

int SSerializeContext::SerializeFieldList(int dir, BNFieldList * pFldList, SBuffer & rBuf)
{
	assert(dir != 0);
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

int SSerializeContext::SerializeStateOfContext(int dir, SBuffer & rBuf)
{
	assert(dir != 0);
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
		blk.StructCount = SVectorBase::GetCount(P_DbtDescrList);
		rBuf.Write(sign);
		rBuf.Write(&blk, sizeof(blk));
		if(Flags & fSeparateDataStruct) {
			for(uint i = 0; i < blk.StructCount; i++) {
				assert(P_DbtDescrList);
				SscDbtItem * p_item = static_cast<SscDbtItem *>(P_DbtDescrList->at(i));
				SymbTbl.GetByAssoc(p_item->DbtID, TempStrBuf);
				rBuf.Write(TempStrBuf);
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
			if(blk.StructCount) {
				if(P_DbtDescrList)
					P_DbtDescrList->freeAll();
				else {
					THROW(P_DbtDescrList = new TSCollection <SscDbtItem>);
				}
				for(uint i = 0; i < blk.StructCount; i++) {
					SscDbtItem * p_new_item = new SscDbtItem;
					THROW(p_new_item);
					THROW(rBuf.Read(TempStrBuf));
					THROW(rBuf.Read(p_new_item->DbtID));
					THROW(SerializeFieldList(-1, &p_new_item->Fields, rBuf));
					THROW(P_DbtDescrList->insert(p_new_item));
					THROW(SymbTbl.Add(TempStrBuf, p_new_item->DbtID, 0));
				}
			}
			else
				ZDELETE(P_DbtDescrList);
		}
	}
	CATCHZOK
	return ok;
}

int SSerializeContext::Serialize(int dir, TYPEID typ, void * pData, uint8 * pInd, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	DataType _t;
	if(pInd) {
		THROW(stype(typ, &_t).Serialize(dir, pData, pInd, rBuf, this));
	}
	else {
		uint8  ind = 0;
		if(dir > 0) {
			TempSBuf.Z();
			THROW(stype(typ, &_t).Serialize(dir, pData, &ind, TempSBuf, this));
			THROW(rBuf.Write(&ind, sizeof(ind)));
			const size_t avsz = TempSBuf.GetAvailableSize();
			if(avsz)
				THROW(rBuf.Write(TempSBuf.GetBuf(TempSBuf.GetRdOffs()), avsz));
		}
		else if(dir < 0) {
			THROW(rBuf.ReadV(&ind, sizeof(ind)));
			THROW(stype(typ, &_t).Serialize(dir, pData, &ind, rBuf, this));
		}
	}
	CATCHZOK
	return ok;
}

int SSerializeContext::Serialize(int dir, int64  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, int32  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, int16  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, int8   & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, int    & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_INT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, uint   & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, uint64 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, uint32 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, uint16 & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, uint8  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UINT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, LDATE  & rV, SBuffer & rBuf) { return Serialize(dir, T_DATE, &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, LTIME  & rV, SBuffer & rBuf) { return Serialize(dir, T_TIME, &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, LDATETIME & rV, SBuffer & rBuf) { return Serialize(dir, T_DATETIME, &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, float  & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FLOAT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, double & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FLOAT, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, S_GUID & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_UUID_, sizeof(rV)), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, SColor & rV, SBuffer & rBuf) { return Serialize(dir, T_COLOR_RGBA, &rV, 0, rBuf); } // @v10.9.10
int SSerializeContext::Serialize(int dir, SPoint2S & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_IPOINT2, 4), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, SPoint2F & rV, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_FPOINT2, 8), &rV, 0, rBuf); }
int SSerializeContext::Serialize(int dir, FRect & rV, SBuffer & rBuf) 
{ 
	return (Serialize(dir, MKSTYPE(S_FPOINT2, 8), &rV.a, 0, rBuf) && Serialize(dir, MKSTYPE(S_FPOINT2, 8), &rV.b, 0, rBuf));
}
int SSerializeContext::Serialize(int dir, char * pV, size_t valBufLen, SBuffer & rBuf) { return Serialize(dir, MKSTYPE(S_ZSTRING, valBufLen), pV, 0, rBuf); }

int SSerializeContext::Serialize(int dir, DateRange & rV, SBuffer & rBuf)
{
	assert(dir != 0);
    int    ok = 1;
    THROW(Serialize(dir, rV.low, rBuf));
    THROW(Serialize(dir, rV.upp, rBuf));
    CATCHZOK
    return ok;
}

int SSerializeContext::Serialize(const char * pDbtName, BNFieldList * pFldList, const void * pData, SBuffer & rBuf)
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
			THROW(p_ind_list = static_cast<uint8 *>(SAlloc::M(ind_len)));
			own_ind_list = 1;
		}
		else
			p_ind_list = st_ind_list;
		memzero(p_ind_list, ind_len);
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pFldList->getField(i);
			THROW(Serialize(+1, r_fld.T, const_cast<uint8 *>(PTR8C(pData)+r_fld.Offs), p_ind_list+i, temp_buf)); // @badcast
		}
		rBuf.Write(p_ind_list, ind_len);
		rBuf.Write(temp_buf);
	}
	CATCHZOK
	if(own_ind_list)
		SAlloc::F(p_ind_list);
	return ok;
}

int SSerializeContext::Unserialize(const char * pDbtName, const BNFieldList * pFldList, void * pData, SBuffer & rBuf)
{
	int    ok = 1, r;
	uint32 dbt_id = 0;
	uint16 temp_val = -999; // -999 - умышленный мусор
	uint8 * p_ind_list = 0;
	uint8  st_ind_list[512];
	int    own_ind_list = 0;
	BNFieldList inner_fld_list;
	THROW(rBuf.Read(dbt_id));
	THROW(rBuf.Read(temp_val));
	THROW_S(oneof2(temp_val, 0, 1), SLERR_SRLZ_COMMRDFAULT);
	// @v9.8.11 THROW(r = GetDbtDescr(dbt_id, &inner_fld_list));
	r = GetDbtDescr(dbt_id, &inner_fld_list); // @v9.8.11
	if(r > 0) {
		if(temp_val == 1) {
			BNFieldList temp_flist;
			THROW(SerializeFieldList(-1, &temp_flist, rBuf));
			THROW_S(temp_flist.IsEq(inner_fld_list), SLERR_SRLZ_UNEQFLDLIST);
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
		if(ind_len > SIZEOFARRAY(st_ind_list)) {
			//
			// Страховка от случая, когда количество полей больше, чем 512
			//
			THROW(p_ind_list = static_cast<uint8 *>(SAlloc::M(ind_len)));
			own_ind_list = 1;
		}
		else
			p_ind_list = st_ind_list;
		THROW(rBuf.ReadV(p_ind_list, ind_len));
		THROW(rBuf.Read(temp_buf));
		if(pFldList) {
			const int is_eq_struc = inner_fld_list.IsEq(*pFldList);
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
		SAlloc::F(p_ind_list);
	return ok;
}

int SSerializeContext::Serialize(int dir, SStrCollection * pColl, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	SString temp_buf;
	if(dir > 0) {
		//uint32 c = pColl ? pColl->getCount() : 0;
		const uint32 c = SVectorBase::GetCount(pColl);
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

int SSerializeContext::Serialize(int dir, SArray * pArray, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	if(dir > 0) {
		ok = rBuf.Write(pArray, SBuffer::ffAryCount32);
	}
	else if(dir < 0) {
		ok = rBuf.Read(pArray, SBuffer::ffAryCount32);
	}
	return ok;
}

int SSerializeContext::Serialize(int dir, SVector * pArray, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	if(dir > 0) {
		ok = rBuf.Write(pArray, SBuffer::ffAryCount32);
	}
	else if(dir < 0) {
		ok = rBuf.Read(pArray, SBuffer::ffAryCount32);
	}
	return ok;
}

int SSerializeContext::Serialize(int dir, StrAssocArray & rArray, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	if(dir > 0) {
		ok = rArray.Write(rBuf, 0);
	}
	else if(dir < 0) {
		ok = rArray.Read(rBuf, 0);
	}
	return ok;
}

int SSerializeContext::Serialize(int dir, SString & rStr, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	if(dir > 0) {
		ok = rBuf.Write(rStr);
	}
	else if(dir < 0) {
		ok = rBuf.Read(rStr);
	}
	return ok;
}

int SSerializeContext::Serialize(int dir, SStringU & rStr, SBuffer & rBuf)
{
	assert(dir != 0);
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

int SSerializeContext::Serialize(int dir, SBinaryChunk & rBc, SBuffer & rBuf)
{
	assert(dir != 0);
	int    ok = 1;
	uint32 sz;
	if(dir > 0) {
		sz = rBc.Len();
		THROW(rBuf.Write(sz));
		if(sz) {
			THROW(rBuf.Write(rBc.PtrC(), sz));
		}
	}
	else if(dir < 0) {
		THROW(rBuf.Read(sz));
		if(sz) {
			THROW(rBc.Ensure(sz));
			THROW(rBuf.ReadV(rBc.Ptr(), sz));
		}
		else {
			rBc.Z();
		}
	}
	CATCHZOK
	return ok;
}

int SSerializeContext::SerializeBlock(int dir, uint32 size, void * pData, SBuffer & rBuf, int skipMissizedBlock)
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

void SSerializeContext::SetLocalSignatureTag(uint32 signature, uint32 tag)
{
	LocalSignatureTagList.Update(static_cast<long>(signature), static_cast<long>(tag), 0);
}

void SSerializeContext::ResetLocalSignatureTag(uint32 signature)
{
	LocalSignatureTagList.Remove(static_cast<long>(signature), 0);
}

int SSerializeContext::GetLocalSignatureTag(uint32 signature, uint32 * pTag) const
{
	long   val = 0;
	if(LocalSignatureTagList.Search(signature, &val, 0)) {
		ASSIGN_PTR(pTag, static_cast<uint32>(val));
		return 1;
	}
	else
		return 0;
}
//
//
//
SSerializer::SSerializer(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) : R_Buf(rBuf), Dir(dir), P_SCtx(pSCtx)
{
}

int STDCALL  SSerializer::Serialize(TYPEID typ, void * pData, uint8 * pInd) { return P_SCtx->Serialize(Dir, typ, pData, pInd, R_Buf); }
int FASTCALL SSerializer::Serialize(SString & rStr) { return P_SCtx->Serialize(Dir, rStr, R_Buf); }
int FASTCALL SSerializer::Serialize(SStringU & rStr) { return P_SCtx->Serialize(Dir, rStr, R_Buf); }
int FASTCALL SSerializer::Serialize(int64 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(int32 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(int16 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(int8 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(int & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(uint64 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(uint32 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(uint16 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(uint8 & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(LDATE & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(LTIME & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(LDATETIME & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(float & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(double & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(S_GUID & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int STDCALL  SSerializer::Serialize(char * pV, size_t valBufLen) { return P_SCtx->Serialize(Dir, pV, valBufLen, R_Buf); }
int FASTCALL SSerializer::SerializeFieldList(BNFieldList * pFldList) { return P_SCtx->SerializeFieldList(Dir, pFldList, R_Buf); }
int FASTCALL SSerializer::Serialize(SPoint2S & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(SPoint2F & rV) { return P_SCtx->Serialize(Dir, rV, R_Buf); }
int FASTCALL SSerializer::Serialize(SArray * pArray) { return P_SCtx->Serialize(Dir, pArray, R_Buf); }
int FASTCALL SSerializer::Serialize(SStrCollection * pColl) { return P_SCtx->Serialize(Dir, pColl, R_Buf); }
int FASTCALL SSerializer::Serialize(StrAssocArray & rArray) { return P_SCtx->Serialize(Dir, rArray, R_Buf); }
int STDCALL SSerializer::SerializeBlock(uint32 size, void * pData, int skipMissizedBlock) { return P_SCtx->SerializeBlock(Dir, size, pData, R_Buf, skipMissizedBlock); }
//
//
//
SBufferPipe::SBufferPipe(size_t initSize, long flags) : SBuffer(initSize, flags), Status(0)
{
}

int FASTCALL SBufferPipe::Put(const void * pSrc, size_t srcLen)
{
	Lck.Lock();
	int    ok = SBuffer::Write(pSrc, srcLen);
	Lck.Unlock();
	return ok;
}

size_t SBufferPipe::GetAvailableSize()
{
	Lck.Lock();
	size_t s = SBuffer::GetAvailableSize();
	Lck.Unlock();
	return s;
}

size_t FASTCALL SBufferPipe::Get(void * pBuf, size_t bufLen)
{
	Lck.Lock();
	size_t s = SBuffer::Read(pBuf, bufLen);
	Lck.Unlock();
	return s;
}

void SBufferPipe::Reset()
{
	Lck.Lock();
	SBuffer::Z();
	Status = 0;
	Lck.Unlock();
}

long SBufferPipe::GetStatus()
{
	Lck.Lock();
	long st = Status;
	Lck.Unlock();
	return st;
}

void SBufferPipe::SetStatus(long st, int set)
{
	Lck.Lock();
	SETFLAG(Status, st, set);
	Lck.Unlock();
}

#if SLTEST_RUNNING // {

SLTEST_R(SBuffer)
{
	{
		SString temp_buf;
		{
			SBuffer sbuf;
			long   lval;
			double rval;
			{
				SLTEST_CHECK_Z(sbuf.GetAvailableSize());
				SLTEST_CHECK_Z(sbuf.GetBuf());
				SLTEST_CHECK_Z(sbuf.GetWrOffs());
				SLTEST_CHECK_Z(sbuf.GetRdOffs());
			}
			{
				sbuf.Write(1L);
				sbuf.Write(17.0);
				SLTEST_CHECK_EQ(sbuf.GetWrOffs(), sizeof(1L)+sizeof(17.0));
				SLTEST_CHECK_Z(sbuf.GetRdOffs());
				SLTEST_CHECK_NZ(sbuf.Read(lval));
				SLTEST_CHECK_NZ(sbuf.Read(rval));
				SLTEST_CHECK_EQ(lval, 1L);
				SLTEST_CHECK_EQ(rval, 17.0);
				SLTEST_CHECK_EQ(sbuf.GetRdOffs(), sizeof(lval)+sizeof(rval));
			}
			{
				sbuf.Z(); // Сбрасываем буфер - все должно быть теперь по-нулям (указатель на буфер - нет: не нулевой)
				SLTEST_CHECK_Z(sbuf.GetAvailableSize());
				SLTEST_CHECK_Z(sbuf.GetWrOffs());
				SLTEST_CHECK_Z(sbuf.GetRdOffs());
			}
			{
				const char * p_terminated_string = "Некоторая строка у которой есть терминатор";
				const char * p_terminated_string_part2 = "хвост строки с внутренним терминатором";
				const char * pp_term_list[] = { "\xD", "\xD\xA", "\xA", "перекинь копыто через забор" };
				for(uint i = 0; i < SIZEOFARRAY(pp_term_list); i++) {
					const char * p_term = pp_term_list[i];
					temp_buf.Z().Cat(p_terminated_string).Cat(p_term).Cat(p_terminated_string_part2);
					sbuf.Z();
					sbuf.Write(temp_buf.cptr(), temp_buf.Len());
					sbuf.ReadTermStr(p_term, temp_buf);
					SLTEST_CHECK_EQ(sbuf.GetRdOffs(), sstrlen(p_terminated_string)+sstrlen(p_term));
					SLTEST_CHECK_EQ(sbuf.GetAvailableSize(), sstrlen(p_terminated_string_part2));
					SLTEST_CHECK_EQ(sbuf.GetAvailableSize(), sbuf.GetAvailableSizeI());
					SLTEST_CHECK_Z(temp_buf.CmpSuffix(p_term, 0));
					temp_buf.Trim(temp_buf.Len()-strlen(p_term));
					SLTEST_CHECK_EQ(temp_buf, p_terminated_string);
				}
			}
		}
		{
			SString js_file_name;
			SString js_text;
			js_file_name = this->GetSuiteEntry()->InPath;
			//D:\Papyrus\Src\PPTEST\DATA\json\bigjson.json 
			js_file_name.SetLastSlash().Cat("json").SetLastSlash().Cat("bigjson.json");
			if(fileExists(js_file_name)) {
				SFile f_in(js_file_name, SFile::mRead);
				if(f_in.IsValid()) {
					STempBuffer rb(SKILOBYTE(512));
					size_t rs = 0;
					while(f_in.Read(rb, rb.GetSize(), &rs)) {
						js_text.CatN(rb.cptr(), rs);
						if(rs < rb.GetSize())
							break;
					}
					SJson * p_js = SJson::Parse(js_text);
					if(p_js) {
						SBinarySet set;
						// 1, 2, 3, 4, 5, 6
						const char * p_text_chunks[] = { "one", "two", "three", "four", "five" };
						{
							SBinarySet::DeflateStrategy ds(512);
							for(uint i = 0; i < SIZEOFARRAY(p_text_chunks); i++) {
								const char * p_text = p_text_chunks[i];
								SLTEST_CHECK_NZ(set.Put(i+1, p_text, sstrlen(p_text)));
							}
							SLTEST_CHECK_NZ(set.Put(SIZEOFARRAY(p_text_chunks)+1, js_text, js_text.Len(), &ds));
						}
						{
							SBinaryChunk chunk;
							for(uint i = 0; i < SIZEOFARRAY(p_text_chunks); i++) {
								SLTEST_CHECK_NZ(set.Get(i+1, &chunk));
								SLTEST_CHECK_NZ(chunk.Len());
								temp_buf.Z().CatN(static_cast<const char *>(chunk.PtrC()), chunk.Len());
								SLTEST_CHECK_EQ(temp_buf, p_text_chunks[i]);
							}
							SLTEST_CHECK_NZ(set.Get(SIZEOFARRAY(p_text_chunks)+1, &chunk));
							temp_buf.Z().CatN(static_cast<const char *>(chunk.PtrC()), chunk.Len());
							SLTEST_CHECK_EQ(temp_buf, js_text);
						}
						ZDELETE(p_js); // @v11.3.11 @fix
					}
				}
			}
		}
	}
	{
		//
		// Проверено отключение блокировок в SBufferPipe - моментальное исключение по доступу к памяти
		//
		static const char p_eot_string[] = "end of transmission!";
		int    ok = 1;
		SFile file;
		SString in_file_name(MakeInputFilePath("binfile"));
		SString out_file_name(MakeOutputFilePath("bufferpipe-result"));
		SBufferPipe pipe;

		class ThreadReader : public SlThread {
		public:
			ThreadReader(SBufferPipe * pPipe, const char * pOutFileName) : SlThread(), P_Pipe(pPipe), OutFileName(pOutFileName)
			{
			}
		private:
			virtual void Run()
			{
				assert(SLS.GetConstTLA().Id == GetThreadID());
				uint64 total_rd = 0;
				uint64 total_wr = 0;
				SFile f_out(OutFileName, SFile::mWrite|SFile::mBinary);
				if(f_out.IsValid()) {
					uint8  temp_buf[1024];
					while(1) {
						size_t actual_size = P_Pipe->Get(temp_buf, sizeof(temp_buf));
						total_rd += actual_size;
						if(actual_size) {
							f_out.Write(temp_buf, actual_size);
							total_wr += actual_size;
						}
						else if(P_Pipe->GetStatus() & SBufferPipe::statusEOT)
							break;
					};
				}
			}
			SString OutFileName;
			SBufferPipe * P_Pipe;
		};
		class ThreadWriter : public SlThread {
		public:
			ThreadWriter(SBufferPipe * pPipe, const char * pInFileName) : SlThread(), P_Pipe(pPipe), InFileName(pInFileName)
			{
			}
		private:
			virtual void Run()
			{
				assert(SLS.GetConstTLA().Id == GetThreadID());
				uint64 total_rd = 0;
				uint64 total_wr = 0;
				SFile f_in(InFileName, SFile::mRead|SFile::mBinary);
				if(f_in.IsValid()) {
					uint8  temp_buf[1024];
					size_t actual_sz = 0;
					do {
						size_t sz = SLS.GetTLA().Rg.GetUniformInt(sizeof(temp_buf));
						assert(sz >= 0 && sz < sizeof(temp_buf));
						SETIFZ(sz, 1);
						f_in.Read(temp_buf, sz, &actual_sz);
						total_rd += actual_sz;
						if(actual_sz) {
							P_Pipe->Put(temp_buf, actual_sz);
							total_wr += actual_sz;
						}
					} while(actual_sz);
					P_Pipe->SetStatus(SBufferPipe::statusEOT, 1);
				}
			}
			SString InFileName;
			SBufferPipe * P_Pipe;
		};
		for(uint i = 0; i < 4; i++) {
			SFile::Remove(out_file_name);
			pipe.Reset();
			//
			HANDLE objs_to_wait[8];
			MEMSZERO(objs_to_wait);
			size_t objs_to_wait_count = 0;
			ThreadWriter * p_thr_wrr = new ThreadWriter(&pipe, in_file_name);
			ThreadReader * p_thr_rdr = new ThreadReader(&pipe, out_file_name);
			p_thr_wrr->Start();
			objs_to_wait[objs_to_wait_count++] = *p_thr_wrr;
			p_thr_rdr->Start();
			objs_to_wait[objs_to_wait_count++] = *p_thr_rdr;
			WaitForMultipleObjects(objs_to_wait_count, objs_to_wait, TRUE, INFINITE);
			SLTEST_CHECK_LT(0L, SFile::Compare(in_file_name, out_file_name, 0));
			pipe.Reset();
		}
	}
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
