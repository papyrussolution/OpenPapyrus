// SDIREC.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023, 2024
//
#include <slib-internal.h>
#pragma hdrstop

IMPL_CMPFUNC(SDirEntry_Time, i1, i2)
{
	//return cmp(static_cast<const SDirEntry *>(i1)->ModTime, static_cast<const SDirEntry *>(i2)->ModTime);
	return CMPSIGN(static_cast<const SDirEntry *>(i1)->ModTm_, static_cast<const SDirEntry *>(i2)->ModTm_);
}

SDirec::SDirec() : InitFlag(0), /*lPosData(-1),*/vPosData(0), PosDataSize(0)
{
	memzero(InitPath, sizeof(InitPath));
}

SDirec::SDirec(const char * pPath, int enumDir) : InitFlag(0), /*lPosData(-1),*/vPosData(0)
{
	Init(pPath, enumDir);
}

//#define FFIND_DATA struct _wfinddatai64_t // @v11.6.0 _finddatai64_t-->_wfinddatai64_t

SDirEntry::SDirEntry() : SFile::Stat()
{
	//THISZERO();
	Name[0] = 0;
}

SDirEntry & SDirEntry::Z()
{
	SFile::Stat::Z();
	Name[0] = 0;
	return *this;
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
	//struct tm tb;
	STRNSCPY(Name, rS.name);
	Attr = rS.attrib;
	Size = rS.size;
	{
		//
		// Modify time
		//
		ModTm_ = SUniTime_Internal::EpochToNs100(rS.time_write); // @v11.8.11
		/* @v11.8.11
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
		ModTime = _tm_to_ldatetime(p_tb);
		*/
	}
	{
		//
		// Create time
		//
		CrtTm_ = SUniTime_Internal::EpochToNs100(rS.time_create); // @v11.8.11
		/* @v11.8.11
		struct tm * p_tb = localtime(&rS.time_create);
		if(!p_tb) {
			if(p_modify_tb)
				p_tb = p_modify_tb;
			else {
				MEMSZERO(tb);
				p_tb = &tb;
			}
		}
		CrtTime = _tm_to_ldatetime(p_tb);
		*/
	}
	{
		//
		// Access time
		//
		AccsTm_ = SUniTime_Internal::EpochToNs100(rS.time_access); // @v11.8.11
		/* @v11.8.11
		struct tm * p_tb = localtime(&rS.time_access);
		if(!p_tb) {
			if(p_modify_tb)
				p_tb = p_modify_tb;
			else {
				MEMSZERO(tb);
				p_tb = &tb;
			}
		}
		AccsTime = _tm_to_ldatetime(p_tb);
		*/
	}
	if(Attr & SFile::attrReparsePoint) {
		//ReparsePointTag = rS
	}
	else {
		ReparsePointTag = 0;
	}
	return *this;
}

/*SDirEntry & FASTCALL SDirEntry::operator = (const WIN32_FIND_DATAA & rS)
{
	THISZERO();
	STRNSCPY(Name, SUcSwitchW(rS.cFileName));
	Attr = rS.dwFileAttributes;
	{
		const int64 _mult = static_cast<int64>(MAXDWORD)+1;
		Size = (static_cast<int64>(rS.nFileSizeHigh) * _mult) + rS.nFileSizeLow;
	}
	ModTime = rS.ftLastWriteTime;
	CrtTime = rS.ftCreationTime;
	AccsTime = rS.ftLastAccessTime;
	ReparsePointTag = (Attr & SFile::attrReparsePoint) ? rS.dwReserved0 : 0; // @v11.8.9
	return *this;
}*/

SDirEntry & FASTCALL SDirEntry::operator = (const WIN32_FIND_DATAW & rS)
{
	THISZERO();
	STRNSCPY(Name, rS.cFileName);
	Attr = rS.dwFileAttributes;
	{
		const int64 _mult = static_cast<int64>(MAXDWORD)+1;
		Size = (static_cast<int64>(rS.nFileSizeHigh) * _mult) + rS.nFileSizeLow;
	}
	// @v11.8.11 ModTime = rS.ftLastWriteTime;
	// @v11.8.11 CrtTime = rS.ftCreationTime;
	// @v11.8.11 AccsTime = rS.ftLastAccessTime;
	ModTm_ = *reinterpret_cast<const int64 *>(&rS.ftLastWriteTime); // @v11.8.11
	CrtTm_ = *reinterpret_cast<const int64 *>(&rS.ftCreationTime); // @v11.8.11
	AccsTm_ = *reinterpret_cast<const int64 *>(&rS.ftLastAccessTime); // @v11.8.11
	ReparsePointTag = (Attr & SFile::attrReparsePoint) ? rS.dwReserved0 : 0; // @v11.8.9
	return *this;
}

