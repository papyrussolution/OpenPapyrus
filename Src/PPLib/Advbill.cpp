// ADVBILL.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PPObjAdvBillKind::PPObjAdvBillKind(void * extraPtr) : PPObjReference(PPOBJ_ADVBILLKIND, extraPtr)
{
}

int SLAPI PPObjAdvBillKind::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0, r = cmCancel, is_new = 0;
	PPAdvBillKind rec;
	PPIDArray op_type_list;
	TDialog * dlg = new TDialog(DLG_ADVBILLKIND);
	THROW(CheckDialogPtr(&dlg));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setCtrlData(CTL_ADVBILLKIND_NAME, rec.Name);
	dlg->setCtrlLong(CTL_ADVBILLKIND_ID, rec.ID);
	op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, 0L);
	SetupOprKindCombo(dlg, CTLSEL_ADVBILLKIND_LNKOP, rec.LinkOpID, 0, &op_type_list, 0);
	dlg->AddClusterAssoc(CTL_ADVBILLKIND_FLAGS, 0, PPAdvBillKind::fSkipAccturn);
	dlg->SetClusterData(CTL_ADVBILLKIND_FLAGS, rec.Flags);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_ADVBILLKIND_NAME, &rec.Name);
		dlg->getCtrlData(CTLSEL_ADVBILLKIND_LNKOP, &rec.LinkOpID);
		dlg->GetClusterData(CTL_ADVBILLKIND_FLAGS, &rec.Flags);
		if(!CheckName(*pID, rec.Name, 0))
			dlg->selectCtrl(CTL_ADVBILLKIND_NAME);
		else if(EditItem(Obj, *pID, &rec, 1)) {
			Dirty(*pID);
			valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOK
	delete dlg;
	return ok ? r : 0;
}

int SLAPI PPObjAdvBillKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPAdvBillKind * p_rec = (PPAdvBillKind *)p->Data;
		return ProcessObjRefInArray(PPOBJ_OPRKIND, &p_rec->LinkOpID, ary, replace) ? 1 : 0;
	}
	else
		return -1;
}
//
//
//
SLAPI PPAdvBillItem::PPAdvBillItem()
{
	THISZERO();
}

int FASTCALL PPAdvBillItem::IsEqual(const PPAdvBillItem & rS) const
{
#define CMPF(f) if(f != rS.f) return 0;
	CMPF(BillID);
	CMPF(RByBill);
	CMPF(AdvDt);
	CMPF(AdvBillKindID);
	CMPF(AdvBillID);
	CMPF(AccID);
	CMPF(ArID);
	CMPF(Flags);
	CMPF(Amount);
	CMPF(ExtAmt);
#undef CMPF
	if(strcmp(AdvCode, rS.AdvCode) != 0)
		return 0;
	else if(strcmp(Memo, rS.Memo) != 0)
		return 0;
	return 1;
}
//
//
//
SLAPI PPAdvBillItemList::PPAdvBillItemList() : SArray(sizeof(PPAdvBillItem))
{
}

int FASTCALL PPAdvBillItemList::IsEqual(const PPAdvBillItemList & rS) const
{
	int    eq = 1;
	const  uint c = getCount();
	const  uint c2 = rS.getCount();
	if(c != c2)
		eq = 0;
	else if(c) {
		//
		// Перестановка элементов считается отличием
		//
		for(uint i = 0; eq && i < c; i++) {
			const PPAdvBillItem & r_rec = Get(i);
			const PPAdvBillItem & r_rec2 = rS.Get(i);
			if(!r_rec.IsEqual(r_rec2))
				eq = 0;
		}
	}
	return eq;
}

uint SLAPI PPAdvBillItemList::GetCount() const
{
	return getCount();
}

PPAdvBillItem & FASTCALL PPAdvBillItemList::Get(uint pos) const
{
	return (pos < getCount()) ? *(PPAdvBillItem *)at(pos) : *(PPAdvBillItem *)0;
}

