// FilterCoder.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
/*
   AES filters need 16-bytes alignment for HARDWARE-AES instructions.
   So we call IFilter::Filter(, size), where (size != 16 * N) only for last data block.

   AES-CBC filters need data size aligned for 16-bytes.
   So the encoder can add zeros to the end of original stream.

   Some filters (BCJ and others) don't process data at the end of stream in some cases.
   So the encoder and decoder write such last bytes without change.
 */

static const uint32 kBufSize = 1 << 20;

void CFilterCoder::InitSpecVars()
{
	_bufPos = 0;
	_convPos = 0;
	_convSize = 0;
	_outSizeIsDefined = false;
	_outSize = 0;
	_nowPos64 = 0;
}

STDMETHODIMP CFilterCoder::SetInBufSize(uint32, uint32 size) { _inBufSize = size; return S_OK; }
STDMETHODIMP CFilterCoder::SetOutBufSize(uint32, uint32 size) { _outBufSize = size; return S_OK; }

HRESULT CFilterCoder::Alloc()
{
	uint32 size = smin(_inBufSize, _outBufSize);
	/* minimal bufSize is 16 bytes for AES and IA64 filter.
	   bufSize for AES must be aligned for 16 bytes.
	   We use (1 << 12) min size to support future aligned filters. */
	const uint32 kMinSize = 1 << 12;
	size &= ~(uint32)(kMinSize - 1);
	SETMAX(size, kMinSize);
	if(!_buf || _bufSize != size) {
		AllocAlignedMask(size, 16 - 1);
		if(!_buf)
			return E_OUTOFMEMORY;
		_bufSize = size;
	}
	return S_OK;
}

HRESULT CFilterCoder::Init_and_Alloc()
{
	RINOK(Filter->Init());
	return Alloc();
}

CFilterCoder::CFilterCoder(bool encodeMode) : _bufSize(0), _inBufSize(kBufSize), _outBufSize(kBufSize),
	_encodeMode(encodeMode), _outSizeIsDefined(false), _outSize(0), _nowPos64(0)
{
}

CFilterCoder::~CFilterCoder()
{
}

STDMETHODIMP CFilterCoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
    const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
{
	RINOK(Init_and_Alloc());
	uint64 nowPos64 = 0;
	bool inputFinished = false;
	uint32 pos = 0;
	while(!outSize || nowPos64 < *outSize) {
		uint32 endPos = pos;
		if(!inputFinished) {
			size_t processedSize = _bufSize - pos;
			RINOK(ReadStream(inStream, _buf + pos, &processedSize));
			endPos = pos + (uint32)processedSize;
			inputFinished = (endPos != _bufSize);
		}
		pos = Filter->Filter(_buf, endPos);
		if(pos > endPos) {
			// AES
			if(!inputFinished || pos > _bufSize)
				return E_FAIL;
			if(!_encodeMode)
				return S_FALSE;
			do {
				_buf[endPos] = 0;
			} while(++endPos != pos);
			if(pos != Filter->Filter(_buf, pos))
				return E_FAIL;
		}
		if(endPos == 0)
			return S_OK;
		uint32 size = (pos != 0 ? pos : endPos);
		if(outSize) {
			uint64 remSize = *outSize - nowPos64;
			if(size > remSize)
				size = (uint32)remSize;
		}
		RINOK(WriteStream(outStream, _buf, size));
		nowPos64 += size;
		if(pos == 0)
			return S_OK;
		if(progress)
			RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64));
		uint32 i = 0;
		while(pos < endPos)
			_buf[i++] = _buf[pos++];
		pos = i;
	}
	return S_OK;
}

// ---------- Write to Filter ----------

STDMETHODIMP CFilterCoder::SetOutStream(ISequentialOutStream * outStream)
{
	_outStream = outStream;
	return S_OK;
}

STDMETHODIMP CFilterCoder::ReleaseOutStream()
{
	_outStream.Release();
	return S_OK;
}

HRESULT CFilterCoder::Flush2()
{
	while(_convSize != 0) {
		uint32 num = _convSize;
		if(_outSizeIsDefined) {
			uint64 rem = _outSize - _nowPos64;
			if(num > rem)
				num = (uint32)rem;
			if(num == 0)
				return k_My_HRESULT_WritingWasCut;
		}
		uint32 processed = 0;
		HRESULT res = _outStream->Write(_buf + _convPos, num, &processed);
		if(processed == 0)
			return res != S_OK ? res : E_FAIL;
		_convPos += processed;
		_convSize -= processed;
		_nowPos64 += processed;
		RINOK(res);
	}
	if(_convPos != 0) {
		uint32 num = _bufPos - _convPos;
		for(uint32 i = 0; i < num; i++)
			_buf[i] = _buf[_convPos + i];
		_bufPos = num;
		_convPos = 0;
	}
	return S_OK;
}

