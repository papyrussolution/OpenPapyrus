// 7Z-LZMA.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

static HRESULT SResToHRESULT(SRes res)
{
	switch(res) {
		case SZ_OK: return S_OK;
		case SZ_ERROR_MEM: return E_OUTOFMEMORY;
		case SZ_ERROR_PARAM: return E_INVALIDARG;
		case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
		case SZ_ERROR_DATA: return S_FALSE;
	}
	return E_FAIL;
}

// Lzma2Encoder.cpp Lzma2Decoder.cpp LzmaEncoder.cpp LzmaDecoder.cpp
namespace NCompress {
	//
	// Lzma2Encoder
	//
	namespace NLzma {
		HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep);
	}
	namespace NLzma2 {
		CEncoder::CEncoder()
		{
			_encoder = 0;
			_encoder = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
			if(_encoder == 0)
				throw 1;
		}

		CEncoder::~CEncoder()
		{
			if(_encoder != 0)
				Lzma2Enc_Destroy(_encoder);
		}

		HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props)
		{
			switch(propID) {
				case NCoderPropID::kBlockSize:
					if(prop.vt == VT_UI4)
						lzma2Props.blockSize = prop.ulVal;
					else if(prop.vt == VT_UI8) {
						size_t v = (size_t)prop.uhVal.QuadPart;
						if(v != prop.uhVal.QuadPart)
							return E_INVALIDARG;
						lzma2Props.blockSize = v;
					}
					else
						return E_INVALIDARG;
					break;
				case NCoderPropID::kNumThreads:
					if(prop.vt != VT_UI4) 
						return E_INVALIDARG; 
					lzma2Props.numTotalThreads = (int)(prop.ulVal); 
					break;
				default:
					RINOK(NLzma::SetLzmaProp(propID, prop, lzma2Props.lzmaProps));
			}
			return S_OK;
		}

		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
		{
			CLzma2EncProps lzma2Props;
			Lzma2EncProps_Init(&lzma2Props);
			for(uint32 i = 0; i < numProps; i++) {
				RINOK(SetLzma2Prop(propIDs[i], coderProps[i], lzma2Props));
			}
			return SResToHRESULT(Lzma2Enc_SetProps(_encoder, &lzma2Props));
		}

		STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
		{
			Byte prop = Lzma2Enc_WriteProperties(_encoder);
			return WriteStream(outStream, &prop, 1);
		}

		STDMETHODIMP CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			CSeqInStreamWrap inWrap;
			CSeqOutStreamWrap outWrap;
			CCompressProgressWrap progressWrap;
			inWrap.Init(inStream);
			outWrap.Init(outStream);
			progressWrap.Init(progress);
			SRes res = Lzma2Enc_Encode(_encoder, &outWrap.vt, &inWrap.vt, progress ? &progressWrap.vt : NULL);
			if(res == SZ_ERROR_READ && inWrap.Res != S_OK)
				return inWrap.Res;
			else if(res == SZ_ERROR_WRITE && outWrap.Res != S_OK)
				return outWrap.Res;
			else if(res == SZ_ERROR_PROGRESS && progressWrap.Res != S_OK)
				return progressWrap.Res;
			else 
				return SResToHRESULT(res);
		}
		//
		// Lzma2Decoder
		//
		CDecoder::CDecoder() : _inBuf(NULL), _finishMode(false), _outSizeDefined(false), _outStep(1 << 22), _inBufSize(0), _inBufSizeNew(1 << 20)
		{
			Lzma2Dec_Construct(&_state);
		}
		CDecoder::~CDecoder()
		{
			Lzma2Dec_Free(&_state, &g_Alloc);
			MidFree(_inBuf);
		}
		STDMETHODIMP CDecoder::SetInBufSize(uint32, uint32 size) { _inBufSizeNew = size; return S_OK; }
		STDMETHODIMP CDecoder::SetOutBufSize(uint32, uint32 size) { _outStep = size; return S_OK; }
		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * prop, uint32 size)
		{
			if(size != 1)
				return E_NOTIMPL;
			RINOK(SResToHRESULT(Lzma2Dec_Allocate(&_state, prop[0], &g_Alloc)));
			if(!_inBuf || _inBufSize != _inBufSizeNew) {
				MidFree(_inBuf);
				_inBufSize = 0;
				_inBuf = static_cast<Byte *>(MidAlloc(_inBufSizeNew));
				if(!_inBuf)
					return E_OUTOFMEMORY;
				_inBufSize = _inBufSizeNew;
			}
			return S_OK;
		}
		STDMETHODIMP CDecoder::SetOutStreamSize(const uint64 * outSize)
		{
			_outSizeDefined = (outSize != NULL);
			_outSize = 0;
			if(_outSizeDefined)
				_outSize = *outSize;
			_inPos = _inLim = 0;
			_inProcessed = 0;
			_outProcessed = 0;
			Lzma2Dec_Init(&_state);
			return S_OK;
		}
		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			_finishMode = (finishMode != 0);
			return S_OK;
		}
		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _inProcessed;
			return S_OK;
		}
		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			if(!_inBuf)
				return S_FALSE;
			SetOutStreamSize(outSize);
			SizeT wrPos = _state.decoder.dicPos;
			HRESULT readRes = S_OK;
			for(;; ) {
				if(_inPos == _inLim && readRes == S_OK) {
					_inPos = _inLim = 0;
					readRes = inStream->Read(_inBuf, _inBufSize, &_inLim);
				}
				const SizeT dicPos = _state.decoder.dicPos;
				SizeT size;
				{
					SizeT next = _state.decoder.dicBufSize;
					if(next - wrPos > _outStep)
						next = wrPos + _outStep;
					size = next - dicPos;
				}
				ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
				if(_outSizeDefined) {
					const uint64 rem = _outSize - _outProcessed;
					if(size >= rem) {
						size = (SizeT)rem;
						if(_finishMode)
							finishMode = LZMA_FINISH_END;
					}
				}
				SizeT inProcessed = _inLim - _inPos;
				ELzmaStatus status;
				SRes res = Lzma2Dec_DecodeToDic(&_state, dicPos + size, _inBuf + _inPos, &inProcessed, finishMode, &status);
				_inPos += (uint32)inProcessed;
				_inProcessed += inProcessed;
				const SizeT outProcessed = _state.decoder.dicPos - dicPos;
				_outProcessed += outProcessed;
				bool outFinished = (_outSizeDefined && _outProcessed >= _outSize);
				bool needStop = (res != 0 || (inProcessed == 0 && outProcessed == 0) || status == LZMA_STATUS_FINISHED_WITH_MARK || (!_finishMode && outFinished));
				if(needStop || outProcessed >= size) {
					HRESULT res2 = WriteStream(outStream, _state.decoder.dic + wrPos, _state.decoder.dicPos - wrPos);
					if(_state.decoder.dicPos == _state.decoder.dicBufSize)
						_state.decoder.dicPos = 0;
					wrPos = _state.decoder.dicPos;
					RINOK(res2);
					if(needStop) {
						if(res != 0)
							return S_FALSE;
						if(status == LZMA_STATUS_FINISHED_WITH_MARK) {
							if(_finishMode) {
								if(inSize && *inSize != _inProcessed)
									return S_FALSE;
								if(_outSizeDefined && _outSize != _outProcessed)
									return S_FALSE;
							}
							return readRes;
						}
						if(!_finishMode && outFinished)
							return readRes;
						return S_FALSE;
					}
				}
				if(progress) {
					RINOK(progress->SetRatioInfo(&_inProcessed, &_outProcessed));
				}
			}
		}

		#ifndef NO_READ_FROM_CODER
			STDMETHODIMP CDecoder::SetInStream(ISequentialInStream * inStream) 
			{
				_inStream = inStream; return S_OK;
			}
			STDMETHODIMP CDecoder::ReleaseInStream() 
			{
				_inStream.Release(); return S_OK;
			}
			STDMETHODIMP CDecoder::Read(void * data, uint32 size, uint32 * processedSize)
			{
				ASSIGN_PTR(processedSize, 0);
				ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
				if(_outSizeDefined) {
					const uint64 rem = _outSize - _outProcessed;
					if(size >= rem) {
						size = (uint32)rem;
						if(_finishMode)
							finishMode = LZMA_FINISH_END;
					}
				}
				HRESULT readRes = S_OK;
				for(;; ) {
					if(_inPos == _inLim && readRes == S_OK) {
						_inPos = _inLim = 0;
						readRes = _inStream->Read(_inBuf, _inBufSize, &_inLim);
					}
					SizeT inProcessed = _inLim - _inPos;
					SizeT outProcessed = size;
					ELzmaStatus status;
					SRes res = Lzma2Dec_DecodeToBuf(&_state, (Byte *)data, &outProcessed, _inBuf + _inPos, &inProcessed, finishMode, &status);
					_inPos += (uint32)inProcessed;
					_inProcessed += inProcessed;
					_outProcessed += outProcessed;
					size -= (uint32)outProcessed;
					data = (Byte *)data + outProcessed;
					if(processedSize)
						*processedSize += (uint32)outProcessed;
					if(res != 0)
						return S_FALSE;
					/*
					   if(status == LZMA_STATUS_FINISHED_WITH_MARK)
					   return readRes;

					   if(size == 0 && status != LZMA_STATUS_NEEDS_MORE_INPUT)
					   {
					   if(_finishMode && _outSizeDefined && _outProcessed >= _outSize)
						return S_FALSE;
					   return readRes;
					   }
					 */

					if(inProcessed == 0 && outProcessed == 0)
						return readRes;
				}
			}
		#endif
	}
	//
	// LzmaEncoder
	//
	namespace NLzma {
		CEncoder::CEncoder()
		{
			_encoder = NULL;
			_encoder = LzmaEnc_Create(&g_Alloc);
			if(!_encoder)
				throw 1;
		}
		CEncoder::~CEncoder()
		{
			if(_encoder)
				LzmaEnc_Destroy(_encoder, &g_Alloc, &g_BigAlloc);
		}
		static inline wchar_t GetUpperChar(wchar_t c)
		{
			if(c >= 'a' && c <= 'z')
				c -= 0x20;
			return c;
		}
		static int ParseMatchFinder(const wchar_t * s, int * btMode, int * numHashBytes)
		{
			wchar_t c = GetUpperChar(*s++);
			if(c == L'H') {
				if(GetUpperChar(*s++) != L'C')
					return 0;
				int numHashBytesLoc = (int)(*s++ - L'0');
				if(numHashBytesLoc < 4 || numHashBytesLoc > 4)
					return 0;
				if(*s != 0)
					return 0;
				*btMode = 0;
				*numHashBytes = numHashBytesLoc;
				return 1;
			}
			if(c != L'B')
				return 0;
			if(GetUpperChar(*s++) != L'T')
				return 0;
			int numHashBytesLoc = (int)(*s++ - L'0');
			if(numHashBytesLoc < 2 || numHashBytesLoc > 4)
				return 0;
			if(*s != 0)
				return 0;
			*btMode = 1;
			*numHashBytes = numHashBytesLoc;
			return 1;
		}

		#define SET_PROP_32(_id_, _dest_) case NCoderPropID::_id_: ep._dest_ = v; break;

		HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep)
		{
			if(propID == NCoderPropID::kMatchFinder) {
				if(prop.vt != VT_BSTR)
					return E_INVALIDARG;
				return ParseMatchFinder(prop.bstrVal, &ep.btMode, &ep.numHashBytes) ? S_OK : E_INVALIDARG;
			}
			if(propID > NCoderPropID::kReduceSize)
				return S_OK;
			if(propID == NCoderPropID::kReduceSize) {
				if(prop.vt == VT_UI8)
					ep.reduceSize = prop.uhVal.QuadPart;
				return S_OK;
			}
			if(prop.vt != VT_UI4)
				return E_INVALIDARG;
			uint32 v = prop.ulVal;
			switch(propID) {
				case NCoderPropID::kDefaultProp: if(v > 31) return E_INVALIDARG; ep.dictSize = (uint32)1 << (uint)v; break;
					SET_PROP_32(kLevel, level)
					SET_PROP_32(kNumFastBytes, fb)
					SET_PROP_32(kMatchFinderCycles, mc)
					SET_PROP_32(kAlgorithm, algo)
					SET_PROP_32(kDictionarySize, dictSize)
					SET_PROP_32(kPosStateBits, pb)
					SET_PROP_32(kLitPosBits, lp)
					SET_PROP_32(kLitContextBits, lc)
					SET_PROP_32(kNumThreads, numThreads)
				default: return E_INVALIDARG;
			}
			return S_OK;
		}
		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
		{
			CLzmaEncProps props;
			LzmaEncProps_Init(&props);
			for(uint32 i = 0; i < numProps; i++) {
				const PROPVARIANT &prop = coderProps[i];
				PROPID propID = propIDs[i];
				switch(propID) {
					case NCoderPropID::kEndMarker:
						if(prop.vt != VT_BOOL) return E_INVALIDARG; props.writeEndMark = (prop.boolVal != VARIANT_FALSE); break;
					default:
						RINOK(SetLzmaProp(propID, prop, props));
				}
			}
			return SResToHRESULT(LzmaEnc_SetProps(_encoder, &props));
		}
		STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
		{
			Byte props[LZMA_PROPS_SIZE];
			size_t size = LZMA_PROPS_SIZE;
			RINOK(LzmaEnc_WriteProperties(_encoder, props, &size));
			return WriteStream(outStream, props, size);
		}
		STDMETHODIMP CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			CSeqInStreamWrap inWrap;
			CSeqOutStreamWrap outWrap;
			CCompressProgressWrap progressWrap;
			inWrap.Init(inStream);
			outWrap.Init(outStream);
			progressWrap.Init(progress);
			SRes res = LzmaEnc_Encode(_encoder, &outWrap.vt, &inWrap.vt, progress ? &progressWrap.vt : NULL, &g_Alloc, &g_BigAlloc);
			_inputProcessed = inWrap.Processed;
			if(res == SZ_ERROR_READ && inWrap.Res != S_OK)
				return inWrap.Res;
			if(res == SZ_ERROR_WRITE && outWrap.Res != S_OK)
				return outWrap.Res;
			if(res == SZ_ERROR_PROGRESS && progressWrap.Res != S_OK)
				return progressWrap.Res;
			return SResToHRESULT(res);
		}
	}
	//
	// LzmaDecoder
	//
	namespace NLzma {
		CDecoder::CDecoder() : _inBuf(NULL), _lzmaStatus(LZMA_STATUS_NOT_SPECIFIED), FinishStream(false), _propsWereSet(false),
			_outSizeDefined(false), _outStep(1 << 22), _inBufSize(0), _inBufSizeNew(1 << 20)
		{
			_inProcessed = 0;
			_inPos = _inLim = 0;
			LzmaDec_Construct(&_state);
		}

		CDecoder::~CDecoder()
		{
			LzmaDec_Free(&_state, &g_Alloc);
			SAlloc::F(_inBuf);
		}

		STDMETHODIMP CDecoder::SetInBufSize(uint32, uint32 size) { _inBufSizeNew = size; return S_OK; }
		STDMETHODIMP CDecoder::SetOutBufSize(uint32, uint32 size) { _outStep = size; return S_OK; }

		HRESULT CDecoder::CreateInputBuffer()
		{
			if(!_inBuf || _inBufSizeNew != _inBufSize) {
				SAlloc::F(_inBuf);
				_inBufSize = 0;
				_inBuf = (Byte *)SAlloc::M(_inBufSizeNew);
				if(!_inBuf)
					return E_OUTOFMEMORY;
				_inBufSize = _inBufSizeNew;
			}
			return S_OK;
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * prop, uint32 size)
		{
			RINOK(SResToHRESULT(LzmaDec_Allocate(&_state, prop, size, &g_Alloc)));
			_propsWereSet = true;
			return CreateInputBuffer();
		}

		void CDecoder::SetOutStreamSizeResume(const uint64 * outSize)
		{
			_outSizeDefined = (outSize != NULL);
			_outSize = 0;
			if(_outSizeDefined)
				_outSize = *outSize;
			_outProcessed = 0;
			_lzmaStatus = LZMA_STATUS_NOT_SPECIFIED;

			LzmaDec_Init(&_state);
		}

		STDMETHODIMP CDecoder::SetOutStreamSize(const uint64 * outSize)
		{
			_inProcessed = 0;
			_inPos = _inLim = 0;
			SetOutStreamSizeResume(outSize);
			return S_OK;
		}

		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			FinishStream = (finishMode != 0);
			return S_OK;
		}

		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _inProcessed;
			return S_OK;
		}

		HRESULT CDecoder::CodeSpec(ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress)
		{
			if(!_inBuf || !_propsWereSet)
				return S_FALSE;

			const uint64 startInProgress = _inProcessed;
			SizeT wrPos = _state.dicPos;
			HRESULT readRes = S_OK;

			for(;; ) {
				if(_inPos == _inLim && readRes == S_OK) {
					_inPos = _inLim = 0;
					readRes = inStream->Read(_inBuf, _inBufSize, &_inLim);
				}

				const SizeT dicPos = _state.dicPos;
				SizeT size;
				{
					SizeT next = _state.dicBufSize;
					if(next - wrPos > _outStep)
						next = wrPos + _outStep;
					size = next - dicPos;
				}

				ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
				if(_outSizeDefined) {
					const uint64 rem = _outSize - _outProcessed;
					if(size >= rem) {
						size = (SizeT)rem;
						if(FinishStream)
							finishMode = LZMA_FINISH_END;
					}
				}

				SizeT inProcessed = _inLim - _inPos;
				ELzmaStatus status;

				SRes res = LzmaDec_DecodeToDic(&_state, dicPos + size, _inBuf + _inPos, &inProcessed, finishMode, &status);

				_lzmaStatus = status;
				_inPos += (uint32)inProcessed;
				_inProcessed += inProcessed;
				const SizeT outProcessed = _state.dicPos - dicPos;
				_outProcessed += outProcessed;

				// we check for LZMA_STATUS_NEEDS_MORE_INPUT to allow RangeCoder initialization, if(_outSizeDefined &&
				// _outSize == 0)
				bool outFinished = (_outSizeDefined && _outProcessed >= _outSize);

				bool needStop = (res != 0
							|| (inProcessed == 0 && outProcessed == 0)
							|| status == LZMA_STATUS_FINISHED_WITH_MARK
							|| (outFinished && status != LZMA_STATUS_NEEDS_MORE_INPUT));

				if(needStop || outProcessed >= size) {
					HRESULT res2 = WriteStream(outStream, _state.dic + wrPos, _state.dicPos - wrPos);

					if(_state.dicPos == _state.dicBufSize)
						_state.dicPos = 0;
					wrPos = _state.dicPos;

					RINOK(res2);

					if(needStop) {
						if(res != 0)
							return S_FALSE;

						if(status == LZMA_STATUS_FINISHED_WITH_MARK) {
							if(FinishStream)
								if(_outSizeDefined && _outSize != _outProcessed)
									return S_FALSE;
							return readRes;
						}

						if(outFinished && status != LZMA_STATUS_NEEDS_MORE_INPUT)
							if(!FinishStream || status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
								return readRes;

						return S_FALSE;
					}
				}

				if(progress) {
					const uint64 inSize = _inProcessed - startInProgress;
					RINOK(progress->SetRatioInfo(&inSize, &_outProcessed));
				}
			}
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			if(!_inBuf)
				return E_INVALIDARG;
			SetOutStreamSize(outSize);
			HRESULT res = CodeSpec(inStream, outStream, progress);
			if(res == S_OK)
				if(FinishStream && inSize && *inSize != _inProcessed)
					res = S_FALSE;
			return res;
		}

		#ifndef NO_READ_FROM_CODER
		STDMETHODIMP CDecoder::SetInStream(ISequentialInStream * inStream) { _inStream = inStream; return S_OK; }
		STDMETHODIMP CDecoder::ReleaseInStream() { _inStream.Release(); return S_OK; }

		STDMETHODIMP CDecoder::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
			if(_outSizeDefined) {
				const uint64 rem = _outSize - _outProcessed;
				if(size >= rem) {
					size = (uint32)rem;
					if(FinishStream)
						finishMode = LZMA_FINISH_END;
				}
			}
			HRESULT readRes = S_OK;
			for(;; ) {
				if(_inPos == _inLim && readRes == S_OK) {
					_inPos = _inLim = 0;
					readRes = _inStream->Read(_inBuf, _inBufSize, &_inLim);
				}
				SizeT inProcessed = _inLim - _inPos;
				SizeT outProcessed = size;
				ELzmaStatus status;
				SRes res = LzmaDec_DecodeToBuf(&_state, (Byte *)data, &outProcessed, _inBuf + _inPos, &inProcessed, finishMode, &status);
				_lzmaStatus = status;
				_inPos += (uint32)inProcessed;
				_inProcessed += inProcessed;
				_outProcessed += outProcessed;
				size -= (uint32)outProcessed;
				data = (Byte *)data + outProcessed;
				if(processedSize)
					*processedSize += (uint32)outProcessed;
				if(res != 0)
					return S_FALSE;
				/*
				   if(status == LZMA_STATUS_FINISHED_WITH_MARK)
				   return readRes;

				   if(size == 0 && status != LZMA_STATUS_NEEDS_MORE_INPUT)
				   {
				   if(FinishStream
					  && _outSizeDefined && _outProcessed >= _outSize
					  && status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
					return S_FALSE;
				   return readRes;
				   }
				 */

				if(inProcessed == 0 && outProcessed == 0)
					return readRes;
			}
		}
		HRESULT CDecoder::CodeResume(ISequentialOutStream * outStream, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			SetOutStreamSizeResume(outSize);
			return CodeSpec(_inStream, outStream, progress);
		}
		HRESULT CDecoder::ReadFromInputStream(void * data, uint32 size, uint32 * processedSize)
		{
			RINOK(CreateInputBuffer());
			ASSIGN_PTR(processedSize, 0);
			HRESULT readRes = S_OK;
			while(size != 0) {
				if(_inPos == _inLim) {
					_inPos = _inLim = 0;
					if(readRes == S_OK)
						readRes = _inStream->Read(_inBuf, _inBufSize, &_inLim);
					if(_inLim == 0)
						break;
				}
				uint32 cur = _inLim - _inPos;
				if(cur > size)
					cur = size;
				memcpy(data, _inBuf + _inPos, cur);
				_inPos += cur;
				_inProcessed += cur;
				size -= cur;
				data = (Byte *)data + cur;
				if(processedSize)
					*processedSize += cur;
			}
			return readRes;
		}
		#endif
	}
}
//
// LzmaHandler.cpp
namespace NArchive {
	namespace NLzma {
		static bool CheckDicSize(const Byte * p)
		{
			uint32 dicSize = GetUi32(p);
			if(dicSize == 1)
				return true;
			else {
				for(uint i = 0; i <= 30; i++)
					if(dicSize == ((uint32)2 << i) || dicSize == ((uint32)3 << i))
						return true;
				return (dicSize == 0xFFFFFFFF);
			}
		}

