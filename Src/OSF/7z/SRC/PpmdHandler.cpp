/* PpmdHandler.cpp -- PPMd format handler
   2015-11-30 : Igor Pavlov : Public domain
   This code is based on:
   PPMd var.H (2001) / var.I (2002): Dmitry Shkarin : Public domain
   Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

namespace NArchive {
	namespace NPpmd {
		static const uint32 kHeaderSize = 16;
		static const uint32 kSignature = 0x84ACAF8F;
		static const uint kNewHeaderVer = 8;

		struct CItem {
			uint32 Attrib;
			uint32 Time;
			AString Name;
			uint   Order;
			uint   MemInMB;
			uint   Ver;
			uint   Restor;

			HRESULT ReadHeader(ISequentialInStream * s, uint32 &headerSize);
			bool IsSupported() const { return Ver == 7 || (Ver == 8 && Restor <= 1); }
		};

		HRESULT CItem::ReadHeader(ISequentialInStream * s, uint32 &headerSize)
		{
			Byte h[kHeaderSize];
			RINOK(ReadStream_FALSE(s, h, kHeaderSize));
			if(GetUi32(h) != kSignature)
				return S_FALSE;
			Attrib = GetUi32(h + 4);
			Time = GetUi32(h + 12);
			unsigned info = GetUi16(h + 8);
			Order = (info & 0xF) + 1;
			MemInMB = ((info >> 4) & 0xFF) + 1;
			Ver = info >> 12;
			if(Ver < 6 || Ver > 11) 
				return S_FALSE;
			uint32 nameLen = GetUi16(h + 10);
			Restor = nameLen >> 14;
			if(Restor > 2)
				return S_FALSE;
			if(Ver >= kNewHeaderVer)
				nameLen &= 0x3FFF;
			if(nameLen > (1 << 9))
				return S_FALSE;
			char * name = Name.GetBuf(nameLen);
			HRESULT res = ReadStream_FALSE(s, name, nameLen);
			Name.ReleaseBuf_CalcLen(nameLen);
			headerSize = kHeaderSize + nameLen;
			return res;
		}

		class CHandler : public IInArchive, public IArchiveOpenSeq, public CMyUnknownImp {
			CItem _item;
			uint32 _headerSize;
			bool _packSize_Defined;
			uint64 _packSize;
			CMyComPtr<ISequentialInStream> _stream;
			void GetVersion(NCOM::CPropVariant &prop);
		public:
			MY_UNKNOWN_IMP2(IInArchive, IArchiveOpenSeq)
			INTERFACE_IInArchive(; )
			STDMETHOD(OpenSeq) (ISequentialInStream *stream);
		};

		static const Byte kProps[] = { kpidPath, kpidMTime, kpidAttrib, kpidMethod };
		static const Byte kArcProps[] = { kpidMethod };

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		void CHandler::GetVersion(NCOM::CPropVariant &prop)
		{
			AString s("PPMd");
			s += (char)('A' + _item.Ver);
			s += ":o";
			s.Add_UInt32(_item.Order);
			s += ":mem";
			s.Add_UInt32(_item.MemInMB);
			s += 'm';
			if(_item.Ver >= kNewHeaderVer && _item.Restor != 0) {
				s += ":r";
				s.Add_UInt32(_item.Restor);
			}
			prop = s;
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPhySize: if(_packSize_Defined) prop = _packSize; break;
				case kpidMethod: GetVersion(prop); break;
			}
			prop.Detach(value);
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = 1;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetProperty(uint32 /* index */, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPath: prop = MultiByteToUnicodeString(_item.Name, CP_ACP); break;
				case kpidMTime:
				{
					// time can be in Unix format ???
					FILETIME utc;
					if(NTime::DosTimeToFileTime(_item.Time, utc))
						prop = utc;
					break;
				}
				case kpidAttrib: prop = _item.Attrib; break;
				case kpidPackSize: if(_packSize_Defined) prop = _packSize; break;
				case kpidMethod: GetVersion(prop); break;
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 *, IArchiveOpenCallback *)
		{
			return OpenSeq(stream);
		}

		STDMETHODIMP CHandler::OpenSeq(ISequentialInStream * stream)
		{
			COM_TRY_BEGIN
			HRESULT res;
			try {
				Close();
				res = _item.ReadHeader(stream, _headerSize);
			}
			catch(...) { res = S_FALSE; }
			if(res == S_OK)
				_stream = stream;
			else
				Close();
			return res;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			_packSize = 0;
			_packSize_Defined = false;
			_stream.Release();
			return S_OK;
		}

		static const uint32 kTopValue = (1 << 24);
		static const uint32 kBot = (1 << 15);

		struct CRangeDecoder {
			IPpmd7_RangeDec vt;
			uint32 Range;
			uint32 Code;
			uint32 Low;
			CByteInBufWrap * Stream;
		public:
			bool Init()
			{
				Code = 0;
				Low = 0;
				Range = 0xFFFFFFFF;
				for(int i = 0; i < 4; i++)
					Code = (Code << 8) | Stream->ReadByte();
				return Code < 0xFFFFFFFF;
			}
			void Normalize()
			{
				while((Low ^ (Low + Range)) < kTopValue || Range < kBot && ((Range = (0 - Low) & (kBot - 1)), 1)) {
					Code = (Code << 8) | Stream->ReadByte();
					Range <<= 8;
					Low <<= 8;
				}
			}

			CRangeDecoder();
		};

		extern "C" {
			#define GET_RangeDecoder CRangeDecoder *p = CONTAINER_FROM_VTBL(pp, CRangeDecoder, vt);

			static uint32 Range_GetThreshold(const IPpmd7_RangeDec * pp, uint32 total)
			{
				GET_RangeDecoder
				return p->Code / (p->Range /= total);
			}
			static void Range_Decode(const IPpmd7_RangeDec * pp, uint32 start, uint32 size)
			{
				GET_RangeDecoder
					start *= p->Range;
				p->Low += start;
				p->Code -= start;
				p->Range *= size;
				p->Normalize();
			}
			static uint32 Range_DecodeBit(const IPpmd7_RangeDec * pp, uint32 size0)
			{
				GET_RangeDecoder
				if(p->Code / (p->Range >>= 14) < size0) {
					Range_Decode(&p->vt, 0, size0);
					return 0;
				}
				else {
					Range_Decode(&p->vt, size0, (1 << 14) - size0);
					return 1;
				}
			}
		}

		CRangeDecoder::CRangeDecoder()
		{
			vt.GetThreshold = Range_GetThreshold;
			vt.Decode = Range_Decode;
			vt.DecodeBit = Range_DecodeBit;
		}

		struct CPpmdCpp {
			uint   Ver;
			CRangeDecoder _rc;
			CPpmd7 _ppmd7;
			CPpmd8 _ppmd8;

			CPpmdCpp(unsigned version)
			{
				Ver = version;
				Ppmd7_Construct(&_ppmd7);
				Ppmd8_Construct(&_ppmd8);
			}
			~CPpmdCpp()
			{
				Ppmd7_Free(&_ppmd7, &g_BigAlloc);
				Ppmd8_Free(&_ppmd8, &g_BigAlloc);
			}
			bool Alloc(uint32 memInMB)
			{
				memInMB <<= 20;
				return (Ver == 7) ? (Ppmd7_Alloc(&_ppmd7, memInMB, &g_BigAlloc) != 0) : (Ppmd8_Alloc(&_ppmd8, memInMB, &g_BigAlloc) != 0);
			}
			void Init(unsigned order, unsigned restor)
			{
				if(Ver == 7)
					Ppmd7_Init(&_ppmd7, order);
				else
					Ppmd8_Init(&_ppmd8, order, restor); ;
			}
			bool InitRc(CByteInBufWrap * inStream)
			{
				if(Ver == 7) {
					_rc.Stream = inStream;
					return _rc.Init();
				}
				else {
					_ppmd8.Stream.In = &inStream->vt;
					return Ppmd8_RangeDec_Init(&_ppmd8) != 0;
				}
			}
			bool IsFinishedOK()
			{
				return (Ver == 7) ? Ppmd7z_RangeDec_IsFinishedOK(&_rc) : Ppmd8_RangeDec_IsFinishedOK(&_ppmd8);
			}
		};

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
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
						Buf = (Byte *)::MidAlloc(kBufSize);
					return (Buf != 0);
				}
				Byte * Buf;
			};
			//
			if(numItems == 0)
				return S_OK;
			if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
				return E_INVALIDARG;
			// extractCallback->SetTotal(_packSize);
			uint64 currentTotalPacked = 0;
			RINOK(extractCallback->SetCompleted(&currentTotalPacked));
			CMyComPtr<ISequentialOutStream> realOutStream;
			int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
			RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
			if(!testMode && !realOutStream)
				return S_OK;
			extractCallback->PrepareOperation(askMode);
			CByteInBufWrap inBuf;
			if(!inBuf.Alloc(1 << 20))
				return E_OUTOFMEMORY;
			inBuf.Stream = _stream;
			CBuf outBuf;
			if(!outBuf.Alloc())
				return E_OUTOFMEMORY;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, true);
			CPpmdCpp ppmd(_item.Ver);
			if(!ppmd.Alloc(_item.MemInMB))
				return E_OUTOFMEMORY;
			int32 opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
			if(_item.IsSupported()) {
				opRes = NExtractArc::NOperationResult::kDataError;
				ppmd.Init(_item.Order, _item.Restor);
				inBuf.Init();
				uint64 outSize = 0;
				if(ppmd.InitRc(&inBuf) && !inBuf.Extra && inBuf.Res == S_OK) {
					for(;;) {
						lps->InSize = _packSize = inBuf.GetProcessed();
						lps->OutSize = outSize;
						RINOK(lps->SetCur());
						size_t i;
						int sym = 0;
						if(ppmd.Ver == 7) {
							for(i = 0; i < kBufSize; i++) {
								sym = Ppmd7_DecodeSymbol(&ppmd._ppmd7, &ppmd._rc.vt);
								if(inBuf.Extra || sym < 0)
									break;
								outBuf.Buf[i] = (Byte)sym;
							}
						}
						else {
							for(i = 0; i < kBufSize; i++) {
								sym = Ppmd8_DecodeSymbol(&ppmd._ppmd8);
								if(inBuf.Extra || sym < 0)
									break;
								outBuf.Buf[i] = (Byte)sym;
							}
						}
						outSize += i;
						_packSize = _headerSize + inBuf.GetProcessed();
						_packSize_Defined = true;
						if(realOutStream) {
							RINOK(WriteStream(realOutStream, outBuf.Buf, i));
						}
						if(inBuf.Extra) {
							opRes = NExtractArc::NOperationResult::kUnexpectedEnd;
							break;
						}
						if(sym < 0) {
							if(sym == -1 && ppmd.IsFinishedOK())
								opRes = NExtractArc::NOperationResult::kOK;
							break;
						}
					}
				}
				RINOK(inBuf.Res);
			}
			realOutStream.Release();
			return extractCallback->SetOperationResult(opRes);
		}

		static const Byte k_Signature[] = { 0x8F, 0xAF, 0xAC, 0x84 };

		REGISTER_ARC_I("Ppmd", "pmd", 0, 0xD, k_Signature, 0, 0, NULL)
	}
}
