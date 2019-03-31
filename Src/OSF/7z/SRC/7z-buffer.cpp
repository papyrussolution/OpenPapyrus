// 7Z-BUFFER.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
using namespace NFile;
using namespace NDir;

// OutBuffer.cpp
COutBuffer::COutBuffer() : _buf(0), _pos(0), _stream(0), _buf2(0) 
{
}

COutBuffer::~COutBuffer() 
{
	Free();
}

bool COutBuffer::Create(uint32 bufSize) throw()
{
	const uint32 kMinBlockSize = 1;
	SETMAX(bufSize, kMinBlockSize);
	if(_buf != 0 && _bufSize == bufSize)
		return true;
	else {
		Free();
		_bufSize = bufSize;
		_buf = static_cast<Byte *>(::MidAlloc(bufSize));
		return (_buf != 0);
	}
}

void COutBuffer::Free() throw()
{
	::MidFree(_buf);
	_buf = 0;
}

void COutBuffer::Init() throw()
{
	_streamPos = 0;
	_limitPos = _bufSize;
	_pos = 0;
	_processedSize = 0;
	_overDict = false;
  #ifdef _NO_EXCEPTIONS
	ErrorCode = S_OK;
  #endif
}

void FASTCALL COutBuffer::WriteByte(Byte b)
{
	uint32 pos = _pos;
	_buf[pos] = b;
	pos++;
	_pos = pos;
	if(pos == _limitPos)
		FlushWithCheck();
}

void FASTCALL COutBuffer::WriteBytes(const void * data, size_t size)
{
	for(size_t i = 0; i < size; i++)
		WriteByte(((const Byte*)data)[i]);
}

uint64 COutBuffer::GetProcessedSize() const throw()
{
	uint64 res = _processedSize + _pos - _streamPos;
	if(_streamPos > _pos)
		res += _bufSize;
	return res;
}

HRESULT COutBuffer::FlushPart() throw()
{
	// _streamPos < _bufSize
	uint32 size = (_streamPos >= _pos) ? (_bufSize - _streamPos) : (_pos - _streamPos);
	HRESULT result = S_OK;
  #ifdef _NO_EXCEPTIONS
	result = ErrorCode;
  #endif
	if(_buf2 != 0) {
		memcpy(_buf2, _buf + _streamPos, size);
		_buf2 += size;
	}
	if(_stream != 0
      #ifdef _NO_EXCEPTIONS
	    && (ErrorCode == S_OK)
      #endif
	    ) {
		uint32 processedSize = 0;
		result = _stream->Write(_buf + _streamPos, size, &processedSize);
		size = processedSize;
	}
	_streamPos += size;
	if(_streamPos == _bufSize)
		_streamPos = 0;
	if(_pos == _bufSize) {
		_overDict = true;
		_pos = 0;
	}
	_limitPos = (_streamPos > _pos) ? _streamPos : _bufSize;
	_processedSize += size;
	return result;
}

HRESULT COutBuffer::Flush() throw()
{
  #ifdef _NO_EXCEPTIONS
	if(ErrorCode != S_OK)
		return ErrorCode;
  #endif
	while(_streamPos != _pos) {
		HRESULT result = FlushPart();
		if(result != S_OK)
			return result;
	}
	return S_OK;
}

void COutBuffer::FlushWithCheck()
{
	HRESULT result = Flush();
  #ifdef _NO_EXCEPTIONS
	ErrorCode = result;
  #else
	if(result != S_OK)
		throw COutBufferException(result);
  #endif
}
//
// InBuffer.cpp
CInBufferBase::CInBufferBase() throw() : _buf(0), _bufLim(0), _bufBase(0), _stream(0), _processedSize(0), _bufSize(0), _wasFinished(false), NumExtraBytes(0)
{
}

void CInBufferBase::SetBuf(Byte * buf, size_t bufSize, size_t end, size_t pos)
{
	_bufBase = buf;
	_bufSize = bufSize;
	_processedSize = 0;
	_buf = buf + pos;
	_bufLim = buf + end;
	_wasFinished = false;
#ifdef _NO_EXCEPTIONS
	ErrorCode = S_OK;
#endif
	NumExtraBytes = 0;
}

CInBuffer::~CInBuffer() 
{
	Free();
}

bool CInBuffer::Create(size_t bufSize) throw()
{
	const uint kMinBlockSize = 1;
	SETMAX(bufSize, kMinBlockSize);
	if(_bufBase != 0 && _bufSize == bufSize)
		return true;
	else {
		Free();
		_bufSize = bufSize;
		_bufBase = static_cast<Byte *>(::MidAlloc(bufSize));
		return (_bufBase != 0);
	}
}

void CInBuffer::Free() throw()
{
	::MidFree(_bufBase);
	_bufBase = 0;
}

void CInBufferBase::Init() throw()
{
	_processedSize = 0;
	_buf = _bufBase;
	_bufLim = _buf;
	_wasFinished = false;
  #ifdef _NO_EXCEPTIONS
	ErrorCode = S_OK;
  #endif
	NumExtraBytes = 0;
}