int SDirec::FindFirst()
{
	// @v11.8.9 lPosData = _wfindfirsti64(InitPath, static_cast<FFIND_DATA *>(vPosData));
	// @v11.8.9 return (lPosData >= 0) ? 1 : -1;
	// @v11.8.9 {
	if(vPosData) {
		H = ::FindFirstFileExW(InitPath, FindExInfoBasic, static_cast<WIN32_FIND_DATAW *>(vPosData),
			FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
	}
	else
		H.Z();
	return !H ? -1 : 1;
	// } @v11.8.9 
}

int SDirec::FindNext()
{
	// @v11.8.9 return (_wfindnexti64(lPosData, static_cast<FFIND_DATA *>(vPosData)) == 0) ? 1 : -1;
	if(vPosData) {
		return ::FindNextFileW(H, static_cast<WIN32_FIND_DATAW *>(vPosData)) ? 1 : -1;
	}
	else
		return -1;
}

/*static*/int SDirec::GetSingle(const char * pPath, SDirEntry * pEntry)
{
	int    ok = 0;
	if(!isempty(pPath)) {
		/* @v11.8.9 
		FFIND_DATA ff;
		intptr_t ffr = _wfindfirsti64(SUcSwitchW(pPath), &ff);
		if(ffr >= 0) {
			ASSIGN_PTR(pEntry, ff);
			_findclose(ffr);
			ok = 1;
		}*/
		// @v11.8.9 {
		SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = pPath).Strip().CopyToUnicode(r_temp_buf_u);
		WIN32_FIND_DATAW ff;
		HANDLE h = ::FindFirstFileExW(r_temp_buf_u, FindExInfoBasic, &ff, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
		if(!!h) {
			::FindClose(h);
			ASSIGN_PTR(pEntry, ff);
			ok = 1;
		}
		// } @v11.8.9 
	}
	return ok;
}

void SDirec::Close()
{
	/* @v11.8.9
	if(lPosData >= 0) {
		_findclose(lPosData);
		lPosData = -1;
	}
	*/
	// @v11.8.9 {
	if(!!H) {
		::FindClose(H);
		H.Z();
	}
	ZFREE(vPosData);
	InitFlag = 0;
	// } @v11.8.9
}

SDirec::~SDirec()
{
	Close();
}

int SDirec::Init(const char * pPathName, int enumDir)
{
	Close();
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
	// @v11.8.9 (Close()) SAlloc::F(vPosData);
	// @v11.8.9 vPosData = SAlloc::M(PosDataSize = sizeof(FFIND_DATA));
	PosDataSize = sizeof(WIN32_FIND_DATAW); // @v11.8.9 
	vPosData = SAlloc::M(PosDataSize); // @v11.8.9 
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
			CurEntry = *static_cast<const WIN32_FIND_DATAW *>(vPosData);
			InitFlag = -1;
		}
	} while(ok > 0 && (EnumDir && (CurEntry.Attr & 0x10L) == 0));
	if(ok > 0) {
		ASSIGN_PTR(pEntry, CurEntry);
	}
	else {
		InitFlag = 0;
		Close();
	}
	return ok;
}
//
//
//
SFileEntryPool::Entry::Entry() : SFile::Stat()
{
}

SFileEntryPool::Entry & SFileEntryPool::Entry::Z()
{
	SFile::Stat::Z();
	//Attr = 0;
	//CrtTime.Z();
	//AccsTime.Z();
	//ModTime.Z();
	//Size = 0;
	Name.Z();
	Path.Z();
	RemoteUrl.Z(); // @v10.8.2
	return *this;
}

