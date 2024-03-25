// HashCalc.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

class CHashMidBuf {
	void * _data;
public:
	CHashMidBuf() : _data(0) 
	{
	}
	operator void *() 
	{
		return _data;
	}
	bool Alloc(size_t size)
	{
		if(_data != 0)
			return false;
		else {
			_data = ::MidAlloc(size);
			return _data != 0;
		}
	}
	~CHashMidBuf() 
	{
		::MidFree(_data);
	}
};

static const char * const k_DefaultHashMethod = "CRC32";

void CHashBundle::Init()
{
	NumDirs = NumFiles = NumAltStreams = FilesSize = AltStreamsSize = NumErrors = 0;
}

HRESULT CHashBundle::SetMethods(DECL_EXTERNAL_CODECS_LOC_VARS const UStringVector &hashMethods)
{
	UStringVector names = hashMethods;
	if(names.IsEmpty())
		names.Add(UString(k_DefaultHashMethod));
	CRecordVector<CMethodId> ids;
	CObjectVector<COneMethodInfo> methods;
	uint i;
	for(i = 0; i < names.Size(); i++) {
		COneMethodInfo m;
		RINOK(m.ParseMethodFromString(names[i]));
		if(m.MethodName.IsEmpty())
			m.MethodName = k_DefaultHashMethod;
		if(m.MethodName == "*") {
			CRecordVector<CMethodId> tempMethods;
			GetHashMethods(EXTERNAL_CODECS_LOC_VARS tempMethods);
			methods.Clear();
			ids.Clear();
			FOR_VECTOR(t, tempMethods) {
				uint index = ids.AddToUniqueSorted(tempMethods[t]);
				if(ids.Size() != methods.Size())
					methods.Insert(index, m);
			}
			break;
		}
		else {
			// m.MethodName.RemoveChar(L'-');
			CMethodId id;
			if(!FindHashMethod(EXTERNAL_CODECS_LOC_VARS m.MethodName, id))
				return E_NOTIMPL;
			uint index = ids.AddToUniqueSorted(id);
			if(ids.Size() != methods.Size())
				methods.Insert(index, m);
		}
	}
	for(i = 0; i < ids.Size(); i++) {
		CMyComPtr<IHasher> hasher;
		AString name;
		RINOK(CreateHasher(EXTERNAL_CODECS_LOC_VARS ids[i], name, hasher));
		if(!hasher)
			throw "Can't create hasher";
		const COneMethodInfo &m = methods[i];
		{
			CMyComPtr<ICompressSetCoderProperties> scp;
			hasher.QueryInterface(IID_ICompressSetCoderProperties, &scp);
			if(scp)
				RINOK(m.SetCoderProps(scp, NULL));
		}
		uint32 digestSize = hasher->GetDigestSize();
		if(digestSize > k_HashCalc_DigestSize_Max)
			return E_NOTIMPL;
		CHasherState &h = Hashers.AddNew();
		h.Hasher = hasher;
		h.Name = name;
		h.DigestSize = digestSize;
		for(uint k = 0; k < k_HashCalc_NumGroups; k++)
			memzero(h.Digests[k], digestSize);
	}
	return S_OK;
}

void CHashBundle::InitForNewFile()
{
	CurSize = 0;
	FOR_VECTOR(i, Hashers) {
		CHasherState &h = Hashers[i];
		h.Hasher->Init();
		memzero(h.Digests[k_HashCalc_Index_Current], h.DigestSize);
	}
}

void CHashBundle::Update(const void * data, uint32 size)
{
	CurSize += size;
	FOR_VECTOR(i, Hashers) {
		Hashers[i].Hasher->Update(data, size);
	}
}

void CHashBundle::SetSize(uint64 size)
{
	CurSize = size;
}

static void AddDigests(Byte * dest, const Byte * src, uint32 size)
{
	unsigned next = 0;
	for(uint32 i = 0; i < size; i++) {
		next += (uint)dest[i] + (uint)src[i];
		dest[i] = (Byte)next;
		next >>= 8;
	}
}

void CHashBundle::Final(bool isDir, bool isAltStream, const UString &path)
{
	if(isDir)
		NumDirs++;
	else if(isAltStream) {
		NumAltStreams++;
		AltStreamsSize += CurSize;
	}
	else {
		NumFiles++;
		FilesSize += CurSize;
	}
	Byte pre[16];
	memzero(pre, sizeof(pre));
	if(isDir)
		pre[0] = 1;
	FOR_VECTOR(i, Hashers) {
		CHasherState &h = Hashers[i];
		if(!isDir) {
			h.Hasher->Final(h.Digests[0]);
			if(!isAltStream)
				AddDigests(h.Digests[k_HashCalc_Index_DataSum], h.Digests[0], h.DigestSize);
		}
		h.Hasher->Init();
		h.Hasher->Update(pre, sizeof(pre));
		h.Hasher->Update(h.Digests[0], h.DigestSize);
		for(uint k = 0; k < path.Len(); k++) {
			wchar_t c = path[k];
			Byte temp[2] = { (Byte)(c & 0xFF), (Byte)((c >> 8) & 0xFF) };
			h.Hasher->Update(temp, 2);
		}
		Byte tempDigest[k_HashCalc_DigestSize_Max];
		h.Hasher->Final(tempDigest);
		if(!isAltStream)
			AddDigests(h.Digests[k_HashCalc_Index_NamesSum], tempDigest, h.DigestSize);
		AddDigests(h.Digests[k_HashCalc_Index_StreamsSum], tempDigest, h.DigestSize);
	}
}

