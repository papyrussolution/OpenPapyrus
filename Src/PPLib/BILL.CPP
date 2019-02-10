// BILL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// BillCore
//
#define WHITE_LABEL_SIGN '!'
#define BILLCODE_SIZ     sizeof(((BillTbl::Rec*)0)->Code)

// static
char * FASTCALL BillCore::SetCode(char * code, long f)
{
	char   temp[BILLCODE_SIZ];
	strip(code);
	if(f & BILLF_WHITELABEL) {
		if(code[0] != WHITE_LABEL_SIGN) {
			temp[0] = WHITE_LABEL_SIGN;
			strnzcpy(temp+1, code, BILLCODE_SIZ-1);
			strcpy(code, temp);
		}
	}
	else if(code[0] == WHITE_LABEL_SIGN)
		strcpy(code, code+1);
	return code;
}

//static
double SLAPI BillCore::GetQttyEpsilon()
{
	int    eps = CConfig.IltiCvtQttyEpsilon;
	return (eps > 0 && eps <= 10000) ? (1.0E-7 * (double)eps) : 1.0E-7; // @v8.1.11 (eps <= 1000)-->(eps <= 10000) // @v9.3.10 (: 1.0E-6)-->(: 1.0E-7)
}

//static
int FASTCALL BillCore::GetRecadvStatus(const BillTbl::Rec & rRec)
{
	int   recadv_status = PPEDI_RECADV_STATUS_UNDEF;
	if(rRec.Flags2 & BILLF2_RECADV_ACCP)
		recadv_status = (rRec.Flags2 & BILLF2_RECADV_DECL) ? PPEDI_RECADV_STATUS_PARTACCEPT : PPEDI_RECADV_STATUS_ACCEPT;
	else if(rRec.Flags2 & BILLF2_RECADV_DECL)
		recadv_status = PPEDI_RECADV_STATUS_REJECT;
	return recadv_status;
}

//static
int FASTCALL BillCore::SetRecadvStatus(int recadvStatus, BillTbl::Rec & rRec)
{
	int    ok = -1;
	const  long preserve_flags2 = rRec.Flags2;
	switch(recadvStatus) {
		case PPEDI_RECADV_STATUS_ACCEPT:
			rRec.Flags2 |= BILLF2_RECADV_ACCP;
			rRec.Flags2 &= ~BILLF2_RECADV_DECL;
			break;
		case PPEDI_RECADV_STATUS_PARTACCEPT:
			rRec.Flags2 |= BILLF2_RECADV_ACCP;
			rRec.Flags2 |= BILLF2_RECADV_DECL;
			break;
		case PPEDI_RECADV_STATUS_REJECT:
			rRec.Flags2 &= ~BILLF2_RECADV_ACCP;
			rRec.Flags2 |= BILLF2_RECADV_DECL;
			break;
		case PPEDI_RECADV_STATUS_UNDEF:
			rRec.Flags2 &= ~BILLF2_RECADV_ACCP;
			rRec.Flags2 &= ~BILLF2_RECADV_DECL;
			break;
		default:
			ok = 0;
			break;
	}
	if(preserve_flags2 != rRec.Flags2)
		ok = 1;
	return ok;
}

//static
int FASTCALL BillCore::GetRecadvConfStatus(const BillTbl::Rec & rRec)
{
	int    recadv_conf_status = PPEDI_RECADVCONF_STATUS_UNDEF;
	const  long _f = CHKXORFLAGS(rRec.Flags2, BILLF2_EDIAR_AGR, BILLF2_EDIAR_DISAGR);
	if(_f & BILLF2_EDIAR_AGR)
		recadv_conf_status = PPEDI_RECADVCONF_STATUS_ACCEPT;
	else if(_f & BILLF2_EDIAR_DISAGR)
		recadv_conf_status = PPEDI_RECADVCONF_STATUS_REJECT;
	return recadv_conf_status;
}

//static
int SLAPI BillCore::SetRecadvConfStatus(int recadvConfStatus, BillTbl::Rec & rRec)
{
	int    ok = -1;
	const  long preserve_flags2 = rRec.Flags2;
	switch(recadvConfStatus) {
		case PPEDI_RECADVCONF_STATUS_ACCEPT:
			rRec.Flags2 |= BILLF2_EDIAR_AGR;
			rRec.Flags2 &= ~BILLF2_EDIAR_DISAGR;
			break;
		case PPEDI_RECADVCONF_STATUS_REJECT:
			rRec.Flags2 &= ~BILLF2_EDIAR_AGR;
			rRec.Flags2 |= BILLF2_EDIAR_DISAGR;
			break;
		case PPEDI_RECADVCONF_STATUS_UNDEF:
			rRec.Flags2 &= ~(BILLF2_EDIAR_AGR|BILLF2_EDIAR_DISAGR);
			break;
		default:
			ok = 0;
	}
	if(preserve_flags2 != rRec.Flags2)
		ok = 1;
	return ok;
}

// static
char * FASTCALL BillCore::GetCode(char * code)
	{ return (code[0] == WHITE_LABEL_SIGN) ? strcpy(code, code+1) : code; }
//static
SString & FASTCALL BillCore::GetCode(SString & rCode)
	{ return rCode.Strip().ShiftLeftChr(WHITE_LABEL_SIGN); }

SLAPI BillCore::BillCore() : BillTbl()
{
}

int SLAPI BillCore::Search(PPID id, BillTbl::Rec * pRec)
	{ return SearchByID(this, PPOBJ_BILL, id, pRec); }

int SLAPI BillCore::CheckAmtTypeRef(PPID amtTypeID, PPID * pBillID)
{
	BillAmountTbl::Key0 k;
	BillAmountTbl * t = &AmtT;
	BExtQuery q(t, 0, 1);
	q.select(t->AmtTypeID, t->BillID, 0).where(t->AmtTypeID == amtTypeID);
	MEMSZERO(k);
	q.initIteration(0, &k, spFirst);
	if(q.nextIteration() > 0) {
		ASSIGN_PTR(pBillID, t->data.BillID);
		return 1;
	}
	else
		return -1;
}

int SLAPI BillCore::GetAmount(PPID billID, PPID amtTypeID, PPID curID, double * pAmount)
{
	int    ok = -1;
	double val = 0.0;
	BillAmountTbl::Key0 k;
	k.BillID    = billID;
	k.AmtTypeID = amtTypeID;
	k.CurID     = curID;
	if(AmtT.search(0, &k, spEq)) {
		val = AmtT.data.Amount;
		ok = 1;
	}
	else
		ok = PPDbSearchError();
	ASSIGN_PTR(pAmount, val);
	return ok;
}

int SLAPI BillCore::GetAmountList(PPID billID, AmtList * pList)
{
	const int use_ext_query = 0; // @v8.4.7 1-->0 (фактически, добавлен код, реализующий !use_ext_query)
	int    ok = 1;
	BillAmountTbl::Key0 k;
	k.BillID    = billID;
	k.AmtTypeID = 0;
	k.CurID     = 0;
	PROFILE_START
	if(use_ext_query) {
		BExtQuery q(&AmtT, 0);
		q.selectAll().where(AmtT.BillID == billID);
		for(q.initIteration(0, &k, spGt); ok && q.nextIteration() > 0;) {
			AmtEntry entry(AmtT.data.AmtTypeID, AmtT.data.CurID, AmtT.data.Amount);
			if(!pList->insert(&entry))
				ok = PPSetErrorSLib();
		}
	}
	else {
		if(AmtT.search(0, &k, spGt) && AmtT.data.BillID == billID) do {
			AmtEntry entry(AmtT.data.AmtTypeID, AmtT.data.CurID, AmtT.data.Amount);
			if(!pList->insert(&entry))
				ok = PPSetErrorSLib();
		} while(ok && AmtT.search(0, &k, spNext) && AmtT.data.BillID == billID);
	}
	PROFILE_END
	return ok;
}

