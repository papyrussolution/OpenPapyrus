// 7Z-DEFLATE.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
//
// DeflateEncoder.cpp DeflateDecoder.cpp Deflate64Register.cpp
#undef NO_INLINE

#ifdef _MSC_VER
	#define NO_INLINE MY_NO_INLINE
#else
	#define NO_INLINE
#endif

namespace NCompress {
	namespace NDeflate {
		namespace NEncoder {
			static const uint kNumDivPassesMax = 10; // [0, 16); ratio/speed/ram tradeoff; use big value for better compression ratio.
			static const uint32 kNumTables = (1 << kNumDivPassesMax);
			static const uint32 kFixedHuffmanCodeBlockSizeMax = (1 << 8); // [0, (1 << 32)); ratio/speed tradeoff; use big value for
															  // better compression ratio.
			static const uint32 kDivideCodeBlockSizeMin = (1 << 7); // [1, (1 << 32)); ratio/speed tradeoff; use small value for
														// better compression ratio.
			static const uint32 kDivideBlockSizeMin = (1 << 6); // [1, (1 << 32)); ratio/speed tradeoff; use small value for better
													// compression ratio.
			static const uint32 kMaxUncompressedBlockSize = ((1 << 16) - 1) * 1; // [1, (1 << 32))
			static const uint32 kMatchArraySize = kMaxUncompressedBlockSize * 10; // [kMatchMaxLen * 2, (1 << 32))
			static const uint32 kMatchArrayLimit = kMatchArraySize - kMatchMaxLen * 4 * sizeof(uint16);
			static const uint32 kBlockUncompressedSizeThreshold = kMaxUncompressedBlockSize -
							kMatchMaxLen - kNumOpts;

			static const uint kMaxCodeBitLength = 11;
			static const uint kMaxLevelBitLength = 7;

			static const Byte kNoLiteralStatPrice = 11;
			static const Byte kNoLenStatPrice = 11;
			static const Byte kNoPosStatPrice = 6;

			static Byte g_LenSlots[kNumLenSymbolsMax];
			static Byte g_FastPos[1 << 9];

			class CFastPosInit {
			public:
				CFastPosInit()
				{
					for(uint i = 0; i < kNumLenSlots; i++) {
						uint c = kLenStart32[i];
						uint j = 1 << kLenDirectBits32[i];
						for(uint k = 0; k < j; k++, c++)
							g_LenSlots[c] = (Byte)i;
					}
					const uint kFastSlots = 18;
					uint c = 0;
					for(Byte slotFast = 0; slotFast < kFastSlots; slotFast++) {
						uint32 k = (1 << kDistDirectBits[slotFast]);
						for(uint32 j = 0; j < k; j++, c++)
							g_FastPos[c] = slotFast;
					}
				}
			};

			static CFastPosInit g_FastPosInit;

			inline uint32 GetPosSlot(uint32 pos)
			{
				return (pos < 0x200) ? g_FastPos[pos] : (g_FastPos[pos >> 8] + 16);
			}

			CEncProps::CEncProps() : Level(-1), mc(0), algo(-1), fb(-1), btMode(-1), numPasses((uint32)(int32)-1)
			{
			}

			void CEncProps::Normalize()
			{
				int level = Level;
				if(level < 0) 
					level = 5;
				Level = level;
				if(algo < 0) algo = (level < 5 ? 0 : 1);
				if(fb < 0) fb = (level < 7 ? 32 : (level < 9 ? 64 : 128));
				if(btMode < 0) btMode = (algo == 0 ? 0 : 1);
				if(mc == 0) mc = (16 + (fb >> 1));
				if(numPasses == (uint32)(int32)-1) numPasses = (level < 7 ? 1 : (level < 9 ? 3 : 10));
			}

			void CCoder::SetProps(const CEncProps * props2)
			{
				CEncProps props = *props2;
				props.Normalize();
				m_MatchFinderCycles = props.mc;
				{
					unsigned fb = props.fb;
					if(fb < kMatchMinLen)
						fb = kMatchMinLen;
					if(fb > m_MatchMaxLen)
						fb = m_MatchMaxLen;
					m_NumFastBytes = fb;
				}
				_fastMode = (props.algo == 0);
				_btMode = (props.btMode != 0);
				m_NumDivPasses = props.numPasses;
				if(m_NumDivPasses == 0)
					m_NumDivPasses = 1;
				if(m_NumDivPasses == 1)
					m_NumPasses = 1;
				else if(m_NumDivPasses <= kNumDivPassesMax)
					m_NumPasses = 2;
				else {
					m_NumPasses = 2 + (m_NumDivPasses - kNumDivPassesMax);
					m_NumDivPasses = kNumDivPassesMax;
				}
			}

			CCoder::CCoder(bool deflate64Mode) : m_Deflate64Mode(deflate64Mode), m_OnePosMatchesMemory(0), m_DistanceMemory(0),
				m_Created(false), m_Values(0), m_Tables(0)
			{
				m_MatchMaxLen = deflate64Mode ? kMatchMaxLen64 : kMatchMaxLen32;
				m_NumLenCombinations = deflate64Mode ? kNumLenSymbols64 : kNumLenSymbols32;
				m_LenStart = deflate64Mode ? kLenStart64 : kLenStart32;
				m_LenDirectBits = deflate64Mode ? kLenDirectBits64 : kLenDirectBits32;
				{
					CEncProps props;
					SetProps(&props);
				}
				MatchFinder_Construct(&_lzInWindow);
			}

