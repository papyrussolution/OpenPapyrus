// V_FRGHT.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewFreight)
//
IMPLEMENT_PPFILT_FACTORY(Freight); FreightFilt::FreightFilt() : PPBaseFilt(PPFILT_FREIGHT, 0, 1), P_TagF(0) // @v10.3.11 ver 0-->1
{
	SetFlatChunk(offsetof(FreightFilt, ReserveStart),
		offsetof(FreightFilt, Reserve)-offsetof(FreightFilt, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(FreightFilt, P_TagF));
	Init(1, 0);
}

FreightFilt & FASTCALL FreightFilt::operator = (const FreightFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

int FreightFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		class FreightFilt_v0 : public PPBaseFilt {
		public:
			FreightFilt_v0() : PPBaseFilt(PPFILT_FREIGHT, 0, 0)
			{
				SetFlatChunk(offsetof(FreightFilt, ReserveStart),
					offsetof(FreightFilt, Reserve)-offsetof(FreightFilt, ReserveStart)+sizeof(Reserve));
				Init(1, 0);
			}
			char   ReserveStart[24]; // @anchor
			PPID   StorageLocID;     // Место хранения
			PPID   PortOfLoading;    // Пункт погрузки
			DateRange BillPeriod;
			DateRange ShipmPeriod;
			DateRange ArrvlPeriod;
			PPID   LocID;
			PPID   OpID;
			PPID   ObjectID;
			PPID   ShipID;
			PPID   PortID;           // Пункт разгрузки
			PPID   CaptainID;
			long   Flags;
			long   Order;
			long   Reserve;          // @anchor Заглушка для отмера "плоского" участка фильтра
		};
		FreightFilt_v0 fv0;
		THROW(fv0.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
		memzero(&Reserve, sizeof(Reserve));
#define CPYFLD(f) f = fv0.f
			CPYFLD(StorageLocID);
			CPYFLD(PortOfLoading);
			CPYFLD(BillPeriod);
			CPYFLD(ShipmPeriod);
			CPYFLD(ArrvlPeriod);
			CPYFLD(LocID);
			CPYFLD(OpID);
			CPYFLD(ObjectID);
			CPYFLD(ShipID);
			CPYFLD(PortID);
			CPYFLD(CaptainID);
			CPYFLD(Flags);
			CPYFLD(Order);
#undef CPYFLD
		ok = 1;
	}
	CATCHZOK
	return ok;
}

PPViewFreight::PPViewFreight() : PPView(0, &Filt, PPVIEW_FREIGHT, 0, 0), P_BObj(BillObj), P_TmpTbl(0)
{
}

PPViewFreight::~PPViewFreight()
{
	delete P_TmpTbl;
}
//
//
//
class FreightFiltDialog : public TDialog {
public:
	FreightFiltDialog() : TDialog(DLG_FRGHTFLT)
	{
		SetupCalCtrl(CTLCAL_FRGHTFLT_PERIOD, this, CTL_FRGHTFLT_PERIOD, 1);
		SetupCalCtrl(CTLCAL_FRGHTFLT_SHIPMPERIOD, this, CTL_FRGHTFLT_SHIPMPERIOD, 1);
		SetupCalCtrl(CTLCAL_FRGHTFLT_ARRVLPERIOD, this, CTL_FRGHTFLT_ARRVLPERIOD, 1);
	}
	int    setDTS(const FreightFilt *);
	int    getDTS(FreightFilt *);
private:
	DECL_HANDLE_EVENT;
	void   setupAccSheet(PPID accSheetID);

	FreightFilt Data;
};

IMPL_HANDLE_EVENT(FreightFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmTags)) { // @v10.3.11
		SETIFZ(Data.P_TagF, new TagFilt());
		if(!Data.P_TagF)
			PPError(PPERR_NOMEM);
		else if(!EditTagFilt(PPOBJ_BILL, Data.P_TagF))
			PPError();
		if(Data.P_TagF->IsEmpty())
			ZDELETE(Data.P_TagF);
	}
	else if(event.isCbSelected(CTLSEL_FRGHTFLT_OP)) {
		if(getCtrlView(CTLSEL_FRGHTFLT_OBJECT)) {
			PPID   acc_sheet_id = 0;
			getCtrlData(CTLSEL_FRGHTFLT_OP, &Data.OpID);
			GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
			setupAccSheet(acc_sheet_id);
		}
		if(!Data.OpID) {
			Data.Flags &= ~(FreightFilt::fUseCargoParam);
			setCtrlUInt16(CTL_FRGHTFLT_CARGOPAR, 0);
		}
		disableCtrl(CTL_FRGHTFLT_CARGOPAR, (Data.OpID == 0));
	}
	else if(event.isClusterClk(CTL_FRGHTFLT_CARGOPAR)) {
		if(getCtrlUInt16(CTL_FRGHTFLT_CARGOPAR) & 0x01)
			setCtrlUInt16(CTL_FRGHTFLT_FLAGS, getCtrlUInt16(CTL_FRGHTFLT_FLAGS) | 0x04);
	}
	else if(event.isClusterClk(CTL_FRGHTFLT_FLAGS)) {
		long   f = 0;
		GetClusterData(CTL_FRGHTFLT_FLAGS, &f);
		if(f & FreightFilt::fUnshippedOnly)
			f &= ~FreightFilt::fShippedOnly;
		if(f & FreightFilt::fShippedOnly)
			f &= ~FreightFilt::fUnshippedOnly;
		DisableClusterItem(CTL_FRGHTFLT_FLAGS, 0, BIN(f & FreightFilt::fShippedOnly));
		DisableClusterItem(CTL_FRGHTFLT_FLAGS, 1, BIN(f & FreightFilt::fUnshippedOnly));
		if(f != Data.Flags) {
			Data.Flags = f;
			SetClusterData(CTL_FRGHTFLT_FLAGS, Data.Flags);
		}
	}
	else
		return;
	clearEvent(event);
}

