// CHKINPSN.CPP
// Copyright (c) A.Sobolev 2013, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

SLAPI PPCheckInPersonConfig::PPCheckInPersonConfig()
{
	THISZERO();
}

SLAPI PPCheckInPersonConfig::PPCheckInPersonConfig(PPObjTSession & rTSesObj, const TSessionPacket & rTSessPack)
{
	THISZERO();
	{
		ProcessorTbl::Rec prc_rec;
		TechTbl::Rec tec_rec;
		THROW(rTSesObj.GetPrc(rTSessPack.Rec.PrcID, &prc_rec, 1, 1) > 0);
		THROW_PP_S(prc_rec.Flags & PRCF_ALLOWCIP, PPERR_PRCNOTALLOWEDCIP, prc_rec.Name);
		Flags |= fInitPrc;
		PersonKindID = prc_rec.CipPersonKindID;
		Capacity = prc_rec.CipMax; // Имеет меньший приоритет, чем TechTbl::Rec::CipMax
		if(rTSessPack.Rec.TechID) {
			THROW(rTSesObj.GetTech(rTSessPack.Rec.TechID, &tec_rec) > 0);
			Flags |= fInitTech;
			if(tec_rec.CipMax > 0)
				Capacity = tec_rec.CipMax;
			GoodsID = tec_rec.GoodsID;
		}
		LocID = prc_rec.LocID;
	}
	CATCH
		Flags |= fError;
	ENDCATCH
}

int SLAPI PPCheckInPersonConfig::operator !() const
{
	return BIN(Flags & fError);
}

IMPL_INVARIANT_C(PPCheckInPersonItem)
{
	S_INVARIANT_PROLOG(pInvP);

	S_ASSERT_P((Flags & fAnonym) || PersonID, pInvP);
	S_ASSERT_P((Flags & fAnonym) || (PersonID & 0xFFC00000) == 0, pInvP);
	S_ASSERT_P(!(Flags & fAnonym) || (PersonID & ~0xFFC00000) == 0, pInvP);
	S_ASSERT_P(!ID || oneof3(GetStatus(), statusRegistered, statusCheckedIn, statusCanceled), pInvP);
	S_ASSERT_P(Kind == kTSession, pInvP);
	S_ASSERT_P(PrmrID != 0, pInvP);
	S_ASSERT_P(checkdate(RegDtm.d, 1), pInvP);
	S_ASSERT_P(checktime(RegDtm.t), pInvP);
	S_ASSERT_P(checkdate(CiDtm.d, 1), pInvP);
	S_ASSERT_P(checktime(CiDtm.t), pInvP);
	S_ASSERT_P(Amount >= 0.0 && Amount <= 1000000.0, pInvP);

	S_INVARIANT_EPILOG(pInvP);
}

SLAPI PPCheckInPersonItem::PPCheckInPersonItem()
{
	THISZERO();
}

PPCheckInPersonItem & SLAPI PPCheckInPersonItem::Clear()
{
	THISZERO();
	return *this;
}

int SLAPI PPCheckInPersonItem::CalcPinCode(SString & rCode) const
{
    rCode.Z();
    SString temp_buf;
    temp_buf.Cat(PrmrID).CatChar('#').Cat(PersonID).CatChar('#').Cat(PlaceCode);
	uint32 h = BobJencHash(temp_buf.cptr(), temp_buf.Len());
	rCode.CatLongZ((long)(h % 10000), 4);
	return 1;
}

int SLAPI PPCheckInPersonItem::Cancel(long flags, const char * pPinCode)
{
	int    ok = -1;
    if(flags & opfVerifyPinCode) {
        SString pin_code;
        CalcPinCode(pin_code);
        THROW_PP(pin_code.CmpNC(pPinCode) == 0, PPERR_TSESSCIPOP_INVPINCODE);
    }
    if(GetStatus() == statusCanceled) {
		PPSetError(PPERR_TSESSCIPOP_ALLRCANCELED);
		// Возврат -1
    }
    else {
		THROW(SetStatus(statusCanceled));
		ok = 1;
    }
    CATCHZOK
	return ok;
}

int SLAPI PPCheckInPersonItem::CheckIn(long flags, const char * pPinCode)
{
	int    ok = -1;
	const  int current_status = GetStatus();
    if(current_status == statusCheckedIn) {
		PPSetError(PPERR_TSESSCIPOP_ALLRCHECKEDIN);
		// Возврат -1
    }
    else {
		THROW_PP(current_status != statusCanceled, PPERR_TSESSCIPOP_ALLRCANCELED);
		//THROW_PP(current_status == statusRegistered, PPERR_TSESSCIPOP_UNREGISTERED);
		if(current_status == statusRegistered) {
			if(flags & opfVerifyPinCode) {
				SString pin_code;
				CalcPinCode(pin_code);
				THROW_PP(pin_code.CmpNC(pPinCode) == 0, PPERR_TSESSCIPOP_INVPINCODE);
			}
		}
		else {
            SetAnonym();
		}
		THROW(SetStatus(statusCheckedIn));
		CiDtm = getcurdatetime_();
		ok = 1;
    }
    CATCHZOK
	return ok;
}

int FASTCALL PPCheckInPersonItem::SetPerson(PPID personID)
{
	PersonID = personID;
	SETFLAG(Flags, fAnonym, PersonID == 0);
	return 1;
}

PPID SLAPI PPCheckInPersonItem::GetPerson() const
{
	return (Flags & fAnonym) ? 0 : PersonID;
}

int SLAPI PPCheckInPersonItem::GetPersonName(SString & rBuf) const
{
	int    ok = 1;
	if(Flags & fAnonym) {
		rBuf = "Anonym";
		ok = 2;
	}
	else
		ok = ::GetPersonName(PersonID, rBuf.Z());
	return ok;
}

int SLAPI PPCheckInPersonItem::SetAnonym()
{
	PersonID = 0;
	Flags |= fAnonym;
	return 1;
}

int SLAPI PPCheckInPersonItem::IsAnonym() const
{
	return BIN(Flags & fAnonym);
}

//int SLAPI PPCheckInPersonItem::Count(uint * pRegCount, uint * pCiCount, uint * pCanceledCount) const
void SLAPI PPCheckInPersonItem::Count(Total & rT) const
{
	//uint   reg_count = 0;
	//uint   ci_count = 0;
	//uint   canceled_count = 0;
	const uint rc = (RegCount > 0) ? RegCount : 1;
	const int  status = GetStatus();
	if(status == statusCheckedIn) {
		if(CiCount > 0)
			rT.CiCount += CiCount;
		else
			rT.CiCount++;
	}
	else if(status == statusCanceled)
		rT.CalceledCount += rc;
	rT.RegCount += rc;
	//ASSIGN_PTR(pRegCount, reg_count);
	//ASSIGN_PTR(pCiCount, ci_count);
	//ASSIGN_PTR(pCanceledCount, canceled_count);
	//return 1;
}

