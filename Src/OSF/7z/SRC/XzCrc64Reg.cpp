// XzCrc64Reg.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

class CXzCrc64Hasher : public IHasher, public CMyUnknownImp {
	uint64 _crc;
	Byte mtDummy[1 << 7];
public:
	CXzCrc64Hasher() : _crc(CRC64_INIT_VAL) 
	{
	}
	MY_UNKNOWN_IMP1(IHasher)
	INTERFACE_IHasher(; )
};

STDMETHODIMP_(void) CXzCrc64Hasher::Init() throw()
{
	_crc = CRC64_INIT_VAL;
}

STDMETHODIMP_(void) CXzCrc64Hasher::Update(const void * data, uint32 size) throw()
{
	_crc = Crc64Update(_crc, data, size);
}

STDMETHODIMP_(void) CXzCrc64Hasher::Final(Byte *digest) throw()
{
	uint64 val = CRC64_GET_DIGEST(_crc);
	SetUi64(digest, val);
}

REGISTER_HASHER(CXzCrc64Hasher, 0x4, "CRC64", 8)
