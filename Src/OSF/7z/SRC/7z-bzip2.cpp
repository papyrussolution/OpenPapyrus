// 7Z-BZIP2.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

#define TICKS_START
#define TICKS_UPDATE(n)
//#define PRIN(s) printf(s "\n"); fflush(stdout);
//#define PRIN_VAL(s, val) printf(s " = %u \n", val); fflush(stdout);
#define PRIN(s)
#define PRIN_VAL(s, val)
#define PRIN_MT(s) PRIN("    " s)

// BZip2Crc.cpp
uint32 CBZip2Crc::Table[256];

static const uint32 kBZip2CrcPoly = 0x04c11db7; // AUTODIN II, Ethernet, & FDDI

void CBZip2Crc::InitTable()
{
	for(uint32 i = 0; i < 256; i++) {
		uint32 r = (i << 24);
		for(uint j = 0; j < 8; j++)
			r = (r << 1) ^ (kBZip2CrcPoly & ((uint32)0 - (r >> 31)));
		Table[i] = r;
	}
}

class CBZip2CrcTableInit { 
public: 
	CBZip2CrcTableInit() { CBZip2Crc::InitTable(); } 
} g_BZip2CrcTableInit;
//
// BZip2Encoder.cpp BZip2Decoder.cpp
namespace NCompress {
	namespace NBZip2 {
		//
		// BZip2Encoder
		//
		class CMsbfEncoderTemp {
		public:
			void   SetStream(Byte * buf);
			Byte * GetStream() const;
			void   Init();
			void   Flush();
			void   WriteBits(uint32 value, unsigned numBits);
			uint32 GetBytePos() const;
			uint32 GetPos() const;
			Byte   GetCurByte() const;
			void   SetPos(uint32 bitPos);
			void   SetCurState(unsigned bitPos, Byte curByte);
		private:
			uint32 _pos;
			uint   _bitPos;
			Byte   _curByte;
			Byte * _buf;
		};

		void CMsbfEncoderTemp::Init()
		{
			_pos = 0;
			_bitPos = 8;
			_curByte = 0;
		}
		void CMsbfEncoderTemp::Flush()
		{
			if(_bitPos < 8)
				WriteBits(0, _bitPos);
		}
		void CMsbfEncoderTemp::WriteBits(uint32 value, unsigned numBits)
		{
			while(numBits > 0) {
				uint   numNewBits = MyMin(numBits, _bitPos);
				numBits -= numNewBits;
				_curByte <<= numNewBits;
				uint32 newBits = value >> numBits;
				_curByte |= Byte(newBits);
				value -= (newBits << numBits);
				_bitPos -= numNewBits;
				if(_bitPos == 0) {
					_buf[_pos++] = _curByte;
					_bitPos = 8;
				}
			}
		}

		void CMsbfEncoderTemp::SetStream(Byte * buf) { _buf = buf; }
		Byte * CMsbfEncoderTemp::GetStream() const  { return _buf; }
		uint32 CMsbfEncoderTemp::GetBytePos() const { return _pos; }
		uint32 CMsbfEncoderTemp::GetPos() const { return _pos * 8 + (8 - _bitPos); }
		Byte CMsbfEncoderTemp::GetCurByte() const { return _curByte; }

		void CMsbfEncoderTemp::SetPos(uint32 bitPos)
		{
			_pos = bitPos >> 3;
			_bitPos = 8 - ((uint)bitPos & 7);
		}

		void CMsbfEncoderTemp::SetCurState(unsigned bitPos, Byte curByte)
		{
			_bitPos = 8 - bitPos;
			_curByte = curByte;
		}

		const uint kMaxHuffmanLenForEncoding = 16; // it must be < kMaxHuffmanLen = 20

		static const uint32 kBufferSize = (1 << 17);
		static const uint kNumHuffPasses = 4;

		bool CThreadInfo::Alloc()
		{
			if(m_BlockSorterIndex == 0) {
				m_BlockSorterIndex = static_cast<uint32 *>(::BigAlloc(BLOCK_SORT_BUF_SIZE(kBlockSizeMax) * sizeof(uint32)));
				if(m_BlockSorterIndex == 0)
					return false;
			}

			if(m_Block == 0) {
				m_Block = static_cast<Byte *>(::MidAlloc(kBlockSizeMax * 5 + kBlockSizeMax / 10 + (20 << 10)));
				if(m_Block == 0)
					return false;
				m_MtfArray = m_Block + kBlockSizeMax;
				m_TempArray = m_MtfArray + kBlockSizeMax * 2 + 2;
			}
			return true;
		}

		void CThreadInfo::Free()
		{
			::BigFree(m_BlockSorterIndex);
			m_BlockSorterIndex = 0;
			::MidFree(m_Block);
			m_Block = 0;
		}

#ifndef _7ZIP_ST
			static THREAD_FUNC_DECL MFThread(void * threadCoderInfo)
			{
				return ((CThreadInfo*)threadCoderInfo)->ThreadFunc();
			}

			#define RINOK_THREAD(x) { WRes __result_ = (x); if(__result_ != 0) return __result_; }

			HRESULT CThreadInfo::Create()
			{
				RINOK_THREAD(StreamWasFinishedEvent.Create());
				RINOK_THREAD(WaitingWasStartedEvent.Create());
				RINOK_THREAD(CanWriteEvent.Create());
				RINOK_THREAD(Thread.Create(MFThread, this));
				return S_OK;
			}

			void CThreadInfo::FinishStream(bool needLeave)
			{
				Encoder->StreamWasFinished = true;
				StreamWasFinishedEvent.Set();
				if(needLeave)
					Encoder->CS.Leave();
				Encoder->CanStartWaitingEvent.Lock();
				WaitingWasStartedEvent.Set();
			}

			DWORD CThreadInfo::ThreadFunc()
			{
				for(;;) {
					Encoder->CanProcessEvent.Lock();
					Encoder->CS.Enter();
					if(Encoder->CloseThreads) {
						Encoder->CS.Leave();
						return 0;
					}
					if(Encoder->StreamWasFinished) {
						FinishStream(true);
						continue;
					}
					HRESULT res = S_OK;
					bool needLeave = true;
					try {
						uint32 blockSize = Encoder->ReadRleBlock(m_Block);
						m_PackSize = Encoder->m_InStream.GetProcessedSize();
						m_BlockIndex = Encoder->NextBlockIndex;
						if(++Encoder->NextBlockIndex == Encoder->NumThreads)
							Encoder->NextBlockIndex = 0;
						if(blockSize == 0) {
							FinishStream(true);
							continue;
						}
						Encoder->CS.Leave();
						needLeave = false;
						res = EncodeBlock3(blockSize);
					}
					catch(const CInBufferException &e) { res = e.ErrorCode; }
					catch(const COutBufferException &e) { res = e.ErrorCode; }
					catch(...) { res = E_FAIL; }
					if(res != S_OK) {
						Encoder->Result = res;
						FinishStream(needLeave);
						continue;
					}
				}
			}
#endif
		CEncProps::CEncProps() : BlockSizeMult((uint32)(int32)-1), NumPasses((uint32)(int32)-1)
		{
		}

		void CEncProps::Normalize(int level)
		{
			if(level < 0) level = 5;
			if(level > 9) level = 9;
			if(NumPasses == (uint32)(int32)-1)
				NumPasses = (level >= 9 ? 7 : (level >= 7 ? 2 : 1));
			if(NumPasses < 1) NumPasses = 1;
			if(NumPasses > kNumPassesMax) NumPasses = kNumPassesMax;
			if(BlockSizeMult == (uint32)(int32)-1)
				BlockSizeMult = (level >= 5 ? 9 : (level >= 1 ? level * 2 - 1 : 1));
			if(BlockSizeMult < kBlockSizeMultMin) BlockSizeMult = kBlockSizeMultMin;
			if(BlockSizeMult > kBlockSizeMultMax) BlockSizeMult = kBlockSizeMultMax;
		}

		bool   FASTCALL CEncoder::CBitmEncoder::Create(uint32 bufferSize) { return _stream.Create(bufferSize); }
		void   FASTCALL CEncoder::CBitmEncoder::SetStream(ISequentialOutStream * outStream) { _stream.SetStream(outStream); }
		uint64 CEncoder::CBitmEncoder::GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
		void   CEncoder::CBitmEncoder::Init()
		{
			_stream.Init();
			_bitPos = 8;
			_curByte = 0;
		}
		HRESULT CEncoder::CBitmEncoder::Flush()
		{
			if(_bitPos < 8)
				WriteBits(0, _bitPos);
			return _stream.Flush();
		}
		void CEncoder::CBitmEncoder::WriteBits(uint32 value, uint numBits)
		{
			while(numBits > 0) {
				if(numBits < _bitPos) {
					_curByte |= ((Byte)value << (_bitPos -= numBits));
					return;
				}
				numBits -= _bitPos;
				uint32 newBits = (value >> numBits);
				value -= (newBits << numBits);
				_stream.WriteByte((Byte)(_curByte | newBits));
				_bitPos = 8;
				_curByte = 0;
			}
		}

		CEncoder::CEncoder()
		{
			_props.Normalize(-1);
		  #ifndef _7ZIP_ST
			ThreadsInfo = 0;
			m_NumThreadsPrev = 0;
			NumThreads = 1;
		  #endif
		}

		#ifndef _7ZIP_ST
		CEncoder::~CEncoder()
		{
			Free();
		}

		HRESULT CEncoder::Create()
		{
			RINOK_THREAD(CanProcessEvent.CreateIfNotCreated());
			RINOK_THREAD(CanStartWaitingEvent.CreateIfNotCreated());
			if(ThreadsInfo != 0 && m_NumThreadsPrev == NumThreads)
				return S_OK;
			try {
				Free();
				MtMode = (NumThreads > 1);
				m_NumThreadsPrev = NumThreads;
				ThreadsInfo = new CThreadInfo[NumThreads];
				if(ThreadsInfo == 0)
					return E_OUTOFMEMORY;
			}
			catch(...) { return E_OUTOFMEMORY; }
			for(uint32 t = 0; t < NumThreads; t++) {
				CThreadInfo &ti = ThreadsInfo[t];
				ti.Encoder = this;
				if(MtMode) {
					HRESULT res = ti.Create();
					if(res != S_OK) {
						NumThreads = t;
						Free();
						return res;
					}
				}
			}
			return S_OK;
		}

		void CEncoder::Free()
		{
			if(ThreadsInfo) {
				CloseThreads = true;
				CanProcessEvent.Set();
				for(uint32 t = 0; t < NumThreads; t++) {
					CThreadInfo &ti = ThreadsInfo[t];
					if(MtMode)
						ti.Thread.Wait();
					ti.Free();
				}
				delete []ThreadsInfo;
				ThreadsInfo = 0;
			}
		}

		#endif

