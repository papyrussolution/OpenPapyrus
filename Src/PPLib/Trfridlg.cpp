// TRFRIDLG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// Fully private class
// Accessible from EditTransferItem only.
//
class TrfrItemDialog : public TDialog {
private:
	friend int SLAPI EditTransferItem(PPBillPacket *, int itemNo, TIDlgInitData *, const PPTransferItem * pOrder, int sign);

	TrfrItemDialog(uint dlgID, PPID opID);
	int    setDTS(PPTransferItem *);
	int    getDTS(PPTransferItem *, double * pExtraQtty);

	DECL_HANDLE_EVENT;
	void   selectLot();
	int    addLotEntry(SArray *, const ReceiptTbl::Rec *);
	void   replyGoodsGroupSelection();
	int    replyGoodsSelection(int recurse = 1);
	void   setupQuantity(uint master, int readFlds);
	int    getCtrlCost();
	int    setCtrlCost();
	void   setupQttyFldPrec();
	void   setupCtrls();
	void   setupCtrlsOnGoodsSelection();
	void   setupRest();
	void   setupVaPct();
	void   setupVatSum();
	void   setupBaseQuot();
	void   setupPriceLimit();
	int    setupLot();
	int    setupValuation();
	void   SetupSerialWarn();
	void   SetupInheritedSerial();
	int    evaluateBasePrice(double curPrice, double * pBasePrice);
	void   setupCurPrice();
	void   setupQuotation(int reset, int autoQuot);
	int    setupGoodsList();
	int    setupGoodsListByPrice();
	int    setupManuf();
	int    getManuf();
	int    selectGoodsByBarCode();
	void   calcOrderRest();
	void   recalcUnitsToPhUnits();
	void   editQCertData();
	void   GenerateSerial(); // called by handleEvent() on F2 (P_Current->Id == CTL_LOT_SERIAL)
	void   calcPrice();
	int    isModifPlus() const;
	PPID   GetQuotLocID();
	int    CheckPrice();
	int    GetPriceRestrictions(RealRange * pRange);
	void   SetupQttyByLotDimTag();
	//
	// Descr: Возвращает !0 если с редактируемой строкой может быть сопоставлен серийный номер или иные теги.
	//   Применяется для определения возможности редактировать серийный номер или иные теги.
	//
	int    IsTaggedItem() const
	{
		return BIN(oneof3(P_Pack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, PPOPT_GOODSORDER) || isModifPlus());
	}
	int    IsSourceSerialUsed();
	int    GetGoodsListSuitableForSourceSerial(PPID goodsID, PPIDArray & rList);

	PPObjBill * P_BObj;
	Transfer  * P_Trfr;
	PPObjGoodsType GTObj;
	PPObjGoods     GObj;
	PPObjTag       TagObj;
	PPBillPacket * P_Pack;
	PPTransferItem Item;
	const PPTransferItem * P_OrderItem; // Указатель на строку заказа. Передается извне и не изменяется методами класса.
	int    ItemNo;
	int    EditMode;
	enum {
		stModified      = 0x0001,
		stGoodsByPrice  = 0x0002, // Признак выбора товара по цене
		stAllowSupplSel = 0x0004, // Позволяет выбор поставщика
		stGoodsFixed    = 0x0008, // Фиксированный товар (выбран до входа в диалог)
		stWasCostInput  = 0x0010
	};
	long   St;
	enum {
		dummyFirst = 1,
		brushChangedCost,    // Цвет поля цены поступления, если значение было изменено по сравнению с последним лотом
		brushQuotedPrice,    // Цвет поля цены реализации, если на строку установлена котировка
		brushPriceBelowCost, // Цвет поля цены реализации, если цена не по котировке и ниже цены поступления //
		brushQuotedPriceNoCancel, // Цвет поля цены реализации, если на строку установлена котировка без права отмены пользователем
		brushVetisUuidExists, // Позитивный цвет индикатора наличия сертификата ВЕТИС
		brushVetisUuidAbsence // Негативный цвет индикатора наличия сертификата ВЕТИС
	};
	SPaintToolBox Ptb;     //
	PPID   InitGoodsGrpID; //
	PPID   GoodsGrpID;     //
	const  PPID OpID;
	const  PPID OpTypeID;
	PPID   SelectedPckgID;
	double Rest;
	double TotalRest;
	double NumPacks;
	double Price;
	double OrdRest;        // Заказанное количество товара
	double OrdReserved;    // Зарезервированное количество (OrdReserved <= OrdRest)
	double MinQtty;        // Минимальное количество, которое может быть введено
	double MaxQtty;        // Максимальное количество, которое может быть введено
	double OrgQtty;        // Для корректирующего документа (Item.Flags & PPTFR_CORRECTION) - количество,
		// поступившее в оригинальном документе.
	double OrgPrice;       // @v9.4.3 Для корректирующего документа (Item.Flags & PPTFR_CORRECTION) - Чистая цена реализации
		// в оригинальном документе
	PPSupplDeal Sd;        // Контрактная цена по поставщику
	enum {
		strNoVAT     = 0,
		strComplete  = 1,
		strOrderQtty = 2
	};
	SString Strings;
	ObjTagList InheritedLotTagList; // Список тегов, унаследованных от предыдущего лота того же товара
		// Список необходимо отделить от общего пула тегов из-за необходимость отличать создаваемые в ручную
		// теги от тегов унаследованных.

	int    readQttyFld(uint master, uint ctl, double * val);
	int    checkQuantityForIntVal();
	int    checkQuantityVal(double * pExtraQtty);
	int    isDiscountInSum() const;
	int    isAllowZeroPrice();
	int    setupAllQuantity(int byLot);
	void   setQuotSign();
	SArray * SelectGoodsByPrice(PPID loc, double price);
	int    ProcessRevalOnAllLots(const PPTransferItem *);
};

static int SLAPI CanUpdateSuppl(const PPBillPacket * pBp, int itemNo)
{
	int   yes = 0;
	if(pBp->OpTypeID == PPOPT_GOODSRECEIPT) {
		if(pBp->Rec.Object == 0 /*|| (p->AccSheet && p->AccSheet != GetSupplAccSheet())*/)
			yes = 1;
		else if(itemNo >= 0 && pBp->TI(itemNo).Flags & PPTFR_FORCESUPPL)
			yes = 1;
	}
	return yes;
}

int SLAPI ViewSpoilList(SpecSeriesCore * pTbl, const char * pSerial, int useText)
{
	int    ok = -1;
	PPListDialog * dlg = new PPListDialog(DLG_SPOILLIST, CTL_SPOILLIST_LIST);
	if(CheckDialogPtrErr(&dlg)) {
		StrAssocArray name_list;
		dlg->setCtrlString(CTL_SPOILLIST_SERIAL, pSerial);
		pTbl->GetListBySerial(SPCSERIK_SPOILAGE, pSerial, &name_list);
		for(uint i = 0; i < name_list.getCount(); i++)
			dlg->addStringToList(i+1, name_list.Get(i).Txt);
		if(!useText)
			dlg->showCtrl(CTL_SPOILLIST_TEXT, 0);
		if(ExecViewAndDestroy(dlg) == cmOK)
			ok = 1;
	}
	else
		ok = 0;
	return ok;
}