int SLAPI PPAdvBillItemList::SearchBillLink(PPID billID, uint * pPos) const
{
	if(billID)
		for(uint i = 0; i < getCount(); i++)
			if(Get(i).AdvBillID == billID) {
				ASSIGN_PTR(pPos, i);
				return 1;
			}
	return 0;
}

int SLAPI PPAdvBillItemList::GetStorageForm(uint pos, AdvBillItemTbl::Rec * pItem) const
{
	if(pos < GetCount()) {
		const PPAdvBillItem & item = Get(pos);
		memzero(pItem, sizeof(*pItem));
		pItem->BillID  = item.BillID;
		pItem->RByBill = item.RByBill;
		STRNSCPY(pItem->AdvCode, item.AdvCode);
		pItem->AdvDt   = item.AdvDt;
		pItem->AdvBillKindID = item.AdvBillKindID;
		pItem->AdvBillID     = item.AdvBillID;
		pItem->AccID   = item.AccID;
		pItem->ArID    = item.ArID;
		pItem->Flags   = item.Flags;
		pItem->Amount  = item.Amount;
		pItem->ExtAmt1 = item.ExtAmt;
		STRNSCPY(pItem->Memo, item.Memo);
		return 1;
	}
	else
		return 0;
}

int SLAPI PPAdvBillItemList::AddStorageForm(const AdvBillItemTbl::Rec * pItem)
{
	PPAdvBillItem item;
	item.BillID  = pItem->BillID;
	item.RByBill = pItem->RByBill;
	STRNSCPY(item.AdvCode, pItem->AdvCode);
	item.AdvDt   = pItem->AdvDt;
	item.AdvBillKindID = pItem->AdvBillKindID;
	item.AdvBillID     = pItem->AdvBillID;
	item.AccID   = pItem->AccID;
	item.ArID    = pItem->ArID;
	item.Flags   = pItem->Flags;
	item.Amount  = pItem->Amount;
	item.ExtAmt  = pItem->ExtAmt1;
	STRNSCPY(item.Memo, pItem->Memo);
	return Add(&item);
}

int SLAPI PPAdvBillItemList::Add(const PPAdvBillItem * pItem)
{
	return insert(pItem) ? 1 : PPSetErrorSLib();
}

int SLAPI PPAdvBillItemList::Remove(uint pos)
{
	return atFree(pos);
}

int SLAPI PPAdvBillItemList::Clear()
{
	freeAll();
	return 1;
}

int SLAPI PPAdvBillItemList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	return pCtx->Serialize(dir, (SArray *)this, rBuf) ? 1 : PPSetErrorSLib();
}
//
// AdvBillItemDialog
//
#define GRP_ACC   1

class AdvBillItemDialog : public TDialog {
public:
	AdvBillItemDialog(uint dlgID) : TDialog(dlgID)
	{
		P_Pack = 0;
	}
	void   setBillPacket(const PPBillPacket * pPack)
	{
		P_Pack = pPack;
	}
	virtual int setDTS(const PPAdvBillItem *);
	virtual int getDTS(PPAdvBillItem *);
protected:
	DECL_HANDLE_EVENT;
	void   setupLinkBill();
	void   editLink();
	void   removeLink();
	PPAdvBillItem Data;
	const PPBillPacket * P_Pack;
};

void AdvBillItemDialog::setupLinkBill()
{
	SString info_buf;
	BillTbl::Rec bill_rec;
	if(Data.AdvBillID && BillObj->Search(Data.AdvBillID, &bill_rec) > 0) {
		PPObjBill::MakeCodeString(&bill_rec, 1, info_buf);
		enableCommand(cmAdvBillItemRemoveLink, 1);
		if(getCtrlReal(CTL_ADVBITEM_AMOUNT) == 0.0)
			setCtrlReal(CTL_ADVBITEM_AMOUNT, bill_rec.Amount);
	}
	else
		enableCommand(cmAdvBillItemRemoveLink, 0);
	setStaticText(CTL_ADVBITEM_LINKINFO, info_buf);
}

