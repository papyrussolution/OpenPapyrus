// LzxDecoder.cpp

#include <7z-internal.h>
#pragma hdrstop

#ifdef SHOW_DEBUG_INFO
	#define PRF(x) x
#else
	#define PRF(x)
#endif
//#include <LzxDecoder.h>

namespace NCompress {
	namespace NLzx {
		static void x86_Filter(Byte * data, uint32 size, uint32 processedSize, uint32 translationSize)
		{
			const uint32 kResidue = 10;
			if(size <= kResidue)
				return;
			size -= kResidue;
			Byte save = data[(size_t)size + 4];
			data[(size_t)size + 4] = 0xE8;
			for(uint32 i = 0;; ) {
				const Byte * p = data + i;
				for(;; ) {
					if(*p++ == 0xE8) break;
					if(*p++ == 0xE8) break;
					if(*p++ == 0xE8) break;
					if(*p++ == 0xE8) break;
				}
				i = (uint32)(p - data);
				if(i > size)
					break;
				{
					int32 v = GetUi32(p);
					int32 pos = (int32)((int32)1 - (int32)(processedSize + i));
					i += 4;
					if(v >= pos && v < (int32)translationSize) {
						v += (v >= 0 ? pos : translationSize);
						SetUi32(p, v);
					}
				}
			}
			data[(size_t)size + 4] = save;
		}

		void CBitDecoder::Init(const Byte * data, size_t size)
		{
			_buf = data;
			_bufLim = data + size - 1;
			_bitPos = 0;
			_extraSize = 0;
		}

		size_t CBitDecoder::GetRem() const { return _bufLim + 1 - _buf; }
		bool   CBitDecoder::WasExtraReadError_Fast() const { return _extraSize > 4; }
		uint32 FASTCALL CBitDecoder::GetValue(uint numBits) const { return (_value >> (_bitPos - numBits)) & (((uint32)1 << numBits) - 1); }
		bool   CBitDecoder::IsOneDirectByteLeft() const { return _buf == _bufLim && _extraSize == 0; }

		bool CBitDecoder::WasFinishedOK() const
		{
			if(_buf != _bufLim + 1)
				return false;
			if((_bitPos >> 4) * 2 != _extraSize)
				return false;
			unsigned numBits = _bitPos & 15;
			return (((_value >> (_bitPos - numBits)) & (((uint32)1 << numBits) - 1)) == 0);
		}

		void CBitDecoder::NormalizeSmall()
		{
			if(_bitPos <= 16) {
				uint32 val;
				if(_buf >= _bufLim) {
					val = 0xFFFF;
					_extraSize += 2;
				}
				else {
					val = GetUi16(_buf);
					_buf += 2;
				}
				_value = (_value << 16) | val;
				_bitPos += 16;
			}
		}

		void CBitDecoder::NormalizeBig()
		{
			if(_bitPos <= 16) {
				{
					uint32 val;
					if(_buf >= _bufLim) {
						val = 0xFFFF;
						_extraSize += 2;
					}
					else {
						val = GetUi16(_buf);
						_buf += 2;
					}
					_value = (_value << 16) | val;
					_bitPos += 16;
				}
				if(_bitPos <= 16) {
					uint32 val;
					if(_buf >= _bufLim) {
						val = 0xFFFF;
						_extraSize += 2;
					}
					else {
						val = GetUi16(_buf);
						_buf += 2;
					}
					_value = (_value << 16) | val;
					_bitPos += 16;
				}
			}
		}

		void FASTCALL CBitDecoder::MovePos(uint numBits)
		{
			_bitPos -= numBits;
			NormalizeSmall();
		}

		uint32 FASTCALL CBitDecoder::ReadBitsSmall(uint numBits)
		{
			_bitPos -= numBits;
			uint32 val = (_value >> _bitPos) & (((uint32)1 << numBits) - 1);
			NormalizeSmall();
			return val;
		}

		uint32 FASTCALL CBitDecoder::ReadBitsBig(uint numBits)
		{
			_bitPos -= numBits;
			uint32 val = (_value >> _bitPos) & (((uint32)1 << numBits) - 1);
			NormalizeBig();
			return val;
		}

		bool CBitDecoder::PrepareUncompressed()
		{
			if(_extraSize != 0)
				return false;
			unsigned numBits = _bitPos - 16;
			if(((_value >> 16) & (((uint32)1 << numBits) - 1)) != 0)
				return false;
			_buf -= 2;
			_bitPos = 0;
			return true;
		}

