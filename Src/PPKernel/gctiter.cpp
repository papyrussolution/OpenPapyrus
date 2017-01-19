// GCTITER.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017
// @Kernel
//
// GCTIterator
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(GCT); SLAPI GCTFilt::GCTFilt() : PPBaseFilt(PPFILT_GCT, 0, 0)
{
	SetFlatChunk(offsetof(GCTFilt, ReserveStart), offsetof(GCTFilt, BillList) - offsetof(GCTFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(GCTFilt, BillList));
	SetBranchObjIdListFilt(offsetof(GCTFilt, LocList));
	SetBranchObjIdListFilt(offsetof(GCTFilt, GoodsList));
	SetBranchObjIdListFilt(offsetof(GCTFilt, ArList));
	SetBranchObjIdListFilt(offsetof(GCTFilt, AgentList));
	Init(1, 0);
}

/*SLAPI GCTFilt::GCTFilt()
{
	Init();
}*/

/*int SLAPI GCTFilt::Init()
{
	BillList.Set(0);
	LocList.Set(0);
	GoodsList.Set(0);
	ArList.Set(0);    // @v7.9.11
	AgentList.Set(0); // @v7.9.11
	THISZERO();
	return 1;
}*/

int FASTCALL GCTFilt::CheckWL(long billFlags) const
{
	return BIN(!(Flags & OPG_LABELONLY) || (billFlags & BILLF_WHITELABEL));
}

int FASTCALL GCTFilt::AcceptIntr3(const BillTbl::Rec & rRec) const
{
	int    ok = 0;
	const  int intr = IsIntrOp(rRec.OpID);
	if(intr == INTREXPND) {
		if(LocList.GetCount() == 0) {
			ok = 2;
		}
		else if(LocList.Search(rRec.LocID, 0)) {
			ok = 1;
			const PPIDArray & r_loc_list = LocList.Get();
			for(uint i = 0; i < r_loc_list.getCount(); i++) {
				const PPID loc_id = r_loc_list.get(i);
				if(rRec.Object == PPObjLocation::WarehouseToObj(loc_id)) {
					ok = 2;
					break;
				}
			}
		}
		else {
			const PPIDArray & r_loc_list = LocList.Get();
			for(uint i = 0; i < r_loc_list.getCount(); i++) {
				const PPID loc_id = r_loc_list.get(i);
				if(rRec.Object == PPObjLocation::WarehouseToObj(loc_id)) {
					ok = 3;
					break;
				}
			}
		}
	}
	else if(!LocList.GetCount() || LocList.CheckID(rRec.LocID))
		ok = 1;
	return ok;
}
//
// GCT_BillCache
//
SLAPI GCTIterator::GCT_BillCache::GCT_BillCache()
{
	P_BObj = BillObj;
}

int FASTCALL GCTIterator::GCT_BillCache::CheckBillForAgent(PPID billID) const
{
	return ExtIdList.bsearch(billID);
}

int SLAPI GCTIterator::GCT_BillCache::SetupFilt(const GCTFilt * pFilt, const ObjIdListFilt & rArList, int disableCaching)
{
	int    ok = 1;
	Filt = *pFilt;
	if(Filt.OpID) {
		if(IsGenericOp(Filt.OpID) > 0) {
			PPIDArray temp_list;
			GetGenericOpList(Filt.OpID, &temp_list);
			OpList.Set(&temp_list);
		}
		else
			OpList.Add(Filt.OpID);
	}
	else
		OpList.Set(0);
	DisableCaching = BIN(disableCaching || Filt.GoodsID || Filt.SupplID);
	ExtIdList.freeAll();
	if(Filt.AgentList.GetCount()) {
		const PPIDArray & r_agent_list = Filt.AgentList.Get();
		PPIDArray temp_list;
		BillCore * p_billc = P_BObj->P_Tbl;
		for(uint i = 0; i < r_agent_list.getCount(); i++) {
			const PPID agent_id = r_agent_list.get(i);
			temp_list.clear();
			THROW(p_billc->GetBillListByExt(agent_id, 0, temp_list));
			THROW_SL(ExtIdList.add(&temp_list));
		}
		ExtIdList.sortAndUndup();
	}
	ArList = rArList;
	CATCHZOK
	return ok;
}

int FASTCALL GCTIterator::GCT_BillCache::CheckBillRec(const BillTbl::Rec * pRec)
{
	if(!Filt.CheckWL(pRec->Flags))
		return 0;
	else if(!OpList.CheckID(pRec->OpID))
		return 0;
	// @v8.9.0 {
	else if((Filt.Flags & OPG_SKIPNOUPDLOTREST) && CheckOpFlags(pRec->OpID, OPKF_NOUPDLOTREST, 0))
		return 0;
	// } @v8.9.0
	if(!Filt.SoftRestrict) {
		if(!ArList.CheckID(pRec->Object))
			return 0;
		else if(Filt.Flags & OPG_BYZEROAGENT) {
			PPBillExt bext_rec;
			if(P_BObj->FetchExt(pRec->ID, &bext_rec) && bext_rec.AgentID)
				return 0;
		}
		else if(Filt.AgentList.GetCount() && !ExtIdList.bsearch(pRec->ID))
			return 0;
	}
	if(ArList.GetSingle() && (Filt.DlvrAddrID || Filt.Flags & OPG_BYZERODLVRADDR)) {
		PPFreight freight;
		if(pRec->Flags & BILLF_FREIGHT && P_BObj->P_Tbl->GetFreight(pRec->ID, &freight) > 0) {
			if(Filt.Flags & OPG_BYZERODLVRADDR) {
				if(freight.DlvrAddrID != 0)
					return 0;
			}
			else if(freight.DlvrAddrID != Filt.DlvrAddrID)
				return 0;
		}
		else if(!(Filt.Flags & OPG_BYZERODLVRADDR) && Filt.DlvrAddrID)
			return 0;
	}
	return 1;
}

int FASTCALL GCTIterator::GCT_BillCache::Get(PPID id, BillTbl::Rec * pRec)
{
	int    ok = -1;
	if(DisableCaching) {
		if(!(Filt.Flags & OPG_BYZEROAGENT) && Filt.AgentList.GetCount() && !Filt.SoftRestrict && !ExtIdList.bsearch(id))
			ok = -1;
		else
			ok = (P_BObj->Search(id, pRec) > 0 && CheckBillRec(pRec)) ? 1 : -1;
	}
	else if(P_BObj->Fetch(id, pRec) > 0 && CheckBillRec(pRec))
		ok = 1;
	return ok;
}
//
//
//
IMPL_CMPCFUNC(GCTIterator_GoodsRestEntry, p1, p2)
{
	int    si = 0;
    CMPCASCADE3(si, (const GCTIterator::GoodsRestEntry *)p1, (const GCTIterator::GoodsRestEntry *)p2, GoodsID, LocID, Dt);
	return si;
}

IMPL_CMPCFUNC(GCTIterator_GoodsRestEntry_ByGoodsLoc, p1, p2)
{
	int    si = 0;
    CMPCASCADE2(si, (const GCTIterator::GoodsRestEntry *)p1, (const GCTIterator::GoodsRestEntry *)p2, GoodsID, LocID);
	return si;
}

SLAPI GCTIterator::GoodsRestArray::GoodsRestArray() : TSArray <GoodsRestEntry> ()
{
	Init();
}

void SLAPI GCTIterator::GoodsRestArray::Init()
{
	State = stAccumulation;
	AccumPeriod.Set(MAXDATE, encodedate(1, 1, 1900));
	LocList.clear();
	SArray::clear();
}

int SLAPI GCTIterator::GoodsRestArray::SetAccumItem(PPID goodsID, PPID locID, LDATE dt, double qtty)
{
	int    ok = -1;
	if(State & stAccumulation) {
		if(checkdate(dt, 0)) {
			GoodsRestEntry new_entry;
			new_entry.GoodsID = goodsID;
			new_entry.LocID = locID;
			new_entry.Dt = dt;
			uint   p = 0;
			if(lsearch(&new_entry, &p, PTR_CMPCFUNC(GCTIterator_GoodsRestEntry))) {
				GoodsRestEntry & r_entry = at(p);
				r_entry.Rest += qtty;
				ok = 2;
			}
			else {
				new_entry.Rest = qtty;
				THROW_SL(insert(&new_entry));
				AccumPeriod.AdjustToDate(dt);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI GCTIterator::GoodsRestArray::SetInitRest(PPID goodsID, PPID locID, double rest)
{
	int    ok = -1;
	if(State & stAccumulation) {
		GoodsRestEntry new_entry;
		new_entry.GoodsID = goodsID;
		new_entry.LocID = locID;
		new_entry.Dt = ZERODATE;
		uint   p = 0;
		if(lsearch(&new_entry, &p, PTR_CMPCFUNC(GCTIterator_GoodsRestEntry))) {
			GoodsRestEntry & r_entry = at(p);
			r_entry.Rest += rest;
			ok = 2;
		}
		else {
			new_entry.Rest = rest;
			THROW_SL(insert(&new_entry));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

void SLAPI GCTIterator::GoodsRestArray::Finish()
{
	if(State & stAccumulation) {
		sort(PTR_CMPCFUNC(GCTIterator_GoodsRestEntry));
		{
			LocList.clear();
			PPID   current_goods_id = 0;
			PPID   current_loc_id = 0;
			double current_rest = 0.0;
			for(uint i = 0; i < getCount(); i++) {
				GoodsRestEntry & r_entry = at(i);
				const PPID loc_id = r_entry.LocID;
				const PPID goods_id = r_entry.GoodsID;
				LocList.add(loc_id);
				if(loc_id != current_loc_id || goods_id != current_goods_id) {
					current_goods_id = goods_id;
					current_loc_id = loc_id;
                    current_rest = (!r_entry.Dt) ? r_entry.Rest : 0.0;
				}
                if(r_entry.Dt) {
					const double new_rest = current_rest + r_entry.Rest;
                    r_entry.Rest = new_rest;
					current_rest = new_rest;
                }
			}
			LocList.sortAndUndup();
		}
		State &= ~stAccumulation;
	}
}

double SLAPI GCTIterator::GoodsRestArray::GetRest(PPID goodsID, PPID locID, LDATE dt) const
{
	double result = 0.0;
	uint   pos = 0;
	GCTIterator::GoodsRestEntry key;
	MEMSZERO(key);
	key.GoodsID = goodsID;
	key.LocID = locID;
	if(bsearch(&key, &pos, PTR_CMPCFUNC(GCTIterator_GoodsRestEntry_ByGoodsLoc))) {
		uint last_pos = pos;
        while(pos < getCount() && at(pos).GoodsID == goodsID && at(pos).LocID == locID) {
			last_pos = pos++;
        }
		for(int ipos = (int)last_pos; ipos >= 0; ipos--) {
            const GoodsRestEntry & r_entry = at(ipos);
			if(r_entry.GoodsID == goodsID && r_entry.LocID == locID && r_entry.Dt <= dt) {
				result = r_entry.Rest;
				break;
			}
		}
	}
	return result;
}

double SLAPI GCTIterator::GoodsRestArray::GetRest(PPID goodsID, LDATE dt) const
{
	double result = 0.0;
	for(uint i = 0; i < LocList.getCount(); i++) {
		const PPID loc_id = LocList.get(i);
		const double lrest = GetRest(goodsID, loc_id, dt);
		result += lrest;
	}
	return result;
}

double SLAPI GCTIterator::GoodsRestArray::GetAverageRest(PPID goodsID, PPID locID, const DateRange & rPeriod) const
{
	double result = 0.0;
	DateRange period = AccumPeriod; // rPeriod;
	if(period.low <= period.upp) {
		RealArray list;
		for(LDATE dt = period.low; dt <= period.upp; dt = plusdate(dt, 1)) {
			const double rest = locID ? GetRest(goodsID, locID, dt) : GetRest(goodsID, dt);
			list.insert(&rest);
		}
		if(list.getCount()) // @paranoic
			result = list.Sum() / list.getCount();
	}
	return result;
}

int SLAPI GCTIterator::GoodsRestArray::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, (SArray *)this, rBuf));
	THROW_SL(pSCtx->Serialize(dir, State, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AccumPeriod, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &LocList, rBuf));
	CATCHZOK
	return ok;
}

// static
long SLAPI GCTIterator::AnalyzeOp(PPID opID, PPIDArray * pResultOpList)
{
	long   result = 0;
	PPIDArray op_list;
	if(IsGenericOp(opID) > 0) {
		result |= aorfGeneric;
		GetGenericOpList(opID, &op_list);
	}
	else if(opID == PPOPK_INTRRECEIPT) {
		result |= aorfIntrRcpt;
		PPOprKind op_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); en.Next(&op_rec) > 0;)
			if(IsIntrExpndOp(op_rec.ID))
				op_list.add(op_rec.ID);
	}
	else if(opID)
		op_list.add(opID);
	{
		const uint oc = op_list.getCount();
		if(oc) {
			result |= aorfOnlyDrafts;
			result |= aorfOnlyOrders;
			for(uint i = 0; i < oc; i++) {
				const PPID op_id = op_list.get(i);
				const PPID op_type_id = GetOpType(op_id);
				if(oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
					result |= stThereAreDrafts;
					result &= ~stOnlyOrders;
				}
				else {
					result &= ~stOnlyDrafts;
					if(op_type_id == PPOPT_GOODSORDER)
						result |= stThereAreOrders;
					else
						result &= ~stOnlyOrders;
				}
			}
		}
	}
	ASSIGN_PTR(pResultOpList, op_list);
	return result;
}

//
// GCTIterator
//
SLAPI GCTIterator::GCTIterator(const GCTFilt * pFilt, const DateRange * pDRange)
{
	Filt = *pFilt;
	State = 0;
	IterPhase = iterphaseInit;
	Period = *pDRange;
	trfr_q = rcpt_q = 0;
	cptrfr_q = 0; // @v9.4.1
	{
		PPObjBill * p_bobj = BillObj;
		Trfr = p_bobj->trfr;
		CpTrfr = p_bobj->P_CpTrfr;
		BT   = p_bobj->P_Tbl;
	}
	BCache = 0;
	P_GoodsRestList = 0;
	if(Filt.ArList.GetCount()) {
		const PPIDArray & r_ar_list = Filt.ArList.Get();
		PPIDArray list;
		PPObjPersonRelType prt_obj;
		PPIDArray grp_prt_list;
		list.add(&r_ar_list);
		if(prt_obj.GetGroupingList(&grp_prt_list) > 0) {
			PPObjArticle ar_obj;
			PPObjPerson psn_obj;
			PPIDArray temp_list;
			for(uint j = 0; j < r_ar_list.getCount(); j++) {
				const PPID ar_id = r_ar_list.get(j);
				temp_list.clear();
				for(uint i = 0; i < grp_prt_list.getCount(); i++)
					ar_obj.GetRelPersonList(ar_id, grp_prt_list.get(i), 1, &temp_list);
				list.add(&temp_list);
			}
		}
		if(list.getCount()) {
			list.sortAndUndup();
			ArList.Set(&list);
		}
	}
	if(Filt.SupplAgentID) {
		BT->GetBillListByExt(Filt.SupplAgentID, 0L, SupplAgentBillList);
		// @v8.1.0 (сортировку теперь выполняет GetBillListByExt) SupplAgentBillList.sort();
	}
	{
		// @v9.4.10 {
		PPIDArray op_list;
		const long aor = AnalyzeOp(Filt.OpID, &op_list);
		SETFLAG(State, stOnlyDrafts, aor & aorfOnlyDrafts);
		SETFLAG(State, stOnlyOrders, aor & aorfOnlyOrders);
		SETFLAG(State, stThereAreDrafts, aor & aorfThereAreDrafts);
		SETFLAG(State, stThereAreOrders, aor & aorfThereAreOrders);
		if(op_list.getCount())
			OpList.Set(&op_list);
		// } @v9.4.10
#if 0 // @v9.4.10 {
		if(IsGenericOp(Filt.OpID) > 0)
			GetGenericOpList(Filt.OpID, &op_list);
		else if(Filt.OpID == PPOPK_INTRRECEIPT) {
			PPOprKind op_rec;
			for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); en.Next(&op_rec) > 0;)
				if(IsIntrExpndOp(op_rec.ID))
					op_list.add(op_rec.ID);
		}
		else if(Filt.OpID)
			op_list.add(Filt.OpID);
		{
			const uint oc = op_list.getCount();
			if(oc) {
				// @v9.4.2 {
				State |= stOnlyDrafts;
				State |= stOnlyOrders;
				for(uint i = 0; i < oc; i++) {
					const PPID op_id = op_list.get(i);
					const PPID op_type_id = GetOpType(op_id);
					if(oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
						State |= stThereAreDrafts;
						State &= ~stOnlyOrders;
					}
					else {
						State &= ~stOnlyDrafts;
						if(op_type_id == PPOPT_GOODSORDER)
							State |= stThereAreOrders;
						else
							State &= ~stOnlyOrders;
					}
				}
				/*
				if(Filt.SupplID || Filt.SupplAgentID || !Filt.LotsPeriod.IsZero())
					State &= ~stThereAreDrafts;
				*/
				// } @v9.4.2
				OpList.Set(&op_list);
			}
		}
#endif // } 0 @v9.4.10
	}
}

SLAPI GCTIterator::~GCTIterator()
{
	delete trfr_q;
	delete rcpt_q;
	delete cptrfr_q; // @v9.4.1
	delete BCache;
	delete P_GoodsRestList;
}

int FASTCALL GCTIterator::CheckBillForFilt(const BillTbl::Rec & rBillRec) const
{
	const  int soft_restr = BIN(Filt.SoftRestrict);
	if((Filt.Flags & OPG_LABELONLY) && !(rBillRec.Flags & BILLF_WHITELABEL))
		return 0;
	else if(!Period.CheckDate(rBillRec.Dt))
		return 0;
	else if(!OpList.CheckID(rBillRec.OpID))
		return 0;
	else if(!soft_restr && !ArList.CheckID(rBillRec.Object))
		return 0;
	else if(!Filt.BillList.CheckID(rBillRec.ID))
		return 0;
	else if(!Filt.AcceptIntr3(rBillRec))
		return 0;
	else if(!soft_restr) {
		PPObjBill * p_bobj = BillObj;
		PPBillExt ext_rec;
		if(Filt.Flags & OPG_BYZEROAGENT) {
			if(p_bobj->FetchExt(rBillRec.ID, &ext_rec) > 0 && ext_rec.AgentID)
				return 0;
		}
		else if(Filt.AgentList.GetCount()) {
			const PPIDArray & r_agent_list = Filt.AgentList.Get();
			int    f = 0;
			for(uint i = 0; !f && i < r_agent_list.getCount(); i++) {
				const PPID agent_id = r_agent_list.get(i);
				if(p_bobj->FetchExt(rBillRec.ID, &ext_rec) > 0 && ext_rec.AgentID == agent_id)
					f = 1;
			}
			if(!f)
				return 0;
		}
	}
	return 1;
}

const GCTIterator::GoodsRestArray * GCTIterator::GetGoodsRestList() const
{
    return P_GoodsRestList;
}

int FASTCALL GCTIterator::SetupGoodsRest(TransferTbl::Rec * pRec)
{
	return P_GoodsRestList ? P_GoodsRestList->SetAccumItem(pRec->GoodsID, pRec->LocID, pRec->Dt, pRec->Quantity) : -1;
}

int SLAPI GCTIterator::InitQuery(int cpMode)
{
	int    ok = 1;
	const  int soft_restr = BIN(Filt.SoftRestrict);
	struct {
		PPID  obj;
		LDATE dt;
		long  oprno;
	} k; // #
	int    idx = 0;
	DBQ *  dbq = 0;
	ReceiptTbl * rt = & Trfr->Rcpt;
	PPObjGoods g_obj;
	ByWhat_ = bwNone;
	CurrID = 0;
	State &= ~stUseGoodsList;
	BillList.clear();
	GoodsArray.clear();
	if(Filt.GoodsList.GetCount()) {
		Filt.GoodsGrpID = 0;
		Filt.GoodsID = Filt.GoodsList.GetSingle();
		if(Filt.GoodsID)
			Filt.GoodsList.FreeAll();
		else {
			GoodsArray = Filt.GoodsList.Get();
			State |= stUseGoodsList;
		}
	}
	if(Filt.GoodsID) {
		if(g_obj.IsGeneric(Filt.GoodsID) > 0) {
			THROW(g_obj.GetGenericList(Filt.GoodsID, &GoodsArray));
			State |= stUseGoodsList;
		}
		else {
			if(State & stOnlyOrders)
				Filt.GoodsID = -labs(Filt.GoodsID);
			else if(State & stThereAreOrders) {
				GoodsArray.addUnique(labs(Filt.GoodsID));
				State |= stUseGoodsList;
			}
		}
	}
	else if(Filt.GoodsGrpID || Filt.BrandID) {
		GoodsFilt goods_filt;
		goods_filt.GrpID = Filt.GoodsGrpID;
		goods_filt.BrandList.Add(Filt.BrandID);
		THROW(GoodsIterator::GetListByFilt(&goods_filt, &GoodsArray));
		State |= stUseGoodsList;
	}
	if(State & stUseGoodsList) {
		if(State & stThereAreOrders) {
			const uint _c = GoodsArray.getCount();
			if(State & stOnlyOrders) {
				for(uint i = 0; i < _c; i++)
					GoodsArray.at(i) = -labs(GoodsArray.get(i));
			}
			else {
				for(uint i = 0; i < _c; i++)
					GoodsArray.add(-labs(GoodsArray.get(i))); // @v8.1.0 addUnique-->add
			}
		}
		GoodsArray.sortAndUndup(); // @v8.1.0 sort-->sortAndUndup
	}
	if(Filt.SupplID) {
		if(cpMode)
			ok = -1;
		else {
			LDATE  e = NZOR(Period.upp, MAXDATE);
			if(Filt.GoodsID && !(State & stUseGoodsList)) {
				dbq   = & (rt->Dt <= e && rt->SupplID == Filt.SupplID);
				if(!soft_restr) {
					dbq = & (rt->GoodsID == Filt.GoodsID);
					k.obj = Filt.GoodsID;
				}
				else
					k.obj = 0;
				idx   = 2;
			}
			else {
				dbq   = &(rt->SupplID == Filt.SupplID && rt->Dt <= e);
				k.obj = Filt.SupplID;
				idx   = 5;
			}
			dbq = ppcheckfiltidlist(dbq, rt->LocID, &Filt.LocList.Get());
			dbq = ppcheckfiltidlist(dbq, rt->BillID, &Filt.BillList.Get());
			THROW_MEM(rcpt_q = new BExtQuery(rt, idx, 64));
			rcpt_q->select(rt->ID, rt->GoodsID, 0L).where(*dbq);
			k.dt    = ZERODATE;
			k.oprno = 0;
			ok = -1;
			for(rcpt_q->initIteration(0, &k, spGt); rcpt_q->nextIteration() > 0;) {
				ok = 1;
				if(!(State & stUseGoodsList) || soft_restr || GoodsArray.bsearch(rt->data.GoodsID)) {
					ByWhat_ = bwLot;
					CurrID = rt->data.ID;
					break;
				}
			}
		}
	}
	else {
		if(!cpMode && Filt.Flags & OPG_STOREDAYLYRESTS) {
            if(P_GoodsRestList)
				P_GoodsRestList->clear();
			else
				THROW_MEM(P_GoodsRestList = new GCTIterator::GoodsRestArray);
		}
		if(cpMode || (!Filt.GoodsID && ((ArList.GetCount() && !soft_restr) || OpList.GetCount() || Filt.BillList.GetCount()))) {
			if(Filt.BillList.GetCount()) {
				BillList = Filt.BillList.Get();
			}
			else if(OpList.GetCount()) {
				for(uint i = 0; i < OpList.GetCount(); i++) {
					const PPID op_id = OpList.Get().get(i);
					if(!cpMode || IsDraftOp(op_id)) {
						BillTbl::Key2 k2;
						MEMSZERO(k2);
						k2.OpID = op_id;
						k2.Dt = Period.low;
						BExtQuery q(BT, 2, 256);
						q.select(BT->ID, BT->Code, BT->Dt, BT->OpID, BT->Object, BT->Flags, BT->LocID, 0L).
							where(BT->OpID == op_id && daterange(BT->Dt, &Period));
						for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
							if(CheckBillForFilt(BT->data)) {
								BillList.add(BT->data.ID);
							}
						}
					}
				}
			}
			else { // ArList.GetCount() && !soft_restr
				for(uint i = 0; i < ArList.GetCount(); i++) {
					const PPID ar_id = ArList.Get().get(i);
					BillTbl::Key3 k3;
					MEMSZERO(k3);
					k3.Object = ar_id;
					k3.Dt = Period.low;
					BExtQuery q(BT, 3, 256);
					q.select(BT->ID, BT->Code, BT->Dt, BT->OpID, BT->Object, BT->Flags, BT->LocID, 0L).
						where(BT->Object == ar_id && daterange(BT->Dt, &Period));
					for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
						if(CheckBillForFilt(BT->data)) {
							BillList.add(BT->data.ID);
						}
					}
				}
			}
			if(BillList.getCount()) {
				BillList.sort();
				if(!cpMode && P_GoodsRestList) {
					//
					// Если необходимо аккумулировать товарные остатки по датам, то придется перебирать все операции,
					// а не только по заданным документам
					//
					ByWhat_ = bwNone;
					CurrID = 0;
				}
				else {
					ByWhat_ = bwBill;
					BillList.setPointer(0);
					CurrID = BillList.get(BillList.incPointer());
				}
			}
			else
				ok = -1;
		}
		else {
			ByWhat_ = bwNone;
			CurrID = 0;
		}
	}
	if(ok > 0) {
		assert(!cpMode || ByWhat_ == bwBill);
		if(!BCache) {
			THROW_MEM(BCache = new GCT_BillCache);
			THROW(BCache->SetupFilt(&Filt, ArList, (Filt.Flags & OPG_FORCEBILLCACHE) ? 0 : 1));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI GCTIterator::NextOuter()
{
	int    ok = -1;
	const  int soft_restr = BIN(Filt.SoftRestrict);
	Cbb.Clear();
	switch(ByWhat_) {
		case bwLot:
			while(ok < 0 && rcpt_q->nextIteration() > 0) {
				if(!(State & stUseGoodsList) || soft_restr || GoodsArray.bsearch(Trfr->Rcpt.data.GoodsID)) {
					CurrID = Trfr->Rcpt.data.ID;
					ok = 1;
				}
			}
			break;
		case bwBill:
			while(ok < 0 && BillList.getPointer() < BillList.getCount()) {
				const PPID next_id = BillList.get(BillList.incPointer());
				if(next_id != CurrID) {
					CurrID = next_id;
					ok = 1;
				}
			}
			break;
		case bwGoods:
			GoodsArray.incPointer();
			if(GoodsArray.getPointer() < GoodsArray.getCount())
				ok = 1;
			break;
	}
	return ok;
}

int SLAPI GCTIterator::AcceptTrfrRec(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = -1;
	const  int    soft_restr = BIN(Filt.SoftRestrict);
	TransferTbl::Rec & rec = Trfr->data;
	const PPID goods_id = rec.GoodsID;
	const PPID bill_id = rec.BillID;
	int  goods_found = 0;
	if(State & stUseGoodsList) {
		PROFILE(goods_found = GoodsArray.bsearch(goods_id));
	}
	if(!(State & stUseGoodsList) || goods_found || soft_restr) {
		ok = 1;
		SetupGoodsRest(&rec); // @v9.1.3
		if(!soft_restr && (!Filt.LotsPeriod.IsZero() || Filt.SupplAgentID)) {
			PPID   lot_id = rec.LotID;
			if(Filt.SupplAgentID && !lot_id)
				ok = -1;
			else {
				PPIDArray lot_id_list; // Storage for cycle safe code
				ReceiptTbl::Rec lot_rec;
				while(lot_id && Trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					const PPID prev_id = lot_rec.PrevLotID;
					lot_id_list.add(lot_id);
					if(prev_id && !lot_id_list.lsearch(prev_id))
						lot_id = prev_id;
					else if(Filt.LotsPeriod.CheckDate(lot_rec.Dt) && (!Filt.SupplAgentID || SupplAgentBillList.bsearch(lot_rec.BillID)))
						lot_id = 0;
					else {
						lot_id = 0;
						ok = -1;
					}
				}
				if(lot_id)
					ok = -1;
			}
		}
		if(ok > 0) {
			ok = (BCache && BCache->Get(bill_id, pBillRec) > 0) ? 1 : -1;
			if(ok > 0) {
				Trfr->copyBufTo(pTrfrRec);
				SETMAX(SurrOprNo, pTrfrRec->OprNo);
			}
			if(soft_restr) {
				PPObjBill * p_bobj = BillObj;
				PPBillExt ext_rec;
				int    article_notvalid = !ArList.CheckID(pBillRec->Object);
				int    goods_notvalid = BIN(Filt.GoodsID && labs(Filt.GoodsID) != goods_id || (State & stUseGoodsList) && !goods_found); // @v7.1.3 labs()
				int    agent_notvalid = 0;
				if(Filt.Flags & OPG_BYZEROAGENT) {
					if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID != 0)
						agent_notvalid = 1;
				}
				else if(Filt.AgentList.GetCount()) {
					if(BCache) {
						if(!BCache->CheckBillForAgent(bill_id))
							agent_notvalid = 1;
					}
					else {
						const PPIDArray & r_agent_list = Filt.AgentList.Get();
						int    f = 0;
						for(uint i = 0; !f && i < r_agent_list.getCount(); i++) {
							const PPID agent_id = r_agent_list.get(i);
							if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID == agent_id)
								f = 1;
						}
						if(!f)
							agent_notvalid = 1;
					}
				}
				if(article_notvalid || agent_notvalid || goods_notvalid) {
					pTrfrRec->Cost     = 0.0;
					pTrfrRec->Rest     = 0.0;
					pTrfrRec->CurPrice = 0.0;
					pTrfrRec->WtQtty   = 0.0f;
					pTrfrRec->WtRest   = 0.0f;
					pTrfrRec->Price    = 0.0;
					pTrfrRec->Quantity = 0.0;
					pBillRec->Amount   = 0.0;
					if(article_notvalid)
						pBillRec->Object = Filt.ArList.GetSingle(); // Здесь присваиваем контрагента (возможно группирующего) из фильтра
					if(goods_notvalid)
						pTrfrRec->GoodsID = NZOR(Filt.GoodsID, Filt.GoodsGrpID);
				}
			}
		}
	}
	return ok;
}

