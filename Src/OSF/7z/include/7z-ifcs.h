// 7z-ifcs.h
// @sobolev
// Объединенный заголовочный файл для определений 7z, ссылающихся на com-интефейсы
//
#ifndef __7ZIFCS_H
#define __7ZIFCS_H

#include <7z-internal.h> // for vs inline syntax processing only

//#include <IStream.h>
#define STREAM_INTERFACE_SUB(i, base, x) DECL_INTERFACE_SUB(i, base, 3, x)
#define STREAM_INTERFACE(i, x) STREAM_INTERFACE_SUB(i, IUnknown, x)

STREAM_INTERFACE(ISequentialInStream, 0x01)
{
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize) PURE;
	/*
	   The requirement for caller: (processedSize != NULL).
	   The callee can allow (processedSize == NULL) for compatibility reasons.

	   if(size == 0), this function returns S_OK and (*processedSize) is set to 0.

	   if(size != 0) {
	   Partial read is allowed: (*processedSize <= avail_size && *processedSize <= size),
	    where (avail_size) is the size of remaining bytes in stream.
	   If (avail_size != 0), this function must read at least 1 byte: (*processedSize > 0).
	   You must call Read() in loop, if you need to read exact amount of data.
	   }

	   If seek pointer before Read() call was changed to position past the end of stream:
	   if(seek_pointer >= stream_size), this function returns S_OK and (*processedSize) is set to 0.

	   ERROR CASES:
	   If the function returns error code, then (*processedSize) is size of
	   data written to (data) buffer (it can be data before error or data with errors).
	   The recommended way for callee to work with reading errors:
	    1) write part of data before error to (data) buffer and return S_OK.
	    2) return error code for further calls of Read().
	 */
};

STREAM_INTERFACE(ISequentialOutStream, 0x02)
{
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize) PURE;
	/*
	   The requirement for caller: (processedSize != NULL).
	   The callee can allow (processedSize == NULL) for compatibility reasons.

	   if(size != 0)
	   {
	   Partial write is allowed: (*processedSize <= size),
	   but this function must write at least 1 byte: (*processedSize > 0).
	   You must call Write() in loop, if you need to write exact amount of data.
	   }

	   ERROR CASES:
	   If the function returns error code, then (*processedSize) is size of
	   data written from (data) buffer.
	 */
};

#ifdef __HRESULT_FROM_WIN32
	#define HRESULT_WIN32_ERROR_NEGATIVE_SEEK __HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK)
#else
	#define HRESULT_WIN32_ERROR_NEGATIVE_SEEK   HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK)
#endif
// 
// Seek() Function
// If you seek before the beginning of the stream, Seek() function returns error code:
//   Recommended error code is __HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK).
//   or STG_E_INVALIDFUNCTION
// It is allowed to seek past the end of the stream.
// 
// if Seek() returns error, then the value of *newPosition is undefined.
// 
STREAM_INTERFACE_SUB(IInStream, ISequentialInStream, 0x03) { STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition) PURE; };
STREAM_INTERFACE_SUB(IOutStream, ISequentialOutStream, 0x04) { STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition) PURE; STDMETHOD(SetSize) (uint64 newSize) PURE; };
STREAM_INTERFACE(IStreamGetSize, 0x06) { STDMETHOD(GetSize) (uint64 *size) PURE; };
STREAM_INTERFACE(IOutStreamFinish, 0x07) { STDMETHOD(OutStreamFinish) () PURE; };
STREAM_INTERFACE(IStreamGetProps, 0x08) { STDMETHOD(GetProps) (uint64 *size, FILETIME *cTime, FILETIME *aTime, FILETIME *mTime, uint32 *attrib) PURE; };

STREAM_INTERFACE(IStreamGetProps2, 0x09) { STDMETHOD(GetProps2) (CStreamFileProps *props) PURE; };
//
//#include <FileStreams.h>
#ifdef _WIN32
	#define USE_WIN_FILE
#endif
#ifdef USE_WIN_FILE
	//#include <Windows/FileIO.h>
#else
	//#include <C_FileIO.h>
	#ifdef _WIN32
		#ifdef _MSC_VER
			typedef size_t ssize_t;
		#endif
	#endif
	namespace NC {
		namespace NFile {
			namespace NIO {

				class CFileBase {
				protected:
					int _handle;
					bool OpenBinary(const char * name, int flags);
				public:
					CFileBase() : _handle(-1) 
					{
					};
					~CFileBase() 
					{
						Close();
					}
					bool Close();
					bool GetLength(uint64 &length) const;
					off_t Seek(off_t distanceToMove, int moveMethod) const;
				};

				class CInFile : public CFileBase {
				public:
					bool Open(const char * name);
					bool OpenShared(const char * name, bool shareForWrite);
					ssize_t Read(void * data, size_t size);
				};

				class COutFile : public CFileBase {
				public:
					bool Create(const char * name, bool createAlways);
					bool Open(const char * name, DWORD creationDisposition);
					ssize_t Write(const void * data, size_t size);
				};
			}
		}
	}
	//
#endif
//#include <IStream.h>

#ifdef _WIN32
	typedef UINT_PTR My_UINT_PTR;
#else
	typedef UINT My_UINT_PTR;
#endif

struct IInFileStream_Callback {
	virtual HRESULT InFileStream_On_Error(My_UINT_PTR val, DWORD error) = 0;
	virtual void InFileStream_On_Destroy(My_UINT_PTR val) = 0;
};

class CInFileStream : public IInStream, public IStreamGetSize,
  #ifdef USE_WIN_FILE
	public IStreamGetProps,
	public IStreamGetProps2,
  #endif
	public CMyUnknownImp {
public:
  #ifdef USE_WIN_FILE
	NWindows::NFile::NIO::CInFile File;
  #ifdef SUPPORT_DEVICE_FILE
	uint64 VirtPos;
	uint64 PhyPos;
	uint64 BufStartPos;
	Byte * Buf;
	uint32 BufSize;
  #endif
  #else
	NC::NFile::NIO::CInFile File;
  #endif
	bool SupportHardLinks;
	IInFileStream_Callback * Callback;
	My_UINT_PTR CallbackRef;

	virtual ~CInFileStream();
	CInFileStream();
	bool Open(CFSTR fileName) { return File.Open(fileName); }
	bool OpenShared(CFSTR fileName, bool shareForWrite) { return File.OpenShared(fileName, shareForWrite); }
	MY_QUERYINTERFACE_BEGIN2(IInStream)
	MY_QUERYINTERFACE_ENTRY(IStreamGetSize)
  #ifdef USE_WIN_FILE
	MY_QUERYINTERFACE_ENTRY(IStreamGetProps)
	MY_QUERYINTERFACE_ENTRY(IStreamGetProps2)
  #endif
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);

	STDMETHOD(GetSize) (uint64 *size);
  #ifdef USE_WIN_FILE
	STDMETHOD(GetProps) (uint64 *size, FILETIME *cTime, FILETIME *aTime, FILETIME *mTime, uint32 *attrib);
	STDMETHOD(GetProps2) (CStreamFileProps *props);
  #endif
};

class CStdInFileStream : public ISequentialInStream, public CMyUnknownImp {
public:
	MY_UNKNOWN_IMP
	virtual ~CStdInFileStream() 
	{
	}
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
};

class COutFileStream : public IOutStream, public CMyUnknownImp {
public:
  #ifdef USE_WIN_FILE
	NWindows::NFile::NIO::COutFile File;
  #else
	NC::NFile::NIO::COutFile File;
  #endif
	virtual ~COutFileStream();
	bool Create(CFSTR fileName, bool createAlways);
	bool Open(CFSTR fileName, DWORD creationDisposition);

	HRESULT Close();
	uint64 ProcessedSize;
  #ifdef USE_WIN_FILE
	bool SetTime(const FILETIME * cTime, const FILETIME * aTime, const FILETIME * mTime) { return File.SetTime(cTime, aTime, mTime); }
	bool SetMTime(const FILETIME * mTime) { return File.SetMTime(mTime); }
  #endif
	MY_UNKNOWN_IMP1(IOutStream)
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	STDMETHOD(SetSize) (uint64 newSize);
	HRESULT GetSize(uint64 * size);
};

class CStdOutFileStream : public ISequentialOutStream, public CMyUnknownImp {
	uint64 _size;
public:
	MY_UNKNOWN_IMP
	uint64 GetSize() const { return _size; }
	CStdOutFileStream() : _size(0) 
	{
	}
	virtual ~CStdOutFileStream() 
	{
	}
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
};
//
//#include <LimitedStreams.h>
class CLimitedSequentialInStream : public ISequentialInStream, public CMyUnknownImp {
	CMyComPtr <ISequentialInStream> _stream;
	uint64 _size;
	uint64 _pos;
	bool   _wasFinished;
public:
	void SetStream(ISequentialInStream * stream);
	void ReleaseStream();
	void Init(uint64 streamSize);
	MY_UNKNOWN_IMP1(ISequentialInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	uint64 GetSize() const { return _pos; }
	uint64 GetRem() const { return _size - _pos; }
	bool WasFinished() const { return _wasFinished; }
};

class CLimitedInStream : public IInStream, public CMyUnknownImp {
public:
	void SetStream(IInStream * stream) { _stream = stream; }
	HRESULT InitAndSeek(uint64 startOffset, uint64 size);
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	HRESULT SeekToStart() { return Seek(0, STREAM_SEEK_SET, NULL); }
private:
	CMyComPtr<IInStream> _stream;
	uint64 _virtPos;
	uint64 _physPos;
	uint64 _size;
	uint64 _startOffset;
	HRESULT SeekToPhys();
};

HRESULT CreateLimitedInStream(IInStream * inStream, uint64 pos, uint64 size, ISequentialInStream ** resStream);

class CClusterInStream : public IInStream, public CMyUnknownImp {
	uint64 _virtPos;
	uint64 _physPos;
	uint32 _curRem;
public:
	uint   BlockSizeLog;
	uint64 Size;
	CMyComPtr<IInStream> Stream;
	CRecordVector<uint32> Vector;
	uint64 StartOffset;

	HRESULT SeekToPhys();
	HRESULT InitAndSeek();
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
};

class CLimitedSequentialOutStream : public ISequentialOutStream, public CMyUnknownImp {
	CMyComPtr<ISequentialOutStream> _stream;
	uint64 _size;
	bool _overflow;
	bool _overflowIsAllowed;
public:
	MY_UNKNOWN_IMP1(ISequentialOutStream)
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	void SetStream(ISequentialOutStream * stream);
	void ReleaseStream();
	void Init(uint64 size, bool overflowIsAllowed = false);
	bool IsFinishedOK() const;
	uint64 GetRem() const { return _size; }
};

class CLimitedCachedInStream : public IInStream, public CMyUnknownImp {
public:
	CByteBuffer Buffer;
	void SetStream(IInStream * stream) { _stream = stream; }
	void SetCache(size_t cacheSize, size_t cachePos);
	HRESULT InitAndSeek(uint64 startOffset, uint64 size);
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	HRESULT SeekToStart() { return Seek(0, STREAM_SEEK_SET, NULL); }
private:
	CMyComPtr<IInStream> _stream;
	uint64 _virtPos;
	uint64 _physPos;
	uint64 _size;
	uint64 _startOffset;
	const Byte * _cache;
	size_t _cacheSize;
	size_t _cachePhyPos;

	HRESULT SeekToPhys() { return _stream->Seek(_physPos, STREAM_SEEK_SET, NULL); }
};

class CTailOutStream : public IOutStream, public CMyUnknownImp {
public:
	CMyComPtr<IOutStream> Stream;
	uint64 Offset;
	virtual ~CTailOutStream() 
	{
	}
	MY_UNKNOWN_IMP2(ISequentialOutStream, IOutStream)
	void Init()
	{
		_virtPos = 0;
		_virtSize = 0;
	}
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	STDMETHOD(SetSize) (uint64 newSize);
private:
	uint64 _virtPos;
	uint64 _virtSize;
};
//
//#include <MultiStream.h>
class CMultiStream : public IInStream, public CMyUnknownImp {
public:
	struct CSubStreamInfo {
		CSubStreamInfo();
		CMyComPtr <IInStream> Stream;
		uint64 Size;
		uint64 GlobalOffset;
		uint64 LocalPos;
	};
	CObjectVector<CSubStreamInfo> Streams;
	HRESULT Init();
	MY_UNKNOWN_IMP1(IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
private:
	uint64 _pos;
	uint64 _totalLength;
	uint   _streamIndex;
};

/* class COutMultiStream: public IOutStream, public CMyUnknownImp {
	unsigned _streamIndex; // required stream
	uint64 _offsetPos; // offset from start of _streamIndex index
	uint64 _absPos;
	uint64 _length;
	struct CSubStreamInfo {
		CMyComPtr<ISequentialOutStream> Stream;
		uint64 Size;
		uint64 Pos;
	};
	CObjectVector<CSubStreamInfo> Streams;
public:
	CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
	void Init()
	{
		_streamIndex = 0;
		_offsetPos = 0;
		_absPos = 0;
		_length = 0;
	}
	MY_UNKNOWN_IMP1(IOutStream)
	STDMETHOD(Write)(const void *data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek)(int64 offset, uint32 seekOrigin, uint64 *newPosition);
};*/
//
//#include <DummyOutStream.h>
class CDummyOutStream : public ISequentialOutStream, public CMyUnknownImp {
public:
	void SetStream(ISequentialOutStream * outStream) { _stream = outStream; }
	void ReleaseStream() { _stream.Release(); }
	void Init() { _size = 0; }

	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	uint64 GetSize() const { return _size; }
private:
	CMyComPtr<ISequentialOutStream> _stream;
	uint64 _size;
};
//
//#include <StreamObjects.h>
class CBufferInStream : public IInStream, public CMyUnknownImp {
public:
	CByteBuffer Buf;
	void Init() 
	{
		_pos = 0;
	}
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
private:
	uint64 _pos;
};

struct CReferenceBuf : public IUnknown, public CMyUnknownImp {
	CByteBuffer Buf;
	MY_UNKNOWN_IMP
};

class CBufInStream : public IInStream, public CMyUnknownImp {
public:
	void Init(const Byte * data, size_t size, IUnknown * ref = 0);
	void FASTCALL Init(CReferenceBuf * ref);
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
private:
	const Byte * _data;
	uint64 _pos;
	size_t _size;
	CMyComPtr<IUnknown> _ref;
};

void Create_BufInStream_WithReference(const void * data, size_t size, IUnknown * ref, ISequentialInStream ** stream);
void Create_BufInStream_WithNewBuffer(const void * data, size_t size, ISequentialInStream ** stream);
inline void Create_BufInStream_WithNewBuffer(const CByteBuffer &buf, ISequentialInStream ** stream)
{
	Create_BufInStream_WithNewBuffer(buf, buf.Size(), stream);
}

class CBufPtrSeqOutStream : public ISequentialOutStream, public CMyUnknownImp {
public:
	void Init(Byte * buffer, size_t size);
	size_t GetPos() const { return _pos; }
	MY_UNKNOWN_IMP1(ISequentialOutStream)
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
private:
	Byte * _buffer;
	size_t _size;
	size_t _pos;
};

class CCachedInStream : public IInStream, public CMyUnknownImp {
public:
	CCachedInStream();
	virtual ~CCachedInStream(); // the destructor must be virtual (release calls it) !!!
	void Free() throw();
	bool Alloc(unsigned blockSizeLog, unsigned numBlocksLog) throw();
	void Init(uint64 size) throw();

	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
protected:
	virtual HRESULT ReadBlock(uint64 blockIndex, Byte * dest, size_t blockSize) = 0;
private:
	uint64 * _tags;
	Byte * _data;
	size_t _dataSize;
	uint   _blockSizeLog;
	uint   _numBlocksLog;
	uint64 _size;
	uint64 _pos;
};
//
//#include <StreamUtils.h>
HRESULT ReadStream(ISequentialInStream *stream, void *data, size_t *size) throw();
HRESULT ReadStream_FALSE(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT ReadStream_FAIL(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT WriteStream(ISequentialOutStream *stream, const void *data, size_t size) throw();
//
//#include <ICoder.h>
#define CODER_INTERFACE(i, x) DECL_INTERFACE(i, 4, x)

CODER_INTERFACE(ICompressProgressInfo, 0x04)
{
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize) PURE;

	/* (inSize) can be NULL, if unknown
	   (outSize) can be NULL, if unknown

	   returns:
	   S_OK
	   E_ABORT  : Break by user
	   another error codes
	 */
};

CODER_INTERFACE(ICompressCoder, 0x05)
{
	STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, 
		const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress) PURE;
};

CODER_INTERFACE(ICompressCoder2, 0x18)
{
	STDMETHOD(Code) (ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
	    ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams,
	    ICompressProgressInfo *progress) PURE;
};
/*
   ICompressCoder::Code
   ICompressCoder2::Code

   returns:
    S_OK     : OK
    S_FALSE  : data error (for decoders)
    E_OUTOFMEMORY : memory allocation error
    another error code : some error. For example, it can be error code received from inStream or outStream function.

   Parameters:
    (inStream != NULL)
    (outStream != NULL)

    if(inSize != NULL)
    {
      Encoders in 7-Zip ignore (inSize).
      Decoder can use (*inSize) to check that stream was decoded correctly.
      Some decoder in 7-Zip check it, if(full_decoding mode was set via ICompressSetFinishMode)
    }

    If it's required to limit the reading from input stream (inStream), it can
      be done with ISequentialInStream implementation.

    if(outSize != NULL)
    {
      Encoders in 7-Zip ignore (outSize).
      Decoder unpacks no more than (*outSize) bytes.
    }

    (progress == NULL) is allowed.


   Decoding with Code() function
   -----------------------------

   You can request some interfaces before decoding
   - ICompressSetDecoderProperties2
   - ICompressSetFinishMode

   If you need to decode full stream:
   {
    1) try to set full_decoding mode with ICompressSetFinishMode::SetFinishMode(1);
    2) call the Code() function with specified (inSize) and (outSize), if these sizes are known.
   }

   If you need to decode only part of stream:
   {
    1) try to set partial_decoding mode with ICompressSetFinishMode::SetFinishMode(0);
    2) Call the Code() function with specified (inSize = NULL) and specified (outSize).
   }

   Encoding with Code() function
   -----------------------------

   You can request some interfaces :
   - ICompressSetCoderProperties   - use it before encoding to set properties
   - ICompressWriteCoderProperties - use it before or after encoding to request encoded properties.

   ICompressCoder2 is used when (numInStreams != 1 || numOutStreams != 1)
     The rules are similar to ICompressCoder rules
 */
CODER_INTERFACE(ICompressSetCoderProperties, 0x20)
{
	STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps) PURE;
};

/*CODER_INTERFACE(ICompressSetCoderProperties, 0x21)
   {
   STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream) PURE;
   };*/

CODER_INTERFACE(ICompressSetDecoderProperties2, 0x22)
{
	/* returns:
	   S_OK
	   E_NOTIMP      : unsupported properties
	   E_INVALIDARG  : incorrect (or unsupported) properties
	   E_OUTOFMEMORY : memory allocation error
	 */
	STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size) PURE;
};

CODER_INTERFACE(ICompressWriteCoderProperties, 0x23) { STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream) PURE; };
CODER_INTERFACE(ICompressGetInStreamProcessedSize, 0x24) { STDMETHOD(GetInStreamProcessedSize) (uint64 *value) PURE; };
CODER_INTERFACE(ICompressSetCoderMt, 0x25) { STDMETHOD(SetNumberOfThreads) (uint32 numThreads) PURE; };

CODER_INTERFACE(ICompressSetFinishMode, 0x26)
{
	STDMETHOD(SetFinishMode) (uint32 finishMode) PURE;
	/* finishMode:
	   0 : partial decoding is allowed. It's default mode for ICompressCoder::Code(), if(outSize) is defined.
	   1 : full decoding. The stream must be finished at the end of decoding. */
};

CODER_INTERFACE(ICompressGetInStreamProcessedSize2, 0x27) { STDMETHOD(GetInStreamProcessedSize2) (uint32 streamIndex, uint64 *value) PURE; };

CODER_INTERFACE(ICompressGetSubStreamSize, 0x30)
{
	STDMETHOD(GetSubStreamSize) (uint64 subStream, uint64 *value) PURE;
	/* returns:
	   S_OK     : (*value) contains the size or estimated size (can be incorrect size)
	   S_FALSE  : size is undefined
	   E_NOTIMP : the feature is not implemented

	   Let's (read_size) is size of data that was already read by ISequentialInStream::Read().
	   The caller should call GetSubStreamSize() after each Read() and check sizes:
	   if(start_of_subStream + *value < read_size)
	   {
	    // (*value) is correct, and it's allowed to call GetSubStreamSize() for next subStream:
	    start_of_subStream += *value;
	    subStream++;
	   }
	 */
};

CODER_INTERFACE(ICompressSetInStream, 0x31)
{
	STDMETHOD(SetInStream) (ISequentialInStream *inStream) PURE;
	STDMETHOD(ReleaseInStream) () PURE;
};

CODER_INTERFACE(ICompressSetOutStream, 0x32)
{
	STDMETHOD(SetOutStream) (ISequentialOutStream *outStream) PURE;
	STDMETHOD(ReleaseOutStream) () PURE;
};
/*
   CODER_INTERFACE(ICompressSetInStreamSize, 0x33)
   {
   STDMETHOD(SetInStreamSize)(const uint64 *inSize) PURE;
   };
 */
CODER_INTERFACE(ICompressSetOutStreamSize, 0x34)
{
	STDMETHOD(SetOutStreamSize) (const uint64 *outSize) PURE;
	/* That function initializes decoder structures.
	   Call this function only for stream version of decoder.
	     if(outSize == NULL), then output size is unknown
	     if(outSize != NULL), then the decoder must stop decoding after (*outSize) bytes. */
};
CODER_INTERFACE(ICompressSetBufSize, 0x35)
{
	STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size) PURE;
	STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size) PURE;
};
CODER_INTERFACE(ICompressInitEncoder, 0x36)
{
	STDMETHOD(InitEncoder) () PURE;
	/* That function initializes encoder structures.
	   Call this function only for stream version of encoder. */
};
CODER_INTERFACE(ICompressSetInStream2, 0x37)
{
	STDMETHOD(SetInStream2) (uint32 streamIndex, ISequentialInStream *inStream) PURE;
	STDMETHOD(ReleaseInStream2) (uint32 streamIndex) PURE;
};
/*
   CODER_INTERFACE(ICompressSetOutStream2, 0x38)
   {
   STDMETHOD(SetOutStream2)(uint32 streamIndex, ISequentialOutStream *outStream) PURE;
   STDMETHOD(ReleaseOutStream2)(uint32 streamIndex) PURE;
   };

   CODER_INTERFACE(ICompressSetInStreamSize2, 0x39)
   {
   STDMETHOD(SetInStreamSize2)(uint32 streamIndex, const uint64 *inSize) PURE;
   };
 */

/*
   ICompressFilter
   Filter() converts as most as possible bytes
     returns: (outSize):
       if(outSize <= size) : Filter have converted outSize bytes
       if(outSize >  size) : Filter have not converted anything.
           and it needs at least outSize bytes to convert one block
           (it's for crypto block algorithms).
 */
#define INTERFACE_ICompressFilter(x) STDMETHOD(Init) ()x; STDMETHOD_(uint32, Filter) (Byte *data, uint32 size)x;

CODER_INTERFACE(ICompressFilter, 0x40) { INTERFACE_ICompressFilter(PURE); };

CODER_INTERFACE(ICompressCodecsInfo, 0x60)
{
	STDMETHOD(GetNumMethods) (uint32 *numMethods) PURE;
	STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value) PURE;
	STDMETHOD(CreateDecoder) (uint32 index, const GUID *iid, void ** coder) PURE;
	STDMETHOD(CreateEncoder) (uint32 index, const GUID *iid, void ** coder) PURE;
};

CODER_INTERFACE(ISetCompressCodecsInfo, 0x61) { STDMETHOD(SetCompressCodecsInfo) (ICompressCodecsInfo *compressCodecsInfo) PURE; };

CODER_INTERFACE(ICryptoProperties, 0x80)
{
	STDMETHOD(SetKey) (const Byte *data, uint32 size) PURE;
	STDMETHOD(SetInitVector) (const Byte *data, uint32 size) PURE;
};

/*
   CODER_INTERFACE(ICryptoResetSalt, 0x88)
   {
   STDMETHOD(ResetSalt)() PURE;
   };
 */

CODER_INTERFACE(ICryptoResetInitVector, 0x8C)
{
	STDMETHOD(ResetInitVector) () PURE;
	/* Call ResetInitVector() only for encoding.
	   Call ResetInitVector() before encoding and before WriteCoderProperties().
	   Crypto encoder can create random IV in that function. */
};

CODER_INTERFACE(ICryptoSetPassword, 0x90) { STDMETHOD(CryptoSetPassword) (const Byte *data, uint32 size) PURE; };
CODER_INTERFACE(ICryptoSetCRC, 0xA0) { STDMETHOD(CryptoSetCRC) (uint32 crc) PURE; };

#define INTERFACE_IHasher(x) \
	STDMETHOD_(void, Init) () throw()x; \
	STDMETHOD_(void, Update) (const void * data, uint32 size) throw()x; \
	STDMETHOD_(void, Final) (Byte *digest) throw()x; \
	STDMETHOD_(uint32, GetDigestSize) () throw()x; \

CODER_INTERFACE(IHasher, 0xC0) { INTERFACE_IHasher(PURE) };

CODER_INTERFACE(IHashers, 0xC1)
{
	STDMETHOD_(uint32, GetNumHashers) () PURE;
	STDMETHOD(GetHasherProp) (uint32 index, PROPID propID, PROPVARIANT *value) PURE;
	STDMETHOD(CreateHasher) (uint32 index, IHasher **hasher) PURE;
};

extern "C" {
	typedef HRESULT (WINAPI *Func_GetNumberOfMethods)(uint32 * numMethods);
	typedef HRESULT (WINAPI *Func_GetMethodProperty)(uint32 index, PROPID propID, PROPVARIANT * value);
	typedef HRESULT (WINAPI *Func_CreateDecoder)(uint32 index, const GUID * iid, void ** outObject);
	typedef HRESULT (WINAPI *Func_CreateEncoder)(uint32 index, const GUID * iid, void ** outObject);
	typedef HRESULT (WINAPI *Func_GetHashers)(IHashers ** hashers);
	typedef HRESULT (WINAPI *Func_SetCodecs)(ICompressCodecsInfo * compressCodecsInfo);
}
//
//#include <IPassword.h>
#define PASSWORD_INTERFACE(i, x) DECL_INTERFACE(i, 5, x)
PASSWORD_INTERFACE(ICryptoGetTextPassword, 0x10) { STDMETHOD(CryptoGetTextPassword)(BSTR *password) PURE; };
PASSWORD_INTERFACE(ICryptoGetTextPassword2, 0x11) { STDMETHOD(CryptoGetTextPassword2)(int32 *passwordIsDefined, BSTR *password) PURE; };
//
//#include <IProgress.h>
#define INTERFACE_IProgress(x) STDMETHOD(SetTotal)(uint64 total) x; STDMETHOD(SetCompleted)(const uint64 *completeValue) x;
DECL_INTERFACE(IProgress, 0, 5) { INTERFACE_IProgress(PURE) };
//
//#include <IArchive.h>
#define ARCHIVE_INTERFACE_SUB(i, base, x) DECL_INTERFACE_SUB(i, base, 6, x)
#define ARCHIVE_INTERFACE(i, x) ARCHIVE_INTERFACE_SUB(i, IUnknown, x)

namespace NArcInfoFlags {
	const uint32 kKeepName        = 1 << 0;    // keep name of file in archive name
	const uint32 kAltStreams      = 1 << 1;    // the handler supports alt streams
	const uint32 kNtSecure        = 1 << 2;    // the handler supports NT security
	const uint32 kFindSignature   = 1 << 3;    // the handler can find start of archive
	const uint32 kMultiSignature  = 1 << 4;    // there are several signatures
	const uint32 kUseGlobalOffset = 1 << 5;    // the seek position of stream must be set as global offset
	const uint32 kStartOpen       = 1 << 6;    // call handler for each start position
	const uint32 kPureStartOpen   = 1 << 7;    // call handler only for start of file
	const uint32 kBackwardOpen    = 1 << 8;    // archive can be open backward
	const uint32 kPreArc          = 1 << 9;    // such archive can be stored before real archive (like SFX stub)
	const uint32 kSymLinks        = 1 << 10;   // the handler supports symbolic links
	const uint32 kHardLinks       = 1 << 11;   // the handler supports hard links
}
namespace NArchive {
	namespace NExtractArc { // @sobolev NExtract-->NExtractArc
		namespace NAskMode {
			enum {
				kExtract = 0,
				kTest,
				kSkip
			};
		}
		namespace NOperationResult {
			enum {
				kOK = 0,
				kUnsupportedMethod,
				kDataError,
				kCRCError,
				kUnavailable,
				kUnexpectedEnd,
				kDataAfterEnd,
				kIsNotArc,
				kHeadersError,
				kWrongPassword
			};
		}
	}
}

#define INTERFACE_IArchiveOpenCallback(x) \
	STDMETHOD(SetTotal) (const uint64 *files, const uint64 *bytes)x; \
	STDMETHOD(SetCompleted) (const uint64 *files, const uint64 *bytes)x; \

ARCHIVE_INTERFACE(IArchiveOpenCallback, 0x10) { INTERFACE_IArchiveOpenCallback(PURE); };
/*
   IArchiveExtractCallback::

   7-Zip doesn't call IArchiveExtractCallback functions
   GetStream()
   PrepareOperation()
   SetOperationResult()
   from different threads simultaneously.
   But 7-Zip can call functions for IProgress or ICompressProgressInfo functions
   from another threads simultaneously with calls for IArchiveExtractCallback interface.

   IArchiveExtractCallback::GetStream()
   uint32 index - index of item in Archive
   int32 askExtractMode  (Extract::NAskMode)
    if(askMode != NExtractArc::NAskMode::kExtract)
    {
      then the callee can not real stream: (*inStream == NULL)
    }

   Out:
      (*inStream == NULL) - for directories
      (*inStream == NULL) - if link (hard link or symbolic link) was created
      if(*inStream == NULL && askMode == NExtractArc::NAskMode::kExtract)
      {
        then the caller must skip extracting of that file.
      }

   returns:
    S_OK     : OK
    S_FALSE  : data error (for decoders)

   if(IProgress::SetTotal() was called)
   {
   IProgress::SetCompleted(completeValue) uses
    packSize   - for some stream formats (xz, gz, bz2, lzma, z, ppmd).
    unpackSize - for another formats.
   }
   else
   {
   IProgress::SetCompleted(completeValue) uses packSize.
   }

   SetOperationResult()
   7-Zip calls SetOperationResult at the end of extracting,
   so the callee can close the file, set attributes, timestamps and security information.

   int32 opRes (NExtractArc::NOperationResult)
 */

#define INTERFACE_IArchiveExtractCallback(x) \
	INTERFACE_IProgress(x) \
	STDMETHOD(GetStream) (uint32 index, ISequentialOutStream **outStream, int32 askExtractMode)x; \
	STDMETHOD(PrepareOperation) (int32 askExtractMode)x; \
	STDMETHOD(SetOperationResult) (int32 opRes)x; \

ARCHIVE_INTERFACE_SUB(IArchiveExtractCallback, IProgress, 0x20) { INTERFACE_IArchiveExtractCallback(PURE) };
/*
   IArchiveExtractCallbackMessage can be requested from IArchiveExtractCallback object
   by Extract() or UpdateItems() functions to report about extracting errors
   ReportExtractResult()
   uint32 indexType (NEventIndexType)
   uint32 index
   int32 opRes (NExtractArc::NOperationResult)
 */

#define INTERFACE_IArchiveExtractCallbackMessage(x) STDMETHOD(ReportExtractResult) (uint32 indexType, uint32 index, int32 opRes)x; 

ARCHIVE_INTERFACE_SUB(IArchiveExtractCallbackMessage, IProgress, 0x21) { INTERFACE_IArchiveExtractCallbackMessage(PURE) };

#define INTERFACE_IArchiveOpenVolumeCallback(x)	\
	STDMETHOD(GetProperty) (PROPID propID, PROPVARIANT *value)x; \
	STDMETHOD(GetStream) (const wchar_t * name, IInStream **inStream)x; \

ARCHIVE_INTERFACE(IArchiveOpenVolumeCallback, 0x30) { INTERFACE_IArchiveOpenVolumeCallback(PURE); };
ARCHIVE_INTERFACE(IInArchiveGetStream, 0x40) { STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream) PURE; };
ARCHIVE_INTERFACE(IArchiveOpenSetSubArchiveName, 0x50) { STDMETHOD(SetSubArchiveName) (const wchar_t * name) PURE; };
/*
   IInArchive::Open
    stream
      if(kUseGlobalOffset), stream current position can be non 0.
      if(!kUseGlobalOffset), stream current position is 0.
    if(maxCheckStartPosition == NULL), the handler can try to search archive start in stream
    if(*maxCheckStartPosition == 0), the handler must check only current position as archive start

   IInArchive::Extract:
   indices must be sorted
   numItems = (uint32)(int32)-1 = 0xFFFFFFFF means "all files"
   testMode != 0 means "test files without writing to outStream"

   IInArchive::GetArchiveProperty:
   kpidOffset  - start offset of archive.
      VT_EMPTY : means offset = 0.
      VT_UI4, VT_UI8, VT_I8 : result offset; negative values is allowed
   kpidPhySize - size of archive. VT_EMPTY means unknown size.
    kpidPhySize is allowed to be larger than file size. In that case it must show
    supposed size.

   kpidIsDeleted:
   kpidIsAltStream:
   kpidIsAux:
   kpidINode:
    must return VARIANT_TRUE (VT_BOOL), if archive can support that property in GetProperty.


   Notes:
   Don't call IInArchive functions for same IInArchive object from different threads simultaneously.
   Some IInArchive handlers will work incorrectly in that case.
 */
#ifdef _MSC_VER
	#define MY_NO_THROW_DECL_ONLY throw()
#else
	#define MY_NO_THROW_DECL_ONLY
#endif
#define INTERFACE_IInArchive(x)	\
	STDMETHOD(Open) (IInStream *stream, const uint64 *maxCheckStartPosition, IArchiveOpenCallback *	\
	    openCallback) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(Close) () MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetNumberOfItems) (uint32 *numItems) MY_NO_THROW_DECL_ONLY x;	\
	STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(Extract) (const uint32* indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * \
	    extractCallback) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetArchiveProperty) (PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetNumberOfProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetPropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetNumberOfArchiveProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
	STDMETHOD(GetArchivePropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x;	\

ARCHIVE_INTERFACE(IInArchive, 0x60) { INTERFACE_IInArchive(PURE) };

namespace NPropDataType {
	const uint32 kMask_ZeroEnd   = 1 << 4;
	// const uint32 kMask_BigEndian = 1 << 5;
	const uint32 kMask_Utf       = 1 << 6;
	const uint32 kMask_Utf8  = kMask_Utf | 0;
	const uint32 kMask_Utf16 = kMask_Utf | 1;
	// const uint32 kMask_Utf32 = kMask_Utf | 2;
	const uint32 kNotDefined = 0;
	const uint32 kRaw = 1;
	const uint32 kUtf8z  = kMask_Utf8  | kMask_ZeroEnd;
	const uint32 kUtf16z = kMask_Utf16 | kMask_ZeroEnd;
};

// UTF string (pointer to wchar_t) with zero end and little-endian.
#define PROP_DATA_TYPE_wchar_t_PTR_Z_LE ((NPropDataType::kMask_Utf | NPropDataType::kMask_ZeroEnd) + (sizeof(wchar_t) >> 1))
/*
   GetRawProp:
   Result:
    S_OK - even if property is not set
 */
#define INTERFACE_IArchiveGetRawProps(x) \
	STDMETHOD(GetParent) (uint32 index, uint32 *parent, uint32 *parentType)x; \
	STDMETHOD(GetRawProp) (uint32 index, PROPID propID, const void ** data, uint32 *dataSize, uint32 *propType)x; \
	STDMETHOD(GetNumRawProps) (uint32 *numProps)x; \
	STDMETHOD(GetRawPropInfo) (uint32 index, BSTR *name, PROPID *propID)x;

ARCHIVE_INTERFACE(IArchiveGetRawProps, 0x70) { INTERFACE_IArchiveGetRawProps(PURE) };

#define INTERFACE_IArchiveGetRootProps(x) \
	STDMETHOD(GetRootProp) (PROPID propID, PROPVARIANT *value)x; \
	STDMETHOD(GetRootRawProp) (PROPID propID, const void ** data, uint32 *dataSize, uint32 *propType)x; \

ARCHIVE_INTERFACE(IArchiveGetRootProps, 0x71) { INTERFACE_IArchiveGetRootProps(PURE) };
ARCHIVE_INTERFACE(IArchiveOpenSeq, 0x61) { STDMETHOD(OpenSeq) (ISequentialInStream *stream) PURE; };
/*
   OpenForSize
   Result:
    S_FALSE - is not archive
    ? - DATA error
 */

/*
   const uint32 kOpenFlags_RealPhySize = 1 << 0;
   const uint32 kOpenFlags_NoSeek = 1 << 1;
   // const uint32 kOpenFlags_BeforeExtract = 1 << 2;
 */

/*
   Flags:
   0 - opens archive with IInStream, if IInStream interface is supported
     - if phySize is not available, it doesn't try to make full parse to get phySize
   kOpenFlags_NoSeek -  ArcOpen2 function doesn't use IInStream interface, even if it's available
   kOpenFlags_RealPhySize - the handler will try to get PhySize, even if it requires full decompression for file

   if handler is not allowed to use IInStream and the flag kOpenFlags_RealPhySize is not specified,
   the handler can return S_OK, but it doesn't check even Signature.
   So next Extract can be called for that sequential stream.
 */

/*
   ARCHIVE_INTERFACE(IArchiveOpen2, 0x62)
   {
   STDMETHOD(ArcOpen2)(ISequentialInStream *stream, uint32 flags, IArchiveOpenCallback *openCallback) PURE;
   };
 */

// ---------- UPDATE ----------

/*
   GetUpdateItemInfo outs:
   *newData  *newProps
   0        0      - Copy data and properties from archive
   0        1      - Copy data from archive, request new properties
   1        0      - that combination is unused now
   1        1      - Request new data and new properties. It can be used even for folders

   indexInArchive = -1 if there is no item in archive, or if it doesn't matter.


   GetStream out:
   Result:
    S_OK:
      (*inStream == NULL) - only for directories
                          - the bug was fixed in 9.33: (*Stream == NULL) was in case of anti-file
      (*inStream != NULL) - for any file, even for empty file or anti-file
    S_FALSE - skip that file (don't add item to archive) - (client code can't open stream of that file by some reason)
      (*inStream == NULL)

   The order of calling for hard links:
   - GetStream()
   - GetProperty(kpidHardLink)

   SetOperationResult()
   int32 opRes (NExtractArc::NOperationResult::kOK)
 */

#define INTERFACE_IArchiveUpdateCallback(x) \
	INTERFACE_IProgress(x);	\
	STDMETHOD(GetUpdateItemInfo) (uint32 index, int32 *newData, int32 *newProps, uint32 *indexInArchive)x; \
	STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value)x; \
	STDMETHOD(GetStream) (uint32 index, ISequentialInStream **inStream)x; \
	STDMETHOD(SetOperationResult) (int32 operationResult)x;	\

ARCHIVE_INTERFACE_SUB(IArchiveUpdateCallback, IProgress, 0x80)
{
	INTERFACE_IArchiveUpdateCallback(PURE);
};

#define INTERFACE_IArchiveUpdateCallback2(x) \
	INTERFACE_IArchiveUpdateCallback(x) \
	STDMETHOD(GetVolumeSize) (uint32 index, uint64 *size)x;	\
	STDMETHOD(GetVolumeStream) (uint32 index, ISequentialOutStream **volumeStream)x; \

ARCHIVE_INTERFACE_SUB(IArchiveUpdateCallback2, IArchiveUpdateCallback, 0x82)
{
	INTERFACE_IArchiveUpdateCallback2(PURE);
};
/*
   IArchiveUpdateCallbackFile::ReportOperation
   uint32 indexType (NEventIndexType)
   uint32 index
   uint32 notifyOp (NUpdateNotifyOp)
 */

#define INTERFACE_IArchiveUpdateCallbackFile(x)	\
	STDMETHOD(GetStream2) (uint32 index, ISequentialInStream **inStream, uint32 notifyOp)x;	\
	STDMETHOD(ReportOperation) (uint32 indexType, uint32 index, uint32 notifyOp)x; \

ARCHIVE_INTERFACE(IArchiveUpdateCallbackFile, 0x83) { INTERFACE_IArchiveUpdateCallbackFile(PURE); };

/*
   UpdateItems()
   -------------

   outStream: output stream. (the handler) MUST support the case when
    Seek position in outStream is not ZERO.
    but the caller calls with empty outStream and seek position is ZERO??

   archives with stub:

   If archive is open and the handler and (Offset > 0), then the handler
   knows about stub size.
   UpdateItems():
   1) the handler MUST copy that stub to outStream
   2) the caller MUST NOT copy the stub to outStream, if
     "rsfx" property is set with SetProperties

   the handler must support the case where
    ISequentialOutStream *outStream
 */

#define INTERFACE_IOutArchive(x) \
	STDMETHOD(UpdateItems) (ISequentialOutStream *outStream, uint32 numItems, IArchiveUpdateCallback *updateCallback)x; \
	STDMETHOD(GetFileTimeType) (uint32 *type)x;

ARCHIVE_INTERFACE(IOutArchive, 0xA0) { INTERFACE_IOutArchive(PURE) };
ARCHIVE_INTERFACE(ISetProperties, 0x03) { STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps) PURE; };
ARCHIVE_INTERFACE(IArchiveKeepModeForNextOpen, 0x04) { STDMETHOD(KeepModeForNextOpen) () PURE; };

