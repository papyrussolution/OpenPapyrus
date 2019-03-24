// 7Z-STREAM.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
#ifndef _WIN32
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
#endif
//
// StreamUtils.cpp
static const uint32 kBlockSize = ((uint32)1 << 31);

HRESULT ReadStream(ISequentialInStream * stream, void * data, size_t * processedSize) throw()
{
	size_t size = *processedSize;
	*processedSize = 0;
	while(size != 0) {
		uint32 curSize = (size < kBlockSize) ? (uint32)size : kBlockSize;
		uint32 processedSizeLoc;
		HRESULT res = stream->Read(data, curSize, &processedSizeLoc);
		*processedSize += processedSizeLoc;
		data = (void *)((Byte *)data + processedSizeLoc);
		size -= processedSizeLoc;
		RINOK(res);
		if(processedSizeLoc == 0)
			return S_OK;
	}
	return S_OK;
}

HRESULT ReadStream_FALSE(ISequentialInStream * stream, void * data, size_t size) throw()
{
	size_t processedSize = size;
	RINOK(ReadStream(stream, data, &processedSize));
	return (size == processedSize) ? S_OK : S_FALSE;
}

HRESULT ReadStream_FAIL(ISequentialInStream * stream, void * data, size_t size) throw()
{
	size_t processedSize = size;
	RINOK(ReadStream(stream, data, &processedSize));
	return (size == processedSize) ? S_OK : E_FAIL;
}

HRESULT WriteStream(ISequentialOutStream * stream, const void * data, size_t size) throw()
{
	while(size != 0) {
		uint32 curSize = (size < kBlockSize) ? (uint32)size : kBlockSize;
		uint32 processedSizeLoc;
		HRESULT res = stream->Write(data, curSize, &processedSizeLoc);
		data = (const void*)((const Byte*)data + processedSizeLoc);
		size -= processedSizeLoc;
		RINOK(res);
		if(processedSizeLoc == 0)
			return E_FAIL;
	}
	return S_OK;
}
//
// StreamObjects.cpp
STDMETHODIMP CBufferInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(_pos >= Buf.Size())
		return S_OK;
	size_t rem = Buf.Size() - (size_t)_pos;
	if(rem > size)
		rem = (size_t)size;
	memcpy(data, (const Byte*)Buf + (size_t)_pos, rem);
	_pos += rem;
	ASSIGN_PTR(processedSize, (uint32)rem);
	return S_OK;
}

STDMETHODIMP CBufferInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _pos; break;
		case STREAM_SEEK_END: offset += Buf.Size(); break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	_pos = offset;
	ASSIGN_PTR(newPosition, offset);
	return S_OK;
}

void CBufInStream::Init(const Byte * data, size_t size, IUnknown * ref/*= 0*/)
{
	_data = data;
	_size = size;
	_pos = 0;
	_ref = ref;
}

void FASTCALL CBufInStream::Init(CReferenceBuf * ref) 
{
	Init(ref->Buf, ref->Buf.Size(), ref);
}

STDMETHODIMP CBufInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(_pos >= _size)
		return S_OK;
	size_t rem = _size - (size_t)_pos;
	if(rem > size)
		rem = (size_t)size;
	memcpy(data, _data + (size_t)_pos, rem);
	_pos += rem;
	ASSIGN_PTR(processedSize, (uint32)rem);
	return S_OK;
}

STDMETHODIMP CBufInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _pos; break;
		case STREAM_SEEK_END: offset += _size; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	else {
		_pos = offset;
		ASSIGN_PTR(newPosition, offset);
		return S_OK;
	}
}

void Create_BufInStream_WithReference(const void * data, size_t size, IUnknown * ref, ISequentialInStream ** stream)
{
	*stream = NULL;
	CBufInStream * inStreamSpec = new CBufInStream;
	CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
	inStreamSpec->Init((const Byte*)data, size, ref);
	*stream = streamTemp.Detach();
}

void Create_BufInStream_WithNewBuffer(const void * data, size_t size, ISequentialInStream ** stream)
{
	*stream = NULL;
	CBufferInStream * inStreamSpec = new CBufferInStream;
	CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
	inStreamSpec->Buf.CopyFrom((const Byte*)data, size);
	inStreamSpec->Init();
	*stream = streamTemp.Detach();
}

void CBufPtrSeqOutStream::Init(Byte * buffer, size_t size)
{
	_buffer = buffer;
	_pos = 0;
	_size = size;
}

STDMETHODIMP CBufPtrSeqOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	size_t rem = _size - _pos;
	SETMIN(rem, (size_t)size);
	if(rem != 0) {
		memcpy(_buffer + _pos, data, rem);
		_pos += rem;
	}
	ASSIGN_PTR(processedSize, (uint32)rem);
	return (rem != 0 || size == 0) ? S_OK : E_FAIL;
}

static const uint64 kEmptyTag = static_cast<uint64>(-1LL);

CCachedInStream::CCachedInStream() : _tags(0), _data(0) 
{
}

CCachedInStream::~CCachedInStream()  // the destructor must be virtual (release calls it) !!!
{
	Free();
} 

void CCachedInStream::Free() throw()
{
	ZFREE(_tags);
	MidFree(_data);
	_data = 0;
}

bool CCachedInStream::Alloc(unsigned blockSizeLog, unsigned numBlocksLog) throw()
{
	unsigned sizeLog = blockSizeLog + numBlocksLog;
	if(sizeLog >= sizeof(size_t) * 8)
		return false;
	size_t dataSize = (size_t)1 << sizeLog;
	if(_data == 0 || dataSize != _dataSize) {
		MidFree(_data);
		_data = (Byte *)MidAlloc(dataSize);
		if(_data == 0)
			return false;
		_dataSize = dataSize;
	}
	if(_tags == 0 || numBlocksLog != _numBlocksLog) {
		SAlloc::F(_tags);
		_tags = (uint64 *)SAlloc::M(sizeof(uint64) << numBlocksLog);
		if(_tags == 0)
			return false;
		_numBlocksLog = numBlocksLog;
	}
	_blockSizeLog = blockSizeLog;
	return true;
}

