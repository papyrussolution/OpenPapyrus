// 7Z-7Z.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

// define FORMAT_7Z_RECOVERY if you want to recover multivolume archives with empty StartHeader
#ifndef _SFX
	#define FORMAT_7Z_RECOVERY
#endif

using namespace NWindows;
using namespace NCOM;

// 7zEncode.cpp 7zDecode.cpp 7zExtract.cpp 7zFolderInStream.cpp 7zIn.cpp 7zOut.cpp 7zProperties.cpp 7zUpdate.cpp 7zHandler.cpp 7zHandlerOut.cpp
class CDynBufSeqOutStream : public ISequentialOutStream, public CMyUnknownImp {
public:
	CDynBufSeqOutStream() : _size(0) 
	{
	}
	void Init() 
	{
		_size = 0;
	}
	size_t GetSize() const { return _size; }
	const Byte * GetBuffer() const { return _buffer; }
	void CopyToBuffer(CByteBuffer &dest) const;
	Byte * GetBufPtrForWriting(size_t addSize);
	void UpdateSize(size_t addSize) 
	{
		_size += addSize;
	}
	MY_UNKNOWN_IMP1(ISequentialOutStream)
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
private:
	class CByteDynBuffer {
	public:
		// there is no copy constructor. So don't copy this object.
		CByteDynBuffer();
		~CByteDynBuffer();
		void Free() throw();
		size_t GetCapacity() const { return _capacity; }
		operator Byte*() const { return _buf; }
		operator const Byte*() const { return _buf; }
		bool EnsureCapacity(size_t capacity) throw();
	private:
		size_t _capacity;
		Byte * _buf;
	};
	CByteDynBuffer _buffer;
	size_t _size;
};

CDynBufSeqOutStream::CByteDynBuffer::CByteDynBuffer() : _capacity(0), _buf(0) 
{
}

CDynBufSeqOutStream::CByteDynBuffer::~CByteDynBuffer() 
{
	Free();
}

void CDynBufSeqOutStream::CByteDynBuffer::Free() throw()
{
	ZFREE(_buf);
	_capacity = 0;
}

bool CDynBufSeqOutStream::CByteDynBuffer::EnsureCapacity(size_t cap) throw()
{
	if(cap > _capacity) {
		size_t delta;
		if(_capacity > 64)
			delta = _capacity / 4;
		else if(_capacity > 8)
			delta = 16;
		else
			delta = 4;
		cap = MyMax(_capacity + delta, cap);
		Byte * buf = (Byte*)SAlloc::R(_buf, cap);
		if(!buf)
			return false;
		_buf = buf;
		_capacity = cap;
	}
	return true;
}

Byte * CDynBufSeqOutStream::GetBufPtrForWriting(size_t addSize)
{
	addSize += _size;
	return (addSize < _size || !_buffer.EnsureCapacity(addSize)) ? NULL : ((Byte*)_buffer + _size);
}

void CDynBufSeqOutStream::CopyToBuffer(CByteBuffer &dest) const
{
	dest.CopyFrom((const Byte*)_buffer, _size);
}

STDMETHODIMP CDynBufSeqOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	Byte * buf = GetBufPtrForWriting(size);
	if(!buf)
		return E_OUTOFMEMORY;
	memcpy(buf, data, size);
	UpdateSize(size);
	ASSIGN_PTR(processedSize, size);
	return S_OK;
}
//
//
//
class CSequentialOutStreamSizeCount : public ISequentialOutStream, public CMyUnknownImp {
public:
	void SetStream(ISequentialOutStream * stream) 
	{
		_stream = stream;
	}
	void Init() 
	{
		_size = 0;
	}
	uint64 GetSize() const { return _size; }
	MY_UNKNOWN_IMP1(ISequentialOutStream)
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
private:
	CMyComPtr<ISequentialOutStream> _stream;
	uint64 _size;
};

STDMETHODIMP CSequentialOutStreamSizeCount::Write(const void * data, uint32 size, uint32 * processedSize)
{
	uint32 realProcessedSize;
	HRESULT result = _stream->Write(data, size, &realProcessedSize);
	_size += realProcessedSize;
	ASSIGN_PTR(processedSize, realProcessedSize);
	return result;
}

namespace NArchive {
	namespace N7z {
		//
		// Encode
		//
		void CEncoder::InitBindConv()
		{
			unsigned numIn = _bindInfo.Coders.Size();
			_SrcIn_to_DestOut.ClearAndSetSize(numIn);
			_DestOut_to_SrcIn.ClearAndSetSize(numIn);
			unsigned numOut = _bindInfo.GetNum_Bonds_and_PackStreams();
			_SrcOut_to_DestIn.ClearAndSetSize(numOut);
			// _DestIn_to_SrcOut.ClearAndSetSize(numOut);
			uint32 destIn = 0;
			uint32 destOut = 0;
			for(uint i = _bindInfo.Coders.Size(); i != 0; ) {
				i--;
				const NCoderMixer2::CCoderStreamsInfo &coder = _bindInfo.Coders[i];
				numIn--;
				numOut -= coder.NumStreams;
				_SrcIn_to_DestOut[numIn] = destOut;
				_DestOut_to_SrcIn[destOut] = numIn;
				destOut++;
				for(uint32 j = 0; j < coder.NumStreams; j++, destIn++) {
					uint32 index = numOut + j;
					_SrcOut_to_DestIn[index] = destIn;
					// _DestIn_to_SrcOut[destIn] = index;
				}
			}
		}
		void CEncoder::SetFolder(CFolder &folder)
		{
			folder.Bonds.SetSize(_bindInfo.Bonds.Size());
			uint i;
			for(i = 0; i < _bindInfo.Bonds.Size(); i++) {
				CBond &fb = folder.Bonds[i];
				const NCoderMixer2::CBond &mixerBond = _bindInfo.Bonds[_bindInfo.Bonds.Size() - 1 - i];
				fb.PackIndex = _SrcOut_to_DestIn[mixerBond.PackIndex];
				fb.UnpackIndex = _SrcIn_to_DestOut[mixerBond.UnpackIndex];
			}
			folder.Coders.SetSize(_bindInfo.Coders.Size());
			for(i = 0; i < _bindInfo.Coders.Size(); i++) {
				CCoderInfo &coderInfo = folder.Coders[i];
				const NCoderMixer2::CCoderStreamsInfo &coderStreamsInfo = _bindInfo.Coders[_bindInfo.Coders.Size() - 1 - i];
				coderInfo.NumStreams = coderStreamsInfo.NumStreams;
				coderInfo.MethodID = _decompressionMethods[i];
				// we don't free coderInfo.Props here. So coderInfo.Props can be non-empty.
			}
			folder.PackStreams.SetSize(_bindInfo.PackStreams.Size());
			for(i = 0; i < _bindInfo.PackStreams.Size(); i++)
				folder.PackStreams[i] = _SrcOut_to_DestIn[_bindInfo.PackStreams[i]];
		}
		static HRESULT SetCoderProps2(const CProps &props, const uint64 * dataSizeReduce, IUnknown * coder)
		{
			CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
			coder->QueryInterface(IID_ICompressSetCoderProperties, (void**)&setCoderProperties);
			return setCoderProperties ? props.SetCoderProps(setCoderProperties, dataSizeReduce) : (props.AreThereNonOptionalProps() ? E_INVALIDARG : S_OK);
		}
		void CMtEncMultiProgress::Init(ICompressProgressInfo * progress)
		{
			_progress = progress;
			OutSize = 0;
		}

		STDMETHODIMP CMtEncMultiProgress::SetRatioInfo(const uint64 * inSize, const uint64 * /* outSize */)
		{
			uint64 outSize2;
			{
			#ifndef _7ZIP_ST
				NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
			#endif
				outSize2 = OutSize;
			}
			return _progress ? _progress->SetRatioInfo(inSize, &outSize2) : S_OK;
		}

		HRESULT CEncoder::CreateMixerCoder(DECL_EXTERNAL_CODECS_LOC_VARS const uint64 * inSizeForReduce)
		{
		  #ifdef USE_MIXER_MT
		  #ifdef USE_MIXER_ST
			if(_options.MultiThreadMixer)
		  #endif
			{
				_mixerMT = new NCoderMixer2::CMixerMT(true);
				_mixerRef = _mixerMT;
				_mixer = _mixerMT;
			}
		  #ifdef USE_MIXER_ST
			else
		  #endif
		  #endif
			{
			#ifdef USE_MIXER_ST
				_mixerST = new NCoderMixer2::CMixerST(true);
				_mixerRef = _mixerST;
				_mixer = _mixerST;
			#endif
			}

			RINOK(_mixer->SetBindInfo(_bindInfo));

			FOR_VECTOR(m, _options.Methods) {
				const CMethodFull &methodFull = _options.Methods[m];
				CCreatedCoder cod;
				RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS methodFull.Id, true, cod));
				if(cod.NumStreams != methodFull.NumStreams)
					return E_FAIL;
				if(!cod.Coder && !cod.Coder2)
					return E_FAIL;
				CMyComPtr<IUnknown> encoderCommon = cod.Coder ? (IUnknown*)cod.Coder : (IUnknown*)cod.Coder2;
			#ifndef _7ZIP_ST
				{
					CMyComPtr<ICompressSetCoderMt> setCoderMt;
					encoderCommon.QueryInterface(IID_ICompressSetCoderMt, &setCoderMt);
					if(setCoderMt) {
						RINOK(setCoderMt->SetNumberOfThreads(_options.NumThreads));
					}
				}
			#endif

				RINOK(SetCoderProps2(methodFull, inSizeForReduce, encoderCommon));

				/*
				   CMyComPtr<ICryptoResetSalt> resetSalt;
				   encoderCommon.QueryInterface(IID_ICryptoResetSalt, (void **)&resetSalt);
				   if(resetSalt)
				   {
				   resetSalt->ResetSalt();
				   }
				 */

