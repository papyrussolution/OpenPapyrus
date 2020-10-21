// OBJSTAFF.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Штатное расписание
//
#include <pp.h>
#pragma hdrstop
//
// Helper function
//
int SetupStaffListCombo(TDialog * dlg, uint ctl, PPID id, uint flags, PPID orgID, PPID divID)
{
	PPObjStaffList::Filt flt;
	MEMSZERO(flt);
	flt.OrgID = orgID;
	flt.DivID = divID;
	return SetupPPObjCombo(dlg, ctl, PPOBJ_STAFFLIST2, id, flags, &flt);
}
//
//
//
IMPL_INVARIANT_C(StaffAmtEntry)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(checkirange(AmtTypeID, 1, 0x00ffffff), pInvP);
	S_ASSERT_P(checkirange(CurID, 1, 0x00ffffff), pInvP);
	S_ASSERT(Period.InvariantC(pInvP));
	S_ASSERT_P(checkfrange(Amt, 0, 1.E9), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

StaffAmtEntry::StaffAmtEntry(PPID amtTypeID, PPID curID, double amt) : AmtTypeID(amtTypeID), CurID(curID), Amt(amt)
{
	Period.Z();
}

int FASTCALL StaffAmtEntry::IsEqual(const StaffAmtEntry & rS) const
{
	int    eq = 1;
	if(AmtTypeID != rS.AmtTypeID)
		eq = 0;
	else if(CurID != rS.CurID)
		eq = 0;
	else if(Period != rS.Period)
		eq = 0;
	else if(Amt != rS.Amt)
		eq = 0;
	return eq;
}
//
//
//
StaffAmtList::StaffAmtList() : TSVector <StaffAmtEntry>() // @v9.8.6 TSArray-->TSVector
{
}

int FASTCALL StaffAmtList::IsEqual(const StaffAmtList & rS) const
{
	int    eq = 1;
	const  uint c = getCount();
	const  uint c2 = rS.getCount();
	if(c != c2)
		eq = 0;
	else {
		for(uint i = 0; eq && i < c; i++) {
			const StaffAmtEntry & r_rec = at(i);
			const StaffAmtEntry & r_rec2 = rS.at(i);
			if(!r_rec.IsEqual(r_rec2))
				eq = 0;
		}
	}
	return eq;
}

int StaffAmtList::Search(PPID amtTypeID, PPID curID, LDATE dt, uint * pPos) const
{
	StaffAmtEntry * p_entry = 0;
	for(uint i = 0; enumItems(&i, reinterpret_cast<void **>(&p_entry));) {
		if(p_entry->AmtTypeID == amtTypeID && p_entry->CurID == curID && p_entry->Period.CheckDate(dt)) {
			ASSIGN_PTR(pPos, (i-1));
			return 1;
		}
	}
	return 0;
}

int StaffAmtList::Search(PPID amtTypeID, uint * pPos) const
{
	StaffAmtEntry * p_entry = 0;
	for(uint i = 0; enumItems(&i, (void **)&p_entry);) {
		if(p_entry->AmtTypeID == amtTypeID) {
			ASSIGN_PTR(pPos, (i-1));
			return 1;
		}
	}
	return 0;
}

int StaffAmtList::Get(PPID amtTypeID, PPID curID, LDATE dt, double * pAmount) const
{
	int    ok = -1;
	double amt = 0.0;
	uint   pos = 0;
	if(Search(amtTypeID, curID, dt, &pos)) {
		amt = at(pos).Amt;
		ok = 1;
	}
	ASSIGN_PTR(pAmount, amt);
	return ok;
}

int StaffAmtList::CheckDup(int pos, const StaffAmtEntry * pEntry) const
{
	StaffAmtEntry * p_entry = 0;
	for(uint i = 0; enumItems(&i, (void **)&p_entry);)
		if(pos != (int)(i-1) && p_entry->AmtTypeID == pEntry->AmtTypeID && p_entry->CurID == pEntry->CurID &&
			p_entry->Period.IsIntersect(pEntry->Period)) {
			return PPSetError(PPERR_STAFFAMTINTERSECT);
		}
	return 1;
}

int StaffAmtList::Add(const StaffAmtEntry * pItem)
{
	int    ok = 1;
	THROW_INVARG(pItem);
	THROW(CheckDup(-1, pItem));
	THROW_SL(insert(pItem));
	CATCHZOK
	return ok;
}

int StaffAmtList::Put(uint pos, const StaffAmtEntry * pItem)
{
	int    ok = 1;
	THROW_SL(checkupper(pos, getCount()));
	if(pItem) {
		THROW(CheckDup((int)pos, pItem));
		at(pos) = *pItem;
	}
	else
		atFree(pos);
	CATCHZOK
	return ok;
}
//
//
//
PersonPostArray::PersonPostArray() : TSVector <PersonPostTbl::Rec> () // @v9.8.4 TSArray-->TSVector
{
}

static IMPL_CMPFUNC(PersonPost_Closed_Finish_Dt, i1, i2)
{
	const PersonPostTbl::Rec * p1 = static_cast<const PersonPostTbl::Rec *>(i1);
	const PersonPostTbl::Rec * p2 = static_cast<const PersonPostTbl::Rec *>(i2);
	if(p1->Closed && !p2->Closed)
		return +1;
	else if(!p1->Closed && p2->Closed)
		return -1;
	else if(p1->Finish == p2->Finish)
		return CMPSIGN(p2->Dt, p1->Dt);
	else
		return CMPSIGN(p2->Finish, p1->Finish);
}

void PersonPostArray::Sort()
{
	SVector::sort(PTR_CMPFUNC(PersonPost_Closed_Finish_Dt), 0); // @v9.8.4 SArray-->SVector
}

uint PersonPostArray::GetBusyCount() const
{
	uint   c = 0;
	for(uint i = 0; i < getCount(); i++) {
		if(!at(i).Closed)
			c++;
	}
	return c;
}
//
//
//
PPStaffPacket::PPStaffPacket()
{
	Init();
}

void PPStaffPacket::Init()
{
	MEMSZERO(Rec);
	Amounts.freeAll();
}

PPStaffPacket & FASTCALL PPStaffPacket::operator = (const PPStaffPacket & rSrc)
{
	Rec = rSrc.Rec;
	Amounts.copy(rSrc.Amounts);
	return *this;
}

PPPsnPostPacket::PPPsnPostPacket()
{
	Init();
}

void PPPsnPostPacket::Init()
{
	MEMSZERO(Rec);
	Amounts.freeAll();
}

PPPsnPostPacket & FASTCALL PPPsnPostPacket::operator = (const PPPsnPostPacket & rSrc)
{
	Rec = rSrc.Rec;
	Amounts.copy(rSrc.Amounts);
	return *this;
}

TLP_IMPL(PPObjStaffList, PersonPostTbl, P_PostTbl);

PPObjStaffList::PPObjStaffList(void * extraPtr) : PPObjReference(PPOBJ_STAFFLIST2, extraPtr)
{
	ImplementFlags |= implStrAssocMakeList;
	TLP_OPEN(P_PostTbl);
}

PPObjStaffList::~PPObjStaffList()
{
	TLP_CLOSE(P_PostTbl);
}

int PPObjStaffList::DeleteObj(PPID id)
{
	return PutPacket(&id, 0, 0);
}

int PPObjStaffList::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTSTAFF, CTL_RTSTAFF_FLAGS, CTL_RTSTAFF_SFLAGS, bufSize, rt, pDlg);
}

