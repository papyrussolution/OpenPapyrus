// CoderMixer2.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

#ifdef USE_MIXER_ST
	class CSequentialInStreamCalcSize : public ISequentialInStream, public CMyUnknownImp {
	public:
		MY_UNKNOWN_IMP1(ISequentialInStream)
		STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	private:
		CMyComPtr<ISequentialInStream> _stream;
		uint64 _size;
		bool _wasFinished;
	public:
		void Init();
		void SetStream(ISequentialInStream * stream);
		void ReleaseStream();
		uint64 GetSize() const { return _size; }
		bool WasFinished() const { return _wasFinished; }
	};

	class COutStreamCalcSize : public ISequentialOutStream, public IOutStreamFinish, public CMyUnknownImp {
		CMyComPtr <ISequentialOutStream> _stream;
		uint64 _size;
	public:
		MY_UNKNOWN_IMP2(ISequentialOutStream, IOutStreamFinish)
		STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
		STDMETHOD(OutStreamFinish) ();
		void SetStream(ISequentialOutStream * stream) { _stream = stream; }
		void ReleaseStream() { _stream.Release(); }
		void Init() { _size = 0; }
		uint64 GetSize() const { return _size; }
	};
	//
	void CSequentialInStreamCalcSize::Init()
	{
		_size = 0;
		_wasFinished = false;
	}

	void CSequentialInStreamCalcSize::SetStream(ISequentialInStream * stream) { _stream = stream; }
	void CSequentialInStreamCalcSize::ReleaseStream() { _stream.Release(); }

	STDMETHODIMP CSequentialInStreamCalcSize::Read(void * data, uint32 size, uint32 * processedSize)
	{
		uint32 realProcessed = 0;
		HRESULT result = S_OK;
		if(_stream)
			result = _stream->Read(data, size, &realProcessed);
		_size += realProcessed;
		if(size != 0 && realProcessed == 0)
			_wasFinished = true;
		ASSIGN_PTR(processedSize, realProcessed);
		return result;
	}
	STDMETHODIMP COutStreamCalcSize::Write(const void * data, uint32 size, uint32 * processedSize)
	{
		HRESULT result = S_OK;
		if(_stream)
			result = _stream->Write(data, size, &size);
		_size += size;
		ASSIGN_PTR(processedSize, size);
		return result;
	}
	STDMETHODIMP COutStreamCalcSize::OutStreamFinish()
	{
		HRESULT result = S_OK;
		if(_stream) {
			CMyComPtr<IOutStreamFinish> outStreamFinish;
			_stream.QueryInterface(IID_IOutStreamFinish, &outStreamFinish);
			if(outStreamFinish)
				result = outStreamFinish->OutStreamFinish();
		}
		return result;
	}
#endif

namespace NCoderMixer2 {
	static void BoolVector_Fill_False(CBoolVector &v, uint size)
	{
		v.ClearAndSetSize(size);
		bool * p = &v[0];
		for(uint i = 0; i < size; i++)
			p[i] = false;
	}

