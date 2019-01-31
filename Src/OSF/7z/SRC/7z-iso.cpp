// 7Z-ISO.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
using namespace NTime;

// IsoHandler.cpp IsoIn.cpp
struct CSeekExtent {
	uint64 Phy;
	uint64 Virt;
};

class CExtentsStream : public IInStream, public CMyUnknownImp {
public:
	CMyComPtr <IInStream> Stream;
	CRecordVector <CSeekExtent> Extents;
	MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
	STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	void ReleaseStream();
	void Init();
private:
	HRESULT SeekToPhys();

	uint64 _phyPos;
	uint64 _virtPos;
	bool   _needStartSeek;
};

void CExtentsStream::Init()
{
	_virtPos = 0;
	_phyPos = 0;
	_needStartSeek = true;
}

void CExtentsStream::ReleaseStream() { Stream.Release(); }
HRESULT CExtentsStream::SeekToPhys() { return Stream->Seek(_phyPos, STREAM_SEEK_SET, NULL); }

STDMETHODIMP CExtentsStream::Read(void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	if(_virtPos >= Extents.Back().Virt)
		return S_OK;
	if(size == 0)
		return S_OK;
	unsigned left = 0, right = Extents.Size() - 1;
	for(;; ) {
		unsigned mid = (left + right) / 2;
		if(mid == left)
			break;
		if(_virtPos < Extents[mid].Virt)
			right = mid;
		else
			left = mid;
	}
	const CSeekExtent &extent = Extents[left];
	uint64 phyPos = extent.Phy + (_virtPos - extent.Virt);
	if(_needStartSeek || _phyPos != phyPos) {
		_needStartSeek = false;
		_phyPos = phyPos;
		RINOK(SeekToPhys());
	}
	uint64 rem = Extents[left + 1].Virt - _virtPos;
	SETMIN(size, (uint32)rem);
	HRESULT res = Stream->Read(data, size, &size);
	_phyPos += size;
	_virtPos += size;
	ASSIGN_PTR(processedSize, size);
	return res;
}

STDMETHODIMP CExtentsStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	switch(seekOrigin) {
		case STREAM_SEEK_SET: break;
		case STREAM_SEEK_CUR: offset += _virtPos; break;
		case STREAM_SEEK_END: offset += Extents.Back().Virt; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if(offset < 0)
		return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
	else {
		_virtPos = offset;
		ASSIGN_PTR(newPosition, _virtPos);
		return S_OK;
	}
}

namespace NArchive {
	namespace NIso {
		static const Byte kProps[] =
		{
			kpidPath,
			kpidIsDir,
			kpidSize,
			kpidPackSize,
			kpidMTime,
			// kpidCTime,
			// kpidATime,
			kpidPosixAttrib,
			// kpidUser,
			// kpidGroup,
			// kpidLinks,
			kpidSymLink
		};

