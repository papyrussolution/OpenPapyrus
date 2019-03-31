// 7Z-PPMD.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

// PpmdEncoder.cpp PpmdDecoder.cpp
namespace NCompress {
	namespace NPpmd {
		//
		// PpmdEncoder
		//
		static const uint32 kBufSize = (1 << 20);
		static const Byte kOrders[10] = { 3, 4, 4, 5, 5, 6, 8, 16, 24, 32 };

		CEncProps::CEncProps() : MemSize((uint32)(int32)-1), ReduceSize((uint32)(int32)-1), Order(-1)
		{
		}
		void CEncProps::Normalize(int level)
		{
			if(level < 0) 
				level = 5;
			if(level > 9) 
				level = 9;
			if(MemSize == (uint32)(int32)-1)
				MemSize = (level >= 9) ? ((uint32)192 << 20) : ((uint32)1 << (level + 19));
			const uint kMult = 16;
			if(MemSize / kMult > ReduceSize) {
				for(uint i = 16; i <= 31; i++) {
					uint32 m = (uint32)1 << i;
					if(ReduceSize <= m / kMult) {
						SETMIN(MemSize, m);
						break;
					}
				}
			}
			if(Order == -1) Order = kOrders[(uint)level];
		}
		CEncoder::CEncoder() : _inBuf(NULL)
		{
			_props.Normalize(-1);
			_rangeEnc.Stream = &_outStream.vt;
			Ppmd7_Construct(&_ppmd);
		}
		CEncoder::~CEncoder()
		{
			::MidFree(_inBuf);
			Ppmd7_Free(&_ppmd, &g_BigAlloc);
		}
		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
		{
			int level = -1;
			CEncProps props;
			for(uint32 i = 0; i < numProps; i++) {
				const PROPVARIANT &prop = coderProps[i];
				PROPID propID = propIDs[i];
				if(propID > NCoderPropID::kReduceSize)
					continue;
				if(propID == NCoderPropID::kReduceSize) {
					if(prop.vt == VT_UI8 && prop.uhVal.QuadPart < (uint32)(int32)-1)
						props.ReduceSize = (uint32)prop.uhVal.QuadPart;
					continue;
				}
				if(prop.vt != VT_UI4)
					return E_INVALIDARG;
				uint32 v = (uint32)prop.ulVal;
				switch(propID) {
					case NCoderPropID::kUsedMemorySize:
						if(v < (1 << 16) || v > PPMD7_MAX_MEM_SIZE || (v & 3) != 0)
							return E_INVALIDARG;
						props.MemSize = v;
						break;
					case NCoderPropID::kOrder:
						if(v < 2 || v > 32)
							return E_INVALIDARG;
						props.Order = (Byte)v;
						break;
					case NCoderPropID::kNumThreads: break;
					case NCoderPropID::kLevel: level = (int)v; break;
					default: return E_INVALIDARG;
				}
			}
			props.Normalize(level);
			_props = props;
			return S_OK;
		}

		STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
		{
			const uint32 kPropSize = 5;
			Byte props[kPropSize];
			props[0] = static_cast<Byte>(_props.Order);
			SetUi32(props + 1, _props.MemSize);
			return WriteStream(outStream, props, kPropSize);
		}

		HRESULT CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			if(!_inBuf) {
				_inBuf = static_cast<Byte *>(::MidAlloc(kBufSize));
				if(!_inBuf)
					return E_OUTOFMEMORY;
			}
			if(!_outStream.Alloc(1 << 20))
				return E_OUTOFMEMORY;
			if(!Ppmd7_Alloc(&_ppmd, _props.MemSize, &g_BigAlloc))
				return E_OUTOFMEMORY;
			_outStream.Stream = outStream;
			_outStream.Init();
			Ppmd7z_RangeEnc_Init(&_rangeEnc);
			Ppmd7_Init(&_ppmd, _props.Order);
			uint64 processed = 0;
			for(;; ) {
				uint32 size;
				RINOK(inStream->Read(_inBuf, kBufSize, &size));
				if(size == 0) {
					// We don't write EndMark in PPMD-7z.
					// Ppmd7_EncodeSymbol(&_ppmd, &_rangeEnc, -1);
					Ppmd7z_RangeEnc_FlushData(&_rangeEnc);
					return _outStream.Flush();
				}
				for(uint32 i = 0; i < size; i++) {
					Ppmd7_EncodeSymbol(&_ppmd, &_rangeEnc, _inBuf[i]);
					RINOK(_outStream.Res);
				}
				processed += size;
				if(progress) {
					uint64 outSize = _outStream.GetProcessed();
					RINOK(progress->SetRatioInfo(&processed, &outSize));
				}
			}
		}
		//
		// PpmdDecoder
		//
		enum {
			kStatus_NeedInit,
			kStatus_Normal,
			kStatus_Finished,
			kStatus_Error
		};

