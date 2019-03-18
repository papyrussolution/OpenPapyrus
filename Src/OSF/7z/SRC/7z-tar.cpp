// 7Z-TAR.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

// TarIn.cpp TarOut.cpp TarUpdate.cpp TarHandler.cpp TarHandlerOut.cpp
namespace NArchive { 
	namespace NTar {
		static const UINT k_DefaultCodePage = CP_OEMCP; // it uses it if UTF8 check in names shows error
		static const Byte kProps[] = { kpidPath, kpidIsDir, kpidSize, kpidPackSize, kpidMTime, kpidPosixAttrib, kpidUser, kpidGroup, kpidSymLink, kpidHardLink, /*kpidLinkType*/ };
		static const Byte kArcProps[] = { kpidHeadersSize, kpidCodePage };

		static void FASTCALL MyStrNCpy(char * dest, const char * src, uint size)
		{
			for(uint i = 0; i < size; i++) {
				char c = src[i];
				dest[i] = c;
				if(c == 0)
					break;
			}
		}
		static bool FASTCALL OctalToNumber(const char * srcString, uint size, uint64 &res)
		{
			char sz[32];
			MyStrNCpy(sz, srcString, size);
			sz[size] = 0;
			const char * end;
			uint i;
			for(i = 0; sz[i] == ' '; i++) 
				;
			res = ConvertOctStringToUInt64(sz + i, &end);
			return (end == (sz + i)) ? false : (*end == ' ' || *end == 0);
		}
		static bool FASTCALL OctalToNumber32(const char * srcString, uint size, uint32 &res)
		{
			uint64 res64;
			if(!OctalToNumber(srcString, size, res64))
				return false;
			res = (uint32)res64;
			return (res64 <= 0xFFFFFFFF);
		}
		static bool FASTCALL WriteOctal_8(char * s, uint32 val)
		{
			const uint kNumDigits = 8 - 1;
			if(val >= ((uint32)1 << (kNumDigits * 3)))
				return false;
			else {
				for(uint i = 0; i < kNumDigits; i++) {
					s[kNumDigits - 1 - i] = (char)('0' + (val & 7));
					val >>= 3;
				}
				return true;
			}
		}
		static void FASTCALL WriteOctal_12(char * s, uint64 val)
		{
			const uint kNumDigits = 12 - 1;
			if(val >= ((uint64)1 << (kNumDigits * 3))) {
				// GNU extension;
				s[0] = (char)(Byte)0x80;
				s[1] = s[2] = s[3] = 0;
				for(uint i = 0; i < 8; i++, val <<= 8)
					s[4 + i] = (char)(val >> 56);
			}
			else {
				for(uint i = 0; i < kNumDigits; i++) {
					s[kNumDigits - 1 - i] = (char)('0' + (val & 7));
					val >>= 3;
				}
			}
		}
		static void WriteOctal_12_Signed(char * s, int64 val)
		{
			if(val >= 0)
				WriteOctal_12(s, val);
			else {
				s[0] = s[1] = s[2] = s[3] = (char)(Byte)0xFF;
				for(uint i = 0; i < 8; i++, val <<= 8)
					s[4 + i] = (char)(val >> 56);
			}
		}
		static bool FASTCALL CopyString(char * dest, const AString &src, unsigned maxSize)
		{
			if(src.Len() >= maxSize)
				return false;
			else {
				sstrcpy(dest, (const char *)src);
				return true;
			}
		}

		#define RIF(x) { if(!(x)) return S_OK; }

		/*
		   static bool IsEmptyData(const char *buf, size_t size)
		   {
		   for(unsigned i = 0; i < size; i++)
			if(buf[i] != 0)
			  return false;
		   return true;
		   }
		 */
		static bool IsRecordLast(const char * buf)
		{
			for(uint i = 0; i < NFileHeader::kRecordSize; i++)
				if(buf[i] != 0)
					return false;
			return true;
		}
		static void ReadString(const char * s, uint size, AString &result)
		{
			char temp[NFileHeader::kRecordSize + 1];
			MyStrNCpy(temp, s, size);
			temp[size] = '\0';
			result = temp;
		}
		static bool ParseInt64(const char * p, int64 &val)
		{
			uint32 h = GetBe32(p);
			val = GetBe64(p + 4);
			if(h == (uint32)1 << 31)
				return ((val >> 63) & 1) == 0;
			if(h == (uint32)(int32)-1)
				return ((val >> 63) & 1) != 0;
			uint64 uv;
			bool res = OctalToNumber(p, 12, uv);
			val = uv;
			return res;
		}
		static bool ParseInt64_MTime(const char * p, int64 &val)
		{
			// rare case tar contains spaces instead of MTime
			for(uint i = 0; i < 12; i++)
				if(p[i] != ' ')
					return ParseInt64(p, val);
			val = 0;
			return true;
		}
		static bool ParseSize(const char * p, uint64 &val)
		{
			if(GetBe32(p) == (uint32)1 << 31) {
				// GNU extension
				val = GetBe64(p + 4);
				return ((val >> 63) & 1) == 0;
			}
			else
				return OctalToNumber(p, 12, val);
		}

		#define CHECK(x) { if(!(x)) return k_IsArc_Res_NO; }

		API_FUNC_IsArc IsArc_Tar(const Byte * p2, size_t size)
		{
			if(size < NFileHeader::kRecordSize)
				return k_IsArc_Res_NEED_MORE;
			const char * p = (const char *)p2;
			p += NFileHeader::kNameSize;
			uint32 mode;
			CHECK(OctalToNumber32(p, 8, mode)); p += 8;
			// if(!OctalToNumber32(p, 8, item.UID)) item.UID = 0;
			p += 8;
			// if(!OctalToNumber32(p, 8, item.GID)) item.GID = 0;
			p += 8;
			uint64 packSize;
			int64 time;
			uint32 checkSum;
			CHECK(ParseSize(p, packSize)); p += 12;
			CHECK(ParseInt64_MTime(p, time)); p += 12;
			CHECK(OctalToNumber32(p, 8, checkSum));
			return k_IsArc_Res_YES;
		}