			HRESULT CCoder::Create()
			{
				// COM_TRY_BEGIN
				if(m_Values == 0) {
					m_Values = (CCodeValue*)SAlloc::M((kMaxUncompressedBlockSize) * sizeof(CCodeValue));
					if(m_Values == 0)
						return E_OUTOFMEMORY;
				}
				if(m_Tables == 0) {
					m_Tables = (CTables*)SAlloc::M((kNumTables) * sizeof(CTables));
					if(m_Tables == 0)
						return E_OUTOFMEMORY;
				}
				if(m_IsMultiPass) {
					if(m_OnePosMatchesMemory == 0) {
						m_OnePosMatchesMemory = (uint16 *)::MidAlloc(kMatchArraySize * sizeof(uint16));
						if(m_OnePosMatchesMemory == 0)
							return E_OUTOFMEMORY;
					}
				}
				else {
					if(m_DistanceMemory == 0) {
						m_DistanceMemory = (uint16 *)SAlloc::M((kMatchMaxLen + 2) * 2 * sizeof(uint16));
						if(m_DistanceMemory == 0)
							return E_OUTOFMEMORY;
						m_MatchDistances = m_DistanceMemory;
					}
				}
				if(!m_Created) {
					_lzInWindow.btMode = (Byte)(_btMode ? 1 : 0);
					_lzInWindow.numHashBytes = 3;
					if(!MatchFinder_Create(&_lzInWindow, m_Deflate64Mode ? kHistorySize64 : kHistorySize32,
						kNumOpts + kMaxUncompressedBlockSize, m_NumFastBytes, m_MatchMaxLen - m_NumFastBytes, &g_Alloc))
						return E_OUTOFMEMORY;
					if(!m_OutStream.Create(1 << 20))
						return E_OUTOFMEMORY;
				}
				if(m_MatchFinderCycles != 0)
					_lzInWindow.cutValue = m_MatchFinderCycles;
				m_Created = true;
				return S_OK;
				// COM_TRY_END
			}

			HRESULT CCoder::BaseSetEncoderProperties2(const PROPID * propIDs, const PROPVARIANT * coderProps, uint32 numProps)
			{
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
						case NCoderPropID::kNumPasses: props.numPasses = v; break;
						case NCoderPropID::kNumFastBytes: props.fb = v; break;
						case NCoderPropID::kMatchFinderCycles: props.mc = v; break;
						case NCoderPropID::kAlgorithm: props.algo = v; break;
						case NCoderPropID::kLevel: props.Level = v; break;
						case NCoderPropID::kNumThreads: break;
						default: return E_INVALIDARG;
					}
				}
				SetProps(&props);
				return S_OK;
			}

			void CCoder::Free()
			{
				::MidFree(m_OnePosMatchesMemory); m_OnePosMatchesMemory = 0;
				ZFREE(m_DistanceMemory);
				ZFREE(m_Values);
				ZFREE(m_Tables);
			}

			CCoder::~CCoder()
			{
				Free();
				MatchFinder_Free(&_lzInWindow, &g_Alloc);
			}