/* Exe handler: the handler for executable format (PE, ELF, Mach-O).
   SFX archive: executable stub + some tail data.
     before 9.31: exe handler didn't parse SFX archives as executable format.
     for 9.31+: exe handler parses SFX archives as executable format, only if AllowTail(1) was called */

ARCHIVE_INTERFACE(IArchiveAllowTail, 0x05) { STDMETHOD(AllowTail) (int32 allowTail) PURE; };

#define IMP_IInArchive_GetProp(k) \
	(uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) \
	{ if(index >= SIZEOFARRAY(k)) return E_INVALIDARG; \
	  *propID = k[index]; *varType = k7z_PROPID_To_VARTYPE[(uint)*propID];  *name = 0; return S_OK; } \


struct CStatProp {
	const char * Name;
	uint32 PropID;
	VARTYPE vt;
};

#define IMP_IInArchive_GetProp_WITH_NAME(k) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) \
	{ if(index >= SIZEOFARRAY(k)) return E_INVALIDARG; \
	  const CStatProp &prop = k[index]; \
	  *propID = (PROPID)prop.PropID; *varType = prop.vt; \
	  *name = NWindows::NCOM::AllocBstrFromAscii(prop.Name); return S_OK; }	\

#define IMP_IInArchive_Props \
	STDMETHODIMP CHandler::GetNumberOfProperties(uint32 *numProps) { *numProps = SIZEOFARRAY(kProps); return S_OK; } \
	STDMETHODIMP CHandler::GetPropertyInfo IMP_IInArchive_GetProp(kProps)

#define IMP_IInArchive_Props_WITH_NAME \
	STDMETHODIMP CHandler::GetNumberOfProperties(uint32 *numProps) { *numProps = SIZEOFARRAY(kProps); return S_OK; } \
	STDMETHODIMP CHandler::GetPropertyInfo IMP_IInArchive_GetProp_WITH_NAME(kProps)

#define IMP_IInArchive_ArcProps	\
	STDMETHODIMP CHandler::GetNumberOfArchiveProperties(uint32 *numProps) { *numProps = SIZEOFARRAY(kArcProps); return S_OK; } \
	STDMETHODIMP CHandler::GetArchivePropertyInfo IMP_IInArchive_GetProp(kArcProps)

#define IMP_IInArchive_ArcProps_WITH_NAME \
	STDMETHODIMP CHandler::GetNumberOfArchiveProperties(uint32 *numProps) { *numProps = SIZEOFARRAY(kArcProps); return S_OK; } \
	STDMETHODIMP CHandler::GetArchivePropertyInfo IMP_IInArchive_GetProp_WITH_NAME(kArcProps)

#define IMP_IInArchive_ArcProps_NO_Table \
	STDMETHODIMP CHandler::GetNumberOfArchiveProperties(uint32 *numProps) { *numProps = 0; return S_OK; }	\
	STDMETHODIMP CHandler::GetArchivePropertyInfo(uint32, BSTR *, PROPID *, VARTYPE *) { return E_NOTIMPL; } \

#define IMP_IInArchive_ArcProps_NO \
	IMP_IInArchive_ArcProps_NO_Table STDMETHODIMP CHandler::GetArchiveProperty(PROPID, PROPVARIANT *value) { value->vt = VT_EMPTY; return S_OK; }

#define k_IsArc_Res_NO   0
#define k_IsArc_Res_YES  1
#define k_IsArc_Res_NEED_MORE 2
// #define k_IsArc_Res_YES_LOW_PROB 3

#define API_FUNC_IsArc EXTERN_C uint32 WINAPI
#define API_FUNC_static_IsArc extern "C" { static uint32 WINAPI

extern "C" {
	typedef HRESULT (WINAPI *Func_CreateObject)(const GUID * clsID, const GUID * iid, void ** outObject);
	typedef uint32 (WINAPI *Func_IsArc)(const Byte * p, size_t size);
	typedef HRESULT (WINAPI *Func_GetIsArc)(uint32 formatIndex, Func_IsArc * isArc);
	typedef HRESULT (WINAPI *Func_GetNumberOfFormats)(uint32 * numFormats);
	typedef HRESULT (WINAPI *Func_GetHandlerProperty)(PROPID propID, PROPVARIANT * value);
	typedef HRESULT (WINAPI *Func_GetHandlerProperty2)(uint32 index, PROPID propID, PROPVARIANT * value);
	typedef HRESULT (WINAPI *Func_SetCaseSensitive)(int32 caseSensitive);
	typedef HRESULT (WINAPI *Func_SetLargePageMode)();
	typedef IOutArchive * (*Func_CreateOutArchive)();
	typedef IInArchive * (*Func_CreateInArchive)();
}
//
//#include <FilterCoder.h>
#define MY_QUERYINTERFACE_ENTRY_AG(i, sub0, sub) else if(iid == IID_ ## i) \
	{ if(!sub) RINOK(sub0->QueryInterface(IID_ ## i, (void **)&sub))	*outObject = (void *)(i*)this; }

struct CAlignedMidBuffer {
  #ifdef _WIN32
	Byte * _buf;
	CAlignedMidBuffer() : _buf(NULL) 
	{
	}
	~CAlignedMidBuffer() 
	{
		::MidFree(_buf);
	}
	void AllocAlignedMask(size_t size, size_t)
	{
		::MidFree(_buf);
		_buf = static_cast<Byte *>(::MidAlloc(size));
	}
  #else
	Byte * _bufBase;
	Byte * _buf;
	CAlignedMidBuffer() : _bufBase(NULL), _buf(NULL) 
	{
	}
	~CAlignedMidBuffer() 
	{
		::MidFree(_bufBase);
	}
	void AllocAlignedMask(size_t size, size_t alignMask)
	{
		::MidFree(_bufBase);
		_buf = NULL;
		_bufBase = static_cast<Byte *>(::MidAlloc(size + alignMask));
		if(_bufBase) {
			// _buf = (Byte *)(((uintptr_t)_bufBase + alignMask) & ~(uintptr_t)alignMask);
			_buf = (Byte *)(((ptrdiff_t)_bufBase + alignMask) & ~(ptrdiff_t)alignMask);
		}
	}
  #endif
};

class CFilterCoder : public ICompressCoder, public ICompressSetOutStreamSize, public ICompressInitEncoder,
	public ICompressSetInStream, public ISequentialInStream, public ICompressSetOutStream, public ISequentialOutStream,
	public IOutStreamFinish, public ICompressSetBufSize,
#ifndef _NO_CRYPTO
	public ICryptoSetPassword, public ICryptoProperties,
#endif
#ifndef EXTRACT_ONLY
	public ICompressSetCoderProperties, public ICompressWriteCoderProperties, /*public ICryptoResetSalt,*/ public ICryptoResetInitVector,
#endif
	public ICompressSetDecoderProperties2, public CMyUnknownImp, public CAlignedMidBuffer
{
	uint32 _bufSize;
	uint32 _inBufSize;
	uint32 _outBufSize;
	bool   _encodeMode;
	bool   _outSizeIsDefined;
	uint64 _outSize;
	uint64 _nowPos64;
	CMyComPtr <ISequentialInStream> _inStream;
	CMyComPtr <ISequentialOutStream> _outStream;
	uint32 _bufPos;
	uint32 _convPos; // current pos in buffer for converted data
	uint32 _convSize; // size of converted data starting from _convPos

	void InitSpecVars();
	HRESULT Alloc();
	HRESULT Init_and_Alloc();
	HRESULT Flush2();
#ifndef _NO_CRYPTO
	CMyComPtr<ICryptoSetPassword> _SetPassword;
	CMyComPtr<ICryptoProperties> _CryptoProperties;
#endif
#ifndef EXTRACT_ONLY
	CMyComPtr<ICompressSetCoderProperties> _SetCoderProperties;
	CMyComPtr<ICompressWriteCoderProperties> _WriteCoderProperties;
	// CMyComPtr<ICryptoResetSalt> _CryptoResetSalt;
	CMyComPtr<ICryptoResetInitVector> _CryptoResetInitVector;
#endif
	CMyComPtr<ICompressSetDecoderProperties2> _SetDecoderProperties2;
public:
	CMyComPtr<ICompressFilter> Filter;
	CFilterCoder(bool encodeMode);
	~CFilterCoder();
	class C_InStream_Releaser {
	public:
		CFilterCoder * FilterCoder;
		C_InStream_Releaser() : FilterCoder(NULL) 
		{
		}
		~C_InStream_Releaser() 
		{
			CALLPTRMEMB(FilterCoder, ReleaseInStream());
		}
	};
	class C_OutStream_Releaser {
	public:
		CFilterCoder * FilterCoder;
		C_OutStream_Releaser() : FilterCoder(NULL) 
		{
		}
		~C_OutStream_Releaser() 
		{
			CALLPTRMEMB(FilterCoder, ReleaseOutStream());
		}
	};
	class C_Filter_Releaser {
	public:
		CFilterCoder * FilterCoder;
		C_Filter_Releaser() : FilterCoder(NULL) 
		{
		}
		~C_Filter_Releaser() 
		{
			if(FilterCoder) 
				FilterCoder->Filter.Release();
		}
	};
	MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
	MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
	MY_QUERYINTERFACE_ENTRY(ICompressInitEncoder)
	MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
	MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
	MY_QUERYINTERFACE_ENTRY(ICompressSetOutStream)
	MY_QUERYINTERFACE_ENTRY(ISequentialOutStream)
	MY_QUERYINTERFACE_ENTRY(IOutStreamFinish)
	MY_QUERYINTERFACE_ENTRY(ICompressSetBufSize)
#ifndef _NO_CRYPTO
	MY_QUERYINTERFACE_ENTRY_AG(ICryptoSetPassword, Filter, _SetPassword)
	MY_QUERYINTERFACE_ENTRY_AG(ICryptoProperties, Filter, _CryptoProperties)
#endif
#ifndef EXTRACT_ONLY
	MY_QUERYINTERFACE_ENTRY_AG(ICompressSetCoderProperties, Filter, _SetCoderProperties)
	MY_QUERYINTERFACE_ENTRY_AG(ICompressWriteCoderProperties, Filter, _WriteCoderProperties)
	// MY_QUERYINTERFACE_ENTRY_AG(ICryptoResetSalt, Filter, _CryptoResetSalt)
	MY_QUERYINTERFACE_ENTRY_AG(ICryptoResetInitVector, Filter, _CryptoResetInitVector)
#endif
	MY_QUERYINTERFACE_ENTRY_AG(ICompressSetDecoderProperties2, Filter, _SetDecoderProperties2)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE
	STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
	STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
	STDMETHOD(InitEncoder) ();
	STDMETHOD(SetInStream) (ISequentialInStream *inStream);
	STDMETHOD(ReleaseInStream) ();
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(SetOutStream) (ISequentialOutStream *outStream);
	STDMETHOD(ReleaseOutStream) ();
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(OutStreamFinish) ();
	STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size);
	STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size);
#ifndef _NO_CRYPTO
	STDMETHOD(CryptoSetPassword) (const Byte *data, uint32 size);
	STDMETHOD(SetKey) (const Byte *data, uint32 size);
	STDMETHOD(SetInitVector) (const Byte *data, uint32 size);
#endif
#ifndef EXTRACT_ONLY
	STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *properties, uint32 numProperties);
	STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);
	// STDMETHOD(ResetSalt)();
	STDMETHOD(ResetInitVector) ();
#endif
	STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
	HRESULT Init_NoSubFilterInit();
};
//
//#include <InBuffer.h>
#ifndef _NO_EXCEPTIONS
	struct CInBufferException : public CSystemException {
		CInBufferException(HRESULT errorCode) : CSystemException(errorCode) 
		{
		}
	};
#endif

class CInBufferBase {
protected:
	Byte * _buf;
	Byte * _bufLim;
	Byte * _bufBase;
	ISequentialInStream * _stream;
	uint64 _processedSize;
	size_t _bufSize; // actually it's number of Bytes for next read. The buf can be larger only up to 32-bits values now are supported!
	bool   _wasFinished;
	uint8  Reserve[3]; // @alignment

	bool   ReadBlock();
	bool   FASTCALL ReadByte_FromNewBlock(Byte &b);
	Byte   ReadByte_FromNewBlock();
public:
  #ifdef _NO_EXCEPTIONS
	HRESULT ErrorCode;
  #endif
	uint32 NumExtraBytes;

	CInBufferBase() throw();
	uint64 GetStreamSize() const { return _processedSize + (_buf - _bufBase); }
	uint64 GetProcessedSize() const { return _processedSize + NumExtraBytes + (_buf - _bufBase); }
	bool   WasFinished() const { return _wasFinished; }
	void   SetStream(ISequentialInStream * stream) { _stream = stream; }
	void   SetBuf(Byte * buf, size_t bufSize, size_t end, size_t pos);
	void   Init() throw();
	FORCEINLINE bool ReadByte(Byte &b)
	{
		if(_buf >= _bufLim)
			return ReadByte_FromNewBlock(b);
		b = *_buf++;
		return true;
	}
	FORCEINLINE Byte ReadByte()
	{
		return (_buf >= _bufLim) ? ReadByte_FromNewBlock() : *_buf++;
	}
	size_t FASTCALL ReadBytes(Byte * buf, size_t size);
	size_t Skip(size_t size);
};

class CInBuffer : public CInBufferBase {
public: 
	~CInBuffer();
	bool Create(size_t bufSize) throw(); // only up to 32-bits values now are supported!
	void Free() throw();
};
//
//#include <OutBuffer.h>
#ifndef _NO_EXCEPTIONS
struct COutBufferException : public CSystemException {
	COutBufferException(HRESULT errorCode) : CSystemException(errorCode) 
	{
	}
};
#endif

class COutBuffer {
protected:
	Byte * _buf;
	uint32 _pos;
	uint32 _limitPos;
	uint32 _streamPos;
	uint32 _bufSize;
	ISequentialOutStream * _stream;
	uint64 _processedSize;
	Byte  * _buf2;
	bool   _overDict;
	uint8  Reserve[3]; // @alignment

	HRESULT FlushPart() throw();
public:
#ifdef _NO_EXCEPTIONS
	HRESULT ErrorCode;
#endif
	COutBuffer();
	~COutBuffer();
	bool Create(uint32 bufSize) throw();
	void Free() throw();
	void SetMemStream(Byte * buf) { _buf2 = buf; }
	void SetStream(ISequentialOutStream * stream) { _stream = stream; }
	void Init() throw();
	HRESULT Flush() throw();
	void FlushWithCheck();
	void FASTCALL WriteByte(Byte b);
	void FASTCALL WriteBytes(const void * data, size_t size);
	uint64 GetProcessedSize() const throw();
};
//
//#include <CreateCoder.h>
// 
// if EXTERNAL_CODECS is not defined, the code supports only codecs that
//   are statically linked at compile-time and link-time.
// if EXTERNAL_CODECS is defined, the code supports also codecs from another
//   executable modules, that can be linked dynamically at run-time:
//     - EXE module can use codecs from external DLL files.
//     - DLL module can use codecs from external EXE and DLL files.
// CExternalCodecs contains information about codecs and interfaces to create them.
// The order of codecs:
//   1) Internal codecs
//   2) External codecs
// 
#ifdef EXTERNAL_CODECS
	struct CCodecInfoEx {
		CCodecInfoEx() : EncoderIsAssigned(false), DecoderIsAssigned(false) 
		{
		}
		CMethodId Id;
		AString Name;
		uint32 NumStreams;
		bool EncoderIsAssigned;
		bool DecoderIsAssigned;
	};

	struct CHasherInfoEx {
		CMethodId Id;
		AString Name;
	};

	#define PUBLIC_ISetCompressCodecsInfo public ISetCompressCodecsInfo,
	#define QUERY_ENTRY_ISetCompressCodecsInfo MY_QUERYINTERFACE_ENTRY(ISetCompressCodecsInfo)
	#define DECL_ISetCompressCodecsInfo STDMETHOD(SetCompressCodecsInfo) (ICompressCodecsInfo *compressCodecsInfo);
	#define IMPL_ISetCompressCodecsInfo2(x)	\
		STDMETHODIMP x::SetCompressCodecsInfo(ICompressCodecsInfo *compressCodecsInfo) { \
			COM_TRY_BEGIN __externalCodecs.GetCodecs = compressCodecsInfo;  return __externalCodecs.Load(); COM_TRY_END }
	#define IMPL_ISetCompressCodecsInfo IMPL_ISetCompressCodecsInfo2(CHandler)

	struct CExternalCodecs {
		bool IsSet() const { return GetCodecs != NULL || GetHashers != NULL; }
		HRESULT Load();
		void ClearAndRelease()
		{
			Hashers.Clear();
			Codecs.Clear();
			GetHashers.Release();
			GetCodecs.Release();
		}
		~CExternalCodecs()
		{
			GetHashers.Release();
			GetCodecs.Release();
		}
		CMyComPtr<ICompressCodecsInfo> GetCodecs;
		CMyComPtr<IHashers> GetHashers;
		CObjectVector<CCodecInfoEx> Codecs;
		CObjectVector<CHasherInfoEx> Hashers;
	};

	extern CExternalCodecs g_ExternalCodecs;

	#define EXTERNAL_CODECS_VARS2   (__externalCodecs.IsSet() ? &__externalCodecs : &g_ExternalCodecs)
	#define EXTERNAL_CODECS_VARS2_L (&__externalCodecs)
	#define EXTERNAL_CODECS_VARS2_G (&g_ExternalCodecs)
	#define DECL_EXTERNAL_CODECS_VARS CExternalCodecs __externalCodecs;
	#define EXTERNAL_CODECS_VARS   EXTERNAL_CODECS_VARS2,
	#define EXTERNAL_CODECS_VARS_L EXTERNAL_CODECS_VARS2_L,
	#define EXTERNAL_CODECS_VARS_G EXTERNAL_CODECS_VARS2_G,
	#define DECL_EXTERNAL_CODECS_LOC_VARS2 const CExternalCodecs *__externalCodecs
	#define EXTERNAL_CODECS_LOC_VARS2 __externalCodecs
	#define DECL_EXTERNAL_CODECS_LOC_VARS DECL_EXTERNAL_CODECS_LOC_VARS2,
	#define EXTERNAL_CODECS_LOC_VARS EXTERNAL_CODECS_LOC_VARS2,
#else
	#define PUBLIC_ISetCompressCodecsInfo
	#define QUERY_ENTRY_ISetCompressCodecsInfo
	#define DECL_ISetCompressCodecsInfo
	#define IMPL_ISetCompressCodecsInfo
	#define EXTERNAL_CODECS_VARS2
	#define DECL_EXTERNAL_CODECS_VARS
	#define EXTERNAL_CODECS_VARS
	#define EXTERNAL_CODECS_VARS_L
	#define EXTERNAL_CODECS_VARS_G
	#define DECL_EXTERNAL_CODECS_LOC_VARS2
	#define EXTERNAL_CODECS_LOC_VARS2
	#define DECL_EXTERNAL_CODECS_LOC_VARS
	#define EXTERNAL_CODECS_LOC_VARS
#endif

bool FindMethod(DECL_EXTERNAL_CODECS_LOC_VARS const AString &name, CMethodId &methodId, uint32 &numStreams);
bool FindMethod(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, AString &name);
bool FindHashMethod(DECL_EXTERNAL_CODECS_LOC_VARS const AString &name, CMethodId &methodId);
void GetHashMethods(DECL_EXTERNAL_CODECS_LOC_VARS CRecordVector<CMethodId> &methods);

struct CCreatedCoder {
	CMyComPtr <ICompressCoder> Coder;
	CMyComPtr <ICompressCoder2> Coder2;
	bool   IsExternal;
	bool   IsFilter; // = true, if Coder was created from filter
	uint32 NumStreams;
	// CCreatedCoder(): IsExternal(false), IsFilter(false), NumStreams(1) {}
};

HRESULT CreateCoder(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, bool encode, CMyComPtr<ICompressFilter> &filter, CCreatedCoder &cod);
HRESULT CreateCoder(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, bool encode, CCreatedCoder &cod);
HRESULT CreateCoder(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, bool encode, CMyComPtr<ICompressCoder> &coder);
HRESULT CreateFilter(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, bool encode, CMyComPtr<ICompressFilter> &filter);
HRESULT CreateHasher(DECL_EXTERNAL_CODECS_LOC_VARS CMethodId methodId, AString &name, CMyComPtr<IHasher> &hasher);
//
//#include <ProgressUtils.h>
class CLocalProgress : public ICompressProgressInfo, public CMyUnknownImp {
private:
	CMyComPtr <IProgress> _progress;
	CMyComPtr <ICompressProgressInfo> _ratioProgress;
	bool _inSizeIsMain;
public:
	CLocalProgress();
	void Init(IProgress * progress, bool inSizeIsMain);
	HRESULT SetCur();
	MY_UNKNOWN_IMP1(ICompressProgressInfo)
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
	uint64 ProgressOffset;
	uint64 InSize;
	uint64 OutSize;
	bool SendRatio;
	bool SendProgress;
};
//
//#include <CWrappers.h>
SRes HRESULT_To_SRes(HRESULT res, SRes defaultRes) throw();
HRESULT SResToHRESULT(SRes res) throw();

struct CCompressProgressWrap {
	void FASTCALL Init(ICompressProgressInfo * progress) throw();

	ICompressProgress vt;
	ICompressProgressInfo * Progress;
	HRESULT Res;
};

struct CSeqInStreamWrap {
	void FASTCALL Init(ISequentialInStream * stream) throw();

	ISeqInStream vt;
	ISequentialInStream * Stream;
	HRESULT Res;
	uint64 Processed;
};

struct CSeekInStreamWrap {
	void FASTCALL Init(IInStream * stream) throw();

	ISeekInStream vt;
	IInStream * Stream;
	HRESULT Res;
};

struct CSeqOutStreamWrap {
	void FASTCALL Init(ISequentialOutStream * stream) throw();

	ISeqOutStream vt;
	ISequentialOutStream * Stream;
	HRESULT Res;
	uint64 Processed;
};

struct CByteInBufWrap {
	CByteInBufWrap();
	~CByteInBufWrap();
	void   Free() throw();
	bool   Alloc(uint32 size) throw();
	void   Init();
	uint64 GetProcessed() const { return Processed + (Cur - Buf); }
	Byte   ReadByteFromNewBlock() throw();
	Byte   ReadByte() { return (Cur != Lim) ? *Cur++ : ReadByteFromNewBlock(); }

	IByteIn vt;
	const Byte * Cur;
	const Byte * Lim;
	Byte * Buf;
	uint32 Size;
	ISequentialInStream * Stream;
	uint64 Processed;
	bool Extra;
	HRESULT Res;
};

struct CByteOutBufWrap {
	CByteOutBufWrap() throw();
	~CByteOutBufWrap();
	void Free() throw();
	bool Alloc(size_t size) throw();
	void Init();
	uint64 GetProcessed() const { return Processed + (Cur - Buf); }
	HRESULT Flush() throw();
	void FASTCALL WriteByte(Byte b);

	IByteOut vt;
	Byte * Cur;
	const Byte * Lim;
	Byte * Buf;
	size_t Size;
	ISequentialOutStream * Stream;
	uint64 Processed;
	HRESULT Res;
};
//
//#include <BitlDecoder.h>
namespace NBitl {
	const uint kNumBigValueBits = 8 * 4;
	const uint kNumValueBytes = 3;
	const uint kNumValueBits = 8 * kNumValueBytes;
	const uint32 kMask = (1 << kNumValueBits) - 1;

	extern Byte kInvertTable[256];

	/* TInByte must support "Extra Bytes" (bytes that can be read after the end of stream
	   TInByte::ReadByte() returns 0xFF after the end of stream
	   TInByte::NumExtraBytes contains the number "Extra Bytes"

	   Bitl decoder can read up to 4 bytes ahead to internal buffer. */

	template <class TInByte> class CBaseDecoder {
	protected:
		uint   _bitPos;
		uint32 _value;
		TInByte _stream;
	public:
		bool Create(uint32 bufSize) { return _stream.Create(bufSize); }
		void SetStream(ISequentialInStream * inStream) { _stream.SetStream(inStream); }
		void Init()
		{
			_stream.Init();
			_bitPos = kNumBigValueBits;
			_value = 0;
		}
		uint64 GetStreamSize() const { return _stream.GetStreamSize(); }
		uint64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }
		bool ThereAreDataInBitsBuffer() const { return this->_bitPos != kNumBigValueBits; }
		FORCEINLINE void Normalize()
		{
			for(; _bitPos >= 8; _bitPos -= 8)
				_value = ((uint32)_stream.ReadByte() << (kNumBigValueBits - _bitPos)) | _value;
		}
		FORCEINLINE uint32 ReadBits(uint numBits)
		{
			Normalize();
			uint32 res = _value & ((1 << numBits) - 1);
			_bitPos += numBits;
			_value >>= numBits;
			return res;
		}
		bool ExtraBitsWereRead() const
		{
			return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3));
		}
		bool ExtraBitsWereRead_Fast() const
		{
			// full version is not inlined in vc6.
			// return _stream.NumExtraBytes != 0 && (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos <
			// (_stream.NumExtraBytes << 3));

			// (_stream.NumExtraBytes > 4) is fast overread detection. It's possible that
			// it doesn't return true, if small number of extra bits were read.
			return (_stream.NumExtraBytes > 4);
		}

		// it must be fixed !!! with extra bits
		// uint32 GetNumExtraBytes() const { return _stream.NumExtraBytes; }
	};

	template <class TInByte> class CDecoder : public CBaseDecoder<TInByte> {
		uint32 _normalValue;
	public:
		void Init()
		{
			CBaseDecoder<TInByte>::Init();
			_normalValue = 0;
		}
		FORCEINLINE void Normalize()
		{
			for(; this->_bitPos >= 8; this->_bitPos -= 8) {
				Byte b = this->_stream.ReadByte();
				_normalValue = ((uint32)b << (kNumBigValueBits - this->_bitPos)) | _normalValue;
				this->_value = (this->_value << 8) | kInvertTable[b];
			}
		}
		FORCEINLINE uint32 GetValue(uint numBits)
		{
			Normalize();
			return ((this->_value >> (8 - this->_bitPos)) & kMask) >> (kNumValueBits - numBits);
		}
		FORCEINLINE void MovePos(uint numBits)
		{
			this->_bitPos += numBits;
			_normalValue >>= numBits;
		}
		FORCEINLINE uint32 ReadBits(uint numBits)
		{
			Normalize();
			uint32 res = _normalValue & ((1 << numBits) - 1);
			MovePos(numBits);
			return res;
		}
		void AlignToByte() 
		{
			MovePos((32 - this->_bitPos) & 7);
		}
		FORCEINLINE Byte ReadDirectByte() 
		{
			return this->_stream.ReadByte();
		}
		FORCEINLINE Byte ReadAlignedByte()
		{
			if(this->_bitPos == kNumBigValueBits)
				return this->_stream.ReadByte();
			Byte b = (Byte)(_normalValue & 0xFF);
			MovePos(8);
			return b;
		}
	};
}
//
//#include <BitlEncoder.h>
class CBitlEncoder {
public:
	bool   FASTCALL Create(uint32 bufSize);
	void   FASTCALL SetStream(ISequentialOutStream * outStream);
	// unsigned GetBitPosition() const;
	uint64 GetProcessedSize() const;
	void   Init();
	HRESULT Flush();
	void   FlushByte();
	void   WriteBits(uint32 value, unsigned numBits);
	void   FASTCALL WriteByte(Byte b);
private:
	COutBuffer _stream;
	uint   _bitPos;
	Byte   _curByte;
	uint8  Reserve[3]; // @alignment
};
//
//#include <BitmDecoder.h>
namespace NBitm {
	const uint kNumBigValueBits = 8 * 4;
	const uint kNumValueBytes = 3;
	const uint kNumValueBits = 8 * kNumValueBytes;
	const uint32 kMask = (1 << kNumValueBits) - 1;

	// _bitPos - the number of free bits (high bits in _value)
	// (kNumBigValueBits - _bitPos) = (32 - _bitPos) == the number of ready to read bits (low bits of _value)

	template <class TInByte> class CDecoder {
		uint   _bitPos;
		uint32 _value;
		TInByte _stream;
	public:
		bool Create(uint32 bufSize) { return _stream.Create(bufSize); }
		void SetStream(ISequentialInStream * inStream) { _stream.SetStream(inStream); }
		void Init()
		{
			_stream.Init();
			_bitPos = kNumBigValueBits;
			_value = 0;
			Normalize();
		}
		uint64 GetStreamSize() const { return _stream.GetStreamSize(); }
		uint64 GetProcessedSize() const { return _stream.GetProcessedSize() - ((kNumBigValueBits - _bitPos) >> 3); }
		bool ExtraBitsWereRead() const { return (_stream.NumExtraBytes > 4 || kNumBigValueBits - _bitPos < (_stream.NumExtraBytes << 3)); }
		bool ExtraBitsWereRead_Fast() const { return (_stream.NumExtraBytes > 4); }
		FORCEINLINE void Normalize()
		{
			for(; _bitPos >= 8; _bitPos -= 8)
				_value = (_value << 8) | _stream.ReadByte();
		}
		FORCEINLINE uint32 GetValue(uint numBits) const
		{
			// return (_value << _bitPos) >> (kNumBigValueBits - numBits);
			return ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - numBits);
		}
		FORCEINLINE void MovePos(uint numBits)
		{
			_bitPos += numBits;
			Normalize();
		}
		FORCEINLINE uint32 ReadBits(uint numBits)
		{
			uint32 res = GetValue(numBits);
			MovePos(numBits);
			return res;
		}
		/*
		   unsigned ReadBit()
		   {
		   uint32 res = ((_value >> (8 - _bitPos)) & kMask) >> (kNumValueBits - 1);
		   if(++_bitPos >= 8) {
			_value = (_value << 8) | _stream.ReadByte();
			_bitPos -= 8;
		   }
		   return (uint)res;
		   }
		 */
		void AlignToByte() { MovePos((kNumBigValueBits - _bitPos) & 7); }
		FORCEINLINE uint32 ReadAlignBits() 
		{
			return ReadBits((kNumBigValueBits - _bitPos) & 7);
		}
	};
}
//
//#include <LzOutWindow.h>
#ifndef _NO_EXCEPTIONS
	typedef COutBufferException CLzOutWindowException;
#endif

class CLzOutWindow : public COutBuffer {
public:
	void   Init(bool solid = false) throw();
	// distance >= 0, len > 0,
	bool   CopyBlock(uint32 distance, uint32 len);
	void   FASTCALL PutByte(Byte b);
	Byte   FASTCALL GetByte(uint32 distance) const;
};
//
//#include <CopyCoder.h> <LzmaDecoder.h> <Lzma2Decoder.h> <LzmaEncoder.h> <Lzma2Encoder.h> <XzDecoder.h> <XzEncoder.h> <DeflateDecoder.h> <DeflateEncoder.h>
//  <BZip2Decoder.h> <BZip2Encoder.h> <ShrinkDecoder.h> <PpmdZip.h> <BcjCoder.h> <Bcj2Coder.h> <PpmdDecoder.h> <PpmdEncoder.h> <QuantumDecoder.h> <ImplodeDecoder.h>
namespace NCompress {
	class CCopyCoder : public ICompressCoder, public ICompressSetInStream, public ISequentialInStream,
		public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
		Byte * _buf;
		CMyComPtr <ISequentialInStream> _inStream;
	public:
		uint64 TotalSize;
		CCopyCoder();
		~CCopyCoder();

		MY_UNKNOWN_IMP5(ICompressCoder, ICompressSetInStream, ISequentialInStream, ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
		STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
		STDMETHOD(SetInStream) (ISequentialInStream *inStream);
		STDMETHOD(ReleaseInStream) ();
		STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		STDMETHOD(SetFinishMode) (uint32 finishMode);
		STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
	};

	HRESULT CopyStream(ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress);
	HRESULT CopyStream_ExactSize(ISequentialInStream * inStream, ISequentialOutStream * outStream, uint64 size, ICompressProgressInfo * progress);

	namespace NLzma {
		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public ICompressSetFinishMode,
			public ICompressGetInStreamProcessedSize, public ICompressSetBufSize,
		  #ifndef NO_READ_FROM_CODER
			public ICompressSetInStream, public ICompressSetOutStreamSize, public ISequentialInStream,
		  #endif
			public CMyUnknownImp {
			Byte * _inBuf;
			uint32 _inPos;
			uint32 _inLim;
			CLzmaDec _state;
			ELzmaStatus _lzmaStatus;
		public:
			bool   FinishStream; // set it before decoding, if you need to decode full LZMA stream
		private:
			bool   _propsWereSet;
			bool   _outSizeDefined;
			uint64 _outSize;
			uint64 _inProcessed;
			uint64 _outProcessed;
			uint32 _outStep;
			uint32 _inBufSize;
			uint32 _inBufSizeNew;

			HRESULT CreateInputBuffer();
			HRESULT CodeSpec(ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress);
			void SetOutStreamSizeResume(const uint64 * outSize);
		public:
			MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
			MY_QUERYINTERFACE_ENTRY(ICompressSetDecoderProperties2)
			MY_QUERYINTERFACE_ENTRY(ICompressSetFinishMode)
			MY_QUERYINTERFACE_ENTRY(ICompressGetInStreamProcessedSize)
			MY_QUERYINTERFACE_ENTRY(ICompressSetBufSize)
		  #ifndef NO_READ_FROM_CODER
			MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
			MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
			MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
		  #endif
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE

			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
			STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
			STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size);
			STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size);