void AdvBillItemDialog::editLink()
{
	int    r = 1;
	PPObjBill * p_bobj = BillObj;
	PPObjAdvBillKind abk_obj;
	PPAdvBillKind abk_rec;
	getCtrlData(CTLSEL_ADVBITEM_BILLKIND, &Data.AdvBillKindID);
	if(abk_obj.Search(Data.AdvBillKindID, &abk_rec) > 0 && abk_rec.LinkOpID) {
		if(Data.AdvBillID && p_bobj->Search(Data.AdvBillID, 0) > 0) {
			p_bobj->Edit(&Data.AdvBillID, 0);
			setupLinkBill();
		}
		else {
			uint v = 0;
			if(SelectorDialog(DLG_SELLINKBILLADD, STDCTL_SELECTOR_WHAT, &v) > 0) {
				PPID   bill_id = 0;
				BillFilt bill_filt;
				{
					AcctCtrlGroup::Rec acc_rec;
					getCtrlData(CTL_ADVBITEM_DT,   &Data.AdvDt);
					getCtrlData(CTL_ADVBITEM_CODE,  Data.AdvCode);
					getCtrlData(CTL_ADVBITEM_AMOUNT, &Data.Amount);
					getCtrlData(CTL_ADVBITEM_MEMO,   Data.Memo);
					getGroupData(GRP_ACC, &acc_rec);
					Data.AccID = acc_rec.AcctId.ac;
					Data.ArID  = acc_rec.AcctId.ar;
				}
				const  PPID current_loc_id = LConfig.Location;
				PPID   op_id = abk_rec.LinkOpID;
				PPID   loc_id = P_Pack ? P_Pack->Rec.LocID : current_loc_id;
				if(v == 0) {
					PPIDArray op_list;
					op_list.add(abk_rec.LinkOpID);
					if(BillPrelude(&op_list, OPKLF_OPLIST, 0, &op_id, &loc_id) > 0) {
						DS.SetLocation(loc_id);
						if(GetOpType(op_id) == PPOPT_ACCTURN && !CheckOpFlags(op_id, OPKF_EXTACCTURN))
							r = p_bobj->AddGenAccturn(&bill_id, op_id, 0);
						else {
							bill_filt.LocList.Add(loc_id, 1);
							bill_filt.OpID = op_id;
							bill_filt.ObjectID = Data.ArID;
							bill_filt.Period.SetDate(Data.AdvDt);
							bill_filt.AmtRange.SetVal(Data.Amount);
							r = p_bobj->AddGoodsBillByFilt(&bill_id, &bill_filt, op_id);
						}
						DS.SetLocation(current_loc_id);
						if(r > 0)
							Data.AdvBillID = bill_id;
						setupLinkBill();
					}
				}
				else if(v == 1) {
					bill_filt.LocList.Add(loc_id, 1);
					bill_filt.OpID  = op_id;
					bill_filt.ObjectID = Data.ArID;
					bill_filt.Period.SetDate(Data.AdvDt);
					bill_filt.Flags |= BillFilt::fAsSelector;
					PPViewBill bill_view;
					if(bill_view.Init_(&bill_filt)) {
						if(bill_view.Browse(0) > 0) {
							int r = -1;
							bill_id = ((BillFilt*)bill_view.GetBaseFilt())->Sel;
							//
							// Проверка на то, чтобы на выбранный документ не было
							// ссылок в этом или других авансовых отчетах
							//
							if(bill_id) {
								if(P_Pack) {
									uint pos = 0;
									if(P_Pack->AdvList.SearchBillLink(bill_id, &pos))
										if(P_Pack->AdvList.Get(pos).RByBill != Data.RByBill)
											r = BIN(CONFIRM(PPCFM_ADVBILLLINKEXISTS));
								}
								if(r < 0) {
									AdvBillItemTbl::Rec abi_rec;
									BillTbl::Rec bill_rec;
									if(p_bobj->SearchAdvLinkToBill(bill_id, &abi_rec, &bill_rec) > 0) {
										if(P_Pack) {
											if(bill_rec.ID != P_Pack->Rec.ID || abi_rec.RByBill != Data.RByBill)
												r = BIN(CONFIRM(PPCFM_ADVBILLLINKEXISTS));
										}
										else
											r = BIN(CONFIRM(PPCFM_ADVBILLLINKEXISTS));
									}
								}
							}
							if(r) {
								Data.AdvBillID = ((BillFilt*)bill_view.GetBaseFilt())->Sel;
								setupLinkBill();
							}
						}
					}
					else
						PPError();
				}
			}
		}
	}
}

