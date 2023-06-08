// Xz.c - Xz
// 2017-04-03 : Igor Pavlov : Public domain */
// 
#include <7z-internal.h>
#pragma hdrstop
#ifdef USE_SUBBLOCK
	#include "Bcj3Enc.c"
	#include "Bcj3Dec.c"
	#include "SbFind.c"
	#include "SbEnc.c"
	#include "SbDec.c"
#endif
//#define XZ_DUMP 

const Byte XZ_SIG[XZ_SIG_SIZE] = { 0xFD, '7', 'z', 'X', 'Z', 0 };
const Byte XZ_FOOTER_SIG[XZ_FOOTER_SIG_SIZE] = { 'Y', 'Z' };

static uint FASTCALL Xz_WriteVarInt(Byte * buf, uint64 v)
{
	uint   i = 0;
	do {
		buf[i++] = (Byte)((v & 0x7F) | 0x80);
		v >>= 7;
	} while(v != 0);
	buf[(size_t)i - 1] &= 0x7F;
	return i;
}

static uint FASTCALL Xz_ReadVarInt(const Byte * p, size_t maxSize, uint64 * value)
{
	*value = 0;
	const uint limit = (maxSize > 9) ? 9 : (uint)maxSize;
	for(uint i = 0; i < limit;) {
		Byte b = p[i];
		*value |= (uint64)(b & 0x7F) << (7 * i++);
		if((b & 0x80) == 0)
			return (b == 0 && i != 1) ? 0 : i;
	}
	return 0;
}

static void FASTCALL Xz_Construct(CXzStream * p)
{
	p->numBlocks = p->numBlocksAllocated = 0;
	p->blocks = 0;
	p->flags = 0;
}

static void FASTCALL Xz_Free(CXzStream * p, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, p->blocks);
	p->numBlocks = p->numBlocksAllocated = 0;
	p->blocks = 0;
}

static uint FASTCALL XzFlags_GetCheckSize(CXzStreamFlags f)
{
	uint   t = XzFlags_GetCheckType(f);
	return (t == 0) ? 0 : (4 << ((t - 1) / 3));
}

static void FASTCALL XzCheck_Init(CXzCheck * p, unsigned mode)
{
	p->mode = mode;
	switch(mode) {
		case XZ_CHECK_CRC32: p->crc = CRC_INIT_VAL; break;
		case XZ_CHECK_CRC64: p->crc64 = CRC64_INIT_VAL; break;
		case XZ_CHECK_SHA256: Sha256_Init(&p->sha); break;
	}
}

static void FASTCALL XzCheck_Update(CXzCheck * p, const void * data, size_t size)
{
	switch(p->mode) {
		case XZ_CHECK_CRC32: p->crc = CrcUpdate(p->crc, data, size); break;
		case XZ_CHECK_CRC64: p->crc64 = Crc64Update(p->crc64, data, size); break;
		case XZ_CHECK_SHA256: Sha256_Update(&p->sha, (const Byte *)data, size); break;
	}
}

static int FASTCALL XzCheck_Final(CXzCheck * p, Byte * digest)
{
	switch(p->mode) {
		case XZ_CHECK_CRC32:
		    SetUi32(digest, CRC_GET_DIGEST(p->crc));
		    break;
		case XZ_CHECK_CRC64:
	    {
		    int i;
		    uint64 v = CRC64_GET_DIGEST(p->crc64);
		    for(i = 0; i < 8; i++, v >>= 8)
			    digest[i] = (Byte)(v & 0xFF);
		    break;
	    }
		case XZ_CHECK_SHA256:
		    Sha256_Final(&p->sha, digest);
		    break;
		default:
		    return 0;
	}
	return 1;
}
//
// XzEnc.c
//
// Xz Encode
//
#define XzBlock_ClearFlags(p)       (p)->flags = 0;
#define XzBlock_SetNumFilters(p, n) (p)->flags |= ((n) - 1);
#define XzBlock_SetHasPackSize(p)   (p)->flags |= XZ_BF_PACK_SIZE;
#define XzBlock_SetHasUnpackSize(p) (p)->flags |= XZ_BF_UNPACK_SIZE;

static SRes FASTCALL WriteBytes(ISeqOutStream * s, const void * buf, uint32 size)
{
	return (ISeqOutStream_Write(s, buf, size) == size) ? SZ_OK : SZ_ERROR_WRITE;
}

static SRes FASTCALL WriteBytesAndCrc(ISeqOutStream * s, const void * buf, uint32 size, uint32 * crc)
{
	*crc = CrcUpdate(*crc, buf, size);
	return WriteBytes(s, buf, size);
}

static SRes Xz_WriteHeader(CXzStreamFlags f, ISeqOutStream * s)
{
	uint32 crc;
	Byte header[XZ_STREAM_HEADER_SIZE];
	memcpy(header, XZ_SIG, XZ_SIG_SIZE);
	header[XZ_SIG_SIZE] = (Byte)(f >> 8);
	header[XZ_SIG_SIZE + 1] = (Byte)(f & 0xFF);
	crc = CrcCalc(header + XZ_SIG_SIZE, XZ_STREAM_FLAGS_SIZE);
	SetUi32(header + XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE, crc);
	return WriteBytes(s, header, XZ_STREAM_HEADER_SIZE);
}

static SRes XzBlock_WriteHeader(const CXzBlock * p, ISeqOutStream * s)
{
	Byte   header[XZ_BLOCK_HEADER_SIZE_MAX];
	uint   pos = 1;
	uint   numFilters, i;
	header[pos++] = p->flags;

	if(XzBlock_HasPackSize(p)) 
		pos += Xz_WriteVarInt(header + pos, p->packSize);
	if(XzBlock_HasUnpackSize(p)) 
		pos += Xz_WriteVarInt(header + pos, p->unpackSize);
	numFilters = XzBlock_GetNumFilters(p);
	for(i = 0; i < numFilters; i++) {
		const CXzFilter * f = &p->filters[i];
		pos += Xz_WriteVarInt(header + pos, f->id);
		pos += Xz_WriteVarInt(header + pos, f->propsSize);
		memcpy(header + pos, f->props, f->propsSize);
		pos += f->propsSize;
	}
	while((pos & 3) != 0)
		header[pos++] = 0;
	header[0] = (Byte)(pos >> 2);
	SetUi32(header + pos, CrcCalc(header, pos));
	return WriteBytes(s, header, pos + 4);
}

static SRes Xz_WriteFooter(CXzStream * p, ISeqOutStream * s)
{
	Byte buf[32];
	uint64 globalPos;
	{
		uint32 crc = CRC_INIT_VAL;
		uint   pos = 1 + Xz_WriteVarInt(buf + 1, p->numBlocks);
		size_t i;
		globalPos = pos;
		buf[0] = 0;
		RINOK(WriteBytesAndCrc(s, buf, pos, &crc));
		for(i = 0; i < p->numBlocks; i++) {
			const CXzBlockSizes * block = &p->blocks[i];
			pos = Xz_WriteVarInt(buf, block->totalSize);
			pos += Xz_WriteVarInt(buf + pos, block->unpackSize);
			globalPos += pos;
			RINOK(WriteBytesAndCrc(s, buf, pos, &crc));
		}
		pos = ((uint)globalPos & 3);
		if(pos != 0) {
			buf[0] = buf[1] = buf[2] = 0;
			RINOK(WriteBytesAndCrc(s, buf, 4 - pos, &crc));
			globalPos += 4 - pos;
		}
		{
			SetUi32(buf, CRC_GET_DIGEST(crc));
			RINOK(WriteBytes(s, buf, 4));
			globalPos += 4;
		}
	}
	{
		uint32 indexSize = (uint32)((globalPos >> 2) - 1);
		SetUi32(buf + 4, indexSize);
		buf[8] = (Byte)(p->flags >> 8);
		buf[9] = (Byte)(p->flags & 0xFF);
		SetUi32(buf, CrcCalc(buf + 4, 6));
		memcpy(buf + 10, XZ_FOOTER_SIG, XZ_FOOTER_SIG_SIZE);
		return WriteBytes(s, buf, 12);
	}
}

