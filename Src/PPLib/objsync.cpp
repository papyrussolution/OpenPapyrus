// OBJSYNC.CPP
// Copyright (c) A.Sobolev 1997-2001, 2002, 2003, 2006, 2007, 2008, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2025
// @codepage UTF-8
// Поддержка синхронизации объектов в распределенной базе данных
//
#include <pp.h>
#pragma hdrstop
//
//
//
PPCommSyncID::PPCommSyncID() : P(0), I(0)
{
}

PPCommSyncID::PPCommSyncID(const PPCommSyncID & rS) : P(rS.P), I(rS.I)
{
}

PPCommSyncID::PPCommSyncID(const ObjSyncTbl::Rec & rRec) : P(rRec.CommIdPfx), I(rRec.CommID)
{
}

PPCommSyncID::PPCommSyncID(const ObjSyncQueueTbl::Rec & rRec) : P(rRec.CommIdPfx), I(rRec.CommID)
{
}

PPCommSyncID::PPCommSyncID(const TempSyncCmpTbl::Rec & rRec) : P(rRec.CommIdPfx), I(rRec.CommID)
{
}

int FASTCALL PPCommSyncID::operator == (const PPCommSyncID s) const { return (P == s.P && I == s.I); }
int FASTCALL PPCommSyncID::operator != (const PPCommSyncID s) const { return (P != s.P || I != s.I); }
bool PPCommSyncID::IsZero() const { return (P == 0 && I == 0); }

PPCommSyncID & PPCommSyncID::Z()
{
	P = 0;
	I = 0;
	return *this;
}

PPCommSyncID FASTCALL PPCommSyncID::operator = (const PPCommSyncID s)
{
	P = s.P;
	I = s.I;
	return *this;
}

PPCommSyncID FASTCALL PPCommSyncID::operator = (const ObjSyncTbl::Rec & rRec)
{
	P = rRec.CommIdPfx;
	I = rRec.CommID;
	return *this;
}

PPCommSyncID FASTCALL PPCommSyncID::operator = (const ObjSyncQueueTbl::Rec & rRec)
{
	P = rRec.CommIdPfx;
	I = rRec.CommID;
	return *this;
}

PPCommSyncID FASTCALL PPCommSyncID::operator = (const TempSyncCmpTbl::Rec & rRec)
{
	P = rRec.CommIdPfx;
	I = rRec.CommID;
	return *this;
}

void FASTCALL PPCommSyncID::Get(ObjSyncTbl::Rec * pRec) const
{
	pRec->CommIdPfx = P;
	pRec->CommID = I;
}

void FASTCALL PPCommSyncID::Get(ObjSyncQueueTbl::Rec * pRec) const
{
	pRec->CommIdPfx = P;
	pRec->CommID = I;
}

void FASTCALL PPCommSyncID::Get(TempSyncCmpTbl::Rec * pRec) const
{
	pRec->CommIdPfx = P;
	pRec->CommID = I;
}

SString & PPCommSyncID::ToStr(long fmt, SString & rBuf) const
{
	return rBuf.Z().Cat(P).CatChar('-').Cat(I);
}

int FASTCALL PPCommSyncID::FromStr(const char * pStr)
{
	SString buf(pStr);
	SString p, i;
	if(buf.Divide('-', p, i) > 0) {
		P = (short)p.ToLong();
		I = i.ToLong();
		return 1;
	}
	else {
		P = 0;
		I = p.ToLong();
		return -1;
	}
}
//
// ObjSync
//
ObjSyncCore::ObjSyncCore() : ObjSyncTbl()
{
}

int ObjSyncCore::SearchPrivate(PPID obj, PPID id, PPID dbid, ObjSyncTbl::Rec * pRec)
{
	ObjSyncTbl::Key1 k1;
	k1.ObjType = static_cast<short>(obj);
	k1.ObjID   = id;
	k1.DBID    = static_cast<short>(NZOR(dbid, LConfig.DBDiv));
	return SearchByKey(this, 1, &k1, pRec);
}