void AdvBillItemDialog::removeLink()
{
	PPObjBill * p_bobj = BillObj;
	if(Data.AdvBillID && p_bobj->Search(Data.AdvBillID, 0) > 0) {
		uint v = 0;
		if(SelectorDialog(DLG_SELLINKBILLRMV, STDCTL_SELECTOR_WHAT, &v) > 0) {
			if(v == 0) {
				Data.AdvBillID = 0;
				setupLinkBill();
			}
			else if(v == 1) {
				if(CONFIRM(PPCFM_DELETE)) {
					if(p_bobj->RemovePacket(Data.AdvBillID, 1)) {
						Data.AdvBillID = 0;
						setupLinkBill();
					}
					else
						PPError();
				}
			}
		}
	}
}

IMPL_HANDLE_EVENT(AdvBillItemDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmAdvBillItemEditLink))
		editLink();
	else if(event.isCmd(cmAdvBillItemRemoveLink))
		removeLink();
	else
		return;
	clearEvent(event);
}

int AdvBillItemDialog::setDTS(const PPAdvBillItem * pData)
{
	int    ok = 1;
	Data = *pData;
	AcctCtrlGroup::Rec acc_rec;
	AcctCtrlGroup * p_acc_grp = new AcctCtrlGroup(CTL_ADVBITEM_ACC, CTL_ADVBITEM_ART, CTLSEL_ADVBITEM_ACCNAME, CTLSEL_ADVBITEM_ARTNAME);
	THROW_MEM(p_acc_grp);
	addGroup(GRP_ACC, p_acc_grp);
	acc_rec.AcctId.ac   = Data.AccID;
	acc_rec.AcctId.ar   = Data.ArID;
	BillObj->atobj->P_Tbl->AccObj.InitAccSheetForAcctID(&acc_rec.AcctId, &acc_rec.AccSheetID);
	acc_rec.AccSelParam = ACY_SEL_BAL;
	setGroupData(GRP_ACC, &acc_rec);
	SetupCalDate(CTLCAL_ADVBITEM_DT, CTL_ADVBITEM_DT);
	setCtrlData(CTL_ADVBITEM_DT,     &Data.AdvDt);
	SetupPPObjCombo(this, CTLSEL_ADVBITEM_BILLKIND, PPOBJ_ADVBILLKIND, Data.AdvBillKindID, OLW_CANINSERT);
	setCtrlData(CTL_ADVBITEM_CODE,   Data.AdvCode);
	setCtrlData(CTL_ADVBITEM_AMOUNT, &Data.Amount);
	setCtrlData(CTL_ADVBITEM_MEMO,   Data.Memo);
	setupLinkBill();
	CATCHZOK
	return ok;
}

