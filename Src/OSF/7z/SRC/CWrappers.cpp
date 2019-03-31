// CWrappers.h

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

SRes HRESULT_To_SRes(HRESULT res, SRes defaultRes) throw()
{
	switch(res) {
		case S_OK: return SZ_OK;
		case E_OUTOFMEMORY: return SZ_ERROR_MEM;
		case E_INVALIDARG: return SZ_ERROR_PARAM;
		case E_ABORT: return SZ_ERROR_PROGRESS;
		case S_FALSE: return SZ_ERROR_DATA;
		case E_NOTIMPL: return SZ_ERROR_UNSUPPORTED;
	}
	return defaultRes;
}

HRESULT SResToHRESULT(SRes res) throw()
{
	switch(res) {
		case SZ_OK: return S_OK;
		case SZ_ERROR_MEM: return E_OUTOFMEMORY;
		case SZ_ERROR_PARAM: return E_INVALIDARG;
		case SZ_ERROR_PROGRESS: return E_ABORT;
		case SZ_ERROR_DATA: return S_FALSE;
		case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
		    // case SZ_ERROR_READ: return E_NOTIMPL;
	}
	return E_FAIL;
}

#define PROGRESS_UNKNOWN_VALUE (static_cast<uint64>(-1LL))

#define CONVERT_PR_VAL(x) (x == PROGRESS_UNKNOWN_VALUE ? NULL : &x)

static SRes CompressProgress(const ICompressProgress * pp, uint64 inSize, uint64 outSize) throw()
{
	CCompressProgressWrap * p = CONTAINER_FROM_VTBL(pp, CCompressProgressWrap, vt);
	p->Res = p->Progress->SetRatioInfo(CONVERT_PR_VAL(inSize), CONVERT_PR_VAL(outSize));
	return HRESULT_To_SRes(p->Res, SZ_ERROR_PROGRESS);
}

void FASTCALL CCompressProgressWrap::Init(ICompressProgressInfo * progress) throw()
{
	vt.Progress = CompressProgress;
	Progress = progress;
	Res = SZ_OK;
}

static const uint32 kStreamStepSize = (uint32)1 << 31;

static SRes MyRead(const ISeqInStream * pp, void * data, size_t * size) throw()
{
	CSeqInStreamWrap * p = CONTAINER_FROM_VTBL(pp, CSeqInStreamWrap, vt);
	uint32 curSize = ((*size < kStreamStepSize) ? (uint32)*size : kStreamStepSize);
	p->Res = (p->Stream->Read(data, curSize, &curSize));
	*size = curSize;
	p->Processed += curSize;
	if(p->Res == S_OK)
		return SZ_OK;
	return HRESULT_To_SRes(p->Res, SZ_ERROR_READ);
}

static size_t MyWrite(const ISeqOutStream * pp, const void * data, size_t size) throw()
{
	CSeqOutStreamWrap * p = CONTAINER_FROM_VTBL(pp, CSeqOutStreamWrap, vt);
	if(p->Stream) {
		p->Res = WriteStream(p->Stream, data, size);
		if(p->Res != 0)
			return 0;
	}
	else
		p->Res = S_OK;
	p->Processed += size;
	return size;
}

void FASTCALL CSeqInStreamWrap::Init(ISequentialInStream * stream) throw()
{
	vt.Read = MyRead;
	Stream = stream;
	Processed = 0;
	Res = S_OK;
}

void FASTCALL CSeqOutStreamWrap::Init(ISequentialOutStream * stream) throw()
{
	vt.Write = MyWrite;
	Stream = stream;
	Res = SZ_OK;
	Processed = 0;
}

static SRes InStreamWrap_Read(const ISeekInStream * pp, void * data, size_t * size) throw()
{
	CSeekInStreamWrap * p = CONTAINER_FROM_VTBL(pp, CSeekInStreamWrap, vt);
	uint32 curSize = ((*size < kStreamStepSize) ? (uint32)*size : kStreamStepSize);
	p->Res = p->Stream->Read(data, curSize, &curSize);
	*size = curSize;
	return (p->Res == S_OK) ? SZ_OK : SZ_ERROR_READ;
}

