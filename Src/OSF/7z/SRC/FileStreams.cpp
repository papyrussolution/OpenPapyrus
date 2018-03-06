// FileStreams.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
#ifndef _WIN32
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
#endif

static inline HRESULT ConvertBoolToHRESULT(bool result)
{
  #ifdef _WIN32
	if(result)
		return S_OK;
	DWORD lastError = ::GetLastError();
	if(lastError == 0)
		return E_FAIL;
	return HRESULT_FROM_WIN32(lastError);
  #else
	return result ? S_OK : E_FAIL;
  #endif
}

static const uint32 kClusterSize = 1 << 18;

CInFileStream::CInFileStream() :
  #ifdef SUPPORT_DEVICE_FILE
	VirtPos(0), PhyPos(0), Buf(0), BufSize(0),
  #endif
	SupportHardLinks(false), Callback(NULL), CallbackRef(0)
{
}

CInFileStream::~CInFileStream()
{
#ifdef SUPPORT_DEVICE_FILE
	MidFree(Buf);
#endif
	CALLPTRMEMB(Callback, InFileStream_On_Destroy(CallbackRef));
}

STDMETHODIMP CInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
{
  #ifdef USE_WIN_FILE

  #ifdef SUPPORT_DEVICE_FILE
	ASSIGN_PTR(processedSize, 0);
	if(size == 0)
		return S_OK;
	if(File.IsDeviceFile) {
		if(File.SizeDefined) {
			if(VirtPos >= File.Size)
				return VirtPos == File.Size ? S_OK : E_FAIL;
			uint64 rem = File.Size - VirtPos;
			if(size > rem)
				size = (uint32)rem;
		}
		for(;; ) {
			const uint32 mask = kClusterSize - 1;
			const uint64 mask2 = ~(uint64)mask;
			uint64 alignedPos = VirtPos & mask2;
			if(BufSize > 0 && BufStartPos == alignedPos) {
				uint32 pos = (uint32)VirtPos & mask;
				if(pos >= BufSize)
					return S_OK;
				uint32 rem = MyMin(BufSize - pos, size);
				memcpy(data, Buf + pos, rem);
				VirtPos += rem;
				if(processedSize)
					*processedSize += rem;
				return S_OK;
			}
			bool useBuf = false;
			if((VirtPos & mask) != 0 || ((ptrdiff_t)data & mask) != 0)
				useBuf = true;
			else {
				uint64 end = VirtPos + size;
				if((end & mask) != 0) {
					end &= mask2;
					if(end <= VirtPos)
						useBuf = true;
					else
						size = (uint32)(end - VirtPos);
				}
			}
			if(!useBuf)
				break;
			if(alignedPos != PhyPos) {
				uint64 realNewPosition;
				bool result = File.Seek(alignedPos, FILE_BEGIN, realNewPosition);
				if(!result)
					return ConvertBoolToHRESULT(result);
				PhyPos = realNewPosition;
			}
			BufStartPos = alignedPos;
			uint32 readSize = kClusterSize;
			if(File.SizeDefined)
				readSize = (uint32)MyMin(File.Size - PhyPos, (uint64)kClusterSize);
			if(!Buf) {
				Buf = (Byte*)MidAlloc(kClusterSize);
				if(!Buf)
					return E_OUTOFMEMORY;
			}
			bool result = File.Read1(Buf, readSize, BufSize);
			if(!result)
				return ConvertBoolToHRESULT(result);
			if(BufSize == 0)
				return S_OK;
			PhyPos += BufSize;
		}

		if(VirtPos != PhyPos) {
			uint64 realNewPosition;
			bool result = File.Seek(VirtPos, FILE_BEGIN, realNewPosition);
			if(!result)
				return ConvertBoolToHRESULT(result);
			PhyPos = VirtPos = realNewPosition;
		}
	}
  #endif

	uint32 realProcessedSize;
	bool result = File.ReadPart(data, size, realProcessedSize);
	ASSIGN_PTR(processedSize, realProcessedSize);
  #ifdef SUPPORT_DEVICE_FILE
	VirtPos += realProcessedSize;
	PhyPos += realProcessedSize;
  #endif
	if(result)
		return S_OK;
	{
		DWORD error = ::GetLastError();
		if(Callback)
			return Callback->InFileStream_On_Error(CallbackRef, error);
		else if(error == 0)
			return E_FAIL;
		else
			return HRESULT_FROM_WIN32(error);
	}
  #else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res = File.Read(data, (size_t)size);
	if(res == -1) {
		return Callback ? Callback->InFileStream_On_Error(CallbackRef, E_FAIL) : E_FAIL;
	}
	else {
		ASSIGN_PTR(processedSize, (uint32)res);
		return S_OK;
	}
  #endif
}

