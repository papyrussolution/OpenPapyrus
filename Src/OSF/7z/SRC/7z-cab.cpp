// 7Z-CAB.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
//
// CabHandler.cpp CabIn.cpp
using namespace NWindows;

// QuantumDecoder.cpp
namespace NCompress {
	namespace NQuantum {
		class CBitDecoder {
			uint32 Value;
			bool _extra;
			const Byte * _buf;
			const Byte * _bufLim;
		public:
			void SetStreamAndInit(const Byte * inData, size_t inSize);
			bool WasExtraRead() const;
			bool WasFinishedOK() const;
			uint32 ReadBit();
			uint32 ReadStart16Bits();
			uint32 FASTCALL ReadBits(uint numBits); // numBits > 0
		};

		class CRangeDecoder {
			uint32 Low;
			uint32 Range;
			uint32 Code;
		public:
			CBitDecoder Stream;
			void Init();
			bool Finish();
			uint32 FASTCALL GetThreshold(uint32 total) const;
			void Decode(uint32 start, uint32 end, uint32 total);
		};

		const uint kNumLitSelectorBits = 2;
		const uint kNumLitSelectors = (1 << kNumLitSelectorBits);
		const uint kNumLitSymbols = 1 << (8 - kNumLitSelectorBits);
		const uint kNumMatchSelectors = 3;
		const uint kNumSelectors = kNumLitSelectors + kNumMatchSelectors;
		const uint kNumSymbolsMax = kNumLitSymbols; // 64

		class CModelDecoder {
			uint   NumItems;
			uint   ReorderCount;
			uint16 Freqs[kNumSymbolsMax + 1];
			Byte   Vals[kNumSymbolsMax];
		public:
			void   Init(uint numItems);
			uint   Decode(CRangeDecoder * rc);
		};

		class CDecoder : public IUnknown, public CMyUnknownImp {
			CLzOutWindow _outWindow;
			uint   _numDictBits;
			CModelDecoder m_Selector;
			CModelDecoder m_Literals[kNumLitSelectors];
			CModelDecoder m_PosSlot[kNumMatchSelectors];
			CModelDecoder m_LenSlot;

			void Init();
			HRESULT CodeSpec(const Byte * inData, size_t inSize, uint32 outSize);
		public:
			MY_UNKNOWN_IMP
			HRESULT Code(const Byte * inData, size_t inSize, ISequentialOutStream * outStream, uint32 outSize, bool keepHistory);
			HRESULT SetParams(unsigned numDictBits);
			CDecoder();
			virtual ~CDecoder();
		};

		static const uint kNumLenSymbols = 27;
		static const uint kMatchMinLen = 3;
		static const uint kNumSimplePosSlots = 4;
		static const uint kNumSimpleLenSlots = 6;

		static const uint16 kUpdateStep = 8;
		static const uint16 kFreqSumMax = 3800;
		static const uint kReorderCountStart = 4;
		static const uint kReorderCount = 50;

		void CBitDecoder::SetStreamAndInit(const Byte * inData, size_t inSize)
		{
			_buf = inData;
			_bufLim = inData + inSize;
			Value = 0x10000;
			_extra = false;
		}
		bool CBitDecoder::WasExtraRead() const { return _extra; }
		bool CBitDecoder::WasFinishedOK() const { return !_extra && _buf == _bufLim; }

		uint32 CBitDecoder::ReadBit()
		{
			if(Value >= 0x10000) {
				Byte b;
				if(_buf >= _bufLim) {
					b = 0xFF;
					_extra = true;
				}
				else
					b = *_buf++;
				Value = 0x100 | b;
			}
			uint32 res = (Value >> 7) & 1;
			Value <<= 1;
			return res;
		}

		uint32 CBitDecoder::ReadStart16Bits()
		{
			// we use check for extra read in another code.
			uint32 val = ((uint32)*_buf << 8) | _buf[1];
			_buf += 2;
			return val;
		}

		uint32 FASTCALL CBitDecoder::ReadBits(uint numBits) // numBits > 0
		{
			uint32 res = 0;
			do {
				res = (res << 1) | ReadBit();
			} while(--numBits);
			return res;
		}

		void CRangeDecoder::Init()
		{
			Low = 0;
			Range = 0x10000;
			Code = Stream.ReadStart16Bits();
		}

		bool CRangeDecoder::Finish()
		{
			// do all streams use these two bits at end?
			if(Stream.ReadBit() != 0) return false;
			if(Stream.ReadBit() != 0) return false;
			return Stream.WasFinishedOK();
		}

		uint32 FASTCALL CRangeDecoder::GetThreshold(uint32 total) const
		{
			return ((Code + 1) * total - 1) / Range; // & 0xFFFF is not required;
		}

		void CRangeDecoder::Decode(uint32 start, uint32 end, uint32 total)
		{
			uint32 high = Low + end * Range / total - 1;
			uint32 offset = start * Range / total;
			Code -= offset;
			Low += offset;
			for(;;) {
				if((Low & 0x8000) != (high & 0x8000)) {
					if((Low & 0x4000) == 0 || (high & 0x4000) != 0)
						break;
					Low &= 0x3FFF;
					high |= 0x4000;
				}
				Low = (Low << 1) & 0xFFFF;
				high = ((high << 1) | 1) & 0xFFFF;
				Code = ((Code << 1) | Stream.ReadBit());
			}
			Range = high - Low + 1;
		}

		void CModelDecoder::Init(uint numItems)
		{
			NumItems = numItems;
			ReorderCount = kReorderCountStart;
			for(uint i = 0; i < numItems; i++) {
				Freqs[i] = static_cast<uint16>(numItems - i);
				Vals[i] = (Byte)i;
			}
			Freqs[numItems] = 0;
		}
		unsigned CModelDecoder::Decode(CRangeDecoder * rc)
		{
			uint32 threshold = rc->GetThreshold(Freqs[0]);
			uint   i;
			for(i = 1; Freqs[i] > threshold; i++) 
				;
			rc->Decode(Freqs[i], Freqs[(size_t)i - 1], Freqs[0]);
			uint   res = Vals[--i];
			do {
				Freqs[i] += kUpdateStep;
			} while(i--);
			if(Freqs[0] > kFreqSumMax) {
				if(--ReorderCount == 0) {
					ReorderCount = kReorderCount;
					for(i = 0; i < NumItems; i++)
						Freqs[i] = static_cast<uint16>(((Freqs[i] - Freqs[(size_t)i + 1]) + 1) >> 1);
					for(i = 0; i < NumItems - 1; i++)
						for(uint j = i + 1; j < NumItems; j++)
							if(Freqs[i] < Freqs[j]) {
								uint16 tmpFreq = Freqs[i];
								Byte tmpVal = Vals[i];
								Freqs[i] = Freqs[j];
								Vals[i] = Vals[j];
								Freqs[j] = tmpFreq;
								Vals[j] = tmpVal;
							}

					do
						Freqs[i] = static_cast<uint16>(Freqs[i] + Freqs[(size_t)i + 1]);
					while(i--);
				}
				else {
					i = NumItems - 1;
					do {
						Freqs[i] >>= 1;
						if(Freqs[i] <= Freqs[(size_t)i + 1])
							Freqs[i] = static_cast<uint16>(Freqs[(size_t)i + 1] + 1);
					}
					while(i--);
				}
			}

			return res;
		}

		CDecoder::CDecoder() : _numDictBits(0) 
		{
		}

		CDecoder::~CDecoder() 
		{
		}

		void CDecoder::Init()
		{
			m_Selector.Init(kNumSelectors);
			uint i;
			for(i = 0; i < kNumLitSelectors; i++)
				m_Literals[i].Init(kNumLitSymbols);
			uint   numItems = (_numDictBits == 0 ? 1 : (_numDictBits << 1));
			const  uint kNumPosSymbolsMax[kNumMatchSelectors] = { 24, 36, 42 };
			for(i = 0; i < kNumMatchSelectors; i++)
				m_PosSlot[i].Init(smin(numItems, kNumPosSymbolsMax[i]));
			m_LenSlot.Init(kNumLenSymbols);
		}

