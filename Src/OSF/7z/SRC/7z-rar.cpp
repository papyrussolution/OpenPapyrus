// 7Z-RAR.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
//
//#include <FindSignature.h>
//HRESULT FindSignatureInStream(ISequentialInStream *stream, const Byte *signature, uint signatureSize, const uint64 *limit, uint64 &resPos);
//
// FindSignature.cpp
HRESULT FindSignatureInStream(ISequentialInStream * stream, const Byte * signature, uint signatureSize, const uint64 * limit, uint64 &resPos)
{
	resPos = 0;
	CByteBuffer byteBuffer2(signatureSize);
	RINOK(ReadStream_FALSE(stream, byteBuffer2, signatureSize));
	if(memcmp(byteBuffer2, signature, signatureSize) == 0)
		return S_OK;
	const uint32 kBufferSize = (1 << 16);
	CByteBuffer byteBuffer(kBufferSize);
	Byte * buffer = byteBuffer;
	uint32 numPrevBytes = signatureSize - 1;
	memcpy(buffer, (const Byte *)byteBuffer2 + 1, numPrevBytes);
	resPos = 1;
	for(;; ) {
		if(limit)
			if(resPos > *limit)
				return S_FALSE;
		do {
			uint32 numReadBytes = kBufferSize - numPrevBytes;
			uint32 processedSize;
			RINOK(stream->Read(buffer + numPrevBytes, numReadBytes, &processedSize));
			numPrevBytes += processedSize;
			if(processedSize == 0)
				return S_FALSE;
		} while(numPrevBytes < signatureSize);
		uint32 numTests = numPrevBytes - signatureSize + 1;
		for(uint32 pos = 0; pos < numTests; pos++) {
			Byte b = signature[0];
			for(; buffer[pos] != b && pos < numTests; pos++) ;
			if(pos == numTests)
				break;
			if(memcmp(buffer + pos, signature, signatureSize) == 0) {
				resPos += pos;
				return S_OK;
			}
		}
		resPos += numTests;
		numPrevBytes -= numTests;
		memmove(buffer, buffer + numTests, numPrevBytes);
	}
}
//
// Rar1Decoder.cpp Rar2Decoder.cpp Rar3Decoder.cpp Rar3Vm.cpp Rar5Decoder.cpp
//   According to unRAR license, this code may not be used to develop a program that creates RAR archives
namespace NCompress {
	namespace NRar1 {
		static const uint32 PosL1[] = {0, 0, 0, 2, 3, 5, 7, 11, 16, 20, 24, 32, 32, 256};
		static const uint32 PosL2[] = {0, 0, 0, 0, 5, 7, 9, 13, 18, 22, 26, 34, 36, 256};
		static const uint32 PosHf0[] = {0, 0, 0, 0, 0, 8, 16, 24, 33, 33, 33, 33, 33, 257};
		static const uint32 PosHf1[] = {0, 0, 0, 0, 0, 0, 4, 44, 60, 76, 80, 80, 127, 257};
		static const uint32 PosHf2[] = {0, 0, 0, 0, 0, 0, 2, 7, 53, 117, 233, 257, 0};
		static const uint32 PosHf3[] = {0, 0, 0, 0, 0, 0, 0, 2, 16, 218, 251, 257, 0};
		static const uint32 PosHf4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 257, 0, 0};

		static const uint32 kHistorySize = (1 << 16);

		/*
		   class CCoderReleaser
		   {
		   CDecoder *m_Coder;
		   public:
		   CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
		   ~CCoderReleaser() { m_Coder->ReleaseStreams(); }
		   };
		 */

		CDecoder::CDecoder() : m_IsSolid(false) 
		{
		}
		void CDecoder::InitStructures()
		{
			for(int i = 0; i < kNumRepDists; i++)
				m_RepDists[i] = 0;
			m_RepDistPtr = 0;
			LastLength = 0;
			LastDist = 0;
		}
		uint32 CDecoder::ReadBits(int numBits) { return m_InBitStream.ReadBits(numBits); }
		HRESULT CDecoder::CopyBlock(uint32 distance, uint32 len)
		{
			if(len == 0)
				return S_FALSE;
			m_UnpackSize -= len;
			return m_OutWindowStream.CopyBlock(distance, len) ? S_OK : S_FALSE;
		}
		uint32 CDecoder::DecodeNum(const uint32 * posTab)
		{
			uint32 startPos = 2;
			uint32 num = m_InBitStream.GetValue(12);
			for(;; ) {
				uint32 cur = (posTab[(size_t)startPos + 1] - posTab[startPos]) << (12 - startPos);
				if(num < cur)
					break;
				startPos++;
				num -= cur;
			}
			m_InBitStream.MovePos(startPos);
			return ((num >> (12 - startPos)) + posTab[startPos]);
		}

		static const Byte kShortLen1 [] = {1, 3, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6 };
		static const Byte kShortLen1a[] = {1, 4, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6, 4 };
		static const Byte kShortLen2 [] = {2, 3, 3, 3, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6 };
		static const Byte kShortLen2a[] = {2, 3, 3, 4, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6, 4 };
		static const uint32 kShortXor1[] = {0, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};
		static const uint32 kShortXor2[] = {0, 0x40, 0x60, 0xa0, 0xd0, 0xe0, 0xf0, 0xf8, 0xfc, 0xc0, 0x80, 0x90, 0x98, 0x9c, 0xb0};

		HRESULT CDecoder::ShortLZ()
		{
			uint32 len, saveLen, dist;
			int distancePlace;
			const Byte * kShortLen;
			const uint32 * kShortXor;
			NumHuf = 0;
			if(LCount == 2) {
				if(ReadBits(1))
					return CopyBlock(LastDist, LastLength);
				LCount = 0;
			}
			uint32 bitField = m_InBitStream.GetValue(8);
			if(AvrLn1 < 37) {
				kShortLen = Buf60 ? kShortLen1a : kShortLen1;
				kShortXor = kShortXor1;
			}
			else {
				kShortLen = Buf60 ? kShortLen2a : kShortLen2;
				kShortXor = kShortXor2;
			}
			for(len = 0; ((bitField ^ kShortXor[len]) & (~(0xff >> kShortLen[len]))) != 0; len++) 
				;
			m_InBitStream.MovePos(kShortLen[len]);
			if(len >= 9) {
				if(len == 9) {
					LCount++;
					return CopyBlock(LastDist, LastLength);
				}
				if(len == 14) {
					LCount = 0;
					len = DecodeNum(PosL2) + 5;
					dist = 0x8000 + ReadBits(15) - 1;
					LastLength = len;
					LastDist = dist;
					return CopyBlock(dist, len);
				}

				LCount = 0;
				saveLen = len;
				dist = m_RepDists[(m_RepDistPtr - (len - 9)) & 3];
				len = DecodeNum(PosL1) + 2;
				if(len == 0x101 && saveLen == 10) {
					Buf60 ^= 1;
					return S_OK;
				}
				if(dist >= 256)
					len++;
				if(dist >= MaxDist3 - 1)
					len++;
			}
			else {
				LCount = 0;
				AvrLn1 += len;
				AvrLn1 -= AvrLn1 >> 4;

				distancePlace = DecodeNum(PosHf2) & 0xff;
				dist = ChSetA[(uint)distancePlace];
				if(--distancePlace != -1) {
					PlaceA[dist]--;
					uint32 lastDistance = ChSetA[(uint)distancePlace];
					PlaceA[lastDistance]++;
					ChSetA[(size_t)(uint)distancePlace + 1] = lastDistance;
					ChSetA[(uint)distancePlace] = dist;
				}
				len += 2;
			}

			m_RepDists[m_RepDistPtr++] = dist;
			m_RepDistPtr &= 3;
			LastLength = len;
			LastDist = dist;
			return CopyBlock(dist, len);
		}

		HRESULT CDecoder::LongLZ()
		{
			uint32 len;
			uint32 dist;
			uint32 distancePlace, newDistancePlace;
			uint32 oldAvr2, oldAvr3;
			NumHuf = 0;
			Nlzb += 16;
			if(Nlzb > 0xff) {
				Nlzb = 0x90;
				Nhfb >>= 1;
			}
			oldAvr2 = AvrLn2;
			if(AvrLn2 >= 122)
				len = DecodeNum(PosL2);
			else if(AvrLn2 >= 64)
				len = DecodeNum(PosL1);
			else {
				uint32 bitField = m_InBitStream.GetValue(16);
				if(bitField < 0x100) {
					len = bitField;
					m_InBitStream.MovePos(16);
				}
				else {
					for(len = 0; ((bitField << len) & 0x8000) == 0; len++)
						;
					m_InBitStream.MovePos(len+1);
				}
			}

			AvrLn2 += len;
			AvrLn2 -= AvrLn2 >> 5;

			if(AvrPlcB > 0x28ff)
				distancePlace = DecodeNum(PosHf2);
			else if(AvrPlcB > 0x6ff)
				distancePlace = DecodeNum(PosHf1);
			else
				distancePlace = DecodeNum(PosHf0);

			AvrPlcB += distancePlace;
			AvrPlcB -= AvrPlcB >> 8;

			for(;; ) {
				dist = ChSetB[distancePlace & 0xff];
				newDistancePlace = NToPlB[dist++ & 0xff]++;
				if(!(dist & 0xff))
					CorrHuff(ChSetB, NToPlB);
				else
					break;
			}

			ChSetB[distancePlace] = ChSetB[newDistancePlace];
			ChSetB[newDistancePlace] = dist;

			dist = ((dist & 0xff00) >> 1) | ReadBits(7);

			oldAvr3 = AvrLn3;

			if(len != 1 && len != 4)
				if(len == 0 && dist <= MaxDist3) {
					AvrLn3++;
					AvrLn3 -= AvrLn3 >> 8;
				}
				else if(AvrLn3 > 0)
					AvrLn3--;

			len += 3;

			if(dist >= MaxDist3)
				len++;
			if(dist <= 256)
				len += 8;

			if(oldAvr3 > 0xb0 || AvrPlc >= 0x2a00 && oldAvr2 < 0x40)
				MaxDist3 = 0x7f00;
			else
				MaxDist3 = 0x2001;

			m_RepDists[m_RepDistPtr++] = --dist;
			m_RepDistPtr &= 3;
			LastLength = len;
			LastDist = dist;

			return CopyBlock(dist, len);
		}

		HRESULT CDecoder::HuffDecode()
		{
			uint32 curByte, newBytePlace;
			uint32 len;
			uint32 dist;
			int bytePlace;

			if(AvrPlc > 0x75ff) bytePlace = DecodeNum(PosHf4);
			else if(AvrPlc > 0x5dff) bytePlace = DecodeNum(PosHf3);
			else if(AvrPlc > 0x35ff) bytePlace = DecodeNum(PosHf2);
			else if(AvrPlc > 0x0dff) bytePlace = DecodeNum(PosHf1);
			else bytePlace = DecodeNum(PosHf0);

			if(StMode) {
				if(--bytePlace == -1) {
					if(ReadBits(1)) {
						NumHuf = StMode = 0;
						return S_OK;
					}
					else {
						len = (ReadBits(1)) ? 4 : 3;
						dist = DecodeNum(PosHf2);
						dist = (dist << 5) | ReadBits(5);
						return CopyBlock(dist - 1, len);
					}
				}
			}
			else if(NumHuf++ >= 16 && FlagsCnt == 0)
				StMode = 1;

			bytePlace &= 0xff;
			AvrPlc += bytePlace;
			AvrPlc -= AvrPlc >> 8;
			Nhfb += 16;

			if(Nhfb > 0xff) {
				Nhfb = 0x90;
				Nlzb >>= 1;
			}

			m_UnpackSize--;
			m_OutWindowStream.PutByte((Byte)(ChSet[bytePlace] >> 8));

			for(;; ) {
				curByte = ChSet[bytePlace];
				newBytePlace = NToPl[curByte++ & 0xff]++;
				if((curByte & 0xff) > 0xa1)
					CorrHuff(ChSet, NToPl);
				else
					break;
			}

			ChSet[bytePlace] = ChSet[newBytePlace];
			ChSet[newBytePlace] = curByte;
			return S_OK;
		}

		void CDecoder::GetFlagsBuf()
		{
			uint32 flags, newFlagsPlace;
			uint32 flagsPlace = DecodeNum(PosHf2);
			for(;; ) {
				flags = ChSetC[flagsPlace];
				FlagBuf = flags >> 8;
				newFlagsPlace = NToPlC[flags++ & 0xff]++;
				if((flags & 0xff) != 0)
					break;
				CorrHuff(ChSetC, NToPlC);
			}
			ChSetC[flagsPlace] = ChSetC[newFlagsPlace];
			ChSetC[newFlagsPlace] = flags;
		}

		void CDecoder::InitData()
		{
			if(!m_IsSolid) {
				AvrPlcB = AvrLn1 = AvrLn2 = AvrLn3 = NumHuf = Buf60 = 0;
				AvrPlc = 0x3500;
				MaxDist3 = 0x2001;
				Nhfb = Nlzb = 0x80;
			}
			FlagsCnt = 0;
			FlagBuf = 0;
			StMode = 0;
			LCount = 0;
		}

		void CDecoder::CorrHuff(uint32 * CharSet, uint32 * NumToPlace)
		{
			int i;
			for(i = 7; i >= 0; i--)
				for(int j = 0; j < 32; j++, CharSet++)
					*CharSet = (*CharSet & ~0xff) | i;
			memzero(NumToPlace, sizeof(NToPl));
			for(i = 6; i >= 0; i--)
				NumToPlace[i] = (7 - i) * 32;
		}

		void CDecoder::InitHuff()
		{
			for(uint32 i = 0; i < 256; i++) {
				Place[i] = PlaceA[i] = PlaceB[i] = i;
				PlaceC[i] = (~i + 1) & 0xff;
				ChSet[i] = ChSetB[i] = i << 8;
				ChSetA[i] = i;
				ChSetC[i] = ((~i + 1) & 0xff) << 8;
			}
			memzero(NToPl, sizeof(NToPl));
			memzero(NToPlB, sizeof(NToPlB));
			memzero(NToPlC, sizeof(NToPlC));
			CorrHuff(ChSetB, NToPlB);
		}

