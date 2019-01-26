// SDIREC.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

IMPL_CMPFUNC(SDirEntry_Time, i1, i2)
{
	return cmp(static_cast<const SDirEntry *>(i1)->WriteTime, static_cast<const SDirEntry *>(i2)->WriteTime);
}

SLAPI SDirec::SDirec() : InitFlag(0), lPosData(-1), vPosData(0), PosDataSize(0)
{
	memzero(InitPath, sizeof(InitPath));
}

SLAPI SDirec::SDirec(const char * pPath, int enumDir) : InitFlag(0), lPosData(-1), vPosData(0)
{
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
	STRNSCPY(FileName, rS.cFileName); // @unicodeproblem
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
	SAlloc::F(vPosData);
}

int SLAPI SDirec::Init(const char * pPathName, int enumDir)
{
	FindClose();
	STRNSCPY(InitPath, pPathName);
	InitFlag = 1;
	EnumDir = enumDir;
	SAlloc::F(vPosData);
	vPosData = SAlloc::M(PosDataSize = sizeof(FFIND_DATA));
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
SLAPI SFileEntryPool::Entry::Entry()
{
	Clear();
}

void SLAPI SFileEntryPool::Entry::Clear()
{
	Attr = 0;
	CreateTime.Z();
	AccessTime.Z();
	WriteTime.Z();
	Size = 0;
	Name.Z();
	Path.Z();
}

SLAPI SFileEntryPool::SFileEntryPool() : SStrGroup(), State(0), InitPathP(0)
{
}

void SLAPI SFileEntryPool::Clear()
{
	ClearS();
	L.clear();
	State = 0;
	InitPathP = 0;
}

void FASTCALL SFileEntryPool::SetInitPath(const char * pPath)
{
	SString temp_buf;
	GetS(InitPathP, temp_buf);
	if(temp_buf != pPath) {
		(temp_buf = pPath).Strip();
		AddS(temp_buf, &InitPathP);
	}
}

int FASTCALL SFileEntryPool::GetInitPath(SString & rPath) const
{
	GetS(InitPathP, rPath);
	return rPath.NotEmpty() ? 1 : -1;
}

int SLAPI SFileEntryPool::Scan(const char * pPath, const char * pWildcard, long flags)
{
	int    ok = -1;
	Clear();
	if(!isempty(pPath)) {
		SDirEntry fb;
		SString srch_path = pPath;
		if(pWildcard) {
			srch_path.SetLastSlash();
			SetInitPath(srch_path);
			srch_path.Cat(pWildcard);
		}
		else {
			SString temp_buf;
			SPathStruc ps(pPath);
			ps.Merge(~(SPathStruc::fNam|SPathStruc::fExt), temp_buf);
			SetInitPath(temp_buf.SetLastSlash());
		}
		for(SDirec sdirec(srch_path); sdirec.Next(&fb) > 0;) {
			THROW(Add(pPath, fb));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

uint SLAPI SFileEntryPool::GetCount() const
{
	return L.getCount();
}

int FASTCALL SFileEntryPool::Remove(uint pos)
{
	int    ok = 1;
	if(pos < L.getCount())
		L.atFree(pos);
	else
		ok = 0;
	return ok;
}

int SLAPI SFileEntryPool::Get(uint pos, SFileEntryPool::Entry * pEntry, SString * pFullPath) const
{
	CALLPTRMEMB(pFullPath, Z());
    if(pos < L.getCount()) {
		const InnerEntry & r_item = L.at(pos);
		if(pEntry) {
			ConvertEntry(r_item, *pEntry);
			if(pFullPath)
				pFullPath->Z().Cat(pEntry->Path).SetLastSlash().Cat(pEntry->Name);
		}
		else if(pFullPath) {
			pFullPath->Z();
			SString & r_temp_buf = SLS.AcquireRvlStr();
			GetS(r_item.PathP, r_temp_buf);
			pFullPath->Cat(r_temp_buf);
			GetS(r_item.NameP, r_temp_buf);
			pFullPath->SetLastSlash().Cat(r_temp_buf);
		}
		return 1;
    }
    else
		return 0;
}

int SLAPI SFileEntryPool::Add(SFileEntryPool::Entry & rDe)
{
	int    ok = 1;
    InnerEntry new_entry;
    MEMSZERO(new_entry);
    new_entry.Attr = rDe.Attr;
    new_entry.CreateTime = rDe.CreateTime;
    new_entry.AccessTime = rDe.AccessTime;
    new_entry.WriteTime = rDe.WriteTime;
    new_entry.Size = rDe.Size;
    if(rDe.Path.NotEmptyS()) {
		SString & r_temp_path = SLS.AcquireRvlStr();
		THROW(AddS(SPathStruc::NormalizePath(rDe.Path, 0, r_temp_path), &new_entry.PathP));
    }
    THROW(AddS(rDe.Name, &new_entry.NameP));
    THROW(L.insert(&new_entry));
    CATCHZOK
    return ok;
}

int SLAPI SFileEntryPool::Add(const char * pPath, const SDirEntry & rDe)
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
		SString & r_temp_path = SLS.AcquireRvlStr();
		THROW(AddS(SPathStruc::NormalizePath(pPath, 0, r_temp_path), &new_entry.PathP));
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
	SFileEntryPool_SortExtraBlock * p_blk = static_cast<SFileEntryPool_SortExtraBlock *>(pExtraData);
    const SFileEntryPool::InnerEntry * p_entry1 = static_cast<const SFileEntryPool::InnerEntry *>(p1);
    const SFileEntryPool::InnerEntry * p_entry2 = static_cast<const SFileEntryPool::InnerEntry *>(p2);
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
    	case SFileEntryPool::scByCrTime: si = cmp(p_entry1->CreateTime, p_entry2->CreateTime); break;
    	case SFileEntryPool::scByAccsTime: si = cmp(p_entry1->AccessTime, p_entry2->AccessTime); break;
    	case SFileEntryPool::scByWrTime: si = cmp(p_entry1->WriteTime, p_entry2->WriteTime); break;
    	case SFileEntryPool::scBySize: si = CMPSIGN(p_entry1->Size, p_entry2->Size); break;
    }
    return (si && _desc) ? -si : si;
}

void SLAPI SFileEntryPool::Sort(int criteria)
{
	SFileEntryPool_SortExtraBlock blk;
	blk.Order = criteria;
	blk.P_Pool = this;
	L.sort(PTR_CMPCFUNC(SFileEntryPool_Entry), &blk);
}