int AdvBillItemDialog::getDTS(PPAdvBillItem * pData)
{
	int    ok = 1;
	uint   sel = 0;
	AcctCtrlGroup::Rec acc_rec;
	getCtrlData(sel = CTL_ADVBITEM_DT,   &Data.AdvDt);
	THROW_SL(checkdate(Data.AdvDt, 1));
	getCtrlData(CTL_ADVBITEM_CODE, Data.AdvCode);
	getCtrlData(CTLSEL_ADVBITEM_BILLKIND, &Data.AdvBillKindID);
	getCtrlData(CTL_ADVBITEM_AMOUNT, &Data.Amount);
	getCtrlData(CTL_ADVBITEM_MEMO,   Data.Memo);
	getGroupData(GRP_ACC, &acc_rec);
	Data.AccID = acc_rec.AcctId.ac;
	Data.ArID  = acc_rec.AcctId.ar;
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}
//
// WarrantItemDialog
//
class WarrantItemDialog : public AdvBillItemDialog {
public:
	WarrantItemDialog(uint dlgID) : AdvBillItemDialog(dlgID) 
	{
	}
	virtual int WarrantItemDialog::setDTS(const PPAdvBillItem * pData)
	{
		Data = *pData;
		setCtrlData(CTL_WARRITEM_NAME, Data.Memo);
		SetupPPObjCombo(this, CTLSEL_WARRITEM_UNIT, PPOBJ_UNIT,	Data.ArID, OLW_CANINSERT);
		setCtrlData(CTL_WARRITEM_QTTY, &Data.Amount);
		return 1;
	}
	virtual int WarrantItemDialog::getDTS(PPAdvBillItem * pData)
	{
		getCtrlData(CTL_WARRITEM_NAME, Data.Memo);
		getCtrlData(CTLSEL_WARRITEM_UNIT, &Data.ArID);
		getCtrlData(CTL_WARRITEM_QTTY, &Data.Amount);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
};
//
//
//
class DebtInventItemDialog : public AdvBillItemDialog {
public:
	DebtInventItemDialog(PPID accSheetID) : AdvBillItemDialog(DLG_DINVITEM)
	{
		AccSheetID = accSheetID;
	}
	virtual int setDTS(const PPAdvBillItem * pData)
	{
		Data = *pData;
		SetupArCombo(this, CTLSEL_DINVITEM_AR, Data.ArID, OLW_CANINSERT, AccSheetID, sacfDisableIfZeroSheet);
		setCtrlData(CTL_DINVITEM_AMOUNT, &Data.Amount);
		setCtrlData(CTL_DINVITEM_MEMO,    Data.Memo);
		return 1;
	}
	virtual int getDTS(PPAdvBillItem * pData)
	{
		getCtrlData(CTLSEL_DINVITEM_AR,  &Data.ArID);
		getCtrlData(CTL_DINVITEM_AMOUNT, &Data.Amount);
		getCtrlData(CTL_DINVITEM_MEMO,    Data.Memo);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	PPID   AccSheetID;
};
//
//
//
struct AdvBillItemEntry {
	LDATE  Dt;             // #00
	char   Code[24];       // #04 UnitName (WarrantItemBrowser) // @v6.2.2 [10]-->[24]
	char   BillKind[48];   // #28                               // @v6.2.2 [30]-->[48]
	char   Account[48];    // #76
	double Amount;         // #124
	char   Memo[128];      // #132
};

typedef int (* SetupAdvItemRowProc)(AdvBillItemEntry *, const PPAdvBillItem *, PPObjBill *);

class AdvBillItemBrowser : public BrowserWindow {
public:
	AdvBillItemBrowser(uint rezID, PPObjBill *, PPBillPacket *, SetupAdvItemRowProc);
	~AdvBillItemBrowser();
	int    update(int);
private:
	DECL_HANDLE_EVENT;
	int    getCurItemPos();
	int    editAdvBillItem(PPAdvBillItem * pData);
	int    addItem();
	int    addItemBySample();
	int    editItem();
	int    delItem();

