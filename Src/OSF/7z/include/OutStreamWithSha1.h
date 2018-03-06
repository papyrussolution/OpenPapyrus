// OutStreamWithSha1.h

#ifndef __OUT_STREAM_WITH_SHA1_H
#define __OUT_STREAM_WITH_SHA1_H

//#include <Sha1.h>
//#include <MyCom.h>
//#include <IStream.h>

class COutStreamWithSha1 : public ISequentialOutStream, public CMyUnknownImp {
	CMyComPtr<ISequentialOutStream> _stream;
	uint64 _size;
	CSha1 _sha;
	bool _calculate;
public:
	MY_UNKNOWN_IMP
	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	void SetStream(ISequentialOutStream * stream) {
		_stream = stream;
	}

	void ReleaseStream() {
		_stream.Release();
	}

	void Init(bool calculate = true)
	{
		_size = 0;
		_calculate = calculate;
		Sha1_Init(&_sha);
	}

	void InitSha1() {
		Sha1_Init(&_sha);
	}

	uint64 GetSize() const {
		return _size;
	}

	void Final(Byte * digest) {
		Sha1_Final(&_sha, digest);
	}
};

#endif