int ObjSyncCore::SearchCommon(PPID obj, PPCommSyncID id, PPID dbid, ObjSyncTbl::Rec * pRec)
{
	ObjSyncTbl::Key0 k0;
	k0.ObjType   = static_cast<short>(obj);
	k0.CommIdPfx = id.P;
	k0.CommID    = id.I;
	k0.DBID      = static_cast<short>(NZOR(dbid, LConfig.DBDiv));
	return SearchByKey(this, 0, &k0, pRec);
}

int ObjSyncCore::SearchSync(PPID obj, PPID privateID, PPID dbID, int foreign, ObjSyncTbl::Rec * pRec)
{
	int    ok = -1, r;
	ObjSyncTbl::Rec rec;
	THROW(r = SearchPrivate(obj, privateID, (foreign ? dbID : 0), &rec));
	if(r > 0) {
		PPCommSyncID comm_id(rec);
		THROW(r = SearchCommon(obj, comm_id, (foreign ? 0L : dbID), &rec));
		if(r > 0) {
			ASSIGN_PTR(pRec, rec);
			ok = 1;
		}
		else
			ok = -2;
	}
	CATCHZOK
	return ok;
}

int ObjSyncCore::GetSyncStatus(PPID objType, PPID privID, PPCommSyncID * pCommId, PPIDArray * pDbDivList)
{
	int    ok = -1;
	PPIDArray db_div_list;
	PPCommSyncID comm_id;
	ObjSyncTbl::Rec rec;
	int    r = SearchPrivate(objType, privID, 0, &rec);
	if(r > 0) {
		if(pDbDivList) {
			ObjSyncTbl::Key0 k0;
			k0.ObjType   = static_cast<short>(objType);
			k0.CommIdPfx = rec.CommIdPfx;
			k0.CommID    = rec.CommID;
			k0.DBID      = 0;
			while(search(0, &k0, spGt)) {
				if(data.ObjType == static_cast<short>(objType) && data.CommIdPfx == rec.CommIdPfx && data.CommID == rec.CommID) {
					db_div_list.addUnique(data.DBID);
				}
				else
					break;
			}
		}
		comm_id = rec;
		if(comm_id.P == static_cast<int16>(LConfig.DBDiv))
			ok = 2;
		else
			ok = 1;
	}
	else if(!r)
		ok = 0;
	ASSIGN_PTR(pCommId, comm_id);
	ASSIGN_PTR(pDbDivList, db_div_list);
	return ok;
}

int ObjSyncCore::GetPrivateObjectsByForeignID(PPID objType, PPID foreignID, PPIDArray * pPrivateIdList)
{
	int    ok = -1;
	const  PPID local_dbdiv_id = LConfig.DBDiv;
	PPIDArray list;
	TSArray <PPCommSyncID> comm_id_list;
	ObjSyncTbl::Key1 k1;
	k1.ObjType = static_cast<short>(objType);
	k1.ObjID   = foreignID;
	k1.DBID    = 0;
	if(search(1, &k1, spGe) && data.ObjType == objType && data.ObjID == foreignID) do {
        if(data.DBID != local_dbdiv_id && !(data.Flags & 0x0001)) {
			PPCommSyncID comm_id(data);
            THROW_SL(comm_id_list.insert(&comm_id));
        }
	} while(search(1, &k1, spNext) && data.ObjType == objType && data.ObjID == foreignID);
	for(uint i = 0; i < comm_id_list.getCount(); i++) {
		PPCommSyncID comm_id(comm_id_list.at(i));
		ObjSyncTbl::Rec rec;
		if(SearchCommon(objType, comm_id, local_dbdiv_id, &rec) > 0) {
			THROW_SL(list.add(rec.ObjID));
			ok = 1;
		}
	}
	list.sortAndUndup();
	CATCHZOK
	ASSIGN_PTR(pPrivateIdList, list);
	return ok;
}

int ObjSyncCore::Search(const ObjSyncIdent * pIdent, ObjSyncTbl::Rec * pRec)
{
	if(pIdent->fComm)
		return SearchCommon(pIdent->ObjType, pIdent->CommID, pIdent->DBID, pRec);
	else
		return SearchPrivate(pIdent->ObjType, pIdent->ObjID, pIdent->DBID, pRec);
}

