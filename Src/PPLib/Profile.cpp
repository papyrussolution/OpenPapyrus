// PROFILE.CPP
// Copyright (c) A.Sobolev 1999-2002, 2003, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// Profile
//
/*
	uint16 FuncId;
	uint16 FuncVer;
	uint16 FactorCount;
	uint16 Flags;
*/
static const PPUserProfileFuncEntry PPUserProfileFuncTab[] = {
	{ PPUPRF_LOGIN,             0, 0, 0, 0 },
	{ PPUPRF_SESSION,           0, 0, 0, 0 },
	{ PPUPRF_BILLTURN_RCPT,     0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_EXP,      0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_IEXP,     0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_MOD,      0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_RVL,      0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_ORD,      0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_DRFT,     0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_RET,      0, 1, 0, 0 },
	{ PPUPRF_BILLTURN_ETC,      0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_RCPT,      0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_EXP,       0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_IEXP,      0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_MOD,       0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_RVL,       0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_ORD,       0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_DRFT,      0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_RET,       0, 1, 0, 0 },
	{ PPUPRF_BILLUPD_ETC,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_RCPT,      0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_EXP,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_IEXP,      0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_MOD,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_RVL,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_ORD,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_DRFT,      0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_RET,       0, 1, 0, 0 },
	{ PPUPRF_BILLRMV_ETC,       0, 1, 0, 0 },
	{ PPUPRF_VIEW_GREST,        0, 1, 0, 0 },
	{ PPUPRF_VIEW_GREST_DT,     0, 1, 0, 0 },
	{ PPUPRF_VATFREEPERSONLIST, 0, 1, 0, 0 },
	{ PPUPRF_PSALBLDGOODS,      0, 2, 0, 0 },
	{ PPUPRF_PSALBLDGOODSTEST,  0, 2, 0, 0 },
	{ PPUPRF_GOODSPUT,          0, 1, PPUserProfileFuncEntry::fAccumulate, 100 },
	{ PPUPRF_CCHECKPUT,         0, 3, PPUserProfileFuncEntry::fAccumulate, 1000 },
	{ PPUPRF_GETMTXLISTBYLOC,   0, 1, 0, 0 },
	{ PPUPRF_CSESSTXMRESTORE,   0, 1, 0, 0 },
	{ PPUPRF_PACKCACHETEXT,     0, 1, 0, 0 },
	{ PPUPRF_MTXCACHEACTUALIZE, 0, 2, PPUserProfileFuncEntry::fAccumulate, 100 },
	{ PPUPRF_SCARDUPDBYRULE,    0, 2, 0, 0 },
	{ PPUPRF_GETINVSTOCKREST,   0, 1, PPUserProfileFuncEntry::fAccumulate, 1000 },
	{ PPUPRF_VIEW_GOODSOPANLZ,  0, 1, 0, 0 },
	{ PPUPRF_VIEW_TRFRANLZ,     0, 1, 0, 0 },
	{ PPUPRF_INVENTAUTOBUILD,   0, 1, 0, 0 },
	{ PPUPRF_CALCARTDEBT,       0, 2, PPUserProfileFuncEntry::fAccumulate, 100 },
	{ PPUPRF_VIEW_DEBT,         0, 1, 0, 0 },
	{ PPUPRF_GETOPENEDLOTS,     0, 2, PPUserProfileFuncEntry::fAccumulate, 10000 },
	{ PPUPRF_BHTPREPBILL,       0, 1, 0, 0 }, // @v9.4.11
	{ PPUPRF_BHTPREPGOODS,      0, 1, 0, 0 }, // @v9.4.11
	{ PPUPRF_OSMXMLPARSETAG,    0, 1, PPUserProfileFuncEntry::fAccumulate, 1000000 },  // @v9.5.8
	{ PPUPRF_BUILDGOODSFL,      0, 1, 0, 0 },  // @v10.1.4
	{ PPUPRF_SRCHINGOODSFL,     0, 2, 0, 0 },  // @v10.1.4
	{ PPUPRF_TIMSERWRITE,       0, 1, PPUserProfileFuncEntry::fAccumulate, 100 }, // @v10.3.3
	{ PPUPRF_TSEVALSTAKES,      0, 1, PPUserProfileFuncEntry::fAccumulate, 600 }, // @v10.3.3
};

static const PPUserProfileFuncEntry * FASTCALL _GetUserProfileFuncEntry(int funcId)
{
	for(uint i = 0; i < SIZEOFARRAY(PPUserProfileFuncTab); i++) {
		const PPUserProfileFuncEntry & r_entry = PPUserProfileFuncTab[i];
		if(((int)r_entry.FuncId) == funcId) {
			return &r_entry;
		}
	}
	return 0;
}

//static
uint16 FASTCALL PPUserProfileFuncEntry::FromLoggedFuncId(long logFuncId, uint16 * pFuncVer)
{
	ASSIGN_PTR(pFuncVer, (uint16)(logFuncId%1000));
	return (uint16)(logFuncId/1000);
}

long SLAPI PPUserProfileFuncEntry::GetLoggedFuncId() const
{
	return (((long)FuncId)*1000)+((long)FuncVer);
}

struct ProfileEntry {
	SLAPI  ProfileEntry();
	// @nodestructor
	void   Destroy();

	uint32 Hash;
	ulong  LineNum;
	char * P_FileName;
	char * P_AddedInfo;
	int64  NSecs100;        // Время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	int64  StartEntryClock; // Время в промежутках по 100 нс начиная с полуночи 01/01/1601 GMT
	int64  StartEntryMks;   // Отметка начала выполнения кода в мкс
	int64  Mks;             // Полное время выполнения кода в мкс
	int64  Hits;            // @v7.9.3 long-->int64
	uint64 Iter;            // @v7.9.3 ulong-->uint64
};

static uint32 FASTCALL PeHashString(const char * pSymb)
{
	const size_t len = sstrlen(pSymb);
	return BobJencHash(pSymb, len);
}

SLAPI ProfileEntry::ProfileEntry() : P_FileName(0), P_AddedInfo(0)
{
	THISZERO();
}

void ProfileEntry::Destroy()
{
	ZFREE(P_FileName);
	ZFREE(P_AddedInfo);
}

SLAPI Profile::Profile(int singleThreaded) : SProfile(singleThreaded), SArray(sizeof(ProfileEntry), /*64,*/O_ARRAY|aryEachItem)
{
	/*
	StartClock = 0;
	EndClock = 0;
	LARGE_INTEGER cf;
	QueryPerformanceFrequency(&cf);
	ClockFrequency = cf.QuadPart;
	QueryPerformanceCounter(&cf);

	Gtb.PrevHrc   = 0;
	Gtb.StartHrc  = (int64)cf.QuadPart;
	Gtb.StartTick = GetTickCount();
	*/
}

SLAPI Profile::~Profile()
{
	freeAll();
}

//virtual
void FASTCALL Profile::freeItem(void * pItem) { ((ProfileEntry *)pItem)->Destroy(); }

ProfileEntry & FASTCALL Profile::at(uint p) const
{
	return *(ProfileEntry*)SArray::at(p);
}

IMPL_CMPFUNC(PrflEnKey, i1, i2)
{
	const  ProfileEntry * k1 = (const ProfileEntry*)i1;
	const  ProfileEntry * k2 = (const ProfileEntry*)i2;
	int    r = ((k1->Hash)>(k2->Hash)) ? 1 : (((k1->Hash)<(k2->Hash)) ? -1 : 0);
	if(r == 0)
		r = ((k1->LineNum)>(k2->LineNum)) ? 1 : (((k1->LineNum)<(k2->LineNum)) ? -1 : 0);
	return r;
}

