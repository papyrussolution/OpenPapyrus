// 7Z-ZIP.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
//
//#include <ProgressMt.h>
class CMtCompressProgressMixer {
public:
	void Init(int numItems, ICompressProgressInfo * progress);
	void Reinit(int index);
	HRESULT SetRatioInfo(int index, const uint64 * inSize, const uint64 * outSize);

	NWindows::NSynchronization::CCriticalSection CriticalSection;
private:
	CMyComPtr <ICompressProgressInfo> _progress;
	CRecordVector <uint64> InSizes;
	CRecordVector <uint64> OutSizes;
	uint64 TotalInSize;
	uint64 TotalOutSize;
};

class CMtCompressProgress : public ICompressProgressInfo, public CMyUnknownImp {
public:
	void Init(CMtCompressProgressMixer * progress, int index);
	void Reinit();
	MY_UNKNOWN_IMP
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
private:
	CMtCompressProgressMixer * _progress;
	int _index;
};
//
// ProgressMt.cpp
void CMtCompressProgressMixer::Init(int numItems, ICompressProgressInfo * progress)
{
	NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
	InSizes.Clear();
	OutSizes.Clear();
	for(int i = 0; i < numItems; i++) {
		InSizes.Add(0);
		OutSizes.Add(0);
	}
	TotalInSize = 0;
	TotalOutSize = 0;
	_progress = progress;
}

void CMtCompressProgressMixer::Reinit(int index)
{
	NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
	InSizes[index] = 0;
	OutSizes[index] = 0;
}

HRESULT CMtCompressProgressMixer::SetRatioInfo(int index, const uint64 * inSize, const uint64 * outSize)
{
	NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
	if(inSize != 0) {
		uint64 diff = *inSize - InSizes[index];
		InSizes[index] = *inSize;
		TotalInSize += diff;
	}
	if(outSize != 0) {
		uint64 diff = *outSize - OutSizes[index];
		OutSizes[index] = *outSize;
		TotalOutSize += diff;
	}
	return _progress ? _progress->SetRatioInfo(&TotalInSize, &TotalOutSize) : S_OK;
}

void CMtCompressProgress::Init(CMtCompressProgressMixer * progress, int index)
{
	_progress = progress;
	_index = index;
}

void CMtCompressProgress::Reinit() { _progress->Reinit(_index); }

STDMETHODIMP CMtCompressProgress::SetRatioInfo(const uint64 * inSize, const uint64 * outSize)
{
	return _progress->SetRatioInfo(_index, inSize, outSize);
}
//
//
//
#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(offs, v) v = Get16(p + (offs))
#define G32(offs, v) v = Get32(p + (offs))
#define G64(offs, v) v = Get64(p + (offs))

using namespace NWindows;
using namespace NCOM;
using namespace NTime;
using namespace NSynchronization;

class COffsetOutStream : public IOutStream, public CMyUnknownImp {
public:
	HRESULT Init(IOutStream * stream, uint64 offset);
	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	STDMETHOD(SetSize) (uint64 newSize);
private:
	uint64 _offset;
	CMyComPtr <IOutStream> _stream;
};
//

HRESULT COffsetOutStream::Init(IOutStream * stream, uint64 offset)
{
	_offset = offset;
	_stream = stream;
	return _stream->Seek(offset, STREAM_SEEK_SET, NULL);
}

STDMETHODIMP COffsetOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	return _stream->Write(data, size, processedSize);
}

STDMETHODIMP COffsetOutStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	uint64 absoluteNewPosition;
	if(seekOrigin == STREAM_SEEK_SET) {
		if(offset < 0)
			return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
		offset += _offset;
	}
	HRESULT result = _stream->Seek(offset, seekOrigin, &absoluteNewPosition);
	ASSIGN_PTR(newPosition, absoluteNewPosition - _offset);
	return result;
}

STDMETHODIMP COffsetOutStream::SetSize(uint64 newSize)
{
	return _stream->SetSize(_offset + newSize);
}
//
//
//
//#include <MemBlocks.h>
class CMemBlockManager {
public:
	CMemBlockManager(size_t blockSize = (1 << 20)) : _data(0), _blockSize(blockSize), _headFree(0) 
	{
	}
	~CMemBlockManager() 
	{
		FreeSpace();
	}
	bool   AllocateSpace(size_t numBlocks);
	void   FreeSpace();
	size_t GetBlockSize() const { return _blockSize; }
	void * AllocateBlock();
	void   FreeBlock(void * p);
private:
	void * _data;
	size_t _blockSize;
	void * _headFree;
};

class CMemBlockManagerMt : public CMemBlockManager {
public:
	CMemBlockManagerMt(size_t blockSize = (1 << 20)) : CMemBlockManager(blockSize) 
	{
	}
	~CMemBlockManagerMt() 
	{
		FreeSpace();
	}
	HRes   AllocateSpace(size_t numBlocks, size_t numNoLockBlocks = 0);
	HRes   AllocateSpaceAlways(size_t desiredNumberOfBlocks, size_t numNoLockBlocks = 0);
	void   FreeSpace();
	void * AllocateBlock();
	void   FreeBlock(void * p, bool lockMode = true);
	HRes   ReleaseLockedBlocks(int number);

	NWindows::NSynchronization::CSemaphore Semaphore;
private:
	NWindows::NSynchronization::CCriticalSection _criticalSection;
};

class CMemBlocks {
public:
	CMemBlocks() : TotalSize(0) 
	{
	}
	void   FreeOpt(CMemBlockManagerMt * manager);
	HRESULT WriteToStream(size_t blockSize, ISequentialOutStream * outStream) const;

	CRecordVector <void *> Blocks;
	uint64 TotalSize;
private:
	void Free(CMemBlockManagerMt * manager);
};

struct CMemLockBlocks : public CMemBlocks {
	CMemLockBlocks() : LockMode(true) 
	{
	}
	void   Free(CMemBlockManagerMt * memManager);
	void   FreeBlock(int index, CMemBlockManagerMt * memManager);
	HRes   SwitchToNoLockMode(CMemBlockManagerMt * memManager);
	void   Detach(CMemLockBlocks &blocks, CMemBlockManagerMt * memManager);

	bool   LockMode;
	uint8  Reserve[3]; // @alignment
};
//
// MemBlocks.cpp
bool CMemBlockManager::AllocateSpace(size_t numBlocks)
{
	FreeSpace();
	if(_blockSize < sizeof(void *) || numBlocks < 1)
		return false;
	else {
		size_t totalSize = numBlocks * _blockSize;
		if(totalSize / _blockSize != numBlocks)
			return false;
		else {
			_data = ::MidAlloc(totalSize);
			if(_data == 0)
				return false;
			else {
				Byte * p = (Byte *)_data;
				for(size_t i = 0; i + 1 < numBlocks; i++, p += _blockSize)
					*(Byte**)p = (p + _blockSize);
				*(Byte**)p = 0;
				_headFree = _data;
				return true;
			}
		}
	}
}

void CMemBlockManager::FreeSpace()
{
	::MidFree(_data);
	_data = 0;
	_headFree = 0;
}

void * CMemBlockManager::AllocateBlock()
{
	void * p = _headFree;
	if(p)
		_headFree = *(void **)_headFree;
	return p;
}

void CMemBlockManager::FreeBlock(void * p)
{
	if(p) {
		*(void **)p = _headFree;
		_headFree = p;
	}
}

HRes CMemBlockManagerMt::ReleaseLockedBlocks(int number) 
{
	return Semaphore.Release(number);
}

HRes CMemBlockManagerMt::AllocateSpace(size_t numBlocks, size_t numNoLockBlocks)
{
	if(numNoLockBlocks > numBlocks)
		return E_INVALIDARG;
	if(!CMemBlockManager::AllocateSpace(numBlocks))
		return E_OUTOFMEMORY;
	size_t numLockBlocks = numBlocks - numNoLockBlocks;
	Semaphore.Close();
	return Semaphore.Create((LONG)numLockBlocks, (LONG)numLockBlocks);
}

HRes CMemBlockManagerMt::AllocateSpaceAlways(size_t desiredNumberOfBlocks, size_t numNoLockBlocks)
{
	if(numNoLockBlocks > desiredNumberOfBlocks)
		return E_INVALIDARG;
	for(;; ) {
		if(AllocateSpace(desiredNumberOfBlocks, numNoLockBlocks) == 0)
			return 0;
		if(desiredNumberOfBlocks == numNoLockBlocks)
			return E_OUTOFMEMORY;
		desiredNumberOfBlocks = numNoLockBlocks + ((desiredNumberOfBlocks - numNoLockBlocks) >> 1);
	}
}

void CMemBlockManagerMt::FreeSpace()
{
	Semaphore.Close();
	CMemBlockManager::FreeSpace();
}

void * CMemBlockManagerMt::AllocateBlock()
{
	// Semaphore.Lock();
	NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
	return CMemBlockManager::AllocateBlock();
}

void CMemBlockManagerMt::FreeBlock(void * p, bool lockMode)
{
	if(p) {
		{
			NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
			CMemBlockManager::FreeBlock(p);
		}
		if(lockMode)
			Semaphore.Release();
	}
}

void CMemBlocks::Free(CMemBlockManagerMt * manager)
{
	while(Blocks.Size() > 0) {
		manager->FreeBlock(Blocks.Back());
		Blocks.DeleteBack();
	}
	TotalSize = 0;
}

void CMemBlocks::FreeOpt(CMemBlockManagerMt * manager)
{
	Free(manager);
	Blocks.ClearAndFree();
}

HRESULT CMemBlocks::WriteToStream(size_t blockSize, ISequentialOutStream * outStream) const
{
	uint64 totalSize = TotalSize;
	for(uint blockIndex = 0; totalSize > 0; blockIndex++) {
		uint32 curSize = (uint32)blockSize;
		if(totalSize < curSize)
			curSize = (uint32)totalSize;
		if(blockIndex >= Blocks.Size())
			return E_FAIL;
		RINOK(WriteStream(outStream, Blocks[blockIndex], curSize));
		totalSize -= curSize;
	}
	return S_OK;
}

void CMemLockBlocks::FreeBlock(int index, CMemBlockManagerMt * memManager)
{
	memManager->FreeBlock(Blocks[index], LockMode);
	Blocks[index] = 0;
}

void CMemLockBlocks::Free(CMemBlockManagerMt * memManager)
{
	while(Blocks.Size() > 0) {
		FreeBlock(Blocks.Size() - 1, memManager);
		Blocks.DeleteBack();
	}
	TotalSize = 0;
}

HRes CMemLockBlocks::SwitchToNoLockMode(CMemBlockManagerMt * memManager)
{
	if(LockMode) {
		if(Blocks.Size() > 0) {
			RINOK(memManager->ReleaseLockedBlocks(Blocks.Size()));
		}
		LockMode = false;
	}
	return 0;
}

void CMemLockBlocks::Detach(CMemLockBlocks &blocks, CMemBlockManagerMt * memManager)
{
	blocks.Free(memManager);
	blocks.LockMode = LockMode;
	uint64 totalSize = 0;
	size_t blockSize = memManager->GetBlockSize();
	FOR_VECTOR(i, Blocks)
	{
		if(totalSize < TotalSize)
			blocks.Blocks.Add(Blocks[i]);
		else
			FreeBlock(i, memManager);
		Blocks[i] = 0;
		totalSize += blockSize;
	}
	blocks.TotalSize = TotalSize;
	Free(memManager);
}
//
class COutMemStream : public IOutStream, public CMyUnknownImp {
public:
	HRes CreateEvents();
	void SetOutStream(IOutStream * outStream);
	void SetSeqOutStream(ISequentialOutStream * outStream);
	void ReleaseOutStream();
	COutMemStream(CMemBlockManagerMt * memManager);
	~COutMemStream(); 
	void Free();
	void Init();
	HRESULT WriteToRealStream();
	void DetachData(CMemLockBlocks & blocks);
	bool WasUnlockEventSent() const { return _unlockEventWasSent; }
	void SetRealStreamMode();
	//void SetNoLockMode();
	void StopWriting(HRESULT res);
	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	STDMETHOD(SetSize) (uint64 newSize);
private:
	uint64 GetPos() const { return (uint64)_curBlockIndex * _memManager->GetBlockSize() + _curBlockPos; }

	CMemBlockManagerMt * _memManager;
	uint   _curBlockIndex;
	size_t _curBlockPos;
	bool   _realStreamMode;
	bool   _unlockEventWasSent;
	NWindows::NSynchronization::CAutoResetEvent StopWritingEvent;
	NWindows::NSynchronization::CAutoResetEvent WriteToRealStreamEvent;
	// NWindows::NSynchronization::CAutoResetEvent NoLockEvent;
	HRESULT StopWriteResult;
	CMemLockBlocks Blocks;
	CMyComPtr <ISequentialOutStream> OutSeqStream;
	CMyComPtr <IOutStream> OutStream;
};
//
// OutMemStream.cpp
COutMemStream::COutMemStream(CMemBlockManagerMt * memManager) : _memManager(memManager)  
{
}

COutMemStream::~COutMemStream() 
{
	Free();
}

void COutMemStream::SetRealStreamMode()
{
	_unlockEventWasSent = true;
	WriteToRealStreamEvent.Set();
}

/*void COutMemStream::SetNoLockMode()
{
	_unlockEventWasSent = true;
	NoLockEvent.Set();
}*/

void COutMemStream::StopWriting(HRESULT res)
{
	StopWriteResult = res;
	StopWritingEvent.Set();
}

HRes COutMemStream::CreateEvents()
{
	RINOK(StopWritingEvent.CreateIfNotCreated());
	return WriteToRealStreamEvent.CreateIfNotCreated();
}

void COutMemStream::SetOutStream(IOutStream * outStream)
{
	OutStream = outStream;
	OutSeqStream = outStream;
}
void COutMemStream::SetSeqOutStream(ISequentialOutStream * outStream)
{
	OutStream = NULL;
	OutSeqStream = outStream;
}
void COutMemStream::ReleaseOutStream()
{
	OutStream.Release();
	OutSeqStream.Release();
}

void COutMemStream::Free()
{
	Blocks.Free(_memManager);
	Blocks.LockMode = true;
}

void COutMemStream::Init()
{
	WriteToRealStreamEvent.Reset();
	_unlockEventWasSent = false;
	_realStreamMode = false;
	Free();
	_curBlockPos = 0;
	_curBlockIndex = 0;
}

void COutMemStream::DetachData(CMemLockBlocks &blocks)
{
	Blocks.Detach(blocks, _memManager);
	Free();
}

HRESULT COutMemStream::WriteToRealStream()
{
	RINOK(Blocks.WriteToStream(_memManager->GetBlockSize(), OutSeqStream));
	Blocks.Free(_memManager);
	return S_OK;
}

STDMETHODIMP COutMemStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	if(_realStreamMode)
		return OutSeqStream->Write(data, size, processedSize);
	ASSIGN_PTR(processedSize, 0);
	while(size != 0) {
		if(_curBlockIndex < Blocks.Blocks.Size()) {
			Byte * p = (Byte *)Blocks.Blocks[_curBlockIndex] + _curBlockPos;
			size_t curSize = _memManager->GetBlockSize() - _curBlockPos;
			SETMIN(curSize, size);
			memcpy(p, data, curSize);
			if(processedSize != 0)
				*processedSize += (uint32)curSize;
			data = (const void *)((const Byte*)data + curSize);
			size -= (uint32)curSize;
			_curBlockPos += curSize;
			uint64 pos64 = GetPos();
			if(pos64 > Blocks.TotalSize)
				Blocks.TotalSize = pos64;
			if(_curBlockPos == _memManager->GetBlockSize()) {
				_curBlockIndex++;
				_curBlockPos = 0;
			}
			continue;
		}
		HANDLE events[3] = { StopWritingEvent, WriteToRealStreamEvent, /* NoLockEvent, */ _memManager->Semaphore };
		DWORD waitResult = ::WaitForMultipleObjects((Blocks.LockMode ? 3 : 2), events, FALSE, INFINITE);
		switch(waitResult) {
			case (WAIT_OBJECT_0 + 0): return StopWriteResult;
			case (WAIT_OBJECT_0 + 1):
		    {
			    _realStreamMode = true;
			    RINOK(WriteToRealStream());
			    uint32 processedSize2;
			    HRESULT res = OutSeqStream->Write(data, size, &processedSize2);
			    if(processedSize != 0)
				    *processedSize += processedSize2;
			    return res;
		    }
			/*
			   case (WAIT_OBJECT_0 + 2):
			   {
			   // it has bug: no write.
			   if(!Blocks.SwitchToNoLockMode(_memManager))
			    return E_FAIL;
			   break;
			   }
			 */
			case (WAIT_OBJECT_0 + 2):
			    break;
			default:
			    return E_FAIL;
		}
		Blocks.Blocks.Add(_memManager->AllocateBlock());
		if(Blocks.Blocks.Back() == 0)
			return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP COutMemStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(_realStreamMode) {
		return OutStream ? OutStream->Seek(offset, seekOrigin, newPosition) : E_FAIL;
	}
	else {
		if(seekOrigin == STREAM_SEEK_CUR) {
			if(offset != 0)
				return E_NOTIMPL;
		}
		else if(seekOrigin == STREAM_SEEK_SET) {
			if(offset != 0)
				return E_NOTIMPL;
			_curBlockIndex = 0;
			_curBlockPos = 0;
		}
		else
			return E_NOTIMPL;
		ASSIGN_PTR(newPosition, GetPos());
		return S_OK;
	}
}

STDMETHODIMP COutMemStream::SetSize(uint64 newSize)
{
	if(_realStreamMode)
		return OutStream ? OutStream->SetSize(newSize) : E_FAIL;
	else {
		Blocks.TotalSize = newSize;
		return S_OK;
	}
}
//
// PpmdZip.cpp ShrinkDecoder.cpp ImplodeDecoder.cpp
namespace NCompress {
	namespace NPpmdZip {
		static const uint32 kBufSize = (1 << 20);

		struct CBuf {
			CBuf() : Buf(0) 
			{
			}
			~CBuf() 
			{
				::MidFree(Buf);
			}
			bool Alloc()
			{
				if(!Buf)
					Buf = static_cast<Byte *>(::MidAlloc(kBufSize));
				return (Buf != 0);
			}
			Byte * Buf;
		};

		class CDecoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
			CByteInBufWrap _inStream;
			CBuf _outStream;
			CPpmd8 _ppmd;
			bool _fullFileMode;
		public:
			MY_UNKNOWN_IMP2(ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);

			CDecoder(bool fullFileMode);
			~CDecoder();
		};

		struct CEncProps {
			CEncProps();
			void   Normalize(int level);
			uint32 MemSizeMB;
			uint32 ReduceSize;
			int    Order;
			int    Restor;
		};

		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties, public CMyUnknownImp {
			CByteOutBufWrap _outStream;
			CBuf _inStream;
			CPpmd8 _ppmd;
			CEncProps _props;
		public:
			MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream,
						const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			CEncoder();
			~CEncoder();
		};

		CDecoder::CDecoder(bool fullFileMode) : _fullFileMode(fullFileMode)
		{
			_ppmd.Stream.In = &_inStream.vt;
			Ppmd8_Construct(&_ppmd);
		}
		CDecoder::~CDecoder()
		{
			Ppmd8_Free(&_ppmd, &g_BigAlloc);
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			if(!_outStream.Alloc())
				return E_OUTOFMEMORY;
			if(!_inStream.Alloc(1 << 20))
				return E_OUTOFMEMORY;
			_inStream.Stream = inStream;
			_inStream.Init();
			{
				Byte buf[2];
				for(int i = 0; i < 2; i++)
					buf[i] = _inStream.ReadByte();
				if(_inStream.Extra)
					return S_FALSE;
				uint32 val = GetUi16(buf);
				uint32 order = (val & 0xF) + 1;
				uint32 mem = ((val >> 4) & 0xFF) + 1;
				uint32 restor = (val >> 12);
				if(order < 2 || restor > 2)
					return S_FALSE;
			#ifndef PPMD8_FREEZE_SUPPORT
				if(restor == 2)
					return E_NOTIMPL;
			#endif
				if(!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc))
					return E_OUTOFMEMORY;
				if(!Ppmd8_RangeDec_Init(&_ppmd))
					return S_FALSE;
				Ppmd8_Init(&_ppmd, order, restor);
			}
			bool wasFinished = false;
			uint64 processedSize = 0;
			for(;; ) {
				size_t size = kBufSize;
				if(outSize) {
					const uint64 rem = *outSize - processedSize;
					if(size > rem) {
						size = (size_t)rem;
						if(size == 0)
							break;
					}
				}
				Byte * data = _outStream.Buf;
				size_t i = 0;
				int sym = 0;
				do {
					sym = Ppmd8_DecodeSymbol(&_ppmd);
					if(_inStream.Extra || sym < 0)
						break;
					data[i] = (Byte)sym;
				} while(++i != size);
				processedSize += i;
				RINOK(WriteStream(outStream, _outStream.Buf, i));
				RINOK(_inStream.Res);
				if(_inStream.Extra)
					return S_FALSE;
				if(sym < 0) {
					if(sym != -1)
						return S_FALSE;
					wasFinished = true;
					break;
				}
				if(progress) {
					const uint64 inProccessed = _inStream.GetProcessed();
					RINOK(progress->SetRatioInfo(&inProccessed, &processedSize));
				}
			}
			RINOK(_inStream.Res);
			if(_fullFileMode) {
				if(!wasFinished) {
					int res = Ppmd8_DecodeSymbol(&_ppmd);
					RINOK(_inStream.Res);
					if(_inStream.Extra || res != -1)
						return S_FALSE;
				}
				if(!Ppmd8_RangeDec_IsFinishedOK(&_ppmd))
					return S_FALSE;
				if(inSize && *inSize != _inStream.GetProcessed())
					return S_FALSE;
			}
			return S_OK;
		}
		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			_fullFileMode = (finishMode != 0);
			return S_OK;
		}
		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _inStream.GetProcessed();
			return S_OK;
		}
		//
		// Encoder
		//
		CEncProps::CEncProps() : MemSizeMB((uint32)(int32)-1), ReduceSize((uint32)(int32)-1), Order(-1), Restor(-1)
		{
		}

		void CEncProps::Normalize(int level)
		{
			if(level < 0) level = 5;
			if(level == 0) level = 1;
			if(level > 9) level = 9;
			if(MemSizeMB == (uint32)(int32)-1)
				MemSizeMB = (1 << ((level > 8 ? 8 : level) - 1));
			const uint kMult = 16;
			if((MemSizeMB << 20) / kMult > ReduceSize) {
				for(uint32 m = (1 << 20); m <= (1 << 28); m <<= 1) {
					if(ReduceSize <= m / kMult) {
						m >>= 20;
						if(MemSizeMB > m)
							MemSizeMB = m;
						break;
					}
				}
			}
			if(Order == -1) 
				Order = 3 + level;
			if(Restor == -1)
				Restor = level < 7 ? PPMD8_RESTORE_METHOD_RESTART : PPMD8_RESTORE_METHOD_CUT_OFF;
		}

		CEncoder::~CEncoder()
		{
			Ppmd8_Free(&_ppmd, &g_BigAlloc);
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
						if(v < (1 << 20) || v > (1 << 28))
							return E_INVALIDARG;
						props.MemSizeMB = v >> 20;
						break;
					case NCoderPropID::kOrder:
						if(v < PPMD8_MIN_ORDER || v > PPMD8_MAX_ORDER)
							return E_INVALIDARG;
						props.Order = (Byte)v;
						break;
					case NCoderPropID::kNumThreads: break;
					case NCoderPropID::kLevel: level = (int)v; break;
					case NCoderPropID::kAlgorithm:
						if(v > 1)
							return E_INVALIDARG;
						props.Restor = v;
						break;
					default: return E_INVALIDARG;
				}
			}
			props.Normalize(level);
			_props = props;
			return S_OK;
		}

		CEncoder::CEncoder()
		{
			_props.Normalize(-1);
			_ppmd.Stream.Out = &_outStream.vt;
			Ppmd8_Construct(&_ppmd);
		}

		STDMETHODIMP CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			if(!_inStream.Alloc())
				return E_OUTOFMEMORY;
			if(!_outStream.Alloc(1 << 20))
				return E_OUTOFMEMORY;
			if(!Ppmd8_Alloc(&_ppmd, _props.MemSizeMB << 20, &g_BigAlloc))
				return E_OUTOFMEMORY;

			_outStream.Stream = outStream;
			_outStream.Init();

			Ppmd8_RangeEnc_Init(&_ppmd);
			Ppmd8_Init(&_ppmd, _props.Order, _props.Restor);

			uint32 val = (uint32)((_props.Order - 1) + ((_props.MemSizeMB - 1) << 4) + (_props.Restor << 12));
			_outStream.WriteByte((Byte)(val & 0xFF));
			_outStream.WriteByte((Byte)(val >> 8));
			RINOK(_outStream.Res);

			uint64 processed = 0;
			for(;; ) {
				uint32 size;
				RINOK(inStream->Read(_inStream.Buf, kBufSize, &size));
				if(size == 0) {
					Ppmd8_EncodeSymbol(&_ppmd, -1);
					Ppmd8_RangeEnc_FlushData(&_ppmd);
					return _outStream.Flush();
				}
				for(uint32 i = 0; i < size; i++) {
					Ppmd8_EncodeSymbol(&_ppmd, _inStream.Buf[i]);
					RINOK(_outStream.Res);
				}
				processed += size;
				if(progress) {
					const uint64 outProccessed = _outStream.GetProcessed();
					RINOK(progress->SetRatioInfo(&processed, &outProccessed));
				}
			}
		}
	}
	namespace NShrink {
		const uint kNumMaxBits = 13;
		const uint kNumItems = 1 << kNumMaxBits;

		static const uint32 kBufferSize = (1 << 18);
		static const uint kNumMinBits = 9;

		class CDecoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
			uint64 _inProcessed;
			bool _fullStreamMode;
			uint16 _parents[kNumItems];
			Byte _suffixes[kNumItems];
			Byte _stack[kNumItems];
			HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
		public:
			MY_UNKNOWN_IMP2(ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
		};

		HRESULT CDecoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			NBitl::CBaseDecoder<CInBuffer> inBuffer;
			COutBuffer outBuffer;
			if(!inBuffer.Create(kBufferSize))
				return E_OUTOFMEMORY;
			if(!outBuffer.Create(kBufferSize))
				return E_OUTOFMEMORY;
			inBuffer.SetStream(inStream);
			inBuffer.Init();
			outBuffer.SetStream(outStream);
			outBuffer.Init();
			{
				uint   i;
				for(i = 0; i < 257; i++)
					_parents[i] = (uint16)i;
				for(; i < kNumItems; i++)
					_parents[i] = kNumItems;
				for(i = 0; i < kNumItems; i++)
					_suffixes[i] = 0;
			}
			uint64 prevPos = 0, inPrev = 0;
			uint   numBits = kNumMinBits;
			uint   head = 257;
			int lastSym = -1;
			Byte lastChar2 = 0;
			bool moreOut = false;
			HRESULT res = S_FALSE;
			for(;; ) {
				_inProcessed = inBuffer.GetProcessedSize();
				const uint64 nowPos = outBuffer.GetProcessedSize();
				bool eofCheck = false;
				if(outSize && nowPos >= *outSize) {
					if(!_fullStreamMode || moreOut) {
						res = S_OK;
						break;
					}
					eofCheck = true;
					// Is specSym(=256) allowed after end of stream
					// Do we need to read it here
				}
				if(progress) {
					if(nowPos - prevPos >= (1 << 18) || _inProcessed - inPrev >= (1 << 20)) {
						prevPos = nowPos;
						inPrev = _inProcessed;
						RINOK(progress->SetRatioInfo(&_inProcessed, &nowPos));
					}
				}
				uint32 sym = inBuffer.ReadBits(numBits);
				if(inBuffer.ExtraBitsWereRead()) {
					res = S_OK;
					break;
				}
				if(sym == 256) {
					sym = inBuffer.ReadBits(numBits);
					if(inBuffer.ExtraBitsWereRead())
						break;
					if(sym == 1) {
						if(numBits >= kNumMaxBits)
							break;
						numBits++;
						continue;
					}
					if(sym != 2)
						break;
					{
						uint   i;
						for(i = 257; i < kNumItems; i++)
							_stack[i] = 0;
						for(i = 257; i < kNumItems; i++) {
							uint   par = _parents[i];
							if(par != kNumItems)
								_stack[par] = 1;
						}
						for(i = 257; i < kNumItems; i++)
							if(_stack[i] == 0)
								_parents[i] = kNumItems;
						head = 257;
						continue;
					}
				}
				if(eofCheck) {
					// It's can be error case.
					// That error can be detected later in (*inSize != _inProcessed) check.
					res = S_OK;
					break;
				}
				bool needPrev = false;
				if(head < kNumItems && lastSym >= 0) {
					while(head < kNumItems && _parents[head] != kNumItems)
						head++;
					if(head < kNumItems) {
						if(head == (uint)lastSym) {
							// we need to fix the code for that case
							// _parents[head] is not allowed to link to itself
							res = E_NOTIMPL;
							break;
						}
						needPrev = true;
						_parents[head] = (uint16)lastSym;
						_suffixes[head] = (Byte)lastChar2;
						head++;
					}
				}
				if(_parents[sym] == kNumItems)
					break;
				lastSym = sym;
				uint   cur = sym;
				uint   i = 0;
				while(cur >= 256) {
					_stack[i++] = _suffixes[cur];
					cur = _parents[cur];
				}
				_stack[i++] = (Byte)cur;
				lastChar2 = (Byte)cur;
				if(needPrev)
					_suffixes[(size_t)head - 1] = (Byte)cur;
				if(outSize) {
					const uint64 limit = *outSize - nowPos;
					if(i > limit) {
						moreOut = true;
						i = (uint)limit;
					}
				}
				do {
					outBuffer.WriteByte(_stack[--i]);
				} while(i);
			}
			RINOK(outBuffer.Flush());
			if(res == S_OK)
				if(_fullStreamMode) {
					if(moreOut)
						res = S_FALSE;
					const uint64 nowPos = outBuffer.GetProcessedSize();
					if(outSize && *outSize != nowPos)
						res = S_FALSE;
					if(inSize && *inSize != _inProcessed)
						res = S_FALSE;
				}

			return res;
		}
		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
			// catch(const CInBufferException &e) { return e.ErrorCode; }
			// catch(const COutBufferException &e) { return e.ErrorCode; }
			catch(const CSystemException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}
		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			_fullStreamMode = (finishMode != 0);
			return S_OK;
		}
		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = _inProcessed;
			return S_OK;
		}
	}
	namespace NImplode {
		namespace NDecoder {
			typedef NBitl::CDecoder <CInBuffer> CInBit;

			const uint kNumHuffmanBits = 16;
			const uint kMaxHuffTableSize = 1 << 8;

			class CHuffmanDecoder {
			public:
				bool   Build(const Byte * lens, unsigned numSymbols) throw();
				uint32 Decode(CInBit * inStream) const throw();
			private:
				uint32 _limits[kNumHuffmanBits + 1];
				uint32 _poses[kNumHuffmanBits + 1];
				Byte   _symbols[kMaxHuffTableSize];
			};

			class CCoder : public ICompressCoder, public ICompressSetDecoderProperties2, public ICompressSetFinishMode,
				public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
				CLzOutWindow _outWindowStream;
				CInBit _inBitStream;
				CHuffmanDecoder _litDecoder;
				CHuffmanDecoder _lenDecoder;
				CHuffmanDecoder _distDecoder;
				Byte _flags;
				bool _fullStreamMode;

				bool BuildHuff(CHuffmanDecoder &table, unsigned numSymbols);
				HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
			public:
				MY_UNKNOWN_IMP3(ICompressSetDecoderProperties2, ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
				STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
				STDMETHOD(SetDecoderProperties2)(const Byte *data, uint32 size);
				STDMETHOD(SetFinishMode)(uint32 finishMode);
				STDMETHOD(GetInStreamProcessedSize)(uint64 *value);

				CCoder();
			};

			bool CHuffmanDecoder::Build(const Byte * lens, unsigned numSymbols) throw()
			{
				uint   counts[kNumHuffmanBits + 1];
				uint   i;
				for(i = 0; i <= kNumHuffmanBits; i++)
					counts[i] = 0;
				uint   sym;
				for(sym = 0; sym < numSymbols; sym++)
					counts[lens[sym]]++;
				const uint32 kMaxValue = (uint32)1 << kNumHuffmanBits;
				// _limits[0] = kMaxValue;
				uint32 startPos = kMaxValue;
				uint32 sum = 0;
				for(i = 1; i <= kNumHuffmanBits; i++) {
					const uint32 cnt = counts[i];
					const uint32 range = cnt << (kNumHuffmanBits - i);
					if(startPos < range)
						return false;
					startPos -= range;
					_limits[i] = startPos;
					_poses[i] = sum;
					sum += cnt;
					counts[i] = sum;
				}
				// counts[0] += sum;
				if(startPos != 0)
					return false;
				else {
					for(sym = 0; sym < numSymbols; sym++) {
						uint   len = lens[sym];
						if(len != 0)
							_symbols[--counts[len]] = (Byte)sym;
					}
					return true;
				}
			}

			uint32 CHuffmanDecoder::Decode(CInBit * inStream) const throw()
			{
				uint32 val = inStream->GetValue(kNumHuffmanBits);
				uint   numBits;
				for(numBits = 1; val < _limits[numBits]; numBits++) 
					;
				uint32 sym = _symbols[_poses[numBits] + ((val - _limits[numBits]) >> (kNumHuffmanBits - numBits))];
				inStream->MovePos(numBits);
				return sym;
			}

			static const uint kNumLenDirectBits = 8;
			static const uint kNumDistDirectBitsSmall = 6;
			static const uint kNumDistDirectBitsBig = 7;
			static const uint kLitTableSize = (1 << 8);
			static const uint kDistTableSize = 64;
			static const uint kLenTableSize = 64;
			static const uint32 kHistorySize = (1 << kNumDistDirectBitsBig) * kDistTableSize; // 8 KB

			CCoder::CCoder() : _fullStreamMode(false), _flags(0)
			{
			}

			bool CCoder::BuildHuff(CHuffmanDecoder &decoder, unsigned numSymbols)
			{
				Byte levels[kMaxHuffTableSize];
				uint   numRecords = (uint)_inBitStream.ReadAlignedByte() + 1;
				uint   index = 0;
				do {
					uint   b = (uint)_inBitStream.ReadAlignedByte();
					Byte level = (Byte)((b & 0xF) + 1);
					uint   rep = ((uint)b >> 4) + 1;
					if(index + rep > numSymbols)
						return false;
					for(uint j = 0; j < rep; j++)
						levels[index++] = level;
				} while(--numRecords);
				return (index != numSymbols) ? false : decoder.Build(levels, numSymbols);
			}
			HRESULT CCoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				if(!_inBitStream.Create(1 << 18))
					return E_OUTOFMEMORY;
				if(!_outWindowStream.Create(kHistorySize << 1)) // 16 KB
					return E_OUTOFMEMORY;
				if(!outSize)
					return E_INVALIDARG;
				_outWindowStream.SetStream(outStream);
				_outWindowStream.Init(false);
				_inBitStream.SetStream(inStream);
				_inBitStream.Init();
				const uint numDistDirectBits = (_flags & 2) ? kNumDistDirectBitsBig : kNumDistDirectBitsSmall;
				const bool literalsOn = ((_flags & 4) != 0);
				const uint32 minMatchLen = (literalsOn ? 3 : 2);
				if(literalsOn)
					if(!BuildHuff(_litDecoder, kLitTableSize))
						return S_FALSE;
				if(!BuildHuff(_lenDecoder, kLenTableSize))
					return S_FALSE;
				if(!BuildHuff(_distDecoder, kDistTableSize))
					return S_FALSE;
				uint64 prevProgress = 0;
				bool moreOut = false;
				uint64 pos = 0, unPackSize = *outSize;
				while(pos < unPackSize) {
					if(progress && (pos - prevProgress) >= (1 << 18)) {
						const uint64 packSize = _inBitStream.GetProcessedSize();
						RINOK(progress->SetRatioInfo(&packSize, &pos));
						prevProgress = pos;
					}
					if(_inBitStream.ReadBits(1) != 0) {
						Byte b;
						if(literalsOn) {
							uint32 sym = _litDecoder.Decode(&_inBitStream);
							// if(sym >= kLitTableSize) break;
							b = (Byte)sym;
						}
						else
							b = (Byte)_inBitStream.ReadBits(8);
						_outWindowStream.PutByte(b);
						pos++;
					}
					else {
						uint32 lowDistBits = _inBitStream.ReadBits(numDistDirectBits);
						uint32 dist = _distDecoder.Decode(&_inBitStream);
						// if(dist >= kDistTableSize) break;
						dist = (dist << numDistDirectBits) + lowDistBits;
						uint32 len = _lenDecoder.Decode(&_inBitStream);
						// if(len >= kLenTableSize) break;
						if(len == kLenTableSize - 1)
							len += _inBitStream.ReadBits(kNumLenDirectBits);
						len += minMatchLen;
						{
							const uint64 limit = unPackSize - pos;
							if(len > limit) {
								moreOut = true;
								len = (uint32)limit;
							}
						}
						while(dist >= pos && len != 0) {
							_outWindowStream.PutByte(0);
							pos++;
							len--;
						}
						if(len != 0) {
							_outWindowStream.CopyBlock(dist, len);
							pos += len;
						}
					}
				}
				HRESULT res = _outWindowStream.Flush();
				if(res == S_OK) {
					if(_fullStreamMode) {
						if(moreOut)
							res = S_FALSE;
						if(inSize && *inSize != _inBitStream.GetProcessedSize())
							res = S_FALSE;
					}
					if(pos != unPackSize)
						res = S_FALSE;
				}
				return res;
			}
			STDMETHODIMP CCoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				try { return CodeReal(inStream, outStream, inSize, outSize, progress);  }
				// catch(const CInBufferException &e)  { return e.ErrorCode; }
				// catch(const CLzOutWindowException &e) { return e.ErrorCode; }
				catch(const CSystemException &e) { return e.ErrorCode; }
				catch(...) { return S_FALSE; }
			}
			STDMETHODIMP CCoder::SetDecoderProperties2(const Byte * data, uint32 size)
			{
				if(size == 0)
					return E_NOTIMPL;
				else {
					_flags = data[0];
					return S_OK;
				}
			}
			STDMETHODIMP CCoder::SetFinishMode(uint32 finishMode)
			{
				_fullStreamMode = (finishMode != 0);
				return S_OK;
			}
			STDMETHODIMP CCoder::GetInStreamProcessedSize(uint64 * value)
			{
				*value = _inBitStream.GetProcessedSize();
				return S_OK;
			}
		}
	}
}
//
// ZipItem.cpp ZipAddCommon.cpp ZipIn.cpp ZipOut.cpp ZipHandler.cpp ZipHandlerOut.cpp ZipUpdate.cpp
namespace NArchive {
	namespace NZip {
		using namespace NFileHeader;