static SRes Xz_AddIndexRecord(CXzStream * p, uint64 unpackSize, uint64 totalSize, ISzAllocPtr alloc)
{
	if(!p->blocks || p->numBlocksAllocated == p->numBlocks) {
		size_t num = p->numBlocks * 2 + 1;
		size_t newSize = sizeof(CXzBlockSizes) * num;
		CXzBlockSizes * blocks;
		if(newSize / sizeof(CXzBlockSizes) != num)
			return SZ_ERROR_MEM;
		blocks = (CXzBlockSizes*)ISzAlloc_Alloc(alloc, newSize);
		if(!blocks)
			return SZ_ERROR_MEM;
		if(p->numBlocks != 0) {
			memcpy(blocks, p->blocks, p->numBlocks * sizeof(CXzBlockSizes));
			ISzAlloc_Free(alloc, p->blocks);
		}
		p->blocks = blocks;
		p->numBlocksAllocated = num;
	}
	{
		CXzBlockSizes * block = &p->blocks[p->numBlocks++];
		block->unpackSize = unpackSize;
		block->totalSize = totalSize;
	}
	return SZ_OK;
}
// 
// CSeqCheckInStream
// 
typedef struct {
	ISeqInStream vt;
	ISeqInStream * realStream;
	uint64 processed;
	CXzCheck check;
} CSeqCheckInStream;

static void SeqCheckInStream_Init(CSeqCheckInStream * p, unsigned mode)
{
	p->processed = 0;
	XzCheck_Init(&p->check, mode);
}

static void SeqCheckInStream_GetDigest(CSeqCheckInStream * p, Byte * digest)
{
	XzCheck_Final(&p->check, digest);
}

static SRes SeqCheckInStream_Read(const ISeqInStream * pp, void * data, size_t * size)
{
	CSeqCheckInStream * p = CONTAINER_FROM_VTBL(pp, CSeqCheckInStream, vt);
	SRes res = ISeqInStream_Read(p->realStream, data, size);
	XzCheck_Update(&p->check, data, *size);
	p->processed += *size;
	return res;
}
// 
// CSeqSizeOutStream
// 
typedef struct {
	ISeqOutStream vt;
	ISeqOutStream * realStream;
	uint64 processed;
} CSeqSizeOutStream;

static size_t MyWrite(const ISeqOutStream * pp, const void * data, size_t size)
{
	CSeqSizeOutStream * p = CONTAINER_FROM_VTBL(pp, CSeqSizeOutStream, vt);
	size = ISeqOutStream_Write(p->realStream, data, size);
	p->processed += size;
	return size;
}
// 
// CSeqInFilter
// 
#define FILTER_BUF_SIZE (1 << 20)

typedef struct {
	ISeqInStream p;
	ISeqInStream * realStream;
	IStateCoder StateCoder;
	Byte * buf;
	size_t curPos;
	size_t endPos;
	int srcWasFinished;
} CSeqInFilter;

static SRes SeqInFilter_Read(const ISeqInStream * pp, void * data, size_t * size)
{
	CSeqInFilter * p = CONTAINER_FROM_VTBL(pp, CSeqInFilter, p);
	size_t sizeOriginal = *size;
	if(sizeOriginal == 0)
		return SZ_OK;
	*size = 0;
	for(;;) {
		if(!p->srcWasFinished && p->curPos == p->endPos) {
			p->curPos = 0;
			p->endPos = FILTER_BUF_SIZE;
			RINOK(ISeqInStream_Read(p->realStream, p->buf, &p->endPos));
			if(p->endPos == 0)
				p->srcWasFinished = 1;
		}
		{
			SizeT srcLen = p->endPos - p->curPos;
			int wasFinished;
			SRes res;
			*size = sizeOriginal;
			res = p->StateCoder.Code(p->StateCoder.p, (Byte *)data, size, p->buf + p->curPos, &srcLen, p->srcWasFinished, CODER_FINISH_ANY, &wasFinished);
			p->curPos += srcLen;
			if(*size != 0 || srcLen == 0 || res != 0)
				return res;
		}
	}
}

static void SeqInFilter_Construct(CSeqInFilter * p)
{
	p->buf = NULL;
	p->p.Read = SeqInFilter_Read;
}

static void SeqInFilter_Free(CSeqInFilter * p)
{
	if(p->buf) {
		ISzAlloc_Free(&g_Alloc, p->buf);
		p->buf = NULL;
	}
}

SRes BraState_SetFromMethod(IStateCoder * p, uint64 id, int encodeMode, ISzAllocPtr alloc);

static SRes SeqInFilter_Init(CSeqInFilter * p, const CXzFilter * props)
{
	if(!p->buf) {
		p->buf = (Byte *)ISzAlloc_Alloc(&g_Alloc, FILTER_BUF_SIZE);
		if(!p->buf)
			return SZ_ERROR_MEM;
	}
	p->curPos = p->endPos = 0;
	p->srcWasFinished = 0;
	RINOK(BraState_SetFromMethod(&p->StateCoder, props->id, 1, &g_Alloc));
	RINOK(p->StateCoder.SetProps(p->StateCoder.p, props->props, props->propsSize, &g_Alloc));
	p->StateCoder.Init(p->StateCoder.p);
	return SZ_OK;
}
// 
// CSbEncInStream
// 
#ifdef USE_SUBBLOCK

typedef struct {
	ISeqInStream vt;
	ISeqInStream * inStream;
	CSbEnc enc;
} CSbEncInStream;

static SRes SbEncInStream_Read(const ISeqInStream * pp, void * data, size_t * size)
{
	CSbEncInStream * p = CONTAINER_FROM_VTBL(pp, CSbEncInStream, vt);
	size_t sizeOriginal = *size;
	if(sizeOriginal == 0)
		return SZ_OK;
	for(;;) {
		if(p->enc.needRead && !p->enc.readWasFinished) {
			size_t processed = p->enc.needReadSizeMax;
			RINOK(p->inStream->Read(p->inStream, p->enc.buf + p->enc.readPos, &processed));
			p->enc.readPos += processed;
			if(processed == 0) {
				p->enc.readWasFinished = True;
				p->enc.isFinalFinished = True;
			}
			p->enc.needRead = False;
		}

		*size = sizeOriginal;
		RINOK(SbEnc_Read(&p->enc, data, size));
		if(*size != 0 || !p->enc.needRead)
			return SZ_OK;
	}
}

void SbEncInStream_Construct(CSbEncInStream * p, ISzAllocPtr alloc)
{
	SbEnc_Construct(&p->enc, alloc);
	p->vt.Read = SbEncInStream_Read;
}

SRes SbEncInStream_Init(CSbEncInStream * p) { return SbEnc_Init(&p->enc); }
void SbEncInStream_Free(CSbEncInStream * p) { SbEnc_Free(&p->enc); }

#endif

typedef struct {
	CLzma2EncHandle lzma2;
  #ifdef USE_SUBBLOCK
	CSbEncInStream sb;
  #endif
	CSeqInFilter filter;
	ISzAllocPtr alloc;
	ISzAllocPtr bigAlloc;
} CLzma2WithFilters;

