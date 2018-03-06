// ZHandler.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

namespace NArchive {
	namespace NZ {
		class CHandler : public IInArchive, public CMyUnknownImp {
			CMyComPtr<IInStream> _stream;
			uint64 _packSize;
			// uint64 _unpackSize;
			// bool _unpackSize_Defined;
		public:
			MY_UNKNOWN_IMP1(IInArchive)
			INTERFACE_IInArchive(; )
		};

		static const Byte kProps[] = { kpidPackSize };

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps_NO_Table

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = 1;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NWindows::NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPhySizeCantBeDetected: prop = true; break;
			}
			prop.Detach(value);
			return S_OK;
		}

		STDMETHODIMP CHandler::GetProperty(uint32 /* index */, PROPID propID, PROPVARIANT * value)
		{
			NWindows::NCOM::CPropVariant prop;
			switch(propID) {
				// case kpidSize: if(_unpackSize_Defined) prop = _unpackSize; break;
				case kpidPackSize: prop = _packSize; break;
			}
			prop.Detach(value);
			return S_OK;
		}

		/*
		   class CCompressProgressInfoImp:
		   public ICompressProgressInfo,
		   public CMyUnknownImp
		   {
		   CMyComPtr<IArchiveOpenCallback> Callback;
		   public:
		   MY_UNKNOWN_IMP1(ICompressProgressInfo)
		   STDMETHOD(SetRatioInfo)(const uint64 *inSize, const uint64 *outSize);
		   void Init(IArchiveOpenCallback *callback) { Callback = callback; }
		   };

		   STDMETHODIMP CCompressProgressInfoImp::SetRatioInfo(const uint64 *inSize, const uint64 *outSize)
		   {
		   if(Callback)
		   {
			uint64 files = 1;
			return Callback->SetCompleted(&files, inSize);
		   }
		   return S_OK;
		   }
		 */

		API_FUNC_static_IsArc IsArc_Z(const Byte * p, size_t size)
		{
			return (size < 3) ? k_IsArc_Res_NEED_MORE : (NCompress::NZ::CheckStream(p, MIN(size, NCompress::NZ::kRecommendedCheckSize)) ? k_IsArc_Res_YES : k_IsArc_Res_NO);
		}
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 * /* maxCheckStartPosition */, IArchiveOpenCallback * /* openCallback */)
		{
			COM_TRY_BEGIN
			{
				// RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition));
				Byte buffer[NCompress::NZ::kRecommendedCheckSize];
				// Byte buffer[1500];
				size_t size = NCompress::NZ::kRecommendedCheckSize;
				// size = 700;
				RINOK(ReadStream(stream, buffer, &size));
				if(!NCompress::NZ::CheckStream(buffer, size))
					return S_FALSE;
				uint64 endPos;
				RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
				_packSize = endPos;

				/*
				   bool fullCheck = false;
				   if(fullCheck)
				   {
				   CCompressProgressInfoImp *compressProgressSpec = new CCompressProgressInfoImp;
				   CMyComPtr<ICompressProgressInfo> compressProgress = compressProgressSpec;
				   compressProgressSpec->Init(openCallback);

				   NCompress::NZ::CDecoder *decoderSpec = new NCompress::NZ::CDecoder;
				   CMyComPtr<ICompressCoder> decoder = decoderSpec;

				   CDummyOutStream *outStreamSpec = new CDummyOutStream;
				   CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
				   outStreamSpec->SetStream(NULL);
				   outStreamSpec->Init();
				   decoderSpec->SetProp(_prop);
				   if(openCallback)
				   {
					uint64 files = 1;
					RINOK(openCallback->SetTotal(&files, &endPos));
				   }
				   RINOK(stream->Seek(_streamStartPosition + kSignatureSize, STREAM_SEEK_SET, NULL));
				   HRESULT res = decoder->Code(stream, outStream, NULL, NULL, openCallback ? compressProgress : NULL);
				   if(res != S_OK)
					return S_FALSE;
				   _packSize = decoderSpec->PackSize;
				   }
				 */
				_stream = stream;
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			_packSize = 0;
			// _unpackSize_Defined = false;
			_stream.Release();
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			if(numItems == 0)
				return S_OK;
			if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
				return E_INVALIDARG;
			extractCallback->SetTotal(_packSize);
			uint64 currentTotalPacked = 0;
			RINOK(extractCallback->SetCompleted(&currentTotalPacked));
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
			RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
			NCompress::NZ::CDecoder * decoderSpec = new NCompress::NZ::CDecoder;
			CMyComPtr<ICompressCoder> decoder = decoderSpec;
			int opRes;
			{
				HRESULT result = decoder->Code(_stream, outStream, NULL, NULL, progress);
				if(result == S_FALSE)
					opRes = NExtractArc::NOperationResult::kDataError;
				else {
					RINOK(result);
					opRes = NExtractArc::NOperationResult::kOK;
				}
			}
			// _unpackSize = outStreamSpec->GetSize();
			// _unpackSize_Defined = true;
			outStream.Release();
			return extractCallback->SetOperationResult(opRes);
			COM_TRY_END
		}

		static const Byte k_Signature[] = { 0x1F, 0x9D };

		REGISTER_ARC_I("Z", "z taz", "* .tar", 5, k_Signature, 0, 0, IsArc_Z)
	}
}
