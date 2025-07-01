// OBJVATBK.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025
// @codepage windows-1251
// Книги продаж и покупок
//
#include <pp.h>
#pragma hdrstop
//
// @VATBCfg {
//
VATBCfg::VATBCfg() : Kind(0), AccSheetID(0), Flags(0), AcctgBasis(0), AcctgBasisAtPeriod(0)
{
	Period.Z();
}

VATBCfg & FASTCALL VATBCfg::operator = (const VATBCfg & rS)
{
	List = rS.List;
	Kind = rS.Kind;
	AccSheetID = rS.AccSheetID;
	Flags      = rS.Flags;
	AcctgBasis = rS.AcctgBasis;
	AcctgBasisAtPeriod = rS.AcctgBasisAtPeriod;
	Period     = rS.Period;
	return *this;
}

bool VATBCfg::CheckFlag(PPID opID, long f) const
{
	uint   p = 0;
	return (List.lsearch(&opID, &p, CMPF_LONG) && List.at(p).Flags & f);
}

int VATBCfg::CheckOp(PPID /*opID*/, PPID * /*pAccSheetID*/)
{
	return 1;
}

int VATBCfg::CheckList(PPID * pAccSheetID)
{
	for(uint j = 0; j < List.getCount(); j++) {
		if(!CheckOp(List.at(j).OpID, pAccSheetID))
			return 0;
	}
	return 1;
}

int VATBCfg::Setup()
{
	int    ok = 1;
	PPID   acs_id = 0;
	if(!CheckList(&acs_id)) {
		List.freeAll();
		ok = 0;
	}
	else {
		SETIFZ(AccSheetID, acs_id);
	}
	return ok;
}

int VATBCfg::CheckAccSheet(PPID accSheetID)
{
	int    ok = 1;
	const  PPID  preserve_acs_id = AccSheetID;
	AccSheetID = accSheetID;
	if(!CheckList())
		ok = PPSetError(PPERR_UNMATCHOPSHEET);
	AccSheetID = preserve_acs_id;
	return ok;
}
//
// } @VATBCfg
//
// @PPObjVATBook {
//
static const double VatIndexToRateAssocList[] = { 10.0, 18.0, 20.0, 5.0, 7.0 };

/*static*/double FASTCALL PPObjVATBook::GetVatRate(uint idx)
{
	return (idx < SIZEOFARRAY(VatIndexToRateAssocList)) ? VatIndexToRateAssocList[idx] : 0.0;
}

/*static*/bool FASTCALL PPObjVATBook::IsVatRate(uint idx, double rate)
{
	return (idx < SIZEOFARRAY(VatIndexToRateAssocList)) ? feqeps(rate, VatIndexToRateAssocList[idx], 1E-7) : false;
}

/*static*/int FASTCALL PPObjVATBook::IsValidKind(int kind)
{
	return oneof3(kind, PPVTB_SELL, PPVTB_BUY, PPVTB_SIMPLELEDGER) ? 1 : PPSetError(PPERR_INVVATBOOKKIND);
}

TLP_IMPL(PPObjVATBook, VATBookTbl, P_Tbl);

PPObjVATBook::PPObjVATBook(void * extraPtr) : PPObject(PPOBJ_VATBOOK), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ReadConfig();
}

PPObjVATBook::~PPObjVATBook()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjVATBook::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }
int PPObjVATBook::DeleteObj(PPID) { return P_Tbl->deleteRec() ? 1 : PPSetErrorDB(); }
const char * PPObjVATBook::GetNamePtr() { return P_Tbl->data.Code; }

int PPObjVATBook::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	VATBookTbl::Key1 k1;
	VATBookTbl::Key4 k4;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_ARTICLE) {
			MEMSZERO(k4);
			k4.ArID = _id;
			if(P_Tbl->search(4, &k4, spGt) && k4.ArID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
		if(_obj == PPOBJ_BILL) {
			MEMSZERO(k1);
			k1.LinkBillID = _id;
			if(P_Tbl->search(1, &k1, spGe) && k1.LinkBillID == _id)
				if(P_Tbl->data.Flags & VATBF_FIX)
					ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
				else if(!P_Tbl->deleteRec())
					ok = (PPSetErrorDB(), DBRPL_ERROR);
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_ARTICLE) {
			MEMSZERO(k4);
			k4.ArID = _id;
			// @todo update_for
			while(ok == DBRPL_OK && P_Tbl->searchForUpdate(4, &k4, spGt) && k4.ArID == _id) {
				P_Tbl->data.ArID = reinterpret_cast<long>(extraPtr);
				if(!P_Tbl->updateRec()) // @sfu
					ok = PPSetErrorDB();
			}
		}
	}
	return ok;
}

static const SVerT __VATBookFormatVer(7, 6, 1);

/*static*/int PPObjVATBook::ReadCfgList(PPID kind, VATBCfg * pConfig)
{
	int    r = 1, i;
	uint   sz = sizeof(PPVATBConfig);
	PPID   prop;
	PPVATBConfig * p_cfg = 0;
	pConfig->AcctgBasis = INCM_DEFAULT;
	switch(kind) {
		case PPVTB_BUY:
			prop = PPPRP_VATBBCFG;
			pConfig->AccSheetID = GetSupplAccSheet();
			break;
		case PPVTB_SELL:
			prop = PPPRP_VATBSCFG;
			pConfig->AccSheetID = GetSellAccSheet();
			break;
		case PPVTB_SIMPLELEDGER:
			prop = PPPRP_SMPLLEDGCFG;
			pConfig->AccSheetID = GetSellAccSheet();
			break;
		default:
			CALLEXCEPT_PP(PPERR_INVPARAM);
	}
	pConfig->Kind = kind;
	pConfig->List.freeAll();
	{
		uint   delta = 2;
		uint   delta_pre7602 = (sizeof(VATBCfg::Item) * delta) / sizeof(ObjRestrictItem);
		do {
			if(p_cfg) {
				if(p_cfg->Ver.IsLt(7, 6, 1)) {
					if(p_cfg->ListCount > (long)delta_pre7602) {
						sz += sizeof(ObjRestrictItem) * ((uint)p_cfg->ListCount - delta_pre7602);
						delta_pre7602 = (uint)p_cfg->ListCount;
					}
					else
						break;
				}
				else if(p_cfg->ListCount > (long)delta) {
					sz += sizeof(VATBCfg::Item) * ((uint)p_cfg->ListCount - delta);
					delta = (uint)p_cfg->ListCount;
				}
				else
					break;
			}
			else
				sz += sizeof(VATBCfg::Item) * delta;
			THROW_MEM(p_cfg = static_cast<PPVATBConfig *>(SAlloc::R(p_cfg, sz)));
			THROW(r = PPRef->GetPropMainConfig(prop, p_cfg, sz));
		} while(r > 0);
	}
	if(r > 0) {
		pConfig->AccSheetID = p_cfg->AccSheetID;
		pConfig->Flags      = p_cfg->Flags;
		pConfig->AcctgBasis = p_cfg->AcctgBasis;
		pConfig->AcctgBasisAtPeriod = p_cfg->AcctgBasisAtPeriod;
		pConfig->Period.Set(p_cfg->BegDt, p_cfg->EndDt);
		if(p_cfg->Ver.IsLt(7, 6, 1)) {
			for(i = 0; i < p_cfg->ListCount; i++) {
				const ObjRestrictItem & r_oitem = (reinterpret_cast<ObjRestrictItem *>(p_cfg+1))[i];
				VATBCfg::Item item;
				MEMSZERO(item);
				item.OpID = r_oitem.ObjID;
				item.Flags = r_oitem.Flags;
				THROW_SL(pConfig->List.insert(&item));
			}
		}
		else {
			for(i = 0; i < p_cfg->ListCount; i++) {
				const VATBCfg::Item & r_item = (reinterpret_cast<const VATBCfg::Item *>(p_cfg+1))[i];
				THROW_SL(pConfig->List.insert(&r_item));
			}
		}
	}
	CATCH
		r = 0;
	ENDCATCH
	SAlloc::F(p_cfg);
	return r;
}

/*static*/int PPObjVATBook::WriteCfgList(PPID kind, const VATBCfg * pConfig, int use_ta)
{
	int    ok = 1;
	const  uint items_count = pConfig->List.getCount();
	const  uint sz = sizeof(PPVATBConfig) + items_count * sizeof(VATBCfg::Item);
	uint   i;
	PPID   prop = 0;
	PPID   cfg_id = 0;
	PPVATBConfig * p_cfg = 0;
	if(kind == PPVTB_BUY) {
		prop = PPPRP_VATBBCFG;
		cfg_id = PPCFGOBJ_VATBOOKBUY;
	}
	else if(kind == PPVTB_SELL) {
		prop = PPPRP_VATBSCFG;
		cfg_id = PPCFGOBJ_VATBOOKSELL;
	}
	else if(kind == PPVTB_SIMPLELEDGER) {
		prop = PPPRP_SMPLLEDGCFG;
		cfg_id = PPCFGOBJ_SIMPLELEDGER;
	}
	THROW_INVARG(oneof3(kind, PPVTB_SELL, PPVTB_BUY, PPVTB_SIMPLELEDGER));
	THROW(CheckCfgRights(cfg_id, PPR_MOD, 0));
	THROW_MEM(p_cfg = static_cast<PPVATBConfig *>(SAlloc::M(sz)));
	memzero(p_cfg, sz);
	p_cfg->Ver = __VATBookFormatVer;
	p_cfg->AccSheetID = pConfig->AccSheetID;
	p_cfg->Flags      = pConfig->Flags;
	p_cfg->AcctgBasis = pConfig->AcctgBasis;
	p_cfg->AcctgBasisAtPeriod = pConfig->AcctgBasisAtPeriod;
	p_cfg->BegDt      = pConfig->Period.low;
	p_cfg->EndDt      = pConfig->Period.upp;
	p_cfg->ListCount  = items_count;
	for(i = 0; i < items_count; i++) {
		reinterpret_cast<VATBCfg::Item *>(&p_cfg[1])[i] = pConfig->List.at(i);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, prop, p_cfg, sz, 0));
		DS.LogAction(PPACN_CONFIGUPDATED, cfg_id, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

int PPObjVATBook::ReadConfig()
{
	int   ok = 1;
	THROW(ReadCfgList(PPVTB_SELL, &VATBCSell));
	THROW(ReadCfgList(PPVTB_BUY, &VATBCBuy));
	THROW(ReadCfgList(PPVTB_SIMPLELEDGER, &VATBSmplLedg));
	CATCHZOK
	return ok;
}

const VATBCfg & FASTCALL PPObjVATBook::GetConfig(PPID kind) const
{
	if(kind == PPVTB_SELL)
		return VATBCSell;
	else if(kind == PPVTB_BUY)
		return VATBCBuy;
	else if(kind == PPVTB_SIMPLELEDGER)
		return VATBSmplLedg;
	else {
		PPSetError(PPERR_INVVATBOOKKIND);
		return *static_cast<const VATBCfg *>(0);
	}
}

int PPObjVATBook::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_VATBOOK, 0, PPView::exefModeless, 0); }
//
// @VATBookDialog {
//
class VATBookDialog : public TDialog {
	DECL_DIALOG_DATA(VATBookTbl::Rec);
	struct VatCtlEntry {
		uint   Idx; // [0..]
		uint   VatCtlId;
		uint   SVatCtlId;
		uint   LabelCtlId;
		double * P_AmtV; // Значение суммы без НДС, соответствующее ставке с индексом Idx (поле VatCtlId)
		double * P_VatV; // Значение суммы НДС, соответствующее ставке с индексом Idx (поле SVatCtlId)
	};
	static VatCtlEntry VatCtlList[5];
public:
	VATBookDialog(uint rezID, PPObjVATBook * pVbObj) : TDialog(rezID), P_VbObj(pVbObj)
	{
		VatCtlList[0].P_AmtV = &Data.VAT1;
		VatCtlList[1].P_AmtV = &Data.VAT2;
		VatCtlList[2].P_AmtV = &Data.VAT3;
		VatCtlList[3].P_AmtV = &Data.VAT4;
		VatCtlList[4].P_AmtV = &Data.VAT5;
		VatCtlList[0].P_VatV = &Data.SVAT1;
		VatCtlList[1].P_VatV = &Data.SVAT2;
		VatCtlList[2].P_VatV = &Data.SVAT3;
		VatCtlList[3].P_VatV = &Data.SVAT4;
		VatCtlList[4].P_VatV = &Data.SVAT5;
		disableCtrl(CTL_VATBOOK_AMT, true);
		SetupCalDate(CTLCAL_VATBOOK_DT,      CTL_VATBOOK_DT);
		SetupCalDate(CTLCAL_VATBOOK_INVCDT,  CTL_VATBOOK_INVCDT);
		SetupCalDate(CTLCAL_VATBOOK_PAYMDT,  CTL_VATBOOK_PAYMDT);
		SetupCalDate(CTLCAL_VATBOOK_RCPTDT,  CTL_VATBOOK_RCPTDT);
		SetupCalDate(CTLCAL_VATBOOK_CBILLDT, CTL_VATBOOK_CBILLDT);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPID   acs_id = 0;
		if(Data.ArID) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			if(ar_obj.Fetch(Data.ArID, &ar_rec) > 0)
				acs_id = ar_rec.AccSheetID;
		}
		if(!acs_id && P_VbObj->IsValidKind(Data.LineType_))
			acs_id = P_VbObj->GetConfig(Data.LineType_).AccSheetID;
		enableCommand(cmVATBookLink, (int)Data.LinkBillID);
		setCtrlDate(CTL_VATBOOK_DT,     Data.Dt);
		setCtrlData(CTL_VATBOOK_CODE,   Data.Code);
		setCtrlData(CTL_VATBOOK_INVCDT, &Data.InvcDt);
		setCtrlData(CTL_VATBOOK_PAYMDT, &Data.PaymDt);
		setCtrlData(CTL_VATBOOK_RCPTDT, &Data.RcptDt);
		setCtrlData(CTL_VATBOOK_AMT,    &Data.Amount);
		setCtrlDate(CTL_VATBOOK_CBILLDT,   Data.CBillDt);
		setCtrlData(CTL_VATBOOK_CBILLCODE, Data.CBillCode);
		setCtrlData(CTL_VATBOOK_TAXOPCODE, Data.TaxOpCode);
		{
			SString rate_buf;
			for(uint i = 0; i < SIZEOFARRAY(VatCtlList); i++) {
				setStaticText(VatCtlList[i].LabelCtlId, VatRateStr(PPObjVATBook::GetVatRate(VatCtlList[i].Idx), rate_buf));
				setCtrlData(VatCtlList[i].VatCtlId, VatCtlList[i].P_AmtV);
				setCtrlData(VatCtlList[i].SVatCtlId, VatCtlList[i].P_VatV);
			}
		}
		setCtrlData(CTL_VATBOOK_VAT0,   &Data.VAT0);
		setCtrlData(CTL_VATBOOK_EXP,    &Data.Export);
		SetupArCombo(this, CTLSEL_VATBOOK_OBJ, Data.ArID, OLW_LOADDEFONOPEN|OLW_CANINSERT, acs_id, 0);
		SetupPPObjCombo(this, CTLSEL_VATBOOK_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
		if(Data.Excluded)
			Data.Flags |= VATBF_EXCLUDED;
		AddClusterAssoc(CTL_VATBOOK_FLAGS, 0, VATBF_FIX);
		AddClusterAssoc(CTL_VATBOOK_FLAGS, 1, VATBF_EXCLUDED);
		AddClusterAssoc(CTL_VATBOOK_FLAGS, 2, VATBF_VATFREE);
		SetClusterData(CTL_VATBOOK_FLAGS, Data.Flags);
		disableCtrls(BIN(Data.Flags & VATBF_FIX), 
			CTL_VATBOOK_VAT1, CTL_VATBOOK_VAT2, CTL_VATBOOK_VAT3, CTL_VATBOOK_VAT4, CTL_VATBOOK_VAT5, 
			CTL_VATBOOK_SVAT1, CTL_VATBOOK_SVAT2, CTL_VATBOOK_SVAT3, CTL_VATBOOK_SVAT4, CTL_VATBOOK_SVAT5,
			CTL_VATBOOK_VAT0, CTL_VATBOOK_EXP, 0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		CalcSum();
		getCtrlData(CTL_VATBOOK_DT,          &Data.Dt);
		getCtrlData(CTL_VATBOOK_CODE,        Data.Code);
		getCtrlData(CTL_VATBOOK_CBILLDT,     &Data.CBillDt);
		getCtrlData(CTL_VATBOOK_CBILLCODE,   Data.CBillCode);
		getCtrlData(CTL_VATBOOK_TAXOPCODE,   Data.TaxOpCode);
		if(!getCtrlData(CTL_VATBOOK_INVCDT, &Data.InvcDt))
			Data.InvcDt = Data.Dt;
		getCtrlData(CTL_VATBOOK_PAYMDT, &Data.PaymDt);
		getCtrlData(CTL_VATBOOK_RCPTDT, &Data.RcptDt);
		getCtrlData(CTL_VATBOOK_AMT,    &Data.Amount);
		for(uint i = 0; i < SIZEOFARRAY(VatCtlList); i++) {
			getCtrlData(VatCtlList[i].VatCtlId, VatCtlList[i].P_AmtV);
			getCtrlData(VatCtlList[i].SVatCtlId, VatCtlList[i].P_VatV);
		}
		getCtrlData(CTL_VATBOOK_VAT0,   &Data.VAT0);
		getCtrlData(CTL_VATBOOK_EXP,    &Data.Export);
		getCtrlData(CTLSEL_VATBOOK_OBJ, &Data.ArID);
		getCtrlData(CTL_VATBOOK_LOC, &Data.LocID);
		GetClusterData(CTL_VATBOOK_FLAGS, &Data.Flags);
		Data.Excluded = BIN(Data.Flags & VATBF_EXCLUDED);
		if(P_VbObj->ValidateData(&Data, 0)) {
			ASSIGN_PTR(pData, Data);
		}
		else
			ok = 0;
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   CalcSum();
	void   CalcVatSum(uint ctl);
	void   linkBill()
	{
		PPID _id_to_edit = labs(Data.LinkBillID);
		if(_id_to_edit)
			BillObj->Edit(&_id_to_edit, 0);
	}
	PPObjVATBook * P_VbObj;
};

VATBookDialog::VatCtlEntry VATBookDialog::VatCtlList[5] = {
	{ 0, CTL_VATBOOK_VAT1, CTL_VATBOOK_SVAT1, CTL_VATBOOK_TVAT1 },
	{ 1, CTL_VATBOOK_VAT2, CTL_VATBOOK_SVAT2, CTL_VATBOOK_TVAT2 },
	{ 2, CTL_VATBOOK_VAT3, CTL_VATBOOK_SVAT3, CTL_VATBOOK_TVAT3 },
	{ 3, CTL_VATBOOK_VAT4, CTL_VATBOOK_SVAT4, CTL_VATBOOK_TVAT4 },
	{ 4, CTL_VATBOOK_VAT5, CTL_VATBOOK_SVAT5, CTL_VATBOOK_TVAT5 },
};

void VATBookDialog::CalcSum()
{
	double sum = 0.0;
	LongArray ctl_id_list;
	{
		for(uint i = 0; i < SIZEOFARRAY(VatCtlList); i++) {
			ctl_id_list.add(VatCtlList[i].VatCtlId);
			ctl_id_list.add(VatCtlList[i].SVatCtlId);
		}
		ctl_id_list.add(CTL_VATBOOK_VAT0);
		ctl_id_list.add(CTL_VATBOOK_EXP);
	}
	{
		for(uint i = 0; i < ctl_id_list.getCount(); i++) {
			double t = getCtrlReal(ctl_id_list.get(i));
			sum += t;
		}
	}
	setCtrlData(CTL_VATBOOK_AMT, &sum);
}

void VATBookDialog::CalcVatSum(uint ctl)
{
	char   text[256];
	double m = 0.0;
	getCtrlData(ctl, &m);
	for(uint i = 0; i < SIZEOFARRAY(VatCtlList); i++) {
		if(ctl == VatCtlList[i].VatCtlId) {
			ctl = VatCtlList[i].SVatCtlId;
			const double rate = PPObjVATBook::GetVatRate(VatCtlList[i].Idx);
			double v = m;
			{
				TInputLine * il = static_cast<TInputLine *>(getCtrlView(ctl));
				if(il) {
					strip(strcpy(text, il->getText()));
					if(text[0] == 0) {
						v *= fdiv100r(rate);
						m = v;
						il->TransmitData(+1, &m);
					}
				}
			}
			break;
		}
	}
}

IMPL_HANDLE_EVENT(VATBookDialog)
{
	TDialog::handleEvent(event);
	if(TVBROADCAST && oneof2(TVCMD, cmReleasedFocus, cmCommitInput)) {
		const uint i = TVINFOVIEW->GetId();
		if(i == CTL_VATBOOK_DT) {
			LDATE  tmp_dt;
			LDATE  dt = getCtrlDate(CTL_VATBOOK_DT);
			if(dt) {
				if(getCtrlData(CTL_VATBOOK_INVCDT, &(tmp_dt = ZERODATE)) && !tmp_dt)
					setCtrlData(CTL_VATBOOK_INVCDT, &dt);
				if(getCtrlData(CTL_VATBOOK_RCPTDT, &(tmp_dt = ZERODATE)) && !tmp_dt)
					setCtrlData(CTL_VATBOOK_RCPTDT, &dt);
			}
		}
		else if(oneof5(i, CTL_VATBOOK_VAT1, CTL_VATBOOK_VAT2, CTL_VATBOOK_VAT3, CTL_VATBOOK_VAT4, CTL_VATBOOK_VAT5)) {
			CalcVatSum(i);
			CalcSum();
		}
		else if(oneof7(i, CTL_VATBOOK_VAT0, CTL_VATBOOK_SVAT1, CTL_VATBOOK_SVAT2, CTL_VATBOOK_SVAT3, CTL_VATBOOK_SVAT4, CTL_VATBOOK_SVAT5, CTL_VATBOOK_EXP))
			CalcSum();
		else
			return;
	}
	else if(event.isCmd(cmVATBookLink))
		linkBill();
	else if(event.isCmd(cmVATBookSum))
		CalcSum();
	else if(event.isClusterClk(CTL_VATBOOK_FLAGS)) {
		ushort v = getCtrlUInt16(CTL_VATBOOK_FLAGS);
		disableCtrls(v & 0x01, CTL_VATBOOK_VAT1, CTL_VATBOOK_VAT2, CTL_VATBOOK_VAT3, CTL_VATBOOK_VAT4, CTL_VATBOOK_VAT5,
			CTL_VATBOOK_SVAT1, CTL_VATBOOK_SVAT2, CTL_VATBOOK_SVAT3, CTL_VATBOOK_SVAT4, CTL_VATBOOK_SVAT5,
			CTL_VATBOOK_VAT0, CTL_VATBOOK_EXP, 0);
	}
	else
		return;
	clearEvent(event);
}
//
// } @VATBookDialog
//

//
// SimpleLedgerDialog {
//
class SimpleLedgerDialog : public TDialog {
	DECL_DIALOG_DATA(VATBookTbl::Rec);
public:
	explicit SimpleLedgerDialog(PPObjVATBook * aPPObj) : TDialog(DLG_SMPLLEDG), ppobj(aPPObj)
	{
		SetupCalDate(CTLCAL_SMPLLEDG_DT, CTL_SMPLLEDG_DT);
	}
	DECL_DIALOG_SETDTS()
	{
		ushort v = 0;
		PPID   acs_id = 0;
		RVALUEPTR(Data, pData);
		if(Data.ArID) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			if(ar_obj.Fetch(Data.ArID, &ar_rec) > 0)
				acs_id = ar_rec.AccSheetID;
		}
		if(acs_id == 0 && ppobj->IsValidKind(Data.LineType_))
			acs_id = ppobj->GetConfig(Data.LineType_).AccSheetID;
		SetupArCombo(this, CTLSEL_SMPLLEDG_OBJ, Data.ArID, OLW_LOADDEFONOPEN|OLW_CANINSERT, acs_id, 0);
		enableCommand(cmVATBookLink, (int)Data.LinkBillID);
		setCtrlData(CTL_SMPLLEDG_DT,     &Data.Dt);
		setCtrlData(CTL_SMPLLEDG_CODE,   Data.Code);
		SETFLAG(v, 0x01, Data.Flags & VATBF_FIX);
		SETFLAG(v, 0x02, ((Data.Flags & VATBF_EXCLUDED) || Data.Excluded));
		setCtrlData(CTL_SMPLLEDG_FLAGS,  &v);
		setIncExpCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		getCtrlData(CTL_SMPLLEDG_DT, &Data.Dt);
		LDATE  rcpt_dt = Data.RcptDt;
		LDATE  invc_dt = Data.InvcDt;
		Data.RcptDt = Data.InvcDt = Data.Dt; // нужно для того, чтобы ValidateData() не выдавала ошибку
		Data.PaymDt = Data.Dt;
		getCtrlData(CTL_SMPLLEDG_CODE, Data.Code);
		ushort v = getCtrlUInt16(CTL_SMPLLEDG_INCEXPSEL);
		getIncExpData(v);
		getCtrlData(CTLSEL_SMPLLEDG_OBJ, &Data.ArID);
		getCtrlData(CTL_SMPLLEDG_FLAGS,  &v);
		SETFLAG(Data.Flags, VATBF_FIX,      v & 0x01);
		SETFLAG(Data.Flags, VATBF_EXCLUDED, v & 0x02);
		Data.Excluded = BIN(v & 0x02);
		if(ppobj->ValidateData(&Data, 0)) {
			Data.RcptDt = rcpt_dt;
			Data.InvcDt = invc_dt;
			ASSIGN_PTR(pData, Data);
			return 1;
		}
		else
			return 0;
	}
private:
	DECL_HANDLE_EVENT;
	void   linkBill()
	{
		PPID _id_to_edit = labs(Data.LinkBillID);
		if(_id_to_edit)
			BillObj->Edit(&_id_to_edit, 0);
	}
	void   setIncExpData(int incomeTxt);
	void   getIncExpData(int expend);
	void   setIncExpCtrls()
	{
		const double amt  = Data.VAT0;
		const double amtv = Data.Export;
		ushort v = BIN(amt != 0.0 || amtv != 0.0);
		setIncExpData(v);
		setCtrlData(CTL_SMPLLEDG_INCEXPSEL, &v);
	}
	PPObjVATBook * ppobj;
};

void SimpleLedgerDialog::setIncExpData(int expend)
{
	double amt = 0.0;
	double amtv = 0.0;
	SString amt_txt;
	SString amtv_txt;
	if(!expend) {
		PPLoadText(PPTXT_SMPLLEDGINC,  amt_txt);
		PPLoadText(PPTXT_SMPLLEDGINCV, amtv_txt);
		amt = Data.Amount;
		amtv = Data.Excise;
		Data.VAT0 = 0.0;
		Data.Export = 0.0;
	}
	else {
		PPLoadText(PPTXT_SMPLLEDGEXP,  amt_txt);
		PPLoadText(PPTXT_SMPLLEDGEXPV, amtv_txt);
		amt = Data.VAT0;
		amtv = Data.Export;
		Data.Amount = 0.0;
		Data.Excise = 0.0;
	}
	setStaticText(CTL_SMPLLEDG_INCEXPTXT, amt_txt);
	setStaticText(CTL_SMPLLEDG_INCEXPVTXT, amtv_txt);
	setCtrlData(CTL_SMPLLEDG_INCEXPAMT,  &amt);
	setCtrlData(CTL_SMPLLEDG_INCEXPVAMT, &amtv);
}

void SimpleLedgerDialog::getIncExpData(int expend)
{
	if(!expend) {
		getCtrlData(CTL_SMPLLEDG_INCEXPAMT,  &Data.Amount);
		getCtrlData(CTL_SMPLLEDG_INCEXPVAMT, &Data.Excise);
		Data.Export = 0.0;
		Data.VAT0 = 0.0;
	}
	else {
		getCtrlData(CTL_SMPLLEDG_INCEXPAMT,  &Data.VAT0);
		getCtrlData(CTL_SMPLLEDG_INCEXPVAMT, &Data.Export);
		Data.Amount = 0.0;
		Data.Excise = 0.0;
	}
}

IMPL_HANDLE_EVENT(SimpleLedgerDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_SMPLLEDG_INCEXPSEL))
		setIncExpData(getCtrlUInt16(CTL_SMPLLEDG_INCEXPSEL));
	else if(event.isCmd(cmVATBookLink))
		linkBill();
	else
		return;
	clearEvent(event);
}
//
// } SimpleLedgerDialog
//
PPID PPObjVATBook::SelectLineType(uint * pResID, PPID what)
{
	uint   ri = 0;
	ushort v  = 0;
	TDialog * dlg = 0;
	if(what == PPVTB_SELL)
		ri = DLG_VATSELL;
	else if(what == PPVTB_BUY)
		ri = DLG_VATBUY;
	else if(what == PPVTB_SIMPLELEDGER)
		ri = DLG_SMPLLEDG;
	else if(CheckDialogPtr(&(dlg = new TDialog(DLG_VATBWHAT)))) {
		dlg->setCtrlData(CTL_VATBWHAT_WHAT, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_VATBWHAT_WHAT, &v);
			if(v == 0) {
				ri = DLG_VATSELL;
				what = PPVTB_SELL;
			}
			else if(v == 1) {
				ri = DLG_VATBUY;
				what = PPVTB_BUY;
			}
			else {
				ri = DLG_SMPLLEDG;
				what = PPVTB_SIMPLELEDGER;
			}
		}
		else
			what = -1;
	}
	else
		what = 0;
	ASSIGN_PTR(pResID, ri)
	delete dlg;
	return what;
}