int ObjSyncCore::Remove_S(const ObjSyncIdent * pIdent, int use_ta)
{
	PPID   div_id = NZOR(pIdent->DBID, LConfig.DBDiv);
	DBQ  * dbq = pIdent->fComm ?
		&(this->ObjType == (long)pIdent->ObjType && this->CommIdPfx == (long)pIdent->CommID.P &&
			this->CommID == pIdent->CommID.I && this->DBID == div_id) :
		&(this->ObjType == pIdent->ObjType && this->ObjID == pIdent->ObjID && this->DBID == div_id);
	return deleteFrom(this, use_ta, *dbq);
	// return (Search(pIdent, 0) > 0) ? (deleteRec() ? 1 : PPSetErrorDB()) : -1;
}

int ObjSyncCore::AddRawRecord(ObjSyncTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	THROW_INVARG(pRec);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(insertRecBuf(pRec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ObjSyncCore::Update(const ObjSyncIdent * pIdent, const ObjSyncTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	ObjSyncTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(pIdent, &rec) > 0);
		THROW_DB(updateRecBuf(pRec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
	//return (Search(pIdent, 0) > 0) ? (updateRecBuf(pRec) ? 1 : PPSetErrorDB()) : -1;
}

int ObjSyncCore::TransmitObj(PPObjID obj, PPCommSyncID * pCommID, int use_ta)
{
	int    ok = 1, r;
	PPCommSyncID ci;
	ObjSyncTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = SearchPrivate(obj.Obj, obj.Id, 0, &rec));
		if(r > 0) {
			if(rec.Flags & 0x0001) {
				// @todo
			}
			else {
				ci = rec;
			}
		}
		else if(r < 0) {
			THROW(GetFreeCommonID(obj.Obj, &ci));
			clearDataBuf();
			data.ObjType = (short)obj.Obj;
			data.ObjID   = obj.Id;
			data.DBID    = (short)LConfig.DBDiv;
			ci.Get(&data);
			THROW_DB(insertRec());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pCommID, ci);
	return ok;
}

static void LogRcvObjDiag(uint msgID, PPID obj, PPID id, PPCommSyncID commID)
{
	SString msg_buf, fmt_buf, added_buf, temp_buf;
	GetObjectTitle(obj, temp_buf);
	added_buf.Cat(temp_buf).CatDiv('-', 1).Cat(commID.ToStr(0, temp_buf)).CatDiv('-', 1).Cat(id);
	msg_buf.Printf(PPLoadTextS(msgID, fmt_buf), added_buf.cptr());
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
}

int ObjSyncCore::_RcvObj(PPID obj, PPID id, PPCommSyncID commID, PPID dbid, const LDATETIME * pMoment, int use_ta)
{
	int    ok = 1, r;
	if(id) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = SearchCommon(obj, commID, dbid));
		if(r > 0) {
			if(data.ObjID != id) {
				THROW(r = SearchPrivate(obj, id, dbid));
				//
				// В том случае, если наш новый ид объекта уже занят под синхронизацию некоторого другого объекта,
				// удаляем ту синхронизацию. Причиной такой коллизии может быть либо то, что прежний объект был
				// удален, либо вся синхронизаци "слегка" попорчена. В обоих случаях удаление записи о
				// синронизации наиболее мягкий выход из ситуации.
				//
				if(r > 0) {
			   		THROW_DB(deleteRec());
					// Конфликт общих идентификаторов синхронизации (старая запись удалена) [%s]
					LogRcvObjDiag(PPTXT_LOG_OBJSYNC_MISM_COMM, obj, id, commID);
				}
				THROW(SearchCommon(obj, commID, dbid) > 0);
				if(r < 0) {
					if(data.Flags & OBJSYNCF_DELETED) {
						// Замещение синхронизации _DELETED_ объекта по общему идентификатору [%s]
						LogRcvObjDiag(PPTXT_LOG_OBJSYNC_RPLC_COMM_DELD, obj, id, commID);
					}
					else {
						// Замещение синхронизации _NOT_DELETED_ объекта по общему идентификатору [%s]
						LogRcvObjDiag(PPTXT_LOG_OBJSYNC_RPLC_COMM_NDELD, obj, id, commID);
					}
				}
				data.ObjID = id;
				data.Flags &= ~OBJSYNCF_DELETED;
				data.Dt = pMoment ? pMoment->d : ZERODATE;
				data.Tm = pMoment ? pMoment->t : ZEROTIME;
				THROW_DB(updateRec());
			}
			else if(data.Flags & OBJSYNCF_DELETED) {
				// Отмена статуса _DELETED_ записи синхронизации [%s]
				LogRcvObjDiag(PPTXT_LOG_OBJSYNC_UNDEL, obj, id, commID);
				data.Flags &= ~OBJSYNCF_DELETED;
				data.Dt = pMoment ? pMoment->d : ZERODATE;
				data.Tm = pMoment ? pMoment->t : ZEROTIME;
				THROW_DB(updateRec());
			}
		}
		else {
			THROW(r = SearchPrivate(obj, id, dbid));
			if(r > 0) {
				THROW_DB(deleteRec());
				// Конфликт частных идентификаторов синхронизации (старая запись удалена) [%s]
				LogRcvObjDiag(PPTXT_LOG_OBJSYNC_MISM_PRIV, obj, id, commID);
			}
			clearDataBuf();
			data.ObjType = static_cast<short>(obj);
			data.ObjID   = id;
			data.DBID    = static_cast<short>(NZOR(dbid, LConfig.DBDiv));
			commID.Get(&data);
			data.Dt      = pMoment ? pMoment->d : ZERODATE;
			data.Tm      = pMoment ? pMoment->t : ZEROTIME;
			THROW_DB(insertRec());
		}
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int ObjSyncCore::SearchCommonObj(PPID objType, PPCommSyncID commID, PPID * pID, ObjSyncTbl::Rec * pRec)
{
	int    r = SearchCommon(objType, commID, 0, pRec);
	ASSIGN_PTR(pID, (r > 0) ? data.ObjID : 0);
	return r;
}

int ObjSyncCore::AckObj(PPID objType, PPID foreignID, PPCommSyncID commID, PPID dbid, const LDATETIME * pDtm, int use_ta)
{
	return _RcvObj(objType, foreignID, commID, dbid, pDtm, use_ta);
}

int ObjSyncCore::GetFreeCommonID(PPID objType, PPCommSyncID * pCommID)
{
	// {ObjType, CommIdPfx, CommID, DBID (unique mod)};
	int    ok = 1;
	ObjSyncTbl::Key0 k0; // #0
	PPID   div = LConfig.DBDiv & 0x0000ffffL;
	k0.ObjType   = static_cast<short>(objType);
	k0.CommIdPfx = static_cast<short>(div + 1);
	k0.CommID    = 0;
	k0.DBID      = 0;
	if(search(0, &k0, spLt)) {
		pCommID->P = static_cast<short>(div);
		pCommID->I = (k0.ObjType == objType && k0.CommIdPfx == div) ? (k0.CommID+1) : 1;
	}
	else if(BTRNFOUND) {
		pCommID->P = static_cast<short>(div);
		pCommID->I = 1;
	}
	else {
		pCommID->P = 0;
		pCommID->I = 0;
		ok = PPSetErrorDB();
	}
	return ok;
}

int ObjSyncCore::SelfSync(PPID obj, PPID id, PPID destDbID, int use_ta)
{
	int    ok = 1;
	int    prim_found = 0, foreign_found = 0;
	PPCommSyncID comm_id;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(prim_found = SearchPrivate(obj, id, 0));
		if(prim_found > 0)
			comm_id = data;
		else {
			THROW(GetFreeCommonID(obj, &comm_id));
		}
		THROW(foreign_found = SearchPrivate(obj, id, destDbID));
		if(foreign_found < 0 || prim_found < 0) {
			LDATETIME moment = ZERODATETIME;
			CALLPTRMEMB(p_sj, GetLastObjModifEvent(obj, id, &moment, 0));
			if(prim_found < 0) {
				clearDataBuf();
				data.ObjType  = (short)obj;
				data.ObjID    = id;
				data.DBID     = (short)LConfig.DBDiv;
				comm_id.Get(&data);
				data.Dt       = moment.d;
				data.Tm       = moment.t;
				THROW_DB(insertRec());
			}
			if(foreign_found < 0) {
				clearDataBuf();
				data.ObjType  = (short)obj;
				data.ObjID    = id;
				data.DBID     = (short)destDbID;
				comm_id.Get(&data);
				data.Dt       = moment.d;
				data.Tm       = moment.t;
				THROW_DB(insertRec());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ObjSyncCore::RemoveByPrivateID(PPID objType, PPID objID, int use_ta)
{
	int    ok = -1;
	if(objType > 0 && objID > 0) {
		ObjSyncTbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		int    r = SearchPrivate(objType, objID, 0, &rec);
		THROW(r);
		if(r > 0) {
			if(rec.Flags & OBJSYNCF_DELETED) {
				SString fmt_buf, add_buf, msg_buf;
				PPLoadText(PPTXT_ERR_OBJSYNCALLREADYRMVD, fmt_buf);
				msg_buf.Printf(fmt_buf, add_buf.Cat(objType).CatDiv(';', 2).Cat(objID).cptr());
				PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			rec.Flags |= OBJSYNCF_DELETED;
			getcurdatetime(&rec.Dt, &rec.Tm);
			THROW_DB(updateRecBuf(&rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ObjSyncCore::RemoveByCommID(PPID objType, PPCommSyncID commID, PPID dbid, int use_ta)
{
	DBQ  * dbq = &(this->ObjType == objType && this->CommIdPfx == (long)commID.P && this->CommID == commID.I);
	if(dbid)
		dbq = &(*dbq && this->DBID == dbid);
	return deleteFrom(this, use_ta, *dbq) ? 1 : PPSetErrorDB();
}
//
//
//
ObjSyncQueueCore::FileInfo::FileInfo() : QueueID(0), Flags(0), Mod(ZERODATETIME)
{
}

int ObjSyncQueueCore::AddFileRecord(PPID * pID, const ObjSyncQueueCore::FileInfo & rInfo, int use_ta)
{
	int    ok = 1;
	ObjSyncQueueTbl::Rec rec;
	SString name_buf, temp_buf;
	SFsPath ps(rInfo.InnerFileName);
	name_buf = ps.Nam; // Файл не имеет расширения //
	ps.Split(rInfo.OrgFileName);
	ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, temp_buf);
	name_buf.Semicol().Cat(temp_buf);
	// @v12.3.3 @ctr MEMSZERO(rec);
	name_buf.CopyTo(rec.ObjName, sizeof(rec.ObjName));
	rec.ModDt = rInfo.Mod.d;
	rec.ModTm = rInfo.Mod.t;
	rec.CommID = (long)(uint32)rInfo.Ver;
	rec.Flags  = (int16)rInfo.Flags;
	THROW(AddByID(this, pID, &rec, use_ta));
	CATCHZOK
	return ok;
}

int ObjSyncQueueCore::GetFileRecord(PPID id, ObjSyncQueueCore::FileInfo & rInfo)
{
	int    ok = 1;
	SString left, right;
	ObjSyncQueueTbl::Rec file_rec;
	int    r = Search(id, &file_rec);
	THROW(r);
	THROW_PP_S(r > 0, PPERR_OBJSYNCQFILERECNFOUND, left.Cat(id));
	(rInfo.InnerFileName = file_rec.ObjName).Divide(';', left, right);
	PPObjectTransmit::GetQueueFilePath(rInfo.InnerFileName).SetLastSlash().Cat(left);
	rInfo.QueueID = id;
	rInfo.OrgFileName = right;
	rInfo.Mod.Set(file_rec.ModDt, file_rec.ModTm);
	rInfo.Ver.Set((uint32)file_rec.CommID);
	rInfo.Flags = file_rec.Flags;
	THROW_SL(::fileExists(rInfo.InnerFileName));
	CATCHZOK
	return ok;
}

int ObjSyncQueueCore::SearchRefToOrgFile(const char * pFileName, ObjSyncQueueCore::FileInfo * pInfo)
{
	int    ok = -1;
	SString temp_buf, left, right, nam;
	ObjSyncQueueTbl::Key0 k0;
	SFsPath ps(pFileName);
	ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, nam);
	MEMSZERO(k0);
	BExtQuery q(this, 0);
	q.select(this->ID, this->ObjName, 0L).where(this->ObjType == 0L && this->ObjID == 0L);
	for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
		if((temp_buf = data.ObjName).Divide(';', left, right) > 0) {
			if(nam.CmpNC(right) == 0) {
				if(pInfo)
					GetFileRecord(data.ID, *pInfo);
				ok = 1;
			}
		}
	}
	return ok;
}

int ObjSyncQueueCore::MarkAsProcessed(PPID queueID, PPID primaryID, int use_ta)
{
	int    ok = 1;
	ObjSyncQueueTbl::Rec queue_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(queueID, &queue_rec) > 0) {
			queue_rec.PrimObjID = primaryID;
			queue_rec.Flags |= PPObjPack::fProcessed;
			THROW_DB(updateRecBuf(&queue_rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

ObjSyncQueueCore::ObjSyncQueueCore() : ObjSyncQueueTbl()
{
}

int ObjSyncQueueCore::Search(PPID queueID, ObjSyncQueueTbl::Rec * pRec) { return SearchByID(this, 0, queueID, pRec); }

int ObjSyncQueueCore::SearchObject_(PPID objType, PPID objID, PPID dbID, ObjSyncQueueTbl::Rec * pRec)
{
	ObjSyncQueueTbl::Key1 k1;
	k1.ObjType = (short)objType;
	k1.ObjID   = objID;
	k1.DBID    = (short)dbID;
	return SearchByKey(this, 1, &k1, pRec);
}

int ObjSyncQueueCore::PrintDebugObjList(PPID objType)
{
	int    ok = 1;
    SString buf, temp_buf;
    ObjSyncQueueTbl::Key0 k0;
    MEMSZERO(k0);
    if(search(0, &k0, spFirst)) do {
    	if(!objType || data.ObjType == objType) {
			PPCommSyncID commid(data);
			commid.ToStr(0, temp_buf.Z());
			if(buf.IsEmpty())
				buf.Cat("ObjSyncQueue Debug List").Space().CatEq("ObjType", objType).CR();
			buf.CatEq("DBID", (long)data.DBID).Space().CatEq("ObjType", (long)data.ObjType).Space().CatEq("ObjID", data.ObjID).Space().
				CatEq("CommId", temp_buf).Space().Cat("Flags").CatDiv('=', 0).CatHex((long)data.Flags).Space().CatEq("PrimObjID", data.PrimObjID).CR();
    	}
    } while(search(0, &k0, spNext));
    if(buf.NotEmpty()) {
		PPLogMessage(PPFILNAM_SYNCLOT_LOG, buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
    }
    else
		ok = -1;
    return ok;
}

int ObjSyncQueueCore::GetUnprocessedList(PPIDArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	ObjSyncQueueTbl::Key3 k3;
	BExtQuery q(this, 3);
	q.select(this->ID, this->ObjType, this->ObjID, this->Flags, 0).where(this->ObjType > 0L && this->ObjID > 0L);
	MEMSZERO(k3);
	for(q.initIteration(false, &k3, spFirst); q.nextIteration() > 0;)
		if(!(data.Flags & PPObjPack::fProcessed) && data.Priority != PPObjectTransmit::DependedPriority) {
			ok = 1;
			if(pList)
				pList->add(data.ID);
			else
				break;
		}
	return ok;
}

int ObjSyncQueueCore::Clear()
{
	int    ok = 1;
	SString file_path;
	PPObjectTransmit::GetQueueFilePath(file_path);
	THROW_DB(deleteFrom(this, 1, *reinterpret_cast<DBQ *>(0)));
	PPRemoveFilesByExt(file_path, 0, 0, 0); // Все файлы - без расширений
	CATCHZOK
	return ok;
}
