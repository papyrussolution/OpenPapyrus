// 7Z-XZ.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
using namespace NArchive;
using namespace NWindows;
//using namespace NExtract;
//
// XzEncoder.cpp XzDecoder.cpp
namespace NCompress {
	namespace NLzma2 {
		HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);
	}
	namespace NXz {
		extern "C" {
			static void * SzBigAlloc(ISzAllocPtr, size_t size) { return BigAlloc(size); }
			static void SzBigFree(ISzAllocPtr, void * address) { BigFree(address); }
			static const ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };
			static void * SzAlloc(ISzAllocPtr, size_t size) { return SAlloc::M(size); }
			static void SzFree(ISzAllocPtr, void * address) { SAlloc::F(address); }
			static const ISzAlloc g_Alloc = { SzAlloc, SzFree };
		}
		void CEncoder::InitCoderProps()
		{
			Lzma2EncProps_Init(&_lzma2Props);
			XzProps_Init(&xzProps);
			XzFilterProps_Init(&filter);
			xzProps.lzma2Props = &_lzma2Props;
			// xzProps.filterProps = (_filterId != 0 ? &filter : NULL);
			xzProps.filterProps = NULL;
		}
		CEncoder::CEncoder()
		{
			InitCoderProps();
		}
		CEncoder::~CEncoder()
		{
		}
		HRESULT CEncoder::SetCoderProp(PROPID propID, const PROPVARIANT &prop)
		{
			return NLzma2::SetLzma2Prop(propID, prop, _lzma2Props);
		}

		STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
		{
			Lzma2EncProps_Init(&_lzma2Props);
			for(uint32 i = 0; i < numProps; i++) {
				RINOK(SetCoderProp(propIDs[i], coderProps[i]));
			}
			return S_OK;
			// return SResToHRESULT(Lzma2Enc_SetProps(_encoder, &lzma2Props));
		}

		STDMETHODIMP CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream, 
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			CSeqOutStreamWrap seqOutStream;
			seqOutStream.Init(outStream);
			// if(IntToBool(newData))
			{
				/*
				   uint64 size;
				   {
				   NCOM::CPropVariant prop;
				   RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
				   if(prop.vt != VT_UI8)
					return E_INVALIDARG;
				   size = prop.uhVal.QuadPart;
				   RINOK(updateCallback->SetTotal(size));
				   }
				 */

				/*
				   CLzma2EncProps lzma2Props;
				   Lzma2EncProps_Init(&lzma2Props);

				   lzma2Props.lzmaProps.level = GetLevel();
				 */
				CSeqInStreamWrap seqInStream;
				seqInStream.Init(inStream);
				/*
				   {
				   NCOM::CPropVariant prop = (uint64)size;
				   RINOK(NCompress::NLzma2::SetLzma2Prop(NCoderPropID::kReduceSize, prop, lzma2Props));
				   }

				   FOR_VECTOR (i, _methods) {
				   COneMethodInfo &m = _methods[i];
				   SetGlobalLevelAndThreads(m
				   #ifndef _7ZIP_ST
				   , _numThreads
				   #endif
				   );
				   {
					FOR_VECTOR (j, m.Props) {
					  const CProp &prop = m.Props[j];
					  RINOK(NCompress::NLzma2::SetLzma2Prop(prop.Id, prop.Value, lzma2Props));
					}
				   }
				   }

				   #ifndef _7ZIP_ST
				   lzma2Props.numTotalThreads = _numThreads;
				   #endif

				 */

				CCompressProgressWrap progressWrap;

				progressWrap.Init(progress);

				xzProps.checkId = XZ_CHECK_CRC32;
				// xzProps.checkId = XZ_CHECK_CRC64;
				/*
				   CXzProps xzProps;
				   CXzFilterProps filter;
				   XzProps_Init(&xzProps);
				   XzFilterProps_Init(&filter);
				   xzProps.lzma2Props = &_lzma2Props;
				 */
				/*
				   xzProps.filterProps = (_filterId != 0 ? &filter : NULL);
				   switch (_crcSize)
				   {
				   case  0: xzProps.checkId = XZ_CHECK_NO; break;
				   case  4: xzProps.checkId = XZ_CHECK_CRC32; break;
				   case  8: xzProps.checkId = XZ_CHECK_CRC64; break;
				   case 32: xzProps.checkId = XZ_CHECK_SHA256; break;
				   default: return E_INVALIDARG;
				   }
				   filter.id = _filterId;
				   if(_filterId == XZ_ID_Delta)
				   {
				   bool deltaDefined = false;
				   FOR_VECTOR (j, _filterMethod.Props) {
					const CProp &prop = _filterMethod.Props[j];
					if(prop.Id == NCoderPropID::kDefaultProp && prop.Value.vt == VT_UI4)
					{
					  uint32 delta = (uint32)prop.Value.ulVal;
					  if(delta < 1 || delta > 256)
						return E_INVALIDARG;
					  filter.delta = delta;
					  deltaDefined = true;
					}
				   }
				   if(!deltaDefined)
					return E_INVALIDARG;
				   }
				 */
				SRes res = Xz_Encode(&seqOutStream.vt, &seqInStream.vt, &xzProps, progress ? &progressWrap.vt : NULL);
				/*
				   if(res == SZ_OK)
				   return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
				 */
				return SResToHRESULT(res);
			}
		}
		//
		CStatInfo::CStatInfo() 
		{ 
			Clear(); 
		}

		void CStatInfo::Clear()
		{
			InSize = 0;
			OutSize = 0;
			PhySize = 0;
			NumStreams = 0;
			NumBlocks = 0;
			UnpackSize_Defined = false;
			NumStreams_Defined = false;
			NumBlocks_Defined = false;
			IsArc = false;
			UnexpectedEnd = false;
			DataAfterEnd = false;
			Unsupported = false;
			HeadersError = false;
			DataError = false;
			CrcError = false;
		}

		CXzUnpackerCPP::CXzUnpackerCPP() : InBuf(0), OutBuf(0)
		{
			XzUnpacker_Construct(&p, &g_Alloc);
		}
		CXzUnpackerCPP::~CXzUnpackerCPP()
		{
			XzUnpacker_Free(&p);
			MidFree(InBuf);
			MidFree(OutBuf);
		}
		CDecoder::CDecoder() : DecodeRes(SZ_OK) 
		{
		}
		HRESULT CDecoder::Decode(ISequentialInStream * seqInStream, ISequentialOutStream * outStream,
			const uint64 * outSizeLimit, bool finishStream, ICompressProgressInfo * progress)
		{
			const size_t kInBufSize = (size_t)1 << 20;
			const size_t kOutBufSize = (size_t)1 << 21;
			Clear();
			DecodeRes = SZ_OK;
			XzUnpacker_Init(&xzu.p);
			if(!xzu.InBuf) {
				xzu.InBuf = static_cast<Byte *>(MidAlloc(kInBufSize));
				if(!xzu.InBuf)
					return E_OUTOFMEMORY;
			}
			if(!xzu.OutBuf) {
				xzu.OutBuf = static_cast<Byte *>(MidAlloc(kOutBufSize));
				if(!xzu.OutBuf)
					return E_OUTOFMEMORY;
			}
			uint32 inSize = 0;
			uint32 inPos = 0;
			SizeT outPos = 0;
			HRESULT readRes = S_OK;
			for(;;) {
				if(inPos == inSize && readRes == S_OK) {
					inPos = inSize = 0;
					readRes = seqInStream->Read(xzu.InBuf, kInBufSize, &inSize);
				}
				SizeT inLen = inSize - inPos;
				SizeT outLen = kOutBufSize - outPos;
				ECoderFinishMode finishMode = CODER_FINISH_ANY;
				if(inSize == 0)
					finishMode = CODER_FINISH_END;
				if(outSizeLimit) {
					const uint64 rem = *outSizeLimit - OutSize;
					if(outLen >= rem) {
						outLen = (SizeT)rem;
						if(finishStream)
							finishMode = CODER_FINISH_END;
					}
				}
				ECoderStatus status;
				const SizeT outLenRequested = outLen;
				SRes res = XzUnpacker_Code(&xzu.p, xzu.OutBuf + outPos, &outLen, xzu.InBuf + inPos, &inLen, finishMode, &status);
				DecodeRes = res;
				inPos += (uint32)inLen;
				outPos += outLen;
				InSize += inLen;
				OutSize += outLen;
				bool finished = ((inLen == 0 && outLen == 0) || res != SZ_OK);
				if(outLen >= outLenRequested || finished) {
					if(outStream && outPos != 0) {
						RINOK(WriteStream(outStream, xzu.OutBuf, outPos));
					}
					outPos = 0;
				}
				if(progress) {
					RINOK(progress->SetRatioInfo(&InSize, &OutSize));
				}
				if(!finished)
					continue;
				{
					PhySize = InSize;
					NumStreams = xzu.p.numStartedStreams;
					if(NumStreams > 0)
						IsArc = true;
					NumBlocks = xzu.p.numTotalBlocks;
					UnpackSize_Defined = true;
					NumStreams_Defined = true;
					NumBlocks_Defined = true;
					uint64 extraSize = XzUnpacker_GetExtraSize(&xzu.p);
					if(res == SZ_OK) {
						if(status == CODER_STATUS_NEEDS_MORE_INPUT) {
							extraSize = 0;
							if(!XzUnpacker_IsStreamWasFinished(&xzu.p)) {
								// finished at padding bytes, but padding is not aligned for 4
								UnexpectedEnd = true;
								res = SZ_ERROR_DATA;
							}
						}
						else // status == CODER_STATUS_NOT_FINISHED
							res = SZ_ERROR_DATA;
					}
					else if(res == SZ_ERROR_NO_ARCHIVE) {
						if(InSize == extraSize)
							IsArc = false;
						else {
							if(extraSize != 0 || inPos != inSize) {
								DataAfterEnd = true;
								res = SZ_OK;
							}
						}
					}
					DecodeRes = res;
					PhySize -= extraSize;
					switch(res) {
						case SZ_OK: break;
						case SZ_ERROR_NO_ARCHIVE: IsArc = false; break;
						case SZ_ERROR_ARCHIVE: HeadersError = true; break;
						case SZ_ERROR_UNSUPPORTED: Unsupported = true; break;
						case SZ_ERROR_CRC: CrcError = true; break;
						case SZ_ERROR_DATA: DataError = true; break;
						default: DataError = true; break;
					}
					return readRes;
				}
			}
		}
		int32 CDecoder::Get_Extract_OperationResult() const
		{
			int32 opRes;
			if(!IsArc)
				opRes = NExtractArc::NOperationResult::kIsNotArc;
			else if(UnexpectedEnd)
				opRes = NExtractArc::NOperationResult::kUnexpectedEnd;
			else if(DataAfterEnd)
				opRes = NExtractArc::NOperationResult::kDataAfterEnd;
			else if(CrcError)
				opRes = NExtractArc::NOperationResult::kCRCError;
			else if(Unsupported)
				opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
			else if(HeadersError)
				opRes = NExtractArc::NOperationResult::kDataError;
			else if(DataError)
				opRes = NExtractArc::NOperationResult::kDataError;
			else if(DecodeRes != SZ_OK)
				opRes = NExtractArc::NOperationResult::kDataError;
			else
				opRes = NExtractArc::NOperationResult::kOK;
			return opRes;
		}
		CComDecoder::CComDecoder() : _finishStream(false) 
		{
		}

		HRESULT CComDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			RINOK(_decoder.Decode(inStream, outStream, outSize, _finishStream, progress));
			int32 opRes = _decoder.Get_Extract_OperationResult();
			if(opRes == NArchive::NExtractArc::NOperationResult::kUnsupportedMethod)
				return E_NOTIMPL;
			if(opRes != NArchive::NExtractArc::NOperationResult::kOK)
				return S_FALSE;
			return S_OK;
		}
		STDMETHODIMP CComDecoder::SetFinishMode(uint32 finishMode)
		{
			_finishStream = (finishMode != 0);
			return S_OK;
		}
		STDMETHODIMP CComDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _decoder.InSize;
			return S_OK;
		}
	}
}
//
// XzHandler.cpp
namespace NArchive {
	namespace NXz {
	#define k_LZMA2_Name "LZMA2"