void * FASTCALL Profile::Search(const char * pFileName, long lineNum) const
{
	uint pos = 0;
	ProfileEntry pe;
	pe.Hash    = PeHashString(pFileName);
	pe.LineNum = (ulong)lineNum;
	return bsearch(&pe, &pos, PTR_CMPFUNC(PrflEnKey)) ? &at(pos) : 0;
}

int SLAPI Profile::Insert(ProfileEntry * e, uint * pPos)
{
	return ordInsert(e, pPos, PTR_CMPFUNC(PrflEnKey)) ? 1 : PPSetErrorSLib();
}

int SLAPI Profile::Output(uint fileId, const char * pDescription)
{
	Lck.Lock();
	if(getCount()) { // @v8.1.6
		SETIFZ(fileId, PPFILNAM_PROFILE_LOG);
		char   empty_descr[16];
		SString temp_buf;
		if(pDescription == 0) {
			empty_descr[0] = 0;
			pDescription = empty_descr;
		}
		temp_buf.Printf("Start profile at clock %ld: %s", (long)(StartClock / 10000) , pDescription);
		PPLogMessage(fileId, temp_buf, /*LOGMSGF_USER|*/LOGMSGF_TIME|LOGMSGF_DIRECTOUTP); // @v9.2.0 LOGMSGF_DIRECTOUTP
		ProfileEntry * p_pe = 0;
		for(uint i = 0; enumItems(&i, (void**)&p_pe);) {
			double msh = p_pe->Hits ? (((double)p_pe->NSecs100) / ((double)p_pe->Hits * 10000.0)) : 0; // приведение к миллисекундам
			double msh_full = p_pe->Hits ? (((double)p_pe->Mks) / ((double)p_pe->Hits * 1000.0)) : 0; // приведение к миллисекундам
			uint32 stub = 0;
			const  char * p_added_info = NZOR(p_pe->P_AddedInfo, reinterpret_cast<const char *>(&stub));
			const  char * p_file_name  = NZOR(p_pe->P_FileName, reinterpret_cast<const char *>(&stub));
			temp_buf.Printf("%8I64d\t%8.0lf\t%12.4lf\t%8.0lf\t%12.4lf\t%s[%ld]\t%s",
				p_pe->Hits, (double)(p_pe->NSecs100 / 10000.0), msh,
				(double)(((double)p_pe->Mks) / 1000.0), msh_full,
				p_file_name, p_pe->LineNum, p_added_info);
			PPLogMessage(fileId, temp_buf, LOGMSGF_DIRECTOUTP); // @v9.2.0 LOGMSGF_DIRECTOUTP
		}
		temp_buf.Printf("End profile at clock %ld", (long)(EndClock / 10000));
		PPLogMessage(fileId, temp_buf, /*LOGMSGF_USER|*/LOGMSGF_TIME|LOGMSGF_DIRECTOUTP); // @v9.2.0 LOGMSGF_DIRECTOUTP
	}
	Lck.Unlock();
	return 1;
}

enum {
	rmvIter = 0,
	addIter = 1
};

int SLAPI Profile::AddEntry(const char * pFileName, long lineNum, int iterOp, const char * pAddedInfo)
{
	int64  finish = NSec100Clock();
	int64  finish_mks = Helper_GetAbsTimeMicroseconds();
	uint   pos = 0;
	ProfileEntry * p_entry = (ProfileEntry *)Search(pFileName, lineNum);
	if(p_entry) {
		int    do_zero_entry = 1;
		if(iterOp == addIter) {
			if(p_entry->Iter == 0) {
				do_zero_entry = 0;
				p_entry->StartEntryClock = 0; // Will be assigned at end of proc
				p_entry->StartEntryMks = 0;   // Will be assigned at end of proc
			}
			p_entry->Hits++;
			p_entry->Iter++;
		}
		else if(--p_entry->Iter == 0) {
			p_entry->NSecs100 += finish - p_entry->StartEntryClock;
			p_entry->Mks += finish_mks - p_entry->StartEntryMks;
			p_entry->StartEntryClock = 0;
			p_entry->StartEntryMks = 0;
		}
		if(do_zero_entry)
			p_entry = 0;
	}
	else {
		ProfileEntry pe;
		pe.P_FileName = newStr(pFileName);
		pe.Hash = PeHashString(pFileName);
		pe.P_AddedInfo = newStr(pAddedInfo);
		pe.LineNum = lineNum;
		pe.NSecs100 = 0;
		pe.Mks     = 0;
		pe.Hits    = 1;
		pe.Iter    = iterOp;
		pe.StartEntryClock = 0; // Will be assigned at end of proc
		pe.StartEntryMks = 0;   // Will be assigned at end of proc
		if(Insert(&pe, &pos))
			p_entry = &at(pos);
		else
			return 0;
	}
	if(p_entry) {
		p_entry->StartEntryClock = NSec100Clock();
		p_entry->StartEntryMks   = Helper_GetAbsTimeMicroseconds();
	}
	return 1;
}

void SLAPI Profile::Helper_Start(const char * pFileName, long lineNum, const char * pAddedInfo)
{
	SETIFZ(StartClock, NSec100Clock());
	AddEntry(pFileName, lineNum, addIter, pAddedInfo);
	EndClock = NSec100Clock();
}

void SLAPI Profile::Helper_Finish(const char * pFileName, long lineNum)
{
	EndClock = NSec100Clock();
	AddEntry(pFileName, lineNum, rmvIter);
}

void SLAPI Profile::Start(const char * pFileName, long lineNum, const char * pAddedInfo)
{
	Lck.Lock();
	Helper_Start(pFileName, lineNum, pAddedInfo);
	Lck.Unlock();
}

void SLAPI Profile::Finish(const char * pFileName, long lineNum)
{
	Lck.Lock();
	Helper_Finish(pFileName, lineNum);
	Lck.Unlock();
}

int SLAPI Profile::Start(uint logFileId, const char * pName, const char * pAddedInfo)
{
	int    ok = 1;
	uint32 stub = 0;
	SString temp_buff;
	ProfileEntry * p_pe = 0;
	const char * p_added_info, * p_name;
	//
	Lck.Lock();
	Helper_Start(pName, pAddedInfo ? sstrlen(pAddedInfo) : 0, pAddedInfo);
	p_pe = & at(0);
	p_name = NZOR(p_pe->P_FileName, reinterpret_cast<const char *>(&stub));
	p_added_info = NZOR(p_pe->P_AddedInfo, reinterpret_cast<const char *>(&stub));
	temp_buff.Printf("Start:\t%s {%s}", p_name, p_added_info);
	PPLogMessage(logFileId, temp_buff, LOGMSGF_TIME|LOGMSGF_USER);
	Lck.Unlock();
	return ok;
}

