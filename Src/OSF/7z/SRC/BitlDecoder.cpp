// BitlDecoder.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

namespace NBitl {
	Byte kInvertTable[256];

	struct CInverterTableInitializer {
		CInverterTableInitializer()
		{
			for(uint i = 0; i < 256; i++) {
				uint   x = ((i & 0x55) << 1) | ((i & 0xAA) >> 1);
				x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2);
				kInvertTable[i] = (Byte)(((x & 0x0F) << 4) | ((x & 0xF0) >> 4));
			}
		}
	} g_InverterTableInitializer;
}

bool FASTCALL CBitlEncoder::Create(uint32 bufSize) { return _stream.Create(bufSize); }
void FASTCALL CBitlEncoder::SetStream(ISequentialOutStream * outStream) { _stream.SetStream(outStream); }
// unsigned CBitlEncoder::GetBitPosition() const { return (8 - _bitPos); }
uint64 CBitlEncoder::GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
void FASTCALL CBitlEncoder::WriteByte(Byte b) { _stream.WriteByte(b); }

void CBitlEncoder::Init()
{
	_stream.Init();
	_bitPos = 8;
	_curByte = 0;
}

HRESULT CBitlEncoder::Flush()
{
	FlushByte();
	return _stream.Flush();
}

void CBitlEncoder::FlushByte()
{
	if(_bitPos < 8)
		_stream.WriteByte(_curByte);
	_bitPos = 8;
	_curByte = 0;
}

void CBitlEncoder::WriteBits(uint32 value, uint numBits)
{
	while(numBits > 0) {
		if(numBits < _bitPos) {
			_curByte |= (value & ((1 << numBits) - 1)) << (8 - _bitPos);
			_bitPos -= numBits;
			return;
		}
		numBits -= _bitPos;
		_stream.WriteByte((Byte)(_curByte | (value << (8 - _bitPos))));
		value >>= _bitPos;
		_bitPos = 8;
		_curByte = 0;
	}
}