	HRESULT CCoder::CheckDataAfterEnd(bool &dataAfterEnd_Error /* , bool &InternalPackSizeError */) const
	{
		if(Coder) {
			if(PackSizePointers.IsEmpty() || !PackSizePointers[0])
				return S_OK;
			CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
			Coder.QueryInterface(IID_ICompressGetInStreamProcessedSize, (void **)&getInStreamProcessedSize);
			// if(!getInStreamProcessedSize) return E_FAIL;
			if(getInStreamProcessedSize) {
				uint64 processed;
				RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&processed));
				if(processed != static_cast<uint64>(-1LL)) {
					const uint64 size = PackSizes[0];
					if(processed < size && Finish)
						dataAfterEnd_Error = true;
					if(processed > size) {
						// InternalPackSizeError = true;
						// return S_FALSE;
					}
				}
			}
		}
		else if(Coder2) {
			CMyComPtr<ICompressGetInStreamProcessedSize2> getInStreamProcessedSize2;
			Coder2.QueryInterface(IID_ICompressGetInStreamProcessedSize2, (void **)&getInStreamProcessedSize2);
			FOR_VECTOR(i, PackSizePointers) {
				if(PackSizePointers[i]) {
					uint64 processed;
					RINOK(getInStreamProcessedSize2->GetInStreamProcessedSize2(i, &processed));
					if(processed != static_cast<uint64>(-1LL)) {
						const uint64 size = PackSizes[i];
						if(processed < size && Finish)
							dataAfterEnd_Error = true;
						else if(processed > size) {
							// InternalPackSizeError = true;
							// return S_FALSE;
						}
					}
				}
			}
		}

		return S_OK;
	}

	class CBondsChecks {
		CBoolVector _coderUsed;
		bool Init();
		bool CheckCoder(unsigned coderIndex);
	public:
		const CBindInfo * BindInfo;
		bool Check();
	};

	bool CBondsChecks::CheckCoder(unsigned coderIndex)
	{
		const CCoderStreamsInfo &coder = BindInfo->Coders[coderIndex];
		if(coderIndex >= _coderUsed.Size() || _coderUsed[coderIndex])
			return false;
		else {
			_coderUsed[coderIndex] = true;
			uint32 start = BindInfo->Coder_to_Stream[coderIndex];
			for(uint i = 0; i < coder.NumStreams; i++) {
				uint32 ind = start + i;
				if(BindInfo->IsStream_in_PackStreams(ind))
					continue;
				int bond = BindInfo->FindBond_for_PackStream(ind);
				if(bond < 0)
					return false;
				if(!CheckCoder(BindInfo->Bonds[bond].UnpackIndex))
					return false;
			}
			return true;
		}
	}
	bool CBondsChecks::Check()
	{
		BoolVector_Fill_False(_coderUsed, BindInfo->Coders.Size());
		if(!CheckCoder(BindInfo->UnpackCoder))
			return false;
		else {
			FOR_VECTOR(i, _coderUsed) {
				if(!_coderUsed[i])
					return false;
			}
			return true;
		}
	}
	void CBindInfo::ClearMaps()
	{
		Coder_to_Stream.Clear();
		Stream_to_Coder.Clear();
	}

	bool CBindInfo::CalcMapsAndCheck()
	{
		ClearMaps();
		uint32 numStreams = 0;
		if(Coders.Size() == 0)
			return false;
		if(Coders.Size() - 1 != Bonds.Size())
			return false;
		FOR_VECTOR(i, Coders) {
			Coder_to_Stream.Add(numStreams);
			const CCoderStreamsInfo &c = Coders[i];
			for(uint j = 0; j < c.NumStreams; j++)
				Stream_to_Coder.Add(i);
			numStreams += c.NumStreams;
		}
		if(numStreams != GetNum_Bonds_and_PackStreams())
			return false;
		CBondsChecks bc;
		bc.BindInfo = this;
		return bc.Check();
	}

	uint CBindInfo::GetNum_Bonds_and_PackStreams() const { return Bonds.Size() + PackStreams.Size(); }

	int CBindInfo::FindBond_for_PackStream(uint32 packStream) const
	{
		FOR_VECTOR(i, Bonds) {
			if(Bonds[i].PackIndex == packStream)
				return i;
		}
		return -1;
	}

	int CBindInfo::FindBond_for_UnpackStream(uint32 unpackStream) const
	{
		FOR_VECTOR(i, Bonds) {
			if(Bonds[i].UnpackIndex == unpackStream)
				return i;
		}
		return -1;
	}

	bool CBindInfo::SetUnpackCoder()
	{
		bool isOk = false;
		FOR_VECTOR(i, Coders) {
			if(FindBond_for_UnpackStream(i) < 0) {
				if(isOk)
					return false;
				UnpackCoder = i;
				isOk = true;
			}
		}
		return isOk;
	}
	bool CBindInfo::IsStream_in_PackStreams(uint32 streamIndex) const { return FindStream_in_PackStreams(streamIndex) >= 0; }
	int CBindInfo::FindStream_in_PackStreams(uint32 streamIndex) const
	{
		FOR_VECTOR(i, PackStreams) {
			if(PackStreams[i] == streamIndex)
				return i;
		}
		return -1;
	}

	// that function is used before Maps is calculated
	uint32 CBindInfo::GetStream_for_Coder(uint32 coderIndex) const
	{
		uint32 streamIndex = 0;
		for(uint32 i = 0; i < coderIndex; i++)
			streamIndex += Coders[i].NumStreams;
		return streamIndex;
	}

	void CBindInfo::Clear()
	{
		Coders.Clear();
		Bonds.Clear();
		PackStreams.Clear();
		ClearMaps();
	}

	void CBindInfo::GetCoder_for_Stream(uint32 streamIndex, uint32 &coderIndex, uint32 &coderStreamIndex) const
	{
		coderIndex = Stream_to_Coder[streamIndex];
		coderStreamIndex = streamIndex - Coder_to_Stream[coderIndex];
	}

	void CCoder::SetCoderInfo(const uint64 * unpackSize, const uint64 * const * packSizes, bool finish)
	{
		Finish = finish;
		if(unpackSize) {
			UnpackSize = *unpackSize;
			UnpackSizePointer = &UnpackSize;
		}
		else {
			UnpackSize = 0;
			UnpackSizePointer = NULL;
		}
		PackSizes.ClearAndSetSize((uint)NumStreams);
		PackSizePointers.ClearAndSetSize((uint)NumStreams);
		for(uint i = 0; i < NumStreams; i++) {
			if(packSizes && packSizes[i]) {
				PackSizes[i] = *(packSizes[i]);
				PackSizePointers[i] = &PackSizes[i];
			}
			else {
				PackSizes[i] = 0;
				PackSizePointers[i] = NULL;
			}
		}
	}

	CMixer::CMixer(bool encodeMode) : EncodeMode(encodeMode), MainCoderIndex(0)
		// , InternalPackSizeError(false)
	{
	}

	//virtual 
	HRESULT CMixer::SetBindInfo(const CBindInfo &bindInfo)
	{
		_bi = bindInfo;
		IsFilter_Vector.Clear();
		MainCoderIndex = 0;
		return S_OK;
	}
	bool CMixer::Is_UnpackSize_Correct_for_Coder(uint32 coderIndex)
	{
		if(coderIndex == _bi.UnpackCoder)
			return true;
		int bond = _bi.FindBond_for_UnpackStream(coderIndex);
		if(bond < 0)
			throw 20150213;
		/*
		   uint32 coderIndex, coderStreamIndex;
		   _bi.GetCoder_for_Stream(_bi.Bonds[bond].PackIndex, coderIndex, coderStreamIndex);
		 */
		uint32 nextCoder = _bi.Stream_to_Coder[_bi.Bonds[bond].PackIndex];
		return IsFilter_Vector[nextCoder] ? Is_UnpackSize_Correct_for_Coder(nextCoder) : false;
	}
	bool CMixer::Is_PackSize_Correct_for_Stream(uint32 streamIndex)
	{
		if(_bi.IsStream_in_PackStreams(streamIndex))
			return true;
		int bond = _bi.FindBond_for_PackStream(streamIndex);
		if(bond < 0)
			throw 20150213;
		uint32 nextCoder = _bi.Bonds[bond].UnpackIndex;
		if(!IsFilter_Vector[nextCoder])
			return false;
		return Is_PackSize_Correct_for_Coder(nextCoder);
	}

	int CMixer::FindBond_for_Stream(bool forInputStream, uint32 streamIndex) const
	{
		return (EncodeMode == forInputStream) ? _bi.FindBond_for_UnpackStream(streamIndex) : _bi.FindBond_for_PackStream(streamIndex);
	}
	bool CMixer::Is_PackSize_Correct_for_Coder(uint32 coderIndex)
	{
		uint32 startIndex = _bi.Coder_to_Stream[coderIndex];
		uint32 numStreams = _bi.Coders[coderIndex].NumStreams;
		for(uint32 i = 0; i < numStreams; i++)
			if(!Is_PackSize_Correct_for_Stream(startIndex + i))
				return false;
		return true;
	}
	bool CMixer::IsThere_ExternalCoder_in_PackTree(uint32 coderIndex)
	{
		if(IsExternal_Vector[coderIndex])
			return true;
		else {
			uint32 startIndex = _bi.Coder_to_Stream[coderIndex];
			uint32 numStreams = _bi.Coders[coderIndex].NumStreams;
			for(uint32 i = 0; i < numStreams; i++) {
				uint32 si = startIndex + i;
				if(_bi.IsStream_in_PackStreams(si))
					continue;
				int bond = _bi.FindBond_for_PackStream(si);
				if(bond < 0)
					throw 20150213;
				if(IsThere_ExternalCoder_in_PackTree(_bi.Bonds[bond].UnpackIndex))
					return true;
			}
			return false;
		}
	}

	#ifdef USE_MIXER_ST // {
		CMixerST::CMixerST(bool encodeMode) : CMixer(encodeMode)
		{
		}
		CMixerST::~CMixerST() {
		}
		void CMixerST::AddCoder(const CCreatedCoder &cod)
		{
			IsFilter_Vector.Add(cod.IsFilter);
			IsExternal_Vector.Add(cod.IsExternal);
			// const CCoderStreamsInfo &c = _bi.Coders[_coders.Size()];
			CCoderST & c2 = _coders.AddNew();
			c2.NumStreams = cod.NumStreams;
			c2.Coder = cod.Coder;
			c2.Coder2 = cod.Coder2;
			/*
			   if(isFilter) {
			   c2.CanRead = true;
			   c2.CanWrite = true;
			   }
			   else
			 */
			{
				IUnknown * unk = (cod.Coder ? (IUnknown*)cod.Coder : (IUnknown*)cod.Coder2);
				{
					CMyComPtr<ISequentialInStream> s;
					unk->QueryInterface(IID_ISequentialInStream, (void **)&s);
					c2.CanRead = (s != NULL);
				}
				{
					CMyComPtr<ISequentialOutStream> s;
					unk->QueryInterface(IID_ISequentialOutStream, (void **)&s);
					c2.CanWrite = (s != NULL);
				}
			}
		}
		CCoder & CMixerST::GetCoder(uint index) { return _coders[index]; }
		void CMixerST::ReInit() 
		{
		}
		HRESULT CMixerST::GetInStream2(ISequentialInStream * const * inStreams, /* const uint64 * const *inSizes, */
				uint32 outStreamIndex, ISequentialInStream ** inStreamRes)
		{
			uint32 coderIndex = outStreamIndex, coderStreamIndex = 0;
			if(EncodeMode) {
				_bi.GetCoder_for_Stream(outStreamIndex, coderIndex, coderStreamIndex);
				if(coderStreamIndex != 0)
					return E_NOTIMPL;
			}
			const CCoder &coder = _coders[coderIndex];
			CMyComPtr <ISequentialInStream> seqInStream;
			coder.QueryInterface(IID_ISequentialInStream, (void **)&seqInStream);
			if(!seqInStream)
				return E_NOTIMPL;
			uint32 numInStreams = EncodeMode ? 1 : coder.NumStreams;
			uint32 startIndex = EncodeMode ? coderIndex : _bi.Coder_to_Stream[coderIndex];
			bool isSet = false;
			if(numInStreams == 1) {
				CMyComPtr <ICompressSetInStream> setStream;
				coder.QueryInterface(IID_ICompressSetInStream, (void **)&setStream);
				if(setStream) {
					CMyComPtr<ISequentialInStream> seqInStream2;
					RINOK(GetInStream(inStreams, /* inSizes, */ startIndex + 0, &seqInStream2));
					RINOK(setStream->SetInStream(seqInStream2));
					isSet = true;
				}
			}
			if(!isSet && numInStreams != 0) {
				CMyComPtr <ICompressSetInStream2> setStream2;
				coder.QueryInterface(IID_ICompressSetInStream2, (void **)&setStream2);
				if(!setStream2)
					return E_NOTIMPL;
				for(uint32 i = 0; i < numInStreams; i++) {
					CMyComPtr <ISequentialInStream> seqInStream2;
					RINOK(GetInStream(inStreams, /* inSizes, */ startIndex + i, &seqInStream2));
					RINOK(setStream2->SetInStream2(i, seqInStream2));
				}
			}
			*inStreamRes = seqInStream.Detach();
			return S_OK;
		}

		HRESULT CMixerST::GetInStream(ISequentialInStream * const * inStreams, /* const uint64 * const *inSizes, */
			uint32 inStreamIndex, ISequentialInStream ** inStreamRes)
		{
			CMyComPtr<ISequentialInStream> seqInStream;
			{
				int index = -1;
				if(EncodeMode) {
					if(_bi.UnpackCoder == inStreamIndex)
						index = 0;
				}
				else
					index = _bi.FindStream_in_PackStreams(inStreamIndex);
				if(index >= 0) {
					seqInStream = inStreams[(uint)index];
					*inStreamRes = seqInStream.Detach();
					return S_OK;
				}
			}
			int bond = FindBond_for_Stream(true, /*forInputStream*/ inStreamIndex);
			if(bond < 0)
				return E_INVALIDARG;
			RINOK(GetInStream2(inStreams, /* inSizes, */ _bi.Bonds[bond].Get_OutIndex(EncodeMode), &seqInStream));
			while(_binderStreams.Size() <= (uint)bond)
				_binderStreams.AddNew();
			CStBinderStream &bs = _binderStreams[bond];
			if(bs.StreamRef || bs.InStreamSpec)
				return E_NOTIMPL;
			CSequentialInStreamCalcSize * spec = new CSequentialInStreamCalcSize;
			bs.StreamRef = spec;
			bs.InStreamSpec = spec;
			spec->SetStream(seqInStream);
			spec->Init();
			seqInStream = bs.InStreamSpec;
			*inStreamRes = seqInStream.Detach();
			return S_OK;
		}

		CMixerST::CCoderST::CCoderST() : CanRead(false), CanWrite(false) 
		{
		}
		CMixerST::CStBinderStream::CStBinderStream() : InStreamSpec(NULL), OutStreamSpec(NULL) 
		{
		}
		HRESULT CMixerST::GetOutStream(ISequentialOutStream * const * outStreams, /* const uint64 * const *outSizes, */
			uint32 outStreamIndex, ISequentialOutStream ** outStreamRes)
		{
			CMyComPtr<ISequentialOutStream> seqOutStream;
			{
				int index = -1;
				if(!EncodeMode) {
					if(_bi.UnpackCoder == outStreamIndex)
						index = 0;
				}
				else
					index = _bi.FindStream_in_PackStreams(outStreamIndex);
				if(index >= 0) {
					seqOutStream = outStreams[(uint)index];
					*outStreamRes = seqOutStream.Detach();
					return S_OK;
				}
			}
			int bond = FindBond_for_Stream(false/*forInputStream*/, outStreamIndex);
			if(bond < 0)
				return E_INVALIDARG;
			uint32 inStreamIndex = _bi.Bonds[bond].Get_InIndex(EncodeMode);
			uint32 coderIndex = inStreamIndex;
			uint32 coderStreamIndex = 0;
			if(!EncodeMode)
				_bi.GetCoder_for_Stream(inStreamIndex, coderIndex, coderStreamIndex);
			CCoder & coder = _coders[coderIndex];
			/*
			   if(!coder.Coder)
			   return E_NOTIMPL;
			 */
			coder.QueryInterface(IID_ISequentialOutStream, (void **)&seqOutStream);
			if(!seqOutStream)
				return E_NOTIMPL;
			uint32 numOutStreams = EncodeMode ? coder.NumStreams : 1;
			uint32 startIndex = EncodeMode ? _bi.Coder_to_Stream[coderIndex] : coderIndex;
			bool isSet = false;
			if(numOutStreams == 1) {
				CMyComPtr<ICompressSetOutStream> setOutStream;
				coder.Coder.QueryInterface(IID_ICompressSetOutStream, &setOutStream);
				if(setOutStream) {
					CMyComPtr<ISequentialOutStream> seqOutStream2;
					RINOK(GetOutStream(outStreams, /* outSizes, */ startIndex + 0, &seqOutStream2));
					RINOK(setOutStream->SetOutStream(seqOutStream2));
					isSet = true;
				}
			}
			if(!isSet && numOutStreams != 0) {
				return E_NOTIMPL;
				/*
				   CMyComPtr<ICompressSetOutStream2> setStream2;
				   coder.QueryInterface(IID_ICompressSetOutStream2, (void **)&setStream2);
				   if(!setStream2)
				   return E_NOTIMPL;
				   for(uint32 i = 0; i < numOutStreams; i++) {
				   CMyComPtr<ISequentialOutStream> seqOutStream2;
				   RINOK(GetOutStream(outStreams, startIndex + i, &seqOutStream2));
				   RINOK(setStream2->SetOutStream2(i, seqOutStream2));
				   }
				 */
			}
			while(_binderStreams.Size() <= (uint)bond)
				_binderStreams.AddNew();
			CStBinderStream &bs = _binderStreams[bond];
			if(bs.StreamRef || bs.OutStreamSpec)
				return E_NOTIMPL;
			COutStreamCalcSize * spec = new COutStreamCalcSize;
			bs.StreamRef = (ISequentialOutStream*)spec;
			bs.OutStreamSpec = spec;
			spec->SetStream(seqOutStream);
			spec->Init();
			seqOutStream = bs.OutStreamSpec;
			*outStreamRes = seqOutStream.Detach();
			return S_OK;
		}

		static HRESULT GetError(HRESULT res, HRESULT res2)
		{
			if(res == res2)
				return res;
			if(res == S_OK)
				return res2;
			if(res == k_My_HRESULT_WritingWasCut) {
				if(res2 != S_OK)
					return res2;
			}
			return res;
		}

		HRESULT CMixerST::FinishStream(uint32 streamIndex)
		{
			{
				int index = -1;
				if(!EncodeMode) {
					if(_bi.UnpackCoder == streamIndex)
						index = 0;
				}
				else
					index = _bi.FindStream_in_PackStreams(streamIndex);
				if(index >= 0)
					return S_OK;
			}
			int bond = FindBond_for_Stream(false, // forInputStream
					streamIndex);
			if(bond < 0)
				return E_INVALIDARG;
			uint32 inStreamIndex = _bi.Bonds[bond].Get_InIndex(EncodeMode);
			uint32 coderIndex = inStreamIndex;
			uint32 coderStreamIndex = 0;
			if(!EncodeMode)
				_bi.GetCoder_for_Stream(inStreamIndex, coderIndex, coderStreamIndex);
			CCoder &coder = _coders[coderIndex];
			CMyComPtr<IOutStreamFinish> finish;
			coder.QueryInterface(IID_IOutStreamFinish, (void **)&finish);
			HRESULT res = S_OK;
			if(finish) {
				res = finish->OutStreamFinish();
			}
			return GetError(res, FinishCoder(coderIndex));
		}

		HRESULT CMixerST::FinishCoder(uint32 coderIndex)
		{
			CCoder &coder = _coders[coderIndex];
			uint32 numOutStreams = EncodeMode ? coder.NumStreams : 1;
			uint32 startIndex = EncodeMode ? _bi.Coder_to_Stream[coderIndex] : coderIndex;
			HRESULT res = S_OK;
			for(uint i = 0; i < numOutStreams; i++)
				res = GetError(res, FinishStream(startIndex + i));
			return res;
		}

		void CMixerST::SelectMainCoder(bool useFirst)
		{
			uint   ci = _bi.UnpackCoder;
			int    firstNonFilter = -1;
			int    firstAllowed = ci;
			for(;;) {
				const CCoderST & coder = _coders[ci];
				// break;
				if(ci != _bi.UnpackCoder)
					if(EncodeMode ? !coder.CanWrite : !coder.CanRead) {
						firstAllowed = ci;
						firstNonFilter = -2;
					}
				if(coder.NumStreams != 1)
					break;
				uint32 st = _bi.Coder_to_Stream[ci];
				if(_bi.IsStream_in_PackStreams(st))
					break;
				int bond = _bi.FindBond_for_PackStream(st);
				if(bond < 0)
					throw 20150213;
				if(EncodeMode ? !coder.CanRead : !coder.CanWrite)
					break;
				if(firstNonFilter == -1 && !IsFilter_Vector[ci])
					firstNonFilter = ci;
				ci = _bi.Bonds[bond].UnpackIndex;
			}
			if(useFirst)
				ci = firstAllowed;
			else if(firstNonFilter >= 0)
				ci = firstNonFilter;
			MainCoderIndex = ci;
		}

		HRESULT CMixerST::Code(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams,
			ICompressProgressInfo * progress, bool &dataAfterEnd_Error)
		{
			// InternalPackSizeError = false;
			dataAfterEnd_Error = false;
			_binderStreams.Clear();
			unsigned ci = MainCoderIndex;
			const CCoder &mainCoder = _coders[MainCoderIndex];
			CObjectVector< CMyComPtr<ISequentialInStream> > seqInStreams;
			CObjectVector< CMyComPtr<ISequentialOutStream> > seqOutStreams;
			uint32 numInStreams  =  EncodeMode ? 1 : mainCoder.NumStreams;
			uint32 numOutStreams = !EncodeMode ? 1 : mainCoder.NumStreams;
			uint32 startInIndex  =  EncodeMode ? ci : _bi.Coder_to_Stream[ci];
			uint32 startOutIndex = !EncodeMode ? ci : _bi.Coder_to_Stream[ci];
			uint32 i;
			for(i = 0; i < numInStreams; i++) {
				CMyComPtr<ISequentialInStream> seqInStream;
				RINOK(GetInStream(inStreams, /* inSizes, */ startInIndex + i, &seqInStream));
				seqInStreams.Add(seqInStream);
			}
			for(i = 0; i < numOutStreams; i++) {
				CMyComPtr<ISequentialOutStream> seqOutStream;
				RINOK(GetOutStream(outStreams, /* outSizes, */ startOutIndex + i, &seqOutStream));
				seqOutStreams.Add(seqOutStream);
			}
			CRecordVector <ISequentialInStream *> seqInStreamsSpec;
			CRecordVector <ISequentialOutStream *> seqOutStreamsSpec;
			for(i = 0; i < numInStreams; i++)
				seqInStreamsSpec.Add(seqInStreams[i]);
			for(i = 0; i < numOutStreams; i++)
				seqOutStreamsSpec.Add(seqOutStreams[i]);
			for(i = 0; i < _coders.Size(); i++) {
				if(i == ci)
					continue;
				CCoder & coder = _coders[i];
				if(EncodeMode) {
					CMyComPtr<ICompressInitEncoder> initEncoder;
					coder.QueryInterface(IID_ICompressInitEncoder, (void **)&initEncoder);
					if(initEncoder)
						RINOK(initEncoder->InitEncoder());
				}
				else {
					CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
					coder.QueryInterface(IID_ICompressSetOutStreamSize, (void **)&setOutStreamSize);
					if(setOutStreamSize)
						RINOK(setOutStreamSize->SetOutStreamSize(EncodeMode ? coder.PackSizePointers[0] : coder.UnpackSizePointer));
				}
			}
			const uint64 * const * isSizes2 = EncodeMode ? &mainCoder.UnpackSizePointer : &mainCoder.PackSizePointers.Front();
			const uint64 * const * outSizes2 = EncodeMode ? &mainCoder.PackSizePointers.Front() : &mainCoder.UnpackSizePointer;
			HRESULT res;
			if(mainCoder.Coder) {
				res = mainCoder.Coder->Code(seqInStreamsSpec[0], seqOutStreamsSpec[0], isSizes2[0], outSizes2[0], progress);
			}
			else {
				res = mainCoder.Coder2->Code(&seqInStreamsSpec.Front(), isSizes2, numInStreams, &seqOutStreamsSpec.Front(), outSizes2, numOutStreams, progress);
			}
			if(res == k_My_HRESULT_WritingWasCut)
				res = S_OK;
			if(res == S_OK || res == S_FALSE) {
				res = GetError(res, FinishCoder(ci));
			}
			for(i = 0; i < _binderStreams.Size(); i++) {
				const CStBinderStream & bs = _binderStreams[i];
				if(bs.InStreamSpec)
					bs.InStreamSpec->ReleaseStream();
				else
					bs.OutStreamSpec->ReleaseStream();
			}
			if(res == k_My_HRESULT_WritingWasCut)
				res = S_OK;
			if(res != S_OK)
				return res;
			for(i = 0; i < _coders.Size(); i++) {
				RINOK(_coders[i].CheckDataAfterEnd(dataAfterEnd_Error /*, InternalPackSizeError */));
			}
			return S_OK;
		}

		HRESULT CMixerST::GetMainUnpackStream(ISequentialInStream * const * inStreams, ISequentialInStream ** inStreamRes)
		{
			CMyComPtr<ISequentialInStream> seqInStream;
			RINOK(GetInStream2(inStreams, /* inSizes, */ _bi.UnpackCoder, &seqInStream))
			FOR_VECTOR(i, _coders) {
				CCoder &coder = _coders[i];
				CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
				coder.QueryInterface(IID_ICompressSetOutStreamSize, (void **)&setOutStreamSize);
				if(setOutStreamSize) {
					RINOK(setOutStreamSize->SetOutStreamSize(coder.UnpackSizePointer));
				}
			}

			*inStreamRes = seqInStream.Detach();
			return S_OK;
		}

		uint64 CMixerST::GetBondStreamSize(unsigned bondIndex) const
		{
			const CStBinderStream &bs = _binderStreams[bondIndex];
			if(bs.InStreamSpec)
				return bs.InStreamSpec->GetSize();
			return bs.OutStreamSpec->GetSize();
		}
	#endif // } USE_MIXER_ST
	#ifdef USE_MIXER_MT // {
		CCoderMT::CCoderMT() : EncodeMode(false) 
		{
		}

		CCoderMT::~CCoderMT() 
		{
			CVirtThread::WaitThreadFinish();
		}

		void CCoderMT::Execute()
		{
			try {
				Code(NULL);
			}
			catch(...) {
				Result = E_FAIL;
			}
		}

		void CCoderMT::Release()
		{
			InStreamPointers.Clear();
			OutStreamPointers.Clear();
			uint   i;
			for(i = 0; i < InStreams.Size(); i++)
				InStreams[i].Release();
			for(i = 0; i < OutStreams.Size(); i++)
				OutStreams[i].Release();
		}

		void CCoderMT::Code(ICompressProgressInfo * progress)
		{
			uint   numInStreams = EncodeMode ? 1 : NumStreams;
			uint   numOutStreams = EncodeMode ? NumStreams : 1;
			InStreamPointers.ClearAndReserve(numInStreams);
			OutStreamPointers.ClearAndReserve(numOutStreams);
			uint i;
			for(i = 0; i < numInStreams; i++)
				InStreamPointers.AddInReserved((ISequentialInStream*)InStreams[i]);
			for(i = 0; i < numOutStreams; i++)
				OutStreamPointers.AddInReserved((ISequentialOutStream*)OutStreams[i]);
			// we suppose that UnpackSizePointer and PackSizePointers contain correct pointers.
			/*
			   if(UnpackSizePointer)
			   UnpackSizePointer = &UnpackSize;
			   for(i = 0; i < NumStreams; i++)
			   if(PackSizePointers[i])
				PackSizePointers[i] = &PackSizes[i];
			 */
			CReleaser releaser(*this);
			if(Coder)
				Result = Coder->Code(InStreamPointers[0], OutStreamPointers[0], EncodeMode ? UnpackSizePointer : PackSizePointers[0],
					EncodeMode ? PackSizePointers[0] : UnpackSizePointer, progress);
			else
				Result = Coder2->Code(&InStreamPointers.Front(),  EncodeMode ? &UnpackSizePointer : &PackSizePointers.Front(), numInStreams,
					&OutStreamPointers.Front(), EncodeMode ? &PackSizePointers.Front() : &UnpackSizePointer, numOutStreams, progress);
		}

		HRESULT CMixerMT::SetBindInfo(const CBindInfo &bindInfo)
		{
			CMixer::SetBindInfo(bindInfo);
			_streamBinders.Clear();
			FOR_VECTOR(i, _bi.Bonds) {
				RINOK(_streamBinders.AddNew().CreateEvents());
			}
			return S_OK;
		}

		void CMixerMT::AddCoder(const CCreatedCoder &cod)
		{
			IsFilter_Vector.Add(cod.IsFilter);
			IsExternal_Vector.Add(cod.IsExternal);
			// const CCoderStreamsInfo &c = _bi.Coders[_coders.Size()];
			CCoderMT &c2 = _coders.AddNew();
			c2.NumStreams = cod.NumStreams;
			c2.Coder = cod.Coder;
			c2.Coder2 = cod.Coder2;
			c2.EncodeMode = EncodeMode;
		}

		CCoder & CMixerMT::GetCoder(uint index) { return _coders[index]; }

		void CMixerMT::ReInit()
		{
			FOR_VECTOR(i, _streamBinders) {
				_streamBinders[i].ReInit();
			}
		}

		void CMixerMT::SelectMainCoder(bool useFirst)
		{
			uint   ci = _bi.UnpackCoder;
			if(!useFirst)
				for(;;) {
					if(_coders[ci].NumStreams != 1)
						break;
					if(!IsFilter_Vector[ci])
						break;
					uint32 st = _bi.Coder_to_Stream[ci];
					if(_bi.IsStream_in_PackStreams(st))
						break;
					int bond = _bi.FindBond_for_PackStream(st);
					if(bond < 0)
						throw 20150213;
					ci = _bi.Bonds[bond].UnpackIndex;
				}
			MainCoderIndex = ci;
		}

		HRESULT CMixerMT::Init(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams)
		{
			uint i;
			for(i = 0; i < _coders.Size(); i++) {
				CCoderMT &coderInfo = _coders[i];
				const CCoderStreamsInfo &csi = _bi.Coders[i];
				uint32 j;
				unsigned numInStreams = EncodeMode ? 1 : csi.NumStreams;
				unsigned numOutStreams = EncodeMode ? csi.NumStreams : 1;
				coderInfo.InStreams.Clear();
				for(j = 0; j < numInStreams; j++)
					coderInfo.InStreams.AddNew();
				coderInfo.OutStreams.Clear();
				for(j = 0; j < numOutStreams; j++)
					coderInfo.OutStreams.AddNew();
			}
			for(i = 0; i < _bi.Bonds.Size(); i++) {
				const CBond &bond = _bi.Bonds[i];
				uint32 inCoderIndex, inCoderStreamIndex;
				uint32 outCoderIndex, outCoderStreamIndex;
				{
					uint32 coderIndex, coderStreamIndex;
					_bi.GetCoder_for_Stream(bond.PackIndex, coderIndex, coderStreamIndex);
					inCoderIndex = EncodeMode ? bond.UnpackIndex : coderIndex;
					outCoderIndex = EncodeMode ? coderIndex : bond.UnpackIndex;
					inCoderStreamIndex = EncodeMode ? 0 : coderStreamIndex;
					outCoderStreamIndex = EncodeMode ? coderStreamIndex : 0;
				}
				_streamBinders[i].CreateStreams(&_coders[inCoderIndex].InStreams[inCoderStreamIndex], &_coders[outCoderIndex].OutStreams[outCoderStreamIndex]);
				CMyComPtr<ICompressSetBufSize> inSetSize, outSetSize;
				_coders[inCoderIndex].QueryInterface(IID_ICompressSetBufSize, (void **)&inSetSize);
				_coders[outCoderIndex].QueryInterface(IID_ICompressSetBufSize, (void **)&outSetSize);
				if(inSetSize && outSetSize) {
					const uint32 kBufSize = 1 << 19;
					inSetSize->SetInBufSize(inCoderStreamIndex, kBufSize);
					outSetSize->SetOutBufSize(outCoderStreamIndex, kBufSize);
				}
			}
			{
				CCoderMT &cod = _coders[_bi.UnpackCoder];
				if(EncodeMode)
					cod.InStreams[0] = inStreams[0];
				else
					cod.OutStreams[0] = outStreams[0];
			}
			for(i = 0; i < _bi.PackStreams.Size(); i++) {
				uint32 coderIndex, coderStreamIndex;
				_bi.GetCoder_for_Stream(_bi.PackStreams[i], coderIndex, coderStreamIndex);
				CCoderMT &cod = _coders[coderIndex];
				if(EncodeMode)
					cod.OutStreams[coderStreamIndex] = outStreams[i];
				else
					cod.InStreams[coderStreamIndex] = inStreams[i];
			}
			return S_OK;
		}

		HRESULT CMixerMT::ReturnIfError(HRESULT code)
		{
			FOR_VECTOR(i, _coders) {
				if(_coders[i].Result == code)
					return code;
			}
			return S_OK;
		}

		HRESULT CMixerMT::Code(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams, ICompressProgressInfo * progress, bool &dataAfterEnd_Error)
		{
			// InternalPackSizeError = false;
			dataAfterEnd_Error = false;
			Init(inStreams, outStreams);
			uint i;
			for(i = 0; i < _coders.Size(); i++)
				if(i != MainCoderIndex) {
					RINOK(_coders[i].Create());
				}
			for(i = 0; i < _coders.Size(); i++)
				if(i != MainCoderIndex)
					_coders[i].Start();
			_coders[MainCoderIndex].Code(progress);
			for(i = 0; i < _coders.Size(); i++)
				if(i != MainCoderIndex)
					_coders[i].WaitExecuteFinish();
			RINOK(ReturnIfError(E_ABORT));
			RINOK(ReturnIfError(E_OUTOFMEMORY));
			for(i = 0; i < _coders.Size(); i++) {
				HRESULT result = _coders[i].Result;
				if(result != S_OK && result != k_My_HRESULT_WritingWasCut && result != S_FALSE && result != E_FAIL)
					return result;
			}
			RINOK(ReturnIfError(S_FALSE));
			for(i = 0; i < _coders.Size(); i++) {
				HRESULT result = _coders[i].Result;
				if(result != S_OK && result != k_My_HRESULT_WritingWasCut)
					return result;
			}
			for(i = 0; i < _coders.Size(); i++) {
				RINOK(_coders[i].CheckDataAfterEnd(dataAfterEnd_Error /* , InternalPackSizeError */));
			}
			return S_OK;
		}

		uint64 CMixerMT::GetBondStreamSize(unsigned bondIndex) const
		{
			return _streamBinders[bondIndex].ProcessedSize;
		}
	#endif // } USE_MIXER_MT
}
