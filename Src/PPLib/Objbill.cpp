// OBJBILL.CPP
// Copyright (c) A.Sobolev, A.Starodub 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPObjBill::LockSet::LockSet(PPID id, PPID linkID) : ID(id), LinkID(linkID)
{
}

PPObjBill::EditParam::EditParam() : Flags(0)
{
}

PPObjBill::SelectLotParam::SelectLotParam(PPID goodsID, PPID locID, PPID excludeLotID, long flags) : LocID(locID), ExcludeLotID(excludeLotID), Flags(flags), RetLotID(0)
{
	GoodsList.addnz(goodsID);
	Period.Z();
	// @v10.6.4 MEMSZERO(RetLotRec);
}

PPObjBill::ReckonParam::ReckonParam(int automat, int dontConfirm) : Flags(0), ForceBillID(0), ForceBillDate(ZERODATE)
{
	SETFLAG(Flags, fAutomat, automat);
	SETFLAG(Flags, fDontConfirm, dontConfirm);
	PTR32(ForceBillCode)[0] = 0;
}

/*static*/int FASTCALL PPObjBill::IsPoolOwnedByBill(PPID assocID)
{
	return BIN(oneof2(assocID, PPASS_PAYMBILLPOOL, PPASS_OPBILLPOOL));
}

int PPObjBill::CheckRightsWithOp(PPID opID, long rtflags)
{
	int    ok = 0;
	const  PPRights & r_rt = ObjRts;
	int    op_result = r_rt.CheckOpID(opID, rtflags); //
	if(op_result > 0)
		ok = 1;
	else if(op_result < 0)
		ok = CheckRights(rtflags);
	return ok;
}

static int FASTCALL _Lock(PPID id)
{
	if(id) {
		PPSyncItem sync_item;
		PPID   mutex_id = 0;
		int    r = DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_BILL, id, &mutex_id, &sync_item);
		if(r < 0)
			return PPSetError(PPERR_BILLISLOCKED, sync_item.Name);
		else if(r == 0)
			return 0;
	}
	return 1;
}

static int FASTCALL _Unlock(PPID id)
{
	return id ? DS.GetSync().ReleaseMutex(PPOBJ_BILL, id) : 1;
}

int PPObjBill::Lock(PPID id)
{
	int    ok = 1;
	int    bill_locked = 0;
	int    link_bill_locked = 0;
	LockSet set(id, 0);
	if(id && !locks.bsearch(&set, 0, CMPF_LONG) && !locks.bsearch(&set, 0, CMPF_LONG, sizeof(set.ID), 0)) {
		BillTbl::Rec bill_rec, link_bill_rec;
		THROW(_Lock(id));
		bill_locked = 1;
		if(P_Tbl->Search(id, &bill_rec) > 0) {
			const  PPID link_id = bill_rec.LinkBillID;
			int    do_lock_link = 1;
			if(!locks.bsearch(&link_id, 0, CMPF_LONG)) {
				if(Fetch(link_id, &link_bill_rec) > 0) {
					//
					// При открытии документа списания драфт-документа, если драфт-документ допускает
					// множественное списание, не следует блокировать драфт-документ (это помешает
					// редактированию других документов списания).
					//
					if(IsDraftOp(link_bill_rec.OpID)) {
						PPObjOprKind op_obj;
						PPDraftOpEx doe;
						if(op_obj.GetDraftExData(link_bill_rec.OpID, &doe) && doe.Flags & DROXF_MULTWROFF)
							do_lock_link = 0;
					}
					if(do_lock_link) {
						THROW(_Lock(link_id));
						link_bill_locked = 1;
						set.LinkID = link_id;
					}
				}
			}
		}
		THROW_SL(locks.ordInsert(&set, 0, CMPF_LONG));
	}
	CATCH
		{
			const int save_err_code = PPErrCode;
			if(bill_locked)
				_Unlock(set.ID);
			if(link_bill_locked)
				_Unlock(set.LinkID);
			PPErrCode = save_err_code;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int PPObjBill::Unlock(PPID id)
{
	int     ok = 1;
	uint    p = 0;
	LockSet set(id, 0);
	if(id && locks.bsearch(&set, &p, CMPF_LONG)) {
		_Unlock(locks.at(p).ID);
		_Unlock(locks.at(p).LinkID);
		locks.atFree(p);
	}
	return ok;
}

TLP_IMPL(PPObjBill, BillCore, P_Tbl);
TLP_IMPL(PPObjBill, Transfer, trfr);
TLP_IMPL(PPObjBill, CpTransfCore, P_CpTrfr);
TLP_IMPL(PPObjBill, AdvBillItemTbl, P_AdvBI);
TLP_IMPL(PPObjBill, LotExtCodeCore, P_LotXcT); // @v10.2.9 LotExtCodeTbl-->LotExtCodeCore

PPObjBill::PPObjBill(void * extraPtr) : PPObject(PPOBJ_BILL), CcFlags(CConfig.Flags), P_CpTrfr(0),
	P_AdvBI(0), P_InvT(0), P_GsT(0), P_ScObj(0), /*HistBill(0),*/ P_LotXcT(0), P_Cr(0), ExtraPtr(extraPtr),
	State2(0) /*, DemoRestrict(-1)*/
{
	atobj   = new PPObjAccTurn(0);
	P_OpObj = new PPObjOprKind(0);
	P_PckgT = (CcFlags & CCFLG_USEGOODSPCKG) ? new PackageCore : 0;
	TLP_OPEN(P_Tbl);
	TLP_OPEN(trfr);
	TLP_OPEN(P_CpTrfr);
	if(CcFlags & CCFLG_USEADVBILLITEMS)
		TLP_OPEN(P_AdvBI);
	if(CcFlags & CCFLG_USEHISTBILL) {
		State2 |= stDoObjVer;
	}
	if(CConfig.Flags2 & CCFLG2_USELOTXCODE)
		TLP_OPEN(P_LotXcT);
	ReadConfig(&Cfg);
}

PPObjBill::~PPObjBill()
{
	for(uint i = 0; i < locks.getCount(); i++)
		Unlock(locks.at(i).ID);
	delete atobj;
	delete P_OpObj;
	delete P_PckgT;
	delete P_Cr;
	delete P_InvT;
	delete P_GsT;
	delete P_ScObj;
	TLP_CLOSE(P_Tbl);
	TLP_CLOSE(trfr);
	TLP_CLOSE(P_CpTrfr);
	TLP_CLOSE(P_AdvBI);
	TLP_CLOSE(P_LotXcT);
}

int PPObjBill::Search(PPID id, void * b)
{
	return P_Tbl->Search(id, static_cast<BillTbl::Rec *>(b));
}

int PPObjBill::SearchByGuid(const S_GUID & rUuid, BillTbl::Rec * pRec)
{
	int    ok = -1;
	BillTbl::Rec _rec;
	ObjTagItem tag;
	PPIDArray id_list;
	THROW(tag.SetGuid(PPTAG_BILL_UUID, &rUuid));
	if(PPRef->Ot.SearchObjectsByStr(Obj, PPTAG_BILL_UUID, tag.Val.PStr, &id_list) > 0) {
		LDATE max_date = ZERODATE;
		PPID  _id = 0;
		long  _n = 0;
		//
		// Следующий цикл решает параноидальную проблему существования нескольких документов с одинаковым UUID'ом
		//
		for(uint i = 0; i < id_list.getCount(); i++) {
			BillTbl::Rec temp_rec;
			const PPID temp_id = id_list.get(i);
			if(Search(temp_id, &temp_rec) > 0) {
				if(temp_rec.Dt > max_date || (temp_rec.Dt == max_date && temp_rec.BillNo > _n)) {
					ok = max_date ? 2 : 1; // Найдено более одного документа - код возврата 2 сигнализирует о том.
					_id = temp_id;
					_rec = temp_rec;
					max_date = temp_rec.Dt;
					_n = temp_rec.BillNo;
				}
			}
			else if(!_id) {
				ok = -2; // Сигнализирует о существовании висячей записи тега.
			}
		}
	}
	CATCHZOK
	if(ok > 0) {
		ASSIGN_PTR(pRec, _rec);
	}
	else {
		memzero(pRec, sizeof(*pRec));
	}
	return ok;
}
//
//
//
int PPObjBill::PutGuid(PPID id, const S_GUID * pUuid, int use_ta)
{
	int    ok = 1;
	ObjTagItem tag;
	BillTbl::Rec bill_rec;
	PPObjTag tagobj;
	PPObjectTag tag_rec;
	THROW_PP(tagobj.Fetch(PPTAG_BILL_UUID, &tag_rec) > 0, PPERR_BILLTAGUUIDABS);
	if(pUuid && !pUuid->IsZero()) {
		THROW(Search(id, &bill_rec) > 0);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(tag.SetGuid(PPTAG_BILL_UUID, pUuid));
		THROW(PPRef->Ot.PutTag(PPOBJ_BILL, id, &tag, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::GetGuid(PPID id, S_GUID * pUuid)
{
	ObjTagItem tag;
	CALLPTRMEMB(pUuid, Z());
	return (PPRef->Ot.GetTag(PPOBJ_BILL, id, PPTAG_BILL_UUID, &tag) > 0) ? BIN(tag.GetGuid(pUuid)) : -1;
}

/*static*/SString & FASTCALL PPObjBill::MakeCodeString(const BillTbl::Rec * pRec, int options, SString & rBuf)
{
	char   code[64];
	SString name;
	STRNSCPY(code, pRec->Code);
	rBuf.Z();
	rBuf.Cat(pRec->Dt, DATF_DMY|DATF_CENTURY).CatDiv('-', 1).Cat(BillCore::GetCode(code));
	if(options == 1 || options & mcsAddOpName) {
		GetOpName(pRec->OpID, name.Z());
		rBuf.CatDivIfNotEmpty('-', 1).Cat(name);
	}
	if(options & mcsAddObjName && pRec->Object) {
		GetArticleName(pRec->Object, name.Z());
		rBuf.CatDivIfNotEmpty('-', 1).Cat(name);
	}
	if(options & mcsAddLocName) {
		GetLocationName(pRec->LocID, name.Z());
		rBuf.CatDivIfNotEmpty('-', 1).Cat(name);
	}
	if(options & mcsAddSCard && pRec->SCardID) {
		GetObjectName(PPOBJ_SCARD, pRec->SCardID, name.Z());
		rBuf.CatDivIfNotEmpty('-', 1).Cat(name);
	}
	return rBuf;
}

const char * PPObjBill::GetNamePtr()
{
	return PPObjBill::MakeCodeString(&P_Tbl->data, 1, NameBuf).cptr();
}

int PPObjBill::CheckStatusFlag(PPID statusID, long flag)
{
	if(statusID) {
		PPObjBillStatus bs_obj;
		PPBillStatus bs_rec;
		return BIN(bs_obj.Fetch(statusID, &bs_rec) > 0 && bs_rec.Flags & flag);
	}
	else
		return 0;
}

int FASTCALL PPObjBill::GetEdiUserStatus(const BillTbl::Rec & rRec)
{
	int    status = 0;
	int    recadv_status = PPEDI_RECADV_STATUS_UNDEF;
	int    recadv_conf_status = PPEDI_RECADVCONF_STATUS_UNDEF;
    if(oneof4(rRec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_DESADV)) {
        switch(BillCore::GetRecadvStatus(rRec)) {
			case PPEDI_RECADV_STATUS_ACCEPT: status = BEDIUS_DESADV_IN_RECADV_ACC; break;
			case PPEDI_RECADV_STATUS_PARTACCEPT: status = BEDIUS_DESADV_IN_RECADV_PACC; break;
			case PPEDI_RECADV_STATUS_REJECT: status = BEDIUS_DESADV_IN_RECADV_REJ; break;
			default: status = BEDIUS_DESADV_IN_ACCEPTED; break;
		}
    }
    else {
		if(rRec.StatusID && CheckStatusFlag(rRec.StatusID, BILSTF_READYFOREDIACK) > 0) {
			PPObjTag tag_obj;
			ObjTagItem ediack_tag;
			ObjTagItem ediid_tag;
			if(tag_obj.FetchTag(rRec.ID, PPTAG_BILL_EDIACK, &ediack_tag) > 0) {
				if(tag_obj.FetchTag(rRec.ID, PPTAG_BILL_EDIIDENT, &ediid_tag) > 0) {
					status = BEDIUS_DESADV_OUT_PROCESSED;
					recadv_status = BillCore::GetRecadvStatus(rRec);
					recadv_conf_status = BillCore::GetRecadvConfStatus(rRec);
					if(recadv_status == PPEDI_RECADV_STATUS_ACCEPT) {
						status = BEDIUS_DESADV_OUT_RECADV_ACC;
						if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_ACCEPT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_ACC;
						else if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_REJECT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_REJ;
					}
					else if(recadv_status == PPEDI_RECADV_STATUS_PARTACCEPT) {
						status = BEDIUS_DESADV_OUT_RECADV_PACC;
						if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_ACCEPT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_ACC;
						else if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_REJECT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_REJ;
					}
					else if(recadv_status == PPEDI_RECADV_STATUS_REJECT) {
						status = BEDIUS_DESADV_OUT_RECADV_REJ;
						if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_ACCEPT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_ACC;
						else if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_REJECT)
							status += BEDIUS_DESADV_OUT_RECADV_CONF_REJ;
					}
					else
						status = BEDIUS_DESADV_OUT_PROCESSED;
				}
				else {
					status = BEDIUS_DESADV_OUT_SENDED;
				}
			}
		}
    }
	return status;
}

int PPObjBill::IsPacketEq(const PPBillPacket & rS1, const PPBillPacket & rS2, long flags)
{
	int    eq = 1;
	if(!rS1.PPBill::IsEqual(rS2))
		eq = 0;
	else if(rS1.LnkFiles.getCount()) // Увы, если есть хоть один прикрепленный файл, то придется признать документ изменившимся в любом случае. @todo Решить это проблему.
		eq = 0;
	if(eq) {
		const uint c1 = rS1.GetTCount();
		const uint c2 = rS2.GetTCount();
		if(c1 != c2)
			eq = 0;
		else {
			const int is_intr = IsIntrExpndOp(rS1.Rec.OpID);
			SString n1, n2;
			for(uint i = 0; eq && i < c1; i++) {
				const PPTransferItem & r_ti1 = rS1.ConstTI(i);
				const PPTransferItem & r_ti2 = rS2.ConstTI(i);
				if(!r_ti1.IsEqual(r_ti2))
					eq = 0;
				else if((r_ti1.Flags & PPTFR_RECEIPT) || is_intr || rS1.IsDraft()){
					const ObjTagList * p_t1 = rS1.LTagL.Get(i);
					const ObjTagList * p_t2 = rS2.LTagL.Get(i);
					if(p_t1 != 0 && p_t2 != 0) {
						if(!p_t1->IsEqual(*p_t2))
							eq = 0;
					}
					else if(BIN(p_t1) != BIN(p_t2))
						eq = 0;
				}
				if(eq && is_intr) {
					const ObjTagList * p_t1 = rS1.P_MirrorLTagL ? rS1.P_MirrorLTagL->Get(i) : 0;
					const ObjTagList * p_t2 = rS2.P_MirrorLTagL ? rS2.P_MirrorLTagL->Get(i) : 0;
					if(p_t1 != 0 && p_t2 != 0) {
						if(!p_t1->IsEqual(*p_t2))
							eq = 0;
					}
					else if(BIN(p_t1) != BIN(p_t2))
						eq = 0;
				}
			}
		}
	}
	return eq;
}

int PPObjBill::ValidatePacket(PPBillPacket * pPack, long flags)
{
	int    ok = 1;
	SString temp_buf;
	if(pPack) {
		// CheckOpFlags PPOPKF_NEEDPAYM
		if(!(flags & vpfFreightOnly)) {
			THROW_SL(checkdate(pPack->Rec.Dt));
			THROW_SL(checkdate(pPack->Rec.DueDate, 1));
		}
		if(pPack->Rec.OpID) // Для теневого документа не проверяем период доступа
			THROW(ObjRts.CheckBillDate(pPack->Rec.Dt));
		if(pPack->Rec.StatusID) {
			PPObjBillStatus bs_obj;
			PPBillStatus bs_rec;
			THROW(bs_obj.Fetch(pPack->Rec.StatusID, &bs_rec) > 0);
			if(bs_rec.CheckFields) {
				if(!(flags & vpfFreightOnly)) {
					PPOprKind op_rec;
					if(bs_rec.CheckFields & BILCHECKF_OBJECT && !pPack->Rec.Object) {
						if(GetOpData(pPack->Rec.OpID, &op_rec) > 0) {
							THROW_PP(op_rec.AccSheetID == 0, PPERR_BILLSTCHECKFLD_OBJECT);
						}
					}
					if(bs_rec.CheckFields & BILCHECKF_OBJECT2 && !pPack->Rec.Object2) {
						if(GetOpData(pPack->Rec.OpID, &op_rec) > 0)
							THROW_PP(op_rec.AccSheet2ID == 0, PPERR_BILLSTCHECKFLD_OBJECT2);
					}
                    BillCore::GetCode(temp_buf = pPack->Rec.Code);
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_CODE) || temp_buf.NotEmpty(), PPERR_BILLSTCHECKFLD_CODE);
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_AGENT) || pPack->Ext.AgentID, PPERR_BILLSTCHECKFLD_AGENT);
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_PAYER) || pPack->Ext.PayerID, PPERR_BILLSTCHECKFLD_PAYER);
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_DUEDATE) || checkdate(pPack->Rec.DueDate), PPERR_BILLSTCHECKFLD_DUEDATE);
				}
				if(CheckOpFlags(pPack->Rec.OpID, OPKF_FREIGHT)) {
					const PPFreight * p_fr = pPack->P_Freight;
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_FREIGHT) || p_fr, PPERR_BILLSTCHECKFLD_FREIGHT);
					THROW_PP(!(bs_rec.CheckFields & BILCHECKF_DLVRADDR) || (p_fr && p_fr->DlvrAddrID), PPERR_BILLSTCHECKFLD_DLVRADDR);
					if(p_fr) {
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_PORTOFLOADING) || p_fr->PortOfLoading, PPERR_BILLSTCHECKFLD_PORTOFLD);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_PORTOFDISCHARGE) || p_fr->PortOfDischarge, PPERR_BILLSTCHECKFLD_PORTOFDCHG);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_ISSUEDT) || p_fr->IssueDate, PPERR_BILLSTCHECKFLD_ISSUEDATE);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_ARRIVALDT) || p_fr->ArrivalDate, PPERR_BILLSTCHECKFLD_ARRIVALDATE);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_SHIP) || p_fr->ShipID, PPERR_BILLSTCHECKFLD_SHIP);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_FREIGHTCOST) || p_fr->Cost > 0.0, PPERR_BILLSTCHECKFLD_FREIGHTCOST);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_CAPTAIN) || p_fr->CaptainID, PPERR_BILLSTCHECKFLD_CAPTAIN);
						THROW_PP(!(bs_rec.CheckFields & BILCHECKF_TRBROKER) || p_fr->AgentID, PPERR_BILLSTCHECKFLD_TRBROKER);
					}
				}
			}
			// @v10.2.5 {
			if(bs_rec.Flags & BILSTF_STRICTPRICECONSTRAINS) {
				RealRange restr_bounds;
				for(uint tidx = 0; tidx < pPack->GetTCount(); tidx++) {
					const PPTransferItem & r_ti = pPack->ConstTI(tidx);
					if(GetPriceRestrictions(*pPack, r_ti, tidx, &restr_bounds) > 0) {
						const double validated_price = r_ti.NetPrice();
						if(!restr_bounds.CheckValEps(validated_price, 1E-7)) { // @v10.2.12
							//THROW(restr_bounds.CheckVal(validated_price));
							if(restr_bounds.low > 0.0) {
								SString & r_nam_buf = SLS.AcquireRvlStr();
								temp_buf.Z().Cat(restr_bounds.low, SFMT_MONEY).Space().Cat(GetGoodsName(r_ti.GoodsID, r_nam_buf));
								THROW_PP_S(validated_price >= restr_bounds.low, PPERR_PRICERESTRLOW, temp_buf);
							}
							if(restr_bounds.upp > 0.0) {
								SString & r_nam_buf = SLS.AcquireRvlStr();
								temp_buf.Z().Cat(restr_bounds.upp, SFMT_MONEY).Space().Cat(GetGoodsName(r_ti.GoodsID, r_nam_buf));
								THROW_PP_S(validated_price <= restr_bounds.upp, PPERR_PRICERESTRUPP, temp_buf);
							}
						}
					}
				}
			}
			// } @v10.2.5
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjBill::IsPaymentBill(const BillTbl::Rec & rRec)
{
	int    ok = 0;
	if(IsOpPaymOrRetn(rRec.OpID)) {
		if(!rRec.StatusID || !CheckStatusFlag(rRec.StatusID, BILSTF_LOCK_PAYMENT))
			ok = 1;
	}
	return ok;
}

int PPObjBill::GetCurRate(PPID curID, PPID rateTypeID, PPID relCurID, LDATE * pDt, double * pRate)
{
	SETIFZ(P_Cr, new CurRateCore);
	return P_Cr->GetRate(curID, rateTypeID, relCurID, pDt, pRate);
}

int PPObjBill::GetCurRate(PPID curID, LDATE * pDt, double * pRate)
{
	const PPConfig & r_cfg = LConfig;
	return GetCurRate(curID, r_cfg.BaseRateTypeID, r_cfg.BaseCurID, pDt, pRate);
}

int PPObjBill::GetShipmByOrder(PPID orderID, const DateRange * pRange, PPIDArray * pList)
{
	int    ok = 1;
	BillTbl * p_tbl = P_Tbl;
	BillTbl::Key3 k3;
	BExtQuery q(p_tbl, 3);
	q.select(p_tbl->ID, p_tbl->OpID, p_tbl->Object, p_tbl->LinkBillID, 0L).
		where(p_tbl->Object == orderID && daterange(p_tbl->Dt, pRange) && p_tbl->OpID == 0L);
	k3.Object = orderID;
	k3.Dt   = pRange ? pRange->low : ZERODATE;
	k3.BillNo = 0;
	pList->clear();
	for(q.initIteration(0, &k3, spGt); ok && q.nextIteration() > 0;)
		if(!pList->addUnique(p_tbl->data.LinkBillID))
			ok = 0;
	return ok;
}

int PPObjBill::EnumMembersOfPool(PPID poolType, PPID poolOwnerID, PPID * pMemberID, BillTbl::Rec * pRec)
{
	int    ok = -1, r;
	PPID   bill_id = *pMemberID;
	if((r = P_Tbl->EnumMembersOfPool(poolType, poolOwnerID, &bill_id, 0)) > 0)
		if(!pRec || Search(bill_id, pRec) > 0)
			ok = 1;
		else {
			memzero(pRec, sizeof(*pRec));
			ok = 2;
		}
	else
		ok = r;
	ASSIGN_PTR(pMemberID, bill_id);
	return ok;
}

int PPObjBill::IsMemberOfPool(PPID billID, PPID poolType, PPID * pPullOwnerID)
{
	return P_Tbl->IsMemberOfPool(billID, poolType, pPullOwnerID);
}

int PPObjBill::UpdateOpCounter(PPBillPacket * pPack)
{
	int    ok = -1;
	int    valid_data = 0;
	TDialog * dlg = 0;
	PPOprKind op_rec;
	PPObjOpCounter opc_obj;
	THROW_INVARG(pPack);
	THROW(opc_obj.CheckRights(PPR_MOD));
	THROW(CheckDialogPtrErr(&(dlg = new TDialog(DLG_UPDCNTR))));
	if(GetOpData(pPack->Rec.OpID, &op_rec) > 0) {
		PPID   cntr_id = op_rec.OpCounterID;
		PPOpCounterPacket opc_pack;
		if(opc_obj.GetPacket(cntr_id, &opc_pack) > 0) {
			ushort v = 0;
			long   cntr, fl = opc_pack.Head.Flags;
			opc_pack.GetCounter(pPack->Rec.LocID, &cntr);
			cntr++;
			dlg->setCtrlData(CTL_UPDCNTR_COUNTER, &cntr);
			SETFLAG(v, 0x01, fl & OPCNTF_LOCKINCR);
			dlg->setCtrlData(CTL_UPDCNTR_FLAGS, &v);
			while(!valid_data && ExecView(dlg) == cmOK) {
				dlg->getCtrlData(CTL_UPDCNTR_COUNTER, &cntr);
				dlg->getCtrlData(CTL_UPDCNTR_FLAGS, &(v = 0));
				SETFLAG(fl, OPCNTF_LOCKINCR, v & 0x01);
				if((ok = opc_obj.UpdateCounter(cntr_id, cntr-1, fl, pPack->Rec.LocID, 1)) == 0)
					PPError();
				else {
					if(ok > 0) {
						pPack->Counter = -1L;
						ok = 1;
					}
					else
						ok = -1;
					valid_data = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjBill::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
 	PPID   op_type_id;
	const  PPID preserve_loc_id = LConfig.Location;
	const EditParam * p_extra_param = static_cast<const EditParam *>(extraPtr);
	if(*pID == 0) {
		AddBlock ab;
		ok = AddGoodsBill(pID, &ab); // @todo всегда вызывает ошибку - недопустимый вид операции
	}
	else {
		BillTbl::Rec bill_rec;
		THROW(P_Tbl->Search(*pID, &bill_rec) > 0);
		DS.SetLocation(bill_rec.LocID);
		THROW(op_type_id = GetOpType(bill_rec.OpID));
		if(op_type_id == PPOPT_ACCTURN && !(CheckOpFlags(bill_rec.OpID, OPKF_EXTACCTURN)))
			ok = EditAccTurn(*pID);
		else
			ok = EditGoodsBill(*pID, p_extra_param);
	}
	CATCHZOK
	DS.SetLocation(preserve_loc_id);
	return ok;
}

/*virtual*/int  PPObjBill::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    r = 1;
	PPBillPacket pack;
	if(!(options & PPObject::user_request) || PPMessage(mfConf|mfYesNo, PPCFM_DELETE) == cmYes) {
		if(options & PPObject::user_request)
			PPWait(1);
		if((r = ExtractPacketWithFlags(id, &pack, BPLD_SKIPTRFR|BPLD_LOCK)) != 0) {
			r = (pack.OpTypeID == PPOPT_ACCTURN && !CheckOpFlags(pack.Rec.OpID, OPKF_EXTACCTURN)) ?
				atobj->CheckRights(PPR_DEL) : CheckRights(PPR_DEL);
			if(r)
				r = RemovePacket(pack.Rec.ID, BIN(options & PPObject::use_transaction)) ? 1 : 0;
		}
		if(options & PPObject::user_request) {
			if(r <= 0)
				PPError();
			PPWait(0);
		}
		Unlock(id);
	}
	else
		r = -1;
	return r;
}

int PPObjBill::Browse(void * extraPtr)
{
	return 0;
}

void PPObjBill::DiagGoodsTurnError(const PPBillPacket * pPack)
{
	PPError();
	const  int ln = pPack->ErrLine;
	SString fmt_buf, msg_buf;
	if(pPack->ErrCause == PPBillPacket::err_on_line && pPack->ChkTIdx(ln)) {
		PPFormatS(PPMSG_INFORMATION, PPINF_GOODSLINE, &msg_buf, pPack->ConstTI(ln).GoodsID, pPack->ConstTI(ln).Quantity_);
		PPOutputMessage(msg_buf, mfInfo | mfOK);
	}
	else if(pPack->ErrCause == PPBillPacket::err_on_advline && ln < (int)pPack->AdvList.GetCount()) {
		// PPINF_BILLADVLINE     "Строка расширения документа @int. Вид: @zstr; сумма: @real; счет: @zstr"
		const PPAdvBillItemList::Item & r_item = pPack->AdvList.Get(ln);
		SString advbillkind_buf, acc_buf;
		GetObjectName(PPOBJ_ADVBILLKIND, r_item.AdvBillKindID, advbillkind_buf, 0);
		{
			AcctID acctid;
			Acct   acct;
			acctid.ac = r_item.AccID;
			acctid.ar = r_item.ArID;
			atobj->P_Tbl->ConvertAcctID(acctid, &acct, 0, 0);
			acct.ToStr(ACCF_DEFAULT, acc_buf);
		}
		PPFormatS(PPMSG_INFORMATION, PPINF_BILLADVLINE, &msg_buf, ln+1, advbillkind_buf.cptr(), r_item.Amount, acc_buf.cptr());
		PPOutputMessage(msg_buf, mfInfo|mfOK);
	}
}

int PPObjBill::InsertShipmentItemByOrder(PPBillPacket * pPack, const PPBillPacket * pOrderPack, int orderItemIdx, PPID srcLotID, int interactive)
{
	int    ok = -1;
	LongArray row_idx_list;
	if(pOrderPack->ChkTIdx(orderItemIdx)) {
		DateIter diter;
		int    zero_rest = 1;
		PPID   loc_id = pPack->Rec.LocID;
		double rest;
		double qtty;
		double reserve = 0.0; // Количество, занятое резервирующими заказами
		PPTransferItem ti;
		PPTransferItem * tmp_sti = 0;
		PPTransferItem * p_ord_item = & pOrderPack->TI(orderItemIdx);
		PPID     goods_id = labs(p_ord_item->GoodsID);
		Goods2Tbl::Rec  goods_rec;
		LotArray lot_list;
		THROW(GObj.Fetch(goods_id, &goods_rec) > 0);
		THROW(pPack->RestByOrderLot(p_ord_item->LotID, 0, -1, &qtty));
		if(pPack->CheckGoodsForRestrictions(-1, goods_id, TISIGN_MINUS, qtty, PPBillPacket::cgrfAll, 0)) {
			//
			// Если данная отгрузка осуществляется по резервирующему заказу, то
			// не проверяем наличие других резервирующих заказов на этот товар:
			// действует правило "кто первый встал - того и сапоги".
			//
			ReceiptTbl::Rec ord_lot_rec;
			const int i_am_reserve_order = (trfr->Rcpt.Search(p_ord_item->LotID, &ord_lot_rec) > 0 && ord_lot_rec.Flags & LOTF_ORDRESERVE) ? 1 : 0;
			if(!i_am_reserve_order) {
				//
				// Уменьшаем отгружаемое количество на величину зарезервированного
				// товара (резервирующий заказ не принадлежит данному контрагенту и
				// не является собственно заказом, по которому осуществляется данная отгрузка.
				//
				trfr->Rcpt.GetListOfOpenedLots(-1, -goods_id, loc_id, MAXDATE, &lot_list);
				for(uint i = 0; i < lot_list.getCount(); i++) {
					const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
					if(r_lot_rec.Flags & LOTF_ORDRESERVE && r_lot_rec.SupplID != pPack->Rec.Object && r_lot_rec.BillID != pOrderPack->Rec.ID) {
						trfr->GetRest(r_lot_rec.ID, MAXDATE, &rest);
						reserve += rest;
					}
				}
			}
			lot_list.clear();
			trfr->Rcpt.GetListOfOpenedLots(-1, goods_id, loc_id, pPack->Rec.Dt, &lot_list);
			// @v10.4.12 {
			if(srcLotID) {
				//
				// Если вызывающая функция задала лот, из которого следует расходовать товар, 
				// и этот лот в списке, то перемещаем его вверх списка для того, чтобы применить с приоритетом.
				//
				uint src_lot_pos = 0;
				if(lot_list.lsearch(&srcLotID, &src_lot_pos, CMPF_LONG) && src_lot_pos != 0)
					lot_list.swap(src_lot_pos, 0);
			}
			// } @v10.4.12 
			for(uint lotidx = 0; lotidx < lot_list.getCount() && qtty > 0.0; lotidx++) {
				const ReceiptTbl::Rec & r_lot_rec = lot_list.at(lotidx);
				THROW(pPack->BoundsByLot(r_lot_rec.ID, 0, -1, &rest, 0));
				if(reserve > 0.0) { // Снижаем доступный остаток на величину резерва.
					const double decr = MIN(rest, reserve);
					rest -= decr;
					reserve -= decr;
				}
				rest = MIN(rest, qtty);
				if(rest > 0.0) {
					SString temp_buf;
					const int ord_price_low_prior = BIN(GetConfig().Flags & BCF_ORDPRICELOWPRIORITY);
					const int is_isales_order = BIN(pOrderPack->Rec.EdiOp == PPEDIOP_SALESORDER &&
						pOrderPack->BTagL.GetItemStr(PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("ISALES-PEPSI"));
					const double ord_qtty = fabs(p_ord_item->Quantity_);
					const double ord_price = fabs(p_ord_item->Price) * ord_qtty;
					const double ord_dis   = p_ord_item->Discount * ord_qtty;
					const double ord_pct_dis = (ord_price > 0.0 && ord_dis > 0.0) ? R4(ord_dis / ord_price) : 0.0;
					THROW(ti.Init(&pPack->Rec));
					THROW(ti.SetupGoods(goods_id));
					THROW(ti.SetupLot(r_lot_rec.ID, &r_lot_rec, 0));
					if(p_ord_item->NetPrice() <= 0.0 || (ord_price_low_prior && CheckOpFlags(pOrderPack->Rec.OpID, OPKF_ORDERBYLOC)) ||
						(ord_price_low_prior && LConfig.Flags & CFGFLG_AUTOQUOT)) {
						double quot = 0.0;
						if(SelectQuotKind(pPack, &ti, 0/*strictly noninteractive*/, &quot) > 0) {
							if(is_isales_order && ord_pct_dis > 0.0)
								quot = R5(quot * (1 - ord_pct_dis));
							ti.Discount = ti.Price - quot;
							ti.SetupQuot(quot, 1);
						}
					}
					else if(is_isales_order && ord_pct_dis > 0.0) {
						const double quot = R5(ti.Price * (1 - ord_pct_dis));
						ti.Discount = ti.Price - quot;
						ti.SetupQuot(quot, 1);
					}
					else if(p_ord_item->NetPrice() > 0.0)
						ti.Discount = ti.Price - p_ord_item->NetPrice();
					ti.OrdLotID = p_ord_item->LotID; // @ordlotid
					ti.Flags   |= PPTFR_ONORDER;
					// @v10.5.0 ti.Quantity_ = -rest; // @v10.4.12 @fix rest-->-rest
					ti.Quantity_ = interactive ? rest : -rest; // @v10.5.0
					{
						uint   sh_lot_row_pos = 0;
						//
						// После двух следующих строк индекс sh_lot_row_pos правильно указывает
						// позицию строки теневого документа, которой соответствует наша новая строка
						//
						if(!pPack->SearchShLot(ti.OrdLotID, &sh_lot_row_pos)) // @ordlotid
							THROW(pPack->AddShadowItem(p_ord_item, &sh_lot_row_pos));
						THROW(pPack->InsertRow(&ti, &row_idx_list));
						tmp_sti = &pPack->P_ShLots->at(sh_lot_row_pos);
						THROW(pPack->CalcShadowQuantity(tmp_sti->LotID, &tmp_sti->Quantity_));
					}
					qtty -= rest;
					zero_rest = 0;
					ok = 1;
				}
			}
			if(zero_rest) {
				if(goods_rec.Flags & GF_UNLIM) {
					THROW(ti.Init(&pPack->Rec));
					THROW(ti.SetupGoods(goods_id));
					ti.LotID    = 0;
					ti.Price    = p_ord_item->Price;
					ti.Discount = 0.0;
					ti.OrdLotID = p_ord_item->LotID; // @ordlotid
					ti.Flags   |= PPTFR_ONORDER;
					ti.Quantity_ = p_ord_item->LotID ? qtty : fabs(p_ord_item->Quantity_);
					{
						uint   sh_lot_row_pos = 0;
						if(!pPack->SearchShLot(ti.OrdLotID, &sh_lot_row_pos)) // @ordlotid
							THROW(pPack->AddShadowItem(p_ord_item, &sh_lot_row_pos));
						THROW(pPack->InsertRow(&ti, &row_idx_list));
						if(p_ord_item->LotID) {
							tmp_sti = &pPack->P_ShLots->at(sh_lot_row_pos);
							THROW(pPack->CalcShadowQuantity(tmp_sti->LotID, &tmp_sti->Quantity_));
						}
					}
					ok = 1;
				}
				else if(goods_rec.Flags & GF_AUTOCOMPL) {
					THROW(ti.Init(&pPack->Rec));
					THROW(ti.SetupGoods(goods_id));
					ti.LotID    = 0;
					ti.Price    = p_ord_item->Price;
					ti.Discount = 0.0;
					ti.OrdLotID = p_ord_item->LotID; // @ordlotid
					ti.Flags   |= (PPTFR_ONORDER | PPTFR_AUTOCOMPL);
					ti.Quantity_ = qtty;
					{
						uint   sh_lot_row_pos = 0;
						if(!pPack->SearchShLot(ti.OrdLotID, &sh_lot_row_pos)) { // @ordlotid
							THROW(pPack->AddShadowItem(p_ord_item, &sh_lot_row_pos));
						}
						THROW(pPack->InsertRow(&ti, &row_idx_list, interactive ? PCUG_USERCHOICE : PCUG_CANCEL));
						tmp_sti = &pPack->P_ShLots->at(sh_lot_row_pos);
					}
					THROW(pPack->CalcShadowQuantity(tmp_sti->LotID, &tmp_sti->Quantity_));
					ok = 1;
				}
			}
		}
	}
	CATCH
		pPack->RemoveRows(&row_idx_list);
		ok = 0;
	ENDCATCH
	return ok;
}
//
// Конвертированный чек пригоден только для печати
//
int PPBillPacket::ConvertToCheck(CCheckPacket * pCheckPack) const
{
	int    ok = 1;
	if(oneof2(Rec.OpID, GetCashOp(), GetCashRetOp()) && Rec.Flags & BILLF_CASH) {
		double amount   = 0.0;
		double discount = 0.0;
		pCheckPack->Z();
		if(Rec.Flags & BILLF_CHECK)
			pCheckPack->Rec.Flags |= CCHKF_PRINTED;
		pCheckPack->Rec.UserID = Rec.UserID;
		for(uint i = 0; i < GetTCount(); i++) {
			const PPTransferItem & r_ti = ConstTI(i);
			const double _qtty = r_ti.Quantity_;
			THROW(pCheckPack->InsertItem(r_ti.GoodsID, _qtty, r_ti.NetPrice(), 0));
			amount   += (r_ti.NetPrice() * _qtty);
			discount += (r_ti.Discount   * _qtty);
		}
		LDBLTOMONEY(amount,   pCheckPack->Rec.Amount);
		LDBLTOMONEY(discount, pCheckPack->Rec.Discount);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjBill::PrintCheck__(PPBillPacket * pPack, PPID posNodeID, int addSummator)
{
	int    ok = 1;
	CCheckPacket cp;
	PPCashMachine * p_cm = PPCashMachine::CreateInstance(posNodeID ? posNodeID : (pPack ? pPack->Rec.UserID : LConfig.Cash));
	THROW(p_cm);
	if(pPack)
		THROW(ok = pPack->ConvertToCheck(&cp));
	if(ok > 0) {
		int    r = 0, sync_prn_err = 0;
		if((r = p_cm->SyncPrintCheck(&cp, addSummator)) == 0)
			sync_prn_err = p_cm->SyncGetPrintErrCode();
		if(pPack && pPack->Rec.ID && !(pPack->Rec.Flags & BILLF_CHECK) && (r || sync_prn_err == 1)) {
			pPack->Rec.Flags |= BILLF_CHECK;
			THROW(P_Tbl->Edit(&pPack->Rec.ID, pPack, 1));
		}
		if(r == 0 && sync_prn_err != 3)
			PPError();
	}
	CATCHZOK
	delete p_cm;
	return ok;
}

struct _CcByBillParam {
	_CcByBillParam() : PosNodeID(0), PaymType(0), LocID(0), DivisionN(0), Amount(0.0), Flags(0)
	{
	}
	PPID   PosNodeID;
	int    PaymType;
	PPID   LocID;
    int    DivisionN;
	SString Info;
	double Amount; //@erik v10.5.9
	long   Flags;  //@erik v10.5.9
};

static int _EditCcByBillParam(_CcByBillParam & rParam)
{
	int    ok = -1;

	//@erik v10.5.9 {
	class CCByBill: public TDialog {
	public:
		CCByBill(const _CcByBillParam & rParam): TDialog(DLG_CCBYBILL), R_P(rParam)
		{
			setCtrlReal(CTL_CCBYBILL_CASH, 0.0);
			setCtrlReal(CTL_CCBYBILL_DIFF, -R_P.Amount);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_CCBYBILL_CASH)) {
					double cash = R2(getCtrlReal(CTL_CCBYBILL_CASH));
					setCtrlReal(CTL_CCBYBILL_DIFF, cash - R_P.Amount);
					clearEvent(event);
				}
			}
		}
		const _CcByBillParam & R_P;
	};
	// } @erik 

	CCByBill * dlg = new CCByBill(rParam);
	if(CheckDialogPtrErr(&dlg)) {
		// @v10.0.0 {
		{
			PPObjCashNode::SelFilt f;
			f.LocID = 0;
			f.SyncGroup = 1; // only sync nodes
			SetupPPObjCombo(dlg, CTLSEL_CCBYBILL_POSNODE, PPOBJ_CASHNODE, rParam.PosNodeID, 0, &f);
		}
		// } @v10.0.0
		// @erik v10.5.9 {
		dlg->AddClusterAssocDef(CTL_CCBYBILL_PAYMTYPE, 0, cpmCash);
		dlg->AddClusterAssoc(CTL_CCBYBILL_PAYMTYPE, 1, cpmBank);
		const long __p = CHKXORFLAGS(rParam.Flags, OPKFX_PAYMENT_CASH, OPKFX_PAYMENT_NONCASH);
		if(__p > 0) {
			if(__p & OPKFX_PAYMENT_CASH)
				dlg->SetClusterData(CTL_CCBYBILL_PAYMTYPE, cpmCash);
			else if(__p & OPKFX_PAYMENT_NONCASH)
				dlg->SetClusterData(CTL_CCBYBILL_PAYMTYPE, cpmBank);
			dlg->disableCtrl(CTL_CCBYBILL_PAYMTYPE, 1);
		}
		else {
			dlg->SetClusterData(CTL_CCBYBILL_PAYMTYPE, rParam.PaymType);
		}
		// } @erik v10.5.9
        dlg->setCtrlLong(CTL_CCBYBILL_DIVISION, rParam.DivisionN);
        dlg->setStaticText(CTL_CCBYBILL_ST_INFO, rParam.Info);
        while(ok < 0 && ExecView(dlg) == cmOK) {
			rParam.PosNodeID = dlg->getCtrlLong(CTLSEL_CCBYBILL_POSNODE); // @v10.0.0
			if(!rParam.PosNodeID) {
				PPErrorByDialog(dlg, CTL_CCBYBILL_POSNODE, PPERR_CASHNODENEEDED);
			}
			else {
				rParam.PaymType = dlg->GetClusterData(CTL_CCBYBILL_PAYMTYPE);
				rParam.DivisionN = dlg->getCtrlLong(CTL_CCBYBILL_DIVISION);
				ok = 1;
			}
        }
    }
    else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjBill::PosPrintByBill(PPID billID)
{
	int   ok = -1;
	PPCashMachine * p_cm = 0;
	CCheckCore * p_cc = 0;
	BillTbl::Rec bill_rec;
	if(billID && Search(billID, &bill_rec) > 0) {
		int    _mode = 0; // 1 - check, 2 - correction
		if(GetOpSubType(bill_rec.OpID) == OPSUBT_POSCORRECTION)
			_mode = 2;
		else if(CheckOpPrnFlags(bill_rec.OpID, OPKF_PRT_CHECK))
			_mode = 1;
		else
			PPMessage(mfInfo|mfOK, PPINF_NPRNTCASHCHKBYOPRKIND);
		if(_mode && (!(bill_rec.Flags & BILLF_CHECK) || (PPMaster && PPMessage(mfConf|mfYesNo, PPCFM_BILLCHECKED) == cmYes))) {
			const PPID  __node_id = NZOR(LConfig.DefBillCashID, Cfg.CashNodeID);
			PPOprKindPacket op_pack;  // @erik v10.5.9
			P_OpObj->GetPacket(bill_rec.OpID, &op_pack);  // @erik v10.5.9
			_CcByBillParam param;
			param.PosNodeID = __node_id;
			param.LocID = bill_rec.LocID;
			param.DivisionN = 0;
			param.PaymType = cpmCash;
			param.Amount = bill_rec.Amount;  //@erik v10.5.9
			param.Flags = op_pack.Rec.ExtFlags;
			if(_EditCcByBillParam(param) > 0) {
				int    sync_prn_err = 0;
				PPObjCashNode cn_obj;
				PPBillPacket pack;
				Goods2Tbl::Rec prepay_goods_rec;
				CCheckPacket cp;
				CCheckPacket * p_cp = 0; // Указатель на cp, который будет инициализирован в случае, если чек надо сохранить в кассовой сессии
				const  PPCommConfig & r_ccfg = CConfig;
				const  PPID prepay_goods_id = (r_ccfg.PrepayInvoiceGoodsID && GObj.Fetch(r_ccfg.PrepayInvoiceGoodsID, &prepay_goods_rec) > 0) ? prepay_goods_rec.ID : 0;
				THROW(p_cm = PPCashMachine::CreateInstance(param.PosNodeID));
				THROW(p_cm->SyncAllowPrint());
				THROW(ExtractPacket(billID, &pack) > 0);
				if(_mode == 2) {
					PPCashMachine::FiscalCorrection fc;
					fc.Dt = pack.Rec.Dt;
					BillCore::GetCode(fc.Code = pack.Rec.Code);
					fc.Reason = pack.Rec.Memo;
					fc.AmtCash = pack.Amounts.Get(PPAMT_CS_CASH, 0);
					fc.AmtBank = pack.Amounts.Get(PPAMT_CS_BANK, 0);
					if((fc.AmtBank * fc.AmtCash) >= 0.0) {
						const double _amount = (fc.AmtCash+fc.AmtBank);
						if(_amount != 0.0) {
							const  int is_vat_free = BIN(cn_obj.IsVatFree(param.PosNodeID) > 0);
							if(is_vat_free)
								fc.Flags |= fc.fVatFree;
							else if(prepay_goods_id) {
								PPGoodsTaxEntry gtx;
								if(GObj.GTxObj.Fetch(prepay_goods_rec.TaxGrpID, pack.Rec.Dt, pack.Rec.OpID, &gtx) > 0) {
									fc.VatRate = gtx.GetVatRate();
									if(fc.VatRate == 20.0)
										fc.AmtVat20 = _amount;
									else if(fc.VatRate == 18.0)
										fc.AmtVat18 = _amount;
									else if(fc.VatRate == 10.0)
										fc.AmtVat18 = _amount;
									else if(fc.VatRate == 0.0)
										fc.AmtVat00 = _amount;
									else // @default
										fc.AmtVat20 = _amount;
								}
							}
							ok = p_cm->SyncPrintFiscalCorrection(&fc);
						}
					}
				}
				else if(_mode == 1) {
					double cc_amount = 0.0;
					double dscnt = 0.0;
					// @v10.8.7 @ctr cp.Z();
					// @v10.9.7 {
					cp.Rec.SessID = p_cm->GetCurSessID();
					cp.Rec.CashID = p_cm->GetNodeData().ID;
					{
						long  code = 1;
						CCheckTbl::Rec last_cc_rec;
						if(!p_cc) {
							THROW(p_cc = new CCheckCore);
						}
						if(p_cc->GetLastCheckByCode(p_cm->GetNodeData().ID, &last_cc_rec) > 0)
							cp.Rec.Code = last_cc_rec.Code + 1;
					}
					// } @v10.9.7 
					if(pack.Rec.Memo[0])
						STRNSCPY(cp.Ext.Memo, pack.Rec.Memo);
					PPWait(1);
					if(oneof3(pack.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSRETURN) || pack.IsDraft()) {
						if(CheckOpPrnFlags(pack.Rec.OpID, OPKF_PRT_CHECKTI)) {
							// @v10.9.1 {
							//SString mark_buf;
							SString temp_buf; 
							StringSet ss;
							PPLotExtCodeContainer::MarkSet lotxcode_set;
							GtinStruc gts;
							// } @v10.9.1 
							for(uint i = 0; i < pack.GetTCount(); i++) {
								const PPTransferItem & r_ti = pack.ConstTI(i);
								const double org_qtty = fabs(r_ti.Quantity_);
								double qtty_ = org_qtty;
								const double n_pr = r_ti.NetPrice();
								// @v10.9.1 {
								{
									pack.XcL.Get(i+1, 0, lotxcode_set);
									lotxcode_set.GetByBoxID(0, ss);
									const double _one = 1.0;
									for(uint ssp = 0; qtty_ >= _one && ss.get(&ssp, temp_buf);) {
										const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
										if(pczcr > 0) {
											THROW(cp.InsertItem(r_ti.GoodsID, _one, n_pr, 0.0, param.DivisionN));
											const int cp_idx = static_cast<int>(cp.GetCount());
											cc_amount += R2(n_pr * _one);
											dscnt += R2(r_ti.Discount * _one);
											qtty_ -= _one;
											cp.SetLineTextExt(cp_idx, CCheckPacket::lnextChZnMark, temp_buf);
										}
									}
									if(qtty_ > 0.0) {
										THROW(cp.InsertItem(r_ti.GoodsID, qtty_, n_pr, 0.0, param.DivisionN));
										cc_amount += R2(n_pr * qtty_);
										dscnt += R2(r_ti.Discount * qtty_);
									}
								}
								// } @v10.9.1 
							}
						}
						else if(prepay_goods_id) {
							const double qtty = 1.0;
							const double n_pr = pack.GetAmount();
							THROW(cp.InsertItem(prepay_goods_id, qtty, n_pr, 0.0, param.DivisionN));
							cc_amount += R2(n_pr * qtty);
						}
						if(cp.GetCount()) {
							LDBLTOMONEY(cc_amount, cp.Rec.Amount);
							LDBLTOMONEY(dscnt, cp.Rec.Discount);
							cp._Cash = cc_amount;
							if(oneof3(pack.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSRETURN, PPOPT_DRAFTRECEIPT))
								cp.Rec.Flags |= CCHKF_RETURN;
							if(param.PaymType == cpmBank)
								cp.Rec.Flags |= CCHKF_BANKING;
							ok = p_cm->SyncPrintCheck(&cp, 1);
							p_cp = &cp; // @v10.9.7
						}
					}
					else if(pack.OpTypeID == PPOPT_PAYMENT) {
						PPBillPacket link_pack;
						THROW(ExtractPacket(pack.Rec.LinkBillID, &link_pack) > 0);
						const double amt = link_pack.GetAmount();
						if(amt != 0.0) {
							const double cc_req_amount = pack.GetAmount(); // Сумма платежа - сумма чека должна быть равна этому же значению
							const double mult = cc_req_amount / amt;
							{
								PPTransferItem * ti;
								if(CheckOpPrnFlags(pack.Rec.OpID, OPKF_PRT_CHECKTI)) {
									for(uint i = 0; link_pack.EnumTItems(&i, &ti);) {
										const double qtty = R6(fabs(ti->Quantity_) * mult);
										const double n_pr = ti->NetPrice();
										THROW(cp.InsertItem(ti->GoodsID, qtty, n_pr, 0.0, param.DivisionN));
										cc_amount += R2(n_pr * qtty);
										dscnt += R2(ti->Discount * qtty);
									}
								}
								else if(prepay_goods_id) {
									const double qtty = 1.0;
									const double n_pr = cc_req_amount;
									THROW(cp.InsertItem(prepay_goods_id, qtty, n_pr, 0.0, param.DivisionN));
									cc_amount += R2(n_pr * qtty);
								}
								if(cp.GetCount()) {
									double result_amount = 0.0;
									double result_discount = 0.0;
									cp.CalcAmount(&result_amount, &result_discount);
									LDBLTOMONEY(/*cc_amount*/result_amount, cp.Rec.Amount);
									LDBLTOMONEY(/*dscnt*/result_discount, cp.Rec.Discount);
									if(oneof3(link_pack.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSRETURN, PPOPT_DRAFTRECEIPT)) {
										if(pack.Rec.Amount > 0.0)
											cp.Rec.Flags |= CCHKF_RETURN;
									}
									else {
										if(pack.Rec.Amount < 0.0)
											cp.Rec.Flags |= CCHKF_RETURN;
									}
									if(param.PaymType == cpmBank)
										cp.Rec.Flags |= CCHKF_BANKING;
									if(!feqeps(fabs(result_amount), fabs(cc_req_amount), 1E-8)) {
										double fixup_discount = result_discount + (fabs(result_amount) - fabs(cc_req_amount));
										cp.SetTotalDiscount__(fabs(fixup_discount), (fixup_discount < 0.0) ? CCheckPacket::stdfPlus : 0);
										cp.CalcAmount(&result_amount, &result_discount);
										LDBLTOMONEY(result_amount, cp.Rec.Amount);
										LDBLTOMONEY(result_discount, cp.Rec.Discount);
									}
									cp._Cash = /*cc_amount*/result_amount;
									ok = p_cm->SyncPrintCheck(&cp, 1);
									p_cp = &cp; // @v10.9.7
								}
							}
						}
					}
					else if(pack.OpTypeID == PPOPT_ACCTURN) {
						double bill_amount = pack.GetAmount();
						int    is_ret = 0;
						if(bill_amount < 0.0) {
							is_ret = 1;
							bill_amount = -bill_amount;
						}
						if(prepay_goods_id) {
							double qtty = 1.0;
							double n_pr = bill_amount;
							THROW(cp.InsertItem(prepay_goods_id, qtty, n_pr, 0.0, param.DivisionN));
							cc_amount += R2(n_pr * qtty);
						}
						if(cp.GetCount()) {
							LDBLTOMONEY(cc_amount, cp.Rec.Amount);
							LDBLTOMONEY(dscnt, cp.Rec.Discount);
							cp._Cash = cc_amount;
							if(param.PaymType == cpmBank)
								cp.Rec.Flags |= CCHKF_BANKING;
							if(is_ret)
								cp.Rec.Flags |= CCHKF_RETURN;
							ok = p_cm->SyncPrintCheck(&cp, 1);
							p_cp = &cp; // @v10.9.7
						}
					}
					PPWait(0);
				}
				if(ok == 0)
					sync_prn_err = p_cm->SyncGetPrintErrCode();
				if(ok > 0 || sync_prn_err == 1) {
					pack.Rec.Flags |= BILLF_CHECK;
					if((p_cp || pack.Ext.CcID) && !p_cc) {
						THROW(p_cc = new CCheckCore);
					}
					PPTransaction tra(1);
					THROW(tra);
					// @v10.9.7 {
					if(pack.Ext.CcID) {
						if(p_cc) {
							//if(p_cc->Search(pack.Ext.CcID))
						}
					}
					if(p_cp /* @v10.9.9 {*/ && p_cc && p_cc->GetEqCfg().Flags & PPEquipConfig::fAttachBillChecksToCSess /* } @v10.9.9 */) {
						assert(p_cm);
						if(p_cm) {
							const  PPCashNode & r_cn = p_cm->GetNodeData();
							THROW(p_cc->TurnCheck(p_cp, 0));
							pack.Ext.CcID = p_cp->Rec.ID;
						}
					}
					// } @v10.9.7 
					THROW(P_Tbl->Edit(&pack.Rec.ID, &pack, 0));
					DS.LogAction(PPACN_BILLCCHKPRINTED, Obj, pack.Rec.ID, param.PosNodeID, 0);
					THROW(tra.Commit());
				}
				if(ok == 0 && sync_prn_err != 3)
					PPError();
			}
		}
	}
	CATCHZOKPPERR
	delete p_cm;
	delete p_cc;
	return ok;
}

int PPObjBill::Debug_TrfrError(const PPBillPacket * pPack)
{
	int    ok = 1;
	PPLogger logger;
	if(pPack && CcFlags & CCFLG_DEBUGTRFRERROR) {
		uint   i;
		PPIDArray lot_list, child_list;
		PPIDArray goods_list;
		PPTransferItem * p_item;
		for(i = 0; pPack->EnumTItems(&i, &p_item);) {
			if(p_item->LotID) {
				lot_list.addUnique(p_item->LotID);
				child_list.clear();
				trfr->Rcpt.GatherChilds(p_item->LotID, &child_list, 0, 0);
				lot_list.addUnique(&child_list);
			}
			goods_list.addUnique(labs(p_item->GoodsID));
		}
		for(i = 0; i < lot_list.getCount(); i++) {
			const PPID lot_id = lot_list.get(i);
			PPLotFaultArray lfa(lot_id, logger);
			THROW(trfr->CheckLot(lot_id, 0, 0, lfa));
		}
		for(i = 0; i < goods_list.getCount(); i++) {
			THROW(trfr->CorrectCurRest(goods_list.get(i), 0, &logger, 0));
		}
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjBill::CheckModificationAfterLoading(const PPBillPacket & rPack)
{
	int    ok = 1;
	if(rPack.Rec.ID && !!rPack.LoadMoment) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj) {
			LDATETIME last_ev_dtm = ZERODATETIME;
			int    is_creation = 0;
			SysJournalTbl::Rec sj_rec;
			if(p_sj->GetLastObjModifEvent(PPOBJ_BILL, rPack.Rec.ID, &last_ev_dtm, &is_creation, &sj_rec) > 0) {
				if(cmp(last_ev_dtm, rPack.LoadMoment) > 0) {
					if(!CONFIRM(PPCFM_UPDBILLAFTERFRNMOD))
						ok = 0;
				}
			}
		}
	}
	return ok;
}

int PPObjBill::GetOriginalPacket(PPID billID, SysJournalTbl::Rec * pSjRec, PPBillPacket * pPack)
{
	int    ok = -1;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	SysJournalTbl::Rec ev_cr;
	SysJournalTbl::Rec ev_mod;
	THROW(p_sj);
	if(p_sj->GetObjCreationEvent(Obj, billID, &ev_cr) > 0) {
		ASSIGN_PTR(pSjRec, ev_cr);
		PPIDArray acn_list;
		acn_list.add(PPACN_UPDBILL);
		LDATETIME since;
		since.Set(ev_cr.Dt, ev_cr.Tm);
		if(p_sj->GetNextObjEvent(Obj, billID, &acn_list, since, &ev_mod) > 0) {
			ObjVersioningCore * p_ovc = PPRef->P_OvT;
			if(p_ovc && p_ovc->InitSerializeContext(1)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				long   vv = 0;
				SBuffer ov_buf;
				PPObjID oid;
				ov_buf.Z();
				if(p_ovc->Search(ev_mod.Extra, &oid, &vv, &ov_buf) > 0 && oid.IsEqual(ev_mod.ObjType, ev_mod.ObjID)) {
					PPBillPacket org_pack;
					THROW(SerializePacket__(-1, &org_pack, ov_buf, &r_sctx));
					org_pack.ProcessFlags |= (PPBillPacket::pfZombie|PPBillPacket::pfUpdateProhibited);
					ASSIGN_PTR(pPack, org_pack);
					ok = 2;
				}
			}
		}
		else
			ok = 1; // Документ не менялся
	}
	else
		ok = -2;
	CATCHZOK
	return ok;
}

int PPObjBill::Helper_EditGoodsBill(PPID * pBillID, PPBillPacket * pPack)
{
	int    ok = cmCancel;
	if(pPack && GetOpType(pPack->Rec.OpID) == PPOPT_INVENTORY) {
		ok = EditInventory(pPack, 0);
	}
	else {
		for(int valid_data = 0; !valid_data && (ok = ::EditGoodsBill(pPack, 0)) == cmOK;) {
			PPID   id = pPack->Rec.ID;
			if(CheckModificationAfterLoading(*pPack)) {
				PPWait(1);
				pPack->ProcessFlags |= PPBillPacket::pfViewPercentOnTurn;
				if(!FillTurnList(pPack))
					PPError();
				else if(id ? UpdatePacket(pPack, 1) : TurnPacket(pPack, 1)) {
					PPWait(0);
					if(id == 0) {
						if(pPack->Rec.Flags & BILLF_CASH) {
							if(!PrintCheck__(pPack, 0, 1))
								PPError();
						}
						else if(pPack->Rec.StatusID) {
							if(CheckStatusFlag(pPack->Rec.StatusID, BILSTF_LOCDISPOSE)) {
								LocTransfDisposer disposer;
								PPIDArray bill_list;
								bill_list.add(id);
								if(!disposer.Dispose(bill_list, 0, 1)) {
									PPError();
								}
							}
						}
					}
					Debug_TrfrError(pPack);
					valid_data = 1;
					ASSIGN_PTR(pBillID, pPack->Rec.ID);
					{
						ReckonParam rp(1, 0);
						rp.Flags |= rp.fPopupInfo;
						if(CheckOpFlags(pPack->Rec.OpID, OPKF_RECKON))
							ReckoningPaym(pPack->Rec.ID, rp, 1);
						if(CheckOpFlags(pPack->Rec.OpID, OPKF_NEEDPAYMENT))
							ReckoningDebt(pPack->Rec.ID, rp, 1);
					}
				}
				else
					DiagGoodsTurnError(pPack);
			}
		}
	}
	return ok;
}

int PPObjBill::AddExpendByReceipt(PPID * pBillID, PPID sampleBillID, const SelAddBySampleParam * pParam)
{
	int    ok = 1, res = cmCancel;
	PPID   save_loc = LConfig.Location;
	PPID   op_type = 0;
	PPID   org_acc_sheet_id = 0, new_acc_sheet_id = 0;
	uint   i;
	SString clb;
	PPTransferItem * p_ti;
	PPOprKind    op_rec;
	PPBillPacket pack, sample_pack;
	ASSIGN_PTR(pBillID, 0L);
	THROW_INVARG(pParam);
	THROW(CheckRights(PPR_INS));
	THROW(ExtractPacket(sampleBillID, &sample_pack) > 0);
	THROW_PP(pParam->OpID > 0, PPERR_INVOPRKIND);
	op_type = GetOpType(pParam->OpID, &op_rec);
	THROW_PP(oneof3(op_type, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_DRAFTEXPEND), PPERR_INVOPRKIND); // @v10.9.1 PPOPT_DRAFTEXPEND
	THROW(pack.CreateBlank(pParam->OpID, 0, sample_pack.Rec.LocID, 1));
	pack.Rec.LocID = sample_pack.Rec.LocID;
	GetOpCommonAccSheet(sample_pack.Rec.OpID, &org_acc_sheet_id, 0);
	GetOpCommonAccSheet(pack.Rec.OpID, &new_acc_sheet_id, 0);
	if(new_acc_sheet_id == org_acc_sheet_id)
		pack.Rec.Object = sample_pack.Rec.Object;
	for(i = 0; sample_pack.EnumTItems(&i, &p_ti);) {
		if(p_ti->Flags & PPTFR_PLUS) {
			double down = 0.0;
			PPTransferItem ti(&pack.Rec, 0);
			THROW(ti.SetupGoods(p_ti->GoodsID));
			ti.SetupLot(p_ti->LotID, 0, 0);
			if(ti.Flags & PPTFR_RECEIPT)
				ti.LotID = 0;
			pack.BoundsByLot(p_ti->LotID, &ti, -1, &down, 0);
			if(down > 0) {
				LongArray rows;
				ti.Quantity_ = down;
				THROW(pack.InsertRow(&ti, &rows, 0));
				if(ti.Flags & PPTFR_RECEIPT || IsIntrExpndOp(pack.Rec.OpID)) {
					if(sample_pack.LTagL.GetNumber(PPTAG_LOT_CLB, i-1, clb) > 0)
						pack.LTagL.AddNumber(PPTAG_LOT_CLB, &rows, clb);
					if(sample_pack.LTagL.GetNumber(PPTAG_LOT_SN, i-1, clb) > 0)
						pack.LTagL.AddNumber(PPTAG_LOT_SN, &rows, clb);
				}
				// @v10.8.1 {
				// Если приходная операция превращается в расходную и количество единиц в строке расхода точно равно
				// количеству единиц в приходе, то мы имеем право перенести все марки из прихода в расход
				if(ti.Flags & PPTFR_MINUS && fabs(down) == fabs(p_ti->Quantity_) && rows.getCount() == 1) {
					PPLotExtCodeContainer::MarkSet lotxcode_set;
					sample_pack.XcL.Get(i, 0, lotxcode_set);
					if(lotxcode_set.GetCount())
						pack.XcL.Set_2(rows.at(0)+1, &lotxcode_set);
				}
				// } @v10.8.1
			}
		}
	}
	pack.SampleBillID = sampleBillID;
	res = Helper_EditGoodsBill(pBillID, &pack);
	if(res != cmOK)
		pack.UngetCounter();
	CATCHZOKPPERR
	DS.SetLocation(save_loc);
	return ok ? res : 0;
}

int PPObjBill::AddExpendByOrder(PPID * pBillID, PPID sampleBillID, const SelAddBySampleParam * pParam)
{
	int    ok = 1;
	int    r = 1;
	int    res = cmCancel;
	const  PPID preserve_cfg_loc = LConfig.Location;
	PPID   loc_id = preserve_cfg_loc;
	PPID   op_type = 0;
	SString temp_buf;
	PPOprKind    op_rec;
	PPBillPacket pack, sample_pack;
	ASSIGN_PTR(pBillID, 0L);
	THROW_INVARG(pParam);
	THROW(CheckRights(PPR_INS));
	THROW(ExtractPacket(sampleBillID, &sample_pack) > 0);
	THROW_PP(pParam->OpID > 0, PPERR_INVOPRKIND);
	op_type = GetOpType(pParam->OpID, &op_rec);
	THROW_PP(op_type == PPOPT_GOODSEXPEND, PPERR_INVOPRKIND);
	if(pParam->LocID)
		loc_id = pParam->LocID;
	else if(pParam->Flags & pParam->fNonInteractive) {
		loc_id = sample_pack.Rec.LocID;
	}
	else {
		while(r > 0 && !LConfig.Location) {
			THROW(r = PPObjLocation::SelectWarehouse());
		}
		if(r > 0)
			loc_id = LConfig.Location;
	}
	if(loc_id) {
		PPBillPacket rcpt_bpack;
		PPBillPacket * p_rcpt_bpack = 0;
		LAssocArray pos_to_src_lot_list; // Список ассоциаций номеров строк исходного документа с номерами строк p_rcpt_bpack, 
			// для определения лотов, из которых необходимо расходовать товары.
			// Используется при установленном флаге SelAddBySampleParam::fRcptAllOnShipm
		PPBillPacket::SetupObjectBlock sob;
		if(pParam->Flags & SelAddBySampleParam::fCopyBillCode) {
			THROW(pack.CreateBlank_WithoutCode(pParam->OpID, 0, loc_id, 1));
			STRNSCPY(pack.Rec.Code, sample_pack.Rec.Code);
		}
		else {
			THROW(pack.CreateBlank(pParam->OpID, 0, loc_id, 1));
		}
		if(checkdate(pParam->Dt)) {
			LDATE   new_bill_dt = pParam->Dt;
			new_bill_dt.getactual(sample_pack.Rec.Dt);
			pack.Rec.Dt = (new_bill_dt > sample_pack.Rec.Dt) ? new_bill_dt : sample_pack.Rec.Dt;
		}
		THROW(pack.SetupObject(sample_pack.Rec.Object, sob));
		pack.SampleBillID = sampleBillID;
		if(pack.Rec.SCardID == 0 && sample_pack.Rec.SCardID > 0)
			pack.Rec.SCardID = sample_pack.Rec.SCardID;
		if(sample_pack.P_Freight) {
			THROW(pack.SetFreight(sample_pack.P_Freight));
		}
		if(sample_pack.Ext.AgentID)
			pack.Ext.AgentID = sample_pack.Ext.AgentID;
		STRNSCPY(pack.Rec.Memo, sample_pack.Rec.Memo);
		if(sample_pack.Rec.EdiOp == PPEDIOP_SALESORDER && sample_pack.BTagL.GetItemStr(PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("STYLOAGENT")) {
			PPStyloPalmConfig sp_cfg;
			PPObjStyloPalm::ReadConfig(&sp_cfg);
			if(sp_cfg.InhBillTagID) {
				const ObjTagItem * p_tag_item = sample_pack.BTagL.GetItem(sp_cfg.InhBillTagID);
				long   tag_val = 0;
				if(p_tag_item && p_tag_item->GetInt(&tag_val) && tag_val) {
					ObjTagItem tag;
					if(tag.SetInt(sp_cfg.InhBillTagID, tag_val))
						pack.BTagL.PutItem(sp_cfg.InhBillTagID, &tag);
				}
			}
		}
		if(pParam->QuotKindID)
			pack.QuotKindID = pParam->QuotKindID;
		if(pParam->Flags & SelAddBySampleParam::fRcptAllOnShipm) {
			Goods2Tbl::Rec goods_rec;
			int is_there_limited_goods = 0;
			for(uint i = 0; !is_there_limited_goods && i < sample_pack.GetTCount(); i++) {
				if(GObj.Fetch(sample_pack.ConstTI(i).GoodsID, &goods_rec) > 0 && !(goods_rec.Flags & GF_UNLIM))
					is_there_limited_goods = 1;
			}
			if(is_there_limited_goods) {
				PPID   rcpt_op_id = GetReceiptOp();
				PPOprKind rcpt_op_rec;
				PPID   rcpt_ar_id = 0;
				PPBillPacket::SetupObjectBlock rcpt_sob;
				THROW_PP(rcpt_op_id, PPERR_UNDEFRECEIPTOP);
				THROW(rcpt_bpack.CreateBlank(rcpt_op_id, 0, loc_id, 1));
				GetOpData(rcpt_op_id, &rcpt_op_rec);
				rcpt_bpack.Rec.Dt = pack.Rec.Dt;
				if(sample_pack.Rec.Object) {
					ArticleTbl::Rec ar_rec;
					PPID   temp_ar_id = 0;
					if(ArObj.Fetch(sample_pack.Rec.Object, &ar_rec) > 0) {
						if(ar_rec.AccSheetID == rcpt_op_rec.AccSheetID)
							rcpt_ar_id = sample_pack.Rec.Object;
						else if(ArObj.GetByPerson(rcpt_op_rec.AccSheetID, ObjectToPerson(sample_pack.Rec.Object, 0), &temp_ar_id) > 0)
							rcpt_ar_id = temp_ar_id;
					}
				}
				THROW(rcpt_bpack.SetupObject(rcpt_ar_id, rcpt_sob));
				{
					SString memo_buf;
					PPObjBill::MakeCodeString(&sample_pack.Rec, PPObjBill::mcsAddObjName, temp_buf);
					(memo_buf = "@autoreceipt").Space().Cat(temp_buf);
					STRNSCPY(rcpt_bpack.Rec.Memo, memo_buf);
				}
				for(uint i = 0; i < sample_pack.GetTCount(); i++) {
					const PPTransferItem & r_src_ti = sample_pack.ConstTI(i);
					if(GObj.Fetch(r_src_ti.GoodsID, &goods_rec) > 0 && !(goods_rec.Flags & GF_UNLIM)) {
						PPTransferItem ti;
						LongArray row_idx_list;
						ti.Init(&rcpt_bpack.Rec, 0, 0);
						THROW(ti.SetupGoods(labs(r_src_ti.GoodsID)));
						ti.RByBill = r_src_ti.RByBill;
						ti.Quantity_ = fabs(r_src_ti.Quantity_);
						ti.Cost = (r_src_ti.Cost > 0.0) ? r_src_ti.Cost : r_src_ti.Price;
						ti.Price = r_src_ti.Price;
						ti.Expiry = r_src_ti.Expiry;
						THROW(rcpt_bpack.InsertRow(&ti, &row_idx_list));
						{
							ObjTagList rcpt_row_tag_list;
							assert(row_idx_list.getCount() == 1);
							const uint ti_pos = row_idx_list.get(0);
							if(sample_pack.LTagL.GetTagStr(i, PPTAG_LOT_SN, temp_buf) > 0)
								rcpt_row_tag_list.PutItemStr(PPTAG_LOT_SN, temp_buf);
							if(sample_pack.LTagL.GetTagStr(i, PPTAG_LOT_CLB, temp_buf) > 0)
								rcpt_row_tag_list.PutItemStr(PPTAG_LOT_CLB, temp_buf);
							rcpt_bpack.LTagL.Set(ti_pos, &rcpt_row_tag_list);
							pos_to_src_lot_list.Add(static_cast<long>(i), ti_pos);
						}
					}
				}
				if(rcpt_bpack.GetTCount()) {
					THROW(rcpt_bpack.InitAmounts());
					THROW(FillTurnList(&rcpt_bpack));
					THROW(TurnPacket(&rcpt_bpack, 1)); 
					p_rcpt_bpack = &rcpt_bpack;
					/*for(uint j = 0; j < rcpt_bpack.GetTCount(); j++) {
						const PPTransferItem & r_ti = rcpt_bpack.ConstTI(j);
						uint  pos = 0;
						long  src_ti_pos = 0;
						if(pos_to_src_lot_list.SearchByVal(j, &src_ti_pos, &pos)) {
						}
					}*/
				}
			}
		}
		{
			// @v10.4.12 {
			if(pParam->Flags & (pParam->fNonInteractive|pParam->fRcptAllOnShipm)) { // @v10.5.0
				for(uint i = 0; i < sample_pack.GetTCount(); i++) {
					PPID   src_lot_id = 0;
					if(p_rcpt_bpack) {
						uint  pos = 0;
						long  rcpt_ti_pos = 0;
						if(pos_to_src_lot_list.Search(i, &rcpt_ti_pos, &pos))
							src_lot_id = p_rcpt_bpack->ConstTI(rcpt_ti_pos).LotID;
					}
					THROW(InsertShipmentItemByOrder(&pack, &sample_pack, i, src_lot_id, 0 /*noninteractive*/));
				}
			}
			// @v10.4.12 {
			if(pParam->Flags & pParam->fNonInteractive) {
#if 0  // @v10.4.12 {
				for(uint i = 0; i < sample_pack.GetTCount(); i++) {
					THROW(InsertShipmentItemByOrder(&pack, &sample_pack, i, 0/*srcLotID*/, 0 /*noninteractive*/));
				}
#endif // } 0 @v10.4.12
				// @v10.4.12 @fix (странно что никто не пожаловался - документ не проводился) {
				THROW(pack.InitAmounts());
				THROW(FillTurnList(&pack));
				THROW(TurnPacket(&pack, 1)); 
				// } @v10.4.12
			}
			else {
				res = Helper_EditGoodsBill(pBillID, &pack);
			}
		}
	}
	if(res != cmOK)
		pack.UngetCounter();
	CATCH
		ok = (pParam->Flags & pParam->fNonInteractive) ? 0 : PPErrorZ();
	ENDCATCH
	DS.SetLocation(preserve_cfg_loc);
	return ok ? res : 0;
}

int PPObjBill::AddDraftBySample(PPID * pBillID, PPID sampleBillID, const SelAddBySampleParam * pParam)
{
	int    ok = 1;
	int    r = 1;
	int    res = cmCancel;
	const  PPID preserve_loc = LConfig.Location;
	PPID   loc_id = preserve_loc;
	PPID   op_type = 0;
	SString temp_buf;
	PPOprKind    op_rec;
	PPBillPacket pack;
	PPBillPacket sample_pack;
	ASSIGN_PTR(pBillID, 0L);
	THROW_INVARG(pParam);
	THROW(CheckRights(PPR_INS));
	THROW(ExtractPacket(sampleBillID, &sample_pack) > 0);
	THROW_PP(pParam->OpID > 0, PPERR_INVOPRKIND);
	op_type = GetOpType(pParam->OpID, &op_rec);
	THROW_PP(oneof3(op_type, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT), PPERR_INVOPRKIND);
	if(pParam->LocID)
		loc_id = pParam->LocID;
	else if(pParam->Flags & pParam->fNonInteractive) {
		loc_id = sample_pack.Rec.LocID;
	}
	else {
		while(r > 0 && !LConfig.Location) {
			THROW(r = PPObjLocation::SelectWarehouse());
		}
		if(r > 0)
			loc_id = LConfig.Location;
	}
	if(loc_id) {
		const int is_src_draft = IsDraftOp(sample_pack.Rec.OpID);
		PPBillPacket::SetupObjectBlock sob;
		if(pParam->Flags & SelAddBySampleParam::fCopyBillCode) {
			THROW(pack.CreateBlank_WithoutCode(pParam->OpID, 0, loc_id, 1));
			STRNSCPY(pack.Rec.Code, sample_pack.Rec.Code);
		}
		else {
			THROW(pack.CreateBlank(pParam->OpID, 0, loc_id, 1));
		}
		if(checkdate(pParam->Dt)) {
			LDATE new_bill_dt = pParam->Dt.getactual(sample_pack.Rec.Dt);
			pack.Rec.Dt = (new_bill_dt > sample_pack.Rec.Dt) ? new_bill_dt : sample_pack.Rec.Dt;
		}
		THROW(pack.SetupObject(sample_pack.Rec.Object, sob));
		pack.SampleBillID = sampleBillID;
		if(pack.Rec.SCardID == 0 && sample_pack.Rec.SCardID > 0)
			pack.Rec.SCardID = sample_pack.Rec.SCardID;
		if(sample_pack.P_Freight && op_type == PPOPT_DRAFTEXPEND)
			THROW(pack.SetFreight(sample_pack.P_Freight));
		if(sample_pack.Ext.AgentID && op_type == PPOPT_DRAFTEXPEND)
			pack.Ext.AgentID = sample_pack.Ext.AgentID;
		STRNSCPY(pack.Rec.Memo, sample_pack.Rec.Memo);
		{
			pack.Rec.LinkBillID = sample_pack.Rec.ID; // Сохраняем привязку драфт-документа к документу заказа.
				// По этой привязке при списании драфт-документа мы учтем исполнение заказа.
			LongArray row_idx_list;
			PPLotExtCodeContainer::MarkSet lotxcode_set;
			PPTransferItem * p_ti = 0;
			for(uint i = 0; sample_pack.EnumTItems(&i, &p_ti);) {
				double qtty = 0.0;
				double price = 0.0;
				ReceiptTbl::Rec lot_rec;
				PPTransferItem new_ti(&pack.Rec, TISIGN_UNDEF);
				THROW(new_ti.SetupGoods(labs(p_ti->GoodsID)));
				if(!is_src_draft && p_ti->LotID && (op_type == PPOPT_DRAFTRECEIPT || (op_type == PPOPT_DRAFTEXPEND && pParam->Action == pParam->acnDraftExpRestByOrder)))
					trfr->GetRest(p_ti->LotID, pack.Rec.Dt, &qtty);
				else
					qtty = p_ti->Quantity_;
				if(qtty > 0.0) {
					new_ti.Quantity_ = qtty;
					new_ti.Price = p_ti->NetPrice();
					if(GetCurGoodsPrice(labs(p_ti->GoodsID), pack.Rec.LocID, GPRET_INDEF, &price, &lot_rec) > 0) {
						if(new_ti.Price == 0.0)
							new_ti.Price = price;
						new_ti.Cost = lot_rec.Cost;
					}
					new_ti.SetupSign(pack.Rec.OpID); // @v10.0.08
					// @v11.0.2 {
					{
						row_idx_list.Z();
						THROW(pack.InsertRow(&new_ti, &row_idx_list));
						if(is_src_draft && row_idx_list.getCount() == 1) {
							const uint ti_pos = row_idx_list.get(0);
							ObjTagList row_tag_list;
							const PPID _tag_id_list[] = { PPTAG_LOT_SN, PPTAG_LOT_CLB, PPTAG_LOT_FSRARINFA, PPTAG_LOT_FSRARINFB, PPTAG_LOT_FSRARLOTGOODSCODE };
							for(uint tagidx = 0; tagidx < SIZEOFARRAY(_tag_id_list); tagidx++) {
								const PPID row_tag_id = _tag_id_list[tagidx];
								if(sample_pack.LTagL.GetTagStr(i-1, row_tag_id, temp_buf) > 0) // @v11.0.3 @fix i-->(i-1)
									row_tag_list.PutItemStr(row_tag_id, temp_buf);
							}
							pack.LTagL.Set(ti_pos, &row_tag_list); 
							// Marks:
							sample_pack.XcL.Get(i, 0, lotxcode_set);
							if(lotxcode_set.GetCount())
								pack.XcL.Set_2(ti_pos+1, &lotxcode_set);
						}
					}
					// } @v11.0.2 
				}
			}
		}
		if(pParam->Flags & pParam->fNonInteractive) {
			if(pack.GetTCount()) {
				int    local_result = __TurnPacket(&pack, 0, 1, 1);
				THROW(local_result);
				if(local_result > 0) {
					ASSIGN_PTR(pBillID, pack.Rec.ID);
					res = cmOK;
				}
				else
					res = cmCancel;
			}
			else
				res = cmCancel;
		}
		else
			res = Helper_EditGoodsBill(pBillID, &pack);
	}
	if(res != cmOK)
		pack.UngetCounter();
	CATCH
		ok = (pParam->Flags & pParam->fNonInteractive) ? 0 : PPErrorZ();
	ENDCATCH
	DS.SetLocation(preserve_loc);
	return ok ? res : 0;
}

int PPObjBill::AddGoodsBillByFilt(PPID * pBillID, const BillFilt * pFilt, PPID opID, PPID sCardID, const CCheckTbl::Rec * pChkRec)
{
	int    ok = 1, r = cmCancel;
	PPID   op_type = 0L;
	PPOprKind op_rec;
	PPBillPacket pack;
	ASSIGN_PTR(pBillID, 0L);
	THROW(CheckRights(PPR_INS));
	THROW(pack.CreateBlankByFilt(opID, pFilt, 1));
	pack.Rec.Dt = getcurdate_(); //@v10.9.4 @SevaSob
	op_type = GetOpType(pack.Rec.OpID, &op_rec);
	while(r > 0 && !pack.Rec.LocID) {
		if(op_type == PPOPT_ACCTURN && op_rec.DefLocID)
			pack.Rec.LocID = op_rec.DefLocID;
		else {
			THROW(r = PPObjLocation::SelectWarehouse());
			if(r > 0)
				pack.Rec.LocID = op_rec.DefLocID;
		}
	}
	if(op_type == PPOPT_GOODSMODIF || (op_type == PPOPT_GOODSRECEIPT && op_rec.AccSheetID == 0)) {
		//
		// Так как чаще всего при модификации товаров в
		// образующихся лотах поставщиком выступает главная //
		// организация, проверим наличие соответствующей
		// статьи и, если отсутствует - создадим.
		//
		PPID   moas = 0;
		THROW(ArObj.GetMainOrgAsSuppl(&moas, 1, 1));
	}
	if(sCardID && oneof5(op_type, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER, PPOPT_PAYMENT, PPOPT_ACCTURN)) {
		int    use_total_dis = 1;
		PPObjSCard sc_obj;
		SCardTbl::Rec sc_rec;
		if(op_type == PPOPT_DRAFTEXPEND && pChkRec && pChkRec->ID) {
			// @v10.4.2 CCheckCore cc_core;
			CCheckLineTbl::Rec cc_line;
			// @v10.6.4 MEMSZERO(cc_line);
			pack.Rec.Dt = pChkRec->Dt;
			for(int i = 0; sc_obj.P_CcTbl->EnumLines(pChkRec->ID, &i, &cc_line) > 0;) {
				ReceiptTbl::Rec lot_rec;
				PPTransferItem ti;
				THROW(ti.Init(&pack.Rec));
				THROW(ti.SetupGoods(cc_line.GoodsID));
				ti.SetupLot(0, 0, 0);
				ti.Quantity_ = cc_line.Quantity;
				if(trfr->Rcpt.GetLastLot(ti.GoodsID, 0L, pack.Rec.Dt, &lot_rec) > 0) {
					ti.Cost  = R5(lot_rec.Cost);
					ti.QCert = lot_rec.QCertID;
					ti.UnitPerPack = lot_rec.UnitPerPack;
				}
				ti.Price = TR5(intmnytodbl(cc_line.Price) - cc_line.Dscnt);
				THROW(pack.InsertRow(&ti, 0));
			}
			use_total_dis = 0;
		}
		if(sc_obj.Search(sCardID, &sc_rec) > 0) {
			if(sc_rec.PDis && use_total_dis) {
				pack.SetTotalDiscount(fdiv100i(sc_rec.PDis), 1, 0);
				pack.Rec.Flags |= BILLF_TOTALDISCOUNT;
			}
			pack.Rec.SCardID = sCardID;
		}
	}
	r = Helper_EditGoodsBill(pBillID, &pack);
	if(r != cmOK)
		pack.UngetCounter();
	CATCHZOKPPERR
	return ok ? r : 0;
}

PPObjBill::AddBlock::AddBlock(const AddBlock * pBlk)
{
	if(pBlk) {
		SampleBillID = pBlk->SampleBillID;
		OpID = pBlk->OpID;
		RegisterID = pBlk->RegisterID;
		LinkBillID = pBlk->LinkBillID;
		ObjectID = pBlk->ObjectID;
		Object2ID = pBlk->Object2ID;
		Pk = pBlk->Pk;
		PoolID = pBlk->PoolID;
		LocID = pBlk->LocID;
		FirstItemLotID = pBlk->FirstItemLotID;
		FirstItemSign  = pBlk->FirstItemSign;
		FirstItemQtty = pBlk->FirstItemQtty;
	}
	else {
		THISZERO();
	}
}

int PPObjBill::AddRetBillByLot(PPID lotID)
{
	int    ok = -1;
	PPID   save_loc_id = LConfig.Location;
	ReceiptTbl::Rec lot_rec;
	if(lotID && trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
		BillTbl::Rec bill_rec;
		PPID   loc_id = lot_rec.LocID;
		while(lot_rec.PrevLotID) {
			THROW(trfr->Rcpt.Search(lot_rec.PrevLotID, &lot_rec) > 0);
		}
		if(Search(lot_rec.BillID, &bill_rec) > 0) {
			if(GetOpType(bill_rec.OpID) == PPOPT_GOODSRECEIPT) {
				DS.SetLocation(loc_id);
				if(AddRetBill(0, bill_rec.ID, loc_id) == cmOK)
					ok = 1;
			}
			else
				PPMessage(mfInfo|mfOK, PPINF_RETONINTRLOT);
		}
	}
	CATCHZOKPPERR
	DS.SetLocation(save_loc_id);
	return ok;
}

int PPObjBill::AddRetBill(PPID op, long link, PPID locID)
{
	int    r = 1, ok = 1, res = cmCancel;
	PPID   bill_id = 0;
	BillTbl::Rec link_rec;
	PPBillPacket pack;
	THROW_PP(link, PPERR_UNDEFLINKBILLFORRET);
	THROW(CheckRights(PPR_INS));
	while(r > 0 && !DS.CheckStateFlag(CFGST_WAREHOUSE)) {
		THROW(r = PPObjLocation::SelectWarehouse());
	}
	if(r > 0) {
		PPID   loc_id = 0;
		THROW(P_Tbl->Search(link, &link_rec) > 0);
		loc_id = NZOR(locID, link_rec.LocID);
		if(op == 0) {
			PPIDArray op_type_list;
			op_type_list.addzlist(PPOPT_GOODSRETURN, PPOPT_CORRECTION, 0);
			const int r = BillPrelude(&op_type_list, OPKLF_FIXEDLOC, link_rec.OpID, &op, &loc_id);
			if(r <= 0)
				op = 0;
			//THROW(op = SelectOprKind(0, link_rec.OpID, (PPID)PPOPT_GOODSRETURN, (PPID)PPOPT_CORRECTION, 0L));
		}
		if(op > 0) {
			THROW(pack.CreateBlank(op, link, loc_id, 1));
			if(loc_id)
				pack.Rec.LocID = loc_id;
			res = Helper_EditGoodsBill(&bill_id, &pack);
			if(res != cmOK)
				pack.UngetCounter();
		}
	}
	CATCHZOKPPERR
	return ok ? res : 0;
}

int PPObjBill::AddGoodsBill(PPID * pBillID, const AddBlock * pBlk)
{
	const PPConfig & r_cfg = LConfig;
	const  PPID preserve_loc = r_cfg.Location;

	int    ok = 1;
	int    r = 1, res = cmCancel;
	PPID   op_type = 0;
	PPOprKind    op_rec;
	PPBillPacket pack;
	const AddBlock blk(pBlk);
	ASSIGN_PTR(pBillID, 0L);
	THROW(CheckRights(PPR_INS));
	THROW(Lock(blk.LinkBillID));
	if(blk.SampleBillID) {
		THROW(pack.CreateBlankBySample(blk.SampleBillID, 1));
		if(blk.PoolID) {
			pack.SetPoolMembership(blk.Pk, blk.PoolID);
		}
		res = Helper_EditGoodsBill(pBillID, &pack);
		if(res != cmOK)
			pack.UngetCounter();
	}
	else {
		PPID   loc_id = NZOR(blk.LocID, r_cfg.Location);
		THROW_PP(blk.OpID > 0, PPERR_INVOPRKIND);
		op_type = GetOpType(blk.OpID, &op_rec);
		while(r > 0 && !loc_id) {
			if(op_type == PPOPT_ACCTURN && op_rec.DefLocID) {
				loc_id = op_rec.DefLocID;
			}
			else {
				THROW(r = PPObjLocation::SelectWarehouse());
				if(r > 0)
					loc_id = r_cfg.Location;
			}
		}
		if(r > 0) {
			DS.SetLocation(loc_id);
			if(op_type == PPOPT_GOODSMODIF || (op_type == PPOPT_GOODSRECEIPT && op_rec.AccSheetID == 0)) {
				//
				// Так как чаще всего при модификации товаров в
				// образующихся лотах поставщиком выступает главная //
				// организация, проверим наличие соответствующей
				// статьи и, если отсутствует - создадим.
				//
				PPID moas = 0;
				THROW(ArObj.GetMainOrgAsSuppl(&moas, 1, 1));
			}
			THROW(pack.CreateBlank(blk.OpID, blk.LinkBillID, 0, 1));
			if(blk.PoolID) {
				pack.SetPoolMembership(blk.Pk, blk.PoolID);
			}
			{
				ArticleTbl::Rec ar_rec;
				PPID   alt_ar_id = 0;
				if(pack.Rec.Object == 0 && blk.ObjectID != 0) {
					if(op_rec.AccSheetID && ArObj.Fetch(blk.ObjectID, &ar_rec) > 0) {
						if(ar_rec.AccSheetID == op_rec.AccSheetID)
							pack.Rec.Object = blk.ObjectID;
						else if(GetAlternateArticle(blk.ObjectID, op_rec.AccSheetID, &alt_ar_id) > 0)
							pack.Rec.Object = alt_ar_id;
					}
				}
				if(pack.Rec.Object2 == 0 && blk.Object2ID != 0) {
					if(op_rec.AccSheet2ID && ArObj.Fetch(blk.Object2ID, &ar_rec) > 0) {
						if(ar_rec.AccSheetID == op_rec.AccSheet2ID)
							pack.Rec.Object2 = blk.Object2ID;
						else if(GetAlternateArticle(blk.Object2ID, op_rec.AccSheet2ID, &alt_ar_id) > 0)
							pack.Rec.Object2 = alt_ar_id;
					}
				}
			}
			if(blk.FirstItemLotID) {
				ReceiptTbl::Rec lot_rec;
				if(trfr->Rcpt.Search(blk.FirstItemLotID, &lot_rec) > 0) {
					PPTransferItem ti(&pack.Rec, blk.FirstItemSign);
					THROW(ti.SetupGoods(lot_rec.GoodsID, 0));
					THROW(ti.SetupLot(blk.FirstItemLotID, 0, 0));
					if(blk.FirstItemQtty != 0.0) {
						ti.Quantity_ = blk.FirstItemQtty;
					}
					THROW(pack.InsertRow(&ti, 0));
				}
			}
			res = Helper_EditGoodsBill(pBillID, &pack);
			if(res != cmOK)
				pack.UngetCounter();
		}
	}
	CATCHZOKPPERR
	Unlock(blk.LinkBillID);
	DS.SetLocation(preserve_loc);
	return ok ? res : 0;
}

int PPObjBill::EditGoodsBill(PPID id, const EditParam * pExtraParam)
{
	int    ok = cmYes;
	long   egbf = efEdit;
	uint   flags = BPLD_LOCK;
	const  PPID save_loc_id = LConfig.Location;
	PPBillPacket pack;
	if(pExtraParam)
		egbf |= pExtraParam->Flags;
	// @v10.1.8 SETFLAG(flags, BPLD_FORCESERIALS, (Cfg.Flags & BCF_SHOWSERIALSINGBLINES));
	flags |= BPLD_FORCESERIALS; // @v10.1.8
	THROW(ExtractPacketWithFlags(id, &pack, flags));
	DS.SetLocation(pack.Rec.LocID);
	if(GetOpType(pack.Rec.OpID) == PPOPT_INVENTORY) {
		THROW(CheckRights(PPR_MOD));
		ok = EditInventory(&pack, 0);
	}
	else {
		while((ok = ::EditGoodsBill(&pack, egbf)) == cmOK) {
			THROW(CheckRights(PPR_MOD));
			if(CheckModificationAfterLoading(pack)) {
				PPWait(1);
				if(!FillTurnList(&pack)) {
					DiagGoodsTurnError(&pack);
					egbf |= efForceModify;
				}
				else if(!UpdatePacket(&pack, 1)) {
					DiagGoodsTurnError(&pack);
					egbf |= efForceModify;
				}
				else {
					Debug_TrfrError(&pack);
					double amt_paym = 0.0;
					PPWait(0);
					ReckonParam rp(1, 0);
					rp.Flags |= rp.fPopupInfo;
					if(CheckOpFlags(pack.Rec.OpID, OPKF_RECKON)) {
						amt_paym = pack.GetAmount() - pack.Amounts.Get(PPAMT_PAYMENT, pack.Rec.CurID);
						if(amt_paym != 0) // @v10.3.4 (amt_paym > 0)-->(amt_paym != 0) Теперь возможен инвертированный зачет
							ReckoningPaym(pack.Rec.ID, rp, 1);
					}
					if(CheckOpFlags(pack.Rec.OpID, OPKF_NEEDPAYMENT)) {
						amt_paym = pack.GetAmount() - pack.Amounts.Get(PPAMT_PAYMENT, pack.Rec.CurID);
						if(amt_paym > 0)
							ReckoningDebt(pack.Rec.ID, rp, 1);
					}
					break;
				}
			}
		}
	}
	CATCHZOKPPERR
	Unlock(id);
	DS.SetLocation(save_loc_id);
	return ok;
}

int PPObjBill::GetAccturn(const AccTurnTbl::Rec * pATRec, PPAccTurn * pAturn, int useCache)
{
	int    ok = 0;
	BillTbl::Rec bill_rec;
	if(atobj->P_Tbl->ConvertRec(pATRec, pAturn, useCache) && P_Tbl->Search(pAturn->BillID, &bill_rec) > 0) {
		pAturn->Opr = bill_rec.OpID;
		if(pAturn->CurID)
			P_Tbl->GetAmount(pAturn->BillID, PPAMT_CRATE, pAturn->CurID, &pAturn->CRate);
		else
			pAturn->CRate = 0.0;
		memcpy(pAturn->BillCode, bill_rec.Code, sizeof(pAturn->BillCode));
		ok = 1;
	}
	return ok;
}

int PPObjBill::ViewAccturns(PPID billID)
{
	int    ok = -1;
	if(billID) {
		AccturnFilt flt;
		flt.Flags |= (AccturnFilt::fLastOnly | AccturnFilt::fAllCurrencies);
		flt.BillID = billID;
		THROW(PPView::Execute(PPVIEW_ACCTURN, &flt, 0, 0));
	}
	CATCHZOKPPERR
	return ok;
}

static void FASTCALL _processFlags(TDialog * dlg, long flags)
{
	static const struct { uint f, c; } _tab[] = {
		{(uint)ATDF_DSBLDOC,    CTL_ATURN_DOC},
		{(uint)ATDF_DSBLDATE,   CTL_ATURN_DATE},
		{(uint)ATDF_DSBLDACC,   CTL_ATURN_DACC},
		{(uint)ATDF_DSBLDART,   CTL_ATURN_DART},
		{(uint)ATDF_DSBLCACC,   CTL_ATURN_CACC},
		{(uint)ATDF_DSBLCART,   CTL_ATURN_CART},
		{(uint)ATDF_DSBLAMOUNT, CTL_ATURN_AMOUNT}
	};
	int    sel = -1;
	for(int i = 0; i < SIZEOFARRAY(_tab); i++) {
		if(flags & _tab[i].f)
			dlg->disableCtrl(_tab[i].c, 1);
		else if(sel == -1)
			sel = _tab[i].c;
	}
	if(sel == -1)
		sel = STDCTL_CANCELBUTTON;
	dlg->selectCtrl(sel);
}

int PPObjBill::EditGenericAccTurn(PPBillPacket * pPack, long flags)
{
	int    ok = 1, r = 0, valid_data = 0;
	PPAccTurn at;
	AccTurnDialog * dlg = 0;
	uint   dlg_id = 0;
	if(pPack->Turns.getCount())
		at = pPack->Turns.at(0);
	else
		pPack->CreateAccTurn(&at);
	if(GetOpSubType(pPack->Rec.OpID) == OPSUBT_REGISTER)
		dlg_id = DLG_REGATURN;
	else if(at.Flags & PPAF_OUTBAL)
		dlg_id = DLG_OUTBALATURN;
	else
		dlg_id = DLG_ATURN;
	THROW(CheckDialogPtr(&(dlg = new AccTurnDialog(dlg_id, this))));
	dlg->setDTS(&at, pPack);
	_processFlags(dlg, flags);
	for(r = cmCancel; !valid_data && (r = ExecView(dlg)) == cmOK;)
		if(dlg->getDTS(&at))
			valid_data = 1;
	CATCHZOK
	delete dlg;
	return ok ? r : 0;
}

long FASTCALL ATTF_TO_ATDF(long attf)
{
	long   f = 0;
	SETFLAG(f, ATDF_DSBLDACC, attf & ATTF_DACCFIX);
	SETFLAG(f, ATDF_DSBLDART, attf & ATTF_DARTFIX);
	SETFLAG(f, ATDF_DSBLCACC, attf & ATTF_CACCFIX);
	SETFLAG(f, ATDF_DSBLCART, attf & ATTF_CARTFIX);
	return f;
}

int PPObjBill::AddAccturnBySample(PPID * pBillID, PPID sampleBillID)
{
	int    ok = 1, r;
	long   flags = 0;
	PPBillPacket pack, sample_pack;
	THROW(ExtractPacketWithFlags(sampleBillID, &sample_pack, BPLD_SKIPTRFR) > 0);
	if(CheckOpFlags(sample_pack.Rec.OpID, OPKF_EXTACCTURN)) {
		AddBlock ab;
		ab.SampleBillID = sampleBillID;
		THROW(r = AddGoodsBill(pBillID, &ab));
	}
	else {
		THROW(atobj->CheckRights(PPR_INS));
		THROW(pack.CreateBlankBySample(sampleBillID, 1));
		pack.UngetCounter();
		THROW(atobj->CreateBlankAccTurnBySample(&pack, &sample_pack, &flags));
		do {
			THROW(r = EditGenericAccTurn(&pack, flags));
			if(r == cmOK)
				if(!TurnPacket(&pack, 1))
					r = PPErrorZ();
				else
					ASSIGN_PTR(pBillID, pack.Rec.ID);
		} while(r == 0);
		if(r != cmOK)
			pack.UngetCounter();
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjBill::AddGenAccturn(PPID * pBillID, PPID opID, PPID registerID)
{
	int    ok = 1, r;
	long   flags;
	PPBillPacket pack;
	THROW(atobj->CheckRights(PPR_INS));
	THROW(atobj->CreateBlankAccTurn(opID, &pack, &flags, 1));
	if(pack.Turns.getCount()) {
		PPAccTurn & r_at = pack.Turns.at(0);
		SETIFZ(r_at.DbtID.ac, registerID);
	}
	do {
		THROW(r = EditGenericAccTurn(&pack, flags));
		if(r == cmOK)
			if(!TurnPacket(&pack, 1))
				r = PPErrorZ();
			else
				ASSIGN_PTR(pBillID, pack.Rec.ID);
	} while(r == 0);
	if(r != cmOK)
		pack.UngetCounter();
	CATCHZOK
	return ok ? r : 0;
}

int PPObjBill::AddAccturn(PPID * pBillID, const AddBlock * pBlk)
{
	int    ok = 1, r = 1;
	const  PPID save_loc_id = LConfig.Location;
	PPID   loc_id = save_loc_id;
	PPBillPacket pack;
	AddBlock blk(pBlk);
	if(blk.OpID == 0) {
		PPIDArray op_list;
		uint   opfl = 0;
		if(blk.RegisterID) {
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(PPOPT_ACCTURN, &op_id, &op_rec) > 0;) {
				if(op_rec.SubType == OPSUBT_REGISTER)
					op_list.add(op_id);
			}
			THROW_PP(op_list.getCount(), PPERR_NONEREGISTEROPS);
			blk.OpID = op_list.getSingle();
			opfl = OPKLF_OPLIST;
		}
		else
			op_list.add(PPOPT_ACCTURN);
		if(blk.OpID == 0)
			if(BillPrelude(&op_list, opfl, 0, &blk.OpID, &loc_id) > 0)
				DS.SetLocation(loc_id);
			else
				blk.OpID = 0;
	}
	if(blk.OpID > 0) {
		r = (CheckOpFlags(blk.OpID, OPKF_EXTACCTURN) && !blk.RegisterID) ?
			AddGoodsBill(pBillID, &blk) : AddGenAccturn(pBillID, blk.OpID, blk.RegisterID);
	}
	THROW(r);
	CATCHZOKPPERR
	DS.SetLocation(save_loc_id);
	return ok ? r : 0;
}

int PPObjBill::EditAccTurn(PPID id)
{
	int    ok = cmCancel, r;
	long   flags = 0;
	PPAccTurnTemplArray att_list;
	PPID   org_loc_id = 0;
	SString org_mem;
	AmtList org_amt_list;
	PPAccTurn    at;
	PPBillPacket pack;
	THROW(CheckRights(PPR_MOD));
	THROW(atobj->CheckRights(PPR_MOD));
	THROW(ExtractPacketWithFlags(id, &pack, BPLD_LOCK));
	org_amt_list.copy(pack.Amounts);
	org_loc_id = pack.Rec.LocID;
	org_mem = pack.Rec.Memo;
	if(pack.Turns.getCount())
		at = pack.Turns.at(0);
	THROW(PPObjOprKind::GetATTemplList(pack.Rec.OpID, &att_list));
	if(att_list.getCount())
		flags = ATTF_TO_ATDF(att_list.at(0).Flags);
	flags |= (ATDF_DSBLDACC | ATDF_DSBLDART | ATDF_DSBLCACC | ATDF_DSBLCART);
	THROW(r = EditGenericAccTurn(&pack, flags));
	if(r == cmOK && (memcmp(&pack.Turns.at(0), &at, sizeof(at)) || !pack.Amounts.IsEqual(&org_amt_list) || pack.Rec.LocID != org_loc_id ||
		org_mem.Cmp(pack.Rec.Memo, 0) != 0)) {
		THROW(UpdatePacket(&pack, 1));
	}
	else
		r = cmCancel;
	CATCHZOKPPERR
	Unlock(id);
	return ok ? r : 0;
}

int PPObjBill::ReplyGoodsDel(PPID id)
{
	{
		ReceiptTbl::Key2 k;
		MEMSZERO(k);
		k.GoodsID = id;
		if(trfr->Rcpt.search(2, &k, spGe) && trfr->Rcpt.data.GoodsID == id)
			return RetRefsExistsErr(PPOBJ_LOT, trfr->Rcpt.data.ID);
	}
	{
		TransferTbl::Key3 trk;
		MEMSZERO(trk);
		trk.GoodsID = id;
		if(trfr->search(3, &trk, spGe) && trfr->data.GoodsID == id)
			return RetRefsExistsErr(Obj, trfr->data.BillID);
	}
	if(P_CpTrfr) {
		CpTransfTbl::Rec cpt_rec;
		if(P_CpTrfr->SearchGoodsRef(id, &cpt_rec) > 0)
			return RetRefsExistsErr(Obj, cpt_rec.BillID);
	}
	return (id > 0) ? ReplyGoodsDel(-id) : DBRPL_OK; // @recursion
}

int PPObjBill::ReplyGoodsReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	ReceiptTbl::Key2 k;
	MEMSZERO(k);
	k.GoodsID = dest;
	while(trfr->Rcpt.search(2, &k, spGt)) {
		ReceiptTbl::Rec * p_lotrec = & trfr->Rcpt.data;
		if(p_lotrec->GoodsID == dest) {
			TrUCL_Param ucl_param;
			MEMSZERO(ucl_param);
			ucl_param.GoodsID = src;
			THROW(trfr->UpdateCascadeLot(p_lotrec->ID, 0, &ucl_param, TRUCLF_UPDGOODS, 0));
		}
		else
			break;
	}
	if(P_CpTrfr) {
		THROW(P_CpTrfr->ReplaceGoods(dest, src, 0));
	}
	if(dest > 0) {
		THROW(ReplyGoodsReplace(-dest, -src)); // @recursion
	}
	else {
		THROW(ReplyInventGoodsReplace(labs(dest), labs(src)));
	}
	CATCHZOK
	return ok;
}

int PPObjBill::ReplyInventGoodsReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	InventoryCore & r_inv_tbl = GetInvT();
	InventoryTbl::Key2 k;
	MEMSZERO(k);
	k.GoodsID = dest;
	while(r_inv_tbl.searchForUpdate(2, &k, spGt) && k.GoodsID == dest) {
		InventoryTbl::Rec inv_rec = r_inv_tbl.data;
		if(inv_rec.GoodsID == dest) {
			InventoryTbl::Key2 k_d;
			THROW_DB(r_inv_tbl.deleteRec()); // @sfu
			k_d.GoodsID = src;
			k_d.BillID = k.BillID;
			STRNSCPY(k_d.Serial, k.Serial);
			if(r_inv_tbl.searchForUpdate(2, &k_d, spEq) > 0) {
				r_inv_tbl.data.Quantity  += inv_rec.Quantity;
				r_inv_tbl.data.StockRest += inv_rec.StockRest;
				INVENT_SETDIFFSIGN(r_inv_tbl.data.Flags, r_inv_tbl.data.StockRest);
				THROW_DB(r_inv_tbl.updateRec()); // @sfu
			}
			else {
				inv_rec.GoodsID = src;
				THROW_DB(r_inv_tbl.insertRecBuf(&inv_rec));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::ReplyArticleReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	BillCore * p_tbl = P_Tbl;
	uint   i;
	PPBillExt ext_rec;
	BillTbl::Key3    bk3;
	ReceiptTbl::Key5 rk5;
	PPIDArray bill_list;
	MEMSZERO(bk3);
	bk3.Object = dest;
	// @todo update_for
	while(p_tbl->search(3, &bk3, spGt) && bk3.Object == dest) {
		THROW_DB(p_tbl->rereadForUpdate(3, &bk3));
		p_tbl->data.Object = src;
		THROW_DB(p_tbl->updateRec()); // @sfu
	}
	MEMSZERO(rk5);
	rk5.SupplID = dest;
	// @todo update_for
	while(trfr->Rcpt.search(5, &rk5, spGt) && rk5.SupplID == dest) {
		THROW_DB(trfr->Rcpt.rereadForUpdate(5, &rk5));
		trfr->Rcpt.data.SupplID = src;
		THROW_DB(trfr->Rcpt.updateRec()); // @sfu
	}
	// Agent
	THROW(p_tbl->GetBillListByExt(dest, 0, bill_list));
	for(i = 0; i < bill_list.getCount(); i++)
		if(p_tbl->GetExtraData(bill_list.at(i), &ext_rec) > 0) {
			ext_rec.AgentID = src;
			THROW(p_tbl->PutExtraData(bill_list.at(i), &ext_rec, 0));
		}
	// Payer
	bill_list.clear();
	THROW(p_tbl->GetBillListByExt(0, dest, bill_list));
	for(i = 0; i < bill_list.getCount(); i++)
		if(p_tbl->GetExtraData(bill_list.at(i), &ext_rec) > 0) {
			ext_rec.PayerID = src;
			THROW(p_tbl->PutExtraData(bill_list.at(i), &ext_rec, 0));
		}
	CATCHZOK
	return ok;
}

int PPObjBill::ReplyArticleDel(PPID id)
{
	BillCore * p_tbl = P_Tbl;
	for(DateIter diter; p_tbl->EnumByObj(id, &diter) > 0;)
		if(p_tbl->data.OpID) // В теневом док-те Object имеет спец. назначение
			return RetRefsExistsErr(Obj, p_tbl->data.ID);
	{
		PPID   k = 0;
		BExtQuery q(p_tbl, 0);
		q.select(p_tbl->ID, 0L).where(p_tbl->Object2 == id);
		if(q.fetchFirst(&k, spFirst) > 0)
			return RetRefsExistsErr(Obj, p_tbl->data.ID);
	}
	return DBRPL_OK;
}

int PPObjBill::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	PPID   k = 0;
	PPID   bill_id = 0;
	BillCore * p_tbl = P_Tbl;
	if(msg == DBMSG_OBJDELETE) {
		switch(_obj) {
			case PPOBJ_GOODS:
				ok = ReplyGoodsDel(_id);
				break;
			case PPOBJ_GOODSTAX:
				{
					BExtQuery q(&trfr->Rcpt, 0);
					q.select(trfr->Rcpt.ID, 0L).where(trfr->Rcpt.InTaxGrpID == _id);
					if(q.fetchFirst(&(k = 0), spGt) > 0)
						ok = RetRefsExistsErr(PPOBJ_LOT, trfr->Rcpt.data.ID);
				}
				break;
			case PPOBJ_OPRKIND:
				if(p_tbl->EnumByOpr(_id, 0) > 0)
					ok = RetRefsExistsErr(Obj, p_tbl->data.ID);
				break;
			case PPOBJ_LOCATION:
				{
					LocationTbl::Rec loc_rec;
					if(LocObj.Search(reinterpret_cast<long>(extraPtr), &loc_rec) > 0) {
						if(loc_rec.Type == LOCTYP_WAREHOUSE) {
							BillTbl::Key5 k5;
							MEMSZERO(k5);
							k5.LocID = _id;
							if(p_tbl->search(5, &k5, spGe) && k5.LocID == _id)
								ok = RetRefsExistsErr(Obj, p_tbl->data.ID);
						}
						else if(loc_rec.Type == LOCTYP_ADDRESS) {
							LAssocArray dlvr_addr_list;
							p_tbl->GetDlvrAddrList(&dlvr_addr_list);
							for(uint i = 0; i < dlvr_addr_list.getCount(); i++)
								if(dlvr_addr_list.at(i).Val == _id) {
									THROW(RetRefsExistsErr(Obj, dlvr_addr_list.at(i).Key));
								}
						}
					}
				}
				break;
			case PPOBJ_WORLD:
				{
					UintHashTable list;
					FreightFilt ff;
					ff.Flags |= FreightFilt::fStrictPort;
					ff.PortID = _id;
					if(p_tbl->GetListByFreightFilt(ff, list) > 0) {
						ulong first_id = 0;
						list.Enum(&first_id);
						THROW(RetRefsExistsErr(Obj, (long)first_id));
					}
				}
				{
					UintHashTable list;
					FreightFilt ff;
					ff.Flags |= FreightFilt::fStrictPort;
					ff.PortOfLoading = _id;
					if(p_tbl->GetListByFreightFilt(ff, list) > 0) {
						ulong first_id = 0;
						list.Enum(&first_id);
						THROW(RetRefsExistsErr(Obj, (long)first_id));
					}
				}
				break;
			case PPOBJ_ARTICLE:
				ok = ReplyArticleDel(_id);
				break;
			case PPOBJ_QCERT:
				{
					ReceiptTbl::Key6 k6;
					MEMSZERO(k6);
					k6.QCertID = _id;
					if(trfr->Rcpt.search(6, &k6, spGt) && trfr->Rcpt.data.QCertID == _id)
						ok = RetRefsExistsErr(PPOBJ_LOT, trfr->Rcpt.data.ID);
				}
				break;
			case PPOBJ_AMOUNTTYPE:
				if(p_tbl->CheckAmtTypeRef(_id, &bill_id) > 0)
					ok = RetRefsExistsErr(Obj, bill_id);
				break;
			case PPOBJ_BILLSTATUS:
				{
					BillTbl::Key0 k0;
					BExtQuery q(p_tbl, 0, 4);
					q.select(p_tbl->ID, 0L).where(p_tbl->StatusID == _id);
					MEMSZERO(k0);
					if(q.fetchFirst(&k0, spFirst) > 0)
						ok = RetRefsExistsErr(Obj, p_tbl->data.ID);
				}
				break;
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_GOODS)
			ok = ReplyGoodsReplace(_id, reinterpret_cast<long>(extraPtr));
		else if(_obj == PPOBJ_ARTICLE)
			ok = ReplyArticleReplace(_id, reinterpret_cast<long>(extraPtr));
		else if(_obj == PPOBJ_GOODSTAX) {
			// @todo update_for
			for(k = 0; trfr->Rcpt.search(0, &k, spGt);) {
				if(trfr->Rcpt.data.InTaxGrpID == _id) {
					THROW_DB(trfr->Rcpt.rereadForUpdate(0, &k));
					trfr->Rcpt.data.InTaxGrpID = reinterpret_cast<long>(extraPtr);
					THROW_DB(trfr->Rcpt.updateRec()); // @sfu
				}
			}
		}
		else if(_obj == PPOBJ_LOCATION) {
			LAssocArray dlvr_addr_list;
			LocationTbl::Rec replacement_rec;
			THROW(LocObj.Search(reinterpret_cast<long>(extraPtr), &replacement_rec) > 0);
			THROW_PP(replacement_rec.Type == LOCTYP_ADDRESS, PPERR_REPLACEMENTID_NOTADDR);
			p_tbl->GetDlvrAddrList(&dlvr_addr_list);
			for(uint i = 0; i < dlvr_addr_list.getCount(); i++) {
				if(dlvr_addr_list.at(i).Val == _id) {
					const PPID bill_id = dlvr_addr_list.at(i).Key;
					PPFreight freight;
					if(p_tbl->GetFreight(bill_id, &freight) > 0 && freight.DlvrAddrID == _id) {
						freight.DlvrAddrID = reinterpret_cast<long>(extraPtr);
						THROW(p_tbl->SetFreight(bill_id, &freight, 0));
					}
				}
			}
		}
		else if(_obj == PPOBJ_WORLD) {
			{
				UintHashTable list;
				FreightFilt ff;
				ff.Flags |= FreightFilt::fStrictPort;
				ff.PortID = _id;
				if(p_tbl->GetListByFreightFilt(ff, list) > 0) {
					for(ulong bill_id = 0; list.Enum(&bill_id);) {
						PPFreight freight;
						if(p_tbl->GetFreight((long)bill_id, &freight) > 0 && freight.PortOfDischarge == _id) {
							freight.PortOfDischarge = reinterpret_cast<long>(extraPtr);
							THROW(p_tbl->SetFreight((long)bill_id, &freight, 0));
						}
					}
				}
			}
			{
				UintHashTable list;
				FreightFilt ff;
				ff.Flags |= FreightFilt::fStrictPort;
				ff.PortOfLoading = _id;
				if(p_tbl->GetListByFreightFilt(ff, list) > 0) {
					for(ulong bill_id = 0; list.Enum(&bill_id);) {
						PPFreight freight;
						if(p_tbl->GetFreight((long)bill_id, &freight) > 0 && freight.PortOfLoading == _id) {
							freight.PortOfLoading = reinterpret_cast<long>(extraPtr);
							THROW(p_tbl->SetFreight((long)bill_id, &freight, 0));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	class BillRightsDlg : public TDialog {
	public:
		BillRightsDlg() : TDialog(DLG_RTBILL)
		{
		}
		virtual int TransmitData(int dir, void * pData)
		{
			int    s = 1;
			if(dir > 0)
				setDTS((ObjRights*)pData);
			else if(dir < 0)
				getDTS((ObjRights*)pData);
			else
				s = TDialog::TransmitData(dir, pData);
			return s;
		}
		int setDTS(const ObjRights * pData)
		{
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 0, BILLRT_CASH);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 1, BILLRT_CLOSECASH);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 2, BILLRT_OPENCASH);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 3, BILLRT_SYSINFO);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 4, BILLRT_MODDATE);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 5, BILLRT_MODGOODS);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 6, BILLRT_USEWLABEL);
			AddClusterAssoc(CTL_RTBILL_SFLAGS, 7, BILLRT_ACCSCOST);

			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  0, BILLOPRT_MODOBJ);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  1, BILLOPRT_MODFREIGHT);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  2, BILLOPRT_MULTUPD);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  3, BILLOPRT_UNITEBILLS);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  4, BILLOPRT_MODSTATUS);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  5, BILLOPRT_CANCELQUOT);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  6, BILLOPRT_TOTALDSCNT);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  7, BILLOPRT_MODTRANSM);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  8, BILLOPRT_ACCSSUPPL);
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS,  9, BILLOPRT_REJECT);    // @v9.0.1
			AddClusterAssoc(CTL_RTBILL_OPRFLAGS, 10, BILLOPRT_EMPTY);    // @v9.3.1

			ushort comm_rt = pData ? pData->Flags : 0;
			setCtrlData(CTL_RTBILL_FLAGS, &comm_rt);
			SetClusterData(CTL_RTBILL_SFLAGS, pData ? pData->Flags : 0);
			SetClusterData(CTL_RTBILL_OPRFLAGS, pData ? pData->OprFlags : 0);
			return 1;
		}
		int getDTS(ObjRights * pData)
		{
			if(pData) {
				ushort comm_rt = 0;
				getCtrlData(CTL_RTBILL_FLAGS, &comm_rt);
				pData->Flags = (ushort)GetClusterData(CTL_RTBILL_SFLAGS);
				pData->OprFlags = (ushort)GetClusterData(CTL_RTBILL_OPRFLAGS);
				pData->Flags &= ~0x00ff;
				pData->Flags |= (comm_rt & 0x00ff);
				pData->Size  = sizeof(ObjRights);
			}
			return 1;
		}
	};
	int    r = 1;
	BillRightsDlg * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new BillRightsDlg())));
	if(pDlg)
		pDlg->Embed(dlg);
	else {
		THROW_PP(bufSize >= sizeof(ObjRights), PPERR_OBJRTBUFSIZ);
		dlg->setDTS(rt);
		if((r = ExecView(dlg)) == cmOK)
			dlg->getDTS(rt);
		else
			r = -1;
	}
	CATCH
		r = 0;
	ENDCATCH
	if(!pDlg)
		delete dlg;
	return r;
}

int PPObjBill::SetWLabel(PPID id, int mode)
{
	int    ok = -1;
	if(id && CheckRights(BILLRT_USEWLABEL)) {
		if(!(Cfg.Flags & BCF_CONFIRMWL) || CONFIRM(PPCFM_SETWL)) {
			ok = Lock(id) ? P_Tbl->SetWLabel(id, mode, 1) : 0;
			Unlock(id);
			if(ok == 0)
				PPError();
		}
	}
	return ok;
}

int PPObjBill::SetStatus(PPID id, PPID statusID, int use_ta)
{
	int    ok = -1, is_locked = 0;
	LocTransfDisposer * p_disposer = 0;
	if(id) {
		int    set_noaturn_flag = -1; // 1 - установить флаг, 0 - снять флаг, -1 - оставить как есть.
		int    set_nopaym_flag  = -1; // 1 - установить флаг, 0 - снять флаг, -1 - оставить как есть.
		int    set_counter = -1;      // 1 - обновить номер документа в соответствии со счетчиком статуса
		BillTbl::Rec rec;
		PPObjBillStatus bs_obj;
		PPBillStatus bs_rec, prev_bs_rec;
		PPOprKind op_rec;
		PPObjOpCounter opc_obj;
		PPLogger logger;
		THROW(CheckRights(BILLOPRT_MODSTATUS, 1));
		THROW(bs_obj.Fetch(statusID, &bs_rec) > 0);
		if(bs_rec.Flags & BILSTF_LOCDISPOSE) {
			THROW_MEM(p_disposer = new LocTransfDisposer);
		}
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(Lock(id));
			is_locked = 1;
			THROW(Search(id, &rec) > 0);
			if(rec.StatusID != statusID) {
				PPID   prev_status_id = rec.StatusID;
				const  BillTbl::Rec org_rec = rec;
				if(bs_rec.RestrictOpID)
					THROW_PP_S(IsOpBelongTo(rec.OpID, bs_rec.RestrictOpID), PPERR_BILLSTATUSRESTRICTOP, bs_rec.Name);
				if(bs_rec.CounterID) {
					THROW(GetOpData(rec.OpID, &op_rec) > 0);
					if(op_rec.OpCounterID != bs_rec.CounterID)
						set_counter = 1;
				}
				if(bs_obj.Fetch(prev_status_id, &prev_bs_rec) > 0) {
					THROW_PP(!(prev_bs_rec.Flags & BILSTF_DENY_RANKDOWN) || bs_rec.Rank >= prev_bs_rec.Rank, PPERR_BILLST_DENY_RANKDOWN);
					if((prev_bs_rec.Flags & BILSTF_LOCK_ACCTURN) != (bs_rec.Flags & BILSTF_LOCK_ACCTURN))
						set_noaturn_flag = BIN(bs_rec.Flags & BILSTF_LOCK_ACCTURN);
					if((prev_bs_rec.Flags & BILSTF_LOCK_PAYMENT) != (bs_rec.Flags & BILSTF_LOCK_PAYMENT))
						set_nopaym_flag = BIN(bs_rec.Flags & BILSTF_LOCK_PAYMENT);
					if(set_counter > 0 && prev_bs_rec.CounterID == bs_rec.CounterID)
						set_counter = 0;
				}
				else {
					if(bs_rec.Flags & BILSTF_LOCK_ACCTURN)
						set_noaturn_flag = 1;
					if(bs_rec.Flags & BILSTF_LOCK_PAYMENT)
						set_nopaym_flag = 1;
				}
				//
				// Сверяем требование установки/снятия флага BILLF_NOATURN с тем, что установлено
				// на текущий момент в документе.
				//
				if(set_noaturn_flag > 0 && rec.Flags & BILLF_NOATURN)
					set_noaturn_flag = -1; // Флаг уже и так установлен
				else if(set_noaturn_flag == 0 && !(rec.Flags & BILLF_NOATURN))
					set_noaturn_flag = -1; // Флаг уже и так снят
				if(set_noaturn_flag >= 0) {
					//
					// Если требуется изменить флаг BILLF_NOATURN то придется перепровести
					// документ поскольку изменение этого флага влечет удаление или проведение бух проводок
					//
					PPBillPacket pack;
					THROW(ExtractPacket(id, &pack) > 0);
					pack.Rec.StatusID = statusID;
					SETFLAG(pack.Rec.Flags, BILLF_NOATURN, (set_noaturn_flag > 0));
					if(bs_rec.CheckFields || (bs_rec.Flags & BILSTF_STRICTPRICECONSTRAINS)) // @v10.3.6 @fix (bs_rec.Flags & BILSTF_STRICTPRICECONSTRAINS)
						THROW(ValidatePacket(&pack, 0));
					if(set_counter > 0)
						THROW(opc_obj.GetCode(bs_rec.CounterID, 0, pack.Rec.Code, sizeof(pack.Rec.Code), pack.Rec.LocID, 0));
					pack.Rec.Flags |= BILLF_NOLOADTRFR;
					THROW(FillTurnList(&pack));
					THROW(UpdatePacket(&pack, 0));
				}
				else {
					rec.StatusID = statusID;
					if(bs_rec.CheckFields || (bs_rec.Flags & BILSTF_STRICTPRICECONSTRAINS)) { // @v10.3.6 @fix (bs_rec.Flags & BILSTF_STRICTPRICECONSTRAINS)
						//
						// Если новый статус требует проверки заполнения полей, то придется извлечь
						// пакет документа полностью.
						//
						PPBillPacket pack;
						THROW(ExtractPacket(id, &pack) > 0);
						pack.Rec.StatusID = statusID;
						THROW(ValidatePacket(&pack, 0));
					}
					if(set_counter > 0)
						THROW(opc_obj.GetCode(bs_rec.CounterID, 0, rec.Code, sizeof(rec.Code), rec.LocID, 0));
					THROW(P_Tbl->EditRec(&id, &rec, 0));
					if(set_nopaym_flag >= 0) {
						PPID   paym_link_id = 0;
						IsMemberOfPool(id, PPASS_PAYMBILLPOOL, &paym_link_id);
						THROW(ProcessLink(rec, paym_link_id, &org_rec));
					}
				}
				if(bs_rec.Flags & BILSTF_LOCDISPOSE && p_disposer) {
					PPIDArray bill_list;
					bill_list.add(id);
					THROW(p_disposer->Dispose(bill_list, &logger, 0));
				}
				DS.LogAction(PPACN_BILLSTATUSUPD, Obj, id, prev_status_id, 0);
				ok = 1;
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	if(is_locked)
		Unlock(id);
	delete p_disposer;
	return ok;
}

//int PPObjBill::GetSnByTemplate(const char * pBillCode, PPID goodsID, const ClbNumberList * pExclList, const char * pTempl, SString & rBuf)
int PPObjBill::GetSnByTemplate(const char * pBillCode, PPID goodsID, const PPLotTagContainer * pExclList, const char * pTempl, SString & rBuf)
{
	const  long sGR  = 0x00524740L; // "@GR" Код группы товаров
	const  long sGS  = 0x00534740L; // "@GS" Штрихкод товара
	const  long sBN  = 0x004E4240L; // "@BN" Номер документа

	int    ok = 1;
	size_t x_len = 0, r_len = 0;
	double low = 0.0, upp = 0.0;
	char   pfx[48], t[48];
	SString code;
	char   templ[64];
	char * c = pfx;
	char * x;
	const  char * p = 0;
	Goods2Tbl::Rec goods_rec;

	rBuf.Z();

	PPObjOpCounter opc_obj;
	PPOpCounterPacket opc_pack;
	{
		PPTransaction tra(1);
		THROW(tra);
		{
			//
			// Обратная совместимость с версиями, меньшими, чем @v5.0.0
			//
			if(Cfg.SnCntrID == 0) {
				STRNSCPY(opc_pack.Head.CodeTemplate, Cfg.SnTemplt);
				opc_pack.Init(0, 0);
				opc_pack.Head.Counter = Cfg.SnrCounter;
				THROW(PPObjBill_WriteConfig(&Cfg, &opc_pack, 0));
			}
		}
		THROW(opc_obj.GetPacket(Cfg.SnCntrID, &opc_pack) > 0);
		{
			STRNSCPY(templ, pTempl);
			if(*strip(templ) == 0) {
				STRNSCPY(templ, opc_pack.Head.CodeTemplate);
				strip(templ);
			}
			p = templ;
		}
		if(*p) {
			memzero(pfx, sizeof(pfx));
			while(*p) {
				if(isdec(*p))
					*c++ = *p++;
				else if(strnicmp(p, (char *)&sGR, 3) == 0) {
					if(GObj.Fetch(goodsID, &goods_rec) > 0) {
						if(GObj.GetSingleBarcode(goods_rec.ParentID, code) > 0) {
							code.ShiftLeftChr('@').Strip();
							c += sstrlen(strcpy(c, code));
						}
						p += 3;
					}
				}
				else if(strnicmp(p, (char *)&sGS, 3) == 0) {
					if(GObj.GetSingleBarcode(goodsID, code) > 0)
						c += sstrlen(strcpy(c, code.Strip()));
					p += 3;
				}
				else if(strnicmp(p, (char *)&sBN, 3) == 0) {
					c += sstrlen(strcpy(c, (code = pBillCode).Strip()));
					p += 3;
				}
				else if(*p == '%') {
					x_len = sstrlen(pfx);
					for(++p, x = t; isdec(*p);)
						*x++ = *p++;
					*x = 0;
					r_len = atoi(t);
					if(*p == '[') {
						for(++p, x = t; *p && *p != ']';)
							*x++ = *p++;
						*x = 0;
						strtorrng(t, &low, &upp);
					}
					if(low <= 0)
						low = 1;
					if(upp <= 0 || upp > (fpow10i((int)r_len) - 1))
						upp = fpow10i((int)r_len) - 1;
					{
						long   counter = 0;
						LongArray frlist;
						char   pttrn[48];
						int    f = 0;
						opc_pack.GetCounter(0, &counter);
						low = (low >= (double)(counter % (long)upp)) ? low : (double)(counter % (long)upp); // AHTOXA
						for(long n = (long)low; !f && n <= (long)upp; n++) {
							memzero(pttrn, sizeof(pttrn));
							sprintf(pttrn, "%s%0*ld", pfx, (int)r_len, n);
							// @v9.8.11 if(pExclList && pExclList->SearchNumber(pttrn, 0) > 0)
							if(pExclList && pExclList->SearchString(pttrn, PPTAG_LOT_SN, 0, frlist) > 0) // @v9.8.11
								continue;
							else {
								f = 1;
								counter += (n - counter % (long)upp + 1); // AHTOXA
							}
						}
						if(f) {
							strcpy(pfx, pttrn);
							counter = (counter <= 0) ? 1 : counter; // AHTOXA
							THROW(opc_pack.CounterIncr(0, 0));
							opc_pack.Flags |= PPOpCounterPacket::fDontLogUpdAction;
							THROW(opc_obj.PutPacket(&Cfg.SnCntrID, &opc_pack, 0));
						}
						else {
							memset(pttrn, '0', x_len + r_len);
							pttrn[x_len + r_len] = 0;
							rBuf = pttrn;
							ok = 0;
						}
					}
					break;
				}
				else
					*c++ = *p++;
			}
			if(ok > 0)
				rBuf = pfx;
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::GetLabelLotInfo(PPID lotID, RetailGoodsInfo * pData)
{
	int    ok = 1;
	RetailGoodsInfo rgi;
	ReceiptTbl::Rec lot_rec;
	if(trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
		SString temp_buf;
		BillTbl::Rec bill_rec;
		GObj.GetRetailGoodsInfo(labs(lot_rec.GoodsID), lot_rec.LocID, &rgi);
		rgi.LotID  = lot_rec.ID;
		rgi.LocID  = lot_rec.LocID;
		//rgi.Expiry = lot_rec.Expiry;
		rgi.Qtty        = lot_rec.Quantity;
		rgi.UnitPerPack = lot_rec.UnitPerPack;
		rgi.PhQtty      = R6(lot_rec.Quantity * rgi.PhUPerU);
		//rgi.Price  = R5(lot_rec.Price);
		if(Search(lot_rec.BillID, &bill_rec) > 0) {
			rgi.BillDate = bill_rec.Dt;
			P_Tbl->GetCode(STRNSCPY(rgi.BillCode, bill_rec.Code));
			if(bill_rec.Object) {
				GetArticleName(bill_rec.Object, temp_buf);
				temp_buf.CopyTo(rgi.ArName, sizeof(rgi.ArName));
			}
			if(bill_rec.Object2) {
				GetArticleName(bill_rec.Object2, temp_buf);
				temp_buf.CopyTo(rgi.Ar2Name, sizeof(rgi.Ar2Name));
			}
		}
		GetSerialNumberByLot(lotID, temp_buf, 0);
		temp_buf.CopyTo(rgi.Serial, sizeof(rgi.Serial));
		//
		// Цены поступления и реализации должны в данной ситуации безусловно
		// браться из строки (если, конечно, они в ней определены).
		//
		rgi.LineCost = lot_rec.Cost;
		rgi.LinePrice = lot_rec.Price;
		//
		rgi.RevalPrice = rgi.Price;
	}
	else
		ok = -1;
	ASSIGN_PTR(pData, rgi);
	return ok;
}

int PPObjBill::GetComplementGoodsBillList(PPID billID, PPIDArray & rComplBillList)
{
	int    ok = -1;
	PPIDArray rh_bill_list;
	BillTbl::Rec bill_rec, link_rec;
	if(Fetch(billID, &bill_rec) > 0) {
		if(IsDraftOp(bill_rec.OpID)) {
			for(DateIter di; P_Tbl->EnumLinks(billID, &di, BLNK_ALL, &link_rec) > 0;) {
				const PPID op_type_id = GetOpType(link_rec.OpID);
				if(oneof5(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT))
					rh_bill_list.addUnique(link_rec.ID);
			}
		}
		else if(bill_rec.LinkBillID && Fetch(bill_rec.LinkBillID, &link_rec) > 0) {
			PPID   op_type_id = GetOpType(bill_rec.OpID);
			if(oneof5(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
				if(IsDraftOp(link_rec.OpID))
					rh_bill_list.addUnique(link_rec.ID);
			}
			else if(op_type_id == PPOPT_GOODSACK) {
				op_type_id = GetOpType(link_rec.OpID);
				if(oneof2(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT))
					rh_bill_list.addUnique(link_rec.ID);
			}
		}
		else {
			for(DateIter di; P_Tbl->EnumLinks(billID, &di, BLNK_ACK, &link_rec) > 0;) {
				rh_bill_list.addUnique(link_rec.ID);
			}
		}
	}
	if(rh_bill_list.getCount())
		ok = 1;
	rComplBillList = rh_bill_list;
	return ok;
}

int PPObjBill::GetCorrectionBackChain(const BillTbl::Rec & rBillRec, PPIDArray & rChainList)
{
	int    ok = -1;
	rChainList.clear();
	BillTbl::Rec correction_org_bill_rec;
	BillTbl::Rec temp_bill_rec;
	if(rBillRec.LinkBillID && Fetch(rBillRec.LinkBillID, &correction_org_bill_rec) > 0) { // @v10.3.5 Search-->Fetch
		ok = 1;
		const LDATE tbdt = rBillRec.Dt;
		const PPID  tbop_id = rBillRec.OpID;
		const long  tbbillno = NZOR(rBillRec.BillNo, MAXLONG);
		rChainList.add(correction_org_bill_rec.ID);
		for(DateIter di(correction_org_bill_rec.Dt, ZERODATE); P_Tbl->EnumLinks(correction_org_bill_rec.ID, &di, BLNK_CORRECTION, &temp_bill_rec) > 0;) {
			if(temp_bill_rec.OpID == tbop_id && temp_bill_rec.ID != rBillRec.ID &&
				(temp_bill_rec.Dt < tbdt || (temp_bill_rec.Dt == tbdt && temp_bill_rec.BillNo < tbbillno))) {
				rChainList.add(temp_bill_rec.ID);
				ok = 2;
			}
		}
	}
	return ok;
}

int PPObjBill::GetCorrectionBackChain(PPID billID, PPIDArray & rChainList)
{
	int    ok = -1;
	rChainList.clear();
	BillTbl::Rec this_bill_rec;
	if(Fetch(billID, &this_bill_rec) > 0) { // @v10.3.5 Search-->Fetch
		ok = GetCorrectionBackChain(this_bill_rec, rChainList);
	}
	return ok;
}

const PPBillConfig & PPObjBill::GetConfig() const
{
	return Cfg;
}
//
//
//
PPBillConfig::PPBillConfig()
{
	Z();
}

PPBillConfig & PPBillConfig::Z()
{
	memzero(PTR8(this)+offsetof(PPBillConfig, SecurID), offsetof(PPBillConfig, TagIndFilt)-offsetof(PPBillConfig, SecurID));
	Ver = DS.GetVersion();
	AddFilesFolder = 0;
	TagIndFilt.Init(1, 0);
	return *this;
}

struct __PPBillConfig {    // @persistent @store(PropertyTbl)
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_BILLCFG
	char   OpCodePrfx[8];  //
	char   ClCodePrfx[8];  //
	PPID   ClCodeRegTypeID;  //
	long   Flags;          //
	PPID   CashNodeID;     //
	PPID   InitStatusID;   // Статус, присваиваемый новым документам
	int16  GoodsSubstMethod; // Способ подстановки товара вместо дефицитного
	PPID   LnkFilesCntrID; //
	LDATE  SwitchedTDisCalcMethodDate; //
	uint8  TDisCalcMethod; //
	uint8  TDisCalcPrec;   // @#[0..5]
	PPID   SnCntrID;       //
	long   SnrCounter;     // @obsolete since @v5.0.0
	char   SnTemplt[16];   // @obsolete since @v5.0.0
	char   InvSnTemplt[16];  //
	int16  ValuationRndDir;    // Направление округления при расценке
	int16  Reserve;            // @reserve
	PPID   ValuationQuotKindID; // ->Ref(PPOBJ_QUOTKIND) Вид котировки для расценки приходных документов
	float  ValuationRndPrec;   // Точность округления при расценке
	char   UniqSerialSfx[16];  // Сигнатура суффикса, присоединяемого к серийному номеру для обеспечения его уникальности.
	SVerT  Ver;                // @anchor
	PPID   ContractRegTypeID;  //
	PPID   MnfCountryLotTagID; //
	LDATE  LowDebtCalcDate;    //
	int16  WarnLotExpirDays;   // @v10.4.4 Количество дней до предупреждения об истечении срока годности лота
	uint16 WarnLotExpirFlags;  // @v10.4.4 Опции действий при угрозе истечения срока годности лота
	uint8  Reserve2[16];       // @reserve @v10.4.4 [20]-->[16]
};

// @v10.7.10 replaced with (PPConstParam::WrParam_BillAddFilesFolder) const char * BillAddFilesFolder = "BillAddFilesFolder";

/*static*/int FASTCALL PPObjBill::ReadConfig(PPBillConfig * pCfg)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	pCfg->Z();
	size_t sz = 0;
	const  size_t fix_size = sizeof(__PPBillConfig);
	if(p_ref->GetPropActualSize(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_BILLCFG, &sz) > 0) {
		STempBuffer temp_buf(0);
		__PPBillConfig temp;
		const __PPBillConfig * p_temp = 0;
		MEMSZERO(temp);
		if(sz <= fix_size) {
			ok = p_ref->GetPropMainConfig(PPPRP_BILLCFG, &temp, sz);
			assert(ok > 0); // Раз нам удалось считать размер буфера, то последующая ошибка чтения - критична
			THROW(ok > 0);
			p_temp = &temp;
		}
		else {
			THROW_SL(temp_buf.Alloc(sz));
			ok = p_ref->GetPropMainConfig(PPPRP_BILLCFG, temp_buf.vptr(), sz);
			assert(ok > 0); // Раз нам удалось считать размер буфера, то последующая ошибка чтения - критична
			THROW(ok > 0);
			p_temp = static_cast<const __PPBillConfig *>(temp_buf.vcptr());
		}
		pCfg->SecurID = p_temp->ID;
		pCfg->ClCodeRegTypeID = p_temp->ClCodeRegTypeID;
		pCfg->Flags        = p_temp->Flags;
		pCfg->CashNodeID   = p_temp->CashNodeID;
		pCfg->InitStatusID = p_temp->InitStatusID;
		STRNSCPY(pCfg->OpCodePrfx, p_temp->OpCodePrfx);
		STRNSCPY(pCfg->ClCodePrfx, p_temp->ClCodePrfx);
		STRNSCPY(pCfg->SnTemplt, p_temp->SnTemplt); // @obsolete since @v5.0.0
		STRNSCPY(pCfg->InvSnTemplt, p_temp->InvSnTemplt);
		pCfg->SwitchedTDisCalcMethodDate = p_temp->SwitchedTDisCalcMethodDate;
		pCfg->TDisCalcMethod = p_temp->TDisCalcMethod;
		pCfg->TDisCalcPrec   = p_temp->TDisCalcPrec;
		pCfg->SnrCounter = (p_temp->SnrCounter <= 0) ? 1 : p_temp->SnrCounter; // AHTOXA // @obsolete since @v5.0.0
		pCfg->SnCntrID   = p_temp->SnCntrID;
		pCfg->ContractRegTypeID = p_temp->ContractRegTypeID;
		pCfg->MnfCountryLotTagID = p_temp->MnfCountryLotTagID;
		pCfg->LowDebtCalcDate = p_temp->LowDebtCalcDate;
		pCfg->WarnLotExpirDays = p_temp->WarnLotExpirDays; // @v10.4.4
		pCfg->WarnLotExpirFlags = p_temp->WarnLotExpirFlags; // @v10.4.4
		pCfg->GoodsSubstMethod = p_temp->GoodsSubstMethod;
		pCfg->LnkFilesCntrID = p_temp->LnkFilesCntrID;
		pCfg->ValuationRndDir = p_temp->ValuationRndDir;
		pCfg->ValuationQuotKindID = p_temp->ValuationQuotKindID;
		pCfg->ValuationRndPrec = p_temp->ValuationRndPrec;
		STRNSCPY(pCfg->UniqSerialSfx, p_temp->UniqSerialSfx);
		pCfg->Ver = p_temp->Ver;
		if(sz > fix_size) {
			SBuffer ser_buf;
			THROW_SL(ser_buf.Write(PTR8C(p_temp)+fix_size, sz - fix_size));
			if(!pCfg->TagIndFilt.Read(ser_buf, 0)) {
				pCfg->TagIndFilt.Init(1, 0);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
			// @v10.4.3 {
			if(pCfg->Ver.IsGt(10, 4, 2)) {
				if(!pCfg->LotTagIndFilt.Read(ser_buf, 0)) {
					pCfg->LotTagIndFilt.Init(1, 0);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				}
			}
			// } @v10.4.3
		}
		{
			size_t buf_size = 0;
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
			if(reg_key.GetRecSize(_PPConst.WrParam_BillAddFilesFolder, &buf_size) > 0 && buf_size > 0) {
				SString param_buf;
				reg_key.GetString(_PPConst.WrParam_BillAddFilesFolder, param_buf);
				pCfg->AddFilesFolder.CopyFrom(param_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill_WriteConfig(PPBillConfig * pCfg, PPOpCounterPacket * pSnCntr, int use_ta)
{
	int    ok = 1;
	PPObjOpCounter opc_obj;
	__PPBillConfig temp;
	__PPBillConfig * p_temp = 0;
	STempBuffer temp_buf(0);
	size_t sz = sizeof(__PPBillConfig);
	THROW(CheckCfgRights(PPCFGOBJ_BILL, PPR_MOD, 0));
	if(pCfg->TagIndFilt.IsEmpty() && pCfg->LotTagIndFilt.IsEmpty()) { // @v10.4.3 (&& pCfg->LotTagIndFilt.IsEmpty())
		p_temp = &temp;
	}
	else {
		SBuffer ser_buf;
		THROW(pCfg->TagIndFilt.Write(ser_buf, 0));
		THROW(pCfg->LotTagIndFilt.Write(ser_buf, 0)); // @v10.4.3
		sz += ser_buf.GetAvailableSize();
		const  size_t offs = sizeof(__PPBillConfig);
		THROW_SL(temp_buf.Alloc(sz));
		THROW_SL(ser_buf.Read(PTR8(temp_buf.vptr())+offs, sz - offs));
		p_temp = static_cast<__PPBillConfig *>(temp_buf.vptr());
	}
	memzero(p_temp, sizeof(__PPBillConfig));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pSnCntr)	{
			pSnCntr->Head.ObjType = PPOBJ_BILL;
			pSnCntr->Head.OwnerObjID = -1;
			THROW(opc_obj.PutPacket(&pCfg->SnCntrID, pSnCntr, 0));
		}
		p_temp->ID = pCfg->SecurID;
		p_temp->ClCodeRegTypeID = pCfg->ClCodeRegTypeID;
		p_temp->Flags = pCfg->Flags;
		p_temp->CashNodeID   = pCfg->CashNodeID;
		p_temp->InitStatusID = pCfg->InitStatusID;
		p_temp->SwitchedTDisCalcMethodDate = pCfg->SwitchedTDisCalcMethodDate;
		p_temp->TDisCalcMethod = pCfg->TDisCalcMethod;
		p_temp->TDisCalcPrec   = pCfg->TDisCalcPrec;
		p_temp->SnrCounter = (pCfg->SnrCounter <= 0) ? 1 : pCfg->SnrCounter; // @obsolete since @v5.0.0
		STRNSCPY(p_temp->SnTemplt,    pCfg->SnTemplt);                       // @obsolete since @v5.0.0
		p_temp->SnCntrID = pCfg->SnCntrID;
		p_temp->ContractRegTypeID = pCfg->ContractRegTypeID;
		p_temp->MnfCountryLotTagID = pCfg->MnfCountryLotTagID;
		p_temp->LowDebtCalcDate = pCfg->LowDebtCalcDate;
		p_temp->WarnLotExpirDays = pCfg->WarnLotExpirDays; // @v10.4.4
		p_temp->WarnLotExpirFlags = pCfg->WarnLotExpirFlags; // @v10.4.4
		STRNSCPY(p_temp->InvSnTemplt, pCfg->InvSnTemplt);
		STRNSCPY(p_temp->OpCodePrfx, pCfg->OpCodePrfx);
		STRNSCPY(p_temp->ClCodePrfx, pCfg->ClCodePrfx);
		p_temp->GoodsSubstMethod = pCfg->GoodsSubstMethod;
		p_temp->LnkFilesCntrID = pCfg->LnkFilesCntrID;

		p_temp->ValuationRndDir = pCfg->ValuationRndDir;
		p_temp->ValuationQuotKindID = pCfg->ValuationQuotKindID;
		p_temp->ValuationRndPrec = static_cast<float>(pCfg->ValuationRndPrec);
		STRNSCPY(p_temp->UniqSerialSfx, pCfg->UniqSerialSfx);
		p_temp->Ver = DS.GetVersion();
		{
			char reg_buf[32];
			memzero(reg_buf, sizeof(reg_buf));
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
			reg_key.PutString(_PPConst.WrParam_BillAddFilesFolder, (pCfg->AddFilesFolder.Len() == 0) ? reg_buf : pCfg->AddFilesFolder);
		}
		THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_BILLCFG, p_temp, sz, 0));
		DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_BILL, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

struct TDisCalcMethodParam {
	TDisCalcMethodParam() : Method(0), ThresholdDate(ZERODATE), Prec(0), Pad(0)
	{
	}
	long   Method;        //
	LDATE  ThresholdDate; //
	int16  Prec;          //
	uint16 Pad;           // @alignment
};

static int EditTDisCalcMethod(TDisCalcMethodParam * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_TDISCALC);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->AddClusterAssocDef(CTL_TDISCALC_METHOD, 0, PPBillConfig::tdcmSimple);
		dlg->AddClusterAssoc(CTL_TDISCALC_METHOD, 1, PPBillConfig::tdcmRegress);
		dlg->AddClusterAssoc(CTL_TDISCALC_METHOD, 2, PPBillConfig::tdcmRegress2);
		dlg->SetClusterData(CTL_TDISCALC_METHOD, pData->Method);
		dlg->setCtrlData(CTL_TDISCALC_PREC, &pData->Prec);
		dlg->SetupCalDate(CTLCAL_TDISCALC_DTSWITCH, CTL_TDISCALC_DTSWITCH);
		dlg->setCtrlData(CTL_TDISCALC_DTSWITCH, &pData->ThresholdDate);
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			dlg->GetClusterData(CTL_TDISCALC_METHOD, &pData->Method);
			dlg->getCtrlData(CTL_TDISCALC_PREC,      &pData->Prec);
			dlg->getCtrlData(CTL_TDISCALC_DTSWITCH,  &pData->ThresholdDate);
			if(!checkdate(pData->ThresholdDate, 1))
				PPErrorByDialog(dlg, CTL_TDISCALC_DTSWITCH, PPERR_SLIB);
			else if(pData->Prec < 0 || pData->Prec > 5)
				PPErrorByDialog(dlg, CTL_TDISCALC_PREC, PPERR_USERINPUT);
			else
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

static int EditBillCfgValuationParam(PPBillConfig * pData)
{
	int    ok = -1;
	struct ValuationParam : public CalcPriceParam {
	public:
		ValuationParam() : CalcPriceParam(), _Flags(0)
		{
		}
		int    Set(const PPBillConfig & rCfg)
		{
			QuotKindID = rCfg.ValuationQuotKindID;
			RoundDir = rCfg.ValuationRndDir;
			RoundPrec = rCfg.ValuationRndPrec;
			SETFLAG(Flags, fRoundVat, rCfg.Flags & BCF_VALUATION_RNDVAT);
			SETFLAG(_Flags, _fStrict, rCfg.Flags & BCF_VALUATION_STRICT);
			SETFLAG(_Flags, _fByContract, rCfg.Flags & BCF_VALUATION_BYCONTRACT);
			return 1;
		}
		int    Get(PPBillConfig & rCfg)
		{
			rCfg.ValuationQuotKindID = QuotKindID;
			rCfg.ValuationRndDir = RoundDir;
			rCfg.ValuationRndPrec = RoundPrec;
			SETFLAG(rCfg.Flags, BCF_VALUATION_RNDVAT, Flags & fRoundVat);
			SETFLAG(rCfg.Flags, BCF_VALUATION_STRICT, _Flags & _fStrict);
			SETFLAG(rCfg.Flags, BCF_VALUATION_BYCONTRACT, _Flags & _fByContract);
			return 1;
		}
		enum {
			_fStrict     = 0x0001, // Параметры расценки устанавливать только по конфигурации //
			_fByContract = 0x0002
		};
		long   _Flags;
	};
	ValuationParam param;
	param.Set(*pData);
	TDialog * dlg = new TDialog(DLG_BILLCFGVAL);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_SELQUOT2_KIND, PPOBJ_QUOTKIND, param.QuotKindID, 0, 0);
		dlg->setCtrlData(CTL_SELQUOT2_PREC, &param.RoundPrec);
		dlg->AddClusterAssocDef(CTL_SELQUOT2_ROUND,  0, 0);
		dlg->AddClusterAssoc(CTL_SELQUOT2_ROUND,  1, -1);
		dlg->AddClusterAssoc(CTL_SELQUOT2_ROUND,  2, +1);
		dlg->SetClusterData(CTL_SELQUOT2_ROUND, param.RoundDir);
		dlg->AddClusterAssoc(CTL_SELQUOT2_ROUNDVAT, 0, CalcPriceParam::fRoundVat);
		dlg->SetClusterData(CTL_SELQUOT2_ROUNDVAT, param.Flags);
		dlg->AddClusterAssoc(CTL_SELQUOT2_FLAGS_, 0, ValuationParam::_fStrict);
		dlg->AddClusterAssoc(CTL_SELQUOT2_FLAGS_, 1, ValuationParam::_fByContract);
		dlg->SetClusterData(CTL_SELQUOT2_FLAGS_, param._Flags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_SELQUOT2_KIND, &param.QuotKindID);
			dlg->GetClusterData(CTL_SELQUOT2_FLAGS_, &param._Flags);
			if(param._Flags & ValuationParam::_fStrict && !param.QuotKindID) {
				PPErrorByDialog(dlg, CTL_SELQUOT2_KIND, PPERR_STRICTMODE_QKNEEDED);
			}
			else {
				dlg->getCtrlData(CTL_SELQUOT2_PREC,        &param.RoundPrec);
				dlg->GetClusterData(CTL_SELQUOT2_ROUNDVAT, &param.Flags);
				param.RoundDir = static_cast<int16>(dlg->GetClusterData(CTL_SELQUOT2_ROUND));
				param.Get(*pData);
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class BillConfigAddednumDialog : public TDialog {
	typedef PPBillConfig DlgDataType;
	DlgDataType Data;
	enum {
		ctlgroupFilesFold = 1
	};
public:
	BillConfigAddednumDialog() : TDialog(DLG_BILLCFGEXT)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_BILLCFG_FILESFOLD, CTL_BILLCFG_FILESFOLDER, ctlgroupFilesFold, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		SetupCalDate(CTLCAL_BILLCFG_LOWDEBTDATE, CTL_BILLCFG_LOWDEBTDATE);
	}
	int setDTS(const DlgDataType * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		SetupPPObjCombo(this, CTLSEL_BILLCFG_CASHNODE, PPOBJ_CASHNODE, Data.CashNodeID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_BILLCFG_CREGTYP, PPOBJ_REGISTERTYPE, Data.ContractRegTypeID, 0, 0);
		{
			ObjTagFilt ot_filt;
			ot_filt.ObjTypeID = PPOBJ_LOT;
			SetupObjTagCombo(this, CTLSEL_BILLCFG_MNFLTAG, Data.MnfCountryLotTagID, 0, &ot_filt);
		}
		setCtrlData(CTL_BILLCFG_LOWDEBTDATE, &Data.LowDebtCalcDate);
		setCtrlString(CTL_BILLCFG_FILESFOLDER, Data.AddFilesFolder);
		// @v10.4.4 {
		AddClusterAssoc(CTL_BILLCFG_WLEF, 0, Data.wlefIndicator);
		AddClusterAssoc(CTL_BILLCFG_WLEF, 1, Data.wlefDisalbePosOp);
		AddClusterAssoc(CTL_BILLCFG_WLEF, 2, Data.wlefDisableBillOp);
		SetClusterData(CTL_BILLCFG_WLEF, Data.WarnLotExpirFlags);
		setCtrlData(CTL_BILLCFG_WLED, &Data.WarnLotExpirDays);
		// } @v10.4.4
		return ok;
	}
	int getDTS(DlgDataType * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(CTLSEL_BILLCFG_CASHNODE,  &Data.CashNodeID);
		getCtrlData(CTLSEL_BILLCFG_CREGTYP,   &Data.ContractRegTypeID);
		getCtrlData(CTLSEL_BILLCFG_MNFLTAG,   &Data.MnfCountryLotTagID);
		getCtrlData(sel = CTL_BILLCFG_LOWDEBTDATE,  &Data.LowDebtCalcDate);
		THROW_SL(checkdate(Data.LowDebtCalcDate, 1));
		getCtrlString(CTL_BILLCFG_FILESFOLDER, Data.AddFilesFolder);
		// @v10.4.4 {
		Data.WarnLotExpirFlags = static_cast<uint16>(GetClusterData(CTL_BILLCFG_WLEF));
		getCtrlData(CTL_BILLCFG_WLED, &Data.WarnLotExpirDays);
		// } @v10.4.4
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
};

static int EditBillCfgAddendum(PPBillConfig * pData) { DIALOG_PROC_BODY(BillConfigAddednumDialog, pData); }

/*static*/int PPObjBill::EditConfig()
{
	class BillConfigDialog : public TDialog {
	public:
		explicit BillConfigDialog(PPBillConfig * pCfg) : TDialog(DLG_BILLCFG), P_Cfg(pCfg)
		{
		}
		TDisCalcMethodParam Tdcmp;
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmBillCfgTDisMethod)) {
				TDisCalcMethodParam p = Tdcmp;
				if(EditTDisCalcMethod(&p) > 0)
					Tdcmp = p;
			}
			else if(event.isCmd(cmBillCfgValuation)) {
				EditBillCfgValuationParam(P_Cfg);
			}
			else if(event.isCmd(cmTagIndFilt)) {
				P_Cfg->TagIndFilt.Flags |= TagFilt::fColors; // @v9.8.6
				EditTagFilt(PPOBJ_BILL, &P_Cfg->TagIndFilt);
			}
			// @v10.4.3 {
			else if(event.isCmd(cmLotTagIndFilt)) {
				P_Cfg->LotTagIndFilt.Flags |= TagFilt::fColors;
				EditTagFilt(PPOBJ_LOT, &P_Cfg->LotTagIndFilt);
			}
			// } @v10.4.3
			else if(event.isCmd(cmaMore)) {
				EditBillCfgAddendum(P_Cfg);
			}
			else
				return;
			clearEvent(event);
		}
		PPBillConfig * P_Cfg;
	};
	int    ok = 1, r;
	int    valid_data = 0;
	ushort v = 0;
	char   sn_buf[48], invsn_buf[48];
	PPBillConfig cfg;
	PPObjOpCounter opc_obj;
	PPOpCounterPacket sn_cntr;
	BillConfigDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_BILL, PPR_READ, 0));
	THROW(ReadConfig(&cfg));
	if(!opc_obj.GetPacket(cfg.SnCntrID, &sn_cntr)) // THROW не используем чтобы не заблокировать
		PPError();                                 // доступ ко всей конфигурации только из-за счетчика
	if(sn_cntr.Head.ID == 0) {
		STRNSCPY(sn_cntr.Head.CodeTemplate, cfg.SnTemplt);
		sn_cntr.Head.Counter = cfg.SnrCounter;
	}
	THROW(CheckDialogPtr(&(dlg = new BillConfigDialog(&cfg))));
	dlg->setCtrlData(CTL_BILLCFG_OPPRFX, cfg.OpCodePrfx);
	dlg->setCtrlData(CTL_BILLCFG_SNRCNTR, &sn_cntr.Head.Counter);
	dlg->setCtrlData(CTL_BILLCFG_CLPRFX, cfg.ClCodePrfx);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  0, BCF_CONFIRMWL);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  1, BCF_ALLOWZSUPPLINCOSTREVAL);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  2, BCF_WARNMATCLIDEBT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  3, BCF_WARNAGREEMENT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  4, BCF_SIGNDIFFLOTCOST);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  5, BCF_WARNADDBILLNOFLT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  6, BCF_ALLOWMULTIPRINT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  7, BCF_ADDAUTOQTTYBYBRCODE);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  8, BCF_DONTWARNDUPGOODS);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS,  9, BCF_RETAILEDPRICEINLABEL);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS, 10, BCF_AUTOSERIAL);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS, 11, BCF_AUTOCOMPLOUTBYQUOT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS, 12, BCF_PAINTSHIPPEDBILLS);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS, 13, BCF_OVRRDAGTQUOT);
	dlg->SetClusterData(CTL_BILLCFG_FLAGS, cfg.Flags);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  0, BCF_CHECKRESERVEDORDERS);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  1, BCF_ORDPRICELOWPRIORITY);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  2, BCF_EXTOBJASMAINORG);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  3, BCF_ACCEPTGOODSBILLCHG);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  4, BCF_SENDATTACHMENT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  5, BCF_DONTINHQCERT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  6, BCF_RETINHERITFREIGHT);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  7, BCF_PICKLOTS);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  8, BCF_INHSERIAL);
	dlg->AddClusterAssoc(CTL_BILLCFG_FLAGS2,  9, BCF_DONTVERIFEXTCODECHAIN); // @v10.8.0
	dlg->SetClusterData(CTL_BILLCFG_FLAGS2, cfg.Flags);
	dlg->AddClusterAssoc(CTL_BILLCFG_SHOWADDFLD, 0, BCF_SHOWBARCODESINGBLINES);
	dlg->AddClusterAssoc(CTL_BILLCFG_SHOWADDFLD, 1, BCF_SHOWSERIALSINGBLINES);
	dlg->SetClusterData(CTL_BILLCFG_SHOWADDFLD, cfg.Flags);
	SetupPPObjCombo(dlg, CTLSEL_BILLCFG_CLCODEREG, PPOBJ_REGISTERTYPE, cfg.ClCodeRegTypeID, 0, 0);
	//SetupPPObjCombo(dlg, CTLSEL_BILLCFG_CASHNODE, PPOBJ_CASHNODE, cfg.CashNodeID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_BILLCFG_INITST, PPOBJ_BILLSTATUS, cfg.InitStatusID, 0, 0);
	// SetupPPObjCombo(dlg, CTLSEL_BILLCFG_CREGTYP, PPOBJ_REGISTERTYPE, cfg.ContractRegTypeID, 0, 0); // @v8.4.0
	// @v8.4.11 {
	/*
	{
		ObjTagFilt ot_filt;
		ot_filt.ObjTypeID = PPOBJ_LOT;
		SetupObjTagCombo(dlg, CTLSEL_BILLCFG_MNFLTAG, cfg.MnfCountryLotTagID, 0, &ot_filt);
	}
	*/
	// } @v8.4.11
	STRNSCPY(sn_buf, sn_cntr.Head.CodeTemplate);
	STRNSCPY(invsn_buf, cfg.InvSnTemplt);
	dlg->setCtrlData(CTL_BILLCFG_SNTEMPLT, sn_buf);
	dlg->setCtrlData(CTL_BILLCFG_INVSNTEMPLT, invsn_buf);
	dlg->setCtrlData(CTL_BILLCFG_UNIQSNSFX, cfg.UniqSerialSfx);
	dlg->AddClusterAssocDef(CTL_BILLCFG_DEFICITSUBST, 0, PPBillConfig::gsmGeneric);
	dlg->AddClusterAssoc(CTL_BILLCFG_DEFICITSUBST, 1, PPBillConfig::gsmSubstStruc);
	dlg->AddClusterAssoc(CTL_BILLCFG_DEFICITSUBST, 2, PPBillConfig::gsmNone);
	dlg->SetClusterData(CTL_BILLCFG_DEFICITSUBST, cfg.GoodsSubstMethod);
	/*
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_BILLCFG_FILESFOLD, CTL_BILLCFG_FILESFOLDER, GRP_FILESFOLD, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
	dlg->setCtrlString(CTL_BILLCFG_FILESFOLDER, cfg.AddFilesFolder);
	*/
	dlg->Tdcmp.Method = cfg.TDisCalcMethod;
	dlg->Tdcmp.Prec   = cfg.TDisCalcPrec;
	dlg->Tdcmp.ThresholdDate = cfg.SwitchedTDisCalcMethodDate;
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		long   temp_long = 0;
		dlg->getCtrlData(CTL_BILLCFG_OPPRFX,  cfg.OpCodePrfx);
		dlg->getCtrlData(CTL_BILLCFG_SNRCNTR, &cfg.SnrCounter);
		cfg.SnrCounter = (cfg.SnrCounter <= 0) ? 1 : cfg.SnrCounter;
		sn_cntr.Head.Counter = cfg.SnrCounter;
		dlg->getCtrlData(CTL_BILLCFG_CLPRFX,       cfg.ClCodePrfx);
		dlg->getCtrlData(CTLSEL_BILLCFG_CLCODEREG, &cfg.ClCodeRegTypeID);
		//dlg->getCtrlData(CTLSEL_BILLCFG_CASHNODE,  &cfg.CashNodeID);
		dlg->getCtrlData(CTLSEL_BILLCFG_INITST,    &cfg.InitStatusID);
		//dlg->getCtrlData(CTLSEL_BILLCFG_CREGTYP,   &cfg.ContractRegTypeID); // @v8.4.0
		//dlg->getCtrlData(CTLSEL_BILLCFG_MNFLTAG,   &cfg.MnfCountryLotTagID); // @v8.4.11
		dlg->getCtrlData(CTL_BILLCFG_SNTEMPLT,     sn_buf);
		dlg->getCtrlData(CTL_BILLCFG_INVSNTEMPLT,  invsn_buf);
		STRNSCPY(sn_cntr.Head.CodeTemplate, sn_buf);
		STRNSCPY(cfg.InvSnTemplt, invsn_buf);
		dlg->getCtrlData(CTL_BILLCFG_UNIQSNSFX, cfg.UniqSerialSfx);
		dlg->GetClusterData(CTL_BILLCFG_FLAGS,  &cfg.Flags);
		dlg->GetClusterData(CTL_BILLCFG_FLAGS2, &cfg.Flags);
		dlg->GetClusterData(CTL_BILLCFG_SHOWADDFLD, &cfg.Flags);
		dlg->GetClusterData(CTL_BILLCFG_DEFICITSUBST, &temp_long);
		cfg.GoodsSubstMethod = (int16)temp_long;
		cfg.TDisCalcMethod = (uint8)dlg->Tdcmp.Method;
		cfg.TDisCalcPrec   = (uint8)dlg->Tdcmp.Prec;
		cfg.SwitchedTDisCalcMethodDate = dlg->Tdcmp.ThresholdDate;
		//dlg->getCtrlString(CTL_BILLCFG_FILESFOLDER, cfg.AddFilesFolder);
		if(cfg.AddFilesFolder.Len() == 0 || pathValid(cfg.AddFilesFolder, 1))
			valid_data = 1;
		else
			PPError(PPERR_DIRNOTEXISTS, cfg.AddFilesFolder);
	}
	if(r == cmOK) {
		THROW(PPObjBill_WriteConfig(&cfg, &sn_cntr, 1));
		BillObj->Cfg = cfg;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int PPObjBill::GetComplete(PPID lotID, long flags, CompleteArray * pList)
{
	int    ok = -1;
	int    r;
	ReceiptTbl::Rec org_lot_rec;
	PPID   org_lot_id = 0;
	SString serial;
	PPBillPacket pack;
	if(flags & gcfGatherSources) {
		if(trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, 0, &org_lot_rec) > 0) {
			PPIDArray lot_list;
			pList->LotID  = org_lot_id;
			pList->BillID = org_lot_rec.BillID;
			trfr->Rcpt.GatherChilds(org_lot_id, &lot_list, 0, 0);
			lot_list.atInsert(0, &org_lot_id);
			for(uint i = 0; i < lot_list.getCount(); i++) {
				TransferTbl::Rec trfr_rec;
				PPID   lot_id = lot_list.at(i);
				for(DateIter di; trfr->EnumByLot(lot_id, &di, &trfr_rec) > 0;) {
					if(trfr_rec.Flags & PPTFR_MODIF && (trfr_rec.Flags & (PPTFR_PLUS|PPTFR_REVAL))) {
						THROW(ExtractPacketWithFlags(trfr_rec.BillID, &pack, BPLD_FORCESERIALS) > 0);
						THROW(r = pack.GetComplete(lot_id, pList));
						if(ok < 0 && r > 0)
							ok = 1;
					}
				}
			}
		}
	}
	if(flags & gcfGatherBranches) {
		TransferTbl::Rec trfr_rec;
		for(DateIter di; trfr->EnumByLot(lotID, &di, &trfr_rec) > 0;) {
			if(trfr_rec.Flags & PPTFR_MODIF && (trfr_rec.Flags & PPTFR_MINUS)) {
				PPTransferItem * p_ti = 0;
				THROW(ExtractPacketWithFlags(trfr_rec.BillID, &pack, BPLD_FORCESERIALS) > 0);
				for(uint p = 0; pack.EnumTItems(&p, &p_ti);) {
					if(p_ti->Flags & PPTFR_PLUS) {
						CompleteItem item;
						// @v10.8.4 @ctr MEMSZERO(item);
						item.Flags  |= CompleteItem::fBranch;
						item.LotID   = p_ti->LotID;
						item.GoodsID = p_ti->GoodsID;
						item.Dt      = p_ti->Date;
						item.Expiry  = p_ti->Expiry;
						item.ArID    = pack.Rec.Object;
						item.Qtty    = fabs(p_ti->Quantity_);
						item.Cost    = p_ti->Cost;
						item.Price   = p_ti->Price;
						pack.LTagL.GetNumber(PPTAG_LOT_SN, p-1, serial);
						STRNSCPY(item.Serial, serial);
						THROW_SL(pList->insert(&item));
						if(p_ti->Flags & PPTFR_RECEIPT && p_ti->LotID) {
							THROW(GetComplete(p_ti->LotID, gcfGatherBranches, pList)); // @recursion
						}
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

// AHTOXA {
int PPObjBill::GetDeficitList(const DateRange * pPeriod, const PPIDArray * pLocList, RAssocArray * pAry)
{
	int    ok = 1;
	PPID   prmr_id = 0;
	LAssocArray pool_memb_ary;
	RAssocArray ary;
	CpTransfTbl::Key0 k0;
	THROW(P_Tbl->GetPoolList(PPASS_CSDBILLPOOL, &pool_memb_ary));
	for(uint i = 0; i < pool_memb_ary.getCount(); i++) {
		BillTbl::Rec b_rec;
		const PPID bill_id = pool_memb_ary.at(i).Val;
		if(Search(bill_id, &b_rec) > 0) {
			if((!pPeriod || pPeriod->CheckDate(b_rec.Dt)) && (!pLocList || pLocList->lsearch(b_rec.LocID))) {
				BExtQuery q(P_CpTrfr, 0);
				q.select(P_CpTrfr->GoodsID, P_CpTrfr->Qtty, 0).where(P_CpTrfr->BillID == bill_id);
				MEMSZERO(k0);
				k0.BillID = bill_id;
				for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;)
					THROW_SL(ary.Add(P_CpTrfr->data.GoodsID, fabs(P_CpTrfr->data.Qtty), 1));
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pAry, ary);
	return ok;
}
// } AHTOXA

DraftRcptItem::DraftRcptItem() : GoodsID(0), LocID(0), Qtty(0.0)
{
}

IMPL_CMPFUNC(DraftRcptItem, _i1, _i2)
{
	int    r = cmp_long(static_cast<const DraftRcptItem *>(_i1)->GoodsID, static_cast<const DraftRcptItem *>(_i2)->GoodsID);
	return NZOR(r, cmp_long(static_cast<const DraftRcptItem *>(_i1)->LocID, static_cast<const DraftRcptItem *>(_i2)->LocID));
}

int PPObjBill::GetDraftRcptList(const DateRange * pPeriod, const PPIDArray * pLocList, DraftRcptArray * pList)
{
	int    ok = 1;
	const  PPID draft_op_id = DS.GetTLA().Cc.DraftRcptOp;
	CALLPTRMEMB(pList, clear());
	if(draft_op_id) {
		BillTbl::Key2 k2;
		DBQ  * dbq = 0;
		BExtQuery q_b(P_Tbl, 2);
		dbq = ppcheckfiltid(dbq, P_Tbl->OpID, draft_op_id);
		dbq = & daterange(P_Tbl->Dt, pPeriod);
		q_b.select(P_Tbl->ID, P_Tbl->Dt, P_Tbl->LocID, P_Tbl->Flags, 0).where(*dbq);
		MEMSZERO(k2);
		k2.OpID = draft_op_id;
		k2.Dt   = pPeriod ? pPeriod->low : ZERODATE;
		for(q_b.initIteration(0, &k2, spGt); q_b.nextIteration() > 0;) {
			if(!(P_Tbl->data.Flags & BILLF_CLOSEDORDER) && (!pPeriod || pPeriod->CheckDate(P_Tbl->data.Dt)) &&
				(!pLocList || pLocList->lsearch(P_Tbl->data.LocID) > 0)) {
				const PPID bill_id = P_Tbl->data.ID;
				TransferTbl::Key0 k0;
				BExtQuery q(P_CpTrfr, 0);
				q.select(P_CpTrfr->GoodsID, P_CpTrfr->Qtty, 0).where(P_CpTrfr->BillID == bill_id);
				MEMSZERO(k0);
				k0.BillID = bill_id;
				for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;) {
					if(pList) {
						DraftRcptItem item;
						item.GoodsID = P_CpTrfr->data.GoodsID;
						item.LocID   = 0;
						item.Qtty    = P_CpTrfr->data.Qtty;
						for(uint i = 0; i < 2; i++) {
							uint p = 0;
							if(pList->lsearch(&item, &p, PTR_CMPFUNC(DraftRcptItem)) > 0)
								pList->at(p).Qtty += item.Qtty;
							else
								THROW_SL(pList->insert(&item));
							item.LocID = P_Tbl->data.LocID;
							if(item.LocID == 0)
								break;
						}
					}
				}
			}
		}
		CALLPTRMEMB(pList, sort(PTR_CMPFUNC(DraftRcptItem)));
	}
	else
		ok = -1;
	CATCHZOK
    return ok;
}

int PPObjBill::CalcDraftTransitRest(PPID restOpID, PPID orderOpID, PPID goodsID, PPID locID, long flags, double * pRest, LDATE * pDt)
{
	int    ok = -1;
	double rest = 0.0;
	LDATE  dt;
	struct CpEntry {
		LDATE  Dt;      // @anchor
		int32  RByBill; // @anchor
		PPID   BillID;
		PPID   OpID;
		double Qtty;
		double Cost;
		double Price;
	};
	SArray cp_list(sizeof(CpEntry));
	CpTransfTbl::Key1 k1;
	k1.GoodsID = labs(goodsID);
	k1.LocID = locID;
	BExtQuery q(P_CpTrfr, 1);
	q.selectAll().where(P_CpTrfr->GoodsID == labs(goodsID) && P_CpTrfr->LocID == locID);
	for(q.initIteration(0, &k1, spEq); q.nextIteration() > 0;) {
		CpTransfTbl::Rec cpt_rec;
		BillTbl::Rec bill_rec;
		P_CpTrfr->copyBufTo(&cpt_rec);
		if(Fetch(cpt_rec.BillID, &bill_rec) > 0) {
			CpEntry entry;
			MEMSZERO(entry);
			entry.BillID = cpt_rec.BillID;
			entry.Dt = bill_rec.Dt;
			entry.RByBill = cpt_rec.RByBill;
			entry.OpID = bill_rec.OpID;
			entry.Qtty = cpt_rec.Qtty;
			entry.Cost = cpt_rec.Cost;
			entry.Price = cpt_rec.Price;
			cp_list.insert(&entry);
		}
	}
	{
		uint cpc = cp_list.getCount();
		if(cpc) {
			cp_list.sort(PTR_CMPFUNC(_2long));
			do {
				const CpEntry * p_entry = static_cast<const CpEntry *>(cp_list.at(--cpc));
				if(p_entry->OpID == orderOpID) {
					rest -= fabs(p_entry->Qtty);
				}
				else if(p_entry->OpID == restOpID) {
					//
					// Последний документ с остатками считается последней фиксацией ВСЕх остатков по складу.
					// Следовательно, мы сразу покидаем цикл (заказы, принятые до записи с остатками не учитываем ???).
					//
					rest += p_entry->Qtty;
					dt = p_entry->Dt;
					ok = 1;
					break;
				}
			} while(cpc);
		}
	}
	ASSIGN_PTR(pRest, rest);
	ASSIGN_PTR(pDt, dt);
	return ok;
}

int PPObjBill::MoveLotTagsFromDraftBillToWrOffBill(PPID billID, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	SString bill_text;
	THROW(Search(billID, &bill_rec) > 0);
	if(!(bill_rec.Flags & BILLF_WRITEDOFF)) {
		PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, bill_text);
		if(pLogger)
			pLogger->LogMsgCode(mfInfo, PPINF_DRAFTNOTWROFF, bill_text);
		else
			PPMessage(mfInfo|mfOK, PPINF_DRAFTNOTWROFF, bill_text);
	}
	else {
		PPIDArray wroff_bill_list;
		BillTbl::Rec wroff_bill_rec;
		for(DateIter diter; P_Tbl->EnumLinks(bill_rec.ID, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;)
			wroff_bill_list.add(wroff_bill_rec.ID);
		if(wroff_bill_list.getCount() == 1) {
			SString temp_buf;
			PPBillPacket _this_bp;
			PPBillPacket _link_bp;
			const PPID   _link_id = wroff_bill_list.get(0);
			PPLotExtCodeContainer::MarkSet _this_lxc_set;
			PPLotExtCodeContainer::MarkSet _link_lxc_set;
			int    do_update = 0;
			THROW(ExtractPacketWithFlags(billID, &_this_bp, BPLD_FORCESERIALS) > 0);
			THROW(ExtractPacketWithFlags(_link_id, &_link_bp, BPLD_FORCESERIALS) > 0);
			PPObjBill::MakeCodeString(&_link_bp.Rec, PPObjBill::mcsAddOpName, bill_text);
			for(uint tbpi = 0; tbpi < _this_bp.GetTCount(); tbpi++) {
				const PPTransferItem & r_ti = _this_bp.ConstTI(tbpi);
				if(r_ti.RByBill > 0) {
					const ObjTagList * p_tl = _this_bp.LTagL.Get(tbpi);
					uint  _lp = 0;
					if(p_tl && p_tl->GetCount() && _link_bp.SearchTI(r_ti.RByBill, &_lp)) {
						int    do_update_local = 0;
						const  PPTransferItem & r_link_ti = _link_bp.ConstTI(_lp);
						ObjTagList * p_link_tl = _link_bp.LTagL.Get(_lp);
						ObjTagList _link_tl;
						RVALUEPTR(_link_tl, p_link_tl);
						for(uint tli = 0; tli < p_tl->GetCount(); tli++) {
							const ObjTagItem * p_tag = p_tl->GetItemByPos(tli);
							if(p_tag && !p_tag->IsZeroVal()) {
								const PPID tag_id = p_tag->TagID;
								if(tag_id) {
									const ObjTagItem * p_ex_link_tag = _link_tl.GetItem(tag_id);
									if(!p_ex_link_tag || *p_ex_link_tag != *p_tag) {
										_link_tl.PutItem(p_tag->TagID, p_tag);
										do_update_local = 1;
									}
								}
							}
						}
						if(do_update_local) {
							_link_bp.LTagL.Set(_lp, &_link_tl);
							do_update = 1;
						}
					}
					if(_this_bp.XcL.Get(tbpi+1, 0, _this_lxc_set) > 0) {
						_link_bp.XcL.Get(_lp+1, 0, _link_lxc_set);
						if(_link_lxc_set.GetCount() == 0) {
							PPLotExtCodeContainer::MarkSet::Entry lxentry;
							for(uint thislxidx = 0; thislxidx < _this_lxc_set.GetCount(); thislxidx++) {
								if(_this_lxc_set.GetByIdx(thislxidx, lxentry)) {
									_link_bp.XcL.Add(_lp+1, lxentry.BoxID, static_cast<int16>(lxentry.Flags), lxentry.Num, 0);
									do_update = 1;
								}
							}
						}
					}
				}
			}
			if(do_update) {
				THROW(UpdatePacket(&_link_bp, use_ta));
				if(pLogger)
					pLogger->LogMsgCode(mfInfo, PPINF_TAGSINWROFFBILLUPD, bill_text);
				else
					PPMessage(mfInfo|mfOK, PPINF_TAGSINWROFFBILLUPD, bill_text);
				ok = 1;
			}
			else {
				if(pLogger)
					pLogger->LogMsgCode(mfInfo, PPINF_TAGSINWROFFBILLNUPD, bill_text);
				else
					PPMessage(mfInfo|mfOK, PPINF_TAGSINWROFFBILLNUPD, bill_text);
			}
		}
		else if(wroff_bill_list.getCount() > 1) {
			; // Не понятно что делать - не делаем ничего
			PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, bill_text);
			if(pLogger)
				pLogger->LogMsgCode(mfInfo, PPINF_DRAFTHASGT1WROFFBILL, bill_text);
			else
				PPMessage(mfInfo|mfOK, PPINF_DRAFTHASGT1WROFFBILL, bill_text);
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjBill::SearchQuoteReqSeq(const DateRange * pPeriod, TSArray <QuoteReqLink> & rList)
{
	int    ok = -1;
	PPIDArray bill_id_list;
	PPIDArray op_list;
	PPOprKind op_rec;
	for(PPID op_id = 0; EnumOperations(PPOPT_DRAFTQUOTREQ, &op_id, &op_rec) > 0;) {
		if(op_rec.LinkOpID)
			op_list.add(op_id);
	}
	for(uint opidx = 0; opidx < op_list.getCount(); opidx++) {
		const PPID op_id = op_list.get(opidx);
		BillTbl::Rec bill_rec;
		for(SEnum en = P_Tbl->EnumByOp(op_id, pPeriod, 0); en.Next(&bill_rec) > 0;) {
			bill_id_list.add(bill_rec.ID);
		}
	}
	if(bill_id_list.getCount()) {
		bill_id_list.sortAndUndup();
		//for(uint bidx = 0; bidx < bill_id_list.getCount(); )
		CpTransfTbl::Rec cpt_rec;
		CpTransfTbl::Key0 k0;
		CpTransfCore * p_cpt = P_CpTrfr;
		const PPID bill_id_beg = bill_id_list.get(0);
		const PPID bill_id_end = bill_id_list.getLast();
		MEMSZERO(k0);
		k0.BillID = bill_id_beg;
		BExtQuery q(p_cpt, 0);
		q.select(p_cpt->BillID, p_cpt->RByBill, p_cpt->Flags, p_cpt->Cost, p_cpt->Qtty, p_cpt->Tail, 0L).
			where(p_cpt->BillID >= bill_id_beg && p_cpt->BillID <= bill_id_end);
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
			p_cpt->copyBufTo(&cpt_rec);
			if(bill_id_list.bsearch(cpt_rec.BillID)) {
				CpTrfrExt cpext;
				CpTransfCore::GetExt__(cpt_rec, &cpext);
				if(cpext.LinkBillID > 0 && cpext.LinkRbb > 0) {
					QuoteReqLink new_item;
					MEMSZERO(new_item);
					new_item.LeadBillID = cpext.LinkBillID;
					new_item.LeadRbb = cpext.LinkRbb;
					new_item.SeqBillID = cpt_rec.BillID;
					new_item.SeqRbb = cpt_rec.RByBill;
					new_item.AckStatus = cpext.QrSeqAckStatus;
					if(new_item.AckStatus == 1) {
						new_item.AckCost = cpt_rec.Cost;
						new_item.AckQtty = cpt_rec.Qtty;
					}
					rList.insert(&new_item);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

PPID PPObjBill::GetSupplAgent(PPID lotID)
{
	PPID   suppl_id = 1000000;
	ReceiptTbl::Rec org_rec;
	if(trfr->Rcpt.SearchOrigin(lotID, 0, 0, &org_rec) > 0) {
		PPBillExt b_ext;
		P_Tbl->GetExtraData(org_rec.BillID, &b_ext);
		suppl_id = b_ext.AgentID;
	}
	return suppl_id;
}

SString & PPObjBill::MakeLotText(const ReceiptTbl::Rec * pLotRec, long fmt, SString & rBuf)
{
	BillTbl::Rec bill_rec;
	SString temp_buf;
	rBuf = "LOT";
	if(Search(pLotRec->BillID, &bill_rec) > 0)
		MakeCodeString(&bill_rec, 0, temp_buf);
	else
		ideqvalstr(pLotRec->BillID, temp_buf).CatDiv(':', 1).Cat(pLotRec->Dt);
	temp_buf.CatDiv(':', 1).Cat(pLotRec->OprNo);
	rBuf.Space().Cat(temp_buf);
	if(fmt & PPObjBill::ltfGoodsName) {
		// @v9.5.5 GetGoodsName(pLotRec->GoodsID, temp_buf);
		GObj.FetchNameR(pLotRec->GoodsID, temp_buf); // @v9.5.5
		rBuf.CatDiv(':', 1).Cat(temp_buf);
	}
	if(fmt & PPObjBill::ltfLocName) {
		GetLocationName(pLotRec->LocID, temp_buf);
		rBuf.CatDiv(':', 1).Cat(temp_buf);
	}
	return rBuf;
}

int PPObjBill::IsInfluenceToStock(PPID billID, PPID locID)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	if(Search(billID, &bill_rec) > 0) {
		PPID   op_type_id = GetOpType(bill_rec.OpID);
		if(oneof6(op_type_id, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_GOODSREVAL, PPOPT_GOODSRETURN, PPOPT_GOODSMODIF, PPOPT_GOODSACK)) {
			if(!locID || bill_rec.LocID == locID)
				ok = 1;
			else if(IsIntrExpndOp(bill_rec.OpID)) {
				if(locID == PPObjLocation::ObjToWarehouse(bill_rec.Object)) // @v10.2.11 @fix (bill_rec.LocID ==)-->(locID ==)
					ok = 1;
			}
		}
	}
	return ok;
}

int PPObjBill::GetGoodsListByUpdatedBills(PPID locID, const LDATETIME & rDtm, PPIDArray & rGoodsList, PPIDArray * pBillList)
{
	int    ok = 1;
	CALLPTRMEMB(pBillList, clear());
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj) {
		PPIDArray acn_list, bill_list;
		acn_list.addzlist(PPACN_OBJADD, PPACN_OBJUPD, PPACN_TURNBILL, PPACN_UPDBILL, 0L);
		THROW(p_sj->GetObjListByEventSince(PPOBJ_BILL, &acn_list, rDtm, bill_list));
		{
			PPIDArray goods_list;
			for(uint i = 0; i < bill_list.getCount(); i++) {
				const PPID bill_id = bill_list.at(i);
				int r = IsInfluenceToStock(bill_id, locID);
				THROW(r);
				if(r > 0) {
					goods_list.clear();
					if(trfr->CalcBillTotal(bill_id, 0, &goods_list) > 0 && goods_list.getCount()) {
						THROW(rGoodsList.add(&goods_list));
						CALLPTRMEMB(pBillList, add(bill_id));
					}
					goods_list.clear();
				}
			}
			rGoodsList.sortAndUndup();
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int PPObjBill::Helper_PutBillToMrpTab(PPID billID, MrpTabPacket * pMrpPack, const PPDraftOpEx * pWrOffParam, int use_ta)
{
	int    ok = -1;
	CpTrfrExt cpext;
	PPTransferItem src_ti, ti;
	BillTbl::Rec bill_rec;
	PPObjMrpTab mrp_obj;
	PPID   mrp_tab_id = 0, intr_tab_id = 0;
	const  PPID wroff_op_type_id = pWrOffParam ? GetOpType(pWrOffParam->WrOffOpID) : 0; // @v10.7.11 @fix (pWrOffParam ?)
	{
		THROW(Search(billID, &bill_rec) > 0);
		THROW(mrp_obj.GetTabID(pMrpPack, bill_rec.LocID, bill_rec.Dt, &mrp_tab_id, use_ta));
		{
			const PPID op_type_id = GetOpType(bill_rec.OpID);
			switch(op_type_id) {
				case PPOPT_DRAFTEXPEND:
					if(oneof3(wroff_op_type_id, 0, PPOPT_GOODSEXPEND, PPOPT_DRAFTRECEIPT)) { // @v8.6.2 PPOPT_DRAFTRECEIPT
						if(IsIntrExpndOp(pWrOffParam->WrOffOpID)) {
							const PPID dest_loc_id = PPObjLocation::ObjToWarehouse(bill_rec.Object);
							if(dest_loc_id)
								THROW(mrp_obj.GetTabID(pMrpPack, dest_loc_id, bill_rec.Dt, &intr_tab_id, use_ta));
						}
						for(int rbybill = 0; P_CpTrfr->EnumItems(billID, &rbybill, &src_ti, &cpext) > 0;) {
							const double req_qtty = fabs(src_ti.Quantity_);
							const double req_price = src_ti.NetPrice() * req_qtty;
							THROW(mrp_obj.AddIndep(pMrpPack, mrp_tab_id, src_ti.GoodsID, req_qtty, req_price, 0/*IgnoreRest*/));
							// Инициализация приходов на склад-приемник
							if(intr_tab_id)
								THROW(mrp_obj.AddIndep(pMrpPack, intr_tab_id, src_ti.GoodsID, -req_qtty, -req_price, 0/*IgnoreRest*/));
						}
						ok = 1;
					}
					else if(wroff_op_type_id == PPOPT_GOODSMODIF) {
						; // Не обрабатывается //
					}
					break;
				case PPOPT_DRAFTRECEIPT:
					if(wroff_op_type_id == PPOPT_GOODSRECEIPT) {
						for(int rbybill = 0; P_CpTrfr->EnumItems(billID, &rbybill, &src_ti, &cpext) > 0;) {
							const double req_qtty = fabs(src_ti.Quantity_);
							THROW(mrp_obj.AddIndep(pMrpPack, mrp_tab_id, src_ti.GoodsID, req_qtty, src_ti.NetPrice() * req_qtty, 1/*IgnoreRest*/));
						}
						ok = 1;
					}
					else if(wroff_op_type_id == PPOPT_GOODSMODIF) {
						for(int rbybill = 0; P_CpTrfr->EnumItems(billID, &rbybill, &src_ti, &cpext) > 0;) {
							const double req_qtty = fabs(src_ti.Quantity_);
							THROW(mrp_obj.AddIndep(pMrpPack, mrp_tab_id, src_ti.GoodsID, req_qtty, src_ti.NetPrice() * req_qtty, 1/*IgnoreRest*/));
						}
						ok = 1;
					}
					break;
				case PPOPT_GOODSORDER:
					{
						PPTransferItem ti;
						for(int rbybill = 0; trfr->EnumItems(billID, &rbybill, &ti) > 0;) {
							double req_qtty = ti.Quantity_;
							if(ti.LotID)
								trfr->GetRest(ti.LotID, MAXDATE, &req_qtty);
							THROW(mrp_obj.AddIndep(pMrpPack, mrp_tab_id, labs(ti.GoodsID), fabs(req_qtty), ti.NetPrice() * req_qtty, 0/*IgnoreRest*/));
						}
						ok = 1;
					}
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::CreateMrpTab(const PPIDArray * pList, MrpTabPacket * pMrpPack, PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	uint   i;
	const  uint list_count = SVectorBase::GetCount(pList);
	if(list_count) {
		SString fmt_buf, msg_buf, bill_name; // for logger
		PPObjMrpTab mrp_obj;
		IterCounter cntr;
		cntr.Init(list_count);
		PPLoadText(PPTXT_MRPTABBUILDING_BYBILL, fmt_buf);
		for(i = 0; i < list_count; i++) {
			const PPID bill_id = pList->at(i);
			BillTbl::Rec bill_rec;
			if(Search(bill_id, &bill_rec) > 0) {
				const PPID op_type_id = GetOpType(bill_rec.OpID);
				const int  is_draft = IsDraftOp(bill_rec.OpID);
				PPWaitPercent(cntr.Increment(), GetNamePtr()); // @! GetNamePtr вызывается сразу после Search
				if(op_type_id == PPOPT_GOODSORDER || (is_draft && !(bill_rec.Flags & BILLF_WRITEDOFF))) {
					PPOprKindPacket op_pack;
					THROW(P_OpObj->GetPacket(bill_rec.OpID, &op_pack) > 0);
					if(is_draft)
						THROW_PP(op_pack.P_DraftData && (op_pack.P_DraftData->WrOffOpID || op_pack.P_DraftData->WrOffComplOpID), PPERR_UNDEFWROFFOP);
					MakeCodeString(&bill_rec, 1, bill_name);
					msg_buf.Printf(fmt_buf, bill_name.cptr());
					CALLPTRMEMB(pLogger, Log(msg_buf));
					PPWaitMsg(msg_buf);
					THROW(ok = Helper_PutBillToMrpTab(bill_id, pMrpPack, op_pack.P_DraftData, use_ta));
				}
			}
		}
		if(pMrpPack->getCount()) {
			PPLoadText(PPTXT_MRPTABFINISHING, msg_buf);
			CALLPTRMEMB(pLogger, Log(msg_buf));
			PPWaitMsg(msg_buf);
			THROW(mrp_obj.FinishPacket(pMrpPack, 0, use_ta));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int PPObjBill::IsAssetLot(PPID lotID)
{
	ReceiptTbl::Rec lot_rec;
	return (trfr->Rcpt.Search(lotID, &lot_rec) > 0 && GObj.CheckFlag(lot_rec.GoodsID, GF_ASSETS) > 0) ? 1 : -1;
}

int PPObjBill::MakeAssetCard(PPID lotID, AssetCard * pCard)
{
	int    ok = 1;
	memzero(pCard, sizeof(*pCard));
	PPID   org_lot_id = 0;
	ReceiptTbl::Rec lot_rec, org_lot_rec;
	if(trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, &lot_rec, &org_lot_rec) > 0) {
		const  LDATE  lot_date = org_lot_rec.Dt;
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(lot_rec.GoodsID, &goods_rec) > 0 && goods_rec.Flags & GF_ASSETS) {
			//
			// Расчет поправки на НДС {
			//
			double tax_factor = 1.0;
			PPID   tax_grp_id = lot_rec.InTaxGrpID ? lot_rec.InTaxGrpID : goods_rec.TaxGrpID;
			GTaxVect vect;
			PPGoodsTaxEntry gt;
			int    vat_free = IsLotVATFree(lot_rec);
			GObj.MultTaxFactor(goods_rec.ID, &tax_factor);
			int    adj_vat = BIN(GObj.GTxObj.Fetch(tax_grp_id, lot_date, 0L, &gt) > 0);
			// }
			pCard->LotID = lotID;
			//
			// Определяем балансовый счет основных средств
			//
			AcctID accid;
			PPID   acc_sheet_id = 0;
			if(atobj->ConvertAcct(&CConfig.AssetAcct, 0 /*@curID*/, &accid, &acc_sheet_id) > 0)
				pCard->AssetAcctID = accid;

			int    op_code = 0;
			PPID   lot_id = lotID;
			TransferTbl::Rec rec;
			for(DateIter iter; trfr->EnumAssetOp(&lot_id, &iter, &op_code, &rec) > 0;) {
				if(oneof2(op_code, ASSTOPC_RCPT, ASSTOPC_RCPTEXPL)) {
					pCard->OrgLotID = lot_id;
					pCard->OrgCost  = TR5(rec.Cost);
					pCard->OrgPrice = TR5(rec.Price);
					if(adj_vat) {
						if(lot_rec.Flags & LOTF_COSTWOVAT) {
							GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &pCard->OrgCost, 1, vat_free);
							GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &pCard->OrgPrice, 1, vat_free);
						}
						long   amt_fl = ~GTAXVF_SALESTAX;
						long   excl_fl = (vat_free > 0) ? GTAXVF_VAT : 0;
						vect.Calc_(&gt, pCard->OrgCost, tax_factor, amt_fl, excl_fl);
						pCard->OrgCost -= vect.GetValue(GTAXVF_VAT);
						vect.Calc_(&gt, pCard->OrgPrice, tax_factor, amt_fl, excl_fl);
						pCard->OrgPrice -= vect.GetValue(GTAXVF_VAT);
					}
				}
				if(oneof2(op_code, ASSTOPC_RCPTEXPL, ASSTOPC_EXPL)) {
					pCard->ExplBillID = rec.BillID;
				}
				if(oneof6(op_code, ASSTOPC_MOV, ASSTOPC_RCPT, ASSTOPC_RCPTEXPL, ASSTOPC_EXPEND, ASSTOPC_EXPL, ASSTOPC_EXPLOUT)) {
					if(pCard->P_MovList == 0)
						THROW_MEM(pCard->P_MovList = new SVector(sizeof(AssetCard::MovItem))); // @v10.7.11 SArray-->SVector
					AssetCard::MovItem item;
					item.BillID    = rec.BillID;
					item.LotID     = rec.LotID;
					item.DestLocID = rec.LocID;
					item.Price     = TR5(rec.Price);
					if(adj_vat) {
						if(lot_rec.Flags & LOTF_COSTWOVAT)
							GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &item.Price, 1, vat_free);
						long   amt_fl = ~GTAXVF_SALESTAX;
						long   excl_fl = (vat_free > 0) ? GTAXVF_VAT : 0;
						vect.Calc_(&gt, item.Price, tax_factor, amt_fl, excl_fl);
						item.Price -= vect.GetValue(GTAXVF_VAT);
					}
					THROW_SL(pCard->P_MovList->insert(&item));
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int PPObjBill::Helper_GetShipmentByLot(PPID lotID, const DateRange * pPeriod,
	const ObjIdListFilt & rOpList, long flags, double * pShipment, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	double shipment = 0.0;
	TransferTbl::Rec rec, mirror_rec;
	if(pRecurTrace && pRecurTrace->addUnique(lotID) < 0)
		ok = -1; // Засекли рекурсию
	else {
		for(DateIter di; trfr->EnumByLot(lotID, &di, &rec) > 0;) {
			BillTbl::Rec bill_rec;
			if(Search(rec.BillID, &bill_rec) > 0) {
				if(IsIntrExpndOp(bill_rec.OpID) && !(flags & gsporIntrAsShipment)) {
					if(trfr->SearchMirror(rec.Dt, rec.OprNo, &mirror_rec) > 0 && mirror_rec.LotID) {
						double s = 0.0;
						Helper_GetShipmentByLot(mirror_rec.LotID, pPeriod, rOpList, flags, &s, pRecurTrace); // @recursion
						shipment += s;
					}
				}
				else if(rOpList.CheckID(bill_rec.OpID)) {
					if(flags & gsporPayment && CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT)) {
						double s = 0.0;
						P_Tbl->CalcPayment(bill_rec.ID, 1, pPeriod, bill_rec.CurID, &s);
						shipment += fdivnz(fabs(rec.Cost * rec.Quantity) * s, bill_rec.Amount);
					}
					else if(!pPeriod || pPeriod->CheckDate(bill_rec.Dt)) {
						shipment += fabs(rec.Cost * rec.Quantity);
					}
				}
			}
		}
	}
	ASSIGN_PTR(pShipment, shipment);
	return ok;
}

int PPObjBill::GetShippedPartOfReceipt(PPID rcptBillID, const DateRange * pPeriod, const ObjIdListFilt & rOpList, long flags, double * pPart)
{
	int    ok = -1;
	double part = 0.0;
	double shipment = 0.0;
	PPBillPacket pack;
	if(ExtractPacket(rcptBillID, &pack) > 0) {
		PPIDArray recur_trace;
		PPTransferItem * p_ti;
		for(uint i = 0; pack.EnumTItems(&i, &p_ti);) {
			if(p_ti->Flags & PPTFR_RECEIPT && p_ti->LotID) {
				double s = 0.0;
				THROW(Helper_GetShipmentByLot(p_ti->LotID, pPeriod, rOpList, flags, &s, &recur_trace));
				shipment += s;
				ok = 1;
			}
		}
		part = fdivnz(shipment, pack.Rec.Amount);
	}
	CATCHZOK
	ASSIGN_PTR(pPart, part);
	return ok;
}
//
//
//
int PPObjBill::CalcGoodsSaldo(PPID goodsID, PPID arID, PPID dlvrLocID, const DateRange * pPeriod, long endOprNo, double * pSaldoQtty, double * pSaldoAmt)
{
	int    ok = 1;
	double qt = 0.0, am = 0.0;
	DBQ  * t_dbq = 0;
	PPFreight freight;
	TransferTbl::Key3 tk3;
	BExtQuery tq(trfr, 3, 64);
	MEMSZERO(tk3);
	tk3.GoodsID = goodsID;
	tk3.Dt      = pPeriod ? pPeriod->low : ZERODATE;
	if(!pPeriod || !pPeriod->upp)
		endOprNo = 0;
	t_dbq = &(trfr->GoodsID == goodsID && daterange(trfr->Dt, pPeriod));
	tq.select(trfr->Dt, trfr->OprNo, trfr->BillID, trfr->Quantity, trfr->Price, trfr->Discount, 0L).where(*t_dbq);
	for(tq.initIteration(0, &tk3, spGe); tq.nextIteration() > 0;) {
		BillTbl::Rec bill_rec;
		if(!endOprNo || trfr->data.Dt < pPeriod->upp || (trfr->data.Dt == pPeriod->upp && trfr->data.OprNo < endOprNo)) {
			if(Fetch(trfr->data.BillID, &bill_rec) > 0 && bill_rec.Object == arID) {
				if(!dlvrLocID || (P_Tbl->GetFreight(bill_rec.ID, &freight) > 0 && freight.DlvrAddrID == dlvrLocID)) {
					qt += trfr->data.Quantity;
					am += (TR5(trfr->data.Price) - TR5(trfr->data.Discount)) * trfr->data.Quantity;
				}
			}
			if(!PPCheckUserBreak()) {
				ok = -1;
				break;
			}
		}
	}
	ASSIGN_PTR(pSaldoQtty, qt);
	ASSIGN_PTR(pSaldoAmt,  am);
	return ok;
}

int PPObjBill::GetGoodsSaldo(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, long oprNo, double * pSaldoQtty, double * pSaldoAmt)
{
	int    ok = 1;
	double qtty = 0.0, amt = 0.0;
	if(goodsID && dt) {
		SETIFZ(P_GsT, new GoodsSaldoCore);
		GoodsDebtTbl::Rec gd_rec;
		double q = 0.0, am = 0.0;
		int    r = 0;
		DateRange period;
		period.SetDate(dt);
		THROW(r = P_GsT->GetLastSaldo(goodsID, arID, dlvrLocID, &period.low, &gd_rec));
		period.low = (r > 0) ? plusdate(period.low, 1) : ZERODATE;
		qtty = gd_rec.SaldoQtty;
		amt  = gd_rec.SaldoAmount;
		THROW(ok = CalcGoodsSaldo(goodsID, arID, dlvrLocID, &period, oprNo, &q, &am));
		qtty += q;
		amt  += am;
	}
	CATCH
		qtty = 0.0;
		amt  = 0.0;
		ok   = 0;
	ENDCATCH
	ASSIGN_PTR(pSaldoQtty, qtty);
	ASSIGN_PTR(pSaldoAmt,  amt);
	return ok;
}

int PPObjBill::SetupImportedPrice(const PPBillPacket * pPack, PPTransferItem * pTi, long flags)
{
	int    result = -1;
	if(pTi->Price <= 0.0) {
		if(oneof2(pPack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
			//
			// Для приходных документов пытаемся назначить цену реализации
			// при помощи механизма расценки
			//
			if(CheckOpFlags(pPack->Rec.OpID, OPKF_NEEDVALUATION) && pTi->Valuation(GetConfig(), 0, 0) > 0)
				result = 3;
		}
		if(pTi->Price < 0.0 || (!(flags & sipfAllowZeroPrice) && pTi->Price == 0.0)) {
			//
			// Если цена реализации (даже после попытки расценки) не определена,
			// то используем правило последнего лота.
			//
			THROW(GetCurGoodsPrice(pTi->GoodsID, pPack->Rec.LocID, GPRET_MOSTRECENT, &pTi->Price, 0));
			if(pTi->Price > 0.0)
				result = 1;
			else {
				//
				// Наконец, если ничего не помогло для определения цены реализации,
				// то приравниваем ее цене поступления.
				//
				pTi->Price = pTi->Cost;
				result = 2;
			}
		}
	}
	CATCH
		result = 0;
	ENDCATCH
	return result;
}
//
//
//
struct AutoCalcPricesParam : public CalcPriceParam {
public:
	explicit AutoCalcPricesParam(int strictPriceValuation) : CalcPriceParam(), StrictPriceValuation(strictPriceValuation), _Action(0), SupplID(0)
	{
	}
	enum {
		_aPrice = 0,
		_aCost,
		_aCostByContract
	};
	int    StrictPriceValuation;
	long   _Action;
	PPID   SupplID;
};

static int AutoCalcSelectQuot(PPObjBill * pBObj, AutoCalcPricesParam * pData)
{
	class AutoCalcSelectQuotDialog : public TDialog {
	public:
		explicit AutoCalcSelectQuotDialog(PPObjBill * pBObj) : TDialog(DLG_SELQUOT3), Data(0), P_BObj(pBObj)
		{
			const PPObjQuotKind::Special sqk(PPObjQuotKind::Special::ctrInitializeWithCache);
			SupplDealQkID = sqk.SupplDealID;
		}
		int    setDTS(const AutoCalcPricesParam * pData)
		{
			Data = *pData;
			PreserveData = *static_cast<const CalcPriceParam *>(pData);
			int    ok = 1;
			AddClusterAssocDef(CTL_SELQUOT2_ACTION, 0, AutoCalcPricesParam::_aPrice);
			AddClusterAssoc(CTL_SELQUOT2_ACTION, 1, AutoCalcPricesParam::_aCost);
			AddClusterAssoc(CTL_SELQUOT2_ACTION, 2, AutoCalcPricesParam::_aCostByContract);
			if(P_BObj->CheckRights(BILLRT_ACCSCOST)) {
				DisableClusterItem(CTL_SELQUOT2_ACTION, 1, 0);
				DisableClusterItem(CTL_SELQUOT2_ACTION, 2, !SupplDealQkID);
				if(!SupplDealQkID && Data._Action == AutoCalcPricesParam::_aCostByContract)
					Data._Action = AutoCalcPricesParam::_aCost;
			}
			else {
				DisableClusterItem(CTL_SELQUOT2_ACTION, 1, 1);
				DisableClusterItem(CTL_SELQUOT2_ACTION, 2, 1);
				if(oneof2(Data._Action, AutoCalcPricesParam::_aCost, AutoCalcPricesParam::_aCostByContract))
					Data._Action = AutoCalcPricesParam::_aPrice;
			}
			SetClusterData(CTL_SELQUOT2_ACTION, Data._Action);
			if(Data._Action == AutoCalcPricesParam::_aCostByContract)
				SetupPPObjCombo(this, CTLSEL_SELQUOT2_KIND, PPOBJ_QUOTKIND, Data.QuotKindID, 0, reinterpret_cast<void *>(QuotKindFilt::fSupplDeal));
			else
				SetupPPObjCombo(this, CTLSEL_SELQUOT2_KIND, PPOBJ_QUOTKIND, Data.QuotKindID, 0);
			setCtrlData(CTL_SELQUOT2_PREC, &Data.RoundPrec);
			AddClusterAssocDef(CTL_SELQUOT2_ROUND,  0, 0);
			AddClusterAssoc(CTL_SELQUOT2_ROUND,  1, -1);
			AddClusterAssoc(CTL_SELQUOT2_ROUND,  2, +1);
			SetClusterData(CTL_SELQUOT2_ROUND, Data.RoundDir);
			AddClusterAssoc(CTL_SELQUOT2_ROUNDVAT, 0, CalcPriceParam::fRoundVat);
			SetClusterData(CTL_SELQUOT2_ROUNDVAT, Data.Flags);
			disableCtrls((Data._Action == AutoCalcPricesParam::_aPrice && Data.StrictPriceValuation),
				CTLSEL_SELQUOT2_KIND, CTL_SELQUOT2_ROUND, CTL_SELQUOT2_PREC, CTL_SELQUOT2_ROUNDVAT, 0);
			return ok;
		}
		int    getDTS(AutoCalcPricesParam * pData)
		{
			int    ok = 1;
			Data._Action = GetClusterData(CTL_SELQUOT2_ACTION);
			getCtrlData(CTLSEL_SELQUOT2_KIND, &Data.QuotKindID);
			if(Data.QuotKindID) {
				getCtrlData(CTL_SELQUOT2_PREC, &Data.RoundPrec);
				Data.Flags = GetClusterData(CTL_SELQUOT2_ROUNDVAT);
				Data.RoundDir = (int16)GetClusterData(CTL_SELQUOT2_ROUND);
				ok = 1;
			}
			else {
				ok = PPErrorByDialog(this, CTLSEL_SELQUOT2_KIND, PPERR_QUOTNOTSEL);
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELQUOT2_ACTION)) {
				Data._Action = GetClusterData(CTL_SELQUOT2_ACTION);
				if(Data._Action == AutoCalcPricesParam::_aCostByContract) {
					SetupPPObjCombo(this, CTLSEL_SELQUOT2_KIND, PPOBJ_QUOTKIND, Data.QuotKindID = SupplDealQkID, 0, reinterpret_cast<void *>(QuotKindFilt::fSupplDeal));
				}
				else {
					if(Data._Action == AutoCalcPricesParam::_aPrice && Data.StrictPriceValuation) {
						Data.QuotKindID = PreserveData.QuotKindID;
						Data.RoundPrec = PreserveData.RoundPrec;
						Data.RoundDir = PreserveData.RoundDir;
						Data.Flags = PreserveData.Flags;

						setCtrlData(CTL_SELQUOT2_PREC, &Data.RoundPrec);
						SetClusterData(CTL_SELQUOT2_ROUND, Data.RoundDir);
						SetClusterData(CTL_SELQUOT2_ROUNDVAT, Data.Flags);
					}
					else {
						Data.QuotKindID = getCtrlLong(CTLSEL_SELQUOT2_KIND);
						if(Data.QuotKindID == SupplDealQkID)
							Data.QuotKindID = 0;
					}
					SETIFZ(Data.QuotKindID, PreserveData.QuotKindID);
					SetupPPObjCombo(this, CTLSEL_SELQUOT2_KIND, PPOBJ_QUOTKIND, Data.QuotKindID, 0);
				}
				disableCtrls((Data._Action == AutoCalcPricesParam::_aPrice && Data.StrictPriceValuation),
					CTLSEL_SELQUOT2_KIND, CTL_SELQUOT2_ROUND, CTL_SELQUOT2_PREC, CTL_SELQUOT2_ROUNDVAT, 0);
				clearEvent(event);
			}
		}
		PPObjBill * P_BObj;
		PPID   SupplDealQkID;
		AutoCalcPricesParam Data;
		CalcPriceParam PreserveData;
	};
	int    ok = -1;
	AutoCalcSelectQuotDialog * dlg = new AutoCalcSelectQuotDialog(pBObj);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pData);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(pData))
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjBill::AutoCalcPrices(PPBillPacket * pPack, int interactive, int * pIsModified)
{
	class NewPricesDialog : public PPListDialog {
	public:
		NewPricesDialog(const PPBillPacket * pPack, RAssocArray * pData, int byCost = 0) :
			PPListDialog(DLG_AUTONEWP, CTL_AUTONEWP_LIST), P_Pack(pPack), ByCost(byCost)
		{
			RVALUEPTR(Data, pData);
			updateList(-1);
		}
		int    getDTS(RAssocArray * pData)
		{
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		virtual int setupList()
		{
			int    ok = -1;
			if(P_Pack) {
				PPTransferItem * p_item = 0;
				SString sub;
				PPObjGoods goods_obj;
				StringSet ss(SLBColumnDelim);
				for(uint i = 0; P_Pack->EnumTItems(&i, &p_item);) {
					double new_price = Data.at(i-1).Val;
					double old_price = ByCost ? p_item->Cost : p_item->Price;
					double diff = new_price-old_price;
					if(new_price > 0.0) {
						ss.clear();
						// @v9.5.5 GetGoodsName(p_item->GoodsID, sub);
						goods_obj.FetchNameR(p_item->GoodsID, sub); // @v9.5.5
						ss.add(sub);
						ss.add(sub.Z().Cat(old_price, SFMT_MONEY));
						ss.add(sub.Z().Cat(new_price, SFMT_MONEY));
						ss.add(sub.Z().Cat(diff,      SFMT_MONEY));
						THROW(addStringToList(i, ss.getBuf()));
					}
				}
				ok = 1;
			}
			CATCHZOK
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			if(id > 0 && id <= Data.getCountI()) {
				Data.at(id-1).Val = 0;
				ok = 1;
			}
			return ok;
		}
		const  PPBillPacket * P_Pack;
		int    ByCost;
		RAssocArray Data;
	};
	int    ok = -1, valid_data = 0, r = -1, r2, is_modif = 0;
	PPID   valuation_qk_id = 0;
	NewPricesDialog * p_dlg = 0;
	const PPBillConfig & r_cfg = GetConfig();
	AutoCalcPricesParam param(BIN(r_cfg.Flags & BCF_VALUATION_STRICT));
	param.Dt = pPack->Rec.Dt;
	param.QuotKindID = r_cfg.ValuationQuotKindID;
	if(r_cfg.Flags & BCF_VALUATION_STRICT || param.Restore() <= 0) {
		param.RoundDir = r_cfg.ValuationRndDir;
		param.RoundPrec = r_cfg.ValuationRndPrec;
		SETFLAG(param.Flags, CalcPriceParam::fRoundVat, r_cfg.Flags & BCF_VALUATION_RNDVAT);
		if(r_cfg.Flags & BCF_VALUATION_STRICT && pPack->OpTypeID == PPOPT_GOODSRECEIPT)
			r = 1;
	}
	if(r > 0 || (interactive && AutoCalcSelectQuot(this, &param) > 0)) {
		if(r < 0) {
			//
			// В "строгом" режиме расценки не сохраняем опции, посколько их никто не уставливал в ручную
			//
			param.Save();
		}
		param.VaPercent = 0.0;
		uint   i = 0;
		RAssocArray prices_ary;
		PPTransferItem * p_ti = 0;
		for(i = 0; pPack->EnumTItems(&i, &p_ti) > 0;) {
			const PPID goods_id = labs(p_ti->GoodsID);
			double new_price = 0.0;
			PPBillConfig cfg = r_cfg;
			cfg.ValuationRndDir = param.RoundDir;
			cfg.ValuationRndPrec = param.RoundPrec;
			if(param.QuotKindID)
				cfg.ValuationQuotKindID = param.QuotKindID;
			valuation_qk_id = cfg.ValuationQuotKindID; // @v8.2.0
			SETFLAG(cfg.Flags, BCF_VALUATION_RNDVAT, param.Flags & CalcPriceParam::fRoundVat);
			{
				const PPID preserve_suppl = p_ti->Suppl; // @v9.4.8
				SETIFZ(p_ti->Suppl, pPack->Rec.Object); // @v9.4.8
				r2 = p_ti->Valuation(cfg, 1, &new_price);
				p_ti->Suppl = preserve_suppl; // @v9.4.8
				THROW(r2);
			}
			THROW_SL(prices_ary.Add(goods_id, new_price, 0, 0));
		}
		if(interactive)
			THROW(CheckDialogPtr(&(p_dlg = new NewPricesDialog(pPack, &prices_ary, oneof2(param._Action, AutoCalcPricesParam::_aCost, AutoCalcPricesParam::_aCostByContract)))));
		while(ok < 0 && (!interactive || ExecView(p_dlg) == cmOK)) {
			if(!interactive || p_dlg->getDTS(&prices_ary)) {
				for(i = 0; pPack->EnumTItems(&i, &p_ti) > 0;) {
					const double new_price = prices_ary.at(i-1).Val;
					if(oneof2(param._Action, AutoCalcPricesParam::_aCost, AutoCalcPricesParam::_aCostByContract)) {
						if(new_price > 0.0 && new_price != p_ti->Cost) {
							p_ti->Cost = new_price;
							is_modif = 1;
						}
					}
					else if(new_price > 0.0) {
						pPack->SetupItemQuotInfo(i-1, valuation_qk_id, new_price, 0); // @v8.2.0
						if(new_price != p_ti->Price) {
							p_ti->Price = new_price;
							if(p_ti->Flags & (PPTFR_RECEIPT|PPTFR_UNITEINTR)) {
								p_ti->Flags |= PPTFR_QUOT;
							}
							is_modif = 1;
						}
					}
					else {
						pPack->SetupItemQuotInfo(i-1, valuation_qk_id, 0.0, PPBillPacket::QuotSetupInfoItem::fMissingQuot); // @v8.2.0
					}
				}
				if(is_modif)
					THROW(pPack->InitAmounts(0));
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pIsModified, is_modif);
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
//
//
int PPObjBill::SelectQuotKind(PPBillPacket * pPack, const PPTransferItem * pTi, int interactive, double * pQuot)
{
	struct QuotKindSelItem { // @flat
		char   Name[48];
		PPID   ID;
		long   Rank;
		double Price;
	};
	class QuotKindSelDialog : public TDialog {
	public:
		QuotKindSelDialog() : TDialog(DLG_SELQUOT)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmLBDblClk)) {
				TView::messageCommand(this, cmOK);
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	QuotKindSelDialog * dlg = 0;
	PPObjQuotKind qkobj;
	PPQuotKind qk_rec;
	PPID   qk_id = 0;
	PPIDArray ql;
	double quot = 0.0;
	if(pPack->QuotKindID || pPack->GetQuotKindList(&ql) > 0) {
		double cost = pTi->Cost, price = pTi->Price;
		const  PPID goods_id = pTi->GoodsID;
		const  PPID loc_id = IsIntrExpndOp(pPack->Rec.OpID) ? PPObjLocation::ObjToWarehouse(pPack->Rec.Object) : pPack->Rec.LocID;
		Goods2Tbl::Rec g_rec;
		ReceiptTbl::Rec lot_rec;
		PPQuotArray q_ary, parent_q_ary;
		THROW(GObj.GetQuotList(goods_id, 0, q_ary)); // если список пустой, то извлекаем котировки для группы
		if(GObj.Fetch(goods_id, &g_rec) > 0)
			THROW(GObj.GetQuotList(g_rec.ParentID, 0, parent_q_ary));
		if(pTi->LotID && !(pTi->Flags & PPTFR_RECEIPT) && trfr->Rcpt.Search(pTi->LotID, &lot_rec) > 0) {
			trfr->GetLotPrices(&lot_rec, pTi->Date, 0);
			cost  = R5(lot_rec.Cost);
			price = R5(lot_rec.Price);
		}
		if(pPack->QuotKindID)
			qk_id = pPack->QuotKindID;
		else if(ql.getCount() > 1) {
			uint   i;
			QuotKindSelItem * p_item;
			SVector qks_list(sizeof(QuotKindSelItem)); // @v10.0.02 SArray-->SVector
			for(i = 0; i < ql.getCount(); i++) {
				QuotKindSelItem item;
				MEMSZERO(item);
				item.ID = ql.get(i);
				if(qkobj.Fetch(item.ID, &qk_rec) > 0) {
					STRNSCPY(item.Name, qk_rec.Name);
					item.Rank = qk_rec.Rank;
					const QuotIdent qi(QIDATE(pPack->Rec.Dt), loc_id, item.ID, pTi->CurID, pPack->Rec.Object);
					if(GObj.GetQuotExt(goods_id, qi, cost, price, &quot, 1) > 0)
						if(!q_ary.IsDisabled(qi, &ql, &parent_q_ary)) {
							item.Price = quot;
							qks_list.insert(&item);
						}
				}
			}
			if(qks_list.getCount() == 1)
				qk_id = static_cast<const QuotKindSelItem *>(qks_list.at(0))->ID;
			else if(qks_list.getCount() == 0)
				qk_id = 0;
			else {
				PPID   agt_qk_id = 0; // Вид котировки, определенный соглашением
				if(pPack->AgtQuotKindID && qks_list.lsearch(&pPack->AgtQuotKindID, 0, CMPF_LONG, offsetof(QuotKindSelItem, ID))) {
					if(interactive && GetConfig().Flags & BCF_OVRRDAGTQUOT)
						agt_qk_id = pPack->AgtQuotKindID;
					else
						qk_id = pPack->AgtQuotKindID;
				}
				if(!qk_id) {
					if(interactive) {
						SString sub;
						SmartListBox * p_lbx = 0;
						StringSet ss(SLBColumnDelim);
						THROW(CheckDialogPtrErr(&(dlg = new QuotKindSelDialog())));
 						p_lbx = static_cast<SmartListBox *>(dlg->getCtrlView(CTL_SELQUOT_LIST));
						THROW(SetupStrListBox(p_lbx));
						qks_list.sort(PTR_CMPFUNC(PcharNoCase));
						for(i = 0; qks_list.enumItems(&i, (void **)&p_item);) {
							ss.clear();
							ss.add(p_item->Name);
							ss.add(sub.Z().Cat(p_item->Price, SFMT_MONEY));
							p_lbx->addItem(p_item->ID, ss.getBuf());
						}
						if(agt_qk_id)
							p_lbx->TransmitData(+1, &agt_qk_id);
						p_lbx->Draw_();
						if(ExecView(dlg) == cmOK) {
							long   p = 0;
							if(p_lbx->getCurID(&p) && p > 0) {
								qk_id = p;
								if(dlg->getCtrlUInt16(CTL_SELQUOT_FLAGS) & 0x01)
									pPack->QuotKindID = qk_id;
							}
						}
					}
					else {
						long   max_rank = -MAXLONG;
						for(i = 0; qks_list.enumItems(&i, (void **)&p_item);) {
							if(max_rank < p_item->Rank) {
								max_rank = p_item->Rank;
								qk_id = p_item->ID;
							}
						}
					}
				}
			}
		}
		else
			qk_id = ql.get(0);
		if(qk_id) {
			const QuotIdent qi(QIDATE(pPack->Rec.Dt), loc_id, qk_id, pTi->CurID, pPack->Rec.Object);
			if(!q_ary.IsDisabled(qi, &ql, &parent_q_ary) && GObj.GetQuotExt(goods_id, qi, cost, price, &quot, 1) > 0)
				ok = 1;
		}
		else
			quot = 0.0;
	}
	CATCHZOK
	delete dlg;
	ASSIGN_PTR(pQuot, quot);
	return ok;
}

int PPObjBill::SetupQuot(PPBillPacket * pPack, PPID forceArID)
{
	int    ok = -1;
	PPObjQuotKind qkobj;
	PPQuotKind qk_rec;
	PPIDArray     ql;
	PPID   qk_id = 0;
	TDialog * dlg = 0;
	SArray * p_qbo_ary = 0;
	// @v9.3.12 PPOPT_GOODSORDER
	if(pPack && oneof3(pPack->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER) && /*pPack->SampleBillID &&*/ pPack->GetQuotKindList(&ql) > 0) {
		PPID   ar_id = NZOR(forceArID, pPack->Rec.Object);
		int    is_quot = 0;
		PPTransferItem * p_ti;
		PPID   loc_id = IsIntrExpndOp(pPack->Rec.OpID) ? PPObjLocation::ObjToWarehouse(ar_id) : pPack->Rec.LocID;
		SArray * p_qbo_ary = qkobj.MakeListByIDList(&ql);
		THROW(p_qbo_ary);
		if(p_qbo_ary->getCount() > 0) {
			for(uint i = 0; is_quot == 0 && pPack->EnumTItems(&i, &p_ti);) {
				//
				// Проверяем, чтобы хотя бы для одного товара из документа существовала
				// возможность установки цены по любой котировке из списка p_qbo_ary
				//
				for(uint c = 0; !is_quot && c < p_qbo_ary->getCount(); c++) {
					const PPID qk_id = ((PPObjQuotKind::ListEntry *)p_qbo_ary->at(c))->ID;
					double quot = 0.0;
					QuotIdent qi(QIDATE(pPack->Rec.Dt), loc_id, qk_id, pPack->Rec.CurID, pPack->Rec.Object);
					if(GObj.GetQuotExt(p_ti->GoodsID, qi, p_ti->Cost, p_ti->Price, &quot, 1) > 0)
						is_quot = 1;
				}
			}
		}
		if(is_quot && CONFIRM(PPCFM_SETPRICEBYQUOT)) {
			uint i;
			PPClientAgreement cliagt;
			ArObj.GetClientAgreement(pPack->Rec.Object, &cliagt, 1);
			/* @construction {
			// @v7.3.12 {
			if(cliagt.DefQuotKindID && qkobj.Fetch(cliagt.DefQuotKindID, &qk_rec) > 0)
				qk_id = cliagt.DefQuotKindID;
			} @construction */
			if(p_qbo_ary->getCount() > 1) {
				qk_id = NZOR(pPack->QuotKindID, static_cast<const PPObjQuotKind::ListEntry *>(p_qbo_ary->at(0))->ID);
				int  valid_data = 0;
				THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SELQUOT2))));
				StdListBoxDef * def = new StdListBoxDef(p_qbo_ary, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, sizeof(PPObjQuotKind::ListEntry)-sizeof(PPID)));
				THROW_MEM(def);
				{
					ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(CTLSEL_SELQUOT2_KIND));
					if(p_combo) {
						THROW_MEM(p_combo->setListWindow(new ListWindow(def, 0, 0), qk_id));
					}
					while(!valid_data && ExecView(dlg) == cmOK) {
						dlg->getCtrlData(CTLSEL_SELQUOT2_KIND, &qk_id);
						valid_data = 1;
					}
				}
				ZDELETE(dlg);
				def = 0;
				p_qbo_ary = 0;
			}
			else
				qk_id = static_cast<const PPObjQuotKind::ListEntry *>(p_qbo_ary->at(0))->ID;
			{
				ReceiptTbl::Rec ord_lot_rec;
				BillTbl::Rec ord_bill_rec;
				SString edi_channel;
				for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
					double quot = 0.0;
					QuotIdent qi(QIDATE(pPack->Rec.Dt), loc_id, qk_id, pPack->Rec.CurID, pPack->Rec.Object);
					if(GObj.GetQuotExt(p_ti->GoodsID, qi, p_ti->Cost, p_ti->Price, &quot, 1) > 0) {
						// @v9.2.7 {
						//
						// Специальный случай - для отгрузки, привязанной к заказу, принятому из некоторых
						// систем, требуется поправлять конечную цену на величину процентной скидки из заказа
						//
						TransferTbl::Rec ord_item;
						if(quot > 0.0 && p_ti->OrdLotID && trfr->Rcpt.Search(p_ti->OrdLotID, &ord_lot_rec) > 0) {
							if(Fetch(ord_lot_rec.BillID, &ord_bill_rec) > 0 && ord_bill_rec.EdiOp == PPEDIOP_SALESORDER) {
								if(PPRef->Ot.GetTagStr(PPOBJ_BILL, ord_bill_rec.ID, PPTAG_BILL_EDICHANNEL, edi_channel) > 0 && edi_channel.CmpNC("ISALES-PEPSI") == 0) {
									DateIter di;
									if(trfr->EnumByLot(ord_lot_rec.ID, &di, &ord_item) > 0 && ord_item.Flags & PPTFR_RECEIPT) {
										const double ord_qtty = fabs(ord_item.Quantity);
										const double ord_price = fabs(ord_item.Price) * ord_qtty;
										const double ord_dis   = ord_item.Discount * ord_qtty;
										const double ord_pct_dis = (ord_price > 0.0 && ord_dis > 0.0) ? R4(ord_dis / ord_price) : 0.0;
										if(ord_pct_dis > 0.0)
											quot = R5(quot * (1.0 - ord_pct_dis));
									}
								}
							}
						}
						// } @v9.2.7
						if(cliagt.Flags & AGTF_PRICEROUNDING) {
							quot = p_ti->RoundPrice(quot, cliagt.PriceRoundPrec, cliagt.PriceRoundDir,
								(cliagt.Flags & AGTF_PRICEROUNDVAT) ? PPTransferItem::valfRoundVat : 0);
						}
						// @v10.5.1 {
						if(oneof2(pPack->OpTypeID, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER) && p_ti->Price <= 0.0) {
							p_ti->Price = R2(quot);
							p_ti->Discount = 0.0;
						}
						else // } @v10.5.1 
							p_ti->Discount = R2(p_ti->Price - quot);
						p_ti->SetupQuot(quot, 1);
						pPack->SetupItemQuotInfo(i-1, qk_id, quot, 0);
						ok = 1;
					}
					else {
						pPack->SetupItemQuotInfo(i-1, qk_id, 0.0, PPBillPacket::QuotSetupInfoItem::fMissingQuot);
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	if(dlg)
		delete dlg;
	else
		delete p_qbo_ary;
	return ok;
}
//
//
//
class BillCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry { // size=48+16 // @v8.8.0 44-->48 // @v10.0.04 48-->52 // @v10.3.8 52-->56
		LDATE  Dt;
		long   BillNo; // @v10.3.8
		PPID   OpID;
		PPID   LocID;
		PPID   Object;
		PPID   Object2;
		PPID   StatusID;
		PPID   LinkBillID;
		int16  Reserve;
		int16  EdiOp;
		long   Flags;
		long   Flags2;
		LDATE  DueDate; // @v10.0.04
		double Amount;
		PPID   CurID;   // @v10.5.8
	};
	BillCache() : ObjCacheHash(PPOBJ_BILL, sizeof(Data),
		(DS.CheckExtFlag(ECF_SYSSERVICE) ? (12*1024*1024) : (4*1024U*1024U)),
		(DS.CheckExtFlag(ECF_SYSSERVICE) ? 16 : 12)), FullSerialList(1)
	{
	}
	virtual int FASTCALL Dirty(PPID id); // @sync_w
	int    FetchExtMemo(PPID id, SString & rBuf) { return EmBlk.Fetch(id, rBuf, 0); } // @sync_w
	int    FetchExt(PPID id, PPBillExt * pExt)   { return ExtCache.Get(id, pExt); } // @sync_w
	int    FetchFreight(PPID id, PPFreight * pFreight) { return FreightCache.Get(id, pFreight); } // @sync_w
	int    GetCrBillEntry(long & rTempID, PPBillPacket * pPack); // @sync_w
	int    SetCrBillEntry(long tempID, const PPBillPacket * pPack);    // @sync_w
	int    GetPrjConfig(PPProjectConfig * pCfg, int enforce);    // @sync_w
	const  StrAssocArray * GetFullSerialList(); // @sync_w
	void   ReleaseFullSerialList(const StrAssocArray * pList);
	void   ResetFullSerialList(); // @sync_w
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long extraData);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	class BillCache_ExtText_Block : public ObjCache::ExtTextBlock {
	private:
		virtual int Implement_Get(PPID id, SString & rBuf, void * extraPtr)
		{
			int    ok = -1;
			if(PPRef->GetPropVlrString(PPOBJ_BILL, id, PPPRP_BILLMEMO, rBuf) > 0 && rBuf.Len())
				ok = 1;
			return ok;
		}
	};
	class BillExtCache : public ObjCacheHash {
	public:
		struct Data : public ObjCacheEntry { // size=8+16
			PPID   AgentID;
			PPID   PayerID;
		};
		BillExtCache() : ObjCacheHash(PPOBJ_BILLEXT, sizeof(Data), 128*1024, 4, ObjCache::fUseUndefList)
		{
		}
	private:
		virtual int FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
		{
			int    ok = -1;
			PPObjBill * p_bobj = BillObj;
			if(p_bobj) {
				Data * p_cache_rec = static_cast<Data *>(pEntry);
				PPBillExt ext_rec;
				if(id) {
					if(p_bobj->P_Tbl->GetExtraData(id, &ext_rec) > 0) {
						p_cache_rec->AgentID = ext_rec.AgentID;
						p_cache_rec->PayerID = ext_rec.PayerID;
						ok = 1;
					}
					else {
						p_cache_rec->AgentID = 0;
						p_cache_rec->PayerID = 0;
						ok = -100;
					}
				}
			}
			return ok;
		}
		virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
		{
			PPBillExt * p_data_rec = static_cast<PPBillExt *>(pDataRec);
			const Data * p_cache_rec = static_cast<const Data *>(pEntry);
			memzero(p_data_rec, sizeof(*p_data_rec));
			if(!(p_cache_rec->F & ObjCacheEntry::fUndef)) {
				#define FLD(f) p_data_rec->f = p_cache_rec->f
				FLD(PayerID);
				FLD(AgentID);
				#undef FLD
			}
		}
	};
	class BillFreightCache : public ObjCacheHash {
	public:
		struct Data : public ObjCacheEntry { // size=48+16
			PPID   DlvrAddrID;
			long   TrType;
			PPID   PortOfLoading;
			PPID   PortOfDischarge;
			LDATE  IssueDate;
			LDATE  ArrivalDate;
			PPID   CaptainID;
			double Cost;
			PPID   AgentID;
			PPID   ShipID;
			PPID   StorageLocID;
		};
		BillFreightCache() : ObjCacheHash(PPOBJ_BILLFREIGHT, sizeof(Data), 512*1024, 4, ObjCache::fUseUndefList)
		{
		}
	private:
		virtual int FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
		{
			int    ok = -1;
			PPObjBill * p_bobj = BillObj;
			if(p_bobj) {
				Data * p_cache_rec = static_cast<Data *>(pEntry);
				PPFreight freight;
				if(id) {
					if(p_bobj->P_Tbl->GetFreight(id, &freight) > 0) {
						#define FLD(f) p_cache_rec->f = freight.f
						FLD(DlvrAddrID);
						FLD(TrType);
						FLD(PortOfLoading);
						FLD(PortOfDischarge);
						FLD(IssueDate);
						FLD(ArrivalDate);
						FLD(CaptainID);
						FLD(Cost);
						FLD(AgentID);
						FLD(ShipID);
						FLD(StorageLocID);
						#undef FLD
						ok = 1;
					}
					else {
						#define FLDZERO(f) p_cache_rec->f = 0
						FLDZERO(DlvrAddrID);
						FLDZERO(TrType);
						FLDZERO(PortOfLoading);
						FLDZERO(PortOfDischarge);
						FLDZERO(CaptainID);
						FLDZERO(Cost);
						FLDZERO(AgentID);
						FLDZERO(ShipID);
						FLDZERO(StorageLocID);
						#undef FLDZERO
						p_cache_rec->IssueDate.Z();
						p_cache_rec->ArrivalDate.Z();
						ok = -100;
					}
				}
			}
			return ok;
		}
		virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
		{
			PPFreight * p_data_rec = static_cast<PPFreight *>(pDataRec);
			const Data * p_cache_rec = static_cast<const Data *>(pEntry);
			memzero(p_data_rec, sizeof(*p_data_rec));
			if(!(p_cache_rec->F & ObjCacheEntry::fUndef)) {
				#define FLD(f) p_data_rec->f = p_cache_rec->f
				FLD(DlvrAddrID);
				FLD(TrType);
				FLD(PortOfLoading);
				FLD(PortOfDischarge);
				FLD(IssueDate);
				FLD(ArrivalDate);
				FLD(CaptainID);
				FLD(Cost);
				FLD(AgentID);
				FLD(ShipID);
				FLD(StorageLocID);
				#undef FLD
			}
		}
	};
	struct CrBillEntry : public PPBillPacket {
		long   TempID;
		LDATETIME CrDtm;
	};
	class FslArray : public StrAssocArray {
	public:
		explicit FslArray(int use) : StrAssocArray(), Use(use), Inited(0)
		{
		}
		void   FASTCALL Dirty(PPID lotID)
		{
			DirtyTable.Add((uint32)labs(lotID));
		}
		int    Use;
		int    Inited;
		UintHashTable DirtyTable;
	};
	//
	BillCache_ExtText_Block EmBlk;
	ReadWriteLock CrbLock; // Блокировка списка CrBillList
	ReadWriteLock FslLock; // Блокировка полного списка серийных номеров лотов
	BillExtCache ExtCache;
	BillFreightCache FreightCache;
	TSCollection <CrBillEntry> CrBillList;
	FslArray FullSerialList;
	PPProjectConfig PrjCfg;   //
	ReadWriteLock PrjCfgLock; // Блокировка конфигурации проектов
};

int FASTCALL BillCache::Dirty(PPID id)
{
	int    ok = 1;
	ObjCacheHash::Dirty(id);
	{
		EmBlk.Dirty(id);
		ExtCache.Dirty(id);
		FreightCache.Dirty(id);
	}
	return ok;
}

int BillCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	if(p_bobj) {
		Data * p_cache_rec = static_cast<Data *>(pEntry);
		BillTbl::Rec rec;
		if(id && p_bobj->Search(id, &rec) > 0) {
			#define FLD(f) p_cache_rec->f = rec.f
			FLD(Dt);
			FLD(BillNo); // @v10.3.8
			FLD(OpID);
			FLD(LocID);
			FLD(Object);
			FLD(Object2);
			FLD(StatusID);
			FLD(LinkBillID);
			FLD(EdiOp);
			FLD(Flags);
			FLD(Flags2);
			FLD(DueDate); // @v10.0.04
			FLD(Amount);
			FLD(CurID); // @v10.5.8
			#undef FLD

			MultTextBlock b;
			b.Add(rec.Code);
			b.Add(rec.Memo);
			ok = PutTextBlock(b, p_cache_rec);
		}
	}
	return ok;
}

void BillCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	BillTbl::Rec * p_data_rec = static_cast<BillTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(Dt);
	FLD(BillNo); // @v10.3.8
	FLD(OpID);
	FLD(LocID);
	FLD(Object);
	FLD(Object2);
	FLD(StatusID);
	FLD(LinkBillID);
	FLD(EdiOp);
	FLD(Flags);
	FLD(Flags2);
	FLD(DueDate); // @v10.0.04
	FLD(Amount);
	FLD(CurID); // @v10.5.8
	#undef FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
	b.Get(p_data_rec->Memo, sizeof(p_data_rec->Memo));
}

int BillCache::GetCrBillEntry(long & rTempID, PPBillPacket * pPack)
{
	int    ok = 0;
	if(rTempID) {
		SRWLOCKER(CrbLock, SReadWriteLocker::Read);
		for(uint i = 0; !ok && i < CrBillList.getCount(); i++) {
			const CrBillEntry * p_entry = CrBillList.at(i);
			if(p_entry && p_entry->TempID == rTempID) {
				ASSIGN_PTR(pPack, *static_cast<const PPBillPacket *>(p_entry));
				ok = 1;
			}
		}
	}
	else {
		SRWLOCKER(CrbLock, SReadWriteLocker::Write);
		CrBillEntry * p_new_entry = new CrBillEntry;
		if(p_new_entry) {
			if(pPack) {
				*static_cast<PPBillPacket *>(p_new_entry) = *pPack;
			}
			p_new_entry->TempID = static_cast<long>(SLS.GetSequenceValue());
			p_new_entry->CrDtm = getcurdatetime_();
			rTempID = p_new_entry->TempID;
			CrBillList.insert(p_new_entry);
			ok = 1;
		}
	}
	return ok;
}

int BillCache::SetCrBillEntry(long tempID, const PPBillPacket * pPack)
{
	int    ok = 0;
	if(tempID) {
		SRWLOCKER(CrbLock, SReadWriteLocker::Write);
		for(uint i = 0; !ok && i < CrBillList.getCount(); i++) {
			CrBillEntry * p_entry = CrBillList.at(i);
			if(p_entry && p_entry->TempID == tempID) {
				if(pPack)
					*static_cast<PPBillPacket *>(p_entry) = *pPack;
				else
					CrBillList.atFree(i);
				ok = 1;
			}
		}
	}
	return ok;
}
//
// Три метода управляения кэшированием конфигурации проектов. В сязи с тем, что PPObjProject не
// имеет собственного класса кэша, (надеюсь) временно хранением конфигурации проектов будет заниматься класс BillCache.
//
int BillCache::GetPrjConfig(PPProjectConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(PrjCfgLock, SReadWriteLocker::Read);
		if(!(PrjCfg.Flags & PRJCFGF_VALID) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(PrjCfg.Flags & PRJCFGF_VALID) || enforce) {
				PPObjProject::ReadConfig(&PrjCfg);
				PrjCfg.Flags |= PRJCFGF_VALID;
			}
		}
		ASSIGN_PTR(pCfg, PrjCfg);
	}
	return 1;
}

/*static*/int FASTCALL PPObjProject::FetchConfig(PPProjectConfig * pCfg)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	if(p_cache) {
		return p_cache->GetPrjConfig(pCfg, 0);
	}
	else {
		pCfg->Z();
		return 0;
	}
}

/*static*/int PPObjProject::DirtyConfig()
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->GetPrjConfig(0, 1) : 0;
}
//
//
//
int PPObjBill::Fetch(PPID id, BillTbl::Rec * pRec)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->Get(id, pRec, 0) : Search(id, pRec);
}

IMPL_OBJ_DIRTY(PPObjBill, BillCache);

int PPObjBill::FetchExt(PPID id, PPBillExt * pExt)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->FetchExt(id, pExt) : BillObj->P_Tbl->GetExtraData(id, pExt);
}

int PPObjBill::FetchFreight(PPID id, PPFreight * pFreight)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->FetchFreight(id, pFreight) : BillObj->P_Tbl->GetFreight(id, pFreight);
}

int PPObjBill::FetchExtMemo(PPID id, SString & rBuf)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	if(p_cache)
		return p_cache->FetchExtMemo(id, rBuf);
	else
		return PPRef->GetPropVlrString(PPOBJ_BILL, id, PPPRP_BILLMEMO, rBuf);
}

int PPObjBill::GetCrBillEntry(long & rTempID, PPBillPacket * pPack)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->GetCrBillEntry(rTempID, pPack) : 0;
}

int PPObjBill::SetCrBillEntry(long tempID, const PPBillPacket * pPack)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	return p_cache ? p_cache->SetCrBillEntry(tempID, pPack) : 0;
}

const StrAssocArray * PPObjBill::GetFullSerialList()
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
    return p_cache ? p_cache->GetFullSerialList() : 0;
}

void PPObjBill::ReleaseFullSerialList(const StrAssocArray * pList)
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	CALLPTRMEMB(p_cache, ReleaseFullSerialList(pList));
}

void PPObjBill::ResetFullSerialList()
{
	BillCache * p_cache = GetDbLocalCachePtr <BillCache> (PPOBJ_BILL);
	CALLPTRMEMB(p_cache, ResetFullSerialList());
}

void BillCache::ResetFullSerialList()
{
	SRWLOCKER(FslLock, SReadWriteLocker::Write);
	FullSerialList.Inited = 0;
	FullSerialList.DirtyTable.Clear();
}

const StrAssocArray * BillCache::GetFullSerialList()
{
	int    err = 0;
	const  StrAssocArray * p_result = 0;
	if(FullSerialList.Use) {
		if(!FullSerialList.Inited || FullSerialList.DirtyTable.GetCount()) {
			Reference * p_ref = PPRef;
			//FslLock.WriteLock();
			SRWLOCKER(FslLock, SReadWriteLocker::Write);
			if(!FullSerialList.Inited || FullSerialList.DirtyTable.GetCount()) {
				if(!FullSerialList.Inited) {
					PROFILE_START
					p_ref->Ot.GetObjTextList(PPOBJ_LOT, PPTAG_LOT_SN, FullSerialList);
					PROFILE_END
				}
				else {
					PROFILE_START
					SString serial;
					for(ulong id = 0; FullSerialList.DirtyTable.Enum(&id);) {
						ObjTagItem tag_item;
						if(p_ref->Ot.GetTag(PPOBJ_LOT, id, PPTAG_LOT_SN, &tag_item) > 0)
							tag_item.GetStr(serial);
						else
							serial.Z();
						FullSerialList.Remove(id);
						if(serial.NotEmptyS())
							FullSerialList.AddFast(id, serial);
					}
					PROFILE_END
				}
				if(!err) {
					FullSerialList.DirtyTable.Clear();
					FullSerialList.Inited = 1;
				}
			}
			//FslLock.Unlock();
		}
		if(!err) {
			#if SLTRACELOCKSTACK
			SLS.LockPush(SLockStack::ltRW_R, __FILE__, __LINE__);
			#endif
			FslLock.ReadLock_();
			p_result = &FullSerialList;
		}
	}
	return p_result;
}

void BillCache::ReleaseFullSerialList(const StrAssocArray * pList)
{
	if(pList && pList == &FullSerialList) {
		FslLock.Unlock_();
		#if SLTRACELOCKSTACK
		SLS.LockPop();
		#endif
	}
}
//
//
//
#if 0 // @projection {

class LotCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry { // size=104+16
		PPID   BillID;
		PPID   LocID;
		LDATE  Dt;
		long   OprNo;
		PPID   GoodsID;
		PPID   QCertID;
		double UnitPerPack;
		double Quantity;
		float  WtQtty;
		float  WtRest;
		double Cost;
		double ExtCost;
		double Price;
		double Rest;
		long   PrevLotID;
		long   SupplID;
		LDATE  CloseDate;
		LDATE  Expiry;
		PPID   InTaxGrpID;
		long   Flags;
	};
	LotCache();
	virtual int FASTCALL Dirty(PPID id); // @sync_w
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long extraData);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

LotCache::LotCache() : ObjCacheHash(PPOBJ_LOT, sizeof(Data),
	(DS.CheckExtFlag(ECF_SYSSERVICE) ? (16*1024*1024) : (4*1024U*1024U)),
	(DS.CheckExtFlag(ECF_SYSSERVICE) ? 32 : 12))
{
}

int LotCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
{
	int    ok = -1;
	if(BillObj && BillObj->trfr) {
		Data * p_cache_rec = static_cast<Data *>(pEntry);
		ReceiptTbl::Rec rec;
		if(id && BillObj->trfr->Rcpt.Search(id, &rec) > 0) {
			#define FLD(f) p_cache_rec->f = rec.f
			FLD(BillID);
			FLD(LocID);
			FLD(Dt);
			FLD(OprNo);
			FLD(GoodsID);
			FLD(QCertID);
			FLD(UnitPerPack);
			FLD(Quantity);
			FLD(WtQtty);
			FLD(WtRest);
			FLD(Cost);
			FLD(ExtCost);
			FLD(Price);
			FLD(Rest);
			FLD(PrevLotID);
			FLD(SupplID);
			FLD(CloseDate);
			FLD(Expiry);
			FLD(InTaxGrpID);
			FLD(Flags);
			#undef FLD
			SETFLAG(p_cache_rec->Flags, LOTF_CLOSED, rec.Closed);
			ok = 1;
		}
	}
	return ok;
}

void LotCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	BillTbl::Rec * p_data_rec = static_cast<BillTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(BillID);
	FLD(LocID);
	FLD(Dt);
	FLD(OprNo);
	FLD(GoodsID);
	FLD(QCertID);
	FLD(UnitPerPack);
	FLD(Quantity);
	FLD(WtQtty);
	FLD(WtRest);
	FLD(Cost);
	FLD(ExtCost);
	FLD(Price);
	FLD(Rest);
	FLD(PrevLotID);
	FLD(SupplID);
	FLD(CloseDate);
	FLD(Expiry);
	FLD(InTaxGrpID);
	FLD(Flags);
	#undef FLD
	p_data_rec->Closed = BIN(p_cache_rec->Flags & LOTF_CLOSED);
	p_data_rec->Flags &= ~LOTF_CLOSED;
}

#endif // } 0 @projection
//
//
//
SubstGrpBill::SubstGrpBill() : S(sgbNone)
{
	S2.Sgd = sgdNone;
}

void SubstGrpBill::Reset()
{
	S = sgbNone;
	S2.Sgd = sgdNone;
}

int SubstGrpBill::operator !() const
{
	return (S == sgbNone);
}
//
//
//
PPObjBill::SubstParam::SubstParam() : P_DebtDimAgentList(0)
{
}

PPObjBill::SubstParam::~SubstParam()
{
	ZDELETE(P_DebtDimAgentList);
}

void FASTCALL PPObjBill::SubstParam::Init(SubstGrpBill sgb)
{
	Sgb = sgb;
	if(oneof5(Sgb.S, SubstGrpBill::sgbObject, SubstGrpBill::sgbObject2, SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer, SubstGrpBill::sgbDlvrLoc))
		Psp.Init(Sgb.S2.Sgp);
	else
		Psp.Init(sgpNone);
	AsscList.freeAll();
	ZDELETE(P_DebtDimAgentList);
}

int PPObjBill::SubstParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint8  ind = 0;
	THROW_SL(pCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(Sgb.S)), &Sgb.S, 0, rBuf));
	THROW_SL(pCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(Sgb.S2)), &Sgb.S2, 0, rBuf));
	THROW(Psp.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, &AsscList, rBuf));
	if(dir > 0) {
		if(!P_DebtDimAgentList) {
			ind = 1;
			THROW_SL(rBuf.Write(ind));
		}
		else {
			ind = 0;
			THROW_SL(rBuf.Write(ind));
			THROW_SL(pCtx->Serialize(dir, P_DebtDimAgentList, rBuf));
		}
	}
	else {
		THROW_SL(rBuf.Read(ind));
		ZDELETE(P_DebtDimAgentList);
		if(ind == 0) {
			THROW_MEM(P_DebtDimAgentList = new LAssocArray);
			THROW_SL(pCtx->Serialize(dir, P_DebtDimAgentList, rBuf));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::LoadForSubst(const SubstParam * pParam, PPBill * pPack)
{
	int    ar = 0;
	switch(pParam->Sgb.S) {
		case SubstGrpBill::sgbNone:
		case SubstGrpBill::sgbDate:
		case SubstGrpBill::sgbOp:
		case SubstGrpBill::sgbStatus:
		case SubstGrpBill::sgbLocation:
			break;
		case SubstGrpBill::sgbObject:
		case SubstGrpBill::sgbObject2:
			ar = 1;
			break;
		case SubstGrpBill::sgbAgent:
		case SubstGrpBill::sgbPayer:
		case SubstGrpBill::sgbDebtDim:
			ar = 1;
			FetchExt(pPack->Rec.ID, &pPack->Ext);
			break;
		case SubstGrpBill::sgbDlvrLoc: // @v9.1.5
			break;
	}
	if(((ar && pParam->Sgb.S2.Sgp) || oneof2(pParam->Sgb.S, SubstGrpBill::sgbStorageLoc, SubstGrpBill::sgbDlvrLoc)) && pPack->Rec.Flags & BILLF_FREIGHT) {
		PPFreight freight;
		if(P_Tbl->GetFreight(pPack->Rec.ID, &freight) > 0)
			pPack->SetFreight(&freight);
		else
			pPack->SetFreight(0);
	}
	return 1;
}

int PPObjBill::Subst(const PPBill * pPack, PPID * pDestID, SubstParam * pParam)
{
	int    ok = 1;
	long   val = 0;
	int    ar = 0, debt_dim = 0;
	switch(pParam->Sgb.S) {
		case SubstGrpBill::sgbNone:     val = pPack->Rec.ID;       break;
		case SubstGrpBill::sgbOp:       val = pPack->Rec.OpID;     break;
		case SubstGrpBill::sgbStatus:   val = pPack->Rec.StatusID; break;
		case SubstGrpBill::sgbLocation: val = pPack->Rec.LocID;    break;
		case SubstGrpBill::sgbObject:   val = pPack->Rec.Object;  ar = 1; break;
		case SubstGrpBill::sgbObject2:  val = pPack->Rec.Object2; ar = 1; break;
		case SubstGrpBill::sgbAgent:    val = pPack->Ext.AgentID; ar = 1; break;
		case SubstGrpBill::sgbPayer:    val = pPack->Ext.PayerID; ar = 1; break;
		case SubstGrpBill::sgbStorageLoc: val = pPack->P_Freight ? pPack->P_Freight->StorageLocID : 0; break;
		case SubstGrpBill::sgbDlvrLoc: val = pPack->GetDlvrAddrID(); break;
		case SubstGrpBill::sgbDebtDim:  debt_dim = 1; break;
		case SubstGrpBill::sgbDate:
			{
				LDATE  dest_dt;
				ShrinkSubstDate(pParam->Sgb.S2.Sgd, pPack->Rec.Dt, &dest_dt);
				val = dest_dt.v;
			}
			break;
	}
	if(debt_dim) {
		PPID   agent_id = pPack->Ext.AgentID;
		if(!pParam->P_DebtDimAgentList) {
			pParam->P_DebtDimAgentList = new LAssocArray;
			PPObjDebtDim dd_obj;
			dd_obj.FetchAgentList(pParam->P_DebtDimAgentList);
		}
		if(pParam->P_DebtDimAgentList) {
			LongArray dd_list;
			pParam->P_DebtDimAgentList->GetListByVal(agent_id, dd_list);
			if(dd_list.getCount())
				val = dd_list.get(0);
		}
	}
	else if(ar && val && pParam->Sgb.S2.Sgp) {
		long   temp_val = 0;
		PPObjPerson psn_obj;
		psn_obj.Subst((val | sgpArticleMask), pPack->GetDlvrAddrID(), &pParam->Psp, 0, &temp_val);
		val = temp_val;
	}
	pParam->AsscList.Add(val, pPack->Rec.ID, 0);
	ASSIGN_PTR(pDestID, val);
	return ok;
}

int PPObjBill::GetSubstObjType(long id, const SubstParam * pParam, PPObjID * pObjID) const
{
	int    ok = 1;
	PPObjID obj_id;
	switch(pParam->Sgb.S) {
		case SubstGrpBill::sgbNone: break;
		case SubstGrpBill::sgbObject:
		case SubstGrpBill::sgbObject2:
		case SubstGrpBill::sgbAgent:
		case SubstGrpBill::sgbPayer: obj_id.Set(PPOBJ_ARTICLE, id); break;
		case SubstGrpBill::sgbOp: obj_id.Set(PPOBJ_OPRKIND, id); break;
		case SubstGrpBill::sgbLocation:
		case SubstGrpBill::sgbDlvrLoc:  obj_id.Set(PPOBJ_LOCATION, id); break;
		case SubstGrpBill::sgbDebtDim: obj_id.Set(PPOBJ_DEBTDIM, id); break;
		case SubstGrpBill::sgbStatus: obj_id.Set(PPOBJ_BILLSTATUS, id); break;
		case SubstGrpBill::sgbDate: obj_id.Set(0, id); break;
	}
	if(obj_id.Obj == PPOBJ_ARTICLE) {
		if(pParam->Sgb.S2.Sgp) {
			PPObjPerson psn_obj;
			psn_obj.GetSubstObjType(id, &pParam->Psp, &obj_id);
		}
	}
	ASSIGN_PTR(pObjID, obj_id);
	return ok;
}

void PPObjBill::GetSubstText(PPID srcID, SubstParam * pParam, SString & rBuf)
{
	long   val = srcID;
	int    ar = 0;
	rBuf.Z();
	switch(pParam->Sgb.S) {
		case SubstGrpBill::sgbNone: break;
		case SubstGrpBill::sgbObject:
		case SubstGrpBill::sgbObject2:
		case SubstGrpBill::sgbAgent:
		case SubstGrpBill::sgbPayer: ar = 1; break;
		case SubstGrpBill::sgbOp: GetOpName(val, rBuf); break;
		case SubstGrpBill::sgbLocation: GetLocationName(val, rBuf); break;
		case SubstGrpBill::sgbDebtDim:
			if(srcID) {
				PPObjDebtDim dd_obj;
				PPDebtDim dd_rec;
				if(dd_obj.Search(srcID, &dd_rec) > 0)
					rBuf = dd_rec.Name;
				else
					ideqvalstr(srcID, rBuf);
			}
			break;
		case SubstGrpBill::sgbStatus:
			{
				PPObjBillStatus bs_obj;
				PPBillStatus bs_rec;
				if(bs_obj.Fetch(val, &bs_rec) > 0)
					rBuf = bs_rec.Name;
				else
					ideqvalstr(val, rBuf);
			}
			break;
		case SubstGrpBill::sgbDate:
			{
				LDATE  dt;
				dt.v = val;
				FormatSubstDate(pParam->Sgb.S2.Sgd, dt, rBuf, DATF_DMY|DATF_CENTURY);
			}
			break;
		case SubstGrpBill::sgbStorageLoc:
			GetLocationName(val, rBuf);
			break;
		case SubstGrpBill::sgbDlvrLoc:
			{
                PPObjLocation loc_obj;
                LocationTbl::Rec loc_rec;
                if(loc_obj.Fetch(val, &loc_rec) > 0) {
					rBuf.Cat(loc_rec.Name);
					SString addr_buf;
					LocationCore::GetAddress(loc_rec, 0, addr_buf);
                    if(addr_buf.NotEmptyS())
						rBuf.CatDivIfNotEmpty(';', 2).Cat(addr_buf);
                }
                if(!rBuf.NotEmptyS())
                    ideqvalstr(val, rBuf);
			}
			break;
	}
	if(ar) {
		if(pParam->Sgb.S2.Sgp) {
			PPObjPerson psn_obj;
			psn_obj.GetSubstText(val, 0, &pParam->Psp, rBuf);
		}
		else
			GetArticleName(val, rBuf);
	}
}

PPObjBill::PplBlock::PplBlock(const DateRange & rPeriod, const PPIDArray * pOpList, const PPIDArray * pPaymOpList) :
	Flags(0), Period(rPeriod), Amount(0.0), NominalAmount(0.0), Payment(0.0), PaymentBefore(0.0), Part(1.0), PartBefore(1.0)
{
	GatherPaymPeriod.Z();
	if(pOpList) {
		OpList = *pOpList;
		Flags |= fUseOpList;
	}
	if(pPaymOpList) {
		PaymOpList = *pPaymOpList;
		Flags |= fUsePaymOpList;
	}
}

void PPObjBill::PplBlock::Reset()
{
	Amount = 0.0;
	NominalAmount = 0.0;
	Payment = 0.0;
	PaymentBefore = 0.0;
	Part = 1.0;
	PartBefore = 1.0;
	PaymList.clear();
}

int PPObjBill::PplBlock::AddOp(PPID opID)
{
	Flags |= fUseOpList;
	return OpList.addUnique(opID);
}

int PPObjBill::PplBlock::AddPaymOpList(const PPIDArray & rOpList)
{
	if(rOpList.getCount()) {
		Flags |= fUsePaymOpList;
		return PaymOpList.addUnique(&rOpList);
	}
	else
		return -1;
}

int FASTCALL PPObjBill::PplBlock::CheckOp(PPID opID) const { return BIN(!(Flags & fUseOpList) || OpList.lsearch(opID)); }
int FASTCALL PPObjBill::PplBlock::CheckPaymOp(PPID opID) const { return BIN(!(Flags & fUsePaymOpList) || PaymOpList.lsearch(opID)); }

void FASTCALL PPObjBill::PplBlock::AddPaym(const BillTbl::Rec & rRec)
{
	const double amt = rRec.Amount;
	if(GetOpType(rRec.OpID) == PPOPT_GOODSRETURN)
		Amount -= amt;
	else {
		Payment += amt;
		if(rRec.Dt < Period.low)
			PaymentBefore += amt;
		if(Flags & fGatherPaym && GatherPaymPeriod.CheckDate(rRec.Dt))
			PaymList.Add(rRec.ID, amt);
	}
}

void FASTCALL PPObjBill::PplBlock::FinishLot(PPID orgLotID)
{
	if(Amount != 0.0) {
		Part = Payment / Amount;
		PartBefore = PaymentBefore / Amount;
	}
	if(!(Flags & fGatherPaym))
		OrgPartList.Add(orgLotID, Part, 0);
}

int PPObjBill::GetPayoutPartOfLot(PPID lotID, PplBlock & rBlk, double * pPart)
{
	return Helper_GetPayoutPartOfLot(lotID, rBlk, pPart, 0);
}

int PPObjBill::Helper_GetPayoutPartOfLot(PPID lotID, PplBlock & rBlk, double * pPart, int recur)
{
	int    ok = -1;
	rBlk.Reset();
	if(lotID) {
		PPID   org_lot_id = 0;
		ReceiptTbl::Rec org_lot_rec;
		if(trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, 0, &org_lot_rec) > 0) {
			const int non_org = BIN(org_lot_id != lotID); // Признак того, что лот не является оригинальным (порожден от другого).
			double tp = 0.0;
			if(rBlk.OrgPartList.Search(org_lot_id, &tp, 0)) {
				rBlk.Part = tp;
				ok = non_org ? 2 : 1;
			}
			else {
				BillTbl::Rec exp_rec, paym_rec/*, rckn_rec*/;
				if(Fetch(org_lot_rec.BillID, &exp_rec) > 0) {
					// @v9.0.6 {
					const int    is_modif = BIN(GetOpType(exp_rec.OpID) == PPOPT_GOODSMODIF);
					const double _camt = is_modif ? fabs(org_lot_rec.Cost * org_lot_rec.Quantity) : exp_rec.Amount;
					rBlk.NominalAmount = _camt;
					rBlk.Amount = _camt;
					if(_camt != 0.0 && rBlk.CheckOp(exp_rec.OpID)) {
						if(is_modif) {
							double total_outp_cost = 0.0;
							PPBillPacket mbpack;
							PplBlock temp_blk(rBlk.Period, (rBlk.Flags & rBlk.fUseOpList) ? &rBlk.OpList : 0,
								(rBlk.Flags & rBlk.fUsePaymOpList) ? &rBlk.PaymOpList : 0);
							SETFLAGBYSAMPLE(temp_blk.Flags, rBlk.fGatherPaym, rBlk.Flags);
							temp_blk.GatherPaymPeriod = rBlk.GatherPaymPeriod;
                            THROW(ExtractPacket(exp_rec.ID, &mbpack) > 0);
                            for(uint i = 0; i < mbpack.GetTCount(); i++) {
								const PPTransferItem & r_ti = mbpack.ConstTI(i);
								if(r_ti.Flags & PPTFR_MINUS) {
									if(r_ti.LotID) {
										double temp_part = 0.0;
										THROW(Helper_GetPayoutPartOfLot(r_ti.LotID, temp_blk, (pPart ? &temp_part : 0), recur+1)); // @recursion
										{
											rBlk.Amount  += temp_blk.Amount;
											rBlk.Payment += temp_blk.Payment;
											rBlk.PaymentBefore += temp_blk.PaymentBefore;
											if(rBlk.Flags & rBlk.fGatherPaym) {
												THROW_SL(rBlk.PaymList.Add(temp_blk.PaymList));
											}
										}
									}
								}
								else if(r_ti.Flags & PPTFR_PLUS) {
									total_outp_cost += fabs(r_ti.Cost * r_ti.Quantity_);
								}
                            }
						}
						else if(CheckOpFlags(exp_rec.OpID, OPKF_NEEDPAYMENT)) {
							PPIDArray paym_id_list;
							for(DateIter di(0, rBlk.Period.upp); P_Tbl->EnumLinks(exp_rec.ID, &di, BLNK_PAYMRETN, &paym_rec) > 0;) {
								paym_id_list.add(paym_rec.ID);
								if(rBlk.CheckPaymOp(paym_rec.OpID)) {
									rBlk.AddPaym(paym_rec);
								}
							}
						}
						else {
							// @v7.3.0 @debug Проверка влияния ошибки на результат rBlk.AddPaym(exp_rec); // @v7.2.10 paym_rec-->exp_rec (defect)
							// @v9.0.6 rBlk.AddPaym(paym_rec); // @v7.3.0 @debug Проверка влияния ошибки на результат
							// @v9.0.6 {
							MEMSZERO(paym_rec);
							paym_rec.OpID = exp_rec.OpID;
							paym_rec.Amount = rBlk.NominalAmount;
							paym_rec.Dt = exp_rec.Dt;
							rBlk.AddPaym(paym_rec);
							// } @v9.0.6
						}
						ok = (non_org || is_modif) ? 2 : 1;
					}
				}
				rBlk.FinishLot(org_lot_id);
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPart, rBlk.Part);
	return ok;
}

int PPObjBill::Helper_GetExpendedPartOfReceipt(PPID lotID, const DateIter & rDi, const DateRange * pPaymPeriod, const PPIDArray * pOpList, EprBlock & rBlk, PPIDArray & rRecurList)
{
	int    ok = 1;
	PPID   org_bill_id = 0;
	//DateRange period;
	//period.Set(ZERODATE, rDi.end);
	TransferTbl::Rec rec;
	BillTbl::Rec bill_rec;
	DateIter di = rDi;
	rRecurList.add(lotID);
	while(trfr->EnumByLot(lotID, &di, &rec) > 0) {
		if(rec.Flags & PPTFR_RECEIPT && rec.LotID == lotID) {
			// Строку собственно прихода пропускаем, но фиксируем ИД документа для специального учета возвратов
			org_bill_id = rec.BillID;
		}
		else {
			if(Fetch(rec.BillID, &bill_rec) > 0) {
				if(IsIntrExpndOp(bill_rec.OpID) == INTREXPND) {
					if(trfr->SearchMirror(rec.Dt, rec.OprNo, &rec) > 0 && rec.Flags & PPTFR_RECEIPT) {
						if(rRecurList.lsearch(rec.LotID)) {
							SString added_msg_buf;
							//PPERR_LOTERR_RECURINTROP "Обнаружена рекурсивная петля во внутренних перемещениях Transfer: %s"
							added_msg_buf.Z().CatEq("LotID", rec.LotID).CatDiv(';', 2).CatEq("Date", rec.Dt).CatDiv(';', 2).CatEq("OprNo", rec.OprNo);
							PPSetError(PPERR_LOTERR_RECURINTROP, added_msg_buf);
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
						}
						else {
							DateIter di2 = di;
							di2.dt = rec.Dt;
							di2.oprno = rec.OprNo;
							THROW(Helper_GetExpendedPartOfReceipt(rec.LotID, di2, pPaymPeriod, pOpList, rBlk, rRecurList)); // @recursion
						}
					}
				}
				else if((!pOpList || pOpList->lsearch(bill_rec.OpID))) {
					if(!(rec.Flags & PPTFR_REVAL)) {
						double cost = rec.Cost;
						double qtty = (PPTransferItem::GetSign(bill_rec.OpID, rec.Flags) >= 0) ? -fabs(rec.Quantity) : +fabs(rec.Quantity);
						/* @construction if(GetOpType(bill_rec.OpID) == PPOPT_GOODSRETURN && bill_rec.LinkBillID == org_bill_id) {
							// Возвраты поставщику убираем как будто и не было товара.
							rBlk.Amount -= (cost * qtty);
						}
						else @construction */ {
							rBlk.Expend += (cost * qtty);
							if(bill_rec.Amount != 0.0 && CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT)) {
								double p = 0.0;
								THROW(P_Tbl->CalcPayment(bill_rec.ID, 1, pPaymPeriod, 0, &p));
								rBlk.Payout += (cost * qtty * p / bill_rec.Amount);
							}
							else
								rBlk.Payout += (cost * qtty);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::GetExpendedPartOfReceipt(PPID lotID, const DateRange * pPeriod, const PPIDArray * pOpList, EprBlock & rBlk)
{
	int    ok = 1;
	rBlk.Amount = 0.0;
	rBlk.Expend = 0.0;
	rBlk.Payout = 0.0;
	ReceiptTbl::Rec lot_rec;
	DateRange paym_period;
	paym_period.Set(pPeriod);
	if(trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
		const double tolerance = 1.0e-9;
		PPIDArray recur_list;
		rBlk.Amount += fabs(lot_rec.Cost * lot_rec.Quantity);
		THROW(Helper_GetExpendedPartOfReceipt(lotID, DateIter(pPeriod), &paym_period, pOpList, rBlk, recur_list));
		if(fabs(rBlk.Payout) > rBlk.Amount)
			rBlk.Payout = rBlk.Amount;
		else if(fabs(rBlk.Payout) < tolerance)
			rBlk.Payout = 0.0;
		if(fabs(rBlk.Expend) > rBlk.Amount)
			rBlk.Expend = rBlk.Amount;
		else if(fabs(rBlk.Expend) < tolerance)
			rBlk.Expend = 0.0;
	}
	CATCHZOK
	return ok;
}

int PPObjBill::GetTagListByLot(PPID lotID, int skipReserveTags, ObjTagList * pList)
{
	int    ok = -1, is_parent_lot = 0;
	ObjTagList list;
	if(lotID) {
		PPIDArray lot_id_list;
		ReceiptTbl::Rec lot_rec;
		ObjTagCore & r_ot = PPRef->Ot;
		do {
			ObjTagList temp_list;
			if(r_ot.GetList(PPOBJ_LOT, lotID, &temp_list) && temp_list.GetCount()) {
				if(skipReserveTags) {
					temp_list.PutItem(PPTAG_LOT_CLB, 0);
					temp_list.PutItem(PPTAG_LOT_SN, 0);
				}
				temp_list.Merge(list, ObjTagList::mumAdd|ObjTagList::mumUpdate);
				list = temp_list;
				ok = 1;
			}
			if(trfr->Rcpt.Search(lotID, &lot_rec) > 0 && lot_id_list.addUnique(lotID = lot_rec.PrevLotID) > 0)
				is_parent_lot = 1;
			else
				lotID = 0;
		} while(lotID);
	}
	ASSIGN_PTR(pList, list);
	return ok;
}

int PPObjBill::GetClbNumberByLot(PPID lotID, int * pIsParentLot, SString & rBuf)
{
	int    ok = -1, is_parent_lot = 0;
	rBuf.Z();
	if(lotID) {
		ObjTagItem oti;
		PPIDArray lot_id_list;
		do {
			if(PPRef->Ot.EnumTags(PPOBJ_LOT, lotID, PPTAG_LOT_CLB, 0, &oti) > 0 && oti.Val.PStr) {
				(rBuf = oti.Val.PStr).Strip();
				ASSIGN_PTR(pIsParentLot, is_parent_lot);
				ok = 1;
			}
			else if(trfr->Rcpt.Search(lotID, 0) > 0) {
				lotID = trfr->Rcpt.data.PrevLotID;
				if(lot_id_list.addUnique(lotID) > 0)
					is_parent_lot = 1;
				else
					lotID = 0;
			}
			else
				lotID = 0;
		} while(ok < 0 && lotID);
	}
	return ok;
}

int PPObjBill::GetSerialNumberByLot(PPID lotID, SString & rBuf, int useCache)
{
	int    ok = -1;
	rBuf.Z();
	if(lotID) {
		ObjTagItem oti;
		if(useCache) {
			PPObjTag tag_obj;
			if(tag_obj.FetchTag(lotID, PPTAG_LOT_SN, &oti) > 0 && oti.Val.PStr) {
				(rBuf = oti.Val.PStr).Strip();
				ok = 1;
			}
		}
		else {
			if(PPRef->Ot.EnumTags(PPOBJ_LOT, lotID, PPTAG_LOT_SN, 0, &oti) > 0 && oti.Val.PStr) {
				(rBuf = oti.Val.PStr).Strip();
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjBill::SelectLotBySerial(const char * pSerial, PPID goodsID, PPID locID, ReceiptTbl::Rec * pRec)
{
	int    ok = -1, r = -1;
	if(!isempty(pSerial)) {
		PPIDArray lot_list;
		PPID   lot_id = 0;
		ReceiptTbl::Rec lot_rec;
		if(SearchLotsBySerialExactly(pSerial, &lot_list) > 0) { // @v9.1.1 SearchLotsBySerial-->SearchLotsBySerialExactly
			while(ok < 0 && (r = SelectLotFromSerialList(&lot_list, locID, &lot_id, &lot_rec)) > 0) {
				if(!goodsID || lot_rec.GoodsID == goodsID) {
					ASSIGN_PTR(pRec, lot_rec);
					ok = 1;
				}
				else
					lot_list.freeByKey(lot_rec.ID, 0);
			}
		}
	}
	return ok;
}

int PPObjBill::SelectLotFromSerialList(const PPIDArray * pList, PPID locID, PPID * pLotID, ReceiptTbl::Rec * pRec)
{
	int    ok = -1;
	LDATE  last_date = ZERODATE;
	long   last_oprno = 0;
	PPID   last_id = 0;
	LDATE  last_clsd_date = ZERODATE;
	long   last_clsd_oprno = 0;
	for(uint i = 0; i < pList->getCount(); i++) {
		PPID   lot_id = pList->at(i);
		ReceiptTbl::Rec lot_rec;
		if(trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
			if(ok < 0 && locID && lot_rec.LocID != locID) {
				if(!last_id) {
					last_id = lot_rec.ID;
					ASSIGN_PTR(pRec, lot_rec);
				}
				ok = -2;
			}
			else if(ok < 0 && lot_rec.Closed) {
				if(lot_rec.Dt > last_clsd_date || (lot_rec.Dt == last_clsd_date && lot_rec.OprNo > last_clsd_oprno)) {
					last_clsd_date  = lot_rec.Dt;
					last_clsd_oprno = lot_rec.OprNo;
					last_id = lot_rec.ID;
					ASSIGN_PTR(pRec, lot_rec);
				}
				ok = -3;
			}
			else if(lot_rec.Dt > last_date || (lot_rec.Dt == last_date && lot_rec.OprNo > last_oprno)) {
				last_date  = lot_rec.Dt;
				last_oprno = lot_rec.OprNo;
				last_id    = lot_rec.ID;
				ASSIGN_PTR(pRec, lot_rec);
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pLotID, last_id);
	return ok;
}

int PPObjBill::LoadRowTagListForDraft(PPID billID, PPLotTagContainer & rContainer)
{
	int    ok = -1;
	SBuffer sbuf;
	rContainer.Release();
	if(PPRef->GetPropSBuffer(Obj, billID, BILLPRP_DRAFTTAGLIST, sbuf) > 0) {
		SSerializeContext sctx;
		THROW(rContainer.Serialize(-1, sbuf, &sctx));
		if(rContainer.GetCount() > 0)
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjBill::LoadClbList(PPBillPacket * pPack, int force)
{
	int    ok = 1;
	PPObjTag * p_tag_obj = 0;
	SString b;
	const int is_intrexpnd = IsIntrExpndOp(pPack->Rec.OpID);
	ZDELETE(pPack->P_MirrorLTagL);
	if(pPack->IsDraft()) {
		// @v10.0.0 {
		if(force == 2)
			pPack->LTagL.Release();
		// } @v10.0.0
		if(pPack->LTagL.GetCount() == 0) {
			const int lrtr = LoadRowTagListForDraft(pPack->Rec.ID, pPack->LTagL);
			THROW(lrtr);
			if(lrtr > 0) {
				SString img_path;
				SString img_tag_addendum;
				SPathStruc sp;
				for(uint i = 0; i < pPack->GetTCount(); i++) {
					ObjTagList * p_tag_list = pPack->LTagL.Get(i);
					if(p_tag_list) {
						const PPTransferItem & r_ti = pPack->ConstTI(i);
						for(uint j = 0; j < p_tag_list->GetCount(); j++) {
							const ObjTagItem * p_tag_item = p_tag_list->GetItemByPos(j);
							if(p_tag_item->TagDataType == OTTYP_IMAGE) {
								ObjTagItem tag_item = *p_tag_item;
								img_tag_addendum.Z().Cat(pPack->Rec.ID).CatChar('-').Cat(r_ti.RByBill);
								ObjLinkFiles link_files(PPOBJ_TAG);
								link_files.Load(tag_item.TagID, img_tag_addendum);
								link_files.At(0, img_path);
								tag_item.SetStr(tag_item.TagID, img_path);
								p_tag_list->PutItem(tag_item.TagID, &tag_item);
							}
						}
					}
				}
			}
		}
	}
	else if(oneof3(pPack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_GOODSORDER) || is_intrexpnd || force) {
		// @v9.1.5 PPOPT_GOODSORDER // @v10.0.0 else
		PPTransferItem * p_ti;
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti);) {
			if(p_ti->LotID) {
				const int row_idx = (int)(i-1);
				if((p_ti->Flags & PPTFR_RECEIPT) || force || is_intrexpnd) {
					ObjTagList tag_list;
					GetTagListByLot(p_ti->LotID, 0/*skipReserveTags*/, &tag_list); // @v9.8.11 skipReserved 1-->0
					pPack->LTagL.Set(row_idx, tag_list.GetCount() ? &tag_list : 0);
				}
				if(is_intrexpnd && trfr->SearchByBill(p_ti->BillID, 1, p_ti->RByBill, 0) > 0) {
					const PPID mirror_lot_id = trfr->data.LotID;
					if(mirror_lot_id) {
						ObjTagList mirror_tag_list;
						GetTagListByLot(mirror_lot_id, 1, &mirror_tag_list);
						uint   j = mirror_tag_list.GetCount();
						if(j) {
							do {
								const ObjTagItem * p_item = mirror_tag_list.GetItemByPos(--j);
								THROW_MEM(SETIFZ(p_tag_obj, new PPObjTag));
								if(p_item && !p_tag_obj->IsUnmirrored(p_item->TagID)) {
									mirror_tag_list.PutItem(p_item->TagID, 0);
								}
							} while(j);
							if(mirror_tag_list.GetCount()) {
								THROW_MEM(SETIFZ(pPack->P_MirrorLTagL, new PPLotTagContainer));
								pPack->P_MirrorLTagL->Set(row_idx, &mirror_tag_list);
							}
						}
					}
				}
			}
		}
	}
	if(P_LotXcT) {
		SBuffer vxcl_buf;
		THROW(P_LotXcT->GetContainer(pPack->Rec.ID, pPack->XcL));
		// @v10.3.0 {
		if(PPRef->GetPropSBuffer(Obj, pPack->Rec.ID, BILLPRP_VALXCL, vxcl_buf) > 0) {
			SSerializeContext sctx;
			const size_t actual_size = vxcl_buf.GetAvailableSize();
			const size_t cs_size = SSerializeContext::GetCompressPrefix(0);
			if(actual_size > cs_size && SSerializeContext::IsCompressPrefix(vxcl_buf.GetBuf(vxcl_buf.GetRdOffs()))) {
				SCompressor compr(SCompressor::tZLib);
				SBuffer dbuf;
				THROW_SL(compr.DecompressBlock(vxcl_buf.GetBuf(vxcl_buf.GetRdOffs()+cs_size), actual_size-cs_size, dbuf));
				if(!pPack->_VXcL.Serialize(-1, dbuf, &sctx)) {
					pPack->_VXcL.Release();
					// @todo log error
				}
			}
			else {
				if(!pPack->_VXcL.Serialize(-1, vxcl_buf, &sctx)) {
					pPack->_VXcL.Release();
					// @todo log error
				}
			}
		}
		// } @v10.3.0
	}
	pPack->BTagL.Destroy();
	THROW(GetTagList(pPack->Rec.ID, &pPack->BTagL));
	CATCHZOK
	delete p_tag_obj;
	return ok;
}

int PPObjBill::SetTagNumberByLot(PPID lotID, PPID tagID, const char * pNumber, int use_ta)
{
	int    ok = 1;
	if(lotID) {
		ObjTagItem tagitem;
		THROW(tagitem.SetStr(tagID, pNumber));
		THROW(PPRef->Ot.PutTag(PPOBJ_LOT, lotID, &tagitem, use_ta));
	}
	CATCHZOK
	return ok;
}

int PPObjBill::GetTagList(PPID billID, ObjTagList * pTagList)
	{ return PPRef->Ot.GetList(Obj, billID, pTagList); }
int PPObjBill::SetTagList(PPID billID, const ObjTagList * pTagList, int use_ta)
	{ return PPRef->Ot.PutList(Obj, billID, pTagList, use_ta); }
int PPObjBill::SearchLotsBySerial(const char * pSerial, PPIDArray * pList)
	{ return PPRef->Ot.SearchObjectsByStr(PPOBJ_LOT, PPTAG_LOT_SN, pSerial, pList); }
int PPObjBill::SearchLotsBySerialExactly(const char * pSerial, PPIDArray * pList)
	{ return PPRef->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_SN, pSerial, pList); }
int PPObjBill::SetClbNumberByLot(PPID lotID, const char * pNumber, int use_ta)
	{ return SetTagNumberByLot(lotID, PPTAG_LOT_CLB, pNumber, use_ta); }
int PPObjBill::SetSerialNumberByLot(PPID lotID, const char * pNumber, int use_ta)
	{ return SetTagNumberByLot(lotID, PPTAG_LOT_SN, pNumber, use_ta); }

/*static*/int FASTCALL PPObjBill::VerifyUniqSerialSfx(const char * pSfx)
{
	int    ok = -1;
	if(!isempty(pSfx)) {
		size_t len = sstrlen(pSfx);
		if(len < 2 || len > 6)
			ok = PPSetError(PPERR_INVUNIQSNSFXLEN, pSfx);
		else if(pSfx[len-1] < '1' || pSfx[len-1] > '9')
			ok = PPSetError(PPERR_INVUNIQSNSFX, pSfx);
		else
			ok = 1;
	}
	return ok;
}

int PPObjBill::ReleaseSerialFromUniqSuffix(SString & rSerial) const
{
	int    ok = -1;
	if(rSerial.NotEmpty()) {
		const size_t fmt_len = sstrlen(Cfg.UniqSerialSfx);
		const char nd_c = Cfg.UniqSerialSfx[fmt_len-1];
		if(fmt_len && nd_c >= '0' && nd_c <= '9') {
			const size_t nd = (size_t)(nd_c - '0');
			const size_t sfx_len = nd + fmt_len - 1;
			const size_t sn_len = rSerial.Len();
			if(sn_len > sfx_len && memcmp(rSerial.cptr()+sn_len-sfx_len, Cfg.UniqSerialSfx, fmt_len-1) == 0) {
				rSerial.Trim(sn_len-sfx_len);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjBill::AdjustSerialForUniq(PPID goodsID, PPID lotID, int checkOnly, SString & rSerial)
{
	int    ok = -1;
	SString adjusted_serial;
	if(rSerial.NotEmpty() && (checkOnly || VerifyUniqSerialSfx(Cfg.UniqSerialSfx) > 0)) {
		const size_t fmt_len = sstrlen(Cfg.UniqSerialSfx);
		const int    nd = Cfg.UniqSerialSfx[fmt_len-1] - '0';
		if(checkOnly || (nd >= 1 && nd <= 9)) {
			long   c = 0;
			int    found = 0;
			adjusted_serial = rSerial;
			PPIDArray lot_list;
			do {
				ReceiptTbl::Rec lot_rec;
				lot_list.clear();
				// @v9.5.6 SearchLotsBySerial(adjusted_serial, &lot_list);
				SearchLotsBySerialExactly(adjusted_serial, &lot_list); // @v9.5.6
				found = 0;
				for(uint i = 0; !found && i < lot_list.getCount(); i++) {
					const PPID lot_id = lot_list.get(i);
					if(lot_id != lotID && trfr->Rcpt.Search(lot_id, &lot_rec) > 0 && labs(lot_rec.GoodsID) == labs(goodsID)) {
						if(!checkOnly) {
							(adjusted_serial = rSerial).CatN(Cfg.UniqSerialSfx, fmt_len-1).CatLongZ(++c, nd);
							found = 1;
						}
						ok = 1;
					}
				}
			} while(!checkOnly && found);
		}
	}
	if(ok > 0 && !checkOnly)
		rSerial = adjusted_serial;
	return ok;
}

int PPObjBill::Helper_StoreClbList(PPBillPacket * pPack)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPObjTag * p_tag_obj = 0;
	const  int is_intrexpnd = IsIntrExpndOp(pPack->Rec.OpID);
	const  int do_force_unmirr = BIN(strstr(pPack->Rec.Memo, "#MIRROR-REFAB")); // @v9.5.5
	SString img_path;
	SString img_tag_addendum;
	SString fname;
	SString temp_buf;
	if(oneof3(pPack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_GOODSORDER) || is_intrexpnd) {
		PPTransferItem * p_ti;
		SString clb;
		ObjTagList mirror_tag_list;
		PPIDArray excl_tag_list;
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti);) {
			if(p_ti->LotID) {
				const int row_idx = (int)(i-1);
				const char * p_clb = 0;
				ObjTagList * p_tag_list = pPack->LTagL.Get(row_idx); // PPLotTagContainer
				if(p_ti->Flags & PPTFR_RECEIPT) {
					THROW(p_ref->Ot.PutListExcl(PPOBJ_LOT, p_ti->LotID, p_tag_list, &excl_tag_list, 0));
				}
				//
				// Сохраняем серийные номера и пользовательские теги для порожденных лотов
				//
				else if(is_intrexpnd) {
					if(trfr->SearchByBill(p_ti->BillID, 1, p_ti->RByBill, 0) > 0) {
						const PPID mirror_lot_id = trfr->data.LotID;
						if(mirror_lot_id) {
							ObjTagList * p_mirror_tag_list = pPack->P_MirrorLTagL ? pPack->P_MirrorLTagL->Get(row_idx) : 0;
							if(p_mirror_tag_list) {
								mirror_tag_list = *p_mirror_tag_list;
								uint   j = mirror_tag_list.GetCount();
								if(j) do {
                                    const ObjTagItem * p_item = mirror_tag_list.GetItemByPos(--j);
                                    THROW_MEM(SETIFZ(p_tag_obj, new PPObjTag));
                                    if(p_item && (!p_tag_obj->IsUnmirrored(p_item->TagID) || do_force_unmirr)) {
										mirror_tag_list.PutItem(p_item->TagID, 0);
                                    }
								} while(j);
							}
							else
								mirror_tag_list.Destroy();
							if(p_tag_list) {
								for(uint j = 0; j < p_tag_list->GetCount(); j++) {
                                    const ObjTagItem * p_item = p_tag_list->GetItemByPos(j);
                                    THROW_MEM(SETIFZ(p_tag_obj, new PPObjTag));
                                    if(p_item && (!p_tag_obj->IsUnmirrored(p_item->TagID) || do_force_unmirr)) {
										if(p_item->TagDataType == OTTYP_IMAGE) {
											ObjTagItem tag_item = *p_item;
											ObjLinkFiles _lf_src(PPOBJ_TAG);
											_lf_src.Load(p_item->TagID, p_ti->LotID);
											_lf_src.At(0, img_path);
											if(::fileExists(img_path)) {
												ObjLinkFiles _lf_dest(PPOBJ_TAG);
												_lf_dest.SetMode_IgnoreCheckStorageDir(1);
												_lf_dest.Replace(0, img_path);
												_lf_dest.SaveSingle(p_item->TagID, temp_buf.Z().Cat(mirror_lot_id), 0, &fname);
												tag_item.SetStr(p_item->TagID, fname);
												mirror_tag_list.PutItem(tag_item.TagID, &tag_item);
											}
										}
										else
											mirror_tag_list.PutItem(p_item->TagID, p_item);
                                    }
								}
							}
							THROW(p_ref->Ot.PutList(PPOBJ_LOT, mirror_lot_id, &mirror_tag_list, 0));
						}
					}
				}
			}
		}
	}
	else if(oneof3(pPack->OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
        SBuffer sbuf;
        if(pPack->LTagL.GetCount()) {
			SPathStruc sp;
			for(uint i = 0; i < pPack->GetTCount(); i++) {
				ObjTagList * p_tag_list = pPack->LTagL.Get(i);
				if(p_tag_list) {
					const PPTransferItem & r_ti = pPack->ConstTI(i);
					for(uint j = 0; j < p_tag_list->GetCount(); j++) {
						const ObjTagItem * p_tag_item = p_tag_list->GetItemByPos(j);
						if(p_tag_item->TagDataType == OTTYP_IMAGE) {
							ObjTagItem tag_item = *p_tag_item;
							//
							ObjLinkFiles _lf(PPOBJ_TAG);
							img_tag_addendum.Z().Cat(pPack->Rec.ID).CatChar('-').Cat(r_ti.RByBill);
							_lf.Load(tag_item.TagID, img_tag_addendum);
							if(sstrlen(tag_item.Val.PStr)) {
								fname = tag_item.Val.PStr;
								_lf.Replace(0, fname);
							}
							else
								_lf.Remove(0);
							_lf.SaveSingle(tag_item.TagID, img_tag_addendum, 0, &fname);
							tag_item.SetStr(tag_item.TagID, fname);
							p_tag_list->PutItem(tag_item.TagID, &tag_item);
						}
					}
				}
			}
			{
        		SSerializeContext sctx;
        		THROW(pPack->LTagL.Serialize(+1, sbuf, &sctx));
			}
        }
        THROW(p_ref->PutPropSBuffer(Obj, pPack->Rec.ID, BILLPRP_DRAFTTAGLIST, sbuf, 0));
	}
	THROW(SetTagList(pPack->Rec.ID, &pPack->BTagL, 0));
	if(P_LotXcT) {
		SBuffer cbuf;
		THROW(P_LotXcT->PutContainer(pPack->Rec.ID, &pPack->XcL, 0));
		// @v10.3.0 {
		if(pPack->_VXcL.GetCount()) {
			SBuffer vxcl_buf; // @v10.3.0
			SCompressor compr(SCompressor::tZLib);
			SSerializeContext sctx;
			THROW(pPack->_VXcL.Serialize(+1, vxcl_buf, &sctx));
			if(vxcl_buf.GetAvailableSize() > 128) {
				uint8 cs[32];
				size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
				THROW_SL(cbuf.Write(cs, cs_size));
				THROW_SL(compr.CompressBlock(vxcl_buf.GetBuf(0), vxcl_buf.GetAvailableSize(), cbuf, 0, 0));
			}
			else {
				cbuf = vxcl_buf;
			}
		}
		THROW(p_ref->PutPropSBuffer(Obj, pPack->Rec.ID, BILLPRP_VALXCL, cbuf, 0));
		// } @v10.3.0
	}
	CATCHZOK
	delete p_tag_obj;
	return ok;
}

int PPObjBill::FillTurnList(PPBillPacket * pPack)
{
	int    ok = 1;
	PPAccTurnTemplArray att_list;
	PPAccTurnTempl * p_att;
	pPack->Turns.freeAll();
	if(!(pPack->Rec.Flags & BILLF_NOATURN)) {
		THROW(PPObjOprKind::GetATTemplList(pPack->Rec.OpID, &att_list));
		for(uint i = 0; att_list.enumItems(&i, (void **)&p_att);) {
			if(!(p_att->Flags & ATTF_PASSIVE)) {
				if(p_att->Flags & ATTF_BASEPROJECTION) {
					THROW(p_att->CreateBaseProjectionAccturns(pPack));
				}
				else
					THROW(p_att->CreateAccturns(pPack));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::ProcessLink(BillTbl::Rec & rRec, PPID paymLinkID, const BillTbl::Rec * pOrgRec)
{
	int    ok = 1;
	if(rRec.OpID) {
		SString msg_buf;
		BillTbl::Rec link_rec;
		PPID   link_id = rRec.LinkBillID;
		double amount = BR2(rRec.Amount);
		double org_amount = 0.0;
		const PPID paym_t = IsOpPaymOrRetn(rRec.OpID);
		if(paym_t) {
			const int org_lock_paym = BIN(!pOrgRec || (pOrgRec->StatusID && CheckStatusFlag(pOrgRec->StatusID, BILSTF_LOCK_PAYMENT)));
			const int new_lock_paym = BIN(pOrgRec == &rRec || (rRec.StatusID && CheckStatusFlag(rRec.StatusID, BILSTF_LOCK_PAYMENT)));
			//
			// Следующая конструкция должна охватить все четыре комбинации org_lock_paym = 0|1; new_lock_paym = 0|1 {
			//
			if(org_lock_paym) {
				org_amount = 0.0;
				if(new_lock_paym)
					amount = 0.0;
			}
			else if(!new_lock_paym) {
				org_amount = BR2(pOrgRec->Amount);
				amount = rRec.Amount;
			}
			else
				org_amount = 2.0 * BR2(rRec.Amount);
			// }
		}
		if(link_id) {
			THROW_PP_S(P_Tbl->Search(link_id, &link_rec) > 0, PPERR_LINKBILLNFOUND, msg_buf.Z().Cat(link_id));
			THROW_PP_S(rRec.Dt >= link_rec.Dt, PPERR_LNKBILLDT, PPObjBill::MakeCodeString(&link_rec, 1, msg_buf));
			//
			// Если добавляемый документ - оплата или возврат, то в связанном
			// документе (если он требует оплаты) изменяем оплаченную сумму
			//
			if(paym_t) {
				if(CcFlags & CCFLG_SETWLONLINK)
					SETFLAG(rRec.Flags, BILLF_WHITELABEL, P_Tbl->data.Flags & BILLF_WHITELABEL);
				if(P_Tbl->data.Flags & BILLF_NEEDPAYMENT) {
					const int is_neg = (paym_t == PPOPT_CHARGE && CheckOpFlags(rRec.OpID, OPKF_CHARGENEGPAYM));
					THROW(P_Tbl->UpdatePaymAmount(link_id, rRec.CurID, (is_neg ? -amount : amount), (is_neg ? -org_amount : org_amount)));
				}
			}
			else if(IsDraftOp(link_rec.OpID)) {
				if(pOrgRec == 0 && !(rRec.Flags2 & BILLF2_DONTCLOSDRAFT)) {
					THROW(P_Tbl->SetRecFlag(link_id, BILLF_WRITEDOFF, 1, 0));
				}
			}
		}
		if(paymLinkID && paym_t == PPOPT_PAYMENT) {
			THROW_PP_S(P_Tbl->Search(paymLinkID, &link_rec) > 0, PPERR_LINKBILLNFOUND, msg_buf.Z().Cat(link_id));
			const int is_neg = BIN(link_rec.Amount < 0.0); // @v10.3.2
			THROW(P_Tbl->UpdatePaymAmount(paymLinkID, rRec.CurID, (is_neg ? -amount : amount), (is_neg ? -org_amount : org_amount)));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::SetupModifPacket(PPBillPacket * pPack)
{
	int    ok = 1;
	if(pPack->OpTypeID == PPOPT_GOODSMODIF) {
		enum {
			mhPlus  = 0x0001,
			mhMinus = 0x0002
		};
		uint   mh = 0;
		PPTransferItem * p_ti;
		uint   i = 0;
		int    diff  = 0;
		PPID   suppl_id = 0;
		PPID   prev_suppl_id = 0;
		ArticleTbl::Rec ar_rec;
		for(i = 0; /* !diff && */ pPack->EnumTItems(&i, &p_ti);) {
			if(p_ti->Flags & PPTFR_MINUS) {
				mh |= mhMinus;
				if(p_ti->Suppl && prev_suppl_id && p_ti->Suppl != prev_suppl_id)
					diff = 1;
				else if(p_ti->Suppl)
					prev_suppl_id = p_ti->Suppl;
			}
			else if(p_ti->Flags & PPTFR_PLUS)
				mh |= mhPlus;
		}
		if((mh & (mhPlus|mhMinus)) != (mhPlus|mhMinus)) {
            PPOprKind op_rec;
            if(GetOpData(pPack->Rec.OpID, &op_rec) > 0) {
				THROW_PP(!(op_rec.ExtFlags & OPKFX_DSBLHALFMODIF), PPERR_HALFMODIFBILLDISABLED);
            }
		}
		if(diff || !prev_suppl_id) {
			THROW(ArObj.GetMainOrgAsSuppl(&suppl_id));
		}
		else if(pPack->Rec.Object && ArObj.Fetch(pPack->Rec.Object, &ar_rec) > 0 && ar_rec.AccSheetID == GetSupplAccSheet())
			suppl_id = pPack->Rec.Object;
		else
			suppl_id = prev_suppl_id;
		for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
			if(p_ti->Flags & PPTFR_RECEIPT) {
				p_ti->Suppl = suppl_id;
				p_ti->Flags |= PPTFR_FORCESUPPL;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::ProcessACPacket(PPBillPacket * pack)
{
	int    ok = 1;
	if(pack->P_ACPack && pack->P_ACPack->GetTCount()) {
		pack->P_ACPack->SetQuantitySign(-1);
		THROW(pack->P_ACPack->InitAmounts(0));
		THROW(FillTurnList(pack->P_ACPack));
		THROW(TurnPacket(pack->P_ACPack, 0));
	}
	CATCHZOK
	return ok;
}

int PPBillPacket::CreateShadowPacket(PPBillPacket * pShadow)
{
	int    ok = -1;
	PPID   order = 0;
	PPTransferItem * p_ti, ti;
	if(P_ShLots) {
		for(uint i = 0; P_ShLots->enumItems(&i, (void **)&p_ti);) {
			if(!pShadow) {
				p_ti->TFlags &= ~PPTransferItem::tfDirty;
				order = 1;
			}
			else if(!(p_ti->TFlags & PPTransferItem::tfDirty) && oneof2(order, 0, p_ti->BillID) && R6(p_ti->Quantity_) != 0.0) {
			   	if(!order) {
					ReceiptTbl::Rec ord_lot_rec;
					THROW(P_BObj->trfr->Rcpt.Search(p_ti->LotID, &ord_lot_rec) > 0);
				   	THROW(pShadow->CreateBlank(0, Rec.ID, 0, 1));
					pShadow->Rec.LocID = ord_lot_rec.LocID;
					pShadow->Rec.Dt = Rec.Dt;
					order = pShadow->Rec.Object = p_ti->BillID;
				}
	   	        ti = *p_ti;
				ti.BillID = ti.OrdLotID; // @ordlotid (? почему ti.BillID ?)
				if(pShadow->Rec.ID == 0)
					pShadow->Rec.ID = ti.BillID;
				else if(ti.BillID == 0)
					ti.BillID = pShadow->Rec.ID;
				else {
					THROW_PP(pShadow->Rec.ID == ti.BillID, PPERR_SHADOWIDFAULT);
				}
				ti.OrdLotID = 0; // @ordlotid
				THROW(ti.SetupGoods(-labs(p_ti->GoodsID), 0));
		   		ti.Quantity_ = -fabs(p_ti->Quantity_);
				ti.Flags   |= PPTFR_SHADOW;
				ti.TFlags  &= ~PPTransferItem::tfDirty;
   		        THROW(pShadow->InsertRow(&ti, 0));
				p_ti->TFlags |= PPTransferItem::tfDirty;
			}
		}
	}
	ok = order ? 1 : -1;
	CATCHZOK
	return ok;
}

int PPObjBill::ProcessShadowPacket(PPBillPacket * pPack, int doUpdate)
{
	int    ok = 1, r;
	uint   pos;
	ReceiptTbl::Rec lot_rec;
	PPIDArray old_shadow_bills;
	PPIDArray new_shadow_bills; // @v9.5.3
	PPIDArray orders;
	PPTransferItem ti;
	if(pPack->Rec.ID) {
		for(DateIter di; (r = P_Tbl->EnumLinks(pPack->Rec.ID, &di, BLNK_SHADOW)) > 0;) {
			const  PPID bill_id = P_Tbl->data.ID;
			int    rbybill = 0;
			THROW_SL(old_shadow_bills.add(bill_id));
			while(trfr->EnumItems(bill_id, &rbybill, &ti) > 0) {
				if(ti.Flags & PPTFR_SHADOW && ti.LotID && trfr->Rcpt.Search(ti.LotID, &lot_rec) > 0)
					THROW_SL(orders.addUnique(lot_rec.BillID));
			}
		}
	}
	{
		THROW(r = pPack->CreateShadowPacket(0));
		if(r > 0) {
			PPBillPacket shadow;
			while((r = pPack->CreateShadowPacket(&shadow)) > 0) {
				/* @v9.5.3
				if(shadow.Rec.ID == 0)
					update = 0;
				THROW(update ? UpdatePacket(&shadow, 0) : TurnPacket(&shadow, 0));
				*/
				// @v9.5.3 {
				if(doUpdate && shadow.Rec.ID) {
					THROW(UpdatePacket(&shadow, 0));
				}
				else {
					THROW(TurnPacket(&shadow, 0));
				}
				// } @v9.5.3
				new_shadow_bills.add(shadow.Rec.ID); // @v9.5.3
				// @v9.5.3 old_shadow_bills.freeByKey(shadow.Rec.ID, 0);
				THROW_SL(orders.addUnique(shadow.Rec.Object));
			}
			THROW(r);
		}
	}
	for(pos = 0; pos < old_shadow_bills.getCount(); pos++) {
		const PPID bill_id_to_remove = old_shadow_bills.get(pos);
		if(!new_shadow_bills.lsearch(bill_id_to_remove)) {
			THROW(RemovePacket(bill_id_to_remove, 0));
		}
	}
	for(pos = 0; pos < orders.getCount(); pos++) {
		const  PPID order_id = orders.get(pos);
		int    rbybill = 0;
		int    closed = 1;
		while(closed && trfr->EnumItems(order_id, &rbybill, &ti) > 0) {
			if(ti.Flags & PPTFR_ORDER && ti.LotID && trfr->Rcpt.Search(ti.LotID, &lot_rec) > 0)
				closed = BIN(lot_rec.Closed);
		}
		//
		// Флаг BILLF_CLOSEDORDER (Закрытый заказ) здесь не следует снимать - только устанавливать.
		// Связано это с тем, что данный признак мог быть установлен в ручную.
		//
		if(closed) {
			THROW(P_Tbl->SetRecFlag(order_id, BILLF_CLOSEDORDER, closed, 0));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::LockFRR(LDATE dt, int * pFRRL_Tag, int use_ta)
{
	int    ok = -1;
	const  int16 frrl_days = CConfig.FRRL_Days;
	if(frrl_days) {
		const LDATE _cd = LConfig.OperDate;
		if(diffdate(&_cd, &dt, 0) >= frrl_days)
			ok = atobj->P_Tbl->LockingFRR(1, pFRRL_Tag, use_ta);
	}
	return ok;
}

int PPObjBill::UnlockFRR(int * pFRRL_Tag, int isCrash, int use_ta)
{
	return atobj->P_Tbl->LockingFRR(isCrash ? -1 : 0, pFRRL_Tag, use_ta);
}

int PPObjBill::GenPckgCode(PPID pckgTypeID, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	char   code[32];
	code[0] = 0;
	if(CcFlags & CCFLG_USEGOODSPCKG && pckgTypeID) {
		PPObjPckgType pt_obj;
		PPGdsPckgType pt_rec;
		if(pt_obj.Get(pckgTypeID, &pt_rec) > 0) {
			long cntr = pt_rec.Counter;
			for(int r = -1; r < 0;) {
				pt_obj.CodeByTemplate(pt_rec.CodeTempl, cntr+1, code, sizeof(code));
				THROW(r = P_PckgT->CheckCodeUnique(pckgTypeID, 0L, code, 0));
				if(r < 0)
					cntr++;
				else if(r > 0) {
					pt_rec.Counter = cntr+1;
					THROW(pt_obj.Put(&pckgTypeID, &pt_rec, 1));
				}
			}
		}
	}
	CATCHZOK
	strnzcpy(pBuf, code, bufLen);
	return ok;
}

int PPObjBill::InitPckg(LPackage * pPckg)
{
	int    ok = 1;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		PPObjPckgType pt_obj;
		pPckg->Init();
		PPID   pt_id = GObj.GetConfig().DefPckgTypeID;
		if(pt_id > 0 || (pt_id = pt_obj.GetSingle()) > 0) {
			PPGdsPckgType pt_rec;
			if(pt_obj.Get(pt_id, &pt_rec) > 0) {
				pPckg->PckgTypeID = pt_id;
				SETFLAG(pPckg->Flags, PCKGF_UNIQUECODE, pt_rec.Flags & GF_UNIQPCKGCODE);
				GenPckgCode(pPckg->PckgTypeID, pPckg->Code, sizeof(pPckg->Code));
			}
		}
	}
	return ok;
}

int PPObjBill::IsLotInPckg(PPID lotID)
{
	return BIN(CcFlags & CCFLG_USEGOODSPCKG && lotID && P_PckgT->GetLotLink(lotID, 0, 0) > 0);
}

int PPObjBill::CheckPckgCodeUnique(const LPackage * pPckg, PPBillPacket * pPack)
{
	if((pPack && pPack->P_PckgList &&
		!pPack->P_PckgList->CheckCodeUnique(pPckg->PckgTypeID, pPckg->Code, pPckg->PckgIdx)) ||
		(P_PckgT && (P_PckgT->CheckCodeUnique(pPckg->PckgTypeID, pPckg->ID, pPckg->Code, 0) < 0)))
		return 0;
	return 1;
}

int PPObjBill::PutPckgList(PPBillPacket * pPack, int use_ta)
{
	int    ok = 1;
	uint   i, j;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		THROW(pPack->InitPckg());
		if(pPack->P_PckgList) {
			LPackage * p_pckg;
			ReceiptTbl::Rec lot_rec;
			PPTransaction tra(use_ta);
			THROW(tra);
			for(i = 0; pPack->P_PckgList->EnumItems(&i, &p_pckg);) {
				if(trfr->Rcpt.Search(p_pckg->ID, &lot_rec) > 0) {
					p_pckg->Closed = (lot_rec.Rest <= 0) ? 1 : 0;
					p_pckg->LocID  = lot_rec.LocID;
					THROW(P_PckgT->PutPckg(p_pckg->ID, p_pckg, 0));
					if(IsIntrExpndOp(pPack->Rec.OpID)) {
						int    idx = 0;
						PPID   lot_id = 0;
						PPTransferItem * p_ti = 0;
						ReceiptTbl::Rec lot_rec;
						LPackage mirror;
					   	mirror.PckgIdx = p_pckg->PckgIdx;
					   	mirror.PckgTypeID = p_pckg->PckgTypeID;
					   	memcpy(mirror.Code, p_pckg->Code, sizeof(p_pckg->Code));
					   	mirror.UniqCntr = 0;
					   	mirror.Flags  = (p_pckg->Flags | PCKGF_MIRROR);
				   		LDATE  dt = ZERODATE;
						long   oprno = 0;
				   		while(trfr->Rcpt.EnumRefs(p_pckg->ID, &dt, &oprno, &lot_rec) > 0)
				   			if(lot_rec.BillID == pPack->Rec.ID) {
				   				mirror.ID    = lot_rec.ID;
			   					mirror.LocID = lot_rec.LocID;
			   					break;
			   				}
						THROW_PP(mirror.ID, PPERR_INVPCKGIDS);
					   	mirror.PrevID = p_pckg->ID;
					   	for(j = 0; p_pckg->EnumItems(&j, &idx, &lot_id) > 0;) {
							THROW(pPack->ChkTIdx(idx));
							p_ti = & pPack->TI(idx);
							if(trfr->SearchByBill(p_ti->BillID, 1, p_ti->RByBill, 0) > 0)
								mirror.AddItem(trfr->data.LotID, idx);
				   		}
						THROW(P_PckgT->PutPckg(mirror.ID, &mirror, 0));
					}
				}
				else
					THROW(P_PckgT->PutPckg(p_pckg->ID, 0, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::LoadPckgList(PPBillPacket * pPack)
{
	int    ok = -1;
	LPackageList * p_pckg_list = 0;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		int    is_intrexpnd = IsIntrExpndOp(pPack->Rec.OpID);
		uint   i = 0;
		PPTransferItem * p_ti;
		THROW_MEM(p_pckg_list = new LPackageList);
		while(pPack->EnumTItems(&i, &p_ti)) {
			if(p_ti->Flags & PPTFR_PCKG) {
				uint  j, lot_pos;
				int   idx;
				PPID  lot_id = 0;
				LPackage pckg;
				if(is_intrexpnd) {
					THROW(trfr->SearchByBill(pPack->Rec.ID, 1, p_ti->RByBill, 0) > 0);
					lot_id = trfr->data.LotID;
				}
				else
					lot_id = p_ti->LotID;
				THROW(P_PckgT->GetPckg(lot_id, &pckg) > 0);
				pckg.PckgIdx = i-1;
				pckg.Cost    = p_ti->Cost;
				pckg.Price   = p_ti->Price;
				for(j = 0; pckg.EnumItems(&j, &idx, &lot_id);) {
					int found = 0;
					//
					// If package mounted in this bill or transfered to other location,
					// then relink package item to source lots
					//
					if(p_ti->Flags & PPTFR_MODIF || is_intrexpnd) {
						ReceiptTbl::Rec lot_rec;
						THROW_PP(trfr->Rcpt.Search(lot_id, &lot_rec) > 0 &&
							lot_rec.PrevLotID /*&& lot_rec.BillID == pPack->Rec.ID*/, PPERR_INVLOTREFINPCKG);
						lot_id = lot_rec.PrevLotID;
					}
					for(lot_pos = 0; !found && pPack->SearchLot(lot_id, &lot_pos);)
						if(pPack->ConstTI(lot_pos).Flags & PPTFR_PCKGGEN) {
							pckg.UpdateItem(j-1, lot_pos, lot_id);
							found = 1;
						}
					THROW_PP(found, PPERR_INVLOTREFINPCKG);
				}
				THROW(p_pckg_list->Add(&pckg));
			}
		}
		delete pPack->P_PckgList;
		pPack->P_PckgList = p_pckg_list;
		pPack->CalcPckgTotals();
	}
	CATCHZOK
	return ok;
}

int PPObjBill::SearchAdvLinkToBill(PPID billID, AdvBillItemTbl::Rec * pItemRec, BillTbl::Rec * pBillRec)
{
	int    ok = -1;
	if(CcFlags & CCFLG_USEADVBILLITEMS && P_AdvBI && billID) {
		AdvBillItemTbl::Key2 k2;
		MEMSZERO(k2);
		k2.AdvBillID = billID;
		if(P_AdvBI->search(2, &k2, spGe) && k2.AdvBillID == billID) {
			P_AdvBI->copyBufTo(pItemRec);
			if(pBillRec)
				Search(P_AdvBI->data.BillID, pBillRec);
			ok = 1;
		}
	}
	return ok;
}

int PPObjBill::LoadAdvList(PPID billID, PPID opID, PPAdvBillItemList * pList)
{
	int    ok = 1;
	pList->Clear();
	if(CcFlags & CCFLG_USEADVBILLITEMS && P_AdvBI && billID) {
		PPOprKind op_rec;
		if(GetOpType(opID, &op_rec) == PPOPT_ACCTURN && op_rec.Flags & OPKF_ADVACC) {
			AdvBillItemTbl::Key0 k0;
			k0.BillID  = billID;
			k0.RByBill = 0;
			while(P_AdvBI->search(0, &k0, spGt) && k0.BillID == billID)
				THROW(pList->AddStorageForm(&P_AdvBI->data));
			THROW_DB(BTROKORNFOUND);
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::TurnAdvList(PPID billID, PPBillPacket * pPack, int use_ta)
{
	int    ok = 1;
	if(CcFlags & CCFLG_USEADVBILLITEMS && P_AdvBI && billID) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(P_AdvBI, 0, (P_AdvBI->BillID == billID)));
		if(pPack) {
			for(uint i = 0; i < pPack->AdvList.GetCount(); i++) {
				AdvBillItemTbl::Rec abi_rec;
				pPack->AdvList.Get(i).BillID = billID;
				pPack->AdvList.Get(i).RByBill = i+1;
				THROW(pPack->AdvList.GetStorageForm(i, &abi_rec));
				THROW_DB(P_AdvBI->insertRecBuf(&abi_rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::CheckPoolStatus(PPID billID, int poolType)
{
	int    ok = 1;
	PPID   owner_id = 0;
	if(IsMemberOfPool(billID, poolType, &owner_id) > 0) {
		BillTbl::Rec bill_rec;
		if(Search(owner_id, &bill_rec) > 0) {
			THROW_PP(!bill_rec.StatusID || !CheckStatusFlag(bill_rec.StatusID, BILSTF_DENY_CHANGELINK), PPERR_BILLST_DENY_CHANGELINK);
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::CheckParentStatus(PPID billID)
{
	int    ok = 1;
	BillTbl::Rec brec;
	if(Search(billID, &brec) > 0 && brec.LinkBillID && Search(brec.LinkBillID, &brec) > 0) {
		THROW_PP(!brec.StatusID || !CheckStatusFlag(brec.StatusID, BILSTF_DENY_CHANGELINK), PPERR_BILLST_DENY_CHANGELINK);
	}
	THROW(CheckPoolStatus(billID, PPASS_PAYMBILLPOOL));
	THROW(CheckPoolStatus(billID, PPASS_OPBILLPOOL));
	//THROW(CheckPoolStatus(billID, PPASS_CSESSBILLPOOL));
	//THROW(CheckPoolStatus(billID, PPASS_TSESSBILLPOOL));
	CATCHZOK
	return ok;
}

int PPObjBill::PutSCardOp(PPBillPacket * pPack, int use_ta)
{
	int    ok = 1, r = 0;
	int    set_flag = 0;
	double amount = 0.0;
	PPID   scard_id = 0;
	PPSCardConfig sc_cfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack->Rec.SCardID) {
			if(pPack->OpTypeID == PPOPT_PAYMENT) {
				scard_id = pPack->Rec.SCardID;
				amount = -pPack->Rec.Amount;
			}
			else if(PPObjSCard::FetchConfig(&sc_cfg) > 0 && sc_cfg.ChargeAmtID) {
				if(pPack->Amounts.Get(sc_cfg.ChargeAmtID, pPack->Rec.CurID, &amount) > 0)
					scard_id = pPack->Rec.SCardID;
			}
			if((scard_id && amount != 0.0) || pPack->Rec.Flags & BILLF_SCARDOP) {
				SETIFZ(P_ScObj, new PPObjSCard);
				if(P_ScObj && P_ScObj->P_Tbl) {
					PPObjSCardSeries scs_obj;
					PPSCardSeries scs_rec;
					SCardTbl::Rec sc_rec;
					THROW(P_ScObj->Search(scard_id, &sc_rec) > 0);
					if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.Flags & SCRDSF_CREDIT) {
						THROW(r = P_ScObj->P_Tbl->PutOpByBill(pPack->Rec.ID, scard_id, pPack->Rec.Dt, amount, 0));
						if(r == 2)
							set_flag = 1;
					}
				}
			}
		}
		if((set_flag && !(pPack->Rec.Flags & BILLF_SCARDOP)) || (!set_flag && (pPack->Rec.Flags & BILLF_SCARDOP))) {
			THROW(P_Tbl->SetRecFlag(pPack->Rec.ID, BILLF_SCARDOP, set_flag, 0));
			SETFLAG(pPack->Rec.Flags, BILLF_SCARDOP, set_flag);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PPObjBill::TBlock::TBlock() : BillID(0), OrgLastRByBill(0), CurRByBill(0)
{
}

int16 PPObjBill::TBlock::GetNewRbb()
{
	return (++CurRByBill);
}

int PPObjBill::BeginTFrame(PPID billID, TBlock & rBlk)
{
	int    ok = 1;
	BillTbl::Rec bill_rec;
	if(billID && Search(billID, &bill_rec) > 0) {
		rBlk.BillID = billID;
		rBlk.CurRByBill = rBlk.OrgLastRByBill = bill_rec.LastRByBill;
	}
	else {
		rBlk.BillID = 0;
		rBlk.CurRByBill = rBlk.OrgLastRByBill = 0;
	}
	return ok;
}

int PPObjBill::FinishTFrame(PPID billID, TBlock & rBlk)
{
	int    ok = 1;
	assert(billID != 0);
	assert(rBlk.BillID == 0 || rBlk.BillID == billID);
	assert(rBlk.CurRByBill >= rBlk.OrgLastRByBill);
	rBlk.BillID = billID;
	if(rBlk.BillID && rBlk.CurRByBill != rBlk.OrgLastRByBill) {
		BillTbl::Rec rec;
		THROW(SearchByID_ForUpdate(P_Tbl, PPOBJ_BILL, rBlk.BillID, &rec) > 0);
		rec.LastRByBill = rBlk.CurRByBill;
		THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
	}
	CATCHZOK
	return ok;
}

static int FASTCALL GetBillOpUserProfileFunc(PPID opID, int action)
{
	int    func_id = 0;
	if(opID == 0) {
		switch(action) {
			case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_RCPT; break;
			case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_RCPT; break;
			case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_RCPT; break;
		}
	}
	else {
		const  PPID op_type = GetOpType(opID);
		switch(op_type) {
			case PPOPT_DRAFTRECEIPT:
			case PPOPT_DRAFTEXPEND:
			case PPOPT_DRAFTTRANSIT:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_DRFT; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_DRFT; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_DRFT; break;
				}
				break;
			case PPOPT_GOODSRECEIPT:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_RCPT; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_RCPT; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_RCPT; break;
				}
				break;
			case PPOPT_GOODSEXPEND:
				if(IsIntrExpndOp(opID)) {
					switch(action) {
						case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_IEXP; break;
						case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_IEXP; break;
						case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_IEXP; break;
					}
				}
				else {
					switch(action) {
						case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_EXP; break;
						case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_EXP; break;
						case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_EXP; break;
					}
				}
				break;
			case PPOPT_GOODSRETURN:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_RET; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_RET; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_RET; break;
				}
				break;
			case PPOPT_GOODSREVAL:
			case PPOPT_CORRECTION:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_RVL; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_RVL; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_RVL; break;
				}
				break;
			case PPOPT_GOODSORDER:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_ORD; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_ORD; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_ORD; break;
				}
				break;
			case PPOPT_GOODSMODIF:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_MOD; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_MOD; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_MOD; break;
				}
				break;
			/*
			case PPOPT_CASHSESS:
			case PPOPT_WAREHOUSE:
			case PPOPT_EXTERNAL:
			case PPOPT_ACCTURN:
			case PPOPT_PAYMENT:
			case PPOPT_CHARGE:
			case PPOPT_AGREEMENT:
			case PPOPT_GOODSACK: // ?
			case PPOPT_INVENTORY:
			case PPOPT_POOL:
			*/
			default:
				switch(action) {
					case PPACN_TURNBILL: func_id = PPUPRF_BILLTURN_ETC; break;
					case PPACN_UPDBILL:  func_id = PPUPRF_BILLUPD_ETC; break;
					case PPACN_RMVBILL:  func_id = PPUPRF_BILLRMV_ETC; break;
				}
				break;
		}
	}
	return func_id;
}

struct BillUserProfileCounter {
	BillUserProfileCounter()
	{
		THISZERO();
	}
	double FASTCALL CalcFactor(uint factorN) const
	{
		double val = 0.0;
		if(factorN == 0) {
			val += (TiAddCount + TiUpdCount + TiRmvCount) + 1.5 * (AtAddCount + AtUpdCount + AtRmvCount);
		}
		return val;
	}
	uint   TiAddCount;
	uint   TiUpdCount;
	uint   TiRmvCount;
	uint   AtAddCount;
	uint   AtUpdCount;
	uint   AtRmvCount;
};

int PPObjBill::TurnPacket(PPBillPacket * pPack, int use_ta)
{
	uint   i;
	int    ok = 1;
	int    ta = 0;
	int    r;
	int    frrl_tag = 0;
	TBlock tb_;
	uint   pos;
	PPID   id = 0;
	SString wait_msg;
	PPTransferItem * pti = 0;
	PPAccTurn * pat = 0;
	BillUserProfileCounter ufp_counter;
	PPUserFuncProfiler ufp(GetBillOpUserProfileFunc(pPack->Rec.OpID, PPACN_TURNBILL));
	PPIDArray correction_exp_chain;
	const PPTrfrArray preserve_lots(pPack->GetLots()); // Сохраняем строки на случай аварии в проведении документа
	pPack->ErrCause = pPack->ErrLine = 0;
	if(pPack->Rec.OpID) { // Для теневого документа не проверяем период доступа
		THROW(ObjRts.CheckBillDate(pPack->Rec.Dt));
		// @v10.2.3 THROW(ObjRts.CheckOpID(pPack->Rec.OpID, PPR_INS));
		THROW(CheckRightsWithOp(pPack->Rec.OpID, PPR_INS)); // @v10.2.3
		if(pPack->OpTypeID == PPOPT_CORRECTION)
			GetCorrectionBackChain(pPack->Rec, correction_exp_chain);
	}
	if(!(State2 & stDemoRestrictInit)) {
		uint   major, minor, revision;
		char   demo[32];
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetVersion(&major, &minor, &revision, demo);
		SETFLAG(State2, stDemoRestrict, (demo[0] ? 1 : DS.CheckStateFlag(CFGST_DEMOMODE)));
		State2 |= stDemoRestrictInit;
	}
	if(State2 & stDemoRestrict) {
		RECORDNUMBER num_recs;
		P_Tbl->getNumRecs(&num_recs);
		THROW_PP(num_recs <= 1000, PPERR_BILLDEMORESTRICT);
	}
	if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
		PPLoadText(PPTXT_WAIT_TURNBILLTRFR, wait_msg);
	THROW(SetupModifPacket(pPack));
	THROW(SetupSpecialAmounts(pPack));
	{
		THROW(PPStartTransaction(&ta, use_ta));
		THROW(BeginTFrame(0, tb_));
		THROW(LockFRR(pPack->Rec.Dt, &frrl_tag, 0));
		THROW(ProcessLink(pPack->Rec, pPack->PaymBillID, 0));
		THROW(ProcessACPacket(pPack));
		THROW(P_Tbl->Edit(&id, pPack, 0));
		pPack->Rec.ID = id;
		THROW(CheckParentStatus(pPack->Rec.ID));
		pPack->ErrCause = PPBillPacket::err_on_line;
		if(pPack->IsDraft()) {
			if(P_CpTrfr) {
				SString clb;
				const int zero_rbybill = (pPack->ProcessFlags & PPBillPacket::pfForceRByBill) ? 0 : 1;
				for(i = 0; pPack->EnumTItems(&i, &pti);) {
					pPack->ErrLine = i-1;
					CpTrfrExt cte;
					pPack->LTagL.GetNumber(PPTAG_LOT_CLB, i-1, clb);
					STRNSCPY(cte.Clb, clb);
					pPack->LTagL.GetNumber(PPTAG_LOT_SN, i-1, clb);
					STRNSCPY(cte.PartNo, clb);
					// @v10.5.8 {
					cte.LinkBillID = pti->Lbr.ID; 
					cte.LinkRbb = pti->Lbr.RByBill;
					if(pti->TFlags & PPTransferItem::tfQrSeqAccepted)
						cte.QrSeqAckStatus = 1;
					else if(pti->TFlags & PPTransferItem::tfQrSeqRejected)
						cte.QrSeqAckStatus = 2;
					// } @v10.5.8 
					THROW(pti->Init(&pPack->Rec, zero_rbybill));
					THROW(P_CpTrfr->PutItem(pti, (zero_rbybill ? 0 : pti->RByBill), &cte, 0));
					ufp_counter.TiAddCount++;
					if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
						PPWaitPercent(i, pPack->GetTCount(), wait_msg);
				}
			}
		}
		else {
			const int zero_rbybill = ((pPack->ProcessFlags & PPBillPacket::pfForeignSync &&
				pPack->Rec.Flags2 & BILLF2_FULLSYNC) || (pPack->ProcessFlags & PPBillPacket::pfForceRByBill)) ? 0 : 1;
			for(i = 0; pPack->EnumTItems(&i, &pti);) {
				PPID   ac_link_lot_id = 0;
				pPack->ErrLine = i-1;
				if(pPack->P_Outer && pti->LotID < 0) { // Inner packet
					ac_link_lot_id = pti->LotID;
					pti->LotID = 0;
				}
				if(pti->Flags & PPTFR_ORDER)
					SETFLAG(pti->Flags, PPTFR_CLOSEDORDER, pPack->Rec.Flags & BILLF_CLOSEDORDER);
				const long preserve_tflags = pti->TFlags;
				if(pti->Init(&pPack->Rec, (zero_rbybill && !pti->IsCorrectionExp()))) {
					SETFLAGBYSAMPLE(pti->TFlags, PPTransferItem::tfForceNew,   preserve_tflags);
					SETFLAGBYSAMPLE(pti->TFlags, PPTransferItem::tfForceLotID, preserve_tflags);
					THROW(trfr->PreprocessCorrectionExp(*pti, correction_exp_chain));
					r = trfr->AddItem(pti, tb_.Rbb(), 0);
					ufp_counter.TiAddCount++;
				}
				else {
					r = 0;
				}
				pti->Flags &= ~PPTFR_CLOSEDORDER;
				if(ac_link_lot_id) { // Inner packet
					if(r) {
						pti->ACLinkLotID = ac_link_lot_id;
						for(pos = 0; pPack->P_Outer->SearchLot(ac_link_lot_id, &pos) > 0; pos++)
							pPack->P_Outer->TI(pos).LotID = pti->LotID;
					}
					else
						pti->LotID = ac_link_lot_id;
				}
				THROW(r);
				if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
					PPWaitPercent(i, pPack->GetTCount(), wait_msg);
			}
		}
		THROW(PutPckgList(pPack, 0));
		THROW(TurnAdvList(id, pPack, 0));
		THROW(pPack->LnkFiles.WriteToProp(pPack->Rec.ID, 0));
		if(!(pPack->Rec.Flags & BILLF_NOATURN) && !(CcFlags & CCFLG_DISABLEACCTURN)) {
			pPack->ErrCause = PPBillPacket::err_on_accturn;
			for(i = 0; pPack->Turns.enumItems(&i, (void **)&pat);) {
				pPack->ErrLine = i-1;
				pat->BillID = id;
				THROW(atobj->P_Tbl->Turn(pat, 0));
				ufp_counter.AtAddCount++;
			}
		}
		pPack->ErrCause = pPack->ErrLine = 0;
		THROW(ProcessShadowPacket(pPack, 0));
		if(pPack->PaymBillID)
			THROW(P_Tbl->UpdatePool(pPack->Rec.ID, PPASS_PAYMBILLPOOL, pPack->PaymBillID, 0));
		if(pPack->CSessID) {
			long   pool_type = 0;
			if(pPack->Rec.Flags & BILLF_CSESSWROFF)
				pool_type = PPASS_CSESSBILLPOOL;
			else if(pPack->Rec.Flags & BILLF_CDFCTWROFF)
				pool_type = PPASS_CSDBILLPOOL;
			else if(pPack->Rec.Flags & BILLF_TSESSWROFF || pPack->Rec.Flags2 & BILLF2_TSESSPAYM)
				pool_type = PPASS_TSESSBILLPOOL;
			else if(pPack->Rec.Flags & BILLF_TDFCTWROFF)
				pool_type = PPASS_TSDBILLPOOL;
			if(pool_type)
				THROW(P_Tbl->UpdatePool(pPack->Rec.ID, pool_type, pPack->CSessID, 0));
		}
		THROW(Helper_StoreClbList(pPack));
		THROW(PutSCardOp(pPack, 0));
		THROW(UnlockFRR(&frrl_tag, 0, 0));
		THROW(FinishTFrame(pPack->Rec.ID, tb_));
		{
			TSVector <PPCheckInPersonItem> * p_cip_list = pPack->CipB.P_CipList;
			const uint _c = SVectorBase::GetCount(p_cip_list);
			if(_c && pPack->CipB.P_TSesObj) {
				PPIDArray tses_list;
				const PPID cip_person_id = ObjectToPerson(pPack->Rec.Object);
				const LDATETIME _curdtm = getcurdatetime_();
				for(i = 0; i < _c; i++) {
					tses_list.addnz(p_cip_list->at(i).PrmrID);
				}
				tses_list.sortAndUndup();
				for(uint j = 0; j < tses_list.getCount(); j++) {
					int    do_tses_put = 0;
					const PPID tses_id = tses_list.get(j);
					TSessionPacket tses_pack;
					THROW(pPack->CipB.P_TSesObj->GetPacket(tses_id, &tses_pack, 0) > 0);
					{
						PPCheckInPersonConfig cipc(*pPack->CipB.P_TSesObj, tses_pack);
						THROW(cipc);
						for(i = 0; i < _c; i++) {
							const PPCheckInPersonItem & r_cip = p_cip_list->at(i);
							if(r_cip.PrmrID == tses_id) {
								PPCheckInPersonItem temp_cip = r_cip;
								temp_cip.BillID = pPack->Rec.ID;
								if(cip_person_id)
									temp_cip.SetPerson(cip_person_id);
								else
									temp_cip.SetAnonym();
								if(temp_cip.Flags & PPCheckInPersonItem::fCheckedIn) {
									SETIFZ(temp_cip.CiDtm, _curdtm);
									SETIFZ(temp_cip.CiCount, 1);
								}
								else {
									SETIFZ(temp_cip.RegDtm, _curdtm);
									SETIFZ(temp_cip.RegCount, 1);
								}
								THROW(tses_pack.CiList.AddItem(temp_cip, &cipc, 0));
								do_tses_put = 1;
							}
						}
						if(do_tses_put) {
							PPID   temp_tses_id = tses_id;
							THROW(pPack->CipB.P_TSesObj->PutPacket(&temp_tses_id, &tses_pack, 0));
						}
					}
				}
			}
		}
		if(pPack->Rec.OpID) // Проводку теневых документов в журнале не отмечаем
			DS.LogAction(PPACN_TURNBILL, PPOBJ_BILL, pPack->Rec.ID, 0, 0);
		THROW(PPCommitWork(&ta));
	}
	ufp.SetFactor(0, ufp_counter.CalcFactor(0));
	ufp.Commit();
	CATCH
		ok = 0;
		UnlockFRR(&frrl_tag, 1, 0);
		PPRollbackWork(&ta);
		//
		// Если пакет не был проведен корректно, то очищаем признаки
		// успешной проводки тех элементов пакета, которые были проведены
		//
		pPack->Rec.ID = 0;
		pPack->Rec.BillNo = 0;
		pPack->SetLots(preserve_lots);
		for(i = 0; pPack->Turns.enumItems(&i, (void **)&pat);)
			pat->BillID = 0;
	ENDCATCH
	return ok;
}

int PPObjBill::RemoveTransferItem(PPID billID, int rByBill, int force)
{
	int    ok = 1;
	if(trfr->SearchByBill(billID, 0, rByBill, 0) > 0) {
		Reference * p_ref = PPRef;
		ReceiptTbl::Rec lot_rec;
		PPID   lot_id   = trfr->data.LotID;
		long   ti_flags = trfr->data.Flags;
		PPID   mirror_lot_id = 0;
		long   mirror_flags  = 0;
		if(trfr->SearchByBill(billID, 1, rByBill, 0) > 0) {
			mirror_lot_id = trfr->data.LotID;
			mirror_flags  = trfr->data.Flags;
		}
		THROW(trfr->RemoveItem(billID, rByBill, force, 0));
		if(CcFlags & CCFLG_USEGOODSPCKG && ti_flags & PPTFR_PCKG) {
			if(mirror_lot_id && mirror_flags & PPTFR_RECEIPT)
				THROW(P_PckgT->PutPckg(mirror_lot_id, 0, 0));
			if(ti_flags & PPTFR_RECEIPT) {
				THROW(P_PckgT->PutPckg(lot_id, 0, 0));
			}
			else if(trfr->Rcpt.Search(lot_id, &lot_rec) > 0)
				THROW(P_PckgT->SetClosedTag(lot_id, ((lot_rec.Rest > 0) ? 0 : 1), 0));
		}
		if(ti_flags & PPTFR_RECEIPT) {
			// @v8.9.2 THROW(SetClbNumberByLot(lot_id, 0, 0));
			// @v8.9.2 THROW(SetSerialNumberByLot(lot_id, 0, 0));
			THROW(p_ref->Ot.PutList(PPOBJ_LOT, lot_id, 0, 0)); // @v8.9.2
		}
		if(mirror_flags & PPTFR_RECEIPT && mirror_lot_id) {
			// @v8.9.2 THROW(SetClbNumberByLot(mirror_lot_id, 0, 0));
			// @v8.9.2 THROW(SetSerialNumberByLot(mirror_lot_id, 0, 0));
			THROW(p_ref->Ot.PutList(PPOBJ_LOT, mirror_lot_id, 0, 0)); // @v8.9.2
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::UpdatePacket(PPBillPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    ta = 0;
	int    frrl_tag = 0;
	int    rest_checking = -1;
	int    r;
	int    found;
	int    rbybill;
	uint   i, pos;
	const  PPRights & r_rt = ObjRts;
	Reference * p_ref = PPRef;
	ObjVersioningCore * p_ovc = p_ref->P_OvT;
	const  PPID  id = pPack->Rec.ID;
	TBlock tb_;
	PPIDArray added_lot_items; // Список позиций товарных строк с признаком
		// PPTFR_RECEIPT, которые были добавлены. Нобходим для корректной очистки после ошибки.
	PPIDArray _debug_org_ord_bill_list; // @v9.5.2 @debug Список документов заказов, к которым до изменения был привязан данный документ
	SString wait_msg, bill_code, clb;
	SString fmt_buf, msg_buf, temp_buf;
	DateIter diter;
	BillTbl::Rec org;
	PPAccTurn    at, * p_at;
	PPTransferItem ti, * p_ti;
	PPBillPacket org_pack;
	SBuffer hist_buf;
	BillUserProfileCounter ufp_counter;
	PPUserFuncProfiler ufp(GetBillOpUserProfileFunc(pPack->Rec.OpID, PPACN_UPDBILL));
	PPIDArray correction_exp_chain;
	pPack->ErrCause = 0;
	THROW_PP_S(!(pPack->ProcessFlags & PPBillPacket::pfUpdateProhibited), PPERR_UPDBPACKPROHIBITED, PPObjBill::MakeCodeString(&pPack->Rec, 0, bill_code));
	if(!(pPack->ProcessFlags & PPBillPacket::pfIgnoreStatusRestr)) {
		THROW_PP_S(!pPack->Rec.StatusID || !CheckStatusFlag(pPack->Rec.StatusID, BILSTF_DENY_MOD), PPERR_BILLST_DENY_MOD,
			PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, bill_code));
	}
	THROW(CheckParentStatus(pPack->Rec.ID));
	if(pPack->Rec.OpID) { // Для теневого документа не проверяем период доступа
		THROW(r_rt.CheckBillDate(pPack->Rec.Dt));
		if(!(pPack->ProcessFlags & PPBillPacket::pfIgnoreOpRtList)) { // @v9.8.3
			// @v10.2.3 THROW(r_rt.CheckOpID(pPack->Rec.OpID, PPR_MOD)); // @v9.6.1
			THROW(CheckRightsWithOp(pPack->Rec.OpID, PPR_MOD)); // @v10.2.3
		}
		else {
			THROW(CheckRights(PPR_MOD)); // @v10.2.3
		}
		if(pPack->OpTypeID == PPOPT_CORRECTION)
			GetCorrectionBackChain(pPack->Rec, correction_exp_chain);
		if(CcFlags & CCFLG_DEBUG) {
			if(CheckOpFlags(pPack->Rec.OpID, OPKF_ONORDER)) {
				P_Tbl->GetListOfOrdersByLading(pPack->Rec.ID, &_debug_org_ord_bill_list);
				if(_debug_org_ord_bill_list.getCount() && (!pPack->P_ShLots || !pPack->P_ShLots->getCount())) {
					PPLoadText(PPTXT_LOG_BILLHASLORDEMPTYSHL2, fmt_buf);
					PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, bill_code);
					msg_buf.Printf(fmt_buf, bill_code.cptr());
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
				}
			}
		}
	}
	if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
		PPLoadText(PPTXT_WAIT_TURNBILLTRFR, wait_msg);
	THROW(SetupModifPacket(pPack));
	if(CheckOpFlags(pPack->Rec.OpID, OPKF_NEEDPAYMENT) || CheckOpFlags(pPack->Rec.OpID, OPKF_RECKON)) {
		const double amt  = pPack->GetAmount();
		const double paym = pPack->Amounts.Get(PPAMT_PAYMENT, pPack->Rec.CurID);
		if(!(LConfig.Flags & CFGFLG_ALLOWOVERPAY))
			THROW_PP(paym == 0.0 || paym <= amt, PPERR_EXTRAPAYM);
		SETFLAG(pPack->Rec.Flags, BILLF_PAYOUT, paym >= amt);
	}
	THROW(SetupSpecialAmounts(pPack));

	THROW(ExtractPacket(id, &org_pack) > 0);
	if(IsPacketEq(*pPack, org_pack, 0))
		ok = -1;
	else {
		if(State2 & stDoObjVer) {
			if(p_ovc && p_ovc->InitSerializeContext(0)) {
				SSerializeContext & r_sctx = p_ovc->GetSCtx();
				THROW(SerializePacket__(+1, &org_pack, hist_buf, &r_sctx));
			}
		}
		THROW(PPStartTransaction(&ta, use_ta));
		THROW(BeginTFrame(id, tb_));
		THROW(LockFRR(pPack->Rec.Dt, &frrl_tag, 0));
		//
		// Получаем оригинальную запись. Допускаем, что Идент документа может
		// измениться (хотя и не должен), поэтому сначала изменяем сам документ,
		// а затем приводим строки документа в соответствие изменениям
		//
		THROW(P_Tbl->Search(id, &org) > 0);
		THROW(r_rt.CheckBillDate(org.Dt));
		THROW_PP(pPack->OpTypeID != PPOPT_GOODSREVAL || pPack->Rec.Dt == org.Dt, PPERR_REVALDTUPD);
		THROW(ProcessLink(pPack->Rec, pPack->PaymBillID, &org));
		diter.Init();
		{
			BillTbl::Rec link_rec;
			if(P_Tbl->EnumLinks(id, &diter, BLNK_ALL & ~BLNK_SHADOW, &link_rec) > 0) {
				THROW_PP_S(pPack->Rec.Dt <= link_rec.Dt, PPERR_LNKBILLDT, PPObjBill::MakeCodeString(&link_rec, 1, bill_code));
			}
		}
		if(pPack->Rec.Dt != org.Dt) {
			THROW(CheckRights(BILLRT_MODDATE));
			pPack->Rec.BillNo = 0;
		}
		if(pPack->Rec.Object != org.Object)
			THROW(CheckRights(BILLOPRT_MODOBJ, 1));
		THROW(ProcessACPacket(pPack));
		{
			PPID   temp_id = id;
			THROW(P_Tbl->Edit(&temp_id, pPack, 0));
			assert(temp_id == id && pPack->Rec.ID == id);
		}
		pPack->Rec.ID   = id;
		pPack->ErrCause = PPBillPacket::err_on_line;
		if(!(pPack->Rec.Flags & BILLF_NOLOADTRFR)) {
			PPIDArray not_changed_lines;
			const int zero_rbybill = (pPack->ProcessFlags & PPBillPacket::pfForeignSync && pPack->Rec.Flags2 & BILLF2_FULLSYNC) ? 0 : 1;
			const int full_update = (org.ID != id || (org.Object != pPack->Rec.Object && IsIntrOp(org.OpID))) ? 1 : 0;
			for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
				pPack->ErrLine = i-1;
				const long preserve_tflags = p_ti->TFlags;
				THROW(p_ti->Init(&pPack->Rec, /*full_update*/0));
				SETFLAGBYSAMPLE(p_ti->TFlags, PPTransferItem::tfForceNew, preserve_tflags);
				SETFLAGBYSAMPLE(p_ti->TFlags, PPTransferItem::tfQrSeqAccepted, preserve_tflags); // @v10.5.8
				SETFLAGBYSAMPLE(p_ti->TFlags, PPTransferItem::tfQrSeqRejected, preserve_tflags); // @v10.5.8
				if(full_update)
					p_ti->TFlags |= (PPTransferItem::tfForceReplace|PPTransferItem::tfForceNew);
			}
			if(pPack->IsDraft()) {
				CpTrfrExt cte;
				for(rbybill = 0; (r = P_CpTrfr->EnumItems(id, &rbybill, &ti, &cte)) > 0;) {
					ti.Date = org.Dt; // @v8.9.10 P_CpTrfr->EnumItems не инициализирует дату, а это пагубно сказывается на
						// сравнении PPTransferItem::IsEqual()
					for(found = i = 0; !found && pPack->EnumTItems(&i, &p_ti);) {
						if(p_ti->BillID == id && p_ti->RByBill == rbybill && !(p_ti->Flags & PPTransferItem::tfForceReplace)) {
							pPack->ErrLine = i-1;
							found = 1;
							if(p_ti->IsEqual(ti)) {
								pPack->LTagL.GetNumber(PPTAG_LOT_CLB, i-1, clb);
								if(clb.Strip().CmpNC(strip(cte.Clb)) == 0) {
									pPack->LTagL.GetNumber(PPTAG_LOT_SN, i-1, clb);
									if(clb.Strip().CmpNC(strip(cte.PartNo)) == 0) {
										if(cte.LinkBillID == p_ti->Lbr.ID && cte.LinkRbb == p_ti->Lbr.RByBill) // @v10.5.8
											not_changed_lines.add(i);
									}
								}
							}
						}
					}
					if(!found) {
						THROW(P_CpTrfr->RemoveItem(id, rbybill, 0));
						ufp_counter.TiRmvCount++;
					}
				}
				THROW(r);
				//
				// Добавляем или модифицируем строки
				//
				not_changed_lines.sort();
				for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
					if(!not_changed_lines.bsearch(i)) {
						p_ti->BillID   = id;
						pPack->ErrLine = i-1;
						if(!p_ti->BillID || !p_ti->RByBill) {
							THROW(p_ti->Init(&pPack->Rec));
							p_ti->TFlags |= PPTransferItem::tfDirty;
						}
						CpTrfrExt cte;
						pPack->LTagL.GetNumber(PPTAG_LOT_CLB, i-1, clb);
						STRNSCPY(cte.Clb, clb);
						pPack->LTagL.GetNumber(PPTAG_LOT_SN, i-1, clb);
						STRNSCPY(cte.PartNo, clb);
						// @v10.5.8 {
						cte.LinkBillID = p_ti->Lbr.ID; 
						cte.LinkRbb = p_ti->Lbr.RByBill;
						if(p_ti->TFlags & PPTransferItem::tfQrSeqAccepted)
							cte.QrSeqAckStatus = 1;
						else if(p_ti->TFlags & PPTransferItem::tfQrSeqRejected)
							cte.QrSeqAckStatus = 2;
						// } @v10.5.8
						THROW(P_CpTrfr->PutItem(p_ti, 0 /*forceRByBill*/, &cte, 0));
						ufp_counter.TiAddCount++;
					}
					if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
						PPWaitPercent(i, pPack->GetTCount(), wait_msg);
				}
			}
			else {
				//
				// Вычищаем удаленные и сильно измененные товарные строки
				//
				int    chg_closedorder_tag = BIN(pPack->OpTypeID == PPOPT_GOODSORDER && !TESTFLAG(org.Flags, pPack->Rec.Flags, BILLF_CLOSEDORDER));
				for(rbybill = 0; (r = trfr->EnumItems(id, &rbybill, &ti)) > 0;) {
					int    force_remove = 0;
					for(found = i = 0; !found && pPack->EnumTItems(&i, &p_ti);) {
						if(p_ti->BillID == id && p_ti->RByBill == rbybill && !(p_ti->TFlags & PPTransferItem::tfForceReplace)) {
							pPack->ErrLine = i-1;
							found = 1;
							if(p_ti->TFlags & PPTransferItem::tfForceRemove)
								force_remove = 1;
							if(p_ti->Flags & PPTFR_RECEIPT)
								p_ti->LotID = ti.LotID;
							if(p_ti->IsEqual(ti) && !chg_closedorder_tag)
								not_changed_lines.add(i);
						}
					}
					if(!found) {
						THROW(RemoveTransferItem(id, rbybill));
						ufp_counter.TiRmvCount++;
					}
				}
				THROW(r);
				//
				// Добавляем или модифицируем строки
				//
				not_changed_lines.sort();
				for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
					if(!not_changed_lines.bsearch(i)) {
						p_ti->BillID   = id;
						pPack->ErrLine = i-1;
						if(!p_ti->BillID || !p_ti->RByBill || (p_ti->TFlags & PPTransferItem::tfForceNew)) {
							const long preserve_tflags = p_ti->TFlags;
							THROW(p_ti->Init(&pPack->Rec, (zero_rbybill && !p_ti->IsCorrectionExp())));
							SETFLAGBYSAMPLE(p_ti->TFlags, PPTransferItem::tfForceNew, preserve_tflags);
							if(p_ti->Flags & PPTFR_ORDER)
								SETFLAG(p_ti->Flags, PPTFR_CLOSEDORDER, pPack->Rec.Flags & BILLF_CLOSEDORDER);
							p_ti->TFlags |= PPTransferItem::tfDirty;
							THROW(trfr->PreprocessCorrectionExp(*p_ti, correction_exp_chain));
							THROW(trfr->AddItem(p_ti, tb_.Rbb(), 0));
							ufp_counter.TiAddCount++;
							if(p_ti->Flags & PPTFR_RECEIPT)
								added_lot_items.add(i-1);
						}
						else {
							if(p_ti->Flags & PPTFR_ORDER)
								SETFLAG(p_ti->Flags, PPTFR_CLOSEDORDER, pPack->Rec.Flags & BILLF_CLOSEDORDER);
							THROW(trfr->PreprocessCorrectionExp(*p_ti, correction_exp_chain));
							THROW(trfr->UpdateItem(p_ti, tb_.Rbb(), 0, 0));
							ufp_counter.TiUpdCount++;
						}
					}
					if(pPack->ProcessFlags & PPBillPacket::pfViewPercentOnTurn)
						PPWaitPercent(i, pPack->GetTCount(), wait_msg);
				}
			}
			THROW(PutPckgList(pPack, 0));
		}
		THROW(TurnAdvList(id, pPack, 0));
		THROW(pPack->LnkFiles.WriteToProp(pPack->Rec.ID, 0));
		//
		// Вычищаем и модифицируем проводки.
		//
		if(!(CcFlags & CCFLG_DISABLEACCTURN)) {
			if(!(pPack->Rec.Flags & BILLF_NOATURN)) {
				pPack->ErrCause = PPBillPacket::err_on_accturn;
				pPack->ErrLine  = -1;
				rest_checking   = DS.RestCheckingStatus(0);
				rbybill = 0;
				do {
					int prev_rbb = rbybill;
					while((r = atobj->P_Tbl->EnumByBill(id, &rbybill, &at)) > 0 && rbybill < BASE_RBB_BIAS) {
						at.CRate = (at.CurID && at.CurID == LConfig.BaseCurID) ? 1.0 : pPack->Amounts.Get(PPAMT_CRATE, at.CurID);
						for(i = 0, found = 0; !found && pPack->Turns.enumItems(&i, (void **)&p_at);) {
							if(at.Date == p_at->Date && at.DbtID == p_at->DbtID && at.CrdID == p_at->CrdID) {
								DS.RestCheckingStatus(rest_checking);
								THROW(atobj->P_Tbl->UpdateAmount(at.BillID, at.RByBill, p_at->Amount, at.CRate, 0));
								ufp_counter.AtUpdCount++;
								DS.RestCheckingStatus(0);
								pPack->Turns.atFree(--i);
								found = 1;
							}
						}
						if(!found) {
							THROW(atobj->P_Tbl->RollbackTurn(at.BillID, at.RByBill, 0));
							ufp_counter.AtRmvCount++;
						}
					}
					if(r == 0) {
						const uint msg_id = (prev_rbb != rbybill) ? PPTXT_LOG_LOADACCTURNFAULT_C : PPTXT_LOG_LOADACCTURNFAULT;
						PPGetLastErrorMessage(1, temp_buf);
						PPFormatT(msg_id, &msg_buf, id, temp_buf.cptr());
						PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
						if(prev_rbb != rbybill) {
							THROW(atobj->P_Tbl->RollbackTurn(id, rbybill, 0));
							ufp_counter.AtRmvCount++;
						}
						else
							CALLEXCEPT();
					}
				} while(r == 0);
				//
				// Восстанавливаем первоначальное значение статуса проверки
				// остатков по счетам и заносим все оставшиеся проводки
				//
				DS.RestCheckingStatus(rest_checking);
				for(i = 0; pPack->Turns.enumItems(&i, (void **)&p_at);) {
					pPack->ErrLine = i-1;
					p_at->BillID   = id;
					p_at->RByBill  = 0;
					THROW(atobj->P_Tbl->Turn(p_at, 0));
					ufp_counter.AtAddCount++;
				}
			}
			else {
				for(rbybill = 0; (r = atobj->P_Tbl->EnumByBill(id, &rbybill, &at)) > 0 && rbybill < BASE_RBB_BIAS;) {
					THROW(atobj->P_Tbl->RollbackTurn(id, rbybill, 0));
					ufp_counter.AtRmvCount++;
				}
				THROW(r);
			}
		}
		pPack->ErrCause = 0;
		THROW(ProcessShadowPacket(pPack, 1));
		THROW(Helper_StoreClbList(pPack));
		THROW(PutSCardOp(pPack, 0));
		//
		// При необходимости, изменяем привязанные документы (оплаты, возвраты, подтверждения)
		//
		// @CAUTION
		//    Этот процесс несет в себе потенциальную опасность рекурентного
		//    зацикливания, поскольку изменение привязанного документа может
		//    повлечь за собой изменение самого документа и так далее.
		//
		if(org.Object != pPack->Rec.Object) {
			for(diter.Init(); P_Tbl->EnumLinks(id, &diter, BLNK_ALL & ~BLNK_SHADOW) > 0;) {
				const PPID link_op_id = P_Tbl->data.OpID;
				if(link_op_id) {
					const PPID link_id = P_Tbl->data.ID;
					PPOprKind link_op_rec;
					if(GetOpData(link_op_id, &link_op_rec) > 0 && !link_op_rec.AccSheetID && P_Tbl->data.Object == org.Object) {
						PPBillPacket link_pack;
						THROW(ExtractPacket(link_id, &link_pack) > 0);
						link_pack.Rec.Object = pPack->Rec.Object;
						THROW(FillTurnList(&link_pack));
						THROW(UpdatePacket(&link_pack, 0));
					}
				}
			}
		}
		if(pPack->PaymBillID) {
			THROW(P_Tbl->UpdatePool(pPack->Rec.ID, PPASS_PAYMBILLPOOL, pPack->PaymBillID, 0));
		}
		{
			PPIDArray  pool_list;
			if(P_Tbl->GetPoolOwnerList(pPack->Rec.ID, PPASS_OPBILLPOOL, &pool_list) > 0) {
				for(i = 0; i < pool_list.getCount(); i++) {
					THROW(UpdatePool(pool_list.get(i), 0));
				}
			}
		}
		THROW(UnlockFRR(&frrl_tag, 0, 0));
		THROW(FinishTFrame(id, tb_));
		if(pPack->Rec.OpID) { // Проводку теневого документа не регистрируем
			PPID   h_id = 0;
			/* @v9.8.11 if(TLP(HistBill).IsOpened()) {
				THROW(HistBill->PutPacket(&h_id, &hist_pack, 0, 0));
			} */
			if(State2 & stDoObjVer) {
				if(p_ovc && p_ovc->InitSerializeContext(0)) {
					THROW(p_ovc->Add(&h_id, PPObjID(Obj, id), &hist_buf, 0));
				}
			}
			if(CcFlags & CCFLG_DEBUG) {
				if(CheckOpFlags(pPack->Rec.OpID, OPKF_ONORDER)) {
					PPIDArray _debug_new_ord_bill_list;
					P_Tbl->GetListOfOrdersByLading(pPack->Rec.ID, &_debug_new_ord_bill_list);
					_debug_org_ord_bill_list.sortAndUndup();
					_debug_new_ord_bill_list.sortAndUndup();
					if(!_debug_org_ord_bill_list.IsEqual(&_debug_new_ord_bill_list)) {
						PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, bill_code);
						PPLoadText(PPTXT_LOG_BILLCHGLINKTOORD, fmt_buf);
						{
							uint _i;
							temp_buf.Z().CatChar('[');
							for(_i = 0; _i < _debug_org_ord_bill_list.getCount(); _i++) {
								const PPID _ord_bill_id = _debug_org_ord_bill_list.get(_i);
								if(_i)
									temp_buf.CatDiv(',', 2);
								temp_buf.Cat(_ord_bill_id);
							}
							temp_buf.CatChar(']').Cat("->").CatChar('[');
							for(_i = 0; _i < _debug_new_ord_bill_list.getCount(); _i++) {
								const PPID _ord_bill_id = _debug_new_ord_bill_list.get(_i);
								if(_i)
									temp_buf.CatDiv(',', 2);
								temp_buf.Cat(_ord_bill_id);
							}
							temp_buf.CatChar(']');
						}
						msg_buf.Printf(fmt_buf, bill_code.cptr(), temp_buf.cptr());
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
					}
				}
			}
			DS.LogAction(PPACN_UPDBILL, PPOBJ_BILL, pPack->Rec.ID, h_id, 0);
		}
		THROW(PPCommitWork(&ta));
	}
	ufp.SetFactor(0, ufp_counter.CalcFactor(0));
	ufp.Commit();
	CATCH
		ok = 0;
		DS.RestCheckingStatus(rest_checking);
		UnlockFRR(&frrl_tag, 1, 0);
		PPRollbackWork(&ta);
		if(pPack->P_ACPack && pPack->P_ACPack->GetTCount()) {
			for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
				if(p_ti->Flags & PPTFR_AUTOCOMPL && p_ti->LotID > 0) {
			   	    if(pPack->P_ACPack->SearchLot(p_ti->LotID, &(pos = 0)) > 0) {
				   	    PPTransferItem & acti = pPack->P_ACPack->TI(pos);
						acti.LotID = p_ti->LotID = acti.ACLinkLotID;
   	                    acti.ACLinkLotID = 0;
	   	            }
				}
			}
		}
		if(!pPack->P_Outer) {
			const uint _c = added_lot_items.getCount();
			for(i = 0; i < _c; i++)
				pPack->TI(added_lot_items.at(i)).LotID = 0;
		}
	ENDCATCH
	for(i = 0; pPack->EnumTItems(&i, &p_ti);) {
		p_ti->Flags &= ~PPTFR_CLOSEDORDER;
		if(p_ti->TFlags & PPTransferItem::tfDirty) {
			if(!ok)
				p_ti->RByBill = 0;
			p_ti->TFlags &= ~PPTransferItem::tfDirty;
		}
		p_ti->SetupSign(pPack->Rec.OpID);
	}
	return ok;
}

int PPObjBill::RemovePacket(PPID id, int use_ta)
{
	int    ok = 1;
	int    ta = 0;
	int    frrl_tag = 0;
	int    r;
	int    rbybill = 0;
	int    is_inventory = 0;
	PPID   paym_link_id = 0;
	PPID   pull_member_id;
	PPID   op_type_id = 0;
	SString bill_code;
	SString msg_buf;
	DateIter diter;
	BillTbl::Rec brec;
	InventoryCore * p_inv_tbl = 0;
	THROW(P_Tbl->Search(id, &brec) > 0);
	{
		Reference * p_ref = PPRef;
		const  PPRights & r_rt = ObjRts;
		ObjVersioningCore * p_ovc = p_ref->P_OvT;
		SBuffer hist_buf;
		BillUserProfileCounter ufp_counter;
		PPUserFuncProfiler ufp(GetBillOpUserProfileFunc(brec.OpID, PPACN_RMVBILL));
		const int is_shadow = BIN(brec.OpID == 0);
		if(!is_shadow) { // Для теневого документа не проверяем период доступа и права на удаление
			// @v10.1.12 THROW(CheckRights(PPR_DEL));
			// @v10.1.12 {
			// @v10.2.3 const int cor = r_rt.CheckOpID(brec.OpID, PPR_DEL);
			// @v10.2.3 THROW(cor);
			// @v10.2.3 THROW((cor > 0) || CheckRights(PPR_DEL));
			// } @v10.1.12
			THROW(CheckRightsWithOp(brec.OpID, PPR_DEL)); // @v10.2.3
			THROW(r_rt.CheckBillDate(brec.Dt));
			// @v10.1.12 THROW(r_rt.CheckOpID(brec.OpID, PPR_DEL)); // @v9.6.1
			if(State2 & stDoObjVer) {
				if(p_ovc && p_ovc->InitSerializeContext(0)) {
					SSerializeContext & r_sctx = p_ovc->GetSCtx();
					PPBillPacket org_pack;
					THROW(ExtractPacketWithFlags(id, &org_pack, BPLD_LOADINVLINES) > 0); // @v9.9.12 BPLD_LOADINVLINES
					THROW(SerializePacket__(+1, &org_pack, hist_buf, &r_sctx));
				}
			}
		}
		THROW_PP_S(!brec.StatusID || !CheckStatusFlag(brec.StatusID, BILSTF_DENY_DEL), PPERR_BILLST_DENY_DEL, PPObjBill::MakeCodeString(&brec, 1, bill_code));
		THROW(CheckParentStatus(id));
		op_type_id = GetOpType(brec.OpID);
		if(op_type_id == PPOPT_INVENTORY) {
			is_inventory = 1;
			p_inv_tbl = &GetInvT();
		}
		IsMemberOfPool(id, PPASS_PAYMBILLPOOL, &paym_link_id);
		THROW(PPStartTransaction(&ta, use_ta));
		THROW(LockFRR(brec.Dt, &frrl_tag, 0));
		{
			BillTbl::Rec link_bill_rec;
			for(diter.Init(); (r = P_Tbl->EnumLinks(id, &diter, BLNK_ALL, &link_bill_rec)) > 0;) {
				THROW_PP_S(link_bill_rec.OpID == 0, PPERR_BILLHASLINKS, PPObjBill::MakeCodeString(&link_bill_rec, 1, msg_buf)); // Теневые документы удаляем безусловно
				THROW(RemovePacket(link_bill_rec.ID, 0));
			}
		}
		pull_member_id = 0;
		if(P_Tbl->EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &pull_member_id) > 0) {
			BillTbl::Rec pool_bill_rec;
			if(Search(pull_member_id, &pool_bill_rec) > 0) {
				CALLEXCEPT_PP_S(PPERR_BILLHASPAYMPOOL, MakeCodeString(&pool_bill_rec, 1, msg_buf));
			}
			else {
				CALLEXCEPT_PP_S(PPERR_BILLHASINVPAYMPOOL, ideqvalstr(pull_member_id, msg_buf));
			}
		}
		THROW(r && P_Tbl->Search(id, &brec) > 0);
		THROW(ProcessLink(brec, paym_link_id, &brec));
		THROW(TurnAdvList(id, 0, 0));
		while((r = atobj->P_Tbl->EnumByBill(id, &rbybill, 0)) > 0) {
			THROW(atobj->P_Tbl->RollbackTurn(id, rbybill, 0));
			ufp_counter.AtRmvCount++;
		}
		THROW(r);
		if(oneof4(op_type_id, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ)) { // @v10.5.8 PPOPT_DRAFTQUOTREQ
			if(P_CpTrfr) {
				for(rbybill = 0; (r = P_CpTrfr->EnumItems(id, &rbybill, 0, 0)) > 0;) {
					THROW(P_CpTrfr->RemoveItem(id, rbybill, 0));
					ufp_counter.TiRmvCount++;
				}
			}
		}
		else {
			for(rbybill = 0; (r = trfr->EnumItems(id, &rbybill, 0)) > 0;) {
				THROW(RemoveTransferItem(id, rbybill));
				ufp_counter.TiRmvCount++;
			}
		}
		// @v9.8.11 (из-за условия возможно, что какие-то свойства удалены не будут) if(brec.Flags & BILLF_EXTRA)
			THROW(p_ref->RemoveProperty(PPOBJ_BILL, id, 0, 0));
		if(is_inventory)
			THROW(p_inv_tbl->Remove(id, 0));
		if(paym_link_id)
			THROW(P_Tbl->RemoveFromPool(id, PPASS_PAYMBILLPOOL, paym_link_id, 0));
		if(brec.Flags & BILLF_CSESSWROFF)
			THROW(P_Tbl->RemoveFromPool(id, PPASS_CSESSBILLPOOL, 0L, 0));
		if(brec.Flags & BILLF_CDFCTWROFF)
			THROW(P_Tbl->RemoveFromPool(id, PPASS_CSDBILLPOOL, 0L, 0));
		if(brec.Flags & BILLF_TSESSWROFF || brec.Flags2 & BILLF2_TSESSPAYM)
			THROW(P_Tbl->RemoveFromPool(id, PPASS_TSESSBILLPOOL, 0L, 0));
		if(brec.Flags & BILLF_TDFCTWROFF)
			THROW(P_Tbl->RemoveFromPool(id, PPASS_TSDBILLPOOL, 0L, 0));
		{
			PPIDArray  pool_list;
			if(P_Tbl->GetPoolOwnerList(id, PPASS_OPBILLPOOL, &pool_list) > 0)
				for(uint i = 0; i < pool_list.getCount(); i++)
					THROW(UpdatePool(pool_list.get(i), 0));
		}
		if(brec.Flags & BILLF_SCARDOP) {
			SETIFZ(P_ScObj, new PPObjSCard);
			if(P_ScObj && P_ScObj->P_Tbl)
				THROW(P_ScObj->P_Tbl->PutOpByBill(id, brec.SCardID, brec.Dt, 0.0, 0));
		}
		THROW(r && P_Tbl->Remove(id, 0));
		THROW(p_ref->PutPropVlrString(PPOBJ_BILL, id, PPPRP_BILLMEMO, 0));
		THROW(p_ref->Ot.PutList(Obj, id, 0, 0));
		// @v10.2.9 THROW(PPLotExtCodeContainer::RemoveAllByBill(P_LotXcT, id, 0)); // @v9.8.11
		// @v10.2.9 {
		if(P_LotXcT)
			THROW(P_LotXcT->RemoveAllByBill(id, 0));
		// } @v10.2.9
		THROW(RemoveSync(id));
		THROW(UnlockFRR(&frrl_tag, 0, 0));
		if(!is_shadow) {
			PPID   h_id = 0;
			/* @v9.8.11 if(TLP(HistBill).IsOpened())
				THROW(HistBill->PutPacket(&h_id, &hist_pack, 1, 0) > 0); */
			if(State2 & stDoObjVer) {
				if(p_ovc && p_ovc->InitSerializeContext(0)) {
					THROW(p_ovc->Add(&h_id, PPObjID(Obj, id), &hist_buf, 0));
				}
			}
			DS.LogAction(PPACN_RMVBILL, PPOBJ_BILL, id, h_id, 0);
		}
		THROW(PPCommitWork(&ta));
		ufp.SetFactor(0, ufp_counter.CalcFactor(0));
		ufp.Commit();
	}
	CATCH
		ok = 0;
		UnlockFRR(&frrl_tag, 1, 0);
		PPRollbackWork(&ta);
	ENDCATCH
	return ok;
}

int PPObjBill::Helper_GetPoolMembership(PPID id, const PPBillPacket * pPack, long flag, PPID poolType, PPID * pPoolID)
{
	int    ok = -1;
	PPID   pool_id = 0;
	if(!flag || pPack->Rec.Flags & flag)
		if(IsMemberOfPool(id, poolType, &pool_id) > 0) {
			ASSIGN_PTR(pPoolID, pool_id);
			ok = 2;
		}
		else
			ok = 1;
	return ok;
}

int PPObjBill::GetPoolsMembership(PPID id, PPBillPacket * pPack)
{
	Helper_GetPoolMembership(id, pPack, 0, PPASS_PAYMBILLPOOL, &pPack->PaymBillID);
	if(Helper_GetPoolMembership(id, pPack, BILLF_CSESSWROFF, PPASS_CSESSBILLPOOL, &pPack->CSessID) < 0)
		if(Helper_GetPoolMembership(id, pPack, BILLF_CDFCTWROFF,  PPASS_CSDBILLPOOL, &pPack->CSessID) < 0)
			if(Helper_GetPoolMembership(id, pPack, BILLF_TSESSWROFF, PPASS_TSESSBILLPOOL, &pPack->CSessID) < 0)
				Helper_GetPoolMembership(id, pPack, BILLF_TDFCTWROFF,  PPASS_TSDBILLPOOL, &pPack->CSessID);
	return 1;
}

int PPObjBill::SetupSpecialAmounts(PPBillPacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		if(pPack->CSessID) {
			// @v10.3.0 long   pool_type = 0;
			/* Временно, инициировать сумму будем только по техн сессиям (но не по кассовым)
			if(pPack->Rec.Flags & (BILLF_CSESSWROFF|BILLF_CDFCTWROFF)) {
				PPObjCSession cs_obj;
				CSessionTbl::Rec cs_rec;
				if(cs_obj.Search(pPack->CSessID, &cs_rec) > 0) {
					pPack->Amounts.Put(PPAMT_POOLAMOUNT, 0, cs_rec.Amount, 1, 1);
					ok = 1;
				}
			}
			else*/ if(pPack->Rec.Flags & (BILLF_TSESSWROFF|BILLF_TDFCTWROFF)) {
				PPObjTSession tses_obj;
				TSessionTbl::Rec tses_rec;
				if(tses_obj.Search(pPack->CSessID, &tses_rec) > 0) {
					pPack->Amounts.Put(PPAMT_POOLAMOUNT, 0, tses_rec.Amount, 1, 1);
					ok = 1;
				}
			}
		}
		else if(pPack->Rec.ID) {
			PPID   pool_id = 0;
			if(IsMemberOfPool(pPack->Rec.ID, PPASS_TSESSBILLPOOL, &pool_id) > 0) {
				PPObjTSession tses_obj;
				TSessionTbl::Rec tses_rec;
				if(tses_obj.Search(pool_id, &tses_rec) > 0 && pPack->Rec.Flags & BILLF_TSESSWROFF) {
					pPack->Amounts.Put(PPAMT_POOLAMOUNT, 0, tses_rec.Amount, 1, 1);
					ok = 1;
				}
			}
		}
		{
			//
			// Расчет и установка сумм документа, определяемых формулами
			//
			PPObjAmountType at_obj;
			StrAssocArray fa_list;
			if(at_obj.GetFormulaList(&fa_list) > 0) {
				PPIDArray op_at_list;
				SString formula;
				if(P_OpObj->GetExAmountList(pPack->Rec.OpID, &op_at_list) > 0) {
					for(uint i = 0; i < op_at_list.getCount(); i++) {
						const PPID at_id = op_at_list.get(i);
						if(fa_list.GetText(at_id, formula) > 0) {
							double value = 0.0;
							THROW(PPCalcExpression(formula, &value, pPack, pPack->Rec.CurID, 0));
							pPack->Amounts.Put(at_id, pPack->Rec.CurID, value, 0, 1);
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

int PPObjBill::GetOrderLotForTransfer(const TransferTbl::Rec & rTrfrRec, PPID * pLotID)
{
	int    ok = -1;
	PPID   ord_lot_id = 0;
	if(rTrfrRec.Flags & PPTFR_ONORDER) {
		int    r;
		const  PPID bill_id = rTrfrRec.BillID;
		BillTbl::Rec shadow_bill_rec;
		PPTrfrArray candidate_shadow_ti_list;
		for(DateIter diter; (r = P_Tbl->EnumLinks(bill_id, &diter, BLNK_SHADOW)) > 0;) {
			P_Tbl->copyBufTo(&shadow_bill_rec);
			const PPID shadow_bill_id = shadow_bill_rec.ID;
			PPTransferItem shadow_ti;
			for(int rbb = 0; trfr->EnumItems(shadow_bill_id, &rbb, &shadow_ti) > 0;) {
				if(labs(shadow_ti.GoodsID) == labs(rTrfrRec.GoodsID)) {
					shadow_ti.OrdLotID = shadow_ti.BillID; // @ordlotid
					shadow_ti.BillID   = shadow_bill_rec.Object;
					shadow_ti.QCert    = 0;
					THROW_SL(candidate_shadow_ti_list.insert(&shadow_ti));
					//ord_lot_id = shadow_ti.LotID;
				}
			}
		}
		if(candidate_shadow_ti_list.getCount() == 1) {
			ord_lot_id = candidate_shadow_ti_list.at(0).LotID;
		}
		else if(candidate_shadow_ti_list.getCount() > 1) {
			for(uint i = 0; i < candidate_shadow_ti_list.getCount(); i++) {
				PPTransferItem & r_shadow_ti = candidate_shadow_ti_list.at(i);
				if(fabs(r_shadow_ti.Quantity_) == fabs(rTrfrRec.Quantity)) {
					ord_lot_id = r_shadow_ti.LotID;
					break;
				}
			}
		}
		if(ord_lot_id) {
			ok = 1;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pLotID, ord_lot_id);
	return ok;
}

int PPObjBill::ExtractPacket(PPID id, PPBillPacket * pPack)
	{ return Helper_ExtractPacket(id, pPack, 0, 0); }
int PPObjBill::ExtractPacketWithFlags(PPID id, PPBillPacket * pPack, uint fl /* BPLD_XXX */)
	{ return Helper_ExtractPacket(id, pPack, fl, 0); }
int PPObjBill::ExtractPacketWithRestriction(PPID id, PPBillPacket * pPack, uint fl /* BPLD_XXX */, const PPIDArray * pGoodsList)
	{ return Helper_ExtractPacket(id, pPack, fl, pGoodsList); }

int PPObjBill::Helper_ExtractPacket(PPID id, PPBillPacket * pPack, uint fl, const PPIDArray * pGoodsList)
{
	int    ok = 1, r, rbybill = 0;
	uint   i;
	SString msg_buf;
	SString fmt_buf, temp_buf;
	PPAccTurn at;
	PPOprKind opk;
	PPOprKind link_opk;
	PPBillPacket shadow;
	PPTransferItem * p_ti;
	pPack->destroy();
	THROW(!(fl & BPLD_LOCK) || Lock(id));
	THROW(P_Tbl->Extract(id, pPack));
	THROW(GetPoolsMembership(id, pPack));
	if(pPack->Rec.OpID) {
		THROW(GetOpData(pPack->Rec.OpID, &opk));
		pPack->OpTypeID  = opk.OpTypeID;
		pPack->AccSheetID = opk.AccSheetID;
	}
	//
	// Несмотря на то, что признаки товарной операции должны быть
	// установлены при занесении документа в БД, здесь мы продублируем
	// операцию инициализации этих флагов для надежности
	//
	r = IsExpendOp(pPack->Rec.OpID);
	SETFLAG(pPack->Rec.Flags, BILLF_GEXPEND,  r > 0);
	SETFLAG(pPack->Rec.Flags, BILLF_GRECEIPT, r == 0);
	SETFLAG(pPack->Rec.Flags, BILLF_GREVAL,   pPack->OpTypeID == PPOPT_GOODSREVAL);
	if(pPack->Rec.LinkBillID) {
		BillTbl::Rec link_bill_rec;
		msg_buf.Z().Cat(pPack->Rec.LinkBillID);
		THROW(r = P_Tbl->Search(pPack->Rec.LinkBillID, &link_bill_rec));
		THROW_PP_S(r > 0, PPERR_LINKBILLNFOUND, msg_buf);
		THROW(GetOpData(link_bill_rec.OpID, &link_opk));
		SETIFZ(pPack->AccSheetID, link_opk.AccSheetID);
		if(pPack->OpTypeID == PPOPT_CORRECTION /*&& link_opk.OpTypeID == PPOPT_GOODSEXPEND*/) {
			THROW_MEM(SETIFZ(pPack->P_LinkPack, new PPBillPacket));
			pPack->P_LinkPack->destroy();
			THROW(ExtractPacket(pPack->Rec.LinkBillID, pPack->P_LinkPack) > 0);
		}
	}
	THROW(LoadAdvList(id, pPack->Rec.OpID, &pPack->AdvList));
	THROW(pPack->LnkFiles.ReadFromProp(id));
	while((r = atobj->P_Tbl->EnumByBill(id, &rbybill, &at)) > 0 && rbybill < BASE_RBB_BIAS) {
		at.Opr   = pPack->Rec.OpID;
		at.CurID = pPack->Rec.CurID;
		at.CRate = pPack->Amounts.Get(PPAMT_CRATE, at.CurID);
		memcpy(at.BillCode, pPack->Rec.Code, sizeof(at.BillCode));
		THROW_SL(pPack->Turns.insert(&at));
	}
	if(!r) {
		if(CcFlags & CCFLG_DEBUG) {
			PPGetLastErrorMessage(1, temp_buf);
			PPFormatT(PPTXT_LOG_LOADACCTURNFAULT, &msg_buf, id, temp_buf.cptr());
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
		else
			CALLEXCEPT();
	}
	if(fl & BPLD_SKIPTRFR) {
		pPack->ProcessFlags |= PPBillPacket::pfUpdateProhibited;
		//
		// Эти функции вызываются в pPack->LoadClbList, однако при (fl & BPLD_SKIPTRFR)
		// LoadClbList не вызывается, потому приходится делать это здесь.
		//
		pPack->BTagL.Destroy();
		THROW(GetTagList(pPack->Rec.ID, &pPack->BTagL));
	}
	else {
		if(pGoodsList)
			pPack->ProcessFlags |= PPBillPacket::pfUpdateProhibited;
		if(pPack->IsDraft()) {
			if(P_CpTrfr) {
				THROW(P_CpTrfr->LoadItems(id, pPack, pGoodsList));
				THROW(LoadClbList(pPack, 2)); // @v10.0.0 force 0-->2
			}
		}
		else {
			int    is_there_shadow_bill = 0; // @debug
			DateIter diter;
			trfr->LoadItems(*pPack, pGoodsList);
			THROW(LoadPckgList(pPack));
			THROW(LoadClbList(pPack, BIN(fl & BPLD_FORCESERIALS)));
			while((r = P_Tbl->EnumLinks(id, &diter, BLNK_SHADOW)) > 0) {
				is_there_shadow_bill = 1; // @debug
				THROW(ExtractPacket(P_Tbl->data.ID, &shadow));
				for(i = 0; shadow.EnumTItems(&i, &p_ti);) {
					p_ti->OrdLotID = p_ti->BillID; // @ordlotid
					p_ti->BillID   = shadow.Rec.Object;
					p_ti->QCert    = 0;
					//
					// Попытка установить соответствие между строками документа и лотами теневого документа
					//
					int    corr_item_founded = 0;
					uint   j;
					//
					// На первой итерации пытаемся установить соответствие с учетом количества
					//
					for(j = 0; !corr_item_founded && pPack->SearchGoods(labs(p_ti->GoodsID), &j); j++) {
						PPTransferItem & r_loc_ti = pPack->TI(j);
						if(r_loc_ti.Flags & PPTFR_ONORDER && r_loc_ti.OrdLotID == 0) {
							if(fabs(r_loc_ti.Quantity_) == fabs(p_ti->Quantity_)) {
								r_loc_ti.OrdLotID = p_ti->LotID;
								corr_item_founded = 1;
							}
						}
					}
					//
					// На второй итерации, если с учетом количества ничего не вышло (мало ли какие сбои случаются),
					// устанавливаем соответствие без учета количества
					//
					for(j = 0; /* @v9.3.10 !corr_item_founded &&*/ pPack->SearchGoods(labs(p_ti->GoodsID), &j); j++) {
						PPTransferItem & r_loc_ti = pPack->TI(j);
						if(r_loc_ti.Flags & PPTFR_ONORDER && r_loc_ti.OrdLotID == 0) {
							r_loc_ti.OrdLotID = p_ti->LotID;
							corr_item_founded = 1;
						}
					}
					//
					THROW(pPack->AddShadowItem(p_ti));
				}
			}
			// @v9.5.3 {
			/* Проблема решена - привязка терялась из-за неинициализированных членов PPBillPacket: OpTypeID AccSheetID (скорее всего, OpTypeID)
			if(CConfig.Flags & CCFLG_DEBUG) {
				if(is_there_shadow_bill) {
					PPLoadText(PPTXT_LOG_BILLHASLORDEMPTYSHL, fmt_buf);
					PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, temp_buf);
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
				}
			}
			*/
			// } @v9.5.3
		}
		if(pPack->OpTypeID == PPOPT_INVENTORY && fl & BPLD_LOADINVLINES) {
			THROW(LoadInventoryArray(id, pPack->InvList));
		}
	}
	{
		PPBillPacket::SetupObjectBlock sob;
		pPack->SetupObject(pPack->Rec.Object, sob);
	}
	pPack->LoadMoment = getcurdatetime_();
	CATCHZOK
	return ok;
}
//
// Descr: Пересчитывает долговые атрибуты результирующего документа
//
int PPObjBill::RecalcPayment(PPID id, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		if(CheckOpFlags(rec.OpID, OPKF_NEEDPAYMENT) || CheckOpFlags(rec.OpID, OPKF_RECKON)) {
			long   f, f1;
			const  PPID   cur_id = rec.CurID;
			const  double bpaym = rec.PaymAmount;
			const  double amt   = BR2(rec.Amount);
			double paym = 0.0;
			double real_paym = 0.0;
			DateIter diter;
			f = f1 = rec.Flags;
			THROW(P_Tbl->GetAmount(id, PPAMT_PAYMENT, cur_id, &paym));
			THROW(P_Tbl->CalcPayment(id, 1, 0, cur_id, &real_paym));
			real_paym = R6(real_paym);
			SETFLAG(f, BILLF_PAYOUT, R2(real_paym - amt) >= 0);
			if(paym != real_paym || f != f1 || bpaym != real_paym) {
				PPTransaction tra(use_ta);
				THROW(tra);
				if(paym != real_paym) {
					AmtEntry ae(PPAMT_PAYMENT, cur_id, real_paym);
					THROW(P_Tbl->UpdateAmount(id, &ae, 1));
				}
				if(f != f1 || bpaym != real_paym) {
					THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == id), set(P_Tbl->Flags, dbconst(f)).set(P_Tbl->PaymAmount, dbconst(real_paym))));
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBill::RecalcTurns(PPID id, long flags, int use_ta)
{
	int    ok = 1;
	if(Search(id) > 0) {
		PPBillPacket pack;
		THROW(ExtractPacket(id, &pack));
		if(pack.Rec.OpID && !(pack.Rec.Flags & BILLF_NOATURN) && (pack.OpTypeID != PPOPT_ACCTURN || CheckOpFlags(pack.Rec.OpID, OPKF_EXTACCTURN))) {
			if(flags & BORTF_IGNOREOPRTLIST)
				pack.ProcessFlags |= PPBillPacket::pfIgnoreOpRtList;
			if(flags & BORTF_RECALCTRFRS) {
				PPTransferItem * p_ti;
				for(uint i = 0; pack.EnumTItems(&i, &p_ti);)
					THROW(p_ti->Init(&pack.Rec, 0));
				flags &= ~BORTF_NORECALCAMOUNTS;
			}
			else
				pack.Rec.Flags |= BILLF_NOLOADTRFR;
			if(!(flags & BORTF_NORECALCAMOUNTS) && pack.IsGoodsDetail())
				THROW(pack.InitAmounts(0));
			THROW(FillTurnList(&pack));
			THROW(UpdatePacket(&pack, use_ta));
		}
	}
	CATCHZOK
	return ok;
}

static int IsBillsCompatible(const BillTbl::Rec * pBillPack1, const BillTbl::Rec * pBillPack2)
{
	int    ok = 0;
	int    reason = -1;
	if(pBillPack1->Object != pBillPack2->Object)
		reason = BILLUNCOMPATREASON_OBJ;
	else if(pBillPack1->Object2 != pBillPack2->Object2)
		reason = BILLUNCOMPATREASON_OBJ2;
	else if(pBillPack1->OpID != pBillPack2->OpID)
		reason = BILLUNCOMPATREASON_OP;
	else if(pBillPack1->LocID != pBillPack2->LocID)
		reason = BILLUNCOMPATREASON_LOC;
	else if(pBillPack1->CurID != pBillPack2->CurID)
		reason = BILLUNCOMPATREASON_CUR;
	else if(pBillPack1->LinkBillID != pBillPack2->LinkBillID)
		reason = BILLUNCOMPATREASON_LINK;
	else
		ok = 1;
	if(!ok) {
		SString added_buf;
		if(reason >= 0)
			PPGetSubStr(PPTXT_BILLUNCOMPATREASON, reason, added_buf);
		PPSetError(PPERR_UNMATCHEDUBILLS, added_buf);
	}
	return ok;
}
//
// Функция реализована пока только для документов розничной торговли
// Бухгалтерские проводки не пересчитывает.
// Это должна сделать вызывающая функция.
//
int PPObjBill::UniteGoodsBill(PPBillPacket * pPack, PPID addBillID, int use_ta)
{
	int    ok = 1;
	//uint   i, j;
	short  rbybill;
	//int    done;
	// @v10.3.0 int    is_intrexnd = 0; // Признак того, что объединяются документы внутреннего перемещения - сложный случай.
	DateIter diter;
	PPBillPacket add_pack;
	PPTransferItem ti;
	PPLotExtCodeContainer::MarkSet src_lotxcode_set; // @v10.9.7
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ExtractPacket(addBillID, &add_pack));
		THROW(IsBillsCompatible(&pPack->Rec, &add_pack.Rec));
		THROW_PP((pPack->OpTypeID == PPOPT_GOODSEXPEND && !IsIntrOp(pPack->OpTypeID)) ||
			pPack->Rec.OpID == GetCashOp() || pPack->Rec.OpID == GetCashRetOp(), PPERR_UNITERETAIL);
		if(IsIntrOp(pPack->Rec.OpID) == INTREXPND) {
			CALLEXCEPT_PP(PPERR_UNITEINTREXPND);
			// @todo Сделать объединение документов внутренней передачи
		}
		else {
			const double _eps = 1.0e07; // @v10.9.7
			TBlock tb_;
			THROW(BeginTFrame(pPack->Rec.ID, tb_));
			//
			// Цикл по строкам исходного документа (который будет удален)
			//
			for(uint atiidx = 0; atiidx < add_pack.GetTCount(); atiidx++) {
				PPTransferItem & r_ati = add_pack.TI(atiidx);
				bool done = false;
				add_pack.XcL.Get(atiidx+1, 0, src_lotxcode_set); // @v10.9.7
				for(uint j = 0; !done && pPack->SearchLot(r_ati.LotID, &j); j++) {
					PPTransferItem * p_ti = &pPack->TI(j);
					if(feqeps(p_ti->Cost, r_ati.Cost, _eps) && feqeps(p_ti->Price, r_ati.Price, _eps) && feqeps(p_ti->Discount, r_ati.Discount, _eps)) {
						p_ti->Quantity_ += r_ati.Quantity_;
						p_ti->WtQtty += r_ati.WtQtty;
						if(r_ati.Quantity_ > 0.0) {
							THROW(trfr->UpdateItem(p_ti, tb_.Rbb(), 0, 0));
							THROW(trfr->RemoveItem(r_ati.BillID, r_ati.RByBill, 0, 0));
						}
						else {
							THROW(trfr->RemoveItem(r_ati.BillID, r_ati.RByBill, 0, 0));
							THROW(trfr->UpdateItem(p_ti, tb_.Rbb(), 0, 0));
						}
						pPack->XcL.Add(j+1, src_lotxcode_set); // @v10.9.7
						done = true;
					}
				}
				if(!done) {
					const PPID a_bill_id = r_ati.BillID;
					const short a_r_by_bill = r_ati.RByBill;
					rbybill = tb_.GetNewRbb();
					THROW(trfr->SearchByBill(a_bill_id, 0, a_r_by_bill, 0) > 0);
					trfr->data.BillID  = r_ati.BillID  = pPack->Rec.ID;
					trfr->data.RByBill = r_ati.RByBill = rbybill;
					trfr->data.Flags &= ~PPTFR_QUOT;
					trfr->data.QuotPrice = 0.0;
					THROW_DB(trfr->updateRec());
					{
						LongArray new_row_idx_list;
						THROW(pPack->InsertRow(&r_ati, &new_row_idx_list));
						// @v10.9.7 {
						if(src_lotxcode_set.GetCount()) {
							if(new_row_idx_list.getCount() == 1) {
								pPack->XcL.Add(new_row_idx_list.get(0)+1, src_lotxcode_set); 
							}
						}
						// } @v10.9.7 
					}
					r_ati.QCert = 0;
				}
			}
			add_pack.destroy();
			THROW(FinishTFrame(pPack->Rec.ID, tb_));
			pPack->Rec.LastRByBill = tb_.Rbb();
		}
		//
		// Для всех документов, слинкованных с addBill, перекидываем ссылки на pPack->Rec.ID.
		//
		THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->LinkBillID == addBillID), set(P_Tbl->LinkBillID, dbconst(pPack->Rec.ID))));
		THROW(RemovePacket(addBillID, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::UniteReceiptBill(PPID destBillID, const PPIDArray & rSrcList, int use_ta)
{
	int    ok = 1, frrl_tag = 0;
	int    r_by_bill = 0;
	PPID   src_bill_id = 0, src_lot_id = 0; // @debug
	SString src_clb, dest_clb;
	LAssocArray ary;
	uint   i, j;
	PPTransferItem * p_ti, ti;
	PPBillPacket dest_pack;
	THROW(atobj->P_Tbl->LockingFRR(1, &frrl_tag, use_ta));
	THROW(ExtractPacketWithFlags(destBillID, &dest_pack, BPLD_LOCK) > 0);
	for(i = 0; dest_pack.EnumTItems(&i, &p_ti);)
		THROW_SL(ary.Add(p_ti->GoodsID, p_ti->LotID, 0, 0));
	ary.Sort();
	for(j = 0; j < rSrcList.getCount(); j++) {
		/*PPID*/   src_bill_id = rSrcList.at(j);
		BillTbl::Rec src_bill_rec;
		THROW(Search(src_bill_id, &src_bill_rec) > 0);
		if(dest_pack.Rec.Object == src_bill_rec.Object && dest_pack.Rec.OpID == src_bill_rec.OpID &&
			dest_pack.Rec.LocID == src_bill_rec.LocID && dest_pack.Rec.LinkBillID == src_bill_rec.LinkBillID) {
			int    recalc_dest_bill = 0;
			int    remove_src_bill  = 1;
			TBlock tb_;
			assert(destBillID == dest_pack.Rec.ID); // @paranoic
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(BeginTFrame(destBillID, tb_));
			for(r_by_bill = 0; trfr->EnumItems(src_bill_id, &r_by_bill, &ti) > 0;) {
				long   tmlof = TMLOF_RMVSRCLOT | TMLOF_AVGCOST | TMLOF_AVGPRICE;
				uint   pos = 0;
				/*PPID*/ src_lot_id  = ti.LotID;
				PPID   dest_lot_id = 0;
				PPID   goods_id    = ti.GoodsID;
				int    src_is_derived = 0, dest_is_derived = 0;
				GetClbNumberByLot(src_lot_id, &src_is_derived, src_clb);
				if(ary.BSearch(goods_id, &dest_lot_id, &pos)) {
					tmlof |= TMLOF_ADDLOTS;
					GetClbNumberByLot(dest_lot_id, &dest_is_derived, dest_clb);
					if(dest_clb.IsEmpty() && src_clb.NotEmpty())
						THROW(SetClbNumberByLot(dest_lot_id, src_clb, 0));
				}
				else {
					PPTransferItem new_ti;
					THROW(new_ti.Init(&dest_pack.Rec));
					THROW(new_ti.SetupGoods(goods_id));
					new_ti.UnitPerPack = ti.UnitPerPack;
					new_ti.Quantity_   = ti.Quantity_;
					new_ti.Cost        = ti.Cost;
					new_ti.Price       = ti.Price;
					new_ti.QCert       = ti.QCert;
					new_ti.Expiry      = ti.Expiry;
					THROW(trfr->AddItem(&new_ti, tb_.Rbb(), 0));
					dest_lot_id   = new_ti.LotID;
					THROW_SL(ary.Add(goods_id, dest_lot_id, 0, 1));
					recalc_dest_bill = 1;
					THROW(SetClbNumberByLot(dest_lot_id, src_clb, 0));
				}
				THROW(trfr->MoveLotOps(src_lot_id, dest_lot_id, tmlof, 0));
			}
			if(remove_src_bill)
				THROW(RemovePacket(src_bill_id, 0));
			if(recalc_dest_bill) {
				THROW(RecalcPayment(destBillID, 0));
				THROW(RecalcTurns(destBillID, 0, 0));
			}
			THROW(FinishTFrame(destBillID, tb_));
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	if(ok == 0)
		PPSaveErrContext();
	// @Caution @v4.0.7
	// Если этот вызов завершится с ошибкой, то остатки по
	// счетам останутся неправильными.
	if(!atobj->P_Tbl->LockingFRR(0, &frrl_tag, use_ta))
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
	if(ok == 0)
		PPRestoreErrContext();
	return ok;
}

int PPObjBill::UpdatePool(PPID poolID, int use_ta)
{
	int    ok = 1;
	PPBillPacket pack;
	AmtList amounts;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(ExtractPacket(poolID, &pack) > 0) {
			P_Tbl->CalcPoolAmounts(PPASS_OPBILLPOOL, poolID, &amounts);
			pack.Amounts.copy(amounts);
			const double main_amount = amounts.Get(PPAMT_MAIN, 0L/*@curID*/);
			pack.Rec.Amount = BR2(main_amount);
			THROW(FillTurnList(&pack));
			THROW(UpdatePacket(&pack, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::__TurnPacket(PPBillPacket * pPack, PPIDArray * pList, int skipEmpty, int use_ta)
{
	int    ok = 1;
	if(!skipEmpty || pPack->GetTCount()) {
		if(oneof2(pPack->OpTypeID, PPOPT_ACCTURN, PPOPT_PAYMENT)) {
			AmtList amt_list;
			amt_list.Put(PPAMT_MAIN, pPack->Rec.CurID, fabs(pPack->Rec.Amount), 0, 1);
			THROW(pPack->InitAmounts(&amt_list));
		}
		else {
			THROW(pPack->InitAmounts(0));
		}
		THROW(FillTurnList(pPack));
		THROW(TurnPacket(pPack, use_ta));
		if(pList)
			THROW(pList->addUnique(pPack->Rec.ID));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/int PPObjBill::ParseText(const char * pText, const char * pTemplate, PPImpExpParam::PtTokenList & rResultList, SString * pFileTemplate)
{
	enum {
		billsymbmodLink   = 0x40000000,
		billsymbmodReckon = 0x20000000,
		billsymbmodTSess  = 0x10000000,
	};

	// @INN_@DLVRLOCID_@BILLNO_@FGDATE.txt
	// @DLVRLOCTAG.tagsymb
	// 1001135228_7982_AB260_18042015.txt
	int    ok = 1;
	SString result_file_template;
	if(pTemplate) {
		const char * p_text = pText;
		SString field_value;
		PPSymbTranslator st;
		for(const char * p = pTemplate; *p;) {
			size_t next = 1;
			if(*p != '@') {
				THROW(*p_text == *p);
				result_file_template.CatChar(*p);
				p_text++;
			}
			else { // *p == '@'
				long   modif = 0;
				long   sym  = st.Translate(p, &next);
				long   ext_id = 0;
				field_value = 0;
				if(sym == 0) {
					assert(next == 1); // Если подстановка не удалась, то PPSymbTranslator::Translate не должен сдвигать позицию
					next = 1;
					THROW(*p_text == *p);
					result_file_template.CatChar(*p); // @v9.8.4 В случае, если '@' является назависимым символом - втыкаем ее в шаблон
					p_text++;
				}
				else {
					switch(sym) {
						case PPSYM_LINK:
							if(p[next] == '.') {
								modif = billsymbmodLink;
								p += (next+1);
								next = 0;
								sym = st.Translate(p, &next); // @recursion
							}
							else
								sym = 0;
							break;
						case PPSYM_RECKON:
							if(p[next] == '.') {
								modif = billsymbmodReckon;
								p += (next+1);
								next = 0;
								sym = st.Translate(p, &next); // @recursion
							}
							else
								sym = 0;
							break;
						case PPSYM_TSESS:
							modif = billsymbmodTSess;
							if(p[next] == '.') {
								p += (next+1);
								next = 0;
								sym = st.Translate(p, &next); // @recursion
							}
							else
								sym = 0;
							break;
						case PPSYM_DLVRLOCTAG:
							if(p[next] == '.') {
								char   tag_symb[64];
								p += (next+1);
								const char * p2 = p;
								PPObjTag tag_obj;
								PPObjectTag tag_rec;
								for(size_t tsp = 0; !ext_id && (tsp+1) < SIZEOFARRAY(tag_symb) && *p2;) {
									tag_symb[tsp++] = *p2++;
									tag_symb[tsp] = 0;
									PPID   local_tag_id = 0;
									if(tag_obj.FetchBySymb(tag_symb, &local_tag_id) > 0) {
										if(tag_obj.Fetch(local_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOCATION) {
											ext_id = local_tag_id;
											p = p2;
											next = 0;
										}
									}
								}
								if(ext_id)
									sym = PPSYM_DLVRLOCTAG;
								else
									sym = 0;
							}
							else
								sym = 0;
							break;
					}
					{
                        const char * p_text_next = p[next] ? sstrchr(p_text, p[next]) : 0;
                        size_t fld_len = 0;
                        if(p_text_next) {
							fld_len = (p_text_next - p_text);
							field_value.CatN(p_text, fld_len);
                        }
                        else {
							fld_len = sstrlen(p_text);
							field_value = p_text;
                        }
                        p_text += fld_len;
					}
					if(sym) {
						rResultList.Add(sym | modif, ext_id, field_value);
						result_file_template.CatChar('*');
					}
				}
			}
			p += next;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pFileTemplate, result_file_template);
	return ok;
}

int PPObjBill::SubstText(const PPBillPacket * pPack, const char * pTemplate, SString & rResult)
{
	class AgtBlock {
	public:
		AgtBlock() : Kind(0), P_CliAgt(0)
		{
		}
		~AgtBlock()
		{
			if(Kind == 1) {
				ZDELETE(P_CliAgt);
			}
			else if(Kind == 2) {
				ZDELETE(P_SupplAgt);
			}
		}
		int    Init(PPID arID)
		{
			int    ok = -1;
			if(Kind == 0) {
				PPObjArticle ar_obj;
				ArticleTbl::Rec ar_rec;
				if(ar_obj.Fetch(arID, &ar_rec) > 0) {
					int    k = PPObjArticle::GetAgreementKind(&ar_rec);
					if(k == 1) {
						PPClientAgreement ca_rec;
						if(ar_obj.GetClientAgreement(arID, &ca_rec, 0) > 0) {
							P_CliAgt = new PPClientAgreement;
							*P_CliAgt = ca_rec;
							Kind = k;
							ok = 1;
						}
					}
					else if(k == 2) {
						PPSupplAgreement sa_rec;
						if(ar_obj.GetSupplAgreement(arID, &sa_rec, 1) > 0) {
							P_SupplAgt = new PPSupplAgreement;
							*P_SupplAgt = sa_rec;
							Kind = k;
							ok = 2;
						}
					}
				}
			}
			else if(Kind == 1)
				ok = 1;
			else if(Kind == 2)
				ok = 2;
			return ok;
		}
		int    Kind;
		union {
			PPClientAgreement * P_CliAgt;
			PPSupplAgreement * P_SupplAgt;
		};
	};
	rResult.Z();

	int    ok = 1;
	SString subst_buf;
	PPObjTSession * p_tses_obj = 0;
	PPObjCSession * p_cses_obj = 0;
	TSessionTbl::Rec tsess_rec;
	CSessionTbl::Rec csess_rec;
	LocationTbl::Rec loc_rec;
	PPBillPacket * p_link_pack = 0, * p_rckn_pack = 0;
	if(pPack && !isempty(pTemplate)) {
		AgtBlock agt_blk;
		PPSymbTranslator st;
		for(const char * p = pTemplate; *p;) {
			size_t next = 1;
			if(*p == '@') {
				long   sym  = st.Translate(p, &next);
				long   ext_id = 0; // @v10.4.2
				if(sym == 0) {
					rResult.CatChar(*p);
					assert(next == 1); // Если подстановка не удалась, то PPSymbTranslator::Translate не должен сдвигать позицию
					next = 1;
				}
				else {
					const  PPBillPacket * pk = pPack;
					const TSessionTbl::Rec * p_tsess_rec = 0;
					const CSessionTbl::Rec * p_csess_rec = 0;
					switch(sym) {
						case PPSYM_LINK:
							if(p[next] == '.' && pPack->Rec.LinkBillID) {
								if(p_link_pack == 0) {
									THROW_MEM(p_link_pack = new PPBillPacket);
									THROW(ExtractPacketWithFlags(pPack->Rec.LinkBillID, p_link_pack, BPLD_SKIPTRFR));
								}
								pk = p_link_pack;
								p += (next+1);
								next = 0;
								sym = st.Translate(p, &next);
							}
							else
								sym = 0;
							break;
						case PPSYM_RECKON:
							if(p[next] == '.' && pPack->PaymBillID) {
								if(p_rckn_pack == 0) {
									THROW_MEM(p_rckn_pack = new PPBillPacket);
									THROW(ExtractPacketWithFlags(pPack->PaymBillID, p_rckn_pack, BPLD_SKIPTRFR));
								}
								pk = p_rckn_pack;
								p += (next+1);
								next = 0;
								sym = st.Translate(p, &next);
							}
							else
								sym = 0;
							break;
						case PPSYM_TSESS:
							if(pPack->CSessID && pPack->Rec.Flags & BILLF_TSESSWROFF) {
								SETIFZ(p_tses_obj, new PPObjTSession);
								THROW_MEM(p_tses_obj);
								const int r = p_tses_obj->Search(pPack->CSessID, &tsess_rec);
								if(r > 0) {
									if(p[next] == '.') {
										p_tsess_rec = &tsess_rec;
										p += (next+1);
										next = 0;
										sym = st.Translate(p, &next);
									}
									else {
										p_tses_obj->MakeName(&tsess_rec, subst_buf.Z());
										rResult.Cat(subst_buf);
										sym = 0;
									}
								}
								else {
									sym = 0;
									THROW(r);
								}
							}
							else
								sym = 0;
							break;
						case PPSYM_CSESS:
							if(pPack->CSessID && pPack->Rec.Flags & BILLF_CSESSWROFF) {
								SETIFZ(p_cses_obj, new PPObjCSession);
								THROW_MEM(p_cses_obj);
								const int r = p_cses_obj->Search(pPack->CSessID, &csess_rec);
								if(r > 0) {
									if(p[next] == '.') {
										p_csess_rec = &csess_rec;
										p += (next+1);
										next = 0;
										sym = st.Translate(p, &next);
									}
									else {
										PPObjCSession::MakeCodeString(&csess_rec, subst_buf);
										rResult.Cat(subst_buf);
										sym = 0;
									}
								}
								else {
									sym = 0;
									THROW(r);
								}
							}
							else
								sym = 0;
							break;
						case PPSYM_DLVRLOCTAG: // @v10.4.2
							if(p[next] == '.') {
								char   tag_symb[64];
								p += (next+1);
								const char * p2 = p;
								PPObjTag tag_obj;
								PPObjectTag tag_rec;
								for(size_t tsp = 0; !ext_id && (tsp+1) < SIZEOFARRAY(tag_symb) && *p2;) {
									tag_symb[tsp++] = *p2++;
									tag_symb[tsp] = 0;
									PPID   local_tag_id = 0;
									if(tag_obj.FetchBySymb(tag_symb, &local_tag_id) > 0) {
										if(tag_obj.Fetch(local_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOCATION) {
											ext_id = local_tag_id;
											p = p2;
											next = 0;
										}
									}
								}
								if(ext_id)
									sym = PPSYM_DLVRLOCTAG;
								else
									sym = 0;
							}
							else
								sym = 0;
							break;
					}
					subst_buf.Z();
					switch(sym) {
						case PPSYM_BILLNO: BillCore::GetCode(subst_buf = pk->Rec.Code); break;
						case PPSYM_DATE: subst_buf.Cat(pk->Rec.Dt, DATF_DMY); break;
						case PPSYM_DUEDATE: // @v10.4.8
							if(checkdate(pk->Rec.DueDate))
								subst_buf.Cat(pk->Rec.DueDate, DATF_DMY);
							break;
						case PPSYM_FGDATE: subst_buf.Cat(pk->Rec.Dt, DATF_DMY|DATF_CENTURY|DATF_NODIV); break;
						case PPSYM_FGDUEDATE: // @v10.4.8
							subst_buf.Cat(pk->Rec.DueDate, DATF_DMY|DATF_CENTURY|DATF_NODIV);
							break;
						case PPSYM_PAYDATE:
							{
								LDATE dt = ZERODATE;
								if(pk->GetLastPayDate(&dt) > 0)
									subst_buf.Cat(dt, DATF_DMY);
							}
							break;
						case PPSYM_INVOICEDATE:
							if(checkdate(pk->Ext.InvoiceDate))
								subst_buf.Cat(pk->Ext.InvoiceDate, DATF_DMY);
							else
								subst_buf.Cat(pk->Rec.Dt, DATF_DMY);
							break;
						case PPSYM_INVOICENO:
							if(pk->Ext.InvoiceCode[0])
								subst_buf = pk->Ext.InvoiceCode;
							else
								BillCore::GetCode(subst_buf = pk->Rec.Code);
							break;
						case PPSYM_AMOUNT: subst_buf.Cat(pk->GetAmount(), MKSFMTD(0, 0, NMBF_TRICOMMA)); break;
						case PPSYM_LOCCODE:
							if(pk->Rec.LocID && LocObj.Fetch(pk->Rec.LocID, &loc_rec) > 0)
								subst_buf.Cat(loc_rec.Code);
							break;
						case PPSYM_LOCATION: GetLocationName(pk->Rec.LocID, subst_buf); break;
						case PPSYM_OBJECT:   GetArticleName(pk->Rec.Object, subst_buf); break;
						case PPSYM_OBJINN: // @v10.5.0
						case PPSYM_OBJKPP: // @v10.5.0
							if(pk->Rec.Object) {
								PPID   person_id = ObjectToPerson(pk->Rec.Object);
								if(person_id) {
									PPObjPerson psn_obj;
									const PPID reg_type_id = (sym == PPSYM_OBJINN) ? PPREGT_TPID : ((sym == PPSYM_OBJKPP) ? PPREGT_KPP : 0);
									psn_obj.GetRegNumber(person_id, reg_type_id, pk->Rec.Dt, subst_buf);
								}
							}
							break;
						// @v10.7.3 {
						case PPSYM_OBJ2INN:     // @obj2inn ИНН персоналии, ассоциированной со дополнительной статьей документа
						case PPSYM_OBJ2KPP:     // @obj2kpp КПП персоналии, ассоциированной со дополнительной статьей документа
							if(pk->Rec.Object2) {
								PPID   person_id = ObjectToPerson(pk->Rec.Object2);
								if(person_id) {
									PPObjPerson psn_obj;
									const PPID reg_type_id = (sym == PPSYM_OBJ2INN) ? PPREGT_TPID : ((sym == PPSYM_OBJ2KPP) ? PPREGT_KPP : 0);
									psn_obj.GetRegNumber(person_id, reg_type_id, pk->Rec.Dt, subst_buf);
								}
							}
							break;
						// } @v10.7.3
						case PPSYM_BILLOBJ2: GetArticleName(pk->Rec.Object2, subst_buf); break;
						case PPSYM_DLVRLOCCODE:
							if(pk->GetDlvrAddrID() && LocObj.Fetch(pk->GetDlvrAddrID(), &loc_rec) > 0)
								subst_buf.Cat(loc_rec.Code);
							break;
						case PPSYM_DLVRLOCID:
							if(pk->GetDlvrAddrID() && LocObj.Fetch(pk->GetDlvrAddrID(), &loc_rec) > 0)
								subst_buf.Cat(loc_rec.ID);
							break;
						case PPSYM_DLVRLOCTAG: // @v10.4.1
							if(ext_id && pk->GetDlvrAddrID() && LocObj.Fetch(pk->GetDlvrAddrID(), &loc_rec) > 0)
								PPRef->Ot.GetTagStr(PPOBJ_LOCATION, pk->GetDlvrAddrID(), ext_id, subst_buf);
							break;
						case PPSYM_INN:
							{
								PPID   main_org_id = 0;
								pk->GetMainOrgID_(&main_org_id);
								if(main_org_id) {
									PPObjPerson psn_obj;
									psn_obj.GetRegNumber(main_org_id, PPREGT_TPID, pk->Rec.Dt, subst_buf);
								}
							}
							break;
						case PPSYM_KPP:
							{
								PPID   main_org_id = 0;
								pk->GetMainOrgID_(&main_org_id);
								if(main_org_id) {
									PPObjPerson psn_obj;
									psn_obj.GetRegNumber(main_org_id, PPREGT_KPP, pk->Rec.Dt, subst_buf);
								}
							}
							break;
						case PPSYM_MEMO:
							if(p_tsess_rec) {
								subst_buf = p_tsess_rec->Memo;
								break;
							}
							// @fallthrough
						case PPSYM_BILLMEMO:
							subst_buf = pk->Rec.Memo;
							break;
						case PPSYM_CLIENTADDR:
							{
								const PPID psn_id = ObjectToPerson(pk->Rec.Object);
								if(psn_id) {
									PPObjPerson psn_obj;
									psn_obj.GetAddress(psn_id, subst_buf);
								}
							}
							break;
						case PPSYM_PRC:
							if(p_tsess_rec)
								GetObjectName(PPOBJ_PROCESSOR, p_tsess_rec->PrcID, subst_buf, 0);
							break;
						case PPSYM_TECH:
							if(p_tsess_rec)
								GetObjectName(PPOBJ_TECH, p_tsess_rec->TechID, subst_buf, 0);
							break;
						case PPSYM_POSNODE:
							if(p_csess_rec && p_csess_rec->CashNodeID) {
								GetObjectName(PPOBJ_CASHNODE, p_csess_rec->CashNodeID, subst_buf, 0);
							}
							break;
						case PPSYM_AGTCODE:
							if(agt_blk.Init(pk->Rec.Object) == 1) {
								//
								// В @v5.7.12 номера есть только у соглашений с покупателями
								//
								subst_buf = agt_blk.P_CliAgt->Code2; // @v10.2.9 Code-->Code2
							}
							break;
						case PPSYM_AGTDATE:
							if(agt_blk.Init(pk->Rec.Object) > 0)
								if(agt_blk.Kind == 1)
									subst_buf.Cat(agt_blk.P_CliAgt->BegDt, DATF_DMY|DATF_CENTURY);
								else if(agt_blk.Kind == 2)
									subst_buf.Cat(agt_blk.P_SupplAgt->BegDt, DATF_DMY|DATF_CENTURY);
							break;
						case PPSYM_AGTEXPIRY:
							if(agt_blk.Init(pk->Rec.Object) > 0)
								if(agt_blk.Kind == 1)
									subst_buf.Cat(agt_blk.P_CliAgt->Expiry, DATF_DMY|DATF_CENTURY);
								else if(agt_blk.Kind == 2)
									subst_buf.Cat(agt_blk.P_SupplAgt->Expiry, DATF_DMY|DATF_CENTURY);
							break;
						case PPSYM_DUMMY:
							break;
						default:
							break;
					}
					rResult.Cat(subst_buf);
					ok = 2;
				}
			}
			else {
                rResult.CatChar(*p);
			}
			p += next;
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_link_pack;
	delete p_rckn_pack;
	delete p_tses_obj;
	delete p_cses_obj;
	return ok;
}

int PPObjBill::SubstMemo(PPBillPacket * pPack)
{
	int    ok = 1;
	SString temp_buf, result_buf;
	if(pPack->Rec.Memo[0] == 0 && P_OpObj->GetExtStrData(pPack->Rec.OpID, OPKEXSTR_MEMO, temp_buf) > 0) {
        THROW(SubstText(pPack, temp_buf.Strip(), result_buf));
		STRNSCPY(pPack->Rec.Memo, result_buf);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjBill::ConvertGenAccturnToExtAccBill(PPID srcID, PPID * pDestID, const CvtAt2Ab_Param * pParam, int use_ta)
{
	int    ok = 1;
	double amt;
	PPOprKind op_rec;
	PPBillPacket src_pack;
	PPBillPacket dest_pack;
	PPAccTurn * p_at = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ExtractPacket(srcID, &src_pack) > 0);
		THROW_PP(src_pack.Turns.getCount(), PPERR_UNABLECVTBILL2BILL);
		THROW(GetOpData(src_pack.Rec.OpID, &op_rec) > 0);
		THROW_PP(op_rec.OpTypeID == PPOPT_ACCTURN /*&&!(op_rec.Flags & OPKF_EXTACCTURN)*/, PPERR_UNABLECVTBILL2BILL);
		THROW(GetOpData(pParam->OpID, &op_rec) > 0);
		p_at = & src_pack.Turns.at(0);
		THROW(dest_pack.CreateBlank(pParam->OpID, 0, pParam->LocID ? pParam->LocID : src_pack.Rec.LocID, use_ta));
		dest_pack.Rec.Object  = pParam->ObjID;
		dest_pack.Rec.Object2 = pParam->ExtObjID;
		if(p_at->DbtSheet) {
			if(p_at->DbtSheet == op_rec.AccSheetID) {
				SETIFZ(dest_pack.Rec.Object, p_at->DbtID.ar);
			}
			else if(p_at->DbtSheet == op_rec.AccSheet2ID) {
				SETIFZ(dest_pack.Rec.Object2, p_at->DbtID.ar);
			}
		}
		if(p_at->CrdSheet) {
			if(p_at->CrdSheet == op_rec.AccSheetID) {
				SETIFZ(dest_pack.Rec.Object, p_at->CrdID.ar);
			}
			else if(p_at->CrdSheet == op_rec.AccSheet2ID) {
				SETIFZ(dest_pack.Rec.Object2, p_at->CrdID.ar);
			}
		}
		dest_pack.Rec.Dt = src_pack.Rec.Dt;
		STRNSCPY(dest_pack.Rec.Code, src_pack.Rec.Code);
		dest_pack.Rec.LocID = pParam->LocID ? pParam->LocID : src_pack.Rec.LocID;
		STRNSCPY(dest_pack.Rec.Memo, src_pack.Rec.Memo);
		amt = (pParam->Flags & CvtAt2Ab_Param::fNegAmount) ? -p_at->Amount : p_at->Amount;
		dest_pack.Rec.Amount = BR2(amt);
		dest_pack.Amounts.Put(PPAMT_MAIN, 0L, amt, 0, 1);
		//dest_pack.InitAmounts();
		THROW(FillTurnList(&dest_pack));
		THROW(TurnPacket(&dest_pack, 0));
		ASSIGN_PTR(pDestID, dest_pack.Rec.ID);
		THROW(RemovePacket(srcID, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PPObjBill::CreateNewInteractive_Param::CreateNewInteractive_Param() : PredefOp(poUndef), Bbt(0), OpID(0), LocID(0), Flags(0)
{
}

int PPObjBill::CreateNewInteractive_Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	const  uint64 signature = 0x18080BCD1FFC030BULL;
	int    ok = 1;
	int    do_regular = 1;
	if(dir < 0) {
		uint64 in_s = 0;
		THROW_SL(rBuf.Read(in_s));
		if(in_s == signature) {
			do_regular = 1;
		}
		else {
			THROW(rBuf.Unread(sizeof(in_s)));
			THROW_SL(rBuf.Read(Bbt));
			THROW_SL(rBuf.Read(OpID));
			THROW_SL(rBuf.Read(LocID));
		}
	}
	else if(dir > 0) {
		THROW_SL(rBuf.Write(signature));
	}
	if(do_regular) {
		THROW_SL(pSCtx->Serialize(dir, PredefOp, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Bbt, rBuf));
		THROW_SL(pSCtx->Serialize(dir, OpID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, LocID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjBill::CreateNewInteractive_Param::OpTypeListByBbt(PPID bbt, PPIDArray * pOpTypeList)
{
	int    ok = 1;
	THROW_INVARG(pOpTypeList);
	switch(bbt) {
		case bbtGoodsBills:
			pOpTypeList->addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
				PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CORRECTION, 0L);
			break;
		case bbtOrderBills: pOpTypeList->add(PPOPT_GOODSORDER); break;
		case bbtAccturnBills: pOpTypeList->add(PPOPT_ACCTURN); break;
		case bbtInventoryBills: pOpTypeList->add(PPOPT_INVENTORY); break;
		case bbtDraftBills: pOpTypeList->addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTQUOTREQ, 0L); break; // @v10.5.7 PPOPT_DRAFTQUOTREQ
		case bbtSpcChargeOnMarks: pOpTypeList->add(PPOPT_DRAFTRECEIPT); break; // @v10.9.0
	}
	pOpTypeList->sort();
	CATCHZOK
	return ok;
}

class BillCreateNewInteractiveParamDialog : public TDialog {
public:
	DECL_DIALOG_DATA(PPObjBill::CreateNewInteractive_Param);

	BillCreateNewInteractiveParamDialog() : TDialog(DLG_ADDBILLFLT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		PPID   bbt = 0;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		AddClusterAssocDef(CTL_ADDBILLFLT_RESERVED, 0, DlgDataType::poUndef);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 1, DlgDataType::poBuyerOrder);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 2, DlgDataType::poSupplOrder);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 3, DlgDataType::poReceipt);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 4, DlgDataType::poBuyerSale);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 5, DlgDataType::poIntrExpend);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 6, DlgDataType::poInventory);
		AddClusterAssoc(CTL_ADDBILLFLT_RESERVED, 7, DlgDataType::poAccTurn);
		SetClusterData(CTL_ADDBILLFLT_RESERVED, Data.PredefOp);
		AddClusterAssoc(CTL_ADDBILLFLT_FLAGS, 0, DlgDataType::fShowBrowserAfterCreation);
		SetClusterData(CTL_ADDBILLFLT_FLAGS, Data.Flags);
		bbt = (Data.Bbt >= 0) ? (Data.Bbt + 1) : 0;
		SetupPPObjCombo(this, CTLSEL_ADDBILLFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0);
		SetupStringCombo(this, CTLSEL_ADDBILLFLT_BBT, PPTXT_BILLTYPES, bbt);
		SetupOprKindList(Data.poUndef, bbt, Data.OpID);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		Data.PredefOp = GetClusterData(CTL_ADDBILLFLT_RESERVED);
		getCtrlData(CTLSEL_ADDBILLFLT_OP,  &Data.OpID);
		getCtrlData(CTLSEL_ADDBILLFLT_LOC, &Data.LocID);
		getCtrlData(CTLSEL_ADDBILLFLT_BBT, &Data.Bbt);
		GetClusterData(CTL_ADDBILLFLT_FLAGS, &Data.Flags);
		Data.Bbt--;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_ADDBILLFLT_BBT)) {
			PPID   bbt = getCtrlLong(CTLSEL_ADDBILLFLT_BBT);
			PPID   op_id = getCtrlLong(CTLSEL_ADDBILLFLT_OP);
			SetupOprKindList(Data.poUndef, bbt, op_id);
		}
		else if(event.isClusterClk(CTL_ADDBILLFLT_RESERVED)) {
			Data.PredefOp = GetClusterData(CTL_ADDBILLFLT_RESERVED);
			SetupOprKindList(Data.PredefOp, -1, 0);
		}
		else
			return;
		clearEvent(event);
	}
	void SetupOprKindList(long predefOp, PPID bbt, PPID opID)
	{
		UI_LOCAL_LOCK_ENTER
			PPObjOprKind op_obj;
			PPID   selected_op_id = 0;
			PPIDArray op_list;
			int    is_op_list_inited = 0;
			PPOprKind enum_opk_rec;
			if(predefOp) {
				bbt = -1;
				const PPCommConfig & r_ccfg = CConfig;
				switch(predefOp) {
					case DlgDataType::poUndef:
						break;
					case DlgDataType::poBuyerOrder:
						bbt = bbtOrderBills;
						{
							const PPID acs_id = r_ccfg.SellAccSheet;
							if(acs_id) {
								for(PPID enum_op_id = 0; EnumOperations(PPOPT_GOODSORDER, &enum_op_id, &enum_opk_rec) > 0;) {
									if(enum_opk_rec.AccSheetID == acs_id) {
										op_list.add(enum_op_id);
										is_op_list_inited = 1;
									}
								}
								SETIFZ(selected_op_id, op_list.getSingle());
							}
						}
						break;
					case DlgDataType::poSupplOrder:
						bbt = bbtDraftBills;
						selected_op_id = r_ccfg.DraftRcptOp;
						break;
					case DlgDataType::poReceipt:
						bbt = bbtGoodsBills;
						selected_op_id = r_ccfg.ReceiptOp;
						break;
					case DlgDataType::poBuyerSale:
						bbt = bbtGoodsBills;
						{
							const PPID acs_id = r_ccfg.SellAccSheet;
							if(acs_id) {
								for(PPID enum_op_id = 0; EnumOperations(PPOPT_GOODSEXPEND, &enum_op_id, &enum_opk_rec) > 0;) {
									if(enum_opk_rec.AccSheetID == acs_id) {
										if(!selected_op_id && enum_opk_rec.Flags & OPKF_ONORDER)
											selected_op_id = enum_op_id;
										op_list.add(enum_op_id);
										is_op_list_inited = 1;
									}
								}
							}
						}
						break;
					case DlgDataType::poIntrExpend:
						bbt = bbtGoodsBills;
						{
							for(PPID enum_op_id = 0; EnumOperations(PPOPT_GOODSEXPEND, &enum_op_id, &enum_opk_rec) > 0;) {
								if(IsIntrExpndOp(enum_opk_rec.ID)) {
									op_list.add(enum_op_id);
									is_op_list_inited = 1;
								}
							}
							SETIFZ(selected_op_id, op_list.getSingle());
						}
						break;
					case DlgDataType::poInventory:
						bbt = bbtInventoryBills;
						break;
					case DlgDataType::poAccTurn:
						bbt = bbtAccturnBills;
						selected_op_id = PPOPK_GENERICACCTURN;
						break;
				}
				if(bbt >= 0)
					setCtrlLong(CTLSEL_ADDBILLFLT_BBT, bbt+1);
				if(selected_op_id)
					setCtrlLong(CTLSEL_ADDBILLFLT_OP, selected_op_id);
			}
			else {
				selected_op_id = opID;
				bbt--;
			}
			if((bbt+1) == bbtSpcChargeOnMarks) {
				PPID   temp_op_id = PPOPK_EDI_CHARGEONWITHMARKS;
				if(GetOpData(temp_op_id, &enum_opk_rec) > 0) {
					op_list.add(temp_op_id);
					is_op_list_inited = 1;
					selected_op_id = temp_op_id;
				}
				else if(CONFIRM(PPCFM_CREATESPCOPRKIND)) { // Выбранная категории документов требует специальной зарезервированной операции. Создать ее сейчас?
					temp_op_id = 0;
					if(op_obj.GetEdiChargeOnWithMarksOp(&temp_op_id, 1) > 0) {
						op_list.add(temp_op_id);
						is_op_list_inited = 1;
						selected_op_id = temp_op_id;
					}
					else {
						op_list.clear();
						is_op_list_inited = 1;
						selected_op_id = 0;
						PPError();
					}
				}
				else {
					op_list.clear();
					is_op_list_inited = 1;
					selected_op_id = 0;
				}
			}
			if(!is_op_list_inited) {
				PPIDArray op_type_list;
				DlgDataType::OpTypeListByBbt(bbt, &op_type_list);
				for(PPID enum_op_id = 0; EnumOperations(0, &enum_op_id, &enum_opk_rec) > 0;) {
					if(op_type_list.bsearch(enum_opk_rec.OpTypeID, 0) > 0)
						op_list.add(enum_op_id);
				}
				SETIFZ(selected_op_id, op_list.getSingle());
			}
			// @v10.9.0 opID = oneof2(PrevBbt, 0, bbt) ? opID : 0;
			if(!op_list.lsearch(selected_op_id))
				selected_op_id = 0;
			SetupOprKindCombo(this, CTLSEL_ADDBILLFLT_OP, selected_op_id, 0, &op_list, OPKLF_OPLIST);
		UI_LOCAL_LOCK_LEAVE
	}
};

int PPObjBill::EditCreateNewInteractiveParam(CreateNewInteractive_Param * pData)
{
	DIALOG_PROC_BODY(BillCreateNewInteractiveParamDialog, pData);
}

int PPObjBill::CreateNewInteractive(CreateNewInteractive_Param * pP)
{
	int    ok = -1;
	CreateNewInteractive_Param param;
	if(!pP && EditCreateNewInteractiveParam(&param) > 0) {
		pP = &param;
	}
	if(pP && pP->Bbt >= 0) {
		int    r = 1;
		if(!pP->LocID || !pP->OpID) {
			PPIDArray op_type_list;
			SETIFZ(pP->LocID, LConfig.Location);
			PPObjBill::CreateNewInteractive_Param::OpTypeListByBbt(pP->Bbt, &op_type_list);
			r = BillPrelude(&op_type_list, 0, 0, &pP->OpID, &pP->LocID);
		}
		if(r > 0) {
			PPID   id = 0;
			const  PPID save_loc_id = LConfig.Location;
			DS.SetLocation(pP->LocID);
			if(GetOpType(pP->OpID) == PPOPT_ACCTURN && !CheckOpFlags(pP->OpID, OPKF_EXTACCTURN))
				r = AddGenAccturn(&id, pP->OpID, 0);
			else {
				BillFilt bill_filt;
				bill_filt.SetupBrowseBillsType(static_cast<BrowseBillsType>(pP->Bbt));
				bill_filt.OpID = pP->OpID;
				bill_filt.LocList.Add(pP->LocID);
				r = AddGoodsBillByFilt(&id, &bill_filt, pP->OpID);
			}
			// @v10.9.0 {
			if(r > 0 && pP->Flags & pP->fShowBrowserAfterCreation) {
				BillTbl::Rec bill_rec;
				if(Search(id, &bill_rec) > 0) {
					BillFilt bill_flt;
					switch(GetOpType(bill_rec.OpID)) {
						case PPOPT_DRAFTEXPEND:
						case PPOPT_DRAFTRECEIPT:
						case PPOPT_DRAFTQUOTREQ:
						case PPOPT_DRAFTTRANSIT: bill_flt.Bbt = bbtDraftBills; break;
						case PPOPT_ACCTURN: bill_flt.Bbt = bbtAccturnBills; break;
						case PPOPT_INVENTORY: bill_flt.Bbt = bbtInventoryBills; break;
						case PPOPT_GOODSORDER: bill_flt.Bbt = bbtOrderBills; break;
						case PPOPT_POOL: bill_flt.Bbt = bbtPoolBills;
					}
					bill_flt.SetupBrowseBillsType(bill_flt.Bbt);
					bill_flt.OpID = bill_rec.OpID;
					bill_flt.Period.SetDate(bill_rec.Dt);
					bill_flt.Sel = id;
					BillFilt::FiltExtraParam p(0, bill_flt.Bbt);
					PPView::Execute(PPVIEW_BILL, &bill_flt, GetModelessStatus(), &p);
				}
			}
			// } @v10.9.0 
			DS.SetLocation(save_loc_id);
			ok = (r == cmOK) ? 1 : -1;
		}
	}
	return ok;
}

int PPObjBill::ConvertUuid7601()
{
	int    ok = 1;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	THROW_PP(tag_obj.Fetch(PPTAG_BILL_UUID, &tag_rec) > 0, PPERR_BILLTAGUUIDABS);
	{
		Reference * p_ref = PPRef;
		PropertyTbl::Key1 pk1;
		PPTransaction tra(1);
		THROW(tra);
		MEMSZERO(pk1);
		pk1.ObjType = PPOBJ_BILL;
		pk1.Prop = BILLPRP_GUID;
		for(int sp = spGe; p_ref->Prop.searchForUpdate(1, &pk1, sp) && p_ref->Prop.data.ObjType == PPOBJ_BILL && p_ref->Prop.data.Prop == BILLPRP_GUID; sp = spNext) {
			const S_GUID uuid = *reinterpret_cast<const S_GUID *>(p_ref->Prop.data.Text);
			const PPID bill_id = p_ref->Prop.data.ObjID;
			BillTbl::Rec bill_rec;
			if(Search(bill_id, &bill_rec) > 0) {
				ObjTagItem tag;
				THROW(tag.SetGuid(PPTAG_BILL_UUID, &uuid));
				THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag, 0));
			}
			// @temp(Временно не будем удалять старые записи) THROW_DB(p_ref->Prop.deleteRec()); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

/* @debug
int TestPPObjBillParseText()
{
	SString templ, name;
	templ = "@INN_@DLVRLOCID_@BILLNO_@FGDATE";
	name = "1001135228_7982_AB260_18042015";
	StrAssocArray result_list;
	int    ok = PPObjBill::ParseText(name, templ, result_list);
	return ok;
}*/

#if SLTEST_RUNNING // {

struct BillGuidAssocItem {
	PPID   BillID;
	S_GUID Uuid;
};

SLTEST_R(PPBillGuid)
{
	int    ok = 1, r;
	TSArray <BillGuidAssocItem> g_list;
	SString temp_buf, op_symb;
	PPOprKind op_rec;
	BillTbl::Rec bill_rec;
	uint argp = 0;
	if(EnumArg(&argp, temp_buf)) {
		op_symb = temp_buf;
	}
	THROW(SLTEST_CHECK_LT(0L, GetOpBySymb(op_symb, &op_rec)));
	{
		const uint   max_count = 50;
		PPTransaction tra(1);
		THROW(SLTEST_CHECK_NZ(tra));
		for(SEnum en = BillObj->P_Tbl->EnumByOp(op_rec.ID, 0, 0); g_list.getCount() < max_count && en.Next(&bill_rec) > 0;) {
			BillGuidAssocItem g_item;
			g_item.BillID = bill_rec.ID;
			g_item.Uuid.Generate();
			THROW(SLTEST_CHECK_NZ(BillObj->PutGuid(g_item.BillID, &g_item.Uuid, 0)));
			g_list.insert(&g_item);
		}
		THROW(SLTEST_CHECK_NZ(tra.Commit()));
	}
	{
		g_list.shuffle();
		for(uint i = 0; i < g_list.getCount(); i++) {
			const BillGuidAssocItem & r_item = g_list.at(i);
			S_GUID uuid;
			SLTEST_CHECK_LT(0L, (r = BillObj->GetGuid(r_item.BillID, &uuid)));
			if(r > 0) {
				SLTEST_CHECK_NZ(uuid == r_item.Uuid);
			}
		}
	}
	{
		g_list.shuffle();
		for(uint i = 0; i < g_list.getCount(); i++) {
			const BillGuidAssocItem & r_item = g_list.at(i);
			SLTEST_CHECK_NZ(BillObj->SearchByGuid(r_item.Uuid, &bill_rec) == 1);
			if(r == 0) {
				SLTEST_CHECK_EQ(r_item.BillID, bill_rec.ID);
			}
		}
	}
	CATCHZOK
	return CurrentStatus;
}

SLTEST_R(PPBillFormula)
{
	int    ok = 1;
	SString temp_buf, out_msg;
	PPIDArray bill_list;
	SStrCollection l_formula_list, r_formula_list;
	PPObjBill obj_bill;

	(temp_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("ppbillformula.tab");
	STabFile in_file(temp_buf, 0);

	(temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("ppbillformula.log");
	SFile out_file(temp_buf, SFile::mWrite);
	STab tab;
	STab::Row row;

	THROW_SL(in_file.IsValid());
	THROW_SL(in_file.LoadTab("BILLFORMULATEST", tab) > 0);
	{
		for(uint i = 0; i < tab.GetCount(); i++) {
			tab.GetRow(i, row);
			if(row.Get(0, temp_buf.Z())) {
				if(temp_buf.CmpNC("Bill") == 0) {
					PPID  op_id   = 0;
					LDATE bill_dt = ZERODATE;
					if(row.Get(1, temp_buf.Z())) {
						op_id = temp_buf.ToLong();
						if(row.Get(2, temp_buf.Z())) {
							strtodate(temp_buf, DATF_DMY, &bill_dt);
							if(row.Get(3, temp_buf.Z())) {
								PPID id = 0;
								BillTbl::Rec bill_rec;
								// @v10.6.4 MEMSZERO(bill_rec);
								temp_buf.CopyTo(bill_rec.Code, sizeof(bill_rec.Code));
								bill_rec.OpID = op_id;
								bill_rec.Dt   = bill_dt;
								if(obj_bill.P_Tbl->SearchAnalog(&bill_rec, BillCore::safDefault, &id, 0) > 0)
									bill_list.add(id);
								else {
									out_msg.Printf("FAIL: Bill not found code=%s", temp_buf.cptr()).CR();
									out_file.WriteLine(out_msg);
								}
							}
							else
								out_file.WriteLine("FAIL: Bill code not valid");
						}
						else
							out_file.WriteLine("FAIL: Bill date not valid");
					}
					else
						out_file.WriteLine("FAIL: Bill op_id not valid");
				}
				else {
					SString l_formula;
					l_formula = temp_buf;
					if(row.Get(1, temp_buf.Z())) {
						char * p = newStr(l_formula);
						if(p)
							l_formula_list.insert(p);
						p = newStr(temp_buf);
						if(p)
							r_formula_list.insert(p);
					}
					else {
						out_msg.Printf("FAIL: right formula not defined recno=%d", i).CR();
						out_file.WriteLine(out_msg);
					}
				}
			}
		}
	}
	if(bill_list.getCount() > 0 && l_formula_list.getCount() > 0 && r_formula_list.getCount() == l_formula_list.getCount()) {
		uint formulas_count = l_formula_list.getCount();
		for(uint i = 0; i < bill_list.getCount(); i++) {
			PPBillPacket bpack;
			if(obj_bill.ExtractPacket(bill_list.at(i), &bpack) > 0) {
				// @v10.3.0 double bill_amt = bpack.GetAmount();
				for(uint j = 0; j < formulas_count; j++) {
					const char * p_lformula = l_formula_list.at(j), * p_rformula = r_formula_list.at(j);
					double left_amt = 0.0, right_amt = 0.0;
					PPCalcExpression(p_lformula, &left_amt,  &bpack, bpack.Rec.CurID, 0);
					PPCalcExpression(p_rformula, &right_amt, &bpack, bpack.Rec.CurID, 0);
					if(left_amt == right_amt)
						out_msg.Z().Printf("OK: left=\"%s\" == right=\"%s\" result=%.2lf, doc_no=%s", p_lformula, p_rformula, right_amt, bpack.Rec.Code).CR();
					else
						out_msg.Z().Printf("FAIL: left=\"%s\" != right=\"%s\" result_left=%.2lf, result_right=%.2lf, doc_no=%s", p_lformula, p_rformula, left_amt, right_amt, bpack.Rec.Code).CR();
					out_file.WriteLine(out_msg);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING
