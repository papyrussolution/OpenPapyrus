// PPSYNC.CPP
// (c) Alexander Simanov, 1999
// Database lock file implementation
// Modified by Anton Sobolev 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2011, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <errno.h>
#include <fcntl.h>
#include <share.h>
#include <sys\stat.h>
#include <sys\locking.h>

#define LOCK_MAGIC        0x18051974L
#define LOCK_PRFX_SIZE    sizeof(long)
#define HDR_SZ            (sizeof(PPSyncHeader) + LOCK_PRFX_SIZE)
#define BLK_SZ            (sizeof(PPSyncItem) + LOCK_PRFX_SIZE)

SLAPI PPSync::PPSync()
{
	Handle = -1;
}

SLAPI PPSync::~PPSync()
{
	if(Handle >= 0)
		close(Handle);
}

int FASTCALL PPSync::CheckOffset(ulong offs) const
{
	return (offs && ((offs - HDR_SZ) % BLK_SZ) != 0) ? PPSetError(PPERR_SYNCINVOFFS, DataPath) : 1;
}

int SLAPI PPSync::Release()
{
	int    ok = 1;
	if(Handle >= 0) {
		const int is_db_locked = IsDBInUse();
		SString file_name;
		close(Handle);
		Handle = -1;
		if(!is_db_locked)
			if(SFile::Remove(GetFileName(file_name)) == 0)
				ok = 2;
		DataPath = 0;
	}
	else
		ok = -1;
	return ok;
}

SString & SLAPI PPSync::GetFileName(SString & rBuf) const
{
	return (rBuf = DataPath).SetLastSlash().Cat("ppsync").Dot().Cat("bin");
}

