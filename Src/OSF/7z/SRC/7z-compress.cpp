// 7Z-COMPRESS.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
// CopyCoder.cpp ByteSwap.cpp DeltaFilter.cpp BcjCoder.cpp Bcj2Coder.cpp
// CopyRegister.cpp RarCodecsRegister.cpp BcjRegister.cpp Bcj2Register.cpp LzmaRegister.cpp Lzma2Register.cpp PpmdRegister.cpp BZip2Register.cpp DeflateRegister.cpp BranchRegister.cpp
namespace NCompress {
	static const uint32 kBufSize = 1 << 17;

	CCopyCoder::CCopyCoder() : _buf(0), TotalSize(0) 
	{
	}
	CCopyCoder::~CCopyCoder()
	{
		::MidFree(_buf);
	}
	STDMETHODIMP CCopyCoder::SetFinishMode(uint32 /* finishMode */)
	{
		return S_OK;
	}
	STDMETHODIMP CCopyCoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
		const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
	{
		if(!_buf) {
			_buf = (Byte*)::MidAlloc(kBufSize);
			if(!_buf)
				return E_OUTOFMEMORY;
		}
		TotalSize = 0;
		for(;; ) {
			uint32 size = kBufSize;
			if(outSize && size > *outSize - TotalSize)
				size = (uint32)(*outSize - TotalSize);
			if(size == 0)
				return S_OK;
			HRESULT readRes = inStream->Read(_buf, size, &size);
			if(size == 0)
				return readRes;
			if(outStream) {
				uint32 pos = 0;
				do {
					uint32 curSize = size - pos;
					HRESULT res = outStream->Write(_buf + pos, curSize, &curSize);
					pos += curSize;
					TotalSize += curSize;
					RINOK(res);
					if(curSize == 0)
						return E_FAIL;
				} while(pos < size);
			}
			else
				TotalSize += size;
			RINOK(readRes);
			if(progress) {
				RINOK(progress->SetRatioInfo(&TotalSize, &TotalSize));
			}
		}
	}
	STDMETHODIMP CCopyCoder::SetInStream(ISequentialInStream * inStream)
	{
		_inStream = inStream;
		TotalSize = 0;
		return S_OK;
	}
	STDMETHODIMP CCopyCoder::ReleaseInStream()
	{
		_inStream.Release();
		return S_OK;
	}
	STDMETHODIMP CCopyCoder::Read(void * data, uint32 size, uint32 * processedSize)
	{
		uint32 realProcessedSize = 0;
		HRESULT res = _inStream->Read(data, size, &realProcessedSize);
		TotalSize += realProcessedSize;
		ASSIGN_PTR(processedSize, realProcessedSize);
		return res;
	}
	STDMETHODIMP CCopyCoder::GetInStreamProcessedSize(uint64 * value)
	{
		*value = TotalSize;
		return S_OK;
	}
	HRESULT CopyStream(ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress)
	{
		CMyComPtr<ICompressCoder> copyCoder = new CCopyCoder;
		return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
	}
	HRESULT CopyStream_ExactSize(ISequentialInStream * inStream, ISequentialOutStream * outStream, uint64 size, ICompressProgressInfo * progress)
	{
		NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
		CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
		RINOK(copyCoder->Code(inStream, outStream, NULL, &size, progress));
		return copyCoderSpec->TotalSize == size ? S_OK : E_FAIL;
	}
	//
	namespace NByteSwap {
		class CByteSwap2 : public ICompressFilter, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP1(ICompressFilter);
			INTERFACE_ICompressFilter(; )
		};
		class CByteSwap4 : public ICompressFilter, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP1(ICompressFilter);
			INTERFACE_ICompressFilter(; )
		};
		STDMETHODIMP CByteSwap2::Init() 
		{
			return S_OK;
		}
		STDMETHODIMP_(uint32) CByteSwap2::Filter(Byte *data, uint32 size)
		{
			const uint32 kStep = 2;
			if(size < kStep)
				return 0;
			size &= ~(kStep - 1);
			const Byte * end = data + (size_t)size;
			do {
				Byte b0 = data[0];
				data[0] = data[1];
				data[1] = b0;
				data += kStep;
			} while(data != end);
			return size;
		}
		STDMETHODIMP CByteSwap4::Init() 
		{
			return S_OK;
		}
		STDMETHODIMP_(uint32) CByteSwap4::Filter(Byte *data, uint32 size)
		{
			const uint32 kStep = 4;
			if(size < kStep)
				return 0;
			size &= ~(kStep - 1);
			const Byte * end = data + (size_t)size;
			do {
				Byte b0 = data[0];
				Byte b1 = data[1];
				data[0] = data[3];
				data[1] = data[2];
				data[2] = b1;
				data[3] = b0;
				data += kStep;
			} while(data != end);
			return size;
		}
		REGISTER_FILTER_CREATE(CreateFilter2, CByteSwap2())
		REGISTER_FILTER_CREATE(CreateFilter4, CByteSwap4())
		REGISTER_CODECS_VAR {
			REGISTER_FILTER_ITEM(CreateFilter2, CreateFilter2, 0x20302, "Swap2"),
			REGISTER_FILTER_ITEM(CreateFilter4, CreateFilter4, 0x20304, "Swap4")
		};
		REGISTER_CODECS(ByteSwap)
	}
	//
	namespace NDelta {
		struct CDelta {
			uint   _delta;
			Byte   _state[DELTA_STATE_SIZE];
			CDelta() : _delta(1) 
			{
			}
			void DeltaInit() 
			{
				Delta_Init(_state);
			}
		};

		#ifndef EXTRACT_ONLY

		class CEncoder : public ICompressFilter, public ICompressSetCoderProperties, public ICompressWriteCoderProperties, CDelta, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP3(ICompressFilter, ICompressSetCoderProperties, ICompressWriteCoderProperties)
			INTERFACE_ICompressFilter(; )
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);
		};

		STDMETHODIMP CEncoder::Init()
		{
			DeltaInit();
			return S_OK;
		}

		STDMETHODIMP_(uint32) CEncoder::Filter(Byte *data, uint32 size)
		{
			Delta_Encode(_state, _delta, data, size);
			return size;
		}

		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps)
		{
			uint32 delta = _delta;
			for(uint32 i = 0; i < numProps; i++) {
				const PROPVARIANT &prop = props[i];
				PROPID propID = propIDs[i];
				if(propID >= NCoderPropID::kReduceSize)
					continue;
				if(prop.vt != VT_UI4)
					return E_INVALIDARG;
				switch(propID) {
					case NCoderPropID::kDefaultProp:
						delta = (uint32)prop.ulVal;
						if(delta < 1 || delta > 256)
							return E_INVALIDARG;
						break;
					case NCoderPropID::kNumThreads: break;
					case NCoderPropID::kLevel: break;
					default: return E_INVALIDARG;
				}
			}
			_delta = delta;
			return S_OK;
		}

		STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
		{
			Byte prop = (Byte)(_delta - 1);
			return outStream->Write(&prop, 1, NULL);
		}

		#endif

		class CDecoder : public ICompressFilter, public ICompressSetDecoderProperties2, CDelta, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP2(ICompressFilter, ICompressSetDecoderProperties2)
			INTERFACE_ICompressFilter(; )
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
		};

		STDMETHODIMP CDecoder::Init()
		{
			DeltaInit();
			return S_OK;
		}

		STDMETHODIMP_(uint32) CDecoder::Filter(Byte *data, uint32 size)
		{
			Delta_Decode(_state, _delta, data, size);
			return size;
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * props, uint32 size)
		{
			if(size != 1)
				return E_INVALIDARG;
			_delta = (uint)props[0] + 1;
			return S_OK;
		}

		REGISTER_FILTER_E(Delta, CDecoder(), CEncoder(), 3, "Delta")
	}
	//
	// CopyRegister
	//
	REGISTER_CODEC_CREATE(CreateCodec, CCopyCoder())
	REGISTER_CODEC_2(Copy, CreateCodec, CreateCodec, 0, "Copy")
	//
	// RarCodecsRegister
	//
	#define CREATE_CODEC(x) REGISTER_CODEC_CREATE(CreateCodec ## x, NRar ## x::CDecoder())

	CREATE_CODEC(1)
	CREATE_CODEC(2)
	CREATE_CODEC(3)
	CREATE_CODEC(5)

	#define RAR_CODEC(x, name) { CreateCodec ## x, NULL, 0x40300 + x, "Rar" name, 1, false }

	REGISTER_CODECS_VAR { RAR_CODEC(1, "1"), RAR_CODEC(2, "2"), RAR_CODEC(3, "3"), RAR_CODEC(5, "5"), };
	REGISTER_CODECS(Rar)
	//
	//
	//
	namespace NLzma  { REGISTER_CODEC_E(LZMA, CDecoder(), CEncoder(), 0x30101, "LZMA") }
	namespace NLzma2 { REGISTER_CODEC_E(LZMA2, CDecoder(), CEncoder(), 0x21, "LZMA2") }
	namespace NPpmd  { REGISTER_CODEC_E(PPMD, CDecoder(), CEncoder(), 0x30401, "PPMD") }
	namespace NBcj {
		STDMETHODIMP CCoder::Init()
		{
			_bufferPos = 0;
			x86_Convert_Init(_prevMask);
			return S_OK;
		}
		STDMETHODIMP_(uint32) CCoder::Filter(Byte *data, uint32 size)
		{
			uint32 processed = (uint32) ::x86_Convert(data, size, _bufferPos, &_prevMask, _encode);
			_bufferPos += processed;
			return processed;
		}

		REGISTER_FILTER_E(BCJ, CCoder(false), CCoder(true), 0x3030103, "BCJ") 
	}
	namespace NBcj2 {
		CBaseCoder::CBaseCoder()
		{
			for(int i = 0; i < BCJ2_NUM_STREAMS + 1; i++) {
				_bufs[i] = NULL;
				_bufsCurSizes[i] = 0;
				_bufsNewSizes[i] = (1 << 18);
			}
		}
		CBaseCoder::~CBaseCoder()
		{
			for(int i = 0; i < BCJ2_NUM_STREAMS + 1; i++)
				::MidFree(_bufs[i]);
		}
		HRESULT CBaseCoder::Alloc(bool allocForOrig)
		{
			uint   num = allocForOrig ? BCJ2_NUM_STREAMS + 1 : BCJ2_NUM_STREAMS;
			for(uint i = 0; i < num; i++) {
				uint32 newSize = _bufsNewSizes[i];
				const uint32 kMinBufSize = 1;
				SETMAX(newSize, kMinBufSize);
				if(!_bufs[i] || newSize != _bufsCurSizes[i]) {
					if(_bufs[i]) {
						::MidFree(_bufs[i]);
						_bufs[i] = 0;
					}
					_bufsCurSizes[i] = 0;
					Byte * buf = (Byte*)::MidAlloc(newSize);
					_bufs[i] = buf;
					if(!buf)
						return E_OUTOFMEMORY;
					_bufsCurSizes[i] = newSize;
				}
			}
			return S_OK;
		}

		#ifndef EXTRACT_ONLY
		CEncoder::CEncoder() : _relatLim(BCJ2_RELAT_LIMIT) 
		{
		}

		CEncoder::~CEncoder() 
		{
		}

		STDMETHODIMP CEncoder::SetInBufSize(uint32, uint32 size) 
		{
			_bufsNewSizes[BCJ2_NUM_STREAMS] = size; return S_OK;
		}

		STDMETHODIMP CEncoder::SetOutBufSize(uint32 streamIndex, uint32 size) 
		{
			_bufsNewSizes[streamIndex] = size; return S_OK;
		}

		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps)
		{
			uint32 relatLim = BCJ2_RELAT_LIMIT;
			for(uint32 i = 0; i < numProps; i++) {
				const PROPVARIANT &prop = props[i];
				PROPID propID = propIDs[i];
				if(propID < NCoderPropID::kReduceSize) {
					switch(propID) {
						/*
						   case NCoderPropID::kDefaultProp:
						   {
						   if(prop.vt != VT_UI4)
							return E_INVALIDARG;
						   uint32 v = prop.ulVal;
						   if(v > 31)
							return E_INVALIDARG;
						   relatLim = (uint32)1 << v;
						   break;
						   }
						 */
						case NCoderPropID::kDictionarySize:
							if(prop.vt != VT_UI4)
								return E_INVALIDARG;
							else {
								relatLim = prop.ulVal;
								if(relatLim > ((uint32)1 << 31))
									return E_INVALIDARG;
							}
							break;
						case NCoderPropID::kNumThreads:
							continue;
						case NCoderPropID::kLevel:
							continue;
						default: return E_INVALIDARG;
					}
				}
			}
			_relatLim = relatLim;
			return S_OK;
		}

		HRESULT CEncoder::CodeReal(ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
			ISequentialOutStream * const * outStreams, const uint64 * const * /* outSizes */, uint32 numOutStreams, ICompressProgressInfo * progress)
		{
			if(numInStreams != 1 || numOutStreams != BCJ2_NUM_STREAMS)
				return E_INVALIDARG;
			RINOK(Alloc());
			uint32 fileSize_for_Conv = 0;
			if(inSizes && inSizes[0]) {
				uint64 inSize = *inSizes[0];
				if(inSize <= BCJ2_FileSize_MAX)
					fileSize_for_Conv = (uint32)inSize;
			}
			CMyComPtr<ICompressGetSubStreamSize> getSubStreamSize;
			inStreams[0]->QueryInterface(IID_ICompressGetSubStreamSize, (void**)&getSubStreamSize);
			CBcj2Enc enc;
			enc.src = _bufs[BCJ2_NUM_STREAMS];
			enc.srcLim = enc.src;
			{
				for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
					enc.bufs[i] = _bufs[i];
					enc.lims[i] = _bufs[i] + _bufsCurSizes[i];
				}
			}
			size_t numBytes_in_ReadBuf = 0;
			uint64 prevProgress = 0;
			uint64 totalStreamRead = 0; // size read from InputStream
			uint64 currentInPos = 0; // data that was processed, it doesn't include data in input buffer and data in enc.temp
			uint64 outSizeRc = 0;
			Bcj2Enc_Init(&enc);
			enc.fileIp = 0;
			enc.fileSize = fileSize_for_Conv;
			enc.relatLimit = _relatLim;
			enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
			bool needSubSize = false;
			uint64 subStreamIndex = 0;
			uint64 subStreamStartPos = 0;
			bool readWasFinished = false;
			for(;; ) {
				if(needSubSize && getSubStreamSize) {
					enc.fileIp = 0;
					enc.fileSize = fileSize_for_Conv;
					enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
					for(;; ) {
						uint64 subStreamSize = 0;
						HRESULT result = getSubStreamSize->GetSubStreamSize(subStreamIndex, &subStreamSize);
						needSubSize = false;
						if(result == S_OK) {
							uint64 newEndPos = subStreamStartPos + subStreamSize;
							bool isAccurateEnd = (newEndPos < totalStreamRead || (newEndPos <= totalStreamRead && readWasFinished));
							if(newEndPos <= currentInPos && isAccurateEnd) {
								subStreamStartPos = newEndPos;
								subStreamIndex++;
								continue;
							}

							enc.srcLim = _bufs[BCJ2_NUM_STREAMS] + numBytes_in_ReadBuf;

							if(isAccurateEnd) {
								// data in enc.temp is possible here
								size_t rem = (size_t)(totalStreamRead - newEndPos);

								/* Pos_of(enc.src) <= old newEndPos <= newEndPos
								   in another case, it's fail in some code */
								if((size_t)(enc.srcLim - enc.src) < rem)
									return E_FAIL;

								enc.srcLim -= rem;
								enc.finishMode = BCJ2_ENC_FINISH_MODE_END_BLOCK;
							}

							if(subStreamSize <= BCJ2_FileSize_MAX) {
								enc.fileIp = enc.ip + (uint32)(subStreamStartPos - currentInPos);
								enc.fileSize = (uint32)subStreamSize;
							}
							break;
						}

						if(result == S_FALSE)
							break;
						if(result == E_NOTIMPL) {
							getSubStreamSize.Release();
							break;
						}
						return result;
					}
				}
				if(readWasFinished && totalStreamRead - currentInPos == Bcj2Enc_Get_InputData_Size(&enc))
					enc.finishMode = BCJ2_ENC_FINISH_MODE_END_STREAM;
				Bcj2Enc_Encode(&enc);
				currentInPos = totalStreamRead - numBytes_in_ReadBuf + (enc.src - _bufs[BCJ2_NUM_STREAMS]) - enc.tempPos;
				if(Bcj2Enc_IsFinished(&enc))
					break;
				if(enc.state < BCJ2_NUM_STREAMS) {
					size_t curSize = enc.bufs[enc.state] - _bufs[enc.state];
					// printf("Write stream = %2d %6d\n", enc.state, curSize);
					RINOK(WriteStream(outStreams[enc.state], _bufs[enc.state], curSize));
					if(enc.state == BCJ2_STREAM_RC)
						outSizeRc += curSize;
					enc.bufs[enc.state] = _bufs[enc.state];
					enc.lims[enc.state] = _bufs[enc.state] + _bufsCurSizes[enc.state];
				}
				else if(enc.state != BCJ2_ENC_STATE_ORIG)
					return E_FAIL;
				else {
					needSubSize = true;
					if(numBytes_in_ReadBuf != (size_t)(enc.src - _bufs[BCJ2_NUM_STREAMS])) {
						enc.srcLim = _bufs[BCJ2_NUM_STREAMS] + numBytes_in_ReadBuf;
						continue;
					}
					if(readWasFinished)
						continue;
					numBytes_in_ReadBuf = 0;
					enc.src    = _bufs[BCJ2_NUM_STREAMS];
					enc.srcLim = _bufs[BCJ2_NUM_STREAMS];
					uint32 curSize = _bufsCurSizes[BCJ2_NUM_STREAMS];
					RINOK(inStreams[0]->Read(_bufs[BCJ2_NUM_STREAMS], curSize, &curSize));
					// printf("Read %6d bytes\n", curSize);
					if(curSize == 0) {
						readWasFinished = true;
						continue;
					}
					numBytes_in_ReadBuf = curSize;
					totalStreamRead += numBytes_in_ReadBuf;
					enc.srcLim = _bufs[BCJ2_NUM_STREAMS] + numBytes_in_ReadBuf;
				}
				if(progress && currentInPos - prevProgress >= (1 << 20)) {
					uint64 outSize2 = currentInPos + outSizeRc + enc.bufs[BCJ2_STREAM_RC] - enc.bufs[BCJ2_STREAM_RC];
					prevProgress = currentInPos;
					// printf("progress %8d, %8d\n", (int)inSize2, (int)outSize2);
					RINOK(progress->SetRatioInfo(&currentInPos, &outSize2));
				}
			}
			for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
				RINOK(WriteStream(outStreams[i], _bufs[i], enc.bufs[i] - _bufs[i]));
			}
			// if(currentInPos != subStreamStartPos + subStreamSize) return E_FAIL;
			return S_OK;
		}

		STDMETHODIMP CEncoder::Code(ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
			ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams, ICompressProgressInfo * progress)
		{
			try {
				return CodeReal(inStreams, inSizes, numInStreams, outStreams, outSizes, numOutStreams, progress);
			}
			catch(...) { return E_FAIL; }
		}
		#endif

		CDecoder::CDecoder() : _finishMode(false), _outSizeDefined(false), _outSize(0)
		{
		}
		STDMETHODIMP CDecoder::SetInBufSize(uint32 streamIndex, uint32 size) 
		{
			_bufsNewSizes[streamIndex] = size; return S_OK;
		}
		STDMETHODIMP CDecoder::SetOutBufSize(uint32, uint32 size) 
		{
			_bufsNewSizes[BCJ2_NUM_STREAMS] = size; return S_OK;
		}
		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			_finishMode = (finishMode != 0);
			return S_OK;
		}
		void CDecoder::InitCommon()
		{
			{
				for(int i = 0; i < BCJ2_NUM_STREAMS; i++)
					dec.lims[i] = dec.bufs[i] = _bufs[i];
			}
			{
				for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
					_extraReadSizes[i] = 0;
					_inStreamsProcessed[i] = 0;
					_readRes[i] = S_OK;
				}
			}
			Bcj2Dec_Init(&dec);
		}
		HRESULT CDecoder::Code(ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
			ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams, ICompressProgressInfo * progress)
		{
			if(numInStreams != BCJ2_NUM_STREAMS || numOutStreams != 1)
				return E_INVALIDARG;
			RINOK(Alloc());
			InitCommon();
			dec.destLim = dec.dest = _bufs[BCJ2_NUM_STREAMS];
			uint64 outSizeProcessed = 0;
			uint64 prevProgress = 0;
			HRESULT res = S_OK;
			for(;; ) {
				if(Bcj2Dec_Decode(&dec) != SZ_OK)
					return S_FALSE;
				if(dec.state < BCJ2_NUM_STREAMS) {
					size_t totalRead = _extraReadSizes[dec.state];
					{
						Byte * buf = _bufs[dec.state];
						for(size_t i = 0; i < totalRead; i++)
							buf[i] = dec.bufs[dec.state][i];
						dec.lims[dec.state] = dec.bufs[dec.state] = buf;
					}
					if(_readRes[dec.state] != S_OK) {
						res = _readRes[dec.state];
						break;
					}
					do {
						uint32 curSize = _bufsCurSizes[dec.state] - (uint32)totalRead;
						/*
						   we want to call Read even even if size is 0
						   if(inSizes && inSizes[dec.state])
						   {
						   uint64 rem = *inSizes[dec.state] - _inStreamsProcessed[dec.state];
						   if(curSize > rem)
							curSize = (uint32)rem;
						   }
						 */
						HRESULT res2 = inStreams[dec.state]->Read(_bufs[dec.state] + totalRead, curSize, &curSize);
						_readRes[dec.state] = res2;
						if(curSize == 0)
							break;
						_inStreamsProcessed[dec.state] += curSize;
						totalRead += curSize;
						if(res2 != S_OK)
							break;
					} while(totalRead < 4 && BCJ2_IS_32BIT_STREAM(dec.state));
					if(_readRes[dec.state] != S_OK)
						res = _readRes[dec.state];
					if(totalRead == 0)
						break;
					// res == S_OK;
					if(BCJ2_IS_32BIT_STREAM(dec.state)) {
						unsigned extraSize = ((uint)totalRead & 3);
						_extraReadSizes[dec.state] = extraSize;
						if(totalRead < 4) {
							res = (_readRes[dec.state] != S_OK) ? _readRes[dec.state] : S_FALSE;
							break;
						}
						totalRead -= extraSize;
					}
					dec.lims[dec.state] = _bufs[dec.state] + totalRead;
				}
				else { // if(dec.state <= BCJ2_STATE_ORIG)
					size_t curSize = dec.dest - _bufs[BCJ2_NUM_STREAMS];
					if(curSize != 0) {
						outSizeProcessed += curSize;
						RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize));
					}
					dec.dest = _bufs[BCJ2_NUM_STREAMS];
					{
						size_t rem = _bufsCurSizes[BCJ2_NUM_STREAMS];
						if(outSizes && outSizes[0]) {
							uint64 outSize = *outSizes[0] - outSizeProcessed;
							SETMIN(rem, (size_t)outSize);
						}
						dec.destLim = dec.dest + rem;
						if(rem == 0)
							break;
					}
				}
				if(progress) {
					const uint64 outSize2 = outSizeProcessed + (dec.dest - _bufs[BCJ2_NUM_STREAMS]);
					if(outSize2 - prevProgress >= (1 << 22)) {
						const uint64 inSize2 = outSize2 + _inStreamsProcessed[BCJ2_STREAM_RC] - (dec.lims[BCJ2_STREAM_RC] - dec.bufs[BCJ2_STREAM_RC]);
						RINOK(progress->SetRatioInfo(&inSize2, &outSize2));
						prevProgress = outSize2;
					}
				}
			}
			size_t curSize = dec.dest - _bufs[BCJ2_NUM_STREAMS];
			if(curSize != 0) {
				outSizeProcessed += curSize;
				RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize));
			}
			if(res != S_OK)
				return res;
			if(_finishMode) {
				if(!Bcj2Dec_IsFinished(&dec))
					return S_FALSE;
				// we still allow the cases when input streams are larger than required for decoding.
				// so the case (dec.state == BCJ2_STATE_ORIG) is also allowed, if MAIN stream is larger than required.
				if(!oneof2(dec.state, BCJ2_STREAM_MAIN, BCJ2_DEC_STATE_ORIG))
					return S_FALSE;
				if(inSizes) {
					for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
						size_t rem = dec.lims[i] - dec.bufs[i] + _extraReadSizes[i];
						/*
						   if(rem != 0)
						   return S_FALSE;
						 */
						if(inSizes[i] && *inSizes[i] != _inStreamsProcessed[i] - rem)
							return S_FALSE;
					}
				}
			}
			return S_OK;
		}
		STDMETHODIMP CDecoder::SetInStream2(uint32 streamIndex, ISequentialInStream * inStream)
		{
			_inStreams[streamIndex] = inStream;
			return S_OK;
		}
		STDMETHODIMP CDecoder::ReleaseInStream2(uint32 streamIndex)
		{
			_inStreams[streamIndex].Release();
			return S_OK;
		}
		STDMETHODIMP CDecoder::SetOutStreamSize(const uint64 * outSize)
		{
			_outSizeDefined = (outSize != NULL);
			_outSize = 0;
			if(_outSizeDefined)
				_outSize = *outSize;
			_outSize_Processed = 0;
			HRESULT res = Alloc(false);
			InitCommon();
			dec.destLim = dec.dest = NULL;
			return res;
		}
		STDMETHODIMP CDecoder::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			if(size == 0)
				return S_OK;
			uint32 totalProcessed = 0;
			if(_outSizeDefined) {
				uint64 rem = _outSize - _outSize_Processed;
				SETMIN(size, (uint32)rem);
			}
			dec.dest = (Byte*)data;
			dec.destLim = (const Byte*)data + size;
			HRESULT res = S_OK;
			for(;; ) {
				SRes sres = Bcj2Dec_Decode(&dec);
				if(sres != SZ_OK)
					return S_FALSE;
				{
					uint32 curSize = (uint32)(dec.dest - (Byte*)data);
					if(curSize != 0) {
						totalProcessed += curSize;
						ASSIGN_PTR(processedSize, totalProcessed);
						data = (void *)((Byte*)data + curSize);
						size -= curSize;
						_outSize_Processed += curSize;
					}
				}
				if(dec.state >= BCJ2_NUM_STREAMS)
					break;
				{
					size_t totalRead = _extraReadSizes[dec.state];
					{
						Byte * buf = _bufs[dec.state];
						for(size_t i = 0; i < totalRead; i++)
							buf[i] = dec.bufs[dec.state][i];
						dec.lims[dec.state] = dec.bufs[dec.state] = buf;
					}
					if(_readRes[dec.state] != S_OK)
						return _readRes[dec.state];
					do {
						uint32 curSize = _bufsCurSizes[dec.state] - (uint32)totalRead;
						HRESULT res2 = _inStreams[dec.state]->Read(_bufs[dec.state] + totalRead, curSize, &curSize);
						_readRes[dec.state] = res2;
						if(curSize == 0)
							break;
						_inStreamsProcessed[dec.state] += curSize;
						totalRead += curSize;
						if(res2 != S_OK)
							break;
					} while(totalRead < 4 && BCJ2_IS_32BIT_STREAM(dec.state));
					if(totalRead == 0) {
						if(totalProcessed == 0)
							res = _readRes[dec.state];
						break;
					}
					if(BCJ2_IS_32BIT_STREAM(dec.state)) {
						unsigned extraSize = ((uint)totalRead & 3);
						_extraReadSizes[dec.state] = extraSize;
						if(totalRead < 4) {
							if(totalProcessed != 0)
								return S_OK;
							return (_readRes[dec.state] != S_OK) ? _readRes[dec.state] : S_FALSE;
						}
						totalRead -= extraSize;
					}
					dec.lims[dec.state] = _bufs[dec.state] + totalRead;
				}
			}
			if(_finishMode && _outSizeDefined && _outSize == _outSize_Processed) {
				if(!Bcj2Dec_IsFinished(&dec))
					return S_FALSE;
				if(dec.state != BCJ2_STREAM_MAIN && dec.state != BCJ2_DEC_STATE_ORIG)
					return S_FALSE;
				/*
				   for(int i = 0; i < BCJ2_NUM_STREAMS; i++)
				   if(dec.bufs[i] != dec.lims[i] || _extraReadSizes[i] != 0)
					return S_FALSE;
				 */
			}
			return res;
		}

		STDMETHODIMP CDecoder::GetInStreamProcessedSize2(uint32 streamIndex, uint64 * value)
		{
			const size_t rem = dec.lims[streamIndex] - dec.bufs[streamIndex] + _extraReadSizes[streamIndex];
			*value = _inStreamsProcessed[streamIndex] - rem;
			return S_OK;
		}

		REGISTER_CODEC_CREATE_2(CreateCodec, CDecoder(), ICompressCoder2)
		#ifndef EXTRACT_ONLY
			REGISTER_CODEC_CREATE_2(CreateCodecOut, CEncoder(), ICompressCoder2)
		#else
			#define CreateCodecOut NULL
		#endif
		REGISTER_CODEC_VAR { CreateCodec, CreateCodecOut, 0x303011B, "BCJ2", 4, false };
		REGISTER_CODEC(BCJ2)
	}
	namespace NBZip2 {
		REGISTER_CODEC_CREATE(CreateDec, CDecoder)
		#if !defined(EXTRACT_ONLY) && !defined(BZIP2_EXTRACT_ONLY)
			REGISTER_CODEC_CREATE(CreateEnc, CEncoder)
		#else
			#define CreateEnc NULL
		#endif
		REGISTER_CODEC_2(BZip2, CreateDec, CreateEnc, 0x40202, "BZip2")
	}
	namespace NDeflate {
		REGISTER_CODEC_CREATE(CreateDec, NDecoder::CCOMCoder)
		#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
			REGISTER_CODEC_CREATE(CreateEnc, NEncoder::CCOMCoder)
		#else
			#define CreateEnc NULL
		#endif
		REGISTER_CODEC_2(Deflate, CreateDec, CreateEnc, 0x40108, "Deflate")
	}
	namespace NBranch {
		// BranchMisc.cpp
		STDMETHODIMP CCoder::Init()
		{
			_bufferPos = 0;
			return S_OK;
		}
		STDMETHODIMP_(uint32) CCoder::Filter(Byte *data, uint32 size)
		{
			uint32 processed = (uint32)BraFunc(data, size, _bufferPos, _encode);
			_bufferPos += processed;
			return processed;
		}
		//
		#define CREATE_BRA(n) \
			REGISTER_FILTER_CREATE(CreateBra_Decoder_ ## n, CCoder(n ## _Convert, false)) \
			REGISTER_FILTER_CREATE(CreateBra_Encoder_ ## n, CCoder(n ## _Convert, true)) \

		CREATE_BRA(PPC)
		CREATE_BRA(IA64)
		CREATE_BRA(ARM)
		CREATE_BRA(ARMT)
		CREATE_BRA(SPARC)

		#define METHOD_ITEM(n, id, name) REGISTER_FILTER_ITEM(CreateBra_Decoder_ ## n, CreateBra_Encoder_ ## n, 0x3030000 + id, name)

		REGISTER_CODECS_VAR {
			METHOD_ITEM(PPC,   0x205, "PPC"),
			METHOD_ITEM(IA64,  0x401, "IA64"),
			METHOD_ITEM(ARM,   0x501, "ARM"),
			METHOD_ITEM(ARMT,  0x701, "ARMT"),
			METHOD_ITEM(SPARC, 0x805, "SPARC")
		};

		REGISTER_CODECS(Branch)
	}
}
//