		uint32 CEncoder::ReadRleBlock(Byte * buffer)
		{
			uint32 i = 0;
			Byte prevByte;
			if(m_InStream.ReadByte(prevByte)) {
				uint32 blockSize = _props.BlockSizeMult * kBlockSizeStep - 1;
				unsigned numReps = 1;
				buffer[i++] = prevByte;
				while(i < blockSize) { // "- 1" to support RLE
					Byte b;
					if(!m_InStream.ReadByte(b))
						break;
					if(b != prevByte) {
						if(numReps >= kRleModeRepSize)
							buffer[i++] = (Byte)(numReps - kRleModeRepSize);
						buffer[i++] = b;
						numReps = 1;
						prevByte = b;
						continue;
					}
					numReps++;
					if(numReps <= kRleModeRepSize)
						buffer[i++] = b;
					else if(numReps == kRleModeRepSize + 255) {
						buffer[i++] = (Byte)(numReps - kRleModeRepSize);
						numReps = 0;
					}
				}
				// it's to support original BZip2 decoder
				if(numReps >= kRleModeRepSize)
					buffer[i++] = (Byte)(numReps - kRleModeRepSize);
			}
			return i;
		}

		void FASTCALL CThreadInfo::WriteBits2(uint32 value, uint numBits) { m_OutStreamCurrent->WriteBits(value, numBits); }
		void FASTCALL CThreadInfo::WriteByte2(Byte b) { WriteBits2(b, 8); }
		void FASTCALL CThreadInfo::WriteBit2(Byte v) { WriteBits2(v, 1); }

		void FASTCALL CThreadInfo::WriteCrc2(uint32 v)
		{
			for(uint i = 0; i < 4; i++)
				WriteByte2(((Byte)(v >> (24 - i * 8))));
		}

		void CEncoder::WriteBits(uint32 value, unsigned numBits) { m_OutStream.WriteBits(value, numBits); }
		void CEncoder::WriteByte(Byte b) { WriteBits(b, 8); }

		// void CEncoder::WriteBit(Byte v) { WriteBits(v, 1); }
		void CEncoder::WriteCrc(uint32 v)
		{
			for(uint i = 0; i < 4; i++)
				WriteByte(((Byte)(v >> (24 - i * 8))));
		}

		// blockSize > 0
		void CThreadInfo::EncodeBlock(const Byte * block, uint32 blockSize)
		{
			WriteBit2(0); // Randomised = false
			{
				uint32 origPtr = BlockSort(m_BlockSorterIndex, block, blockSize);
				// if(m_BlockSorterIndex[origPtr] != 0) throw 1;
				m_BlockSorterIndex[origPtr] = blockSize;
				WriteBits2(origPtr, kNumOrigBits);
			}
			CMtf8Encoder mtf;
			uint   numInUse = 0;
			{
				Byte inUse[256];
				Byte inUse16[16];
				uint32 i;
				for(i = 0; i < 256; i++)
					inUse[i] = 0;
				for(i = 0; i < 16; i++)
					inUse16[i] = 0;
				for(i = 0; i < blockSize; i++)
					inUse[block[i]] = 1;
				for(i = 0; i < 256; i++)
					if(inUse[i]) {
						inUse16[i >> 4] = 1;
						mtf.Buf[numInUse++] = (Byte)i;
					}
				for(i = 0; i < 16; i++)
					WriteBit2(inUse16[i]);
				for(i = 0; i < 256; i++)
					if(inUse16[i >> 4])
						WriteBit2(inUse[i]);
			}
			uint   alphaSize = numInUse + 2;
			Byte * mtfs = m_MtfArray;
			uint32 mtfArraySize = 0;
			uint32 symbolCounts[kMaxAlphaSize];
			{
				for(uint i = 0; i < kMaxAlphaSize; i++)
					symbolCounts[i] = 0;
			}
			{
				uint32 rleSize = 0;
				uint32 i = 0;
				const uint32 * bsIndex = m_BlockSorterIndex;
				block--;
				do {
					uint   pos = mtf.FindAndMove(block[bsIndex[i]]);
					if(pos == 0)
						rleSize++;
					else {
						while(rleSize != 0) {
							rleSize--;
							mtfs[mtfArraySize++] = (Byte)(rleSize & 1);
							symbolCounts[rleSize & 1]++;
							rleSize >>= 1;
						}
						if(pos >= 0xFE) {
							mtfs[mtfArraySize++] = 0xFF;
							mtfs[mtfArraySize++] = (Byte)(pos - 0xFE);
						}
						else
							mtfs[mtfArraySize++] = (Byte)(pos + 1);
						symbolCounts[(size_t)pos + 1]++;
					}
				} while(++i < blockSize);
				while(rleSize != 0) {
					rleSize--;
					mtfs[mtfArraySize++] = (Byte)(rleSize & 1);
					symbolCounts[rleSize & 1]++;
					rleSize >>= 1;
				}
				if(alphaSize < 256)
					mtfs[mtfArraySize++] = (Byte)(alphaSize - 1);
				else {
					mtfs[mtfArraySize++] = 0xFF;
					mtfs[mtfArraySize++] = (Byte)(alphaSize - 256);
				}
				symbolCounts[(size_t)alphaSize - 1]++;
			}
			uint32 numSymbols = 0;
			{
				for(uint i = 0; i < kMaxAlphaSize; i++)
					numSymbols += symbolCounts[i];
			}
			uint   bestNumTables = kNumTablesMin;
			uint32 bestPrice = 0xFFFFFFFF;
			uint32 startPos = m_OutStreamCurrent->GetPos();
			Byte startCurByte = m_OutStreamCurrent->GetCurByte();
			for(uint nt = kNumTablesMin; nt <= kNumTablesMax + 1; nt++) {
				uint   numTables;
				if(m_OptimizeNumTables) {
					m_OutStreamCurrent->SetPos(startPos);
					m_OutStreamCurrent->SetCurState((startPos & 7), startCurByte);
					if(nt <= kNumTablesMax)
						numTables = nt;
					else
						numTables = bestNumTables;
				}
				else {
					if(numSymbols < 200) numTables = 2;
					else if(numSymbols < 600) numTables = 3;
					else if(numSymbols < 1200) numTables = 4;
					else if(numSymbols < 2400) numTables = 5;
					else numTables = 6;
				}
				WriteBits2(numTables, kNumTablesBits);
				uint32 numSelectors = (numSymbols + kGroupSize - 1) / kGroupSize;
				WriteBits2(numSelectors, kNumSelectorsBits);
				{
					uint32 remFreq = numSymbols;
					uint   gs = 0;
					uint   t = numTables;
					do {
						uint32 tFreq = remFreq / t;
						uint   ge = gs;
						uint32 aFreq = 0;
						while(aFreq < tFreq) //  && ge < alphaSize)
							aFreq += symbolCounts[ge++];
						if(ge > gs + 1 && t != numTables && t != 1 && (((numTables - t) & 1) == 1))
							aFreq -= symbolCounts[--ge];
						Byte * lens = Lens[(size_t)t - 1];
						uint   i = 0;
						do {
							lens[i] = (Byte)((i >= gs && i < ge) ? 0 : 1);
						} while(++i < alphaSize);
						gs = ge;
						remFreq -= aFreq;
					} while(--t != 0);
				}
				for(uint pass = 0; pass < kNumHuffPasses; pass++) {
					{
						uint   t = 0;
						do {
							memzero(Freqs[t], sizeof(Freqs[t]));
						} while(++t < numTables);
					}
					{
						uint32 mtfPos = 0;
						uint32 g = 0;
						do {
							uint32 symbols[kGroupSize];
							uint   i = 0;
							do {
								uint32 symbol = mtfs[mtfPos++];
								if(symbol >= 0xFF)
									symbol += mtfs[mtfPos++];
								symbols[i] = symbol;
							} while(++i < kGroupSize && mtfPos < mtfArraySize);
							uint32 bestPrice2 = 0xFFFFFFFF;
							uint   t = 0;
							do {
								const Byte * lens = Lens[t];
								uint32 price = 0;
								uint   j = 0;
								do {
									price += lens[symbols[j]];
								} while(++j < i);
								if(price < bestPrice2) {
									m_Selectors[g] = (Byte)t;
									bestPrice2 = price;
								}
							} while(++t < numTables);
							uint32 * freqs = Freqs[m_Selectors[g++]];
							uint   j = 0;
							do {
								freqs[symbols[j]]++;
							} while(++j < i);
						} while(mtfPos < mtfArraySize);
					}
					uint   t = 0;
					do {
						uint32 * freqs = Freqs[t];
						uint   i = 0;
						do {
							if(freqs[i] == 0)
								freqs[i] = 1;
						} while(++i < alphaSize);
						Huffman_Generate(freqs, Codes[t], Lens[t], kMaxAlphaSize, kMaxHuffmanLenForEncoding);
					} while(++t < numTables);
				}
				{
					Byte mtfSel[kNumTablesMax];
					{
						uint   t = 0;
						do {
							mtfSel[t] = (Byte)t;
						} while(++t < numTables);
					}
					uint32 i = 0;
					do {
						Byte sel = m_Selectors[i];
						uint   pos;
						for(pos = 0; mtfSel[pos] != sel; pos++)
							WriteBit2(1);
						WriteBit2(0);
						for(; pos > 0; pos--)
							mtfSel[pos] = mtfSel[(size_t)pos - 1];
						mtfSel[0] = sel;
					} while(++i < numSelectors);
				}
				{
					uint   t = 0;
					do {
						const Byte * lens = Lens[t];
						uint32 len = lens[0];
						WriteBits2(len, kNumLevelsBits);
						uint   i = 0;
						do {
							uint32 level = lens[i];
							while(len != level) {
								WriteBit2(1);
								if(len < level) {
									WriteBit2(0);
									len++;
								}
								else {
									WriteBit2(1);
									len--;
								}
							}
							WriteBit2(0);
						} while(++i < alphaSize);
					} while(++t < numTables);
				}
				{
					uint32 groupSize = 0;
					uint32 groupIndex = 0;
					const Byte * lens = 0;
					const uint32 * codes = 0;
					uint32 mtfPos = 0;
					do {
						uint32 symbol = mtfs[mtfPos++];
						if(symbol >= 0xFF)
							symbol += mtfs[mtfPos++];
						if(groupSize == 0) {
							groupSize = kGroupSize;
							uint   t = m_Selectors[groupIndex++];
							lens = Lens[t];
							codes = Codes[t];
						}
						groupSize--;
						m_OutStreamCurrent->WriteBits(codes[symbol], lens[symbol]);
					} while(mtfPos < mtfArraySize);
				}
				if(!m_OptimizeNumTables)
					break;
				uint32 price = m_OutStreamCurrent->GetPos() - startPos;
				if(price <= bestPrice) {
					if(nt == kNumTablesMax)
						break;
					bestPrice = price;
					bestNumTables = nt;
				}
			}
		}