static void Lzma2WithFilters_Construct(CLzma2WithFilters * p, ISzAllocPtr alloc, ISzAllocPtr bigAlloc)
{
	p->alloc = alloc;
	p->bigAlloc = bigAlloc;
	p->lzma2 = NULL;
  #ifdef USE_SUBBLOCK
	SbEncInStream_Construct(&p->sb, alloc);
  #endif
	SeqInFilter_Construct(&p->filter);
}

static SRes Lzma2WithFilters_Create(CLzma2WithFilters * p)
{
	p->lzma2 = Lzma2Enc_Create(p->alloc, p->bigAlloc);
	return p->lzma2 ? SZ_OK : SZ_ERROR_MEM;
}

static void Lzma2WithFilters_Free(CLzma2WithFilters * p)
{
	SeqInFilter_Free(&p->filter);
  #ifdef USE_SUBBLOCK
	SbEncInStream_Free(&p->sb);
  #endif
	if(p->lzma2) {
		Lzma2Enc_Destroy(p->lzma2);
		p->lzma2 = NULL;
	}
}

void FASTCALL XzProps_Init(CXzProps * p)
{
	p->lzma2Props = NULL;
	p->filterProps = NULL;
	p->checkId = XZ_CHECK_CRC32;
}

void FASTCALL XzFilterProps_Init(CXzFilterProps * p)
{
	p->id = 0;
	p->delta = 0;
	p->ip = 0;
	p->ipDefined = False;
}

static SRes Xz_Compress(CXzStream * xz, CLzma2WithFilters * lzmaf, ISeqOutStream * outStream, ISeqInStream * inStream,
    const CXzProps * props, ICompressProgress * progress)
{
	xz->flags = (Byte)props->checkId;
	RINOK(Lzma2Enc_SetProps(lzmaf->lzma2, props->lzma2Props));
	RINOK(Xz_WriteHeader(xz->flags, outStream));
	{
		CSeqCheckInStream checkInStream;
		CSeqSizeOutStream seqSizeOutStream;
		CXzBlock block;
		unsigned filterIndex = 0;
		CXzFilter * filter = NULL;
		const CXzFilterProps * fp = props->filterProps;
		XzBlock_ClearFlags(&block);
		XzBlock_SetNumFilters(&block, 1 + (fp ? 1 : 0));
		if(fp) {
			filter = &block.filters[filterIndex++];
			filter->id = fp->id;
			filter->propsSize = 0;
			if(fp->id == XZ_ID_Delta) {
				filter->props[0] = (Byte)(fp->delta - 1);
				filter->propsSize = 1;
			}
			else if(fp->ipDefined) {
				SetUi32(filter->props, fp->ip);
				filter->propsSize = 4;
			}
		}
		{
			CXzFilter * f = &block.filters[filterIndex++];
			f->id = XZ_ID_LZMA2;
			f->propsSize = 1;
			f->props[0] = Lzma2Enc_WriteProperties(lzmaf->lzma2);
		}
		seqSizeOutStream.vt.Write = MyWrite;
		seqSizeOutStream.realStream = outStream;
		seqSizeOutStream.processed = 0;
		RINOK(XzBlock_WriteHeader(&block, &seqSizeOutStream.vt));
		checkInStream.vt.Read = SeqCheckInStream_Read;
		checkInStream.realStream = inStream;
		SeqCheckInStream_Init(&checkInStream, XzFlags_GetCheckType(xz->flags));
		if(fp) {
      #ifdef USE_SUBBLOCK
			if(fp->id == XZ_ID_Subblock) {
				lzmaf->sb.inStream = &checkInStream.vt;
				RINOK(SbEncInStream_Init(&lzmaf->sb));
			}
			else
      #endif
			{
				lzmaf->filter.realStream = &checkInStream.vt;
				RINOK(SeqInFilter_Init(&lzmaf->filter, filter));
			}
		}
		{
			uint64 packPos = seqSizeOutStream.processed;
			SRes res = Lzma2Enc_Encode(lzmaf->lzma2, &seqSizeOutStream.vt, fp ?
	    #ifdef USE_SUBBLOCK
			    (fp->id == XZ_ID_Subblock) ? &lzmaf->sb.vt :
	    #endif
			    &lzmaf->filter.p : &checkInStream.vt, progress);
			RINOK(res);
			block.unpackSize = checkInStream.processed;
			block.packSize = seqSizeOutStream.processed - packPos;
		}
		{
			uint   padSize = 0;
			Byte   buf[128];
			while((((uint)block.packSize + padSize) & 3) != 0)
				buf[padSize++] = 0;
			SeqCheckInStream_GetDigest(&checkInStream, buf + padSize);
			RINOK(WriteBytes(&seqSizeOutStream.vt, buf, padSize + XzFlags_GetCheckSize(xz->flags)));
			RINOK(Xz_AddIndexRecord(xz, block.unpackSize, seqSizeOutStream.processed - padSize, &g_Alloc));
		}
	}
	return Xz_WriteFooter(xz, outStream);
}

SRes Xz_Encode(ISeqOutStream * outStream, ISeqInStream * inStream, const CXzProps * props, ICompressProgress * progress)
{
	SRes res;
	CXzStream xz;
	CLzma2WithFilters lzmaf;
	Xz_Construct(&xz);
	Lzma2WithFilters_Construct(&lzmaf, &g_Alloc, &g_BigAlloc);
	res = Lzma2WithFilters_Create(&lzmaf);
	if(res == SZ_OK)
		res = Xz_Compress(&xz, &lzmaf, outStream, inStream, props, progress);
	Lzma2WithFilters_Free(&lzmaf);
	Xz_Free(&xz, &g_Alloc);
	return res;
}

SRes Xz_EncodeEmpty(ISeqOutStream * outStream)
{
	SRes res;
	CXzStream xz;
	Xz_Construct(&xz);
	res = Xz_WriteHeader(xz.flags, outStream);
	if(res == SZ_OK)
		res = Xz_WriteFooter(&xz, outStream);
	Xz_Free(&xz, &g_Alloc);
	return res;
}
//
// XzDec.c
//
// Xz Decode
//
#define XZ_CHECK_SIZE_MAX 64
#define CODER_BUF_SIZE ((size_t)1 << 17)
// 
// BraState
// 
#define BRA_BUF_SIZE (1 << 14)

typedef struct {
	size_t bufPos;
	size_t bufConv;
	size_t bufTotal;
	uint32 methodId;
	int encodeMode;
	uint32 delta;
	uint32 ip;
	uint32 x86State;
	Byte deltaState[DELTA_STATE_SIZE];
	Byte buf[BRA_BUF_SIZE];
} CBraState;

static void BraState_Free(void * pp, ISzAllocPtr alloc)
{
	ISzAlloc_Free(alloc, pp);
}

static SRes BraState_SetProps(void * pp, const Byte * props, size_t propSize, ISzAllocPtr alloc)
{
	CBraState * p = ((CBraState*)pp);
	UNUSED_VAR(alloc);
	p->ip = 0;
	if(p->methodId == XZ_ID_Delta) {
		if(propSize != 1)
			return SZ_ERROR_UNSUPPORTED;
		p->delta = (uint)props[0] + 1;
	}
	else {
		if(propSize == 4) {
			uint32 v = GetUi32(props);
			switch(p->methodId) {
				case XZ_ID_PPC:
				case XZ_ID_ARM:
				case XZ_ID_SPARC:
				    if((v & 3) != 0)
					    return SZ_ERROR_UNSUPPORTED;
				    break;
				case XZ_ID_ARMT:
				    if((v & 1) != 0)
					    return SZ_ERROR_UNSUPPORTED;
				    break;
				case XZ_ID_IA64:
				    if((v & 0xF) != 0)
					    return SZ_ERROR_UNSUPPORTED;
				    break;
			}
			p->ip = v;
		}
		else if(propSize != 0)
			return SZ_ERROR_UNSUPPORTED;
	}
	return SZ_OK;
}