void CCachedInStream::Init(uint64 size) throw()
{
	_size = size;
	_pos = 0;
	size_t numBlocks = (size_t)1 << _numBlocksLog;
	for(size_t i = 0; i < numBlocks; i++)
		_tags[i] = kEmptyTag;
}

STDMETHODIMP CCachedInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size && _pos < _size) {
		{
			const uint64 rem = _size - _pos;
			SETMIN(size, (uint32)rem);
		}
		while(size != 0) {
			uint64 cacheTag = _pos >> _blockSizeLog;
			size_t cacheIndex = (size_t)cacheTag & (((size_t)1 << _numBlocksLog) - 1);
			Byte * p = _data + (cacheIndex << _blockSizeLog);
			if(_tags[cacheIndex] != cacheTag) {
				const  uint64 remInBlock = _size - (cacheTag << _blockSizeLog);
				size_t blockSize = (size_t)1 << _blockSizeLog;
				SETMIN(blockSize, (size_t)remInBlock);
				RINOK(ReadBlock(cacheTag, p, blockSize));
				_tags[cacheIndex] = cacheTag;
			}
			size_t offset = (size_t)_pos & (((size_t)1 << _blockSizeLog) - 1);
			uint32 cur = (uint32)MyMin(((size_t)1 << _blockSizeLog) - offset, (size_t)size);
			memcpy(data, p + offset, cur);
			if(processedSize)
				*processedSize += cur;
			data = (void *)((const Byte*)data + cur);
			_pos += cur;
			size -= cur;
		}
	}
	return S_OK;
}

STDMETHODIMP CCachedInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _pos; break;
		case STREAM_SEEK_END: offset += _size; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	else {
		_pos = offset;
		ASSIGN_PTR(newPosition, offset);
		return S_OK;
	}
}
//
// StreamBinder.cpp
WRes CStreamBinder::CreateEvents()
{
	RINOK(_canWrite_Event.Create());
	RINOK(_canRead_Event.Create());
	return _readingWasClosed_Event.Create();
}

void CStreamBinder::ReInit()
{
	_canWrite_Event.Reset();
	_canRead_Event.Reset();
	_readingWasClosed_Event.Reset();
	// _readingWasClosed = false;
	_readingWasClosed2 = false;
	_waitWrite = true;
	_bufSize = 0;
	_buf = NULL;
	ProcessedSize = 0;
	// WritingWasCut = false;
}

void CStreamBinder::CreateStreams(ISequentialInStream ** inStream, ISequentialOutStream ** outStream)
{
	class CBinderInStream : public ISequentialInStream, public CMyUnknownImp {
	public:
		CBinderInStream(CStreamBinder * binder) : _binder(binder) 
		{
		}
		~CBinderInStream() 
		{
			_binder->CloseRead();
		}
		MY_UNKNOWN_IMP1(ISequentialInStream)
		STDMETHOD(Read)(void * data, uint32 size, uint32 * processedSize) { return _binder->Read(data, size, processedSize); }
	private:
		CStreamBinder * _binder;
	};
	class CBinderOutStream : public ISequentialOutStream, public CMyUnknownImp {
	public:
		CBinderOutStream(CStreamBinder * binder) : _binder(binder) 
		{
		}
		~CBinderOutStream() 
		{
			_binder->CloseWrite();
		}
		MY_UNKNOWN_IMP1(ISequentialOutStream)
		STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize) { return _binder->Write(data, size, processedSize); }
	private:
		CStreamBinder * _binder;
	};
	// _readingWasClosed = false;
	_readingWasClosed2 = false;
	_waitWrite = true;
	_bufSize = 0;
	_buf = NULL;
	ProcessedSize = 0;
	// WritingWasCut = false;
	CBinderInStream * inStreamSpec = new CBinderInStream(this);
	CMyComPtr <ISequentialInStream> inStreamLoc(inStreamSpec);
	*inStream = inStreamLoc.Detach();
	CBinderOutStream * outStreamSpec = new CBinderOutStream(this);
	CMyComPtr<ISequentialOutStream> outStreamLoc(outStreamSpec);
	*outStream = outStreamLoc.Detach();
}

// (_canRead_Event && _bufSize == 0) means that stream is finished.

HRESULT CStreamBinder::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size != 0) {
		if(_waitWrite) {
			RINOK(_canRead_Event.Lock());
			_waitWrite = false;
		}
		SETMIN(size, _bufSize);
		if(size != 0) {
			memcpy(data, _buf, size);
			_buf = ((const Byte*)_buf) + size;
			ProcessedSize += size;
			ASSIGN_PTR(processedSize, size);
			_bufSize -= size;
			if(_bufSize == 0) {
				_waitWrite = true;
				_canRead_Event.Reset();
				_canWrite_Event.Set();
			}
		}
	}
	return S_OK;
}

HRESULT CStreamBinder::Write(const void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(!_readingWasClosed2) {
		_buf = data;
		_bufSize = size;
		_canRead_Event.Set();
		/*
		   _canWrite_Event.Lock();
		   if(_readingWasClosed)
		   _readingWasClosed2 = true;
		 */
		HANDLE events[2] = { _canWrite_Event, _readingWasClosed_Event };
		DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
		if(waitResult >= WAIT_OBJECT_0 + 2)
			return E_FAIL;
		size -= _bufSize;
		if(size != 0) {
			ASSIGN_PTR(processedSize, size);
			return S_OK;
		}
		// if(waitResult == WAIT_OBJECT_0 + 1)
		_readingWasClosed2 = true;
	}
	// WritingWasCut = true;
	return k_My_HRESULT_WritingWasCut;
}

