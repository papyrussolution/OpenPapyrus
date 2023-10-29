// SDIREC.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop

IMPL_CMPFUNC(SDirEntry_Time, i1, i2)
{
	return cmp(static_cast<const SDirEntry *>(i1)->WriteTime, static_cast<const SDirEntry *>(i2)->WriteTime);
}

SDirec::SDirec() : InitFlag(0), lPosData(-1), vPosData(0), PosDataSize(0)
{
	memzero(InitPath, sizeof(InitPath));
}

SDirec::SDirec(const char * pPath, int enumDir) : InitFlag(0), lPosData(-1), vPosData(0)
{
	Init(pPath, enumDir);
}

#define FFIND_DATA struct _wfinddatai64_t // @v11.6.0 _finddatai64_t-->_wfinddatai64_t

SDirEntry::SDirEntry()
{
	THISZERO();
}

SString & SDirEntry::GetNameUtf8(SString & rBuf) const
{
	rBuf.CopyUtf8FromUnicode(Name, sstrlen(Name), 1);
	return rBuf;
}

SString & SDirEntry::GetNameA(SString & rBuf) const
{
	rBuf.CopyUtf8FromUnicode(Name, sstrlen(Name), 1);
	return rBuf.Utf8ToChar();
}

SString & SDirEntry::GetNameUtf8(const char * pPath, SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(!isempty(pPath))
		rBuf.Cat(pPath).SetLastSlash();
	temp_buf.CopyUtf8FromUnicode(Name, sstrlen(Name), 1);
	return rBuf.Cat(temp_buf);
}

SString & SDirEntry::GetNameA(const char * pPath, SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(!isempty(pPath))
		rBuf.Cat(pPath).SetLastSlash();
	temp_buf.CopyUtf8FromUnicode(Name, sstrlen(Name), 1);
	return rBuf.Cat(temp_buf);
}

bool   SDirEntry::IsFolder() const { return LOGIC(Attr & 0x10); }
bool   SDirEntry::IsUpFolder() const { return (Name[0] == L'.' && Name[1] == L'.' && Name[2] == 0); }
bool   SDirEntry::IsSelf() const { return (Name[0] == L'.' && Name[1] == 0); }
bool   SDirEntry::IsFile() const { return !(IsSelf() || IsUpFolder() || IsFolder()); }

static LDATETIME _tm_to_ldatetime(const struct tm * pTm)
{
	LDATETIME dtm;
	return dtm.Set(encodedate(pTm->tm_mday, pTm->tm_mon+1, pTm->tm_year+1900), encodetime(pTm->tm_hour, pTm->tm_min, pTm->tm_sec, 0));
}

SDirEntry & FASTCALL SDirEntry::operator = (const _wfinddatai64_t & rS)
{
	THISZERO();
	struct tm * p_modify_tb = 0;
	struct tm tb;
	STRNSCPY(Name, rS.name);
	Attr = rS.attrib;
	Size = rS.size;
	{
		//
		// Modify time
		//
		struct tm modify_tb;
		struct tm * p_tb = p_modify_tb = localtime(&rS.time_write);
		if(!p_tb) {
			MEMSZERO(tb);
			p_tb = &tb;
		}
		else {
			modify_tb = *p_modify_tb;
			p_modify_tb = &modify_tb;
		}
		WriteTime = _tm_to_ldatetime(p_tb);
	}
	{
		//
		// Create time
		//
		struct tm * p_tb = localtime(&rS.time_create);
		if(!p_tb) {
			if(p_modify_tb)
				p_tb = p_modify_tb;
			else {
				MEMSZERO(tb);
				p_tb = &tb;
			}
		}
		CreateTime = _tm_to_ldatetime(p_tb);
	}
	{
		//
		// Access time
		//
		struct tm * p_tb = localtime(&rS.time_access);
		if(!p_tb) {
			if(p_modify_tb)
				p_tb = p_modify_tb;
			else {
				MEMSZERO(tb);
				p_tb = &tb;
			}
		}
		AccessTime = _tm_to_ldatetime(p_tb);
	}
	return *this;
}

SDirEntry & FASTCALL SDirEntry::operator = (const WIN32_FIND_DATAA & rS)
{
	THISZERO();
	STRNSCPY(Name, SUcSwitchW(rS.cFileName));
	Attr = rS.dwFileAttributes;
	{
		const int64 _mult = static_cast<int64>(MAXDWORD)+1;
		Size = (static_cast<int64>(rS.nFileSizeHigh) * _mult) + rS.nFileSizeLow;
	}
	WriteTime = rS.ftLastWriteTime;
	CreateTime = rS.ftCreationTime;
	AccessTime = rS.ftLastAccessTime;
	return *this;
}