		static const Byte kProps[] = { kpidSize, kpidPackSize, kpidMethod };
		static const Byte kArcProps[] = { kpidNumStreams, kpidMethod };

		struct CHeader {
			uint64 Size;
			Byte FilterID;
			Byte LzmaProps[5];

			Byte GetProp() const { return LzmaProps[0]; }
			uint32 GetDicSize() const { return GetUi32(LzmaProps + 1); }
			bool HasSize() const { return (Size != (uint64)-1LL); }
			bool Parse(const Byte * buf, bool isThereFilter);
		};

		bool CHeader::Parse(const Byte * buf, bool isThereFilter)
		{
			FilterID = 0;
			if(isThereFilter)
				FilterID = buf[0];
			const Byte * sig = buf + (isThereFilter ? 1 : 0);
			for(int i = 0; i < 5; i++)
				LzmaProps[i] = sig[i];
			Size = GetUi64(sig + 5);
			return LzmaProps[0] < 5 * 5 * 9 && FilterID < 2 && (!HasSize() || Size < ((uint64)1 << 56)) && CheckDicSize(LzmaProps + 1);
		}

		class CDecoder {
			CMyComPtr<ISequentialOutStream> _bcjStream;
			CFilterCoder * _filterCoder;
			CMyComPtr<ICompressCoder> _lzmaDecoder;
		public:
			NCompress::NLzma::CDecoder * _lzmaDecoderSpec;
			~CDecoder();
			HRESULT Create(bool filtered, ISequentialInStream * inStream);
			HRESULT Code(const CHeader &header, ISequentialOutStream * outStream, ICompressProgressInfo * progress);
			uint64 GetInputProcessedSize() const { return _lzmaDecoderSpec->GetInputProcessedSize(); }
			void ReleaseInStream() 
			{
				if(_lzmaDecoder) 
					_lzmaDecoderSpec->ReleaseInStream();
			}
			HRESULT ReadInput(Byte * data, uint32 size, uint32 * processedSize) { return _lzmaDecoderSpec->ReadFromInputStream(data, size, processedSize); }
		};