		bool   CItem::IsSymLink() const { return LinkFlag == NFileHeader::NLinkFlag::kSymLink && (Size == 0); }
		bool   CItem::IsHardLink() const { return LinkFlag == NFileHeader::NLinkFlag::kHardLink; }
		bool   CItem::IsSparse() const { return LinkFlag == NFileHeader::NLinkFlag::kSparse; }
		uint64 CItem::GetUnpackSize() const { return IsSymLink() ? LinkName.Len() : Size; }
		uint64 CItem::GetPackSizeAligned() const { return (PackSize + 0x1FF) & (~((uint64)0x1FF)); }
		bool   CItem::IsPaxExtendedHeader() const
		{
			switch(LinkFlag) {
				case 'g':
				case 'x':
				case 'X': // Check it
					return true;
			}
			return false;
		}
		uint32 CItem::Get_Combined_Mode() const
		{
			return (Mode & ~(uint32)MY_LIN_S_IFMT) | Get_FileTypeMode_from_LinkFlag();
		}
		uint32 CItem::Get_FileTypeMode_from_LinkFlag() const
		{
			switch(LinkFlag) {
				/*
					case NFileHeader::NLinkFlag::kDirectory:
					case NFileHeader::NLinkFlag::kDumpDir:
					return MY_LIN_S_IFDIR;
					*/
				case NFileHeader::NLinkFlag::kSymLink: return MY_LIN_S_IFLNK;
				case NFileHeader::NLinkFlag::kBlock: return MY_LIN_S_IFBLK;
				case NFileHeader::NLinkFlag::kCharacter: return MY_LIN_S_IFCHR;
				case NFileHeader::NLinkFlag::kFIFO: return MY_LIN_S_IFIFO;
					// case return MY_LIN_S_IFSOCK;
			}
			return IsDir() ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG;
		}
		bool CItem::IsDir() const
		{
			switch(LinkFlag) {
				case NFileHeader::NLinkFlag::kDirectory:
				case NFileHeader::NLinkFlag::kDumpDir: return true;
				case NFileHeader::NLinkFlag::kOldNormal:
				case NFileHeader::NLinkFlag::kNormal:
				case NFileHeader::NLinkFlag::kSymLink: return NItemName::HasTailSlash(Name, CP_OEMCP);
			}
			return false;
		}
		bool CItem::IsUstarMagic() const
		{
			for(int i = 0; i < 5; i++)
				if(Magic[i] != NFileHeader::NMagic::kUsTar_00[i])
					return false;
			return true;
		}

		static HRESULT GetNextItemReal(ISequentialInStream * stream, bool &filled, CItemEx & item, EErrorType &error)
		{
			char buf[NFileHeader::kRecordSize];
			char * p = buf;
			error = k_ErrorType_OK;
			filled = false;
			bool thereAreEmptyRecords = false;
			for(;; ) {
				size_t processedSize = NFileHeader::kRecordSize;
				RINOK(ReadStream(stream, buf, &processedSize));
				if(processedSize == 0) {
					if(!thereAreEmptyRecords)
						error = k_ErrorType_UnexpectedEnd;  // "There are no trailing zero-filled records";
					return S_OK;
				}
				if(processedSize != NFileHeader::kRecordSize) {
					if(!thereAreEmptyRecords)
						error = k_ErrorType_UnexpectedEnd;  // error = "There is no correct record at the end of archive";
					else {
						/*
						   if(IsEmptyData(buf, processedSize))
						   error = k_ErrorType_UnexpectedEnd;
						   else {
						   // extraReadSize = processedSize;
						   // error = k_ErrorType_Corrupted; // some data after the end tail zeros
						   }
						 */
					}
					return S_OK;
				}
				if(!IsRecordLast(buf))
					break;
				item.HeaderSize += NFileHeader::kRecordSize;
				thereAreEmptyRecords = true;
			}
			if(thereAreEmptyRecords) {
				// error = "There are data after end of archive";
				return S_OK;
			}
			error = k_ErrorType_Corrupted;
			ReadString(p, NFileHeader::kNameSize, item.Name); p += NFileHeader::kNameSize;
			item.NameCouldBeReduced = oneof2(item.Name.Len(), NFileHeader::kNameSize, (NFileHeader::kNameSize - 1));
			RIF(OctalToNumber32(p, 8, item.Mode)); p += 8;
			if(!OctalToNumber32(p, 8, item.UID)) item.UID = 0; p += 8;
			if(!OctalToNumber32(p, 8, item.GID)) item.GID = 0; p += 8;
			RIF(ParseSize(p, item.PackSize));
			item.Size = item.PackSize;
			p += 12;
			RIF(ParseInt64_MTime(p, item.MTime)); p += 12;
			uint32 checkSum;
			RIF(OctalToNumber32(p, 8, checkSum));
			memset(p, ' ', 8); p += 8;
			item.LinkFlag = *p++;
			ReadString(p, NFileHeader::kNameSize, item.LinkName); p += NFileHeader::kNameSize;
			item.LinkNameCouldBeReduced = oneof2(item.LinkName.Len(), NFileHeader::kNameSize, (NFileHeader::kNameSize - 1));
			memcpy(item.Magic, p, 8); p += 8;
			ReadString(p, NFileHeader::kUserNameSize, item.User); p += NFileHeader::kUserNameSize;
			ReadString(p, NFileHeader::kGroupNameSize, item.Group); p += NFileHeader::kGroupNameSize;
			item.DeviceMajorDefined = (p[0] != 0); if(item.DeviceMajorDefined) {
				RIF(OctalToNumber32(p, 8, item.DeviceMajor));
			}
			p += 8;
			item.DeviceMinorDefined = (p[0] != 0); if(item.DeviceMinorDefined) {
				RIF(OctalToNumber32(p, 8, item.DeviceMinor));
			}
			p += 8;
			if(p[0] != 0) {
				AString prefix;
				ReadString(p, NFileHeader::kPrefixSize, prefix);
				if(!prefix.IsEmpty() && item.IsUstarMagic() && (item.LinkFlag != 'L' /* || prefix != "00000000000" */ ))
					item.Name = prefix + '/' + item.Name;
			}
			p += NFileHeader::kPrefixSize;
			if(item.LinkFlag == NFileHeader::NLinkFlag::kHardLink) {
				item.PackSize = 0;
				item.Size = 0;
			}
			/*
			   TAR standard requires sum of unsigned byte values.
			   But some TAR programs use sum of signed byte values.
			   So we check both values.
			 */
			uint32 checkSumReal = 0;
			int32 checkSumReal_Signed = 0;
			for(uint i = 0; i < NFileHeader::kRecordSize; i++) {
				char c = buf[i];
				checkSumReal_Signed += (signed char)c;
				checkSumReal += (Byte)buf[i];
			}
			if(checkSumReal != checkSum) {
				if((uint32)checkSumReal_Signed != checkSum)
					return S_OK;
			}
			item.HeaderSize += NFileHeader::kRecordSize;
			if(item.LinkFlag == NFileHeader::NLinkFlag::kSparse) {
				Byte isExtended = buf[482];
				if(isExtended != 0 && isExtended != 1)
					return S_OK;
				RIF(ParseSize(buf + 483, item.Size));
				uint64 min = 0;
				for(uint i = 0; i < 4; i++) {
					p = buf + 386 + 24 * i;
					if(GetBe32(p) == 0) {
						if(isExtended != 0)
							return S_OK;
						break;
					}
					CSparseBlock sb;
					RIF(ParseSize(p, sb.Offset));
					RIF(ParseSize(p + 12, sb.Size));
					item.SparseBlocks.Add(sb);
					if(sb.Offset < min || sb.Offset > item.Size)
						return S_OK;
					if((sb.Offset & 0x1FF) != 0 || (sb.Size & 0x1FF) != 0)
						return S_OK;
					min = sb.Offset + sb.Size;
					if(min < sb.Offset)
						return S_OK;
				}
				if(min > item.Size)
					return S_OK;
				while(isExtended != 0) {
					size_t processedSize = NFileHeader::kRecordSize;
					RINOK(ReadStream(stream, buf, &processedSize));
					if(processedSize != NFileHeader::kRecordSize) {
						error = k_ErrorType_UnexpectedEnd;
						return S_OK;
					}
					item.HeaderSize += NFileHeader::kRecordSize;
					isExtended = buf[21 * 24];
					if(isExtended != 0 && isExtended != 1)
						return S_OK;
					for(uint i = 0; i < 21; i++) {
						p = buf + 24 * i;
						if(GetBe32(p) == 0) {
							if(isExtended != 0)
								return S_OK;
							break;
						}
						CSparseBlock sb;
						RIF(ParseSize(p, sb.Offset));
						RIF(ParseSize(p + 12, sb.Size));
						item.SparseBlocks.Add(sb);
						if(sb.Offset < min || sb.Offset > item.Size)
							return S_OK;
						if((sb.Offset & 0x1FF) != 0 || (sb.Size & 0x1FF) != 0)
							return S_OK;
						min = sb.Offset + sb.Size;
						if(min < sb.Offset)
							return S_OK;
					}
				}
				if(min > item.Size)
					return S_OK;
			}
			filled = true;
			error = k_ErrorType_OK;
			return S_OK;
		}

