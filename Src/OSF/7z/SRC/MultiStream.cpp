// MultiStream.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

CMultiStream::CSubStreamInfo::CSubStreamInfo() : Size(0), GlobalOffset(0), LocalPos(0) 
{
}

HRESULT CMultiStream::Init()
{
	uint64 total = 0;
	FOR_VECTOR(i, Streams) {
		CSubStreamInfo &s = Streams[i];
		s.GlobalOffset = total;
		total += Streams[i].Size;
		RINOK(s.Stream->Seek(0, STREAM_SEEK_CUR, &s.LocalPos));
	}
	_totalLength = total;
	_pos = 0;
	_streamIndex = 0;
	return S_OK;
}

STDMETHODIMP CMultiStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(_pos >= _totalLength)
		return S_OK;
	{
		unsigned left = 0, mid = _streamIndex, right = Streams.Size();
		for(;; ) {
			CSubStreamInfo &m = Streams[mid];
			if(_pos < m.GlobalOffset)
				right = mid;
			else if(_pos >= m.GlobalOffset + m.Size)
				left = mid + 1;
			else {
				_streamIndex = mid;
				break;
			}
			mid = (left + right) / 2;
		}
		_streamIndex = mid;
	}
	CSubStreamInfo &s = Streams[_streamIndex];
	uint64 localPos = _pos - s.GlobalOffset;
	if(localPos != s.LocalPos) {
		RINOK(s.Stream->Seek(localPos, STREAM_SEEK_SET, &s.LocalPos));
	}
	uint64 rem = s.Size - localPos;
	if(size > rem)
		size = (uint32)rem;
	HRESULT result = s.Stream->Read(data, size, &size);
	_pos += size;
	s.LocalPos += size;
	ASSIGN_PTR(processedSize, size);
	return result;
}

STDMETHODIMP CMultiStream::Seek(Int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _pos; break;
		case STREAM_SEEK_END: offset += _totalLength; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	_pos = offset;
	ASSIGN_PTR(newPosition, offset);
	return S_OK;
}

/*
   class COutVolumeStream:
   public ISequentialOutStream,
   public CMyUnknownImp
   {
   unsigned _volIndex;
   uint64 _volSize;
   uint64 _curPos;
   CMyComPtr<ISequentialOutStream> _volumeStream;
   COutArchive _archive;
   CCRC _crc;

   public:
   MY_UNKNOWN_IMP

   CFileItem _file;
   CUpdateOptions _options;
   CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
   void Init(IArchiveUpdateCallback2 *volumeCallback,
      const UString &name)
   {
    _file.Name = name;
    _file.IsStartPosDefined = true;
    _file.StartPos = 0;

    VolumeCallback = volumeCallback;
    _volIndex = 0;
    _volSize = 0;
   }

   HRESULT Flush();
   STDMETHOD(Write)(const void *data, uint32 size, uint32 *processedSize);
   };

   HRESULT COutVolumeStream::Flush()
   {
   if(_volumeStream)
   {
    _file.UnPackSize = _curPos;
    _file.FileCRC = _crc.GetDigest();
    RINOK(WriteVolumeHeader(_archive, _file, _options));
    _archive.Close();
    _volumeStream.Release();
    _file.StartPos += _file.UnPackSize;
   }
   return S_OK;
   }
 */

/*
   STDMETHODIMP COutMultiStream::Write(const void *data, uint32 size, uint32 *processedSize)
   {
   if(processedSize)
   *processedSize = 0;
   while (size > 0) {
    if(_streamIndex >= Streams.Size()) {
      CSubStreamInfo subStream;
      RINOK(VolumeCallback->GetVolumeSize(Streams.Size(), &subStream.Size));
      RINOK(VolumeCallback->GetVolumeStream(Streams.Size(), &subStream.Stream));
      subStream.Pos = 0;
      Streams.Add(subStream);
      continue;
    }
    CSubStreamInfo &subStream = Streams[_streamIndex];
    if(_offsetPos >= subStream.Size) {
      _offsetPos -= subStream.Size;
      _streamIndex++;
      continue;
    }
    if(_offsetPos != subStream.Pos) {
      CMyComPtr<IOutStream> outStream;
      RINOK(subStream.Stream.QueryInterface(IID_IOutStream, &outStream));
      RINOK(outStream->Seek(_offsetPos, STREAM_SEEK_SET, NULL));
      subStream.Pos = _offsetPos;
    }

    uint32 curSize = (uint32)MyMin((uint64)size, subStream.Size - subStream.Pos);
    uint32 realProcessed;
    RINOK(subStream.Stream->Write(data, curSize, &realProcessed));
    data = (void *)((Byte *)data + realProcessed);
    size -= realProcessed;
    subStream.Pos += realProcessed;
    _offsetPos += realProcessed;
    _absPos += realProcessed;
    if(_absPos > _length)
      _length = _absPos;
    if(processedSize)
   *processedSize += realProcessed;
    if(subStream.Pos == subStream.Size) {
      _streamIndex++;
      _offsetPos = 0;
    }
    if(realProcessed != curSize && realProcessed == 0)
      return E_FAIL;
   }
   return S_OK;
   }

   STDMETHODIMP COutMultiStream::Seek(Int64 offset, uint32 seekOrigin, uint64 *newPosition)
   {
   switch (seekOrigin)
   {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _absPos; break;
    case STREAM_SEEK_END: offset += _length; break;
    default: return STG_E_INVALIDFUNCTION;
   }
   if(offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
   _absPos = offset;
   _offsetPos = _absPos;
   _streamIndex = 0;
   if(newPosition)
   *newPosition = offset;
   return S_OK;
   }
 */