		// blockSize > 0
		uint32 CThreadInfo::EncodeBlockWithHeaders(const Byte * block, uint32 blockSize)
		{
			WriteByte2(kBlockSig0);
			WriteByte2(kBlockSig1);
			WriteByte2(kBlockSig2);
			WriteByte2(kBlockSig3);
			WriteByte2(kBlockSig4);
			WriteByte2(kBlockSig5);
			CBZip2Crc crc;
			unsigned numReps = 0;
			Byte prevByte = block[0];
			uint32 i = 0;
			do {
				Byte b = block[i];
				if(numReps == kRleModeRepSize) {
					for(; b > 0; b--)
						crc.UpdateByte(prevByte);
					numReps = 0;
					continue;
				}
				if(prevByte == b)
					numReps++;
				else {
					numReps = 1;
					prevByte = b;
				}
				crc.UpdateByte(b);
			} while(++i < blockSize);
			uint32 crcRes = crc.GetDigest();
			WriteCrc2(crcRes);
			EncodeBlock(block, blockSize);
			return crcRes;
		}

		void CThreadInfo::EncodeBlock2(const Byte * block, uint32 blockSize, uint32 numPasses)
		{
			uint32 numCrcs = m_NumCrcs;
			bool needCompare = false;

			uint32 startBytePos = m_OutStreamCurrent->GetBytePos();
			uint32 startPos = m_OutStreamCurrent->GetPos();
			Byte startCurByte = m_OutStreamCurrent->GetCurByte();
			Byte endCurByte = 0;
			uint32 endPos = 0;
			if(numPasses > 1 && blockSize >= (1 << 10)) {
				uint32 blockSize0 = blockSize / 2; // ????
				for(; (block[blockSize0] == block[(size_t)blockSize0 - 1] || block[(size_t)blockSize0 - 1] == block[(size_t)blockSize0 - 2]) && blockSize0 < blockSize; blockSize0++) 
					;
				if(blockSize0 < blockSize) {
					EncodeBlock2(block, blockSize0, numPasses - 1);
					EncodeBlock2(block + blockSize0, blockSize - blockSize0, numPasses - 1);
					endPos = m_OutStreamCurrent->GetPos();
					endCurByte = m_OutStreamCurrent->GetCurByte();
					if((endPos & 7) > 0)
						WriteBits2(0, 8 - (endPos & 7));
					m_OutStreamCurrent->SetCurState((startPos & 7), startCurByte);
					needCompare = true;
				}
			}
			uint32 startBytePos2 = m_OutStreamCurrent->GetBytePos();
			uint32 startPos2 = m_OutStreamCurrent->GetPos();
			uint32 crcVal = EncodeBlockWithHeaders(block, blockSize);
			uint32 endPos2 = m_OutStreamCurrent->GetPos();
			if(needCompare) {
				uint32 size2 = endPos2 - startPos2;
				if(size2 < endPos - startPos) {
					uint32 numBytes = m_OutStreamCurrent->GetBytePos() - startBytePos2;
					Byte * buffer = m_OutStreamCurrent->GetStream();
					for(uint32 i = 0; i < numBytes; i++)
						buffer[startBytePos + i] = buffer[startBytePos2 + i];
					m_OutStreamCurrent->SetPos(startPos + endPos2 - startPos2);
					m_NumCrcs = numCrcs;
					m_CRCs[m_NumCrcs++] = crcVal;
				}
				else {
					m_OutStreamCurrent->SetPos(endPos);
					m_OutStreamCurrent->SetCurState((endPos & 7), endCurByte);
				}
			}
			else {
				m_NumCrcs = numCrcs;
				m_CRCs[m_NumCrcs++] = crcVal;
			}
		}

		CThreadInfo::CThreadInfo() : m_BlockSorterIndex(0), m_Block(0) 
		{
		}

		CThreadInfo::~CThreadInfo() 
		{
			Free();
		}

		HRESULT CThreadInfo::EncodeBlock3(uint32 blockSize)
		{
			CMsbfEncoderTemp outStreamTemp;
			outStreamTemp.SetStream(m_TempArray);
			outStreamTemp.Init();
			m_OutStreamCurrent = &outStreamTemp;
			m_NumCrcs = 0;
			EncodeBlock2(m_Block, blockSize, Encoder->_props.NumPasses);
		  #ifndef _7ZIP_ST
			if(Encoder->MtMode)
				Encoder->ThreadsInfo[m_BlockIndex].CanWriteEvent.Lock();
		  #endif
			for(uint32 i = 0; i < m_NumCrcs; i++)
				Encoder->CombinedCrc.Update(m_CRCs[i]);
			Encoder->WriteBytes(m_TempArray, outStreamTemp.GetPos(), outStreamTemp.GetCurByte());
			HRESULT res = S_OK;
		  #ifndef _7ZIP_ST
			if(Encoder->MtMode) {
				uint32 blockIndex = m_BlockIndex + 1;
				if(blockIndex == Encoder->NumThreads)
					blockIndex = 0;
				if(Encoder->Progress) {
					uint64 unpackSize = Encoder->m_OutStream.GetProcessedSize();
					res = Encoder->Progress->SetRatioInfo(&m_PackSize, &unpackSize);
				}
				Encoder->ThreadsInfo[blockIndex].CanWriteEvent.Set();
			}
		  #endif
			return res;
		}

		void CEncoder::WriteBytes(const Byte * data, uint32 sizeInBits, Byte lastByte)
		{
			uint32 bytesSize = (sizeInBits >> 3);
			for(uint32 i = 0; i < bytesSize; i++)
				m_OutStream.WriteBits(data[i], 8);
			WriteBits(lastByte, (sizeInBits & 7));
		}

		HRESULT CEncoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
		  #ifndef _7ZIP_ST
			Progress = progress;
			RINOK(Create());
			for(uint32 t = 0; t < NumThreads; t++)
		  #endif
			{
			#ifndef _7ZIP_ST
				CThreadInfo &ti = ThreadsInfo[t];
				if(MtMode) {
					RINOK(ti.StreamWasFinishedEvent.Reset());
					RINOK(ti.WaitingWasStartedEvent.Reset());
					RINOK(ti.CanWriteEvent.Reset());
				}
			#else
				CThreadInfo &ti = ThreadsInfo;
				ti.Encoder = this;
			#endif
				ti.m_OptimizeNumTables = _props.DoOptimizeNumTables();
				if(!ti.Alloc())
					return E_OUTOFMEMORY;
			}
			if(!m_InStream.Create(kBufferSize))
				return E_OUTOFMEMORY;
			if(!m_OutStream.Create(kBufferSize))
				return E_OUTOFMEMORY;
			m_InStream.SetStream(inStream);
			m_InStream.Init();
			m_OutStream.SetStream(outStream);
			m_OutStream.Init();
			CombinedCrc.Init();
		  #ifndef _7ZIP_ST
			NextBlockIndex = 0;
			StreamWasFinished = false;
			CloseThreads = false;
			CanStartWaitingEvent.Reset();
		  #endif
			WriteByte(kArSig0);
			WriteByte(kArSig1);
			WriteByte(kArSig2);
			WriteByte((Byte)(kArSig3 + _props.BlockSizeMult));
		  #ifndef _7ZIP_ST
			if(MtMode) {
				ThreadsInfo[0].CanWriteEvent.Set();
				Result = S_OK;
				CanProcessEvent.Set();
				uint32 t;
				for(t = 0; t < NumThreads; t++)
					ThreadsInfo[t].StreamWasFinishedEvent.Lock();
				CanProcessEvent.Reset();
				CanStartWaitingEvent.Set();
				for(t = 0; t < NumThreads; t++)
					ThreadsInfo[t].WaitingWasStartedEvent.Lock();
				CanStartWaitingEvent.Reset();
				RINOK(Result);
			}
			else
		  #endif
			{
				for(;;) {
			  #ifndef _7ZIP_ST
					CThreadInfo & ti = ThreadsInfo[0];
			  #else
					CThreadInfo & ti = ThreadsInfo;
			  #endif
					uint32 blockSize = ReadRleBlock(ti.m_Block);
					if(blockSize == 0)
						break;
					RINOK(ti.EncodeBlock3(blockSize));
					if(progress) {
						uint64 packSize = m_InStream.GetProcessedSize();
						uint64 unpackSize = m_OutStream.GetProcessedSize();
						RINOK(progress->SetRatioInfo(&packSize, &unpackSize));
					}
				}
			}
			WriteByte(kFinSig0);
			WriteByte(kFinSig1);
			WriteByte(kFinSig2);
			WriteByte(kFinSig3);
			WriteByte(kFinSig4);
			WriteByte(kFinSig5);
			WriteCrc(CombinedCrc.GetDigest());
			return Flush();
		}

		STDMETHODIMP CEncoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream, const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(const COutBufferException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}

		HRESULT CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
		{
			int level = -1;
			CEncProps props;
			for(uint32 i = 0; i < numProps; i++) {
				const PROPVARIANT &prop = coderProps[i];
				PROPID propID = propIDs[i];
				if(propID >= NCoderPropID::kReduceSize)
					continue;
				if(prop.vt != VT_UI4)
					return E_INVALIDARG;
				uint32 v = (uint32)prop.ulVal;
				switch(propID) {
					case NCoderPropID::kNumPasses: props.NumPasses = v; break;
					case NCoderPropID::kDictionarySize: props.BlockSizeMult = v / kBlockSizeStep; break;
					case NCoderPropID::kLevel: level = v; break;
					case NCoderPropID::kNumThreads:
					{
			#ifndef _7ZIP_ST
						SetNumberOfThreads(v);
			#endif
						break;
					}
					default: return E_INVALIDARG;
				}
			}
			props.Normalize(level);
			_props = props;
			return S_OK;
		}

		#ifndef _7ZIP_ST
			STDMETHODIMP CEncoder::SetNumberOfThreads(uint32 numThreads)
			{
				const uint32 kNumThreadsMax = 64;
				SETMAX(numThreads, 1);
				SETMIN(numThreads, kNumThreadsMax);
				NumThreads = numThreads;
				return S_OK;
			}
		#endif
		//
		// BZip2Decoder
		//
		// #undef NO_INLINE
		#define NO_INLINE MY_NO_INLINE
		#define BZIP2_BYTE_MODE

