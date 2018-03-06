// LzOutWindow.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

void CLzOutWindow::Init(bool solid) throw()
{
	if(!solid)
		COutBuffer::Init();
  #ifdef _NO_EXCEPTIONS
	ErrorCode = S_OK;
  #endif
}

bool CLzOutWindow::CopyBlock(uint32 distance, uint32 len)
{
	uint32 pos = _pos - distance - 1;
	if(distance >= _pos) {
		if(!_overDict || distance >= _bufSize)
			return false;
		pos += _bufSize;
	}
	if(_limitPos - _pos > len && _bufSize - pos > len) {
		const Byte * src = _buf + pos;
		Byte * dest = _buf + _pos;
		_pos += len;
		do {
			*dest++ = *src++;
		} while(--len != 0);
	}
	else { 
		do {
			uint32 pos2;
			if(pos == _bufSize)
				pos = 0;
			pos2 = _pos;
			_buf[pos2++] = _buf[pos++];
			_pos = pos2;
			if(pos2 == _limitPos)
				FlushWithCheck();
		} while(--len != 0);
	}
	return true;
}
	
void FASTCALL CLzOutWindow::PutByte(Byte b)
{
	uint32 pos = _pos;
	_buf[pos++] = b;
	_pos = pos;
	if(pos == _limitPos)
		FlushWithCheck();
}

Byte FASTCALL CLzOutWindow::GetByte(uint32 distance) const
{
	uint32 pos = _pos - distance - 1;
	if(distance >= _pos)
		pos += _bufSize;
	return _buf[pos];
}