void CStreamBinder::CloseRead()
{
	_readingWasClosed_Event.Set();
	// _readingWasClosed = true;
	// _canWrite_Event.Set();
}

void CStreamBinder::CloseWrite()
{
	_buf = NULL;
	_bufSize = 0;
	_canRead_Event.Set();
}
//
// 7zSpecStream.cpp
void CSequentialInStreamSizeCount2::Init(ISequentialInStream * stream)
{
	_size = 0;
	_getSubStreamSize.Release();
	_stream = stream;
	_stream.QueryInterface(IID_ICompressGetSubStreamSize, &_getSubStreamSize);
}

STDMETHODIMP CSequentialInStreamSizeCount2::Read(void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessedSize;
	HRESULT result = _stream->Read(data, size, &realProcessedSize);
	_size += realProcessedSize;
	ASSIGN_PTR(processedSize, realProcessedSize);
	return result;
}

STDMETHODIMP CSequentialInStreamSizeCount2::GetSubStreamSize(uint64 subStream, uint64 * value)
{
	return _getSubStreamSize ? _getSubStreamSize->GetSubStreamSize(subStream, value) : E_NOTIMPL;
}
//
// DummyOutStream.cpp
STDMETHODIMP CDummyOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessedSize = size;
	HRESULT res = S_OK;
	if(_stream)
		res = _stream->Write(data, size, &realProcessedSize);
	_size += realProcessedSize;
	ASSIGN_PTR(processedSize, realProcessedSize);
	return res;
}
//
// OutStreamWithCRC.cpp
void COutStreamWithCRC::Init(bool calculate /*= true*/)
{
	_size = 0;
	_calculate = calculate;
	_crc = CRC_INIT_VAL;
}

void COutStreamWithCRC::SetStream(ISequentialOutStream * stream) { _stream = stream; }
void COutStreamWithCRC::ReleaseStream() { _stream.Release(); }
void COutStreamWithCRC::EnableCalc(bool calculate) { _calculate = calculate; }
void COutStreamWithCRC::InitCRC() { _crc = CRC_INIT_VAL; }
uint32 COutStreamWithCRC::GetCRC() const { return CRC_GET_DIGEST(_crc); }

STDMETHODIMP COutStreamWithCRC::Write(const void * data, uint32 size, uint32 * processedSize)
{
	HRESULT result = S_OK;
	if(_stream)
		result = _stream->Write(data, size, &size);
	if(_calculate)
		_crc = CrcUpdate(_crc, data, size);
	_size += size;
	ASSIGN_PTR(processedSize, size);
	return result;
}
//
// LimitedStreams.cpp
void CLimitedSequentialInStream::Init(uint64 streamSize)
{
	_size = streamSize;
	_pos = 0;
	_wasFinished = false;
}

void CLimitedSequentialInStream::SetStream(ISequentialInStream * stream) { _stream = stream; }
void CLimitedSequentialInStream::ReleaseStream() { _stream.Release(); }

STDMETHODIMP CLimitedSequentialInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessedSize = 0;
	{
		const uint64 rem = _size - _pos;
		SETMIN(size, (uint32)rem);
	}
	HRESULT result = S_OK;
	if(size != 0) {
		result = _stream->Read(data, size, &realProcessedSize);
		_pos += realProcessedSize;
		if(realProcessedSize == 0)
			_wasFinished = true;
	}
	ASSIGN_PTR(processedSize, realProcessedSize);
	return result;
}

STDMETHODIMP CLimitedInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(_virtPos >= _size) {
		// 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
		return S_OK;
		// return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
	}
	{
		const uint64 rem = _size - _virtPos;
		SETMIN(size, (uint32)rem);
	}
	uint64 newPos = _startOffset + _virtPos;
	if(newPos != _physPos) {
		_physPos = newPos;
		RINOK(SeekToPhys());
	}
	HRESULT res = _stream->Read(data, size, &size);
	ASSIGN_PTR(processedSize, size);
	_physPos += size;
	_virtPos += size;
	return res;
}

HRESULT CLimitedInStream::SeekToPhys() 
{ 
	return _stream->Seek(_physPos, STREAM_SEEK_SET, NULL); 
}

HRESULT CLimitedInStream::InitAndSeek(uint64 startOffset, uint64 size)
{
	_startOffset = startOffset;
	_physPos = startOffset;
	_virtPos = 0;
	_size = size;
	return SeekToPhys();
}

STDMETHODIMP CLimitedInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _virtPos; break;
		case STREAM_SEEK_END: offset += _size; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	else {
		_virtPos = offset;
		ASSIGN_PTR(newPosition, _virtPos);
		return S_OK;
	}
}

HRESULT CreateLimitedInStream(IInStream * inStream, uint64 pos, uint64 size, ISequentialInStream ** resStream)
{
	*resStream = 0;
	CLimitedInStream * streamSpec = new CLimitedInStream;
	CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
	streamSpec->SetStream(inStream);
	RINOK(streamSpec->InitAndSeek(pos, size));
	streamSpec->SeekToStart();
	*resStream = streamTemp.Detach();
	return S_OK;
}

HRESULT CClusterInStream::SeekToPhys() { return Stream->Seek(_physPos, STREAM_SEEK_SET, NULL); }

HRESULT CClusterInStream::InitAndSeek()
{
	_curRem = 0;
	_virtPos = 0;
	_physPos = StartOffset;
	if(Vector.Size() > 0) {
		_physPos = StartOffset + (Vector[0] << BlockSizeLog);
		return SeekToPhys();
	}
	return S_OK;
}