		HRESULT CDecoder::Create(bool filteredMode, ISequentialInStream * inStream)
		{
			if(!_lzmaDecoder) {
				_lzmaDecoderSpec = new NCompress::NLzma::CDecoder;
				_lzmaDecoderSpec->FinishStream = true;
				_lzmaDecoder = _lzmaDecoderSpec;
			}
			if(filteredMode) {
				if(!_bcjStream) {
					_filterCoder = new CFilterCoder(false);
					CMyComPtr<ICompressCoder> coder = _filterCoder;
					_filterCoder->Filter = new NCompress::NBcj::CCoder(false);
					_bcjStream = _filterCoder;
				}
			}

			return _lzmaDecoderSpec->SetInStream(inStream);
		}

		CDecoder::~CDecoder()
		{
			ReleaseInStream();
		}

		HRESULT CDecoder::Code(const CHeader &header, ISequentialOutStream * outStream, ICompressProgressInfo * progress)
		{
			if(header.FilterID > 1)
				return E_NOTIMPL;
			{
				CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
				_lzmaDecoder.QueryInterface(IID_ICompressSetDecoderProperties2, &setDecoderProperties);
				if(!setDecoderProperties)
					return E_NOTIMPL;
				RINOK(setDecoderProperties->SetDecoderProperties2(header.LzmaProps, 5));
			}
			bool filteredMode = (header.FilterID == 1);
			if(filteredMode) {
				RINOK(_filterCoder->SetOutStream(outStream));
				outStream = _bcjStream;
				RINOK(_filterCoder->SetOutStreamSize(NULL));
			}
			const uint64 * Size = header.HasSize() ? &header.Size : NULL;
			HRESULT res = _lzmaDecoderSpec->CodeResume(outStream, Size, progress);
			if(filteredMode) {
				{
					HRESULT res2 = _filterCoder->OutStreamFinish();
					if(res == S_OK)
						res = res2;
				}
				HRESULT res2 = _filterCoder->ReleaseOutStream();
				if(res == S_OK)
					res = res2;
			}
			RINOK(res);
			if(header.HasSize())
				if(_lzmaDecoderSpec->GetOutputProcessedSize() != header.Size)
					return S_FALSE;
			return S_OK;
		}