		static const CUInt32PCharPair g_ExtraTypes[] = {
			{ NExtraID::kZip64, "Zip64" },
			{ NExtraID::kNTFS, "NTFS" },
			{ NExtraID::kStrongEncrypt, "StrongCrypto" },
			{ NExtraID::kUnixTime, "UT" },
			{ NExtraID::kUnixExtra, "UX" },
			{ NExtraID::kIzUnicodeComment, "uc" },
			{ NExtraID::kIzUnicodeName, "up" },
			{ NExtraID::kWzAES, "WzAES" }
		};

		bool FASTCALL CStrongCryptoExtra::ParseFromSubBlock(const CExtraSubBlock & sb)
		{
			if(sb.ID != NFileHeader::NExtraID::kStrongEncrypt)
				return false;
			else {
				const Byte * p = (const Byte*)sb.Data;
				if(sb.Data.Size() < 8)
					return false;
				else {
					Format = GetUi16(p + 0);
					AlgId  = GetUi16(p + 2);
					BitLen = GetUi16(p + 4);
					Flags  = GetUi16(p + 6);
					return (Format == 2);
				}
			}
		}

		bool CStrongCryptoExtra::CertificateIsUsed() const { return (Flags > 0x0001); }

		void CExtraSubBlock::PrintInfo(AString &s) const
		{
			for(uint i = 0; i < ARRAY_SIZE(g_ExtraTypes); i++) {
				const CUInt32PCharPair &pair = g_ExtraTypes[i];
				if(pair.Value == ID) {
					s += pair.Name;
					return;
				}
			}
			{
				char sz[32];
				sz[0] = '0';
				sz[1] = 'x';
				ConvertUInt32ToHex(ID, sz + 2);
				s += sz;
			}
		}

		CExtraBlock::CExtraBlock() : Error(false), MinorError(false), IsZip64(false), IsZip64_Error(false) 
		{
		}
		void CExtraBlock::Clear()
		{
			SubBlocks.Clear();
			IsZip64 = false;
		}
		size_t CExtraBlock::GetSize() const
		{
			size_t res = 0;
			FOR_VECTOR(i, SubBlocks) {
				res += SubBlocks[i].Data.Size() + 2 + 2;
			}
			return res;
		}
		bool FASTCALL CExtraBlock::GetWzAes(CWzAesExtra & e) const
		{
			FOR_VECTOR(i, SubBlocks) {
				if(e.ParseFromSubBlock(SubBlocks[i]))
					return true;
			}
			return false;
		}

		bool CExtraBlock::HasWzAes() const
		{
			CWzAesExtra e;
			return GetWzAes(e);
		}

		bool FASTCALL CExtraBlock::GetStrongCrypto(CStrongCryptoExtra & e) const
		{
			FOR_VECTOR(i, SubBlocks) {
				if(e.ParseFromSubBlock(SubBlocks[i]))
					return true;
			}
			return false;
		}

		void CExtraBlock::PrintInfo(AString &s) const
		{
			if(Error)
				s.Add_OptSpaced("Extra_ERROR");
			if(MinorError)
				s.Add_OptSpaced("Minor_Extra_ERROR");
			if(IsZip64 || IsZip64_Error) {
				s.Add_OptSpaced("Zip64");
				if(IsZip64_Error)
					s += "_ERROR";
			}
			FOR_VECTOR(i, SubBlocks) {
				s.Add_Space_if_NotEmpty();
				SubBlocks[i].PrintInfo(s);
			}
		}

		void CExtraBlock::RemoveUnknownSubBlocks()
		{
			for(uint i = SubBlocks.Size(); i != 0; ) {
				i--;
				if(SubBlocks[i].ID != NFileHeader::NExtraID::kWzAES)
					SubBlocks.Delete(i);
			}
		}

		bool CExtraSubBlock::ExtractNtfsTime(uint index, FILETIME &ft) const
		{
			ft.dwHighDateTime = ft.dwLowDateTime = 0;
			uint32 size = (uint32)Data.Size();
			if(ID != NExtraID::kNTFS || size < 32)
				return false;
			const Byte * p = (const Byte*)Data;
			p += 4; // for reserved
			size -= 4;
			while(size > 4) {
				uint16 tag = GetUi16(p);
				unsigned attrSize = GetUi16(p + 2);
				p += 4;
				size -= 4;
				if(attrSize > size)
					attrSize = size;

				if(tag == NNtfsExtra::kTagTime && attrSize >= 24) {
					p += 8 * index;
					ft.dwLowDateTime = GetUi32(p);
					ft.dwHighDateTime = GetUi32(p + 4);
					return true;
				}
				p += attrSize;
				size -= attrSize;
			}
			return false;
		}

		bool CExtraSubBlock::ExtractUnixTime(bool isCentral, unsigned index, uint32 &res) const
		{
			res = 0;
			uint32 size = (uint32)Data.Size();
			if(ID != NExtraID::kUnixTime || size < 5)
				return false;
			const Byte * p = (const Byte*)Data;
			Byte flags = *p++;
			size--;
			if(isCentral) {
				if(index != NUnixTime::kMTime ||
							(flags & (1 << NUnixTime::kMTime)) == 0 ||
							size < 4)
					return false;
				res = GetUi32(p);
				return true;
			}
			for(uint i = 0; i < 3; i++)
				if((flags & (1 << i)) != 0) {
					if(size < 4)
						return false;
					if(index == i) {
						res = GetUi32(p);
						return true;
					}
					p += 4;
					size -= 4;
				}
			return false;
		}

		bool CExtraSubBlock::ExtractUnixExtraTime(uint index, uint32 &res) const
		{
			res = 0;
			const size_t size = Data.Size();
			unsigned offset = index * 4;
			if(ID != NExtraID::kUnixExtra || size < offset + 4)
				return false;
			const Byte * p = (const Byte*)Data + offset;
			res = GetUi32(p);
			return true;
		}

		bool CExtraBlock::GetNtfsTime(uint index, FILETIME &ft) const
		{
			FOR_VECTOR(i, SubBlocks) {
				const CExtraSubBlock &sb = SubBlocks[i];
				if(sb.ID == NFileHeader::NExtraID::kNTFS)
					return sb.ExtractNtfsTime(index, ft);
			}
			return false;
		}

		bool CExtraBlock::GetUnixTime(bool isCentral, unsigned index, uint32 &res) const
		{
			{
				FOR_VECTOR(i, SubBlocks) {
					const CExtraSubBlock &sb = SubBlocks[i];
					if(sb.ID == NFileHeader::NExtraID::kUnixTime)
						return sb.ExtractUnixTime(isCentral, index, res);
				}
			}
			switch(index) {
				case NUnixTime::kMTime: index = NUnixExtra::kMTime; break;
				case NUnixTime::kATime: index = NUnixExtra::kATime; break;
				default: return false;
			}
			{
				FOR_VECTOR(i, SubBlocks) {
					const CExtraSubBlock &sb = SubBlocks[i];
					if(sb.ID == NFileHeader::NExtraID::kUnixExtra)
						return sb.ExtractUnixExtraTime(index, res);
				}
			}
			return false;
		}

		bool CLocalItem::IsDir() const
		{
			return NItemName::HasTailSlash(Name, GetCodePage());
		}

		bool CItem::IsDir() const
		{
			if(NItemName::HasTailSlash(Name, GetCodePage()))
				return true;
			Byte hostOS = GetHostOS();
			if(Size == 0 && PackSize == 0 && !Name.IsEmpty() && Name.Back() == '\\') {
				// do we need to use CharPrevExA?
				// .NET Framework 4.5 : System.IO.Compression::CreateFromDirectory() probably writes backslashes to
				// headers?
				// so we support that case
				switch(hostOS) {
					case NHostOS::kFAT:
					case NHostOS::kNTFS:
					case NHostOS::kHPFS:
					case NHostOS::kVFAT:
						return true;
				}
			}
			if(!FromCentral)
				return false;
			uint16 highAttrib = static_cast<uint16>((ExternalAttrib >> 16 ) & 0xFFFF);
			switch(hostOS) {
				case NHostOS::kAMIGA:
					switch(highAttrib & NAmigaAttrib::kIFMT) {
						case NAmigaAttrib::kIFDIR: return true;
						case NAmigaAttrib::kIFREG: return false;
						default: return false; // change it throw kUnknownAttributes;
					}
				case NHostOS::kFAT:
				case NHostOS::kNTFS:
				case NHostOS::kHPFS:
				case NHostOS::kVFAT:
					return ((ExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
				case NHostOS::kAtari:
				case NHostOS::kMac:
				case NHostOS::kVMS:
				case NHostOS::kVM_CMS:
				case NHostOS::kAcorn:
				case NHostOS::kMVS:
					return false; // change it throw kUnknownAttributes;
				case NHostOS::kUnix:
					return MY_LIN_S_ISDIR(highAttrib);
				default:
					return false;
			}
		}

		uint32 CItem::GetWinAttrib() const
		{
			uint32 winAttrib = 0;
			switch(GetHostOS()) {
				case NHostOS::kFAT:
				case NHostOS::kNTFS:
					if(FromCentral)
						winAttrib = ExternalAttrib;
					break;
				case NHostOS::kUnix:
					// do we need to clear 16 low bits in this case?
					if(FromCentral)
						winAttrib = ExternalAttrib & 0xFFFF0000;
					break;
			}
			if(IsDir()) // test it;
				winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
			return winAttrib;
		}

		bool CItem::GetPosixAttrib(uint32 &attrib) const
		{
			// some archivers can store PosixAttrib in high 16 bits even with HostOS=FAT.
			if(FromCentral && GetHostOS() == NHostOS::kUnix) {
				attrib = ExternalAttrib >> 16;
				return (attrib != 0);
			}
			attrib = 0;
			if(IsDir())
				attrib = MY_LIN_S_IFDIR;
			return false;
		}

		void CItem::GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const
		{
			bool isUtf8 = IsUtf8();
			bool ignore_Utf8_Errors = true;

			if(!isUtf8) {
				{
					const uint id = isComment ? NFileHeader::NExtraID::kIzUnicodeComment : NFileHeader::NExtraID::kIzUnicodeName;
					const CObjectVector<CExtraSubBlock> &subBlocks = GetMainExtra().SubBlocks;
					FOR_VECTOR(i, subBlocks) {
						const CExtraSubBlock &sb = subBlocks[i];
						if(sb.ID == id) {
							AString utf;
							if(sb.ExtractIzUnicode(CrcCalc(s, s.Len()), utf))
								if(ConvertUTF8ToUnicode(utf, res))
									return;
							break;
						}
					}
				}
				if(useSpecifiedCodePage)
					isUtf8 = (codePage == CP_UTF8);
			#ifdef _WIN32
				else if(GetHostOS() == NFileHeader::NHostOS::kUnix) {
					/* Some ZIP archives in Unix use UTF-8 encoding without Utf8 flag in header.
					   We try to get name as UTF-8.
					   Do we need to do it in POSIX version also? */
					isUtf8 = true;
					ignore_Utf8_Errors = false;
				}
			#endif
			}

			if(isUtf8)
				if(ConvertUTF8ToUnicode(s, res) || ignore_Utf8_Errors)
					return;

			MultiByteToUnicodeString2(res, s, useSpecifiedCodePage ? codePage : GetCodePage());
		}

		using namespace NFileHeader;

		static const uint32 kLzmaPropsSize = 5;
		static const uint32 kLzmaHeaderSize = 4 + kLzmaPropsSize;

		class CLzmaEncoder : public ICompressCoder, public ICompressSetCoderProperties, public CMyUnknownImp {
		public:
			NCompress::NLzma::CEncoder * EncoderSpec;
			CMyComPtr<ICompressCoder> Encoder;
			Byte Header[kLzmaHeaderSize];
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
		};

		STDMETHODIMP CLzmaEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps)
		{
			if(!Encoder) {
				EncoderSpec = new NCompress::NLzma::CEncoder;
				Encoder = EncoderSpec;
			}
			CBufPtrSeqOutStream * outStreamSpec = new CBufPtrSeqOutStream;
			CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
			outStreamSpec->Init(Header + 4, kLzmaPropsSize);
			RINOK(EncoderSpec->SetCoderProperties(propIDs, props, numProps));
			RINOK(EncoderSpec->WriteCoderProperties(outStream));
			if(outStreamSpec->GetPos() != kLzmaPropsSize)
				return E_FAIL;
			Header[0] = MY_VER_MAJOR;
			Header[1] = MY_VER_MINOR;
			Header[2] = kLzmaPropsSize;
			Header[3] = 0;
			return S_OK;
		}

		STDMETHODIMP CLzmaEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			RINOK(WriteStream(outStream, Header, kLzmaHeaderSize));
			return Encoder->Code(inStream, outStream, inSize, outSize, progress);
		}

		CAddCommon::CAddCommon(const CCompressionMethodMode &options) : _options(options), _copyCoderSpec(NULL), _cryptoStreamSpec(NULL), _buf(NULL), _isLzmaEos(false)
		{
		}

		CAddCommon::~CAddCommon()
		{
			MidFree(_buf);
		}

		static const uint32 kBufSize = ((uint32)1 << 16);

		HRESULT CAddCommon::CalcStreamCRC(ISequentialInStream * inStream, uint32 &resultCRC)
		{
			if(!_buf) {
				_buf = static_cast<Byte *>(MidAlloc(kBufSize));
				if(!_buf)
					return E_OUTOFMEMORY;
			}
			uint32 crc = CRC_INIT_VAL;
			for(;; ) {
				uint32 processed;
				RINOK(inStream->Read(_buf, kBufSize, &processed));
				if(processed == 0) {
					resultCRC = CRC_GET_DIGEST(crc);
					return S_OK;
				}
				crc = CrcUpdate(crc, _buf, (size_t)processed);
			}
		}