int SLAPI GCTIterator::AcceptCpTrfrRec(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = -1;
	const  int    soft_restr = BIN(Filt.SoftRestrict);
	CpTransfTbl::Rec & r_cprec = CpTrfr->data;
	const PPID goods_id = r_cprec.GoodsID;
	const PPID bill_id = r_cprec.BillID;
	int  goods_found = 0;
	if(State & stUseGoodsList) {
		PROFILE(goods_found = GoodsArray.bsearch(goods_id));
	}
	if(!(State & stUseGoodsList) || goods_found || soft_restr) {
		ok = (BCache && BCache->Get(bill_id, pBillRec) > 0) ? 1 : -1;
		if(ok > 0) {
			memzero(pTrfrRec, sizeof(*pTrfrRec));
			pTrfrRec->BillID = r_cprec.BillID;
			pTrfrRec->OprNo = ++SurrOprNo;
			pTrfrRec->Dt = pBillRec->Dt;
			pTrfrRec->Flags = r_cprec.Flags;
			pTrfrRec->RByBill = r_cprec.RByBill;
			pTrfrRec->LocID = r_cprec.LocID;
			pTrfrRec->GoodsID = r_cprec.GoodsID;
			pTrfrRec->CurID = r_cprec.CurID;
			pTrfrRec->Quantity = r_cprec.Qtty;
			pTrfrRec->Cost = r_cprec.Cost;
			pTrfrRec->Price = r_cprec.Price;
			pTrfrRec->Discount = r_cprec.Discount;
			pTrfrRec->CurPrice = r_cprec.CurPrice;
		}
		if(soft_restr) {
			PPObjBill * p_bobj = BillObj;
			PPBillExt ext_rec;
			const int article_notvalid = !ArList.CheckID(pBillRec->Object);
			const int goods_notvalid = BIN(Filt.GoodsID && labs(Filt.GoodsID) != goods_id || (State & stUseGoodsList) && !goods_found);
			int    agent_notvalid = 0;
			if(Filt.Flags & OPG_BYZEROAGENT) {
				if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID != 0)
					agent_notvalid = 1;
			}
			else if(Filt.AgentList.GetCount()) {
				if(BCache) {
					if(!BCache->CheckBillForAgent(bill_id))
						agent_notvalid = 1;
				}
				else {
					const PPIDArray & r_agent_list = Filt.AgentList.Get();
					int    f = 0;
					for(uint i = 0; !f && i < r_agent_list.getCount(); i++) {
						const PPID agent_id = r_agent_list.get(i);
						if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID == agent_id)
							f = 1;
					}
					if(!f)
						agent_notvalid = 1;
				}
			}
			if(article_notvalid || agent_notvalid || goods_notvalid) {
				pTrfrRec->Cost     = 0.0;
				pTrfrRec->Rest     = 0.0;
				pTrfrRec->CurPrice = 0.0;
				pTrfrRec->WtQtty   = 0.0f;
				pTrfrRec->WtRest   = 0.0f;
				pTrfrRec->Price    = 0.0;
				pTrfrRec->Quantity = 0.0;
				pBillRec->Amount   = 0.0;
				if(article_notvalid)
					pBillRec->Object = Filt.ArList.GetSingle(); // Здесь присваиваем контрагента (возможно группирующего) из фильтра
				if(goods_notvalid)
					pTrfrRec->GoodsID = NZOR(Filt.GoodsID, Filt.GoodsGrpID);
			}
		}
	}
	return ok;
}