SDirEntry & FASTCALL SDirEntry::operator = (const WIN32_FIND_DATAW & rS)
{
	THISZERO();
	STRNSCPY(Name, rS.cFileName);
	Attr = rS.dwFileAttributes;
	{
		const int64 _mult = static_cast<int64>(MAXDWORD)+1;
		Size = (static_cast<int64>(rS.nFileSizeHigh) * _mult) + rS.nFileSizeLow;
	}
	WriteTime = rS.ftLastWriteTime;
	CreateTime = rS.ftCreationTime;
	AccessTime = rS.ftLastAccessTime;
	return *this;
}

int SDirec::FindFirst()
{
	lPosData = _wfindfirsti64(InitPath, static_cast<FFIND_DATA *>(vPosData));
	return (lPosData >= 0) ? 1 : -1;
}

int SDirec::FindNext()
{
	return (_wfindnexti64(lPosData, static_cast<FFIND_DATA *>(vPosData)) == 0) ? 1 : -1;
}

/*static*/int SDirec::GetSingle(const char * pPath, SDirEntry * pEntry)
{
	int    ok = 0;
	if(!isempty(pPath)) {
		FFIND_DATA ff;
		intptr_t ffr = _wfindfirsti64(SUcSwitchW(pPath), &ff);
		if(ffr >= 0) {
			ASSIGN_PTR(pEntry, ff);
			_findclose(ffr);
			ok = 1;
		}
	}
	return ok;
}

int SDirec::FindClose()
{
	if(lPosData >= 0) {
		_findclose(lPosData);
		lPosData = -1;
	}
	return 1;
}

SDirec::~SDirec()
{
	SAlloc::F(vPosData);
}

