// PercentPrinter.cpp

#include <7z-internal.h>
#pragma hdrstop
//#include <PercentPrinter.h>

static const uint kPercentsSize = 4;

CPercentPrinter::CPercentPrinter(uint32 tickStep /*= 200*/) : _tickStep(tickStep), _prevTick(0), NeedFlush(true), MaxLen(80 - 1)
{
}

CPercentPrinter::~CPercentPrinter()
{
	ClosePrint(false);
}

CPercentPrinterState::CPercentPrinterState() : Completed(0), Total((uint64)(int64)-1), Files(0)
{
}

void CPercentPrinterState::ClearCurState()
{
	Completed = 0;
	Total = ((uint64)(int64)-1);
	Files = 0;
	Command.Empty();
	FileName.Empty();
}

void CPercentPrinter::ClosePrint(bool needFlush)
{
	unsigned num = _printedString.Len();
	if(num != 0) {
		uint i;

		/* '\r' in old MAC OS means "new line".
		   So we can't use '\r' in some systems */

  #ifdef _WIN32
		char * start = _temp.GetBuf(num  + 2);
		char * p = start;
		*p++ = '\r';
		for(i = 0; i < num; i++) *p++ = ' ';
		*p++ = '\r';
  #else
		char * start = _temp.GetBuf(num * 3);
		char * p = start;
		for(i = 0; i < num; i++) *p++ = '\b';
		for(i = 0; i < num; i++) *p++ = ' ';
		for(i = 0; i < num; i++) *p++ = '\b';
  #endif

		*p = 0;
		_temp.ReleaseBuf_SetLen((uint)(p - start));
		*_so << _temp;
	}
	if(needFlush)
		_so->Flush();
	_printedString.Empty();
}

void CPercentPrinter::GetPercents()
{
	char   s[32];
	uint   size;
	{
		char c = '%';
		uint64 val = 0;
		if(Total == (uint64)(int64)-1) {
			val = Completed >> 20;
			c = 'M';
		}
		else if(Total != 0)
			val = Completed * 100 / Total;
		ConvertUInt64ToString(val, s);
		size = (uint)sstrlen(s);
		s[size++] = c;
		s[size] = 0;
	}
	while(size < kPercentsSize) {
		_s += ' ';
		size++;
	}
	_s += s;
}

void CPercentPrinter::Print()
{
	DWORD tick = 0;
	if(_tickStep != 0)
		tick = GetTickCount();
	bool onlyPercentsChanged = false;
	if(!_printedString.IsEmpty()) {
		if(_tickStep != 0 && (uint32)(tick - _prevTick) < _tickStep)
			return;
		CPercentPrinterState &st = *this;
		if(_printedState.Command == st.Command && _printedState.FileName == st.FileName && _printedState.Files == st.Files) {
			if(_printedState.Total == st.Total && _printedState.Completed == st.Completed)
				return;
			onlyPercentsChanged = true;
		}
	}
	_s.Empty();
	GetPercents();
	if(onlyPercentsChanged && _s == _printedPercents)
		return;
	_printedPercents = _s;
	if(Files != 0) {
		char s[32];
		ConvertUInt64ToString(Files, s);
		// uint size = (uint)strlen(s);
		// for(; size < 3; size++) _s += ' ';
		_s += ' ';
		_s += s;
		// _s += "f";
	}

	if(!Command.IsEmpty()) {
		_s += ' ';
		_s += Command;
	}

	if(!FileName.IsEmpty() && _s.Len() < MaxLen) {
		_s += ' ';

		StdOut_Convert_UString_to_AString(FileName, _temp);
		_temp.Replace('\n', ' ');
		if(_s.Len() + _temp.Len() > MaxLen) {
			uint len = FileName.Len();
			for(; len != 0; ) {
				unsigned delta = len / 8;
				if(delta == 0)
					delta = 1;
				len -= delta;
				_tempU = FileName;
				_tempU.Delete(len / 2, FileName.Len() - len);
				_tempU.Insert(len / 2, L" . ");
				StdOut_Convert_UString_to_AString(_tempU, _temp);
				if(_s.Len() + _temp.Len() <= MaxLen)
					break;
			}
			if(len == 0)
				_temp.Empty();
		}
		_s += _temp;
	}
	if(_printedString != _s) {
		ClosePrint(false);
		*_so << _s;
		if(NeedFlush)
			_so->Flush();
		_printedString = _s;
	}
	_printedState = *this;
	if(_tickStep != 0)
		_prevTick = tick;
}