int SLAPI PPCheckInPersonItem::CalcAmount(const PPCheckInPersonConfig * pCfg, double * pPrice, double * pAmount) const
{
	int    ok = -1;
	double price = 0.0;
	double amount = 0.0;
	double qtty = (CiCount > 0) ? (double)CiCount : 1.0;
	if(pCfg) {
		if(pCfg->GoodsID) {
			PPID   ar_id = 0;
			RetailExtrItem rei;
			RetailPriceExtractor rpe(pCfg->LocID, 0, ar_id, ZERODATETIME, 0);
			rpe.GetPrice(pCfg->GoodsID, 0, qtty, &rei);
			price = rei.Price;
			if(price > 0.0)
				ok = 2;
		}
		amount = price * qtty;
	}
	ASSIGN_PTR(pPrice, price);
	ASSIGN_PTR(pAmount, amount);
	return ok;
}

int SLAPI PPCheckInPersonItem::GetStatus() const
{
	int    status = statusUnkn;
	if(Flags & fCheckedIn)
		status = statusCheckedIn;
	else if(Flags & fCanceled)
		status = statusCanceled;
	else if(ID)
		status = statusRegistered;
	return status;
}

int SLAPI PPCheckInPersonItem::SetStatus(int status)
{
	int    ok = 1;
	if(status == statusRegistered) {
		Flags &= ~(fCheckedIn | fCanceled);
	}
	else if(status == statusCheckedIn) {
		Flags &= ~fCanceled;
		Flags |= fCheckedIn;
	}
	else if(status == statusCanceled) {
		Flags |= fCanceled;
		Flags &= ~fCheckedIn;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPCheckInPersonItem::IsEqual(const PPCheckInPersonItem & rS, long options) const
{
	int    ok = 1;
	#define FLDEQ(f) (f==rS.f)
	THROW(FLDEQ(Kind));
	THROW(FLDEQ(PrmrID));
	THROW((Flags & fAnonym) == (rS.Flags & fAnonym));
	THROW(FLDEQ(PersonID));
	if(!(options & eqoKeyOnly)) {
		THROW((options & eqoNoID) || FLDEQ(ID));
		THROW((options & eqoNoNum) || FLDEQ(Num));
		THROW(FLDEQ(RegCount));
		THROW(FLDEQ(CiCount));
		THROW(FLDEQ(Flags));
		THROW(FLDEQ(RegDtm));
		THROW(FLDEQ(CiDtm));
		THROW(FLDEQ(Amount));
		THROW(FLDEQ(CCheckID));
		THROW(FLDEQ(SCardID));
		THROW(FLDEQ(MemoPos));
		THROW(stricmp(PlaceCode, rS.PlaceCode) == 0);
	}
	#undef FLDEQ
	CATCHZOK
	return ok;
}

int FASTCALL PPCheckInPersonItem::operator == (const PPCheckInPersonItem & rS) const
{
	return IsEqual(rS, 0);
}
//
//
//
IMPL_INVARIANT_C(PPCheckInPersonArray)
{
	S_INVARIANT_PROLOG(pInvP);

	S_ASSERT_P(Ver.IsGt(7, 8, 0), pInvP);
	{
		SInvariantParam invp;
		invp.LocalOk = 1;
		for(uint i = 0; invp.LocalOk && i < getCount(); i++) {
			const PPCheckInPersonItem & r_item = at(i);
			S_ASSERT_P(r_item.InvariantC(&invp), pInvP);
			S_ASSERT_P(r_item.MemoPos < MemoPool.getDataLen(), pInvP);
		}
	}

	S_INVARIANT_EPILOG(pInvP);
}


SLAPI PPCheckInPersonArray::PPCheckInPersonArray() : TSVector <PPCheckInPersonItem> (), MemoPool(SLBColumnDelim), // @v9.8.6 TSArray-->TSVector
	Ver(DS.GetVersion()), Kind(0), PrmrID(0), LastAnonymN(0)
{
	MemoPool.add("$"); // zero index - is empty string
}

PPCheckInPersonArray & FASTCALL PPCheckInPersonArray::Copy(const PPCheckInPersonArray & rS)
{
	Ver = rS.Ver;
	Kind = rS.Kind;
	PrmrID = rS.PrmrID;
	LastAnonymN = rS.LastAnonymN;
	SVector::copy(rS); // @v9.8.6 SArray-->SVector
	MemoPool.copy(rS.MemoPool);
	return *this;
}

PPCheckInPersonArray & FASTCALL PPCheckInPersonArray::operator = (const PPCheckInPersonArray & rS)
{
	return Copy(rS);
}

PPCheckInPersonArray & SLAPI PPCheckInPersonArray::Init(int kind, PPID prmrID)
{
	Clear();
	Ver = DS.GetVersion();
	Kind = kind;
	PrmrID = prmrID;
	return *this;
}

void FASTCALL PPCheckInPersonArray::InitItem(PPCheckInPersonItem & rItem) const
{
	rItem.Clear();
	rItem.Kind = Kind;
	rItem.PrmrID = PrmrID;
	rItem.RegDtm = getcurdatetime_();
}

PPCheckInPersonArray & SLAPI PPCheckInPersonArray::Clear()
{
	LastAnonymN = 0;
	SVector::clear(); // @v9.8.6 SArray-->SVector
	MemoPool.clear();
	MemoPool.add("$"); // zero index - is empty string
	return *this;
}

int SLAPI PPCheckInPersonArray::SetMemo(uint rowPos, const char * pMemo)
{
	int    ok = 0;
	if(rowPos < getCount()) {
		PPCheckInPersonItem & r_item = at(rowPos);
		uint   text_pos = 0;
		if(!isempty(pMemo)) {
			MemoPool.add(pMemo, &text_pos);
		}
		r_item.MemoPos = text_pos;
		ok = 1;
	}
	return ok;
}

int SLAPI PPCheckInPersonArray::GetMemo(uint rowPos, SString & rMemo) const
{
	int    ok = 0;
	if(rowPos < getCount()) {
		const PPCheckInPersonItem & r_item = Get(rowPos);
		MemoPool.getnz(r_item.MemoPos, rMemo);
		ok = 1;
	}
	return ok;
}

int SLAPI PPCheckInPersonArray::AddItem(const PPCheckInPersonItem & rItem, const PPCheckInPersonConfig * pCipCfg, uint * pPos)
{
	int    ok = 1;
	PPCheckInPersonItem item;
	item = rItem;
	if(pCipCfg) {
		//uint   reg_count = 0, _reg_count = 0;
		//uint   ci_count = 0, _ci_count = 0;
		//uint   cnc_count = 0, _cnc_count = 0;
		PPCheckInPersonItem::Total rcount;
		PPCheckInPersonItem::Total _rcount;
		//Count(&reg_count, &ci_count, &cnc_count);
		Count(rcount);
		//rItem.Count(&_reg_count, &_ci_count, &_cnc_count);
		rItem.Count(_rcount);
        //THROW_PP_S(!pCipCfg->Capacity || (reg_count + _reg_count) <= (uint)pCipCfg->Capacity, PPERR_CHKINP_CAPACITYEXCESS, (long)pCipCfg->Capacity);
		THROW_PP_S(!pCipCfg->Capacity || (rcount.RegCount + _rcount.RegCount) <= (uint)pCipCfg->Capacity, PPERR_CHKINP_CAPACITYEXCESS, (long)pCipCfg->Capacity);
        if(rItem.PlaceCode[0]) {
			SString place_code = rItem.PlaceCode;
			SString temp_buf;
			ProcessorPlaceCodeTemplate::NormalizeCode(place_code);
            for(uint i = 0; i < getCount(); i++) {
				const PPCheckInPersonItem & r_item = at(i);
				if(!(r_item.Flags & PPCheckInPersonItem::fCanceled)) {
					temp_buf = r_item.PlaceCode;
					ProcessorPlaceCodeTemplate::NormalizeCode(temp_buf);
					THROW_PP_S(temp_buf != place_code, PPERR_CHKINP_PLACEISBUSY, place_code);
				}
            }
        }
	}
	if(item.Flags & item.fAnonym) {
		if(item.PersonID) {
			long   temp_n = (item.PersonID >> 22);
			SETMAX(LastAnonymN, temp_n);
		}
		else {
			LastAnonymN++;
			item.PersonID = (LastAnonymN << 22);
		}
	}
	THROW_SL(SVector::insert(&item)); // @v9.8.6 SArray-->SVector
	ASSIGN_PTR(pPos, SVector::getCount()-1); // @v9.8.6 SArray-->SVector
	CATCHZOK
	return ok;
}

int SLAPI PPCheckInPersonArray::UpdateItem(uint pos, const PPCheckInPersonItem & rItem, const PPCheckInPersonConfig * pCipCfg)
{
	if(pos < getCount()) {
		PPCheckInPersonItem item;
		item = rItem;
		if(item.Flags & item.fAnonym && item.PersonID == 0) {
			LastAnonymN++;
			item.PersonID = (LastAnonymN << 22);
		}
		at(pos) = item;
		return 1;
	}
	else
		return 0;
}

int FASTCALL PPCheckInPersonArray::RemoveItem(uint pos)
{
	int    ok = 1;
	if(pos < SVector::getCount()) { // @v9.8.6 SArray-->SVector
		SVector::atFree(pos); // @v9.8.6 SArray-->SVector
	}
	else
		ok = 0;
	return ok;
}

void SLAPI PPCheckInPersonArray::Normalize(int kind, PPID prmrID)
{
	Kind = kind;
	PrmrID = prmrID;
	SString temp_buf;
	StringSet temp_memo_pool(SLBColumnDelim);
	temp_memo_pool.add("$"); // zero index - is empty string
	for(uint i = 0; i < getCount(); i++) {
		PPCheckInPersonItem & r_item = at(i);
		r_item.Kind = kind;
		r_item.PrmrID = prmrID;
		if(r_item.MemoPos) {
			MemoPool.getnz(r_item.MemoPos, temp_buf.Z());
			temp_memo_pool.add(temp_buf, &r_item.MemoPos);
		}
	}
	MemoPool = temp_memo_pool;
}

int SLAPI PPCheckInPersonArray::ProcessObjRefs(PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	// Ссылка CCheckID не разрешается.
	for(uint i = 0; i < getCount(); i++) {
		PPCheckInPersonItem & r_item = at(i);
		PPID   person_id = r_item.GetPerson();
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_PERSON, &person_id, ary, replace));
		if(replace)
			r_item.SetPerson(person_id);
	}
	CATCHZOK
	return ok;
}