#ifdef UNDER_CE
	STDMETHODIMP CStdInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
	{
		size_t s2 = fread(data, 1, size, stdin);
		int error = ferror(stdin);
		ASSIGN_PTR(processedSize, s2);
		return (s2 <= size && error == 0) ? S_OK : E_FAIL;
	}
	#else
STDMETHODIMP CStdInFileStream::Read(void * data, uint32 size, uint32 * processedSize)
{
  #ifdef _WIN32
	DWORD realProcessedSize;
	uint32 sizeTemp = (1 << 20);
	SETMIN(sizeTemp, size);
	BOOL res = ::ReadFile(GetStdHandle(STD_INPUT_HANDLE), data, sizeTemp, &realProcessedSize, NULL);
	ASSIGN_PTR(processedSize, realProcessedSize);
	if(res == FALSE && GetLastError() == ERROR_BROKEN_PIPE)
		return S_OK;
	return ConvertBoolToHRESULT(res != FALSE);
  #else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res;
	do {
		res = read(0, data, (size_t)size);
	} while(res < 0 && (errno == EINTR));
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(processedSize, (uint32)res);
	return S_OK;
  #endif
}

#endif

STDMETHODIMP CInFileStream::Seek(Int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin >= 3)
		return STG_E_INVALIDFUNCTION;
  #ifdef USE_WIN_FILE
  #ifdef SUPPORT_DEVICE_FILE
	if(File.IsDeviceFile && (File.SizeDefined || seekOrigin != STREAM_SEEK_END)) {
		switch(seekOrigin) {
			case STREAM_SEEK_SET: break;
			case STREAM_SEEK_CUR: offset += VirtPos; break;
			case STREAM_SEEK_END: offset += File.Size; break;
			default: return STG_E_INVALIDFUNCTION;
		}
		if(offset < 0)
			return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
		VirtPos = offset;
		ASSIGN_PTR(newPosition, offset);
		return S_OK;
	}
  #endif
	uint64 realNewPosition;
	bool result = File.Seek(offset, seekOrigin, realNewPosition);
  #ifdef SUPPORT_DEVICE_FILE
	PhyPos = VirtPos = realNewPosition;
  #endif
	ASSIGN_PTR(newPosition, realNewPosition);
	return ConvertBoolToHRESULT(result);
  #else
	off_t res = File.Seek((off_t)offset, seekOrigin);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(newPosition, (uint64)res);
	return S_OK;
  #endif
}

STDMETHODIMP CInFileStream::GetSize(uint64 * size)
{
	return ConvertBoolToHRESULT(File.GetLength(*size));
}