int PPObjStaffList::SerializePacket(int dir, PPStaffPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, &pPack->Amounts, rBuf));
	CATCHZOK
	return ok;
}

int PPObjStaffList::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjStaffList, PPStaffPacket>(this, p, id, stream, pCtx); }

int PPObjStaffList::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPStaffPacket * p_pack = static_cast<PPStaffPacket *>(p->Data);
		if(stream == 0) {
			if(*pID == 0) {
				PPID   same_id = 0;
				//
				// Для объекта PPObjStaffList зарезервированное пространство [1..PP_FIRSTUSRREF-1] не применимо
				//
				if(ref->SearchSymb(Obj, &same_id, p_pack->Rec.Name, offsetof(PPStaffEntry, Name)) > 0 ||
					ref->SearchSymb(Obj, &same_id, p_pack->Rec.Symb, offsetof(PPStaffEntry, Symb)) > 0) {
					PPStaffEntry same_rec;
					if(Search(same_id, &same_rec) > 0) {
						if(!PutPacket(&same_id, p_pack, 1)) {
							pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSTAFFENTRY, p_pack->Rec.ID, p_pack->Rec.Name);
							ok = -1;
						}
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_pack->Rec.ID = 0;
					if(!CheckRights(PPR_INS) || !PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSTAFFENTRY, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
				}
			}
			else {
				if(!(p->Flags & PPObjPack::fDispatcher)) {
					p_pack->Rec.ID = *pID;
					if(!CheckRights(PPR_MOD) || !PutPacket(pID, p_pack, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSTAFFENTRY, p_pack->Rec.ID, p_pack->Rec.Name);
						ok = -1;
					}
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

int PPObjStaffList::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		PPStaffPacket * p_pack = static_cast<PPStaffPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->Rec.OrgID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->Rec.DivisionID, ary, replace));
		for(uint i = 0; i < p_pack->Amounts.getCount(); i++) {
			StaffAmtEntry & r_item = p_pack->Amounts.at(i);
			THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE,  &r_item.AmtTypeID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_CURRENCY,    &r_item.CurID, ary, replace));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_PERSON) {
			PPStaffEntry rec;
			for(SEnum en = ref->EnumByIdxVal(Obj, 1, _id); ok && en.Next(&rec) > 0;) {
				ok = RetRefsExistsErr(Obj, rec.ID);
			}
			if(ok) {
				PersonPostTbl::Key2 k2;
				MEMSZERO(k2);
				k2.PersonID = _id;
				if(P_PostTbl->search(2, &k2, spGe) && k2.PersonID == _id)
					ok = RetRefsExistsErr(PPOBJ_PERSONPOST, P_PostTbl->data.ID);
			}
		}
		else if(_obj == PPOBJ_LOCATION) {
			PPStaffEntry rec;
			for(SEnum en = ref->EnumByIdxVal(Obj, 2, _id); ok && en.Next(&rec) > 0;) {
				ok = RetRefsExistsErr(Obj, rec.ID);
			}
		}
	}
	return ok;
}

