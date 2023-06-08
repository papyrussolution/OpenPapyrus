// 7Z-ARCHIVE.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

// HandlerCont.cpp HandlerOut.cpp ItemNameUtils.cpp ARCHEADER.CPP(7zHeader.cpp CabHeader.cpp IsoHeader.cpp TarHeader.cpp) SplitHandler.cpp
// 7zRegister.cpp TarRegister.cpp IsoRegister.cpp CabRegister.cpp ZipRegister.cpp
namespace NArchive {
	//
	// HandlerCont
	//
	STDMETHODIMP CHandlerCont::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
	{
		COM_TRY_BEGIN
		bool allFilesMode = (numItems == (uint32)(int32)-1);
		if(allFilesMode) {
			RINOK(GetNumberOfItems(&numItems));
		}
		if(numItems == 0)
			return S_OK;
		uint64 totalSize = 0;
		uint32 i;
		for(i = 0; i < numItems; i++) {
			uint64 pos, size;
			GetItem_ExtractInfo(allFilesMode ? i : indices[i], pos, size);
			totalSize += size;
		}
		extractCallback->SetTotal(totalSize);
		totalSize = 0;
		NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder();
		CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> progress = lps;
		lps->Init(extractCallback, false);
		CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
		CMyComPtr<ISequentialInStream> inStream(streamSpec);
		streamSpec->SetStream(_stream);
		for(i = 0; i < numItems; i++) {
			lps->InSize = totalSize;
			lps->OutSize = totalSize;
			RINOK(lps->SetCur());
			CMyComPtr<ISequentialOutStream> outStream;
			int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
			int32 index = allFilesMode ? i : indices[i];
			RINOK(extractCallback->GetStream(index, &outStream, askMode));
			uint64 pos, size;
			int opRes = GetItem_ExtractInfo(index, pos, size);
			totalSize += size;
			if(!testMode && !outStream)
				continue;
			RINOK(extractCallback->PrepareOperation(askMode));
			if(opRes == NExtractArc::NOperationResult::kOK) {
				RINOK(_stream->Seek(pos, STREAM_SEEK_SET, NULL));
				streamSpec->Init(size);
				RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
				opRes = NExtractArc::NOperationResult::kDataError;
				if(copyCoderSpec->TotalSize == size)
					opRes = NExtractArc::NOperationResult::kOK;
				else if(copyCoderSpec->TotalSize < size)
					opRes = NExtractArc::NOperationResult::kUnexpectedEnd;
			}
			outStream.Release();
			RINOK(extractCallback->SetOperationResult(opRes));
		}
		return S_OK;
		COM_TRY_END
	}
	STDMETHODIMP CHandlerCont::GetStream(uint32 index, ISequentialInStream ** stream)
	{
		COM_TRY_BEGIN
		*stream = NULL;
		uint64 pos, size;
		return (GetItem_ExtractInfo(index, pos, size) != NExtractArc::NOperationResult::kOK) ? S_FALSE : CreateLimitedInStream(_stream, pos, size, stream);
		COM_TRY_END
	}
	CHandlerImg::CHandlerImg() : _imgExt(NULL)
	{
		ClearStreamVars();
	}
	CHandlerImg::~CHandlerImg() 
	{
	}
	void CHandlerImg::ClearStreamVars()
	{
		_stream_unavailData = false;
		_stream_unsupportedMethod = false;
		_stream_dataError = false;
		// _stream_UsePackSize = false;
		// _stream_PackSize = 0;
	}