int SLAPI GCTIterator::TrfrQuery(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	union {
		TransferTbl::Key0 k0;
		TransferTbl::Key1 k1;
		TransferTbl::Key2 k2;
		TransferTbl::Key3 k3;
	} k;
	union {
		CpTransfTbl::Key0 k0;
		CpTransfTbl::Key1 k1;
	} cpk;
	const  int opt_for_psales = BIN(Filt.Flags & OPG_OPTIMIZEFORPSALES && (State & stUseGoodsList) && !Filt.SoftRestrict);
	int    idx;
	DBQ  * dbq = 0;
	MEMSZERO(k);
	MEMSZERO(cpk);
	if(ByWhat_ == bwLot) {
		idx = 2;
		k.k2.LotID = CurrID;
		k.k2.Dt = Period.low;
		dbq = & (Trfr->LotID == CurrID);
	}
	else if(ByWhat_ == bwBill) {
		idx = 0;
		k.k0.BillID = CurrID;
		dbq = & (Trfr->BillID == CurrID);
	}
	else if(ByWhat_ == bwGoods) {
		idx = 3;
		PPID   goods_id = GoodsArray.get(GoodsArray.getPointer());
		k.k3.GoodsID = goods_id;
		k.k3.Dt = Period.low;
		dbq = & (Trfr->GoodsID == goods_id);
	}
	else if(Filt.GoodsID && !(State & stUseGoodsList) && !Filt.SoftRestrict) {
		idx = 3;
		k.k3.GoodsID = Filt.GoodsID;
		k.k3.Dt = Period.low;
		dbq = & (Trfr->GoodsID == Filt.GoodsID);
	}
	else {
		idx = 1;
		k.k1.Dt = Period.low;
	}
	if(ByWhat_ != bwBill) {
		dbq = & (*dbq && daterange(Trfr->Dt, &Period));
		if(ByWhat_ != bwGoods && (State & stUseGoodsList) && GoodsArray.getCount() && !Filt.SoftRestrict) {
			dbq = &(*dbq && Trfr->GoodsID >= GoodsArray.get(0) && Trfr->GoodsID <= GoodsArray.getLast());
		}
	}
	else if((State & stUseGoodsList) && GoodsArray.getCount() && !Filt.SoftRestrict) {
		dbq = &(*dbq && Trfr->GoodsID >= GoodsArray.get(0) && Trfr->GoodsID <= GoodsArray.getLast());
	}
	dbq = ppcheckfiltidlist(dbq, Trfr->LocID, &Filt.LocList.Get());
	BExtQuery * q = new BExtQuery(Trfr, idx, 256);
	if(q == 0)
		return PPSetErrorNoMem();
	if(opt_for_psales) {
		q->select(Trfr->Dt, Trfr->BillID, Trfr->GoodsID, Trfr->LocID, Trfr->Flags,
			Trfr->Quantity, Trfr->Price, Trfr->Discount, 0L).where(*dbq);
	}
	else {
		q->select(Trfr->Dt, Trfr->OprNo, Trfr->BillID, Trfr->GoodsID, Trfr->LocID, Trfr->LotID, Trfr->Flags,
			Trfr->Quantity, Trfr->Rest, Trfr->Cost, Trfr->Price, Trfr->Discount, 0L).where(*dbq);
	}
	delete trfr_q;
	trfr_q  = q;
	trfr_q->initIteration(0, &k, spGt);
	return NextTrfr(pTrfrRec, pBillRec, pExt);
}

