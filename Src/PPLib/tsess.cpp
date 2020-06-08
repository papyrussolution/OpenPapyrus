// TSESS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2015, 2016, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
void SLAPI PrcBusy::Init(const LDATETIME & start, const LDATETIME & finish, int status, int idle)
{
	STimeChunk::Init(start, finish);
	TSessID = 0;
	Status = status;
	Idle   = BIN(idle);
}

void SLAPI PrcBusy::Init(const LDATETIME & start, long cont, int status, int idle)
{
	STimeChunk::Init(start, cont);
	TSessID = 0;
	Status = status;
	Idle = BIN(idle);
}

int SLAPI PrcBusy::Intersect(const PrcBusy & test, PrcBusy * pResult) const
{
	int    is = STimeChunk::Intersect(test, pResult);
	if(pResult) {
		pResult->Status = Status;
		pResult->TSessID = TSessID;
		pResult->Idle = Idle;
	}
	return is;
}
//
//
//
SLAPI PrcBusyArray::PrcBusyArray() : STimeChunkArray(sizeof(PrcBusy))
{
}

int SLAPI PrcBusyArray::IsFreeEntry(const PrcBusy & entry, PPID * pTSessID) const
{
	uint   pos = 0;
	int    ok = STimeChunkArray::IsFreeEntry(*static_cast<const STimeChunk *>(&entry), &pos);
	if(!ok)
		ASSIGN_PTR(pTSessID, static_cast<const PrcBusy *>(at(pos))->TSessID);
	return ok;
}

int SLAPI PrcBusyArray::IsFreeEntry(const LDATETIME & start, long cont, PPID * pTSessID) const
{
	PrcBusy entry;
	entry.Init(start, cont, 0);
	return start.d ? IsFreeEntry(entry, pTSessID) : 1;
}

int SLAPI PrcBusyArray::Add(const PrcBusy & entry, int checkForFree)
{
	int    ok = STimeChunkArray::Add(&entry, checkForFree);
	if(!ok)
		PPSetErrorSLib();
	return ok;
}

int SLAPI PrcBusyArray::GetFreeList(PrcBusyArray * pList) const
{
	int    ok = 1;
	STimeChunkArray free_list;
	STimeChunkArray::GetFreeList(&free_list);
	STimeChunk * p_item;
	for(uint i = 0; free_list.enumItems(&i, (void **)&p_item);) {
		PrcBusy free_entry;
		MEMSZERO(free_entry);
		free_entry.Start = p_item->Start;
		free_entry.Finish = p_item->Finish;
		pList->insert(&free_entry);
	}
	return ok;
}
//
//
//
SLAPI TSessionCore::TSessionCore() : TSessionTbl()
{
}

int SLAPI TSessionCore::Search(PPID id, TSessionTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_TSESSION, id, pRec);
}