		static const uint32 kInBufSize = (uint32)1 << 17;
		static const size_t kOutBufSize = (size_t)1 << 20;
		static const uint32 kProgressStep = (uint32)1 << 16;
		static const uint16 kRandNums[512] = {
			619, 720, 127, 481, 931, 816, 813, 233, 566, 247,
			985, 724, 205, 454, 863, 491, 741, 242, 949, 214,
			733, 859, 335, 708, 621, 574, 73, 654, 730, 472,
			419, 436, 278, 496, 867, 210, 399, 680, 480, 51,
			878, 465, 811, 169, 869, 675, 611, 697, 867, 561,
			862, 687, 507, 283, 482, 129, 807, 591, 733, 623,
			150, 238, 59, 379, 684, 877, 625, 169, 643, 105,
			170, 607, 520, 932, 727, 476, 693, 425, 174, 647,
			73, 122, 335, 530, 442, 853, 695, 249, 445, 515,
			909, 545, 703, 919, 874, 474, 882, 500, 594, 612,
			641, 801, 220, 162, 819, 984, 589, 513, 495, 799,
			161, 604, 958, 533, 221, 400, 386, 867, 600, 782,
			382, 596, 414, 171, 516, 375, 682, 485, 911, 276,
			98, 553, 163, 354, 666, 933, 424, 341, 533, 870,
			227, 730, 475, 186, 263, 647, 537, 686, 600, 224,
			469, 68, 770, 919, 190, 373, 294, 822, 808, 206,
			184, 943, 795, 384, 383, 461, 404, 758, 839, 887,
			715, 67, 618, 276, 204, 918, 873, 777, 604, 560,
			951, 160, 578, 722, 79, 804, 96, 409, 713, 940,
			652, 934, 970, 447, 318, 353, 859, 672, 112, 785,
			645, 863, 803, 350, 139, 93, 354, 99, 820, 908,
			609, 772, 154, 274, 580, 184, 79, 626, 630, 742,
			653, 282, 762, 623, 680, 81, 927, 626, 789, 125,
			411, 521, 938, 300, 821, 78, 343, 175, 128, 250,
			170, 774, 972, 275, 999, 639, 495, 78, 352, 126,
			857, 956, 358, 619, 580, 124, 737, 594, 701, 612,
			669, 112, 134, 694, 363, 992, 809, 743, 168, 974,
			944, 375, 748, 52, 600, 747, 642, 182, 862, 81,
			344, 805, 988, 739, 511, 655, 814, 334, 249, 515,
			897, 955, 664, 981, 649, 113, 974, 459, 893, 228,
			433, 837, 553, 268, 926, 240, 102, 654, 459, 51,
			686, 754, 806, 760, 493, 403, 415, 394, 687, 700,
			946, 670, 656, 610, 738, 392, 760, 799, 887, 653,
			978, 321, 576, 617, 626, 502, 894, 679, 243, 440,
			680, 879, 194, 572, 640, 724, 926, 56, 204, 700,
			707, 151, 457, 449, 797, 195, 791, 558, 945, 679,
			297, 59, 87, 824, 713, 663, 412, 693, 342, 606,
			134, 108, 571, 364, 631, 212, 174, 643, 304, 329,
			343, 97, 430, 751, 497, 314, 983, 374, 822, 928,
			140, 206, 73, 263, 980, 736, 876, 478, 430, 305,
			170, 514, 364, 692, 829, 82, 855, 953, 676, 246,
			369, 970, 294, 750, 807, 827, 150, 790, 288, 923,
			804, 378, 215, 828, 592, 281, 565, 555, 710, 82,
			896, 831, 547, 261, 524, 462, 293, 465, 502, 56,
			661, 821, 976, 991, 658, 869, 905, 758, 745, 193,
			768, 550, 608, 933, 378, 286, 215, 979, 792, 961,
			61, 688, 793, 644, 986, 403, 106, 366, 905, 644,
			372, 567, 466, 434, 645, 210, 389, 550, 919, 135,
			780, 773, 635, 389, 707, 100, 626, 958, 165, 504,
			920, 176, 193, 713, 857, 265, 203, 50, 668, 108,
			645, 990, 626, 197, 510, 357, 358, 850, 858, 364,
			936, 638
		};

		enum EState {
			STATE_STREAM_SIGNATURE,
			STATE_BLOCK_SIGNATURE,
			STATE_BLOCK_START,
			STATE_ORIG_BITS,
			STATE_IN_USE,
			STATE_IN_USE2,
			STATE_NUM_TABLES,
			STATE_NUM_SELECTORS,
			STATE_SELECTORS,
			STATE_LEVELS,
			STATE_BLOCK_SYMBOLS,
			STATE_STREAM_FINISHED
		};