		uint32 CBitDecoder::ReadUInt32()
		{
			uint32 v = GetUi32(_buf);
			_buf += 4;
			return v;
		}

		void CBitDecoder::CopyTo(Byte * dest, size_t size)
		{
			memcpy(dest, _buf, size);
			_buf += size;
		}

		Byte CBitDecoder::DirectReadByte()
		{
			if(_buf > _bufLim) {
				_extraSize++;
				return 0xFF;
			}
			return *_buf++;
		}

		CDecoder::CDecoder(bool wimMode) : _win(NULL), _keepHistory(false), _skipByte(false), _wimMode(wimMode), _numDictBits(15),
			_unpackBlockSize(0), _x86_buf(NULL), _x86_translationSize(0), KeepHistoryForNext(true), NeedAlloc(true), _unpackedData(NULL)
		{
		}

		CDecoder::~CDecoder()
		{
			if(NeedAlloc)
				::MidFree(_win);
			::MidFree(_x86_buf);
		}

		HRESULT CDecoder::Flush()
		{
			if(_x86_translationSize != 0) {
				Byte * destData = _win + _writePos;
				uint32 curSize = _pos - _writePos;
				if(KeepHistoryForNext) {
					if(!_x86_buf) {
						// we must change it to support another chunk sizes
						const size_t kChunkSize = (size_t)1 << 15;
						if(curSize > kChunkSize)
							return E_NOTIMPL;
						_x86_buf = (Byte *)::MidAlloc(kChunkSize);
						if(!_x86_buf)
							return E_OUTOFMEMORY;
					}
					memcpy(_x86_buf, destData, curSize);
					_unpackedData = _x86_buf;
					destData = _x86_buf;
				}
				x86_Filter(destData, (uint32)curSize, _x86_processedSize, _x86_translationSize);
				_x86_processedSize += (uint32)curSize;
				if(_x86_processedSize >= ((uint32)1 << 30))
					_x86_translationSize = 0;
			}

			return S_OK;
		}
		uint32 CDecoder::ReadBits(uint numBits) { return _bitStream.ReadBitsSmall(numBits); }

		#define RIF(x) { if(!(x)) return false;	}

		bool CDecoder::ReadTable(Byte * levels, unsigned numSymbols)
		{
			{
				Byte levels2[kLevelTableSize];
				for(uint i = 0; i < kLevelTableSize; i++)
					levels2[i] = (Byte)ReadBits(kNumLevelBits);
				RIF(_levelDecoder.Build(levels2));
			}
			unsigned i = 0;
			do {
				uint32 sym = _levelDecoder.Decode(&_bitStream);
				if(sym <= kNumHuffmanBits) {
					int delta = (int)levels[i] - (int)sym;
					delta += (delta < 0) ? (kNumHuffmanBits + 1) : 0;
					levels[i++] = (Byte)delta;
					continue;
				}

				unsigned num;
				Byte symbol;

				if(sym < kLevelSym_Same) {
					sym -= kLevelSym_Zero1;
					num = kLevelSym_Zero1_Start + ((uint)sym << kLevelSym_Zero1_NumBits) +
								(uint)ReadBits(kLevelSym_Zero1_NumBits + sym);
					symbol = 0;
				}
				else if(sym == kLevelSym_Same) {
					num = kLevelSym_Same_Start + (uint)ReadBits(kLevelSym_Same_NumBits);
					sym = _levelDecoder.Decode(&_bitStream);
					if(sym > kNumHuffmanBits)
						return false;
					int delta = (int)levels[i] - (int)sym;
					delta += (delta < 0) ? (kNumHuffmanBits + 1) : 0;
					symbol = (Byte)delta;
				}
				else
					return false;

				unsigned limit = i + num;
				if(limit > numSymbols)
					return false;

				do
					levels[i++] = symbol;
				while(i < limit);
			}
			while(i < numSymbols);

			return true;
		}