		  #ifndef NO_READ_FROM_CODER
		private:
			CMyComPtr<ISequentialInStream> _inStream;
		public:
			STDMETHOD(SetInStream) (ISequentialInStream *inStream);
			STDMETHOD(ReleaseInStream) ();
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			HRESULT CodeResume(ISequentialOutStream * outStream, const uint64 * outSize, ICompressProgressInfo * progress);
			HRESULT ReadFromInputStream(void * data, uint32 size, uint32 * processedSize);
		  #endif
			uint64 GetInputProcessedSize() const { return _inProcessed; }
			CDecoder();
			virtual ~CDecoder();
			uint64 GetOutputProcessedSize() const { return _outProcessed; }
			bool NeedsMoreInput() const { return _lzmaStatus == LZMA_STATUS_NEEDS_MORE_INPUT; }
			bool CheckFinishStatus(bool withEndMark) const { return _lzmaStatus == (withEndMark ? LZMA_STATUS_FINISHED_WITH_MARK : LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK); }
		};

		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties, public ICompressWriteCoderProperties, public CMyUnknownImp {
			CLzmaEncHandle _encoder;
			uint64 _inputProcessed;
		public:
			MY_UNKNOWN_IMP3(ICompressCoder, ICompressSetCoderProperties, ICompressWriteCoderProperties)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);

			CEncoder();
			virtual ~CEncoder();
			uint64 GetInputProcessedSize() const { return _inputProcessed; }
			bool IsWriteEndMark() const { return LzmaEnc_IsWriteEndMark(_encoder) != 0; }
		};
	}
	namespace NLzma2 {
		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2,
			public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public ICompressSetBufSize,
		#ifndef NO_READ_FROM_CODER
			public ICompressSetInStream, public ICompressSetOutStreamSize, public ISequentialInStream,
		#endif
			public CMyUnknownImp
		{
			Byte * _inBuf;
			uint32 _inPos;
			uint32 _inLim;
			bool   _finishMode;
			bool   _outSizeDefined;
			uint64 _outSize;
			uint64 _inProcessed;
			uint64 _outProcessed;
			uint32 _outStep;
			uint32 _inBufSize;
			uint32 _inBufSizeNew;
			CLzma2Dec _state;
		public:
			MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
			MY_QUERYINTERFACE_ENTRY(ICompressSetDecoderProperties2)
			MY_QUERYINTERFACE_ENTRY(ICompressSetFinishMode)
			MY_QUERYINTERFACE_ENTRY(ICompressGetInStreamProcessedSize)
			MY_QUERYINTERFACE_ENTRY(ICompressSetBufSize)
		  #ifndef NO_READ_FROM_CODER
			MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
			MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
			MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
		  #endif
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE

			STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
			STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
			STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size);
			STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size);
		#ifndef NO_READ_FROM_CODER
		private:
			CMyComPtr<ISequentialInStream> _inStream;
		public:
			STDMETHOD(SetInStream) (ISequentialInStream *inStream);
			STDMETHOD(ReleaseInStream) ();
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		#endif
			CDecoder();
			virtual ~CDecoder();
		};

		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties, public ICompressWriteCoderProperties, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP3(ICompressCoder, ICompressSetCoderProperties, ICompressWriteCoderProperties)
			STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);
			CEncoder();
			virtual ~CEncoder();
		private:
			CLzma2EncHandle _encoder;
		};
	}
	namespace NXz {
		struct CXzUnpackerCPP {
			CXzUnpackerCPP();
			~CXzUnpackerCPP();
			Byte * InBuf;
			Byte * OutBuf;
			CXzUnpacker p;
		};

		struct CStatInfo {
			CStatInfo(); 
			void Clear();

			uint64 InSize;
			uint64 OutSize;
			uint64 PhySize;
			uint64 NumStreams;
			uint64 NumBlocks;
			bool   UnpackSize_Defined;
			bool   NumStreams_Defined;
			bool   NumBlocks_Defined;
			bool   IsArc;
			bool   UnexpectedEnd;
			bool   DataAfterEnd;
			bool   Unsupported;
			bool   HeadersError;
			bool   DataError;
			bool   CrcError;
			uint8  Reserve[2]; // @alignment
		};

		struct CDecoder : public CStatInfo {
			CXzUnpackerCPP xzu;
			SRes DecodeRes; // it's not HRESULT
			CDecoder();
			/* Decode() can return ERROR code only if there is progress or stream error.
			   Decode() returns S_OK in case of xz decoding error, but DecodeRes and CStatInfo contain error information */
			HRESULT Decode(ISequentialInStream * seqInStream, ISequentialOutStream * outStream, const uint64 * outSizeLimit, bool finishStream, ICompressProgressInfo * compressProgress);
			int32 Get_Extract_OperationResult() const;
		};

		class CComDecoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize, public CMyUnknownImp {
		public:
			CComDecoder();
			MY_UNKNOWN_IMP2(ICompressSetFinishMode, ICompressGetInStreamProcessedSize)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
		private:
			CDecoder _decoder;
			bool _finishStream;
		};

		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties, public CMyUnknownImp {
			// CXzEncHandle _encoder;
		public:
			CLzma2EncProps _lzma2Props;
			CXzProps xzProps;
			CXzFilterProps filter;
			MY_UNKNOWN_IMP2(ICompressCoder, ICompressSetCoderProperties)
			void InitCoderProps();
			HRESULT SetCoderProp(PROPID propID, const PROPVARIANT &prop);
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			CEncoder();
			virtual ~CEncoder();
		};
	}
	namespace NDeflate {
		namespace NDecoder {
			const int kLenIdFinished = -1;
			const int kLenIdNeedInit = -2;

			class CCoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize,
			#ifndef NO_READ_FROM_CODER
				public ICompressSetInStream, public ICompressSetOutStreamSize, public ISequentialInStream,
			#endif
				public CMyUnknownImp {
				CLzOutWindow m_OutWindowStream;
				CMyComPtr <ISequentialInStream> m_InStreamRef;
				NBitl::CDecoder <CInBuffer> m_InBitStream;
				NCompress::NHuffman::CDecoder <kNumHuffmanBits, kFixedMainTableSize> m_MainDecoder;
				NCompress::NHuffman::CDecoder <kNumHuffmanBits, kFixedDistTableSize> m_DistDecoder;
				NCompress::NHuffman::CDecoder7b <kLevelTableSize> m_LevelDecoder;

				uint32 m_StoredBlockSize;
				uint32 _numDistLevels;
				int32 _remainLen;
				uint32 _rep0;
				uint64 _outSize;
				uint64 _outStartPos;
				bool   m_FinalBlock;
				bool   m_StoredMode;
				bool   _deflateNSIS;
				bool   _deflate64Mode;
				bool   _keepHistory;
				bool   _needFinishInput;
				bool   _needInitInStream;
				bool   _needReadTable;
				bool   _outSizeDefined;
				uint8  Reserve[3]; // @alignment

				void SetOutStreamSizeResume(const uint64 * outSize);
				uint64 GetOutProcessedCur() const { return m_OutWindowStream.GetProcessedSize() - _outStartPos; }
				uint32 ReadBits(uint numBits);
				bool DecodeLevels(Byte * levels, unsigned numSymbols);
				bool ReadTables();
				HRESULT Flush() { return m_OutWindowStream.Flush(); }
				class CCoderReleaser {
					CCoder * _coder;
				public:
					bool NeedFlush;
					CCoderReleaser(CCoder * coder);
					~CCoderReleaser();
				};
				friend class CCoderReleaser;
				HRESULT CodeSpec(uint32 curSize, bool finishInputStream, uint32 inputProgressLimit = 0);
			public:
				bool ZlibMode;
				Byte ZlibFooter[4];

				CCoder(bool deflate64Mode);
				virtual ~CCoder() 
				{
				}
				void SetNsisMode(bool nsisMode) { _deflateNSIS = nsisMode; }
				void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
				void Set_NeedFinishInput(bool needFinishInput) { _needFinishInput = needFinishInput; }
				bool IsFinished() const { return _remainLen == kLenIdFinished;; }
				bool IsFinalBlock() const { return m_FinalBlock; }
				HRESULT CodeReal(ISequentialOutStream * outStream, ICompressProgressInfo * progress);
				MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
				MY_QUERYINTERFACE_ENTRY(ICompressSetFinishMode)
				MY_QUERYINTERFACE_ENTRY(ICompressGetInStreamProcessedSize)
			  #ifndef NO_READ_FROM_CODER
				MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
				MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
				MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
			  #endif
				MY_QUERYINTERFACE_END
				MY_ADDREF_RELEASE
				STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream,
					const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
				STDMETHOD(SetFinishMode) (uint32 finishMode);
				STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
				STDMETHOD(SetInStream) (ISequentialInStream *inStream);
				STDMETHOD(ReleaseInStream) ();
				STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
			  #ifndef NO_READ_FROM_CODER
				STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			  #endif
				HRESULT CodeResume(ISequentialOutStream * outStream, const uint64 * outSize, ICompressProgressInfo * progress);
				HRESULT InitInStream(bool needInit);
				void AlignToByte() { m_InBitStream.AlignToByte(); }
				Byte ReadAlignedByte();
				uint32 ReadAligned_UInt16() // aligned for Byte range
				{
					uint32 v = m_InBitStream.ReadAlignedByte();
					return v | ((uint32)m_InBitStream.ReadAlignedByte() << 8);
				}
				bool InputEofError() const { return m_InBitStream.ExtraBitsWereRead(); }
				uint64 GetStreamSize() const { return m_InBitStream.GetStreamSize(); }
				uint64 GetInputProcessedSize() const { return m_InBitStream.GetProcessedSize(); }
			};

			class CCOMCoder : public CCoder { 
			public:
				CCOMCoder() : CCoder(false) 
				{
				} 
			};

			class CCOMCoder64 : public CCoder { 
			public: 
				CCOMCoder64() : CCoder(true) 
				{
				} 
			};
		}
		namespace NEncoder {
			struct CCodeValue {
				uint16 Len;
				uint16 Pos;
				void SetAsLiteral() { Len = (1 << 15); }
				bool IsLiteral() const { return (Len >= (1 << 15)); }
			};

			struct COptimal {
				uint32 Price;
				uint16 PosPrev;
				uint16 BackPrev;
			};

			const uint32 kNumOptsBase = 1 << 12;
			const uint32 kNumOpts = kNumOptsBase + kMatchMaxLen;

			class CCoder;

			struct CTables : public CLevels {
				void   InitStructures();

				bool   UseSubBlocks;
				bool   StoreMode;
				bool   StaticMode;
				uint8  Reserve; // @alignment
				uint32 BlockSizeRes;
				uint32 m_Pos;
			};

			struct CEncProps {
				CEncProps();
				void Normalize();

				int    Level;
				int    algo;
				int    fb;
				int    btMode;
				uint32 mc;
				uint32 numPasses;
			};

			class CCoder {
				CMatchFinder _lzInWindow;
				CBitlEncoder m_OutStream;
			public:
				CCodeValue * m_Values;
				uint16 * m_MatchDistances;
				uint32 m_NumFastBytes;
				uint16 * m_OnePosMatchesMemory;
				uint16 * m_DistanceMemory;
				uint32 m_Pos;
				uint   m_NumPasses;
				uint   m_NumDivPasses;
				uint32 m_ValueBlockSize;
				uint32 m_NumLenCombinations;
				uint32 m_MatchMaxLen;
				const  Byte * m_LenStart;
				const  Byte * m_LenDirectBits;
				Byte   m_LevelLevels[kLevelTableSize];
				uint8  Reserve1; // @alignment
				uint   m_NumLitLenLevels;
				uint   m_NumDistLevels;
				uint32 m_NumLevelCodes;
				uint32 m_ValueIndex;
				uint32 m_AdditionalOffset;
				uint32 m_OptimumEndIndex;
				uint32 m_OptimumCurrentIndex;
				Byte   m_LiteralPrices[256];
				Byte   m_LenPrices[kNumLenSymbolsMax];
				Byte   m_PosPrices[kDistTableSize64];
				CLevels m_NewLevels;
				uint32 mainFreqs[kFixedMainTableSize];
				uint32 distFreqs[kDistTableSize64];
				uint32 mainCodes[kFixedMainTableSize];
				uint32 distCodes[kDistTableSize64];
				uint32 levelCodes[kLevelTableSize];
				Byte   levelLens[kLevelTableSize];
				uint8  Reserve3[1]; // @alignment
				uint32 BlockSizeRes;
				CTables * m_Tables;
				COptimal m_Optimum[kNumOpts];
				uint32 m_MatchFinderCycles;
				bool   _fastMode;
				bool   _btMode;
				bool   m_CheckStatic;
				bool   m_IsMultiPass;
				bool   m_Created;
				bool   m_Deflate64Mode;
				bool   m_SecondPass;
				uint8  Reserve2[1]; // @alignment

				void GetMatches();
				void MovePos(uint32 num);
				uint32 Backward(uint32 &backRes, uint32 cur);
				uint32 GetOptimal(uint32 &backRes);
				uint32 GetOptimalFast(uint32 &backRes);
				void LevelTableDummy(const Byte * levels, unsigned numLevels, uint32 * freqs);
				void WriteBits(uint32 value, unsigned numBits);
				void LevelTableCode(const Byte * levels, unsigned numLevels, const Byte * lens, const uint32 * codes);
				void MakeTables(unsigned maxHuffLen);
				uint32 GetLzBlockPrice() const;
				void TryBlock();
				uint32 TryDynBlock(unsigned tableIndex, uint32 numPasses);
				uint32 TryFixedBlock(unsigned tableIndex);
				void SetPrices(const CLevels &levels);
				void WriteBlock();
				HRESULT Create();
				void Free();
				void WriteStoreBlock(uint32 blockSize, uint32 additionalOffset, bool finalBlock);
				void WriteTables(bool writeMode, bool finalBlock);
				void WriteBlockData(bool writeMode, bool finalBlock);
				uint32 GetBlockPrice(unsigned tableIndex, unsigned numDivPasses);
				void CodeBlock(unsigned tableIndex, bool finalBlock);
				void SetProps(const CEncProps * props2);
			public:
				CCoder(bool deflate64Mode = false);
				~CCoder();
				HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
				HRESULT BaseCode(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
				HRESULT BaseSetEncoderProperties2(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps);
			};

			class CCOMCoder : public ICompressCoder, public ICompressSetCoderProperties, public CMyUnknownImp, public CCoder {
			public:
				MY_UNKNOWN_IMP2(ICompressCoder, ICompressSetCoderProperties) CCOMCoder() : CCoder(false) 
				{
				}
				STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
				STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			};

			class CCOMCoder64 : public ICompressCoder, public ICompressSetCoderProperties, public CMyUnknownImp, public CCoder {
			public:
				MY_UNKNOWN_IMP2(ICompressCoder, ICompressSetCoderProperties) CCOMCoder64() : CCoder(true) 
				{
				}
				STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
				STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			};
		}
	}
	namespace NBZip2 {
		//bool   IsEndSig(const Byte * p) throw();
		//bool   IsBlockSig(const Byte * p) throw();
		const  uint kNumTableBits = 9;
		const  uint kNumBitsMax = kMaxHuffmanLen;
		typedef NHuffman::CDecoder <kMaxHuffmanLen, kMaxAlphaSize, kNumTableBits> CHuffmanDecoder;

		struct CBlockProps {
			CBlockProps() : blockSize(0), origPtr(0), randMode(0) 
			{
			}
			uint32 blockSize;
			uint32 origPtr;
			uint   randMode;
		};
		struct CBitDecoder {
			void InitBitDecoder();
			void AlignToByte();
			// bool AreRemainByteBitsEmpty() const;
			SRes ReadByte(int &b);

			uint   _numBits;
			uint32 _value;
			const Byte * _buf;
			const Byte * _lim;
		};

		struct CBase : public CBitDecoder {
			uint   numInUse;
			uint32 groupIndex;
			uint32 groupSize;
			uint   runPower;
			uint32 runCounter;
			uint32 blockSize;
			uint32 * Counters;
			uint32 blockSizeMax;
			uint   state;
			uint   state2;
			uint   state3;
			uint   state4;
			uint   state5;
			uint   numTables;
			uint32 numSelectors;
			CBlockProps Props;
		private:
			CMtf8Decoder mtf;
			Byte selectors[kNumSelectorsMax];
			CHuffmanDecoder huffs[kNumTablesMax];
			Byte lens[kMaxAlphaSize];
			Byte temp[10];
		public:
			uint32 crc;
			CBZip2CombinedCrc CombinedCrc;
			bool   IsBz;
			bool   StreamCrcError;
			bool   MinorError;
			bool   NeedMoreInput;
			bool   DecodeAllStreams;
			uint8  Reserve[3]; // @alignment
			uint64 NumStreams;
			uint64 NumBlocks;
			uint64 FinishedPackSize;
			ISequentialInStream * InStream;
		  #ifndef NO_READ_FROM_CODER
			CMyComPtr<ISequentialInStream> InStreamRef;
		  #endif
			CBase();
			void InitNumStreams2();
			SRes ReadStreamSignature2();
			SRes ReadBlockSignature2();
			/* ReadBlock2() : Props->randMode:
				 in:  need read randMode bit
				 out: randMode status */
			SRes ReadBlock2();
		};

		class CDecoder : public ICompressCoder, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize,
		  #ifndef NO_READ_FROM_CODER
			public ICompressSetInStream, public ICompressSetOutStreamSize, public ISequentialInStream,
		  #endif
		  #ifndef _7ZIP_ST
			public ICompressSetCoderMt,
		  #endif
			public CMyUnknownImp
		{
			Byte * _outBuf;
			size_t _outPos;
			uint64 _outWritten;
			ISequentialOutStream * _outStream;
			HRESULT _writeRes;
		protected:
			HRESULT ErrorResult; // for ISequentialInStream::Read mode only
		public:
			class CSpecState {
			public:
				void Init(uint32 origPtr, unsigned randMode) throw();
				bool Finished() const { return _reps <= 0 && _blockSize == 0; }
				Byte * Decode(Byte * data, size_t size) throw();

				CBZip2Crc _crc;
				uint32 _blockSize;
				uint32 * _tt;
				int    _randToGo;
				uint   _randIndex;
			private:
				uint32 _tPos;
				uint   _prevByte;
				int    _reps;
			};
			uint32 _calcedBlockCrc;
			bool   _blockFinished;
			bool   BlockCrcError;
			bool   FinishMode;
			bool   _outSizeDefined;
			uint64 _outSize;
			uint64 _outPosTotal;
			CSpecState _spec;
			uint32 * _counters;
		#ifndef _7ZIP_ST
			struct CBlock {
				bool StopScout;
				bool WasFinished;
				bool Crc_Defined;
				// bool NextCrc_Defined;
				uint32 Crc;
				uint32 NextCrc;
				HRESULT Res;
				uint64 PackPos;
				CBlockProps Props;
			};

			CBlock _block;
			bool NeedWaitScout;
			bool MtMode;
			NWindows::CThread Thread;
			NWindows::NSynchronization::CAutoResetEvent DecoderEvent;
			NWindows::NSynchronization::CAutoResetEvent ScoutEvent;
			// HRESULT ScoutRes;
			Byte MtPad[1 << 7]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.

			void RunScout();
			void WaitScout();

			class CWaitScout_Releaser {
				CDecoder * _decoder;
			public:
				explicit CWaitScout_Releaser(CDecoder * decoder);
				~CWaitScout_Releaser();
			};

			HRESULT CreateThread();
		  #endif
			Byte * _inBuf;
			uint64 _inProcessed;
			bool   _inputFinished;
			HRESULT _inputRes;
			CBase Base;

			bool GetCrcError() const { return BlockCrcError || Base.StreamCrcError; }
			void InitOutSize(const uint64 * outSize);
			bool CreateInputBufer();
			void InitInputBuffer();
			uint64 GetInputProcessedSize() const;
			uint64 GetOutProcessedSize() const;
			HRESULT ReadInput();
			void StartNewStream();
			HRESULT ReadStreamSignature();
			HRESULT StartRead();
			HRESULT ReadBlockSignature();
			HRESULT ReadBlock();
			HRESULT Flush();
			HRESULT DecodeBlock(const CBlockProps &props);
			HRESULT DecodeStreams(ICompressProgressInfo * progress);
			MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
			MY_QUERYINTERFACE_ENTRY(ICompressSetFinishMode)
			MY_QUERYINTERFACE_ENTRY(ICompressGetInStreamProcessedSize)
		  #ifndef NO_READ_FROM_CODER
			MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
			MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
			MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
		  #endif
		  #ifndef _7ZIP_ST
			MY_QUERYINTERFACE_ENTRY(ICompressSetCoderMt)
		  #endif
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
			uint64 GetNumStreams() const { return Base.NumStreams; }
			uint64 GetNumBlocks() const { return Base.NumBlocks; }
		  #ifndef NO_READ_FROM_CODER
			STDMETHOD(SetInStream) (ISequentialInStream *inStream);
			STDMETHOD(ReleaseInStream) ();
			STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		  #endif
		  #ifndef _7ZIP_ST
			STDMETHOD(SetNumberOfThreads) (uint32 numThreads);
		  #endif
			CDecoder();
			~CDecoder();
		};

		#ifndef NO_READ_FROM_CODER
			class CNsisDecoder : public CDecoder {
			public:
				STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			};
		#endif

		class CEncoder;
		class CMsbfEncoderTemp;

		const uint kNumPassesMax = 10;

		class CThreadInfo {
		public:
			Byte * m_Block;
		private:
			Byte * m_MtfArray;
			Byte * m_TempArray;
			uint32 * m_BlockSorterIndex;
			CMsbfEncoderTemp * m_OutStreamCurrent;
			Byte   Lens[kNumTablesMax][kMaxAlphaSize];
			uint32 Freqs[kNumTablesMax][kMaxAlphaSize];
			uint32 Codes[kNumTablesMax][kMaxAlphaSize];
			Byte   m_Selectors[kNumSelectorsMax];
			uint32 m_CRCs[1 << kNumPassesMax];
			uint32 m_NumCrcs;
			uint32 m_BlockIndex;

			void   FASTCALL WriteBits2(uint32 value, uint numBits);
			void   FASTCALL WriteByte2(Byte b);
			void   FASTCALL WriteBit2(Byte v);
			void   FASTCALL WriteCrc2(uint32 v);
			void   EncodeBlock(const Byte * block, uint32 blockSize);
			uint32 EncodeBlockWithHeaders(const Byte * block, uint32 blockSize);
			void   EncodeBlock2(const Byte * block, uint32 blockSize, uint32 numPasses);
		public:
			bool   m_OptimizeNumTables;
			CEncoder * Encoder;
		  #ifndef _7ZIP_ST
			NWindows::CThread Thread;
			NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
			NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;
			// it's not member of this thread. We just need one event per thread
			NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;
			uint64 m_PackSize;
			Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
			HRESULT Create();
			void FinishStream(bool needLeave);
			DWORD ThreadFunc();
		  #endif
			CThreadInfo();
			~CThreadInfo();
			bool   Alloc();
			void   Free();
			HRESULT EncodeBlock3(uint32 blockSize);
		};

		struct CEncProps {
			CEncProps();
			void Normalize(int level);
			bool DoOptimizeNumTables() const { return NumPasses > 1; }

			uint32 BlockSizeMult;
			uint32 NumPasses;
		};
		//#include <BitmEncoder.h>
		//
		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties,
		  #ifndef _7ZIP_ST
			public ICompressSetCoderMt,
		  #endif
			public CMyUnknownImp {
			uint32 m_NumThreadsPrev;
		public:
			/*template <class TOutByte>*/ class CBitmEncoder {
				uint   _bitPos;
				Byte   _curByte;
				uint8  Reserve[3]; // @alignment
				//TOutByte _stream;
				COutBuffer _stream;
			public:
				bool   FASTCALL Create(uint32 bufferSize);
				void   FASTCALL SetStream(ISequentialOutStream * outStream);
				uint64 GetProcessedSize() const;
				void   Init();
				HRESULT Flush();
				void WriteBits(uint32 value, uint numBits);
			};
			CInBuffer m_InStream;
			Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
			CBitmEncoder /*<COutBuffer>*/ m_OutStream;
			CEncProps _props;
			CBZip2CombinedCrc CombinedCrc;

		  #ifndef _7ZIP_ST
			CThreadInfo * ThreadsInfo;
			NWindows::NSynchronization::CManualResetEvent CanProcessEvent;
			NWindows::NSynchronization::CCriticalSection CS;
			uint32 NumThreads;
			uint32 NextBlockIndex;
			bool   MtMode;
			bool   CloseThreads;
			bool   StreamWasFinished;
			uint8  Reserve; // @alignment
			NWindows::NSynchronization::CManualResetEvent CanStartWaitingEvent;
			HRESULT Result;
			ICompressProgressInfo * Progress;
		  #else
			CThreadInfo ThreadsInfo;
		  #endif

			uint32 ReadRleBlock(Byte * buf);
			void WriteBytes(const Byte * data, uint32 sizeInBits, Byte lastByte);
			void WriteBits(uint32 value, unsigned numBits);
			void WriteByte(Byte b);
			// void WriteBit(Byte v);
			void WriteCrc(uint32 v);
		  #ifndef _7ZIP_ST
			HRESULT Create();
			void Free();
		  #endif
		public:
			CEncoder();
		  #ifndef _7ZIP_ST
			~CEncoder();
		  #endif
			HRESULT Flush() { return m_OutStream.Flush(); }
			MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
		  #ifndef _7ZIP_ST
			MY_QUERYINTERFACE_ENTRY(ICompressSetCoderMt)
		  #endif
			MY_QUERYINTERFACE_ENTRY(ICompressSetCoderProperties)
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE

			HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);

		  #ifndef _7ZIP_ST
			STDMETHOD(SetNumberOfThreads) (uint32 numThreads);
		  #endif
		};
	}
	namespace NBcj {
		class CCoder : public ICompressFilter, public CMyUnknownImp {
			uint32 _bufferPos;
			uint32 _prevMask;
			int _encode;
		public:
			MY_UNKNOWN_IMP1(ICompressFilter);
			INTERFACE_ICompressFilter(; ) CCoder(int encode) : _bufferPos(0), _encode(encode) { x86_Convert_Init(_prevMask); }
		};
	}
	namespace NBcj2 {
		class CBaseCoder {
		protected:
			Byte * _bufs[BCJ2_NUM_STREAMS + 1];
			uint32 _bufsCurSizes[BCJ2_NUM_STREAMS + 1];
			uint32 _bufsNewSizes[BCJ2_NUM_STREAMS + 1];
			HRESULT Alloc(bool allocForOrig = true);
		public:
			CBaseCoder();
			~CBaseCoder();
		};

		#ifndef EXTRACT_ONLY

		class CEncoder : public ICompressCoder2, public ICompressSetCoderProperties, public ICompressSetBufSize,
			public CMyUnknownImp, public CBaseCoder {
			uint32 _relatLim;
			HRESULT CodeReal(ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
				ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams,
				ICompressProgressInfo * progress);
		public:
			MY_UNKNOWN_IMP3(ICompressCoder2, ICompressSetCoderProperties, ICompressSetBufSize)
			STDMETHOD(Code) (ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
				ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams,
				ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size);
			STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size);
			CEncoder();
			~CEncoder();
		};

		#endif

		class CDecoder : public ICompressCoder2, public ICompressSetFinishMode, public ICompressGetInStreamProcessedSize2,
			public ICompressSetInStream2, public ISequentialInStream, public ICompressSetOutStreamSize,
			public ICompressSetBufSize, public CMyUnknownImp, public CBaseCoder {
			uint   _extraReadSizes[BCJ2_NUM_STREAMS];
			uint64 _inStreamsProcessed[BCJ2_NUM_STREAMS];
			HRESULT _readRes[BCJ2_NUM_STREAMS];
			CMyComPtr <ISequentialInStream> _inStreams[BCJ2_NUM_STREAMS];
			bool   _finishMode;
			bool   _outSizeDefined;
			uint8  Reserve[2]; // @alignment
			uint64 _outSize;
			uint64 _outSize_Processed;
			CBcj2Dec dec;

			void InitCommon();
			// HRESULT ReadSpec();
		public:
			MY_UNKNOWN_IMP7(ICompressCoder2, ICompressSetFinishMode, ICompressGetInStreamProcessedSize2,
				ICompressSetInStream2, ISequentialInStream, ICompressSetOutStreamSize, ICompressSetBufSize);
			STDMETHOD(Code) (ISequentialInStream * const * inStreams, const uint64 * const * inSizes, uint32 numInStreams,
				ISequentialOutStream * const * outStreams, const uint64 * const * outSizes, uint32 numOutStreams,
				ICompressProgressInfo *progress);
			STDMETHOD(SetFinishMode) (uint32 finishMode);
			STDMETHOD(GetInStreamProcessedSize2) (uint32 streamIndex, uint64 *value);
			STDMETHOD(SetInStream2) (uint32 streamIndex, ISequentialInStream *inStream);
			STDMETHOD(ReleaseInStream2) (uint32 streamIndex);
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
			STDMETHOD(SetInBufSize) (uint32 streamIndex, uint32 size);
			STDMETHOD(SetOutBufSize) (uint32 streamIndex, uint32 size);
			CDecoder();
		};
	}
	namespace NPpmd {
		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public ICompressGetInStreamProcessedSize,
		  #ifndef NO_READ_FROM_CODER
			public ICompressSetInStream, public ICompressSetOutStreamSize, public ISequentialInStream,
		  #endif
			public CMyUnknownImp {
			Byte * _outBuf;
			CPpmd7z_RangeDec _rangeDec;
			CByteInBufWrap _inStream;
			CPpmd7 _ppmd;
			Byte _order;
			bool _outSizeDefined;
			int _status;
			uint64 _outSize;
			uint64 _processedSize;

			HRESULT CodeSpec(Byte * memStream, uint32 size);
		public:
		  #ifndef NO_READ_FROM_CODER
			CMyComPtr<ISequentialInStream> InSeqStream;
		  #endif

			MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
			MY_QUERYINTERFACE_ENTRY(ICompressSetDecoderProperties2)
			// MY_QUERYINTERFACE_ENTRY(ICompressSetFinishMode)
			MY_QUERYINTERFACE_ENTRY(ICompressGetInStreamProcessedSize)
		  #ifndef NO_READ_FROM_CODER
			MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
			MY_QUERYINTERFACE_ENTRY(ICompressSetOutStreamSize)
			MY_QUERYINTERFACE_ENTRY(ISequentialInStream)
		  #endif
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
			STDMETHOD(GetInStreamProcessedSize) (uint64 *value);
			STDMETHOD(SetOutStreamSize) (const uint64 *outSize);
		  #ifndef NO_READ_FROM_CODER
			STDMETHOD(SetInStream) (ISequentialInStream *inStream);
			STDMETHOD(ReleaseInStream) ();
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		  #endif
			CDecoder();
			/*CDecoder() : _outBuf(NULL), _outSizeDefined(false)
			{
				Ppmd7z_RangeDec_CreateVTable(&_rangeDec);
				_rangeDec.Stream = &_inStream.vt;
				Ppmd7_Construct(&_ppmd);
			}*/
			~CDecoder();
		};

		struct CEncProps {
			CEncProps();
			void   Normalize(int level);
			uint32 MemSize;
			uint32 ReduceSize;
			int    Order;
		};

		class CEncoder : public ICompressCoder, public ICompressSetCoderProperties, public ICompressWriteCoderProperties, public CMyUnknownImp {
			Byte * _inBuf;
			CByteOutBufWrap _outStream;
			CPpmd7z_RangeEnc _rangeEnc;
			CPpmd7 _ppmd;
			CEncProps _props;
		public:
			MY_UNKNOWN_IMP3(ICompressCoder, ICompressSetCoderProperties, ICompressWriteCoderProperties)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
			STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);
			CEncoder();
			~CEncoder();
		};
	}
}
//
//#include <InStreamWithCRC.h>
class CSequentialInStreamWithCRC : public ISequentialInStream, public CMyUnknownImp {
public:
	MY_UNKNOWN_IMP
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
private:
	CMyComPtr <ISequentialInStream> _stream;
	uint64 _size;
	uint32 _crc;
	bool   _wasFinished;
public:
	void   SetStream(ISequentialInStream * stream);
	void   Init();
	void   ReleaseStream();
	uint32 GetCRC() const;
	uint64 GetSize() const { return _size; }
	bool   WasFinished() const { return _wasFinished; }
};

class CInStreamWithCRC : public IInStream, public CMyUnknownImp {
public:
	MY_UNKNOWN_IMP1(IInStream)

	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
private:
	CMyComPtr<IInStream> _stream;
	uint64 _size;
	uint32 _crc;
	// bool _wasFinished;
public:
	void SetStream(IInStream * stream) { _stream = stream; }
	void Init()
	{
		_size = 0;
		// _wasFinished = false;
		_crc = CRC_INIT_VAL;
	}
	void ReleaseStream() { _stream.Release(); }
	uint32 GetCRC() const { return CRC_GET_DIGEST(_crc); }
	uint64 GetSize() const { return _size; }
	// bool WasFinished() const { return _wasFinished; }
};
//
//#include <OutStreamWithCRC.h>
class COutStreamWithCRC : public ISequentialOutStream, public CMyUnknownImp {
	CMyComPtr<ISequentialOutStream> _stream;
	uint64 _size;
	uint32 _crc;
	bool _calculate;
public:
	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	void SetStream(ISequentialOutStream * stream);
	void ReleaseStream();
	void Init(bool calculate = true);
	void EnableCalc(bool calculate);
	void InitCRC();
	uint64 GetSize() const { return _size; }
	uint32 GetCRC() const;
};
//
//#include <RegisterCodec.h>
typedef void * (*CreateCodecP)();

struct CCodecInfo {
	CreateCodecP CreateDecoder;
	CreateCodecP CreateEncoder;
	CMethodId Id;
	const char * Name;
	uint32 NumStreams;
	bool IsFilter;
};

void RegisterCodec(const CCodecInfo * codecInfo) throw();

#define REGISTER_CODEC_CREATE_2(name, cls, i) static void * name() { return (void *)(i*)(new cls); }
#define REGISTER_CODEC_CREATE(name, cls) REGISTER_CODEC_CREATE_2(name, cls, ICompressCoder)
#define REGISTER_CODEC_NAME(x) CRegisterCodec ## x
#define REGISTER_CODEC_VAR static const CCodecInfo g_CodecInfo =
#define REGISTER_CODEC(x) struct REGISTER_CODEC_NAME (x) { REGISTER_CODEC_NAME(x) () { RegisterCodec(&g_CodecInfo); }}; \
	static REGISTER_CODEC_NAME(x) g_RegisterCodec;

#define REGISTER_CODECS_NAME(x) CRegisterCodecs ## x
#define REGISTER_CODECS_VAR static const CCodecInfo g_CodecsInfo[] =

#define REGISTER_CODECS(x) struct REGISTER_CODECS_NAME (x) { \
	REGISTER_CODECS_NAME(x) () { for(uint i = 0; i < SIZEOFARRAY(g_CodecsInfo); i++) RegisterCodec(&g_CodecsInfo[i]); }}; \
	static REGISTER_CODECS_NAME(x) g_RegisterCodecs;

#define REGISTER_CODEC_2(x, crDec, crEnc, id, name) REGISTER_CODEC_VAR { crDec, crEnc, id, name, 1, false }; REGISTER_CODEC(x)

#ifdef EXTRACT_ONLY
  #define REGISTER_CODEC_E(x, clsDec, clsEnc, id, name)	REGISTER_CODEC_CREATE(CreateDec, clsDec) REGISTER_CODEC_2(x, CreateDec, NULL, id, name)
#else
  #define REGISTER_CODEC_E(x, clsDec, clsEnc, id, name)	\
	REGISTER_CODEC_CREATE(CreateDec, clsDec) REGISTER_CODEC_CREATE(CreateEnc, clsEnc) REGISTER_CODEC_2(x, CreateDec, CreateEnc, id, name)
#endif

#define REGISTER_FILTER_CREATE(name, cls) REGISTER_CODEC_CREATE_2(name, cls, ICompressFilter)
#define REGISTER_FILTER_ITEM(crDec, crEnc, id, name) { crDec, crEnc, id, name, 1, true }
#define REGISTER_FILTER(x, crDec, crEnc, id, name) REGISTER_CODEC_VAR REGISTER_FILTER_ITEM(crDec, crEnc, id, name); REGISTER_CODEC(x)
#ifdef EXTRACT_ONLY
  #define REGISTER_FILTER_E(x, clsDec, clsEnc, id, name) REGISTER_FILTER_CREATE(CreateDec, clsDec) REGISTER_FILTER(x, CreateDec, NULL, id, name)
#else
  #define REGISTER_FILTER_E(x, clsDec, clsEnc, id, name) \
	REGISTER_FILTER_CREATE(CreateDec, clsDec) REGISTER_FILTER_CREATE(CreateEnc, clsEnc) REGISTER_FILTER(x, CreateDec, CreateEnc, id, name)
#endif

struct CHasherInfo {
	IHasher * (*CreateHasher)();
	CMethodId Id;
	const char * Name;
	uint32 DigestSize;
};

void RegisterHasher(const CHasherInfo * hasher) throw();

#define REGISTER_HASHER_NAME(x) CRegHasher_ ## x

#define REGISTER_HASHER(cls, id, name, size) \
	STDMETHODIMP_(uint32) cls::GetDigestSize() throw() { return size; } \
	static IHasher * CreateHasherSpec() { return new cls(); } \
	static const CHasherInfo g_HasherInfo = { CreateHasherSpec, id, name, size }; \
	struct REGISTER_HASHER_NAME (cls) {REGISTER_HASHER_NAME(cls) () { RegisterHasher(&g_HasherInfo); }}; \
	static REGISTER_HASHER_NAME(cls) g_RegisterHasher;
//
//#include <RegisterArc.h>
struct CArcInfo {
	bool IsMultiSignature() const { return (Flags & NArcInfoFlags::kMultiSignature) != 0; }

	uint16 Flags;
	Byte Id;
	Byte SignatureSize;
	uint16 SignatureOffset;
	const Byte * Signature;
	const char * Name;
	const char * Ext;
	const char * AddExt;
	Func_CreateInArchive CreateInArchive;
	Func_CreateOutArchive CreateOutArchive;
	Func_IsArc IsArc;
};

void RegisterArc(const CArcInfo * arcInfo) throw();

#define IMP_CreateArcIn_2(c) static IInArchive *CreateArc() { return new c; }
#define IMP_CreateArcIn IMP_CreateArcIn_2(CHandler())
#ifdef EXTRACT_ONLY
	#define IMP_CreateArcOut
	#define CreateArcOut NULL
#else
	#define IMP_CreateArcOut static IOutArchive *CreateArcOut() { return new CHandler(); }
#endif

#define REGISTER_ARC_V(n, e, ae, id, sigSize, sig, offs, flags, crIn, crOut, isArc) \
	static const CArcInfo g_ArcInfo = { flags, id, sigSize, offs, sig, n, e, ae, crIn, crOut, isArc }; \

#define REGISTER_ARC_R(n, e, ae, id, sigSize, sig, offs, flags, crIn, crOut, isArc) \
	REGISTER_ARC_V(n, e, ae, id, sigSize, sig, offs, flags, crIn, crOut, isArc) \
	struct CRegisterArc { CRegisterArc() { RegisterArc(&g_ArcInfo); }}; \
	static CRegisterArc g_RegisterArc;

#define REGISTER_ARC_I_CLS(cls, n, e, ae, id, sig, offs, flags, isArc) \
	IMP_CreateArcIn_2(cls) REGISTER_ARC_R(n, e, ae, id, SIZEOFARRAY(sig), sig, offs, flags, CreateArc, NULL, isArc)