		HRESULT CDecoder::CodeSpec(const Byte * inData, size_t inSize, uint32 outSize)
		{
			if(inSize < 2)
				return S_FALSE;
			CRangeDecoder rc;
			rc.Stream.SetStreamAndInit(inData, inSize);
			rc.Init();
			while(outSize != 0) {
				if(rc.Stream.WasExtraRead())
					return S_FALSE;
				uint   selector = m_Selector.Decode(&rc);
				if(selector < kNumLitSelectors) {
					Byte b = (Byte)((selector << (8 - kNumLitSelectorBits)) + m_Literals[selector].Decode(&rc));
					_outWindow.PutByte(b);
					outSize--;
				}
				else {
					selector -= kNumLitSelectors;
					uint len = selector + kMatchMinLen;
					if(selector == 2) {
						uint   lenSlot = m_LenSlot.Decode(&rc);
						if(lenSlot >= kNumSimpleLenSlots) {
							lenSlot -= 2;
							unsigned numDirectBits = (uint)(lenSlot >> 2);
							len += ((4 | (lenSlot & 3)) << numDirectBits) - 2;
							if(numDirectBits < 6)
								len += rc.Stream.ReadBits(numDirectBits);
						}
						else
							len += lenSlot;
					}
					uint32 dist = m_PosSlot[selector].Decode(&rc);
					if(dist >= kNumSimplePosSlots) {
						uint   numDirectBits = (uint)((dist >> 1) - 1);
						dist = ((2 | (dist & 1)) << numDirectBits) + rc.Stream.ReadBits(numDirectBits);
					}
					uint   locLen = len;
					if(len > outSize)
						locLen = (uint)outSize;
					if(!_outWindow.CopyBlock(dist, locLen))
						return S_FALSE;
					outSize -= locLen;
					len -= locLen;
					if(len != 0)
						return S_FALSE;
				}
			}
			return rc.Finish() ? S_OK : S_FALSE;
		}

		HRESULT CDecoder::Code(const Byte * inData, size_t inSize, ISequentialOutStream * outStream, uint32 outSize, bool keepHistory)
		{
			try {
				_outWindow.SetStream(outStream);
				_outWindow.Init(keepHistory);
				if(!keepHistory)
					Init();
				HRESULT res = CodeSpec(inData, inSize, outSize);
				HRESULT res2 = _outWindow.Flush();
				return res != S_OK ? res : res2;
			}
			catch(const CLzOutWindowException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}

		HRESULT CDecoder::SetParams(unsigned numDictBits)
		{
			if(numDictBits > 21)
				return E_INVALIDARG;
			else {
				_numDictBits = numDictBits;
				return _outWindow.Create((uint32)1 << _numDictBits) ? S_OK : E_OUTOFMEMORY;
			}
		}
	}
}
//
namespace NArchive {
	namespace NCab {
		static const uint32 kBlockSize = (1 << 16);

		class CCabBlockInStream : public ISequentialInStream, public CMyUnknownImp {
		public:
			uint32 ReservedSize; // < 256
			bool MsZip;
			MY_UNKNOWN_IMP CCabBlockInStream() : _buf(0), ReservedSize(0), MsZip(false) 
			{
			}
			~CCabBlockInStream();
			bool Create();
			void InitForNewBlock();
			HRESULT PreRead(ISequentialInStream * stream, uint32 &packSize, uint32 &unpackSize);
			uint32 GetPackSizeAvail() const { return _size - _pos; }
			const Byte * GetData() const { return _buf + _pos; }
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		private:
			Byte * _buf;
			uint32 _size;
			uint32 _pos;
		};

		bool CCabBlockInStream::Create() { return SETIFZ(_buf, (Byte *)SAlloc::M(kBlockSize)); }
		
		CCabBlockInStream::~CCabBlockInStream()
		{
			SAlloc::F(_buf);
		}
		void CCabBlockInStream::InitForNewBlock() 
		{
			_size = 0; 
			_pos = 0;
		}
		static uint32 CheckSum(const Byte * p, uint32 size)
		{
			uint32 sum = 0;
			for(; size >= 8; size -= 8) {
				sum ^= GetUi32(p) ^ GetUi32(p + 4);
				p += 8;
			}
			if(size >= 4) {
				sum ^= GetUi32(p);
				p += 4;
			}
			size &= 3;
			if(size > 2) sum ^= (uint32)(*p++) << 16;
			if(size > 1) sum ^= (uint32)(*p++) << 8;
			if(size > 0) sum ^= (uint32)(*p++);
			return sum;
		}

		HRESULT CCabBlockInStream::PreRead(ISequentialInStream * stream, uint32 &packSize, uint32 &unpackSize)
		{
			const uint32 kHeaderSize = 8;
			const uint32 kReservedMax = 256;
			Byte header[kHeaderSize + kReservedMax];
			RINOK(ReadStream_FALSE(stream, header, kHeaderSize + ReservedSize))
			packSize = GetUi16(header + 4);
			unpackSize = GetUi16(header + 6);
			if(packSize > kBlockSize - _size)
				return S_FALSE;
			RINOK(ReadStream_FALSE(stream, _buf + _size, packSize));
			if(MsZip) {
				if(_size == 0) {
					if(packSize < 2 || _buf[0] != 0x43 || _buf[1] != 0x4B)
						return S_FALSE;
					_pos = 2;
				}
				if(_size + packSize > ((uint32)1 << 15) + 12) /* v9.31 fix. MSZIP specification */
					return S_FALSE;
			}
			if(GetUi32(header) != 0) // checkSum
				if(CheckSum(header, kHeaderSize + ReservedSize) != CheckSum(_buf + _size, packSize))
					return S_FALSE;
			_size += packSize;
			return S_OK;
		}

		STDMETHODIMP CCabBlockInStream::Read(void * data, uint32 size, uint32 * processedSize)
		{
			if(size != 0) {
				uint32 rem = _size - _pos;
				SETMIN(size, rem);
				memcpy(data, _buf + _pos, size);
				_pos += size;
			}
			ASSIGN_PTR(processedSize, size);
			return S_OK;
		}
		//
		// #define _CAB_DETAILS

		#ifdef _CAB_DETAILS
		enum {
			kpidBlockReal = kpidUserDefined
		};
		#endif

		static const Byte kProps[] = {
			kpidPath,
			kpidSize,
			kpidMTime,
			kpidAttrib,
			kpidMethod,
			kpidBlock
		  #ifdef _CAB_DETAILS
			,
			// kpidBlockReal, // L"BlockReal",
			kpidOffset,
			kpidVolume
		  #endif
		};

		static const Byte kArcProps[] = { kpidTotalPhySize, kpidMethod, /*kpidSolid,*/ kpidNumBlocks, kpidNumVolumes, kpidVolumeIndex, kpidId };

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		static const char * const kMethods[] = { "None", "MSZip", "Quantum", "LZX" };
		static const uint kMethodNameBufSize = 32; // "Quantum:255"