int TrfrItemDialog::ProcessRevalOnAllLots(const PPTransferItem * pItem)
{
	int    ok = 1, r;
	if(pItem->Flags & PPTFR_REVAL && P_Pack->Rec.Object == 0) {
		uint   i;
		PPIDArray list;
		ReceiptTbl::Rec lot_rec;
		DateIter di(0, pItem->Date);
		while((r = P_Trfr->Rcpt.EnumLots(pItem->GoodsID, pItem->LocID, &di, &lot_rec)) > 0)
			if(!P_Pack->SearchLot(lot_rec.ID, &(i = 0)) && R5(lot_rec.Price) != pItem->Price)
				list.add(lot_rec.ID);
		THROW(r);
		if(list.getCount() && CONFIRM(PPCFM_REVALALLLOTS)) {
			for(i = 0; i < list.getCount(); i++) {
				PPTransferItem item(&P_Pack->Rec, TISIGN_UNDEF);
				THROW(item.SetupGoods(pItem->GoodsID, 0));
				item.SetupLot(list.get(i), 0, 0);
				if(item.Price != pItem->Price) {
					item.RevalCost = item.Cost;
					item.Discount = item.Price;
					item.Price    = pItem->Price;
					THROW(P_Pack->BoundsByLot(item.LotID, &item, -1, &item.Rest_, 0));
					THROW(P_Pack->InsertRow(&item, 0, 0));
					P_Pack->LTagL.ReplacePosition(-1, P_Pack->GetTCount() - 1); // @v9.8.11
					// @v9.8.11 P_Pack->ClbL.ReplacePosition(-1, P_Pack->GetTCount() - 1);
					// @v9.8.11 P_Pack->SnL.ReplacePosition(-1, P_Pack->GetTCount() - 1);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// Если itemNo == -1, то добавляется новая строка
//
int SLAPI EditTransferItem(PPBillPacket * pPack, int itemNo, TIDlgInitData * pInitData, const PPTransferItem * pOrder, int sign)
{
	const PPConfig & r_cfg = LConfig;
	const int    allow_suppl_sel = BIN(CanUpdateSuppl(pPack, itemNo) && BillObj->CheckRights(BILLOPRT_ACCSSUPPL, 1));
	const int    goods_fixed     = BIN(itemNo >= 0 || (pInitData && pInitData->GoodsID));
	const PPID   op_id = pPack->Rec.OpID;
	int    r = cmCancel;
	int    skip_dlg = (pInitData && (pInitData->Flags & TIDIF_AUTOQTTY));
	int    goods_by_price  = 0;
	int    valid_data = 0;
	int    modified = 0;
	uint   i, dlg_id = 0;
	int    rt_to_modif = 1;
	SpecSeriesCore * p_spc_core = 0;
	PPOprKind op_rec;
	PPTransferItem pattern, * p_item = 0, * p_ti = 0;
	TrfrItemDialog * dlg = 0;
	switch(pPack->OpTypeID) {
		case PPOPT_GOODSRETURN:
			GetOpData(op_id, &op_rec);
			switch(GetOpType(op_rec.LinkOpID)) {
				case PPOPT_GOODSRECEIPT: dlg_id = DLG_LOTITEM; break;
				case PPOPT_GOODSEXPEND:
					if(r_cfg.Flags & CFGFLG_SELGOODSBYPRICE && !goods_fixed)
						dlg_id = ((goods_by_price = 1), DLG_PSELLITEM);
					else
						dlg_id = DLG_SELLITEM;
					break;
			}
			break;
		case PPOPT_DRAFTEXPEND: dlg_id = DLG_SELLITEMDRAFT; break;
		case PPOPT_GOODSORDER:  dlg_id = DLG_ORDLOTITEM;    break;
		case PPOPT_GOODSACK:    dlg_id = DLG_ACKITEM;       break;
		case PPOPT_DRAFTRECEIPT:
		case PPOPT_DRAFTTRANSIT: dlg_id = /*allow_suppl_sel ? DLG_SLOTITEM :*/ DLG_LOTITEM; break;
		case PPOPT_GOODSREVAL:   dlg_id = (GetOpSubType(op_id) == OPSUBT_ASSETEXPL) ? DLG_ASSETEXPLITEM : DLG_REVALITEM; break;
		case PPOPT_GOODSEXPEND:
			if(IsIntrOp(op_id))
				dlg_id = DLG_INTRITEM;
			else if(r_cfg.Flags & CFGFLG_SELGOODSBYPRICE && !goods_fixed)
				dlg_id = ((goods_by_price = 1), DLG_PSELLITEM);
			else
				dlg_id = DLG_SELLITEM;
			break;
		case PPOPT_GOODSRECEIPT:
			if(GetOpSubType(op_id) == OPSUBT_ASSETRCV)
				dlg_id = DLG_ASSETLOTITEM;
			else
				dlg_id = /*allow_suppl_sel ? DLG_SLOTITEM :*/ DLG_LOTITEM;
			break;
		case PPOPT_CORRECTION: dlg_id = DLG_REVALITEM; break;
		case PPOPT_GOODSMODIF:
			if(itemNo >= 0)
				sign = pPack->ConstTI(itemNo).GetSign(pPack->Rec.OpID);
			if(sign == TISIGN_PLUS) {
				if(pPack->Rec.Flags & BILLF_RECOMPLETE)
					dlg_id = DLG_SELLITEM;
				else if(GetOpSubType(op_id) == OPSUBT_ASSETMODIF)
					dlg_id = DLG_ASSETMODIFLOTITEM;
				else
					dlg_id = DLG_LOTITEM;
			}
			else if(sign == TISIGN_MINUS)
				dlg_id = DLG_SELLITEM;
			else if(sign == TISIGN_RECOMPLETE)
				dlg_id = DLG_REVALITEM;
			else {
				CALLEXCEPT_PP(PPERR_GMODIFITEMSIGN);
			}
			break;
	}
	THROW_PP(dlg_id, PPERR_INVOPRKIND);
	THROW(CheckDialogPtr(&(dlg = new TrfrItemDialog(dlg_id, op_id))));
	SETFLAG(dlg->St, TrfrItemDialog::stAllowSupplSel, allow_suppl_sel);
	if(pInitData) {
		dlg->MinQtty = pInitData->QttyBounds.low;
		dlg->MaxQtty = pInitData->QttyBounds.upp;
	}
	if(itemNo < 0) {
		THROW_MEM(p_item = new PPTransferItem(&pPack->Rec, sign));
		dlg->EditMode = 0;
		if(pInitData) {
			if(pInitData->GoodsID) {
				THROW(p_item->SetupGoods(labs(pInitData->GoodsID), TISG_SETPWOTF));
				if(pInitData->LotID)
					p_item->SetupLot(pInitData->LotID, 0, 0);
				SETFLAG(dlg->St, TrfrItemDialog::stGoodsFixed, 1);
			}
			else if(pInitData->GoodsGrpID)
				dlg->InitGoodsGrpID = pInitData->GoodsGrpID;
		}
		if(!dlg->InitGoodsGrpID && !p_item->GoodsID) {
			PPAccessRestriction accsr;
			dlg->InitGoodsGrpID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
		}
		if(pOrder) {
			THROW(p_item->SetupGoods(labs(pOrder->GoodsID), TISG_SETPWOTF));
			p_item->LotID    = 0;
			p_item->OrdLotID = pOrder->LotID; // @ordlotid
			p_item->Flags   |= PPTFR_ONORDER;
		}
		if(pPack->OpTypeID == PPOPT_GOODSRECEIPT || (pPack->OpTypeID == PPOPT_GOODSMODIF && sign > 0))
			if(CConfig.Flags & CCFLG_COSTWOVATBYDEF)
				p_item->Flags |= PPTFR_COSTWOVAT;
	}
	else {
		p_item = &pPack->TI(itemNo);
		if(p_item->Flags & PPTFR_ONORDER)
			if(pPack->SearchShLot(p_item->OrdLotID, &(i = 0))) // @ordlotid
				pOrder = &pPack->P_ShLots->at(i);
			else {
				pOrder = 0;
				p_item->Flags &= ~PPTFR_ONORDER;
			}
		dlg->EditMode = 1;
	}
	SETFLAG(dlg->St, TrfrItemDialog::stGoodsByPrice, goods_by_price);
	dlg->ItemNo       = itemNo;
	dlg->P_Pack       = pPack;
	pattern           = *p_item;
	p_item->GoodsID   = labs(p_item->GoodsID);
	dlg->P_OrderItem  = pOrder;
	THROW(dlg->setDTS(p_item));
	if(itemNo < 0 && pInitData && pInitData->Quantity >= 0.0 && dlg->Item.Quantity_ == 0.0) {
		uint ctl_set = (r_cfg.Flags & CFGFLG_USEPACKAGE && (pInitData->Quantity == 0.0 || pInitData->Flags & TIDIF_PACKS)) ? CTL_LOT_PACKS : CTL_LOT_QUANTITY;
		uint ctl_select = (!(r_cfg.Flags & CFGFLG_USEPACKAGE) || (pInitData->Quantity && !(pInitData->Flags & TIDIF_PACKS))) ? CTL_LOT_QUANTITY : CTL_LOT_PACKS;
		dlg->setCtrlData(ctl_set, &pInitData->Quantity);
		if(!(dlg->St & TrfrItemDialog::stGoodsByPrice) || pInitData->GoodsID)
			dlg->selectCtrl(ctl_select);
	}
	else {
		//
		// Устанавливаем текущим управляющим первый элемент, который не заблокирован и имеет статус остановки по TAB (WS_TABSTOP)
		//
		for(TView * p_cur = dlg->P_Current; p_cur;) {
			long   wnd_style = TView::GetWindowStyle(p_cur->getHandle());
			if(wnd_style & WS_TABSTOP && !(wnd_style & WS_DISABLED))
				p_cur = 0;
			else {
				dlg->selectNext();
				p_cur = dlg->P_Current;
			}
		}
	}
	if(pPack->GetSyncStatus() > 0) {
		//
		// Если у пользователя нет прав на изменение синхронизированного документа,
		// то менять скидку на весь документ он не может - это приведет к изменению сумм по строкам.
		//
		if(oneof6(pPack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF,
			PPOPT_GOODSRETURN, PPOPT_GOODSORDER)) {
			if(!BillObj->CheckRights(BILLOPRT_MODTRANSM, 1))
				rt_to_modif = 0;
		}
	}
	if(!rt_to_modif)
		dlg->enableCommand(cmOK, 0);
	while(!valid_data && (skip_dlg || (r = ExecView(dlg)) == cmOK)) {
		double extra_qtty = 0.0;
		valid_data = dlg->getDTS(p_item, &extra_qtty);
		if(valid_data && r_cfg.Flags & CFGFLG_UNIQUELOT)
			for(i = 0; valid_data && pPack->EnumTItems(&i, &p_ti);)
				if((i-1) != (uint)itemNo && p_ti->LotID && p_ti->LotID == p_item->LotID)
					valid_data = (PPError(PPERR_DUPLOTSINPACKET, 0), 0);
		if(CConfig.Flags & CCFLG_CHECKSPOILAGE && valid_data && oneof2(pPack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND)) {
			SString serial;
			SETIFZ(p_spc_core, new SpecSeriesCore);
			if(p_spc_core) {
				SpecSeries2Tbl::Rec spc_rec;
				// @v9.8.11 if(pPack->SnL.GetNumber(itemNo, &serial) > 0) {
				if(pPack->LTagL.GetNumber(PPTAG_LOT_SN, itemNo, serial) > 0) { // @v9.8.11 
					serial.Transf(CTRANSF_INNER_TO_OUTER);
					if(p_spc_core->SearchBySerial(SPCSERIK_SPOILAGE, serial, &spc_rec) > 0) {
						if(ViewSpoilList(p_spc_core, serial, 1) < 0)
							valid_data = 0;
						/*
						if(PPMessage(mfConf|mfYesNo, PPCFM_ISGOODSSPOILAGE, spc_rec.GoodsName) == cmNo)
							valid_data = 0;
						*/
					}
				}
			}
		}
		if(valid_data) {
			if(pattern.GoodsID < 0)
				p_item->GoodsID = -labs(p_item->GoodsID);
			if(itemNo >= 0) {
				if(p_item->LotID == 0)
					pattern.LotID = 0;
				if(!modified && memcmp(p_item, &pattern, sizeof(pattern)) == 0)
					r = cmCancel;
			}
			THROW(pPack->SetupRow(itemNo, p_item, pOrder, extra_qtty));
			if(p_item->Flags & PPTFR_REVAL && !p_item->IsRecomplete())
				THROW(dlg->ProcessRevalOnAllLots(p_item));
			r = cmOK;
		}
		else
			skip_dlg = 0;
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	if(itemNo == -1)
		delete p_item;
	delete dlg;
	delete p_spc_core;
	return r;
}

#define GRP_QCERT  1
#define GRP_LOC    2

TrfrItemDialog::TrfrItemDialog(uint dlgID, PPID opID) : TDialog(dlgID), OpID(opID), OpTypeID(GetOpType(opID)),
	InitGoodsGrpID(0), GoodsGrpID(0), P_BObj(BillObj), Rest(0.0), OrdRest(0.0), OrdReserved(0.0), MinQtty(0.0), MaxQtty(0.0), 
	NumPacks(0.0), OrgQtty(0.0), OrgPrice(0.0), EditMode(0), ItemNo(0), P_Pack(0), P_OrderItem(0), St(0)
{
	P_Trfr = P_BObj->trfr;
	Ptb.SetBrush(brushChangedCost, SPaintObj::bsSolid, LightenColor(GetColorRef(SClrBlue), 0.3f), 0);
	Ptb.SetBrush(brushQuotedPrice, SPaintObj::bsSolid, GetColorRef(SClrYellow), 0);
	Ptb.SetBrush(brushQuotedPriceNoCancel, SPaintObj::bsSolid, GetColorRef(SClrLime), 0);
	Ptb.SetBrush(brushPriceBelowCost, SPaintObj::bsSolid, GetColorRef(SClrAqua), 0);
	Ptb.SetBrush(brushVetisUuidExists, SPaintObj::bsSolid, GetColorRef(SClrGreen), 0); // @v10.1.8
	Ptb.SetBrush(brushVetisUuidAbsence, SPaintObj::bsSolid, GetColorRef(SClrRed), 0); // @v10.1.8

	MEMSZERO(Sd);
	PPLoadText(PPTXT_TIDLG_STRINGS, Strings);
	setCtrlOption(CTL_LOT_STREST, ofFramed, 1);
	addGroup(GRP_QCERT, new QCertCtrlGroup(CTL_LOT_QCERT));
	SetupCalDate(CTLCAL_LOT_EXPIRY, CTL_LOT_EXPIRY);
	PPSetupCtrlMenu(this, CTL_LOT_GOODSGRP, CTLMNU_LOT_GOODSGRP, CTRLMENU_TI_GOODS);
	PPSetupCtrlMenu(this, CTL_LOT_GOODS,    CTLMNU_LOT_GOODS,    CTRLMENU_TI_GOODS);
	PPSetupCtrlMenu(this, CTL_LOT_PACKS,    CTLMNU_LOT_PACKS,    CTRLMENU_TI_PACKS);
	PPSetupCtrlMenu(this, CTL_LOT_QUANTITY, CTLMNU_LOT_QUANTITY, CTRLMENU_TI_QUANTITY);
	PPSetupCtrlMenu(this, CTL_LOT_PRICE,    CTLMNU_LOT_PRICE,    CTRLMENU_TI_PRICE);
	PPSetupCtrlMenu(this, CTL_LOT_CURPRICE, CTLMNU_LOT_CURPRICE, CTRLMENU_TI_CURPRICE);
	PPSetupCtrlMenu(this, CTL_LOT_SERIAL,   CTLMNU_LOT_SERIAL,   CTRLMENU_TI_SERIAL);
	if(!P_BObj->CheckRights(BILLRT_ACCSCOST)) {
		SetCtrlState(CTL_LOT_COST, sfVisible, false);
		SetCtrlState(CTLMNU_LOT_COST, sfVisible, false);
	}
	else
		PPSetupCtrlMenu(this, CTL_LOT_COST, CTLMNU_LOT_COST, CTRLMENU_TI_COST);
	// @v9.4.3 {
	{
		SString temp_buf;
		if(GetOpName(opID, temp_buf) > 0)
			setTitle(temp_buf);
	}
	// } @v9.4.3
}

int TrfrItemDialog::isModifPlus() const
{
	return BIN(OpTypeID == PPOPT_GOODSMODIF && Item.Flags & PPTFR_PLUS);
}

void TrfrItemDialog::setupCtrls() // Called from TrfrItemDialog::setDTS
{
	const PPConfig & r_cfg = LConfig;
	int    disable_goods = 0;
	disableCtrls(1, CTL_LOT_OLDCOST, CTL_LOT_OLDPRICE, CTL_LOT_ASSETEXPL, 0);
	if(!(r_cfg.Flags & CFGFLG_USEPACKAGE))
		disableCtrls(1, CTL_LOT_UNITPERPACK, CTL_LOT_PACKS, 0);
	else if(!(Item.Flags & PPTFR_RECEIPT) && OpTypeID != PPOPT_DRAFTRECEIPT)
		disableCtrl(CTL_LOT_UNITPERPACK, 1);
	//
	// Переоценка допускает ввод и цены и скидки (у скидки другое значение)
	//
	if(!(Item.Flags & PPTFR_REVAL)) {
		//
		// Полагаемся на то, что флаг BILLF_TOTALDISCOUNT может быть установлен
		// только для операций продажи и возврата товара от покупателя //
		// Кроме того, продажа по котировке тоже запрешает модификацию
		// цены реализации и скидки
		//
		if(P_Pack->Rec.Flags & BILLF_TOTALDISCOUNT || Item.Flags & PPTFR_QUOT)
			disableCtrls(1, CTL_LOT_DISCOUNT, CTL_LOT_PRICE, 0);
		else if(!(Item.Flags & PPTFR_ORDER)) { // @v9.2.10 Для заказа разрешается менять и цену реализации и скидку
			//
			// Флаг конфигурации CFGFLG_DISCOUNTBYSUM запрещает ввод скидки
			// @v9.2.9 Для заказа разрешается вводить скидку
			//
			if(r_cfg.Flags & CFGFLG_DISCOUNTBYSUM)
				disableCtrl(CTL_LOT_DISCOUNT, 1);
			//
			// Поступление товара (с генерацией лота) однозначно допускает
			// ввод цены реализации. С другой стороны внутреннее перемещение товара
			// косвенно генерирует лот, но его цена может отличаться от цены
			// родительского лота только если установлен флаг CFGFLG_FREEPRICE.
			//
			else if(!(Item.Flags & PPTFR_RECEIPT) && !(IsIntrExpndOp(OpID) && r_cfg.Flags & CFGFLG_FREEPRICE))
				disableCtrl(CTL_LOT_PRICE, 1);
		}
	}
	if((Item.Flags & PPTFR_ONORDER) || (EditMode && (!(Item.Flags & (PPTFR_RECEIPT|PPTFR_DRAFT)) || !P_BObj->CheckRights(BILLRT_MODGOODS))))
		disable_goods = 1;
	if(OpTypeID == PPOPT_GOODSRETURN) {
		disable_goods = 1;
		disableCtrls(1, CTL_LOT_UNITPERPACK, CTL_LOT_COST, /*@v10.1.12 CTL_LOT_PRICE, CTL_LOT_DISCOUNT,*/ CTL_LOT_LOT, 0);
		// @v10.1.12 {
		if(Item.Flags & PPTFR_PLUS)
			disableCtrls(1, CTL_LOT_PRICE, CTL_LOT_DISCOUNT, 0);
		// } @v10.1.12 
	}
	else if(IsIntrExpndOp(OpID))
		disableCtrl(CTL_LOT_COST, 1);
	if(disable_goods || St & stGoodsFixed) {
		St |= stGoodsFixed;
		disableCtrls(1, CTLSEL_LOT_GOODSGRP, CTLSEL_LOT_GOODS, 0);
	}
	if(!(oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT) || isModifPlus()) || !Item.GoodsID) {
		disableCtrls(1, CTL_LOT_EXPIRY, CTLCAL_LOT_EXPIRY, CTLSEL_LOT_QCERT, 0);
		setCtrlReadOnly(CTL_LOT_CLB, 1);
		setCtrlReadOnly(CTL_LOT_SERIAL, 1);
	}
	if(St & stGoodsByPrice) {
		disableCtrl(CTL_LOT_PRICE, 0);
		selectCtrl(CTL_LOT_PRICE);
	}
	if(Item.CurID)
		if(Item.Flags & PPTFR_SELLING)
			disableCtrls(1, CTL_LOT_PRICE, CTL_LOT_DISCOUNT, 0);
		else
			disableCtrl(CTL_LOT_COST, 1);
	else
		disableCtrl(CTL_LOT_CURPRICE, 1);
	//
	// В переоценке количество изменять не допускается //
	//
	if(Item.Flags & PPTFR_REVAL) {
		if(!(Item.Flags & PPTFR_CORRECTION))
			disableCtrl(CTL_LOT_QUANTITY, 1);
		if(!Item.IsRecomplete()) {
			disableCtrl(CTL_LOT_INDEPPHQTTY, 1);
			if(CheckOpFlags(OpID, OPKF_DENYREVALCOST, 0))
				disableCtrl(CTL_LOT_COST, 1);
		}
	}
	setupCtrlsOnGoodsSelection();
}
//
// Helper function. Called only from TrfrItemDialog::handleEvent.
//
int TrfrItemDialog::setupAllQuantity(int byLot)
{
	int    ok = -1;
	const  uint i = GetCurrId();
	if(oneof2(i, CTL_LOT_QUANTITY, CTL_LOT_PACKS) && !getCtrlView(CTL_LOT_QUANTITY)->IsInState(sfDisabled)) {
		if(Item.Flags & PPTFR_MINUS) {
			const double preserve_qtty = Item.Quantity_;
			double qtty = 0.0;
			Item.Quantity_ = 0.0;
			if(!byLot) {
				P_Pack->GoodsRest(Item.GoodsID, &Item, ItemNo, &qtty);
				setCtrlReal(CTL_LOT_QUANTITY, qtty);
				ok = 1;
			}
			else if(P_Pack->BoundsByLot(Item.LotID, &Item, ItemNo, &qtty, 0)) {
				setCtrlReal(CTL_LOT_QUANTITY, qtty);
				ok = 1;
			}
			else {
				Item.Quantity_ = preserve_qtty;
				ok = PPErrorZ();
			}
		}
		else if(Item.Flags & PPTFR_PLUS) {
			if(IsSourceSerialUsed() && Item.GoodsID) {
				PPIDArray src_goods_id_list;
				SString source_serial;
				getCtrlString(CTL_LOT_SOURCESERIAL, source_serial);
				if(source_serial.NotEmptyS()) {
					if(GetGoodsListSuitableForSourceSerial(Item.GoodsID, src_goods_id_list) > 0) {
						src_goods_id_list.sortAndUndup();
						PPIDArray lot_list;
						P_BObj->SearchLotsBySerialExactly(source_serial, &lot_list);
						for(uint i = 0; i < lot_list.getCount(); i++) {
							const PPID lot_id = lot_list.get(i);
							ReceiptTbl::Rec lot_rec;
							if(P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0 && src_goods_id_list.bsearch(lot_rec.GoodsID)) {
								if(lot_rec.Rest > 0.0) {
									setCtrlReal(CTL_LOT_QUANTITY, lot_rec.Rest);
									ok = 1;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int TrfrItemDialog::evaluateBasePrice(double curPrice, double * pBasePrice)
{
	int    ok = -1;
	double base_price = 0.0;
	if(Item.CurID) {
		base_price = TR5(curPrice * P_Pack->Amounts.Get(PPAMT_CRATE, Item.CurID));
		ok = 1;
	}
	ASSIGN_PTR(pBasePrice, base_price);
	return ok;
}

int TrfrItemDialog::getCtrlCost()
{
	double c = Item.Cost;
	if(getCtrlData(CTL_LOT_COST, &c)) {
		Item.SetOrgCost(c);
		return 1;
	}
	else
		return 0;
}

int TrfrItemDialog::setCtrlCost()
{
	return setCtrlReal(CTL_LOT_COST, Item.GetOrgCost());
}

void TrfrItemDialog::setupCurPrice()
{
	double cur_price = 0.0;
	double base_price;
	if(Item.CurID && getCtrlData(CTL_LOT_CURPRICE, &cur_price)) {
		uint   ctl_id = 0;
		evaluateBasePrice(cur_price, &base_price);
		if(Item.Flags & PPTFR_REVAL) {
			ctl_id = (Item.Flags & PPTFR_SELLING) ? CTL_LOT_PRICE : CTL_LOT_COST;
		}
		else if(Item.Flags & PPTFR_SELLING)
			if(LConfig.Flags & CFGFLG_DISCOUNTBYSUM)
				ctl_id = CTL_LOT_PRICE;
			else
				setCtrlReal(CTL_LOT_DISCOUNT, TR5(Item.Price - base_price));
		else
			ctl_id = CTL_LOT_COST;
		if(ctl_id) {
			setCtrlData(ctl_id, &base_price);
			setupVaPct();
		}
	}
}

void TrfrItemDialog::recalcUnitsToPhUnits()
{
	double phuperu;
	if(Item.GoodsID && GObj.GetPhUPerU(Item.GoodsID, 0, &phuperu) > 0) {
		setCtrlReal(CTL_LOT_QUANTITY, R6(getCtrlReal(CTL_LOT_QUANTITY) / phuperu));
		setupQuantity(CTL_LOT_QUANTITY, 1);
	}
}

void TrfrItemDialog::editQCertData()
{
	if(Item.Flags & PPTFR_DRAFT) {
		LotQCertDialog * dlg = 0;
		int    valid_data = 0;
		LotQCertData lqcd;
		SString org_clb, serial;
		if(CheckDialogPtr(&(dlg = new LotQCertDialog(P_BObj)))) {
			getCtrlData(CTL_LOT_EXPIRY, &Item.Expiry);
			if(checkdate(Item.Expiry, 1)) {
				lqcd.LotID   = Item.LotID;
				lqcd.QCertID = Item.QCert;
				lqcd.Expiry  = Item.Expiry;
				lqcd.IsInheritedClb = 1;
				P_BObj->GetClbNumberByLot(Item.LotID, 0, org_clb);
				// @v9.8.11 P_Pack->SnL.GetNumber(ItemNo, &serial);
				P_Pack->LTagL.GetNumber(PPTAG_LOT_SN, ItemNo, serial); // @v9.8.11 
				org_clb.Strip().CopyTo(lqcd.CLB, sizeof(lqcd.CLB));
				serial.Strip().CopyTo(lqcd.Serial, sizeof(lqcd.Serial));
				dlg->setDTS(&lqcd);
				while(!valid_data && ExecView(dlg) == cmOK)
					if(dlg->getDTS(&lqcd)) {
						valid_data = 1;
						Item.QCert = lqcd.QCertID;
						Item.Expiry = lqcd.Expiry;
						// @v9.8.11 P_Pack->SnL.AddNumber(ItemNo, lqcd.Serial);
						P_Pack->LTagL.AddNumber(PPTAG_LOT_SN, ItemNo, lqcd.Serial); // @v9.8.11 
						setCtrlData(CTL_LOT_EXPIRY, &Item.Expiry);
					}
			}
			else
				PPError(PPERR_SLIB);
		}
		else
			PPError();
		delete dlg;
	}
	else
		ViewQCertDialog(Item.QCert);
}

void TrfrItemDialog::GenerateSerial()
{
	if(IsTaggedItem()) {
		SString templt = (GObj.IsAsset(Item.GoodsID) > 0) ? P_BObj->Cfg.InvSnTemplt : P_BObj->Cfg.SnTemplt;
		SString serial;
		if(P_BObj->GetSnByTemplate(P_Pack->Rec.Code, labs(Item.GoodsID), &P_Pack->LTagL/*SnL*/, templt, serial) > 0)
			setCtrlString(CTL_LOT_SERIAL, serial);
	}
}

void TrfrItemDialog::calcPrice()
{
	CalcPriceParam param;
	param.VaPercent = DS.GetTLA().Lid.VaPercent;
	if(getCtrlCost() && getCtrlData(CTL_LOT_PRICE, &param.Price) && !getCtrlView(CTL_LOT_PRICE)->IsInState(sfDisabled)) {
		param.Cost       = Item.Cost;
		param.Dt         = P_Pack->Rec.Dt;
		param.GoodsID    = Item.GoodsID;
		param.InTaxGrpID = getCtrlLong(CTLSEL_LOT_INTAXGRP);
		ushort v = getCtrlUInt16(CTL_LOT_NOVAT);
		SETFLAG(param.Flags, CalcPriceParam::fCostWoVat, v);
		param.Restore();
		if(CalcPrice(&param) > 0) {
			param.Save();
			setCtrlReal(CTL_LOT_PRICE, param.Price);
			selectCtrl(CTL_LOT_PRICE);
			setupVaPct();
		}
	}
}

int TrfrItemDialog::setupValuation()
{
	int    ok = -1;
	const PPBillConfig & r_cfg = P_BObj->GetConfig();
	if(oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT) && !Item.RByBill &&
		LConfig.Flags & CFGFLG_AUTOQUOT && r_cfg.ValuationQuotKindID && CheckOpFlags(OpID, OPKF_NEEDVALUATION)) {
		ENTER_CRITICAL_SECTION
		static int locking = 0;
		if(!locking) {
			locking = 1;
			getCtrlCost();
			double cost  = Item.Cost;
			double price = getCtrlReal(CTL_LOT_PRICE);
			Item.Price = TR5(price);
			ushort v = 0;
			if(getCtrlData(CTL_LOT_NOVAT, &v))
				SETFLAG(Item.Flags, PPTFR_COSTWOVAT, v);
			if(Item.Valuation(r_cfg, 0, 0) > 0) {
				setCtrlReal(CTL_LOT_PRICE, Item.Price);
				disableCtrl(CTL_LOT_PRICE, 1);
				ok = 1;
			}
			locking = 0;
		}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

void TrfrItemDialog::SetupSerialWarn()
{
	if(getCtrlView(CTL_LOT_ST_SERIALWARN)) {
		SString temp_buf;
		getCtrlString(CTL_LOT_SERIAL, temp_buf);
		if(P_BObj->AdjustSerialForUniq(Item.GoodsID, Item.LotID, 1, temp_buf) > 0)
			PPLoadText(PPTXT_NONUNIQLOTSERIAL, temp_buf);
		else
			temp_buf.Z();
		setStaticText(CTL_LOT_ST_SERIALWARN, temp_buf);
	}
}

void TrfrItemDialog::SetupQttyByLotDimTag()
{
}

int TrfrItemDialog::GetGoodsListSuitableForSourceSerial(PPID goodsID, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Search(goodsID, &goods_rec) > 0 && goods_rec.StrucID) {
		PPGoodsStruc gs;
		PPGoodsStruc::Ident gsident(0, GSF_COMPL, 0);
		TSCollection <PPGoodsStruc> gs_list;
		if(GObj.GSObj.Get(goods_rec.StrucID, &gs) > 0 && gs.Select(&gsident, gs_list) > 0) {
			for(uint i = 0; i < gs_list.getCount(); i++) {
				const PPGoodsStruc * p_gs = gs_list.at(i);
				if(p_gs) {
					for(uint j = 0; j < p_gs->Items.getCount(); j++) {
						const PPGoodsStrucItem & r_item = p_gs->Items.at(j);
						if(r_item.Flags & (GSIF_MAINITEM|GSIF_QUERYEXPLOT)) {
							rList.addnz(labs(r_item.GoodsID));
							ok = 1;
						}
					}
				}
			}
		}
	}
	return ok;
}

IMPL_HANDLE_EVENT(TrfrItemDialog)
{
	uint   i;
	if(event.isCmd(cmExecute)) {
		if(InitGoodsGrpID) {
			setCtrlLong(CTLSEL_LOT_GOODSGRP, InitGoodsGrpID);
			replyGoodsGroupSelection();
		}
		else if(GoodsGrpID == 0)
			messageToCtrl(CTLSEL_LOT_GOODSGRP, cmCBActivate, 0);
		else if(Item.Flags & PPTFR_ONORDER && Item.LotID == 0)
			replyGoodsSelection();
		// Далее управление передается базовому классу
	}
	else if(event.isCmd(cmOK)) {
		setupValuation();
	}
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(event.isCbSelected(CTLSEL_LOT_GOODSGRP))
			replyGoodsGroupSelection();
		else if(event.isCbSelected(CTLSEL_LOT_GOODS))
			replyGoodsSelection();
		else if(event.isCbSelected(CTLSEL_LOT_INTAXGRP) || event.isClusterClk(CTL_LOT_NOVAT)) {
			setupValuation();
			setupVatSum();
			setupVaPct();
		}
		else {
			switch(TVCMD) {
				case cmQCert:  editQCertData(); break;
				case cmLot:    selectLot();     break;
				case cmOrdLot:
					if(Item.GoodsID) {
						LotFilt filt;
						filt.GoodsID = Item.GoodsID;
						filt.ClosedTag = 1;
						filt.Flags |= LotFilt::fOrders;
						ViewLots(&filt, 1, 0);
					}
					break;
				case cmQuot:
					{
						PPID   loc_id = GetQuotLocID();
						GObj.EditQuotations(Item.GoodsID, loc_id, Item.CurID, 0 /* ArID */, PPQuot::clsGeneral);
						setupBaseQuot();
						setupPriceLimit();
					}
					break;
				case cmVetisMatch: // @v10.1.8
					if(Item.Flags & PPTFR_RECEIPT && CConfig.Flags2 & CCFLG2_USEVETIS && checkdate(P_Pack->Rec.Dt)) {
						const PPID suppl_person_id = ObjectToPerson(P_Pack->Rec.Object, 0);
						if(suppl_person_id) {
							SString temp_buf;
							VetisDocumentFilt vetis_filt;

							int    delay_days = 0;
							PPAlbatrosConfig acfg;
							if(DS.FetchAlbatrosConfig(&acfg) > 0 && acfg.Hdr.VetisCertDelay > 0)
								delay_days = acfg.Hdr.VetisCertDelay;
							else 
								delay_days = 3;
							vetis_filt.VDStatusFlags = (1<<vetisdocstCONFIRMED) | (1<<vetisdocstUTILIZED);
							//vetis_filt.Period.Set(plusdate(P_Pack->Rec.Dt, -3), P_Pack->Rec.Dt);
							vetis_filt.WayBillPeriod.Set(plusdate(P_Pack->Rec.Dt, -delay_days), P_Pack->Rec.Dt);
							vetis_filt.FromPersonID = suppl_person_id;
							vetis_filt.Flags |= (VetisDocumentFilt::fAsSelector);
							if(P_Pack->LTagL.GetNumber(PPTAG_LOT_VETIS_UUID, ItemNo, temp_buf) > 0) {
								vetis_filt.SelLotUuid.FromStr(temp_buf);
							}
							PPViewVetisDocument vetis_view;
							if(vetis_view.Init_(&vetis_filt)) {
								//temp_buf.Z().Cat(P_Pack->R)
								PPObjBill::MakeCodeString(&P_Pack->Rec, 0, temp_buf);
								SString item_info_buf;
								GetGoodsName(Item.GoodsID, item_info_buf);
								double phupu = 0.0;
								if(GObj.GetPhUPerU(Item.GoodsID, 0, &phupu) > 0 && phupu > 0.0) {
									item_info_buf.Space().Cat(fabs(Item.Quantity_) * phupu, MKSFMTD(0, 3, 0));
								}
								temp_buf.CatDiv('-', 1).Cat(item_info_buf);
								vetis_view.SetOuterTitle(temp_buf);
								//
								if(vetis_view.Browse(0) > 0) {
									VetisDocumentFilt * p_result_filt = (VetisDocumentFilt *)vetis_view.GetBaseFilt();
									PPID vetis_doc_id = p_result_filt->Sel;
									if(!!p_result_filt->SelLotUuid && vetis_doc_id) {
										p_result_filt->SelLotUuid.ToStr(S_GUID::fmtIDL, temp_buf);
										P_Pack->LTagL.AddNumber(PPTAG_LOT_VETIS_UUID, ItemNo, temp_buf);
										if(P_Pack->Rec.ID && Item.RByBill > 0 && Item.LotID) {
											if(!vetis_view.EC.MatchDocument(vetis_doc_id, P_Pack->Rec.ID, Item.RByBill, 1/*fromBill*/, 1)) 
												PPError();
										}
									}
								}
							}
						}
					}
					break;
				case cmTags:
					{
						getManuf();
						ObjTagList tag_list;
						ObjTagList * p_list = P_Pack->LTagL.Get(ItemNo);
						RVALUEPTR(tag_list, p_list);
						if(InheritedLotTagList.GetCount())
							tag_list.Merge(InheritedLotTagList, ObjTagList::mumAdd);
						tag_list.ObjType = PPOBJ_LOT;
						if(EditObjTagValList(&tag_list, 0) > 0) {
							P_Pack->LTagL.Set(ItemNo, &tag_list);
							setupPriceLimit();
							setupManuf();
						}
					}
					break;
				case cmLotDim:
					{
						PPGdsClsPacket gc_pack;
						if(Item.GoodsID && GObj.FetchCls(Item.GoodsID, 0, &gc_pack) > 0 && gc_pack.Rec.LotDimCount > 0) {
							int    skip = 0;
							ObjTagList tag_list;
							ObjTagList * p_list = P_Pack->LTagL.Get(ItemNo);
							RVALUEPTR(tag_list, p_list);
							if(InheritedLotTagList.GetCount())
								tag_list.Merge(InheritedLotTagList, ObjTagList::mumAdd);
							tag_list.ObjType = PPOBJ_LOT;
							const ObjTagItem * p_dim_tagitem = tag_list.GetItem(PPTAG_LOT_DIMENTIONS);
							ObjTagItem dim_tagitem;
							if(!RVALUEPTR(dim_tagitem, p_dim_tagitem)) {
								if(!dim_tagitem.Init(PPTAG_LOT_DIMENTIONS))
									skip = 1;
							}
							if(!skip && ReceiptCore::LotDimensions::EditTag(&gc_pack, &dim_tagitem) > 0) {
								tag_list.PutItem(PPTAG_LOT_DIMENTIONS, &dim_tagitem);
								P_Pack->LTagL.Set(ItemNo, &tag_list);
								if(gc_pack.LotDimQtty_Formula.NotEmpty()) {
									SString temp_buf;
									GoodsContext gctx(&Item, P_Pack);
									GoodsContext::Param gcp = gctx.GetParam();
									if(dim_tagitem.GetStr(temp_buf) > 0 && gcp.LotDim.FromString(temp_buf)) {
										gctx.SetParam(gcp);
										double qtty = 0.0;
										if(PPExprParser::CalcExpression(gc_pack.LotDimQtty_Formula, &qtty, 0, &gctx) && qtty > 0.0) {
											setCtrlReal(CTL_LOT_QUANTITY, R6(qtty));
											setupQuantity(CTL_LOT_QUANTITY, 1);
										}
									}
								}
							}
						}
					}
					break;
				case cmSourceSerial:
					if(IsSourceSerialUsed() && Item.GoodsID) {
						PPIDArray src_goods_id_list;
						if(GetGoodsListSuitableForSourceSerial(Item.GoodsID, src_goods_id_list) > 0) {
							SString current_serial;
							getCtrlString(CTL_LOT_SOURCESERIAL, current_serial); // @v9.4.5
							src_goods_id_list.sortAndUndup();
							PPObjBill::SelectLotParam slp(0, Item.LocID, Item.LotID, PPObjBill::SelectLotParam::fNotEmptySerial);
							slp.RetLotSerial = current_serial; // @v9.4.5
							slp.GoodsList = src_goods_id_list;
							if(P_BObj->SelectLot2(slp) > 0) {
								setCtrlString(CTL_LOT_SOURCESERIAL, slp.RetLotSerial);
							}
						}
					}
					break;
				case cmInputUpdated:
					if(event.isCtlEvent(CTL_LOT_PRICE) || event.isCtlEvent(CTL_LOT_DISCOUNT)) {
						//
						// При изменении поля цены реализации либо скидки необходимо перерисовать его
						// дабы его цвет изменился (см. ниже cmCtlColor)
						//
						drawCtrl(CTL_LOT_PRICE);
					}
					else if(event.isCtlEvent(CTL_LOT_SERIAL))
						SetupSerialWarn();
					break;
				case cmCtlColor:
					{
						// TCanvas
						TDrawCtrlData * p_dc = (TDrawCtrlData *)TVINFOPTR;
						if(p_dc) {
							if(St & stWasCostInput) {
								if(getCtrlHandle(CTL_LOT_COST) == p_dc->H_Ctl) {
									::SetBkMode(p_dc->H_DC, TRANSPARENT);
									::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
									p_dc->H_Br = (HBRUSH)Ptb.Get(brushChangedCost);
									clearEvent(event);
								}
							}
							if(getCtrlHandle(CTL_LOT_PRICE) == p_dc->H_Ctl) {
								if(Item.Flags & PPTFR_QUOT) {
									::SetBkMode(p_dc->H_DC, TRANSPARENT);
									if(P_BObj->CheckRights(BILLOPRT_CANCELQUOT)) {
										p_dc->H_Br = (HBRUSH)Ptb.Get(brushQuotedPrice);
									}
									else {
										p_dc->H_Br = (HBRUSH)Ptb.Get(brushQuotedPriceNoCancel);
									}
									clearEvent(event);
								}
								else if(oneof4(OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND, PPOPT_GOODSRECEIPT)) {
									getCtrlCost();
									double pc = TR5(Item.Price);
									double ds = TR5(Item.Discount);
									getCtrlData(CTL_LOT_PRICE,    &pc);
									getCtrlData(CTL_LOT_DISCOUNT, &ds);
									if((pc-ds) < Item.Cost) {
										::SetBkMode(p_dc->H_DC, TRANSPARENT);
										p_dc->H_Br = (HBRUSH)Ptb.Get(brushPriceBelowCost);
										clearEvent(event);
									}
								}
							}
							// @v10.1.8 {
							else if(getCtrlHandle(CTL_LOT_VETISIND) == p_dc->H_Ctl) {
								SString temp_buf;
								P_Pack->LTagL.GetNumber(PPTAG_LOT_VETIS_UUID, ItemNo, temp_buf);
								if(temp_buf.NotEmpty()) {
									p_dc->H_Br = (HBRUSH)Ptb.Get(brushVetisUuidExists);
									clearEvent(event);
								}
								else if(GObj.CheckFlag(Item.GoodsID, GF_WANTVETISCERT)) {
									p_dc->H_Br = (HBRUSH)Ptb.Get(brushVetisUuidAbsence);
									clearEvent(event);
								}
							}
							// } @v10.1.8 
						}
					}
					return;
				default:
					return;
			}
		}
		clearEvent(event);
	}
	else if(TVBROADCAST) {
		if(TVCMD == cmChangedFocus) {
			if(event.isCtlEvent(CTL_LOT_PRICE) && (St & stGoodsByPrice) && P_Owner)
				setupGoodsListByPrice();
		}
		else if(oneof2(TVCMD, cmReleasedFocus, cmCommitInput)) {
			i = TVINFOVIEW->GetId();
			if(oneof3(i, CTL_LOT_UNITPERPACK, CTL_LOT_PACKS, CTL_LOT_QUANTITY)) {
				setupQuantity(i, 1);
				setupVatSum();
			}
			else if(i == CTL_LOT_CURPRICE)
				setupCurPrice();
			else if(i == CTL_LOT_COST) {
				if(setupValuation() <= 0) {
					if(OpTypeID == PPOPT_GOODSRECEIPT && R5(getCtrlReal(CTL_LOT_PRICE)) == 0.0) {
						getCtrlCost();
						if(Item.Cost != 0.0)
							setupQuotation(0, 0);
					}
					else
						setQuotSign();
				}
				setupVatSum();
				setupVaPct();
				setupBaseQuot();
			}
			else if(i == CTL_LOT_PRICE)
				setupVaPct();
		}
	}
	else if(TVKEYDOWN) {
		i = GetCurrId();
		if(TVKEY == kbF2) {
			if(oneof2(i, CTL_LOT_COST, CTL_LOT_PRICE))
				calcPrice();
			else if(oneof2(i, CTL_LOT_GOODSGRP, CTL_LOT_GOODS) && !(St & stGoodsFixed))
				selectGoodsByBarCode();
			else if(i == CTL_LOT_SERIAL)
				GenerateSerial();
			else if(i && setupAllQuantity(1) < 0)
				return;
		}
		else if(TVKEY == kbShiftF2) {
			if(setupAllQuantity(0) < 0)
				return;
		}
		else if(TVKEY == kbF3)
			setupQuotation(0, 0);
		else if(TVKEY == kbShiftF3)
			setupQuotation(1, 0);
		else if(TVKEY == kbF4 || TVCHR == kbCtrlP) {
			double phuperu;
			if(i == CTL_LOT_QUANTITY)
				recalcUnitsToPhUnits();
			else if(i == CTL_LOT_PRICE) {
				if(Item.GoodsID && GObj.GetPhUPerU(Item.GoodsID, 0, &phuperu) > 0) {
					setCtrlReal(CTL_LOT_PRICE, TR5(getCtrlReal(CTL_LOT_PRICE) * phuperu));
					setupVaPct();
				}
			}
			else if(i == CTL_LOT_COST) {
				if(Item.GoodsID && GObj.GetPhUPerU(Item.GoodsID, 0, &phuperu) > 0) {
					getCtrlCost();
					Item.SetOrgCost(Item.GetOrgCost() * phuperu);
					setCtrlCost();
					setupVaPct();
				}
			}
			else if(i == CTL_LOT_CURPRICE) {
				double phuperu;
				if(Item.GoodsID && GObj.GetPhUPerU(Item.GoodsID, 0, &phuperu) > 0) {
					setCtrlReal(CTL_LOT_CURPRICE, TR5(getCtrlReal(CTL_LOT_CURPRICE) * phuperu));
					setupVaPct();
					setupCurPrice();
				}
			}
		}
		else if(TVKEY == kbF5) {
			if(isCurrCtlID(CTL_LOT_PRICE)) {
				if(Item.Flags & PPTFR_PRICEWOTAXES)
					CalcTaxPrice(Item.GoodsID, OpID, P_Pack->Rec.Dt, getCtrlReal(CTL_LOT_PRICE));
			}
			else if(isCurrCtlID(CTL_LOT_QUANTITY)) {
				GoodsStockExt gse;
				if(GObj.GetStockExt(Item.GoodsID, &gse, 1) > 0 && gse.Package > 0.0) {
					double num_pckg = getCtrlReal(CTL_LOT_QUANTITY);
					setCtrlReal(CTL_LOT_QUANTITY, num_pckg * gse.Package);
					setupQuantity(CTL_LOT_QUANTITY, 1);
					setupVatSum();
				}
			}
		}
		else if(TVKEY == kbF6) {
			if(oneof2(i, CTL_LOT_COST, CTL_LOT_PRICE)) {
				double _p   = getCtrlReal(i);
				double qtty = getCtrlReal(CTL_LOT_QUANTITY);
				if(qtty) {
					setCtrlReal(i, TR5(_p / qtty));
					setupVatSum();
				}
			}
		}
		else if(TVKEY == kbF9) {
			if(oneof3(i, CTL_LOT_COST, CTL_LOT_PRICE, CTL_LOT_QUANTITY)) {
				double _arg = getCtrlReal(i);
				double result = 0.0;
				if(PPGoodsCalculator(Item.GoodsID, 0, 1, _arg, &result) > 0 && result > 0) {
					setCtrlReal(i, (i == CTL_LOT_QUANTITY) ? R6(result) : TR5(result));
					if(i == CTL_LOT_QUANTITY)
						setupQuantity(CTL_LOT_QUANTITY, 1);
					else
						setupVatSum();
				}
			}
		}
		else
			return;
		clearEvent(event);
	}
}

int TrfrItemDialog::selectGoodsByBarCode()
{
	int    ok = -1;
	Goods2Tbl::Rec rec;
	if(GObj.SelectGoodsByBarcode(0, P_Pack->Rec.Object, &rec, 0, 0) > 0) {
		GoodsGrpID = rec.ParentID;
		Item.SetupGoods(rec.ID, TISG_SETPWOTF);
		setCtrlData(CTLSEL_LOT_GOODSGRP, &GoodsGrpID);
		setupGoodsList();
		replyGoodsSelection();
		ok = 1;
	}
	return ok;
}

int TrfrItemDialog::setupGoodsListByPrice()
{
	int    ok = 1;
	double tmp = getCtrlReal(CTL_LOT_PRICE);
	if(getCtrlView(CTLSEL_LOT_GOODS) && tmp && Item.Price != tmp) {
		Item.Price = tmp;
		ListWindow * p_lw  = 0;
		StrAssocArray * p_ary = 0;
		THROW(p_ary = GObj.CreateListByPrice(P_Pack->Rec.LocID, Item.Price));
		p_lw = CreateListWindow(p_ary, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		THROW_MEM(p_lw);
		((ComboBox*)getCtrlView(CTLSEL_LOT_GOODS))->setListWindow(p_lw);
		messageToCtrl(CTLSEL_LOT_GOODS, cmCBActivate, 0);
	}
	else
		Item.Price = tmp;
	CATCHZOKPPERR
	return ok;
}

int TrfrItemDialog::setupGoodsList()
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	if(St & stGoodsFixed && Item.GoodsID && GObj.Fetch(Item.GoodsID, &goods_rec) > 0)
		SetComboBoxLinkText(this, CTLSEL_LOT_GOODS, goods_rec.Name);
	else {
		PPID   grp_id = GoodsGrpID;
		if(P_Pack->OpTypeID == PPOPT_GOODSORDER && CheckOpFlags(P_Pack->Rec.OpID, OPKF_ORDEXSTONLY))
			grp_id = -GoodsGrpID;
		else if(!Item.GoodsID && !(Item.Flags & (PPTFR_ACK|PPTFR_DRAFT)) && (Item.Flags & (PPTFR_MINUS | PPTFR_REVAL)))
			grp_id = -GoodsGrpID;
		ok = SetupPPObjCombo(this, CTLSEL_LOT_GOODS, PPOBJ_GOODS, Item.GoodsID, OLW_LOADDEFONOPEN|OLW_CANINSERT, (void *)grp_id);
	}
	setupQttyFldPrec();
	return ok;
}

void TrfrItemDialog::replyGoodsGroupSelection()
{
	getCtrlData(CTLSEL_LOT_GOODSGRP, &GoodsGrpID);
	Item.SetupGoods(0, 0);
	Item.QCert   = 0;
	disableCtrl(CTLSEL_LOT_QCERT, 1);
	setupGoodsList();
	selectCtrl(CTL_LOT_GOODS);
	messageToCtrl(CTLSEL_LOT_GOODS, cmCBActivate, 0);
}

void TrfrItemDialog::calcOrderRest()
{
	if(Item.GoodsID && !CheckOpFlags(OpID, OPKF_NOCALCTIORD)) {
		int    i = 0;
		uint   pos = 0;
		const  PPTransferItem * oi = 0;
		if(P_OrderItem && ItemNo >= 0 && P_Pack->SearchShLot(P_OrderItem->LotID, &pos)) {
			oi = P_OrderItem;
			i = (int)pos;
		}
		else if(Item.Flags & PPTFR_ORDER) {
			oi = &Item;
			i = ItemNo;
		}
		else
			i = -1;
		P_Pack->GoodsRest(-labs(Item.GoodsID), oi, i, &OrdRest, &OrdReserved);
	}
	else
		OrdReserved = OrdRest = 0;
}

void TrfrItemDialog::setupQttyFldPrec()
{
	TInputLine * il = (TInputLine*)getCtrlView(CTL_LOT_QUANTITY);
	if(Item.GoodsID && il) {
		uint   prec = GObj.CheckFlag(Item.GoodsID, GF_INTVAL) ? 0 : 6;
		long   f    = il->getFormat();
		if(SFMTPRC(f) != prec) {
			SETSFMTPRC(f, prec);
			il->setFormat(f);
		}
	}
}

int TrfrItemDialog::isAllowZeroPrice()
{
	PPGoodsType gt_rec;
	Goods2Tbl::Rec goods_rec;
	return BIN(GObj.Fetch(Item.GoodsID, &goods_rec) > 0 &&
		GTObj.Fetch(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.Flags & GTF_ALLOWZEROPRICE);
}

int TrfrItemDialog::checkQuantityForIntVal()
{
	TView * v = getCtrlView(CTL_LOT_QUANTITY);
	if(v && !v->IsInState(sfDisabled) && GObj.CheckFlag(Item.GoodsID, GF_INTVAL) && ffrac(R6(Item.Quantity_)) != 0.0)
		return PPSetError(PPERR_INTVALUNIT);
	else
		return 1;
}

void TrfrItemDialog::setupCtrlsOnGoodsSelection()
{
	int    allow_dim_button = 0;
	SString tax_grp_name;
	Goods2Tbl::Rec goods_rec;
	setStaticText(CTL_LOT_ST_PHQTTY, 0);
	if(Item.GoodsID && GObj.Fetch(labs(Item.GoodsID), &goods_rec) > 0) {
		PPUnit unit_rec;
		if(GObj.FetchUnit(goods_rec.PhUnitID, &unit_rec) > 0)
			setStaticText(CTL_LOT_ST_PHQTTY, unit_rec.Name);
		GetObjectName(PPOBJ_GOODSTAX, goods_rec.TaxGrpID, tax_grp_name);
		if(goods_rec.GdsClsID) {
			PPGdsClsPacket gc_pack;
			if(GObj.FetchCls(goods_rec.ID, 0, &gc_pack) > 0 && gc_pack.Rec.LotDimCount > 0)
                allow_dim_button = 1;
		}
	}
	showCtrl(CTL_LOT_PHQTTY, !(Item.Flags & PPTFR_INDEPPHQTTY));
	showCtrl(CTL_LOT_INDEPPHQTTY, Item.Flags & PPTFR_INDEPPHQTTY);
	showButton(cmLotDim, allow_dim_button);
	setStaticText(CTL_LOT_ST_GOODSTAXGRP, tax_grp_name);
	setQuotSign();
}

int TrfrItemDialog::replyGoodsSelection(int recurse)
{
	int    ok = 1;
	int    r, dir, again = 0;
	int    all_lots_in_pckg = 0;
	double quot = 0.0;
	double suppl_deal_cost = 0.0;
	int    op_subtype = GetOpSubType(OpID);
	SString temp_buf;
	ReceiptTbl::Rec lot_rec;
	LotArray lot_list;
	uint   lot_idx = 0;

	InheritedLotTagList.Destroy();
	Rest = 0.0;
	Item.Flags &= ~PPTFR_AUTOCOMPL;
	getCtrlData(CTLSEL_LOT_GOODS, &Item.GoodsID);
	if(Item.GoodsID == 0) {
		again = 1;
		CALLEXCEPT_PP(PPERR_USERINPUT);
	}
	else if(Item.SetupGoods(Item.GoodsID, TISG_SETPWOTF) <= 0) {
		again = 1;
		CALLEXCEPT();
	}
	//
	// Защита от несоответствия лота товару
	//
	if(Item.LotID && (P_Trfr->Rcpt.Search(Item.LotID, &lot_rec) <= 0 || labs(lot_rec.GoodsID) != labs(Item.GoodsID)))
		Item.LotID = 0;
	//
	if(oneof2(op_subtype, OPSUBT_ASSETEXPL, OPSUBT_ASSETRCV)) {
		if(!GObj.IsAsset(Item.GoodsID)) {
			again = 1;
			CALLEXCEPT_PP(PPERR_ASSETGOODSNEEDED);
		}
	}
	if(oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
		const QuotIdent qi(Item.LocID, 0, Item.CurID, P_Pack->Rec.Object);
		GObj.GetSupplDeal(Item.GoodsID, qi, &Sd, 1);
		if(Sd.IsDisabled) {
			again = 1;
			CALLEXCEPT_PP(PPERR_GOODSRCPTDISABLED);
		}
		else
			suppl_deal_cost = Sd.Cost;
	}
	if(!P_Pack->CheckGoodsForRestrictions(ItemNo, Item.GoodsID, Item.GetSign(OpID), 0.0, (PPBillPacket::cgrfAll&~(PPBillPacket::cgrfQtty)), 0)) {
		again = 1;
		CALLEXCEPT();
	}
	if(Item.Flags & PPTFR_UNLIM && !(OpTypeID == PPOPT_GOODSRECEIPT)) {
		const QuotIdent qi(Item.LocID, PPQUOTK_BASE, Item.CurID, P_Pack->Rec.Object);
		THROW_PP(!(Item.Flags & PPTFR_REVAL), PPERR_REVALONUNLIM);
		THROW(r = GObj.GetQuot(Item.GoodsID, qi, 0.0, 0.0, &quot));
		if(r < 0) {
			if(PPMessage(mfConf|mfYes|mfNo, PPCFM_SETUPQUOT) == cmYes) {
				THROW(r = GObj.EditQuotations(Item.GoodsID, 0, Item.CurID, 0 /* ArID */, PPQuot::clsGeneral));
				if(r > 0) {
					THROW(GObj.GetQuot(Item.GoodsID, qi, 0L, 0L, &quot) > 0);
				}
				else
					again = 1;
			}
			else
				again = 1;
			if(again)
				CALLEXCEPT_PP(recurse ? 0 : PPERR_QUOTNEEDED);
		}
		if(Item.Flags & PPTFR_MINUS && GObj.CheckFlag(Item.GoodsID, GF_AUTOCOMPL))
			Item.Flags |= PPTFR_AUTOCOMPL;
		{
			double dis = P_Pack->Amounts.Get(PPAMT_PCTDIS, 0L /* @curID */);
			if(dis != 0.0)
				Item.Discount = fdiv100r(dis * quot);
		}
		Price = quot;
		dir = 2;        // Don't enumerate lots
		Item.LotID = 0; // quot.ID; //0;
	}
	//
	// В случае прихода товара от поставщика, заказа или подтверждения надо
	// просто найти (или не найти) последний лот (хоть открытый, хоть закрытый)
	// чтобы скопировать из него некоторые параметры.
	//
	else if(oneof3(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSACK, PPOPT_GOODSORDER) || P_Pack->IsDraft() || isModifPlus()) {
		const  PPID  lid = Item.LocID;
		const  PPID  lot_id = Item.LotID;
		const  LDATE sd = MAXDATE;
		Item.LotID = 0;
		dir = 2; // Don't enumerate lots
		Price = 0.0;
		if(ItemNo < 0 && isModifPlus() && P_BObj->GetConfig().Flags & BCF_AUTOCOMPLOUTBYQUOT) {
			const QuotIdent qi(Item.LocID, PPQUOTK_BASE, Item.CurID, P_Pack->Rec.Object);
			THROW(r = GObj.GetQuot(Item.GoodsID, qi, 0.0, 0.0, &quot));
		}
		if(ItemNo < 0 && lot_id && OpTypeID == PPOPT_DRAFTEXPEND) {
			int    ret = 0;
			Item.LotID = lot_id;
			THROW(ret = P_BObj->GetSerialNumberByLot(Item.LotID, temp_buf.Z(), 0));
			if(ret > 0) {
				// @v9.8.11 THROW(P_Pack->SnL.AddNumber(ItemNo, temp_buf));
				THROW(P_Pack->LTagL.AddNumber(PPTAG_LOT_SN, ItemNo, temp_buf)); // @v9.8.11 
			}
		}
		else if(P_Trfr->Rcpt.GetLastLot(Item.GoodsID, -lid, sd, &lot_rec) > 0) {
			if(OpTypeID == PPOPT_GOODSACK)
				Price = R5(lot_rec.Price);
			else
				Item.LotID = lot_rec.ID;
		}
		else if(OpTypeID == PPOPT_GOODSORDER) {
			if(P_Trfr->Rcpt.GetLastLot(-labs(Item.GoodsID), -lid, sd, &lot_rec) > 0) {
				Item.QCert = lot_rec.QCertID;
				Item.UnitPerPack = lot_rec.UnitPerPack;
			}
		}
	}
	else if(Item.LotID) {
		dir = 2; // Don't enumerate lots
		Price = 0.0;
	}
	else {
		P_Trfr->Rcpt.GetListOfOpenedLots(-1, Item.GoodsID, Item.LocID, Item.Date, &lot_list);
	}
	lot_idx = 0;
	do {
		if(dir != 2) {
			if(lot_idx < lot_list.getCount()) {
				lot_rec = lot_list.at(lot_idx++);
				Item.LotID = lot_rec.ID;
				all_lots_in_pckg = 0;
				if(Item.Flags & PPTFR_MINUS && P_BObj->IsLotInPckg(Item.LotID) > 0) {
					all_lots_in_pckg = 1;
					continue;
				}
				else {
					//
					// При выборе товара по цене останавливаемся на том лоте, который имеет выбранную цену.
					//
					double p = R5(lot_rec.Price);
					if(St & stGoodsByPrice && p != Item.Price) {
						Rest = 0.0;
						continue;
					}
					else {
						//
						// Если документ привязан к конкретному поставщику (кроме прихода),
						// то проверяем чтобы товар был оприходован от этого поставщика.
						//
						if(OpID == _PPOPK_SUPPLRET || (P_Pack->OpTypeID == PPOPT_GOODSREVAL && P_Pack->Rec.Object))
							if(lot_rec.SupplID != P_Pack->Rec.Object) {
								Rest = 0.0;
								continue;
							}
						if(P_Pack->OpTypeID == PPOPT_GOODSREVAL && P_Pack->SearchLot(Item.LotID, 0)) {
							Rest = 0.0;
							continue;
						}
					}
				}
			}
			else if(Item.Flags & PPTFR_RECEIPT)
				Item.LotID = 0;
			else if(all_lots_in_pckg) {
				again = 1;
				CALLEXCEPT_PP(PPERR_ALLLOTSINPCKG);
			}
			else if(Item.Flags & PPTFR_MINUS && GObj.CheckFlag(Item.GoodsID, GF_AUTOCOMPL)) {
				Item.Flags |= PPTFR_AUTOCOMPL;
				Item.LotID  = 0;
			}
			else {
				again = 1;
				CALLEXCEPT_PP_S(PPERR_NOGOODS, GetGoodsName(Item.GoodsID, temp_buf));
			}
		}
		Item.Price = quot;
		Item.Cost  = 0.0;
		Item.Rest_ = 0.0;
		Rest = 0.0;
		{
			if(Item.Flags & PPTFR_AUTOCOMPL) {
				if(P_Trfr->Rcpt.GetLastLot(Item.GoodsID, Item.LocID, MAXDATE, &lot_rec) > 0) {
					Item.Price = R5(lot_rec.Price);
					Item.UnitPerPack = lot_rec.UnitPerPack;
				}
			}
			else if(Item.Flags & PPTFR_REVAL) {
				Item.RevalCost = 0.0;
				Item.Discount = 0.0;
			}
			if(suppl_deal_cost > 0.0) {
				PPOprKind op_rec;
				if(GetOpData(P_Pack->Rec.OpID, &op_rec) > 0 && op_rec.ExtFlags & OPKFX_USESUPPLDEAL)
					Item.Cost = suppl_deal_cost;
			}
			//
			// Наследуем теги из предыдущего лота данного товара
			//
			if(ItemNo < 0 && Item.LotID && Item.Flags & PPTFR_RECEIPT) {
				ObjTagList inh_tag_list;
				P_BObj->GetTagListByLot(Item.LotID, 1, &inh_tag_list);
				const uint tc = inh_tag_list.GetCount();
				if(tc) {
					PPObjectTag tag_rec;
					for(uint i_ = 0; i_ < tc; i_++) {
						const ObjTagItem * p_tag = inh_tag_list.GetItemByPos(i_);
						if(p_tag && TagObj.Fetch(p_tag->TagID, &tag_rec) > 0 && tag_rec.Flags & OTF_INHERITABLE)
							InheritedLotTagList.PutItem(p_tag->TagID, p_tag);
					}
				}
			}
			THROW(setupLot());
			setCtrlData(CTL_LOT_DISCOUNT, &Item.Discount);
		}
	} while(dir != 2 && Item.LotID && Rest <= 0.0);
	calcOrderRest();
	setupRest();
	if(Item.GoodsID) {
		{
			const int enbl_lot_ctrl = BIN(oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT) || isModifPlus());
			disableCtrls(!enbl_lot_ctrl, CTL_LOT_EXPIRY, CTLCAL_LOT_EXPIRY, CTLSEL_LOT_QCERT, CTL_LOT_CLB, 0);
			setCtrlReadOnly(CTL_LOT_CLB, !enbl_lot_ctrl);
			setCtrlReadOnly(CTL_LOT_SERIAL, !enbl_lot_ctrl);
		}
		if(P_Pack->OpTypeID != PPOPT_GOODSRETURN && OpID != _PPOPK_SUPPLRET)
			disableCtrl(CTL_LOT_LOT, 0);
		setupQttyFldPrec();
		if(LConfig.Flags & CFGFLG_AUTOQUOT)
			setupQuotation(0, 1);
		if((!Item.LotID && (Item.Cost == 0.0 || Item.Price == 0.0)) && (ItemNo < 0 && (Item.Flags & PPTFR_RECEIPT || IsDraftOp(OpID)))) {
			PPIDArray dyn_gen_list;
			uint spos = 0;
			int  analog_found = 0;
			int  alt_gen_inited = 0;
			if(P_Pack) {
				if(P_Pack->SearchGoods(Item.GoodsID, &spos)) {
					const PPTransferItem & r_ti = P_Pack->ConstTI(spos);
					Item.Cost = r_ti.Cost;
					Item.CurPrice = r_ti.CurPrice;
					Item.Price = r_ti.Price;
					Item.Discount = r_ti.Discount;
					analog_found = 1;
				}
				else if(GObj.P_Tbl->BelongToDynGen(Item.GoodsID, 0, &dyn_gen_list) > 0) {
					alt_gen_inited = 1;
					for(uint i = 0; i < dyn_gen_list.getCount(); i++) {
						const PPID alt_goods_id = dyn_gen_list.get(i);
						if(P_Pack->SearchGoods(alt_goods_id, &(spos = 0))) {
							const PPTransferItem & r_ti = P_Pack->ConstTI(spos);
							Item.Cost = r_ti.Cost;
							Item.CurPrice = r_ti.CurPrice;
							Item.Price = r_ti.Price;
							Item.Discount = r_ti.Discount;
							analog_found = 1;
							break;
						}
					}
				}
			}
			if(!analog_found) {
				if(alt_gen_inited || GObj.P_Tbl->BelongToDynGen(Item.GoodsID, 0, &dyn_gen_list) > 0) {
					const  PPID lid = Item.LocID;
					for(uint i = 0; i < dyn_gen_list.getCount(); i++) {
						const PPID alt_goods_id = dyn_gen_list.get(i);
						if(P_Trfr->Rcpt.GetLastLot(alt_goods_id, -lid, MAXDATE, &lot_rec) > 0) {
							Item.Cost = lot_rec.Cost;
							Item.Price = lot_rec.Price;
							Item.Discount = 0.0;
							analog_found = 1;
							break;
						}
					}
				}
			}
			if(analog_found) {
				setCtrlCost();
				setCtrlReal(CTL_LOT_PRICE, Item.Price);
			}
		}
	}
	else
		disableCtrls(1, CTLSEL_LOT_QCERT, CTL_LOT_LOT, CTL_LOT_EXPIRY, CTLCAL_LOT_EXPIRY, CTL_LOT_CLB, 0);
	setupCtrlsOnGoodsSelection();
	setupVatSum();
	setupVaPct();
	setupBaseQuot();
	CATCH
		if(!recurse)
			ok = 0;
		else {
			if(PPErrCode || !again)
				PPError();
			setCtrlLong(CTLSEL_LOT_GOODS, Item.GoodsID = 0);
			if(again && !(Item.Flags & PPTFR_ONORDER))
				messageToCtrl(CTLSEL_LOT_GOODS, cmCBActivate, 0); // Этот вызов приводит к рекурсивному входу в текущую функцию.
		}
	ENDCATCH
	return ok;
}

int TrfrItemDialog::readQttyFld(uint master, uint ctl, double * val)
{
	double tmp = R6(getCtrlReal(ctl));
	return (tmp == *val && master == ctl) ? 0 : ((*val = tmp), 1);
}

void TrfrItemDialog::setupQuantity(uint master, int readFlds)
{
	if(!(Item.Flags & PPTFR_REVAL) || (Item.Flags & PPTFR_CORRECTION)) {
		// @v9.4.3 const double prev_qtty = Item.Quantity_;
		const double prev_qtty = Item.GetEffCorrectionExpQtty();
		if(readFlds) {
			if(Item.Flags & PPTFR_INDEPPHQTTY)
				getCtrlData(CTL_LOT_INDEPPHQTTY, &Item.WtQtty);
			THROW(readQttyFld(master, CTL_LOT_UNITPERPACK, &Item.UnitPerPack));
			THROW(readQttyFld(master, CTL_LOT_PACKS,       &NumPacks));
			THROW(readQttyFld(master, CTL_LOT_QUANTITY,    &Item.Quantity_));
		}
		if(Item.UnitPerPack != 0.0) {
			if(oneof2(master, CTL_LOT_PACKS, CTL_LOT_UNITPERPACK) || (!master && Item.Quantity_ == 0.0 && NumPacks > 0.0)) {
				if(ffrac(NumPacks) == 0.0)
					Item.Quantity_ = Item.UnitPerPack * NumPacks;
			}
			else if(master == 0 || master == CTL_LOT_QUANTITY)
				NumPacks = Item.Quantity_ / Item.UnitPerPack;
		}
		else
			NumPacks = 0;
		setCtrlReal(CTL_LOT_UNITPERPACK, Item.UnitPerPack);
		setCtrlReal(CTL_LOT_PACKS,       NumPacks);
		setCtrlReal(CTL_LOT_QUANTITY,    Item.Quantity_);
		// @v9.4.3 {
		if(Item.IsCorrectionExp()) {
			const double eff_qtty = Item.GetEffCorrectionExpQtty();
			Rest -= (prev_qtty - eff_qtty);
			if(Item.Flags & PPTFR_ONORDER) {
				OrdRest -= (prev_qtty - eff_qtty);
				if(P_OrderItem && P_OrderItem->TFlags & PPTransferItem::tfOrdReserve)
					OrdReserved -= (prev_qtty - eff_qtty);
			}
		} // } // @v9.4.3
		else if(Item.Flags & PPTFR_ORDER) {
			OrdRest -= (prev_qtty - Item.Quantity_);
			if(Item.TFlags & PPTransferItem::tfOrdReserve)
				OrdReserved -= (prev_qtty - Item.Quantity_);
		}
		else if(Item.Flags & PPTFR_MINUS) {
			Rest += (prev_qtty - Item.Quantity_);
			if(Item.Flags & PPTFR_ONORDER) {
				OrdRest += (prev_qtty - Item.Quantity_);
				if(P_OrderItem && P_OrderItem->TFlags & PPTransferItem::tfOrdReserve)
					OrdReserved += (prev_qtty - Item.Quantity_);
			}
		}
		else
			Rest -= (prev_qtty - Item.Quantity_);
	}
	else {
		if(readFlds && Item.IsRecomplete()) {
			if(Item.Flags & PPTFR_INDEPPHQTTY)
				getCtrlData(CTL_LOT_INDEPPHQTTY, &Item.WtQtty);
		}
		setCtrlReal(CTL_LOT_QUANTITY, Item.Qtty());
	}
	{
		SString phq_txt;
		double phuperu;
		if(Item.Flags & PPTFR_INDEPPHQTTY)
			setCtrlReal(CTL_LOT_INDEPPHQTTY, R6(Item.WtQtty));
		else if(GObj.GetPhUPerU(Item.GoodsID, 0, &phuperu) > 0)
			phq_txt.Cat(R6(Item.Qtty() * phuperu), MKSFMTD(0, 6, NMBF_NOZERO|NMBF_NOTRAILZ));
		setStaticText(CTL_LOT_PHQTTY, phq_txt);
	}
	setupRest();
	CATCH
		;
	ENDCATCH
}

void TrfrItemDialog::setupVatSum()
{
	if(getCtrlView(CTL_LOT_ST_VATSUM)) {
		SString out_buf;
		double vat_sum = 0.0;
		double qtty = getCtrlReal(CTL_LOT_QUANTITY);
		if(qtty != 0.0) {
			if(P_Pack) {
				GTaxVect vect;
				vect.CalcTI(&Item, P_Pack->Rec.OpID, (OpTypeID == PPOPT_GOODSRECEIPT) ? TIAMT_COST : TIAMT_AMOUNT);
				vat_sum = vect.GetValue(GTAXVF_VAT);
				// @v9.0.2 PPGetWord(PPWORD_VAT, 0, out_buf);
				PPLoadString("vat", out_buf); // @v9.0.2
				out_buf.CatDiv(':', 2).Cat(vat_sum, MKSFMTD(0, 4, 0));
			}
		}
		setStaticText(CTL_LOT_ST_VATSUM, out_buf);
	}
}

void TrfrItemDialog::setupVaPct()
{
	if(getCtrlView(CTL_LOT_ST_VAPCT)) {
		SString out_buf;
		if(P_BObj->CheckRights(BILLRT_ACCSCOST)) {
			double pc = TR5(Item.Price);
			double ds = TR5(Item.Discount);
			getCtrlCost();
			getCtrlData(CTL_LOT_PRICE,    &pc);
			getCtrlData(CTL_LOT_DISCOUNT, &ds);
			if(Item.Cost > 0.0 && (pc - ds) > 0.0) {
				PPGetWord(PPWORD_PCTADDEDVAL, 0, out_buf);
				out_buf.CatDiv(':', 2).Cat(100.0 * (pc - ds - Item.Cost) / Item.Cost, MKSFMTD(0, 1, 0));
			}
			else
				out_buf = 0;
		}
		setStaticText(CTL_LOT_ST_VAPCT, out_buf);
	}
}

int TrfrItemDialog::isDiscountInSum() const
{
	return (Item.LotID && (!Item.IsLotRet() || Item.IsCorrectionExp()) &&
		((LConfig.Flags & CFGFLG_DISCOUNTBYSUM && !(Item.Flags & PPTFR_RECEIPT) && P_Pack->OpTypeID != PPOPT_DRAFTRECEIPT) ||
		IsIntrExpndOp(OpID)));
}

void TrfrItemDialog::setupBaseQuot()
{
	if(getCtrlView(CTL_LOT_BASEQUOT)) {
		double q = 0.0;
		if(Item.GoodsID) {
			double price = getCtrlReal(CTL_LOT_PRICE);
			getCtrlCost();
			QuotIdent qi(GetQuotLocID(), PPQUOTK_BASE, Item.CurID, 0);
			GObj.GetQuotExt(Item.GoodsID, qi, Item.Cost, price, &q, 1);
		}
		setCtrlReal(CTL_LOT_BASEQUOT, q);
	}
}

void TrfrItemDialog::setupPriceLimit()
{
	if(getCtrlView(CTL_LOT_ST_PRICELIMIT)) {
		SString text_buf;
		RealRange range;
		if(GetPriceRestrictions(&range) > 0)
			text_buf.Cat(range.low, MKSFMTD(0, 2, NMBF_NOZERO)).Dot().Dot().Cat(range.upp, MKSFMTD(0, 2, NMBF_NOZERO));
		setStaticText(CTL_LOT_ST_PRICELIMIT, text_buf);
	}
}

int TrfrItemDialog::setupManuf()
{
	int    ok = -1;
	if(P_Pack && Item.Flags & PPTFR_RECEIPT && getCtrlView(CTLSEL_LOT_MANUF)) {
		const PPID mnf_lot_tag_id = P_BObj->GetConfig().MnfCountryLotTagID;
		PPObjectTag tag_rec;
		if(mnf_lot_tag_id && TagObj.Fetch(mnf_lot_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
			PPID   manuf_id = 0;
			const ObjTagItem * p_mnf_tag = P_Pack->LTagL.GetTag(ItemNo, mnf_lot_tag_id);
			CALLPTRMEMB(p_mnf_tag, GetInt(&manuf_id));
			SetupPersonCombo(this, CTLSEL_LOT_MANUF, manuf_id, OLW_CANINSERT, tag_rec.LinkObjGrp, 1);
            setLabelText(CTL_LOT_MANUF, tag_rec.Name);
			ok = 1;
		}
	}
	showCtrl(CTLSEL_LOT_MANUF, ok > 0);
	showCtrl(CTL_LOT_MANUF, ok > 0);
	return ok;
}

int TrfrItemDialog::getManuf()
{
	int    ok = -1;
	if(P_Pack && Item.Flags & PPTFR_RECEIPT && getCtrlView(CTLSEL_LOT_MANUF)) {
		const PPID mnf_lot_tag_id = P_BObj->GetConfig().MnfCountryLotTagID;
		PPObjectTag tag_rec;
		if(mnf_lot_tag_id && TagObj.Fetch(mnf_lot_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
			PPID   manuf_id = 0;
			getCtrlData(CTLSEL_LOT_MANUF, &manuf_id);
            ObjTagItem mnf_tag;
            mnf_tag.SetInt(mnf_lot_tag_id, manuf_id);

			ObjTagList tag_list;
			ObjTagList * p_list = P_Pack->LTagL.Get(ItemNo);
			RVALUEPTR(tag_list, p_list);
			tag_list.PutItem(mnf_lot_tag_id, &mnf_tag);
			P_Pack->LTagL.Set(ItemNo, &tag_list);
			ok = 1;
		}
	}
	return ok;
}

int TrfrItemDialog::IsSourceSerialUsed()
{
	int    yes = 0;
	if(P_Pack && IsTaggedItem() && getCtrlView(CTL_LOT_SOURCESERIAL)) {
		PPOprKind op_rec;
		PPObjectTag tag_rec;
		if(GetOpData(P_Pack->Rec.OpID, &op_rec) > 0 && op_rec.ExtFlags & OPKFX_SOURCESERIAL && TagObj.Fetch(PPTAG_LOT_SOURCESERIAL, &tag_rec) > 0)
			yes = 1;
	}
	return yes;
}

int TrfrItemDialog::setDTS(PPTransferItem * pItem)
{
	int    ok = 1;
	Goods2Tbl::Rec grp_rec;
	ushort v;
	SString temp_buf;
	Item  = *pItem;
	Price = Item.Price;
	setCtrlCost();
	setupCtrls();
	if(Item.GoodsID) {
		Goods2Tbl::Rec goods_rec;
		THROW(GObj.Fetch(Item.GoodsID, &goods_rec) > 0);
		GoodsGrpID = goods_rec.ParentID;
	}
	else {
		disableCtrl(CTL_LOT_LOT, 1);
		GoodsGrpID = 0;
	}
	if(St & stGoodsFixed && GoodsGrpID && GObj.Fetch(GoodsGrpID, &grp_rec) > 0)
		SetComboBoxLinkText(this, CTLSEL_LOT_GOODSGRP, grp_rec.Name);
	else {
		PPID   parent_grp = (GoodsGrpID && GObj.Fetch(GoodsGrpID, &grp_rec) > 0) ? grp_rec.ParentID : 0;
		SetupPPObjCombo(this, CTLSEL_LOT_GOODSGRP, PPOBJ_GOODSGROUP, GoodsGrpID, OLW_LOADDEFONOPEN|OLW_CANINSERT, (void *)parent_grp);
	}
	if(GoodsGrpID)
		setupGoodsList();
	if(pItem->Flags & PPTFR_REVAL) {
		setCtrlReal(CTL_LOT_OLDCOST,  Item.RevalCost);
		setCtrlReal(CTL_LOT_OLDPRICE, Item.Discount);
	}
	else {
		// @v9.4.3 {
		if(P_Pack->OpTypeID == PPOPT_CORRECTION) {
			/*
			if(P_Pack->P_LinkPack) {
				uint   _pos = 0;
				if(P_Pack->P_LinkPack->SearchTI(Item.RByBill, &_pos)) {
					const PPTransferItem & r_org_ti = P_Pack->P_LinkPack->ConstTI(_pos);
					OrgQtty = fabs(r_org_ti.Quantity_);
					OrgPrice = r_org_ti.NetPrice();
				}
			}
			*/
			OrgQtty = Item.QuotPrice;
			OrgPrice = Item.RevalCost;
			setCtrlReal(CTL_LOT_ORGQTTY, OrgQtty);
			setCtrlReal(CTL_LOT_OLDPRICE, OrgPrice);
		}
		// } @v9.4.3
		if(isDiscountInSum()) {
			Item.Price   -= Item.Discount;
			Item.Discount = 0.0;
		}
	}
	SetupPPObjCombo(this, CTLSEL_LOT_INTAXGRP, PPOBJ_GOODSTAX, Item.LotTaxGrpID, OLW_LOADDEFONOPEN|OLW_CANINSERT, 0);
	{
		SString text;
		PPObjCurrency cur_obj;
		PPCurrency cur_rec;
		if(Item.CurID) {
			if(cur_obj.Fetch(Item.CurID, &cur_rec) > 0)
				text = cur_rec.Symb;
		}
		else if(cur_obj.Fetch(LConfig.BaseCurID, &cur_rec) > 0)
			text.CatChar('*').Cat(cur_rec.Symb);
		setStaticText(CTL_LOT_CURSYMB, text);
	}
	setCtrlData(CTL_LOT_CURPRICE, &Item.CurPrice);
	v = BIN(Item.Flags & PPTFR_COSTWOVAT);
	setCtrlData(CTL_LOT_NOVAT, &v);
	AddClusterAssoc(CTL_LOT_FIXEDMODIFCOST, 0, PPTFR_FIXMODIFCOST);
	SetClusterData(CTL_LOT_FIXEDMODIFCOST, Item.Flags);
	THROW(setupLot());
	setCtrlData(CTL_LOT_DISCOUNT, &Item.Discount);
	calcOrderRest();
	setupRest();
	setupVatSum();
	setupVaPct();
	setupBaseQuot();
	setupPriceLimit();
	SetupPPObjCombo(this, CTLSEL_LOT_QCERT, PPOBJ_QCERT, Item.QCert, OLW_LOADDEFONOPEN|OLW_CANINSERT, 0L);
	{
		QCertCtrlGroup::Rec qc_rec(Item.QCert);
		setGroupData(GRP_QCERT, &qc_rec);
	}
	if(St & stAllowSupplSel)
		SetupArCombo(this, CTLSEL_LOT_SUPPL, Item.Suppl, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetSupplAccSheet(), sacfNonGeneric);
	else {
		showCtrl(CTL_LOT_SUPPL, 0);
		showCtrl(CTLSEL_LOT_SUPPL, 0);
	}
	if(Item.Flags & PPTFR_RECEIPT) {
		int    subtyp = GetOpSubType(OpID);
		if(oneof2(subtyp, OPSUBT_ASSETRCV, OPSUBT_ASSETMODIF)) {
			AddClusterAssoc(CTL_LOT_IMMASSETEXPL, 0, PPTFR_ASSETEXPL);
			SetClusterData(CTL_LOT_IMMASSETEXPL, Item.Flags);
			if(Item.Quantity_ == 0.0 || Item.Quantity_ == 1.0) {
				setCtrlReal(CTL_LOT_QUANTITY, Item.Quantity_ = 1.0);
				disableCtrl(CTL_LOT_QUANTITY, 1);
			}
		}
	}
	if(ItemNo < 0 && Item.GoodsID) {
		Item.UnitPerPack = 0.0;
		THROW(replyGoodsSelection(0));
	}
	else if(OpTypeID == PPOPT_GOODSRECEIPT) {
		QuotIdent qi(Item.LocID, 0, Item.CurID, P_Pack->Rec.Object);
		GObj.GetSupplDeal(Item.GoodsID, qi, &Sd, 1);
	}
	if(P_Pack) {
		if(ItemNo >= 0) {
			// @v9.8.11 if(P_Pack->ClbL.GetNumber(ItemNo, &temp_buf) > 0)
			if(P_Pack->LTagL.GetNumber(PPTAG_LOT_CLB, ItemNo, temp_buf) > 0) // @v9.8.11 
				setCtrlString(CTL_LOT_CLB, temp_buf);
			// @v9.8.11 if(P_Pack->SnL.GetNumber(ItemNo, &temp_buf) > 0)
			if(P_Pack->LTagL.GetNumber(PPTAG_LOT_SN, ItemNo, temp_buf) > 0) // @v9.8.11 
				setCtrlString(CTL_LOT_SERIAL, temp_buf);
		}
		else if(IsTaggedItem() && P_BObj->GetConfig().Flags & BCF_AUTOSERIAL) {
			if(getCtrlString(CTL_LOT_SERIAL, temp_buf) && temp_buf.Empty())
				GenerateSerial();
		}
		else {
			SetupInheritedSerial();
		}
	}
	// @v9.3.6 {
	if(IsSourceSerialUsed()) {
		temp_buf.Z();
		if(ItemNo >= 0)
			P_Pack->LTagL.GetTagStr(ItemNo, PPTAG_LOT_SOURCESERIAL, temp_buf);
		showCtrl(CTL_LOT_SOURCESERIAL, 1);
		showButton(cmSourceSerial, 1);
		setCtrlString(CTL_LOT_SOURCESERIAL, temp_buf);
	}
	else {
		showCtrl(CTL_LOT_SOURCESERIAL, 0);
		showButton(cmSourceSerial, 0);
	}
	// } @v9.3.6
	setupManuf();
	CATCHZOK
	return ok;
}
//
// Helper function (called from TrfrItemDialog::getDTS())
//
int TrfrItemDialog::checkQuantityVal(double * pExtraQtty)
{
	int    ok = 1;
	double _rest = 0;
	const  double _qtty = Item.Quantity_;
	SString msg_buf;
	*pExtraQtty = 0;
	THROW_PP(MinQtty <= 0 || fabs(_qtty) >= MinQtty, PPERR_TIMINQTTY);
	THROW_PP(MaxQtty <= 0 || fabs(_qtty) <= MaxQtty, PPERR_TIMAXQTTY);
	if(Rest < 0.0 && !(Item.Flags & (PPTFR_UNLIM|PPTFR_AUTOCOMPL|PPTFR_ACK|PPTFR_DRAFT))) {
		PPObject::SetLastErrObj(PPOBJ_GOODS, labs(Item.GoodsID));
		THROW_PP(TotalRest >= 0 && OpTypeID != PPOPT_GOODSRETURN && (Item.Flags & PPTFR_MINUS), PPERR_LOTRESTBOUND);
		if(PPMessage(mfConf|mfYes|mfCancel, PPCFM_LOTRESTBOUND) == cmYes) {
			Item.Quantity_ = 0.0;
			THROW(P_Pack->BoundsByLot(Item.LotID, &Item, ItemNo, &_rest, 0));
			Item.Quantity_ = R6(_rest);
			*pExtraQtty   = R6(_qtty - _rest);
			ok = 2;
		}
		else
			ok = -1;
	}
	if(OrdReserved > 0 && P_BObj->Cfg.Flags & BCF_CHECKRESERVEDORDERS && !(Item.Flags & (PPTFR_UNLIM|PPTFR_ACK|PPTFR_DRAFT|PPTFR_ORDER|PPTFR_PLUS))) {
		//
		// Если редактируется существующая строка и количество по ней уменьшается, то не
		// проверяем нарушение зарезервированного заказа, поскольку такая операция приведет к увеличению
		// доступного остатка для зарезервированного заказа.
		//
		int    skip_rsrv_test = 0;
		if(Item.BillID && Item.RByBill) {
			TransferTbl::Rec trfr_rec;
			if(P_Trfr->SearchByBill(Item.BillID, 0, Item.RByBill, &trfr_rec) > 0) {
				double org_qtty = fabs(trfr_rec.Quantity);
				if(fabs(Item.Quantity_) < org_qtty)
					skip_rsrv_test = 1;
			}
		}
		if(!skip_rsrv_test) {
			GetGoodsName(labs(Item.GoodsID), msg_buf.Z());
			THROW_PP_S((TotalRest - OrdReserved) >= 0, PPERR_ORDRESERVEEXHAUSTED, msg_buf.CatDiv(':', 1).Cat(OrdReserved, NMBF_NOTRAILZ));
		}
	}
	{
		PPBillPacket::CgrRetBlock cgr_ret_blk;
		int    rc = P_Pack->CheckGoodsForRestrictions(ItemNo, Item.GoodsID, Item.GetSign(OpID), _qtty,
			PPBillPacket::cgrfShipmControl|PPBillPacket::cgrfQtty|PPBillPacket::cgrfModifCmplmnry, &cgr_ret_blk);
		THROW(rc);
		if(rc == 100) {
			msg_buf.Z().Cat(fabs(_qtty), MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatDiv(':', 2).Cat(cgr_ret_blk.ScpRange.low).
				CatCharN('.', 2).Cat(cgr_ret_blk.ScpRange.upp);
			if(PPMessage(mfConf|mfYes|mfCancel, PPCFM_TRFRSHIPMCTRL, msg_buf) == cmYes)
				ok = 1;
			else
				ok = -1;
		}
	}
	CATCH
		Item.Quantity_ = _qtty;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPObjBill::GetPriceRestrictions(PPBillPacket & rPack, const PPTransferItem & rTi, int itemPos, RealRange * pRange)
{
	int    ok = -1;
	RealRange range;
	range.SetVal(0.0);
	PPObjGoodsType gt_obj;
	PPGoodsType gt_rec;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(rTi.GoodsID, &goods_rec) > 0 && gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec) > 0) {
		if(gt_rec.PriceRestrID) {
			PPObjGoodsValRestr gvr_obj;
			PPGoodsValRestrPacket gvr_pack;
			if(gvr_obj.Fetch(gt_rec.PriceRestrID, &gvr_pack) > 0) {
				if(gvr_pack.LowBoundFormula.NotEmptyS()) {
					double bound = 0.0;
					rPack.SetTPointer(itemPos);
					GdsClsCalcExprContext ctx(&rTi, &rPack);
					if(PPCalcExpression(gvr_pack.LowBoundFormula, &bound, &ctx) && bound > 0.0) {
						range.low = bound;
						ok = 1;
					}
				}
				if(gvr_pack.UppBoundFormula.NotEmptyS()) {
					double bound = 0.0;
					//
					// При редактировании строки приходного документа LotID
					// "затирается". Поэтому создаем временный PPTransferItem и
					// устанавливаем в нем LotID равный тому, что в пакете.
					//
					rPack.SetTPointer(itemPos);
					GdsClsCalcExprContext ctx(&rTi, &rPack);
					if(PPCalcExpression(gvr_pack.UppBoundFormula, &bound, &ctx) && bound > 0.0) {
						range.upp = bound;
						ok = 1;
					}
				}
			}
		}
	}
	/*
	CATCHZOK
	*/
	ASSIGN_PTR(pRange, range);
	return ok;
}

int TrfrItemDialog::GetPriceRestrictions(RealRange * pRange)
{
	int    ok = -1;
	RealRange range;
	range.SetVal(0.0);
	PPOprKind op_rec;
	GetOpData(OpID, &op_rec);
	if(op_rec.ExtFlags & OPKFX_RESTRICTPRICE) {
		ok = P_BObj->GetPriceRestrictions(*P_Pack, Item, ItemNo, &range);
	}
	ASSIGN_PTR(pRange, range);
	return ok;
}

int TrfrItemDialog::CheckPrice()
{
	int    ok = 1;
	SString msg;
	if(OpTypeID == PPOPT_GOODSRECEIPT || (P_Pack->Rec.OpID == CConfig.DraftRcptOp && CConfig.Flags2 & CCFLG2_USESDONPURCHOP)) {
		int    ret = Sd.CheckCost(Item.Cost);
		if(!ret) {
			int    invp_act = DS.GetTLA().InvalidSupplDealQuotAction;
			if(invp_act == PPSupplAgreement::invpaWarning)
				ret = CONFIRM(PPCFM_INVSUPPLDEAL);
			else if(invp_act == PPSupplAgreement::invpaDoNothing)
				ret = 1;
			THROW_PP_S(ret, PPERR_SUPPLDEALVIOLATION, Sd.Format(msg));
		}
	}
	{
		RealRange range;
		if(GetPriceRestrictions(&range) > 0) {
			if(range.low > 0.0) {
				msg.Z().Cat(range.low, SFMT_MONEY);
				THROW_PP_S(Item.NetPrice() >= range.low, PPERR_PRICERESTRLOW, msg);
			}
			if(range.upp > 0.0) {
				msg.Z().Cat(range.upp, SFMT_MONEY);
				THROW_PP_S(Item.NetPrice() <= range.upp, PPERR_PRICERESTRUPP, msg);
			}
		}
	}
	CATCHZOK
	return ok;
}

int TrfrItemDialog::getDTS(PPTransferItem * pItem, double * pExtraQtty)
{
	int    ok = 1;
	int    r;
	int    sel = 0;
	int    no_err_msg = 0;
	long   o;
	double neck, ph_neck, extra_qtty = 0L;
	double cur_price = TR5(Item.CurPrice);
	double ct = TR5(Item.Cost);
	double pc = TR5(Item.Price);
	double ds = TR5(Item.Discount);
	SString clb_number;
	SString temp_buf;
	const  int op_subtype = GetOpSubType(OpID);
	ASSIGN_PTR(pExtraQtty, 0L);
	if(!(St & stGoodsFixed) || !Item.GoodsID)
		getCtrlData(sel = CTLSEL_LOT_GOODS, &Item.GoodsID);
	THROW_PP(Item.GoodsID, PPERR_GOODSNEEDED);
	THROW(Item.SetupGoods(Item.GoodsID, TISG_SETPWOTF) > 0);
	sel = CTL_LOT_QUANTITY;
	setupQuantity(0/*sel*/, 1);
	THROW(checkQuantityForIntVal());
	THROW_PP((Item.Flags & PPTFR_REVAL) || Item.IsCorrectionExp() || oneof2(P_Pack->Rec.OpID, PPOPK_EDI_STOCK, PPOPK_EDI_SHOPCHARGEON) ||
		Item.Quantity_ > 0.0, PPERR_INVQTTY); // @v9.3.1 P_Pack->Rec.OpID == PPOPK_EDI_STOCK // @v9.4.3 Item.IsCorrectionExp()
	THROW(r = checkQuantityVal(&extra_qtty));
	ASSIGN_PTR(pExtraQtty, extra_qtty);
	if(r < 0) {
		no_err_msg = 1;
		CALLEXCEPT();
	}
	else if(Item.IsUnlimWoLot() || (Item.Flags & PPTFR_RECEIPT && !Item.RByBill)) {
		Item.LotID = 0;
	}
	else if(Item.LotID && (Item.Flags & PPTFR_MINUS) && !(Item.Flags & PPTFR_DRAFT)) {
		if(Item.BillID == 0 || Item.RByBill == 0) {
			neck    = -fabs(Item.Quantity_);
			ph_neck = -fabs(Item.WtQtty);
			THROW(IncDateKey(P_Trfr, 1, P_Pack->Rec.Dt, &o));
			sel = CTL_LOT_QUANTITY;
			THROW(P_Trfr->UpdateForward(Item.LotID, P_Pack->Rec.Dt, o, 1, &neck, &ph_neck) > 0);
		}
		else {
			THROW(r = P_Trfr->SearchByBill(Item.BillID, 0, Item.RByBill, 0));
			if(r > 0) {
				neck    = -fabs(Item.Quantity_) - P_Trfr->data.Quantity;
				ph_neck = -fabs(Item.WtQtty) - R6(P_Trfr->data.WtQtty);
				o    = P_Trfr->data.OprNo;
				sel = CTL_LOT_QUANTITY;
				THROW(P_Trfr->UpdateForward(Item.LotID, P_Pack->Rec.Dt, o, 1, &neck, &ph_neck) > 0);
			}
			else
				Item.RByBill = 0;
		}
	}
	getCtrlCost();
	getCtrlData(CTL_LOT_PRICE,    &pc);
	getCtrlData(CTL_LOT_DISCOUNT, &ds);
	pc = TR5(pc);
	ds = TR5(ds);
	if(Item.Flags & PPTFR_RECEIPT && oneof2(op_subtype, OPSUBT_ASSETRCV, OPSUBT_ASSETMODIF)) {
		GetClusterData(CTL_LOT_IMMASSETEXPL, &Item.Flags);
		if(op_subtype != OPSUBT_ASSETMODIF) {
			pc = Item.Cost; // Цена реализации (балансовая стоимость) для основных средств
				// автомататически равна цене поступлени (если речь не идет о модификации основных средств)
			ds = 0.0;
		}
	}
	if(Item.CurID) {
		double base_price = 0.0;
		getCtrlData(CTL_LOT_CURPRICE, &cur_price);
		cur_price = TR5(cur_price);
		evaluateBasePrice(cur_price, &base_price);
		if(Item.Flags & PPTFR_SELLING) {
			pc = base_price;
			ds = 0.0;
		}
		else
			Item.SetOrgCost(base_price);
	}
	if(Item.Flags & PPTFR_UNLIM) {
		if(OpTypeID == PPOPT_GOODSORDER || Item.Flags & PPTFR_MINUS) {
			sel = CTL_LOT_PRICE;
			THROW_PP(pc > 0.0 || (pc == 0.0 && isAllowZeroPrice()), PPERR_INVPRICE);
			Item.SetZeroCost();
		}
		else if(OpTypeID != PPOPT_GOODSRETURN) {
			sel = CTL_LOT_COST;
			THROW_PP(Item.GetOrgCost() > 0.0, PPERR_INVCOST);
		}
	}
	else {
		sel = CTL_LOT_PRICE;
		THROW_PP(pc > 0.0 || (pc == 0.0 && isAllowZeroPrice()) || OpID == PPOPK_EDI_ACTCHARGEON, PPERR_INVPRICE);
		if(OpTypeID == PPOPT_GOODSORDER) {
			// @v9.2.9 ds = 0.0;
		}
		else {
			if(!isModifPlus()) {
				sel = CTL_LOT_COST;
				THROW_PP(Item.GetOrgCost() >= 0.0, PPERR_INVCOST);
			}
		}
	}
	if(Item.Flags & PPTFR_REVAL) {
		if(Item.Flags & PPTFR_CORRECTION) {
			THROW_PP(Item.Cost != Item.RevalCost || Item.Quantity_ != OrgQtty, PPERR_ZEROTICORRECTION);
		}
		else {
			//
			// Если это - не ввод в эксплуатацию объекта основных средств
			//
			if(Item.Flags & PPTFR_ASSETEXPL) {
				pc = Item.Cost;
			}
			else if(!Item.IsRecomplete()) {
				//
				// Если изменилась цена поступления, то должен быть указан поставщик
				//
				if(!(P_BObj->Cfg.Flags & BCF_ALLOWZSUPPLINCOSTREVAL))
					THROW_PP(P_Pack->Rec.Object || Item.Cost == Item.RevalCost, PPERR_REVALWOSUPPL);
				//
				// В любом случае должна измениться цена поступления или реализации
				//
				THROW_PP(pc != ds || Item.Cost != Item.RevalCost, PPERR_ZEROREVAL);
			}
		}
	}
	else {
		Item.Rest_ = R6(Rest);
		if(Item.Flags & PPTFR_DRAFT) {
			pc = pc - ds;
			ds = 0;
		}
		else {
			if(isDiscountInSum()) {
				ds = Price - pc;
				pc = Price;
			}
			if(Item.Flags & PPTFR_CORRECTION) {
				THROW_PP(!feqeps(Item.NetPrice(), pc-ds, 1.0E-6) || Item.Quantity_ != OrgQtty, PPERR_ZEROTICORRECTION);
			}
		}
		sel = CTL_LOT_DISCOUNT;
		THROW_PP(ds <= pc, PPERR_INVDISCOUNT);
	}
	Item.CurPrice = TR5(cur_price);
	Item.Price    = TR5(pc);
	Item.Discount = TR5(ds);
	THROW(CheckPrice());
	//
	// @v4.3.7 {
	// Это временный код. Попытка исправить проблему с драфтами,
	// возникшую в предыдущих версиях, когда скидка становилась равной
	// цене реализации в документах с типом PPOPT_DRAFTRECEIPT.
	if(P_Pack->OpTypeID == PPOPT_DRAFTRECEIPT)
		Item.Discount = 0.0;
	// }
	getCtrlData(CTLSEL_LOT_QCERT, &Item.QCert);
	if(getCtrlView(CTL_LOT_QCERT)) {
		QCertCtrlGroup::Rec qc_rec;
		getGroupData(GRP_QCERT, &qc_rec);
		Item.QCert = qc_rec.QCertID;
	}
	getCtrlData(CTL_LOT_EXPIRY, &Item.Expiry);
	THROW_SL(checkdate(Item.Expiry, 1));
	if(St & stAllowSupplSel) {
		getCtrlData(CTLSEL_LOT_SUPPL, &Item.Suppl);
		SETFLAG(Item.Flags, PPTFR_FORCESUPPL, Item.Suppl);
	}
	if(IsTaggedItem()) {
		if(P_Pack->OpTypeID != PPOPT_GOODSORDER) {
			ushort v = 0;
			if(getCtrlData(CTL_LOT_NOVAT, &v))
				SETFLAG(Item.Flags, PPTFR_COSTWOVAT, v);
			GetClusterData(CTL_LOT_FIXEDMODIFCOST, &Item.Flags);
			getCtrlString(CTL_LOT_CLB, clb_number.Z());
			// @v9.8.11 THROW(P_Pack->ClbL.AddNumber(ItemNo, clb_number));
			THROW(P_Pack->LTagL.AddNumber(PPTAG_LOT_CLB, ItemNo, clb_number)); // @v9.8.11 
			getCtrlData(CTLSEL_LOT_INTAXGRP, &Item.LotTaxGrpID);
		}
		getCtrlString(CTL_LOT_SERIAL, clb_number.Z());
		P_BObj->AdjustSerialForUniq(Item.GoodsID, Item.LotID, 0, clb_number);
		// @v9.8.11 THROW(P_Pack->SnL.AddNumber(ItemNo, clb_number));
		THROW(P_Pack->LTagL.AddNumber(PPTAG_LOT_SN, ItemNo, clb_number)); // @v9.8.11 
		// @v9.3.6 {
		if(IsSourceSerialUsed()) {
			getCtrlString(CTL_LOT_SOURCESERIAL, temp_buf);
			ObjTagList tag_list;
			const ObjTagList * p_tag_list = P_Pack->LTagL.Get(ItemNo);
			RVALUEPTR(tag_list, p_tag_list);
			ObjTagItem tag_item;
			if(tag_item.SetStr(PPTAG_LOT_SOURCESERIAL, temp_buf)) {
				tag_list.PutItem(PPTAG_LOT_SOURCESERIAL, &tag_item);
				P_Pack->LTagL.Set(ItemNo, &tag_list);
			}
		}
		// } @v9.3.6
		if(InheritedLotTagList.GetCount()) {
			ObjTagList tag_list;
			const ObjTagList * p_list = P_Pack->LTagL.Get(ItemNo);
			RVALUEPTR(tag_list, p_list);
			tag_list.Merge(InheritedLotTagList, ObjTagList::mumAdd);
			P_Pack->LTagL.Set(ItemNo, &tag_list);
		}
	}
	else if(IsIntrExpndOp(P_Pack->Rec.OpID)) {
		{
			ObjTagList tag_list;
			P_BObj->GetTagListByLot(Item.LotID, 0, &tag_list); // @v9.8.11 skipReserved 1-->0
			P_Pack->LTagL.Set(ItemNo, tag_list.GetCount() ? &tag_list : 0);
		}
		/* @v9.8.11 if(P_BObj->GetSerialNumberByLot(Item.LotID, clb_number, 0) > 0) {
			THROW(P_Pack->SnL.AddNumber(ItemNo, clb_number));
		}*/
	}
	if(P_Pack->OpTypeID == PPOPT_DRAFTEXPEND) {
		getCtrlString(CTL_LOT_SERIAL, clb_number.Z());
		// @v9.8.11 THROW(P_Pack->SnL.AddNumber(ItemNo, clb_number));
		THROW(P_Pack->LTagL.AddNumber(PPTAG_LOT_SN, ItemNo, clb_number)); // @v9.8.11 
		if(P_BObj->GetClbNumberByLot(Item.LotID, 0, clb_number.Z()) > 0) {
			// @v9.8.11 THROW(P_Pack->ClbL.AddNumber(ItemNo, clb_number));
			THROW(P_Pack->LTagL.AddNumber(PPTAG_LOT_CLB, ItemNo, clb_number)); // @v9.8.11 
		}
	}
	getManuf();
	*pItem = Item;
	CATCH
		if(!no_err_msg)
			PPError();
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

static SString & minus_ord_reserved(double rest, double reserved, SString & rBuf, long fmt)
{
	if(reserved > 0) {
		long   f1 = fmt;
		SETSFMTFLAG(f1, ALIGN_LEFT | NMBF_NOTRAILZ);
		rBuf.CatChar('-').Cat(reserved, fmt).Strip(1).Eq().Cat(rest-reserved, f1).Strip(1);
	}
	return rBuf;
}

void TrfrItemDialog::setupRest()
{
	long   f = MKSFMTD(0, 6, ALIGN_LEFT | NMBF_NOTRAILZ | NMBF_NOZERO);
	char   qtty_buf[64];
	SString c, temp_buf;
	if(getStaticText(CTL_LOT_STREST, c) > 0) {
		size_t p = 0;
		if(c.StrChr(':', &p))
			c.Trim(p+1);
		else
			c = 0;
		if(!(Item.Flags & PPTFR_UNLIM)) {
			c.Space();
			if(Item.Flags & PPTFR_AUTOCOMPL) {
				PPGetSubStr(Strings, strComplete, temp_buf);
				c.Cat(temp_buf);
			}
			else {
				P_Pack->GoodsRest(Item.GoodsID, &Item, ItemNo, &TotalRest);
				if(!(OpTypeID == PPOPT_GOODSORDER)) {
					if(DS.CheckExtFlag(ECF_TRFRITEMPACK)) {
						if(Rest < 0)
							c.CatChar('-');
						QttyToStr(Rest, Item.UnitPerPack, QTTYF_COMPLPACK|QTTYF_FRACTION|NMBF_NOZERO, qtty_buf);
						c.Cat(qtty_buf);
					}
					else
						c.Cat(Rest, f).Strip(1);
					if((TotalRest && TotalRest != Rest) || OrdReserved > 0) {
						c.Space().CatChar('(');
						if(DS.CheckExtFlag(ECF_TRFRITEMPACK)) {
							QttyToStr(TotalRest, Item.UnitPerPack, QTTYF_COMPLPACK|QTTYF_FRACTION|NMBF_NOZERO, qtty_buf);
							c.Cat(qtty_buf);
						}
						else
							c.Cat(TotalRest, f).Strip(1);
						minus_ord_reserved(TotalRest, OrdReserved, c, f).CatChar(')');
					}
				}
				else if(TotalRest || OrdReserved > 0) {
					c.Space().Cat(TotalRest, f).Strip(1);
					minus_ord_reserved(TotalRest, OrdReserved, c, f);
				}
			}
			if(OrdRest > 0) {
				PPGetSubStr(Strings, strOrderQtty, temp_buf);
				c.Cat(temp_buf).Cat(OrdRest, f).Strip(1);
			}
		}
		setStaticText(CTL_LOT_STREST, c.Strip(1));
	}
}

void TrfrItemDialog::setQuotSign()
{
	SString text;
	if(Item.Flags & PPTFR_QUOT)
		text.CatChar('q');
	if(Item.Flags & PPTFR_PRICEWOTAXES)
		text.CatChar('_');
	if(P_BObj->Cfg.Flags & BCF_SIGNDIFFLOTCOST && OpTypeID == PPOPT_GOODSRECEIPT)
		if(Item.RByBill == 0) {
			ReceiptTbl::Rec lot_rec;
			if(::GetCurGoodsPrice(Item.GoodsID, Item.LocID, GPRET_INDEF, 0, &lot_rec) > 0) {
				getCtrlCost();
				SETFLAG(St, stWasCostInput, Item.Cost != R5(lot_rec.Cost));
				setCtrlCost();
			}
		}
	setStaticText(CTL_LOT_QUOTSIGN, text);
}

int TrfrItemDialog::setupLot()
{
	int    ok = 1;
	uint   fl = 0;
	if(Item.Flags & PPTFR_CORRECTION && Item.Flags & PPTFR_REVAL) { // @v9.4.3
		OrgQtty = 0.0;
	}
	SString temp_buf;
	if(Item.UnitPerPack == 0.0) {
		GoodsStockExt gse;
		if(GObj.GetStockExt(labs(Item.GoodsID), &gse) > 0 && gse.Package > 0)
			Item.UnitPerPack = gse.Package;
	}
	if(Item.LotID && !(Item.Flags & PPTFR_AUTOCOMPL)) {
		ReceiptTbl::Rec lot_rec;
		if(Item.Cost  != 0.0)
			fl |= TISL_IGNCOST;
		if(Item.Price != 0.0)
			fl |= TISL_IGNPRICE;
		else {
			double adj_intr_price = 0.0;
			if(P_BObj->AdjustIntrPrice(P_Pack, Item.GoodsID, &adj_intr_price) > 0) {
				Item.Price = adj_intr_price;
				fl |= TISL_ADJPRICE;
			}
		}
		if(Item.Flags & PPTFR_RECEIPT) {
			fl |= (TISL_IGNQCERT|TISL_IGNEXPIRY);
		}
		THROW(P_Trfr->Rcpt.Search(Item.LotID, &lot_rec) > 0);
		OrgQtty = lot_rec.Quantity;
		THROW(Item.SetupLot(Item.LotID, &lot_rec, fl));
		if(!(fl & TISL_IGNPRICE))
			Price = Item.Price;
		if(Item.Flags & PPTFR_REVAL) {
			// Предварительная попытка разрешить рекомплектацию лотов, которые не были перед этим
			// скомплектованы. Это оказалось актуально при учете ОС.
			for(uint pos = 0; P_Pack->SearchLot(Item.LotID, &pos) > 0; pos++) {
				THROW_PP(pos == (uint)ItemNo, PPERR_DUPLOTREVAL);
			}
			if(Item.RevalCost == 0.0)
				Item.RevalCost = Item.Cost;
			if(Item.Discount == 0.0)
				Item.Discount = Item.Price;
			setCtrlReal(CTL_LOT_OLDCOST,  Item.RevalCost);
			setCtrlReal(CTL_LOT_OLDPRICE, Item.Discount);
			setCtrlReal(CTL_LOT_ORGQTTY, OrgQtty);
			if(GetOpSubType(OpID) == OPSUBT_ASSETEXPL) {
				ushort v = 0;
				PPID   lot_id = Item.LotID;
				int    op_code = 0;
				int    in_expl = 0;
				for(DateIter iter; P_Trfr->EnumAssetOp(&lot_id, &iter, &op_code, 0) > 0;) {
					if(iter.dt >= Item.Date)
						break;
					if(oneof2(op_code, ASSTOPC_RCPTEXPL, ASSTOPC_EXPL))
						in_expl = 1;
					else if(op_code == ASSTOPC_EXPLOUT)
						in_expl = 0;
				}
				if(in_expl) {
					// Объект находится в эксплуатации, следовательно это - вывод из эксплуатации
					Item.Flags |= (PPTFR_ASSETEXPL | PPTFR_MODIF);
					v = 1;
				}
				else {
					// Объект находится вне эксплуатации, следовательно это - ввод в эксплуатацию
					Item.Flags |= PPTFR_ASSETEXPL;
					Item.Flags &= ~PPTFR_MODIF;
					v = 0;
				}
				setCtrlData(CTL_LOT_ASSETEXPL, &v);
			}
		}
		setCtrlLong(CTLSEL_LOT_SUPPL, Item.Suppl);
		THROW(P_Pack->BoundsByLot(Item.LotID, &Item, ItemNo, &Rest, 0));
		if((Item.Flags & PPTFR_RECEIPT) || OpTypeID == PPOPT_DRAFTRECEIPT) {
			getCtrlString(CTL_LOT_CLB, temp_buf.Z());
			if(!temp_buf.NotEmptyS() && P_BObj->GetClbNumberByLot(Item.LotID, 0, temp_buf) > 0)
				setCtrlString(CTL_LOT_CLB, temp_buf);
		}
	}
	else
	   	Rest = (Item.Flags & (PPTFR_RECEIPT | PPTFR_UNLIM)) ? Item.Quantity_ : 0.0;
	if(!Item.QCert && Item.GoodsID && !(P_BObj->GetConfig().Flags & BCF_DONTINHQCERT))
		P_Trfr->Rcpt.GetLastQCert(labs(Item.GoodsID), &Item.QCert);
	setCtrlLong(CTLSEL_LOT_QCERT, Item.QCert);
	{
		QCertCtrlGroup::Rec qc_rec(Item.QCert);
		setGroupData(GRP_QCERT, &qc_rec);
	}
	// @v10.1.8 {
	if(Item.Flags & PPTFR_RECEIPT) {
		P_Pack->LTagL.GetNumber(PPTAG_LOT_VETIS_UUID, ItemNo, temp_buf);
		if(temp_buf.NotEmpty() || GObj.CheckFlag(Item.GoodsID, GF_WANTVETISCERT)) {
			showCtrl(CTL_LOT_VETISIND, 1);
			showCtrl(CTL_LOT_VETISMATCHBUTTON, 1);
		}
		else {
			showCtrl(CTL_LOT_VETISIND, 0);
			showCtrl(CTL_LOT_VETISMATCHBUTTON, 0);
		}
		drawCtrl(CTL_LOT_VETISIND);
	}
	else {
		showCtrl(CTL_LOT_VETISIND, 0);
		showCtrl(CTL_LOT_VETISMATCHBUTTON, 0);
	}
	// } @v10.1.8 
	enableCommand(cmQCert, (Item.QCert && Item.LotID) || (Item.Flags & PPTFR_DRAFT));
	setupQuantity(0, 0);
	setCtrlData(CTL_LOT_EXPIRY, &Item.Expiry);
	setCtrlCost(); // Установка COST должна предшествовать установке PRICE (следующая строка)
	if(fl & TISL_ADJPRICE && isDiscountInSum()) {
		setCtrlReal(CTL_LOT_PRICE,  Item.NetPrice());
		setCtrlReal(CTL_LOT_DISCOUNT, 0.0);
	}
	else
		setCtrlData(CTL_LOT_PRICE,  &Item.Price);
	setCtrlLong(CTLSEL_LOT_INTAXGRP, Item.LotTaxGrpID);
	{
		if(Item.Flags & PPTFR_COSTWOVAT) {
			// @v9.2.2 PPGetSubStr(Strings, strNoVAT, temp_buf);
			PPLoadString("novat", temp_buf); // @v9.2.2
		}
		else
			temp_buf.Space();
		setStaticText(CTL_LOT_NOVATTAG, temp_buf);
	}
	if(getCtrlView(CTL_LOT_LOTINFO)) {
		ReceiptTbl::Rec lot_rec;
		if(BillObj->trfr->Rcpt.Search(Item.LotID, &lot_rec) > 0) {
			ReceiptCore::MakeCodeString(&lot_rec, 0, temp_buf);
			setCtrlString(CTL_LOT_LOTINFO, temp_buf);
		}
	}
	SetupInheritedSerial();
	setQuotSign();
	CATCHZOK
	return ok;
}

void TrfrItemDialog::SetupInheritedSerial()
{
	if(getCtrlView(CTL_LOT_SERIAL)) {
		SString serial;
#if 0 // @v8.5.5 {
		if(Item.LotID /* @v8.3.11 && P_Pack && IsIntrExpndOp(P_Pack->Rec.OpID) */)
			P_BObj->GetSerialNumberByLot(Item.LotID, serial, 0);
#endif // } 0 @v8.5.5
		if(Item.LotID) {
			const int inh_serial = BIN(P_BObj->GetConfig().Flags & BCF_INHSERIAL);
			if((P_Pack && IsIntrExpndOp(P_Pack->Rec.OpID)) || (Item.Flags & PPTFR_RECEIPT && inh_serial))
				P_BObj->GetSerialNumberByLot(Item.LotID, serial, 0);
		}
		setCtrlString(CTL_LOT_SERIAL, serial);
	}
}

PPID TrfrItemDialog::GetQuotLocID()
{
	return IsIntrExpndOp(OpID) ? PPObjLocation::ObjToWarehouse(P_Pack->Rec.Object) : P_Pack->Rec.LocID;
}

void TrfrItemDialog::setupQuotation(int reset, int autoQuot)
{
	const PPConfig & r_cfg = LConfig;
	if(reset) {
		if(Item.Flags & PPTFR_QUOT) {
			if(P_BObj->CheckRights(BILLOPRT_CANCELQUOT, 1)) {
				Item.SetupQuot(0.0, 0);
				if(!Item.CurID) {
					disableCtrl(CTL_LOT_PRICE, 0);
					if(!(r_cfg.Flags & CFGFLG_DISCOUNTBYSUM))
						disableCtrl(CTL_LOT_DISCOUNT, 0);
					drawCtrl(CTL_LOT_PRICE);
				}
				else
					disableCtrl(CTL_LOT_CURPRICE, 0);
			}
		}
	}
	else if(Item.GoodsID && oneof3(OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
		double quot = 0.0, dis;
		int    gqr = 0; // @v9.9.2 Результат извлечения котировки (для правильной обработки нулевого значения котировки)
		const  PPID loc_id = GetQuotLocID();
		if(loc_id) {
			quot = Item.Price;
			gqr = P_BObj->SelectQuotKind(P_Pack, &Item, ((autoQuot && P_Pack->AgtQuotKindID) ? 0 : 1), &quot);
			if(gqr > 0) {
				;
			}
			else if(autoQuot) {
				quot = 0.0;
				if(Item.CurID) {
					const QuotIdent qi(loc_id, PPQUOTK_BASE, Item.CurID, P_Pack->Rec.Object);
					gqr = GObj.GetQuot(Item.GoodsID, qi, 0L, 0L, &quot, 1);
					if(gqr <= 0)
						quot = 0.0;
				}
			}
		}
		if(gqr <= 0 && !autoQuot && r_cfg.Flags & CFGFLG_ENABLEFIXDIS && !(Item.Flags & PPTFR_QUOT)) { // @v9.9.2 (quot == 0.0)-->(gqr <= 0)
			if(r_cfg.Flags & CFGFLG_DISCOUNTBYSUM)
				getCtrlData(CTL_LOT_PRICE, &quot);
			else {
				getCtrlData(CTL_LOT_DISCOUNT, &dis);
				quot = Item.Price - dis;
			}
			quot = R2(quot);
		}
		if(quot > 0.0 || (quot == 0.0 && gqr > 0)) {
			PPObjArticle ar_obj;
			PPClientAgreement cliagt;
			// @v9.2.9 {
			//
			// Специальный случай - для отгрузки, привязанной к заказу, принятому из некоторых
			// систем, требуется поправлять конечную цену на величину процентной скидки из заказа
			//
			TransferTbl::Rec ord_item;
			double ord_pct_dis = 0.0;
			ReceiptTbl::Rec ord_lot_rec;
			BillTbl::Rec ord_bill_rec;
			SString edi_channel;
			if(quot > 0.0 && Item.OrdLotID && P_BObj->trfr->Rcpt.Search(Item.OrdLotID, &ord_lot_rec) > 0) {
				if(P_BObj->Fetch(ord_lot_rec.BillID, &ord_bill_rec) > 0 && ord_bill_rec.EdiOp == PPEDIOP_SALESORDER) {
					if(PPRef->Ot.GetTagStr(PPOBJ_BILL, ord_bill_rec.ID, PPTAG_BILL_EDICHANNEL, edi_channel) > 0 && edi_channel.CmpNC("ISALES-PEPSI") == 0) {
						DateIter di;
						if(P_BObj->trfr->EnumByLot(ord_lot_rec.ID, &di, &ord_item) > 0 && ord_item.Flags & PPTFR_RECEIPT) {
							const double ord_qtty = fabs(ord_item.Quantity);
							const double ord_price = fabs(ord_item.Price) * ord_qtty;
							const double ord_dis   = ord_item.Discount * ord_qtty;
							ord_pct_dis = (ord_price > 0.0 && ord_dis > 0.0) ? R4(ord_dis / ord_price) : 0.0;
							if(ord_pct_dis > 0.0)
								quot = R5(quot * (1 - ord_pct_dis));
						}
					}
				}
			}
			// } @v9.2.9
			ar_obj.GetClientAgreement(P_Pack->Rec.Object, &cliagt, 1);
			if(cliagt.Flags & AGTF_PRICEROUNDING) {
				quot = Item.RoundPrice(quot, cliagt.PriceRoundPrec, cliagt.PriceRoundDir,
					(cliagt.Flags & AGTF_PRICEROUNDVAT) ? PPTransferItem::valfRoundVat : 0);
			}
			Item.SetupQuot(quot, 1);
			if(Item.CurID) {
				setCtrlReal(CTL_LOT_CURPRICE, quot);
				evaluateBasePrice(quot, &quot);
			}
			else if(r_cfg.Flags & CFGFLG_DISCOUNTBYSUM)
				setCtrlReal(CTL_LOT_PRICE, quot);
			else
				setCtrlReal(CTL_LOT_DISCOUNT, R2(Item.Price - quot));
			disableCtrls(1, CTL_LOT_CURPRICE, CTL_LOT_DISCOUNT, CTL_LOT_PRICE, 0);
		}
	}
	else if(Item.GoodsID && oneof2(OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
		getCtrlCost();
		double price = getCtrlReal(CTL_LOT_PRICE);
		const PPBillConfig & r_cfg = P_BObj->GetConfig();
		if(r_cfg.ValuationQuotKindID) {
			Item.Price = TR5(price);
			ushort v = 0;
			if(getCtrlData(CTL_LOT_NOVAT, &v))
				SETFLAG(Item.Flags, PPTFR_COSTWOVAT, v);
			if(Item.Valuation(r_cfg, 0, 0) > 0) {
				setCtrlReal(CTL_LOT_PRICE, Item.Price);
				disableCtrl(CTL_LOT_PRICE, 1);
			}
		}
		else if(P_Pack->GetQuotExt(Item, &price) > 0)
			setCtrlReal(CTL_LOT_PRICE, price);
	}
	setQuotSign();
}
//
//
//
IMPL_CMPFUNC(SelLotBrowser_Entry_dt_oprno, i1, i2) { RET_CMPCASCADE2((const SelLotBrowser::Entry *)i1, (const SelLotBrowser::Entry *)i2, Dt, OprNo); }

//static
SArray * SLAPI SelLotBrowser::CreateArray()
{
	SArray * p_ary = new SArray(sizeof(Entry));
	if(!p_ary)
		PPSetErrorNoMem();
	return p_ary;
}

//static
int SLAPI SelLotBrowser::AddItemToArray(SArray * pAry, const ReceiptTbl::Rec * pRec, LDATE billDate, double rest, int onlyWithSerial)
{
	int    ok = 1;
	if(!pAry->lsearch(&pRec->ID, 0, CMPF_LONG, offsetof(Entry, LotID))) {
		PPObjBill * p_bobj = BillObj;
		int    sr = 0;
		SString temp_buf;
		ReceiptTbl::Rec lot_rec = *pRec;
		Entry  entry;
		MEMSZERO(entry);
		entry.LotID = lot_rec.ID;
		entry.Dt    = lot_rec.Dt;
		if(billDate)
			THROW(p_bobj->trfr->GetLotPrices(&lot_rec, billDate));
		entry.Cost  = p_bobj->CheckRights(BILLRT_ACCSCOST) ? lot_rec.Cost : 0;
		entry.Price = lot_rec.Price;
		entry.Qtty  = lot_rec.Quantity; // @v10.1.0
		entry.Rest  = rest;
		entry.GoodsID = labs(lot_rec.GoodsID);
		entry.Expiry  = lot_rec.Expiry;
		entry.SupplID = lot_rec.SupplID; // @v9.3.7
		sr = p_bobj->GetSerialNumberByLot(lot_rec.ID, temp_buf, 1);
		temp_buf.CopyTo(entry.Serial, sizeof(entry.Serial));
		{
			PPObjGoods goods_obj;
			goods_obj.FetchSingleBarcode(labs(lot_rec.GoodsID), temp_buf);
			STRNSCPY(entry.Barcode, temp_buf);
		}
		if(!onlyWithSerial || sr > 0)
			THROW_SL(pAry->insert(&entry));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

//static
int SelLotBrowser::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	SelLotBrowser * p_brw = (SelLotBrowser *)pBlk->ExtraPtr;
	return p_brw ? p_brw->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI SelLotBrowser::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		SString temp_buf;
		const  SelLotBrowser::Entry * p_item = (const SelLotBrowser::Entry *)pBlk->P_SrcData;
		{
			ok = 1;
			AryBrowserDef * p_def = (AryBrowserDef *)getDef();
			const SArray * p_list = p_def ? (const SArray *)p_def->getArray() : 0;
			double real_val = 0.0;
			Goods2Tbl::Rec goods_rec;
			void * p_dest = pBlk->P_DestData;
			switch(pBlk->ColumnN) {
				case 0: pBlk->Set(p_item->LotID); break; // LotID
				case 1: pBlk->Set(p_item->Dt);    break; // Lot date
				case 2: // Supplier Name
					temp_buf.Z();
					if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1))
						GetArticleName(p_item->SupplID, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 3: pBlk->Set(p_item->Rest); break; // Rest
				case 4: // Unit Name
					temp_buf.Z();
					if(GObj.Fetch(p_item->GoodsID, &goods_rec) > 0) {
						PPUnit unit_rec;
						if(GObj.FetchUnit(goods_rec.UnitID, &unit_rec) > 0)
							temp_buf = unit_rec.Name;
					}
					pBlk->Set(temp_buf);
					break;
				case 5: pBlk->Set(p_item->Serial); break; // Serial
				case 6: pBlk->Set(p_item->Cost);   break; // Cost
				case 7: pBlk->Set(p_item->Price);  break; // Price
				case 8: pBlk->Set(p_item->Expiry); break; // Expiry
				case 9: // Goods Name
					GetGoodsName(p_item->GoodsID, temp_buf);
					pBlk->Set(temp_buf);
					break;
				case 10: // Alco code of lot
					{
						PPObjTag tag_obj;
						ObjTagItem tag_item;
						temp_buf.Z();
						if(tag_obj.FetchTag(p_item->LotID, PPTAG_LOT_FSRARLOTGOODSCODE, &tag_item) > 0)
							tag_item.GetStr(temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				case 11: // Alco REF-A
					{
						PPObjTag tag_obj;
						ObjTagItem tag_item;
						temp_buf.Z();
						if(tag_obj.FetchTag(p_item->LotID, PPTAG_LOT_FSRARINFA, &tag_item) > 0)
							tag_item.GetStr(temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				case 12: // Alco REF-B
					{
						PPObjTag tag_obj;
						ObjTagItem tag_item;
						temp_buf.Z();
						if(tag_obj.FetchTag(p_item->LotID, PPTAG_LOT_FSRARINFB, &tag_item) > 0)
							tag_item.GetStr(temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				case 13: pBlk->Set(p_item->Barcode); break; // Barcode
				case 14: pBlk->Set(p_item->Qtty); break; // Quantity
				case 15: // Phisycal quantity
					{
						double phuperu = 0.0;
						GObj.GetPhUPerU(p_item->GoodsID, 0, &phuperu);
						pBlk->Set(p_item->Qtty * phuperu); 
					}
					break;
				case 16: // VETIS UUID лота
					{
						PPObjTag tag_obj;
						ObjTagItem tag_item;
						temp_buf.Z();
						if(tag_obj.FetchTag(p_item->LotID, PPTAG_LOT_VETIS_UUID, &tag_item) > 0)
							tag_item.GetStr(temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				default:
					ok = 0;
			}
		}
	}
	return ok;
}

SelLotBrowser::SelLotBrowser(PPObjBill * pBObj, SArray * pAry, uint pos, long flags) : BrowserWindow(BROWSER_SELECTLOT, pAry), State(0), Flags(flags)
{
	PPID   single_goods_id = 0;
	SString single_serial;
	SString temp_buf;
	if(pAry) {
		State |= stNoSerials;
		for(uint i = 0; i < pAry->getCount(); i++) {
			const Entry * p_entry = (const Entry *)pAry->at(i);
			if(!(State & stMultipleGoods) && p_entry->GoodsID) {
				if(!single_goods_id)
					single_goods_id = labs(p_entry->GoodsID);
				else if(labs(p_entry->GoodsID) != single_goods_id) {
					single_goods_id = 0;
					State |= stMultipleGoods;
				}
			}
			temp_buf = p_entry->Serial;
			if(temp_buf.NotEmptyS()) {
				State &= ~stNoSerials;
				if(!(State & stMultipleSerial)) {
					if(single_serial.Empty())
						single_serial = temp_buf;
					else if(single_serial.CmpNC(temp_buf) != 0) {
						single_serial = 0;
						State |= stMultipleSerial;
					}
				}
			}
		}
		{
			int    at_pos = 0;
			if(State & stMultipleGoods) {
				PPLoadString("ware", temp_buf);
				insertColumn(at_pos++, temp_buf, 9, MKSTYPE(S_ZSTRING, 128), 0, BCO_CAPLEFT|BCO_USERPROC);
			}
			at_pos += 4; // @v10.1.3
			if(Flags & fShowBarcode) {
				PPLoadString("barcode", temp_buf);
				insertColumn(at_pos++, temp_buf, 13, MKSTYPE(S_ZSTRING, 20), 0, BCO_CAPLEFT|BCO_USERPROC);
			}
			if(!(State & stNoSerials)) {
				PPLoadString("serial", temp_buf);
				insertColumn(at_pos++, temp_buf, 5, MKSTYPE(S_ZSTRING, 32), 0, BCO_CAPLEFT|BCO_USERPROC);
			}
			{
				at_pos += 2;
				if(Flags & fShowQtty) {
					PPLoadString("qtty", temp_buf);
					insertColumn(at_pos++, temp_buf, 14, T_DOUBLE, MKSFMTD(0, 3, 0), BCO_CAPRIGHT|BCO_USERPROC);
				}
				if(Flags & fShowPhQtty) {
					PPLoadString("phqtty", temp_buf);
					insertColumn(at_pos++, temp_buf, 15, T_DOUBLE, MKSFMTD(0, 3, 0), BCO_CAPRIGHT|BCO_USERPROC);
				}
			}
		}
		if(Flags & fShowEgaisTags) {
			PPLoadString("egaiscode", temp_buf);
			insertColumn(-1, temp_buf, 10, MKSTYPE(S_ZSTRING, 20), 0, BCO_CAPLEFT|BCO_USERPROC);
			PPLoadString("egaisrefa", temp_buf);
			insertColumn(-1, temp_buf, 11, MKSTYPE(S_ZSTRING, 20), 0, BCO_CAPLEFT|BCO_USERPROC);
			PPLoadString("egaisrefb", temp_buf);
			insertColumn(-1, temp_buf, 12, MKSTYPE(S_ZSTRING, 20), 0, BCO_CAPLEFT|BCO_USERPROC);
		}
		if(Flags & fShowVetisTag) {
			PPLoadString("rtag_lotvetisuuid", temp_buf);
			insertColumn(-1, temp_buf, 16, MKSTYPE(S_ZSTRING, 40), 0, BCO_CAPLEFT|BCO_USERPROC);
		}
	}
	P_BObj = pBObj;
	setInitPos(pos);
	if(single_goods_id) {
		SString goods_name;
		setSubTitle(GetGoodsName(single_goods_id, goods_name));
	}
	else if(single_serial.NotEmptyS())
		setSubTitle(single_serial);
	BrowserDef * p_def = getDef();
	CALLPTRMEMB(p_def, SetUserProc(SelLotBrowser::GetDataForBrowser, this));
}

IMPL_HANDLE_EVENT(SelLotBrowser)
{
	const SelLotBrowser::Entry * p_entry = 0;
	BrowserWindow::handleEvent(event);
	if(event.isCmd(cmaEdit)) {
		if(IsInState(sfModal))
			endModal(cmOK);
	}
	else if(event.isKeyDown(kbF3)) {
		if(P_BObj->CheckRights(BILLRT_ACCSCOST)) {
			p_entry = (const SelLotBrowser::Entry *)view->getCurItem();
			if(p_entry && p_entry->LotID)
				ViewOpersByLot(p_entry->LotID, 0);
		}
	}
	else if(event.isKeyDown(kbF4)) {
		p_entry = (const SelLotBrowser::Entry *)view->getCurItem();
		if(p_entry && p_entry->LotID)
			P_BObj->EditLotExtData(p_entry->LotID);
	}
	else if(event.isKeyDown(kbCtrlF6)) {
		p_entry = (const SelLotBrowser::Entry *)view->getCurItem();
		if(p_entry && p_entry->LotID)
			EditObjTagValList(PPOBJ_LOT, p_entry->LotID, 0);
	}
	else if(event.isKeyDown(kbCtrlF3)) {
		p_entry = (const SelLotBrowser::Entry *)view->getCurItem();
		if(p_entry)
			P_BObj->EditLotSystemInfo(p_entry->LotID);
	}
	else
		return;
	clearEvent(event);
}

int TrfrItemDialog::addLotEntry(SArray * pAry, const ReceiptTbl::Rec * pLotRec)
{
	double rest = 0.0;
	return BIN(P_Pack->BoundsByLot(pLotRec->ID, &Item, ItemNo, &rest, 0) && SelLotBrowser::AddItemToArray(pAry, pLotRec, Item.Date, rest));
}

void TrfrItemDialog::selectLot()
{
	SelLotBrowser::Entry * p_sel;
	int    r, found = 0;
	uint   s = 0;
	PPID   op_id = P_Pack->Rec.OpID;
	DateIter diter;
	SArray * p_ary = 0;
	SelLotBrowser * p_brw = 0;
	PPTransferItem save_item = Item;
	ReceiptTbl::Rec * p_lot_rec = &P_Trfr->Rcpt.data;
	getCtrlData(CTLSEL_LOT_GOODS, &Item.GoodsID);
	if(!(Item.Flags & PPTFR_AUTOCOMPL)) {
		THROW(p_ary = SelLotBrowser::CreateArray());
		diter.Init(0, Item.Date);
		while((r = P_Trfr->Rcpt.EnumLots(Item.GoodsID, Item.LocID, &diter)) > 0) {
			//
			// Для некоторых операций отбираем только те лоты, которые поступили от специфицированного поставщика
			//
			if(p_lot_rec->SupplID == P_Pack->Rec.Object || (op_id != _PPOPK_SUPPLRET && (P_Pack->OpTypeID != PPOPT_GOODSREVAL || !P_Pack->Rec.Object))) {
				THROW(addLotEntry(p_ary, p_lot_rec));
				if(p_lot_rec->ID == Item.LotID) {
					s = p_ary->getCount() - 1;
					found = 1;
				}
			}
		}
		THROW(r);
		if(!found) {
			if(Item.LotID && !(Item.Flags & PPTFR_ORDER)) {
				THROW(P_Trfr->Rcpt.Search(Item.LotID) > 0);
				found = 1;
			}
			else if(Item.Flags & PPTFR_ORDER) {
				if(P_Trfr->Rcpt.GetLastLot(Item.GoodsID, Item.LocID, Item.Date, 0) > 0)
					found = 1;
				else if(P_Trfr->Rcpt.GetLastLot(Item.GoodsID, 0L, Item.Date, 0) > 0)
					found = 1;
			}
			if(found && !p_ary->lsearch(&p_lot_rec->ID, 0, CMPF_LONG, offsetof(SelLotBrowser::Entry, LotID))) {
				THROW(addLotEntry(p_ary, p_lot_rec));
				s = p_ary->getCount() - 1;
			}
		}
		THROW_MEM(p_brw = new SelLotBrowser(P_BObj, p_ary, s, 0));
		if(ExecView(p_brw) == cmOK && (p_sel = (SelLotBrowser::Entry *)p_brw->view->getCurItem()) != 0)
			if(p_sel->LotID != Item.LotID && !(Item.Flags & (PPTFR_RECEIPT|PPTFR_CORRECTION))) {
				Item.LotID = p_sel->LotID;
				Item.Cost  = 0.0;
				Item.Price = 0.0;
				Item.Rest_ = 0.0;
				Rest = 0.0;
				Item.Expiry = ZERODATE;
				if(P_Pack->OpTypeID == PPOPT_GOODSREVAL)
					Item.RevalCost = Item.Discount = 0.0;
				THROW(setupLot());
				setupRest();
			}
	}
	CATCH
		if(p_brw == 0)
			delete p_ary;
		PPError();
		Item = save_item;
	ENDCATCH
	delete p_brw;
}
//
//
//
int SLAPI PPObjBill::SelectLot2(SelectLotParam & rParam)
{
	PPID     lotid = 0;
	SelLotBrowser::Entry * p_sel;
	int      r = -1;
	DateIter diter;
	SArray * p_ary = 0;
	SString serial;
	ReceiptTbl::Rec lot_rec;
	SelLotBrowser * p_brw = 0;
	for(uint gidx = 0; gidx < rParam.GoodsList.getCount(); gidx++) {
		Goods2Tbl::Rec goods_rec;
		const PPID goods_id = rParam.GoodsList.get(gidx);
		if(goods_id && GObj.Fetch(labs(goods_id), &goods_rec) > 0) {
			uint   s = 0;
			THROW(SETIFZ(p_ary, SelLotBrowser::CreateArray()));
			diter.Init(&rParam.Period);
			while(1) {
				r = rParam.LocID ? trfr->Rcpt.EnumLots(goods_id, rParam.LocID, &diter, &lot_rec) : trfr->Rcpt.EnumByGoods(goods_id, &diter, &lot_rec);
				if(r > 0) {
					int    do_skip = 0;
					if(lot_rec.ID != rParam.ExcludeLotID) {
						if(goods_id >= 0 || (lot_rec.Rest > 0.0 || (rParam.Flags & rParam.fEnableZeroRest))) {
							if(rParam.Flags & rParam.fNotEmptySerial) {
								GetSerialNumberByLot(lot_rec.ID, serial = 0, 1);
								if(!serial.NotEmptyS())
									do_skip = 1;
							}
							if(!do_skip)
								THROW(r = SelLotBrowser::AddItemToArray(p_ary, &lot_rec, ZERODATE, lot_rec.Rest));
						}
					}
				}
				else {
					THROW(r);
					if(rParam.RetLotID) {
						if(trfr->Rcpt.Search(rParam.RetLotID, &lot_rec) > 0 && lot_rec.ID != rParam.ExcludeLotID) {
							if(goods_id >= 0 || (lot_rec.Rest > 0.0 || (rParam.Flags & rParam.fEnableZeroRest))) {
								THROW(r = SelLotBrowser::AddItemToArray(p_ary, &lot_rec, ZERODATE, lot_rec.Rest));
							}
						}
					}
					break;
				}
			}
		}
	}
	if(rParam.AddendumLotList.getCount()) {
		THROW(SETIFZ(p_ary, SelLotBrowser::CreateArray()));
		for(uint i = 0; i < rParam.AddendumLotList.getCount(); i++) {
            const PPID addendum_lot_id = rParam.AddendumLotList.get(i);
            if(trfr->Rcpt.Search(addendum_lot_id, &lot_rec) > 0 && lot_rec.ID != rParam.ExcludeLotID) {
				THROW(r = SelLotBrowser::AddItemToArray(p_ary, &lot_rec, ZERODATE, lot_rec.Rest));
            }
		}
	}
	if(p_ary) {
		uint   s = 0;
		p_ary->sort(PTR_CMPFUNC(SelLotBrowser_Entry_dt_oprno));
		if(rParam.RetLotID) {
			if(p_ary->lsearch(&rParam.RetLotID, &(s = 0), CMPF_LONG, offsetof(SelLotBrowser::Entry, LotID)))
				s++;
			else
				s = 0;
		}
		// @v9.4.5 {
		if(!s && rParam.RetLotSerial.NotEmpty()) {
			for(uint i = 0; !s && i < p_ary->getCount(); i++) {
				const SelLotBrowser::Entry * p_entry = (const SelLotBrowser::Entry *)p_ary->at(i);
				if(rParam.RetLotSerial.CmpNC(p_entry->Serial) == 0)
					s = i+1;
			}
		}
		// } @v9.4.5
		if(s)
			s--;
		long   slb_flags = 0;
		if(rParam.Flags & rParam.fShowEgaisTags) 
			slb_flags |= SelLotBrowser::fShowEgaisTags;
		if(rParam.Flags & rParam.fShowBarcode) 
			slb_flags |= SelLotBrowser::fShowBarcode;
		if(rParam.Flags & rParam.fShowQtty) 
			slb_flags |= SelLotBrowser::fShowQtty;
		if(rParam.Flags & rParam.fShowPhQtty) 
			slb_flags |= SelLotBrowser::fShowPhQtty;
		if(rParam.Flags & rParam.fShowVetisTag) 
			slb_flags |= SelLotBrowser::fShowVetisTag;
		THROW_MEM(p_brw = new SelLotBrowser(this, p_ary, s, slb_flags));
		if(rParam.Title.NotEmpty())
			p_brw->setTitle(rParam.Title);
		if(ExecView(p_brw) == cmOK) {
			if((p_sel = (SelLotBrowser::Entry *)p_brw->view->getCurItem()) != 0) {
				lotid = p_sel->LotID;
				GetSerialNumberByLot(lotid, serial = 0, 1);
				if(rParam.Flags & rParam.fFillLotRec && lotid)
					r = (trfr->Rcpt.Search(lotid, &rParam.RetLotRec) > 0) ? 1 : -1;
				else
					r = 1;
				(rParam.RetLotSerial = p_sel->Serial).Strip();
				if(rParam.Flags & rParam.fNotEmptySerial && rParam.RetLotSerial.Empty())
					r = -1;
			}
		}
	}
	CATCH
		r = 0;
		if(p_brw == 0)
			delete p_ary;
		PPError();
	ENDCATCH
	delete p_brw;
	rParam.RetLotID = lotid;
	return r;
}

#if 0 // @v9.3.6 {
int SLAPI SelectLot__(PPID locID, PPID goodsID, PPID excludeLotID, PPID * pLotID, ReceiptTbl::Rec * pRec)
{
	PPObjBill * p_bobj = BillObj;
	PPID     lotid = 0;
	SelLotBrowser::Entry * p_sel;
	int      r = -1;
	DateIter diter;
	SArray * p_ary = 0;
	SelLotBrowser * p_brw = 0;
	ReceiptCore & rcpt  = p_bobj->trfr->Rcpt;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(goodsID && goods_obj.Fetch(labs(goodsID), &goods_rec) > 0) {
		uint   s = 0;
		THROW(p_ary = SelLotBrowser::CreateArray());
		diter.Init();
		while(1) {
			ReceiptTbl::Rec lot_rec;
			r = locID ? rcpt.EnumLots(goodsID, locID, &diter, &lot_rec) : rcpt.EnumByGoods(goodsID, &diter, &lot_rec);
			if(r > 0) {
				if(lot_rec.ID != excludeLotID) {
					if(goodsID >= 0 || lot_rec.Rest > 0.0)
						THROW(r = SelLotBrowser::AddItemToArray(p_ary, &lot_rec, ZERODATE, lot_rec.Rest));
				}
			}
			else {
				THROW(r);
				if(pLotID && *pLotID)
					if(rcpt.Search(*pLotID, &lot_rec) > 0 && lot_rec.ID != excludeLotID) {
						if(goodsID >= 0 || lot_rec.Rest > 0.0)
							THROW(r = SelLotBrowser::AddItemToArray(p_ary, &lot_rec, ZERODATE, lot_rec.Rest));
					}
				break;
			}
		}
		p_ary->sort(PTR_CMPFUNC(SelLotBrowser_Entry_dt_oprno));
		if(pLotID && *pLotID)
			if(!p_ary->lsearch(pLotID, &(s = 0), CMPF_LONG, offsetof(SelLotBrowser::Entry, LotID)))
				s = 0;
		THROW_MEM(p_brw = new SelLotBrowser(p_bobj, p_ary, s, goodsID, 0));
		if(ExecView(p_brw) == cmOK)
			if((p_sel = (SelLotBrowser::Entry *)p_brw->view->getCurItem()) != 0) {
				lotid = p_sel->LotID;
				if(pRec && lotid)
					r = (rcpt.Search(lotid, pRec) > 0) ? 1 : -1;
				else
					r = 1;
			}
	}
	CATCH
		r = 0;
		if(p_brw == 0)
			delete p_ary;
		PPError();
	ENDCATCH
	delete p_brw;
	ASSIGN_PTR(pLotID, lotid);
	return r;
}
#endif // } 0 @v9.3.6