bool CInBufferBase::ReadBlock()
{
  #ifdef _NO_EXCEPTIONS
	if(ErrorCode != S_OK)
		return false;
  #endif
	if(_wasFinished)
		return false;
	_processedSize += (_buf - _bufBase);
	_buf = _bufBase;
	_bufLim = _bufBase;
	uint32 processed;
	// FIX_ME: we can improve it to support (_bufSize >= (1 << 32))
	HRESULT result = _stream->Read(_bufBase, (uint32)_bufSize, &processed);
  #ifdef _NO_EXCEPTIONS
	ErrorCode = result;
  #else
	if(result != S_OK)
		throw CInBufferException(result);
  #endif
	_bufLim = _buf + processed;
	_wasFinished = (processed == 0);
	return !_wasFinished;
}

bool FASTCALL CInBufferBase::ReadByte_FromNewBlock(Byte & b)
{
	if(!ReadBlock()) {
		NumExtraBytes++;
		b = 0xFF;
		return false;
	}
	b = *_buf++;
	return true;
}

Byte CInBufferBase::ReadByte_FromNewBlock()
{
	if(!ReadBlock()) {
		NumExtraBytes++;
		return 0xFF;
	}
	else
		return *_buf++;
}

size_t FASTCALL CInBufferBase::ReadBytes(Byte * buf, size_t size)
{
	if((size_t)(_bufLim - _buf) >= size) {
		const Byte * src = _buf;
		for(size_t i = 0; i < size; i++)
			buf[i] = src[i];
		_buf += size;
		return size;
	}
	for(size_t i = 0; i < size; i++) {
		if(_buf >= _bufLim)
			if(!ReadBlock())
				return i;
		buf[i] = *_buf++;
	}
	return size;
}

size_t CInBufferBase::Skip(size_t size)
{
	size_t processed = 0;
	for(;; ) {
		size_t rem = (_bufLim - _buf);
		if(rem >= size) {
			_buf += size;
			return processed + size;
		}
		_buf += rem;
		processed += rem;
		size -= rem;
		if(!ReadBlock())
			return processed;
	}
}
//
// InOutTempBuffer.cpp
static const size_t kTempBufSize = (1 << 20);

#define kTempFilePrefixString FTEXT("7zt")

CInOutTempBuffer::CInOutTempBuffer() : _buf(NULL) 
{
}

void CInOutTempBuffer::Create()
{
	SETIFZ(_buf, new Byte[kTempBufSize]);
}

CInOutTempBuffer::~CInOutTempBuffer()
{
	delete []_buf;
}

void CInOutTempBuffer::InitWriting()
{
	_bufPos = 0;
	_tempFileCreated = false;
	_size = 0;
	_crc = CRC_INIT_VAL;
}

bool CInOutTempBuffer::WriteToFile(const void * data, uint32 size)
{
	if(size == 0)
		return true;
	if(!_tempFileCreated) {
		if(!_tempFile.CreateRandomInTempFolder(kTempFilePrefixString, &_outFile))
			return false;
		_tempFileCreated = true;
	}
	uint32 processed;
	if(!_outFile.Write(data, size, processed))
		return false;
	_crc = CrcUpdate(_crc, data, processed);
	_size += processed;
	return (processed == size);
}

bool CInOutTempBuffer::Write(const void * data, uint32 size)
{
	if(size == 0)
		return true;
	size_t cur = kTempBufSize - _bufPos;
	if(cur != 0) {
		if(cur > size)
			cur = size;
		memcpy(_buf + _bufPos, data, cur);
		_crc = CrcUpdate(_crc, data, cur);
		_bufPos += cur;
		_size += cur;
		size -= (uint32)cur;
		data = ((const Byte*)data) + cur;
	}
	return WriteToFile(data, size);
}

HRESULT CInOutTempBuffer::WriteToStream(ISequentialOutStream * stream)
{
	if(!_outFile.Close())
		return E_FAIL;

	uint64 size = 0;
	uint32 crc = CRC_INIT_VAL;

	if(_bufPos != 0) {
		RINOK(WriteStream(stream, _buf, _bufPos));
		crc = CrcUpdate(crc, _buf, _bufPos);
		size += _bufPos;
	}

	if(_tempFileCreated) {
		NIO::CInFile inFile;
		if(!inFile.Open(_tempFile.GetPath()))
			return E_FAIL;
		while(size < _size) {
			uint32 processed;
			if(!inFile.ReadPart(_buf, kTempBufSize, processed))
				return E_FAIL;
			if(processed == 0)
				break;
			RINOK(WriteStream(stream, _buf, processed));
			crc = CrcUpdate(crc, _buf, processed);
			size += processed;
		}
	}

	return (_crc == crc && size == _size) ? S_OK : E_FAIL;
}

/*
   STDMETHODIMP CSequentialOutTempBufferImp::Write(const void *data, uint32 size, uint32 *processed)
   {
   if(!_buf->Write(data, size))
   {
    if(processed)
   *processed = 0;
    return E_FAIL;
   }
   if(processed)
   *processed = size;
   return S_OK;
   }
 */
//