		static HRESULT ReadDataToString(ISequentialInStream * stream, CItemEx &item, AString &s, EErrorType &error)
		{
			const uint packSize = (uint)item.GetPackSizeAligned();
			size_t processedSize = packSize;
			HRESULT res = ReadStream(stream, s.GetBuf(packSize), &processedSize);
			item.HeaderSize += (uint)processedSize;
			s.ReleaseBuf_CalcLen((uint)item.PackSize);
			RINOK(res);
			if(processedSize != packSize)
				error = k_ErrorType_UnexpectedEnd;
			return S_OK;
		}

		static bool ParsePaxLongName(const AString &src, AString &dest)
		{
			dest.Empty();
			for(uint pos = 0;; ) {
				if(pos >= src.Len())
					return false;
				const char * start = src.Ptr(pos);
				const char * end;
				const uint32 lineLen = ConvertStringToUInt32(start, &end);
				if(end == start)
					return false;
				if(*end != ' ')
					return false;
				if(lineLen > src.Len() - pos)
					return false;
				unsigned offset = (uint)(end - start) + 1;
				if(lineLen < offset)
					return false;
				if(IsString1PrefixedByString2(src.Ptr(pos + offset), "path=")) {
					offset += 5; // "path="
					dest = src.Mid(pos + offset, lineLen - offset);
					if(dest.IsEmpty())
						return false;
					if(dest.Back() != '\n')
						return false;
					dest.DeleteBack();
					return true;
				}
				pos += lineLen;
			}
		}

		HRESULT ReadItem(ISequentialInStream * stream, bool &filled, CItemEx &item, EErrorType &error)
		{
			item.HeaderSize = 0;
			bool flagL = false;
			bool flagK = false;
			AString nameL;
			AString nameK;
			AString pax;
			for(;; ) {
				RINOK(GetNextItemReal(stream, filled, item, error));
				if(!filled) {
					if(error == k_ErrorType_OK && (flagL || flagK))
						error = k_ErrorType_Corrupted;
					return S_OK;
				}
				if(error != k_ErrorType_OK)
					return S_OK;
				// file contains a long name or file contains a long linkname
				if(oneof2(item.LinkFlag, NFileHeader::NLinkFlag::kGnu_LongName, NFileHeader::NLinkFlag::kGnu_LongLink)) { 
					AString * name;
					if(item.LinkFlag == NFileHeader::NLinkFlag::kGnu_LongName) {
						if(flagL) 
							return S_OK; 
						flagL = true; 
						name = &nameL;
					}
					else {
						if(flagK) 
							return S_OK; 
						flagK = true; 
						name = &nameK; 
					}
					if(item.Name != NFileHeader::kLongLink && item.Name != NFileHeader::kLongLink2)
						return S_OK;
					if(item.PackSize > (1 << 14))
						return S_OK;
					RINOK(ReadDataToString(stream, item, *name, error));
					if(error != k_ErrorType_OK)
						return S_OK;
					continue;
				}
				switch(item.LinkFlag) {
					case 'g':
					case 'x':
					case 'X':
						// pax Extended Header
						if(item.Name.IsPrefixedBy("PaxHeader/")) {
							RINOK(ReadDataToString(stream, item, pax, error));
							if(error != k_ErrorType_OK)
								return S_OK;
							continue;
						}
						break;
					case NFileHeader::NLinkFlag::kDumpDir:
						break;
						// GNU Extensions to the Archive Format
					case NFileHeader::NLinkFlag::kSparse:
						break;
						// GNU Extensions to the Archive Format
					default:
						if(item.LinkFlag > '7' || (item.LinkFlag < '0' && item.LinkFlag != 0))
							return S_OK;
				}
				if(flagL) {
					item.Name = nameL;
					item.NameCouldBeReduced = false;
				}
				if(flagK) {
					item.LinkName = nameK;
					item.LinkNameCouldBeReduced = false;
				}
				error = k_ErrorType_OK;
				if(!pax.IsEmpty()) {
					AString name;
					if(ParsePaxLongName(pax, name))
						item.Name = name;
					else
						error = k_ErrorType_Warning;
				}
				return S_OK;
			}
		}

		HRESULT COutArchive::WriteBytes(const void * data, uint size)
		{
			Pos += size;
			return WriteStream(m_Stream, data, size);
		}

