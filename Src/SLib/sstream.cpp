// SSTREAM.CPP
// Copyright (c) A.Sobolev 2002, 2007, 2009, 2010, 2015, 2016, 2017
//
// Writing/Reading some objects to/from FILE
//
// @v9.7.11 Методы этого модуля элиминированы (фактически, перенесены как static в dl200.cpp поскольку только там и используются).
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#if 0 // @v9.7.11  { 

int SLAPI WriteSArrayToFile(const SArray * pAry, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 i, c = pAry ? pAry->getCount() : 0;
	size_t item_size = pAry ? pAry->getItemSize() : 0;
	long   beg_pos = ftell(pStream);
	THROW_V(fwrite(&c, sizeof(c), 1, pStream) == 1, SLERR_WRITEFAULT);
	for(i = 0; i < c; i++)
		THROW_V(fwrite(pAry->at(i), item_size, 1, pStream) == 1, SLERR_WRITEFAULT);
	CATCH
		fseek(pStream, beg_pos, SEEK_SET);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ReadSArrayFromFile(SArray * pAry, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 i, c = 0;
	size_t item_size = pAry->getItemSize();
	long   beg_pos = ftell(pStream);
	pAry->freeAll();
	char * p_buf = (char*)SAlloc::M(item_size);
	THROW(p_buf);
	THROW_V(fread(&c, sizeof(c), 1, pStream) == 1, SLERR_READFAULT);
	for(i = 0; i < c; i++) {
		SLibError = SLERR_READFAULT;
		THROW_V(fread(p_buf, item_size, 1, (FILE*)pStream) == 1, SLERR_READFAULT);
		THROW_V(pAry->insert(p_buf), SLERR_READFAULT);
	}
	CATCH
		if(beg_pos >= 0)
			fseek(pStream, beg_pos, SEEK_SET);
		ok = 0;
	ENDCATCH
	SAlloc::F(p_buf);
	return ok;
}

int SLAPI WritePStrToFile(const char * pStr, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 s;
	if(pStr) {
		s = (uint16)(strlen(pStr)+1);
		THROW_V(fwrite(&s, sizeof(s), 1, pStream) == 1, SLERR_WRITEFAULT);
		THROW_V(fwrite(pStr, s, 1, pStream) == 1, SLERR_WRITEFAULT);
	}
	else {
		s = 0;
		THROW_V(fwrite(&s, sizeof(s), 1, pStream) == 1, SLERR_WRITEFAULT);
	}
	CATCHZOK
	return ok;
}

int SLAPI ReadPStrFromFile(char ** ppStr, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 s;
	THROW_V(fread(&s, sizeof(s), 1, pStream) == 1, SLERR_READFAULT);
	if(s) {
		THROW_V(*ppStr = new char[s], SLERR_NOMEM);
		THROW_V(fread(*ppStr, s, 1, pStream) == 1, SLERR_READFAULT);
	}
	else
		*ppStr = 0;
	CATCHZOK
	return ok;
}

int SLAPI ReadPStrFromFile(SString & rStr, FILE * pStream)
{
	int    ok = 1;
	uint16 s;
	rStr = 0;
	THROW_S(fread(&s, sizeof(s), 1, pStream) == 1, SLERR_READFAULT);
	if(s) {
		STempBuffer temp_buf(s);
		THROW_S(temp_buf.IsValid(), SLERR_NOMEM);
		THROW_S(fread(temp_buf, s, 1, pStream) == 1, SLERR_READFAULT);
		rStr = temp_buf;
	}
	CATCHZOK
	return ok;
}

#endif // } 0 @v9.7.11 