int SLAPI GCTIterator::CpTrfrQuery(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = -1;
	int    done = 0;
	union {
		CpTransfTbl::Key0 k0;
		CpTransfTbl::Key1 k1;
	} cpk;
	int    idx;
	int    opt_for_psales = BIN(Filt.Flags & OPG_OPTIMIZEFORPSALES && (State & stUseGoodsList) && !Filt.SoftRestrict);
	DBQ  * dbq = 0;
	MEMSZERO(cpk);
	// @v9.4.10 {
	Cbb.Clear();
	if(Filt.Flags & OPG_COMPAREWROFF && ByWhat_ == bwBill) {
		BillTbl::Rec bill_rec;
        if(BCache && BCache->Get(CurrID, &bill_rec) > 0 && IsDraftOp(bill_rec.OpID)) {
			PPObjBill * p_bobj = BillObj;
			PPTransferItem ti;
            THROW_MEM(SETIFZ(Cbb.P_Pack, new PPBillPacket));
			if(p_bobj->ExtractPacket(CurrID, Cbb.P_Pack, 0) > 0) {
				for(DateIter di; BT->EnumLinks(Cbb.P_Pack->Rec.ID, &di, BLNK_ALL, &bill_rec) > 0;) {
					PPBillPacket temp_pack;
					THROW_MEM(SETIFZ(Cbb.P_WrOffPack, new PPBillPacket));
					if(p_bobj->ExtractPacket(bill_rec.ID, &temp_pack) > 0) {
						for(temp_pack.InitExtTIter(ETIEF_UNITEBYGOODS, 0); temp_pack.EnumTItemsExt(0, &ti) > 0;) {
							THROW(Cbb.P_WrOffPack->LoadTItem(&ti, 0, 0));
						}
					}
				}
				if(Cbb.P_WrOffPack) {
					for(Cbb.P_WrOffPack->InitExtTIter(ETIEF_UNITEBYGOODS, 0); Cbb.P_WrOffPack->EnumTItemsExt(0, &ti) > 0;) {
						if(!Cbb.P_Pack->SearchGoods(ti.GoodsID, 0)) {
							ti.Quantity_ = 0.0;
							ti.Cost = 0.0;
							ti.Price = 0.0;
							ti.Discount = 0.0;
							ti.WtQtty = 0.0;
							THROW(Cbb.P_Pack->LoadTItem(&ti, 0, 0));
						}
					}
				}
				Cbb.P_Pack->InitExtTIter(ETIEF_UNITEBYGOODS, 0);
				ok = NextCpTrfr(pTrfrRec, pBillRec, pExt);
			}
			else {
				Cbb.Clear();
				ok = -1;
			}
			done = 1;
        }
	}
	// } @v9.4.10
	if(!done && oneof2(ByWhat_, bwBill, bwGoods)) {
		if(ByWhat_ == bwBill) {
			idx = 0;
			cpk.k0.BillID = CurrID;
			dbq = & (CpTrfr->BillID == CurrID);
		}
		else if(ByWhat_ == bwGoods) {
			idx = 3;
			const PPID goods_id = GoodsArray.get(GoodsArray.getPointer());
			cpk.k1.GoodsID = goods_id;
			dbq = & (CpTrfr->GoodsID == goods_id);
		}
		if(ByWhat_ != bwBill) {
			if(ByWhat_ != bwGoods && (State & stUseGoodsList) && GoodsArray.getCount() && !Filt.SoftRestrict) {
				dbq = &(*dbq && CpTrfr->GoodsID >= GoodsArray.get(0) && CpTrfr->GoodsID <= GoodsArray.getLast());
			}
		}
		else if((State & stUseGoodsList) && GoodsArray.getCount() && !Filt.SoftRestrict) {
			dbq = &(*dbq && CpTrfr->GoodsID >= GoodsArray.get(0) && CpTrfr->GoodsID <= GoodsArray.getLast());
		}
		else
			dbq = ppcheckfiltid(dbq, CpTrfr->GoodsID, Filt.GoodsID);
		dbq = ppcheckfiltidlist(dbq, CpTrfr->LocID, &Filt.LocList.Get());
		BExtQuery * q = new BExtQuery(CpTrfr, idx, 256);
		THROW_MEM(q);
		q->select(CpTrfr->BillID, CpTrfr->GoodsID, CpTrfr->LocID, CpTrfr->Flags,
			CpTrfr->Qtty, CpTrfr->Rest, CpTrfr->Cost, CpTrfr->Price, CpTrfr->Discount, 0L).where(*dbq);
		delete cptrfr_q;
		cptrfr_q  = q;
		cptrfr_q->initIteration(0, &cpk, spGt);
		ok = NextCpTrfr(pTrfrRec, pBillRec, pExt);
	}
	CATCHZOK
	return ok;
}