		static void SetMethodName(char * s, unsigned method, unsigned param)
		{
			if(method < SIZEOFARRAY(kMethods)) {
				s = MyStpCpy(s, kMethods[method]);
				if(method != NHeader::NMethod::kLZX && method != NHeader::NMethod::kQuantum)
					return;
				*s++ = ':';
				method = param;
			}
			ConvertUInt32ToString(method, s);
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidMethod:
				{
					uint32 mask = 0;
					uint32 params[2] = { 0, 0 };
					{
						FOR_VECTOR(v, m_Database.Volumes) {
							const CRecordVector<CFolder> &folders = m_Database.Volumes[v].Folders;
							FOR_VECTOR(i, folders) {
								const CFolder &folder = folders[i];
								unsigned method = folder.GetMethod();
								mask |= ((uint32)1 << method);
								if(oneof2(method, NHeader::NMethod::kLZX, NHeader::NMethod::kQuantum)) {
									unsigned di = (method == NHeader::NMethod::kQuantum) ? 0 : 1;
									if(params[di] < folder.MethodMinor)
										params[di] = folder.MethodMinor;
								}
							}
						}
					}
					AString s;
					for(uint i = 0; i < kNumMethodsMax; i++) {
						if((mask & (1 << i)) == 0)
							continue;
						s.Add_Space_if_NotEmpty();
						char temp[kMethodNameBufSize];
						SetMethodName(temp, i, params[i == NHeader::NMethod::kQuantum ? 0 : 1]);
						s += temp;
					}
					prop = s;
					break;
				}
				// case kpidSolid: prop = _database.IsSolid(); break;
				case kpidNumBlocks:
				{
					uint32 numFolders = 0;
					FOR_VECTOR(v, m_Database.Volumes)
					numFolders += m_Database.Volumes[v].Folders.Size();
					prop = numFolders;
					break;
				}
				case kpidTotalPhySize:
					if(m_Database.Volumes.Size() > 1) {
						uint64 sum = 0;
						FOR_VECTOR(v, m_Database.Volumes)
						sum += m_Database.Volumes[v].ArcInfo.Size;
						prop = sum;
					}
					break;
				case kpidNumVolumes:
					prop = (uint32)m_Database.Volumes.Size();
					break;
				case kpidVolumeIndex:
					if(!m_Database.Volumes.IsEmpty()) {
						const CDatabaseEx &db = m_Database.Volumes[0];
						const CDatabase::CInArcInfo &ai = db.ArcInfo;
						prop = (uint32)ai.CabinetNumber;
					}
					break;
				case kpidId:
					if(m_Database.Volumes.Size() != 0) {
						prop = (uint32)m_Database.Volumes[0].ArcInfo.SetID;
					}
					break;
				case kpidOffset:
					/*
					   if(m_Database.Volumes.Size() == 1)
					   prop = m_Database.Volumes[0].StartPosition;
					 */
					prop = _offset;
					break;
				case kpidPhySize:
					/*
					   if(m_Database.Volumes.Size() == 1)
					   prop = (uint64)m_Database.Volumes[0].ArcInfo.Size;
					 */
					prop = (uint64)_phySize;
					break;
				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!_isArc) v |= kpv_ErrorFlags_IsNotArc;
					if(_errorInHeaders) v |= kpv_ErrorFlags_HeadersError;
					if(_unexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
					prop = v;
					break;
				}
				case kpidError:
					if(!_errorMessage.IsEmpty())
						prop = _errorMessage;
					break;
				case kpidName:
				{
					if(m_Database.Volumes.Size() == 1) {
						const CDatabaseEx &db = m_Database.Volumes[0];
						const CDatabase::CInArcInfo &ai = db.ArcInfo;
						if(ai.SetID != 0) {
							AString s;
							s.Add_UInt32(ai.SetID);
							s += '_';
							s.Add_UInt32(ai.CabinetNumber + 1);
							s += ".cab";
							prop = s;
						}
						/*
						   // that code is incomplete. It gcan give accurate name of volume
						   char s[32];
						   ConvertUInt32ToString(ai.CabinetNumber + 2, s);
						   uint len = sstrlen(s);
						   if(ai.IsThereNext())
						   {
						   AString fn = ai.NextArc.FileName;
						   if(fn.Len() > 4 && sstreqi_ascii(fn.RightPtr(4), ".cab"))
							fn.DeleteFrom(fn.Len() - 4);
						   if(len < fn.Len())
						   {
							if(strcmp(s, fn.RightPtr(len)) == 0)
							{
							  AString s2 = fn;
							  s2.DeleteFrom(fn.Len() - len);
							  ConvertUInt32ToString(ai.CabinetNumber + 1, s);
							  s2 += s;
							  s2 += ".cab";
							  prop = GetUnicodeString(s2);
							}
						   }
						   }
						 */
					}
					break;
				}
					// case kpidShortComment:
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CMvDatabaseEx::CMvItem & mvItem = m_Database.Items[index];
			const CDatabaseEx &db = m_Database.Volumes[mvItem.VolumeIndex];
			unsigned itemIndex = mvItem.ItemIndex;
			const CItem &item = db.Items[itemIndex];
			switch(propID) {
				case kpidPath:
				{
					UString unicodeName;
					if(item.IsNameUTF())
						ConvertUTF8ToUnicode(item.Name, unicodeName);
					else
						unicodeName = MultiByteToUnicodeString(item.Name, CP_ACP);
					prop = (const wchar_t *)NItemName::WinPathToOsPath(unicodeName);
					break;
				}
				case kpidIsDir:  prop = item.IsDir(); break;
				case kpidSize:  prop = item.Size; break;
				case kpidAttrib:  prop = item.GetWinAttrib(); break;
				case kpidMTime:
				{
					FILETIME localFileTime, utcFileTime;
					if(NTime::DosTimeToFileTime(item.Time, localFileTime)) {
						if(!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
							utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
					}
					else
						utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
					prop = utcFileTime;
					break;
				}
				case kpidMethod:
				{
					uint32 realFolderIndex = item.GetFolderIndex(db.Folders.Size());
					const CFolder &folder = db.Folders[realFolderIndex];
					char s[kMethodNameBufSize];;
					SetMethodName(s, folder.GetMethod(), folder.MethodMinor);
					prop = s;
					break;
				}
				case kpidBlock:  prop = (int32)m_Database.GetFolderIndex(&mvItem); break;
			#ifdef _CAB_DETAILS
				// case kpidBlockReal:  prop = (uint32)item.FolderIndex; break;
				case kpidOffset:  prop = (uint32)item.Offset; break;
				case kpidVolume:  prop = (uint32)mvItem.VolumeIndex; break;
			#endif
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Open(IInStream * inStream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * callback)
		{
			COM_TRY_BEGIN
			Close();
			CInArchive archive;
			CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
			callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
			CMyComPtr<IInStream> nextStream = inStream;
			bool prevChecked = false;
			UString startVolName;
			bool startVolName_was_Requested = false;
			uint64 numItems = 0;
			uint   numTempVolumes = 0;
			// try
			{
				while(nextStream) {
					CDatabaseEx db;
					db.Stream = nextStream;
					HRESULT res = archive.Open(db, maxCheckStartPosition);
					_errorInHeaders |= archive.HeaderError;
					_errorInHeaders |= archive.ErrorInNames;
					_unexpectedEnd |= archive.UnexpectedEnd;
					if(res == S_OK && !m_Database.Volumes.IsEmpty()) {
						const  CArchInfo &lastArc = m_Database.Volumes.Back().ArcInfo;
						uint   cabNumber = db.ArcInfo.CabinetNumber;
						if(lastArc.SetID != db.ArcInfo.SetID)
							res = S_FALSE;
						else if(prevChecked) {
							if(cabNumber != lastArc.CabinetNumber + 1)
								res = S_FALSE;
						}
						else if(cabNumber >= lastArc.CabinetNumber)
							res = S_FALSE;
						else if(numTempVolumes != 0) {
							const CArchInfo &prevArc = m_Database.Volumes[numTempVolumes - 1].ArcInfo;
							if(cabNumber != prevArc.CabinetNumber + 1)
								res = S_FALSE;
						}
					}

					if(archive.IsArc || res == S_OK) {
						_isArc = true;
						if(m_Database.Volumes.IsEmpty()) {
							_offset = db.StartPosition;
							_phySize = db.ArcInfo.Size;
						}
					}
					if(res == S_OK) {
						numItems += db.Items.Size();
						m_Database.Volumes.Insert(prevChecked ? m_Database.Volumes.Size() : numTempVolumes, db);
						if(!prevChecked && m_Database.Volumes.Size() > 1) {
							numTempVolumes++;
							if(db.ArcInfo.CabinetNumber + 1 == m_Database.Volumes[numTempVolumes].ArcInfo.CabinetNumber)
								numTempVolumes = 0;
						}
					}
					else {
						if(res != S_FALSE)
							return res;
						if(m_Database.Volumes.IsEmpty())
							return S_FALSE;
						if(prevChecked)
							break;
						prevChecked = true;
						if(numTempVolumes != 0) {
							m_Database.Volumes.DeleteFrontal(numTempVolumes);
							numTempVolumes = 0;
						}
					}
					RINOK(callback->SetCompleted(&numItems, NULL));
					nextStream = NULL;
					for(;;) {
						const COtherArc * otherArc = NULL;
						if(!prevChecked) {
							if(numTempVolumes == 0) {
								const CDatabase::CInArcInfo & ai = m_Database.Volumes[0].ArcInfo;
								if(ai.IsTherePrev())
									otherArc = &ai.PrevArc;
								else
									prevChecked = true;
							}
							else {
								const CDatabase::CInArcInfo & ai = m_Database.Volumes[numTempVolumes - 1].ArcInfo;
								if(ai.IsThereNext())
									otherArc = &ai.NextArc;
								else {
									prevChecked = true;
									m_Database.Volumes.DeleteFrontal(numTempVolumes);
									numTempVolumes = 0;
								}
							}
						}
						if(!otherArc) {
							const CDatabase::CInArcInfo & ai = m_Database.Volumes.Back().ArcInfo;
							if(ai.IsThereNext())
								otherArc = &ai.NextArc;
						}
						if(!otherArc)
							break;
						if(!openVolumeCallback)
							break;
						// printf("\n%s", otherArc->FileName);
						const UString fullName = MultiByteToUnicodeString(otherArc->FileName, CP_ACP);

						if(!startVolName_was_Requested) {
							// some "bad" cab example can contain the link to itself.
							startVolName_was_Requested = true;
							{
								NCOM::CPropVariant prop;
								RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
								if(prop.vt == VT_BSTR)
									startVolName = prop.bstrVal;
							}
							if(fullName == startVolName)
								break;
						}

						HRESULT result = openVolumeCallback->GetStream(fullName, &nextStream);
						if(result == S_OK)
							break;
						if(result != S_FALSE)
							return result;

						if(!_errorMessage.IsEmpty())
							_errorMessage.Add_LF();
						_errorMessage += "Can't open volume: ";
						_errorMessage += fullName;

						if(prevChecked)
							break;
						prevChecked = true;
						if(numTempVolumes != 0) {
							m_Database.Volumes.DeleteFrontal(numTempVolumes);
							numTempVolumes = 0;
						}
					}
				} // read nextStream iteration

				if(numTempVolumes != 0) {
					m_Database.Volumes.DeleteFrontal(numTempVolumes);
					numTempVolumes = 0;
				}
				if(m_Database.Volumes.IsEmpty())
					return S_FALSE;
				else {
					m_Database.FillSortAndShrink();
					if(!m_Database.Check())
						return S_FALSE;
				}
			}
			COM_TRY_END
			return S_OK;
		}

		STDMETHODIMP CHandler::Close()
		{
			_errorMessage.Empty();
			_isArc = false;
			_errorInHeaders = false;
			_unexpectedEnd = false;
			// _mainVolIndex = -1;
			_phySize = 0;
			_offset = 0;
			m_Database.Clear();
			return S_OK;
		}

		class CFolderOutStream : public ISequentialOutStream, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
		private:
			const CMvDatabaseEx * m_Database;
			const CRecordVector<bool> * m_ExtractStatuses;
			Byte * TempBuf;
			uint32 TempBufSize;
			uint   NumIdenticalFiles;
			bool TempBufMode;
			uint32 m_BufStartFolderOffset;
			uint   m_StartIndex;
			uint   m_CurrentIndex;
			CMyComPtr <IArchiveExtractCallback> m_ExtractCallback;
			bool m_TestMode;
			CMyComPtr <ISequentialOutStream> m_RealOutStream;
			bool m_IsOk;
			bool m_FileIsOpen;
			uint32 m_RemainFileSize;
			uint64 m_FolderSize;
			uint64 m_PosInFolder;

			void FreeTempBuf()
			{
				ZFREE(TempBuf);
			}
			HRESULT OpenFile();
			HRESULT CloseFileWithResOp(int32 resOp);
			HRESULT CloseFile();
			HRESULT Write2(const void * data, uint32 size, uint32 * processedSize, bool isOK);
		public:
			HRESULT WriteEmptyFiles();
			CFolderOutStream() : TempBuf(NULL) 
			{
			}
			~CFolderOutStream() 
			{
				FreeTempBuf();
			}
			void Init(const CMvDatabaseEx * database, const CRecordVector<bool> * extractStatuses, unsigned startIndex, uint64 folderSize,
				IArchiveExtractCallback * extractCallback, bool testMode);
			HRESULT FlushCorrupted(unsigned folderIndex);
			HRESULT Unsupported();

			bool NeedMoreWrite() const { return (m_FolderSize > m_PosInFolder); }
			uint64 GetRemain() const { return m_FolderSize - m_PosInFolder; }
			uint64 GetPosInFolder() const { return m_PosInFolder; }
		};

		void CFolderOutStream::Init(const CMvDatabaseEx * database, const CRecordVector<bool> * extractStatuses,
			unsigned startIndex, uint64 folderSize, IArchiveExtractCallback * extractCallback, bool testMode)
		{
			m_Database = database;
			m_ExtractStatuses = extractStatuses;
			m_StartIndex = startIndex;
			m_FolderSize = folderSize;
			m_ExtractCallback = extractCallback;
			m_TestMode = testMode;
			m_CurrentIndex = 0;
			m_PosInFolder = 0;
			m_FileIsOpen = false;
			m_IsOk = true;
			TempBufMode = false;
			NumIdenticalFiles = 0;
		}

		HRESULT CFolderOutStream::CloseFileWithResOp(int32 resOp)
		{
			m_RealOutStream.Release();
			m_FileIsOpen = false;
			NumIdenticalFiles--;
			return m_ExtractCallback->SetOperationResult(resOp);
		}

		HRESULT CFolderOutStream::CloseFile()
		{
			return CloseFileWithResOp(m_IsOk ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kDataError);
		}

		HRESULT CFolderOutStream::OpenFile()
		{
			if(NumIdenticalFiles == 0) {
				const CMvDatabaseEx::CMvItem & mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
				const CItem & item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
				uint   numExtractItems = 0;
				uint   curIndex;
				for(curIndex = m_CurrentIndex; curIndex < m_ExtractStatuses->Size(); curIndex++) {
					const CMvDatabaseEx::CMvItem & mvItem2 = m_Database->Items[m_StartIndex + curIndex];
					const CItem & item2 = m_Database->Volumes[mvItem2.VolumeIndex].Items[mvItem2.ItemIndex];
					if(item.Offset != item2.Offset || item.Size != item2.Size || item.Size == 0)
						break;
					if(!m_TestMode && (*m_ExtractStatuses)[curIndex])
						numExtractItems++;
				}
				NumIdenticalFiles = (curIndex - m_CurrentIndex);
				SETIFZ(NumIdenticalFiles, 1);
				TempBufMode = false;
				if(numExtractItems > 1) {
					if(!TempBuf || item.Size > TempBufSize) {
						FreeTempBuf();
						TempBuf = (Byte *)SAlloc::M(item.Size);
						TempBufSize = item.Size;
						if(!TempBuf)
							return E_OUTOFMEMORY;
					}
					TempBufMode = true;
					m_BufStartFolderOffset = item.Offset;
				}
				else if(numExtractItems == 1) {
					while(NumIdenticalFiles && !(*m_ExtractStatuses)[m_CurrentIndex]) {
						CMyComPtr<ISequentialOutStream> stream;
						RINOK(m_ExtractCallback->GetStream(m_StartIndex + m_CurrentIndex, &stream, NExtractArc::NAskMode::kSkip));
						if(stream)
							return E_FAIL;
						RINOK(m_ExtractCallback->PrepareOperation(NExtractArc::NAskMode::kSkip));
						m_CurrentIndex++;
						m_FileIsOpen = true;
						CloseFile();
					}
				}
			}
			int32 askMode = (*m_ExtractStatuses)[m_CurrentIndex] ? (m_TestMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract) : NExtractArc::NAskMode::kSkip;
			RINOK(m_ExtractCallback->GetStream(m_StartIndex + m_CurrentIndex, &m_RealOutStream, askMode));
			if(!m_RealOutStream && !m_TestMode)
				askMode = NExtractArc::NAskMode::kSkip;
			return m_ExtractCallback->PrepareOperation(askMode);
		}

		HRESULT CFolderOutStream::WriteEmptyFiles()
		{
			if(m_FileIsOpen)
				return S_OK;
			for(; m_CurrentIndex < m_ExtractStatuses->Size(); m_CurrentIndex++) {
				const CMvDatabaseEx::CMvItem & mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
				const CItem & item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
				uint64 fileSize = item.Size;
				if(fileSize != 0)
					return S_OK;
				HRESULT result = OpenFile();
				m_RealOutStream.Release();
				RINOK(result);
				RINOK(m_ExtractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
			}
			return S_OK;
		}

		HRESULT CFolderOutStream::Write2(const void * data, uint32 size, uint32 * processedSize, bool isOK)
		{
			COM_TRY_BEGIN
			uint32 realProcessed = 0;
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(m_FileIsOpen) {
					uint32 numBytesToWrite = smin(m_RemainFileSize, size);
					HRESULT res = S_OK;
					if(numBytesToWrite != 0) {
						if(!isOK)
							m_IsOk = false;
						if(m_RealOutStream) {
							uint32 processedSizeLocal = 0;
							res = m_RealOutStream->Write((const Byte *)data, numBytesToWrite, &processedSizeLocal);
							numBytesToWrite = processedSizeLocal;
						}
						if(TempBufMode && TempBuf)
							memcpy(TempBuf + (m_PosInFolder - m_BufStartFolderOffset), data, numBytesToWrite);
					}
					realProcessed += numBytesToWrite;
					ASSIGN_PTR(processedSize, realProcessed);
					data = (const void *)((const Byte *)data + numBytesToWrite);
					size -= numBytesToWrite;
					m_RemainFileSize -= numBytesToWrite;
					m_PosInFolder += numBytesToWrite;
					if(res != S_OK)
						return res;
					if(m_RemainFileSize == 0) {
						RINOK(CloseFile());
						while(NumIdenticalFiles) {
							HRESULT result = OpenFile();
							m_FileIsOpen = true;
							m_CurrentIndex++;
							if(result == S_OK && m_RealOutStream && TempBuf)
								result = WriteStream(m_RealOutStream, TempBuf, (size_t)(m_PosInFolder - m_BufStartFolderOffset));
							if(!TempBuf && TempBufMode && m_RealOutStream) {
								RINOK(CloseFileWithResOp(NExtractArc::NOperationResult::kUnsupportedMethod));
							}
							else {
								RINOK(CloseFile());
							}
							RINOK(result);
						}
						TempBufMode = false;
					}
					if(realProcessed > 0)
						break;  // with this break this function works as Write-Part
				}
				else {
					if(m_CurrentIndex >= m_ExtractStatuses->Size()) {
						// we ignore extra data;
						realProcessed += size;
						ASSIGN_PTR(processedSize, realProcessed);
						m_PosInFolder += size;
						return S_OK;
						// return E_FAIL;
					}
					const CMvDatabaseEx::CMvItem & mvItem = m_Database->Items[m_StartIndex + m_CurrentIndex];
					const CItem & item = m_Database->Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
					m_RemainFileSize = item.Size;
					uint32 fileOffset = item.Offset;
					if(fileOffset < m_PosInFolder)
						return E_FAIL;
					if(fileOffset > m_PosInFolder) {
						uint32 numBytesToWrite = smin(fileOffset - (uint32)m_PosInFolder, size);
						realProcessed += numBytesToWrite;
						ASSIGN_PTR(processedSize, realProcessed);
						data = (const void *)((const Byte *)data + numBytesToWrite);
						size -= numBytesToWrite;
						m_PosInFolder += numBytesToWrite;
					}
					if(fileOffset == m_PosInFolder) {
						RINOK(OpenFile());
						m_FileIsOpen = true;
						m_CurrentIndex++;
						m_IsOk = true;
					}
				}
			}
			return WriteEmptyFiles();
			COM_TRY_END
		}

		STDMETHODIMP CFolderOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			return Write2(data, size, processedSize, true);
		}