STDMETHODIMP CClusterInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(_virtPos >= Size)
		return S_OK;
	{
		uint64 rem = Size - _virtPos;
		SETMIN(size, (uint32)rem);
	}
	if(size == 0)
		return S_OK;
	if(_curRem == 0) {
		const uint32 blockSize = (uint32)1 << BlockSizeLog;
		const uint32 virtBlock = (uint32)(_virtPos >> BlockSizeLog);
		const uint32 offsetInBlock = (uint32)_virtPos & (blockSize - 1);
		const uint32 phyBlock = Vector[virtBlock];
		uint64 newPos = StartOffset + ((uint64)phyBlock << BlockSizeLog) + offsetInBlock;
		if(newPos != _physPos) {
			_physPos = newPos;
			RINOK(SeekToPhys());
		}
		_curRem = blockSize - offsetInBlock;
		for(int i = 1; i < 64 && (virtBlock + i) < (uint32)Vector.Size() && phyBlock + i == Vector[virtBlock + i]; i++)
			_curRem += (uint32)1 << BlockSizeLog;
	}
	if(size > _curRem)
		size = _curRem;
	HRESULT res = Stream->Read(data, size, &size);
	ASSIGN_PTR(processedSize, size);
	_physPos += size;
	_virtPos += size;
	_curRem -= size;
	return res;
}

STDMETHODIMP CClusterInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _virtPos; break;
		case STREAM_SEEK_END: offset += Size; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	if(_virtPos != (uint64)offset)
		_curRem = 0;
	_virtPos = offset;
	ASSIGN_PTR(newPosition, offset);
	return S_OK;
}

void CLimitedSequentialOutStream::Init(uint64 size, bool overflowIsAllowed/*= false*/)
{
	_size = size;
	_overflow = false;
	_overflowIsAllowed = overflowIsAllowed;
}

void CLimitedSequentialOutStream::SetStream(ISequentialOutStream * stream) { _stream = stream; }
void CLimitedSequentialOutStream::ReleaseStream() { _stream.Release(); }
bool CLimitedSequentialOutStream::IsFinishedOK() const { return (_size == 0 && !_overflow); }

STDMETHODIMP CLimitedSequentialOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	HRESULT result = S_OK;
	ASSIGN_PTR(processedSize, 0);
	if(size > _size) {
		if(_size == 0) {
			_overflow = true;
			if(!_overflowIsAllowed)
				return E_FAIL;
			ASSIGN_PTR(processedSize, size);
			return S_OK;
		}
		size = (uint32)_size;
	}
	if(_stream)
		result = _stream->Write(data, size, &size);
	_size -= size;
	ASSIGN_PTR(processedSize, size);
	return result;
}

void CLimitedCachedInStream::SetCache(size_t cacheSize, size_t cachePos)
{
	_cache = Buffer;
	_cacheSize = cacheSize;
	_cachePhyPos = cachePos;
}

HRESULT CLimitedCachedInStream::InitAndSeek(uint64 startOffset, uint64 size)
{
	_startOffset = startOffset;
	_physPos = startOffset;
	_virtPos = 0;
	_size = size;
	return SeekToPhys();
}

STDMETHODIMP CLimitedCachedInStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(_virtPos >= _size) {
		// 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
		return S_OK;
		// return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
	}
	uint64 rem = _size - _virtPos;
	SETMIN(size, (uint32)rem);
	uint64 newPos = _startOffset + _virtPos;
	uint64 offsetInCache = newPos - _cachePhyPos;
	HRESULT res = S_OK;
	if(newPos >= _cachePhyPos && offsetInCache <= _cacheSize && size <= _cacheSize - (size_t)offsetInCache) {
		if(size != 0)
			memcpy(data, _cache + (size_t)offsetInCache, size);
	}
	else {
		if(newPos != _physPos) {
			_physPos = newPos;
			RINOK(SeekToPhys());
		}
		res = _stream->Read(data, size, &size);
		_physPos += size;
	}
	ASSIGN_PTR(processedSize, size);
	_virtPos += size;
	return res;
}

STDMETHODIMP CLimitedCachedInStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _virtPos; break;
		case STREAM_SEEK_END: offset += _size; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	_virtPos = offset;
	ASSIGN_PTR(newPosition, _virtPos);
	return S_OK;
}

STDMETHODIMP CTailOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	uint32 cur;
	HRESULT res = Stream->Write(data, size, &cur);
	ASSIGN_PTR(processedSize, cur);
	_virtPos += cur;
	SETMAX(_virtSize, _virtPos);
	return res;
}

STDMETHODIMP CTailOutStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _virtPos; break;
		case STREAM_SEEK_END: offset += _virtSize; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	else {
		_virtPos = offset;
		ASSIGN_PTR(newPosition, _virtPos);
		return Stream->Seek(Offset + _virtPos, STREAM_SEEK_SET, NULL);
	}
}

STDMETHODIMP CTailOutStream::SetSize(uint64 newSize)
{
	_virtSize = newSize;
	return Stream->SetSize(Offset + newSize);
}
//
// InStreamWithCRC.cpp
void CSequentialInStreamWithCRC::Init()
{
	_size = 0;
	_wasFinished = false;
	_crc = CRC_INIT_VAL;
}

void   CSequentialInStreamWithCRC::SetStream(ISequentialInStream * stream) { _stream = stream; }
void   CSequentialInStreamWithCRC::ReleaseStream() { _stream.Release(); }
uint32 CSequentialInStreamWithCRC::GetCRC() const { return CRC_GET_DIGEST(_crc); }