		HRESULT CDecoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * /* progress */)
		{
			if(!inSize || !outSize)
				return E_INVALIDARG;
			if(!m_OutWindowStream.Create(kHistorySize))
				return E_OUTOFMEMORY;
			if(!m_InBitStream.Create(1 << 20))
				return E_OUTOFMEMORY;
			m_UnpackSize = (int64)*outSize;
			m_OutWindowStream.SetStream(outStream);
			m_OutWindowStream.Init(m_IsSolid);
			m_InBitStream.SetStream(inStream);
			m_InBitStream.Init();
			// CCoderReleaser coderReleaser(this);
			InitData();
			if(!m_IsSolid) {
				InitStructures();
				InitHuff();
			}
			if(m_UnpackSize > 0) {
				GetFlagsBuf();
				FlagsCnt = 8;
			}
			while(m_UnpackSize > 0) {
				if(StMode) {
					RINOK(HuffDecode());
					continue;
				}
				if(--FlagsCnt < 0) {
					GetFlagsBuf();
					FlagsCnt = 7;
				}
				if(FlagBuf & 0x80) {
					FlagBuf <<= 1;
					if(Nlzb > Nhfb) {
						RINOK(LongLZ());
					}
					else {
						RINOK(HuffDecode());
					}
				}
				else {
					FlagBuf <<= 1;
					if(--FlagsCnt < 0) {
						GetFlagsBuf();
						FlagsCnt = 7;
					}
					if(FlagBuf & 0x80) {
						FlagBuf <<= 1;
						if(Nlzb > Nhfb) {
							RINOK(HuffDecode());
						}
						else {
							RINOK(LongLZ());
						}
					}
					else {
						FlagBuf <<= 1;
						RINOK(ShortLZ());
					}
				}
			}
			return (m_UnpackSize >= 0) ? m_OutWindowStream.Flush() : S_FALSE;
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(const CLzOutWindowException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			if(size < 1)
				return E_INVALIDARG;
			m_IsSolid = ((data[0] & 1) != 0);
			return S_OK;
		}
	}
	namespace NRar2 {
		namespace NMultimedia {
			Byte CFilter::Decode(int &channelDelta, Byte deltaByte)
			{
				D4 = D3;
				D3 = D2;
				D2 = LastDelta - D1;
				D1 = LastDelta;
				int predictedValue = ((8 * LastChar + K1 * D1 + K2 * D2 + K3 * D3 + K4 * D4 + K5 * channelDelta) >> 3);
				Byte realValue = (Byte)(predictedValue - deltaByte);
				{
					int i = ((int)(signed char)deltaByte) << 3;
					Dif[0] += abs(i);
					Dif[1] += abs(i - D1);
					Dif[2] += abs(i + D1);
					Dif[3] += abs(i - D2);
					Dif[4] += abs(i + D2);
					Dif[5] += abs(i - D3);
					Dif[6] += abs(i + D3);
					Dif[7] += abs(i - D4);
					Dif[8] += abs(i + D4);
					Dif[9] += abs(i - channelDelta);
					Dif[10] += abs(i + channelDelta);
				}
				channelDelta = LastDelta = (signed char)(realValue - LastChar);
				LastChar = realValue;
				if(((++ByteCount) & 0x1F) == 0) {
					uint32 minDif = Dif[0];
					uint32 numMinDif = 0;
					Dif[0] = 0;
					for(uint i = 1; i < ARRAY_SIZE(Dif); i++) {
						if(Dif[i] < minDif) {
							minDif = Dif[i];
							numMinDif = i;
						}
						Dif[i] = 0;
					}
					switch(numMinDif) {
						case 1: if(K1 >= -16) K1--; break;
						case 2: if(K1 <   16) K1++; break;
						case 3: if(K2 >= -16) K2--; break;
						case 4: if(K2 <   16) K2++; break;
						case 5: if(K3 >= -16) K3--; break;
						case 6: if(K3 <   16) K3++; break;
						case 7: if(K4 >= -16) K4--; break;
						case 8: if(K4 <   16) K4++; break;
						case 9: if(K5 >= -16) K5--; break;
						case 10: if(K5 <   16) K5++; break;
					}
				}

				return realValue;
			}
		}

		static const uint32 kHistorySize = 1 << 20;
		static const uint32 kWindowReservSize = (1 << 22) + 256;

		CDecoder::CDecoder() : m_IsSolid(false), m_TablesOK(false)
		{
		}

		void CDecoder::InitStructures()
		{
			m_MmFilter.Init();
			for(uint i = 0; i < kNumRepDists; i++)
				m_RepDists[i] = 0;
			m_RepDistPtr = 0;
			m_LastLength = 0;
			memzero(m_LastLevels, kMaxTableSize);
		}
		uint32 CDecoder::ReadBits(uint numBits) { return m_InBitStream.ReadBits(numBits); }

		#define RIF(x) { if(!(x)) return false;	}

		bool CDecoder::ReadTables(void)
		{
			m_TablesOK = false;
			Byte levelLevels[kLevelTableSize];
			Byte lens[kMaxTableSize];
			m_AudioMode = (ReadBits(1) == 1);
			if(ReadBits(1) == 0)
				memzero(m_LastLevels, kMaxTableSize);
			unsigned numLevels;
			if(m_AudioMode) {
				m_NumChannels = ReadBits(2) + 1;
				if(m_MmFilter.CurrentChannel >= m_NumChannels)
					m_MmFilter.CurrentChannel = 0;
				numLevels = m_NumChannels * kMMTableSize;
			}
			else
				numLevels = kHeapTablesSizesSum;
			uint i;
			for(i = 0; i < kLevelTableSize; i++)
				levelLevels[i] = (Byte)ReadBits(4);
			RIF(m_LevelDecoder.Build(levelLevels));

			i = 0;

			while(i < numLevels) {
				uint32 sym = m_LevelDecoder.Decode(&m_InBitStream);
				if(sym < kTableDirectLevels) {
					lens[i] = (Byte)((sym + m_LastLevels[i]) & kLevelMask);
					i++;
				}
				else {
					if(sym == kTableLevelRepNumber) {
						unsigned num = ReadBits(2) + 3;
						if(i == 0) {
							// return false;
							continue; // original unRAR
						}
						num += i;
						if(num > numLevels) {
							// return false;
							num = numLevels; // original unRAR
						}
						Byte v = lens[(size_t)i - 1];
						do
							lens[i++] = v;
						while(i < num);
					}
					else {
						unsigned num;
						if(sym == kTableLevel0Number)
							num = ReadBits(3) + 3;
						else if(sym == kTableLevel0Number2)
							num = ReadBits(7) + 11;
						else
							return false;
						num += i;
						if(num > numLevels) {
							// return false;
							num = numLevels; // original unRAR
						}
						do
							lens[i++] = 0;
						while(i < num);
					}
				}
			}

			if(m_AudioMode)
				for(i = 0; i < m_NumChannels; i++) {
					RIF(m_MMDecoders[i].Build(&lens[i * kMMTableSize]));
				}
			else {
				RIF(m_MainDecoder.Build(&lens[0]));
				RIF(m_DistDecoder.Build(&lens[kMainTableSize]));
				RIF(m_LenDecoder.Build(&lens[kMainTableSize + kDistTableSize]));
			}
			memcpy(m_LastLevels, lens, kMaxTableSize);
			m_TablesOK = true;
			return true;
		}
		#undef RIF

		bool CDecoder::ReadLastTables()
		{
			// it differs a little from pure RAR sources;
			// uint64 ttt = m_InBitStream.GetProcessedSize() + 2;
			// + 2 works for: return 0xFF; in CInBuffer::ReadByte.
			if(m_InBitStream.GetProcessedSize() + 7 <= m_PackSize) // test it: probably incorrect;
				// if(m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect;
				if(m_AudioMode) {
					uint32 symbol = m_MMDecoders[m_MmFilter.CurrentChannel].Decode(&m_InBitStream);
					if(symbol == 256)
						return ReadTables();
					if(symbol >= kMMTableSize)
						return false;
				}
				else {
					uint32 sym = m_MainDecoder.Decode(&m_InBitStream);
					if(sym == kReadTableNumber)
						return ReadTables();
					if(sym >= kMainTableSize)
						return false;
				}
			return true;
		}

		/*
		   class CCoderReleaser
		   {
		   CDecoder *m_Coder;
		   public:
		   CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
		   ~CCoderReleaser()
		   {
			m_Coder->ReleaseStreams();
		   }
		   };
		 */

		bool CDecoder::DecodeMm(uint32 pos)
		{
			while(pos-- != 0) {
				uint32 symbol = m_MMDecoders[m_MmFilter.CurrentChannel].Decode(&m_InBitStream);
				if(symbol >= 256)
					return symbol == 256;
				/*
				   Byte byPredict = m_Predictor.Predict();
				   Byte byReal = (Byte)(byPredict - (Byte)symbol);
				   m_Predictor.Update(byReal, byPredict);
				 */
				Byte byReal = m_MmFilter.Decode((Byte)symbol);
				m_OutWindowStream.PutByte(byReal);
				if(++m_MmFilter.CurrentChannel == m_NumChannels)
					m_MmFilter.CurrentChannel = 0;
			}
			return true;
		}

		bool CDecoder::DecodeLz(int32 pos)
		{
			while(pos > 0) {
				uint32 sym = m_MainDecoder.Decode(&m_InBitStream);
				uint32 length, distance;
				if(sym < 256) {
					m_OutWindowStream.PutByte(Byte(sym));
					pos--;
					continue;
				}
				else if(sym >= kMatchNumber) {
					if(sym >= kMainTableSize)
						return false;
					sym -= kMatchNumber;
					length = kNormalMatchMinLen + uint32(kLenStart[sym]) +
								m_InBitStream.ReadBits(kLenDirectBits[sym]);
					sym = m_DistDecoder.Decode(&m_InBitStream);
					if(sym >= kDistTableSize)
						return false;
					distance = kDistStart[sym] + m_InBitStream.ReadBits(kDistDirectBits[sym]);
					if(distance >= kDistLimit3) {
						length += 2 - ((distance - kDistLimit4) >> 31);
						// length++;
						// if(distance >= kDistLimit4)
						//  length++;
					}
				}
				else if(sym == kRepBothNumber) {
					length = m_LastLength;
					if(length == 0)
						return false;
					distance = m_RepDists[(m_RepDistPtr + 4 - 1) & 3];
				}
				else if(sym < kLen2Number) {
					distance = m_RepDists[(m_RepDistPtr - (sym - kRepNumber + 1)) & 3];
					sym = m_LenDecoder.Decode(&m_InBitStream);
					if(sym >= kLenTableSize)
						return false;
					length = 2 + kLenStart[sym] + m_InBitStream.ReadBits(kLenDirectBits[sym]);
					if(distance >= kDistLimit2) {
						length++;
						if(distance >= kDistLimit3) {
							length += 2 - ((distance - kDistLimit4) >> 31);
							// length++;
							// if(distance >= kDistLimit4)
							//   length++;
						}
					}
				}
				else if(sym < kReadTableNumber) {
					sym -= kLen2Number;
					distance = kLen2DistStarts[sym] +
								m_InBitStream.ReadBits(kLen2DistDirectBits[sym]);
					length = 2;
				}
				else // (sym == kReadTableNumber)
					return true;

				m_RepDists[m_RepDistPtr++ & 3] = distance;
				m_LastLength = length;
				if(!m_OutWindowStream.CopyBlock(distance, length))
					return false;
				pos -= length;
			}
			return true;
		}

		HRESULT CDecoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			if(!inSize || !outSize)
				return E_INVALIDARG;
			if(!m_OutWindowStream.Create(kHistorySize))
				return E_OUTOFMEMORY;
			if(!m_InBitStream.Create(1 << 20))
				return E_OUTOFMEMORY;
			m_PackSize = *inSize;
			uint64 pos = 0, unPackSize = *outSize;
			m_OutWindowStream.SetStream(outStream);
			m_OutWindowStream.Init(m_IsSolid);
			m_InBitStream.SetStream(inStream);
			m_InBitStream.Init();
			// CCoderReleaser coderReleaser(this);
			if(!m_IsSolid) {
				InitStructures();
				if(unPackSize == 0) {
					if(m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect;
						if(!ReadTables())
							return S_FALSE;
					return S_OK;
				}
				ReadTables();
			}
			if(!m_TablesOK)
				return S_FALSE;
			uint64 startPos = m_OutWindowStream.GetProcessedSize();
			while(pos < unPackSize) {
				uint32 blockSize = 1 << 20;
				if(blockSize > unPackSize - pos)
					blockSize = (uint32)(unPackSize - pos);
				uint64 blockStartPos = m_OutWindowStream.GetProcessedSize();
				if(m_AudioMode) {
					if(!DecodeMm(blockSize))
						return S_FALSE;
				}
				else {
					if(!DecodeLz((int32)blockSize))
						return S_FALSE;
				}
				uint64 globalPos = m_OutWindowStream.GetProcessedSize();
				pos = globalPos - blockStartPos;
				if(pos < blockSize)
					if(!ReadTables())
						return S_FALSE;
				pos = globalPos - startPos;
				if(progress != 0) {
					uint64 packSize = m_InBitStream.GetProcessedSize();
					RINOK(progress->SetRatioInfo(&packSize, &pos));
				}
			}
			if(pos > unPackSize)
				return S_FALSE;
			return ReadLastTables() ? m_OutWindowStream.Flush() : S_FALSE;
		}
		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(const CLzOutWindowException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}
		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			if(size < 1)
				return E_INVALIDARG;
			m_IsSolid = ((data[0] & 1) != 0);
			return S_OK;
		}
	}
	// This code uses Carryless rangecoder (1999): Dmitry Subbotin : Public domain 
	namespace NRar3 {
		static const uint32 kNumAlignReps = 15;
		static const uint32 kSymbolReadTable = 256;
		static const uint32 kSymbolRep = 259;
		static const uint32 kSymbolLen2 = kSymbolRep + kNumReps;

		static const Byte kLenStart     [kLenTableSize] =
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
		static const Byte kLenDirectBits[kLenTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};

		static const Byte kDistDirectBits[kDistTableSize] =
		{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
		 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
		 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18};

		static const Byte kLevelDirectBits[kLevelTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7};
		static const Byte kLen2DistStarts[kNumLen2Symbols] = {0, 4, 8, 16, 32, 64, 128, 192};
		static const Byte kLen2DistDirectBits[kNumLen2Symbols] = {2, 2, 3, 4, 5, 6,  6,  6};
		static const uint32 kDistLimit3 = 0x2000 - 2;
		static const uint32 kDistLimit4 = 0x40000 - 2;
		static const uint32 kNormalMatchMinLen = 3;
		static const uint32 kVmDataSizeMax = 1 << 16;
		static const uint32 kVmCodeSizeMax = 1 << 16;

		bool FASTCALL CBitDecoder::Create(uint32 bufSize) 
		{
			return Stream.Create(bufSize);
		}

		void FASTCALL CBitDecoder::SetStream(ISequentialInStream * inStream) 
		{
			Stream.SetStream(inStream);
		}

		void CBitDecoder::Init()
		{
			Stream.Init();
			_bitPos = 0;
			_value = 0;
		}

		bool CBitDecoder::ExtraBitsWereRead() const
		{
			return (Stream.NumExtraBytes > 4 || _bitPos < (Stream.NumExtraBytes << 3));
		}

		uint64 CBitDecoder::GetProcessedSize() const { return Stream.GetProcessedSize() - (_bitPos >> 3); }

		void CBitDecoder::AlignToByte()
		{
			_bitPos &= ~(uint)7;
			_value = _value & ((1 << _bitPos) - 1);
		}

		uint32 FASTCALL CBitDecoder::GetValue(uint numBits)
		{
			if(_bitPos < numBits) {
				_bitPos += 8;
				_value = (_value << 8) | Stream.ReadByte();
				if(_bitPos < numBits) {
					_bitPos += 8;
					_value = (_value << 8) | Stream.ReadByte();
				}
			}
			return _value >> (_bitPos - numBits);
		}

		void FASTCALL CBitDecoder::MovePos(uint numBits)
		{
			_bitPos -= numBits;
			_value = _value & ((1 << _bitPos) - 1);
		}

		uint32 FASTCALL CBitDecoder::ReadBits(uint numBits)
		{
			uint32 res = GetValue(numBits);
			MovePos(numBits);
			return res;
		}

		extern "C" {
		#define GET_RangeDecoder CRangeDecoder *p = CONTAINER_FROM_VTBL_CLS(pp, CRangeDecoder, vt);

		static uint32 Range_GetThreshold(const IPpmd7_RangeDec * pp, uint32 total)
		{
			GET_RangeDecoder;
			return p->Code / (p->Range /= total);
		}

		static void Range_Decode(const IPpmd7_RangeDec * pp, uint32 start, uint32 size)
		{
			GET_RangeDecoder;
			start *= p->Range;
			p->Low += start;
			p->Code -= start;
			p->Range *= size;
			p->Normalize();
		}

		static uint32 Range_DecodeBit(const IPpmd7_RangeDec * pp, uint32 size0)
		{
			GET_RangeDecoder;
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

		CRangeDecoder::CRangeDecoder() throw()
		{
			vt.GetThreshold = Range_GetThreshold;
			vt.Decode = Range_Decode;
			vt.DecodeBit = Range_DecodeBit;
		}

		void CRangeDecoder::InitRangeCoder()
		{
			Code = 0;
			Low = 0;
			Range = 0xFFFFFFFF;
			for(int i = 0; i < 4; i++)
				Code = (Code << 8) | BitDecoder.ReadBits(8);
		}

		void CRangeDecoder::Normalize()
		{
			while((Low ^ (Low + Range)) < kTopValue || Range < kBot && ((Range = (0 - Low) & (kBot - 1)), 1)) {
				Code = (Code << 8) | BitDecoder.Stream.ReadByte();
				Range <<= 8;
				Low <<= 8;
			}
		}

		CDecoder::CDecoder() : _window(0), _winPos(0), _wrPtr(0), _lzSize(0), _writtenFileSize(0), _vmData(0), _vmCode(0), m_IsSolid(false)
		{
			Ppmd7_Construct(&_ppmd);
		}

		CDecoder::~CDecoder()
		{
			InitFilters();
			::MidFree(_vmData);
			::MidFree(_window);
			Ppmd7_Free(&_ppmd, &g_BigAlloc);
		}

		void CDecoder::CopyBlock(uint32 distance, uint32 len)
		{
			_lzSize += len;
			uint32 pos = (_winPos - distance - 1) & kWindowMask;
			Byte * window = _window;
			uint32 winPos = _winPos;
			if(kWindowSize - winPos > len && kWindowSize - pos > len) {
				const Byte * src = window + pos;
				Byte * dest = window + winPos;
				_winPos += len;
				do {
					*dest++ = *src++;
				} while(--len != 0);
				return;
			}
			do {
				window[winPos] = window[pos];
				winPos = (winPos + 1) & kWindowMask;
				pos = (pos + 1) & kWindowMask;
			} while(--len != 0);
			_winPos = winPos;
		}

		void FASTCALL CDecoder::PutByte(Byte b)
		{
			_window[_winPos] = b;
			_winPos = (_winPos + 1) & kWindowMask;
			_lzSize++;
		}

		HRESULT CDecoder::WriteDataToStream(const Byte * data, uint32 size)
		{
			return WriteStream(_outStream, data, size);
		}

		HRESULT CDecoder::WriteData(const Byte * data, uint32 size)
		{
			HRESULT res = S_OK;
			if(_writtenFileSize < _unpackSize) {
				uint32 curSize = size;
				uint64 remain = _unpackSize - _writtenFileSize;
				if(remain < curSize)
					curSize = (uint32)remain;
				res = WriteDataToStream(data, curSize);
			}
			_writtenFileSize += size;
			return res;
		}

		HRESULT CDecoder::WriteArea(uint32 startPtr, uint32 endPtr)
		{
			if(startPtr <= endPtr)
				return WriteData(_window + startPtr, endPtr - startPtr);
			RINOK(WriteData(_window + startPtr, kWindowSize - startPtr));
			return WriteData(_window, endPtr);
		}

		CDecoder::CTempFilter::CTempFilter()
		{
			AllocateEmptyFixedGlobal(); // all filters must contain at least FixedGlobal block
		}

		void CDecoder::ExecuteFilter(int tempFilterIndex, NVm::CBlockRef &outBlockRef)
		{
			CTempFilter * tempFilter = _tempFilters[tempFilterIndex];
			tempFilter->InitR[6] = (uint32)_writtenFileSize;
			NVm::SetValue32(&tempFilter->GlobalData[0x24], (uint32)_writtenFileSize);
			NVm::SetValue32(&tempFilter->GlobalData[0x28], (uint32)(_writtenFileSize >> 32));
			CFilter * filter = _filters[tempFilter->FilterIndex];
			if(!filter->IsSupported)
				_unsupportedFilter = true;
			_vm.Execute(filter, tempFilter, outBlockRef, filter->GlobalData);
			delete tempFilter;
			_tempFilters[tempFilterIndex] = 0;
		}

		HRESULT CDecoder::WriteBuf()
		{
			uint32 writtenBorder = _wrPtr;
			uint32 writeSize = (_winPos - writtenBorder) & kWindowMask;
			FOR_VECTOR(i, _tempFilters)
			{
				CTempFilter * filter = _tempFilters[i];
				if(!filter)
					continue;
				if(filter->NextWindow) {
					filter->NextWindow = false;
					continue;
				}
				uint32 blockStart = filter->BlockStart;
				uint32 blockSize = filter->BlockSize;
				if(((blockStart - writtenBorder) & kWindowMask) < writeSize) {
					if(writtenBorder != blockStart) {
						RINOK(WriteArea(writtenBorder, blockStart));
						writtenBorder = blockStart;
						writeSize = (_winPos - writtenBorder) & kWindowMask;
					}
					if(blockSize <= writeSize) {
						uint32 blockEnd = (blockStart + blockSize) & kWindowMask;
						if(blockStart < blockEnd || blockEnd == 0)
							_vm.SetMemory(0, _window + blockStart, blockSize);
						else {
							uint32 tailSize = kWindowSize - blockStart;
							_vm.SetMemory(0, _window + blockStart, tailSize);
							_vm.SetMemory(tailSize, _window, blockEnd);
						}
						NVm::CBlockRef outBlockRef;
						ExecuteFilter(i, outBlockRef);
						while(i + 1 < _tempFilters.Size()) {
							CTempFilter * nextFilter = _tempFilters[i+1];
							if(!nextFilter || nextFilter->BlockStart != blockStart || nextFilter->BlockSize != outBlockRef.Size || nextFilter->NextWindow)
								break;
							_vm.SetMemory(0, _vm.GetDataPointer(outBlockRef.Offset), outBlockRef.Size);
							ExecuteFilter(++i, outBlockRef);
						}
						WriteDataToStream(_vm.GetDataPointer(outBlockRef.Offset), outBlockRef.Size);
						_writtenFileSize += outBlockRef.Size;
						writtenBorder = blockEnd;
						writeSize = (_winPos - writtenBorder) & kWindowMask;
					}
					else {
						for(uint j = i; j < _tempFilters.Size(); j++) {
							CTempFilter * filter2 = _tempFilters[j];
							if(filter2 && filter2->NextWindow)
								filter2->NextWindow = false;
						}
						_wrPtr = writtenBorder;
						return S_OK; // check it
					}
				}
			}

			_wrPtr = _winPos;
			return WriteArea(writtenBorder, _winPos);
		}

		void CDecoder::InitFilters()
		{
			_lastFilter = 0;
			uint i;
			for(i = 0; i < _tempFilters.Size(); i++)
				delete _tempFilters[i];
			_tempFilters.Clear();
			for(i = 0; i < _filters.Size(); i++)
				delete _filters[i];
			_filters.Clear();
		}

		static const uint MAX_UNPACK_FILTERS = 8192;

		bool CDecoder::AddVmCode(uint32 firstByte, uint32 codeSize)
		{
			CMemBitDecoder inp;
			inp.Init(_vmData, codeSize);
			uint32 filterIndex;
			if(firstByte & 0x80) {
				filterIndex = inp.ReadEncodedUInt32();
				if(filterIndex == 0)
					InitFilters();
				else
					filterIndex--;
			}
			else
				filterIndex = _lastFilter;
			if(filterIndex > (uint32)_filters.Size())
				return false;
			_lastFilter = filterIndex;
			bool newFilter = (filterIndex == (uint32)_filters.Size());

			CFilter * filter;
			if(newFilter) {
				// check if too many filters
				if(filterIndex > MAX_UNPACK_FILTERS)
					return false;
				filter = new CFilter;
				_filters.Add(filter);
			}
			else {
				filter = _filters[filterIndex];
				filter->ExecCount++;
			}
			unsigned numEmptyItems = 0;
			{
				FOR_VECTOR(i, _tempFilters) {
					_tempFilters[i - numEmptyItems] = _tempFilters[i];
					if(!_tempFilters[i])
						numEmptyItems++;
					if(numEmptyItems != 0)
						_tempFilters[i] = NULL;
				}
			}
			if(numEmptyItems == 0) {
				_tempFilters.Add(NULL);
				numEmptyItems = 1;
			}
			CTempFilter * tempFilter = new CTempFilter;
			_tempFilters[_tempFilters.Size() - numEmptyItems] = tempFilter;
			tempFilter->FilterIndex = filterIndex;

			uint32 blockStart = inp.ReadEncodedUInt32();
			if(firstByte & 0x40)
				blockStart += 258;
			tempFilter->BlockStart = (blockStart + _winPos) & kWindowMask;
			if(firstByte & 0x20)
				filter->BlockSize = inp.ReadEncodedUInt32();
			tempFilter->BlockSize = filter->BlockSize;
			tempFilter->NextWindow = _wrPtr != _winPos && ((_wrPtr - _winPos) & kWindowMask) <= blockStart;

			memzero(tempFilter->InitR, sizeof(tempFilter->InitR));
			tempFilter->InitR[3] = NVm::kGlobalOffset;
			tempFilter->InitR[4] = tempFilter->BlockSize;
			tempFilter->InitR[5] = filter->ExecCount;
			if(firstByte & 0x10) {
				uint32 initMask = inp.ReadBits(NVm::kNumGpRegs);
				for(uint i = 0; i < NVm::kNumGpRegs; i++)
					if(initMask & (1 << i))
						tempFilter->InitR[i] = inp.ReadEncodedUInt32();
			}

			bool isOK = true;
			if(newFilter) {
				uint32 vmCodeSize = inp.ReadEncodedUInt32();
				if(vmCodeSize >= kVmCodeSizeMax || vmCodeSize == 0)
					return false;
				for(uint32 i = 0; i < vmCodeSize; i++)
					_vmCode[i] = (Byte)inp.ReadBits(8);
				isOK = filter->PrepareProgram(_vmCode, vmCodeSize);
			}

			{
				Byte * globalData = &tempFilter->GlobalData[0];
				for(uint i = 0; i < NVm::kNumGpRegs; i++)
					NVm::SetValue32(&globalData[i * 4], tempFilter->InitR[i]);
				NVm::SetValue32(&globalData[NVm::NGlobalOffset::kBlockSize], tempFilter->BlockSize);
				NVm::SetValue32(&globalData[NVm::NGlobalOffset::kBlockPos], 0); // It was commented. why?
				NVm::SetValue32(&globalData[NVm::NGlobalOffset::kExecCount], filter->ExecCount);
			}

			if(firstByte & 8) {
				uint32 dataSize = inp.ReadEncodedUInt32();
				if(dataSize > NVm::kGlobalSize - NVm::kFixedGlobalSize)
					return false;
				CRecordVector<Byte> &globalData = tempFilter->GlobalData;
				unsigned requiredSize = (uint)(dataSize + NVm::kFixedGlobalSize);
				if(globalData.Size() < requiredSize)
					globalData.ChangeSize_KeepData(requiredSize);
				Byte * dest = &globalData[NVm::kFixedGlobalSize];
				for(uint32 i = 0; i < dataSize; i++)
					dest[i] = (Byte)inp.ReadBits(8);
			}

			return isOK;
		}

		bool CDecoder::ReadVmCodeLZ()
		{
			uint32 firstByte = ReadBits(8);
			uint32 length = (firstByte & 7) + 1;
			if(length == 7)
				length = ReadBits(8) + 7;
			else if(length == 8)
				length = ReadBits(16);
			if(length > kVmDataSizeMax)
				return false;
			for(uint32 i = 0; i < length; i++)
				_vmData[i] = (Byte)ReadBits(8);
			return AddVmCode(firstByte, length);
		}

		bool CDecoder::ReadVmCodePPM()
		{
			int firstByte = DecodePpmSymbol();
			if(firstByte < 0)
				return false;
			uint32 length = (firstByte & 7) + 1;
			if(length == 7) {
				int b1 = DecodePpmSymbol();
				if(b1 < 0)
					return false;
				length = b1 + 7;
			}
			else if(length == 8) {
				int b1 = DecodePpmSymbol();
				if(b1 < 0)
					return false;
				int b2 = DecodePpmSymbol();
				if(b2 < 0)
					return false;
				length = b1 * 256 + b2;
			}
			if(length > kVmDataSizeMax)
				return false;
			if(InputEofError_Fast())
				return false;
			for(uint32 i = 0; i < length; i++) {
				int b = DecodePpmSymbol();
				if(b < 0)
					return false;
				_vmData[i] = (Byte)b;
			}
			return AddVmCode(firstByte, length);
		}

		#define RIF(x) { if(!(x)) return S_FALSE; }

		uint32 CDecoder::ReadBits(int numBits) 
		{
			return m_InBitStream.BitDecoder.ReadBits(numBits);
		}

		// ---------- PPM ----------

		HRESULT CDecoder::InitPPM()
		{
			unsigned maxOrder = (uint)ReadBits(7);
			bool reset = ((maxOrder & 0x20) != 0);
			int maxMB = 0;
			if(reset)
				maxMB = (Byte)ReadBits(8);
			else {
				if(PpmError || !Ppmd7_WasAllocated(&_ppmd))
					return S_FALSE;
			}
			if(maxOrder & 0x40)
				PpmEscChar = (Byte)ReadBits(8);
			m_InBitStream.InitRangeCoder();
			/*
			   if(m_InBitStream.m_BitPos != 0)
			   return S_FALSE;
			 */
			if(reset) {
				PpmError = true;
				maxOrder = (maxOrder & 0x1F) + 1;
				if(maxOrder > 16)
					maxOrder = 16 + (maxOrder - 16) * 3;
				if(maxOrder == 1) {
					Ppmd7_Free(&_ppmd, &g_BigAlloc);
					return S_FALSE;
				}
				if(!Ppmd7_Alloc(&_ppmd, (maxMB + 1) << 20, &g_BigAlloc))
					return E_OUTOFMEMORY;
				Ppmd7_Init(&_ppmd, maxOrder);
				PpmError = false;
			}
			return S_OK;
		}

		int CDecoder::DecodePpmSymbol() {
			return Ppmd7_DecodeSymbol(&_ppmd, &m_InBitStream.vt);
		}

		HRESULT CDecoder::DecodePPM(int32 num, bool &keepDecompressing)
		{
			keepDecompressing = false;
			if(PpmError)
				return S_FALSE;
			do {
				if(((_wrPtr - _winPos) & kWindowMask) < 260 && _wrPtr != _winPos) {
					RINOK(WriteBuf());
					if(_writtenFileSize > _unpackSize) {
						keepDecompressing = false;
						return S_OK;
					}
				}
				if(InputEofError_Fast())
					return false;
				int c = DecodePpmSymbol();
				if(c < 0) {
					PpmError = true;
					return S_FALSE;
				}
				if(c == PpmEscChar) {
					int nextCh = DecodePpmSymbol();
					if(nextCh < 0) {
						PpmError = true;
						return S_FALSE;
					}
					if(nextCh == 0)
						return ReadTables(keepDecompressing);
					if(nextCh == 2 || nextCh == -1)
						return S_OK;
					if(nextCh == 3) {
						if(!ReadVmCodePPM()) {
							PpmError = true;
							return S_FALSE;
						}
						continue;
					}
					if(nextCh == 4 || nextCh == 5) {
						uint32 distance = 0;
						uint32 length = 4;
						if(nextCh == 4) {
							for(int i = 0; i < 3; i++) {
								int c2 = DecodePpmSymbol();
								if(c2 < 0) {
									PpmError = true;
									return S_FALSE;
								}
								distance = (distance << 8) + (Byte)c2;
							}
							distance++;
							length += 28;
						}
						int c2 = DecodePpmSymbol();
						if(c2 < 0) {
							PpmError = true;
							return S_FALSE;
						}
						length += c2;
						if(distance >= _lzSize)
							return S_FALSE;
						CopyBlock(distance, length);
						num -= (int32)length;
						continue;
					}
				}
				PutByte((Byte)c);
				num--;
			}
			while(num >= 0);
			keepDecompressing = true;
			return S_OK;
		}

		// ---------- LZ ----------

		HRESULT CDecoder::ReadTables(bool &keepDecompressing)
		{
			keepDecompressing = true;
			m_InBitStream.BitDecoder.AlignToByte();
			if(ReadBits(1) != 0) {
				_lzMode = false;
				return InitPPM();
			}
			TablesRead = false;
			TablesOK = false;
			_lzMode = true;
			PrevAlignBits = 0;
			PrevAlignCount = 0;
			Byte levelLevels[kLevelTableSize];
			Byte newLevels[kTablesSizesSum];
			if(ReadBits(1) == 0)
				memzero(m_LastLevels, kTablesSizesSum);
			int i;
			for(i = 0; i < kLevelTableSize; i++) {
				uint32 length = ReadBits(4);
				if(length == 15) {
					uint32 zeroCount = ReadBits(4);
					if(zeroCount != 0) {
						zeroCount += 2;
						while(zeroCount-- > 0 && i < kLevelTableSize)
							levelLevels[i++] = 0;
						i--;
						continue;
					}
				}
				levelLevels[i] = (Byte)length;
			}
			RIF(m_LevelDecoder.Build(levelLevels));
			i = 0;
			while(i < kTablesSizesSum) {
				uint32 sym = m_LevelDecoder.Decode(&m_InBitStream.BitDecoder);
				if(sym < 16) {
					newLevels[i] = Byte((sym + m_LastLevels[i]) & 15);
					i++;
				}
				else if(sym > kLevelTableSize)
					return S_FALSE;
				else {
					int num;
					if(((sym - 16) & 1) == 0)
						num = ReadBits(3) + 3;
					else
						num = ReadBits(7) + 11;
					if(sym < 18) {
						if(i == 0)
							return S_FALSE;
						for(; num > 0 && i < kTablesSizesSum; num--, i++)
							newLevels[i] = newLevels[(size_t)i - 1];
					}
					else {
						for(; num > 0 && i < kTablesSizesSum; num--)
							newLevels[i++] = 0;
					}
				}
			}

			TablesRead = true;

			// original code has check here:
			/*
			   if(InAddr > ReadTop)
			   {
			   keepDecompressing = false;
			   return true;
			   }
			 */
			RIF(m_MainDecoder.Build(&newLevels[0]));
			RIF(m_DistDecoder.Build(&newLevels[kMainTableSize]));
			RIF(m_AlignDecoder.Build(&newLevels[kMainTableSize + kDistTableSize]));
			RIF(m_LenDecoder.Build(&newLevels[kMainTableSize + kDistTableSize + kAlignTableSize]));
			memcpy(m_LastLevels, newLevels, kTablesSizesSum);
			TablesOK = true;
			return S_OK;
		}
		#undef RIF
		/*
		   class CCoderReleaser {
		   CDecoder *m_Coder;
		   public:
		   CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
		   ~CCoderReleaser() { m_Coder->ReleaseStreams(); }
		   };
		 */

		HRESULT CDecoder::ReadEndOfBlock(bool &keepDecompressing)
		{
			if(ReadBits(1) == 0) {
				// new file
				keepDecompressing = false;
				TablesRead = (ReadBits(1) == 0);
				return S_OK;
			}
			TablesRead = false;
			return ReadTables(keepDecompressing);
		}

		uint32 kDistStart[kDistTableSize];

		class CDistInit
		{
		public:
			CDistInit() {
				Init();
			}

			void Init()
			{
				uint32 start = 0;
				for(uint32 i = 0; i < kDistTableSize; i++) {
					kDistStart[i] = start;
					start += (1 << kDistDirectBits[i]);
				}
			}
		} g_DistInit;

		HRESULT CDecoder::DecodeLZ(bool &keepDecompressing)
		{
			uint32 rep0 = _reps[0];
			uint32 rep1 = _reps[1];
			uint32 rep2 = _reps[2];
			uint32 rep3 = _reps[3];
			uint32 length = _lastLength;
			for(;; ) {
				if(((_wrPtr - _winPos) & kWindowMask) < 260 && _wrPtr != _winPos) {
					RINOK(WriteBuf());
					if(_writtenFileSize > _unpackSize) {
						keepDecompressing = false;
						return S_OK;
					}
				}

				if(InputEofError_Fast())
					return S_FALSE;

				uint32 sym = m_MainDecoder.Decode(&m_InBitStream.BitDecoder);
				if(sym < 256) {
					PutByte((Byte)sym);
					continue;
				}
				else if(sym == kSymbolReadTable) {
					RINOK(ReadEndOfBlock(keepDecompressing));
					break;
				}
				else if(sym == 257) {
					if(!ReadVmCodeLZ())
						return S_FALSE;
					continue;
				}
				else if(sym == 258) {
					if(length == 0)
						return S_FALSE;
				}
				else if(sym < kSymbolRep + 4) {
					if(sym != kSymbolRep) {
						uint32 distance;
						if(sym == kSymbolRep + 1)
							distance = rep1;
						else {
							if(sym == kSymbolRep + 2)
								distance = rep2;
							else {
								distance = rep3;
								rep3 = rep2;
							}
							rep2 = rep1;
						}
						rep1 = rep0;
						rep0 = distance;
					}

					const uint32 sym2 = m_LenDecoder.Decode(&m_InBitStream.BitDecoder);
					if(sym2 >= kLenTableSize)
						return S_FALSE;
					length = 2 + kLenStart[sym2] + m_InBitStream.BitDecoder.ReadBits(kLenDirectBits[sym2]);
				}
				else {
					rep3 = rep2;
					rep2 = rep1;
					rep1 = rep0;
					if(sym < 271) {
						sym -= 263;
						rep0 = kLen2DistStarts[sym] + m_InBitStream.BitDecoder.ReadBits(kLen2DistDirectBits[sym]);
						length = 2;
					}
					else if(sym < 299) {
						sym -= 271;
						length = kNormalMatchMinLen + (uint32)kLenStart[sym] + m_InBitStream.BitDecoder.ReadBits(
									kLenDirectBits[sym]);
						const uint32 sym2 = m_DistDecoder.Decode(&m_InBitStream.BitDecoder);
						if(sym2 >= kDistTableSize)
							return S_FALSE;
						rep0 = kDistStart[sym2];
						int numBits = kDistDirectBits[sym2];
						if(sym2 >= (kNumAlignBits * 2) + 2) {
							if(numBits > kNumAlignBits)
								rep0 += (m_InBitStream.BitDecoder.ReadBits(numBits - kNumAlignBits) << kNumAlignBits);
							if(PrevAlignCount > 0) {
								PrevAlignCount--;
								rep0 += PrevAlignBits;
							}
							else {
								const uint32 sym3 = m_AlignDecoder.Decode(&m_InBitStream.BitDecoder);
								if(sym3 < (1 << kNumAlignBits)) {
									rep0 += sym3;
									PrevAlignBits = sym3;
								}
								else if(sym3 == (1 << kNumAlignBits)) {
									PrevAlignCount = kNumAlignReps;
									rep0 += PrevAlignBits;
								}
								else
									return S_FALSE;
							}
						}
						else
							rep0 += m_InBitStream.BitDecoder.ReadBits(numBits);
						length += ((kDistLimit4 - rep0) >> 31) + ((kDistLimit3 - rep0) >> 31);
					}
					else
						return S_FALSE;
				}
				if(rep0 >= _lzSize)
					return S_FALSE;
				CopyBlock(rep0, length);
			}
			_reps[0] = rep0;
			_reps[1] = rep1;
			_reps[2] = rep2;
			_reps[3] = rep3;
			_lastLength = length;

			return S_OK;
		}

		HRESULT CDecoder::CodeReal(ICompressProgressInfo * progress)
		{
			_writtenFileSize = 0;
			_unsupportedFilter = false;
			if(!m_IsSolid) {
				_lzSize = 0;
				_winPos = 0;
				_wrPtr = 0;
				for(int i = 0; i < kNumReps; i++)
					_reps[i] = 0;
				_lastLength = 0;
				memzero(m_LastLevels, kTablesSizesSum);
				TablesRead = false;
				PpmEscChar = 2;
				PpmError = true;
				InitFilters();
			}
			if(!m_IsSolid || !TablesRead) {
				bool keepDecompressing;
				RINOK(ReadTables(keepDecompressing));
				if(!keepDecompressing)
					return S_OK;
			}

			for(;; ) {
				bool keepDecompressing;
				if(_lzMode) {
					if(!TablesOK)
						return S_FALSE;
					RINOK(DecodeLZ(keepDecompressing))
				}
				else {
					RINOK(DecodePPM(1 << 18, keepDecompressing))
				}

				if(InputEofError())
					return S_FALSE;

				uint64 packSize = m_InBitStream.BitDecoder.GetProcessedSize();
				RINOK(progress->SetRatioInfo(&packSize, &_writtenFileSize));
				if(!keepDecompressing)
					break;
			}
			RINOK(WriteBuf());
			uint64 packSize = m_InBitStream.BitDecoder.GetProcessedSize();
			RINOK(progress->SetRatioInfo(&packSize, &_writtenFileSize));
			if(_writtenFileSize < _unpackSize)
				return S_FALSE;

			if(_unsupportedFilter)
				return E_NOTIMPL;

			return S_OK;
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try {
				if(!inSize)
					return E_INVALIDARG;
				if(!_vmData) {
					_vmData = static_cast<Byte *>(::MidAlloc(kVmDataSizeMax + kVmCodeSizeMax));
					if(!_vmData)
						return E_OUTOFMEMORY;
					_vmCode = _vmData + kVmDataSizeMax;
				}
				if(!_window) {
					_window = static_cast<Byte *>(::MidAlloc(kWindowSize));
					if(!_window)
						return E_OUTOFMEMORY;
				}
				if(!m_InBitStream.BitDecoder.Create(1 << 20))
					return E_OUTOFMEMORY;
				if(!_vm.Create())
					return E_OUTOFMEMORY;
				m_InBitStream.BitDecoder.SetStream(inStream);
				m_InBitStream.BitDecoder.Init();
				_outStream = outStream;
				// CCoderReleaser coderReleaser(this);
				_unpackSize = outSize ? *outSize : static_cast<uint64>(-1LL);
				return CodeReal(progress);
			}
			catch(const CInBufferException &e)  { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
			// CNewException is possible here. But probably CNewException is caused
			// by error in data stream.
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			if(size < 1)
				return E_INVALIDARG;
			m_IsSolid = ((data[0] & 1) != 0);
			return S_OK;
		}
	}
	// 
	// Note: Due to performance considerations Rar VM may set Flags C incorrectly
	// for some operands (SHL x, 0, ... ).
	// Check implementation of concrete VM command to see if it sets flags right.
	// 
	namespace NRar3 {
		uint32 CMemBitDecoder::ReadBits(uint numBits)
		{
			uint32 res = 0;
			for(;; ) {
				unsigned b = (_bitPos < _bitSize) ? (uint)_data[_bitPos >> 3] : 0;
				unsigned avail = (uint)(8 - (_bitPos & 7));
				if(numBits <= avail) {
					_bitPos += numBits;
					return res | (b >> (avail - numBits)) & ((1 << numBits) - 1);
				}
				numBits -= avail;
				res |= (uint32)(b & ((1 << avail) - 1)) << numBits;
				_bitPos += avail;
			}
		}

		uint32 CMemBitDecoder::ReadBit() 
		{
			return ReadBits(1);
		}

		uint32 CMemBitDecoder::ReadEncodedUInt32()
		{
			unsigned v = (uint)ReadBits(2);
			uint32 res = ReadBits(4 << v);
			if(v == 1 && res < 16)
				res = 0xFFFFFF00 | (res << 4) | ReadBits(4);
			return res;
		}

		namespace NVm {
			static const uint32 kStackRegIndex = kNumRegs - 1;

			#ifdef RARVM_VM_ENABLE

			static const uint32 FLAG_C = 1;
			static const uint32 FLAG_Z = 2;
			static const uint32 FLAG_S = 0x80000000;

			static const Byte CF_OP0 = 0;
			static const Byte CF_OP1 = 1;
			static const Byte CF_OP2 = 2;
			static const Byte CF_OPMASK = 3;
			static const Byte CF_BYTEMODE = 4;
			static const Byte CF_JUMP = 8;
			static const Byte CF_PROC = 16;
			static const Byte CF_USEFLAGS = 32;
			static const Byte CF_CHFLAGS = 64;

			static const Byte kCmdFlags[] = {
				/* CMD_MOV   */ CF_OP2 | CF_BYTEMODE,
				/* CMD_CMP   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_ADD   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_SUB   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_JZ    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JNZ   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_INC   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_DEC   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_JMP   */ CF_OP1 | CF_JUMP,
				/* CMD_XOR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_AND   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_OR    */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_TEST  */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_JS    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JNS   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JB    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JBE   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JA    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_JAE   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
				/* CMD_PUSH  */ CF_OP1,
				/* CMD_POP   */ CF_OP1,
				/* CMD_CALL  */ CF_OP1 | CF_PROC,
				/* CMD_RET   */ CF_OP0 | CF_PROC,
				/* CMD_NOT   */ CF_OP1 | CF_BYTEMODE,
				/* CMD_SHL   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_SHR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_SAR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_NEG   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
				/* CMD_PUSHA */ CF_OP0,
				/* CMD_POPA  */ CF_OP0,
				/* CMD_PUSHF */ CF_OP0 | CF_USEFLAGS,
				/* CMD_POPF  */ CF_OP0 | CF_CHFLAGS,
				/* CMD_MOVZX */ CF_OP2,
				/* CMD_MOVSX */ CF_OP2,
				/* CMD_XCHG  */ CF_OP2 | CF_BYTEMODE,
				/* CMD_MUL   */ CF_OP2 | CF_BYTEMODE,
				/* CMD_DIV   */ CF_OP2 | CF_BYTEMODE,
				/* CMD_ADC   */ CF_OP2 | CF_BYTEMODE | CF_USEFLAGS | CF_CHFLAGS,
				/* CMD_SBB   */ CF_OP2 | CF_BYTEMODE | CF_USEFLAGS | CF_CHFLAGS,
				/* CMD_PRINT */ CF_OP0
			};

			#endif

			CVm::CVm() : Mem(NULL) 
			{
			}

			bool CVm::Create()
			{
				return SETIFZ(Mem, (Byte *)SAlloc::M(kSpaceSize + 4));
			}

			CVm::~CVm()
			{
				SAlloc::F(Mem);
			}

			// CVm::Execute can change CProgram object: it clears progarm if VM returns error.

			bool CVm::Execute(CProgram *prg, const CProgramInitState *initState, CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData)
			{
				memcpy(R, initState->InitR, sizeof(initState->InitR));
				R[kStackRegIndex] = kSpaceSize;
				R[kNumRegs] = 0;
				Flags = 0;
				uint32 globalSize = MyMin((uint32)initState->GlobalData.Size(), kGlobalSize);
				if(globalSize != 0)
					memcpy(Mem + kGlobalOffset, &initState->GlobalData[0], globalSize);
				uint32 staticSize = MyMin((uint32)prg->StaticData.Size(), kGlobalSize - globalSize);
				if(staticSize != 0)
					memcpy(Mem + kGlobalOffset + globalSize, &prg->StaticData[0], staticSize);
				bool res = true;
			#ifdef RARVM_STANDARD_FILTERS
				if(prg->StandardFilterIndex >= 0)
					ExecuteStandardFilter(prg->StandardFilterIndex);
				else
			#endif
				{
			#ifdef RARVM_VM_ENABLE
					res = ExecuteCode(prg);
					if(!res) {
						prg->Commands.Clear();
						prg->Commands.Add(CCommand());
						prg->Commands.Back().OpCode = CMD_RET;
					}
			#else
					res = false;
			#endif
				}
				uint32 newBlockPos = GetFixedGlobalValue32(NGlobalOffset::kBlockPos) & kSpaceMask;
				uint32 newBlockSize = GetFixedGlobalValue32(NGlobalOffset::kBlockSize) & kSpaceMask;
				if(newBlockPos + newBlockSize >= kSpaceSize)
					newBlockPos = newBlockSize = 0;
				outBlockRef.Offset = newBlockPos;
				outBlockRef.Size = newBlockSize;
				outGlobalData.Clear();
				uint32 dataSize = GetFixedGlobalValue32(NGlobalOffset::kGlobalMemOutSize);
				dataSize = MyMin(dataSize, kGlobalSize - kFixedGlobalSize);
				if(dataSize != 0) {
					dataSize += kFixedGlobalSize;
					outGlobalData.ClearAndSetSize(dataSize);
					memcpy(&outGlobalData[0], Mem + kGlobalOffset, dataSize);
				}
				return res;
			}

			#ifdef RARVM_VM_ENABLE
			#define SET_IP(IP) if((IP) >= numCommands) return true; if(--maxOpCount <= 0) return false; cmd = commands + (IP);
			#define GET_FLAG_S_B(res) (((res) & 0x80) ? FLAG_S : 0)
			#define SET_IP_OP1 { uint32 val = GetOperand32(&cmd->Op1); SET_IP(val);	}
			#define FLAGS_UPDATE_SZ Flags = res == 0 ? FLAG_Z : res & FLAG_S
			#define FLAGS_UPDATE_SZ_B Flags = (res & 0xFF) == 0 ? FLAG_Z : GET_FLAG_S_B(res)

			uint32 CVm::GetOperand32(const COperand *op) const
			{
				switch(op->Type) {
					case OP_TYPE_REG: return R[op->Data];
					case OP_TYPE_REGMEM: return GetValue32(&Mem[(op->Base + R[op->Data]) & kSpaceMask]);
					default: return op->Data;
				}
			}

			void CVm::SetOperand32(const COperand *op, uint32 val)
			{
				switch(op->Type) {
					case OP_TYPE_REG: R[op->Data] = val; return;
					case OP_TYPE_REGMEM: SetValue32(&Mem[(op->Base + R[op->Data]) & kSpaceMask], val); return;
				}
			}

			Byte CVm::GetOperand8(const COperand *op) const
			{
				switch(op->Type) {
					case OP_TYPE_REG: return (Byte)R[op->Data];
					case OP_TYPE_REGMEM: return Mem[(op->Base + R[op->Data]) & kSpaceMask];;
					default: return (Byte)op->Data;
				}
			}

			void CVm::SetOperand8(const COperand *op, Byte val)
			{
				switch(op->Type) {
					case OP_TYPE_REG: R[op->Data] = (R[op->Data] & 0xFFFFFF00) | val; return;
					case OP_TYPE_REGMEM: Mem[(op->Base + R[op->Data]) & kSpaceMask] = val; return;
				}
			}

			uint32 CVm::GetOperand(bool byteMode, const COperand *op) const
			{
				return byteMode ? GetOperand8(op) : GetOperand32(op);
			}

			void CVm::SetOperand(bool byteMode, const COperand *op, uint32 val)
			{
				if(byteMode)
					SetOperand8(op, (Byte)(val & 0xFF));
				else
					SetOperand32(op, val);
			}

			bool CVm::ExecuteCode(const CProgram *prg)
			{
				int32 maxOpCount = 25000000;
				const CCommand * commands = &prg->Commands[0];
				const CCommand * cmd = commands;
				uint32 numCommands = prg->Commands.Size();
				if(numCommands == 0)
					return false;
				for(;; ) {
					switch(cmd->OpCode) {
						case CMD_MOV:
						SetOperand32(&cmd->Op1, GetOperand32(&cmd->Op2));
						break;
						case CMD_MOVB:
						SetOperand8(&cmd->Op1, GetOperand8(&cmd->Op2));
						break;
						case CMD_CMP:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							uint32 res = v1 - GetOperand32(&cmd->Op2);
							Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
						}
						break;
						case CMD_CMPB:
						{
							Byte v1 = GetOperand8(&cmd->Op1);
							Byte res = (Byte)((v1 - GetOperand8(&cmd->Op2)) & 0xFF);
							Flags = res == 0 ? FLAG_Z : (res > v1) | GET_FLAG_S_B(res);
						}
						break;
						case CMD_ADD:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							uint32 res = v1 + GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
							Flags = (res < v1) | (res == 0 ? FLAG_Z : (res & FLAG_S));
						}
						break;
						case CMD_ADDB:
						{
							Byte v1 = GetOperand8(&cmd->Op1);
							Byte res = (Byte)((v1 + GetOperand8(&cmd->Op2)) & 0xFF);
							SetOperand8(&cmd->Op1, (Byte)res);
							Flags = (res < v1) | (res == 0 ? FLAG_Z : GET_FLAG_S_B(res));
						}
						break;
						case CMD_ADC:
						{
							uint32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
							uint32 FC = (Flags & FLAG_C);
							uint32 res = v1 + GetOperand(cmd->ByteMode, &cmd->Op2) + FC;
							if(cmd->ByteMode)
								res &= 0xFF;
							SetOperand(cmd->ByteMode, &cmd->Op1, res);
							Flags =
								(res < v1 || res == v1 &&
								FC) | (res == 0 ? FLAG_Z : (res & FLAG_S));
						}
						break;
						case CMD_SUB:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							uint32 res = v1 - GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
							Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
						}
						break;
						case CMD_SUBB:
						{
							uint32 v1 = GetOperand8(&cmd->Op1);
							uint32 res = v1 - GetOperand8(&cmd->Op2);
							SetOperand8(&cmd->Op1, (Byte)res);
							Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
						}
						break;
						case CMD_SBB:
						{
							uint32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
							uint32 FC = (Flags & FLAG_C);
							uint32 res = v1 - GetOperand(cmd->ByteMode, &cmd->Op2) - FC;
								// Flags = res == 0 ? FLAG_Z : (res > v1
								// || res == v1 && FC) | (res & FLAG_S);
							if(cmd->ByteMode)
								res &= 0xFF;
							SetOperand(cmd->ByteMode, &cmd->Op1, res);
							Flags = (res > v1 || res == v1 && FC) | (res == 0 ? FLAG_Z : (res & FLAG_S));
						}
						break;
						case CMD_INC:
						{
							uint32 res = GetOperand32(&cmd->Op1) + 1;
							SetOperand32(&cmd->Op1, res);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_INCB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) + 1);
							SetOperand8(&cmd->Op1, res);;
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_DEC:
						{
							uint32 res = GetOperand32(&cmd->Op1) - 1;
							SetOperand32(&cmd->Op1, res);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_DECB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) - 1);
							SetOperand8(&cmd->Op1, res);;
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_XOR:
						{
							uint32 res = GetOperand32(&cmd->Op1) ^ GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_XORB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) ^ GetOperand8(&cmd->Op2));
							SetOperand8(&cmd->Op1, res);
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_AND:
						{
							uint32 res = GetOperand32(&cmd->Op1) & GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_ANDB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) & GetOperand8(&cmd->Op2));
							SetOperand8(&cmd->Op1, res);
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_OR:
						{
							uint32 res = GetOperand32(&cmd->Op1) | GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_ORB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) | GetOperand8(&cmd->Op2));
							SetOperand8(&cmd->Op1, res);
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_TEST:
						{
							uint32 res = GetOperand32(&cmd->Op1) & GetOperand32(&cmd->Op2);
							FLAGS_UPDATE_SZ;
						}
						break;
						case CMD_TESTB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) & GetOperand8(&cmd->Op2));
							FLAGS_UPDATE_SZ_B;
						}
						break;
						case CMD_NOT:
						SetOperand(cmd->ByteMode, &cmd->Op1, ~GetOperand(cmd->ByteMode, &cmd->Op1));
						break;
						case CMD_NEG:
						{
							uint32 res = 0 - GetOperand32(&cmd->Op1);
							SetOperand32(&cmd->Op1, res);
							Flags = res == 0 ? FLAG_Z : FLAG_C | (res & FLAG_S);
						}
						break;
						case CMD_NEGB:
						{
							Byte res = (Byte)(0 - GetOperand8(&cmd->Op1));
							SetOperand8(&cmd->Op1, res);
							Flags = res == 0 ? FLAG_Z : FLAG_C | GET_FLAG_S_B(res);
						}
						break;

						case CMD_SHL:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							int v2 = (int)GetOperand32(&cmd->Op2);
							uint32 res = v1 << v2;
							SetOperand32(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 << (v2 - 1)) & 0x80000000 ? FLAG_C : 0);
						}
						break;
						case CMD_SHLB:
						{
							Byte v1 = GetOperand8(&cmd->Op1);
							int v2 = (int)GetOperand8(&cmd->Op2);
							Byte res = (Byte)(v1 << v2);
							SetOperand8(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 << (v2 - 1)) & 0x80 ? FLAG_C : 0);
						}
						break;
						case CMD_SHR:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							int v2 = (int)GetOperand32(&cmd->Op2);
							uint32 res = v1 >> v2;
							SetOperand32(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 >> (v2 - 1)) & FLAG_C);
						}
						break;
						case CMD_SHRB:
						{
							Byte v1 = GetOperand8(&cmd->Op1);
							int v2 = (int)GetOperand8(&cmd->Op2);
							Byte res = (Byte)(v1 >> v2);
							SetOperand8(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 >> (v2 - 1)) & FLAG_C);
						}
						break;
						case CMD_SAR:
						{
							uint32 v1 = GetOperand32(&cmd->Op1);
							int v2 = (int)GetOperand32(&cmd->Op2);
							uint32 res = uint32(((int32)v1) >> v2);
							SetOperand32(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 >> (v2 - 1)) & FLAG_C);
						}
						break;
						case CMD_SARB:
						{
							Byte v1 = GetOperand8(&cmd->Op1);
							int v2 = (int)GetOperand8(&cmd->Op2);
							Byte res = (Byte)(((signed char)v1) >> v2);
							SetOperand8(&cmd->Op1, res);
							Flags = (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 >> (v2 - 1)) & FLAG_C);
						}
						break;

						case CMD_JMP:
							SET_IP_OP1;
							continue;
						case CMD_JZ:
							if((Flags & FLAG_Z) != 0) {
								SET_IP_OP1;
								continue;
							}
							break;
						case CMD_JNZ:
							if((Flags & FLAG_Z) == 0) {
								SET_IP_OP1;
								continue;
							}
							break;
						case CMD_JS:
							if((Flags & FLAG_S) != 0) {
								SET_IP_OP1;
								continue;
							}
							break;
						case CMD_JNS:
							if((Flags & FLAG_S) == 0) {
								SET_IP_OP1;
								continue;
							}
							break;
						case CMD_JB:
							if((Flags & FLAG_C) != 0) {
								SET_IP_OP1;
								continue;
							}
							break;
						case CMD_JBE:
						if((Flags & (FLAG_C | FLAG_Z)) != 0) {
							SET_IP_OP1;
							continue;
						}
						break;
						case CMD_JA:
						if((Flags & (FLAG_C | FLAG_Z)) == 0) {
							SET_IP_OP1;
							continue;
						}
						break;
						case CMD_JAE:
						if((Flags & FLAG_C) == 0) {
							SET_IP_OP1;
							continue;
						}
						break;
						case CMD_PUSH:
						R[kStackRegIndex] -= 4;
						SetValue32(&Mem[R[kStackRegIndex] & kSpaceMask], GetOperand32(&cmd->Op1));
						break;
						case CMD_POP:
						SetOperand32(&cmd->Op1, GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]));
						R[kStackRegIndex] += 4;
						break;
						case CMD_CALL:
						R[kStackRegIndex] -= 4;
						SetValue32(&Mem[R[kStackRegIndex] & kSpaceMask], (uint32)(cmd - commands + 1));
						SET_IP_OP1;
						continue;
						case CMD_PUSHA:
						{
							for(uint32 i = 0, SP = R[kStackRegIndex] - 4; i < kNumRegs; i++, SP -= 4)
								SetValue32(&Mem[SP & kSpaceMask], R[i]);
							R[kStackRegIndex] -= kNumRegs * 4;
						}
						break;
						case CMD_POPA:
						{
							for(uint32 i = 0, SP = R[kStackRegIndex]; i < kNumRegs; i++, SP += 4)
								R[kStackRegIndex - i] = GetValue32(&Mem[SP & kSpaceMask]);
						}
						break;
						case CMD_PUSHF:
						R[kStackRegIndex] -= 4;
						SetValue32(&Mem[R[kStackRegIndex]&kSpaceMask], Flags);
						break;
						case CMD_POPF:
						Flags = GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]);
						R[kStackRegIndex] += 4;
						break;
						case CMD_MOVZX:
						SetOperand32(&cmd->Op1, GetOperand8(&cmd->Op2));
						break;
						case CMD_MOVSX:
						SetOperand32(&cmd->Op1, (uint32)(int32)(signed char)GetOperand8(&cmd->Op2));
						break;
						case CMD_XCHG:
						{
							uint32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
							SetOperand(cmd->ByteMode, &cmd->Op1, GetOperand(cmd->ByteMode, &cmd->Op2));
							SetOperand(cmd->ByteMode, &cmd->Op2, v1);
						}
						break;
						case CMD_MUL:
						{
							uint32 res = GetOperand32(&cmd->Op1) * GetOperand32(&cmd->Op2);
							SetOperand32(&cmd->Op1, res);
						}
						break;
						case CMD_MULB:
						{
							Byte res = (Byte)(GetOperand8(&cmd->Op1) * GetOperand8(&cmd->Op2));
							SetOperand8(&cmd->Op1, res);
						}
						break;
						case CMD_DIV:
						{
							uint32 divider = GetOperand(cmd->ByteMode, &cmd->Op2);
							if(divider != 0) {
								uint32 res = GetOperand(cmd->ByteMode, &cmd->Op1) / divider;
								SetOperand(cmd->ByteMode, &cmd->Op1, res);
							}
						}
						break;
						case CMD_RET:
						{
							if(R[kStackRegIndex] >= kSpaceSize)
								return true;
							uint32 ip = GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]);
							SET_IP(ip);
							R[kStackRegIndex] += 4;
							continue;
						}
						case CMD_PRINT:
						break;
					}
					cmd++;
					--maxOpCount;
				}
			}
			// 
			// Read program
			//
			static void DecodeArg(CMemBitDecoder &inp, COperand &op, bool byteMode)
			{
				if(inp.ReadBit()) {
					op.Type = OP_TYPE_REG;
					op.Data = inp.ReadBits(kNumRegBits);
				}
				else if(inp.ReadBit() == 0) {
					op.Type = OP_TYPE_INT;
					if(byteMode)
						op.Data = inp.ReadBits(8);
					else
						op.Data = inp.ReadEncodedUInt32();
				}
				else {
					op.Type = OP_TYPE_REGMEM;
					if(inp.ReadBit() == 0) {
						op.Data = inp.ReadBits(kNumRegBits);
						op.Base = 0;
					}
					else {
						if(inp.ReadBit() == 0)
							op.Data = inp.ReadBits(kNumRegBits);
						else
							op.Data = kNumRegs;
						op.Base = inp.ReadEncodedUInt32();
					}
				}
			}

			void CProgram::ReadProgram(const Byte *code, uint32 codeSize)
			{
				CMemBitDecoder inp;
				inp.Init(code, codeSize);

				StaticData.Clear();

				if(inp.ReadBit()) {
					uint32 dataSize = inp.ReadEncodedUInt32() + 1;
					for(uint32 i = 0; inp.Avail() && i < dataSize; i++)
						StaticData.Add((Byte)inp.ReadBits(8));
				}

				while(inp.Avail()) {
					Commands.Add(CCommand());
					CCommand * cmd = &Commands.Back();

					if(inp.ReadBit() == 0)
						cmd->OpCode = (ECommand)inp.ReadBits(3);
					else
						cmd->OpCode = (ECommand)(8 + inp.ReadBits(5));

					if(kCmdFlags[(uint)cmd->OpCode] & CF_BYTEMODE)
						cmd->ByteMode = (inp.ReadBit()) ? true : false;
					else
						cmd->ByteMode = 0;

					int opNum = (kCmdFlags[(uint)cmd->OpCode] & CF_OPMASK);

					if(opNum > 0) {
						DecodeArg(inp, cmd->Op1, cmd->ByteMode);
						if(opNum == 2)
							DecodeArg(inp, cmd->Op2, cmd->ByteMode);
						else {
							if(cmd->Op1.Type == OP_TYPE_INT &&
								(kCmdFlags[(uint)cmd->OpCode] &
									(CF_JUMP | CF_PROC))) {
								int dist = cmd->Op1.Data;
								if(dist >= 256)
									dist -= 256;
								else {
									if(dist >= 136)
										dist -= 264;
									else if(dist >= 16)
										dist -= 8;
									else if(dist >= 8)
										dist -= 16;
									dist += Commands.Size() - 1;
								}
								cmd->Op1.Data = dist;
							}
						}
					}
					if(cmd->ByteMode) {
						switch(cmd->OpCode) {
							case CMD_MOV: cmd->OpCode = CMD_MOVB; break;
							case CMD_CMP: cmd->OpCode = CMD_CMPB; break;
							case CMD_ADD: cmd->OpCode = CMD_ADDB; break;
							case CMD_SUB: cmd->OpCode = CMD_SUBB; break;
							case CMD_INC: cmd->OpCode = CMD_INCB; break;
							case CMD_DEC: cmd->OpCode = CMD_DECB; break;
							case CMD_XOR: cmd->OpCode = CMD_XORB; break;
							case CMD_AND: cmd->OpCode = CMD_ANDB; break;
							case CMD_OR: cmd->OpCode = CMD_ORB; break;
							case CMD_TEST: cmd->OpCode = CMD_TESTB; break;
							case CMD_NEG: cmd->OpCode = CMD_NEGB; break;
							case CMD_SHL: cmd->OpCode = CMD_SHLB; break;
							case CMD_SHR: cmd->OpCode = CMD_SHRB; break;
							case CMD_SAR: cmd->OpCode = CMD_SARB; break;
							case CMD_MUL: cmd->OpCode = CMD_MULB; break;
						}
					}
				}
			}

			#endif

			#ifdef RARVM_STANDARD_FILTERS

			enum EStandardFilter {
				SF_E8,
				SF_E8E9,
				SF_ITANIUM,
				SF_RGB,
				SF_AUDIO,
				SF_DELTA
				// SF_UPCASE
			};

			static const struct CStandardFilterSignature {
				uint32 Length;
				uint32 CRC;
				EStandardFilter Type;
			}

			kStdFilters[] = {
				{  53, 0xad576887, SF_E8 },
				{  57, 0x3cd7e57e, SF_E8E9 },
				{ 120, 0x3769893f, SF_ITANIUM },
				{  29, 0x0e06077d, SF_DELTA },
				{ 149, 0x1c2c5dc8, SF_RGB },
				{ 216, 0xbc85e701, SF_AUDIO }
				// {  40, 0x46b9c560, SF_UPCASE }
			};

			static int FindStandardFilter(const Byte *code, uint32 codeSize)
			{
				uint32 crc = CrcCalc(code, codeSize);
				for(uint i = 0; i < ARRAY_SIZE(kStdFilters); i++) {
					const CStandardFilterSignature &sfs = kStdFilters[i];
					if(sfs.CRC == crc && sfs.Length == codeSize)
						return i;
				}
				return -1;
			}

			#endif

			bool CProgram::PrepareProgram(const Byte *code, uint32 codeSize)
			{
				IsSupported = false;
			#ifdef RARVM_VM_ENABLE
				Commands.Clear();
			#endif

			#ifdef RARVM_STANDARD_FILTERS
				StandardFilterIndex = -1;
			#endif
				bool isOK = false;
				Byte xorSum = 0;
				for(uint32 i = 0; i < codeSize; i++)
					xorSum ^= code[i];
				if(xorSum == 0 && codeSize != 0) {
					IsSupported = true;
					isOK = true;
			#ifdef RARVM_STANDARD_FILTERS
					StandardFilterIndex = FindStandardFilter(code, codeSize);
					if(StandardFilterIndex >= 0)
						return true;
			#endif

			#ifdef RARVM_VM_ENABLE
					ReadProgram(code + 1, codeSize - 1);
			#else
					IsSupported = false;
			#endif
				}
			#ifdef RARVM_VM_ENABLE
				Commands.Add(CCommand());
				Commands.Back().OpCode = CMD_RET;
			#endif
				return isOK;
			}

			void CVm::SetMemory(uint32 pos, const Byte *data, uint32 dataSize)
			{
				if(pos < kSpaceSize && data != Mem + pos)
					memmove(Mem + pos, data, MyMin(dataSize, kSpaceSize - pos));
			}

			#ifdef RARVM_STANDARD_FILTERS

			static void E8E9Decode(Byte *data, uint32 dataSize, uint32 fileOffset, bool e9)
			{
				if(dataSize <= 4)
					return;
				dataSize -= 4;
				const uint32 kFileSize = 0x1000000;
				Byte cmpMask = (Byte)(e9 ? 0xFE : 0xFF);
				for(uint32 curPos = 0; curPos < dataSize; ) {
					curPos++;
					if(((*data++) & cmpMask) == 0xE8) {
						uint32 offset = curPos + fileOffset;
						uint32 addr = (int32)GetValue32(data);
						if(addr < kFileSize)
							SetValue32(data, addr - offset);
						else if((int32)addr < 0 && (int32)(addr + offset) >= 0)
							SetValue32(data, addr + kFileSize);
						data += 4;
						curPos += 4;
					}
				}
			}

			static void ItaniumDecode(Byte *data, uint32 dataSize, uint32 fileOffset)
			{
				if(dataSize <= 21)
					return;
				fileOffset >>= 4;
				dataSize -= 21;
				dataSize += 15;
				dataSize >>= 4;
				dataSize += fileOffset;
				do {
					unsigned m = ((uint32)0x334B0000 >> (data[0] & 0x1E)) & 3;
					if(m) {
						m++;
						do {
							Byte * p = data + ((size_t)m * 5 - 8);
							if(((p[3] >> m) & 15) == 5) {
								const uint32 kMask = 0xFFFFF;
								// uint32 raw = ((uint32)p[0]) | ((uint32)p[1] << 8) | ((uint32)p[2] << 16);
								uint32 raw = GetUi32(p);
								uint32 v = raw >> m;
								v -= fileOffset;
								v &= kMask;
								raw &= ~(kMask << m);
								raw |= (v << m);
								// p[0] = (Byte)raw; p[1] = (Byte)(raw >> 8); p[2] = (Byte)(raw >> 16);
								SetUi32(p, raw);
							}
						} while(++m <= 4);
					}
					data += 16;
				} while(++fileOffset != dataSize);
			}

			static void DeltaDecode(Byte *data, uint32 dataSize, uint32 numChannels)
			{
				uint32 srcPos = 0;
				uint32 border = dataSize * 2;
				for(uint32 curChannel = 0; curChannel < numChannels; curChannel++) {
					Byte prevByte = 0;
					for(uint32 destPos = dataSize + curChannel;
						destPos < border;
						destPos += numChannels)
						data[destPos] = (prevByte = (Byte)(prevByte - data[srcPos++]));
				}
			}

			static void RgbDecode(Byte *srcData, uint32 dataSize, uint32 width, uint32 posR)
			{
				Byte * destData = srcData + dataSize;
				const uint32 numChannels = 3;
				for(uint32 curChannel = 0; curChannel < numChannels; curChannel++) {
					Byte prevByte = 0;
					for(uint32 i = curChannel; i < dataSize; i += numChannels) {
						unsigned int predicted;
						if(i < width)
							predicted = prevByte;
						else {
							unsigned int upperLeftByte = destData[i - width];
							unsigned int upperByte = destData[i - width + 3];
							predicted = prevByte + upperByte - upperLeftByte;
							int pa = abs((int)(predicted - prevByte));
							int pb = abs((int)(predicted - upperByte));
							int pc = abs((int)(predicted - upperLeftByte));
							if(pa <= pb && pa <= pc)
								predicted = prevByte;
							else if(pb <= pc)
								predicted = upperByte;
							else
								predicted = upperLeftByte;
						}
						destData[i] = prevByte = (Byte)(predicted - *(srcData++));
					}
				}
				if(dataSize < 3)
					return;
				for(uint32 i = posR, border = dataSize - 2; i < border; i += 3) {
					Byte g = destData[i+1];
					destData[i    ] = (Byte)(destData[i    ] + g);
					destData[i+2] = (Byte)(destData[i+2] + g);
				}
			}
			static void AudioDecode(Byte *srcData, uint32 dataSize, uint32 numChannels)
			{
				Byte * destData = srcData + dataSize;
				for(uint32 curChannel = 0; curChannel < numChannels; curChannel++) {
					uint32 prevByte = 0, prevDelta = 0, dif[7];
					int32 D1 = 0, D2 = 0, D3;
					int32 K1 = 0, K2 = 0, K3 = 0;
					memzero(dif, sizeof(dif));
					for(uint32 i = curChannel, byteCount = 0; i < dataSize; i += numChannels, byteCount++) {
						D3 = D2;
						D2 = prevDelta - D1;
						D1 = prevDelta;
						uint32 predicted = 8 * prevByte + K1 * D1 + K2 * D2 + K3 * D3;
						predicted = (predicted >> 3) & 0xFF;
						uint32 curByte = *(srcData++);
						predicted -= curByte;
						destData[i] = (Byte)predicted;
						prevDelta = (uint32)(int32)(signed char)(predicted - prevByte);
						prevByte = predicted;
						int32 D = ((int32)(signed char)curByte) << 3;
						dif[0] += abs(D);
						dif[1] += abs(D - D1);
						dif[2] += abs(D + D1);
						dif[3] += abs(D - D2);
						dif[4] += abs(D + D2);
						dif[5] += abs(D - D3);
						dif[6] += abs(D + D3);
						if((byteCount & 0x1F) == 0) {
							uint32 minDif = dif[0], numMinDif = 0;
							dif[0] = 0;
							for(uint j = 1; j < ARRAY_SIZE(dif); j++) {
								if(dif[j] < minDif) {
									minDif = dif[j];
									numMinDif = j;
								}
								dif[j] = 0;
							}
							switch(numMinDif) {
								case 1: if(K1 >= -16) K1--; break;
								case 2: if(K1 <   16) K1++; break;
								case 3: if(K2 >= -16) K2--; break;
								case 4: if(K2 <   16) K2++; break;
								case 5: if(K3 >= -16) K3--; break;
								case 6: if(K3 <   16) K3++; break;
							}
						}
					}
				}
			}
			/*
			   static uint32 UpCaseDecode(Byte *data, uint32 dataSize)
			   {
			   uint32 srcPos = 0, destPos = dataSize;
			   while (srcPos < dataSize) {
				Byte curByte = data[srcPos++];
				if(curByte == 2 && (curByte = data[srcPos++]) != 2)
				  curByte -= 32;
				data[destPos++] = curByte;
			   }
			   return destPos - dataSize;
			   }
			 */
			void CVm::ExecuteStandardFilter(unsigned filterIndex)
			{
				uint32 dataSize = R[4];
				if(dataSize >= kGlobalOffset)
					return;
				EStandardFilter filterType = kStdFilters[filterIndex].Type;
				switch(filterType) {
					case SF_E8:
					case SF_E8E9:
					E8E9Decode(Mem, dataSize, R[6], (filterType == SF_E8E9));
					break;
					case SF_ITANIUM:
					ItaniumDecode(Mem, dataSize, R[6]);
					break;
					case SF_DELTA:
					if(dataSize >= kGlobalOffset / 2)
						break;
					SetBlockPos(dataSize);
					DeltaDecode(Mem, dataSize, R[0]);
					break;
					case SF_RGB:
					if(dataSize >= kGlobalOffset / 2)
						break;
					{
						uint32 width = R[0];
						if(width <= 3)
							break;
						SetBlockPos(dataSize);
						RgbDecode(Mem, dataSize, width, R[1]);
					}
					break;
					case SF_AUDIO:
					if(dataSize >= kGlobalOffset / 2)
						break;
					SetBlockPos(dataSize);
					AudioDecode(Mem, dataSize, R[0]);
					break;
					/*
						case SF_UPCASE:
						if(dataSize >= kGlobalOffset / 2)
						break;
						uint32 destSize = UpCaseDecode(Mem, dataSize);
						SetBlockSize(destSize);
						SetBlockPos(dataSize);
						break;
						*/
				}
			}
			#endif
		}
	}
	namespace NRar5 {
		static const size_t kInputBufSize = 1 << 20;

		void CBitDecoder::SetCheck2()
		{
			_bufCheck2 = _bufCheck;
			if(_bufCheck > _buf) {
				uint64 processed = GetProcessedSize_Round();
				if(_blockEnd < processed)
					_bufCheck2 = _buf;
				else {
					uint64 delta = _blockEnd - processed;
					if((size_t)(_bufCheck - _buf) > delta)
						_bufCheck2 = _buf + (size_t)delta;
				}
			}
		}
		bool CBitDecoder::IsBlockOverRead() const
		{
			uint64 v = GetProcessedSize_Round();
			if(v < _blockEnd)
				return false;
			else if(v > _blockEnd)
				return true;
			else
				return _bitPos > _blockEndBits7;
		}

		//CBitDecoder::CBitDecoder() throw(): _buf(0), _bufLim(0), _bufBase(0), _stream(0), _processedSize(0), _wasFinished(false) {}

		void CBitDecoder::Init() throw()
		{
			_blockEnd = 0;
			_blockEndBits7 = 0;
			_bitPos = 0;
			_processedSize = 0;
			_buf = _bufBase;
			_bufLim = _bufBase;
			_bufCheck = _buf;
			_bufCheck2 = _buf;
			_wasFinished = false;
		}

		void CBitDecoder::Prepare() throw()
		{
			if(_buf >= _bufCheck)
				Prepare2();
		}

		bool CBitDecoder::ExtraBitsWereRead() const { return _buf >= _bufLim && (_buf > _bufLim || _bitPos != 0); }
		bool CBitDecoder::InputEofError() const { return ExtraBitsWereRead(); }
		uint CBitDecoder::GetProcessedBits7() const { return _bitPos; }
		uint64 CBitDecoder::GetProcessedSize_Round() const { return _processedSize + (_buf - _bufBase); }
		uint64 CBitDecoder::GetProcessedSize() const { return _processedSize + (_buf - _bufBase) + ((_bitPos + 7) >> 3); }

		void CBitDecoder::AlignToByte()
		{
			_buf += (_bitPos + 7) >> 3;
			_bitPos = 0;
		}

		Byte CBitDecoder::ReadByteInAligned() { return *_buf++; }

		uint32 FASTCALL CBitDecoder::GetValue(uint numBits)
		{
			uint32 v = ((uint32)_buf[0] << 16) | ((uint32)_buf[1] << 8) | (uint32)_buf[2];
			v >>= (24 - numBits - _bitPos);
			return v & ((1 << numBits) - 1);
		}

		void FASTCALL CBitDecoder::MovePos(uint numBits)
		{
			_bitPos += numBits;
			_buf += (_bitPos >> 3);
			_bitPos &= 7;
		}

		uint32 FASTCALL CBitDecoder::ReadBits9(uint numBits)
		{
			const Byte * buf = _buf;
			uint32 v = ((uint32)buf[0] << 8) | (uint32)buf[1];
			v &= ((uint32)0xFFFF >> _bitPos);
			numBits += _bitPos;
			v >>= (16 - numBits);
			_buf = buf + (numBits >> 3);
			_bitPos = numBits & 7;
			return v;
		}

		uint32 FASTCALL CBitDecoder::ReadBits9fix(uint numBits)
		{
			const Byte * buf = _buf;
			uint32 v = ((uint32)buf[0] << 8) | (uint32)buf[1];
			uint32 mask = ((1 << numBits) - 1);
			numBits += _bitPos;
			v >>= (16 - numBits);
			_buf = buf + (numBits >> 3);
			_bitPos = numBits & 7;
			return v & mask;
		}

		uint32 FASTCALL CBitDecoder::ReadBits32(uint numBits)
		{
			uint32 mask = ((1 << numBits) - 1);
			numBits += _bitPos;
			const Byte * buf = _buf;
			uint32 v = GetBe32(buf);
			if(numBits > 32) {
				v <<= (numBits - 32);
				v |= (uint32)buf[4] >> (40 - numBits);
			}
			else
				v >>= (32 - numBits);
			_buf = buf + (numBits >> 3);
			_bitPos = numBits & 7;
			return v & mask;
		}

		void CBitDecoder::Prepare2() throw()
		{
			const uint kSize = 16;
			if(_buf > _bufLim)
				return;
			size_t rem = _bufLim - _buf;
			if(rem != 0)
				memmove(_bufBase, _buf, rem);
			_bufLim = _bufBase + rem;
			_processedSize += (_buf - _bufBase);
			_buf = _bufBase;
			if(!_wasFinished) {
				uint32 processed = (uint32)(kInputBufSize - rem);
				_hres = _stream->Read(_bufLim, (uint32)processed, &processed);
				_bufLim += processed;
				_wasFinished = (processed == 0);
				if(_hres != S_OK) {
					_wasFinished = true;
					// throw CInBufferException(result);
				}
			}
			rem = _bufLim - _buf;
			_bufCheck = _buf;
			if(rem < kSize)
				memset(_bufLim, 0xFF, kSize - rem);
			else
				_bufCheck = _bufLim - kSize;
			SetCheck2();
		}

		enum FilterType {
			FILTER_DELTA = 0,
			FILTER_E8,
			FILTER_E8E9,
			FILTER_ARM
		};

		static const size_t kWriteStep = (size_t)1 << 22;

		CDecoder::CDecoder() : _window(NULL), _winPos(0), _winSizeAllocated(0), _lzSize(0), _lzEnd(0),
			_writtenFileSize(0), _dictSizeLog(0), _isSolid(false), _wasInit(false), _inputBuf(NULL)
		{
		}

		CDecoder::~CDecoder()
		{
			::MidFree(_window);
			::MidFree(_inputBuf);
		}

		void CDecoder::InitFilters()
		{
			_numUnusedFilters = 0;
			_filters.Clear();
		}

		void CDecoder::DeleteUnusedFilters()
		{
			if(_numUnusedFilters != 0) {
				_filters.DeleteFrontal(_numUnusedFilters);
				_numUnusedFilters = 0;
			}
		}

		HRESULT CDecoder::WriteData(const Byte * data, size_t size)
		{
			HRESULT res = S_OK;
			if(!_unpackSize_Defined || _writtenFileSize < _unpackSize) {
				size_t cur = size;
				if(_unpackSize_Defined) {
					uint64 rem = _unpackSize - _writtenFileSize;
					if(cur > rem)
						cur = (size_t)rem;
				}
				res = WriteStream(_outStream, data, cur);
				if(res != S_OK)
					_writeError = true;
			}
			_writtenFileSize += size;
			return res;
		}

		HRESULT CDecoder::ExecuteFilter(const CFilter &f)
		{
			bool useDest = false;
			Byte * data = _filterSrc;
			uint32 dataSize = f.Size;
			// printf("\nType = %d offset = %9d  size = %5d", f.Type, (uint)(f.Start - _lzFileStart), dataSize);
			switch(f.Type) {
				case FILTER_E8:
				case FILTER_E8E9:
				{
					// printf("  FILTER_E8");
					if(dataSize > 4) {
						dataSize -= 4;
						uint32 fileOffset = (uint32)(f.Start - _lzFileStart);
						const uint32 kFileSize = (uint32)1 << 24;
						Byte cmpMask = (Byte)(f.Type == FILTER_E8 ? 0xFF : 0xFE);
						for(uint32 curPos = 0; curPos < dataSize; ) {
							curPos++;
							if(((*data++) & cmpMask) == 0xE8) {
								uint32 offset = (curPos + fileOffset) & (kFileSize - 1);
								uint32 addr = GetUi32(data);

								if(addr < kFileSize) {
									SetUi32(data, addr - offset);
								}
								else if(addr > ((uint32)0xFFFFFFFF - offset)) { // (addr > ~(offset))
									SetUi32(data, addr + kFileSize);
								}

								data += 4;
								curPos += 4;
							}
						}
					}
					break;
				}
				case FILTER_ARM:
				{
					if(dataSize >= 4) {
						dataSize -= 4;
						uint32 fileOffset = (uint32)(f.Start - _lzFileStart);
						for(uint32 curPos = 0; curPos <= dataSize; curPos += 4) {
							Byte * d = data + curPos;
							if(d[3] == 0xEB) {
								uint32 offset = d[0] | ((uint32)d[1] << 8) | ((uint32)d[2] << 16);
								offset -= (fileOffset + curPos) >> 2;
								d[0] = (Byte)offset;
								d[1] = (Byte)(offset >> 8);
								d[2] = (Byte)(offset >> 16);
							}
						}
					}
					break;
				}

				case FILTER_DELTA:
				{
					// printf("  channels = %d", f.Channels);
					_filterDst.AllocAtLeast(dataSize);
					if(!_filterDst.IsAllocated())
						return E_OUTOFMEMORY;
					Byte * dest = _filterDst;
					uint32 numChannels = f.Channels;
					for(uint32 curChannel = 0; curChannel < numChannels; curChannel++) {
						Byte prevByte = 0;
						for(uint32 destPos = curChannel; destPos < dataSize; destPos += numChannels)
							dest[destPos] = (prevByte = (Byte)(prevByte - *data++));
					}
					useDest = true;
					break;
				}
				default:
					_unsupportedFilter = true;
			}
			return WriteData(useDest ? (const Byte *)_filterDst : (const Byte *)_filterSrc, f.Size);
		}

		HRESULT CDecoder::WriteBuf()
		{
			DeleteUnusedFilters();
			for(uint i = 0; i < _filters.Size(); ) {
				const CFilter &f = _filters[i];
				uint64 blockStart = f.Start;
				size_t lzAvail = (size_t)(_lzSize - _lzWritten);
				if(lzAvail == 0)
					break;
				if(blockStart > _lzWritten) {
					uint64 rem = blockStart - _lzWritten;
					size_t size = lzAvail;
					if(size > rem)
						size = (size_t)rem;
					if(size != 0) {
						RINOK(WriteData(_window + _winPos - lzAvail, size));
						_lzWritten += size;
					}
					continue;
				}

				uint32 blockSize = f.Size;
				size_t offset = (size_t)(_lzWritten - blockStart);
				if(offset == 0) {
					_filterSrc.AllocAtLeast(blockSize);
					if(!_filterSrc.IsAllocated())
						return E_OUTOFMEMORY;
				}

				size_t blockRem = (size_t)blockSize - offset;
				size_t size = lzAvail;
				if(size > blockRem)
					size = blockRem;
				memcpy(_filterSrc + offset, _window + _winPos - lzAvail, size);
				_lzWritten += size;
				offset += size;
				if(offset != blockSize)
					return S_OK;

				_numUnusedFilters = ++i;
				RINOK(ExecuteFilter(f));
			}

			DeleteUnusedFilters();

			if(!_filters.IsEmpty())
				return S_OK;

			size_t lzAvail = (size_t)(_lzSize - _lzWritten);
			RINOK(WriteData(_window + _winPos - lzAvail, lzAvail));
			_lzWritten += lzAvail;
			return S_OK;
		}

		static uint32 ReadUInt32(CBitDecoder &bi)
		{
			unsigned numBytes = bi.ReadBits9fix(2) + 1;
			uint32 v = 0;
			for(uint i = 0; i < numBytes; i++)
				v += ((uint32)bi.ReadBits9fix(8) << (i * 8));
			return v;
		}

		static const uint MAX_UNPACK_FILTERS = 8192;

		HRESULT CDecoder::AddFilter(CBitDecoder &_bitStream)
		{
			DeleteUnusedFilters();
			if(_filters.Size() >= MAX_UNPACK_FILTERS) {
				RINOK(WriteBuf());
				DeleteUnusedFilters();
				if(_filters.Size() >= MAX_UNPACK_FILTERS) {
					_unsupportedFilter = true;
					InitFilters();
				}
			}

			_bitStream.Prepare();

			CFilter f;
			uint32 blockStart = ReadUInt32(_bitStream);
			f.Size = ReadUInt32(_bitStream);

			// if(f.Size > ((uint32)1 << 16)) _unsupportedFilter = true;

			f.Type = (Byte)_bitStream.ReadBits9fix(3);
			f.Channels = 0;
			if(f.Type == FILTER_DELTA)
				f.Channels = (Byte)(_bitStream.ReadBits9fix(5) + 1);
			f.Start = _lzSize + blockStart;

			if(f.Start < _filterEnd)
				_unsupportedFilter = true;
			else {
				_filterEnd = f.Start + f.Size;
				if(f.Size != 0)
					_filters.Add(f);
			}
			return S_OK;
		}

		#define RIF(x) { if(!(x)) return S_FALSE; }

		HRESULT CDecoder::ReadTables(CBitDecoder &_bitStream)
		{
			if(_progress) {
				uint64 packSize = _bitStream.GetProcessedSize();
				RINOK(_progress->SetRatioInfo(&packSize, &_writtenFileSize));
			}
			_bitStream.AlignToByte();
			_bitStream.Prepare();
			unsigned flags = _bitStream.ReadByteInAligned();
			unsigned checkSum = _bitStream.ReadByteInAligned();
			checkSum ^= flags;
			uint32 blockSize;
			{
				unsigned num = (flags >> 3) & 3;
				if(num == 3)
					return S_FALSE;
				blockSize = _bitStream.ReadByteInAligned();
				if(num > 0) {
					blockSize += (uint32)_bitStream.ReadByteInAligned() << 8;
					if(num > 1)
						blockSize += (uint32)_bitStream.ReadByteInAligned() << 16;
				}
			}
			checkSum ^= blockSize ^ (blockSize >> 8) ^ (blockSize >> 16);
			if((Byte)checkSum != 0x5A)
				return S_FALSE;
			unsigned blockSizeBits7 = (flags & 7) + 1;
			if(blockSize == 0 && blockSizeBits7 != 8)
				return S_FALSE;
			blockSize += (blockSizeBits7 >> 3);
			blockSize--;
			_bitStream._blockEndBits7 = (Byte)(blockSizeBits7 & 7);
			_bitStream._blockEnd = _bitStream.GetProcessedSize_Round() + blockSize;
			_bitStream.SetCheck2();
			_isLastBlock = ((flags & 0x40) != 0);
			if((flags & 0x80) == 0) {
				if(!_tableWasFilled && blockSize != 0)
					return S_FALSE;
				return S_OK;
			}
			_tableWasFilled = false;
			{
				Byte lens2[kLevelTableSize];
				for(uint i = 0; i < kLevelTableSize; ) {
					_bitStream.Prepare();
					uint len = (uint)_bitStream.ReadBits9fix(4);
					if(len == 15) {
						unsigned num = (uint)_bitStream.ReadBits9fix(4);
						if(num != 0) {
							num += 2;
							num += i;
							if(num > kLevelTableSize)
								num = kLevelTableSize;
							do {
								lens2[i++] = 0;
							} while(i < num);
							continue;
						}
					}
					lens2[i++] = (Byte)len;
				}
				if(_bitStream.IsBlockOverRead())
					return S_FALSE;
				RIF(m_LevelDecoder.Build(lens2));
			}
			Byte lens[kTablesSizesSum];
			unsigned i = 0;
			while(i < kTablesSizesSum) {
				if(_bitStream._buf >= _bitStream._bufCheck2) {
					if(_bitStream._buf >= _bitStream._bufCheck)
						_bitStream.Prepare();
					if(_bitStream.IsBlockOverRead())
						return S_FALSE;
				}
				uint32 sym = m_LevelDecoder.Decode(&_bitStream);
				if(sym < 16)
					lens[i++] = (Byte)sym;
				else if(sym > kLevelTableSize)
					return S_FALSE;
				else {
					sym -= 16;
					unsigned sh = ((sym & 1) << 2);
					unsigned num = (uint)_bitStream.ReadBits9(3 + sh) + 3 + (sh << 1);
					num += i;
					if(num > kTablesSizesSum)
						num = kTablesSizesSum;
					if(sym < 2) {
						if(i == 0) {
							// return S_FALSE;
							continue; // original unRAR
						}
						Byte v = lens[(size_t)i - 1];
						do {
							lens[i++] = v;
						} while(i < num);
					}
					else {
						do {
							lens[i++] = 0;
						} while(i < num);
					}
				}
			}
			if(_bitStream.IsBlockOverRead())
				return S_FALSE;
			if(_bitStream.InputEofError())
				return S_FALSE;
			RIF(m_MainDecoder.Build(&lens[0]));
			RIF(m_DistDecoder.Build(&lens[kMainTableSize]));
			RIF(m_AlignDecoder.Build(&lens[kMainTableSize + kDistTableSize]));
			RIF(m_LenDecoder.Build(&lens[kMainTableSize + kDistTableSize + kAlignTableSize]));
			_useAlignBits = false;
			// _useAlignBits = true;
			for(i = 0; i < kAlignTableSize; i++)
				if(lens[kMainTableSize + kDistTableSize + (size_t)i] != kNumAlignBits) {
					_useAlignBits = true;
					break;
				}
			_tableWasFilled = true;
			return S_OK;
		}

		static inline unsigned SlotToLen(CBitDecoder &_bitStream, unsigned slot)
		{
			if(slot < 8)
				return (slot + 2);
			else {
				uint   numBits = (slot >> 2) - 1;
				return (2 + ((4 | (slot & 3)) << numBits) + _bitStream.ReadBits9(numBits));
			}
		}

		static const uint32 kSymbolRep = 258;
		// static const uint kMaxMatchLen = 0x1001 + 3;

		HRESULT CDecoder::DecodeLZ()
		{
			CBitDecoder _bitStream;
			_bitStream._stream = _inStream;
			_bitStream._bufBase = _inputBuf;
			_bitStream.Init();
			uint32 rep0 = _reps[0];
			uint32 remLen = 0;
			size_t limit;
			{
				size_t rem = _winSize - _winPos;
				if(rem > kWriteStep)
					rem = kWriteStep;
				limit = _winPos + rem;
			}
			for(;; ) {
				if(_winPos >= limit) {
					RINOK(WriteBuf());
					if(_unpackSize_Defined && _writtenFileSize > _unpackSize)
						break;  // return S_FALSE;
					{
						size_t rem = _winSize - _winPos;
						if(rem == 0) {
							_winPos = 0;
							rem = _winSize;
						}
						if(rem > kWriteStep)
							rem = kWriteStep;
						limit = _winPos + rem;
					}
					if(remLen != 0) {
						size_t winPos = _winPos;
						size_t winMask = _winMask;
						size_t pos = (winPos - (size_t)rep0 - 1) & winMask;
						Byte * win = _window;
						do {
							if(winPos >= limit)
								break;
							win[winPos] = win[pos];
							winPos++;
							pos = (pos + 1) & winMask;
						} while(--remLen != 0);
						_lzSize += winPos - _winPos;
						_winPos = winPos;
						continue;
					}
				}
				if(_bitStream._buf >= _bitStream._bufCheck2) {
					if(_bitStream.InputEofError())
						break;  // return S_FALSE;
					if(_bitStream._buf >= _bitStream._bufCheck)
						_bitStream.Prepare2();
					uint64 processed = _bitStream.GetProcessedSize_Round();
					if(processed >= _bitStream._blockEnd) {
						if(processed > _bitStream._blockEnd)
							break;  // return S_FALSE;
						{
							unsigned bits7 = _bitStream.GetProcessedBits7();
							if(bits7 > _bitStream._blockEndBits7)
								break;  // return S_FALSE;
							if(bits7 == _bitStream._blockEndBits7) {
								if(_isLastBlock) {
									_reps[0] = rep0;
									if(_bitStream.InputEofError())
										break;
									/*
									   // packSize can be 15 bytes larger for encrypted archive
									   if(_packSize_Defined && _packSize <
										  _bitStream.GetProcessedSize())
									   break;
									 */

									return _bitStream._hres;
									// break;
								}
								RINOK(ReadTables(_bitStream));
								continue;
							}
						}
					}
				}
				uint32 sym = m_MainDecoder.Decode(&_bitStream);
				if(sym < 256) {
					size_t winPos = _winPos;
					_window[winPos] = (Byte)sym;
					_winPos = winPos + 1;
					_lzSize++;
					continue;
				}
				uint32 len;
				if(sym < kSymbolRep + kNumReps) {
					if(sym >= kSymbolRep) {
						if(sym != kSymbolRep) {
							uint32 dist;
							if(sym == kSymbolRep + 1)
								dist = _reps[1];
							else {
								if(sym == kSymbolRep + 2)
									dist = _reps[2];
								else {
									dist = _reps[3];
									_reps[3] = _reps[2];
								}
								_reps[2] = _reps[1];
							}
							_reps[1] = rep0;
							rep0 = dist;
						}
						const uint32 sym2 = m_LenDecoder.Decode(&_bitStream);
						if(sym2 >= kLenTableSize)
							break;  // return S_FALSE;
						len = SlotToLen(_bitStream, sym2);
					}
					else {
						if(sym == 256) {
							RINOK(AddFilter(_bitStream));
							continue;
						}
						else { // if(sym == 257)
							len = _lastLen;
							// if(len = 0), we ignore that symbol, like original unRAR code, but it can
							// mean error in stream.
							// if(len == 0) return S_FALSE;
							if(len == 0)
								continue;
						}
					}
				}
				else if(sym >= kMainTableSize)
					break;  // return S_FALSE;
				else {
					_reps[3] = _reps[2];
					_reps[2] = _reps[1];
					_reps[1] = rep0;
					len = SlotToLen(_bitStream, sym - (kSymbolRep + kNumReps));
					rep0 = m_DistDecoder.Decode(&_bitStream);
					if(rep0 >= 4) {
						if(rep0 >= _numCorrectDistSymbols)
							break;  // return S_FALSE;
						unsigned numBits = (rep0 >> 1) - 1;
						rep0 = (2 | (rep0 & 1)) << numBits;
						if(numBits < kNumAlignBits)
							rep0 += _bitStream.ReadBits9(numBits);
						else {
							len += (numBits >= 7);
							len += (numBits >= 12);
							len += (numBits >= 17);
							if(_useAlignBits) {
								// if(numBits > kNumAlignBits)
								rep0 += (_bitStream.ReadBits32(numBits - kNumAlignBits) << kNumAlignBits);
								uint32 a = m_AlignDecoder.Decode(&_bitStream);
								if(a >= kAlignTableSize)
									break;  // return S_FALSE;
								rep0 += a;
							}
							else
								rep0 += _bitStream.ReadBits32(numBits);
						}
					}
				}
				_lastLen = len;
				if(rep0 >= _lzSize)
					_lzError = true;
				{
					uint32 lenCur = len;
					size_t winPos = _winPos;
					size_t pos = (winPos - (size_t)rep0 - 1) & _winMask;
					{
						size_t rem = limit - winPos;
						// size_t rem = _winSize - winPos;
						if(lenCur > rem) {
							lenCur = (uint32)rem;
							remLen = len - lenCur;
						}
					}
					Byte * win = _window;
					_lzSize += lenCur;
					_winPos = winPos + lenCur;
					if(_winSize - pos >= lenCur) {
						const Byte * src = win + pos;
						Byte * dest = win + winPos;
						do {
							*dest++ = *src++;
						} while(--lenCur != 0);
					}
					else {
						do {
							win[winPos] = win[pos];
							winPos++;
							pos = (pos + 1) & _winMask;
						} while(--lenCur != 0);
					}
				}
			}
			return (_bitStream._hres != S_OK) ? _bitStream._hres : S_FALSE;
		}

		HRESULT CDecoder::CodeReal()
		{
			_unsupportedFilter = false;
			_lzError = false;
			_writeError = false;
			if(!_isSolid || !_wasInit) {
				size_t clearSize = _winSize;
				if(_lzSize < _winSize)
					clearSize = (size_t)_lzSize;
				memzero(_window, clearSize);
				_wasInit = true;
				_lzSize = 0;
				_lzWritten = 0;
				_winPos = 0;
				for(uint i = 0; i < kNumReps; i++)
					_reps[i] = (uint32)0 - 1;
				_lastLen = 0;
				_tableWasFilled = false;
			}
			_isLastBlock = false;
			InitFilters();
			_filterEnd = 0;
			_writtenFileSize = 0;
			_lzFileStart = _lzSize;
			_lzWritten = _lzSize;
			HRESULT res = DecodeLZ();
			HRESULT res2 = S_OK;
			if(!_writeError && res != E_OUTOFMEMORY)
				res2 = WriteBuf();

			/*
			   if(res == S_OK)
			   if(InputEofError())
				res = S_FALSE;
			 */

			if(res == S_OK)
				res = res2;

			if(res == S_OK && _unpackSize_Defined && _writtenFileSize != _unpackSize)
				return S_FALSE;
			return res;
		}

		// Original unRAR claims that maximum possible filter block size is (1 << 16) now,
		// and (1 << 17) is minimum win size required to support filter.
		// Original unRAR uses (1 << 18) for "extra safety and possible filter area size expansion"
		// We can use any win size.

		static const uint kWinSize_Log_Min = 17;

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * /* inSize */, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try {
				if(_dictSizeLog >= sizeof(size_t) * 8)
					return E_NOTIMPL;
				if(!_isSolid)
					_lzEnd = 0;
				else {
					if(_lzSize < _lzEnd) {
						if(_window) {
							uint64 rem = _lzEnd - _lzSize;
							if(rem >= _winSize)
								memzero(_window, _winSize);
							else {
								size_t pos = (size_t)_lzSize & _winSize;
								size_t rem2 = _winSize - pos;
								if(rem2 > rem)
									rem2 = (size_t)rem;
								memzero(_window + pos, rem2);
								rem -= rem2;
								memzero(_window, (size_t)rem);
							}
						}
						_lzEnd &= ((((uint64)1) << 33) - 1);
						_lzSize = _lzEnd;
						_winPos = (size_t)(_lzSize & _winSize);
					}
					_lzEnd = _lzSize;
				}
				size_t newSize;
				{
					unsigned newSizeLog = _dictSizeLog;
					if(newSizeLog < kWinSize_Log_Min)
						newSizeLog = kWinSize_Log_Min;
					newSize = (size_t)1 << newSizeLog;
					_numCorrectDistSymbols = newSizeLog * 2;
				}
				// If dictionary was reduced, we use allocated dictionary block
				// for compatibility with original unRAR decoder.
				if(_window && newSize < _winSizeAllocated)
					_winSize = _winSizeAllocated;
				else if(!_window || _winSize != newSize) {
					if(!_isSolid) {
						::MidFree(_window);
						_window = NULL;
						_winSizeAllocated = 0;
					}
					Byte * win;
					{
						win = static_cast<Byte *>(::MidAlloc(newSize));
						if(!win)
							return E_OUTOFMEMORY;
						memzero(win, newSize);
					}
					if(_isSolid && _window) {
						// original unRAR claims:
						// "Archiving code guarantees that win size does not grow in the same solid stream",
						// but the original unRAR decoder still supports such grow case.

						Byte * winOld = _window;
						size_t oldSize = _winSize;
						size_t newMask = newSize - 1;
						size_t oldMask = _winSize - 1;
						size_t winPos = _winPos;
						for(size_t i = 1; i <= oldSize; i++)
							win[(winPos - i) & newMask] = winOld[(winPos - i) & oldMask];
						::MidFree(_window);
					}
					_window = win;
					_winSizeAllocated = newSize;
					_winSize = newSize;
				}
				_winMask = _winSize - 1;
				_winPos &= _winMask;
				if(!_inputBuf) {
					_inputBuf = static_cast<Byte *>(::MidAlloc(kInputBufSize));
					if(!_inputBuf)
						return E_OUTOFMEMORY;
				}
				_inStream = inStream;
				_outStream = outStream;
				/*
				   _packSize = 0;
				   _packSize_Defined = (inSize != NULL);
				   if(_packSize_Defined)
				   _packSize = *inSize;
				 */
				_unpackSize = 0;
				_unpackSize_Defined = (outSize != NULL);
				if(_unpackSize_Defined)
					_unpackSize = *outSize;
				if((int64)_unpackSize >= 0)
					_lzEnd += _unpackSize;
				else
					_lzEnd = 0;
				_progress = progress;

				HRESULT res = CodeReal();

				if(res != S_OK)
					return res;
				if(_lzError)
					return S_FALSE;
				if(_unsupportedFilter)
					return E_NOTIMPL;
				return S_OK;
			}
			// catch(const CInBufferException &e)  { return e.ErrorCode; }
			// catch(...) { return S_FALSE; }
			catch(...) { return E_OUTOFMEMORY; }
			// CNewException is possible here. But probably CNewException is caused
			// by error in data stream.
		}

		STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * data, uint32 size)
		{
			if(size != 2)
				return E_NOTIMPL;
			_dictSizeLog = (Byte)((data[0] & 0xF) + 17);
			_isSolid = ((data[1] & 1) != 0);
			return S_OK;
		}
	}
}
//
// RarHandler.cpp Rar5Handler.cpp
#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