#define REGISTER_ARC_I_CLS_NO_SIG(cls, n, e, ae, id, offs, flags, isArc) \
	IMP_CreateArcIn_2(cls) REGISTER_ARC_R(n, e, ae, id, 0, NULL, offs, flags, CreateArc, NULL, isArc)

#define REGISTER_ARC_I(n, e, ae, id, sig, offs, flags, isArc)   REGISTER_ARC_I_CLS(CHandler(), n, e, ae, id, sig, offs, flags, isArc)
#define REGISTER_ARC_I_NO_SIG(n, e, ae, id, offs, flags, isArc)	REGISTER_ARC_I_CLS_NO_SIG(CHandler(), n, e, ae, id, offs, flags, isArc)

#define REGISTER_ARC_IO(n, e, ae, id, sig, offs, flags, isArc) \
	IMP_CreateArcIn	IMP_CreateArcOut REGISTER_ARC_R(n, e, ae, id, SIZEOFARRAY(sig), sig, offs, flags, CreateArc, CreateArcOut, isArc)

#define REGISTER_ARC_IO_DECREMENT_SIG(n, e, ae, id, sig, offs, flags, isArc) \
	IMP_CreateArcIn	IMP_CreateArcOut \
	REGISTER_ARC_V(n, e, ae, id, SIZEOFARRAY(sig), sig, offs, flags, CreateArc, CreateArcOut, isArc)	\
	struct CRegisterArcDecSig { CRegisterArcDecSig() { sig[0]--; RegisterArc(&g_ArcInfo); }}; \
	static CRegisterArcDecSig g_RegisterArc;
//
//#include <LoadCodecs.h>
/*
   Client application uses LoadCodecs.* to load plugins to
   CCodecs object, that contains 3 lists of plugins:
   1) Formats - internal and external archive handlers
   2) Codecs  - external codecs
   3) Hashers - external hashers

   EXTERNAL_CODECS
   ---------------

   if EXTERNAL_CODECS is defined, then the code tries to load external
   plugins from DLL files (shared libraries).

   There are two types of executables in 7-Zip:

   1) Executable that uses external plugins must be compiled
     with EXTERNAL_CODECS defined:
       - 7z.exe, 7zG.exe, 7zFM.exe

     Note: EXTERNAL_CODECS is used also in CPP/7zip/Common/CreateCoder.h
           that code is used in plugin module (7z.dll).

   2) Standalone modules are compiled without EXTERNAL_CODECS:
    - SFX modules: 7z.sfx, 7zCon.sfx
    - standalone versions of console 7-Zip: 7za.exe, 7zr.exe

   if EXTERNAL_CODECS is defined, CCodecs class implements interfaces:
    - ICompressCodecsInfo : for Codecs
    - IHashers            : for Hashers

   The client application can send CCodecs object to each plugin module.
   And plugin module can use ICompressCodecsInfo or IHashers interface to access
   another plugins.

   There are 2 ways to send (ICompressCodecsInfo * compressCodecsInfo) to plugin
    1) for old versions:
        a) request ISetCompressCodecsInfo from created archive handler.
        b) call ISetCompressCodecsInfo::SetCompressCodecsInfo(compressCodecsInfo)
    2) for new versions:
        a) request "SetCodecs" function from DLL file
        b) call SetCodecs(compressCodecsInfo) function from DLL file
 */
#ifdef EXTERNAL_CODECS
struct CDllCodecInfo {
	unsigned LibIndex;
	uint32 CodecIndex;
	bool EncoderIsAssigned;
	bool DecoderIsAssigned;
	CLSID Encoder;
	CLSID Decoder;
};

struct CDllHasherInfo {
	unsigned LibIndex;
	uint32 HasherIndex;
};
#endif

struct CArcExtInfo {
	CArcExtInfo() 
	{
	}
	CArcExtInfo(const UString &ext) : Ext(ext) 
	{
	}
	CArcExtInfo(const UString &ext, const UString &addExt) : Ext(ext), AddExt(addExt) 
	{
	}
	UString Ext;
	UString AddExt;
};

struct CArcInfoEx {
	CArcInfoEx();
	bool   Flags_KeepName() const;
	bool   Flags_FindSignature() const;
	bool   Flags_AltStreams() const;
	bool   Flags_NtSecure() const;
	bool   Flags_SymLinks() const;
	bool   Flags_HardLinks() const;
	bool   Flags_UseGlobalOffset() const;
	bool   Flags_StartOpen() const;
	bool   Flags_BackwardOpen() const;
	bool   Flags_PreArc() const;
	bool   Flags_PureStartOpen() const;
	UString GetMainExt() const;
	int    FindExtension(const UString & ext) const;
	/*UString GetAllExtensions() const
	{
		UString s;
		for(int i = 0; i < Exts.Size(); i++) {
			if(i > 0)
				s += ' ';
			s += Exts[i].Ext;
		}
		return s;
	}*/
	void   AddExts(const UString &ext, const UString &addExt);
	bool   IsSplit() const { return sstreqi_ascii(Name, "Split"); }
	// bool IsRar() const { return sstreqi_ascii(Name, "Rar"); }

	uint32 Flags;
	Func_CreateInArchive CreateInArchive;
	Func_IsArc IsArcFunc;
	UString Name;
	CObjectVector <CArcExtInfo> Exts;
#ifndef _SFX
	Func_CreateOutArchive CreateOutArchive;
	bool   UpdateEnabled;
	bool   NewInterface;
	// uint32 Version;
	uint32 SignatureOffset;
	CObjectVector <CByteBuffer> Signatures;
    #ifdef NEW_FOLDER_INTERFACE
	UStringVector AssociateExts;
    #endif
#endif
#ifdef EXTERNAL_CODECS
	int LibIndex;
	uint32 FormatIndex;
	CLSID ClassID;
#endif
};

#ifdef NEW_FOLDER_INTERFACE
struct CCodecIcons {
	void LoadIcons(HMODULE m);
	bool FindIconIndex(const UString &ext, int &iconIndex) const;

	struct CIconPair {
		UString Ext;
		int IconIndex;
	};
	CObjectVector<CIconPair> IconPairs;
};
#endif

#ifdef EXTERNAL_CODECS
struct CCodecLib
  #ifdef NEW_FOLDER_INTERFACE
	: public CCodecIcons
  #endif
{
	NWindows::NDLL::CLibrary Lib;
	FString Path;
	Func_CreateObject CreateObject;
	Func_GetMethodProperty GetMethodProperty;
	Func_CreateDecoder CreateDecoder;
	Func_CreateEncoder CreateEncoder;
	Func_SetCodecs SetCodecs;
	CMyComPtr<IHashers> ComHashers;
  #ifdef NEW_FOLDER_INTERFACE
	void LoadIcons() 
	{
		CCodecIcons::LoadIcons((HMODULE)Lib);
	}
  #endif
	CCodecLib() : CreateObject(NULL), GetMethodProperty(NULL), CreateDecoder(NULL), CreateEncoder(NULL), SetCodecs(NULL)
	{
	}
};
#endif

class CCodecs :
  #ifdef EXTERNAL_CODECS
	public ICompressCodecsInfo, public IHashers,
  #else
	public IUnknown,
  #endif
	public CMyUnknownImp
{
	CLASS_NO_COPY(CCodecs);
public:
#ifdef EXTERNAL_CODECS
	CObjectVector<CCodecLib> Libs;
	FString MainDll_ErrorPath;
	void CloseLibs();
	class CReleaser {
		CLASS_NO_COPY(CReleaser);
		/* CCodecsReleaser object releases CCodecs links.
		     1) CCodecs is COM object that is deleted when all links to that object will be released/
		     2) CCodecs::Libs[i] can hold (ICompressCodecsInfo *) link to CCodecs object itself.
		   To break that reference loop, we must close all CCodecs::Libs in CCodecsReleaser desttructor. */
		CCodecs * _codecs;
public:
		CReleaser() : _codecs(NULL) 
		{
		}
		void Set(CCodecs * codecs) 
		{
			_codecs = codecs;
		}
		~CReleaser() 
		{
			if(_codecs) 
				_codecs->CloseLibs();
		}
	};
	bool NeedSetLibCodecs; // = false, if we don't need to set codecs for archive handler via ISetCompressCodecsInfo
	HRESULT LoadCodecs();
	HRESULT LoadFormats();
	HRESULT LoadDll(const FString &path, bool needCheckDll, bool * loadedOK = NULL);
	HRESULT LoadDllsFromFolder(const FString &folderPrefix);
	HRESULT CreateArchiveHandler(const CArcInfoEx &ai, bool outHandler, void ** archive) const
	{
		return Libs[ai.LibIndex].CreateObject(&ai.ClassID, outHandler ? &IID_IOutArchive : &IID_IInArchive, (void **)archive);
	}
  #endif
#ifdef NEW_FOLDER_INTERFACE
	CCodecIcons InternalIcons;
#endif
	CObjectVector<CArcInfoEx> Formats;
#ifdef EXTERNAL_CODECS
	CRecordVector<CDllCodecInfo> Codecs;
	CRecordVector<CDllHasherInfo> Hashers;
#endif
	bool CaseSensitiveChange;
	bool CaseSensitive;
	CCodecs();
	~CCodecs();
	const wchar_t * FASTCALL GetFormatNamePtr(int formatIndex) const;
	HRESULT Load();
#ifndef _SFX
	int FindFormatForArchiveName(const UString &arcPath) const;
	int FindFormatForExtension(const UString &ext) const;
	int FindFormatForArchiveType(const UString &arcType) const;
	bool FindFormatForArchiveType(const UString &arcType, CIntVector &formatIndices) const;
	HRESULT CreateOutArchive(unsigned formatIndex, CMyComPtr<IOutArchive> &archive) const;
	int FindOutFormatFromName(const UString &name) const;
#endif
#ifdef EXTERNAL_CODECS
	MY_UNKNOWN_IMP2(ICompressCodecsInfo, IHashers)
	STDMETHOD(GetNumMethods) (uint32 *numMethods);
	STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value);
	STDMETHOD(CreateDecoder) (uint32 index, const GUID *iid, void ** coder);
	STDMETHOD(CreateEncoder) (uint32 index, const GUID *iid, void ** coder);
	STDMETHOD_(uint32, GetNumHashers) ();
	STDMETHOD(GetHasherProp) (uint32 index, PROPID propID, PROPVARIANT *value);
	STDMETHOD(CreateHasher) (uint32 index, IHasher **hasher);

	int GetCodec_LibIndex(uint32 index) const;
	bool GetCodec_DecoderIsAssigned(uint32 index) const;
	bool GetCodec_EncoderIsAssigned(uint32 index) const;
	uint32 GetCodec_NumStreams(uint32 index);
	HRESULT GetCodec_Id(uint32 index, uint64 &id);
	AString GetCodec_Name(uint32 index);
	int GetHasherLibIndex(uint32 index);
	uint64 GetHasherId(uint32 index);
	AString GetHasherName(uint32 index);
	uint32 GetHasherDigestSize(uint32 index);
#else
	MY_UNKNOWN_IMP
#endif // EXTERNAL_CODECS
	HRESULT CreateInArchive(unsigned formatIndex, CMyComPtr<IInArchive> &archive) const;
};

#ifdef EXTERNAL_CODECS
	#define CREATE_CODECS_OBJECT \
		CCodecs *codecs = new CCodecs; \
		CExternalCodecs __externalCodecs; \
		__externalCodecs.GetCodecs = codecs; \
		__externalCodecs.GetHashers = codecs; \
		CCodecs::CReleaser codecsReleaser; \
		codecsReleaser.Set(codecs);
#else
	#define CREATE_CODECS_OBJECT CCodecs *codecs = new CCodecs; CMyComPtr<IUnknown> __codecsRef = codecs;
#endif
//
//#include <CoderMixer2.h>
#ifdef _7ZIP_ST
	#define USE_MIXER_ST
#else
	#define USE_MIXER_MT
	#ifndef _SFX
		#define USE_MIXER_ST
	#endif
#endif
//#ifdef USE_MIXER_MT
	//#include <StreamBinder.h>
	//#include <VirtThread.h>
//#endif
#ifdef USE_MIXER_ST
	class CSequentialInStreamCalcSize;
	class COutStreamCalcSize;
#endif

namespace NCoderMixer2 {
	struct CBond {
		uint32 PackIndex;
		uint32 UnpackIndex;
		uint32 Get_InIndex(bool encodeMode) const { return encodeMode ? UnpackIndex : PackIndex; }
		uint32 Get_OutIndex(bool encodeMode) const { return encodeMode ? PackIndex : UnpackIndex; }
	};

	struct CCoderStreamsInfo {
		uint32 NumStreams;
	};

	struct CBindInfo {
		CRecordVector<CCoderStreamsInfo> Coders;
		CRecordVector<CBond> Bonds;
		CRecordVector<uint32> PackStreams;
		uint   UnpackCoder;

		uint   GetNum_Bonds_and_PackStreams() const;
		int FindBond_for_PackStream(uint32 packStream) const;
		int FindBond_for_UnpackStream(uint32 unpackStream) const;
		bool SetUnpackCoder();
		bool IsStream_in_PackStreams(uint32 streamIndex) const;
		int FindStream_in_PackStreams(uint32 streamIndex) const;
		// that function is used before Maps is calculated
		uint32 GetStream_for_Coder(uint32 coderIndex) const;

		// ---------- Maps Section ----------
		CRecordVector<uint32> Coder_to_Stream;
		CRecordVector<uint32> Stream_to_Coder;

		void ClearMaps();
		bool CalcMapsAndCheck();
		// ---------- End of Maps Section ----------

		void Clear();
		void GetCoder_for_Stream(uint32 streamIndex, uint32 &coderIndex, uint32 &coderStreamIndex) const;
	};

	class CCoder {
		CLASS_NO_COPY(CCoder);
	public:
		CMyComPtr<ICompressCoder> Coder;
		CMyComPtr<ICompressCoder2> Coder2;
		uint32 NumStreams;
		uint64 UnpackSize;
		const uint64 * UnpackSizePointer;
		CRecordVector<uint64> PackSizes;
		CRecordVector<const uint64 *> PackSizePointers;
		bool Finish;

		CCoder() : Finish(false) 
		{
		}
		void SetCoderInfo(const uint64 * unpackSize, const uint64 * const * packSizes, bool finish);
		HRESULT CheckDataAfterEnd(bool &dataAfterEnd_Error /* , bool &InternalPackSizeError */) const;
		IUnknown * GetUnknown() const { return Coder ? (IUnknown*)Coder : (IUnknown*)Coder2; }
		HRESULT QueryInterface(REFGUID iid, void** pp) const { return GetUnknown()->QueryInterface(iid, pp); }
	};

	class CMixer {
	private:
		bool Is_PackSize_Correct_for_Stream(uint32 streamIndex);
	protected:
		int FindBond_for_Stream(bool forInputStream, uint32 streamIndex) const;

		CBindInfo _bi;
		CBoolVector IsFilter_Vector;
		CBoolVector IsExternal_Vector;
		bool   EncodeMode;
	public:
		uint   MainCoderIndex;
		// bool InternalPackSizeError;
		CMixer(bool encodeMode);
		/*
		   Sequence of calling:

			SetBindInfo();
			for each coder
			  AddCoder();
			SelectMainCoder();

			for each file {
			  ReInit()
			  for each coder
				SetCoderInfo();
			  Code();
			}
		 */
		virtual HRESULT SetBindInfo(const CBindInfo & bindInfo);
		virtual void AddCoder(const CCreatedCoder &cod) = 0;
		virtual CCoder &GetCoder(uint index) = 0;
		virtual void SelectMainCoder(bool useFirst) = 0;
		virtual void ReInit() = 0;
		virtual void SetCoderInfo(unsigned coderIndex, const uint64 * unpackSize, const uint64 * const * packSizes, bool finish) = 0;
		virtual HRESULT Code(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams,
			ICompressProgressInfo * progress, bool &dataAfterEnd_Error) = 0;
		virtual uint64 GetBondStreamSize(unsigned bondIndex) const = 0;

		bool Is_UnpackSize_Correct_for_Coder(uint32 coderIndex);
		bool Is_PackSize_Correct_for_Coder(uint32 coderIndex);
		bool IsThere_ExternalCoder_in_PackTree(uint32 coderIndex);
	};

	#ifdef USE_MIXER_ST
		class CMixerST : public IUnknown, public CMixer, public CMyUnknownImp {
			HRESULT GetInStream2(ISequentialInStream * const * inStreams, /* const uint64 * const *inSizes, */ uint32 outStreamIndex, ISequentialInStream ** inStreamRes);
			HRESULT GetInStream(ISequentialInStream * const * inStreams, /* const uint64 * const *inSizes, */ uint32 inStreamIndex, ISequentialInStream ** inStreamRes);
			HRESULT GetOutStream(ISequentialOutStream * const * outStreams, /* const uint64 * const *outSizes, */ uint32 outStreamIndex, ISequentialOutStream ** outStreamRes);
			HRESULT FinishStream(uint32 streamIndex);
			HRESULT FinishCoder(uint32 coderIndex);
		public:
			struct CCoderST : public CCoder {
				CCoderST();
				bool CanRead;
				bool CanWrite;
			};
			struct CStBinderStream {
				CStBinderStream();
				CSequentialInStreamCalcSize * InStreamSpec;
				COutStreamCalcSize * OutStreamSpec;
				CMyComPtr<IUnknown> StreamRef;
			};
			CObjectVector <CCoderST> _coders;
			CObjectVector <CStBinderStream> _binderStreams;
			MY_UNKNOWN_IMP CMixerST(bool encodeMode);
			~CMixerST();
			virtual void AddCoder(const CCreatedCoder &cod);
			virtual CCoder &GetCoder(uint index);
			virtual void SelectMainCoder(bool useFirst);
			virtual void ReInit();
			virtual void SetCoderInfo(unsigned coderIndex, const uint64 * unpackSize, const uint64 * const * packSizes, bool finish)
			{
				_coders[coderIndex].SetCoderInfo(unpackSize, packSizes, finish);
			}
			virtual HRESULT Code(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams,
				ICompressProgressInfo * progress, bool &dataAfterEnd_Error);
			virtual uint64 GetBondStreamSize(unsigned bondIndex) const;
			HRESULT GetMainUnpackStream(ISequentialInStream * const * inStreams, ISequentialInStream ** inStreamRes);
		};
	#endif
	#ifdef USE_MIXER_MT
		class CCoderMT : public CCoder, public CVirtThread {
			CLASS_NO_COPY(CCoderMT)
			CRecordVector<ISequentialInStream*> InStreamPointers;
			CRecordVector<ISequentialOutStream*> OutStreamPointers;
		private:
			void Execute();
		public:
			bool EncodeMode;
			HRESULT Result;
			CObjectVector < CMyComPtr<ISequentialInStream> > InStreams;
			CObjectVector < CMyComPtr<ISequentialOutStream> > OutStreams;
			void Release();
			class CReleaser {
				CLASS_NO_COPY(CReleaser)
				CCoderMT &_c;
			public:
				CReleaser(CCoderMT &c) : _c(c) 
				{
				}
				~CReleaser() 
				{
					_c.Release();
				}
			};
			CCoderMT();
			~CCoderMT();
			void Code(ICompressProgressInfo * progress);
		};

		class CMixerMT : public IUnknown, public CMixer, public CMyUnknownImp {
			CObjectVector <CStreamBinder> _streamBinders;
			HRESULT Init(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams);
			HRESULT ReturnIfError(HRESULT code);
		public:
			CObjectVector<CCoderMT> _coders;
			MY_UNKNOWN_IMP
			virtual HRESULT SetBindInfo(const CBindInfo &bindInfo);
			virtual void AddCoder(const CCreatedCoder &cod);
			virtual CCoder &GetCoder(uint index);
			virtual void SelectMainCoder(bool useFirst);
			virtual void ReInit();
			virtual void SetCoderInfo(unsigned coderIndex, const uint64 * unpackSize, const uint64 * const * packSizes, bool finish)
			{
				_coders[coderIndex].SetCoderInfo(unpackSize, packSizes, finish);
			}
			virtual HRESULT Code(ISequentialInStream * const * inStreams, ISequentialOutStream * const * outStreams,
				ICompressProgressInfo * progress, bool &dataAfterEnd_Error);
			virtual uint64 GetBondStreamSize(unsigned bondIndex) const;
			CMixerMT(bool encodeMode) : CMixer(encodeMode) 
			{
			}
		};
	#endif
}
//
//#include <DirItem.h>
#define INTERFACE_IDirItemsCallback(x) \
	virtual HRESULT ScanError(const FString &path, DWORD systemError) x; \
	virtual HRESULT ScanProgress(const CDirItemsStat &st, const FString &path, bool isDir) x; \

struct IDirItemsCallback {
	INTERFACE_IDirItemsCallback( = 0)
};
//
//#include <EnumDirItems.h>
void AddDirFileInfo(int phyParent, int logParent, int secureIndex, const NWindows::NFile::NFind::CFileInfo &fi, CObjectVector<CDirItem> &dirItems);
HRESULT EnumerateItems(const NWildcard::CCensor &censor, NWildcard::ECensorPathMode pathMode, const UString &addPathPrefix, CDirItems &dirItems);
//
//#include <HashCalc.h>
const uint k_HashCalc_DigestSize_Max = 64;
const uint k_HashCalc_NumGroups = 4;

struct CHasherState {
	CMyComPtr <IHasher> Hasher;
	AString Name;
	uint32 DigestSize;
	Byte Digests[k_HashCalc_NumGroups][k_HashCalc_DigestSize_Max];
};

struct IHashCalc {
	virtual void InitForNewFile() = 0;
	virtual void Update(const void * data, uint32 size) = 0;
	virtual void SetSize(uint64 size) = 0;
	virtual void Final(bool isDir, bool isAltStream, const UString &path) = 0;
};

struct CHashBundle : public IHashCalc {
	HRESULT SetMethods(DECL_EXTERNAL_CODECS_LOC_VARS const UStringVector &methods);
	void Init();
	void InitForNewFile();
	void Update(const void * data, uint32 size);
	void SetSize(uint64 size);
	void Final(bool isDir, bool isAltStream, const UString &path);

	CObjectVector<CHasherState> Hashers;
	uint64 NumDirs;
	uint64 NumFiles;
	uint64 NumAltStreams;
	uint64 FilesSize;
	uint64 AltStreamsSize;
	uint64 NumErrors;
	uint64 CurSize;
};

#define INTERFACE_IHashCallbackUI(x) \
	INTERFACE_IDirItemsCallback(x) \
	virtual HRESULT StartScanning() x; \
	virtual HRESULT FinishScanning(const CDirItemsStat &st) x; \
	virtual HRESULT SetNumFiles(uint64 numFiles) x;	\
	virtual HRESULT SetTotal(uint64 size) x; \
	virtual HRESULT SetCompleted(const uint64 *completeValue) x; \
	virtual HRESULT CheckBreak() x;	\
	virtual HRESULT BeforeFirstFile(const CHashBundle &hb) x; \
	virtual HRESULT GetStream(const wchar_t * name, bool isFolder) x; \
	virtual HRESULT OpenFileError(const FString &path, DWORD systemError) x; \
	virtual HRESULT SetOperationResult(uint64 fileSize, const CHashBundle &hb, bool showHash) x; \
	virtual HRESULT AfterLastFile(const CHashBundle &hb) x;	\

struct IHashCallbackUI : public IDirItemsCallback {
	INTERFACE_IHashCallbackUI( = 0)
};

struct CHashOptions {
	CHashOptions() : StdInMode(false), OpenShareForWrite(false), AltStreamsMode(false), PathMode(NWildcard::k_RelatPath) 
	{
	}
	UStringVector Methods;
	bool   OpenShareForWrite;
	bool   StdInMode;
	bool   AltStreamsMode;
	uint8  Reserve[1]; // @alignment
	NWildcard::ECensorPathMode PathMode;
};

HRESULT HashCalc(DECL_EXTERNAL_CODECS_LOC_VARS const NWildcard::CCensor &censor, const CHashOptions &options, AString &errorInfo, IHashCallbackUI * callback);
void AddHashHexToString(char * dest, const Byte * data, uint32 size);
//
//#include <MyAes.h> <WzAes.h> <ZipCrypto.h> <ZipStrong.h> <Rar20Crypto.h> <RarAes.h> <Rar5Aes.h> <7zAes.h>
namespace NCrypto {
	class CAesCbcCoder : public ICompressFilter, public ICryptoProperties, public ICompressSetCoderProperties, public CMyUnknownImp {
		AES_CODE_FUNC _codeFunc;
		uint   _offset;
		uint   _keySize;
		bool   _keyIsSet;
		bool   _encodeMode;
		uint32 _aes[AES_NUM_IVMRK_WORDS + 3];
		Byte   _iv[AES_BLOCK_SIZE];
		bool SetFunctions(uint32 algo);
	public:
		CAesCbcCoder(bool encodeMode, unsigned keySize);
		virtual ~CAesCbcCoder() { }; // we need virtual destructor for derived classes
		MY_UNKNOWN_IMP3(ICompressFilter, ICryptoProperties, ICompressSetCoderProperties)
		INTERFACE_ICompressFilter(; )
		STDMETHOD(SetKey) (const Byte *data, uint32 size);
		STDMETHOD(SetInitVector) (const Byte *data, uint32 size);
		STDMETHOD(SetCoderProperties) (const PROPID *propIDs, const PROPVARIANT *props, uint32 numProps);
	};

	struct CAesCbcDecoder : public CAesCbcCoder {
		CAesCbcDecoder(unsigned keySize = 0) : CAesCbcCoder(false, keySize) 
		{
		}
	};
	namespace N7z {
		const uint kKeySize = 32;
		const uint kSaltSizeMax = 16;
		const uint kIvSizeMax = 16; // AES_BLOCK_SIZE;

		class CBase {
		public:
			class CKeyInfo {
			public:
				CKeyInfo();
				void ClearProps();
				bool IsEqualTo(const CKeyInfo &a) const;
				void CalcKey();

				uint   NumCyclesPower;
				uint   SaltSize;
				Byte   Salt[kSaltSizeMax];
				CByteBuffer Password;
				Byte   Key[kKeySize];
			};
			class CKeyInfoCache {
			public:
				explicit CKeyInfoCache(uint size) : Size(size) 
				{
				}
				bool GetKey(CKeyInfo &key);
				void Add(const CKeyInfo &key);
				void FindAndAdd(const CKeyInfo &key);
			private:
				unsigned Size;
				CObjectVector <CKeyInfo> Keys;
			};
		protected:
			CBase();
			void   PrepareKey();

			CKeyInfo _key;
			Byte   _iv[kIvSizeMax];
			uint   _ivSize;
		private:
			CKeyInfoCache _cachedKeys;
		};

		class CBaseCoder : public ICompressFilter, public ICryptoSetPassword, public CMyUnknownImp, public CBase {
		protected:
			CMyComPtr <ICompressFilter> _aesFilter;
		public:
			INTERFACE_ICompressFilter(; )
			STDMETHOD(CryptoSetPassword) (const Byte *data, uint32 size);
		};

		#ifndef EXTRACT_ONLY
		class CEncoder : public CBaseCoder, public ICompressWriteCoderProperties,
			// public ICryptoResetSalt,
			public ICryptoResetInitVector {
		public:
			MY_UNKNOWN_IMP4(ICompressFilter, ICryptoSetPassword, ICompressWriteCoderProperties, /*ICryptoResetSalt,*/ ICryptoResetInitVector)
			STDMETHOD(WriteCoderProperties) (ISequentialOutStream *outStream);
			// STDMETHOD(ResetSalt)();
			STDMETHOD(ResetInitVector) ();
			CEncoder();
		};
		#endif

		class CDecoder : public CBaseCoder, public ICompressSetDecoderProperties2 {
		public:
			MY_UNKNOWN_IMP3(ICompressFilter, ICryptoSetPassword, ICompressSetDecoderProperties2)
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
			CDecoder();
		};
	}
	namespace NWzAes {
		// 
		// This code implements Brian Gladman's scheme
		// specified in "A Password Based File Encryption Utility":
		// - AES encryption (128,192,256-bit) in Counter (CTR) mode.
		// - HMAC-SHA1 authentication for encrypted data (10 bytes)
		// - Keys are derived by PPKDF2(RFC2898)-HMAC-SHA1 from ASCII password and Salt (saltSize = aesKeySize / 2).
		// - 2 bytes contain Password Verifier's Code
		// 
		/* ICompressFilter::Init() does nothing for this filter.

		   Call to init:
			Encoder:
			  CryptoSetPassword();
			  WriteHeader();
			Decoder:
			  [CryptoSetPassword();]
			  ReadHeader();
			  [CryptoSetPassword();] Init_and_CheckPassword();
			  [CryptoSetPassword();] Init_and_CheckPassword();
		 */

		const uint32 kPasswordSizeMax = 99; // 128;

		const uint kSaltSizeMax = 16;
		const uint kPwdVerifSize = 2;
		const uint kMacSize = 10;

		enum EKeySizeMode {
			kKeySizeMode_AES128 = 1,
			kKeySizeMode_AES192 = 2,
			kKeySizeMode_AES256 = 3
		};

		struct CAesCtr2 {
			CAesCtr2();
			uint   pos;
			uint   offset;
			uint32 aes[4 + AES_NUM_IVMRK_WORDS + 3];
		};

		void AesCtr2_Init(CAesCtr2 * p);
		void AesCtr2_Code(CAesCtr2 * p, Byte * data, SizeT size);

		class CBaseCoder : public ICompressFilter, public ICryptoSetPassword, public CMyUnknownImp {
		protected:
			struct CKeyInfo {
				CKeyInfo() : KeySizeMode(kKeySizeMode_AES256) 
				{
				}
				uint   GetKeySize()  const { return (8 * KeySizeMode + 8); }
				uint   GetSaltSize() const { return (4 * KeySizeMode + 4); }
				uint   GetNumSaltWords() const { return (KeySizeMode + 1); }

				EKeySizeMode KeySizeMode;
				Byte   Salt[kSaltSizeMax];
				Byte   PwdVerifComputed[kPwdVerifSize];
				CByteBuffer Password;
			};
			CKeyInfo _key;
			NSha1::CHmac _hmac;
			CAesCtr2 _aes;
			void Init2();
		public:
			MY_UNKNOWN_IMP1(ICryptoSetPassword)
			STDMETHOD(CryptoSetPassword)(const Byte *data, uint32 size);
			STDMETHOD(Init)();
			unsigned GetHeaderSize() const;
			unsigned GetAddPackSize() const;
			bool SetKeyMode(unsigned mode);
			virtual ~CBaseCoder();
		};

		class CEncoder : public CBaseCoder {
		public:
			STDMETHOD_(uint32, Filter) (Byte *data, uint32 size);
			HRESULT WriteHeader(ISequentialOutStream * outStream);
			HRESULT WriteFooter(ISequentialOutStream * outStream);
		};

		class CDecoder : public CBaseCoder /*public ICompressSetDecoderProperties2*/ {
			Byte _pwdVerifFromArchive[kPwdVerifSize];
		public:
			// ICompressSetDecoderProperties2
			// STDMETHOD(SetDecoderProperties2)(const Byte *data, uint32 size);
			STDMETHOD_(uint32, Filter) (Byte *data, uint32 size);
			HRESULT ReadHeader(ISequentialInStream * inStream);
			bool Init_and_CheckPassword();
			HRESULT CheckMac(ISequentialInStream * inStream, bool &isOK);
		};
	}
	namespace NZip {
		const uint kHeaderSize = 12;

		/* ICompressFilter::Init() does nothing for this filter.
		   Call to init:
			Encoder:
			  CryptoSetPassword();
			  WriteHeader();
			Decoder:
			  [CryptoSetPassword();]
			  ReadHeader();
			  [CryptoSetPassword();] Init_and_GetCrcByte();
			  [CryptoSetPassword();] Init_and_GetCrcByte();
		 */

		class CCipher : public ICompressFilter, public ICryptoSetPassword, public CMyUnknownImp {
		protected:
			uint32 Key0;
			uint32 Key1;
			uint32 Key2;
			uint32 KeyMem0;
			uint32 KeyMem1;
			uint32 KeyMem2;

			void RestoreKeys();
		public:
			MY_UNKNOWN_IMP1(ICryptoSetPassword)
			STDMETHOD(Init) ();
			STDMETHOD(CryptoSetPassword) (const Byte *data, uint32 size);
			virtual ~CCipher();
		};

		class CEncoder : public CCipher {
		public:
			STDMETHOD_(uint32, Filter) (Byte *data, uint32 size);
			HRESULT WriteHeader_Check16(ISequentialOutStream * outStream, uint16 crc);
		};

		class CDecoder : public CCipher {
		public:
			Byte _header[kHeaderSize];
			STDMETHOD_(uint32, Filter) (Byte *data, uint32 size);
			HRESULT ReadHeader(ISequentialInStream * inStream);
			void Init_BeforeDecode();
		};
	}
	namespace NZipStrong {
		/* ICompressFilter::Init() does nothing for this filter.
		   Call to init:
			Decoder:
			  [CryptoSetPassword();]
			  ReadHeader();
			  [CryptoSetPassword();] Init_and_CheckPassword();
			  [CryptoSetPassword();] Init_and_CheckPassword();
		 */
		class CBaseCoder : public CAesCbcDecoder, public ICryptoSetPassword {
		protected:
			struct CKeyInfo {
				void   SetPassword(const Byte * data, uint32 size);
				Byte   MasterKey[32];
				uint32 KeySize;
			};
			CKeyInfo _key;
			CByteBuffer _buf;
			Byte * _bufAligned;
		public:
			STDMETHOD(Init) ();
			STDMETHOD(CryptoSetPassword) (const Byte *data, uint32 size);
		};

		const uint kAesPadAllign = AES_BLOCK_SIZE;

		class CDecoder : public CBaseCoder {
		public:
			MY_UNKNOWN_IMP1(ICryptoSetPassword)
			HRESULT ReadHeader(ISequentialInStream * inStream, uint32 crc, uint64 unpackSize);
			HRESULT Init_and_CheckPassword(bool &passwOK);
			uint32 FASTCALL GetPadSize(uint32 packSize32) const;
		private:
			uint32 _ivSize;
			Byte _iv[16];
			uint32 _remSize;
		};
	}
	namespace NRar2 {
		/* ICompressFilter::Init() does nothing for this filter.
		   Call SetPassword() to initialize filter. */

		class CData {
			Byte SubstTable[256];
			uint32 Keys[4];
			uint32 FASTCALL SubstLong(uint32 t) const;
			void UpdateKeys(const Byte * data);
			void CryptBlock(Byte * buf, bool encrypt);
		public:
			void EncryptBlock(Byte * buf) { CryptBlock(buf, true); }
			void DecryptBlock(Byte * buf) { CryptBlock(buf, false); }
			void SetPassword(const Byte * password, unsigned passwordLen);
		};

		class CDecoder : public ICompressFilter, public CMyUnknownImp, public CData {
		public:
			MY_UNKNOWN_IMP
			INTERFACE_ICompressFilter(; )
		};
	}
	namespace NRar3 {
		const uint kAesKeySize = 16;

		class CDecoder : public CAesCbcDecoder/*, public ICompressSetDecoderProperties2, public ICryptoSetPassword*/ {
			Byte _salt[8];
			bool _thereIsSalt;
			bool _needCalc;
			// bool _rar350Mode;
			CByteBuffer _password;
			Byte _key[kAesKeySize];
			Byte _iv[AES_BLOCK_SIZE];
			void CalcKey();
		public:
			/*
			   MY_UNKNOWN_IMP1(
			   ICryptoSetPassword
			   // ICompressSetDecoderProperties2
			 */
			STDMETHOD(Init) ();
			void SetPassword(const Byte * data, uint size);
			HRESULT SetDecoderProperties2(const Byte * data, uint32 size);
			CDecoder();
			// void SetRar350Mode(bool rar350Mode) { _rar350Mode = rar350Mode; }
		};
	}
	namespace NRar5 {
		const uint kSaltSize = 16;
		const uint kPswCheckSize = 8;
		const uint kAesKeySize = 32;

		namespace NCryptoFlags {
			const uint kPswCheck = 1 << 0;
			const uint kUseMAC   = 1 << 1;
		}

		struct CKey {
			CKey();
			void   FASTCALL CopyCalcedKeysFrom(const CKey &k);
			bool   FASTCALL IsKeyEqualTo(const CKey &key);

			bool   _needCalc;
			uint8  Reserve[3]; // @alignment
			uint   _numIterationsLog;
			Byte   _salt[kSaltSize];
			CByteBuffer _password;
			Byte   _key[kAesKeySize];
			Byte   _check_Calced[kPswCheckSize];
			Byte   _hashKey[SHA256_DIGEST_SIZE];
		};

		class CDecoder : public CAesCbcDecoder, public CKey {
		public:
			CDecoder();
			STDMETHOD(Init) ();
			void SetPassword(const Byte * data, size_t size);
			HRESULT SetDecoderProps(const Byte * data, uint size, bool includeIV, bool isService);
			bool CalcKey_and_CheckPassword();
			bool UseMAC() const { return (Flags & NCryptoFlags::kUseMAC) != 0; }
			uint32 Hmac_Convert_Crc32(uint32 crc) const;
			void Hmac_Convert_32Bytes(Byte * data) const;

			Byte _iv[AES_BLOCK_SIZE];
		private:
			bool IsThereCheck() const { return ((Flags & NCryptoFlags::kPswCheck) != 0); }

			Byte _check[kPswCheckSize];
			bool _canCheck;
			uint64 Flags;
		};
	}
}
//
//#include <ArchiveOpenCallback.h>
#ifdef _NO_CRYPTO
	#define INTERFACE_IOpenCallbackUI_Crypto(x)
#else
	#define INTERFACE_IOpenCallbackUI_Crypto(x) \
		virtual HRESULT Open_CryptoGetTextPassword(BSTR *password) x; \
			/* virtual HRESULT Open_GetPasswordIfAny(bool &passwordIsDefined, UString &password) x; */ \
			/* virtual bool Open_WasPasswordAsked() x; */ \
			/* virtual void Open_Clear_PasswordWasAsked_Flag() x; */  \

#endif
#define INTERFACE_IOpenCallbackUI(x) \
	virtual HRESULT Open_CheckBreak() x; \
	virtual HRESULT Open_SetTotal(const uint64 *files, const uint64 *bytes) x; \
	virtual HRESULT Open_SetCompleted(const uint64 *files, const uint64 *bytes) x; \
	virtual HRESULT Open_Finished() x; \
	INTERFACE_IOpenCallbackUI_Crypto(x)