void FreightFiltDialog::setupAccSheet(PPID accSheetID)
{
	SETIFZ(accSheetID, GetSellAccSheet());
	if(getCtrlView(CTLSEL_FRGHTFLT_OBJECT)) {
		SetupArCombo(this, CTLSEL_FRGHTFLT_OBJECT, Data.ObjectID, OLW_LOADDEFONOPEN, accSheetID, sacfDisableIfZeroSheet);
		if(!accSheetID && isCurrCtlID(CTL_FRGHTFLT_OBJECT))
			selectNext();
	}
}

int FreightFiltDialog::setDTS(const FreightFilt * pData)
{
	Data = *pData;

	ushort v = 0;
	PPIDArray op_list;
	PPIDArray gen_op_list;
	PPOprKind op_rec;
	PPID   acc_sheet_id = 0;
	PPID   op_id = 0;
	SetPeriodInput(this, CTL_FRGHTFLT_PERIOD, &Data.BillPeriod);
	SetPeriodInput(this, CTL_FRGHTFLT_SHIPMPERIOD, &Data.ShipmPeriod);
	SetPeriodInput(this, CTL_FRGHTFLT_ARRVLPERIOD, &Data.ArrvlPeriod);
	for(op_id = 0; EnumOperations(/*PPOPT_GOODSEXPEND*/0, &op_id, &op_rec) > 0;)
		if(op_rec.Flags & OPKF_FREIGHT)
			op_list.add(op_id);
	for(op_id = 0; EnumOperations(PPOPT_GENERIC, &op_id, 0) > 0;) {
		gen_op_list.clear();
		if(GetGenericOpList(op_id, &gen_op_list))
			for(uint i = 0; i < gen_op_list.getCount(); i++)
				if(op_list.lsearch(gen_op_list.get(i))) {
					op_list.add(op_id);
					break;
				}
	}
	SetupOprKindCombo(this, CTLSEL_FRGHTFLT_OP, Data.OpID, 0, &op_list, OPKLF_OPLIST);
	GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
	setupAccSheet(acc_sheet_id);
	SetupPPObjCombo(this, CTLSEL_FRGHTFLT_LOC, PPOBJ_LOCATION, Data.LocID, 0);
	SetupPPObjCombo(this, CTLSEL_FRGHTFLT_SHIP, PPOBJ_TRANSPORT, Data.ShipID, 0);
	SetupPPObjCombo(this, CTLSEL_FRGHTFLT_PORT, PPOBJ_WORLD, Data.PortID, OLW_CANSELUPLEVEL|OLW_WORDSELECTOR,
		PPObjWorld::MakeExtraParam(WORLDOBJ_CITY|WORLDOBJ_CITYAREA, 0, 0)); // @v10.7.8 OLW_WORDSELECTOR
	AddClusterAssoc(CTL_FRGHTFLT_ORDER,  0, PPViewFreight::OrdByBillDate);
	AddClusterAssoc(CTL_FRGHTFLT_ORDER, -1, PPViewFreight::OrdByDefault);
	AddClusterAssoc(CTL_FRGHTFLT_ORDER,  1, PPViewFreight::OrdByArrivalDate);
	AddClusterAssoc(CTL_FRGHTFLT_ORDER,  2, PPViewFreight::OrdByDlvrAddr);
	AddClusterAssoc(CTL_FRGHTFLT_ORDER,  3, PPViewFreight::OrdByPortName);
	SetClusterData(CTL_FRGHTFLT_ORDER, Data.Order);
	{
		long   f = Data.Flags;
		if(f & FreightFilt::fUnshippedOnly)
			f &= ~FreightFilt::fShippedOnly;
		if(f & FreightFilt::fShippedOnly)
			f &= ~FreightFilt::fUnshippedOnly;
		DisableClusterItem(CTL_FRGHTFLT_FLAGS, 0, BIN(f & FreightFilt::fShippedOnly));
		DisableClusterItem(CTL_FRGHTFLT_FLAGS, 1, BIN(f & FreightFilt::fUnshippedOnly));
		Data.Flags = f;
	}
	AddClusterAssoc(CTL_FRGHTFLT_FLAGS, 0, FreightFilt::fUnshippedOnly);
	AddClusterAssoc(CTL_FRGHTFLT_FLAGS, 1, FreightFilt::fShippedOnly);
	AddClusterAssoc(CTL_FRGHTFLT_FLAGS, 2, FreightFilt::fFillLaggageFields);
	SetClusterData(CTL_FRGHTFLT_FLAGS, Data.Flags);

	v = 0;
	SetupPPObjCombo(this, CTLSEL_FRGHTFLT_CAPTAIN, PPOBJ_PERSON, Data.CaptainID, 0, reinterpret_cast<void *>(PPPRK_CAPTAIN));
	disableCtrl(CTL_FRGHTFLT_CARGOPAR, (Data.OpID == 0));
	SETFLAG(v, 0x01, (Data.Flags & FreightFilt::fUseCargoParam) && Data.OpID);
	setCtrlData(CTL_FRGHTFLT_CARGOPAR, &v);
	return 1;
}

