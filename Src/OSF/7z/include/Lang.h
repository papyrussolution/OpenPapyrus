// Common/Lang.h

#ifndef __COMMON_LANG_H
#define __COMMON_LANG_H

class CLang {
	wchar_t * _text;
	CRecordVector<uint32> _ids;
	CRecordVector<uint32> _offsets;
	bool OpenFromString(const AString &s);
public:
	CLang() : _text(0) 
	{
	}
	~CLang() 
	{
		Clear();
	}
	bool Open(CFSTR fileName, const char * id);
	void Clear() throw();
	const wchar_t * Get(uint32 id) const throw();
};

#endif
