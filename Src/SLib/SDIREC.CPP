// SDIREC.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2009, 2010, 2011, 2013, 2015, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

IMPL_CMPFUNC(SDirEntry_Time, i1, i2)
{
	LDATE d1 = ((SDirEntry *)i1)->WriteTime.d;
	LDATE d2 = ((SDirEntry *)i2)->WriteTime.d;
	LTIME t1 = ((SDirEntry *)i1)->WriteTime.t;
	LTIME t2 = ((SDirEntry *)i2)->WriteTime.t;
	if(d1 > d2) return 1;
	if(d1 < d2) return -1;
	if(t1 > t2) return 1;
	if(t1 < t2) return -1;
	return 0;
}

SLAPI SDirec::SDirec()
{
	InitFlag = 0;
	memzero(InitPath, sizeof(InitPath));
	lPosData = -1;
	vPosData = 0;
	PosDataSize = 0;
}

SLAPI SDirec::SDirec(const char * pPath, int enumDir)
{
	InitFlag = 0;
	lPosData = -1;
	vPosData = 0;
	Init(pPath, enumDir);
}

#define FFIND_DATA struct _finddatai64_t

int SLAPI SDirEntry::IsFolder() const
{
	return BIN(Attr & 0x10);
}

int SLAPI SDirEntry::IsUpFolder() const
{
	return BIN(FileName[0] == '.' && FileName[1] == '.' && FileName[2] == 0);
}

int SLAPI SDirEntry::IsSelf() const
{
	return BIN(FileName[0] == '.' && FileName[1] == 0);
}

int SLAPI SDirEntry::IsFile() const
{
	return !(IsSelf() || IsUpFolder() || IsFolder());
}

SDirEntry & FASTCALL SDirEntry::operator = (const _finddatai64_t & rS)
{
	THISZERO();
	struct tm * p_tb = 0, * p_modify_tb = 0, tb, modify_tb;
	STRNSCPY(FileName, rS.name);
	Attr = rS.attrib;
	Size = rS.size;
	//
	// Modify time
	//
	p_tb = p_modify_tb = localtime(&rS.time_write);
	if(!p_tb) {
		MEMSZERO(tb);
		p_tb = &tb;
	}
	else {
		modify_tb = *p_modify_tb;
		p_modify_tb = &modify_tb;
	}
	WriteTime.d = encodedate(p_tb->tm_mday, p_tb->tm_mon+1, p_tb->tm_year+1900);
	WriteTime.t = encodetime(p_tb->tm_hour, p_tb->tm_min, p_tb->tm_sec, 0);
	//
	// Create time
	//
	p_tb = localtime(&rS.time_create);
	if(!p_tb)
		if(p_modify_tb)
			p_tb = p_modify_tb;
		else {
			MEMSZERO(tb);
			p_tb = &tb;
		}
	CreateTime.d = encodedate(p_tb->tm_mday, p_tb->tm_mon+1, p_tb->tm_year+1900);
	CreateTime.t = encodetime(p_tb->tm_hour, p_tb->tm_min, p_tb->tm_sec, 0);
	//
	// Access time
	//
	p_tb = localtime(&rS.time_access);
	if(!p_tb)
		if(p_modify_tb)
			p_tb = p_modify_tb;
		else {
			MEMSZERO(tb);
			p_tb = &tb;
		}
	AccessTime.d = encodedate(p_tb->tm_mday, p_tb->tm_mon+1, p_tb->tm_year+1900);
	AccessTime.t = encodetime(p_tb->tm_hour, p_tb->tm_min, p_tb->tm_sec, 0);
	return *this;
}

SDirEntry & FASTCALL SDirEntry::operator = (const WIN32_FIND_DATA & rS)
{
	THISZERO();
	STRNSCPY(FileName, rS.cFileName);
	Attr = rS.dwFileAttributes;
	{
		const int64 _mult = (int64)MAXDWORD+1;
		Size = ((int64)rS.nFileSizeHigh * _mult) + rS.nFileSizeLow;
	}
	WriteTime = rS.ftLastWriteTime;
	CreateTime = rS.ftCreationTime;
	AccessTime = rS.ftLastAccessTime;
	return *this;
}

int SLAPI SDirec::FindFirst()
{
	lPosData = _findfirsti64(InitPath, (FFIND_DATA*)vPosData);
	return (lPosData >= 0) ? 1 : -1;
}

int SLAPI SDirec::FindNext()
{
	return (_findnexti64(lPosData, (FFIND_DATA*)vPosData) == 0) ? 1 : -1;
}

int SLAPI SDirec::FindClose()
{
	if(lPosData >= 0) {
		_findclose(lPosData);
		lPosData = -1;
	}
	return 1;
}

SLAPI SDirec::~SDirec()
{
	free(vPosData);
}

int SLAPI SDirec::Init(const char * pPathName, int enumDir)
{
	FindClose();
	STRNSCPY(InitPath, pPathName);
	InitFlag = 1;
	EnumDir = enumDir;
	free(vPosData);
	vPosData = malloc(PosDataSize = sizeof(FFIND_DATA));
	return 1;
}

int FASTCALL SDirec::Next(SDirEntry * pEntry)
{
	int    ok = 0;
	do {
		if(InitFlag > 0)
			ok = FindFirst();
		else if(InitFlag < 0)
			ok = FindNext();
		else
			ok = 0;
		if(ok > 0) {
			CurEntry = *(FFIND_DATA*)vPosData;
			InitFlag = -1;
		}
	} while(ok > 0 && (EnumDir && (CurEntry.Attr & 0x10L) == 0));
	if(ok > 0) {
		ASSIGN_PTR(pEntry, CurEntry);
	}
	else {
		InitFlag = 0;
		FindClose();
	}
	return ok;
}