	class CHandler : public IInArchive, public IArchiveOpenSeq,
	#ifndef EXTRACT_ONLY
		public IOutArchive, public ISetProperties, public CMultiMethodProps,
	#endif
		public CMyUnknownImp {
		NCompress::NXz::CStatInfo _stat;
		bool _isArc;
		bool _needSeekToStart;
		bool _phySize_Defined;
		CMyComPtr<IInStream> _stream;
		CMyComPtr<ISequentialInStream> _seqStream;
		AString _methodsString;
	  #ifndef EXTRACT_ONLY
		uint32 _filterId;

		void Init()
		{
			_filterId = 0;
			CMultiMethodProps::Init();
		}
	  #endif
		HRESULT Open2(IInStream * inStream, /* uint32 flags, */ IArchiveOpenCallback * callback);
		HRESULT Decode2(ISequentialInStream * seqInStream, ISequentialOutStream * outStream, NCompress::NXz::CDecoder &decoder, ICompressProgressInfo * progress)
		{
			RINOK(decoder.Decode(seqInStream, outStream, NULL/* *outSizeLimit*/, true/*finishStream*/, progress));
			_stat = decoder;
			_phySize_Defined = true;
			return S_OK;
		}

	public:
		MY_QUERYINTERFACE_BEGIN2(IInArchive)
		MY_QUERYINTERFACE_ENTRY(IArchiveOpenSeq)
	  #ifndef EXTRACT_ONLY
		MY_QUERYINTERFACE_ENTRY(IOutArchive)
		MY_QUERYINTERFACE_ENTRY(ISetProperties)
	  #endif
		MY_QUERYINTERFACE_END
		MY_ADDREF_RELEASE

		INTERFACE_IInArchive(; )
		STDMETHOD(OpenSeq) (ISequentialInStream *stream);

	  #ifndef EXTRACT_ONLY
		INTERFACE_IOutArchive(; )
		STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps);
	  #endif