int SLAPI BillCore::UpdateAmount(PPID billID, const AmtEntry * pEntry, int removeZero)
{
	int    ok = 1;
	BillAmountTbl::Key0 k;
	if(pEntry) {
		k.BillID    = billID;
		k.AmtTypeID = pEntry->AmtTypeID;
		k.CurID     = pEntry->CurID;
		if(AmtT.search(0, &k, spEq)) {
			if(removeZero && pEntry->Amt == 0.0) {
				DBRowId _dbpos;
				THROW_DB(AmtT.getPosition(&_dbpos));
				THROW_DB(AmtT.getDirectForUpdate(0, &k, _dbpos));
				THROW_DB(AmtT.deleteRec()); // @sfu
			}
			else if(pEntry->Amt != AmtT.data.Amount) {
				DBRowId _dbpos;
				THROW_DB(AmtT.getPosition(&_dbpos));
				THROW_DB(AmtT.getDirectForUpdate(0, &k, _dbpos));
				AmtT.data.Amount = pEntry->Amt;
				THROW_DB(AmtT.updateRec()); // @sfu
			}
			else {
				ok = -1;
			}
		}
		else {
			THROW(PPDbSearchError());
			if(!removeZero || pEntry->Amt != 0.0) {
				AmtT.data.BillID    = billID;
				AmtT.data.AmtTypeID = pEntry->AmtTypeID;
				AmtT.data.CurID     = pEntry->CurID;
				AmtT.data.Amount    = pEntry->Amt;
				THROW_DB(AmtT.insertRec());
			}
		}
	}
	else {
		THROW_DB(deleteFrom(&AmtT, 0, (AmtT.BillID == billID)));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::PayPlanEnumerator(const DateRange * pPeriod, PayPlanEnumProc proc, void * extraPtr)
{
	int    ok = 1;
	PayPlanTbl::Key0 k0;
	BExtQuery q(&Pays, 0, 64);
	q.selectAll().where(daterange(Pays.PayDate, pPeriod));
	MEMSZERO(k0);
	for(q.initIteration(0, &k0, spFirst); ok > 0 && q.nextIteration() > 0;) {
		PayPlanTbl::Rec rec;
		Pays.copyBufTo(&rec);
		int r = proc(&rec, extraPtr);
		if(r <= 0)
			ok = r;
	}
	return ok;
}

int SLAPI BillCore::GetLastPayDate(PPID billID, LDATE * pDate)
{
	PayPlanTbl::Key0 k;
	k.BillID  = billID;
	k.PayDate = MAXDATE;
	if(Pays.search(0, &k, spLt) && k.BillID == billID) {
		*pDate = Pays.data.PayDate;
		return 1;
	}
	else {
		*pDate = ZERODATE;
		return PPDbSearchError();
	}
}

int SLAPI BillCore::GetPayPlan(PPID billID, PayPlanArray * pList)
{
	PayPlanTbl::Key0 k;
	MEMSZERO(k);
	k.BillID  = billID;
	if(Pays.search(0, &k, spGe) && k.BillID == billID) {
		do {
			if(!pList->insert(&Pays.data))
				return PPSetErrorSLib();
		} while(Pays.search(0, &k, spNext) && k.BillID == billID);
	}
	return PPDbSearchError() ? 1 : 0;
}

int SLAPI BillCore::PutPayPlan(PPID billID, const PayPlanArray * pList, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(&Pays, 0, Pays.BillID == billID));
		if(pList) {
			PayPlanTbl::Rec * p_rec;
			for(uint i = 0; pList->enumItems(&i, (void **)&p_rec);) {
				PayPlanTbl::Rec rec = *p_rec;
				rec.BillID = billID;
				THROW_DB(Pays.insertRecBuf(&rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::GetLastPayment(PPID billID, BillTbl::Rec * pRec)
{
	BillTbl::Key4 k;
	MEMSZERO(k);
	k.LinkBillID = billID;
	k.Dt = MAXDATE;
	k.BillNo = MAXLONG;
	while(search(4, &k, spLt) && k.LinkBillID == billID)
		if(GetOpType(data.OpID) == PPOPT_PAYMENT && data.Amount > 0.0) {
			copyBufTo(pRec);
			return 1;
		}
	return PPDbSearchError();
}

int SLAPI BillCore::_GetBillNo(LDATE date, long * pBillNo)
{
	return IncDateKey(this, 1, date, pBillNo);
}

int SLAPI BillCore::GetRentCondition(PPID id, PPRentCondition * pRC)
{
	PropertyTbl::Rec prp_rec;
	int    r = PPRef->GetProperty(PPOBJ_BILL, id, BILLPRP_RENT, &prp_rec, sizeof(prp_rec));
	if(pRC)
		if(r > 0)
			memcpy(pRC, prp_rec.Text, sizeof(*pRC));
		else
			memzero(pRC, sizeof(*pRC));
	return r;
}

int SLAPI BillCore::SetFreight(PPID id, const PPFreight * pFreight, int use_ta)
{
	return PPRef->PutProp(PPOBJ_BILL, id, BILLPRP_FREIGHT, pFreight, sizeof(*pFreight), use_ta);
}

int SLAPI BillCore::GetFreight(PPID id, PPFreight * pFreight)
{
	PPFreight freight;
	if(PPRef->GetProperty(PPOBJ_BILL, id, BILLPRP_FREIGHT, &freight, sizeof(freight)) > 0) {
		ASSIGN_PTR(pFreight, freight);
		return 1;
	}
	else {
		memzero(pFreight, sizeof(*pFreight));
		return -1;
	}
}

int SLAPI BillCore::GetListByFreightFilt(const FreightFilt & rFilt, UintHashTable & rList)
{
	int    ok = -1;
	if(!rFilt.ShipmPeriod.IsZero())
		ok = 0;
	else if(!rFilt.ArrvlPeriod.IsZero())
		ok = 0;
	else if(rFilt.ShipID)
		ok = 0;
	else if(rFilt.PortID)
		ok = 0;
	else if(rFilt.PortOfLoading)
		ok = 0;
	else if(rFilt.CaptainID)
		ok = 0;
	if(ok == 0) {
		ok = -1;
		PropertyTbl * p_prop = &PPRef->Prop;
		PropertyTbl::Key0 k0;
		BExtQuery q(p_prop, 0, 256);
		DBQ * dbq = &(p_prop->ObjType == PPOBJ_BILL && p_prop->Prop == BILLPRP_FREIGHT);
		if(rFilt.ShipID)
			dbq = &(*dbq && p_prop->Val2 == rFilt.ShipID);
		q.select(p_prop->ObjType, p_prop->ObjID, p_prop->Prop,
			p_prop->Text, p_prop->Val1, p_prop->Val2, 0L).where(*dbq);
		MEMSZERO(k0);
		k0.ObjType = PPOBJ_BILL;
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
			const PPFreight & r_fr = *(PPFreight *)&p_prop->data;
			if(r_fr.CheckForFilt(rFilt)) {
				rList.Add((uint32)r_fr.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI BillCore::GetDlvrAddrList(LAssocArray * pList)
{
	int    ok = -1;
	PropertyTbl * p_prop = &PPRef->Prop;
	PropertyTbl::Key0 k0;
	BExtQuery q(p_prop, 0);
	q.select(p_prop->ObjID, p_prop->Text, 0L).where(p_prop->ObjType == PPOBJ_BILL && p_prop->Prop == BILLPRP_FREIGHT);
	MEMSZERO(k0);
	k0.ObjType = PPOBJ_BILL;
	CALLPTRMEMB(pList, clear());
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
		PPFreight * p_freight = (PPFreight *)&p_prop->data;
		if(p_freight->DlvrAddrID) {
			if(pList) {
				THROW_SL(pList->Add(p_freight->ID, p_freight->DlvrAddrID, 0));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
// Storage for value added record for PPOBJ_BILL
// Used if (BillTbl::Rec::Flags & BILLF_EXTRA)
//
struct PPBillExtra_Strg_Before6407 {  // sizeof(PPBillExtra_Strg) == PROPRECFIXSIZE
	PPID   ObjType;            // const=PPOBJ_BILL
	PPID   ObjID;              // -> Bill.ID
	PPID   PropID;             // const=BILLPRP_EXTRA
	PPID   PayerID;            // Плательщик -> Article.ID
	PPID   AgentID;            // Агент      -> Article.ID
	char   InvoiceCode[10];    //
	LDATE  InvoiceDate;        //
	char   PaymBillCode[10];   // Номер платежного документа (для печати в счете-фактуре)
	LDATE  PaymBillDate;       // Дата  платежного документа (для печати в счете-фактуре)
	PPID   ExtPriceQuotKindID; //
	char   Reserve[32];        // @reserve
};

int SLAPI ConvertBillExtRec_6407(PropertyTbl::Rec * pRec)
{
	int    ok = -1;
	BillCore::Extra_Strg * p_rec = (BillCore::Extra_Strg *)pRec;
	if(p_rec->CvtTag != 1) {
		PPBillExtra_Strg_Before6407 * p_b6407 = (PPBillExtra_Strg_Before6407 *)pRec;
		BillCore::Extra_Strg rec;
		MEMSZERO(rec);
#define __FLD(f) rec.f = p_b6407->f
		__FLD(ObjType);
		__FLD(ObjID);
		__FLD(PropID);
		__FLD(ExtPriceQuotKindID);
		__FLD(InvoiceDate);
		__FLD(PaymBillDate);
		__FLD(AgentID);
		__FLD(PayerID);
#undef __FLD
		STRNSCPY(rec.InvoiceCode, p_b6407->InvoiceCode);
		STRNSCPY(rec.PaymBillCode, p_b6407->PaymBillCode);
		rec.CvtTag = 1;
		memcpy(pRec, &rec, sizeof(rec));
		ok = 1;
	}
	return ok;
}

int SLAPI BillCore::SetRecFlag(PPID billID, long flag, int set, int use_ta)
{
	int    ok = -1, do_update = 0;
	BillTbl::Rec rec;
	if(flag) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(this, PPOBJ_BILL, billID, &rec) > 0); // @v8.1.4 SearchByID_ForUpdate-->SearchByID
		if(set && !(rec.Flags & flag)) {
			rec.Flags |= flag;
			do_update = 1;
		}
		else if(!set && (rec.Flags & flag)) {
			rec.Flags &= ~flag;
			do_update = 1;
		}
		if(do_update) {
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(updateRecBuf(&rec)); // @sfu
			if(flag == BILLF_WRITEDOFF && IsDraftOp(rec.OpID)) {
				const PPID acn = set ? PPACN_BILLWROFF : PPACN_BILLWROFFUNDO;
				DS.LogAction(acn, PPOBJ_BILL, billID, 0, 0);
			}
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::SetRecFlag2(PPID billID, long flag2, int set, int use_ta)
{
	int    ok = -1, do_update = 0;
	BillTbl::Rec rec;
	if(flag2) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(this, PPOBJ_BILL, billID, &rec) > 0);
		if(set && !(rec.Flags2 & flag2)) {
			rec.Flags2 |= flag2;
			do_update = 1;
		}
		else if(!set && (rec.Flags2 & flag2)) {
			rec.Flags2 &= ~flag2;
			do_update = 1;
		}
		if(do_update) {
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(updateRecBuf(&rec)); // @sfu
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::PutExtraData(PPID id, const PPBillExt * pExt, int use_ta)
{
	int    ok = 1;
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pExt && !pExt->IsEmpty()) {
			BillCore::Extra_Strg rec;
			MEMSZERO(rec);
			rec.AgentID = pExt->AgentID;
			rec.PayerID = pExt->PayerID;
			STRNSCPY(rec.InvoiceCode, pExt->InvoiceCode);
			rec.InvoiceDate = pExt->InvoiceDate;
			STRNSCPY(rec.PaymBillCode, pExt->PaymBillCode);
			rec.PaymBillDate = pExt->PaymBillDate;
			rec.ExtPriceQuotKindID = pExt->ExtPriceQuotKindID;
			THROW(p_ref->PutProp(PPOBJ_BILL, id, BILLPRP_EXTRA, &rec, sizeof(rec)));
			THROW(SetRecFlag(id, BILLF_EXTRA, 1, 0));
		}
		else {
			THROW(p_ref->RemoveProperty(PPOBJ_BILL, id, BILLPRP_EXTRA, 0));
			THROW(SetRecFlag(id, BILLF_EXTRA, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::GetExtraData(PPID id, PPBillExt * pExt)
{
	BillCore::Extra_Strg exr;
	int    r = PPRef->GetProperty(PPOBJ_BILL, id, BILLPRP_EXTRA, &exr, sizeof(exr));
	if(r > 0) {
		if(pExt) {
			pExt->PayerID = exr.PayerID;
			pExt->AgentID = exr.AgentID;
			STRNSCPY(pExt->InvoiceCode, exr.InvoiceCode);
			pExt->InvoiceDate = exr.InvoiceDate;
			STRNSCPY(pExt->PaymBillCode, exr.PaymBillCode);
			pExt->PaymBillDate = exr.PaymBillDate;
			pExt->ExtPriceQuotKindID = exr.ExtPriceQuotKindID;
		}
	}
	else
		memzero(pExt, sizeof(*pExt));
	return r;
}

/* @v10.1.12 (inlined)
int SLAPI BillCore::GetAdvanceRep(PPID id, PPAdvanceRep ** ppData)
{
	int    ok = 1;
	PPAdvanceRep advrep;
	if(!(*ppData))
		(*ppData) = new PPAdvanceRep;
	if((*ppData) == 0)
		ok = PPSetErrorNoMem();
	else if(PPRef->GetProperty(PPOBJ_BILL, id, BILLPRP_ADVANCEREP, &advrep, sizeof(advrep)) > 0)
		memcpy(*ppData, &advrep, sizeof(advrep));
	else {
		memzero(*ppData, sizeof(PPAdvanceRep));
		ok = -1;
	}
	return ok;
}*/

int SLAPI BillCore::Extract(PPID id, PPBill * pInfo)
{
	int    ok = 1;
	int    r = 1;
	Reference * p_ref = PPRef;
	pInfo->BaseDestroy();
	THROW(Search(id, &pInfo->Rec) > 0);
	BillCore::GetCode(pInfo->Rec.Code);
	if(pInfo->Rec.Flags & BILLF_EXTRA)
		THROW(GetExtraData(id, &pInfo->Ext));
	if(pInfo->Rec.Flags & BILLF_RENT) {
		THROW(r = GetRentCondition(id, &pInfo->Rent));
		if(r < 0)
			pInfo->Rec.Flags &= ~BILLF_RENT;
	}
	if(pInfo->Rec.Flags & BILLF_BANKING) {
		PPBankingOrder paym_order;
		MEMSZERO(paym_order);
		if(p_ref->GetProperty(PPOBJ_BILL, id, BILLPRP_PAYMORDER, &paym_order, sizeof(paym_order)) > 0) {
			THROW_MEM(pInfo->P_PaymOrder = new PPBankingOrder);
			memcpy(pInfo->P_PaymOrder, &paym_order, sizeof(paym_order));
		}
		else
			pInfo->Rec.Flags &= ~BILLF_BANKING;
	}
	if(pInfo->Rec.Flags & BILLF_FREIGHT) {
		PPFreight freight;
		if(GetFreight(id, &freight) > 0) {
			THROW(pInfo->SetFreight(&freight));
		}
		else
			THROW(pInfo->SetFreight(0));
	}
	if(pInfo->Rec.Flags & BILLF_ADVANCEREP) {
		//THROW(GetAdvanceRep(id, &pInfo->P_AdvRep));
		PPAdvanceRep advrep;
		THROW_MEM(SETIFZ(pInfo->P_AdvRep, new PPAdvanceRep));
		if(p_ref->GetProperty(PPOBJ_BILL, id, BILLPRP_ADVANCEREP, &advrep, sizeof(advrep)) > 0) {
			memcpy(pInfo->P_AdvRep, &advrep, sizeof(advrep));
		}
		else
			memzero(pInfo->P_AdvRep, sizeof(*pInfo->P_AdvRep));
	}
	if(GetOpType(pInfo->Rec.OpID) == PPOPT_AGREEMENT) {
		PPBill::Agreement agt;
		if(p_ref->GetProperty(PPOBJ_BILL, id, BILLPRP_AGREEMENT, &agt, sizeof(agt)) > 0) {
			THROW_MEM(SETIFZ(pInfo->P_Agt, new PPBill::Agreement(agt)));
		}
		else
			ZDELETE(pInfo->P_Agt);
	}
	THROW(GetAmountList(id, &pInfo->Amounts));
	THROW(GetPayPlan(id, &pInfo->Pays));
	THROW(r);
	CATCHZOK
	return ok;
}

int SLAPI BillCore::Remove(PPID id, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(UpdateAmount(id, 0, 0)); // Удаляем все суммы для документа id
		THROW(PutPayPlan(id, 0, 0));
		THROW(PPRef->RemoveProperty(PPOBJ_BILL, id, 0, 0));
		THROW_DB(deleteFrom(this, 0, (this->ID == id)));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

void SLAPI BillCore::_PrepareWriting(const BillTbl::Rec * pRec)
{
	copyBufFrom(pRec);
	BillCore::SetCode(data.Code, data.Flags);
	data.Flags &= ~BILLF_NOLOADTRFR;
	data.Flags2 &= ~BILLF2_DONTCLOSDRAFT;
}

int SLAPI BillCore::EditRec(PPID * pID, BillTbl::Rec * pRec, int use_ta)
{
	int    ok = 1, is_new = (*pID == 0);
	uint   i;
	long   billno;
	THROW_PP(pRec->LocID != 0, PPERR_ZEROBILLLOC);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec->BillNo == 0) {
			THROW(_GetBillNo(pRec->Dt, &billno));
			pRec->BillNo = billno;
		}
		if(!is_new) {
			BillTbl::Rec org_rec;
			THROW(SearchByID_ForUpdate(this, PPOBJ_BILL, *pID, &org_rec) > 0);
			pRec->ID = *pID;
			_PrepareWriting(pRec);
			THROW_DB(updateRec()); // @sfu
			if(pRec->SCardID != org_rec.SCardID)
				DS.LogAction(PPACN_SCARDBINDUPD, PPOBJ_BILL, *pID, org_rec.SCardID, 0);
			if(pRec->Object != org_rec.Object)
				DS.LogAction(PPACN_UPDBILLOBJECT, PPOBJ_BILL, *pID, org_rec.Object, 0);
		}
		else {
			int    r = 0;
			pRec->ID = 0;
			THROW(ok = AdjustNewObjID(this, PPOBJ_BILL, pRec));
			_PrepareWriting(pRec);
			//
			// Если при добавлении записи вываливается ошибка BE_DUP, то это может означать то, что другой пользователь
			// успел впихнуть запись с нашим номером BillNo. Несколько раз повторим, увеличивая значение счетчика.
			//
			for(i = 0; (r = insertRec(0, pID)) == 0 && BtrError == BE_DUP && i < 200; i++) {
				pRec->BillNo++;
				pRec->ID = 0;
				THROW(ok = AdjustNewObjID(this, PPOBJ_BILL, pRec));
				_PrepareWriting(pRec);
			}
			THROW_DB(r);
		}
		THROW(tra.Commit());
	}
	if(!is_new) {
		PPObjBill * p_bobj = BillObj;
		CALLPTRMEMB(p_bobj, Dirty(*pID));
	}
	CATCHZOK
	if(ok == 2 && CConfig.Flags & CCFLG_DEBUG) {
		SString msg_buf, fmt_buf, obj_title;
		GetObjectTitle(PPOBJ_BILL, obj_title);
		msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ADDOBJREC_JUMPED_ID, fmt_buf), obj_title.cptr());
		PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	}
	return ok;
}

int SLAPI BillCore::PutBillProp(PPID billID, PPID propID, long isThere, void * pBuf, size_t bufSize)
{
	Reference * p_ref = PPRef;
	return isThere ? p_ref->PutProp(PPOBJ_BILL, billID, propID, pBuf, bufSize) : p_ref->RemoveProperty(PPOBJ_BILL, billID, propID, 0);
}

int SLAPI BillCore::Edit(PPID * pID, PPBill * pInfo, int use_ta)
{
	int    ok = 1;
	const  int is_new = BIN(*pID == 0);
	uint   i;
	AmtEntry * p_ae;
	PropertyTbl::Rec prp_rec;
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(use_ta);
		THROW(tra);
		SETFLAG(pInfo->Rec.Flags, BILLF_EXTRA, !pInfo->Ext.IsEmpty());
		SETFLAG(pInfo->Rec.Flags, BILLF_RENT,  !pInfo->Rent.IsEmpty());
		SETFLAG(pInfo->Rec.Flags, BILLF_BANKING, pInfo->P_PaymOrder);
		SETFLAG(pInfo->Rec.Flags, BILLF_FREIGHT, pInfo->P_Freight);
		THROW(EditRec(pID, &pInfo->Rec, 0));
		THROW(PutExtraData(*pID, ((pInfo->Rec.Flags & BILLF_EXTRA) ? &pInfo->Ext : 0), 0));
		if(pInfo->Rec.Flags & BILLF_RENT) {
			MEMSZERO(prp_rec);
			memcpy(&prp_rec.Text, &pInfo->Rent, sizeof(pInfo->Rent));
			THROW(p_ref->PutProp(PPOBJ_BILL, *pID, BILLPRP_RENT, &prp_rec));
			pInfo->Amounts.Put(PPAMT_RENTPRD, pInfo->Rec.CurID, pInfo->Rent.PartAmount, 1, 1);
		}
		else {
			THROW(p_ref->RemoveProperty(PPOBJ_BILL, *pID, BILLPRP_RENT, 0));
			pInfo->Amounts.Remove(PPAMT_RENTPRD, -1);
		}
		THROW(PutBillProp(*pID, BILLPRP_PAYMORDER, pInfo->Rec.Flags & BILLF_BANKING, pInfo->P_PaymOrder, sizeof(PPBankingOrder)));
		THROW(PutBillProp(*pID, BILLPRP_FREIGHT, pInfo->Rec.Flags & BILLF_FREIGHT, pInfo->P_Freight, sizeof(PPFreight)));
		THROW(PutBillProp(*pID, BILLPRP_ADVANCEREP, pInfo->Rec.Flags & BILLF_ADVANCEREP, pInfo->P_AdvRep, sizeof(PPAdvanceRep)));
		THROW(PutBillProp(*pID, BILLPRP_AGREEMENT, GetOpType(pInfo->Rec.OpID) == PPOPT_AGREEMENT, pInfo->P_Agt, sizeof(PPBill::Agreement))); // @v10.1.12
		if(is_new) {
			{
				//
				// Теоретически, для нового документа в БД не должно существовать
				// записей сумм, но практически такое изредка случается. Поэтому
				// удаляем все суммы для идентификатора *pID (если там ничего нет,
				// то много времени эта операция не займет).
				//
				THROW(UpdateAmount(*pID, 0, 0));
			}
			{
				BExtInsert bei(&AmtT);
				for(i = 0; pInfo->Amounts.enumItems(&i, (void**)&p_ae);) {
					BillAmountTbl::Rec amt_rec;
					MEMSZERO(amt_rec);
					amt_rec.BillID    = *pID;
					amt_rec.AmtTypeID = p_ae->AmtTypeID;
					amt_rec.CurID     = p_ae->CurID;
					amt_rec.Amount    = p_ae->Amt;
					THROW_DB(bei.insert(&amt_rec));
				}
				THROW_DB(bei.flash());
			}
		}
		else {
			//
			// Пришлось уйти от механизма ВСЕ-УДАЛИ-И-СНОВА-ВСТАВЬ из-за
			// возникновения проблем с блокировками.
			// Теперь алгоритм такой:
			// 1. извлекаем все существующие суммы
			// 2. для каждой из существующих сумм:
			//    2.1 если такой суммы в новом списке нет, то удаляем ее
			//    2.2 если такая сумма есть и ее величина не равна существующей, то изменяем
			//    2.3 в противном случае - ничего не делаем.
			// 3. Наконец, вставляет в БД те записи из нового списка, которые не были просмотрены
			//    на этапе 2 (то есть, их точно не было в БД).
			//
			// Ожидаемый выигрыш связан с уменьшением количества изменяемых (следовательно, блокируемых)
			// записей.
			//
			AmtList temp_list;
			LongArray pos_list;
			GetAmountList(*pID, &temp_list);
			for(i = 0; i < temp_list.getCount(); i++) {
				const AmtEntry & r_temp_entry = temp_list.at(i);
				uint   pos = 0;
				if(pInfo->Amounts.Search(r_temp_entry.AmtTypeID, r_temp_entry.CurID, &pos)) {
					const AmtEntry & r_new_entry = pInfo->Amounts.at(pos);
					if(r_temp_entry.Amt != r_new_entry.Amt) {
						THROW(UpdateAmount(*pID, &r_new_entry, 0));
					}
					pos_list.addUnique((long)pos);
				}
				else {
					THROW_DB(deleteFrom(&AmtT, 0, (AmtT.BillID == *pID &&
						AmtT.AmtTypeID == r_temp_entry.AmtTypeID && AmtT.CurID == r_temp_entry.CurID)));
				}
			}
			if(pos_list.getCount() < pInfo->Amounts.getCount()) {
				for(i = 0; i < pInfo->Amounts.getCount(); i++) {
					if(!pos_list.lsearch((long)i)) {
						const AmtEntry & r_new_entry = pInfo->Amounts.at(i);
						AmtT.data.BillID    = *pID;
						AmtT.data.AmtTypeID = r_new_entry.AmtTypeID;
						AmtT.data.CurID     = r_new_entry.CurID;
						AmtT.data.Amount    = r_new_entry.Amt;
						THROW_DB(AmtT.insertRec());
					}
				}
			}
		}
		THROW(PutPayPlan(*pID, &pInfo->Pays, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// @v8.5.8 Новый вариант функции, включающий изменени поля BillTbl::Rec::PaymAmount
//
int SLAPI BillCore::UpdatePaymAmount(PPID billID, PPID curID, double newAmt, double orgAmt)
{
	int    ok = 1;
	double org_paym = 0.0;
	BillTbl::Rec rec;
	DBRowId _dbpos;
	THROW(SearchByID(this, PPOBJ_BILL, billID, &rec) > 0);
	THROW_DB(getPosition(&_dbpos));
	THROW(GetAmount(billID, PPAMT_PAYMENT, curID, &org_paym));
	{
		const  double amt = R2(org_paym + newAmt - R2(orgAmt));
		const  double paym = BR2(rec.Amount);
		const  int is_neg = BIN(rec.Amount < 0.0); // @v10.3.3
		//
		// Следующая проверка дублирует аналогичную в функции UpdateAmount,
		// однако, мы ее делаем с тем, чтобы избежать сообщений о превышении
		// оплаты над суммой документа в том случае, если ничего не изменилось.
		//
		if(amt != org_paym) {
			AmtEntry ae(PPAMT_PAYMENT, curID, amt);
			if(!(LConfig.Flags & CFGFLG_ALLOWOVERPAY)) {
				// @v10.3.3 Инверсия знака при оплате отрицательной суммы
				if(is_neg) {
					THROW_PP(R2(paym - amt) <= 0, PPERR_EXTRAPAYM);
				}
				else {
					THROW_PP(R2(paym - amt) >= 0, PPERR_EXTRAPAYM);
				}
			}
			THROW(UpdateAmount(billID, &ae, 0));
		}
		{
			const int payout_flag_state = BIN(R2(paym - amt) <= 0.0);
			const int amt_updated = (curID == rec.CurID) ? (amt != rec.PaymAmount) : 0;
			if(amt_updated || payout_flag_state != BIN(rec.Flags & BILLF_PAYOUT)) {
				BillTbl::Key0 k0;
				THROW_DB(getDirectForUpdate(0, &k0, _dbpos));
				SETFLAG(rec.Flags, BILLF_PAYOUT, payout_flag_state);
				if(amt_updated)
					rec.PaymAmount = amt;
				THROW_DB(updateRecBuf(&rec)); // @sfu
			}
		}
	}
	CATCHZOK
	return ok;
}

#if 0 // @v8.5.8 Этот вариант заменен тем, что выше {
int SLAPI BillCore::UpdatePaymAmount(PPID billID, PPID curID, double newAmt, double orgAmt)
{
	int    ok = 1;
	double org_paym = 0.0;
	THROW(GetAmount(billID, PPAMT_PAYMENT, curID, &org_paym));
	{
		double amt = R2(org_paym + newAmt - R2(orgAmt));
		double paym = BR2(data.Amount);
		//
		// Следующая проверка дублирует аналогичную в функции UpdateAmount,
		// однако, мы ее делаем с тем, чтобы избежать сообщений о превышении
		// оплаты над суммой документа в том случае, если ничего не изменилось.
		//
		if(amt != org_paym) {
			AmtEntry ae(PPAMT_PAYMENT, curID, amt);
			if(!(LConfig.Flags & CFGFLG_ALLOWOVERPAY))
				THROW_PP(R2(paym - amt) >= 0, PPERR_EXTRAPAYM);
			THROW(UpdateAmount(billID, &ae, 0));
		}
		THROW(SetRecFlag(billID, BILLF_PAYOUT, BIN(R2(paym - amt) <= 0.0), 0));
	}
	CATCHZOK
	return ok;
}
#endif // } 0

int SLAPI BillCore::BreakOffLink(PPID linkBillID, PPID baseBillID, int linkKind, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec bill_rec, base_bill_rec;
	if(oneof2(linkKind, 1, 3)) {
		if(Search(linkBillID, &bill_rec) > 0 && bill_rec.LinkBillID == baseBillID) {
			PPTransaction tra(use_ta);
			THROW(tra);
			bill_rec.LinkBillID = 0;
			THROW(EditRec(&linkBillID, &bill_rec, 0));
			if(linkKind == 3 && Search(baseBillID, &base_bill_rec) > 0) {
				THROW(SetRecFlag(baseBillID, BILLF_WRITEDOFF, 0, 0));
			}
			THROW(tra.Commit());
			ok = 1;
		}
	}
	else if(linkKind == 2) {
		THROW(RemoveFromPool(linkBillID, PPASS_PAYMBILLPOOL, baseBillID, use_ta));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::_Enum(int idx, PPID id, DateIter * pDI, void * b)
{
	int    ok = 1;
	struct {
		PPID  id;
		LDATE date;
		long  n;
	} k;
	k.id = id;
	if(pDI) {
		k.date = pDI->dt;
		k.n    = pDI->oprno;
	}
	else {
		k.date = ZERODATE;
		k.n    = 0;
	}
	if(search(idx, &k, spGt) && k.id == id && (!pDI || pDI->Advance(k.date, k.n) > 0)) {
		copyBufTo(b);
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI BillCore::GetListOfOrdersByLading(PPID billID, PPIDArray * pOrderBillList)
{
	int    ok = -1;
	DateIter diter;
	BillTbl::Rec sh_bill_rec, ord_bill_rec;
	while(EnumLinks(billID, &diter, BLNK_SHADOW, &sh_bill_rec) > 0) {
		if(Search(sh_bill_rec.Object, &ord_bill_rec) > 0) {
			pOrderBillList->addUnique(ord_bill_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI BillCore::GetListOfActualAgreemts(PPID arID, LDATE dt, int maxDays, int maxItems, PPIDArray & rList)
{
	rList.clear();
    int    ok = -1;
	if(arID) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		if(ar_obj.Fetch(arID, &ar_rec) > 0) {
			PPIDArray op_list;
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(PPOPT_AGREEMENT, &op_id, &op_rec) > 0;) {
				if(op_rec.AccSheetID == ar_rec.AccSheetID) {
					op_list.add(op_rec.ID);
				}
			}
			if(op_list.getCount()) {
				op_list.sortAndUndup();
				BillTbl::Key3 k3;
				MEMSZERO(k3);
				k3.Object = arID;
				k3.Dt = dt;
				if(search(3, &k3, spLe) && data.Object == arID) do {
					if(!(data.Flags2 & BILLF2_DECLINED) && op_list.bsearch(data.OpID)) {
						rList.add(data.ID);
						ok = 1;
						if(maxItems > 0 && (int)rList.getCount() >= maxItems)
							break;
					}
					else if(dt && maxDays > 0 && diffdate(dt, data.Dt) > maxDays)
						break;
				} while(search(3, &k3, spPrev) && data.Object == arID);
			}
		}
	}
    return ok;
}

int SLAPI BillCore::EnumLinks(PPID id, DateIter * diter, uint blnk, BillTbl::Rec * pRec)
{
	int    r;
	while((r = _Enum(4, id, diter, pRec)) > 0) {
		PPOprKind op_rec;
		BillTbl::Rec bill_rec;
		if(blnk == BLNK_ALL)                                    break;
		if(blnk & BLNK_SHADOW && data.OpID == 0)                break;
		const PPID op_type = GetOpType(data.OpID, &op_rec);
		if(blnk & BLNK_PAYMENT && op_type == PPOPT_PAYMENT)     break;
		if(blnk & BLNK_RETURN  && op_type == PPOPT_GOODSRETURN) break;
		if(blnk & BLNK_CHARGE  && op_type == PPOPT_CHARGE)      break;
		if(blnk & BLNK_ACK     && op_type == PPOPT_GOODSACK)    break;
		if(blnk & BLNK_CORRECTION && op_type == PPOPT_CORRECTION)  break;
		if(blnk & BLNK_CHARGEPAYM && op_type == PPOPT_CHARGE && op_rec.Flags & OPKF_CHARGENEGPAYM) break;
		if(blnk & BLNK_EDIRECADV && data.OpID == PPOPK_EDI_RECADV) break;
		if(blnk & BLNK_WROFFDRAFT) {
			if(BillObj->Fetch(id, &bill_rec) > 0 && IsDraftOp(bill_rec.OpID))
				break;
		}
	}
	return r;
}

int SLAPI BillCore::EnumByObj(PPID obj, DateIter * diter, void * b)
	{ return _Enum(3, obj, diter, b); }
int SLAPI BillCore::EnumByOpr(PPID opr, DateIter * diter, void * b)
	{ return _Enum(2, opr, diter, b); }

int SLAPI BillCore::InitEnumByOp(PPID opID, DateRange * pPeriod, int flags, long * pHandle)
{
	int    idx = 2;
	BExtQuery * q = new BExtQuery(this, idx);
	q->select(this->ID, this->Code, this->Dt, this->BillNo, this->DueDate, this->OpID, this->StatusID, this->UserID,
		this->MainOrgID, this->LocID, this->Object, this->Object2, this->CurID, this->CRate,
		this->Amount, this->LinkBillID, this->Flags, this->Flags2, this->SCardID, 0L);
	q->where(this->OpID == opID && daterange(this->Dt, pPeriod));
	BillTbl::Key2 k2;
	MEMSZERO(k2);
	k2.OpID = opID;
	k2.Dt = pPeriod ? pPeriod->low : ZERODATE;
	q->initIteration(0, &k2, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

int SLAPI BillCore::NextEnum(long enumHandle, BillTbl::Rec * pRec)
	{ return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1; }
int SLAPI BillCore::DestroyIter(long enumHandle)
	{ return EnumList.DestroyIterHandler(enumHandle); }

SEnumImp * SLAPI BillCore::EnumByOp(PPID opID, DateRange * pPeriod, int options)
{
	long   h = -1;
	return InitEnumByOp(opID, pPeriod, options, &h) ? new PPTblEnum <BillCore>(this, h) : 0;
}

int SLAPI BillCore::GetTrnovrBySCard(PPID cardID, const DateRange * pPeriod, PPID restrGoodsGrpID, double * pDbt, double * pCrd)
{
	double dbt = 0.0, crd = 0.0;
	DateIter diter(pPeriod);
	BillTbl::Rec rec;
	while(_Enum(6, cardID, &diter, &rec) > 0) {
		PPOprKind op_rec;
		PPID op_type_id = GetOpType(rec.OpID, &op_rec);
		if(op_type_id == PPOPT_GOODSEXPEND)
			crd += BR2(rec.Amount);
		else if(op_type_id == PPOPT_GOODSRETURN && GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND)
			dbt += BR2(rec.Amount);
	}
	if(pDbt)
		*pDbt += dbt;
	if(pCrd)
		*pCrd += crd;
	return 1;
}

int SLAPI BillCore::CreateSCardsTurnoverList(const DateRange * pPeriod, RAssocArray * pList)
{
	int    ok = 1;
	IterCounter cntr;
	PPOprKind op_rec;
	BillTbl::Key6 k, k_;
	BExtQuery q(this, 6, 64);
	q.select(this->ID, this->OpID, this->Amount, this->SCardID, 0L).where(this->SCardID > 0L && daterange(this->Dt, pPeriod));
	MEMSZERO(k);
	k.SCardID = 1L;
	k.Dt = pPeriod->low;
	k_ = k;
	cntr.Init(q.countIterations(0, &k_, spGe));
	for(q.initIteration(0, &k, spGe); ok && q.nextIteration() > 0; PPWaitPercent(cntr.Increment())) {
		PPID op_type_id = GetOpType(data.OpID, &op_rec);
		if(op_type_id == PPOPT_GOODSEXPEND) {
			if(!pList->Add(data.SCardID, BR2(data.Amount), 1, 0))
				ok = 0;
		}
		else if(op_type_id == PPOPT_GOODSRETURN) {
			if(GetOpType(op_rec.LinkOpID) == PPOPT_GOODSEXPEND) {
				if(!pList->Add(data.SCardID, -BR2(data.Amount), 1, 0))
					ok = 0;
			}
		}
	}
	return ok;
}

int SLAPI BillCore::EnumByDate(DateIter * pDI, BillTbl::Rec * pRec)
{
	int    ok = 1;
	BillTbl::Key1 k;
	k.Dt     = pDI ? pDI->dt : ZERODATE;
	k.BillNo = pDI ? pDI->oprno : 0;
	if(search(1, &k, spGt) && (!pDI || pDI->Advance(k.Dt, k.BillNo) > 0))
		copyBufTo(pRec);
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI BillCore::SearchAnalog(const BillTbl::Rec * pSample, long flags, PPID * pID, BillTbl::Rec * pRec)
{
	int    ok = -1;
	char   scode[32];
	DBQ  * dbq = 0;
	union {
		BillTbl::Key1 k1;
		BillTbl::Key2 k2;
		BillTbl::Key3 k3;
	} k;
	int    idx = 2;
	STRNSCPY(scode, pSample->Code);
	GetCode(scode);
	MEMSZERO(k);
	if(pSample->Object) {
		k.k3.Object = pSample->Object;
		k.k3.Dt     = pSample->Dt;
		dbq = & (this->Object == pSample->Object && this->Dt == pSample->Dt);
		if(!(flags & safIgnoreOp))
			dbq = &(*dbq && this->OpID == pSample->OpID);
		idx = 3;
	}
	else if(!(flags & safIgnoreOp)) {
		k.k2.OpID = pSample->OpID;
		k.k2.Dt   = pSample->Dt;
		dbq = & (this->OpID == pSample->OpID && this->Dt == pSample->Dt);
		idx = 2;
	}
	else {
		k.k1.Dt = pSample->Dt;
		dbq = & (this->Dt == pSample->Dt);
		idx = 1;
	}
	BExtQuery q(this, idx, 256);
	q.select(this->ID, this->Dt, this->Code, this->EdiOp, 0L).where(*dbq);
	for(q.initIteration(0, &k, spGe); ok < 0 && q.nextIteration() > 0;) {
		if(!(flags & safCheckEdiOp) || data.EdiOp == pSample->EdiOp) {
			GetCode(data.Code);
			if(*strip(data.Code) && stricmp866(data.Code, scode) == 0) {
				const PPID bill_id = data.ID;
				if(!pRec || Search(bill_id, pRec) > 0) {
					ASSIGN_PTR(pID, bill_id);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI BillCore::GetCreditList(PPID id, PctChargeArray * pList)
{
	int    r = 1;
	PPIDArray id_list;
	BillTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		double paym = 0.0;
		pList->Add(rec.Dt, BR2(rec.Amount));
		if(GetAmount(id, PPAMT_FIXPAYMENT, 0, &paym) > 0)
			pList->Add(rec.Dt, -paym);
		for(DateIter di; (r = EnumLinks(id, &di, BLNK_PAYMRETN | BLNK_CHARGEPAYM)) > 0;) {
			id_list.add(data.ID);
			double p = -BR2(data.Amount);
			if(CheckOpFlags(data.OpID, OPKF_CHARGENEGPAYM))
				p = -p;
			pList->Add(data.Dt, p);
		}
		for(PPID reckon_id = 0; EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &reckon_id) > 0;)
			if(!id_list.lsearch(reckon_id) && Search(reckon_id, &rec) > 0)
				pList->Add(rec.Dt, -BR2(rec.Amount));
	}
	return r ? 1 : 0;
}

int SLAPI BillCore::CalcPayment(PPID id, int byLinks, const DateRange * pPeriod, PPID curID, double * pPaym)
{
	if(byLinks) {
		if(pPeriod) {
			PPCycleArray temp_sieve(pPeriod->low, pPeriod->upp, 0, 0);
			return Helper_CalcPayment(id, curID, &temp_sieve, 0, pPaym);
		}
		else
			return Helper_CalcPayment(id, curID, 0, 0, pPaym);
	}
	else {
		double paym = 0.0;
		int    r = GetAmount(id, PPAMT_PAYMENT, curID, &paym);
		ASSIGN_PTR(pPaym, R2(paym));
		return r ? 1 : 0;
	}
}

int SLAPI BillCore::CalcPaymentSieve(PPID id, PPID curID, const PPCycleArray * pSieve, RAssocArray * pList, double * pPaym)
{
	return Helper_CalcPayment(id, curID, pSieve, pList, pPaym);
}

int SLAPI BillCore::Helper_CalcPayment(PPID id, PPID curID, const PPCycleArray * pSieve, RAssocArray * pList, double * pPaym)
{
	int    r = 1;
	uint   pos;
	PPObjBillStatus bs_obj;
	PPBillStatus bs_rec;
	double paym = 0.0, p = 0.0;
	PPIDArray id_list;
	GetAmount(id, PPAMT_FIXPAYMENT, curID, &paym);
	DateIter di;
	if(pSieve) {
		DateRange period;
		pSieve->getCycleParams(&period, 0);
		di.Init(&period);
	}
	PROFILE_START
	while((r = EnumLinks(id, &di, BLNK_PAYMRETN|BLNK_CHARGEPAYM)) > 0) {
		if(data.CurID == curID) {
			const int no_paym = BIN(data.StatusID && bs_obj.Fetch(data.StatusID, &bs_rec) > 0 && bs_rec.Flags & BILSTF_LOCK_PAYMENT);
			if(!no_paym) {
				id_list.add(data.ID);
				p = BR2(data.Amount);
				if(CheckOpFlags(data.OpID, OPKF_CHARGENEGPAYM))
					p = -p;
				if(pSieve) {
					if(pSieve->searchDate(data.Dt, &(pos = 0))) {
						CALLPTRMEMB(pList, Add(pos, p, 1, 0));
						paym += p;
					}
				}
				else
					paym += p;
			}
		}
	}
	PROFILE_END
	PROFILE_START
	for(PPID reckon_id = 0; EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &reckon_id) > 0;) {
		if(!id_list.lsearch(reckon_id) && Search(reckon_id) > 0 && data.CurID == curID) {
			int    no_paym = 0;
			if(data.StatusID && bs_obj.Fetch(data.StatusID, &bs_rec) > 0 && bs_rec.Flags & BILSTF_LOCK_PAYMENT)
				no_paym = 1;
			if(!no_paym) {
				p = BR2(data.Amount);
				if(pSieve) {
					if(pSieve->searchDate(data.Dt, &(pos = 0))) {
						CALLPTRMEMB(pList, Add(pos, p, 1, 0));
						paym += p;
					}
				}
				else
					paym += p;
			}
		}
	}
	PROFILE_END
	ASSIGN_PTR(pPaym, R2(paym));
	return r ? 1 : 0;
}

int SLAPI BillCore::GetFirstDate(PPID opID, LDATE * pDate)
{
	int    r = 1;
	LDATE  dt = MAXDATE;
	BillTbl::Rec rec;
	if(opID) {
		PPIDArray op_list;
		if(GetGenericOpList(opID, &op_list) <= 0)
			op_list.add(opID);
		for(uint i = 0; r && i < op_list.getCount(); i++)
			if((r = EnumByOpr(op_list.at(i), 0, &rec)) > 0)
				SETMIN(dt, rec.Dt);
	}
	else {
		DateIter di;
		while((r = EnumByDate(&di, &rec)) > 0 && !rec.Dt)
			dt = rec.Dt;
	}
	ASSIGN_PTR(pDate, dt);
	return r ? (dt ? 1 : (PPSetError(PPERR_NOBILLS), -1)) : 0;
}

int SLAPI BillCore::GetLastDate(PPID opID, LDATE * pDt)
{
	int    r = 1;
	LDATE  dt = ZERODATE;
	if(opID) {
		PPIDArray op_list;
		if(GetGenericOpList(opID, &op_list) <= 0)
			op_list.add(opID);
		for(uint i = 0; r && i < op_list.getCount(); i++) {
			BillTbl::Key2 k2;
			k2.OpID = op_list.at(i);
			k2.Dt   = MAXDATE;
			k2.BillNo = MAXLONG;
			if(search(2, &k2, spLe) && k2.OpID == op_list.at(i))
				SETMAX(dt, data.Dt);
		}
	}
	else {
		BillTbl::Key1 k1;
		k1.Dt = MAXDATE;
		k1.BillNo = MAXLONG;
		if(search(1, &k1, spLe))
			dt = data.Dt;
	}
	ASSIGN_PTR(pDt, dt);
	return r ? ((dt < MAXLONG) ? 1 : (PPSetError(PPERR_NOBILLS), -1)) : 0;
}

int SLAPI BillCore::IsThereBill(PPID locID, const PPIDArray * pOpList, LDATE dt)
{
	BillTbl::Key5 k5;
	BExtQuery q(this, 5);
	q.select(this->ID, 0L).where(this->LocID == locID && this->Dt == dt && ppidlist(this->OpID, pOpList));
	MEMSZERO(k5);
	k5.LocID = locID;
	k5.Dt    = dt;
	q.initIteration(0, &k5, spGe);
	return (q.nextIteration() > 0) ? 1 : -1;
}

int SLAPI BillCore::ScanHolidays(PPID locID, PPID opID, const DateRange * pPeriod, PPHolidays * pHld)
{
	int    ok = 1;
	if(locID == 0) {
		PPObjLocation loc_obj;
		PPIDArray wh_list;
		loc_obj.GetWarehouseList(&wh_list);
		for(uint i = 0; i < wh_list.getCount(); i++) {
			const PPID loc_id = wh_list.at(i);
			if(loc_id)
				THROW(ScanHolidays(loc_id, opID, pPeriod, pHld)); // @recursion
		}
	}
	else {
		PPIDArray days, weekdays, op_list;
		LDATE  dt, end, last_bill_date = ZERODATE;
		if(pPeriod && pPeriod->low)
			dt = pPeriod->low;
		else
			THROW(GetFirstDate(opID, &dt) > 0);
		THROW(GetLastDate(opID, &last_bill_date) > 0);
		end = (pPeriod && pPeriod->upp) ? pPeriod->upp : last_bill_date;
		if(opID && GetGenericOpList(opID, &op_list) <= 0)
			op_list.add(opID);
		for(; dt <= end; dt = plusdate(dt, 1)) {
			ObjIdListFilt loc_list;
			loc_list.Add(locID);
			if(pHld->IsHoliday(&loc_list, dt)) {
				THROW(pHld->SetHoliday(locID, dt, 0));
			}
			else if(dt > last_bill_date || IsThereBill(locID, &op_list, dt) < 0) {
				THROW(pHld->SetHoliday(locID, dt, 0));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::SetShippedTag(PPID id, int mode)
{
	int    ok = -1;
	BillTbl::Rec rec;
	if(id && (ok = Search(id, &rec)) > 0) {
		const long preserve_flags = rec.Flags;
		if(mode < 0) {
			INVERSEFLAG(rec.Flags, BILLF_SHIPPED);
		}
		else {
			SETFLAG(rec.Flags, BILLF_SHIPPED, mode > 0);
		}
		if(rec.Flags != preserve_flags) { // @v8.5.2
			PPTransaction tra(1);
			THROW(tra);
			THROW(EditRec(&id, &rec, 0));
			DS.LogAction(PPACN_UPDBILLFREIGHT, PPOBJ_BILL, id, 1, 0);
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::SetWLabel(PPID id, int mode, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(id && (ok = Search(id, &rec)) > 0) {
			const long org_flags = rec.Flags;
			if(mode < 0) {
				INVERSEFLAG(rec.Flags, BILLF_WHITELABEL);
			}
			else {
				SETFLAG(rec.Flags, BILLF_WHITELABEL, mode > 0);
			}
			if(rec.Flags != org_flags) {
				int    wl_on = (rec.Flags & BILLF_WHITELABEL) ? 1 : 0;
				THROW(EditRec(&id, &rec, 0));
				DS.LogAction(PPACN_UPDBILLWLABEL, PPOBJ_BILL, id, BIN(org_flags & BILLF_WHITELABEL), 0);
				if(CConfig.Flags & CCFLG_SETWLONLINK) {
					for(DateIter diter; ok && EnumLinks(id, &diter, BLNK_PAYMRETN, &rec) > 0;) {
						THROW(SetWLabel(rec.ID, wl_on, 0)); // @recursion
					}
				}
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::HasWLabel(PPID id)
{
	if(Search(id) > 0)
		if(data.Flags & BILLF_WHITELABEL)
			return 1;
		else if(data.LinkBillID && IsOpPaymOrRetn(data.OpID))
			if(Search(data.LinkBillID) > 0 && data.Flags & BILLF_WHITELABEL)
   				return 1;
	return 0;
}
//
//
//
int SLAPI BillCore::EnumMembersOfPool(PPID poolType, PPID poolOwnerID, PPID * pMemberID, ObjAssocTbl::Rec * b)
{
	return PPRef->Assc.EnumByPrmr(poolType, poolOwnerID, pMemberID, b);
}

int SLAPI BillCore::GetPoolMembersList(PPID poolType, PPID poolOwnerID, PPIDArray * pList)
{
	return PPRef->Assc.GetListByPrmr(poolType, poolOwnerID, pList);
}

int SLAPI BillCore::GetPoolOwnerList(PPID billID, PPID poolType, PPIDArray * pList)
{
	int    ok = -1;
	ObjAssocTbl::Rec assc_rec;
	for(SEnum en = PPRef->Assc.Enum(poolType, billID, 1); en.Next(&assc_rec) > 0;) {
		CALLPTRMEMB(pList, addUnique(assc_rec.PrmrObjID));
		ok = 1;
	}
	return ok;
}

// AHTOXA {
int SLAPI BillCore::GetPoolList(PPID poolType, LAssocArray * pAry)
{
	return PPRef->Assc.GetList(poolType, pAry);
}

int SLAPI BillCore::IsMemberOfPool(PPID billID, PPID poolType, PPID * pPoolOwnerID)
{
	int    yes = 0;
	PPID   owner_id = DEREFPTRORZ(pPoolOwnerID);
	Reference * p_ref = PPRef;
	if(owner_id) {
		if(p_ref->Assc.Search(poolType, owner_id, billID) > 0)
			yes = 1;
	}
	else {
		ObjAssocTbl::Rec assc_rec;
		for(SEnum en = p_ref->Assc.Enum(poolType, billID, 1); !yes && en.Next(&assc_rec) > 0;) {
			owner_id = assc_rec.PrmrObjID;
			yes = 1;
		}
	}
	ASSIGN_PTR(pPoolOwnerID, owner_id);
	return yes;
}

int SLAPI BillCore::UpdatePool(PPID billID, PPID pullType, PPID pullOwnerID, int use_ta)
{
	int    ok = 1;
	PPID   assc_id = 0;
	size_t max_va_sz = sizeof(ObjAssocTbl::Rec) - offsetof(ObjAssocTbl::Rec, /*Name*/Text);
	ObjAssocTbl::Rec assc_rec;
	Reference * p_ref = PPRef;
	if(p_ref->Assc.Search(pullType, pullOwnerID, billID, &assc_rec) > 0) {
		ObjAssocTbl::Rec org_assc_rec = assc_rec;
		assc_id = assc_rec.ID;
		memzero(assc_rec.Text, max_va_sz);
		if(memcmp(&assc_rec, &org_assc_rec, sizeof(assc_rec)) != 0)
			THROW(p_ref->Assc.Update(assc_id, &assc_rec, use_ta));
	}
	else {
		MEMSZERO(assc_rec);
		assc_rec.AsscType  = pullType;
		assc_rec.PrmrObjID = pullOwnerID;
		assc_rec.ScndObjID = billID;
		THROW(p_ref->Assc.SearchFreeNum(pullType, pullOwnerID, &assc_rec.InnerNum, 0));
		THROW(p_ref->Assc.Add(&assc_id, &assc_rec, use_ta));
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::RemoveFromPool(PPID billID, PPID poolType, PPID poolOwnerID, int use_ta)
{
	return PPRef->Assc.Remove(poolType, poolOwnerID, billID, use_ta);
}

int SLAPI BillCore::CalcPoolAmounts(PPID poolType, PPID poolOwnerID, AmtList * pAmounts)
{
	int    ok = 1;
	for(PPID member_id = 0; EnumMembersOfPool(poolType, poolOwnerID, &member_id) > 0;) {
		AmtList list;
		THROW(GetAmountList(member_id, &list));
		list.Remove(PPAMT_PCTDIS, -1L);
		list.Remove(PPAMT_CRATE,  -1L);
		list.Remove(PPAMT_TRANSITCRATE,  -1L);
		THROW(pAmounts->Add(&list));
	}
	CATCHZOK
	return ok;
}

int SLAPI BillCore::GetBillListByExt(PPID agentID, PPID payerID, PPIDArray & rList)
{
	int    ok = -1;
	rList.clear(); // @v8.1.0
	if(payerID || agentID) {
		PropertyTbl::Key0 k;
		PropertyTbl * p_tbl = &PPRef->Prop;
		DBQ * dbq = 0;
		BExtQuery q(p_tbl, 0);
		dbq = &(p_tbl->ObjType == PPOBJ_BILL && p_tbl->Prop == BILLPRP_EXTRA);
		if(agentID)
			dbq	= &(*dbq && p_tbl->Val1 == agentID);
		if(payerID)
			dbq	= &(*dbq && p_tbl->Val2 == payerID);
		q.select(p_tbl->ObjID, p_tbl->Text, p_tbl->Val1, p_tbl->Val2, 0L).where(*dbq);
		k.ObjType = PPOBJ_BILL;
		k.ObjID   = 0;
		k.Prop    = 0;
		for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
			const BillCore::Extra_Strg * p_ext = (BillCore::Extra_Strg*)&p_tbl->data;
			if((!payerID || p_ext->PayerID == payerID) && (!agentID || p_ext->AgentID == agentID)) {
				rList.add(p_tbl->data.ObjID);
				ok = 1;
			}
		}
		rList.sortAndUndup();
	}
	return ok;
}

int SLAPI BillCore::GetAdvanceBillList(PPID objID, PPID exclBillID, DateRange * pPeriod, double * pAdvRest, PPIDArray * pList)
{
	int    ok = 1;
	double rest = 0.0;
	Reference * p_ref = PPRef;
	PPAdvanceRep advrep;
	BillTbl::Key3 k3;
	BExtQuery q(this, 3);
	q.select(this->ID, this->OpID, this->Flags, 0L).where(this->Object == objID && daterange(this->Dt, pPeriod));
	MEMSZERO(k3);
	k3.Object = objID;
	k3.Dt = pPeriod ? pPeriod->low : ZERODATE;
	for(q.initIteration(0, &k3, spGt); q.nextIteration() > 0;)
		if(data.ID != exclBillID && data.Flags & BILLF_ADVANCEREP)
			if(p_ref->GetProperty(PPOBJ_BILL, data.ID, BILLPRP_ADVANCEREP, &advrep, sizeof(advrep)) > 0) {
				rest = advrep.OutRest;
				CALLPTRMEMB(pList, add(data.ID));
			}
	ASSIGN_PTR(pAdvRest, rest);
	return ok;
}

int SLAPI BillCore::GetLocListByOp(PPID opID, PPIDArray & rList)
{
	int    ok = -1;
	rList.clear();
	BillTbl::Key2 k2;
	MEMSZERO(k2);
	k2.OpID = opID;
	BExtQuery q(this, 2);
	q.select(this->LocID, 0).where(this->OpID == opID);
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		const PPID loc_id = data.LocID;
		if(loc_id != 0) {
			if(rList.getCount() > 1024)
				rList.sortAndUndup();
			rList.add(loc_id);
			ok = 1;
		}
	}
	rList.sortAndUndup();
	return ok;
}