int SLAPI GCTIterator::NextTrfr(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	while(trfr_q->nextIteration() > 0) {
		if(AcceptTrfrRec(pTrfrRec, pBillRec, pExt) > 0)
			return 1;
	}
	return -1;
}

int SLAPI GCTIterator::NextCpTrfr(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = -1;
	if(Cbb.P_Pack) {
		PPTransferItem ti;
		const  int soft_restr = BIN(Filt.SoftRestrict);
		while(ok < 0 && Cbb.P_Pack->EnumTItemsExt(0, &ti) > 0) {
			const PPID goods_id = labs(ti.GoodsID);
			int  goods_found = 0;
			if(State & stUseGoodsList) {
				PROFILE(goods_found = GoodsArray.bsearch(goods_id));
			}
			else
				goods_found = 1;
			if(goods_found || soft_restr) {
				const PPID bill_id = Cbb.P_Pack->Rec.ID;
				ASSIGN_PTR(pBillRec, Cbb.P_Pack->Rec);
				memzero(pTrfrRec, sizeof(*pTrfrRec));
				pTrfrRec->BillID = bill_id;
				pTrfrRec->OprNo = ++SurrOprNo;
				pTrfrRec->Dt = Cbb.P_Pack->Rec.Dt;
				pTrfrRec->Flags = ti.Flags;
				pTrfrRec->RByBill = ti.RByBill;
				pTrfrRec->LocID = ti.LocID;
				pTrfrRec->GoodsID = goods_id;
				pTrfrRec->CurID = ti.CurID;
				pTrfrRec->Quantity = ti.Quantity_;
				pTrfrRec->Cost = ti.Cost;
				pTrfrRec->Price = ti.Price;
				pTrfrRec->Discount = ti.Discount;
				pTrfrRec->CurPrice = ti.CurPrice;
				if(soft_restr) {
					PPObjBill * p_bobj = BillObj;
					PPBillExt ext_rec;
					const int article_notvalid = !ArList.CheckID(pBillRec->Object);
					const int goods_notvalid = BIN(Filt.GoodsID && labs(Filt.GoodsID) != goods_id || (State & stUseGoodsList) && !goods_found);
					int    agent_notvalid = 0;
					if(Filt.Flags & OPG_BYZEROAGENT) {
						if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID != 0)
							agent_notvalid = 1;
					}
					else if(Filt.AgentList.GetCount()) {
						if(BCache) {
							if(!BCache->CheckBillForAgent(bill_id))
								agent_notvalid = 1;
						}
						else {
							const PPIDArray & r_agent_list = Filt.AgentList.Get();
							int    f = 0;
							for(uint i = 0; !f && i < r_agent_list.getCount(); i++) {
								const PPID agent_id = r_agent_list.get(i);
								if(p_bobj->FetchExt(bill_id, &ext_rec) > 0 && ext_rec.AgentID == agent_id)
									f = 1;
							}
							if(!f)
								agent_notvalid = 1;
						}
					}
					if(article_notvalid || agent_notvalid || goods_notvalid) {
						pTrfrRec->Cost     = 0.0;
						pTrfrRec->Rest     = 0.0;
						pTrfrRec->CurPrice = 0.0;
						pTrfrRec->WtQtty   = 0.0f;
						pTrfrRec->WtRest   = 0.0f;
						pTrfrRec->Price    = 0.0;
						pTrfrRec->Quantity = 0.0;
						pBillRec->Amount   = 0.0;
						if(article_notvalid)
							pBillRec->Object = Filt.ArList.GetSingle(); // Здесь присваиваем контрагента (возможно группирующего) из фильтра
						if(goods_notvalid)
							pTrfrRec->GoodsID = NZOR(Filt.GoodsID, Filt.GoodsGrpID);
					}
				}
				if(Cbb.P_WrOffPack) {
					if(Filt.Flags & OPG_COMPAREWROFF && pExt) {
						double sum_cost = 0.0;
						double sum_price = 0.0;
						double sum_qtty = 0.0;
						double sum_qtty_abs = 0.0;
						for(uint wop = 0; Cbb.P_WrOffPack->SearchGoods(goods_id, &wop); wop++) {
							const PPTransferItem & r_ti = Cbb.P_WrOffPack->ConstTI(wop);
							const double aq = fabs(r_ti.Quantity_);
							sum_qtty += r_ti.Quantity_;
							sum_qtty_abs += aq;
							sum_cost += fabs(r_ti.Cost) * aq;
							sum_price += fabs(r_ti.Price - r_ti.Discount) * aq;
						}
						pExt->LinkQtty = sum_qtty;
						pExt->LinkCost = fdivnz(sum_cost, sum_qtty_abs);
						pExt->LinkPrice = fdivnz(sum_price, sum_qtty_abs);
					}
				}
				ok = 1;
			}
		}
	}
	else {
		while(ok < 0 && cptrfr_q->nextIteration() > 0) {
			if(AcceptCpTrfrRec(pTrfrRec, pBillRec, pExt) > 0)
				ok = 1;
		}
	}
	return ok;
}