uint SLAPI PPCheckInPersonArray::GetCount() const
{
	return SVector::getCount(); // @v9.8.6 SArray-->SVector
}

const  PPCheckInPersonItem & FASTCALL PPCheckInPersonArray::Get(uint pos) const
{
	return at(pos);
}

void SLAPI PPCheckInPersonArray::InitIteration()
{
	SVector::setPointer(0); // @v9.8.6 SArray-->SVector
}

int FASTCALL PPCheckInPersonArray::NextIteration(PPCheckInPersonItem & rItem)
{
	int    ok = 1;
	if(SVector::getPointer() < SVector::getCount()) { // @v9.8.6 SArray-->SVector
		rItem = at(SVector::incPointer()); // @v9.8.6 SArray-->SVector
	}
	else {
		rItem.Clear();
		ok = 0;
	}
	return ok;
}

//int SLAPI PPCheckInPersonArray::Count(uint * pRegCount, uint * pCiCount, uint * pCanceledCount) const
void SLAPI PPCheckInPersonArray::Count(PPCheckInPersonItem::Total & rT) const
{
	//uint   reg_count = 0;
	//uint   ci_count = 0;
	//uint   canceled_count = 0;
	for(uint i = 0; i < getCount(); i++) {
		const PPCheckInPersonItem & r_item = at(i);
		//uint   reg_count_ = 0;
		//uint   ci_count_ = 0;
		//uint   canceled_count_ = 0;
		//r_item.Count(&reg_count_, &ci_count_, &canceled_count_);
		r_item.Count(rT);
		//reg_count += reg_count_;
		//ci_count += ci_count_;
		//canceled_count += canceled_count_;
	}
	//ASSIGN_PTR(pRegCount, reg_count);
	//ASSIGN_PTR(pCiCount, ci_count);
	//ASSIGN_PTR(pCanceledCount, canceled_count);
	//return 1;
}

int SLAPI PPCheckInPersonArray::CalcAmount(const PPCheckInPersonConfig * pCfg, double * pAmount) const
{
	int    ok = -1;
	double amount = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const PPCheckInPersonItem & r_item = at(i);
		double price = 0.0;
		double amt_ = 0.0;
		if(r_item.CalcAmount(pCfg, &price, &amt_) > 0) {
			amount += amt_;
			ok = 1;
		}
	}
	ASSIGN_PTR(pAmount, amount);
	return ok;
}

int SLAPI PPCheckInPersonArray::SearchByID(PPID id, uint * pPos) const
{
	return lsearch(&id, pPos, CMPF_LONG);
}

int SLAPI PPCheckInPersonArray::SearchByNum(long num, uint * pPos) const
{
	return lsearch(&num, pPos, CMPF_LONG, offsetof(PPCheckInPersonItem, Num));
}