		#define UPDATE_VAL_2(val) { val |= (uint32)(*_buf) << (24 - _numBits); _numBits += 8; _buf++; }
		#define UPDATE_VAL  UPDATE_VAL_2(VAL)
		#define READ_BITS(res, num) { \
				while(_numBits < num) {	\
					if(_buf == _lim) return SZ_OK; \
					UPDATE_VAL_2(_value) } \
				res = _value >> (32 - num); \
				_value <<= num;	\
				_numBits -= num; \
		}

		#define READ_BITS_8(res, num) {	\
				if(_numBits < num) { \
					if(_buf == _lim) return SZ_OK; \
					UPDATE_VAL_2(_value) } \
				res = _value >> (32 - num); \
				_value <<= num;	\
				_numBits -= num; \
		}

		#define READ_BIT(res) READ_BITS_8(res, 1)

		#define VAL _value2
		#define BLOCK_SIZE blockSize2
		#define RUN_COUNTER runCounter2

		#define LOAD_LOCAL \
			uint32 VAL = this->_value; \
			uint32 BLOCK_SIZE = this->blockSize; \
			uint32 RUN_COUNTER = this->runCounter; \

		#define SAVE_LOCAL \
			this->_value = VAL; \
			this->blockSize = BLOCK_SIZE; \
			this->runCounter = RUN_COUNTER;	\


		void CBitDecoder::InitBitDecoder()
		{
			_numBits = 0;
			_value = 0;
		}

		void CBitDecoder::AlignToByte()
		{
			uint   bits = _numBits & 7;
			_numBits -= bits;
			_value <<= bits;
		}
		/*
		bool CBitDecoder::AreRemainByteBitsEmpty() const
		{
			unsigned bits = _numBits & 7;
			if(bits != 0)
				return (_value >> (32 - bits)) == 0;
			return true;
		}
		*/
		SRes CBitDecoder::ReadByte(int &b)
		{
			b = -1;
			READ_BITS_8(b, 8);
			return SZ_OK;
		}

		CBase::CBase() : StreamCrcError(false), MinorError(false), NeedMoreInput(false), DecodeAllStreams(false), NumStreams(0), NumBlocks(0), FinishedPackSize(0)
		{
		}

		void CBase::InitNumStreams2()
		{
			StreamCrcError = false;
			MinorError = false;
			NeedMoreInput = 0;
			NumStreams = 0;
			NumBlocks = 0;
			FinishedPackSize = 0;
		}

		NO_INLINE SRes CBase::ReadStreamSignature2()
		{
			for(;;) {
				uint   b;
				READ_BITS_8(b, 8);
				if(state2 == 0 && b != kArSig0 || state2 == 1 && b != kArSig1 || state2 == 2 && b != kArSig2 || state2 == 3 && (b <= kArSig3 || b > kArSig3 + kBlockSizeMultMax))
					return SZ_ERROR_DATA;
				state2++;
				if(state2 == 4) {
					blockSizeMax = (uint32)(b - kArSig3) * kBlockSizeStep;
					CombinedCrc.Init();
					state = STATE_BLOCK_SIGNATURE;
					state2 = 0;
					return SZ_OK;
				}
			}
		}

		static bool FASTCALL IsEndSig(const Byte * p) throw()
			{ return p[0] == kFinSig0 && p[1] == kFinSig1 && p[2] == kFinSig2 && p[3] == kFinSig3 && p[4] == kFinSig4 && p[5] == kFinSig5; }
		static bool FASTCALL IsBlockSig(const Byte * p) throw()
			{ return p[0] == kBlockSig0 && p[1] == kBlockSig1 && p[2] == kBlockSig2 && p[3] == kBlockSig3 && p[4] == kBlockSig4 && p[5] == kBlockSig5; }

		NO_INLINE SRes CBase::ReadBlockSignature2()
		{
			while(state2 < 10) {
				uint   b;
				READ_BITS_8(b, 8);
				temp[state2] = (Byte)b;
				state2++;
			}
			crc = 0;
			for(uint i = 0; i < 4; i++) {
				crc <<= 8;
				crc |= temp[6 + i];
			}
			if(IsBlockSig(temp)) {
				if(!IsBz)
					NumStreams++;
				NumBlocks++;
				IsBz = true;
				CombinedCrc.Update(crc);
				state = STATE_BLOCK_START;
				return SZ_OK;
			}
			if(!IsEndSig(temp))
				return SZ_ERROR_DATA;
			if(!IsBz)
				NumStreams++;
			IsBz = true;
			if(_value != 0)
				MinorError = true;
			AlignToByte();
			state = STATE_STREAM_FINISHED;
			if(crc != CombinedCrc.GetDigest()) {
				StreamCrcError = true;
				return SZ_ERROR_DATA;
			}
			return SZ_OK;
		}

		NO_INLINE SRes CBase::ReadBlock2()
		{
			if(state != STATE_BLOCK_SYMBOLS) {
				PRIN("ReadBlock2")
				if(state == STATE_BLOCK_START) {
					if(Props.randMode) {
						READ_BIT(Props.randMode);
					}
					state = STATE_ORIG_BITS;
					// g_Tick = GetCpuTicks();
				}
				if(state == STATE_ORIG_BITS) {
					READ_BITS(Props.origPtr, kNumOrigBits);
					if(Props.origPtr >= blockSizeMax)
						return SZ_ERROR_DATA;
					state = STATE_IN_USE;
				}
				// why original code compares origPtr to (uint32)(10 + blockSizeMax)) ?
				if(state == STATE_IN_USE) {
					READ_BITS(state2, 16);
					state = STATE_IN_USE2;
					state3 = 0;
					numInUse = 0;
					mtf.StartInit();
				}
				if(state == STATE_IN_USE2) {
					for(; state3 < 256; state3++)
						if(state2 & ((uint32)0x8000 >> (state3 >> 4))) {
							uint   b;
							READ_BIT(b);
							if(b)
								mtf.Add(numInUse++, (Byte)state3);
						}
					if(numInUse == 0)
						return SZ_ERROR_DATA;
					state = STATE_NUM_TABLES;
				}
				if(state == STATE_NUM_TABLES) {
					READ_BITS_8(numTables, kNumTablesBits);
					state = STATE_NUM_SELECTORS;
					if(numTables < kNumTablesMin || numTables > kNumTablesMax)
						return SZ_ERROR_DATA;
				}
				if(state == STATE_NUM_SELECTORS) {
					READ_BITS(numSelectors, kNumSelectorsBits);
					state = STATE_SELECTORS;
					state2 = 0x543210;
					state3 = 0;
					state4 = 0;
					if(numSelectors == 0 || numSelectors > kNumSelectorsMax)
						return SZ_ERROR_DATA;
				}
				if(state == STATE_SELECTORS) {
					const uint kMtfBits = 4;
					const uint32 kMtfMask = (1 << kMtfBits) - 1;
					do {
						for(;;) {
							uint   b;
							READ_BIT(b);
							if(!b)
								break;
							if(++state4 >= numTables)
								return SZ_ERROR_DATA;
						}
						uint32 tmp = (state2 >> (kMtfBits * state4)) & kMtfMask;
						uint32 mask = ((uint32)1 << ((state4 + 1) * kMtfBits)) - 1;
						state4 = 0;
						state2 = ((state2 << kMtfBits) & mask) | (state2 & ~mask) | tmp;
						selectors[state3] = (Byte)tmp;
					}
					while(++state3 < numSelectors);

					state = STATE_LEVELS;
					state2 = 0;
					state3 = 0;
				}

				if(state == STATE_LEVELS) {
					do {
						if(state3 == 0) {
							READ_BITS_8(state3, kNumLevelsBits);
							state4 = 0;
							state5 = 0;
						}
						const uint alphaSize = numInUse + 2;
						for(; state4 < alphaSize; state4++) {
							for(;;) {
								if(state3 < 1 || state3 > kMaxHuffmanLen)
									return SZ_ERROR_DATA;
								if(state5 == 0) {
									unsigned b;
									READ_BIT(b);
									if(!b)
										break;
								}
								state5 = 1;
								unsigned b;
								READ_BIT(b);
								state5 = 0;
								state3++;
								state3 -= (b << 1);
							}
							lens[state4] = (Byte)state3;
							state5 = 0;
						}
						/*
						   for(unsigned i = state4; i < kMaxAlphaSize; i++)
						   lens[i] = 0;
						 */
						if(!huffs[state2].BuildFull(lens, state4))
							return SZ_ERROR_DATA;
						state3 = 0;
					} while(++state2 < numTables);
					{
						uint32 * counters = this->Counters;
						for(uint i = 0; i < 256; i++)
							counters[i] = 0;
					}
					state = STATE_BLOCK_SYMBOLS;
					groupIndex = 0;
					groupSize = kGroupSize;
					runPower = 0;
					runCounter = 0;
					blockSize = 0;
				}
				if(state != STATE_BLOCK_SYMBOLS)
					return SZ_ERROR_DATA;
				// g_Ticks[3] += GetCpuTicks() - g_Tick;
			}
			{
				LOAD_LOCAL
				const CHuffmanDecoder * huff = &huffs[selectors[groupIndex]];
				for(;;) {
					if(groupSize == 0) {
						if(++groupIndex >= numSelectors)
							return SZ_ERROR_DATA;
						huff = &huffs[selectors[groupIndex]];
						groupSize = kGroupSize;
					}
					if(_numBits <= 8 && _buf != _lim) {
						UPDATE_VAL
						if(_buf != _lim) {
							UPDATE_VAL
							if(_buf != _lim) {
								UPDATE_VAL
							}
						}
					}
					uint32 sym;
					uint32 val = VAL >> (32 - kMaxHuffmanLen);
					if(val >= huff->_limits[kNumTableBits]) {
						if(_numBits <= kMaxHuffmanLen && _buf != _lim) {
							UPDATE_VAL
							if(_numBits <= kMaxHuffmanLen && _buf != _lim) {
								UPDATE_VAL
							}
						}
						val = VAL >> (32 - kMaxHuffmanLen);
						uint   len;
						for(len = kNumTableBits + 1; val >= huff->_limits[len]; len++) 
							;
						/*
						   if(len > kNumBitsMax)
						   return SZ_ERROR_DATA;
						 */
						if(_numBits < len) {
							SAVE_LOCAL
							return SZ_OK;
						}
						sym = huff->_symbols[huff->_poses[len] + ((val - huff->_limits[(size_t)len - 1]) >> (kNumBitsMax - len))];
						VAL <<= len;
						_numBits -= len;
					}
					else {
						sym = huff->_lens[val >> (kMaxHuffmanLen - kNumTableBits)];
						uint   len = (sym & NHuffman::kPairLenMask);
						sym >>= NHuffman::kNumPairLenBits;
						if(_numBits < len) {
							SAVE_LOCAL
							return SZ_OK;
						}
						VAL <<= len;
						_numBits -= len;
					}
					groupSize--;
					if(sym < 2) {
						RUN_COUNTER += ((uint32)(sym + 1) << runPower);
						runPower++;
						if(blockSizeMax - BLOCK_SIZE < RUN_COUNTER)
							return SZ_ERROR_DATA;
						continue;
					}
					uint32 * counters = this->Counters;
					if(RUN_COUNTER != 0) {
						uint32 b = (uint32)(mtf.Buf[0] & 0xFF);
						counters[b] += RUN_COUNTER;
						runPower = 0;
			#ifdef BZIP2_BYTE_MODE
						Byte * dest = (Byte *)(&counters[256 + kBlockSizeMax]) + BLOCK_SIZE;
						const Byte * limit = dest + RUN_COUNTER;
						BLOCK_SIZE += RUN_COUNTER;
						RUN_COUNTER = 0;
						do {
							dest[0] = (Byte)b;
							dest[1] = (Byte)b;
							dest[2] = (Byte)b;
							dest[3] = (Byte)b;
							dest += 4;
						} while(dest < limit);
			#else
						uint32 * dest = &counters[256 + BLOCK_SIZE];
						const uint32 * limit = dest + RUN_COUNTER;
						BLOCK_SIZE += RUN_COUNTER;
						RUN_COUNTER = 0;
						do {
							dest[0] = b;
							dest[1] = b;
							dest[2] = b;
							dest[3] = b;
							dest += 4;
						} while(dest < limit);
			#endif
					}
					sym -= 1;
					if(sym < numInUse) {
						if(BLOCK_SIZE >= blockSizeMax)
							return SZ_ERROR_DATA;
						// uint32 b = (uint32)mtf.GetAndMove((uint)sym);
						const uint lim = sym >> MTF_MOVS;
						const uint pos = (sym & MTF_MASK) << 3;
						CMtfVar next = mtf.Buf[lim];
						CMtfVar prev = (next >> pos) & 0xFF;
			#ifdef BZIP2_BYTE_MODE
						((Byte *)(counters + 256 + kBlockSizeMax))[BLOCK_SIZE++] = (Byte)prev;
			#else
						(counters + 256)[BLOCK_SIZE++] = (uint32)prev;
			#endif
						counters[prev]++;
						CMtfVar * m = mtf.Buf;
						CMtfVar * mLim = m + lim;
						if(lim != 0) {
							do {
								CMtfVar n0 = *m;
								*m = (n0 << 8) | prev;
								prev = (n0 >> (MTF_MASK << 3));
							} while(++m != mLim);
						}
						CMtfVar mask = (((CMtfVar)0x100 << pos) - 1);
						*mLim = (next & ~mask) | (((next << 8) | prev) & mask);
						continue;
					}
					if(sym != numInUse)
						return SZ_ERROR_DATA;
					break;
				}
				// we write additional item that will be read in DecodeBlock1 for prefetching
			#ifdef BZIP2_BYTE_MODE
				((Byte *)(Counters + 256 + kBlockSizeMax))[BLOCK_SIZE] = 0;
			#else
				(counters + 256)[BLOCK_SIZE] = 0;
			#endif
				SAVE_LOCAL
				Props.blockSize = blockSize;
				state = STATE_BLOCK_SIGNATURE;
				state2 = 0;
				PRIN_VAL("origPtr", Props.origPtr);
				PRIN_VAL("blockSize", Props.blockSize);
				return (Props.origPtr < Props.blockSize) ? SZ_OK : SZ_ERROR_DATA;
			}
		}

		NO_INLINE static void DecodeBlock1(uint32 * counters, uint32 blockSize)
		{
			{
				uint32 sum = 0;
				for(uint32 i = 0; i < 256; i++) {
					const uint32 v = counters[i];
					counters[i] = sum;
					sum += v;
				}
			}

			uint32 * tt = counters + 256;
			// Compute the T^(-1) vector

			// blockSize--;

		  #ifdef BZIP2_BYTE_MODE

			unsigned c = ((const Byte *)(tt + kBlockSizeMax))[0];

			for(uint32 i = 0; i < blockSize; i++) {
				unsigned c1 = c;
				const uint32 pos = counters[c];
				c = ((const Byte *)(tt + kBlockSizeMax))[(size_t)i + 1];
				counters[c1] = pos + 1;
				tt[pos] = (i << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
			}

			/*
			   // last iteration without next character prefetching
			   {
			   const uint32 pos = counters[c];
			   counters[c] = pos + 1;
			   tt[pos] = (blockSize << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
			   }
			 */

		  #else

			unsigned c = (uint)(tt[0] & 0xFF);

			for(uint32 i = 0; i < blockSize; i++) {
				unsigned c1 = c;
				const uint32 pos = counters[c];
				c = (uint)(tt[(size_t)i + 1] & 0xFF);
				counters[c1] = pos + 1;
				tt[pos] |= (i << 8);
			}

			/*
			   {
			   const uint32 pos = counters[c];
			   counters[c] = pos + 1;
			   tt[pos] |= (blockSize << 8);
			   }
			 */

		  #endif

			/*
			   for(uint32 i = 0; i < blockSize; i++)
			   {
			   #ifdef BZIP2_BYTE_MODE
				const uint c = ((const Byte *)(tt + kBlockSizeMax))[i];
				const uint32 pos = counters[c]++;
				tt[pos] = (i << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
			   #else
				const uint c = (uint)(tt[i] & 0xFF);
				const uint32 pos = counters[c]++;
				tt[pos] |= (i << 8);
			   #endif
			   }
			 */
		}

		void CDecoder::CSpecState::Init(uint32 origPtr, unsigned randMode) throw()
		{
			_tPos = _tt[_tt[origPtr] >> 8];
			_prevByte = (uint)(_tPos & 0xFF);
			_reps = 0;
			_randIndex = 0;
			_randToGo = -1;
			if(randMode) {
				_randIndex = 1;
				_randToGo = kRandNums[0] - 2;
			}
			_crc.Init();
		}

		NO_INLINE Byte * CDecoder::CSpecState::Decode(Byte * data, size_t size) throw()
		{
			if(size == 0)
				return data;
			unsigned prevByte = _prevByte;
			int reps = _reps;
			CBZip2Crc crc = _crc;
			const Byte * lim = data + size;
			while(reps > 0) {
				reps--;
				*data++ = (Byte)prevByte;
				crc.UpdateByte(prevByte);
				if(data == lim)
					break;
			}
			uint32 tPos = _tPos;
			uint32 blockSize = _blockSize;
			const uint32 * tt = _tt;
			if(data != lim && blockSize) {
				for(;;) {
					unsigned b = (uint)(tPos & 0xFF);
					tPos = tt[tPos >> 8];
					blockSize--;
					if(_randToGo >= 0) {
						if(_randToGo == 0) {
							b ^= 1;
							_randToGo = kRandNums[_randIndex];
							_randIndex++;
							_randIndex &= 0x1FF;
						}
						_randToGo--;
					}
					if(reps != -(int)kRleModeRepSize) {
						if(b != prevByte)
							reps = 0;
						reps--;
						prevByte = b;
						*data++ = (Byte)b;
						crc.UpdateByte(b);
						if(data == lim || blockSize == 0)
							break;
						continue;
					}
					reps = b;
					while(reps) {
						reps--;
						*data++ = (Byte)prevByte;
						crc.UpdateByte(prevByte);
						if(data == lim)
							break;
					}
					if(data == lim)
						break;
					if(blockSize == 0)
						break;
				}
			}
			if(blockSize == 1 && reps == -(int)kRleModeRepSize) {
				unsigned b = (uint)(tPos & 0xFF);
				tPos = tt[tPos >> 8];
				blockSize--;

				if(_randToGo >= 0) {
					if(_randToGo == 0) {
						b ^= 1;
						_randToGo = kRandNums[_randIndex];
						_randIndex++;
						_randIndex &= 0x1FF;
					}
					_randToGo--;
				}

				reps = b;
			}
			_tPos = tPos;
			_prevByte = prevByte;
			_reps = reps;
			_crc = crc;
			_blockSize = blockSize;
			return data;
		}

		HRESULT CDecoder::Flush()
		{
			if(_writeRes == S_OK) {
				_writeRes = WriteStream(_outStream, _outBuf, _outPos);
				_outWritten += _outPos;
				_outPos = 0;
			}
			return _writeRes;
		}

		NO_INLINE HRESULT CDecoder::DecodeBlock(const CBlockProps &props)
		{
			_calcedBlockCrc = 0;
			_blockFinished = false;
			CSpecState block;
			block._blockSize = props.blockSize;
			block._tt = _counters + 256;
			block.Init(props.origPtr, props.randMode);
			for(;;) {
				Byte * data = _outBuf + _outPos;
				size_t size = kOutBufSize - _outPos;
				if(_outSizeDefined) {
					const uint64 rem = _outSize - _outPosTotal;
					if(size >= rem) {
						size = (size_t)rem;
						if(size == 0)
							return FinishMode ? S_FALSE : S_OK;
					}
				}
				TICKS_START
				const size_t processed = block.Decode(data, size) - data;
				TICKS_UPDATE(2)
				_outPosTotal += processed;
				_outPos += processed;
				if(processed >= size) {
					RINOK(Flush());
				}
				if(block.Finished()) {
					_blockFinished = true;
					_calcedBlockCrc = block._crc.GetDigest();
					return S_OK;
				}
			}
		}

		CDecoder::CDecoder() : _inBuf(NULL), _outBuf(NULL), _counters(NULL), FinishMode(false), _outSizeDefined(false)
		{
		  #ifndef _7ZIP_ST
			MtMode = false;
			NeedWaitScout = false;
			// ScoutRes = S_OK;
		  #endif
		}

		CDecoder::~CDecoder()
		{
			PRIN("\n~CDecoder()");

		  #ifndef _7ZIP_ST
			if(Thread.IsCreated()) {
				WaitScout();
				_block.StopScout = true;
				PRIN("\nScoutEvent.Set()");
				ScoutEvent.Set();
				PRIN("\nThread.Wait()()");
				Thread.Wait();
				PRIN("\n after Thread.Wait()()");
				Thread.Close();

				// if(ScoutRes != S_OK) throw ScoutRes;
			}
		  #endif
			BigFree(_counters);
			MidFree(_outBuf);
			MidFree(_inBuf);
		}

		HRESULT CDecoder::ReadInput()
		{
			if(Base._buf != Base._lim || _inputFinished || _inputRes != S_OK)
				return _inputRes;

			_inProcessed += (Base._buf - _inBuf);
			Base._buf = _inBuf;
			Base._lim = _inBuf;
			uint32 size = 0;
			_inputRes = Base.InStream->Read(_inBuf, kInBufSize, &size);
			_inputFinished = (size == 0);
			Base._lim = _inBuf + size;
			return _inputRes;
		}

		void CDecoder::StartNewStream()
		{
			Base.state = STATE_STREAM_SIGNATURE;
			Base.state2 = 0;
			Base.IsBz = false;
		}

		HRESULT CDecoder::ReadStreamSignature()
		{
			for(;;) {
				RINOK(ReadInput());
				SRes res = Base.ReadStreamSignature2();
				if(res != SZ_OK)
					return S_FALSE;
				if(Base.state == STATE_BLOCK_SIGNATURE)
					return S_OK;
				if(_inputFinished) {
					Base.NeedMoreInput = true;
					return S_FALSE;
				}
			}
		}

		HRESULT CDecoder::StartRead()
		{
			StartNewStream();
			return ReadStreamSignature();
		}

		HRESULT CDecoder::ReadBlockSignature()
		{
			for(;;) {
				RINOK(ReadInput());
				SRes res = Base.ReadBlockSignature2();
				if(Base.state == STATE_STREAM_FINISHED)
					Base.FinishedPackSize = GetInputProcessedSize();
				if(res != SZ_OK)
					return S_FALSE;
				if(Base.state != STATE_BLOCK_SIGNATURE)
					return S_OK;
				if(_inputFinished) {
					Base.NeedMoreInput = true;
					return S_FALSE;
				}
			}
		}

		HRESULT CDecoder::ReadBlock()
		{
			for(;;) {
				RINOK(ReadInput());
				SRes res = Base.ReadBlock2();
				if(res != SZ_OK)
					return S_FALSE;
				if(Base.state == STATE_BLOCK_SIGNATURE)
					return S_OK;
				if(_inputFinished) {
					Base.NeedMoreInput = true;
					return S_FALSE;
				}
			}
		}

		HRESULT CDecoder::DecodeStreams(ICompressProgressInfo * progress)
		{
			{
			#ifndef _7ZIP_ST
				_block.StopScout = false;
			#endif
			}
			RINOK(StartRead());
			uint64 inPrev = 0;
			uint64 outPrev = 0;
			{
			#ifndef _7ZIP_ST
				CWaitScout_Releaser waitScout_Releaser(this);
				bool useMt = false;
			#endif
				bool wasFinished = false;
				uint32 crc = 0;
				uint32 nextCrc = 0;
				HRESULT nextRes = S_OK;
				uint64 packPos = 0;
				CBlockProps props;
				props.blockSize = 0;
				for(;;) {
					if(progress) {
						const uint64 outCur = GetOutProcessedSize();
						if(packPos - inPrev >= kProgressStep || outCur - outPrev >= kProgressStep) {
							RINOK(progress->SetRatioInfo(&packPos, &outCur));
							inPrev = packPos;
							outPrev = outCur;
						}
					}
					if(props.blockSize == 0)
						if(wasFinished || nextRes != S_OK)
							return nextRes;
					if(
			  #ifndef _7ZIP_ST
								!useMt &&
			  #endif
								!wasFinished && Base.state == STATE_BLOCK_SIGNATURE) {
						nextRes = ReadBlockSignature();
						nextCrc = Base.crc;
						packPos = GetInputProcessedSize();
						wasFinished = true;
						if(nextRes != S_OK)
							continue;
						if(Base.state == STATE_STREAM_FINISHED) {
							if(!Base.DecodeAllStreams) {
								wasFinished = true;
								continue;
							}
							nextRes = StartRead();
							if(Base.NeedMoreInput) {
								if(Base.state2 == 0)
									Base.NeedMoreInput = false;
								wasFinished = true;
								nextRes = S_OK;
								continue;
							}
							if(nextRes != S_OK)
								continue;
							wasFinished = false;
							continue;
						}
						wasFinished = false;
			#ifndef _7ZIP_ST
						if(MtMode)
							if(props.blockSize != 0) {
								// we start multithreading, if next block is big enough.
								const uint32 k_Mt_BlockSize_Threshold = (1 << 12); // (1 << 13)
								if(props.blockSize > k_Mt_BlockSize_Threshold) {
									if(!Thread.IsCreated()) {
										PRIN("=== MT_MODE");
										RINOK(CreateThread());
									}
									useMt = true;
								}
							}
			#endif
					}

					if(props.blockSize == 0) {
						crc = nextCrc;

			#ifndef _7ZIP_ST
						if(useMt) {
							PRIN("DecoderEvent.Lock()");
							RINOK(DecoderEvent.Lock());
							NeedWaitScout = false;
							PRIN("-- DecoderEvent.Lock()");
							props = _block.Props;
							nextCrc = _block.NextCrc;
							if(_block.Crc_Defined)
								crc = _block.Crc;
							packPos = _block.PackPos;
							wasFinished = _block.WasFinished;
							RINOK(_block.Res);
						}
						else
			#endif
						{
							if(Base.state != STATE_BLOCK_START)
								return E_FAIL;
							TICKS_START
							Base.Props.randMode = 1;
							RINOK(ReadBlock());
							TICKS_UPDATE(0)
							props = Base.Props;
							continue;
						}
					}
					if(props.blockSize != 0) {
						TICKS_START
						DecodeBlock1(_counters, props.blockSize);
						TICKS_UPDATE(1)
					}
			  #ifndef _7ZIP_ST
					if(useMt && !wasFinished) {
						/*
						   if(props.blockSize == 0)
						   {
						   // this codes switches back to single-threadMode
						   useMt = false;
						   PRIN("=== ST_MODE");
						   continue;
						   }
						 */
						PRIN("ScoutEvent.Set()");
						RINOK(ScoutEvent.Set());
						NeedWaitScout = true;
					}
			  #endif
					if(props.blockSize == 0)
						continue;
					RINOK(DecodeBlock(props));
					if(!_blockFinished)
						return nextRes;
					props.blockSize = 0;
					if(_calcedBlockCrc != crc) {
						BlockCrcError = true;
						return S_FALSE;
					}
				}
			}
		}

		bool CDecoder::CreateInputBufer()
		{
			if(!_inBuf) {
				_inBuf = static_cast<Byte *>(MidAlloc(kInBufSize));
				if(!_inBuf)
					return false;
			}
			if(!_counters) {
				_counters = static_cast<uint32 *>(::BigAlloc((256 + kBlockSizeMax) * sizeof(uint32)
			  #ifdef BZIP2_BYTE_MODE
							+ kBlockSizeMax
			  #endif
							+ 256));
				if(!_counters)
					return false;
				Base.Counters = _counters;
			}
			return true;
		}

		void CDecoder::InitInputBuffer()
		{
			_inProcessed = 0;
			Base._buf = _inBuf;
			Base._lim = _inBuf;
			Base.InitBitDecoder();
		}

		uint64 CDecoder::GetInputProcessedSize() const
		{
			// for NSIS case : we need also look the number of bits in bitDecoder
			return _inProcessed + (Base._buf - _inBuf);
		}

		uint64 CDecoder::GetOutProcessedSize() const { return _outWritten + _outPos; }

		void CDecoder::InitOutSize(const uint64 * outSize)
		{
			_outPosTotal = 0;
			_outSizeDefined = false;
			_outSize = 0;
			if(outSize) {
				_outSize = *outSize;
				_outSizeDefined = true;
			}
			BlockCrcError = false;
			Base.InitNumStreams2();
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			/*
			   {
			   RINOK(SetInStream(inStream));
			   RINOK(SetOutStreamSize(outSize));

			   RINOK(CopyStream(this, outStream, progress));
			   return ReleaseInStream();
			   }
			 */
			InitOutSize(outSize);
			_inputFinished = false;
			_inputRes = S_OK;
			_writeRes = S_OK;
			try {
				if(!CreateInputBufer())
					return E_OUTOFMEMORY;
				if(!_outBuf) {
					_outBuf = static_cast<Byte *>(MidAlloc(kOutBufSize));
					if(!_outBuf)
						return E_OUTOFMEMORY;
				}
				Base.InStream = inStream;
				InitInputBuffer();
				_outStream = outStream;
				_outWritten = 0;
				_outPos = 0;
				HRESULT res = DecodeStreams(progress);
				Flush();
				Base.InStream = NULL;
				_outStream = NULL;
				/*
				   if(res == S_OK)
				   if(FinishMode && inSize && *inSize != GetInputProcessedSize())
					res = S_FALSE;
				 */
				if(res != S_OK)
					return res;
			} catch(...) { return E_FAIL; }
			return _writeRes;
		}

		STDMETHODIMP CDecoder::SetFinishMode(uint32 finishMode)
		{
			FinishMode = (finishMode != 0);
			return S_OK;
		}

		STDMETHODIMP CDecoder::GetInStreamProcessedSize(uint64 * value)
		{
			*value = GetInputProcessedSize();
			return S_OK;
		}

		#ifndef _7ZIP_ST

		#define RINOK_THREAD(x) { WRes __result_ = (x); if(__result_ != 0) return __result_; }

		static THREAD_FUNC_DECL RunScout2(void * p) 
		{
			((CDecoder*)p)->RunScout(); return 0;
		}

		HRESULT CDecoder::CreateThread()
		{
			RINOK_THREAD(DecoderEvent.CreateIfNotCreated());
			RINOK_THREAD(ScoutEvent.CreateIfNotCreated());
			RINOK_THREAD(Thread.Create(RunScout2, this));
			return S_OK;
		}

		void CDecoder::RunScout()
		{
			for(;;) {
				{
					PRIN_MT("ScoutEvent.Lock()");
					WRes wres = ScoutEvent.Lock();
					PRIN_MT("-- ScoutEvent.Lock()");
					if(wres != 0) {
						// ScoutRes = wres;
						return;
					}
				}
				CBlock &block = _block;
				if(block.StopScout) {
					// ScoutRes = S_OK;
					return;
				}
				block.Res = S_OK;
				block.WasFinished = false;
				HRESULT res = S_OK;
				try {
					uint64 packPos = GetInputProcessedSize();
					block.Props.blockSize = 0;
					block.Crc_Defined = false;
					// block.NextCrc_Defined = false;
					block.NextCrc = 0;
					for(;;) {
						if(Base.state == STATE_BLOCK_SIGNATURE) {
							res = ReadBlockSignature();
							if(res != S_OK)
								break;
							if(block.Props.blockSize == 0) {
								block.Crc = Base.crc;
								block.Crc_Defined = true;
							}
							else {
								block.NextCrc = Base.crc;
								// block.NextCrc_Defined = true;
							}
							continue;
						}
						if(Base.state == STATE_BLOCK_START) {
							if(block.Props.blockSize != 0)
								break;
							Base.Props.randMode = 1;
							res = ReadBlock();
							PRIN_MT("-- Base.ReadBlock");
							if(res != S_OK)
								break;
							block.Props = Base.Props;
							continue;
						}
						if(Base.state == STATE_STREAM_FINISHED) {
							if(!Base.DecodeAllStreams) {
								block.WasFinished = true;
								break;
							}
							res = StartRead();
							if(Base.NeedMoreInput) {
								if(Base.state2 == 0)
									Base.NeedMoreInput = false;
								block.WasFinished = true;
								res = S_OK;
								break;
							}
							if(res != S_OK)
								break;
							if(GetInputProcessedSize() - packPos > 0) // kProgressStep
								break;
							continue;
						}
						// throw 1;
						res = E_FAIL;
						break;
					}
				}
				catch(...) { res = E_FAIL; }
				if(res != S_OK) {
					PRIN_MT("error");
					block.Res = res;
					block.WasFinished = true;
				}
				block.PackPos = GetInputProcessedSize();
				PRIN_MT("DecoderEvent.Set()");
				WRes wres = DecoderEvent.Set();
				if(wres != 0) {
					// ScoutRes = wres;
					return;
				}
			}
		}

		void CDecoder::WaitScout()
		{
			if(NeedWaitScout) {
				DecoderEvent.Lock();
				NeedWaitScout = false;
			}
		}

		CDecoder::CWaitScout_Releaser::CWaitScout_Releaser(CDecoder * decoder) : _decoder(decoder) 
		{
		}

		CDecoder::CWaitScout_Releaser::~CWaitScout_Releaser() 
		{
			_decoder->WaitScout();
		}

		STDMETHODIMP CDecoder::SetNumberOfThreads(uint32 numThreads)
		{
			MtMode = (numThreads > 1);
		  #ifndef BZIP2_BYTE_MODE
			MtMode = false;
		  #endif
			// MtMode = false;
			return S_OK;
		}

		#endif

		#ifndef NO_READ_FROM_CODER

		STDMETHODIMP CDecoder::SetInStream(ISequentialInStream * inStream)
		{
			Base.InStreamRef = inStream;
			Base.InStream = inStream;
			return S_OK;
		}

		STDMETHODIMP CDecoder::ReleaseInStream()
		{
			Base.InStreamRef.Release();
			Base.InStream = NULL;
			return S_OK;
		}

		STDMETHODIMP CDecoder::SetOutStreamSize(const uint64 * outSize)
		{
			InitOutSize(outSize);
			if(!CreateInputBufer())
				return E_OUTOFMEMORY;
			InitInputBuffer();
			StartNewStream();
			_blockFinished = true;
			ErrorResult = S_OK;
			_inputFinished = false;
			_inputRes = S_OK;
			return S_OK;
		}

		STDMETHODIMP CDecoder::Read(void * data, uint32 size, uint32 * processedSize)
		{
			*processedSize = 0;
			try {
				if(ErrorResult != S_OK)
					return ErrorResult;
				for(;;) {
					if(Base.state == STATE_STREAM_FINISHED) {
						if(!Base.DecodeAllStreams)
							return ErrorResult;
						StartNewStream();
						continue;
					}
					if(Base.state == STATE_STREAM_SIGNATURE) {
						ErrorResult = ReadStreamSignature();
						if(Base.NeedMoreInput)
							if(Base.state2 == 0 && Base.NumStreams != 0) {
								Base.NeedMoreInput = false;
								ErrorResult = S_OK;
								return S_OK;
							}
						if(ErrorResult != S_OK)
							return ErrorResult;
						continue;
					}

					if(_blockFinished && Base.state == STATE_BLOCK_SIGNATURE) {
						ErrorResult = ReadBlockSignature();
						if(ErrorResult != S_OK)
							return ErrorResult;
						continue;
					}
					if(_outSizeDefined) {
						const uint64 rem = _outSize - _outPosTotal;
						SETMIN(size, (uint32)rem);
					}
					if(size == 0)
						return S_OK;
					if(_blockFinished) {
						if(Base.state != STATE_BLOCK_START) {
							ErrorResult = E_FAIL;
							return ErrorResult;
						}
						Base.Props.randMode = 1;
						ErrorResult = ReadBlock();
						if(ErrorResult != S_OK)
							return ErrorResult;
						DecodeBlock1(_counters, Base.Props.blockSize);
						_spec._blockSize = Base.Props.blockSize;
						_spec._tt = _counters + 256;
						_spec.Init(Base.Props.origPtr, Base.Props.randMode);
						_blockFinished = false;
					}
					{
						Byte * ptr = _spec.Decode((Byte *)data, size);
						const uint32 processed = (uint32)(ptr - (Byte *)data);
						data = ptr;
						size -= processed;
						(*processedSize) += processed;
						_outPosTotal += processed;
						if(_spec.Finished()) {
							_blockFinished = true;
							if(Base.crc != _spec._crc.GetDigest()) {
								BlockCrcError = true;
								ErrorResult = S_FALSE;
								return ErrorResult;
							}
						}
					}
				}
			} catch(...) { ErrorResult = S_FALSE; return S_FALSE; }
		}

		// ---------- NSIS ----------

		STDMETHODIMP CNsisDecoder::Read(void * data, uint32 size, uint32 * processedSize)
		{
			*processedSize = 0;
			try {
				if(ErrorResult != S_OK)
					return ErrorResult;
				if(Base.state == STATE_STREAM_FINISHED)
					return S_OK;
				if(Base.state == STATE_STREAM_SIGNATURE) {
					Base.blockSizeMax = 9 * kBlockSizeStep;
					Base.state = STATE_BLOCK_SIGNATURE;
					// Base.state2 = 0;
				}
				for(;;) {
					if(_blockFinished && Base.state == STATE_BLOCK_SIGNATURE) {
						ErrorResult = ReadInput();
						if(ErrorResult != S_OK)
							return ErrorResult;
						int b;
						Base.ReadByte(b);
						if(b < 0) {
							ErrorResult = S_FALSE;
							return ErrorResult;
						}
						if(b == kFinSig0) {
							/*
							   if(!Base.AreRemainByteBitsEmpty())
							   ErrorResult = S_FALSE;
							 */
							Base.state = STATE_STREAM_FINISHED;
							return ErrorResult;
						}
						if(b != kBlockSig0) {
							ErrorResult = S_FALSE;
							return ErrorResult;
						}
						Base.state = STATE_BLOCK_START;
					}
					if(_outSizeDefined) {
						const uint64 rem = _outSize - _outPosTotal;
						SETMIN(size, (uint32)rem);
					}
					if(size == 0)
						return S_OK;
					if(_blockFinished) {
						if(Base.state != STATE_BLOCK_START) {
							ErrorResult = E_FAIL;
							return ErrorResult;
						}
						Base.Props.randMode = 0;
						ErrorResult = ReadBlock();
						if(ErrorResult != S_OK)
							return ErrorResult;
						DecodeBlock1(_counters, Base.Props.blockSize);
						_spec._blockSize = Base.Props.blockSize;
						_spec._tt = _counters + 256;
						_spec.Init(Base.Props.origPtr, Base.Props.randMode);
						_blockFinished = false;
					}
					{
						Byte * ptr = _spec.Decode((Byte *)data, size);
						const uint32 processed = (uint32)(ptr - (Byte *)data);
						data = ptr;
						size -= processed;
						(*processedSize) += processed;
						_outPosTotal += processed;
						if(_spec.Finished())
							_blockFinished = true;
					}
				}
			} catch(...) { ErrorResult = S_FALSE; return S_FALSE; }
		}
		#endif
	}
}
//
// Bz2Handler.cpp
namespace NArchive {
	namespace NBz2 {
		class CHandler : public IInArchive, public IArchiveOpenSeq, public IOutArchive, public ISetProperties, public CMyUnknownImp {
			CMyComPtr <IInStream> _stream;
			CMyComPtr <ISequentialInStream> _seqStream;
			bool   _isArc;
			bool   _needSeekToStart;
			bool   _dataAfterEnd;
			bool   _needMoreInput;
			bool   _packSize_Defined;
			bool   _unpackSize_Defined;
			bool   _numStreams_Defined;
			bool   _numBlocks_Defined;
			uint64 _packSize;
			uint64 _unpackSize;
			uint64 _numStreams;
			uint64 _numBlocks;
			CSingleMethodProps _props;
		public:
			MY_UNKNOWN_IMP4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)
			INTERFACE_IInArchive(; )
			INTERFACE_IOutArchive(; )
			STDMETHOD(OpenSeq) (ISequentialInStream *stream);
			STDMETHOD(SetProperties) (const wchar_t * const * names, const PROPVARIANT *values, uint32 numProps);
			CHandler() 
			{
			}
		};