	STDMETHODIMP CHandlerImg::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
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
		ASSIGN_PTR(newPosition, offset);
		return S_OK;
	}

	static const Byte k_GDP_Signature[] = { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T' };

	static const char * GetImgExt(ISequentialInStream * stream)
	{
		const size_t kHeaderSize = 1 << 10;
		Byte buf[kHeaderSize];
		if(ReadStream_FAIL(stream, buf, kHeaderSize) == S_OK) {
			if(buf[0x1FE] == 0x55 && buf[0x1FF] == 0xAA) {
				return (memcmp(buf + 512, k_GDP_Signature, sizeof(k_GDP_Signature)) == 0) ? "gpt" : "mbr";
			}
		}
		return NULL;
	}

	void CHandlerImg::CloseAtError()
	{
		Stream.Release();
	}

	STDMETHODIMP CHandlerImg::Open(IInStream * stream, const uint64 * /* maxCheckStartPosition */, IArchiveOpenCallback * openCallback)
	{
		COM_TRY_BEGIN
		{
			Close();
			HRESULT res;
			try {
				res = Open2(stream, openCallback);
				if(res == S_OK) {
					CMyComPtr<ISequentialInStream> inStream;
					HRESULT res2 = GetStream(0, &inStream);
					if(res2 == S_OK && inStream)
						_imgExt = GetImgExt(inStream);
					return S_OK;
				}
			}
			catch(...) {
				CloseAtError();
				throw;
			}
			CloseAtError();
			return res;
		}
		COM_TRY_END
	}

	STDMETHODIMP CHandlerImg::GetNumberOfItems(uint32 * numItems)
	{
		*numItems = 1;
		return S_OK;
	}

	STDMETHODIMP CHandlerImg::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
	{
		COM_TRY_BEGIN
		if(numItems == 0)
			return S_OK;
		if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
			return E_INVALIDARG;
		RINOK(extractCallback->SetTotal(_size));
		CMyComPtr<ISequentialOutStream> outStream;
		int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
		RINOK(extractCallback->GetStream(0, &outStream, askMode));
		if(!testMode && !outStream)
			return S_OK;
		RINOK(extractCallback->PrepareOperation(askMode));
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> progress = lps;
		lps->Init(extractCallback, false);
		int opRes = NExtractArc::NOperationResult::kDataError;
		ClearStreamVars();
		CMyComPtr<ISequentialInStream> inStream;
		HRESULT hres = GetStream(0, &inStream);
		if(hres == S_FALSE)
			hres = E_NOTIMPL;

		if(hres == S_OK && inStream) {
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder();
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

			hres = copyCoder->Code(inStream, outStream, NULL, &_size, progress);
			if(hres == S_OK) {
				if(copyCoderSpec->TotalSize == _size)
					opRes = NExtractArc::NOperationResult::kOK;

				if(_stream_unavailData)
					opRes = NExtractArc::NOperationResult::kUnavailable;
				else if(_stream_unsupportedMethod)
					opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
				else if(_stream_dataError)
					opRes = NExtractArc::NOperationResult::kDataError;
				else if(copyCoderSpec->TotalSize < _size)
					opRes = NExtractArc::NOperationResult::kUnexpectedEnd;
			}
		}
		inStream.Release();
		outStream.Release();
		if(hres != S_OK) {
			if(hres == S_FALSE)
				opRes = NExtractArc::NOperationResult::kDataError;
			else if(hres == E_NOTIMPL)
				opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
			else
				return hres;
		}
		return extractCallback->SetOperationResult(opRes);
		COM_TRY_END
	}

	HRESULT ReadZeroTail(ISequentialInStream * stream, bool &areThereNonZeros, uint64 &numZeros, uint64 maxSize)
	{
		areThereNonZeros = false;
		numZeros = 0;
		const size_t kBufSize = 1 << 11;
		Byte buf[kBufSize];
		for(;;) {
			uint32 size = 0;
			HRESULT(stream->Read(buf, kBufSize, &size));
			if(size == 0)
				return S_OK;
			for(uint32 i = 0; i < size; i++)
				if(buf[i] != 0) {
					areThereNonZeros = true;
					numZeros += i;
					return S_OK;
				}
			numZeros += size;
			if(numZeros > maxSize)
				return S_OK;
		}
	}
	//
	// HandlerOut
	//
	static void SetMethodProp32(COneMethodInfo &m, PROPID propID, uint32 value)
	{
		if(m.FindProp(propID) < 0)
			m.AddProp32(propID, value);
	}

	void CMultiMethodProps::SetGlobalLevelTo(COneMethodInfo &oneMethodInfo) const
	{
		uint32 level = _level;
		if(level != (uint32)(int32)-1)
			SetMethodProp32(oneMethodInfo, NCoderPropID::kLevel, (uint32)level);
	}

	#ifndef _7ZIP_ST
	void CMultiMethodProps::SetMethodThreadsTo(COneMethodInfo &oneMethodInfo, uint32 numThreads)
	{
		SetMethodProp32(oneMethodInfo, NCoderPropID::kNumThreads, numThreads);
	}
	#endif

	uint CMultiMethodProps::GetNumEmptyMethods() const
	{
		uint i;
		for(i = 0; i < _methods.Size(); i++)
			if(!_methods[i].IsEmpty())
				break;
		return i;
	}

	void CMultiMethodProps::Init()
	{
	  #ifndef _7ZIP_ST
		_numProcessors = _numThreads = NSystem::GetNumberOfProcessors();
	  #endif
		_level = (uint32)(int32)-1;
		_analysisLevel = -1;
		_autoFilter = true;
		_crcSize = 4;
		_filterMethod.Clear();
		_methods.Clear();
	}

	HRESULT CMultiMethodProps::SetProperty(const wchar_t * nameSpec, const PROPVARIANT &value)
	{
		UString name = nameSpec;
		name.MakeLower_Ascii();
		if(name.IsEmpty())
			return E_INVALIDARG;
		else if(name[0] == 'x') {
			name.Delete(0);
			_level = 9;
			return ParsePropToUInt32(name, value, _level);
		}
		else if(name.IsPrefixedBy_Ascii_NoCase("yx")) {
			name.Delete(0, 2);
			uint32 v = 9;
			RINOK(ParsePropToUInt32(name, value, v));
			_analysisLevel = (int)v;
			return S_OK;
		}
		else if(name.IsPrefixedBy_Ascii_NoCase("crc")) {
			name.Delete(0, 3);
			_crcSize = 4;
			return ParsePropToUInt32(name, value, _crcSize);
		}
		else {
			uint32 number;
			uint   index = ParseStringToUInt32(name, number);
			UString realName = name.Ptr(index);
			if(index == 0) {
				if(name.IsPrefixedBy_Ascii_NoCase("mt")) {
			  #ifndef _7ZIP_ST
					RINOK(ParseMtProp(name.Ptr(2), value, _numProcessors, _numThreads));
			  #endif
					return S_OK;
				}
				if(name.IsEqualTo("f")) {
					HRESULT res = PROPVARIANT_to_bool(value, _autoFilter);
					if(res == S_OK)
						return res;
					else if(value.vt != VT_BSTR)
						return E_INVALIDARG;
					else
						return _filterMethod.ParseMethodFromPROPVARIANT(UString(), value);
				}
				number = 0;
			}
			if(number > 64)
				return E_FAIL;
			else {
				for(int j = _methods.Size(); j <= (int)number; j++)
					_methods.Add(COneMethodInfo());
				return _methods[number].ParseMethodFromPROPVARIANT(realName, value);
			}
		}
	}

	void CSingleMethodProps::Init()
	{
		Clear();
		_level = (uint32)(int32)-1;
	  #ifndef _7ZIP_ST
		_numProcessors = _numThreads = NWindows::NSystem::GetNumberOfProcessors();
		AddProp_NumThreads(_numThreads);
	  #endif
	}

	HRESULT CSingleMethodProps::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
	{
		Init();
		for(uint32 i = 0; i < numProps; i++) {
			UString name = names[i];
			name.MakeLower_Ascii();
			if(name.IsEmpty())
				return E_INVALIDARG;
			const PROPVARIANT &value = values[i];
			if(name[0] == L'x') {
				uint32 a = 9;
				RINOK(ParsePropToUInt32(name.Ptr(1), value, a));
				_level = a;
				AddProp_Level(a);
			}
			else if(name.IsPrefixedBy_Ascii_NoCase("mt")) {
		  #ifndef _7ZIP_ST
				RINOK(ParseMtProp(name.Ptr(2), value, _numProcessors, _numThreads));
				AddProp_NumThreads(_numThreads);
		  #endif
			}
			else {
				RINOK(ParseMethodFromPROPVARIANT(names[i], value));
			}
		}
		return S_OK;
	}
	//
	namespace NItemName {
		static const wchar_t kOsPathSepar = WCHAR_PATH_SEPARATOR;
		static const wchar_t kUnixPathSepar = L'/';
		#if WCHAR_PATH_SEPARATOR != L'/'
			void ReplaceSlashes_OsToUnix(UString &name)
			{
				name.Replace(kOsPathSepar, kUnixPathSepar);
			}
		#else
			void ReplaceSlashes_OsToUnix(UString &) {}
		#endif

		UString GetOsPath(const UString &name)
		{
		  #if WCHAR_PATH_SEPARATOR != L'/'
			UString newName = name;
			newName.Replace(kUnixPathSepar, kOsPathSepar);
			return newName;
		  #else
			return name;
		  #endif
		}
		UString GetOsPath_Remove_TailSlash(const UString &name)
		{
			if(name.IsEmpty())
				return UString();
			else {
				UString newName = GetOsPath(name);
				if(newName.Back() == kOsPathSepar)
					newName.DeleteBack();
				return newName;
			}
		}
		void ReplaceToOsSlashes_Remove_TailSlash(UString &name)
		{
			if(!name.IsEmpty()) {
			#if WCHAR_PATH_SEPARATOR != L'/'
				name.Replace(kUnixPathSepar, kOsPathSepar);
			#endif
				if(name.Back() == kOsPathSepar)
					name.DeleteBack();
			}
		}
		bool HasTailSlash(const AString &name, UINT
		  #if defined(_WIN32) && !defined(UNDER_CE)
					codePage
		  #endif
					)
		{
			if(name.IsEmpty())
				return false;
			char c =
			#if defined(_WIN32) && !defined(UNDER_CE)
						*CharPrevExA((WORD)codePage, name, name.Ptr(name.Len()), 0);
			#else
						name.Back();
			#endif
			return (c == '/');
		}
		#ifndef _WIN32
			UString WinPathToOsPath(const UString &name)
			{
				UString newName = name;
				newName.Replace(L'\\', WCHAR_PATH_SEPARATOR);
				return newName;
			}
		#endif
	}
	//
	namespace NSplit {
		static const Byte kProps[] = { kpidPath, kpidSize };
		static const Byte kArcProps[] = { kpidNumVolumes, kpidTotalPhySize };

		class CHandler : public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
			CObjectVector<CMyComPtr<IInStream> > _streams;
			CRecordVector<uint64> _sizes;
			UString _subName;
			uint64 _totalSize;

			HRESULT Open2(IInStream * stream, IArchiveOpenCallback * callback);
		public:
			MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
			INTERFACE_IInArchive(; )
			STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream);
		};

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidMainSubfile: prop = (uint32)0; break;
				case kpidPhySize: if(!_sizes.IsEmpty()) prop = _sizes[0]; break;
				case kpidTotalPhySize: prop = _totalSize; break;
				case kpidNumVolumes: prop = (uint32)_streams.Size(); break;
			}
			prop.Detach(value);
			return S_OK;
		}

		struct CSeqName {
			UString _unchangedPart;
			UString _changedPart;
			bool _splitStyle;

			bool GetNextName(UString &s)
			{
				{
					uint   i = _changedPart.Len();
					for(;;) {
						wchar_t c = _changedPart[--i];
						if(_splitStyle) {
							if(c == 'z') {
								_changedPart.ReplaceOneCharAtPos(i, L'a');
								if(i == 0)
									return false;
								continue;
							}
							else if(c == 'Z') {
								_changedPart.ReplaceOneCharAtPos(i, L'A');
								if(i == 0)
									return false;
								continue;
							}
						}
						else {
							if(c == '9') {
								_changedPart.ReplaceOneCharAtPos(i, L'0');
								if(i == 0) {
									_changedPart.InsertAtFront(L'1');
									break;
								}
								continue;
							}
						}
						c++;
						_changedPart.ReplaceOneCharAtPos(i, c);
						break;
					}
				}
				s = _unchangedPart + _changedPart;
				return true;
			}
		};

		HRESULT CHandler::Open2(IInStream * stream, IArchiveOpenCallback * callback)
		{
			Close();
			if(!callback)
				return S_FALSE;
			CMyComPtr<IArchiveOpenVolumeCallback> volumeCallback;
			callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volumeCallback);
			if(!volumeCallback)
				return S_FALSE;
			UString name;
			{
				NCOM::CPropVariant prop;
				RINOK(volumeCallback->GetProperty(kpidName, &prop));
				if(prop.vt != VT_BSTR)
					return S_FALSE;
				name = prop.bstrVal;
			}
			int    dotPos = name.ReverseFind_Dot();
			const  UString prefix = name.Left(dotPos + 1);
			const  UString ext = name.Ptr(dotPos + 1);
			UString ext2 = ext;
			ext2.MakeLower_Ascii();
			CSeqName seqName;
			uint   numLetters = 2;
			bool   splitStyle = false;
			if(ext2.Len() >= 2 && StringsAreEqual_Ascii(ext2.RightPtr(2), "aa")) {
				splitStyle = true;
				while(numLetters < ext2.Len()) {
					if(ext2[ext2.Len() - numLetters - 1] != 'a')
						break;
					numLetters++;
				}
			}
			else if(ext.Len() >= 2 && StringsAreEqual_Ascii(ext2.RightPtr(2), "01")) {
				while(numLetters < ext2.Len()) {
					if(ext2[ext2.Len() - numLetters - 1] != '0')
						break;
					numLetters++;
				}
				if(numLetters != ext.Len())
					return S_FALSE;
			}
			else
				return S_FALSE;

			seqName._unchangedPart = prefix + ext.Left(ext2.Len() - numLetters);
			seqName._changedPart = ext.RightPtr(numLetters);
			seqName._splitStyle = splitStyle;
			if(prefix.Len() < 1)
				_subName = "file";
			else
				_subName.SetFrom(prefix, prefix.Len() - 1);
			uint64 size;
			{
				/*
				   NCOM::CPropVariant prop;
				   RINOK(volumeCallback->GetProperty(kpidSize, &prop));
				   if(prop.vt != VT_UI8)
				   return E_INVALIDARG;
				   size = prop.uhVal.QuadPart;
				 */
				RINOK(stream->Seek(0, STREAM_SEEK_END, &size));
				RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
			}
			_totalSize += size;
			_sizes.Add(size);
			_streams.Add(stream);
			{
				const uint64 numFiles = _streams.Size();
				RINOK(callback->SetCompleted(&numFiles, NULL));
			}
			for(;;) {
				UString fullName;
				if(!seqName.GetNextName(fullName))
					break;
				CMyComPtr<IInStream> nextStream;
				HRESULT result = volumeCallback->GetStream(fullName, &nextStream);
				if(result == S_FALSE)
					break;
				if(result != S_OK)
					return result;
				if(!nextStream)
					break;
				{
					/*
					   NCOM::CPropVariant prop;
					   RINOK(volumeCallback->GetProperty(kpidSize, &prop));
					   if(prop.vt != VT_UI8)
					   return E_INVALIDARG;
					   size = prop.uhVal.QuadPart;
					 */
					RINOK(nextStream->Seek(0, STREAM_SEEK_END, &size));
					RINOK(nextStream->Seek(0, STREAM_SEEK_SET, NULL));
				}
				_totalSize += size;
				_sizes.Add(size);
				_streams.Add(nextStream);
				{
					const uint64 numFiles = _streams.Size();
					RINOK(callback->SetCompleted(&numFiles, NULL));
				}
			}
			if(_streams.Size() == 1) {
				if(splitStyle)
					return S_FALSE;
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 *, IArchiveOpenCallback * callback)
		{
			COM_TRY_BEGIN
			HRESULT res = Open2(stream, callback);
			if(res != S_OK)
				Close();
			return res;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			_totalSize = 0;
			_subName.Empty();
			_streams.Clear();
			_sizes.Clear();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _streams.IsEmpty() ? 0 : 1;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetProperty(uint32 /* index */, PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPath: prop = _subName; break;
				case kpidSize:
				case kpidPackSize: prop = _totalSize; break;
			}
			prop.Detach(value);
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			if(numItems == 0)
				return S_OK;
			if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
				return E_INVALIDARG;
			uint64 currentTotalSize = 0;
			RINOK(extractCallback->SetTotal(_totalSize));
			CMyComPtr<ISequentialOutStream> outStream;
			int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
			RINOK(extractCallback->GetStream(0, &outStream, askMode));
			if(!testMode && !outStream)
				return S_OK;
			RINOK(extractCallback->PrepareOperation(askMode));
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			FOR_VECTOR(i, _streams) {
				lps->InSize = lps->OutSize = currentTotalSize;
				RINOK(lps->SetCur());
				IInStream * inStream = _streams[i];
				RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
				RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
				currentTotalSize += copyCoderSpec->TotalSize;
			}
			outStream.Release();
			return extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK);
			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetStream(uint32 index, ISequentialInStream ** stream)
		{
			COM_TRY_BEGIN
			if(index != 0)
				return E_INVALIDARG;
			*stream = 0;
			CMultiStream * streamSpec = new CMultiStream;
			CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
			FOR_VECTOR(i, _streams) {
				CMultiStream::CSubStreamInfo subStreamInfo;
				subStreamInfo.Stream = _streams[i];
				subStreamInfo.Size = _sizes[i];
				streamSpec->Streams.Add(subStreamInfo);
			}
			streamSpec->Init();
			*stream = streamTemp.Detach();
			return S_OK;
			COM_TRY_END
		}

		REGISTER_ARC_I_NO_SIG("Split", "001", 0, 0xEA, 0, 0, NULL)
	}
	namespace N7z {
		Byte kSignature[kSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
		#ifdef _7Z_VOL
			Byte kFinishSignature[kSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C + 1};
		#endif
		// We can change signature. So file doesn't contain correct signature.
		// struct SignatureInitializer { SignatureInitializer() { kSignature[0]--; } };
		// static SignatureInitializer g_SignatureInitializer;
		static Byte k_Signature_Dec[kSignatureSize] = {'7' + 1, 'z', 0xBC, 0xAF, 0x27, 0x1C};
		REGISTER_ARC_IO_DECREMENT_SIG("7z", "7z", NULL, 7, k_Signature_Dec, 0, NArcInfoFlags::kFindSignature, NULL);
	}
	namespace NTar {
		namespace NFileHeader {
			const char * const kLongLink = "././@LongLink";
			const char * const kLongLink2 = "@LongLink";

			// The magic field is filled with this if uname and gname are valid.
			namespace NMagic {
				// const char * const kUsTar  = "ustar";   // 5 chars
				// const char * const kGNUTar = "GNUtar "; // 7 chars and a null
				// const char * const kEmpty = "\0\0\0\0\0\0\0\0";
				const char kUsTar_00[8] = { 'u', 's', 't', 'a', 'r', 0, '0', '0' };
			}
		}
		static const Byte k_Signature[] = { 'u', 's', 't', 'a', 'r' };

		REGISTER_ARC_IO("tar", "tar ova", 0, 0xEE, k_Signature, NFileHeader::kUstarMagic_Offset, 
			NArcInfoFlags::kStartOpen|NArcInfoFlags::kSymLinks|NArcInfoFlags::kHardLinks, IsArc_Tar)
	}
	namespace NIso {
		const char * const kElToritoSpec = "EL TORITO SPECIFICATION\0\0\0\0\0\0\0\0\0";
		static const Byte k_Signature[] = { 'C', 'D', '0', '0', '1' };
		REGISTER_ARC_I("Iso", "iso img", 0, 0xE7, k_Signature, NArchive::NIso::kStartPos + 1, 0, NULL)
	}
	namespace NCab {
		namespace NHeader {
			const Byte kMarker[kMarkerSize] = {'M', 'S', 'C', 'F', 0, 0, 0, 0 };
			// struct CSignatureInitializer { CSignatureInitializer() { kMarker[0]--; } } g_SignatureInitializer;
		}
		REGISTER_ARC_I("Cab", "cab", 0, 8, NHeader::kMarker, 0, NArcInfoFlags::kFindSignature, NULL)
	}
	namespace NZip {
		static const Byte k_Signature[] = {
			4, 0x50, 0x4B, 0x03, 0x04,               // Local
			4, 0x50, 0x4B, 0x05, 0x06,               // Ecd
			4, 0x50, 0x4B, 0x06, 0x06,               // Ecd64
			6, 0x50, 0x4B, 0x07, 0x08, 0x50, 0x4B,   // Span / Descriptor
			6, 0x50, 0x4B, 0x30, 0x30, 0x50, 0x4B }; // NoSpan

		REGISTER_ARC_IO("zip", "zip z01 zipx jar xpi odt ods docx xlsx epub ipa apk appx", 0, 1, k_Signature, 0,
			NArcInfoFlags::kFindSignature|NArcInfoFlags::kMultiSignature|NArcInfoFlags::kUseGlobalOffset, IsArc_Zip)
	}
}