int FreightFiltDialog::getDTS(FreightFilt * pData)
{
	int    ok = 1;
	uint   sel = 0;
	ushort v = 0;
	THROW(GetPeriodInput(this, sel = CTL_FRGHTFLT_PERIOD, &Data.BillPeriod));
	THROW(GetPeriodInput(this, sel = CTL_FRGHTFLT_SHIPMPERIOD, &Data.ShipmPeriod));
	THROW(GetPeriodInput(this, sel = CTL_FRGHTFLT_ARRVLPERIOD, &Data.ArrvlPeriod));
	getCtrlData(CTLSEL_FRGHTFLT_OP, &Data.OpID);
	getCtrlData(CTLSEL_FRGHTFLT_OBJECT, &Data.ObjectID);
	getCtrlData(CTLSEL_FRGHTFLT_LOC, &Data.LocID);
	getCtrlData(CTLSEL_FRGHTFLT_SHIP, &Data.ShipID);
	getCtrlData(CTLSEL_FRGHTFLT_PORT, &Data.PortID);
	getCtrlData(CTLSEL_FRGHTFLT_CAPTAIN, &Data.CaptainID); // AHTOXA
	GetClusterData(CTL_FRGHTFLT_ORDER, &Data.Order);
	GetClusterData(CTL_FRGHTFLT_FLAGS, &Data.Flags);
	v = getCtrlUInt16(CTL_FRGHTFLT_CARGOPAR);
	SETFLAG(Data.Flags, FreightFilt::fUseCargoParam, (v & 0x01) && Data.OpID);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int PPViewFreight::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return 0;
	FreightFilt * p_filt = static_cast<FreightFilt *>(pFilt);
	DIALOG_PROC_BODY(FreightFiltDialog, p_filt);
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempFreight);