static SRes InStreamWrap_Seek(const ISeekInStream * pp, int64 * offset, ESzSeek origin) throw()
{
	CSeekInStreamWrap * p = CONTAINER_FROM_VTBL(pp, CSeekInStreamWrap, vt);
	uint32 moveMethod;
	switch(origin) {
		case SZ_SEEK_SET: moveMethod = STREAM_SEEK_SET; break;
		case SZ_SEEK_CUR: moveMethod = STREAM_SEEK_CUR; break;
		case SZ_SEEK_END: moveMethod = STREAM_SEEK_END; break;
		default: return SZ_ERROR_PARAM;
	}
	uint64 newPosition;
	p->Res = p->Stream->Seek(*offset, moveMethod, &newPosition);
	*offset = (int64)newPosition;
	return (p->Res == S_OK) ? SZ_OK : SZ_ERROR_READ;
}

void FASTCALL CSeekInStreamWrap::Init(IInStream * stream) throw()
{
	Stream = stream;
	vt.Read = InStreamWrap_Read;
	vt.Seek = InStreamWrap_Seek;
	Res = S_OK;
}

/* ---------- CByteInBufWrap ---------- */

void CByteInBufWrap::Free() throw()
{
	::MidFree(Buf);
	Buf = 0;
}

bool CByteInBufWrap::Alloc(uint32 size) throw()
{
	if(Buf == 0 || size != Size) {
		Free();
		Lim = Cur = Buf = static_cast<Byte *>(::MidAlloc((size_t)size));
		Size = size;
	}
	return (Buf != 0);
}

Byte CByteInBufWrap::ReadByteFromNewBlock() throw()
{
	if(Res == S_OK) {
		uint32 avail;
		Processed += (Cur - Buf);
		Res = Stream->Read(Buf, Size, &avail);
		Cur = Buf;
		Lim = Buf + avail;
		if(avail != 0)
			return *Cur++;
	}
	Extra = true;
	return 0;
}

static Byte Wrap_ReadByte(const IByteIn * pp) throw()
{
	CByteInBufWrap * p = CONTAINER_FROM_VTBL_CLS(pp, CByteInBufWrap, vt);
	if(p->Cur != p->Lim)
		return *p->Cur++;
	return p->ReadByteFromNewBlock();
}

CByteInBufWrap::CByteInBufWrap() : Buf(0)
{
	vt.Read = Wrap_ReadByte;
}

CByteInBufWrap::~CByteInBufWrap() 
{
	Free();
}

void CByteInBufWrap::Init()
{
	Lim = Cur = Buf;
	Processed = 0;
	Extra = false;
	Res = S_OK;
}

/* ---------- CByteOutBufWrap ---------- */

void CByteOutBufWrap::Free() throw()
{
	::MidFree(Buf);
	Buf = 0;
}

bool CByteOutBufWrap::Alloc(size_t size) throw()
{
	if(Buf == 0 || size != Size) {
		Free();
		Buf = static_cast<Byte *>(::MidAlloc(size));
		Size = size;
	}
	return (Buf != 0);
}

HRESULT CByteOutBufWrap::Flush() throw()
{
	if(Res == S_OK) {
		size_t size = (Cur - Buf);
		Res = WriteStream(Stream, Buf, size);
		if(Res == S_OK)
			Processed += size;
		Cur = Buf;
	}
	return Res;
}

static void Wrap_WriteByte(const IByteOut * pp, Byte b) throw()
{
	CByteOutBufWrap * p = CONTAINER_FROM_VTBL_CLS(pp, CByteOutBufWrap, vt);
	Byte * dest = p->Cur;
	*dest = b;
	p->Cur = ++dest;
	if(dest == p->Lim)
		p->Flush();
}

CByteOutBufWrap::CByteOutBufWrap() throw() : Buf(0)
{
	vt.Write = Wrap_WriteByte;
}

CByteOutBufWrap::~CByteOutBufWrap() 
{
	Free();
}

void CByteOutBufWrap::Init()
{
	Cur = Buf;
	Lim = Buf + Size;
	Processed = 0;
	Res = S_OK;
}

void FASTCALL CByteOutBufWrap::WriteByte(Byte b)
{
	* Cur++ = b;
	if(Cur == Lim)
		Flush();
}