struct IOpenCallbackUI {
	INTERFACE_IOpenCallbackUI( = 0)
};

class COpenCallbackImp : public IArchiveOpenCallback, public IArchiveOpenVolumeCallback, public IArchiveOpenSetSubArchiveName,
  #ifndef _NO_CRYPTO
	public ICryptoGetTextPassword,
  #endif
	public CMyUnknownImp {
public:
	MY_QUERYINTERFACE_BEGIN2(IArchiveOpenVolumeCallback)
	MY_QUERYINTERFACE_ENTRY(IArchiveOpenSetSubArchiveName)
  #ifndef _NO_CRYPTO
	MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
  #endif
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE
	INTERFACE_IArchiveOpenCallback(; )
	INTERFACE_IArchiveOpenVolumeCallback(; )
  #ifndef _NO_CRYPTO
	STDMETHOD(CryptoGetTextPassword)(BSTR *password);
  #endif
	STDMETHOD(SetSubArchiveName(const wchar_t * name))
	{
		_subArchiveMode = true;
		_subArchiveName = name;
		// TotalSize = 0;
		return S_OK;
	}
private:
	FString _folderPrefix;
	NWindows::NFile::NFind::CFileInfo _fileInfo;
	bool _subArchiveMode;
	UString _subArchiveName;
public:
	COpenCallbackImp();
	void Init(const FString &folderPrefix, const FString &fileName);
	bool FASTCALL SetSecondFileInfo(CFSTR newName);

	UStringVector FileNames;
	CBoolVector FileNames_WasUsed;
	CRecordVector<uint64> FileSizes;
	bool PasswordWasAsked;
	IOpenCallbackUI * Callback;
	CMyComPtr<IArchiveOpenCallback> ReOpenCallback;
	// uint64 TotalSize;
};
//
//#include <OpenArchive.h>
#ifndef _SFX
	#define SUPPORT_ALT_STREAMS
#endif

HRESULT FASTCALL Archive_GetItemBoolProp(IInArchive * arc, uint32 index, PROPID propID, bool &result) throw();
HRESULT FASTCALL Archive_IsItem_Dir(IInArchive * arc, uint32 index, bool &result) throw();
HRESULT FASTCALL Archive_IsItem_Aux(IInArchive * arc, uint32 index, bool &result) throw();
HRESULT FASTCALL Archive_IsItem_AltStream(IInArchive * arc, uint32 index, bool &result) throw();
HRESULT FASTCALL Archive_IsItem_Deleted(IInArchive * arc, uint32 index, bool &deleted) throw();

#ifdef SUPPORT_ALT_STREAMS
	int FindAltStreamColon_in_Path(const wchar_t * path);
#endif
/*
   struct COptionalOpenProperties
   {
   UString FormatName;
   CObjectVector<CProperty> Props;
   };
 */
#ifdef _SFX
	#define OPEN_PROPS_DECL
#else
	#define OPEN_PROPS_DECL const CObjectVector<CProperty> * props;
	// #define OPEN_PROPS_DECL , const CObjectVector<COptionalOpenProperties> *props
#endif

struct COpenSpecFlags {
	COpenSpecFlags();
	bool CanReturn_NonStart() const;

	// bool CanReturnFull;
	bool   CanReturnFrontal;
	bool   CanReturnTail;
	bool   CanReturnMid;
	uint8  Reserve[1]; // @alignment
};

struct COpenType {
	COpenType();
	const COpenSpecFlags & GetSpec(bool isForced, bool isMain, bool isUnknown) const;

	int FormatIndex;
	COpenSpecFlags SpecForcedType;
	COpenSpecFlags SpecMainType;
	COpenSpecFlags SpecWrongExt;
	COpenSpecFlags SpecUnknownExt;
	/* @construction
	enum {
		fRecursive              = 0x0001,
		fCanReturnArc           = 0x0002,
		fCanReturnParser        = 0x0004,
		fEachPos                = 0x0008,
		fZerosTailIsAllowed     = 0x0010,
		fMaxStartOffset_Defined = 0x0020,	
		//fSkipSfxStub            = 0x0040,
		//fExeAsUnknown           = 0x0080
	};
	uint   Flags;
	*/
	bool   Recursive;
	bool   CanReturnArc;
	bool   CanReturnParser;
	bool   EachPos;
	bool   ZerosTailIsAllowed;
	bool   MaxStartOffset_Defined;
	uint8  Reserve[2]; // @alignment
	// bool SkipSfxStub;
	// bool ExeAsUnknown;
	uint64 MaxStartOffset;
};

struct COpenOptions {
	COpenOptions();

	CCodecs * codecs;
	COpenType openType;
	const CObjectVector <COpenType> * types;
	const CIntVector * excludedFormats;
	IInStream * stream;
	ISequentialInStream * seqStream;
	IArchiveOpenCallback * callback;
	COpenCallbackImp * callbackSpec;
	OPEN_PROPS_DECL
	// bool openOnlySpecifiedByExtension,
	bool   stdInMode;
	uint8  Reserve[3]; // @alignment
	UString filePath;
};

uint32 GetOpenArcErrorFlags(const NWindows::NCOM::CPropVariant &prop, bool * isDefinedProp = NULL);

struct CArcErrorInfo {
	CArcErrorInfo();
	// call IsArc_After_NonOpen only if Open returns S_FALSE
	bool IsArc_After_NonOpen() const;
	void ClearErrors();
	void ClearErrors_Full();
	bool IsThereErrorOrWarning() const;
	bool AreThereErrors() const;
	bool AreThereWarnings() const;
	bool NeedTailWarning() const;
	uint32 GetWarningFlags() const;
	uint32 GetErrorFlags() const;

	bool ThereIsTail;
	bool UnexpecedEnd;
	bool IgnoreTail; // all are zeros
	// bool NonZerosTail;
	bool ErrorFlags_Defined;
	uint32 ErrorFlags;
	uint32 WarningFlags;
	int ErrorFormatIndex; // - 1 means no Error. if FormatIndex == ErrorFormatIndex, the archive is open with offset
	uint64 TailSize;
	/* if CArc is Open OK with some format:
	      - ErrorFormatIndex shows error format index, if extension is incorrect
	      - other variables show message and warnings of archive that is open */
	UString ErrorMessage;
	UString WarningMessage;
};

struct CReadArcItem {
	UString Path;      // Path from root (including alt stream name, if alt stream)
	UStringVector PathParts; // without altStream name, path from root or from _baseParentFolder, if _use_baseParentFolder_mode
  #ifdef SUPPORT_ALT_STREAMS
	UString MainPath;
	/* MainPath = Path for non-AltStream,
	   MainPath = Path of parent, if there is parent for AltStream. */
	UString AltStreamName;
	bool IsAltStream;
	bool WriteToAltStreamIfColon;
  #endif
	bool IsDir;
	bool MainIsDir;
	uint32 ParentIndex; // use it, if IsAltStream
  #ifndef _SFX
	bool _use_baseParentFolder_mode;
	int _baseParentFolder;
  #endif
	CReadArcItem()
	{
    #ifdef SUPPORT_ALT_STREAMS
		WriteToAltStreamIfColon = false;
    #endif

    #ifndef _SFX
		_use_baseParentFolder_mode = false;
		_baseParentFolder = -1;
    #endif
	}
};

class CArc {
	HRESULT PrepareToOpen(const COpenOptions &op, unsigned formatIndex, CMyComPtr<IInArchive> &archive);
	HRESULT CheckZerosTail(const COpenOptions &op, uint64 offset);
	HRESULT OpenStream2(const COpenOptions &options);
  #ifndef _SFX
	// parts.Back() can contain alt stream name "nams:AltName"
	HRESULT GetItemPathToParent(uint32 index, uint32 parent, UStringVector &parts) const;
  #endif
public:
	CMyComPtr<IInArchive> Archive;
	CMyComPtr<IInStream> InStream;
	// we use InStream in 2 cases (ArcStreamOffset != 0):
	// 1) if we use additional cache stream
	// 2) we reopen sfx archive with CTailInStream
	CMyComPtr<IArchiveGetRawProps> GetRawProps;
	CMyComPtr<IArchiveGetRootProps> GetRootProps;
	CArcErrorInfo ErrorInfo; // for OK archives
	CArcErrorInfo NonOpen_ErrorInfo; // ErrorInfo for mainArchive (false OPEN)
	UString Path;
	UString filePath;
	UString DefaultName;
	int FormatIndex; // - 1 means Parser.
	int SubfileIndex;
	FILETIME MTime;
	bool MTimeDefined;
	int64 Offset; // it's offset of start of archive inside stream that is open by Archive Handler
	uint64 PhySize;
	// uint64 OkPhySize;
	bool PhySizeDefined;
	// bool OkPhySize_Defined;
	uint64 FileSize;
	uint64 AvailPhySize; // PhySize, but it's reduced if exceed end of file
	// bool offsetDefined;

	uint64 GetEstmatedPhySize() const { return PhySizeDefined ? PhySize : FileSize; }
	uint64 ArcStreamOffset; // offset of stream that is open by Archive Handler
	int64 GetGlobalOffset() const { return ArcStreamOffset + Offset; } // it's global offset of archive
	// AString ErrorFlagsText;

	bool IsParseArc;
	bool IsTree;
	bool IsReadOnly;
	bool Ask_Deleted;
	bool Ask_AltStream;
	bool Ask_Aux;
	bool Ask_INode;
	bool IgnoreSplit; // don't try split handler

	// void Set_ErrorFlagsText();
	CArc();
	HRESULT ReadBasicProps(IInArchive * archive, uint64 startPos, HRESULT openRes);
	// ~CArc();
	HRESULT Close();
	HRESULT GetItemPath(uint32 index, UString &result) const;
	HRESULT GetDefaultItemPath(uint32 index, UString &result) const;
	// GetItemPath2 adds [DELETED] dir prefix for deleted items.
	HRESULT GetItemPath2(uint32 index, UString &result) const;
	HRESULT GetItem(uint32 index, CReadArcItem &item) const;
	HRESULT GetItemSize(uint32 index, uint64 &size, bool &defined) const;
	HRESULT GetItemMTime(uint32 index, FILETIME &ft, bool &defined) const;
	HRESULT IsItemAnti(uint32 index, bool &result) const;
	HRESULT OpenStream(const COpenOptions &options);
	HRESULT OpenStreamOrFile(COpenOptions &options);
	HRESULT ReOpen(const COpenOptions &options);
	HRESULT CreateNewTailStream(CMyComPtr<IInStream> &stream);
};

struct CArchiveLink {
	CArchiveLink();
	~CArchiveLink();
	void KeepModeForNextOpen();
	HRESULT Close();
	void Release();
	const CArc * GetArc() const { return &Arcs.Back(); }
	IInArchive * GetArchive() const { return Arcs.Back().Archive; }
	IArchiveGetRawProps * GetArchiveGetRawProps() const { return Arcs.Back().GetRawProps; }
	IArchiveGetRootProps * GetArchiveGetRootProps() const { return Arcs.Back().GetRootProps; }
	HRESULT Open(COpenOptions &options);
	HRESULT Open2(COpenOptions &options, IOpenCallbackUI * callbackUI);
	HRESULT Open3(COpenOptions &options, IOpenCallbackUI * callbackUI);
	HRESULT Open_Strict(COpenOptions &options, IOpenCallbackUI * callbackUI);
	HRESULT ReOpen(COpenOptions &options);
	// void Set_ErrorsText();

	CObjectVector<CArc> Arcs;
	UStringVector VolumePaths;
	uint64 VolumesSize;
	bool   IsOpen;
	bool   PasswordWasAsked;
	uint8  Reserve[2]; // @alignment
	// UString Password;
	// int NonOpenErrorFormatIndex; // - 1 means no Error.
	UString NonOpen_ArcPath;
	CArcErrorInfo NonOpen_ErrorInfo;
	// UString ErrorsText;
};

bool ParseOpenTypes(CCodecs &codecs, const UString &s, CObjectVector<COpenType> &types);
//
//#include <UpdateCallback.h>
struct CArcToDoStat {
	uint64 Get_NumDataItems_Total() const { return NewData.Get_NumDataItems() + OldData.Get_NumDataItems(); }
	CDirItemsStat NewData;
	CDirItemsStat OldData;
	CDirItemsStat DeleteData;
};

#define INTERFACE_IUpdateCallbackUI(x) \
	virtual HRESULT WriteSfx(const wchar_t * name, uint64 size) x; \
	virtual HRESULT SetTotal(uint64 size) x; \
	virtual HRESULT SetCompleted(const uint64 *completeValue) x; \
	virtual HRESULT SetRatioInfo(const uint64 *inSize, const uint64 *outSize) x; \
	virtual HRESULT CheckBreak() x;	\
        /* virtual HRESULT Finalize() x; */ \
	virtual HRESULT SetNumItems(const CArcToDoStat &stat) x; \
	virtual HRESULT GetStream(const wchar_t * name, bool isDir, bool isAnti, uint32 mode) x; \
	virtual HRESULT OpenFileError(const FString &path, DWORD systemError) x; \
	virtual HRESULT ReadingFileError(const FString &path, DWORD systemError) x; \
	virtual HRESULT SetOperationResult(int32 opRes) x; \
	virtual HRESULT ReportExtractResult(int32 opRes, int32 isEncrypted, const wchar_t * name) x; \
	virtual HRESULT ReportUpdateOpeartion(uint32 op, const wchar_t * name, bool isDir) x; \
        /* virtual HRESULT SetPassword(const UString &password) x; */ \
	virtual HRESULT CryptoGetTextPassword2(int32 *passwordIsDefined, BSTR *password) x; \
	virtual HRESULT CryptoGetTextPassword(BSTR *password) x; \
	virtual HRESULT ShowDeleteFile(const wchar_t * name, bool isDir) x; \
        /* virtual HRESULT CloseProgress() { return S_OK; } */

struct IUpdateCallbackUI {
	INTERFACE_IUpdateCallbackUI( = 0)
};
//
HRESULT Bench(DECL_EXTERNAL_CODECS_LOC_VARS IBenchPrintCallback * printCallback, IBenchCallback * benchCallback,
    /*IBenchFreqCallback *freqCallback,*/ const CObjectVector<CProperty> &props, uint32 numIterations, bool multiDict);
//
//#include <BenchCon.h>
HRESULT BenchCon(DECL_EXTERNAL_CODECS_LOC_VARS const CObjectVector<CProperty> &props, uint32 numIterations, FILE *f);
//
//#include <ConsoleClose.h>
namespace NConsoleClose {
	extern unsigned g_BreakCounter;

	inline bool TestBreakSignal()
	{
	#ifdef UNDER_CE
		return false;
	#else
		return (g_BreakCounter != 0);
	#endif
	}

	class CCtrlHandlerSetter {
	public:
		CCtrlHandlerSetter();
		virtual ~CCtrlHandlerSetter();
	};
	class CCtrlBreakException {
	};
	// void CheckCtrlBreak();
}
//
//#include <UserInputUtils.h>

NUserAnswerMode::EEnum ScanUserYesNoAllQuit(CStdOutStream * outStream);
// bool GetPassword(CStdOutStream *outStream, UString &psw);
HRESULT GetPassword_HRESULT(CStdOutStream * outStream, UString &psw);
//
//#include <HandlerOut.h> <7zCompressionMode.h> <7zEncode.h> <7zOut.h> <7zIn.h> <7zDecode.h> <7zHandler.h> <7zFolderInStream.h> <7zUpdate.h>
//  <ZipCompressionMode.h> <ZipAddCommon.h> <ZipOut.h> <ZipIn.h> <ZipHandler.h> <ZipUpdate.h> <CabIn.h> <CabIn.h> <CabHandler.h>
//  <TarIn.h> <TarOut.h> <TarHandler.h> <TarUpdate.h>

API_FUNC_IsArc IsArc_Zip(const Byte * p, size_t size);

namespace NArchive {
	class CMultiMethodProps {
		uint32 _level;
		int _analysisLevel;
	public:
	#ifndef _7ZIP_ST
		uint32 _numThreads;
		uint32 _numProcessors;
	#endif
		uint32 _crcSize;
		CObjectVector<COneMethodInfo> _methods;
		COneMethodInfo _filterMethod;
		bool _autoFilter;

		void SetGlobalLevelTo(COneMethodInfo &oneMethodInfo) const;
	  #ifndef _7ZIP_ST
		static void SetMethodThreadsTo(COneMethodInfo &oneMethodInfo, uint32 numThreads);
	  #endif
		uint   GetNumEmptyMethods() const;
		int    GetLevel() const { return _level == (uint32)(int32)-1 ? 5 : (int)_level; }
		int    GetAnalysisLevel() const { return _analysisLevel; }
		void   Init();
		CMultiMethodProps() 
		{ 
			Init();
		}
		HRESULT SetProperty(const wchar_t * name, const PROPVARIANT &value);
	};