int PPViewFreight::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.BillPeriod.Actualize(ZERODATE);
	Filt.ShipmPeriod.Actualize(ZERODATE);
	Filt.ArrvlPeriod.Actualize(ZERODATE);
	StrPool.ClearS();
	{
		ZDELETE(P_TmpTbl);
		THROW(P_TmpTbl = CreateTempFile());
		{
			PPViewBill v_bill;
			BillFilt bill_filt;
			BillViewItem bill_item;
			bill_filt.SetupBrowseBillsType(bbtGoodsBills);
			bill_filt.Period = Filt.BillPeriod;
			bill_filt.LocList.Add(Filt.LocID);
			bill_filt.OpID = Filt.OpID;
			bill_filt.ObjectID = Filt.ObjectID;
			// @v10.3.11 {
			if(Filt.P_TagF) {
				ZDELETE(bill_filt.P_TagF);
				bill_filt.P_TagF = new TagFilt(*Filt.P_TagF);
			}
			// } @v10.3.11 
			if(!(Filt.Flags & FreightFilt::fUseCargoParam)) // AHTOXA
				bill_filt.Flags |= BillFilt::fFreightedOnly;
			THROW(v_bill.Init_(&bill_filt));
			{
				int    r = 1;
				UintHashTable fr_bill_list;
				TempFreightTbl::Rec rec;
				BExtInsert bei(P_TmpTbl);
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				{
					//
					// Извлечение списка фрахтов по всем документам ускоряет расчет
					// только в том случае, если общий список документов, подпадающий
					// под действие общих критериев фильтра (не относящихся к фрахту)
					// достаточно велик.
					//
					// Таким образом, если в фильтре указан контрагент либо установлен
					// период документов до полугода, то применяем перебор "по документам",
					// в противном случае пытаемся "по фрахтам" (функция GetListByFreightFilt
					// сама определит возможность такого перебора).
					//
					if(Filt.ObjectID)
						r = -1;
					else {
						long   days = 0;
						LDATE  lo = Filt.BillPeriod.low;
						LDATE  up = Filt.BillPeriod.upp;
						if(lo) {
							SETIFZ(up, getcurdate_());
							if(labs(diffdate(up, lo)) <= 185)
								r = -1;
						}
					}
				}
				r = (r < 0) ? -1 : P_BObj->P_Tbl->GetListByFreightFilt(Filt, fr_bill_list);
				THROW(r);
				if(r > 0) {
					for(ulong uid = 0; fr_bill_list.Enum(&uid);) {
						const PPID bill_id = static_cast<PPID>(uid);
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(bill_id, &bill_rec) > 0 && v_bill.CheckIDForFilt(bill_id, 0)) {
							if(FillTempTableRec(&bill_rec, &rec) > 0)
								THROW_DB(bei.insert(&rec));
						}
					}
				}
				else {
					for(v_bill.InitIteration(PPViewBill::OrdByDefault); v_bill.NextIteration(&bill_item) > 0;) {
						if(FillTempTableRec(&bill_item, &rec) > 0)
							THROW_DB(bei.insert(&rec));
					}
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_TmpTbl);
	ENDCATCH
	return ok;
}

int PPViewFreight::FillTempTableRec(const BillTbl::Rec * pBillRec, TempFreightTbl::Rec * pRec)
{
	int    ok = -1;
	int    is_freight = 0, is_cargo = 0;
	BillTotalData btd;
	PPBillPacket pack;
	SString temp_buf;
	if((!(Filt.Flags & FreightFilt::fUnshippedOnly) || !(pBillRec->Flags & BILLF_SHIPPED)) &&
		(!(Filt.Flags & FreightFilt::fShippedOnly) || (pBillRec->Flags & BILLF_SHIPPED))) {
		PPFreight freight;
		if(P_BObj->P_Tbl->GetFreight(pBillRec->ID, &freight) > 0)
			is_freight = 1;
		else if(Filt.Flags & FreightFilt::fUseCargoParam)
			if(P_BObj->ExtractPacket(pBillRec->ID, &pack) > 0) {
				pack.CalcTotal(&btd, 0);
				is_cargo = BIN(btd.Brutto > 0.0 || btd.Volume > 0.0 || btd.PackCount > 0.0);
			}
		if((is_freight || is_cargo) && freight.CheckForFilt(Filt)) {
			Goods2Tbl::Rec tr_rec;
			WorldTbl::Rec city_rec;
			memzero(pRec, sizeof(*pRec));
			pRec->BillID     = pBillRec->ID;
			pRec->BillDate   = pBillRec->Dt;
			STRNSCPY(pRec->Code, pBillRec->Code);
			StrPool.AddS(pBillRec->Memo, &pRec->MemoP);
			pRec->LocID      = pBillRec->LocID;
			pRec->ObjectID   = pBillRec->Object;
			pRec->Amount     = BR2(pBillRec->Amount);
			pRec->Shipped[0] = (pBillRec->Flags & BILLF_SHIPPED) ? 'X' : 0;
			pRec->Shipped[1] = 0;
			pRec->ShipmDate  = freight.IssueDate;
			pRec->ArrvlDate  = freight.ArrivalDate;
			pRec->ShipID     = freight.ShipID;
			pRec->PortID     = freight.PortOfDischarge;
			pRec->DlvrAddrID = freight.DlvrAddrID;
			if(pRec->ShipID && TrObj.Search(pRec->ShipID, &tr_rec) > 0) {
				StrPool.AddS(tr_rec.Name, &pRec->ShipNameP);
			}
			if(pRec->PortID && WObj.Fetch(pRec->PortID, &city_rec) > 0)
				STRNSCPY(pRec->PortName, city_rec.Name);
			if(pRec->DlvrAddrID) {
				LocObj.GetAddress(pRec->DlvrAddrID, 0, temp_buf);
				temp_buf.CopyTo(pRec->DlvrAddr, sizeof(pRec->DlvrAddr));
			}
			if(pack.Rec.ID) {
				pRec->Brutto = btd.Brutto;
				pRec->Volume = btd.Volume;
				pRec->PackCount = btd.PackCount;
				pRec->AgentID = pack.Ext.AgentID;
			}
			else {
				if(Filt.Flags & FreightFilt::fFillLaggageFields) {
					if(P_BObj->ExtractPacket(pBillRec->ID, &pack) > 0) {
						pack.CalcTotal(&btd, 0);
						pRec->Brutto = btd.Brutto;
						pRec->Volume = btd.Volume;
						pRec->PackCount = btd.PackCount;
						pRec->AgentID = pack.Ext.AgentID;
					}
				}
				else {
					PPBillExt ext;
					if(P_BObj->P_Tbl->GetExtraData(pBillRec->ID, &ext) > 0)
						pRec->AgentID = ext.AgentID;
				}
			}
			ok = 1;
		}
	}
	return ok;
}

int PPViewFreight::UpdateTempTableRec(PPID billID)
{
	int    ok = 1;
	PPID   k = billID;
	PPTransaction tra(ppDbDependTransaction, 1);
	THROW(tra);
	if(P_TmpTbl && P_TmpTbl->searchForUpdate(0, &k, spEq)) {
		TempFreightTbl::Rec rec;
		BillTbl::Rec bill_rec;
		if(P_BObj->Search(billID, &bill_rec) > 0 && FillTempTableRec(&bill_rec, &rec) > 0) {
			THROW_DB(P_TmpTbl->updateRecBuf(&rec)); // @sfu
		}
		else
			THROW_DB(P_TmpTbl->deleteRec()); // @sfu
	}
	else
		ok = -1;
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

int PPViewFreight::InitIteration(IterOrder order)
{
	int    ok = 1, idx = 0;
	// @v10.6.8 char   k[MAXKEYLEN];
	BtrDbKey k_; // @v10.6.8 
	PPInitIterCounter(Counter, P_TmpTbl);
	switch(order) {
		case OrdByDefault: idx = 0; break;
		case OrdByBillID: idx = 0; break;
		case OrdByBillDate: idx = 1; break;
		case OrdByArrivalDate: idx = 2; break;
		case OrdByPortName: idx = 3; break;
		case OrdByDlvrAddr: idx = 4; break;
	}
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(P_TmpTbl, idx));
	P_IterQuery->selectAll();
	// @v10.6.8 @ctr memzero(k, sizeof(k));
	THROW(P_IterQuery->initIteration(0, k_, spFirst));
	CATCHZOK
	return ok;
}

int FASTCALL PPViewFreight::NextIteration(FreightViewItem * pItem)
{
	SString temp_buf;
	if(P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		if(pItem) {
			memzero(pItem, sizeof(*pItem));
			pItem->BillID    = P_TmpTbl->data.BillID;
			pItem->BillDate  = P_TmpTbl->data.BillDate;
			STRNSCPY(pItem->Code, P_TmpTbl->data.Code);
			pItem->ObjectID  = P_TmpTbl->data.ObjectID;
			pItem->Amount    = P_TmpTbl->data.Amount;
			pItem->IsShipped = BIN(P_TmpTbl->data.Shipped[0]);
			pItem->Brutto    = P_TmpTbl->data.Brutto;
			pItem->PackCount = P_TmpTbl->data.PackCount;
			pItem->Volume    = P_TmpTbl->data.Volume;
			pItem->ShipmDate = P_TmpTbl->data.ShipmDate;
			pItem->ArrvlDate = P_TmpTbl->data.ArrvlDate;
			pItem->ShipID    = P_TmpTbl->data.ShipID;
			// @v9.8.4 STRNSCPY(pItem->ShipName, P_TmpTbl->data.ShipName);
			StrPool.GetS(P_TmpTbl->data.ShipNameP, temp_buf); // @v9.8.4 
			STRNSCPY(pItem->ShipName, temp_buf); // @v9.8.4 
			pItem->PortID    = P_TmpTbl->data.PortID;
			STRNSCPY(pItem->PortName, P_TmpTbl->data.PortName);
			pItem->DlvrAddrID = P_TmpTbl->data.DlvrAddrID;
			STRNSCPY(pItem->DlvrAddr, P_TmpTbl->data.DlvrAddr);
		}
		return 1;
	}
	else
		return -1;
}

int PPViewFreight::ViewTotal()
{
	int    ok = -1;
	long   count = 0;
	double amount = 0.0;
	double pack_count = 0.0;
	double brutto = 0.0;
	double volume = 0.0;
	FreightViewItem item;
	TDialog * dlg = 0;
	for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
		count++;
		amount += item.Amount;
		pack_count += item.PackCount;
		brutto += item.Brutto;
		volume += item.Volume;
	}
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_FRGHTTOTAL)))) {
		dlg->setCtrlLong(CTL_FRGHTTOTAL_COUNT,  count);
		dlg->setCtrlReal(CTL_FRGHTTOTAL_AMOUNT, amount);
		dlg->setCtrlReal(CTL_FRGHTTOTAL_PACKCOUNT, pack_count);
		dlg->setCtrlReal(CTL_FRGHTTOTAL_BRUTTO, brutto);
		dlg->setCtrlReal(CTL_FRGHTTOTAL_VOLUME, volume);
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

DBQuery * PPViewFreight::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = (Filt.Flags & FreightFilt::fFillLaggageFields) ? BROWSER_FREIGHT_LAGG : BROWSER_FREIGHT;
	TempFreightTbl * tbl = new TempFreightTbl(P_TmpTbl->GetName());
	DBE    dbe_loc;
	DBE    dbe_ar;
	DBE    dbe_agent;
	DBE    dbe_ship; // @v9.8.4 
	DBE    dbe_memo; // @v9.8.4 
	DBQuery * q = 0;
	THROW(CheckTblPtr(tbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_ar,    PPDbqFuncPool::IdObjNameAr,  tbl->ObjectID);
	PPDbqFuncPool::InitObjNameFunc(dbe_agent, PPDbqFuncPool::IdObjNameAr,  tbl->AgentID);
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc, tbl->LocID);
	PPDbqFuncPool::InitStrPoolRefFunc(dbe_ship, tbl->ShipNameP, &StrPool); // @v9.8.4 
	PPDbqFuncPool::InitStrPoolRefFunc(dbe_memo, tbl->MemoP, &StrPool); // @v9.8.4 
	q = & select(
		tbl->BillID,    //  #0
		tbl->BillDate,  //  #1
		tbl->Code,      //  #2
		dbe_ar,         //  #3
		tbl->ArrvlDate, //  #4
		// @v9.8.4 tbl->ShipName,  //  #5 @v9.8.4
		dbe_ship,       //  #5 
		tbl->PortName,  //  #6
		tbl->DlvrAddr,  //  #7
		tbl->Amount,    //  #8
		tbl->Brutto,    //  #9
		tbl->Volume,    // #10
		tbl->PackCount, // #11
		tbl->Shipped,   // #12
		dbe_loc,        // #13
		dbe_agent,      // #14
		// @v9.8.4 tbl->Memo,      // #15
		dbe_memo,       // #15 @v9.8.4
		0L).from(tbl, 0L);
	if(Filt.Order == OrdByBillID)
		q->orderBy(tbl->BillID, 0L);
	else if(oneof2(Filt.Order, OrdByDefault, OrdByBillDate))
		q->orderBy(tbl->BillDate, 0L);
	else if(Filt.Order == OrdByArrivalDate)
		q->orderBy(tbl->ArrvlDate, 0L);
	else if(Filt.Order == OrdByPortName)
		q->orderBy(tbl->PortName, 0L);
	else if(Filt.Order == OrdByDlvrAddr)
		q->orderBy(tbl->DlvrAddr, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else
			delete tbl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void PPViewFreight::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && Filt.LocID == 0) {
		// @v9.0.2 pBrw->InsColumnWord(1, PPWORD_WAREHOUSE, 13, 0, 0, 0);
		pBrw->InsColumn(1, "@warehouse", 13, 0, 0, 0); // @v9.0.2
	}
}