		CDecoder::~CDecoder()
		{
			::MidFree(_outBuf);
			Ppmd7_Free(&_ppmd, &g_BigAlloc);
		}

		CDecoder::CDecoder() : _outBuf(NULL), _outSizeDefined(false)
		{
			Ppmd7z_RangeDec_CreateVTable(&_rangeDec);
			_rangeDec.Stream = &_inStream.vt;
			Ppmd7_Construct(&_ppmd);
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * props, uint32 size)
		{
			if(size < 5)
				return E_INVALIDARG;
			_order = props[0];
			uint32 memSize = GetUi32(props + 1);
			if(_order < PPMD7_MIN_ORDER || _order > PPMD7_MAX_ORDER || memSize < PPMD7_MIN_MEM_SIZE || memSize > PPMD7_MAX_MEM_SIZE)
				return E_NOTIMPL;
			if(!_inStream.Alloc(1 << 20))
				return E_OUTOFMEMORY;
			if(!Ppmd7_Alloc(&_ppmd, memSize, &g_BigAlloc))
				return E_OUTOFMEMORY;
			return S_OK;
		}

		HRESULT CDecoder::CodeSpec(Byte * memStream, uint32 size)
		{
			switch(_status) {
				case kStatus_Finished: return S_OK;
				case kStatus_Error: return S_FALSE;
				case kStatus_NeedInit:
					_inStream.Init();
					if(!Ppmd7z_RangeDec_Init(&_rangeDec)) {
						_status = kStatus_Error;
						return S_FALSE;
					}
					_status = kStatus_Normal;
					Ppmd7_Init(&_ppmd, _order);
					break;
			}
			if(_outSizeDefined) {
				const uint64 rem = _outSize - _processedSize;
				SETMIN(size, (uint32)rem);
			}
			uint32 i;
			int sym = 0;
			for(i = 0; i != size; i++) {
				sym = Ppmd7_DecodeSymbol(&_ppmd, &_rangeDec.vt);
				if(_inStream.Extra || sym < 0)
					break;
				memStream[i] = (Byte)sym;
			}
			_processedSize += i;
			if(_inStream.Extra) {
				_status = kStatus_Error;
				return _inStream.Res;
			}
			if(sym < 0)
				_status = (sym < -1) ? kStatus_Error : kStatus_Finished;
			return S_OK;
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			if(!_outBuf) {
				_outBuf = static_cast<Byte *>(::MidAlloc(kBufSize));
				if(!_outBuf)
					return E_OUTOFMEMORY;
			}
			_inStream.Stream = inStream;
			SetOutStreamSize(outSize);
			do {
				const uint64 startPos = _processedSize;
				HRESULT res = CodeSpec(_outBuf, kBufSize);
				size_t processed = static_cast<size_t>(_processedSize - startPos);
				RINOK(WriteStream(outStream, _outBuf, processed));
				RINOK(res);
				if(_status == kStatus_Finished)
					break;
				if(progress) {
					uint64 inSize = _inStream.GetProcessed();
					RINOK(progress->SetRatioInfo(&inSize, &_processedSize));
				}
			} while(!_outSizeDefined || _processedSize < _outSize);
			return S_OK;
		}

		STDMETHODIMP CDecoder::SetOutStreamSize(const uint64 * outSize)
		{
			_outSizeDefined = (outSize != NULL);
			if(_outSizeDefined)
				_outSize = *outSize;
			_processedSize = 0;
			_status = kStatus_NeedInit;
			return S_OK;
		}
		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _inStream.GetProcessed();
			return S_OK;
		}
		#ifndef NO_READ_FROM_CODER
			STDMETHODIMP CDecoder::SetInStream(ISequentialInStream * inStream)
			{
				InSeqStream = inStream;
				_inStream.Stream = inStream;
				return S_OK;
			}
			STDMETHODIMP CDecoder::ReleaseInStream()
			{
				InSeqStream.Release();
				return S_OK;
			}
			STDMETHODIMP CDecoder::Read(void * data, uint32 size, uint32 * processedSize)
			{
				const uint64 startPos = _processedSize;
				HRESULT res = CodeSpec((Byte *)data, size);
				ASSIGN_PTR(processedSize, (uint32)(_processedSize - startPos));
				return res;
			}
		#endif
	}
}