		static const Byte kProps[] = { kpidSize, kpidPackSize };
		static const Byte kArcProps[] = { kpidNumStreams, kpidNumBlocks };

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPhySize: if(_packSize_Defined) prop = _packSize; break;
				case kpidUnpackSize: if(_unpackSize_Defined) prop = _unpackSize; break;
				case kpidNumStreams: if(_numStreams_Defined) prop = _numStreams; break;
				case kpidNumBlocks: if(_numBlocks_Defined) prop = _numBlocks; break;
				case kpidErrorFlags:
				{
					uint32 v = 0;
					if(!_isArc) v |= kpv_ErrorFlags_IsNotArc; ;
					if(_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
					if(_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
					prop = v;
				}
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
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidPackSize: if(_packSize_Defined) prop = _packSize; break;
				case kpidSize: if(_unpackSize_Defined) prop = _unpackSize; break;
			}
			prop.Detach(value);
			return S_OK;
		}

		static const uint kSignatureCheckSize = 10;

		API_FUNC_static_IsArc IsArc_BZip2(const Byte * p, size_t size)
		{
			if(size < kSignatureCheckSize)
				return k_IsArc_Res_NEED_MORE;
			if(p[0] != 'B' || p[1] != 'Z' || p[2] != 'h' || p[3] < '1' || p[3] > '9')
				return k_IsArc_Res_NO;
			p += 4;
			if(NCompress::NBZip2::IsBlockSig(p))
				return k_IsArc_Res_YES;
			if(NCompress::NBZip2::IsEndSig(p))
				return k_IsArc_Res_YES;
			return k_IsArc_Res_NO;
		}
	}
	STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 *, IArchiveOpenCallback *)
	{
		COM_TRY_BEGIN
		Close();
		{
			Byte buf[kSignatureCheckSize];
			RINOK(ReadStream_FALSE(stream, buf, kSignatureCheckSize));
			if(IsArc_BZip2(buf, kSignatureCheckSize) == k_IsArc_Res_NO)
				return S_FALSE;
			_isArc = true;
			_stream = stream;
			_seqStream = stream;
			_needSeekToStart = true;
		}
		return S_OK;
		COM_TRY_END
	}
	STDMETHODIMP CHandler::OpenSeq(ISequentialInStream * stream)
	{
		Close();
		_isArc = true;
		_seqStream = stream;
		return S_OK;
	}
	STDMETHODIMP CHandler::Close()
	{
		_isArc = false;
		_needSeekToStart = false;
		_dataAfterEnd = false;
		_needMoreInput = false;
		_packSize_Defined = false;
		_unpackSize_Defined = false;
		_numStreams_Defined = false;
		_numBlocks_Defined = false;
		_packSize = 0;
		_seqStream.Release();
		_stream.Release();
		return S_OK;
	}

	STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
	{
		COM_TRY_BEGIN
		if(numItems == 0)
			return S_OK;
		if(numItems != (uint32)(int32)-1 && (numItems != 1 || indices[0] != 0))
			return E_INVALIDARG;
		if(_packSize_Defined)
			extractCallback->SetTotal(_packSize);
		CMyComPtr<ISequentialOutStream> realOutStream;
		int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
		RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
		if(!testMode && !realOutStream)
			return S_OK;
		extractCallback->PrepareOperation(askMode);
		if(_needSeekToStart) {
			if(!_stream)
				return E_FAIL;
			RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
		}
		else
			_needSeekToStart = true;
		// try {
		NCompress::NBZip2::CDecoder * decoderSpec = new NCompress::NBZip2::CDecoder;
		CMyComPtr<ICompressCoder> decoder = decoderSpec;
	  #ifndef _7ZIP_ST
		RINOK(decoderSpec->SetNumberOfThreads(_props._numThreads));
	  #endif
		CDummyOutStream * outStreamSpec = new CDummyOutStream;
		CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
		outStreamSpec->SetStream(realOutStream);
		outStreamSpec->Init();
		realOutStream.Release();
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> progress = lps;
		lps->Init(extractCallback, true);
		decoderSpec->FinishMode = true;
		decoderSpec->Base.DecodeAllStreams = true;
		_dataAfterEnd = false;
		_needMoreInput = false;
		lps->InSize = 0;
		lps->OutSize = 0;
		HRESULT result = decoderSpec->Code(_seqStream, outStream, NULL, NULL, progress);
		if(result != S_FALSE && result != S_OK)
			return result;
		if(decoderSpec->Base.NumStreams == 0) {
			_isArc = false;
			result = S_FALSE;
		}
		else {
			const uint64 inProcessedSize = decoderSpec->GetInputProcessedSize();
			uint64 packSize = inProcessedSize;
			if(decoderSpec->Base.NeedMoreInput)
				_needMoreInput = true;
			if(!decoderSpec->Base.IsBz) {
				packSize = decoderSpec->Base.FinishedPackSize;
				if(packSize != inProcessedSize)
					_dataAfterEnd = true;
			}
			_packSize = packSize;
			_unpackSize = decoderSpec->GetOutProcessedSize();
			_numStreams = decoderSpec->Base.NumStreams;
			_numBlocks = decoderSpec->GetNumBlocks();
			_packSize_Defined = true;
			_unpackSize_Defined = true;
			_numStreams_Defined = true;
			_numBlocks_Defined = true;
		}
		outStream.Release();
		int32 opRes;
		if(!_isArc)
			opRes = NExtractArc::NOperationResult::kIsNotArc;
		else if(_needMoreInput)
			opRes = NExtractArc::NOperationResult::kUnexpectedEnd;
		else if(decoderSpec->GetCrcError())
			opRes = NExtractArc::NOperationResult::kCRCError;
		else if(_dataAfterEnd)
			opRes = NExtractArc::NOperationResult::kDataAfterEnd;
		else if(result == S_FALSE)
			opRes = NExtractArc::NOperationResult::kDataError;
		else if(decoderSpec->Base.MinorError)
			opRes = NExtractArc::NOperationResult::kDataError;
		else if(result == S_OK)
			opRes = NExtractArc::NOperationResult::kOK;
		else
			return result;
		return extractCallback->SetOperationResult(opRes);
		// } catch(...) { return E_FAIL; }
		COM_TRY_END
	}

	static HRESULT UpdateArchive(uint64 unpackSize, ISequentialOutStream * outStream, const CProps &props, IArchiveUpdateCallback * updateCallback)
	{
		RINOK(updateCallback->SetTotal(unpackSize));
		CMyComPtr<ISequentialInStream> fileInStream;
		RINOK(updateCallback->GetStream(0, &fileInStream));
		CLocalProgress * localProgressSpec = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
		localProgressSpec->Init(updateCallback, true);
		NCompress::NBZip2::CEncoder * encoderSpec = new NCompress::NBZip2::CEncoder;
		CMyComPtr<ICompressCoder> encoder = encoderSpec;
		RINOK(props.SetCoderProps(encoderSpec, NULL));
		RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
		return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
	}

	STDMETHODIMP CHandler::GetFileTimeType(uint32 * type)
	{
		*type = NFileTimeType::kUnix;
		return S_OK;
	}

	STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream * outStream, uint32 numItems, IArchiveUpdateCallback * updateCallback)
	{
		COM_TRY_BEGIN
		if(numItems != 1)
			return E_INVALIDARG;
		int32 newData, newProps;
		uint32 indexInArchive;
		if(!updateCallback)
			return E_FAIL;
		RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive));
		if(IntToBool(newProps)) {
			{
				NCOM::CPropVariant prop;
				RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop));
				if(prop.vt != VT_EMPTY)
					if(prop.vt != VT_BOOL || prop.boolVal != VARIANT_FALSE)
						return E_INVALIDARG;
			}
		}
		if(IntToBool(newData)) {
			uint64 size;
			{
				NCOM::CPropVariant prop;
				RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
				if(prop.vt != VT_UI8)
					return E_INVALIDARG;
				size = prop.uhVal.QuadPart;
			}
			return UpdateArchive(size, outStream, _props, updateCallback);
		}
		if(indexInArchive != 0)
			return E_INVALIDARG;
		CLocalProgress * lps = new CLocalProgress;
		CMyComPtr<ICompressProgressInfo> progress = lps;
		lps->Init(updateCallback, true);
		CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
		updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
		if(opCallback) {
			RINOK(opCallback->ReportOperation(NEventIndexType::kInArcIndex, 0, NUpdateNotifyOp::kReplicate))
		}
		if(_stream)
			RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
		return NCompress::CopyStream(_stream, outStream, progress);
		COM_TRY_END
	}

	STDMETHODIMP CHandler::SetProperties(const wchar_t * const * names, const PROPVARIANT * values, uint32 numProps)
	{
		return _props.SetProperties(names, values, numProps);
	}

	static const Byte k_Signature[] = { 'B', 'Z', 'h' };

	REGISTER_ARC_IO("bzip2", "bz2 bzip2 tbz2 tbz", "* * .tar .tar", 2, k_Signature, 0, NArcInfoFlags::kKeepName, IsArc_BZip2)
	}
}