static void BraState_Init(void * pp)
{
	CBraState * p = ((CBraState*)pp);
	p->bufPos = p->bufConv = p->bufTotal = 0;
	x86_Convert_Init(p->x86State);
	if(p->methodId == XZ_ID_Delta)
		Delta_Init(p->deltaState);
}

#define CASE_BRA_CONV(isa) case XZ_ID_ ## isa: p->bufConv = isa ## _Convert(p->buf, p->bufTotal, p->ip, p->encodeMode); break;

static SRes BraState_Code(void * pp, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
    int srcWasFinished, ECoderFinishMode finishMode, int * wasFinished)
{
	CBraState * p = ((CBraState*)pp);
	SizeT destLenOrig = *destLen;
	SizeT srcLenOrig = *srcLen;
	UNUSED_VAR(finishMode);
	*destLen = 0;
	*srcLen = 0;
	*wasFinished = 0;
	while(destLenOrig > 0) {
		if(p->bufPos != p->bufConv) {
			size_t curSize = p->bufConv - p->bufPos;
			SETMIN(curSize, destLenOrig);
			memcpy(dest, p->buf + p->bufPos, curSize);
			p->bufPos += curSize;
			*destLen += curSize;
			dest += curSize;
			destLenOrig -= curSize;
			continue;
		}
		p->bufTotal -= p->bufPos;
		memmove(p->buf, p->buf + p->bufPos, p->bufTotal);
		p->bufPos = 0;
		p->bufConv = 0;
		{
			size_t curSize = BRA_BUF_SIZE - p->bufTotal;
			SETMIN(curSize, srcLenOrig);
			memcpy(p->buf + p->bufTotal, src, curSize);
			*srcLen += curSize;
			src += curSize;
			srcLenOrig -= curSize;
			p->bufTotal += curSize;
		}
		if(p->bufTotal == 0)
			break;
		switch(p->methodId) {
			case XZ_ID_Delta:
			    if(p->encodeMode)
				    Delta_Encode(p->deltaState, p->delta, p->buf, p->bufTotal);
			    else
				    Delta_Decode(p->deltaState, p->delta, p->buf, p->bufTotal);
			    p->bufConv = p->bufTotal;
			    break;
			case XZ_ID_X86:
			    p->bufConv = x86_Convert(p->buf, p->bufTotal, p->ip, &p->x86State, p->encodeMode);
			    break;
			    CASE_BRA_CONV(PPC)
			    CASE_BRA_CONV(IA64)
			    CASE_BRA_CONV(ARM)
			    CASE_BRA_CONV(ARMT)
			    CASE_BRA_CONV(SPARC)
			default:
			    return SZ_ERROR_UNSUPPORTED;
		}
		p->ip += (uint32)p->bufConv;
		if(p->bufConv == 0) {
			if(!srcWasFinished)
				break;
			p->bufConv = p->bufTotal;
		}
	}
	if(p->bufTotal == p->bufPos && srcLenOrig == 0 && srcWasFinished)
		*wasFinished = 1;
	return SZ_OK;
}

SRes BraState_SetFromMethod(IStateCoder * p, uint64 id, int encodeMode, ISzAllocPtr alloc)
{
	if(!oneof7(id, XZ_ID_Delta, XZ_ID_X86, XZ_ID_PPC, XZ_ID_IA64, XZ_ID_ARM, XZ_ID_ARMT, XZ_ID_SPARC))
		return SZ_ERROR_UNSUPPORTED;
	else {
		p->p = 0;
		CBraState * decoder = (CBraState*)ISzAlloc_Alloc(alloc, sizeof(CBraState));
		if(!decoder)
			return SZ_ERROR_MEM;
		else {
			decoder->methodId = (uint32)id;
			decoder->encodeMode = encodeMode;
			p->p = decoder;
			p->Free = BraState_Free;
			p->SetProps = BraState_SetProps;
			p->Init = BraState_Init;
			p->Code = BraState_Code;
			return SZ_OK;
		}
	}
}
// 
// SbState
// 
#ifdef USE_SUBBLOCK
static void SbState_Free(void * pp, ISzAllocPtr alloc)
{
	CSbDec * p = (CSbDec*)pp;
	SbDec_Free(p);
	ISzAlloc_Free(alloc, pp);
}