			NO_INLINE void CCoder::GetMatches()
			{
				if(m_IsMultiPass) {
					m_MatchDistances = m_OnePosMatchesMemory + m_Pos;
					if(m_SecondPass) {
						m_Pos += *m_MatchDistances + 1;
						return;
					}
				}
				uint32 distanceTmp[kMatchMaxLen * 2 + 3];
				uint32 numPairs = (_btMode) ? Bt3Zip_MatchFinder_GetMatches(&_lzInWindow, distanceTmp) : Hc3Zip_MatchFinder_GetMatches(&_lzInWindow, distanceTmp);
				*m_MatchDistances = (uint16)numPairs;
				if(numPairs > 0) {
					uint32 i;
					for(i = 0; i < numPairs; i += 2) {
						m_MatchDistances[(size_t)i + 1] = (uint16)distanceTmp[i];
						m_MatchDistances[(size_t)i + 2] = (uint16)distanceTmp[(size_t)i + 1];
					}
					uint32 len = distanceTmp[(size_t)numPairs - 2];
					if(len == m_NumFastBytes && m_NumFastBytes != m_MatchMaxLen) {
						uint32 numAvail = Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) + 1;
						const Byte * pby = Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - 1;
						const Byte * pby2 = pby - (distanceTmp[(size_t)numPairs - 1] + 1);
						if(numAvail > m_MatchMaxLen)
							numAvail = m_MatchMaxLen;
						for(; len < numAvail && pby[len] == pby2[len]; len++) ;
						m_MatchDistances[(size_t)i - 1] = (uint16)len;
					}
				}
				if(m_IsMultiPass)
					m_Pos += numPairs + 1;
				if(!m_SecondPass)
					m_AdditionalOffset++;
			}

			void CCoder::MovePos(uint32 num)
			{
				if(!m_SecondPass && num > 0) {
					if(_btMode)
						Bt3Zip_MatchFinder_Skip(&_lzInWindow, num);
					else
						Hc3Zip_MatchFinder_Skip(&_lzInWindow, num);
					m_AdditionalOffset += num;
				}
			}

			static const uint32 kIfinityPrice = 0xFFFFFFF;

			NO_INLINE uint32 CCoder::Backward(uint32 &backRes, uint32 cur)
			{
				m_OptimumEndIndex = cur;
				uint32 posMem = m_Optimum[cur].PosPrev;
				uint16 backMem = m_Optimum[cur].BackPrev;
				do {
					uint32 posPrev = posMem;
					uint16 backCur = backMem;
					backMem = m_Optimum[posPrev].BackPrev;
					posMem = m_Optimum[posPrev].PosPrev;
					m_Optimum[posPrev].BackPrev = backCur;
					m_Optimum[posPrev].PosPrev = (uint16)cur;
					cur = posPrev;
				}
				while(cur > 0);
				backRes = m_Optimum[0].BackPrev;
				m_OptimumCurrentIndex = m_Optimum[0].PosPrev;
				return m_OptimumCurrentIndex;
			}

			NO_INLINE uint32 CCoder::GetOptimal(uint32 &backRes)
			{
				if(m_OptimumEndIndex != m_OptimumCurrentIndex) {
					uint32 len = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
					backRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
					m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
					return len;
				}
				m_OptimumCurrentIndex = m_OptimumEndIndex = 0;
				GetMatches();
				uint32 lenEnd;
				{
					const uint32 numDistancePairs = m_MatchDistances[0];
					if(numDistancePairs == 0)
						return 1;
					const uint16 * matchDistances = m_MatchDistances + 1;
					lenEnd = matchDistances[(size_t)numDistancePairs - 2];

					if(lenEnd > m_NumFastBytes) {
						backRes = matchDistances[(size_t)numDistancePairs - 1];
						MovePos(lenEnd - 1);
						return lenEnd;
					}

					m_Optimum[1].Price = m_LiteralPrices[*(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - m_AdditionalOffset)];
					m_Optimum[1].PosPrev = 0;

					m_Optimum[2].Price = kIfinityPrice;
					m_Optimum[2].PosPrev = 1;

					uint32 offs = 0;

					for(uint32 i = kMatchMinLen; i <= lenEnd; i++) {
						uint32 distance = matchDistances[(size_t)offs + 1];
						m_Optimum[i].PosPrev = 0;
						m_Optimum[i].BackPrev = (uint16)distance;
						m_Optimum[i].Price = m_LenPrices[(size_t)i - kMatchMinLen] + m_PosPrices[GetPosSlot(distance)];
						if(i == matchDistances[offs])
							offs += 2;
					}
				}

				uint32 cur = 0;

				for(;; ) {
					++cur;
					if(cur == lenEnd || cur == kNumOptsBase || m_Pos >= kMatchArrayLimit)
						return Backward(backRes, cur);
					GetMatches();
					const uint16 * matchDistances = m_MatchDistances + 1;
					const uint32 numDistancePairs = m_MatchDistances[0];
					uint32 newLen = 0;
					if(numDistancePairs != 0) {
						newLen = matchDistances[(size_t)numDistancePairs - 2];
						if(newLen > m_NumFastBytes) {
							uint32 len = Backward(backRes, cur);
							m_Optimum[cur].BackPrev = matchDistances[(size_t)numDistancePairs - 1];
							m_OptimumEndIndex = cur + newLen;
							m_Optimum[cur].PosPrev = (uint16)m_OptimumEndIndex;
							MovePos(newLen - 1);
							return len;
						}
					}
					uint32 curPrice = m_Optimum[cur].Price;
					{
						const uint32 curAnd1Price = curPrice +
										m_LiteralPrices[*(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) + cur -
											m_AdditionalOffset)];
						COptimal &optimum = m_Optimum[(size_t)cur + 1];
						if(curAnd1Price < optimum.Price) {
							optimum.Price = curAnd1Price;
							optimum.PosPrev = (uint16)cur;
						}
					}
					if(numDistancePairs == 0)
						continue;
					while(lenEnd < cur + newLen)
						m_Optimum[++lenEnd].Price = kIfinityPrice;
					uint32 offs = 0;
					uint32 distance = matchDistances[(size_t)offs + 1];
					curPrice += m_PosPrices[GetPosSlot(distance)];
					for(uint32 lenTest = kMatchMinLen;; lenTest++) {
						uint32 curAndLenPrice = curPrice + m_LenPrices[(size_t)lenTest - kMatchMinLen];
						COptimal &optimum = m_Optimum[cur + lenTest];
						if(curAndLenPrice < optimum.Price) {
							optimum.Price = curAndLenPrice;
							optimum.PosPrev = (uint16)cur;
							optimum.BackPrev = (uint16)distance;
						}
						if(lenTest == matchDistances[offs]) {
							offs += 2;
							if(offs == numDistancePairs)
								break;
							curPrice -= m_PosPrices[GetPosSlot(distance)];
							distance = matchDistances[(size_t)offs + 1];
							curPrice += m_PosPrices[GetPosSlot(distance)];
						}
					}
				}
			}

			uint32 CCoder::GetOptimalFast(uint32 &backRes)
			{
				GetMatches();
				uint32 numDistancePairs = m_MatchDistances[0];
				if(numDistancePairs == 0)
					return 1;
				uint32 lenMain = m_MatchDistances[(size_t)numDistancePairs - 1];
				backRes = m_MatchDistances[numDistancePairs];
				MovePos(lenMain - 1);
				return lenMain;
			}

			void CTables::InitStructures()
			{
				uint32 i;
				for(i = 0; i < 256; i++)
					litLenLevels[i] = 8;
				litLenLevels[i++] = 13;
				for(; i < kFixedMainTableSize; i++)
					litLenLevels[i] = 5;
				for(i = 0; i < kFixedDistTableSize; i++)
					distLevels[i] = 5;
			}

			NO_INLINE void CCoder::LevelTableDummy(const Byte * levels, unsigned numLevels, uint32 * freqs)
			{
				uint prevLen = 0xFF;
				uint nextLen = levels[0];
				uint count = 0;
				uint maxCount = 7;
				uint minCount = 4;
				if(nextLen == 0) {
					maxCount = 138;
					minCount = 3;
				}
				for(uint n = 0; n < numLevels; n++) {
					unsigned curLen = nextLen;
					nextLen = (n < numLevels - 1) ? levels[(size_t)n + 1] : 0xFF;
					count++;
					if(count < maxCount && curLen == nextLen)
						continue;

					if(count < minCount)
						freqs[curLen] += (uint32)count;
					else if(curLen != 0) {
						if(curLen != prevLen) {
							freqs[curLen]++;
							count--;
						}
						freqs[kTableLevelRepNumber]++;
					}
					else if(count <= 10)
						freqs[kTableLevel0Number]++;
					else
						freqs[kTableLevel0Number2]++;
					count = 0;
					prevLen = curLen;
					if(nextLen == 0) {
						maxCount = 138;
						minCount = 3;
					}
					else if(curLen == nextLen) {
						maxCount = 6;
						minCount = 3;
					}
					else {
						maxCount = 7;
						minCount = 4;
					}
				}
			}

			NO_INLINE void CCoder::WriteBits(uint32 value, unsigned numBits)
			{
				m_OutStream.WriteBits(value, numBits);
			}

			#define WRITE_HF2(codes, lens, i) m_OutStream.WriteBits(codes[i], lens[i])
			#define WRITE_HF(i) WriteBits(codes[i], lens[i])

			NO_INLINE void CCoder::LevelTableCode(const Byte * levels, unsigned numLevels, const Byte * lens, const uint32 * codes)
			{
				uint prevLen = 0xFF;
				uint nextLen = levels[0];
				uint count = 0;
				uint maxCount = 7;
				uint minCount = 4;
				if(nextLen == 0) {
					maxCount = 138;
					minCount = 3;
				}
				for(uint n = 0; n < numLevels; n++) {
					uint curLen = nextLen;
					nextLen = (n < numLevels - 1) ? levels[(size_t)n + 1] : 0xFF;
					count++;
					if(count < maxCount && curLen == nextLen)
						continue;

					if(count < minCount)
						for(uint i = 0; i < count; i++)
							WRITE_HF(curLen);
					else if(curLen != 0) {
						if(curLen != prevLen) {
							WRITE_HF(curLen);
							count--;
						}
						WRITE_HF(kTableLevelRepNumber);
						WriteBits(count - 3, 2);
					}
					else if(count <= 10) {
						WRITE_HF(kTableLevel0Number);
						WriteBits(count - 3, 3);
					}
					else {
						WRITE_HF(kTableLevel0Number2);
						WriteBits(count - 11, 7);
					}

					count = 0;
					prevLen = curLen;

					if(nextLen == 0) {
						maxCount = 138;
						minCount = 3;
					}
					else if(curLen == nextLen) {
						maxCount = 6;
						minCount = 3;
					}
					else {
						maxCount = 7;
						minCount = 4;
					}
				}
			}

			NO_INLINE void CCoder::MakeTables(unsigned maxHuffLen)
			{
				Huffman_Generate(mainFreqs, mainCodes, m_NewLevels.litLenLevels, kFixedMainTableSize, maxHuffLen);
				Huffman_Generate(distFreqs, distCodes, m_NewLevels.distLevels, kDistTableSize64, maxHuffLen);
			}

			NO_INLINE uint32 Huffman_GetPrice(const uint32 * freqs, const Byte * lens, uint32 num)
			{
				uint32 price = 0;
				for(uint32 i = 0; i < num; i++)
					price += lens[i] * freqs[i];
				return price;
			}

			NO_INLINE uint32 Huffman_GetPrice_Spec(const uint32 * freqs, const Byte * lens, uint32 num, const Byte * extraBits, uint32 extraBase)
			{
				return Huffman_GetPrice(freqs, lens, num) +
					   Huffman_GetPrice(freqs + extraBase, extraBits, num - extraBase);
			}

			NO_INLINE uint32 CCoder::GetLzBlockPrice() const
			{
				return
					Huffman_GetPrice_Spec(mainFreqs, m_NewLevels.litLenLevels, kFixedMainTableSize, m_LenDirectBits, kSymbolMatch) +
					Huffman_GetPrice_Spec(distFreqs, m_NewLevels.distLevels, kDistTableSize64, kDistDirectBits, 0);
			}

			NO_INLINE void CCoder::TryBlock()
			{
				memzero(mainFreqs, sizeof(mainFreqs));
				memzero(distFreqs, sizeof(distFreqs));
				m_ValueIndex = 0;
				uint32 blockSize = BlockSizeRes;
				BlockSizeRes = 0;
				for(;; ) {
					if(m_OptimumCurrentIndex == m_OptimumEndIndex) {
						if(m_Pos >= kMatchArrayLimit || BlockSizeRes >= blockSize || !m_SecondPass &&
										((Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) ==
												0) || m_ValueIndex >= m_ValueBlockSize))
							break;
					}
					uint32 pos;
					uint32 len;
					if(_fastMode)
						len = GetOptimalFast(pos);
					else
						len = GetOptimal(pos);
					CCodeValue &codeValue = m_Values[m_ValueIndex++];
					if(len >= kMatchMinLen) {
						uint32 newLen = len - kMatchMinLen;
						codeValue.Len = (uint16)newLen;
						mainFreqs[kSymbolMatch + (size_t)g_LenSlots[newLen]]++;
						codeValue.Pos = (uint16)pos;
						distFreqs[GetPosSlot(pos)]++;
					}
					else {
						Byte b = *(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - m_AdditionalOffset);
						mainFreqs[b]++;
						codeValue.SetAsLiteral();
						codeValue.Pos = b;
					}
					m_AdditionalOffset -= len;
					BlockSizeRes += len;
				}
				mainFreqs[kSymbolEndOfBlock]++;
				m_AdditionalOffset += BlockSizeRes;
				m_SecondPass = true;
			}

			NO_INLINE void CCoder::SetPrices(const CLevels &levels)
			{
				if(!_fastMode) {
					uint32 i;
					for(i = 0; i < 256; i++) {
						Byte price = levels.litLenLevels[i];
						m_LiteralPrices[i] = ((price != 0) ? price : kNoLiteralStatPrice);
					}
					for(i = 0; i < m_NumLenCombinations; i++) {
						uint32 slot = g_LenSlots[i];
						Byte price = levels.litLenLevels[kSymbolMatch + (size_t)slot];
						m_LenPrices[i] = (Byte)(((price != 0) ? price : kNoLenStatPrice) + m_LenDirectBits[slot]);
					}
					for(i = 0; i < kDistTableSize64; i++) {
						Byte price = levels.distLevels[i];
						m_PosPrices[i] = (Byte)(((price != 0) ? price : kNoPosStatPrice) + kDistDirectBits[i]);
					}
				}
			}

			NO_INLINE void Huffman_ReverseBits(uint32 * codes, const Byte * lens, uint32 num)
			{
				for(uint32 i = 0; i < num; i++) {
					uint32 x = codes[i];
					x = ((x & 0x5555) << 1) | ((x & 0xAAAA) >> 1);
					x = ((x & 0x3333) << 2) | ((x & 0xCCCC) >> 2);
					x = ((x & 0x0F0F) << 4) | ((x & 0xF0F0) >> 4);
					codes[i] = (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8)) >> (16 - lens[i]);
				}
			}

			NO_INLINE void CCoder::WriteBlock()
			{
				Huffman_ReverseBits(mainCodes, m_NewLevels.litLenLevels, kFixedMainTableSize);
				Huffman_ReverseBits(distCodes, m_NewLevels.distLevels, kDistTableSize64);
				for(uint32 i = 0; i < m_ValueIndex; i++) {
					const CCodeValue &codeValue = m_Values[i];
					if(codeValue.IsLiteral())
						WRITE_HF2(mainCodes, m_NewLevels.litLenLevels, codeValue.Pos);
					else {
						uint32 len = codeValue.Len;
						uint32 lenSlot = g_LenSlots[len];
						WRITE_HF2(mainCodes, m_NewLevels.litLenLevels, kSymbolMatch + lenSlot);
						m_OutStream.WriteBits(len - m_LenStart[lenSlot], m_LenDirectBits[lenSlot]);
						uint32 dist = codeValue.Pos;
						uint32 posSlot = GetPosSlot(dist);
						WRITE_HF2(distCodes, m_NewLevels.distLevels, posSlot);
						m_OutStream.WriteBits(dist - kDistStart[posSlot], kDistDirectBits[posSlot]);
					}
				}
				WRITE_HF2(mainCodes, m_NewLevels.litLenLevels, kSymbolEndOfBlock);
			}

			static uint32 GetStorePrice(uint32 blockSize, unsigned bitPosition)
			{
				uint32 price = 0;
				do {
					uint32 nextBitPosition = (bitPosition + kFinalBlockFieldSize + kBlockTypeFieldSize) & 7;
					unsigned numBitsForAlign = nextBitPosition > 0 ? (8 - nextBitPosition) : 0;
					uint32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
					price += kFinalBlockFieldSize + kBlockTypeFieldSize + numBitsForAlign + (2 + 2) * 8 + curBlockSize * 8;
					bitPosition = 0;
					blockSize -= curBlockSize;
				} while(blockSize != 0);
				return price;
			}

			void CCoder::WriteStoreBlock(uint32 blockSize, uint32 additionalOffset, bool finalBlock)
			{
				do {
					uint32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
					blockSize -= curBlockSize;
					WriteBits((finalBlock && (blockSize == 0) ? NFinalBlockField::kFinalBlock : NFinalBlockField::kNotFinalBlock),
									kFinalBlockFieldSize);
					WriteBits(NBlockType::kStored, kBlockTypeFieldSize);
					m_OutStream.FlushByte();
					WriteBits((uint16)curBlockSize, kStoredBlockLengthFieldSize);
					WriteBits((uint16) ~curBlockSize, kStoredBlockLengthFieldSize);
					const Byte * data = Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow)- additionalOffset;
					for(uint32 i = 0; i < curBlockSize; i++)
						m_OutStream.WriteByte(data[i]);
					additionalOffset -= curBlockSize;
				} while(blockSize != 0);
			}

			NO_INLINE uint32 CCoder::TryDynBlock(unsigned tableIndex, uint32 numPasses)
			{
				CTables &t = m_Tables[tableIndex];
				BlockSizeRes = t.BlockSizeRes;
				uint32 posTemp = t.m_Pos;
				SetPrices(t);
				for(uint32 p = 0; p < numPasses; p++) {
					m_Pos = posTemp;
					TryBlock();
					uint numHuffBits = (m_ValueIndex > 18000 ? 12 : (m_ValueIndex >  7000 ? 11 : (m_ValueIndex >  2000 ? 10 : 9)));
					MakeTables(numHuffBits);
					SetPrices(m_NewLevels);
				}
				(CLevels &)t = m_NewLevels;
				m_NumLitLenLevels = kMainTableSize;
				while(m_NumLitLenLevels > kNumLitLenCodesMin && m_NewLevels.litLenLevels[(size_t)m_NumLitLenLevels - 1] == 0)
					m_NumLitLenLevels--;
				m_NumDistLevels = kDistTableSize64;
				while(m_NumDistLevels > kNumDistCodesMin && m_NewLevels.distLevels[(size_t)m_NumDistLevels - 1] == 0)
					m_NumDistLevels--;
				uint32 levelFreqs[kLevelTableSize];
				memzero(levelFreqs, sizeof(levelFreqs));
				LevelTableDummy(m_NewLevels.litLenLevels, m_NumLitLenLevels, levelFreqs);
				LevelTableDummy(m_NewLevels.distLevels, m_NumDistLevels, levelFreqs);
				Huffman_Generate(levelFreqs, levelCodes, levelLens, kLevelTableSize, kMaxLevelBitLength);
				m_NumLevelCodes = kNumLevelCodesMin;
				for(uint32 i = 0; i < kLevelTableSize; i++) {
					Byte level = levelLens[kCodeLengthAlphabetOrder[i]];
					if(level > 0 && i >= m_NumLevelCodes)
						m_NumLevelCodes = i + 1;
					m_LevelLevels[i] = level;
				}
				return GetLzBlockPrice() + Huffman_GetPrice_Spec(levelFreqs, levelLens, kLevelTableSize, kLevelDirectBits, kTableDirectLevels) +
					kNumLenCodesFieldSize + kNumDistCodesFieldSize + kNumLevelCodesFieldSize + m_NumLevelCodes * kLevelFieldSize + kFinalBlockFieldSize + kBlockTypeFieldSize;
			}

			NO_INLINE uint32 CCoder::TryFixedBlock(unsigned tableIndex)
			{
				CTables &t = m_Tables[tableIndex];
				BlockSizeRes = t.BlockSizeRes;
				m_Pos = t.m_Pos;
				m_NewLevels.SetFixedLevels();
				SetPrices(m_NewLevels);
				TryBlock();
				return kFinalBlockFieldSize + kBlockTypeFieldSize + GetLzBlockPrice();
			}

			NO_INLINE uint32 CCoder::GetBlockPrice(unsigned tableIndex, unsigned numDivPasses)
			{
				CTables &t = m_Tables[tableIndex];
				t.StaticMode = false;
				uint32 price = TryDynBlock(tableIndex, m_NumPasses);
				t.BlockSizeRes = BlockSizeRes;
				uint32 numValues = m_ValueIndex;
				uint32 posTemp = m_Pos;
				uint32 additionalOffsetEnd = m_AdditionalOffset;
				if(m_CheckStatic && m_ValueIndex <= kFixedHuffmanCodeBlockSizeMax) {
					const uint32 fixedPrice = TryFixedBlock(tableIndex);
					t.StaticMode = (fixedPrice < price);
					if(t.StaticMode)
						price = fixedPrice;
				}
				const uint32 storePrice = GetStorePrice(BlockSizeRes, 0); // bitPosition
				t.StoreMode = (storePrice <= price);
				if(t.StoreMode)
					price = storePrice;
				t.UseSubBlocks = false;
				if(numDivPasses > 1 && numValues >= kDivideCodeBlockSizeMin) {
					CTables &t0 = m_Tables[(tableIndex << 1)];
					(CLevels &)t0 = t;
					t0.BlockSizeRes = t.BlockSizeRes >> 1;
					t0.m_Pos = t.m_Pos;
					uint32 subPrice = GetBlockPrice((tableIndex << 1), numDivPasses - 1);
					uint32 blockSize2 = t.BlockSizeRes - t0.BlockSizeRes;
					if(t0.BlockSizeRes >= kDivideBlockSizeMin && blockSize2 >= kDivideBlockSizeMin) {
						CTables &t1 = m_Tables[(tableIndex << 1) + 1];
						(CLevels &)t1 = t;
						t1.BlockSizeRes = blockSize2;
						t1.m_Pos = m_Pos;
						m_AdditionalOffset -= t0.BlockSizeRes;
						subPrice += GetBlockPrice((tableIndex << 1) + 1, numDivPasses - 1);
						t.UseSubBlocks = (subPrice < price);
						if(t.UseSubBlocks)
							price = subPrice;
					}
				}
				m_AdditionalOffset = additionalOffsetEnd;
				m_Pos = posTemp;
				return price;
			}

			void CCoder::CodeBlock(unsigned tableIndex, bool finalBlock)
			{
				CTables &t = m_Tables[tableIndex];
				if(t.UseSubBlocks) {
					CodeBlock((tableIndex << 1), false);
					CodeBlock((tableIndex << 1) + 1, finalBlock);
				}
				else {
					if(t.StoreMode)
						WriteStoreBlock(t.BlockSizeRes, m_AdditionalOffset, finalBlock);
					else {
						WriteBits((finalBlock ? NFinalBlockField::kFinalBlock : NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
						if(t.StaticMode) {
							WriteBits(NBlockType::kFixedHuffman, kBlockTypeFieldSize);
							TryFixedBlock(tableIndex);
							uint i;
							const uint kMaxStaticHuffLen = 9;
							for(i = 0; i < kFixedMainTableSize; i++)
								mainFreqs[i] = (uint32)1 << (kMaxStaticHuffLen - m_NewLevels.litLenLevels[i]);
							for(i = 0; i < kFixedDistTableSize; i++)
								distFreqs[i] = (uint32)1 << (kMaxStaticHuffLen - m_NewLevels.distLevels[i]);
							MakeTables(kMaxStaticHuffLen);
						}
						else {
							if(m_NumDivPasses > 1 || m_CheckStatic)
								TryDynBlock(tableIndex, 1);
							WriteBits(NBlockType::kDynamicHuffman, kBlockTypeFieldSize);
							WriteBits(m_NumLitLenLevels - kNumLitLenCodesMin, kNumLenCodesFieldSize);
							WriteBits(m_NumDistLevels - kNumDistCodesMin, kNumDistCodesFieldSize);
							WriteBits(m_NumLevelCodes - kNumLevelCodesMin, kNumLevelCodesFieldSize);
							for(uint32 i = 0; i < m_NumLevelCodes; i++)
								WriteBits(m_LevelLevels[i], kLevelFieldSize);
							Huffman_ReverseBits(levelCodes, levelLens, kLevelTableSize);
							LevelTableCode(m_NewLevels.litLenLevels, m_NumLitLenLevels, levelLens, levelCodes);
							LevelTableCode(m_NewLevels.distLevels, m_NumDistLevels, levelLens, levelCodes);
						}
						WriteBlock();
					}
					m_AdditionalOffset -= t.BlockSizeRes;
				}
			}

			HRESULT CCoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			 const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
			{
				m_CheckStatic = (m_NumPasses != 1 || m_NumDivPasses != 1);
				m_IsMultiPass = (m_CheckStatic || (m_NumPasses != 1 || m_NumDivPasses != 1));
				RINOK(Create());
				m_ValueBlockSize = (7 << 10) + (1 << 12) * m_NumDivPasses;
				uint64 nowPos = 0;
				CSeqInStreamWrap _seqInStream;
				_seqInStream.Init(inStream);
				_lzInWindow.stream = &_seqInStream.vt;
				MatchFinder_Init(&_lzInWindow);
				m_OutStream.SetStream(outStream);
				m_OutStream.Init();
				m_OptimumEndIndex = m_OptimumCurrentIndex = 0;
				CTables &t = m_Tables[1];
				t.m_Pos = 0;
				t.InitStructures();
				m_AdditionalOffset = 0;
				do {
					t.BlockSizeRes = kBlockUncompressedSizeThreshold;
					m_SecondPass = false;
					GetBlockPrice(1, m_NumDivPasses);
					CodeBlock(1, Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) == 0);
					nowPos += m_Tables[1].BlockSizeRes;
					if(progress != NULL) {
						uint64 packSize = m_OutStream.GetProcessedSize();
						RINOK(progress->SetRatioInfo(&nowPos, &packSize));
					}
				} while(Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) != 0);
				return (_lzInWindow.result != SZ_OK) ? SResToHRESULT(_lzInWindow.result) : m_OutStream.Flush();
			}

			HRESULT CCoder::BaseCode(ISequentialInStream * inStream, ISequentialOutStream * outStream,
				const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
				catch(const COutBufferException &e) { return e.ErrorCode; }
				catch(...) { return E_FAIL; }
			}

			STDMETHODIMP CCOMCoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
				const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				return BaseCode(inStream, outStream, inSize, outSize, progress);
			}

			STDMETHODIMP CCOMCoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps)
			{
				return BaseSetEncoderProperties2(propIDs, props, numProps);
			}

			STDMETHODIMP CCOMCoder64::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
				const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				return BaseCode(inStream, outStream, inSize, outSize, progress);
			}

			STDMETHODIMP CCOMCoder64::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * props, uint32 numProps)
			{
				return BaseSetEncoderProperties2(propIDs, props, numProps);
			}
		}
		namespace NDecoder {
			CCoder::CCoder(bool deflate64Mode) : _deflate64Mode(deflate64Mode), _deflateNSIS(false),
				_keepHistory(false), _needFinishInput(false), _needInitInStream(true), _outSizeDefined(false),
				_outStartPos(0), ZlibMode(false) 
			{
			}
			uint32 CCoder::ReadBits(uint numBits)
			{
				return m_InBitStream.ReadBits(numBits);
			}
			Byte CCoder::ReadAlignedByte()
			{
				return m_InBitStream.ReadAlignedByte();
			}
			bool CCoder::DecodeLevels(Byte * levels, unsigned numSymbols)
			{
				uint   i = 0;
				do {
					uint32 sym = m_LevelDecoder.Decode(&m_InBitStream);
					if(sym < kTableDirectLevels)
						levels[i++] = (Byte)sym;
					else {
						if(sym >= kLevelTableSize)
							return false;
						uint   num;
						uint   numBits;
						Byte   symbol;
						if(sym == kTableLevelRepNumber) {
							if(i == 0)
								return false;
							numBits = 2;
							num = 0;
							symbol = levels[(size_t)i - 1];
						}
						else {
							sym -= kTableLevel0Number;
							sym <<= 2;
							numBits = 3 + (uint)sym;
							num = ((uint)sym << 1);
							symbol = 0;
						}
						num += i + 3 + ReadBits(numBits);
						if(num > numSymbols)
							return false;
						do {
							levels[i++] = symbol;
						} while(i < num);
					}
				} while(i < numSymbols);
				return true;
			}

			#define RIF(x) { if(!(x)) return false;	}

			bool CCoder::ReadTables(void)
			{
				m_FinalBlock = (ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);
				if(m_InBitStream.ExtraBitsWereRead())
					return false;
				uint32 blockType = ReadBits(kBlockTypeFieldSize);
				if(blockType > NBlockType::kDynamicHuffman)
					return false;
				if(m_InBitStream.ExtraBitsWereRead())
					return false;
				if(blockType == NBlockType::kStored) {
					m_StoredMode = true;
					m_InBitStream.AlignToByte();
					m_StoredBlockSize = ReadAligned_UInt16(); // ReadBits(kStoredBlockLengthFieldSize)
					return _deflateNSIS ? true : (m_StoredBlockSize == (uint16) ~ReadAligned_UInt16());
				}
				m_StoredMode = false;
				CLevels levels;
				if(blockType == NBlockType::kFixedHuffman) {
					levels.SetFixedLevels();
					_numDistLevels = _deflate64Mode ? kDistTableSize64 : kDistTableSize32;
				}
				else {
					unsigned numLitLenLevels = ReadBits(kNumLenCodesFieldSize) + kNumLitLenCodesMin;
					_numDistLevels = ReadBits(kNumDistCodesFieldSize) + kNumDistCodesMin;
					unsigned numLevelCodes = ReadBits(kNumLevelCodesFieldSize) + kNumLevelCodesMin;
					if(!_deflate64Mode)
						if(_numDistLevels > kDistTableSize32)
							return false;
					Byte levelLevels[kLevelTableSize];
					for(uint i = 0; i < kLevelTableSize; i++) {
						unsigned position = kCodeLengthAlphabetOrder[i];
						if(i < numLevelCodes)
							levelLevels[position] = (Byte)ReadBits(kLevelFieldSize);
						else
							levelLevels[position] = 0;
					}
					if(m_InBitStream.ExtraBitsWereRead())
						return false;
					RIF(m_LevelDecoder.Build(levelLevels));
					Byte tmpLevels[kFixedMainTableSize + kFixedDistTableSize];
					if(!DecodeLevels(tmpLevels, numLitLenLevels + _numDistLevels))
						return false;
					if(m_InBitStream.ExtraBitsWereRead())
						return false;
					levels.SubClear();
					memcpy(levels.litLenLevels, tmpLevels, numLitLenLevels);
					memcpy(levels.distLevels, tmpLevels + numLitLenLevels, _numDistLevels);
				}
				RIF(m_MainDecoder.Build(levels.litLenLevels));
				return m_DistDecoder.Build(levels.distLevels);
			}

			HRESULT CCoder::InitInStream(bool needInit)
			{
				if(needInit) {
					// for HDD-Windows:
					// (1 << 15) - best for reading only prefetch
					// (1 << 22) - best for real reading / writing
					if(!m_InBitStream.Create(1 << 20))
						return E_OUTOFMEMORY;
					m_InBitStream.Init();
					_needInitInStream = false;
				}
				return S_OK;
			}

			HRESULT CCoder::CodeSpec(uint32 curSize, bool finishInputStream, uint32 inputProgressLimit)
			{
				if(_remainLen == kLenIdFinished)
					return S_OK;
				if(_remainLen == kLenIdNeedInit) {
					if(!_keepHistory)
						if(!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64 : kHistorySize32))
							return E_OUTOFMEMORY;
					RINOK(InitInStream(_needInitInStream));
					m_OutWindowStream.Init(_keepHistory);
					m_FinalBlock = false;
					_remainLen = 0;
					_needReadTable = true;
				}
				while(_remainLen > 0 && curSize > 0) {
					_remainLen--;
					Byte b = m_OutWindowStream.GetByte(_rep0);
					m_OutWindowStream.PutByte(b);
					curSize--;
				}
				uint64 inputStart = 0;
				if(inputProgressLimit != 0)
					inputStart = m_InBitStream.GetProcessedSize();
				while(curSize > 0 || finishInputStream) {
					if(m_InBitStream.ExtraBitsWereRead())
						return S_FALSE;
					if(_needReadTable) {
						if(m_FinalBlock) {
							_remainLen = kLenIdFinished;
							break;
						}
						if(inputProgressLimit != 0)
							if(m_InBitStream.GetProcessedSize() - inputStart >= inputProgressLimit)
								return S_OK;
						if(!ReadTables())
							return S_FALSE;
						if(m_InBitStream.ExtraBitsWereRead())
							return S_FALSE;
						_needReadTable = false;
					}
					if(m_StoredMode) {
						if(finishInputStream && curSize == 0 && m_StoredBlockSize != 0)
							return S_FALSE;
						/* NSIS version contains some bits in bitl bits buffer.
						   So we must read some first bytes via ReadAlignedByte */
						for(;m_StoredBlockSize > 0 && curSize > 0 && m_InBitStream.ThereAreDataInBitsBuffer(); m_StoredBlockSize--, curSize--)
							m_OutWindowStream.PutByte(ReadAlignedByte());
						for(; m_StoredBlockSize > 0 && curSize > 0; m_StoredBlockSize--, curSize--)
							m_OutWindowStream.PutByte(m_InBitStream.ReadDirectByte());
						_needReadTable = (m_StoredBlockSize == 0);
						continue;
					}
					while(curSize > 0) {
						if(m_InBitStream.ExtraBitsWereRead_Fast())
							return S_FALSE;
						uint32 sym = m_MainDecoder.Decode(&m_InBitStream);
						if(sym < 0x100) {
							m_OutWindowStream.PutByte((Byte)sym);
							curSize--;
							continue;
						}
						else if(sym == kSymbolEndOfBlock) {
							_needReadTable = true;
							break;
						}
						else if(sym < kMainTableSize) {
							sym -= kSymbolMatch;
							uint32 len;
							{
								unsigned numBits;
								if(_deflate64Mode) {
									len = kLenStart64[sym];
									numBits = kLenDirectBits64[sym];
								}
								else {
									len = kLenStart32[sym];
									numBits = kLenDirectBits32[sym];
								}
								len += kMatchMinLen + m_InBitStream.ReadBits(numBits);
							}
							uint32 locLen = len;
							if(locLen > curSize)
								locLen = (uint32)curSize;
							sym = m_DistDecoder.Decode(&m_InBitStream);
							if(sym >= _numDistLevels)
								return S_FALSE;
							uint32 distance = kDistStart[sym] + m_InBitStream.ReadBits(kDistDirectBits[sym]);
							if(!m_OutWindowStream.CopyBlock(distance, locLen))
								return S_FALSE;
							curSize -= locLen;
							len -= locLen;
							if(len != 0) {
								_remainLen = (int32)len;
								_rep0 = distance;
								break;
							}
						}
						else
							return S_FALSE;
					}
					if(finishInputStream && curSize == 0) {
						if(m_MainDecoder.Decode(&m_InBitStream) != kSymbolEndOfBlock)
							return S_FALSE;
						_needReadTable = true;
					}
				}
				if(m_InBitStream.ExtraBitsWereRead())
					return S_FALSE;
				return S_OK;
			}

			#ifdef _NO_EXCEPTIONS
				#define DEFLATE_TRY_BEGIN
				#define DEFLATE_TRY_END(res)
			#else
				#define DEFLATE_TRY_BEGIN try {
				#define DEFLATE_TRY_END(res) } catch(const CSystemException &e) { res = e.ErrorCode; }	catch(...) { res = S_FALSE; }
				// catch(const CInBufferException &e)  { res = e.ErrorCode; }
				// catch(const CLzOutWindowException &e)  { res = e.ErrorCode; }
			#endif

			CCoder::CCoderReleaser::CCoderReleaser(CCoder * coder) : _coder(coder), NeedFlush(true) 
			{
			}

			CCoder::CCoderReleaser::~CCoderReleaser()
			{
				if(NeedFlush)
					_coder->Flush();
			}

			HRESULT CCoder::CodeReal(ISequentialOutStream * outStream, ICompressProgressInfo * progress)
			{
				HRESULT res;
				DEFLATE_TRY_BEGIN
				m_OutWindowStream.SetStream(outStream);
				CCoderReleaser flusher(this);
				const uint64 inStart = _needInitInStream ? 0 : m_InBitStream.GetProcessedSize();
				for(;; ) {
					const uint32 kInputProgressLimit = 1 << 21;
					uint32 curSize = 1 << 20;
					bool finishInputStream = false;
					if(_outSizeDefined) {
						const uint64 rem = _outSize - GetOutProcessedCur();
						if(curSize >= rem) {
							curSize = (uint32)rem;
							if(ZlibMode || _needFinishInput)
								finishInputStream = true;
						}
					}
					if(!finishInputStream && curSize == 0)
						break;

					RINOK(CodeSpec(curSize, finishInputStream, progress ? kInputProgressLimit : 0));
					if(_remainLen == kLenIdFinished)
						break;
					if(progress) {
						const uint64 inSize = m_InBitStream.GetProcessedSize() - inStart;
						const uint64 nowPos64 = GetOutProcessedCur();
						RINOK(progress->SetRatioInfo(&inSize, &nowPos64));
					}
				}
				if(_remainLen == kLenIdFinished && ZlibMode) {
					m_InBitStream.AlignToByte();
					for(uint i = 0; i < 4; i++)
						ZlibFooter[i] = ReadAlignedByte();
				}
				flusher.NeedFlush = false;
				res = Flush();
				if(res == S_OK && _remainLen != kLenIdNeedInit && InputEofError())
					return S_FALSE;
				DEFLATE_TRY_END(res)
				return res;
			}

			HRESULT CCoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
				const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				SetInStream(inStream);
				SetOutStreamSize(outSize);
				HRESULT res = CodeReal(outStream, progress);
				ReleaseInStream();
				/*
				   if(res == S_OK)
				   if(_needFinishInput && inSize && *inSize != m_InBitStream.GetProcessedSize())
					res = S_FALSE;
				 */
				return res;
			}

			STDMETHODIMP CCoder::SetFinishMode(uint32 finishMode)
			{
				Set_NeedFinishInput(finishMode != 0);
				return S_OK;
			}

			STDMETHODIMP CCoder::GetInStreamProcessedSize(uint64 * value)
			{
				if(!value)
					return E_INVALIDARG;
				*value = m_InBitStream.GetProcessedSize();
				return S_OK;
			}

			STDMETHODIMP CCoder::SetInStream(ISequentialInStream * inStream)
			{
				m_InStreamRef = inStream;
				m_InBitStream.SetStream(inStream);
				return S_OK;
			}

			STDMETHODIMP CCoder::ReleaseInStream()
			{
				m_InStreamRef.Release();
				return S_OK;
			}

			void CCoder::SetOutStreamSizeResume(const uint64 * outSize)
			{
				_outSizeDefined = (outSize != NULL);
				_outSize = 0;
				if(_outSizeDefined)
					_outSize = *outSize;
				m_OutWindowStream.Init(_keepHistory);
				_outStartPos = m_OutWindowStream.GetProcessedSize();
				_remainLen = kLenIdNeedInit;
			}

			STDMETHODIMP CCoder::SetOutStreamSize(const uint64 * outSize)
			{
				_needInitInStream = true;
				SetOutStreamSizeResume(outSize);
				return S_OK;
			}

			#ifndef NO_READ_FROM_CODER
				STDMETHODIMP CCoder::Read(void * data, uint32 size, uint32 * processedSize)
				{
					HRESULT res;
					ASSIGN_PTR(processedSize, 0);
					const uint64 outPos = GetOutProcessedCur();
					bool finishInputStream = false;
					if(_outSizeDefined) {
						const uint64 rem = _outSize - outPos;
						if(size >= rem) {
							size = (uint32)rem;
							if(ZlibMode || _needFinishInput)
								finishInputStream = true;
						}
					}
					if(!finishInputStream && size == 0)
						return S_OK;
					DEFLATE_TRY_BEGIN
					m_OutWindowStream.SetMemStream((Byte*)data);
					res = CodeSpec(size, finishInputStream);
					DEFLATE_TRY_END(res)
					{
						HRESULT res2 = Flush();
						if(res2 != S_OK)
							res = res2;
					}
					ASSIGN_PTR(processedSize, (uint32)(GetOutProcessedCur() - outPos));
					m_OutWindowStream.SetMemStream(NULL);
					return res;
				}
			#endif

			HRESULT CCoder::CodeResume(ISequentialOutStream * outStream, const uint64 * outSize, ICompressProgressInfo * progress)
			{
				SetOutStreamSizeResume(outSize);
				return CodeReal(outStream, progress);
			}
		}
		//
		REGISTER_CODEC_CREATE(CreateDec, NDecoder::CCOMCoder64())
		#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
			REGISTER_CODEC_CREATE(CreateEnc, NEncoder::CCOMCoder64())
		#else
			#define CreateEnc NULL
		#endif
		REGISTER_CODEC_2(Deflate64, CreateDec, CreateEnc, 0x40109, "Deflate64")
	}
}