		#define RETURN_IF_NOT_TRUE(x) { if(!(x)) return E_FAIL;	}
		HRESULT COutArchive::WriteHeaderReal(const CItem &item)
		{
			char record[NFileHeader::kRecordSize];
			memzero(record, NFileHeader::kRecordSize);
			char * cur = record;
			if(item.Name.Len() > NFileHeader::kNameSize)
				return E_FAIL;
			MyStrNCpy(cur, item.Name, NFileHeader::kNameSize);
			cur += NFileHeader::kNameSize;
			RETURN_IF_NOT_TRUE(WriteOctal_8(cur, item.Mode)); cur += 8;
			RETURN_IF_NOT_TRUE(WriteOctal_8(cur, item.UID)); cur += 8;
			RETURN_IF_NOT_TRUE(WriteOctal_8(cur, item.GID)); cur += 8;
			WriteOctal_12(cur, item.PackSize); cur += 12;
			WriteOctal_12_Signed(cur, item.MTime); cur += 12;
			memset(cur, ' ', 8);
			cur += 8;
			*cur++ = item.LinkFlag;
			RETURN_IF_NOT_TRUE(CopyString(cur, item.LinkName, NFileHeader::kNameSize));
			cur += NFileHeader::kNameSize;
			memcpy(cur, item.Magic, 8);
			cur += 8;
			RETURN_IF_NOT_TRUE(CopyString(cur, item.User, NFileHeader::kUserNameSize));
			cur += NFileHeader::kUserNameSize;
			RETURN_IF_NOT_TRUE(CopyString(cur, item.Group, NFileHeader::kGroupNameSize));
			cur += NFileHeader::kGroupNameSize;
			if(item.DeviceMajorDefined) RETURN_IF_NOT_TRUE(WriteOctal_8(cur, item.DeviceMajor)); cur += 8;
			if(item.DeviceMinorDefined) RETURN_IF_NOT_TRUE(WriteOctal_8(cur, item.DeviceMinor)); cur += 8;
			if(item.IsSparse()) {
				record[482] = (char)(item.SparseBlocks.Size() > 4 ? 1 : 0);
				WriteOctal_12(record + 483, item.Size);
				for(uint i = 0; i < item.SparseBlocks.Size() && i < 4; i++) {
					const CSparseBlock &sb = item.SparseBlocks[i];
					char * p = record + 386 + 24 * i;
					WriteOctal_12(p, sb.Offset);
					WriteOctal_12(p + 12, sb.Size);
				}
			}
			{
				uint32 checkSum = 0;
				{
					for(uint i = 0; i < NFileHeader::kRecordSize; i++)
						checkSum += (Byte)record[i];
				}
				/* we use GNU TAR scheme:
				   checksum field is formatted differently from the
				   other fields: it has [6] digits, a null, then a space. */
				// RETURN_IF_NOT_TRUE(WriteOctal_8(record + 148, checkSum));
				const uint kNumDigits = 6;
				for(uint i = 0; i < kNumDigits; i++) {
					record[148 + kNumDigits - 1 - i] = (char)('0' + (checkSum & 7));
					checkSum >>= 3;
				}
				record[148 + 6] = 0;
			}
			RINOK(WriteBytes(record, NFileHeader::kRecordSize));
			if(item.IsSparse()) {
				for(uint i = 4; i < item.SparseBlocks.Size(); ) {
					memzero(record, NFileHeader::kRecordSize);
					for(uint t = 0; t < 21 && i < item.SparseBlocks.Size(); t++, i++) {
						const CSparseBlock &sb = item.SparseBlocks[i];
						char * p = record + 24 * t;
						WriteOctal_12(p, sb.Offset);
						WriteOctal_12(p + 12, sb.Size);
					}
					record[21 * 24] = (char)(i < item.SparseBlocks.Size() ? 1 : 0);
					RINOK(WriteBytes(record, NFileHeader::kRecordSize));
				}
			}
			return S_OK;
		}
		void COutArchive::Create(ISequentialOutStream * outStream)
		{
			m_Stream = outStream;
		}
		HRESULT COutArchive::WriteHeader(const CItem &item)
		{
			unsigned nameSize = item.Name.Len();
			unsigned linkSize = item.LinkName.Len();
			/* There two versions of GNU tar:
			   OLDGNU_FORMAT: it writes short name and zero at the  end
			   GNU_FORMAT:    it writes only short name without zero at the end
			   we write it as OLDGNU_FORMAT with zero at the end */
			if(nameSize < NFileHeader::kNameSize && linkSize < NFileHeader::kNameSize)
				return WriteHeaderReal(item);
			CItem mi = item;
			mi.Name = NFileHeader::kLongLink;
			mi.LinkName.Empty();
			for(int i = 0; i < 2; i++) {
				const AString * name;
				// We suppose that GNU tar also writes item for long link before item for LongName?
				if(i == 0) {
					mi.LinkFlag = NFileHeader::NLinkFlag::kGnu_LongLink;
					name = &item.LinkName;
				}
				else {
					mi.LinkFlag = NFileHeader::NLinkFlag::kGnu_LongName;
					name = &item.Name;
				}
				if(name->Len() >= NFileHeader::kNameSize) {
					uint   nameStreamSize = name->Len() + 1;
					mi.PackSize = nameStreamSize;
					RINOK(WriteHeaderReal(mi));
					RINOK(WriteBytes((const char *)*name, nameStreamSize));
					RINOK(FillDataResidual(nameStreamSize));
				}
			}
			mi = item;
			if(mi.Name.Len() >= NFileHeader::kNameSize)
				mi.Name.SetFrom(item.Name, NFileHeader::kNameSize - 1);
			if(mi.LinkName.Len() >= NFileHeader::kNameSize)
				mi.LinkName.SetFrom(item.LinkName, NFileHeader::kNameSize - 1);
			return WriteHeaderReal(mi);
		}

		HRESULT COutArchive::FillDataResidual(uint64 dataSize)
		{
			uint   lastRecordSize = ((uint)dataSize & (NFileHeader::kRecordSize - 1));
			if(lastRecordSize == 0)
				return S_OK;
			else {
				uint   rem = NFileHeader::kRecordSize - lastRecordSize;
				Byte buf[NFileHeader::kRecordSize];
				memzero(buf, rem);
				return WriteBytes(buf, rem);
			}
		}

		HRESULT COutArchive::WriteFinishHeader()
		{
			Byte record[NFileHeader::kRecordSize];
			memzero(record, NFileHeader::kRecordSize);
			for(uint i = 0; i < 2; i++) {
				RINOK(WriteBytes(record, NFileHeader::kRecordSize));
			}
			return S_OK;
		}

		//HRESULT GetPropString(IArchiveUpdateCallback * callback, uint32 index, PROPID propId, AString &res, UINT codePage, bool convertSlash = false);
		HRESULT GetPropString(IArchiveUpdateCallback * callback, uint32 index, PROPID propId, AString &res, UINT codePage, bool convertSlash = false)
		{
			NCOM::CPropVariant prop;
			RINOK(callback->GetProperty(index, propId, &prop));
			if(prop.vt == VT_BSTR) {
				UString s = prop.bstrVal;
				if(convertSlash)
					NItemName::ReplaceSlashes_OsToUnix(s);

				if(codePage == CP_UTF8) {
					ConvertUnicodeToUTF8(s, res);
					// if(!ConvertUnicodeToUTF8(s, res)) // return E_INVALIDARG;
				}
				else
					UnicodeStringToMultiByte2(res, s, codePage);
			}
			else if(prop.vt != VT_EMPTY)
				return E_INVALIDARG;

			return S_OK;
		}