STDMETHODIMP CSequentialInStreamWithCRC::Read(void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessed = 0;
	HRESULT result = S_OK;
	if(_stream)
		result = _stream->Read(data, size, &realProcessed);
	_size += realProcessed;
	if(size != 0 && realProcessed == 0)
		_wasFinished = true;
	_crc = CrcUpdate(_crc, data, realProcessed);
	ASSIGN_PTR(processedSize, realProcessed);
	return result;
}

STDMETHODIMP CInStreamWithCRC::Read(void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessed = 0;
	HRESULT result = S_OK;
	if(_stream)
		result = _stream->Read(data, size, &realProcessed);
	_size += realProcessed;
	/*
	   if(size != 0 && realProcessed == 0)
	   _wasFinished = true;
	 */
	_crc = CrcUpdate(_crc, data, realProcessed);
	ASSIGN_PTR(processedSize, realProcessed);
	return result;
}

STDMETHODIMP CInStreamWithCRC::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin != STREAM_SEEK_SET || offset != 0)
		return E_FAIL;
	else {
		_size = 0;
		_crc = CRC_INIT_VAL;
		return _stream->Seek(offset, seekOrigin, newPosition);
	}
}
//
// StdInStream.cpp StdOutStream.cpp

// #define kEOFMessage "Unexpected end of input stream"
// #define kReadErrorMessage "Error reading input stream"
// #define kIllegalCharMessage "Illegal zero character in input stream"

extern int g_CodePage;

CStdInStream g_StdIn(stdin);
CStdOutStream g_StdOut(stdout);
CStdOutStream g_StdErr(stderr);

bool CStdInStream::Open(LPCTSTR fileName) throw()
{
	Close();
	_stream = _tfopen(fileName, TEXT("r"));
	_streamIsOpen = (_stream != 0);
	return _streamIsOpen;
}

bool CStdInStream::Close() throw()
{
	if(!_streamIsOpen)
		return true;
	_streamIsOpen = (fclose(_stream) != 0);
	return !_streamIsOpen;
}

bool CStdInStream::ScanAStringUntilNewLine(AString &s)
{
	s.Empty();
	for(;; ) {
		int intChar = GetChar();
		if(intChar == EOF)
			return true;
		char c = (char)intChar;
		if(c == 0)
			return false;
		if(c == '\n')
			return true;
		s += c;
	}
}

bool CStdInStream::ScanUStringUntilNewLine(UString &dest)
{
	dest.Empty();
	AString s;
	bool res = ScanAStringUntilNewLine(s);
	int codePage = g_CodePage;
	if(codePage == -1)
		codePage = CP_OEMCP;
	if(codePage == CP_UTF8)
		ConvertUTF8ToUnicode(s, dest);
	else
		MultiByteToUnicodeString2(dest, s, (UINT)codePage);
	return res;
}
/*
   bool CStdInStream::ReadToString(AString &resultString)
   {
   resultString.Empty();
   for(;;) {
    int intChar = GetChar();
    if(intChar == EOF)
      return !Error();
    char c = (char)intChar;
    if(c == 0)
      return false;
    resultString += c;
   }
   }
 */
int CStdInStream::GetChar()
{
	return fgetc(_stream); // getc() doesn't work in BeOS?
}
//
//
//
bool CStdOutStream::Open(const char * fileName) throw()
{
	Close();
	_stream = fopen(fileName, "wt");
	_streamIsOpen = (_stream != 0);
	return _streamIsOpen;
}

bool CStdOutStream::Close() throw()
{
	if(!_streamIsOpen)
		return true;
	else if(fclose(_stream) != 0)
		return false;
	else {
		_stream = 0;
		_streamIsOpen = false;
		return true;
	}
}

bool CStdOutStream::Flush() throw()
{
	return (fflush(_stream) == 0);
}

CStdOutStream & endl(CStdOutStream & outStream) throw()
{
	return outStream << '\n';
}

CStdOutStream & CStdOutStream::operator << (const wchar_t * s)
{
	int codePage = g_CodePage;
	if(codePage == -1)
		codePage = CP_OEMCP;
	AString dest;
	if(codePage == CP_UTF8)
		ConvertUnicodeToUTF8(s, dest);
	else
		UnicodeStringToMultiByte2(dest, s, (UINT)codePage);
	return operator<<((const char *)dest);
}

void StdOut_Convert_UString_to_AString(const UString &s, AString &temp)
{
	int codePage = g_CodePage;
	if(codePage == -1)
		codePage = CP_OEMCP;
	if(codePage == CP_UTF8)
		ConvertUnicodeToUTF8(s, temp);
	else
		UnicodeStringToMultiByte2(temp, s, (UINT)codePage);
}

void CStdOutStream::PrintUString(const UString &s, AString &temp)
{
	StdOut_Convert_UString_to_AString(s, temp);
	*this << (const char *)temp;
}

CStdOutStream & CStdOutStream::operator<<(int32 number) throw()
{
	char s[32];
	ConvertInt64ToString(number, s);
	return operator<<(s);
}

CStdOutStream & CStdOutStream::operator << (int64 number) throw()
{
	char s[32];
	ConvertInt64ToString(number, s);
	return operator<<(s);
}

CStdOutStream & CStdOutStream::operator << (uint32 number) throw()
{
	char s[16];
	ConvertUInt32ToString(number, s);
	return operator<<(s);
}

CStdOutStream & CStdOutStream::operator << (uint64 number) throw()
{
	char s[32];
	ConvertUInt64ToString(number, s);
	return operator<<(s);
}
//
// FileStreams.cpp
static inline HRESULT FASTCALL ConvertBoolToHRESULT(bool result)
{
  #ifdef _WIN32
	if(result)
		return S_OK;
	DWORD lastError = ::GetLastError();
	if(lastError == 0)
		return E_FAIL;
	return HRESULT_FROM_WIN32(lastError);
  #else
	return result ? S_OK : E_FAIL;
  #endif
}