STDMETHODIMP CFilterCoder::Write(const void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	while(size != 0) {
		RINOK(Flush2());
		// _convSize is 0
		// _convPos is 0
		// _bufPos is small
		if(_bufPos != _bufSize) {
			uint32 num = smin(size, _bufSize - _bufPos);
			memcpy(_buf + _bufPos, data, num);
			size -= num;
			data = (const Byte *)data + num;
			if(processedSize)
				*processedSize += num;
			_bufPos += num;
			if(_bufPos != _bufSize)
				continue;
		}
		// _bufPos == _bufSize
		_convSize = Filter->Filter(_buf, _bufPos);
		if(_convSize == 0)
			break;
		if(_convSize > _bufPos) {
			// that case is not possible.
			_convSize = 0;
			return E_FAIL;
		}
	}
	return S_OK;
}

STDMETHODIMP CFilterCoder::OutStreamFinish()
{
	for(;;) {
		RINOK(Flush2());
		if(_bufPos == 0)
			break;
		_convSize = Filter->Filter(_buf, _bufPos);
		if(_convSize == 0)
			_convSize = _bufPos;
		else if(_convSize > _bufPos) {
			// AES
			if(_convSize > _bufSize) {
				_convSize = 0;
				return E_FAIL;
			}
			if(!_encodeMode) {
				_convSize = 0;
				return S_FALSE;
			}
			for(; _bufPos < _convSize; _bufPos++)
				_buf[_bufPos] = 0;
			_convSize = Filter->Filter(_buf, _bufPos);
			if(_convSize != _bufPos)
				return E_FAIL;
		}
	}

	CMyComPtr<IOutStreamFinish> finish;
	_outStream.QueryInterface(IID_IOutStreamFinish, &finish);
	if(finish)
		return finish->OutStreamFinish();
	return S_OK;
}

// ---------- Init functions ----------

STDMETHODIMP CFilterCoder::InitEncoder()
{
	InitSpecVars();
	return Init_and_Alloc();
}

HRESULT CFilterCoder::Init_NoSubFilterInit()
{
	InitSpecVars();
	return Alloc();
}

STDMETHODIMP CFilterCoder::SetOutStreamSize(const uint64 * outSize)
{
	InitSpecVars();
	if(outSize) {
		_outSize = *outSize;
		_outSizeIsDefined = true;
	}
	return Init_and_Alloc();
}

// ---------- Read from Filter ----------

STDMETHODIMP CFilterCoder::SetInStream(ISequentialInStream * inStream)
{
	_inStream = inStream;
	return S_OK;
}

STDMETHODIMP CFilterCoder::ReleaseInStream()
{
	_inStream.Release();
	return S_OK;
}

STDMETHODIMP CFilterCoder::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	while(size != 0) {
		if(_convSize != 0) {
			if(size > _convSize)
				size = _convSize;
			if(_outSizeIsDefined) {
				uint64 rem = _outSize - _nowPos64;
				if(size > rem)
					size = (uint32)rem;
			}
			memcpy(data, _buf + _convPos, size);
			_convPos += size;
			_convSize -= size;
			_nowPos64 += size;
			ASSIGN_PTR(processedSize, size);
			break;
		}
		if(_convPos != 0) {
			uint32 num = _bufPos - _convPos;
			for(uint32 i = 0; i < num; i++)
				_buf[i] = _buf[_convPos + i];
			_bufPos = num;
			_convPos = 0;
		}
		{
			size_t readSize = _bufSize - _bufPos;
			HRESULT res = ReadStream(_inStream, _buf + _bufPos, &readSize);
			_bufPos += (uint32)readSize;
			RINOK(res);
		}
		_convSize = Filter->Filter(_buf, _bufPos);
		if(_convSize == 0) {
			if(_bufPos == 0)
				break;
			// BCJ
			_convSize = _bufPos;
			continue;
		}
		if(_convSize > _bufPos) {
			// AES
			if(_convSize > _bufSize)
				return E_FAIL;
			if(!_encodeMode)
				return S_FALSE;
			do {
				_buf[_bufPos] = 0;
			} while(++_bufPos != _convSize);
			_convSize = Filter->Filter(_buf, _convSize);
			if(_convSize != _bufPos)
				return E_FAIL;
		}
	}

	return S_OK;
}
#ifndef _NO_CRYPTO
	STDMETHODIMP CFilterCoder::CryptoSetPassword(const Byte * data, uint32 size) { return _SetPassword->CryptoSetPassword(data, size); }
	STDMETHODIMP CFilterCoder::SetKey(const Byte * data, uint32 size) { return _CryptoProperties->SetKey(data, size); }
	STDMETHODIMP CFilterCoder::SetInitVector(const Byte * data, uint32 size) { return _CryptoProperties->SetInitVector(data, size); }
#endif
#ifndef EXTRACT_ONLY
	STDMETHODIMP CFilterCoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * properties, uint32 numProperties)
		{ return _SetCoderProperties->SetCoderProperties(propIDs, properties, numProperties); }
	STDMETHODIMP CFilterCoder::WriteCoderProperties(ISequentialOutStream * outStream) { return _WriteCoderProperties->WriteCoderProperties(outStream); }
	/*
	   STDMETHODIMP CFilterCoder::ResetSalt()
	   { return _CryptoResetSalt->ResetSalt(); }
	 */
	STDMETHODIMP CFilterCoder::ResetInitVector() { return _CryptoResetInitVector->ResetInitVector(); }
#endif
STDMETHODIMP CFilterCoder::SetDecoderProperties2(const Byte * data, uint32 size) { return _SetDecoderProperties2->SetDecoderProperties2(data, size); }