		HRESULT UpdateArchive(IInStream * inStream, ISequentialOutStream * outStream, const CObjectVector <NArchive::NTar::CItemEx> &inputItems,
			const CObjectVector<CUpdateItem> &updateItems, UINT codePage, IArchiveUpdateCallback * updateCallback)
		{
			COutArchive outArchive;
			outArchive.Create(outStream);
			outArchive.Pos = 0;
			CMyComPtr<IOutStream> outSeekStream;
			outStream->QueryInterface(IID_IOutStream, (void **)&outSeekStream);
			CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
			updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
			uint64 complexity = 0;
			uint i;
			for(i = 0; i < updateItems.Size(); i++) {
				const CUpdateItem &ui = updateItems[i];
				if(ui.NewData)
					complexity += ui.Size;
				else
					complexity += inputItems[ui.IndexInArc].GetFullSize();
			}
			RINOK(updateCallback->SetTotal(complexity));
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(updateCallback, true);
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
			streamSpec->SetStream(inStream);
			complexity = 0;
			for(i = 0; i < updateItems.Size(); i++) {
				lps->InSize = lps->OutSize = complexity;
				RINOK(lps->SetCur());
				const CUpdateItem &ui = updateItems[i];
				CItem item;
				if(ui.NewProps) {
					item.Mode = ui.Mode;
					item.Name = ui.Name;
					item.User = ui.User;
					item.Group = ui.Group;
					if(ui.IsDir) {
						item.LinkFlag = NFileHeader::NLinkFlag::kDirectory;
						item.PackSize = 0;
					}
					else {
						item.LinkFlag = NFileHeader::NLinkFlag::kNormal;
						item.PackSize = ui.Size;
					}
					item.MTime = ui.MTime;
					item.DeviceMajorDefined = false;
					item.DeviceMinorDefined = false;
					item.UID = 0;
					item.GID = 0;
					memcpy(item.Magic, NFileHeader::NMagic::kUsTar_00, 8);
				}
				else
					item = inputItems[ui.IndexInArc];

				AString symLink;
				if(ui.NewData || ui.NewProps) {
					RINOK(GetPropString(updateCallback, ui.IndexInClient, kpidSymLink, symLink, codePage, true));
					if(!symLink.IsEmpty()) {
						item.LinkFlag = NFileHeader::NLinkFlag::kSymLink;
						item.LinkName = symLink;
					}
				}
				if(ui.NewData) {
					item.SparseBlocks.Clear();
					item.PackSize = ui.Size;
					item.Size = ui.Size;
					if(ui.Size == static_cast<uint64>(-1LL))
						return E_INVALIDARG;
					CMyComPtr<ISequentialInStream> fileInStream;
					bool needWrite = true;
					if(!symLink.IsEmpty()) {
						item.PackSize = 0;
						item.Size = 0;
					}
					else {
						HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
						if(res == S_FALSE)
							needWrite = false;
						else {
							RINOK(res);
							if(fileInStream) {
								CMyComPtr<IStreamGetProps> getProps;
								fileInStream->QueryInterface(IID_IStreamGetProps, (void **)&getProps);
								if(getProps) {
									FILETIME mTime;
									uint64 size2;
									if(getProps->GetProps(&size2, NULL, NULL, &mTime, NULL) == S_OK) {
										item.PackSize = size2;
										item.Size = size2;
										item.MTime = NWindows::NTime::FileTimeToUnixTime64(mTime);;
									}
								}
							}
							else {
								item.PackSize = 0;
								item.Size = 0;
							}
							{
								AString hardLink;
								RINOK(GetPropString(updateCallback, ui.IndexInClient, kpidHardLink, hardLink, codePage, true));
								if(!hardLink.IsEmpty()) {
									item.LinkFlag = NFileHeader::NLinkFlag::kHardLink;
									item.LinkName = hardLink;
									item.PackSize = 0;
									item.Size = 0;
									fileInStream.Release();
								}
							}
						}
					}
					if(needWrite) {
						uint64 fileHeaderStartPos = outArchive.Pos;
						RINOK(outArchive.WriteHeader(item));
						if(fileInStream) {
							RINOK(copyCoder->Code(fileInStream, outStream, NULL, NULL, progress));
							outArchive.Pos += copyCoderSpec->TotalSize;
							if(copyCoderSpec->TotalSize != item.PackSize) {
								if(!outSeekStream)
									return E_FAIL;
								uint64 backOffset = outArchive.Pos - fileHeaderStartPos;
								RINOK(outSeekStream->Seek(-(int64)backOffset, STREAM_SEEK_CUR, NULL));
								outArchive.Pos = fileHeaderStartPos;
								item.PackSize = copyCoderSpec->TotalSize;
								RINOK(outArchive.WriteHeader(item));
								RINOK(outSeekStream->Seek(item.PackSize, STREAM_SEEK_CUR, NULL));
								outArchive.Pos += item.PackSize;
							}
							RINOK(outArchive.FillDataResidual(item.PackSize));
						}
					}
					complexity += item.PackSize;
					RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
				}
				else {
					const CItemEx &existItem = inputItems[ui.IndexInArc];
					uint64 size;
					if(ui.NewProps) {
						// memcpy(item.Magic, NFileHeader::NMagic::kEmpty, 8);
						if(!symLink.IsEmpty()) {
							item.PackSize = 0;
							item.Size = 0;
						}
						else {
							if(ui.IsDir == existItem.IsDir())
								item.LinkFlag = existItem.LinkFlag;
							item.SparseBlocks = existItem.SparseBlocks;
							item.Size = existItem.Size;
							item.PackSize = existItem.PackSize;
						}
						item.DeviceMajorDefined = existItem.DeviceMajorDefined;
						item.DeviceMinorDefined = existItem.DeviceMinorDefined;
						item.DeviceMajor = existItem.DeviceMajor;
						item.DeviceMinor = existItem.DeviceMinor;
						item.UID = existItem.UID;
						item.GID = existItem.GID;
						RINOK(outArchive.WriteHeader(item));
						RINOK(inStream->Seek(existItem.GetDataPosition(), STREAM_SEEK_SET, NULL));
						size = existItem.PackSize;
					}
					else {
						RINOK(inStream->Seek(existItem.HeaderPos, STREAM_SEEK_SET, NULL));
						size = existItem.GetFullSize();
					}
					streamSpec->Init(size);
					if(opCallback) {
						RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, (uint32)ui.IndexInArc, NUpdateNotifyOp::kReplicate))
					}
					RINOK(copyCoder->Code(inStreamLimited, outStream, NULL, NULL, progress));
					if(copyCoderSpec->TotalSize != size)
						return E_FAIL;
					outArchive.Pos += size;
					RINOK(outArchive.FillDataResidual(existItem.PackSize));
					complexity += size;
				}
			}
			lps->InSize = lps->OutSize = complexity;
			RINOK(lps->SetCur());
			return outArchive.WriteFinishHeader();
		}

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPhySize: if(_phySizeDefined) prop = _phySize; break;
				case kpidHeadersSize: if(_phySizeDefined) prop = _headersSize; break;
				case kpidErrorFlags:
				{
					uint32 flags = 0;
					if(!_isArc)
						flags |= kpv_ErrorFlags_IsNotArc;
					else 
						switch(_error) {
							case k_ErrorType_UnexpectedEnd: flags = kpv_ErrorFlags_UnexpectedEnd; break;
							case k_ErrorType_Corrupted: flags = kpv_ErrorFlags_HeadersError; break;
						}
					prop = flags;
					break;
				}
				case kpidWarningFlags:
					if(_warning)
						prop = kpv_ErrorFlags_HeadersError;
					break;
				case kpidCodePage:
				{
					char sz[16];
					const char * name = NULL;
					switch(_openCodePage) {
						case CP_OEMCP: name = "OEM"; break;
						case CP_UTF8: name = "UTF-8"; break;
					}
					if(!name) {
						ConvertUInt32ToString(_openCodePage, sz);
						name = sz;
					}
					prop = name;
					break;
				}
			}
			prop.Detach(value);
			return S_OK;
		}

		HRESULT CHandler::ReadItem2(ISequentialInStream * stream, bool &filled, CItemEx &item)
		{
			item.HeaderPos = _phySize;
			EErrorType error;
			HRESULT res = ReadItem(stream, filled, item, error);
			if(error == k_ErrorType_Warning)
				_warning = true;
			else if(error != k_ErrorType_OK)
				_error = error;
			RINOK(res);
			if(filled) {
				/*
				   if(item.IsSparse())
				   _isSparse = true;
				 */
				if(item.IsPaxExtendedHeader())
					_thereIsPaxExtendedHeader = true;
			}
			_phySize += item.HeaderSize;
			_headersSize += item.HeaderSize;
			return S_OK;
		}

		HRESULT CHandler::Open2(IInStream * stream, IArchiveOpenCallback * callback)
		{
			uint64 endPos = 0;
			{
				RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
				RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
			}
			_phySizeDefined = true;
			bool utf8_OK = true;
			if(!_forceCodePage) {
				if(!utf8_OK)
					_curCodePage = k_DefaultCodePage;
			}
			for(;; ) {
				CItemEx item;
				bool filled;
				RINOK(ReadItem2(stream, filled, item));
				if(!filled)
					break;
				_isArc = true;
				_items.Add(item);
				if(!_forceCodePage) {
					if(utf8_OK) utf8_OK = CheckUTF8(item.Name, item.NameCouldBeReduced);
					if(utf8_OK) utf8_OK = CheckUTF8(item.LinkName, item.LinkNameCouldBeReduced);
					if(utf8_OK) utf8_OK = CheckUTF8(item.User);
					if(utf8_OK) utf8_OK = CheckUTF8(item.Group);
				}
				RINOK(stream->Seek(item.GetPackSizeAligned(), STREAM_SEEK_CUR, &_phySize));
				if(_phySize > endPos) {
					_error = k_ErrorType_UnexpectedEnd;
					break;
				}
				/*
				   if(_phySize == endPos)
				   {
				   _errorMessage = "There are no trailing zero-filled records";
				   break;
				   }
				 */
				if(callback) {
					if(_items.Size() == 1) {
						RINOK(callback->SetTotal(NULL, &endPos));
					}
					if((_items.Size() & 0x3FF) == 0) {
						uint64 numFiles = _items.Size();
						RINOK(callback->SetCompleted(&numFiles, &_phySize));
					}
				}
			}

			if(!_forceCodePage) {
				if(!utf8_OK)
					_curCodePage = k_DefaultCodePage;
			}
			_openCodePage = _curCodePage;
			if(_items.Size() == 0) {
				if(_error != k_ErrorType_OK) {
					_isArc = false;
					return S_FALSE;
				}
				CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
				if(!callback)
					return S_FALSE;
				callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
				if(!openVolumeCallback)
					return S_FALSE;
				NCOM::CPropVariant prop;
				if(openVolumeCallback->GetProperty(kpidName, &prop) != S_OK)
					return S_FALSE;
				if(prop.vt != VT_BSTR)
					return S_FALSE;
				uint len = sstrlen(prop.bstrVal);
				if(len < 4 || MyStringCompareNoCase(prop.bstrVal + len - 4, L".tar") != 0)
					return S_FALSE;
			}

			_isArc = true;
			return S_OK;
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 *, IArchiveOpenCallback * openArchiveCallback)
		{
			COM_TRY_BEGIN
			{
				Close();
				RINOK(Open2(stream, openArchiveCallback));
				_stream = stream;
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::OpenSeq(ISequentialInStream * stream)
		{
			Close();
			_seqStream = stream;
			_isArc = true;
			return S_OK;
		}

		STDMETHODIMP CHandler::Close()
		{
			_isArc = false;
			_warning = false;
			_error = k_ErrorType_OK;

			_phySizeDefined = false;
			_phySize = 0;
			_headersSize = 0;
			_curIndex = 0;
			_latestIsRead = false;
			// _isSparse = false;
			_thereIsPaxExtendedHeader = false;
			_items.Clear();
			_seqStream.Release();
			_stream.Release();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = (_stream ? _items.Size() : (uint32)(int32)-1);
			return S_OK;
		}

		CHandler::CHandler()
		{
			copyCoderSpec = new NCompress::CCopyCoder();
			copyCoder = copyCoderSpec;
			_openCodePage = CP_UTF8;
			Init();
		}

		HRESULT CHandler::SkipTo(uint32 index)
		{
			while(_curIndex < index || !_latestIsRead) {
				if(_latestIsRead) {
					uint64 packSize = _latestItem.GetPackSizeAligned();
					RINOK(copyCoderSpec->Code(_seqStream, NULL, &packSize, &packSize, NULL));
					_phySize += copyCoderSpec->TotalSize;
					if(copyCoderSpec->TotalSize != packSize) {
						_error = k_ErrorType_UnexpectedEnd;
						return S_FALSE;
					}
					_latestIsRead = false;
					_curIndex++;
				}
				else {
					bool filled;
					RINOK(ReadItem2(_seqStream, filled, _latestItem));
					if(!filled) {
						_phySizeDefined = true;
						return E_INVALIDARG;
					}
					_latestIsRead = true;
				}
			}
			return S_OK;
		}
		void CHandler::TarStringToUnicode(const AString &s, NWindows::NCOM::CPropVariant &prop, bool toOs) const
		{
			UString dest;
			if(_curCodePage == CP_UTF8)
				ConvertUTF8ToUnicode(s, dest);
			else
				MultiByteToUnicodeString2(dest, s, _curCodePage);
			if(toOs)
				NItemName::ReplaceToOsSlashes_Remove_TailSlash(dest);
			prop = dest;
		}
		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CItemEx * item;
			if(_stream)
				item = &_items[index];
			else {
				if(index < _curIndex)
					return E_INVALIDARG;
				else {
					RINOK(SkipTo(index));
					item = &_latestItem;
				}
			}
			switch(propID) {
				case kpidPath: TarStringToUnicode(item->Name, prop, true); break;
				case kpidIsDir: prop = item->IsDir(); break;
				case kpidSize: prop = item->GetUnpackSize(); break;
				case kpidPackSize: prop = item->GetPackSizeAligned(); break;
				case kpidMTime:
					if(item->MTime != 0) {
						FILETIME ft;
						if(NTime::UnixTime64ToFileTime(item->MTime, ft))
							prop = ft;
					}
					break;
				case kpidPosixAttrib: prop = item->Get_Combined_Mode(); break;
				case kpidUser:  TarStringToUnicode(item->User, prop); break;
				case kpidGroup: TarStringToUnicode(item->Group, prop); break;
				case kpidSymLink:  if(item->LinkFlag == NFileHeader::NLinkFlag::kSymLink  && !item->LinkName.IsEmpty()) TarStringToUnicode(item->LinkName, prop); break;
				case kpidHardLink: if(item->LinkFlag == NFileHeader::NLinkFlag::kHardLink && !item->LinkName.IsEmpty()) TarStringToUnicode(item->LinkName, prop); break;
					// case kpidLinkType: prop = (int)item->LinkFlag; break;
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		HRESULT CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			ISequentialInStream * stream = _seqStream;
			bool seqMode = (_stream == NULL);
			if(!seqMode)
				stream = _stream;
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _items.Size();
			if(_stream && numItems == 0)
				return S_OK;
			uint64 totalSize = 0;
			uint32 i;
			for(i = 0; i < numItems; i++)
				totalSize += _items[allFilesMode ? i : indices[i]].GetUnpackSize();
			extractCallback->SetTotal(totalSize);
			uint64 totalPackSize;
			totalSize = totalPackSize = 0;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> inStream(streamSpec);
			streamSpec->SetStream(stream);
			CLimitedSequentialOutStream * outStreamSpec = new CLimitedSequentialOutStream;
			CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
			for(i = 0; i < numItems || seqMode; i++) {
				lps->InSize = totalPackSize;
				lps->OutSize = totalSize;
				RINOK(lps->SetCur());
				CMyComPtr<ISequentialOutStream> realOutStream;
				int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				int32 index = allFilesMode ? i : indices[i];
				const CItemEx * item;
				if(seqMode) {
					HRESULT res = SkipTo(index);
					if(res == E_INVALIDARG)
						break;
					RINOK(res);
					item = &_latestItem;
				}
				else
					item = &_items[index];

				RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
				uint64 unpackSize = item->GetUnpackSize();
				totalSize += unpackSize;
				totalPackSize += item->GetPackSizeAligned();
				if(item->IsDir()) {
					RINOK(extractCallback->PrepareOperation(askMode));
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					continue;
				}
				bool skipMode = false;
				if(!testMode && !realOutStream) {
					if(!seqMode) {
						/*
						   // probably we must show extracting info it callback handler instead
						   if(item->IsHardLink() ||
							item->IsSymLink())
						   {
						   RINOK(extractCallback->PrepareOperation(askMode));
						   RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
						   }
						 */
						continue;
					}
					skipMode = true;
					askMode = NExtractArc::NAskMode::kSkip;
				}
				RINOK(extractCallback->PrepareOperation(askMode));

				outStreamSpec->SetStream(realOutStream);
				realOutStream.Release();
				outStreamSpec->Init(skipMode ? 0 : unpackSize, true);

				int32 opRes = NExtractArc::NOperationResult::kOK;
				CMyComPtr<ISequentialInStream> inStream2;
				if(!item->IsSparse())
					inStream2 = inStream;
				else {
					GetStream(index, &inStream2);
					if(!inStream2)
						return E_FAIL;
				}
				{
					if(item->IsSymLink()) {
						RINOK(WriteStream(outStreamSpec, (const char *)item->LinkName, item->LinkName.Len()));
					}
					else {
						if(!seqMode) {
							RINOK(_stream->Seek(item->GetDataPosition(), STREAM_SEEK_SET, NULL));
						}
						streamSpec->Init(item->GetPackSizeAligned());
						RINOK(copyCoder->Code(inStream2, outStream, NULL, NULL, progress));
					}
					if(outStreamSpec->GetRem() != 0)
						opRes = NExtractArc::NOperationResult::kDataError;
				}
				if(seqMode) {
					_latestIsRead = false;
					_curIndex++;
				}
				outStreamSpec->ReleaseStream();
				RINOK(extractCallback->SetOperationResult(opRes));
			}
			return S_OK;
			COM_TRY_END
		}

		class CSparseStream : public IInStream, public CMyUnknownImp {
			uint64 _phyPos;
			uint64 _virtPos;
			bool _needStartSeek;
		public:
			CHandler * Handler;
			CMyComPtr<IUnknown> HandlerRef;
			unsigned ItemIndex;
			CRecordVector<uint64> PhyOffsets;

			MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
			void Init()
			{
				_virtPos = 0;
				_phyPos = 0;
				_needStartSeek = true;
			}
		};

		STDMETHODIMP CSparseStream::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			if(size == 0)
				return S_OK;
			const CItemEx &item = Handler->_items[ItemIndex];
			if(_virtPos >= item.Size)
				return S_OK;
			{
				uint64 rem = item.Size - _virtPos;
				if(size > rem)
					size = (uint32)rem;
			}
			HRESULT res = S_OK;
			if(item.SparseBlocks.IsEmpty())
				memzero(data, size);
			else {
				uint   left = 0, right = item.SparseBlocks.Size();
				for(;; ) {
					uint   mid = (left + right) / 2;
					if(mid == left)
						break;
					if(_virtPos < item.SparseBlocks[mid].Offset)
						right = mid;
					else
						left = mid;
				}
				const CSparseBlock &sb = item.SparseBlocks[left];
				uint64 relat = _virtPos - sb.Offset;
				if(_virtPos >= sb.Offset && relat < sb.Size) {
					uint64 rem = sb.Size - relat;
					SETMIN(size, (uint32)rem);
					uint64 phyPos = PhyOffsets[left] + relat;
					if(_needStartSeek || _phyPos != phyPos) {
						RINOK(Handler->_stream->Seek(item.GetDataPosition() + phyPos, STREAM_SEEK_SET, NULL));
						_needStartSeek = false;
						_phyPos = phyPos;
					}
					res = Handler->_stream->Read(data, size, &size);
					_phyPos += size;
				}
				else {
					uint64 next = item.Size;
					if(_virtPos < sb.Offset)
						next = sb.Offset;
					else if(left + 1 < item.SparseBlocks.Size())
						next = item.SparseBlocks[left + 1].Offset;
					uint64 rem = next - _virtPos;
					SETMIN(size, (uint32)rem);
					memzero(data, size);
				}
			}
			_virtPos += size;
			ASSIGN_PTR(processedSize, size);
			return res;
		}

		STDMETHODIMP CSparseStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
		{
			switch(seekOrigin) {
				case STREAM_SEEK_SET: break;
				case STREAM_SEEK_CUR: offset += _virtPos; break;
				case STREAM_SEEK_END: offset += Handler->_items[ItemIndex].Size; break;
				default: return STG_E_INVALIDFUNCTION;
			}
			if(offset < 0)
				return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
			_virtPos = offset;
			ASSIGN_PTR(newPosition, _virtPos);
			return S_OK;
		}

		STDMETHODIMP CHandler::GetStream(uint32 index, ISequentialInStream ** stream)
		{
			COM_TRY_BEGIN
			const CItemEx &item = _items[index];
			if(item.IsSparse()) {
				CSparseStream * streamSpec = new CSparseStream;
				CMyComPtr<IInStream> streamTemp = streamSpec;
				streamSpec->Init();
				streamSpec->Handler = this;
				streamSpec->HandlerRef = (IInArchive*)this;
				streamSpec->ItemIndex = index;
				streamSpec->PhyOffsets.Reserve(item.SparseBlocks.Size());
				uint64 offs = 0;
				FOR_VECTOR(i, item.SparseBlocks)
				{
					const CSparseBlock &sb = item.SparseBlocks[i];
					streamSpec->PhyOffsets.AddInReserved(offs);
					offs += sb.Size;
				}
				*stream = streamTemp.Detach();
				return S_OK;
			}
			else if(item.IsSymLink()) {
				Create_BufInStream_WithReference((const Byte*)(const char *)item.LinkName, item.LinkName.Len(), (IInArchive*)this, stream);
				return S_OK;
			}
			else
				return CreateLimitedInStream(_stream, item.GetDataPosition(), item.PackSize, stream);
			COM_TRY_END
		}

		void CHandler::Init()
		{
			_forceCodePage = false;
			// _codePage = CP_OEMCP;
			_curCodePage = _specifiedCodePage = CP_UTF8; // CP_OEMCP;
			_thereIsPaxExtendedHeader = false;
		}

		STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
		{
			Init();
			for(uint32 i = 0; i < numProps; i++) {
				UString name = names[i];
				name.MakeLower_Ascii();
				if(name.IsEmpty())
					return E_INVALIDARG;
				const PROPVARIANT &prop = values[i];
				if(name[0] == L'x') {
					// some clients write 'x' property. So we support it
					uint32 level = 0;
					RINOK(ParsePropToUInt32(name.Ptr(1), prop, level));
				}
				else if(name.IsEqualTo("cp")) {
					uint32 cp = CP_OEMCP;
					RINOK(ParsePropToUInt32(L"", prop, cp));
					_forceCodePage = true;
					_curCodePage = _specifiedCodePage = cp;
				}
				else
					return E_INVALIDARG;
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::GetFileTimeType(uint32 * type)
		{
			*type = NFileTimeType::kUnix;
			return S_OK;
		}
		// sort old files with original order.
		static int CompareUpdateItems(void * const * p1, void * const * p2, void *)
		{
			const CUpdateItem & u1 = *(*((const CUpdateItem**)p1));
			const CUpdateItem & u2 = *(*((const CUpdateItem**)p2));
			if(!u1.NewProps)
				return u2.NewProps ? -1 : MyCompare(u1.IndexInArc, u2.IndexInArc);
			else if(!u2.NewProps)
				return 1;
			else
				return MyCompare(u1.IndexInClient, u2.IndexInClient);
		}
		STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream * outStream, uint32 numItems, IArchiveUpdateCallback * callback)
		{
			COM_TRY_BEGIN
			if((_stream && (_error != k_ErrorType_OK || _warning /* || _isSparse */)) || _seqStream)
				return E_NOTIMPL;
			CObjectVector<CUpdateItem> updateItems;
			UINT codePage = (_forceCodePage ? _specifiedCodePage : _openCodePage);
			for(uint32 i = 0; i < numItems; i++) {
				CUpdateItem ui;
				int32 newData;
				int32 newProps;
				uint32 indexInArc;
				if(!callback)
					return E_FAIL;
				RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArc));
				ui.NewProps = IntToBool(newProps);
				ui.NewData = IntToBool(newData);
				ui.IndexInArc = indexInArc;
				ui.IndexInClient = i;
				if(IntToBool(newProps)) {
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
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidPosixAttrib, &prop));
						if(prop.vt == VT_EMPTY)
							ui.Mode = MY_LIN_S_IRWXO|MY_LIN_S_IRWXG|MY_LIN_S_IRWXU|(ui.IsDir ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG);
						else if(prop.vt != VT_UI4)
							return E_INVALIDARG;
						else
							ui.Mode = prop.ulVal;
						// FIXME : we can clear high file type bits to be more compatible with tars created by
						// GNU TAR.
						// ui.Mode &= ~(uint32)MY_LIN_S_IFMT;
					}
					{
						NCOM::CPropVariant prop;
						RINOK(callback->GetProperty(i, kpidMTime, &prop));
						if(prop.vt == VT_EMPTY)
							ui.MTime = 0;
						else if(prop.vt != VT_FILETIME)
							return E_INVALIDARG;
						else
							ui.MTime = NTime::FileTimeToUnixTime64(prop.filetime);
					}

					RINOK(GetPropString(callback, i, kpidPath, ui.Name, codePage, true));
					if(ui.IsDir && !ui.Name.IsEmpty() && ui.Name.Back() != '/')
						ui.Name += '/';
					RINOK(GetPropString(callback, i, kpidUser, ui.User, codePage));
					RINOK(GetPropString(callback, i, kpidGroup, ui.Group, codePage));
				}
				if(IntToBool(newData)) {
					NCOM::CPropVariant prop;
					RINOK(callback->GetProperty(i, kpidSize, &prop));
					if(prop.vt != VT_UI8)
						return E_INVALIDARG;
					ui.Size = prop.uhVal.QuadPart;
					/*
					   // now we support GNU extension for big files
					   if(ui.Size >= ((uint64)1 << 33))
					   return E_INVALIDARG;
					 */
				}
				updateItems.Add(ui);
			}
			if(_thereIsPaxExtendedHeader) {
				// we restore original order of files, if there is pax header block
				updateItems.Sort(CompareUpdateItems, NULL);
			}
			return UpdateArchive(_stream, outStream, _items, updateItems, codePage, callback);
			COM_TRY_END
		}
	}
}