HRESULT HashCalc(DECL_EXTERNAL_CODECS_LOC_VARS const NWildcard::CCensor &censor, const CHashOptions &options, AString &errorInfo, IHashCallbackUI * callback)
{
	CDirItems dirItems;
	dirItems.Callback = callback;
	if(options.StdInMode) {
		CDirItem di;
		di.Size = static_cast<uint64>(-1LL);
		di.Attrib = 0;
		di.MTime.dwLowDateTime = 0;
		di.MTime.dwHighDateTime = 0;
		di.CTime = di.ATime = di.MTime;
		dirItems.Items.Add(di);
	}
	else {
		RINOK(callback->StartScanning());
		dirItems.ScanAltStreams = options.AltStreamsMode;
		HRESULT res = EnumerateItems(censor, options.PathMode, UString(), dirItems);
		if(res != S_OK) {
			if(res != E_ABORT)
				errorInfo = "Scanning error";
			return res;
		}
		RINOK(callback->FinishScanning(dirItems.Stat));
	}
	uint i;
	CHashBundle hb;
	RINOK(hb.SetMethods(EXTERNAL_CODECS_LOC_VARS options.Methods));
	hb.Init();
	hb.NumErrors = dirItems.Stat.NumErrors;
	if(options.StdInMode) {
		RINOK(callback->SetNumFiles(1));
	}
	else {
		RINOK(callback->SetTotal(dirItems.Stat.GetTotalBytes()));
	}
	const uint32 kBufSize = 1 << 15;
	CHashMidBuf buf;
	if(!buf.Alloc(kBufSize))
		return E_OUTOFMEMORY;
	uint64 completeValue = 0;
	RINOK(callback->BeforeFirstFile(hb));
	for(i = 0; i < dirItems.Items.Size(); i++) {
		CMyComPtr<ISequentialInStream> inStream;
		UString path;
		bool isDir = false;
		bool isAltStream = false;
		if(options.StdInMode) {
			inStream = new CStdInFileStream;
		}
		else {
			CInFileStream * inStreamSpec = new CInFileStream;
			inStream = inStreamSpec;
			const CDirItem &dirItem = dirItems.Items[i];
			isDir = dirItem.IsDir();
			isAltStream = dirItem.IsAltStream;
			path = dirItems.GetLogPath(i);
			if(!isDir) {
				FString phyPath = dirItems.GetPhyPath(i);
				if(!inStreamSpec->OpenShared(phyPath, options.OpenShareForWrite)) {
					HRESULT res = callback->OpenFileError(phyPath, ::GetLastError());
					hb.NumErrors++;
					if(res != S_FALSE)
						return res;
					continue;
				}
			}
		}
		RINOK(callback->GetStream(path, isDir));
		uint64 fileSize = 0;
		hb.InitForNewFile();
		if(!isDir) {
			for(uint32 step = 0;; step++) {
				if((step & 0xFF) == 0)
					RINOK(callback->SetCompleted(&completeValue));
				uint32 size;
				RINOK(inStream->Read(buf, kBufSize, &size));
				if(size == 0)
					break;
				hb.Update(buf, size);
				fileSize += size;
				completeValue += size;
			}
		}
		hb.Final(isDir, isAltStream, path);
		RINOK(callback->SetOperationResult(fileSize, hb, !isDir));
		RINOK(callback->SetCompleted(&completeValue));
	}
	return callback->AfterLastFile(hb);
}

static char FASTCALL GetHex(unsigned v) { return (char)((v < 10) ? ('0' + v) : ('A' + (v - 10))); }

void AddHashHexToString(char * dest, const Byte * data, uint32 size)
{
	dest[size * 2] = 0;
	if(!data) {
		for(uint32 i = 0; i < size; i++) {
			dest[0] = ' ';
			dest[1] = ' ';
			dest += 2;
		}
	}
	else {
		int step = 2;
		if(size <= 8) {
			step = -2;
			dest += size * 2 - 2;
		}
		for(uint32 i = 0; i < size; i++) {
			uint   b = data[i];
			dest[0] = GetHex((b >> 4) & 0xF);
			dest[1] = GetHex(b & 0xF);
			dest += step;
		}
	}
}