int SLAPI Profile::Finish(uint logFileId, const char * pName, const char * pAddedInfo)
{
	int    ok = 1;
	uint32 stub = 0;
	SString temp_buff;
	ProfileEntry *p_pe = 0;
	double msh, msh_full;
	const char * p_added_info, * p_name;
	//
	Lck.Lock();
	Helper_Finish(pName, pAddedInfo ? sstrlen(pAddedInfo) : 0);
	p_pe = & at(0);
	p_name  = NZOR(p_pe->P_FileName, reinterpret_cast<const char *>(&stub));
	p_added_info = NZOR(p_pe->P_AddedInfo, reinterpret_cast<const char *>(&stub));
	msh = p_pe->Hits ? (((double)p_pe->NSecs100) / ((int64)p_pe->Hits * 10000)) : 0;
	msh_full = p_pe->Hits ? (((double)p_pe->Mks) / ((int64)p_pe->Hits * 1000)) : 0;
	temp_buff.Printf("Finish:\t%s {%s}: %8I64d %12.4lf %12.4lf", p_name, p_added_info, p_pe->Hits, msh, msh_full);
	PPLogMessage(logFileId, temp_buff, LOGMSGF_TIME|LOGMSGF_USER);
	Lck.Unlock();
	return ok;
}
//
//
//
int SLAPI Profile::InitUserProfile(const char * pUserName)
{
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	SString temp_buf, fname, line_buf;
	const ThreadID thread_id = DS.GetConstTLA().GetThreadID();

	UPSB.LogFileName_Start.Z();
	UPSB.LogFileName_Finish.Z();
	UPSB.LogItemPrefix.Z();
	UPSB.SessUuid = SLS.GetSessUuid();
	if(p_dict) {
		p_dict->GetDbUUID(&UPSB.DbUuid);
		p_dict->GetDbSymb(UPSB.DbSymb);
		if(!isempty(pUserName)) {
			GetUserProfileFileName(fkSession, fname);
			if(fname.NotEmptyS()) {
				//
				// sessuuid;threadid;dbuuid;ver;dbsymb;username; machinename;macaddr; datetime
				//
				line_buf.Z();
				UPSB.SessUuid.ToStr(S_GUID::fmtIDL, temp_buf);
				line_buf.Cat(temp_buf);
				line_buf.Semicol().Cat(thread_id);
				UPSB.DbUuid.ToStr(S_GUID::fmtIDL, temp_buf);
				line_buf.Semicol().Cat(temp_buf);
				DS.GetVersion().ToStr(temp_buf);
				line_buf.Semicol().Cat(temp_buf);
				line_buf.Semicol().Cat(UPSB.DbSymb);
				line_buf.Semicol().Cat(pUserName);
				{
					SGetComputerName(temp_buf);
					line_buf.Semicol().Cat(temp_buf);
				}
				{
					MACAddr addr;
					if(GetFirstMACAddr(&addr))
						addr.ToStr(temp_buf);
					else
						temp_buf.Z();
					line_buf.Semicol().Cat(temp_buf);
				}
				line_buf.Semicol().Cat(getcurdatetime_(), DATF_DMY|DATF_CENTURY, TIMF_HMS|TIMF_MSEC);
				PPLogMessage(fname, line_buf, LOGMSGF_UNLIMITSIZE|LOGMSGF_NODUPFORJOB);
			}
		}
	}
	else {
		UPSB.DbUuid.Z();
		UPSB.DbSymb = "nologin";
	}
	{
		GetUserProfileFileName(fkStart, UPSB.LogFileName_Start);
		GetUserProfileFileName(fkFinish, UPSB.LogFileName_Finish);
	}
	{
		UPSB.SessUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		(UPSB.LogItemPrefix = temp_buf).Semicol().Cat(thread_id);
	}
	return ok;
}

uint FASTCALL Profile::StartUserProfileFunc(int funcId)
{
	uint   handle = 0;
	const  PPUserProfileFuncEntry * p_func_entry = _GetUserProfileFuncEntry(funcId);
	assert(p_func_entry);
	assert(UPSB.LogFileName_Start.NotEmpty());
	assert(UPSB.LogItemPrefix.NotEmpty());
	if(p_func_entry && UPSB.LogFileName_Start.NotEmpty() && UPSB.LogItemPrefix.NotEmpty()) {
		assert(p_func_entry->FuncId == funcId);
		int    do_log_start = 1;
		UserProfileEntry profile_entry;
		MEMSZERO(profile_entry);
		profile_entry.Fe = *p_func_entry;
		profile_entry.Seq = SLS.GetSequenceValue();
		profile_entry.StartDtm = getcurdatetime_();
		profile_entry.StartClock = GetAbsTimeMicroseconds();
		UserProfileStack.push(profile_entry);
		handle = UserProfileStack.getPointer();
		if(p_func_entry->Flags & PPUserProfileFuncEntry::fAccumulate) {
			int    found = 0;
			for(uint i = 0; i < UserProfileAccum.getCount(); i++) {
				UserProfileEntry & r_accum_entry = UserProfileAccum.at(i);
				if(r_accum_entry.Fe.FuncId == funcId) {
					if(r_accum_entry.Accum == 0)
						r_accum_entry = profile_entry;
					else
						do_log_start = 0;
					found = 1;
					break;
				}
			}
			if(!found) {
				UserProfileEntry accum_entry;
				accum_entry = profile_entry;
				UserProfileAccum.insert(&accum_entry);
			}
		}
		if(do_log_start) {
			SString line_buf;
			(line_buf = UPSB.LogItemPrefix).Semicol().Cat(profile_entry.Seq).Semicol().
				Cat(profile_entry.Fe.GetLoggedFuncId()).Semicol().Cat(profile_entry.StartDtm, DATF_DMY|DATF_CENTURY, TIMF_HMS|TIMF_MSEC);
			PPLogMessage(UPSB.LogFileName_Start, line_buf, LOGMSGF_UNLIMITSIZE|LOGMSGF_NODUPFORJOB);
		}
	}
	return handle;
}

int FASTCALL Profile::GetUserProfileFuncID(uint handle) const
{
	assert(handle > 0 && handle <= UserProfileStack.getCount());
	const UserProfileEntry * p_profile_entry = (const UserProfileEntry *)UserProfileStack.at(handle-1);
	return p_profile_entry->Fe.FuncId;
}

double SLAPI Profile::GetUserProfileFactor(uint handle, uint factorN) const
{
	assert(handle > 0 && handle <= UserProfileStack.getCount());
	const UserProfileEntry * p_profile_entry = (const UserProfileEntry *)UserProfileStack.at(handle-1);
	assert(factorN < p_profile_entry->Fe.FactorCount);
	return p_profile_entry->Factors[factorN];
}

int SLAPI Profile::SetUserProfileFactor(uint handle, uint factorN, double value)
{
	int    ok = 1;
	assert(handle > 0 && handle <= UserProfileStack.getCount());
	UserProfileEntry * p_profile_entry = (UserProfileEntry *)UserProfileStack.at(handle-1);
	assert(factorN < p_profile_entry->Fe.FactorCount);
	p_profile_entry->Factors[factorN] = value;
	return ok;
}