	PPBillPacket * P_Pack;
	PPObjBill    * P_BObj;
	SetupAdvItemRowProc P_SetEntry;
	int    IsWarrant;
};

enum { // Параметр функции AdvBillItemBrowser::update
	pos_top    = -1,
	pos_cur    = -2,
	pos_bottom = -3
};

AdvBillItemBrowser::AdvBillItemBrowser(uint rezID, PPObjBill * pBObj, PPBillPacket * p,
	SetupAdvItemRowProc pSetEntry) : BrowserWindow(rezID, (SArray *)0)
{
	P_BObj     = pBObj;
	P_Pack     = p;
	P_SetEntry = pSetEntry;
	IsWarrant  = BIN(GetOpSubType(P_Pack->Rec.OpID) == OPSUBT_WARRANT);
	SString code;
	setSubTitle(PPObjBill::MakeCodeString(&P_Pack->Rec, 0, code));
}

AdvBillItemBrowser::~AdvBillItemBrowser()
{
}

int AdvBillItemBrowser::getCurItemPos()
{
	return (int16)view->getDef()->_curItem();
}
//
//
//
int AdvBillItemBrowser::editAdvBillItem(PPAdvBillItem * pData)
{
	int    ok = -1;
	AdvBillItemDialog * dlg = 0;
	if(GetOpSubType(P_Pack->Rec.OpID) == OPSUBT_WARRANT)
		dlg = new WarrantItemDialog(DLG_WARRITEM);
	else if(GetOpSubType(P_Pack->Rec.OpID) == OPSUBT_DEBTINVENT)
		dlg = new DebtInventItemDialog(P_Pack->AccSheetID);
	else
		dlg = new AdvBillItemDialog(DLG_ADVREPITEM);
	THROW(CheckDialogPtr(&dlg));
	dlg->setBillPacket(P_Pack);
	THROW(dlg->setDTS(pData));
	while(ok <= 0 && ExecView(dlg) == cmOK)
		if(dlg->getDTS(pData))
			ok = 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPL_HANDLE_EVENT(AdvBillItemBrowser)
{
	BrowserWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaEdit:   editItem(); break;
			case cmaInsert: addItem();  break;
			case cmaDelete: delItem();  break;
			default: return;
		}
	}
	else if(event.isKeyDown(kbAltF2))
		addItemBySample();
	else
		return;
	clearEvent(event);
}

static char * SLAPI TotalLinesStr(char * pBuf, int numLines)
{
	PPGetWord(PPWORD_TOTAL, 0, pBuf, 0);
	if(numLines) {
		SString temp_buf;
		PPGetWord(PPWORD_LINES, 0, temp_buf).Quot(' ', ' ').Cat(numLines);
		strcat(pBuf, temp_buf);
	}
	return pBuf;
}

int AdvBillItemBrowser::update(int pos)
{
	int    ok = -1;
	uint   i;
	SArray * p_list = 0;
	AryBrowserDef * p_def = (AryBrowserDef *)view->getDef();
	if(p_def) {
		uint   count = P_Pack->AdvList.GetCount();
		AdvBillItemEntry total;
		MEMSZERO(total);
		int16  c = (int16)view->getDef()->_curItem();
		p_def->setArray(0, 0, 1);
		THROW_MEM(p_list = new SArray(sizeof(AdvBillItemEntry)));
		for(i = 0; i < count; i++) {
			const PPAdvBillItem & item = P_Pack->AdvList.Get(i);
			AdvBillItemEntry entry;
			P_SetEntry(&entry, &item, P_BObj);
			THROW_SL(p_list->insert(&entry));
			total.Amount += entry.Amount;
		}
		TotalLinesStr(IsWarrant ? total.Memo : total.Account, count);
		THROW_SL(p_list->insert(&total));
		lock();
		p_def->setArray(p_list, 0, 0);
		view->setRange(p_list->getCount());
		if(pos == pos_cur && c >= 0 && c < (int)p_list->getCount())
			view->go(c);
		else if(pos == pos_bottom && p_list->getCount() >= 2)
			view->go(p_list->getCount() - 2);
		else if(pos >= 0 && pos < (int)p_list->getCount())
			view->go(pos);
		else
			p_def->top();
		unlock();
	}
	CATCHZOK
	return ok;
}

// SetupAdvItemRowProc
static int SetAdvBillItemEntry(AdvBillItemEntry * pEntry, const PPAdvBillItem * pItem, PPObjBill * pBObj)
{
	int    ok = 1;
	SString result_buf, temp_buf;
	char   acc_buf[64];
	AcctID acctid;
	Acct   acct;

	memzero(pEntry, sizeof(AdvBillItemEntry));
	THROW_INVARG(pEntry && pItem && pBObj);
	pEntry->Dt = pItem->AdvDt;
	STRNSCPY(pEntry->Code, pItem->AdvCode);
	pEntry->Amount = pItem->Amount;
	GetObjectName(PPOBJ_ADVBILLKIND, pItem->AdvBillKindID, temp_buf, 0);
	temp_buf.CopyTo(pEntry->BillKind, sizeof(pEntry->BillKind));
	acctid.ac = pItem->AccID;
	acctid.ar = pItem->ArID;
	pBObj->atobj->P_Tbl->ConvertAcctID(&acctid, &acct, 0, 0);
	acct.ToStr(ACCF_DEFAULT, acc_buf);
	GetAcctIDName(&acctid, 0, temp_buf);
	(result_buf = acc_buf).Space().Space().Cat(temp_buf).CopyTo(pEntry->Account, sizeof(pEntry->Account));
	CATCHZOK
	return ok;
}

// SetupAdvItemRowProc
static int SetWarrantItemEntry(AdvBillItemEntry * pEntry, const PPAdvBillItem * pItem, PPObjBill *)
{
	int    ok = 1;
	memzero(pEntry, sizeof(AdvBillItemEntry));
	THROW_INVARG(pEntry && pItem);
	STRNSCPY(pEntry->Memo, pItem->Memo);
	GetObjectName(PPOBJ_UNIT, pItem->ArID, pEntry->Code, sizeof(pEntry->Code));
	pEntry->Amount = pItem->Amount;
	CATCHZOK
	return ok;
}

// SetupAdvItemRowProc
static int SetDebtInventItemEntry(AdvBillItemEntry * pEntry, const PPAdvBillItem * pItem, PPObjBill *)
{
	int    ok = 1;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	memzero(pEntry, sizeof(AdvBillItemEntry));
	THROW_INVARG(pEntry && pItem);
	STRNSCPY(pEntry->Memo, pItem->Memo);
	if(ar_obj.Fetch(pItem->ArID, &ar_rec) > 0)
		STRNSCPY(pEntry->Account, ar_rec.Name);
	pEntry->Amount = pItem->Amount;
	CATCHZOK
	return ok;
}

int AdvBillItemBrowser::addItem()
{
	int    ok = -1;
	PPAdvBillItem item;
	MEMSZERO(item);
	if(editAdvBillItem(&item) > 0) {
		P_Pack->AdvList.Add(&item);
		P_Pack->CheckLargeBill(1);
		update(pos_bottom);
		ok = 1;
	}
	return ok;
}

int AdvBillItemBrowser::addItemBySample()
{
	int    ok = -1;
	int16  c = getCurItemPos();
	if(c >= 0 && c < (int16)P_Pack->AdvList.GetCount()) {
		PPAdvBillItem item = P_Pack->AdvList.Get(c);
		item.BillID = 0;
		item.RByBill = 0;
		if(editAdvBillItem(&item) > 0) {
			P_Pack->AdvList.Add(&item);
			P_Pack->CheckLargeBill(1);
			update(pos_bottom);
			ok = 1;
		}
	}
	return ok;
}

int AdvBillItemBrowser::editItem()
{
	int16  c = getCurItemPos();
	if(c >= 0 && c < (int16)P_Pack->AdvList.GetCount()) {
		PPAdvBillItem & r_item = P_Pack->AdvList.Get(c);
		if(editAdvBillItem(&r_item) > 0) {
			update(pos_cur);
			return 1;
		}
	}
	return -1;
}

int AdvBillItemBrowser::delItem()
{
	int16  c = getCurItemPos();
	if(c >= 0 && c < (int16)P_Pack->AdvList.GetCount()) {
		P_Pack->AdvList.Remove(c);
		update(pos_cur);
		return 1;
	}
	else
		return -1;
}
//
// Used by class BillDialog (BILLDLG.CPP)
//
int SLAPI ViewAdvBillDetails(PPBillPacket * pPack, PPObjBill * pBObj)
{
	int    r = -1;
	uint   res_id;
	int    (*set_entry)(AdvBillItemEntry * pEntry, const PPAdvBillItem * pItem, PPObjBill *) = 0;
	if(pPack->OpTypeID == PPOPT_ACCTURN) {
		if(GetOpSubType(pPack->Rec.OpID) == OPSUBT_WARRANT) {
			res_id    = BROWSER_WARRANTITEM;
			set_entry = SetWarrantItemEntry;
		}
		else if(GetOpSubType(pPack->Rec.OpID) == OPSUBT_DEBTINVENT) {
			res_id = BROWSER_DEBTINVITEM;
			set_entry = SetDebtInventItemEntry;
		}
		else {
			res_id    = BROWSER_ADVANCEREPITEM;
			set_entry = SetAdvBillItemEntry;
		}
		AdvBillItemBrowser * brw = new AdvBillItemBrowser(res_id, pBObj, pPack, set_entry);
		if(brw == 0)
			r = (PPError(PPERR_NOMEM, 0), 0);
		else {
			brw->update(pos_top);
			ExecViewAndDestroy(brw);
			//r = brw->IsModified ? 1 : -1;
		}
	}
	return r;
}
//
//
//
int SLAPI PPObjBill::WriteOffDebtInventory(PPID billID, int use_ta)
{
	int    ok = -1;
	uint   i;
	PPObjOprKind op_obj;
	PPOprKindPacket op_pack;
	PPBillPacket pack;
	PPIDArray wo_bill_list;
	THROW(ExtractPacket(billID, &pack) > 0);
	THROW(GetOpType(pack.Rec.OpID));
	THROW(GetOpSubType(pack.Rec.OpID) == OPSUBT_DEBTINVENT);
	THROW(op_obj.GetPacket(pack.Rec.OpID, &op_pack) > 0);
	THROW(op_pack.P_DIOE);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < pack.AdvList.GetCount(); i++) {
			PPAdvBillItem & r_item = pack.AdvList.Get(i);
			if(!(r_item.Flags & PPAdvBillItem::fWritedOff)) {
				PPBillPacket wo_pack;
				if(r_item.Amount > 0) {
					if(op_pack.P_DIOE->WrDnOp) {
						THROW(wo_pack.CreateBlank2(op_pack.P_DIOE->WrDnOp, pack.Rec.Dt, pack.Rec.LocID, 0));
						wo_pack.Rec.Amount = r_item.Amount;
						wo_pack.Amounts.Put(PPAMT_MAIN, 0L, r_item.Amount, 0, 1);
						wo_pack.Rec.Object = r_item.ArID;
						THROW(FillTurnList(&pack));
						THROW(TurnPacket(&wo_pack, 0));
					}
				}
				else if(r_item.Amount < 0) {
					if(op_pack.P_DIOE->WrUpOp) {
						THROW(wo_pack.CreateBlank2(op_pack.P_DIOE->WrUpOp, pack.Rec.Dt, pack.Rec.LocID, 0));
						wo_pack.Rec.Amount = -r_item.Amount;
						wo_pack.Amounts.Put(PPAMT_MAIN, 0L, -r_item.Amount, 0, 1);
						wo_pack.Rec.Object = r_item.ArID;
						THROW(FillTurnList(&pack));
						THROW(TurnPacket(&wo_pack, 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}