	class CSingleMethodProps : public COneMethodInfo {
		uint32 _level;
	public:
	  #ifndef _7ZIP_ST
		uint32 _numThreads;
		uint32 _numProcessors;
	  #endif
		void Init();
		CSingleMethodProps() 
		{
			Init();
		}
		int GetLevel() const { return _level == (uint32)(int32)-1 ? 5 : (int)_level; }
		HRESULT SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps);
	};
	
	namespace N7z {
		struct CMethodFull : public CMethodProps {
			bool IsSimpleCoder() const { return NumStreams == 1; }
			CMethodId Id;
			uint32 NumStreams;
		};
		struct CBond2 {
			uint32 OutCoder;
			uint32 OutStream;
			uint32 InCoder;
		};

		struct CCompressionMethodMode {
			/*
			if(Bonds.Empty()), then default bonds must be created
			if(Filter_was_Inserted) {
				Methods[0] is filter method
				Bonds don't contain bonds for filter (these bonds must be created)
			}
			*/
			CObjectVector<CMethodFull> Methods;
			CRecordVector<CBond2> Bonds;

			bool IsThereBond_to_Coder(unsigned coderIndex) const
			{
				FOR_VECTOR(i, Bonds) {
					if(Bonds[i].InCoder == coderIndex)
						return true;
				}
				return false;
			}
			bool DefaultMethod_was_Inserted;
			bool Filter_was_Inserted;
		#ifndef _7ZIP_ST
			uint32 NumThreads;
			bool MultiThreadMixer;
		#endif
			bool PasswordIsDefined;
			UString Password;
			
			bool IsEmpty() const { return (Methods.IsEmpty() && !PasswordIsDefined); }
			CCompressionMethodMode() : DefaultMethod_was_Inserted(false), Filter_was_Inserted(false), PasswordIsDefined(false)
			#ifndef _7ZIP_ST
				, NumThreads(1), MultiThreadMixer(true)
			#endif
			{
			}
		};

		class CMtEncMultiProgress : public ICompressProgressInfo, public CMyUnknownImp {
			CMyComPtr<ICompressProgressInfo> _progress;
		  #ifndef _7ZIP_ST
			NWindows::NSynchronization::CCriticalSection CriticalSection;
		  #endif
		public:
			uint64 OutSize;
			CMtEncMultiProgress() : OutSize(0) 
			{
			}
			void Init(ICompressProgressInfo * progress);
			void AddOutSize(uint64 addOutSize)
			{
			#ifndef _7ZIP_ST
				NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
			#endif
				OutSize += addOutSize;
			}
			MY_UNKNOWN_IMP1(ICompressProgressInfo)
			STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
		};

		class CEncoder {
		  #ifdef USE_MIXER_ST
			NCoderMixer2::CMixerST * _mixerST;
		  #endif
		  #ifdef USE_MIXER_MT
			NCoderMixer2::CMixerMT * _mixerMT;
		  #endif
			NCoderMixer2::CMixer * _mixer;
			CMyComPtr<IUnknown> _mixerRef;
			CCompressionMethodMode _options;
			NCoderMixer2::CBindInfo _bindInfo;
			CRecordVector<CMethodId> _decompressionMethods;
			CRecordVector<uint32> _SrcIn_to_DestOut;
			CRecordVector<uint32> _SrcOut_to_DestIn;
			// CRecordVector<uint32> _DestIn_to_SrcOut;
			CRecordVector<uint32> _DestOut_to_SrcIn;

			void InitBindConv();
			void SetFolder(CFolder &folder);
			HRESULT CreateMixerCoder(DECL_EXTERNAL_CODECS_LOC_VARS const uint64 * inSizeForReduce);

			bool _constructed;
		public:
			CEncoder(const CCompressionMethodMode &options);
			~CEncoder();
			HRESULT EncoderConstr();
			HRESULT Encode(DECL_EXTERNAL_CODECS_LOC_VARS ISequentialInStream * inStream, /*const uint64 *inStreamSize,*/
				const uint64 * inSizeForReduce, CFolder &folderItem, CRecordVector<uint64> &coderUnpackSizes, uint64 &unpackSize,
				ISequentialOutStream * outStream, CRecordVector<uint64> &packSizes, ICompressProgressInfo * compressProgress);
		};

		struct CHeaderOptions {
			bool CompressMainHeader;
			/*
			   bool WriteCTime;
			   bool WriteATime;
			   bool WriteMTime;
			 */
			CHeaderOptions() : CompressMainHeader(true)
				//, WriteCTime(false), WriteATime(false), WriteMTime(true)
			{
			}
		};

		struct CFileItem2 {
			uint64 CTime;
			uint64 ATime;
			uint64 MTime;
			uint64 StartPos;
			uint32 Attrib;
			bool CTimeDefined;
			bool ATimeDefined;
			bool MTimeDefined;
			bool StartPosDefined;
			bool AttribDefined;
			bool IsAnti;
			// bool IsAux;
			/*
			   void Init()
			   {
			   CTimeDefined = false;
			   ATimeDefined = false;
			   MTimeDefined = false;
			   StartPosDefined = false;
			   AttribDefined = false;
			   IsAnti = false;
			   // IsAux = false;
			   }
			 */
		};

		struct COutFolders {
			void OutFoldersClear();
			void OutFoldersReserveDown();

			CUInt32DefVector FolderUnpackCRCs; // Now we use it for headers only.
			CRecordVector <CNum> NumUnpackStreamsVector;
			CRecordVector <uint64> CoderUnpackSizes; // including unpack sizes of bond coders
		};

		struct CArchiveDatabaseOut : public COutFolders {
			CRecordVector<uint64> PackSizes;
			CUInt32DefVector PackCRCs;
			CObjectVector<CFolder> Folders;
			CRecordVector<CFileItem> Files;
			UStringVector Names;
			CUInt64DefVector CTime;
			CUInt64DefVector ATime;
			CUInt64DefVector MTime;
			CUInt64DefVector StartPos;
			CUInt32DefVector Attrib;
			CBoolVector IsAnti;
			/*
			   CBoolVector IsAux;

			   CByteBuffer SecureBuf;
			   CRecordVector<uint32> SecureSizes;
			   CRecordVector<uint32> SecureIDs;

			   void ClearSecure()
			   {
			   SecureBuf.Free();
			   SecureSizes.Clear();
			   SecureIDs.Clear();
			   }
			 */
			void Clear();
			void ReserveDown();
			bool IsEmpty() const;
			bool CheckNumFiles() const;
			bool IsItemAnti(uint index) const;
			// bool IsItemAux(uint index) const;
			void SetItem_Anti(uint index, bool isAnti);
			/*
			   void SetItem_Aux(uint index, bool isAux)
			   {
			   while (index >= IsAux.Size())
				IsAux.Add(false);
			   IsAux[index] = isAux;
			   }
			 */
			void AddFile(const CFileItem &file, const CFileItem2 &file2, const UString &name);
		};

		class COutArchive {
		private:
			class CWriteBufferLoc {
			public:
				CWriteBufferLoc();
				void Init(Byte * data, size_t size);
				void WriteBytes(const void * data, size_t size);
				void FASTCALL WriteByte(Byte b);
				size_t GetPos() const { return _pos; }
			private:
				Byte * _data;
				size_t _size;
				size_t _pos;
			};
			uint64 _prefixHeaderPos;
			HRESULT WriteDirect(const void * data, uint32 size) { return WriteStream(SeqStream, data, size); }
			uint64 GetPos() const;
			void WriteBytes(const void * data, size_t size);
			void WriteBytes(const CByteBuffer &data) { WriteBytes(data, data.Size()); }
			void WriteByte(Byte b);
			void WriteUInt32(uint32 value);
			void WriteUInt64(uint64 value);
			void WriteNumber(uint64 value);
			void WriteID(uint64 value) { WriteNumber(value); }
			void WriteFolder(const CFolder &folder);
			HRESULT WriteFileHeader(const CFileItem &itemInfo);
			void WriteBoolVector(const CBoolVector &boolVector);
			void WritePropBoolVector(Byte id, const CBoolVector &boolVector);
			void WriteHashDigests(const CUInt32DefVector &digests);
			void WritePackInfo(uint64 dataOffset, const CRecordVector<uint64> &packSizes, const CUInt32DefVector &packCRCs);
			void WriteUnpackInfo(const CObjectVector<CFolder> &folders, const COutFolders &outFolders);
			void WriteSubStreamsInfo(const CObjectVector<CFolder> &folders, const COutFolders &outFolders,
				const CRecordVector<uint64> &unpackSizes, const CUInt32DefVector &digests);
			void SkipToAligned(unsigned pos, unsigned alignShifts);
			void WriteAlignedBools(const CBoolVector &v, unsigned numDefined, Byte type, unsigned itemSizeShifts);
			void WriteUInt64DefVector(const CUInt64DefVector &v, Byte type);
			HRESULT EncodeStream(DECL_EXTERNAL_CODECS_LOC_VARS CEncoder &encoder, const CByteBuffer &data,
				CRecordVector<uint64> &packSizes, CObjectVector<CFolder> &folders, COutFolders &outFolders);
			void WriteHeader(const CArchiveDatabaseOut &db, /*const CHeaderOptions &headerOptions,*/ uint64 &headerOffset);

			bool _countMode;
			bool _writeToStream;
			size_t _countSize;
			uint32 _crc;
			COutBuffer _outByte;
			CWriteBufferLoc _outByte2;
		  #ifdef _7Z_VOL
			bool _endMarker;
		  #endif
			bool _useAlign;
			HRESULT WriteSignature();
		  #ifdef _7Z_VOL
			HRESULT WriteFinishSignature();
		  #endif
			HRESULT WriteStartHeader(const CStartHeader &h);
		  #ifdef _7Z_VOL
			HRESULT WriteFinishHeader(const CFinishHeader &h);
		  #endif
			CMyComPtr<IOutStream> Stream;
		public:
			COutArchive() { _outByte.Create(1 << 16); }
			CMyComPtr<ISequentialOutStream> SeqStream;
			HRESULT Create(ISequentialOutStream * stream, bool endMarker);
			void Close();
			HRESULT SkipPrefixArchiveHeader();
			HRESULT WriteDatabase(DECL_EXTERNAL_CODECS_LOC_VARS const CArchiveDatabaseOut &db, const CCompressionMethodMode * options, const CHeaderOptions &headerOptions);
		  #ifdef _7Z_VOL
			static uint32 GetVolHeadersSize(uint64 dataSize, int nameLength = 0, bool props = false);
			static uint64 GetVolPureSize(uint64 volSize, int nameLength = 0, bool props = false);
		  #endif
		};
		// 
		// We don't need to init isEncrypted and passwordIsDefined
		// We must upgrade them only 
		// 
		#ifdef _NO_CRYPTO
			#define _7Z_DECODER_CRYPRO_VARS_DECL
			#define _7Z_DECODER_CRYPRO_VARS
		#else
			#define _7Z_DECODER_CRYPRO_VARS_DECL , ICryptoGetTextPassword *getTextPassword, bool &isEncrypted, bool &passwordIsDefined, \
			UString &password
			#define _7Z_DECODER_CRYPRO_VARS , getTextPassword, isEncrypted, passwordIsDefined, password
		#endif

		struct CParsedMethods {
			CParsedMethods() : Lzma2Prop(0), LzmaDic(0) 
			{
			}
			Byte Lzma2Prop;
			uint32 LzmaDic;
			CRecordVector<uint64> IDs;
		};

		struct CFolderEx : public CFolder {
			uint   UnpackCoder;
		};

		struct CFolders {
			CFolders();
			void   Clear();
			void   ParseFolderInfo(uint folderIndex, CFolder & folder) const;
			void   ParseFolderEx(uint folderIndex, CFolderEx & folder) const;
			uint   FASTCALL GetNumFolderUnpackSizes(uint folderIndex) const;
			uint64 FASTCALL GetFolderUnpackSize(uint folderIndex) const;
			uint64 FASTCALL GetStreamPackSize(uint index) const;

			CNum NumPackStreams;
			CNum NumFolders;
			CObjArray<uint64> PackPositions; // NumPackStreams + 1
			// CUInt32DefVector PackCRCs; // we don't use PackCRCs now
			CUInt32DefVector FolderCRCs;       // NumFolders
			CObjArray<CNum> NumUnpackStreamsVector; // NumFolders
			CObjArray<uint64> CoderUnpackSizes; // including unpack sizes of bond coders
			CObjArray<CNum> FoToCoderUnpackSizes; // NumFolders + 1
			CObjArray<CNum> FoStartPackStreamIndex; // NumFolders + 1
			CObjArray<Byte> FoToMainUnpackSizeIndex; // NumFolders
			CObjArray<size_t> FoCodersDataOffset; // NumFolders + 1
			CByteBuffer CodersData;
			CParsedMethods ParsedMethods;
		};

		struct CDatabase : public CFolders {
			CRecordVector<CFileItem> Files;
			CUInt64DefVector CTime;
			CUInt64DefVector ATime;
			CUInt64DefVector MTime;
			CUInt64DefVector StartPos;
			CUInt32DefVector Attrib;
			CBoolVector IsAnti;
			//CBoolVector IsAux;
			//CByteBuffer SecureBuf;
			//CRecordVector<uint32> SecureIDs;
			CByteBuffer NamesBuf;
			CObjArray<size_t> NameOffsets; // numFiles + 1, offsets of utf-16 symbols
			//void ClearSecure();
			void Clear();
			bool IsSolid() const;
			bool FASTCALL IsItemAnti(uint index) const;
			// bool FASTCALL IsItemAux(uint index) const;
			//const void * FASTCALL GetName(uint index) const;
			void GetPath(uint index, UString &path) const;
			HRESULT GetPath_Prop(uint index, PROPVARIANT * path) const throw();
		};

		struct CInArchiveInfo {
			void Clear();
			CArchiveVersion Version;
			uint64 StartPosition;
			uint64 StartPositionAfterHeader;
			uint64 DataStartPosition;
			uint64 DataStartPosition2;
			CRecordVector<uint64> FileInfoPopIDs;
		};

		struct CDbEx : public CDatabase {
			CInArchiveInfo ArcInfo;
			CObjArray<CNum> FolderStartFileIndex;
			CObjArray<CNum> FileIndexToFolderIndexMap;
			uint64 HeadersSize;
			uint64 PhySize;
			//CRecordVector<size_t> SecureOffsets;
			//bool IsTree;
			//bool ThereAreAltStreams;
			bool IsArc;
			bool PhySizeWasConfirmed;
			bool ThereIsHeaderError;
			bool UnexpectedEnd;
			// bool UnsupportedVersion;
			bool StartHeaderWasRecovered;
			bool UnsupportedFeatureWarning;
			bool UnsupportedFeatureError;
			/*void ClearSecureEx()
			{
			ClearSecure();
			SecureOffsets.Clear();
			}*/
			void Clear();
			void FillLinks();
			uint64 GetFolderStreamPos(CNum folderIndex, unsigned indexInFolder) const;
			uint64 GetFolderFullPackSize(CNum folderIndex) const;
			uint64 GetFolderPackStreamSize(CNum folderIndex, unsigned streamIndex) const;
			uint64 GetFilePackSize(CNum fileIndex) const;
		};

		const uint kNumBufLevelsMax = 4;

		struct CInByte2 {
			const Byte * _buffer;
		public:
			size_t _size;
			size_t _pos;
			size_t GetRem() const { return _size - _pos; }
			const Byte * GetPtr() const { return _buffer + _pos; }
			void Init(const Byte * buffer, size_t size)
			{
				_buffer = buffer;
				_size = size;
				_pos = 0;
			}
			Byte ReadByte();
			void ReadBytes(Byte * data, size_t size);
			void SkipDataNoCheck(uint64 size) { _pos += (size_t)size; }
			void SkipData(uint64 size);
			void SkipData();
			void SkipRem() { _pos = _size; }
			uint64 ReadNumber();
			CNum ReadNum();
			uint32 ReadUInt32();
			uint64 ReadUInt64();
			void ParseFolder(CFolder &folder);
		};

		class CStreamSwitch;

		const uint32 kHeaderSize = 32;

		class CInArchive {
			friend class CStreamSwitch;
			CMyComPtr<IInStream> _stream;
			unsigned _numInByteBufs;
			CInByte2 _inByteVector[kNumBufLevelsMax];
			CInByte2 * _inByteBack;
			bool ThereIsHeaderError;
			uint64 _arhiveBeginStreamPosition;
			uint64 _fileEndPosition;
			Byte _header[kHeaderSize];
			uint64 HeadersSize;
			bool _useMixerMT;

			void AddByteStream(const Byte * buffer, size_t size);
			void DeleteByteStream(bool needUpdatePos);
			HRESULT FindAndReadSignature(IInStream * stream, const uint64 * searchHeaderSizeLimit);
			void ReadBytes(Byte * data, size_t size) { _inByteBack->ReadBytes(data, size); }
			Byte ReadByte() { return _inByteBack->ReadByte(); }
			uint64 ReadNumber() { return _inByteBack->ReadNumber(); }
			CNum ReadNum() { return _inByteBack->ReadNum(); }
			uint64 ReadID() { return _inByteBack->ReadNumber(); }
			uint32 ReadUInt32() { return _inByteBack->ReadUInt32(); }
			uint64 ReadUInt64() { return _inByteBack->ReadUInt64(); }
			void SkipData(uint64 size) { _inByteBack->SkipData(size); }
			void SkipData() { _inByteBack->SkipData(); }
			void WaitId(uint64 id);
			void Read_UInt32_Vector(CUInt32DefVector &v);
			void ReadArchiveProperties(CInArchiveInfo &archiveInfo);
			void ReadHashDigests(unsigned numItems, CUInt32DefVector &crcs);
			void ReadPackInfo(CFolders &f);
			void ReadUnpackInfo(const CObjectVector<CByteBuffer> * dataVector, CFolders &folders);
			void ReadSubStreamsInfo(CFolders &folders, CRecordVector<uint64> &unpackSizes, CUInt32DefVector &digests);
			void ReadStreamsInfo(const CObjectVector<CByteBuffer> * dataVector, uint64 &dataOffset,
				CFolders &folders, CRecordVector<uint64> &unpackSizes, CUInt32DefVector &digests);
			void ReadBoolVector(unsigned numItems, CBoolVector &v);
			void ReadBoolVector2(unsigned numItems, CBoolVector &v);
			void ReadUInt64DefVector(const CObjectVector<CByteBuffer> &dataVector, CUInt64DefVector &v, unsigned numItems);
			HRESULT ReadAndDecodePackedStreams(DECL_EXTERNAL_CODECS_LOC_VARS uint64 baseOffset, uint64 &dataOffset,
				CObjectVector<CByteBuffer> &dataVector _7Z_DECODER_CRYPRO_VARS_DECL);
			HRESULT ReadHeader(DECL_EXTERNAL_CODECS_LOC_VARS CDbEx &db _7Z_DECODER_CRYPRO_VARS_DECL);
			HRESULT ReadDatabase2(DECL_EXTERNAL_CODECS_LOC_VARS CDbEx &db _7Z_DECODER_CRYPRO_VARS_DECL);
		public:
			CInArchive(bool useMixerMT) : _numInByteBufs(0), _useMixerMT(useMixerMT)
			{
			}
			HRESULT Open(IInStream * stream, const uint64 * searchHeaderSizeLimit); // S_FALSE means is not archive
			void Close();
			HRESULT ReadDatabase(DECL_EXTERNAL_CODECS_LOC_VARS CDbEx &db _7Z_DECODER_CRYPRO_VARS_DECL);
		};

		struct CBindInfoEx : public NCoderMixer2::CBindInfo {
			CRecordVector<CMethodId> CoderMethodIDs;
			void Clear()
			{
				CBindInfo::Clear();
				CoderMethodIDs.Clear();
			}
		};

		class CDecoder {
			bool _bindInfoPrev_Defined;
			CBindInfoEx _bindInfoPrev;
			bool _useMixerMT;
		  #ifdef USE_MIXER_ST
			NCoderMixer2::CMixerST * _mixerST;
		  #endif
		  #ifdef USE_MIXER_MT
			NCoderMixer2::CMixerMT * _mixerMT;
		  #endif
			NCoderMixer2::CMixer * _mixer;
			CMyComPtr<IUnknown> _mixerRef;
		public:
			CDecoder(bool useMixerMT);
			HRESULT Decode(DECL_EXTERNAL_CODECS_LOC_VARS IInStream * inStream, uint64 startPos, const CFolders &folders, unsigned folderIndex,
				const uint64 * unpackSize // if(!unpackSize), then full folder is required
				// if(unpackSize), then only *unpackSize bytes from folder are required
				, ISequentialOutStream * outStream, ICompressProgressInfo * compressProgress, ISequentialInStream ** inStreamMainRes, bool &dataAfterEnd_Error
				_7Z_DECODER_CRYPRO_VARS_DECL
			  #if !defined(_7ZIP_ST) && !defined(_SFX)
				, bool mtMode, uint32 numThreads
			  #endif
			);
		};

		#ifndef __7Z_SET_PROPERTIES
			#ifdef EXTRACT_ONLY
				#if !defined(_7ZIP_ST) && !defined(_SFX)
					#define __7Z_SET_PROPERTIES
				#endif
			#else
				#define __7Z_SET_PROPERTIES
			#endif
		#endif
		#ifndef EXTRACT_ONLY
		class COutHandler : public CMultiMethodProps {
			HRESULT SetSolidFromString(const UString &s);
			HRESULT SetSolidFromPROPVARIANT(const PROPVARIANT &value);
		public:
			bool _removeSfxBlock;
			uint64 _numSolidFiles;
			uint64 _numSolidBytes;
			bool _numSolidBytesDefined;
			bool _solidExtension;
			bool _useTypeSorting;
			bool _compressHeaders;
			bool _encryptHeadersSpecified;
			bool _encryptHeaders;
			// bool _useParents; 9.26
			CBoolPair Write_CTime;
			CBoolPair Write_ATime;
			CBoolPair Write_MTime;
			CBoolPair Write_Attrib;
			bool _useMultiThreadMixer;
			// bool _volumeMode;

			void InitSolidFiles();
			void InitSolidSize();
			void InitSolid();
			void InitProps();
			COutHandler() { InitProps(); }

			HRESULT SetProperty(const wchar_t * name, const PROPVARIANT &value);
		};
		#endif

		class CHandler : public IInArchive, public IArchiveGetRawProps,
		  #ifdef __7Z_SET_PROPERTIES
			public ISetProperties,
		  #endif
		  #ifndef EXTRACT_ONLY
			public IOutArchive,
		  #endif
			PUBLIC_ISetCompressCodecsInfo
			public CMyUnknownImp
		  #ifndef EXTRACT_ONLY
			, public COutHandler
		  #endif
		{
		public:
			MY_QUERYINTERFACE_BEGIN2(IInArchive)
			MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
		  #ifdef __7Z_SET_PROPERTIES
			MY_QUERYINTERFACE_ENTRY(ISetProperties)
		  #endif
		  #ifndef EXTRACT_ONLY
			MY_QUERYINTERFACE_ENTRY(IOutArchive)
		  #endif
			QUERY_ENTRY_ISetCompressCodecsInfo
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE
			INTERFACE_IInArchive(; )
			INTERFACE_IArchiveGetRawProps(; )
		  #ifdef __7Z_SET_PROPERTIES
			STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps);
		  #endif
		  #ifndef EXTRACT_ONLY
			INTERFACE_IOutArchive(; )
		  #endif
			DECL_ISetCompressCodecsInfo CHandler();
		private:
			CMyComPtr<IInStream> _inStream;
			NArchive::N7z::CDbEx _db;
		  #ifndef _NO_CRYPTO
			bool _isEncrypted;
			bool _passwordIsDefined;
			UString _password;
		  #endif
		  #ifdef EXTRACT_ONLY
		  #ifdef __7Z_SET_PROPERTIES
			uint32 _numThreads;
			bool _useMultiThreadMixer;
		  #endif
			uint32 _crcSize;
		  #else
			CRecordVector<CBond2> _bonds;
			HRESULT PropsMethod_To_FullMethod(CMethodFull &dest, const COneMethodInfo &m);
			HRESULT SetHeaderMethod(CCompressionMethodMode &headerMethod);
			HRESULT SetMainMethod(CCompressionMethodMode &method
			  #ifndef _7ZIP_ST
						, uint32 numThreads
			  #endif
						);

		  #endif
			bool IsFolderEncrypted(CNum folderIndex) const;
		  #ifndef _SFX
			CRecordVector<uint64> _fileInfoPopIDs;
			void FillPopIDs();
			void AddMethodName(AString &s, uint64 id);
			HRESULT SetMethodToProp(CNum folderIndex, PROPVARIANT * prop) const;
		  #endif
			DECL_EXTERNAL_CODECS_VARS
		};

		class CFolderInStream : public ISequentialInStream, public ICompressGetSubStreamSize, public CMyUnknownImp {
			CMyComPtr <ISequentialInStream> _stream;
			uint64 _pos;
			uint32 _crc;
			bool   _size_Defined;
			uint64 _size;
			const  uint32 * _indexes;
			uint   _numFiles;
			uint   _index;
			CMyComPtr<IArchiveUpdateCallback> _updateCallback;

			HRESULT OpenStream();
			void AddFileInfo(bool isProcessed);
		public:
			CRecordVector<bool> Processed;
			CRecordVector<uint32> CRCs;
			CRecordVector<uint64> Sizes;

			MY_UNKNOWN_IMP2(ISequentialInStream, ICompressGetSubStreamSize)
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			STDMETHOD(GetSubStreamSize) (uint64 subStream, uint64 *value);

			void Init(IArchiveUpdateCallback * updateCallback, const uint32 * indexes, unsigned numFiles);
			bool WasFinished() const;
			uint64 GetFullSize() const;
		};
		/*
			struct CTreeFolder {
				UString Name;
				int Parent;
				CIntVector SubFolders;
				int UpdateItemIndex;
				int SortIndex;
				int SortIndexEnd;

				CTreeFolder(): UpdateItemIndex(-1) {}
			};
		 */
		struct CUpdateItem {
			int IndexInArchive;
			int IndexInClient;
			uint64 CTime;
			uint64 ATime;
			uint64 MTime;
			uint64 Size;
			UString Name;
			/*
			   bool IsAltStream;
			   int ParentFolderIndex;
			   int TreeFolderIndex;
			 */
			// that code is not used in 9.26
			// int ParentSortIndex;
			// int ParentSortIndexEnd;
			uint32 Attrib;
			bool NewData;
			bool NewProps;
			bool IsAnti;
			bool IsDir;
			bool AttribDefined;
			bool CTimeDefined;
			bool ATimeDefined;
			bool MTimeDefined;
			// int SecureIndex; // 0 means (no_security)
			bool HasStream() const { return !IsDir && !IsAnti && Size != 0; }
			// bool HasStream() const { return !IsDir && !IsAnti /* && Size != 0 */; } // for test purposes
			CUpdateItem() :
				// ParentSortIndex(-1),
				// IsAltStream(false),
				IsAnti(false), IsDir(false), AttribDefined(false), CTimeDefined(false), ATimeDefined(false), MTimeDefined(false)
				// SecureIndex(0)
			{
			}
			void SetDirStatusFromAttrib() { IsDir = ((Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0); }
			// unsigned GetExtensionPos() const;
			// UString GetExtension() const;
		};

		struct CUpdateOptions {
			const CCompressionMethodMode * Method;
			const CCompressionMethodMode * HeaderMethod;
			bool UseFilters; // use additional filters for some files
			bool MaxFilter; // use BCJ2 filter instead of BCJ
			int AnalysisLevel;
			CHeaderOptions HeaderOptions;
			uint64 NumSolidFiles;
			uint64 NumSolidBytes;
			bool SolidExtension;
			bool UseTypeSorting;
			bool RemoveSfxBlock;
			bool MultiThreadMixer;

			CUpdateOptions() : Method(NULL), HeaderMethod(NULL), UseFilters(false), MaxFilter(false), AnalysisLevel(-1),
				NumSolidFiles(static_cast<uint64>(-1LL)), NumSolidBytes(static_cast<uint64>(-1LL)), SolidExtension(false),
				UseTypeSorting(true), RemoveSfxBlock(false), MultiThreadMixer(true)
			{
			}
		};

		HRESULT Update(DECL_EXTERNAL_CODECS_LOC_VARS IInStream * inStream, const CDbEx * db, const CObjectVector<CUpdateItem> &updateItems,
			// const CObjectVector<CTreeFolder> &treeFolders, // treeFolders[0] is root
			// const CUniqBlocks &secureBlocks,
			COutArchive &archive, CArchiveDatabaseOut &newDatabase, ISequentialOutStream * seqOutStream, IArchiveUpdateCallback * updateCallback, const CUpdateOptions &options
			#ifndef _NO_CRYPTO
					, ICryptoGetTextPassword * getDecoderPassword
			#endif
					);
	}
	namespace NZip {
		const CMethodId kMethodId_ZipBase = 0x040100;
		const CMethodId kMethodId_BZip2   = 0x040202;

		struct CBaseProps : public CMultiMethodProps {
			void Init()
			{
				CMultiMethodProps::Init();
				IsAesMode = false;
				AesKeyMode = 3;
			}
			bool IsAesMode;
			Byte AesKeyMode;
		};

		struct CCompressionMethodMode : public CBaseProps {
			CCompressionMethodMode() : PasswordIsDefined(false), _dataSizeReduceDefined(false), _dataSizeReduce(0)
			{
			}
			bool IsRealAesMode() const { return PasswordIsDefined && IsAesMode; }

			CRecordVector<Byte> MethodSequence;
			bool PasswordIsDefined;
			AString Password;
			uint64 _dataSizeReduce;
			bool _dataSizeReduceDefined;
		};

		struct CCompressingResult {
			uint64 UnpackSize;
			uint64 PackSize;
			uint32 CRC;
			uint16 Method;
			Byte   ExtractVersion;
			bool   FileTimeWasUsed;
			bool   LzmaEos;
			uint8  Reserve[3]; // @alignment
		};

		class CAddCommon {
			CCompressionMethodMode _options;
			NCompress::CCopyCoder * _copyCoderSpec;
			CMyComPtr<ICompressCoder> _copyCoder;
			CMyComPtr<ICompressCoder> _compressEncoder;
			Byte _compressExtractVersion;
			bool _isLzmaEos;
			CFilterCoder * _cryptoStreamSpec;
			CMyComPtr<ISequentialOutStream> _cryptoStream;
			NCrypto::NZip::CEncoder * _filterSpec;
			NCrypto::NWzAes::CEncoder * _filterAesSpec;
			Byte * _buf;

			HRESULT CalcStreamCRC(ISequentialInStream * inStream, uint32 &resultCRC);
		public:
			CAddCommon(const CCompressionMethodMode &options);
			~CAddCommon();
			HRESULT Set_Pre_CompressionResult(bool seqMode, uint64 unpackSize, CCompressingResult &opRes) const;
			HRESULT Compress(DECL_EXTERNAL_CODECS_LOC_VARS ISequentialInStream * inStream, IOutStream * outStream,
				bool seqMode, uint32 fileTime, ICompressProgressInfo * progress, CCompressingResult &opRes);
		};

		class CItemOut : public CItem {
		public:
			// It's possible that NtfsTime is not defined, but there is NtfsTime in Extra.
			CItemOut() : NtfsTimeIsDefined(false) 
			{
			}
			FILETIME Ntfs_MTime;
			FILETIME Ntfs_ATime;
			FILETIME Ntfs_CTime;
			bool NtfsTimeIsDefined;
		};

		// COutArchive can throw CSystemException and COutBufferException

		class COutArchive {
			COutBuffer m_OutBuffer;
			CMyComPtr<IOutStream> m_Stream;
			uint64 m_Base; // Base of archive (offset in output Stream)
			uint64 m_CurPos; // Curent position in archive (relative from m_Base)
			uint64 m_LocalHeaderPos; // LocalHeaderPos (relative from m_Base) for last WriteLocalHeader() call
			uint32 m_LocalFileHeaderSize;
			uint32 m_ExtraSize;
			bool m_IsZip64;

			void WriteBytes(const void * data, size_t size);
			void FASTCALL Write8(Byte b);
			void FASTCALL Write16(uint16 val);
			void FASTCALL Write32(uint32 val);
			void FASTCALL Write64(uint64 val);
			void FASTCALL WriteNtfsTime(const FILETIME &ft);
			void FASTCALL WriteExtra(const CExtraBlock &extra);
			void WriteCommonItemInfo(const CLocalItem &item, bool isZip64);
			void WriteCentralHeader(const CItemOut &item);
			void SeekToCurPos();
		public:
			HRESULT Create(IOutStream * outStream);
			uint64 GetCurPos() const { return m_CurPos; }
			void FASTCALL MoveCurPos(uint64 distanceToMove);
			void WriteLocalHeader(CItemOut &item, bool needCheck = false);
			void WriteLocalHeader_Replace(CItemOut &item);
			void WriteDescriptor(const CItemOut &item);
			void WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer * comment);
			void CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream);
			void CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream);
		};

		class CItemEx : public CItem {
		public:
			CItemEx();
			uint64 GetLocalFullSize() const;
			uint64 GetDataPosition() const;
			uint32 LocalFullHeaderSize; // including Name and Extra
			bool DescriptorWasRead;
		};

		class CInArchive {
		public:
			struct CCdInfo {
				CCdInfo();
				bool   IsEmptyArc() const;
				void   ParseEcd32(const Byte * p); // (p) includes signature
				void   ParseEcd64e(const Byte * p); // (p) exclude signature

				bool   IsFromEcd64;
				uint16 CommentSize;
				// 64
				uint16 VersionMade;
				uint16 VersionNeedExtract;
				// old zip
				uint32 ThisDisk;
				uint32 CdDisk;
				uint64 NumEntries_in_ThisDisk;
				uint64 NumEntries;
				uint64 Size;
				uint64 Offset;
			};
			CInArchive();
			uint64 GetPhySize() const;
			uint64 GetOffset() const;
			void   ClearRefs();
			void   Close();
			HRESULT Open(IInStream * stream, const uint64 * searchLimit, IArchiveOpenCallback * callback, CObjectVector<CItemEx> &items);
			bool   IsOpen() const { return IsArcOpen; }
			bool   AreThereErrors() const;
			bool   IsLocalOffsetOK(const CItemEx &item) const;
			uint64 GetEmbeddedStubSize() const;
			HRESULT CheckDescriptor(const CItemEx &item);
			HRESULT ReadLocalItemAfterCdItem(CItemEx &item, bool &isAvail, bool &headersError);
			HRESULT ReadLocalItemAfterCdItemFull(CItemEx &item);
			HRESULT GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr <ISequentialInStream> &stream);
			IInStream * GetBaseStream() { return StreamRef; }
			bool   CanUpdate() const;

			struct CInArchiveInfo {
				CInArchiveInfo();
				void   Clear();
				int64 Base; /* Base offset of start of archive in stream.
						Offsets in headers must be calculated from that Base.
						Base is equal to MarkerPos for normal ZIPs.
						Base can point to PE stub for some ZIP SFXs.
						if CentralDir was read,
						 Base can be negative, if start of data is not available,
						if CentralDirs was not read,
						 Base = ArcInfo.MarkerPos; */

				/* The following *Pos variables contain absolute offsets in Stream */
				uint64 MarkerPos; // Pos of first signature, it can point to kSpan/kNoSpan signature
					// = MarkerPos2      in most archives; = MarkerPos2 - 4  if there is kSpan/kNoSpan signature 
				uint64 MarkerPos2; // Pos of first local item signature in stream
				uint64 FinishPos; // Finish pos of archive data in starting volume
				uint64 FileEndPos; // Finish pos of stream
				uint64 FirstItemRelatOffset; // Relative offset of first local (read from cd) (relative to Base).
					// = 0 in most archives; = size of stub for some SFXs 
				int    MarkerVolIndex;
				bool   CdWasRead;
				bool   IsSpanMode;
				bool   ThereIsTail;
				// uint32 BaseVolIndex;
				CByteBuffer Comment;
			};
			struct CVols {
				void   ClearRefs();
				void   Clear();
				HRESULT ParseArcName(IArchiveOpenVolumeCallback * volCallback);
				HRESULT Read(void * data, uint32 size, uint32 * processedSize);

				struct CSubStreamInfo {
					CSubStreamInfo();
					HRESULT SeekToStart() const;

					CMyComPtr <IInStream> Stream;
					uint64 Size;
				};
				CObjectVector <CSubStreamInfo> Streams;
				int    StreamIndex; // -1 for StartStream
					// -2 for ZipStream at multivol detection code
					// >=0 volume index in multivol
				bool   NeedSeek;
				bool   StartIsExe; // is .exe
				bool   StartIsZ; // is .zip or .zNN
				bool   StartIsZip; // is .zip
				bool   IsUpperCase;
				bool   MissingZip;
				bool   ecd_wasRead;
				int32  StartVolIndex; // -1, if unknown vol index
					// = (NN - 1), if StartStream is .zNN
					// = 0, if start vol is exe
				int32  StartParsingVol; // if we need local parsing, we must use that stream
				uint   NumVols;
				int    EndVolIndex; // index of last volume (ecd volume),
					// -1, if is not multivol
				UString BaseName; // name of archive including '.'
				UString MissingName;
				CMyComPtr <IInStream> ZipStream;
				CCdInfo ecd;
				uint64 TotalBytesSize; // for MultiVol only
			};
			CInArchiveInfo ArcInfo;
			bool   IsArc;
			bool   IsZip64;
			bool   HeadersError;
			bool   HeadersWarning;
			bool   ExtraMinorError;
			bool   UnexpectedEnd;
			bool   LocalsWereRead;
			bool   LocalsCenterMerged;
			bool   NoCentralDir;
			bool   Overflow32bit; // = true, if zip without Zip64 extension support and it has some fields values truncated to 32-bits.
			bool   Cd_NumEntries_Overflow_16bit; // = true, if no Zip64 and 16-bit ecd:NumEntries was overflowed.
			bool   MarkerIsFound;
			bool   MarkerIsSafe;
			bool   IsMultiVol;
			bool   UseDisk_in_SingleVol;
			uint32 EcdVolIndex;
			CVols  Vols;
		private:
			size_t GetAvail() const { return _bufCached - _bufPos; }
			void   InitBuf() { _bufPos = 0; _bufCached = 0; }
			void   DisableBufMode() { InitBuf(); _inBufMode = false; }
			void   SkipLookahed(size_t skip)
			{
				_bufPos += skip;
				_cnt += skip;
			}
			uint64 GetVirtStreamPos() { return _streamPos - _bufCached + _bufPos; }
			HRESULT Seek_SavePos(uint64 offset);
			HRESULT SeekToVol(int volIndex, uint64 offset);
			HRESULT ReadFromCache(Byte * data, uint size, unsigned &processed);
			HRESULT ReadVols2(IArchiveOpenVolumeCallback * volCallback, unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols);
			HRESULT ReadVols();
			HRESULT FindMarker(const uint64 * searchLimit);
			HRESULT IncreaseRealPosition(uint64 addValue, bool &isFinished);
			HRESULT LookAhead(size_t minRequiredInBuffer);
			void   FASTCALL SafeRead(Byte * data, uint size);
			void   ReadBuffer(CByteBuffer &buffer, uint size);
			// Byte ReadByte();
			// uint16 ReadUInt16();
			uint32 ReadUInt32();
			uint64 ReadUInt64();
			void   ReadSignature();
			void   FASTCALL Skip(size_t num);
			HRESULT Skip64(uint64 num, unsigned numFiles);
			bool   ReadFileName(unsigned nameSize, AString &dest);
			bool   ReadExtra(unsigned extraSize, CExtraBlock &extra, uint64 &unpackSize, uint64 &packSize, uint64 &localOffset, uint32 &disk);
			bool   ReadLocalItem(CItemEx &item);
			HRESULT FindDescriptor(CItemEx &item, unsigned numFiles);
			HRESULT ReadCdItem(CItemEx &item);
			HRESULT TryEcd64(uint64 offset, CCdInfo &cdInfo);
			HRESULT FindCd(bool checkOffsetMode);
			HRESULT TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, uint64 cdOffset, uint64 cdSize);
			HRESULT ReadCd(CObjectVector<CItemEx> &items, uint32 &cdDisk, uint64 &cdOffset, uint64 &cdSize);
			HRESULT ReadLocals(CObjectVector<CItemEx> &localItems);
			HRESULT ReadHeaders(CObjectVector<CItemEx> &items);
			HRESULT GetVolStream(unsigned vol, uint64 pos, CMyComPtr<ISequentialInStream> &stream);

			CMidBuffer Buffer;
			size_t _bufPos;
			size_t _bufCached;
			uint64 _streamPos;
			uint64 _cnt;
			bool   _inBufMode;
			bool   IsArcOpen;
			bool   CanStartNewVol;
			uint32 _signature;
			CMyComPtr<IInStream> StreamRef;
			IInStream * Stream;
			IInStream * StartStream;
			IArchiveOpenCallback * Callback;
		};

		const uint kNumMethodNames1 = NFileHeader::NCompressionMethod::kLZMA + 1;
		const uint kMethodNames2Start = NFileHeader::NCompressionMethod::kXz;
		const uint kNumMethodNames2 = NFileHeader::NCompressionMethod::kWzAES + 1 - kMethodNames2Start;

		extern const char * const kMethodNames1[kNumMethodNames1];
		extern const char * const kMethodNames2[kNumMethodNames2];

		class CHandler : public IInArchive, public IOutArchive, public ISetProperties, PUBLIC_ISetCompressCodecsInfo public CMyUnknownImp {
		public:
			MY_QUERYINTERFACE_BEGIN2(IInArchive)
			MY_QUERYINTERFACE_ENTRY(IOutArchive)
			MY_QUERYINTERFACE_ENTRY(ISetProperties)
			QUERY_ENTRY_ISetCompressCodecsInfo
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE
			INTERFACE_IInArchive(; )
			INTERFACE_IOutArchive(; )
			STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps);
			DECL_ISetCompressCodecsInfo CHandler();
		private:
			CObjectVector<CItemEx> m_Items;
			CInArchive m_Archive;
			CBaseProps _props;
			int    m_MainMethod;
			bool   m_ForceAesMode;
			bool   m_WriteNtfsTimeExtra;
			bool   _removeSfxBlock;
			bool   m_ForceLocal;
			bool   m_ForceUtf8;
			bool   _forceCodePage;
			uint8  Reserve[2]; // @alignment
			uint32 _specifiedCodePage;

			DECL_EXTERNAL_CODECS_VARS
			void InitMethodProps();
		};
		/*
		   struct CUpdateRange
		   {
		   uint64 Position;
		   uint64 Size;

		   // CUpdateRange() {};
		   CUpdateRange(uint64 position, uint64 size): Position(position), Size(size) {};
		   };
		 */
		struct CUpdateItem {
			CUpdateItem();
			void   Clear();

			bool   NewData;
			bool   NewProps;
			bool   IsDir;
			bool   NtfsTimeIsDefined;
			bool   IsUtf8;
			uint8  Reserve[3]; // @alignment
			int    IndexInArc;
			int    IndexInClient;
			uint32 Attrib;
			uint32 Time;
			uint64 Size;
			AString Name;
			CByteBuffer Comment;
			// bool Commented;
			// CUpdateRange CommentRange;
			FILETIME Ntfs_MTime;
			FILETIME Ntfs_ATime;
			FILETIME Ntfs_CTime;
		};

		HRESULT Update(DECL_EXTERNAL_CODECS_LOC_VARS const CObjectVector<CItemEx> &inputItems, CObjectVector<CUpdateItem> &updateItems,
			ISequentialOutStream * seqOutStream, CInArchive * inArchive, bool removeSfx, const CCompressionMethodMode &compressionMethodMode, IArchiveUpdateCallback * updateCallback);
	}
	namespace NCab {
		struct COtherArc {
			AString FileName;
			AString DiskName;
			void Clear();
		};

		struct CArchInfo {
			CArchInfo();
			void Clear();
			bool ReserveBlockPresent() const;
			bool IsTherePrev() const;
			bool IsThereNext() const;
			Byte GetDataBlockReserveSize() const;

			Byte VersionMinor; // cabinet file format version, minor
			Byte VersionMajor; // cabinet file format version, major
			uint32 NumFolders; // number of CFFOLDER entries in this cabinet
			uint32 NumFiles; // number of CFFILE entries in this cabinet
			uint32 Flags; // cabinet file option indicators
			uint32 SetID; // must be the same for all cabinets in a set
			uint32 CabinetNumber; // number of this cabinet file in a set
			uint16 PerCabinet_AreaSize; // (optional) size of per-cabinet reserved area
			Byte PerFolder_AreaSize; // (optional) size of per-folder reserved area
			Byte PerDataBlock_AreaSize; // (optional) size of per-datablock reserved area
			COtherArc PrevArc; // prev link can skip some volumes !!!
			COtherArc NextArc;
		};

		struct CDatabase {
			void Clear();
			bool IsTherePrevFolder() const;
			int GetNumberOfNewFolders() const;

			struct CInArcInfo : public CArchInfo {
				bool Parse(const Byte * p);

				uint32 Size; // size of this cabinet file in bytes
				uint32 FileHeadersOffset; // offset of the first CFFILE entry
			};
			CRecordVector<CFolder> Folders;
			CObjectVector<CItem> Items;
			uint64 StartPosition;
			CInArcInfo ArcInfo;
		};
		struct CDatabaseEx : public CDatabase {
			CMyComPtr <IInStream> Stream;
		};

		class CMvDatabaseEx {
		private:
			bool AreItemsEqual(unsigned i1, unsigned i2);
		public:
			struct CMvItem {
				uint   VolumeIndex;
				uint   ItemIndex;
			};
			int FASTCALL GetFolderIndex(const CMvItem * mvi) const;
			void Clear();
			void FillSortAndShrink();
			bool Check();

			CObjectVector <CDatabaseEx> Volumes;
			CRecordVector <CMvItem> Items;
			CRecordVector <int> StartFolderOfVol; // can be negative
			CRecordVector <uint> FolderStartFileIndex;
		};

		class CInArchive {
			CInBufferBase _inBuffer;
			CByteBuffer _tempBuf;
			void Skip(uint size);
			void Read(Byte * data, uint size);
			void ReadName(AString &s);
			void ReadOtherArc(COtherArc &oa);
			HRESULT Open2(CDatabaseEx &db, const uint64 * searchHeaderSizeLimit);
		public:
			bool IsArc;
			bool ErrorInNames;
			bool UnexpectedEnd;
			bool HeaderError;

			HRESULT Open(CDatabaseEx &db, const uint64 * searchHeaderSizeLimit);
		};

		class CHandler : public IInArchive, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP1(IInArchive)
			INTERFACE_IInArchive(; )
		private:
			CMvDatabaseEx m_Database;
			UString _errorMessage;
			bool _isArc;
			bool _errorInHeaders;
			bool _unexpectedEnd;
			// int _mainVolIndex;
			uint32 _phySize;
			uint64 _offset;
		};
	}
	namespace NTar {
		enum EErrorType {
			k_ErrorType_OK,
			k_ErrorType_Corrupted,
			k_ErrorType_UnexpectedEnd,
			k_ErrorType_Warning
		};

		HRESULT ReadItem(ISequentialInStream * stream, bool &filled, CItemEx &itemInfo, EErrorType &error);
		API_FUNC_IsArc IsArc_Tar(const Byte * p, size_t size);

		class COutArchive {
		public:
			void Create(ISequentialOutStream * outStream);
			HRESULT WriteHeader(const CItem &item);
			HRESULT FillDataResidual(uint64 dataSize);
			HRESULT WriteFinishHeader();

			uint64 Pos;
		private:
			HRESULT WriteBytes(const void * data, uint size);
			HRESULT WriteHeaderReal(const CItem &item);

			CMyComPtr <ISequentialOutStream> m_Stream;
		};

		class CHandler : public IInArchive, public IArchiveOpenSeq, public IInArchiveGetStream, public ISetProperties, public IOutArchive, public CMyUnknownImp {
		public:
			CObjectVector <CItemEx> _items;
			CMyComPtr <IInStream> _stream;
			CMyComPtr <ISequentialInStream> _seqStream;
		private:
			uint32 _curIndex;
			CItemEx _latestItem;
			uint64 _phySize;
			uint64 _headersSize;
			EErrorType _error;
			uint32 _specifiedCodePage;
			uint32 _curCodePage;
			uint32 _openCodePage;
			NCompress::CCopyCoder * copyCoderSpec;
			CMyComPtr <ICompressCoder> copyCoder;
			bool   _latestIsRead;
			bool   _phySizeDefined;
			bool   _warning;
			bool   _isArc;
			bool   _thereIsPaxExtendedHeader;
			bool   _forceCodePage;
			// bool _isSparse;
			uint8  Reserve[2]; // @alignment

			HRESULT ReadItem2(ISequentialInStream * stream, bool &filled, CItemEx &itemInfo);
			HRESULT Open2(IInStream * stream, IArchiveOpenCallback * callback);
			HRESULT SkipTo(uint32 index);
			void TarStringToUnicode(const AString &s, NWindows::NCOM::CPropVariant &prop, bool toOs = false) const;
		public:
			MY_UNKNOWN_IMP5(IInArchive, IArchiveOpenSeq, IInArchiveGetStream, ISetProperties, IOutArchive)
			INTERFACE_IInArchive(; )
			INTERFACE_IOutArchive(; )
			STDMETHOD(OpenSeq) (ISequentialInStream *stream);
			STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream);
			STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps);

			void Init();
			CHandler();
		};

		struct CUpdateItem {
			CUpdateItem() : Size(0), IsDir(false) 
			{
			}
			int IndexInArc;
			int IndexInClient;
			uint64 Size;
			int64 MTime;
			uint32 Mode;
			bool NewData;
			bool NewProps;
			bool IsDir;
			AString Name;
			AString User;
			AString Group;
		};

		HRESULT UpdateArchive(IInStream * inStream, ISequentialOutStream * outStream, const CObjectVector<CItemEx> &inputItems,
			const CObjectVector<CUpdateItem> &updateItems, UINT codePage, IArchiveUpdateCallback * updateCallback);
	}
}
//
//#include <IFileExtractCallback.h>
/* ---------- IFolderArchiveExtractCallback ----------
   is implemented by
   Console/ExtractCallbackConsole.h  CExtractCallbackConsole
   FileManager/ExtractCallback.h     CExtractCallbackImp
   FAR/ExtractEngine.cpp             CExtractCallBackImp: (QueryInterface is not supported)

   IID_IFolderArchiveExtractCallback is requested by:
   - Agent/ArchiveFolder.cpp
      CAgentFolder::CopyTo(..., IFolderOperationsExtractCallback *callback)
      is sent to IArchiveFolder::Extract()

   - FileManager/PanelCopy.cpp
      CPanel::CopyTo(), if(options->testMode)
      is sent to IArchiveFolder::Extract()

   IFolderArchiveExtractCallback is used by Common/ArchiveExtractCallback.cpp
 */

#define INTERFACE_IFolderArchiveExtractCallback(x) \
	STDMETHOD(AskOverwrite) ( \
	    const wchar_t * existName, const FILETIME *existTime, const uint64 *existSize, \
	    const wchar_t * newName, const FILETIME *newTime, const uint64 *newSize, \
	    int32 *answer)x; \
	STDMETHOD(PrepareOperation) (const wchar_t * name, int32 isFolder, int32 askExtractMode, const uint64 *position)x; \
	STDMETHOD(MessageError) (const wchar_t* message)x; \
	STDMETHOD(SetOperationResult) (int32 opRes, int32 encrypted)x; \

DECL_INTERFACE_SUB(IFolderArchiveExtractCallback, IProgress, 0x01, 0x07) { INTERFACE_IFolderArchiveExtractCallback(PURE) };

#define INTERFACE_IFolderArchiveExtractCallback2(x) STDMETHOD(ReportExtractResult) (int32 opRes, int32 encrypted, const wchar_t * name)x;

DECL_INTERFACE_SUB(IFolderArchiveExtractCallback2, IUnknown, 0x01, 0x08) { INTERFACE_IFolderArchiveExtractCallback2(PURE) };

/* ---------- IExtractCallbackUI ----------
   is implemented by
   Console/ExtractCallbackConsole.h  CExtractCallbackConsole
   FileManager/ExtractCallback.h     CExtractCallbackImp
 */

#ifdef _NO_CRYPTO
  #define INTERFACE_IExtractCallbackUI_Crypto(x)
#else
  #define INTERFACE_IExtractCallbackUI_Crypto(x) virtual HRESULT SetPassword(const UString &password) x;
#endif

#define INTERFACE_IExtractCallbackUI(x)	\
	virtual HRESULT BeforeOpen(const wchar_t * name, bool testMode) x; \
	virtual HRESULT OpenResult(const CCodecs *codecs, const CArchiveLink &arcLink, const wchar_t * name, HRESULT result) x;	\
	virtual HRESULT ThereAreNoFiles() x; \
	virtual HRESULT ExtractResult(HRESULT result) x; \
	INTERFACE_IExtractCallbackUI_Crypto(x)

struct IExtractCallbackUI : IFolderArchiveExtractCallback { INTERFACE_IExtractCallbackUI(PURE) };

#define INTERFACE_IGetProp(x) STDMETHOD(GetProp) (PROPID propID, PROPVARIANT *value)x; \

DECL_INTERFACE_SUB(IGetProp, IUnknown, 0x01, 0x20) { INTERFACE_IGetProp(PURE) };

#define INTERFACE_IFolderExtractToStreamCallback(x) \
	STDMETHOD(UseExtractToStream) (int32 *res)x; \
	STDMETHOD(GetStream7) (const wchar_t * name, int32 isDir, ISequentialOutStream **outStream, int32 askExtractMode, IGetProp * getProp)x; \
	STDMETHOD(PrepareOperation7) (int32 askExtractMode)x; \
	STDMETHOD(SetOperationResult7) (int32 resultEOperationResult, int32 encrypted)x; \

DECL_INTERFACE_SUB(IFolderExtractToStreamCallback, IUnknown, 0x01, 0x30) { INTERFACE_IFolderExtractToStreamCallback(PURE) };
//
//#include <ArchiveExtractCallback.h>
#ifndef _SFX
class COutStreamWithHash : public ISequentialOutStream, public CMyUnknownImp {
	CMyComPtr <ISequentialOutStream> _stream;
	uint64 _size;
	bool _calculate;
public:
	IHashCalc * _hash;

	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	void SetStream(ISequentialOutStream * stream);
	void ReleaseStream();
	void Init(bool calculate = true);
	void EnableCalc(bool calculate);
	void InitCRC();
	uint64 GetSize() const { return _size; }
};
#endif

#ifndef _SFX
class CGetProp : public IGetProp, public CMyUnknownImp {
public:
	const CArc * Arc;
	uint32 IndexInArc;
	// UString Name; // relative path
	MY_UNKNOWN_IMP1(IGetProp)
	INTERFACE_IGetProp(; )
};
#endif
#ifndef _SFX
	#ifndef UNDER_CE
		#define SUPPORT_LINKS
	#endif
#endif

#ifdef SUPPORT_LINKS
	struct CHardLinkNode {
		int    Compare(const CHardLinkNode &a) const;
		uint64 StreamId;
		uint64 INode;
	};

	class CHardLinks {
	public:
		void   Clear();
		void   PrepareLinks();

		CRecordVector<CHardLinkNode> IDs;
		CObjectVector<FString> Links;
	};
#endif
#ifdef SUPPORT_ALT_STREAMS
	struct CIndexToPathPair {
		uint32 Index;
		FString Path;

		CIndexToPathPair(uint32 index) : Index(index) 
		{
		}
		CIndexToPathPair(uint32 index, const FString &path) : Index(index), Path(path) 
		{
		}
		int Compare(const CIndexToPathPair &pair) const { return MyCompare(Index, pair.Index); }
	};
#endif

class CArchiveExtractCallback : public IArchiveExtractCallback, public IArchiveExtractCallbackMessage, public ICryptoGetTextPassword,
	public ICompressProgressInfo, public CMyUnknownImp {
	const CArc * _arc;
	CExtractNtOptions _ntOptions;
	const NWildcard::CCensorNode * _wildcardCensor; // we need wildcard for single pass mode (stdin)
	CMyComPtr<IFolderArchiveExtractCallback> _extractCallback2;
	CMyComPtr<ICompressProgressInfo> _compressProgress;
	CMyComPtr<ICryptoGetTextPassword> _cryptoGetTextPassword;
	CMyComPtr<IArchiveExtractCallbackMessage> _callbackMessage;
	CMyComPtr<IFolderArchiveExtractCallback2> _folderArchiveExtractCallback2;
	FString _dirPathPrefix;
	FString _dirPathPrefix_Full;
	NExtract::NPathMode::EEnum _pathMode;
	NExtract::NOverwriteMode::EEnum _overwriteMode;
  #ifndef _SFX
	CMyComPtr<IFolderExtractToStreamCallback> ExtractToStreamCallback;
	CGetProp * GetProp_Spec;
	CMyComPtr<IGetProp> GetProp;
  #endif
	CReadArcItem _item;
	FString _diskFilePath;
	uint64 _position;
	bool _isSplit;
	bool _extractMode;
	bool WriteCTime;
	bool WriteATime;
	bool WriteMTime;
	bool _encrypted;
	struct CProcessedFileInfo {
		FILETIME CTime;
		FILETIME ATime;
		FILETIME MTime;
		uint32 Attrib;
		bool CTimeDefined;
		bool ATimeDefined;
		bool MTimeDefined;
		bool AttribDefined;
	} _fi;
	uint32 _index;
	uint64 _curSize;
	bool _curSizeDefined;
	bool _fileLengthWasSet;
	COutFileStream * _outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;
  #ifndef _SFX
	COutStreamWithHash * _hashStreamSpec;
	CMyComPtr<ISequentialOutStream> _hashStream;
	bool _hashStreamWasUsed;
  #endif
	bool _removePartsForAltStreams;
	UStringVector _removePathParts;
  #ifndef _SFX
	bool _use_baseParentFolder_mode;
	uint32 _baseParentFolder;
  #endif
	bool _stdOutMode;
	bool _testMode;
	bool _multiArchives;
	CMyComPtr<ICompressProgressInfo> _localProgress;
	uint64 _packTotal;
	uint64 _progressTotal;
	bool _progressTotal_Defined;
	FStringVector _extractedFolderPaths;
	CRecordVector<uint32> _extractedFolderIndices;
  #if defined(_WIN32) && !defined(UNDER_CE) && !defined(_SFX)
	bool _saclEnabled;
  #endif
	void CreateComplexDirectory(const UStringVector &dirPathParts, FString &fullPath);
	HRESULT GetTime(int index, PROPID propID, FILETIME &filetime, bool &filetimeIsDefined);
	HRESULT GetUnpackSize();
	HRESULT SendMessageError(const char * message, const FString &path);
	HRESULT SendMessageError_with_LastError(const char * message, const FString &path);
	HRESULT SendMessageError2(const char * message, const FString &path1, const FString &path2);
public:
	CLocalProgress * LocalProgressSpec;
	uint64 NumFolders;
	uint64 NumFiles;
	uint64 NumAltStreams;
	uint64 UnpackSize;
	uint64 AltStreams_UnpackSize;

	MY_UNKNOWN_IMP3(IArchiveExtractCallbackMessage, ICryptoGetTextPassword, ICompressProgressInfo)
	INTERFACE_IArchiveExtractCallback(; )
	INTERFACE_IArchiveExtractCallbackMessage(; )
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
	STDMETHOD(CryptoGetTextPassword) (BSTR *password);
	CArchiveExtractCallback();
	void InitForMulti(bool multiArchives, NExtract::NPathMode::EEnum pathMode, NExtract::NOverwriteMode::EEnum overwriteMode);
#ifndef _SFX
	void FASTCALL SetHashMethods(IHashCalc * hash);
	void FASTCALL SetBaseParentFolderIndex(uint32 indexInArc);
#endif
	void Init(const CExtractNtOptions &ntOptions, const NWildcard::CCensorNode * wildcardCensor, const CArc * arc,
	    IFolderArchiveExtractCallback * extractCallback2, bool stdOutMode, bool testMode, const FString &directoryPath,
	    const UStringVector &removePathParts, bool removePartsForAltStreams, uint64 packSize);
  #ifdef SUPPORT_LINKS
private:
	CHardLinks _hardLinks;
	UString linkPath;
	// FString _CopyFile_Path;
	// HRESULT MyCopyFile(ISequentialOutStream *outStream);
public:
	// call PrepareHardLinks() after Init()
	HRESULT PrepareHardLinks(const CRecordVector<uint32> * realIndices); // NULL means all items
  #endif
#ifdef SUPPORT_ALT_STREAMS
	CObjectVector<CIndexToPathPair> _renamedFiles;
#endif
	// call it after Init()
	HRESULT SetDirsTimes();
};