int SLAPI PPCheckInPersonArray::SearchItem(const PPCheckInPersonItem & rItem, uint * pPos) const
{
	int    ok = 0;
	for(uint   i = DEREFPTRORZ(pPos); !ok && i < getCount(); i++) {
		const PPCheckInPersonItem & r_test = at(i);
		if(r_test.IsEqual(rItem, PPCheckInPersonItem::eqoKeyOnly)) {
			ASSIGN_PTR(pPos, i);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPCheckInPersonArray::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, Kind, rBuf));
	THROW_SL(pCtx->Serialize(dir, PrmrID, rBuf));
	THROW_SL(pCtx->Serialize(dir, (SArray *)this, rBuf));
	if(dir < 0) {
		if(Ver.IsGt(7, 8, 9)) {
			THROW_SL(MemoPool.Serialize(dir, rBuf, pCtx));
		}
		else {
			MemoPool.clear();
		}
	}
	else if(dir > 0) {
		THROW_SL(MemoPool.Serialize(dir, rBuf, pCtx));
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPCheckInPersonMngr::PPCheckInPersonMngr()
{
}

int SLAPI PPCheckInPersonMngr::Search(PPID id, PPCheckInPersonItem * pItem)
{
	ObjAssocTbl::Rec rec;
	int    ok = PPRef->Assc.Search(id, &rec);
	if(ok > 0) {
		if(pItem)
			StorageToItem(rec, *pItem);
	}
	return ok;
}

int SLAPI PPCheckInPersonMngr::Search(/*int kind, PPID prmrID, PPID personID*/const PPCheckInPersonItem & rKeyItem, PPCheckInPersonItem * pItem)
{
	int    ok = -1;
	PPID   assc_type = GetAssocType(rKeyItem.Kind);
	if(assc_type) {
		ObjAssocTbl::Rec rec;
		PPID   person_id = rKeyItem.PersonID;
		ok = PPRef->Assc.Search(assc_type, rKeyItem.PrmrID, person_id, &rec);
		if(ok > 0) {
			if(pItem)
				StorageToItem(rec, *pItem);
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPCheckInPersonMngr::GetList(int kind, PPID prmrID, PPCheckInPersonArray & rList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	TSVector <ObjAssocTbl::Rec> items_list; // @v9.8.4 TSArray-->TSVector
	PPID   assc_type = GetAssocType(kind);
	rList.Init(kind, prmrID);
	THROW(assc_type);
	THROW(p_ref->Assc.GetItemsListByPrmr(assc_type, prmrID, &items_list));
	for(uint i = 0; i < items_list.getCount(); i++) {
		const ObjAssocTbl::Rec & r_rec = items_list.at(i);
		PPCheckInPersonItem item;
		THROW(StorageToItem(r_rec, item));
		THROW(rList.AddItem(item, 0, 0));
		ok = 1;
	}
	{
		SString mempool_buf;
		THROW(p_ref->GetPropVlrString(assc_type, prmrID, PPPRP_CHKINPMEMO, mempool_buf));
		if(mempool_buf.NotEmpty()) {
			rList.MemoPool.setBuf(mempool_buf, mempool_buf.Len()+1);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCheckInPersonMngr::Put(PPCheckInPersonItem & rItem, int use_ta)
{
	int    ok = 1, r = 0;
	PPCheckInPersonItem org_item;
	ObjAssocTbl::Rec assc_rec;
	const PPID assc_type = GetAssocType(rItem.Kind);
	THROW(assc_type);
	{
		SInvariantParam invp;
		THROW_PP(rItem.InvariantC(&invp), PPERR_CHKINP_ITEMINVARIANT);
	}
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(use_ta);
		THROW(tra);
		if((r = Search(rItem, &org_item)) > 0) {
			// PPCheckInPersonItem::eqoNoID|PPCheckInPersonItem::eqoNoNum
			SETIFZ(rItem.ID, org_item.ID);
			SETIFZ(rItem.Num, org_item.Num);
			if(rItem.IsEqual(org_item, 0)) {
				ok = -1;
			}
			else {
				THROW_PP_S(rItem.ID == org_item.ID, PPERR_CHKINP_CONFLICTID, rItem.ID);
				if(rItem.Num != org_item.Num) {
					THROW(r = p_ref->Assc.SearchNum(assc_type, rItem.PrmrID, rItem.Num, &assc_rec));
					THROW_PP_S(r < 0, PPERR_CHKINP_CONFLICTNUM, rItem.Num);
				}
				THROW(ItemToStorage(rItem, assc_rec));
				THROW(p_ref->Assc.Update(rItem.ID, &assc_rec, 0));
			}
		}
		else {
			PPID   assc_id = 0;
			THROW(r);
			THROW(p_ref->Assc.SearchFreeNum(assc_type, rItem.PrmrID, &rItem.Num, 0));
			THROW(ItemToStorage(rItem, assc_rec));
			THROW(p_ref->Assc.Add(&assc_id, &assc_rec, use_ta));
			rItem.ID = assc_id;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPCheckInPersonMngr::Put(PPCheckInPersonArray & rList, int use_ta)
{
	int    ok = -1, r = 0;
	PPCheckInPersonItem org_item;
	PPID   assc_type = 0;
	assert(rList.Kind);
	assert(rList.PrmrID);
	{
		SInvariantParam invp;
		THROW_PP(rList.InvariantC(&invp), PPERR_CHKINP_LISTINVARIANT);
	}
	{
		Reference * p_ref = PPRef;
		int    is_any_memo = 0;
		uint   i;
		SString memo_buf, memo_buf2;
		LongArray upd_pos_list, rmv_orgpos_list;
		PPCheckInPersonArray org_list;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(GetList(rList.Kind, rList.PrmrID, org_list));
		for(i = 0; i < rList.GetCount(); i++) {
			const PPCheckInPersonItem & r_item = rList.Get(i);
			PPID local_assc_type = GetAssocType(r_item.Kind);
			THROW(local_assc_type);
			THROW_PP(!assc_type || assc_type == local_assc_type, PPERR_CHKINP_MISMKIND);
			assc_type = local_assc_type;

			rList.GetMemo(i, memo_buf);
			if(memo_buf.NotEmpty())
				is_any_memo = 1;
			uint   pos = 0;
			if(org_list.SearchItem(r_item, &pos)) {
				org_list.GetMemo(pos, memo_buf2);
				if(!r_item.IsEqual(org_list.Get(pos), 0) || memo_buf != memo_buf2) {
					upd_pos_list.add(i);
				}
			}
			else {
				upd_pos_list.add(i);
			}
		}
		for(i = 0; i < org_list.GetCount(); i++) {
			const  PPCheckInPersonItem & r_org_item = org_list.Get(i);
			uint   pos = 0;
			if(!rList.SearchItem(r_org_item, &pos)) {
				rmv_orgpos_list.add(i);
			}
		}
		for(i = 0; i < rmv_orgpos_list.getCount(); i++) {
			const  PPCheckInPersonItem & r_org_item = org_list.Get(rmv_orgpos_list.get(i));
			THROW(p_ref->Assc.Remove(r_org_item.ID, 0));
			ok = 1;
		}
		for(i = 0; i < upd_pos_list.getCount(); i++) {
			const uint upd_pos = upd_pos_list.get(i);
			PPCheckInPersonItem item = rList.Get(upd_pos);
			THROW(r = Put(item, 0));
			THROW(rList.UpdateItem(upd_pos, item, 0));
			if(r > 0) {
				ok = 1;
			}
		}
		{
			PPID   assc_type = GetAssocType(rList.Kind);
			const  char * p_memopool_buf = is_any_memo ? rList.MemoPool.getBuf() : 0;
			THROW(p_ref->PutPropVlrString(assc_type, rList.PrmrID, PPPRP_CHKINPMEMO, p_memopool_buf, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

struct PPCheckInPersonItem_Strg {
	long   ID;
	long   AsscType;
	long   PrmrID;
	long   PersonID;
	long   Num;

	uint16 RegCount;
	uint16 CiCount;
	long   Flags;
	LDATETIME RegDtm;      // Время регистрации //
	LDATETIME CiDtm;       // Время подтверждения регистрации (CheckID)
	double Amount;         // Сумма, уплаченная или которая должна быть уплачена за регистрацию (подтверждение)
	PPID   CCheckID;       // Кассовый чек, которым оплачено подтверждение регистрации
	PPID   SCardID;        // Карта, с которой ассоциирована зерегистрированная персоналия //
	uint   MemoPos;        // @internal
	char   PlaceCode[8];   // Номер места (для регистрации, ассоциированной с посадочным местом)
	uint8  Reserve[12];
};

//static
PPID FASTCALL PPCheckInPersonMngr::GetAssocType(int kind)
{
	PPID   assc_type = 0;
	if(kind == PPCheckInPersonItem::kTSession) {
		assc_type = PPASS_CHECKINPSNTSES;
	}
	else {
		SString added_msg;
		added_msg.Cat(kind);
		PPSetError(PPERR_CHKINP_INVKIND, added_msg);
	}
	return assc_type;
}

//static
int  FASTCALL PPCheckInPersonMngr::GetKind(PPID asscType)
{
	int    kind = 0;
	if(asscType == PPASS_CHECKINPSNTSES)
		kind = PPCheckInPersonItem::kTSession;
	else {
		SString added_msg;
		added_msg.Cat(asscType);
		PPSetError(PPERR_CHKINP_INVASSCTYPE, added_msg);
	}
	return kind;
}

//static
int FASTCALL PPCheckInPersonMngr::ItemToStorage(const PPCheckInPersonItem & rItem, ObjAssocTbl::Rec & rRec)
{
	assert(sizeof(PPCheckInPersonItem_Strg) == sizeof(ObjAssocTbl::Rec));
	int    ok = 1;
	MEMSZERO(rRec);
	PPCheckInPersonItem_Strg & r_rec = *(PPCheckInPersonItem_Strg *)&rRec;
	#define FLD(f) r_rec.f = rItem.f
	FLD(ID);
	FLD(PrmrID);
	FLD(PersonID);
	FLD(Num);

	FLD(RegCount);
	FLD(CiCount);
	FLD(Flags);
	FLD(RegDtm);
	FLD(CiDtm);
	FLD(Amount);
	FLD(CCheckID);
	FLD(SCardID);
	FLD(MemoPos);
	#undef FLD
	STRNSCPY(r_rec.PlaceCode, rItem.PlaceCode);
	r_rec.AsscType = GetAssocType(rItem.Kind);
	if(!r_rec.AsscType)
		ok = 0;
	return ok;
}

//static
int FASTCALL PPCheckInPersonMngr::StorageToItem(const ObjAssocTbl::Rec & rRec, PPCheckInPersonItem & rItem)
{
	assert(sizeof(PPCheckInPersonItem_Strg) == sizeof(ObjAssocTbl::Rec));
	int    ok = 1;
	rItem.Clear();
	const PPCheckInPersonItem_Strg & r_rec = *(const PPCheckInPersonItem_Strg *)&rRec;
	#define FLD(f) rItem.f = r_rec.f
	FLD(ID);
	FLD(PrmrID);
	FLD(PersonID);
	FLD(Num);
	FLD(RegCount);
	FLD(CiCount);
	FLD(Flags);
	FLD(RegDtm);
	FLD(CiDtm);
	FLD(Amount);
	FLD(CCheckID);
	FLD(SCardID);
	FLD(MemoPos);
	#undef FLD
	STRNSCPY(rItem.PlaceCode, r_rec.PlaceCode);
	rItem.Kind = GetKind(r_rec.AsscType);
	if(!rItem.Kind)
		ok = 0;
	return ok;
}

#define GRP_CHKINP_PERSON 1

class CheckInPersonDialog : public TDialog {
public:
	CheckInPersonDialog(const PPCheckInPersonConfig * pCfg, SString & rMemoBuf) : TDialog(DLG_CHKINP), R_MemoBuf(rMemoBuf)
	{
		RVALUEPTR(Cfg, pCfg);
		addGroup(GRP_CHKINP_PERSON, new PersonCtrlGroup(CTLSEL_CHKINP_PERSON, CTL_CHKINP_SCARD, 0, PersonCtrlGroup::fCanInsert/*|PersonCtrlGroup::fLoadDefOnOpen*/));
		PersonCtrlGroup * p_grp = (PersonCtrlGroup *)getGroup(GRP_CHKINP_PERSON);
		CALLPTRMEMB(p_grp, SetAnonymCtrlId(CTL_CHKINP_ANONYM));
		SetupCalDate(CTLCAL_CHKINP_REGDT, CTL_CHKINP_REGDT);
		SetupCalDate(CTLCAL_CHKINP_CIDT, CTL_CHKINP_CIDT);
		SetupTimePicker(this, CTL_CHKINP_REGTM, CTLTM_CHKINP_REGTM);
		SetupTimePicker(this, CTL_CHKINP_CITM, CTLTM_CHKINP_CITM);
		setCtrlReadOnly(CTL_CHKINP_PRICE, 1);
		setCtrlReadOnly(CTL_CHKINP_AMOUNT, 1);
		showButton(cmCCheck, Cfg.GoodsID);
		enableCommand(cmCCheck, Cfg.GoodsID);
	}
	int    setDTS(const PPCheckInPersonItem * pData)
	{
		int    ok = 1;
		SString temp_buf;
		Data = *pData;
		{
			PersonCtrlGroup::Rec rec;
			rec.PersonID = Data.GetPerson();
			rec.PsnKindID = Cfg.PersonKindID;
			rec.SCardID = Data.SCardID;
			SETFLAG(rec.Flags, rec.fAnonym, Data.IsAnonym());
			setGroupData(GRP_CHKINP_PERSON, &rec);
		}
		SetupPerson();
		long   status = Data.GetStatus();
		AddClusterAssocDef(CTL_CHKINP_STATUS, 0, PPCheckInPersonItem::statusRegistered);
		AddClusterAssoc(CTL_CHKINP_STATUS, 1, PPCheckInPersonItem::statusCheckedIn);
		AddClusterAssoc(CTL_CHKINP_STATUS, 2, PPCheckInPersonItem::statusCanceled);
		SetClusterData(CTL_CHKINP_STATUS, status);

		setCtrlData(CTL_CHKINP_PLACE, Data.PlaceCode);

		setCtrlData(CTL_CHKINP_REGDT, &Data.RegDtm.d);
		setCtrlData(CTL_CHKINP_REGTM, &Data.RegDtm.t);
		setCtrlData(CTL_CHKINP_CIDT,  &Data.CiDtm.d);
		setCtrlData(CTL_CHKINP_CITM,  &Data.CiDtm.t);

		if(Data.RegCount <= 0)
			Data.RegCount = 1;
		setCtrlData(CTL_CHKINP_REGCOUNT, &Data.RegCount);
		SetupSpin(CTLSPN_CHKINP_REGCOUNT, CTL_CHKINP_REGCOUNT, 1, (Cfg.Capacity > 0) ? Cfg.Capacity : 10000, Data.RegCount);
		if(Data.CiCount < 0 || (Data.CiCount == 0 && status == PPCheckInPersonItem::statusCheckedIn))
			Data.CiCount = 1;
		setCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
		SetupPrice();
		SetupSpin(CTLSPN_CHKINP_CICOUNT, CTL_CHKINP_CICOUNT, 1, (Cfg.Capacity > 0) ? Cfg.Capacity : 10000, Data.CiCount);
		disableCtrls(status != PPCheckInPersonItem::statusCheckedIn, CTL_CHKINP_CIDT, CTL_CHKINP_CITM, 0);
		temp_buf.Z();
		if(Data.CCheckID) {
			CCheckTbl::Rec cc_rec;
			if(ScObj.P_CcTbl->Search(Data.CCheckID, &cc_rec) > 0)
				CCheckCore::MakeCodeString(&cc_rec, temp_buf);
		}
		setStaticText(CTL_CHKINP_ST_CCINFO, temp_buf);
		setCtrlString(CTL_CHKINP_MEMO, R_MemoBuf);
		return ok;
	}
	int    getDTS(PPCheckInPersonItem * pData)
	{
		int    ok = 1;
		long   status = 1;
		{
			PersonCtrlGroup::Rec rec;
			getGroupData(GRP_CHKINP_PERSON, &rec);
			if(rec.Flags & rec.fAnonym)
				Data.SetAnonym();
			else
				Data.SetPerson(rec.PersonID);
			Data.SCardID = rec.SCardID;
		}
		GetClusterData(CTL_CHKINP_STATUS, &status);
		Data.SetStatus(status);
		getCtrlData(CTL_CHKINP_PLACE, Data.PlaceCode);
		getCtrlData(CTL_CHKINP_REGDT, &Data.RegDtm.d);
		getCtrlData(CTL_CHKINP_REGTM, &Data.RegDtm.t);
		getCtrlData(CTL_CHKINP_CIDT,  &Data.CiDtm.d);
		getCtrlData(CTL_CHKINP_CITM,  &Data.CiDtm.t);
		getCtrlData(CTL_CHKINP_REGCOUNT, &Data.RegCount);
		getCtrlData(CTL_CHKINP_CICOUNT,  &Data.CiCount);
		getCtrlString(CTL_CHKINP_MEMO, R_MemoBuf);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_CHKINP_STATUS)) {
			getCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
			int16  prev_ci_count = Data.CiCount;
			long   prev_status = Data.GetStatus();
			long   status = 0;
			GetClusterData(CTL_CHKINP_STATUS, &status);
			if(status != prev_status) {
				Data.SetStatus(status);
				getCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
				setCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
				disableCtrls(status != PPCheckInPersonItem::statusCheckedIn, CTL_CHKINP_CIDT, CTL_CHKINP_CITM, 0);
				getCtrlData(CTL_CHKINP_CIDT,  &Data.CiDtm.d);
				getCtrlData(CTL_CHKINP_CITM,  &Data.CiDtm.t);
				if(status == PPCheckInPersonItem::statusCheckedIn) {
					if(!checkdate(Data.CiDtm.d)) {
						Data.CiDtm = getcurdatetime_();
						setCtrlData(CTL_CHKINP_CIDT,  &Data.CiDtm.d);
						setCtrlData(CTL_CHKINP_CITM,  &Data.CiDtm.t);
					}
					if(Data.CiCount <= 0)
						Data.CiCount = 1;
				}
				else {
					Data.CiCount = 0;
					Data.CiDtm.Z();
					setCtrlData(CTL_CHKINP_CIDT,  &Data.CiDtm.d);
					setCtrlData(CTL_CHKINP_CITM,  &Data.CiDtm.t);
				}
				if(prev_ci_count != Data.CiCount)
					setCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
			}
		}
		else if(event.isCbSelected(CTLSEL_CHKINP_PERSON)) {
			Data.SetPerson(getCtrlLong(CTLSEL_CHKINP_PERSON));
			SetupPerson();
		}
		else if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_CHKINP_CICOUNT)) {
				SetupPrice();
			}
			else if(event.isCtlEvent(CTL_CHKINP_PLACE)) {
				getCtrlData(CTL_CHKINP_PLACE, Data.PlaceCode);
				SetupPinCode();
			}
			else
				return;
		}
		else
			return;
		clearEvent(event);
	}
	void   SetupPerson()
	{
		SString temp_buf;
		if(Data.GetPerson()) {
			PPObjPerson psn_obj;
			PersonTbl::Rec psn_rec;
			if(psn_obj.Search(Data.GetPerson(), &psn_rec) > 0) {
				temp_buf = psn_rec.Memo;
			}
		}
		setStaticText(CTL_CHKINP_ST_PSNMEMO, temp_buf);
		SetupPinCode();
	}
	int    SetupPrice()
	{
		int    ok = -1;
		double price = 0.0;
		getCtrlData(CTL_CHKINP_CICOUNT, &Data.CiCount);
		if(Cfg.GoodsID) {
			PPID   ar_id = 0;
			RetailExtrItem rei;
			RetailPriceExtractor rpe(Cfg.LocID, 0, ar_id, ZERODATETIME, 0);
			rpe.GetPrice(Cfg.GoodsID, 0, (double)Data.CiCount, &rei);
			price = rei.Price;
			if(price > 0.0)
				ok = 2;
		}
		setCtrlReal(CTL_CHKINP_PRICE, price);
		setCtrlReal(CTL_CHKINP_AMOUNT, price * NZOR(labs(Data.CiCount), 1));
		return ok;
	}
	void   SetupPinCode()
	{
		SString pin_code;
		Data.CalcPinCode(pin_code);
		setStaticText(CTL_CHKINP_ST_PINCODE, pin_code);
	}
	PPCheckInPersonConfig Cfg;
	PPCheckInPersonItem Data;
	PPObjSCard ScObj;
	SString & R_MemoBuf;
};

int SLAPI EditCheckInPersonItem(const PPCheckInPersonConfig * pCfg, PPCheckInPersonItem * pData, SString & rMemo) { DIALOG_PROC_BODY_P2(CheckInPersonDialog, pCfg, rMemo, pData); }

class CheckInPersonListDialog : public PPListDialog {
public:
	CheckInPersonListDialog(const PPCheckInPersonConfig * pCfg) : PPListDialog(DLG_CHKINPLIST, CTL_CHKINPLIST_LIST)
	{
		RVALUEPTR(Cfg, pCfg);
		SetupSpin(CTLSPN_CHKINPLIST_CNT, CTL_CHKINPLIST_CNT, 1, (Cfg.Capacity > 0) ? Cfg.Capacity : 10000, 1);
		showCtrl(CTL_CHKINPLIST_CNT, 0);
		showCtrl(CTLSPN_CHKINPLIST_CNT, 0);
	}
	int    setDTS(const PPCheckInPersonArray * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init();
		updateList(-1);
		return 1;
	}
	int    getDTS(PPCheckInPersonArray * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmLBItemFocused)) {
			enableCommand(cmCCheck, CCheck(1) > 0);
			long   pos = 0, id = 0;
			if(getCurItem(&pos, &id) > 0 && pos >= 0 && pos < (long)Data.GetCount()) {
				showCtrl(CTL_CHKINPLIST_CNT, 1);
				showCtrl(CTLSPN_CHKINPLIST_CNT, 1);
				const  PPCheckInPersonItem & r_item = Data.Get(pos);
				int    count = 0;
				if(r_item.Flags & PPCheckInPersonItem::fCheckedIn) {
					count = (int)r_item.CiCount;
				}
				else {
					count = (int)r_item.RegCount;
				}
				setCtrlData(CTL_CHKINPLIST_CNT, &count);
			}
			else {
				showCtrl(CTL_CHKINPLIST_CNT, 0);
				showCtrl(CTLSPN_CHKINPLIST_CNT, 0);
			}
		}
		else if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_CHKINPLIST_CNT)) {
				long   pos = 0, id = 0;
				if(getCurItem(&pos, &id) > 0 && pos >= 0 && pos < (long)Data.GetCount()) {
					PPCheckInPersonItem item_ = Data.Get(pos);
					const  int limit = (Cfg.Capacity > 0) ? Cfg.Capacity : 10000;
					int    count = 0;
					int    do_upd = 0;
					getCtrlData(CTL_CHKINPLIST_CNT, &count);
					if(count > 0 && count <= limit) {
						if(item_.Flags & PPCheckInPersonItem::fCheckedIn) {
							if(count != (int)item_.CiCount) {
								item_.CiCount = (uint16)count;
								do_upd = 1;
							}
						}
						else {
							if(count != (int)item_.RegCount) {
								item_.RegCount = (uint16)count;
								do_upd = 1;
							}
						}
					}
					if(do_upd) {
						Data.UpdateItem(pos, item_, &Cfg);
						updateList(-1);
					}
				}
			}
		}
		else if(event.isCmd(cmCCheck)) {
			CCheck(0);
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    CCheck(int testForEnable)
	{
		int    ok = -1;
		PPCashMachine * p_cm = 0;
		long   pos = 0, id = 0;
		if(getCurItem(&pos, &id) > 0 && pos >= 0 && pos < (long)Data.GetCount()) {
			//
			// @attention Будем работать с копией элемента Data[pos]. Необходимо синхронизировать
			// собственно Data[pos] и item_ при изменении того или другого.
			//
			PPCheckInPersonItem item_ = Data.Get(pos);
			if(item_.CCheckID) {
				if(testForEnable)
					ok = 1;
				else {
					PPID   cn_id = 0;
					CCheckPane(cn_id, item_.CCheckID);
				}
			}
			else {
				if((item_.Flags & PPCheckInPersonItem::fCheckedIn) && Cfg.GoodsID && Cfg.LocID && Cfg.TurnProc) {
					if(testForEnable) {
						ok = 1;
					}
					else {
						PPID   cn_id = PPObjCashNode::Select(Cfg.LocID, 1, 0, 0);
						PPID   chkinp_id = 0;
						double qtty = 0.0;
						PPObjCashNode cn_obj;
						PPCashNode cn_rec;
						if(cn_id > 0 && cn_obj.Search(cn_id, &cn_rec) > 0) {
							PPCheckInPersonMngr cip_mgr;
							SString init_str;
							THROW(p_cm = PPCashMachine::CreateInstance(cn_id));
							THROW(Cfg.TurnProc(&Cfg, &Data, pos, Cfg.P_TurnProcExt));
							//
							// Так как мы работаем с копией элемента Data.Get(pos), то необходимо обновить значение
							// идентификатора, на случай, если он изменился после вызова Cfg.TurnProc()
							//
							item_.ID = Data.Get(pos).ID;
							//
							(init_str = "CHKINP").Cat(item_.ID).CatChar(':').Cat(Cfg.GoodsID).CatChar(':').Cat(NZOR(item_.CiCount, 1));
							THROW(p_cm->SyncBrowseCheckList(init_str, cchkpanfOnce));
							{
								PPCheckInPersonItem cip_item;
								if(cip_mgr.Search(item_.ID, &cip_item) > 0) {
									item_.CCheckID = cip_item.CCheckID;
									Data.UpdateItem(pos, item_, &Cfg);
								}
							}
							updateList(pos);
						}
					}
				}
			}
		}
		CATCHZOKPPERR
		delete p_cm;
		return ok;
	}

	PPCheckInPersonConfig Cfg;
	PPCheckInPersonArray Data;
};

int CheckInPersonListDialog::setupList()
{
	int    ok = 1;
	SString temp_buf, word_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.GetCount(); i++) {
		const PPCheckInPersonItem & r_item = Data.Get(i);
		double price = 0.0;
		double amt = 0.0;
		ss.clear();
		r_item.GetPersonName(temp_buf);
		ss.add(temp_buf);
		if(r_item.Flags & PPCheckInPersonItem::fCheckedIn)
			PPLoadString("checkedin", temp_buf);
		else if(r_item.Flags & PPCheckInPersonItem::fCanceled)
			PPLoadString("canceled", temp_buf);
		else
			PPLoadString("registered", temp_buf);
		ss.add(temp_buf);
		if(r_item.Flags & PPCheckInPersonItem::fCheckedIn) {
			ss.add(temp_buf.Z().Cat(r_item.CiCount));
			r_item.CalcAmount(&Cfg, &price, &amt);
			ss.add(temp_buf.Z().Cat(amt, MKSFMTD(0, 2, NMBF_NOZERO)));
			ss.add(temp_buf.Z().Cat(r_item.CiDtm, DATF_DMY, TIMF_HM));
		}
		else {
			ss.add(temp_buf.Z().Cat(r_item.RegCount));
			ss.add(temp_buf.Z());
			ss.add(temp_buf.Z().Cat(r_item.RegDtm, DATF_DMY, TIMF_HM));
		}
		if(!addStringToList(i, ss.getBuf()))
			ok = 0;
	}
	{
		//uint   reg_count = 0;
		//uint   ci_count = 0;
		//uint   canceled_count = 0;
		PPCheckInPersonItem::Total rcount;
		//Data.Count(&reg_count, &ci_count, &canceled_count);
		Data.Count(rcount);
		temp_buf.Z();
		if(rcount.RegCount) {
			temp_buf.CatEq(PPLoadStringS("registered", word_buf), (ulong)rcount.RegCount);
			if(rcount.CiCount)
				temp_buf.CatDiv(';', 2).CatEq(PPLoadStringS("checkedin", word_buf), (ulong)rcount.CiCount);
			if(rcount.CalceledCount)
				temp_buf.CatDiv(';', 2).CatEq(PPLoadStringS("canceled", word_buf), (ulong)rcount.CalceledCount);
		}
		setStaticText(CTL_CHKINPLIST_ST_TOTAL, temp_buf);
	}
	return ok;
}

int CheckInPersonListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	//uint   reg_count = 0;
	SString added_msg;
	PPCheckInPersonItem::Total rcount;
	Data.Count(rcount);
	if(Cfg.Capacity > 0 && rcount.RegCount >= (uint)Cfg.Capacity) {
		PPError(PPERR_CHKINP_CAPACITYEXCESS, added_msg.Cat(Cfg.Capacity));
		ok = 0;
	}
	else {
		SString memo_buf;
		PPCheckInPersonItem item;
		Data.InitItem(item);
		while(ok < 0 && EditCheckInPersonItem(&Cfg, &item, memo_buf) > 0) {
			//uint   reg_count_ = 0;
			PPCheckInPersonItem::Total rcount_;
			item.Count(rcount_);
			uint   dup_pos = 0;
			if(!item.GetPerson() && !item.IsAnonym()) {
				PPError(PPERR_PERSONNEEDED);
			}
			else if(Data.SearchItem(item, &dup_pos)) {
				item.GetPersonName(added_msg);
				PPError(PPERR_CHKINP_DUPITEM, added_msg);
			}
			else if(Cfg.Capacity > 0 && (rcount.RegCount + rcount_.RegCount) > (uint)Cfg.Capacity) {
				PPError(PPERR_CHKINP_CAPACITYEXCESS, added_msg.Cat(Cfg.Capacity));
			}
			else {
				uint   pos = 0;
				if(Data.AddItem(item, &Cfg, &pos)) {
					Data.SetMemo(pos, memo_buf);
					ASSIGN_PTR(pPos, (long)pos);
					ok = 1;
				}
				else
					PPError();
			}
		}
	}
	return ok;
}

int CheckInPersonListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	SString added_msg;
	if(pos >= 0 && pos < (long)Data.GetCount()) {
		SString memo_buf;
		PPCheckInPersonItem item = Data.Get(pos);
		Data.GetMemo(pos, memo_buf);
		//uint   reg_count = 0;
		PPCheckInPersonItem::Total rcount;
		PPCheckInPersonItem::Total prev_rcount_;
		Data.Count(rcount);
		//uint   prev_reg_count_ = 0;
		item.Count(prev_rcount_);
		while(ok < 0 && EditCheckInPersonItem(&Cfg, &item, memo_buf) > 0) {
			//uint   reg_count_ = 0;
			PPCheckInPersonItem::Total rcount_;
			item.Count(rcount_);
			int    is_dup = 0;
			if(!item.GetPerson() && !item.IsAnonym()) {
				PPError(PPERR_PERSONNEEDED);
			}
			else {
				for(uint dup_pos = 0; Data.SearchItem(item, &dup_pos); dup_pos++) {
					if(dup_pos != pos) {
						item.GetPersonName(added_msg);
						PPError(PPERR_CHKINP_DUPITEM, added_msg);
						is_dup = 1;
					}
				}
				if(!is_dup) {
					if(Cfg.Capacity > 0 && (rcount.RegCount + rcount_.RegCount - prev_rcount_.RegCount) > (uint)Cfg.Capacity) {
						PPError(PPERR_CHKINP_CAPACITYEXCESS, added_msg.Cat(Cfg.Capacity));
					}
					else if(Data.UpdateItem(pos, item, &Cfg)) {
						Data.SetMemo(pos, memo_buf);
						ok = 1;
					}
					else
						PPError();
				}
			}
		}
	}
	return ok;
}

int CheckInPersonListDialog::delItem(long pos, long id)
{
	return Data.RemoveItem((uint)pos) ? 1 : -1;
}

int SLAPI EditCheckInPersonList(const PPCheckInPersonConfig * pCfg, PPCheckInPersonArray * pData) { DIALOG_PROC_BODY_P1(CheckInPersonListDialog, pCfg, pData); }
//
// Implementation of PPALDD_CheckInPerson
//
PPALDD_CONSTRUCTOR(CheckInPerson)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(CheckInPerson) { Destroy(); }

int PPALDD_CheckInPerson::InitData(PPFilt & rFilt, long rsrv)
{
	PPCheckInPersonMngr mngr;
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		PPCheckInPersonItem item;
		MEMSZERO(H);
		H.ID = rFilt.ID;
		if(mngr.Search(rFilt.ID, &item) > 0) {
			H.Kind = item.Kind;
			H.PrmrID = item.PrmrID;
			H.PersonID = item.GetPerson();
			H.SCardID = item.SCardID;
			H.CCheckID = item.CCheckID;
			H.Num = item.Num;
			H.RegCount = item.RegCount;
			H.CiCount = item.CiCount;
			H.Flags = item.Flags;
			H.RegDt = item.RegDtm.d;
			H.RegTm = item.RegDtm.t;
			H.CiDt = item.CiDtm.d;
			H.CiTm = item.CiDtm.t;
			H.Amount = item.Amount;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