static const uint32 kClusterSize = 1 << 18;

CInFileStream::CInFileStream() :
  #ifdef SUPPORT_DEVICE_FILE
	VirtPos(0), PhyPos(0), Buf(0), BufSize(0),
  #endif
	SupportHardLinks(false), Callback(NULL), CallbackRef(0)
{
}

CInFileStream::~CInFileStream()
{
#ifdef SUPPORT_DEVICE_FILE
	MidFree(Buf);
#endif
	CALLPTRMEMB(Callback, InFileStream_On_Destroy(CallbackRef));
}

STDMETHODIMP CInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
{
  #ifdef USE_WIN_FILE

  #ifdef SUPPORT_DEVICE_FILE
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(File.IsDeviceFile) {
		if(File.SizeDefined) {
			if(VirtPos >= File.Size)
				return VirtPos == File.Size ? S_OK : E_FAIL;
			uint64 rem = File.Size - VirtPos;
			if(size > rem)
				size = (uint32)rem;
		}
		for(;; ) {
			const uint32 mask = kClusterSize - 1;
			const uint64 mask2 = ~(uint64)mask;
			uint64 alignedPos = VirtPos & mask2;
			if(BufSize > 0 && BufStartPos == alignedPos) {
				uint32 pos = (uint32)VirtPos & mask;
				if(pos >= BufSize)
					return S_OK;
				uint32 rem = MyMin(BufSize - pos, size);
				memcpy(data, Buf + pos, rem);
				VirtPos += rem;
				if(processedSize)
					*processedSize += rem;
				return S_OK;
			}
			bool useBuf = false;
			if((VirtPos & mask) != 0 || ((ptrdiff_t)data & mask) != 0)
				useBuf = true;
			else {
				uint64 end = VirtPos + size;
				if((end & mask) != 0) {
					end &= mask2;
					if(end <= VirtPos)
						useBuf = true;
					else
						size = (uint32)(end - VirtPos);
				}
			}
			if(!useBuf)
				break;
			if(alignedPos != PhyPos) {
				uint64 realNewPosition;
				bool result = File.Seek(alignedPos, FILE_BEGIN, realNewPosition);
				if(!result)
					return ConvertBoolToHRESULT(result);
				PhyPos = realNewPosition;
			}
			BufStartPos = alignedPos;
			uint32 readSize = kClusterSize;
			if(File.SizeDefined)
				readSize = (uint32)MyMin(File.Size - PhyPos, (uint64)kClusterSize);
			if(!Buf) {
				Buf = (Byte *)MidAlloc(kClusterSize);
				if(!Buf)
					return E_OUTOFMEMORY;
			}
			bool result = File.Read1(Buf, readSize, BufSize);
			if(!result)
				return ConvertBoolToHRESULT(result);
			if(BufSize == 0)
				return S_OK;
			PhyPos += BufSize;
		}

		if(VirtPos != PhyPos) {
			uint64 realNewPosition;
			bool result = File.Seek(VirtPos, FILE_BEGIN, realNewPosition);
			if(!result)
				return ConvertBoolToHRESULT(result);
			PhyPos = VirtPos = realNewPosition;
		}
	}
  #endif

	uint32 realProcessedSize;
	bool result = File.ReadPart(data, size, realProcessedSize);
	ASSIGN_PTR(processedSize, realProcessedSize);
  #ifdef SUPPORT_DEVICE_FILE
	VirtPos += realProcessedSize;
	PhyPos += realProcessedSize;
  #endif
	if(result)
		return S_OK;
	{
		DWORD error = ::GetLastError();
		if(Callback)
			return Callback->InFileStream_On_Error(CallbackRef, error);
		else if(error == 0)
			return E_FAIL;
		else
			return HRESULT_FROM_WIN32(error);
	}
  #else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res = File.Read(data, (size_t)size);
	if(res == -1) {
		return Callback ? Callback->InFileStream_On_Error(CallbackRef, E_FAIL) : E_FAIL;
	}
	else {
		ASSIGN_PTR(processedSize, (uint32)res);
		return S_OK;
	}
  #endif
}

#ifdef UNDER_CE
	STDMETHODIMP CStdInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
	{
		size_t s2 = fread(data, 1, size, stdin);
		int error = ferror(stdin);
		ASSIGN_PTR(processedSize, s2);
		return (s2 <= size && error == 0) ? S_OK : E_FAIL;
	}
	#else
STDMETHODIMP CStdInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
{
  #ifdef _WIN32
	DWORD realProcessedSize;
	uint32 sizeTemp = (1 << 20);
	SETMIN(sizeTemp, size);
	BOOL res = ::ReadFile(GetStdHandle(STD_INPUT_HANDLE), data, sizeTemp, &realProcessedSize, NULL);
	ASSIGN_PTR(processedSize, realProcessedSize);
	return (res == FALSE && GetLastError() == ERROR_BROKEN_PIPE) ? S_OK : ConvertBoolToHRESULT(res != FALSE);
  #else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res;
	do {
		res = read(0, data, (size_t)size);
	} while(res < 0 && (errno == EINTR));
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(processedSize, (uint32)res);
	return S_OK;
  #endif
}

#endif

