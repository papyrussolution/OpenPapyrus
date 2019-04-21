// PSNEVENT.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PersonEventCore::PairIdent::PairIdent()
{
	THISZERO();
}
//
//
//
SLAPI PersonEventCore::PersonEventCore() : PersonEventTbl()
{
}

int SLAPI PersonEventCore::Search(PPID id, PersonEventTbl::Rec * pRec)
	{ return SearchByID(this, PPOBJ_PERSONEVENT, id, pRec); }
int SLAPI PersonEventCore::Add(PPID * pID, PersonEventTbl::Rec * pRec, int use_ta)
	{ return (IncDateKey(this, 1, pRec->Dt, &pRec->OprNo) && AddObjRecByID(this, PPOBJ_PERSONEVENT, pID, pRec, use_ta)); }
int SLAPI PersonEventCore::Update(PPID id, PersonEventTbl::Rec * pRec, int use_ta)
	{ return UpdateByID(this, PPOBJ_PERSONEVENT, id, pRec, use_ta); }
int SLAPI PersonEventCore::Remove(PPID id, int use_ta)
	{ return RemoveByID(this, id, use_ta); }

int SLAPI PersonEventCore::SearchPair(const PairIdent * pIdent, int forward, PersonEventTbl::Rec * pRec)
{
	int    ok = -1;
	PersonEventTbl::Key3 k3;
	MEMSZERO(k3);
	k3.PersonID = pIdent->PersonID;
	k3.Dt = pIdent->Dt;
	k3.OprNo = pIdent->OprNo;
	int    sp = forward ? spGt : spLt;
	while(ok < 0 && search(3, &k3, sp) && k3.PersonID == pIdent->PersonID) {
		if(!(data.Flags & PSNEVF_FORCEPAIR)) {
			if(!(pIdent->Flags & pIdent->fUseSCard) || data.PrmrSCardID == pIdent->SCardID) {
				if(data.OpID == pIdent->ThisOpID) {
					if(pIdent->Flags & PairIdent::fSignalNpError) {
						copyBufTo(pRec);
						ok = PPSetError(PPERR_NONPAIRPSNEVNT);
					}
					else if(pIdent->Flags & PairIdent::fSignalAnalog) {
						copyBufTo(pRec);
						ok = 2;
					}
				}
				else if(pIdent->PairOpID && data.OpID == pIdent->PairOpID) {
					copyBufTo(pRec);
					ok = 1;
				}
			}
		}
	}
	return BTROKORNFOUND ? ok : PPSetErrorDB();
}