bool CensorNode_CheckPath(const NWildcard::CCensorNode &node, const CReadArcItem &item);
//
//#include <Extract.h>
struct CExtractOptionsBase {
	CExtractOptionsBase() : PathMode_Force(false), OverwriteMode_Force(false),
		PathMode(NExtract::NPathMode::kFullPaths), OverwriteMode(NExtract::NOverwriteMode::kAsk)
	{
	}
	CBoolPair ElimDup;
	bool   PathMode_Force;
	bool   OverwriteMode_Force;
	uint8  Reserve[2]; // @alignment
	NExtract::NPathMode::EEnum PathMode;
	NExtract::NOverwriteMode::EEnum OverwriteMode;
	FString OutputDir;
	CExtractNtOptions NtOptions;
};

struct CExtractOptions : public CExtractOptionsBase {
	bool StdInMode;
	bool StdOutMode;
	bool YesToAll;
	bool TestMode;
	// bool ShowDialog;
	// bool PasswordEnabled;
	// UString Password;
#ifndef _SFX
	CObjectVector <CProperty> Properties;
#endif
#ifdef EXTERNAL_CODECS
	CCodecs * Codecs;
#endif
	CExtractOptions() : TestMode(false), StdInMode(false), StdOutMode(false), YesToAll(false)
	{
	}
};

struct CDecompressStat {
	void Clear()
	{
		NumArchives = UnpackSize = AltStreams_UnpackSize = PackSize = NumFolders = NumFiles = NumAltStreams = 0;
	}
	uint64 NumArchives;
	uint64 UnpackSize;
	uint64 AltStreams_UnpackSize;
	uint64 PackSize;
	uint64 NumFolders;
	uint64 NumFiles;
	uint64 NumAltStreams;
};

HRESULT Extract(CCodecs *codecs, const CObjectVector <COpenType> &types, const CIntVector &excludedFormats,
    UStringVector &archivePaths, UStringVector &archivePathsFull, const NWildcard::CCensorNode &wildcardCensor,
    const CExtractOptions &options, IOpenCallbackUI *openCallback, IExtractCallbackUI *extractCallback,
    #ifndef _SFX
		IHashCalc *hash,
    #endif
    UString &errorMessage, CDecompressStat &st);
//
//#include <Update.h>

struct CUpdateArchiveCommand {
	UString UserArchivePath;
	CArchivePath ArchivePath;
	NUpdateArchive::CActionSet ActionSet;
};

struct CCompressionMethodMode {
	CCompressionMethodMode() : Type_Defined(false) 
	{
	}
	bool Type_Defined;
	COpenType Type;
	CObjectVector<CProperty> Properties;
};

struct CRenamePair {
	CRenamePair();
	bool   Prepare();
	bool   GetNewPath(bool isFolder, const UString &src, UString &dest) const;

	UString OldName;
	UString NewName;
	NRecursedType::EEnum RecursedType;
	bool   WildcardParsing;
	uint8  Reserve[3]; // @alignment
};

struct CUpdateOptions {
	CUpdateOptions();
	void SetActionCommand_Add();
	bool InitFormatIndex(const CCodecs * codecs, const CObjectVector<COpenType> &types, const UString &arcPath);
	bool SetArcPath(const CCodecs * codecs, const UString &arcPath);

	CCompressionMethodMode MethodMode;
	CObjectVector<CUpdateArchiveCommand> Commands;
	CArchivePath ArchivePath;
	EArcNameMode ArcNameMode;
	FString SfxModule;
	UString StdInFileName;
	UString EMailAddress;
	FString WorkingDir;
	NWildcard::ECensorPathMode PathMode;
	UString AddPathPrefix;
	CBoolPair NtSecurity;
	CBoolPair AltStreams;
	CBoolPair HardLinks;
	CBoolPair SymLinks;
	CObjectVector <CRenamePair> RenamePairs;
	CRecordVector <uint64> VolumesSizes;
	bool   UpdateArchiveItself;
	bool   SfxMode;
	bool   OpenShareForWrite;
	bool   StdInMode;
	bool   StdOutMode;
	bool   EMailMode;
	bool   EMailRemoveAfter;
	bool   DeleteAfterCompressing;
	bool   SetArcMTime;
	uint8  Reserve[3]; // @alignment
};

struct CUpdateErrorInfo {
	CUpdateErrorInfo();
	bool ThereIsError() const;
	HRESULT Get_HRESULT_Error() const;
	void SetFromLastError(const char * message);
	HRESULT SetFromLastError(const char * message, const FString &fileName);

	DWORD SystemError;
	AString Message;
	FStringVector FileNames;
};

struct CFinishArchiveStat {
	CFinishArchiveStat() : OutArcFileSize(0) 
	{
	}
	uint64 OutArcFileSize;
};

#define INTERFACE_IUpdateCallbackUI2(x)	\
	INTERFACE_IUpdateCallbackUI(x) \
	INTERFACE_IDirItemsCallback(x) \
	virtual HRESULT OpenResult(const CCodecs *codecs, const CArchiveLink &arcLink, const wchar_t * name, HRESULT result) x;	\
	virtual HRESULT StartScanning() x; \
	virtual HRESULT FinishScanning(const CDirItemsStat &st) x; \
	virtual HRESULT StartOpenArchive(const wchar_t * name) x; \
	virtual HRESULT StartArchive(const wchar_t * name, bool updating) x; \
	virtual HRESULT FinishArchive(const CFinishArchiveStat &st) x; \
	virtual HRESULT DeletingAfterArchiving(const FString &path, bool isDir) x; \
	virtual HRESULT FinishDeletingAfterArchiving() x; \

struct IUpdateCallbackUI2 : public IUpdateCallbackUI, public IDirItemsCallback {
	INTERFACE_IUpdateCallbackUI2( = 0)
};

HRESULT UpdateArchive(CCodecs * codecs, const CObjectVector<COpenType> &types, const UString &cmdArcPath2, NWildcard::CCensor &censor,
    CUpdateOptions &options, CUpdateErrorInfo &errorInfo, IOpenCallbackUI * openCallback, IUpdateCallbackUI2 * callback, bool needSetPath);
//
//#include <UpdateCallbackConsole.h>

class CCallbackConsoleBase {
public:
	struct CErrorPathCodes {
		void AddError(const FString &path, DWORD systemError);
		void Clear();

		FStringVector Paths;
		CRecordVector <DWORD> Codes;
	};
	CCallbackConsoleBase();
	bool   NeedPercents() const;
	void   SetWindowWidth(uint width);
	void   Init(CStdOutStream * outStream, CStdOutStream * errorStream, CStdOutStream * percentStream);
	void   ClosePercents2();
	void   ClosePercents_for_so();
	HRESULT PrintProgress(const wchar_t * name, const char * command, bool showInLog);

	bool   StdOutMode;
	bool   NeedFlush;
	uint8  Reserve[2]; // @alignment
	uint   PercentsNameLevel;
	uint   LogLevel;
	AString _tempA;
	UString _tempU;
	CErrorPathCodes FailedFiles;
	CErrorPathCodes ScanErrors;
protected:
	void CommonError(const FString &path, DWORD systemError, bool isWarning);
	HRESULT ScanError_Base(const FString &path, DWORD systemError);
	HRESULT OpenFileError_Base(const FString &name, DWORD systemError);
	HRESULT ReadingFileError_Base(const FString &name, DWORD systemError);

	CPercentPrinter _percent;
	CStdOutStream * _so;
	CStdOutStream * _se;
};

class CUpdateCallbackConsole : public IUpdateCallbackUI2, public CCallbackConsoleBase {
	// void PrintPropPair(const char *name, const wchar_t *val);
public:
#ifndef _NO_CRYPTO
	UString Password;
	bool PasswordIsDefined;
	bool AskPassword;
	uint8 Reserve[1]; // @alignment
#else
	uint8 Reserve[3]; // @alignment
#endif
	bool DeleteMessageWasShown;
	CUpdateCallbackConsole();
	/*
	   void Init(CStdOutStream *outStream)
	   {
	   CCallbackConsoleBase::Init(outStream);
	   }
	 */
	// ~CUpdateCallbackConsole() { if(NeedPercents()) _percent.ClosePrint(); }
	INTERFACE_IUpdateCallbackUI2(; )
};
//
//#include <Rar1Decoder.h> <Rar2Decoder.h> <Rar3Decoder.h> <Rar5Decoder.h> <BranchMisc.h> <ZDecoder.h>

EXTERN_C_BEGIN
	typedef SizeT (*Func_Bra)(Byte * data, SizeT size, uint32 ip, int encoding);
EXTERN_C_END

namespace NCompress {
	namespace NBranch {
		class CCoder : public ICompressFilter, public CMyUnknownImp {
			uint32 _bufferPos;
			int _encode;
			Func_Bra BraFunc;
		public:
			MY_UNKNOWN_IMP1(ICompressFilter);
			INTERFACE_ICompressFilter(; ) CCoder(Func_Bra bra, int encode) :  _bufferPos(0), _encode(encode), BraFunc(bra) 
			{
			}
		};
	}
	namespace NRar1 {
		const uint32 kNumRepDists = 4;

		typedef NBitm::CDecoder <CInBuffer> CBitDecoder;

		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public CMyUnknownImp {
		public:
			CLzOutWindow m_OutWindowStream;
			CBitDecoder m_InBitStream;
			uint32 m_RepDists[kNumRepDists];
			uint32 m_RepDistPtr;
			uint32 LastDist;
			uint32 LastLength;
			int64 m_UnpackSize;
			bool m_IsSolid;

			uint32 ReadBits(int numBits);
			HRESULT CopyBlock(uint32 distance, uint32 len);

			uint32 DecodeNum(const uint32 * posTab);
			HRESULT ShortLZ();
			HRESULT LongLZ();
			HRESULT HuffDecode();
			void GetFlagsBuf();
			void InitData();
			void InitHuff();
			void CorrHuff(uint32 * CharSet, uint32 * NumToPlace);
			void OldUnpWriteBuf();

			uint32 ChSet[256], ChSetA[256], ChSetB[256], ChSetC[256];
			uint32 Place[256], PlaceA[256], PlaceB[256], PlaceC[256];
			uint32 NToPl[256], NToPlB[256], NToPlC[256];
			uint32 FlagBuf, AvrPlc, AvrPlcB, AvrLn1, AvrLn2, AvrLn3;
			int Buf60, NumHuf, StMode, LCount, FlagsCnt;
			uint32 Nhfb, Nlzb, MaxDist3;

			void InitStructures();
			HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
		public:
			CDecoder();
			MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)
			/*
			   void ReleaseStreams()
			   {
			   m_OutWindowStream.ReleaseStream();
			   m_InBitStream.ReleaseStream();
			   }
			 */
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
		};
	}
	namespace NRar2 {
		const uint kNumRepDists = 4;
		const uint kDistTableSize = 48;
		const uint kMMTableSize = 256 + 1;
		const uint32 kMainTableSize = 298;
		const uint32 kLenTableSize = 28;
		const uint32 kDistTableStart = kMainTableSize;
		const uint32 kLenTableStart = kDistTableStart + kDistTableSize;
		const uint32 kHeapTablesSizesSum = kMainTableSize + kDistTableSize + kLenTableSize;
		const uint32 kLevelTableSize = 19;
		const uint32 kMMTablesSizesSum = kMMTableSize * 4;
		const uint32 kMaxTableSize = kMMTablesSizesSum;
		const uint32 kTableDirectLevels = 16;
		const uint32 kTableLevelRepNumber = kTableDirectLevels;
		const uint32 kTableLevel0Number = kTableLevelRepNumber + 1;
		const uint32 kTableLevel0Number2 = kTableLevel0Number + 1;
		const uint32 kLevelMask = 0xF;
		const uint32 kRepBothNumber = 256;
		const uint32 kRepNumber = kRepBothNumber + 1;
		const uint32 kLen2Number = kRepNumber + 4;
		const uint32 kLen2NumNumbers = 8;
		const uint32 kReadTableNumber = kLen2Number + kLen2NumNumbers;
		const uint32 kMatchNumber = kReadTableNumber + 1;

		const Byte kLenStart[kLenTableSize] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
		const Byte kLenDirectBits[kLenTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
		const uint32 kDistStart   [kDistTableSize] =
			{0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192,
			12288, 16384, 24576, 32768U, 49152U, 65536, 98304, 131072, 196608, 262144, 327680, 393216, 458752, 524288, 589824,
			655360, 720896, 786432, 851968, 917504, 983040};
		const Byte kDistDirectBits[kDistTableSize] =
			{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,
			14,    14,   15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16, 16};
		const Byte kLevelDirectBits[kLevelTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7};
		const Byte kLen2DistStarts[kLen2NumNumbers] = {0, 4, 8, 16, 32, 64, 128, 192};
		const Byte kLen2DistDirectBits[kLen2NumNumbers] = {2, 2, 3, 4, 5, 6,  6,  6};
		const uint32 kDistLimit2 = 0x101 - 1;
		const uint32 kDistLimit3 = 0x2000 - 1;
		const uint32 kDistLimit4 = 0x40000 - 1;
		const uint32 kMatchMaxLen = 255 + 2;
		const uint32 kMatchMaxLenMax = 255 + 5;
		const uint32 kNormalMatchMinLen = 3;

		namespace NMultimedia {
			struct CFilter {
				int K1, K2, K3, K4, K5;
				int D1, D2, D3, D4;
				int LastDelta;
				uint32 Dif[11];
				uint32 ByteCount;
				int LastChar;

				Byte Decode(int &channelDelta, Byte delta);
				void Init() 
				{
					memzero(this, sizeof(*this));
				}
			};

			const uint kNumChanelsMax = 4;

			class CFilter2 {
			public:
				void Init() 
				{
					memzero(this, sizeof(*this));
				}
				Byte Decode(Byte delta)
				{
					return m_Filters[CurrentChannel].Decode(m_ChannelDelta, delta);
				}
				CFilter m_Filters[kNumChanelsMax];
				int    m_ChannelDelta;
				uint   CurrentChannel;
			};
		}

		typedef NBitm::CDecoder<CInBuffer> CBitDecoder;

		const uint kNumHuffmanBits = 15;

		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public CMyUnknownImp {
			CLzOutWindow m_OutWindowStream;
			CBitDecoder m_InBitStream;
			uint32 m_RepDistPtr;
			uint32 m_RepDists[kNumRepDists];
			uint32 m_LastLength;
			bool m_IsSolid;
			bool m_TablesOK;
			bool m_AudioMode;
			NHuffman::CDecoder <kNumHuffmanBits, kMainTableSize> m_MainDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kDistTableSize> m_DistDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLenTableSize> m_LenDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kMMTableSize> m_MMDecoders[NMultimedia::kNumChanelsMax];
			NHuffman::CDecoder <kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;
			uint64 m_PackSize;
			unsigned m_NumChannels;
			NMultimedia::CFilter2 m_MmFilter;
			Byte m_LastLevels[kMaxTableSize];

			void InitStructures();
			uint32 ReadBits(uint numBits);
			bool ReadTables();
			bool ReadLastTables();
			bool DecodeMm(uint32 pos);
			bool DecodeLz(int32 pos);
			HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
		public:
			CDecoder();
			MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)
			/*
			   void ReleaseStreams()
			   {
			   m_OutWindowStream.ReleaseStream();
			   m_InBitStream.ReleaseStream();
			   }
			 */
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
		};
	}
	namespace NRar3 {
		const uint32 kWindowSize = 1 << 22;
		const uint32 kWindowMask = (kWindowSize - 1);
		const uint32 kNumReps = 4;
		const uint32 kNumLen2Symbols = 8;
		const uint32 kLenTableSize = 28;
		const uint32 kMainTableSize = 256 + 1 + 1 + 1 + kNumReps + kNumLen2Symbols + kLenTableSize;
		const uint32 kDistTableSize = 60;
		const int kNumAlignBits = 4;
		const uint32 kAlignTableSize = (1 << kNumAlignBits) + 1;
		const uint32 kLevelTableSize = 20;
		const uint32 kTablesSizesSum = kMainTableSize + kDistTableSize + kAlignTableSize + kLenTableSize;

		class CBitDecoder {
		public:
			bool   FASTCALL Create(uint32 bufSize);
			void   FASTCALL SetStream(ISequentialInStream * inStream);
			void   Init();
			bool   ExtraBitsWereRead() const;
			uint64 GetProcessedSize() const;
			void   AlignToByte();
			uint32 FASTCALL GetValue(uint numBits);
			void   FASTCALL MovePos(uint numBits);
			uint32 FASTCALL ReadBits(uint numBits);

			CInBuffer Stream;
		private:
			uint32 _value;
			uint   _bitPos;
		};

		const uint32 kTopValue = (1 << 24);
		const uint32 kBot = (1 << 15);

		struct CRangeDecoder {
		public:
			CRangeDecoder() throw();
			void   InitRangeCoder();
			void   Normalize();

			IPpmd7_RangeDec vt;
			uint32 Range;
			uint32 Code;
			uint32 Low;
			CBitDecoder BitDecoder;
			SRes Res;
		};
		struct CFilter : public NVm::CProgram {
			CFilter() : BlockStart(0), BlockSize(0), ExecCount(0) 
			{
			}
			CRecordVector <Byte> GlobalData;
			uint32 BlockStart;
			uint32 BlockSize;
			uint32 ExecCount;
		};

		const int kNumHuffmanBits = 15;

		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public CMyUnknownImp {
			struct CTempFilter : public NVm::CProgramInitState {
				CTempFilter();
				uint32 BlockStart;
				uint32 BlockSize;
				uint32 FilterIndex;
				bool   NextWindow;
				uint8  Reserve[3]; // @alignment
			};
			CRangeDecoder m_InBitStream;
			Byte * _window;
			uint32 _winPos;
			uint32 _wrPtr;
			uint64 _lzSize;
			uint64 _unpackSize;
			uint64 _writtenFileSize; // if it's > _unpackSize, then _unpackSize only written
			ISequentialOutStream * _outStream;
			NHuffman::CDecoder <kNumHuffmanBits, kMainTableSize> m_MainDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kDistTableSize> m_DistDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kAlignTableSize> m_AlignDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLenTableSize> m_LenDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;
			uint32 _reps[kNumReps];
			uint32 _lastLength;
			Byte m_LastLevels[kTablesSizesSum];
			Byte * _vmData;
			Byte * _vmCode;
			NVm::CVm _vm;
			CRecordVector <CFilter *> _filters;
			CRecordVector <CTempFilter *>  _tempFilters;
			uint32 _lastFilter;
			uint32 PrevAlignBits;
			uint32 PrevAlignCount;
			CPpmd7 _ppmd;
			int    PpmEscChar;
			bool   m_IsSolid;
			bool   _lzMode;
			bool   _unsupportedFilter;
			bool   TablesRead;
			bool   TablesOK;
			bool   PpmError;
			uint8  Reserve[2]; // @alignment

			HRESULT WriteDataToStream(const Byte * data, uint32 size);
			HRESULT WriteData(const Byte * data, uint32 size);
			HRESULT WriteArea(uint32 startPtr, uint32 endPtr);
			void ExecuteFilter(int tempFilterIndex, NVm::CBlockRef &outBlockRef);
			HRESULT WriteBuf();
			void InitFilters();
			bool AddVmCode(uint32 firstByte, uint32 codeSize);
			bool ReadVmCodeLZ();
			bool ReadVmCodePPM();
			uint32 ReadBits(int numBits);
			HRESULT InitPPM();
			int DecodePpmSymbol();
			HRESULT DecodePPM(int32 num, bool &keepDecompressing);
			HRESULT ReadTables(bool &keepDecompressing);
			HRESULT ReadEndOfBlock(bool &keepDecompressing);
			HRESULT DecodeLZ(bool &keepDecompressing);
			HRESULT CodeReal(ICompressProgressInfo * progress);
			bool InputEofError() const { return m_InBitStream.BitDecoder.ExtraBitsWereRead(); }
			bool InputEofError_Fast() const { return (m_InBitStream.BitDecoder.Stream.NumExtraBytes > 2); }
		public:
			CDecoder();
			~CDecoder();
			MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
			void CopyBlock(uint32 distance, uint32 len);
			void FASTCALL PutByte(Byte b);
		};
	}
	namespace NRar5 {
		// struct CInBufferException: public CSystemException { CInBufferException(HRESULT errorCode): CSystemException(errorCode) {} };

		class CBitDecoder {
		public:
			const Byte * _buf;
			unsigned _bitPos;
			bool _wasFinished;
			Byte _blockEndBits7;
			const Byte * _bufCheck2;
			const Byte * _bufCheck;
			Byte * _bufLim;
			Byte * _bufBase;
			uint64 _processedSize;
			uint64 _blockEnd;
			ISequentialInStream * _stream;
			HRESULT _hres;

			void   SetCheck2();
			bool   IsBlockOverRead() const;
			//CBitDecoder() throw();
			void   Init() throw();
			void   Prepare2() throw();
			void   Prepare() throw();
			bool   ExtraBitsWereRead() const;
			bool   InputEofError() const;
			uint   GetProcessedBits7() const;
			uint64 GetProcessedSize_Round() const;
			uint64 GetProcessedSize() const;
			void   AlignToByte();
			Byte   ReadByteInAligned();
			uint32 FASTCALL GetValue(uint numBits);
			void   FASTCALL MovePos(uint numBits);
			uint32 FASTCALL ReadBits9(uint numBits);
			uint32 FASTCALL ReadBits9fix(uint numBits);
			uint32 FASTCALL ReadBits32(uint numBits);
		};

		struct CFilter {
			Byte   Type;
			Byte   Channels;
			uint32 Size;
			uint64 Start;
		};

		const uint kNumReps = 4;
		const uint kLenTableSize = 11 * 4;
		const uint kMainTableSize = 256 + 1 + 1 + kNumReps + kLenTableSize;
		const uint kDistTableSize = 64;
		const uint kNumAlignBits = 4;
		const uint kAlignTableSize = (1 << kNumAlignBits);
		const uint kLevelTableSize = 20;
		const uint kTablesSizesSum = kMainTableSize + kDistTableSize + kAlignTableSize + kLenTableSize;
		const uint kNumHuffmanBits = 15;

		class CDecoder : public ICompressCoder, public ICompressSetDecoderProperties2, public CMyUnknownImp {
			// CBitDecoder _bitStream;
			Byte * _window;
			size_t _winPos;
			size_t _winSize;
			size_t _winMask;
			uint64 _lzSize;
			uint   _numCorrectDistSymbols;
			uint   _numUnusedFilters;
			uint64 _lzWritten;
			uint64 _lzFileStart;
			uint64 _unpackSize;
			// uint64 _packSize;
			uint64 _lzEnd;
			uint64 _writtenFileSize;
			size_t _winSizeAllocated;
			uint32 _reps[kNumReps];
			uint32 _lastLen;
			uint64 _filterEnd;
			CMidBuffer _filterSrc;
			CMidBuffer _filterDst;
			CRecordVector<CFilter> _filters;
			ISequentialInStream * _inStream;
			ISequentialOutStream * _outStream;
			ICompressProgressInfo * _progress;
			Byte * _inputBuf;
			Byte   _dictSizeLog;
			bool   _useAlignBits;
			bool   _isLastBlock;
			bool   _unpackSize_Defined;
			// bool _packSize_Defined;
			bool   _unsupportedFilter;
			bool   _lzError;
			bool   _writeError;
			bool   _tableWasFilled;
			bool   _isSolid;
			bool   _wasInit;
			uint8  Reserve[2]; // @alignment

			NHuffman::CDecoder <kNumHuffmanBits, kMainTableSize> m_MainDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kDistTableSize> m_DistDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kAlignTableSize> m_AlignDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLenTableSize> m_LenDecoder;
			NHuffman::CDecoder <kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

			void InitFilters();
			void DeleteUnusedFilters();
			HRESULT WriteData(const Byte * data, size_t size);
			HRESULT ExecuteFilter(const CFilter &f);
			HRESULT WriteBuf();
			HRESULT AddFilter(CBitDecoder &_bitStream);
			HRESULT ReadTables(CBitDecoder &_bitStream);
			HRESULT DecodeLZ();
			HRESULT CodeReal();
		public:
			CDecoder();
			~CDecoder();
			MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			STDMETHOD(SetDecoderProperties2) (const Byte *data, uint32 size);
		};
	}
	namespace NZ {
		// Z decoder decodes Z data stream, including 3 bytes of header.
		class CDecoder : public ICompressCoder, public CMyUnknownImp {
			uint16 * _parents;
			Byte * _suffixes;
			Byte * _stack;
			unsigned _numMaxBits;
		public:
			CDecoder() : _parents(0), _suffixes(0), _stack(0), /* _prop(0), */ _numMaxBits(0) 
			{
			}
			~CDecoder();
			void Free();
			uint64 PackSize;
			MY_UNKNOWN_IMP1(ICompressCoder)
			HRESULT CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress);
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
		};
		/*
		   There is no end_of_payload_marker in Z stream.
		   Z decoder stops decoding, if it reaches end of input stream.

		   CheckStream function:
			(size) must be at least 3 bytes (size of Z header).
			if(size) is larger than size of real Z stream in (data), CheckStream can return false.
		 */
		const uint kRecommendedCheckSize = 64;
		bool CheckStream(const Byte * data, size_t size);
	}
}
//
//#include <HandlerCont.h> <Rar5Handler.h> <RarHandler.h>
namespace NArchive {
	#define INTERFACE_IInArchive_Cont(x) \
		STDMETHOD(Open) (IInStream *stream, const uint64 *maxCheckStartPosition, IArchiveOpenCallback *	\
			openCallback) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(Close) () MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetNumberOfItems) (uint32 *numItems) MY_NO_THROW_DECL_ONLY x;	\
		STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
			/* STDMETHOD(Extract)(const uint32* indices, uint32 numItems, int32 testMode, IArchiveExtractCallback
			  *extractCallback) MY_NO_THROW_DECL_ONLY x; */	\
		STDMETHOD(GetArchiveProperty) (PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetNumberOfProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetPropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetNumberOfArchiveProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetArchivePropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x;	\

	class CHandlerCont : public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
	protected:
		CMyComPtr<IInStream> _stream;
		virtual int GetItem_ExtractInfo(uint32 index, uint64 &pos, uint64 &size) const = 0;
	public:
		MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
		INTERFACE_IInArchive_Cont(PURE)
		STDMETHOD(Extract) (const uint32* indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback) MY_NO_THROW_DECL_ONLY;
		STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream);
		// destructor must be virtual for this class
		virtual ~CHandlerCont() 
		{
		}
	};

	#define INTERFACE_IInArchive_Img(x) \
			/* STDMETHOD(Open)(IInStream *stream, const uint64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback)
			  MY_NO_THROW_DECL_ONLY x; */ \
		STDMETHOD(Close) () MY_NO_THROW_DECL_ONLY x; \
			/* STDMETHOD(GetNumberOfItems)(uint32 *numItems) MY_NO_THROW_DECL_ONLY x; */ \
		STDMETHOD(GetProperty) (uint32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
			/* STDMETHOD(Extract)(const uint32* indices, uint32 numItems, int32 testMode, IArchiveExtractCallback
			  *extractCallback) MY_NO_THROW_DECL_ONLY x; */	\
		STDMETHOD(GetArchiveProperty) (PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetNumberOfProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetPropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetNumberOfArchiveProperties) (uint32 *numProps) MY_NO_THROW_DECL_ONLY x; \
		STDMETHOD(GetArchivePropertyInfo) (uint32 index, BSTR *name, PROPID *propID, VARTYPE *varType) MY_NO_THROW_DECL_ONLY x;	\

	class CHandlerImg : public IInStream, public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
	protected:
		uint64 _virtPos;
		uint64 _posInArc;
		uint64 _size;
		CMyComPtr<IInStream> Stream;
		const char * _imgExt;
		bool _stream_unavailData;
		bool _stream_unsupportedMethod;
		bool _stream_dataError;
		// bool _stream_UsePackSize;
		// uint64 _stream_PackSize;
		void ClearStreamVars();
		virtual HRESULT Open2(IInStream * stream, IArchiveOpenCallback * openCallback) = 0;
		virtual void CloseAtError();
	public:
		MY_UNKNOWN_IMP3(IInArchive, IInArchiveGetStream, IInStream)
		INTERFACE_IInArchive_Img(PURE)
		STDMETHOD(Open) (IInStream *stream, const uint64 *maxCheckStartPosition, IArchiveOpenCallback *openCallback);
		STDMETHOD(GetNumberOfItems) (uint32 *numItems);
		STDMETHOD(Extract) (const uint32* indices, uint32 numItems, int32 testMode, IArchiveExtractCallback *extractCallback);
		STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream) = 0;
		STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize) = 0;
		STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
		CHandlerImg();
		// destructor must be virtual for this class
		virtual ~CHandlerImg();
	};

	HRESULT ReadZeroTail(ISequentialInStream * stream, bool &areThereNonZeros, uint64 &numZeros, uint64 maxSize);

	namespace NRar5 {
		const uint kArcExtraRecordType_Locator = 1;
		const uint kCryptoAlgo_AES = 0;
		const uint kHashID_Blake2sp = 0;

		namespace NHeaderFlags {
			const uint kExtra   = 1 << 0;
			const uint kData    = 1 << 1;
			// const uint kUnknown = 1 << 2;
			const uint kPrevVol = 1 << 3;
			const uint kNextVol = 1 << 4;
			// const uint kIsChild = 1 << 5;
			// const uint kPreserveChild = 1 << 6;
		}
		namespace NHeaderType {
			enum {
				kArc = 1,
				kFile,
				kService,
				kArcEncrypt,
				kEndOfArc
			};
		}
		namespace NArcFlags {
			const uint kVol       = 1 << 0;
			const uint kVolNumber = 1 << 1;
			const uint kSolid     = 1 << 2;
			// const uint kRecovery  = 1 << 3;
			// const uint kLocked    = 1 << 4;
		}
		namespace NLocatorFlags {
			const uint kQuickOpen  = 1 << 0;
			const uint kRecovery   = 1 << 1;
		}
		namespace NFileFlags {
			const uint kIsDir    = 1 << 0;
			const uint kUnixTime = 1 << 1;
			const uint kCrc32    = 1 << 2;
			const uint kUnknownSize = 1 << 3;
		}
		namespace NMethodFlags {
			// const uint kVersionMask = 0x3F;
			const uint kSolid = 1 << 6;
		}
		namespace NArcEndFlags {
			const uint kMoreVols = 1 << 0;
		}
		namespace NExtraID {
			enum {
				kCrypto = 1,
				kHash,
				kTime,
				kVersion,
				kLink,
				kUnixOwner,
				kSubdata
			};
		}
		namespace NCryptoFlags {
			const uint kPswCheck = 1 << 0;
			const uint kUseMAC   = 1 << 1;
		}
		namespace NTimeRecord {
			enum {
				k_Index_MTime = 0,
				k_Index_CTime,
				k_Index_ATime
			};
			namespace NFlags {
				const uint kUnixTime = 1 << 0;
				const uint kMTime    = 1 << 1;
				const uint kCTime    = 1 << 2;
				const uint kATime    = 1 << 3;
				const uint kUnixNs   = 1 << 4;
			}
		}
		namespace NLinkType {
			enum {
				kUnixSymLink = 1,
				kWinSymLink,
				kWinJunction,
				kHardLink,
				kFileCopy
			};
		}
		namespace NLinkFlags {
			const uint kTargetIsDir = 1 << 0;
		}

		enum EHostOS {
			kHost_Windows = 0,
			kHost_Unix
		};

		struct CLinkInfo {
			bool   Parse(const Byte * p, uint size);
			uint64 Type;
			uint64 Flags;
			uint   NameOffset;
			uint   NameLen;
		};

		struct CItem {
			CItem();
			void   Clear();
			bool   IsSplitBefore()  const;
			bool   IsSplitAfter()   const;
			bool   IsSplit()        const;
			bool   IsDir()          const;
			bool   Has_UnixMTime()  const;
			bool   Has_CRC()        const;
			bool   Is_UnknownSize() const;
			bool   FASTCALL IsNextForItem(const CItem &prev) const;
			bool   IsSolid() const;
			uint   GetAlgoVersion() const;
			uint   GetMethod() const;
			uint32 GetDictSize() const;
			bool   IsService() const;
			bool   Is_STM() const;
			bool   Is_CMT() const;
			bool   Is_ACL() const;
			// bool Is_QO()  const { return IsService() && Name == "QO"; }
			int    FindExtra(unsigned extraID, unsigned &recordDataSize) const;
			void   PrintInfo(AString &s) const;
			bool   IsEncrypted() const;
			int    FindExtra_Blake() const;
			bool   FindExtra_Version(uint64 &version) const;
			bool   FindExtra_Link(CLinkInfo &link) const;
			void   Link_to_Prop(unsigned linkType, NWindows::NCOM::CPropVariant &prop) const;
			bool   Is_CopyLink() const;
			bool   Is_HardLink() const;
			bool   Is_CopyLink_or_HardLink() const;
			bool   NeedUse_as_CopyLink() const { return PackSize == 0 && Is_CopyLink(); }
			bool   NeedUse_as_HardLink() const { return PackSize == 0 && Is_HardLink(); }
			bool   NeedUse_as_CopyLink_or_HardLink() const { return PackSize == 0 && Is_CopyLink_or_HardLink(); }
			bool   GetAltStreamName(AString &name) const;
			uint32 GetWinAttrib() const;
			uint64 GetDataPosition() const { return DataPos; }

			uint32 CommonFlags;
			uint32 Flags;
			Byte   RecordType;
			bool   Version_Defined;
			uint8  Reserve[2]; // @alignment
			int ACL;
			AString Name;
			int VolIndex;
			int NextItem;
			uint32 UnixMTime;
			uint32 CRC;
			uint32 Attrib;
			uint32 Method;
			CByteBuffer Extra;
			uint64 Size;
			uint64 PackSize;
			uint64 HostOS;
			uint64 DataPos;
			uint64 Version;
		};

		struct CRefItem {
			uint   Item;
			uint   Last;
			int    Parent;
			int    Link;
		};

		struct CArc {
			struct CInArcInfo {
				uint64 Flags;
				uint64 VolNumber;
				uint64 StartPos;
				uint64 EndPos;
				uint64 EndFlags;
				bool   EndOfArchive_was_Read;
				bool   IsEncrypted;

				// CByteBuffer Extra;
				/*
				   struct CLocator {
					   uint64 Flags;
					   uint64 QuickOpen;
					   uint64 Recovery;
					   bool Is_QuickOpen() const { return (Flags & NLocatorFlags::kQuickOpen) != 0; }
					   bool Is_Recovery() const { return (Flags & NLocatorFlags::kRecovery) != 0; }
				   };
				   int FindExtra(unsigned extraID, unsigned &recordDataSize) const;
				   bool FindExtra_Locator(CLocator &locator) const;
				 */
				CInArcInfo();
				/*
				   void Clear()
				   {
				   Flags = 0;
				   VolNumber = 0;
				   StartPos = 0;
				   EndPos = 0;
				   EndFlags = 0;
				   EndOfArchive_was_Read = false;
				   Extra.Free();
				   }
				 */
				uint64 GetPhySize() const;
				bool AreMoreVolumes() const;
				bool IsVolume() const;
				bool IsSolid() const;
				bool Is_VolNumber_Defined() const;
				uint64 GetVolIndex() const;
			};
			CMyComPtr <IInStream> Stream;
			CInArcInfo Info;
		};

		class CHandler : public IInArchive, public IArchiveGetRawProps, PUBLIC_ISetCompressCodecsInfo public CMyUnknownImp {
		public:
			CRecordVector<CRefItem> _refs;
			CObjectVector<CItem> _items;
		private:
			CObjectVector<CArc> _arcs;
			CObjectVector<CByteBuffer> _acls;
			uint32 _errorFlags;
			// uint32 _warningFlags;
			bool _isArc;
			CByteBuffer _comment;
			UString _missingVolName;
			DECL_EXTERNAL_CODECS_VARS

			uint64 GetPackSize(unsigned refIndex) const;
			void FillLinks();
			HRESULT Open2(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback);
		public:
			MY_QUERYINTERFACE_BEGIN2(IInArchive)
			MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
			QUERY_ENTRY_ISetCompressCodecsInfo
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE

			INTERFACE_IInArchive(; )
			INTERFACE_IArchiveGetRawProps(; )

			DECL_ISetCompressCodecsInfo
		};
	}
	namespace NRar {
		struct CInArcInfo {
			CInArcInfo() : EndFlags(0), EndOfArchive_was_Read(false) 
			{
			}
			uint64 GetPhySize() const { return EndPos - StartPos; }
			bool ExtraZeroTail_is_Possible() const { return IsVolume() && IsRecovery() && EndOfArchive_was_Read; }
			bool IsVolume() const { return (Flags & NHeader::NArchive::kVolume) != 0; }
			bool IsCommented() const { return (Flags & NHeader::NArchive::kComment) != 0; }
			// kLock
			bool IsSolid() const { return (Flags & NHeader::NArchive::kSolid) != 0; }
			bool HaveNewVolumeName() const { return (Flags & NHeader::NArchive::kNewVolName) != 0; }
			// kAuthenticity
			bool IsRecovery() const { return (Flags & NHeader::NArchive::kRecovery) != 0; }
			bool IsEncrypted() const { return (Flags & NHeader::NArchive::kBlockEncryption) != 0; }
			bool IsFirstVolume() const { return (Flags & NHeader::NArchive::kFirstVolume) != 0; }
			// bool IsThereEncryptVer()  const { return (Flags & NHeader::NArchive::kEncryptVer) != 0; }
			// bool IsEncryptOld() const { return (!IsThereEncryptVer() || EncryptVersion < 36); }
			bool AreMoreVolumes() const { return (EndFlags & NHeader::NArchive::kEndOfArc_Flags_NextVol) != 0; }
			bool Is_VolNumber_Defined() const { return (EndFlags & NHeader::NArchive::kEndOfArc_Flags_VolNumber) != 0; }
			bool Is_DataCRC_Defined() const { return (EndFlags & NHeader::NArchive::kEndOfArc_Flags_DataCRC) != 0; }

			uint32 Flags;
			Byte EncryptVersion;
			uint64 StartPos;
			uint64 EndPos;
			uint64 FileSize;
			uint32 EndFlags;
			uint32 VolNumber;
			uint32 DataCRC;
			bool EndOfArchive_was_Read;
		};

		struct CArc {
			CMyComPtr<IInStream> Stream;
			uint64 PhySize;
			// CByteBuffer Comment;
			CArc() : PhySize(0) 
			{
			}
			ISequentialInStream * CreateLimitedStream(uint64 offset, uint64 size) const;
		};

		struct CRefItem {
			uint   VolumeIndex;
			uint   ItemIndex;
			uint   NumItems;
		};

		class CHandler : public IInArchive, PUBLIC_ISetCompressCodecsInfo public CMyUnknownImp {
			CRecordVector<CRefItem> _refItems;
			CObjectVector<CItem> _items;
			CObjectVector<CArc> _arcs;
			NArchive::NRar::CInArcInfo _arcInfo;
			// AString _errorMessage;
			uint32 _errorFlags;
			uint32 _warningFlags;
			bool _isArc;
			UString _missingVolName;

			DECL_EXTERNAL_CODECS_VARS
			uint64 GetPackSize(unsigned refIndex) const;
			bool IsSolid(unsigned refIndex) const;
			/*
			   void AddErrorMessage(const AString &s)
			   {
			   if(!_errorMessage.IsEmpty())
				_errorMessage += '\n';
			   _errorMessage += s;
			   }
			 */
			HRESULT Open2(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback);
		public:
			MY_QUERYINTERFACE_BEGIN2(IInArchive)
			QUERY_ENTRY_ISetCompressCodecsInfo
			MY_QUERYINTERFACE_END
			MY_ADDREF_RELEASE
			INTERFACE_IInArchive(; )
			DECL_ISetCompressCodecsInfo
		};
	}
}
//
//#include <HashCon.h>
class CHashCallbackConsole : public IHashCallbackUI, public CCallbackConsoleBase {
public:
	CHashCallbackConsole();
	~CHashCallbackConsole();
	INTERFACE_IHashCallbackUI(; )
	bool PrintNameInPercents;
	bool PrintHeaders;
	bool PrintSize;
	bool PrintName;
private:
	void AddSpacesBeforeName();
	void PrintSeparatorLine(const CObjectVector<CHasherState> &hashers);
	void PrintResultLine(uint64 fileSize, const CObjectVector<CHasherState> &hashers, unsigned digestIndex, bool showHash);
	void PrintProperty(const char * name, uint64 value);