int SDirec::Init(const char * pPathName, int enumDir)
{
	FindClose();
	// @v11.8.7 {
	// Модификация для адекватной трактовки utf8
	SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
	SString & r_temp_buf = SLS.AcquireRvlStr();
	r_temp_buf = pPathName;
	r_temp_buf.CopyToUnicode(r_temp_buf_u);
	r_temp_buf_u.CopyTo(InitPath, SIZEOFARRAY(InitPath));
	// } @v11.8.7
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
			CurEntry = *static_cast<const FFIND_DATA *>(vPosData);
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
SFileEntryPool::Entry::Entry() : Attr(0), CreateTime(ZERODATETIME), AccessTime(ZERODATETIME), WriteTime(ZERODATETIME), Size(0)
{
}

SFileEntryPool::Entry & SFileEntryPool::Entry::Z()
{
	Attr = 0;
	CreateTime.Z();
	AccessTime.Z();
	WriteTime.Z();
	Size = 0;
	Name.Z();
	Path.Z();
	RemoteUrl.Z(); // @v10.8.2
	return *this;
}

SFileEntryPool::SFileEntryPool() : SStrGroup(), State(0), InitPathP(0)
{
}

SFileEntryPool & SFileEntryPool::Z()
{
	ClearS();
	L.clear();
	State = 0;
	InitPathP = 0;
	return *this;
}

int STDCALL SFileEntryPool::SearchName(const char * pFileName, Entry * pEntry) const
{
	int    ok = 0;
	if(!isempty(pFileName)) {
		SString temp_buf;
		for(uint i = 0; i < L.getCount(); i++) {
			const InnerEntry & r_ie = L.at(i);
			if(GetS(r_ie.NameP, temp_buf) && temp_buf.IsEqNC(pFileName)) {
				if(pEntry) {
					ConvertEntry(r_ie, *pEntry);
				}
				ok = 1;
			}
		}
	}
	return ok;
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

int SFileEntryPool::Add(const char * pPath, long flags)
{
	return Add(pPath, 0/*pRemoteUrl*/, flags);
}

int SFileEntryPool::Add(const char * pPath, const char * pRemoteUrl, long flags)
{
	int    ok = 0;
	SDirEntry de;
	if(SDirec::GetSingle(pPath, &de) > 0) {
		SString temp_buf;
		const SPathStruc ps(pPath);
		ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
		ok = Implement_Add(temp_buf, de, pRemoteUrl, flags);
	}
	return ok;
}

int SFileEntryPool::Helper_Scan(const char * pPath, const char * pWildcard, long flags)
{
	int    ok = -1;
	uint   non_ascii_count = 0; // @debug
	assert(!isempty(pPath)); // Вызывающая функция обязана проверить это условие
	SDirEntry fb;
	SString temp_buf;
	temp_buf = pPath;
	if(pWildcard) {
		temp_buf.SetLastSlash().Cat(pWildcard);
	}
	const bool do_recursive = LOGIC(flags & scanfRecursive);
	for(SDirec sdirec(temp_buf, 0); sdirec.Next(&fb) > 0;) {
		// @debug {
		if(!sisascii(fb.Name, sstrlen(fb.Name)))
			non_ascii_count++;
		// } @debug 
		if(fb.IsFile()) {
			THROW(Add(pPath, fb, flags));
			ok = 1;
		}
		else if(do_recursive && fb.IsFolder() && !fb.IsSelf() && !fb.IsUpFolder()) {
			fb.GetNameUtf8(pPath, temp_buf);
			const int r = Helper_Scan(temp_buf, pWildcard, flags); // @recursion
			THROW(r);
			if(r > 0)
				ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SFileEntryPool::Scan(const char * pPath, const char * pWildcard, long flags)
{
	Z();
	int    ok = -1;
	if(!isempty(pPath)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf = pPath; // @v11.3.12 @fix
		if(pWildcard) {
			r_temp_buf.SetLastSlash();
			SetInitPath(r_temp_buf);
		}
		else {
			const SPathStruc ps(pPath);
			ps.Merge(~(SPathStruc::fNam|SPathStruc::fExt), r_temp_buf);
			SetInitPath(r_temp_buf.SetLastSlash());
		}
		ok = Helper_Scan(pPath, pWildcard, flags);
	}
	return ok;
}

uint SFileEntryPool::GetCount() const { return L.getCount(); }

int FASTCALL SFileEntryPool::Remove(uint pos)
{
	int    ok = 1;
	if(pos < L.getCount())
		L.atFree(pos);
	else
		ok = 0;
	return ok;
}

int SFileEntryPool::Get(uint pos, SFileEntryPool::Entry * pEntry, SString * pFullPath) const
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

int SFileEntryPool::Add(SFileEntryPool::Entry & rDe, long flags)
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
		const long npf = (flags & scanfKeepCase) ? SPathStruc::npfKeepCase : 0;
		THROW(AddS(SPathStruc::NormalizePath(rDe.Path, npf, r_temp_path), &new_entry.PathP));
    }
    THROW(AddS(rDe.Name, &new_entry.NameP));
    THROW(L.insert(&new_entry));
    CATCHZOK
    return ok;
}

int SFileEntryPool::Add(const char * pPath, const SDirEntry & rDe, long flags)
{
	return Implement_Add(pPath, rDe, 0, flags);
}

int SFileEntryPool::Implement_Add(const char * pPath, const SDirEntry & rDe, const char * pRemoteUrl, long flags)
{
	int    ok = 1;
    InnerEntry new_entry;
    MEMSZERO(new_entry);
    new_entry.Attr = rDe.Attr;
    new_entry.CreateTime = rDe.CreateTime;
    new_entry.AccessTime = rDe.AccessTime;
    new_entry.WriteTime = rDe.WriteTime;
    new_entry.Size = rDe.Size;
    if(!isempty(pPath)) {
		SString & r_temp_path = SLS.AcquireRvlStr();
		const long npf = (flags & scanfKeepCase) ? SPathStruc::npfKeepCase : 0;
		THROW(AddS(SPathStruc::NormalizePath(pPath, npf, r_temp_path), &new_entry.PathP));
    }
	if(!isempty(pRemoteUrl)) {
		THROW(AddS(pRemoteUrl, &new_entry.RemoteUrlP));
	}
	{
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v11.6.0
		r_temp_buf.CopyUtf8FromUnicode(rDe.Name, sstrlen(rDe.Name), 1); // @v11.6.0
		THROW(AddS(/*rDe.Name*/r_temp_buf, &new_entry.NameP));
	}
    THROW(L.insert(&new_entry));
    CATCHZOK
    return ok;
}

void SFileEntryPool::ConvertEntry(const InnerEntry & rIEntry, Entry & rEntry) const
{
	rEntry.Attr = rIEntry.Attr;
	rEntry.CreateTime = rIEntry.CreateTime;
	rEntry.AccessTime = rIEntry.AccessTime;
	rEntry.WriteTime = rIEntry.WriteTime;
	rEntry.Size = rIEntry.Size;
    GetS(rIEntry.PathP, rEntry.Path);
    GetS(rIEntry.NameP, rEntry.Name);
	GetS(rIEntry.RemoteUrlP, rEntry.RemoteUrl); // @v10.8.2
}

struct SFileEntryPool_SortExtraBlock {
    SFileEntryPool * P_Pool;
    int    Order;
    SString TempBuf1;
    SString TempBuf2;
};

IMPL_CMPFUNC(SFileEntryPool_Entry, p1, p2)
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

void SFileEntryPool::Sort(int criteria)
{
	SFileEntryPool_SortExtraBlock blk;
	blk.Order = criteria;
	blk.P_Pool = this;
	L.sort(PTR_CMPFUNC(SFileEntryPool_Entry), &blk);
}