int SLAPI PPSync::Init(const char * pDataPath)
{
	int    ok = 1;
	int    empty_file_created = 0;
	SString file_name;
	PPSyncHeader hdr;
	if(Handle >= 0) {
		_close(Handle);
		Handle = -1;
	}
	NoSHARE = 0;
	DataPath = pDataPath;
	GetFileName(file_name);
	THROW(::access(file_name, 0) == 0 || (empty_file_created = CreateEmptyFile()) != 0);
	Handle = _sopen(file_name, O_RDWR | O_BINARY, SH_DENYNO, S_IREAD | S_IWRITE);
	THROW_PP_S(Handle >= 0, PPErrCode = PPERR_SYNCOPEN, file_name);
	if(!empty_file_created && GetHeader(0, &hdr, 0) && hdr.Ver < 1) {
		const int r = Release();
		THROW_PP_S(r == 2, PPERR_SYNCOPEN, file_name);
		THROW(Init(pDataPath));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPSync::CreateEmptyFile()
{
	int    ok = 0;
	long   lockfield = 0;
	SString file_name;
	PPSyncHeader hdr;
	MEMSZERO(hdr);
	hdr.Magic = LOCK_MAGIC;
	hdr.Ver = 1;
	const int h = ::creat(GetFileName(file_name), S_IWRITE);
	if(h < 0)
		ok = PPSetError(PPERR_SYNCCREAT, file_name);
	else {
		const int r1 = ::write(h, &lockfield, LOCK_PRFX_SIZE);
		const int r2 = ::write(h, &hdr, sizeof(hdr));
		::close(h);
		ok = ((r1 < 0) || (r2 < 0)) ? PPSetError(PPERR_SYNCWRITE, file_name) : 1;
	}
	return ok;
}

int SLAPI PPSync::ImplementGetBlock(ulong offs, void * pBuf, int * pIsLocked)
{
	int    ok = 1;
	union _Blk {
		PPSyncItem   item;
		PPSyncHeader hdr;
	} blk;
	if(CheckOffset(offs)) {
		_lseek(Handle, offs + LOCK_PRFX_SIZE, SEEK_SET);
		MEMSZERO(blk);
		int    r = read(Handle, &blk, offs ? sizeof(PPSyncItem) : sizeof(PPSyncHeader));
		if(r >= 0) {
			ASSIGN_PTR(pIsLocked, BIN(IsLocked(offs)));
			if(pBuf)
				memcpy(pBuf, &blk, offs ? sizeof(blk.item) : sizeof(blk.hdr));
		}
		else
			ok = PPSetError(PPERR_SYNCREAD, DataPath);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPSync::GetHeader(ulong offs, PPSyncHeader * hdr, int * isLocked)
{
	return ImplementGetBlock(offs, hdr, isLocked);
}

int SLAPI PPSync::GetItem(ulong offs, PPSyncItem * blk, int * isLocked)
{
	return offs ? ImplementGetBlock(offs, blk, isLocked) : PPSetError(PPERR_SYNCINVOFFS, DataPath);
}

int SLAPI PPSync::GetItemsList(long recType, PPSyncArray * pList)
{
	int    ok = -1;
	ulong  o = 0;
	long   numrecs = 0;
	PPSyncItem item;
	CALLPTRMEMB(pList, freeAll());
	if(GetInitPosition(&o, &numrecs) > 0) {
		if(pList)
			for(long i = 0; i < numrecs; AdvanceOffset(&o), i++)
				if(GetItem(o, &item, 0) > 0 && item.Type == recType && item.ID && IsLocked(o) > 0)
					ok = pList->insert(&item) ? 1 : PPSetErrorSLib();
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPSync::GetItem(PPID id, PPSyncItem * pItem)
{
	int    ok = -1;
	ulong  o = 0;
	long   numrecs = 0;
	PPSyncItem item;
	if(id) {
		if(GetInitPosition(&o, &numrecs) > 0) {
			for(long i = 0; ok != 1 && i < numrecs; AdvanceOffset(&o), i++)
				if(GetItem(o, &item, 0) > 0 && item.ID == id && IsLocked(o) > 0) {
					ASSIGN_PTR(pItem, item);
					ok = 1;
				}
		}
		else
			ok = 0;
	}
	return ok;
}

int FASTCALL PPSync::AdvanceOffset(ulong * pOffs) const
{
	if(CheckOffset(*pOffs)) {
		(*pOffs) += ((*pOffs) ? BLK_SZ : HDR_SZ);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPSync::GetInitPosition(ulong * pOffs, long * pNumItems)
{
	ulong  o = 0;
	PPSyncHeader hdr;
	if(GetHeader(o, &hdr, 0) > 0) {
		AdvanceOffset(&o);
		*pOffs = o;
		*pNumItems = hdr.RecsCount;
		return 1;
	}
	else {
		*pOffs = 0;
		*pNumItems = 0;
		return 0;
	}
}

int SLAPI PPSync::GetOffsetByID(long id, ulong * pOffs)
{
	int    ok = -1;
	ulong  result_offs = 0;
	ulong  o = 0;
	long   numrecs = 0;
	PPSyncItem   item;
	THROW(GetInitPosition(&o, &numrecs) > 0);
	for(long i = 0; ok < 0 && i < numrecs; AdvanceOffset(&o), i++) {
		THROW(GetItem(o, &item, 0) > 0);
		if(item.ID == id) {
			result_offs = o;
			ok = 1;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pOffs, result_offs);
	return ok;
}

int SLAPI PPSync::GetFreeEntry(ulong * offs, long * id)
{
	int    ok = 1;
	ulong  o = 0, free_offs = 0;
	long   numrecs = 0;
	long   free_id = 0;
	PPSyncItem   item;
	if(GetInitPosition(&o, &numrecs) > 0) {
		for(long i = 0; ok && i < numrecs; AdvanceOffset(&o), i++) {
			if(GetItem(o, &item, 0) > 0) {
				SETMAX(free_id, item.ID);
				if(item.ID == 0 && free_offs == 0 && !IsLocked(o))
					free_offs = o;
				ok = 1;
			}
			else
				ok = 0;
		}
		if(ok && free_offs == 0) {
			free_offs = o;
			ok = -1;
		}
	}
	else
		ok = 0;
	ASSIGN_PTR(offs, free_offs);
	ASSIGN_PTR(id, free_id + 1);
	return ok;
}

long SLAPI PPSync::AddRecord(long type, long appID, const char * pName,
	long objtype, long objid, ulong * pOffs, const MACAddr * pMachineID, ulong terminalSessID)
{
	long   rec_id = 0;
	PPSyncItem rec;
	int    is_new_rec = 0;
	long   lockfield = 0;
	ulong  offs = 0;
	int    r = GetFreeEntry(&offs, &rec_id);
	if(r < 0)
		is_new_rec = 1;
	MEMSZERO(rec);
	rec.ID      = rec_id;
	rec.Type    = type;
	rec.UserID  = appID;
	rec.ObjType = objtype;
	rec.ObjID   = objid;
	RVALUEPTR(rec.MchnID, pMachineID);
	if(terminalSessID)
		rec.TerminalSessID = terminalSessID;
	STRNSCPY(rec.Name, pName);
	ASSIGN_PTR(pOffs, offs);
	_lseek(Handle, offs, SEEK_SET);
	THROW_PP_S(_write(Handle, &lockfield, sizeof(long)) >= 0, PPERR_SYNCWRITE, DataPath);
	THROW_PP_S(_write(Handle, &rec, sizeof(PPSyncItem)) >= 0, PPERR_SYNCWRITE, DataPath);
	if(is_new_rec) {
		PPSyncHeader hdr;
		THROW(GetHeader(0, &hdr, 0) > 0);
		hdr.RecsCount++;
		_lseek(Handle, LOCK_PRFX_SIZE, SEEK_SET);
		THROW_PP_S(_write(Handle, &hdr, sizeof(PPSyncHeader)) >= 0, PPERR_SYNCWRITE, DataPath);
	}
	CATCH
		rec_id = 0;
	ENDCATCH
	return rec_id;
}

int SLAPI PPSync::ReleaseItem(ulong offs)
{
	PPSyncItem rr;
	int    is_locked = 0;
	GetItem(offs, &rr, &is_locked);
	if(is_locked)
		if(!Unlock(offs))
			return 0;
	MEMSZERO(rr);
	_lseek(Handle, offs + LOCK_PRFX_SIZE, SEEK_SET);
	int    r = _write(Handle, &rr, sizeof(PPSyncItem));
	return (r < 0) ? PPSetError(PPERR_SYNCWRITE, DataPath) : 1;
}

int SLAPI PPSync::Lock(ulong offs, int checkOnly)
{
	int    ok = 1;
	if(CheckOffset(offs)) {
		if(!NoSHARE) {
			_lseek(Handle, offs, SEEK_SET);
			if(_locking(Handle, LK_NBLCK, LOCK_PRFX_SIZE) < 0)
				if(errno == EINVAL) {
					NoSHARE = 1;
					PPLogMessage(PPFILNAM_ERR_LOG, "PPSync locking error EINVAL - turned on NoSHARE mode", LOGMSGF_TIME|LOGMSGF_USER); // @7.7.10 @debug
				}
				else
					ok = checkOnly ? 0 : PPSetError(PPERR_SYNCLOCKFAULT, DataPath);
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPSync::Unlock(ulong offs)
{
	int    ok = 1;
	if(CheckOffset(offs)) {
		if(!NoSHARE) {
			_lseek(Handle, offs, SEEK_SET);
			if(_locking(Handle, LK_UNLCK, LOCK_PRFX_SIZE) < 0)
				if(errno == EINVAL) {
					NoSHARE = 1;
					PPLogMessage(PPFILNAM_ERR_LOG, "PPSync unlocking error EINVAL - turned on NoSHARE mode", LOGMSGF_TIME|LOGMSGF_USER); // @7.7.10 @debug
				}
				else
					ok = PPSetError(PPERR_SYNCUNLOCKFAULT, DataPath);
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPSync::IsLocked(ulong offs)
{
	if(Lock(offs, 1) > 0) {
		Unlock(offs);
		return 0;
	}
	else
		return 1;
}

int SLAPI PPSync::LockDB()
{
	PPSetAddedMsgString(DataPath);
	if(IsDBInUse())
		return PPSetError(PPERR_SYNCDBINUSE);
	//PPLogMessage(PPFILNAM_INFO_LOG, "BASE IS BLOCK", LOGMSGF_TIME|LOGMSGF_USER); // @debug turistti @v5.3.4
	return Lock(0, 0) ? 1 : PPSetError(PPERR_SYNCMUTEXBUSY);
}

int SLAPI PPSync::UnlockDB()
{
	//PPLogMessage(PPFILNAM_INFO_LOG, "BASE IS NO BLOCK", LOGMSGF_TIME|LOGMSGF_USER);// @debug turistti @v5.3.4
	return Unlock(0);
}

int SLAPI PPSync::IsDBLocked()
{
	//PPLogMessage(PPFILNAM_INFO_LOG, "BASE IS BLOCK", LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR|LOGMSGF_DBINFO);// @debug turistti @v5.3.4
	return IsLocked(0);
}

int SLAPI PPSync::LoginUser(long userID, const char * pUserName, long * pSessID, const MACAddr * pMachineID, ulong terminalSessID)
{
	int    ok = 1;
	ulong  offs = 0;
	if(IsDBLocked())
		ok = PPSetError(PPERR_SYNCDBLOCKED, DataPath);
	else {
		long   id = AddRecord(PPSYNC_DBLOCK, userID, pUserName, 0, DS.CheckExtFlag(ECF_SYSSERVICE) ? 1 : 0, &offs, pMachineID, terminalSessID);
		if(id && Lock(offs, 0)) {
			ASSIGN_PTR(pSessID, id);
		}
		else
			ok = 0;
	}
	if(!ok)
		PPLogMessage(PPFILNAM_INFO_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);// @debug turistti @v5.3.4
	return ok;
}

int SLAPI PPSync::LogoutUser(long sessID)
{
	ulong  offs = 0;
	if(Handle >= 0 && GetOffsetByID(sessID, &offs) > 0) {
		ReleaseSessionMutex(sessID);
		ReleaseItem(offs);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPSync::IsDBInUse()
{
	ulong  offs = 0;
	long   numrecs = 0;
	PPSyncItem item;
	if(GetInitPosition(&offs, &numrecs) > 0)
		for(long i = 0; i < numrecs; AdvanceOffset(&offs), i++)
			if(GetItem(offs, &item, 0) > 0 && item.ID && item.Type == PPSYNC_DBLOCK && IsLocked(offs))
   	            return 1;
	return 0;
}

int SLAPI PPSync::SearchMutexByObj(long objType, long objID, PPSyncItem * pItem, ulong * pOffs)
{
	ulong  offs = 0;
	long   numrecs = 0;
	PPSyncItem   item;
	if(GetInitPosition(&offs, &numrecs) > 0)
		for(long i = 0; i < numrecs; AdvanceOffset(&offs), i++)
			if(GetItem(offs, &item, 0) > 0) {
				if(item.ID && item.Type == PPSYNC_MUTEX && item.ObjType == objType && item.ObjID == objID) {
					if(IsLocked(offs)) {
						ASSIGN_PTR(pItem, item);
						ASSIGN_PTR(pOffs, offs);
						return 1;
					}
				}
			}
	return -1;
}

int SLAPI PPSync::CreateMutex(long sessID, long objtype, long objid, long * pMutexID, PPSyncItem * pItem)
{
	int    ok = 1;
	PPSyncItem item;
	if(SearchMutexByObj(objtype, objid, &item, 0) > 0) {
		ASSIGN_PTR(pMutexID, item.ID);
		ASSIGN_PTR(pItem, item);
		ok = -1;
	}
	else {
		ulong  offs = 0, term_sess_id = 0;
		SString user_name;
		MACAddr mac_addr;
		DS.GetMachineID(&mac_addr, 0);
		GetCurUserName(user_name);
		ProcessIdToSessionId(GetCurrentProcessId(), &term_sess_id);
		long   recid = AddRecord(PPSYNC_MUTEX, sessID, (const char*)user_name, objtype, objid, &offs, &mac_addr, term_sess_id);
		if(recid) {
			if(!Lock(offs, 0))
				ok = 0;
			ASSIGN_PTR(pMutexID, recid);
			if(pItem) {
				memzero(pItem, sizeof(*pItem));
				pItem->ID = recid;
				pItem->Type = PPSYNC_MUTEX;
				pItem->UserID = sessID;
				STRNSCPY(pItem->Name, user_name);
				pItem->ObjType = objtype;
				pItem->ObjID = objid;
				pItem->MchnID = mac_addr;
			}
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI PPSync::ClearMutex(PPID mutexID)
{
	int    ok = 0;
	ulong  offs = 0;
	PPSyncItem item;
	if(GetOffsetByID(mutexID, &offs) > 0) {
		GetItem(offs, &item, 0);
		MEMSZERO(item);
		_lseek(Handle, offs + LOCK_PRFX_SIZE, SEEK_SET);
		int    r = write(Handle, &item, sizeof(PPSyncItem));
		if(r < 0)
			PPSetError(PPERR_SYNCWRITE, DataPath);
		else
			ok = 1;
	}
	return ok;
}

int SLAPI PPSync::ReleaseSessionMutex(long sessID)
{
	int    ok   = 1;
	ulong  offs = 0;
	long   numrecs = 0;
	int    is_locked = 0;
	PPSyncItem   item;
	if(GetInitPosition(&offs, &numrecs) > 0)
		for(long i = 0; i < numrecs; AdvanceOffset(&offs), i++)
			if(GetItem(offs, &item, &is_locked) > 0 && item.Type == PPSYNC_MUTEX && item.UserID == sessID)
				ReleaseItem(offs);
	return ok;
}

int SLAPI PPSync::ReleaseMutex(long objtype, long objid)
{
	ulong  offs;
	PPSyncItem item;
	return BIN(SearchMutexByObj(objtype, objid, &item, &offs) > 0 && ReleaseItem(offs));
}

int SLAPI PPSync::ReleaseMutex(long mutexid)
{
	ulong  offs = 0;
	PPSyncItem item;
	if(GetOffsetByID(mutexid, &offs) > 0) {
		GetItem(offs, &item, 0);
		return BIN(item.Type == PPSYNC_MUTEX && ReleaseItem(offs));
	}
	else
		return 0;
}

int SLAPI PPSync::IsMyLock(long sessID, long objtype, long objid)
{
	PPSyncItem item;
	return BIN(SearchMutexByObj(objtype, objid, &item, 0) > 0 && item.UserID == sessID);
}

int PPSync::GetLockingText(PPID mutexID, int resolveHostName, SString & rBuf)
{
	int    ok = -1;
	SString addr_buf, ip_buf, mac_addr;
	rBuf = 0;
	PPSyncItem item;
	if(GetItem(mutexID, &item) > 0) {
		InetAddr addr;
		item.MchnID.ToStr(mac_addr);
		GetFirstHostByMACAddr(&item.MchnID, &addr);
		rBuf.CatEq("User", item.Name).CatDiv(',', 2);
		rBuf.CatEq("Host", addr.ToStr(InetAddr::fmtHost, addr_buf)).CatDiv(',', 2);
		rBuf.CatEq("IP", addr.ToStr(InetAddr::fmtAddr, ip_buf)).CatDiv(',', 2).CatEq("MAC addr", mac_addr);
		ok = 1;
	}
	return ok;
}

//#define TEST

#ifdef TEST

void login(PPSync * pps)
{
	char   name[48], * c;
	long   userid = 0, sess_id = 0;
	printf("\nInput user name >");
	scanf("%s", name);
	for(c = name; *c; c++)
		userid += (long)*c;
	if(pps->LoginUser(userid, name, &sess_id))
		printf("Success: sess_id = %ld\n", sess_id);
	else
		printf("Error: %d\n", PPErrCode);
}

void logout(PPSync * pps)
{
	long   sess_id = 0;
	printf("Input session id >");
	scanf("%ld", &sess_id);
	if(pps->LogoutUser(sess_id))
		printf("Success\n");
	else
		printf("Error: %d\n", PPErrCode);
}

void userlist(PPSync * pps)
{
	PPSyncArray ary;
	if(pps->GetItemsList(PPSYNC_DBLOCK, &ary)) {
		printf("User list:\n");
		for(uint i = 0; i < ary.getCount(); i++) {
			PPSyncItem & item = ary.at(i);
			printf("\t%ld\t%ld\t%s\n", item.ID, item.UserID, item.Name);
		}
		printf("\n");
	}
	else
		printf("Error: %d\n", PPErrCode);
}

void main()
{
	PPSync lock;
	int choice;
	if(!lock.Init("c:\\ppy")) {
		printf("Error initializing PPSYNC object (error = %d)\n", PPErrCode);
		return;
	}
	do {
		printf("1. login\n");
		printf("2. logout\n");
		printf("3. user list\n");
		printf("4. exit\n");
		printf("Your choice >");
		scanf("%d", &choice);
		switch(choice) {
			case 1: login(&lock); break;
			case 2: logout(&lock); break;
			case 3: userlist(&lock); break;
		}
	} while(choice != 4);
}

#endif // TEST