		class CHandler : public IInArchive, public IArchiveOpenSeq, public CMyUnknownImp {
			CHeader _header;
			CMyComPtr<IInStream> _stream;
			CMyComPtr<ISequentialInStream> _seqStream;
			bool _lzma86;
			bool _isArc;
			bool _needSeekToStart;
			bool _dataAfterEnd;
			bool _needMoreInput;
			bool _packSize_Defined;
			bool _unpackSize_Defined;
			bool _numStreams_Defined;
			bool _unsupported;
			bool _dataError;
			uint8 Reserve[2]; // @alignment
			uint64 _packSize;
			uint64 _unpackSize;
			uint64 _numStreams;

			void GetMethod(NCOM::CPropVariant &prop);
		public:
			MY_UNKNOWN_IMP2(IInArchive, IArchiveOpenSeq)

			INTERFACE_IInArchive(; )
			STDMETHOD(OpenSeq) (ISequentialInStream *stream);

			CHandler(bool lzma86) : _lzma86(lzma86)
			{
			}
			unsigned GetHeaderSize() const { return 5 + 8 + (_lzma86 ? 1 : 0); }
		};

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPhySize: if(_packSize_Defined) prop = _packSize; break;
				case kpidNumStreams: if(_numStreams_Defined) prop = _numStreams; break;
				case kpidUnpackSize: if(_unpackSize_Defined) prop = _unpackSize; break;
				case kpidMethod: GetMethod(prop); break;
				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!_isArc) v |= kpv_ErrorFlags_IsNotArc; ;
					if(_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
					if(_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
					if(_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
					if(_dataError) v |= kpv_ErrorFlags_DataError;
					prop = v;
					break;
				}
			}
			prop.Detach(value);
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = 1;
			return S_OK;
		}