int FASTCALL Profile::FinishUserProfileFunc(uint handle)
{
	int    ok = 0;
	assert(handle > 0);
	assert(handle == UserProfileStack.getPointer());
	assert(UPSB.LogFileName_Finish.NotEmpty());
	assert(UPSB.LogItemPrefix.NotEmpty());
	if(handle == UserProfileStack.getPointer() && UPSB.LogFileName_Finish.NotEmpty() && UPSB.LogItemPrefix.NotEmpty()) {
		UserProfileEntry profile_entry;
		if(UserProfileStack.pop(profile_entry)) {
			uint64 end_clock = GetAbsTimeMicroseconds();
			const  int64 clock = end_clock - profile_entry.StartClock;
			int    do_log_finish = 1;
			UserProfileEntry * p_flash_accum_entry = 0;
			if(profile_entry.Fe.Flags & PPUserProfileFuncEntry::fAccumulate) {
				for(uint i = 0; i < UserProfileAccum.getCount(); i++) {
					UserProfileEntry & r_accum_entry = UserProfileAccum.at(i);
					if(r_accum_entry.Fe.FuncId == profile_entry.Fe.FuncId) {
						r_accum_entry.AccumClock += clock;
						r_accum_entry.Accum++;
						if(r_accum_entry.Fe.FactorCount > 1) {
							assert(r_accum_entry.Fe.FactorCount <= SIZEOFARRAY(profile_entry.Factors));
							for(uint i = 1; i < (uint)r_accum_entry.Fe.FactorCount; i++)
								r_accum_entry.Factors[i] += profile_entry.Factors[i];
						}
						if(r_accum_entry.Accum >= r_accum_entry.Fe.AccumLimit)
							p_flash_accum_entry = &r_accum_entry;
						else
							do_log_finish = 0;
						break;
					}
				}
			}
			if(do_log_finish) {
				SString line_buf;
				if(p_flash_accum_entry) {
					line_buf.Z().Cat(UPSB.LogItemPrefix).Semicol().Cat(p_flash_accum_entry->Seq).Semicol().
						Cat(profile_entry.Fe.GetLoggedFuncId()).Semicol().Cat(p_flash_accum_entry->AccumClock).Semicol();
					line_buf.Cat(p_flash_accum_entry->Accum);
					p_flash_accum_entry->Accum = 0;
					p_flash_accum_entry->AccumClock = 0;
					if(profile_entry.Fe.FactorCount > 1) {
						assert(profile_entry.Fe.FactorCount <= SIZEOFARRAY(profile_entry.Factors));
						for(uint i = 1; i < (uint)profile_entry.Fe.FactorCount; i++) {
							line_buf.CatChar(',').Cat(p_flash_accum_entry->Factors[i]);
							p_flash_accum_entry->Factors[i] = 0.0;
						}
					}
				}
				else {
					line_buf.Z().Cat(UPSB.LogItemPrefix).Semicol().Cat(profile_entry.Seq).Semicol().
						Cat(profile_entry.Fe.GetLoggedFuncId()).Semicol().Cat(clock).Semicol();
					if(profile_entry.Fe.FactorCount) {
						assert(profile_entry.Fe.FactorCount <= SIZEOFARRAY(profile_entry.Factors));
						for(uint i = 0; i < (uint)profile_entry.Fe.FactorCount; i++) {
							if(i)
								line_buf.CatChar(',');
							line_buf.Cat(profile_entry.Factors[i]);
						}
					}
				}
				PPLogMessage(UPSB.LogFileName_Finish, line_buf, LOGMSGF_UNLIMITSIZE|LOGMSGF_NODUPFORJOB);
			}
			ok = 1;
		}
	}
	return ok;
}

int SLAPI Profile::FlashUserProfileAccumEntries()
{
	int    ok = 1;
	if(UserProfileAccum.getCount()) {
		SString line_buf;
		assert(UPSB.LogFileName_Finish.NotEmpty());
		assert(UPSB.LogItemPrefix.NotEmpty());
		for(uint i = 0; i < UserProfileAccum.getCount(); i++) {
			UserProfileEntry & r_accum_entry = UserProfileAccum.at(i);
			if(r_accum_entry.Accum > 0) {
				line_buf.Z().Cat(UPSB.LogItemPrefix).Semicol().Cat(r_accum_entry.Seq).Semicol().
					Cat(r_accum_entry.Fe.GetLoggedFuncId()).Semicol().Cat(r_accum_entry.AccumClock).Semicol();
				line_buf.Cat(r_accum_entry.Accum);
				r_accum_entry.Accum = 0;
				r_accum_entry.AccumClock = 0;
				if(r_accum_entry.Fe.FactorCount > 1) {
					assert(r_accum_entry.Fe.FactorCount <= SIZEOFARRAY(r_accum_entry.Factors));
					for(uint i = 1; i < (uint)r_accum_entry.Fe.FactorCount; i++) {
						line_buf.CatChar(',').Cat(r_accum_entry.Factors[i]);
						r_accum_entry.Factors[i] = 0.0;
					}
				}
				PPLogMessage(UPSB.LogFileName_Finish, line_buf, LOGMSGF_UNLIMITSIZE|LOGMSGF_NODUPFORJOB);
			}
		}
	}
	return ok;
}

SString & SLAPI Profile::GetUserProfileFileName(int fk, SString & rBuf)
{
	rBuf.Z();
	if(oneof3(fk, fkSession, fkStart, fkFinish)) {
		SString fname, temp_buf;
		UPSB.DbUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		temp_buf.CatChar('_').Cat(UPSB.DbSymb).CatChar('_');
		fname = "up_";
		if(fk == fkSession)
			fname.Cat(temp_buf).Cat("sess");
		else if(fk == fkStart)
			fname.Cat(temp_buf).Cat("start");
		else if(fk == fkFinish)
			fname.Cat(temp_buf).Cat("finish");
		else {
			assert(0);
		}
		fname.CatChar('.').Cat("log");
		PPGetFilePath(PPPATH_LOG, fname, rBuf);
	}
	return rBuf;
}
//
//
//
//static
int SLAPI PPUserFuncProfiler::Init()
{
	return DS.GetTLA().Prf.InitUserProfile(0);
}

// static
SString & SLAPI PPUserFuncProfiler::GetFileName_(int fk, SString & rBuf)
{
 	return DS.GetTLA().Prf.GetUserProfileFileName(fk, rBuf);
}

SLAPI PPUserFuncProfiler::PPUserFuncProfiler(int funcId) : H(funcId ? DS.GetTLA().Prf.StartUserProfileFunc(funcId) : 0)
{
}

SLAPI PPUserFuncProfiler::~PPUserFuncProfiler()
{
	Commit();
}

int SLAPI PPUserFuncProfiler::operator !() const
{
	return (H == 0);
}

int SLAPI PPUserFuncProfiler::FlashAccumEntries()
{
	return DS.GetTLA().Prf.FlashUserProfileAccumEntries();
}

int FASTCALL PPUserFuncProfiler::Begin(int funcId)
{
	int    ok = 0;
	if(H) {
		ok = -1;
	}
	else {
		H = funcId ? DS.GetTLA().Prf.StartUserProfileFunc(funcId) : 0;
		ok = BIN(H);
	}
	return ok;
}

int SLAPI PPUserFuncProfiler::Commit()
{
	int    ok = -1;
	if(H) {
		ok = DS.GetTLA().Prf.FinishUserProfileFunc(H);
		H = 0;
	}
	return ok;
}

int SLAPI PPUserFuncProfiler::CommitAndRestart()
{
	int    ok = -1;
	if(H) {
		//int FASTCALL Profile::GetUserProfileFuncID(uint handle) const
		Profile & r_prf = DS.GetTLA().Prf;
		const int func_id = r_prf.GetUserProfileFuncID(H);
		ok = r_prf.FinishUserProfileFunc(H);
		if(ok) {
			H = func_id ? r_prf.StartUserProfileFunc(func_id) : 0;
			ok = BIN(H);
		}
		else
			H = 0;
	}
	return ok;
}

int SLAPI PPUserFuncProfiler::SetFactor(uint factorN, double value)
{
	return H ? DS.GetTLA().Prf.SetUserProfileFactor(H, factorN, value) : -1;
}

double SLAPI PPUserFuncProfiler::GetFactor(uint factorN) const
{
	return H ? DS.GetConstTLA().Prf.GetUserProfileFactor(H, factorN) : 0.0;
}
//
//
//
SLAPI PPUserProfileCore::StateItem::StateItem()
{
	Clear();
}

PPUserProfileCore::StateItem & SLAPI PPUserProfileCore::StateItem::Clear()
{
	DbID.Z();
	SessCrDtm.Z();
	StartCrDtm.Z();
	FinishCrDtm.Z();
	SessOffs = 0;
	StartOffs = 0;
	FinishOffs = 0;
	DbSymb.Z();
	return *this;
}

PPUserProfileCore::StateBlock::StateBlock() : SStrGroup(), Ver(DS.GetVersion())
{
}