		bool CDecoder::ReadTables(void)
		{
			{
				if(_skipByte) {
					if(_bitStream.DirectReadByte() != 0)
						return false;
				}
				_bitStream.NormalizeBig();
				unsigned blockType = (uint)ReadBits(kBlockType_NumBits);
				if(blockType > kBlockType_Uncompressed)
					return false;
				_unpackBlockSize = (1 << 15);
				if(!_wimMode || ReadBits(1) == 0) {
					_unpackBlockSize = ReadBits(16);
					// wimlib supports chunks larger than 32KB (unsupported my MS wim).
					if(!_wimMode || _numDictBits >= 16) {
						_unpackBlockSize <<= 8;
						_unpackBlockSize |= ReadBits(8);
					}
				}
				PRF(printf("\nBlockSize = %6d   %s  ", _unpackBlockSize, (_pos & 1) ? "@@@" : "   "));
				_isUncompressedBlock = (blockType == kBlockType_Uncompressed);
				_skipByte = false;
				if(_isUncompressedBlock) {
					_skipByte = ((_unpackBlockSize & 1) != 0);
					PRF(printf(" UncompressedBlock "));
					if(_unpackBlockSize & 1) {
						PRF(printf(" ######### "));
					}
					if(!_bitStream.PrepareUncompressed())
						return false;
					if(_bitStream.GetRem() < kNumReps * 4)
						return false;
					for(uint i = 0; i < kNumReps; i++) {
						uint32 rep = _bitStream.ReadUInt32();
						if(rep > _winSize)
							return false;
						_reps[i] = rep;
					}
					return true;
				}
				_numAlignBits = 64;
				if(blockType == kBlockType_Aligned) {
					Byte levels[kAlignTableSize];
					_numAlignBits = kNumAlignBits;
					for(uint i = 0; i < kAlignTableSize; i++)
						levels[i] = (Byte)ReadBits(kNumAlignLevelBits);
					RIF(_alignDecoder.Build(levels));
				}
			}
			RIF(ReadTable(_mainLevels, 256));
			RIF(ReadTable(_mainLevels + 256, _numPosLenSlots));
			unsigned end = 256 + _numPosLenSlots;
			memzero(_mainLevels + end, kMainTableSize - end);
			RIF(_mainDecoder.Build(_mainLevels));
			RIF(ReadTable(_lenLevels, kNumLenSymbols));
			return _lenDecoder.Build(_lenLevels);
		}