STDMETHODIMP CInFileStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin >= 3)
		return STG_E_INVALIDFUNCTION;
  #ifdef USE_WIN_FILE
  #ifdef SUPPORT_DEVICE_FILE
	if(File.IsDeviceFile && (File.SizeDefined || seekOrigin != STREAM_SEEK_END)) {
		switch(seekOrigin) {
			case STREAM_SEEK_SET: break;
			case STREAM_SEEK_CUR: offset += VirtPos; break;
			case STREAM_SEEK_END: offset += File.Size; break;
			default: return STG_E_INVALIDFUNCTION;
		}
		if(offset < 0)
			return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
		VirtPos = offset;
		ASSIGN_PTR(newPosition, offset);
		return S_OK;
	}
  #endif
	uint64 realNewPosition;
	bool result = File.Seek(offset, seekOrigin, realNewPosition);
  #ifdef SUPPORT_DEVICE_FILE
	PhyPos = VirtPos = realNewPosition;
  #endif
	ASSIGN_PTR(newPosition, realNewPosition);
	return ConvertBoolToHRESULT(result);
  #else
	off_t res = File.Seek((off_t)offset, seekOrigin);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(newPosition, (uint64)res);
	return S_OK;
  #endif
}

STDMETHODIMP CInFileStream::GetSize(uint64 * size)
{
	return ConvertBoolToHRESULT(File.GetLength(*size));
}

#ifdef USE_WIN_FILE
	STDMETHODIMP CInFileStream::GetProps(uint64 * size, FILETIME * cTime, FILETIME * aTime, FILETIME * mTime, uint32 * attrib)
	{
		BY_HANDLE_FILE_INFORMATION info;
		if(File.GetFileInformation(&info)) {
			if(size) *size = (((uint64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
			if(cTime) *cTime = info.ftCreationTime;
			if(aTime) *aTime = info.ftLastAccessTime;
			if(mTime) *mTime = info.ftLastWriteTime;
			if(attrib) *attrib = info.dwFileAttributes;
			return S_OK;
		}
		return GetLastError();
	}

	STDMETHODIMP CInFileStream::GetProps2(CStreamFileProps * props)
	{
		BY_HANDLE_FILE_INFORMATION info;
		if(File.GetFileInformation(&info)) {
			props->Size = (((uint64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
			props->VolID = info.dwVolumeSerialNumber;
			props->FileID_Low = (((uint64)info.nFileIndexHigh) << 32) + info.nFileIndexLow;
			props->FileID_High = 0;
			props->NumLinks = SupportHardLinks ? info.nNumberOfLinks : 1;
			props->Attrib = info.dwFileAttributes;
			props->CTime = info.ftCreationTime;
			props->ATime = info.ftLastAccessTime;
			props->MTime = info.ftLastWriteTime;
			return S_OK;
		}
		return GetLastError();
	}
#endif
//
// COutFileStream
//
//virtual 
COutFileStream::~COutFileStream() 
{
}

bool COutFileStream::Create(CFSTR fileName, bool createAlways)
{
	ProcessedSize = 0;
	return File.Create(fileName, createAlways);
}

bool COutFileStream::Open(CFSTR fileName, DWORD creationDisposition)
{
	ProcessedSize = 0;
	return File.Open(fileName, creationDisposition);
}

HRESULT COutFileStream::Close()
{
	return ConvertBoolToHRESULT(File.Close());
}

STDMETHODIMP COutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
#ifdef USE_WIN_FILE
	uint32 realProcessedSize;
	bool result = File.Write(data, size, realProcessedSize);
	ProcessedSize += realProcessedSize;
	ASSIGN_PTR(processedSize, realProcessedSize);
	return ConvertBoolToHRESULT(result);
#else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res = File.Write(data, (size_t)size);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(processedSize, (uint32)res);
	ProcessedSize += res;
	return S_OK;
#endif
}

STDMETHODIMP COutFileStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin >= 3)
		return STG_E_INVALIDFUNCTION;
  #ifdef USE_WIN_FILE
	uint64 realNewPosition;
	bool result = File.Seek(offset, seekOrigin, realNewPosition);
	ASSIGN_PTR(newPosition, realNewPosition);
	return ConvertBoolToHRESULT(result);
  #else
	off_t res = File.Seek((off_t)offset, seekOrigin);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(newPosition, (uint64)res);
	return S_OK;
  #endif
}

STDMETHODIMP COutFileStream::SetSize(uint64 newSize)
{
  #ifdef USE_WIN_FILE
	uint64 currentPos;
	if(!File.Seek(0, FILE_CURRENT, currentPos))
		return E_FAIL;
	bool result = File.SetLength(newSize);
	uint64 currentPos2;
	result = result && File.Seek(currentPos, currentPos2);
	return result ? S_OK : E_FAIL;
  #else
	return E_FAIL;
  #endif
}

HRESULT COutFileStream::GetSize(uint64 * size)
{
	return ConvertBoolToHRESULT(File.GetLength(*size));
}

#ifdef UNDER_CE
STDMETHODIMP CStdOutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	size_t s2 = fwrite(data, 1, size, stdout);
	ASSIGN_PTR(processedSize, s2);
	return (s2 == size) ? S_OK : E_FAIL;
}
#else
	STDMETHODIMP CStdOutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
	{
		ASSIGN_PTR(processedSize, 0);
	  #ifdef _WIN32
		uint32 realProcessedSize;
		BOOL res = TRUE;
		if(size > 0) {
			// Seems that Windows doesn't like big amounts writing to stdout.
			// So we limit portions by 32KB.
			uint32 sizeTemp = (1 << 15);
			SETMIN(sizeTemp, size);
			res = ::WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), data, sizeTemp, (DWORD*)&realProcessedSize, NULL);
			_size += realProcessedSize;
			size -= realProcessedSize;
			data = (const void*)((const Byte*)data + realProcessedSize);
			if(processedSize)
				*processedSize += realProcessedSize;
		}
		return ConvertBoolToHRESULT(res != FALSE);
	  #else
		ssize_t res;
		do {
			res = write(1, data, (size_t)size);
		} while(res < 0 && (errno == EINTR));
		if(res == -1)
			return E_FAIL;
		_size += (size_t)res;
		ASSIGN_PTR(processedSize, (uint32)res);
		return S_OK;
	  #endif
	}