		HRESULT CFolderOutStream::FlushCorrupted(unsigned folderIndex)
		{
			if(!NeedMoreWrite()) {
				CMyComPtr<IArchiveExtractCallbackMessage> callbackMessage;
				m_ExtractCallback.QueryInterface(IID_IArchiveExtractCallbackMessage, &callbackMessage);
				if(callbackMessage) {
					RINOK(callbackMessage->ReportExtractResult(NEventIndexType::kBlockIndex, folderIndex, NExtractArc::NOperationResult::kDataError));
				}
				return S_OK;
			}
			const uint kBufSize = (1 << 12);
			Byte buf[kBufSize];
			for(uint i = 0; i < kBufSize; i++)
				buf[i] = 0;
			for(;;) {
				if(!NeedMoreWrite())
					return S_OK;
				uint64 remain = GetRemain();
				uint32 size = (remain < kBufSize ? (uint32)remain : (uint32)kBufSize);
				uint32 processedSizeLocal = 0;
				RINOK(Write2(buf, size, &processedSizeLocal, false));
			}
		}

		HRESULT CFolderOutStream::Unsupported()
		{
			while(m_CurrentIndex < m_ExtractStatuses->Size()) {
				HRESULT result = OpenFile();
				if(result != S_FALSE && result != S_OK)
					return result;
				m_RealOutStream.Release();
				RINOK(m_ExtractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
				m_CurrentIndex++;
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testModeSpec, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = m_Database.Items.Size();
			if(numItems == 0)
				return S_OK;
			bool testMode = (testModeSpec != 0);
			uint64 totalUnPacked = 0;
			uint32 i;
			int lastFolder = -2;
			uint64 lastFolderSize = 0;
			for(i = 0; i < numItems; i++) {
				uint   index = allFilesMode ? i : indices[i];
				const CMvDatabaseEx::CMvItem & mvItem = m_Database.Items[index];
				const CItem & item = m_Database.Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
				if(!item.IsDir()) {
					int folderIndex = m_Database.GetFolderIndex(&mvItem);
					if(folderIndex != lastFolder)
						totalUnPacked += lastFolderSize;
					lastFolder = folderIndex;
					lastFolderSize = item.GetEndOffset();
				}
			}
			totalUnPacked += lastFolderSize;
			extractCallback->SetTotal(totalUnPacked);
			totalUnPacked = 0;
			uint64 totalPacked = 0;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			NCompress::NDeflate::NDecoder::CCOMCoder * deflateDecoderSpec = NULL;
			CMyComPtr<ICompressCoder> deflateDecoder;
			NCompress::NLzx::CDecoder * lzxDecoderSpec = NULL;
			CMyComPtr<IUnknown> lzxDecoder;
			NCompress::NQuantum::CDecoder * quantumDecoderSpec = NULL;
			CMyComPtr<IUnknown> quantumDecoder;
			CCabBlockInStream * cabBlockInStreamSpec = new CCabBlockInStream();
			CMyComPtr<ISequentialInStream> cabBlockInStream = cabBlockInStreamSpec;
			if(!cabBlockInStreamSpec->Create())
				return E_OUTOFMEMORY;
			CRecordVector <bool> extractStatuses;
			for(i = 0;;) {
				lps->OutSize = totalUnPacked;
				lps->InSize = totalPacked;
				RINOK(lps->SetCur());
				if(i >= numItems)
					break;
				uint   index = allFilesMode ? i : indices[i];
				const  CMvDatabaseEx::CMvItem & mvItem = m_Database.Items[index];
				const  CDatabaseEx & db = m_Database.Volumes[mvItem.VolumeIndex];
				uint   itemIndex = mvItem.ItemIndex;
				const  CItem & item = db.Items[itemIndex];
				i++;
				if(item.IsDir()) {
					int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
					CMyComPtr<ISequentialOutStream> realOutStream;
					RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
					RINOK(extractCallback->PrepareOperation(askMode));
					realOutStream.Release();
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					continue;
				}
				int folderIndex = m_Database.GetFolderIndex(&mvItem);
				if(folderIndex < 0) {
					// If we need previous archive
					int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
					CMyComPtr<ISequentialOutStream> realOutStream;
					RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
					RINOK(extractCallback->PrepareOperation(askMode));
					realOutStream.Release();
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kDataError));
					continue;
				}
				uint   startIndex2 = m_Database.FolderStartFileIndex[folderIndex];
				uint   startIndex = startIndex2;
				extractStatuses.Clear();
				for(; startIndex < index; startIndex++)
					extractStatuses.Add(false);
				extractStatuses.Add(true);
				startIndex++;
				uint64 curUnpack = item.GetEndOffset();
				for(; i < numItems; i++) {
					uint   indexNext = allFilesMode ? i : indices[i];
					const  CMvDatabaseEx::CMvItem & mvItem2 = m_Database.Items[indexNext];
					const  CItem & item2 = m_Database.Volumes[mvItem2.VolumeIndex].Items[mvItem2.ItemIndex];
					if(!item2.IsDir()) {
						int newFolderIndex = m_Database.GetFolderIndex(&mvItem2);
						if(newFolderIndex != folderIndex)
							break;
						for(; startIndex < indexNext; startIndex++)
							extractStatuses.Add(false);
						extractStatuses.Add(true);
						startIndex++;
						curUnpack = item2.GetEndOffset();
					}
				}
				CFolderOutStream * cabFolderOutStream = new CFolderOutStream;
				CMyComPtr<ISequentialOutStream> outStream(cabFolderOutStream);
				uint   folderIndex2 = item.GetFolderIndex(db.Folders.Size());
				const  CFolder &folder = db.Folders[folderIndex2];
				cabFolderOutStream->Init(&m_Database, &extractStatuses, startIndex2, curUnpack, extractCallback, testMode);
				cabBlockInStreamSpec->MsZip = false;
				HRESULT res = S_OK;
				switch(folder.GetMethod()) {
					case NHeader::NMethod::kNone:
						break;
					case NHeader::NMethod::kMSZip:
						if(!deflateDecoder) {
							deflateDecoderSpec = new NCompress::NDeflate::NDecoder::CCOMCoder;
							deflateDecoder = deflateDecoderSpec;
						}
						cabBlockInStreamSpec->MsZip = true;
						break;
					case NHeader::NMethod::kLZX:
						if(!lzxDecoder) {
							lzxDecoderSpec = new NCompress::NLzx::CDecoder;
							lzxDecoder = lzxDecoderSpec;
						}
						res = lzxDecoderSpec->SetParams_and_Alloc(folder.MethodMinor);
						break;
					case NHeader::NMethod::kQuantum:
						if(!quantumDecoder) {
							quantumDecoderSpec = new NCompress::NQuantum::CDecoder;
							quantumDecoder = quantumDecoderSpec;
						}
						res = quantumDecoderSpec->SetParams(folder.MethodMinor);
						break;
					default:
						res = E_INVALIDARG;
						break;
				}
				if(res == E_INVALIDARG) {
					RINOK(cabFolderOutStream->Unsupported());
					totalUnPacked += curUnpack;
					continue;
				}
				RINOK(res);
				{
					uint   volIndex = mvItem.VolumeIndex;
					int    locFolderIndex = item.GetFolderIndex(db.Folders.Size());
					bool   keepHistory = false;
					bool   keepInputBuffer = false;
					bool   thereWasNotAlignedChunk = false;
					for(uint32 bl = 0; cabFolderOutStream->NeedMoreWrite();) {
						if(volIndex >= m_Database.Volumes.Size()) {
							res = S_FALSE;
							break;
						}
						const CDatabaseEx &db2 = m_Database.Volumes[volIndex];
						const CFolder &folder2 = db2.Folders[locFolderIndex];
						if(bl == 0) {
							cabBlockInStreamSpec->ReservedSize = db2.ArcInfo.GetDataBlockReserveSize();
							RINOK(db2.Stream->Seek(db2.StartPosition + folder2.DataStart, STREAM_SEEK_SET, NULL));
						}
						if(bl == folder2.NumDataBlocks) {
							// 
							// CFolder::NumDataBlocks (CFFOLDER::cCFData in CAB specification) is 16-bit.
							// But there are some big CAB archives from MS that contain more
							// than (0xFFFF) CFDATA blocks in folder.
							// Old cab extracting software can show error (or ask next volume)
							// but cab extracting library in new Windows ignores this error.
							// 15.00 : We also try to ignore such error, if archive is not multi-volume.
							// 
							if(m_Database.Volumes.Size() > 1) {
								volIndex++;
								locFolderIndex = 0;
								bl = 0;
								continue;
							}
						}
						bl++;
						if(!keepInputBuffer)
							cabBlockInStreamSpec->InitForNewBlock();
						uint32 packSize, unpackSize;
						res = cabBlockInStreamSpec->PreRead(db2.Stream, packSize, unpackSize);
						if(res == S_FALSE)
							break;
						RINOK(res);
						keepInputBuffer = (unpackSize == 0);
						if(!keepInputBuffer) {
							uint64 totalUnPacked2 = totalUnPacked + cabFolderOutStream->GetPosInFolder();
							totalPacked += packSize;
							lps->OutSize = totalUnPacked2;
							lps->InSize = totalPacked;
							RINOK(lps->SetCur());
							const uint32 kBlockSizeMax = (1 << 15);
							// We don't try to reduce last block.
							// Note that LZX converts data with x86 filter.
							// and filter needs larger input data than reduced size.
							// It's simpler to decompress full chunk here.
							// also we need full block for quantum for more integrity checks 
							if(unpackSize > kBlockSizeMax) {
								res = S_FALSE;
								break;
							}
							if(unpackSize != kBlockSizeMax) {
								if(thereWasNotAlignedChunk) {
									res = S_FALSE;
									break;
								}
								thereWasNotAlignedChunk = true;
							}
							uint64 unpackSize64 = unpackSize;
							uint32 packSizeChunk = cabBlockInStreamSpec->GetPackSizeAvail();
							switch(folder2.GetMethod()) {
								case NHeader::NMethod::kNone:
									res = copyCoder->Code(cabBlockInStream, outStream, NULL, &unpackSize64, NULL);
									break;
								case NHeader::NMethod::kMSZip:
									deflateDecoderSpec->Set_KeepHistory(keepHistory);
									// v9.31: now we follow MSZIP specification that requires to finish deflate
									// stream at the end of each block.
									// But PyCabArc can create CAB archives that doesn't have finish marker at the end of block.
									// Cabarc probably ignores such errors in cab archives.
									// Maybe we also should ignore that error?
									// Or we should extract full file and show the warning? 
									deflateDecoderSpec->Set_NeedFinishInput(true);
									res = deflateDecoder->Code(cabBlockInStream, outStream, NULL, &unpackSize64, NULL);
									if(res == S_OK) {
										if(!deflateDecoderSpec->IsFinished())
											res = S_FALSE;
										if(!deflateDecoderSpec->IsFinalBlock())
											res = S_FALSE;
									}
									break;
								case NHeader::NMethod::kLZX:
									lzxDecoderSpec->SetKeepHistory(keepHistory);
									lzxDecoderSpec->KeepHistoryForNext = true;
									res = lzxDecoderSpec->Code(cabBlockInStreamSpec->GetData(), packSizeChunk, unpackSize);
									if(res == S_OK)
										res = WriteStream(outStream, lzxDecoderSpec->GetUnpackData(), lzxDecoderSpec->GetUnpackSize());
									break;
								case NHeader::NMethod::kQuantum:
									res = quantumDecoderSpec->Code(cabBlockInStreamSpec->GetData(), packSizeChunk, outStream, unpackSize, keepHistory);
									break;
							}
							if(res != S_OK) {
								if(res != S_FALSE)
									RINOK(res);
								break;
							}
							keepHistory = true;
						}
					}
					if(res == S_OK) {
						RINOK(cabFolderOutStream->WriteEmptyFiles());
					}
				}
				if(res != S_OK || cabFolderOutStream->NeedMoreWrite()) {
					RINOK(cabFolderOutStream->FlushCorrupted(folderIndex2));
				}
				totalUnPacked += curUnpack;
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = m_Database.Items.Size();
			return S_OK;
		}
		//
		// CabIn
		//
		struct CUnexpectedEndException {
		};
		void CInArchive::Skip(uint size)
		{
			if(_inBuffer.Skip(size) != size)
				throw CUnexpectedEndException();
		}
		void CInArchive::Read(Byte * data, uint size)
		{
			if(_inBuffer.ReadBytes(data, size) != size)
				throw CUnexpectedEndException();
		}
		void CInArchive::ReadName(AString &s)
		{
			for(size_t i = 0; i < ((size_t)1 << 13); i++) {
				Byte b;
				if(!_inBuffer.ReadByte(b))
					throw CUnexpectedEndException();
				if(b == 0) {
					s.SetFrom((const char *)(const Byte *)_tempBuf, (uint)i);
					return;
				}
				if(_tempBuf.Size() == i)
					_tempBuf.ChangeSize_KeepData(i * 2, i);
				_tempBuf[i] = b;
			}
			for(;;) {
				Byte b;
				if(!_inBuffer.ReadByte(b))
					throw CUnexpectedEndException();
				if(b == 0)
					break;
			}
			ErrorInNames = true;
			s = "[ERROR-LONG-PATH]";
		}
		void CInArchive::ReadOtherArc(COtherArc &oa)
		{
			ReadName(oa.FileName);
			ReadName(oa.DiskName);
		}

		struct CSignatureFinder {
			Byte * Buf;
			uint32 Pos;
			uint32 End;
			const Byte * Signature;
			uint32 SignatureSize;
			uint32 _HeaderSize;
			uint32 _AlignSize;
			uint32 _BufUseCapacity;
			ISequentialInStream * Stream;
			uint64 Processed; // Global offset of start of Buf
			const uint64 * SearchLimit;

			uint32 GetTotalCapacity(uint32 basicSize, uint32 headerSize)
			{
				_HeaderSize = headerSize;
				for(_AlignSize = (1 << 5); _AlignSize < _HeaderSize; _AlignSize <<= 1) 
					;
				_BufUseCapacity = basicSize + _AlignSize;
				return _BufUseCapacity + 16;
			}
			/*
			   returns:
			   S_OK      - signature found (at Pos)
			   S_FALSE   - signature not found
			 */
			HRESULT Find();
		};
		HRESULT CSignatureFinder::Find()
		{
			for(;;) {
				Buf[End] = Signature[0]; // it's for fast search;
				while(End - Pos >= _HeaderSize) {
					const Byte * p = Buf + Pos;
					Byte b = Signature[0];
					for(;;) {
						if(*p == b) break; p++;
						if(*p == b) break; p++;
					}
					Pos = (uint32)(p - Buf);
					if(End - Pos < _HeaderSize) {
						Pos = End - _HeaderSize + 1;
						break;
					}
					uint32 i;
					for(i = 1; i < SignatureSize && p[i] == Signature[i]; i++) 
						;
					if(i == SignatureSize)
						return S_OK;
					Pos++;
				}
				if(Pos >= _AlignSize) {
					uint32 num = (Pos & ~(_AlignSize - 1));
					Processed += num;
					Pos -= num;
					End -= num;
					memmove(Buf, Buf + num, End);
				}
				uint32 rem = _BufUseCapacity - End;
				if(SearchLimit) {
					if(Processed + Pos > *SearchLimit)
						return S_FALSE;
					uint64 rem2 = *SearchLimit - (Processed + End) + _HeaderSize;
					SETMIN(rem, (uint32)rem2);
				}
				uint32 processedSize;
				if(Processed == 0 && rem == _BufUseCapacity - _HeaderSize)
					rem -= _AlignSize;  // to make reads more aligned.
				RINOK(Stream->Read(Buf + End, rem, &processedSize));
				if(processedSize == 0)
					return S_FALSE;
				End += processedSize;
			}
		}
		void COtherArc::Clear()
		{
			FileName.Empty();
			DiskName.Empty();
		}
		CArchInfo::CArchInfo() : PerCabinet_AreaSize(0), PerFolder_AreaSize(0), PerDataBlock_AreaSize(0)
		{
		}
		void CArchInfo::Clear()
		{
			PerCabinet_AreaSize = 0;
			PerFolder_AreaSize = 0;
			PerDataBlock_AreaSize = 0;
			PrevArc.Clear();
			NextArc.Clear();
		}

		bool CArchInfo::ReserveBlockPresent() const { return (Flags & NHeader::NArcFlags::kReservePresent) != 0; }
		bool CArchInfo::IsTherePrev() const { return (Flags & NHeader::NArcFlags::kPrevCabinet) != 0; }
		bool CArchInfo::IsThereNext() const { return (Flags & NHeader::NArcFlags::kNextCabinet) != 0; }
		Byte CArchInfo::GetDataBlockReserveSize() const { return (Byte)(ReserveBlockPresent() ? PerDataBlock_AreaSize : 0); }

		bool CDatabase::CInArcInfo::Parse(const Byte * p)
		{
			if(Get32(p + 0x0C) != 0 || Get32(p + 0x14) != 0)
				return false;
			Size = Get32(p + 8);
			if(Size < 36)
				return false;
			Flags = Get16(p + 0x1E);
			if(Flags > 7)
				return false;
			FileHeadersOffset = Get32(p + 0x10);
			if(FileHeadersOffset != 0 && FileHeadersOffset > Size)
				return false;
			VersionMinor = p[0x18];
			VersionMajor = p[0x19];
			NumFolders = Get16(p + 0x1A);
			NumFiles = Get16(p + 0x1C);
			return true;
		}
		void CDatabase::Clear()
		{
			ArcInfo.Clear();
			Folders.Clear();
			Items.Clear();
		}
		bool CDatabase::IsTherePrevFolder() const
		{
			FOR_VECTOR(i, Items) {
				if(Items[i].ContinuedFromPrev())
					return true;
			}
			return false;
		}
		int CDatabase::GetNumberOfNewFolders() const
		{
			int res = Folders.Size();
			if(IsTherePrevFolder())
				res--;
			return res;
		}
		HRESULT CInArchive::Open2(CDatabaseEx &db, const uint64 * searchHeaderSizeLimit)
		{
			IsArc = false;
			ErrorInNames = false;
			UnexpectedEnd = false;
			HeaderError = false;
			db.Clear();
			RINOK(db.Stream->Seek(0, STREAM_SEEK_CUR, &db.StartPosition));
			// uint64 temp = db.StartPosition;
			CByteBuffer buffer;
			CDatabase::CInArcInfo & ai = db.ArcInfo;
			uint64 startInBuf = 0;
			CLimitedSequentialInStream * limitedStreamSpec = NULL;
			CMyComPtr<ISequentialInStream> limitedStream;
			// for(int iii = 0; iii < 10000; iii++)
			{
				// db.StartPosition = temp; RINOK(db.Stream->Seek(db.StartPosition, STREAM_SEEK_SET, NULL));
				const uint32 kMainHeaderSize = 32;
				Byte header[kMainHeaderSize];
				const uint32 kBufSize = 1 << 15;
				RINOK(ReadStream_FALSE(db.Stream, header, kMainHeaderSize));
				if(memcmp(header, NHeader::kMarker, NHeader::kMarkerSize) == 0 && ai.Parse(header)) {
					limitedStreamSpec = new CLimitedSequentialInStream;
					limitedStream = limitedStreamSpec;
					limitedStreamSpec->SetStream(db.Stream);
					limitedStreamSpec->Init(ai.Size - NHeader::kMarkerSize);
					buffer.Alloc(kBufSize);
					memcpy(buffer, header, kMainHeaderSize);
					uint32 numProcessedBytes;
					RINOK(limitedStream->Read(buffer + kMainHeaderSize, kBufSize - kMainHeaderSize, &numProcessedBytes));
					_inBuffer.SetBuf(buffer, (uint32)kBufSize, kMainHeaderSize + numProcessedBytes, kMainHeaderSize);
				}
				else {
					if(searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
						return S_FALSE;
					CSignatureFinder finder;
					finder.Stream = db.Stream;
					finder.Signature = NHeader::kMarker;
					finder.SignatureSize = NHeader::kMarkerSize;
					finder.SearchLimit = searchHeaderSizeLimit;
					buffer.Alloc(finder.GetTotalCapacity(kBufSize, kMainHeaderSize));
					finder.Buf = buffer;
					memcpy(buffer, header, kMainHeaderSize);
					finder.Processed = db.StartPosition;
					finder.End = kMainHeaderSize;
					finder.Pos = 1;
					for(;;) {
						RINOK(finder.Find());
						if(ai.Parse(finder.Buf + finder.Pos)) {
							db.StartPosition = finder.Processed + finder.Pos;
							limitedStreamSpec = new CLimitedSequentialInStream;
							limitedStreamSpec->SetStream(db.Stream);
							limitedStream = limitedStreamSpec;
							uint32 remInFinder = finder.End - finder.Pos;
							if(ai.Size <= remInFinder) {
								limitedStreamSpec->Init(0);
								finder.End = finder.Pos + ai.Size;
							}
							else
								limitedStreamSpec->Init(ai.Size - remInFinder);
							startInBuf = finder.Pos;
							_inBuffer.SetBuf(buffer, (uint32)kBufSize, finder.End, finder.Pos + kMainHeaderSize);
							break;
						}
						finder.Pos++;
					}
				}
			}
			IsArc = true;
			_inBuffer.SetStream(limitedStream);
			if(_tempBuf.Size() == 0)
				_tempBuf.Alloc(1 << 12);
			Byte p[16];
			unsigned nextSize = 4 + (ai.ReserveBlockPresent() ? 4 : 0);
			Read(p, nextSize);
			ai.SetID = Get16(p);
			ai.CabinetNumber = Get16(p + 2);
			if(ai.ReserveBlockPresent()) {
				ai.PerCabinet_AreaSize = Get16(p + 4);
				ai.PerFolder_AreaSize = p[6];
				ai.PerDataBlock_AreaSize = p[7];
				Skip(ai.PerCabinet_AreaSize);
			}
			if(ai.IsTherePrev()) ReadOtherArc(ai.PrevArc);
			if(ai.IsThereNext()) ReadOtherArc(ai.NextArc);
			uint32 i;
			db.Folders.ClearAndReserve(ai.NumFolders);
			for(i = 0; i < ai.NumFolders; i++) {
				Read(p, 8);
				CFolder folder;
				folder.DataStart = Get32(p);
				folder.NumDataBlocks = Get16(p + 4);
				folder.MethodMajor = p[6];
				folder.MethodMinor = p[7];
				Skip(ai.PerFolder_AreaSize);
				db.Folders.AddInReserved(folder);
			}
			// for(int iii = 0; iii < 10000; iii++) {
			if(_inBuffer.GetProcessedSize() - startInBuf != ai.FileHeadersOffset) {
				// printf("\n!!! Seek Error !!!!\n");
				// fflush(stdout);
				RINOK(db.Stream->Seek(db.StartPosition + ai.FileHeadersOffset, STREAM_SEEK_SET, NULL));
				limitedStreamSpec->Init(ai.Size - ai.FileHeadersOffset);
				_inBuffer.Init();
			}
			db.Items.ClearAndReserve(ai.NumFiles);
			for(i = 0; i < ai.NumFiles; i++) {
				Read(p, 16);
				CItem &item = db.Items.AddNewInReserved();
				item.Size = Get32(p);
				item.Offset = Get32(p + 4);
				item.FolderIndex = Get16(p + 8);
				uint16 pureDate = Get16(p + 10);
				uint16 pureTime = Get16(p + 12);
				item.Time = (((uint32)pureDate << 16)) | pureTime;
				item.Attributes = Get16(p + 14);
				ReadName(item.Name);
				if(item.GetFolderIndex(db.Folders.Size()) >= (int)db.Folders.Size()) {
					HeaderError = true;
					return S_FALSE;
				}
			}
			// }
			return S_OK;
		}

		HRESULT CInArchive::Open(CDatabaseEx &db, const uint64 * searchHeaderSizeLimit)
		{
			try {
				return Open2(db, searchHeaderSizeLimit);
			}
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(CUnexpectedEndException &) { UnexpectedEnd = true; return S_FALSE; }
		}

		#define RINOZ(x) { int __tt = (x); if(__tt != 0) return __tt; }

		static int CompareMvItems(const CMvDatabaseEx::CMvItem * p1, const CMvDatabaseEx::CMvItem * p2, void * param)
		{
			const CMvDatabaseEx & mvDb = *(const CMvDatabaseEx*)param;
			const CDatabaseEx & db1 = mvDb.Volumes[p1->VolumeIndex];
			const CDatabaseEx & db2 = mvDb.Volumes[p2->VolumeIndex];
			const CItem &item1 = db1.Items[p1->ItemIndex];
			const CItem &item2 = db2.Items[p2->ItemIndex];;
			bool isDir1 = item1.IsDir();
			bool isDir2 = item2.IsDir();
			if(isDir1 && !isDir2) return -1;
			if(isDir2 && !isDir1) return 1;
			int f1 = mvDb.GetFolderIndex(p1);
			int f2 = mvDb.GetFolderIndex(p2);
			RINOZ(MyCompare(f1, f2));
			RINOZ(MyCompare(item1.Offset, item2.Offset));
			RINOZ(MyCompare(item1.Size, item2.Size));
			RINOZ(MyCompare(p1->VolumeIndex, p2->VolumeIndex));
			return MyCompare(p1->ItemIndex, p2->ItemIndex);
		}
		int FASTCALL CMvDatabaseEx::GetFolderIndex(const CMvItem * mvi) const
		{
			const CDatabaseEx &db = Volumes[mvi->VolumeIndex];
			return StartFolderOfVol[mvi->VolumeIndex] + db.Items[mvi->ItemIndex].GetFolderIndex(db.Folders.Size());
		}
		void CMvDatabaseEx::Clear()
		{
			Volumes.Clear();
			Items.Clear();
			StartFolderOfVol.Clear();
			FolderStartFileIndex.Clear();
		}
		bool CMvDatabaseEx::AreItemsEqual(unsigned i1, unsigned i2)
		{
			const CMvItem * p1 = &Items[i1];
			const CMvItem * p2 = &Items[i2];
			const CDatabaseEx &db1 = Volumes[p1->VolumeIndex];
			const CDatabaseEx &db2 = Volumes[p2->VolumeIndex];
			const CItem &item1 = db1.Items[p1->ItemIndex];
			const CItem &item2 = db2.Items[p2->ItemIndex];;
			return GetFolderIndex(p1) == GetFolderIndex(p2) && item1.Offset == item2.Offset && item1.Size == item2.Size && item1.Name == item2.Name;
		}
		void CMvDatabaseEx::FillSortAndShrink()
		{
			Items.Clear();
			StartFolderOfVol.Clear();
			FolderStartFileIndex.Clear();
			int offset = 0;
			FOR_VECTOR(v, Volumes) {
				const CDatabaseEx &db = Volumes[v];
				int curOffset = offset;
				if(db.IsTherePrevFolder())
					curOffset--;
				StartFolderOfVol.Add(curOffset);
				offset += db.GetNumberOfNewFolders();

				CMvItem mvItem;
				mvItem.VolumeIndex = v;
				FOR_VECTOR(i, db.Items) {
					mvItem.ItemIndex = i;
					Items.Add(mvItem);
				}
			}
			if(Items.Size() > 1) {
				Items.Sort(CompareMvItems, (void *)this);
				uint   j = 1;
				for(uint i = 1; i < Items.Size(); i++)
					if(!AreItemsEqual(i, i-1))
						Items[j++] = Items[i];
				Items.DeleteFrom(j);
			}
			FOR_VECTOR(i, Items) {
				int folderIndex = GetFolderIndex(&Items[i]);
				while(folderIndex >= (int)FolderStartFileIndex.Size())
					FolderStartFileIndex.Add(i);
			}
		}

		bool CMvDatabaseEx::Check()
		{
			for(uint v = 1; v < Volumes.Size(); v++) {
				const CDatabaseEx &db1 = Volumes[v];
				if(db1.IsTherePrevFolder()) {
					const CDatabaseEx &db0 = Volumes[v - 1];
					if(db0.Folders.IsEmpty() || db1.Folders.IsEmpty())
						return false;
					const CFolder &f0 = db0.Folders.Back();
					const CFolder &f1 = db1.Folders.Front();
					if(f0.MethodMajor != f1.MethodMajor ||
								f0.MethodMinor != f1.MethodMinor)
						return false;
				}
			}

			uint32 beginPos = 0;
			uint64 endPos = 0;
			int prevFolder = -2;
			FOR_VECTOR(i, Items) {
				const CMvItem &mvItem = Items[i];
				int fIndex = GetFolderIndex(&mvItem);
				if(fIndex >= (int)FolderStartFileIndex.Size())
					return false;
				const CItem &item = Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
				if(item.IsDir())
					continue;
				int folderIndex = GetFolderIndex(&mvItem);
				if(folderIndex != prevFolder)
					prevFolder = folderIndex;
				else if(item.Offset < endPos && (item.Offset != beginPos || item.GetEndOffset() != endPos))
					return false;
				beginPos = item.Offset;
				endPos = item.GetEndOffset();
			}
			return true;
		}
	}
}
//