		HRESULT CDecoder::CodeSpec(uint32 curSize)
		{
			if(!_keepHistory || !_isUncompressedBlock)
				_bitStream.NormalizeBig();
			if(!_keepHistory) {
				_skipByte = false;
				_unpackBlockSize = 0;
				memzero(_mainLevels, kMainTableSize);
				memzero(_lenLevels, kNumLenSymbols);
				{
					_x86_translationSize = 12000000;
					if(!_wimMode) {
						_x86_translationSize = 0;
						if(ReadBits(1) != 0) {
							uint32 v = ReadBits(16) << 16;
							v |= ReadBits(16);
							_x86_translationSize = v;
						}
					}
					_x86_processedSize = 0;
				}
				_reps[0] = 1;
				_reps[1] = 1;
				_reps[2] = 1;
			}
			while(curSize > 0) {
				if(_bitStream.WasExtraReadError_Fast())
					return S_FALSE;
				if(_unpackBlockSize == 0) {
					if(!ReadTables())
						return S_FALSE;
					continue;
				}
				uint32 next = _unpackBlockSize;
				SETMIN(next, curSize);
				if(_isUncompressedBlock) {
					size_t rem = _bitStream.GetRem();
					if(rem == 0)
						return S_FALSE;
					SETMIN(next, (uint32)rem);
					_bitStream.CopyTo(_win + _pos, next);
					_pos += next;
					curSize -= next;
					_unpackBlockSize -= next;
					// we don't know where skipByte can be placed, if it's end of chunk:
					// 1) in current chunk - there are such cab archives, if chunk is last
					// 2) in next chunk - are there such archives ? 
					if(_skipByte && _unpackBlockSize == 0 && curSize == 0 && _bitStream.IsOneDirectByteLeft()) {
						_skipByte = false;
						if(_bitStream.DirectReadByte() != 0)
							return S_FALSE;
					}
					continue;
				}
				curSize -= next;
				_unpackBlockSize -= next;
				Byte * win = _win;
				while(next > 0) {
					if(_bitStream.WasExtraReadError_Fast())
						return S_FALSE;
					uint32 sym = _mainDecoder.Decode(&_bitStream);
					if(sym < 256) {
						win[_pos++] = (Byte)sym;
						next--;
						continue;
					}
					{
						sym -= 256;
						if(sym >= _numPosLenSlots)
							return S_FALSE;
						uint32 posSlot = sym / kNumLenSlots;
						uint32 lenSlot = sym % kNumLenSlots;
						uint32 len = kMatchMinLen + lenSlot;
						if(lenSlot == kNumLenSlots - 1) {
							uint32 lenTemp = _lenDecoder.Decode(&_bitStream);
							if(lenTemp >= kNumLenSymbols)
								return S_FALSE;
							len = kMatchMinLen + kNumLenSlots - 1 + lenTemp;
						}
						uint32 dist;
						if(posSlot < kNumReps) {
							dist = _reps[posSlot];
							_reps[posSlot] = _reps[0];
							_reps[0] = dist;
						}
						else {
							uint   numDirectBits;
							if(posSlot < kNumPowerPosSlots) {
								numDirectBits = (uint)(posSlot >> 1) - 1;
								dist = ((2 | (posSlot & 1)) << numDirectBits);
							}
							else {
								numDirectBits = kNumLinearPosSlotBits;
								dist = ((posSlot - 0x22) << kNumLinearPosSlotBits);
							}
							if(numDirectBits >= _numAlignBits) {
								dist += (_bitStream.ReadBitsSmall(numDirectBits - kNumAlignBits) << kNumAlignBits);
								uint32 alignTemp = _alignDecoder.Decode(&_bitStream);
								if(alignTemp >= kAlignTableSize)
									return S_FALSE;
								dist += alignTemp;
							}
							else
								dist += _bitStream.ReadBitsBig(numDirectBits);
							dist -= kNumReps - 1;
							_reps[2] = _reps[1];
							_reps[1] = _reps[0];
							_reps[0] = dist;
						}
						if(len > next)
							return S_FALSE;
						if(dist > _pos && !_overDict)
							return S_FALSE;
						Byte * dest = win + _pos;
						const uint32 mask = (_winSize - 1);
						uint32 srcPos = (_pos - dist) & mask;
						next -= len;
						if(len > _winSize - srcPos) {
							_pos += len;
							do {
								*dest++ = win[srcPos++];
								srcPos &= mask;
							} while(--len);
						}
						else {
							ptrdiff_t src = (ptrdiff_t)srcPos - (ptrdiff_t)_pos;
							_pos += len;
							const Byte * lim = dest + len;
							*(dest) = *(dest + src);
							dest++;
							do {
								*(dest) = *(dest + src);
							} while(++dest != lim);
						}
					}
				}
			}
			return _bitStream.WasFinishedOK() ? S_OK : S_FALSE;
		}
		HRESULT CDecoder::Code(const Byte * inData, size_t inSize, uint32 outSize)
		{
			if(!_keepHistory) {
				_pos = 0;
				_overDict = false;
			}
			else if(_pos == _winSize) {
				_pos = 0;
				_overDict = true;
			}
			_writePos = _pos;
			_unpackedData = _win + _pos;
			if(outSize > _winSize - _pos)
				return S_FALSE;
			PRF(printf("\ninSize = %d", inSize));
			if((inSize & 1) != 0) {
				PRF(printf(" ---------"));
			}
			if(inSize < 1)
				return S_FALSE;
			_bitStream.Init(inData, inSize);
			HRESULT res = CodeSpec(outSize);
			HRESULT res2 = Flush();
			return (res == S_OK ? res2 : res);
		}

		HRESULT CDecoder::SetParams2(unsigned numDictBits)
		{
			_numDictBits = numDictBits;
			if(numDictBits < kNumDictBits_Min || numDictBits > kNumDictBits_Max)
				return E_INVALIDARG;
			unsigned numPosSlots = (numDictBits < 20) ? numDictBits * 2 : 34 + ((uint)1 << (numDictBits - 17));
			_numPosLenSlots = numPosSlots * kNumLenSlots;
			return S_OK;
		}

		HRESULT CDecoder::SetParams_and_Alloc(unsigned numDictBits)
		{
			RINOK(SetParams2(numDictBits));
			uint32 newWinSize = (uint32)1 << numDictBits;
			if(NeedAlloc) {
				if(!_win || newWinSize != _winSize) {
					::MidFree(_win);
					_winSize = 0;
					_win = (Byte *)::MidAlloc(newWinSize);
					if(!_win)
						return E_OUTOFMEMORY;
				}
			}
			_winSize = (uint32)newWinSize;
			return S_OK;
		}
	}
}