SString & SFileEntryPool::Entry::GetFullPath(SString & rBuf) const
{
	return rBuf.Z().Cat(Path).SetLastSlash().Cat(Name);
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
		const SFsPath ps(pPath);
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
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
	const bool reckon_dirs = LOGIC(flags & scanfReckonDirs); // @v11.8.10
	//scanfReckonDirs
	for(SDirec sdirec(temp_buf, 0); sdirec.Next(&fb) > 0;) {
		// @debug {
		if(!sisascii(fb.Name, sstrlen(fb.Name)))
			non_ascii_count++;
		// } @debug 
		if(!fb.IsSelf() && !fb.IsUpFolder()) {
			if(fb.IsFile()) {
				THROW(Add(pPath, fb, flags));
				ok = 1;
			}
			else if(fb.IsFolder()) {
				if(do_recursive) {
					fb.GetNameUtf8(pPath, temp_buf);
					const int r = Helper_Scan(temp_buf, pWildcard, flags); // @recursion
					THROW(r);
					if(r > 0)
						ok = 1;
				}
				else if(reckon_dirs) {
					THROW(Add(pPath, fb, flags));
					ok = 1;
				}
			}
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
			const SFsPath ps(pPath);
			ps.Merge(~(SFsPath::fNam|SFsPath::fExt), r_temp_buf);
			SetInitPath(r_temp_buf.SetLastSlash());
			pWildcard = "*.*"; // @v11.8.10
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
				pEntry->GetFullPath(*pFullPath);
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

int SFileEntryPool::Add(const SFileEntryPool::Entry & rDe, long flags)
{
	int    ok = 1;
    InnerEntry new_entry;
    MEMSZERO(new_entry);
    new_entry.Attr = rDe.Attr;
    // @v11.8.11 new_entry.CrtTime = rDe.CrtTime;
    // @v11.8.11 new_entry.AccsTime = rDe.AccsTime;
    // @v11.8.11 new_entry.ModTime = rDe.ModTime;
	new_entry.CrtTm_ = rDe.CrtTm_; // @v11.8.11
	new_entry.AccsTm_ = rDe.AccsTm_; // @v11.8.11
	new_entry.ModTm_ = rDe.ModTm_; // @v11.8.11
    new_entry.Size = rDe.Size;
    {
		SString & r_temp_path = SLS.AcquireRvlStr();
		r_temp_path = rDe.Path;
		if(r_temp_path.NotEmptyS()) {
			const long npf = (flags & scanfKeepCase) ? SFsPath::npfKeepCase : 0;
			THROW(AddS(SFsPath::NormalizePath(rDe.Path, npf, r_temp_path), &new_entry.PathP));
		}
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
    // @v11.8.11 new_entry.CrtTime = rDe.CrtTime;
    // @v11.8.11 new_entry.AccsTime = rDe.AccsTime;
    // @v11.8.11 new_entry.ModTime = rDe.ModTime;
	new_entry.CrtTm_ = rDe.CrtTm_; // @v11.8.11
	new_entry.AccsTm_ = rDe.AccsTm_; // @v11.8.11
	new_entry.ModTm_ = rDe.ModTm_; // @v11.8.11
    new_entry.Size = rDe.Size;
	new_entry.ReparsePointTag = (rDe.Attr & SFile::attrReparsePoint) ? rDe.ReparsePointTag : 0; // @v11.8.10
    if(!isempty(pPath)) {
		SString & r_temp_path = SLS.AcquireRvlStr();
		const long npf = (flags & scanfKeepCase) ? SFsPath::npfKeepCase : 0;
		THROW(AddS(SFsPath::NormalizePath(pPath, npf, r_temp_path), &new_entry.PathP));
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
	// @v11.8.11 rEntry.CrtTime = rIEntry.CrtTime;
	// @v11.8.11 rEntry.AccsTime = rIEntry.AccsTime;
	// @v11.8.11 rEntry.ModTime = rIEntry.ModTime;
	rEntry.CrtTm_ = rIEntry.CrtTm_; // @v11.8.11
	rEntry.AccsTm_ = rIEntry.AccsTm_; // @v11.8.11
	rEntry.ModTm_ = rIEntry.ModTm_; // @v11.8.11
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
                si = p_blk->TempBuf1.CmpNC(p_blk->TempBuf2); // @todo @fixme Здесь должно быть сравнение utf8
    		}
			break;
    	case SFileEntryPool::scByExt:
    		{
    			SString ext1, ext2;
				p_blk->P_Pool->GetS(p_entry1->NameP, p_blk->TempBuf1);
				p_blk->P_Pool->GetS(p_entry2->NameP, p_blk->TempBuf2);
				SFsPath::GetExt(p_blk->TempBuf1, &ext1);
				SFsPath::GetExt(p_blk->TempBuf2, &ext2);
				si = ext1.CmpNC(ext2);
				SETIFZ(si, p_blk->TempBuf1.CmpNC(p_blk->TempBuf2)); // @todo @fixme Здесь должно быть сравнение utf8
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
				si = fp1.CmpNC(fp2); // @todo @fixme Здесь должно быть сравнение utf8
    		}
    		break;
    	// @v11.8.11 case SFileEntryPool::scByCrTime: si = cmp(p_entry1->CrtTime, p_entry2->CrtTime); break;
    	// @v11.8.11 case SFileEntryPool::scByAccsTime: si = cmp(p_entry1->AccsTime, p_entry2->AccsTime); break;
    	// @v11.8.11 case SFileEntryPool::scByWrTime: si = cmp(p_entry1->ModTime, p_entry2->ModTime); break;
    	case SFileEntryPool::scByCrTime: si = CMPSIGN(p_entry1->CrtTm_, p_entry2->CrtTm_); break; // @v11.8.11 
    	case SFileEntryPool::scByAccsTime: si = CMPSIGN(p_entry1->AccsTm_, p_entry2->AccsTm_); break; // @v11.8.11 
    	case SFileEntryPool::scByWrTime: si = CMPSIGN(p_entry1->ModTm_, p_entry2->ModTm_); break; // @v11.8.11 
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

bool SFileEntryPool::Search(const wchar_t * pName, uint * pIdx) const
{
	bool    ok = false;
	uint    idx = 0;
	SString temp_buf;
	SStringU temp_buf_u;
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const SFileEntryPool::InnerEntry & r_entry = L.at(i);
		GetS(r_entry.NameP, temp_buf);
		if(temp_buf_u.CopyFromUtf8(temp_buf)) {
			if(_wcsicmp(temp_buf_u, pName) == 0) {
				idx = i;
				ok = true;
			}
		}
	}
	ASSIGN_PTR(pIdx, idx);
	return ok;
}
//
//
//
SDirecDiffPool::Entry::Entry() : Kind(kEqual), P_RecurDir(0)
{
}
		
SDirecDiffPool::Entry::Entry(const Entry & rS) : Kind(kEqual), P_RecurDir(0)
{
	Copy(rS);
}
		
SDirecDiffPool::Entry::~Entry()
{
	delete P_RecurDir;
}
		
SDirecDiffPool::Entry & FASTCALL SDirecDiffPool::Entry::operator = (const Entry & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL SDirecDiffPool::Entry::Copy(const Entry & rS)
{
	int    ok = 1;
	Kind = rS.Kind;
	StatLeft = rS.StatLeft;
	StatRight = rS.StatRight;
	Name = rS.Name;
	if(rS.P_RecurDir) {
		SETIFZQ(P_RecurDir, new TSCollection <Entry>);
		if(!TSCollection_Copy(*P_RecurDir, *rS.P_RecurDir)) 
			ok = 0;
	}
	else {
		ZDELETE(P_RecurDir);
	}
	return ok;
}

SDirecDiffPool::SDirecDiffPool() : Result(0)
{
}
	
SDirecDiffPool::~SDirecDiffPool()
{
}

SDirecDiffPool & SDirecDiffPool::Z()
{
	Result = 0;
	DirLeft.Z();
	DirRight.Z();
	L.freeAll();
	return *this;
}

int SDirecDiffPool::MakeReport(StringSet & rSs) const
{
	rSs.Z();
	return Helper_MakeReport(L, 0, rSs);
}

int SDirecDiffPool::Helper_MakeReport(const TSCollection <Entry> & rL, uint indent, StringSet & rSs) const
{
	int    ok = 1;
	SString line_buf;
	SString temp_buf;
	if(indent == 0) {
		rSs.add(line_buf.Z().Tab(indent).Cat("left").CatDiv(':', 2).Cat(DirLeft));
		rSs.add(line_buf.Z().Tab(indent).Cat("right").CatDiv(':', 2).Cat(DirRight));
		rSs.add(line_buf.Z().Tab(indent).Cat("result").CatDiv(':', 2).Cat(Result));
		if(Result == 0) {
			; // @todo report error
		}
	}
	for(uint i = 0; i < rL.getCount(); i++) {
		const Entry * p_entry = rL.at(i);
		if(p_entry) {
			switch(p_entry->Kind) {
				case kEqual: temp_buf = "equal(unexpected-value)"; break;
				case kAbsenceLeft: temp_buf = "left-is-missing"; break;
				case kAbsenceRight: temp_buf = "right-is-missing"; break;
				case kDiffType: temp_buf = "element-types-are-different"; break;
				default: temp_buf.Z(); break;
			}
			//temp_buf.Z();
			if(p_entry->Kind & kDiffAttr) {
				temp_buf.CatDivIfNotEmpty(' ', 0).Cat("attributes-are-different").CatDiv(':', 2).
					CatHex(p_entry->StatLeft.Attr).Space().CatHex(p_entry->StatRight.Attr);
			}
			if(p_entry->Kind & kDiffSize) {
				temp_buf.CatDivIfNotEmpty(' ', 0).Cat("sizes-are-different").CatDiv(':', 2).Cat(p_entry->StatLeft.Size).Space().Cat(p_entry->StatRight.Size);
			}
			if(p_entry->Kind & kDiffModTime) {
				LDATETIME dtm_left;
				LDATETIME dtm_right;
				dtm_left.SetNs100(p_entry->StatLeft.ModTm_);
				dtm_right.SetNs100(p_entry->StatRight.ModTm_);
				temp_buf.CatDivIfNotEmpty(' ', 0).Cat("modtimestamps-are-different").CatDiv(':', 2).
					Cat(dtm_left, DATF_ISO8601CENT, 0).Space().Cat(dtm_right, DATF_ISO8601CENT, 0);
			}
			if(p_entry->Kind & kDiffSubDir) {
				temp_buf.CatDivIfNotEmpty(' ', 0).Cat("subdirs-are-different");
			}
			rSs.add(line_buf.Z().Tab(indent).Cat(p_entry->Name).CatDiv(':', 2).Cat(temp_buf));
			if(p_entry->P_RecurDir && p_entry->P_RecurDir->getCount()) {
				Helper_MakeReport(*p_entry->P_RecurDir, indent+1, rSs); // @recursion
			}
		}
	}
	return ok;
}

int SDirecDiffPool::Run(const char * pDirLeftUtf8, const char * pDirRightUtf8)
{
	Z();
	int    ok = 1;
	THROW_S_S(!isempty(pDirLeftUtf8), SLERR_INVPARAM, __FUNCTION__"/pDirLeftUtf8");
	THROW_S_S(!isempty(pDirRightUtf8), SLERR_INVPARAM, __FUNCTION__"/pDirRightUtf8");
	{
		SFsPath::NormalizePath(pDirLeftUtf8, SFsPath::npfCompensateDotDot, DirLeft);
		SFsPath::NormalizePath(pDirRightUtf8, SFsPath::npfCompensateDotDot, DirRight);
		if(DirLeft.IsEqiUtf8(DirRight))
			ok = 2;
		else
			ok = Helper_Compare(DirLeft, DirRight);
	}
	CATCHZOK
	Result = ok;
	return ok;
}

int SDirecDiffPool::Helper_Compare(const SString & rDirLeftUtf8, const SString & rDirRightUtf8)
{
	int    ok = 1;
	Entry * p_new_entry = 0;
	assert(!rDirLeftUtf8.IsEmpty());
	assert(!rDirRightUtf8.IsEmpty());
	assert(!rDirLeftUtf8.IsEqiUtf8(rDirRightUtf8));
	{
		SString temp_buf;
		SStringU temp_buf_u;
		SFileEntryPool pool_left;
		SFileEntryPool pool_right;
		if(pool_left.Scan(rDirLeftUtf8, 0, SFileEntryPool::scanfReckonDirs) > 0) {
			if(pool_right.Scan(rDirRightUtf8, 0, SFileEntryPool::scanfReckonDirs) > 0) {
				pool_left.Sort(SFileEntryPool::scByName);
				pool_right.Sort(SFileEntryPool::scByName);

				SFileEntryPool::Entry entry_left;
				SFileEntryPool::Entry entry_right;
				SString full_path_left;
				SString full_path_right;
				LongArray seen_ridx_list; // Список позиций в правом наборе, которые мы проверили на итерации по левому набору
				{
					for(uint lidx = 0; lidx < pool_left.GetCount(); lidx++) {
						const int lgr = pool_left.Get(lidx, &entry_left, &full_path_left);
						assert(lgr);
						uint ridx = 0;
						int  _k = kEqual;
						ZDELETE(p_new_entry);
						if(temp_buf_u.CopyFromUtf8(entry_left.Name) && pool_right.Search(temp_buf_u, &ridx)) {
							seen_ridx_list.add(static_cast<long>(ridx));
							const int rgr = pool_right.Get(ridx, &entry_right, &full_path_right);
							assert(rgr);
							const uint tattr_mask = (SFile::attrSubdir|SFile::attrReparsePoint); // Атрибуты, определяющие тип элемента
							const uint iattr_mask = (SFile::attrNotContentIndexed|SFile::attrOffline|SFile::attrNoScrubData|SFile::attrCompressed); // Игнорируемые при сравнении атрибуты
							if((entry_left.Attr & tattr_mask) != (entry_right.Attr & tattr_mask)) {
								_k = kDiffType;
							}
							else {
								if((entry_left.Attr & ~iattr_mask) != (entry_right.Attr & ~iattr_mask)) {
									_k |= kDiffAttr;
								}
								// @v11.8.12 {
								if(entry_left.Attr & SFile::attrReparsePoint) {
									assert(entry_right.Attr & SFile::attrReparsePoint); // Иначе бы мы не попали в этот блок кода
									SFile::Stat stat_left;
									SFile::Stat stat_right;
									SBinarySet set_left;
									SBinarySet set_right;
									const int gsrr = SFile::GetStat(full_path_left, 0, &stat_left, &set_left);
									const int gsrl = SFile::GetStat(full_path_right, 0, &stat_right, &set_right);
									if(gsrr && gsrl) {
										SBinaryChunk rt_left;
										SBinaryChunk rt_right;
										set_left.Get(SFile::Stat::sbiRaparseTag, &rt_left);
										set_right.Get(SFile::Stat::sbiRaparseTag, &rt_right);
										if(!rt_left.IsEq(rt_right)) {
											_k |= kDiffReparsePoint;
										}
									}
								}
								// } @v11.8.12 
								else if(entry_left.IsFolder()) {
									SDirecDiffPool inner;
									int inner_result = inner.Helper_Compare(full_path_left, full_path_right); // @recursion
									assert(inner_result != 2);
									THROW(inner_result);
									if(inner_result > 0) {
										;
									}
									else {
										SETIFZQ(p_new_entry, new Entry);
										p_new_entry->P_RecurDir = new TSCollection <Entry>();
										TSCollection_Copy(*p_new_entry->P_RecurDir, inner.L);
										_k |= kDiffSubDir;
									}
								}
								else {
									// Для каталогов время модификации и размер не сравниваем
									if(entry_left.ModTm_ != entry_right.ModTm_) {
										_k |= kDiffModTime;
									}
									if(entry_left.Size != entry_right.Size) {
										_k |= kDiffSize;
									}
								}
							}
						}
						else {
							entry_right.Z();
							_k = kAbsenceRight;
						}
						if(_k != kEqual) {
							SETIFZQ(p_new_entry, new Entry);
							p_new_entry->Kind = _k;
							p_new_entry->StatLeft = entry_left;
							p_new_entry->StatRight = entry_right;
							p_new_entry->Name = entry_left.Name;
							L.insert(p_new_entry);
							p_new_entry = 0; // @mandatory
							ok = -1;
						}
						else {
							assert(p_new_entry == 0); // Экземпляр создавался только если были различия. Тем менее, ниже, так я - параноик, разрушаю объект по указателю.
							ZDELETE(p_new_entry);
						}
					}
				}
				{
					seen_ridx_list.sortAndUndup();
					for(uint ridx = 0; ridx < pool_right.GetCount(); ridx++) {
						if(!seen_ridx_list.bsearch(ridx)) {
							const int rgr = pool_right.Get(ridx, &entry_right, &full_path_right);
							assert(rgr);
							uint lidx = 0;
							int  _k = kEqual;
							ZDELETE(p_new_entry);
							if(temp_buf_u.CopyFromUtf8(entry_right.Name) && pool_left.Search(temp_buf_u, &lidx)) {
								; // Если слева существует элемент с таким именем, то nothing-to-do: в цикле по левому дереву выше я проработал этот случай.
							}
							else {
								entry_left.Z();
								_k = kAbsenceLeft;
							}
							if(_k != kEqual) {
								SETIFZQ(p_new_entry, new Entry);
								p_new_entry->Kind = _k;
								p_new_entry->StatLeft = entry_left;
								p_new_entry->StatRight = entry_right;
								p_new_entry->Name = entry_right.Name;
								L.insert(p_new_entry);
								p_new_entry = 0; // @mandatory
								ok = -1;
							}
							else {
								assert(p_new_entry == 0); // Экземпляр создавался только если были различия. Тем менее, ниже, так я - параноик, разрушаю объект по указателю.
								ZDELETE(p_new_entry);
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_new_entry);
	return ok;
}