	UString _fileName;
	AString _s;
};

void PrintHashStat(CStdOutStream &so, const CHashBundle &hb);
//
//#include <List.h>

// @construction {
struct ListArchivesParam {
	enum {
		fStdInMode         = 0x0001,
		fProcessAltStreams = 0x0002,
		fShowAltStreams    = 0x0004,
		fEnableHeaders     = 0x0008,
		fTechMode          = 0x0010
	};
	uint   Flags;
};
// } @construction 

HRESULT ListArchives(CCodecs *codecs, const CObjectVector<COpenType> &types, const CIntVector &excludedFormats,
    bool stdInMode, UStringVector &archivePaths, UStringVector &archivePathsFull, bool processAltStreams, bool showAltStreams,
    const NWildcard::CCensorNode &wildcardCensor, bool enableHeaders, bool techMode,
    #ifndef _NO_CRYPTO
    bool & passwordEnabled, UString &password,
    #endif
    #ifndef _SFX
    const CObjectVector<CProperty> *props,
    #endif
    uint64 &errors, uint64 &numWarnings);
//
//#include <OpenCallbackConsole.h>
class COpenCallbackConsole : public IOpenCallbackUI {
protected:
	CPercentPrinter _percent;
	CStdOutStream * _so;
	CStdOutStream * _se;
	bool _totalFilesDefined;
	// bool _totalBytesDefined;
	// uint64 _totalFiles;
	uint64 _totalBytes;

	bool NeedPercents() const { return _percent._so != NULL; }
public:
	void ClosePercents();
	COpenCallbackConsole();
	void Init(CStdOutStream * outStream, CStdOutStream * errorStream, CStdOutStream * percentStream);
	INTERFACE_IOpenCallbackUI(; )
#ifndef _NO_CRYPTO
	UString Password;
	bool PasswordIsDefined;
	// bool PasswordWasAsked;
#endif
	bool MultiArcMode;
};
//
//#include <7zSpecStream.h>
class CSequentialInStreamSizeCount2 : public ISequentialInStream, public ICompressGetSubStreamSize, public CMyUnknownImp {
public:
	void Init(ISequentialInStream * stream);
	uint64 GetSize() const { return _size; }
	MY_UNKNOWN_IMP2(ISequentialInStream, ICompressGetSubStreamSize)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(GetSubStreamSize) (uint64 subStream, uint64 *value);
private:
	CMyComPtr<ISequentialInStream> _stream;
	CMyComPtr<ICompressGetSubStreamSize> _getSubStreamSize;
	uint64 _size;
};
//
//#include <ArchiveCommandLine.h>
struct CArcCmdLineException : public UString {
	CArcCmdLineException(const char * a, const wchar_t * u = NULL);
};

struct CArcCommand {
	bool IsFromExtractGroup() const;
	bool IsFromUpdateGroup() const;
	bool IsTestCommand() const { return CommandType == NCommandType::kTest; }
	NExtract::NPathMode::EEnum GetPathMode() const;

	NCommandType::EEnum CommandType;
};

struct CArcCmdLineOptions {
	// bool IsOutAllowed() const { return Number_for_Out != k_OutStream_disabled; }
	CArcCmdLineOptions();

	bool   HelpMode;
	bool   LargePages;
	bool   CaseSensitiveChange;
	bool   CaseSensitive;
	bool   IsInTerminal;
	bool   IsStdOutTerminal;
	bool   IsStdErrTerminal;
	bool   StdInMode;
	bool   StdOutMode;
	bool   EnableHeaders;
	bool   YesToAll;
	bool   ShowDialog;
	NWildcard::CCensor Censor;
	CArcCommand Command;
	UString ArchiveName;
  #ifndef _NO_CRYPTO
	bool   PasswordEnabled;
	UString Password;
  #endif
	bool   TechMode;
	bool   ShowTime;
	UStringVector HashMethods;
	bool   AppendName;
	// UStringVector ArchivePathsSorted;
	// UStringVector ArchivePathsFullSorted;
	NWildcard::CCensor arcCensor;
	UString ArcName_for_StdInMode;
	CObjectVector <CProperty> Properties;
	CExtractOptionsBase ExtractOptions;
	CBoolPair NtSecurity;
	CBoolPair AltStreams;
	CBoolPair HardLinks;
	CBoolPair SymLinks;
	CUpdateOptions UpdateOptions;
	CHashOptions HashOptions;
	UString ArcType;
	UStringVector ExcludedArcTypes;
	uint   Number_for_Out;
	uint   Number_for_Errors;
	uint   Number_for_Percents;
	uint   LogLevel;
	uint32 NumIterations; // Benchmark
};

class CArcCmdLineParser {
	NCommandLineParser::CParser parser;
public:
	void Parse1(const UStringVector &commandStrings, CArcCmdLineOptions &options);
	void Parse2(CArcCmdLineOptions &options);
};

HRESULT EnumerateDirItemsAndSort(NWildcard::CCensor &censor, NWildcard::ECensorPathMode pathMode, const UString &addPathPrefix,
    UStringVector &sortedPaths, UStringVector &sortedFullPaths, CDirItemsStat &st, IDirItemsCallback * callback);
//
//#include <ChmIn.h> <ChmHandler.h> <IsoIn.h> <IsoHandler.h>
namespace NArchive {
	namespace NChm {
		struct CItem {
			uint64 Section;
			uint64 Offset;
			uint64 Size;
			AString Name;

			bool IsFormatRelatedItem() const { return (Name.Len() >= 2) ? (Name[0] == ':' && Name[1] == ':') : false; }
			bool IsUserItem() const { return (Name.Len() >= 2) ? Name[0] == '/' : false; }
			bool IsDir() const { return Name.IsEmpty() ? false : (Name.Back() == '/'); }
		};

		struct CDatabase {
			uint64 StartPosition;
			uint64 ContentOffset;
			CObjectVector<CItem> Items;
			AString NewFormatString;
			bool Help2Format;
			bool NewFormat;
			uint64 PhySize;

			void UpdatePhySize(uint64 v) 
			{
				SETMAX(PhySize, v);
			}
			int FASTCALL FindItem(const AString & name) const
			{
				FOR_VECTOR(i, Items) {
					if(Items[i].Name == name)
						return i;
				}
				return -1;
			}
			void Clear()
			{
				NewFormat = false;
				NewFormatString.Empty();
				Help2Format = false;
				Items.Clear();
				StartPosition = 0;
				PhySize = 0;
			}
		};

		const uint32 kBlockSize = 1 << 15;

		struct CResetTable {
			uint64 UncompressedSize;
			uint64 CompressedSize;
			// unsigned BlockSizeBits;
			CRecordVector<uint64> ResetOffsets;
			bool GetCompressedSizeOfBlocks(uint64 blockIndex, uint32 numBlocks, uint64 &size) const
			{
				if(blockIndex >= ResetOffsets.Size())
					return false;
				uint64 startPos = ResetOffsets[(uint)blockIndex];
				if(blockIndex + numBlocks >= ResetOffsets.Size())
					size = CompressedSize - startPos;
				else
					size = ResetOffsets[(uint)(blockIndex + numBlocks)] - startPos;
				return true;
			}
			bool GetCompressedSizeOfBlock(uint64 blockIndex, uint64 &size) const
			{
				return GetCompressedSizeOfBlocks(blockIndex, 1, size);
			}
			uint64 GetNumBlocks(uint64 size) const
			{
				return (size + kBlockSize - 1) / kBlockSize;
			}
		};

		struct CLzxInfo {
			uint   GetNumDictBits() const;
			uint64 GetFolderSize() const;
			uint64 GetFolder(uint64 offset) const;
			uint64 GetFolderPos(uint64 folderIndex) const;
			uint64 GetBlockIndexFromFolderIndex(uint64 folderIndex) const;
			bool   GetOffsetOfFolder(uint64 folderIndex, uint64 &offset) const;
			bool GetCompressedSizeOfFolder(uint64 folderIndex, uint64 &size) const;

			uint32 Version;
			uint   ResetIntervalBits;
			uint   WindowSizeBits;
			uint32 CacheSize;
			CResetTable ResetTable;
		};

		struct CMethodInfo {
			bool IsLzx() const;
			bool IsDes() const;
			AString GetGuidString() const;
			AString GetName() const;

			Byte Guid[16];
			CByteBuffer ControlData;
			CLzxInfo LzxInfo;
		};

		struct CSectionInfo {
			bool IsLzx() const;
			UString GetMethodName() const;

			uint64 Offset;
			uint64 CompressedSize;
			uint64 UncompressedSize;
			AString Name;
			CObjectVector<CMethodInfo> Methods;
		};

		class CFilesDatabase : public CDatabase {
		public:
			uint64 FASTCALL GetFileSize(uint fileIndex) const;
			uint64 FASTCALL GetFileOffset(uint fileIndex) const;
			uint64 FASTCALL GetFolder(uint fileIndex) const;
			uint64 FASTCALL GetLastFolder(uint fileIndex) const;
			void HighLevelClear();
			void Clear();
			void SetIndices();
			void Sort();
			bool Check();
			bool CheckSectionRefs();

			bool LowLevel;
			CUIntVector Indices;
			CObjectVector<CSectionInfo> Sections;
		};

		class CInArchive {
			CMyComPtr <ISequentialInStream> m_InStreamRef;
			::CInBuffer _inBuffer;
			uint64 _chunkSize;
			bool _help2;

			Byte ReadByte();
			void ReadBytes(Byte * data, uint32 size);
			void Skip(size_t size);
			uint16 ReadUInt16();
			uint32 ReadUInt32();
			uint64 ReadUInt64();
			uint64 ReadEncInt();
			void ReadString(uint size, AString &s);
			void ReadUString(uint size, UString &s);
			void ReadGUID(Byte * g);
			HRESULT ReadChunk(IInStream * inStream, uint64 pos, uint64 size);
			HRESULT ReadDirEntry(CDatabase &database);
			HRESULT DecompressStream(IInStream * inStream, const CDatabase &database, const AString &name);
		public:
			CInArchive(bool help2) 
			{
				_help2 = help2;
			}
			HRESULT OpenChm(IInStream * inStream, CDatabase &database);
			HRESULT OpenHelp2(IInStream * inStream, CDatabase &database);
			HRESULT OpenHighLevel(IInStream * inStream, CFilesDatabase &database);
			HRESULT Open2(IInStream * inStream, const uint64 * searchHeaderSizeLimit, CFilesDatabase &database);
			HRESULT Open(IInStream * inStream, const uint64 * searchHeaderSizeLimit, CFilesDatabase &database);

			bool IsArc;
			bool HeadersError;
			bool UnexpectedEnd;
			bool UnsupportedFeature;
		};

		class CHandler : public IInArchive, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP1(IInArchive)
			INTERFACE_IInArchive(; )
			bool _help2;
			CHandler(bool help2) : _help2(help2) 
			{
			}
		private:
			CFilesDatabase m_Database;
			CMyComPtr<IInStream> m_Stream;
			uint32 m_ErrorFlags;
		};
	}
	namespace NIso {
		struct CDir : public CDirRecord {
			void Clear();
			AString GetPath(bool checkSusp, unsigned skipSize) const;
			void FASTCALL GetPathU(UString &s) const;

			CDir * Parent;
			CObjectVector <CDir> _subItems;
		};

		struct CDateTime {
			bool NotSpecified() const;
			bool FASTCALL GetFileTime(FILETIME &ft) const;

			uint16 Year;
			Byte Month;
			Byte Day;
			Byte Hour;
			Byte Minute;
			Byte Second;
			Byte Hundredths;
			signed char GmtOffset; // min intervals from -48 (West) to +52 (East) recorded.
		};

		struct CBootRecordDescriptor {
			Byte BootSystemId[32]; // a-characters
			Byte BootId[32];  // a-characters
			Byte BootSystemUse[1977];
		};

		struct CBootValidationEntry {
			Byte PlatformId;
			Byte Id[24]; // to identify the manufacturer/developer of the CD-ROM.
		};

		struct CBootInitialEntry {
			bool Bootable;
			Byte BootMediaType;
			uint16 LoadSegment;
			/* This is the load segment for the initial boot image. If this
			   value is 0 the system will use the traditional segment of 7C0. If this value
			   is non-zero the system will use the specified segment. This applies to x86
			   architectures only. For "flat" model architectures (such as Motorola) this
			   is the address divided by 10. */
			Byte SystemType; // This must be a copy of byte 5 (System Type) from the Partition Table found in the boot image.
			uint16 SectorCount; // This is the number of virtual/emulated sectors the system will store at Load Segment during the initial boot procedure.
			uint32 LoadRBA; // This is the start address of the virtual disk. CDs use Relative/Logical block addressing.
			Byte VendorSpec[20];
			uint32 GetSize() const
			{
				// if(BootMediaType == NBootMediaType::k1d44Floppy) (1440 << 10);
				return (uint32)SectorCount * 512;
			}
			bool Parse(const Byte * p);
			AString GetName() const;
		};

		struct CVolumeDescriptor {
			bool IsJoliet() const;

			Byte VolFlags;
			Byte SystemId[32]; // a-characters. An identification of a system which can recognize and act upon the content of the Logical
				// Sectors with logical Sector Numbers 0 to 15 of the volume.
			Byte VolumeId[32]; // d-characters. An identification of the volume.
			uint32 VolumeSpaceSize; // the number of Logical Blocks in which the Volume Space of the volume is recorded
			Byte EscapeSequence[32];
			uint16 VolumeSetSize;
			uint16 VolumeSequenceNumber; // the ordinal number of the volume in the Volume Set of which the volume is a
								 // member.
			uint16 LogicalBlockSize;
			uint32 PathTableSize;
			uint32 LPathTableLocation;
			uint32 LOptionalPathTableLocation;
			uint32 MPathTableLocation;
			uint32 MOptionalPathTableLocation;
			CDirRecord RootDirRecord;
			Byte VolumeSetId[128];
			Byte PublisherId[128];
			Byte DataPreparerId[128];
			Byte ApplicationId[128];
			Byte CopyrightFileId[37];
			Byte AbstractFileId[37];
			Byte BibFileId[37];
			CDateTime CTime;
			CDateTime MTime;
			CDateTime ExpirationTime;
			CDateTime EffectiveTime;
			Byte FileStructureVersion; // = 1;
			Byte ApplicationUse[512];
		};

		struct CRef {
			const CDir * Dir;
			uint32 Index;
			uint32 NumExtents;
			uint64 TotalSize;
		};

		const uint32 kBlockSize = 1 << 11;

		class CInArchive {
			IInStream * _stream;
			uint64 _position;
			uint32 m_BufferPos;
			CDir _rootDir;
			bool _bootIsDefined;
			CBootRecordDescriptor _bootDesc;

			void Skip(size_t size);
			void SkipZeros(size_t size);
			Byte ReadByte();
			void ReadBytes(Byte * data, uint32 size);
			uint16 ReadUInt16();
			uint32 ReadUInt32Le();
			uint32 ReadUInt32Be();
			uint32 ReadUInt32();
			uint64 ReadUInt64();
			uint32 ReadDigits(int numDigits);
			void ReadDateTime(CDateTime &d);
			void ReadRecordingDateTime(CRecordingDateTime &t);
			void ReadDirRecord2(CDirRecord &r, Byte len);
			void ReadDirRecord(CDirRecord &r);
			void ReadBootRecordDescriptor(CBootRecordDescriptor &d);
			void ReadVolumeDescriptor(CVolumeDescriptor &d);
			void SeekToBlock(uint32 blockIndex);
			void ReadDir(CDir &d, int level);
			void CreateRefs(CDir &d);
			void ReadBootInfo();
			HRESULT Open2();
		public:
			HRESULT Open(IInStream * inStream);
			void Clear();
			void UpdatePhySize(uint32 blockIndex, uint64 size);
			bool IsJoliet() const;
			uint64 FASTCALL GetBootItemSize(int index) const;

			uint64 _fileSize;
			uint64 PhySize;
			CRecordVector<CRef> Refs;
			CObjectVector<CVolumeDescriptor> VolDescs;
			int    MainVolDescIndex;
			// uint32 BlockSize;
			CObjectVector <CBootInitialEntry> BootEntries;
			CRecordVector<uint32> UniqStartLocations;
			Byte   m_Buffer[kBlockSize];
			uint   SuspSkipSize;
			bool   IsArc;
			bool   UnexpectedEnd;
			bool   HeadersError;
			bool   IncorrectBigEndian;
			bool   TooDeepDirs;
			bool   SelfLinkedDirs;
			bool   IsSusp;
			uint8  Reserve[1]; // @alignment
		};

		class CHandler : public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
			CMyComPtr<IInStream> _stream;
			CInArchive _archive;
		public:
			MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
			INTERFACE_IInArchive(; )
			STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream);
		};
	}
}
//
//#include <ExtractCallbackConsole.h>
class CExtractScanConsole : public IDirItemsCallback {
	CStdOutStream * _so;
	CStdOutStream * _se;
	CPercentPrinter _percent;
	bool NeedPercents() const { return _percent._so != NULL; }
	void ClosePercentsAndFlush()
	{
		if(NeedPercents())
			_percent.ClosePrint(true);
		CALLPTRMEMB(_so, Flush());
	}
public:
	void Init(CStdOutStream * outStream, CStdOutStream * errorStream, CStdOutStream * percentStream)
	{
		_so = outStream;
		_se = errorStream;
		_percent._so = percentStream;
	}
	void SetWindowWidth(unsigned width) { _percent.MaxLen = width - 1; }
	void StartScanning();
	INTERFACE_IDirItemsCallback(; )
	void CloseScanning()
	{
		if(NeedPercents())
			_percent.ClosePrint(true);
	}
	void PrintStat(const CDirItemsStat &st);
};

class CExtractCallbackConsole : public IExtractCallbackUI, /*public IArchiveExtractCallbackMessage,*/ public IFolderArchiveExtractCallback2,
  #ifndef _NO_CRYPTO
	public ICryptoGetTextPassword,
  #endif
	public COpenCallbackConsole, public CMyUnknownImp {
	AString _tempA;
	UString _tempU;
	UString _currentName;
	void ClosePercents_for_so()
	{
		if(NeedPercents() && _so == _percent._so)
			_percent.ClosePrint(false);
	}
	void ClosePercentsAndFlush()
	{
		if(NeedPercents())
			_percent.ClosePrint(true);
		CALLPTRMEMB(_so, Flush());
	}
public:
	MY_QUERYINTERFACE_BEGIN2(IFolderArchiveExtractCallback)
	// MY_QUERYINTERFACE_ENTRY(IArchiveExtractCallbackMessage)
	MY_QUERYINTERFACE_ENTRY(IFolderArchiveExtractCallback2)
  #ifndef _NO_CRYPTO
	MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
  #endif
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	STDMETHOD(SetTotal) (uint64 total);
	STDMETHOD(SetCompleted) (const uint64 *completeValue);
	INTERFACE_IFolderArchiveExtractCallback(; )
	INTERFACE_IExtractCallbackUI(; )
	// INTERFACE_IArchiveExtractCallbackMessage(;)
	INTERFACE_IFolderArchiveExtractCallback2(; )

  #ifndef _NO_CRYPTO
	STDMETHOD(CryptoGetTextPassword) (BSTR *password);
  #endif
	uint64 NumTryArcs;
	bool ThereIsError_in_Current;
	bool ThereIsWarning_in_Current;
	uint64 NumOkArcs;
	uint64 NumCantOpenArcs;
	uint64 NumArcsWithError;
	uint64 NumArcsWithWarnings;
	uint64 NumOpenArcErrors;
	uint64 NumOpenArcWarnings;
	uint64 NumFileErrors;
	uint64 NumFileErrors_in_Current;
	bool NeedFlush;
	unsigned PercentsNameLevel;
	unsigned LogLevel;
	CExtractCallbackConsole() : NeedFlush(false), PercentsNameLevel(1), LogLevel(0)
	{
	}
	void SetWindowWidth(unsigned width) 
	{
		_percent.MaxLen = width - 1;
	}
	void Init(CStdOutStream * outStream, CStdOutStream * errorStream, CStdOutStream * percentStream)
	{
		COpenCallbackConsole::Init(outStream, errorStream, percentStream);
		NumTryArcs = 0;
		ThereIsError_in_Current = false;
		ThereIsWarning_in_Current = false;
		NumOkArcs = 0;
		NumCantOpenArcs = 0;
		NumArcsWithError = 0;
		NumArcsWithWarnings = 0;
		NumOpenArcErrors = 0;
		NumOpenArcWarnings = 0;
		NumFileErrors = 0;
		NumFileErrors_in_Current = 0;
	}
};
//
//#include <NsisDecode.h>
namespace NArchive {
	namespace NNsis {
		namespace NMethodType {
			enum EEnum {
				kCopy,
				kDeflate,
				kBZip2,
				kLZMA
			};
		}
		// 
		// 7-Zip installers 4.38 - 9.08 used modified version of NSIS that
		// supported BCJ filter for better compression ratio.
		// We support such modified NSIS archives. 
		// 
		class CDecoder {
			NMethodType::EEnum _curMethod; // method of created decoder
			CFilterCoder * _filter;
			CMyComPtr<ISequentialInStream> _filterInStream;
			CMyComPtr<ISequentialInStream> _codecInStream;
			CMyComPtr<ISequentialInStream> _decoderInStream;
			NCompress::NBZip2::CNsisDecoder * _bzDecoder;
			NCompress::NDeflate::NDecoder::CCOMCoder * _deflateDecoder;
			NCompress::NLzma::CDecoder * _lzmaDecoder;
		public:
			CMyComPtr<IInStream> InputStream; // for non-solid
			uint64 StreamPos; // the pos in unpacked for solid, the pos in Packed for non-solid

			NMethodType::EEnum Method;
			bool FilterFlag;
			bool Solid;
			bool IsNsisDeflate;

			CByteBuffer Buffer; // temp buf

			CDecoder() : FilterFlag(false), Solid(true), IsNsisDeflate(true)
			{
				_bzDecoder = NULL;
				_deflateDecoder = NULL;
				_lzmaDecoder = NULL;
			}
			void Release()
			{
				_filterInStream.Release();
				_codecInStream.Release();
				_decoderInStream.Release();
				InputStream.Release();

				_bzDecoder = NULL;
				_deflateDecoder = NULL;
				_lzmaDecoder = NULL;
			}
			uint64 GetInputProcessedSize() const;
			HRESULT Init(ISequentialInStream * inStream, bool &useFilter);
			HRESULT Read(void * data, size_t * processedSize)
			{
				return ReadStream(_decoderInStream, data, processedSize);;
			}
			HRESULT SetToPos(uint64 pos, ICompressProgressInfo * progress); // for solid
			HRESULT Decode(CByteBuffer * outBuf, bool unpackSizeDefined, uint32 unpackSize, ISequentialOutStream * realOutStream, ICompressProgressInfo * progress, uint32 &packSizeRes, uint32 &unpackSizeRes);
		};
	}
}
//
//#include <NsisIn.h>
//
// If NSIS_SCRIPT is defined, it will decompile NSIS script to [NSIS].nsi file.
// The code is much larger in that case. 
//
// #define NSIS_SCRIPT
namespace NArchive {
	namespace NNsis {
		const size_t kScriptSizeLimit = 1 << 27;

		const uint kSignatureSize = 16;
		extern const Byte kSignature[kSignatureSize];
		#define NSIS_SIGNATURE { 0xEF, 0xBE, 0xAD, 0xDE, 'N', 'u', 'l', 'l', 's', 'o', 'f', 't', 'I', 'n', 's', 't' }

		const uint32 kFlagsMask = 0xF;

		namespace NFlags {
			const uint32 kUninstall = 1;
			const uint32 kSilent = 2;
			const uint32 kNoCrc = 4;
			const uint32 kForceCrc = 8;
		}

		struct CFirstHeader {
			uint32 Flags;
			uint32 HeaderSize;
			uint32 ArcSize;
			bool ThereIsCrc() const
			{
				return (Flags & NFlags::kForceCrc) != 0 || (Flags & NFlags::kNoCrc) == 0;
			}
			uint32 GetDataSize() const { return ArcSize - (ThereIsCrc() ? 4 : 0); }
		};

		struct CBlockHeader {
			uint32 Offset;
			uint32 Num;
			void Parse(const Byte * p)
			{
				Offset = GetUi32(p);
				Num = GetUi32(p + 4);
			}
		};

		struct CItem {
			bool IsCompressed;
			bool Size_Defined;
			bool CompressedSize_Defined;
			bool EstimatedSize_Defined;
			bool Attrib_Defined;
			bool IsUninstaller;
			// bool UseFilter;

			uint32 Attrib;
			uint32 Pos;
			uint32 Size;
			uint32 CompressedSize;
			uint32 EstimatedSize;
			uint32 DictionarySize;
			uint32 PatchSize; // for Uninstaller.exe
			int Prefix; // - 1 means no prefix

			FILETIME MTime;
			AString NameA;
			UString NameU;

			CItem() : IsCompressed(true), Size_Defined(false), CompressedSize_Defined(false), EstimatedSize_Defined(false), Attrib_Defined(false),
				IsUninstaller(false), /*UseFilter(false),*/ Attrib(0), Pos(0), Size(0), CompressedSize(0), EstimatedSize(0), DictionarySize(1),
				PatchSize(0), Prefix(-1)
			{
				MTime.dwLowDateTime = 0;
				MTime.dwHighDateTime = 0;
			}
			/*
			   bool IsINSTDIR() const
			   {
			   return (PrefixA.Len() >= 3 || PrefixU.Len() >= 3);
			   }
			 */
		};

		enum ENsisType {
			k_NsisType_Nsis2,
			k_NsisType_Nsis3,
			k_NsisType_Park1, // Park 2.46.1-
			k_NsisType_Park2, // Park 2.46.2  : GetFontVersion
			k_NsisType_Park3 // Park 2.46.3+ : GetFontName
		};

		#ifdef NSIS_SCRIPT
			struct CSection {
				uint32 InstallTypes; // bits set for each of the different install_types, if any.
				uint32 Flags; // SF_* - defined above
				uint32 StartCmdIndex; // code;
				uint32 NumCommands; // code_size;
				uint32 SizeKB;
				uint32 Name;

				void Parse(const Byte * data);
			};

			struct CLicenseFile {
				uint32 Offset;
				uint32 Size;
				AString Name;
				CByteBuffer Text;
			};
		#endif

		class CInArchive {
		public:
		  #ifdef NSIS_SCRIPT
			CDynLimBuf Script;
		  #endif
			CByteBuffer _data;
			CObjectVector<CItem> Items;
			bool IsUnicode;
		private:
			uint32 _stringsPos; // relative to _data
			uint32 NumStringChars;
			size_t _size;     // it's Header Size

			AString Raw_AString;
			UString Raw_UString;

			ENsisType NsisType;
			bool IsNsis200; // NSIS 2.03 and before
			bool IsNsis225; // NSIS 2.25 and before
			bool LogCmdIsEnabled;
			int BadCmd; // -1: no bad command; in another cases lowest bad command id

			bool IsPark() const { return NsisType >= k_NsisType_Park1; }

			uint64 _fileSize;

			bool _headerIsCompressed;
			uint32 _nonSolidStartOffset;

		  #ifdef NSIS_SCRIPT

			CByteBuffer strUsed;

			CBlockHeader bhPages;
			CBlockHeader bhSections;
			CBlockHeader bhCtlColors;
			CBlockHeader bhData;
			uint32 AfterHeaderSize;
			CByteBuffer _afterHeader;

			uint32 SectionSize;
			const Byte * _mainLang;
			uint32 _numLangStrings;
			AString LangComment;
			CRecordVector<uint32> langStrIDs;
			uint32 numOnFunc;
			uint32 onFuncOffset;
			// CRecordVector<uint32> OnFuncs;
			unsigned _numRootLicenses;
			CRecordVector<uint32> noParseStringIndexes;
			AString _tempString_for_GetVar;
			AString _tempString_for_AddFuncName;
			AString _tempString;

		  #endif

		public:
			CMyComPtr<IInStream> _stream; // it's limited stream that contains only NSIS archive
			uint64 StartOffset;     // offset in original stream.
			uint64 DataStreamOffset; // = sizeof(FirstHeader) = offset of Header in _stream

			bool IsArc;

			CDecoder Decoder;
			CByteBuffer ExeStub;
			CFirstHeader FirstHeader;
			NMethodType::EEnum Method;
			uint32 DictionarySize;
			bool IsSolid;
			bool UseFilter;
			bool FilterFlag;

			bool IsInstaller;
			AString Name;
			AString BrandingText;
			UStringVector UPrefixes;
			AStringVector APrefixes;

		  #ifdef NSIS_SCRIPT
			CObjectVector<CLicenseFile> LicenseFiles;
		  #endif

		private:
			void GetShellString(AString &s, unsigned index1, unsigned index2);
			void GetNsisString_Raw(const Byte * s);
			void GetNsisString_Unicode_Raw(const Byte * s);
			void ReadString2_Raw(uint32 pos);
			bool IsGoodString(uint32 param) const;
			bool AreTwoParamStringsEqual(uint32 param1, uint32 param2) const;

			void Add_LangStr(AString &res, uint32 id);

		  #ifdef NSIS_SCRIPT

			void Add_UInt(uint32 v);
			void AddLicense(uint32 param, int32 langID);

			void Add_LangStr_Simple(uint32 id);
			void Add_FuncName(const uint32 * labels, uint32 index);
			void AddParam_Func(const uint32 * labels, uint32 index);
			void Add_LabelName(uint32 index);

			void Add_Color2(uint32 v);
			void Add_ColorParam(uint32 v);
			void Add_Color(uint32 index);

			void Add_ButtonID(uint32 buttonID);

			void Add_ShowWindow_Cmd(uint32 cmd);
			void Add_TypeFromList(const char * const * table, unsigned tableSize, uint32 type);
			void Add_ExecFlags(uint32 flagsType);
			void Add_SectOp(uint32 opType);

			void Add_Var(uint32 index);
			void AddParam_Var(uint32 value);
			void AddParam_UInt(uint32 value);

			void Add_GotoVar(uint32 param);
			void Add_GotoVar1(uint32 param);
			void Add_GotoVars2(const uint32 * params);

			bool PrintSectionBegin(const CSection &sect, unsigned index);
			void PrintSectionEnd();

			void GetNsisString(AString &res, const Byte * s);
			void GetNsisString_Unicode(AString &res, const Byte * s);
			uint32 GetNumUsedVars() const;
			void ReadString2(AString &s, uint32 pos);

			void MessageBox_MB_Part(uint32 param);
			void AddParam(uint32 pos);
			void AddOptionalParam(uint32 pos);
			void AddParams(const uint32 * params, unsigned num);
			void AddPageOption1(uint32 param, const char * name);
			void AddPageOption(const uint32 * params, unsigned num, const char * name);
			void AddOptionalParams(const uint32 * params, unsigned num);
			void AddRegRoot(uint32 value);

			void ClearLangComment();
			void Separator();
			void Space();
			void Tab();
			void Tab(bool commented);
			void BigSpaceComment();
			void SmallSpaceComment();
			void AddCommentAndString(const char * s);
			void AddError(const char * s);
			void AddErrorLF(const char * s);
			void CommentOpen();
			void CommentClose();
			void AddLF();
			void AddQuotes();
			void TabString(const char * s);
			void AddStringLF(const char * s);
			void NewLine();
			void PrintNumComment(const char * name, uint32 value);
			void Add_QuStr(const AString &s);
			void SpaceQuStr(const AString &s);
			bool CompareCommands(const Byte * rawCmds, const Byte * sequence, size_t numCommands);

		  #endif

		  #ifdef NSIS_SCRIPT
			unsigned GetNumSupportedCommands() const;
		  #endif

			uint32 GetCmd(uint32 a);
			void FindBadCmd(const CBlockHeader &bh, const Byte *);
			void DetectNsisType(const CBlockHeader &bh, const Byte *);

			HRESULT ReadEntries(const CBlockHeader &bh);
			HRESULT SortItems();
			HRESULT Parse();
			HRESULT Open2(const Byte * data, size_t size);
			void Clear2();

			void GetVar2(AString &res, uint32 index);
			void GetVar(AString &res, uint32 index);
			int32 GetVarIndex(uint32 strPos) const;
			int32 GetVarIndex(uint32 strPos, uint32 &resOffset) const;
			int32 GetVarIndexFinished(uint32 strPos, Byte endChar, uint32 &resOffset) const;
			bool IsVarStr(uint32 strPos, uint32 varIndex) const;
			bool IsAbsolutePathVar(uint32 strPos) const;
			void SetItemName(CItem &item, uint32 strPos);

		public:
			HRESULT Open(IInStream * inStream, const uint64 * maxCheckStartPosition);
			AString GetFormatDescription() const;
			HRESULT InitDecoder()
			{
				bool useFilter;
				return Decoder.Init(_stream, useFilter);
			}
			HRESULT SeekTo(uint64 pos)
			{
				return _stream->Seek(pos, STREAM_SEEK_SET, NULL);
			}
			HRESULT SeekTo_DataStreamOffset()
			{
				return SeekTo(DataStreamOffset);
			}
			HRESULT SeekToNonSolidItem(uint index)
			{
				return SeekTo(GetPosOfNonSolidItem(index));
			}
			void Clear();
			bool IsDirectString_Equal(uint32 offset, const char * s) const;
			/*
			   uint64 GetDataPos(uint index)
			   {
			   const CItem &item = Items[index];
			   return GetOffset() + FirstHeader.HeaderSize + item.Pos;
			   }
			 */

			uint64 GetPosOfSolidItem(uint index) const
			{
				const CItem &item = Items[index];
				return 4 + (uint64)FirstHeader.HeaderSize + item.Pos;
			}

			uint64 GetPosOfNonSolidItem(uint index) const
			{
				const CItem &item = Items[index];
				return DataStreamOffset + _nonSolidStartOffset + 4 + item.Pos;
			}
			void Release()
			{
				Decoder.Release();
			}
			bool IsTruncated() const { return (_fileSize - StartOffset < FirstHeader.ArcSize); }
			UString GetReducedName(uint index) const
			{
				const CItem &item = Items[index];
				UString s;
				if(item.Prefix >= 0) {
					if(IsUnicode)
						s = UPrefixes[item.Prefix];
					else
						s = MultiByteToUnicodeString(APrefixes[item.Prefix]);
					if(s.Len() > 0)
						if(s.Back() != L'\\')
							s += '\\';
				}

				if(IsUnicode) {
					s += item.NameU;
					if(item.NameU.IsEmpty())
						s += "file";
				}
				else {
					s += MultiByteToUnicodeString(item.NameA);
					if(item.NameA.IsEmpty())
						s += "file";
				}

				const char * const kRemoveStr = "$INSTDIR\\";
				if(s.IsPrefixedBy_Ascii_NoCase(kRemoveStr)) {
					s.Delete(0, sstrlen(kRemoveStr));
					if(s[0] == L'\\')
						s.DeleteFrontal(1);
				}
				if(item.IsUninstaller && ExeStub.Size() == 0)
					s += ".nsis";
				return s;
			}

			UString ConvertToUnicode(const AString &s) const;

			CInArchive()
			#ifdef NSIS_SCRIPT
				: Script(kScriptSizeLimit)
			#endif
			{
			}
		};
	}
}
//
//#include <NsisHandler.h>
namespace NArchive {
	namespace NNsis {
		class CHandler : public IInArchive, public CMyUnknownImp {
			CInArchive _archive;
			AString _methodString;

			bool GetUncompressedSize(uint index, uint32 &size) const;
			bool GetCompressedSize(uint index, uint32 &size) const;
			// AString GetMethod(NMethodType::EEnum method, bool useItemFilter, uint32 dictionary) const;
		public:
			MY_UNKNOWN_IMP1(IInArchive)
			INTERFACE_IInArchive(; )
		};
	}
}
//
#endif
