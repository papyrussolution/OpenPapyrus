// SDIREC.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2009, 2010, 2011, 2013, 2015, 2016, 2017
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
//
//
//
SLAPI SFileEntryPool::SFileEntryPool() : SStrGroup()
{
	State = 0;
}

uint SLAPI SFileEntryPool::GetCount() const
{
	return L.getCount();
}

int SLAPI SFileEntryPool::Get(uint pos, Entry & rEntry) const
{
    if(pos < L.getCount()) {
		ConvertEntry(L.at(pos), rEntry);
		return 1;
    }
    else
		return 0;
}

int SLAPI SFileEntryPool::Add(const char * pPath, SDirEntry & rDe)
{
	int    ok = 1;
    InnerEntry new_entry;
    MEMSZERO(new_entry);
    new_entry.Attr = rDe.Attr;
    new_entry.CreateTime = rDe.CreateTime;
    new_entry.AccessTime = rDe.AccessTime;
    new_entry.WriteTime = rDe.WriteTime;
    new_entry.Size = rDe.Size;
    if(pPath) {
		SString temp_path;
		SPathStruc::NormalizePath(pPath, 0, temp_path);
		THROW(AddS(temp_path, &new_entry.PathP));
    }
    THROW(AddS(rDe.FileName, &new_entry.NameP));
    THROW(L.insert(&new_entry));
    CATCHZOK
    return ok;
}

void SLAPI SFileEntryPool::ConvertEntry(const InnerEntry & rIEntry, Entry & rEntry) const
{
	rEntry.Attr = rIEntry.Attr;
	rEntry.CreateTime = rIEntry.CreateTime;
	rEntry.AccessTime = rIEntry.AccessTime;
	rEntry.WriteTime = rIEntry.WriteTime;
	rEntry.Size = rIEntry.Size;
    GetS(rIEntry.PathP, rEntry.Path);
    GetS(rIEntry.NameP, rEntry.Name);
}

struct SFileEntryPool_SortExtraBlock {
    SFileEntryPool * P_Pool;
    int    Order;
    SString TempBuf1;
    SString TempBuf2;
};

IMPL_CMPCFUNC(SFileEntryPool_Entry, p1, p2)
{
	int    si = 0;
	SFileEntryPool_SortExtraBlock * p_blk = (SFileEntryPool_SortExtraBlock *)pExtraData;
    const SFileEntryPool::InnerEntry * p_entry1 = (const SFileEntryPool::InnerEntry *)p1;
    const SFileEntryPool::InnerEntry * p_entry2 = (const SFileEntryPool::InnerEntry *)p2;
    const int _desc = BIN(p_blk->Order & SFileEntryPool::scDesc);
    switch(p_blk->Order & ~SFileEntryPool::scDesc) {
    	case SFileEntryPool::scByName:
    		{
				p_blk->P_Pool->GetS(p_entry1->NameP, p_blk->TempBuf1);
				p_blk->P_Pool->GetS(p_entry2->NameP, p_blk->TempBuf2);
                si = p_blk->TempBuf1.CmpNC(p_blk->TempBuf2);
    		}
			break;
    	case SFileEntryPool::scByExt:
    		{
    			SString ext1, ext2;
				p_blk->P_Pool->GetS(p_entry1->NameP, p_blk->TempBuf1);
				p_blk->P_Pool->GetS(p_entry2->NameP, p_blk->TempBuf2);
				SPathStruc::GetExt(p_blk->TempBuf1, &ext1);
				SPathStruc::GetExt(p_blk->TempBuf2, &ext2);
				si = ext1.CmpNC(ext2);
				SETIFZ(si, p_blk->TempBuf1.CmpNC(p_blk->TempBuf2));
    		}
    		break;
    	case SFileEntryPool::scByFullName:
    		{
    			SString fp1, fp2;
				p_blk->P_Pool->GetS(p_entry1->PathP, fp1);
				p_blk->P_Pool->GetS(p_entry2->PathP, fp2);
				p_blk->P_Pool->GetS(p_entry1->NameP, p_blk->TempBuf1);
				p_blk->P_Pool->GetS(p_entry2->NameP, p_blk->TempBuf2);
				fp1.SetLastSlash().Cat(p_blk->TempBuf1);
				fp2.SetLastSlash().Cat(p_blk->TempBuf2);
				si = fp1.CmpNC(fp2);
    		}
    		break;
    	case SFileEntryPool::scByCrTime:
			si = cmp(p_entry1->CreateTime, p_entry2->CreateTime);
    		break;
    	case SFileEntryPool::scByAccsTime:
    		si = cmp(p_entry1->AccessTime, p_entry2->AccessTime);
    		break;
    	case SFileEntryPool::scByWrTime:
    		si = cmp(p_entry1->WriteTime, p_entry2->WriteTime);
    		break;
    	case SFileEntryPool::scBySize:
    		si = CMPSIGN(p_entry1->Size, p_entry2->Size);
    		break;
    }
    return (si && _desc) ? -si : si;
}

int SLAPI SFileEntryPool::Sort(int criteria)
{
	SFileEntryPool_SortExtraBlock blk;
	blk.Order = criteria;
	blk.P_Pool = this;
	return L.sort(PTR_CMPCFUNC(SFileEntryPool_Entry), &blk);
}