		HRESULT CAddCommon::Set_Pre_CompressionResult(bool seqMode, uint64 unpackSize, CCompressingResult &opRes) const
		{
			// We use Zip64, if unPackSize size is larger than 0xF8000000 to support
			// cases when compressed size can be about 3% larger than uncompressed size
			const uint32 kUnpackZip64Limit = 0xF8000000;
			opRes.UnpackSize = unpackSize;
			opRes.PackSize = (uint64)1 << 60; // we use big value to force Zip64 mode.
			if(unpackSize < kUnpackZip64Limit)
				opRes.PackSize = (uint32)0xFFFFFFFF - 1;  // it will not use Zip64 for that size
			if(opRes.PackSize < unpackSize)
				opRes.PackSize = unpackSize;
			Byte method = _options.MethodSequence[0];
			if(method == NCompressionMethod::kStore && !_options.PasswordIsDefined)
				opRes.PackSize = unpackSize;
			opRes.CRC = 0;
			opRes.LzmaEos = false;
			opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
			opRes.FileTimeWasUsed = false;
			if(_options.PasswordIsDefined) {
				opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;
				if(_options.IsAesMode)
					opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
				else {
					if(seqMode)
						opRes.FileTimeWasUsed = true;
				}
			}
			opRes.Method = method;
			Byte ver = 0;
			switch(method) {
				case NCompressionMethod::kStore: break;
				case NCompressionMethod::kDeflate: ver = NCompressionMethod::kExtractVersion_Deflate; break;
				case NCompressionMethod::kDeflate64: ver = NCompressionMethod::kExtractVersion_Deflate64; break;
				case NCompressionMethod::kXz: ver = NCompressionMethod::kExtractVersion_Xz; break;
				case NCompressionMethod::kPPMd: ver = NCompressionMethod::kExtractVersion_PPMd; break;
				case NCompressionMethod::kBZip2: ver = NCompressionMethod::kExtractVersion_BZip2; break;
				case NCompressionMethod::kLZMA:
				{
					ver = NCompressionMethod::kExtractVersion_LZMA;
					const COneMethodInfo * oneMethodMain = &_options._methods[0];
					opRes.LzmaEos = oneMethodMain->Get_Lzma_Eos();
					break;
				}
			}
			SETMAX(opRes.ExtractVersion, ver);
			return S_OK;
		}
		HRESULT CAddCommon::Compress(DECL_EXTERNAL_CODECS_LOC_VARS ISequentialInStream * inStream, IOutStream * outStream,
			bool seqMode, uint32 fileTime, ICompressProgressInfo * progress, CCompressingResult &opRes)
		{
			opRes.LzmaEos = false;
			if(!inStream) {
				// We can create empty stream here. But it was already implemented in caller code in 9.33+
				return E_INVALIDARG;
			}
			CSequentialInStreamWithCRC * inSecCrcStreamSpec = new CSequentialInStreamWithCRC;
			CMyComPtr<ISequentialInStream> inCrcStream = inSecCrcStreamSpec;
			CMyComPtr<IInStream> inStream2;
			if(!seqMode)
				inStream->QueryInterface(IID_IInStream, (void **)&inStream2);
			inSecCrcStreamSpec->SetStream(inStream);
			inSecCrcStreamSpec->Init();
			uint   numTestMethods = _options.MethodSequence.Size();
			if(seqMode || (numTestMethods > 1 && !inStream2))
				numTestMethods = 1;
			uint32 crc = 0;
			bool   crc_IsCalculated = false;
			Byte   method = 0;
			CFilterCoder::C_OutStream_Releaser outStreamReleaser;
			opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
			opRes.FileTimeWasUsed = false;

			for(uint i = 0; i < numTestMethods; i++) {
				opRes.LzmaEos = false;
				opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
				if(inStream2 && i != 0) {
					inSecCrcStreamSpec->Init();
					RINOK(inStream2->Seek(0, STREAM_SEEK_SET, NULL));
				}
				RINOK(outStream->SetSize(0));
				RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL));
				if(_options.PasswordIsDefined) {
					opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;
					if(!_cryptoStream) {
						_cryptoStreamSpec = new CFilterCoder(true);
						_cryptoStream = _cryptoStreamSpec;
					}
					if(_options.IsAesMode) {
						opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
						if(!_cryptoStreamSpec->Filter) {
							_cryptoStreamSpec->Filter = _filterAesSpec = new NCrypto::NWzAes::CEncoder;
							_filterAesSpec->SetKeyMode(_options.AesKeyMode);
							RINOK(_filterAesSpec->CryptoSetPassword((const Byte*)(const char *)_options.Password, _options.Password.Len()));
						}
						RINOK(_filterAesSpec->WriteHeader(outStream));
					}
					else {
						if(!_cryptoStreamSpec->Filter) {
							_cryptoStreamSpec->Filter = _filterSpec = new NCrypto::NZip::CEncoder;
							_filterSpec->CryptoSetPassword((const Byte*)(const char *)_options.Password, _options.Password.Len());
						}
						uint32 check;
						if(inStream2) {
							if(!crc_IsCalculated) {
								RINOK(CalcStreamCRC(inStream, crc));
								crc_IsCalculated = true;
								RINOK(inStream2->Seek(0, STREAM_SEEK_SET, NULL));
								inSecCrcStreamSpec->Init();
							}
							check = (crc >> 16);
						}
						else {
							opRes.FileTimeWasUsed = true;
							check = (fileTime & 0xFFFF);
						}
						RINOK(_filterSpec->WriteHeader_Check16(outStream, (uint16)check));
					}
					RINOK(_cryptoStreamSpec->SetOutStream(outStream));
					RINOK(_cryptoStreamSpec->InitEncoder());
					outStreamReleaser.FilterCoder = _cryptoStreamSpec;
				}
				method = _options.MethodSequence[i];
				switch(method) {
					case NCompressionMethod::kStore:
					{
						if(!_copyCoderSpec) {
							_copyCoderSpec = new NCompress::CCopyCoder;
							_copyCoder = _copyCoderSpec;
						}
						CMyComPtr <ISequentialOutStream> outStreamNew = _options.PasswordIsDefined ? _cryptoStream : outStream;
						RINOK(_copyCoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress));
						break;
					}
					default:
					{
						if(!_compressEncoder) {
							CLzmaEncoder * _lzmaEncoder = NULL;
							if(method == NCompressionMethod::kLZMA) {
								_compressExtractVersion = NCompressionMethod::kExtractVersion_LZMA;
								_lzmaEncoder = new CLzmaEncoder();
								_compressEncoder = _lzmaEncoder;
							}
							else if(method == NCompressionMethod::kXz) {
								_compressExtractVersion = NCompressionMethod::kExtractVersion_Xz;
								NCompress::NXz::CEncoder * encoder = new NCompress::NXz::CEncoder();
								_compressEncoder = encoder;
							}
							else if(method == NCompressionMethod::kPPMd) {
								_compressExtractVersion = NCompressionMethod::kExtractVersion_PPMd;
								NCompress::NPpmdZip::CEncoder * encoder = new NCompress::NPpmdZip::CEncoder();
								_compressEncoder = encoder;
							}
							else {
								CMethodId methodId;
								switch(method) {
									case NCompressionMethod::kBZip2:
										methodId = kMethodId_BZip2;
										_compressExtractVersion = NCompressionMethod::kExtractVersion_BZip2;
										break;
									default:
										_compressExtractVersion = ((method == NCompressionMethod::kDeflate64) ?
											NCompressionMethod::kExtractVersion_Deflate64 : NCompressionMethod::kExtractVersion_Deflate);
										methodId = kMethodId_ZipBase + method;
										break;
								}
								RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS methodId, true, _compressEncoder));
								if(!_compressEncoder)
									return E_NOTIMPL;
								if(method == NCompressionMethod::kDeflate || method == NCompressionMethod::kDeflate64) {
								}
								else if(method == NCompressionMethod::kBZip2) {
								}
							}
							{
								CMyComPtr<ICompressSetCoderProperties> setCoderProps;
								_compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProps);
								if(setCoderProps) {
									if(!_options._methods.IsEmpty()) {
										COneMethodInfo * oneMethodMain = &_options._methods[0];
										RINOK(oneMethodMain->SetCoderProps(setCoderProps, _options._dataSizeReduceDefined ? &_options._dataSizeReduce : NULL));
									}
								}
							}
							if(method == NCompressionMethod::kLZMA)
								_isLzmaEos = _lzmaEncoder->EncoderSpec->IsWriteEndMark();
						}
						if(method == NCompressionMethod::kLZMA)
							opRes.LzmaEos = _isLzmaEos;
						CMyComPtr <ISequentialOutStream> outStreamNew = _options.PasswordIsDefined ? _cryptoStream : outStream;
						if(_compressExtractVersion > opRes.ExtractVersion)
							opRes.ExtractVersion = _compressExtractVersion;
						RINOK(_compressEncoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress));
						break;
					}
				}
				if(_options.PasswordIsDefined) {
					RINOK(_cryptoStreamSpec->OutStreamFinish());
					if(_options.IsAesMode) {
						RINOK(_filterAesSpec->WriteFooter(outStream));
					}
				}
				RINOK(outStream->Seek(0, STREAM_SEEK_CUR, &opRes.PackSize));
				{
					opRes.CRC = inSecCrcStreamSpec->GetCRC();
					opRes.UnpackSize = inSecCrcStreamSpec->GetSize();
				}
				if(_options.PasswordIsDefined) {
					if(opRes.PackSize < opRes.UnpackSize + (_options.IsAesMode ? _filterAesSpec->GetAddPackSize() : NCrypto::NZip::kHeaderSize))
						break;
				}
				else if(opRes.PackSize < opRes.UnpackSize)
					break;
			}
			opRes.Method = method;
			return S_OK;
		}
		//
		// (kBufferSize >= kDataDescriptorSize64 + 4)
		static const size_t kSeqBufferSize = (size_t)1 << 14;
		/*
		   if(not defined ZIP_SELF_CHECK) : it reads CD and if error in first pass CD reading, it reads LOCALS-CD-MODE
		   if(    defined ZIP_SELF_CHECK) : it always reads CD and LOCALS-CD-MODE
		   use ZIP_SELF_CHECK to check LOCALS-CD-MODE for any zip archive
		 */
		// #define ZIP_SELF_CHECK

		struct CEcd {
			uint16 ThisDisk;
			uint16 CdDisk;
			uint16 NumEntries_in_ThisDisk;
			uint16 NumEntries;
			uint32 Size;
			uint32 Offset;
			uint16 CommentSize;

			bool IsEmptyArc() const
			{
				return ThisDisk == 0 && CdDisk == 0 && NumEntries_in_ThisDisk == 0 && NumEntries == 0 && Size == 0 && Offset == 0; // test it
			}
			void Parse(const Byte * p); // (p) doesn't include signature
		};

		void CEcd::Parse(const Byte * p)
		{
			// (p) doesn't include signature
			G16(0, ThisDisk);
			G16(2, CdDisk);
			G16(4, NumEntries_in_ThisDisk);
			G16(6, NumEntries);
			G32(8, Size);
			G32(12, Offset);
			G16(16, CommentSize);
		}

		CInArchive::CCdInfo::CCdInfo()
		{
			memzero(this, sizeof(*this)); 
			IsFromEcd64 = false;
		}

		bool CInArchive::CCdInfo::IsEmptyArc() const
		{
			return (ThisDisk == 0 && CdDisk == 0 && NumEntries_in_ThisDisk == 0 && NumEntries == 0 && Size == 0 && Offset == 0); // test it
		}

		void CInArchive::CCdInfo::ParseEcd32(const Byte * p)
		{
			IsFromEcd64 = false;
			// (p) includes signature
			p += 4;
			G16(0, ThisDisk);
			G16(2, CdDisk);
			G16(4, NumEntries_in_ThisDisk);
			G16(6, NumEntries);
			G32(8, Size);
			G32(12, Offset);
			G16(16, CommentSize);
		}

		void CInArchive::CCdInfo::ParseEcd64e(const Byte * p)
		{
			IsFromEcd64 = true;
			// (p) exclude signature
			G16(0, VersionMade);
			G16(2, VersionNeedExtract);
			G32(4, ThisDisk);
			G32(8, CdDisk);

			G64(12, NumEntries_in_ThisDisk);
			G64(20, NumEntries);
			G64(28, Size);
			G64(36, Offset);
		}

		struct CLocator {
			CLocator() : Ecd64Disk(0), NumDisks(0), Ecd64Offset(0) 
			{
			}
			void Parse(const Byte * p)
			{
				G32(0, Ecd64Disk);
				G64(4, Ecd64Offset);
				G32(12, NumDisks);
			}
			bool IsEmptyArc() const
			{
				return Ecd64Disk == 0 && NumDisks == 0 && Ecd64Offset == 0;
			}
			uint32 Ecd64Disk;
			uint32 NumDisks;
			uint64 Ecd64Offset;
		};

		CInArchive::CInArchiveInfo::CInArchiveInfo() : Base(0), MarkerPos(0), MarkerPos2(0), FinishPos(0), FileEndPos(0),
			FirstItemRelatOffset(0), MarkerVolIndex(-1), CdWasRead(false), IsSpanMode(false), ThereIsTail(false)
			// BaseVolIndex(0)
		{
		}
		void CInArchive::CInArchiveInfo::Clear()
		{
			// BaseVolIndex = 0;
			Base = 0;
			MarkerPos = 0;
			MarkerPos2 = 0;
			FinishPos = 0;
			FileEndPos = 0;
			MarkerVolIndex = -1;
			ThereIsTail = false;
			FirstItemRelatOffset = 0;
			CdWasRead = false;
			IsSpanMode = false;
			Comment.Free();
		}

		CInArchive::CInArchive() : Stream(NULL), StartStream(NULL), Callback(NULL), IsArcOpen(false) 
		{
		}

		uint64 CInArchive::GetPhySize() const { return IsMultiVol ? ArcInfo.FinishPos : (ArcInfo.FinishPos - ArcInfo.Base); }
		uint64 CInArchive::GetOffset() const { return IsMultiVol ? 0 : ArcInfo.Base; }

		void CInArchive::ClearRefs()
		{
			StreamRef.Release();
			Stream = NULL;
			StartStream = NULL;
			Callback = NULL;
			Vols.Clear();
		}
		void CInArchive::Close()
		{
			_cnt = 0;
			DisableBufMode();
			IsArcOpen = false;
			IsArc = false;
			IsZip64 = false;
			HeadersError = false;
			HeadersWarning = false;
			ExtraMinorError = false;
			UnexpectedEnd = false;
			LocalsWereRead = false;
			LocalsCenterMerged = false;
			NoCentralDir = false;
			Overflow32bit = false;
			Cd_NumEntries_Overflow_16bit = false;
			MarkerIsFound = false;
			MarkerIsSafe = false;
			IsMultiVol = false;
			UseDisk_in_SingleVol = false;
			EcdVolIndex = 0;
			ArcInfo.Clear();
			ClearRefs();
		}

		HRESULT CInArchive::Seek_SavePos(uint64 offset)
		{
			// InitBuf();
			// if(!Stream) return S_FALSE;
			return Stream->Seek(offset, STREAM_SEEK_SET, &_streamPos);
		}

		HRESULT CInArchive::SeekToVol(int volIndex, uint64 offset)
		{
			if(volIndex != Vols.StreamIndex) {
				InitBuf();
				if(IsMultiVol && volIndex >= 0) {
					if((uint)volIndex >= Vols.Streams.Size())
						return S_FALSE;
					if(!Vols.Streams[volIndex].Stream)
						return S_FALSE;
					Stream = Vols.Streams[volIndex].Stream;
				}
				else if(volIndex == -2) {
					if(!Vols.ZipStream)
						return S_FALSE;
					Stream = Vols.ZipStream;
				}
				else
					Stream = StartStream;
				Vols.StreamIndex = volIndex;
			}
			else {
				if(offset <= _streamPos) {
					const uint64 back = _streamPos - offset;
					if(back <= _bufCached) {
						_bufPos = _bufCached - (size_t)back;
						return S_OK;
					}
				}
				InitBuf();
			}
			return Seek_SavePos(offset);
		}

		// ---------- ReadFromCache ----------
		// reads from cache and from Stream
		// move to next volume can be allowed if(CanStartNewVol) and only before first byte reading

		HRESULT CInArchive::ReadFromCache(Byte * data, uint size, unsigned &processed)
		{
			HRESULT result = S_OK;
			processed = 0;
			for(;; ) {
				if(size == 0)
					return S_OK;
				else {
					const size_t avail = GetAvail();
					if(avail) {
						uint   cur = size;
						SETMIN(cur, (uint)avail);
						memcpy(data, (const Byte*)Buffer + _bufPos, cur);
						data += cur;
						size -= cur;
						processed += cur;
						_bufPos += cur;
						_cnt += cur;
						CanStartNewVol = false;
						continue;
					}
					else {
						InitBuf();
						if(_inBufMode) {
							uint32 cur = 0;
							result = Stream->Read(Buffer, (uint32)Buffer.Size(), &cur);
							_bufPos = 0;
							_bufCached = cur;
							_streamPos += cur;
							if(cur != 0)
								CanStartNewVol = false;
							if(result != S_OK)
								break;
							else if(cur != 0)
								continue;
						}
						else {
							uint32 cur = 0;
							result = Stream->Read(data, size, &cur);
							data += cur;
							size -= cur;
							processed += cur;
							_streamPos += cur;
							_cnt += cur;
							if(cur != 0) {
								CanStartNewVol = false;
								break;
							}
							else if(result != S_OK)
								break;
						}
						if(!IsMultiVol || !CanStartNewVol || Vols.StreamIndex < 0 || (uint)Vols.StreamIndex + 1 >= Vols.Streams.Size())
							break;
						else {
							const CVols::CSubStreamInfo &s = Vols.Streams[Vols.StreamIndex + 1];
							if(!s.Stream)
								break;
							else {
								result = s.SeekToStart();
								if(result != S_OK)
									break;
								else {
									Vols.StreamIndex++;
									_streamPos = 0;
									// Vols.NeedSeek = false;
									Stream = s.Stream;
								}
							}
						}
					}
				}
			}
			return result;
		}

		static bool CheckDosTime(uint32 dosTime)
		{
			if(dosTime) {
				uint   month = (dosTime >> 21) & 0xF;
				uint   day = (dosTime >> 16) & 0x1F;
				uint   hour = (dosTime >> 11) & 0x1F;
				uint   min = (dosTime >> 5) & 0x3F;
				uint   sec = (dosTime & 0x1F) * 2;
				if(month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
					return false;
			}
			return true;
		}

		API_FUNC_IsArc IsArc_Zip(const Byte * p, size_t size)
		{
			if(size < 8)
				return k_IsArc_Res_NEED_MORE;
			if(p[0] != 'P')
				return k_IsArc_Res_NO;
			uint32 sig = Get32(p);
			if(sig == NSignature::kNoSpan || sig == NSignature::kSpan) {
				p += 4;
				size -= 4;
			}
			sig = Get32(p);
			if(sig == NSignature::kEcd64) {
				if(size < kEcd64_FullSize)
					return k_IsArc_Res_NEED_MORE;
				const uint64 recordSize = Get64(p + 4);
				if(recordSize < kEcd64_MainSize || recordSize > kEcd64_MainSize + (1 << 20))
					return k_IsArc_Res_NO;
				CInArchive::CCdInfo cdInfo;
				cdInfo.ParseEcd64e(p + 12);
				if(!cdInfo.IsEmptyArc())
					return k_IsArc_Res_NO;
				return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
			}
			if(sig == NSignature::kEcd) {
				if(size < kEcdSize)
					return k_IsArc_Res_NEED_MORE;
				CEcd ecd;
				ecd.Parse(p + 4);
				// if(ecd.cdSize != 0)
				if(!ecd.IsEmptyArc())
					return k_IsArc_Res_NO;
				return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
			}
			if(sig != NSignature::kLocalFileHeader)
				return k_IsArc_Res_NO;
			if(size < kLocalHeaderSize)
				return k_IsArc_Res_NEED_MORE;
			p += 4;
			{
				const uint kPureHeaderSize = kLocalHeaderSize - 4;
				uint i;
				for(i = 0; i < kPureHeaderSize && p[i] == 0; i++) ;
				if(i == kPureHeaderSize)
					return k_IsArc_Res_NEED_MORE;
			}
			/*
			   if(p[0] >= 128) // ExtractVersion.Version;
			   return k_IsArc_Res_NO;
			 */

			// ExtractVersion.Version = p[0];
			// ExtractVersion.HostOS = p[1];
			// Flags = Get16(p + 2);
			// Method = Get16(p + 4);
			/*
			   // 9.33: some zip archives contain incorrect value in timestamp. So we don't check it now
			   uint32 dosTime = Get32(p + 6);
			   if(!CheckDosTime(dosTime))
			   return k_IsArc_Res_NO;
			 */
			// Crc = Get32(p + 10);
			// PackSize = Get32(p + 14);
			// Size = Get32(p + 18);
			const uint nameSize = Get16(p + 22);
			unsigned extraSize = Get16(p + 24);
			const uint32 extraOffset = kLocalHeaderSize + (uint32)nameSize;
			if(extraOffset + extraSize > (1 << 16))
				return k_IsArc_Res_NO;
			p -= 4;
			{
				size_t rem = size - kLocalHeaderSize;
				SETMIN(rem, nameSize);
				const Byte * p2 = p + kLocalHeaderSize;
				for(size_t i = 0; i < rem; i++)
					if(p2[i] == 0) {
						// we support some "bad" zip archives that contain zeros after name
						for(size_t k = i + 1; k < rem; k++)
							if(p2[k] != 0)
								return k_IsArc_Res_NO;
						break;
						/*
						   if(i != nameSize - 1)
						   return k_IsArc_Res_NO;
						 */
					}
			}

			if(size < extraOffset)
				return k_IsArc_Res_NEED_MORE;

			if(extraSize > 0) {
				p += extraOffset;
				size -= extraOffset;
				while(extraSize != 0) {
					if(extraSize < 4) {
						// 7-Zip before 9.31 created incorrect WsAES Extra in folder's local headers.
						// so we return k_IsArc_Res_YES to support such archives.
						// return k_IsArc_Res_NO; // do we need to support such extra ?
						return k_IsArc_Res_YES;
					}
					if(size < 4)
						return k_IsArc_Res_NEED_MORE;
					unsigned dataSize = Get16(p + 2);
					size -= 4;
					extraSize -= 4;
					p += 4;
					if(dataSize > extraSize)
						return k_IsArc_Res_NO;
					if(dataSize > size)
						return k_IsArc_Res_NEED_MORE;
					size -= dataSize;
					extraSize -= dataSize;
					p += dataSize;
				}
			}
			return k_IsArc_Res_YES;
		}

		static uint32 IsArc_Zip_2(const Byte * p, size_t size, bool isFinal)
		{
			uint32 res = IsArc_Zip(p, size);
			return (res == k_IsArc_Res_NEED_MORE && isFinal) ? k_IsArc_Res_NO : res;
		}

		MY_NO_INLINE static const Byte * FASTCALL FindPK(const Byte * p, const Byte * limit)
		{
			for(;; ) {
				for(;; ) {
					Byte b0 = p[0];
					if(p >= limit)
						return p;
					p++;
					if(b0 == 0x50)
						break;
				}
				if(p[0] == 0x4B)
					return p - 1;
			}
		}
		/*
		   ---------- FindMarker ----------
		   returns:
		   S_OK:
			ArcInfo.MarkerVolIndex : volume of marker
			ArcInfo.MarkerPos   : Pos of first signature
			ArcInfo.MarkerPos2  : Pos of main signature (local item signature in most cases)
			_streamPos          : stream pos
			_cnt                : The number of virtal Bytes after start of search to offset after signature
			_signature          : main signature

		   S_FALSE: can't find marker, or there is some non-zip data after marker

		   Error code: stream reading error.
		 */

		HRESULT CInArchive::FindMarker(const uint64 * searchLimit)
		{
			ArcInfo.MarkerPos = GetVirtStreamPos();
			ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
			ArcInfo.MarkerVolIndex = Vols.StreamIndex;
			_cnt = 0;
			CanStartNewVol = false;
			if(searchLimit && *searchLimit == 0) {
				Byte startBuf[kMarkerSize];
				unsigned processed;
				RINOK(ReadFromCache(startBuf, kMarkerSize, processed));
				if(processed != kMarkerSize)
					return S_FALSE;
				uint32 marker = Get32(startBuf);
				_signature = marker;
				if(marker == NSignature::kNoSpan || marker == NSignature::kSpan) {
					RINOK(ReadFromCache(startBuf, kMarkerSize, processed));
					if(processed != kMarkerSize)
						return S_FALSE;
					_signature = Get32(startBuf);
				}
				if(_signature != NSignature::kEcd && _signature != NSignature::kEcd64 && _signature != NSignature::kLocalFileHeader)
					return S_FALSE;
				ArcInfo.MarkerPos2 = GetVirtStreamPos() - 4;
				ArcInfo.IsSpanMode = (marker == NSignature::kSpan);
				// we use weak test in case of (*searchLimit == 0)
				// since error will be detected later in Open function
				return S_OK;
			}
			const size_t kCheckSize = (size_t)1 << 16; // must be smaller than kBufSize
			const size_t kBufSize   = (size_t)1 << 17; // must be larger than kCheckSize
			if(Buffer.Size() < kBufSize) {
				InitBuf();
				Buffer.AllocAtLeast(kBufSize);
				if(!Buffer.IsAllocated())
					return E_OUTOFMEMORY;
			}
			_inBufMode = true;
			uint64 progressPrev = 0;
			for(;; ) {
				RINOK(LookAhead(kBufSize));
				const size_t avail = GetAvail();
				size_t limitPos;
				const bool isFinished = (avail != kBufSize);
				if(isFinished) {
					const uint kMinAllowed = 4;
					if(avail <= kMinAllowed) {
						if(!IsMultiVol || Vols.StreamIndex < 0 || (uint)Vols.StreamIndex + 1 >= Vols.Streams.Size())
							break;
						SkipLookahed(avail);
						const CVols::CSubStreamInfo &s = Vols.Streams[Vols.StreamIndex + 1];
						if(!s.Stream)
							break;
						RINOK(s.SeekToStart());
						InitBuf();
						Vols.StreamIndex++;
						_streamPos = 0;
						Stream = s.Stream;
						continue;
					}
					limitPos = avail - kMinAllowed;
				}
				else
					limitPos = (avail - kCheckSize);

				// we don't check at (limitPos) for good fast aligned operations

				if(searchLimit) {
					if(_cnt > *searchLimit)
						break;
					uint64 rem = *searchLimit - _cnt;
					if(limitPos > rem)
						limitPos = (size_t)rem + 1;
				}

				if(limitPos == 0)
					break;
				const Byte * const pStart = Buffer + _bufPos;
				const Byte * p = pStart;
				const Byte * const limit = pStart + limitPos;
				for(;; p++) {
					p = FindPK(p, limit);
					if(p >= limit)
						break;
					const size_t rem = pStart + avail - p;
					uint32 res = IsArc_Zip_2(p, rem, isFinished);
					if(res != k_IsArc_Res_NO) {
						if(rem < kMarkerSize)
							return S_FALSE;
						_signature = Get32(p);
						SkipLookahed(p - pStart);
						ArcInfo.MarkerVolIndex = Vols.StreamIndex;
						ArcInfo.MarkerPos = GetVirtStreamPos();
						ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
						SkipLookahed(4);
						if(oneof2(_signature, NSignature::kNoSpan, NSignature::kSpan)) {
							if(rem < kMarkerSize * 2)
								return S_FALSE;
							ArcInfo.IsSpanMode = (_signature == NSignature::kSpan);
							_signature = Get32(p + 4);
							ArcInfo.MarkerPos2 += 4;
							SkipLookahed(4);
						}
						return S_OK;
					}
				}

				if(!IsMultiVol && isFinished)
					break;

				SkipLookahed(p - pStart);

				if(Callback && (_cnt - progressPrev) >= ((uint32)1 << 23)) {
					progressPrev = _cnt;
					// const uint64 numFiles64 = 0;
					RINOK(Callback->SetCompleted(NULL, &_cnt));
				}
			}
			return S_FALSE;
		}
		/*
		   ---------- IncreaseRealPosition ----------
		   moves virtual offset in virtual stream.
		   changing to new volumes is allowed
		 */
		HRESULT CInArchive::IncreaseRealPosition(uint64 offset, bool &isFinished)
		{
			isFinished = false;
			for(;; ) {
				const size_t avail = GetAvail();
				if(offset <= avail) {
					_bufPos += (size_t)offset;
					_cnt += offset;
					return S_OK;
				}
				_cnt += avail;
				offset -= avail;
				_bufCached = 0;
				_bufPos = 0;
				if(!_inBufMode)
					break;
				CanStartNewVol = true;
				LookAhead(1);
				if(GetAvail() == 0)
					return S_OK;
			}
			if(!IsMultiVol) {
				_cnt += offset;
				return Stream->Seek(offset, STREAM_SEEK_CUR, &_streamPos);
			}
			for(;; ) {
				if(offset == 0)
					return S_OK;
				if(Vols.StreamIndex < 0)
					return S_FALSE;
				if((uint)Vols.StreamIndex >= Vols.Streams.Size()) {
					isFinished = true;
					return S_OK;
				}
				{
					const CVols::CSubStreamInfo &s = Vols.Streams[Vols.StreamIndex];
					if(!s.Stream) {
						isFinished = true;
						return S_OK;
					}
					if(_streamPos > s.Size)
						return S_FALSE;
					const uint64 rem = s.Size - _streamPos;
					if((uint64)offset <= rem) {
						_cnt += offset;
						return Stream->Seek(offset, STREAM_SEEK_CUR, &_streamPos);
					}
					RINOK(Seek_SavePos(s.Size));
					offset -= rem;
					_cnt += rem;
				}
				Stream = NULL;
				_streamPos = 0;
				Vols.StreamIndex++;
				if((uint)Vols.StreamIndex >= Vols.Streams.Size()) {
					isFinished = true;
					return S_OK;
				}
				const CVols::CSubStreamInfo &s2 = Vols.Streams[Vols.StreamIndex];
				if(!s2.Stream) {
					isFinished = true;
					return S_OK;
				}
				Stream = s2.Stream;
				RINOK(Seek_SavePos(0));
			}
		}
		/*
		   ---------- LookAhead ----------
		   Reads data to buffer, if required.

		   It can read from volumes as long as Buffer.Size().
		   But it moves to new volume, only if it's required to provide minRequired bytes in buffer.

		   in:
		   (minRequired <= Buffer.Size())

		   return:
		   S_OK : if(GetAvail() < minRequired) after function return, it's end of stream(s) data, or no new volume stream.
		   Error codes: IInStream::Read() error or IInStream::Seek() error for multivol
		 */
		HRESULT CInArchive::LookAhead(size_t minRequired)
		{
			for(;; ) {
				const size_t avail = GetAvail();
				if(minRequired <= avail)
					return S_OK;
				if(_bufPos != 0) {
					if(avail != 0)
						memmove(Buffer, Buffer + _bufPos, avail);
					_bufPos = 0;
					_bufCached = avail;
				}
				const size_t pos = _bufCached;
				uint32 processed = 0;
				HRESULT res = Stream->Read(Buffer + pos, (uint32)(Buffer.Size() - pos), &processed);
				_streamPos += processed;
				_bufCached += processed;
				if(res != S_OK)
					return res;
				if(processed != 0)
					continue;
				if(!IsMultiVol || !CanStartNewVol || Vols.StreamIndex < 0 || (uint)Vols.StreamIndex + 1 >= Vols.Streams.Size())
					return S_OK;
				const CVols::CSubStreamInfo &s = Vols.Streams[Vols.StreamIndex + 1];
				if(!s.Stream)
					return S_OK;
				RINOK(s.SeekToStart());
				Vols.StreamIndex++;
				_streamPos = 0;
				Stream = s.Stream;
				// Vols.NeedSeek = false;
			}
		}

		class CUnexpectEnd {};

		/*
		   ---------- SafeRead ----------

		   reads data of exact size from stream(s)

		   in:
		   _inBufMode
		   if(CanStartNewVol) it can go to next volume before first byte reading, if there is end of volume data.

		   in, out:
		   _streamPos  :  position in Stream
		   Stream
		   Vols  :  if(IsMultiVol)
		   _cnt

		   out:
		   (CanStartNewVol == false), if some data was read

		   return:
		   S_OK : success reading of requested data

		   exceptions:
		   CSystemException() - stream reading error
		   CUnexpectEnd()  :  could not read data of requested size
		 */
		void FASTCALL CInArchive::SafeRead(Byte * data, uint size)
		{
			uint   processed;
			HRESULT result = ReadFromCache(data, size, processed);
			if(result != S_OK)
				throw CSystemException(result);
			if(size != processed)
				throw CUnexpectEnd();
		}
		void CInArchive::ReadBuffer(CByteBuffer &buffer, uint size)
		{
			buffer.Alloc(size);
			if(size != 0)
				SafeRead(buffer, size);
		}

		// Byte CInArchive::ReadByte  () { Byte b;      SafeRead(&b, 1); return b; }
		// uint16 CInArchive::ReadUInt16() { Byte buf[2]; SafeRead(buf, 2); return Get16(buf); }
		uint32 CInArchive::ReadUInt32() { Byte buf[4]; SafeRead(buf, 4); return Get32(buf); }
		uint64 CInArchive::ReadUInt64() { Byte buf[8]; SafeRead(buf, 8); return Get64(buf); }

		void CInArchive::ReadSignature()
		{
			CanStartNewVol = true;
			_signature = ReadUInt32();
			// CanStartNewVol = false; // it's already changed in SafeRead
		}
		//
		// we Skip() inside headers only, so no need for stream change in multivol.
		//
		void FASTCALL CInArchive::Skip(size_t num)
		{
			while(num != 0) {
				const  uint kBufSize = (size_t)1 << 10;
				Byte   buf[kBufSize];
				uint   step = kBufSize;
				SETMIN(step, (uint)num);
				SafeRead(buf, step);
				num -= step;
			}
		}
		/*
		   HRESULT CInArchive::Callback_Completed(unsigned numFiles)
		   {
		   const uint64 numFiles64 = numFiles;
		   return Callback->SetCompleted(&numFiles64, &_cnt);
		   }
		 */
		HRESULT CInArchive::Skip64(uint64 num, unsigned numFiles)
		{
			if(num == 0)
				return S_OK;
			for(;; ) {
				size_t step = (size_t)1 << 24;
				SETMIN(step, (size_t)num);
				Skip(step);
				num -= step;
				if(num == 0)
					return S_OK;
				if(Callback) {
					const uint64 numFiles64 = numFiles;
					RINOK(Callback->SetCompleted(&numFiles64, &_cnt));
				}
			}
		}

		bool CInArchive::ReadFileName(uint size, AString &s)
		{
			if(size == 0) {
				s.Empty();
				return true;
			}
			else {
				char * p = s.GetBuf(size);
				SafeRead((Byte *)p, size);
				uint   i = size;
				do {
					if(p[i - 1] != 0)
						break;
				} while(--i);
				s.ReleaseBuf_CalcLen(size);
				return s.Len() == i;
			}
		}

		#define ZIP64_IS_32_MAX(n) ((n) == 0xFFFFFFFF)
		#define ZIP64_IS_16_MAX(n) ((n) == 0xFFFF)

		bool CInArchive::ReadExtra(unsigned extraSize, CExtraBlock &extra, uint64 &unpackSize, uint64 &packSize, uint64 &localOffset, uint32 &disk)
		{
			extra.Clear();
			while(extraSize >= 4) {
				CExtraSubBlock subBlock;
				const uint32 pair = ReadUInt32();
				subBlock.ID = (pair & 0xFFFF);
				uint size = (uint)(pair >> 16);
				extraSize -= 4;
				if(size > extraSize) {
					// it's error in extra
					HeadersWarning = true;
					extra.Error = true;
					Skip(extraSize);
					return false;
				}
				extraSize -= size;
				if(subBlock.ID == NFileHeader::NExtraID::kZip64) {
					extra.IsZip64 = true;
					bool isOK = true;
					if(ZIP64_IS_32_MAX(unpackSize))
						if(size < 8) 
							isOK = false; 
						else { 
							size -= 8; 
							unpackSize = ReadUInt64(); 
						}
					if(isOK && ZIP64_IS_32_MAX(packSize))
						if(size < 8) 
							isOK = false; 
						else { 
							size -= 8; 
							packSize = ReadUInt64(); 
						}
					if(isOK && ZIP64_IS_32_MAX(localOffset))
						if(size < 8) 
							isOK = false; 
						else { 
							size -= 8; 
							localOffset = ReadUInt64(); 
						}
					if(isOK && ZIP64_IS_16_MAX(disk))
						if(size < 4) 
							isOK = false; 
						else { 
							size -= 4; 
							disk = ReadUInt32(); 
						}
					if(!isOK || size != 0) {
						HeadersWarning = true;
						extra.Error = true;
						extra.IsZip64_Error = true;
						Skip(size);
					}
				}
				else {
					ReadBuffer(subBlock.Data, size);
					extra.SubBlocks.Add(subBlock);
				}
			}
			if(extraSize) {
				ExtraMinorError = true;
				extra.MinorError = true;
				// 7-Zip before 9.31 created incorrect WsAES Extra in folder's local headers.
				// so we don't return false, but just set warning flag
				// return false;
				Skip(extraSize);
			}
			return true;
		}

		bool CInArchive::ReadLocalItem(CItemEx &item)
		{
			item.Disk = 0;
			if(IsMultiVol && Vols.StreamIndex >= 0)
				item.Disk = Vols.StreamIndex;
			const uint kPureHeaderSize = kLocalHeaderSize - 4;
			Byte p[kPureHeaderSize];
			SafeRead(p, kPureHeaderSize);
			{
				uint i;
				for(i = 0; i < kPureHeaderSize && p[i] == 0; i++) 
					;
				if(i == kPureHeaderSize)
					return false;
			}
			item.ExtractVersion.Version = p[0];
			item.ExtractVersion.HostOS = p[1];
			G16(2, item.Flags);
			G16(4, item.Method);
			G32(6, item.Time);
			G32(10, item.Crc);
			G32(14, item.PackSize);
			G32(18, item.Size);
			const uint nameSize = Get16(p + 22);
			const uint extraSize = Get16(p + 24);
			bool isOkName = ReadFileName(nameSize, item.Name);
			item.LocalFullHeaderSize = kLocalHeaderSize + (uint32)nameSize + extraSize;
			item.DescriptorWasRead = false;

			/*
			   if(item.IsDir())
			   item.Size = 0; // check It
			 */
			if(extraSize > 0) {
				uint64 localOffset = 0;
				uint32 disk = 0;
				if(!ReadExtra(extraSize, item.LocalExtra, item.Size, item.PackSize, localOffset, disk)) {
					/* Most of archives are OK for Extra. But there are some rare cases
					   that have error. And if error in first item, it can't open archive.
					   So we ignore that error */
					// return false;
				}
			}
			if(!CheckDosTime(item.Time)) {
				HeadersWarning = true;
				// return false;
			}
			if(item.Name.Len() != nameSize) {
				// we support some "bad" zip archives that contain zeros after name
				if(!isOkName)
					return false;
				HeadersWarning = true;
			}
			return item.LocalFullHeaderSize <= ((uint32)1 << 16);
		}

		static bool FASTCALL FlagsAreSame(const CItem &i1, const CItem &i2)
		{
			if(i1.Method != i2.Method)
				return false;
			else if(i1.Flags == i2.Flags)
				return true;
			else {
				uint32 mask = 0xFFFF;
				switch(i1.Method) {
					case NFileHeader::NCompressionMethod::kDeflate:
						mask = 0x7FF9;
						break;
					default:
						if(i1.Method <= NFileHeader::NCompressionMethod::kImplode)
							mask = 0x7FFF;
				}
				// we can ignore utf8 flag, if name is ascii
				if((i1.Flags ^ i2.Flags) & NFileHeader::NFlags::kUtf8)
					if(i1.Name.IsAscii() && i2.Name.IsAscii())
						mask &= ~NFileHeader::NFlags::kUtf8;
				return ((i1.Flags & mask) == (i2.Flags & mask));
			}
		}

		// #ifdef _WIN32
		static bool AreEqualPaths_IgnoreSlashes(const char * s1, const char * s2)
		{
			for(;; ) {
				char c1 = *s1++;
				char c2 = *s2++;
				if(c1 == c2) {
					if(c1 == 0)
						return true;
				}
				else {
					if(c1 == '\\') c1 = '/';
					if(c2 == '\\') c2 = '/';
					if(c1 != c2)
						return false;
				}
			}
		}

		// #endif

		static bool AreItemsEqual(const CItemEx &localItem, const CItemEx &cdItem)
		{
			if(!FlagsAreSame(cdItem, localItem))
				return false;
			if(!localItem.HasDescriptor()) {
				// some program writes 0 to crc field in central directory
				if(cdItem.PackSize != localItem.PackSize || cdItem.Size != localItem.Size || cdItem.Crc != localItem.Crc && cdItem.Crc != 0) 
					return false;
			}
			/* pkzip 2.50 creates incorrect archives. It uses
				 - WIN encoding for name in local header
				 - OEM encoding for name in central header
			   We don't support these strange items. */

			/* if(cdItem.Name.Len() != localItem.Name.Len())
			   return false;
			 */
			if(cdItem.Name != localItem.Name) {
				// #ifdef _WIN32
				// some xap files use backslash in central dir items.
				// we can ignore such errors in windows, where all slashes are converted to backslashes
				unsigned hostOs = cdItem.GetHostOS();
				if(hostOs == NFileHeader::NHostOS::kFAT || hostOs == NFileHeader::NHostOS::kNTFS) {
					if(!AreEqualPaths_IgnoreSlashes(cdItem.Name, localItem.Name)) {
						// pkzip 2.50 uses DOS encoding in central dir and WIN encoding in local header.
						// so we ignore that error
						if(hostOs != NFileHeader::NHostOS::kFAT || cdItem.MadeByVersion.Version < 25 || cdItem.MadeByVersion.Version > 40)
							return false;
					}
				}
				/*
				   else
				   #endif
				   return false;
				 */
			}
			return true;
		}

		HRESULT CInArchive::ReadLocalItemAfterCdItem(CItemEx &item, bool &isAvail, bool &headersError)
		{
			InitBuf();
			_inBufMode = false;
			isAvail = true;
			headersError = false;
			if(item.FromLocal)
				return S_OK;
			try {
				uint64 offset = item.LocalHeaderPos;
				if(IsMultiVol) {
					if(item.Disk >= Vols.Streams.Size()) {
						isAvail = false;
						return S_FALSE;
					}
					Stream = Vols.Streams[item.Disk].Stream;
					Vols.StreamIndex = item.Disk;
					if(!Stream) {
						isAvail = false;
						return S_FALSE;
					}
				}
				else {
					if(UseDisk_in_SingleVol && item.Disk != EcdVolIndex) {
						isAvail = false;
						return S_FALSE;
					}
					Stream = StreamRef;

					offset += ArcInfo.Base;
					if(ArcInfo.Base < 0 && (int64)offset < 0) {
						isAvail = false;
						return S_FALSE;
					}
				}

				RINOK(Seek_SavePos(offset));

				/*
				   // we can check buf mode
				   InitBuf();
				   _inBufMode = true;
				   Buffer.AllocAtLeast(1 << 10);
				 */

				CItemEx localItem;
				if(ReadUInt32() != NSignature::kLocalFileHeader)
					return S_FALSE;
				ReadLocalItem(localItem);
				if(!AreItemsEqual(localItem, item))
					return S_FALSE;
				item.LocalFullHeaderSize = localItem.LocalFullHeaderSize;
				item.LocalExtra = localItem.LocalExtra;
				if(item.Crc != localItem.Crc && !localItem.HasDescriptor()) {
					item.Crc = localItem.Crc;
					headersError = true;
				}
				item.FromLocal = true;
			}
			catch(...) { return S_FALSE; }
			return S_OK;
		}

		/*
		   ---------- FindDescriptor ----------

		   in:
		   _streamPos : position in Stream
		   Stream :
		   Vols : if(IsMultiVol)

		   action:
		   searches descriptor in input stream(s).
		   sets
			item.DescriptorWasRead = true;
			item.Size
			item.PackSize
			item.Crc
		   if descriptor was found

		   out:
		   S_OK:
			  if( item.DescriptorWasRead) : if descriptor was found
			  if(!item.DescriptorWasRead) : if descriptor was not found : unexpected end of stream(s)

		   S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

		   another error code: Callback error.

		   exceptions :
		   CSystemException() : stream reading error
		 */

		HRESULT CInArchive::FindDescriptor(CItemEx &item, unsigned numFiles)
		{
			// const size_t kBufSize = (size_t)1 << 5; // don't increase it too much. It reads data look ahead.
			// Buffer.Alloc(kBufSize);
			// Byte *buf = Buffer;
			uint64 packedSize = 0;
			uint64 progressPrev = _cnt;
			for(;; ) {
				/* appnote specification claims that we must use 64-bit descriptor, if there is zip64 extra.
				   But some old third-party xps archives used 64-bit descriptor without zip64 extra. */
				// unsigned descriptorSize = kDataDescriptorSize64 + kNextSignatureSize;

				// const uint kNextSignatureSize = 0;  // we can disable check for next signatuire
				const uint kNextSignatureSize = 4; // we check also for signature for next File headear

				const uint descriptorSize4 = item.GetDescriptorSize() + kNextSignatureSize;

				if(descriptorSize4 > Buffer.Size()) return E_FAIL;

				// size_t processedSize;
				CanStartNewVol = true;
				RINOK(LookAhead(descriptorSize4));
				const size_t avail = GetAvail();

				if(avail < descriptorSize4) {
					// we write to packSize all these available bytes.
					// later it's simpler to work with such value than with 0
					if(item.PackSize == 0)
						item.PackSize = packedSize + avail;
					return S_OK;
				}
				const Byte * const pStart = Buffer + _bufPos;
				const Byte * p = pStart;
				const Byte * const limit = pStart + (avail - descriptorSize4);
				for(; p <= limit; p++) {
					// descriptor signature field is Info-ZIP's extension to pkware Zip specification.
					// New ZIP specification also allows descriptorSignature.

					p = FindPK(p, limit + 1);
					if(p > limit)
						break;
					/*
					   if(*p != 0x50)
					   continue;
					 */
					if(Get32(p) != NSignature::kDataDescriptor)
						continue;
					// we check next signatuire after descriptor
					// maybe we need check only 2 bytes "PK" instead of 4 bytes, if some another type of header is
					// possible after descriptor
					const uint32 sig = Get32(p + descriptorSize4 - kNextSignatureSize);
					if(sig != NSignature::kLocalFileHeader && sig != NSignature::kCentralFileHeader)
						continue;
					const uint64 packSizeCur = packedSize + (p - pStart);
					if(descriptorSize4 == kDataDescriptorSize64 + kNextSignatureSize) { // if(item.LocalExtra.IsZip64)
						const uint64 descriptorPackSize = Get64(p + 8);
						if(descriptorPackSize != packSizeCur)
							continue;
						item.Size = Get64(p + 16);
					}
					else {
						const uint32 descriptorPackSize = Get32(p + 8);
						if(descriptorPackSize != (uint32)packSizeCur)
							continue;
						item.Size = Get32(p + 12);
						// that item.Size can be truncated to 32-bit value here
					}
					// We write calculated 64-bit packSize, even if descriptor64 was not used
					item.PackSize = packSizeCur;
					item.DescriptorWasRead = true;
					item.Crc = Get32(p + 4);
					const size_t skip = (p - pStart) + descriptorSize4 - kNextSignatureSize;
					SkipLookahed(skip);
					return S_OK;
				}
				const size_t skip = (p - pStart);
				SkipLookahed(skip);
				packedSize += skip;
				if(Callback)
					if(_cnt - progressPrev >= ((uint32)1 << 22)) {
						progressPrev = _cnt;
						const uint64 numFiles64 = numFiles;
						RINOK(Callback->SetCompleted(&numFiles64, &_cnt));
					}
			}
		}

		bool CInArchive::AreThereErrors() const { return HeadersError || UnexpectedEnd || !Vols.MissingName.IsEmpty(); }
		bool CInArchive::IsLocalOffsetOK(const CItemEx &item) const { return item.FromLocal ? true : ((int64)GetOffset() + (int64)item.LocalHeaderPos >= 0); }

		uint64 CInArchive::GetEmbeddedStubSize() const
		{
			if(ArcInfo.CdWasRead)
				return ArcInfo.FirstItemRelatOffset;
			else if(IsMultiVol)
				return 0;
			else
				return (ArcInfo.MarkerPos2 - ArcInfo.Base);
		}

		bool CInArchive::CanUpdate() const
		{
			if(AreThereErrors() || IsMultiVol || ArcInfo.Base < 0 || (int64)ArcInfo.MarkerPos2 < ArcInfo.Base || ArcInfo.ThereIsTail || GetEmbeddedStubSize() != 0)
				return false;
			else {
				// 7-zip probably can update archives with embedded stubs.
				// we just disable that feature for more safety.
				return true;
			}
		}

		HRESULT CInArchive::CheckDescriptor(const CItemEx &item)
		{
			if(!item.HasDescriptor())
				return S_OK;
			// pkzip's version without descriptor signature is not supported
			bool isFinished = false;
			RINOK(IncreaseRealPosition(item.PackSize, isFinished));
			if(isFinished)
				return S_FALSE;
			/*
			   if(!IsMultiVol)
			   {
			   RINOK(Seek_SavePos(ArcInfo.Base + item.GetDataPosition() + item.PackSize));
			   }
			 */

			Byte buf[kDataDescriptorSize64];
			try {
				CanStartNewVol = true;
				SafeRead(buf, item.GetDescriptorSize());
			}
			catch(const CSystemException &e) { return e.ErrorCode; }
			// catch (const CUnexpectEnd &)
			catch(...) {
				return S_FALSE;
			}
			// RINOK(ReadStream_FALSE(Stream, buf, item.GetDescriptorSize()));

			if(Get32(buf) != NSignature::kDataDescriptor)
				return S_FALSE;
			uint32 crc = Get32(buf + 4);
			uint64 packSize, unpackSize;
			if(item.LocalExtra.IsZip64) {
				packSize = Get64(buf + 8);
				unpackSize = Get64(buf + 16);
			}
			else {
				packSize = Get32(buf + 8);
				unpackSize = Get32(buf + 12);
			}
			return (crc != item.Crc || item.PackSize != packSize || item.Size != unpackSize) ? S_FALSE : S_OK;
		}

		HRESULT CInArchive::ReadLocalItemAfterCdItemFull(CItemEx &item)
		{
			if(item.FromLocal)
				return S_OK;
			try {
				bool isAvail = true;
				bool headersError = false;
				RINOK(ReadLocalItemAfterCdItem(item, isAvail, headersError));
				if(headersError)
					return S_FALSE;
				if(item.HasDescriptor())
					return CheckDescriptor(item);
			}
			catch(...) { return S_FALSE; }
			return S_OK;
		}

		HRESULT CInArchive::ReadCdItem(CItemEx &item)
		{
			item.FromCentral = true;
			Byte p[kCentralHeaderSize - 4];
			SafeRead(p, kCentralHeaderSize - 4);

			item.MadeByVersion.Version = p[0];
			item.MadeByVersion.HostOS = p[1];
			item.ExtractVersion.Version = p[2];
			item.ExtractVersion.HostOS = p[3];
			G16(4, item.Flags);
			G16(6, item.Method);
			G32(8, item.Time);
			G32(12, item.Crc);
			G32(16, item.PackSize);
			G32(20, item.Size);
			const uint nameSize = Get16(p + 24);
			const uint extraSize = Get16(p + 26);
			const uint commentSize = Get16(p + 28);
			G16(30, item.Disk);
			G16(32, item.InternalAttrib);
			G32(34, item.ExternalAttrib);
			G32(38, item.LocalHeaderPos);
			ReadFileName(nameSize, item.Name);

			if(extraSize > 0)
				ReadExtra(extraSize, item.CentralExtra, item.Size, item.PackSize, item.LocalHeaderPos, item.Disk);

			// May be these strings must be deleted
			/*
			   if(item.IsDir())
			   item.Size = 0;
			 */

			ReadBuffer(item.Comment, commentSize);
			return S_OK;
		}

		HRESULT CInArchive::TryEcd64(uint64 offset, CCdInfo &cdInfo)
		{
			if(offset >= ((uint64)1 << 63))
				return S_FALSE;
			Byte buf[kEcd64_FullSize];
			RINOK(SeekToVol(Vols.StreamIndex, offset));
			unsigned processed = 0;
			ReadFromCache(buf, kEcd64_FullSize, processed);
			if(processed != kEcd64_FullSize)
				return S_FALSE;
			if(Get32(buf) != NSignature::kEcd64)
				return S_FALSE;
			uint64 mainSize = Get64(buf + 4);
			if(mainSize < kEcd64_MainSize || mainSize > ((uint64)1 << 40))
				return S_FALSE;
			cdInfo.ParseEcd64e(buf + 12);
			return S_OK;
		}

		HRESULT CInArchive::FindCd(bool checkOffsetMode)
		{
			CCdInfo &cdInfo = Vols.ecd;
			uint64 endPos;
			// There are no useful data in cache in most cases here.
			// So here we don't use cache data from previous operations .
			InitBuf();
			RINOK(Stream->Seek(0, STREAM_SEEK_END, &endPos));
			_streamPos = endPos;
			// const uint32 kBufSizeMax2 = ((uint32)1 << 16) + kEcdSize + kEcd64Locator_Size + kEcd64_FullSize;
			const size_t kBufSizeMax = ((size_t)1 << 17); // must be larger than kBufSizeMax2
			const size_t bufSize = (endPos < kBufSizeMax) ? (size_t)endPos : kBufSizeMax;
			if(bufSize < kEcdSize)
				return S_FALSE;
			// CByteArr byteBuffer(bufSize);
			if(Buffer.Size() < kBufSizeMax) {
				// InitBuf();
				Buffer.AllocAtLeast(kBufSizeMax);
				if(!Buffer.IsAllocated())
					return E_OUTOFMEMORY;
			}
			RINOK(Seek_SavePos(endPos - bufSize));
			size_t processed = bufSize;
			HRESULT res = ReadStream(Stream, Buffer, &processed);
			_streamPos += processed;
			_bufCached = processed;
			_bufPos = 0;
			_cnt += processed;
			if(res != S_OK)
				return res;
			if(processed != bufSize)
				return S_FALSE;
			for(size_t i = bufSize - kEcdSize + 1;; ) {
				if(i == 0)
					return S_FALSE;
				const Byte * buf = Buffer;
				for(;; ) {
					i--;
					if(buf[i] == 0x50)
						break;
					if(i == 0)
						return S_FALSE;
				}
				if(Get32(buf + i) != NSignature::kEcd)
					continue;
				cdInfo.ParseEcd32(buf + i);
				if(i >= kEcd64Locator_Size) {
					const size_t locatorIndex = i - kEcd64Locator_Size;
					if(Get32(buf + locatorIndex) == NSignature::kEcd64Locator) {
						CLocator locator;
						locator.Parse(buf + locatorIndex + 4);
						if((cdInfo.ThisDisk == locator.NumDisks - 1 || ZIP64_IS_16_MAX(cdInfo.ThisDisk)) && locator.Ecd64Disk < locator.NumDisks) {
							if(locator.Ecd64Disk != cdInfo.ThisDisk && !ZIP64_IS_16_MAX(cdInfo.ThisDisk))
								return E_NOTIMPL;
							// Most of the zip64 use fixed size Zip64 ECD
							// we try relative backward reading.
							uint64 absEcd64 = endPos - bufSize + i - (kEcd64Locator_Size + kEcd64_FullSize);
							if(locatorIndex >= kEcd64_FullSize)
								if(checkOffsetMode || absEcd64 == locator.Ecd64Offset) {
									const Byte * ecd64 = buf + locatorIndex - kEcd64_FullSize;
									if(Get32(ecd64) == NSignature::kEcd64) {
										uint64 mainEcd64Size = Get64(ecd64 + 4);
										if(mainEcd64Size == kEcd64_MainSize) {
											cdInfo.ParseEcd64e(ecd64 + 12);
											ArcInfo.Base = absEcd64 - locator.Ecd64Offset;
											// ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
											return S_OK;
										}
									}
								}
							// some zip64 use variable size Zip64 ECD.
							// we try to use absolute offset from locator.
							if(absEcd64 != locator.Ecd64Offset) {
								if(TryEcd64(locator.Ecd64Offset, cdInfo) == S_OK) {
									ArcInfo.Base = 0;
									// ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
									return S_OK;
								}
							}
							// for variable Zip64 ECD with for archives with offset != 0.
							if(checkOffsetMode && ArcInfo.MarkerPos != 0 && ArcInfo.MarkerPos + locator.Ecd64Offset != absEcd64) {
								if(TryEcd64(ArcInfo.MarkerPos + locator.Ecd64Offset, cdInfo) == S_OK) {
									ArcInfo.Base = ArcInfo.MarkerPos;
									// ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
									return S_OK;
								}
							}
						}
					}
				}

				// bool isVolMode = (Vols.EndVolIndex != -1);
				// uint32 searchDisk = (isVolMode ? Vols.EndVolIndex : 0);

				if(/* searchDisk == thisDisk && */ cdInfo.CdDisk <= cdInfo.ThisDisk) {
					// if(isVolMode)
					{
						if(cdInfo.CdDisk != cdInfo.ThisDisk)
							return S_OK;
					}

					uint64 absEcdPos = endPos - bufSize + i;
					uint64 cdEnd = cdInfo.Size + cdInfo.Offset;
					ArcInfo.Base = 0;
					// ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
					if(absEcdPos != cdEnd) {
						/*
						   if(cdInfo.Offset <= 16 && cdInfo.Size != 0)
						   {
						   // here we support some rare ZIP files with Central directory at the start
						   ArcInfo.Base = 0;
						   }
						   else
						 */
						ArcInfo.Base = absEcdPos - cdEnd;
					}
					return S_OK;
				}
			}
		}

		HRESULT CInArchive::TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, uint64 cdOffset, uint64 cdSize)
		{
			items.Clear();
			RINOK(SeekToVol(IsMultiVol ? cdInfo.CdDisk : -1, cdOffset));
			_inBufMode = true;
			_cnt = 0;
			if(Callback) {
				RINOK(Callback->SetTotal(&cdInfo.NumEntries, IsMultiVol ? &Vols.TotalBytesSize : NULL));
			}
			uint64 numFileExpected = cdInfo.NumEntries;
			const uint64 * totalFilesPtr = &numFileExpected;
			bool isCorrect_NumEntries = (cdInfo.IsFromEcd64 || numFileExpected >= ((uint32)1 << 16));

			while(_cnt < cdSize) {
				CanStartNewVol = true;
				if(ReadUInt32() != NSignature::kCentralFileHeader)
					return S_FALSE;
				CanStartNewVol = false;
				{
					CItemEx cdItem;
					RINOK(ReadCdItem(cdItem));
					items.Add(cdItem);
				}
				if(Callback && (items.Size() & 0xFFF) == 0) {
					const uint64 numFiles = items.Size();
					if(numFiles > numFileExpected && totalFilesPtr) {
						if(isCorrect_NumEntries)
							totalFilesPtr = NULL;
						else
							while(numFiles > numFileExpected)
								numFileExpected += (uint32)1 << 16;
						RINOK(Callback->SetTotal(totalFilesPtr, NULL));
					}
					RINOK(Callback->SetCompleted(&numFiles, &_cnt));
				}
			}
			CanStartNewVol = true;
			return (_cnt == cdSize) ? S_OK : S_FALSE;
		}

		HRESULT CInArchive::ReadCd(CObjectVector<CItemEx> &items, uint32 &cdDisk, uint64 &cdOffset, uint64 &cdSize)
		{
			bool checkOffsetMode = true;
			if(IsMultiVol) {
				if(Vols.EndVolIndex == -1)
					return S_FALSE;
				Stream = Vols.Streams[Vols.EndVolIndex].Stream;
				if(!Vols.StartIsZip)
					checkOffsetMode = false;
			}
			else
				Stream = StartStream;
			if(!Vols.ecd_wasRead) {
				RINOK(FindCd(checkOffsetMode));
			}
			CCdInfo &cdInfo = Vols.ecd;
			HRESULT res = S_FALSE;
			cdSize = cdInfo.Size;
			cdOffset = cdInfo.Offset;
			cdDisk = cdInfo.CdDisk;
			if(!IsMultiVol) {
				if(cdInfo.ThisDisk != cdInfo.CdDisk)
					return S_FALSE;
			}
			const uint64 base = (IsMultiVol ? 0 : ArcInfo.Base);
			res = TryReadCd(items, cdInfo, base + cdOffset, cdSize);
			if(res == S_FALSE && !IsMultiVol && base != ArcInfo.MarkerPos) {
				// do we need that additional attempt to read cd?
				res = TryReadCd(items, cdInfo, ArcInfo.MarkerPos + cdOffset, cdSize);
				if(res == S_OK)
					ArcInfo.Base = ArcInfo.MarkerPos;
			}
			return res;
		}

		static int FASTCALL FindItem(const CObjectVector<CItemEx> &items, const CItemEx &item)
		{
			for(uint left = 0, right = items.Size();;) {
				if(left >= right)
					return -1;
				uint   index = (left + right) / 2;
				const  CItemEx & item2 = items[index];
				if(item.Disk < item2.Disk)
					right = index;
				else if(item.Disk > item2.Disk)
					left = index + 1;
				else if(item.LocalHeaderPos == item2.LocalHeaderPos)
					return index;
				else if(item.LocalHeaderPos < item2.LocalHeaderPos)
					right = index;
				else
					left = index + 1;
			}
		}

		static bool FASTCALL IsStrangeItem(const CItem &item)
		{
			return item.Name.Len() > (1 << 14) || item.Method > (1 << 8);
		}
		/*
		   ---------- ReadLocals ----------

		   in:
		   (_signature == NSignature::kLocalFileHeader)
		   VirtStreamPos : after _signature : position in Stream
		   Stream :
		   Vols : if(IsMultiVol)
		   (_inBufMode == false)

		   action:
		   it parses local items.

		   if( IsMultiVol) it writes absolute offsets to CItemEx::LocalHeaderPos
		   if(!IsMultiVol) it writes relative (from ArcInfo.Base) offsets to CItemEx::LocalHeaderPos
					   later we can correct CItemEx::LocalHeaderPos values, if
					   some new value for ArcInfo.Base will be detected
		   out:
		   S_OK:
			(_signature != NSignature::kLocalFileHeade)
			_streamPos : after _signature

		   S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

		   another error code: stream reading error or Callback error.

		   CUnexpectEnd() exception : it's not fatal exception here.
			  It means that reading was interrupted by unexpected end of input stream,
			  but some CItemEx items were parsed OK.
			  We can stop further archive parsing.
			  But we can use all filled CItemEx items.
		 */

		HRESULT CInArchive::ReadLocals(CObjectVector<CItemEx> &items)
		{
			items.Clear();
			uint64 progressPrev = _cnt;
			if(Callback) {
				RINOK(Callback->SetTotal(NULL, IsMultiVol ? &Vols.TotalBytesSize : NULL));
			}
			while(_signature == NSignature::kLocalFileHeader) {
				CItemEx item;
				item.LocalHeaderPos = GetVirtStreamPos() - 4;
				if(!IsMultiVol)
					item.LocalHeaderPos -= ArcInfo.Base;
				try {
					ReadLocalItem(item);
					item.FromLocal = true;
					bool isFinished = false;
					if(item.HasDescriptor()) {
						RINOK(FindDescriptor(item, items.Size()));
						isFinished = !item.DescriptorWasRead;
					}
					else {
						if(item.PackSize >= ((uint64)1 << 62))
							throw CUnexpectEnd();
						RINOK(IncreaseRealPosition(item.PackSize, isFinished));
					}
					items.Add(item);
					if(isFinished)
						throw CUnexpectEnd();
					ReadSignature();
				}
				catch(CUnexpectEnd &) {
					if(items.IsEmpty() || items.Size() == 1 && IsStrangeItem(items[0]))
						return S_FALSE;
					throw;
				}
				if(Callback)
					if((items.Size() & 0xFF) == 0 || _cnt - progressPrev >= ((uint32)1 << 22)) {
						progressPrev = _cnt;
						const uint64 numFiles = items.Size();
						RINOK(Callback->SetCompleted(&numFiles, &_cnt));
					}
			}

			if(items.Size() == 1 && _signature != NSignature::kCentralFileHeader)
				if(IsStrangeItem(items[0]))
					return S_FALSE;

			return S_OK;
		}

		CInArchive::CVols::CSubStreamInfo::CSubStreamInfo() : Size(0) 
		{
		}

		HRESULT CInArchive::CVols::CSubStreamInfo::SeekToStart() const { return Stream->Seek(0, STREAM_SEEK_SET, NULL); }

		void CInArchive::CVols::ClearRefs()
		{
			Streams.Clear();
			ZipStream.Release();
			TotalBytesSize = 0;
		}

		void CInArchive::CVols::Clear()
		{
			StreamIndex = -1;
			NeedSeek = false;
			StartIsExe = false;
			StartIsZ = false;
			StartIsZip = false;
			IsUpperCase = false;
			StartVolIndex = -1;
			StartParsingVol = 0;
			NumVols = 0;
			EndVolIndex = -1;
			BaseName.Empty();
			MissingName.Empty();
			MissingZip = false;
			ecd_wasRead = false;
			ClearRefs();
		}

		HRESULT CInArchive::CVols::ParseArcName(IArchiveOpenVolumeCallback * volCallback)
		{
			UString name;
			{
				NWindows::NCOM::CPropVariant prop;
				RINOK(volCallback->GetProperty(kpidName, &prop));
				if(prop.vt != VT_BSTR)
					return S_OK;
				name = prop.bstrVal;
			}
			int dotPos = name.ReverseFind_Dot();
			if(dotPos < 0)
				return S_OK;
			const UString ext = name.Ptr(dotPos + 1);
			name.DeleteFrom(dotPos + 1);
			StartVolIndex = (int32)(-1);
			if(ext.IsEmpty())
				return S_OK;
			{
				wchar_t c = ext[0];
				IsUpperCase = (c >= 'A' && c <= 'Z');
				if(ext.IsEqualTo_Ascii_NoCase("zip")) {
					BaseName = name;
					StartIsZ = true;
					StartIsZip = true;
					return S_OK;
				}
				else if(ext.IsEqualTo_Ascii_NoCase("exe")) {
					StartIsExe = true;
					BaseName = name;
					StartVolIndex = 0;
					/* sfx-zip can use both arc.exe and arc.zip
					   We can open arc.zip, if it was requesed to open arc.exe.
					   But it's possible that arc.exe and arc.zip are not parts of same archive.
					   So we can disable such operation */
					return S_FALSE; // don't open arc.zip instead of arc.exe
				}
				else if(ext[0] == 'z' || ext[0] == 'Z') {
					if(ext.Len() < 3)
						return S_OK;
					const wchar_t * end = NULL;
					uint32 volNum = ConvertStringToUInt32(ext.Ptr(1), &end);
					if(*end != 0 || volNum < 1 || volNum > ((uint32)1 << 30))
						return S_OK;
					StartVolIndex = volNum - 1;
					BaseName = name;
					StartIsZ = true;
				}
				else
					return S_OK;
			}
			UString volName = BaseName;
			volName += (IsUpperCase ? "ZIP" : "zip");
			HRESULT res = volCallback->GetStream(volName, &ZipStream);
			if(res == S_FALSE || !ZipStream) {
				if(MissingName.IsEmpty()) {
					MissingZip = true;
					MissingName = volName;
				}
				return S_OK;
			}
			return res;
		}

		HRESULT CInArchive::ReadVols2(IArchiveOpenVolumeCallback * volCallback,
			unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols)
		{
			numMissingVols = 0;
			for(uint i = start;; i++) {
				if(lastDisk >= 0 && i >= (uint)lastDisk)
					break;
				if(i < Vols.Streams.Size())
					if(Vols.Streams[i].Stream)
						continue;
				CMyComPtr<IInStream> stream;
				if((int)i == zipDisk) {
					stream = Vols.ZipStream;
				}
				else if((int)i == Vols.StartVolIndex) {
					stream = StartStream;
				}
				else {
					UString volName = Vols.BaseName;
					{
						volName += (char)(Vols.IsUpperCase ? 'Z' : 'z');
						unsigned v = i + 1;
						if(v < 10)
							volName += '0';
						volName.Add_UInt32(v);
					}
					HRESULT res = volCallback->GetStream(volName, &stream);
					if(res != S_OK && res != S_FALSE)
						return res;
					if(res == S_FALSE || !stream) {
						if(i == 0) {
							UString volName_exe = Vols.BaseName;
							volName_exe += (Vols.IsUpperCase ? "EXE" : "exe");
							HRESULT res2 = volCallback->GetStream(volName_exe, &stream);
							if(res2 != S_OK && res2 != S_FALSE)
								return res2;
							res = res2;
						}
					}
					if(res == S_FALSE || !stream) {
						if(Vols.MissingName.IsEmpty())
							Vols.MissingName = volName;
						numMissingVols++;
						if(numMissingVols > numMissingVolsMax)
							return S_OK;
						if(lastDisk == -1 && numMissingVols != 0)
							return S_OK;
						continue;
					}
				}

				uint64 size;
				uint64 pos;
				RINOK(stream->Seek(0, STREAM_SEEK_CUR, &pos));
				RINOK(stream->Seek(0, STREAM_SEEK_END, &size));
				RINOK(stream->Seek(pos, STREAM_SEEK_SET, NULL));

				while(i >= Vols.Streams.Size())
					Vols.Streams.AddNew();

				CVols::CSubStreamInfo &ss = Vols.Streams[i];
				Vols.NumVols++;
				Vols.TotalBytesSize += size;

				ss.Stream = stream;
				ss.Size = size;

				if((int)i == zipDisk) {
					Vols.EndVolIndex = Vols.Streams.Size() - 1;
					break;
				}
			}

			return S_OK;
		}

		HRESULT CInArchive::ReadVols()
		{
			CMyComPtr <IArchiveOpenVolumeCallback> volCallback;
			Callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volCallback);
			if(!volCallback)
				return S_OK;
			RINOK(Vols.ParseArcName(volCallback));
			// const int startZIndex = Vols.StartVolIndex;
			if(!Vols.StartIsZ) {
				if(!Vols.StartIsExe)
					return S_OK;
			}
			int zipDisk = -1;
			int cdDisk = -1;
			if(Vols.StartIsZip)
				Vols.ZipStream = StartStream;
			if(Vols.ZipStream) {
				Stream = Vols.ZipStream;
				if(Vols.StartIsZip)
					Vols.StreamIndex = -1;
				else {
					Vols.StreamIndex = -2;
					InitBuf();
				}
				HRESULT res = FindCd(true);
				CCdInfo &ecd = Vols.ecd;
				if(res == S_OK) {
					zipDisk = ecd.ThisDisk;
					Vols.ecd_wasRead = true;
					// if is not multivol or bad multivol, we return to main single stream code
					if(ecd.ThisDisk == 0 || ecd.ThisDisk >= ((uint32)1 << 30) || ecd.ThisDisk < ecd.CdDisk)
						return S_OK;
					cdDisk = ecd.CdDisk;
					if(Vols.StartVolIndex < 0)
						Vols.StartVolIndex = ecd.ThisDisk;
					else if((uint32)Vols.StartVolIndex >= ecd.ThisDisk)
						return S_OK;

					// Vols.StartVolIndex = ecd.ThisDisk;
					// Vols.EndVolIndex = ecd.ThisDisk;
					unsigned numMissingVols;
					if(cdDisk != zipDisk) {
						// get volumes required for cd.
						RINOK(ReadVols2(volCallback, cdDisk, zipDisk, zipDisk, 0, numMissingVols));
						if(numMissingVols != 0) {
							// cdOK = false;
						}
					}
				}
				else if(res != S_FALSE)
					return res;
			}
			if(Vols.StartVolIndex < 0) {
				// is not mutivol;
				return S_OK;
			}
			/*
			   if(!Vols.Streams.IsEmpty())
			   IsMultiVol = true;
			 */
			uint   numMissingVols;
			if(cdDisk != 0) {
				// get volumes that were no requested still
				const uint kNumMissingVolsMax = 1 << 12;
				RINOK(ReadVols2(volCallback, 0, cdDisk < 0 ? -1 : cdDisk, zipDisk, kNumMissingVolsMax, numMissingVols));
			}
			// if(Vols.StartVolIndex >= 0)
			{
				if(Vols.Streams.IsEmpty())
					if(Vols.StartVolIndex > (1 << 20))
						return S_OK;
				if((uint)Vols.StartVolIndex >= Vols.Streams.Size() || !Vols.Streams[Vols.StartVolIndex].Stream) {
					// we get volumes starting from StartVolIndex, if they we not requested before know the volume
					// index (if FindCd() was ok)
					RINOK(ReadVols2(volCallback, Vols.StartVolIndex, zipDisk, zipDisk, 0, numMissingVols));
				}
			}

			if(Vols.ZipStream) {
				// if there is no another volumes and volumeIndex is too big, we don't use multivol mode
				if(Vols.Streams.IsEmpty())
					if(zipDisk > (1 << 10))
						return S_OK;
				if(zipDisk >= 0) {
					// we create item in Streams for ZipStream, if we know the volume index (if FindCd() was ok)
					RINOK(ReadVols2(volCallback, zipDisk, zipDisk + 1, zipDisk, 0, numMissingVols));
				}
			}

			if(!Vols.Streams.IsEmpty()) {
				IsMultiVol = true;
				/*
				   if(cdDisk)
				   IsMultiVol = true;
				 */
				const int startZIndex = Vols.StartVolIndex;
				if(startZIndex >= 0) {
					// if all volumes before start volume are OK, we can start parsing from 0
					// if there are missing volumes before startZIndex, we start parsing in current startZIndex
					if((uint)startZIndex < Vols.Streams.Size()) {
						for(uint i = 0; i <= (uint)startZIndex; i++)
							if(!Vols.Streams[i].Stream) {
								Vols.StartParsingVol = startZIndex;
								break;
							}
					}
				}
			}
			return S_OK;
		}

		HRESULT CInArchive::CVols::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			if(size == 0)
				return S_OK;
			for(;; ) {
				if(StreamIndex < 0)
					return S_OK;
				if((uint)StreamIndex >= Streams.Size())
					return S_OK;
				const CVols::CSubStreamInfo &s = Streams[StreamIndex];
				if(!s.Stream)
					return S_FALSE;
				if(NeedSeek) {
					RINOK(s.SeekToStart());
					NeedSeek = false;
				}
				uint32 realProcessedSize = 0;
				HRESULT res = s.Stream->Read(data, size, &realProcessedSize);
				ASSIGN_PTR(processedSize, realProcessedSize);
				if(res != S_OK)
					return res;
				if(realProcessedSize != 0)
					return res;
				StreamIndex++;
				NeedSeek = true;
			}
		}

		#define COPY_ECD_ITEM_16(n) if(!isZip64 || !ZIP64_IS_16_MAX(ecd.n)) cdInfo.n = ecd.n;
		#define COPY_ECD_ITEM_32(n) if(!isZip64 || !ZIP64_IS_32_MAX(ecd.n)) cdInfo.n = ecd.n;

		HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items)
		{
			if(Buffer.Size() < kSeqBufferSize) {
				InitBuf();
				Buffer.AllocAtLeast(kSeqBufferSize);
				if(!Buffer.IsAllocated())
					return E_OUTOFMEMORY;
			}
			_inBufMode = false;
			HRESULT res = S_OK;
			bool localsWereRead = false;

			/* we try to open archive with the following modes:
			   1) CD-MODE        : fast mode : we read backward ECD and CD, compare CD items with first Local item.
			   2) LOCALS-CD-MODE : slow mode, if CD-MODE fails : we sequentially read all Locals and then CD.
			   Then we read sequentially ECD64, Locator, ECD again at the end.

			   - in LOCALS-CD-MODE we use use the following
				   variables (with real cd properties) to set Base archive offset
				   and check real cd properties with values from ECD/ECD64.
			 */
			uint64 cdSize = 0;
			uint64 cdRelatOffset = 0;
			uint32 cdDisk = 0;
			uint64 cdAbsOffset = 0; // absolute cd offset, for LOCALS-CD-MODE only.
			if(!MarkerIsFound || !MarkerIsSafe) {
				IsArc = true;
				res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
				if(res == S_OK)
					ReadSignature();
				else if(res != S_FALSE)
					return res;
			}
			else {
				// _signature must be kLocalFileHeader or kEcd or kEcd64
				SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2 + 4);
				CanStartNewVol = false;
				if(_signature == NSignature::kEcd64) {
					// uint64 ecd64Offset = GetVirtStreamPos() - 4;
					IsZip64 = true;
					{
						const uint64 recordSize = ReadUInt64();
						if(recordSize < kEcd64_MainSize)
							return S_FALSE;
						if(recordSize >= ((uint64)1 << 62))
							return S_FALSE;
						{
							const uint kBufSize = kEcd64_MainSize;
							Byte buf[kBufSize];
							SafeRead(buf, kBufSize);
							CCdInfo cdInfo;
							cdInfo.ParseEcd64e(buf);
							if(!cdInfo.IsEmptyArc())
								return S_FALSE;
						}
						RINOK(Skip64(recordSize - kEcd64_MainSize, 0));
					}
					ReadSignature();
					if(_signature != NSignature::kEcd64Locator)
						return S_FALSE;
					{
						const uint kBufSize = 16;
						Byte buf[kBufSize];
						SafeRead(buf, kBufSize);
						CLocator locator;
						locator.Parse(buf);
						if(!locator.IsEmptyArc())
							return S_FALSE;
					}
					ReadSignature();
					if(_signature != NSignature::kEcd)
						return S_FALSE;
				}
				if(_signature == NSignature::kEcd) {
					// It must be empty archive or backware archive
					// we don't support backware archive still
					const uint kBufSize = kEcdSize - 4;
					Byte buf[kBufSize];
					SafeRead(buf, kBufSize);
					CEcd ecd;
					ecd.Parse(buf);
					// if(ecd.cdSize != 0)
					// Do we need also to support the case where empty zip archive with PK00 uses cdOffset = 4 ??
					if(!ecd.IsEmptyArc())
						return S_FALSE;
					ArcInfo.Base = ArcInfo.MarkerPos;
					IsArc = true; // check it: we need more tests?
					RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2));
					ReadSignature();
				}
				else {
					CItemEx firstItem;
					try {
						try {
							if(!ReadLocalItem(firstItem))
								return S_FALSE;
						}
						catch(CUnexpectEnd &) {
							return S_FALSE;
						}
						IsArc = true;
						res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
						if(res == S_OK)
							ReadSignature();
					}
					catch(CUnexpectEnd &) { res = S_FALSE; }
					if(res != S_FALSE && res != S_OK)
						return res;
					if(res == S_OK && items.Size() == 0)
						res = S_FALSE;
					if(res == S_OK) {
						// we can't read local items here to keep _inBufMode state
						if((int64)ArcInfo.MarkerPos2 < ArcInfo.Base)
							res = S_FALSE;
						else {
							firstItem.LocalHeaderPos = ArcInfo.MarkerPos2 - ArcInfo.Base;
							int index = FindItem(items, firstItem);
							if(index == -1)
								res = S_FALSE;
							else if(!AreItemsEqual(firstItem, items[index]))
								res = S_FALSE;
							else {
								ArcInfo.CdWasRead = true;
								ArcInfo.FirstItemRelatOffset = items[0].LocalHeaderPos;
							}
						}
					}
				}
			}
			CObjectVector <CItemEx> cdItems;
			bool   needSetBase = false; // we set needSetBase only for LOCALS_CD_MODE
			uint   numCdItems = items.Size();
		  #ifdef ZIP_SELF_CHECK
			res = S_FALSE; // if uncommented, it uses additional LOCALS-CD-MODE mode to check the code
		  #endif
			if(res != S_OK) {
				// ---------- LOCALS-CD-MODE ----------
				// CD doesn't match firstItem,
				// so we clear items and read Locals and CD.

				items.Clear();
				localsWereRead = true;

				// we can use any mode: with buffer and without buffer
				//   without buffer : skips packed data : fast for big files : slow for small files
				//   with    buffer : reads packed data : slow for big files : fast for small files

				_inBufMode = false;
				// _inBufMode = true;
				InitBuf();
				ArcInfo.Base = 0;
				if(!MarkerIsFound) {
					if(!IsMultiVol)
						return S_FALSE;
					if(Vols.StartParsingVol != 0)
						return S_FALSE;
					// if(StartParsingVol == 0) and we didn't find marker, we use default zero marker.
					// so we suppose that there is no sfx stub
					RINOK(SeekToVol(0, ArcInfo.MarkerPos2));
				}
				else {
					if(ArcInfo.MarkerPos != 0) {
						/*
						   If multi-vol or there is (No)Span-marker at start of stream, we set (Base) as 0.
						   In another caes:
						   (No)Span-marker is supposed as false positive. So we set (Base) as main marker
							  (MarkerPos2).
						   The (Base) can be corrected later after ECD reading.
						   But sfx volume with stub and (No)Span-marker in (!IsMultiVol) mode will have
							  incorrect (Base) here.
						 */
						ArcInfo.Base = ArcInfo.MarkerPos2;
					}
					RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2));
				}
				_cnt = 0;
				ReadSignature();
				LocalsWereRead = true;
				RINOK(ReadLocals(items));
				if(_signature != NSignature::kCentralFileHeader) {
					// GetVirtStreamPos() - 4
					if(items.IsEmpty())
						return S_FALSE;
					NoCentralDir = true;
					HeadersError = true;
					return S_OK;
				}
				_inBufMode = true;
				cdAbsOffset = GetVirtStreamPos() - 4;
				cdDisk = Vols.StreamIndex;
			#ifdef ZIP_SELF_CHECK
				if(!IsMultiVol && _cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
					return E_FAIL;
			#endif
				const uint64 processedCnt_start = _cnt;
				for(;; ) {
					CItemEx cdItem;
					RINOK(ReadCdItem(cdItem));
					cdItems.Add(cdItem);
					if(Callback && (cdItems.Size() & 0xFFF) == 0) {
						const uint64 numFiles = items.Size();
						const uint64 numBytes = _cnt;
						RINOK(Callback->SetCompleted(&numFiles, &numBytes));
					}
					ReadSignature();
					if(_signature != NSignature::kCentralFileHeader)
						break;
				}
				cdSize = _cnt - processedCnt_start;
			#ifdef ZIP_SELF_CHECK
				if(!IsMultiVol) {
					if(_cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
						return E_FAIL;
					if(cdSize != (GetVirtStreamPos() - 4) - cdAbsOffset)
						return E_FAIL;
				}
			#endif
				needSetBase = true;
				numCdItems = cdItems.Size();
				cdRelatOffset = cdAbsOffset - ArcInfo.Base;
				if(!cdItems.IsEmpty()) {
					ArcInfo.CdWasRead = true;
					ArcInfo.FirstItemRelatOffset = cdItems[0].LocalHeaderPos;
				}
			}
			CCdInfo cdInfo;
			CLocator locator;
			bool isZip64 = false;
			const uint64 ecd64AbsOffset = GetVirtStreamPos() - 4;
			int ecd64Disk = -1;
			if(_signature == NSignature::kEcd64) {
				ecd64Disk = Vols.StreamIndex;
				IsZip64 = isZip64 = true;
				{
					const uint64 recordSize = ReadUInt64();
					if(recordSize < kEcd64_MainSize || recordSize >= ((uint64)1 << 62)) {
						HeadersError = true;
						return S_OK;
					}
					{
						const uint kBufSize = kEcd64_MainSize;
						Byte buf[kBufSize];
						SafeRead(buf, kBufSize);
						cdInfo.ParseEcd64e(buf);
					}
					RINOK(Skip64(recordSize - kEcd64_MainSize, items.Size()));
				}
				ReadSignature();
				if(_signature != NSignature::kEcd64Locator) {
					HeadersError = true;
					return S_OK;
				}
				{
					const uint kBufSize = 16;
					Byte buf[kBufSize];
					SafeRead(buf, kBufSize);
					locator.Parse(buf);
				}

				ReadSignature();
			}

			if(_signature != NSignature::kEcd) {
				HeadersError = true;
				return S_OK;
			}
			CanStartNewVol = false;
			//
			// ECD
			//
			CEcd ecd;
			{
				const uint kBufSize = kEcdSize - 4;
				Byte buf[kBufSize];
				SafeRead(buf, kBufSize);
				ecd.Parse(buf);
			}
			COPY_ECD_ITEM_16(ThisDisk);
			COPY_ECD_ITEM_16(CdDisk);
			COPY_ECD_ITEM_16(NumEntries_in_ThisDisk);
			COPY_ECD_ITEM_16(NumEntries);
			COPY_ECD_ITEM_32(Size);
			COPY_ECD_ITEM_32(Offset);
			bool cdOK = true;
			if((uint32)cdInfo.Size != (uint32)cdSize) {
				// return S_FALSE;
				cdOK = false;
			}
			if(isZip64) {
				if(cdInfo.NumEntries != numCdItems || cdInfo.Size != cdSize) {
					cdOK = false;
				}
			}

			if(IsMultiVol) {
				if(cdDisk != (int)cdInfo.CdDisk)
					HeadersError = true;
			}
			else if(needSetBase && cdOK) {
				const uint64 oldBase = ArcInfo.Base;
				// localsWereRead == true
				// ArcInfo.Base == ArcInfo.MarkerPos2
				// cdRelatOffset == (cdAbsOffset - ArcInfo.Base)

				if(isZip64) {
					if(ecd64Disk == Vols.StartVolIndex) {
						const int64 newBase = (int64)ecd64AbsOffset - locator.Ecd64Offset;
						if(newBase <= (int64)ecd64AbsOffset) {
							if(!localsWereRead || newBase <= (int64)ArcInfo.MarkerPos2) {
								ArcInfo.Base = newBase;
								cdRelatOffset = cdAbsOffset - newBase;
							}
							else
								cdOK = false;
						}
					}
				}
				else if(numCdItems != 0) { // we can't use ecd.Offset in empty archive?
					if((int)cdDisk == Vols.StartVolIndex) {
						const int64 newBase = (int64)cdAbsOffset - cdInfo.Offset;
						if(newBase <= (int64)cdAbsOffset) {
							if(!localsWereRead || newBase <= (int64)ArcInfo.MarkerPos2) {
								// cd can be more accurate, when it points before Locals
								// so we change Base and cdRelatOffset
								ArcInfo.Base = newBase;
								cdRelatOffset = cdInfo.Offset;
							}
							else {
								// const uint64 delta = ((uint64)cdRelatOffset - cdInfo.Offset);
								const uint64 delta = ((uint64)(newBase - ArcInfo.Base));
								if((uint32)delta == 0) {
									// we set Overflow32bit mode, only if there is (x<<32) offset
									// between real_CD_offset_from_MarkerPos and CD_Offset_in_ECD.
									// Base and cdRelatOffset unchanged
									Overflow32bit = true;
								}
								else
									cdOK = false;
							}
						}
						else
							cdOK = false;
					}
				}
				// cdRelatOffset = cdAbsOffset - ArcInfo.Base;
				if(localsWereRead) {
					const uint64 delta = oldBase - ArcInfo.Base;
					if(delta != 0) {
						FOR_VECTOR(i, items)
						items[i].LocalHeaderPos += delta;
					}
				}
			}
			if(!cdOK)
				HeadersError = true;
			EcdVolIndex = cdInfo.ThisDisk;
			if(!IsMultiVol) {
				if(EcdVolIndex == 0 && Vols.MissingZip && Vols.StartIsExe) {
					Vols.MissingName.Empty();
					Vols.MissingZip = false;
				}
				if(localsWereRead) {
					if(EcdVolIndex != 0) {
						FOR_VECTOR(i, items) {
							items[i].Disk = EcdVolIndex;
						}
					}
				}
				UseDisk_in_SingleVol = true;
			}
			if(isZip64) {
				if(cdInfo.ThisDisk == 0 && ecd64AbsOffset != ArcInfo.Base + locator.Ecd64Offset
									// || cdInfo.NumEntries_in_ThisDisk != numCdItems
							|| cdInfo.NumEntries != numCdItems || cdInfo.Size != cdSize || (cdInfo.Offset != cdRelatOffset && !items.IsEmpty())) {
					HeadersError = true;
					return S_OK;
				}
			}

			if(cdOK && !cdItems.IsEmpty()) {
				// ---------- merge Central Directory Items ----------
				CRecordVector<unsigned> items2;
				int nextLocalIndex = 0;
				LocalsCenterMerged = true;
				FOR_VECTOR(i, cdItems) {
					if(Callback)
						if((i & 0x3FFF) == 0) {
							const uint64 numFiles64 = items.Size() + items2.Size();
							RINOK(Callback->SetCompleted(&numFiles64, &_cnt));
						}
					const CItemEx &cdItem = cdItems[i];
					int index = -1;
					if(nextLocalIndex != -1) {
						if((uint)nextLocalIndex < items.Size()) {
							CItemEx &item = items[nextLocalIndex];
							if(item.Disk == cdItem.Disk && (item.LocalHeaderPos == cdItem.LocalHeaderPos || Overflow32bit && (uint32)item.LocalHeaderPos == cdItem.LocalHeaderPos))
								index = nextLocalIndex++;
							else
								nextLocalIndex = -1;
						}
					}
					if(index == -1)
						index = FindItem(items, cdItem);
					// index = -1;
					if(index == -1) {
						items2.Add(i);
						HeadersError = true;
						continue;
					}
					CItemEx &item = items[index];
					if(item.Name != cdItem.Name
										// || item.Name.Len() != cdItem.Name.Len()
								|| item.PackSize != cdItem.PackSize || item.Size != cdItem.Size
										// item.ExtractVersion != cdItem.ExtractVersion
								|| !FlagsAreSame(item, cdItem) || item.Crc != cdItem.Crc) {
						HeadersError = true;
						continue;
					}

					// item.Name = cdItem.Name;
					item.MadeByVersion = cdItem.MadeByVersion;
					item.CentralExtra = cdItem.CentralExtra;
					item.InternalAttrib = cdItem.InternalAttrib;
					item.ExternalAttrib = cdItem.ExternalAttrib;
					item.Comment = cdItem.Comment;
					item.FromCentral = cdItem.FromCentral;
				}
				FOR_VECTOR(k, items2) {
					items.Add(cdItems[items2[k]]);
				}
			}
			if(ecd.NumEntries < ecd.NumEntries_in_ThisDisk)
				HeadersError = true;
			if(ecd.ThisDisk == 0) {
				// if(isZip64)
				{
					if(ecd.NumEntries != ecd.NumEntries_in_ThisDisk)
						HeadersError = true;
				}
			}
			if(isZip64) {
				if(cdInfo.NumEntries != items.Size() || ecd.NumEntries != items.Size() && ecd.NumEntries != 0xFFFF)
					HeadersError = true;
			}
			else {
				// old 7-zip could store 32-bit number of CD items to 16-bit field.
				// if(ecd.NumEntries != items.Size())
				if(ecd.NumEntries > items.Size())
					HeadersError = true;
				if(cdInfo.NumEntries != numCdItems) {
					if((uint16)cdInfo.NumEntries != (uint16)numCdItems)
						HeadersError = true;
					else
						Cd_NumEntries_Overflow_16bit = true;
				}
			}
			ReadBuffer(ArcInfo.Comment, ecd.CommentSize);
			_inBufMode = false;
			// DisableBufMode();
			// Buffer.Free();
			/* we can't clear buf varibles. we need them to calculate PhySize of archive */
			if((uint16)cdInfo.NumEntries != (uint16)numCdItems || (uint32)cdInfo.Size != (uint32)cdSize || ((uint32)cdInfo.Offset != (uint32)cdRelatOffset && !items.IsEmpty())) {
				// return S_FALSE;
				HeadersError = true;
			}

		  #ifdef ZIP_SELF_CHECK
			if(localsWereRead) {
				const uint64 endPos = ArcInfo.MarkerPos2 + _cnt;
				if(endPos != (IsMultiVol ? Vols.TotalBytesSize : ArcInfo.FileEndPos)) {
					// there are some data after the end of archive or error in code;
					return E_FAIL;
				}
			}
		  #endif
			// printf("\nOpen OK");
			return S_OK;
		}

		HRESULT CInArchive::Open(IInStream * stream, const uint64 * searchLimit, IArchiveOpenCallback * callback, CObjectVector<CItemEx> &items)
		{
			items.Clear();
			Close();
			uint64 startPos;
			RINOK(stream->Seek(0, STREAM_SEEK_CUR, &startPos));
			RINOK(stream->Seek(0, STREAM_SEEK_END, &ArcInfo.FileEndPos));
			_streamPos = ArcInfo.FileEndPos;
			StartStream = stream;
			Stream = stream;
			Callback = callback;
			DisableBufMode();
			bool volWasRequested = false;
			if(callback && (startPos == 0 || !searchLimit || *searchLimit != 0)) {
				// we try to read volumes only if it's first call (offset == 0) or scan is allowed.
				volWasRequested = true;
				RINOK(ReadVols());
			}
			if(IsMultiVol && Vols.StartParsingVol == 0 && (uint)Vols.StartParsingVol < Vols.Streams.Size()) {
				// only StartParsingVol = 0 is safe search.
				RINOK(SeekToVol(0, 0));
				// if(Stream)
				{
					// uint64 limit = 1 << 22; // for sfx
					uint64 limit = 0; // without sfx
					HRESULT res = FindMarker(&limit);
					if(res == S_OK) {
						MarkerIsFound = true;
						MarkerIsSafe = true;
					}
					else if(res != S_FALSE)
						return res;
				}
			}
			else {
				// printf("\nOpen offset = %u\n", (uint)startPos);
				if(IsMultiVol && (uint)Vols.StartParsingVol < Vols.Streams.Size() && Vols.Streams[Vols.StartParsingVol].Stream) {
					RINOK(SeekToVol(Vols.StartParsingVol, Vols.StreamIndex == Vols.StartVolIndex ? startPos : 0));
				}
				else {
					RINOK(SeekToVol(-1, startPos));
				}
				// uint64 limit = 1 << 22;
				// HRESULT res = FindMarker(&limit);
				HRESULT res = FindMarker(searchLimit);
				// const uint64 curPos = GetVirtStreamPos();
				const uint64 curPos = ArcInfo.MarkerPos2 + 4;
				if(res == S_OK)
					MarkerIsFound = true;
				else if(!IsMultiVol) {
					/*
					   // if(startPos != 0), probably CD copuld be already tested with another call with (startPos
						  == 0).
					   // so we don't want to try to open CD again in that ase.
					   if(startPos != 0)
					   return res;
					   // we can try to open CD, if there is no Marker and (startPos == 0).
					   // is it OK to open such files as ZIP, or big number of false positive, when CD can be find
						  in end of file ?
					 */
					return res;
				}

				if(ArcInfo.IsSpanMode && !volWasRequested) {
					RINOK(ReadVols());
					if(IsMultiVol && MarkerIsFound && ArcInfo.MarkerVolIndex < 0)
						ArcInfo.MarkerVolIndex = Vols.StartVolIndex;
				}
				MarkerIsSafe = !IsMultiVol || (ArcInfo.MarkerVolIndex == 0 && ArcInfo.MarkerPos == 0);
				if(IsMultiVol) {
					if((uint)Vols.StartVolIndex < Vols.Streams.Size()) {
						Stream = Vols.Streams[Vols.StartVolIndex].Stream;
						if(Stream) {
							RINOK(Seek_SavePos(curPos));
						}
						else
							IsMultiVol = false;
					}
					else
						IsMultiVol = false;
				}
				if(!IsMultiVol) {
					if(Vols.StreamIndex != -1) {
						Stream = StartStream;
						Vols.StreamIndex = -1;
						InitBuf();
						RINOK(Seek_SavePos(curPos));
					}
					ArcInfo.MarkerVolIndex = -1;
					StreamRef = stream;
					Stream = stream;
				}
			}
			if(!IsMultiVol)
				Vols.ClearRefs();
			{
				HRESULT res;
				try {
					res = ReadHeaders(items);
				}
				catch(const CSystemException &e) { res = e.ErrorCode; }
				catch(const CUnexpectEnd &) {
					if(items.IsEmpty())
						return S_FALSE;
					UnexpectedEnd = true;
					res = S_OK;
				}
				catch(...) {
					DisableBufMode();
					throw;
				}
				if(IsMultiVol) {
					ArcInfo.FinishPos = ArcInfo.FileEndPos;
					if((uint)Vols.StreamIndex < Vols.Streams.Size())
						if(GetVirtStreamPos() < Vols.Streams[Vols.StreamIndex].Size)
							ArcInfo.ThereIsTail = true;
				}
				else {
					ArcInfo.FinishPos = GetVirtStreamPos();
					ArcInfo.ThereIsTail = (ArcInfo.FileEndPos > ArcInfo.FinishPos);
				}
				DisableBufMode();
				IsArcOpen = true;
				if(!IsMultiVol)
					Vols.Streams.Clear();
				return res;
			}
		}

		HRESULT CInArchive::GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr<ISequentialInStream> &stream)
		{
			stream.Release();
			uint64 pos = item.LocalHeaderPos;
			if(seekPackData)
				pos += item.LocalFullHeaderSize;
			if(!IsMultiVol) {
				if(UseDisk_in_SingleVol && item.Disk != EcdVolIndex)
					return S_OK;
				pos += ArcInfo.Base;
				RINOK(StreamRef->Seek(pos, STREAM_SEEK_SET, NULL));
				stream = StreamRef;
				return S_OK;
			}
			if(item.Disk >= Vols.Streams.Size())
				return S_OK;
			IInStream * str2 = Vols.Streams[item.Disk].Stream;
			if(str2) {
				class CVolStream : public ISequentialInStream, public CMyUnknownImp {
				public:
					CVols * Vols;
					MY_UNKNOWN_IMP1(ISequentialInStream)
					STDMETHOD(Read) (void * data, uint32 size, uint32 * processedSize)
					{
						return Vols->Read(data, size, processedSize);
					}
				};
				RINOK(str2->Seek(pos, STREAM_SEEK_SET, NULL));
				Vols.NeedSeek = false;
				Vols.StreamIndex = item.Disk;
				CVolStream * volsStreamSpec = new CVolStream;
				volsStreamSpec->Vols = &Vols;
				stream = volsStreamSpec;
			}
			return S_OK;
		}

		HRESULT COutArchive::Create(IOutStream * outStream)
		{
			m_CurPos = 0;
			if(!m_OutBuffer.Create(1 << 16))
				return E_OUTOFMEMORY;
			m_Stream = outStream;
			m_OutBuffer.SetStream(outStream);
			m_OutBuffer.Init();
			return m_Stream->Seek(0, STREAM_SEEK_CUR, &m_Base);
		}

		void FASTCALL COutArchive::MoveCurPos(uint64 distanceToMove)
		{
			m_CurPos += distanceToMove;
		}

		void COutArchive::SeekToCurPos()
		{
			HRESULT res = m_Stream->Seek(m_Base + m_CurPos, STREAM_SEEK_SET, NULL);
			if(res != S_OK)
				throw CSystemException(res);
		}

		#define DOES_NEED_ZIP64(v) (v >= (uint32)0xFFFFFFFF)
		// #define DOES_NEED_ZIP64(v) (v >= 0)

		void COutArchive::WriteBytes(const void * data, size_t size)
		{
			m_OutBuffer.WriteBytes(data, size);
			m_CurPos += size;
		}
		void FASTCALL COutArchive::Write8(Byte b)
		{
			m_OutBuffer.WriteByte(b);
			m_CurPos++;
		}
		void FASTCALL COutArchive::Write16(uint16 val)
		{
			Write8((Byte)val);
			Write8((Byte)(val >> 8));
		}
		void FASTCALL COutArchive::Write32(uint32 val)
		{
			for(int i = 0; i < 4; i++) {
				Write8((Byte)val);
				val >>= 8;
			}
		}
		void FASTCALL COutArchive::Write64(uint64 val)
		{
			for(int i = 0; i < 8; i++) {
				Write8((Byte)val);
				val >>= 8;
			}
		}
		void FASTCALL COutArchive::WriteNtfsTime(const FILETIME &ft)
		{
			Write32(ft.dwLowDateTime);
			Write32(ft.dwHighDateTime);
		}
		void FASTCALL COutArchive::WriteExtra(const CExtraBlock &extra)
		{
			FOR_VECTOR(i, extra.SubBlocks) {
				const CExtraSubBlock &subBlock = extra.SubBlocks[i];
				Write16((uint16)subBlock.ID);
				Write16((uint16)subBlock.Data.Size());
				WriteBytes(subBlock.Data, (uint16)subBlock.Data.Size());
			}
		}
		void COutArchive::WriteCommonItemInfo(const CLocalItem &item, bool isZip64)
		{
			{
				Byte ver = item.ExtractVersion.Version;
				if(isZip64 && ver < NFileHeader::NCompressionMethod::kExtractVersion_Zip64)
					ver = NFileHeader::NCompressionMethod::kExtractVersion_Zip64;
				Write8(ver);
			}
			Write8(item.ExtractVersion.HostOS);
			Write16(item.Flags);
			Write16(item.Method);
			Write32(item.Time);
		}

		#define WRITE_32_VAL_SPEC(__v, __isZip64) Write32((__isZip64) ? 0xFFFFFFFF : (uint32)(__v));

		void COutArchive::WriteLocalHeader(CItemOut &item, bool needCheck)
		{
			m_LocalHeaderPos = m_CurPos;
			item.LocalHeaderPos = m_CurPos;
			bool isZip64 = DOES_NEED_ZIP64(item.PackSize) || DOES_NEED_ZIP64(item.Size);
			if(needCheck && m_IsZip64)
				isZip64 = true;
			const uint32 localExtraSize = (uint32)((isZip64 ? (4 + 8 + 8) : 0) + item.LocalExtra.GetSize());
			if((uint16)localExtraSize != localExtraSize)
				throw CSystemException(E_FAIL);
			if(needCheck && m_ExtraSize != localExtraSize)
				throw CSystemException(E_FAIL);
			m_IsZip64 = isZip64;
			m_ExtraSize = localExtraSize;
			item.LocalExtra.IsZip64 = isZip64;
			Write32(NSignature::kLocalFileHeader);
			WriteCommonItemInfo(item, isZip64);
			Write32(item.HasDescriptor() ? 0 : item.Crc);
			uint64 packSize = item.PackSize;
			uint64 size = item.Size;
			if(item.HasDescriptor()) {
				packSize = 0;
				size = 0;
			}
			WRITE_32_VAL_SPEC(packSize, isZip64);
			WRITE_32_VAL_SPEC(size, isZip64);
			Write16((uint16)item.Name.Len());
			Write16((uint16)localExtraSize);
			WriteBytes((const char *)item.Name, (uint16)item.Name.Len());
			if(isZip64) {
				Write16(NFileHeader::NExtraID::kZip64);
				Write16(8 + 8);
				Write64(size);
				Write64(packSize);
			}
			WriteExtra(item.LocalExtra);
			// Why don't we write NTFS timestamps to local header?
			// Probably we want to reduce size of archive?
			const uint32 localFileHeaderSize = (uint32)(m_CurPos - m_LocalHeaderPos);
			if(needCheck && m_LocalFileHeaderSize != localFileHeaderSize)
				throw CSystemException(E_FAIL);
			m_LocalFileHeaderSize = localFileHeaderSize;
			m_OutBuffer.FlushWithCheck();
		}

		void COutArchive::WriteLocalHeader_Replace(CItemOut &item)
		{
			m_CurPos = m_LocalHeaderPos + m_LocalFileHeaderSize + item.PackSize;
			if(item.HasDescriptor()) {
				WriteDescriptor(item);
				m_OutBuffer.FlushWithCheck();
			}
			const uint64 nextPos = m_CurPos;
			m_CurPos = m_LocalHeaderPos;
			SeekToCurPos();
			WriteLocalHeader(item, true);
			m_CurPos = nextPos;
			SeekToCurPos();
		}

		void COutArchive::WriteDescriptor(const CItemOut &item)
		{
			Byte buf[kDataDescriptorSize64];
			SetUi32(buf, NSignature::kDataDescriptor);
			SetUi32(buf + 4, item.Crc);
			unsigned descriptorSize;
			if(m_IsZip64) {
				SetUi64(buf + 8, item.PackSize);
				SetUi64(buf + 16, item.Size);
				descriptorSize = kDataDescriptorSize64;
			}
			else {
				SetUi32(buf + 8, (uint32)item.PackSize);
				SetUi32(buf + 12, (uint32)item.Size);
				descriptorSize = kDataDescriptorSize32;
			}
			WriteBytes(buf, descriptorSize);
		}

		void COutArchive::WriteCentralHeader(const CItemOut &item)
		{
			bool isUnPack64 = DOES_NEED_ZIP64(item.Size);
			bool isPack64 = DOES_NEED_ZIP64(item.PackSize);
			bool isPosition64 = DOES_NEED_ZIP64(item.LocalHeaderPos);
			bool isZip64 = isPack64 || isUnPack64 || isPosition64;
			Write32(NSignature::kCentralFileHeader);
			Write8(item.MadeByVersion.Version);
			Write8(item.MadeByVersion.HostOS);
			WriteCommonItemInfo(item, isZip64);
			Write32(item.Crc);
			WRITE_32_VAL_SPEC(item.PackSize, isPack64);
			WRITE_32_VAL_SPEC(item.Size, isUnPack64);
			Write16((uint16)item.Name.Len());
			uint16 zip64ExtraSize = static_cast<uint16>((isUnPack64 ? 8 : 0) + (isPack64 ? 8 : 0) + (isPosition64 ? 8 : 0));
			const uint16 kNtfsExtraSize = 4 + 2 + 2 + (3 * 8);
			const uint16 centralExtraSize = static_cast<uint16>((isZip64 ? 4 + zip64ExtraSize : 0) + (item.NtfsTimeIsDefined ? 4 + kNtfsExtraSize : 0) + item.CentralExtra.GetSize());
			Write16(centralExtraSize); // test it;
			const uint16 commentSize = (uint16)item.Comment.Size();
			Write16(commentSize);
			Write16(0); // DiskNumberStart;
			Write16(item.InternalAttrib);
			Write32(item.ExternalAttrib);
			WRITE_32_VAL_SPEC(item.LocalHeaderPos, isPosition64);
			WriteBytes((const char *)item.Name, item.Name.Len());
			if(isZip64) {
				Write16(NFileHeader::NExtraID::kZip64);
				Write16(zip64ExtraSize);
				if(isUnPack64)
					Write64(item.Size);
				if(isPack64)
					Write64(item.PackSize);
				if(isPosition64)
					Write64(item.LocalHeaderPos);
			}

			if(item.NtfsTimeIsDefined) {
				Write16(NFileHeader::NExtraID::kNTFS);
				Write16(kNtfsExtraSize);
				Write32(0); // reserved
				Write16(NFileHeader::NNtfsExtra::kTagTime);
				Write16(8 * 3);
				WriteNtfsTime(item.Ntfs_MTime);
				WriteNtfsTime(item.Ntfs_ATime);
				WriteNtfsTime(item.Ntfs_CTime);
			}

			WriteExtra(item.CentralExtra);
			if(commentSize != 0)
				WriteBytes(item.Comment, commentSize);
		}

		void COutArchive::WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer * comment)
		{
			uint64 cdOffset = GetCurPos();
			FOR_VECTOR(i, items) {
				WriteCentralHeader(items[i]);
			}
			uint64 cd64EndOffset = GetCurPos();
			uint64 cdSize = cd64EndOffset - cdOffset;
			bool cdOffset64 = DOES_NEED_ZIP64(cdOffset);
			bool cdSize64 = DOES_NEED_ZIP64(cdSize);
			bool items64 = items.Size() >= 0xFFFF;
			bool isZip64 = (cdOffset64 || cdSize64 || items64);

			// isZip64 = true; // to test Zip64

			if(isZip64) {
				Write32(NSignature::kEcd64);
				Write64(kEcd64_MainSize);

				// to test extra block:
				// const uint32 extraSize = 1 << 26;
				// Write64(kEcd64_MainSize + extraSize);

				Write16(45); // made by version
				Write16(45); // extract version
				Write32(0); // ThisDiskNumber = 0;
				Write32(0); // StartCentralDirectoryDiskNumber;;
				Write64((uint64)items.Size());
				Write64((uint64)items.Size());
				Write64((uint64)cdSize);
				Write64((uint64)cdOffset);

				// for(uint32 iii = 0; iii < extraSize; iii++) Write8(1);

				Write32(NSignature::kEcd64Locator);
				Write32(0); // number of the disk with the start of the zip64 end of central directory
				Write64(cd64EndOffset);
				Write32(1); // total number of disks
			}

			Write32(NSignature::kEcd);
			Write16(0); // ThisDiskNumber = 0;
			Write16(0); // StartCentralDirectoryDiskNumber;
			Write16(static_cast<uint16>(items64 ? 0xFFFF : items.Size()));
			Write16(static_cast<uint16>(items64 ? 0xFFFF : items.Size()));

			WRITE_32_VAL_SPEC(cdSize, cdSize64);
			WRITE_32_VAL_SPEC(cdOffset, cdOffset64);

			const uint16 commentSize = static_cast<uint16>(comment ? comment->Size() : 0);
			Write16((uint16)commentSize);
			if(commentSize != 0)
				WriteBytes((const Byte*)*comment, commentSize);
			m_OutBuffer.FlushWithCheck();
		}

		void COutArchive::CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream)
		{
			COffsetOutStream * streamSpec = new COffsetOutStream;
			outStream = streamSpec;
			streamSpec->Init(m_Stream, m_Base + m_CurPos);
		}

		void COutArchive::CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream)
		{
			outStream = m_Stream;
		}

		CItemEx::CItemEx() : DescriptorWasRead(false) 
		{
		}

		uint64 CItemEx::GetLocalFullSize() const { return LocalFullHeaderSize + GetPackSizeWithDescriptor(); }
		uint64 CItemEx::GetDataPosition() const { return LocalHeaderPos + LocalFullHeaderSize; }

		static const char * const kHostOS[] = {
			"FAT", "AMIGA", "VMS", "Unix", "VM/CMS", "Atari", "HPFS", "Macintosh", "Z-System", "CP/M", 
			"TOPS-20", "NTFS", "SMS/QDOS", "Acorn", "VFAT", "MVS", "BeOS", "Tandem", "OS/400", "OS/X"
		};
		const char * const kMethodNames1[kNumMethodNames1] = {
			"Store", "Shrink", "Reduce1", "Reduce2", "Reduce3", "Reduce4", "Implode", NULL /*"Tokenize"*/, 
			"Deflate", "Deflate64", "PKImploding", NULL, "BZip2", NULL, "LZMA"
		};

		const char * const kMethodNames2[kNumMethodNames2] = { "xz", "Jpeg", "WavPack", "PPMd", "WzAES" };

		#define kMethod_AES "AES"
		#define kMethod_ZipCrypto "ZipCrypto"
		#define kMethod_StrongCrypto "StrongCrypto"

		static const char * const kDeflateLevels[4] = { "Normal", "Maximum", "Fast", "Fastest" };
		static const CUInt32PCharPair g_HeaderCharacts[] = { { 0, "Encrypt" }, { 3, "Descriptor" }, /*{ 5, "Patched" },*/ { 6, kMethod_StrongCrypto }, { 11, "UTF8" } };

		struct CIdToNamePair {
			unsigned Id;
			const char * Name;
		};

		static const CIdToNamePair k_StrongCryptoPairs[] = {
			{ NStrongCrypto_AlgId::kDES, "DES" }, { NStrongCrypto_AlgId::kRC2old, "RC2a" },
			{ NStrongCrypto_AlgId::k3DES168, "3DES-168" }, { NStrongCrypto_AlgId::k3DES112, "3DES-112" },
			{ NStrongCrypto_AlgId::kAES128, "pkAES-128" }, { NStrongCrypto_AlgId::kAES192, "pkAES-192" },
			{ NStrongCrypto_AlgId::kAES256, "pkAES-256" }, { NStrongCrypto_AlgId::kRC2, "RC2" },
			{ NStrongCrypto_AlgId::kBlowfish, "Blowfish" }, { NStrongCrypto_AlgId::kTwofish, "Twofish" },
			{ NStrongCrypto_AlgId::kRC4, "RC4" }
		};

		static const char * FindNameForId(const CIdToNamePair * pairs, unsigned num, unsigned id)
		{
			for(uint i = 0; i < num; i++) {
				const CIdToNamePair & pair = pairs[i];
				if(id == pair.Id)
					return pair.Name;
			}
			return NULL;
		}

		static const Byte kProps[] = {
			kpidPath,
			kpidIsDir,
			kpidSize,
			kpidPackSize,
			kpidMTime,
			kpidCTime,
			kpidATime,
			kpidAttrib,
			// kpidPosixAttrib,
			kpidEncrypted,
			kpidComment,
			kpidCRC,
			kpidMethod,
			kpidCharacts,
			kpidHostOS,
			kpidUnpackVer,
			kpidVolumeIndex,
			kpidOffset
		};

		static const Byte kArcProps[] = {
			kpidEmbeddedStubSize,
			kpidBit64,
			kpidComment,
			kpidCharacts,
			kpidTotalPhySize,
			kpidIsVolume,
			kpidVolumeIndex,
			kpidNumVolumes
		};

		CHandler::CHandler()
		{
			InitMethodProps();
		}

		void CHandler::InitMethodProps()
		{
			_props.Init();
			m_MainMethod = -1;
			m_ForceAesMode = false;
			m_WriteNtfsTimeExtra = true;
			_removeSfxBlock = false;
			m_ForceLocal = false;
			m_ForceUtf8 = false;
			_forceCodePage = false;
			_specifiedCodePage = CP_OEMCP;
		}

		static AString BytesToString(const CByteBuffer & data)
		{
			AString s;
			s.SetFrom_CalcLen((const char *)(const Byte*)data, (uint)data.Size());
			return s;
		}

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NWindows::NCOM::CPropVariant prop;
			switch(propID) {
				case kpidBit64:  if(m_Archive.IsZip64) prop = m_Archive.IsZip64; break;
				case kpidComment:  
					if(m_Archive.ArcInfo.Comment.Size() != 0) 
						prop = MultiByteToUnicodeString(BytesToString(m_Archive.ArcInfo.Comment), CP_ACP); break;
				case kpidPhySize:  prop = m_Archive.GetPhySize(); break;
				case kpidOffset:  prop = m_Archive.GetOffset(); break;
				case kpidEmbeddedStubSize:
				{
					uint64 stubSize = m_Archive.GetEmbeddedStubSize();
					if(stubSize != 0)
						prop = stubSize;
					break;
				}
				case kpidTotalPhySize: if(m_Archive.IsMultiVol) prop = m_Archive.Vols.TotalBytesSize; break;
				case kpidVolumeIndex: if(m_Archive.IsMultiVol) prop = (uint32)m_Archive.Vols.StartVolIndex; break;
				case kpidIsVolume: if(m_Archive.IsMultiVol) prop = true; break;
				case kpidNumVolumes: if(m_Archive.IsMultiVol) prop = (uint32)m_Archive.Vols.Streams.Size(); break;
				case kpidCharacts:
				{
					AString s;
					if(m_Archive.LocalsWereRead) {
						s.Add_OptSpaced("Local");
						if(m_Archive.LocalsCenterMerged)
							s.Add_OptSpaced("Central");
					}
					if(m_Archive.IsZip64)
						s.Add_OptSpaced("Zip64");
					if(m_Archive.ExtraMinorError)
						s.Add_OptSpaced("Minor_Extra_ERROR");
					if(!s.IsEmpty())
						prop = s;
					break;
				}
				case kpidWarningFlags:
				{
					uint32 v = 0;
					// if(m_Archive.ExtraMinorError) v |= kpv_ErrorFlags_HeadersError;
					if(m_Archive.HeadersWarning) v |= kpv_ErrorFlags_HeadersError;
					if(v != 0)
						prop = v;
					break;
				}
				case kpidWarning:
				{
					AString s;
					if(m_Archive.Overflow32bit)
						s.Add_OptSpaced("32-bit overflow in headers");
					if(m_Archive.Cd_NumEntries_Overflow_16bit)
						s.Add_OptSpaced("16-bit overflow for number of files in headers");
					if(!s.IsEmpty())
						prop = s;
					break;
				}
				case kpidError:
				{
					if(!m_Archive.Vols.MissingName.IsEmpty()) {
						UString s("Missing volume : ");
						s += m_Archive.Vols.MissingName;
						prop = s;
					}
					break;
				}
				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!m_Archive.IsArc) v |= kpv_ErrorFlags_IsNotArc;
					if(m_Archive.HeadersError) v |= kpv_ErrorFlags_HeadersError;
					if(m_Archive.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
					if(m_Archive.ArcInfo.Base < 0) {
						/* We try to support case when we have sfx-zip with embedded stub,
						   but the stream has access only to zip part.
						   In that case we ignore UnavailableStart error.
						   maybe we must show warning in that case. */
						uint64 stubSize = m_Archive.GetEmbeddedStubSize();
						if(stubSize < (uint64)-m_Archive.ArcInfo.Base)
							v |= kpv_ErrorFlags_UnavailableStart;
					}
					if(m_Archive.NoCentralDir) v |= kpv_ErrorFlags_UnconfirmedStart;
					prop = v;
					break;
				}
				case kpidReadOnly:
				{
					if(m_Archive.IsOpen())
						if(!m_Archive.CanUpdate())
							prop = true;
					break;
				}
			}
			prop.Detach(value);
			COM_TRY_END
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = m_Items.Size();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NWindows::NCOM::CPropVariant prop;
			const CItemEx &item = m_Items[index];
			const CExtraBlock &extra = item.GetMainExtra();
			switch(propID) {
				case kpidIsDir:  prop = item.IsDir(); break;
				case kpidAttrib:  prop = item.GetWinAttrib(); break;
				case kpidEncrypted:  prop = item.IsEncrypted(); break;
				case kpidUnpackVer: prop = (uint32)item.ExtractVersion.Version; break;
				case kpidVolumeIndex: prop = item.Disk; break;
				case kpidOffset: prop = item.LocalHeaderPos; break;
				case kpidPackSize:  prop = item.PackSize; break;
				case kpidPath:
				{
					UString res;
					item.GetUnicodeString(res, item.Name, false, _forceCodePage, _specifiedCodePage);
					NItemName::ReplaceToOsSlashes_Remove_TailSlash(res);
					prop = res;
					break;
				}
				case kpidSize:
				{
					if(item.FromCentral || !item.FromLocal || !item.HasDescriptor() || item.DescriptorWasRead)
						prop = item.Size;
					break;
				}
				case kpidTimeType:
				{
					FILETIME ft;
					uint32 unixTime;
					uint32 type;
					if(extra.GetNtfsTime(NFileHeader::NNtfsExtra::kMTime, ft))
						type = NFileTimeType::kWindows;
					else if(extra.GetUnixTime(true, NFileHeader::NUnixTime::kMTime, unixTime))
						type = NFileTimeType::kUnix;
					else
						type = NFileTimeType::kDOS;
					prop = type;
					break;
				}
				case kpidCTime:
				{
					FILETIME utc;
					bool defined = true;
					if(!extra.GetNtfsTime(NFileHeader::NNtfsExtra::kCTime, utc)) {
						uint32 unixTime = 0;
						if(extra.GetUnixTime(true, NFileHeader::NUnixTime::kCTime, unixTime))
							NTime::UnixTimeToFileTime(unixTime, utc);
						else
							defined = false;
					}
					if(defined)
						prop = utc;
					break;
				}
				case kpidATime:
				{
					FILETIME utc;
					bool defined = true;
					if(!extra.GetNtfsTime(NFileHeader::NNtfsExtra::kATime, utc)) {
						uint32 unixTime = 0;
						if(extra.GetUnixTime(true, NFileHeader::NUnixTime::kATime, unixTime))
							NTime::UnixTimeToFileTime(unixTime, utc);
						else
							defined = false;
					}
					if(defined)
						prop = utc;

					break;
				}
				case kpidMTime:
				{
					FILETIME utc;
					bool defined = true;
					if(!extra.GetNtfsTime(NFileHeader::NNtfsExtra::kMTime, utc)) {
						uint32 unixTime = 0;
						if(extra.GetUnixTime(true, NFileHeader::NUnixTime::kMTime, unixTime))
							NTime::UnixTimeToFileTime(unixTime, utc);
						else {
							FILETIME localFileTime;
							if(item.Time == 0)
								defined = false;
							else if(!NTime::DosTimeToFileTime(item.Time, localFileTime) || !LocalFileTimeToFileTime(&localFileTime, &utc))
								utc.dwHighDateTime = utc.dwLowDateTime = 0;
						}
					}
					if(defined)
						prop = utc;
					break;
				}
				case kpidPosixAttrib:
				{
					uint32 attrib;
					if(item.GetPosixAttrib(attrib))
						prop = attrib;
					break;
				}
				case kpidComment:
					if(item.Comment.Size() != 0) {
						UString res;
						item.GetUnicodeString(res, BytesToString(item.Comment), true, _forceCodePage, _specifiedCodePage);
						prop = res;
					}
					break;
				case kpidCRC:  if(item.IsThereCrc()) prop = item.Crc; break;
				case kpidMethod:
				{
					uint   id = item.Method;
					AString m;
					if(item.IsEncrypted()) {
						if(id == NFileHeader::NCompressionMethod::kWzAES) {
							m += kMethod_AES;
							CWzAesExtra aesField;
							if(extra.GetWzAes(aesField)) {
								m += '-';
								m.Add_UInt32(((uint)aesField.Strength + 1) * 64);
								id = aesField.Method;
							}
						}
						else if(item.IsStrongEncrypted()) {
							CStrongCryptoExtra f;
							f.AlgId = 0;
							if(extra.GetStrongCrypto(f)) {
								const char * s = FindNameForId(k_StrongCryptoPairs, ARRAY_SIZE(k_StrongCryptoPairs), f.AlgId);
								if(s)
									m += s;
								else {
									m += kMethod_StrongCrypto;
									m += ':';
									m.Add_UInt32(f.AlgId);
								}
								if(f.CertificateIsUsed())
									m += "-Cert";
							}
							else
								m += kMethod_StrongCrypto;
						}
						else
							m += kMethod_ZipCrypto;
						m += ' ';
					}
					{
						const char * s = NULL;
						if(id < kNumMethodNames1)
							s = kMethodNames1[id];
						else {
							int id2 = (int)id - (int)kMethodNames2Start;
							if(id2 >= 0 && id2 < kNumMethodNames2)
								s = kMethodNames2[id2];
						}
						if(s)
							m += s;
						else
							m.Add_UInt32(id);
					}
					{
						uint   level = item.GetDeflateLevel();
						if(level != 0) {
							if(id == NFileHeader::NCompressionMethod::kLZMA) {
								if(level & 1)
									m += ":eos";
								level &= ~1;
							}
							else if(id == NFileHeader::NCompressionMethod::kDeflate) {
								m += ':';
								m += kDeflateLevels[level];
								level = 0;
							}

							if(level != 0) {
								m += ":v";
								m.Add_UInt32(level);
							}
						}
					}
					prop = m;
					break;
				}
				case kpidCharacts:
				{
					AString s;
					if(item.FromLocal) {
						s.Add_OptSpaced("Local");
						item.LocalExtra.PrintInfo(s);
						if(item.FromCentral) {
							s.Add_OptSpaced(":");
							s.Add_OptSpaced("Central");
						}
					}
					if(item.FromCentral) {
						item.CentralExtra.PrintInfo(s);
					}
					uint32 flags = item.Flags;
					flags &= ~(6); // we don't need compression related bits here.
					if(flags != 0) {
						AString s2 = FlagsToString(g_HeaderCharacts, ARRAY_SIZE(g_HeaderCharacts), flags);
						if(!s2.IsEmpty()) {
							s.Add_OptSpaced(":");
							s.Add_OptSpaced(s2);
						}
					}
					if(!item.FromCentral && item.FromLocal && item.HasDescriptor() && !item.DescriptorWasRead)
						s.Add_OptSpaced("Descriptor_ERROR");
					if(!s.IsEmpty())
						prop = s;
					break;
				}
				case kpidHostOS:
				{
					const Byte hostOS = item.GetHostOS();
					TYPE_TO_PROP(kHostOS, hostOS, prop);
					break;
				}
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Open(IInStream * inStream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * callback)
		{
			COM_TRY_BEGIN
			try {
				Close();
				HRESULT res = m_Archive.Open(inStream, maxCheckStartPosition, callback, m_Items);
				if(res != S_OK) {
					m_Items.Clear();
					m_Archive.ClearRefs(); // we don't want to clear error flags
				}
				return res;
			}
			catch(...) { Close(); throw; }
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			m_Items.Clear();
			m_Archive.Close();
			return S_OK;
		}

		class CLzmaDecoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
		public:
			NCompress::NLzma::CDecoder * DecoderSpec;
			CMyComPtr<ICompressCoder> Decoder;
			MY_UNKNOWN_IMP2(ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
			CLzmaDecoder();
		};

		CLzmaDecoder::CLzmaDecoder()
		{
			DecoderSpec = new NCompress::NLzma::CDecoder;
			Decoder = DecoderSpec;
		}

		static const uint kZipLzmaPropsSize = 4 + LZMA_PROPS_SIZE;

		HRESULT CLzmaDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			Byte buf[kZipLzmaPropsSize];
			RINOK(ReadStream_FALSE(inStream, buf, kZipLzmaPropsSize));
			if(buf[2] != LZMA_PROPS_SIZE || buf[3] != 0)
				return E_NOTIMPL;
			RINOK(DecoderSpec->SetDecoderProperties2(buf + 4, LZMA_PROPS_SIZE));
			uint64 inSize2 = 0;
			if(inSize) {
				inSize2 = *inSize;
				if(inSize2 < kZipLzmaPropsSize)
					return S_FALSE;
				inSize2 -= kZipLzmaPropsSize;
			}
			return Decoder->Code(inStream, outStream, inSize ? &inSize2 : NULL, outSize, progress);
		}

		STDMETHODIMP CLzmaDecoder::SetFinishMode(uint32 finishMode)
		{
			DecoderSpec->FinishStream = (finishMode != 0);
			return S_OK;
		}

		STDMETHODIMP CLzmaDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = DecoderSpec->GetInputProcessedSize() + kZipLzmaPropsSize;
			return S_OK;
		}

		struct CMethodItem {
			unsigned ZipMethod;
			CMyComPtr<ICompressCoder> Coder;
		};

		class CZipDecoder {
			NCrypto::NZip::CDecoder * _zipCryptoDecoderSpec;
			NCrypto::NZipStrong::CDecoder * _pkAesDecoderSpec;
			NCrypto::NWzAes::CDecoder * _wzAesDecoderSpec;
			CMyComPtr<ICompressFilter> _zipCryptoDecoder;
			CMyComPtr<ICompressFilter> _pkAesDecoder;
			CMyComPtr<ICompressFilter> _wzAesDecoder;
			CFilterCoder * filterStreamSpec;
			CMyComPtr<ISequentialInStream> filterStream;
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			CObjectVector<CMethodItem> methodItems;
			CLzmaDecoder * lzmaDecoderSpec;
		public:
			CZipDecoder() : _zipCryptoDecoderSpec(0), _pkAesDecoderSpec(0), _wzAesDecoderSpec(0), filterStreamSpec(0), lzmaDecoderSpec(0)
			{
			}
			HRESULT Decode(DECL_EXTERNAL_CODECS_LOC_VARS CInArchive &archive, const CItemEx &item,
				ISequentialOutStream *realOutStream, IArchiveExtractCallback *extractCallback, ICompressProgressInfo *compressProgress,
			#ifndef _7ZIP_ST
						uint32 numThreads,
			#endif
						int32 &res);
		};

		static HRESULT SkipStreamData(ISequentialInStream * stream, bool &thereAreData)
		{
			thereAreData = false;
			const size_t kBufSize = 1 << 12;
			Byte buf[kBufSize];
			for(;; ) {
				size_t size = kBufSize;
				RINOK(ReadStream(stream, buf, &size));
				if(size == 0)
					return S_OK;
				thereAreData = true;
			}
		}

		HRESULT CZipDecoder::Decode(DECL_EXTERNAL_CODECS_LOC_VARS CInArchive &archive, const CItemEx &item,
			ISequentialOutStream * realOutStream, IArchiveExtractCallback * extractCallback, ICompressProgressInfo * compressProgress,
			#ifndef _7ZIP_ST
					uint32 numThreads,
			#endif
					int32 &res)
		{
			res = NExtractArc::NOperationResult::kHeadersError;
			CFilterCoder::C_InStream_Releaser inStreamReleaser;
			CFilterCoder::C_Filter_Releaser filterReleaser;
			bool needCRC = true;
			bool wzAesMode = false;
			bool pkAesMode = false;
			unsigned id = item.Method;
			if(item.IsEncrypted()) {
				if(item.IsStrongEncrypted()) {
					CStrongCryptoExtra f;
					if(!item.CentralExtra.GetStrongCrypto(f)) {
						res = NExtractArc::NOperationResult::kUnsupportedMethod;
						return S_OK;
					}
					pkAesMode = true;
				}
				else if(id == NFileHeader::NCompressionMethod::kWzAES) {
					CWzAesExtra aesField;
					if(!item.GetMainExtra().GetWzAes(aesField))
						return S_OK;
					wzAesMode = true;
					needCRC = aesField.NeedCrc();
				}
			}
			COutStreamWithCRC * outStreamSpec = new COutStreamWithCRC;
			CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
			outStreamSpec->SetStream(realOutStream);
			outStreamSpec->Init(needCRC);
			CMyComPtr<ISequentialInStream> packStream;
			CLimitedSequentialInStream * limitedStreamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> inStream(limitedStreamSpec);
			{
				uint64 packSize = item.PackSize;
				if(wzAesMode) {
					if(packSize < NCrypto::NWzAes::kMacSize)
						return S_OK;
					packSize -= NCrypto::NWzAes::kMacSize;
				}
				RINOK(archive.GetItemStream(item, true, packStream));
				if(!packStream) {
					res = NExtractArc::NOperationResult::kUnavailable;
					return S_OK;
				}
				limitedStreamSpec->SetStream(packStream);
				limitedStreamSpec->Init(packSize);
			}
			res = NExtractArc::NOperationResult::kDataError;
			CMyComPtr<ICompressFilter> cryptoFilter;
			if(item.IsEncrypted()) {
				if(wzAesMode) {
					CWzAesExtra aesField;
					if(!item.GetMainExtra().GetWzAes(aesField))
						return S_OK;
					id = aesField.Method;
					if(!_wzAesDecoder) {
						_wzAesDecoderSpec = new NCrypto::NWzAes::CDecoder;
						_wzAesDecoder = _wzAesDecoderSpec;
					}
					cryptoFilter = _wzAesDecoder;
					if(!_wzAesDecoderSpec->SetKeyMode(aesField.Strength)) {
						res = NExtractArc::NOperationResult::kUnsupportedMethod;
						return S_OK;
					}
				}
				else if(pkAesMode) {
					if(!_pkAesDecoder) {
						_pkAesDecoderSpec = new NCrypto::NZipStrong::CDecoder;
						_pkAesDecoder = _pkAesDecoderSpec;
					}
					cryptoFilter = _pkAesDecoder;
				}
				else {
					if(!_zipCryptoDecoder) {
						_zipCryptoDecoderSpec = new NCrypto::NZip::CDecoder;
						_zipCryptoDecoder = _zipCryptoDecoderSpec;
					}
					cryptoFilter = _zipCryptoDecoder;
				}
				CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
				RINOK(cryptoFilter.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword));
				if(!cryptoSetPassword)
					return E_FAIL;
				if(!getTextPassword)
					extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
				if(getTextPassword) {
					CMyComBSTR password;
					RINOK(getTextPassword->CryptoGetTextPassword(&password));
					AString charPassword;
					if(password) {
						UnicodeStringToMultiByte2(charPassword, (const wchar_t*)password, CP_ACP);
						/*
						   if(wzAesMode || pkAesMode)
						   {
						   }
						   else
						   {
						   // PASSWORD encoding for ZipCrypto:
						   // pkzip25 / WinZip / Windows probably use ANSI
						   // 7-Zip <  4.43 creates ZIP archives with OEM encoding in password
						   // 7-Zip >= 4.43 creates ZIP archives only with ASCII characters in password
						   // 7-Zip <  17.00 uses CP_OEMCP for password decoding
						   // 7-Zip >= 17.00 uses CP_ACP   for password decoding
						   }
						 */
					}
					HRESULT result = cryptoSetPassword->CryptoSetPassword((const Byte*)(const char *)charPassword, charPassword.Len());
					if(result != S_OK) {
						res = NExtractArc::NOperationResult::kWrongPassword;
						return S_OK;
					}
				}
				else {
					res = NExtractArc::NOperationResult::kWrongPassword;
					return S_OK;
					// RINOK(cryptoSetPassword->CryptoSetPassword(NULL, 0));
				}
			}
			uint   m;
			for(m = 0; m < methodItems.Size(); m++)
				if(methodItems[m].ZipMethod == id)
					break;
			if(m == methodItems.Size()) {
				CMethodItem mi;
				mi.ZipMethod = id;
				if(id == NFileHeader::NCompressionMethod::kStore)
					mi.Coder = new NCompress::CCopyCoder;
				else if(id == NFileHeader::NCompressionMethod::kShrink)
					mi.Coder = new NCompress::NShrink::CDecoder;
				else if(id == NFileHeader::NCompressionMethod::kImplode)
					mi.Coder = new NCompress::NImplode::NDecoder::CCoder;
				else if(id == NFileHeader::NCompressionMethod::kLZMA) {
					lzmaDecoderSpec = new CLzmaDecoder;
					mi.Coder = lzmaDecoderSpec;
				}
				else if(id == NFileHeader::NCompressionMethod::kXz)
					mi.Coder = new NCompress::NXz::CComDecoder;
				else if(id == NFileHeader::NCompressionMethod::kPPMd)
					mi.Coder = new NCompress::NPpmdZip::CDecoder(true);
				else {
					CMethodId szMethodID;
					if(id == NFileHeader::NCompressionMethod::kBZip2)
						szMethodID = kMethodId_BZip2;
					else {
						if(id > 0xFF) {
							res = NExtractArc::NOperationResult::kUnsupportedMethod;
							return S_OK;
						}
						szMethodID = kMethodId_ZipBase + (Byte)id;
					}
					RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS szMethodID, false, mi.Coder));
					if(!mi.Coder) {
						res = NExtractArc::NOperationResult::kUnsupportedMethod;
						return S_OK;
					}
				}
				m = methodItems.Add(mi);
			}
			ICompressCoder * coder = methodItems[m].Coder;
			{
				CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
				coder->QueryInterface(IID_ICompressSetDecoderProperties2, (void **)&setDecoderProperties);
				if(setDecoderProperties) {
					Byte properties = (Byte)item.Flags;
					RINOK(setDecoderProperties->SetDecoderProperties2(&properties, 1));
				}
			}
		  #ifndef _7ZIP_ST
			{
				CMyComPtr<ICompressSetCoderMt> setCoderMt;
				coder->QueryInterface(IID_ICompressSetCoderMt, (void **)&setCoderMt);
				if(setCoderMt) {
					RINOK(setCoderMt->SetNumberOfThreads(numThreads));
				}
			}
		  #endif
			CMyComPtr<ISequentialInStream> inStreamNew;
			bool isFullStreamExpected = (!item.HasDescriptor() || item.PackSize != 0);
			bool needReminderCheck = false;
			bool dataAfterEnd = false;
			bool truncatedError = false;
			bool lzmaEosError = false;
			{
				HRESULT result = S_OK;
				if(item.IsEncrypted()) {
					if(!filterStream) {
						filterStreamSpec = new CFilterCoder(false);
						filterStream = filterStreamSpec;
					}
					filterReleaser.FilterCoder = filterStreamSpec;
					filterStreamSpec->Filter = cryptoFilter;
					if(wzAesMode) {
						result = _wzAesDecoderSpec->ReadHeader(inStream);
						if(result == S_OK) {
							if(!_wzAesDecoderSpec->Init_and_CheckPassword()) {
								res = NExtractArc::NOperationResult::kWrongPassword;
								return S_OK;
							}
						}
					}
					else if(pkAesMode) {
						isFullStreamExpected = false;
						result = _pkAesDecoderSpec->ReadHeader(inStream, item.Crc, item.Size);
						if(result == S_OK) {
							bool passwOK;
							result = _pkAesDecoderSpec->Init_and_CheckPassword(passwOK);
							if(result == S_OK && !passwOK) {
								res = NExtractArc::NOperationResult::kWrongPassword;
								return S_OK;
							}
						}
					}
					else {
						result = _zipCryptoDecoderSpec->ReadHeader(inStream);
						if(result == S_OK) {
							_zipCryptoDecoderSpec->Init_BeforeDecode();

							/* Info-ZIP modification to ZipCrypto format:
								 if bit 3 of the general purpose bit flag is set,
								 it uses high byte of 16-bit File Time.
							   Info-ZIP code probably writes 2 bytes of File Time.
							   We check only 1 byte. */

							// uint32 v1 = GetUi16(_zipCryptoDecoderSpec->_header +
							// NCrypto::NZip::kHeaderSize - 2);
							// uint32 v2 = (item.HasDescriptor() ? (item.Time & 0xFFFF) : (item.Crc >> 16));

							Byte v1 = _zipCryptoDecoderSpec->_header[NCrypto::NZip::kHeaderSize - 1];
							Byte v2 = (Byte)(item.HasDescriptor() ? (item.Time >> 8) : (item.Crc >> 24));
							if(v1 != v2) {
								res = NExtractArc::NOperationResult::kWrongPassword;
								return S_OK;
							}
						}
					}
					if(result == S_OK) {
						inStreamReleaser.FilterCoder = filterStreamSpec;
						RINOK(filterStreamSpec->SetInStream(inStream));
						/* IFilter::Init() does nothing in all zip crypto filters.
						   So we can call any Initialize function in CFilterCoder. */
						RINOK(filterStreamSpec->Init_NoSubFilterInit());
						// RINOK(filterStreamSpec->SetOutStreamSize(NULL));
						inStreamNew = filterStream;
					}
				}
				else
					inStreamNew = inStream;
				if(result == S_OK) {
					CMyComPtr<ICompressSetFinishMode> setFinishMode;
					coder->QueryInterface(IID_ICompressSetFinishMode, (void **)&setFinishMode);
					if(setFinishMode) {
						RINOK(setFinishMode->SetFinishMode(BoolToInt(true)));
					}
					const uint64 coderPackSize = limitedStreamSpec->GetRem();
					bool useUnpackLimit = (id == 0 || !item.HasDescriptor() || item.Size >= ((uint64)1 << 32) || item.LocalExtra.IsZip64 || item.CentralExtra.IsZip64);
					result = coder->Code(inStreamNew, outStream, isFullStreamExpected ? &coderPackSize : NULL,
										// NULL,
								useUnpackLimit ? &item.Size : NULL, compressProgress);
					if(result == S_OK) {
						CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
						coder->QueryInterface(IID_ICompressGetInStreamProcessedSize, (void **)&getInStreamProcessedSize);
						if(getInStreamProcessedSize && setFinishMode) {
							uint64 processed;
							RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&processed));
							if(processed != static_cast<uint64>(-1LL)) {
								if(pkAesMode) {
									const uint32 padSize = _pkAesDecoderSpec->GetPadSize((uint32)processed);
									if(processed + padSize > coderPackSize)
										truncatedError = true;
									else {
										if(processed + padSize < coderPackSize)
											dataAfterEnd = true;
										// also here we can check PKCS7 padding data from
										// reminder (it can be inside stream buffer in coder).
									}
								}
								else {
									if(processed < coderPackSize) {
										if(isFullStreamExpected)
											dataAfterEnd = true;
									}
									else if(processed > coderPackSize)
										truncatedError = true;
									needReminderCheck = isFullStreamExpected;
								}
							}
						}
					}
					if(result == S_OK && id == NFileHeader::NCompressionMethod::kLZMA)
						if(!lzmaDecoderSpec->DecoderSpec->CheckFinishStatus(item.IsLzmaEOS()))
							lzmaEosError = true;
				}
				if(result == S_FALSE)
					return S_OK;
				if(result == E_NOTIMPL) {
					res = NExtractArc::NOperationResult::kUnsupportedMethod;
					return S_OK;
				}
				RINOK(result);
			}
			bool crcOK = true;
			bool authOk = true;
			if(needCRC)
				crcOK = (outStreamSpec->GetCRC() == item.Crc);
			if(wzAesMode) {
				bool thereAreData = false;
				if(SkipStreamData(inStreamNew, thereAreData) != S_OK)
					authOk = false;
				if(needReminderCheck && thereAreData)
					dataAfterEnd = true;
				limitedStreamSpec->Init(NCrypto::NWzAes::kMacSize);
				if(_wzAesDecoderSpec->CheckMac(inStream, authOk) != S_OK)
					authOk = false;
			}
			res = NExtractArc::NOperationResult::kCRCError;

			if(crcOK && authOk) {
				res = NExtractArc::NOperationResult::kOK;
				if(dataAfterEnd)
					res = NExtractArc::NOperationResult::kDataAfterEnd;
				else if(truncatedError)
					res = NExtractArc::NOperationResult::kUnexpectedEnd;
				else if(lzmaEosError)
					res = NExtractArc::NOperationResult::kHeadersError;

				// CheckDescriptor() supports only data descriptor with signature and
				// it doesn't support "old" pkzip's data descriptor without signature.
				// So we disable that check.
				/*
				   if(item.HasDescriptor() && archive.CheckDescriptor(item) != S_OK)
				   res = NExtractArc::NOperationResult::kHeadersError;
				 */
			}

			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			CZipDecoder myDecoder;
			uint64 totalUnPacked = 0, totalPacked = 0;
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = m_Items.Size();
			if(numItems == 0)
				return S_OK;
			uint32 i;
			for(i = 0; i < numItems; i++) {
				const CItemEx &item = m_Items[allFilesMode ? i : indices[i]];
				totalUnPacked += item.Size;
				totalPacked += item.PackSize;
			}
			RINOK(extractCallback->SetTotal(totalUnPacked));
			uint64 currentTotalUnPacked = 0, currentTotalPacked = 0;
			uint64 currentItemUnPacked, currentItemPacked;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			for(i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked, currentTotalPacked += currentItemPacked) {
				currentItemUnPacked = 0;
				currentItemPacked = 0;
				lps->InSize = currentTotalPacked;
				lps->OutSize = currentTotalUnPacked;
				RINOK(lps->SetCur());
				CMyComPtr<ISequentialOutStream> realOutStream;
				int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				uint32 index = allFilesMode ? i : indices[i];
				CItemEx item = m_Items[index];
				bool isLocalOffsetOK = m_Archive.IsLocalOffsetOK(item);
				bool skip = !isLocalOffsetOK && !item.IsDir();
				if(skip)
					askMode = NExtractArc::NAskMode::kSkip;
				currentItemUnPacked = item.Size;
				currentItemPacked = item.PackSize;
				RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
				if(!isLocalOffsetOK) {
					RINOK(extractCallback->PrepareOperation(askMode));
					realOutStream.Release();
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnavailable));
					continue;
				}
				bool headersError = false;
				if(!item.FromLocal) {
					bool isAvail = true;
					HRESULT res = m_Archive.ReadLocalItemAfterCdItem(item, isAvail, headersError);
					if(res == S_FALSE) {
						if(item.IsDir() || realOutStream || testMode) {
							RINOK(extractCallback->PrepareOperation(askMode));
							realOutStream.Release();
							RINOK(extractCallback->SetOperationResult(isAvail ? NExtractArc::NOperationResult::kHeadersError : NExtractArc::NOperationResult::kUnavailable));
						}
						continue;
					}
					RINOK(res);
				}
				if(item.IsDir()) {
					// if(!testMode)
					{
						RINOK(extractCallback->PrepareOperation(askMode));
						realOutStream.Release();
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					}
					continue;
				}
				if(!testMode && !realOutStream)
					continue;
				RINOK(extractCallback->PrepareOperation(askMode));
				int32 res;
				HRESULT hres = myDecoder.Decode(EXTERNAL_CODECS_VARS m_Archive, item, realOutStream, extractCallback, progress,
			#ifndef _7ZIP_ST
							_props._numThreads,
			#endif
							res);
				RINOK(hres);
				realOutStream.Release();
				if(res == NExtractArc::NOperationResult::kOK && headersError)
					res = NExtractArc::NOperationResult::kHeadersError;
				RINOK(extractCallback->SetOperationResult(res))
			}
			lps->InSize = currentTotalPacked;
			lps->OutSize = currentTotalUnPacked;
			return lps->SetCur();
			COM_TRY_END
		}

		IMPL_ISetCompressCodecsInfo

		STDMETHODIMP CHandler::GetFileTimeType(uint32 * timeType)
		{
			*timeType = NFileTimeType::kDOS;
			return S_OK;
		}
		static bool IsSimpleAsciiString(const wchar_t * s)
		{
			for(;; ) {
				wchar_t c = *s++;
				if(c == 0)
					return true;
				if(c < 0x20 || c > 0x7F)
					return false;
			}
		}
		static int FindZipMethod(const char * s, const char * const * names, unsigned num)
		{
			for(uint i = 0; i < num; i++) {
				const char * name = names[i];
				if(name && sstreqi_ascii(s, name))
					return i;
			}
			return -1;
		}
		static int FindZipMethod(const char * s)
		{
			int k = FindZipMethod(s, kMethodNames1, kNumMethodNames1);
			if(k >= 0)
				return k;
			k = FindZipMethod(s, kMethodNames2, kNumMethodNames2);
			if(k >= 0)
				return kMethodNames2Start + k;
			return -1;
		}

		#define COM_TRY_BEGIN2 try {
		#define COM_TRY_END2 } catch(const CSystemException &e) { return e.ErrorCode; } catch(...) { return E_OUTOFMEMORY; }

		static HRESULT GetTime(IArchiveUpdateCallback * callback, int index, PROPID propID, FILETIME &filetime)
		{
			filetime.dwHighDateTime = filetime.dwLowDateTime = 0;
			NCOM::CPropVariant prop;
			RINOK(callback->GetProperty(index, propID, &prop));
			if(prop.vt == VT_FILETIME)
				filetime = prop.filetime;
			else if(prop.vt != VT_EMPTY)
				return E_INVALIDARG;
			return S_OK;
		}

		STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream * outStream, uint32 numItems, IArchiveUpdateCallback * callback)
		{
			COM_TRY_BEGIN2
			if(m_Archive.IsOpen()) {
				if(!m_Archive.CanUpdate())
					return E_NOTIMPL;
			}
			CObjectVector <CUpdateItem> updateItems;
			updateItems.ClearAndReserve(numItems);
			bool thereAreAesUpdates = false;
			uint64 largestSize = 0;
			bool largestSizeDefined = false;
			UString name;
			CUpdateItem ui;
			for(uint32 i = 0; i < numItems; i++) {
				int32 newData;
				int32 newProps;
				uint32 indexInArc;
				if(!callback)
					return E_FAIL;
				RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArc));
				name.Empty();
				ui.Clear();
				ui.NewProps = IntToBool(newProps);
				ui.NewData = IntToBool(newData);
				ui.IndexInArc = indexInArc;
				ui.IndexInClient = i;

				bool existInArchive = (indexInArc != (uint32)(int32)-1);
				if(existInArchive) {
					const CItemEx &inputItem = m_Items[indexInArc];
					if(inputItem.IsAesEncrypted())
						thereAreAesUpdates = true;
					if(!IntToBool(newProps))
						ui.IsDir = inputItem.IsDir();
				}

				if(IntToBool(newProps)) {
					{
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidAttrib, &prop));
						if(prop.vt == VT_EMPTY)
							ui.Attrib = 0;
						else if(prop.vt != VT_UI4)
							return E_INVALIDARG;
						else
							ui.Attrib = prop.ulVal;
					}
					{
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidPath, &prop));
						if(prop.vt == VT_EMPTY) {
							// name.Empty();
						}
						else if(prop.vt != VT_BSTR)
							return E_INVALIDARG;
						else
							name = prop.bstrVal;
					}

					{
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidIsDir, &prop));
						if(prop.vt == VT_EMPTY)
							ui.IsDir = false;
						else if(prop.vt != VT_BOOL)
							return E_INVALIDARG;
						else
							ui.IsDir = (prop.boolVal != VARIANT_FALSE);
					}

					{
						CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidTimeType, &prop));
						if(prop.vt == VT_UI4)
							ui.NtfsTimeIsDefined = (prop.ulVal == NFileTimeType::kWindows);
						else
							ui.NtfsTimeIsDefined = m_WriteNtfsTimeExtra;
					}
					RINOK(GetTime(callback, i, kpidMTime, ui.Ntfs_MTime));
					RINOK(GetTime(callback, i, kpidATime, ui.Ntfs_ATime));
					RINOK(GetTime(callback, i, kpidCTime, ui.Ntfs_CTime));

					{
						FILETIME localFileTime = { 0, 0 };
						if(ui.Ntfs_MTime.dwHighDateTime != 0 || ui.Ntfs_MTime.dwLowDateTime != 0)
							if(!FileTimeToLocalFileTime(&ui.Ntfs_MTime, &localFileTime))
								return E_INVALIDARG;
						FileTimeToDosTime(localFileTime, ui.Time);
					}

					NItemName::ReplaceSlashes_OsToUnix(name);

					bool needSlash = ui.IsDir;
					const wchar_t kSlash = L'/';
					if(!name.IsEmpty()) {
						if(name.Back() == kSlash) {
							if(!ui.IsDir)
								return E_INVALIDARG;
							needSlash = false;
						}
					}
					if(needSlash)
						name += kSlash;
					UINT codePage = _forceCodePage ? _specifiedCodePage : CP_OEMCP;
					bool tryUtf8 = true;
					if((m_ForceLocal || !m_ForceUtf8) && codePage != CP_UTF8) {
						bool defaultCharWasUsed;
						ui.Name = UnicodeStringToMultiByte(name, codePage, '_', defaultCharWasUsed);
						tryUtf8 = (!m_ForceLocal && (defaultCharWasUsed || MultiByteToUnicodeString(ui.Name, codePage) != name));
					}
					if(tryUtf8) {
						ui.IsUtf8 = !name.IsAscii();
						ConvertUnicodeToUTF8(name, ui.Name);
					}

					if(ui.Name.Len() >= (1 << 16))
						return E_INVALIDARG;

					{
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidComment, &prop));
						if(prop.vt == VT_EMPTY) {
							// ui.Comment.Free();
						}
						else if(prop.vt != VT_BSTR)
							return E_INVALIDARG;
						else {
							UString s = prop.bstrVal;
							AString a;
							if(ui.IsUtf8)
								ConvertUnicodeToUTF8(s, a);
							else {
								bool defaultCharWasUsed;
								a = UnicodeStringToMultiByte(s, codePage, '_', defaultCharWasUsed);
							}
							if(a.Len() >= (1 << 16))
								return E_INVALIDARG;
							ui.Comment.CopyFrom((const Byte*)(const char *)a, a.Len());
						}
					}

					/*
					   if(existInArchive)
					   {
					   const CItemEx &itemInfo = m_Items[indexInArc];
					   // ui.Commented = itemInfo.IsCommented();
					   ui.Commented = false;
					   if(ui.Commented)
					   {
						ui.CommentRange.Position = itemInfo.GetCommentPosition();
						ui.CommentRange.Size  = itemInfo.CommentSize;
					   }
					   }
					   else
					   ui.Commented = false;
					 */
				}

				if(IntToBool(newData)) {
					uint64 size = 0;
					if(!ui.IsDir) {
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidSize, &prop));
						if(prop.vt != VT_UI8)
							return E_INVALIDARG;
						size = prop.uhVal.QuadPart;
						if(largestSize < size)
							largestSize = size;
						largestSizeDefined = true;
					}
					ui.Size = size;
				}
				updateItems.Add(ui);
			}

			CMyComPtr<ICryptoGetTextPassword2> getTextPassword;
			{
				CMyComPtr<IArchiveUpdateCallback> udateCallBack2(callback);
				udateCallBack2.QueryInterface(IID_ICryptoGetTextPassword2, &getTextPassword);
			}
			CCompressionMethodMode options;
			(CBaseProps &)options = _props;
			options._dataSizeReduce = largestSize;
			options._dataSizeReduceDefined = largestSizeDefined;
			options.PasswordIsDefined = false;
			options.Password.Empty();
			if(getTextPassword) {
				CMyComBSTR password;
				int32 passwordIsDefined;
				RINOK(getTextPassword->CryptoGetTextPassword2(&passwordIsDefined, &password));
				options.PasswordIsDefined = IntToBool(passwordIsDefined);
				if(options.PasswordIsDefined) {
					if(!m_ForceAesMode)
						options.IsAesMode = thereAreAesUpdates;
					if(!IsSimpleAsciiString(password))
						return E_INVALIDARG;
					if(password)
						options.Password = UnicodeStringToMultiByte((LPCOLESTR)password, CP_OEMCP);
					if(options.IsAesMode) {
						if(options.Password.Len() > NCrypto::NWzAes::kPasswordSizeMax)
							return E_INVALIDARG;
					}
				}
			}
			int mainMethod = m_MainMethod;
			if(mainMethod < 0) {
				if(!_props._methods.IsEmpty()) {
					const AString &methodName = _props._methods.Front().MethodName;
					if(!methodName.IsEmpty()) {
						mainMethod = FindZipMethod(methodName);
						if(mainMethod < 0) {
							CMethodId methodId;
							uint32 numStreams;
							if(!FindMethod(EXTERNAL_CODECS_VARS methodName, methodId, numStreams))
								return E_NOTIMPL;
							if(numStreams != 1)
								return E_NOTIMPL;
							if(methodId == kMethodId_BZip2)
								mainMethod = NFileHeader::NCompressionMethod::kBZip2;
							else {
								if(methodId < kMethodId_ZipBase)
									return E_NOTIMPL;
								methodId -= kMethodId_ZipBase;
								if(methodId > 0xFF)
									return E_NOTIMPL;
								mainMethod = (int)methodId;
							}
						}
					}
				}
			}
			if(mainMethod < 0)
				mainMethod = (Byte)(((_props.GetLevel() == 0) ? NFileHeader::NCompressionMethod::kStore : NFileHeader::NCompressionMethod::kDeflate));
			else
				mainMethod = (Byte)mainMethod;
			options.MethodSequence.Add((Byte)mainMethod);
			if(mainMethod != NFileHeader::NCompressionMethod::kStore)
				options.MethodSequence.Add(NFileHeader::NCompressionMethod::kStore);
			return Update(EXTERNAL_CODECS_VARS m_Items, updateItems, outStream, m_Archive.IsOpen() ? &m_Archive : NULL, _removeSfxBlock, options, callback);
			COM_TRY_END2
		}
		STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
		{
			InitMethodProps();
			for(uint32 i = 0; i < numProps; i++) {
				UString name = names[i];
				name.MakeLower_Ascii();
				if(name.IsEmpty())
					return E_INVALIDARG;
				const PROPVARIANT &prop = values[i];
				if(name.IsEqualTo_Ascii_NoCase("em")) {
					if(prop.vt != VT_BSTR)
						return E_INVALIDARG;
					{
						const wchar_t * m = prop.bstrVal;
						if(IsString1PrefixedByString2_NoCase_Ascii(m, "aes")) {
							m += 3;
							if(StringsAreEqual_Ascii(m, "128"))
								_props.AesKeyMode = 1;
							else if(StringsAreEqual_Ascii(m, "192"))
								_props.AesKeyMode = 2;
							else if(StringsAreEqual_Ascii(m, "256") || m[0] == 0)
								_props.AesKeyMode = 3;
							else
								return E_INVALIDARG;
							_props.IsAesMode = true;
							m_ForceAesMode = true;
						}
						else if(sstreqi_ascii(m, "ZipCrypto")) {
							_props.IsAesMode = false;
							m_ForceAesMode = true;
						}
						else
							return E_INVALIDARG;
					}
				}
				else if(name.IsEqualTo("tc")) {
					RINOK(PROPVARIANT_to_bool(prop, m_WriteNtfsTimeExtra));
				}
				else if(name.IsEqualTo("cl")) {
					RINOK(PROPVARIANT_to_bool(prop, m_ForceLocal));
					if(m_ForceLocal)
						m_ForceUtf8 = false;
				}
				else if(name.IsEqualTo("cu")) {
					RINOK(PROPVARIANT_to_bool(prop, m_ForceUtf8));
					if(m_ForceUtf8)
						m_ForceLocal = false;
				}
				else if(name.IsEqualTo("cp")) {
					uint32 cp = CP_OEMCP;
					RINOK(ParsePropToUInt32(L"", prop, cp));
					_forceCodePage = true;
					_specifiedCodePage = cp;
				}
				else if(name.IsEqualTo("rsfx")) {
					RINOK(PROPVARIANT_to_bool(prop, _removeSfxBlock));
				}
				else {
					if(name.IsEqualTo_Ascii_NoCase("m") && prop.vt == VT_UI4) {
						uint32 id = prop.ulVal;
						if(id > 0xFF)
							return E_INVALIDARG;
						m_MainMethod = id;
					}
					else {
						RINOK(_props.SetProperty(name, prop));
					}
					// RINOK(_props.MethodInfo.ParseParamsFromPROPVARIANT(name, prop));
				}
			}
			_props._methods.DeleteFrontal(_props.GetNumEmptyMethods());
			if(_props._methods.Size() > 1)
				return E_INVALIDARG;
			if(_props._methods.Size() == 1) {
				const AString & methodName = _props._methods[0].MethodName;
				if(!methodName.IsEmpty()) {
					const char * end;
					uint32 id = ConvertStringToUInt32(methodName, &end);
					if(*end == 0 && id <= 0xFF)
						m_MainMethod = id;
					else if(methodName.IsEqualTo_Ascii_NoCase("Copy")) // it's alias for "Store"
						m_MainMethod = 0;
				}
			}

			return S_OK;
		}

		#ifdef _WIN32
			static const Byte kHostOS_ID = NFileHeader::NHostOS::kFAT;
		#else
			static const Byte kHostOS_ID = NFileHeader::NHostOS::kUnix;
		#endif
		static const Byte kMadeByHostOS = kHostOS_ID;
		static const Byte kExtractHostOS = kHostOS_ID;
		static const Byte kMethodForDirectory = NFileHeader::NCompressionMethod::kStore;

		static void AddAesExtra(CItem &item, Byte aesKeyMode, uint16 method)
		{
			CWzAesExtra wzAesField;
			wzAesField.Strength = aesKeyMode;
			wzAesField.Method = method;
			item.Method = NFileHeader::NCompressionMethod::kWzAES;
			item.Crc = 0;
			CExtraSubBlock sb;
			wzAesField.SetSubBlock(sb);
			item.LocalExtra.SubBlocks.Add(sb);
			item.CentralExtra.SubBlocks.Add(sb);
		}

		CUpdateItem::CUpdateItem() : NtfsTimeIsDefined(false), IsUtf8(false), Size(0) 
		{
		}

		void CUpdateItem::Clear()
		{
			IsDir = false;
			NtfsTimeIsDefined = false;
			IsUtf8 = false;
			Size = 0;
			Name.Empty();
			Comment.Free();
		}

		static void SetFileHeader(const CCompressionMethodMode &options, const CUpdateItem &ui, /*bool isSeqMode,*/ CItemOut &item)
		{
			item.Size = ui.Size;
			bool isDir = ui.IsDir;
			item.ClearFlags();
			if(ui.NewProps) {
				item.Name = ui.Name;
				item.Comment = ui.Comment;
				item.SetUtf8(ui.IsUtf8);
				item.ExternalAttrib = ui.Attrib;
				item.Time = ui.Time;
				item.Ntfs_MTime = ui.Ntfs_MTime;
				item.Ntfs_ATime = ui.Ntfs_ATime;
				item.Ntfs_CTime = ui.Ntfs_CTime;
				item.NtfsTimeIsDefined = ui.NtfsTimeIsDefined;
			}
			/*
			   else
			   isDir = item.IsDir();
			 */

			item.MadeByVersion.HostOS = kMadeByHostOS;
			item.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;

			item.ExtractVersion.HostOS = kExtractHostOS;

			item.InternalAttrib = 0; // test it
			item.SetEncrypted(!isDir && options.PasswordIsDefined);
			// item.SetDescriptorMode(isSeqMode);

			if(isDir) {
				item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
				item.Method = kMethodForDirectory;
				item.PackSize = 0;
				item.Size = 0;
				item.Crc = 0;
			}

			item.LocalExtra.Clear();
			item.CentralExtra.Clear();

			if(isDir) {
				item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
				item.Method = kMethodForDirectory;
				item.PackSize = 0;
				item.Size = 0;
				item.Crc = 0;
			}
			else if(options.IsRealAesMode())
				AddAesExtra(item, options.AesKeyMode, (Byte)(options.MethodSequence.IsEmpty() ? 8 : options.MethodSequence[0]));
		}

		// we call SetItemInfoFromCompressingResult() after SetFileHeader()

		static void SetItemInfoFromCompressingResult(const CCompressingResult &compressingResult, bool isAesMode, Byte aesKeyMode, CItem &item)
		{
			item.ExtractVersion.Version = compressingResult.ExtractVersion;
			item.Method = compressingResult.Method;
			if(compressingResult.Method == NFileHeader::NCompressionMethod::kLZMA && compressingResult.LzmaEos)
				item.Flags |= NFileHeader::NFlags::kLzmaEOS;
			item.Crc = compressingResult.CRC;
			item.Size = compressingResult.UnpackSize;
			item.PackSize = compressingResult.PackSize;
			item.LocalExtra.Clear();
			item.CentralExtra.Clear();
			if(isAesMode)
				AddAesExtra(item, aesKeyMode, compressingResult.Method);
		}

		#ifndef _7ZIP_ST
			//static THREAD_FUNC_DECL CoderThread(void * threadCoderInfo);

			struct CThreadInfo {
				DECL_EXTERNAL_CODECS_LOC_VARS2;

				NWindows::CThread Thread;
				NWindows::NSynchronization::CAutoResetEvent CompressEvent;
				NWindows::NSynchronization::CAutoResetEvent CompressionCompletedEvent;
				CMtCompressProgress * ProgressSpec;
				CMyComPtr <ICompressProgressInfo> Progress;
				COutMemStream * OutStreamSpec;
				CMyComPtr <IOutStream> OutStream;
				CMyComPtr <ISequentialInStream> InStream;
				CAddCommon Coder;
				HRESULT Result;
				CCompressingResult CompressingResult;
				int    ExitThread;
				//bool   ExitThread;
				bool   SeqMode;
				bool   IsFree;
				uint8  Reserve[2]; // @alignment
				uint32 UpdateIndex;
				uint32 FileTime;

				CThreadInfo(const CCompressionMethodMode &options) : ExitThread(false), ProgressSpec(0), OutStreamSpec(0), 
					Coder(options), SeqMode(false), FileTime(0)
				{
				}
				HRESULT CreateEvents()
				{
					RINOK(CompressEvent.CreateIfNotCreated());
					return CompressionCompletedEvent.CreateIfNotCreated();
				}
				static THREAD_FUNC_DECL CoderThread(void * threadCoderInfo)
				{
					((CThreadInfo*)threadCoderInfo)->WaitAndCode();
					return 0;
				}
				HRes CreateThread() { return Thread.Create(CThreadInfo::CoderThread, this); }
				void WaitAndCode()
				{
					for(;; ) {
						CompressEvent.Lock();
						if(!ExitThread) {
							Result = Coder.Compress(EXTERNAL_CODECS_LOC_VARS InStream, OutStream, SeqMode, FileTime, Progress, CompressingResult);
							if(Result == S_OK && Progress)
								Result = Progress->SetRatioInfo(&CompressingResult.UnpackSize, &CompressingResult.PackSize);
							CompressionCompletedEvent.Set();
						}
						else
							break;
					}
				}
				void StopWaitClose()
				{
					ExitThread = true;
					CALLPTRMEMB(OutStreamSpec, StopWriting(E_ABORT));
					if(CompressEvent.IsCreated())
						CompressEvent.Set();
					Thread.Wait();
					Thread.Close();
				}
			};

			struct CMemBlocks2 : public CMemLockBlocks {
				CMemBlocks2() : Defined(false), Skip(false) 
				{
				}
				CCompressingResult CompressingResult;
				bool Defined;	
				bool Skip;
			};

			class CMtProgressMixer2 : public ICompressProgressInfo, public CMyUnknownImp {
				uint64 ProgressOffset;
				uint64 InSizes[2];
				uint64 OutSizes[2];
				CMyComPtr<IProgress> Progress;
				CMyComPtr<ICompressProgressInfo> RatioProgress;
				bool _inSizeIsMain;
			public:
				NWindows::NSynchronization::CCriticalSection CriticalSection;
				MY_UNKNOWN_IMP
				void Create(IProgress * progress, bool inSizeIsMain);
				void SetProgressOffset(uint64 progressOffset);
				HRESULT SetRatioInfo(uint index, const uint64 * inSize, const uint64 * outSize);
				STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
			};

			void CMtProgressMixer2::Create(IProgress * progress, bool inSizeIsMain)
			{
				Progress = progress;
				Progress.QueryInterface(IID_ICompressProgressInfo, &RatioProgress);
				_inSizeIsMain = inSizeIsMain;
				ProgressOffset = InSizes[0] = InSizes[1] = OutSizes[0] = OutSizes[1] = 0;
			}

			void CMtProgressMixer2::SetProgressOffset(uint64 progressOffset)
			{
				CriticalSection.Enter();
				InSizes[1] = OutSizes[1] = 0;
				ProgressOffset = progressOffset;
				CriticalSection.Leave();
			}

			HRESULT CMtProgressMixer2::SetRatioInfo(uint index, const uint64 * inSize, const uint64 * outSize)
			{
				NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
				if(index == 0 && RatioProgress) {
					RINOK(RatioProgress->SetRatioInfo(inSize, outSize));
				}
				RVALUEPTR(InSizes[index], inSize);
				RVALUEPTR(OutSizes[index], outSize);
				uint64 v = ProgressOffset + (_inSizeIsMain  ? (InSizes[0] + InSizes[1]) : (OutSizes[0] + OutSizes[1]));
				return Progress->SetCompleted(&v);
			}

			STDMETHODIMP CMtProgressMixer2::SetRatioInfo(const uint64 * inSize, const uint64 * outSize)
			{
				return SetRatioInfo(0, inSize, outSize);
			}
		#endif

		static HRESULT UpdateItemOldData(COutArchive &archive, CInArchive * inArchive, const CItemEx &itemEx, const CUpdateItem &ui,
			CItemOut &item, /* bool izZip64, */ ICompressProgressInfo * progress, IArchiveUpdateCallbackFile * opCallback, uint64 &complexity)
		{
			if(opCallback) {
				RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, (uint32)ui.IndexInArc, NUpdateNotifyOp::kReplicate))
			}
			uint64 rangeSize;
			if(ui.NewProps) {
				if(item.HasDescriptor())
					return E_NOTIMPL;
				// use old name size.

				// we keep ExternalAttrib and some another properties from old archive
				// item.ExternalAttrib = ui.Attrib;

				// if we don't change Comment, we keep Comment from OldProperties
				item.Comment = ui.Comment;
				item.Name = ui.Name;
				item.SetUtf8(ui.IsUtf8);
				item.Time = ui.Time;
				item.Ntfs_MTime = ui.Ntfs_MTime;
				item.Ntfs_ATime = ui.Ntfs_ATime;
				item.Ntfs_CTime = ui.Ntfs_CTime;
				item.NtfsTimeIsDefined = ui.NtfsTimeIsDefined;

				item.CentralExtra.RemoveUnknownSubBlocks();
				item.LocalExtra.RemoveUnknownSubBlocks();

				archive.WriteLocalHeader(item);
				rangeSize = item.GetPackSizeWithDescriptor();
			}
			else {
				item.LocalHeaderPos = archive.GetCurPos();
				rangeSize = itemEx.GetLocalFullSize();
			}
			CMyComPtr <ISequentialInStream> packStream;
			RINOK(inArchive->GetItemStream(itemEx, ui.NewProps, packStream));
			if(!packStream)
				return E_NOTIMPL;
			complexity += rangeSize;
			CMyComPtr <ISequentialOutStream> outStream;
			archive.CreateStreamForCopying(outStream);
			HRESULT res = NCompress::CopyStream_ExactSize(packStream, outStream, rangeSize, progress);
			archive.MoveCurPos(rangeSize);
			return res;
		}
		static void WriteDirHeader(COutArchive &archive, const CCompressionMethodMode * options, const CUpdateItem &ui, CItemOut &item)
		{
			SetFileHeader(*options, ui, item);
			archive.WriteLocalHeader(item);
		}
		static inline bool IsZero_FILETIME(const FILETIME &ft)
		{
			return (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0);
		}
		static void UpdatePropsFromStream(CUpdateItem &item, ISequentialInStream * fileInStream, IArchiveUpdateCallback * updateCallback, uint64 &totalComplexity)
		{
			CMyComPtr<IStreamGetProps> getProps;
			fileInStream->QueryInterface(IID_IStreamGetProps, (void **)&getProps);
			if(!getProps)
				return;
			FILETIME cTime, aTime, mTime;
			uint64 size;
			// uint32 attrib;
			if(getProps->GetProps(&size, &cTime, &aTime, &mTime, NULL) != S_OK)
				return;
			if(size != item.Size && size != static_cast<uint64>(-1LL)) {
				int64 newComplexity = totalComplexity + ((int64)size - (int64)item.Size);
				if(newComplexity > 0) {
					totalComplexity = newComplexity;
					updateCallback->SetTotal(totalComplexity);
				}
				item.Size = size;
			}
			if(!IsZero_FILETIME(mTime)) {
				item.Ntfs_MTime = mTime;
				FILETIME loc = { 0, 0 };
				if(FileTimeToLocalFileTime(&mTime, &loc)) {
					item.Time = 0;
					NTime::FileTimeToDosTime(loc, item.Time);
				}
			}
			if(!IsZero_FILETIME(cTime)) 
				item.Ntfs_CTime = cTime;
			if(!IsZero_FILETIME(aTime)) 
				item.Ntfs_ATime = aTime;
			// item.Attrib = attrib;
		}

		static HRESULT Update2St(DECL_EXTERNAL_CODECS_LOC_VARS COutArchive &archive, CInArchive * inArchive,
			const CObjectVector<CItemEx> &inputItems, CObjectVector<CUpdateItem> &updateItems,
			const CCompressionMethodMode * options, const CByteBuffer * comment,
			IArchiveUpdateCallback * updateCallback, uint64 &totalComplexity, IArchiveUpdateCallbackFile * opCallback)
		{
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(updateCallback, true);
			CAddCommon compressor(*options);
			CObjectVector<CItemOut> items;
			uint64 unpackSizeTotal = 0, packSizeTotal = 0;
			FOR_VECTOR(itemIndex, updateItems) {
				lps->InSize = unpackSizeTotal;
				lps->OutSize = packSizeTotal;
				RINOK(lps->SetCur());
				CUpdateItem &ui = updateItems[itemIndex];
				CItemEx itemEx;
				CItemOut item;
				if(!ui.NewProps || !ui.NewData) {
					// Note: for(ui.NewProps && !ui.NewData) it copies Props from old archive,
					// But we will rewrite all important properties later. But we can keep some properties like
					// Comment
					itemEx = inputItems[ui.IndexInArc];
					if(inArchive->ReadLocalItemAfterCdItemFull(itemEx) != S_OK)
						return E_NOTIMPL;
					(CItem &)item = itemEx;
				}
				if(ui.NewData) {
					// bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
					bool isDir = ui.IsDir;
					if(isDir) {
						WriteDirHeader(archive, options, ui, item);
					}
					else {
						CMyComPtr<ISequentialInStream> fileInStream;
						HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
						if(res == S_FALSE) {
							lps->ProgressOffset += ui.Size;
							RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
							continue;
						}
						RINOK(res);
						if(!fileInStream)
							return E_INVALIDARG;
						bool seqMode;
						{
							CMyComPtr<IInStream> inStream2;
							fileInStream->QueryInterface(IID_IInStream, (void **)&inStream2);
							seqMode = (inStream2 == NULL);
						}
						// seqMode = true; // to test seqMode
						UpdatePropsFromStream(ui, fileInStream, updateCallback, totalComplexity);
						SetFileHeader(*options, ui, item);
						item.SetDescriptorMode(seqMode);
						// file Size can be 64-bit !!!
						CCompressingResult compressingResult;
						RINOK(compressor.Set_Pre_CompressionResult(seqMode, ui.Size, compressingResult));
						SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);
						archive.WriteLocalHeader(item);
						CMyComPtr<IOutStream> outStream;
						archive.CreateStreamForCompressing(outStream);
						RINOK(compressor.Compress(EXTERNAL_CODECS_LOC_VARS fileInStream, outStream, seqMode, ui.Time, progress, compressingResult));
						if(compressingResult.FileTimeWasUsed) {
							/*
							   if(!item.HasDescriptor())
							   return E_FAIL;
							 */
							item.SetDescriptorMode(true);
						}
						SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);
						archive.WriteLocalHeader_Replace(item);
						RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
						unpackSizeTotal += item.Size;
						packSizeTotal += item.PackSize;
					}
				}
				else {
					uint64 complexity = 0;
					lps->SendRatio = false;
					RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity));
					lps->SendRatio = true;
					lps->ProgressOffset += complexity;
				}
				items.Add(item);
				lps->ProgressOffset += kLocalHeaderSize;
			}
			lps->InSize = unpackSizeTotal;
			lps->OutSize = packSizeTotal;
			RINOK(lps->SetCur());
			archive.WriteCentralDir(items, comment);
			return S_OK;
		}

		static HRESULT Update2(DECL_EXTERNAL_CODECS_LOC_VARS COutArchive &archive,
			CInArchive * inArchive, const CObjectVector<CItemEx> &inputItems, CObjectVector<CUpdateItem> &updateItems,
			const CCompressionMethodMode &options, const CByteBuffer * comment, IArchiveUpdateCallback * updateCallback)
		{
			CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
			updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
			bool unknownComplexity = false;
			uint64 complexity = 0;
			uint64 numFilesToCompress = 0;
			uint64 numBytesToCompress = 0;
			uint i;
			for(i = 0; i < updateItems.Size(); i++) {
				const CUpdateItem &ui = updateItems[i];
				if(ui.NewData) {
					if(ui.Size == static_cast<uint64>(-1LL))
						unknownComplexity = true;
					else
						complexity += ui.Size;
					numBytesToCompress += ui.Size;
					numFilesToCompress++;
					/*
					   if(ui.Commented)
					   complexity += ui.CommentRange.Size;
					 */
				}
				else {
					CItemEx inputItem = inputItems[ui.IndexInArc];
					if(inArchive->ReadLocalItemAfterCdItemFull(inputItem) != S_OK)
						return E_NOTIMPL;
					complexity += inputItem.GetLocalFullSize();
					// complexity += inputItem.GetCentralExtraPlusCommentSize();
				}
				complexity += kLocalHeaderSize;
				complexity += kCentralHeaderSize;
			}
			if(comment)
				complexity += comment->Size();
			complexity++; // end of central
			if(!unknownComplexity)
				updateCallback->SetTotal(complexity);
			uint64 totalComplexity = complexity;
			CCompressionMethodMode options2 = options;
			if(options2._methods.IsEmpty()) {
				// we need method item, if default method was used
				options2._methods.AddNew();
			}
			CAddCommon compressor(options2);
			complexity = 0;
			const Byte method = options.MethodSequence.Front();
			COneMethodInfo * oneMethodMain = NULL;
			if(!options2._methods.IsEmpty())
				oneMethodMain = &options2._methods[0];
			{
				FOR_VECTOR(mi, options2._methods) {
					options2.SetGlobalLevelTo(options2._methods[mi]);
				}
			}
			if(oneMethodMain) {
				// appnote recommends to use EOS marker for LZMA.
				if(method == NFileHeader::NCompressionMethod::kLZMA)
					oneMethodMain->AddProp_EndMarker_if_NotFound(true);
			}
		  #ifndef _7ZIP_ST
			uint32 numThreads = options._numThreads;
			const uint32 kNumMaxThreads = 64;
			SETMIN(numThreads, kNumMaxThreads);
			SETMIN(numThreads, MAXIMUM_WAIT_OBJECTS); // is 64 in Windows (is it 64 in all versions?)
			SETMAX(numThreads, 1);
			const size_t kMemPerThread = (1 << 25);
			const size_t kBlockSize = 1 << 16;
			bool mtMode = (numThreads > 1);
			if(numFilesToCompress <= 1)
				mtMode = false;
			if(!mtMode) {
				FOR_VECTOR(mi, options2._methods) {
					COneMethodInfo &onem = options2._methods[mi];
					if(onem.FindProp(NCoderPropID::kNumThreads) < 0) {
						// fixed for 9.31. bzip2 default is just one thread.
						onem.AddProp_NumThreads(numThreads);
					}
				}
			}
			else {
				if(method == NFileHeader::NCompressionMethod::kStore && !options.PasswordIsDefined)
					numThreads = 1;
				if(oneMethodMain) {
					if(method == NFileHeader::NCompressionMethod::kBZip2) {
						bool fixedNumber;
						uint32 numBZip2Threads = oneMethodMain->Get_BZip2_NumThreads(fixedNumber);
						if(!fixedNumber) {
							const uint64 averageSize = numBytesToCompress / numFilesToCompress;
							const uint32 blockSize = oneMethodMain->Get_BZip2_BlockSize();
							const uint64 averageNumberOfBlocks = averageSize / blockSize + 1;
							numBZip2Threads = 32;
							SETMIN(numBZip2Threads, (uint32)averageNumberOfBlocks);
							oneMethodMain->AddProp_NumThreads(numBZip2Threads);
						}
						numThreads /= numBZip2Threads;
					}
					if(method == NFileHeader::NCompressionMethod::kXz) {
						bool   fixedNumber;
						uint32 numLzma2Threads = oneMethodMain->Get_Lzma2_NumThreads(fixedNumber);
						if(!fixedNumber) {
							const uint64 averageSize = numBytesToCompress / numFilesToCompress;
							const uint64 blockSize = oneMethodMain->Get_Lzma2_BlockSize();
							const uint64 averageNumberOfBlocks = averageSize / blockSize + 1;
							numLzma2Threads = 2;
							SETMIN(numLzma2Threads, (uint32)averageNumberOfBlocks);
							oneMethodMain->AddProp_NumThreads(numLzma2Threads);
						}
						numThreads /= numLzma2Threads;
					}
					if(method == NFileHeader::NCompressionMethod::kLZMA) {
						// we suppose that default LZMA is 2 thread. So we don't change it
						uint32 numLZMAThreads = oneMethodMain->Get_Lzma_NumThreads();
						numThreads /= numLZMAThreads;
					}
				}
				SETMIN(numThreads, (uint32)numFilesToCompress);
				if(numThreads <= 1)
					mtMode = false;
			}
			if(!mtMode)
		  #endif
			return Update2St(EXTERNAL_CODECS_LOC_VARS archive, inArchive, inputItems, updateItems, &options2, comment, updateCallback, totalComplexity, opCallback);
		  #ifndef _7ZIP_ST
			class CMtProgressMixer : public ICompressProgressInfo, public CMyUnknownImp {
			public:
				void Create(IProgress * progress, bool inSizeIsMain)
				{
					Mixer2 = new CMtProgressMixer2;
					RatioProgress = Mixer2;
					Mixer2->Create(progress, inSizeIsMain);
				}
				MY_UNKNOWN_IMP
				STDMETHOD(SetRatioInfo)(const uint64 *inSize, const uint64 *outSize)
				{
					return Mixer2->SetRatioInfo(1, inSize, outSize);
				}
				CMtProgressMixer2 * Mixer2;
				CMyComPtr <ICompressProgressInfo> RatioProgress;
			};
			CMtProgressMixer * mtProgressMixerSpec = new CMtProgressMixer;
			CObjectVector <CItemOut> items;
			CMyComPtr<ICompressProgressInfo> progress = mtProgressMixerSpec;
			mtProgressMixerSpec->Create(updateCallback, true);
			CMtCompressProgressMixer mtCompressProgressMixer;
			mtCompressProgressMixer.Init(numThreads, mtProgressMixerSpec->RatioProgress);

			CMemBlockManagerMt memManager(kBlockSize);
			
			class CMemRefs {
			public:
				CMemRefs(CMemBlockManagerMt * manager) : Manager(manager) {}
				~CMemRefs() { FOR_VECTOR(i, Refs) { Refs[i].FreeOpt(Manager); } }
				CMemBlockManagerMt * Manager;
				CObjectVector <CMemBlocks2> Refs;
			} refs(&memManager);

			class CThreads {
			public:
				~CThreads() { FOR_VECTOR(i, Threads) { Threads[i].StopWaitClose(); } }
				CObjectVector <CThreadInfo> Threads;
			} threads;
			CRecordVector <HANDLE> compressingCompletedEvents;
			CUIntVector threadIndices; // list threads in order of updateItems
			{
				RINOK(memManager.AllocateSpaceAlways((size_t)numThreads * (kMemPerThread / kBlockSize)));
				for(i = 0; i < updateItems.Size(); i++)
					refs.Refs.Add(CMemBlocks2());
				for(i = 0; i < numThreads; i++)
					threads.Threads.Add(CThreadInfo(options2));
				for(i = 0; i < numThreads; i++) {
					CThreadInfo & threadInfo = threads.Threads[i];
			  #ifdef EXTERNAL_CODECS
					threadInfo.__externalCodecs = __externalCodecs;
			  #endif
					RINOK(threadInfo.CreateEvents());
					threadInfo.OutStreamSpec = new COutMemStream(&memManager);
					RINOK(threadInfo.OutStreamSpec->CreateEvents());
					threadInfo.OutStream = threadInfo.OutStreamSpec;
					threadInfo.IsFree = true;
					threadInfo.ProgressSpec = new CMtCompressProgress();
					threadInfo.Progress = threadInfo.ProgressSpec;
					threadInfo.ProgressSpec->Init(&mtCompressProgressMixer, (int)i);
					threadInfo.SeqMode = false; // fix it !
					threadInfo.FileTime = 0; // fix it !
					RINOK(threadInfo.CreateThread());
				}
			}
			uint   mtItemIndex = 0;
			uint   itemIndex = 0;
			int    lastRealStreamItemIndex = -1;
			while(itemIndex < updateItems.Size()) {
				if(threadIndices.Size() < numThreads && mtItemIndex < updateItems.Size()) {
					CUpdateItem & ui = updateItems[mtItemIndex++];
					if(!ui.NewData)
						continue;
					CItemEx itemEx;
					CItemOut item;
					if(ui.NewProps) {
						if(ui.IsDir)
							continue;
					}
					else {
						itemEx = inputItems[ui.IndexInArc];
						if(inArchive->ReadLocalItemAfterCdItemFull(itemEx) != S_OK)
							return E_NOTIMPL;
						(CItem &)item = itemEx;
						if(item.IsDir() != ui.IsDir)
							return E_NOTIMPL;
						if(ui.IsDir)
							continue;
					}
					CMyComPtr <ISequentialInStream> fileInStream;
					{
						NWindows::NSynchronization::CCriticalSectionLock lock(mtProgressMixerSpec->Mixer2->CriticalSection);
						HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
						if(res == S_FALSE) {
							complexity += ui.Size;
							complexity += kLocalHeaderSize;
							mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
							RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
							refs.Refs[mtItemIndex - 1].Skip = true;
							continue;
						}
						RINOK(res);
						if(!fileInStream)
							return E_INVALIDARG;
						UpdatePropsFromStream(ui, fileInStream, updateCallback, totalComplexity);
						RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
					}

					for(uint32 k = 0; k < numThreads; k++) {
						CThreadInfo &threadInfo = threads.Threads[k];
						if(threadInfo.IsFree) {
							threadInfo.IsFree = false;
							threadInfo.InStream = fileInStream;
							// !!!!! we must release ref before sending event
							// BUG was here in v4.43 and v4.44. It could change ref counter in two threads
							// in same time
							fileInStream.Release();
							threadInfo.OutStreamSpec->Init();
							threadInfo.ProgressSpec->Reinit();
							threadInfo.CompressEvent.Set();
							threadInfo.UpdateIndex = mtItemIndex - 1;
							compressingCompletedEvents.Add(threadInfo.CompressionCompletedEvent);
							threadIndices.Add(k);
							break;
						}
					}
					continue;
				}
				if(refs.Refs[itemIndex].Skip) {
					itemIndex++;
					continue;
				}
				const CUpdateItem &ui = updateItems[itemIndex];
				CItemEx itemEx;
				CItemOut item;
				if(!ui.NewProps || !ui.NewData) {
					itemEx = inputItems[ui.IndexInArc];
					if(inArchive->ReadLocalItemAfterCdItemFull(itemEx) != S_OK)
						return E_NOTIMPL;
					(CItem &)item = itemEx;
				}
				if(ui.NewData) {
					// bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
					bool isDir = ui.IsDir;
					if(isDir) {
						WriteDirHeader(archive, &options, ui, item);
					}
					else {
						CMemBlocks2 & memRef = refs.Refs[itemIndex];
						if(memRef.Defined) {
							SETMAX(lastRealStreamItemIndex, (int)itemIndex);
							SetFileHeader(options, ui, item);
							// the BUG was fixed in 9.26:
							// SetItemInfoFromCompressingResult must be after SetFileHeader
							// to write correct Size.
							SetItemInfoFromCompressingResult(memRef.CompressingResult, options.IsRealAesMode(), options.AesKeyMode, item);
							archive.WriteLocalHeader(item);
							//
							// RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
							CMyComPtr<ISequentialOutStream> outStream;
							archive.CreateStreamForCopying(outStream);
							memRef.WriteToStream(memManager.GetBlockSize(), outStream);
							archive.MoveCurPos(item.PackSize);
							memRef.FreeOpt(&memManager);
						}
						else {
							if(lastRealStreamItemIndex < (int)itemIndex) {
								lastRealStreamItemIndex = itemIndex;
								SetFileHeader(options, ui, item);
								CCompressingResult compressingResult;
								RINOK(compressor.Set_Pre_CompressionResult(false, /*seqMode*/ ui.Size, compressingResult));
								SetItemInfoFromCompressingResult(compressingResult, options.IsRealAesMode(), options.AesKeyMode, item);
								// file Size can be 64-bit !!!
								archive.WriteLocalHeader(item);
							}
							{
								CThreadInfo &thread = threads.Threads[threadIndices.Front()];
								if(!thread.OutStreamSpec->WasUnlockEventSent()) {
									CMyComPtr<IOutStream> outStream;
									archive.CreateStreamForCompressing(outStream);
									thread.OutStreamSpec->SetOutStream(outStream);
									thread.OutStreamSpec->SetRealStreamMode();
								}
							}
							DWORD result = ::WaitForMultipleObjects(compressingCompletedEvents.Size(), &compressingCompletedEvents.Front(), FALSE, INFINITE);
							if(result == WAIT_FAILED) {
								DWORD lastError = GetLastError();
								return lastError != 0 ? lastError : E_FAIL;
							}
							unsigned t = (uint)(result - WAIT_OBJECT_0);
							if(t >= compressingCompletedEvents.Size())
								return E_FAIL;
							CThreadInfo & threadInfo = threads.Threads[threadIndices[t]];
							threadInfo.InStream.Release();
							threadInfo.IsFree = true;
							RINOK(threadInfo.Result);
							threadIndices.Delete(t);
							compressingCompletedEvents.Delete(t);
							if(t == 0) {
								RINOK(threadInfo.OutStreamSpec->WriteToRealStream());
								threadInfo.OutStreamSpec->ReleaseOutStream();
								SetFileHeader(options, ui, item);
								SetItemInfoFromCompressingResult(threadInfo.CompressingResult, options.IsRealAesMode(), options.AesKeyMode, item);
								archive.WriteLocalHeader_Replace(item);
							}
							else {
								CMemBlocks2 & memRef2 = refs.Refs[threadInfo.UpdateIndex];
								threadInfo.OutStreamSpec->DetachData(memRef2);
								memRef2.CompressingResult = threadInfo.CompressingResult;
								memRef2.Defined = true;
								continue;
							}
						}
					}
				}
				else {
					RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity));
				}

				items.Add(item);
				complexity += kLocalHeaderSize;
				mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
				itemIndex++;
			}
			RINOK(mtCompressProgressMixer.SetRatioInfo(0, NULL, NULL));
			archive.WriteCentralDir(items, comment);
			return S_OK;
		  #endif
		}

		static const size_t kCacheBlockSize = (1 << 20);
		static const size_t kCacheSize = (kCacheBlockSize << 2);
		static const size_t kCacheMask = (kCacheSize - 1);

		class CCacheOutStream : public IOutStream, public CMyUnknownImp {
			CMyComPtr<IOutStream> _stream;
			Byte * _cache;
			uint64 _virtPos;
			uint64 _virtSize;
			uint64 _phyPos;
			uint64 _phySize; // <= _virtSize
			uint64 _cachedPos; // (_cachedPos + _cachedSize) <= _virtSize
			size_t _cachedSize;
			HRESULT MyWrite(size_t size);
			HRESULT MyWriteBlock()
			{
				return MyWrite(kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1)));
			}
			HRESULT FlushCache();
		public:
			CCacheOutStream() : _cache(0) 
			{
			}
			~CCacheOutStream();
			bool Allocate();
			HRESULT Init(IOutStream * stream);

			MY_UNKNOWN_IMP

			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
			STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
			STDMETHOD(SetSize) (uint64 newSize);
		};

		bool CCacheOutStream::Allocate()
		{
			return SETIFZ(_cache, static_cast<Byte *>(::MidAlloc(kCacheSize)));
		}

		HRESULT CCacheOutStream::Init(IOutStream * stream)
		{
			_virtPos = _phyPos = 0;
			_stream = stream;
			RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &_virtPos));
			RINOK(_stream->Seek(0, STREAM_SEEK_END, &_virtSize));
			RINOK(_stream->Seek(_virtPos, STREAM_SEEK_SET, &_virtPos));
			_phyPos = _virtPos;
			_phySize = _virtSize;
			_cachedPos = 0;
			_cachedSize = 0;
			return S_OK;
		}

		HRESULT CCacheOutStream::MyWrite(size_t size)
		{
			while(size != 0 && _cachedSize != 0) {
				if(_phyPos != _cachedPos) {
					RINOK(_stream->Seek(_cachedPos, STREAM_SEEK_SET, &_phyPos));
				}
				size_t pos = (size_t)_cachedPos & kCacheMask;
				size_t curSize = MyMin(kCacheSize - pos, _cachedSize);
				curSize = MyMin(curSize, size);
				RINOK(WriteStream(_stream, _cache + pos, curSize));
				_phyPos += curSize;
				SETMAX(_phySize, _phyPos);
				_cachedPos += curSize;
				_cachedSize -= curSize;
				size -= curSize;
			}
			return S_OK;
		}

		HRESULT CCacheOutStream::FlushCache()
		{
			return MyWrite(_cachedSize);
		}

		CCacheOutStream::~CCacheOutStream()
		{
			FlushCache();
			if(_virtSize != _phySize)
				_stream->SetSize(_virtSize);
			if(_virtPos != _phyPos)
				_stream->Seek(_virtPos, STREAM_SEEK_SET, NULL);
			::MidFree(_cache);
		}

		STDMETHODIMP CCacheOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			if(size == 0)
				return S_OK;
			uint64 zerosStart = _virtPos;
			if(_cachedSize != 0) {
				if(_virtPos < _cachedPos) {
					RINOK(FlushCache());
				}
				else {
					uint64 cachedEnd = _cachedPos + _cachedSize;
					if(cachedEnd < _virtPos) {
						if(cachedEnd < _phySize) {
							RINOK(FlushCache());
						}
						else
							zerosStart = cachedEnd;
					}
				}
			}
			if(_cachedSize == 0 && _phySize < _virtPos)
				_cachedPos = zerosStart = _phySize;
			if(zerosStart != _virtPos) {
				// write zeros to [cachedEnd ... _virtPos)
				for(;; ) {
					uint64 cachedEnd = _cachedPos + _cachedSize;
					size_t endPos = (size_t)cachedEnd & kCacheMask;
					size_t curSize = kCacheSize - endPos;
					if(curSize > _virtPos - cachedEnd)
						curSize = (size_t)(_virtPos - cachedEnd);
					if(curSize == 0)
						break;
					while(curSize > (kCacheSize - _cachedSize)) {
						RINOK(MyWriteBlock());
					}
					memzero(_cache + endPos, curSize);
					_cachedSize += curSize;
				}
			}
			if(_cachedSize == 0)
				_cachedPos = _virtPos;
			size_t pos = (size_t)_virtPos & kCacheMask;
			size = (uint32)MyMin((size_t)size, kCacheSize - pos);
			uint64 cachedEnd = _cachedPos + _cachedSize;
			if(_virtPos != cachedEnd) // _virtPos < cachedEnd
				size = (uint32)MyMin((size_t)size, (size_t)(cachedEnd - _virtPos));
			else {
				// _virtPos == cachedEnd
				if(_cachedSize == kCacheSize) {
					RINOK(MyWriteBlock());
				}
				size_t startPos = (size_t)_cachedPos & kCacheMask;
				if(startPos > pos)
					size = (uint32)MyMin((size_t)size, (size_t)(startPos - pos));
				_cachedSize += size;
			}
			memcpy(_cache + pos, data, size);
			ASSIGN_PTR(processedSize, size);
			_virtPos += size;
			SETMAX(_virtSize, _virtPos);
			return S_OK;
		}

		STDMETHODIMP CCacheOutStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
		{
			switch(seekOrigin) {
				case STREAM_SEEK_SET: break;
				case STREAM_SEEK_CUR: offset += _virtPos; break;
				case STREAM_SEEK_END: offset += _virtSize; break;
				default: return STG_E_INVALIDFUNCTION;
			}
			if(offset < 0)
				return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
			_virtPos = offset;
			ASSIGN_PTR(newPosition, offset);
			return S_OK;
		}

		STDMETHODIMP CCacheOutStream::SetSize(uint64 newSize)
		{
			_virtSize = newSize;
			if(newSize < _phySize) {
				RINOK(_stream->SetSize(newSize));
				_phySize = newSize;
			}
			if(newSize <= _cachedPos) {
				_cachedSize = 0;
				_cachedPos = newSize;
			}
			if(newSize < _cachedPos + _cachedSize)
				_cachedSize = (size_t)(newSize - _cachedPos);
			return S_OK;
		}

		HRESULT Update(DECL_EXTERNAL_CODECS_LOC_VARS const CObjectVector<CItemEx> &inputItems, CObjectVector<CUpdateItem> &updateItems,
			ISequentialOutStream * seqOutStream, CInArchive * inArchive, bool removeSfx, const CCompressionMethodMode &compressionMethodMode,
			IArchiveUpdateCallback * updateCallback)
		{
			if(inArchive) {
				if(!inArchive->CanUpdate())
					return E_NOTIMPL;
			}
			CMyComPtr <IOutStream> outStream;
			{
				CMyComPtr <IOutStream> outStreamReal;
				seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStreamReal);
				if(!outStreamReal)
					return E_NOTIMPL;
				if(inArchive) {
					if(!inArchive->IsMultiVol && inArchive->ArcInfo.Base > 0 && !removeSfx) {
						IInStream * baseStream = inArchive->GetBaseStream();
						RINOK(baseStream->Seek(0, STREAM_SEEK_SET, NULL));
						RINOK(NCompress::CopyStream_ExactSize(baseStream, outStreamReal, inArchive->ArcInfo.Base, NULL));
					}
				}
				CCacheOutStream * cacheStream = new CCacheOutStream();
				outStream = cacheStream;
				if(!cacheStream->Allocate())
					return E_OUTOFMEMORY;
				RINOK(cacheStream->Init(outStreamReal));
			}
			COutArchive outArchive;
			RINOK(outArchive.Create(outStream));
			if(inArchive) {
				if(!inArchive->IsMultiVol && (int64)inArchive->ArcInfo.MarkerPos2 > inArchive->ArcInfo.Base) {
					IInStream * baseStream = inArchive->GetBaseStream();
					RINOK(baseStream->Seek(inArchive->ArcInfo.Base, STREAM_SEEK_SET, NULL));
					uint64 embStubSize = inArchive->ArcInfo.MarkerPos2 - inArchive->ArcInfo.Base;
					RINOK(NCompress::CopyStream_ExactSize(baseStream, outStream, embStubSize, NULL));
					outArchive.MoveCurPos(embStubSize);
				}
			}
			return Update2(EXTERNAL_CODECS_LOC_VARS outArchive, inArchive, inputItems, updateItems,
				compressionMethodMode, inArchive ? &inArchive->ArcInfo.Comment : NULL, updateCallback);
		}
	}
}
//