int SLAPI GCTIterator::Iterate(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = -1;
	if(pExt)
		memzero(pExt, sizeof(*pExt));
	if(IterPhase == iterphaseInit) {
		SurrOprNo = 0;
		IterPhase = iterphaseTrfr;
		if(!(State & stOnlyDrafts) && InitQuery(0) > 0) {
			do {
				if(TrfrQuery(pTrfrRec, pBillRec, pExt) > 0)
					ok = 1;
			} while(ok < 0 && NextOuter() > 0);
		}
		if(ok < 0)
			IterPhase = iterphaseCpInit;
	}
	else if(IterPhase == iterphaseTrfr) {
		if(NextTrfr(pTrfrRec, pBillRec, pExt) > 0) {
	 		ok = 1;
		}
		else {
			while(ok < 0 && NextOuter() > 0) {
				if(TrfrQuery(pTrfrRec, pBillRec, pExt) > 0)
					ok = 1;
			}
		}
		if(ok < 0)
			IterPhase = iterphaseCpInit;
	}
	if(State & stThereAreDrafts) {
		if(IterPhase == iterphaseCpInit) {
			IterPhase = iterphaseCpTrfr;
			if(InitQuery(1) > 0) {
				do {
					if(CpTrfrQuery(pTrfrRec, pBillRec, pExt) > 0)
						ok = 1;
				} while(ok < 0 && NextOuter() > 0);
			}
		}
		else if(IterPhase == iterphaseCpTrfr) {
			if(NextCpTrfr(pTrfrRec, pBillRec, pExt) > 0) {
	 			ok = 1;
			}
			else {
				while(ok < 0 && NextOuter() > 0) {
					if(CpTrfrQuery(pTrfrRec, pBillRec, pExt) > 0)
						ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI GCTIterator::First(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec)
{
	IterPhase = iterphaseInit;
	return Iterate(pTrfrRec, pBillRec, 0);
}

int SLAPI GCTIterator::Next(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec)
{
	return Iterate(pTrfrRec, pBillRec, 0);
}

int SLAPI GCTIterator::First(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	IterPhase = iterphaseInit;
	return Iterate(pTrfrRec, pBillRec, pExt);
}

int SLAPI GCTIterator::Next(TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	return Iterate(pTrfrRec, pBillRec, pExt);
}