int PPViewFreight::GetBillList(ObjIdListFilt * pList)
{
	int    ok = -1;
	PPWait(1);
	if(pList) {
		FreightViewItem item;
		for(InitIteration(static_cast<PPViewFreight::IterOrder>(Filt.Order)); NextIteration(&item) > 0;) {
			pList->Add(item.BillID);
			PPWaitPercent(GetCounter());
		}
		ok = 1;
	}
	PPWait(0);
	return ok;
}

int PPViewFreight::Print(const void *)
{
	return Helper_Print(((Filt.LocID == 0) ? REPORT_FREIGHTLAGGLOC : REPORT_FREIGHTLAGG), Filt.Order);
}

int PPViewFreight::PrintBill(PPID billID/* @v10.0.0, int addCashSummator*/)
{
	PPViewBill b_v;
	return b_v.PrintBill(billID/* @v10.0.0, addCashSummator*/);
}

int PPViewFreight::PrintBillList()
{
	int    ok = -1;
	BillFilt bill_filt;
	GetBillList(&bill_filt.List);
	if(!bill_filt.List.IsEmpty()) {
		PPViewBill v_b;
		bill_filt.OpID = Filt.OpID;
		THROW(v_b.Init_(&bill_filt));
		v_b.Print();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewFreight::PrintBillInfoList()
{
	int    ok = -1;
	BillFilt bill_filt;
	GetBillList(&bill_filt.List);
	if(!bill_filt.List.IsEmpty()) {
		PPViewBill v_b;
		bill_filt.OpID = Filt.OpID;
		THROW(v_b.Init_(&bill_filt));
		v_b.PrintBillInfoList();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;

}

int PPViewFreight::PrintAllBills()
{
	int    ok = -1;
	BillFilt bill_filt;
	GetBillList(&bill_filt.List);
	if(!bill_filt.List.IsEmpty()) {
		PPViewBill v_b;
		bill_filt.OpID = Filt.OpID;
		THROW(v_b.Init_(&bill_filt));
		v_b.PrintAllBills();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewFreight::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   bill_id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(bill_id && P_BObj->Edit(&bill_id, 0) == cmOK) {
		if(UpdateTempTableRec(bill_id) > 0)
			return 1;
	}
	return -1;
}

int PPViewFreight::Export()
{
	int    ok = -1, r = 0;
	PPIDArray bill_id_list;
	FreightViewItem item;
	PPBillPacket pack;
	PPBillExporter b_e;
	for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
		BillTbl::Rec bill_rec;
		// @v9.4.3 if(P_BObj->Fetch(item.BillID, &bill_rec) > 0 && ((bill_rec.Flags & BILLF_GOODS) || IsDraftOp(bill_rec.OpID))) {
		if(P_BObj->Fetch(item.BillID, &bill_rec) > 0 && IsGoodsDetailOp(bill_rec.OpID)) { // @v9.4.3
			bill_id_list.add(bill_rec.ID);
		}
	}
	if(bill_id_list.getCount()) {
		THROW(P_BObj->ExtractPacket(bill_id_list.get(0), &pack) > 0);
		THROW(r = b_e.Init(0, 0, &pack, 0));
		if(r > 0) {
			PPWait(1);
			for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
				const PPID bill_id = bill_id_list.get(_idx);
				THROW(P_BObj->ExtractPacket(bill_id, &pack) > 0);
				THROW(b_e.PutPacket(&pack));
				PPWaitPercent(_idx+1, bill_id_list.getCount());
			}
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int PPViewFreight::UpdateFeatures()
{
    int    ok = -1;
    PPID   tr_type = PPTRTYP_CAR;
    PPID   ship_id = 0;
	PPID   captain_id = 0; // @v10.0.11
    long   shipm_flag_mode = -1;
    LDATE  issue_date = ZERODATE;
    LDATE  arrival_date = ZERODATE;
    TDialog * dlg = new TDialog(DLG_UPDFREIGHT);
    THROW(CheckDialogPtr(&dlg));
	SetupPPObjCombo(dlg, CTLSEL_UPDFREIGHT_TR, PPOBJ_TRANSPORT, ship_id, OLW_CANINSERT, reinterpret_cast<void *>(tr_type));
	SetupPPObjCombo(dlg, CTLSEL_UPDFREIGHT_CAPT, PPOBJ_PERSON, captain_id, OLW_CANINSERT/*|OLW_LOADDEFONOPEN*/, reinterpret_cast<void *>(PPPRK_CAPTAIN)); // @v10.0.11
	dlg->SetupCalDate(CTLCAL_UPDFREIGHT_ISSDT, CTL_UPDFREIGHT_ISSDT);
	dlg->SetupCalDate(CTLCAL_UPDFREIGHT_ARRDT, CTL_UPDFREIGHT_ARRDT);
	dlg->setCtrlData(CTL_UPDFREIGHT_ISSDT, &issue_date);
	dlg->setCtrlData(CTL_UPDFREIGHT_ARRDT, &arrival_date);
	dlg->AddClusterAssocDef(CTL_UPDFREIGHT_SHPF,  0, -1);
	dlg->AddClusterAssoc(CTL_UPDFREIGHT_SHPF,  1,  1);
	dlg->AddClusterAssoc(CTL_UPDFREIGHT_SHPF,  2,  0);
	dlg->SetClusterData(CTL_UPDFREIGHT_SHPF, shipm_flag_mode);
	if(ExecView(dlg) == cmOK) {
		ship_id = dlg->getCtrlLong(CTLSEL_UPDFREIGHT_TR);
		captain_id = dlg->getCtrlLong(CTLSEL_UPDFREIGHT_CAPT); // @v10.0.11
        issue_date = dlg->getCtrlDate(CTL_UPDFREIGHT_ISSDT);
        arrival_date = dlg->getCtrlDate(CTL_UPDFREIGHT_ARRDT);
        shipm_flag_mode = dlg->GetClusterData(CTL_UPDFREIGHT_SHPF);
        if(ship_id || captain_id || checkdate(issue_date) || checkdate(arrival_date) || oneof2(shipm_flag_mode, 0, 1)) {
			PPIDArray bill_list;
			PPWait(1);
			{
				FreightViewItem item;
				for(InitIteration(OrdByDefault); NextIteration(&item) > 0;)
					bill_list.add(item.BillID);
			}
			bill_list.sortAndUndup();
			for(uint i = 0; i < bill_list.getCount(); i++) {
				BillTbl::Rec bill_rec;
				const  PPID bill_id = bill_list.get(i);
				int    do_update = 0;
				if(P_BObj->Search(bill_id, &bill_rec) > 0) {
                    PPFreight freight;
                    if(P_BObj->P_Tbl->GetFreight(bill_id, &freight) > 0) {
                        if(ship_id && freight.ShipID != ship_id) {
							freight.ShipID = ship_id;
							do_update |= 1;
                        }
						// @v10.0.11 {
						if(captain_id && freight.CaptainID != captain_id) {
							freight.CaptainID = captain_id;
							do_update |= 1;
						}
						// } @v10.0.11 
                        if(checkdate(issue_date) && freight.IssueDate != issue_date) {
							freight.IssueDate = issue_date;
							do_update |= 1;
                        }
                        if(checkdate(arrival_date) && freight.ArrivalDate != arrival_date) {
							freight.ArrivalDate = arrival_date;
							do_update |= 1;
                        }
                    }
                    if(oneof2(shipm_flag_mode, 0, 1)) {
						if(shipm_flag_mode == 0 && bill_rec.Flags & BILLF_SHIPPED) {
							bill_rec.Flags &= ~BILLF_SHIPPED;
							do_update |= 2;
						}
						else if(shipm_flag_mode == 1 && !(bill_rec.Flags & BILLF_SHIPPED)) {
							bill_rec.Flags |= BILLF_SHIPPED;
							do_update |= 2;
						}
                    }
                    if(do_update) {
						int    bill_updated = 0;
						PPTransaction tra(1);
						THROW(tra);
						if(do_update & 1) {
							THROW(P_BObj->P_Tbl->SetFreight(bill_id, &freight, 0));
							DS.LogAction(PPACN_UPDBILLFREIGHT, PPOBJ_BILL, bill_id, 0, 0);
						}
						if(do_update & 2) {
							THROW(P_BObj->P_Tbl->SetRecFlag(bill_id, BILLF_SHIPPED, shipm_flag_mode, 0));
							bill_updated = 1;
						}
						if(bill_updated)
							DS.LogAction(PPACN_UPDBILL, PPOBJ_BILL, bill_id, 0, 0);
						UpdateTempTableRec(bill_id);
						THROW(tra.Commit());
						ok = 1;
                    }
				}
				PPWaitPercent(i+1, bill_list.getCount());
			}
			PPWait(0);
        }
	}
    CATCHZOKPPERR
    delete dlg;
    return ok;
}

int PPViewFreight::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_TOGGLE:
				ok = -1;
				if(id) {
					if(P_BObj->CheckRights(PPR_MOD) && P_BObj->P_Tbl->SetShippedTag(id, -1)) {
						if(UpdateTempTableRec(id) > 0)
							ok = 1;
					}
					else
						ok = PPErrorZ();
				}
				break;
			case PPVCMD_REPLACEMENT: // @v9.4.3
				ok = -1;
				if(UpdateFeatures() > 0)
					ok = 1;
				break;
			case PPVCMD_EDITBILLFREIGHT: ok = P_BObj->EditBillFreight(id); break;
			case PPVCMD_PRINTBILL: PrintBill(id/* @v10.0.0, 0*/); break;
			// @v10.0.0 case PPVCMD_PRINTCHECK: PrintBill(id, 1); break;
			case PPVCMD_PRINTBILLLIST: ok = PrintBillList(); break;
			case PPVCMD_PRINTALLBILLS: ok = PrintAllBills(); break;
			case PPVCMD_PRINTBILLINFOLIST: ok = PrintBillInfoList(); break;
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_SYSJ: // @v9.4.3
				if(id)
					ViewSysJournal(PPOBJ_BILL, id, 0);
				break;
			case PPVCMD_DORECOVER:
				ok = -1;
				if(CONFIRM(PPCFM_RECOVERFREIGHT))
					P_BObj->RecoverUnitedFreightPorts();
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_FreightList
//
PPALDD_CONSTRUCTOR(FreightList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(FreightList) { Destroy(); }

int PPALDD_FreightList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Freight, rsrv);
	H.FltBillBeg = p_filt->BillPeriod.low;
	H.FltBillEnd = p_filt->BillPeriod.upp;
	H.FltShipmBeg = p_filt->ShipmPeriod.low;
	H.FltShipmEnd = p_filt->ShipmPeriod.upp;
	H.FltArrvlBeg = p_filt->ArrvlPeriod.low;
	H.FltArrvlEnd = p_filt->ArrvlPeriod.upp;
	H.FltLocID = p_filt->LocID;
	H.FltOpID  = p_filt->OpID;
	H.FltObjectID = p_filt->ObjectID;
	H.FltPortID = p_filt->PortID;
	H.FltFlags  = p_filt->Flags;
	H.fUnshippedOnly     = BIN(p_filt->Flags & FreightFilt::fUnshippedOnly);
	H.fShippedOnly       = BIN(p_filt->Flags & FreightFilt::fShippedOnly);
	H.fFillLaggageFields = BIN(p_filt->Flags & FreightFilt::fFillLaggageFields);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_FreightList::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(Freight, static_cast<PPViewFreight::IterOrder>(sortId));
}

int PPALDD_FreightList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Freight);
	I.BillID = item.BillID;
	I.ObjectID = item.ObjectID;
	I.BillDate = item.BillDate;
	I.ArrvlDate = item.ArrvlDate;
	I.ShipmDate = item.ShipmDate;
	I.IsShipped = item.IsShipped;
	I.PortID = item.PortID;
	I.DlvrAddrID = item.DlvrAddrID;
	I.Amount = item.Amount;
	I.Brutto = item.Brutto;
	I.Volume = item.Volume;
	I.PackCount = item.PackCount;
	STRNSCPY(I.BillCode, item.Code);
	STRNSCPY(I.ShipName, item.ShipName);
	STRNSCPY(I.PortName, item.PortName);
	STRNSCPY(I.DlvrAddr, item.DlvrAddr);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_FreightList::Destroy() { DESTROY_PPVIEW_ALDD(Freight); }