#ifdef USE_WIN_FILE
	STDMETHODIMP CInFileStream::GetProps(uint64 * size, FILETIME * cTime, FILETIME * aTime, FILETIME * mTime, uint32 * attrib)
	{
		BY_HANDLE_FILE_INFORMATION info;
		if(File.GetFileInformation(&info)) {
			if(size) *size = (((uint64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
			if(cTime) *cTime = info.ftCreationTime;
			if(aTime) *aTime = info.ftLastAccessTime;
			if(mTime) *mTime = info.ftLastWriteTime;
			if(attrib) *attrib = info.dwFileAttributes;
			return S_OK;
		}
		return GetLastError();
	}

	STDMETHODIMP CInFileStream::GetProps2(CStreamFileProps * props)
	{
		BY_HANDLE_FILE_INFORMATION info;
		if(File.GetFileInformation(&info)) {
			props->Size = (((uint64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
			props->VolID = info.dwVolumeSerialNumber;
			props->FileID_Low = (((uint64)info.nFileIndexHigh) << 32) + info.nFileIndexLow;
			props->FileID_High = 0;
			props->NumLinks = SupportHardLinks ? info.nNumberOfLinks : 1;
			props->Attrib = info.dwFileAttributes;
			props->CTime = info.ftCreationTime;
			props->ATime = info.ftLastAccessTime;
			props->MTime = info.ftLastWriteTime;
			return S_OK;
		}
		return GetLastError();
	}
#endif
//
// COutFileStream
//
//virtual 
COutFileStream::~COutFileStream() 
{
}

bool COutFileStream::Create(CFSTR fileName, bool createAlways)
{
	ProcessedSize = 0;
	return File.Create(fileName, createAlways);
}

bool COutFileStream::Open(CFSTR fileName, DWORD creationDisposition)
{
	ProcessedSize = 0;
	return File.Open(fileName, creationDisposition);
}

HRESULT COutFileStream::Close()
{
	return ConvertBoolToHRESULT(File.Close());
}

STDMETHODIMP COutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
#ifdef USE_WIN_FILE
	uint32 realProcessedSize;
	bool result = File.Write(data, size, realProcessedSize);
	ProcessedSize += realProcessedSize;
	ASSIGN_PTR(processedSize, realProcessedSize);
	return ConvertBoolToHRESULT(result);
#else
	ASSIGN_PTR(processedSize, 0);
	ssize_t res = File.Write(data, (size_t)size);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(processedSize, (uint32)res);
	ProcessedSize += res;
	return S_OK;
#endif
}

STDMETHODIMP COutFileStream::Seek(Int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin >= 3)
		return STG_E_INVALIDFUNCTION;
  #ifdef USE_WIN_FILE
	uint64 realNewPosition;
	bool result = File.Seek(offset, seekOrigin, realNewPosition);
	ASSIGN_PTR(newPosition, realNewPosition);
	return ConvertBoolToHRESULT(result);
  #else
	off_t res = File.Seek((off_t)offset, seekOrigin);
	if(res == -1)
		return E_FAIL;
	ASSIGN_PTR(newPosition, (uint64)res);
	return S_OK;
  #endif
}

STDMETHODIMP COutFileStream::SetSize(uint64 newSize)
{
  #ifdef USE_WIN_FILE
	uint64 currentPos;
	if(!File.Seek(0, FILE_CURRENT, currentPos))
		return E_FAIL;
	bool result = File.SetLength(newSize);
	uint64 currentPos2;
	result = result && File.Seek(currentPos, currentPos2);
	return result ? S_OK : E_FAIL;
  #else
	return E_FAIL;
  #endif
}

HRESULT COutFileStream::GetSize(uint64 * size)
{
	return ConvertBoolToHRESULT(File.GetLength(*size));
}

#ifdef UNDER_CE
STDMETHODIMP CStdOutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	size_t s2 = fwrite(data, 1, size, stdout);
	ASSIGN_PTR(processedSize, s2);
	return (s2 == size) ? S_OK : E_FAIL;
}
#else
	STDMETHODIMP CStdOutFileStream::Write(const void * data, uint32 size, uint32 * processedSize)
	{
		ASSIGN_PTR(processedSize, 0);
	  #ifdef _WIN32
		uint32 realProcessedSize;
		BOOL res = TRUE;
		if(size > 0) {
			// Seems that Windows doesn't like big amounts writing to stdout.
			// So we limit portions by 32KB.
			uint32 sizeTemp = (1 << 15);
			if(sizeTemp > size)
				sizeTemp = size;
			res = ::WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
				data, sizeTemp, (DWORD*)&realProcessedSize, NULL);
			_size += realProcessedSize;
			size -= realProcessedSize;
			data = (const void*)((const Byte*)data + realProcessedSize);
			if(processedSize)
				*processedSize += realProcessedSize;
		}
		return ConvertBoolToHRESULT(res != FALSE);
	  #else
		ssize_t res;
		do {
			res = write(1, data, (size_t)size);
		} while(res < 0 && (errno == EINTR));
		if(res == -1)
			return E_FAIL;
		_size += (size_t)res;
		ASSIGN_PTR(processedSize, (uint32)res);
		return S_OK;
	  #endif
	}
#endif