int PPObjStaffList::Recover(PPID staffID, int use_ta)
{
	int    ok = -1;
	uint   busy_count = 0;
	PPStaffEntry staff_rec;
	PersonPostArray post_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(staffID, &staff_rec) > 0);
		THROW(GetPostList(staffID, &post_list));
		busy_count = post_list.GetBusyCount();
		if(staff_rec.VacancyBusy != busy_count) {
			staff_rec.VacancyBusy = busy_count;
			THROW(UpdateItem(staffID, &staff_rec, 0));
			ok = 2;
		}
		else
			ok = 1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::IncrementStaffVacancy(PPID staffID, int decr, int use_ta)
{
	int    ok = 1;
	PPStaffEntry staff_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(staffID, &staff_rec) > 0);
		if(decr) {
			if(staff_rec.VacancyBusy > 0)
				staff_rec.VacancyBusy--;
		}
		else {
			THROW_PP(staff_rec.VacancyBusy < staff_rec.VacancyCount, PPERR_NOVACANCY);
			staff_rec.VacancyBusy++;
		}
		THROW(UpdateItem(staffID, &staff_rec, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::AssignPersonToStaff(PPID psnID, PPID staffID, LDATE dt, int use_ta)
{
	PPPsnPostPacket pack;
	PPID   post_id = 0;
	pack.Rec.PersonID = psnID;
	pack.Rec.StaffID  = staffID;
	pack.Rec.Dt       = dt;
	return PutPostPacket(&post_id, &pack, use_ta);
}

int PPObjStaffList::SearchPost(PPID postID, PersonPostTbl::Rec * pRec)
{
	return SearchByID(P_PostTbl, PPOBJ_PERSONPOST, postID, pRec);
}

int PPObjStaffList::GetPostPacket(PPID postID, PPPsnPostPacket * pPack)
{
	int    ok = -1;
	PPPsnPostPacket pack;
	if(SearchByID(P_PostTbl, PPOBJ_PERSONPOST, postID, &pack.Rec) > 0) {
		ok = ref->GetPropArray(PPOBJ_PERSONPOST, postID, PSNPPPRP_AMTLIST, &pack.Amounts) ? 1 : 0;
		ASSIGN_PTR(pPack, pack);
	}
	return ok;
}

int PPObjStaffList::PutPostPacket(PPID * pID, PPPsnPostPacket * pPack, int use_ta)
{
	int    ok = 1, r;
	SString msg_buf;
	PPStaffEntry sl_rec;
	PersonTbl::Rec psn_rec;
	PersonPostTbl::Rec post_rec; // Временная запись
	SalaryCore * p_sal_tbl = (*pID && pPack == 0) ? new SalaryCore : 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PersonPostTbl::Rec org_rec;
			THROW(SearchPost(*pID, &org_rec) > 0);
			if(pPack) {
				//
				// Если запись не изменилась, то и суетиться не следует
				//
				int    was_updated = 0;
				// @v10.3.0 (never used) int    was_list_updated = 0;
				if(memcmp(&org_rec, &pPack->Rec, sizeof(PersonPostTbl::Rec)) != 0) {
					was_updated = 1;
					//
					// Извлекаем должность и персоналию для проверки ссылок и вспомогательных целей
					//
					THROW(Fetch(pPack->Rec.StaffID, &sl_rec) > 0);
					THROW(PsnObj.Fetch(pPack->Rec.PersonID, &psn_rec) > 0);
					if(pPack->Rec.Closed) {
						//
						// У закрытого назначения нельзя менять ни персоналию ни должность
						//
						THROW_PP(pPack->Rec.PersonID == org_rec.PersonID, PPERR_CLOSEDPOSTUPD);
						THROW_PP(pPack->Rec.StaffID == org_rec.StaffID, PPERR_CLOSEDPOSTUPD);
						if(org_rec.Closed == 0) {
							//
							// Если статус назначения изменился с "открытого" на закрытый, то
							// устанавливаем значение Closed (максимальным+1) для сочетания {StaffID, PersonID}
							//
							THROW(r = GetPersonPost(-1, pPack->Rec.StaffID, pPack->Rec.PersonID, &post_rec));
							pPack->Rec.Closed = (r > 0) ? (post_rec.Closed + 1) : 1;
							//
							// Уменьшаем (с проверкой корректности) количество занятых вакансий
							//
							THROW(IncrementStaffVacancy(pPack->Rec.StaffID, 1, 0));
						}
					}
					THROW(UpdateByID(P_PostTbl, PPOBJ_PERSONPOST, *pID, &pPack->Rec, 0));
				}
				{
					StaffAmtList org_list;
					THROW(ref->GetPropArray(PPOBJ_PERSONPOST, *pID, PSNPPPRP_AMTLIST, &org_list));
					if(!org_list.IsEqual(pPack->Amounts)) {
						was_updated = 1;
						// @v10.3.0 (never used) was_list_updated = 1;
						THROW(ref->PutPropArray(PPOBJ_PERSONPOST, *pID, SLPPRP_AMTLIST, &pPack->Amounts, 0));
					}
				}
				if(was_updated)
					DS.LogAction(PPACN_OBJUPD, PPOBJ_PERSONPOST, *pID, 0, 0);
			}
			else {
				//
				// Удаление назначения //
				// Не путать с закрытием:
				// Удаление - радикальная операция, стирающая все следы назначения.
				// Закрытие - аккуратная учетная операция, отмечающая назначение как закрытое, но
				//   не удаляющая его.
				//
				if(p_sal_tbl) {
					DateRange period;
					period.Set(ZERODATE, MAXDATE);
					PPIDArray sal_list;
					double amt = 0.0;
					THROW(p_sal_tbl->GetListByObject(PPOBJ_PERSONPOST, *pID, period, 0, &sal_list, &amt));
					THROW_PP(sal_list.getCount() == 0, PPERR_RMVPPOSTHASSALARY);
				}
				THROW(RemoveByID(P_PostTbl, *pID, 0));
				THROW(ref->PutPropArray(Obj, *pID, SLPPRP_AMTLIST, 0, 0));
				//
				// Уменьшаем (с проверкой корректности) количество занятых вакансий
				//
				THROW(IncrementStaffVacancy(org_rec.StaffID, 1, 0));
				DS.LogAction(PPACN_OBJRMV, PPOBJ_PERSONPOST, *pID, 0, 0);
			}
			DirtyPost(*pID);
		}
		else if(pPack) {
			//
			// Закрытое штатное назначение создавать нельзя (оно может возникнуть только
			// в результате отзыва должности у персоналии).
			//
			THROW_PP(pPack->Rec.Closed == 0, PPERR_CLOSEDPPOSTCREATING);
			//
			// Извлекаем должность и персоналию для проверки ссылок и вспомогательных целей
			//
			THROW(Fetch(pPack->Rec.StaffID, &sl_rec) > 0);
			THROW(PsnObj.Fetch(pPack->Rec.PersonID, &psn_rec) > 0);
			//
			// Проверяем, чтобы персоналия не назначалась дважды на одну должность.
			//
			THROW(r = GetPersonPost(0, pPack->Rec.StaffID, pPack->Rec.PersonID, &post_rec));
			msg_buf.Z().Cat(psn_rec.Name).CatDiv('-', 1).Cat(sl_rec.Name);
			THROW_PP_S(r < 0, PPERR_DUPPERSONPOST, msg_buf);
			THROW(AddObjRecByID(P_PostTbl, PPOBJ_PERSONPOST, pID, &pPack->Rec, 0));
			THROW(ref->PutPropArray(PPOBJ_PERSONPOST, *pID, PSNPPPRP_AMTLIST, &pPack->Amounts, 0));
			//
			// Увеличиваем (с проверкой корректности) количество занятых вакансий
			//
			THROW(IncrementStaffVacancy(pPack->Rec.StaffID, 0, 0));
			DS.LogAction(PPACN_OBJADD, PPOBJ_PERSONPOST, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_sal_tbl;
	return ok;
}

int PPObjStaffList::MakeCodeString(const PersonPostTbl::Rec * pRec, SString & rBuf)
{
	PPStaffEntry sl_rec;
	PersonTbl::Rec psn_rec;
	rBuf.Z();
	if(Fetch(pRec->StaffID, &sl_rec) > 0)
		rBuf.Cat(sl_rec.Name);
	else
		ideqvalstr(pRec->StaffID, rBuf);
	rBuf.CatDiv('-', 1);
	if(PsnObj.Fetch(pRec->PersonID, &psn_rec) > 0)
		rBuf.Cat(psn_rec.Name);
	else
		ideqvalstr(pRec->PersonID, rBuf);
	return 1;
}

int PPObjStaffList::RevokePersonPost(PPID personID, PPID staffID, LDATE dt, int use_ta)
{
	int    ok = -1;
	PersonPostTbl::Rec rec;
	if(GetPersonPost(0, staffID, personID, &rec) > 0) {
		PPPsnPostPacket pack;
		THROW(GetPostPacket(rec.ID, &pack) > 0);
		pack.Rec.Closed = 1;
		pack.Rec.Finish = dt;
		THROW(PutPostPacket(&rec.ID, &pack, use_ta));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::RevokeAllPersonPosts(PPID personID, PPID orgID, LDATE dt, int use_ta)
{
	int    ok = -1, r;
	PersonPostArray post_list;
	PersonPostTbl::Rec * p_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = GetPostByPersonList(personID, orgID, 1, &post_list));
		for(uint i = 0; post_list.enumItems(&i, (void **)&p_rec);) {
			PPPsnPostPacket pack;
			THROW(GetPostPacket(p_rec->ID, &pack) > 0);
			pack.Rec.Closed = 1;
			pack.Rec.Finish = dt;
			THROW(PutPostPacket(&p_rec->ID, &pack, 0));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::GetPersonPostList(PPID staffID, PPID personID, SArray * pList)
{
	int    ok = -1;
	if(pList == 0 || pList->getItemSize() != sizeof(PersonPostTbl::Rec))
		ok = PPSetErrorInvParam();
	else {
		PersonPostTbl::Key1 k1;
		k1.StaffID  = staffID;
		k1.PersonID = personID;
		k1.Closed = -1;
		while(P_PostTbl->search(1, &k1, spGt) && k1.StaffID == staffID && k1.PersonID == personID) {
			pList->insert(&P_PostTbl->data);
			ok = 1;
		}
		if(!BTROKORNFOUND)
			ok = PPSetErrorDB();
	}
	return ok;
}

int PPObjStaffList::GetPersonPost(int closed, PPID staffID, PPID personID, PersonPostTbl::Rec * pRec)
{
	int    ok = -1, sp = spEq;
	PersonPostTbl::Key1 k1;
	k1.StaffID  = staffID;
	k1.PersonID = personID;
	if(closed < 0) {
		k1.Closed = MAXSHORT;
		sp = spLe;
	}
	else {
		k1.Closed = closed;
		sp = spEq;
	}
	if(P_PostTbl->search(1, &k1, sp)) {
		if(closed >= 0 || (k1.StaffID == staffID && k1.PersonID == personID && k1.Closed)) {
			P_PostTbl->copyBufTo(pRec);
			ok = 1;
		}
	}
	else if(!BTROKORNFOUND)
		ok = PPSetErrorDB();
	return ok;
}

int PPObjStaffList::GetList(const Filt & rFilt, PPIDArray * pList, StrAssocArray * pNameList)
{
	int    ok = -1;
	{
		SEnum en;
		if(rFilt.DivID)
			en = ref->EnumByIdxVal(Obj, 2, rFilt.DivID);
		else if(rFilt.OrgID)
			en = ref->EnumByIdxVal(Obj, 1, rFilt.OrgID);
		else
			en = ref->Enum(Obj, 0);
		PPStaffEntry rec;
		while(en.Next(&rec) > 0) {
			if((!rFilt.OrgID || rec.OrgID == rFilt.OrgID) && (!rFilt.DivID || rec.DivisionID == rFilt.DivID)) {
				if(pNameList)
					THROW_SL(pNameList->Add(rec.ID, rec.Name));
				if(pList)
					THROW(pList->addUnique(rec.ID));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::GetPostByPersonList(PPID personID, PPID employerID, int openedOnly, PersonPostArray * pList)
{
	int    ok = -1;
	PersonPostTbl::Key2 k2;
	MEMSZERO(k2);
	k2.PersonID  = personID;
	THROW_INVARG(!pList || pList->getItemSize() == sizeof(P_PostTbl->data));
	while(P_PostTbl->search(2, &k2, spGt) && k2.PersonID == personID) {
		if((openedOnly > 0 && !P_PostTbl->data.Closed) || (openedOnly < 0 && P_PostTbl->data.Closed)) {
			if(employerID) {
				PPStaffEntry sl_rec;
				if(Fetch(P_PostTbl->data.StaffID, &sl_rec) > 0 && sl_rec.OrgID == employerID)
					ok = 1;
			}
			else
				ok = 1;
			if(ok > 0 && pList)
				THROW_SL(pList->insert(&P_PostTbl->data));
		}
	}
	THROW_DB(ok > 0 || BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int PPObjStaffList::GetPostList(PPID staffID, PersonPostArray * pList)
{
	int    ok = -1;
	PersonPostTbl::Key1 k1;
	MEMSZERO(k1);
	k1.StaffID  = staffID;
	THROW_INVARG(!pList || pList->getItemSize() == sizeof(P_PostTbl->data));
	while(P_PostTbl->search(1, &k1, spGt) && k1.StaffID == staffID) {
		CALLPTRMEMB(pList, insert(&P_PostTbl->data));
		ok = 1;
	}
	THROW_DB(ok > 0 || BTROKORNFOUND);
	CATCHZOK
	return ok;
}

#define GRP_DIV 1

class StaffDialog : public TDialog {
	DECL_DIALOG_DATA(PPStaffPacket);
public:
	StaffDialog() : TDialog(DLG_STAFF)
	{
		addGroup(GRP_DIV, new DivisionCtrlGroup(CTLSEL_STAFF_ORG, CTLSEL_STAFF_DIV, 0, 0));
		enableCommand(cmAmounts, SlObj.CheckRights(PPObjStaffList::rtReadAmounts));
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init();
		{
			DivisionCtrlGroup::Rec grp_rec(Data.Rec.OrgID, Data.Rec.DivisionID);
			setGroupData(GRP_DIV, &grp_rec);
		}
		setCtrlData(CTL_STAFF_NAME,   Data.Rec.Name);
		setCtrlData(CTL_STAFF_RANK,   &Data.Rec.Rank);
		setCtrlData(CTL_STAFF_COUNT,  &Data.Rec.VacancyCount);
		SetupStringCombo(this, CTLSEL_STAFF_FIX, PPTXT_FIXSTAFFLIST, Data.Rec.FixedStaff);
		SetupPPObjCombo(this, CTLSEL_STAFF_CHARGE, PPOBJ_SALCHARGE, Data.Rec.ChargeGrpID, OLW_CANINSERT, reinterpret_cast<void *>(-1000));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_STAFF_NAME,   Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
		getCtrlData(CTL_STAFF_COUNT,  &Data.Rec.VacancyCount);
		getCtrlData(CTLSEL_STAFF_FIX, &Data.Rec.FixedStaff);
		{
			DivisionCtrlGroup::Rec grp_rec;
			getGroupData(GRP_DIV, &grp_rec);
			Data.Rec.OrgID      = grp_rec.OrgID;
			Data.Rec.DivisionID = grp_rec.DivID;
		}
		getCtrlData(CTL_STAFF_RANK,   &Data.Rec.Rank);
		getCtrlData(CTL_STAFF_CHARGE, &Data.Rec.ChargeGrpID);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmAmounts)) {
			if(SlObj.CheckRights(PPObjStaffList::rtReadAmounts)) {
				SString word;
				// @v9.2.7 PPGetWord(PPWORD_STAFFAMTLIST, 0, word);
				PPLoadString("staffamount_pl", word); // @v9.2.7
				EditStaffAmtList(&Data.Amounts, word, SlObj.CheckRights(PPObjStaffList::rtModAmounts));
			}
			clearEvent(event);
		}
	}
	PPObjStaffList SlObj;
};

int PPObjStaffList::EditDialog(PPStaffPacket * pPack) { DIALOG_PROC_BODY(StaffDialog, pPack); }

int PPObjStaffList::GetPacket(PPID id, PPStaffPacket * pPack)
{
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		ok = ref->GetPropArray(Obj, id, SLPPRP_AMTLIST, &pPack->Amounts) ? 1 : 0;
	}
	return ok;
}

int PPObjStaffList::PutPacket(PPID * pID, PPStaffPacket * pPack, int use_ta)
{
	int    ok = 1, r;
	uint   i;
	PPStaffEntry rec;
	PPIDArray idlist;
	if(pPack && pPack->Rec.FixedStaff)
		THROW(GetFixedStaffList(pPack->Rec.OrgID, pPack->Rec.FixedStaff, &idlist));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				for(i = 0; i < idlist.getCount(); i++) {
					const PPID staff_id = idlist.get(i);
					if(staff_id != *pID && Search(staff_id, &rec) > 0) {
						rec.FixedStaff = 0;
						THROW(UpdateItem(staff_id, &rec, 0));
					}
				}
				THROW(UpdateItem(*pID, &pPack->Rec, 0));
				THROW(ref->PutPropArray(Obj, *pID, SLPPRP_AMTLIST, &pPack->Amounts, 0));
				DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(r = GetPostList(*pID, 0));
				THROW_PP(r < 0, PPERR_RMVFILLEDSTAFF);
				THROW(ref->RemoveItem(Obj, *pID, 0));
				THROW(ref->RemoveProperty(Obj, *pID, 0, 0));
				THROW(RemoveSync(*pID));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			Dirty(*pID);
		}
		else {
			for(i = 0; i < idlist.getCount(); i++) {
				const PPID staff_id = idlist.get(i);
				if(Search(staff_id, &rec) > 0) {
					rec.FixedStaff = 0;
					THROW(UpdateItem(staff_id, &rec, 0));
				}
			}
			THROW(AddItem(pID, &pPack->Rec, 0));
			THROW(ref->PutPropArray(Obj, *pID, SLPPRP_AMTLIST, &pPack->Amounts, 0));
			DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1, r = cmCancel, is_new = 0;
	PPStaffPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		pack.Init();
		if(extraPtr) {
			pack.Rec.OrgID      = static_cast<const Filt *>(extraPtr)->OrgID;
			pack.Rec.DivisionID = static_cast<const Filt *>(extraPtr)->DivID;
		}
		pack.Rec.Rank         = 100;
		pack.Rec.VacancyCount = 1;
	}
	if(EditDialog(&pack) > 0) {
		THROW(PutPacket(pID, &pack, 1));
		r = cmOK;
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjStaffList::EditAmounts(PPID id)
{
	int    ok = -1, r = cmCancel;
	PPStaffPacket pack;
	THROW(CheckRights(PPR_MOD) && CheckRights(PPObjStaffList::rtReadAmounts));
	THROW(GetPacket(id, &pack) > 0);
	{
		SString word;
		// @v9.2.7 PPGetWord(PPWORD_STAFFAMTLIST, 0, word);
		PPLoadString("staffamount_pl", word); // @v9.2.7
		while(ok < 0 && EditStaffAmtList(&pack.Amounts, word, CheckRights(PPObjStaffList::rtModAmounts)) > 0) {
			if(PutPacket(&id, &pack, 1))
				ok = 1;
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjStaffList::EditPostAmounts(PPID id)
{
	int    ok = -1;
	PPPsnPostPacket pack;
	THROW(CheckRights(PPR_MOD) && CheckRights(PPObjStaffList::rtReadAmounts));
	THROW(GetPostPacket(id, &pack) > 0);
	{
		SString word;
		// @v9.2.7 PPGetWord(PPWORD_STAFFAMTLIST, 0, word);
		PPLoadString("staffamount_pl", word); // @v9.2.7
		while(ok < 0 && EditStaffAmtList(&pack.Amounts, word, CheckRights(PPObjStaffList::rtModAmounts)) > 0)
			if(PutPostPacket(&id, &pack, 1))
				ok = 1;
			else
				PPError();
	}
	CATCHZOKPPERR
	return ok;
}

#define GRP_DIV 1

class PersonPostDialog : public TDialog {
public:
	PersonPostDialog() : TDialog(DLG_PERSONPOST), Flags(0), PrevClosedVal(0), FixedPost(0)
	{
		addGroup(GRP_DIV, new DivisionCtrlGroup(
			CTLSEL_PERSONPOST_ORG, CTLSEL_PERSONPOST_DIV, CTLSEL_PERSONPOST_POST, 0));
		SetupCalDate(CTLCAL_PERSONPOST_DATE,   CTL_PERSONPOST_DATE);
		SetupCalDate(CTLCAL_PERSONPOST_FINISH, CTL_PERSONPOST_FINISH);
		enableCommand(cmAmounts, SlObj.CheckRights(PPObjStaffList::rtReadAmounts));
		setCtrlReadOnly(CTL_PERSONPOST_ID, 1);
	}
	int    setDTS(const PPPsnPostPacket * pData, long flags);
	int    getDTS(PPPsnPostPacket * pData);
private:
	DECL_HANDLE_EVENT;
	int    FixedPost;
	int    PrevClosedVal;
	long   Flags; // PPObjStaffList::epdfXXX
	PPPsnPostPacket Data;
	PPObjStaffList SlObj;
};

IMPL_HANDLE_EVENT(PersonPostDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmAmounts)) {
		if(SlObj.CheckRights(PPObjStaffList::rtReadAmounts)) {
			SString word;
			// @v9.2.7 PPGetWord(PPWORD_STAFFAMTLIST, 0, word);
			PPLoadString("staffamount_pl", word); // @v9.2.7
			EditStaffAmtList(&Data.Amounts, word, SlObj.CheckRights(PPObjStaffList::rtModAmounts));
		}
	}
	else if(event.isClusterClk(CTL_PERSONPOST_CLOSED)) {
		if(getCtrlUInt16(CTL_PERSONPOST_CLOSED)) {
			Data.Rec.Closed = 1;
			Data.Rec.Finish = getcurdate_();
		}
		else {
			Data.Rec.Closed = 0;
			Data.Rec.Finish = ZERODATE;
		}
		setCtrlData(CTL_PERSONPOST_FINISH, &Data.Rec.Finish);
	}
	else
		return;
	clearEvent(event);
}

int PersonPostDialog::setDTS(const PPPsnPostPacket * pData, long flags)
{
	int    ok = 1;
	PPStaffEntry sl_rec;
	DivisionCtrlGroup::Rec dcgrec;
	Flags = flags;
	if(pData)
		Data = *pData;
	else
		Data.Init();
	PrevClosedVal = Data.Rec.Closed;
	if(SlObj.Fetch(Data.Rec.StaffID, &sl_rec) > 0) {
		dcgrec.OrgID = sl_rec.OrgID;
		dcgrec.DivID = sl_rec.DivisionID;
	}
	dcgrec.StaffID = Data.Rec.StaffID;
	setGroupData(GRP_DIV, &dcgrec);
	disableCtrls(BIN(Flags & PPObjStaffList::epdfFixedPost), CTLSEL_PERSONPOST_ORG, CTLSEL_PERSONPOST_DIV, CTLSEL_PERSONPOST_POST, 0);
	setCtrlReadOnly(CTL_PERSONPOST_ID, !BIN(Flags & PPObjStaffList::epdfRecover));
	setCtrlLong(CTL_PERSONPOST_ID, Data.Rec.ID);
	SetupPPObjCombo(this, CTLSEL_PERSONPOST_PERSON, PPOBJ_PERSON, Data.Rec.PersonID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
	SetupPPObjCombo(this, CTLSEL_PERSONPOST_CHARGE, PPOBJ_SALCHARGE, Data.Rec.ChargeGrpID, OLW_CANINSERT, reinterpret_cast<void *>(-1000));
	disableCtrl(CTLSEL_PERSONPOST_PERSON, Data.Rec.ID && !(Flags & PPObjStaffList::epdfRecover));
	setCtrlData(CTL_PERSONPOST_DATE, &Data.Rec.Dt);
	setCtrlData(CTL_PERSONPOST_FINISH, &Data.Rec.Finish);
	setCtrlUInt16(CTL_PERSONPOST_CLOSED, BIN(Data.Rec.Closed));
	disableCtrl(CTL_PERSONPOST_CLOSED, Data.Rec.Closed);
	return ok;
}

int PersonPostDialog::getDTS(PPPsnPostPacket * pData)
{
	int    ok = 1;
	DivisionCtrlGroup::Rec dcgrec;
	if(!(Flags & PPObjStaffList::epdfFixedPost)) {
		getGroupData(GRP_DIV, &dcgrec);
		Data.Rec.StaffID = dcgrec.StaffID;
	}
	if(Flags & PPObjStaffList::epdfRecover)
		Data.Rec.ID = getCtrlLong(CTL_PERSONPOST_ID);
	getCtrlData(CTLSEL_PERSONPOST_PERSON, &Data.Rec.PersonID);
	if(!Data.Rec.PersonID)
		ok = PPErrorByDialog(this, CTL_PERSONPOST_PERSON, PPERR_PERSONNEEDED);
	else {
		getCtrlData(CTLSEL_PERSONPOST_CHARGE, &Data.Rec.ChargeGrpID);
		getCtrlData(CTL_PERSONPOST_DATE,   &Data.Rec.Dt);
		getCtrlData(CTL_PERSONPOST_FINISH, &Data.Rec.Finish);
		if(getCtrlUInt16(CTL_PERSONPOST_CLOSED) && !PrevClosedVal)
			Data.Rec.Closed = 1;
		*pData = Data;
	}
	return ok;
}

int PPObjStaffList::EditPostDialog(PPPsnPostPacket * pPack, long flags)
{
	int    ok = -1;
	PersonPostDialog * dlg = new PersonPostDialog;
	THROW(CheckDialogPtr(&dlg));
	if(pPack->Rec.ID == 0) {
		if(pPack->Rec.StaffID)
			THROW(Search(pPack->Rec.StaffID) > 0);
	}
	dlg->setDTS(pPack, flags);
	while(ok < 0 && ExecView(dlg) == cmOK)
		if(dlg->getDTS(pPack))
			ok = 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

StrAssocArray * PPObjStaffList::MakeStrAssocList(void * extraPtr)
{
	Filt filt;
	if(extraPtr)
		filt = *static_cast<const Filt *>(extraPtr);
	else {
		GetMainEmployerID(&filt.OrgID);
		filt.DivID = 0;
	}
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	THROW(GetList(filt, 0, p_list));
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjStaffList::MakePostStrAssocList(PPID orgID, PPID divID, PPID staffID, StrAssocArray * pList)
{
	int    ok = 1;
	uint   i;
	SString temp_buf;
	PPIDArray staff_list;
	PersonPostArray post_list;
	PersonPostTbl::Rec * p_post_rec;
	THROW_INVARG(pList);
	pList->Z();
	if(staffID) {
		THROW_SL(staff_list.add(staffID));
	}
	else {
		Filt filt;
		MEMSZERO(filt);
		filt.OrgID = orgID;
		filt.DivID = divID;
		THROW(GetList(filt, &staff_list, 0));
	}
	for(i = 0; i < staff_list.getCount(); i++) {
		THROW(GetPostList(staff_list.at(i), &post_list));
	}
	for(i = 0; post_list.enumItems(&i, (void **)&p_post_rec);) {
		THROW(MakeCodeString(p_post_rec, temp_buf));
		THROW_SL(pList->Add(p_post_rec->ID, temp_buf));
	}
	CATCH
		ok = 0;
		pList->Z();
	ENDCATCH
	return ok;
}

int PPObjStaffList::Browse(void * extraPtr /*lFilt*/)
{
	const Filt * p_obj_filt = static_cast<const Filt *>(extraPtr);
	StaffListFilt * p_filt = 0;
	StaffListFilt filt;
	if(p_obj_filt) {
		filt.OrgID = p_obj_filt->OrgID;
		filt.DivID = p_obj_filt->DivID;
		p_filt = &filt;
	}
	return PPView::Execute(PPVIEW_STAFFLIST, p_filt, PPView::exefModeless, 0);
}
//
//
//
int PPObjStaffList::GetFixedStaffList(PPID orgID, PPID fixID, PPIDArray * pList)
{
	CALLPTRMEMB(pList, clear());
	int    ok = 1;
	PPStaffEntry rec;
	for(SEnum en = ref->EnumByIdxVal(Obj, 1, orgID); en.Next(&rec) > 0;) {
		if(rec.FixedStaff == fixID) {
			if(pList)
				THROW_SL(pList->add(rec.ID));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjStaffList::CreateFixedStaff(PPID * pID, PPID orgID, PPID divID, PPID fixID, int use_ta)
{
	int    ok = -1;
	SString str, item_buf, id_str, name_str;
	if(PPLoadText(PPTXT_FIXSTAFFLIST, str)) {
		for(int idx = 0; ok < 0 && PPGetSubStr(str, idx, item_buf) > 0; idx++) {
			item_buf.Divide(',', id_str, name_str);
			long   id = id_str.ToLong();
			if(id == 0)
				id = idx+1;
			if(id == fixID) {
				PPStaffEntry rec;
				MEMSZERO(rec);
				STRNSCPY(rec.Name, name_str);
				rec.OrgID      = orgID;
				rec.DivisionID = divID;
				rec.FixedStaff = fixID;
				rec.Rank       = 100;
				rec.VacancyCount = 1;
				ok = AddItem(pID, &rec, use_ta);
			}
		}
	}
	return ok;
}

int PPObjStaffList::GetFixedPostList(PPID orgID, PPID fixID, PersonPostArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, freeAll());
	PPIDArray idlist;
	GetFixedStaffList(orgID, fixID, &idlist);
	if(idlist.getCount()) {
		const PPID staff_id = idlist.get(0);
		if(staff_id)
			ok = GetPostList(staff_id, pList);
	}
	return ok;
}

int PPObjStaffList::GetFixedPostOnDate(PPID orgID, PPID fixID, LDATE dt, PersonPostTbl::Rec * pRec)
{
	int    ok = -1;
	int    pos = -1;
	PersonPostArray post_list;
	if(GetFixedPostList(orgID, fixID, &post_list) > 0) {
		int    def_pos = -1;
		post_list.Sort();
		for(uint i = 0; i < post_list.getCount(); i++) {
			const PersonPostTbl::Rec & r_item = post_list.at(i);
			if(!r_item.Closed && def_pos < 0)
				def_pos = i;
			if(!dt) {
				if(!r_item.Closed) {
					pos = i;
					break;
				}
			}
			else if(dt >= r_item.Dt && dt <= r_item.Finish) {
				pos = i;
				break;
			}
		}
		if(pos >= 0)
			ok = 1;
		else if(def_pos >= 0) {
			pos = def_pos;
			ok = 2;
		}
	}
	if(pos >= 0) {
		ASSIGN_PTR(pRec, post_list.at(pos));
	}
	else
		memzero(pRec, sizeof(*pRec));
	return ok;
}

int PPObjStaffList::EditFixedStaffPost(PPID orgID)
{
	int    ok = -1;
	TDialog * dlg = 0;
	struct StaffPair {
		PPID   PostID;
		PPID   StaffID;
		PPID   PsnID;
	};
	PersonTbl::Rec org_rec;
	PersonPostTbl::Rec dir;
	PersonPostTbl::Rec acc;
	MEMSZERO(dir);
	MEMSZERO(acc);
	THROW(PsnObj.Search(orgID, &org_rec) > 0);
	THROW(GetFixedPostOnDate(orgID, PPFIXSTF_DIRECTOR, ZERODATE, &dir));
	THROW(GetFixedPostOnDate(orgID, PPFIXSTF_ACCOUNTANT, ZERODATE, &acc));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_FIXSTAFF))));
	dlg->setStaticText(CTL_FIXSTAFF_ORGNAME, org_rec.Name);
	SetupPPObjCombo(dlg, CTLSEL_FIXSTAFF_DIRECTOR, PPOBJ_PERSON, dir.PersonID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
	SetupPPObjCombo(dlg, CTLSEL_FIXSTAFF_ACCTNT, PPOBJ_PERSON, acc.PersonID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
	if(ExecView(dlg) == cmOK) {
		PPID   new_dir_id = dlg->getCtrlLong(CTLSEL_FIXSTAFF_DIRECTOR);
		PPID   new_acc_id = dlg->getCtrlLong(CTLSEL_FIXSTAFF_ACCTNT);
		const  LDATE oper_date = LConfig.OperDate;
		PPTransaction tra(1);
		THROW(tra);
		if(dir.PersonID != new_dir_id) {
			if(dir.StaffID == 0) {
				THROW(CreateFixedStaff(&dir.StaffID, orgID, 0, PPFIXSTF_DIRECTOR, 0));
			}
			else if(dir.ID) {
				THROW(RevokePersonPost(dir.PersonID, dir.StaffID, oper_date, 0));
			}
			THROW(AssignPersonToStaff(new_dir_id, dir.StaffID, ZERODATE, 0));
		}
		if(acc.PersonID != new_acc_id) {
			if(acc.StaffID == 0) {
				THROW(CreateFixedStaff(&acc.StaffID, orgID, 0, PPFIXSTF_ACCOUNTANT, 0));
			}
			else if(acc.ID) {
				THROW(RevokePersonPost(acc.PersonID, acc.StaffID, oper_date, 0));
			}
			THROW(AssignPersonToStaff(new_acc_id, acc.StaffID, ZERODATE, 0));
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok;
}

/*static*/int PPObjStaffList::SetupPostCombo(TDialog * dlg, uint ctl, PPID id, uint /*olwFlags*/, PPID orgID, PPID divID, PPID staffID)
{
	int    ok = -1;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(p_combo) {
		PPObjStaffList sl_obj;
		StrAssocArray * p_list = new StrAssocArray;
		ListWindow * p_lw = 0;
		THROW_MEM(p_list);
		THROW(sl_obj.MakePostStrAssocList(orgID, divID, staffID, p_list));
		THROW_MEM(p_lw = new ListWindow(new StrAssocListBoxDef(p_list, lbtDisposeData|lbtDblClkNotify), 0, 0));
		p_combo->setListWindow(p_lw, id);
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class StaffListCache : public ObjCache {
public:
	struct Data : public ObjCacheEntry {
		PPID   OrgID;         // ->Person.ID Работодатель
		PPID   DivisionID;    // ->Location.ID (LOCTYP_DIVISION)
		long   Rank;
		long   Flags;
		long   FixedStaff;    // Зарезервированный ИД должности (PPFIXSTF_XXX)
		long   ChargeGrpID;   // ->Ref(PPOBJ_SALCHARGE) Группа начислений, используемая для этой должности
	};
	StaffListCache() : ObjCache(PPOBJ_STAFFLIST2, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int StaffListCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjStaffList sl_obj;
	PPStaffEntry rec;
	if(sl_obj.Search(id, &rec) > 0) {
		#define FLD(f) p_cache_rec->f = rec.f
		FLD(OrgID);
		FLD(DivisionID);
		FLD(Flags);
		FLD(Rank);
		FLD(FixedStaff);
		FLD(ChargeGrpID);
		#undef FLD
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void StaffListCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPStaffEntry * p_data_rec = static_cast<PPStaffEntry *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(OrgID);
	FLD(DivisionID);
	FLD(Flags);
	FLD(Rank);
	FLD(FixedStaff);
	FLD(ChargeGrpID);
	#undef FLD
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}

int FASTCALL PPObjStaffList::Fetch(PPID id, PPStaffEntry * pRec)
{
	StaffListCache * p_cache = GetDbLocalCachePtr <StaffListCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int FASTCALL PPObjStaffList::Dirty(PPID id)
{
	StaffListCache * p_cache = GetDbLocalCachePtr <StaffListCache> (Obj, 0);
	return p_cache ? p_cache->Dirty(id) : -1;
}
//
//
//
class PersonPostCache : public ObjCache {
public:
	struct Data : public ObjCacheEntry {
		PPID   StaffID;       // ->Person.ID Работодатель
		PPID   PersonID;      // ->Location.ID (LOCTYP_DIVISION)
		long   ChargeGrpID;   // ->Ref(PPOBJ_SALCHARGE) Группа начислений, используемая для этой должности
		long   Flags;
		int16  Closed;
		int16  Reserve;       // @alignment
	};

	PersonPostCache() : ObjCache(PPOBJ_PERSONPOST, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int PersonPostCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjStaffList sl_obj;
	PersonPostTbl::Rec rec;
	if(sl_obj.SearchPost(id, &rec) > 0) {
		#define FLD(f) p_cache_rec->f = rec.f
		FLD(StaffID);
		FLD(PersonID);
		FLD(ChargeGrpID);
		FLD(Flags);
		FLD(Closed);
		#undef FLD
		ok = 1;
	}
	else
		ok = -1;
	return ok;
}

void PersonPostCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PersonPostTbl::Rec * p_data_rec = static_cast<PersonPostTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(StaffID);
	FLD(PersonID);
	FLD(ChargeGrpID);
	FLD(Flags);
	FLD(Closed);
	#undef FLD
}

int PPObjStaffList::FetchPost(PPID id, PersonPostTbl::Rec * pRec)
{
	PersonPostCache * p_cache = GetDbLocalCachePtr <PersonPostCache> (PPOBJ_PERSONPOST);
	return p_cache ? p_cache->Get(id, pRec) : SearchPost(id, pRec);
}

int PPObjStaffList::DirtyPost(PPID id)
{
	PersonPostCache * p_cache = GetDbLocalCachePtr <PersonPostCache> (PPOBJ_PERSONPOST, 0);
	return p_cache ? p_cache->Dirty(id) : -1;
}
//
// Implementation of PPALDD_StaffNom
//
PPALDD_CONSTRUCTOR(StaffNom)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjStaffList;
}

PPALDD_DESTRUCTOR(StaffNom)
{
	Destroy();
	delete static_cast<PPObjStaffList *>(Extra[0].Ptr);
}

int PPALDD_StaffNom::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(Extra[0].Ptr);
		PPStaffEntry rec;
		if(p_obj->Search(rFilt.ID, &rec) > 0) {
			H.ID           = rec.ID;
			H.OrgID        = rec.OrgID;
			H.DivID        = rec.DivisionID;
			H.ChargeGrpID  = rec.ChargeGrpID;
			H.Rank         = rec.Rank;
			H.Flags        = rec.Flags;
			H.VacancyCount = rec.VacancyCount;
			H.VacancyBusy  = rec.VacancyBusy;
			H.FixedStaff   = rec.FixedStaff;
			STRNSCPY(H.Name, rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonPost
//
PPALDD_CONSTRUCTOR(PersonPost)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjStaffList;
}

PPALDD_DESTRUCTOR(PersonPost)
{
	Destroy();
	delete static_cast<PPObjStaffList *>(Extra[0].Ptr);
}

int PPALDD_PersonPost::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(Extra[0].Ptr);
		PersonPostTbl::Rec rec;
		if(p_obj->SearchPost(rFilt.ID, &rec) > 0) {
			H.ID           = rec.ID;
			H.StaffID      = rec.StaffID;
			H.PersonID     = rec.PersonID;
			H.Dt           = rec.Dt;
			H.Finish       = rec.Finish;
			H.ChargeGrpID  = rec.ChargeGrpID;
			H.Flags        = rec.Flags;
			H.Closed       = rec.Closed;
			H.PsnEventID   = rec.PsnEventID;
			STRNSCPY(H.Code, rec.Code);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// coclass(PPObjStaff)
//
#define USE_IMPL_DL6ICLS_PPObjStaff
#include "..\rsrc\dl600\ppifc_auto.cpp"

DL6_IC_CONSTRUCTION_EXTRA(PPObjStaff, DL6ICLS_PPObjStaff_VTab, PPObjStaffList)
//
// Interface IPapyrusObject implementation
//
static void FillStaffRec(const PPStaffEntry * pInner, SPpyO_Staff * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = ppoStaff;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(OrgID);
	FLD(DivisionID);
	FLD(Rank);
	FLD(Flags);
	FLD(FixedStaff);
	FLD(ChargeGrpID);
	FLD(VacancyCount);
	FLD(VacancyBusy);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

int32 DL6ICLS_PPObjStaff::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(ExtraPtr);
	if(p_obj) {
		PPStaffEntry inner_rec;
		ok = p_obj->Search(id, &inner_rec);
		FillStaffRec(&inner_rec, (SPpyO_Staff *)rec);
	}
	if(!ok)
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPObjStaff::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjStaff::GetName(int32 id)
{
	PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(ExtraPtr);
	if(p_obj) {
		PPStaffEntry rec;
		if(p_obj->Fetch(id, &rec) > 0)
			RetStrBuf = rec.Name;
		else
			ideqvalstr(id, RetStrBuf);
	}
	else {
		RetStrBuf.Z();
		AppError = 1;
	}
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjStaff::GetSelector(int32 extraParam)
{
	PPObjStaffList::Filt filt;
	filt.OrgID = extraParam;
	filt.DivID = 0;
	IStrAssocList * p = reinterpret_cast<IStrAssocList *>(GetPPObjIStrAssocList(this, static_cast<PPObject *>(ExtraPtr), &filt));
	if(!p)
		AppError = 1;
	return p;
}

int32 DL6ICLS_PPObjStaff::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 1;
	PPID   id = 0;
	PPStaffPacket pack;
	SString temp_buf;
	const SPpyO_Staff * p_outer_rec = static_cast<const SPpyO_Staff *>(pRec);
	PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(ExtraPtr);
	THROW(p_obj);
	THROW_PP_S(p_outer_rec->RecTag == ppoStaff, PPERR_INVSTRUCTAG, "ppoStaff");
	pack.Rec.OrgID        = p_outer_rec->OrgID;
	pack.Rec.DivisionID   = p_outer_rec->DivisionID;
	pack.Rec.Rank         = p_outer_rec->Rank;
	pack.Rec.Flags        = p_outer_rec->Flags;
	pack.Rec.FixedStaff   = p_outer_rec->FixedStaff;
	pack.Rec.ChargeGrpID  = p_outer_rec->ChargeGrpID;
	pack.Rec.VacancyCount = (int16)p_outer_rec->VacancyCount;
	temp_buf.CopyFromOleStr(p_outer_rec->Name).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	ASSIGN_PTR(pID, id);
	CATCH
		ok = RaiseAppError();
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjStaff::Update(int32 id, long flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPStaffPacket pack;
	SString temp_buf;
	SPpyO_Staff * p_outer_rec = (SPpyO_Staff *)rec;
	PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(ExtraPtr);
	THROW(p_obj);
	THROW_PP_S(p_outer_rec->RecTag == ppoStaff, PPERR_INVSTRUCTAG, "ppoStaff");
	THROW(p_obj->GetPacket(id, &pack) > 0);
	// noupd pack.Rec.OrgID        = p_outer_rec->OrgID;
	// noupd pack.Rec.DivisionID   = p_outer_rec->DivisionID;
	pack.Rec.Rank         = p_outer_rec->Rank;
	pack.Rec.Flags        = p_outer_rec->Flags;
	// noupd pack.Rec.FixedStaff   = p_outer_rec->FixedStaff;
	pack.Rec.ChargeGrpID  = p_outer_rec->ChargeGrpID;
	pack.Rec.VacancyCount = (int16)p_outer_rec->VacancyCount;
	temp_buf.CopyFromOleStr(p_outer_rec->Name).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
	temp_buf.CopyFromOleStr(p_outer_rec->Name).CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	CATCH
		ok = RaiseAppError();
	ENDCATCH
	return ok;
}