		CHandler();
	};

	CHandler::CHandler()
	{
	  #ifndef EXTRACT_ONLY
		Init();
	  #endif
	}

	static const Byte kProps[] = { kpidSize, kpidPackSize, kpidMethod };
	static const Byte kArcProps[] = { kpidMethod, kpidNumStreams, kpidNumBlocks };

	IMP_IInArchive_Props
	IMP_IInArchive_ArcProps

	static char FASTCALL GetHex(unsigned value) { return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10))); }

	static inline void AddHexToString(AString &s, Byte value)
	{
		s += GetHex(value >> 4);
		s += GetHex(value & 0xF);
	}

	static void Lzma2PropToString(AString &s, unsigned prop)
	{
		char c = 0;
		uint32 size;
		if((prop & 1) == 0)
			size = prop / 2 + 12;
		else {
			c = 'k';
			size = (uint32)(2 | (prop & 1)) << (prop / 2 + 1);
			if(prop > 17) {
				size >>= 10;
				c = 'm';
			}
		}
		s.Add_UInt32(size);
		if(c != 0)
			s += c;
	}

	struct CMethodNamePair {
		uint32 Id;
		const char * Name;
	};

	static const CMethodNamePair g_NamePairs[] = {
		{ XZ_ID_Subblock, "SB" }, { XZ_ID_Delta, "Delta" }, { XZ_ID_X86, "BCJ" }, { XZ_ID_PPC, "PPC" },
		{ XZ_ID_IA64, "IA64" }, { XZ_ID_ARM, "ARM" }, { XZ_ID_ARMT, "ARMT" }, { XZ_ID_SPARC, "SPARC" }, { XZ_ID_LZMA2, "LZMA2" }
	};

	static AString GetMethodString(const CXzFilter &f)
	{
		const char * p = NULL;
		for(uint i = 0; i < SIZEOFARRAY(g_NamePairs); i++)
			if(g_NamePairs[i].Id == f.id) {
				p = g_NamePairs[i].Name;
				break;
			}
		char temp[32];
		if(!p) {
			::ConvertUInt64ToString(f.id, temp);
			p = temp;
		}

		AString s(p);

		if(f.propsSize > 0) {
			s += ':';
			if(f.id == XZ_ID_LZMA2 && f.propsSize == 1)
				Lzma2PropToString(s, f.props[0]);
			else if(f.id == XZ_ID_Delta && f.propsSize == 1)
				s.Add_UInt32((uint32)f.props[0] + 1);
			else {
				s += '[';
				for(uint32 bi = 0; bi < f.propsSize; bi++)
					AddHexToString(s, f.props[bi]);
				s += ']';
			}
		}
		return s;
	}

	static void AddString(AString &dest, const AString &src)
	{
		dest.Add_Space_if_NotEmpty();
		dest += src;
	}

	static const char * const kChecks[] = { "NoCheck", "CRC32", NULL, NULL, "CRC64", NULL, NULL, NULL, NULL, NULL, "SHA256", NULL, NULL, NULL, NULL, NULL };

	static AString GetCheckString(const CXzs &xzs)
	{
		size_t i;
		uint32 mask = 0;
		for(i = 0; i < xzs.num; i++)
			mask |= ((uint32)1 << XzFlags_GetCheckType(xzs.streams[i].flags));
		AString s;
		for(i = 0; i <= XZ_CHECK_MASK; i++)
			if(((mask >> i) & 1) != 0) {
				AString s2;
				if(kChecks[i])
					s2 = kChecks[i];
				else {
					s2 = "Check-";
					s2.Add_UInt32((uint32)i);
				}
				AddString(s, s2);
			}
		return s;
	}

	STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
	{
		COM_TRY_BEGIN
		NCOM::CPropVariant prop;
		switch(propID) {
			case kpidPhySize: if(_phySize_Defined) prop = _stat.PhySize; break;
			case kpidNumStreams: if(_stat.NumStreams_Defined) prop = _stat.NumStreams; break;
			case kpidNumBlocks: if(_stat.NumBlocks_Defined) prop = _stat.NumBlocks; break;
			case kpidUnpackSize: if(_stat.UnpackSize_Defined) prop = _stat.OutSize; break;
			case kpidMethod: if(!_methodsString.IsEmpty()) prop = _methodsString; break;
			case kpidErrorFlags:
			{
				uint32 v = 0;
				if(!_isArc) v |= kpv_ErrorFlags_IsNotArc; ;
				if(_stat.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
				if(_stat.DataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
				if(_stat.HeadersError) v |= kpv_ErrorFlags_HeadersError;
				if(_stat.Unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
				if(_stat.DataError) v |= kpv_ErrorFlags_DataError;
				if(_stat.CrcError) v |= kpv_ErrorFlags_CrcError;
				prop = v;
			}
		}
		prop.Detach(value);
		return S_OK;
		COM_TRY_END
	}

	STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
	{
		*numItems = 1;
		return S_OK;
	}

	STDMETHODIMP CHandler::GetProperty(uint32, PROPID propID, PROPVARIANT * value)
	{
		COM_TRY_BEGIN
		NCOM::CPropVariant prop;
		switch(propID) {
			case kpidSize: if(_stat.UnpackSize_Defined) prop = _stat.OutSize; break;
			case kpidPackSize: if(_phySize_Defined) prop = _stat.PhySize; break;
			case kpidMethod: if(!_methodsString.IsEmpty()) prop = _methodsString; break;
		}
		prop.Detach(value);
		return S_OK;
		COM_TRY_END
	}

	struct COpenCallbackWrap {
		ICompressProgress vt;
		IArchiveOpenCallback * OpenCallback;
		HRESULT Res;
		COpenCallbackWrap(IArchiveOpenCallback * progress);
	};

	static SRes OpenCallbackProgress(const ICompressProgress * pp, uint64 inSize, uint64 /* outSize */)
	{
		COpenCallbackWrap * p = CONTAINER_FROM_VTBL(pp, COpenCallbackWrap, vt);
		if(p->OpenCallback)
			p->Res = p->OpenCallback->SetCompleted(NULL, &inSize);
		return HRESULT_To_SRes(p->Res, SZ_ERROR_PROGRESS);
	}

	COpenCallbackWrap::COpenCallbackWrap(IArchiveOpenCallback * callback)
	{
		vt.Progress = OpenCallbackProgress;
		OpenCallback = callback;
		Res = SZ_OK;
	}

	#define kInputBufSize ((size_t)1 << 10)

	struct CLookToRead2_CPP : public CLookToRead2 {
		CLookToRead2_CPP()
		{
			buf = NULL;
			LookToRead2_CreateVTable(this, True); // Lookahead ?
		}
		void Alloc(size_t allocSize)
		{
			buf = (Byte *)SAlloc::M(allocSize);
			if(buf)
				this->bufSize = allocSize;
		}
		~CLookToRead2_CPP()
		{
			SAlloc::F(buf);
		}
	};

	static HRESULT SRes_to_Open_HRESULT(SRes res)
	{
		switch(res) {
			case SZ_OK: return S_OK;
			case SZ_ERROR_MEM: return E_OUTOFMEMORY;
			case SZ_ERROR_PROGRESS: return E_ABORT;
				/*
				   case SZ_ERROR_UNSUPPORTED:
				   case SZ_ERROR_CRC:
				   case SZ_ERROR_DATA:
				   case SZ_ERROR_ARCHIVE:
				   case SZ_ERROR_NO_ARCHIVE:
				   return S_FALSE;
				 */
		}
		return S_FALSE;
	}
	HRESULT CHandler::Open2(IInStream * inStream, /* uint32 flags, */ IArchiveOpenCallback * callback)
	{
		_needSeekToStart = true;
		{
			CXzStreamFlags st;
			CSeqInStreamWrap inStreamWrap;
			inStreamWrap.Init(inStream);
			SRes res = Xz_ReadHeader(&st, &inStreamWrap.vt);
			if(res != SZ_OK)
				return SRes_to_Open_HRESULT(res);
			{
				CXzBlock block;
				Bool isIndex;
				uint32 headerSizeRes;
				SRes res2 = XzBlock_ReadHeader(&block, &inStreamWrap.vt, &isIndex, &headerSizeRes);
				if(res2 == SZ_OK && !isIndex) {
					uint numFilters = XzBlock_GetNumFilters(&block);
					for(uint i = 0; i < numFilters; i++)
						AddString(_methodsString, GetMethodString(block.filters[i]));
				}
			}
		}

		RINOK(inStream->Seek(0, STREAM_SEEK_END, &_stat.PhySize));
		if(callback) {
			RINOK(callback->SetTotal(NULL, &_stat.PhySize));
		}
		CSeekInStreamWrap inStreamImp;;
		inStreamImp.Init(inStream);
		CLookToRead2_CPP lookStream;
		lookStream.Alloc(kInputBufSize);
		if(!lookStream.buf)
			return E_OUTOFMEMORY;
		lookStream.realStream = &inStreamImp.vt;
		LookToRead2_Init(&lookStream);
		COpenCallbackWrap openWrap(callback);
		struct CXzsCPP {
			CXzsCPP() 
			{
				Xzs_Construct(&p);
			}
			~CXzsCPP() 
			{
				Xzs_Free(&p, &g_Alloc);
			}
			CXzs p;
		};
		CXzsCPP xzs;
		int64 startPosition;
		SRes res = Xzs_ReadBackward(&xzs.p, &lookStream.vt, &startPosition, &openWrap.vt, &g_Alloc);
		if(res == SZ_ERROR_PROGRESS)
			return (openWrap.Res == S_OK) ? E_FAIL : openWrap.Res;
		/*
		   if(res == SZ_ERROR_NO_ARCHIVE && xzs.p.num > 0)
		   res = SZ_OK;
		 */
		if(res == SZ_OK && startPosition == 0) {
			_phySize_Defined = true;
			_stat.OutSize = Xzs_GetUnpackSize(&xzs.p);
			_stat.UnpackSize_Defined = true;
			_stat.NumStreams = xzs.p.num;
			_stat.NumStreams_Defined = true;
			_stat.NumBlocks = Xzs_GetNumBlocks(&xzs.p);
			_stat.NumBlocks_Defined = true;
			AddString(_methodsString, GetCheckString(xzs.p));
		}
		else {
			res = SZ_OK;
		}

		RINOK(SRes_to_Open_HRESULT(res));
		_stream = inStream;
		_seqStream = inStream;
		_isArc = true;
		return S_OK;
	}

	STDMETHODIMP CHandler::Open(IInStream * inStream, const uint64 *, IArchiveOpenCallback * callback)
	{
		COM_TRY_BEGIN
		{
			Close();
			return Open2(inStream, callback);
		}
		COM_TRY_END
	}

	STDMETHODIMP CHandler::OpenSeq(ISequentialInStream * stream)
	{
		Close();
		_seqStream = stream;
		_isArc = true;
		_needSeekToStart = false;
		return S_OK;
	}

	STDMETHODIMP CHandler::Close()
	{
		_stat.Clear();
		_isArc = false;
		_needSeekToStart = false;
		_phySize_Defined = false;
		_methodsString.Empty();
		_stream.Release();
		_seqStream.Release();
		return S_OK;
	}

	class CSeekToSeqStream : public IInStream, public CMyUnknownImp {
	public:
		CMyComPtr<ISequentialInStream> Stream;
		MY_UNKNOWN_IMP1(IInStream)
		STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	};

	STDMETHODIMP CSeekToSeqStream::Read(void * data, uint32 size, uint32 * processedSize) { return Stream->Read(data, size, processedSize); }
	STDMETHODIMP CSeekToSeqStream::Seek(int64, uint32, uint64 *) { return E_NOTIMPL; }

	STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
	{
		COM_TRY_BEGIN
		if(numItems == 0)
			return S_OK;
		if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
			return E_INVALIDARG;

		if(_phySize_Defined)
			extractCallback->SetTotal(_stat.PhySize);

		uint64 currentTotalPacked = 0;
		RINOK(extractCallback->SetCompleted(&currentTotalPacked));
		CMyComPtr<ISequentialOutStream> realOutStream;
		int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
		RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
		if(!testMode && !realOutStream)
			return S_OK;
		extractCallback->PrepareOperation(askMode);
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> lpsRef = lps;
		lps->Init(extractCallback, true);
		if(_needSeekToStart) {
			if(!_stream)
				return E_FAIL;
			RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
		}
		else
			_needSeekToStart = true;
		NCompress::NXz::CDecoder decoder;
		RINOK(Decode2(_seqStream, realOutStream, decoder, lpsRef));
		int32 opRes = decoder.Get_Extract_OperationResult();
		realOutStream.Release();
		return extractCallback->SetOperationResult(opRes);
		COM_TRY_END
	}

	#ifndef EXTRACT_ONLY

	STDMETHODIMP CHandler::GetFileTimeType(uint32 * timeType)
	{
		*timeType = NFileTimeType::kUnix;
		return S_OK;
	}

	STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream * outStream, uint32 numItems, IArchiveUpdateCallback * updateCallback)
	{
		COM_TRY_BEGIN
		if(numItems == 0) {
			CSeqOutStreamWrap seqOutStream;
			seqOutStream.Init(outStream);
			SRes res = Xz_EncodeEmpty(&seqOutStream.vt);
			return SResToHRESULT(res);
		}
		if(numItems != 1)
			return E_INVALIDARG;
		int32 newData, newProps;
		uint32 indexInArchive;
		if(!updateCallback)
			return E_FAIL;
		RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive));
		if(IntToBool(newProps)) {
			{
				NCOM::CPropVariant prop;
				RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop));
				if(prop.vt != VT_EMPTY)
					if(prop.vt != VT_BOOL || prop.boolVal != VARIANT_FALSE)
						return E_INVALIDARG;
			}
		}
		if(IntToBool(newData)) {
			uint64 size;
			{
				NCOM::CPropVariant prop;
				RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
				if(prop.vt != VT_UI8)
					return E_INVALIDARG;
				size = prop.uhVal.QuadPart;
				RINOK(updateCallback->SetTotal(size));
			}
			NCompress::NXz::CEncoder * encoderSpec = new NCompress::NXz::CEncoder;
			CMyComPtr<ICompressCoder> encoder = encoderSpec;
			CLzma2EncProps &lzma2Props = encoderSpec->_lzma2Props;
			lzma2Props.lzmaProps.level = GetLevel();
			CMyComPtr<ISequentialInStream> fileInStream;
			RINOK(updateCallback->GetStream(0, &fileInStream));
			{
				NCOM::CPropVariant prop = (uint64)size;
				RINOK(encoderSpec->SetCoderProp(NCoderPropID::kReduceSize, prop));
			}
			FOR_VECTOR(i, _methods) {
				COneMethodInfo &m = _methods[i];
				/*
				   SetGlobalLevelTo(m);
				   #ifndef _7ZIP_ST
				   CMultiMethodProps::SetMethodThreads(m, _numThreads);
				   #endif
				 */
				{
					FOR_VECTOR(j, m.Props) {
						const CProp &prop = m.Props[j];
						RINOK(encoderSpec->SetCoderProp(prop.Id, prop.Value));
					}
				}
			}

		#ifndef _7ZIP_ST
			lzma2Props.numTotalThreads = _numThreads;
		#endif

			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(updateCallback, true);

			CXzProps &xzProps = encoderSpec->xzProps;
			CXzFilterProps &filter = encoderSpec->filter;

			XzProps_Init(&xzProps);
			XzFilterProps_Init(&filter);

			xzProps.lzma2Props = &lzma2Props;
			xzProps.filterProps = (_filterId != 0 ? &filter : NULL);
			switch(_crcSize) {
				case  0: xzProps.checkId = XZ_CHECK_NO; break;
				case  4: xzProps.checkId = XZ_CHECK_CRC32; break;
				case  8: xzProps.checkId = XZ_CHECK_CRC64; break;
				case 32: xzProps.checkId = XZ_CHECK_SHA256; break;
				default: return E_INVALIDARG;
			}
			filter.id = _filterId;
			if(_filterId == XZ_ID_Delta) {
				bool deltaDefined = false;
				FOR_VECTOR(j, _filterMethod.Props)
				{
					const CProp &prop = _filterMethod.Props[j];
					if(prop.Id == NCoderPropID::kDefaultProp && prop.Value.vt == VT_UI4) {
						uint32 delta = (uint32)prop.Value.ulVal;
						if(delta < 1 || delta > 256)
							return E_INVALIDARG;
						filter.delta = delta;
						deltaDefined = true;
					}
				}
				if(!deltaDefined)
					return E_INVALIDARG;
			}
			return encoderSpec->Code(fileInStream, outStream, NULL, NULL, progress);
		}
		if(indexInArchive != 0)
			return E_INVALIDARG;
		CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
		updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
		if(opCallback) {
			RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, 0, NUpdateNotifyOp::kReplicate))
		}
		if(_stream) {
			if(_phySize_Defined)
				RINOK(updateCallback->SetTotal(_stat.PhySize));
			RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
		}
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> progress = lps;
		lps->Init(updateCallback, true);
		return NCompress::CopyStream(_stream, outStream, progress);
		COM_TRY_END
	}

	STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
	{
		COM_TRY_BEGIN
		Init();
		for(uint32 i = 0; i < numProps; i++) {
			RINOK(SetProperty(names[i], values[i]));
		}
		if(!_filterMethod.MethodName.IsEmpty()) {
			uint k;
			for(k = 0; k < SIZEOFARRAY(g_NamePairs); k++) {
				const CMethodNamePair &pair = g_NamePairs[k];
				if(sstreqi_ascii(_filterMethod.MethodName, pair.Name)) {
					_filterId = pair.Id;
					break;
				}
			}
			if(k == SIZEOFARRAY(g_NamePairs))
				return E_INVALIDARG;
		}
		_methods.DeleteFrontal(GetNumEmptyMethods());
		if(_methods.Size() > 1)
			return E_INVALIDARG;
		if(_methods.Size() == 1) {
			AString &methodName = _methods[0].MethodName;
			if(methodName.IsEmpty())
				methodName = k_LZMA2_Name;
			else if(!methodName.IsEqualTo_Ascii_NoCase(k_LZMA2_Name))
				return E_INVALIDARG;
		}
		return S_OK;
		COM_TRY_END
	}

	#endif

	REGISTER_ARC_IO("xz", "xz txz", "* .tar", 0xC, XZ_SIG, 0, NArcInfoFlags::kKeepName, NULL)
	}
}