int SLAPI TSessionCore::SearchAnyRef(PPID objType, PPID objID, PPID * pID)
{
	int   ok = -1;
	if(objType == PPOBJ_TECH) {
		TSessionTbl::Key5 k5;
		MEMSZERO(k5);
		k5.TechID = objID;
		if(search(5, &k5, spGe) && k5.TechID == objID) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_PROCESSOR) {
		TSessionTbl::Key4 k4;
		MEMSZERO(k4);
		k4.PrcID = objID;
		if(search(4, &k4, spGe) && k4.PrcID == objID) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_ARTICLE) {
		TSessionTbl::Key0 k0;
		BExtQuery q(this, 0, 1);
		q.select(this->ID, 0L).where(this->ArID == objID || this->Ar2ID == objID);
		MEMSZERO(k0);
		for(q.initIteration(0, &k0, spFirst); ok < 0 && q.nextIteration() > 0;) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_TSESSION) {
		TSessionTbl::Key6 k6;
		MEMSZERO(k6);
		k6.ParentID = objID;
		if(search(6, &k6, spGe) && k6.ParentID == objID) {
			ASSIGN_PTR(pID, data.ID);
			ok = 1;
		}
	}
	else if(objType == PPOBJ_GOODS) {
		TSessLineTbl::Key2 k2;
		MEMSZERO(k2);
		k2.GoodsID = objID;
		if(Lines.search(2, &k2, spGe) && k2.GoodsID == objID) {
			ASSIGN_PTR(pID, Lines.data.TSessID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI TSessionCore::ReplaceArticle(PPID dest, PPID src)
{
	int    ok = -1;
	uint   i;
	TSessionTbl::Rec rec;
	PPIDArray ses_list;
	TSessionTbl::Key0 k0;
	BExtQuery q(this, 0, 128);
	q.select(this->ID, this->ArID, this->Ar2ID, 0L).where(this->ArID == dest || this->Ar2ID == dest);
	MEMSZERO(k0);
	for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;)
		if(data.ArID == dest || data.Ar2ID == dest)
			ses_list.addUnique(data.ID);
	for(i = 0; i < ses_list.getCount(); i++)
		if(Search(ses_list.at(i), &rec) > 0) {
			int    r = 0;
			if(rec.ArID == dest) {
				r = 1;
				rec.ArID = src;
			}
			if(rec.Ar2ID == dest) {
				r = 1;
				rec.Ar2ID = src;
			}
			if(r > 0) {
				THROW_DB(updateRecBuf(&rec));
				ok = 1;
			}
		}
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::ReplaceGoods(PPID dest, PPID src)
{
	int    ok = -1;
	TSessLineTbl::Key2 k2;
	MEMSZERO(k2);
	k2.GoodsID = dest;
	if(Lines.search(2, &k2, spGe) && k2.GoodsID == dest)
		do {
			Lines.data.GoodsID = src;
			THROW_DB(Lines.updateRec());
			ok = 1;
		} while(Lines.search(2, &k2, spNext) && k2.GoodsID == dest);
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::SearchByPrcTime(PPID prcID, int kind, const LDATETIME & rDtm, TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	int    sp = spLe;
	TSessionTbl::Key4 k4;
	THROW_INVARG(oneof5(kind, TSESK_SESSION, TSESK_SUPERSESS, TSESK_PLAN, TSESK_IDLE, TSESK_SUBSESS));
	MEMSZERO(k4);
	k4.PrcID = prcID;
	k4.StDt = rDtm.d;
	k4.StTm = rDtm.t;
	while(ok < 0 && search(4, &k4, sp) && k4.PrcID == prcID) {
		if(PPObjTSession::GetSessionKind(data, 0) == kind && data.Status != TSESST_CANCELED) {
			LDATETIME finish;
			finish.Set(data.FinDt, data.FinTm);
			if(finish.d == 0 || cmp(finish, rDtm) > 0) {
				copyBufTo(pRec);
				ok = 1;
			}
			else {
				//
				// Если обнаружена сессия, которая закончилась ранее заданного момента, то
				// поиск можно не продолжать - все остальные сессии имеют меньшее время окончания,
				// в противном случае мы имели бы пересечение сессий.
				//
				break;
			}
		}
		sp = spLt;
	}
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::Helper_GetChildIDList(PPID superSessID, long flags, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray list;
	TSessionTbl::Key6 k6;
	MEMSZERO(k6);
	BExtQuery q(this, 6);
	q.select(this->ID, this->Flags, 0L).where(this->ParentID == superSessID);
	k6.ParentID = superSessID;
	for(q.initIteration(0, &k6, spGe); q.nextIteration() > 0;) {
		if(!(flags & gclfSubSess) || data.Flags & TSESF_SUBSESS) {
			THROW_SL(list.addUnique(data.ID));
			ok = 1;
		}
	}
	if(flags & gclfRecursive) {
		PPIDArray inner_list;
		for(uint i = 0; i < list.getCount(); i++) {
			int    r;
			THROW(r = Helper_GetChildIDList(list.get(i), flags, &inner_list)); // @recursion
			if(r > 0)
				ok = 1;
		}
		THROW_SL(list.addUnique(&inner_list));
	}
	CALLPTRMEMB(pList, addUnique(&list));
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::GetChildIDList(PPID superSessID, long flags, PPIDArray * pList)
{
	return Helper_GetChildIDList(superSessID, flags, pList);
}

int SLAPI TSessionCore::SearchSessNumber(PPID prcID, long * pNumber, TSessionTbl::Rec * pRec)
{
	int    ok = 1;
	TSessionTbl::Key1 k1;
	k1.PrcID = prcID;
	if(*pNumber) {
		k1.Num = *pNumber;
		ok = SearchByKey(this, 1, &k1, pRec);
	}
	else {
		k1.Num = MAXLONG;
		*pNumber = (search(1, &k1, spLe) && k1.PrcID == prcID) ? (data.Num + 1) : 1;
	}
	return ok;
}

int SLAPI TSessionCore::SearchOprNo(PPID sessID, long * pOprNo, TSessLineTbl::Rec * pRec)
{
	int    ok = 1;
	TSessLineTbl::Key0 k0;
	MEMSZERO(k0);
	k0.TSessID = sessID;
	if(*pOprNo) {
		k0.OprNo = *pOprNo;
		ok = SearchByKey(&Lines, 0, &k0, pRec);
	}
	else {
		k0.OprNo = MAXLONG;
		if(Lines.search(0, &k0, spLe) && k0.TSessID == sessID)
			*pOprNo = Lines.data.OprNo + 1;
		else
			*pOprNo = 1;
	}
	return ok;
}

int SLAPI TSessionCore::SearchLine(PPID sessID, long oprNo, TSessLineTbl::Rec * pRec)
{
	return oprNo ? SearchOprNo(sessID, &oprNo, pRec) : -1;
}

int SLAPI TSessionCore::SearchLineByTime(PPID sessID, const LDATETIME & rDtm, TSessLineTbl::Rec * pRec)
{
	TSessLineTbl::Key1 k1;
	MEMSZERO(k1);
	k1.TSessID = sessID;
	k1.Dt = rDtm.d;
	k1.Tm = rDtm.t;
	return SearchByKey(&Lines, 1, &k1, pRec);
}
//
//
//
int SLAPI TSessionCore::InitLineEnum(PPID sessID, long * pHandle)
{
	int    ok = 1;
	BExtQuery * q = new BExtQuery(&Lines, 0);
	q->selectAll().where(Lines.TSessID == sessID);
	TSessLineTbl::Key0 k0;
	MEMSZERO(k0);
	k0.TSessID = sessID;
	q->initIteration(0, &k0, spGe);
	ok = Lines.EnumList.RegisterIterHandler(q, pHandle);
	return ok;
}

int SLAPI TSessionCore::InitLineEnumBySerial(const char * pSerial, int sign, long * pHandle)
{
	int    ok = 1;
	char   temp_serial[32];
	STRNSCPY(temp_serial, pSerial);
	BExtQuery * q = new BExtQuery(&Lines, 3);
	q->selectAll().where(Lines.Serial == temp_serial && Lines.Sign == (long)sign);
	TSessLineTbl::Key3 k3;
	MEMSZERO(k3);
	STRNSCPY(k3.Serial, temp_serial);
	k3.Sign = sign;
	q->initIteration(0, &k3, spEq);
	ok = Lines.EnumList.RegisterIterHandler(q, pHandle);
	return ok;
}

int SLAPI TSessionCore::NextLineEnum(long enumHandle, TSessLineTbl::Rec * pRec)
{
	int    ok = -1;
	if(Lines.EnumList.NextIter(enumHandle) > 0) {
		Lines.copyBufTo(pRec);
		ok = 1;
	}
	return ok;
}

int SLAPI TSessionCore::DestroyIter(long enumHandle)
{
	return Lines.EnumList.DestroyIterHandler(enumHandle);
}

SEnumImp * SLAPI TSessionCore::EnumLines(PPID sessID)
{
	long   h = -1;
	return InitLineEnum(sessID, &h) ? new PPTblEnum <TSessLineCore>(&Lines, h) : 0;
}

SEnumImp * SLAPI TSessionCore::EnumLinesBySerial(const char * pSerial, int sign)
{
	long   h = -1;
	return InitLineEnumBySerial(pSerial, sign, &h) ? new PPTblEnum <TSessLineCore>(&Lines, h) : 0;
}

int SLAPI TSessionCore::CalcGoodsTotal(PPID sessID, PPID goodsID, TSessGoodsTotal * pTotal)
{
	int    ok = -1;
	long   eh = -1;
	int    is_grp = 0;
	int    is_gen = 0;
	double amount = 0.0;
	TSessLineTbl::Rec line_rec;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(goods_obj.Fetch(goodsID, &goods_rec) > 0)
		if(goods_rec.Flags & GF_GENERIC)
			is_gen = 1;
		else if(goods_rec.Kind == PPGDSK_GROUP)
			is_grp = 1;
	for(InitLineEnum(sessID, &eh); NextLineEnum(eh, &line_rec) > 0;) {
		if(!goodsID || line_rec.GoodsID == goodsID ||
			(is_gen && goods_obj.BelongToGen(line_rec.GoodsID, &goodsID, 0) > 0) ||
			(is_grp && goods_obj.BelongToGroup(line_rec.GoodsID, goodsID, 0) > 0)) {
			pTotal->Count++;
			pTotal->Qtty = faddwsign(pTotal->Qtty, line_rec.Qtty, line_rec.Sign);
			{
				double phqtty = 0;
				if(line_rec.Flags & TSESLF_INDEPPHQTTY)
					phqtty = line_rec.WtQtty;
				else if(line_rec.Flags & TSESLF_PLAN_PHUNIT)
					phqtty = line_rec.Qtty;
				else {
					double phuperu;
					goods_obj.GetPhUPerU(line_rec.GoodsID, 0, &phuperu);
					phqtty = line_rec.Qtty * phuperu;
				}
				pTotal->PhQtty = faddwsign(pTotal->PhQtty, phqtty, line_rec.Sign);
			}
			amount = faddwsign(amount, -R2(line_rec.Qtty * (line_rec.Price - line_rec.Discount)), line_rec.Sign);
			ok = 1;
		}
	}
	DestroyIter(eh);
	pTotal->Amount = amount;
	return ok;
}

void SLAPI TSessionCore::InitPrcEntry(const TSessionTbl::Rec & rRec, PrcBusy & rEntry) const
{
	rEntry.Start.Set(rRec.StDt, rRec.StTm);
	rEntry.Finish.Set(rRec.FinDt, rRec.FinTm);
	rEntry.TSessID = rRec.ID;
	rEntry.Status  = rRec.Status;
	rEntry.Idle    = BIN(rRec.Flags & TSESF_IDLE);
}

int SLAPI TSessionCore::LoadBusyArray(PPID prcID, PPID exclTSesID, int kind, const STimeChunk * pPeriod, PrcBusyArray * pList)
{
	int    ok = 1;
	DBQ  * dbq = 0;
	TSessionTbl::Key4 k4;
	MEMSZERO(k4);
	k4.PrcID = prcID;
	BExtQuery q(this, 4, 1024); // @v10.0.05 128-->1024
	THROW_INVARG(oneof3(kind, TSESK_SESSION, TSESK_PLAN, TSESK_IDLE));
	dbq = &(this->PrcID == prcID);
	if(pPeriod) {
		if(pPeriod->Start.d) {
			TSessionTbl::Rec low_sess_rec;
			if(SearchByPrcTime(prcID, kind, pPeriod->Start, &low_sess_rec) > 0) {
				dbq = &(*dbq && this->StDt >= low_sess_rec.StDt);
				k4.StDt = low_sess_rec.StDt;
			}
			dbq = &(*dbq && this->FinDt >= pPeriod->Start.d);
		}
		if(pPeriod->Finish.d && !pPeriod->Finish.IsFar())
			dbq = &(*dbq && this->StDt <= pPeriod->Finish.d);
	}
	q.select(this->ID, this->StDt, this->StTm, this->FinDt, this->FinTm, this->Status, this->Flags, 0).where(*dbq);
	for(q.initIteration(0, &k4, spGe); q.nextIteration() > 0;)
		if(data.Status != TSESST_CANCELED && data.ID != exclTSesID) {
			if(kind == TSESK_IDLE && !(data.Flags & TSESF_IDLE))
				continue;
			else if(kind == TSESK_SESSION && (data.Flags & (TSESF_IDLE|TSESF_PLAN|TSESF_SUPERSESS)))
				continue;
			else if(kind == TSESK_SUPERSESS && (data.Flags & (TSESF_IDLE|TSESF_PLAN)) && !(data.Flags & TSESF_SUPERSESS))
				continue;
			else if(kind == TSESK_PLAN && !(data.Flags & TSESF_PLAN))
				continue;
			PrcBusy entry;
			InitPrcEntry(data, entry);
			THROW(pList->Add(entry, 0));
		}
	pList->Limit(pPeriod);
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::GetProcessed(PPID prcID, int kind, TSessionTbl::Rec * pRec)
{
	int    ok = -1;
	TSessionTbl::Key4 k4;
	MEMSZERO(k4);
	k4.PrcID = prcID;
	BExtQuery q(this, 4);
	q.selectAll().where(this->PrcID == prcID && this->Status == (long)TSESST_INPROCESS);
	for(q.initIteration(0, &k4, spGe); ok < 0 && q.nextIteration() > 0;) {
		if((kind == TSESK_IDLE && data.Flags & TSESF_IDLE) ||
			(kind == TSESK_PLAN && data.Flags & TSESF_PLAN) ||
			(kind == TSESK_SESSION && !(data.Flags & (TSESF_IDLE | TSESF_PLAN)))) {
			//if((idle && data.Flags & TSESF_IDLE) || (!idle && !(data.Flags & TSESF_IDLE))) {
			copyBufTo(pRec);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI TSessionCore::AdjustTime(TSessionTbl::Rec * pRec)
{
	TSessionTbl::Key4 k4;
	MEMSZERO(k4);
	k4.PrcID = pRec->PrcID;
	k4.StDt = pRec->StDt;
	k4.StTm = pRec->StTm;
	while(search(4, &k4, spEq) && (!pRec->ID || data.ID != pRec->ID)) {
		k4.StTm = pRec->StTm.addhs(1);
	}
	return 1;
}

int SLAPI TSessionCore::Put(PPID * pID, TSessionTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	PPID   log_action_id = 0;
	TSessionTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Search(*pID, &rec) > 0);
			if(pRec == 0) {
				//
				// Удаление пакета
				//
				THROW(deleteFrom(&Lines, 0, Lines.TSessID == *pID));
				THROW(RemoveByID(this, *pID, 0));
				THROW(PPRef->Assc.Remove(PPASS_CHECKINPSNTSES, 0, *pID, 0)); // @v7.7.2
				// log_action_id не инициализируем, по-скольку удаление объекта обычно
				// реализуется функцией PPObject::Remove, которая самостоятельно заносит
				// это событие
			}
			else {
				//
				// Изменение пакета
				//
				// Если сессия является незначительным простоем, то просто удаляем ее
				if(PPObjTSession::IsIdleInsignificant(pRec, rec.Status)) {
					THROW(deleteFrom(&Lines, 0, Lines.TSessID == *pID));
					THROW(RemoveByID(this, *pID, 0));
					THROW(PPRef->Assc.Remove(PPASS_CHECKINPSNTSES, 0, *pID, 0)); // @v7.7.2
					log_action_id = PPACN_OBJRMV;
				}
				else {
					THROW(AdjustTime(pRec));
					if(rec.PrcID != pRec->PrcID || pRec->Num == 0) { // @v7.6.5 (|| pRec->Num == 0)
						//
						// Если изменяется процессор в сессии, то обновляем значение счетчика Number
						// для того, чтобы избежать дублирования индекса
						//
						long   sess_number = 0;
						THROW(SearchSessNumber(pRec->PrcID, &sess_number, 0) > 0);
						pRec->Num = sess_number;
						DS.LogAction(PPACN_UPDTSESSPRC, PPOBJ_TSESSION, rec.ID, rec.PrcID, 0); // @v6.9.4
					}
					if(memcmp(pRec, &rec, sizeof(rec)) != 0) {
						THROW(UpdateByID(this, PPOBJ_TSESSION, *pID, pRec, 0));
						log_action_id = PPACN_OBJUPD;
					}
				}
			}
		}
		else if(pRec) {
			//
			// Добавление пакета
			//
			long   sess_number = 0;
			THROW(SearchSessNumber(pRec->PrcID, &sess_number, 0) > 0);
			pRec->Num = sess_number;
			THROW(AdjustTime(pRec));
			THROW(AddObjRecByID(this, PPOBJ_TSESSION, pID, pRec, 0));
			pRec->ID = *pID;
			log_action_id = PPACN_OBJADD;
		}
		DS.LogAction(log_action_id, PPOBJ_TSESSION, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::AdjustLineTime(TSessLineTbl::Rec * pRec)
{
	TSessLineTbl::Key1 k1;
	k1.TSessID = pRec->TSessID;
	k1.Dt = pRec->Dt;
	k1.Tm = pRec->Tm;
	while(Lines.search(1, &k1, spEq))
		k1.Tm = pRec->Tm.addhs(1);
	return 1;
}

int SLAPI TSessionCore::UpdateFlags(PPID id, long setF, long resetF, int use_ta)
{
	int    ok = -1;
	TSessionTbl::Rec rec;
	if(setF || resetF) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_TSESSION, id, &rec) > 0) {
			long   old_f = rec.Flags;
			if(setF & TSESF_HASIMAGES) rec.Flags |= TSESF_HASIMAGES;
			if(resetF & TSESF_HASIMAGES) rec.Flags &= ~TSESF_HASIMAGES;
			if(old_f != rec.Flags) {
				THROW_DB(updateRecBuf(&rec)); // @sfu
				DS.LogAction(PPACN_OBJUPD, PPOBJ_TSESSION, id, 0, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI TSessionCore::UpdateSuperSessCompleteness(PPID sessID, int use_ta)
{
	int    ok = -1;
	TSessionTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(sessID, &rec) > 0) {
			if(rec.Flags & TSESF_SUPERSESS) {
				int16 max_compl = 0;
				TSessionTbl::Key6 k6;
				MEMSZERO(k6);
				BExtQuery q(this, 6);
				q.select(this->ID, this->Incomplete, 0L).where(this->ParentID == sessID);
				k6.ParentID = sessID;
				for(q.initIteration(0, &k6, spGe); q.nextIteration() > 0;) {
					if(this->data.Incomplete > max_compl)
						max_compl = data.Incomplete;
				}
				if(max_compl != rec.Incomplete) {
					rec.Incomplete = max_compl;
					THROW(Put(&sessID, &rec, 0));
					ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