				// now there is no codec that uses another external codec
				/*
				   #ifdef EXTERNAL_CODECS
				   CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
				   encoderCommon.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
				   if(setCompressCodecsInfo)
				   {
				   // we must use g_ExternalCodecs also
				   RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(__externalCodecs->GetCodecs));
				   }
				   #endif
				 */
				CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
				encoderCommon.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);
				if(cryptoSetPassword) {
					const uint sizeInBytes = _options.Password.Len() * 2;
					CByteBuffer buffer(sizeInBytes);
					for(uint i = 0; i < _options.Password.Len(); i++) {
						wchar_t c = _options.Password[i];
						((Byte*)buffer)[i * 2] = (Byte)c;
						((Byte*)buffer)[i * 2 + 1] = (Byte)(c >> 8);
					}
					RINOK(cryptoSetPassword->CryptoSetPassword((const Byte*)buffer, (uint32)sizeInBytes));
				}
				_mixer->AddCoder(cod);
			}
			return S_OK;
		}

		class CSequentialOutTempBufferImp2 : public ISequentialOutStream, public CMyUnknownImp {
			CInOutTempBuffer * _buf;
		public:
			CMtEncMultiProgress * _mtProgresSpec;
			CSequentialOutTempBufferImp2() : _buf(0), _mtProgresSpec(NULL) 
			{
			}
			void Init(CInOutTempBuffer * buffer) { _buf = buffer; }
			MY_UNKNOWN_IMP1(ISequentialOutStream)
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CSequentialOutTempBufferImp2::Write(const void * data, uint32 size, uint32 * processed)
		{
			if(!_buf->Write(data, size)) {
				ASSIGN_PTR(processed, 0);
				return E_FAIL;
			}
			else {
				ASSIGN_PTR(processed, size);
				CALLPTRMEMB(_mtProgresSpec, AddOutSize(size));
				return S_OK;
			}
		}

		class CSequentialOutMtNotify : public ISequentialOutStream, public CMyUnknownImp {
		public:
			CSequentialOutMtNotify() : _mtProgresSpec(NULL) 
			{
			}
			MY_UNKNOWN_IMP1(ISequentialOutStream)
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);

			CMyComPtr<ISequentialOutStream> _stream;
			CMtEncMultiProgress * _mtProgresSpec;
		};

		STDMETHODIMP CSequentialOutMtNotify::Write(const void * data, uint32 size, uint32 * processed)
		{
			uint32 realProcessed = 0;
			HRESULT res = _stream->Write(data, size, &realProcessed);
			ASSIGN_PTR(processed, realProcessed);
			CALLPTRMEMB(_mtProgresSpec, AddOutSize(size));
			return res;
		}
		HRESULT CEncoder::Encode(DECL_EXTERNAL_CODECS_LOC_VARS ISequentialInStream * inStream, /*const uint64 *inStreamSize,*/
			const uint64 * inSizeForReduce, CFolder &folderItem, CRecordVector<uint64> &coderUnpackSizes, uint64 &unpackSize,
			ISequentialOutStream * outStream, CRecordVector<uint64> &packSizes, ICompressProgressInfo * compressProgress)
		{
			RINOK(EncoderConstr());
			if(!_mixerRef) {
				RINOK(CreateMixerCoder(EXTERNAL_CODECS_LOC_VARS inSizeForReduce));
			}
			_mixer->ReInit();
			CMtEncMultiProgress * mtProgressSpec = NULL;
			CMyComPtr<ICompressProgressInfo> mtProgress;
			CSequentialOutMtNotify * mtOutStreamNotifySpec = NULL;
			CMyComPtr<ISequentialOutStream> mtOutStreamNotify;
			CObjectVector<CInOutTempBuffer> inOutTempBuffers;
			CObjectVector<CSequentialOutTempBufferImp2 *> tempBufferSpecs;
			CObjectVector<CMyComPtr<ISequentialOutStream> > tempBuffers;
			uint   numMethods = _bindInfo.Coders.Size();
			uint   i;
			for(i = 1; i < _bindInfo.PackStreams.Size(); i++) {
				CInOutTempBuffer &iotb = inOutTempBuffers.AddNew();
				iotb.Create();
				iotb.InitWriting();
			}
			for(i = 1; i < _bindInfo.PackStreams.Size(); i++) {
				CSequentialOutTempBufferImp2 * tempBufferSpec = new CSequentialOutTempBufferImp2;
				CMyComPtr<ISequentialOutStream> tempBuffer = tempBufferSpec;
				tempBufferSpec->Init(&inOutTempBuffers[i - 1]);
				tempBuffers.Add(tempBuffer);
				tempBufferSpecs.Add(tempBufferSpec);
			}
			for(i = 0; i < numMethods; i++)
				_mixer->SetCoderInfo(i, NULL, NULL, false);

			/* inStreamSize can be used by BCJ2 to set optimal range of conversion.
			   But current BCJ2 encoder uses also another way to check exact size of current file.
			   So inStreamSize is not required. */

			/*
			   if(inStreamSize)
			   _mixer->SetCoderInfo(_bindInfo.UnpackCoder, inStreamSize, NULL);
			 */
			CSequentialInStreamSizeCount2 * inStreamSizeCountSpec = new CSequentialInStreamSizeCount2;
			CMyComPtr<ISequentialInStream> inStreamSizeCount = inStreamSizeCountSpec;
			CSequentialOutStreamSizeCount * outStreamSizeCountSpec = NULL;
			CMyComPtr<ISequentialOutStream> outStreamSizeCount;
			inStreamSizeCountSpec->Init(inStream);
			ISequentialInStream * inStreamPointer = inStreamSizeCount;
			CRecordVector<ISequentialOutStream *> outStreamPointers;
			SetFolder(folderItem);
			for(i = 0; i < numMethods; i++) {
				IUnknown * coder = _mixer->GetCoder(i).GetUnknown();
				CMyComPtr<ICryptoResetInitVector> resetInitVector;
				coder->QueryInterface(IID_ICryptoResetInitVector, (void**)&resetInitVector);
				CALLPTRMEMB(resetInitVector, ResetInitVector());
				CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
				coder->QueryInterface(IID_ICompressWriteCoderProperties, (void**)&writeCoderProperties);
				CByteBuffer &props = folderItem.Coders[numMethods - 1 - i].Props;
				if(writeCoderProperties) {
					CDynBufSeqOutStream * outStreamSpec = new CDynBufSeqOutStream;
					CMyComPtr<ISequentialOutStream> dynOutStream(outStreamSpec);
					outStreamSpec->Init();
					writeCoderProperties->WriteCoderProperties(dynOutStream);
					outStreamSpec->CopyToBuffer(props);
				}
				else
					props.Free();
			}
			_mixer->SelectMainCoder(false);
			uint32 mainCoder = _mixer->MainCoderIndex;
			bool useMtProgress = false;
			if(!_mixer->Is_PackSize_Correct_for_Coder(mainCoder)) {
			#ifdef _7ZIP_ST
				if(!_mixer->IsThere_ExternalCoder_in_PackTree(mainCoder))
			#endif
				useMtProgress = true;
			}
			if(useMtProgress) {
				mtProgressSpec = new CMtEncMultiProgress;
				mtProgress = mtProgressSpec;
				mtProgressSpec->Init(compressProgress);
				mtOutStreamNotifySpec = new CSequentialOutMtNotify;
				mtOutStreamNotify = mtOutStreamNotifySpec;
				mtOutStreamNotifySpec->_stream = outStream;
				mtOutStreamNotifySpec->_mtProgresSpec = mtProgressSpec;
				FOR_VECTOR(t, tempBufferSpecs) {
					tempBufferSpecs[t]->_mtProgresSpec = mtProgressSpec;
				}
			}
			if(_bindInfo.PackStreams.Size() != 0) {
				outStreamSizeCountSpec = new CSequentialOutStreamSizeCount;
				outStreamSizeCount = outStreamSizeCountSpec;
				outStreamSizeCountSpec->SetStream(mtOutStreamNotify ? (ISequentialOutStream*)mtOutStreamNotify : outStream);
				outStreamSizeCountSpec->Init();
				outStreamPointers.Add(outStreamSizeCount);
			}
			for(i = 1; i < _bindInfo.PackStreams.Size(); i++)
				outStreamPointers.Add(tempBuffers[i - 1]);
			bool dataAfterEnd_Error;
			RINOK(_mixer->Code(&inStreamPointer, &outStreamPointers.Front(),
				mtProgress ? (ICompressProgressInfo*)mtProgress : compressProgress, dataAfterEnd_Error));
			if(_bindInfo.PackStreams.Size() != 0)
				packSizes.Add(outStreamSizeCountSpec->GetSize());
			for(i = 1; i < _bindInfo.PackStreams.Size(); i++) {
				CInOutTempBuffer &inOutTempBuffer = inOutTempBuffers[i - 1];
				RINOK(inOutTempBuffer.WriteToStream(outStream));
				packSizes.Add(inOutTempBuffer.GetDataSize());
			}
			unpackSize = 0;
			for(i = 0; i < _bindInfo.Coders.Size(); i++) {
				int bond = _bindInfo.FindBond_for_UnpackStream(_DestOut_to_SrcIn[i]);
				uint64 streamSize;
				if(bond < 0) {
					streamSize = inStreamSizeCountSpec->GetSize();
					unpackSize = streamSize;
				}
				else
					streamSize = _mixer->GetBondStreamSize(bond);
				coderUnpackSizes.Add(streamSize);
			}
			return S_OK;
		}

		CEncoder::CEncoder(const CCompressionMethodMode &options) :  _constructed(false)
		{
			if(options.IsEmpty())
				throw 1;
			_options = options;
		  #ifdef USE_MIXER_ST
			_mixerST = NULL;
		  #endif
		  #ifdef USE_MIXER_MT
			_mixerMT = NULL;
		  #endif
			_mixer = NULL;
		}
		HRESULT CEncoder::EncoderConstr()
		{
			if(_constructed)
				return S_OK;
			if(_options.Methods.IsEmpty()) {
				// it has only password method;
				if(!_options.PasswordIsDefined)
					throw 1;
				if(!_options.Bonds.IsEmpty())
					throw 1;
				CMethodFull method;
				method.Id = k_AES;
				method.NumStreams = 1;
				_options.Methods.Add(method);
				NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
				coderStreamsInfo.NumStreams = 1;
				_bindInfo.Coders.Add(coderStreamsInfo);
				_bindInfo.PackStreams.Add(0);
				_bindInfo.UnpackCoder = 0;
			}
			else {
				uint32 numOutStreams = 0;
				uint   i;
				for(i = 0; i < _options.Methods.Size(); i++) {
					const CMethodFull &methodFull = _options.Methods[i];
					NCoderMixer2::CCoderStreamsInfo cod;
					cod.NumStreams = methodFull.NumStreams;
					if(_options.Bonds.IsEmpty()) {
						// if there are no bonds in options, we create bonds via first streams of coders
						if(i != _options.Methods.Size() - 1) {
							NCoderMixer2::CBond bond;
							bond.PackIndex = numOutStreams;
							bond.UnpackIndex = i + 1; // it's next coder
							_bindInfo.Bonds.Add(bond);
						}
						else if(cod.NumStreams != 0)
							_bindInfo.PackStreams.Insert(0, numOutStreams);
						for(uint32 j = 1; j < cod.NumStreams; j++)
							_bindInfo.PackStreams.Add(numOutStreams + j);
					}
					numOutStreams += cod.NumStreams;
					_bindInfo.Coders.Add(cod);
				}
				if(!_options.Bonds.IsEmpty()) {
					for(i = 0; i < _options.Bonds.Size(); i++) {
						NCoderMixer2::CBond mixerBond;
						const CBond2 &bond = _options.Bonds[i];
						if(bond.InCoder >= _bindInfo.Coders.Size() || bond.OutCoder >= _bindInfo.Coders.Size() || bond.OutStream >= _bindInfo.Coders[bond.OutCoder].NumStreams)
							return E_INVALIDARG;
						mixerBond.PackIndex = _bindInfo.GetStream_for_Coder(bond.OutCoder) + bond.OutStream;
						mixerBond.UnpackIndex = bond.InCoder;
						_bindInfo.Bonds.Add(mixerBond);
					}
					for(i = 0; i < numOutStreams; i++)
						if(_bindInfo.FindBond_for_PackStream(i) == -1)
							_bindInfo.PackStreams.Add(i);
				}
				if(!_bindInfo.SetUnpackCoder())
					return E_INVALIDARG;
				if(!_bindInfo.CalcMapsAndCheck())
					return E_INVALIDARG;
				if(_bindInfo.PackStreams.Size() != 1) {
					/* main_PackStream is pack stream of main path of coders tree.
					   We find main_PackStream, and place to start of list of out streams.
					   It allows to use more optimal memory usage for temp buffers,
					   if main_PackStream is largest stream. */
					uint32 ci = _bindInfo.UnpackCoder;
					for(;; ) {
						if(_bindInfo.Coders[ci].NumStreams == 0)
							break;
						uint32 outIndex = _bindInfo.Coder_to_Stream[ci];
						int bond = _bindInfo.FindBond_for_PackStream(outIndex);
						if(bond >= 0) {
							ci = _bindInfo.Bonds[bond].UnpackIndex;
							continue;
						}
						int si = _bindInfo.FindStream_in_PackStreams(outIndex);
						if(si >= 0)
							_bindInfo.PackStreams.MoveToFront(si);
						break;
					}
				}
				if(_options.PasswordIsDefined) {
					unsigned numCryptoStreams = _bindInfo.PackStreams.Size();
					unsigned numInStreams = _bindInfo.Coders.Size();
					for(i = 0; i < numCryptoStreams; i++) {
						NCoderMixer2::CBond bond;
						bond.UnpackIndex = numInStreams + i;
						bond.PackIndex = _bindInfo.PackStreams[i];
						_bindInfo.Bonds.Add(bond);
					}
					_bindInfo.PackStreams.Clear();
					/*
					   if(numCryptoStreams == 0)
					   numCryptoStreams = 1;
					 */
					for(i = 0; i < numCryptoStreams; i++) {
						CMethodFull method;
						method.NumStreams = 1;
						method.Id = k_AES;
						_options.Methods.Add(method);
						NCoderMixer2::CCoderStreamsInfo cod;
						cod.NumStreams = 1;
						_bindInfo.Coders.Add(cod);
						_bindInfo.PackStreams.Add(numOutStreams++);
					}
				}
			}
			for(uint i = _options.Methods.Size(); i != 0; )
				_decompressionMethods.Add(_options.Methods[--i].Id);
			if(_bindInfo.Coders.Size() > 16)
				return E_INVALIDARG;
			if(_bindInfo.GetNum_Bonds_and_PackStreams() > 16)
				return E_INVALIDARG;
			if(!_bindInfo.CalcMapsAndCheck())
				return E_INVALIDARG;
			InitBindConv();
			_constructed = true;
			return S_OK;
		}
		CEncoder::~CEncoder() 
		{
		}
		//
		// Decode
		//
		class CDecProgress : public ICompressProgressInfo, public CMyUnknownImp {
			CMyComPtr<ICompressProgressInfo> _progress;
		public:
			CDecProgress(ICompressProgressInfo * progress) : _progress(progress) 
			{
			}
			MY_UNKNOWN_IMP1(ICompressProgressInfo)
			STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
		};

		STDMETHODIMP CDecProgress::SetRatioInfo(const uint64 * /* inSize */, const uint64 * outSize)
		{
			return _progress->SetRatioInfo(NULL, outSize);
		}

		static void Convert_FolderInfo_to_BindInfo(const CFolderEx &folder, CBindInfoEx &bi)
		{
			bi.Clear();
			bi.Bonds.ClearAndSetSize(folder.Bonds.Size());
			uint i;
			for(i = 0; i < folder.Bonds.Size(); i++) {
				NCoderMixer2::CBond &bond = bi.Bonds[i];
				const N7z::CBond &folderBond = folder.Bonds[i];
				bond.PackIndex = folderBond.PackIndex;
				bond.UnpackIndex = folderBond.UnpackIndex;
			}
			bi.Coders.ClearAndSetSize(folder.Coders.Size());
			bi.CoderMethodIDs.ClearAndSetSize(folder.Coders.Size());
			for(i = 0; i < folder.Coders.Size(); i++) {
				const CCoderInfo &coderInfo = folder.Coders[i];
				bi.Coders[i].NumStreams = coderInfo.NumStreams;
				bi.CoderMethodIDs[i] = coderInfo.MethodID;
			}
			/*
			   if(!bi.SetUnpackCoder())
			   throw 1112;
			 */
			bi.UnpackCoder = folder.UnpackCoder;
			bi.PackStreams.ClearAndSetSize(folder.PackStreams.Size());
			for(i = 0; i < folder.PackStreams.Size(); i++)
				bi.PackStreams[i] = folder.PackStreams[i];
		}

		static inline bool AreCodersEqual(const NCoderMixer2::CCoderStreamsInfo &a1, const NCoderMixer2::CCoderStreamsInfo &a2)
			{ return (a1.NumStreams == a2.NumStreams); }
		static inline bool AreBondsEqual(const NCoderMixer2::CBond &a1, const NCoderMixer2::CBond &a2)
			{ return (a1.PackIndex == a2.PackIndex) && (a1.UnpackIndex == a2.UnpackIndex); }

		static bool AreBindInfoExEqual(const CBindInfoEx &a1, const CBindInfoEx &a2)
		{
			if(a1.Coders.Size() != a2.Coders.Size())
				return false;
			else {
				uint   i;
				for(i = 0; i < a1.Coders.Size(); i++)
					if(!AreCodersEqual(a1.Coders[i], a2.Coders[i]))
						return false;
				if(a1.Bonds.Size() != a2.Bonds.Size())
					return false;
				for(i = 0; i < a1.Bonds.Size(); i++)
					if(!AreBondsEqual(a1.Bonds[i], a2.Bonds[i]))
						return false;
				for(i = 0; i < a1.CoderMethodIDs.Size(); i++)
					if(a1.CoderMethodIDs[i] != a2.CoderMethodIDs[i])
						return false;
				if(a1.PackStreams.Size() != a2.PackStreams.Size())
					return false;
				for(i = 0; i < a1.PackStreams.Size(); i++)
					if(a1.PackStreams[i] != a2.PackStreams[i])
						return false;
				/*
				   if(a1.UnpackCoder != a2.UnpackCoder)
				   return false;
				 */
				return true;
			}
		}

		CDecoder::CDecoder(bool useMixerMT) : _bindInfoPrev_Defined(false), _useMixerMT(useMixerMT)
		{
		}

		struct CLockedInStream : public IUnknown, public CMyUnknownImp {
			CMyComPtr<IInStream> Stream;
			uint64 Pos;
			MY_UNKNOWN_IMP
		  #ifdef USE_MIXER_MT
			NWindows::NSynchronization::CCriticalSection CriticalSection;
		  #endif
		};

		#ifdef USE_MIXER_MT

		class CLockedSequentialInStreamMT : public ISequentialInStream, public CMyUnknownImp {
			CLockedInStream * _glob;
			uint64 _pos;
			CMyComPtr<IUnknown> _globRef;
		public:
			void Init(CLockedInStream * lockedInStream, uint64 startPos)
			{
				_globRef = lockedInStream;
				_glob = lockedInStream;
				_pos = startPos;
			}
			MY_UNKNOWN_IMP1(ISequentialInStream)
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CLockedSequentialInStreamMT::Read(void * data, uint32 size, uint32 * processedSize)
		{
			NWindows::NSynchronization::CCriticalSectionLock lock(_glob->CriticalSection);
			if(_pos != _glob->Pos) {
				RINOK(_glob->Stream->Seek(_pos, STREAM_SEEK_SET, NULL));
				_glob->Pos = _pos;
			}
			uint32 realProcessedSize = 0;
			HRESULT res = _glob->Stream->Read(data, size, &realProcessedSize);
			_pos += realProcessedSize;
			_glob->Pos = _pos;
			ASSIGN_PTR(processedSize, realProcessedSize);
			return res;
		}

		#endif

		#ifdef USE_MIXER_ST

		class CLockedSequentialInStreamST : public ISequentialInStream, public CMyUnknownImp {
			CLockedInStream * _glob;
			uint64 _pos;
			CMyComPtr<IUnknown> _globRef;
		public:
			void Init(CLockedInStream * lockedInStream, uint64 startPos)
			{
				_globRef = lockedInStream;
				_glob = lockedInStream;
				_pos = startPos;
			}

			MY_UNKNOWN_IMP1(ISequentialInStream)

			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CLockedSequentialInStreamST::Read(void * data, uint32 size, uint32 * processedSize)
		{
			if(_pos != _glob->Pos) {
				RINOK(_glob->Stream->Seek(_pos, STREAM_SEEK_SET, NULL));
				_glob->Pos = _pos;
			}
			uint32 realProcessedSize = 0;
			HRESULT res = _glob->Stream->Read(data, size, &realProcessedSize);
			_pos += realProcessedSize;
			_glob->Pos = _pos;
			ASSIGN_PTR(processedSize, realProcessedSize);
			return res;
		}

		#endif

		HRESULT CDecoder::Decode(DECL_EXTERNAL_CODECS_LOC_VARS IInStream * inStream, uint64 startPos, const CFolders &folders, unsigned folderIndex, 
			const uint64 * unpackSize, ISequentialOutStream * outStream, ICompressProgressInfo * compressProgress, ISequentialInStream **
			#ifdef USE_MIXER_ST
					inStreamMainRes
			#endif
				, bool &dataAfterEnd_Error
				_7Z_DECODER_CRYPRO_VARS_DECL
			#if !defined(_7ZIP_ST) && !defined(_SFX)
				, bool mtMode, uint32 numThreads
			#endif
		)
		{
			dataAfterEnd_Error = false;
			const uint64 * packPositions = &folders.PackPositions[folders.FoStartPackStreamIndex[folderIndex]];
			CFolderEx folderInfo;
			folders.ParseFolderEx(folderIndex, folderInfo);
			if(!folderInfo.IsDecodingSupported())
				return E_NOTIMPL;
			CBindInfoEx bindInfo;
			Convert_FolderInfo_to_BindInfo(folderInfo, bindInfo);
			if(!bindInfo.CalcMapsAndCheck())
				return E_NOTIMPL;
			uint64 folderUnpackSize = folders.GetFolderUnpackSize(folderIndex);
			bool fullUnpack = true;
			if(unpackSize) {
				if(*unpackSize > folderUnpackSize)
					return E_FAIL;
				fullUnpack = (*unpackSize == folderUnpackSize);
			}
			/*
			   We don't need to init isEncrypted and passwordIsDefined
			   We must upgrade them only

			   #ifndef _NO_CRYPTO
			   isEncrypted = false;
			   passwordIsDefined = false;
			   #endif
			 */
			if(!_bindInfoPrev_Defined || !AreBindInfoExEqual(bindInfo, _bindInfoPrev)) {
				_mixerRef.Release();
			#ifdef USE_MIXER_MT
			#ifdef USE_MIXER_ST
				if(_useMixerMT)
			#endif
				{
					_mixerMT = new NCoderMixer2::CMixerMT(false);
					_mixerRef = _mixerMT;
					_mixer = _mixerMT;
				}
			#ifdef USE_MIXER_ST
				else
			#endif
			#endif
				{
			  #ifdef USE_MIXER_ST
					_mixerST = new NCoderMixer2::CMixerST(false);
					_mixerRef = _mixerST;
					_mixer = _mixerST;
			  #endif
				}
				RINOK(_mixer->SetBindInfo(bindInfo));
				FOR_VECTOR(i, folderInfo.Coders) {
					const CCoderInfo &coderInfo = folderInfo.Coders[i];
			  #ifndef _SFX
					// we don't support RAR codecs here
					if((coderInfo.MethodID >> 8) == 0x403)
						return E_NOTIMPL;
			  #endif
					CCreatedCoder cod;
					RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS coderInfo.MethodID, false, cod));
					if(coderInfo.IsSimpleCoder()) {
						if(!cod.Coder)
							return E_NOTIMPL;
						// CMethodId m = coderInfo.MethodID;
						// isFilter = (IsFilterMethod(m) || m == k_AES);
					}
					else {
						if(!cod.Coder2 || cod.NumStreams != coderInfo.NumStreams)
							return E_NOTIMPL;
					}
					_mixer->AddCoder(cod);

					// now there is no codec that uses another external codec
					/*
					   #ifdef EXTERNAL_CODECS
					   CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
					   decoderUnknown.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
					   if(setCompressCodecsInfo)
					   {
					   // we must use g_ExternalCodecs also
					   RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(__externalCodecs->GetCodecs));
					   }
					   #endif
					 */
				}
				_bindInfoPrev = bindInfo;
				_bindInfoPrev_Defined = true;
			}
			_mixer->ReInit();
			uint32 packStreamIndex = 0;
			uint32 unpackStreamIndexStart = folders.FoToCoderUnpackSizes[folderIndex];
			uint i;
			for(i = 0; i < folderInfo.Coders.Size(); i++) {
				const CCoderInfo &coderInfo = folderInfo.Coders[i];
				IUnknown * decoder = _mixer->GetCoder(i).GetUnknown();
				{
					CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
					decoder->QueryInterface(IID_ICompressSetDecoderProperties2, (void**)&setDecoderProperties);
					if(setDecoderProperties) {
						const CByteBuffer &props = coderInfo.Props;
						size_t size = props.Size();
						if(size > 0xFFFFFFFF)
							return E_NOTIMPL;
						HRESULT res = setDecoderProperties->SetDecoderProperties2((const Byte*)props, (uint32)size);
						if(res == E_INVALIDARG)
							res = E_NOTIMPL;
						RINOK(res);
					}
				}

			#if !defined(_7ZIP_ST) && !defined(_SFX)
				if(mtMode) {
					CMyComPtr<ICompressSetCoderMt> setCoderMt;
					decoder->QueryInterface(IID_ICompressSetCoderMt, (void**)&setCoderMt);
					if(setCoderMt) {
						RINOK(setCoderMt->SetNumberOfThreads(numThreads));
					}
				}
			#endif
			#ifndef _NO_CRYPTO
				{
					CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
					decoder->QueryInterface(IID_ICryptoSetPassword, (void**)&cryptoSetPassword);
					if(cryptoSetPassword) {
						isEncrypted = true;
						if(!getTextPassword)
							return E_NOTIMPL;
						CMyComBSTR passwordBSTR;
						RINOK(getTextPassword->CryptoGetTextPassword(&passwordBSTR));
						passwordIsDefined = true;
						password.Empty();
						size_t len = 0;
						if(passwordBSTR) {
							password = passwordBSTR;
							len = password.Len();
						}
						CByteBuffer buffer(len * 2);
						for(size_t k = 0; k < len; k++) {
							wchar_t c = passwordBSTR[k];
							((Byte*)buffer)[k * 2] = (Byte)c;
							((Byte*)buffer)[k * 2 + 1] = (Byte)(c >> 8);
						}
						RINOK(cryptoSetPassword->CryptoSetPassword((const Byte*)buffer, (uint32)buffer.Size()));
					}
				}
			#endif
				bool finishMode = false;
				{
					CMyComPtr<ICompressSetFinishMode> setFinishMode;
					decoder->QueryInterface(IID_ICompressSetFinishMode, (void**)&setFinishMode);
					if(setFinishMode) {
						finishMode = fullUnpack;
						RINOK(setFinishMode->SetFinishMode(BoolToInt(finishMode)));
					}
				}
				uint32 numStreams = (uint32)coderInfo.NumStreams;
				CObjArray<uint64> packSizes(numStreams);
				CObjArray<const uint64 *> packSizesPointers(numStreams);
				for(uint32 j = 0; j < numStreams; j++, packStreamIndex++) {
					int bond = folderInfo.FindBond_for_PackStream(packStreamIndex);
					if(bond >= 0)
						packSizesPointers[j] = &folders.CoderUnpackSizes[unpackStreamIndexStart + folderInfo.Bonds[(uint)bond].UnpackIndex];
					else {
						int index = folderInfo.Find_in_PackStreams(packStreamIndex);
						if(index < 0)
							return E_NOTIMPL;
						packSizes[j] = packPositions[(uint)index + 1] - packPositions[(uint)index];
						packSizesPointers[j] = &packSizes[j];
					}
				}
				const uint64 * unpackSizesPointer = (unpackSize && i == bindInfo.UnpackCoder) ? unpackSize : &folders.CoderUnpackSizes[unpackStreamIndexStart + i];
				_mixer->SetCoderInfo(i, unpackSizesPointer, packSizesPointers, finishMode);
			}
			if(outStream) {
				_mixer->SelectMainCoder(!fullUnpack);
			}
			CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;
			CLockedInStream * lockedInStreamSpec = new CLockedInStream;
			CMyComPtr<IUnknown> lockedInStream = lockedInStreamSpec;
			bool needMtLock = false;
			if(folderInfo.PackStreams.Size() > 1) {
				// lockedInStream.Pos = (uint64)(int64)-1;
				// RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &lockedInStream.Pos));
				RINOK(inStream->Seek(startPos + packPositions[0], STREAM_SEEK_SET, &lockedInStreamSpec->Pos));
				lockedInStreamSpec->Stream = inStream;
			#ifdef USE_MIXER_ST
				if(_mixer->IsThere_ExternalCoder_in_PackTree(_mixer->MainCoderIndex))
			#endif
				needMtLock = true;
			}
			for(uint j = 0; j < folderInfo.PackStreams.Size(); j++) {
				CMyComPtr<ISequentialInStream> packStream;
				uint64 packPos = startPos + packPositions[j];
				if(folderInfo.PackStreams.Size() == 1) {
					RINOK(inStream->Seek(packPos, STREAM_SEEK_SET, NULL));
					packStream = inStream;
				}
				else {
			  #ifdef USE_MIXER_MT
			  #ifdef USE_MIXER_ST
					if(_useMixerMT || needMtLock)
			  #endif
					{
						CLockedSequentialInStreamMT * lockedStreamImpSpec = new CLockedSequentialInStreamMT;
						packStream = lockedStreamImpSpec;
						lockedStreamImpSpec->Init(lockedInStreamSpec, packPos);
					}
			  #ifdef USE_MIXER_ST
					else
			  #endif
			  #endif
					{
			#ifdef USE_MIXER_ST
						CLockedSequentialInStreamST * lockedStreamImpSpec = new CLockedSequentialInStreamST;
						packStream = lockedStreamImpSpec;
						lockedStreamImpSpec->Init(lockedInStreamSpec, packPos);
			#endif
					}
				}
				CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
				inStreams.AddNew() = streamSpec;
				streamSpec->SetStream(packStream);
				streamSpec->Init(packPositions[j + 1] - packPositions[j]);
			}
			unsigned num = inStreams.Size();
			CObjArray<ISequentialInStream *> inStreamPointers(num);
			for(i = 0; i < num; i++)
				inStreamPointers[i] = inStreams[i];
			if(outStream) {
				CMyComPtr<ICompressProgressInfo> progress2;
				if(compressProgress && !_mixer->Is_PackSize_Correct_for_Coder(_mixer->MainCoderIndex))
					progress2 = new CDecProgress(compressProgress);
				ISequentialOutStream * outStreamPointer = outStream;
				return _mixer->Code(inStreamPointers, &outStreamPointer, progress2 ? (ICompressProgressInfo*)progress2 : compressProgress, dataAfterEnd_Error);
			}
		  #ifdef USE_MIXER_ST
			return _mixerST->GetMainUnpackStream(inStreamPointers, inStreamMainRes);
		  #else
			return E_FAIL;
		  #endif
		}
		//
		// Extract
		//
		class CFolderOutStream : public ISequentialOutStream, public CMyUnknownImp {
			CMyComPtr<ISequentialOutStream> _stream;
		public:
			bool TestMode;
			bool CheckCrc;
		private:
			bool _fileIsOpen;
			bool _calcCrc;
			uint32 _crc;
			uint64 _rem;
			const  uint32 * _indexes;
			uint   _numFiles;
			uint   _fileIndex;

			HRESULT OpenFile(bool isCorrupted = false);
			HRESULT CloseFile_and_SetResult(int32 res);
			HRESULT CloseFile();
			HRESULT ProcessEmptyFiles();

		public:
			MY_UNKNOWN_IMP1(ISequentialOutStream)
			const CDbEx *_db;
			CMyComPtr<IArchiveExtractCallback> ExtractCallback;
			bool ExtraWriteWasCut;
			CFolderOutStream() : TestMode(false), CheckCrc(true)
			{
			}
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
			HRESULT Init(unsigned startIndex, const uint32 * indexes, unsigned numFiles);
			HRESULT FlushCorrupted(int32 callbackOperationResult);
			bool WasWritingFinished() const { return _numFiles == 0; }
		};

		HRESULT CFolderOutStream::Init(unsigned startIndex, const uint32 * indexes, unsigned numFiles)
		{
			_fileIndex = startIndex;
			_indexes = indexes;
			_numFiles = numFiles;
			_fileIsOpen = false;
			ExtraWriteWasCut = false;
			return ProcessEmptyFiles();
		}

		HRESULT CFolderOutStream::OpenFile(bool isCorrupted)
		{
			const CFileItem &fi = _db->Files[_fileIndex];
			uint32 nextFileIndex = (_indexes ? *_indexes : _fileIndex);
			int32 askMode = (_fileIndex == nextFileIndex) ? (TestMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract) : NExtractArc::NAskMode::kSkip;
			if(isCorrupted && askMode == NExtractArc::NAskMode::kExtract && !_db->IsItemAnti(_fileIndex) && !fi.IsDir)
				askMode = NExtractArc::NAskMode::kTest;
			CMyComPtr<ISequentialOutStream> realOutStream;
			RINOK(ExtractCallback->GetStream(_fileIndex, &realOutStream, askMode));
			_stream = realOutStream;
			_crc = CRC_INIT_VAL;
			_calcCrc = (CheckCrc && fi.CrcDefined && !fi.IsDir);
			_fileIsOpen = true;
			_rem = fi.Size;
			if(askMode == NExtractArc::NAskMode::kExtract && !realOutStream && !_db->IsItemAnti(_fileIndex) && !fi.IsDir)
				askMode = NExtractArc::NAskMode::kSkip;
			return ExtractCallback->PrepareOperation(askMode);
		}

		HRESULT CFolderOutStream::CloseFile_and_SetResult(int32 res)
		{
			_stream.Release();
			_fileIsOpen = false;
			if(!_indexes)
				_numFiles--;
			else if(*_indexes == _fileIndex) {
				_indexes++;
				_numFiles--;
			}
			_fileIndex++;
			return ExtractCallback->SetOperationResult(res);
		}

		HRESULT CFolderOutStream::CloseFile()
		{
			const CFileItem &fi = _db->Files[_fileIndex];
			return CloseFile_and_SetResult((!_calcCrc || fi.Crc == CRC_GET_DIGEST(_crc)) ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kCRCError);
		}

		HRESULT CFolderOutStream::ProcessEmptyFiles()
		{
			while(_numFiles != 0 && _db->Files[_fileIndex].Size == 0) {
				RINOK(OpenFile());
				RINOK(CloseFile());
			}
			return S_OK;
		}

		STDMETHODIMP CFolderOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(_fileIsOpen) {
					uint32 cur = (size < _rem ? size : (uint32)_rem);
					HRESULT result = S_OK;
					if(_stream)
						result = _stream->Write(data, cur, &cur);
					if(_calcCrc)
						_crc = CrcUpdate(_crc, data, cur);
					if(processedSize)
						*processedSize += cur;
					data = (const Byte*)data + cur;
					size -= cur;
					_rem -= cur;
					if(_rem == 0) {
						RINOK(CloseFile());
						RINOK(ProcessEmptyFiles());
					}
					RINOK(result);
					if(cur == 0)
						break;
					continue;
				}
				RINOK(ProcessEmptyFiles());
				if(_numFiles == 0) {
					// we support partial extracting
					/*
					   if(processedSize)
					   *processedSize += size;
					   break;
					 */
					ExtraWriteWasCut = true;
					// return S_FALSE;
					return k_My_HRESULT_WritingWasCut;
				}
				RINOK(OpenFile());
			}
			return S_OK;
		}

		HRESULT CFolderOutStream::FlushCorrupted(int32 callbackOperationResult)
		{
			while(_numFiles != 0) {
				if(_fileIsOpen) {
					RINOK(CloseFile_and_SetResult(callbackOperationResult));
				}
				else {
					RINOK(OpenFile(true));
				}
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testModeSpec, IArchiveExtractCallback * extractCallbackSpec)
		{
			COM_TRY_BEGIN
			CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
			uint64 importantTotalUnpacked = 0;
			// numItems = (uint32)(int32)-1;
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _db.Files.Size();
			if(numItems == 0)
				return S_OK;
			{
				CNum prevFolder = kNumNoIndex;
				uint32 nextFile = 0;
				uint32 i;
				for(i = 0; i < numItems; i++) {
					uint32 fileIndex = allFilesMode ? i : indices[i];
					CNum folderIndex = _db.FileIndexToFolderIndexMap[fileIndex];
					if(folderIndex == kNumNoIndex)
						continue;
					if(folderIndex != prevFolder || fileIndex < nextFile)
						nextFile = _db.FolderStartFileIndex[folderIndex];
					for(CNum index = nextFile; index <= fileIndex; index++)
						importantTotalUnpacked += _db.Files[index].Size;
					nextFile = fileIndex + 1;
					prevFolder = folderIndex;
				}
			}
			RINOK(extractCallback->SetTotal(importantTotalUnpacked));
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			CDecoder decoder(
			#if !defined(USE_MIXER_MT)
						false
			#elif !defined(USE_MIXER_ST)
						true
			#elif !defined(__7Z_SET_PROPERTIES)
			  #ifdef _7ZIP_ST
						false
			  #else
						true
			  #endif
			#else
						_useMultiThreadMixer
			#endif
						);

			uint64 curPacked, curUnpacked;
			CMyComPtr<IArchiveExtractCallbackMessage> callbackMessage;
			extractCallback.QueryInterface(IID_IArchiveExtractCallbackMessage, &callbackMessage);
			CFolderOutStream * folderOutStream = new CFolderOutStream;
			CMyComPtr<ISequentialOutStream> outStream(folderOutStream);
			folderOutStream->_db = &_db;
			folderOutStream->ExtractCallback = extractCallback;
			folderOutStream->TestMode = (testModeSpec != 0);
			folderOutStream->CheckCrc = (_crcSize != 0);
			for(uint32 i = 0;; lps->OutSize += curUnpacked, lps->InSize += curPacked) {
				RINOK(lps->SetCur());
				if(i >= numItems)
					break;
				curUnpacked = 0;
				curPacked = 0;
				uint32 fileIndex = allFilesMode ? i : indices[i];
				CNum folderIndex = _db.FileIndexToFolderIndexMap[fileIndex];
				uint32 numSolidFiles = 1;
				if(folderIndex != kNumNoIndex) {
					curPacked = _db.GetFolderFullPackSize(folderIndex);
					uint32 nextFile = fileIndex + 1;
					fileIndex = _db.FolderStartFileIndex[folderIndex];
					uint32 k;
					for(k = i + 1; k < numItems; k++) {
						uint32 fileIndex2 = allFilesMode ? k : indices[k];
						if(_db.FileIndexToFolderIndexMap[fileIndex2] != folderIndex || fileIndex2 < nextFile)
							break;
						nextFile = fileIndex2 + 1;
					}
					numSolidFiles = k - i;
					for(k = fileIndex; k < nextFile; k++)
						curUnpacked += _db.Files[k].Size;
				}
				{
					HRESULT result = folderOutStream->Init(fileIndex, allFilesMode ? NULL : indices + i, numSolidFiles);
					i += numSolidFiles;
					RINOK(result);
				}
				// to test solid block with zero unpacked size we disable that code
				if(folderOutStream->WasWritingFinished())
					continue;
			#ifndef _NO_CRYPTO
				CMyComPtr<ICryptoGetTextPassword> getTextPassword;
				if(extractCallback)
					extractCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
			#endif
				try {
			  #ifndef _NO_CRYPTO
					bool isEncrypted = false;
					bool passwordIsDefined = false;
					UString password;
			  #endif
					bool dataAfterEnd_Error = false;
					HRESULT result = decoder.Decode(EXTERNAL_CODECS_VARS _inStream, _db.ArcInfo.DataStartPosition, _db, folderIndex,
						&curUnpacked, outStream, progress, NULL/* *inStreamMainRes*/, dataAfterEnd_Error
								_7Z_DECODER_CRYPRO_VARS
			  #if !defined(_7ZIP_ST) && !defined(_SFX)
								, true, _numThreads
			  #endif
								);
					if(oneof2(result, S_FALSE, E_NOTIMPL) || dataAfterEnd_Error) {
						bool wasFinished = folderOutStream->WasWritingFinished();
						int resOp = NExtractArc::NOperationResult::kDataError;
						if(result != S_FALSE) {
							if(result == E_NOTIMPL)
								resOp = NExtractArc::NOperationResult::kUnsupportedMethod;
							else if(wasFinished && dataAfterEnd_Error)
								resOp = NExtractArc::NOperationResult::kDataAfterEnd;
						}
						RINOK(folderOutStream->FlushCorrupted(resOp));
						if(wasFinished) {
							// we don't show error, if it's after required files
							if(/* !folderOutStream->ExtraWriteWasCut && */ callbackMessage) {
								RINOK(callbackMessage->ReportExtractResult(NEventIndexType::kBlockIndex, folderIndex, resOp));
							}
						}
						continue;
					}
					if(result != S_OK)
						return result;
					RINOK(folderOutStream->FlushCorrupted(NExtractArc::NOperationResult::kDataError));
					continue;
				}
				catch(...) {
					RINOK(folderOutStream->FlushCorrupted(NExtractArc::NOperationResult::kDataError));
					// continue;
					return E_FAIL;
				}
			}
			return S_OK;
			COM_TRY_END
		}
		//
		// FolderInStream
		//
		void CFolderInStream::Init(IArchiveUpdateCallback * updateCallback, const uint32 * indexes, unsigned numFiles)
		{
			_updateCallback = updateCallback;
			_indexes = indexes;
			_numFiles = numFiles;
			_index = 0;
			Processed.ClearAndReserve(numFiles);
			CRCs.ClearAndReserve(numFiles);
			Sizes.ClearAndReserve(numFiles);
			_pos = 0;
			_crc = CRC_INIT_VAL;
			_size_Defined = false;
			_size = 0;
			_stream.Release();
		}

		bool CFolderInStream::WasFinished() const { return _index == _numFiles; }

		uint64 CFolderInStream::GetFullSize() const
		{
			uint64 size = 0;
			FOR_VECTOR(i, Sizes) {
				size += Sizes[i];
			}
			return size;
		}

		HRESULT CFolderInStream::OpenStream()
		{
			_pos = 0;
			_crc = CRC_INIT_VAL;
			_size_Defined = false;
			_size = 0;
			while(_index < _numFiles) {
				CMyComPtr<ISequentialInStream> stream;
				HRESULT result = _updateCallback->GetStream(_indexes[_index], &stream);
				if(result != S_OK) {
					if(result != S_FALSE)
						return result;
				}
				_stream = stream;
				if(stream) {
					CMyComPtr<IStreamGetSize> streamGetSize;
					stream.QueryInterface(IID_IStreamGetSize, &streamGetSize);
					if(streamGetSize) {
						if(streamGetSize->GetSize(&_size) == S_OK)
							_size_Defined = true;
					}
					return S_OK;
				}
				_index++;
				RINOK(_updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
				AddFileInfo(result == S_OK);
			}
			return S_OK;
		}

		void CFolderInStream::AddFileInfo(bool isProcessed)
		{
			Processed.Add(isProcessed);
			Sizes.Add(_pos);
			CRCs.Add(CRC_GET_DIGEST(_crc));
		}

		STDMETHODIMP CFolderInStream::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(_stream) {
					uint32 processed2;
					RINOK(_stream->Read(data, size, &processed2));
					if(processed2 != 0) {
						_crc = CrcUpdate(_crc, data, processed2);
						_pos += processed2;
						ASSIGN_PTR(processedSize, processed2);
						return S_OK;
					}
					_stream.Release();
					_index++;
					AddFileInfo(true);
					_pos = 0;
					_crc = CRC_INIT_VAL;
					_size_Defined = false;
					_size = 0;
					RINOK(_updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
				}
				if(_index >= _numFiles)
					break;
				RINOK(OpenStream());
			}
			return S_OK;
		}

		STDMETHODIMP CFolderInStream::GetSubStreamSize(uint64 subStream, uint64 * value)
		{
			*value = 0;
			if(subStream > Sizes.Size())
				return S_FALSE;  // E_FAIL;
			unsigned index = (uint)subStream;
			if(index < Sizes.Size()) {
				*value = Sizes[index];
				return S_OK;
			}
			if(!_size_Defined) {
				*value = _pos;
				return S_FALSE;
			}
			*value = (_pos > _size ? _pos : _size);
			return S_OK;
		}
		//
		// In
		//
		uint FASTCALL BoolVector_CountSum(const CBoolVector & v)
		{
			uint   sum = 0;
			const  uint size = v.Size();
			for(uint i = 0; i < size; i++)
				if(v[i])
					sum++;
			return sum;
		}
		static inline bool BoolVector_Item_IsValidAndTrue(const CBoolVector &v, unsigned i)
		{
			return (i < v.Size() ? v[i] : false);
		}
		static void BoolVector_Fill_False(CBoolVector &v, uint size)
		{
			v.ClearAndSetSize(size);
			bool * p = &v[0];
			for(uint i = 0; i < size; i++)
				p[i] = false;
		}

		class CInArchiveException {};
		class CUnsupportedFeatureException : public CInArchiveException {};
		static void ThrowException() { throw CInArchiveException(); }
		static inline void ThrowEndOfData()   { ThrowException(); }
		static inline void ThrowUnsupported() { throw CUnsupportedFeatureException(); }
		static inline void ThrowIncorrect() { ThrowException(); }

		class CStreamSwitch {
		public:
			CStreamSwitch() : _needRemove(false), _needUpdatePos(false) 
			{
			}
			~CStreamSwitch() 
			{
				Remove();
			}
			void Remove();
			void Set(CInArchive * archive, const Byte * data, size_t size, bool needUpdatePos);
			void Set(CInArchive * archive, const CByteBuffer &byteBuffer);
			void Set(CInArchive * archive, const CObjectVector<CByteBuffer> * dataVector);
		private:
			CInArchive * _archive;
			bool _needRemove;
			bool _needUpdatePos;
		};

		void CStreamSwitch::Remove()
		{
			if(_needRemove) {
				if(_archive->_inByteBack->GetRem() != 0)
					_archive->ThereIsHeaderError = true;
				_archive->DeleteByteStream(_needUpdatePos);
				_needRemove = false;
			}
		}

		void CStreamSwitch::Set(CInArchive * archive, const Byte * data, size_t size, bool needUpdatePos)
		{
			Remove();
			_archive = archive;
			_archive->AddByteStream(data, size);
			_needRemove = true;
			_needUpdatePos = needUpdatePos;
		}

		void CStreamSwitch::Set(CInArchive * archive, const CByteBuffer &byteBuffer)
		{
			Set(archive, byteBuffer, byteBuffer.Size(), false);
		}

		void CStreamSwitch::Set(CInArchive * archive, const CObjectVector<CByteBuffer> * dataVector)
		{
			Remove();
			Byte external = archive->ReadByte();
			if(external != 0) {
				if(!dataVector)
					ThrowIncorrect();
				CNum dataIndex = archive->ReadNum();
				if(dataIndex >= dataVector->Size())
					ThrowIncorrect();
				Set(archive, (*dataVector)[dataIndex]);
			}
		}

		void CInArchive::AddByteStream(const Byte * buf, size_t size)
		{
			if(_numInByteBufs == kNumBufLevelsMax)
				ThrowIncorrect();
			_inByteBack = &_inByteVector[_numInByteBufs++];
			_inByteBack->Init(buf, size);
		}

		void CInArchive::DeleteByteStream(bool needUpdatePos)
		{
			_numInByteBufs--;
			if(_numInByteBufs > 0) {
				_inByteBack = &_inByteVector[_numInByteBufs - 1];
				if(needUpdatePos)
					_inByteBack->_pos += _inByteVector[_numInByteBufs]._pos;
			}
		}

		Byte CInByte2::ReadByte()
		{
			if(_pos >= _size)
				ThrowEndOfData();
			return _buffer[_pos++];
		}

		void CInByte2::ReadBytes(Byte * data, size_t size)
		{
			if(size == 0)
				return;
			if(size > _size - _pos)
				ThrowEndOfData();
			memcpy(data, _buffer + _pos, size);
			_pos += size;
		}

		void CInByte2::SkipData(uint64 size)
		{
			if(size > _size - _pos)
				ThrowEndOfData();
			_pos += (size_t)size;
		}

		void CInByte2::SkipData()
		{
			SkipData(ReadNumber());
		}

		static uint64 ReadNumberSpec(const Byte * p, size_t size, size_t &processed)
		{
			if(size == 0) {
				processed = 0;
				return 0;
			}
			unsigned b = *p++;
			size--;
			if((b & 0x80) == 0) {
				processed = 1;
				return b;
			}
			if(size == 0) {
				processed = 0;
				return 0;
			}
			uint64 value = (uint64)*p;
			p++;
			size--;
			for(uint i = 1; i < 8; i++) {
				unsigned mask = (uint)0x80 >> i;
				if((b & mask) == 0) {
					uint64 high = b & (mask - 1);
					value |= (high << (i * 8));
					processed = i + 1;
					return value;
				}
				if(size == 0) {
					processed = 0;
					return 0;
				}
				value |= ((uint64)*p << (i * 8));
				p++;
				size--;
			}
			processed = 9;
			return value;
		}

		uint64 CInByte2::ReadNumber()
		{
			size_t processed;
			uint64 res = ReadNumberSpec(_buffer + _pos, _size - _pos, processed);
			if(processed == 0)
				ThrowEndOfData();
			_pos += processed;
			return res;
		}

		CNum CInByte2::ReadNum()
		{
			/*
			   if(_pos < _size)
			   {
			   Byte val = _buffer[_pos];
			   if((uint)val < 0x80)
			   {
				_pos++;
				return (uint)val;
			   }
			   }
			 */
			uint64 value = ReadNumber();
			if(value > kNumMax)
				ThrowUnsupported();
			return (CNum)value;
		}

		uint32 CInByte2::ReadUInt32()
		{
			if(_pos + 4 > _size)
				ThrowEndOfData();
			uint32 res = Get32(_buffer + _pos);
			_pos += 4;
			return res;
		}

		uint64 CInByte2::ReadUInt64()
		{
			if(_pos + 8 > _size)
				ThrowEndOfData();
			uint64 res = Get64(_buffer + _pos);
			_pos += 8;
			return res;
		}

		#define CHECK_SIGNATURE if(p[0] != '7' || p[1] != 'z' || p[2] != 0xBC || p[3] != 0xAF || p[4] != 0x27 || p[5] != 0x1C) return false;

		static inline bool TestSignature(const Byte * p)
		{
			CHECK_SIGNATURE
			return CrcCalc(p + 12, 20) == Get32(p + 8);
		}

		#ifdef FORMAT_7Z_RECOVERY
			static inline bool TestSignature2(const Byte * p)
			{
				CHECK_SIGNATURE;
				if(CrcCalc(p + 12, 20) == Get32(p + 8))
					return true;
				else {
					for(uint i = 8; i < kHeaderSize; i++)
						if(p[i] != 0)
							return false;
					return (p[6] != 0 || p[7] != 0);
				}
			}
		#else
			#define TestSignature2(p) TestSignature(p)
		#endif

		void CInArchiveInfo::Clear()
		{
			StartPosition = 0;
			StartPositionAfterHeader = 0;
			DataStartPosition = 0;
			DataStartPosition2 = 0;
			FileInfoPopIDs.Clear();
		}

		HRESULT CInArchive::FindAndReadSignature(IInStream * stream, const uint64 * searchHeaderSizeLimit)
		{
			RINOK(ReadStream_FALSE(stream, _header, kHeaderSize));
			if(TestSignature2(_header))
				return S_OK;
			if(searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
				return S_FALSE;
			const uint32 kBufSize = 1 << 15;
			CByteArr buf(kBufSize);
			memcpy(buf, _header, kHeaderSize);
			uint64 offset = 0;

			for(;; ) {
				uint32 readSize = kBufSize - kHeaderSize;
				if(searchHeaderSizeLimit) {
					uint64 rem = *searchHeaderSizeLimit - offset;
					if(readSize > rem)
						readSize = (uint32)rem;
					if(readSize == 0)
						return S_FALSE;
				}

				uint32 processed = 0;
				RINOK(stream->Read(buf + kHeaderSize, readSize, &processed));
				if(processed == 0)
					return S_FALSE;

				for(uint32 pos = 0;; ) {
					const Byte * p = buf + pos + 1;
					const Byte * lim = buf + processed;
					for(; p <= lim; p += 4) {
						if(p[0] == '7') break;
						if(p[1] == '7') {
							p += 1; break;
						}
						if(p[2] == '7') {
							p += 2; break;
						}
						if(p[3] == '7') {
							p += 3; break;
						}
					}
					;
					if(p > lim)
						break;
					pos = (uint32)(p - buf);
					if(TestSignature(p)) {
						memcpy(_header, p, kHeaderSize);
						_arhiveBeginStreamPosition += offset + pos;
						return stream->Seek(_arhiveBeginStreamPosition + kHeaderSize, STREAM_SEEK_SET, NULL);
					}
				}

				offset += processed;
				memmove(buf, buf + processed, kHeaderSize);
			}
		}

		// S_FALSE means that file is not archive
		HRESULT CInArchive::Open(IInStream * stream, const uint64 * searchHeaderSizeLimit)
		{
			HeadersSize = 0;
			Close();
			RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_arhiveBeginStreamPosition))
			RINOK(stream->Seek(0, STREAM_SEEK_END, &_fileEndPosition))
			RINOK(stream->Seek(_arhiveBeginStreamPosition, STREAM_SEEK_SET, NULL))
			RINOK(FindAndReadSignature(stream, searchHeaderSizeLimit));
			_stream = stream;
			return S_OK;
		}

		void CInArchive::Close()
		{
			_numInByteBufs = 0;
			_stream.Release();
			ThereIsHeaderError = false;
		}

		void CInArchive::ReadArchiveProperties(CInArchiveInfo & /* archiveInfo */)
		{
			for(;; ) {
				if(ReadID() == NID::kEnd)
					break;
				SkipData();
			}
		}

		// CFolder &folder can be non empty. So we must set all fields

		void CInByte2::ParseFolder(CFolder &folder)
		{
			uint32 numCoders = ReadNum();

			if(numCoders == 0)
				ThrowUnsupported();

			folder.Coders.SetSize(numCoders);

			uint32 numInStreams = 0;
			uint32 i;
			for(i = 0; i < numCoders; i++) {
				CCoderInfo &coder = folder.Coders[i];
				{
					Byte mainByte = ReadByte();
					if((mainByte & 0xC0) != 0)
						ThrowUnsupported();
					unsigned idSize = (mainByte & 0xF);
					if(idSize > 8 || idSize > GetRem())
						ThrowUnsupported();
					const Byte * longID = GetPtr();
					uint64 id = 0;
					for(uint j = 0; j < idSize; j++)
						id = ((id << 8) | longID[j]);
					SkipDataNoCheck(idSize);
					coder.MethodID = id;

					if((mainByte & 0x10) != 0) {
						coder.NumStreams = ReadNum();
						/* numOutStreams = */ ReadNum();
					}
					else {
						coder.NumStreams = 1;
					}

					if((mainByte & 0x20) != 0) {
						CNum propsSize = ReadNum();
						coder.Props.Alloc((size_t)propsSize);
						ReadBytes((Byte*)coder.Props, (size_t)propsSize);
					}
					else
						coder.Props.Free();
				}
				numInStreams += coder.NumStreams;
			}

			uint32 numBonds = numCoders - 1;
			folder.Bonds.SetSize(numBonds);
			for(i = 0; i < numBonds; i++) {
				CBond &bp = folder.Bonds[i];
				bp.PackIndex = ReadNum();
				bp.UnpackIndex = ReadNum();
			}

			if(numInStreams < numBonds)
				ThrowUnsupported();
			uint32 numPackStreams = numInStreams - numBonds;
			folder.PackStreams.SetSize(numPackStreams);
			if(numPackStreams == 1) {
				for(i = 0; i < numInStreams; i++)
					if(folder.FindBond_for_PackStream(i) < 0) {
						folder.PackStreams[0] = i;
						break;
					}
				if(i == numInStreams)
					ThrowUnsupported();
			}
			else
				for(i = 0; i < numPackStreams; i++)
					folder.PackStreams[i] = ReadNum();
		}

		CFolders::CFolders() : NumPackStreams(0), NumFolders(0) 
		{
		}

		void CFolders::Clear()
		{
			NumPackStreams = 0;
			PackPositions.Free();
			// PackCRCs.Clear();
			NumFolders = 0;
			FolderCRCs.Clear();
			NumUnpackStreamsVector.Free();
			CoderUnpackSizes.Free();
			FoToCoderUnpackSizes.Free();
			FoStartPackStreamIndex.Free();
			FoToMainUnpackSizeIndex.Free();
			FoCodersDataOffset.Free();
			CodersData.Free();
		}

		void CFolders::ParseFolderInfo(uint folderIndex, CFolder &folder) const
		{
			size_t startPos = FoCodersDataOffset[folderIndex];
			CInByte2 inByte;
			inByte.Init(CodersData + startPos, FoCodersDataOffset[folderIndex + 1] - startPos);
			inByte.ParseFolder(folder);
			if(inByte.GetRem() != 0)
				throw 20120424;
		}

		void CFolders::ParseFolderEx(uint folderIndex, CFolderEx &folder) const
		{
			ParseFolderInfo(folderIndex, folder);
			folder.UnpackCoder = FoToMainUnpackSizeIndex[folderIndex];
		}

		uint FASTCALL CFolders::GetNumFolderUnpackSizes(uint folderIndex) const
			{ return (uint)(FoToCoderUnpackSizes[folderIndex + 1] - FoToCoderUnpackSizes[folderIndex]); }
		uint64 FASTCALL CFolders::GetFolderUnpackSize(uint folderIndex) const
			{ return CoderUnpackSizes[FoToCoderUnpackSizes[folderIndex] + FoToMainUnpackSizeIndex[folderIndex]]; }
		uint64 FASTCALL CFolders::GetStreamPackSize(uint index) const
			{ return PackPositions[index + 1] - PackPositions[index]; }

		/*void CDatabase::ClearSecure()
		{
			SecureBuf.Free();
			SecureIDs.Clear();
		}*/
		void CDatabase::Clear()
		{
			CFolders::Clear();
			// ClearSecure();
			NamesBuf.Free();
			NameOffsets.Free();
			Files.Clear();
			CTime.Clear();
			ATime.Clear();
			MTime.Clear();
			StartPos.Clear();
			Attrib.Clear();
			IsAnti.Clear();
			// IsAux.Clear();
		}
		bool CDatabase::IsSolid() const
		{
			for(CNum i = 0; i < NumFolders; i++)
				if(NumUnpackStreamsVector[i] > 1)
					return true;
			return false;
		}

		bool FASTCALL CDatabase::IsItemAnti(uint index) const { return (index < IsAnti.Size() && IsAnti[index]); }
		// bool FASTCALL CDatabase::IsItemAux(uint index) const { return (index < IsAux.Size() && IsAux[index]); }

		/*const void * FASTCALL CDatabase::GetName(uint index) const
		{
			if(!NameOffsets || !NamesBuf)
				return NULL;
			return (void *)((const Byte *)NamesBuf + NameOffsets[index] * 2);
		}*/

		void CDatabase::GetPath(uint index, UString &path) const
		{
			path.Empty();
			if(!NameOffsets || !NamesBuf)
				return;
			size_t offset = NameOffsets[index];
			size_t size = NameOffsets[index + 1] - offset;
			if(size >= (1 << 28))
				return;
			wchar_t * s = path.GetBuf((uint)size - 1);
			const Byte * p = ((const Byte*)NamesBuf + offset * 2);
		  #if defined(_WIN32) && defined(MY_CPU_LE)
			wmemcpy(s, (const wchar_t*)p, size);
		  #else
			for(size_t i = 0; i < size; i++) {
				*s = Get16(p);
				p += 2;
				s++;
			}
		  #endif
			path.ReleaseBuf_SetLen((uint)size - 1);
		}

		HRESULT CDatabase::GetPath_Prop(uint index, PROPVARIANT * path) const throw()
		{
			PropVariant_Clear(path);
			if(!NameOffsets || !NamesBuf)
				return S_OK;
			size_t offset = NameOffsets[index];
			size_t size = NameOffsets[index + 1] - offset;
			if(size >= (1 << 14))
				return S_OK;
			RINOK(PropVarEm_Alloc_Bstr(path, (uint)size - 1));
			wchar_t * s = path->bstrVal;
			const Byte * p = ((const Byte*)NamesBuf + offset * 2);
			for(size_t i = 0; i < size; i++) {
				wchar_t c = Get16(p);
				p += 2;
			#if WCHAR_PATH_SEPARATOR != L'/'
				if(c == L'/')
					c = WCHAR_PATH_SEPARATOR;
			#endif
				*s++ = c;
			}
			return S_OK;
			/*
			   unsigned cur = index;
			   uint size = 0;

			   for(int i = 0;; i++)
			   {
			   size_t len = NameOffsets[cur + 1] - NameOffsets[cur];
			   size += (uint)len;
			   if(i > 256 || len > (1 << 14) || size > (1 << 14))
				return PropVarEm_Set_Str(path, "[TOO-LONG]");
			   cur = Files[cur].Parent;
			   if(cur < 0)
				break;
			   }
			   size--;

			   RINOK(PropVarEm_Alloc_Bstr(path, size));
			   wchar_t *s = path->bstrVal;
			   s += size;
			   *s = 0;
			   cur = index;

			   for(;;)
			   {
			   uint len = (uint)(NameOffsets[cur + 1] - NameOffsets[cur] - 1);
			   const Byte *p = (const Byte *)NamesBuf + (NameOffsets[cur + 1] * 2) - 2;
			   for(; len != 0; len--)
			   {
				p -= 2;
				--s;
				wchar_t c = Get16(p);
				if(c == '/')
				  c = WCHAR_PATH_SEPARATOR;
			   *s = c;
			   }

			   const CFileItem &file = Files[cur];
			   cur = file.Parent;
			   if(cur < 0)
				return S_OK;
			   *(--s) = (file.IsAltStream ? ':' : WCHAR_PATH_SEPARATOR);
			   }
			 */
		}

		void CInArchive::WaitId(uint64 id)
		{
			for(;; ) {
				uint64 type = ReadID();
				if(type == id)
					return;
				if(type == NID::kEnd)
					ThrowIncorrect();
				SkipData();
			}
		}

		void CInArchive::Read_UInt32_Vector(CUInt32DefVector &v)
		{
			unsigned numItems = v.Defs.Size();
			v.Vals.ClearAndSetSize(numItems);
			uint32 * p = &v.Vals[0];
			const bool * defs = &v.Defs[0];
			for(uint i = 0; i < numItems; i++) {
				uint32 a = 0;
				if(defs[i])
					a = ReadUInt32();
				p[i] = a;
			}
		}

		void CInArchive::ReadHashDigests(unsigned numItems, CUInt32DefVector &crcs)
		{
			ReadBoolVector2(numItems, crcs.Defs);
			Read_UInt32_Vector(crcs);
		}

		#define k_Scan_NumCoders_MAX 64
		#define k_Scan_NumCodersStreams_in_Folder_MAX 64

		void CInArchive::ReadPackInfo(CFolders &f)
		{
			CNum numPackStreams = ReadNum();

			WaitId(NID::kSize);
			f.PackPositions.Alloc(numPackStreams + 1);
			f.NumPackStreams = numPackStreams;
			uint64 sum = 0;
			for(CNum i = 0; i < numPackStreams; i++) {
				f.PackPositions[i] = sum;
				uint64 packSize = ReadNumber();
				sum += packSize;
				if(sum < packSize)
					ThrowIncorrect();
			}
			f.PackPositions[numPackStreams] = sum;

			uint64 type;
			for(;; ) {
				type = ReadID();
				if(type == NID::kEnd)
					return;
				if(type == NID::kCRC) {
					CUInt32DefVector PackCRCs;
					ReadHashDigests(numPackStreams, PackCRCs);
					continue;
				}
				SkipData();
			}
		}

		void CInArchive::ReadUnpackInfo(const CObjectVector<CByteBuffer> * dataVector, CFolders &folders)
		{
			WaitId(NID::kFolder);
			CNum numFolders = ReadNum();
			CNum numCodersOutStreams = 0;
			{
				CStreamSwitch streamSwitch;
				streamSwitch.Set(this, dataVector);
				const Byte * startBufPtr = _inByteBack->GetPtr();
				folders.NumFolders = numFolders;
				folders.FoStartPackStreamIndex.Alloc(numFolders + 1);
				folders.FoToMainUnpackSizeIndex.Alloc(numFolders);
				folders.FoCodersDataOffset.Alloc(numFolders + 1);
				folders.FoToCoderUnpackSizes.Alloc(numFolders + 1);
				CBoolVector StreamUsed;
				CBoolVector CoderUsed;
				CNum packStreamIndex = 0;
				CNum fo;
				CInByte2 * inByte = _inByteBack;
				for(fo = 0; fo < numFolders; fo++) {
					uint32 indexOfMainStream = 0;
					uint32 numPackStreams = 0;
					folders.FoCodersDataOffset[fo] = _inByteBack->GetPtr() - startBufPtr;
					CNum numInStreams = 0;
					CNum numCoders = inByte->ReadNum();
					if(numCoders == 0 || numCoders > k_Scan_NumCoders_MAX)
						ThrowUnsupported();
					for(CNum ci = 0; ci < numCoders; ci++) {
						Byte mainByte = inByte->ReadByte();
						if((mainByte & 0xC0) != 0)
							ThrowUnsupported();
						unsigned idSize = (mainByte & 0xF);
						if(idSize > 8)
							ThrowUnsupported();
						if(idSize > inByte->GetRem())
							ThrowEndOfData();
						const Byte * longID = inByte->GetPtr();
						uint64 id = 0;
						for(uint j = 0; j < idSize; j++)
							id = ((id << 8) | longID[j]);
						inByte->SkipDataNoCheck(idSize);
						if(folders.ParsedMethods.IDs.Size() < 128)
							folders.ParsedMethods.IDs.AddToUniqueSorted(id);

						CNum coderInStreams = 1;
						if((mainByte & 0x10) != 0) {
							coderInStreams = inByte->ReadNum();
							if(coderInStreams > k_Scan_NumCodersStreams_in_Folder_MAX)
								ThrowUnsupported();
							if(inByte->ReadNum() != 1)
								ThrowUnsupported();
						}

						numInStreams += coderInStreams;
						if(numInStreams > k_Scan_NumCodersStreams_in_Folder_MAX)
							ThrowUnsupported();

						if((mainByte & 0x20) != 0) {
							CNum propsSize = inByte->ReadNum();
							if(propsSize > inByte->GetRem())
								ThrowEndOfData();
							if(id == k_LZMA2 && propsSize == 1) {
								Byte v = *_inByteBack->GetPtr();
								if(folders.ParsedMethods.Lzma2Prop < v)
									folders.ParsedMethods.Lzma2Prop = v;
							}
							else if(id == k_LZMA && propsSize == 5) {
								uint32 dicSize = GetUi32(_inByteBack->GetPtr() + 1);
								if(folders.ParsedMethods.LzmaDic < dicSize)
									folders.ParsedMethods.LzmaDic = dicSize;
							}
							inByte->SkipDataNoCheck((size_t)propsSize);
						}
					}

					if(numCoders == 1 && numInStreams == 1) {
						indexOfMainStream = 0;
						numPackStreams = 1;
					}
					else {
						uint32 i;
						CNum numBonds = numCoders - 1;
						if(numInStreams < numBonds)
							ThrowUnsupported();

						BoolVector_Fill_False(StreamUsed, numInStreams);
						BoolVector_Fill_False(CoderUsed, numCoders);

						for(i = 0; i < numBonds; i++) {
							CNum index = ReadNum();
							if(index >= numInStreams || StreamUsed[index])
								ThrowUnsupported();
							StreamUsed[index] = true;

							index = ReadNum();
							if(index >= numCoders || CoderUsed[index])
								ThrowUnsupported();
							CoderUsed[index] = true;
						}

						numPackStreams = numInStreams - numBonds;

						if(numPackStreams != 1)
							for(i = 0; i < numPackStreams; i++) {
								CNum index = inByte->ReadNum(); // PackStreams
								if(index >= numInStreams || StreamUsed[index])
									ThrowUnsupported();
								StreamUsed[index] = true;
							}

						for(i = 0; i < numCoders; i++)
							if(!CoderUsed[i]) {
								indexOfMainStream = i;
								break;
							}

						if(i == numCoders)
							ThrowUnsupported();
					}

					folders.FoToCoderUnpackSizes[fo] = numCodersOutStreams;
					numCodersOutStreams += numCoders;
					folders.FoStartPackStreamIndex[fo] = packStreamIndex;
					if(numPackStreams > folders.NumPackStreams - packStreamIndex)
						ThrowIncorrect();
					packStreamIndex += numPackStreams;
					folders.FoToMainUnpackSizeIndex[fo] = (Byte)indexOfMainStream;
				}

				size_t dataSize = _inByteBack->GetPtr() - startBufPtr;
				folders.FoToCoderUnpackSizes[fo] = numCodersOutStreams;
				folders.FoStartPackStreamIndex[fo] = packStreamIndex;
				folders.FoCodersDataOffset[fo] = _inByteBack->GetPtr() - startBufPtr;
				folders.CodersData.CopyFrom(startBufPtr, dataSize);

				// if(folders.NumPackStreams != packStreamIndex) ThrowUnsupported();
			}

			WaitId(NID::kCodersUnpackSize);
			folders.CoderUnpackSizes.Alloc(numCodersOutStreams);
			for(CNum i = 0; i < numCodersOutStreams; i++)
				folders.CoderUnpackSizes[i] = ReadNumber();

			for(;; ) {
				uint64 type = ReadID();
				if(type == NID::kEnd)
					return;
				if(type == NID::kCRC) {
					ReadHashDigests(numFolders, folders.FolderCRCs);
					continue;
				}
				SkipData();
			}
		}

		void CInArchive::ReadSubStreamsInfo(CFolders &folders,
					CRecordVector<uint64> &unpackSizes,
					CUInt32DefVector &digests)
		{
			folders.NumUnpackStreamsVector.Alloc(folders.NumFolders);
			CNum i;
			for(i = 0; i < folders.NumFolders; i++)
				folders.NumUnpackStreamsVector[i] = 1;

			uint64 type;

			for(;; ) {
				type = ReadID();
				if(type == NID::kNumUnpackStream) {
					for(i = 0; i < folders.NumFolders; i++)
						folders.NumUnpackStreamsVector[i] = ReadNum();
					continue;
				}
				if(type == NID::kCRC || type == NID::kSize || type == NID::kEnd)
					break;
				SkipData();
			}

			if(type == NID::kSize) {
				for(i = 0; i < folders.NumFolders; i++) {
					// v3.13 incorrectly worked with empty folders
					// v4.07: we check that folder is empty
					CNum numSubstreams = folders.NumUnpackStreamsVector[i];
					if(numSubstreams == 0)
						continue;
					uint64 sum = 0;
					for(CNum j = 1; j < numSubstreams; j++) {
						uint64 size = ReadNumber();
						unpackSizes.Add(size);
						sum += size;
						if(sum < size)
							ThrowIncorrect();
					}
					uint64 folderUnpackSize = folders.GetFolderUnpackSize(i);
					if(folderUnpackSize < sum)
						ThrowIncorrect();
					unpackSizes.Add(folderUnpackSize - sum);
				}
				type = ReadID();
			}
			else {
				for(i = 0; i < folders.NumFolders; i++) {
					/* v9.26 - v9.29 incorrectly worked:
					   if(folders.NumUnpackStreamsVector[i] == 0), it threw error */
					CNum val = folders.NumUnpackStreamsVector[i];
					if(val > 1)
						ThrowIncorrect();
					if(val == 1)
						unpackSizes.Add(folders.GetFolderUnpackSize(i));
				}
			}

			unsigned numDigests = 0;
			for(i = 0; i < folders.NumFolders; i++) {
				CNum numSubstreams = folders.NumUnpackStreamsVector[i];
				if(numSubstreams != 1 || !folders.FolderCRCs.ValidAndDefined(i))
					numDigests += numSubstreams;
			}

			for(;; ) {
				if(type == NID::kEnd)
					break;
				if(type == NID::kCRC) {
					// CUInt32DefVector digests2;
					// ReadHashDigests(numDigests, digests2);
					CBoolVector digests2;
					ReadBoolVector2(numDigests, digests2);

					digests.ClearAndSetSize(unpackSizes.Size());

					unsigned k = 0;
					unsigned k2 = 0;

					for(i = 0; i < folders.NumFolders; i++) {
						CNum numSubstreams = folders.NumUnpackStreamsVector[i];
						if(numSubstreams == 1 && folders.FolderCRCs.ValidAndDefined(i)) {
							digests.Defs[k] = true;
							digests.Vals[k] = folders.FolderCRCs.Vals[i];
							k++;
						}
						else for(CNum j = 0; j < numSubstreams; j++) {
								bool defined = digests2[k2++];
								digests.Defs[k] = defined;
								uint32 crc = 0;
								if(defined)
									crc = ReadUInt32();
								digests.Vals[k] = crc;
								k++;
							}
					}
					// if(k != unpackSizes.Size()) throw 1234567;
				}
				else
					SkipData();

				type = ReadID();
			}

			if(digests.Defs.Size() != unpackSizes.Size()) {
				digests.ClearAndSetSize(unpackSizes.Size());
				unsigned k = 0;
				for(i = 0; i < folders.NumFolders; i++) {
					CNum numSubstreams = folders.NumUnpackStreamsVector[i];
					if(numSubstreams == 1 && folders.FolderCRCs.ValidAndDefined(i)) {
						digests.Defs[k] = true;
						digests.Vals[k] = folders.FolderCRCs.Vals[i];
						k++;
					}
					else for(CNum j = 0; j < numSubstreams; j++) {
							digests.Defs[k] = false;
							digests.Vals[k] = 0;
							k++;
						}
				}
			}
		}

		void CInArchive::ReadStreamsInfo(const CObjectVector<CByteBuffer> * dataVector,
					uint64 &dataOffset,
					CFolders &folders,
					CRecordVector<uint64> &unpackSizes,
					CUInt32DefVector &digests)
		{
			uint64 type = ReadID();

			if(type == NID::kPackInfo) {
				dataOffset = ReadNumber();
				ReadPackInfo(folders);
				type = ReadID();
			}

			if(type == NID::kUnpackInfo) {
				ReadUnpackInfo(dataVector, folders);
				type = ReadID();
			}

			if(folders.NumFolders != 0 && !folders.PackPositions) {
				// if there are folders, we need PackPositions also
				folders.PackPositions.Alloc(1);
				folders.PackPositions[0] = 0;
			}

			if(type == NID::kSubStreamsInfo) {
				ReadSubStreamsInfo(folders, unpackSizes, digests);
				type = ReadID();
			}
			else {
				folders.NumUnpackStreamsVector.Alloc(folders.NumFolders);
				/* If digests.Defs.Size() == 0, it means that there are no crcs.
				   So we don't need to fill digests with values. */
				// digests.Vals.ClearAndSetSize(folders.NumFolders);
				// BoolVector_Fill_False(digests.Defs, folders.NumFolders);
				for(CNum i = 0; i < folders.NumFolders; i++) {
					folders.NumUnpackStreamsVector[i] = 1;
					unpackSizes.Add(folders.GetFolderUnpackSize(i));
					// digests.Vals[i] = 0;
				}
			}

			if(type != NID::kEnd)
				ThrowIncorrect();
		}

		void CInArchive::ReadBoolVector(unsigned numItems, CBoolVector &v)
		{
			v.ClearAndSetSize(numItems);
			Byte b = 0;
			Byte mask = 0;
			bool * p = &v[0];
			for(uint i = 0; i < numItems; i++) {
				if(mask == 0) {
					b = ReadByte();
					mask = 0x80;
				}
				p[i] = ((b & mask) != 0);
				mask >>= 1;
			}
		}

		void CInArchive::ReadBoolVector2(unsigned numItems, CBoolVector &v)
		{
			Byte allAreDefined = ReadByte();
			if(allAreDefined == 0) {
				ReadBoolVector(numItems, v);
				return;
			}
			v.ClearAndSetSize(numItems);
			bool * p = &v[0];
			for(uint i = 0; i < numItems; i++)
				p[i] = true;
		}

		void CInArchive::ReadUInt64DefVector(const CObjectVector<CByteBuffer> &dataVector,
					CUInt64DefVector &v, unsigned numItems)
		{
			ReadBoolVector2(numItems, v.Defs);

			CStreamSwitch streamSwitch;
			streamSwitch.Set(this, &dataVector);

			v.Vals.ClearAndSetSize(numItems);
			uint64 * p = &v.Vals[0];
			const bool * defs = &v.Defs[0];

			for(uint i = 0; i < numItems; i++) {
				uint64 t = 0;
				if(defs[i])
					t = ReadUInt64();
				p[i] = t;
			}
		}

		HRESULT CInArchive::ReadAndDecodePackedStreams(DECL_EXTERNAL_CODECS_LOC_VARS
					uint64 baseOffset,
					uint64 &dataOffset, CObjectVector<CByteBuffer> &dataVector
					_7Z_DECODER_CRYPRO_VARS_DECL
					)
		{
			CFolders folders;
			CRecordVector<uint64> unpackSizes;
			CUInt32DefVector digests;

			ReadStreamsInfo(NULL,
						dataOffset,
						folders,
						unpackSizes,
						digests);

			CDecoder decoder(_useMixerMT);

			for(CNum i = 0; i < folders.NumFolders; i++) {
				CByteBuffer &data = dataVector.AddNew();
				uint64 unpackSize64 = folders.GetFolderUnpackSize(i);
				size_t unpackSize = (size_t)unpackSize64;
				if(unpackSize != unpackSize64)
					ThrowUnsupported();
				data.Alloc(unpackSize);

				CBufPtrSeqOutStream * outStreamSpec = new CBufPtrSeqOutStream;
				CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
				outStreamSpec->Init(data, unpackSize);

				bool dataAfterEnd_Error = false;

				HRESULT result = decoder.Decode(
							EXTERNAL_CODECS_LOC_VARS
							_stream, baseOffset + dataOffset,
							folders, i,
							NULL, // *unpackSize

							outStream,
							NULL, // *compressProgress

							NULL // **inStreamMainRes
							, dataAfterEnd_Error

							_7Z_DECODER_CRYPRO_VARS
			#if !defined(_7ZIP_ST) && !defined(_SFX)
							, false // mtMode
							, 1 // numThreads
			#endif

							);

				RINOK(result);

				if(dataAfterEnd_Error)
					ThereIsHeaderError = true;

				if(folders.FolderCRCs.ValidAndDefined(i))
					if(CrcCalc(data, unpackSize) != folders.FolderCRCs.Vals[i])
						ThrowIncorrect();
			}

			if(folders.PackPositions)
				HeadersSize += folders.PackPositions[folders.NumPackStreams];

			return S_OK;
		}

		HRESULT CInArchive::ReadHeader(DECL_EXTERNAL_CODECS_LOC_VARS
					CDbEx &db
					_7Z_DECODER_CRYPRO_VARS_DECL
					)
		{
			uint64 type = ReadID();

			if(type == NID::kArchiveProperties) {
				ReadArchiveProperties(db.ArcInfo);
				type = ReadID();
			}

			CObjectVector<CByteBuffer> dataVector;

			if(type == NID::kAdditionalStreamsInfo) {
				HRESULT result = ReadAndDecodePackedStreams(
							EXTERNAL_CODECS_LOC_VARS
							db.ArcInfo.StartPositionAfterHeader,
							db.ArcInfo.DataStartPosition2,
							dataVector
							_7Z_DECODER_CRYPRO_VARS
							);
				RINOK(result);
				db.ArcInfo.DataStartPosition2 += db.ArcInfo.StartPositionAfterHeader;
				type = ReadID();
			}

			CRecordVector<uint64> unpackSizes;
			CUInt32DefVector digests;

			if(type == NID::kMainStreamsInfo) {
				ReadStreamsInfo(&dataVector,
							db.ArcInfo.DataStartPosition,
							(CFolders &)db,
							unpackSizes,
							digests);
				db.ArcInfo.DataStartPosition += db.ArcInfo.StartPositionAfterHeader;
				type = ReadID();
			}

			if(type == NID::kFilesInfo) {
				const CNum numFiles = ReadNum();

				db.ArcInfo.FileInfoPopIDs.Add(NID::kSize);
				// if(!db.PackSizes.IsEmpty())
				db.ArcInfo.FileInfoPopIDs.Add(NID::kPackInfo);
				if(numFiles > 0 && !digests.Defs.IsEmpty())
					db.ArcInfo.FileInfoPopIDs.Add(NID::kCRC);

				CBoolVector emptyStreamVector;
				CBoolVector emptyFileVector;
				CBoolVector antiFileVector;
				CNum numEmptyStreams = 0;

				for(;; ) {
					const uint64 type2 = ReadID();
					if(type2 == NID::kEnd)
						break;
					uint64 size = ReadNumber();
					if(size > _inByteBack->GetRem())
						ThrowIncorrect();
					CStreamSwitch switchProp;
					switchProp.Set(this, _inByteBack->GetPtr(), (size_t)size, true);
					bool addPropIdToList = true;
					bool isKnownType = true;
					if(type2 > ((uint32)1 << 30))
						isKnownType = false;
					else 
						switch((uint32)type2) {
							case NID::kName:
							{
								CStreamSwitch streamSwitch;
								streamSwitch.Set(this, &dataVector);
								size_t rem = _inByteBack->GetRem();
								db.NamesBuf.Alloc(rem);
								ReadBytes(db.NamesBuf, rem);
								db.NameOffsets.Alloc(numFiles + 1);
								size_t pos = 0;
								uint i;
								for(i = 0; i < numFiles; i++) {
									size_t curRem = (rem - pos) / 2;
									const uint16 * buf = (const uint16*)(db.NamesBuf + pos);
									size_t j;
									for(j = 0; j < curRem && buf[j] != 0; j++) ;
									if(j == curRem)
										ThrowEndOfData();
									db.NameOffsets[i] = pos / 2;
									pos += j * 2 + 2;
								}
								db.NameOffsets[i] = pos / 2;
								if(pos != rem)
									ThereIsHeaderError = true;
								break;
							}

							case NID::kWinAttrib:
							{
								ReadBoolVector2(numFiles, db.Attrib.Defs);
								CStreamSwitch streamSwitch;
								streamSwitch.Set(this, &dataVector);
								Read_UInt32_Vector(db.Attrib);
								break;
							}

							/*
							   case NID::kIsAux:
							   {
							   ReadBoolVector(numFiles, db.IsAux);
							   break;
							   }
							   case NID::kParent:
							   {
							   db.IsTree = true;
							   // CBoolVector boolVector;
							   // ReadBoolVector2(numFiles, boolVector);
							   // CStreamSwitch streamSwitch;
							   // streamSwitch.Set(this, &dataVector);
							   CBoolVector boolVector;
							   ReadBoolVector2(numFiles, boolVector);

							   db.ThereAreAltStreams = false;
							   for(i = 0; i < numFiles; i++)
							   {
								CFileItem &file = db.Files[i];
								// file.Parent = -1;
								// if(boolVector[i])
								file.Parent = (int)ReadUInt32();
								file.IsAltStream = !boolVector[i];
								if(file.IsAltStream)
								  db.ThereAreAltStreams = true;
							   }
							   break;
							   }
							 */
							case NID::kEmptyStream:
							{
								ReadBoolVector(numFiles, emptyStreamVector);
								numEmptyStreams = BoolVector_CountSum(emptyStreamVector);
								emptyFileVector.Clear();
								antiFileVector.Clear();
								break;
							}
							case NID::kEmptyFile:  ReadBoolVector(numEmptyStreams, emptyFileVector); break;
							case NID::kAnti:  ReadBoolVector(numEmptyStreams, antiFileVector); break;
							case NID::kStartPos:  ReadUInt64DefVector(dataVector, db.StartPos, (uint)numFiles); break;
							case NID::kCTime:  ReadUInt64DefVector(dataVector, db.CTime, (uint)numFiles); break;
							case NID::kATime:  ReadUInt64DefVector(dataVector, db.ATime, (uint)numFiles); break;
							case NID::kMTime:  ReadUInt64DefVector(dataVector, db.MTime, (uint)numFiles); break;
							case NID::kDummy:
							{
								for(uint64 j = 0; j < size; j++)
									if(ReadByte() != 0)
										ThereIsHeaderError = true;
								addPropIdToList = false;
								break;
							}
							/*
							   case NID::kNtSecure:
							   {
							   try {
								{
								  CStreamSwitch streamSwitch;
								  streamSwitch.Set(this, &dataVector);
								  uint32 numDescriptors = ReadUInt32();
								  size_t offset = 0;
								  db.SecureOffsets.Clear();
								  for(i = 0; i < numDescriptors; i++)
								  {
									uint32 size = ReadUInt32();
									db.SecureOffsets.Add(offset);
									offset += size;
								  }
								  // ThrowIncorrect();;
								  db.SecureOffsets.Add(offset);
								  db.SecureBuf.SetCapacity(offset);
								  for(i = 0; i < numDescriptors; i++)
								  {
									offset = db.SecureOffsets[i];
									ReadBytes(db.SecureBuf + offset, db.SecureOffsets[i + 1] - offset);
								  }
								  db.SecureIDs.Clear();
								  for(unsigned i = 0; i < numFiles; i++)
								  {
									db.SecureIDs.Add(ReadNum());
									// db.SecureIDs.Add(ReadUInt32());
								  }
								  // ReadUInt32();
								  if(_inByteBack->GetRem() != 0)
									ThrowIncorrect();;
								}
							   }
							   catch(CInArchiveException &)
							   {
								ThereIsHeaderError = true;
								addPropIdToList = isKnownType = false;
								db.ClearSecure();
							   }
							   break;
							   }
							 */
							default:
								addPropIdToList = isKnownType = false;
						}
					if(isKnownType) {
						if(addPropIdToList)
							db.ArcInfo.FileInfoPopIDs.Add(type2);
					}
					else {
						db.UnsupportedFeatureWarning = true;
						_inByteBack->SkipRem();
					}
					// SkipData worked incorrectly in some versions before v4.59 (7zVer <= 0.02)
					if(_inByteBack->GetRem() != 0)
						ThrowIncorrect();
				}
				type = ReadID(); // Read (NID::kEnd) end of headers
				if(numFiles - numEmptyStreams != unpackSizes.Size())
					ThrowUnsupported();
				CNum emptyFileIndex = 0;
				CNum sizeIndex = 0;
				const CNum numAntiItems = BoolVector_CountSum(antiFileVector);
				if(numAntiItems != 0)
					db.IsAnti.ClearAndSetSize(numFiles);
				db.Files.ClearAndSetSize(numFiles);
				for(CNum i = 0; i < numFiles; i++) {
					CFileItem &file = db.Files[i];
					bool isAnti;
					file.Crc = 0;
					if(!BoolVector_Item_IsValidAndTrue(emptyStreamVector, i)) {
						file.HasStream = true;
						file.IsDir = false;
						isAnti = false;
						file.Size = unpackSizes[sizeIndex];
						file.CrcDefined = digests.ValidAndDefined(sizeIndex);
						if(file.CrcDefined)
							file.Crc = digests.Vals[sizeIndex];
						sizeIndex++;
					}
					else {
						file.HasStream = false;
						file.IsDir = !BoolVector_Item_IsValidAndTrue(emptyFileVector, emptyFileIndex);
						isAnti = BoolVector_Item_IsValidAndTrue(antiFileVector, emptyFileIndex);
						emptyFileIndex++;
						file.Size = 0;
						file.CrcDefined = false;
					}
					if(numAntiItems != 0)
						db.IsAnti[i] = isAnti;
				}
			}

			db.FillLinks();

			if(type != NID::kEnd || _inByteBack->GetRem() != 0) {
				db.UnsupportedFeatureWarning = true;
				// ThrowIncorrect();
			}

			return S_OK;
		}

		void CDbEx::Clear()
		{
			IsArc = false;
			PhySizeWasConfirmed = false;
			ThereIsHeaderError = false;
			UnexpectedEnd = false;
			// UnsupportedVersion = false;
			StartHeaderWasRecovered = false;
			UnsupportedFeatureError = false;
			UnsupportedFeatureWarning = false;
			/*
			IsTree = false;
			ThereAreAltStreams = false;
			*/
			CDatabase::Clear();
			// SecureOffsets.Clear();
			ArcInfo.Clear();
			FolderStartFileIndex.Free();
			FileIndexToFolderIndexMap.Free();
			HeadersSize = 0;
			PhySize = 0;
		}

		uint64 CDbEx::GetFolderStreamPos(CNum folderIndex, unsigned indexInFolder) const
			{ return ArcInfo.DataStartPosition + PackPositions[FoStartPackStreamIndex[folderIndex] + indexInFolder]; }
		uint64 CDbEx::GetFolderFullPackSize(CNum folderIndex) const
			{ return PackPositions[FoStartPackStreamIndex[folderIndex + 1]] - PackPositions[FoStartPackStreamIndex[folderIndex]]; }

		uint64 CDbEx::GetFolderPackStreamSize(CNum folderIndex, unsigned streamIndex) const
		{
			size_t i = FoStartPackStreamIndex[folderIndex] + streamIndex;
			return PackPositions[i + 1] - PackPositions[i];
		}

		uint64 CDbEx::GetFilePackSize(CNum fileIndex) const
		{
			CNum folderIndex = FileIndexToFolderIndexMap[fileIndex];
			if(folderIndex != kNumNoIndex)
				if(FolderStartFileIndex[folderIndex] == fileIndex)
					return GetFolderFullPackSize(folderIndex);
			return 0;
		}

		void CDbEx::FillLinks()
		{
			FolderStartFileIndex.Alloc(NumFolders);
			FileIndexToFolderIndexMap.Alloc(Files.Size());
			CNum folderIndex = 0;
			CNum indexInFolder = 0;
			uint i;
			for(i = 0; i < Files.Size(); i++) {
				bool emptyStream = !Files[i].HasStream;
				if(indexInFolder == 0) {
					if(emptyStream) {
						FileIndexToFolderIndexMap[i] = kNumNoIndex;
						continue;
					}
					// v3.13 incorrectly worked with empty folders
					// v4.07: we skip empty folders
					for(;; ) {
						if(folderIndex >= NumFolders)
							ThrowIncorrect();
						FolderStartFileIndex[folderIndex] = i;
						if(NumUnpackStreamsVector[folderIndex] != 0)
							break;
						folderIndex++;
					}
				}
				FileIndexToFolderIndexMap[i] = folderIndex;
				if(emptyStream)
					continue;
				if(++indexInFolder >= NumUnpackStreamsVector[folderIndex]) {
					folderIndex++;
					indexInFolder = 0;
				}
			}
			if(indexInFolder != 0)
				folderIndex++;
			/*
			   if(indexInFolder != 0)
			   ThrowIncorrect();
			 */
			for(;; ) {
				if(folderIndex >= NumFolders)
					return;
				FolderStartFileIndex[folderIndex] = i;
				/*
				   if(NumUnpackStreamsVector[folderIndex] != 0)
				   ThrowIncorrect();;
				 */
				folderIndex++;
			}
		}

		HRESULT CInArchive::ReadDatabase2(DECL_EXTERNAL_CODECS_LOC_VARS CDbEx &db _7Z_DECODER_CRYPRO_VARS_DECL)
		{
			db.Clear();
			db.ArcInfo.StartPosition = _arhiveBeginStreamPosition;
			db.ArcInfo.Version.Major = _header[6];
			db.ArcInfo.Version.Minor = _header[7];
			if(db.ArcInfo.Version.Major != kMajorVersion) {
				// db.UnsupportedVersion = true;
				return S_FALSE;
			}

			uint64 nextHeaderOffset = Get64(_header + 12);
			uint64 nextHeaderSize = Get64(_header + 20);
			uint32 nextHeaderCRC = Get32(_header + 28);

		  #ifdef FORMAT_7Z_RECOVERY
			uint32 crcFromArc = Get32(_header + 8);
			if(crcFromArc == 0 && nextHeaderOffset == 0 && nextHeaderSize == 0 && nextHeaderCRC == 0) {
				uint64 cur, fileSize;
				RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &cur));
				const uint kCheckSize = 512;
				Byte buf[kCheckSize];
				RINOK(_stream->Seek(0, STREAM_SEEK_END, &fileSize));
				uint64 rem = fileSize - cur;
				unsigned checkSize = kCheckSize;
				if(rem < kCheckSize)
					checkSize = (uint)(rem);
				if(checkSize < 3)
					return S_FALSE;
				RINOK(_stream->Seek(fileSize - checkSize, STREAM_SEEK_SET, NULL));
				RINOK(ReadStream_FALSE(_stream, buf, (size_t)checkSize));

				if(buf[checkSize - 1] != 0)
					return S_FALSE;

				uint i;
				for(i = checkSize - 2;; i--) {
					if(buf[i] == NID::kEncodedHeader && buf[i + 1] == NID::kPackInfo ||
								buf[i] == NID::kHeader && buf[i + 1] == NID::kMainStreamsInfo)
						break;
					if(i == 0)
						return S_FALSE;
				}
				nextHeaderSize = checkSize - i;
				nextHeaderOffset = rem - nextHeaderSize;
				nextHeaderCRC = CrcCalc(buf + i, (size_t)nextHeaderSize);
				RINOK(_stream->Seek(cur, STREAM_SEEK_SET, NULL));
				db.StartHeaderWasRecovered = true;
			}
			else
		  #endif
			{
				// Crc was tested already at signature check
				// if(CrcCalc(_header + 12, 20) != crcFromArchive) ThrowIncorrect();
			}
			db.ArcInfo.StartPositionAfterHeader = _arhiveBeginStreamPosition + kHeaderSize;
			db.PhySize = kHeaderSize;
			db.IsArc = false;
			if((int64)nextHeaderOffset < 0 || nextHeaderSize > ((uint64)1 << 62))
				return S_FALSE;
			if(nextHeaderSize == 0) {
				if(nextHeaderOffset != 0)
					return S_FALSE;
				db.IsArc = true;
				return S_OK;
			}
			if(!db.StartHeaderWasRecovered)
				db.IsArc = true;
			HeadersSize += kHeaderSize + nextHeaderSize;
			db.PhySize = kHeaderSize + nextHeaderOffset + nextHeaderSize;
			if(_fileEndPosition - db.ArcInfo.StartPositionAfterHeader < nextHeaderOffset + nextHeaderSize) {
				db.UnexpectedEnd = true;
				return S_FALSE;
			}
			RINOK(_stream->Seek(nextHeaderOffset, STREAM_SEEK_CUR, NULL));
			size_t nextHeaderSize_t = (size_t)nextHeaderSize;
			if(nextHeaderSize_t != nextHeaderSize)
				return E_OUTOFMEMORY;
			CByteBuffer buffer2(nextHeaderSize_t);
			RINOK(ReadStream_FALSE(_stream, buffer2, nextHeaderSize_t));
			if(CrcCalc(buffer2, nextHeaderSize_t) != nextHeaderCRC)
				ThrowIncorrect();
			if(!db.StartHeaderWasRecovered)
				db.PhySizeWasConfirmed = true;
			CStreamSwitch streamSwitch;
			streamSwitch.Set(this, buffer2);
			CObjectVector<CByteBuffer> dataVector;
			uint64 type = ReadID();
			if(type != NID::kHeader) {
				if(type != NID::kEncodedHeader)
					ThrowIncorrect();
				HRESULT result = ReadAndDecodePackedStreams(EXTERNAL_CODECS_LOC_VARS db.ArcInfo.StartPositionAfterHeader, db.ArcInfo.DataStartPosition2,
					dataVector _7Z_DECODER_CRYPRO_VARS);
				RINOK(result);
				if(dataVector.Size() == 0)
					return S_OK;
				if(dataVector.Size() > 1)
					ThrowIncorrect();
				streamSwitch.Remove();
				streamSwitch.Set(this, dataVector.Front());
				if(ReadID() != NID::kHeader)
					ThrowIncorrect();
			}
			db.IsArc = true;
			db.HeadersSize = HeadersSize;
			return ReadHeader(EXTERNAL_CODECS_LOC_VARS db _7Z_DECODER_CRYPRO_VARS);
		}

		HRESULT CInArchive::ReadDatabase(DECL_EXTERNAL_CODECS_LOC_VARS CDbEx &db _7Z_DECODER_CRYPRO_VARS_DECL)
		{
			try {
				HRESULT res = ReadDatabase2(EXTERNAL_CODECS_LOC_VARS db _7Z_DECODER_CRYPRO_VARS);
				if(ThereIsHeaderError)
					db.ThereIsHeaderError = true;
				if(res == E_NOTIMPL)
					ThrowUnsupported();
				return res;
			}
			catch(CUnsupportedFeatureException &)
			{
				db.UnsupportedFeatureError = true;
				return S_FALSE;
			}
			catch(CInArchiveException &)
			{
				db.ThereIsHeaderError = true;
				return S_FALSE;
			}
		}
		//
		// Out
		//
		void COutFolders::OutFoldersClear()
		{
			FolderUnpackCRCs.Clear();
			NumUnpackStreamsVector.Clear();
			CoderUnpackSizes.Clear();
		}
		void COutFolders::OutFoldersReserveDown()
		{
			FolderUnpackCRCs.ReserveDown();
			NumUnpackStreamsVector.ReserveDown();
			CoderUnpackSizes.ReserveDown();
		}
		COutArchive::CWriteBufferLoc::CWriteBufferLoc() : _size(0), _pos(0), _data(0)/*@sobolev*/
		{
		}
		void COutArchive::CWriteBufferLoc::Init(Byte * data, size_t size)
		{
			_data = data;
			_size = size;
			_pos = 0;
		}
		void COutArchive::CWriteBufferLoc::WriteBytes(const void * data, size_t size)
		{
			if(size) {
				if(size > _size - _pos)
					throw 1;
				memcpy(_data + _pos, data, size);
				_pos += size;
			}
		}
		void FASTCALL COutArchive::CWriteBufferLoc::WriteByte(Byte b)
		{
			if(_size == _pos)
				throw 1;
			_data[_pos++] = b;
		}
		HRESULT COutArchive::WriteSignature()
		{
			Byte buf[8];
			memcpy(buf, kSignature, kSignatureSize);
			buf[kSignatureSize] = kMajorVersion;
			buf[kSignatureSize + 1] = 4;
			return WriteDirect(buf, 8);
		}

		#ifdef _7Z_VOL
			HRESULT COutArchive::WriteFinishSignature()
			{
				RINOK(WriteDirect(kFinishSignature, kSignatureSize));
				CArchiveVersion av;
				av.Major = kMajorVersion;
				av.Minor = 2;
				RINOK(WriteDirectByte(av.Major));
				return WriteDirectByte(av.Minor);
			}
		#endif

		static void FASTCALL SetUInt32(Byte * p, uint32 d)
		{
			for(int i = 0; i < 4; i++, d >>= 8)
				p[i] = (Byte)d;
		}

		static void FASTCALL SetUInt64(Byte * p, uint64 d)
		{
			for(int i = 0; i < 8; i++, d >>= 8)
				p[i] = (Byte)d;
		}

		HRESULT COutArchive::WriteStartHeader(const CStartHeader &h)
		{
			Byte buf[24];
			SetUInt64(buf + 4, h.NextHeaderOffset);
			SetUInt64(buf + 12, h.NextHeaderSize);
			SetUInt32(buf + 20, h.NextHeaderCRC);
			SetUInt32(buf, CrcCalc(buf + 4, 20));
			return WriteDirect(buf, 24);
		}

		#ifdef _7Z_VOL
			HRESULT COutArchive::WriteFinishHeader(const CFinishHeader &h)
			{
				CCRC crc;
				crc.UpdateUInt64(h.NextHeaderOffset);
				crc.UpdateUInt64(h.NextHeaderSize);
				crc.UpdateUInt32(h.NextHeaderCRC);
				crc.UpdateUInt64(h.ArchiveStartOffset);
				crc.UpdateUInt64(h.AdditionalStartBlockSize);
				RINOK(WriteDirectUInt32(crc.GetDigest()));
				RINOK(WriteDirectUInt64(h.NextHeaderOffset));
				RINOK(WriteDirectUInt64(h.NextHeaderSize));
				RINOK(WriteDirectUInt32(h.NextHeaderCRC));
				RINOK(WriteDirectUInt64(h.ArchiveStartOffset));
				return WriteDirectUInt64(h.AdditionalStartBlockSize);
			}
		#endif

		HRESULT COutArchive::Create(ISequentialOutStream * stream, bool endMarker)
		{
			Close();
		  #ifdef _7Z_VOL
			// endMarker = false;
			_endMarker = endMarker;
		  #endif
			SeqStream = stream;
			if(!endMarker) {
				SeqStream.QueryInterface(IID_IOutStream, &Stream);
				if(!Stream) {
					return E_NOTIMPL;
					// endMarker = true;
				}
			}
		  #ifdef _7Z_VOL
			if(endMarker) {
				/*
				   CStartHeader sh;
				   sh.NextHeaderOffset = (uint32)(int32)-1;
				   sh.NextHeaderSize = (uint32)(int32)-1;
				   sh.NextHeaderCRC = 0;
				   WriteStartHeader(sh);
				 */
			}
			else
		  #endif
			{
				if(!Stream)
					return E_FAIL;
				RINOK(WriteSignature());
				RINOK(Stream->Seek(0, STREAM_SEEK_CUR, &_prefixHeaderPos));
			}
			return S_OK;
		}

		void COutArchive::Close()
		{
			SeqStream.Release();
			Stream.Release();
		}

		HRESULT COutArchive::SkipPrefixArchiveHeader()
		{
		  #ifdef _7Z_VOL
			if(_endMarker)
				return S_OK;
		  #endif
			Byte buf[24];
			memzero(buf, 24);
			return WriteDirect(buf, 24);
		}

		uint64 COutArchive::GetPos() const
		{
			if(_countMode)
				return _countSize;
			if(_writeToStream)
				return _outByte.GetProcessedSize();
			return _outByte2.GetPos();
		}

		void COutArchive::WriteBytes(const void * data, size_t size)
		{
			if(_countMode)
				_countSize += size;
			else if(_writeToStream) {
				_outByte.WriteBytes(data, size);
				_crc = CrcUpdate(_crc, data, size);
			}
			else
				_outByte2.WriteBytes(data, size);
		}

		void COutArchive::WriteByte(Byte b)
		{
			if(_countMode)
				_countSize++;
			else if(_writeToStream) {
				_outByte.WriteByte(b);
				_crc = CRC_UPDATE_BYTE(_crc, b);
			}
			else
				_outByte2.WriteByte(b);
		}

		void COutArchive::WriteUInt32(uint32 value)
		{
			for(int i = 0; i < 4; i++) {
				WriteByte((Byte)value);
				value >>= 8;
			}
		}

		void COutArchive::WriteUInt64(uint64 value)
		{
			for(int i = 0; i < 8; i++) {
				WriteByte((Byte)value);
				value >>= 8;
			}
		}

		void COutArchive::WriteNumber(uint64 value)
		{
			Byte firstByte = 0;
			Byte mask = 0x80;
			int i;
			for(i = 0; i < 8; i++) {
				if(value < ((uint64(1) << ( 7  * (i + 1))))) {
					firstByte |= Byte(value >> (8 * i));
					break;
				}
				firstByte |= mask;
				mask >>= 1;
			}
			WriteByte(firstByte);
			for(; i > 0; i--) {
				WriteByte((Byte)value);
				value >>= 8;
			}
		}

		static uint32 GetBigNumberSize(uint64 value)
		{
			int i;
			for(i = 1; i < 9; i++)
				if(value < (((uint64)1 << (i * 7))))
					break;
			return i;
		}

		#ifdef _7Z_VOL
			uint32 COutArchive::GetVolHeadersSize(uint64 dataSize, int nameLength, bool props)
			{
				uint32 result = GetBigNumberSize(dataSize) * 2 + 41;
				if(nameLength != 0) {
					nameLength = (nameLength + 1) * 2;
					result += nameLength + GetBigNumberSize(nameLength) + 2;
				}
				if(props) {
					result += 20;
				}
				if(result >= 128)
					result++;
				result += kSignatureSize + 2 + kFinishHeaderSize;
				return result;
			}

			uint64 COutArchive::GetVolPureSize(uint64 volSize, int nameLength, bool props)
			{
				uint32 headersSizeBase = COutArchive::GetVolHeadersSize(1, nameLength, props);
				int testSize;
				if(volSize > headersSizeBase)
					testSize = volSize - headersSizeBase;
				else
					testSize = 1;
				uint32 headersSize = COutArchive::GetVolHeadersSize(testSize, nameLength, props);
				uint64 pureSize = 1;
				if(volSize > headersSize)
					pureSize = volSize - headersSize;
				return pureSize;
			}
		#endif

		void COutArchive::WriteFolder(const CFolder &folder)
		{
			WriteNumber(folder.Coders.Size());
			uint i;
			for(i = 0; i < folder.Coders.Size(); i++) {
				const CCoderInfo &coder = folder.Coders[i];
				{
					uint64 id = coder.MethodID;
					unsigned idSize;
					for(idSize = 1; idSize < sizeof(id); idSize++)
						if((id >> (8 * idSize)) == 0)
							break;
					idSize &= 0xF;
					Byte temp[16];
					for(uint t = idSize; t != 0; t--, id >>= 8)
						temp[t] = (Byte)(id & 0xFF);

					Byte b = (Byte)(idSize);
					bool isComplex = !coder.IsSimpleCoder();
					b |= (isComplex ? 0x10 : 0);

					size_t propsSize = coder.Props.Size();
					b |= ((propsSize != 0) ? 0x20 : 0);
					temp[0] = b;
					WriteBytes(temp, idSize + 1);
					if(isComplex) {
						WriteNumber(coder.NumStreams);
						WriteNumber(1); // NumOutStreams;
					}
					if(propsSize == 0)
						continue;
					WriteNumber(propsSize);
					WriteBytes(coder.Props, propsSize);
				}
			}

			for(i = 0; i < folder.Bonds.Size(); i++) {
				const CBond &bond = folder.Bonds[i];
				WriteNumber(bond.PackIndex);
				WriteNumber(bond.UnpackIndex);
			}

			if(folder.PackStreams.Size() > 1)
				for(i = 0; i < folder.PackStreams.Size(); i++)
					WriteNumber(folder.PackStreams[i]);
		}

		void COutArchive::WriteBoolVector(const CBoolVector &boolVector)
		{
			Byte b = 0;
			Byte mask = 0x80;
			FOR_VECTOR(i, boolVector) {
				if(boolVector[i])
					b |= mask;
				mask >>= 1;
				if(mask == 0) {
					WriteByte(b);
					mask = 0x80;
					b = 0;
				}
			}
			if(mask != 0x80)
				WriteByte(b);
		}

		static inline uint Bv_GetSizeInBytes(const CBoolVector & v) 
		{
			return ((uint)v.Size() + 7) / 8;
		}

		void COutArchive::WritePropBoolVector(Byte id, const CBoolVector &boolVector)
		{
			WriteByte(id);
			WriteNumber(Bv_GetSizeInBytes(boolVector));
			WriteBoolVector(boolVector);
		}

		uint FASTCALL BoolVector_CountSum(const CBoolVector &v);

		void COutArchive::WriteHashDigests(const CUInt32DefVector &digests)
		{
			const uint numDefined = BoolVector_CountSum(digests.Defs);
			if(numDefined) {
				WriteByte(NID::kCRC);
				if(numDefined == digests.Defs.Size())
					WriteByte(1);
				else {
					WriteByte(0);
					WriteBoolVector(digests.Defs);
				}
				for(uint i = 0; i < digests.Defs.Size(); i++)
					if(digests.Defs[i])
						WriteUInt32(digests.Vals[i]);
			}
		}

		void COutArchive::WritePackInfo(uint64 dataOffset, const CRecordVector<uint64> &packSizes, const CUInt32DefVector &packCRCs)
		{
			if(!packSizes.IsEmpty()) {
				WriteByte(NID::kPackInfo);
				WriteNumber(dataOffset);
				WriteNumber(packSizes.Size());
				WriteByte(NID::kSize);
				FOR_VECTOR(i, packSizes) {
					WriteNumber(packSizes[i]);
				}
				WriteHashDigests(packCRCs);
				WriteByte(NID::kEnd);
			}
		}

		void COutArchive::WriteUnpackInfo(const CObjectVector<CFolder> &folders, const COutFolders &outFolders)
		{
			if(!folders.IsEmpty()) {
				WriteByte(NID::kUnpackInfo);
				WriteByte(NID::kFolder);
				WriteNumber(folders.Size());
				{
					WriteByte(0);
					FOR_VECTOR(i, folders) {
						WriteFolder(folders[i]);
					}
				}
				WriteByte(NID::kCodersUnpackSize);
				FOR_VECTOR(i, outFolders.CoderUnpackSizes) {
					WriteNumber(outFolders.CoderUnpackSizes[i]);
				}
				WriteHashDigests(outFolders.FolderUnpackCRCs);
				WriteByte(NID::kEnd);
			}
		}

		void COutArchive::WriteSubStreamsInfo(const CObjectVector<CFolder> &folders, const COutFolders &outFolders,
			const CRecordVector<uint64> &unpackSizes, const CUInt32DefVector &digests)
		{
			const CRecordVector<CNum> &numUnpackStreamsInFolders = outFolders.NumUnpackStreamsVector;
			WriteByte(NID::kSubStreamsInfo);
			uint i;
			for(i = 0; i < numUnpackStreamsInFolders.Size(); i++)
				if(numUnpackStreamsInFolders[i] != 1) {
					WriteByte(NID::kNumUnpackStream);
					for(i = 0; i < numUnpackStreamsInFolders.Size(); i++)
						WriteNumber(numUnpackStreamsInFolders[i]);
					break;
				}
			for(i = 0; i < numUnpackStreamsInFolders.Size(); i++)
				if(numUnpackStreamsInFolders[i] > 1) {
					WriteByte(NID::kSize);
					CNum index = 0;
					for(i = 0; i < numUnpackStreamsInFolders.Size(); i++) {
						CNum num = numUnpackStreamsInFolders[i];
						for(CNum j = 0; j < num; j++) {
							if(j + 1 != num)
								WriteNumber(unpackSizes[index]);
							index++;
						}
					}
					break;
				}

			CUInt32DefVector digests2;
			unsigned digestIndex = 0;
			for(i = 0; i < folders.Size(); i++) {
				unsigned numSubStreams = (uint)numUnpackStreamsInFolders[i];
				if(numSubStreams == 1 && outFolders.FolderUnpackCRCs.ValidAndDefined(i))
					digestIndex++;
				else
					for(uint j = 0; j < numSubStreams; j++, digestIndex++) {
						digests2.Defs.Add(digests.Defs[digestIndex]);
						digests2.Vals.Add(digests.Vals[digestIndex]);
					}
			}
			WriteHashDigests(digests2);
			WriteByte(NID::kEnd);
		}

		// 7-Zip 4.50 - 4.58 contain BUG, so they do not support .7z archives with Unknown field.

		void COutArchive::SkipToAligned(unsigned pos, unsigned alignShifts)
		{
			if(_useAlign) {
				const uint alignSize = (uint)1 << alignShifts;
				pos += (uint)GetPos();
				pos &= (alignSize - 1);
				if(pos) {
					uint skip = alignSize - pos;
					if(skip < 2)
						skip += alignSize;
					skip -= 2;
					WriteByte(NID::kDummy);
					WriteByte((Byte)skip);
					for(uint i = 0; i < skip; i++)
						WriteByte(0);
				}
			}
		}

		void COutArchive::WriteAlignedBools(const CBoolVector & v, unsigned numDefined, Byte type, unsigned itemSizeShifts)
		{
			const uint bvSize = (numDefined == v.Size()) ? 0 : Bv_GetSizeInBytes(v);
			const uint64 dataSize = ((uint64)numDefined << itemSizeShifts) + bvSize + 2;
			SkipToAligned(3 + (uint)bvSize + (uint)GetBigNumberSize(dataSize), itemSizeShifts);
			WriteByte(type);
			WriteNumber(dataSize);
			if(numDefined == v.Size())
				WriteByte(1);
			else {
				WriteByte(0);
				WriteBoolVector(v);
			}
			WriteByte(0); // 0 means no switching to external stream
		}

		void COutArchive::WriteUInt64DefVector(const CUInt64DefVector &v, Byte type)
		{
			const uint numDefined = BoolVector_CountSum(v.Defs);
			if(numDefined) {
				WriteAlignedBools(v.Defs, numDefined, type, 3);
				for(uint i = 0; i < v.Defs.Size(); i++)
					if(v.Defs[i])
						WriteUInt64(v.Vals[i]);
			}
		}

		HRESULT COutArchive::EncodeStream(DECL_EXTERNAL_CODECS_LOC_VARS CEncoder &encoder, const CByteBuffer &data,
			CRecordVector<uint64> &packSizes, CObjectVector<CFolder> &folders, COutFolders &outFolders)
		{
			CBufInStream * streamSpec = new CBufInStream;
			CMyComPtr<ISequentialInStream> stream = streamSpec;
			streamSpec->Init(data, data.Size());
			outFolders.FolderUnpackCRCs.Defs.Add(true);
			outFolders.FolderUnpackCRCs.Vals.Add(CrcCalc(data, data.Size()));
			// outFolders.NumUnpackStreamsVector.Add(1);
			uint64 dataSize64 = data.Size();
			uint64 unpackSize;
			RINOK(encoder.Encode(EXTERNAL_CODECS_LOC_VARS stream, /*NULL,*/ &dataSize64, folders.AddNew(), outFolders.CoderUnpackSizes, unpackSize, SeqStream, packSizes, NULL))
			return S_OK;
		}

		void COutArchive::WriteHeader(const CArchiveDatabaseOut &db, /*const CHeaderOptions &headerOptions,*/ uint64 &headerOffset)
		{
			/*
			   bool thereIsSecure = (db.SecureBuf.Size() != 0);
			 */
			_useAlign = true;
			{
				uint64 packSize = 0;
				FOR_VECTOR(i, db.PackSizes) {
					packSize += db.PackSizes[i];
				}
				headerOffset = packSize;
			}
			WriteByte(NID::kHeader);
			// Archive Properties
			if(db.Folders.Size() > 0) {
				WriteByte(NID::kMainStreamsInfo);
				WritePackInfo(0, db.PackSizes, db.PackCRCs);
				WriteUnpackInfo(db.Folders, (const COutFolders &)db);

				CRecordVector<uint64> unpackSizes;
				CUInt32DefVector digests;
				FOR_VECTOR(i, db.Files) {
					const CFileItem & file = db.Files[i];
					if(!file.HasStream)
						continue;
					unpackSizes.Add(file.Size);
					digests.Defs.Add(file.CrcDefined);
					digests.Vals.Add(file.Crc);
				}
				WriteSubStreamsInfo(db.Folders, (const COutFolders &)db, unpackSizes, digests);
				WriteByte(NID::kEnd);
			}
			if(db.Files.IsEmpty()) {
				WriteByte(NID::kEnd);
				return;
			}
			WriteByte(NID::kFilesInfo);
			WriteNumber(db.Files.Size());
			{
				/* ---------- Empty Streams ---------- */
				CBoolVector emptyStreamVector;
				emptyStreamVector.ClearAndSetSize(db.Files.Size());
				uint numEmptyStreams = 0;
				{
					FOR_VECTOR(i, db.Files) {
						if(db.Files[i].HasStream)
							emptyStreamVector[i] = false;
						else {
							emptyStreamVector[i] = true;
							numEmptyStreams++;
						}
					}
				}
				if(numEmptyStreams != 0) {
					WritePropBoolVector(NID::kEmptyStream, emptyStreamVector);
					CBoolVector emptyFileVector, antiVector;
					emptyFileVector.ClearAndSetSize(numEmptyStreams);
					antiVector.ClearAndSetSize(numEmptyStreams);
					bool thereAreEmptyFiles = false, thereAreAntiItems = false;
					unsigned cur = 0;
					FOR_VECTOR(i, db.Files) {
						const CFileItem &file = db.Files[i];
						if(file.HasStream)
							continue;
						emptyFileVector[cur] = !file.IsDir;
						if(!file.IsDir)
							thereAreEmptyFiles = true;
						bool isAnti = db.IsItemAnti(i);
						antiVector[cur] = isAnti;
						if(isAnti)
							thereAreAntiItems = true;
						cur++;
					}

					if(thereAreEmptyFiles)
						WritePropBoolVector(NID::kEmptyFile, emptyFileVector);
					if(thereAreAntiItems)
						WritePropBoolVector(NID::kAnti, antiVector);
				}
			}
			{
				/* ---------- Names ---------- */
				uint   numDefined = 0;
				size_t namesDataSize = 0;
				FOR_VECTOR(i, db.Files) {
					const UString &name = db.Names[i];
					if(!name.IsEmpty())
						numDefined++;
					namesDataSize += (name.Len() + 1) * 2;
				}
				if(numDefined > 0) {
					namesDataSize++;
					SkipToAligned(2 + GetBigNumberSize(namesDataSize), 4);
					WriteByte(NID::kName);
					WriteNumber(namesDataSize);
					WriteByte(0);
					FOR_VECTOR(i, db.Files) {
						const UString &name = db.Names[i];
						for(uint t = 0; t <= name.Len(); t++) {
							wchar_t c = name[t];
							WriteByte((Byte)c);
							WriteByte((Byte)(c >> 8));
						}
					}
				}
			}
			/* if(headerOptions.WriteCTime) */ WriteUInt64DefVector(db.CTime, NID::kCTime);
			/* if(headerOptions.WriteATime) */ WriteUInt64DefVector(db.ATime, NID::kATime);
			/* if(headerOptions.WriteMTime) */ WriteUInt64DefVector(db.MTime, NID::kMTime);
			WriteUInt64DefVector(db.StartPos, NID::kStartPos);
			{
				/* ---------- Write Attrib ---------- */
				const uint numDefined = BoolVector_CountSum(db.Attrib.Defs);
				if(numDefined != 0) {
					WriteAlignedBools(db.Attrib.Defs, numDefined, NID::kWinAttrib, 2);
					FOR_VECTOR(i, db.Attrib.Defs) {
						if(db.Attrib.Defs[i])
							WriteUInt32(db.Attrib.Vals[i]);
					}
				}
			}
			/*
			   {
			   // ---------- Write IsAux ----------
			   if(BoolVector_CountSum(db.IsAux) != 0)
				WritePropBoolVector(NID::kIsAux, db.IsAux);
			   }

			   {
			   // ---------- Write Parent ----------
			   CBoolVector boolVector;
			   boolVector.Reserve(db.Files.Size());
			   unsigned numIsDir = 0;
			   unsigned numParentLinks = 0;
			   for(i = 0; i < db.Files.Size(); i++)
			   {
				const CFileItem &file = db.Files[i];
				bool defined = !file.IsAltStream;
				boolVector.Add(defined);
				if(defined)
				  numIsDir++;
				if(file.Parent >= 0)
				  numParentLinks++;
			   }
			   if(numParentLinks > 0)
			   {
				// WriteAlignedBools(boolVector, numDefined, NID::kParent, 2);
				const uint bvSize = (numIsDir == boolVector.Size()) ? 0 : Bv_GetSizeInBytes(boolVector);
				const uint64 dataSize = (uint64)db.Files.Size() * 4 + bvSize + 1;
				SkipToAligned(2 + (uint)bvSize + (uint)GetBigNumberSize(dataSize), 2);

				WriteByte(NID::kParent);
				WriteNumber(dataSize);
				if(numIsDir == boolVector.Size())
				  WriteByte(1);
				else
				{
				  WriteByte(0);
				  WriteBoolVector(boolVector);
				}
				for(i = 0; i < db.Files.Size(); i++)
				{
				  const CFileItem &file = db.Files[i];
				  // if(file.Parent >= 0)
					WriteUInt32(file.Parent);
				}
			   }
			   }

			   if(thereIsSecure)
			   {
			   uint64 secureDataSize = 1 + 4 +
				 db.SecureBuf.Size() +
				 db.SecureSizes.Size() * 4;
			   // secureDataSize += db.SecureIDs.Size() * 4;
			   for(i = 0; i < db.SecureIDs.Size(); i++)
				secureDataSize += GetBigNumberSize(db.SecureIDs[i]);
			   SkipToAligned(2 + GetBigNumberSize(secureDataSize), 2);
			   WriteByte(NID::kNtSecure);
			   WriteNumber(secureDataSize);
			   WriteByte(0);
			   WriteUInt32(db.SecureSizes.Size());
			   for(i = 0; i < db.SecureSizes.Size(); i++)
				WriteUInt32(db.SecureSizes[i]);
			   WriteBytes(db.SecureBuf, db.SecureBuf.Size());
			   for(i = 0; i < db.SecureIDs.Size(); i++)
			   {
				WriteNumber(db.SecureIDs[i]);
				// WriteUInt32(db.SecureIDs[i]);
			   }
			   }
			 */

			WriteByte(NID::kEnd); // for files
			WriteByte(NID::kEnd); // for headers
		}

		HRESULT COutArchive::WriteDatabase(DECL_EXTERNAL_CODECS_LOC_VARS const CArchiveDatabaseOut &db, const CCompressionMethodMode * options, const CHeaderOptions &headerOptions)
		{
			if(!db.CheckNumFiles())
				return E_FAIL;
			uint64 headerOffset;
			uint32 headerCRC;
			uint64 headerSize;
			if(db.IsEmpty()) {
				headerSize = 0;
				headerOffset = 0;
				headerCRC = CrcCalc(0, 0);
			}
			else {
				bool encodeHeaders = false;
				if(options != 0)
					if(options->IsEmpty())
						options = 0;
				if(options != 0)
					if(options->PasswordIsDefined || headerOptions.CompressMainHeader)
						encodeHeaders = true;
				_outByte.SetStream(SeqStream);
				_outByte.Init();
				_crc = CRC_INIT_VAL;
				_countMode = encodeHeaders;
				_writeToStream = true;
				_countSize = 0;
				WriteHeader(db, /* headerOptions, */ headerOffset);
				if(encodeHeaders) {
					CByteBuffer buf(_countSize);
					_outByte2.Init((Byte*)buf, _countSize);
					_countMode = false;
					_writeToStream = false;
					WriteHeader(db, /* headerOptions, */ headerOffset);

					if(_countSize != _outByte2.GetPos())
						return E_FAIL;

					CCompressionMethodMode encryptOptions;
					encryptOptions.PasswordIsDefined = options->PasswordIsDefined;
					encryptOptions.Password = options->Password;
					CEncoder encoder(headerOptions.CompressMainHeader ? *options : encryptOptions);
					CRecordVector<uint64> packSizes;
					CObjectVector<CFolder> folders;
					COutFolders outFolders;
					RINOK(EncodeStream(EXTERNAL_CODECS_LOC_VARS encoder, buf, packSizes, folders, outFolders));
					_writeToStream = true;
					if(folders.Size() == 0)
						throw 1;
					WriteID(NID::kEncodedHeader);
					WritePackInfo(headerOffset, packSizes, CUInt32DefVector());
					WriteUnpackInfo(folders, outFolders);
					WriteByte(NID::kEnd);
					FOR_VECTOR(i, packSizes) {
						headerOffset += packSizes[i];
					}
				}
				RINOK(_outByte.Flush());
				headerCRC = CRC_GET_DIGEST(_crc);
				headerSize = _outByte.GetProcessedSize();
			}
		  #ifdef _7Z_VOL
			if(_endMarker) {
				CFinishHeader h;
				h.NextHeaderSize = headerSize;
				h.NextHeaderCRC = headerCRC;
				h.NextHeaderOffset = uint64(0) - (headerSize + 4 + kFinishHeaderSize);
				h.ArchiveStartOffset = h.NextHeaderOffset - headerOffset;
				h.AdditionalStartBlockSize = 0;
				RINOK(WriteFinishHeader(h));
				return WriteFinishSignature();
			}
			else
		  #endif
			{
				CStartHeader h;
				h.NextHeaderSize = headerSize;
				h.NextHeaderCRC = headerCRC;
				h.NextHeaderOffset = headerOffset;
				RINOK(Stream->Seek(_prefixHeaderPos, STREAM_SEEK_SET, NULL));
				return WriteStartHeader(h);
			}
		}

		void CUInt32DefVector::SetItem(uint index, bool defined, uint32 value)
		{
			while(index >= Defs.Size())
				Defs.Add(false);
			Defs[index] = defined;
			if(!defined)
				return;
			while(index >= Vals.Size())
				Vals.Add(0);
			Vals[index] = value;
		}

		void CUInt64DefVector::SetItem(uint index, bool defined, uint64 value)
		{
			while(index >= Defs.Size())
				Defs.Add(false);
			Defs[index] = defined;
			if(!defined)
				return;
			while(index >= Vals.Size())
				Vals.Add(0);
			Vals[index] = value;
		}

		void CArchiveDatabaseOut::Clear()
		{
			OutFoldersClear();
			PackSizes.Clear();
			PackCRCs.Clear();
			Folders.Clear();
			Files.Clear();
			Names.Clear();
			CTime.Clear();
			ATime.Clear();
			MTime.Clear();
			StartPos.Clear();
			Attrib.Clear();
			IsAnti.Clear();
			/*
				IsAux.Clear();
				ClearSecure();
				*/
		}

		void CArchiveDatabaseOut::ReserveDown()
		{
			OutFoldersReserveDown();
			PackSizes.ReserveDown();
			PackCRCs.ReserveDown();
			Folders.ReserveDown();
			Files.ReserveDown();
			Names.ReserveDown();
			CTime.ReserveDown();
			ATime.ReserveDown();
			MTime.ReserveDown();
			StartPos.ReserveDown();
			Attrib.ReserveDown();
			IsAnti.ReserveDown();
			/*
				IsAux.ReserveDown();
				*/
		}

		bool CArchiveDatabaseOut::IsEmpty() const
		{
			return (PackSizes.IsEmpty() && NumUnpackStreamsVector.IsEmpty() && Folders.IsEmpty() && Files.IsEmpty());
		}

		bool CArchiveDatabaseOut::CheckNumFiles() const
		{
			uint size = Files.Size();
			return (CTime.CheckSize(size) && ATime.CheckSize(size) && MTime.CheckSize(size) && 
				StartPos.CheckSize(size) && Attrib.CheckSize(size) && (size == IsAnti.Size() || IsAnti.Size() == 0));
		}

		bool CArchiveDatabaseOut::IsItemAnti(uint index) const 
		{
			return (index < IsAnti.Size() && IsAnti[index]);
		}

		// bool CArchiveDatabaseOut::IsItemAux(uint index) const { return (index < IsAux.Size() && IsAux[index]); }
		void CArchiveDatabaseOut::SetItem_Anti(uint index, bool isAnti)
		{
			while(index >= IsAnti.Size())
				IsAnti.Add(false);
			IsAnti[index] = isAnti;
		}

		void CArchiveDatabaseOut::AddFile(const CFileItem &file, const CFileItem2 &file2, const UString &name)
		{
			uint   index = Files.Size();
			CTime.SetItem(index, file2.CTimeDefined, file2.CTime);
			ATime.SetItem(index, file2.ATimeDefined, file2.ATime);
			MTime.SetItem(index, file2.MTimeDefined, file2.MTime);
			StartPos.SetItem(index, file2.StartPosDefined, file2.StartPos);
			Attrib.SetItem(index, file2.AttribDefined, file2.Attrib);
			SetItem_Anti(index, file2.IsAnti);
			// SetItem_Aux(index, file2.IsAux);
			Names.Add(name);
			Files.Add(file);
		}
		//
		// Properties
		//
		// #define _MULTI_PACK
		//
		struct CPropMap {
			uint32 FilePropID;
			CStatProp StatProp;
		};

		static const CPropMap kPropMap[] = {
			{ NID::kName, { NULL, kpidPath, VT_BSTR } },
			{ NID::kSize, { NULL, kpidSize, VT_UI8 } },
			{ NID::kPackInfo, { NULL, kpidPackSize, VT_UI8 } },
		#ifdef _MULTI_PACK
			{ 100, { "Pack0", kpidPackedSize0, VT_UI8 } },
			{ 101, { "Pack1", kpidPackedSize1, VT_UI8 } },
			{ 102, { "Pack2", kpidPackedSize2, VT_UI8 } },
			{ 103, { "Pack3", kpidPackedSize3, VT_UI8 } },
			{ 104, { "Pack4", kpidPackedSize4, VT_UI8 } },
		#endif
			{ NID::kCTime, { NULL, kpidCTime, VT_FILETIME } },
			{ NID::kMTime, { NULL, kpidMTime, VT_FILETIME } },
			{ NID::kATime, { NULL, kpidATime, VT_FILETIME } },
			{ NID::kWinAttrib, { NULL, kpidAttrib, VT_UI4 } },
			{ NID::kStartPos, { NULL, kpidPosition, VT_UI8 } },
			{ NID::kCRC, { NULL, kpidCRC, VT_UI4 } },
		//  { NID::kIsAux, { NULL, kpidIsAux, VT_BOOL } },
			{ NID::kAnti, { NULL, kpidIsAnti, VT_BOOL } }
		#ifndef _SFX
			,
			{ 97, { NULL, kpidEncrypted, VT_BOOL } },
			{ 98, { NULL, kpidMethod, VT_BSTR } },
			{ 99, { NULL, kpidBlock, VT_UI4 } }
		#endif
		};

		static void FASTCALL CopyOneItem(CRecordVector<uint64> & src, CRecordVector<uint64> & dest, uint32 item)
		{
			FOR_VECTOR(i, src) {
				if(src[i] == item) {
					dest.Add(item);
					src.Delete(i);
					return;
				}
			}
		}

		static void FASTCALL RemoveOneItem(CRecordVector <uint64> &src, uint32 item)
		{
			FOR_VECTOR(i, src) {
				if(src[i] == item) {
					src.Delete(i);
					return;
				}
			}
		}

		static void FASTCALL InsertToHead(CRecordVector <uint64> &dest, uint32 item)
		{
			FOR_VECTOR(i, dest) {
				if(dest[i] == item) {
					dest.Delete(i);
					break;
				}
			}
			dest.Insert(0, item);
		}

		#define COPY_ONE_ITEM(id) CopyOneItem(fileInfoPopIDs, _fileInfoPopIDs, NID::id);

		void CHandler::FillPopIDs()
		{
			_fileInfoPopIDs.Clear();
		  #ifdef _7Z_VOL
			if(_volumes.Size() < 1)
				return;
			const CVolume &volume = _volumes.Front();
			const CArchiveDatabaseEx &_db = volume.Database;
		  #endif

			CRecordVector <uint64> fileInfoPopIDs = _db.ArcInfo.FileInfoPopIDs;
			RemoveOneItem(fileInfoPopIDs, NID::kEmptyStream);
			RemoveOneItem(fileInfoPopIDs, NID::kEmptyFile);
			/*
			   RemoveOneItem(fileInfoPopIDs, NID::kParent);
			   RemoveOneItem(fileInfoPopIDs, NID::kNtSecure);
			 */
			COPY_ONE_ITEM(kName);
			COPY_ONE_ITEM(kAnti);
			COPY_ONE_ITEM(kSize);
			COPY_ONE_ITEM(kPackInfo);
			COPY_ONE_ITEM(kCTime);
			COPY_ONE_ITEM(kMTime);
			COPY_ONE_ITEM(kATime);
			COPY_ONE_ITEM(kWinAttrib);
			COPY_ONE_ITEM(kCRC);
			COPY_ONE_ITEM(kComment);
			_fileInfoPopIDs += fileInfoPopIDs;
		  #ifndef _SFX
			_fileInfoPopIDs.Add(97);
			_fileInfoPopIDs.Add(98);
			_fileInfoPopIDs.Add(99);
		  #endif

		  #ifdef _MULTI_PACK
			_fileInfoPopIDs.Add(100);
			_fileInfoPopIDs.Add(101);
			_fileInfoPopIDs.Add(102);
			_fileInfoPopIDs.Add(103);
			_fileInfoPopIDs.Add(104);
		  #endif

		  #ifndef _SFX
			InsertToHead(_fileInfoPopIDs, NID::kMTime);
			InsertToHead(_fileInfoPopIDs, NID::kPackInfo);
			InsertToHead(_fileInfoPopIDs, NID::kSize);
			InsertToHead(_fileInfoPopIDs, NID::kName);
		  #endif
		}

		STDMETHODIMP CHandler::GetNumberOfProperties(uint32 * numProps)
		{
			*numProps = _fileInfoPopIDs.Size();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetPropertyInfo(uint32 index, BSTR * name, PROPID * propID, VARTYPE * varType)
		{
			if(index < _fileInfoPopIDs.Size()) {
				uint64 id = _fileInfoPopIDs[index];
				for(uint i = 0; i < ARRAY_SIZE(kPropMap); i++) {
					const CPropMap & pr = kPropMap[i];
					if(pr.FilePropID == id) {
						const CStatProp & st = pr.StatProp;
						*propID = st.PropID;
						*varType = st.vt;
						/*
						   if(st.lpwstrName)
						   *name = ::SysAllocString(st.lpwstrName);
						   else
						 */
						*name = NULL;
						return S_OK;
					}
				}
			}
			return E_INVALIDARG;
		}
		//
		// Update
		//
		#define k_X86 k_BCJ

		struct CFilterMode {
			CFilterMode() : Id(0), Delta(0) 
			{
			}
			void SetDelta()
			{
				if(Id == k_IA64)
					Delta = 16;
				else if(Id == k_ARM || Id == k_PPC || Id == k_SPARC)
					Delta = 4;
				else if(Id == k_ARMT)
					Delta = 2;
				else
					Delta = 0;
			}
			uint32 Id;
			uint32 Delta;
		};
		// 
		// PE 
		// 
		#define MZ_SIG 0x5A4D
		#define PE_SIG 0x00004550
		#define PE_OptHeader_Magic_32 0x10B
		#define PE_OptHeader_Magic_64 0x20B
		#define PE_SectHeaderSize 40
		#define PE_SECT_EXECUTE 0x20000000

		static int Parse_EXE(const Byte * buf, size_t size, CFilterMode * filterMode)
		{
			if(size < 512 || GetUi16(buf) != MZ_SIG)
				return 0;
			const Byte * p;
			uint32 peOffset, optHeaderSize, filterId;
			peOffset = GetUi32(buf + 0x3C);
			if(peOffset >= 0x1000 || peOffset + 512 > size || (peOffset & 7) != 0)
				return 0;
			p = buf + peOffset;
			if(GetUi32(p) != PE_SIG)
				return 0;
			p += 4;
			switch(GetUi16(p)) {
				case 0x014C:
				case 0x8664:  filterId = k_X86; break;
				/*
				   IMAGE_FILE_MACHINE_ARM   0x01C0  // ARM LE
				   IMAGE_FILE_MACHINE_THUMB 0x01C2  // ARM Thumb / Thumb-2 LE
				   IMAGE_FILE_MACHINE_ARMNT 0x01C4  // ARM Thumb-2, LE
				   Note: We use ARM filter for 0x01C2. (WinCE 5 - 0x01C2) files mostly contain ARM code (not Thumb/Thumb-2).
				 */
				case 0x01C0:                // WinCE old
				case 0x01C2:  filterId = k_ARM; break; // WinCE new
				case 0x01C4:  filterId = k_ARMT; break; // WinRT
				case 0x0200:  filterId = k_IA64; break;
				default:  return 0;
			}
			optHeaderSize = GetUi16(p + 16);
			if(optHeaderSize > (1 << 10))
				return 0;
			p += 20; /* headerSize */
			switch(GetUi16(p)) {
				case PE_OptHeader_Magic_32:
				case PE_OptHeader_Magic_64: break;
				default: return 0;
			}
			filterMode->Id = filterId;
			return 1;
		}
		// 
		// ELF
		// 
		#define ELF_SIG 0x464C457F
		#define ELF_CLASS_32  1
		#define ELF_CLASS_64  2
		#define ELF_DATA_2LSB 1
		#define ELF_DATA_2MSB 2

		static uint16 FASTCALL Get16__(const Byte * p, Bool be) { return be ? (uint16)GetBe16(p) :(uint16)GetUi16(p); }
		static uint32 FASTCALL Get32__(const Byte * p, Bool be) { return be ? GetBe32(p) : GetUi32(p); }

		// static uint64 Get64(const Byte *p, Bool be) { if(be) return GetBe64(p); return GetUi64(p); }

		static int Parse_ELF(const Byte * buf, size_t size, CFilterMode * filterMode)
		{
			Bool /* is32, */ be;
			uint32 filterId;
			if(size < 512 || buf[6] != 1) /* ver */
				return 0;
			if(GetUi32(buf) != ELF_SIG)
				return 0;
			switch(buf[4]) {
				case ELF_CLASS_32: /* is32 = True; */ break;
				case ELF_CLASS_64: /* is32 = False; */ break;
				default: return 0;
			}
			switch(buf[5]) {
				case ELF_DATA_2LSB: be = False; break;
				case ELF_DATA_2MSB: be = True; break;
				default: return 0;
			}
			switch(Get16__(buf + 0x12, be)) {
				case 3:
				case 6:
				case 62: filterId = k_X86; break;
				case 2:
				case 18:
				case 43: filterId = k_SPARC; break;
				case 20:
				case 21: if(!be) return 0; filterId = k_PPC; break;
				case 40: if(be) return 0; filterId = k_ARM; break;
				/* Some IA-64 ELF exacutable have size that is not aligned for 16 bytes.
				   So we don't use IA-64 filter for IA-64 ELF */
				// case 50: if( be) return 0; filterId = k_IA64; break;
				default: return 0;
			}
			filterMode->Id = filterId;
			return 1;
		}
		// 
		// Mach-O
		// 
		#define MACH_SIG_BE_32 0xCEFAEDFE
		#define MACH_SIG_BE_64 0xCFFAEDFE
		#define MACH_SIG_LE_32 0xFEEDFACE
		#define MACH_SIG_LE_64 0xFEEDFACF

		#define MACH_ARCH_ABI64 (1 << 24)
		#define MACH_MACHINE_386 7
		#define MACH_MACHINE_ARM 12
		#define MACH_MACHINE_SPARC 14
		#define MACH_MACHINE_PPC 18
		#define MACH_MACHINE_PPC64 (MACH_ARCH_ABI64 | MACH_MACHINE_PPC)
		#define MACH_MACHINE_AMD64 (MACH_ARCH_ABI64 | MACH_MACHINE_386)

		static unsigned Parse_MACH(const Byte * buf, size_t size, CFilterMode * filterMode)
		{
			uint32 filterId, numCommands, commandsSize;
			if(size < 512)
				return 0;
			Bool /* mode64, */ be;
			switch(GetUi32(buf)) {
				case MACH_SIG_BE_32: /* mode64 = False; */ be = True; break;
				case MACH_SIG_BE_64: /* mode64 = True;  */ be = True; break;
				case MACH_SIG_LE_32: /* mode64 = False; */ be = False; break;
				case MACH_SIG_LE_64: /* mode64 = True;  */ be = False; break;
				default: return 0;
			}
			switch(Get32__(buf + 4, be)) {
				case MACH_MACHINE_386:
				case MACH_MACHINE_AMD64: filterId = k_X86; break;
				case MACH_MACHINE_ARM:   if(be) return 0; filterId = k_ARM; break;
				case MACH_MACHINE_SPARC: if(!be) return 0; filterId = k_SPARC; break;
				case MACH_MACHINE_PPC:
				case MACH_MACHINE_PPC64: if(!be) return 0; filterId = k_PPC; break;
				default: return 0;
			}
			numCommands = Get32__(buf + 0x10, be);
			commandsSize = Get32__(buf + 0x14, be);
			if(commandsSize > (1 << 24) || numCommands > (1 << 18))
				return 0;
			filterMode->Id = filterId;
			return 1;
		}
		// 
		// WAV
		// 
		#define WAV_SUBCHUNK_fmt  0x20746D66
		#define WAV_SUBCHUNK_data 0x61746164

		#define RIFF_SIG 0x46464952

		static Bool Parse_WAV(const Byte * buf, size_t size, CFilterMode * filterMode)
		{
			uint32 subChunkSize, pos;
			if(size < 0x2C)
				return False;
			if(GetUi32(buf + 0) != RIFF_SIG || GetUi32(buf + 8) != 0x45564157 || /*WAVE*/ GetUi32(buf + 0xC) != WAV_SUBCHUNK_fmt)
				return False;
			subChunkSize = GetUi32(buf + 0x10);
			/* [0x14 = format] = 1 (PCM) */
			if(subChunkSize < 0x10 || subChunkSize > 0x12 || GetUi16(buf + 0x14) != 1)
				return False;
			unsigned numChannels = GetUi16(buf + 0x16);
			unsigned bitsPerSample = GetUi16(buf + 0x22);
			if((bitsPerSample & 0x7) != 0 || bitsPerSample >= 256 || numChannels >= 256)
				return False;
			pos = 0x14 + subChunkSize;
			const int kNumSubChunksTests = 10;
			// Do we need to scan more than 3 sub-chunks?
			for(int i = 0; i < kNumSubChunksTests; i++) {
				if(pos + 8 > size)
					return False;
				subChunkSize = GetUi32(buf + pos + 4);
				if(GetUi32(buf + pos) == WAV_SUBCHUNK_data) {
					unsigned delta = numChannels * (bitsPerSample >> 3);
					if(delta >= 256)
						return False;
					filterMode->Id = k_Delta;
					filterMode->Delta = delta;
					return True;
				}
				if(subChunkSize > (1 << 16))
					return False;
				pos += subChunkSize + 8;
			}
			return False;
		}

		static Bool ParseFile(const Byte * buf, size_t size, CFilterMode * filterMode)
		{
			filterMode->Id = 0;
			filterMode->Delta = 0;
			if(Parse_EXE(buf, size, filterMode)) return True;
			if(Parse_ELF(buf, size, filterMode)) return True;
			if(Parse_MACH(buf, size, filterMode)) return True;
			return Parse_WAV(buf, size, filterMode);
		}

		struct CFilterMode2 : public CFilterMode {
			bool   Encrypted;
			uint   GroupIndex;

			CFilterMode2() : Encrypted(false), GroupIndex(0)
			{
			}
			int Compare(const CFilterMode2 &m) const
			{
				if(!Encrypted) {
					if(m.Encrypted)
						return -1;
				}
				else if(!m.Encrypted)
					return 1;
				if(Id < m.Id) return -1;
				if(Id > m.Id) return 1;
				if(Delta < m.Delta) return -1;
				if(Delta > m.Delta) return 1;
				return 0;
			}
			bool operator == (const CFilterMode2 &m) const { return Id == m.Id && Delta == m.Delta && Encrypted == m.Encrypted; }
		};

		static unsigned GetGroup(CRecordVector<CFilterMode2> &filters, const CFilterMode2 &m)
		{
			uint i;
			for(i = 0; i < filters.Size(); i++) {
				const CFilterMode2 &m2 = filters[i];
				if(m == m2)
					return i;
				/*
				   if(m.Encrypted != m2.Encrypted) {
				   if(!m.Encrypted)
					break;
				   continue;
				   }
				   if(m.Id < m2.Id)  break;
				   if(m.Id != m2.Id) continue;
				   if(m.Delta < m2.Delta) break;
				   if(m.Delta != m2.Delta) continue;
				 */
			}
			// filters.Insert(i, m);
			// return i;
			return filters.Add(m);
		}
		static inline bool Is86Filter(CMethodId m)
		{
			return (m == k_BCJ || m == k_BCJ2);
		}
		static inline bool IsExeFilter(CMethodId m)
		{
			switch(m) {
				case k_BCJ:
				case k_BCJ2:
				case k_ARM:
				case k_ARMT:
				case k_PPC:
				case k_SPARC:
				case k_IA64:
					return true;
			}
			return false;
		}
		static unsigned Get_FilterGroup_for_Folder(CRecordVector<CFilterMode2> &filters, const CFolderEx &f, bool extractFilter)
		{
			CFilterMode2 m;
			m.Id = 0;
			m.Delta = 0;
			m.Encrypted = f.IsEncrypted();
			if(extractFilter) {
				const CCoderInfo &coder = f.Coders[f.UnpackCoder];
				if(coder.MethodID == k_Delta) {
					if(coder.Props.Size() == 1) {
						m.Delta = (uint)coder.Props[0] + 1;
						m.Id = k_Delta;
					}
				}
				else if(IsExeFilter(coder.MethodID)) {
					m.Id = (uint32)coder.MethodID;
					if(m.Id == k_BCJ2)
						m.Id = k_BCJ;
					m.SetDelta();
				}
			}
			return GetGroup(filters, m);
		}
		static HRESULT WriteRange(IInStream * inStream, ISequentialOutStream * outStream, uint64 position, uint64 size, ICompressProgressInfo * progress)
		{
			RINOK(inStream->Seek(position, STREAM_SEEK_SET, 0));
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
			streamSpec->SetStream(inStream);
			streamSpec->Init(size);

			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			RINOK(copyCoder->Code(inStreamLimited, outStream, NULL, NULL, progress));
			return (copyCoderSpec->TotalSize == size ? S_OK : E_FAIL);
		}

		/*
		   unsigned CUpdateItem::GetExtensionPos() const
		   {
		   int slashPos = Name.ReverseFind_PathSepar();
		   int dotPos = Name.ReverseFind_Dot();
		   if(dotPos <= slashPos)
			return Name.Len();
		   return dotPos + 1;
		   }

		   UString CUpdateItem::GetExtension() const
		   {
		   return Name.Ptr(GetExtensionPos());
		   }
		 */

		#define RINOZ(x) { int __tt = (x); if(__tt != 0) return __tt; }
		#define RINOZ_COMP(a, b) RINOZ(MyCompare(a, b))
		/*
		   static int CompareBuffers(const CByteBuffer &a1, const CByteBuffer &a2)
		   {
		   size_t c1 = a1.GetCapacity();
		   size_t c2 = a2.GetCapacity();
		   RINOZ_COMP(c1, c2);
		   for(size_t i = 0; i < c1; i++)
			RINOZ_COMP(a1[i], a2[i]);
		   return 0;
		   }

		   static int CompareCoders(const CCoderInfo &c1, const CCoderInfo &c2)
		   {
		   RINOZ_COMP(c1.NumInStreams, c2.NumInStreams);
		   RINOZ_COMP(c1.NumOutStreams, c2.NumOutStreams);
		   RINOZ_COMP(c1.MethodID, c2.MethodID);
		   return CompareBuffers(c1.Props, c2.Props);
		   }

		   static int CompareBonds(const CBond &b1, const CBond &b2)
		   {
		   RINOZ_COMP(b1.InIndex, b2.InIndex);
		   return MyCompare(b1.OutIndex, b2.OutIndex);
		   }

		   static int CompareFolders(const CFolder &f1, const CFolder &f2)
		   {
		   int s1 = f1.Coders.Size();
		   int s2 = f2.Coders.Size();
		   RINOZ_COMP(s1, s2);
		   int i;
		   for(i = 0; i < s1; i++)
			RINOZ(CompareCoders(f1.Coders[i], f2.Coders[i]));
		   s1 = f1.Bonds.Size();
		   s2 = f2.Bonds.Size();
		   RINOZ_COMP(s1, s2);
		   for(i = 0; i < s1; i++)
			RINOZ(CompareBonds(f1.Bonds[i], f2.Bonds[i]));
		   return 0;
		   }
		 */

		/*
		   static int CompareFiles(const CFileItem &f1, const CFileItem &f2)
		   {
		   return CompareFileNames(f1.Name, f2.Name);
		   }
		 */

		struct CFolderRepack {
			unsigned FolderIndex;
			CNum NumCopyFiles;
		};

		/*
		   static int CompareFolderRepacks(const CFolderRepack *p1, const CFolderRepack *p2, void *)
		   {
		   int i1 = p1->FolderIndex;
		   int i2 = p2->FolderIndex;
		   // In that version we don't want to parse folders here, so we don't compare folders
		   // probably it must be improved in future
		   // const CDbEx &db = *(const CDbEx *)param;
		   // RINOZ(CompareFolders(
		   //     db.Folders[i1],
		   //     db.Folders[i2]));

		   return MyCompare(i1, i2);

		   // RINOZ_COMP(
		   //     db.NumUnpackStreamsVector[i1],
		   //     db.NumUnpackStreamsVector[i2]);
		   // if(db.NumUnpackStreamsVector[i1] == 0)
		   //   return 0;
		   // return CompareFiles(
		   //     db.Files[db.FolderStartFileIndex[i1]],
		   //     db.Files[db.FolderStartFileIndex[i2]]);
		   }
		 */

		/*
		   we sort empty files and dirs in such order:
		   - Dir.NonAnti   (name sorted)
		   - File.NonAnti  (name sorted)
		   - File.Anti     (name sorted)
		   - Dir.Anti (reverse name sorted)
		 */

		static int CompareEmptyItems(const uint * p1, const uint * p2, void * param)
		{
			const CObjectVector<CUpdateItem> &updateItems = *(const CObjectVector<CUpdateItem> *)param;
			const CUpdateItem &u1 = updateItems[*p1];
			const CUpdateItem &u2 = updateItems[*p2];
			// NonAnti < Anti
			if(u1.IsAnti != u2.IsAnti)
				return (u1.IsAnti ? 1 : -1);
			if(u1.IsDir != u2.IsDir) {
				// Dir.NonAnti < File < Dir.Anti
				if(u1.IsDir)
					return (u1.IsAnti ? 1 : -1);
				return (u2.IsAnti ? -1 : 1);
			}
			int n = CompareFileNames(u1.Name, u2.Name);
			return (u1.IsDir && u1.IsAnti) ? -n : n;
		}

		static const char * g_Exts =
			" 7z xz lzma ace arc arj bz tbz bz2 tbz2 cab deb gz tgz ha lha lzh lzo lzx pak rar rpm sit zoo"
			" zip jar ear war msi"
			" 3gp avi mov mpeg mpg mpe wmv"
			" aac ape fla flac la mp3 m4a mp4 ofr ogg pac ra rm rka shn swa tta wv wma wav"
			" swf"
			" chm hxi hxs"
			" gif jpeg jpg jp2 png tiff  bmp ico psd psp"
			" awg ps eps cgm dxf svg vrml wmf emf ai md"
			" cad dwg pps key sxi"
			" max 3ds"
			" iso bin nrg mdf img pdi tar cpio xpi"
			" vfd vhd vud vmc vsv"
			" vmdk dsk nvram vmem vmsd vmsn vmss vmtm"
			" inl inc idl acf asa"
			" h hpp hxx c cpp cxx m mm go swift"
			" rc java cs rs pas bas vb cls ctl frm dlg def"
			" f77 f f90 f95"
			" asm s"
			" sql manifest dep"
			" mak clw csproj vcproj sln dsp dsw"
			" class"
			" bat cmd bash sh"
			" xml xsd xsl xslt hxk hxc htm html xhtml xht mht mhtml htw asp aspx css cgi jsp shtml"
			" awk sed hta js json php php3 php4 php5 phptml pl pm py pyo rb tcl ts vbs"
			" text txt tex ans asc srt reg ini doc docx mcw dot rtf hlp xls xlr xlt xlw ppt pdf"
			" sxc sxd sxi sxg sxw stc sti stw stm odt ott odg otg odp otp ods ots odf"
			" abw afp cwk lwp wpd wps wpt wrf wri"
			" abf afm bdf fon mgf otf pcf pfa snf ttf"
			" dbf mdb nsf ntf wdb db fdb gdb"
			" exe dll ocx vbx sfx sys tlb awx com obj lib out o so"
			" pdb pch idb ncb opt";

		static unsigned GetExtIndex(const char * ext)
		{
			unsigned extIndex = 1;
			const char * p = g_Exts;
			for(;; ) {
				char c = *p++;
				if(c == 0)
					return extIndex;
				if(c == ' ')
					continue;
				unsigned pos = 0;
				for(;; ) {
					char c2 = ext[pos++];
					if(c2 == 0 && (c == 0 || c == ' '))
						return extIndex;
					if(c != c2)
						break;
					c = *p++;
				}
				extIndex++;
				for(;; ) {
					if(c == 0)
						return extIndex;
					if(c == ' ')
						break;
					c = *p++;
				}
			}
		}

		struct CRefItem {
			const  CUpdateItem * UpdateItem;
			uint32 Index;
			uint   ExtensionPos;
			uint   NamePos;
			uint   ExtensionIndex;

			CRefItem() 
			{
			}
			CRefItem(uint32 index, const CUpdateItem &ui, bool sortByType) : UpdateItem(&ui), Index(index), ExtensionPos(0), NamePos(0), ExtensionIndex(0)
			{
				if(sortByType) {
					int slashPos = ui.Name.ReverseFind_PathSepar();
					NamePos = slashPos + 1;
					int dotPos = ui.Name.ReverseFind_Dot();
					if(dotPos <= slashPos)
						ExtensionPos = ui.Name.Len();
					else {
						ExtensionPos = dotPos + 1;
						if(ExtensionPos != ui.Name.Len()) {
							AString s;
							for(uint pos = ExtensionPos;; pos++) {
								wchar_t c = ui.Name[pos];
								if(c >= 0x80)
									break;
								if(c == 0) {
									ExtensionIndex = GetExtIndex(s);
									break;
								}
								s += (char)MyCharLower_Ascii((char)c);
							}
						}
					}
				}
			}
		};

		struct CSortParam {
			// const CObjectVector<CTreeFolder> *TreeFolders;
			bool SortByType;
		};

		/*
		   we sort files in such order:
		   - Dir.NonAnti   (name sorted)
		   - alt streams
		   - Dirs
		   - Dir.Anti (reverse name sorted)
		 */

		static int CompareUpdateItems(const CRefItem * p1, const CRefItem * p2, void * param)
		{
			const CRefItem &a1 = *p1;
			const CRefItem &a2 = *p2;
			const CUpdateItem &u1 = *a1.UpdateItem;
			const CUpdateItem &u2 = *a2.UpdateItem;
			/*
			   if(u1.IsAltStream != u2.IsAltStream)
			   return u1.IsAltStream ? 1 : -1;
			 */
			// Actually there are no dirs that time. They were stored in other steps
			// So that code is unused?
			if(u1.IsDir != u2.IsDir)
				return u1.IsDir ? 1 : -1;
			if(u1.IsDir) {
				if(u1.IsAnti != u2.IsAnti)
					return (u1.IsAnti ? 1 : -1);
				int n = CompareFileNames(u1.Name, u2.Name);
				return -n;
			}

			// bool sortByType = *(bool *)param;
			const CSortParam * sortParam = (const CSortParam*)param;
			bool sortByType = sortParam->SortByType;
			if(sortByType) {
				RINOZ_COMP(a1.ExtensionIndex, a2.ExtensionIndex);
				RINOZ(CompareFileNames(u1.Name.Ptr(a1.ExtensionPos), u2.Name.Ptr(a2.ExtensionPos)));
				RINOZ(CompareFileNames(u1.Name.Ptr(a1.NamePos), u2.Name.Ptr(a2.NamePos)));
				if(!u1.MTimeDefined && u2.MTimeDefined) return 1;
				if(u1.MTimeDefined && !u2.MTimeDefined) return -1;
				if(u1.MTimeDefined && u2.MTimeDefined) RINOZ_COMP(u1.MTime, u2.MTime);
				RINOZ_COMP(u1.Size, u2.Size);
			}
			/*
			   int par1 = a1.UpdateItem->ParentFolderIndex;
			   int par2 = a2.UpdateItem->ParentFolderIndex;
			   const CTreeFolder &tf1 = (*sortParam->TreeFolders)[par1];
			   const CTreeFolder &tf2 = (*sortParam->TreeFolders)[par2];

			   int b1 = tf1.SortIndex, e1 = tf1.SortIndexEnd;
			   int b2 = tf2.SortIndex, e2 = tf2.SortIndexEnd;
			   if(b1 < b2)
			   {
			   if(e1 <= b2)
				return -1;
			   // p2 in p1
			   int par = par2;
			   for(;;)
			   {
				const CTreeFolder &tf = (*sortParam->TreeFolders)[par];
				par = tf.Parent;
				if(par == par1)
				{
				  RINOZ(CompareFileNames(u1.Name, tf.Name));
				  break;
				}
			   }
			   }
			   else if(b2 < b1)
			   {
			   if(e2 <= b1)
				return 1;
			   // p1 in p2
			   int par = par1;
			   for(;;)
			   {
				const CTreeFolder &tf = (*sortParam->TreeFolders)[par];
				par = tf.Parent;
				if(par == par2)
				{
				  RINOZ(CompareFileNames(tf.Name, u2.Name));
				  break;
				}
			   }
			   }
			 */
			// RINOZ_COMP(a1.UpdateItem->ParentSortIndex, a2.UpdateItem->ParentSortIndex);
			RINOK(CompareFileNames(u1.Name, u2.Name));
			RINOZ_COMP(a1.UpdateItem->IndexInClient, a2.UpdateItem->IndexInClient);
			RINOZ_COMP(a1.UpdateItem->IndexInArchive, a2.UpdateItem->IndexInArchive);
			return 0;
		}

		struct CSolidGroup {
			CRecordVector<uint32> Indices;
			CRecordVector<CFolderRepack> folderRefs;
		};

		static const char * const g_ExeExts[] = { "dll", "exe", "ocx", "sfx", "sys" };

		static bool IsExeExt(const wchar_t * ext)
		{
			for(uint i = 0; i < ARRAY_SIZE(g_ExeExts); i++)
				if(StringsAreEqualNoCase_Ascii(ext, g_ExeExts[i]))
					return true;
			return false;
		}

		struct CAnalysis {
			CMyComPtr<IArchiveUpdateCallbackFile> Callback;
			CByteBuffer Buffer;
			bool    ParseWav;
			bool    ParseExe;
			bool    ParseAll;
			uint8   Reserve; // @alignment
			CAnalysis() : ParseWav(true), ParseExe(false), ParseAll(false)
			{
			}
			HRESULT GetFilterGroup(uint32 index, const CUpdateItem &ui, CFilterMode &filterMode);
		};

		static const size_t kAnalysisBufSize = 1 << 14;

		HRESULT CAnalysis::GetFilterGroup(uint32 index, const CUpdateItem &ui, CFilterMode &filterMode)
		{
			filterMode.Id = 0;
			filterMode.Delta = 0;
			CFilterMode filterModeTemp = filterMode;
			int slashPos = ui.Name.ReverseFind_PathSepar();
			int dotPos = ui.Name.ReverseFind_Dot();
			// if(dotPos > slashPos)
			{
				bool needReadFile = ParseAll;
				bool probablyIsSameIsa = false;
				if(!needReadFile || !Callback) {
					const wchar_t * ext;
					if(dotPos > slashPos)
						ext = ui.Name.Ptr(dotPos + 1);
					else
						ext = ui.Name.RightPtr(0);
					// p7zip uses the trick to store posix attributes in high 16 bits
					if(ui.Attrib & 0x8000) {
						unsigned st_mode = ui.Attrib >> 16;
						// st_mode = 00111;
						if((st_mode & 00111) && (ui.Size >= 2048)) {
			  #ifndef _WIN32
							probablyIsSameIsa = true;
			  #endif
							needReadFile = true;
						}
					}

					if(IsExeExt(ext)) {
						needReadFile = true;
			#ifdef _WIN32
						probablyIsSameIsa = true;
						needReadFile = ParseExe;
			#endif
					}
					else if(StringsAreEqualNoCase_Ascii(ext, "wav")) {
						needReadFile = ParseWav;
					}
					/*
					   else if(!needReadFile && ParseUnixExt)
					   {
					   if(StringsAreEqualNoCase_Ascii(ext, "so")
					 || StringsAreEqualNoCase_Ascii(ext, ""))

						needReadFile = true;
					   }
					 */
				}

				if(needReadFile && Callback) {
					if(Buffer.Size() != kAnalysisBufSize) {
						Buffer.Alloc(kAnalysisBufSize);
					}
					{
						CMyComPtr<ISequentialInStream> stream;
						HRESULT result = Callback->GetStream2(index, &stream, NUpdateNotifyOp::kAnalyze);
						if(result == S_OK && stream) {
							size_t size = kAnalysisBufSize;
							result = ReadStream(stream, Buffer, &size);
							stream.Release();
							// RINOK(Callback->SetOperationResult2(index, NUpdate::NOperationResult::kOK));
							if(result == S_OK) {
								Bool parseRes = ParseFile(Buffer, size, &filterModeTemp);
								if(parseRes && filterModeTemp.Delta == 0) {
									filterModeTemp.SetDelta();
									if(filterModeTemp.Delta != 0 && filterModeTemp.Id != k_Delta) {
										if(ui.Size % filterModeTemp.Delta != 0) {
											parseRes = false;
										}
									}
								}
								if(!parseRes) {
									filterModeTemp.Id = 0;
									filterModeTemp.Delta = 0;
								}
							}
						}
					}
				}
				else if((needReadFile && !Callback) || probablyIsSameIsa) {
			  #ifdef MY_CPU_X86_OR_AMD64
					if(probablyIsSameIsa)
						filterModeTemp.Id = k_X86;
			  #endif
				}
			}
			filterMode = filterModeTemp;
			return S_OK;
		}

		static inline void GetMethodFull(uint64 methodID, uint32 numStreams, CMethodFull &m)
		{
			m.Id = methodID;
			m.NumStreams = numStreams;
		}

		static HRESULT AddBondForFilter(CCompressionMethodMode &mode)
		{
			for(uint c = 1; c < mode.Methods.Size(); c++) {
				if(!mode.IsThereBond_to_Coder(c)) {
					CBond2 bond;
					bond.OutCoder = 0;
					bond.OutStream = 0;
					bond.InCoder = c;
					mode.Bonds.Add(bond);
					return S_OK;
				}
			}
			return E_INVALIDARG;
		}

		static HRESULT AddFilterBond(CCompressionMethodMode &mode)
		{
			return (!mode.Bonds.IsEmpty()) ? AddBondForFilter(mode) : S_OK;
		}

		static HRESULT AddBcj2Methods(CCompressionMethodMode &mode)
		{
			// mode.Methods[0] must be k_BCJ2 method !

			CMethodFull m;
			GetMethodFull(k_LZMA, 1, m);

			m.AddProp32(NCoderPropID::kDictionarySize, 1 << 20);
			m.AddProp32(NCoderPropID::kNumFastBytes, 128);
			m.AddProp32(NCoderPropID::kNumThreads, 1);
			m.AddProp32(NCoderPropID::kLitPosBits, 2);
			m.AddProp32(NCoderPropID::kLitContextBits, 0);
			// m.AddProp_Ascii(NCoderPropID::kMatchFinder, "BT2");

			unsigned methodIndex = mode.Methods.Size();

			if(mode.Bonds.IsEmpty()) {
				for(uint i = 1; i + 1 < mode.Methods.Size(); i++) {
					CBond2 bond;
					bond.OutCoder = i;
					bond.OutStream = 0;
					bond.InCoder = i + 1;
					mode.Bonds.Add(bond);
				}
			}

			mode.Methods.Add(m);
			mode.Methods.Add(m);

			RINOK(AddBondForFilter(mode));
			CBond2 bond;
			bond.OutCoder = 0;
			bond.InCoder = methodIndex;      bond.OutStream = 1;  mode.Bonds.Add(bond);
			bond.InCoder = methodIndex + 1;  bond.OutStream = 2;  mode.Bonds.Add(bond);
			return S_OK;
		}

		static HRESULT MakeExeMethod(CCompressionMethodMode &mode, const CFilterMode &filterMode, /* bool addFilter, */ bool bcj2Filter)
		{
			if(mode.Filter_was_Inserted) {
				const CMethodFull &m = mode.Methods[0];
				CMethodId id = m.Id;
				if(id == k_BCJ2)
					return AddBcj2Methods(mode);
				if(!m.IsSimpleCoder())
					return E_NOTIMPL;
				// if(Bonds.IsEmpty()) we can create bonds later
				return AddFilterBond(mode);
			}
			if(filterMode.Id == 0)
				return S_OK;
			CMethodFull &m = mode.Methods.InsertNew(0);
			{
				FOR_VECTOR(k, mode.Bonds) {
					CBond2 &bond = mode.Bonds[k];
					bond.InCoder++;
					bond.OutCoder++;
				}
			}
			HRESULT res;
			if(bcj2Filter && Is86Filter(filterMode.Id)) {
				GetMethodFull(k_BCJ2, 4, m);
				res = AddBcj2Methods(mode);
			}
			else {
				GetMethodFull(filterMode.Id, 1, m);
				if(filterMode.Id == k_Delta)
					m.AddProp32(NCoderPropID::kDefaultProp, filterMode.Delta);
				res = AddFilterBond(mode);
				int alignBits = -1;
				if(filterMode.Id == k_Delta || filterMode.Delta != 0) {
					if(filterMode.Delta == 1) alignBits = 0;
					else if(filterMode.Delta == 2) alignBits = 1;
					else if(filterMode.Delta == 4) alignBits = 2;
					else if(filterMode.Delta == 8) alignBits = 3;
					else if(filterMode.Delta == 16) alignBits = 4;
				}
				else {
					// alignBits = GetAlignForFilterMethod(filterMode.Id);
				}

				if(res == S_OK && alignBits >= 0) {
					unsigned nextCoder = 1;
					if(!mode.Bonds.IsEmpty()) {
						nextCoder = mode.Bonds.Back().InCoder;
					}
					if(nextCoder < mode.Methods.Size()) {
						CMethodFull &nextMethod = mode.Methods[nextCoder];
						if(nextMethod.Id == k_LZMA || nextMethod.Id == k_LZMA2) {
							if(!nextMethod.Are_Lzma_Model_Props_Defined()) {
								if(alignBits != 0) {
									if(alignBits > 2 || filterMode.Id == k_Delta)
										nextMethod.AddProp32(NCoderPropID::kPosStateBits, alignBits);
									unsigned lc = 0;
									if(alignBits < 3)
										lc = 3 - alignBits;
									nextMethod.AddProp32(NCoderPropID::kLitContextBits, lc);
									nextMethod.AddProp32(NCoderPropID::kLitPosBits, alignBits);
								}
							}
						}
					}
				}
			}
			return res;
		}

		static void UpdateItem_To_FileItem2(const CUpdateItem &ui, CFileItem2 &file2)
		{
			file2.Attrib = ui.Attrib;  file2.AttribDefined = ui.AttribDefined;
			file2.CTime = ui.CTime;  file2.CTimeDefined = ui.CTimeDefined;
			file2.ATime = ui.ATime;  file2.ATimeDefined = ui.ATimeDefined;
			file2.MTime = ui.MTime;  file2.MTimeDefined = ui.MTimeDefined;
			file2.IsAnti = ui.IsAnti;
			// file2.IsAux = false;
			file2.StartPosDefined = false;
			// file2.StartPos = 0;
		}

		static void UpdateItem_To_FileItem(const CUpdateItem &ui,
					CFileItem &file, CFileItem2 &file2)
		{
			UpdateItem_To_FileItem2(ui, file2);

			file.Size = ui.Size;
			file.IsDir = ui.IsDir;
			file.HasStream = ui.HasStream();
			// file.IsAltStream = ui.IsAltStream;
		}

		class CRepackInStreamWithSizes : public ISequentialInStream, public ICompressGetSubStreamSize, public CMyUnknownImp {
			CMyComPtr<ISequentialInStream> _stream;
			// uint64 _size;
			const CBoolVector * _extractStatuses;
			uint32 _startIndex;
		public:
			const CDbEx * _db;
			void Init(ISequentialInStream * stream, uint32 startIndex, const CBoolVector * extractStatuses)
			{
				_startIndex = startIndex;
				_extractStatuses = extractStatuses;
				// _size = 0;
				_stream = stream;
			}
			// uint64 GetSize() const { return _size; }
			MY_UNKNOWN_IMP2(ISequentialInStream, ICompressGetSubStreamSize)
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			STDMETHOD(GetSubStreamSize) (uint64 subStream, uint64 *value);
		};

		STDMETHODIMP CRepackInStreamWithSizes::Read(void * data, uint32 size, uint32 * processedSize)
		{
			return _stream->Read(data, size, processedSize);
			/*
			   uint32 realProcessedSize;
			   HRESULT result = _stream->Read(data, size, &realProcessedSize);
			   _size += realProcessedSize;
			   if(processedSize)
			   *processedSize = realProcessedSize;
			   return result;
			 */
		}

		STDMETHODIMP CRepackInStreamWithSizes::GetSubStreamSize(uint64 subStream, uint64 * value)
		{
			*value = 0;
			if(subStream >= _extractStatuses->Size())
				return S_FALSE;  // E_FAIL;
			unsigned index = (uint)subStream;
			if((*_extractStatuses)[index]) {
				const CFileItem &fi = _db->Files[_startIndex + index];
				if(fi.HasStream)
					*value = fi.Size;
			}
			return S_OK;
		}

		class CRepackStreamBase {
		protected:
			bool _needWrite;
			bool _fileIsOpen;
			bool _calcCrc;
			uint32 _crc;
			uint64 _rem;
			const CBoolVector * _extractStatuses;
			uint32 _startIndex;
			unsigned _currentIndex;
			HRESULT OpenFile();
			HRESULT CloseFile();
			HRESULT ProcessEmptyFiles();
		public:
			const CDbEx * _db;
			CMyComPtr<IArchiveUpdateCallbackFile> _opCallback;
			CMyComPtr<IArchiveExtractCallbackMessage> _extractCallback;
			HRESULT Init(uint32 startIndex, const CBoolVector * extractStatuses);
			HRESULT CheckFinishedState() const { return (_currentIndex == _extractStatuses->Size()) ? S_OK : E_FAIL; }
		};

		HRESULT CRepackStreamBase::Init(uint32 startIndex, const CBoolVector * extractStatuses)
		{
			_startIndex = startIndex;
			_extractStatuses = extractStatuses;
			_currentIndex = 0;
			_fileIsOpen = false;
			return ProcessEmptyFiles();
		}

		HRESULT CRepackStreamBase::OpenFile()
		{
			uint32 arcIndex = _startIndex + _currentIndex;
			const CFileItem &fi = _db->Files[arcIndex];
			_needWrite = (*_extractStatuses)[_currentIndex];
			if(_opCallback) {
				RINOK(_opCallback->ReportOperation(NEventIndexType::kInArcIndex, arcIndex, _needWrite ? NUpdateNotifyOp::kRepack : NUpdateNotifyOp::kSkip));
			}
			_crc = CRC_INIT_VAL;
			_calcCrc = (fi.CrcDefined && !fi.IsDir);
			_fileIsOpen = true;
			_rem = fi.Size;
			return S_OK;
		}

		const HRESULT k_My_HRESULT_CRC_ERROR = 0x20000002;

		HRESULT CRepackStreamBase::CloseFile()
		{
			uint32 arcIndex = _startIndex + _currentIndex;
			const CFileItem &fi = _db->Files[arcIndex];
			_fileIsOpen = false;
			_currentIndex++;
			if(!_calcCrc || fi.Crc == CRC_GET_DIGEST(_crc))
				return S_OK;
			if(_extractCallback) {
				RINOK(_extractCallback->ReportExtractResult(NEventIndexType::kInArcIndex, arcIndex, NExtractArc::NOperationResult::kCRCError));
			}
			// return S_FALSE;
			return k_My_HRESULT_CRC_ERROR;
		}

		HRESULT CRepackStreamBase::ProcessEmptyFiles()
		{
			while(_currentIndex < _extractStatuses->Size() && _db->Files[_startIndex + _currentIndex].Size == 0) {
				RINOK(OpenFile());
				RINOK(CloseFile());
			}
			return S_OK;
		}

		#ifndef _7ZIP_ST

		class CFolderOutStream2 : public CRepackStreamBase, public ISequentialOutStream, public CMyUnknownImp {
		public:
			CMyComPtr<ISequentialOutStream> _stream;
			MY_UNKNOWN_IMP
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CFolderOutStream2::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(_fileIsOpen) {
					uint32 cur = (size < _rem ? size : (uint32)_rem);
					HRESULT result = S_OK;
					if(_needWrite)
						result = _stream->Write(data, cur, &cur);
					if(_calcCrc)
						_crc = CrcUpdate(_crc, data, cur);
					if(processedSize)
						*processedSize += cur;
					data = (const Byte*)data + cur;
					size -= cur;
					_rem -= cur;
					if(_rem == 0) {
						RINOK(CloseFile());
						RINOK(ProcessEmptyFiles());
					}
					RINOK(result);
					if(cur == 0)
						break;
					continue;
				}
				RINOK(ProcessEmptyFiles());
				if(_currentIndex == _extractStatuses->Size()) {
					// we don't support write cut here
					return E_FAIL;
				}
				RINOK(OpenFile());
			}
			return S_OK;
		}

		#endif

		static const uint32 kTempBufSize = 1 << 16;

		class CFolderInStream2 : public CRepackStreamBase, public ISequentialInStream, public CMyUnknownImp {
			Byte * _buf;
		public:
			CMyComPtr<ISequentialInStream> _inStream;
			HRESULT Result;

			MY_UNKNOWN_IMP CFolderInStream2() :
				Result(S_OK)
			{
				_buf = new Byte[kTempBufSize];
			}

			~CFolderInStream2()
			{
				delete []_buf;
			}

			void Init() {
				Result = S_OK;
			}

			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CFolderInStream2::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(_fileIsOpen) {
					uint32 cur = (size < _rem ? size : (uint32)_rem);
					void * buf;
					if(_needWrite)
						buf = data;
					else {
						buf = _buf;
						if(cur > kTempBufSize)
							cur = kTempBufSize;
					}
					HRESULT result = _inStream->Read(buf, cur, &cur);
					_crc = CrcUpdate(_crc, buf, cur);
					_rem -= cur;
					if(_needWrite) {
						data = (Byte*)data + cur;
						size -= cur;
						if(processedSize)
							*processedSize += cur;
					}
					if(result != S_OK)
						Result = result;
					if(_rem == 0) {
						RINOK(CloseFile());
						RINOK(ProcessEmptyFiles());
					}
					RINOK(result);
					if(cur == 0)
						return E_FAIL;
					continue;
				}
				RINOK(ProcessEmptyFiles());
				if(_currentIndex == _extractStatuses->Size()) {
					return S_OK;
				}
				RINOK(OpenFile());
			}
			return S_OK;
		}

		class CThreadDecoder
		  #ifndef _7ZIP_ST
			: public CVirtThread
		  #endif
		{
		public:
			CDecoder Decoder;
			CThreadDecoder(bool multiThreadMixer) : Decoder(multiThreadMixer)
			{
			#ifndef _7ZIP_ST
				if(multiThreadMixer) {
					MtMode = false;
					NumThreads = 1;
					FosSpec = new CFolderOutStream2;
					Fos = FosSpec;
					Result = E_FAIL;
				}
			#endif
				// UnpackSize = 0;
				// send_UnpackSize = false;
			}

		  #ifndef _7ZIP_ST
			bool dataAfterEnd_Error;
			HRESULT Result;
			CMyComPtr<IInStream> InStream;
			CFolderOutStream2 * FosSpec;
			CMyComPtr<ISequentialOutStream> Fos;
			uint64 StartPos;
			const CFolders * Folders;
			int FolderIndex;
			// bool send_UnpackSize;
			// uint64 UnpackSize;
		  #ifndef _NO_CRYPTO
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
		  #endif
			DECL_EXTERNAL_CODECS_LOC_VARS2;
		  #ifndef _7ZIP_ST
			bool MtMode;
			uint32 NumThreads;
		  #endif
			~CThreadDecoder() 
			{
				CVirtThread::WaitThreadFinish();
			}
			virtual void Execute();
		  #endif
		};

		#ifndef _7ZIP_ST

		void CThreadDecoder::Execute()
		{
			try {
			#ifndef _NO_CRYPTO
				bool isEncrypted = false;
				bool passwordIsDefined = false;
				UString password;
			#endif
				dataAfterEnd_Error = false;
				Result = Decoder.Decode(EXTERNAL_CODECS_LOC_VARS InStream, StartPos, *Folders, FolderIndex,
									// send_UnpackSize ? &UnpackSize : NULL,
							NULL, // unpackSize : FULL unpack
							Fos,
							NULL, // compressProgress
							NULL // *inStreamMainRes
							, dataAfterEnd_Error
							_7Z_DECODER_CRYPRO_VARS
			  #ifndef _7ZIP_ST
							, MtMode, NumThreads
			  #endif

							);
			}
			catch(...)
			{
				Result = E_FAIL;
			}

			/*
			   if(Result == S_OK)
			   Result = FosSpec->CheckFinishedState();
			 */
			FosSpec->_stream.Release();
		}

		#endif

		#ifndef _NO_CRYPTO

		class CCryptoGetTextPassword : public ICryptoGetTextPassword, public CMyUnknownImp {
		public:
			UString Password;
			MY_UNKNOWN_IMP
			STDMETHOD(CryptoGetTextPassword) (BSTR *password);
		};

		STDMETHODIMP CCryptoGetTextPassword::CryptoGetTextPassword(BSTR * password)
		{
			return StringToBstr(Password, password);
		}

		#endif

		static void GetFile(const CDatabase &inDb, unsigned index, CFileItem &file, CFileItem2 &file2)
		{
			file = inDb.Files[index];
			file2.CTimeDefined = inDb.CTime.GetItem(index, file2.CTime);
			file2.ATimeDefined = inDb.ATime.GetItem(index, file2.ATime);
			file2.MTimeDefined = inDb.MTime.GetItem(index, file2.MTime);
			file2.StartPosDefined = inDb.StartPos.GetItem(index, file2.StartPos);
			file2.AttribDefined = inDb.Attrib.GetItem(index, file2.Attrib);
			file2.IsAnti = inDb.IsItemAnti(index);
			// file2.IsAux = inDb.IsItemAux(index);
		}

		HRESULT Update(DECL_EXTERNAL_CODECS_LOC_VARS
					IInStream * inStream,
					const CDbEx * db,
					const CObjectVector<CUpdateItem> &updateItems,
							// const CObjectVector<CTreeFolder> &treeFolders,
							// const CUniqBlocks &secureBlocks,
					COutArchive &archive,
					CArchiveDatabaseOut &newDatabase,
					ISequentialOutStream * seqOutStream,
					IArchiveUpdateCallback * updateCallback,
					const CUpdateOptions &options
			#ifndef _NO_CRYPTO
					, ICryptoGetTextPassword * getDecoderPassword
			#endif
					)
		{
			uint64 numSolidFiles = options.NumSolidFiles;
			if(numSolidFiles == 0)
				numSolidFiles = 1;

			CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
			updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void**)&opCallback);

			CMyComPtr<IArchiveExtractCallbackMessage> extractCallback;
			updateCallback->QueryInterface(IID_IArchiveExtractCallbackMessage, (void**)&extractCallback);

			// size_t totalSecureDataSize = (size_t)secureBlocks.GetTotalSizeInBytes();

			/*
			   CMyComPtr<IOutStream> outStream;
			   RINOK(seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStream));
			   if(!outStream)
			   return E_NOTIMPL;
			 */
			uint64 startBlockSize = db ? db->ArcInfo.StartPosition : 0;
			if(startBlockSize > 0 && !options.RemoveSfxBlock) {
				RINOK(WriteRange(inStream, seqOutStream, 0, startBlockSize, NULL));
			}
			CIntArr fileIndexToUpdateIndexMap;
			uint64 complexity = 0;
			uint64 inSizeForReduce2 = 0;
			bool needEncryptedRepack = false;
			CRecordVector<CFilterMode2> filters;
			CObjectVector<CSolidGroup> groups;
			bool thereAreRepacks = false;
			bool useFilters = options.UseFilters;
			if(useFilters) {
				const CCompressionMethodMode &method = *options.Method;
				FOR_VECTOR(i, method.Methods)
				if(IsFilterMethod(method.Methods[i].Id)) {
					useFilters = false;
					break;
				}
			}
			if(db) {
				fileIndexToUpdateIndexMap.Alloc(db->Files.Size());
				uint   i;
				for(i = 0; i < db->Files.Size(); i++)
					fileIndexToUpdateIndexMap[i] = -1;
				for(i = 0; i < updateItems.Size(); i++) {
					int index = updateItems[i].IndexInArchive;
					if(index != -1)
						fileIndexToUpdateIndexMap[(uint)index] = i;
				}
				for(i = 0; i < db->NumFolders; i++) {
					CNum indexInFolder = 0;
					CNum numCopyItems = 0;
					CNum numUnpackStreams = db->NumUnpackStreamsVector[i];
					uint64 repackSize = 0;
					for(CNum fi = db->FolderStartFileIndex[i]; indexInFolder < numUnpackStreams; fi++) {
						const CFileItem &file = db->Files[fi];
						if(file.HasStream) {
							indexInFolder++;
							int updateIndex = fileIndexToUpdateIndexMap[fi];
							if(updateIndex >= 0 && !updateItems[updateIndex].NewData) {
								numCopyItems++;
								repackSize += file.Size;
							}
						}
					}
					if(numCopyItems == 0)
						continue;
					CFolderRepack rep;
					rep.FolderIndex = i;
					rep.NumCopyFiles = numCopyItems;
					CFolderEx f;
					db->ParseFolderEx(i, f);
					const bool isEncrypted = f.IsEncrypted();
					const bool needCopy = (numCopyItems == numUnpackStreams);
					const bool extractFilter = (useFilters || needCopy);
					uint   groupIndex = Get_FilterGroup_for_Folder(filters, f, extractFilter);
					while(groupIndex >= groups.Size())
						groups.AddNew();
					groups[groupIndex].folderRefs.Add(rep);
					if(needCopy)
						complexity += db->GetFolderFullPackSize(i);
					else {
						thereAreRepacks = true;
						complexity += repackSize;
						if(inSizeForReduce2 < repackSize)
							inSizeForReduce2 = repackSize;
						if(isEncrypted)
							needEncryptedRepack = true;
					}
				}
			}
			uint64 inSizeForReduce = 0;
			{
				FOR_VECTOR(i, updateItems) {
					const CUpdateItem & ui = updateItems[i];
					if(ui.NewData) {
						complexity += ui.Size;
						if(numSolidFiles != 1)
							inSizeForReduce += ui.Size;
						else if(inSizeForReduce < ui.Size)
							inSizeForReduce = ui.Size;
					}
				}
			}
			if(inSizeForReduce < inSizeForReduce2)
				inSizeForReduce = inSizeForReduce2;
			RINOK(updateCallback->SetTotal(complexity));
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(updateCallback, true);
		  #ifndef _7ZIP_ST
			CStreamBinder sb;
			if(options.MultiThreadMixer) {
				RINOK(sb.CreateEvents());
			}
		  #endif
			CThreadDecoder threadDecoder(options.MultiThreadMixer);
		  #ifndef _7ZIP_ST
			if(options.MultiThreadMixer && thereAreRepacks) {
			#ifdef EXTERNAL_CODECS
				threadDecoder.__externalCodecs = __externalCodecs;
			#endif
				RINOK(threadDecoder.Create());
			}
		  #endif
			{
				CAnalysis analysis;
				if(options.AnalysisLevel == 0) {
					analysis.ParseWav = false;
					analysis.ParseExe = false;
					analysis.ParseAll = false;
				}
				else {
					analysis.Callback = opCallback;
					if(options.AnalysisLevel > 0) {
						analysis.ParseWav = true;
						if(options.AnalysisLevel >= 7) {
							analysis.ParseExe = true;
							if(options.AnalysisLevel >= 9)
								analysis.ParseAll = true;
						}
					}
				}
				// ---------- Split files to groups ----------
				const CCompressionMethodMode &method = *options.Method;
				FOR_VECTOR(i, updateItems) {
					const CUpdateItem &ui = updateItems[i];
					if(!ui.NewData || !ui.HasStream())
						continue;
					CFilterMode2 fm;
					if(useFilters) {
						RINOK(analysis.GetFilterGroup(i, ui, fm));
					}
					fm.Encrypted = method.PasswordIsDefined;
					unsigned groupIndex = GetGroup(filters, fm);
					while(groupIndex >= groups.Size())
						groups.AddNew();
					groups[groupIndex].Indices.Add(i);
				}
			}

		  #ifndef _NO_CRYPTO

			CCryptoGetTextPassword * getPasswordSpec = NULL;
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			if(needEncryptedRepack) {
				getPasswordSpec = new CCryptoGetTextPassword;
				getTextPassword = getPasswordSpec;

			#ifndef _7ZIP_ST
				threadDecoder.getTextPassword = getPasswordSpec;
			#endif

				if(options.Method->PasswordIsDefined)
					getPasswordSpec->Password = options.Method->Password;
				else {
					if(!getDecoderPassword)
						return E_NOTIMPL;
					CMyComBSTR password;
					RINOK(getDecoderPassword->CryptoGetTextPassword(&password));
					if(password)
						getPasswordSpec->Password = password;
				}
			}

		  #endif

			// ---------- Compress ----------

			RINOK(archive.Create(seqOutStream, false));
			RINOK(archive.SkipPrefixArchiveHeader());

			/*
			   CIntVector treeFolderToArcIndex;
			   treeFolderToArcIndex.Reserve(treeFolders.Size());
			   for(i = 0; i < treeFolders.Size(); i++)
			   treeFolderToArcIndex.Add(-1);
			   // ---------- Write Tree (only AUX dirs) ----------
			   for(i = 1; i < treeFolders.Size(); i++)
			   {
			   const CTreeFolder &treeFolder = treeFolders[i];
			   CFileItem file;
			   CFileItem2 file2;
			   file2.Init();
			   int secureID = 0;
			   if(treeFolder.UpdateItemIndex < 0)
			   {
				// we can store virtual dir item wuthout attrib, but we want all items have attrib.
				file.SetAttrib(FILE_ATTRIBUTE_DIRECTORY);
				file2.IsAux = true;
			   }
			   else
			   {
				const CUpdateItem &ui = updateItems[treeFolder.UpdateItemIndex];
				// if item is not dir, then it's parent for alt streams.
				// we will write such items later
				if(!ui.IsDir)
				  continue;
				secureID = ui.SecureIndex;
				if(ui.NewProps)
				  UpdateItem_To_FileItem(ui, file, file2);
				else
				  GetFile(*db, ui.IndexInArchive, file, file2);
			   }
			   file.Size = 0;
			   file.HasStream = false;
			   file.IsDir = true;
			   file.Parent = treeFolder.Parent;

			   treeFolderToArcIndex[i] = newDatabase.Files.Size();
			   newDatabase.AddFile(file, file2, treeFolder.Name);

			   if(totalSecureDataSize != 0)
				newDatabase.SecureIDs.Add(secureID);
			   }
			 */
			{
				/* ---------- Write non-AUX dirs and Empty files ---------- */
				CUIntVector emptyRefs;
				uint i;
				for(i = 0; i < updateItems.Size(); i++) {
					const CUpdateItem &ui = updateItems[i];
					if(ui.NewData) {
						if(ui.HasStream())
							continue;
					}
					else if(ui.IndexInArchive != -1 && db->Files[ui.IndexInArchive].HasStream)
						continue;
					/*
					   if(ui.TreeFolderIndex >= 0)
					   continue;
					 */
					emptyRefs.Add(i);
				}
				emptyRefs.Sort(CompareEmptyItems, (void*)&updateItems);
				for(i = 0; i < emptyRefs.Size(); i++) {
					const CUpdateItem &ui = updateItems[emptyRefs[i]];
					CFileItem file;
					CFileItem2 file2;
					UString name;
					if(ui.NewProps) {
						UpdateItem_To_FileItem(ui, file, file2);
						file.CrcDefined = false;
						name = ui.Name;
					}
					else {
						GetFile(*db, ui.IndexInArchive, file, file2);
						db->GetPath(ui.IndexInArchive, name);
					}

					/*
					   if(totalSecureDataSize != 0)
					   newDatabase.SecureIDs.Add(ui.SecureIndex);
					   file.Parent = ui.ParentFolderIndex;
					 */
					newDatabase.AddFile(file, file2, name);
				}
			}
			lps->ProgressOffset = 0;
			{
				// ---------- Sort Filters ----------
				FOR_VECTOR(i, filters) {
					filters[i].GroupIndex = i;
				}
				filters.Sort2();
			}
			for(uint groupIndex = 0; groupIndex < filters.Size(); groupIndex++) {
				const CFilterMode2 &filterMode = filters[groupIndex];
				CCompressionMethodMode method = *options.Method;
				{
					HRESULT res = MakeExeMethod(method, filterMode,
			#ifdef _7ZIP_ST
								false
			#else
								options.MaxFilter && options.MultiThreadMixer
			#endif
								);
					RINOK(res);
				}
				if(filterMode.Encrypted) {
					if(!method.PasswordIsDefined) {
			#ifndef _NO_CRYPTO
						if(getPasswordSpec)
							method.Password = getPasswordSpec->Password;
			#endif
						method.PasswordIsDefined = true;
					}
				}
				else {
					method.PasswordIsDefined = false;
					method.Password.Empty();
				}
				CEncoder encoder(method);
				// ---------- Repack and copy old solid blocks ----------
				const CSolidGroup &group = groups[filterMode.GroupIndex];
				FOR_VECTOR(folderRefIndex, group.folderRefs) {
					const CFolderRepack &rep = group.folderRefs[folderRefIndex];
					unsigned folderIndex = rep.FolderIndex;
					CNum numUnpackStreams = db->NumUnpackStreamsVector[folderIndex];
					if(rep.NumCopyFiles == numUnpackStreams) {
						if(opCallback) {
							RINOK(opCallback->ReportOperation(NEventIndexType::kBlockIndex, (uint32)folderIndex, NUpdateNotifyOp::kReplicate));
							// ---------- Copy old solid block ----------
							{
								CNum indexInFolder = 0;
								for(CNum fi = db->FolderStartFileIndex[folderIndex]; indexInFolder < numUnpackStreams; fi++) {
									if(db->Files[fi].HasStream) {
										indexInFolder++;
										RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, (uint32)fi, NUpdateNotifyOp::kReplicate));
									}
								}
							}
						}
						uint64 packSize = db->GetFolderFullPackSize(folderIndex);
						RINOK(WriteRange(inStream, archive.SeqStream, db->GetFolderStreamPos(folderIndex, 0), packSize, progress));
						lps->ProgressOffset += packSize;
						CFolder &folder = newDatabase.Folders.AddNew();
						db->ParseFolderInfo(folderIndex, folder);
						CNum startIndex = db->FoStartPackStreamIndex[folderIndex];
						FOR_VECTOR(j, folder.PackStreams) {
							newDatabase.PackSizes.Add(db->GetStreamPackSize(startIndex + j));
							// newDatabase.PackCRCsDefined.Add(db.PackCRCsDefined[startIndex + j]);
							// newDatabase.PackCRCs.Add(db.PackCRCs[startIndex + j]);
						}
						size_t indexStart = db->FoToCoderUnpackSizes[folderIndex];
						size_t indexEnd = db->FoToCoderUnpackSizes[folderIndex + 1];
						for(; indexStart < indexEnd; indexStart++)
							newDatabase.CoderUnpackSizes.Add(db->CoderUnpackSizes[indexStart]);
					}
					else {
						// ---------- Repack old solid block ----------
						CBoolVector extractStatuses;
						CNum indexInFolder = 0;
						if(opCallback) {
							RINOK(opCallback->ReportOperation(NEventIndexType::kBlockIndex, (uint32)folderIndex, NUpdateNotifyOp::kRepack))
						}
						/* We could reduce data size of decoded folder, if we don't need to repack
						   last files in folder. But the gain in speed is small in most cases.
						   So we unpack full folder. */

						uint64 sizeToEncode = 0;
						/*
						   uint64 importantUnpackSize = 0;
						   unsigned numImportantFiles = 0;
						   uint64 decodeSize = 0;
						 */
						for(CNum fi = db->FolderStartFileIndex[folderIndex]; indexInFolder < numUnpackStreams; fi++) {
							bool needExtract = false;
							const CFileItem &file = db->Files[fi];
							if(file.HasStream) {
								indexInFolder++;
								int updateIndex = fileIndexToUpdateIndexMap[fi];
								if(updateIndex >= 0 && !updateItems[updateIndex].NewData)
									needExtract = true;
								// decodeSize += file.Size;
							}
							extractStatuses.Add(needExtract);
							if(needExtract) {
								sizeToEncode += file.Size;
								/*
								   numImportantFiles = extractStatuses.Size();
								   importantUnpackSize = decodeSize;
								 */
							}
						}
						// extractStatuses.DeleteFrom(numImportantFiles);
						unsigned startPackIndex = newDatabase.PackSizes.Size();
						uint64 curUnpackSize;
						{
							CMyComPtr<ISequentialInStream> sbInStream;
							CRepackStreamBase * repackBase;
							CFolderInStream2 * FosSpec2 = NULL;
							CRepackInStreamWithSizes * inStreamSizeCountSpec = new CRepackInStreamWithSizes;
							CMyComPtr<ISequentialInStream> inStreamSizeCount = inStreamSizeCountSpec;
							{
				#ifndef _7ZIP_ST
								if(options.MultiThreadMixer) {
									repackBase = threadDecoder.FosSpec;
									CMyComPtr<ISequentialOutStream> sbOutStream;
									sb.CreateStreams(&sbInStream, &sbOutStream);
									sb.ReInit();
									threadDecoder.FosSpec->_stream = sbOutStream;
									threadDecoder.InStream = inStream;
									threadDecoder.StartPos = db->ArcInfo.DataStartPosition; // db->GetFolderStreamPos(folderIndex, 0);
									threadDecoder.Folders = (const CFolders*)db;
									threadDecoder.FolderIndex = folderIndex;
									// threadDecoder.UnpackSize = importantUnpackSize;
									// threadDecoder.send_UnpackSize = true;
								}
								else
				#endif
								{
									FosSpec2 = new CFolderInStream2;
									FosSpec2->Init();
									sbInStream = FosSpec2;
									repackBase = FosSpec2;
				  #ifndef _NO_CRYPTO
									bool isEncrypted = false;
									bool passwordIsDefined = false;
									UString password;
				  #endif
									CMyComPtr<ISequentialInStream> decodedStream;
									bool dataAfterEnd_Error = false;
									HRESULT res = threadDecoder.Decoder.Decode(EXTERNAL_CODECS_LOC_VARS inStream,
												db->ArcInfo.DataStartPosition, // db->GetFolderStreamPos(folderIndex, 0);,
												*db, folderIndex,
														// &importantUnpackSize, // *unpackSize
												NULL, // *unpackSize : FULL unpack
												NULL, // *outStream
												NULL, // *compressProgress
												&decodedStream, dataAfterEnd_Error
												_7Z_DECODER_CRYPRO_VARS
				  #ifndef _7ZIP_ST
												, false // mtMode
												, 1 // numThreads
				  #endif
												);
									RINOK(res);
									if(!decodedStream)
										return E_FAIL;
									FosSpec2->_inStream = decodedStream;
								}
								repackBase->_db = db;
								repackBase->_opCallback = opCallback;
								repackBase->_extractCallback = extractCallback;

								uint32 startIndex = db->FolderStartFileIndex[folderIndex];
								RINOK(repackBase->Init(startIndex, &extractStatuses));

								inStreamSizeCountSpec->_db = db;
								inStreamSizeCountSpec->Init(sbInStream, startIndex, &extractStatuses);

				#ifndef _7ZIP_ST
								if(options.MultiThreadMixer) {
									threadDecoder.Start();
								}
				#endif
							}
							HRESULT encodeRes = encoder.Encode(EXTERNAL_CODECS_LOC_VARS inStreamSizeCount, /*NULL,*/ &inSizeForReduce,
								newDatabase.Folders.AddNew(), newDatabase.CoderUnpackSizes, curUnpackSize, archive.SeqStream, newDatabase.PackSizes, progress);
							if(encodeRes == k_My_HRESULT_CRC_ERROR)
								return E_FAIL;
			  #ifndef _7ZIP_ST
							if(options.MultiThreadMixer) {
								// 16.00: hang was fixed : for case if decoding was not finished.
								// We close CBinderInStream and it calls CStreamBinder::CloseRead()
								inStreamSizeCount.Release();
								sbInStream.Release();
								threadDecoder.WaitExecuteFinish();
								HRESULT decodeRes = threadDecoder.Result;
								// if(res == k_My_HRESULT_CRC_ERROR)
								if(decodeRes == S_FALSE || threadDecoder.dataAfterEnd_Error) {
									if(extractCallback) {
										RINOK(extractCallback->ReportExtractResult(NEventIndexType::kInArcIndex, db->FolderStartFileIndex[folderIndex],
																	//
																	// NEventIndexType::kBlockIndex,
																	// (uint32)folderIndex,
														(decodeRes != S_OK ? NExtractArc::NOperationResult::kDataError : NExtractArc::NOperationResult::kDataAfterEnd)));
									}
									if(decodeRes != S_OK)
										return E_FAIL;
								}
								RINOK(decodeRes);
								if(encodeRes == S_OK)
									if(sb.ProcessedSize != sizeToEncode)
										encodeRes = E_FAIL;
							}
							else
			  #endif
							{
								if(FosSpec2->Result == S_FALSE) {
									if(extractCallback) {
										RINOK(extractCallback->ReportExtractResult(NEventIndexType::kBlockIndex, (uint32)folderIndex, NExtractArc::NOperationResult::kDataError));
									}
									return E_FAIL;
								}
								RINOK(FosSpec2->Result);
							}
							RINOK(encodeRes);
							RINOK(repackBase->CheckFinishedState());
							if(curUnpackSize != sizeToEncode)
								return E_FAIL;
						}
						for(; startPackIndex < newDatabase.PackSizes.Size(); startPackIndex++)
							lps->OutSize += newDatabase.PackSizes[startPackIndex];
						lps->InSize += curUnpackSize;
					}

					newDatabase.NumUnpackStreamsVector.Add(rep.NumCopyFiles);

					CNum indexInFolder = 0;
					for(CNum fi = db->FolderStartFileIndex[folderIndex]; indexInFolder < numUnpackStreams; fi++) {
						if(db->Files[fi].HasStream) {
							indexInFolder++;
							int updateIndex = fileIndexToUpdateIndexMap[fi];
							if(updateIndex >= 0) {
								const CUpdateItem &ui = updateItems[updateIndex];
								if(ui.NewData)
									continue;

								UString name;
								CFileItem file;
								CFileItem2 file2;
								GetFile(*db, fi, file, file2);

								if(ui.NewProps) {
									UpdateItem_To_FileItem2(ui, file2);
									file.IsDir = ui.IsDir;
									name = ui.Name;
								}
								else
									db->GetPath(fi, name);

								/*
								   file.Parent = ui.ParentFolderIndex;
								   if(ui.TreeFolderIndex >= 0)
								   treeFolderToArcIndex[ui.TreeFolderIndex] = newDatabase.Files.Size();
								   if(totalSecureDataSize != 0)
								   newDatabase.SecureIDs.Add(ui.SecureIndex);
								 */
								newDatabase.AddFile(file, file2, name);
							}
						}
					}
				}

				// ---------- Compress files to new solid blocks ----------

				unsigned numFiles = group.Indices.Size();
				if(numFiles == 0)
					continue;
				CRecordVector<CRefItem> refItems;
				refItems.ClearAndSetSize(numFiles);
				bool sortByType = (options.UseTypeSorting && numSolidFiles > 1);

				uint i;

				for(i = 0; i < numFiles; i++)
					refItems[i] = CRefItem(group.Indices[i], updateItems[group.Indices[i]], sortByType);

				CSortParam sortParam;
				// sortParam.TreeFolders = &treeFolders;
				sortParam.SortByType = sortByType;
				refItems.Sort(CompareUpdateItems, (void*)&sortParam);

				CObjArray<uint32> indices(numFiles);

				for(i = 0; i < numFiles; i++) {
					uint32 index = refItems[i].Index;
					indices[i] = index;
					/*
					   const CUpdateItem &ui = updateItems[index];
					   CFileItem file;
					   if(ui.NewProps)
					   UpdateItem_To_FileItem(ui, file);
					   else
					   file = db.Files[ui.IndexInArchive];
					   if(file.IsAnti || file.IsDir)
					   return E_FAIL;
					   newDatabase.Files.Add(file);
					 */
				}

				for(i = 0; i < numFiles; ) {
					uint64 totalSize = 0;
					unsigned numSubFiles;

					const wchar_t * prevExtension = NULL;

					for(numSubFiles = 0; i + numSubFiles < numFiles && numSubFiles < numSolidFiles; numSubFiles++) {
						const CUpdateItem &ui = updateItems[indices[i + numSubFiles]];
						totalSize += ui.Size;
						if(totalSize > options.NumSolidBytes)
							break;
						if(options.SolidExtension) {
							int slashPos = ui.Name.ReverseFind_PathSepar();
							int dotPos = ui.Name.ReverseFind_Dot();
							const wchar_t * ext = ui.Name.Ptr(dotPos <= slashPos ? ui.Name.Len() : dotPos + 1);
							if(numSubFiles == 0)
								prevExtension = ext;
							else if(!StringsAreEqualNoCase(ext, prevExtension))
								break;
						}
					}

					if(numSubFiles < 1)
						numSubFiles = 1;

					RINOK(lps->SetCur());

					CFolderInStream * inStreamSpec = new CFolderInStream;
					CMyComPtr<ISequentialInStream> solidInStream(inStreamSpec);
					inStreamSpec->Init(updateCallback, &indices[i], numSubFiles);

					unsigned startPackIndex = newDatabase.PackSizes.Size();
					uint64 curFolderUnpackSize;
					RINOK(encoder.Encode(
									EXTERNAL_CODECS_LOC_VARS
									solidInStream,
												// NULL,
									&inSizeForReduce,
									newDatabase.Folders.AddNew(), newDatabase.CoderUnpackSizes, curFolderUnpackSize,
									archive.SeqStream, newDatabase.PackSizes, progress));

					if(!inStreamSpec->WasFinished())
						return E_FAIL;

					for(; startPackIndex < newDatabase.PackSizes.Size(); startPackIndex++)
						lps->OutSize += newDatabase.PackSizes[startPackIndex];

					lps->InSize += curFolderUnpackSize;
					// for()
					// newDatabase.PackCRCsDefined.Add(false);
					// newDatabase.PackCRCs.Add(0);

					CNum numUnpackStreams = 0;
					uint64 skippedSize = 0;

					for(uint subIndex = 0; subIndex < numSubFiles; subIndex++) {
						const CUpdateItem &ui = updateItems[indices[i + subIndex]];
						CFileItem file;
						CFileItem2 file2;
						UString name;
						if(ui.NewProps) {
							UpdateItem_To_FileItem(ui, file, file2);
							name = ui.Name;
						}
						else {
							GetFile(*db, ui.IndexInArchive, file, file2);
							db->GetPath(ui.IndexInArchive, name);
						}
						if(file2.IsAnti || file.IsDir)
							return E_FAIL;

						/*
						   CFileItem &file = newDatabase.Files[
							  startFileIndexInDatabase + i + subIndex];
						 */
						if(!inStreamSpec->Processed[subIndex]) {
							skippedSize += ui.Size;
							continue;
							// file.Name += ".locked";
						}

						file.Crc = inStreamSpec->CRCs[subIndex];
						file.Size = inStreamSpec->Sizes[subIndex];

						// if(file.Size >= 0) // test purposes
						if(file.Size != 0) {
							file.CrcDefined = true;
							file.HasStream = true;
							numUnpackStreams++;
						}
						else {
							file.CrcDefined = false;
							file.HasStream = false;
						}

						/*
						   file.Parent = ui.ParentFolderIndex;
						   if(ui.TreeFolderIndex >= 0)
						   treeFolderToArcIndex[ui.TreeFolderIndex] = newDatabase.Files.Size();
						   if(totalSecureDataSize != 0)
						   newDatabase.SecureIDs.Add(ui.SecureIndex);
						 */
						newDatabase.AddFile(file, file2, name);
					}

					// numUnpackStreams = 0 is very bad case for locked files
					// v3.13 doesn't understand it.
					newDatabase.NumUnpackStreamsVector.Add(numUnpackStreams);
					i += numSubFiles;

					if(skippedSize != 0 && complexity >= skippedSize) {
						complexity -= skippedSize;
						RINOK(updateCallback->SetTotal(complexity));
					}
				}
			}

			RINOK(lps->SetCur());

			/*
			   fileIndexToUpdateIndexMap.ClearAndFree();
			   groups.ClearAndFree();
			 */

			/*
			   for(i = 0; i < newDatabase.Files.Size(); i++)
			   {
			   CFileItem &file = newDatabase.Files[i];
			   file.Parent = treeFolderToArcIndex[file.Parent];
			   }

			   if(totalSecureDataSize != 0)
			   {
			   newDatabase.SecureBuf.SetCapacity(totalSecureDataSize);
			   size_t pos = 0;
			   newDatabase.SecureSizes.Reserve(secureBlocks.Sorted.Size());
			   for(i = 0; i < secureBlocks.Sorted.Size(); i++)
			   {
				const CByteBuffer &buf = secureBlocks.Bufs[secureBlocks.Sorted[i]];
				size_t size = buf.GetCapacity();
				if(size != 0)
				  memcpy(newDatabase.SecureBuf + pos, buf, size);
				newDatabase.SecureSizes.Add((uint32)size);
				pos += size;
			   }
			   }
			 */
			newDatabase.ReserveDown();
			if(opCallback)
				RINOK(opCallback->ReportOperation(NEventIndexType::kNoIndex, (uint32)(int32)-1, NUpdateNotifyOp::kHeader));
			return S_OK;
		}
		//
		// Handler
		//
		CHandler::CHandler()
		{
		  #ifndef _NO_CRYPTO
			_isEncrypted = false;
			_passwordIsDefined = false;
		  #endif
		  #ifdef EXTRACT_ONLY
			_crcSize = 4;
		  #ifdef __7Z_SET_PROPERTIES
			_numThreads = NSystem::GetNumberOfProcessors();
			_useMultiThreadMixer = true;
		  #endif
		  #endif
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _db.Files.Size();
			return S_OK;
		}

		#ifdef _SFX
			IMP_IInArchive_ArcProps_NO_Table

			STDMETHODIMP CHandler::GetNumberOfProperties(uint32 * numProps)
			{
				*numProps = 0;
				return S_OK;
			}

			STDMETHODIMP CHandler::GetPropertyInfo(uint32 /* index */, BSTR * /* name */, PROPID * /* propID */, VARTYPE * /*varType*/)
			{
				return E_NOTIMPL;
			}
		#else

		static const Byte kArcProps[] = { kpidHeadersSize, kpidMethod, kpidSolid, kpidNumBlocks /*, kpidIsTree*/ };

		IMP_IInArchive_ArcProps

		static char FASTCALL GetHex(unsigned value) { return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10))); }

		static unsigned FASTCALL ConvertMethodIdToString_Back(char * s, uint64 id)
		{
			int len = 0;
			do {
				s[--len] = GetHex((uint)id & 0xF); id >>= 4;
				s[--len] = GetHex((uint)id & 0xF); id >>= 4;
			} while(id != 0);
			return (uint)-len;
		}

		static void ConvertMethodIdToString(AString &res, uint64 id)
		{
			const uint kLen = 32;
			char s[kLen];
			uint len = kLen - 1;
			s[len] = 0;
			res += s + len - ConvertMethodIdToString_Back(s + len, id);
		}

		static unsigned FASTCALL GetStringForSizeValue(char * s, uint32 val)
		{
			uint i;
			for(i = 0; i <= 31; i++) {
				if(((uint32)1 << i) == val) {
					if(i < 10) {
						s[0] = (char)('0' + i);
						s[1] = 0;
						return 1;
					}
					else {
						if(i < 20) {
							s[0] = '1'; s[1] = (char)('0' + i - 10);
						}
						else if(i < 30) {
							s[0] = '2'; s[1] = (char)('0' + i - 20);
						}
						else {             
							s[0] = '3'; s[1] = (char)('0' + i - 30); 
						}
						s[2] = 0;
						return 2;
					}
				}
			}
			char c = 'b';
			if((val & ((1 << 20) - 1)) == 0) {
				val >>= 20; c = 'm';
			}
			else if((val & ((1 << 10) - 1)) == 0) {
				val >>= 10; c = 'k';
			}
			::ConvertUInt32ToString(val, s);
			unsigned pos = sstrlen(s);
			s[pos++] = c;
			s[pos] = 0;
			return pos;
		}
		/*
		   static inline void AddHexToString(UString &res, Byte value)
		   {
		   res += GetHex((Byte)(value >> 4));
		   res += GetHex((Byte)(value & 0xF));
		   }
		 */
		static char * FASTCALL AddProp32(char * s, const char * name, uint32 v)
		{
			*s++ = ':';
			s = MyStpCpy(s, name);
			::ConvertUInt32ToString(v, s);
			return s + sstrlen(s);
		}

		void CHandler::AddMethodName(AString &s, uint64 id)
		{
			AString name;
			FindMethod(EXTERNAL_CODECS_VARS id, name);
			if(name.IsEmpty())
				ConvertMethodIdToString(s, id);
			else
				s += name;
		}

		#endif

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
		  #ifndef _SFX
			COM_TRY_BEGIN
		  #endif
			NCOM::CPropVariant prop;
			switch(propID) {
			#ifndef _SFX
				case kpidMethod:
				{
					AString s;
					const CParsedMethods &pm = _db.ParsedMethods;
					FOR_VECTOR(i, pm.IDs) {
						uint64 id = pm.IDs[i];
						s.Add_Space_if_NotEmpty();
						char temp[16];
						if(id == k_LZMA2) {
							s += "LZMA2:";
							if((pm.Lzma2Prop & 1) == 0)
								ConvertUInt32ToString((pm.Lzma2Prop >> 1) + 12, temp);
							else
								GetStringForSizeValue(temp, 3 << ((pm.Lzma2Prop >> 1) + 11));
							s += temp;
						}
						else if(id == k_LZMA) {
							s += "LZMA:";
							GetStringForSizeValue(temp, pm.LzmaDic);
							s += temp;
						}
						else
							AddMethodName(s, id);
					}
					prop = s;
					break;
				}
				case kpidSolid: prop = _db.IsSolid(); break;
				case kpidNumBlocks: prop = (uint32)_db.NumFolders; break;
				case kpidHeadersSize:  prop = _db.HeadersSize; break;
				case kpidPhySize:  prop = _db.PhySize; break;
				case kpidOffset: if(_db.ArcInfo.StartPosition != 0) prop = _db.ArcInfo.StartPosition; break;
					/*
					   case kpidIsTree: if(_db.IsTree) prop = true; break;
					   case kpidIsAltStream: if(_db.ThereAreAltStreams) prop = true; break;
					   case kpidIsAux: if(_db.IsTree) prop = true; break;
					 */
					// case kpidError: if(_db.ThereIsHeaderError) prop = "Header error"; break;
			#endif

				case kpidWarningFlags:
				{
					uint32 v = 0;
					if(_db.StartHeaderWasRecovered) v |= kpv_ErrorFlags_HeadersError;
					if(_db.UnsupportedFeatureWarning) v |= kpv_ErrorFlags_UnsupportedFeature;
					if(v != 0)
						prop = v;
					break;
				}

				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!_db.IsArc) v |= kpv_ErrorFlags_IsNotArc;
					if(_db.ThereIsHeaderError) v |= kpv_ErrorFlags_HeadersError;
					if(_db.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
					// if(_db.UnsupportedVersion) v |= kpv_ErrorFlags_Unsupported;
					if(_db.UnsupportedFeatureError) v |= kpv_ErrorFlags_UnsupportedFeature;
					prop = v;
					break;
				}
			}
			prop.Detach(value);
			return S_OK;
		  #ifndef _SFX
			COM_TRY_END
		  #endif
		}

		static void FASTCALL SetFileTimeProp_From_UInt64Def(PROPVARIANT * prop, const CUInt64DefVector &v, int index)
		{
			uint64 value;
			if(v.GetItem(index, value))
				PropVarEm_Set_FileTime64(prop, value);
		}

		bool CHandler::IsFolderEncrypted(CNum folderIndex) const
		{
			if(folderIndex == kNumNoIndex)
				return false;
			size_t startPos = _db.FoCodersDataOffset[folderIndex];
			const Byte * p = _db.CodersData + startPos;
			size_t size = _db.FoCodersDataOffset[folderIndex + 1] - startPos;
			CInByte2 inByte;
			inByte.Init(p, size);

			CNum numCoders = inByte.ReadNum();
			for(; numCoders != 0; numCoders--) {
				Byte mainByte = inByte.ReadByte();
				unsigned idSize = (mainByte & 0xF);
				const Byte * longID = inByte.GetPtr();
				uint64 id64 = 0;
				for(uint j = 0; j < idSize; j++)
					id64 = ((id64 << 8) | longID[j]);
				inByte.SkipDataNoCheck(idSize);
				if(id64 == k_AES)
					return true;
				if((mainByte & 0x20) != 0)
					inByte.SkipDataNoCheck(inByte.ReadNum());
			}
			return false;
		}

		STDMETHODIMP CHandler::GetNumRawProps(uint32 * numProps)
		{
			*numProps = 0;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetRawPropInfo(uint32 /* index */, BSTR * name, PROPID * propID)
		{
			*name = NULL;
			*propID = kpidNtSecure;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetParent(uint32 /* index */, uint32 * parent, uint32 * parentType)
		{
			/*
			   const CFileItem &file = _db.Files[index];
			   *parentType = (file.IsAltStream ? NParentType::kAltStream : NParentType::kDir);
			   *parent = (uint32)(int32)file.Parent;
			 */
			*parentType = NParentType::kDir;
			*parent = (uint32)(int32)-1;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetRawProp(uint32 index, PROPID propID, const void ** data, uint32 * dataSize, uint32 * propType)
		{
			*data = NULL;
			*dataSize = 0;
			*propType = 0;
			if(/* _db.IsTree && propID == kpidName || !_db.IsTree && */propID == kpidPath) {
				if(_db.NameOffsets && _db.NamesBuf) {
					size_t offset = _db.NameOffsets[index];
					size_t size = (_db.NameOffsets[index + 1] - offset) * 2;
					if(size < ((uint32)1 << 31)) {
						*data = (const void*)(_db.NamesBuf + offset * 2);
						*dataSize = (uint32)size;
						*propType = NPropDataType::kUtf16z;
					}
				}
				return S_OK;
			}
			/*
			   if(propID == kpidNtSecure)
			   {
			   if(index < (uint32)_db.SecureIDs.Size())
			   {
				int id = _db.SecureIDs[index];
				size_t offs = _db.SecureOffsets[id];
				size_t size = _db.SecureOffsets[id + 1] - offs;
				if(size >= 0)
				{
			   *data = _db.SecureBuf + offs;
			   *dataSize = (uint32)size;
			   *propType = NPropDataType::kRaw;
				}
			   }
			   }
			 */
			return S_OK;
		}

		#ifndef _SFX
			HRESULT CHandler::SetMethodToProp(CNum folderIndex, PROPVARIANT * prop) const
			{
				PropVariant_Clear(prop);
				if(folderIndex == kNumNoIndex)
					return S_OK;
				// for(int ttt = 0; ttt < 1; ttt++) {
				const uint kTempSize = 256;
				char temp[kTempSize];
				unsigned pos = kTempSize;
				temp[--pos] = 0;

				size_t startPos = _db.FoCodersDataOffset[folderIndex];
				const Byte * p = _db.CodersData + startPos;
				size_t size = _db.FoCodersDataOffset[folderIndex + 1] - startPos;
				CInByte2 inByte;
				inByte.Init(p, size);

				// numCoders == 0 ???
				CNum numCoders = inByte.ReadNum();
				bool needSpace = false;

				for(; numCoders != 0; numCoders--, needSpace = true) {
					if(pos < 32) // max size of property
						break;
					Byte mainByte = inByte.ReadByte();
					unsigned idSize = (mainByte & 0xF);
					const Byte * longID = inByte.GetPtr();
					uint64 id64 = 0;
					for(uint j = 0; j < idSize; j++)
						id64 = ((id64 << 8) | longID[j]);
					inByte.SkipDataNoCheck(idSize);

					if((mainByte & 0x10) != 0) {
						inByte.ReadNum(); // NumInStreams
						inByte.ReadNum(); // NumOutStreams
					}

					CNum propsSize = 0;
					const Byte * props = NULL;
					if((mainByte & 0x20) != 0) {
						propsSize = inByte.ReadNum();
						props = inByte.GetPtr();
						inByte.SkipDataNoCheck(propsSize);
					}

					const char * name = NULL;
					char s[32];
					s[0] = 0;

					if(id64 <= (uint32)0xFFFFFFFF) {
						uint32 id = (uint32)id64;
						if(id == k_LZMA) {
							name = "LZMA";
							if(propsSize == 5) {
								uint32 dicSize = GetUi32((const Byte*)props + 1);
								char * dest = s + GetStringForSizeValue(s, dicSize);
								uint32 d = props[0];
								if(d != 0x5D) {
									uint32 lc = d % 9;
									d /= 9;
									uint32 pb = d / 5;
									uint32 lp = d % 5;
									if(lc != 3) dest = AddProp32(dest, "lc", lc);
									if(lp != 0) dest = AddProp32(dest, "lp", lp);
									if(pb != 2) dest = AddProp32(dest, "pb", pb);
								}
							}
						}
						else if(id == k_LZMA2) {
							name = "LZMA2";
							if(propsSize == 1) {
								Byte d = props[0];
								if((d & 1) == 0)
									ConvertUInt32ToString((uint32)((d >> 1) + 12), s);
								else
									GetStringForSizeValue(s, 3 << ((d >> 1) + 11));
							}
						}
						else if(id == k_PPMD) {
							name = "PPMD";
							if(propsSize == 5) {
								Byte order = *props;
								char * dest = s;
								*dest++ = 'o';
								ConvertUInt32ToString(order, dest);
								dest += sstrlen(dest);
								dest = MyStpCpy(dest, ":mem");
								GetStringForSizeValue(dest, GetUi32(props + 1));
							}
						}
						else if(id == k_Delta) {
							name = "Delta";
							if(propsSize == 1)
								ConvertUInt32ToString((uint32)props[0] + 1, s);
						}
						else if(id == k_BCJ2) name = "BCJ2";
						else if(id == k_BCJ) name = "BCJ";
						else if(id == k_AES) {
							name = "7zAES";
							if(propsSize >= 1) {
								Byte firstByte = props[0];
								uint32 numCyclesPower = firstByte & 0x3F;
								ConvertUInt32ToString(numCyclesPower, s);
							}
						}
					}

					if(name) {
						unsigned nameLen = sstrlen(name);
						unsigned propsLen = sstrlen(s);
						unsigned totalLen = nameLen + propsLen;
						if(propsLen != 0)
							totalLen++;
						if(needSpace)
							totalLen++;
						if(totalLen + 5 >= pos)
							break;
						pos -= totalLen;
						sstrcpy(temp + pos, name);
						if(propsLen != 0) {
							char * dest = temp + pos + nameLen;
							*dest++ = ':';
							sstrcpy(dest, s);
						}
						if(needSpace)
							temp[pos + totalLen - 1] = ' ';
					}
					else {
						AString methodName;
						FindMethod(EXTERNAL_CODECS_VARS id64, methodName);
						if(needSpace)
							temp[--pos] = ' ';
						if(methodName.IsEmpty())
							pos -= ConvertMethodIdToString_Back(temp + pos, id64);
						else {
							uint len = methodName.Len();
							if(len + 5 > pos)
								break;
							pos -= len;
							for(uint i = 0; i < len; i++)
								temp[pos + i] = methodName[i];
						}
					}
				}

				if(numCoders != 0 && pos >= 4) {
					temp[--pos] = ' ';
					temp[--pos] = '.';
					temp[--pos] = '.';
					temp[--pos] = '.';
				}

				return PropVarEm_Set_Str(prop, temp + pos);
				// }
			}
		#endif

		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			PropVariant_Clear(value);
			// COM_TRY_BEGIN
			// NCOM::CPropVariant prop;

			/*
			   const CRef2 &ref2 = _refs[index];
			   if(ref2.Refs.IsEmpty())
			   return E_FAIL;
			   const CRef &ref = ref2.Refs.Front();
			 */
			const CFileItem &item = _db.Files[index];
			const uint32 index2 = index;
			switch(propID) {
				case kpidIsDir: PropVarEm_Set_Bool(value, item.IsDir); break;
				case kpidSize:
				{
					PropVarEm_Set_UInt64(value, item.Size);
					// prop = ref2.Size;
					break;
				}
				case kpidPackSize:
				{
					// prop = ref2.PackSize;
					{
						CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
						if(folderIndex != kNumNoIndex) {
							if(_db.FolderStartFileIndex[folderIndex] == (CNum)index2)
								PropVarEm_Set_UInt64(value, _db.GetFolderFullPackSize(folderIndex));
							/*
							   else
							   PropVarEm_Set_UInt64(value, 0);
							 */
						}
						else
							PropVarEm_Set_UInt64(value, 0);
					}
					break;
				}
				// case kpidIsAux: prop = _db.IsItemAux(index2); break;
				case kpidPosition:  { uint64 v; if(_db.StartPos.GetItem(index2, v)) PropVarEm_Set_UInt64(value, v); break; }
				case kpidCTime:  SetFileTimeProp_From_UInt64Def(value, _db.CTime, index2); break;
				case kpidATime:  SetFileTimeProp_From_UInt64Def(value, _db.ATime, index2); break;
				case kpidMTime:  SetFileTimeProp_From_UInt64Def(value, _db.MTime, index2); break;
				case kpidAttrib:  if(_db.Attrib.ValidAndDefined(index2)) PropVarEm_Set_UInt32(value, _db.Attrib.Vals[index2]); break;
				case kpidCRC:  if(item.CrcDefined) PropVarEm_Set_UInt32(value, item.Crc); break;
				case kpidEncrypted:  PropVarEm_Set_Bool(value, IsFolderEncrypted(_db.FileIndexToFolderIndexMap[index2])); break;
				case kpidIsAnti:  PropVarEm_Set_Bool(value, _db.IsItemAnti(index2)); break;
				/*
				   case kpidIsAltStream:  prop = item.IsAltStream; break;
				   case kpidNtSecure:
				   {
					int id = _db.SecureIDs[index];
					size_t offs = _db.SecureOffsets[id];
					size_t size = _db.SecureOffsets[id + 1] - offs;
					if(size >= 0) {
					  prop.SetBlob(_db.SecureBuf + offs, (ULONG)size);
					}
					break;
				   }
				 */

				case kpidPath: return _db.GetPath_Prop(index, value);

			#ifndef _SFX

				case kpidMethod: return SetMethodToProp(_db.FileIndexToFolderIndexMap[index2], value);
				case kpidBlock:
				{
					CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
					if(folderIndex != kNumNoIndex)
						PropVarEm_Set_UInt32(value, (uint32)folderIndex);
				}
				break;
					/*
					   case kpidPackedSize0:
					   case kpidPackedSize1:
					   case kpidPackedSize2:
					   case kpidPackedSize3:
					   case kpidPackedSize4:
					   {
						CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
						if(folderIndex != kNumNoIndex) {
						  if(_db.FolderStartFileIndex[folderIndex] == (CNum)index2 && _db.FoStartPackStreamIndex[folderIndex + 1] - _db.FoStartPackStreamIndex[folderIndex] > (propID - kpidPackedSize0)) {
							PropVarEm_Set_UInt64(value, _db.GetFolderPackStreamSize(folderIndex, propID -
							   kpidPackedSize0));
						  }
						}
						else
						  PropVarEm_Set_UInt64(value, 0);
					   }
					   break;
					 */

			#endif
			}
			// prop.Detach(value);
			return S_OK;
			// COM_TRY_END
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openArchiveCallback)
		{
			COM_TRY_BEGIN
			Close();
		  #ifndef _SFX
			_fileInfoPopIDs.Clear();
		  #endif
			try {
				CMyComPtr<IArchiveOpenCallback> openArchiveCallbackTemp = openArchiveCallback;
			#ifndef _NO_CRYPTO
				CMyComPtr<ICryptoGetTextPassword> getTextPassword;
				if(openArchiveCallback)
					openArchiveCallbackTemp.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
			#endif
				CInArchive archive(
			  #ifdef __7Z_SET_PROPERTIES
							_useMultiThreadMixer
			  #else
							true
			  #endif
							);
				_db.IsArc = false;
				RINOK(archive.Open(stream, maxCheckStartPosition));
				_db.IsArc = true;

				HRESULT result = archive.ReadDatabase(EXTERNAL_CODECS_VARS _db
			#ifndef _NO_CRYPTO
							, getTextPassword, _isEncrypted, _passwordIsDefined, _password
			#endif
							);
				RINOK(result);
				_inStream = stream;
			}
			catch(...) {
				Close();
				// return E_INVALIDARG;
				// return S_FALSE;
				// we must return out_of_memory here
				return E_OUTOFMEMORY;
			}
			// _inStream = stream;
		  #ifndef _SFX
			FillPopIDs();
		  #endif
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			COM_TRY_BEGIN
			_inStream.Release();
			_db.Clear();
		  #ifndef _NO_CRYPTO
			_isEncrypted = false;
			_passwordIsDefined = false;
			_password.Empty();
		  #endif
			return S_OK;
			COM_TRY_END
		}

		#ifdef __7Z_SET_PROPERTIES
			#ifdef EXTRACT_ONLY
				STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
				{
					COM_TRY_BEGIN
					const uint32 numProcessors = NSystem::GetNumberOfProcessors();
					_numThreads = numProcessors;
					_useMultiThreadMixer = true;

					for(uint32 i = 0; i < numProps; i++) {
						UString name = names[i];
						name.MakeLower_Ascii();
						if(name.IsEmpty())
							return E_INVALIDARG;
						const PROPVARIANT &value = values[i];
						uint32 number;
						unsigned index = ParseStringToUInt32(name, number);
						if(index == 0) {
							if(name.IsEqualTo("mtf")) {
								RINOK(PROPVARIANT_to_bool(value, _useMultiThreadMixer));
								continue;
							}
							if(name.IsPrefixedBy_Ascii_NoCase("mt")) {
								RINOK(ParseMtProp(name.Ptr(2), value, numProcessors, _numThreads));
								continue;
							}
							else
								return E_INVALIDARG;
						}
					}
					return S_OK;
					COM_TRY_END
				}
			#endif
		#endif

		IMPL_ISetCompressCodecsInfo
		//
		// HandlerOut
		//
		#define k_LZMA_Name "LZMA"
		#define kDefaultMethodName "LZMA2"
		#define k_Copy_Name "Copy"
		#define k_MatchFinder_ForHeaders "BT2"

		static const uint32 k_NumFastBytes_ForHeaders = 273;
		static const uint32 k_Level_ForHeaders = 5;
		#ifdef UNDER_CE
			static const uint32 k_Dictionary_ForHeaders = 1 << 18;
		#else
			static const uint32 k_Dictionary_ForHeaders = 1 << 20;
		#endif

		STDMETHODIMP CHandler::GetFileTimeType(uint32 * type)
		{
			*type = NFileTimeType::kWindows;
			return S_OK;
		}

		HRESULT CHandler::PropsMethod_To_FullMethod(CMethodFull &dest, const COneMethodInfo &m)
		{
			if(!FindMethod(EXTERNAL_CODECS_VARS m.MethodName, dest.Id, dest.NumStreams))
				return E_INVALIDARG;
			(CProps &)dest = (CProps &)m;
			return S_OK;
		}

		HRESULT CHandler::SetHeaderMethod(CCompressionMethodMode &headerMethod)
		{
			if(!_compressHeaders)
				return S_OK;
			COneMethodInfo m;
			m.MethodName = k_LZMA_Name;
			m.AddProp_Ascii(NCoderPropID::kMatchFinder, k_MatchFinder_ForHeaders);
			m.AddProp_Level(k_Level_ForHeaders);
			m.AddProp32(NCoderPropID::kNumFastBytes, k_NumFastBytes_ForHeaders);
			m.AddProp32(NCoderPropID::kDictionarySize, k_Dictionary_ForHeaders);
			m.AddProp_NumThreads(1);

			CMethodFull & methodFull = headerMethod.Methods.AddNew();
			return PropsMethod_To_FullMethod(methodFull, m);
		}

		HRESULT CHandler::SetMainMethod(CCompressionMethodMode &methodMode
			#ifndef _7ZIP_ST
					, uint32 numThreads
			#endif
					)
		{
			methodMode.Bonds = _bonds;
			CObjectVector<COneMethodInfo> methods = _methods;
			{
				FOR_VECTOR(i, methods) {
					AString &methodName = methods[i].MethodName;
					if(methodName.IsEmpty())
						methodName = kDefaultMethodName;
				}
				if(methods.IsEmpty()) {
					COneMethodInfo &m = methods.AddNew();
					m.MethodName = (GetLevel() == 0 ? k_Copy_Name : kDefaultMethodName);
					methodMode.DefaultMethod_was_Inserted = true;
				}
			}

			if(!_filterMethod.MethodName.IsEmpty()) {
				// if(methodMode.Bonds.IsEmpty())
				{
					FOR_VECTOR(k, methodMode.Bonds) {
						CBond2 &bond = methodMode.Bonds[k];
						bond.InCoder++;
						bond.OutCoder++;
					}
					methods.Insert(0, _filterMethod);
					methodMode.Filter_was_Inserted = true;
				}
			}
			const uint64 kSolidBytes_Min = (1 << 24);
			const uint64 kSolidBytes_Max = ((uint64)1 << 32) - 1;
			bool needSolid = false;
			FOR_VECTOR(i, methods) {
				COneMethodInfo &oneMethodInfo = methods[i];
				SetGlobalLevelTo(oneMethodInfo);
			#ifndef _7ZIP_ST
				CMultiMethodProps::SetMethodThreadsTo(oneMethodInfo, numThreads);
			#endif

				CMethodFull &methodFull = methodMode.Methods.AddNew();
				RINOK(PropsMethod_To_FullMethod(methodFull, oneMethodInfo));

				if(methodFull.Id != k_Copy)
					needSolid = true;
				if(_numSolidBytesDefined)
					continue;
				uint32 dicSize;
				switch(methodFull.Id) {
					case k_LZMA:
					case k_LZMA2: dicSize = oneMethodInfo.Get_Lzma_DicSize(); break;
					case k_PPMD: dicSize = oneMethodInfo.Get_Ppmd_MemSize(); break;
					case k_Deflate: dicSize = (uint32)1 << 15; break;
					case k_BZip2: dicSize = oneMethodInfo.Get_BZip2_BlockSize(); break;
					default: continue;
				}

				_numSolidBytes = (uint64)dicSize << 7;
				if(_numSolidBytes < kSolidBytes_Min) _numSolidBytes = kSolidBytes_Min;
				if(_numSolidBytes > kSolidBytes_Max) _numSolidBytes = kSolidBytes_Max;
				_numSolidBytesDefined = true;
			}

			if(!_numSolidBytesDefined)
				if(needSolid)
					_numSolidBytes = kSolidBytes_Max;
				else
					_numSolidBytes = 0;
			_numSolidBytesDefined = true;
			return S_OK;
		}

		static HRESULT GetTime(IArchiveUpdateCallback * updateCallback, int index, PROPID propID, uint64 &ft, bool &ftDefined)
		{
			// ft = 0;
			// ftDefined = false;
			NCOM::CPropVariant prop;
			RINOK(updateCallback->GetProperty(index, propID, &prop));
			if(prop.vt == VT_FILETIME) {
				ft = prop.filetime.dwLowDateTime | ((uint64)prop.filetime.dwHighDateTime << 32);
				ftDefined = true;
			}
			else if(prop.vt != VT_EMPTY)
				return E_INVALIDARG;
			else {
				ft = 0;
				ftDefined = false;
			}
			return S_OK;
		}
		/*
		   #ifdef _WIN32
		   static const wchar_t kDirDelimiter1 = L'\\';
		   #endif
		   static const wchar_t kDirDelimiter2 = L'/';

		   static inline bool IsCharDirLimiter(wchar_t c)
		   {
		   return (
		   #ifdef _WIN32
			c == kDirDelimiter1 ||
		   #endif
			c == kDirDelimiter2);
		   }

		   static int FillSortIndex(CObjectVector<CTreeFolder> &treeFolders, int cur, int curSortIndex)
		   {
		   CTreeFolder &tf = treeFolders[cur];
		   tf.SortIndex = curSortIndex++;
		   for(int i = 0; i < tf.SubFolders.Size(); i++)
			curSortIndex = FillSortIndex(treeFolders, tf.SubFolders[i], curSortIndex);
		   tf.SortIndexEnd = curSortIndex;
		   return curSortIndex;
		   }

		   static int FindSubFolder(const CObjectVector<CTreeFolder> &treeFolders, int cur, const UString &name, int &insertPos)
		   {
		   const CIntVector &subFolders = treeFolders[cur].SubFolders;
		   int left = 0, right = subFolders.Size();
		   insertPos = -1;
		   for(;;) {
			if(left == right) {
			  insertPos = left;
			  return -1;
			}
			int mid = (left + right) / 2;
			int midFolder = subFolders[mid];
			int compare = CompareFileNames(name, treeFolders[midFolder].Name);
			if(compare == 0)
			  return midFolder;
			if(compare < 0)
			  right = mid;
			else
			  left = mid + 1;
		   }
		   }

		   static int AddFolder(CObjectVector<CTreeFolder> &treeFolders, int cur, const UString &name)
		   {
		   int insertPos;
		   int folderIndex = FindSubFolder(treeFolders, cur, name, insertPos);
		   if(folderIndex < 0) {
			folderIndex = treeFolders.Size();
			CTreeFolder &newFolder = treeFolders.AddNew();
			newFolder.Parent = cur;
			newFolder.Name = name;
			treeFolders[cur].SubFolders.Insert(insertPos, folderIndex);
		   }
		   // else if(treeFolders[folderIndex].IsAltStreamFolder != isAltStreamFolder) throw 1123234234;
		   return folderIndex;
		   }
		 */

		STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream * outStream, uint32 numItems,
					IArchiveUpdateCallback * updateCallback)
		{
			COM_TRY_BEGIN

			const CDbEx * db = 0;
		  #ifdef _7Z_VOL
			if(_volumes.Size() > 1)
				return E_FAIL;
			const CVolume * volume = 0;
			if(_volumes.Size() == 1) {
				volume = &_volumes.Front();
				db = &volume->Database;
			}
		  #else
			if(_inStream != 0)
				db = &_db;
		  #endif

			/*
			   CMyComPtr<IArchiveGetRawProps> getRawProps;
			   updateCallback->QueryInterface(IID_IArchiveGetRawProps, (void **)&getRawProps);

			   CUniqBlocks secureBlocks;
			   secureBlocks.AddUniq(NULL, 0);

			   CObjectVector<CTreeFolder> treeFolders;
			   {
			   CTreeFolder folder;
			   folder.Parent = -1;
			   treeFolders.Add(folder);
			   }
			 */

			CObjectVector<CUpdateItem> updateItems;

			bool need_CTime = (Write_CTime.Def && Write_CTime.Val);
			bool need_ATime = (Write_ATime.Def && Write_ATime.Val);
			bool need_MTime = (Write_MTime.Def && Write_MTime.Val || !Write_MTime.Def);
			bool need_Attrib = (Write_Attrib.Def && Write_Attrib.Val || !Write_Attrib.Def);

			if(db && !db->Files.IsEmpty()) {
				if(!Write_CTime.Def) need_CTime = !db->CTime.Defs.IsEmpty();
				if(!Write_ATime.Def) need_ATime = !db->ATime.Defs.IsEmpty();
				if(!Write_MTime.Def) need_MTime = !db->MTime.Defs.IsEmpty();
				if(!Write_Attrib.Def) need_Attrib = !db->Attrib.Defs.IsEmpty();
			}

			// UString s;
			UString name;

			for(uint32 i = 0; i < numItems; i++) {
				int32 newData, newProps;
				uint32 indexInArchive;
				if(!updateCallback)
					return E_FAIL;
				RINOK(updateCallback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArchive));
				CUpdateItem ui;
				ui.NewProps = IntToBool(newProps);
				ui.NewData = IntToBool(newData);
				ui.IndexInArchive = indexInArchive;
				ui.IndexInClient = i;
				ui.IsAnti = false;
				ui.Size = 0;

				name.Empty();
				// bool isAltStream = false;
				if(ui.IndexInArchive != -1) {
					if(db == 0 || (uint)ui.IndexInArchive >= db->Files.Size())
						return E_INVALIDARG;
					const CFileItem &fi = db->Files[ui.IndexInArchive];
					if(!ui.NewProps) {
						_db.GetPath(ui.IndexInArchive, name);
					}
					ui.IsDir = fi.IsDir;
					ui.Size = fi.Size;
					// isAltStream = fi.IsAltStream;
					ui.IsAnti = db->IsItemAnti(ui.IndexInArchive);

					if(!ui.NewProps) {
						ui.CTimeDefined = db->CTime.GetItem(ui.IndexInArchive, ui.CTime);
						ui.ATimeDefined = db->ATime.GetItem(ui.IndexInArchive, ui.ATime);
						ui.MTimeDefined = db->MTime.GetItem(ui.IndexInArchive, ui.MTime);
					}
				}

				if(ui.NewProps) {
					bool folderStatusIsDefined;
					if(need_Attrib) {
						NCOM::CPropVariant prop;
						RINOK(updateCallback->GetProperty(i, kpidAttrib, &prop));
						if(prop.vt == VT_EMPTY)
							ui.AttribDefined = false;
						else if(prop.vt != VT_UI4)
							return E_INVALIDARG;
						else {
							ui.Attrib = prop.ulVal;
							ui.AttribDefined = true;
						}
					}
					// we need MTime to sort files.
					if(need_CTime) RINOK(GetTime(updateCallback, i, kpidCTime, ui.CTime, ui.CTimeDefined));
					if(need_ATime) RINOK(GetTime(updateCallback, i, kpidATime, ui.ATime, ui.ATimeDefined));
					if(need_MTime) RINOK(GetTime(updateCallback, i, kpidMTime, ui.MTime, ui.MTimeDefined));
					/*
					   if(getRawProps) {
					   const void *data;
					   uint32 dataSize;
					   uint32 propType;
					   getRawProps->GetRawProp(i, kpidNtSecure, &data, &dataSize, &propType);
					   if(dataSize != 0 && propType != NPropDataType::kRaw)
						return E_FAIL;
					   ui.SecureIndex = secureBlocks.AddUniq((const Byte *)data, dataSize);
					   }
					 */

					{
						NCOM::CPropVariant prop;
						RINOK(updateCallback->GetProperty(i, kpidPath, &prop));
						if(prop.vt == VT_EMPTY) {
						}
						else if(prop.vt != VT_BSTR)
							return E_INVALIDARG;
						else {
							name = prop.bstrVal;
							NItemName::ReplaceSlashes_OsToUnix(name);
						}
					}
					{
						NCOM::CPropVariant prop;
						RINOK(updateCallback->GetProperty(i, kpidIsDir, &prop));
						if(prop.vt == VT_EMPTY)
							folderStatusIsDefined = false;
						else if(prop.vt != VT_BOOL)
							return E_INVALIDARG;
						else {
							ui.IsDir = (prop.boolVal != VARIANT_FALSE);
							folderStatusIsDefined = true;
						}
					}

					{
						NCOM::CPropVariant prop;
						RINOK(updateCallback->GetProperty(i, kpidIsAnti, &prop));
						if(prop.vt == VT_EMPTY)
							ui.IsAnti = false;
						else if(prop.vt != VT_BOOL)
							return E_INVALIDARG;
						else
							ui.IsAnti = (prop.boolVal != VARIANT_FALSE);
					}

					/*
					   {
					   NCOM::CPropVariant prop;
					   RINOK(updateCallback->GetProperty(i, kpidIsAltStream, &prop));
					   if(prop.vt == VT_EMPTY)
						isAltStream = false;
					   else if(prop.vt != VT_BOOL)
						return E_INVALIDARG;
					   else
						isAltStream = (prop.boolVal != VARIANT_FALSE);
					   }
					 */

					if(ui.IsAnti) {
						ui.AttribDefined = false;

						ui.CTimeDefined = false;
						ui.ATimeDefined = false;
						ui.MTimeDefined = false;

						ui.Size = 0;
					}

					if(!folderStatusIsDefined && ui.AttribDefined)
						ui.SetDirStatusFromAttrib();
				}
				else {
					/*
					   if(_db.SecureIDs.IsEmpty())
					   ui.SecureIndex = secureBlocks.AddUniq(NULL, 0);
					   else {
					   int id = _db.SecureIDs[ui.IndexInArchive];
					   size_t offs = _db.SecureOffsets[id];
					   size_t size = _db.SecureOffsets[id + 1] - offs;
					   ui.SecureIndex = secureBlocks.AddUniq(_db.SecureBuf + offs, size);
					   }
					 */
				}

				/*
				   {
				   int folderIndex = 0;
				   if(_useParents) {
					int j;
					s.Empty();
					for(j = 0; j < name.Len(); j++) {
					  wchar_t c = name[j];
					  if(IsCharDirLimiter(c)) {
						folderIndex = AddFolder(treeFolders, folderIndex, s);
						s.Empty();
						continue;
					  }
					  s += c;
					}
					if(isAltStream) {
					  int colonPos = s.Find(':');
					  if(colonPos < 0) {
						// isAltStream = false;
						return E_INVALIDARG;
					  }
					  UString mainName = s.Left(colonPos);
					  int newFolderIndex = AddFolder(treeFolders, folderIndex, mainName);
					  if(treeFolders[newFolderIndex].UpdateItemIndex < 0) {
						for(int j = updateItems.Size() - 1; j >= 0; j--) {
						  CUpdateItem &ui2 = updateItems[j];
						  if(ui2.ParentFolderIndex == folderIndex && ui2.Name == mainName) {
							ui2.TreeFolderIndex = newFolderIndex;
							treeFolders[newFolderIndex].UpdateItemIndex = j;
						  }
						}
					  }
					  folderIndex = newFolderIndex;
					  s.Delete(0, colonPos + 1);
					}
					ui.Name = s;
				   }
				   else
					ui.Name = name;
				   ui.IsAltStream = isAltStream;
				   ui.ParentFolderIndex = folderIndex;
				   ui.TreeFolderIndex = -1;
				   if(ui.IsDir && !s.IsEmpty()) {
					ui.TreeFolderIndex = AddFolder(treeFolders, folderIndex, s);
					treeFolders[ui.TreeFolderIndex].UpdateItemIndex = updateItems.Size();
				   }
				   }
				 */
				ui.Name = name;
				if(ui.NewData) {
					ui.Size = 0;
					if(!ui.IsDir) {
						NCOM::CPropVariant prop;
						RINOK(updateCallback->GetProperty(i, kpidSize, &prop));
						if(prop.vt != VT_UI8)
							return E_INVALIDARG;
						ui.Size = (uint64)prop.uhVal.QuadPart;
						if(ui.Size != 0 && ui.IsAnti)
							return E_INVALIDARG;
					}
				}
				updateItems.Add(ui);
			}
			/*
			   FillSortIndex(treeFolders, 0, 0);
			   for(i = 0; i < (uint32)updateItems.Size(); i++) {
			   CUpdateItem &ui = updateItems[i];
			   ui.ParentSortIndex = treeFolders[ui.ParentFolderIndex].SortIndex;
			   ui.ParentSortIndexEnd = treeFolders[ui.ParentFolderIndex].SortIndexEnd;
			   }
			 */
			CCompressionMethodMode methodMode, headerMethod;
			HRESULT res = SetMainMethod(methodMode
			#ifndef _7ZIP_ST
						, _numThreads
			#endif
						);
			RINOK(res);
			RINOK(SetHeaderMethod(headerMethod));
		  #ifndef _7ZIP_ST
			methodMode.NumThreads = _numThreads;
			methodMode.MultiThreadMixer = _useMultiThreadMixer;
			headerMethod.NumThreads = 1;
			headerMethod.MultiThreadMixer = _useMultiThreadMixer;
		  #endif
			CMyComPtr<ICryptoGetTextPassword2> getPassword2;
			updateCallback->QueryInterface(IID_ICryptoGetTextPassword2, (void**)&getPassword2);
			methodMode.PasswordIsDefined = false;
			methodMode.Password.Empty();
			if(getPassword2) {
				CMyComBSTR password;
				int32 passwordIsDefined;
				RINOK(getPassword2->CryptoGetTextPassword2(&passwordIsDefined, &password));
				methodMode.PasswordIsDefined = IntToBool(passwordIsDefined);
				if(methodMode.PasswordIsDefined && password)
					methodMode.Password = password;
			}
			bool compressMainHeader = _compressHeaders; // check it
			bool encryptHeaders = false;
		  #ifndef _NO_CRYPTO
			if(!methodMode.PasswordIsDefined && _passwordIsDefined) {
				// if header is compressed, we use that password for updated archive
				methodMode.PasswordIsDefined = true;
				methodMode.Password = _password;
			}
		  #endif

			if(methodMode.PasswordIsDefined) {
				if(_encryptHeadersSpecified)
					encryptHeaders = _encryptHeaders;
			#ifndef _NO_CRYPTO
				else
					encryptHeaders = _passwordIsDefined;
			#endif
				compressMainHeader = true;
				if(encryptHeaders) {
					headerMethod.PasswordIsDefined = methodMode.PasswordIsDefined;
					headerMethod.Password = methodMode.Password;
				}
			}
			if(numItems < 2)
				compressMainHeader = false;
			int level = GetLevel();
			CUpdateOptions options;
			options.Method = &methodMode;
			options.HeaderMethod = (_compressHeaders || encryptHeaders) ? &headerMethod : NULL;
			options.UseFilters = (level != 0 && _autoFilter && !methodMode.Filter_was_Inserted);
			options.MaxFilter = (level >= 8);
			options.AnalysisLevel = GetAnalysisLevel();

			options.HeaderOptions.CompressMainHeader = compressMainHeader;
			/*
			   options.HeaderOptions.WriteCTime = Write_CTime;
			   options.HeaderOptions.WriteATime = Write_ATime;
			   options.HeaderOptions.WriteMTime = Write_MTime;
			   options.HeaderOptions.WriteAttrib = Write_Attrib;
			 */
			options.NumSolidFiles = _numSolidFiles;
			options.NumSolidBytes = _numSolidBytes;
			options.SolidExtension = _solidExtension;
			options.UseTypeSorting = _useTypeSorting;
			options.RemoveSfxBlock = _removeSfxBlock;
			// options.VolumeMode = _volumeMode;
			options.MultiThreadMixer = _useMultiThreadMixer;
			COutArchive archive;
			CArchiveDatabaseOut newDatabase;
			CMyComPtr<ICryptoGetTextPassword> getPassword;
			updateCallback->QueryInterface(IID_ICryptoGetTextPassword, (void**)&getPassword);

			/*
			   if(secureBlocks.Sorted.Size() > 1) {
			   secureBlocks.GetReverseMap();
			   for(int i = 0; i < updateItems.Size(); i++) {
				int &secureIndex = updateItems[i].SecureIndex;
				secureIndex = secureBlocks.BufIndexToSortedIndex[secureIndex];
			   }
			   }
			 */
			res = Update(EXTERNAL_CODECS_VARS
			  #ifdef _7Z_VOL
						volume ? volume->Stream : 0, volume ? db : 0,
			  #else
						_inStream, db,
			  #endif
						updateItems,
								// treeFolders,
								// secureBlocks,
						archive, newDatabase, outStream, updateCallback, options
			  #ifndef _NO_CRYPTO
						, getPassword
			  #endif
						);
			RINOK(res);
			updateItems.ClearAndFree();
			return archive.WriteDatabase(EXTERNAL_CODECS_VARS newDatabase, options.HeaderMethod, options.HeaderOptions);
			COM_TRY_END
		}

		static HRESULT ParseBond(UString &srcString, uint32 &coder, uint32 &stream)
		{
			stream = 0;
			{
				uint index = ParseStringToUInt32(srcString, coder);
				if(index == 0)
					return E_INVALIDARG;
				srcString.DeleteFrontal(index);
			}
			if(srcString[0] == 's') {
				srcString.Delete(0);
				uint index = ParseStringToUInt32(srcString, stream);
				if(index == 0)
					return E_INVALIDARG;
				srcString.DeleteFrontal(index);
			}
			return S_OK;
		}

		void COutHandler::InitSolidFiles() { _numSolidFiles = (uint64)(int64)(-1); }
		void COutHandler::InitSolidSize()  { _numSolidBytes = (uint64)(int64)(-1); }

		void COutHandler::InitSolid()
		{
			InitSolidFiles();
			InitSolidSize();
			_solidExtension = false;
			_numSolidBytesDefined = false;
		}

		void COutHandler::InitProps()
		{
			CMultiMethodProps::Init();
			_removeSfxBlock = false;
			_compressHeaders = true;
			_encryptHeadersSpecified = false;
			_encryptHeaders = false;
			// _useParents = false;
			Write_CTime.Init();
			Write_ATime.Init();
			Write_MTime.Init();
			Write_Attrib.Init();
			_useMultiThreadMixer = true;

			// _volumeMode = false;

			InitSolid();
			_useTypeSorting = false;
		}

		HRESULT COutHandler::SetSolidFromString(const UString &s)
		{
			UString s2 = s;
			s2.MakeLower_Ascii();
			for(uint i = 0; i < s2.Len(); ) {
				const wchar_t * start = ((const wchar_t*)s2) + i;
				const wchar_t * end;
				uint64 v = ConvertStringToUInt64(start, &end);
				if(start == end) {
					if(s2[i++] != 'e')
						return E_INVALIDARG;
					_solidExtension = true;
					continue;
				}
				i += (int)(end - start);
				if(i == s2.Len())
					return E_INVALIDARG;
				wchar_t c = s2[i++];
				if(c == 'f') {
					if(v < 1)
						v = 1;
					_numSolidFiles = v;
				}
				else {
					unsigned numBits;
					switch(c) {
						case 'b': numBits =  0; break;
						case 'k': numBits = 10; break;
						case 'm': numBits = 20; break;
						case 'g': numBits = 30; break;
						case 't': numBits = 40; break;
						default: return E_INVALIDARG;
					}
					_numSolidBytes = (v << numBits);
					_numSolidBytesDefined = true;
				}
			}
			return S_OK;
		}

		HRESULT COutHandler::SetSolidFromPROPVARIANT(const PROPVARIANT &value)
		{
			bool isSolid;
			switch(value.vt) {
				case VT_EMPTY: isSolid = true; break;
				case VT_BOOL: isSolid = (value.boolVal != VARIANT_FALSE); break;
				case VT_BSTR:
					if(StringToBool(value.bstrVal, isSolid))
						break;
					return SetSolidFromString(value.bstrVal);
				default: return E_INVALIDARG;
			}
			if(isSolid)
				InitSolid();
			else
				_numSolidFiles = 1;
			return S_OK;
		}

		static HRESULT PROPVARIANT_to_BoolPair(const PROPVARIANT &prop, CBoolPair &dest)
		{
			RINOK(PROPVARIANT_to_bool(prop, dest.Val));
			dest.Def = true;
			return S_OK;
		}

		HRESULT COutHandler::SetProperty(const wchar_t * nameSpec, const PROPVARIANT &value)
		{
			UString name = nameSpec;
			name.MakeLower_Ascii();
			if(name.IsEmpty())
				return E_INVALIDARG;

			if(name[0] == L's') {
				name.Delete(0);
				if(name.IsEmpty())
					return SetSolidFromPROPVARIANT(value);
				if(value.vt != VT_EMPTY)
					return E_INVALIDARG;
				return SetSolidFromString(name);
			}

			uint32 number;
			int index = ParseStringToUInt32(name, number);
			// UString realName = name.Ptr(index);
			if(index == 0) {
				if(name.IsEqualTo("rsfx")) return PROPVARIANT_to_bool(value, _removeSfxBlock);
				if(name.IsEqualTo("hc")) return PROPVARIANT_to_bool(value, _compressHeaders);
				// if(name.IsEqualToNoCase(L"HS")) return PROPVARIANT_to_bool(value, _useParents);

				if(name.IsEqualTo("hcf")) {
					bool compressHeadersFull = true;
					RINOK(PROPVARIANT_to_bool(value, compressHeadersFull));
					return compressHeadersFull ? S_OK : E_INVALIDARG;
				}

				if(name.IsEqualTo("he")) {
					RINOK(PROPVARIANT_to_bool(value, _encryptHeaders));
					_encryptHeadersSpecified = true;
					return S_OK;
				}

				if(name.IsEqualTo("tc")) return PROPVARIANT_to_BoolPair(value, Write_CTime);
				if(name.IsEqualTo("ta")) return PROPVARIANT_to_BoolPair(value, Write_ATime);
				if(name.IsEqualTo("tm")) return PROPVARIANT_to_BoolPair(value, Write_MTime);

				if(name.IsEqualTo("tr")) return PROPVARIANT_to_BoolPair(value, Write_Attrib);

				if(name.IsEqualTo("mtf")) return PROPVARIANT_to_bool(value, _useMultiThreadMixer);

				if(name.IsEqualTo("qs")) return PROPVARIANT_to_bool(value, _useTypeSorting);

				// if(name.IsEqualTo("v"))  return PROPVARIANT_to_bool(value, _volumeMode);
			}
			return CMultiMethodProps::SetProperty(name, value);
		}

		STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
		{
			COM_TRY_BEGIN
			_bonds.Clear();
			InitProps();
			for(uint32 i = 0; i < numProps; i++) {
				UString name = names[i];
				name.MakeLower_Ascii();
				if(name.IsEmpty())
					return E_INVALIDARG;
				const PROPVARIANT &value = values[i];
				if(name[0] == 'b') {
					if(value.vt != VT_EMPTY)
						return E_INVALIDARG;
					name.Delete(0);
					CBond2 bond;
					RINOK(ParseBond(name, bond.OutCoder, bond.OutStream));
					if(name[0] != ':')
						return E_INVALIDARG;
					name.Delete(0);
					uint32 inStream = 0;
					RINOK(ParseBond(name, bond.InCoder, inStream));
					if(inStream != 0)
						return E_INVALIDARG;
					if(!name.IsEmpty())
						return E_INVALIDARG;
					_bonds.Add(bond);
					continue;
				}
				RINOK(SetProperty(name, value));
			}
			uint numEmptyMethods = GetNumEmptyMethods();
			if(numEmptyMethods > 0) {
				uint k;
				for(k = 0; k < _bonds.Size(); k++) {
					const CBond2 &bond = _bonds[k];
					if(bond.InCoder < (uint32)numEmptyMethods || bond.OutCoder < (uint32)numEmptyMethods)
						return E_INVALIDARG;
				}
				for(k = 0; k < _bonds.Size(); k++) {
					CBond2 &bond = _bonds[k];
					bond.InCoder -= (uint32)numEmptyMethods;
					bond.OutCoder -= (uint32)numEmptyMethods;
				}
				_methods.DeleteFrontal(numEmptyMethods);
			}
			FOR_VECTOR(k, _bonds) {
				const CBond2 &bond = _bonds[k];
				if(bond.InCoder >= (uint32)_methods.Size() || bond.OutCoder >= (uint32)_methods.Size())
					return E_INVALIDARG;
			}
			return S_OK;
			COM_TRY_END
		}
	}
}