namespace NArchive {
	namespace NRar {
		#define SIGNATURE_RAR { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 }

		static const Byte kMarker[NHeader::kMarkerSize] = SIGNATURE_RAR;
		const uint kPasswordLen_MAX = 127;

		bool CItem::IgnoreItem() const
		{
			switch(HostOS) {
				case NHeader::NFile::kHostMSDOS:
				case NHeader::NFile::kHostOS2:
				case NHeader::NFile::kHostWin32: return ((Attrib & NHeader::NFile::kLabelFileAttribute) != 0);
			}
			return false;
		}

		bool CItem::IsDir() const
		{
			if(GetDictSize() == NHeader::NFile::kDictDirectoryValue)
				return true;
			switch(HostOS) {
				case NHeader::NFile::kHostMSDOS:
				case NHeader::NFile::kHostOS2:
				case NHeader::NFile::kHostWin32:
					if((Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
						return true;
			}
			return false;
		}

		uint32 CItem::GetWinAttrib() const
		{
			uint32 a;
			switch(HostOS) {
				case NHeader::NFile::kHostMSDOS:
				case NHeader::NFile::kHostOS2:
				case NHeader::NFile::kHostWin32:
					a = Attrib;
					break;
				default:
					a = 0; // must be converted from unix value;
			}
			if(IsDir())
				a |= NHeader::NFile::kWinFileDirectoryAttributeMask;
			return a;
		}

		static const char * const kHostOS[] = { "MS DOS", "OS/2", "Win32", "Unix", "Mac OS", "BeOS" };
		static const char * const k_Flags[] =
		{
			"Volume", "Comment", "Lock", "Solid", "NewVolName" /* pack_comment in old versuons */, "Authenticity", "Recovery", "BlockEncryption", "FirstVolume", "EncryptVer" // 9
		};

		enum EErrorType {
			k_ErrorType_OK,
			k_ErrorType_Corrupted,
			k_ErrorType_UnexpectedEnd,
			k_ErrorType_DecryptionError
		};

		class CInArchive {
			IInStream * m_Stream;
			uint64 m_StreamStartPosition;
			UString _unicodeNameBuffer;
			CByteBuffer _comment;
			CByteBuffer m_FileHeaderData;
			NHeader::NBlock::CBlock m_BlockHeader;
			NCrypto::NRar3::CDecoder * m_RarAESSpec;
			CMyComPtr<ICompressFilter> m_RarAES;
			CByteBuffer m_DecryptedData;
			Byte * m_DecryptedDataAligned;
			uint32 m_DecryptedDataSize;
			bool m_CryptoMode;
			uint32 m_CryptoPos;

			HRESULT ReadBytesSpec(void * data, size_t * size);
			bool ReadBytesAndTestSize(void * data, uint32 size);
			void ReadName(const Byte * p, unsigned nameSize, CItem &item);
			bool ReadHeaderReal(const Byte * p, uint size, CItem &item);

			HRESULT Open2(IInStream * stream, const uint64 * searchHeaderSizeLimit);
			void AddToSeekValue(uint64 addValue)
			{
				m_Position += addValue;
			}
			void FinishCryptoBlock()
			{
				if(m_CryptoMode)
					while((m_CryptoPos & 0xF) != 0) {
						m_CryptoPos++;
						m_Position++;
					}
			}
		public:
			uint64 m_Position;
			CInArcInfo ArcInfo;
			bool HeaderErrorWarning;
			HRESULT Open(IInStream * inStream, const uint64 * searchHeaderSizeLimit);
			HRESULT GetNextItem(CItem &item, ICryptoGetTextPassword * getTextPassword, bool &filled, EErrorType &error);
		};

		static bool CheckHeaderCrc(const Byte * header, size_t headerSize)
		{
			return Get16(header) == static_cast<uint16>(CrcCalc(header + 2, headerSize - 2) & 0xFFFF);
		}

		HRESULT CInArchive::Open(IInStream * stream, const uint64 * searchHeaderSizeLimit)
		{
			HeaderErrorWarning = false;
			m_CryptoMode = false;
			RINOK(stream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
			RINOK(stream->Seek(0, STREAM_SEEK_END, &ArcInfo.FileSize));
			RINOK(stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
			m_Position = m_StreamStartPosition;
			uint64 arcStartPos = m_StreamStartPosition;
			{
				Byte marker[NHeader::kMarkerSize];
				RINOK(ReadStream_FALSE(stream, marker, NHeader::kMarkerSize));
				if(memcmp(marker, kMarker, NHeader::kMarkerSize) == 0)
					m_Position += NHeader::kMarkerSize;
				else {
					if(searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
						return S_FALSE;
					RINOK(stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
					RINOK(FindSignatureInStream(stream, kMarker, NHeader::kMarkerSize, searchHeaderSizeLimit, arcStartPos));
					m_Position = arcStartPos + NHeader::kMarkerSize;
					RINOK(stream->Seek(m_Position, STREAM_SEEK_SET, NULL));
				}
			}
			Byte buf[NHeader::NArchive::kArchiveHeaderSize + 1];

			RINOK(ReadStream_FALSE(stream, buf, NHeader::NArchive::kArchiveHeaderSize));
			AddToSeekValue(NHeader::NArchive::kArchiveHeaderSize);
			uint32 blockSize = Get16(buf + 5);
			ArcInfo.EncryptVersion = 0;
			ArcInfo.Flags = Get16(buf + 3);
			uint32 headerSize = NHeader::NArchive::kArchiveHeaderSize;

			/*
			   if(ArcInfo.IsThereEncryptVer())
			   {
			   if(blockSize <= headerSize)
				return S_FALSE;
			   RINOK(ReadStream_FALSE(stream, buf + NHeader::NArchive::kArchiveHeaderSize, 1));
			   AddToSeekValue(1);
			   ArcInfo.EncryptVersion = buf[NHeader::NArchive::kArchiveHeaderSize];
			   headerSize += 1;
			   }
			 */
			if(blockSize < headerSize || buf[2] != NHeader::NBlockType::kArchiveHeader || !CheckHeaderCrc(buf, headerSize))
				return S_FALSE;
			size_t commentSize = blockSize - headerSize;
			_comment.Alloc(commentSize);
			RINOK(ReadStream_FALSE(stream, _comment, commentSize));
			AddToSeekValue(commentSize);
			m_Stream = stream;
			ArcInfo.StartPos = arcStartPos;
			return S_OK;
		}

		HRESULT CInArchive::ReadBytesSpec(void * data, size_t * resSize)
		{
			if(m_CryptoMode) {
				size_t size = *resSize;
				*resSize = 0;
				const Byte * bufData = m_DecryptedDataAligned;
				uint32 bufSize = m_DecryptedDataSize;
				size_t i;
				for(i = 0; i < size && m_CryptoPos < bufSize; i++)
					((Byte *)data)[i] = bufData[m_CryptoPos++];
				*resSize = i;
				return S_OK;
			}
			return ReadStream(m_Stream, data, resSize);
		}

		bool CInArchive::ReadBytesAndTestSize(void * data, uint32 size)
		{
			size_t processed = size;
			return (ReadBytesSpec(data, &processed) != S_OK) ? false : (processed == size);
		}

		static unsigned DecodeUnicodeFileName(const Byte * name, const Byte * encName, unsigned encSize, wchar_t * unicodeName, unsigned maxDecSize)
		{
			unsigned encPos = 0;
			unsigned decPos = 0;
			unsigned flagBits = 0;
			Byte flags = 0;
			if(encPos >= encSize)
				return 0;  // error
			const uint highBits = ((uint)encName[encPos++]) << 8;

			while(encPos < encSize && decPos < maxDecSize) {
				if(flagBits == 0) {
					flags = encName[encPos++];
					flagBits = 8;
				}
				if(encPos >= encSize)
					break;  // error
				uint len = encName[encPos++];

				flagBits -= 2;
				const uint mode = (flags >> flagBits) & 3;

				if(mode != 3) {
					if(mode == 1)
						len += highBits;
					else if(mode == 2) {
						if(encPos >= encSize)
							break;  // error
						len += ((uint)encName[encPos++] << 8);
					}
					unicodeName[decPos++] = (wchar_t)len;
				}
				else {
					if(len & 0x80) {
						if(encPos >= encSize)
							break;  // error
						Byte correction = encName[encPos++];
						for(len = (len & 0x7f) + 2; len > 0 && decPos < maxDecSize; len--, decPos++)
							unicodeName[decPos] = (wchar_t)(((name[decPos] + correction) & 0xff) + highBits);
					}
					else
						for(len += 2; len > 0 && decPos < maxDecSize; len--, decPos++)
							unicodeName[decPos] = name[decPos];
				}
			}
			return decPos < maxDecSize ? decPos : maxDecSize - 1;
		}

		void CInArchive::ReadName(const Byte * p, unsigned nameSize, CItem &item)
		{
			item.UnicodeName.Empty();
			if(nameSize > 0) {
				uint i;
				for(i = 0; i < nameSize && p[i] != 0; i++) 
					;
				item.Name.SetFrom((const char *)p, i);

				if(item.HasUnicodeName()) {
					if(i < nameSize) {
						i++;
						unsigned uNameSizeMax = MyMin(nameSize, (uint)0x400);
						uint len = DecodeUnicodeFileName(p, p + i, nameSize - i, _unicodeNameBuffer.GetBuf(
										uNameSizeMax), uNameSizeMax);
						_unicodeNameBuffer.ReleaseBuf_SetEnd(len);
						item.UnicodeName = _unicodeNameBuffer;
					}
					else if(!ConvertUTF8ToUnicode(item.Name, item.UnicodeName))
						item.UnicodeName.Empty();
				}
			}
			else
				item.Name.Empty();
		}

		static int ReadTime(const Byte * p, uint size, Byte mask, CRarTime &rarTime)
		{
			rarTime.LowSecond = (Byte)(((mask & 4) != 0) ? 1 : 0);
			unsigned numDigits = (mask & 3);
			rarTime.SubTime[0] = rarTime.SubTime[1] = rarTime.SubTime[2] = 0;
			if(numDigits > size)
				return -1;
			for(uint i = 0; i < numDigits; i++)
				rarTime.SubTime[3 - numDigits + i] = p[i];
			return numDigits;
		}

		#define READ_TIME(_mask_, _ttt_) \
			{ int size2 = ReadTime(p, size, _mask_, _ttt_); if(size2 < 0) return false; p += (uint)size2, size -= (uint)size2; }

		#define READ_TIME_2(_mask_, _def_, _ttt_) \
			_def_ = ((_mask_ & 8) != 0); if(_def_) \
			{ if(size < 4) return false; \
			  _ttt_.DosTime = Get32(p); p += 4; size -= 4; \
			  READ_TIME(_mask_, _ttt_); } \


		bool CInArchive::ReadHeaderReal(const Byte * p, uint size, CItem &item)
		{
			const Byte * pStart = p;
			item.Clear();
			item.Flags = m_BlockHeader.Flags;
			const uint kFileHeaderSize = 25;
			if(size < kFileHeaderSize)
				return false;
			item.PackSize = Get32(p);
			item.Size = Get32(p + 4);
			item.HostOS = p[8];
			item.FileCRC = Get32(p + 9);
			item.MTime.DosTime = Get32(p + 13);
			item.UnPackVersion = p[17];
			item.Method = p[18];
			unsigned nameSize = Get16(p + 19);
			item.Attrib = Get32(p + 21);
			item.MTime.LowSecond = 0;
			item.MTime.SubTime[0] = item.MTime.SubTime[1] = item.MTime.SubTime[2] = 0;
			p += kFileHeaderSize;
			size -= kFileHeaderSize;
			if((item.Flags & NHeader::NFile::kSize64Bits) != 0) {
				if(size < 8)
					return false;
				item.PackSize |= ((uint64)Get32(p) << 32);
				item.Size |= ((uint64)Get32(p + 4) << 32);
				p += 8;
				size -= 8;
			}
			if(nameSize > size)
				return false;
			ReadName(p, nameSize, item);
			p += nameSize;
			size -= nameSize;

			/*
			   // It was commented, since it's difficult to support alt Streams for solid archives.
			   if(m_BlockHeader.Type == NHeader::NBlockType::kSubBlock) {
			   if(item.HasSalt()) {
				if(size < sizeof(item.Salt))
				  return false;
				size -= sizeof(item.Salt);
				p += sizeof(item.Salt);
			   }
			   if(item.Name == "ACL" && size == 0) {
				item.IsAltStream = true;
				item.Name.Empty();
				item.UnicodeName.SetFromAscii(".ACL");
			   }
			   else if(item.Name == "STM" && size != 0 && (size & 1) == 0) {
				item.IsAltStream = true;
				item.Name.Empty();
				for(uint32 i = 0; i < size; i += 2) {
				  wchar_t c = Get16(p + i);
				  if(c == 0)
					return false;
				  item.UnicodeName += c;
				}
			   }
			   }
			 */
			if(item.HasSalt()) {
				if(size < sizeof(item.Salt))
					return false;
				/*for(uint i = 0; i < sizeof(item.Salt); i++)
					item.Salt[i] = p[i];*/
				memcpy(item.Salt, p, sizeof(item.Salt));
				p += sizeof(item.Salt);
				size -= sizeof(item.Salt);
			}
			// some rar archives have HasExtTime flag without field.
			if(size >= 2 && item.HasExtTime()) {
				Byte aMask = (Byte)(p[0] >> 4);
				Byte b = p[1];
				p += 2;
				size -= 2;
				Byte mMask = (Byte)(b >> 4);
				Byte cMask = (Byte)(b & 0xF);
				if((mMask & 8) != 0) {
					READ_TIME(mMask, item.MTime);
				}
				READ_TIME_2(cMask, item.CTimeDefined, item.CTime);
				READ_TIME_2(aMask, item.ATimeDefined, item.ATime);
			}
			unsigned fileHeaderWithNameSize = 7 + (uint)(p - pStart);
			item.Position = m_Position;
			item.MainPartSize = fileHeaderWithNameSize;
			item.CommentSize = static_cast<uint16>(m_BlockHeader.HeadSize - fileHeaderWithNameSize);
			if(m_CryptoMode)
				item.AlignSize = static_cast<uint16>((16 - ((m_BlockHeader.HeadSize) & 0xF)) & 0xF);
			else
				item.AlignSize = 0;
			AddToSeekValue(m_BlockHeader.HeadSize);
			// return (m_BlockHeader.Type != NHeader::NBlockType::kSubBlock || item.IsAltStream);
			return true;
		}

		HRESULT CInArchive::GetNextItem(CItem &item, ICryptoGetTextPassword * getTextPassword, bool &filled, EErrorType &error)
		{
			filled = false;
			error = k_ErrorType_OK;
			for(;; ) {
				m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL);
				ArcInfo.EndPos = m_Position;
				if(!m_CryptoMode && (ArcInfo.Flags &
								NHeader::NArchive::kBlockHeadersAreEncrypted) != 0) {
					m_CryptoMode = false;
					if(getTextPassword == 0) {
						error = k_ErrorType_DecryptionError;
						return S_OK; // return S_FALSE;
					}
					if(!m_RarAES) {
						m_RarAESSpec = new NCrypto::NRar3::CDecoder;
						m_RarAES = m_RarAESSpec;
					}
					// m_RarAESSpec->SetRar350Mode(ArcInfo.IsEncryptOld());

					// Salt
					const uint32 kSaltSize = 8;
					Byte salt[kSaltSize];
					if(!ReadBytesAndTestSize(salt, kSaltSize))
						return S_FALSE;
					m_Position += kSaltSize;
					RINOK(m_RarAESSpec->SetDecoderProperties2(salt, kSaltSize))
					// Password
					CMyComBSTR password;
					RINOK(getTextPassword->CryptoGetTextPassword(&password))
					uint len = 0;
					if(password)
						len = sstrlen(password);
					if(len > kPasswordLen_MAX)
						len = kPasswordLen_MAX;

					CByteArr buffer(len * 2);
					for(uint i = 0; i < len; i++) {
						wchar_t c = password[i];
						((Byte *)buffer)[i * 2] = (Byte)c;
						((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
					}

					m_RarAESSpec->SetPassword((const Byte *)buffer, len * 2);

					const uint32 kDecryptedBufferSize = (1 << 12);
					if(m_DecryptedData.Size() == 0) {
						const uint32 kAlign = 16;
						m_DecryptedData.Alloc(kDecryptedBufferSize + kAlign);
						m_DecryptedDataAligned = (Byte *)((ptrdiff_t)((Byte *)m_DecryptedData + kAlign - 1) & ~(ptrdiff_t)(kAlign - 1));
					}
					RINOK(m_RarAES->Init());
					size_t decryptedDataSizeT = kDecryptedBufferSize;
					RINOK(ReadStream(m_Stream, m_DecryptedDataAligned, &decryptedDataSizeT));
					m_DecryptedDataSize = (uint32)decryptedDataSizeT;
					m_DecryptedDataSize = m_RarAES->Filter(m_DecryptedDataAligned, m_DecryptedDataSize);

					m_CryptoMode = true;
					m_CryptoPos = 0;
				}

				m_FileHeaderData.AllocAtLeast(7);
				size_t processed = 7;
				RINOK(ReadBytesSpec((Byte *)m_FileHeaderData, &processed));
				if(processed != 7) {
					if(processed != 0)
						error = k_ErrorType_UnexpectedEnd;
					ArcInfo.EndPos = m_Position + processed; // test it
					return S_OK;
				}

				const Byte * p = m_FileHeaderData;
				m_BlockHeader.CRC = Get16(p + 0);
				m_BlockHeader.Type = p[2];
				m_BlockHeader.Flags = Get16(p + 3);
				m_BlockHeader.HeadSize = Get16(p + 5);

				if(m_BlockHeader.HeadSize < 7) {
					error = k_ErrorType_Corrupted;
					return S_OK;
					// ThrowExceptionWithCode(CInArchiveException::kIncorrectArchive);
				}

				if(m_BlockHeader.Type < NHeader::NBlockType::kFileHeader ||
							m_BlockHeader.Type > NHeader::NBlockType::kEndOfArchive) {
					error = m_CryptoMode ?
								k_ErrorType_DecryptionError :
								k_ErrorType_Corrupted;
					return S_OK;
				}

				if(m_BlockHeader.Type == NHeader::NBlockType::kEndOfArchive) {
					bool footerError = false;

					unsigned expectHeadLen = 7;
					if(m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_DataCRC)
						expectHeadLen += 4;
					if(m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_VolNumber)
						expectHeadLen += 2;
					if(m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_RevSpace)
						expectHeadLen += 7;

					// rar 5.0 beta 1 writes incorrect RevSpace and headSize

					if(m_BlockHeader.HeadSize < expectHeadLen)
						HeaderErrorWarning = true;

					if(m_BlockHeader.HeadSize > 7) {
						/* We suppose that EndOfArchive header is always small.
						   It's only 20 bytes for multivolume
						   Fix the limit, if larger footers are possible */
						if(m_BlockHeader.HeadSize > (1 << 8))
							footerError = true;
						else {
							if(m_FileHeaderData.Size() < m_BlockHeader.HeadSize)
								m_FileHeaderData.ChangeSize_KeepData(m_BlockHeader.HeadSize, 7);
							uint32 afterSize = m_BlockHeader.HeadSize - 7;
							if(ReadBytesAndTestSize(m_FileHeaderData + 7, afterSize))
								processed += afterSize;
							else {
								if(!m_CryptoMode) {
									error = k_ErrorType_UnexpectedEnd;
									return S_OK;
								}
								footerError = true;
							}
						}
					}

					if(footerError || !CheckHeaderCrc(m_FileHeaderData, m_BlockHeader.HeadSize)) {
						error = m_CryptoMode ?
									k_ErrorType_DecryptionError :
									k_ErrorType_Corrupted;
					}
					else {
						ArcInfo.EndFlags = m_BlockHeader.Flags;
						uint32 offset = 7;

						if(m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_DataCRC) {
							if(processed < offset + 4)
								error = k_ErrorType_Corrupted;
							else
								ArcInfo.DataCRC = Get32(m_FileHeaderData + offset);
							offset += 4;
						}

						if(m_BlockHeader.Flags & NHeader::NArchive::kEndOfArc_Flags_VolNumber) {
							if(processed < offset + 2)
								error = k_ErrorType_Corrupted;
							ArcInfo.VolNumber = (uint32)Get16(m_FileHeaderData + offset);
						}

						ArcInfo.EndOfArchive_was_Read = true;
					}

					m_Position += processed;
					FinishCryptoBlock();
					ArcInfo.EndPos = m_Position;
					return S_OK;
				}

				if(m_BlockHeader.Type == NHeader::NBlockType::kFileHeader
									/* || m_BlockHeader.Type == NHeader::NBlockType::kSubBlock */) {
					if(m_FileHeaderData.Size() < m_BlockHeader.HeadSize)
						m_FileHeaderData.ChangeSize_KeepData(m_BlockHeader.HeadSize, 7);
					// m_CurData = (Byte *)m_FileHeaderData;
					// m_PosLimit = m_BlockHeader.HeadSize;
					if(!ReadBytesAndTestSize(m_FileHeaderData + 7, m_BlockHeader.HeadSize - 7)) {
						error = k_ErrorType_UnexpectedEnd;
						return S_OK;
					}

					bool okItem = ReadHeaderReal(m_FileHeaderData + 7, m_BlockHeader.HeadSize - 7, item);
					if(okItem) {
						if(!CheckHeaderCrc(m_FileHeaderData, (uint)m_BlockHeader.HeadSize - item.CommentSize)) {
							error = k_ErrorType_Corrupted; //
												   // ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);
							return S_OK;
						}
						filled = true;
					}

					FinishCryptoBlock();
					m_CryptoMode = false;
					// Move Position to compressed Data;
					m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL);
					AddToSeekValue(item.PackSize); // m_Position points to next header;
					// if(okItem)
					return S_OK;
					/*
					   else
					   continue;
					 */
				}

				if(m_CryptoMode && m_BlockHeader.HeadSize > (1 << 10)) {
					error = k_ErrorType_DecryptionError;
					return S_OK;
				}

				if((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0) {
					if(m_FileHeaderData.Size() < 7 + 4)
						m_FileHeaderData.ChangeSize_KeepData(7 + 4, 7);
					if(!ReadBytesAndTestSize(m_FileHeaderData + 7, 4)) {
						error = k_ErrorType_UnexpectedEnd;
						return S_OK;
					}
					uint32 dataSize = Get32(m_FileHeaderData + 7);
					AddToSeekValue(dataSize);
					if(m_CryptoMode && dataSize > (1 << 27)) {
						error = k_ErrorType_DecryptionError;
						return S_OK;
					}
					m_CryptoPos = m_BlockHeader.HeadSize;
				}
				else
					m_CryptoPos = 0;

				{
					uint64 newPos = m_Position + m_BlockHeader.HeadSize;
					if(newPos > ArcInfo.FileSize) {
						error = k_ErrorType_UnexpectedEnd;
						return S_OK;
					}
				}
				AddToSeekValue(m_BlockHeader.HeadSize);
				FinishCryptoBlock();
				m_CryptoMode = false;
			}
		}
		static const Byte kProps[] = {
			kpidPath,
			kpidIsDir,
			kpidSize,
			kpidPackSize,
			kpidMTime,
			kpidCTime,
			kpidATime,
			kpidAttrib,

			kpidEncrypted,
			kpidSolid,
			kpidCommented,
			kpidSplitBefore,
			kpidSplitAfter,
			kpidCRC,
			kpidHostOS,
			kpidMethod,
			kpidUnpackVer
		};

		static const Byte kArcProps[] =
		{
			kpidTotalPhySize,
			kpidCharacts,
			kpidSolid,
			kpidNumBlocks,
			// kpidEncrypted,
			kpidIsVolume,
			kpidVolumeIndex,
			kpidNumVolumes
			// kpidCommented
		};

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		uint64 CHandler::GetPackSize(unsigned refIndex) const
		{
			const CRefItem &refItem = _refItems[refIndex];
			uint64 totalPackSize = 0;
			for(uint i = 0; i < refItem.NumItems; i++)
				totalPackSize += _items[refItem.ItemIndex + i].PackSize;
			return totalPackSize;
		}

		bool CHandler::IsSolid(unsigned refIndex) const
		{
			const CItem & item = _items[_refItems[refIndex].ItemIndex];
			return (item.UnPackVersion < 20) ? (_arcInfo.IsSolid() ? (refIndex > 0) : false) : item.IsSolid();
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			switch(propID) {
				case kpidVolumeIndex: if(_arcInfo.Is_VolNumber_Defined()) prop = (uint32)_arcInfo.VolNumber; break;
				case kpidSolid: prop = _arcInfo.IsSolid(); break;
				case kpidCharacts:
				{
					AString s(FlagsToString(k_Flags, ARRAY_SIZE(k_Flags), _arcInfo.Flags));
					// FLAGS_TO_PROP(k_Flags, _arcInfo.Flags, prop);
					if(_arcInfo.Is_DataCRC_Defined()) {
						s.Add_Space_if_NotEmpty();
						s += "VolCRC";
					}
					prop = s;
					break;
				}
				// case kpidEncrypted: prop = _arcInfo.IsEncrypted(); break; // it's for encrypted names.
				case kpidIsVolume: prop = _arcInfo.IsVolume(); break;
				case kpidNumVolumes: prop = (uint32)_arcs.Size(); break;
				case kpidOffset: 
					if(_arcs.Size() == 1 && _arcInfo.StartPos != 0) 
						prop = _arcInfo.StartPos; 
					break;
				case kpidTotalPhySize:
					if(_arcs.Size() > 1) {
						uint64 sum = 0;
						FOR_VECTOR(v, _arcs)
						sum += _arcs[v].PhySize;
						prop = sum;
					}
					break;
				case kpidPhySize:
					if(_arcs.Size() != 0)
						prop = _arcInfo.GetPhySize();
					break;
				// case kpidCommented: prop = _arcInfo.IsCommented(); break;
				case kpidNumBlocks:
				{
					uint32 numBlocks = 0;
					FOR_VECTOR(i, _refItems) {
						if(!IsSolid(i))
							numBlocks++;
					}
					prop = (uint32)numBlocks;
					break;
				}
				case kpidError:
				{
					// if(!_errorMessage.IsEmpty()) prop = _errorMessage; break;

					if(/* &_missingVol || */ !_missingVolName.IsEmpty()) {
						UString s("Missing volume : ");
						s += _missingVolName;
						prop = s;
					}
					break;
				}
				case kpidErrorFlags:
				{
					uint32 v = _errorFlags;
					if(!_isArc)
						v |= kpv_ErrorFlags_IsNotArc;
					prop = v;
					break;
				}
				case kpidWarningFlags:
					if(_warningFlags != 0)
						prop = _warningFlags;
					break;
				case kpidExtension:
					if(_arcs.Size() == 1) {
						if(_arcInfo.Is_VolNumber_Defined()) {
							AString s("part");
							uint32 v = (uint32)_arcInfo.VolNumber + 1;
							if(v < 10)
								s += '0';
							s.Add_UInt32(v);
							s += ".rar";
							prop = s;
						}
					}
					break;
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}
		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _refItems.Size();
			return S_OK;
		}
		static bool RarTimeToFileTime(const CRarTime &rarTime, FILETIME &result)
		{
			if(!NTime::DosTimeToFileTime(rarTime.DosTime, result))
				return false;
			else {
				uint64 value =  (((uint64)result.dwHighDateTime) << 32) + result.dwLowDateTime;
				value += (uint64)rarTime.LowSecond * 10000000;
				value += ((uint64)rarTime.SubTime[2] << 16) + ((uint64)rarTime.SubTime[1] << 8) + ((uint64)rarTime.SubTime[0]);
				result.dwLowDateTime = (DWORD)value;
				result.dwHighDateTime = DWORD(value >> 32);
				return true;
			}
		}
		static void RarTimeToProp(const CRarTime &rarTime, NCOM::CPropVariant &prop)
		{
			FILETIME localFileTime, utcFileTime;
			if(RarTimeToFileTime(rarTime, localFileTime)) {
				if(!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
					utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
			}
			else
				utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
			prop = utcFileTime;
		}
		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CRefItem &refItem = _refItems[index];
			const CItem &item = _items[refItem.ItemIndex];
			const CItem &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
			/*
			   const CItem *mainItem = &item;
			   if(item.BaseFileIndex >= 0)
			   mainItem = &_items[_refItems[item.BaseFileIndex].ItemIndex];
			 */
			switch(propID) {
				case kpidPath:
				{
					/*
					   UString u;
					   if(item.BaseFileIndex >= 0)
					   u = mainItem->GetName();
					   u += item.GetName();
					 */
					prop = (const wchar_t *)NItemName::WinPathToOsPath(item.GetName());
					break;
				}
				case kpidIsDir: prop = item.IsDir(); break;
				case kpidSize: if(lastItem.Is_Size_Defined()) prop = lastItem.Size; break;
				case kpidPackSize: prop = GetPackSize(index); break;
				case kpidMTime: RarTimeToProp(item.MTime, prop); break;
				case kpidCTime: if(item.CTimeDefined) RarTimeToProp(item.CTime, prop); break;
				case kpidATime: if(item.ATimeDefined) RarTimeToProp(item.ATime, prop); break;
				case kpidAttrib: prop = item.GetWinAttrib(); break;
				case kpidEncrypted: prop = item.IsEncrypted(); break;
				case kpidSolid: prop = IsSolid(index); break;
				case kpidCommented: prop = item.IsCommented(); break;
				case kpidSplitBefore: prop = item.IsSplitBefore(); break;
				case kpidSplitAfter: prop = _items[refItem.ItemIndex + refItem.NumItems - 1].IsSplitAfter(); break;
				case kpidCRC:
				{
					prop = ((lastItem.IsSplitAfter()) ? item.FileCRC : lastItem.FileCRC);
					break;
				}
				case kpidUnpackVer: prop = item.UnPackVersion; break;
				case kpidMethod:
				{
					char s[16];
					Byte m = item.Method;
					if(m < (Byte)'0' || m > (Byte)'5')
						ConvertUInt32ToString(m, s);
					else {
						s[0] = 'm';
						s[1] = (char)m;
						s[2] = 0;
						if(!item.IsDir()) {
							s[2] = ':';
							ConvertUInt32ToString(16 + item.GetDictSize(), &s[3]);
						}
					}
					prop = s;
					break;
				}
				case kpidHostOS:
					TYPE_TO_PROP(kHostOS, item.HostOS, prop);
					break;
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		HRESULT CHandler::Open2(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback)
		{
			{
				CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
				CMyComPtr<ICryptoGetTextPassword> getTextPassword;
				CVolumeName seqName;
				uint64 totalBytes = 0;
				uint64 curBytes = 0;
				if(openCallback) {
					openCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
					openCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
				}
				bool nextVol_is_Required = false;
				CInArchive archive;
				for(;; ) {
					CMyComPtr<IInStream> inStream;
					if(!_arcs.IsEmpty()) {
						if(!openVolumeCallback)
							break;
						if(_arcs.Size() == 1) {
							if(!_arcInfo.IsVolume())
								break;
							UString baseName;
							{
								NCOM::CPropVariant prop;
								RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
								if(prop.vt != VT_BSTR)
									break;
								baseName = prop.bstrVal;
							}
							if(!seqName.InitName(baseName, _arcInfo.HaveNewVolumeName()))
								break;
							/*
							   if(_arcInfo.HaveNewVolumeName() && !_arcInfo.IsFirstVolume())
							   {
							   seqName.MakeBeforeFirstName();
							   }
							 */
						}
						const UString volName = seqName.GetNextName();
						HRESULT result = openVolumeCallback->GetStream(volName, &inStream);
						if(result != S_OK && result != S_FALSE)
							return result;
						if(!inStream || result != S_OK) {
							if(nextVol_is_Required)
								_missingVolName = volName;
							break;
						}
					}
					else
						inStream = stream;
					uint64 endPos = 0;
					RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
					RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
					if(openCallback) {
						totalBytes += endPos;
						RINOK(openCallback->SetTotal(NULL, &totalBytes));
					}
					RINOK(archive.Open(inStream, maxCheckStartPosition));
					_isArc = true;
					CItem item;
					for(;; ) {
						if(archive.m_Position > endPos) {
							_errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
							break;
						}
						EErrorType error;
						// bool decryptionError;
						// AString errorMessageLoc;
						bool filled;
						HRESULT result = archive.GetNextItem(item, getTextPassword, filled, error);
						if(error != k_ErrorType_OK) {
							if(error == k_ErrorType_UnexpectedEnd)
								_errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
							else if(error == k_ErrorType_Corrupted)
								_errorFlags |= kpv_ErrorFlags_HeadersError;
							else if(error == k_ErrorType_DecryptionError)
								_errorFlags |= kpv_ErrorFlags_EncryptedHeadersError;
							// AddErrorMessage(errorMessageLoc);
						}
						RINOK(result);
						if(!filled) {
							if(error == k_ErrorType_DecryptionError && _items.IsEmpty())
								return S_FALSE;
							if(archive.ArcInfo.ExtraZeroTail_is_Possible()) {
								/* if there is recovery record for multivolume archive,
								   RAR adds 18 bytes (ZERO bytes) at the end for alignment.
								   We must skip these bytes to prevent phySize warning. */
								RINOK(inStream->Seek(archive.ArcInfo.EndPos, STREAM_SEEK_SET, NULL));
								bool areThereNonZeros;
								uint64 numZeros;
								const uint64 maxSize = 1 << 12;
								RINOK(ReadZeroTail(inStream, areThereNonZeros, numZeros, maxSize));
								if(!areThereNonZeros && numZeros != 0 && numZeros <= maxSize)
									archive.ArcInfo.EndPos += numZeros;
							}
							break;
						}
						if(item.IgnoreItem())
							continue;
						bool needAdd = true;
						if(item.IsSplitBefore()) {
							if(!_refItems.IsEmpty()) {
								CRefItem &refItem = _refItems.Back();
								refItem.NumItems++;
								needAdd = false;
							}
						}
						if(needAdd) {
							CRefItem refItem;
							refItem.ItemIndex = _items.Size();
							refItem.NumItems = 1;
							refItem.VolumeIndex = _arcs.Size();
							_refItems.Add(refItem);
						}
						_items.Add(item);
						if(openCallback && _items.Size() % 100 == 0) {
							uint64 numFiles = _items.Size();
							uint64 numBytes = curBytes + item.Position;
							RINOK(openCallback->SetCompleted(&numFiles, &numBytes));
						}
					}
					if(archive.HeaderErrorWarning)
						_warningFlags |= kpv_ErrorFlags_HeadersError;
					/*
					   if(archive.m_Position < endPos)
					   _warningFlags |= kpv_ErrorFlags_DataAfterEnd;
					 */
					if(_arcs.IsEmpty())
						_arcInfo = archive.ArcInfo;
					// _arcInfo.EndPos = archive.EndPos;
					curBytes += endPos;
					{
						CArc &arc = _arcs.AddNew();
						arc.PhySize = archive.ArcInfo.GetPhySize();
						arc.Stream = inStream;
					}
					nextVol_is_Required = false;
					if(!archive.ArcInfo.IsVolume())
						break;
					if(archive.ArcInfo.EndOfArchive_was_Read) {
						if(!archive.ArcInfo.AreMoreVolumes())
							break;
						nextVol_is_Required = true;
					}
				}
			}
			/*
			   int baseFileIndex = -1;
			   for(unsigned i = 0; i < _refItems.Size(); i++) {
			   CItem &item = _items[_refItems[i].ItemIndex];
			   if(item.IsAltStream)
				item.BaseFileIndex = baseFileIndex;
			   else
				baseFileIndex = i;
			   }
			 */
			return S_OK;
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback)
		{
			COM_TRY_BEGIN
			Close();
			// try
			{
				HRESULT res = Open2(stream, maxCheckStartPosition, openCallback);
				/*
				   if(res != S_OK)
				   Close();
				 */
				return res;
			}
			// catch(const CInArchiveException &) { Close(); return S_FALSE; }
			// catch(...) { Close(); throw; }
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			COM_TRY_BEGIN
			// _errorMessage.Empty();
			_missingVolName.Empty();
			_errorFlags = 0;
			_warningFlags = 0;
			_isArc = false;
			_refItems.Clear();
			_items.Clear();
			_arcs.Clear();
			return S_OK;
			COM_TRY_END
		}

		struct CMethodItem {
			Byte RarUnPackVersion;
			CMyComPtr<ICompressCoder> Coder;
		};

		class CVolsInStream : public ISequentialInStream, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP

			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
			void Init(const CObjectVector<CArc> * arcs, const CObjectVector<CItem> * items, const CRefItem &refItem)
			{
				_arcs = arcs;
				_items = items;
				_refItem = refItem;
				_curIndex = 0;
				_stream = NULL;
				CrcIsOK = true;
			}
		private:
			uint64 _rem;
			ISequentialInStream * _stream;
			const CObjectVector<CArc> * _arcs;
			const CObjectVector<CItem> * _items;
			CRefItem _refItem;
			unsigned _curIndex;
			uint32 _crc;
			bool   _calcCrc;
		public:
			bool   CrcIsOK;
			uint8  Reserve[2]; // @alignment
		};

		STDMETHODIMP CVolsInStream::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			uint32 realProcessedSize = 0;
			while(size != 0) {
				if(!_stream) {
					if(_curIndex >= _refItem.NumItems)
						break;
					const CItem &item = (*_items)[_refItem.ItemIndex + _curIndex];
					unsigned volIndex = _refItem.VolumeIndex + _curIndex;
					if(volIndex >= _arcs->Size()) {
						return S_OK;
						// return S_FALSE;
					}
					IInStream * s = (*_arcs)[volIndex].Stream;
					RINOK(s->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
					_stream = s;
					_calcCrc = (CrcIsOK && item.IsSplitAfter());
					_crc = CRC_INIT_VAL;
					_rem = item.PackSize;
				}
				{
					uint32 cur = size;
					if(cur > _rem)
						cur = (uint32)_rem;
					uint32 num = cur;
					HRESULT res = _stream->Read(data, cur, &cur);
					if(_calcCrc)
						_crc = CrcUpdate(_crc, data, cur);
					realProcessedSize += cur;
					ASSIGN_PTR(processedSize, realProcessedSize);
					data = (Byte *)data + cur;
					size -= cur;
					_rem -= cur;
					if(_rem == 0) {
						const CItem &item = (*_items)[_refItem.ItemIndex + _curIndex];
						_curIndex++;
						if(_calcCrc && CRC_GET_DIGEST(_crc) != item.FileCRC)
							CrcIsOK = false;
						_stream = NULL;
					}
					if(res != S_OK)
						return res;
					if(realProcessedSize != 0)
						return S_OK;
					if(cur == 0 && num != 0)
						return S_OK;
				}
			}
			return S_OK;
		}
		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			uint64 censoredTotalUnPacked = 0,
			// censoredTotalPacked = 0,
				importantTotalUnPacked = 0;
			// importantTotalPacked = 0;
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _refItems.Size();
			if(numItems == 0)
				return S_OK;
			uint   lastIndex = 0;
			CRecordVector <unsigned> importantIndexes;
			CRecordVector <bool> extractStatuses;
			bool isThereUndefinedSize = false;
			for(uint32 t = 0; t < numItems; t++) {
				uint   index = allFilesMode ? t : indices[t];
				{
					const CRefItem &refItem = _refItems[index];
					const CItem &item = _items[refItem.ItemIndex + refItem.NumItems - 1];
					if(item.Is_Size_Defined())
						censoredTotalUnPacked += item.Size;
					else
						isThereUndefinedSize = true;
					// censoredTotalPacked += item.PackSize;
				}
				uint   j;
				for(j = lastIndex; j <= index; j++)
					// if(!_items[_refItems[j].ItemIndex].IsSolid())
					if(!IsSolid(j))
						lastIndex = j;
				for(j = lastIndex; j <= index; j++) {
					const CRefItem &refItem = _refItems[j];
					const CItem &item = _items[refItem.ItemIndex + refItem.NumItems - 1];
					if(item.Is_Size_Defined())
						importantTotalUnPacked += item.Size;
					else
						isThereUndefinedSize = true;
					// importantTotalPacked += item.PackSize;
					importantIndexes.Add(j);
					extractStatuses.Add(j == index);
				}
				lastIndex = index + 1;
			}
			if(importantTotalUnPacked != 0 || !isThereUndefinedSize) {
				RINOK(extractCallback->SetTotal(importantTotalUnPacked));
			}
			uint64 currentImportantTotalUnPacked = 0;
			uint64 currentImportantTotalPacked = 0;
			uint64 currentUnPackSize, currentPackSize;
			CObjectVector<CMethodItem> methodItems;
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			CFilterCoder * filterStreamSpec = new CFilterCoder(false);
			CMyComPtr<ISequentialInStream> filterStream = filterStreamSpec;

			NCrypto::NRar2::CDecoder * rar20CryptoDecoderSpec = NULL;
			CMyComPtr<ICompressFilter> rar20CryptoDecoder;
			NCrypto::NRar3::CDecoder * rar3CryptoDecoderSpec = NULL;
			CMyComPtr<ICompressFilter> rar3CryptoDecoder;
			CVolsInStream * volsInStreamSpec = NULL;
			CMyComPtr<ISequentialInStream> volsInStream;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			bool solidStart = true;
			for(uint i = 0;; i++, currentImportantTotalUnPacked += currentUnPackSize, currentImportantTotalPacked += currentPackSize) {
				lps->InSize = currentImportantTotalPacked;
				lps->OutSize = currentImportantTotalUnPacked;
				RINOK(lps->SetCur());
				if(i >= importantIndexes.Size())
					break;
				CMyComPtr<ISequentialOutStream> realOutStream;
				int32 askMode;
				if(extractStatuses[i])
					askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				else
					askMode = NExtractArc::NAskMode::kSkip;
				uint32 index = importantIndexes[i];
				const CRefItem &refItem = _refItems[index];
				const CItem &item = _items[refItem.ItemIndex];
				const CItem &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
				uint64 outSize = static_cast<uint64>(-1LL);
				currentUnPackSize = 0;
				if(lastItem.Is_Size_Defined()) {
					outSize = lastItem.Size;
					currentUnPackSize = outSize;
				}
				currentPackSize = GetPackSize(index);
				if(item.IgnoreItem())
					continue;
				RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
				if(!IsSolid(index))
					solidStart = true;
				if(item.IsDir()) {
					RINOK(extractCallback->PrepareOperation(askMode));
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					continue;
				}
				bool mustBeProcessedAnywhere = false;
				if(i < importantIndexes.Size() - 1) {
					// const CRefItem &nextRefItem = _refItems[importantIndexes[i+1]];
					// const CItem &nextItemInfo = _items[nextRefItem.ItemIndex];
					// mustBeProcessedAnywhere = nextItemInfo.IsSolid();
					mustBeProcessedAnywhere = IsSolid(importantIndexes[i+1]);
				}
				if(!mustBeProcessedAnywhere && !testMode && !realOutStream)
					continue;
				if(!realOutStream && !testMode)
					askMode = NExtractArc::NAskMode::kSkip;
				RINOK(extractCallback->PrepareOperation(askMode));
				COutStreamWithCRC * outStreamSpec = new COutStreamWithCRC;
				CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
				outStreamSpec->SetStream(realOutStream);
				outStreamSpec->Init();
				realOutStream.Release();
				if(!volsInStream) {
					volsInStreamSpec = new CVolsInStream;
					volsInStream = volsInStreamSpec;
				}
				volsInStreamSpec->Init(&_arcs, &_items, refItem);
				uint64 packSize = currentPackSize;
				// packedPos += item.PackSize;
				// unpackedPos += 0;
				CMyComPtr<ISequentialInStream> inStream;
				if(item.IsEncrypted()) {
					// CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
					if(item.UnPackVersion >= 29) {
						if(!rar3CryptoDecoder) {
							rar3CryptoDecoderSpec = new NCrypto::NRar3::CDecoder;
							rar3CryptoDecoder = rar3CryptoDecoderSpec;
						}
						// rar3CryptoDecoderSpec->SetRar350Mode(item.UnPackVersion < 36);
						/*
						   CMyComPtr<ICompressSetDecoderProperties2> cryptoProperties;
						   RINOK(rar3CryptoDecoder.QueryInterface(IID_ICompressSetDecoderProperties2,
							&cryptoProperties));
						 */
						RINOK(rar3CryptoDecoderSpec->SetDecoderProperties2(item.Salt, item.HasSalt() ? sizeof(item.Salt) : 0));
						filterStreamSpec->Filter = rar3CryptoDecoder;
					}
					else if(item.UnPackVersion >= 20) {
						if(!rar20CryptoDecoder) {
							rar20CryptoDecoderSpec = new NCrypto::NRar2::CDecoder;
							rar20CryptoDecoder = rar20CryptoDecoderSpec;
						}
						filterStreamSpec->Filter = rar20CryptoDecoder;
					}
					else {
						outStream.Release();
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
						continue;
					}
					// RINOK(filterStreamSpec->Filter.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword));
					if(!getTextPassword)
						extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
					if(!getTextPassword) {
						outStream.Release();
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
						continue;
					}
					// if(getTextPassword)
					{
						CMyComBSTR password;
						RINOK(getTextPassword->CryptoGetTextPassword(&password));
						if(item.UnPackVersion >= 29) {
							uint len = 0;
							if(password)
								len = sstrlen(password);
							if(len > kPasswordLen_MAX)
								len = kPasswordLen_MAX;
							CByteArr buffer(len * 2);
							for(uint k = 0; k < len; k++) {
								wchar_t c = password[k];
								((Byte *)buffer)[k * 2] = (Byte)c;
								((Byte *)buffer)[k * 2 + 1] = (Byte)(c >> 8);
							}
							rar3CryptoDecoderSpec->SetPassword((const Byte *)buffer, len * 2);
						}
						else {
							AString oemPassword;
							if(password) {
								UString unicode = (LPCOLESTR)password;
								if(unicode.Len() > kPasswordLen_MAX)
									unicode.DeleteFrom(kPasswordLen_MAX);
								oemPassword = UnicodeStringToMultiByte(unicode, CP_OEMCP);
							}
							rar20CryptoDecoderSpec->SetPassword((const Byte *)(const char *)oemPassword, oemPassword.Len());
						}
					}
					/*
					   else
					   {
					   RINOK(cryptoSetPassword->CryptoSetPassword(NULL, 0));
					   }
					 */

					filterStreamSpec->SetInStream(volsInStream);
					filterStreamSpec->SetOutStreamSize(NULL);
					inStream = filterStream;
				}
				else {
					inStream = volsInStream;
				}
				CMyComPtr<ICompressCoder> commonCoder;
				switch(item.Method) {
					case '0':
						commonCoder = copyCoder;
						break;
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					{
						unsigned m;
						for(m = 0; m < methodItems.Size(); m++)
							if(methodItems[m].RarUnPackVersion == item.UnPackVersion)
								break;
						if(m == methodItems.Size()) {
							CMethodItem mi;
							mi.RarUnPackVersion = item.UnPackVersion;
							mi.Coder.Release();
							if(item.UnPackVersion <= 40) {
								uint32 methodID = 0x40300;
								if(item.UnPackVersion < 20)
									methodID += 1;
								else if(item.UnPackVersion < 29)
									methodID += 2;
								else
									methodID += 3;
								RINOK(CreateCoder(EXTERNAL_CODECS_VARS methodID, false, mi.Coder));
							}
							if(mi.Coder == 0) {
								outStream.Release();
								RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
								continue;
							}

							m = methodItems.Add(mi);
						}
						CMyComPtr<ICompressCoder> decoder = methodItems[m].Coder;
						CMyComPtr<ICompressSetDecoderProperties2> compressSetDecoderProperties;
						RINOK(decoder.QueryInterface(IID_ICompressSetDecoderProperties2, &compressSetDecoderProperties));
						Byte isSolid = (Byte)((IsSolid(index) || item.IsSplitBefore()) ? 1 : 0);
						if(solidStart) {
							isSolid = 0;
							solidStart = false;
						}
						RINOK(compressSetDecoderProperties->SetDecoderProperties2(&isSolid, 1));
						commonCoder = decoder;
						break;
					}
					default:
						outStream.Release();
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
						continue;
				}
				HRESULT result = commonCoder->Code(inStream, outStream, &packSize, &outSize, progress);
				if(item.IsEncrypted())
					filterStreamSpec->ReleaseInStream();
				if(outSize == static_cast<uint64>(-1LL))
					currentUnPackSize = outStreamSpec->GetSize();
				int opRes = (volsInStreamSpec->CrcIsOK && outStreamSpec->GetCRC() == lastItem.FileCRC) ?
					NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kCRCError;
				outStream.Release();
				if(result != S_OK) {
					if(result == S_FALSE)
						opRes = NExtractArc::NOperationResult::kDataError;
					else if(result == E_NOTIMPL)
						opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
					else
						return result;
				}
				RINOK(extractCallback->SetOperationResult(opRes));
			}
			return S_OK;
			COM_TRY_END
		}

		IMPL_ISetCompressCodecsInfo

		REGISTER_ARC_I("Rar", "rar r00", 0, 3, kMarker, 0, NArcInfoFlags::kFindSignature, NULL)
	}
	namespace NRar5 {
		static const uint kMarkerSize = 8;

		#define SIGNATURE_RAR5 { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x01, 0	}

		static const Byte kMarker[kMarkerSize] = SIGNATURE_RAR5;
		static const size_t kCommentSize_Max = (size_t)1 << 16;
		static const char * const kHostOS[] = { "Windows", "Unix" };

		static const char * const k_ArcFlags[] = { "Volume", "VolumeField", "Solid", "Recovery", "Lock" /*4*/ };
		static const char * const k_FileFlags[] = { "Dir", "UnixTime", "CRC", "UnknownSize" };
		static const char * const g_ExtraTypes[] = { "0", "Crypto", "Hash", "Time", "Version", "Link", "UnixOwner", "Subdata" };
		static const char * const g_LinkTypes[] = {"0", "UnixSymLink", "WinSymLink", "WinJunction", "HardLink", "FileCopy" };
		static const char g_ExtraTimeFlags[] = { 'u', 'M', 'C', 'A', 'n' };

		static uint FASTCALL ReadVarInt(const Byte * p, size_t maxSize, uint64 * val)
		{
			*val = 0;
			for(uint i = 0; i < maxSize; ) {
				Byte b = p[i];
				if(i < 10)
					*val |= (uint64)(b & 0x7F) << (7 * i);
				i++;
				if((b & 0x80) == 0)
					return i;
			}
			return 0;
		}
		bool CLinkInfo::Parse(const Byte * p, uint size)
		{
			const Byte * pStart = p;
			uint num = ReadVarInt(p, size, &Type);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			num = ReadVarInt(p, size, &Flags);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			uint64 len;
			num = ReadVarInt(p, size, &len);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			if(size != len)
				return false;
			NameLen = (uint)len;
			NameOffset = (uint)(p - pStart);
			return true;
		}
		static void AddHex64(AString &s, uint64 v)
		{
			char sz[32];
			sz[0] = '0';
			sz[1] = 'x';
			ConvertUInt64ToHex(v, sz + 2);
			s += sz;
		}
		static void PrintType(AString &s, const char * const table[], unsigned num, uint64 val)
		{
			char sz[32];
			const char * p = NULL;
			if(val < num)
				p = table[(uint)val];
			if(!p) {
				ConvertUInt64ToString(val, sz);
				p = sz;
			}
			s += p;
		}
		CItem::CItem() 
		{
			Clear();
		}
		void CItem::Clear()
		{
			CommonFlags = 0;
			Flags = 0;
			VolIndex = 0;
			NextItem = -1;
			Version_Defined = false;
			Version = 0;
			Name.Empty();
			Extra.Free();
			ACL = -1;
		}

		bool CItem::IsSplitBefore()  const { return (CommonFlags & NHeaderFlags::kPrevVol) != 0; }
		bool CItem::IsSplitAfter()   const { return (CommonFlags & NHeaderFlags::kNextVol) != 0; }
		bool CItem::IsSplit() const { return (CommonFlags & (NHeaderFlags::kPrevVol | NHeaderFlags::kNextVol)) != 0; }
		bool CItem::IsDir()          const { return (Flags & NFileFlags::kIsDir) != 0; }
		bool CItem::Has_UnixMTime()  const { return (Flags & NFileFlags::kUnixTime) != 0; }
		bool CItem::Has_CRC() const { return (Flags & NFileFlags::kCrc32) != 0; }
		bool CItem::Is_UnknownSize() const { return (Flags & NFileFlags::kUnknownSize) != 0; }

		bool FASTCALL CItem::IsNextForItem(const CItem &prev) const
		{
			return !IsDir() && !prev.IsDir() && IsSplitBefore() && prev.IsSplitAfter() && (Name == prev.Name);
			// && false;
		}

		bool CItem::IsSolid() const { return ((uint32)Method & NMethodFlags::kSolid) != 0; }
		unsigned CItem::GetAlgoVersion() const { return (uint)Method & 0x3F; }
		unsigned CItem::GetMethod() const { return ((uint)Method >> 7) & 0x7; }
		uint32 CItem::GetDictSize() const { return (((uint32)Method >> 10) & 0xF); }
		bool CItem::IsService() const { return RecordType == NHeaderType::kService; }
		bool CItem::Is_STM() const { return IsService() && Name == "STM"; }
		bool CItem::Is_CMT() const { return IsService() && Name == "CMT"; }
		bool CItem::Is_ACL() const { return IsService() && Name == "ACL"; }

		bool CItem::IsEncrypted() const
		{
			uint size;
			return FindExtra(NExtraID::kCrypto, size) >= 0;
		}

		int CItem::FindExtra_Blake() const
		{
			uint size = 0;
			int offset = FindExtra(NExtraID::kHash, size);
			if(offset >= 0 && size == BLAKE2S_DIGEST_SIZE + 1 && Extra[(uint)offset] == kHashID_Blake2sp)
				return offset + 1;
			return -1;
		}

		uint32 CItem::GetWinAttrib() const
		{
			uint32 a;
			switch(HostOS) {
				case kHost_Windows: a = Attrib; break;
				case kHost_Unix: a = (Attrib << 16); break;
				default: a = 0;
			}
			// if(IsDir()) a |= FILE_ATTRIBUTE_DIRECTORY;
			return a;
		}

		int CItem::FindExtra(unsigned extraID, unsigned &recordDataSize) const
		{
			recordDataSize = 0;
			size_t offset = 0;
			for(;; ) {
				size_t rem = Extra.Size() - offset;
				if(rem == 0)
					return -1;
				{
					uint64 size;
					unsigned num = ReadVarInt(Extra + offset, rem, &size);
					if(num == 0)
						return -1;
					offset += num;
					rem -= num;
					if(size > rem)
						return -1;
					rem = (size_t)size;
				}
				{
					uint64 id;
					unsigned num = ReadVarInt(Extra + offset, rem, &id);
					if(num == 0)
						return -1;
					offset += num;
					rem -= num;
					// There was BUG in RAR 5.21- : it stored (size-1) instead of (size)
					// for Subdata record in Service header.
					// That record always was last in bad archives, so we can fix that case.
					if(id == NExtraID::kSubdata && RecordType == NHeaderType::kService && rem + 1 == Extra.Size() - offset)
						rem++;
					if(id == extraID) {
						recordDataSize = (uint)rem;
						return static_cast<int>(offset);
					}
					offset += rem;
				}
			}
		}

		void CItem::PrintInfo(AString &s) const
		{
			size_t offset = 0;
			for(;; ) {
				size_t rem = Extra.Size() - offset;
				if(rem == 0)
					return;
				{
					uint64 size;
					unsigned num = ReadVarInt(Extra + offset, rem, &size);
					if(num == 0)
						return;
					offset += num;
					rem -= num;
					if(size > rem)
						break;
					rem = (size_t)size;
				}
				{
					uint64 id;
					{
						unsigned num = ReadVarInt(Extra + offset, rem, &id);
						if(num == 0)
							break;
						offset += num;
						rem -= num;
					}

					// There was BUG in RAR 5.21- : it stored (size-1) instead of (size)
					// for Subdata record in Service header.
					// That record always was last in bad archives, so we can fix that case.
					if(id == NExtraID::kSubdata && RecordType == NHeaderType::kService && rem + 1 == Extra.Size() - offset)
						rem++;
					s.Add_Space_if_NotEmpty();
					PrintType(s, g_ExtraTypes, ARRAY_SIZE(g_ExtraTypes), id);
					if(id == NExtraID::kTime) {
						const Byte * p = Extra + offset;
						uint64 flags;
						unsigned num = ReadVarInt(p, rem, &flags);
						if(num != 0) {
							s += ':';
							for(uint i = 0; i < ARRAY_SIZE(g_ExtraTimeFlags); i++)
								if((flags & ((uint64)1 << i)) != 0)
									s += g_ExtraTimeFlags[i];
							flags &= ~(((uint64)1 << ARRAY_SIZE(g_ExtraTimeFlags)) - 1);
							if(flags != 0) {
								s += '_';
								AddHex64(s, flags);
							}
						}
					}
					else if(id == NExtraID::kLink) {
						CLinkInfo linkInfo;
						if(linkInfo.Parse(Extra + offset, (uint)rem)) {
							s += ':';
							PrintType(s, g_LinkTypes, ARRAY_SIZE(g_LinkTypes), linkInfo.Type);
							uint64 flags = linkInfo.Flags;
							if(flags != 0) {
								s += ':';
								if(flags & NLinkFlags::kTargetIsDir) {
									s += 'D';
									flags &= ~((uint64)NLinkFlags::kTargetIsDir);
								}
								if(flags != 0) {
									s += '_';
									AddHex64(s, flags);
								}
							}
						}
					}
					offset += rem;
				}
			}
			s.Add_OptSpaced("ERROR");
		}

		struct CCryptoInfo {
			uint64 Algo;
			uint64 Flags;
			Byte   Cnt;
			uint8  Reserve[3]; // @alignment

			bool UseMAC() const { return (Flags & NCryptoFlags::kUseMAC) != 0; }
			bool IsThereCheck() const { return (Flags & NCryptoFlags::kPswCheck) != 0; }
			bool Parse(const Byte * p, size_t size);
		};

		bool CCryptoInfo::Parse(const Byte * p, size_t size)
		{
			Algo = 0;
			Flags = 0;
			Cnt = 0;
			unsigned num = ReadVarInt(p, size, &Algo);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			num = ReadVarInt(p, size, &Flags);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			if(size > 0)
				Cnt = p[0];
			if(size != 1 + 16 + 16 + (uint)(IsThereCheck() ? 12 : 0))
				return false;
			return true;
		}

		bool CItem::FindExtra_Version(uint64 &version) const
		{
			uint size;
			int offset = FindExtra(NExtraID::kVersion, size);
			if(offset < 0)
				return false;
			const Byte * p = Extra + (uint)offset;
			uint64 flags;
			unsigned num = ReadVarInt(p, size, &flags);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			num = ReadVarInt(p, size, &version);
			if(num == 0) 
				return false; 
			p += num; 
			size -= num;
			return size == 0;
		}

		bool CItem::FindExtra_Link(CLinkInfo &link) const
		{
			uint size;
			int offset = FindExtra(NExtraID::kLink, size);
			if(offset < 0)
				return false;
			if(!link.Parse(Extra + (uint)offset, size))
				return false;
			link.NameOffset += offset;
			return true;
		}
		bool CItem::Is_CopyLink() const
		{
			CLinkInfo link;
			return FindExtra_Link(link) && link.Type == NLinkType::kFileCopy;
		}
		bool CItem::Is_HardLink() const
		{
			CLinkInfo link;
			return FindExtra_Link(link) && link.Type == NLinkType::kHardLink;
		}
		bool CItem::Is_CopyLink_or_HardLink() const
		{
			CLinkInfo link;
			return FindExtra_Link(link) && (link.Type == NLinkType::kFileCopy || link.Type == NLinkType::kHardLink);
		}
		void CItem::Link_to_Prop(unsigned linkType, NWindows::NCOM::CPropVariant &prop) const
		{
			CLinkInfo link;
			if(FindExtra_Link(link)) {
				if(link.Type != linkType) {
					if(linkType != NLinkType::kUnixSymLink)
						return;
					switch((uint)link.Type) {
						case NLinkType::kUnixSymLink:
						case NLinkType::kWinSymLink:
						case NLinkType::kWinJunction:
							break;
						default: return;
					}
				}
				AString s;
				s.SetFrom_CalcLen(reinterpret_cast<const char *>(Extra + link.NameOffset), link.NameLen);
				UString unicode;
				if(ConvertUTF8ToUnicode(s, unicode))
					prop = NItemName::GetOsPath(unicode);
			}
		}

		bool CItem::GetAltStreamName(AString &name) const
		{
			name.Empty();
			uint size;
			int offset = FindExtra(NExtraID::kSubdata, size);
			if(offset < 0)
				return false;
			name.SetFrom_CalcLen(reinterpret_cast<const char *>(Extra + (uint)offset), size);
			return true;
		}

		class CHash {
		public:
			void Init_NoCalc()
			{
				_calcCRC = false;
				_crc = CRC_INIT_VAL;
				_blakeOffset = -1;
			}
			void Init(const CItem &item);
			void Update(const void * data, size_t size);
			uint32 GetCRC() const { return CRC_GET_DIGEST(_crc); }
			bool Check(const CItem &item, NCrypto::NRar5::CDecoder * cryptoDecoderSpec);
		private:
			bool   _calcCRC;
			uint8  Reserve[3]; // @alignment
			uint32 _crc;
			int    _blakeOffset;
			CBlake2sp _blake;
		};

		void CHash::Init(const CItem &item)
		{
			_crc = CRC_INIT_VAL;
			_calcCRC = item.Has_CRC();
			_blakeOffset = item.FindExtra_Blake();
			if(_blakeOffset >= 0)
				Blake2sp_Init(&_blake);
		}

		void CHash::Update(const void * data, size_t size)
		{
			if(_calcCRC)
				_crc = CrcUpdate(_crc, data, size);
			if(_blakeOffset >= 0)
				Blake2sp_Update(&_blake, (const Byte *)data, size);
		}

		bool CHash::Check(const CItem &item, NCrypto::NRar5::CDecoder * cryptoDecoderSpec)
		{
			if(_calcCRC) {
				uint32 crc = GetCRC();
				if(cryptoDecoderSpec)
					crc = cryptoDecoderSpec->Hmac_Convert_Crc32(crc);
				if(crc != item.CRC)
					return false;
			}
			if(_blakeOffset >= 0) {
				Byte digest[BLAKE2S_DIGEST_SIZE];
				Blake2sp_Final(&_blake, digest);
				CALLPTRMEMB(cryptoDecoderSpec, Hmac_Convert_32Bytes(digest));
				if(memcmp(digest, &item.Extra[(uint)_blakeOffset], BLAKE2S_DIGEST_SIZE) != 0)
					return false;
			}

			return true;
		}

		class COutStreamWithHash : public ISequentialOutStream, public CMyUnknownImp {
			ISequentialOutStream * _stream;
			uint64 _pos;
			uint64 _size;
			bool _size_Defined;
			Byte * _destBuf;
		public:
			CHash _hash;

			COutStreamWithHash() : _destBuf(NULL) 
			{
			}
			MY_UNKNOWN_IMP
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
			void SetStream(ISequentialOutStream * stream) 
			{
				_stream = stream;
			}
			void Init(const CItem &item, Byte * destBuf)
			{
				_size_Defined = false;
				_size = 0;
				_destBuf = NULL;
				if(!item.Is_UnknownSize()) {
					_size_Defined = true;
					_size = item.Size;
					_destBuf = destBuf;
				}
				_pos = 0;
				_hash.Init(item);
			}

			uint64 GetPos() const {
				return _pos;
			}
		};

		STDMETHODIMP COutStreamWithHash::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			HRESULT result = S_OK;
			if(_size_Defined) {
				uint64 rem = _size - _pos;
				if(size > rem)
					size = (uint32)rem;
			}
			if(_stream)
				result = _stream->Write(data, size, &size);
			if(_destBuf)
				memcpy(_destBuf + (size_t)_pos, data, size);
			_hash.Update(data, size);
			_pos += size;
			ASSIGN_PTR(processedSize, size);
			return result;
		}

		class CInArchive {
		private:
			template <unsigned alignMask> struct CAlignedBuffer {
				Byte * _buf;
				Byte * _bufBase;
				size_t _size;
				CAlignedBuffer() : _buf(NULL), _bufBase(NULL), _size(0) 
				{
				}
				~CAlignedBuffer() 
				{
					SAlloc::F(_bufBase);
				}
			public:
				operator Byte *() { return _buf; }
				operator const Byte *() const { return _buf; }
				void AllocAtLeast(size_t size)
				{
					if(!_buf || _size < size) {
						SAlloc::F(_bufBase);
						_buf = NULL;
						_size = 0;
						_bufBase = static_cast<Byte *>(SAlloc::M(size + alignMask));
						if(_bufBase) {
							_size = size;
							// _buf = (Byte *)(((uintptr_t)_bufBase + alignMask) & ~(uintptr_t)alignMask);
							_buf = (Byte *)(((ptrdiff_t)_bufBase + alignMask) & ~(ptrdiff_t)alignMask);
						}
					}
				}
			};
			CAlignedBuffer <AES_BLOCK_SIZE-1> _buf;
			size_t _bufSize;
			size_t _bufPos;
			ISequentialInStream * _stream;
			NCrypto::NRar5::CDecoder * m_CryptoDecoderSpec;
			CMyComPtr<ICompressFilter> m_CryptoDecoder;
			HRESULT ReadStream_Check(void * data, size_t size);
		public:
			bool m_CryptoMode;
			bool WrongPassword;
			bool IsArc;
			bool UnexpectedEnd;
			uint64 StreamStartPosition;
			uint64 Position;

			bool ReadVar(uint64 &val);

			struct CHeader {
				uint64 Type;
				uint64 Flags;
				size_t ExtraSize;
				uint64 DataSize;
			};

			HRESULT ReadBlockHeader(CHeader &h);
			bool ReadFileHeader(const CHeader &header, CItem &item);
			void AddToSeekValue(uint64 addValue)
			{
				Position += addValue;
			}
			HRESULT Open(IInStream * inStream, const uint64 * searchHeaderSizeLimit, ICryptoGetTextPassword * getTextPassword, CArc::CInArcInfo &info);
		};

		static HRESULT MySetPassword(ICryptoGetTextPassword * getTextPassword, NCrypto::NRar5::CDecoder * cryptoDecoderSpec)
		{
			CMyComBSTR password;
			RINOK(getTextPassword->CryptoGetTextPassword(&password));
			AString utf8;
			const uint kPasswordLen_MAX = 127;
			UString unicode = (LPCOLESTR)password;
			if(unicode.Len() > kPasswordLen_MAX)
				unicode.DeleteFrom(kPasswordLen_MAX);
			ConvertUnicodeToUTF8(unicode, utf8);
			cryptoDecoderSpec->SetPassword((const Byte *)(const char *)utf8, utf8.Len());
			return S_OK;
		}
		bool CInArchive::ReadVar(uint64 &val)
		{
			uint   offset = ReadVarInt(_buf + _bufPos, _bufSize - _bufPos, &val);
			_bufPos += offset;
			return (offset != 0);
		}

		HRESULT CInArchive::ReadStream_Check(void * data, size_t size)
		{
			size_t size2 = size;
			RINOK(ReadStream(_stream, data, &size2));
			if(size2 == size)
				return S_OK;
			UnexpectedEnd = true;
			return S_FALSE;
		}

		HRESULT CInArchive::ReadBlockHeader(CHeader &h)
		{
			h.Type = 0;
			h.Flags = 0;
			h.ExtraSize = 0;
			h.DataSize = 0;

			const  uint kStartSize = 4 + 3;
			const  uint kBufSize = AES_BLOCK_SIZE + AES_BLOCK_SIZE; // must be >= kStartSize;
			Byte   buf[kBufSize];
			uint   filled;
			if(m_CryptoMode) {
				RINOK(ReadStream_Check(buf, kBufSize));
				memcpy(m_CryptoDecoderSpec->_iv, buf, AES_BLOCK_SIZE);
				RINOK(m_CryptoDecoderSpec->Init());
				_buf.AllocAtLeast(1 << 12);
				if(!(Byte *)_buf)
					return E_OUTOFMEMORY;
				memcpy(_buf, buf + AES_BLOCK_SIZE, AES_BLOCK_SIZE);
				if(m_CryptoDecoderSpec->Filter(_buf, AES_BLOCK_SIZE) != AES_BLOCK_SIZE)
					return E_FAIL;
				memcpy(buf, _buf, AES_BLOCK_SIZE);
				filled = AES_BLOCK_SIZE;
			}
			else {
				RINOK(ReadStream_Check(buf, kStartSize));
				filled = kStartSize;
			}
			uint64 val;
			uint   offset = ReadVarInt(buf + 4, 3, &val);
			if(offset == 0)
				return S_FALSE;
			{
				size_t size = (size_t)val;
				_bufPos = (4 + offset);
				_bufSize = _bufPos + size;
				if(size < 2)
					return S_FALSE;
			}
			size_t allocSize = _bufSize;
			if(m_CryptoMode)
				allocSize = (allocSize + AES_BLOCK_SIZE - 1) & ~(size_t)(AES_BLOCK_SIZE - 1);
			_buf.AllocAtLeast(allocSize);
			if(!(Byte *)_buf)
				return E_OUTOFMEMORY;
			memcpy(_buf, buf, filled);
			size_t rem = allocSize - filled;
			AddToSeekValue(allocSize + (m_CryptoMode ? AES_BLOCK_SIZE : 0));
			RINOK(ReadStream_Check(_buf + filled, rem));
			if(m_CryptoMode) {
				if(m_CryptoDecoderSpec->Filter(_buf + filled, (uint32)rem) != rem)
					return E_FAIL;
			}
			if(CrcCalc(_buf + 4, _bufSize - 4) != Get32(buf))
				return S_FALSE;
			if(!ReadVar(h.Type)) return S_FALSE;
			if(!ReadVar(h.Flags)) return S_FALSE;
			if(h.Flags & NHeaderFlags::kExtra) {
				uint64 extraSize;
				if(!ReadVar(extraSize))
					return S_FALSE;
				if(extraSize > _bufSize)
					return S_FALSE;
				h.ExtraSize = (size_t)extraSize;
			}

			if(h.Flags & NHeaderFlags::kData) {
				if(!ReadVar(h.DataSize))
					return S_FALSE;
			}

			return S_OK;
		}
		/*
		   int CInArcInfo::FindExtra(unsigned extraID, unsigned &recordDataSize) const
		   {
		   recordDataSize = 0;
		   size_t offset = 0;
		   for(;;) {
			size_t rem = Extra.Size() - offset;
			if(rem == 0)
			  return -1;
			{
			  uint64 size;
			  unsigned num = ReadVarInt(Extra + offset, rem, &size);
			  if(num == 0)
				return -1;
			  offset += num;
			  rem -= num;
			  if(size > rem)
				return -1;
			  rem = (size_t)size;
			}
			{
			  uint64 id;
			  unsigned num = ReadVarInt(Extra + offset, rem, &id);
			  if(num == 0)
				return -1;
			  offset += num;
			  rem -= num;

			  if(id == extraID)
			  {
				recordDataSize = (uint)rem;
				return static_cast<int>(offset);
			  }

			  offset += rem;
			}
		   }
		   }


		   bool CInArcInfo::FindExtra_Locator(CLocator &locator) const
		   {
		   locator.Flags = 0;
		   locator.QuickOpen = 0;
		   locator.Recovery = 0;

		   uint size;
		   int offset = FindExtra(kArcExtraRecordType_Locator, size);
		   if(offset < 0)
			return false;
		   const Byte *p = Extra + (uint)offset;

		   unsigned num;

		   num = ReadVarInt(p, size, &locator.Flags);
		   if(num == 0) return false; p += num; size -= num;
		   if(locator.Is_QuickOpen()) {
			num = ReadVarInt(p, size, &locator.QuickOpen);
			if(num == 0) return false; p += num; size -= num;
		   }
		   if(locator.Is_Recovery()) {
			num = ReadVarInt(p, size, &locator.Recovery);
			if(num == 0) return false; p += num; size -= num;
		   }
		   return true;
		   }
		 */

		CArc::CInArcInfo::CInArcInfo() : Flags(0), VolNumber(0), StartPos(0), EndPos(0), EndFlags(0), EndOfArchive_was_Read(false), IsEncrypted(false)
		{
		}
		/*
			void Clear()
			{
			Flags = 0;
			VolNumber = 0;
			StartPos = 0;
			EndPos = 0;
			EndFlags = 0;
			EndOfArchive_was_Read = false;
			Extra.Free();
			}
			*/
		uint64 CArc::CInArcInfo::GetPhySize() const { return EndPos - StartPos; }
		bool CArc::CInArcInfo::AreMoreVolumes() const { return (EndFlags & NArcEndFlags::kMoreVols) != 0; }
		bool CArc::CInArcInfo::IsVolume() const { return (Flags & NArcFlags::kVol) != 0; }
		bool CArc::CInArcInfo::IsSolid() const { return (Flags & NArcFlags::kSolid) != 0; }
		bool CArc::CInArcInfo::Is_VolNumber_Defined() const { return (Flags & NArcFlags::kVolNumber) != 0; }
		uint64 CArc::CInArcInfo::GetVolIndex() const { return Is_VolNumber_Defined() ? VolNumber : 0; }

		HRESULT CInArchive::Open(IInStream * stream, const uint64 * searchHeaderSizeLimit, ICryptoGetTextPassword * getTextPassword, CArc::CInArcInfo & info)
		{
			m_CryptoMode = false;
			WrongPassword = false;
			IsArc = false;
			UnexpectedEnd = false;
			Position = StreamStartPosition;
			uint64 arcStartPos = StreamStartPosition;
			{
				Byte marker[kMarkerSize];
				RINOK(ReadStream_FALSE(stream, marker, kMarkerSize));
				if(memcmp(marker, kMarker, kMarkerSize) == 0)
					Position += kMarkerSize;
				else {
					if(searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
						return S_FALSE;
					RINOK(stream->Seek(StreamStartPosition, STREAM_SEEK_SET, NULL));
					RINOK(FindSignatureInStream(stream, kMarker, kMarkerSize, searchHeaderSizeLimit, arcStartPos));
					arcStartPos += StreamStartPosition;
					Position = arcStartPos + kMarkerSize;
					RINOK(stream->Seek(Position, STREAM_SEEK_SET, NULL));
				}
			}
			info.StartPos = arcStartPos;
			_stream = stream;
			CHeader h;
			RINOK(ReadBlockHeader(h));
			info.IsEncrypted = false;
			if(h.Type == NHeaderType::kArcEncrypt) {
				info.IsEncrypted = true;
				IsArc = true;
				if(!getTextPassword)
					return E_NOTIMPL;
				m_CryptoMode = true;
				if(!m_CryptoDecoder) {
					m_CryptoDecoderSpec = new NCrypto::NRar5::CDecoder;
					m_CryptoDecoder = m_CryptoDecoderSpec;
				}
				RINOK(m_CryptoDecoderSpec->SetDecoderProps(_buf + _bufPos, (uint)(_bufSize - _bufPos), false, false));
				RINOK(MySetPassword(getTextPassword, m_CryptoDecoderSpec));
				if(!m_CryptoDecoderSpec->CalcKey_and_CheckPassword()) {
					WrongPassword = True;
					return S_FALSE;
				}
				RINOK(ReadBlockHeader(h));
			}
			if(h.Type != NHeaderType::kArc)
				return S_FALSE;
			IsArc = true;
			info.VolNumber = 0;
			if(!ReadVar(info.Flags))
				return S_FALSE;
			if(info.Flags & NArcFlags::kVolNumber)
				if(!ReadVar(info.VolNumber))
					return S_FALSE;
			if(h.ExtraSize != 0) {
				if(_bufSize - _bufPos < h.ExtraSize)
					return S_FALSE;
				/*
				   info.Extra.Alloc(h.ExtraSize);
				   memcpy(info.Extra, _buf + _bufPos, h.ExtraSize);
				 */
				_bufPos += h.ExtraSize;

				/*
				   CInArcInfo::CLocator locator;
				   if(info.FindExtra_Locator(locator))
				   locator.Flags = locator.Flags;
				 */
			}
			return (_bufPos != _bufSize) ? S_FALSE : S_OK;
		}

		bool CInArchive::ReadFileHeader(const CHeader &header, CItem &item)
		{
			item.UnixMTime = 0;
			item.CRC = 0;
			item.Flags = 0;
			item.CommonFlags = (uint32)header.Flags;
			item.PackSize = header.DataSize;
			uint64 flags64;
			if(!ReadVar(flags64)) 
				return false;
			item.Flags = (uint32)flags64;
			if(!ReadVar(item.Size)) 
				return false;
			{
				uint64 attrib;
				if(!ReadVar(attrib)) return false;
				item.Attrib = (uint32)attrib;
			}
			if(item.Has_UnixMTime()) {
				if(_bufSize - _bufPos < 4)
					return false;
				item.UnixMTime = Get32(_buf + _bufPos);
				_bufPos += 4;
			}
			if(item.Has_CRC()) {
				if(_bufSize - _bufPos < 4)
					return false;
				item.CRC = Get32(_buf + _bufPos);
				_bufPos += 4;
			}
			{
				uint64 method;
				if(!ReadVar(method)) return false;
				item.Method = (uint32)method;
			}

			if(!ReadVar(item.HostOS)) return false;

			{
				uint64 len;
				if(!ReadVar(len)) return false;
				if(len > _bufSize - _bufPos)
					return false;
				item.Name.SetFrom_CalcLen((const char *)(_buf + _bufPos), (uint)len);
				_bufPos += (uint)len;
			}

			item.Extra.Free();
			size_t extraSize = header.ExtraSize;
			if(extraSize != 0) {
				if(_bufSize - _bufPos < extraSize)
					return false;
				item.Extra.Alloc(extraSize);
				memcpy(item.Extra, _buf + _bufPos, extraSize);
				_bufPos += extraSize;
			}

			return (_bufPos == _bufSize);
		}

		struct CLinkFile {
			CLinkFile() : Index(0), NumLinks(0), Res(S_OK), crcOK(true) 
			{
			}
			uint   Index;
			uint   NumLinks;
			CByteBuffer Data;
			HRESULT Res;
			bool   crcOK;
		};

		struct CUnpacker {
			NCompress::CCopyCoder * copyCoderSpec;
			CMyComPtr<ICompressCoder> copyCoder;
			CMyComPtr<ICompressCoder> LzCoders[2];
			bool NeedClearSolid[2];
			CFilterCoder * filterStreamSpec;
			CMyComPtr<ISequentialInStream> filterStream;
			NCrypto::NRar5::CDecoder * cryptoDecoderSpec;
			CMyComPtr<ICompressFilter> cryptoDecoder;
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			COutStreamWithHash * outStreamSpec;
			CMyComPtr<ISequentialOutStream> outStream;
			CByteBuffer _tempBuf;
			CLinkFile * linkFile;

			CUnpacker() : linkFile(NULL) 
			{
				NeedClearSolid[0] = NeedClearSolid[1] = true;
			}
			HRESULT Create(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, bool isSolid, bool &wrongPassword);
			HRESULT Code(const CItem &item, const CItem &lastItem, uint64 packSize,
						ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress, bool &isCrcOK);
			HRESULT DecodeToBuf(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, uint64 packSize, ISequentialInStream * inStream, CByteBuffer &buffer);
		};

		static const uint kLzMethodMax = 5;

		HRESULT CUnpacker::Create(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, bool isSolid, bool &wrongPassword)
		{
			wrongPassword = false;
			if(item.GetAlgoVersion() != 0)
				return E_NOTIMPL;

			if(!outStream) {
				outStreamSpec = new COutStreamWithHash;
				outStream = outStreamSpec;
			}

			unsigned method = item.GetMethod();

			if(method == 0) {
				if(!copyCoder) {
					copyCoderSpec = new NCompress::CCopyCoder;
					copyCoder = copyCoderSpec;
				}
			}
			else {
				if(method > kLzMethodMax)
					return E_NOTIMPL;

				/*
				   if(item.IsSplitBefore())
				   return S_FALSE;
				 */

				int lzIndex = item.IsService() ? 1 : 0;
				CMyComPtr<ICompressCoder> &lzCoder = LzCoders[lzIndex];

				if(!lzCoder) {
					const uint32 methodID = 0x40305;
					RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS methodID, false, lzCoder));
					if(!lzCoder)
						return E_NOTIMPL;
				}
				CMyComPtr<ICompressSetDecoderProperties2> csdp;
				RINOK(lzCoder.QueryInterface(IID_ICompressSetDecoderProperties2, &csdp));
				Byte props[2] = { (Byte)(item.GetDictSize()), (Byte)(isSolid ? 1 : 0) };
				RINOK(csdp->SetDecoderProperties2(props, 2));
			}
			unsigned cryptoSize = 0;
			int cryptoOffset = item.FindExtra(NExtraID::kCrypto, cryptoSize);
			if(cryptoOffset >= 0) {
				if(!filterStream) {
					filterStreamSpec = new CFilterCoder(false);
					filterStream = filterStreamSpec;
				}
				if(!cryptoDecoder) {
					cryptoDecoderSpec = new NCrypto::NRar5::CDecoder;
					cryptoDecoder = cryptoDecoderSpec;
				}
				RINOK(cryptoDecoderSpec->SetDecoderProps(item.Extra + (uint)cryptoOffset, cryptoSize, true, item.IsService()));
				if(!getTextPassword) {
					wrongPassword = True;
					return E_NOTIMPL;
				}
				RINOK(MySetPassword(getTextPassword, cryptoDecoderSpec));
				if(!cryptoDecoderSpec->CalcKey_and_CheckPassword())
					wrongPassword = True;
			}
			return S_OK;
		}

		HRESULT CUnpacker::Code(const CItem &item, const CItem &lastItem, uint64 packSize, ISequentialInStream * volsInStream, 
			ISequentialOutStream * realOutStream, ICompressProgressInfo * progress, bool &isCrcOK)
		{
			isCrcOK = true;
			unsigned method = item.GetMethod();
			if(method > kLzMethodMax)
				return E_NOTIMPL;
			if(linkFile && !lastItem.Is_UnknownSize()) {
				size_t dataSize = (size_t)lastItem.Size;
				if(dataSize != lastItem.Size)
					return E_NOTIMPL;
				linkFile->Data.Alloc(dataSize);
			}
			bool isCryptoMode = false;
			ISequentialInStream * inStream;
			if(item.IsEncrypted()) {
				filterStreamSpec->Filter = cryptoDecoder;
				filterStreamSpec->SetInStream(volsInStream);
				filterStreamSpec->SetOutStreamSize(NULL);
				inStream = filterStream;
				isCryptoMode = true;
			}
			else
				inStream = volsInStream;
			ICompressCoder * commonCoder = (method == 0) ? copyCoder : LzCoders[item.IsService() ? 1 : 0];
			outStreamSpec->SetStream(realOutStream);
			outStreamSpec->Init(lastItem, (linkFile ? (Byte *)linkFile->Data : NULL));
			NeedClearSolid[item.IsService() ? 1 : 0] = false;
			HRESULT res = S_OK;
			if(packSize != 0 || lastItem.Is_UnknownSize() || lastItem.Size != 0) {
				res = commonCoder->Code(inStream, outStream, &packSize, lastItem.Is_UnknownSize() ? NULL : &lastItem.Size, progress);
			}
			else {
				res = res;
			}
			if(isCryptoMode)
				filterStreamSpec->ReleaseInStream();
			uint64 processedSize = outStreamSpec->GetPos();
			if(res == S_OK && !lastItem.Is_UnknownSize() && processedSize != lastItem.Size)
				res = S_FALSE;
			// if(res == S_OK)
			{
				unsigned cryptoSize = 0;
				int cryptoOffset = lastItem.FindExtra(NExtraID::kCrypto, cryptoSize);
				NCrypto::NRar5::CDecoder * crypto = NULL;
				if(cryptoOffset >= 0) {
					CCryptoInfo cryptoInfo;
					if(cryptoInfo.Parse(lastItem.Extra + (uint)cryptoOffset, cryptoSize))
						if(cryptoInfo.UseMAC())
							crypto = cryptoDecoderSpec;
				}
				isCrcOK = outStreamSpec->_hash.Check(lastItem, crypto);
			}
			if(linkFile) {
				linkFile->Res = res;
				linkFile->crcOK = isCrcOK;
				if(!lastItem.Is_UnknownSize() && processedSize != lastItem.Size)
					linkFile->Data.ChangeSize_KeepData((size_t)processedSize, (size_t)processedSize);
			}
			return res;
		}

		HRESULT CUnpacker::DecodeToBuf(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, uint64 packSize, ISequentialInStream * inStream, CByteBuffer &buffer)
		{
			CBufPtrSeqOutStream * outSpec = new CBufPtrSeqOutStream;
			CMyComPtr<ISequentialOutStream> out = outSpec;
			_tempBuf.AllocAtLeast((size_t)item.Size);
			outSpec->Init(_tempBuf, (size_t)item.Size);
			bool wrongPassword;
			if(item.IsSolid())
				return E_NOTIMPL;
			HRESULT res = Create(EXTERNAL_CODECS_LOC_VARS item, item.IsSolid(), wrongPassword);
			if(res == S_OK) {
				if(wrongPassword)
					return S_FALSE;
				CLimitedSequentialInStream * limitedStreamSpec = new CLimitedSequentialInStream;
				CMyComPtr<ISequentialInStream> limitedStream(limitedStreamSpec);
				limitedStreamSpec->SetStream(inStream);
				limitedStreamSpec->Init(packSize);
				bool crcOK = true;
				res = Code(item, item, packSize, limitedStream, out, NULL, crcOK);
				if(res == S_OK) {
					if(!crcOK || outSpec->GetPos() != item.Size)
						res = S_FALSE;
					else
						buffer.CopyFrom(_tempBuf, (size_t)item.Size);
				}
			}
			return res;
		}

		struct CTempBuf {
			CByteBuffer _buf;
			size_t _offset;
			bool _isOK;
			void Clear()
			{
				_offset = 0;
				_isOK = true;
			}
			CTempBuf() 
			{
				Clear();
			}
			HRESULT Decode(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, ISequentialInStream * inStream, CUnpacker &unpacker, CByteBuffer &destBuf);
		};

		HRESULT CTempBuf::Decode(DECL_EXTERNAL_CODECS_LOC_VARS const CItem &item, ISequentialInStream * inStream,
			CUnpacker &unpacker, CByteBuffer &destBuf)
		{
			const size_t kPackSize_Max = (1 << 24);
			if(item.Size > (1 << 24) || item.Size == 0 || item.PackSize >= kPackSize_Max) {
				Clear();
				return S_OK;
			}
			if(item.IsSplit() /* && _isOK */) {
				size_t packSize = (size_t)item.PackSize;
				if(packSize > kPackSize_Max - _offset)
					return S_OK;
				size_t newSize = _offset + packSize;
				if(newSize > _buf.Size())
					_buf.ChangeSize_KeepData(newSize, _offset);
				Byte * data = (Byte *)_buf + _offset;
				RINOK(ReadStream_FALSE(inStream, data, packSize));
				_offset += packSize;
				if(item.IsSplitAfter()) {
					CHash hash;
					hash.Init(item);
					hash.Update(data, packSize);
					_isOK = hash.Check(item, NULL); // RAR5 doesn't use HMAC for packed part
				}
			}
			if(_isOK) {
				if(!item.IsSplitAfter()) {
					if(_offset == 0) {
						RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_LOC_VARS item, item.PackSize, inStream, destBuf));
					}
					else {
						CBufInStream * bufInStreamSpec = new CBufInStream;
						CMyComPtr<ISequentialInStream> bufInStream = bufInStreamSpec;
						bufInStreamSpec->Init(_buf, _offset);
						RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_LOC_VARS item, _offset, bufInStream, destBuf));
					}
				}
			}
			return S_OK;
		}

		static const Byte kProps[] = {
			kpidPath, kpidIsDir, kpidSize, kpidPackSize, kpidMTime, kpidCTime, kpidATime, kpidAttrib,

			kpidIsAltStream, kpidEncrypted, kpidSolid, kpidSplitBefore, kpidSplitAfter, kpidCRC, kpidHostOS,
			kpidMethod, kpidCharacts, kpidSymLink, kpidHardLink, kpidCopyLink,
		};

		static const Byte kArcProps[] = {
			kpidTotalPhySize, kpidCharacts, kpidSolid, kpidNumBlocks, kpidEncrypted, kpidIsVolume, kpidVolumeIndex, kpidNumVolumes, kpidComment
		};

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps

		uint64 CHandler::GetPackSize(unsigned refIndex) const
		{
			uint64 size = 0;
			uint index = _refs[refIndex].Item;
			for(;; ) {
				const CItem &item = _items[index];
				size += item.PackSize;
				if(item.NextItem < 0)
					return size;
				index = item.NextItem;
			}
		}

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CArc::CInArcInfo * arcInfo = NULL;
			if(!_arcs.IsEmpty())
				arcInfo = &_arcs[0].Info;
			switch(propID) {
				case kpidVolumeIndex: if(arcInfo && arcInfo->IsVolume()) prop = arcInfo->GetVolIndex(); break;
				case kpidSolid: if(arcInfo) prop = arcInfo->IsSolid(); break;
				case kpidCharacts:
				{
					if(!_arcs.IsEmpty()) {
						FLAGS_TO_PROP(k_ArcFlags, (uint32)arcInfo->Flags, prop);
					}
					break;
				}
				case kpidEncrypted: if(arcInfo) prop = arcInfo->IsEncrypted; break; // it's for encrypted names.
				case kpidIsVolume: if(arcInfo) prop = arcInfo->IsVolume(); break;
				case kpidNumVolumes: prop = (uint32)_arcs.Size(); break;
				case kpidIsAltStream: prop = true; break;
				case kpidOffset: if(arcInfo && arcInfo->StartPos != 0) prop = arcInfo->StartPos; break;
				case kpidTotalPhySize:
					if(_arcs.Size() > 1) {
						uint64 sum = 0;
						FOR_VECTOR(v, _arcs)
						sum += _arcs[v].Info.GetPhySize();
						prop = sum;
					}
					break;
				case kpidPhySize:
					if(arcInfo)
						prop = arcInfo->GetPhySize();
					break;
				case kpidComment:
					// if(!_arcs.IsEmpty())
					{
						// const CArc &arc = _arcs[0];
						const CByteBuffer &cmt = _comment;
						if(cmt.Size() != 0 && cmt.Size() < (1 << 16)) {
							AString s;
							s.SetFrom_CalcLen((const char *)(const Byte *)cmt, (uint)cmt.Size());
							UString unicode;
							if(ConvertUTF8ToUnicode(s, unicode))
								prop = unicode;
						}
					}
					break;
				case kpidNumBlocks:
				{
					uint32 numBlocks = 0;
					FOR_VECTOR(i, _refs)
					if(!_items[_refs[i].Item].IsSolid())
						numBlocks++;
					prop = (uint32)numBlocks;
					break;
				}
				case kpidError:
					if(/* &_missingVol || */ !_missingVolName.IsEmpty()) {
						UString s("Missing volume : ");
						s += _missingVolName;
						prop = s;
					}
					break;
				case kpidErrorFlags:
				{
					uint32 v = _errorFlags;
					if(!_isArc)
						v |= kpv_ErrorFlags_IsNotArc;
					prop = v;
					break;
				}
				/*
				   case kpidWarningFlags:
				   {
					   if(_warningFlags != 0)
						prop = _warningFlags;
					   break;
				   }
				 */
				case kpidExtension:
					if(_arcs.Size() == 1) {
						if(arcInfo->IsVolume()) {
							AString s("part");
							uint32 v = (uint32)arcInfo->GetVolIndex() + 1;
							if(v < 10)
								s += '0';
							s.Add_UInt32(v);
							s += ".rar";
							prop = s;
						}
					}
					break;
			}
			prop.Detach(value);
			return S_OK;

			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _refs.Size();
			return S_OK;
		}