int SLAPI PersonEventCore::InitEnum(PPID prmrPersonID, const DateRange * pPeriod, long * pHandle)
{
	BExtQuery * q = new BExtQuery(this, 3);
	DBQ * dbq = &(this->PersonID == prmrPersonID);
	if(pPeriod) {
		if(checkdate(pPeriod->low))
            dbq = &(*dbq && this->Dt >= pPeriod->low);
		if(checkdate(pPeriod->upp))
			dbq = &(*dbq && this->Dt <= pPeriod->upp);
	}
	q->selectAll().where(*dbq);
	PersonEventTbl::Key3 k3;
	MEMSZERO(k3);
	k3.PersonID = prmrPersonID;
	k3.OprNo = 0;
	q->initIteration(0, &k3, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

SEnumImp * SLAPI PersonEventCore::EnumByPerson(PPID prmrPesonID, const DateRange * pPeriod)
{
	long   h = -1;
	return InitEnum(prmrPesonID, pPeriod, &h) ? new PPTblEnum <PersonEventCore>(this, h) : 0;
}

int SLAPI PersonEventCore::CalcCountForPeriod(PPID opID, PPID personID, const STimeChunk & rTc, uint * pCount)
{
	int    ok = -1;
	uint   count = 0;
	union {
		PersonEventTbl::Key1 k1; // Dt, OprNo (unique mod);             // #1
		PersonEventTbl::Key2 k2; // OpID, Dt, OprNo (unique mod);       // #2
		PersonEventTbl::Key3 k3; // PersonID, Dt, OprNo (unique mod);   // #3
	} k;
	int    idx = 0;
	DBQ * dbq = 0;
	MEMSZERO(k);
	if(personID) {
		idx = 3;
		k.k3.PersonID = personID;
		k.k3.Dt = rTc.Start.d;
		dbq = &(this->PersonID == personID && this->Dt >= rTc.Start.d && this->Dt <= rTc.Finish.d);
		dbq = ppcheckfiltid(dbq, this->OpID, opID);
	}
	else if(opID) {
		idx = 2;
		k.k2.OpID = opID;
		k.k2.Dt = rTc.Start.d;
		dbq = &(this->OpID == opID && this->Dt >= rTc.Start.d && this->Dt <= rTc.Finish.d);
	}
	else {
		idx = 1;
		k.k1.Dt = rTc.Start.d;
		dbq = &(this->Dt >= rTc.Start.d && this->Dt <= rTc.Finish.d);
	}
	BExtQuery q(this, idx);
	q.select(this->Dt, this->Tm, 0).where(*dbq);
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
		LDATETIME moment;
		moment.Set(data.Dt, data.Tm);
		if(rTc.Has(moment)) {
			count++;
		}
	}
	if(count)
		ok = 1;
	ASSIGN_PTR(pCount, count);
	return ok;
}
//
//
//
PPPsnEventPacket::OnTurnBlock::OnTurnBlock() : SCardID(0), SCardWrOffAmount(0.0), SCardRest(0.0), P_Pdb(0)
{
}

PPPsnEventPacket::OnTurnBlock::~OnTurnBlock()
{
	ZDELETE(P_Pdb);
}

PPPsnEventPacket::OnTurnBlock & PPPsnEventPacket::OnTurnBlock::Z()
{
	ZDELETE(P_Pdb);
	SCardID = 0;
	SCardWrOffAmount = 0.0;
	SCardRest = 0.0;
	return *this;
}
//
//
//
SLAPI PPPsnEventPacket::PPPsnEventPacket()
{
	MEMSZERO(Rec);
	MEMSZERO(Reg);
	Otb.Z();
}

void SLAPI PPPsnEventPacket::Destroy()
{
	MEMSZERO(Rec);
	MEMSZERO(Reg);
	TagL.Destroy();
	LinkFiles.Clear();
	Otb.Z();
}

int  FASTCALL PPPsnEventPacket::Init(PPID op)
{
	Destroy();
	TagL.ObjType = PPOBJ_PERSONEVENT;
	TagL.ObjID   = 0;
	Rec.OpID = op;
	getcurdatetime(&Rec.Dt, &Rec.Tm);
	LinkFiles.Clear();
	return 1;
}

int FASTCALL PPPsnEventPacket::Copy(const PPPsnEventPacket & src)
{
	Destroy();
	Rec = src.Rec;
	Reg = src.Reg;
	TagL.Copy(src.TagL);
	LinkFiles = src.LinkFiles;
	Otb = src.Otb;
	return 1;
}

PPPsnEventPacket & FASTCALL PPPsnEventPacket::operator = (const PPPsnEventPacket & s)
{
	Copy(s);
	return *this;
}

int SLAPI PPObjPersonEvent::SerializePacket(int dir, PPPsnEventPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(RegObj.P_Tbl->SerializeRecord(dir, &pPack->Reg, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}
//
//
//
TLP_IMPL(PPObjPersonEvent, PersonEventCore, P_Tbl);

SLAPI PPObjPersonEvent::PPObjPersonEvent(void * extraPtr) : PPObject(PPOBJ_PERSONEVENT), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	P_ScObj = new PPObjSCard;
}

SLAPI PPObjPersonEvent::~PPObjPersonEvent()
{
	TLP_CLOSE(P_Tbl);
	delete P_ScObj;
}

// static
SString & FASTCALL PPObjPersonEvent::MakeCodeString(const PersonEventTbl::Rec * pRec, int options, SString & rBuf)
{
	rBuf.Z();
	if(pRec) {
		rBuf.Cat(pRec->Dt).Space().Cat(pRec->Tm);
		if(pRec->PersonID) {
			rBuf.CatDiv('-', 1);
			GetObjectName(PPOBJ_PERSON, pRec->PersonID, rBuf, 1);
		}
		else if(pRec->SecondID) {
			rBuf.CatDiv('-', 1);
			GetObjectName(PPOBJ_PERSON, pRec->SecondID, rBuf, 1);
		}
		if(pRec->Memo[0]) {
			rBuf.CatDiv('-', 1).Cat(pRec->Memo);
		}
		rBuf.Trim(63);
	}
	return rBuf;
}

int SLAPI PPObjPersonEvent::Search(PPID id, void * b)
	{ return P_Tbl->Search(id, (PersonEventTbl::Rec *)b); }
const char * SLAPI PPObjPersonEvent::GetNamePtr()
	{ return PPObjPersonEvent::MakeCodeString(&P_Tbl->data, 1, NameBuf).cptr(); }
int SLAPI PPObjPersonEvent::DeleteObj(PPID id)
	{ return PutPacket(&id, 0, 0); }

int SLAPI PPObjPersonEvent::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PersonEventTbl::Key0 k0;
		if(_obj == PPOBJ_PERSONOPKIND) {
			PersonEventTbl::Key2 k2;
			MEMSZERO(k2);
			k2.OpID = _id;
			if(P_Tbl->search(2, &k2, spGe) && k2.OpID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
		else if(_obj == PPOBJ_PERSON) {
			PersonEventTbl::Key3 k3;
			MEMSZERO(k3);
			k3.PersonID = _id;
			if(P_Tbl->search(3, &k3, spGe) && P_Tbl->data.PersonID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
			else {
				BExtQuery q(P_Tbl, 0);
				q.select(P_Tbl->ID, 0).where(P_Tbl->SecondID == _id);
				k0.ID = 0;
				if(q.fetchFirst(&k0, spGt) > 0)
					ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
			}
		}
		else if(_obj == PPOBJ_LOCATION) {
			BExtQuery q(P_Tbl, 0);
			q.select(P_Tbl->ID, 0).where(P_Tbl->LocationID == _id);
			k0.ID = 0;
			if(q.fetchFirst(&k0, spGt) > 0)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
		else if(_obj == PPOBJ_BILL) {
			BExtQuery q(P_Tbl, 0);
			q.select(P_Tbl->ID, 0).where(P_Tbl->LinkBillID == _id);
			k0.ID = 0;
			if(q.fetchFirst(&k0, spGt) > 0)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
		else if(_obj == PPOBJ_SCARD) {
			BExtQuery q(P_Tbl, 0);
			q.select(P_Tbl->ID, 0).where(P_Tbl->PrmrSCardID == _id || P_Tbl->ScndSCardID == _id);
			k0.ID = 0;
			if(q.fetchFirst(&k0, spGt) > 0)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
	}
	return DBRPL_OK;
}

SString & SLAPI PPObjPersonEvent::MakeCodeString(const PersonEventTbl::Rec * pRec, SString & rBuf)
{
	PersonTbl::Rec psn_rec;
	PPObjPsnOpKind pok_obj;
	PPPsnOpKind pok_rec;
	rBuf.Z().Cat(pRec->Dt).CatDiv('-', 1).Cat(pRec->Tm);
	if(pok_obj.Search(pRec->OpID, &pok_rec) > 0)
		rBuf.CatDiv('-', 1).Cat(pok_rec.Name);
	if(PsnObj.Fetch(pRec->PersonID, &psn_rec) > 0)
		rBuf.CatDiv('-', 1).Cat(psn_rec.Name);
	return rBuf;
}

int PPObjPersonEvent::SearchPairEvent(PPID evID, int dirArg, PersonEventTbl::Rec * pRec, PersonEventTbl::Rec * pPairRec)
{
	int    ok = -1;
	PPID   pair_id = 0;
	PersonEventTbl::Rec rec, pair_rec;
	if(Search(evID, &rec) > 0) {
		PPObjPsnOpKind pok_obj;
		PPPsnOpKind pok_rec;
		ASSIGN_PTR(pRec, rec);
		if(pok_obj.Fetch(rec.OpID, &pok_rec) > 0 && pok_rec.PairOp) {
			int dir = -1;
			PersonEventCore::PairIdent pi;
			pi.PersonID = rec.PersonID;
			pi.ThisOpID = rec.ID;
			pi.PairOpID = pok_rec.PairOp;
			pi.Dt    = rec.Dt;
			pi.OprNo = rec.OprNo;
			if(dirArg == -1)
				dir = 0;
			else if(dirArg == 1)
				dir = 1;
			else if(dirArg == 0) {
				if(pok_rec.PairType == POKPT_OPEN)
					dir = 1;
				else if(oneof2(pok_rec.PairType, POKPT_CLOSE, POKPT_NULLCLOSE))
					dir = 0;
				else
					dir = -1;
			}
			else if(dirArg == 2)
				dir = 1;
			if(dir >= 0) {
				if(P_Tbl->SearchPair(&pi, dir, &pair_rec) > 0)
					ok = 1;
				else if(dirArg == 2) {
					dir = 0;
					if(P_Tbl->SearchPair(&pi, dir, &pair_rec) > 0)
						ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pPairRec, pair_rec);
	return ok;
}

int SLAPI PPObjPersonEvent::InitPacket(PPPsnEventPacket * pPack, PPID opID, PPID prmrPersonID)
{
	int    ok = 1;
	pPack->Init(opID);
	if(opID) {
		PPObjPsnOpKind pop_obj;
		PPPsnOpKindPacket pop_pack;
		THROW(pop_obj.GetPacket(opID, &pop_pack) > 0);
		if(prmrPersonID) {
			if(pop_pack.PCPrmr.PersonKindID)
				THROW(PsnObj.P_Tbl->IsBelongToKind(prmrPersonID, pop_pack.PCPrmr.PersonKindID));
			pPack->Rec.PersonID = prmrPersonID;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::InitPacket(PPPsnEventPacket * pPack, const AddPersonEventFilt & rFilt, int interactive)
{
	int    ok = 1;
	PPID   op_id = 0;
	PPID   scard_id = 0;
	PPObjPsnOpKind pop_obj;
	PPPsnOpKindPacket pop_pack;
	PPObjPersonKind pk_obj;
	PPPersonKind2 pk_rec;
	SCardTbl::Rec sc_rec;
	if(rFilt.OpID)
		op_id = rFilt.OpID;
	else if(rFilt.OpCode.NotEmpty()) {
		THROW(pop_obj.SearchBySymb(rFilt.OpCode, &op_id, &pop_pack.Rec) > 0);
		op_id = pop_pack.Rec.ID;
	}
	else if(interactive)
		op_id = PPObjPsnOpKind::Select();
	THROW_PP(op_id, PPERR_UNDEFPEOP);
	THROW(pPack->Init(op_id));
	THROW(pop_obj.GetPacket(op_id, &pop_pack) > 0);
	{
		if(rFilt.PrmrSCardID) {
			scard_id = rFilt.PrmrSCardID;
		}
		else if(rFilt.PrmrSCardCode.NotEmpty()) {
			THROW(P_ScObj->SearchCode(0, rFilt.PrmrSCardCode, &sc_rec) > 0);
			scard_id = sc_rec.ID;
		}
		if(scard_id) {
			THROW(P_ScObj->Fetch(scard_id, &sc_rec) > 0);
			THROW_PP_S(sc_rec.PersonID, PPERR_SCARDMUSTHAVOWNER, sc_rec.Code);
			pPack->Rec.PersonID = sc_rec.PersonID;
			pPack->Rec.PrmrSCardID = scard_id;
		}
		if(!pPack->Rec.PersonID) {
			if(rFilt.PrmrPsnID)
				pPack->Rec.PersonID = rFilt.PrmrPsnID;
			else if(rFilt.PrmrPsnCode.NotEmpty()) {
				PPID   reg_type_id = rFilt.PrmrPsnRegTypeID;
				PPID   pk_id = pop_pack.PCPrmr.PersonKindID;
				PPIDArray psn_list;
				if(!reg_type_id) {
					if(pk_id && pk_obj.Fetch(pk_id, &pk_rec) > 0) {
						reg_type_id = pk_rec.CodeRegTypeID;
					}
				}
				if(reg_type_id && PsnObj.GetListByRegNumber(reg_type_id, pk_id, rFilt.PrmrPsnCode, psn_list) > 0) {
					assert(psn_list.getCount());
					pPack->Rec.PersonID = psn_list.get(0);
				}
			}
		}
		if(pPack->Rec.PersonID && pop_pack.PCPrmr.PersonKindID) {
			THROW(PsnObj.P_Tbl->IsBelongToKind(pPack->Rec.PersonID, pop_pack.PCPrmr.PersonKindID));
		}
	}
	{
		scard_id = 0;
		if(rFilt.ScndSCardID) {
			scard_id = rFilt.ScndSCardID;
		}
		else if(rFilt.ScndSCardCode.NotEmpty()) {
			THROW(P_ScObj->SearchCode(0, rFilt.ScndSCardCode, &sc_rec) > 0);
			scard_id = sc_rec.ID;
		}
		if(scard_id) {
			THROW(P_ScObj->Fetch(scard_id, &sc_rec) > 0);
			THROW_PP_S(sc_rec.PersonID, PPERR_SCARDMUSTHAVOWNER, sc_rec.Code);
			pPack->Rec.PersonID = sc_rec.PersonID;
			pPack->Rec.ScndSCardID = scard_id;
		}
		if(!pPack->Rec.SecondID) {
			if(rFilt.ScndPsnID)
				pPack->Rec.SecondID = rFilt.ScndPsnID;
			else if(rFilt.ScndPsnCode.NotEmpty()) {
				PPID   reg_type_id = rFilt.ScndPsnRegTypeID;
				PPID   pk_id = pop_pack.PCScnd.PersonKindID;
				PPIDArray psn_list;
				if(!reg_type_id) {
					if(pk_id && pk_obj.Fetch(pk_id, &pk_rec) > 0) {
						reg_type_id = pk_rec.CodeRegTypeID;
					}
				}
				if(reg_type_id && PsnObj.GetListByRegNumber(reg_type_id, pk_id, rFilt.ScndPsnCode, psn_list) > 0) {
					assert(psn_list.getCount());
					pPack->Rec.SecondID = psn_list.get(0);
				}
			}
		}
		if(pPack->Rec.SecondID && pop_pack.PCScnd.PersonKindID) {
			THROW(PsnObj.P_Tbl->IsBelongToKind(pPack->Rec.SecondID, pop_pack.PCScnd.PersonKindID));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::GetPacket(PPID id, PPPsnEventPacket * pPack)
{
	int    ok = 1;
	RegisterArray regary;
	THROW_INVARG(pPack);
	THROW(pPack->Init(0));
	THROW(Search(id, &pPack->Rec) > 0);
	THROW(RegObj.P_Tbl->GetByEvent(id, &regary));
	if(regary.getCount())
		pPack->Reg = regary.at(0);
	THROW(PPRef->Ot.GetList(Obj, id, &pPack->TagL));
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::TC_SetCalendar(PPID psnID, const PPPsnOpKind * pPok, const PPPsnEventPacket * pPack, const PoClause_ * pClause)
{
	int    ok = -1, r2;
	if(pClause->DirObj) {
		PPID   cal_id = 0;
		// @v10.3.0 (never used) long   dt_val = 0;
		CALDATE cdt;
		StaffCalendarTbl::Rec entry;
		PPObjID oi;
		oi.Set(PPOBJ_PERSON, psnID);
		if(oneof4(pClause->VerbID, POVERB_SETCALENDAR, POVERB_SETCALENDAR_SKIP, POVERB_SETCALCONT, POVERB_SETCALCONT_SKIP)) {
			THROW(StcObj.CreateChild(&cal_id, pClause->DirObj, oi, 0) > 0);
			MEMSZERO(entry);
			entry.CalID  = cal_id;
			entry.ObjID  = pPack->Rec.ID;
			entry.Flags |= STCALEF_BYPEVENT;
			if(oneof2(pClause->VerbID, POVERB_SETCALCONT, POVERB_SETCALCONT_SKIP))
				entry.Flags |= STCALEF_CONTINUOUS;
			if(oneof2(pClause->VerbID, POVERB_SETCALENDAR_SKIP, POVERB_SETCALCONT_SKIP))
				entry.Flags |= STCALEF_SKIP;
			THROW_SL(cdt.SetDate(pPack->Rec.Dt));
			entry.DtVal = cdt;
			entry.TmStart = pPack->Rec.Tm;
			entry.TmVal = 0;
			THROW(StcObj.SetEntry(entry, 0));
			ok = 1;
		}
		else if(pClause->VerbID == POVERB_RESETCALCONT) {
			CALDATE cd;
			THROW(StcObj.CreateChild(&cal_id, pClause->DirObj, oi, 0) > 0);
			cd = pPack->Rec.Dt;
			if(StcObj.SearchContinuousEntry(cal_id, cd, &entry) > 0) {
				THROW(StcObj.RemoveEntry(entry, 0));
				ok = -1;
			}
		}
		else if(oneof2(pClause->VerbID, POVERB_COMPLETECAL, POVERB_COMPLETECAL_SKIP)) {
			if(pPok->PairOp && oneof2(pPok->PairType, POKPT_CLOSE, POKPT_NULLCLOSE)) {
				PersonEventCore::PairIdent pi;
				pi.PersonID = psnID;
				pi.ThisOpID = pPok->ID;
				pi.PairOpID = pPok->PairOp;
				pi.Dt    = pPack->Rec.Dt;
				pi.OprNo = pPack->Rec.OprNo;
				PersonEventTbl::Rec pair_rec;
				THROW(r2 = P_Tbl->SearchPair(&pi, 0, &pair_rec));
				if(r2 > 0) {
					THROW(StcObj.CreateChild(&cal_id, pClause->DirObj, oi, 0) > 0);
					if(pair_rec.Dt == pPack->Rec.Dt) {
						if(pair_rec.Tm && pPack->Rec.Tm) {
							MEMSZERO(entry);
							entry.CalID  = cal_id;
							entry.ObjID  = pPack->Rec.ID;
							entry.Flags |= STCALEF_BYPEVENT;
							if(pClause->VerbID == POVERB_COMPLETECAL_SKIP)
								entry.Flags |= STCALEF_SKIP;
							THROW_SL(cdt.SetDate(pPack->Rec.Dt));
							entry.DtVal = cdt;
							entry.TmStart = pair_rec.Tm;
							entry.TmEnd = pPack->Rec.Tm;
							entry.TmVal = DiffTime(pPack->Rec.Tm, pair_rec.Tm, 3);
							THROW(StcObj.SetEntry(entry, 0));
							ok = 1;
						}
					}
					else {
						for(LDATE dt = pair_rec.Dt; dt <= pPack->Rec.Dt; dt = plusdate(dt, 1)) {
							MEMSZERO(entry);
							entry.CalID  = cal_id;
							entry.ObjID  = pPack->Rec.ID;
							entry.Flags |= STCALEF_BYPEVENT;
							if(pClause->VerbID == POVERB_COMPLETECAL_SKIP)
								entry.Flags |= STCALEF_SKIP;
							THROW_SL(cdt.SetDate(dt));
							entry.DtVal = cdt;
							if(dt == pPack->Rec.Dt) {
								entry.TmEnd = pPack->Rec.Tm;
								entry.TmVal = DiffTime(pPack->Rec.Tm, ZEROTIME, 3);
							}
							else if(dt == pair_rec.Dt) {
								entry.TmStart = pair_rec.Tm;
								entry.TmEnd = encodetime(24, 0, 0, 0);
								entry.TmVal = DiffTime(entry.TmEnd, pair_rec.Tm, 3);
							}
							else {
								entry.TmEnd = encodetime(24, 0, 0, 0);
								entry.TmVal = 24 * 60 * 60;
							}
							THROW(StcObj.SetEntry(entry, 0));
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

enum {
	egdcoThread        = 0x0001,
	dgdcoWaitIdentical = 0x0002
};

int ExecuteGenericDeviceCommand(PPID dvcID, const char * pCmd, long options)
{
	int    ok = 1;
	if(dvcID && !isempty(pCmd)) {
		SString temp_buf;
		PPObjGenericDevice gd_obj;
		PPGenericDevicePacket gd_pack;
		THROW(gd_obj.GetPacket(dvcID, &gd_pack) > 0);
		gd_pack.GetExtStrData(GENDVCEXSTR_ENTRY, temp_buf);
		THROW_PP(temp_buf.NotEmptyS(), PPERR_UNDEFADVCDESCR);
		{
			class EgdcThread : public PPThread {
			public:
				EgdcThread(int deviceClass, const char * pEntryName, const char * pCmd, const char * pMutexName) :
					PPThread(PPThread::kUnknown, pEntryName, 0), Ad(0), MutexName(pMutexName), Valid(1), P_Mutex(0), CmdText(pCmd)
				{
					Ad.PCpb.Cls = deviceClass;
					THROW(CmdText.NotEmptyS());
					THROW(Ad.IdentifyDevice(deviceClass, pEntryName));
					//THROW(Ad.GetDllName(deviceClass, pEntryName, Ad.PCpb.DllName));
					//THROW(Ad.IdentifyDevice(Ad.PCpb.DllName));
					CATCH
						Valid = 0;
					ENDCATCH
				}
				~EgdcThread()
				{
					delete P_Mutex;
				}
				int    operator !() const { return (Valid == 0); }
			private:
				virtual void Run()
				{
					int    ok = 1;
					THROW(Valid);
					if(MutexName.NotEmpty()) {
						P_Mutex = new SMutex(0, MutexName);
					}
					if(P_Mutex && P_Mutex->Wait(10000) > 0) {
						THROW(Ad.RunCmd("INIT", OutArr));
						THROW(Ad.RunCmd(CmdText, OutArr));
						THROW(Ad.RunCmd("RELEASE", OutArr));
					}
					else {
						SString msg_buf;
						(msg_buf = "Mutex").Space().Cat(MutexName).Space().Cat("waiting time exhausted");
						PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					CATCH
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
						ok = 0;
					ENDCATCH
					CALLPTRMEMB(P_Mutex, Release());
				}
				StrAssocArray OutArr;
				PPAbstractDevice Ad;
				SString CmdText;
				int    Valid;
				SString MutexName;
				SMutex * P_Mutex;
			};
			if(options & egdcoThread) {
				//
				// Драйвер устройства должен быть многопоточным, но, вместе с тем, мы не имеем права
				// параллельно обрабатывать более одного запроса к одному устройству.
				// Допускается только параллельная обработка запросов к разным устройствам.
				//
				SString mutex_name;
				(mutex_name = "EgdcMutex").Cat(dvcID);

				EgdcThread * p_thread = new EgdcThread(gd_pack.Rec.DeviceClass, temp_buf, pCmd, mutex_name);
				THROW_MEM(p_thread);
				THROW(*p_thread);
				p_thread->Start(1);
			}
			else {
				StrAssocArray out_arr;
				PPAbstractDevice ad(0);
				ad.PCpb.Cls = gd_pack.Rec.DeviceClass;
				THROW(ad.IdentifyDevice(gd_pack.Rec.DeviceClass, temp_buf));
				THROW(ad.RunCmd("INIT", out_arr));
				THROW(ad.RunCmd(pCmd, out_arr));
				THROW(ad.RunCmd("RELEASE", out_arr));
			}
		}
	}
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER); // @v7.9.9
	ENDCATCH
	return ok;
}
//
// ARG(action IN): @#[PPACN_OBJADD, PPACN_OBJUPD, PPACN_OBJRMV]
//
int SLAPI PPObjPersonEvent::TurnClause(PPPsnEventPacket * pPack, const PPPsnOpKind * pPok, const PoClause_ * pClause, int action, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   psn_id = 0;
	PPID   sc_id = 0;
	if(pClause->Subj == POCOBJ_PRIMARY) {
		psn_id = pPack->Rec.PersonID;
		sc_id = pPack->Rec.PrmrSCardID;
	}
	else if(pClause->Subj == POCOBJ_SECONDARY) {
		psn_id = pPack->Rec.SecondID;
		sc_id = pPack->Rec.ScndSCardID;
	}
	else {
		CALLEXCEPT_PP(PPERR_INVPOCLAUSESUBJ);
	}
	if(psn_id) {
		const  PPID dir_obj = pClause->DirObj;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			switch(pClause->VerbID) {
				case POVERB_ASSIGNKIND:
					ok = (dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) ?
						(PsnObj.P_Tbl->AddKind(psn_id, dir_obj, 0) ? 1 : 0) : -1;
					break;
				case POVERB_REVOKEKIND:
					ok = (dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) ?
						(PsnObj.P_Tbl->RemoveKind(psn_id, dir_obj, 0) ? 1 : 0) : -1;
					break;
				case POVERB_SETTAG:
					ok = -1;
					if(dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) {
						const ObjTagItem * p_tag_item = pPack->TagL.GetItem(dir_obj);
						if(p_tag_item)
							ok = p_ref->Ot.PutTag(PPOBJ_PERSON, psn_id, p_tag_item, 0);
					}
					break;
				case POVERB_REMOVETAG:
					ok = (dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) ?
						(p_ref->Ot.RemoveTag(PPOBJ_PERSON, psn_id, dir_obj, 0) ? 1 : 0) : -1;
					break;
				case POVERB_INCTAG:
				case POVERB_DECTAG:
					ok = -1;
					if(dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJRMV)) {
						ObjTagItem tag_item;
						ok = p_ref->Ot.GetTag(PPOBJ_PERSON, psn_id, dir_obj, &tag_item);
						if(ok) {
							if(ok < 0)
								tag_item.Init(dir_obj);
							double inc_tag = 0.0;
							if(pClause->VerbID == POVERB_INCTAG) {
								if(action == PPACN_OBJADD)
									inc_tag = +1.0;
								else if(action == PPACN_OBJRMV)
									inc_tag = -1.0;
							}
							else if(pClause->VerbID == POVERB_DECTAG) {
								if(action == PPACN_OBJADD)
									inc_tag = -1.0;
								else if(action == PPACN_OBJRMV)
									inc_tag = +1.0;
							}
							if(inc_tag != 0.0 && tag_item.AddReal(inc_tag) > 0)
								ok = p_ref->Ot.PutTag(PPOBJ_PERSON, psn_id, &tag_item, 0);
							else
								ok = -1;
						}
					}
					break;
				case POVERB_ASSIGNPOST:
					if(pPack->Rec.Extra && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) {
						PPID   post_id = 0;
						PPPsnPostPacket pack;
						PersonPostTbl::Key3 k3;
						MEMSZERO(k3);
						k3.PsnEventID = pPack->Rec.ID;
						if(StLObj.P_PostTbl->search(3, &k3, spEq)) {
							post_id = StLObj.P_PostTbl->data.ID;
							THROW(StLObj.GetPostPacket(post_id, &pack) > 0);
							pack.Rec.PersonID = psn_id;
							pack.Rec.StaffID  = pPack->Rec.Extra;
							pack.Rec.Dt       = pPack->Rec.Dt;
							ok = StLObj.PutPostPacket(&post_id, &pack, use_ta);
						}
						else {
							pack.Rec.PersonID = psn_id;
							pack.Rec.StaffID  = pPack->Rec.Extra;
							pack.Rec.Dt       = pPack->Rec.Dt;
							pack.Rec.PsnEventID = pPack->Rec.ID;
							ok = StLObj.PutPostPacket(&post_id, &pack, use_ta);
						}
					}
					else
						ok = -1;
					break;
				case POVERB_REVOKEPOST:
					if(oneof2(action, PPACN_OBJADD, PPACN_OBJUPD))
						ok = BIN(StLObj.RevokeAllPersonPosts(psn_id, pPack->Rec.SecondID, pPack->Rec.Dt, 0));
					else
						ok = -1;
					break;
				case POVERB_ASSIGNREG:
					if(oneof2(action, PPACN_OBJADD, PPACN_OBJUPD) && pPack->Reg.Dt || *pPack->Reg.Num || pPack->Reg.RegTypeID)
						ok = RegObj.P_Tbl->SetByPerson(psn_id, 0, &pPack->Reg, 0) ? 1 : 0;
					else
						ok = -1;
					break;
				case POVERB_REVOKEREG:
					ok = (dir_obj && oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) ?
						BIN(RegObj.P_Tbl->SetByPerson(psn_id, dir_obj, 0, 0)) : -1;
					break;
				case POVERB_SETCALENDAR:
				case POVERB_SETCALENDAR_SKIP:
				case POVERB_COMPLETECAL:
				case POVERB_COMPLETECAL_SKIP:
				case POVERB_SETCALCONT:
				case POVERB_SETCALCONT_SKIP:
				case POVERB_RESETCALCONT:
					if(oneof2(action, PPACN_OBJADD, PPACN_OBJUPD))
						ok = TC_SetCalendar(psn_id, pPok, pPack, pClause);
					else
						ok = -1;
					break;
				case POVERB_CHKSCARDBILLDEBT:
					if(action == PPACN_OBJADD && sc_id && P_ScObj) {
						if(!P_ScObj->CheckExpiredBillDebt(sc_id))
							ok = 0;
					}
					else
						ok = -1;
					break;
				case POVERB_ADDRELATION:
				case POVERB_REVOKERELATION:
					if(pPack->Rec.PersonID && pPack->Rec.SecondID && pClause->DirObj) {
						PPPersonPacket pack;
						if(PsnObj.GetPacket(pPack->Rec.PersonID, &pack, 0) > 0) {
							if(pClause->VerbID == POVERB_ADDRELATION) {
								ok = pack.AddRelation(pPack->Rec.SecondID, pClause->DirObj, 0);
							}
							else {
								ok = pack.RemoveRelation(pPack->Rec.SecondID, pClause->DirObj);
							}
							if(ok > 0) {
								PPID temp_id = pack.Rec.ID;
								ok = PsnObj.PutPacket(&temp_id, &pack, 0);
							}
						}
					}
					break;
				case POVERB_INCSCARDOP:
				case POVERB_DECSCARDOP:
					{
						PPObjID oi;
						oi.Set(PPOBJ_PERSONEVENT, pPack->Rec.ID);
						if(oneof2(action, PPACN_OBJUPD, PPACN_OBJRMV)) {
							TSVector <SCardCore::OpBlock> ex_link_op_list; // @v9.8.4 TSArray-->TSVector
							THROW(P_ScObj->P_Tbl->GetOpByLinkObj(oi, ex_link_op_list));
							for(uint i = 0; i < ex_link_op_list.getCount(); i++) {
								SCardCore::OpBlock & r_ob = ex_link_op_list.at(i);
								THROW(P_ScObj->P_Tbl->RemoveOp(r_ob.SCardID, r_ob.Dtm.d, r_ob.Dtm.t, 0));
							}
						}
						if(oneof2(action, PPACN_OBJADD, PPACN_OBJUPD)) {
							if(sc_id) {
								double value = 0.0;
								int    scst = scstUnkn;
								int    crret = 0;
								double rest = 0.0;
								SCardTbl::Rec sc_rec;
								THROW(P_ScObj->Search(sc_id, &sc_rec) > 0);
								THROW(crret = P_ScObj->CheckRestrictions(&sc_rec, 0, getcurdatetime_())); // @v7.8.2
								if(crret == 2) {
									if(P_ScObj->ActivateRec(&sc_rec) > 0) {
										THROW(P_ScObj->P_Tbl->Update(sc_rec.ID, &sc_rec, 0));
									}
								}
								scst = P_ScObj->GetSeriesType(sc_rec.SeriesID);
								THROW_PP_S(oneof2(scst, scstCredit, scstBonus), PPERR_SCARDMUSTBECRDBNS, sc_rec.Code);
								{
									const  PPID goods_id = pClause->DirObj;
									int    is_quot = 0;
									int    is_timing = 0;
									double amount = 1.0;
									if(goods_id) {
										PPObjGoods goods_obj;
										Goods2Tbl::Rec goods_rec;
										PPObjSCardSeries scs_obj;
										PPSCardSeries2 scs_rec;
										if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
											double quot = 0.0;
											if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.QuotKindID_s) {
												QuotIdent qi(getcurdate_(), pPack->Rec.LocationID, scs_rec.QuotKindID_s);
												if(goods_obj.GetQuotExt(goods_id, qi, &quot, 1) > 0)
													is_quot = 1;
											}
											if(!is_quot) {
												QuotIdent qi(getcurdate_(), pPack->Rec.LocationID, PPQUOTK_BASE);
												if(goods_obj.GetQuotExt(goods_id, qi, &quot, 1) > 0)
													is_quot = 1;
											}
											if(is_quot || pClause->CmdText.NotEmpty()) {
												double qtty = 1.0;
												double ratio = 0.0;
												// @v9.9.12 PPUnit unit_rec;
												// @v9.9.12 if(goods_obj.FetchUnit(goods_rec.UnitID, &unit_rec) > 0 && unit_rec.BaseUnitID == PPUNT_SECOND && unit_rec.BaseRatio) {
													// @v9.9.12 ratio = unit_rec.BaseRatio;
												if(goods_obj.TranslateGoodsUnitToBase(goods_rec, PPUNT_SECOND, &ratio) > 0) { // @v9.9.12
													if(ratio > 0.0) {
														PersonEventCore::PairIdent pi;
														pi.PersonID = psn_id;
														pi.ThisOpID = pPok->ID;
														pi.PairOpID = pPok->PairOp;
														pi.Dt    = pPack->Rec.Dt;
														pi.OprNo = pPack->Rec.OprNo;
														PersonEventTbl::Rec pair_rec;
														int    r2 = P_Tbl->SearchPair(&pi, 0, &pair_rec);
														THROW(r2);
														if(r2 > 0) {
															LDATETIME dtm1, dtm2;
															dtm1.Set(pPack->Rec.Dt, pPack->Rec.Tm);
															dtm2.Set(pair_rec.Dt, pair_rec.Tm);
															long   numdays = 0;
															long   sec = labs(diffdatetime(dtm1, dtm2, 3, &numdays));
															qtty = (sec + labs(numdays) * 3600 * 24) / ratio;
														}
													}
												}
												if(pClause->CmdText.NotEmpty()) {
													GoodsContext::Param gcp;
													gcp.GoodsID = goods_rec.ID;
													gcp.Qtty = qtty;
													GoodsContext gc(gcp);
													double expr_result = 0.0;
													if(PPExprParser::CalcExpression(pClause->CmdText, &expr_result, 0, &gc))
														amount = R0(expr_result);
													else
														amount = R0(quot * qtty);
												}
												else
													amount = R0(quot * qtty); // @v8.7.10 R0
											}
										}
									}
									value = (pClause->VerbID == POVERB_INCSCARDOP) ? amount : -amount;
								}
								if(value != 0.0) {
									THROW(fabs(value) >= 0.00001); // @todo @err
									THROW(P_ScObj->P_Tbl->GetRest(sc_rec.ID, ZERODATE, &rest));
									rest += sc_rec.MaxCredit;
									THROW_PP_S(value > 0.0 || (rest+value) >= 0.0, PPERR_SCARDRESTNOTENOUGH, sc_rec.Code); // @v8.2.3 (>0.0)-->(>=0.0)
									{
										SCardCore::OpBlock ob;
										ob.SCardID = sc_rec.ID;
										ob.LinkOi = oi;
										ob.Amount = value;
										THROW(P_ScObj->P_Tbl->PutOpBlk(ob, 0, 0));
										pPack->Otb.SCardID = sc_rec.ID;
										pPack->Otb.SCardWrOffAmount = -value;
										pPack->Otb.SCardRest = rest+value;
									}
								}
							}
						}
					}
					break;
				case POVERB_DEVICECMD:
					if(oneof2(action, PPACN_OBJADD, PPACN_PERSONEVENTREDO) && dir_obj && pClause->CmdText.NotEmpty()) {
						long   options = 0;
						const char * p = pClause->CmdText;
						while(*p == ' ' || *p == '\t')
							p++;
						while(*p == '/') {
							p++;
							if(strnicmp(p, "MT", 2) == 0) {
								options |= egdcoThread;
								p += 2;
							}
							while(*p == ' ' || *p == '\t')
								p++;
						}
						THROW(ExecuteGenericDeviceCommand(dir_obj, p, options));
					}
					break;
				case POVERB_STYLODISPLAY:
					if(oneof2(action, PPACN_OBJADD, PPACN_PERSONEVENTREDO) && dir_obj) {
						if(!pPack->Otb.P_Pdb) { // Более одного действия такого типа пока не допускаем
							pPack->Otb.P_Pdb = new PalmDisplayBlock;
							pPack->Otb.P_Pdb->DvcID = dir_obj;
							pPack->Otb.P_Pdb->Ctx = PalmDisplayBlock::ctxPersonEvent;
						}
					}
					break;
				case POVERB_BEEP:
					if(oneof2(action, PPACN_OBJADD, PPACN_PERSONEVENTREDO)) {
						const uint   def_freq = 500;
						const uint   def_dur = 200;
						int    default_beep = 1;
						if(pClause->CmdText.NotEmpty()) {
							SString note_buf, freq_buf, dur_buf;
							PPGetFilePath(PPPATH_DD, pClause->CmdText, freq_buf);
							if(fileExists(freq_buf)) {
								if(::PlaySound(SUcSwitch(freq_buf), 0, SND_FILENAME|SND_ASYNC)) // @unicodeproblem
									default_beep = 0;
							}
							else {
								StringSet ss(';', pClause->CmdText);
								for(uint np = 0; ss.get(&np, note_buf);) {
									uint   freq = 0;
									uint   dur = 0;
									if(note_buf.Divide(',', freq_buf, dur_buf) > 0) {
										freq = (uint)freq_buf.ToLong();
										dur = (uint)dur_buf.ToLong();
									}
									else
										freq = (uint)freq_buf.ToLong();
									Beep(NZOR(freq, def_freq), NZOR(dur, def_dur));
									default_beep = 0;
								}
							}
						}
						if(default_beep) {
							Beep(def_freq, def_dur);
						}
					}
					break;
				default:
					{
						CALLEXCEPT_PP(PPERR_INVPOCLAUSEVERB);
					}
			}
			THROW(ok);
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

struct FrwdPsnEventItem {
	LDATE  Dt;
	long   OprNo;
	PPID   ID;
};

int SLAPI PPObjPersonEvent::GetFrwdList(PPID psnID, int isPrmr, LDATE dt, long oprno, SArray * pList)
{
	int    ok = 1;
	int    idx = isPrmr ? 3 : 1;
	DBQ * dbq = 0;
	union {
		PersonEventTbl::Key1 k1;
		PersonEventTbl::Key3 k3;
	} k;
	MEMSZERO(k);
	BExtQuery q(P_Tbl, idx);
	q.select(P_Tbl->ID, P_Tbl->Dt, P_Tbl->OprNo, P_Tbl->PersonID, P_Tbl->SecondID, 0L);
	if(isPrmr) {
		dbq = & (P_Tbl->PersonID == psnID && P_Tbl->Dt >= dt);
		k.k3.PersonID = psnID;
		k.k3.Dt = dt;
		k.k3.OprNo = oprno;
	}
	else {
		dbq = & (P_Tbl->Dt >= dt && P_Tbl->SecondID == psnID);
		k.k1.Dt = dt;
		k.k1.OprNo = oprno;
	}
	q.where(*dbq);
	for(q.initIteration(0, &k, spGt); q.nextIteration();) {
		FrwdPsnEventItem item;
		MEMSZERO(item);
		item.Dt    = P_Tbl->data.Dt;
		item.OprNo = P_Tbl->data.OprNo;
		item.ID    = P_Tbl->data.ID;
		if(item.Dt > dt || (item.Dt == dt && item.OprNo > oprno))
			if(!pList->lsearch(&item.ID, 0, CMPF_LONG, offsetof(FrwdPsnEventItem, ID)))
				pList->insert(&item);
	}
	pList->sort(PTR_CMPFUNC(_2long));
	return ok;
}

int SLAPI PPObjPersonEvent::GetFrwdList(PPID psnID, LDATE dt, long oprno, SArray * pList)
{
	if(psnID)
		return (GetFrwdList(psnID, 1, dt, oprno, pList) && GetFrwdList(psnID, 0, dt, oprno, pList));
	else
		return -1;
}

int SLAPI PPObjPersonEvent::Helper_PutPacket(PPID evID, int action, PPPsnEventPacket * pPack, PPPsnOpKindPacket * pPokPack)
{
	int    ok = 1;
	const  int redo = BIN(action == PPACN_PERSONEVENTREDO);
	uint   i = 0;
	if(!redo) {
		if(pPokPack->Rec.PairOp && oneof3(pPokPack->Rec.PairType, POKPT_OPEN, POKPT_CLOSE, POKPT_NULLCLOSE)) {
			//
			// Проверка парности события //
			//
			SString msg_buf;
			int    forward = 1;
			PersonEventCore::PairIdent pi;
			pi.PersonID = pPack->Rec.PersonID;
			pi.ThisOpID = pPokPack->Rec.ID;
			pi.PairOpID = pPokPack->Rec.PairOp;
			pi.Flags   |= PersonEventCore::PairIdent::fSignalNpError;
			if(oneof2(pPokPack->Rec.PairType, POKPT_CLOSE, POKPT_NULLCLOSE))
				forward = 0;
			else // if(PokPack.Rec.PairType == POKPT_OPEN)
				forward = 1;
			pi.Dt = pPack->Rec.Dt;
			pi.OprNo = pPack->Rec.OprNo;
			PersonEventTbl::Rec pair_rec;
			int    r = P_Tbl->SearchPair(&pi, forward, &pair_rec);
			if(r == 0) {
				if(PPErrCode == PPERR_NONPAIRPSNEVNT)
					PPSetAddedMsgString(MakeCodeString(&pair_rec, msg_buf));
				CALLEXCEPT();
			}
			else if(r < 0) {
				THROW_PP_S(pPokPack->Rec.PairType != POKPT_CLOSE, PPERR_NONPAIRPSNEVNT,
					MakeCodeString(&pPack->Rec, msg_buf));
			}
			if(pPokPack->Rec.PairType == POKPT_OPEN) {
				r = P_Tbl->SearchPair(&pi, 0 /* backward */, &pair_rec);
				if(r == 0) {
					if(PPErrCode == PPERR_NONPAIRPSNEVNT)
						PPSetAddedMsgString(MakeCodeString(&pair_rec, msg_buf));
					CALLEXCEPT();
				}
			}
			else if(r > 0) {
				pPokPack->Rec.Flags |= PSNEVF_CLOSE;
			}
		}
		if(*strip(pPack->Reg.Num)) {
			RegisterArray regary;
			pPack->Reg.ObjType = 0;
			pPack->Reg.ObjID = 0;
			pPack->Reg.PsnEventID = evID;
			THROW_SL(regary.insert(&pPack->Reg));
			THROW(RegObj.P_Tbl->PutByEvent(evID, &regary, 0));
			pPack->Reg.ID = regary.at(0).ID;
		}
		THROW(PPRef->Ot.PutList(Obj, evID, &pPack->TagL, 0));
		THROW(StcObj.RemoveEntriesByPsnEvent(evID, 0));
		if(pPack->LinkFiles.IsChanged(evID, 0L)) {
			THROW_PP(CheckRights(PSNRT_UPDIMAGE), PPERR_NRT_UPDIMAGE);
			pPack->LinkFiles.Save(evID, 0L);
		}
	}
	{
		PoClause_ clause;
		for(i = 0; i < pPokPack->ClauseList.GetCount(); i++) {
			pPokPack->ClauseList.Get(i, clause);
			if(!redo || (clause.Flags & PoClause_::fOnRedo)) {
				THROW(TurnClause(pPack, &pPokPack->Rec, &clause, action, 0));
				ok = 2;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::CheckRestrictions(const PPPsnEventPacket * pPack, PPID personID, PPID scardID, const PPPsnOpKindPacket::PsnConstr * pConstr)
{
	int    ok = 1;
	SString msg_buf, temp_buf;
	if(pConstr && personID) {
		PersonTbl::Rec psn_rec; // PPObjPerson
		THROW(PsnObj.Fetch(personID, &psn_rec) > 0);
		if(pConstr->PersonKindID) {
			THROW(PsnObj.P_Tbl->IsBelongToKind(personID, pConstr->PersonKindID));
		}
		if(pConstr->StatusType) {
			if(psn_rec.Status) {
				PPObjPersonStatus ps_obj;
				PPPersonStatus st_rec;
				THROW(ps_obj.Fetch(psn_rec.Status, &st_rec) > 0);
				THROW_PP_S(pConstr->StatusType == PSNSTT_PRIVATE && (st_rec.Flags & PSNSTF_PRIVATE), PPERR_PSNEV_NPRIVATE, psn_rec.Name);
				THROW_PP_S(pConstr->StatusType == PSNSTT_LEGAL && !(st_rec.Flags & PSNSTF_PRIVATE), PPERR_PSNEV_NLEGAL, psn_rec.Name);
			}
		}
		if(pConstr->RestrictTagID) {
			ObjTagItem tag;
			PPObjTag tag_obj;
			double val = 0.0;
			PPObjectTag tag_rec;
			if(tag_obj.Fetch(pConstr->RestrictTagID, &tag_rec) > 0)
				temp_buf = tag_rec.Name;
			else
				ideqvalstr(pConstr->RestrictTagID, temp_buf);
			(msg_buf = psn_rec.Name).CatDiv('-', 1).Cat(temp_buf);
			THROW_PP_S(PPRef->Ot.GetTag(PPOBJ_PERSON, personID, pConstr->RestrictTagID, &tag) > 0, PPERR_PSNEV_TAGABS, msg_buf);
			tag.GetReal(&val);
			THROW_PP_S(val > 0.0, PPERR_PSNEV_TAGVAL, msg_buf);
		}
		if(scardID && pConstr->RestrictScSerList.getCount()) {
			SCardTbl::Rec sc_rec;
			if(P_ScObj->Fetch(scardID, &sc_rec) > 0) {
				if(!pConstr->RestrictScSerList.lsearch(sc_rec.SeriesID)) {
					PPObjSCardSeries scs_obj;
					PPSCardSeries scs_rec;
					if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0)
						temp_buf = scs_rec.Name;
					else
						ideqvalstr(sc_rec.SeriesID, temp_buf);
					CALLEXCEPT_PP_S(PPERR_PSNEV_ILLEGALSCSER, temp_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::CheckRestrictions(const PPPsnEventPacket * pPack, const PPPsnOpKindPacket * pPokPack)
{
	int    ok = 1;
	PPPsnOpKindPacket pok_pack;
	if(!pPokPack) {
		PPObjPsnOpKind pok_obj;
		THROW(pok_obj.GetPacket(pPack->Rec.OpID, &pok_pack) > 0);
		pPokPack = &pok_pack;
	}
	if(pPokPack->Rec.RestrStaffCalID) {
		PPObjStaffCal stc_obj;
		PPStaffCal stc_rec;
		THROW(stc_obj.Search(pPokPack->Rec.RestrStaffCalID, &stc_rec) > 0);
		{
			ScObjAssoc scoa;
			STimeChunkArray worktime_list;
			DateRange period;
			period.SetDate(pPack->Rec.Dt);
			stc_obj.InitScObjAssoc(pPokPack->Rec.RestrStaffCalID, 0, pPack->Rec.PersonID, &scoa);
			THROW(stc_obj.CalcPeriod(scoa, period, 0, 0, 0, &worktime_list));
			worktime_list.Sort();
			{
				uint   slot_pos = 0;
				LDATETIME moment;
				moment.Set(pPack->Rec.Dt, pPack->Rec.Tm);
				for(uint i = 0; !slot_pos && i < worktime_list.getCount(); i++) {
					const STimeChunk & r_chunk = *static_cast<const STimeChunk *>(worktime_list.at(i));
					if(r_chunk.Has(moment)) {
						slot_pos = i+1;
					}
				}
				THROW_PP_S(slot_pos, PPERR_PSNEV_MISSCALENTRY, stc_rec.Name);
				if(pPack->Rec.ID == 0 && pPokPack->Rec.RscMaxTimes > 0) {
					const STimeChunk & r_chunk = *static_cast<const STimeChunk *>(worktime_list.at(slot_pos-1));
					uint   _c = 0;
					SString temp_buf;
					P_Tbl->CalcCountForPeriod(pPack->Rec.OpID, pPack->Rec.PersonID, r_chunk, &_c);
					THROW_PP_S(_c < (uint)pPokPack->Rec.RscMaxTimes, PPERR_PSNEV_OVERCOUNTCALENTRY, temp_buf.Z().Cat((long)pPokPack->Rec.RscMaxTimes));
				}
			}
		}
	}
	THROW(CheckRestrictions(pPack, pPack->Rec.PersonID, pPack->Rec.PrmrSCardID, &pPokPack->PCPrmr));
	THROW(CheckRestrictions(pPack, pPack->Rec.SecondID, pPack->Rec.ScndSCardID, &pPokPack->PCScnd));
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::PutPacket(PPID * pID, PPPsnEventPacket * pPack, int use_ta)
{
	int    ok = 1;
	uint   i, j;
	LDATE  op_dt = ZERODATE;
	long   op_no = 0;
	PPID   id = DEREFPTRORZ(pID);
	const  PPID org_id = id;
	PPID   prmr_id = 0;
	PPID   scnd_id = 0;
	PPID   log_action_id = 0;
	SString temp_buf;
	PersonEventTbl::Rec rec;
	PPPsnOpKindPacket pok_pack;
	PPObjPsnOpKind    pok_obj;
	SArray frwd_list(sizeof(FrwdPsnEventItem));
	if(pPack) {
		THROW(pok_obj.GetPacket(pPack->Rec.OpID, &pok_pack) > 0);
		THROW(CheckRestrictions(pPack, &pok_pack));
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(id) {
			THROW(Search(id, &rec) > 0);
			op_dt = rec.Dt;
			op_no = rec.OprNo;
			prmr_id = rec.PersonID;
			scnd_id = rec.SecondID;
			if(pPack == 0) {
				//
				// Удаление пакета
				//
				PPPsnEventPacket temp_pack;
				THROW(GetPacket(*pID, &temp_pack) > 0);
				THROW(pok_obj.GetPacket(temp_pack.Rec.OpID, &pok_pack) > 0);
				{
					PoClause_ clause;
					for(i = 0; i < pok_pack.ClauseList.GetCount(); i++) {
						pok_pack.ClauseList.Get(i, clause);
						THROW(TurnClause(&temp_pack, &pok_pack.Rec, &clause, PPACN_OBJRMV, 0));
					}
				}
				THROW(P_Tbl->Remove(id, 0));
				THROW(RegObj.P_Tbl->PutByEvent(id, 0, 0));
				THROW(PPRef->Ot.PutList(Obj, id, 0, 0));
				THROW(StcObj.RemoveEntriesByPsnEvent(id, 0));
				{
					ObjLinkFiles _lf(Obj);
					_lf.Save(id, 0L);
				}
				log_action_id = PPACN_OBJRMV;
			}
			else {
				//
				// Изменение пакета
				//
				if(rec.Dt != pPack->Rec.Dt) {
					THROW(IncDateKey(P_Tbl, 1, pPack->Rec.Dt, &pPack->Rec.OprNo));
				}
				THROW(P_Tbl->Update(id, &pPack->Rec, 0));
				log_action_id = PPACN_OBJUPD;
				THROW(Helper_PutPacket(id, log_action_id, pPack, &pok_pack));
			}
		}
		else if(pPack) {
			int    r = 0;
			//
			// Добавление пакета
			//
			log_action_id = PPACN_OBJADD;
			//
			// Прежде всего необходимо выяснить не следует ли сделать операцию Redo
			//
			if(pok_pack.Rec.RedoTimeout > 0) {
				PersonEventCore::PairIdent pi;
				pi.PersonID = pPack->Rec.PersonID;
				pi.SCardID  = pPack->Rec.PrmrSCardID;
				pi.ThisOpID = pok_pack.Rec.ID;
				pi.Flags   |= (PersonEventCore::PairIdent::fSignalAnalog|PersonEventCore::PairIdent::fUseSCard);
				pi.Dt = pPack->Rec.Dt;
				long   oprno = 0;
				THROW(IncDateKey(P_Tbl, 1, pPack->Rec.Dt, &oprno));
				pi.OprNo = oprno;
				PersonEventTbl::Rec pair_rec;
				r = P_Tbl->SearchPair(&pi, 0 /* backward */, &pair_rec);
				if(r == 2) {
					const LDATETIME ct = getcurdatetime_();
					LDATETIME pt;
					pt.Set(pair_rec.Dt, pair_rec.Tm);
					long   diffsec = diffdatetimesec(ct, pt);
					if(diffsec >= 0 && diffsec <= pok_pack.Rec.RedoTimeout) {
						log_action_id = PPACN_PERSONEVENTREDO;
						pPack->Rec = pair_rec;
						id = pair_rec.ID;
					}
				}
			}
			if(log_action_id != PPACN_PERSONEVENTREDO) {
				pPack->Rec.OprNo = 0;
				THROW(P_Tbl->Add(&id, &pPack->Rec, 0));
				pPack->Rec.ID = id;
			}
			op_dt = pPack->Rec.Dt;
			op_no = MAXLONG;
			prmr_id = pPack->Rec.PersonID;
			scnd_id = pPack->Rec.SecondID;
			THROW(r = Helper_PutPacket(id, log_action_id, pPack, &pok_pack));
			if(r < 0)
				log_action_id = 0;
		}
		//
		// Перепроводка действий по форвардным операциям
		//
		if(log_action_id) {
			if(log_action_id != PPACN_PERSONEVENTREDO) {
				THROW(GetFrwdList(prmr_id, op_dt, op_no, &frwd_list));
				THROW(GetFrwdList(scnd_id, op_dt, op_no, &frwd_list));
				for(i = 0; i < frwd_list.getCount(); i++) {
					const FrwdPsnEventItem * p_item = (FrwdPsnEventItem *)frwd_list.at(i);
					PPPsnEventPacket temp_pack;
					if(GetPacket(p_item->ID, &temp_pack) > 0) {
						PoClause_ clause;
						THROW(pok_obj.GetPacket(temp_pack.Rec.OpID, &pok_pack) > 0);
						for(j = 0; j < pok_pack.ClauseList.GetCount(); j++) {
							pok_pack.ClauseList.Get(j, clause);
							THROW(TurnClause(&temp_pack, &pok_pack.Rec, &clause, PPACN_OBJUPD, 0));
						}
					}
				}
			}
			DS.LogAction(log_action_id, Obj, id, 0, 0);
		}
		THROW(tra.Commit());
	}
	if(pPack && pPack->Otb.P_Pdb) {
		SString fmt_buf, msg_buf;
		SCardTbl::Rec sc_rec;
		PPFormatT(PPTXT_PSNEVTURNINFO, &msg_buf, pPack->Rec.OpID, pPack->Rec.PersonID);
		if(pPack->Otb.SCardID) {
			if(P_ScObj->Search(pPack->Otb.SCardID, &sc_rec) > 0) {
				PPLoadString("card", temp_buf);
				msg_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(sc_rec.Code);
				if(pPack->Otb.SCardWrOffAmount != 0.0) {
					if(pPack->Otb.SCardWrOffAmount > 0.0) {
						PPLoadString("writtenoff", temp_buf);
						msg_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(pPack->Otb.SCardWrOffAmount, SFMT_MONEY);
					}
					else if(pPack->Otb.SCardWrOffAmount < 0.0) {
						PPLoadString("credited", temp_buf);
						msg_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(pPack->Otb.SCardWrOffAmount, SFMT_MONEY);
					}
					PPLoadString("rest", temp_buf);
					msg_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(pPack->Otb.SCardRest, SFMT_MONEY);
				}
			}
		}
		else if(pPack->Rec.PrmrSCardID) {
			if(P_ScObj->Search(pPack->Rec.PrmrSCardID, &sc_rec) > 0) {
				PPLoadString("card", temp_buf);
				msg_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(sc_rec.Code);
			}
		}
		pPack->Otb.P_Pdb->DirectMsg = msg_buf;
		PPObjStyloPalm::PutDisplayBlock(*pPack->Otb.P_Pdb);
	}
	ASSIGN_PTR(pID, id);
	CATCH
		{
			PoClause_ clause;
			for(i = 0; i < pok_pack.ClauseList.GetCount(); i++) {
				pok_pack.ClauseList.Get(i, clause);
				if(clause.VerbID == POVERB_STYLODISPLAY && clause.DirObj) {
					PPGetLastErrorMessage(1, temp_buf);
					PalmDisplayBlock pdb;
					pdb.DvcID = clause.DirObj;
					pdb.Ctx = pdb.ctxPersonEvent;
					pdb.Flags |= pdb.fError;
					pdb.DirectMsg = temp_buf;
					PPObjStyloPalm::PutDisplayBlock(pdb);
					break;
				}
			}
		}
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
#define GRP_IBG          1
#define GRP_PERSON_PRMR  2
#define GRP_PERSON_SCND  3

// static
int SLAPI PsnEventDialog::GetParam(PPID pokID, Param * pParam)
{
	int    ok = 1;
	PPPsnOpKindPacket pok_pack;
	PPObjPsnOpKind    pok_obj;
	THROW_INVARG(pParam != 0);
	memzero(pParam, sizeof(*pParam));
	THROW(pok_obj.GetPacket(pokID, &pok_pack) > 0);
	STRNSCPY(pParam->DlgTitle, pok_pack.Rec.Name);
	pParam->ExValGrp = pok_pack.Rec.ExValGrp;
	CATCHZOK
	return ok;
}

PsnEventDialog::PsnEventDialog(Param * pParam, PPObjPersonEvent * pPeObj) : PPListDialog(DLG_PSNEVNT, CTL_PSNEVNT_TAGLIST), P_PeObj(pPeObj), P(*pParam)
{
	setTitle(P.DlgTitle);
	SetupCalDate(CTLCAL_PSNEVNT_DATE, CTL_PSNEVNT_DATE);
	addGroup(GRP_IBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_PSNEVNT_IMAGE,
		cmAddImage, cmDelImage, P_PeObj->CheckRights(PSNRT_UPDIMAGE)));
	addGroup(GRP_PERSON_PRMR, new PersonCtrlGroup(CTLSEL_PSNEVNT_PRMR, CTL_PSNEVNT_PRMRSCARD, 0, PersonCtrlGroup::fCanInsert/*|PersonCtrlGroup::fLoadDefOnOpen*/));
	addGroup(GRP_PERSON_SCND, new PersonCtrlGroup(CTLSEL_PSNEVNT_SCND, CTL_PSNEVNT_SCNDSCARD, 0, PersonCtrlGroup::fCanInsert/*|PersonCtrlGroup::fLoadDefOnOpen*/));
	SetupInputLine(CTL_PSNEVNT_MEMO, MKSTYPE(S_ZSTRING, 512), MKSFMT(512, 0)); // @v10.2.3
}

int PsnEventDialog::setupList()
{
	const  ObjTagItem * p_item = 0;
	PPObjTag objtag;
	PPObjectTag tag;
	SString buf;
	for(uint i = 0; (p_item = Pack.TagL.EnumItems(&i)) != 0;) {
		StringSet ss(SLBColumnDelim);
		if(objtag.Fetch(p_item->TagID, &tag) > 0)
			buf = tag.Name;
		else
			buf.Z().Cat(p_item->TagID);
		ss.add(buf);
		TagObj.GetCurrTagVal(p_item, buf);
		ss.add(buf);
		if(!addStringToList(p_item->TagID, ss.getBuf()))
			return 0;
	}
	return 1;
}

int PsnEventDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	ObjTagItem item;
	if(EditObjTagItem(Pack.TagL.ObjType, Pack.TagL.ObjID, &item, 0) > 0) {
		Pack.TagL.PutItem(item.TagID, &item);
		ASSIGN_PTR(pPos, Pack.TagL.GetCount());
		ASSIGN_PTR(pID, item.TagID);
		ok = 1;
	}
	return ok;
}

int PsnEventDialog::editItem(long pos, long id)
{
	int    ok = -1;
	const  ObjTagItem * p_item = Pack.TagL.GetItemByPos(static_cast<uint>(pos));
   	if(p_item && p_item->TagID == id)
		if(EditObjTagItem(Pack.TagL.ObjType, Pack.TagL.ObjID, const_cast<ObjTagItem *>(p_item), 0) > 0) // @badcast
			ok = 1;
	return ok;
}

int PsnEventDialog::delItem(long pos, long id)
{
	return id ? Pack.TagL.PutItem(id, 0) : 0;
}

void PsnEventDialog::editRegister()
{
	RegisterTbl::Rec regrec = Pack.Reg;
	SETIFZ(regrec.RegTypeID, PokPack.Rec.RegTypeID);
	SETIFZ(regrec.Dt, getCtrlDate(CTL_PSNEVNT_DATE));
	if(P_PeObj->RegObj.EditDialog(&regrec, 0, (const PPPersonPacket *)0) > 0)
		Pack.Reg = regrec;
}

IMPL_HANDLE_EVENT(PsnEventDialog)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND)
		if(TVCMD == cmPsnEvntRegister)
			editRegister();
		else if(event.isCbSelected(CTLSEL_PSNEVNT_PRMR))
			setupPair();
		else if(event.isCbSelected(CTLSEL_PSNEVNT_DIV) || event.isCbSelected(CTLSEL_PSNEVNT_SCND))
			setupPost(1);
		else if(TVCMD == cmPrint)
			Print();
		else
			return;
	else
		return;
	clearEvent(event);
}

int PsnEventDialog::setupPost(int fromDialog)
{
	int    ok = 1;
	if(P.ExValGrp == POKEVG_POST) {
		PPID   employer = Pack.Rec.SecondID;
		PPID   div_id   = 0;
		PPID   post_id  = Pack.Rec.Extra;
		if(fromDialog) {
			getCtrlData(CTLSEL_PSNEVNT_SCND, &employer);
			getCtrlData(CTLSEL_PSNEVNT_DIV,  &div_id);
			getCtrlData(CTLSEL_PSNEVNT_POST, &post_id);
		}
		if(employer) {
			PPObjStaffList staffl_obj;
			if(post_id && !fromDialog) {
				PPStaffEntry sl_rec;
				if(staffl_obj.Search(post_id, &sl_rec) > 0)
					div_id = sl_rec.DivisionID;
			}
			SetupLocationCombo(this, CTLSEL_PSNEVNT_DIV, div_id, 0, LOCTYP_DIVISION, employer);
			SetupStaffListCombo(this, CTLSEL_PSNEVNT_POST, post_id, OLW_CANINSERT, employer, div_id);
		}
		else {
			setCtrlLong(CTLSEL_PSNEVNT_DIV,  div_id = 0);
			setCtrlLong(CTLSEL_PSNEVNT_POST, post_id = 0);
		}
	}
	return ok;
}

int PsnEventDialog::setupPair()
{
	int    ok = -1;
	SString line_buf;
	PPID   psn_id = getCtrlLong(CTLSEL_PSNEVNT_PRMR);
	if(PokPack.Rec.PairOp && oneof3(PokPack.Rec.PairType, POKPT_OPEN, POKPT_CLOSE, POKPT_NULLCLOSE) && !(Pack.Rec.Flags & PSNEVF_FORCEPAIR)) {
		int    forward = 1;
		PersonEventCore::PairIdent pi;
		pi.PersonID = psn_id;
		pi.ThisOpID = PokPack.Rec.ID;
		pi.PairOpID = PokPack.Rec.PairOp;
		pi.Flags   |= PersonEventCore::PairIdent::fSignalNpError;
		if(oneof2(PokPack.Rec.PairType, POKPT_CLOSE, POKPT_NULLCLOSE))
			forward = 0;
		else // if(PokPack.Rec.PairType == POKPT_OPEN)
			forward = 1;
		pi.Dt = getCtrlDate(CTL_PSNEVNT_DATE);
		if(Pack.Rec.ID && pi.Dt == Pack.Rec.Dt)
			pi.OprNo = Pack.Rec.OprNo;
		else if(forward) {
			long   oprno = 0;
			IncDateKey(P_PeObj->P_Tbl, 1, pi.Dt, &oprno);
			pi.OprNo = oprno;
		}
		else
			pi.OprNo = MAXLONG;
		PersonEventTbl::Rec pair_rec;
		int    r = P_PeObj->P_Tbl->SearchPair(&pi, forward, &pair_rec);
		if(r > 0) {
			P_PeObj->MakeCodeString(&pair_rec, line_buf);
			LDATETIME dtm1, dtm2;
			dtm1.Set(Pack.Rec.Dt, Pack.Rec.Tm);
			dtm2.Set(pair_rec.Dt, pair_rec.Tm);
			LTIME  dur;
			long   numdays = 0;
			dur.settotalsec(labs(diffdatetime(dtm1, dtm2, 3, &numdays)));
			line_buf.CatDiv(';', 2);
			if(numdays)
				line_buf.Cat(labs(numdays)).Space().Cat("days").Space();
			line_buf.Cat(dur);
			ok = 1;
		}
		else if(r == 0) {
			if(PPErrCode == PPERR_NONPAIRPSNEVNT) {
				SString msg_buf;
				PPSetAddedMsgString(P_PeObj->MakeCodeString(&pair_rec, msg_buf));
				PPError();
			}
			ok = 0;
		}
	}
	setStaticText(CTL_PSNEVNT_ST_PAIR, line_buf);
	return ok;
}

int PsnEventDialog::setDTS(const PPPsnEventPacket * p)
{
	int    ok = 1;
	int    disable_taglist = 0, disable_post = 0;
	PPObjPsnOpKind pok_obj;
	THROW_INVARG(p);
	THROW_PP(p->Rec.OpID, PPERR_UNDEFPEOP);
	THROW(pok_obj.GetPacket(p->Rec.OpID, &PokPack) > 0);
	THROW(PokPack.CheckExVal());
	Pack = *p;
	disable_taglist = BIN(P.ExValGrp != POKEVG_TAG);
	disable_post    = BIN(P.ExValGrp != POKEVG_POST);
	if(Pack.Rec.ID == 0) {
		SETIFZ(Pack.Rec.SecondID, PokPack.PCScnd.DefaultID);
	}
	{
		PersonCtrlGroup::Rec rec;
		rec.PersonID = Pack.Rec.PersonID;
		rec.PsnKindID = PokPack.PCPrmr.PersonKindID;
		rec.SCardID = Pack.Rec.PrmrSCardID;
		setGroupData(GRP_PERSON_PRMR, &rec);
	}
	{
		PersonCtrlGroup::Rec rec;
		rec.PersonID = Pack.Rec.SecondID;
		rec.PsnKindID = PokPack.PCScnd.PersonKindID;
		rec.SCardID = Pack.Rec.ScndSCardID;
		setGroupData(GRP_PERSON_SCND, &rec);
	}
	if(P.ExValGrp == POKEVG_POST) {
		setupPost(0);
	}
	setCtrlDatetime(CTL_PSNEVNT_DATE, CTL_PSNEVNT_TIME, Pack.Rec.Dt, Pack.Rec.Tm);
	setCtrlData(CTL_PSNEVNT_MEMO, Pack.Rec.Memo);
	disableCtrl(CTL_PSNEVNT_TAGLIST, disable_taglist);
	enableCommand(cmaInsert, !disable_taglist);
	enableCommand(cmaEdit,   !disable_taglist);
	enableCommand(cmaDelete, !disable_taglist);
	disableCtrls(disable_post, CTLSEL_PSNEVNT_POST, CTLSEL_PSNEVNT_DIV, 0L);
	{
		ImageBrowseCtrlGroup::Rec rec;
		Pack.LinkFiles.Init(PPOBJ_PERSONEVENT);
		if(Pack.Rec.Flags & PSNEVF_HASIMAGES)
			Pack.LinkFiles.Load(Pack.Rec.ID, 0L);
		Pack.LinkFiles.At(0, rec.Path);
		setGroupData(GRP_IBG, &rec);
	}
	if(PokPack.AllowedTags.IsExists()) {
		PPIDArray allowed_tags = PokPack.AllowedTags.Get();
		for(uint i = 0; i < allowed_tags.getCount(); i++) {
			PPID   tag_id = allowed_tags.at(i);
			const ObjTagItem * p_item = Pack.TagL.GetItem(tag_id);
			if(!p_item) {
				ObjTagItem item;
				THROW(item.Init(tag_id));
				THROW(Pack.TagL.PutItem(tag_id, &item));
			}
		}
	}
	// @v8.0.3 {
	AddClusterAssoc(CTL_PSNEVNT_FLAGS, 0, PSNEVF_FORCEPAIR);
	SetClusterData(CTL_PSNEVNT_FLAGS, Pack.Rec.Flags);
	DisableClusterItem(CTL_PSNEVNT_FLAGS, 0, !P_PeObj->CheckRights(PSNEVRT_SETFORCEPAIR) || !oneof2(PokPack.Rec.PairType, POKPT_OPEN, POKPT_CLOSE));
	// } @v8.0.3
	setupPair();
	updateList(-1);
	CATCHZOK
	return ok;
}

int PsnEventDialog::getDTS(PPPsnEventPacket * pPack)
{
	int    ok = 1;
	{
		PersonCtrlGroup::Rec rec;
		getGroupData(GRP_PERSON_PRMR, &rec);
		Pack.Rec.PersonID = rec.PersonID;
		Pack.Rec.PrmrSCardID = rec.SCardID;
	}
	THROW_PP(Pack.Rec.PersonID, PPERR_PRMRPSNNEEDED);
	{
		PersonCtrlGroup::Rec rec;
		getGroupData(GRP_PERSON_SCND, &rec);
		Pack.Rec.SecondID = rec.PersonID;
		Pack.Rec.ScndSCardID = rec.SCardID;
	}
	getCtrlData(CTL_PSNEVNT_DATE,    &Pack.Rec.Dt);
	getCtrlData(CTL_PSNEVNT_TIME,    &Pack.Rec.Tm);
	THROW_SL(checkdate(Pack.Rec.Dt));
	getCtrlData(CTL_PSNEVNT_MEMO,    Pack.Rec.Memo);
	if(P.ExValGrp == POKEVG_POST)
		getCtrlData(CTLSEL_PSNEVNT_POST, &Pack.Rec.Extra);
	else
		Pack.Rec.Extra = 0;
	{
		ImageBrowseCtrlGroup::Rec rec;
		if(getGroupData(GRP_IBG, &rec))
			if(rec.Path.Len()) {
				THROW(Pack.LinkFiles.Replace(0, rec.Path));
			}
			else
				Pack.LinkFiles.Remove(0);
		SETFLAG(Pack.Rec.Flags, PSNEVF_HASIMAGES, Pack.LinkFiles.GetCount());
	}
	for(uint i = Pack.TagL.GetCount(); i > 0; i--) {
		const ObjTagItem * p_item = Pack.TagL.GetItemByPos(i-1);
		if(p_item->TagDataType == OTTYP_STRING && p_item->Val.PStr == 0)
			Pack.TagL.PutItem(p_item->TagID, 0);
	}
	GetClusterData(CTL_PSNEVNT_FLAGS, &Pack.Rec.Flags); // @v8.0.3
	ASSIGN_PTR(pPack, Pack);
	CATCHZOKPPERR
	return ok;
}

struct PsnEventItemPrintStruc {
	PPPsnEventPacket * P_Pack;
	PPObjTag         * P_ObjTag;
};

int PsnEventDialog::GetReportID()
{
	int    rpt_id = REPORT_PSNEVENTITEM;
	RegisterTbl::Rec regrec = Pack.Reg;
	SETIFZ(regrec.RegTypeID, PokPack.Rec.RegTypeID);
	if(regrec.RegTypeID) {
		PPRegisterType     reg_type;
		PPObjRegisterType  regt_obj;
		if(regt_obj.Search(regrec.RegTypeID, &reg_type) > 0 && reg_type.Symb[0]) {
			int  type_no = -1;
			SString buf;
			PPLoadText(PPTXT_REGISTER_TYPE_SYMB, buf);
			if(PPSearchSubStr(buf, &type_no, reg_type.Symb, 1))
				switch (type_no) {
					case 0: rpt_id = REPORT_DISPATCHORDER; break;
					case 1: rpt_id = REPORT_LEAVEORDER;    break;
					case 2: rpt_id = REPORT_RECEIPTORDER;  break;
					case 3: rpt_id = REPORT_DISMISSORDER;  break;
				}
		}
	}
	return rpt_id;
}

int PsnEventDialog::Print()
{
	PsnEventItemPrintStruc prn_struc;
	PPPsnEventPacket pack;
	PPObjTag objtag;
	getDTS(&pack);
	prn_struc.P_ObjTag = &objtag;
	prn_struc.P_Pack   = &pack;
	PView  pv(&prn_struc);
	return PPAlddPrint(GetReportID(), &pv, 0);
}
//
//
//
int SLAPI PPObjPersonEvent::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTPSNEV, 0, 0, bufSize, rt, pDlg);
}

int SLAPI PPObjPersonEvent::Browse(void * extraPtr /*prmrPersonID*/)
{
	PersonEventFilt * p_filt = 0, flt;
	if(extraPtr) {
		flt.PrmrID = reinterpret_cast<long>(extraPtr);
		p_filt = &flt;
	}
	return PPView::Execute(PPVIEW_PERSONEVENT, p_filt, PPView::exefModeless, 0);
}

int SLAPI PPObjPersonEvent::Edit(PPID * pID, void * extraPtr /*prmrID*/)
{
	const  PPID extra_prmr_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel, valid_data = 0;
	PPID   op = 0;
	PsnEventDialog::Param param;
	PPPsnEventPacket pack;
	PPObjPsnOpKind   pokobj;
	PsnEventDialog * dlg = 0;
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
		op = pack.Rec.OpID;
	}
	else {
		op = PPObjPsnOpKind::Select(0);
		if(op > 0) {
			THROW(pack.Init(op));
			pack.Rec.PersonID = extra_prmr_id;
		}
		else
			return -1;
	}
	THROW(PsnEventDialog::GetParam(op, &param));
	THROW(CheckDialogPtr(&(dlg = new PsnEventDialog(&param, this))));
	THROW(dlg->setDTS(&pack));
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&pack))
			if(PutPacket(pID, &pack, 1))
				valid_data = ok = cmOK;
			else
				ok = PPErrorZ();
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

void SLAPI PPObjPersonEvent::Subst(SubstGrpPersonEvent sgpe, PPID opID, PPID prmrID, PPID scndID, PPID * pID)
{
	PPID   id = 0;
	if(sgpe == sgpeOp)
		id = opID;
	else if(sgpe == sgpePerson)
		id = prmrID;
	else if(sgpe == sgpeCntrAg)
		id = scndID;
	ASSIGN_PTR(pID, id);
}

void SLAPI PPObjPersonEvent::GetSubstName(SubstGrpPersonEvent sgpe, PPID id, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	if(sgpe == sgpeOp) {
		PPObjPsnOpKind pop_obj;
		PPPsnOpKind rec;
		MEMSZERO(rec);
		if(pop_obj.Fetch(id, &rec) > 0)
			temp_buf = rec.Name;
		else
			ideqvalstr(id, temp_buf);
	}
	else if(oneof2(sgpe, sgpePerson, sgpeCntrAg))
		GetPersonName(id, temp_buf);
	temp_buf.CopyTo(pBuf, bufLen);
}

IMPL_DESTROY_OBJ_PACK(PPObjPersonEvent, PPPsnEventPacket);

int SLAPI PPObjPersonEvent::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPPsnEventPacket);
	PPPsnEventPacket * p_pack = static_cast<PPPsnEventPacket *>(p->Data);
	if(stream == 0) {
		THROW(GetPacket(id, p_pack) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPPsnEventPacket * p_pack = static_cast<PPPsnEventPacket *>(p->Data);
		SString added_buf;
		if(stream == 0) {
			if(p_pack->Reg.RegTypeID) {
				RegObj.PreventDup(p_pack->Reg, Obj, *pID);
			}
			if(*pID) {
				if(!PutPacket(pID, p_pack, 1)) {
					MakeCodeString(&p_pack->Rec, added_buf);
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPSNEVENT, p_pack->Rec.ID, added_buf);
					ok = -1;
				}
				else {
					ok = 1; // 102; // @ObjectUpdated
				}
			}
			else {
				p_pack->Rec.ID = *pID = 0;
				if(!PutPacket(pID, p_pack, 1)) {
					MakeCodeString(&p_pack->Rec, added_buf);
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPSNEVENT, p_pack->Rec.ID, added_buf);
					ok = -1;
				}
				else {
					ok = 1; // 101; // @ObjectCreated
				}
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPersonEvent::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPPsnEventPacket * p_pack = static_cast<PPPsnEventPacket *>(p->Data);
		ProcessObjRefInArray(PPOBJ_PERSONOPKIND,  &p_pack->Rec.OpID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,        &p_pack->Rec.PersonID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,        &p_pack->Rec.SecondID, ary, replace);
		ProcessObjRefInArray(PPOBJ_LOCATION,      &p_pack->Rec.LocationID, ary, replace);
		ProcessObjRefInArray(PPOBJ_BILL,          &p_pack->Rec.LinkBillID, ary, replace);
		ProcessObjRefInArray(PPOBJ_SCARD,         &p_pack->Rec.PrmrSCardID, ary, replace);
		ProcessObjRefInArray(PPOBJ_SCARD,         &p_pack->Rec.ScndSCardID, ary, replace);
		{
			if(replace)
				p_pack->Reg.ID = 0;
			ProcessObjRefInArray(PPOBJ_REGISTERTYPE, &p_pack->Reg.RegTypeID, ary, replace);
			ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->Reg.RegOrgID, ary, replace);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PsnEventItem
//
PPALDD_CONSTRUCTOR(PsnEventItem)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PsnEventItem) { Destroy(); }

int PPALDD_PsnEventItem::InitData(PPFilt & rFilt, long rsrv)
{
	PPPsnEventPacket * p_pack = 0;
	if(rsrv) {
		Extra[0].Ptr = rFilt.Ptr;
		if(Extra[0].Ptr)
			p_pack = static_cast<PsnEventItemPrintStruc *>(Extra[0].Ptr)->P_Pack;
	}
	if(p_pack) {
		PPObjRegister    reg_obj;
		RegisterTbl::Rec reg_rec;
		H.LocID      = p_pack->Rec.LocationID;
		H.PersonID   = p_pack->Rec.PersonID;
		H.SecondID   = p_pack->Rec.SecondID;
		H.RegisterID = p_pack->Reg.ID;
		H.Dt         = p_pack->Rec.Dt;
		if(p_pack->Reg.ID && reg_obj.Search(p_pack->Reg.ID, &reg_rec) > 0) {
			long  duration;
			if(reg_rec.Dt.getclass() == LDATE::cNormal && reg_rec.Expiry.getclass() == LDATE::cNormal &&
				(duration = diffdate(reg_rec.Expiry, reg_rec.Dt) + 1) > 0)
				H.Duration = duration;
			else
				H.Duration = 0;
		}
		H.OprNo      = p_pack->Rec.OprNo;
		STRNSCPY(H.Memo, p_pack->Rec.Memo);
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PsnEventItem::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return 1;
}

int PPALDD_PsnEventItem::NextIteration(PPIterID iterId)
{
	int    r = -1;
	if(IterProlog(iterId, 0) < 0)
		I.nn = 0;
	uint   n = static_cast<uint>(I.nn);
	PsnEventItemPrintStruc * p_struc = static_cast<PsnEventItemPrintStruc *>(Extra[0].Ptr);
	PPPsnEventPacket * p_pack = (p_struc) ? p_struc->P_Pack : 0;
	PPObjTag * p_objtag = (p_struc) ? p_struc->P_ObjTag : 0;
	if(p_pack && p_objtag && n < p_pack->TagL.GetCount()) {
		SString buf;
		const ObjTagItem * p_item = p_pack->TagL.GetItemByPos(n);
		p_objtag->GetCurrTagVal(p_item, buf);
		I.TagID = p_item->TagID;
		buf.CopyTo(I.TagValue, sizeof(I.TagValue));
		n++;
		r = 1;
	}
	I.nn = n;
	return (r > 0) ? DlRtm::NextIteration(iterId) : r;
}

void PPALDD_PsnEventItem::Destroy()
{
	Extra[0].Ptr = 0;
	Extra[1].Ptr = 0;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(AddPersonEvent); SLAPI AddPersonEventFilt::AddPersonEventFilt() : PPBaseFilt(PPFILT_ADDPERSONEVENT, 0, 2)
{
	SetFlatChunk(offsetof(AddPersonEventFilt, ReserveStart),
		offsetof(AddPersonEventFilt, OpCode) - offsetof(AddPersonEventFilt, ReserveStart));
	SetBranchSString(offsetof(AddPersonEventFilt, OpCode));
	SetBranchSString(offsetof(AddPersonEventFilt, PrmrPsnCode));
	SetBranchSString(offsetof(AddPersonEventFilt, ScndPsnCode));
	SetBranchSString(offsetof(AddPersonEventFilt, PrmrSCardCode));
	SetBranchSString(offsetof(AddPersonEventFilt, ScndSCardCode));
	Init(1, 0);
}

AddPersonEventFilt & FASTCALL AddPersonEventFilt::operator = (const AddPersonEventFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

int SLAPI AddPersonEventFilt::ReadText(const char * pText, long)
{
	int    ok = 1;
	Init(1, 0);
	if(pText) {
		enum {
			cOp = 1,        // OP
			cPrmrPerson,    // PRMRPERSON PERSON
			cScndPerson,    // SCNDPERSON
			cPrmrSCard,     // PRMRSCARD SCARD
			cScndSCard,     // SCNDSCARD
			cInteractLevel, // INTERACTLEVEL||INTERACTIVELEVEL
			cNonInteractive // NONINTERACT||NONINTERACTIVE
		};
		enum {
			scDefault = 0,
			scID,
			scCode
		};
		SString temp_buf;
		SStrScan scan(pText);
		while(ok && scan.Skip().GetIdent(temp_buf)) {
			int    crit = 0;
			int    subcrit = 0;
			int    no_param = 0;
			if(temp_buf.IsEqiAscii("OP"))
				crit = cOp;
			else if(temp_buf.IsEqiAscii("PRMRSCARD") || temp_buf.IsEqiAscii("SCARD"))
				crit = cPrmrSCard;
			else if(temp_buf.IsEqiAscii("SCNDSCARD"))
				crit = cScndSCard;
			else if(temp_buf.IsEqiAscii("PRMRPERSON") || temp_buf.IsEqiAscii("PERSON"))
				crit = cPrmrPerson;
			else if(temp_buf.IsEqiAscii("SCNDPERSON"))
				crit = cScndPerson;
			else if(temp_buf.IsEqiAscii("INTERACTIVELEVEL") || temp_buf.IsEqiAscii("INTERACTLEVEL")) {
				crit = cInteractLevel;
			}
			else if(temp_buf.IsEqiAscii("NONINTERACTIVE") || temp_buf.IsEqiAscii("NONINTERACT")) {
				crit = cNonInteractive;
				no_param = 1;
			}
			else {
				CALLEXCEPT(); // @error InvalidCriterion
			}
			if(scan[0] == '.') {
				scan.Incr();
				THROW(scan.Skip().GetIdent(temp_buf)); // @error
				if(temp_buf.IsEqiAscii("ID"))
					subcrit = scID;
				else if(temp_buf.IsEqiAscii("CODE"))
					subcrit = scCode;
				else {
					CALLEXCEPT(); // @error InvalidSubcriterion
				}
			}
			if(!no_param) {
				temp_buf.Z();
				THROW(scan.Skip()[0] == '('); // @error
				{
					temp_buf.Z();
					char   c;
					int    par_count = 0;
					do {
						scan.Incr();
						c = scan[0];
						THROW(c); // @error
						if(c == ')') {
							if(par_count)
								par_count--;
							else {
								scan.Incr();
								break;
							}
						}
						else {
							if(c == '(')
								par_count++;
						}
						temp_buf.CatChar(c);
					} while(c);
				}
				temp_buf.Strip();
			}
			switch(crit) {
				case cOp:
					if(subcrit == scCode)
						OpCode = temp_buf;
					else
						OpID = temp_buf.ToLong();
					break;
				case cPrmrPerson:
					if(subcrit == scCode)
						PrmrPsnCode = temp_buf;
					else
						PrmrPsnID = temp_buf.ToLong();
					break;
				case cScndPerson:
					if(subcrit == scCode)
						ScndPsnCode = temp_buf;
					else
						ScndPsnID = temp_buf.ToLong();
					break;
				case cPrmrSCard:
					if(subcrit == scCode)
						PrmrSCardCode = temp_buf;
					else
						PrmrSCardID = temp_buf.ToLong();
					break;
				case cScndSCard:
					if(subcrit == scCode)
						ScndSCardCode = temp_buf;
					else
						ScndSCardID = temp_buf.ToLong();
					break;
				case cInteractLevel:
					InteractiveLevel = static_cast<int16>(temp_buf.ToLong());
					break;
				case cNonInteractive:
					InteractiveLevel = 0;
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

class AddPersonEventFiltDialog : public TDialog {
public:
	AddPersonEventFiltDialog() : TDialog(DLG_FLTADDPSNEV)
	{
		SetupCalDate(CTLCAL_FLTADDPSNEV_DATE, CTL_FLTADDPSNEV_DATE);
		addGroup(GRP_PERSON_PRMR, new PersonCtrlGroup(CTLSEL_FLTADDPSNEV_PRMR, CTL_FLTADDPSNEV_PSCARD, 0, PersonCtrlGroup::fCanInsert));
		addGroup(GRP_PERSON_SCND, new PersonCtrlGroup(CTLSEL_FLTADDPSNEV_SCND, CTL_FLTADDPSNEV_SSCARD, 0, PersonCtrlGroup::fCanInsert));
	}
	int    setDTS(const AddPersonEventFilt * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		setCtrlDate(CTL_FLTADDPSNEV_DATE, Data.Dt);
		SetupPPObjCombo(this, CTLSEL_FLTADDPSNEV_OP,  PPOBJ_PERSONOPKIND,  Data.OpID, 0);
		SetupPPObjCombo(this, CTLSEL_FLTADDPSNEV_DVC, PPOBJ_GENERICDEVICE, Data.ReaderDvcID, 0, (void *)DVCCLS_READER); // @v7.9.6
		{
			long iv = Data.DvcReadCycle;
			setCtrlLong(CTL_FLTADDPSNEV_RDCYCLE, iv);
			iv = Data.DvcReadPeriod * 5;
			setCtrlLong(CTL_FLTADDPSNEV_RDTIME, iv);
		}
		AddClusterAssocDef(CTL_FLTADDPSNEV_INTAL, 0, 2);
		AddClusterAssoc(CTL_FLTADDPSNEV_INTAL, 1, 1);
		AddClusterAssoc(CTL_FLTADDPSNEV_INTAL, 2, 0);
		SetClusterData(CTL_FLTADDPSNEV_INTAL, Data.InteractiveLevel);
		SetupCtrls(Data.OpID);
		return 1;
	}
	int    getDTS(AddPersonEventFilt * pData)
	{
		int    ok = 1;
		uint   ctl = 0;
		Data.Dt = getCtrlDate(CTL_FLTADDPSNEV_DATE);
		Data.OpID = getCtrlLong(ctl = CTLSEL_FLTADDPSNEV_OP);
		THROW_PP(Data.OpID, PPERR_PSNOPKNOTDEF);
		Data.ReaderDvcID = getCtrlLong(ctl = CTLSEL_FLTADDPSNEV_DVC); // @v7.9.6
		{
			long iv = getCtrlLong(CTL_FLTADDPSNEV_RDCYCLE);
			Data.DvcReadCycle = (uint16)iv;
			iv = getCtrlLong(CTL_FLTADDPSNEV_RDTIME);
			Data.DvcReadPeriod = static_cast<uint16>(iv / 5);
		}
		{
			PersonCtrlGroup::Rec rec;
			getGroupData(GRP_PERSON_PRMR, &rec);
			Data.PrmrPsnID = rec.PersonID;
			Data.PrmrSCardID = rec.SCardID;
		}
		{
			PersonCtrlGroup::Rec rec;
			getGroupData(GRP_PERSON_SCND, &rec);
			Data.ScndPsnID = rec.PersonID;
			Data.ScndSCardID = rec.SCardID;
		}
		Data.InteractiveLevel = (int16)GetClusterData(CTL_FLTADDPSNEV_INTAL);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, ctl);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(TVCOMMAND && TVCMD == cmCBSelected && event.isCtlEvent(CTLSEL_FLTADDPSNEV_OP)) {
			PPID   op_id = getCtrlLong(CTLSEL_FLTADDPSNEV_OP);
			SetupCtrls(op_id);
			clearEvent(event);
		}
	}
	void   SetupCtrls(PPID opID)
	{
		PPPsnOpKindPacket pok_pack;
		getCtrlData(CTLSEL_FLTADDPSNEV_PRMR, &Data.PrmrPsnID);
		getCtrlData(CTLSEL_FLTADDPSNEV_SCND, &Data.ScndPsnID);
		if(opID == 0 || opID != Data.OpID) {
			Data.PrmrPsnID = 0;
			Data.ScndPsnID = 0;
			Data.PrmrSCardID = 0;
			Data.ScndSCardID = 0;
		}
		if(opID && PokObj.GetPacket(opID, &pok_pack) > 0) {
			// @v10.3.0 (never used) int    disable_taglist = 0;
			PsnEventDialog::Param param;
			PsnEventDialog::GetParam(opID, &param);
			{
				PersonCtrlGroup::Rec rec;
				rec.PersonID = Data.PrmrPsnID;
				rec.PsnKindID = pok_pack.PCPrmr.PersonKindID;
				rec.SCardID = Data.PrmrSCardID;
				setGroupData(GRP_PERSON_PRMR, &rec);
			}
			{
				PersonCtrlGroup::Rec rec;
				rec.PersonID = Data.ScndPsnID;
				rec.PsnKindID = pok_pack.PCScnd.PersonKindID;
				rec.SCardID = Data.ScndSCardID;
				setGroupData(GRP_PERSON_SCND, &rec);
			}
			disableCtrls(0, CTLSEL_FLTADDPSNEV_PRMR, CTLSEL_FLTADDPSNEV_SCND, 0L);
		}
		else {
			disableCtrls(1, CTLSEL_FLTADDPSNEV_PRMR, CTLSEL_FLTADDPSNEV_SCND, CTL_FLTADDPSNEV_TAGS, 0L);
		}
		Data.OpID = opID;
	}
	AddPersonEventFilt Data;
	PPObjPsnOpKind PokObj;
};

int SLAPI AddPersonEventFilt::Edit() { DIALOG_PROC_BODY(AddPersonEventFiltDialog, this); }
//
//
//
#if 0 // {
/*
// @vmiller {
	int    ok = -1, port_no = -1;
	PPAbstractDevice * P_AbstrDvc = 0;
	P_AbstrDvc = new PPAbstractDevice(0);
	SString temp_buf;
	StrAssocArray in_params, out_params;

	PPAbstractDevice::ConnectionParam cp;
	cp.DeviceNo = rRec.DeviceNumber;
	cp.Address = rRec.Port;
	cp.Cpp = rRec.Cpp;

	if(IsComDvcSymb(rRec.Port, &port_no) == comdvcsCom)
		if(port_no > 0)
			port_no--;

	P_AbstrDvc->PCpb.Cls = DVCCLS_READER;
	P_AbstrDvc->GetDllName(DVCCLS_READER, /*rRec.ID*/1, P_AbstrDvc->PCpb.DllName); // @vmiller Пока напишем номер утсройства в списке - 1, в списке устройств, перечисленных в ppdrv.ini. Хотя ИД здесь и не подойдет, наверное...
	P_AbstrDvc->IdentifyDevice(P_AbstrDvc->PCpb.DllName);
	// инициализируем
	THROW(ExecOper(P_AbstrDvc, DVCCMD_INIT, in_params, out_params.Clear()));
	// соединяемся
	in_params.Add(DVCPARAM_PORT, temp_buf.Z().Cat(/*port_no*/7)); // @vmiller Пока напишем порт устройства - 7 (ибо все равно через эмулятор)
	THROW(ExecOper(P_AbstrDvc, DVCCMD_CONNECT, in_params, out_params.Clear()));
	// читаем с устройства
	THROW(ExecOper(P_AbstrDvc, DVCCMD_LISTEN, in_params.Clear(), out_params.Clear()));
	out_params.Get(0, temp_buf);
	PPOutputMessage(temp_buf, mfOK);
	// } @vmiller
*/
#endif // } 0

SLAPI PPObjPersonEvent::ProcessDeviceInputBlock::ProcessDeviceInputBlock() : Ad(0), State(0)
{
}

const SString & SLAPI PPObjPersonEvent::ProcessDeviceInputBlock::GetDeviceText() const
	{ return DeviceText; }
const SString & SLAPI PPObjPersonEvent::ProcessDeviceInputBlock::GetInfoText() const
	{ return InfoText; }

int SLAPI PPObjPersonEvent::InitProcessDeviceInput(ProcessDeviceInputBlock & rBlk, const AddPersonEventFilt & rFilt)
{
	int    ok = 1;

	rBlk.State = 0;
	rBlk.Out.Z();
	rBlk.TempBuf.Z();
	rBlk.Filt = rFilt;
	rBlk.DeviceText.Z();
	rBlk.InfoText.Z();

	THROW_PP(rFilt.ReaderDvcID, PPERR_ADDPEVUNDEFDVC);
	THROW_PP(rFilt.OpID, PPERR_UNDEFPEOP);
	{
		SString temp_buf, init_buf;
		PPObjGenericDevice gd_obj;
		PPGenericDevicePacket gd_pack;
		PPObjPsnOpKind pok_obj;

		THROW(gd_obj.GetPacket(rFilt.ReaderDvcID, &gd_pack) > 0);
		THROW_PP(gd_pack.Rec.DeviceClass == DVCCLS_READER, PPERR_ADDPEVDVCNOTREADER);
		gd_pack.GetExtStrData(GENDVCEXSTR_ENTRY, temp_buf);
		THROW_PP(temp_buf.NotEmptyS(), PPERR_UNDEFADVCDESCR);

		THROW(pok_obj.GetPacket(rFilt.OpID, &rBlk.PokPack) > 0);

		rBlk.Ad.PCpb.Cls = gd_pack.Rec.DeviceClass;
		THROW(rBlk.Ad.IdentifyDevice(gd_pack.Rec.DeviceClass, temp_buf));
		THROW(rBlk.Ad.RunCmd("INIT", rBlk.Out.Z()));
		rBlk.State |= rBlk.stInitialized;

		gd_pack.GetExtStrData(GENDVCEXSTR_INITSTR, temp_buf.Z());
		if(temp_buf.NotEmptyS()) {
			THROW(rBlk.Ad.RunCmd(temp_buf, rBlk.Out.Z()));
		}
		(rBlk.DeviceText = gd_pack.Rec.Name).CatDiv('-', 1).Cat(rBlk.PokPack.Rec.Name);
	}
	CATCH
		rBlk.State |= rBlk.stError;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjPersonEvent::FinalizeProcessDeviceInput(ProcessDeviceInputBlock & rBlk)
{
	return (rBlk.State & rBlk.stInitialized) ? rBlk.Ad.RunCmd("RELEASE", rBlk.Out.Z()) : 0;
}

int SLAPI PPObjPersonEvent::Helper_ProcessDeviceInput(ProcessDeviceInputBlock & rBlk)
{
	int    ok = 1;
	rBlk.InfoText = 0;
	if(rBlk.State & rBlk.stInitialized) {
		rBlk.Ad.RunCmd("LISTEN", rBlk.Out.Z());
		if(rBlk.Out.GetText(0, rBlk.TempBuf) > 0 && rBlk.TempBuf.NotEmptyS()) {
			rBlk.InfoText = rBlk.TempBuf;

			PPID   ev_id = 0;
			AddPersonEventFilt temp_filt;
			temp_filt = rBlk.Filt;
			temp_filt.PrmrSCardCode = rBlk.TempBuf.Strip();
			PPPsnEventPacket pack;
			if(!InitPacket(&pack, temp_filt, 0 /* noninteractive */)) {
				PoClause_ clause;
				for(uint i = 0; i < rBlk.PokPack.ClauseList.GetCount(); i++) {
					rBlk.PokPack.ClauseList.Get(i, clause);
					if(clause.VerbID == POVERB_STYLODISPLAY && clause.DirObj) {
						PPGetLastErrorMessage(1, rBlk.TempBuf);
						PalmDisplayBlock pdb;
						pdb.DvcID = clause.DirObj;
						pdb.Ctx = pdb.ctxPersonEvent;
						pdb.Flags |= pdb.fError;
						pdb.DirectMsg = rBlk.TempBuf;
						PPObjStyloPalm::PutDisplayBlock(pdb);
						break;
					}
				}
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
				PPErrorTooltip(-1, 0);
			}
			else if(!PutPacket(&ev_id, &pack, 1)) {
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
				PPErrorTooltip(-1, 0);
			}
			else {
				ViewPersonInfoBySCard(temp_filt.PrmrSCardCode);
			}
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPObjPersonEvent::ProcessDeviceInput(const AddPersonEventFilt & rFilt)
{
	class PersonEventByDeviceInputDialog : public TDialog {
	public:
		PersonEventByDeviceInputDialog() : TDialog(DLG_PEVNTBYDVCINFO), T(0), ShowTextT(0)
		{
			DlgFlags &= ~TDialog::fCentered;
		}
		~PersonEventByDeviceInputDialog()
		{
			PeObj.FinalizeProcessDeviceInput(Blk);
		}
		int    Init(const AddPersonEventFilt & rFilt)
		{
			int    ok = 1;
			if(PeObj.InitProcessDeviceInput(Blk, rFilt)) {
				setStaticText(CTL_PEVNTBYDVCINFO_DVC, Blk.GetDeviceText());
				T.Restart(rFilt.DvcReadCycle);
				ShowTextT.Restart(10000);
			}
			else
				ok = 0;
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCMD == cmIdle) {
				if(T.Check(0)) {
					if(PeObj.Helper_ProcessDeviceInput(Blk) > 0) {
						setStaticText(CTL_PEVNTBYDVCINFO_TEXT, Blk.GetInfoText());
						ShowTextT.Restart(10000);
					}
					else if(ShowTextT.Check(0)) {
						setStaticText(CTL_PEVNTBYDVCINFO_TEXT, Blk.GetInfoText());
					}
				}
				clearEvent(event);
			}
		}
		PPObjPersonEvent PeObj;
		PPObjPersonEvent::ProcessDeviceInputBlock Blk;
		SCycleTimer T;
		SCycleTimer ShowTextT;
	};
	int    ok = 1;
	PersonEventByDeviceInputDialog * dlg = new PersonEventByDeviceInputDialog;
	THROW(CheckDialogPtr(&dlg));
	{
		RECT r1, r2;
		GetWindowRect(dlg->H(), &r1);
		GetWindowRect(APPL->H_MainWnd, &r2);
		r1.bottom -= r1.top;
		r1.right -= r1.left;
		r1.top = r2.top+80;
		r1.left = r2.left+40;
		::MoveWindow(dlg->H(), r1.left, r1.top, r1.right, r1.bottom, 1);
	}
	THROW(dlg->Init(rFilt));
	ExecViewAndDestroy(dlg);
	dlg = 0;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