		static const Byte kArcProps[] =
		{
			kpidComment,
			kpidCTime,
			kpidMTime,
			// kpidHeadersSize
		};

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 * /* maxCheckStartPosition */, IArchiveOpenCallback * /* openArchiveCallback */)
		{
			COM_TRY_BEGIN
			Close();
			{
				RINOK(_archive.Open(stream));
				_stream = stream;
			}
			return S_OK;
			COM_TRY_END
		}
		STDMETHODIMP CHandler::Close()
		{
			_archive.Clear();
			_stream.Release();
			return S_OK;
		}
		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _archive.Refs.Size() + _archive.BootEntries.Size();
			return S_OK;
		}

		static void AddString(AString &s, const char * name, const Byte * p, uint size)
		{
			uint i;
			for(i = 0; i < size && p[i]; i++) ;
			for(; i > 0 && p[i - 1] == ' '; i--) ;
			if(i != 0) {
				AString d;
				d.SetFrom((const char*)p, i);
				s += '\n';
				s += name;
				s += ": ";
				s += d;
			}
		}

		#define ADD_STRING(n, v) AddString(s, n, vol.v, sizeof(vol.v))

		static void AddErrorMessage(AString &s, const char * message)
		{
			if(!s.IsEmpty())
				s += ". ";
			s += message;
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			if(_stream) {
				const CVolumeDescriptor &vol = _archive.VolDescs[_archive.MainVolDescIndex];
				switch(propID)
				{
					case kpidComment:
					{
						AString s;
						ADD_STRING("System", SystemId);
						ADD_STRING("Volume", VolumeId);
						ADD_STRING("VolumeSet", VolumeSetId);
						ADD_STRING("Publisher", PublisherId);
						ADD_STRING("Preparer", DataPreparerId);
						ADD_STRING("Application", ApplicationId);
						ADD_STRING("Copyright", CopyrightFileId);
						ADD_STRING("Abstract", AbstractFileId);
						ADD_STRING("Bib", BibFileId);
						prop = s;
						break;
					}
					case kpidCTime: { FILETIME utc; if(vol.CTime.GetFileTime(utc)) prop = utc; break; }
					case kpidMTime: { FILETIME utc; if(vol.MTime.GetFileTime(utc)) prop = utc; break; }
				}
			}
			switch(propID) {
				case kpidPhySize: prop = _archive.PhySize; break;
				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!_archive.IsArc) v |= kpv_ErrorFlags_IsNotArc;
					if(_archive.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
					if(_archive.HeadersError) v |= kpv_ErrorFlags_HeadersError;
					prop = v;
					break;
				}
				case kpidError:
				{
					AString s;
					if(_archive.IncorrectBigEndian)
						AddErrorMessage(s, "Incorrect big-endian headers");
					if(_archive.SelfLinkedDirs)
						AddErrorMessage(s, "Self-linked directory");
					if(_archive.TooDeepDirs)
						AddErrorMessage(s, "Too deep directory levels");
					if(!s.IsEmpty())
						prop = s;
					break;
				}
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}
		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			if(index >= (uint32)_archive.Refs.Size()) {
				index -= _archive.Refs.Size();
				const CBootInitialEntry &be = _archive.BootEntries[index];
				switch(propID) {
					case kpidPath:
					{
						AString s("[BOOT]" STRING_PATH_SEPARATOR);
						if(_archive.BootEntries.Size() != 1) {
							s.Add_UInt32(index + 1);
							s += '-';
						}
						s += be.GetName();
						prop = s;
						break;
					}
					case kpidIsDir: prop = false; break;
					case kpidSize:
					case kpidPackSize:
						prop = (uint64)_archive.GetBootItemSize(index);
						break;
				}
			}
			else {
				const CRef & ref = _archive.Refs[index];
				const CDir & item = ref.Dir->_subItems[ref.Index];
				switch(propID) {
					case kpidPath:
						// if(item.FileId.GetCapacity() >= 0)
					{
						UString s;
						if(_archive.IsJoliet())
							item.GetPathU(s);
						else
							s = MultiByteToUnicodeString(item.GetPath(_archive.IsSusp, _archive.SuspSkipSize), CP_OEMCP);
						if(s.Len() >= 2 && s[s.Len() - 2] == ';' && s.Back() == '1')
							s.DeleteFrom(s.Len() - 2);
						if(!s.IsEmpty() && s.Back() == L'.')
							s.DeleteBack();
						NItemName::ReplaceToOsSlashes_Remove_TailSlash(s);
						prop = s;
					}
					break;

					case kpidSymLink:
						if(_archive.IsSusp) {
							UString s;
							uint32 mode;
							if(item.GetPx(_archive.SuspSkipSize, k_Px_Mode, mode)) {
								if(((mode >> 12) & 0xF) == 10) {
									AString s8;
									if(item.GetSymLink(_archive.SuspSkipSize, s8)) {
										s = MultiByteToUnicodeString(s8, CP_OEMCP);
										prop = s;
									}
								}
							}
						}
						break;

					case kpidPosixAttrib:
						/*
						   case kpidLinks:
						   case kpidUser:
						   case kpidGroup:
						 */
					{
						if(_archive.IsSusp) {
							uint32 t = 0;
							switch(propID) {
								case kpidPosixAttrib: t = k_Px_Mode; break;
								/*
								   case kpidLinks: t = k_Px_Links; break;
								   case kpidUser: t = k_Px_User; break;
								   case kpidGroup: t = k_Px_Group; break;
								 */
							}
							uint32 v;
							if(item.GetPx(_archive.SuspSkipSize, t, v))
								prop = v;
						}
						break;
					}

					case kpidIsDir: prop = item.IsDir(); break;
					case kpidSize:
					case kpidPackSize:
						if(!item.IsDir())
							prop = (uint64)ref.TotalSize;
						break;

					case kpidMTime:
						// case kpidCTime:
						// case kpidATime:
					{
						FILETIME utc;
						if(/* propID == kpidMTime && */ item.DateTime.GetFileTime(utc))
							prop = utc;
						/*
						   else
						   {
						   uint32 t = 0;
						   switch (propID)
						   {
							case kpidMTime: t = k_Tf_MTime; break;
							case kpidCTime: t = k_Tf_CTime; break;
							case kpidATime: t = k_Tf_ATime; break;
						   }
						   CRecordingDateTime dt;
						   if(item.GetTf(_archive.SuspSkipSize, t, dt))
						   {
							FILETIME utc;
							if(dt.GetFileTime(utc))
							  prop = utc;
						   }
						   }
						 */
						break;
					}
				}
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}
		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _archive.Refs.Size();
			if(numItems == 0)
				return S_OK;
			uint64 totalSize = 0;
			uint32 i;
			for(i = 0; i < numItems; i++) {
				uint32 index = (allFilesMode ? i : indices[i]);
				if(index < (uint32)_archive.Refs.Size()) {
					const CRef &ref = _archive.Refs[index];
					const CDir &item = ref.Dir->_subItems[ref.Index];
					if(!item.IsDir())
						totalSize += ref.TotalSize;
				}
				else
					totalSize += _archive.GetBootItemSize(index - _archive.Refs.Size());
			}
			extractCallback->SetTotal(totalSize);

			uint64 currentTotalSize = 0;
			uint64 currentItemSize;

			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder();
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);

			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> inStream(streamSpec);
			streamSpec->SetStream(_stream);

			for(i = 0; i < numItems; i++, currentTotalSize += currentItemSize) {
				lps->InSize = lps->OutSize = currentTotalSize;
				RINOK(lps->SetCur());
				currentItemSize = 0;
				CMyComPtr<ISequentialOutStream> realOutStream;
				int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				uint32 index = allFilesMode ? i : indices[i];
				RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
				uint64 blockIndex;
				if(index < (uint32)_archive.Refs.Size()) {
					const CRef &ref = _archive.Refs[index];
					const CDir &item = ref.Dir->_subItems[ref.Index];
					if(item.IsDir()) {
						RINOK(extractCallback->PrepareOperation(askMode));
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
						continue;
					}
					currentItemSize = ref.TotalSize;
					blockIndex = item.ExtentLocation;
				}
				else {
					unsigned bootIndex = index - _archive.Refs.Size();
					const CBootInitialEntry &be = _archive.BootEntries[bootIndex];
					currentItemSize = _archive.GetBootItemSize(bootIndex);
					blockIndex = be.LoadRBA;
				}
				if(!testMode && !realOutStream)
					continue;
				RINOK(extractCallback->PrepareOperation(askMode));
				bool isOK = true;
				if(index < (uint32)_archive.Refs.Size()) {
					const CRef &ref = _archive.Refs[index];
					uint64 offset = 0;
					for(uint32 e = 0; e < ref.NumExtents; e++) {
						const CDir &item2 = ref.Dir->_subItems[ref.Index + e];
						if(item2.Size) {
							lps->InSize = lps->OutSize = currentTotalSize + offset;
							RINOK(_stream->Seek((uint64)item2.ExtentLocation * kBlockSize, STREAM_SEEK_SET, NULL));
							streamSpec->Init(item2.Size);
							RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
							if(copyCoderSpec->TotalSize != item2.Size) {
								isOK = false;
								break;
							}
							offset += item2.Size;
						}
					}
				}
				else {
					RINOK(_stream->Seek((uint64)blockIndex * kBlockSize, STREAM_SEEK_SET, NULL));
					streamSpec->Init(currentItemSize);
					RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
					if(copyCoderSpec->TotalSize != currentItemSize)
						isOK = false;
				}
				realOutStream.Release();
				RINOK(extractCallback->SetOperationResult(isOK ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kDataError));
			}
			return S_OK;
			COM_TRY_END
		}
		STDMETHODIMP CHandler::GetStream(uint32 index, ISequentialInStream ** stream)
		{
			COM_TRY_BEGIN
			*stream = 0;
			uint64 blockIndex;
			uint64 currentItemSize;
			if(index < _archive.Refs.Size()) {
				const CRef &ref = _archive.Refs[index];
				const CDir &item = ref.Dir->_subItems[ref.Index];
				if(item.IsDir())
					return S_FALSE;
				if(ref.NumExtents > 1) {
					CExtentsStream * extentStreamSpec = new CExtentsStream();
					CMyComPtr<ISequentialInStream> extentStream = extentStreamSpec;
					extentStreamSpec->Stream = _stream;
					uint64 virtOffset = 0;
					for(uint32 i = 0; i < ref.NumExtents; i++) {
						const CDir &item2 = ref.Dir->_subItems[ref.Index + i];
						if(item2.Size) {
							CSeekExtent se;
							se.Phy = (uint64)item2.ExtentLocation * kBlockSize;
							se.Virt = virtOffset;
							extentStreamSpec->Extents.Add(se);
							virtOffset += item2.Size;
						}
					}
					if(virtOffset != ref.TotalSize)
						return S_FALSE;
					CSeekExtent se;
					se.Phy = 0;
					se.Virt = virtOffset;
					extentStreamSpec->Extents.Add(se);
					extentStreamSpec->Init();
					*stream = extentStream.Detach();
					return S_OK;
				}
				currentItemSize = item.Size;
				blockIndex = item.ExtentLocation;
			}
			else {
				unsigned bootIndex = index - _archive.Refs.Size();
				const CBootInitialEntry &be = _archive.BootEntries[bootIndex];
				currentItemSize = _archive.GetBootItemSize(bootIndex);
				blockIndex = be.LoadRBA;
			}
			return CreateLimitedInStream(_stream, (uint64)blockIndex * kBlockSize, currentItemSize, stream);
			COM_TRY_END
		}
		//
		// IsoIn
		//
		struct CUnexpectedEndException {};
		struct CHeaderErrorException {};
		struct CEndianErrorException {};

		static const char * const kMediaTypes[] = { "NoEmul", "1.2M", "1.44M", "2.88M", "HardDisk" };

		bool CBootInitialEntry::Parse(const Byte * p)
		{
			Bootable = (p[0] == NBootEntryId::kInitialEntryBootable);
			BootMediaType = p[1];
			LoadSegment = GetUi16(p + 2);
			SystemType = p[4];
			SectorCount = GetUi16(p + 6);
			LoadRBA = GetUi32(p + 8);
			memcpy(VendorSpec, p + 12, 20);
			if(p[5] != 0)
				return false;
			if(p[0] != NBootEntryId::kInitialEntryBootable && p[0] != NBootEntryId::kInitialEntryNotBootable)
				return false;
			return true;
		}

		AString CBootInitialEntry::GetName() const
		{
			AString s(Bootable ? "Boot" : "NotBoot");
			s += '-';
			if(BootMediaType < ARRAY_SIZE(kMediaTypes))
				s += kMediaTypes[BootMediaType];
			else
				s.Add_UInt32(BootMediaType);
			if(VendorSpec[0] == 1) {
				// "Language and Version Information (IBM)"
				uint i;
				for(i = 1; i < sizeof(VendorSpec); i++)
					if(VendorSpec[i] > 0x7F)
						break;
				if(i == sizeof(VendorSpec)) {
					s += '-';
					for(i = 1; i < sizeof(VendorSpec); i++) {
						char c = VendorSpec[i];
						if(c == 0)
							break;
						if(c == '\\' || c == '/')
							c = '_';
						s += c;
					}
				}
			}
			s += ".img";
			return s;
		}

		void CDir::Clear()
		{
			Parent = 0;
			_subItems.Clear();
		}

		AString CDir::GetPath(bool checkSusp, unsigned skipSize) const
		{
			AString s;
			uint len = 0;
			const CDir * cur = this;
			for(;; ) {
				unsigned curLen;
				cur->GetNameCur(checkSusp, skipSize, curLen);
				len += curLen;
				cur = cur->Parent;
				if(!cur || !cur->Parent)
					break;
				len++;
			}
			char * p = s.GetBuf_SetEnd(len) + len;
			cur = this;
			for(;; ) {
				unsigned curLen;
				const Byte * name = cur->GetNameCur(checkSusp, skipSize, curLen);
				p -= curLen;
				if(curLen != 0)
					memcpy(p, name, curLen);
				cur = cur->Parent;
				if(!cur || !cur->Parent)
					break;
				p--;
				*p = CHAR_PATH_SEPARATOR;
			}
			return s;
		}

		void FASTCALL CDir::GetPathU(UString &s) const
		{
			s.Empty();
			uint len = 0;
			const CDir * cur = this;
			for(;; ) {
				unsigned curLen = (uint)(cur->FileId.Size() / 2);
				const Byte * fid = cur->FileId;

				uint i;
				for(i = 0; i < curLen; i++)
					if(fid[i * 2] == 0 && fid[i * 2 + 1] == 0)
						break;
				len += i;
				cur = cur->Parent;
				if(!cur || !cur->Parent)
					break;
				len++;
			}
			wchar_t * p = s.GetBuf_SetEnd(len) + len;
			cur = this;
			for(;; ) {
				unsigned curLen = (uint)(cur->FileId.Size() / 2);
				const Byte * fid = cur->FileId;
				uint i;
				for(i = 0; i < curLen; i++)
					if(fid[i * 2] == 0 && fid[i * 2 + 1] == 0)
						break;
				curLen = i;
				p -= curLen;
				for(i = 0; i < curLen; i++)
					p[i] = (wchar_t)(((wchar_t)fid[i * 2] << 8) | fid[i * 2 + 1]);
				cur = cur->Parent;
				if(!cur || !cur->Parent)
					break;
				p--;
				*p = WCHAR_PATH_SEPARATOR;
			}
		}

		void CInArchive::UpdatePhySize(uint32 blockIndex, uint64 size)
		{
			const uint64 alignedSize = (size + kBlockSize - 1) & ~((uint64)kBlockSize - 1);
			const uint64 end = (uint64)blockIndex * kBlockSize + alignedSize;
			SETMAX(PhySize, end);
		}

		bool CInArchive::IsJoliet() const { return VolDescs[MainVolDescIndex].IsJoliet(); }

		uint64 FASTCALL CInArchive::GetBootItemSize(int index) const
		{
			const CBootInitialEntry &be = BootEntries[index];
			uint64 size = be.GetSize();
			if(be.BootMediaType == NBootMediaType::k1d2Floppy)
				size = (1200 << 10);
			else if(be.BootMediaType == NBootMediaType::k1d44Floppy)
				size = (1440 << 10);
			else if(be.BootMediaType == NBootMediaType::k2d88Floppy)
				size = (2880 << 10);
			uint64 startPos = (uint64)be.LoadRBA * kBlockSize;
			if(startPos < _fileSize) {
				if(_fileSize - startPos < size)
					size = _fileSize - startPos;
			}
			return size;
		}

		Byte CInArchive::ReadByte()
		{
			if(m_BufferPos >= kBlockSize)
				m_BufferPos = 0;
			if(m_BufferPos == 0) {
				size_t processed = kBlockSize;
				HRESULT res = ReadStream(_stream, m_Buffer, &processed);
				if(res != S_OK)
					throw CSystemException(res);
				if(processed != kBlockSize)
					throw CUnexpectedEndException();
				uint64 end = _position + processed;
				if(PhySize < end)
					PhySize = end;
			}
			Byte b = m_Buffer[m_BufferPos++];
			_position++;
			return b;
		}
		void CInArchive::ReadBytes(Byte * data, uint32 size)
		{
			for(uint32 i = 0; i < size; i++)
				data[i] = ReadByte();
		}
		void CInArchive::Skip(size_t size)
		{
			while(size-- != 0)
				ReadByte();
		}
		void CInArchive::SkipZeros(size_t size)
		{
			while(size-- != 0) {
				Byte b = ReadByte();
				if(b != 0)
					throw CHeaderErrorException();
			}
		}
		uint16 CInArchive::ReadUInt16()
		{
			Byte b[4];
			ReadBytes(b, 4);
			uint32 val = 0;
			for(int i = 0; i < 2; i++) {
				if(b[i] != b[3 - i])
					IncorrectBigEndian = true;
				val |= ((uint16)(b[i]) << (8 * i));
			}
			return (uint16)val;
		}
		uint32 CInArchive::ReadUInt32Le()
		{
			uint32 val = 0;
			for(int i = 0; i < 4; i++)
				val |= ((uint32)(ReadByte()) << (8 * i));
			return val;
		}
		uint32 CInArchive::ReadUInt32Be()
		{
			uint32 val = 0;
			for(int i = 0; i < 4; i++) {
				val <<= 8;
				val |= ReadByte();
			}
			return val;
		}
		uint32 CInArchive::ReadUInt32()
		{
			Byte b[8];
			ReadBytes(b, 8);
			uint32 val = 0;
			for(int i = 0; i < 4; i++) {
				if(b[i] != b[7 - i])
					throw CEndianErrorException();
				val |= ((uint32)(b[i]) << (8 * i));
			}
			return val;
		}
		uint32 CInArchive::ReadDigits(int numDigits)
		{
			uint32 res = 0;
			for(int i = 0; i < numDigits; i++) {
				Byte b = ReadByte();
				if(b < '0' || b > '9') {
					if(b == 0) // it's bug in some CD's
						b = '0';
					else
						throw CHeaderErrorException();
				}
				uint32 d = (uint32)(b - '0');
				res *= 10;
				res += d;
			}
			return res;
		}

		bool CDateTime::NotSpecified() const 
		{
			return Year == 0 && Month == 0 && Day == 0 && Hour == 0 && Minute == 0 && Second == 0 && GmtOffset == 0;
		}

		bool FASTCALL CDateTime::GetFileTime(FILETIME &ft) const
		{
			uint64 value;
			bool res = NWindows::NTime::GetSecondsSince1601(Year, Month, Day, Hour, Minute, Second, value);
			if(res) {
				value -= (int64)((int32)GmtOffset * 15 * 60);
				value *= 10000000;
			}
			ft.dwLowDateTime = (DWORD)value;
			ft.dwHighDateTime = (DWORD)(value >> 32);
			return res;
		}

		void CInArchive::ReadDateTime(CDateTime &d)
		{
			d.Year = (uint16)ReadDigits(4);
			d.Month = (Byte)ReadDigits(2);
			d.Day = (Byte)ReadDigits(2);
			d.Hour = (Byte)ReadDigits(2);
			d.Minute = (Byte)ReadDigits(2);
			d.Second = (Byte)ReadDigits(2);
			d.Hundredths = (Byte)ReadDigits(2);
			d.GmtOffset = (signed char)ReadByte();
		}

		void CInArchive::ReadBootRecordDescriptor(CBootRecordDescriptor &d)
		{
			ReadBytes(d.BootSystemId, sizeof(d.BootSystemId));
			ReadBytes(d.BootId, sizeof(d.BootId));
			ReadBytes(d.BootSystemUse, sizeof(d.BootSystemUse));
		}

		void CInArchive::ReadRecordingDateTime(CRecordingDateTime &t)
		{
			t.Year = ReadByte();
			t.Month = ReadByte();
			t.Day = ReadByte();
			t.Hour = ReadByte();
			t.Minute = ReadByte();
			t.Second = ReadByte();
			t.GmtOffset = (signed char)ReadByte();
		}

		void CInArchive::ReadDirRecord2(CDirRecord &r, Byte len)
		{
			r.ExtendedAttributeRecordLen = ReadByte();
			if(r.ExtendedAttributeRecordLen != 0)
				throw CHeaderErrorException();
			r.ExtentLocation = ReadUInt32();
			r.Size = ReadUInt32();
			ReadRecordingDateTime(r.DateTime);
			r.FileFlags = ReadByte();
			r.FileUnitSize = ReadByte();
			r.InterleaveGapSize = ReadByte();
			r.VolSequenceNumber = ReadUInt16();
			Byte idLen = ReadByte();
			r.FileId.Alloc(idLen);
			ReadBytes((Byte*)r.FileId, idLen);
			unsigned padSize = 1 - (idLen & 1);

			// SkipZeros(padSize);
			Skip(padSize); // it's bug in some cd's. Must be zeros
			unsigned curPos = 33 + idLen + padSize;
			if(curPos > len)
				throw CHeaderErrorException();
			unsigned rem = len - curPos;
			r.SystemUse.Alloc(rem);
			ReadBytes((Byte*)r.SystemUse, rem);
		}

		void CInArchive::ReadDirRecord(CDirRecord &r)
		{
			Byte len = ReadByte();
			// Some CDs can have incorrect value len = 48 ('0') in VolumeDescriptor.
			// But maybe we must use real "len" for other records.
			len = 34;
			ReadDirRecord2(r, len);
		}

		bool CVolumeDescriptor::IsJoliet() const
		{
			if((VolFlags & 1) != 0)
				return false;
			else {
				Byte b = EscapeSequence[2];
				return (EscapeSequence[0] == 0x25 && EscapeSequence[1] == 0x2F && (b == 0x40 || b == 0x43 || b == 0x45));
			}
		}

		void CInArchive::ReadVolumeDescriptor(CVolumeDescriptor & d)
		{
			d.VolFlags = ReadByte();
			ReadBytes(d.SystemId, sizeof(d.SystemId));
			ReadBytes(d.VolumeId, sizeof(d.VolumeId));
			SkipZeros(8);
			d.VolumeSpaceSize = ReadUInt32();
			ReadBytes(d.EscapeSequence, sizeof(d.EscapeSequence));
			d.VolumeSetSize = ReadUInt16();
			d.VolumeSequenceNumber = ReadUInt16();
			d.LogicalBlockSize = ReadUInt16();
			d.PathTableSize = ReadUInt32();
			d.LPathTableLocation = ReadUInt32Le();
			d.LOptionalPathTableLocation = ReadUInt32Le();
			d.MPathTableLocation = ReadUInt32Be();
			d.MOptionalPathTableLocation = ReadUInt32Be();
			ReadDirRecord(d.RootDirRecord);
			ReadBytes(d.VolumeSetId, sizeof(d.VolumeSetId));
			ReadBytes(d.PublisherId, sizeof(d.PublisherId));
			ReadBytes(d.DataPreparerId, sizeof(d.DataPreparerId));
			ReadBytes(d.ApplicationId, sizeof(d.ApplicationId));
			ReadBytes(d.CopyrightFileId, sizeof(d.CopyrightFileId));
			ReadBytes(d.AbstractFileId, sizeof(d.AbstractFileId));
			ReadBytes(d.BibFileId, sizeof(d.BibFileId));
			ReadDateTime(d.CTime);
			ReadDateTime(d.MTime);
			ReadDateTime(d.ExpirationTime);
			ReadDateTime(d.EffectiveTime);
			d.FileStructureVersion = ReadByte(); // = 1
			SkipZeros(1);
			ReadBytes(d.ApplicationUse, sizeof(d.ApplicationUse));

			// Most ISO contains zeros in the following field (reserved for future standardization).
			// But some ISO programs write some data to that area.
			// So we disable check for zeros.
			Skip(653); // SkipZeros(653);
		}

		static const Byte kSig_CD001[5] = { 'C', 'D', '0', '0', '1' };
		static const Byte kSig_NSR02[5] = { 'N', 'S', 'R', '0', '2' };
		static const Byte kSig_NSR03[5] = { 'N', 'S', 'R', '0', '3' };
		static const Byte kSig_BEA01[5] = { 'B', 'E', 'A', '0', '1' };
		static const Byte kSig_TEA01[5] = { 'T', 'E', 'A', '0', '1' };

		static inline bool CheckSignature(const Byte * sig, const Byte * data)
		{
			for(int i = 0; i < 5; i++)
				if(sig[i] != data[i])
					return false;
			return true;
		}

		void CInArchive::SeekToBlock(uint32 blockIndex)
		{
			HRESULT res = _stream->Seek((uint64)blockIndex * VolDescs[MainVolDescIndex].LogicalBlockSize, STREAM_SEEK_SET, &_position);
			if(res != S_OK)
				throw CSystemException(res);
			m_BufferPos = 0;
		}

		static const int kNumLevelsMax = 256;

		void CInArchive::ReadDir(CDir &d, int level)
		{
			if(d.IsDir()) {
				if(level > kNumLevelsMax) {
					TooDeepDirs = true;
				}
				else {
					{
						FOR_VECTOR(i, UniqStartLocations) {
							if(UniqStartLocations[i] == d.ExtentLocation) {
								SelfLinkedDirs = true;
								return;
							}
						}
						UniqStartLocations.Add(d.ExtentLocation);
					}
					SeekToBlock(d.ExtentLocation);
					uint64 startPos = _position;
					bool firstItem = true;
					for(;; ) {
						const uint64 offset = _position - startPos;
						if(offset >= d.Size)
							break;
						const Byte len = ReadByte();
						if(len) {
							CDir subItem;
							ReadDirRecord2(subItem, len);
							if(firstItem && level == 0)
								IsSusp = subItem.CheckSusp(SuspSkipSize);
							if(!subItem.IsSystemItem())
								d._subItems.Add(subItem);
							firstItem = false;
						}
					}
					FOR_VECTOR(i, d._subItems) {
						ReadDir(d._subItems[i], level + 1);
					}
					UniqStartLocations.DeleteBack();
				}
			}
		}

		void CInArchive::CreateRefs(CDir & d)
		{
			if(d.IsDir()) {
				for(uint i = 0; i < d._subItems.Size(); ) {
					CRef ref;
					CDir & subItem = d._subItems[i];
					subItem.Parent = &d;
					ref.Dir = &d;
					ref.Index = i++;
					ref.NumExtents = 1;
					ref.TotalSize = subItem.Size;
					if(subItem.IsNonFinalExtent()) {
						for(;; ) {
							if(i == d._subItems.Size()) {
								HeadersError = true;
								break;
							}
							const CDir &next = d._subItems[i];
							if(!subItem.AreMultiPartEqualWith(next))
								break;
							i++;
							ref.NumExtents++;
							ref.TotalSize += next.Size;
							if(!next.IsNonFinalExtent())
								break;
						}
					}
					Refs.Add(ref);
					CreateRefs(subItem);
				}
			}
		}

		void CInArchive::ReadBootInfo()
		{
			if(!_bootIsDefined)
				return;
			HeadersError = true;
			if(memcmp(_bootDesc.BootSystemId, kElToritoSpec, sizeof(_bootDesc.BootSystemId)) != 0)
				return;
			uint32 blockIndex = GetUi32(_bootDesc.BootSystemUse);
			SeekToBlock(blockIndex);
			Byte buf[32];
			ReadBytes(buf, 32);
			if(buf[0] != NBootEntryId::kValidationEntry || buf[2] != 0 || buf[3] != 0 || buf[30] != 0x55 || buf[31] != 0xAA)
				return;
			{
				uint32 sum = 0;
				for(uint i = 0; i < 32; i += 2)
					sum += GetUi16(buf + i);
				if((sum & 0xFFFF) != 0)
					return;
				/*
				   CBootValidationEntry e;
				   e.PlatformId = buf[1];
				   memcpy(e.Id, buf + 4, sizeof(e.Id));
				   // uint16 checkSum = GetUi16(p + 28);
				 */
			}

			ReadBytes(buf, 32);
			{
				CBootInitialEntry e;
				if(!e.Parse(buf))
					return;
				BootEntries.Add(e);
			}

			bool error = false;

			for(;; ) {
				ReadBytes(buf, 32);
				Byte headerIndicator = buf[0];
				if(headerIndicator != NBootEntryId::kMoreHeaders
							&& headerIndicator != NBootEntryId::kFinalHeader)
					break;

				// Section Header
				// Byte platform = p[1];
				uint   numEntries = GetUi16(buf + 2);
				// id[28]

				for(uint i = 0; i < numEntries; i++) {
					ReadBytes(buf, 32);
					CBootInitialEntry e;
					if(!e.Parse(buf)) {
						error = true;
						break;
					}
					if(e.BootMediaType & (1 << 5)) {
						// Section entry extension
						for(uint j = 0;; j++) {
							ReadBytes(buf, 32);
							if(j > 32 || buf[0] != NBootEntryId::kExtensionIndicator) {
								error = true;
								break;
							}
							if((buf[1] & (1 << 5)) == 0)
								break;
							// info += (buf + 2, 30)
						}
					}
					BootEntries.Add(e);
				}

				if(headerIndicator != NBootEntryId::kMoreHeaders)
					break;
			}

			HeadersError = error;
		}

		HRESULT CInArchive::Open2()
		{
			_position = 0;
			RINOK(_stream->Seek(0, STREAM_SEEK_END, &_fileSize));
			if(_fileSize < kStartPos)
				return S_FALSE;
			RINOK(_stream->Seek(kStartPos, STREAM_SEEK_SET, &_position));

			PhySize = _position;
			m_BufferPos = 0;
			// BlockSize = kBlockSize;

			for(;; ) {
				Byte sig[7];
				ReadBytes(sig, 7);
				Byte ver = sig[6];

				if(!CheckSignature(kSig_CD001, sig + 1)) {
					return S_FALSE;
					/*
					   if(sig[0] != 0 || ver != 1)
					   break;
					   if(CheckSignature(kSig_BEA01, sig + 1))
					   {
					   }
					   else if(CheckSignature(kSig_TEA01, sig + 1))
					   {
					   break;
					   }
					   else if(CheckSignature(kSig_NSR02, sig + 1))
					   {
					   }
					   else
					   break;
					   SkipZeros(0x800 - 7);
					   continue;
					 */
				}

				// version = 2 for ISO 9660:1999?
				if(ver > 2)
					return S_FALSE;

				if(sig[0] == NVolDescType::kTerminator) {
					break;
					// Skip(0x800 - 7);
					// continue;
				}

				switch(sig[0])
				{
					case NVolDescType::kBootRecord:
					{
						_bootIsDefined = true;
						ReadBootRecordDescriptor(_bootDesc);
						break;
					}
					case NVolDescType::kPrimaryVol:
					case NVolDescType::kSupplementaryVol:
					{
						// some ISOs have two PrimaryVols.
						CVolumeDescriptor vd;
						ReadVolumeDescriptor(vd);
						if(sig[0] == NVolDescType::kPrimaryVol) {
							// some burners write "Joliet" Escape Sequence to primary volume
							memzero(vd.EscapeSequence, sizeof(vd.EscapeSequence));
						}
						VolDescs.Add(vd);
						break;
					}
					default:
						break;
				}
			}

			if(VolDescs.IsEmpty())
				return S_FALSE;
			for(MainVolDescIndex = VolDescs.Size() - 1; MainVolDescIndex > 0; MainVolDescIndex--)
				if(VolDescs[MainVolDescIndex].IsJoliet())
					break;
			// MainVolDescIndex = 0; // to read primary volume
			const CVolumeDescriptor &vd = VolDescs[MainVolDescIndex];
			if(vd.LogicalBlockSize != kBlockSize)
				return S_FALSE;

			IsArc = true;

			(CDirRecord &)_rootDir = vd.RootDirRecord;
			ReadDir(_rootDir, 0);
			CreateRefs(_rootDir);
			ReadBootInfo();

			{
				FOR_VECTOR(i, Refs)
				{
					const CRef &ref = Refs[i];
					for(uint32 j = 0; j < ref.NumExtents; j++) {
						const CDir &item = ref.Dir->_subItems[ref.Index + j];
						if(!item.IsDir() && item.Size != 0)
							UpdatePhySize(item.ExtentLocation, item.Size);
					}
				}
			}
			{
				FOR_VECTOR(i, BootEntries)
				{
					const CBootInitialEntry &be = BootEntries[i];
					UpdatePhySize(be.LoadRBA, GetBootItemSize(i));
				}
			}

			if(PhySize < _fileSize) {
				uint64 rem = _fileSize - PhySize;
				const uint64 kRemMax = 1 << 21;
				if(rem <= kRemMax) {
					RINOK(_stream->Seek(PhySize, STREAM_SEEK_SET, NULL));
					bool areThereNonZeros = false;
					uint64 numZeros = 0;
					RINOK(ReadZeroTail(_stream, areThereNonZeros, numZeros, kRemMax));
					if(!areThereNonZeros)
						PhySize += numZeros;
				}
			}

			return S_OK;
		}

		HRESULT CInArchive::Open(IInStream * inStream)
		{
			Clear();
			_stream = inStream;
			try { return Open2(); }
			catch(const CSystemException &e) { return e.ErrorCode; }
			catch(CUnexpectedEndException &) { UnexpectedEnd = true; return S_FALSE; }
			catch(CHeaderErrorException &) { HeadersError = true; return S_FALSE; }
			catch(CEndianErrorException &) { IncorrectBigEndian = true; return S_FALSE; }
		}

		void CInArchive::Clear()
		{
			IsArc = false;
			UnexpectedEnd = false;
			HeadersError = false;
			IncorrectBigEndian = false;
			TooDeepDirs = false;
			SelfLinkedDirs = false;

			UniqStartLocations.Clear();

			Refs.Clear();
			_rootDir.Clear();
			VolDescs.Clear();
			_bootIsDefined = false;
			BootEntries.Clear();
			SuspSkipSize = 0;
			IsSusp = false;
		}
	}
}