#endif
//
// MultiStream.cpp
CMultiStream::CSubStreamInfo::CSubStreamInfo() : Size(0), GlobalOffset(0), LocalPos(0) 
{
}

HRESULT CMultiStream::Init()
{
	uint64 total = 0;
	FOR_VECTOR(i, Streams) {
		CSubStreamInfo &s = Streams[i];
		s.GlobalOffset = total;
		total += Streams[i].Size;
		RINOK(s.Stream->Seek(0, STREAM_SEEK_CUR, &s.LocalPos));
	}
	_totalLength = total;
	_pos = 0;
	_streamIndex = 0;
	return S_OK;
}

STDMETHODIMP CMultiStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(_pos >= _totalLength)
		return S_OK;
	{
		uint   mid = _streamIndex;
		for(uint left = 0, right = Streams.Size();; ) {
			CSubStreamInfo &m = Streams[mid];
			if(_pos < m.GlobalOffset)
				right = mid;
			else if(_pos >= m.GlobalOffset + m.Size)
				left = mid + 1;
			else {
				_streamIndex = mid;
				break;
			}
			mid = (left + right) / 2;
		}
		_streamIndex = mid;
	}
	CSubStreamInfo &s = Streams[_streamIndex];
	uint64 localPos = _pos - s.GlobalOffset;
	if(localPos != s.LocalPos) {
		RINOK(s.Stream->Seek(localPos, STREAM_SEEK_SET, &s.LocalPos));
	}
	uint64 rem = s.Size - localPos;
	SETMIN(size, (uint32)rem);
	HRESULT result = s.Stream->Read(data, size, &size);
	_pos += size;
	s.LocalPos += size;
	ASSIGN_PTR(processedSize, size);
	return result;
}

STDMETHODIMP CMultiStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _pos; break;
		case STREAM_SEEK_END: offset += _totalLength; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	_pos = offset;
	ASSIGN_PTR(newPosition, offset);
	return S_OK;
}
/*
   class COutVolumeStream: public ISequentialOutStream, public CMyUnknownImp {
   unsigned _volIndex;
   uint64 _volSize;
   uint64 _curPos;
   CMyComPtr<ISequentialOutStream> _volumeStream;
   COutArchive _archive;
   CCRC _crc;

   public:
   MY_UNKNOWN_IMP

   CFileItem _file;
   CUpdateOptions _options;
   CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
   void Init(IArchiveUpdateCallback2 *volumeCallback, const UString &name)
   {
    _file.Name = name;
    _file.IsStartPosDefined = true;
    _file.StartPos = 0;

    VolumeCallback = volumeCallback;
    _volIndex = 0;
    _volSize = 0;
   }

   HRESULT Flush();
   STDMETHOD(Write)(const void *data, uint32 size, uint32 *processedSize);
   };

   HRESULT COutVolumeStream::Flush()
   {
   if(_volumeStream) {
    _file.UnPackSize = _curPos;
    _file.FileCRC = _crc.GetDigest();
    RINOK(WriteVolumeHeader(_archive, _file, _options));
    _archive.Close();
    _volumeStream.Release();
    _file.StartPos += _file.UnPackSize;
   }
   return S_OK;
   }
 */

/*
   STDMETHODIMP COutMultiStream::Write(const void *data, uint32 size, uint32 *processedSize)
   {
   if(processedSize)
   *processedSize = 0;
   while (size > 0) {
    if(_streamIndex >= Streams.Size()) {
      CSubStreamInfo subStream;
      RINOK(VolumeCallback->GetVolumeSize(Streams.Size(), &subStream.Size));
      RINOK(VolumeCallback->GetVolumeStream(Streams.Size(), &subStream.Stream));
      subStream.Pos = 0;
      Streams.Add(subStream);
      continue;
    }
    CSubStreamInfo &subStream = Streams[_streamIndex];
    if(_offsetPos >= subStream.Size) {
      _offsetPos -= subStream.Size;
      _streamIndex++;
      continue;
    }
    if(_offsetPos != subStream.Pos) {
      CMyComPtr<IOutStream> outStream;
      RINOK(subStream.Stream.QueryInterface(IID_IOutStream, &outStream));
      RINOK(outStream->Seek(_offsetPos, STREAM_SEEK_SET, NULL));
      subStream.Pos = _offsetPos;
    }

    uint32 curSize = (uint32)MyMin((uint64)size, subStream.Size - subStream.Pos);
    uint32 realProcessed;
    RINOK(subStream.Stream->Write(data, curSize, &realProcessed));
    data = (void *)((Byte *)data + realProcessed);
    size -= realProcessed;
    subStream.Pos += realProcessed;
    _offsetPos += realProcessed;
    _absPos += realProcessed;
    if(_absPos > _length)
      _length = _absPos;
    if(processedSize)
   *processedSize += realProcessed;
    if(subStream.Pos == subStream.Size) {
      _streamIndex++;
      _offsetPos = 0;
    }
    if(realProcessed != curSize && realProcessed == 0)
      return E_FAIL;
   }
   return S_OK;
   }

   STDMETHODIMP COutMultiStream::Seek(int64 offset, uint32 seekOrigin, uint64 *newPosition)
   {
   switch(seekOrigin) {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _absPos; break;
    case STREAM_SEEK_END: offset += _length; break;
    default: return STG_E_INVALIDFUNCTION;
   }
   if(offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
   _absPos = offset;
   _offsetPos = _absPos;
   _streamIndex = 0;
   if(newPosition)
   *newPosition = offset;
   return S_OK;
   }
 */
//