int PPObjVATBook::ValidateData(void * pData, long)
{
	int    ok = 1;
	VATBookTbl::Rec * p_rec = static_cast<VATBookTbl::Rec *>(pData);
	THROW(IsValidKind(p_rec->LineType_));
	THROW_PP(*strip(p_rec->Code) != 0, PPERR_VATBCODENEEDED);
	THROW_PP(checkdate(p_rec->Dt, 1), PPERR_INVVATBDT);
	THROW_PP(checkdate(p_rec->InvcDt, 1), PPERR_INVVATBDT);
	THROW_PP(checkdate(p_rec->PaymDt, 1), PPERR_INVVATBDT);
	THROW_PP(checkdate(p_rec->RcptDt, 1), PPERR_INVVATBDT);
	THROW_PP(checkdate(p_rec->CBillDt, 1), PPERR_INVVATBDT);
	CATCHZOK
	return ok;
}

int PPObjVATBook::EditObj(PPID * pID, void * pData, int use_ta)
{
	int    ok = 1;
	VATBookTbl::Rec * p_rec = static_cast<VATBookTbl::Rec *>(pData);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(UpdateByID(P_Tbl, Obj, *pID, p_rec, 0));
		}
		else {
			THROW(IncDateKey(P_Tbl, 2, p_rec->Dt, &p_rec->LineNo));
			THROW(AddByID(P_Tbl, pID, p_rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SETVATBOOKDTS(TDialog * pDlg, VATBookTbl::Rec * pRec, PPID kind)
	{ return (kind == PPVTB_SIMPLELEDGER) ? static_cast<SimpleLedgerDialog *>(pDlg)->setDTS(pRec) : static_cast<VATBookDialog *>(pDlg)->setDTS(pRec); }
int GETVATBOOKDTS(TDialog * pDlg, VATBookTbl::Rec * pRec, PPID kind)
	{ return (kind == PPVTB_SIMPLELEDGER) ? static_cast<SimpleLedgerDialog *>(pDlg)->getDTS(pRec) : static_cast<VATBookDialog *>(pDlg)->getDTS(pRec); }

int PPObjVATBook::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = -1;
	int    valid_data = 0;
	uint   res_id = 0;
	VATBookTbl::Rec rec;
	TDialog * dlg = 0;
	if(sampleID > 0) {
		THROW(Search(sampleID, &rec) > 0);
		rec.ID = 0;
		rec.LineNo = 0;
		rec.LinkBillID = 0;
		if(rec.LineType_ == PPVTB_SELL)
			res_id = DLG_VATSELL;
		else if(rec.LineType_ == PPVTB_BUY)
			res_id = DLG_VATBUY;
		else if(rec.LineType_ == PPVTB_SIMPLELEDGER)
			res_id = DLG_SMPLLEDG;
		else
			return -1;
		if(rec.LineType_ != PPVTB_SIMPLELEDGER) {
			THROW(CheckDialogPtr(&(dlg = new VATBookDialog(res_id, this))));
		}
		else {
			THROW(CheckDialogPtr(&(dlg = new SimpleLedgerDialog(this))));
		}
		THROW(SETVATBOOKDTS(dlg, &rec, rec.LineType_));
		while(!valid_data && ExecView(dlg) == cmOK) {
			if((valid_data = GETVATBOOKDTS(dlg, &rec, rec.LineType_)) != 0) {
				THROW(EditObj(pID, &rec, 1));
				ok = cmOK;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjVATBook::Edit(PPID * pID, void * extraPtr /*kind*/)
{
	const  PPID extra_kind = reinterpret_cast<const  PPID>(extraPtr);
	int    ok = -1;
	int    valid_data = 0;
	PPID   _kind = 0;
	uint   res_id = 0;
	VATBookTbl::Rec rec;
	TDialog * p_dlg = 0;
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else {
		rec.Dt = getcurdate_();
	}
	THROW(_kind = SelectLineType(&res_id, NZOR(rec.LineType_, extra_kind)));
	if(_kind > 0) {
		THROW(IsValidKind(_kind));
		if(*pID == 0)
			rec.LineType_ = static_cast<int16>(_kind);
		p_dlg = (_kind == PPVTB_SIMPLELEDGER) ? static_cast<TDialog *>(new SimpleLedgerDialog(this)) : static_cast<TDialog *>(new VATBookDialog(res_id, this));
		THROW(CheckDialogPtr(&p_dlg));
		THROW(SETVATBOOKDTS(p_dlg, &rec, _kind));
		while(!valid_data && ExecView(p_dlg) == cmOK)
			if((valid_data = GETVATBOOKDTS(p_dlg, &rec, _kind)) != 0) {
				THROW(EditObj(pID, &rec, 1));
				ok = cmOK;
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
// @VATBCfgDialog {
//
class VATBCfgDialog : public PPListDialog {
public:
	explicit VATBCfgDialog(uint rezID) : PPListDialog(rezID, CTL_VATBCFG_LIST), P_List(0)
	{
		updateList(-1);
	}
	int    setDTS(const VATBCfg *);
	int    getDTS(VATBCfg *);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    editItemDialog(VATBCfg::Item *);

	VATBCfg Data;
	SmartListBox * P_List;
};

int VATBCfgDialog::setupList()
{
	SString sub;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.List.getCount(); i++) {
		const VATBCfg::Item & r_item = Data.List.at(i);
		ss.Z();
		GetOpName(r_item.OpID, sub.Z());
		ss.add(sub);
		sub.Z();
		if(r_item.Flags & VATBCfg::fExclude)
			sub.CatChar('x');
		if(r_item.Flags & VATBCfg::fNegative)
			sub.CatChar('-');
		if(r_item.Flags & VATBCfg::fByExtObj)
			sub.CatChar('E');
		if(r_item.Flags & VATBCfg::fVATFree)
			sub.CatChar('F');
		ss.add(sub);
		if(!P_Box->addItem(/*r_item.OpID*/i+1, ss.getBuf()))
			return PPSetErrorSLib();
	}
	return 1;
}

int VATBCfgDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	VATBCfg::Item item;
	MEMSZERO(item);
	if(editItemDialog(&item) > 0) {
		if(Data.List.insert(&item)) {
			const long c = static_cast<const long>(Data.List.getCount());
			ASSIGN_PTR(pID, /*item.OpID*/c);
			ASSIGN_PTR(pPos, c-1);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int VATBCfgDialog::editItem(long pos, long id)
{
	int    ok = -1;
	uint   p = (uint)pos;
	if(p < Data.List.getCount()) {
		VATBCfg::Item item = Data.List.at(p);
		if(editItemDialog(&item) > 0) {
			Data.List.at(p) = item;
			ok = 1;
		}
	}
	return ok;
}

int VATBCfgDialog::delItem(long pos, long id)
{
	if(pos >= 0 && pos < static_cast<long>(Data.List.getCount())) {
		Data.List.atFree(static_cast<uint>(pos));
		return 1;
	}
	else
		return -1;
}

int VATBCfgDialog::editItemDialog(VATBCfg::Item * pItem)
{
	class VATBItemDlg : public TDialog {
		DECL_DIALOG_DATA(VATBCfg::Item);
	public:
		explicit VATBItemDlg(VATBCfg * pCfg) : TDialog((pCfg->Kind == PPVTB_SIMPLELEDGER) ? DLG_VATBL_SIMPLE : DLG_VATBL), P_Cfg(pCfg)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			PPID   op_id = 0;
			PPIDArray op_list;
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			while(EnumOperations(0L, &op_id) > 0)
				op_list.add(op_id);
			SetupOprKindCombo(this, CTLSEL_VATBL_OPRKIND, Data.OpID, 0, &op_list, OPKLF_OPLIST|OPKLF_SHOWPASSIVE);
			AddClusterAssoc(CTL_VATBL_FLAGS, 0, VATBCfg::fExclude);
			AddClusterAssoc(CTL_VATBL_FLAGS, 1, VATBCfg::fNegative);
			AddClusterAssoc(CTL_VATBL_FLAGS, 2, VATBCfg::fByExtObj);
			AddClusterAssoc(CTL_VATBL_FLAGS, 3, VATBCfg::fVATFromReckon);
			AddClusterAssoc(CTL_VATBL_FLAGS, 4, VATBCfg::fVATFree);
			AddClusterAssoc(CTL_VATBL_FLAGS, 5, VATBCfg::fAsPayment);
			AddClusterAssoc(CTL_VATBL_FLAGS, 6, VATBCfg::fReckonDateByPayment); // @v11.9.10
			SetClusterData(CTL_VATBL_FLAGS, Data.Flags);
			// @v11.0.3 {
			AddClusterAssoc(CTL_VATBL_SIGNFILT, 0, 0);
			AddClusterAssoc(CTL_VATBL_SIGNFILT, 1, +1);
			AddClusterAssoc(CTL_VATBL_SIGNFILT, 2, -1);
			SetClusterData(CTL_VATBL_SIGNFILT, Data.SignFilt);
			// } @v11.0.3 
			{
				long   exp_by_fact = 0;
				if(Data.Flags & VATBCfg::fExpendByFact)
					if(Data.Flags & VATBCfg::fFactByShipment)
						exp_by_fact = 2;
					else
						exp_by_fact = 1;
				AddClusterAssocDef(CTL_VATBL_EXPBYFACT, 0, 0);
				AddClusterAssoc(CTL_VATBL_EXPBYFACT, 1, 1);
				AddClusterAssoc(CTL_VATBL_EXPBYFACT, 2, 2);
				SetClusterData(CTL_VATBL_EXPBYFACT, exp_by_fact);
			}
			DisableClusterItem(CTL_VATBL_FLAGS, 4, P_Cfg->Kind != PPVTB_SIMPLELEDGER);
			DisableClusterItem(CTL_VATBL_FLAGS, 5, P_Cfg->Kind != PPVTB_SIMPLELEDGER); // @v11.9.10 @fix 6-->5
			DisableClusterItem(CTL_VATBL_FLAGS, 6, P_Cfg->Kind == PPVTB_SIMPLELEDGER); // @v11.9.10
			if(P_Cfg->Kind == PPVTB_SIMPLELEDGER)
				SetupPPObjCombo(this, CTLSEL_VATBL_MAINAMT, PPOBJ_AMOUNTTYPE, Data.MainAmtTypeID, 0, 0);
			setVATFld();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPOprKind opk_rec;
			getCtrlData(CTLSEL_VATBL_OPRKIND, &Data.OpID);
			GetClusterData(CTL_VATBL_FLAGS, &Data.Flags);
			{
				const long exp_by_fact = GetClusterData(CTL_VATBL_EXPBYFACT);
				Data.Flags &= ~(VATBCfg::fExpendByFact | VATBCfg::fFactByShipment);
				if(exp_by_fact == 1)
					Data.Flags |= VATBCfg::fExpendByFact;
				else if(exp_by_fact == 2)
					Data.Flags |= (VATBCfg::fExpendByFact | VATBCfg::fFactByShipment);
			}
			{
				long temp_sf = 0;
				GetClusterData(CTL_VATBL_SIGNFILT, &temp_sf);
				Data.SignFilt = oneof3(temp_sf, 0, -1, +1) ? static_cast<int8>(temp_sf) : 0;
			}
			if(P_Cfg->Kind == PPVTB_SIMPLELEDGER)
				getCtrlData(CTLSEL_VATBL_MAINAMT, &Data.MainAmtTypeID);
			if(!Data.OpID || GetOpData(Data.OpID, &opk_rec) <= 0)
				ok = PPErrorByDialog(this, CTL_VATBL_OPRKIND, PPERR_OPRKINDNEEDED);
			else if(Data.Flags & VATBCfg::fByExtObj && !opk_rec.AccSheet2ID)
				ok = PPErrorByDialog(this, CTLSEL_VATBL_OPRKIND, PPERR_OPHASNTEXTOBJ);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_VATBL_OPRKIND)) {
				setVATFld();
				clearEvent(event);
			}
		}
		void   setVATFld()
		{
			int    is_paym_op = 0;
			const  PPID op_id = getCtrlLong(CTLSEL_VATBL_OPRKIND);
			ushort v = getCtrlUInt16(CTL_VATBL_FLAGS);
			if(IsOpPaym(op_id))
				is_paym_op = 1;
			else
				setCtrlUInt16(CTL_VATBL_FLAGS, v & ~(0x08 | 0x10));
			DisableClusterItem(CTL_VATBL_FLAGS, 3, !is_paym_op);
			DisableClusterItem(CTL_VATBL_FLAGS, 5, !is_paym_op);
			DisableClusterItem(CTL_VATBL_FLAGS, 6, !is_paym_op); // @v11.9.10
			disableCtrl(CTL_VATBL_EXPBYFACT, is_paym_op);
		}
		VATBCfg * P_Cfg;
	};
	DIALOG_PROC_BODY_P1(VATBItemDlg, &Data, pItem);
}

int VATBCfgDialog::setDTS(const VATBCfg * pData)
{
	ushort v;
	Data = *pData;
	SetupPPObjCombo(this, CTLSEL_VATBCFG_SHEET, PPOBJ_ACCSHEET, Data.AccSheetID, 0, 0);
	switch(Data.AcctgBasis) {
		case INCM_DEFAULT:    v = 0; break;
		case INCM_BYSHIPMENT: v = 1; break;
		case INCM_BYPAYMENT:  v = 2; break;
		default: v = 0; break;
	}
	setCtrlData(CTL_VATBCFG_INCM, &v);
	if(Data.Flags & VATBCfg::hfD_InvcDate)
		v = 1;
	else if(Data.Flags & VATBCfg::hfD_MaxInvcBill)
		v = 2;
	else
		v = 0;
	setCtrlUInt16(CTL_VATBCFG_SELDATE, v);
	//AddClusterAssoc(CTL_VATBCFG_INCMPRD, -1, INCM_DEFAULT);
	AddClusterAssoc(CTL_VATBCFG_INCMPRD,  0, INCM_BYSHIPMENT);
	AddClusterAssoc(CTL_VATBCFG_INCMPRD,  1, INCM_BYPAYMENT);
	if(Data.Kind == PPVTB_SIMPLELEDGER) {
		AddClusterAssoc(CTL_VATBCFG_INCMPRD,  2, INCM_BYPAYMENTINPERIOD);
	}
	SetClusterData(CTL_VATBCFG_INCMPRD, Data.AcctgBasisAtPeriod);
	SetupCalPeriod(CTLCAL_VATBCFG_PERIOD, CTL_VATBCFG_PERIOD);
	SetPeriodInput(this, CTL_VATBCFG_PERIOD, Data.Period);
	v = 0;
	if(Data.Kind == PPVTB_SELL) {
		AddClusterAssoc(CTL_VATBCFG_SELL_FLAGS, 0, VATBCfg::hfDontStornReckon);
		SetClusterData(CTL_VATBCFG_SELL_FLAGS, Data.Flags);
	}
	else if(Data.Kind == PPVTB_BUY) {
		AddClusterAssoc(CTL_VATBCFG_BUY_FLAGS, 0, VATBCfg::hfDontStornReckon);
		AddClusterAssoc(CTL_VATBCFG_BUY_FLAGS, 1, VATBCfg::hfIterateClb);
		SetClusterData(CTL_VATBCFG_BUY_FLAGS, Data.Flags);
	}
	else if(Data.Kind == PPVTB_SIMPLELEDGER) {
		disableCtrl(CTL_VATBCFG_INCM, true);
		DisableClusterItem(CTL_VATBCFG_BUY_FLAGS, 0, 1);
		AddClusterAssoc(CTL_VATBCFG_BUY_FLAGS, 0, VATBCfg::hfDontStornReckon);
		AddClusterAssoc(CTL_VATBCFG_BUY_FLAGS, 1, VATBCfg::hfWoTax);
		SetClusterData(CTL_VATBCFG_BUY_FLAGS, Data.Flags);
	}
	updateList(-1);
	return 1;
}

int VATBCfgDialog::getDTS(VATBCfg * pData)
{
	getCtrlData(CTLSEL_VATBCFG_SHEET, &Data.AccSheetID);
	ushort v = getCtrlUInt16(CTL_VATBCFG_INCM);
	switch(v) {
		case 0: Data.AcctgBasis = INCM_DEFAULT;    break;
		case 1: Data.AcctgBasis = INCM_BYSHIPMENT; break;
		case 2: Data.AcctgBasis = INCM_BYPAYMENT;  break;
		default: Data.AcctgBasis = INCM_DEFAULT;    break;
	}
	v = getCtrlUInt16(CTL_VATBCFG_SELDATE);
	Data.Flags &= ~(VATBCfg::hfD_InvcDate | VATBCfg::hfD_MaxInvcBill);
	if(v == 1)
		Data.Flags |= VATBCfg::hfD_InvcDate;
	else if(v == 2)
		Data.Flags |= VATBCfg::hfD_MaxInvcBill;
	GetClusterData(CTL_VATBCFG_INCMPRD);
	Data.AcctgBasisAtPeriod = (int16)GetClusterData(CTL_VATBCFG_INCMPRD);
	GetPeriodInput(this, CTL_VATBCFG_PERIOD, &Data.Period);
	GetClusterData((Data.Kind == PPVTB_SELL) ? CTL_VATBCFG_SELL_FLAGS : CTL_VATBCFG_BUY_FLAGS, &Data.Flags);
	if(Data.CheckList()) {
		*pData = Data;
		return 1;
	}
	return 0;
}

IMPL_HANDLE_EVENT(VATBCfgDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_VATBCFG_SHEET)) {
		PPID   acc_sheet_id = getCtrlLong(CTLSEL_VATBCFG_SHEET);
		if(Data.CheckAccSheet(acc_sheet_id))
			Data.AccSheetID = acc_sheet_id;
		else {
		   	PPError();
			if(!Data.CheckAccSheet(Data.AccSheetID))
				Data.AccSheetID = 0;
			setCtrlLong(CTLSEL_VATBCFG_SHEET, Data.AccSheetID);
		}
		clearEvent(event);
	}
}
//
// } @VATBCfgDialog
//
/*static*/int PPObjVATBook::EditConfig(PPID kind, VATBCfg * pConfig)
{
	int    ok = -1;
	uint   dlg_id = 0;
	PPID   cfg_id = 0;
	VATBCfgDialog * dlg = 0;
	PPObjVATBook vbobj;
	if(kind == PPVTB_SELL) {
		dlg_id = DLG_VATBCFG_SELL;
		cfg_id = PPCFGOBJ_VATBOOKSELL;
	}
	else if(kind == PPVTB_BUY) {
		dlg_id = DLG_VATBCFG_BUY;
		cfg_id = PPCFGOBJ_VATBOOKBUY;
	}
	else if(kind == PPVTB_SIMPLELEDGER) {
		dlg_id = DLG_VATBCFG_SMPLLEDG;
		cfg_id = PPCFGOBJ_SIMPLELEDGER;
	}
	else
		THROW_INVARG(0);
	THROW(CheckCfgRights(cfg_id, PPR_READ, 0));
	if(pConfig == 0) {
		if(kind == PPVTB_SELL)
			pConfig = &vbobj.VATBCSell;
		else if(kind == PPVTB_BUY)
			pConfig = &vbobj.VATBCBuy;
		else if(kind == PPVTB_SIMPLELEDGER) {
			pConfig = &vbobj.VATBSmplLedg;
			pConfig->Flags = (pConfig->Flags & VATBCfg::hfWoTax);
			pConfig->Flags |= VATBCfg::hfDontStornReckon;
			pConfig->AcctgBasis = INCM_BYPAYMENT;
		}
	}
	THROW(CheckDialogPtr(&(dlg = new VATBCfgDialog(dlg_id))));
	pConfig->Kind = kind;
	dlg->setDTS(pConfig);
	for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		THROW(CheckCfgRights(cfg_id, PPR_MOD, 0));
		if(dlg->getDTS(pConfig)) {
			WriteCfgList(kind, pConfig, 1);
			ok = valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// } @PPObjVATBook
//
//
// @ModuleDef(PPViewVatBook)
//
IMPLEMENT_PPFILT_FACTORY(VatBook); VatBookFilt::VatBookFilt() : PPBaseFilt(PPFILT_VATBOOK, 0, 1)
{
	SetFlatChunk(offsetof(VatBookFilt, ReserveStart),
		offsetof(VatBookFilt, Reserve)-offsetof(VatBookFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

VatBookFilt & FASTCALL VatBookFilt::operator = (const VatBookFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewVatBook::OpEntry::OpEntry() : OpID(0), AmtTypeID(0), SignFilt(0)
{
}

PPViewVatBook::MainOrgBlock::MainOrgBlock() : MainOrgID(0), SpecialTaxGroupID(0), Initialized(false), IsVatFree(false)
{
}
		
PPViewVatBook::MainOrgBlock & PPViewVatBook::MainOrgBlock::Z()
{
	MainOrgID = 0;
	SpecialTaxGroupID = 0;
	Initialized = false;
	IsVatFree = false;
	return *this;
}

bool PPViewVatBook::MainOrgBlock::FetchSpcTaxEntry(LDATE dt, PPGoodsTaxEntry & rGtx) const
{
	rGtx.Z();
	return (Initialized && SpecialTaxGroupID && PPObjGoodsTax::Fetch(SpecialTaxGroupID, dt, 0, &rGtx) > 0);
}

bool PPViewVatBook::MainOrgBlock::IsVatFree_(LDATE dt) const
{
	bool   result = false;
	PPGoodsTaxEntry gtx;
	if(FetchSpcTaxEntry(dt, gtx) && (gtx.Flags & GTAXF_SPCVAT) && gtx.GetVatRate() == 0.0)
		result = true;
	else if(IsVatFree)
		result = true;
	return result;
}

bool PPViewVatBook::MainOrgBlock::IsSpecialVatRate(LDATE dt, double * pRate) const
{
	bool   result = false;
	double vat_rate = 0.0;
	PPGoodsTaxEntry gtx;
	if(FetchSpcTaxEntry(dt, gtx) && (gtx.Flags & GTAXF_SPCVAT)) {
		vat_rate = gtx.GetVatRate();
		result = true;
	}
	else if(IsVatFree)
		result = true;
	ASSIGN_PTR(pRate, vat_rate);
	return result;
}

int PPViewVatBook::MainOrgBlock::Init()
{
	int   result = -1;
	if(!Initialized) {
		PersonTbl::Rec psn_rec;
		PPObjPerson psn_obj;
		if(GetMainOrgID(&MainOrgID) > 0 && psn_obj.Fetch(MainOrgID, &psn_rec) > 0) {
			if(psn_rec.Flags & PSNF_NOVATAX)
				IsVatFree = true;
			{
				ObjTagItem tag_item;
				if(PPRef->Ot.GetTag(PPOBJ_PERSON, MainOrgID, PPTAG_PERSON_SPCTAXGROUP, &tag_item) > 0) {
					long  tax_grp_id = 0;
					if(tag_item.GetInt(&tax_grp_id)) {
						PPObjGoodsTax gtx_obj;
						PPGoodsTax gtx_rec;
						if(gtx_obj.Search(tax_grp_id, &gtx_rec) > 0) {
							SpecialTaxGroupID = tax_grp_id;
						}
					}
				}
			}
			Initialized = true;
			result = 1;
		}
		else {
			result = 0;
		}
	}
	return result;
}

PPViewVatBook::PPViewVatBook() : PPView(0, &Filt, PPVIEW_VATBOOK, 0, 0), P_BObj(BillObj), P_GObj(0), P_ClbList(0)
{
}

PPViewVatBook::~PPViewVatBook()
{
	ZDELETE(P_ClbList);
	ZDELETE(P_GObj);
}

PPBaseFilt * PPViewVatBook::CreateFilt(const void * extraPtr) const
{
	return new VatBookFilt;
}

int PPViewVatBook::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		BExtQuery::ZDelete(&P_IterQuery);
		Counter.Init();
		MEMSZERO(Total);
		Filt.Period.Actualize(ZERODATE);
		if(Filt.Kind == PPVTB_BUY)
			SETFLAG(Filt.Flags, VatBookFilt::fIterateClb, VBObj.GetConfig(PPVTB_BUY).Flags & VATBCfg::hfIterateClb);
	}
	else
		ok = 0;
	return ok;
}

int PPViewVatBook::LoadClbList(PPID billID)
{
	int    ok = -1;
	SString clb;
	PPCountryBlock country_blk;
	BillTbl::Rec bill_rec;
	CALLPTRMEMB(P_ClbList, freeAll());
	while(billID && P_BObj->Search(billID, &bill_rec) > 0) {
		const PPID  op_type_id = GetOpType(bill_rec.OpID);
		if(op_type_id == PPOPT_PAYMENT)
			billID = bill_rec.LinkBillID;
		else {
			if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND)) {
				PPTransferItem ti;
				for(int rbb = 0; P_BObj->trfr->EnumItems(billID, &rbb, &ti) > 0;) {
					char   list_item[VBV_CLB_ITEM_SIZE];
					size_t li_pos = 0;
					bool   is_parent_lot = false;
					if(P_BObj->GetClbNumberByLot(ti.LotID, &is_parent_lot, clb) > 0) {
						THROW_MEM(SETIFZ(P_GObj, new PPObjGoods));
						THROW_MEM(SETIFZ(P_ClbList, new SArray(VBV_CLB_ITEM_SIZE)));
						THROW(P_GObj->GetManufCountry(labs(ti.GoodsID), 0, 0, &country_blk));
						clb.CopyTo(list_item, sizeof(list_item));
						li_pos = sstrlen(list_item);
						if(li_pos < sizeof(list_item)-1) {
							list_item[li_pos++] = ';';
							strnzcpy(list_item+li_pos, country_blk.Name, sizeof(list_item)-li_pos);
						}
						if(!P_ClbList->lsearch(list_item, 0, PTR_CMPFUNC(PcharNoCase))) {
							THROW_SL(P_ClbList->insert(list_item));
							ok = 1;
						}
					}
				}
			}
			billID = 0;
		}
	}
	CATCHZOK
	return ok;
}

int PPViewVatBook::InitIteration()
{
	int    ok = 1;
	const  int idx = (Filt.Flags & VatBookFilt::fPaymPeriod) ? 5 : 3;
	long   f;
	union {
		char _k[BTRMAXKEYLEN];
		VATBookTbl::Key3 k3;
		VATBookTbl::Key5 k5;
	} k, k_;
	VATBookTbl * p_t = VBObj.P_Tbl;
	DBQ * dbq = 0;

	BExtQuery::ZDelete(&P_IterQuery);
	MEMSZERO(InnerItem);
	ZDELETE(P_ClbList);
	ClbListIterPos = 0;
	Counter.Init();

	P_IterQuery = new BExtQuery(p_t, idx, 24);
	P_IterQuery->selectAll();
	dbq = & (p_t->LineType_ == Filt.Kind);
	if(Filt.Flags & VatBookFilt::fPaymPeriod)
		dbq = & (*dbq && daterange(p_t->PaymDt, &Filt.Period));
	else
		dbq = & (*dbq && daterange(p_t->Dt, &Filt.Period));
	f = (Filt.Flags & (VatBookFilt::fShowLink | VatBookFilt::fShowFree));
	if(f && f != (VatBookFilt::fShowLink | VatBookFilt::fShowFree))
		if(f & VatBookFilt::fShowLink)
			dbq = & (*dbq && p_t->LinkBillID > 0L);
		else
			dbq = & (*dbq && p_t->LinkBillID == 0L);
	dbq = ppcheckfiltid(dbq, p_t->ArID,  Filt.ArticleID);
	dbq = ppcheckfiltid(dbq, p_t->LocID,  Filt.LocID);
	if(Filt.Flags & VatBookFilt::fOnlyEmptyExtAr)
		dbq = &(*dbq && p_t->Ar2ID == 0L);
	else
		dbq = ppcheckfiltid(dbq, p_t->Ar2ID, Filt.Article2ID);
	if(!(Filt.Flags & VatBookFilt::fShowExcluded))
		dbq = & (*dbq && p_t->Excluded == 0L);
	P_IterQuery->where(*dbq);
	MEMSZERO(k);
	if(idx == 3) {
		k.k3.LineType_ = static_cast<int16>(Filt.Kind);
		k.k3.LineSubType = 0;
		k.k3.Dt = Filt.Period.low;
		k.k3.LineNo = 0;
	}
	else if(idx == 5) {
		k.k5.LineType_ = static_cast<int16>(Filt.Kind);
		k.k5.PaymDt = Filt.Period.low;
	}
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewVatBook::NextIteration(VatBookViewItem * pItem)
{
	char * p = 0;
	char   clb_item[64];
	if(P_IterQuery) {
		if(P_ClbList && ClbListIterPos < P_ClbList->getCount()) {
			STRNSCPY(clb_item, static_cast<const char *>(P_ClbList->at(ClbListIterPos)));
			ClbListIterPos++;
			if((p = sstrchr(clb_item, ';')) != 0) {
				*p = 0;
				STRNSCPY(InnerItem.CLB, clb_item);
				STRNSCPY(InnerItem.ManufCountry, p+1);
			}
			else {
				STRNSCPY(InnerItem.CLB, clb_item);
				InnerItem.ManufCountry[0] = 0;
			}
			if(pItem) {
				memzero(pItem, sizeof(VatBookViewItem));
				pItem->ID = InnerItem.ID;
				STRNSCPY(pItem->CLB, InnerItem.CLB);
				STRNSCPY(pItem->ManufCountry, InnerItem.ManufCountry);
			}
			return 1;
		}
		else if(P_IterQuery->nextIteration() > 0) {
			*static_cast<VATBookTbl::Rec *>(&InnerItem) = VBObj.P_Tbl->data;
			InnerItem.CLB[0] = 0;
			InnerItem.ManufCountry[0] = 0;
			ClbListIterPos = 0;
			if(Filt.Kind == PPVTB_BUY && Filt.Flags & VatBookFilt::fIterateClb)
				if(LoadClbList(InnerItem.LinkBillID)) {
					if(P_ClbList && P_ClbList->getCount()) {
						STRNSCPY(clb_item, static_cast<const char *>(P_ClbList->at(ClbListIterPos)));
						ClbListIterPos++;
						if((p = sstrchr(clb_item, ';')) != 0) {
							*p = 0;
							STRNSCPY(InnerItem.CLB, clb_item);
							STRNSCPY(InnerItem.ManufCountry, p+1);
						}
						else {
							STRNSCPY(InnerItem.CLB, clb_item);
							InnerItem.ManufCountry[0] = 0;
						}
					}
				}
				else
					return 0;
			ASSIGN_PTR(pItem, InnerItem);
			Counter.Increment();
			return 1;
		}
	}
	return -1;
}

int PPViewVatBook::CalcTotal(VatBookTotal * pTotal)
{
	VatBookViewItem item;
	memzero(pTotal, sizeof(*pTotal));
	for(InitIteration(); NextIteration(&item) > 0;) {
		if(*strip(item.Code) != 0 || item.Dt != 0 || item.ArID != 0)
			pTotal->Count++;
		pTotal->Amount += item.Amount;
		pTotal->Export += item.Export;
		pTotal->Excise += item.Excise;
		pTotal->Vat0Amount += item.VAT0;
		pTotal->Vat1Amount += item.VAT1;
		pTotal->Vat2Amount += item.VAT2;
		pTotal->Vat3Amount += item.VAT3;
		pTotal->Vat4Amount += item.VAT4; // @v12.2.7
		pTotal->Vat5Amount += item.VAT5; // @v12.2.7
		pTotal->Vat1Sum    += item.SVAT1;
		pTotal->Vat2Sum    += item.SVAT2;
		pTotal->Vat3Sum    += item.SVAT3;
		pTotal->Vat4Sum    += item.SVAT4; // @v12.2.7
		pTotal->Vat5Sum    += item.SVAT5; // @v12.2.7
	}
	return 1;
}

void PPViewVatBook::ViewTotal()
{
	const uint res_id = Filt.IsSimpleLedger() ? DLG_SLEDGTOTAL : DLG_VATBOOKTOTAL;
	TDialog * dlg = 0;
	if(Total.Count == 0)
		CalcTotal(&Total);
	if(CheckDialogPtrErr(&(dlg = new TDialog(res_id)))) {
		if(Filt.IsSimpleLedger()) {
			dlg->setCtrlData(CTL_SLEDGTOTAL_COUNT, &Total.Count);
			dlg->setCtrlData(CTL_SLEDGTOTAL_AMT,  &Total.Amount);
			dlg->setCtrlData(CTL_SLEDGTOTAL_AMTV, &Total.Excise);
			dlg->setCtrlData(CTL_SLEDGTOTAL_EXP,  &Total.Vat0Amount);
			dlg->setCtrlData(CTL_SLEDGTOTAL_EXPV, &Total.Export);
		}
		else {
			// @v11.3.2 @obsolete dlg->setCtrlOption(CTL_VATBOOKTOTAL_FRAME, ofFramed, 1);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_COUNT, &Total.Count);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_AMT,  &Total.Amount);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT0, &Total.Vat0Amount);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT1, &Total.Vat1Amount);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT2, &Total.Vat2Amount);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT3, &Total.Vat3Amount);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT4, &Total.Vat4Amount); // @v12.2.10
			dlg->setCtrlData(CTL_VATBOOKTOTAL_VAT5, &Total.Vat5Amount); // @v12.2.10 // @v12.3.1 @fix CTL_VATBOOKTOTAL_VAT4-->CTL_VATBOOKTOTAL_VAT5
			dlg->setCtrlData(CTL_VATBOOKTOTAL_SVAT1, &Total.Vat1Sum);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_SVAT2, &Total.Vat2Sum);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_SVAT3, &Total.Vat3Sum);
			dlg->setCtrlData(CTL_VATBOOKTOTAL_SVAT4, &Total.Vat4Sum); // @v12.2.10
			dlg->setCtrlData(CTL_VATBOOKTOTAL_SVAT5, &Total.Vat5Sum); // @v12.2.10 // @v12.3.1 @fix CTL_VATBOOKTOTAL_SVAT4-->CTL_VATBOOKTOTAL_SVAT5
			SString rate_buf;
			dlg->setStaticText(CTL_VATBOOKTOTAL_TVAT1, VatRateStr(PPObjVATBook::GetVatRate(0), rate_buf));
			dlg->setStaticText(CTL_VATBOOKTOTAL_TVAT2, VatRateStr(PPObjVATBook::GetVatRate(1), rate_buf));
			dlg->setStaticText(CTL_VATBOOKTOTAL_TVAT3, VatRateStr(PPObjVATBook::GetVatRate(2), rate_buf));
			dlg->setStaticText(CTL_VATBOOKTOTAL_TVAT4, VatRateStr(PPObjVATBook::GetVatRate(3), rate_buf)); // @v12.2.10
			dlg->setStaticText(CTL_VATBOOKTOTAL_TVAT5, VatRateStr(PPObjVATBook::GetVatRate(4), rate_buf)); // @v12.2.10
		}
		ExecViewAndDestroy(dlg);
	}
}
//
//
//
class VATBFiltDialog : public TDialog {
	DECL_DIALOG_DATA(VatBookFilt);
public:
	VATBFiltDialog(uint rezID, PPObjVATBook * pObj) : TDialog(rezID), P_VBObj(pObj)
	{
		SetupCalPeriod(CTLCAL_VATBFLT_PERIOD, CTL_VATBFLT_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		ushort v = 0;
		AddClusterAssocDef(CTL_VATBFLT_WHAT, 0,  PPVTB_SELL);
		AddClusterAssoc(CTL_VATBFLT_WHAT, 1,  PPVTB_BUY);
		AddClusterAssoc(CTL_VATBFLT_WHAT, 2,  PPVTB_SIMPLELEDGER);
		SetClusterData(CTL_VATBFLT_WHAT, Data.Kind);
		SetPeriodInput(this, CTL_VATBFLT_PERIOD, Data.Period);
		SetupObj();
		SETFLAG(v, 1, Data.Flags & VatBookFilt::fShowLink);
		SETFLAG(v, 2, Data.Flags & VatBookFilt::fShowFree);
		if(v == 3)
			v = 0;
		setCtrlData(CTL_VATBFLT_LINKT, &v);
		v = BIN(Data.Flags & Data.fPaymPeriod);
		setCtrlData(CTL_VATBFLT_PAYMPRD, &v);
		AddClusterAssoc(CTL_VATBFLT_FLAGS, 0, VatBookFilt::fShowExcluded);
		AddClusterAssoc(CTL_VATBFLT_FLAGS, 1, VatBookFilt::fOnlyEmptyExtAr);
		SetClusterData(CTL_VATBFLT_FLAGS, Data.Flags);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ushort v;
		GetClusterData(CTL_VATBFLT_WHAT, &Data.Kind);
		GetPeriodInput(this, CTL_VATBFLT_PERIOD, &Data.Period);
		getCtrlData(CTLSEL_VATBFLT_OBJ, &Data.ArticleID);
		getCtrlData(CTL_VATBFLT_LINKT, &v);
		SETFLAG(Data.Flags, Data.fShowLink, oneof2(v, 0, 1));
		SETFLAG(Data.Flags, Data.fShowFree, oneof2(v, 0, 2));
		getCtrlData(CTL_VATBFLT_PAYMPRD, &v);
		SETFLAG(Data.Flags, Data.fPaymPeriod, v);
		GetClusterData(CTL_VATBFLT_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_VATBFLT_EOBJSHEET, &Data.AccSheet2ID);
		getCtrlData(CTLSEL_VATBFLT_EXTOBJ,    &Data.Article2ID);
		getCtrlData(CTLSEL_VATBFLT_LOC, &Data.LocID);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupObj();
	void   SetupCtrls();
	PPObjVATBook * P_VBObj;
};

IMPL_HANDLE_EVENT(VATBFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_VATBFLT_WHAT)) {
		SetupObj();
		SetupCtrls();
	}
	else if(event.isClusterClk(CTL_VATBFLT_FLAGS))
		SetupCtrls();
	else if(event.isCbSelected(CTLSEL_VATBFLT_EOBJSHEET)) {
		getCtrlData(CTLSEL_VATBFLT_EOBJSHEET, &Data.AccSheet2ID);
		SetupArCombo(this, CTLSEL_VATBFLT_EXTOBJ, Data.Article2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
	}
	else
		return;
	clearEvent(event);
}

void VATBFiltDialog::SetupObj()
{
	ushort v = getCtrlUInt16(CTL_VATBFLT_WHAT);
	// @v11.4.5 const  PPID acs_id = (v == 0) ? P_VBObj->GetConfig(PPVTB_SELL).AccSheetID : P_VBObj->GetConfig(PPVTB_BUY).AccSheetID;
	const  PPID ledger_kind = (v == 0) ? PPVTB_SELL : ((v == 2) ? PPVTB_SIMPLELEDGER : PPVTB_BUY); // @v11.4.5
	const  PPID acs_id = P_VBObj->GetConfig(ledger_kind).AccSheetID; // @v11.4.5
	if(Data.ArticleID) {
		PPID   prev_ar_acs_id = 0;
		GetArticleSheetID(Data.ArticleID, &prev_ar_acs_id, 0);
		if(prev_ar_acs_id != acs_id)
			Data.ArticleID = 0;
	}
	SetupArCombo(this, CTLSEL_VATBFLT_OBJ, Data.ArticleID, OLW_LOADDEFONOPEN, acs_id, sacfDisableIfZeroSheet);
}

void VATBFiltDialog::SetupCtrls()
{
	long   kind = 0;
	GetClusterData(CTL_VATBFLT_WHAT, &kind);
	GetClusterData(CTL_VATBFLT_FLAGS, &Data.Flags);
	if(kind == PPVTB_SELL) {
    	if(Data.Flags & VatBookFilt::fOnlyEmptyExtAr)
			Data.Article2ID = Data.AccSheet2ID = 0;
		SetupPPObjCombo(this, CTLSEL_VATBFLT_EOBJSHEET, PPOBJ_ACCSHEET, Data.AccSheet2ID, 0, 0);
		SetupArCombo(this, CTLSEL_VATBFLT_EXTOBJ, Data.Article2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
		disableCtrl(CTLSEL_VATBFLT_EOBJSHEET, LOGIC(Data.Flags & VatBookFilt::fOnlyEmptyExtAr));
	}
	else {
		Data.Article2ID = Data.AccSheet2ID = 0;
		Data.Flags &= ~VatBookFilt::fOnlyEmptyExtAr;
		disableCtrls(1, CTLSEL_VATBFLT_EOBJSHEET, CTLSEL_VATBFLT_EXTOBJ, 0);
		DisableClusterItem(CTL_VATBFLT_FLAGS, 1, 1);
	}
	SetupPPObjCombo(this, CTLSEL_VATBFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	SetClusterData(CTL_VATBFLT_FLAGS, Data.Flags);
}

int PPViewVatBook::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return PPErrorZ();
	VatBookFilt * p_filt = static_cast<VatBookFilt *>(pBaseFilt);
	VBObj.ReadConfig();
	DIALOG_PROC_BODY_P2(VATBFiltDialog, DLG_VATBFLT, &VBObj, p_filt);
}

DBQuery * PPViewVatBook::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst flag_subst(2);  // @global @threadsafe
	uint   brw_id = 0;
	long   f;
	DBQ  * dbq = 0;
	DBE    dbe_ar;
	DBE    dbe_op;
	DBE    dbe_loc;
	DBE    dbe_bill_memo;
	DBE  * dbe_fix  = 0;
	DBE  * dbe_excl = 0;
	DBQuery    * q   = 0;
	VATBookTbl * vt  = new VATBookTbl;
	THROW(CheckTblPtr(vt));
	PPDbqFuncPool::InitObjNameFunc(dbe_ar,  PPDbqFuncPool::IdObjNameAr,  vt->ArID);
	PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc, vt->LocID);
	if(Filt.IsSimpleLedger()) {
		brw_id = BROWSER_SIMPLELEDGER;
		PPDbqFuncPool::InitObjNameFunc(dbe_op, PPDbqFuncPool::IdObjNameOprKind, vt->OpID);
		PPDbqFuncPool::InitObjNameFunc(dbe_bill_memo, PPDbqFuncPool::IdObjMemoBill, vt->LinkBillID);
		q = & select(
			vt->ID,        // #00
			vt->Dt,        // #01
			vt->Code,      // #02
			dbe_ar,        // #03
			dbe_op,        // #04
			dbe_bill_memo, // #05
			vt->Amount,    // #06
			vt->Excise,    // #07
			vt->VAT0,      // #08
			vt->Export,    // #09
			dbe_loc,       // #10
			0L).from(vt, 0L);
		dbq = & (vt->LineType_ == Filt.Kind);
	}
	else {
		if(Filt.Kind == PPVTB_SELL)
			brw_id = BROWSER_VATSELL;
		else if(Filt.Kind == PPVTB_BUY)
			brw_id = BROWSER_VATBUY;
		dbe_fix  = & flagtoa(vt->Flags, VATBF_FIX,      flag_subst.Get(PPTXT_FLAG_YES));
		dbe_excl = & flagtoa(vt->Flags, VATBF_EXCLUDED, flag_subst.Get(PPTXT_FLAG_YES));
		q = & select(
			vt->ID,      // #00
			vt->Dt,      // #01
			vt->Code,    // #02
			dbe_ar,      // #03
			vt->Amount,  // #04
			vt->PaymDt,  // #05
			vt->InvcDt,  // #06
			*dbe_fix,    // #07
			*dbe_excl,   // #08
			dbe_loc,     // #09
			0L).from(vt, 0L);
		dbq = & (vt->LineType_ == Filt.Kind);
	}
	if(Filt.Flags & VatBookFilt::fPaymPeriod)
		dbq = & (*dbq && daterange(vt->PaymDt, &Filt.Period));
	else
		dbq = & (*dbq && daterange(vt->Dt, &Filt.Period));
	f = (Filt.Flags & (VatBookFilt::fShowLink | VatBookFilt::fShowFree));
	if(f && f != (VatBookFilt::fShowLink | VatBookFilt::fShowFree))
		if(f & VatBookFilt::fShowLink)
			dbq = & (*dbq && vt->LinkBillID > 0L);
		else
			dbq = & (*dbq && vt->LinkBillID == 0L);
	dbq = ppcheckfiltid(dbq, vt->ArID,  Filt.ArticleID);
	dbq = ppcheckfiltid(dbq, vt->LocID,  Filt.LocID);
	if(Filt.Flags & VatBookFilt::fOnlyEmptyExtAr)
		dbq = &(*dbq && vt->Ar2ID == 0L);
	else
		dbq = ppcheckfiltid(dbq, vt->Ar2ID, Filt.Article2ID);
	if(!(Filt.Flags & VatBookFilt::fShowExcluded))
		dbq = & (*dbq && vt->Excluded == 0L);
	q->where(*dbq);
	if(Filt.ArticleID)
		q->orderBy(vt->ArID, vt->Dt, vt->LineNo, 0L);
	else if(Filt.Flags & VatBookFilt::fPaymPeriod)
		q->orderBy(vt->LineType_, vt->PaymDt, vt->Dt, 0L);
	else
		q->orderBy(vt->LineType_, vt->Dt, vt->LineNo, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete vt;
			delete dbq;
		}
	ENDCATCH
	delete dbe_fix;
	delete dbe_excl;
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewVatBook::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		PPID   temp_id;
		int    r;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				r = VBObj.Edit(&(temp_id = 0), reinterpret_cast<void *>(Filt.Kind));
				if(r == cmOK) {
					Total.Count = 0; // Reset total data
					ok = 1;
				}
				else
					ok = r ? -1 : 0;
				break;
			case PPVCMD_ADDBYSAMPLE:
				r = VBObj.AddBySample(&(temp_id = 0), id);
				if(r == cmOK) {
					Total.Count = 0; // Reset total data
					ok = 1;
				}
				else
					ok = r ? -1 : 0;
				break;
			case PPVCMD_EDITITEM:
				r = VBObj.Edit(&id, reinterpret_cast<void *>(Filt.Kind));
				if(r == cmOK) {
					Total.Count = 0; // Reset total data
					ok = 1;
				}
				else
					ok = r ? -1 : 0;
				break;
			case PPVCMD_DELETEITEM:
				ok = id ? DeleteItem(id) : -1;
				break;
			case PPVCMD_DELETEALL:
				ok = DeleteItem(0);
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_REFRESH:
				ok = AutoBuild();
				break;
		}
	}
	return ok;
}

int PPViewVatBook::Print(const void * pBrwHdr)
{
	uint   rpt_id = 0;
	if(Filt.Kind == PPVTB_BUY)
		rpt_id = REPORT_VATBOOKBUY;
	else if(Filt.Kind == PPVTB_SELL)
		rpt_id = REPORT_VATBOOKSELL;
	else
		rpt_id = REPORT_SIMPLELEDGER;
	return Helper_Print(rpt_id, 0);
}

int PPViewVatBook::DeleteItem(PPID id)
{
	int    ok = -1;
	if(id) {
		if(VBObj.RemoveObjV(id, 0, PPObject::rmv_default, 0) > 0)
			ok = 1;
	}
	else if(PPMessage(mfCritWarn, PPCFM_REMOVEALLBYFILT) == cmYes) {
		VatBookViewItem item;
		PPIDArray id_list;
		for(InitIteration(); NextIteration(&item) > 0;)
			if(!(item.Flags & VATBF_FIX))
				id_list.add(item.ID);
		{
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < id_list.getCount(); i++)
				if(VBObj.Search(id_list.at(i)) > 0)
					THROW_DB(VBObj.P_Tbl->deleteRec());
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOK
	if(ok > 0)
		Total.Count = 0; // Reset total data
	return ok;
}
//
//
//
int PPViewVatBook::_SetVATParams(VATBookTbl::Rec * pRec, const BVATAccmArray & rVatArray, double scale, bool isSelling, int slUseCostVatAddendum)
{
	int    result = 1;
	double amount = 0.0;
	double cvat = 0.0;
	double pvat = 0.0;
	BVATAccmArray temp_vata(rVatArray);
	assert(temp_vata.getCount() == rVatArray.getCount());
	temp_vata.Scale_(scale, 0);
	for(uint i = 0; i < temp_vata.getCount(); i++) {
		const BVATAccm & r_vati = temp_vata.at(i);
		double a = isSelling ? r_vati.Price : r_vati.Cost;
		double s = isSelling ? r_vati.PVATSum : r_vati.CVATSum;
		amount += a;
		double rate = 0.0;
		cvat += fabs(r_vati.CVATSum);
		pvat += fabs(r_vati.PVATSum);
		if(Filt.Kind == PPVTB_BUY) {
			if(r_vati.IsVatFree)
				pRec->Flags |= VATBF_VATFREE;
			rate = isSelling ? r_vati.PRate : r_vati.CRate;
		}
		else if(Filt.Kind == PPVTB_SELL) {
			if(MOBlk.IsVatFree_(pRec->Dt))
				pRec->Flags |= VATBF_VATFREE;
			rate = isSelling ? r_vati.PRate : r_vati.CRate;
		}
		else {
			rate = -1L;
			if(Filt.IsSimpleLedger() && (VBObj.GetConfig(PPVTB_SIMPLELEDGER).Flags & VATBCfg::hfWoTax))
				amount -= s;
		}
		if(rate == 0.0) {
			a += pRec->VAT0;
			pRec->VAT0 = a;
		}
		else {
			a -= s;
			struct _SetVATParams_InternalEntry {
				_SetVATParams_InternalEntry(double & rVat, double & rSVat) : R_Vat(rVat), R_SVat(rSVat)
				{
				}
				double & R_Vat;
				double & R_SVat;
			};
			/*non-static non-const*/_SetVATParams_InternalEntry _tab[] = {
				_SetVATParams_InternalEntry(pRec->VAT1, pRec->SVAT1),
				_SetVATParams_InternalEntry(pRec->VAT2, pRec->SVAT2),
				_SetVATParams_InternalEntry(pRec->VAT3, pRec->SVAT3),
				_SetVATParams_InternalEntry(pRec->VAT4, pRec->SVAT4),
				_SetVATParams_InternalEntry(pRec->VAT5, pRec->SVAT5),
			};
			for(uint tabidx = 0; tabidx < SIZEOFARRAY(_tab); tabidx++) {
				if(PPObjVATBook::IsVatRate(tabidx, rate)) {
					_SetVATParams_InternalEntry & r_entry = _tab[tabidx];
					a += r_entry.R_Vat;
					s += r_entry.R_SVat;
					r_entry.R_Vat = a;
					r_entry.R_SVat = s;
					break;
				}
			}
		}
	}
	if(Filt.IsSimpleLedger()) {
		uint   pos = 0;
		const  VATBCfg & r_vb_cfg = VBObj.GetConfig(PPVTB_SIMPLELEDGER);
		const  bool is_vat_free = (r_vb_cfg.List.lsearch(&pRec->OpID, &pos, CMPF_LONG)) ? LOGIC(r_vb_cfg.List.at(pos).Flags & VATBCfg::fVATFree) : false;
		amount = fabs(amount);
		if(isSelling) {
			// @v12.3.2 {
			if(slUseCostVatAddendum == 3) {
				// С 2025 года если мы - на общей схеме уплаты НДС, то доходы (так же как и расходы) учитываются без НДС
				amount -= pvat; // Запись просто без НДС
			}
			// } @v12.3.2 
		}
		else {
			if(slUseCostVatAddendum && cvat > 0.0) {
				//
				// Для получения дополнительной записи по НДС поставщика в расходах функция будет
				// вызвана повторно с параметром (slUseCostVatAddendum = 2). Но перед этим
				// вызывающая функция должна получить сигнал в виде (return = 100) для повторного вызова.
				//
				if(slUseCostVatAddendum == 1) {
					amount -= cvat; // Запись без НДС, а на следующем прогоне с параметром (slUseCostVatAddendum == 2) сформируется запись с суммой НДС.
					result = 100;
				}
				else if(slUseCostVatAddendum == 2) {
					amount = cvat;
				}
				else if(slUseCostVatAddendum == 3) {
					amount -= cvat; // Запись просто без НДС
				}
			}
		}
		pRec->Amount = amount;
		pRec->VAT0 = amount;
		if(is_vat_free) {
			pRec->Excise = 0.0;
			pRec->Export = 0.0;
		}
		else {
			pRec->Excise = amount;
			pRec->Export = amount;
		}
	}
	else
		pRec->Amount = amount;
	return result;
}

int PPViewVatBook::CheckBillRec(const AutoBuildFilt * pFilt, const BillTbl::Rec * pRec)
{
	int    ok = 1;
	THROW(!pFilt->LocID || pRec->LocID == pFilt->LocID);
	THROW(!pFilt->ObjectID || pRec->Object == pFilt->ObjectID);
	THROW(!(pFilt->Flags & abfWL) || (pRec->Flags & BILLF_WHITELABEL));
	THROW(BR2(pRec->Amount) != 0.0);
	if(Filt.Kind == PPVTB_SELL) {
		THROW(!(pFilt->Flags & abfOnlyEmptyExtAr) || !pRec->Object2);
		THROW(!pFilt->Object2ID || pRec->Object2 == pFilt->Object2ID);
	}
	CATCHZOK
	return ok;
}
//
//
//
int PPViewVatBook::RemoveZeroBillLinks(int use_ta)
{
	int    ok = -1;
	uint   i;
	VATBookTbl::Rec rec;
	VatBookViewItem item;
	PPIDArray id_list;
	for(InitIteration(); NextIteration(&item) > 0;)
		id_list.add(item.ID);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < id_list.getCount(); i++)
			if(VBObj.Search(id_list.at(i), &rec) > 0 && !(rec.Flags & VATBF_FIX) && rec.LinkBillID && P_BObj->Search(labs(rec.LinkBillID)) < 0) {
				THROW_DB(VBObj.P_Tbl->deleteRec());
				ok = 1;
			}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class AutoBuildFiltDialog : public WLDialog {
	DECL_DIALOG_DATA(PPViewVatBook::AutoBuildFilt);
public:
	explicit AutoBuildFiltDialog(PPID kind) : WLDialog(DLG_VATBABFLT, CTL_VATBABFLT_WL), Kind(kind)
	{
		SString sub_title;
		PPGetSubStr(PPTXT_LEDGERKINDTITLES, Kind, sub_title);
		setSubTitle(sub_title);
		SetupCalPeriod(CTLCAL_VATBABFLT_PERIOD, CTL_VATBABFLT_PERIOD);
		SetupCalPeriod(CTLCAL_VATBABFLT_SHIPMPRD, CTL_VATBABFLT_SHIPMPRD);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		setWL(BIN(Data.Flags & PPViewVatBook::abfWL));
		SetPeriodInput(this, CTL_VATBABFLT_PERIOD,   Data.Period);
		SetPeriodInput(this, CTL_VATBABFLT_SHIPMPRD, Data.ShipmPeriod);
		disableCtrl(CTL_VATBABFLT_SHIPMPRD, !(Data.Flags & PPViewVatBook::abfByPayment));
		SetupArCombo(this, CTLSEL_VATBABFLT_OBJECT, Data.ObjectID,  OLW_LOADDEFONOPEN, Data.AccSheetID, sacfDisableIfZeroSheet);
		if(Kind != PPVTB_SELL)
			Data.Object2ID = Data.AccSheet2ID = 0;
		SetupPPObjCombo(this, CTLSEL_VATBABFLT_SHEET2, PPOBJ_ACCSHEET, Data.AccSheet2ID, 0, 0);
		SetupArCombo(this, CTLSEL_VATBABFLT_OBJECT2, Data.Object2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
		SetupPPObjCombo(this, CTLSEL_VATBABFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
		AddClusterAssoc(CTL_VATBABFLT_FLAGS, 0, PPViewVatBook::abfOnlyEmptyExtAr);
		SetClusterData(CTL_VATBABFLT_FLAGS, Data.Flags);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		SETFLAG(Data.Flags, PPViewVatBook::abfWL, getWL());
		GetPeriodInput(this, CTL_VATBABFLT_PERIOD,   &Data.Period);
		GetPeriodInput(this, CTL_VATBABFLT_SHIPMPRD, &Data.ShipmPeriod);
		getCtrlData(CTLSEL_VATBABFLT_OBJECT, &Data.ObjectID);
		getCtrlData(CTLSEL_VATBABFLT_SHEET2, &Data.AccSheet2ID);
		getCtrlData(CTLSEL_VATBABFLT_OBJECT2, &Data.Object2ID);
		getCtrlData(CTLSEL_VATBABFLT_LOC, &Data.LocID);
		GetClusterData(CTL_VATBABFLT_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		WLDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_VATBABFLT_SHEET2)) {
			getCtrlData(CTLSEL_VATBABFLT_SHEET2, &Data.AccSheet2ID);
			SetupArCombo(this, CTLSEL_VATBABFLT_OBJECT2, Data.Object2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
		}
		else if(event.isClusterClk(CTL_VATBABFLT_FLAGS))
			SetupCtrls();
		else
			return;
		clearEvent(event);
	}
	void   SetupCtrls()
	{
		if(Kind == PPVTB_SELL) {
			GetClusterData(CTL_VATBABFLT_FLAGS, &Data.Flags);
			if(Data.Flags & PPViewVatBook::abfOnlyEmptyExtAr)
				Data.AccSheet2ID = Data.Object2ID = 0;
			SetupPPObjCombo(this, CTLSEL_VATBABFLT_SHEET2, PPOBJ_ACCSHEET, Data.AccSheet2ID, 0, 0);
			SetupArCombo(this, CTLSEL_VATBABFLT_OBJECT2, Data.Object2ID, OLW_LOADDEFONOPEN, Data.AccSheet2ID, sacfDisableIfZeroSheet);
			disableCtrl(CTLSEL_VATBABFLT_SHEET2, LOGIC(Data.Flags & PPViewVatBook::abfOnlyEmptyExtAr));
		}
		else {
			disableCtrls(1, CTLSEL_VATBABFLT_SHEET2, CTLSEL_VATBABFLT_OBJECT2, 0);
			DisableClusterItem(CTL_VATBABFLT_FLAGS, 0, 1);
		}
	}
	const  PPID Kind;
};

int PPViewVatBook::EditAutoBuildFilt(AutoBuildFilt * pFilt)
{
	DIALOG_PROC_BODY_P1(AutoBuildFiltDialog, Filt.Kind, pFilt);
}

enum {
	mrbbfIsNeg          = 0x0001,
	mrbbfIsStorno       = 0x0002,
	mrbbfFactByShipmExp = 0x0004
};

static bool FASTCALL IsSimpleLedgerRecEmpty(const VATBookTbl::Rec & r)
{
	return (r.LineType_ == PPVTB_SIMPLELEDGER && r.Amount == 0.0 && r.Excise == 0.0 && r.VAT0 == 0.0 && r.Export == 0.0);
}

int PPViewVatBook::MRBB(PPID billID, BillTbl::Rec * pPaymRec, const TaxAmountIDs * pTai, long mrbbf, PPObjBill::PplBlock * pEbfBlk, const OpEntry & rOpEntry)
{
	int    ok = 1;
	int    is_ebf_rec = 0;
	const  VATBCfg & r_cfg = VBObj.GetConfig(Filt.Kind);
	int    is_cost_vat_addendum_rec = 0;
	double scale = 1.0;
	LDATE  _dt = ZERODATE; // @v12.0.0
	LDATE  _rcpt_dt = ZERODATE; // @v12.0.0
	LDATE  _invc_dt = ZERODATE; // @v12.0.0
	LDATE  _paym_dt = ZERODATE; // @v12.0.0
	PPBillPacket pack;
	VATBookTbl::Key1 k1;
	VATBookTbl::Rec ebf_rec;
	VATBookTbl::Rec temp_rec;
	VATBookTbl::Rec sl_cost_vat_addenum_rec;
	BVATAccmArray vata((Filt.Kind == PPVTB_BUY) ? (BVATF_SUMZEROVAT | BVATF_DIFFBYCRATE) : BVATF_SUMZEROVAT);
	bool   paym_has_vat_amounts = false;
	SString bill_code;
	THROW(VBObj.IsValidKind(Filt.Kind));
	THROW(P_BObj->ExtractPacket(billID, &pack));
	const int is_selling_op = IsSellingOp(pack.Rec.OpID);
	{
		const  int    sl_use_cost_vat_addendum = Filt.IsSimpleLedger() ? (MOBlk.IsSpecialVatRate(pack.Rec.Dt, 0/*pRate*/) ? 1 : 3) : 0;
		const  double org_pack_amount = pack.Rec.Amount; // Значение номинала может измениться из-за подстановки суммы
		bool   is_subst_amount = false;
		bool   is_reckon = false; // @v12.0.0 Признак того, что запись относится к зачитывающему документу (оплате)
		double subst_amount = 0.0;
		if(pPaymRec && GetOpType(pPaymRec->OpID) == PPOPT_PAYMENT) {
			PPBillPacket paym_pack;
			THROW(P_BObj->ExtractPacket(pPaymRec->ID, &paym_pack));
			if(paym_pack.PaymBillID) {
				const bool reckon_date_by_payment = r_cfg.CheckFlag(pPaymRec->OpID, VATBCfg::fReckonDateByPayment);
				const bool vat_from_reckon = r_cfg.CheckFlag(pPaymRec->OpID, VATBCfg::fVATFromReckon);
				if(reckon_date_by_payment || vat_from_reckon) {
					PPBillPacket reckon_pack;
					if(P_BObj->ExtractPacket(paym_pack.PaymBillID, &reckon_pack) > 0) {
						if(reckon_date_by_payment) {
							_paym_dt = reckon_pack.Rec.Dt;
							_invc_dt = reckon_pack.Rec.Dt;
						}
						if(vat_from_reckon) {
							int    r;
							int    r2;
							bill_code = reckon_pack.Ext.InvoiceCode[0] ? reckon_pack.Ext.InvoiceCode : reckon_pack.Rec.Code;
							const  double mult = paym_pack.GetBaseAmount() / reckon_pack.GetBaseAmount();
							reckon_pack.Rec.Amount = BR2(paym_pack.Rec.Amount);
							{
								for(uint i = 0; i < reckon_pack.Amounts.getCount(); i++) {
									AmtEntry & r_entry = reckon_pack.Amounts.at(i);
									if(r_entry.Amt != 0.0)
										r_entry.Amt = BR2(r_entry.Amt * mult);
								}
							}
							THROW(r2 = reckon_pack.SetupVirtualTItems());
							THROW(r = vata.CalcBill(reckon_pack));
							if(r2 < 0)
								vata.Scale_(mult, 0);
							if(r > 0)
								paym_has_vat_amounts = true;
						}
					}
				}
			}
			if(!paym_has_vat_amounts && paym_pack.Amounts.HasVatSum(pTai)) {
				THROW(vata.CalcBill(paym_pack));
				paym_has_vat_amounts = true;
			}
		}
		if(Filt.IsSimpleLedger() && rOpEntry.AmtTypeID) {
			subst_amount = pack.Amounts.Get(rOpEntry.AmtTypeID, 0);
			if(subst_amount != org_pack_amount) {
				pack.Rec.Amount = subst_amount;
				is_subst_amount = true;
			}
			{
				LongArray to_rmv_pos_list;
				{
					bool   is_there_stax = false;
					PPAmountType amtt_rec;
					for(uint i = 0; i < pack.Amounts.getCount(); i++) {
						const AmtEntry & r_entry = pack.Amounts.at(i);
						if(r_entry.Amt != 0.0 && AmtTObj.Fetch(r_entry.AmtTypeID, &amtt_rec) > 0) {
							if(amtt_rec.IsTax(GTAX_VAT))
								to_rmv_pos_list.addUnique(i-1);
							else if(!is_there_stax && amtt_rec.IsTax(GTAX_SALES)) {
								to_rmv_pos_list.addUnique(i-1);
								is_there_stax = true;
							}
						}
					}
				}
				{
					to_rmv_pos_list.sort();
					uint i = to_rmv_pos_list.getCount();
					if(i) do {
						pack.Amounts.atFree(to_rmv_pos_list.get(--i));
					} while(i);
				}
			}
		}
		if(!rOpEntry.SignFilt || (rOpEntry.SignFilt < 0 && pack.Rec.Amount < 0.0) || (rOpEntry.SignFilt > 0 && pack.Rec.Amount > 0.0)) {
			VATBookTbl::Rec rec;
			const bool reckon_date_by_payment = r_cfg.CheckFlag(pack.Rec.OpID, VATBCfg::fReckonDateByPayment); // @v12.0.0
			if(!paym_has_vat_amounts) {
				if(!pack.Amounts.HasVatSum(pTai))
					THROW(pack.SetupVirtualTItems());
				THROW(vata.CalcBill(pack));
			}
			rec.LineType_ = static_cast<int16>(Filt.Kind);
			rec.LineSubType = 0;
			if(!checkdate(_dt)) {
				if(pPaymRec && checkdate(pPaymRec->Dt)) {
					_dt = pPaymRec->Dt;
				}
				else if(pack.Ext.InvoiceDate && (r_cfg.Flags & VATBCfg::hfD_InvcDate)) {
					_dt = pack.Ext.InvoiceDate;
				}
				else if(pack.Ext.InvoiceDate && (r_cfg.Flags & VATBCfg::hfD_MaxInvcBill)) {
					_dt = MAX(pack.Ext.InvoiceDate, pack.Rec.Dt);
				}
				else {
					_dt = pack.Rec.Dt;
				}
			}
			if(!checkdate(_rcpt_dt))
				_rcpt_dt = pack.Rec.Dt;
			if(!checkdate(_invc_dt)) {
				_invc_dt = pack.Ext.InvoiceDate ? pack.Ext.InvoiceDate : pack.Rec.Dt;
			}
			rec.ArID   = r_cfg.CheckFlag(pack.Rec.OpID, VATBCfg::fByExtObj) ? pack.Rec.Object2 : pack.Rec.Object;
			rec.Ar2ID  = pack.Rec.Object2;
			rec.LocID  = pack.Rec.LocID;
			if(bill_code.NotEmpty())
				bill_code.CopyTo(rec.Code, sizeof(rec.Code));
			else if(pack.Ext.InvoiceCode[0])
				STRNSCPY(rec.Code, pack.Ext.InvoiceCode);
			else
				STRNSCPY(rec.Code, pack.Rec.Code);
			if(pack.OpTypeID == PPOPT_CORRECTION) {
				STRNSCPY(rec.CBillCode, pack.Rec.Code);
				rec.CBillDt = pack.Rec.Dt;
				if(pack.P_LinkPack) {
					if(pack.P_LinkPack->Ext.InvoiceCode[0])
						STRNSCPY(rec.Code, pack.P_LinkPack->Ext.InvoiceCode);
					else
						STRNSCPY(rec.Code, pack.P_LinkPack->Rec.Code);
					_invc_dt = pack.P_LinkPack->Rec.Dt;
					if(checkdate(pack.P_LinkPack->Ext.InvoiceDate)) {
						if(r_cfg.Flags & VATBCfg::hfD_InvcDate)
							_dt = pack.P_LinkPack->Ext.InvoiceDate;
						else if(r_cfg.Flags & VATBCfg::hfD_MaxInvcBill)
							_dt = MAX(pack.P_LinkPack->Ext.InvoiceDate, pack.P_LinkPack->Rec.Dt);
					}
				}
			}
			{
				const double pack_rec_amt = BR2(pack.Rec.Amount);
				if(pPaymRec) {
					const double paym_rec_amt = BR2(pPaymRec->Amount);
					rec.OpID   = pPaymRec->OpID;
					rec.LinkBillID = pPaymRec->ID;
					if(mrbbf & mrbbfIsStorno) {
						_dt = pPaymRec->Dt;
						_invc_dt = pPaymRec->Dt;
					}
					if(!checkdate(_paym_dt))
						_paym_dt = pPaymRec->Dt;
					if(!paym_has_vat_amounts)
						scale =  paym_rec_amt / org_pack_amount;
					{
						const double final_amount = is_subst_amount ? (paym_rec_amt * pack_rec_amt / org_pack_amount) : paym_rec_amt;
						rec.Amount = final_amount;
						if(fabs(paym_rec_amt) < fabs(org_pack_amount))
							rec.Flags |= VATBF_PARTPAYM;
					}
					if(is_subst_amount)
						scale *= fabs(pack_rec_amt / org_pack_amount);
				}
				else {
					rec.LinkBillID = pack.Rec.ID;
					rec.OpID   = pack.Rec.OpID;
					if(Filt.IsSimpleLedger()) {
						if(!checkdate(_paym_dt))
							_paym_dt = pack.Rec.Dt;
					}
					const double final_amount = pack_rec_amt;
					rec.Amount = final_amount;
				}
			}
			if(mrbbf & mrbbfIsStorno)
				rec.LinkBillID = -labs(rec.LinkBillID);
			if(mrbbf & mrbbfIsNeg)
				scale = -scale;
			int skip = 0;
			if(Filt.IsSimpleLedger()) {
				int    set_vat_params_result = 0;
				if(mrbbf & mrbbfIsNeg) {
					double local_scale = scale;
					if(pEbfBlk && pPaymRec && pEbfBlk->CheckPaymOp(pPaymRec->OpID) && pEbfBlk->Period.low) {
						double cost_amt = 0.0;
						double exp_paym_amt = 0.0;
						DateRange op_period;
						op_period.Set(ZERODATE, plusdate(pEbfBlk->Period.low, -1));
						PPTransferItem * p_ti;
						for(uint i = 0; pack.EnumTItems(&i, &p_ti);) {
							if(p_ti->IsReceipt()) {
								PPObjBill::EprBlock epr;
								P_BObj->GetExpendedPartOfReceipt(p_ti->LotID, &op_period, 0, epr);
								cost_amt += epr.Amount;
								exp_paym_amt += ((pEbfBlk->Flags & pEbfBlk->fByShipment) ? epr.Expend : epr.Payout);
							}
						}
						if(cost_amt != 0.0 && exp_paym_amt != 0.0) {
							local_scale = scale * exp_paym_amt / cost_amt;
							set_vat_params_result = _SetVATParams(&rec, vata, local_scale, false/*isSelling*/, sl_use_cost_vat_addendum);
						}
						else
							skip = 1;
					}
					else {
						set_vat_params_result = _SetVATParams(&rec, vata, local_scale, (is_selling_op != 0), sl_use_cost_vat_addendum); // @v12.3.2 (is_selling_op > 0)-->(is_selling_op != 0)
					}
					rec.Amount = 0.0;   // Доход
					rec.Excise = 0.0;   // Доход для налогообложения //
					if(!skip && set_vat_params_result == 100) {
						sl_cost_vat_addenum_rec = rec;
						// @v12.0.8 @fix {
						sl_cost_vat_addenum_rec.Dt       = _dt;
						sl_cost_vat_addenum_rec.RcptDt   = _rcpt_dt;
						sl_cost_vat_addenum_rec.InvcDt   = _invc_dt;
						sl_cost_vat_addenum_rec.PaymDt   = _paym_dt;
						// } @v12.0.8 @fix
						sl_cost_vat_addenum_rec.LineSubType = 1;
						_SetVATParams(&sl_cost_vat_addenum_rec, vata, local_scale, false/*isSelling*/, 2/*slUseCostVatAddendum*/);
						sl_cost_vat_addenum_rec.Amount = 0.0; // Доход
						sl_cost_vat_addenum_rec.Excise = 0.0; // Доход для налогообложения //
						is_cost_vat_addendum_rec = 1;
					}
				}
				else {
					set_vat_params_result = _SetVATParams(&rec, vata, scale, (is_selling_op != 0), sl_use_cost_vat_addendum);
					rec.VAT0 = 0.0;     // Расход
					rec.Export = 0.0;   // Расход для налогообложения //
					double amount = rec.Amount;
					if(mrbbf & mrbbfFactByShipmExp) {
						rec.Amount = 0.0;   // Доход
						rec.Excise = 0.0;   // Доход для налогообложения //
					}
					if(pEbfBlk && amount != 0.0) {
						PPTransferItem * p_ti;
						double exp_amount = 0.0;
						double exp_total_amount = 0.0;
						if(!(pEbfBlk->Flags & pEbfBlk->fByShipment) || mrbbf & mrbbfFactByShipmExp) {
							for(uint i = 0; pack.EnumTItems(&i, &p_ti);) {
								double part = 1.0;
								if(P_BObj->GetPayoutPartOfLot(p_ti->LotID, *pEbfBlk, &part) > 0) {
									const double cost = fabs(p_ti->Cost * p_ti->SQtty(pack.Rec.OpID));
									exp_total_amount += cost;
									exp_amount += cost * part;
								}
							}
						}
						if(exp_amount != 0.0 && exp_total_amount != 0.0) {
							MEMSZERO(ebf_rec);
							ebf_rec = rec;
							// @v12.0.6 {
							ebf_rec.Dt       = _dt;
							ebf_rec.RcptDt   = _rcpt_dt;
							ebf_rec.InvcDt   = _invc_dt;
							ebf_rec.PaymDt   = _paym_dt;
							// } @v12.0.6 
							const double temp_scale = -fabs(scale * exp_amount / exp_total_amount);
							set_vat_params_result = _SetVATParams(&ebf_rec, vata, temp_scale, false/*isSelling*/, sl_use_cost_vat_addendum);
							ebf_rec.Amount = 0.0; // Доход
							ebf_rec.Excise = 0.0; // Доход для налогообложения //
							if(set_vat_params_result == 100) {
								//
								// По сигналу (set_vat_params_result == 100) вставляем дополнительную запись с суммой НДС
								// (функция _SetVATParams с аргументом slUseCostVatAddendum==2)
								//
								sl_cost_vat_addenum_rec = ebf_rec;
								sl_cost_vat_addenum_rec.LineSubType = 1;
								_SetVATParams(&sl_cost_vat_addenum_rec, vata, temp_scale, false/*isSelling*/, 2/*slUseCostVatAddendum*/);
								sl_cost_vat_addenum_rec.Amount = 0.0; // Доход
								sl_cost_vat_addenum_rec.Excise = 0.0; // Доход для налогообложения //
								is_cost_vat_addendum_rec = 2;
							}
							is_ebf_rec = 1;
						}
					}
				}
			}
			else
				_SetVATParams(&rec, vata, scale, (is_selling_op > 0), 0/*slUseCostVatAddendum*/);
			rec.Dt       = _dt;
			rec.RcptDt   = _rcpt_dt;
			rec.InvcDt   = _invc_dt;
			rec.PaymDt   = _paym_dt;
			if(!skip) {
				bool   is_fixed = false;
				long   rbydate = 0;
				const  PPID link_id = rec.LinkBillID;
				MEMSZERO(k1);
				k1.LinkBillID = link_id;
				k1.LineType_ = static_cast<int16>(Filt.Kind);
				{
					PPTransaction tra(1);
					THROW(tra);
					if(!AbBillList.Has(link_id)) {
						for(int sp = spGe; !is_fixed && VBObj.P_Tbl->searchForUpdate(1, &k1, sp) && k1.LinkBillID == link_id && k1.LineType_ == Filt.Kind; sp = spGt) {
							VBObj.P_Tbl->copyBufTo(&temp_rec);
							if(!(temp_rec.Flags & VATBF_FIX) && !temp_rec.Excluded) {
								if(!rbydate && temp_rec.Dt == rec.Dt)
									rbydate = temp_rec.LineNo;
								THROW_DB(VBObj.P_Tbl->deleteRec()); // @sfu
							}
							else
								is_fixed = true;
						}
						THROW_DB(BTROKORNFOUND);
					}
					if(!is_fixed) {
						bool   rec_added = false;
						if(rbydate)
							rec.LineNo = rbydate;
						else
							THROW(IncDateKey(VBObj.P_Tbl, 2, rec.Dt, &rec.LineNo));
						if(!(mrbbf & mrbbfFactByShipmExp)) {
							if(!IsSimpleLedgerRecEmpty(rec)) {
								THROW_DB(VBObj.P_Tbl->insertRecBuf(&rec));
								rec_added = true;
							}
						}
						if(is_ebf_rec) {
							if(!IsSimpleLedgerRecEmpty(ebf_rec)) {
								THROW(IncDateKey(VBObj.P_Tbl, 2, ebf_rec.Dt, &ebf_rec.LineNo));
								THROW_DB(VBObj.P_Tbl->insertRecBuf(&ebf_rec));
								rec_added = true;
								if(is_cost_vat_addendum_rec == 2) {
									if(!IsSimpleLedgerRecEmpty(sl_cost_vat_addenum_rec)) {
										THROW(IncDateKey(VBObj.P_Tbl, 2, sl_cost_vat_addenum_rec.Dt, &sl_cost_vat_addenum_rec.LineNo));
										THROW_DB(VBObj.P_Tbl->insertRecBuf(&sl_cost_vat_addenum_rec));
										rec_added = true;
									}
								}
							}
						}
						else if(is_cost_vat_addendum_rec == 1) {
							if(!IsSimpleLedgerRecEmpty(sl_cost_vat_addenum_rec)) {
								THROW(IncDateKey(VBObj.P_Tbl, 2, sl_cost_vat_addenum_rec.Dt, &sl_cost_vat_addenum_rec.LineNo));
								THROW_DB(VBObj.P_Tbl->insertRecBuf(&sl_cost_vat_addenum_rec));
								rec_added = true;
							}
						}
						if(rec_added)
							AbBillList.Add(static_cast<uint>(link_id));
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewVatBook::ProcessOp2(const OpEntryVector & rList, uint listIdx, const OpEntryVector * pNegList, const AutoBuildFilt * pFilt, int mode, PPObjBill::PplBlock * pEbfBlk)
{
	int    ok = 1;
	PPOprKind op_rec;
	BillTbl::Rec bill_rec;
	SString wait_msg;
	const  OpEntry & r_entry = rList.at(listIdx);
	GetOpData(r_entry.OpID, &op_rec);
	const  bool is_paym = (op_rec.OpTypeID == PPOPT_PAYMENT);
	const  bool is_ret  = (op_rec.OpTypeID == PPOPT_GOODSRETURN);
	const  bool is_neg  = (pNegList && pNegList->Search(r_entry.OpID, r_entry.AmtTypeID, 0));
	const  bool do_storno = (mode < 0);
	if(mode < 0)
		mode = (mode == -1000) ? 0 : -mode;
	const  int storn_reckon = (!do_storno && VBObj.GetConfig(Filt.Kind).Flags & VATBCfg::hfDontStornReckon) ? 0 : 1;
	TaxAmountIDs tai;
	AmtTObj.GetTaxAmountIDs(tai, 1);
	TaxAmountIDs * p_tai = tai.HasAnyVatAmountTypes() ? &tai : 0;
	for(DateIter di(&pFilt->Period); P_BObj->P_Tbl->EnumByOpr(r_entry.OpID, &di, &bill_rec) > 0;)
		if(is_paym || CheckBillRec(pFilt, &bill_rec)) {
			PPWaitMsg(PPObjBill::MakeCodeString(&bill_rec, 1, wait_msg));
			int    r = -1;
			PPID   bill_id = bill_rec.ID;
			PPID   paym_bill_id = 0;
			PPID   pool_id = 0;
			BillTbl::Rec temp_rec;
			BillTbl::Rec * p_paym_rec = 0;
			if(is_paym) {
				p_paym_rec = &bill_rec;
				paym_bill_id = bill_id;
				if(mode == 2) { // Reckoning
					if(P_BObj->IsMemberOfPool(paym_bill_id, PPASS_PAYMBILLPOOL, &pool_id) > 0)
						bill_id = pool_id;
					else
						continue;
				}
				else
					bill_id = bill_rec.LinkBillID;
				if(BR2(bill_rec.Amount) == 0.0)
					continue;
				THROW(r = P_BObj->P_Tbl->Search(bill_id, &temp_rec));
				if(r < 0) {
					PPError(PPERR_ZEROLINKPAYM, wait_msg);
					PPWaitStart();
					continue;
				}
				else if(!CheckBillRec(pFilt, &temp_rec))
					continue;
				else if(!pFilt->ShipmPeriod.CheckDate(temp_rec.Dt))
					continue;
			}
			else if(is_ret && bill_rec.LinkBillID) {
				THROW(r = P_BObj->P_Tbl->Search(bill_rec.LinkBillID, &temp_rec));
				if(r > 0)
					if(pFilt->Flags & (pFilt->ExtPeriod.CheckDate(temp_rec.Dt) ? abfByPaymAtPrd : abfByPayment))
						continue;
			}
			if(mode != 3 && !pFilt->ExtPeriod.IsZero()) {
				if(mode) {
					if(pFilt->Flags & (pFilt->ExtPeriod.CheckDate(temp_rec.Dt) ? abfByPayment : abfByPaymAtPrd))
						continue;
				}
				else if(CheckOpFlags(r_entry.OpID, OPKF_NEEDPAYMENT)) {
					if(pFilt->Flags & (pFilt->ExtPeriod.CheckDate(bill_rec.Dt) ? abfByPaymAtPrd : abfByPayment))
						continue;
				}
			}
			{
				long mrbbf = 0;
				if(mode == 3)
					mrbbf = mrbbfFactByShipmExp;
				else if(is_neg)
					mrbbf = mrbbfIsNeg;
				THROW(MRBB(bill_id, p_paym_rec, p_tai, mrbbf, pEbfBlk, r_entry));
				if(storn_reckon && (mode == 1 || do_storno) && is_paym && P_BObj->IsMemberOfPool(paym_bill_id, PPASS_PAYMBILLPOOL, &pool_id) > 0) {
					if(P_BObj->Search(pool_id, &temp_rec) > 0 && rList.Search(temp_rec.OpID, 0, 0) && p_paym_rec->Dt >= temp_rec.Dt) {
						THROW(MRBB(pool_id, p_paym_rec, p_tai, mrbbfIsNeg | mrbbfIsStorno, pEbfBlk, r_entry));
					}
				}
			}
		}
	CATCHZOK
	return ok;
}

void PPViewVatBook::ConvertOpList(const VATBCfg & rCfg, PPIDArray & rList)
{
	PPIDArray temp_op_list;
	temp_op_list.addUnique(&rList);
	for(uint i = 0; i < rCfg.List.getCount(); i++) {
		if(rCfg.List.at(i).Flags & VATBCfg::fExclude)
			temp_op_list.removeByID(rCfg.List.at(i).OpID);
	}
	rList = temp_op_list;
}

int PPViewVatBook::OpEntryVector::RemoveExcludedByConfig(const VATBCfg & rCfg)
{
	const  uint org_count = getCount();
	if(org_count) {
		for(uint i = 0; i < rCfg.List.getCount(); i++) {
			const VATBCfg::Item & r_cfg_item = rCfg.List.at(i);
			if(r_cfg_item.Flags & VATBCfg::fExclude) {
				if(r_cfg_item.MainAmtTypeID) {
					uint j = getCount();
					if(j) do {
						const OpEntry & r_entry = at(--j);
						if(r_entry.OpID == r_cfg_item.OpID && r_entry.AmtTypeID == r_cfg_item.MainAmtTypeID)
							atFree(j);
					} while(j);
				}
				else {
					uint j = getCount();
					if(j) do {
						const OpEntry & r_entry = at(--j);
						if(r_entry.OpID == r_cfg_item.OpID)
							atFree(j);
					} while(j);
				}
			}
		}
	}
	assert(org_count >= getCount());
	return (org_count > getCount()) ? +1 : -1;
}

PPViewVatBook::OpEntryVector::OpEntryVector() : TSVector <OpEntry>()
{
}

int PPViewVatBook::OpEntryVector::Search(PPID opID, PPID amtTypeID, uint * pIdx) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < getCount(); i++) {
		const OpEntry & r_entry = at(i);
		if(r_entry.OpID == opID && r_entry.AmtTypeID == amtTypeID) {
			ASSIGN_PTR(pIdx, i);
			ok = 1;
		}
	}
	return ok;
}

int PPViewVatBook::OpEntryVector::AddEntry(PPID opID, PPID amtTypeID, int signFilt)
{
	int    ok = -1;
	uint   pos = 0;
	if(Search(opID, amtTypeID, &pos)) {
		// @v11.1.3 {
		OpEntry & r_entry = at(pos);
		assert(opID == r_entry.OpID);
		assert(amtTypeID == r_entry.AmtTypeID);
		int ex_sign_filt = oneof2(r_entry.SignFilt, -1, +1) ? r_entry.SignFilt : 0;
		if(ex_sign_filt == 0 && oneof2(signFilt, -1, +1)) {
			r_entry.SignFilt = signFilt;
		}
		// } @v11.1.3 
	}
	else {
		OpEntry new_entry;
		new_entry.OpID = opID;
		new_entry.AmtTypeID = amtTypeID;
		new_entry.SignFilt = oneof2(signFilt, -1, +1) ? signFilt : 0;
		insert(&new_entry);
		ok = 1;
	}
	return ok;
}

int PPViewVatBook::OpEntryVector::AddOpList(const LongArray & rOpList, PPID amtTypeID, int signFilt)
{
	int    ok = -1;
	for(uint i = 0; i < rOpList.getCount(); i++) {
		const  PPID op_id = rOpList.get(i);
		if(AddEntry(op_id, amtTypeID, signFilt) > 0)
			ok = 1;
	}
	return ok;
}

int PPViewVatBook::OpEntryVector::RemoveByAnotherList(const OpEntryVector & rOtherList)
{
	int    ok = -1;
	for(uint i = 0; i < rOtherList.getCount(); i++) {
		uint idx = 0;
		const OpEntry & r_other_entry = rOtherList.at(i);
		if(Search(r_other_entry.OpID, r_other_entry.AmtTypeID, &idx)) {
			atFree(idx);
		}
	}
	return ok;
}

void PPViewVatBook::ResetMainOrgBlock()
{
	MOBlk.Z();
}
 
int PPViewVatBook::AutoBuild()
{
	int    ok = 1;
	//
	// Способ формирования книги:
	//   0 - по отгрузке
	//   1 - по оплате (основная установка)
	//   2 - по оплате (и основная и расширенная установки)
	//
	int    by_payments = 0;
	//
	AbBillList.Clear();
	AutoBuildFilt flt;
	const VATBCfg & r_cfg = VBObj.GetConfig(Filt.Kind);
	THROW(VBObj.IsValidKind(Filt.Kind));
	if(r_cfg.AcctgBasis == INCM_BYPAYMENT || (r_cfg.AcctgBasis == INCM_DEFAULT && (Filt.Kind == PPVTB_SIMPLELEDGER || CConfig.IncomeCalcMethod == INCM_BYPAYMENT)))
		by_payments = 1;
	MEMSZERO(flt);
	flt.AccSheetID  = r_cfg.AccSheetID;
	flt.Period      = Filt.Period;
	SETFLAG(flt.Flags, abfByPayment, by_payments);
	if(EditAutoBuildFilt(&flt) > 0) {
		uint   i;
		PPObjBill::PplBlock ebf_blk(flt.Period, 0, 0);
		PPID   main_org_id = 0;
		PPObjOprKind op_obj;
		OpEntryVector inc_op_list_; // Список основных операций для включения в книгу
		OpEntryVector paym_op_list_; // Список операций оплат
		OpEntryVector neg_op_list_;  // Список основных операций для включения в книгу, которые должны сторнироваться
		OpEntryVector reckon_op_list_; // Список зачетных операций
		OpEntryVector factbyshipm_exp_op_list_;
		OpEntryVector as_paym_op_list_; // Список операций, имеющих признак VATBCfg::fAsPayment. Для таких операций по связанному документу формируется сторнирующая запись
		PPWaitStart();
		MOBlk.Init(); // @v12.3.2
		/* @v12.3.2 
		IsMainOrgVatFree = 0;
		if(GetMainOrgID(&main_org_id) > 0) {
			PersonTbl::Rec psn_rec;
			if(PsnObj.Fetch(main_org_id, &psn_rec) > 0 && psn_rec.Flags & PSNF_NOVATAX)
				IsMainOrgVatFree = 1;
		}
		*/
		THROW(RemoveZeroBillLinks(1));
		flt.ExtPeriod = r_cfg.Period;
		//
		if(!flt.ExtPeriod.IsZero()) {
			if(r_cfg.AcctgBasisAtPeriod == INCM_BYPAYMENTINPERIOD && Filt.Kind == PPVTB_SIMPLELEDGER) {
				flt.Flags |= abfByPaymAtPrd;
				by_payments = 1;
			}
			else if(r_cfg.AcctgBasisAtPeriod == INCM_BYPAYMENT) {
				if(by_payments == 0) {
					flt.Flags |= abfByPaymAtPrd;
					by_payments = 1;
				}
				else if(by_payments == 1)
					by_payments = 2;
			}
		}
		if(by_payments != 1)
			flt.ExtPeriod.Z();
		//
		// Если книга формируется по отгрузке (не по оплатам), то формирование списка операций простое.
		//
		if(!by_payments) {
			PPIDArray inner_op_list;
			for(i = 0; i < r_cfg.List.getCount(); i++) {
				const VATBCfg::Item & r_item = r_cfg.List.at(i);
				const  PPID base_op_id = r_item.OpID;
				if(!(r_item.Flags & VATBCfg::fExclude)) {
					inner_op_list.clear();
					op_obj.GetCorrectionOpList(base_op_id, &inner_op_list);
					inner_op_list.atInsert(0, &base_op_id);
					for(uint inneropidx = 0; inneropidx < inner_op_list.getCount(); inneropidx++) {
						const  PPID op_id = inner_op_list.get(inneropidx);
						if(Filt.IsSimpleLedger() && r_item.Flags & VATBCfg::fExpendByFact) {
							ebf_blk.AddOp(op_id);
							if(r_item.Flags & VATBCfg::fNegative) {
								inc_op_list_.AddEntry(op_id, r_item.MainAmtTypeID, r_item.SignFilt);
								neg_op_list_.AddEntry(op_id, r_item.MainAmtTypeID, r_item.SignFilt);
							}
						}
						else {
							inc_op_list_.AddEntry(op_id, r_item.MainAmtTypeID, r_item.SignFilt);
							if(r_item.Flags & VATBCfg::fNegative) {
								neg_op_list_.AddEntry(op_id, r_item.MainAmtTypeID, r_item.SignFilt);
							}
							if(r_item.Flags & VATBCfg::fAsPayment) {
								as_paym_op_list_.AddEntry(op_id, r_item.MainAmtTypeID, r_item.SignFilt);
							}
						}
					}
				}
			}
		}
		else {
			//
			// Для книги по оплатам формируем список операций в два этапа
			//
			// Формируем список операций оплат по документам, требующим оплаты
			//
			PPIDArray inner_op_list;
			for(i = 0; i < r_cfg.List.getCount(); i++) {
				const VATBCfg::Item & r_item = r_cfg.List.at(i);
				const  PPID base_op_id = r_item.OpID;
				const  PPID local_amt_type_id = Filt.IsSimpleLedger() ? r_item.MainAmtTypeID : 0;
				if(!(r_item.Flags & VATBCfg::fExclude)) {
					inner_op_list.clear();
					op_obj.GetCorrectionOpList(base_op_id, &inner_op_list);
					inner_op_list.atInsert(0, &base_op_id);
					for(uint inneropidx = 0; inneropidx < inner_op_list.getCount(); inneropidx++) {
						const  PPID op_id = inner_op_list.get(inneropidx);
						if(CheckOpFlags(base_op_id, OPKF_NEEDPAYMENT)) {
							PPIDArray temp_op_list;
							THROW(op_obj.GetPaymentOpList(op_id, &temp_op_list));
							if(Filt.IsSimpleLedger() && r_item.Flags & VATBCfg::fExpendByFact) {
								ebf_blk.AddOp(op_id);
								ebf_blk.AddPaymOpList(temp_op_list);
								if(r_item.Flags & VATBCfg::fNegative) {
									paym_op_list_.AddOpList(temp_op_list, local_amt_type_id, r_item.SignFilt);
									neg_op_list_.AddOpList(temp_op_list, local_amt_type_id, r_item.SignFilt);
									//paym_op_list.addUnique(&temp_op_list);
									//neg_op_list.addUnique(&temp_op_list);
									if(r_item.Flags & VATBCfg::fFactByShipment)
										ebf_blk.Flags |= ebf_blk.fByShipment;
								}
							}
							else {
								paym_op_list_.AddOpList(temp_op_list, local_amt_type_id, r_item.SignFilt);
								//paym_op_list.addUnique(&temp_op_list);
								if(Filt.IsSimpleLedger()) {
									if(r_item.Flags & VATBCfg::fNegative) {
										neg_op_list_.AddOpList(temp_op_list, local_amt_type_id, r_item.SignFilt);
										//neg_op_list.addUnique(&temp_op_list);
									}
									else {
										factbyshipm_exp_op_list_.AddEntry(op_id, local_amt_type_id, r_item.SignFilt);
										//factbyshipm_exp_op_list.addUnique(op_id);
									}
								}
								if(CheckOpFlags(op_id, OPKF_RECKON)) {
									PPReckonOpEx rox;
									THROW(op_obj.GetReckonExData(op_id, &rox));
									reckon_op_list_.AddOpList(rox.OpList, local_amt_type_id, r_item.SignFilt);
									//reckon_op_list.add(&rox.OpList);
								}
							}
						}
						//
						// [01] Для того, чтобы следующий блок имел бы ограниченный эффект только на
						// книгу доходов/расходов устанавливаем соответствующий if().
						// Тем не менее, я думаю, что этот блок нужен и для книг продаж/покупок
						//
						else if(Filt.IsSimpleLedger()) {
							if(r_item.Flags & VATBCfg::fExpendByFact)
								ebf_blk.AddOp(op_id);
							else {
								inc_op_list_.AddEntry(op_id, local_amt_type_id, r_item.SignFilt);
								//inc_op_list.addUnique(op_id);
								if(r_item.Flags & VATBCfg::fNegative) {
									neg_op_list_.AddEntry(op_id, local_amt_type_id, r_item.SignFilt);
									//neg_op_list.addUnique(op_id);
								}
								else {
									factbyshipm_exp_op_list_.AddEntry(op_id, local_amt_type_id, r_item.SignFilt);
									//factbyshipm_exp_op_list.addUnique(op_id);
								}
							}
						}
					}
				}
			}
			//
			// Формируем список операций, перечисленных в конфигурации, и не требующих оплаты
			//
			// { см коммент [01] выше
			if(Filt.IsSimpleLedger()) {
				inc_op_list_.RemoveByAnotherList(paym_op_list_);
			}
			else {
			// }
				for(i = 0; i < r_cfg.List.getCount(); i++) {
					const VATBCfg::Item & r_item = r_cfg.List.at(i);
					if(!(r_item.Flags & VATBCfg::fExclude)) {
						if(GetOpType(r_item.OpID) != PPOPT_PAYMENT || !paym_op_list_.Search(r_item.OpID, r_item.MainAmtTypeID, 0)) {
							inc_op_list_.AddEntry(r_item.OpID, r_item.MainAmtTypeID, r_item.SignFilt);
						}
						if(r_item.Flags & VATBCfg::fNegative) {
							neg_op_list_.AddEntry(r_item.OpID, r_item.MainAmtTypeID, r_item.SignFilt);
						}
						/*if(GetOpType(r_item.OpID) != PPOPT_PAYMENT || !paym_op_list.lsearch(r_item.OpID))
							inc_op_list.addUnique(r_item.OpID);
						if(r_item.Flags & VATBCfg::fNegative)
							neg_op_list.addUnique(r_item.OpID);*/
					}
				}
			}
		}
		Total.Count = 0; // Reset total data
		{
			PPObjBill::PplBlock * p_ebf_blk = ebf_blk.OpList.getCount() ? &ebf_blk : 0;
			inc_op_list_.RemoveExcludedByConfig(r_cfg);
			//ConvertOpList(r_cfg, inc_op_list);
			for(i = 0; i < inc_op_list_.getCount(); i++) {
				const  PPID op_id = inc_op_list_.at(i).OpID;
				const  int by_paym_param = r_cfg.CheckFlag(op_id, VATBCfg::fAsPayment) ? (r_cfg.CheckFlag(op_id, VATBCfg::fVATFromReckon) ? -2 : -1) : 0;
				/*PPID   main_amt_type_id = 0;
				if(Filt.IsSimpleLedger()) {
					for(uint j = 0; !main_amt_type_id && j < r_cfg.List.getCount(); j++) {
						const VATBCfg::Item & r_item = r_cfg.List.at(j);
						if(!(r_item.Flags & VATBCfg::fExclude) && r_item.OpID == op_id && r_item.MainAmtTypeID)
							main_amt_type_id = r_item.MainAmtTypeID;
					}
				}*/
				THROW(ProcessOp2(inc_op_list_, i, &neg_op_list_, &flt, by_paym_param, p_ebf_blk));
			}
			paym_op_list_.RemoveExcludedByConfig(r_cfg);
			for(i = 0; i < paym_op_list_.getCount(); i++)
				THROW(ProcessOp2(paym_op_list_, i, &neg_op_list_, &flt, 1, p_ebf_blk));
			reckon_op_list_.RemoveExcludedByConfig(r_cfg);
			for(i = 0; i < reckon_op_list_.getCount(); i++)
				THROW(ProcessOp2(reckon_op_list_, i, &neg_op_list_, &flt, 2, p_ebf_blk));
			if(p_ebf_blk && p_ebf_blk->Flags & p_ebf_blk->fByShipment) {
				factbyshipm_exp_op_list_.RemoveExcludedByConfig(r_cfg);
				for(i = 0; i < factbyshipm_exp_op_list_.getCount(); i++)
					THROW(ProcessOp2(factbyshipm_exp_op_list_, i, 0, &flt, 3, p_ebf_blk));
			}
		}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}
//
// Implementation of PPALDD_VatBook
//
PPALDD_CONSTRUCTOR(VatBook)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(VatBook) { Destroy(); }

int PPALDD_VatBook::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(VatBook, rsrv);
	H.FltKind = p_filt->Kind;
	H.FltBeg  = p_filt->Period.low;
	H.FltEnd  = p_filt->Period.upp;
	H.HVat1Rate = PPObjVATBook::GetVatRate(0);
	H.HVat2Rate = PPObjVATBook::GetVatRate(1);
	H.HVat3Rate = PPObjVATBook::GetVatRate(2);
	H.fByPayment        = BIN(p_filt->Flags & VatBookFilt::fPaymPeriod);
	H.fShowLinkedOnly   = BIN(p_filt->Flags & VatBookFilt::fShowLink && !(p_filt->Flags & VatBookFilt::fShowFree));
	H.fShowUnlinkedOnly = BIN(p_filt->Flags & VatBookFilt::fShowFree && !(p_filt->Flags & VatBookFilt::fShowLink));
	H.fShowExcluded     = BIN(p_filt->Flags & VatBookFilt::fShowExcluded);
	H.Counter = 0;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_VatBook::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(VatBook);
}

int PPALDD_VatBook::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(VatBook);
	I.ID       = item.ID;
	I.DtLineNo = item.LineNo;
	STRNSCPY(I.Code, item.Code);
	if(*strip(item.Code) != 0 || item.Dt || item.ArID)
		H.Counter++;
	I.LineNo   = H.Counter;
	I.Dt       = item.Dt;
	I.InvcDate = item.InvcDt;
	I.PaymDate = item.PaymDt;
	I.RcptDate = item.RcptDt;
	I.ArticleID  = item.ArID;
	I.PersonID   = ObjectToPerson(item.ArID);
	I.LinkBillID = labs(item.LinkBillID);
	I.OpID       = item.OpID;
	STRNSCPY(I.CLB, item.CLB);
	if(item.ManufCountry[0])
		STRNSCPY(I.ManufCountry, item.ManufCountry);
	else {
		SString temp_buf;
		PPObjWorld::GetNativeCountryName(temp_buf);
		STRNSCPY(I.ManufCountry, temp_buf);
	}
	I.Amount     = item.Amount;
	I.Excise     = item.Excise;
	I.Export     = item.Export;
	I.Vat0Amount = item.VAT0;
	I.Vat1Amount = item.VAT1;
	I.Vat2Amount = item.VAT2;
	I.Vat3Amount = item.VAT3;
	I.Vat4Amount = item.VAT4; // @v12.3.1
	I.Vat5Amount = item.VAT5; // @v12.3.1
	I.Vat1Sum    = item.SVAT1;
	I.Vat2Sum    = item.SVAT2;
	I.Vat3Sum    = item.SVAT3;
	I.Vat4Sum    = item.SVAT4; // @v12.3.1
	I.Vat5Sum    = item.SVAT5; // @v12.3.1
	I.Vat1Rate   = PPObjVATBook::GetVatRate(0); // 10
	I.Vat2Rate   = PPObjVATBook::GetVatRate(1); // 18
	I.Vat3Rate   = PPObjVATBook::GetVatRate(2); // 20
	I.Vat4Rate   = PPObjVATBook::GetVatRate(3); // 5   // @v12.3.1
	I.Vat5Rate   = PPObjVATBook::GetVatRate(4); // 7   // @v12.3.1
	I.fExcluded  = item.Excluded;
	I.fVatFree   = BIN(item.Flags & VATBF_VATFREE);
	I.fFixed     = BIN(item.Flags & VATBF_FIX);
	I.fSlVatAddendum = BIN(item.LineSubType == 1);
	I.CBillDt    = item.CBillDt;
	STRNSCPY(I.CBillCode, item.CBillCode);
	I.VatFreeAmount = item.VAT0;
	I.Vat0Amount = 0.0;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_VatBook::Destroy() { DESTROY_PPVIEW_ALDD(VatBook); }
//
//
//
int PPViewVatBook::GetNalogRuOpIdent(const VatBookViewItem & rItem, SString & rBuf)
{
	// @todo Необходима полная реализация.
	/*
	Описание | Для плательщиков НДС | Для неплательщиков НДС
	----------------------------------------------------------------------
	Приход товара от поставщика                                      01 01
	Продажа товаров покупателю                                       01 01
	Авансы, полученные от покупателей                                02 02
	Авансы, уплаченные поставщику (если предусмотрено договором)     02 02
	Возврат товаров от покупателя                                    03 03
	Возврат товаров поставщику                                       03 03
	Продажа товаров покупателям, не плательщиками НДС(наличие согл.) 26 26
	Возврат товаров от покупателя неплательщика НДС (наличие согл.)  16 16
	Получение коррект счет-фактуры (уменьшение цены, кол-ва)         18 18
	Выставление коррект счет-фактуры (уменьшение цены, кол-ва)       18 18
	Возврат авансовых платежей покупателю                            22 22
	Зачет авансовых платежей в счет отгрузки                         22 22
	*/
	rBuf = rItem.TaxOpCode;
	long _c = rBuf.ToLong();
	if(rBuf.Len() != 2 || _c <= 0 || _c >= 100) {
		rBuf.Z();
		if(rItem.OpID) {
			long exp_symb_val = 0;
			if(!TaxOpSymbAssoc.Search(rItem.OpID, &exp_symb_val, 0)) {
				PPObjOprKind op_obj;
				PPOprKindPacket op_pack;
				if(op_obj.GetPacket(rItem.OpID, &op_pack) > 0) {
					SString temp_buf;
					op_pack.GetExtStrData(OPKEXSTR_EXPSYMB, temp_buf);
					_c = temp_buf.ToLong();
					if(temp_buf.Len() != 2 || _c <= 0 || _c >= 100)
						exp_symb_val = 0;
					else {
						exp_symb_val = _c;
						rBuf.CatLongZ(exp_symb_val, 2);
					}
				}
				else
					exp_symb_val = 0;
				TaxOpSymbAssoc.Add(rItem.OpID, exp_symb_val, 0, 0);
			}
			else if(exp_symb_val) {
				rBuf.CatLongZ(exp_symb_val, 2);
			}
		}
		rBuf.SetIfEmpty("01"); // Если в записи книге и в виде операции не указан валидный код операции - определяем его самостроятельно.
	}
	return 1;
}

int PPViewVatBook::Export()
{
	int    ok = 1;
	uint   i;
	SString temp_buf;
	SString path;
	SString data_name;
	SString head_name;
	SString suffix;
	SString left;
	SString out_file_name;
	SString id_file;
	SString ledger_title; // @v11.3.2
	SString ledger_line_title; // @v11.3.2
	DocNalogRu_Generator g;
	//xmlTextWriter * p_writer = 0;
	const LDATE _cdate = getcurdate_();
	const long  _uniq_suffix = 1;
	// @v11.3.2 const char * p_ledger_title = 0;
	// @v11.3.2 const char * p_ledger_line_title = 0;
	PPID   main_org_id = GetMainOrgID();
	{
		SString sender_ident;
		SString rcvr_ident;
		if(main_org_id) {
			RegisterTbl::Rec reg_rec;
			if(PsnObj.GetRegister(main_org_id, PPREGT_TPID, &reg_rec) > 0) {
				(sender_ident = reg_rec.Num).Strip();
				if(sender_ident.Len() == 12) {
					;
				}
				else if(sender_ident.Len() == 10) {
					if(PsnObj.GetRegister(main_org_id, PPREGT_KPP, &reg_rec) > 0) {
						(temp_buf = reg_rec.Num).Strip();
                        sender_ident.Cat(temp_buf);
					}
					else {
						// @todo Вывод информации о недопустимой комбинации ИНН/КПП
						sender_ident.CatCharN('0', 9);
					}
				}
				else {
					// @todo Вывод информации о недопустимой комбинации ИНН/КПП
					sender_ident.Z().CatCharN('0', 12);
				}
			}
			{
				//
				// Формат представления идентификатора получатея: КОДПОЛУЧАТЕЛЯ_КОДКОНЕЧНОГОПОЛУЧАТЕЛЯ
				//
				ObjTagItem tag_item;
				if(PPRef->Ot.GetTag(PPOBJ_PERSON, main_org_id, PPTAG_PERSON_ENALOGDEST, &tag_item) > 0) {
					tag_item.GetStr(rcvr_ident);
				}
				if(rcvr_ident.NotEmptyS()) {
					if(rcvr_ident.Divide('.', left, temp_buf) > 0) {
                        (rcvr_ident = left).CatChar('_').Cat(temp_buf);
					}
					else if(rcvr_ident.Divide('_', left, temp_buf) > 0) {
						;
					}
					else {
						temp_buf = rcvr_ident;
						(rcvr_ident = temp_buf).CatChar('_').Cat(temp_buf);
					}
				}
				else {
					// @todo Вывод информации о недопустимом коде получателя налоговой отчетности
                    rcvr_ident.Z().CatCharN('0', 4).CatChar('_').CatCharN('0', 4);
				}
			}
		}
		if(sender_ident.IsEmpty()) {
			// @todo Вывод информации о недопустимой комбинации ИНН/КПП
			sender_ident.Z().CatCharN('0', 12);
		}
		if(Filt.Kind == PPVTB_BUY) {
			// @v11.3.2 p_ledger_title = "КнигаПокуп";
			// @v11.3.2 p_ledger_line_title = "КнПокСтр";
			ledger_title = g.GetToken_Ansi(PPHSC_RU_PURCHASELEDGER); // @v11.3.2
			ledger_line_title = g.GetToken_Ansi(PPHSC_RU_PURCHASELEDGERLN); // @v11.3.2
			id_file.Cat("NO_NDS").DotCat("8").CatChar('_').Cat(rcvr_ident).CatChar('_').Cat(sender_ident).
				CatChar('_').Cat(_cdate, DATF_YMD|DATF_CENTURY|DATF_NODIV).CatChar('_').Cat(_uniq_suffix);
		}
		else if(Filt.Kind == PPVTB_SELL) {
			// @v11.3.2 p_ledger_title = "КнигаПрод";
			// @v11.3.2 p_ledger_line_title = "КнПродСтр";
			ledger_title = g.GetToken_Ansi(PPHSC_RU_SALESLEDGER); // @v11.3.2
			ledger_line_title = g.GetToken_Ansi(PPHSC_RU_SALESLEDGERLN); // @v11.3.2
			id_file.Cat("NO_NDS").DotCat("9").CatChar('_').Cat(rcvr_ident).CatChar('_').Cat(sender_ident).
				CatChar('_').Cat(_cdate, DATF_YMD|DATF_CENTURY|DATF_NODIV).CatChar('_').Cat(_uniq_suffix);
		}
	}
	if(ledger_title.NotEmpty()) {
		char   xml_entity_spec[256];
		const  char * p_xml_entity_spec = 0;
		{
			left = DS.GetConstTLA().DL600XMLEntityParam;
			if(left.NotEmptyS()) {
				left.CopyTo(xml_entity_spec, sizeof(xml_entity_spec));
				p_xml_entity_spec = xml_entity_spec;
			}
			left.Z();
		}
		out_file_name.Z().Cat(id_file).DotCat("xml");
		PPGetFilePath(PPPATH_OUT, out_file_name, path);
		g.StartDocument(path, cp1251); // @v11.2.10
		{
			{
				SXml::WNode n_file(g.P_X, g.GetToken_Ansi(PPHSC_RU_FILE));
				n_file.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDFILE), id_file);
				{
					PPVersionInfo vi = DS.GetVersionInfo();
					SVerT ver = vi.GetVersion();
					//vi.GetProductName(left);
					vi.GetTextAttrib(vi.taiProductName, left);
					left.Space().Cat(ver.ToStr(temp_buf));
					n_file.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VERPROG), left);
				}
				n_file.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VERFORM), "5.11"); // @v11.2.12 "5.06"-->"5.08" // @v12.3.2 "5.08"-->"5.11"
				{
					SXml::WNode n_doc(g.P_X, g.GetToken_Ansi(PPHSC_RU_DOCUMENT));
					if(Filt.Kind == PPVTB_BUY) {
						n_doc.PutAttrib(g.GetToken_Ansi(PPHSC_RU_INDEX), "0000080");
					}
					else if(Filt.Kind == PPVTB_SELL) {
						n_doc.PutAttrib(g.GetToken_Ansi(PPHSC_RU_INDEX), "0000090");
					}
					// @v11.3.2 n_doc.PutAttrib("НомКорр", "0");
					n_doc.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CORRECTIONNO_), "0"); // @v11.3.2
					{
						constexpr uint vat_rate_count = 5;
						SXml::WNode n_book(g.P_X, ledger_title);
						double sum_vat0 = 0.0;
						double sum_vatn[vat_rate_count] = { 0.0, 0.0, 0.0, 0.0, 0.0 }; // @v12.3.2 [3]-->[5]
						double sum_svatn[vat_rate_count] = { 0.0, 0.0, 0.0, 0.0, 0.0 }; // @v12.3.2 [3]-->[5]
						double sum_svat = 0.0;
						double sum_amount = 0.0;
						PPObjCurrency cur_obj;
						PPCurrency cur_rec;
						long   base_cur_code = 0;
						VatBookViewItem item;
						uint   line_no = 0;
						const  PPID base_cur_id = LConfig.BaseCurID;
						if(base_cur_id && cur_obj.Fetch(base_cur_id, &cur_rec) > 0) {
							base_cur_code = cur_rec.Code;
						}
						SETIFZ(base_cur_code, 643); // По умолчанию Российский Рубль
						for(InitIteration(); NextIteration(&item) > 0;) {
							const double _vat0 = item.VAT0;
							const double _vatn[vat_rate_count] = { item.VAT1, item.VAT2, item.VAT3, item.VAT4, item.VAT5 }; // @v12.3.2 [3]-->[5]
							const double _svatn[vat_rate_count] = { item.SVAT1, item.SVAT2, item.SVAT3, item.SVAT4, item.SVAT5 }; // @v12.3.2 [3]-->[5]
							const double _svat = _svatn[0] + _svatn[1] + _svatn[2] + _svatn[3] + _svatn[4];
							const double _amount = item.Amount;
							sum_vat0 += _vat0;
							for(i = 0; i < vat_rate_count; i++) {
								sum_vatn[i] += _vatn[i];
								sum_svatn[i] += _svatn[i];
							}
							sum_svat += _svat;
							sum_amount += _amount;
						}
						/* @todo @v11.4.5 Список символов, которые необходимо перенести в ppstr2
							"СумНДСВсКПк"
							"СтПродБезНДС18"
							"СумНДСВсКПр18"
							"СтПродБезНДС20"
							"СумНДСВсКПр20"
							"СтПродБезНДС10"
							"СумНДСВсКПр10"
							"СтПродБезНДС0"
							"СтПродОсвВсКПр"
							"НомерПор"
							"НомСчФПрод"
							"ДатаСчФПрод"
							"НомКСчФПрод"
							"ДатаКСчФПрод"
							"НомТД" // Номер таможенной декларации
							"СтоимПокупВ"
							"СумНДСВыч"
							"СтоимПродСФВ"
							"СтоимПродСФ"
							"СтоимПродСФ18"
							"СумНДССФ18"
							"СтоимПродСФ20"
							"СумНДССФ20"
							"СтоимПродСФ10"
							"СумНДССФ10"
							"СтоимПродСФ0"
							"СтоимПродОсв"
							"КодВидОпер"
							"ДатаУчТов"
						*/
						if(Filt.Kind == PPVTB_BUY) {
							n_book.PutAttrib("СумНДСВсКПк", temp_buf.Z().Cat(sum_svat, SFMT_MONEY));
						}
						else if(Filt.Kind == PPVTB_SELL) {
							for(i = 0; i < vat_rate_count; i++) {
								if(PPObjVATBook::IsVatRate(i, 18.0)) {
									n_book.PutAttrib("СтПродБезНДС18", temp_buf.Z().Cat(sum_vatn[i]/*-sum_svatn[i]*/, SFMT_MONEY)); // Сумма продаж по ставке НДС 18% (без налога)
									n_book.PutAttrib("СумНДСВсКПр18",  temp_buf.Z().Cat(sum_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 18%
								}
								else if(PPObjVATBook::IsVatRate(i, 20.0)) {
									n_book.PutAttrib("СтПродБезНДС20", temp_buf.Z().Cat(sum_vatn[i]/*-sum_svatn[i]*/, SFMT_MONEY)); // Сумма продаж по ставке НДС 20% (без налога)
									n_book.PutAttrib("СумНДСВсКПр20",  temp_buf.Z().Cat(sum_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 20%
								}
								else if(PPObjVATBook::IsVatRate(i, 10.0)) {
									n_book.PutAttrib("СтПродБезНДС10", temp_buf.Z().Cat(sum_vatn[i]/*-sum_svatn[i]*/, SFMT_MONEY)); // Сумма продаж по ставке НДС 10% (без налога)
									n_book.PutAttrib("СумНДСВсКПр10",  temp_buf.Z().Cat(sum_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 10%
								}
								else if(PPObjVATBook::IsVatRate(i, 7.0)) {
									n_book.PutAttrib("СтПродБезНДС7", temp_buf.Z().Cat(sum_vatn[i]/*-sum_svatn[i]*/, SFMT_MONEY)); // Сумма продаж по ставке НДС 7% (без налога)
									n_book.PutAttrib("СумНДСВсКПр7",  temp_buf.Z().Cat(sum_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 7%
								}
								else if(PPObjVATBook::IsVatRate(i, 5.0)) {
									n_book.PutAttrib("СтПродБезНДС5", temp_buf.Z().Cat(sum_vatn[i]/*-sum_svatn[i]*/, SFMT_MONEY)); // Сумма продаж по ставке НДС 5% (без налога)
									n_book.PutAttrib("СумНДСВсКПр5",  temp_buf.Z().Cat(sum_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 5%
								}
							}
							n_book.PutAttrib("СтПродБезНДС0", temp_buf.Z().Cat(0.0, SFMT_MONEY)); // Всего стоимость продаж (без налога) по ставке 0%
							n_book.PutAttrib("СтПродОсвВсКПр", temp_buf.Z().Cat(sum_vat0, SFMT_MONEY)); // Сумма продаж, освобожденных от НДС
						}
						for(InitIteration(); NextIteration(&item) > 0;) {
							SXml::WNode n_item(g.P_X, ledger_line_title);
							line_no++;

							const double _vat0 = item.VAT0;
							const double _vatn[vat_rate_count] = { item.VAT1, item.VAT2, item.VAT3,  item.VAT4, item.VAT5 };
							const double _svatn[vat_rate_count] = { item.SVAT1, item.SVAT2, item.SVAT3,  item.SVAT4, item.SVAT5 };
							const double _svat = _svatn[0] + _svatn[1] + _svatn[2] + _svatn[3] + _svatn[4];
							const double _amount = item.Amount;
                            {
                            	n_item.PutAttrib("НомерПор", temp_buf.Z().Cat(line_no));
                            	n_item.PutAttrib("НомСчФПрод", (temp_buf = item.Code).Transf(CTRANSF_INNER_TO_OUTER));
                            	n_item.PutAttrib("ДатаСчФПрод", temp_buf.Z().Cat(item.InvcDt, DATF_GERMANCENT));
                            	//n_item.PutAttrib("НомИспрСчФ", temp_buf.Z());
                            	//n_item.PutAttrib("ДатаИспрСчФ", temp_buf.Z());
                            	if(item.CBillCode[0]) {
									n_item.PutAttrib("НомКСчФПрод", item.CBillCode);
									n_item.PutAttrib("ДатаКСчФПрод", temp_buf.Z().Cat(item.CBillDt, DATF_GERMANCENT));
                            	}
                            	//n_item.PutAttrib("НомИспрКСчФ", temp_buf.Z());
                            	//n_item.PutAttrib("ДатаИспрКСчФ", temp_buf.Z());
								if(Filt.Kind == PPVTB_BUY) {
									// n_item.PutAttrib("НомТД", temp_buf.Z()); // Номер таможенной декларации
									n_item.PutAttrib(g.GetToken_Ansi(PPHSC_RU_OKV), temp_buf.Z().CatLongZ(base_cur_code, 3)); // Код валюты по ОКВ
									n_item.PutAttrib("СтоимПокупВ", temp_buf.Z().Cat(_amount, SFMT_MONEY));
									n_item.PutAttrib("СумНДСВыч", temp_buf.Z().Cat(_svat, SFMT_MONEY));
								}
								else if(Filt.Kind == PPVTB_SELL) {
									n_item.PutAttrib(g.GetToken_Ansi(PPHSC_RU_OKV), temp_buf.Z().CatLongZ(base_cur_code, 3)); // Код валюты по ОКВ
									if(!oneof2(base_cur_code, 0, 643)) {
										n_item.PutAttrib("СтоимПродСФВ", temp_buf.Z().Cat(_amount, SFMT_MONEY)); // Сумма продаж в валюте
									}
									n_item.PutAttrib("СтоимПродСФ",  temp_buf.Z().Cat(_amount, SFMT_MONEY)); // Сумма продаж в рублях
									for(i = 0; i < vat_rate_count; i++) {
										if(PPObjVATBook::IsVatRate(i, 18.0)) {
											n_item.PutAttrib("СтоимПродСФ18", temp_buf.Z().Cat(_vatn[i], SFMT_MONEY)); // Сумма продаж по ставке НДС 18%
											n_item.PutAttrib("СумНДССФ18", temp_buf.Z().Cat(_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 18%
										}
										else if(PPObjVATBook::IsVatRate(i, 20.0)) {
											n_item.PutAttrib("СтоимПродСФ20", temp_buf.Z().Cat(_vatn[i], SFMT_MONEY)); // Сумма продаж по ставке НДС 20%
											n_item.PutAttrib("СумНДССФ20", temp_buf.Z().Cat(_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 20%
										}
										else if(PPObjVATBook::IsVatRate(i, 10.0)) {
											n_item.PutAttrib("СтоимПродСФ10", temp_buf.Z().Cat(_vatn[i], SFMT_MONEY)); // Сумма продаж по ставке НДС 10%
											n_item.PutAttrib("СумНДССФ10", temp_buf.Z().Cat(_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 10%
										}
										// @v12.3.2 {
										else if(PPObjVATBook::IsVatRate(i, 7.0)) {
											n_item.PutAttrib("СтоимПродСФ7", temp_buf.Z().Cat(_vatn[i], SFMT_MONEY)); // Сумма продаж по ставке НДС 7%
											n_item.PutAttrib("СумНДССФ7", temp_buf.Z().Cat(_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 7%
										}
										else if(PPObjVATBook::IsVatRate(i, 5.0)) {
											n_item.PutAttrib("СтоимПродСФ5", temp_buf.Z().Cat(_vatn[i], SFMT_MONEY)); // Сумма продаж по ставке НДС 5%
											n_item.PutAttrib("СумНДССФ5", temp_buf.Z().Cat(_svatn[i], SFMT_MONEY)); // Сумма НДС по ставке 5%
										}
										// } @v12.3.2 
									}
									n_item.PutAttrib("СтоимПродСФ0", temp_buf.Z().Cat(0.0, SFMT_MONEY)); // Сумма продаж по ставке НДС 0%
									n_item.PutAttrib("СтоимПродОсв", temp_buf.Z().Cat(_vat0, SFMT_MONEY)); // Сумма продаж, освобожденных от НДС
								}
                            	{
                            		GetNalogRuOpIdent(item, temp_buf);
									SXml::WNode n(g.P_X, "КодВидОпер", temp_buf);
                            	}
                           		if(Filt.Kind == PPVTB_BUY) {
									{
										SXml::WNode n(g.P_X, "ДатаУчТов", temp_buf.Z().Cat(item.Dt, DATF_GERMANCENT));
									}
									{
										//SXml::WNode n(g.P_X, "СвПрод");
										//WriteNalogRuPersonBlock(PsnObj, ObjectToPerson(item.Object), g.P_X);
										// (Эту функцию пока нельзя использовать - есть отличия) g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_SELLERINFO), ObjectToPerson(item.Object), /*shipper_loc_id*/0, item.Dt, 0);
										g.WriteOrgInfo_VatLedger(g.GetToken_Ansi(PPHSC_RU_SELLERINFO), ObjectToPerson(item.ArID), /*shipper_loc_id*/0, item.Dt, 0);
									}
                            	}
                            	else if(Filt.Kind == PPVTB_SELL) {
									if(item.ArID) {
										//SXml::WNode n(g.P_X, "СвПокуп");
										//WriteNalogRuPersonBlock(PsnObj, ObjectToPerson(item.Object), g.P_Xw);
										// (Эту функцию пока нельзя использовать - есть отличия) g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_BUYERINFO), ObjectToPerson(item.Object), 0, item.Dt, 0);
										g.WriteOrgInfo_VatLedger(g.GetToken_Ansi(PPHSC_RU_BUYERINFO), ObjectToPerson(item.ArID), 0, item.Dt, 0);
									}
                            	}
                            }
						}
					}
				}
			}
			g.EndDocument();
		}
	}
	return ok;
}