		static const Byte kRawProps[] = { kpidChecksum, kpidNtSecure };

		STDMETHODIMP CHandler::GetNumRawProps(uint32 * numProps)
		{
			*numProps = ARRAY_SIZE(kRawProps);
			return S_OK;
		}

		STDMETHODIMP CHandler::GetRawPropInfo(uint32 index, BSTR * name, PROPID * propID)
		{
			*propID = kRawProps[index];
			*name = 0;
			return S_OK;
		}

		STDMETHODIMP CHandler::GetParent(uint32 index, uint32 * parent, uint32 * parentType)
		{
			*parentType = NParentType::kDir;
			*parent = (uint32)(int32)-1;
			if(index >= _refs.Size())
				return S_OK;
			const CRefItem &ref = _refs[index];
			const CItem &item = _items[ref.Item];
			if(item.Is_STM() && ref.Parent >= 0) {
				*parent = (uint32)ref.Parent;
				*parentType = NParentType::kAltStream;
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::GetRawProp(uint32 index, PROPID propID, const void ** data, uint32 * dataSize, uint32 * propType)
		{
			*data = NULL;
			*dataSize = 0;
			*propType = 0;
			if(index >= _refs.Size())
				return E_INVALIDARG;
			const CItem &item = _items[_refs[index].Item];
			if(propID == kpidNtSecure) {
				if(item.ACL >= 0) {
					const CByteBuffer &buf = _acls[item.ACL];
					*dataSize = (uint32)buf.Size();
					*propType = NPropDataType::kRaw;
					*data = (const Byte *)buf;
				}
				return S_OK;
			}
			if(propID == kpidChecksum) {
				int hashRecOffset = item.FindExtra_Blake();
				if(hashRecOffset >= 0) {
					*dataSize = BLAKE2S_DIGEST_SIZE;
					*propType = NPropDataType::kRaw;
					*data = &item.Extra[hashRecOffset];
				}
				return S_OK;
			}
			return S_OK;
		}
		static void TimeRecordToProp(const CItem &item, unsigned stampIndex, NCOM::CPropVariant &prop)
		{
			uint   size;
			int    offset = item.FindExtra(NExtraID::kTime, size);
			if(offset >= 0) {
				const Byte * p = item.Extra + (uint)offset;
				uint64 flags;
				{
					uint   num = ReadVarInt(p, size, &flags);
					if(num == 0)
						return;
					p += num;
					size -= num;
				}
				if(flags & (NTimeRecord::NFlags::kMTime << stampIndex)) {
					uint   numStamps = 0;
					uint   curStamp = 0;
					uint   i;
					for(i = 0; i < 3; i++)
						if((flags & (NTimeRecord::NFlags::kMTime << i)) != 0) {
							if(i == stampIndex)
								curStamp = numStamps;
							numStamps++;
						}
					FILETIME ft;
					if((flags & NTimeRecord::NFlags::kUnixTime) != 0) {
						curStamp *= 4;
						if(curStamp + 4 > size)
							return;
						const Byte * p2 = p + curStamp;
						uint64 val = NTime::UnixTimeToFileTime64(Get32(p2));
						numStamps *= 4;
						if((flags & NTimeRecord::NFlags::kUnixNs) != 0 && numStamps * 2 <= size) {
							const uint32 ns = Get32(p2 + numStamps) & 0x3FFFFFFF;
							if(ns < 1000000000)
								val += ns / 100;
						}
						ft.dwLowDateTime = (DWORD)val;
						ft.dwHighDateTime = (DWORD)(val >> 32);
					}
					else {
						curStamp *= 8;
						if(curStamp + 8 > size)
							return;
						const Byte * p2 = p + curStamp;
						ft.dwLowDateTime = Get32(p2);
						ft.dwHighDateTime = Get32(p2 + 4);
					}
					prop = ft;
				}
			}
		}
		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CRefItem &ref = _refs[index];
			const CItem &item = _items[ref.Item];
			const CItem &lastItem = _items[ref.Last];
			switch(propID) {
				case kpidPath:
				{
					UString unicodeName;
					if(item.Is_STM()) {
						AString s;
						if(ref.Parent >= 0) {
							CItem &mainItem = _items[_refs[ref.Parent].Item];
							s = mainItem.Name;
						}
						AString name;
						item.GetAltStreamName(name);
						if(name[0] != ':')
							s += ':';
						s += name;
						if(!ConvertUTF8ToUnicode(s, unicodeName))
							break;
					}
					else {
						if(!ConvertUTF8ToUnicode(item.Name, unicodeName))
							break;
						if(item.Version_Defined) {
							char temp[32];
							// temp[0] = ';';
							// ConvertUInt64ToString(item.Version, temp + 1);
							// unicodeName += temp;
							ConvertUInt64ToString(item.Version, temp);
							UString s2("[VER]" STRING_PATH_SEPARATOR);
							s2 += temp;
							s2.Add_PathSepar();
							unicodeName.Insert(0, s2);
						}
					}
					NItemName::ReplaceToOsSlashes_Remove_TailSlash(unicodeName);
					prop = unicodeName;
					break;
				}
				case kpidIsDir: prop = item.IsDir(); break;
				case kpidSize: if(!lastItem.Is_UnknownSize()) prop = lastItem.Size; break;
				case kpidPackSize: prop = GetPackSize(index); break;
				case kpidMTime:
				{
					TimeRecordToProp(item, NTimeRecord::k_Index_MTime, prop);
					if(prop.vt == VT_EMPTY && item.Has_UnixMTime()) {
						FILETIME ft;
						NWindows::NTime::UnixTimeToFileTime(item.UnixMTime, ft);
						prop = ft;
					}
					if(prop.vt == VT_EMPTY && ref.Parent >= 0) {
						const CItem &baseItem = _items[_refs[ref.Parent].Item];
						TimeRecordToProp(baseItem, NTimeRecord::k_Index_MTime, prop);
						if(prop.vt == VT_EMPTY && baseItem.Has_UnixMTime()) {
							FILETIME ft;
							NWindows::NTime::UnixTimeToFileTime(baseItem.UnixMTime, ft);
							prop = ft;
						}
					}
					break;
				}
				case kpidCTime: TimeRecordToProp(item, NTimeRecord::k_Index_CTime, prop); break;
				case kpidATime: TimeRecordToProp(item, NTimeRecord::k_Index_ATime, prop); break;
				case kpidName:
				{
					if(item.Is_STM()) {
						AString name;
						item.GetAltStreamName(name);
						if(name[0] == ':') {
							name.DeleteFrontal(1);
							UString unicodeName;
							if(ConvertUTF8ToUnicode(name, unicodeName))
								prop = unicodeName;
						}
					}
					break;
				}
				case kpidIsAltStream: prop = item.Is_STM(); break;
				case kpidSymLink: item.Link_to_Prop(NLinkType::kUnixSymLink, prop); break;
				case kpidHardLink: item.Link_to_Prop(NLinkType::kHardLink, prop); break;
				case kpidCopyLink: item.Link_to_Prop(NLinkType::kFileCopy, prop); break;
				case kpidAttrib: prop = item.GetWinAttrib(); break;
				case kpidEncrypted: prop = item.IsEncrypted(); break;
				case kpidSolid: prop = item.IsSolid(); break;
				case kpidSplitBefore: prop = item.IsSplitBefore(); break;
				case kpidSplitAfter: prop = lastItem.IsSplitAfter(); break;
				case kpidCRC:
				{
					const CItem * item2 = (lastItem.IsSplitAfter() ? &item : &lastItem);
					if(item2->Has_CRC())
						prop = item2->CRC;
					break;
				}
				case kpidMethod:
				{
					char   temp[128];
					uint   algo = item.GetAlgoVersion();
					char * s = temp;
					if(algo != 0) {
						ConvertUInt32ToString(algo, s);
						s += sstrlen(s);
						*s++ = ':';
					}
					uint   m = item.GetMethod();
					{
						s[0] = 'm';
						s[1] = (char)(m + '0');
						s[2] = 0;
						if(!item.IsDir()) {
							s[2] = ':';
							ConvertUInt32ToString(item.GetDictSize() + 17, s + 3);
						}
					}
					uint   cryptoSize = 0;
					int    cryptoOffset = item.FindExtra(NExtraID::kCrypto, cryptoSize);
					if(cryptoOffset >= 0) {
						s = temp + sstrlen(temp);
						*s++ = ' ';
						CCryptoInfo cryptoInfo;
						bool isOK = cryptoInfo.Parse(item.Extra + (uint)cryptoOffset, cryptoSize);
						if(cryptoInfo.Algo == 0)
							s = MyStpCpy(s, "AES");
						else {
							s = MyStpCpy(s, "Crypto_");
							ConvertUInt64ToString(cryptoInfo.Algo, s);
							s += sstrlen(s);
						}
						if(isOK) {
							*s++ = ':';
							ConvertUInt32ToString(cryptoInfo.Cnt, s);
							s += sstrlen(s);
							*s++ = ':';
							ConvertUInt64ToString(cryptoInfo.Flags, s);
						}
					}
					prop = temp;
					break;
				}
				case kpidCharacts:
				{
					AString s;
					if(item.ACL >= 0) {
						s.Add_OptSpaced("ACL");
					}
					uint32 flags = item.Flags;
					// flags &= ~(6); // we don't need compression related bits here.
					if(flags != 0) {
						AString s2 = FlagsToString(k_FileFlags, ARRAY_SIZE(k_FileFlags), flags);
						if(!s2.IsEmpty()) {
							s.Add_OptSpaced(s2);
						}
					}
					item.PrintInfo(s);
					if(!s.IsEmpty())
						prop = s;
					break;
				}
				case kpidHostOS:
					if(item.HostOS < ARRAY_SIZE(kHostOS))
						prop = kHostOS[(size_t)item.HostOS];
					else
						prop = (uint64)item.HostOS;
					break;
			}

			prop.Detach(value);
			return S_OK;

			COM_TRY_END
		}

		// ---------- Copy Links ----------

		static int CompareItemsPaths(const CHandler &handler, unsigned p1, unsigned p2, const AString * name1)
		{
			const CItem &item1 = handler._items[handler._refs[p1].Item];
			const CItem &item2 = handler._items[handler._refs[p2].Item];
			if(item1.Version_Defined) {
				if(!item2.Version_Defined)
					return -1;
				int res = MyCompare(item1.Version, item2.Version);
				if(res != 0)
					return res;
			}
			else if(item2.Version_Defined)
				return 1;
			SETIFZ(name1, &item1.Name);
			return strcmp(*name1, item2.Name);
		}
		static int CompareItemsPaths2(const CHandler &handler, unsigned p1, unsigned p2, const AString * name1)
		{
			int res = CompareItemsPaths(handler, p1, p2, name1);
			return (res != 0) ? res : MyCompare(p1, p2);
		}
		static int CompareItemsPaths_Sort(const uint * p1, const uint * p2, void * param)
		{
			return CompareItemsPaths2(*(const CHandler*)param, *p1, *p2, NULL);
		}
		static int FindLink(const CHandler &handler, const CUIntVector &sorted, const AString &s, unsigned index)
		{
			unsigned left = 0, right = sorted.Size();
			for(;; ) {
				if(left == right) {
					if(left > 0) {
						unsigned refIndex = sorted[left - 1];
						if(CompareItemsPaths(handler, index, refIndex, &s) == 0)
							return refIndex;
					}
					if(right < sorted.Size()) {
						unsigned refIndex = sorted[right];
						if(CompareItemsPaths(handler, index, refIndex, &s) == 0)
							return refIndex;
					}
					return -1;
				}

				unsigned mid = (left + right) / 2;
				unsigned refIndex = sorted[mid];
				int compare = CompareItemsPaths2(handler, index, refIndex, &s);
				if(compare == 0)
					return refIndex;
				if(compare < 0)
					right = mid;
				else
					left = mid + 1;
			}
		}

		void CHandler::FillLinks()
		{
			uint i;
			for(i = 0; i < _refs.Size(); i++) {
				const CItem &item = _items[_refs[i].Item];
				if(!item.IsDir() && !item.IsService() && item.NeedUse_as_CopyLink())
					break;
			}
			if(i == _refs.Size())
				return;
			CUIntVector sorted;
			for(i = 0; i < _refs.Size(); i++) {
				const CItem &item = _items[_refs[i].Item];
				if(!item.IsDir() && !item.IsService())
					sorted.Add(i);
			}
			if(!sorted.IsEmpty()) {
				sorted.Sort(CompareItemsPaths_Sort, this);
				AString link;
				for(i = 0; i < _refs.Size(); i++) {
					CRefItem &ref = _refs[i];
					const CItem &item = _items[ref.Item];
					if(item.IsDir() || item.IsService() || item.PackSize != 0)
						continue;
					CLinkInfo linkInfo;
					if(!item.FindExtra_Link(linkInfo) || linkInfo.Type != NLinkType::kFileCopy)
						continue;
					link.SetFrom_CalcLen((const char *)(item.Extra + linkInfo.NameOffset), linkInfo.NameLen);
					int linkIndex = FindLink(*this, sorted, link, i);
					if(linkIndex < 0)
						continue;
					if((uint)linkIndex >= i)
						continue;  // we don't support forward links that can lead to loops
					const CRefItem &linkRef = _refs[linkIndex];
					const CItem &linkItem = _items[linkRef.Item];
					if(linkItem.Size == item.Size) {
						if(linkRef.Link >= 0)
							ref.Link = linkRef.Link;
						else if(!linkItem.NeedUse_as_CopyLink())
							ref.Link = linkIndex;
					}
				}
			}
		}

		HRESULT CHandler::Open2(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback)
		{
			CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
			CMyComPtr<ICryptoGetTextPassword> getTextPassword;
			NRar::CVolumeName seqName;
			uint64 totalBytes = 0;
			uint64 curBytes = 0;
			if(openCallback) {
				openCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
				openCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
			}
			CTempBuf tempBuf;
			CUnpacker unpacker;
			unpacker.getTextPassword = getTextPassword;
			int prevSplitFile = -1;
			int prevMainFile = -1;
			bool nextVol_is_Required = false;
			CInArchive arch;
			for(;; ) {
				CMyComPtr <IInStream> inStream;
				if(_arcs.IsEmpty())
					inStream = stream;
				else {
					if(!openVolumeCallback)
						break;
					if(_arcs.Size() == 1) {
						UString baseName;
						{
							NCOM::CPropVariant prop;
							RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
							if(prop.vt != VT_BSTR)
								break;
							baseName = prop.bstrVal;
						}
						if(!seqName.InitName(baseName))
							break;
					}
					const UString volName = seqName.GetNextName();
					HRESULT result = openVolumeCallback->GetStream(volName, &inStream);
					if(result != S_OK && result != S_FALSE)
						return result;
					if(!inStream || result != S_OK) {
						if(nextVol_is_Required)
							_missingVolName = volName;
						break;
					}
				}
				uint64 endPos = 0;
				RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &arch.StreamStartPosition));
				RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
				RINOK(inStream->Seek(arch.StreamStartPosition, STREAM_SEEK_SET, NULL));
				if(openCallback) {
					totalBytes += endPos;
					RINOK(openCallback->SetTotal(NULL, &totalBytes));
				}
				CArc::CInArcInfo arcInfoOpen;
				{
					HRESULT res = arch.Open(inStream, maxCheckStartPosition, getTextPassword, arcInfoOpen);
					if(arch.IsArc && arch.UnexpectedEnd)
						_errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
					if(_arcs.IsEmpty()) {
						_isArc = arch.IsArc;
					}
					if(res != S_OK) {
						if(res != S_FALSE)
							return res;
						if(_arcs.IsEmpty())
							return res;
						break;
					}
				}
				CArc &arc = _arcs.AddNew();
				CArc::CInArcInfo & arcInfo = arc.Info;
				arcInfo = arcInfoOpen;
				arc.Stream = inStream;
				CItem item;
				for(;; ) {
					item.Clear();
					arcInfo.EndPos = arch.Position;
					if(arch.Position > endPos) {
						_errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
						break;
					}
					RINOK(inStream->Seek(arch.Position, STREAM_SEEK_SET, NULL));
					{
						CInArchive::CHeader h;
						HRESULT res = arch.ReadBlockHeader(h);
						if(res != S_OK) {
							if(res != S_FALSE)
								return res;
							if(arch.UnexpectedEnd) {
								_errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
								SETMAX(arcInfo.EndPos, arch.Position);
								SETMAX(arcInfo.EndPos, endPos);
							}
							else
								_errorFlags |= kpv_ErrorFlags_HeadersError;
							break;
						}
						if(h.Type == NHeaderType::kEndOfArc) {
							arcInfo.EndPos = arch.Position;
							arcInfo.EndOfArchive_was_Read = true;
							if(!arch.ReadVar(arcInfo.EndFlags))
								_errorFlags |= kpv_ErrorFlags_HeadersError;
							if(arcInfo.IsVolume()) {
								// for multivolume archives RAR can add ZERO bytes at the end for
								// alignment.
								// We must skip these bytes to prevent phySize warning.
								RINOK(inStream->Seek(arcInfo.EndPos, STREAM_SEEK_SET, NULL));
								bool areThereNonZeros;
								uint64 numZeros;
								const uint64 maxSize = 1 << 12;
								RINOK(ReadZeroTail(inStream, areThereNonZeros, numZeros, maxSize));
								if(!areThereNonZeros && numZeros != 0 && numZeros <= maxSize)
									arcInfo.EndPos += numZeros;
							}
							break;
						}
						if(h.Type != NHeaderType::kFile && h.Type != NHeaderType::kService) {
							_errorFlags |= kpv_ErrorFlags_UnsupportedFeature;
							break;
						}
						item.RecordType = (Byte)h.Type;
						if(!arch.ReadFileHeader(h, item)) {
							_errorFlags |= kpv_ErrorFlags_HeadersError;
							break;
						}
						// item.MainPartSize = (uint32)(Position - item.Position);
						item.DataPos = arch.Position;
					}
					bool isOk_packSize = true;
					{
						arcInfo.EndPos = arch.Position;
						if(arch.Position + item.PackSize < arch.Position) {
							isOk_packSize = false;
							_errorFlags |= kpv_ErrorFlags_HeadersError;
							if(arcInfo.EndPos < endPos)
								arcInfo.EndPos = endPos;
						}
						else {
							arch.AddToSeekValue(item.PackSize); // Position points to next header;
							arcInfo.EndPos = arch.Position;
						}
					}
					bool needAdd = true;
					{
						if(_comment.Size() == 0 && item.Is_CMT() && item.PackSize < kCommentSize_Max && item.PackSize == item.Size && 
							item.PackSize != 0 && item.GetMethod() == 0 && !item.IsSplit()) {
							RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_VARS item, item.PackSize, inStream, _comment));
							needAdd = false;
						}
					}
					if(needAdd) {
						CRefItem ref;
						ref.Item = _items.Size();
						ref.Last = ref.Item;
						ref.Parent = -1;
						ref.Link = -1;
						if(item.IsService()) {
							if(item.Is_STM()) {
								if(prevMainFile >= 0)
									ref.Parent = prevMainFile;
							}
							else {
								needAdd = false;
								if(item.Is_ACL() && (!item.IsEncrypted() || arch.m_CryptoMode)) {
									if(prevMainFile >= 0 && item.Size < (1 << 24) && item.Size != 0) {
										CItem & mainItem = _items[_refs[prevMainFile].Item];
										if(mainItem.ACL < 0) {
											CByteBuffer acl;
											HRESULT res = tempBuf.Decode(EXTERNAL_CODECS_VARS item, inStream, unpacker, acl);
											if(!item.IsSplitAfter())
												tempBuf.Clear();
											if(res != S_OK) {
												tempBuf.Clear();
												if(res != S_FALSE && res != E_NOTIMPL)
													return res;
											}
											// RINOK();
											if(res == S_OK && acl.Size() != 0) {
												if(_acls.IsEmpty() || acl != _acls.Back())
													_acls.Add(acl);
												mainItem.ACL = _acls.Size() - 1;
											}
										}
									}
								}
							}
						}
						if(needAdd) {
							if(item.IsSplitBefore()) {
								if(prevSplitFile >= 0) {
									CRefItem &ref2 = _refs[prevSplitFile];
									CItem &prevItem = _items[ref2.Last];
									if(item.IsNextForItem(prevItem)) {
										ref2.Last = _items.Size();
										prevItem.NextItem = ref2.Last;
										needAdd = false;
									}
								}
							}
						}
						if(needAdd) {
							if(item.IsSplitAfter())
								prevSplitFile = _refs.Size();
							if(!item.IsService())
								prevMainFile = _refs.Size();
							_refs.Add(ref);
						}
					}
					{
						uint64 version;
						if(item.FindExtra_Version(version)) {
							item.Version_Defined = true;
							item.Version = version;
						}
					}
					item.VolIndex = _arcs.Size() - 1;
					_items.Add(item);
					if(openCallback && (_items.Size() & 0xFF) == 0) {
						uint64 numFiles = _items.Size();
						uint64 numBytes = curBytes + item.DataPos;
						RINOK(openCallback->SetCompleted(&numFiles, &numBytes));
					}
					if(!isOk_packSize)
						break;
				}
				curBytes += endPos;
				nextVol_is_Required = false;
				if(!arcInfo.IsVolume())
					break;
				if(arcInfo.EndOfArchive_was_Read) {
					if(!arcInfo.AreMoreVolumes())
						break;
					nextVol_is_Required = true;
				}
			}
			FillLinks();
			return S_OK;
		}

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * openCallback)
		{
			COM_TRY_BEGIN
			Close();
			return Open2(stream, maxCheckStartPosition, openCallback);
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			COM_TRY_BEGIN
			_missingVolName.Empty();
			_errorFlags = 0;
			// _warningFlags = 0;
			_isArc = false;
			_refs.Clear();
			_items.Clear();
			_arcs.Clear();
			_acls.Clear();
			_comment.Free();
			return S_OK;
			COM_TRY_END
		}

		class CVolsInStream : public ISequentialInStream, public CMyUnknownImp {
		private:
			uint64 _rem;
			ISequentialInStream * _stream;
			const CObjectVector<CArc> * _arcs;
			const CObjectVector<CItem> * _items;
			int _itemIndex;
		public:
			bool CrcIsOK;
		private:
			CHash _hash;
		public:
			MY_UNKNOWN_IMP
			void Init(const CObjectVector<CArc> * arcs, const CObjectVector<CItem> * items, unsigned itemIndex)
			{
				_arcs = arcs;
				_items = items;
				_itemIndex = itemIndex;
				_stream = NULL;
				CrcIsOK = true;
			}
			STDMETHOD(Read) (void * data, uint32 size, uint32 *processedSize);
		};

		STDMETHODIMP CVolsInStream::Read(void * data, uint32 size, uint32 * processedSize)
		{
			ASSIGN_PTR(processedSize, 0);
			uint32 realProcessedSize = 0;
			while(size != 0) {
				if(!_stream) {
					if(_itemIndex < 0)
						break;
					const CItem &item = (*_items)[_itemIndex];
					IInStream * s = (*_arcs)[item.VolIndex].Stream;
					RINOK(s->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
					_stream = s;
					if(CrcIsOK && item.IsSplitAfter())
						_hash.Init(item);
					else
						_hash.Init_NoCalc();
					_rem = item.PackSize;
				}
				{
					uint32 cur = size;
					SETMIN(cur, (uint32)_rem);
					uint32 num = cur;
					HRESULT res = _stream->Read(data, cur, &cur);
					_hash.Update(data, cur);
					realProcessedSize += cur;
					ASSIGN_PTR(processedSize, realProcessedSize);
					data = (Byte *)data + cur;
					size -= cur;
					_rem -= cur;
					if(_rem == 0) {
						const CItem &item = (*_items)[_itemIndex];
						_itemIndex = item.NextItem;
						if(!_hash.Check(item, NULL)) // RAR doesn't use MAC here
							CrcIsOK = false;
						_stream = NULL;
					}
					if(res != S_OK)
						return res;
					if(realProcessedSize != 0)
						return S_OK;
					if(cur == 0 && num != 0)
						return S_OK;
				}
			}

			return S_OK;
		}

		static int FASTCALL FindLinkBuf(const CObjectVector <CLinkFile> & linkFiles, unsigned index)
		{
			for(uint left = 0, right = linkFiles.Size();;) {
				if(left == right)
					return -1;
				else {
					uint mid = (left + right) / 2;
					uint linkIndex = linkFiles[mid].Index;
					if(index == linkIndex)
						return mid;
					else if(index < linkIndex)
						right = mid;
					else
						left = mid + 1;
				}
			}
		}

		static inline int DecoderRes_to_OpRes(HRESULT res, bool crcOK)
		{
			if(res == E_NOTIMPL)
				return NExtractArc::NOperationResult::kUnsupportedMethod;
			// if(res == S_FALSE)
			if(res != S_OK)
				return NExtractArc::NOperationResult::kDataError;
			return crcOK ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kCRCError;
		}

		static HRESULT CopyData_with_Progress(const Byte * data, size_t size, ISequentialOutStream * outStream, ICompressProgressInfo * progress)
		{
			for(size_t pos = 0; pos < size;) {
				const uint32 kStepSize = ((uint32)1 << 24);
				uint32 cur32;
				{
					size_t cur = size - pos;
					SETMIN(cur, kStepSize);
					cur32 = (uint32)cur;
				}
				RINOK(outStream->Write(data + pos, cur32, &cur32));
				if(cur32 == 0)
					return E_FAIL;
				pos += cur32;
				if(progress) {
					uint64 pos64 = pos;
					RINOK(progress->SetRatioInfo(&pos64, &pos64));
				}
			}
			return S_OK;
		}

		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _refs.Size();
			if(numItems == 0)
				return S_OK;
			CByteArr extractStatuses(_refs.Size());
			memzero(extractStatuses, _refs.Size());
			// we don't want to use temp buffer for big link files.
			const size_t k_CopyLinkFile_MaxSize = (size_t)1 << (28 + sizeof(size_t) / 2);
			const Byte kStatus_Extract = 1 << 0;
			const Byte kStatus_Skip = 1 << 1;
			const Byte kStatus_Link = 1 << 2;
			CObjectVector<CLinkFile> linkFiles;
			{
				uint64 total = 0;
				bool isThereUndefinedSize = false;
				bool thereAreLinks = false;
				{
					unsigned solidLimit = 0;
					for(uint32 t = 0; t < numItems; t++) {
						unsigned index = allFilesMode ? t : indices[t];
						const CRefItem &ref = _refs[index];
						const CItem &item = _items[ref.Item];
						const CItem &lastItem = _items[ref.Last];
						extractStatuses[index] |= kStatus_Extract;
						if(!lastItem.Is_UnknownSize())
							total += lastItem.Size;
						else
							isThereUndefinedSize = true;
						if(ref.Link >= 0) {
							if(!testMode) {
								if((uint)ref.Link < index) {
									const CRefItem &linkRef = _refs[(uint)ref.Link];
									const CItem &linkItem = _items[linkRef.Item];
									if(linkItem.IsSolid() && linkItem.Size <= k_CopyLinkFile_MaxSize) {
										if(extractStatuses[(uint)ref.Link] == 0) {
											const CItem &lastLinkItem = _items[linkRef.Last];
											if(!lastLinkItem.Is_UnknownSize())
												total += lastLinkItem.Size;
											else
												isThereUndefinedSize = true;
										}
										extractStatuses[(uint)ref.Link] |= kStatus_Link;
										thereAreLinks = true;
									}
								}
							}
							continue;
						}
						if(item.IsService())
							continue;
						if(item.IsSolid()) {
							unsigned j = index;
							while(j > solidLimit) {
								j--;
								const CRefItem &ref2 = _refs[j];
								const CItem &item2 = _items[ref2.Item];
								if(!item2.IsService()) {
									if(extractStatuses[j] == 0) {
										const CItem &lastItem2 = _items[ref2.Last];
										if(!lastItem2.Is_UnknownSize())
											total += lastItem2.Size;
										else
											isThereUndefinedSize = true;
									}
									extractStatuses[j] |= kStatus_Skip;
									if(!item2.IsSolid())
										break;
								}
							}
						}
						solidLimit = index + 1;
					}
				}
				if(thereAreLinks) {
					unsigned solidLimit = 0;
					FOR_VECTOR(i, _refs) {
						if((extractStatuses[i] & kStatus_Link) == 0)
							continue;
						const CItem &item = _items[_refs[i].Item];
						/*
						   if(item.IsService())
						   continue;
						 */
						CLinkFile &linkFile = linkFiles.AddNew();
						linkFile.Index = i;
						if(item.IsSolid()) {
							unsigned j = i;
							while(j > solidLimit) {
								j--;
								const CRefItem &ref2 = _refs[j];
								const CItem &item2 = _items[ref2.Item];
								if(!item2.IsService()) {
									if(extractStatuses[j] != 0)
										break;
									extractStatuses[j] = kStatus_Skip;
									{
										const CItem &lastItem2 = _items[ref2.Last];
										if(!lastItem2.Is_UnknownSize())
											total += lastItem2.Size;
										else
											isThereUndefinedSize = true;
									}
									if(!item2.IsSolid())
										break;
								}
							}
						}
						solidLimit = i + 1;
					}
					for(uint32 t = 0; t < numItems; t++) {
						unsigned index = allFilesMode ? t : indices[t];
						const CRefItem &ref = _refs[index];
						int linkIndex = ref.Link;
						if(linkIndex < 0 || (uint)linkIndex >= index)
							continue;
						const CItem &linkItem = _items[_refs[(uint)linkIndex].Item];
						if(!linkItem.IsSolid() || linkItem.Size > k_CopyLinkFile_MaxSize)
							continue;
						int bufIndex = FindLinkBuf(linkFiles, linkIndex);
						if(bufIndex < 0)
							return E_FAIL;
						linkFiles[bufIndex].NumLinks++;
					}
				}

				if(total != 0 || !isThereUndefinedSize) {
					RINOK(extractCallback->SetTotal(total));
				}
			}

			uint64 totalUnpacked = 0;
			uint64 totalPacked = 0;
			uint64 curUnpackSize = 0;
			uint64 curPackSize = 0;
			CUnpacker unpacker;
			CVolsInStream * volsInStreamSpec = new CVolsInStream;
			CMyComPtr<ISequentialInStream> volsInStream = volsInStreamSpec;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			// bool needClearSolid = true;
			FOR_VECTOR(i, _refs) {
				if(extractStatuses[i]) {
					totalUnpacked += curUnpackSize;
					totalPacked += curPackSize;
					lps->InSize = totalPacked;
					lps->OutSize = totalUnpacked;
					RINOK(lps->SetCur());
					CMyComPtr<ISequentialOutStream> realOutStream;
					int32 askMode = ((extractStatuses[i] & kStatus_Extract) != 0) ? (testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract) : NExtractArc::NAskMode::kSkip;
					unpacker.linkFile = NULL;
					if(((extractStatuses[i] & kStatus_Link) != 0)) {
						int bufIndex = FindLinkBuf(linkFiles, i);
						if(bufIndex < 0)
							return E_FAIL;
						unpacker.linkFile = &linkFiles[bufIndex];
					}
					uint32 index = i;
					const CRefItem * ref = &_refs[index];
					const CItem * item = &_items[ref->Item];
					const CItem &lastItem = _items[ref->Last];
					curUnpackSize = 0;
					if(!lastItem.Is_UnknownSize())
						curUnpackSize = lastItem.Size;
					curPackSize = GetPackSize(index);
					RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
					bool isSolid;
					{
						bool &needClearSolid = unpacker.NeedClearSolid[item->IsService() ? 1 : 0];
						isSolid = (item->IsSolid() && !needClearSolid);
						if(item->IsService())
							isSolid = false;
						needClearSolid = !item->IsSolid();
					}
					if(item->IsDir()) {
						RINOK(extractCallback->PrepareOperation(askMode));
						RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					}
					else {
						int index2 = ref->Link;
						int bufIndex = -1;
						if(index2 >= 0) {
							const CRefItem & ref2 = _refs[index2];
							const CItem & item2 = _items[ref2.Item];
							const CItem & lastItem2 = _items[ref2.Last];
							if(!item2.IsSolid()) {
								item = &item2;
								ref = &ref2;
								curUnpackSize = (!lastItem2.Is_UnknownSize()) ? lastItem2.Size : 0;
								curPackSize = GetPackSize(index2);
							}
							else if((uint)index2 < index)
								bufIndex = FindLinkBuf(linkFiles, index2);
						}
						if(!realOutStream) {
							if(testMode) {
								if(item->NeedUse_as_CopyLink_or_HardLink()) {
									RINOK(extractCallback->PrepareOperation(askMode));
									RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
									continue;
								}
							}
							else if(item->IsService() || item->NeedUse_as_HardLink())
								continue;
							else {
								bool needDecode = false;
								for(uint n = i + 1; n < _refs.Size(); n++) {
									const CItem & nextItem = _items[_refs[n].Item];
									if(!nextItem.IsService()) {
										if(!nextItem.IsSolid())
											break;
										else if(extractStatuses[i] != 0) {
											needDecode = true;
											break;
										}
									}
								}
								if(!needDecode)
									continue;
								else
									askMode = NExtractArc::NAskMode::kSkip;
							}
						}
						RINOK(extractCallback->PrepareOperation(askMode));
						if(bufIndex >= 0) {
							CLinkFile &linkFile = linkFiles[bufIndex];
							if(linkFile.NumLinks == 0)
								return E_FAIL;
							if(realOutStream) {
								RINOK(CopyData_with_Progress(linkFile.Data, linkFile.Data.Size(), realOutStream, progress));
							}
							if(--linkFile.NumLinks == 0)
								linkFile.Data.Free();
							RINOK(extractCallback->SetOperationResult(DecoderRes_to_OpRes(linkFile.Res, linkFile.crcOK)));
						}
						else if(item->NeedUse_as_CopyLink()) {
							RINOK(extractCallback->SetOperationResult(realOutStream ? NExtractArc::NOperationResult::kUnsupportedMethod : NExtractArc::NOperationResult::kOK));
						}
						else {
							volsInStreamSpec->Init(&_arcs, &_items, ref->Item);
							uint64 packSize = curPackSize;
							if(item->IsEncrypted())
								if(!unpacker.getTextPassword)
									extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&unpacker.getTextPassword);
							bool wrongPassword;
							HRESULT result = unpacker.Create(EXTERNAL_CODECS_VARS *item, isSolid, wrongPassword);
							if(wrongPassword) {
								realOutStream.Release();
								RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kWrongPassword));
							}
							else {
								bool crcOK = true;
								if(result == S_OK)
									result = unpacker.Code(*item, _items[ref->Last], packSize, volsInStream, realOutStream, progress, crcOK);
								realOutStream.Release();
								if(!volsInStreamSpec->CrcIsOK)
									crcOK = false;
								int opRes = crcOK ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kCRCError;
								if(result != S_OK) {
									if(result == S_FALSE)
										opRes = NExtractArc::NOperationResult::kDataError;
									else if(result == E_NOTIMPL)
										opRes = NExtractArc::NOperationResult::kUnsupportedMethod;
									else
										return result;
								}
								RINOK(extractCallback->SetOperationResult(opRes));
							}
						}
					}
				}
			}
			{
				FOR_VECTOR(i, linkFiles) {
					if(linkFiles[i].NumLinks != 0)
						return E_FAIL;
				}
			}
			return S_OK;
			COM_TRY_END
		}
		IMPL_ISetCompressCodecsInfo
		REGISTER_ARC_I("Rar5", "rar r00", 0, 0xCC, kMarker, 0, NArcInfoFlags::kFindSignature, NULL)
	}
}

class CBlake2spHasher : public IHasher, public CMyUnknownImp {
public:
	CBlake2spHasher() 
	{
		Init();
	}
	MY_UNKNOWN_IMP
	INTERFACE_IHasher(; )
private:
	CBlake2sp _blake;
	Byte mtDummy[1 << 7];
};

STDMETHODIMP_(void) CBlake2spHasher::Init() throw() { Blake2sp_Init(&_blake); }
STDMETHODIMP_(void) CBlake2spHasher::Update(const void * data, uint32 size) throw() { Blake2sp_Update(&_blake, (const Byte *)data, size); }
STDMETHODIMP_(void) CBlake2spHasher::Final(Byte *digest) throw() { Blake2sp_Final(&_blake, digest); }

REGISTER_HASHER(CBlake2spHasher, 0x202, "BLAKE2sp", BLAKE2S_DIGEST_SIZE)