PPUserProfileCore::StateBlock & PPUserProfileCore::StateBlock::Clear()
{
	L.clear();
	ClearS();
	return *this;
}

int PPUserProfileCore::StateBlock::SetItem(StateItem & rItem)
{
	int    ok = 0;
	uint   p = 0;
	if(L.lsearch(&rItem.DbID, &p, PTR_CMPFUNC(S_GUID))) {
		StateItem_Internal & r_int_item = L.at(p);
		r_int_item.SessCrDtm = rItem.SessCrDtm;
		r_int_item.StartCrDtm = rItem.StartCrDtm;
		r_int_item.FinishCrDtm = rItem.FinishCrDtm;
		r_int_item.SessOffs = rItem.SessOffs;
		r_int_item.StartOffs = rItem.StartOffs;
		r_int_item.FinishOffs = rItem.FinishOffs;
		{
			SString ex_db_symb;
			GetS(r_int_item.DbSymbP, ex_db_symb);
			if(ex_db_symb != rItem.DbSymb) {
				AddS(rItem.DbSymb, &r_int_item.DbSymbP);
			}
		}
		ok = 2;
	}
	else {
		StateItem_Internal int_item;
		MEMSZERO(int_item);
		int_item.DbID = rItem.DbID;
		int_item.SessCrDtm = rItem.SessCrDtm;
		int_item.StartCrDtm = rItem.StartCrDtm;
		int_item.FinishCrDtm = rItem.FinishCrDtm;
		int_item.SessOffs = rItem.SessOffs;
		int_item.StartOffs = rItem.StartOffs;
		int_item.FinishOffs = rItem.FinishOffs;
		int_item.DbSymbP = 0;
		AddS(rItem.DbSymb, &int_item.DbSymbP);
		L.insert(&int_item);
		ok = 1;
	}
	return ok;
}

int PPUserProfileCore::StateBlock::RemoveItem(const S_GUID & rDbId)
{
	int    ok = -1;
	uint   p = 0;
	if(L.lsearch(&rDbId, &p, PTR_CMPFUNC(S_GUID))) {
		L.atFree(p);
		ok = 1;
	}
	return ok;
}

int PPUserProfileCore::StateBlock::Helper_GetItem(uint pos, StateItem & rItem) const
{
	int    ok = 0;
	if(pos < L.getCount()) {
		const StateItem_Internal & r_int_item = L.at(pos);
		rItem.DbID = r_int_item.DbID;
		rItem.SessCrDtm = r_int_item.SessCrDtm;
		rItem.StartCrDtm = r_int_item.StartCrDtm;
		rItem.FinishCrDtm = r_int_item.FinishCrDtm;
		rItem.SessOffs = r_int_item.SessOffs;
		rItem.StartOffs = r_int_item.StartOffs;
		rItem.FinishOffs = r_int_item.FinishOffs;
		GetS(r_int_item.DbSymbP, rItem.DbSymb);
		ok = 1;
	}
	return ok;
}

int PPUserProfileCore::StateBlock::GetItem(const S_GUID & rDbId, StateItem & rItem) const
{
	uint   p = 0;
	return L.lsearch(&rDbId, &p, PTR_CMPFUNC(S_GUID)) ? Helper_GetItem(p, rItem) : 0;
}

int PPUserProfileCore::StateBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint32 c = 0;
	SString temp_buf;
	if(dir > 0) {
		c = L.getCount();
		Ver = DS.GetVersion();
		THROW_SL(Ver.Serialize(dir, rBuf, pCtx));
		THROW(SStrGroup::SerializeS(dir, rBuf, pCtx));
		THROW_SL(pCtx->Serialize(dir, c, rBuf));
		for(uint i = 0; i < c; i++) {
			StateItem_Internal & r_int_item = L.at(i);
			THROW_SL(pCtx->Serialize(dir, r_int_item.DbID, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.SessCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.StartCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.FinishCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.SessOffs, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.StartOffs, rBuf));
			THROW_SL(pCtx->Serialize(dir, r_int_item.FinishOffs, rBuf));
			GetS(r_int_item.DbSymbP, temp_buf.Z());
			THROW_SL(pCtx->Serialize(dir, temp_buf, rBuf));
		}
	}
	else if(dir < 0) {
		Clear();
		THROW_SL(Ver.Serialize(dir, rBuf, pCtx));
		THROW(SStrGroup::SerializeS(dir, rBuf, pCtx));
		THROW_SL(pCtx->Serialize(dir, c, rBuf));
		for(uint i = 0; i < c; i++) {
			StateItem_Internal int_item;
			THROW_SL(pCtx->Serialize(dir, int_item.DbID, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.SessCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.StartCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.FinishCrDtm, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.SessOffs, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.StartOffs, rBuf));
			THROW_SL(pCtx->Serialize(dir, int_item.FinishOffs, rBuf));
			THROW_SL(pCtx->Serialize(dir, temp_buf, rBuf));
			AddS(temp_buf, &int_item.DbSymbP);
			THROW_SL(L.insert(&int_item));
		}
	}
	CATCHZOK
	return ok;
}

SLAPI PPUserProfileCore::PPUserProfileCore() : UserFuncPrfTbl()
{
}

SLAPI PPUserProfileCore::~PPUserProfileCore()
{
}

int SLAPI PPUserProfileCore::WriteState(int use_ta)
{
	int    ok = 1;
	SSerializeContext sctx;
	SBuffer buf;
	THROW(StB.Serialize(+1, buf, &sctx));
	THROW(PPRef->PutPropSBuffer(PPOBJ_USERPROFILE, 1L, USERPROFILEPPRP_FILEINFO2, buf, use_ta));
	CATCHZOK
	return ok;
}