static SRes SbState_SetProps(void * pp, const Byte * props, size_t propSize, ISzAllocPtr alloc)
{
	UNUSED_VAR(pp);
	UNUSED_VAR(props);
	UNUSED_VAR(alloc);
	return (propSize == 0) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static void SbState_Init(void * pp)
{
	SbDec_Init((CSbDec*)pp);
}

static SRes SbState_Code(void * pp, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
    int srcWasFinished, ECoderFinishMode finishMode, int * wasFinished)
{
	CSbDec * p = (CSbDec*)pp;
	SRes res;
	UNUSED_VAR(srcWasFinished);
	p->dest = dest;
	p->destLen = *destLen;
	p->src = src;
	p->srcLen = *srcLen;
	p->finish = finishMode; /* change it */
	res = SbDec_Decode((CSbDec*)pp);
	*destLen -= p->destLen;
	*srcLen -= p->srcLen;
	*wasFinished = (*destLen == 0 && *srcLen == 0); /* change it */
	return res;
}

SRes SbState_SetFromMethod(IStateCoder * p, ISzAllocPtr alloc)
{
	CSbDec * decoder;
	p->p = 0;
	decoder = ISzAlloc_Alloc(alloc, sizeof(CSbDec));
	if(!decoder)
		return SZ_ERROR_MEM;
	p->p = decoder;
	p->Free = SbState_Free;
	p->SetProps = SbState_SetProps;
	p->Init = SbState_Init;
	p->Code = SbState_Code;
	SbDec_Construct(decoder);
	SbDec_SetAlloc(decoder, alloc);
	return SZ_OK;
}
#endif
// 
// Lzma2State
// 
static void Lzma2State_Free(void * pp, ISzAllocPtr alloc)
{
	Lzma2Dec_Free((CLzma2Dec*)pp, alloc);
	ISzAlloc_Free(alloc, pp);
}

static SRes Lzma2State_SetProps(void * pp, const Byte * props, size_t propSize, ISzAllocPtr alloc)
{
	return (propSize != 1) ? SZ_ERROR_UNSUPPORTED : Lzma2Dec_Allocate((CLzma2Dec*)pp, props[0], alloc);
}

static void Lzma2State_Init(void * pp)
{
	Lzma2Dec_Init((CLzma2Dec*)pp);
}

static SRes Lzma2State_Code(void * pp, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen,
    int srcWasFinished, ECoderFinishMode finishMode, int * wasFinished)
{
	ELzmaStatus status;
	/* ELzmaFinishMode fm = (finishMode == LZMA_FINISH_ANY) ? LZMA_FINISH_ANY : LZMA_FINISH_END; */
	SRes res = Lzma2Dec_DecodeToBuf((CLzma2Dec*)pp, dest, destLen, src, srcLen, (ELzmaFinishMode)finishMode, &status);
	UNUSED_VAR(srcWasFinished);
	*wasFinished = (status == LZMA_STATUS_FINISHED_WITH_MARK);
	return res;
}

static SRes Lzma2State_SetFromMethod(IStateCoder * p, ISzAllocPtr alloc)
{
	CLzma2Dec * decoder = (CLzma2Dec*)ISzAlloc_Alloc(alloc, sizeof(CLzma2Dec));
	p->p = decoder;
	if(!decoder)
		return SZ_ERROR_MEM;
	p->Free = Lzma2State_Free;
	p->SetProps = Lzma2State_SetProps;
	p->Init = Lzma2State_Init;
	p->Code = Lzma2State_Code;
	Lzma2Dec_Construct(decoder);
	return SZ_OK;
}

static void FASTCALL MixCoder_Construct(CMixCoder * p, ISzAllocPtr alloc)
{
	uint i;
	p->alloc = alloc;
	p->buf = NULL;
	p->numCoders = 0;
	for(i = 0; i < MIXCODER_NUM_FILTERS_MAX; i++)
		p->coders[i].p = NULL;
}

static void FASTCALL MixCoder_Free(CMixCoder * p)
{
	uint i;
	for(i = 0; i < p->numCoders; i++) {
		IStateCoder * sc = &p->coders[i];
		if(p->alloc && sc->p)
			sc->Free(sc->p, p->alloc);
	}
	p->numCoders = 0;
	if(p->buf) {
		ISzAlloc_Free(p->alloc, p->buf);
		p->buf = NULL; /* 9.31: the BUG was fixed */
	}
}

static void FASTCALL MixCoder_Init(CMixCoder * p)
{
	uint i;
	for(i = 0; i < MIXCODER_NUM_FILTERS_MAX - 1; i++) {
		p->size[i] = 0;
		p->pos[i] = 0;
		p->finished[i] = 0;
	}
	for(i = 0; i < p->numCoders; i++) {
		IStateCoder * coder = &p->coders[i];
		coder->Init(coder->p);
	}
}

static SRes FASTCALL MixCoder_SetFromMethod(CMixCoder * p, unsigned coderIndex, uint64 methodId)
{
	IStateCoder * sc = &p->coders[coderIndex];
	p->ids[coderIndex] = methodId;
	switch(methodId) {
		case XZ_ID_LZMA2: return Lzma2State_SetFromMethod(sc, p->alloc);
    #ifdef USE_SUBBLOCK
		case XZ_ID_Subblock: return SbState_SetFromMethod(sc, p->alloc);
    #endif
	}
	if(coderIndex == 0)
		return SZ_ERROR_UNSUPPORTED;
	return BraState_SetFromMethod(sc, methodId, 0, p->alloc);
}

static SRes FASTCALL MixCoder_Code(CMixCoder * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, int srcWasFinished, ECoderFinishMode finishMode, ECoderStatus * status)
{
	SizeT destLenOrig = *destLen;
	SizeT srcLenOrig = *srcLen;
	Bool allFinished = True;
	*destLen = 0;
	*srcLen = 0;
	*status = CODER_STATUS_NOT_FINISHED;
	if(!p->buf) {
		p->buf = (Byte *)ISzAlloc_Alloc(p->alloc, CODER_BUF_SIZE * (MIXCODER_NUM_FILTERS_MAX - 1));
		if(!p->buf)
			return SZ_ERROR_MEM;
	}
	if(p->numCoders != 1)
		finishMode = CODER_FINISH_ANY;
	for(;;) {
		Bool processed = False;
		uint i;
		/*
		   if(p->numCoders == 1 && *destLen == destLenOrig && finishMode == LZMA_FINISH_ANY)
		   break;
		 */
		for(i = 0; i < p->numCoders; i++) {
			SRes res;
			IStateCoder * coder = &p->coders[i];
			Byte * destCur;
			SizeT destLenCur, srcLenCur;
			const Byte * srcCur;
			int srcFinishedCur;
			int encodingWasFinished;
			if(i == 0) {
				srcCur = src;
				srcLenCur = srcLenOrig - *srcLen;
				srcFinishedCur = srcWasFinished;
			}
			else {
				size_t k = i - 1;
				srcCur = p->buf + (CODER_BUF_SIZE * k) + p->pos[k];
				srcLenCur = p->size[k] - p->pos[k];
				srcFinishedCur = p->finished[k];
			}
			if(i == p->numCoders - 1) {
				destCur = dest;
				destLenCur = destLenOrig - *destLen;
			}
			else {
				if(p->pos[i] != p->size[i])
					continue;
				destCur = p->buf + (CODER_BUF_SIZE * i);
				destLenCur = CODER_BUF_SIZE;
			}
			res = coder->Code(coder->p, destCur, &destLenCur, srcCur, &srcLenCur, srcFinishedCur, finishMode, &encodingWasFinished);
			if(!encodingWasFinished)
				allFinished = False;
			if(i == 0) {
				*srcLen += srcLenCur;
				src += srcLenCur;
			}
			else {
				p->pos[(size_t)i - 1] += srcLenCur;
			}
			if(i == p->numCoders - 1) {
				*destLen += destLenCur;
				dest += destLenCur;
			}
			else {
				p->size[i] = destLenCur;
				p->pos[i] = 0;
				p->finished[i] = encodingWasFinished;
			}
			if(res != SZ_OK)
				return res;
			if(destLenCur != 0 || srcLenCur != 0)
				processed = True;
		}
		if(!processed)
			break;
	}
	if(allFinished)
		*status = CODER_STATUS_FINISHED_WITH_MARK;
	return SZ_OK;
}

SRes Xz_ParseHeader(CXzStreamFlags * p, const Byte * buf)
{
	*p = (CXzStreamFlags)GetBe16(buf + XZ_SIG_SIZE);
	if(CrcCalc(buf + XZ_SIG_SIZE, XZ_STREAM_FLAGS_SIZE) != GetUi32(buf + XZ_SIG_SIZE + XZ_STREAM_FLAGS_SIZE))
		return SZ_ERROR_NO_ARCHIVE;
	return XzFlags_IsSupported(*p) ? SZ_OK : SZ_ERROR_UNSUPPORTED;
}

static Bool Xz_CheckFooter(CXzStreamFlags flags, uint64 indexSize, const Byte * buf)
{
	return indexSize == (((uint64)GetUi32(buf + 4) + 1) << 2) && (GetUi32(buf) == CrcCalc(buf + 4, 6) && flags == GetBe16(buf + 8) && memcmp(buf + 10, XZ_FOOTER_SIG, XZ_FOOTER_SIG_SIZE) == 0);
}

#define READ_VARINT_AND_CHECK(buf, pos, size, res) { uint s = Xz_ReadVarInt(buf + pos, size - pos, res); if(s == 0) return SZ_ERROR_ARCHIVE; pos += s; }

SRes XzBlock_Parse(CXzBlock * p, const Byte * header)
{
	unsigned pos;
	unsigned numFilters, i;
	unsigned headerSize = (uint)header[0] << 2;
	if(CrcCalc(header, headerSize) != GetUi32(header + headerSize))
		return SZ_ERROR_ARCHIVE;
	pos = 1;
	if(pos == headerSize)
		return SZ_ERROR_ARCHIVE;
	p->flags = header[pos++];
	if(XzBlock_HasPackSize(p)) {
		READ_VARINT_AND_CHECK(header, pos, headerSize, &p->packSize);
		if(p->packSize == 0 || p->packSize + headerSize >= (uint64)1 << 63)
			return SZ_ERROR_ARCHIVE;
	}
	if(XzBlock_HasUnpackSize(p))
		READ_VARINT_AND_CHECK(header, pos, headerSize, &p->unpackSize);
	numFilters = XzBlock_GetNumFilters(p);
	for(i = 0; i < numFilters; i++) {
		CXzFilter * filter = p->filters + i;
		uint64 size;
		READ_VARINT_AND_CHECK(header, pos, headerSize, &filter->id);
		READ_VARINT_AND_CHECK(header, pos, headerSize, &size);
		if(size > headerSize - pos || size > XZ_FILTER_PROPS_SIZE_MAX)
			return SZ_ERROR_ARCHIVE;
		filter->propsSize = (uint32)size;
		memcpy(filter->props, header + pos, (size_t)size);
		pos += (uint)size;
    #ifdef XZ_DUMP
		printf("\nf[%u] = %2X: ", i, (uint)filter->id);
		{
			for(uint i = 0; i < size; i++)
				printf(" %2X", filter->props[i]);
		}
    #endif
	}
	while(pos < headerSize)
		if(header[pos++] != 0)
			return SZ_ERROR_ARCHIVE;
	return SZ_OK;
}

SRes XzDec_Init(CMixCoder * p, const CXzBlock * block)
{
	uint i;
	Bool needReInit = True;
	unsigned numFilters = XzBlock_GetNumFilters(block);
	if(numFilters == p->numCoders) {
		for(i = 0; i < numFilters; i++)
			if(p->ids[i] != block->filters[numFilters - 1 - i].id)
				break;
		needReInit = (i != numFilters);
	}
	if(needReInit) {
		MixCoder_Free(p);
		p->numCoders = numFilters;
		for(i = 0; i < numFilters; i++) {
			const CXzFilter * f = &block->filters[numFilters - 1 - i];
			RINOK(MixCoder_SetFromMethod(p, i, f->id));
		}
	}
	for(i = 0; i < numFilters; i++) {
		const CXzFilter * f = &block->filters[numFilters - 1 - i];
		IStateCoder * sc = &p->coders[i];
		RINOK(sc->SetProps(sc->p, f->props, f->propsSize, p->alloc));
	}
	MixCoder_Init(p);
	return SZ_OK;
}

void XzUnpacker_Init(CXzUnpacker * p)
{
	p->state = XZ_STATE_STREAM_HEADER;
	p->pos = 0;
	p->numStartedStreams = 0;
	p->numFinishedStreams = 0;
	p->numTotalBlocks = 0;
	p->padSize = 0;
}

void XzUnpacker_Construct(CXzUnpacker * p, ISzAllocPtr alloc)
{
	MixCoder_Construct(&p->decoder, alloc);
	XzUnpacker_Init(p);
}

void XzUnpacker_Free(CXzUnpacker * p)
{
	MixCoder_Free(&p->decoder);
}

SRes XzUnpacker_Code(CXzUnpacker * p, Byte * dest, SizeT * destLen, const Byte * src, SizeT * srcLen, ECoderFinishMode finishMode, ECoderStatus * status)
{
	SizeT destLenOrig = *destLen;
	SizeT srcLenOrig = *srcLen;
	*destLen = 0;
	*srcLen = 0;
	*status = CODER_STATUS_NOT_SPECIFIED;
	for(;;) {
		SizeT srcRem = srcLenOrig - *srcLen;
		if(p->state == XZ_STATE_BLOCK) {
			SizeT destLen2 = destLenOrig - *destLen;
			SizeT srcLen2 = srcLenOrig - *srcLen;
			SRes res;
			if(srcLen2 == 0 && destLen2 == 0) {
				*status = CODER_STATUS_NOT_FINISHED;
				return SZ_OK;
			}
			res = MixCoder_Code(&p->decoder, dest, &destLen2, src, &srcLen2, False, finishMode, status);
			XzCheck_Update(&p->check, dest, destLen2);
			(*srcLen) += srcLen2;
			src += srcLen2;
			p->packSize += srcLen2;
			(*destLen) += destLen2;
			dest += destLen2;
			p->unpackSize += destLen2;
			RINOK(res);
			if(*status == CODER_STATUS_FINISHED_WITH_MARK) {
				Byte temp[32];
				unsigned num = Xz_WriteVarInt(temp, p->packSize + p->blockHeaderSize + XzFlags_GetCheckSize(p->streamFlags));
				num += Xz_WriteVarInt(temp + num, p->unpackSize);
				Sha256_Update(&p->sha, temp, num);
				p->indexSize += num;
				p->numBlocks++;

				p->state = XZ_STATE_BLOCK_FOOTER;
				p->pos = 0;
				p->alignPos = 0;
			}
			else if(srcLen2 == 0 && destLen2 == 0)
				return SZ_OK;
			continue;
		}
		if(srcRem == 0) {
			*status = CODER_STATUS_NEEDS_MORE_INPUT;
			return SZ_OK;
		}
		switch(p->state) {
			case XZ_STATE_STREAM_HEADER:
		    {
			    if(p->pos < XZ_STREAM_HEADER_SIZE) {
				    if(p->pos < XZ_SIG_SIZE && *src != XZ_SIG[p->pos])
					    return SZ_ERROR_NO_ARCHIVE;
				    p->buf[p->pos++] = *src++;
				    (*srcLen)++;
			    }
			    else {
				    RINOK(Xz_ParseHeader(&p->streamFlags, p->buf));
				    p->numStartedStreams++;
				    p->state = XZ_STATE_BLOCK_HEADER;
				    Sha256_Init(&p->sha);
				    p->indexSize = 0;
				    p->numBlocks = 0;
				    p->pos = 0;
			    }
			    break;
		    }

			case XZ_STATE_BLOCK_HEADER:
		    {
			    if(p->pos == 0) {
				    p->buf[p->pos++] = *src++;
				    (*srcLen)++;
				    if(p->buf[0] == 0) {
					    p->indexPreSize = 1 + Xz_WriteVarInt(p->buf + 1, p->numBlocks);
					    p->indexPos = p->indexPreSize;
					    p->indexSize += p->indexPreSize;
					    Sha256_Final(&p->sha, p->shaDigest);
					    Sha256_Init(&p->sha);
					    p->crc = CrcUpdate(CRC_INIT_VAL, p->buf, p->indexPreSize);
					    p->state = XZ_STATE_STREAM_INDEX;
				    }
				    p->blockHeaderSize = ((uint32)p->buf[0] << 2) + 4;
			    }
			    else if(p->pos != p->blockHeaderSize) {
				    uint32 cur = p->blockHeaderSize - p->pos;
				    if(cur > srcRem)
					    cur = (uint32)srcRem;
				    memcpy(p->buf + p->pos, src, cur);
				    p->pos += cur;
				    (*srcLen) += cur;
				    src += cur;
			    }
			    else {
				    RINOK(XzBlock_Parse(&p->block, p->buf));
				    p->numTotalBlocks++;
				    p->state = XZ_STATE_BLOCK;
				    p->packSize = 0;
				    p->unpackSize = 0;
				    XzCheck_Init(&p->check, XzFlags_GetCheckType(p->streamFlags));
				    RINOK(XzDec_Init(&p->decoder, &p->block));
			    }
			    break;
		    }

			case XZ_STATE_BLOCK_FOOTER:
		    {
			    if(((p->packSize + p->alignPos) & 3) != 0) {
				    (*srcLen)++;
				    p->alignPos++;
				    if(*src++ != 0)
					    return SZ_ERROR_CRC;
			    }
			    else {
				    uint32 checkSize = XzFlags_GetCheckSize(p->streamFlags);
				    uint32 cur = checkSize - p->pos;
				    if(cur != 0) {
					    if(cur > srcRem)
						    cur = (uint32)srcRem;
					    memcpy(p->buf + p->pos, src, cur);
					    p->pos += cur;
					    (*srcLen) += cur;
					    src += cur;
				    }
				    else {
					    Byte digest[XZ_CHECK_SIZE_MAX];
					    p->state = XZ_STATE_BLOCK_HEADER;
					    p->pos = 0;
					    if(XzCheck_Final(&p->check, digest) && memcmp(digest, p->buf, checkSize) != 0)
						    return SZ_ERROR_CRC;
				    }
			    }
			    break;
		    }
			case XZ_STATE_STREAM_INDEX:
		    {
			    if(p->pos < p->indexPreSize) {
				    (*srcLen)++;
				    if(*src++ != p->buf[p->pos++])
					    return SZ_ERROR_CRC;
			    }
			    else {
				    if(p->indexPos < p->indexSize) {
					    uint64 cur = p->indexSize - p->indexPos;
					    if(srcRem > cur)
						    srcRem = (SizeT)cur;
					    p->crc = CrcUpdate(p->crc, src, srcRem);
					    Sha256_Update(&p->sha, src, srcRem);
					    (*srcLen) += srcRem;
					    src += srcRem;
					    p->indexPos += srcRem;
				    }
				    else if((p->indexPos & 3) != 0) {
					    Byte b = *src++;
					    p->crc = CRC_UPDATE_BYTE(p->crc, b);
					    (*srcLen)++;
					    p->indexPos++;
					    p->indexSize++;
					    if(b != 0)
						    return SZ_ERROR_CRC;
				    }
				    else {
					    Byte digest[SHA256_DIGEST_SIZE];
					    p->state = XZ_STATE_STREAM_INDEX_CRC;
					    p->indexSize += 4;
					    p->pos = 0;
					    Sha256_Final(&p->sha, digest);
					    if(memcmp(digest, p->shaDigest, SHA256_DIGEST_SIZE) != 0)
						    return SZ_ERROR_CRC;
				    }
			    }
			    break;
		    }
			case XZ_STATE_STREAM_INDEX_CRC:
		    {
			    if(p->pos < 4) {
				    (*srcLen)++;
				    p->buf[p->pos++] = *src++;
			    }
			    else {
				    p->state = XZ_STATE_STREAM_FOOTER;
				    p->pos = 0;
				    if(CRC_GET_DIGEST(p->crc) != GetUi32(p->buf))
					    return SZ_ERROR_CRC;
			    }
			    break;
		    }
			case XZ_STATE_STREAM_FOOTER:
		    {
			    uint32 cur = XZ_STREAM_FOOTER_SIZE - p->pos;
			    if(cur > srcRem)
				    cur = (uint32)srcRem;
			    memcpy(p->buf + p->pos, src, cur);
			    p->pos += cur;
			    (*srcLen) += cur;
			    src += cur;
			    if(p->pos == XZ_STREAM_FOOTER_SIZE) {
				    p->state = XZ_STATE_STREAM_PADDING;
				    p->numFinishedStreams++;
				    p->padSize = 0;
				    if(!Xz_CheckFooter(p->streamFlags, p->indexSize, p->buf))
					    return SZ_ERROR_CRC;
			    }
			    break;
		    }

			case XZ_STATE_STREAM_PADDING:
		    {
			    if(*src != 0) {
				    if(((uint32)p->padSize & 3) != 0)
					    return SZ_ERROR_NO_ARCHIVE;
				    p->pos = 0;
				    p->state = XZ_STATE_STREAM_HEADER;
			    }
			    else {
				    (*srcLen)++;
				    src++;
				    p->padSize++;
			    }
			    break;
		    }

			case XZ_STATE_BLOCK: break; /* to disable GCC warning */
		}
	}
	/*
	   if(p->state == XZ_STATE_FINISHED)
	   *status = CODER_STATUS_FINISHED_WITH_MARK;
	   return SZ_OK;
	 */
}

Bool FASTCALL XzUnpacker_IsStreamWasFinished(const CXzUnpacker * p)
{
	return (p->state == XZ_STATE_STREAM_PADDING) && (((uint32)p->padSize & 3) == 0);
}

uint64 FASTCALL XzUnpacker_GetExtraSize(const CXzUnpacker * p)
{
	uint64 num = 0;
	if(p->state == XZ_STATE_STREAM_PADDING)
		num += p->padSize;
	else if(p->state == XZ_STATE_STREAM_HEADER)
		num += p->padSize + p->pos;
	return num;
}
//
// XzIn.c
//
// Xz input
//
SRes Xz_ReadHeader(CXzStreamFlags * p, ISeqInStream * inStream)
{
	Byte sig[XZ_STREAM_HEADER_SIZE];
	RINOK(SeqInStream_Read2(inStream, sig, XZ_STREAM_HEADER_SIZE, SZ_ERROR_NO_ARCHIVE));
	if(memcmp(sig, XZ_SIG, XZ_SIG_SIZE) != 0)
		return SZ_ERROR_NO_ARCHIVE;
	return Xz_ParseHeader(p, sig);
}

#define READ_VARINT_AND_CHECK(buf, pos, size, res) { uint s = Xz_ReadVarInt(buf + pos, size - pos, res); if(s == 0) return SZ_ERROR_ARCHIVE; pos += s; }

SRes XzBlock_ReadHeader(CXzBlock * p, ISeqInStream * inStream, Bool * isIndex, uint32 * headerSizeRes)
{
	Byte header[XZ_BLOCK_HEADER_SIZE_MAX];
	unsigned headerSize;
	*headerSizeRes = 0;
	RINOK(SeqInStream_ReadByte(inStream, &header[0]));
	headerSize = ((uint)header[0] << 2) + 4;
	if(headerSize == 0) {
		*headerSizeRes = 1;
		*isIndex = True;
		return SZ_OK;
	}
	*isIndex = False;
	*headerSizeRes = headerSize;
	RINOK(SeqInStream_Read(inStream, header + 1, headerSize - 1));
	return XzBlock_Parse(p, header);
}

#define ADD_SIZE_CHECH(size, val) { uint64 newSize = size + (val); if(newSize < size) return XZ_SIZE_OVERFLOW; size = newSize; }

static uint64 FASTCALL Xz_GetUnpackSize(const CXzStream * p)
{
	uint64 size = 0;
	for(size_t i = 0; i < p->numBlocks; i++)
		ADD_SIZE_CHECH(size, p->blocks[i].unpackSize);
	return size;
}

static uint64 FASTCALL Xz_GetPackSize(const CXzStream * p)
{
	uint64 size = 0;
	for(size_t i = 0; i < p->numBlocks; i++)
		ADD_SIZE_CHECH(size, (p->blocks[i].totalSize + 3) & ~(uint64)3);
	return size;
}
/*
   SRes XzBlock_ReadFooter(CXzBlock *p, CXzStreamFlags f, ISeqInStream *inStream)
   {
   return SeqInStream_Read(inStream, p->check, XzFlags_GetCheckSize(f));
   }
 */
static SRes Xz_ReadIndex2(CXzStream * p, const Byte * buf, size_t size, ISzAllocPtr alloc)
{
	size_t numBlocks, pos = 1;
	uint32 crc;
	if(size < 5 || buf[0] != 0)
		return SZ_ERROR_ARCHIVE;
	size -= 4;
	crc = CrcCalc(buf, size);
	if(crc != GetUi32(buf + size))
		return SZ_ERROR_ARCHIVE;
	{
		uint64 numBlocks64;
		READ_VARINT_AND_CHECK(buf, pos, size, &numBlocks64);
		numBlocks = (size_t)numBlocks64;
		if(numBlocks != numBlocks64 || numBlocks * 2 > size)
			return SZ_ERROR_ARCHIVE;
	}
	Xz_Free(p, alloc);
	if(numBlocks != 0) {
		size_t i;
		p->numBlocks = numBlocks;
		p->numBlocksAllocated = numBlocks;
		p->blocks = (CXzBlockSizes *)ISzAlloc_Alloc(alloc, sizeof(CXzBlockSizes) * numBlocks);
		if(p->blocks == 0)
			return SZ_ERROR_MEM;
		for(i = 0; i < numBlocks; i++) {
			CXzBlockSizes * block = &p->blocks[i];
			READ_VARINT_AND_CHECK(buf, pos, size, &block->totalSize);
			READ_VARINT_AND_CHECK(buf, pos, size, &block->unpackSize);
			if(block->totalSize == 0)
				return SZ_ERROR_ARCHIVE;
		}
	}
	while((pos & 3) != 0)
		if(buf[pos++] != 0)
			return SZ_ERROR_ARCHIVE;
	return (pos == size) ? SZ_OK : SZ_ERROR_ARCHIVE;
}

static SRes Xz_ReadIndex(CXzStream * p, ILookInStream * stream, uint64 indexSize, ISzAllocPtr alloc)
{
	SRes res;
	size_t size;
	Byte * buf;
	if(indexSize > ((uint32)1 << 31))
		return SZ_ERROR_UNSUPPORTED;
	size = (size_t)indexSize;
	if(size != indexSize)
		return SZ_ERROR_UNSUPPORTED;
	buf = (Byte *)ISzAlloc_Alloc(alloc, size);
	if(buf == 0)
		return SZ_ERROR_MEM;
	res = LookInStream_Read2(stream, buf, size, SZ_ERROR_UNSUPPORTED);
	if(res == SZ_OK)
		res = Xz_ReadIndex2(p, buf, size, alloc);
	ISzAlloc_Free(alloc, buf);
	return res;
}

static SRes FASTCALL LookInStream_SeekRead_ForArc(ILookInStream * stream, uint64 offset, void * buf, size_t size)
{
	RINOK(LookInStream_SeekTo(stream, offset));
	return LookInStream_Read(stream, buf, size);
	/* return LookInStream_Read2(stream, buf, size, SZ_ERROR_NO_ARCHIVE); */
}

static SRes Xz_ReadBackward(CXzStream * p, ILookInStream * stream, int64 * startOffset, ISzAllocPtr alloc)
{
	uint64 indexSize;
	Byte buf[XZ_STREAM_FOOTER_SIZE];
	uint64 pos = *startOffset;
	if((pos & 3) != 0 || pos < XZ_STREAM_FOOTER_SIZE)
		return SZ_ERROR_NO_ARCHIVE;
	pos -= XZ_STREAM_FOOTER_SIZE;
	RINOK(LookInStream_SeekRead_ForArc(stream, pos, buf, XZ_STREAM_FOOTER_SIZE));
	if(memcmp(buf + 10, XZ_FOOTER_SIG, XZ_FOOTER_SIG_SIZE) != 0) {
		uint32 total = 0;
		pos += XZ_STREAM_FOOTER_SIZE;
		for(;;) {
			size_t i;
      #define TEMP_BUF_SIZE (1 << 10)
			Byte temp[TEMP_BUF_SIZE];
			i = (pos > TEMP_BUF_SIZE) ? TEMP_BUF_SIZE : (size_t)pos;
			pos -= i;
			RINOK(LookInStream_SeekRead_ForArc(stream, pos, temp, i));
			total += (uint32)i;
			for(; i != 0; i--)
				if(temp[i - 1] != 0)
					break;
			if(i != 0) {
				if((i & 3) != 0)
					return SZ_ERROR_NO_ARCHIVE;
				pos += i;
				break;
			}
			if(pos < XZ_STREAM_FOOTER_SIZE || total > (1 << 16))
				return SZ_ERROR_NO_ARCHIVE;
		}
		if(pos < XZ_STREAM_FOOTER_SIZE)
			return SZ_ERROR_NO_ARCHIVE;
		pos -= XZ_STREAM_FOOTER_SIZE;
		RINOK(LookInStream_SeekRead_ForArc(stream, pos, buf, XZ_STREAM_FOOTER_SIZE));
		if(memcmp(buf + 10, XZ_FOOTER_SIG, XZ_FOOTER_SIG_SIZE) != 0)
			return SZ_ERROR_NO_ARCHIVE;
	}
	p->flags = (CXzStreamFlags)GetBe16(buf + 8);
	if(!XzFlags_IsSupported(p->flags))
		return SZ_ERROR_UNSUPPORTED;
	if(GetUi32(buf) != CrcCalc(buf + 4, 6))
		return SZ_ERROR_ARCHIVE;
	indexSize = ((uint64)GetUi32(buf + 4) + 1) << 2;
	if(pos < indexSize)
		return SZ_ERROR_ARCHIVE;
	pos -= indexSize;
	RINOK(LookInStream_SeekTo(stream, pos));
	RINOK(Xz_ReadIndex(p, stream, indexSize, alloc));
	{
		uint64 totalSize = Xz_GetPackSize(p);
		if(totalSize == XZ_SIZE_OVERFLOW || totalSize >= ((uint64)1 << 63) || pos < totalSize + XZ_STREAM_HEADER_SIZE)
			return SZ_ERROR_ARCHIVE;
		pos -= (totalSize + XZ_STREAM_HEADER_SIZE);
		RINOK(LookInStream_SeekTo(stream, pos));
		*startOffset = pos;
	}
	{
		CXzStreamFlags headerFlags;
		CSecToRead secToRead;
		SecToRead_CreateVTable(&secToRead);
		secToRead.realStream = stream;
		RINOK(Xz_ReadHeader(&headerFlags, &secToRead.vt));
		return (p->flags == headerFlags) ? SZ_OK : SZ_ERROR_ARCHIVE;
	}
}
//
// Xz Streams
//
void Xzs_Construct(CXzs * p)
{
	p->num = p->numAllocated = 0;
	p->streams = 0;
}

void Xzs_Free(CXzs * p, ISzAllocPtr alloc)
{
	for(size_t i = 0; i < p->num; i++)
		Xz_Free(&p->streams[i], alloc);
	ISzAlloc_Free(alloc, p->streams);
	p->num = p->numAllocated = 0;
	p->streams = 0;
}

uint64 Xzs_GetNumBlocks(const CXzs * p)
{
	uint64 num = 0;
	for(size_t i = 0; i < p->num; i++)
		num += p->streams[i].numBlocks;
	return num;
}

uint64 Xzs_GetUnpackSize(const CXzs * p)
{
	uint64 size = 0;
	for(size_t i = 0; i < p->num; i++)
		ADD_SIZE_CHECH(size, Xz_GetUnpackSize(&p->streams[i]));
	return size;
}
/*
   uint64 Xzs_GetPackSize(const CXzs *p)
   {
   uint64 size = 0;
   size_t i;
   for(i = 0; i < p->num; i++)
    ADD_SIZE_CHECH(size, Xz_GetTotalSize(&p->streams[i]));
   return size;
   }
 */
SRes Xzs_ReadBackward(CXzs * p, ILookInStream * stream, int64 * startOffset, ICompressProgress * progress, ISzAllocPtr alloc)
{
	int64 endOffset = 0;
	RINOK(ILookInStream_Seek(stream, &endOffset, SZ_SEEK_END));
	*startOffset = endOffset;
	for(;;) {
		CXzStream st;
		SRes res;
		Xz_Construct(&st);
		res = Xz_ReadBackward(&st, stream, startOffset, alloc);
		st.startOffset = *startOffset;
		RINOK(res);
		if(p->num == p->numAllocated) {
			size_t newNum = p->num + p->num / 4 + 1;
			Byte * data = (Byte *)ISzAlloc_Alloc(alloc, newNum * sizeof(CXzStream));
			if(data == 0)
				return SZ_ERROR_MEM;
			p->numAllocated = newNum;
			if(p->num != 0)
				memcpy(data, p->streams, p->num * sizeof(CXzStream));
			ISzAlloc_Free(alloc, p->streams);
			p->streams = (CXzStream*)data;
		}
		p->streams[p->num++] = st;
		if(*startOffset == 0)
			break;
		RINOK(LookInStream_SeekTo(stream, *startOffset));
		if(progress && ICompressProgress_Progress(progress, endOffset - *startOffset, (uint64)(int64)-1) != SZ_OK)
			return SZ_ERROR_PROGRESS;
	}
	return SZ_OK;
}