		static void DictSizeToString(uint32 val, char * s)
		{
			for(uint i = 0; i <= 31; i++)
				if(((uint32)1 << i) == val) {
					::ConvertUInt32ToString(i, s);
					return;
				}
			char c = 'b';
			if((val & ((1 << 20) - 1)) == 0) {
				val >>= 20; c = 'm';
			}
			else if((val & ((1 << 10) - 1)) == 0) {
				val >>= 10; c = 'k';
			}
			::ConvertUInt32ToString(val, s);
			s += sstrlen(s);
			*s++ = c;
			*s = 0;
		}

		static char * AddProp32(char * s, const char * name, uint32 v)
		{
			*s++ = ':';
			s = MyStpCpy(s, name);
			::ConvertUInt32ToString(v, s);
			return s + sstrlen(s);
		}

		void CHandler::GetMethod(NCOM::CPropVariant &prop)
		{
			if(_stream) {
				char sz[64];
				char * s = sz;
				if(_header.FilterID != 0)
					s = MyStpCpy(s, "BCJ ");
				s = MyStpCpy(s, "LZMA:");
				DictSizeToString(_header.GetDicSize(), s);
				s += sstrlen(s);
				uint32 d = _header.GetProp();
				// if(d != 0x5D)
				{
					uint32 lc = d % 9;
					d /= 9;
					uint32 pb = d / 5;
					uint32 lp = d % 5;
					if(lc != 3) s = AddProp32(s, "lc", lc);
					if(lp != 0) s = AddProp32(s, "lp", lp);
					if(pb != 2) s = AddProp32(s, "pb", pb);
				}
				prop = sz;
			}
		}