int SLAPI PPUserProfileCore::ReadState()
{
	int    ok = 1;
	SSerializeContext sctx;
	SBuffer buf;
	THROW(PPRef->GetPropSBuffer(PPOBJ_USERPROFILE, 1L, USERPROFILEPPRP_FILEINFO2, buf));
	if(buf.GetAvailableSize()) {
		THROW(StB.Serialize(-1, buf, &sctx));
	}
	else {
		StB.Clear();
		ok = -1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPUserProfileCore::ClearState(const S_GUID * pDbId, int use_ta)
{
	int    ok = 1;
	SSerializeContext sctx;
	SBuffer buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pDbId) {
			StateBlock stb;
			THROW(PPRef->GetPropSBuffer(PPOBJ_USERPROFILE, 1L, USERPROFILEPPRP_FILEINFO2, buf));
			if(buf.GetAvailableSize()) {
				THROW(stb.Serialize(-1, buf, &sctx));
				if(stb.RemoveItem(*pDbId) > 0) {
					THROW(stb.Serialize(+1, buf.Z(), &sctx));
					THROW(PPRef->PutPropSBuffer(PPOBJ_USERPROFILE, 1L, USERPROFILEPPRP_FILEINFO2, buf, use_ta));
				}
			}
		}
		else {
			THROW(PPRef->PutProp(PPOBJ_USERPROFILE, 1L, USERPROFILEPPRP_FILEINFO2, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPUserProfileCore::GetDbEntyList(TSArray <PPUserProfileCore::UfpDbEntry> & rList)
{
	rList.clear();

	int    ok = -1;
	StateItem stb_item;
	THROW(ReadState());
	for(uint i = 0; i < StB.GetCount(); i++) {
		if(StB.GetItem(i, stb_item)) {
			UfpDbEntry new_entry;
			new_entry.DbID = stb_item.DbID;
			STRNSCPY(new_entry.DbSymb, stb_item.DbSymb);
			THROW_SL(rList.insert(&new_entry));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI ParseUfpFileName(const char * pFileName, S_GUID & rDbUuid, SString & rDbSymb)
{
	rDbSymb.Z();

	int    kind = 0;
	SString temp_buf;
	StringSet ss('_', pFileName);
	uint p = 0;
	if(ss.get(&p, temp_buf) && temp_buf.CmpNC("up") == 0) {
		if(ss.get(&p, temp_buf) && rDbUuid.FromStr(temp_buf)) {
			while(!kind && ss.get(&p, temp_buf)) {
				if(temp_buf.CmpNC("sess.log") == 0) {
					kind = Profile::fkSession;
				}
				else if(temp_buf.CmpNC("start.log") == 0) {
					kind = Profile::fkStart;
				}
				else if(temp_buf.CmpNC("finish.log") == 0) {
					kind = Profile::fkFinish;
				}
				else {
					rDbSymb.CatDivIfNotEmpty('_', 0).Cat(temp_buf);
				}
			}
		}
	}
	return kind;
}

struct UfpFileSet {
	S_GUID DbUuid;
	SString DbSymb;
	StrAssocArray Set;
};

struct UserProfileLoadCacheFinishEntry { // @flat
	long   SessID;
	int64  SeqID;
	long   FuncID;
	LDATETIME Dtm;
	int64  Clock;
	long   Flags;
	double Factor1;
	double Factor2;
	double Factor3;
};

PPUserProfileCore::UfpLine::UfpLine()
{
	Clear();
}

PPUserProfileCore::UfpLine & PPUserProfileCore::UfpLine::Clear()
{
	Kind = 0;
	memzero(this, offsetof(UfpLine, DbSymb));
	DbSymb = 0;
	UserName = 0;
	MachineName = 0;
	return *this;
}

//static
int SLAPI PPUserProfileCore::ParseUfpLine(StringSet & rSs, SString & rTempBuf, int kind, PPUserProfileCore::UfpLine & rItem)
{
	assert(oneof3(kind, Profile::fkSession, Profile::fkStart, Profile::fkFinish));
	int    ok = 1;
	uint   p = 0;
	rItem.Clear();
	THROW(rSs.get(&p, rTempBuf));
	THROW(rItem.SessUuid.FromStr(rTempBuf));
	THROW(rSs.get(&p, rTempBuf));
	rItem.ThreadId = rTempBuf.ToLong();
	rItem.Kind = kind;
	if(kind == Profile::fkSession) {
		THROW(rSs.get(&p, rTempBuf));
		THROW(rItem.DbUuid.FromStr(rTempBuf));
		THROW(rSs.get(&p, rTempBuf));
		THROW(rItem.Ver.FromStr(rTempBuf));
		THROW(rSs.get(&p, rItem.DbSymb));
		rItem.DbSymb.Transf(CTRANSF_OUTER_TO_INNER); // @v8.1.2
		THROW(rSs.get(&p, rItem.UserName));
		rItem.UserName.Transf(CTRANSF_OUTER_TO_INNER); // @v8.1.2
		THROW(rSs.get(&p, rItem.MachineName));
		THROW(rSs.get(&p, rTempBuf)); // MAC address
		THROW(rSs.get(&p, rTempBuf));
		strtodatetime(rTempBuf, &rItem.Start, DATF_DMY, TIMF_HMS|TIMF_MSEC);
	}
	else if(kind == Profile::fkStart) {
		THROW(rSs.get(&p, rTempBuf));
		rItem.Seq = rTempBuf.ToInt64();
		THROW(rSs.get(&p, rTempBuf));
		rItem.FuncId = rTempBuf.ToLong();
		THROW(rSs.get(&p, rTempBuf));
		strtodatetime(rTempBuf, &rItem.Start, DATF_DMY, TIMF_HMS|TIMF_MSEC);
	}
	else if(kind == Profile::fkFinish) {
		THROW(rSs.get(&p, rTempBuf));
		rItem.Seq = rTempBuf.ToInt64();
		THROW(rSs.get(&p, rTempBuf));
		rItem.FuncId = rTempBuf.ToLong();
		THROW(rSs.get(&p, rTempBuf));
		rItem.Clock = rTempBuf.ToInt64();
		if(rSs.get(&p, rTempBuf)) {
			if(rTempBuf.HasChr(',')) {
				StringSet _ss(',', rTempBuf);
				p = 0;
				for(uint i = 0; i < SIZEOFARRAY(rItem.Factors) && _ss.get(&p, rTempBuf); i++) {
					rItem.Factors[i] = rTempBuf.ToReal();
				}
			}
			else
				rItem.Factors[0] = rTempBuf.ToReal();
		}
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}

#define AGGR_REC        -100
#define DB_REC          -1000
#define PREFIX_AGGR_REC "AGGR"

int SLAPI PPUserProfileCore::SetupSessItem(long * pSessID, const UfpLine & rLine, long funcId /*=0*/)
{
	int    ok = 1;
	long   sess_id = 0;
	SString temp_buf, sess_uuid;
	UserFuncPrfSessTbl::Key1 k1;

	MEMSZERO(k1);
	if(funcId) {
		long db_id = 0L;

		k1.ThreadId = DB_REC;
		rLine.DbUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		temp_buf.CopyTo(k1.SessUUID_s, sizeof(k1.SessUUID_s));
		if(UfpSessT.search(1, &k1, spEq))
			db_id = UfpSessT.data.ID;
		else {
			UserFuncPrfSessTbl::Key0 k0;
			UserFuncPrfSessTbl::Rec rec;
			MEMSZERO(rec);
			temp_buf.CopyTo(rec.SessUUID_s, sizeof(rec.SessUUID_s));
			rec.ThreadId = DB_REC;
			UfpSessT.copyBufFrom(&rec);
			THROW_DB(UfpSessT.insertRec(0, &k0));
			db_id = k0.ID;
		}
		(sess_uuid = PREFIX_AGGR_REC).Cat(funcId).Cat(db_id).Cat(rLine.Ver);
	}
	else {
		rLine.SessUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		sess_uuid = temp_buf;
	}
	MEMSZERO(k1);
	sess_uuid.CopyTo(k1.SessUUID_s, sizeof(k1.SessUUID_s));
	k1.ThreadId = rLine.ThreadId;
	if(UfpSessT.search(1, &k1, spEq)) {
		sess_id = UfpSessT.data.ID;
	}
	else {
		UserFuncPrfSessTbl::Key0 k0;
		UserFuncPrfSessTbl::Rec rec;
		MEMSZERO(rec);
		temp_buf.CopyTo(rec.SessUUID_s, sizeof(rec.SessUUID_s));
		rLine.DbUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		temp_buf.CopyTo(rec.DbUUID_s, sizeof(rec.DbUUID_s));
		rec.ThreadId = rLine.ThreadId;
		rec.Dt = rLine.Start.d;
		rec.Tm = rLine.Start.t;
		rec.Ver = rLine.Ver;
		rec.Flags = 0;
		rLine.DbSymb.CopyTo(rec.DbName, sizeof(rec.DbName));
		rLine.UserName.CopyTo(rec.UserName, sizeof(rec.UserName));
		UfpSessT.copyBufFrom(&rec);
		THROW_DB(UfpSessT.insertRec(0, &k0));
		sess_id = k0.ID;
	}
	{
		//
		// Setup DbID
		//
	}
	CATCH
		ok = 0;
		sess_id = 0;
	ENDCATCH
	ASSIGN_PTR(pSessID, sess_id);
	return ok;
}

struct UserProfileLoadCacheStartEntry { // @flat
	long   SessID;
	int64  SeqID;
	long   FuncID;
	LDATETIME Dtm;
	long   Flags;
};

IMPL_CMPFUNC(UserProfileLoadCacheEntry, i1, i2) { RET_CMPCASCADE2((const UserProfileLoadCacheStartEntry *)i1, (const UserProfileLoadCacheStartEntry *)i2, SessID, SeqID); }

int SLAPI PPUserProfileCore::OpenInputFile(const char * pFileName, int64 offset, SFile & rF)
{
	int    ok = 1;
	THROW_SL(rF.Open(pFileName, SFile::mRead));
	THROW_SL(rF.Seek64(offset));
	CATCHZOK
	return ok;
}

int SLAPI PPUserProfileCore::AddAggrRecs(BExtInsert * pBei, const UserFuncPrfTbl::Rec & rRec, const UfpLine & rLine)
{
	int ok = 1;
	/* Не закончено
	SString avg_ids;
	UfpLine ufp_line = rLine;
	UserFuncPrfTbl::Rec rec;
	TSArray <UfpLine> aggr_sess_list;
	uint16 func_ver = 0;
	long func_id = (long)PPUserProfileFuncEntry::FromLoggedFuncId(rRec.FuncID, &func_ver)

	PPLoadText(PPTXT_UFP_AVG_IDS, avg_ids);
	MEMSZERO(rec);
	rec.SeqID      = -1;
	rec.FuncID     = func_id;
	rec.Dt         = rRec.Dt;
	rec.Tm         = rRec.Tm;
	rec.Clock      = rRec.Clock;
	rec.Flags      = rRec.Flags;
	rec.Factor1    = rRec.Factor1;
	rec.Factor2    = rRec.Factor2;
	rec.Factor3    = rRec.Factor3;
	rec.SqSum1     = rRec.Factor1 * rRec.Factor1;
	rec.SqSum2     = rRec.Factor2 * rRec.Factor2;
	rec.SqSum3     = rRec.Factor3 * rRec.Factor3;
	rec.SqClockSum = rRec.Clock   * rRec.Clock;


	ufp_line.ThreadId = AGGR_REC;
	ufp_line.Dt       = ZERODATE;
	ufp_line.Tm       = ZEROTIME;
	ufp_line.insert(&ufp_line);

	THROW(SetupSessItem(&rec.SessID, ufp_line, func_id));
	{
		{
			UserFuncPrfTbl::Key0 k0;
			k0.SessID = rec.SessID;
			k0.SeqID  = rec.SeqID;
			if(search(0, &k0, spEq)) {
				data.Clock      += rec.Clock;
				data.Factor1    += rec.Factor1;
				data.Factor2    += rec.Factor2;
				data.Factor3    += rec.Factor3;
				data.SqSum1     += rec.SqSum1;
				data.SqSum2     += rec.SqSum2;
				data.SqSum3     += rec.SqSum3;
				data.SqClockSum += rec.SqClockSum;
				THROW_DB(updateRec());
			}
			else {
				THROW_DB(pBei->insert(&rec));
			}
		}
	}
	CATCHZOK
	*/
	return ok;
}

int SLAPI PPUserProfileCore::Load(const char * pPath)
{
	int    ok = -1;
	UfpLine ufp_line;
	SString path, dbsymb, temp_buf, file_name, line_buf, msg_buf;
	TSCollection <UfpFileSet> file_set_list;
	TSVector <UserProfileLoadCacheStartEntry> start_list; // @v9.8.6 TSArray-->TSVector
	TSVector <UserProfileLoadCacheFinishEntry> finish_list; // @v9.8.6 TSArray-->TSVector
	TSVector <UserProfileLoadCacheFinishEntry> average_list; // @v9.8.6 TSArray-->TSVector
	if(!isempty(pPath)) {
		path = pPath;
	}
	else {
		PPGetPath(PPPATH_LOG, path);
	}
	path.Strip().SetLastSlash();
	{
		SDirEntry sde;
		SDirec sd;
		for(sd.Init((temp_buf = path).Cat("up_*.log")); sd.Next(&sde) > 0;) {
			S_GUID db_uuid;
			int kind = ParseUfpFileName(sde.FileName, db_uuid, dbsymb);
			if(kind) {
				uint   fp = 0;
				for(uint i = 0; i < file_set_list.getCount(); i++) {
					UfpFileSet * p_set = file_set_list.at(i);
					if(p_set && p_set->DbUuid == db_uuid) {
						assert(fp == 0);
						int    dup_fault = p_set->Set.Search(kind);
						// @v8.3.3 assert(!dup_fault);
						if(!dup_fault) {
							p_set->Set.AddFast(kind, sde.FileName);
							ok = 1;
						}
						fp = i+1;
					}
				}
				if(!fp) {
					UfpFileSet * p_new_set = file_set_list.CreateNewItem();
					THROW_SL(p_new_set);
					p_new_set->DbUuid = db_uuid;
					p_new_set->DbSymb = dbsymb;
					p_new_set->Set.AddFast(kind, sde.FileName);
					ok = 1;
				}
			}
		}
	}
	//
	//
	//
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(ReadState());
		for(uint i = 0; i < file_set_list.getCount(); i++) {
			long   line_count_sess = 0;
			long   line_count_start = 0;
			long   line_count_finish = 0;
			long   line_no = 0;
			LDATETIME crtm;
			StateItem sti;
			//int64  foffs_sess = 0;
			//int64  foffs_start = 0;
			//int64  foffs_finish = 0;
			const  UfpFileSet * p_set = file_set_list.at(i);
			if(!StB.GetItem(p_set->DbUuid, sti)) {
				sti.Clear();
				sti.DbID = p_set->DbUuid;
			}
			if(sti.DbSymb != p_set->DbSymb) {
				sti.DbSymb = p_set->DbSymb;
			}
			PPLoadText(PPTXT_WAIT_UFPLOADPREPROC, msg_buf);
			PPWaitMsg(msg_buf.Space().Cat(p_set->DbSymb).Space().Cat(p_set->DbUuid.ToStr(S_GUID::fmtIDL, temp_buf)));
			if(p_set->Set.GetText(Profile::fkSession, file_name)) {
				(temp_buf = path).Cat(file_name);
				SFile _f;
				THROW_SL(SFile::GetTime(temp_buf, &crtm, 0, 0));
				if(sti.SessCrDtm != crtm) {
					sti.SessCrDtm = crtm;
					sti.SessOffs = 0;
				}
				THROW(OpenInputFile(temp_buf, sti.SessOffs, _f));
				while(_f.ReadLine(line_buf))
					line_count_sess++;
			}
			if(p_set->Set.GetText(Profile::fkFinish, file_name)) {
				(temp_buf = path).Cat(file_name);
				SFile _f;
				THROW_SL(SFile::GetTime(temp_buf, &crtm, 0, 0));
				if(sti.FinishCrDtm != crtm) {
					sti.FinishCrDtm = crtm;
					sti.FinishOffs = 0;
				}
				THROW(OpenInputFile(temp_buf, sti.FinishOffs, _f));
				while(_f.ReadLine(line_buf))
					line_count_finish++;
			}
			if(p_set->Set.GetText(Profile::fkStart, file_name)) {
				(temp_buf = path).Cat(file_name);
				SFile _f;
				THROW_SL(SFile::GetTime(temp_buf, &crtm, 0, 0));
				if(sti.StartCrDtm != crtm) {
					sti.StartCrDtm = crtm;
					sti.StartOffs = 0;
				}
				THROW(OpenInputFile(temp_buf, sti.StartOffs, _f));
				while(_f.ReadLine(line_buf))
					line_count_start++;
			}
			{
				IterCounter cntr;
				cntr.Init(line_count_sess + line_count_start + line_count_finish);
				PPLoadText(PPTXT_WAIT_UFPLOADINPUT, msg_buf);
				msg_buf.Space().Cat(p_set->DbSymb).Space().Cat(p_set->DbUuid.ToStr(S_GUID::fmtIDL, temp_buf));

				StringSet ss(";");
				if(p_set->Set.GetText(Profile::fkSession, file_name)) {
					SFile _f;
					THROW(OpenInputFile((temp_buf = path).Cat(file_name), sti.SessOffs, _f));
					for(line_no = 0; line_no < line_count_sess && _f.ReadLine(line_buf); line_no++) {
						ss.setBuf(line_buf, line_buf.Len()+1);
						if(ParseUfpLine(ss, temp_buf, Profile::fkSession, ufp_line)) {
							long   sess_id = 0;
							THROW(SetupSessItem(&sess_id, ufp_line));
						}
						PPWaitPercent(cntr.Increment(), msg_buf);
					}
					sti.SessOffs = _f.Tell64();
				}
				{
					start_list.clear();
					finish_list.clear();
					if(p_set->Set.GetText(Profile::fkStart, file_name)) {
						S_GUID db_uuid;
						int    kind = ParseUfpFileName(file_name, db_uuid, dbsymb);
						assert(kind == Profile::fkStart);

						SFile _f;
						THROW(OpenInputFile((temp_buf = path).Cat(file_name), sti.StartOffs, _f));
						for(line_no = 0; line_no < line_count_start && _f.ReadLine(line_buf); line_no++) {
							ss.setBuf(line_buf, line_buf.Len()+1);
							if(ParseUfpLine(ss, temp_buf, Profile::fkStart, ufp_line)) {
								ufp_line.DbUuid = db_uuid;
								ufp_line.DbSymb = dbsymb;
								UserProfileLoadCacheStartEntry entry;
								MEMSZERO(entry);
								THROW(SetupSessItem(&entry.SessID, ufp_line));
								entry.SeqID = ufp_line.Seq;
								entry.FuncID = ufp_line.FuncId;
								entry.Dtm = ufp_line.Start;
								THROW_SL(start_list.insert(&entry));
							}
							PPWaitPercent(cntr.Increment(), msg_buf);
						}
						sti.StartOffs = _f.Tell64();
						start_list.sort(PTR_CMPFUNC(UserProfileLoadCacheEntry));
					}
					if(p_set->Set.GetText(Profile::fkFinish, file_name)) {
						S_GUID db_uuid;
						int    kind = ParseUfpFileName(file_name, db_uuid, dbsymb);
						assert(kind == Profile::fkFinish);

						SFile _f;
						THROW(OpenInputFile((temp_buf = path).Cat(file_name), sti.FinishOffs, _f));
						for(line_no = 0; line_no < line_count_finish && _f.ReadLine(line_buf); line_no++) {
							ss.setBuf(line_buf, line_buf.Len()+1);
							if(ParseUfpLine(ss, temp_buf, Profile::fkFinish, ufp_line)) {
								ufp_line.DbUuid = db_uuid;
								ufp_line.DbSymb = dbsymb;
								uint   spos = 0;
								UserProfileLoadCacheFinishEntry entry;
								MEMSZERO(entry);
								THROW(SetupSessItem(&entry.SessID, ufp_line));
								entry.SeqID = ufp_line.Seq;
								entry.FuncID = ufp_line.FuncId;
								entry.Dtm = ufp_line.Start;
								entry.Clock = ufp_line.Clock;
								entry.Factor1 = ufp_line.Factors[0];
								entry.Factor2 = ufp_line.Factors[1];
								entry.Factor3 = ufp_line.Factors[2];
								if(start_list.bsearch(&entry, &spos, PTR_CMPFUNC(UserProfileLoadCacheEntry))) {
									UserProfileLoadCacheStartEntry & r_start_entry = start_list.at(spos);
									if(r_start_entry.FuncID == entry.FuncID) {
										entry.Dtm = r_start_entry.Dtm;
										r_start_entry.Flags |= USRPROFF_FINISHED;
										entry.Flags = USRPROFF_FINISHED;
										THROW_SL(finish_list.insert(&entry));
									}
								}
								else {
									//
									// Соответствующая START-запись могла быть занесена ранее
									// (в одном из предыдущих сеансов обновления данных)
									//
									entry.Flags = USRPROFF_FINISHED;
									THROW_SL(finish_list.insert(&entry));
								}
							}
							PPWaitPercent(cntr.Increment(), msg_buf);
						}
						sti.FinishOffs = _f.Tell64();
					}
					{
						BExtInsert bei(this);
						{
							PPLoadText(PPTXT_WAIT_UFPSTOREFINISH, msg_buf);
							msg_buf.Space().Cat(p_set->DbSymb).Space().Cat(p_set->DbUuid.ToStr(S_GUID::fmtIDL, temp_buf));
							const uint flc = finish_list.getCount();
							for(uint j = 0; j < flc; j++) {
								UserProfileLoadCacheFinishEntry & r_entry = finish_list.at(j);
								UserFuncPrfTbl::Key0 k0;
								k0.SessID = r_entry.SessID;
								k0.SeqID = r_entry.SeqID;
								if(search(0, &k0, spEq)) {
									if(!(data.Flags & USRPROFF_FINISHED)) {
										data.Flags |= USRPROFF_FINISHED;
										data.Clock = r_entry.Clock;
										data.Factor1 = r_entry.Factor1;
										data.Factor2 = r_entry.Factor2;
										data.Factor3 = r_entry.Factor3;
										THROW_DB(updateRec());
									}
								}
								else {
									UserFuncPrfTbl::Rec rec;
									MEMSZERO(rec);
									rec.SessID = r_entry.SessID;
									rec.SeqID = r_entry.SeqID;
									rec.FuncID = r_entry.FuncID;
									rec.Dt = r_entry.Dtm.d;
									rec.Tm = r_entry.Dtm.t;
									rec.Clock = r_entry.Clock;
									rec.Flags = r_entry.Flags;
									rec.Factor1 = r_entry.Factor1;
									rec.Factor2 = r_entry.Factor2;
									rec.Factor3 = r_entry.Factor3;
									THROW_DB(bei.insert(&rec));
								}
								//
								// Добавим средние значения //
								//
								THROW(AddAggrRecs(&bei, data, ufp_line));
								PPWaitPercent(j+1, flc, msg_buf);
							}
						}
						{
							PPLoadText(PPTXT_WAIT_UFPSTOREUFSTART, msg_buf);
							msg_buf.Space().Cat(p_set->DbSymb).Space().Cat(p_set->DbUuid.ToStr(S_GUID::fmtIDL, temp_buf));
							const uint slc = start_list.getCount();
							for(uint j = 0; j < start_list.getCount(); j++) {
								UserProfileLoadCacheStartEntry & r_entry = start_list.at(j);
								if(!(r_entry.Flags & USRPROFF_FINISHED)) {
									UserFuncPrfTbl::Key0 k0;
									k0.SessID = r_entry.SessID;
									k0.SeqID = r_entry.SeqID;
									if(search(0, &k0, spEq)) {
									}
									else {
										UserFuncPrfTbl::Rec rec;
										MEMSZERO(rec);
										rec.SessID = r_entry.SessID;
										rec.SeqID = r_entry.SeqID;
										rec.FuncID = r_entry.FuncID;
										rec.Dt = r_entry.Dtm.d;
										rec.Tm = r_entry.Dtm.t;
										rec.Flags = r_entry.Flags;
										THROW_DB(bei.insert(&rec));
									}
								}
								PPWaitPercent(j+1, slc, msg_buf);
							}
						}
						THROW_DB(bei.flash());
					}
				}
			}
			THROW(StB.SetItem(sti));
		}
		THROW(WriteState(0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