		STDMETHODIMP CHandler::GetProperty(uint32 /* index */, PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidSize: if(_stream && _header.HasSize()) prop = _header.Size; break;
				case kpidPackSize: if(_packSize_Defined) prop = _packSize; break;
				case kpidMethod: GetMethod(prop); break;
			}
			prop.Detach(value);
			return S_OK;
		}

		API_FUNC_static_IsArc IsArc_Lzma(const Byte * p, size_t size)
		{
			const uint32 kHeaderSize = 1 + 4 + 8;
			if(size < kHeaderSize)
				return k_IsArc_Res_NEED_MORE;
			if(p[0] >= 5 * 5 * 9)
				return k_IsArc_Res_NO;
			uint64 unpackSize = GetUi64(p + 1 + 4);
			if(unpackSize != (uint64)-1LL) {
				if(size >= ((uint64)1 << 56))
					return k_IsArc_Res_NO;
			}
			if(unpackSize != 0) {
				if(size < kHeaderSize + 2)
					return k_IsArc_Res_NEED_MORE;
				if(p[kHeaderSize] != 0)
					return k_IsArc_Res_NO;
				if(unpackSize != (uint64)-1LL) {
					if((p[kHeaderSize + 1] & 0x80) != 0)
						return k_IsArc_Res_NO;
				}
			}
			if(!CheckDicSize(p + 1))
				// return k_IsArc_Res_YES_LOW_PROB;
				return k_IsArc_Res_NO;
			return k_IsArc_Res_YES;
		}
		}

		API_FUNC_static_IsArc IsArc_Lzma86(const Byte * p, size_t size)
		{
			if(size < 1)
				return k_IsArc_Res_NEED_MORE;
			Byte filterID = p[0];
			if(filterID != 0 && filterID != 1)
				return k_IsArc_Res_NO;
			return IsArc_Lzma(p + 1, size - 1);
		}
		}

		STDMETHODIMP CHandler::Open(IInStream * inStream, const uint64 *, IArchiveOpenCallback *)
		{
			Close();
			const uint32 kBufSize = 1 + 5 + 8 + 2;
			Byte buf[kBufSize];
			RINOK(ReadStream_FALSE(inStream, buf, kBufSize));
			if(!_header.Parse(buf, _lzma86))
				return S_FALSE;
			const Byte * start = buf + GetHeaderSize();
			if(start[0] != 0 /* || (start[1] & 0x80) != 0 */) // empty stream with EOS is not 0x80
				return S_FALSE;
			RINOK(inStream->Seek(0, STREAM_SEEK_END, &_packSize));
			if(_packSize >= 24 && _header.Size == 0 && _header.FilterID == 0 && _header.LzmaProps[0] == 0)
				return S_FALSE;
			_isArc = true;
			_stream = inStream;
			_seqStream = inStream;
			_needSeekToStart = true;
			return S_OK;
		}

		STDMETHODIMP CHandler::OpenSeq(ISequentialInStream * stream)
		{
			Close();
			_isArc = true;
			_seqStream = stream;
			return S_OK;
		}

		STDMETHODIMP CHandler::Close()
		{
			_isArc = false;
			_packSize_Defined = false;
			_unpackSize_Defined = false;
			_numStreams_Defined = false;

			_dataAfterEnd = false;
			_needMoreInput = false;
			_unsupported = false;
			_dataError = false;

			_packSize = 0;

			_needSeekToStart = false;

			_stream.Release();
			_seqStream.Release();
			return S_OK;
		}

		class CCompressProgressInfoImp : public ICompressProgressInfo, public CMyUnknownImp {
			CMyComPtr<IArchiveOpenCallback> Callback;
		public:
			uint64 Offset;

			MY_UNKNOWN_IMP1(ICompressProgressInfo)
			STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
			void Init(IArchiveOpenCallback * callback) 
			{
				Callback = callback;
			}
		};

		STDMETHODIMP CCompressProgressInfoImp::SetRatioInfo(const uint64 * inSize, const uint64 * /* outSize */)
		{
			if(Callback) {
				const uint64 files = 0;
				const uint64 val = Offset + *inSize;
				return Callback->SetCompleted(&files, &val);
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			if(numItems == 0)
				return S_OK;
			if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
				return E_INVALIDARG;
			if(_packSize_Defined)
				extractCallback->SetTotal(_packSize);
			CMyComPtr<ISequentialOutStream> realOutStream;
			int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
			RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
			if(!testMode && !realOutStream)
				return S_OK;
			extractCallback->PrepareOperation(askMode);
			CDummyOutStream * outStreamSpec = new CDummyOutStream;
			CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
			outStreamSpec->SetStream(realOutStream);
			outStreamSpec->Init();
			realOutStream.Release();
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, true);

			if(_needSeekToStart) {
				if(!_stream)
					return E_FAIL;
				RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
			}
			else
				_needSeekToStart = true;
			CDecoder decoder;
			HRESULT result = decoder.Create(_lzma86, _seqStream);
			RINOK(result);
			bool firstItem = true;
			uint64 packSize = 0;
			uint64 unpackSize = 0;
			uint64 numStreams = 0;
			bool dataAfterEnd = false;
			for(;; ) {
				lps->InSize = packSize;
				lps->OutSize = unpackSize;
				RINOK(lps->SetCur());
				const uint32 kBufSize = 1 + 5 + 8;
				Byte buf[kBufSize];
				const uint32 headerSize = GetHeaderSize();
				uint32 processed;
				RINOK(decoder.ReadInput(buf, headerSize, &processed));
				if(processed != headerSize) {
					if(processed != 0)
						dataAfterEnd = true;
					break;
				}
				CHeader st;
				if(!st.Parse(buf, _lzma86)) {
					dataAfterEnd = true;
					break;
				}
				numStreams++;
				firstItem = false;
				result = decoder.Code(st, outStream, progress);
				packSize = decoder.GetInputProcessedSize();
				unpackSize = outStreamSpec->GetSize();
				if(result == E_NOTIMPL) {
					_unsupported = true;
					result = S_FALSE;
					break;
				}
				if(result == S_FALSE)
					break;
				RINOK(result);
			}
			if(firstItem) {
				_isArc = false;
				result = S_FALSE;
			}
			else if(result == S_OK || result == S_FALSE) {
				if(dataAfterEnd)
					_dataAfterEnd = true;
				else if(decoder._lzmaDecoderSpec->NeedsMoreInput())
					_needMoreInput = true;
				_packSize = packSize;
				_unpackSize = unpackSize;
				_numStreams = numStreams;
				_packSize_Defined = true;
				_unpackSize_Defined = true;
				_numStreams_Defined = true;
			}
			int32 opResult = NExtractArc::NOperationResult::kOK;
			if(!_isArc)
				opResult = NExtractArc::NOperationResult::kIsNotArc;
			else if(_needMoreInput)
				opResult = NExtractArc::NOperationResult::kUnexpectedEnd;
			else if(_unsupported)
				opResult = NExtractArc::NOperationResult::kUnsupportedMethod;
			else if(_dataAfterEnd)
				opResult = NExtractArc::NOperationResult::kDataAfterEnd;
			else if(result == S_FALSE)
				opResult = NExtractArc::NOperationResult::kDataError;
			else if(result == S_OK)
				opResult = NExtractArc::NOperationResult::kOK;
			else
				return result;
			outStream.Release();
			return extractCallback->SetOperationResult(opResult);
			COM_TRY_END
		}
		namespace NLzmaAr {
			// 2, { 0x5D, 0x00 },
			REGISTER_ARC_I_CLS_NO_SIG(CHandler(false), "lzma", "lzma", 0, 0xA, 0, NArcInfoFlags::kStartOpen|NArcInfoFlags::kKeepName, IsArc_Lzma)
		}
		namespace NLzma86Ar {
			REGISTER_ARC_I_CLS_NO_SIG(CHandler(true), "lzma86", "lzma86", 0, 0xB, 0, NArcInfoFlags::kKeepName, IsArc_Lzma86)
		}
	}
}
//